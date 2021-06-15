// References
// https://github.com/vshymanskyy/TinyGSM
// https://github.com/chrisjoyce911/esp32FOTA
// https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.h
// https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.cpp
// https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.cpp#L221
// https://github.com/vshymanskyy/TinyGSM/blob/master/examples/BlynkClient/BlynkClient.ino
// https://github.com/chrisjoyce911/esp32FOTA/issues/54

// ============== FOTA =============
#include <esp32fotagsm.h>
const char *boardModel = "test-esp32";                    // TO CHANGE
int boardCurrentVersion = 1; // The firmware version      // TO CHANGE

// To define firmware type and version
esp32FOTAGSM esp32FOTAGSM(boardModel, boardCurrentVersion);

// To define link to check update json
#define esp32FOTAGSM_checkHOST      "example.com"         // TO CHANGE
#define esp32FOTAGSM_checkPORT      80                    // TO CHANGE, HTTP ONLY
#define esp32FOTAGSM_checkRESOURCE  "/firmware.json" // TO CHANGE

// ============== GSM ===============
#if (!defined(SRC_TINYGSMCLIENT_H_))
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#endif  // SRC_TINYGSMCLIENT_H_

// To CHANGE to fit your board
#define SerialMon Serial  // For debug serial
#define SerialAT Serial2  // For GSM module

// Your GPRS credentials, if any
const char apn[]  = "m-wap";    // TO CHANGE
const char user[] = "mms";      // TO CHANGE
const char pass[] = "mms";      // TO CHANGE
bool gprs_connected = false;

// GSM variables
TinyGsm       modem(SerialAT);
TinyGsmClient client(modem);
