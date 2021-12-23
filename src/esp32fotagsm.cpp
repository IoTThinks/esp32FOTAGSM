/*
   esp32 firmware OTA
   Date: December 2018
   Purpose: Perform an OTA update from a bin located on a webserver (HTTP Only)
*/

#include "esp32fotagsm.h"
#include "Arduino.h"
#include <Update.h>
#include "ArduinoJson.h"
#include "esp_log.h"

#define CLIENT_TIMEOUT_MS (60000)
#define DOWNLOAD_CHUNK_SIZE (4096)

esp32FOTAGSM::esp32FOTAGSM(Client &client, String firwmareType, int firwmareVersion)
{
    this->setClient(client);
    _firwmareType = firwmareType;
    _firwmareVersion = firwmareVersion;
    useDeviceID = false;
}

static void splitHeader(String src, String &header, String &headerValue)
{
    int idx = 0;

    idx = src.indexOf(':');
    header = src.substring(0, idx);
    headerValue = src.substring(idx + 1, src.length());
    headerValue.trim();

    return;
}

// OTA Logic
bool esp32FOTAGSM::execOTA()
{
    unsigned long timeout = 0;
    int contentLength = 0;
    bool isValidContentType = false;
    bool Accept_Ranges_bytes = false;
    bool gotHTTPStatus = false;

    size_t written = 0;

    ESP_LOGD(TAG, "Connecting to: %S", _host.c_str());

    _client->setTimeout(CLIENT_TIMEOUT_MS);

    // Connect to Webserver
    if (_client->connect(_host.c_str(), _port))
    {

        // Connection Succeed.
        // Fetching the bin HEAD
        Serial.println("Fetching Bin HEAD: " + String(_bin));

        // Get the contents of the bin file
        _client->print(String("HEAD ") + _bin + " HTTP/1.1\r\n" +
                       "Host: " + _host + "\r\n" +
                       "Cache-Control: no-cache\r\n" +
                       "Connection: close\r\n\r\n");

        timeout = millis();
        while (_client->available() == 0)
        {
            if (millis() - timeout > CLIENT_TIMEOUT_MS)
            {
                ESP_LOGD(TAG, "Client Timeout !");
                _client->stop();
                return false;
            }
        }

        while (_client->available())
        {
            String header, headerValue;
            // read line till /n
            String line = _client->readStringUntil('\n');

            ESP_LOGD(TAG, "Header line: %s", line.c_str());

            // remove space, to check if the line is end of headers
            line.trim();

            if (!line.length())
            {
                //headers ended
                break; // and get the OTA started
            }

            // Check if the HTTP Response is 200
            // else break and Exit Update
            if (line.startsWith("HTTP/1.1"))
            {
                if (line.indexOf("200") < 0)
                {
                    ESP_LOGE(TAG, "Got a non 200 status code from server. Exiting OTA Update.");
                    _client->stop();
                    break;
                }
                gotHTTPStatus = true;
            }

            if (false == gotHTTPStatus)
            {
                continue;
            }

            splitHeader(line, header, headerValue);

            // extract headers here
            // Content-Length
            if (header.equalsIgnoreCase("Content-Length"))
            {
                contentLength = headerValue.toInt();
                Serial.println("Content-Length: " + String(contentLength));
            }
            // Content-type
            else if (header.equalsIgnoreCase("Content-type"))
            {
                String contentType = headerValue;
                Serial.println("Got " + contentType + " payload.");
                if (contentType == "application/octet-stream")
                {
                    Serial.println("Content-Length: " + String(contentLength));
                    isValidContentType = true;
                }
            }
            // Accept-Ranges
            else if (header.equalsIgnoreCase("Accept-Ranges"))
            {
                String contentType = headerValue;
                Serial.println("Got " + contentType + " payload.");
                if (contentType == "bytes")
                {
                    Serial.println("Server supports range requests");
                    Accept_Ranges_bytes = true;
                }
            }
        }
    }
    else
    {
        Serial.println("Connection to " + String(_host) + " failed!");
        return false;
    }

    // We will open a new connection to the server later
    _client->stop();

    // check contentLength and content type
    if (contentLength && isValidContentType)
    {

        // Check if there is enough to OTA Update.
        if (Update.begin(contentLength))
        {
            ESP_LOGD(TAG, "OTA file can be downloaded.");

            // Setup Update onProgress callback
            Update.onProgress(
                [](unsigned int progress, unsigned int total)
                {
                    ESP_LOGI(TAG, "Update Progress: %u of %u", progress, total);
                });

            if (Accept_Ranges_bytes)
            {
                Serial.println("OTA file will be downloaded in chunks");

                // Number of chunks
                int numChunks = contentLength / DOWNLOAD_CHUNK_SIZE;
                if (contentLength % DOWNLOAD_CHUNK_SIZE)
                {
                    numChunks++;
                }

                uint chunk_first_byte = 0;
                uint chunk_last_byte = DOWNLOAD_CHUNK_SIZE - 1;
                uint remainig_bytes = contentLength;
                uint8_t chunk_buffer[DOWNLOAD_CHUNK_SIZE];

                while (remainig_bytes > 0)
                {
                    if(!_client->connected()){
                        ESP_LOGE(TAG, "Client Disconnected");
                        
                        // Connect to Webserver
                        if (_client->connect(_host.c_str(), _port))
                        {
                            ESP_LOGD(TAG, "client connected");
                        }
                        else
                        {
                            Serial.println("Connection to " + String(_host) + " failed! Retrying in 5 seconds");
                            delay(5000);
                            continue;
                        }
                    }
                    else{
                        ESP_LOGD(TAG, "Client Connected");
                        ESP_LOGD(TAG, "Downloading chunk from %u to %u, remaining bytes: %u", chunk_first_byte, chunk_last_byte, remainig_bytes);

                        if(remainig_bytes < DOWNLOAD_CHUNK_SIZE){
                            chunk_last_byte = chunk_first_byte + remainig_bytes - 1;
                        }

                        _client->flush();
                        // Get the contents of the bin file
                        _client->print(String("GET ") + _bin + " HTTP/1.1\r\n" +
                                    "Host: " + _host + "\r\n" +
                                    "Cache-Control: no-cache\r\n" +
                                    "Range: bytes=" + String(chunk_first_byte) + "-" + String(chunk_last_byte) + "\r\n" +
                                    "Connection: keep-alive\r\n\r\n");

                        timeout = millis();
                        while (_client->available() == 0)
                        {
                            if (millis() - timeout > CLIENT_TIMEOUT_MS)
                            {
                                ESP_LOGD(TAG, "Client Timeout !");
                                _client->stop();
                                return false;
                            }
                        }

                        // Read the headers
                        while (_client->available())
                        {
                            String header, headerValue;
                            // read line till /n
                            String line = _client->readStringUntil('\n');

                            ESP_LOGV(TAG, "Header line: %s", line.c_str());

                            // remove space, to check if the line is end of headers
                            line.trim();

                            if (!line.length())
                            {
                                ESP_LOGV(TAG, "Headers ended. Get the payload");
                                break;
                            }
                        }

                        // Read the payload
                        size_t readed_bytes = _client->readBytes(chunk_buffer, chunk_last_byte - chunk_first_byte + 1);
                        ESP_LOGD(TAG, "Readed %u bytes from payload", readed_bytes);

                        // Write chunk to flash
                        written += Update.write(chunk_buffer, readed_bytes);
                        ESP_LOGD(TAG, "Written %u bytes to flash", written);

                        remainig_bytes -= readed_bytes;

                        /*
                        @TODO check if written == readed_bytes. If not, we should retry the chunk download, 
                        adjusting the chunk_last_byte and chunk_first_byte acordingly.
                        */
                        // Increment chunk
                        chunk_first_byte += DOWNLOAD_CHUNK_SIZE;
                        chunk_last_byte += DOWNLOAD_CHUNK_SIZE;
                    }
                }
            }
            else
            {
                Serial.println("OTA file will be downloaded in one go");
                _client->flush();

                // Connect to Webserver
                if (_client->connect(_host.c_str(), _port))
                {
                    // Get the contents of the bin file
                    _client->print(String("GET ") + _bin + " HTTP/1.1\r\n" +
                                "Host: " + _host + "\r\n" +
                                "Cache-Control: no-cache\r\n" +
                                "Connection: close\r\n\r\n");

                    timeout = millis();
                    while (_client->available() == 0)
                    {
                        if (millis() - timeout > CLIENT_TIMEOUT_MS)
                        {
                            ESP_LOGD(TAG, "Client Timeout !");
                            _client->stop();
                            return false;
                        }
                    }
                    while (_client->available())
                    {
                        String header, headerValue;
                        // read line till /n
                        String line = _client->readStringUntil('\n');

                        ESP_LOGD(TAG, "Header line: %s", line.c_str());

                        // remove space, to check if the line is end of headers
                        line.trim();

                        if (!line.length())
                        {
                            ESP_LOGD(TAG, "Headers ended. Get the OTA started");
                            break;
                        }
                    }

                    ESP_LOGD(TAG, "Begin OTA. This may take several minutes to complete. Patience!");
                    written = Update.writeStream(*_client);
                }
                else
                {
                    Serial.println("Connection to " + String(_host) + " failed!");
                    return false;
                }
                
            }

            if (written == contentLength)
            {
                Serial.println("Written : " + String(written) + " successfully");
            }
            else
            {
                Serial.println("Written only : " + String(written) + " of " + String(contentLength) + " OTA will not proceed. ");
            }

            if (Update.end())
            {
                Serial.println("OTA done!");
                if (Update.isFinished())
                {
                    Serial.println("Update successfully completed. Rebooting.");
                    ESP.restart();
                }
                else
                {
                    Serial.println("Update not finished? Something went wrong!");
                }
            }
            else
            {
                Serial.println("Error Occurred. Error #: " + String(Update.getError()) + String(Update.errorString()));
            }
        }
        else
        {
            // not enough space to begin OTA
            // Understand the partitions and
            // space availability
            ESP_LOGE(TAG, "Not enough space to begin OTA");
            _client->flush();

            return false;
        }
    }
    else
    {
        ESP_LOGE(TAG, "There was no content in the response or the content type was not application/octet-stream");
        _client->flush();

        return false;
    }
}

bool esp32FOTAGSM::execHTTPcheck()
{
    int contentLength = 0;
    bool isValidContentType = false;
    bool gotHTTPStatus = false;

    String useURL;

    if (useDeviceID)
    {
        useURL = checkRESOURCE + "?id=" + getDeviceID();
    }
    else
    {
        useURL = checkRESOURCE;
    }

    _port = 80;

    Serial.println("Getting HTTP");
    Serial.println(useURL);
    Serial.println("------");

    //current connection status should be checked before calling this function

    _client->setTimeout(CLIENT_TIMEOUT_MS);

    if (_client->connect(checkHOST.c_str(), checkPORT))
    {
        // Connection Succeed.
        // Fetching the bin
        Serial.println("Fetching JSON:" + useURL);

        // Get the contents of the bin file
        _client->print(String("GET ") + checkRESOURCE + " HTTP/1.1\r\n" +
                       "Host: " + checkHOST + "\r\n" +
                       "Cache-Control: no-cache\r\n" +
                       "Connection: close\r\n\r\n");

        unsigned long timeout = millis();
        while (_client->available() == 0)
        {
            if (millis() - timeout > CLIENT_TIMEOUT_MS)
            {
                ESP_LOGD(TAG, "Client Timeout !");
                _client->stop();
                return false;
            }
        }

        while (_client->available())
        {
            String header, headerValue;
            // read line till /n
            String line = _client->readStringUntil('\n');

            ESP_LOGD(TAG, "Header line: %s", line.c_str());

            // remove space, to check if the line is end of headers
            line.trim();

            if (!line.length())
            {
                //headers ended
                break; // and get the OTA started
            }

            // Check if the HTTP Response is 200
            // else break and Exit Update
            if (line.startsWith("HTTP/1.1"))
            {
                if (line.indexOf("200") < 0)
                {
                    Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
                    _client->stop();
                    break;
                }
                gotHTTPStatus = true;
            }

            if (false == gotHTTPStatus)
            {
                continue;
            }

            splitHeader(line, header, headerValue);

            // extract headers here
            // Start with content length
            if (header.equalsIgnoreCase("Content-Length"))
            {
                contentLength = headerValue.toInt();
                Serial.println("Got " + String(contentLength) + " bytes from server");
                continue;
            }

            // Next, the content type
            if (header.equalsIgnoreCase("Content-type"))
            {
                String contentType = headerValue;
                Serial.println("Got " + contentType + " payload.");
                if (contentType == "application/json")
                {
                    isValidContentType = true;
                }
            }
        }

        // Check what is the contentLength and if content type is `application/octet-stream`
        Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

        // check if the contectLength is bigger than the buffer size
        if (contentLength > 256)
        {
            Serial.println("contentLength is bigger than 256 bytes. Exiting Update check.");
            return false;
        }

        // check contentLength and content type
        if (contentLength && isValidContentType)
        {
            char JSONMessage[256];
            _client->readBytes(JSONMessage, contentLength);

            StaticJsonDocument<300> JSONDocument; //Memory pool
            DeserializationError err = deserializeJson(JSONDocument, JSONMessage);

            if (err)
            { //Check for errors in parsing
                Serial.println("Parsing failed");
                delay(5000);
                return false;
            }

            const char *pltype = JSONDocument["type"];
            int plversion = JSONDocument["version"];
            const char *plhost = JSONDocument["host"];
            _port = JSONDocument["port"];
            const char *plbin = JSONDocument["bin"];

            String jshost(plhost);
            String jsbin(plbin);

            _host = jshost;
            _bin = jsbin;

            String fwtype(pltype);

            Serial.println(plhost);
            Serial.println(plbin);
            Serial.println(fwtype);

            if (plversion > _firwmareVersion && fwtype == _firwmareType)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            Serial.println("There was no content in the response");
            _client->flush();
        }
    }
    else
    {
        // Connect to webserver failed
        Serial.println("Connection to " + String(checkHOST) + " failed.");
        return false;
    }
}

String esp32FOTAGSM::getDeviceID()
{
    char deviceid[21];
    uint64_t chipid;
    chipid = ESP.getEfuseMac();
    sprintf(deviceid, "%" PRIu64, chipid);
    String thisID(deviceid);
    return thisID;
}

// Force a firmware update regartless on current version
void esp32FOTAGSM::forceUpdate(String firmwareHost, int firmwarePort, String firmwarePath)
{
    _host = firmwareHost;
    _bin = firmwarePath;
    _port = firmwarePort;
    execOTA();
}

void esp32FOTAGSM::setClient(Client &client)
{
    this->_client = &client;
}
