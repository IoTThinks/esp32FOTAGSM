/*
   esp32 firmware OTA
   Date: December 2018   
   Purpose: Perform an OTA update from a bin located on a webserver (HTTP Only)
*/

#ifndef esp32FOTAGSM_h
#define esp32FOTAGSM_h

#include "Arduino.h"

class esp32FOTAGSM
{
public:
  esp32FOTAGSM(Client& client, String firwmareType, int firwmareVersion);
  void forceUpdate(String firwmareHost, int firwmarePort, String firwmarePath);
  void execOTA();
  bool execHTTPcheck();
  bool useDeviceID;
  // String checkURL; 	// ArduinoHttpClient requires host, port and resource instead
  String checkHOST; 	// example.com
  int checkPORT;		// 80  
  String checkRESOURCE; // /customer01/firmware.json
  void setClient(Client& client);

private:
  String getDeviceID();
  String _firwmareType;
  int _firwmareVersion;
  String _host;
  String _bin;
  int _port;
  Client* _client;
};

#endif
