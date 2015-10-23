#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
//#include "printf.h"
#include <Print.h>
const char* ssid = "SSID";
const char* password = "password";
const char* mqtt_server = "x.x.x.x";

const char* topic_info = "/Lights/Room2/Info";
const char* topic_cmd = "/Lights/Room2/Comm";
const char* mqtt_client = "ESP_Room2";

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 (CE & CS)
RF24 radio(0, 2);
// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;


int ledPin = 15;
bool ledState = LOW;
uint8_t button_state;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

  delay(10);

  // We start by connecting to a WiFi network
  //  Serial.println();
  //  Serial.print("Connecting to ");
  //  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //    Serial.print(".");
  }

  //  Serial.println("");
  //  Serial.println("WiFi connected");
  //  Serial.println("IP address: ");
  //  Serial.println(WiFi.localIP());
}

void setup() {
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  //  printf_begin();

  radio.begin();
  radio.openReadingPipe(1, pipe);
  radio.startListening();
  //radio.printDetails();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  if (!client.connected()) {
    reconnect();
  }

  ledState = HIGH;
  digitalWrite(ledPin, ledState);
  if (ledState) client.publish(topic_cmd, "1"); else client.publish(topic_cmd, "0");
}

void callback(char* topic, byte* payload, unsigned int length) {
  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");
  //for (int i = 0; i < length; i++) {
  //  Serial.print((char)payload[i]);
  //}
  //Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '0') {
    ledState = LOW;
    digitalWrite(ledPin, ledState);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else if ((char)payload[0] == '1') {
    ledState = HIGH;
    digitalWrite(ledPin, ledState);  // Turn the LED off by making the voltage HIGH
  } else if ((char)payload[0] == '2') {
    // Switch the LED state
    ledState ^= HIGH;
    digitalWrite(ledPin, ledState);
  } else if ((char)payload[0] == '3') {
    if (ledState) client.publish(topic_cmd, "1"); else client.publish(topic_cmd, "0");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client)) {
      //Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(topic_info, "Connected");
      // ... and resubscribe
      client.subscribe(topic_cmd);
    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop()
{
  // here is where you'd put code that needs to be running all the time.

  // check to see if it's time to blink the LED; that is, if the
  // difference between the current time and last time you blinked
  // the LED is bigger than the interval at which you want to
  // blink the LED.

  if (!client.connected()) {
    reconnect();
  }

  if ( radio.available() )
  {
    // Dump the payloads until we've gotten everything
    while (radio.available())
    {
      // Fetch the payload, and see if this was the last one.
      radio.read( &button_state, 1 );
      Serial.print("Got buttons ");
      Serial.println(button_state);
      client.publish(topic_info, "Get button " + button_state);
      // For each button, if the button now on, then toggle the LED

      if ( button_state == 2 )
      {
        ledState ^= HIGH;
        digitalWrite(ledPin, ledState);
        if (ledState) client.publish(topic_cmd, "1"); else client.publish(topic_cmd, "0");
      }
    }
  }

  client.loop();

}


