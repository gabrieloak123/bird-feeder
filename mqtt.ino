#include<WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <time.h>
#include <esp_sntp.h>
#include <math.h>
#include <HX711.h>

const char* ssid = "NPITI-IoT"; //<----------Mudar para o seu
const char* password = "NPITI-IoT"; //<----------Mudar para o seu

const char* mqttUser = "gabrielcarvalhopsilva@gmail.com";
const char* mqttPassword = "teste123";
const char* mqttserver = "maqiatto.com";
const int mqttport = 1883;

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *timeZone = "CET+3CEST,M10.5.0/3,M3.5.0";

const char* distanceTopic = "gabrielcarvalhopsilva@gmail.com/distance";
const char* weightTopic = "gabrielcarvalhopsilva@gmail.com/weight";
const char* lastFeedTopic = "gabrielcarvalhopsilva@gmail.com/lastFeed";
const char* lastVisitTopic = "gabrielcarvalhopsilva@gmail.com/lastVisit";
const char* manuallyFeedTopic = "gabrielcarvalhopsilva@gmail.com/manuallyFeed";

WiFiClient espClient;
PubSubClient client(espClient);

#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
unsigned long lastMsg = 0;
int value = 0;

// Servo
#define SERVO 14
Servo servo;

// HC-SR04
#define TRIG  5
#define ECHO  19

#define MIN_DISTANCE 18

// HX711
#define SCK 5
#define DT  18

#define MIN_WEIGHT 4
HX711 scale;
const float calibrationFactor = -80;


bool feedFlag = false;

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
  Serial.print("Message received [");
  Serial.print(topic);
  Serial.print("] ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    //messageTemp += (char)payload[i]; <----------Usar quando tiver uma mensagem na resposta do bloco
  }
  Serial.println();

  if ((char)payload[0] == '1') {
    feedFlag = true;
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Trying MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32 - Sensors";
    clientId += String(random(0xffff), HEX);
    // Se conectado
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.println("connected");
      // Depois de conectado, publique um anúncio ...
      client.publish(distanceTopic, "Initializing communication");
      client.publish(weightTopic, "Initializing communication");
      client.publish(lastFeedTopic, "Initializing communication");
      client.publish(lastVisitTopic, "Initializing communication");
      
      client.subscribe(manuallyFeedTopic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5s");
      delay(5000);
    }
  }
}

String getFormattedDate() {
    struct tm timeInfo;
    char date[30];
    if (!getLocalTime(&timeInfo)) {
      return "TIme unavailable";
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
  scale.power_down();
  delay(2000);
  scale.power_up();


  if(scale.get_units(10) < 0) {
    return 0;
  } else {
    return scale.get_units(10);
  }
}

void rotate_clockwise(int degrees) {
  for (int pos = 0; pos <= degrees; pos += 1) {
    servo.write(pos);
    delay(10);
  }
}

void rotate_anticlockwise(int degrees) {
  for (int pos = degrees; pos <= 0; pos -= 1) {
    servo.write(pos);
    delay(10);
  }
}

void setup() {
  Serial.begin(115200);

  // Distance sensor
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Weight sensor
  scale.begin(DT,SCK);
  scale.set_scale(calibrationFactor);
  scale.tare();

  // Servo
  servo.attach(SERVO, 500, 2400);

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
    // Distance
    char distanceString[10];
    float distanceCm = getDistance();
    dtostrf(distanceCm,1,2,distanceString);
    Serial.print("Distance: ");
    Serial.println(distanceString);
    client.publish(distanceTopic, distanceString);

    // Weight
    char weightString[10];
    float weightGramms = getWeight();
    dtostrf(weightGramms,1,2,weightString);
    Serial.print("Weight: ");
    Serial.println(weightString);
    client.publish(weightTopic, weightString);

    if(distanceCm < MIN_DISTANCE) {
        // Last Visit
        String timeString = getFormattedDate();
        char converted[25];
        timeString.toCharArray(converted, sizeof(converted));
        Serial.print("Time(visit): ");
        Serial.println(timeString);
        client.publish(lastVisitTopic, converted);
        
        if(feedFlag || weightGramms < MIN_WEIGHT) {
            // Last Feed
            timeString = getFormattedDate();
            timeString.toCharArray(converted, sizeof(converted));
            Serial.print("Time(feed): ");
            Serial.println(timeString);
            client.publish(lastFeedTopic, converted);
            
            Serial.println("Feed!");
            rotate_clockwise(180);
            
            rotate_anticlockwise(180);

            feedFlag = false;
        }
    }
  }
}

