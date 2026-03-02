#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif
// Comment or uncomment the following line to display or not debug traces in serial monitor of Arduino IDE
//#define DEBUG_TRACE

#include "Zigbee.h"
#include "DFRobot_CCS811.h"
#include "DFRobot_AHT20.h"

#define ANALOG_DEVICE_ENDPOINT_NUMBER 1

#include <Wire.h>
#include "DFRobot_CCS811.h"
#include "DFRobot_AHT20.h"
#include <cmath>

uint8_t analogPin = 4;
uint8_t button = BOOT_PIN;
DFRobot_AHT20 aht20;
DFRobot_CCS811 CCS811;

/* Zigbee temperature + humidity sensor configuration */
#define TEMP_SENSOR_ENDPOINT_NUMBER 10
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 20        /* Sleep for 20 s */

float temp = 25;
float hum = 50;

ZigbeeAnalog zbAnalogDevice1 = ZigbeeAnalog(ANALOG_DEVICE_ENDPOINT_NUMBER);
ZigbeeAnalog zbAnalogDevice2 = ZigbeeAnalog(ANALOG_DEVICE_ENDPOINT_NUMBER + 1);
ZigbeeAnalog zbAnalogDevice3 = ZigbeeAnalog(ANALOG_DEVICE_ENDPOINT_NUMBER + 2);
ZigbeeAnalog zbAnalogDevice4 = ZigbeeAnalog(ANALOG_DEVICE_ENDPOINT_NUMBER + 3);
ZigbeeAnalog zbAnalogDevice5 = ZigbeeAnalog(ANALOG_DEVICE_ENDPOINT_NUMBER + 4);
ZigbeeAnalog BatteryPercent = ZigbeeAnalog(ANALOG_DEVICE_ENDPOINT_NUMBER + 5);

void setup_HW(){
  // Init button switch
  pinMode(button, INPUT_PULLUP);

  // Pin led
  pinMode(LED_BUILTIN, OUTPUT);

  // Init analog in
  pinMode(analogPin, INPUT);

  // Analog in battery
  pinMode(0,INPUT);

  // Set analog resolution to 10 bits
  analogReadResolution(10);


}

void setup_ZB(){
  //Zigbee.factoryReset(false);
  // Optional: set Zigbee device name and model
  zbAnalogDevice1.setManufacturerAndModel("Groupe 4", "Capteur Tubercules");

  // Set up analog input 1
  zbAnalogDevice1.addAnalogInput();
  zbAnalogDevice1.setAnalogInputApplication(ESP_ZB_ZCL_AI_TEMPERATURE_OTHER);
  zbAnalogDevice1.setAnalogInputDescription("Temperature C");
  zbAnalogDevice1.setAnalogInputResolution(0.01);
  zbAnalogDevice1.setAnalogInputMinMax(-100,100);
   // Set up analog input 2
  zbAnalogDevice2.addAnalogInput();
  zbAnalogDevice2.setAnalogInputApplication(ESP_ZB_ZCL_AI_HUMIDITY_OTHER);
  zbAnalogDevice2.setAnalogInputDescription("Humidity %");
  zbAnalogDevice2.setAnalogInputResolution(0.01);
  zbAnalogDevice2.setAnalogInputMinMax(0,100);
  // Set up analog input 3
  zbAnalogDevice3.addAnalogInput();
  zbAnalogDevice3.setAnalogInputApplication(ESP_ZB_ZCL_AI_PPM_OTHER);
  zbAnalogDevice3.setAnalogInputDescription("CO2 PPM");
  zbAnalogDevice3.setAnalogInputResolution(0.01);
  zbAnalogDevice3.setAnalogInputMinMax(0,1000000);
   // Set up analog input 4
  zbAnalogDevice4.addAnalogInput();
  zbAnalogDevice4.setAnalogInputApplication(ESP_ZB_ZCL_AI_COUNT_UNITLESS_COUNT);
  zbAnalogDevice4.setAnalogInputDescription("VOC PPB");
  zbAnalogDevice4.setAnalogInputResolution(0.01);
  zbAnalogDevice4.setAnalogInputMinMax(0,1000000);
  // Set up analog input 5
  zbAnalogDevice5.addAnalogInput();
  zbAnalogDevice5.setAnalogInputApplication(ESP_ZB_ZCL_AI_COUNT_UNITLESS_COUNT);
  zbAnalogDevice5.setAnalogInputDescription("Luminosité %");
  zbAnalogDevice5.setAnalogInputResolution(0.01);
  zbAnalogDevice5.setAnalogInputMinMax(0,100);
  // Set up battery EP
  BatteryPercent.addAnalogInput();
  BatteryPercent.setAnalogInputApplication(ESP_ZB_ZCL_AI_COUNT_UNITLESS_COUNT);
  BatteryPercent.setAnalogInputDescription("Niveau de batterie %");
  BatteryPercent.setAnalogInputResolution(0.01);
  BatteryPercent.setAnalogInputMinMax(0,100);

  // Add endpoints to Zigbee Core
  Zigbee.addEndpoint(&zbAnalogDevice1);
  Zigbee.addEndpoint(&zbAnalogDevice2);
  Zigbee.addEndpoint(&zbAnalogDevice3);
  Zigbee.addEndpoint(&zbAnalogDevice4);
  Zigbee.addEndpoint(&zbAnalogDevice5);
  Zigbee.addEndpoint(&BatteryPercent);
}

void start_ZB(){
  #ifdef DEBUG_TRACE
  Serial.println("Starting Zigbee...");
  #endif
  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin(ZIGBEE_END_DEVICE)) {
    #ifdef DEBUG_TRACE
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    #endif
    ESP.restart();
  } else {
    #ifdef DEBUG_TRACE
    Serial.println("Zigbee started successfully!");
    #endif
  }
  #ifdef DEBUG_TRACE
  Serial.println("Connecting to network");
  #endif
  while (!Zigbee.connected()) {
    #ifdef DEBUG_TRACE
    Serial.print(".");
    #endif
    delay(100);
  }
  #ifdef DEBUG_TRACE
  Serial.println("Connected");
  #endif
}

void start_sensors(){
  // Sensor init
  #ifdef DEBUG_TRACE
  Serial.println("Starting sensors");
  Serial.println("Starting aht20");
  #endif

  uint8_t status;
  while((status = aht20.begin()) != 0){
    #ifdef DEBUG_TRACE
    Serial.print("AHT20 sensor initialization failed. error status : ");
    Serial.println(status);
    #endif
    delay(1000);
  }
  #ifdef DEBUG_TRACE
  Serial.println("Starting CCS811");
  #endif

  while(CCS811.begin() != 0){
      #ifdef DEBUG_TRACE
      Serial.println("failed to init chip, please check if the chip connection is fine ");
      #endif
      delay(1000);
  }
  #ifdef DEBUG_TRACE
  Serial.print("current configured parameter code is ");
  Serial.println(CCS811.getMeasurementMode(),BIN);
  #endif
  CCS811.setMeasurementMode(CCS811.eCycle_250ms);
  CCS811.setInTempHum(/*temperature=*/25,/*humidity=*/50);
}

void update_AHT_values(){
  
  //aht20
    bool aht20ok = false;
    for(int i = 0; i < 50; i++){
      if(aht20.startMeasurementReady()){
        aht20ok = true;
        break;
      }
      delay(100);
    }
    if(aht20ok){
      temp = aht20.getTemperature_C();
      hum = aht20.getHumidity_RH();
        // update ZB Analog value
      zbAnalogDevice1.setAnalogInput(temp);
      zbAnalogDevice2.setAnalogInput(hum);
        // debug print
      #ifdef DEBUG_TRACE
      Serial.print("temp: ");
      Serial.print(temp);
      Serial.print("  hum: ");
      Serial.println(hum);
      #endif
    }
    else{
      #ifdef DEBUG_TRACE
      Serial.println("ERROR: AHT20 took too long to respond (>5s)");
      #endif
    }
}

void update_CCS_values(){
  
    //CCS811
    
    CCS811.setInTempHum(temp, hum); // Update calibration Temperature and Humidity for more accurate measurements

    bool CCS811ok = false;
    for(int i = 0; i < 50; i++){
      if(CCS811.checkDataReady()){
        CCS811ok = true;
        break;
      }
      delay(100);
    }

    if(CCS811ok){
      uint16_t co2ppm = CCS811.getCO2PPM();
      uint16_t VOCppb = CCS811.getTVOCPPB();

      zbAnalogDevice3.setAnalogInput(co2ppm);
      zbAnalogDevice4.setAnalogInput(VOCppb);
      #ifdef DEBUG_TRACE
      Serial.print("co2ppm: ");
      Serial.print(co2ppm);
      Serial.print("  VOCppb: ");
      Serial.println(VOCppb);
      #endif
    }
    else{
      #ifdef DEBUG_TRACE
      Serial.println("ERROR: CCS811 took too long to respond (>5s)");
      #endif
    }
}

void update_lumi_values(){
    
    //Luminosité moyenne sur 10 mesures
    float lumi = 0;
    for(int i = 0; i<10; i++){
      lumi += analogReadMilliVolts(analogPin);
      delay(50);
    }

    float lumimoy = lumi/10;  // tension en mV
    float lumipercent = lumimoy/33;  //pourcentage de full range
    #ifdef DEBUG_TRACE
    Serial.print("lumipercent: ");
    Serial.print(lumipercent);
    Serial.print("lumimoy: ");
    Serial.println(lumimoy);
    #endif

    zbAnalogDevice5.setAnalogInput(lumipercent);
}

void update_battery(){
  int analogVolts = analogReadMilliVolts(0)*2;

  //estimation courbe de charge
  int percent = 123.0 - (123.0/(pow(1+pow(analogVolts/3700.0,80),0.165)));

  #ifdef DEBUG_TRACE
  Serial.print("Voltage Batt: ");
  Serial.println(analogVolts);
  Serial.print("Percent Batt: ");
  Serial.println(percent);
  #endif

  BatteryPercent.setAnalogInput(percent);
}
void report_values(){
  // Report ZB Analog values
  #ifdef DEBUG_TRACE
  Serial.println("Report values");
  #endif
  
  zbAnalogDevice1.reportAnalogInput();
  zbAnalogDevice2.reportAnalogInput();
  zbAnalogDevice3.reportAnalogInput();
  zbAnalogDevice4.reportAnalogInput();
  zbAnalogDevice5.reportAnalogInput();
  BatteryPercent.reportAnalogInput();
}

// Get data from sensor and go to deep sleep mode
void measureAndSleep() {

  
  #ifdef DEBUG_TRACE
  Serial.printf("Measure and sleep");
  #endif
  update_AHT_values(); // temperature + humidity
  update_CCS_values(); // eCO2 + VOC
  update_lumi_values(); // luminosity
  update_battery();
  report_values(); // report values over ZB
  // Turn on the builtin LED for a very short time
  flashLED(1);
  // Add small delay to allow the data to be sent before going to sleep
  delay(500);
  // Put device to deep sleep
  #ifdef DEBUG_TRACE
  Serial.printf("Going to sleep for %d seconds\r\n", TIME_TO_SLEEP);
  #endif
  esp_deep_sleep_start();
}
// Internal Led flash (n times)
void flashLED(int n) {
  for (int i = 0; i < n; i++) {
    // Turn on LED for 100ms
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
  }
}
/********************* Arduino functions **************************/
void setup() {
#ifdef DEBUG_TRACE
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("Starting Capteur Stockage Tubercules!");
#endif
  
  
  
  digitalWrite(LED_BUILTIN, HIGH);

  // Internal LED flash twice to indicate device start
  flashLED(2);

  // Init sensors
  setup_HW(); // setup IOs 
  
  
  
  // Configure the wake up source and set to wake up every 5 seconds
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  setup_ZB(); // setup EPs Zigbee
  start_ZB(); // start in End Device Mode and wait for connection

  //esp_zb_cfg_t zigbeeConfig = ZIGBEE_DEFAULT_ED_CONFIG();
  start_sensors(); // start sensor communication
  // Delay approx 1s (may be adjusted) to allow establishing proper connection with coordinator, needed for sleepy devices
  delay(3000);
  // Call the function to measure temperature and put the device to deep sleep
  measureAndSleep();
}
void loop() {
  // No actions are performed in the loop (the ESP32C6 enters the setup function when it exits deep sleep).
}