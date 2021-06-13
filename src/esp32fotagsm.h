/*
   esp32 firmware OTA
   Date: December 2018   
   Purpose: Perform an OTA update from a bin located on a webserver (HTTP Only)
*/

#ifndef esp32FOTAGSM_h
#define esp32FOTAGSM_h

#include "Arduino.h"

#if (!defined(SRC_TINYGSMCLIENT_H_))
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#endif  // SRC_TINYGSMCLIENT_H_

class esp32FOTAGSM
{
public:
  esp32FOTAGSM(String firwmareType, int firwmareVersion);
  void forceUpdate(String firwmareHost, int firwmarePort, String firwmarePath);
  void execOTA();
  bool execHTTPcheck();
  bool useDeviceID;
  String checkURL;
  void setModem(TinyGsm& modem);

private:
  String getDeviceID();
  String _firwmareType;
  int _firwmareVersion;
  String _host;
  String _bin;
  int _port;
  
  // https://github.com/blynkkk/blynk-library/blob/master/src/Adapters/BlynkGsmClient.h#L104
  TinyGsm*     _modem;
  // TinyGsmClient client;
};

#endif
