#include <M5StickC.h>
#include <MAX30105.h>
#include "spo2_algorithm.h" 


MAX30105 particleSensor;

uint32_t irBuffer[50]; //infrared LED sensor data
uint32_t redBuffer[50];  //red LED sensor data

int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

bool doStart = false; // flag true if sensing should be started

void startSense();
void printToDisplay();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // to PC via USB

  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(2);
  
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.println("MAX30105 was not found. Please check wiring/power.");
    while (1);
  }
  M5.Lcd.setTextColor(BLUE);
  particleSensor.shutDown();
  M5.Lcd.println("A: start measuring");
}

void loop() {
  // put your main code here, to run repeatedly:
    M5.update();
  if (doStart) {
    startSense();
  }
  if (M5.BtnA.wasReleased()) {
    particleSensor.begin(Wire, I2C_SPEED_FAST);

    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setCursor(0,20);
    M5.Lcd.println("Touch,\n then wait.");

    doStart = true;
  } 
}

void startSense() 
{
  byte ledBrightness = 55; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 200; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings 

  bufferLength = 50; //buffer length of 50 stores 4 seconds of samples running at 25sps

  //read the first 50 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample

  //  Serial.print("red="); Serial.print(redBuffer[i], DEC);
  //  Serial.print(", ir=");  Serial.println(irBuffer[i], DEC);
  }

  //calculate heart rate and SpO2 after first 50 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  //Continuously taking samples from MAX30102. Heart rate and SpO2 are calculated every 1 second
  while (1)
  {
    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < bufferLength; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 25; i < bufferLength; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      //send samples and calculation result to terminal program through UART
/*
      Serial.print("red="); Serial.print(redBuffer[i], DEC);
      Serial.print(", ir=");  Serial.print(irBuffer[i], DEC);

      Serial.print(", HR="); Serial.print(heartRate, DEC);
      Serial.print(", HRvalid="); Serial.print(validHeartRate, DEC);
      Serial.print(", SPO2="); Serial.print(spo2, DEC);
      Serial.print(", SPO2Valid="); Serial.println(validSPO2, DEC);
*/  
 //     Serial.println((float)irBuffer[i]/redBuffer[i], 4);
      Serial.println(redBuffer[i], DEC);
 //     Serial.print(",");  Serial.println(irBuffer[i], DEC);


    }

    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    printToDisplay();
  }
}

void printToDisplay() 
{
  //M5.Lcd.clear(BLACK);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0,20);

  if(validSPO2 && validHeartRate) {
    M5.Lcd.print("HR: "); M5.Lcd.println(heartRate, DEC);
    M5.Lcd.print("SPO2: "); M5.Lcd.println(spo2, DEC);
  } else {
    M5.Lcd.print("Not valid");
  }
}