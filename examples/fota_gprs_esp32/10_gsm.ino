void setupGSM()
{
  // =============== GSM ================
  // Set GSM module baud rate
  SerialAT.begin(115200);
  Serial.println("\nStarted");
  delay(6000);
  SerialMon.println("Initializing modem...");
  modem.restart();

  // To register to GSM
  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) 
  {
    SerialMon.println("Network connected"); 
  }

  // =============== GPRS ================
  // GPRS connection parameters are usually set after network registration
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) 
  {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  gprs_connected = true;
  SerialMon.println(" success");
  SerialMon.println(modem.localIP());  
}
