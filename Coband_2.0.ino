/* On ARDUINO UNO I2C pins are : A4(SDA) & A5(SCL) */

#include <Wire.h>

#include <OneWire.h>
#include <DallasTemperature.h>


#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"

#define ONE_WIRE_BUS 2 //Data Wire is plugged into 2 on the Arduino

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

MAX30105 particleSensor;

const byte RATE_SIZE = 4; // Increase this for averaging larger number of values 
byte rates[RATE_SIZE]; //Array of Heart Rates
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute;
int beatAvg;

void setup()
{
    Serial.begin(115200);
    Serial.println("Initializing Max Sensor ...");
    if(!particleSensor.begin(Wire,I2C_SPEED_FAST))
    {
        Serial.println("MAX30105 was not found, please check the wiring");
        while(1);
    }
    /*pinMode(pulseLED, OUTPUT);
    pinMode(readLED, OUTPUT);
    
    byte ledBrightness = 60; //Options: 0=Off to 255=50mA
    byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
    byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
    byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
    int pulseWidth = 411; //Options: 69, 118, 215, 411
    int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

    particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
    */

    particleSensor.setup(); //Configure sensor with default settings
    particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
    particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
    
    delay(2000);
    
    Serial.println("Initializing Temperature Sensor ...");
    sensors.begin();
    delay(2000);   
}
void loop()
{
  
  Serial.println("Press on the Heart Rate Sensor / SPO2");
  delay(2000);  
  while (1)
  {
    int avg=Heart_Sensor();
    if (avg>60)
    break;
    delay(100);
  }

  /*for(int i=0; i<25; i++)
  {
    SPO2();
    delay(3000);
  }*/
  
  Serial.println("Touch the Temperature Sensor");
  delay(5000);
  temperature();
  delay(5000);
}

int Heart_Sensor()
{
  while(1)
  {
  long irValue = particleSensor.getIR();

    if (checkForBeat(irValue) == true)
    {
        // We sensed a beat!
        long delta = millis() - lastBeat;
        lastBeat = millis();

        beatsPerMinute = 60 / (delta / 1000.0);

        if (beatsPerMinute < 255 && beatsPerMinute > 20)
        {
            rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the array
            rateSpot %= RATE_SIZE;                    // Wrap variable

            // Take average of readings
            beatAvg = 0;
            for (byte x = 0; x < RATE_SIZE; x++)
                beatAvg += rates[x];
            beatAvg /= RATE_SIZE;
        }
    }
    Serial.print("IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.print(beatAvg);

    if (irValue < 50000)
        Serial.print(" No finger?");


    Serial.println();
    if(beatAvg>75 && beatsPerMinute>75)
    { 
      return beatAvg;
    }

  }
    
}

void temperature()
{
    // call sensors.requestTemperatures() to issue a global temperature
    // request to all devices on the bus
    Serial.println("Requesting temperatures...");
    sensors.requestTemperatures(); // Send the command to get temperatures

    Serial.print("Temperature is: ");
    Serial.print(sensors.getTempCByIndex(0)); // Why "byIndex"?
    // You can have more than one IC on the same bus.
    // 0 refers to the first IC on the wire
    Serial.println();
}

/*void SPO2()
{
  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

  //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample

    Serial.print(F("red="));
    Serial.print(redBuffer[i], DEC);
    Serial.print(F(", ir="));
    Serial.println(irBuffer[i], DEC);
  }

  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  while (1)
  {
    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

      digitalWrite(readLED, !digitalRead(readLED)); //Blink onboard LED with every data read

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      //send samples and calculation result to terminal program through UART
      Serial.print(F("red="));
      Serial.print(redBuffer[i], DEC);
      Serial.print(F(", ir="));
      Serial.print(irBuffer[i], DEC);

      Serial.print(F(", HR="));
      Serial.print(heartRate, DEC);

      Serial.print(F(", HRvalid="));
      Serial.print(validHeartRate, DEC);

      Serial.print(F(", SPO2="));

      Serial.print(F(", SPO2Valid="));
      Serial.println(validSPO2, DEC);
    }

    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  }
}*/
