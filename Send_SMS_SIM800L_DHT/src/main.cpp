#include <Arduino.h>
#include <Wire.h>
#include <TinyGsmClientSIM800.h>
#include "DHT.h"
// SIM card PIN (leave empty, if not defined)
const char simPIN[]   = "";

#define SMS_TARGET  "+84971000903"

// Define your temperature Threshold (in this case it's 28.0 degrees Celsius)
float temperatureThreshold = 28.0;
// Flag variable to keep track if alert SMS was sent or not
bool smsSent = false;

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb

#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// TTGO T-Call pins
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22

// Set serial for debug console (to Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to SIM800 module)
#define SerialAT  Serial1

// Define the serial console for debug prints, if needed
// #include <StreamDebugger.h>
// #define DUMP_AT_COMMANDS
// StreamDebugger debugger(SerialAT, SerialMon);
// TinyGsm modem(debugger);

typedef TinyGsmSim800  TinyGsm;
TinyGsm modem(SerialAT);

#define IP5306_ADDR          0x75
#define IP5306_REG_SYS_CTL0  0x00

bool setPowerBoostKeepOn(int en){
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  if (en) {
    Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  } else {
    Wire.write(0x35); // 0x37 is default reg value
  }
  return Wire.endTransmission() == 0;
}

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);

  // Keep power when running from battery
  Wire.begin(I2C_SDA, I2C_SCL);
  bool isOk = setPowerBoostKeepOn(1);
  SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));

  // Set modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Restart SIM800 module, it takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
  // use modem.init() if you don't need the complete restart

  // Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(simPIN);
  }
  
  // Start the DS18B20 sensor
  dht.begin();
}

void loop() {
  dht.read();
  // Temperature in Celsius degrees 
  float temperature = dht.readTemperature(0);
  SerialMon.print(temperature);
  SerialMon.println("*C");
  
  // Humidity in Celsius degrees 
  float humidity = dht.readHumidity(0);
  SerialMon.print(humidity);
  SerialMon.println("%");
  // Temperature in Fahrenheit degrees
  /*float temperature = sensors.getTempFByIndex(0);
  SerialMon.print(temperature);
  SerialMon.println("*F");*/

  // Check if temperature is above threshold and if it needs to send the SMS alert
  if((temperature > temperatureThreshold) && !smsSent){
    String smsMessage = String("Temperature above threshold: ") + 
           String(temperature) + String("C");
    if(modem.sendSMS(SMS_TARGET, smsMessage)){
      SerialMon.println(smsMessage);
      smsSent = true;
    }
    else{
      SerialMon.println("SMS failed to send");
    }    
  }
  // Check if temperature is below threshold and if it needs to send the SMS alert
  else if((temperature < temperatureThreshold) && smsSent){
    String smsMessage = String("Temperature below threshold: ") + 
           String(temperature) + String("C");
    if(modem.sendSMS(SMS_TARGET, smsMessage)){
      SerialMon.println(smsMessage);
      smsSent = false;
    }
    else{
      SerialMon.println("SMS failed to send");
    }
  }
  delay(5000);
}