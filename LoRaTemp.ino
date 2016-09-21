#include "DHT.h"

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

#define STATE_JOIN 0
#define STATE_TX 1

int state = 0;
int wait_time = 10;

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

void send(String &str) {
  Serial.print("mac tx cnf 1 ");
  for(int i = 0; i < str.length(); i++) {
    if(str[i] < 16) {
      Serial.print('0');
    }
    Serial.print(str[i], HEX);
  }
  Serial.print("\r\n");
}

void setup() {
  String res;
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Serial.begin(57600);
  dht.begin();
  
  delay(1000);
  
  clearInput();
  Serial.print("\r\n");
  res = readLine();
}

void loop() {
  String res;

  for(int i = 0; i < wait_time; i++) {
    if(state == STATE_JOIN) digitalWrite(13, i % 2 == 0 ? HIGH : LOW);
    else digitalWrite(13, HIGH);
    delay(1000);
  }

  wait_time = 300;

  if(state == STATE_JOIN) {
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
    clearInput();
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (isnan(h) || isnan(t)) {
      return;
    }
  
    String str = String("{\"temp\": ") + t + ", \"humid\": " + h + " }";
    send(str);
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
