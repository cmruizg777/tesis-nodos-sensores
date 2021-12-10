#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTTYPE DHT11   // DHT 11
// Credenciales para conectarse a la Red Wifi
const char* ssid = "Cristian Ruiz";
const char* password = "Xalcuadrado";
// MQTT broker IP Address
const char* mqtt_server = "192.168.88.150";
// Initializes the espClient.
WiFiClient espClient;
PubSubClient client(espClient);
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

int inputController = 0;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);
// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
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
      Serial.print("Cambiando Lampara a: ");
      if(messageTemp == "on"){
        digitalWrite(lamp, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "off"){
        digitalWrite(lamp, LOW);
        Serial.print("Off");
      }
  }
  // Enciende o apaga la electroválvula, si el topic es level1/valve
  if(topic=="level1/valve"){
      Serial.print("Cambiando Electrovalvula a: ");
      if(messageTemp == "on"){
        digitalWrite(valve, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "off"){
        digitalWrite(valve, LOW);
        Serial.print("Off");
        delay(3000);
        digitalWrite(valve, HIGH);
        char message[3]="";
        client.publish("level1/autoOffValve", message);
      }
  }
  // Enciende o apaga los ventiladores, si el topic es level1/fan
  if(topic=="level1/fan"){
      Serial.print("Cambiando Ventiladores a: ");
      if(messageTemp == "on"){
        digitalWrite(fan, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "off"){
        digitalWrite(fan, LOW);
        Serial.print("Off");
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
  pinMode(lamp, OUTPUT);
  pinMode(valve, OUTPUT);
  pinMode(fan, OUTPUT);
  dht.begin();
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected())
    reconnect();  
  if(!client.loop())
    client.connect("ESP8266123");
  now = millis();
  // Publica las lecturas de los sensores cada 15s.
  if (now - lastMeasure > 1500) {
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
    /* 
    Serial.print("fc28_1 value is: ");
    Serial.print(fc28_1);
    Serial.print("\tfc28_2 value is: ");
    Serial.print(fc28_2);
    Serial.print("\tldr value is: ");
    Serial.println(ldr);
    */
    // El sensor podría tardar mas de dos segundos en leer los valores
    float h = dht.readHumidity();
    // Lee la temperatura en grados Celsios
    float t = dht.readTemperature();
    
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      t = -1.0f;
      h = -1.0f;
    }
    // Computa los valores leidos del sensor DHT
    float hic = dht.computeHeatIndex(t, h, false);
    static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);
    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);
    /*
    // Publica los valores leidos de los sensores
    client.publish("level1/temp", temperatureTemp);
    client.publish("level1/humidity", humidityTemp);
    client.publish("level1/lightness", String(ldr));
    client.publish("level1/soilHumidity1", fc28_1);
    client.publish("level1/soilHumidity2", fc28_2);
    */
    char message[50]="";
    char* formato="%.2f;%.2f;%.2f;%.2f;%.2f";
    sprintf(message, formato, hic, h, ldr, fc28_1, fc28_2);
    client.publish("level1/sensors", message);
    Serial.println(message);
    /*
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print("\t Heat index: ");
    Serial.print(hic);
    Serial.println(" *C ");
    */
  }
} 
