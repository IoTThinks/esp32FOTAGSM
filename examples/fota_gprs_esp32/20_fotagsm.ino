void setupFOTAGSM()
{
  esp32FOTAGSM.checkHOST = esp32FOTAGSM_checkHOST;
  esp32FOTAGSM.checkPORT = esp32FOTAGSM_checkPORT;
  esp32FOTAGSM.checkRESOURCE = esp32FOTAGSM_checkRESOURCE;  
  esp32FOTAGSM.setModem(modem); 
}
