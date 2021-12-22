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
void esp32FOTAGSM::execOTA()
{

    int contentLength = 0;
    bool isValidContentType = false;
    bool gotHTTPStatus = false;

    ESP_LOGD(TAG, "Connecting to: ", _host.c_str());

    _client->setTimeout(60000); // Kevin, muchlonger wait

    // Connect to Webserver
    if (_client->connect(_host.c_str(), _port))
    {

        // Connection Succeed.
        // Fetching the bin
        Serial.println("Fetching Bin: " + String(_bin));

        // Get the contents of the bin file
        _client->print(String("GET ") + _bin + " HTTP/1.1\r\n" +
                       "Host: " + _host + "\r\n" +
                       "Cache-Control: no-cache\r\n" +
                       "Connection: close\r\n\r\n");

        unsigned long timeout = millis();
        while (_client->available() == 0)
        {
            // if (millis() - timeout > 5000) // CHANGE
            if (millis() - timeout > 60000) // Kevin: More timeout
            {
                Serial.println("Client Timeout !");
                _client->stop();
                return;
            }
        }

        while (_client->available())
        {
            String header, headerValue;
            // read line till /n
            String line = _client->readStringUntil('\n');
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
                if (contentType == "application/octet-stream")
                {
                    isValidContentType = true;
                }
            }
        }
        // Check what is the contentLength and if content type is `application/octet-stream`
        Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

        // check contentLength and content type
        if (contentLength && isValidContentType)
        {
            // Check if there is enough to OTA Update
            bool canBegin = Update.begin(contentLength);

            // If yes, begin
            if (canBegin)
            {
                Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
                // No activity would appear on the Serial monitor
                // So be patient. This may take 2 - 5mins to complete
                size_t written = Update.writeStream(*_client);

                if (written == contentLength)
                {
                    Serial.println("Written : " + String(written) + " successfully");
                }
                else
                {
                    Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
                    // retry??
                    // execOTA();
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
                    Serial.println("Error Occurred. Error #: " + String(Update.getError()));
                }
            }
            else
            {
                // not enough space to begin OTA
                // Understand the partitions and
                // space availability
                Serial.println("Not enough space to begin OTA");
                _client->flush();
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
        // May be try?
        // Probably a choppy network?
        Serial.println("Connection to " + String(_host) + " failed. Please check your setup");
        // retry??
        // execOTA();
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
        // String deviceID = getDeviceID() ;
        // useURL = checkURL + "?id=" + getDeviceID();
        useURL = checkRESOURCE + "?id=" + getDeviceID(); // Kevin
    }
    else
    {
        // useURL = checkURL;
        useURL = checkRESOURCE; // Kevin
    }

    _port = 80;

    Serial.println("Getting HTTP");
    Serial.println(useURL);
    Serial.println("------");

    //current connection status should be checked before calling this function
    
    _client->setTimeout(60000); // Kevin, muchlonger wait

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
            // if (millis() - timeout > 5000) // CHANGE
            if (millis() - timeout > 60000) // Kevin: More timeout
            {
                Serial.println("Client Timeout !");
                _client->stop();
                return false;
            }
        }

        while (_client->available())
        {
            String header, headerValue;
            // read line till /n
            String line = _client->readStringUntil('\n');
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
                // if (contentType == "application/octet-stream")
                // {
                //     isValidContentType = true;
                // }
                isValidContentType = true;
            }
        }

        // Check what is the contentLength and if content type is `application/octet-stream`
        Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));

        // check contentLength and content type
        if (contentLength && isValidContentType)
        {
            // String payload;
            // //@TODO String length should be limited

            // payload = _client->readStringUntil('\n');
            // Serial.println("Got payload : " + payload);

            // int str_len = payload.length() + 1;
            // char JSONMessage[str_len];
            // payload.toCharArray(JSONMessage, str_len);
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
        // May be try?
        // Probably a choppy network?
        Serial.println("Connection to " + String(checkHOST) + " failed. Please check your setup");
        // retry??
        // execOTA();
    }

    // int err = http.get(useURL); // Kevin
    // // Serial.println("TinyGsmClient: " + String(client.connected()));
    // if (err != 0)
    // {
    //     Serial.println(F("failed to connect"));
    //     delay(10000);
    //     return false; // Error, nothing to update
    // }
    // else
    // {
    //     // Hinh nhu cho nay ok
    //     // Serial.println("http err:" + String(err));
    // }

    // int httpCode = http.responseStatusCode();
    // // Serial.println("httpCode:" + String(httpCode));

    // if (httpCode == 200)
    // { //Check is a file was returned

    //     // String payload = http.getString(); // CHANGE
    //     String payload = http.responseBody(); // CHANGE

    //     int str_len = payload.length() + 1;
    //     char JSONMessage[str_len];
    //     payload.toCharArray(JSONMessage, str_len);

    //     StaticJsonDocument<300> JSONDocument; //Memory pool
    //     DeserializationError err = deserializeJson(JSONDocument, JSONMessage);

    //     if (err)
    //     { //Check for errors in parsing
    //         Serial.println("Parsing failed");
    //         delay(5000);
    //         return false;
    //     }

    //     const char *pltype = JSONDocument["type"];
    //     int plversion = JSONDocument["version"];
    //     const char *plhost = JSONDocument["host"];
    //     _port = JSONDocument["port"];
    //     const char *plbin = JSONDocument["bin"];

    //     String jshost(plhost);
    //     String jsbin(plbin);

    //     _host = jshost;
    //     _bin = jsbin;

    //     String fwtype(pltype);

    //     if (plversion > _firwmareVersion && fwtype == _firwmareType)
    //     {
    //         return true;
    //     }
    //     else
    //     {
    //         return false;
    //     }
    // }

    // else
    // {
    //     // Serial.println("Error on HTTP request");
    //     Serial.print("Error on HTTP request. Error code:");
    //     Serial.println(httpCode);
    //     return false;
    // }

    // // http.end(); //Free the resources
    // http.stop(); // Kevin
    // return false;
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
