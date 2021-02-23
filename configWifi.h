#include <WiFiManager.h> 
#define pinLedOffWifi 4
#define pinLedOnWifi 0

void conectaWifi() {
  
  Serial.begin(115200);
  pinMode(pinLedOnWifi, OUTPUT);
  pinMode(pinLedOffWifi, OUTPUT);
  
  WiFiManager wifimanager;
  //wifimanager.resetSettings();

  bool comprobarWifiConf;
  digitalWrite(pinLedOffWifi, HIGH);
  delay(100);
  digitalWrite(pinLedOnWifi, LOW);
  
  comprobarWifiConf = wifimanager.autoConnect("ESP32-IOT");

  if(!comprobarWifiConf) {
    Serial.println("FALLO EN LA CONEXION...");
    ESP.restart();
    delay(1000);
  } 
  digitalWrite(pinLedOffWifi, LOW);
  delay(100);
  digitalWrite(pinLedOnWifi, HIGH);
  Serial.println("TE HAS CONECTADO...... :)");

}
