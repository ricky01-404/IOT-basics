#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define RedLed 22
#define GreenLed 23

String recv = "";

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  pinMode(RedLed, OUTPUT);
  pinMode(GreenLed, OUTPUT);

  SerialBT.begin("TEAM-1"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
}

void loop()
{
  while (SerialBT.available()) // Read until the bluetooth client is sending.
  {
    char string = SerialBT.read();
    recv = recv + string;
    delay(1);
    
  }
  
  if (recv == "A")
  {
    digitalWrite(RedLed, HIGH);
  }
  if (recv == "a")
  {
    digitalWrite(RedLed,LOW);
  }
  if (recv == "B")
  {
    digitalWrite(GreenLed, HIGH);
  }
  if (recv == "b")
  {
    digitalWrite(GreenLed,LOW);
  }
  if (recv == "AB")
  {
    digitalWrite(RedLed, HIGH);
    digitalWrite(GreenLed, HIGH);    
  }
  if (recv == "ab")
  {
    digitalWrite(RedLed, LOW);
    digitalWrite(GreenLed, LOW);   
  }
  recv = ""; // clearing the string.
}
