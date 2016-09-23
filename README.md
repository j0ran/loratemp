LoraTemp
========

Arduino project to measure the temperature and send the data using the RN2483.

The hardware is as follows:
* Ardiono UNO
* DHT11 temperature sensor connected to pin 2 of the arduino. The power line should be connected to pin 3.
* A photocell should be connected to A0 with power connected to pin 4. (using 10K resistor pulldown to GND)
* RN2483 connected using serial to the Arduino (pin 0 and 1)

The sketch doesn't configure the RN2483. This should be done manually:
* DevEUI should be set: <code>mac set deveui 0000000000000000</code>
* AppEUI should be set: <code>mac set appeui 0000000000000000</code>
* AppKey should be set: <code>mac set appkey 00000000000000000000000000000000</code>
* The config should be saved: <code>mac save</code>

Led indicator:
* Led is on - trying to join network
* Led is off - joined the network
* Blinking 300ms - waiting for data from RN2483

Startup:

1. blinking 1s for 5 seconds - startup
2. blinking 300ms - waiting for otaa join
3. if led goes off means joined, if led goes on means not joined.
4. wait 60 seconds
5. If not joined go to step 2.
6. measuring sensor data and sending it using the RN2483
7. blinking 300ms - waiting for confirmation.
8. wait 5 minutes
9. goto step 6

  
