// https://github.com/vshymanskyy/TinyGSM
// https://github.com/chrisjoyce911/esp32FOTA
// https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.h
// https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.cpp
// https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/src/HTTPClient.cpp#L221
// https://github.com/vshymanskyy/TinyGSM/blob/master/examples/BlynkClient/BlynkClient.ino
// https://github.com/chrisjoyce911/esp32FOTA/issues/54

// ============== FOTA =============
#include <esp32fotagsm.h>
// Type of Firme for this device, this version
esp32FOTAGSM esp32FOTAGSM("esp32-fota-http", 1);

// ============== GSM ===============
#if (!defined(SRC_TINYGSMCLIENT_H_))
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#endif  // SRC_TINYGSMCLIENT_H_

#define SerialAT Serial1
TinyGsm       modem(SerialAT);
TinyGsmClient client(modem);

void setup()
{
  // ====== GRPS ========
  // To connect GSM and GPRS
  // modem and client should be initialized and connected.
  
  // ====== FOTA ========
  esp32FOTAGSM.checkURL = "http://server/fota/fota.json";
  // https://github.com/vshymanskyy/TinyGSM/blob/master/examples/BlynkClient/BlynkClient.ino#L104
  esp32FOTAGSM.setModem(modem);
}

void loop()
{
  // bool updatedNeeded = esp32FOTA.execHTTPcheck();
  // if (updatedNeeded)
  // {
  //  esp32FOTA.execOTA();
  // }

  // delay(2000);
}
