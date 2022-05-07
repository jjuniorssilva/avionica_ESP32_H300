void loopCore1( void *arg ){
  Serial.println("O Numero está sendo exibido no core "+String(xPortGetCoreID()));
  while(true){
    
    delay(500);
  } 
}

//-----------------------Armazenamento------------------------------//

void send_backup_eeprom(long address, byte data) {
  EEPROM.begin(8);
  EEPROM.write(address, data);
  EEPROM.end();
}
byte get_backup_eeprom(long address) {
  EEPROM.begin(8);
  return EEPROM.read(address);
  EEPROM.end();
}

void writeSPIFSS(int data){
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  File file = SPIFFS.open("/test.txt", FILE_WRITE);
  if (!file) {
    Serial.println("There was an error opening the file for writing");
  }
  if (file.print(String(data))) {
    Serial.println("File was written");
  } else {
    Serial.println("File write failed");
  }
 
  file.close();
}
//-------------------- Fim Armazenamento----------------------------//

//-----------------------Trasmissão------------------------------//


//-------------------- Fim Trasmissão----------------------------//
