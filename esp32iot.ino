/****************************************
 * Define Bibliotecas a Implementar
 ****************************************/
#include "configWifi.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

/****************************************
 * Define Constantes para la Conexion a una Red
 ****************************************/

//#define WIFISSID "NETLIFE-ALVAREZ" // Nombre de la Red
//#define PASSWORD "mx0951268713" // Contraseña de Reed
#define TOKEN "BBFF-c075pzVLsBM91D6Ev7QBaTXH3bJuLd" // Credencial de Ubidots
#define MQTT_CLIENT_NAME "hellowor89" // Nombre del cliente MQTT
                                           
#define DEVICE_LABEL "monitoragua" // Nombre del Dispositivo a crearse
#define VARIABLE_TEMP "temperatura" // Variable de temperatura
#define VARIABLE_TURB "turbidez" // Variable de turbidez
#define VARIABLE_PH "ph" // Variable de pH

#define ESPADC 1024.0   //Conversion de analogico digital
#define ESPVOLTAGE 3.3 //Voltaje del esp32


char mqttBroker[] = "industrial.api.ubidots.com";
char payload[700];
char topic[150];

// Espacio para almacenar los datos a enviar al servidor
char str_temp[10];
char str_turb[10];
char str_ph[10];

// Se definen los pines donde esta conectados los sensores
int sensor_turb = 32;
int sensor_ph = 35;
const int sensor_temp = 26;
float NTU = 0.0;  
float tb = 0.0;

// Inicializar variables para la temperatura
OneWire oneWire(sensor_temp);
DallasTemperature sensors(&oneWire); 

/****************************************
 * Initializar los objectos para la
 * conxexion a los servidores
 ****************************************/
WiFiClient ubidots;
PubSubClient client(ubidots);
AsyncWebServer server(80);
/****************************************
 * Funciones Auxiliares para la conexion
 * Al servidor de Ubidots
 ****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
  Serial.print((char)payload[i]);
  }
  Serial.println();
} 

void reconnect() {
  // realiza un ciclo para comprobar si el cliente
  // Se conecto al servidor de Ubidtots
  while (!client.connected()) {
  Serial.println("Attempting MQTT connection...");

  // Pide las credenciales para la conexion
  if (client.connect(MQTT_CLIENT_NAME, TOKEN,"")) {
    Serial.println("conectado!");
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" Intentado conectarse en 2 segundos");
    // Espera 2 segundos para intentar conectarse
    delay(2000);
  }
  }
}

/****************************************
 * FUNCION PARA REALIZAR MUESTRAS
 ****************************************/
float voltage(int pin) {
  float avg = 0;
  float entrada_analogica = 0;
  int i = 0;
  float volt = 0;
  // Realiza un muestreo de 100 muestras
  for (i; i < 500; i++) {
    entrada_analogica += analogRead(pin); //
  }
  avg = entrada_analogica / i;
  volt = avg * ESPVOLTAGE / ESPADC;
  return volt;
}
/****************************************
 * Obtener Valores de cada sensor
 ****************************************/
 
String readTemp() {
  sensors.requestTemperatures(); 
  float tm = sensors.getTempCByIndex(0);
  return String(tm);
}
String readpH() {
  float p = (7 + ((2.5 - voltage(sensor_ph)) / 0.18)) - 0.50;
  return String(p);
}

String readTurb() {
  float t = voltage(sensor_turb);
  /*
  if(t < 2.5){  
      tb = 3000;  
   }else{  
      tb = -1120.4*(t*t)+5742.3*t-4352.9;   
   }  */
  return String(t);
}



// PROCESA LAS VARIABLES PARA EL SERVIDOR LOCAL
String processor(const String& var){
  if(var == "TEMPERATURE"){
    return readTemp();
  }else if (var == "PH") {
    return readpH();
  }else if(var == "TURBITY"){
    return readTurb();
  } 
  return String();
}
/****************************************
 * Main Functions
 ****************************************/

void setup() {
  conectaWifi();
  //Serial.begin(115200);
  analogReadResolution(10);
  // Inicializa la conexion a la red
  //WiFi.begin(WIFISSID, PASSWORD);
  pinMode(sensor_turb, INPUT);
  pinMode(sensor_ph, INPUT);
  Serial.println();
  Serial.println();
  Serial.print("Esperando a conectarse a la Red... ");

   // Inicializa SPIFFS para cargar los archivos al ESP32
  if(!SPIFFS.begin(true)){
    Serial.println("Un error ha surgido al subir los datos con SPIFFS");
    return;
  }
/*
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
*/
  Serial.println("");
  Serial.println("Conexion Wifi existosa");
  Serial.println("Su Direccion Ip es: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqttBroker, 1883);
  client.setCallback(callback);
  // Envia al servidor local el archivo HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  // Envia al servidor local el archivo css
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });
  
}

void loop() {
  if (!client.connected()) {
      reconnect();
   }
  static unsigned long timepoint = millis();
  if (millis() - timepoint > 2000U) //time interval: 2s
  {
    timepoint = millis();
    
    /****************************************
   * OBTECCION DE DATOS DE LOS SENSORES
   ****************************************/
    // ================ OBTENER TEMPERATURA===========================
    float temp = readTemp().toFloat();
    dtostrf(temp, 4, 2, str_temp);
    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", str_temp);
  });
    // ================ OBTENER PH===========================
    //float ph = -5.70 * voltage(sensor_ph) + 21.34;
    //float ph = 7 + ((2.5 - voltage(sensor_ph)) / 0.18);
    float ph = readpH().toFloat();
    dtostrf(ph, 4, 2, str_ph);
    server.on("/ph", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", str_ph);
  });
    // ================ OBTENER Turbidez===========================
    //float turb = voltage(sensor_turb);
    float NTU = readTurb().toFloat();
    dtostrf(NTU, 4, 2, str_turb);
    server.on("/turbidez", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", str_turb);
    });
    
 
    sprintf(topic, "%s", ""); // Limpia el contenido
    sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  
    sprintf(payload, "%s", ""); 
    sprintf(payload, "{\"%s\":", VARIABLE_TEMP);  
    sprintf(payload, "%s {\"value\": %s}", payload, str_temp); 


    sprintf(payload, "%s, \"%s\":", payload, VARIABLE_PH);   
    sprintf(payload, "%s {\"value\": %s}", payload, str_ph); 
    
    sprintf(payload, "%s, \"%s\":", payload, VARIABLE_TURB);  
    sprintf(payload, "%s {\"value\": %s}", payload, str_turb); 
    sprintf(payload, "%s}", payload); // Cierra el diccionario con los datos
    
    Serial.println(payload);
    server.begin();
    client.publish(topic, payload);
    client.loop();
    // LEVANTA EL SERVIDOR
    
  }
}
