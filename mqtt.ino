#include<WiFi.h>
#include <PubSubClient.h>
#include <math.h>

#define IO_USERNAME  "@gmail.com" //<----------Mudar para o seu
#define IO_KEY       "" //<----------Mudar para o seu
const char* ssid = ""; //<----------Mudar para o seu
const char* password = ""; //<----------Mudar para o seu
//const char* ssid = "sala203"; //<----------Mudar para o seu
//const char* password = "s@l@203#"; //<----------Mudar para o seu

const char* mqttserver = "maqiatto.com";
const int mqttport = 1883;
const char* mqttUser = IO_USERNAME;
const char* mqttPassword = IO_KEY;

WiFiClient espClient;
PubSubClient client(espClient);

#define MSG_BUFFER_SIZE	(50)

char msg[MSG_BUFFER_SIZE];
unsigned long lastMsg = 0;
int value = 0;

#define TRIG 12
#define ECHO 14
#define SOUND_SPEED 0.034

long duration;
float distanceCm;

void setup_wifi() {

  delay(10);
 
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Messagem recebida [");
  Serial.print(topic);
  Serial.print("] ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    //messageTemp += (char)payload[i]; <----------Usar quando tiver uma mensagem na resposta do bloco
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    // Create a random client ID
    String clientId = "ESP32 - Sensores";
    clientId += String(random(0xffff), HEX);
    // Se conectado
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("conectado");
      // Depois de conectado, publique um anúncio ...
      client.publish("gabrielcarvalhopsilva@gmail.com/distance", "Iniciando Comunicação");
      //client.subscribe("3duardonogueira@gmail.com/led"); // <<<<----- mudar aqui
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  setup_wifi();
  client.setServer(mqttserver, 1883); // Publicar
  client.setCallback(callback); // Receber mensagem
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(10000); //Intervalo de 10s entre leituras
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    // Clears the trigPin
    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);

    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(ECHO, HIGH);
    distanceCm = duration * SOUND_SPEED/2;

    char s_dist[8];
    dtostrf(distanceCm,1,2,s_dist);
    Serial.print("Distância: ");
    Serial.println(s_dist);
    Serial.println(distanceCm);
    client.publish("gabrielcarvalhopsilva@gmail.com/distance", s_dist);

  }
}

