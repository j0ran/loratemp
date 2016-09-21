LoraTemp
========

Arduino project to measure the temperature and send the data using the RN2483.

The hardware is as follows:
* Ardiono UNO
* DHT11 temperature sensor connected to pin 2 of the arduino.
* RN2483 connected using serial to the Arduino (pin 0 and 1)

The sketch doesn't configure the RN2483. This should be done manually:
* DevEUI should be set: <code>mac set deveui 0000000000000000</code>
* AppEUI should be set: <code>mac set appeui 0000000000000000</code>
* AppKey should be set: <code>mac set appkey 00000000000000000000000000000000</code>
* The config should be saved: <code>mac save</code>


