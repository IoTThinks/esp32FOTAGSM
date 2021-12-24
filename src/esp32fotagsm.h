/*
   esp32 firmware OTA
   Date: December 2018   
   Purpose: Perform an OTA update from a bin located on a webserver (HTTP Only)
*/

#ifndef esp32FOTAGSM_h
#define esp32FOTAGSM_h

#include "Arduino.h"
#include <FreeRTOS.h>

class esp32FOTAGSM
{
public:
  typedef std::function<bool(void)> TConnectionCheckFunction;

  esp32FOTAGSM(Client &client, String firwmareType, int firwmareVersion,
               TConnectionCheckFunction connectionCheckFunction = NULL,
               SemaphoreHandle_t networkSemaphore = NULL,
               int ledPin = -1,
               uint8_t ledOn = LOW,
               bool chunkedDownload = false
               );

  void forceUpdate(String firwmareHost, int firwmarePort, String firwmarePath);
  bool execOTA();
  bool execHTTPcheck();
  bool useDeviceID;
  String checkHOST;     // example.com
  int checkPORT;        // 80
  String checkRESOURCE; // /customer01/firmware.json
  void setClient(Client &client);

private:
  bool _checkConnection();
  void _blockingNetworkSemaphoreTake();
  void _blockingNetworkSemaphoreGive();
  String _getDeviceID();

  String _firwmareType;
  int _firwmareVersion;
  String _host;
  String _bin;
  int _port;
  Client *_client;
  SemaphoreHandle_t _networkSemaphore;
  TConnectionCheckFunction _connectionCheckFunction;
  int _ledPin;
  uint8_t _ledOn;
  bool _chunkedDownload;
};

#endif
