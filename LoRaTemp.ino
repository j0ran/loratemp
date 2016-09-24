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

#define STATE_JOIN 0
#define STATE_TX 1

#define TOGGLE -1
#define SAVE -2
#define RESTORE -3

#define INITIAL_JOIN_INTERVAL 5
#define JOIN_RETRY_INTERVAL 60
int send_interval = 300;

DHT dht(DHTPIN, DHTTYPE);
byte state = STATE_JOIN;
unsigned long wait_time = INITIAL_JOIN_INTERVAL;
int rxpin = digitalPinToPinChangeInterrupt(0);

void clearInput() {
  while(Serial.read() != -1);
}

String readLine() {
  String line;

  led(SAVE);
  while(true) {
    int c = Serial.read();
    
    if(c == -1) {
      led(TOGGLE);
      delay(300);      
      continue;
    }

    if(c == '\n') {
      led(RESTORE);
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
    powerToSensors(HIGH);
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

    // We are done, turn of the power
    powerToSensors(LOW);
}

// put system in low power mode
void powerDown(int seconds) {
    // send sleep command
    clearInput();
    Serial.print("sys sleep ");
    Serial.print(wait_time * 1000);
    Serial.print("\r\n");
    Serial.flush();

    // Go to sleep and wait for data on uart
    enablePCINT(rxpin);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    disablePCINT(rxpin);

    // Show wakeup by blinking led and receive rest of ok command
    led(SAVE);
    for(int i = 1; i < 10; i++) {
      led(TOGGLE);
      delay(100);
    }
    led(RESTORE);
    
    clearInput(); // read rest of ok
}

void sleepBlink(int seconds) {
    led(SAVE);
    for(int i = 0; i < wait_time; i++) {
      led(TOGGLE);
      delay(1000);
    }
    led(RESTORE);
}

void powerToSensors(int state) {
  digitalWrite(3, state);
  digitalWrite(4, state);
}

void led(int state) {
  static int led_mem;

  if(state == SAVE) {
    led_mem = digitalRead(13);
  }
  else if(state == RESTORE) {
    digitalWrite(13, led_mem);
  }
  else if(state == TOGGLE) {
    digitalWrite(13, !digitalRead(13));
  }
  else { 
    digitalWrite(13, state);
  }
}

void setup() {
  delay(500);
  pinMode(1, OUTPUT); // take the tx pin
  digitalWrite(1, LOW); // pull low for break
  delay(50); // sending break
  Serial.begin(4800);
  Serial.print("\x55");
  
  pinMode(13, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  
  led(HIGH);
  powerToSensors(LOW);
  dht.begin();
  
  delay(500);

  // clear input and send a newline to clear the output
  clearInput();
  Serial.print("\r\n");
  readLine(); // read invalid_parm response
}

void loop() {
  String res;

  led(state == STATE_JOIN ? HIGH : LOW);

  if(wait_time > 10) {    
    powerDown(wait_time);
  }
  else {    
    sleepBlink(wait_time);
  }
  
  if(state == STATE_JOIN) {
    wait_time = JOIN_RETRY_INTERVAL;
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
    wait_time = send_interval;
    clearInput();

    Data data;
    readSensors(data);
        
    if (isnan(data.packet.temp) || isnan(data.packet.humid)) {
      return;
    }
 
    send(data);
    res = readLine();
    if(res == "not_joined") {
      state = STATE_JOIN;
      wait_time = JOIN_RETRY_INTERVAL;
      return;
    }
    if(res != "ok") {
      return;
    }
    res = readLine(); // read the mac_rx 1 response... ignoring
  }
}
