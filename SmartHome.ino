//Inclusão de Bibliotecas
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ArduinoJson.h>


///////////////  DEFINIÇÕES E MAPEAMENTO DE HARDWARE  ////////////////////////////////

#define lightpin 12   // Pino para acionamento da luz (porta D6)

// Configuração do MQTT
/*
#define MQTT_SERVER "192.168.15.8"  // IP do servidor do MQTT
#define userID      "homemqtt"      // ID do usuário MQTT
#define MQTTpass    "lordvader"     // Senha para MQTT
*/
#define MQTT_SERVER "192.168.0.100"  // 192.168.0.100 IP do servidor do MQTT
#define userID      "homemqtt"      // ID do usuário MQTT
#define MQTTpass    "lordvader"     // Senha para MQTT 

// Tópicos de publicação do MQTT
char* topic             = "home/light1";
char* MQTT_SENSOR_TOPIC = "home/sensor";
char* sensor_topic      = "home/sensor1";
char* sensor_topic2     = "home/sensor2";

// Configuração do Sensor DHT11
#define DHTPIN    5         // Porta D1
#define DHTTYPE   DHT11

// Declaração de variáveis
float h, t;   // Variáveis para armazenar umidade e temperatura
int k = 0;    // Contador
int nr = 10;  // Número de medições
int s = 5000; // Tempo em miliseguondos entre as medições
long m;       // Variável para controle do tempo entre as medições

// Configuração de rede
/*
const char* ssid      = "SKYNET 2.0";
const char* password  = "62x37Gz6kn";
*/

const char* ssid      = "Skynet";
const char* password  = "4d894sYB";

void callback(char* topic, byte* payload, unsigned int length);

// Instanciação de Objetos
DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);


// Função para publicar a Umidade e Temperatura
void publishData(float p_temperature, float p_humidity) {
  // create a JSON object
  // doc : https://github.com/bblanchon/ArduinoJson/wiki/API%20Reference
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  // INFO: a informação deve ser transfomada em Strings
  root["temperature"] = (String)p_temperature;
  root["humidity"] = (String)p_humidity;
  root.prettyPrintTo(Serial);
  Serial.println("");
  /*
     {
        "temperature": "23.20" ,
        "humidity": "43.70"
     }
  */
  char data[200];
  root.printTo(data, root.measureLength() + 1);
  client.publish(MQTT_SENSOR_TOPIC, data, true);
}

// Função de Callback
void callback(char* topic, byte* payload, unsigned int length) {
  //convert topic to string to make it easier to work with
  String topicStr = topic; 
  
  
  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);
  Serial.print("PayLoad: ");
  Serial.println(payload[0]);
  
  /*
  x = !x;
  digitalWrite(lightpin, x);
  Serial.print("Status: ");
  Serial.println(x);
  */


  //turn the light on if the payload is '1' and publish to the MQTT server a confirmation message
  if(payload[0] == '1'){
    digitalWrite(lightpin, HIGH);
    client.publish("/test/confirm", "Light On");
  }

  //turn the light off if the payload is '0' and publish to the MQTT server a confirmation message
  else if (payload[0] == '0'){
    digitalWrite(lightpin, LOW);
    client.publish("/test/confirm", "Light Off");
  }

}

void setup() {
  
  pinMode(lightpin, OUTPUT);    // Configuração do pino de saída
  digitalWrite(lightpin, HIGH);
  
  dht.begin();                  // Inicialização do Sensor
  Serial.begin(9600);           // Inicialização da Comunicação Serial
  WiFi.begin(ssid, password);   // Inicialização da Rede
  client.setCallback(callback);
  
  delay(100);
  
  reconnect();                  // Função para conectar/reconectar à rede WiFi e servidor MQTT

  delay(2000);

  k = 0;                        // Ajusta o contador em zero
  m = millis();                 // Lê o tempo atual do sistema
  digitalWrite(lightpin, LOW); 
  
}

void loop() {
  // Verifica se está conectado, e reconecta
  if (!client.connected() && WiFi.status() == 3) {reconnect();}         
  // Mantém ativo o servidor MQTT
  client.loop();  

  // Faz as leitura da umidade e temperatura
  if (millis() > (m + s)){
    if (k <= nr) {
      // Verifica se houve erro na medição
      if(isnan(dht.readHumidity()) || isnan(dht.readTemperature())) {
        Serial.println("ERROR");
        m = millis();
        goto iferror; 
      }  
      h += dht.readHumidity();
      t += dht.readTemperature();
      k += 1;
      m = millis();
      Serial.println(t/k);
      
    }
  }
  
  iferror:
  
  if (k == 10){
    t = (t/k);
    h = (h/10);
    
    String temp_str = String(t);
    char temp[50];
    temp_str.toCharArray(temp, temp_str.length()+1);

    String humid_str = String(h);
    char humid[50];
    humid_str.toCharArray(humid, humid_str.length()+1);

    
    publishData(t, h);
  
    Serial.print("Temp: ");
    Serial.println(temp);
    client.publish(sensor_topic, temp);   // Publica a temperatura no tópico especificado.
    client.publish(sensor_topic2, humid); // Publica a umidade no tópico especificado.
    

    // Zera as variáveis para ua nova medição
    k = 0;
    t = 0;
    h = 0;
    delay(100); 
  }

  
  
}


// Função para reconectar ao WiFi caso caia a conexao
void reconnect() {
  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){
    //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    //print out some more debug once connected
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){

    while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_SERVER,userID,MQTTpass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic","hello world");
      // ... and resubscribe
      client.subscribe(topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      }
    }
  }
}


