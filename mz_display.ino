/************************************************************
 mz_display code                   
 V1.0.0                          
 (c) 2016 hofmann engineering    
                                 
 So this code will run on an Adafruit 
 HUZZAH ESP8266 Board. To program the board do:
   (1) Hold down the GPIO0 button, the red LED will be lit
   (2) While holding down GPIO0, click the RESET button
   (3) Release RESET, then release GPIO0
   (4) When you release the RESET button, the red LED will 
      be lit dimly, this means its ready to bootload
 Upload Speed is 115200 baud.

 This program puts the watt usage of our house, coming from a
 mqtt server on a sh1106 display.
*************************************************************/  
#include <Arduino.h>
#include <U8g2lib.h>
#include <stdlib.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

//#define DEBUG 1                           //Use this in debugging environment only  

#define WIFI_SSID "HWLAN"                   //WIFI SSID of your NETWORK
#define WIFI_PASSWORD "1234567890123456"    //WIFI Password of your NETWORK

#define SERIAL_BAUD_RATE  115200
#define MQTT_SERVER_IP "192.168.178.2"         //MQTT Server Address within your NETWORT
#define MQTT_PORT 1883                         //MQTT standard port is 1883
#define MQTT_CLIENT_NAME  "mqttdisplay"        //MQTT client name - here default is rxtx2mqtt01   

void setup_wifi();    //forward declaration to function setup_wifi();

WiFiClient espClient;                       //WiFiClient Object, needed to connect
PubSubClient client(espClient);             //Connection to MQTT Server

float watt = 0.0f;
char watt_s[7];

boolean wifi_established = false;
boolean mqtt_established = false;

//display object - uses the u8g2 lib
//SH1106 display 128x64
//U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 12, 13);
//SSD1306 Display
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, 5, 4, U8X8_PIN_NONE);

unsigned long timeelapsed = 0;
unsigned long timeframestart = 0;
unsigned int frame = 0;

/************************************
 function setup                  
   initializes everything
   (1) Serial connection
   (2) Wifi
   (3) MQTT Server connection
   (4) u8g2 lib for the display        
 parameter none                  
 return none                     
************************************/
void setup(void) {
  Serial.begin(SERIAL_BAUD_RATE);   //initialize the Serial object with speed of SERIAL_BAUD_RATE
  setup_wifi();                     //initialize the wifi connection
  client.setServer(MQTT_SERVER_IP, MQTT_PORT);  //initialize the connection to the mqtt_server
  u8g2.begin();
}
/************************************
 function update                  
   will be called on receive of a new
   mqtt message.        
 parameter 
   char* topic contains the topic
   byte* payload contains the message
   unsigned int length of the message                  
 return none                     
************************************/
void update(char* topic, byte* payload, unsigned int length) {
  int i=0;
  char message_buff[255];
  for(i=0; i<length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  watt = atof(message_buff);
  #ifdef DEBUG
    Serial.print(topic);
    Serial.print("=");
    Serial.print(message_buff);
    Serial.print("\n");
  #endif  
}
/************************************
 function setup_wifi                  
   starts the WiFi object and connects        
 parameter none                  
 return none                     
************************************/
void setup_wifi() {
  delay(10);
  #ifdef DEBUG
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
  #endif
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);     //connect to WiFi SSID and PASSWORD

  while (WiFi.status() != WL_CONNECTED) {   //wait until connected
    delay(500);
    #ifdef DEBUG
      Serial.print(".");
    #endif
  }
  #ifdef DEBUG
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
}
/************************************
 function reconnect                  
   restarts the MQTT connection        
 parameter none                  
 return none                     
************************************/
void reconnect() {
  #ifdef DEBUG
    Serial.println("Attempting MQTT connection...");
  #endif
  if (client.connect(MQTT_CLIENT_NAME)) {                  //client should connect
    mqtt_established = true;
    #ifdef DEBUG
      Serial.println("connected");
    #endif
    client.setCallback(update);
    boolean rc = client.subscribe("home/techroom/mz/analog");   //subscribe the topic to follow
    #ifdef DEBUG
      if (rc) {
         Serial.println("subscripted topic");      
      }
    #endif
  } else {
    mqtt_established = false;
    #ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
    #endif
  }
}
void render() {
  u8g2.clearBuffer();  
  if (mqtt_established) {
    u8g2.setFont(u8g2_font_profont12_tf);  // choose a suitable font
    u8g2.drawStr(0, 8, "MQTT");    
  } else {
    if (frame % 3 == 0) {
      u8g2.setFont(u8g2_font_profont12_tf);  // choose a suitable font
      u8g2.drawStr(0, 8, "MQTT");
    }
  }
  dtostrf(watt, 6, 0, watt_s);  //convert watts to string
  u8g2.setFont( u8g2_font_logisoso22_tf);  // choose a suitable font
  u8g2.drawStr(10, 27, watt_s);  //draw watts as a string on display
  u8g2.drawStr(110, 27, "W");   //and the unit behing
  u8g2.sendBuffer();            // transfer internal memory to the display
}
/************************************
 function loop                  
   loop function does
     (1) reconnect if connection lost
     (2) convert readed watts to string 
     (3) display watts on the display
 parameter none                  
 return none                     
************************************/
void loop(void) {
  timeframestart = millis();
  if (!client.connected()) {    //if connection is lost, reconnect
    reconnect();
  } else {
    client.loop();                //mqtt loop
  }

  render();
  
  frame++;
  if (frame >= 5) frame = 0;
  timeelapsed = millis();
}

