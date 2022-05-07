#include <ESP32Servo.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>  
#include <TinyGPS++.h>
#include "SPIFFS.h"
#include "RTClib.h"
#include "DHT.h"
#include <HardwareSerial.h>

#define pinoBuzzer 14
#define pinServo 13
#define led2 33
#define led3 35
#define RXD2 16
#define TXD2 17
int GPSBaud = 9600;

Servo servoPara;
Adafruit_BMP280 bmp;
RTC_DS3231 rtc; //OBJETO DO TIPO RTC_DS3231
TinyGPSPlus gps;
DHT dht(4, DHT22);
HardwareSerial gps_serial(2);

static uint8_t CoreZero = 0;
static uint8_t CoreOne  = 1;
int rotina = 0, ciclo = 0, altura_ant = 0; //Lembrar de ajeitar a altura mínima
float altura_min = 0.8;
bool status_alt_rotina = false, status_calib = true, status_servo = false, apogeu = false;
long time_servo = 0;
char daysOfTheWeek[7][12] = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"};


//Aqui as variaveis serão convertada em m array de bytes para a transmissão;
union packet_type_1 {
  //o struct agrupa os dados e o union propociona a recuparação dos bytes desse grupo;
  struct data {
    long time_atual;
    float pressao_bmp;
    float alt_bmp;
    float temp_bmp;
    float temp_dht22;
    float humi_dht22;
    float time_sec_clock;
    float time_min_clock;
    float time_hora_clock;
    float lon_gps;
    float lat_gps;
    float alt_gps;
    byte sat_gps;
  } data;
  byte data_byte[sizeof(data)];
};
union address {
  unsigned int eeaddress;
  byte eeaddress_byte[sizeof(eeaddress)];
};
union alt_init {
  float alt_inicial=0;
  byte alt_inicial_byte[sizeof(alt_inicial)];
};

// "inicaliza" o union para buscas;
union packet_type_1 packet_1;
union address address_eeprom;
union alt_init alt_init_eeprom;

void calibracao_alt() {
  
  float media_alt=0;
  for(int i=0; i<15;i++){
    media_alt += bmp.readAltitude()/15;
    delay(20);
  }
  alt_init_eeprom.alt_inicial = media_alt+2;
  for (int i = 0; i < 4; i++) {
       send_backup_eeprom(i, alt_init_eeprom.alt_inicial_byte[i]);
  }
   
}


void setup() {
  Serial.begin(115200);
  gps_serial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  xTaskCreatePinnedToCore(loopCore0,"coreTaskZero",10000,NULL,1, NULL,CoreZero);                  
  delay(500);
  xTaskCreatePinnedToCore(loopCore1,"coreTaskZero",10000,NULL,1, NULL,CoreOne);                  
  delay(500);
  
  pinMode(pinoBuzzer, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(5, INPUT);
  pinMode(led2, OUTPUT);
  servoPara.attach(pinServo);
  servoPara.write(75);
  delay(10);
  Serial.println("Tentando se conetar ao BMP!");
  while(!bmp.begin(0x76)){ 
    Serial.println("BMP280 erro"); 
    digitalWrite(led2, HIGH);
    delay(500);
  }
  Serial.println("Sensor BMP280 ok");
  while(!rtc.begin()) { 
    Serial.println("DS3231 não encontrado"); 
    delay(500);
  }
   if(rtc.lostPower()){ 
    Serial.println("DS3231 ok"); 
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //CAPTURA A DATA E HORA EM QUE O SKETCH É COMPILADO
  }
  Wire.begin();
  dht.begin();
  Serial.println("Setup Ok");
  if (get_backup_eeprom(0) != 0) {
    for (int i = 0; i < 4; i++) {
       alt_init_eeprom.alt_inicial_byte[i] = get_backup_eeprom(i);
    }
  }else{
    for (int i = 0; i < 4; i++) {
       send_backup_eeprom(i, 0);
       alt_init_eeprom.alt_inicial_byte[i]=0;
    }
   } 
   for (int i = 0; i < 8; i++) {
       send_backup_eeprom(i, 0);
       
   } 
}


void loop() {
}

// ----checagem da conexão i2c------
bool check_I2C(uint8_t ADDRESS) {
  Wire.beginTransmission(ADDRESS);
  byte erro = Wire.endTransmission();
  if (erro == 0)return true;
  return false;
}
