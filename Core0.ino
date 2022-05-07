void loopCore0(void *arg ){
  Serial.println("O Numero está sendo incrementado no core "+String(xPortGetCoreID()));
  while(true){
      if(digitalRead(5)==1)calibracao_alt();
      packet_1.data.time_atual = millis();
      //update_rotina();
      rotinas(rotina);
    
      Serial.println("Rotina.: "+String(rotina));
      while (gps_serial.available() > 0)
      if (gps.encode(gps_serial.read()))
        getDataGPS();
     
      
      delay(50);
  } 
}
//------------------ Recuperação-----------------------------//
bool check_direction() {
  if (altura_ant > packet_1.data.alt_bmp)return true; // se a altura anterior for maior que a atual o foguete está descendo
  return false;
}

bool check_height() {
  if (packet_1.data.alt_bmp >= altura_min)return true; // se a altura atual é maior ou igual a minima(altura minima de abertura do paraquedas)
  return false;
}

void servo_control(int angle) {
  servoPara.write(angle);
}
//------------------ Fim Recuperação-----------------------------//


//--------------------Sensores----------------------------------//

//----------clock-------------------
void getDataCLOCK(){
  DateTime now = rtc.now(); 
  packet_1.data.time_hora_clock = now.hour(); 
  packet_1.data.time_min_clock= now.minute(); 
  packet_1.data.time_sec_clock= now.second(); 
}

//----------dht22-------------------

void getDataDHT22(){
  packet_1.data.humi_dht22 = dht.readHumidity();  
  packet_1.data.temp_dht22 = dht.readTemperature();
}
//----------bmp280-------------------

//Testado - Tudo OK
void getDataBMP() {
  float  media_alt, percent = 0.001;
  packet_1.data.pressao_bmp = bmp.readPressure();
  packet_1.data.temp_bmp = bmp.readTemperature();
  for(int i=0; i<15;i++){
    media_alt += bmp.readAltitude()/15;
    delay(20);
  }
  //if (alt0 >= (media_alt + (media_alt * percent)) || alt0 <= (media_alt - (media_alt * percent))) { // verifica se o valor coletado está dentro do intervalo gerado pela média
    packet_1.data.alt_bmp = media_alt - alt_init_eeprom.alt_inicial;
  //} else {
   // packet_1.data.alt_bmp = alt0 - 60;
  //}
}

//----------GPS-------------------

void getDataGPS(){
  if (gps.location.isValid()){
    
    Serial.print("Latitude: ");
    packet_1.data.lat_gps=gps.location.lat();
    Serial.println(packet_1.data.lat_gps, 6);
    Serial.print("Longitude: ");
    packet_1.data.lon_gps=gps.location.lng();
    Serial.println(packet_1.data.lon_gps, 6);
    Serial.print("Altitude: ");
    packet_1.data.alt_gps=gps.altitude.meters();
    Serial.println(packet_1.data.alt_gps);
  }else{
    Serial.println("Location: Not Available");
    packet_1.data.lon_gps=0;
    packet_1.data.lat_gps=0;
    packet_1.data.alt_gps=0;
  }
  
  Serial.print("Date: ");
  if (gps.date.isValid()){
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.println(gps.date.year());
  }else{
    Serial.println("Not Available");
  }

  Serial.print("Time: ");
  if (gps.time.isValid()){
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(":");
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(":");
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(".");
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.println(gps.time.centisecond());
  }else{
    Serial.println("Not Available");
  }
  Serial.print("Sat: ");
  if(gps.satellites.isValid()){
    packet_1.data.sat_gps=gps.satellites.value();
    Serial.println(String(packet_1.data.sat_gps));
  }else{
    Serial.println("Not Available");
     packet_1.data.sat_gps=0;
  }

  Serial.println();

  delay(100);
}


//-------------------Fim Sensores-------------------------------//

//--------------------- Rotinas -------------------------------//
void rotinas(int op) {
  if (op == 0) {
    if (millis() - ciclo >= 1000) {
      if (check_I2C(0x76) == true) { // realiza a checagem da conexão com os modulos || Mudar os endereços do I2C!!
        digitalWrite(led2, LOW);
      } else {
        digitalWrite(led2, HIGH);
      }
      getDataBMP();
      getDataCLOCK();
      getDataDHT22();
      ciclo = millis();
    }
  } else if (op == 1) {
    //--------coleta----------
    if (check_I2C(0x76) == true) { // realiza a checagem da conexão com os modulos || VERIFICAR PQ SE UM PARAR TÁ PARANDO O CÓDIGO TODO!!
      digitalWrite(led2, LOW);
      getDataBMP(); // coleta dados do bmp
      if (check_direction() == true) {
        Serial.println("check_dir");
        
        apogeu = true;
        //Salvar apogeu na EEPROM!
        if (check_height() == true) {
          Serial.println("check_hei");
          digitalWrite(led3, HIGH);
          status_servo = true;
          time_servo = packet_1.data.time_atual;
          
          for (int i = 0; i < 5; i++) {
            servoPara.write(145);
            delay(50);
            servoPara.write(145);
            delay(300);
          }
        }
      }
      altura_ant = packet_1.data.alt_bmp; // guarda a altura coletada nesse clico para analisar no proximo
    } else {
      digitalWrite(led2, HIGH);
    }

  } else {
    if (status_servo == false) {
      digitalWrite(led3, HIGH);
      for (int i = 0; i < 5; i++) {
        servoPara.write(160);
        delay(50);
        servoPara.write(50);
      }
    }
    digitalWrite(pinoBuzzer, HIGH);
    delay(500);
    digitalWrite(pinoBuzzer, LOW);
    delay(500);
    rotinas(2);
  }
}
void update_rotina() { // a ideia aqui é que >=1m de altura o modo de voo mude e ao retornar a <=1m de altura mude de novo;
  if (packet_1.data.alt_bmp >= 0.5 && status_alt_rotina == false && rotina == 0) {
    status_alt_rotina = true;
    rotina = 1;
  } else if (packet_1.data.alt_bmp <= 0.5 && status_alt_rotina == true && rotina == 1 && apogeu == true) {
    rotina = 2;
  }
}

//--------------------- Fim Rotinas -------------------------------//
