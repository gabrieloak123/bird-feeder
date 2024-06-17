#include<WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <esp_sntp.h>
#include <math.h>
#include <HX711.h>

const char* ssid = ""; //<----------Mudar para o seu
const char* password = ""; //<----------Mudar para o seu

const char* mqttUser = "gabrielcarvalhopsilva@gmail.com";
const char* mqttPassword = "";
const char* mqttserver = "maqiatto.com";
const int mqttport = 1883;

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *timeZone = "CET+3CEST,M10.5.0/3,M3.5.0";

const char* distanceTopic = "gabrielcarvalhopsilva@gmail.com/distance";
const char* weightTopic = "gabrielcarvalhopsilva@gmail.com/weight";
const char* lastFeedTopic = "gabrielcarvalhopsilva@gmail.com/lastFeed";
const char* lastVisitTopic = "gabrielcarvalhopsilva@gmail.com/lastVisit";
const char* manuallyFeedSubTopic = "gabrielcarvalhopsilva@gmail.com/manuallyFeed";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;


// HC-SR04 connection pins
//#define LED 25
#define TRIG 12
#define ECHO 14

// HX711 connection pins
#define SCK  4
#define DT   16
// Initialization
HX711 scale;

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

  // Switch on the LED if an 1 was received as first character
  //if ((char)payload[0] == '1') {
  //  digitalWrite(LED, HIGH);   // Turn the LED on (Note that LOW is the voltage level
  //  Serial.println("LED Ligado");    
  //} else {
  //  digitalWrite(LED, LOW);  // Turn the LED off by making the voltage HIGH
  //  Serial.println("LED Desligado");    
  //}

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
      client.publish(distanceTopic, "Iniciando Comunicação");
      client.publish(weightTopic, "Iniciando Comunicação");
      client.publish(lastFeedTopic, "Iniciando Comunicação");
      client.publish(lastVisitTopic, "Iniciando Comunicação");
      
      client.subscribe(manuallyFeedSubTopic);
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5s");
      delay(5000);
    }
  }
}

String getFormattedDate() {
    struct tm timeInfo;
    char date[30];
    if (!getLocalTime(&timeInfo)) {
      return "Horário indisponível";
    }

    snprintf(date, sizeof(date),"%d/%d/%d - %02d:%02d:%02d",
             timeInfo.tm_mday, timeInfo.tm_mon, timeInfo.tm_year + 1900,
             timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
  
    return date;
}

float getDistance() {
  const float soundSpeed = 0.034;

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH);

  return duration * soundSpeed/2; // distance in cm
}

float getWeight() {
  const float calibrationFactor = 419.8;

  scale.set_scale();

  return scale.get_units(10)/calibrationFactor;
}

void setup() {
  Serial.begin(115200);

  // Distance sensor
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Weight sensor
  scale.begin(DT,SCK);

  setup_wifi();
  client.setServer(mqttserver, 1883); // Setup server
  client.setCallback(callback);       // Receive Message

  configTzTime(timeZone, ntpServer1, ntpServer2);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  delay(10000); //Intervalo de 10s entre leituras
  unsigned long now = millis();

  if (now - lastMsg > 2000) {
    char distanceString[10];
    float distanceCm = getDistance();
    dtostrf(distanceCm,1,2,distanceString);
    Serial.print("Distância: ");
    Serial.println(distanceString);
    client.publish(distanceTopic, distanceString);

    char weightString[10];
    float weightGramms = getWeight();
    dtostrf(weightGramms,1,2,weightString);
    Serial.print("Peso: ");
    Serial.println(weightString);
    client.publish(weightTopic, weightString);

    String timeString = getFormattedDate();
    char converted[25];
    timeString.toCharArray(converted, sizeof(converted));
    Serial.print("Hora: ");
    Serial.println(timeString);
    client.publish(lastVisitTopic, converted);
  }
}
