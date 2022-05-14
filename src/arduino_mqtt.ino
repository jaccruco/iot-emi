/****************************************
 * Include Libraries
 ****************************************/
#include <DHT.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>


#define DHTPIN 13 // DHT Digital Input Pin
#define DHTTYPE DHT11 // DHT11 or DHT22, depends on your sensor
#define PIN_LDR 38
#define PIN_RELE 23
#define PIN_SOLO 39


DHT dht(DHTPIN, DHTTYPE);



/****************************************
 * Define Constants
 ****************************************/

const char* ssid = "SMART-FARM";      // SSID Wifi
const char* password = "Pr0j3ct1ot";      // Senha Wifi
const char* mqttServer = "smartfarm.iot.ifrn"; //Link servidor mqtt



int id_dispositivo = 1;
int status_rele;


const int mqttPort = 1883;
const String topicoTelemetria = "v1/esp32/telemetria";
const String topicoRele = "v1/esp32/telemetria/statusRele";
const char* clientName = "esp1";
unsigned long lastSend;

WiFiClient wifiClient; //Objeto que manipula a conexão wifi
PubSubClient client(wifiClient); //Objeto que manipula a conexão com o broker mqtt



void mqttReConnect() {
  while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
    if (client.connect(clientName)) {
      Serial.println("connected");
      client.subscribe(topicoRele.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
    
  }
}


//Função para publicar no broker
void publicar(String topic, String value) {

  client.publish((char*) topic.c_str(), (char*) value.c_str());
  
}


//Função para tratar retorno
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] payload: ");
  String data;
  for (int i = 0; i < length; i++) {
    data += (char)payload[i];
  }

    Serial.print(data);

  atualizaAtributos(topic, data);
    
}

//Função para configurar wifi
void conectaServidor() {
    delay(10);
    WiFi.begin(ssid, password); //Iniciar de fato a conexão com a rede sem fio
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    randomSeed(micros());
    
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
    
}



void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
    // initialize dht22
  dht.begin();
  conectaServidor();
  pinMode(PIN_RELE, OUTPUT);
  digitalWrite(PIN_RELE, HIGH);
  status_rele = 0;
  lastSend = 0;


}

void loop(){
  
if (!client.connected()) {
    mqttReConnect();
  }

client.loop();


if ( millis() - lastSend > 5000 ) { // Update and send only after 1 seconds


  float umidade = dht.readHumidity();
  float temperatura = dht.readTemperature();
  int sensorValue = analogRead(PIN_LDR);
  int percentualLumi = map(sensorValue, 0, 4095, 0, 100);
  float umidadesolo = analogRead(PIN_SOLO);

  
  StaticJsonDocument<200> doc;
  //doc["time"] = millis();
  doc["temperatura"] = temperatura;
  doc["umidade"] = umidade;
  doc["umidadesolo"] = umidadesolo;
  doc["luminosidade"] = percentualLumi; 
  doc["statusrele"] = status_rele;
  doc["ID"] = id_dispositivo;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
    // Check if any reads failed and exit early (to try again).
//  if (isnan(umidade) || isnan(temperatura)) {
//    Serial.println(F("Failed to read from DHT sensor!"));
//    return;
 // }

  publicar(topicoTelemetria, jsonBuffer);
  //Serial.println("Publicado"); 

 lastSend = millis(); 
  }
  
 

 
}






void atualizaAtributos(String topico, String valor){
  
  if (topico.equals(topicoRele)){
        status_rele = valor.toInt();
        alteraStatusRele();
  }
}

void alteraStatusRele(){

    switch (status_rele){
      case 0:
        digitalWrite(PIN_RELE, HIGH);
 //       Serial.println("led desligado");
        break;
      case 1:
        digitalWrite(PIN_RELE, LOW);
 //       Serial.println("led ligado");
        break;
   }
}



  
