#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//#include <DHT.h>
#include "DHTesp.h"
DHTesp dht;
// MQTT broker IP Address
//const char* mqtt_server = "192.168.88.150";
const char* mqtt_server = "192.168.88.150";
// 4051 S0 - GPIO 16 = D0 on ESP-12E NodeMCU board
const int s0 = 16;
// 4051 S1 - GPIO 5 = D1 on ESP-12E NodeMCU board
const int s1 = 5;
// DHT Sensor - GPIO 4 = D2 on ESP-12E NodeMCU board
const int DHTPin = 4;
// Lamp - GPIO 0 = D3 on ESP-12E NodeMCU board
const int lamp = 0;
// Valve - GPIO 2 = D4 on ESP-12E NodeMCU board
const int valve = 2;
// Fan - GPIO 14 = D5 on ESP-12E NodeMCU board
const int fan = 14;

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// Credenciales para conectarse a la Red Wifi
const char* ssid = "Cristian Ruiz";
const char* password = "Xalcuadrado";

//#define DHTTYPE DHT11   // DHT 11
//DHT dht(DHTPin, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Ha llegado un mensaje del topic: ");
  Serial.print(topic);
  Serial.print(". Mensaje: ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  // Enciende o apaga la lámpara, si el topic es level1/lamp
  if(topic=="level1/lamp"){
      int lampState = 0;
      Serial.print("Cambiando Lampara a: ");
      if(messageTemp == "on"){
        digitalWrite(lamp, HIGH);
        Serial.print("On");
        lampState = 1;
      }
      else if(messageTemp == "off"){
        digitalWrite(lamp, LOW);
        Serial.print("Off");
      }
      char messageActuators[1]="";
      char* formato="%i";
      sprintf(messageActuators, formato, lampState);
      client.publish("level1/actuators/lamp", messageActuators);
  }
  
  // Enciende o apaga los ventiladores, si el topic es level1/fan
  if(topic=="level1/fan"){
      Serial.print("Cambiando Ventiladores a: ");
      int fanState = 0;
      if(messageTemp == "on"){
        digitalWrite(fan, HIGH);
        Serial.print("On");
        fanState = 1;
      }
      else if(messageTemp == "off"){
        digitalWrite(fan, LOW);
        Serial.print("Off");
      }
      char messageActuators[1]="";
      char* formato="%i";
      sprintf(messageActuators, formato, fanState);
      client.publish("level1/actuators/fans", messageActuators);
  }
  // Enciende o apaga la electroválvula, si el topic es level1/valve
  if(topic=="level1/valve"){
      Serial.print("Cambiando Electrovalvula a: ");
      int valveState = 0;
      if(messageTemp == "on"){
        digitalWrite(valve, HIGH);
        Serial.print("On");
        valveState = 1;
        char messageActuators[1]="";
        char* formato="%i";
        sprintf(messageActuators, formato, valveState);
        client.publish("level1/actuators/valve", messageActuators);  
        delay(2000);
        digitalWrite(valve, LOW);
        char message[3]="";
        client.publish("level1/autoOffValve", message);
      }
      else if(messageTemp == "off"){
        digitalWrite(valve, LOW);
        Serial.print("Off");
        char messageActuators[1]="";
        char* formato="%i";
        sprintf(messageActuators, formato, valveState);
        client.publish("level1/actuators/valve", messageActuators);  
      }
  }
  Serial.println();
}

// Esta función reconecta el ESP8266 a el MQTT broker
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Intentando conectar
    if (client.connect("ESP8266123")) {
      Serial.println("connected");  
      // Subscribe o resubscribe a un topic
      client.subscribe("level1/lamp");
      client.subscribe("level1/valve");
      client.subscribe("level1/fan");
    } else {
      //Imprime el codigo de error de la conexión
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Espera 5s antes de volver a intentar
      delay(5000);
    }
  }
}

// Configura ESP GPIOs en Salidas y Entradas según corresponda. 
// Inicia la comunicación serial en un baud rate de 115200
// Llama a la función para establecer la comunicación WiFi, configura el MQTT Broker.
// Configura el callback para las subscripciones y mensajes que llegan a traves de los topics subscritos
void setup() {
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(lamp, OUTPUT);digitalWrite(lamp, LOW);
  pinMode(valve, OUTPUT);digitalWrite(valve, LOW);
  pinMode(fan, OUTPUT);digitalWrite(fan, LOW);
  //dht.begin();
  dht.setup(DHTPin, DHTesp::DHT11); 
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  ArduinoOTA.handle();
  if (!client.connected())
    reconnect();  
  if(!client.loop())
    client.connect("ESP8266123");
  now = millis();
  // Publica las lecturas de los sensores cada 30s.
  if (now - lastMeasure > 1000) {
    lastMeasure = now;
    //Se establece s0 y s1 en LOW para obtener la entrada Y0 - Sensor FC28-1
    digitalWrite(s0, LOW);
    digitalWrite(s1, LOW);
    float fc28_1 = (float)analogRead(0);
    //Espera 250 ms para realizar una siguiente lectura
    delay(250);
    //Se establece s0 en HIGH y s1 en LOW para obtener la entrada Y1 - Sensor FC28-2
    digitalWrite(s0, HIGH);
    digitalWrite(s1, LOW);
    float fc28_2 = (float)analogRead(0);
    //Espera 250 ms para realizar una siguiente lectura
    delay(250);
    //Se establece s0 en LOW y s1 en HIGH para obtener la entrada Y2 - Sensor LDR
    digitalWrite(s0, LOW);
    digitalWrite(s1, HIGH);
    float ldr = (float)analogRead(0);   
    // El sensor podría tardar mas de dos segundos en leer los valores
    //float h = dht.readHumidity();
    // Lee la temperatura en grados Celsios
    //float t = dht.readTemperature();
    int success = 0;
    char message[50]="24.25;94.00;1024.00;1024.00;1024.00";
    /*
    while(success==0){
      dht.setup(DHTPin, DHTesp::DHT11); 
      delay(dht.getMinimumSamplingPeriod());
      float h = dht.getHumidity();
      float t = dht.getTemperature();
      if (isnan(h) || isnan(t)) {
        Serial.println("Error en el sensor DHT!");
        t = -1.0f;
        h = -1.0f;
      }else{
        success=1;
         // Computa los valores leidos del sensor DHT
        float hic = dht.computeHeatIndex(t, h, false);
        static char temperatureTemp[7];
        dtostrf(hic, 6, 2, temperatureTemp);
        static char humidityTemp[7];
        dtostrf(h, 6, 2, humidityTemp);
        char* formato="%.2f;%.2f;%.2f;%.2f;%.2f";
        sprintf(message, formato, hic, h, ldr, fc28_1, fc28_2);
      }
     
    }
    */
    client.publish("level1/sensors", message);
    Serial.println(message);
  }
}
