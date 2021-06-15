// For ESP32 + SIM800 + FOTA
#include "headers.h"
void setup()
{
  // ====== Serial ========
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);
  
  SerialMon.print("Current version: ");  
  SerialMon.println(boardCurrentVersion);  
  
  // ====== GRPS ========
  // To connect GSM and GPRS
  // modem and client should be initialized and connected.
  setupGSM(); // De dung cho chu haha. Hi vong chay luon
  delay(5000);
  
  // ====== FOTA ========
  // Link to json to check update
  setupFOTAGSM();
}

void loop()
{
  SerialMon.print("Current version: ");
  SerialMon.println(boardCurrentVersion);
  
  bool updatedNeeded = esp32FOTAGSM.execHTTPcheck();
  if (updatedNeeded)
  {
    SerialMon.println("Got new update");
    esp32FOTAGSM.execOTA();    
  }
  else
  {    
    SerialMon.println("Already up to date. No need to update");
  }

  delay(2000);
}
