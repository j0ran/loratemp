#include "DHT.h"
#include "LowPower.h"
#include <PinChangeInterrupt.h>

union Data {
  struct {
    unsigned char type;
    float temp;
    float humid;
    int light;
  } packet;
  unsigned char bytes[sizeof(packet)];
};

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

#define STATE_JOIN 0
#define STATE_TX 1

byte state = STATE_JOIN;
unsigned long wait_time = 10;
int rxpin = digitalPinToPinChangeInterrupt(0);

void clearInput() {
  while(Serial.read() != -1);
}

String readLine() {
  String line;

  while(true) {
    int c = Serial.read();
    
    if(c == -1) {
      delay(300);      
      continue;
    }

    if(c == '\n') {
      return line;
    }

    if(c != '\r') {
      if(line.length() < 120) {
        line += char(c);
      }
    }
  }
}

void send(Data &data) {
  Serial.print("mac tx cnf 1 ");
  for(int i = 0; i < sizeof(data.bytes); i++) {
    if(data.bytes[i] < 16) {
      Serial.print('0');
    }
    Serial.print(data.bytes[i], HEX);
  }
  Serial.print("\r\n");
}

void readSensors(Data &data) {
    // Turn on power to sensors
    digitalWrite(3, HIGH);
    digitalWrite(4, HIGH);
    delay(50);

    data.packet.type = 1;
    data.packet.light = 0;
    for(int i = 0; i < 20; i++) {
      data.packet.light += analogRead(0);
      delay(5);
    }
    data.packet.light /= 20;    
    data.packet.humid = dht.readHumidity();
    data.packet.temp = dht.readTemperature();  
}

void setup() {
  pinMode(13, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(13, HIGH);
  Serial.begin(57600);
  dht.begin();
  
  delay(1000);
  
  clearInput();
  Serial.print("\r\n");
  readLine();
}

void loop() {
  String res;

  // turn of power to sensors
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);

  if(wait_time > 10) {    
    clearInput();
    Serial.print("sys sleep ");
    Serial.print(wait_time * 1000);
    Serial.print("\r\n");
    Serial.flush();

    // if not connected keep led on, if connected led is off
    if(state == STATE_TX) digitalWrite(13, LOW);
    else digitalWrite(13, HIGH);

    // Go to sleep and wait for data on uart
    enablePCINT(rxpin);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    disablePCINT(rxpin);

    // Show wakeup by blinking led and receive rest of ok command
    for(int i = 1; i < 10; i++) {
      digitalWrite(13, i % 2 == 0 ? LOW : HIGH);
      delay(100);
    }

    clearInput(); // clear out buffer
  }
  else {    
    for(int i = 0; i < wait_time; i++) {
      if(state == STATE_JOIN) digitalWrite(13, i % 2 == 0 ? HIGH : LOW);
      else digitalWrite(13, HIGH);
      delay(1000);
    }
    digitalWrite(13, HIGH);
  }
  
  if(state == STATE_JOIN) {
    wait_time = 60;
    clearInput();
    
    Serial.print("mac join otaa\r\n");
    res = readLine();
    if(res != "ok") {
      return;
    }
    res = readLine();
    if(res != "accepted") {
      return;
    }
    state = STATE_TX;
  }
  else {
    wait_time = 300;
    clearInput();

    Data data;
    readSensors(data);
        
    if (isnan(data.packet.temp) || isnan(data.packet.humid)) {
      return;
    }
 
    send(data);
    res = readLine();
    if(res == "no_free_ch") {
      return;
    }
    if(res != "ok") {
      state = STATE_JOIN;
      return;
    }
    res = readLine(); // read the mac_rx 1 response... ignoring
  }
}
