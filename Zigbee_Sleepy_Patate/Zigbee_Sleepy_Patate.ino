//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/**
 * @brief Zigbee temperature and humidity sensor Zigbee Sleepy End Device.
 *
 * https://tutoduino.fr/tutoriels/esp32c6-zigbee/
 * This code is based on example "Zigbee temperature and humidity sensor Sleepy device" created by Jan Procházka 
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/Zigbee/examples/Zigbee_Temp_Hum_Sensor_Sleepy
 */
#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif
// Comment or uncomment the following line to display or not debug traces in serial monitor of Arduino IDE
#define DEBUG_TRACE
#include "Zigbee.h"
#define ANALOG_DEVICE_ENDPOINT_NUMBER 1

#include <Wire.h>
#include "DFRobot_CCS811.h"
#include "DFRobot_AHT20.h"

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

void setup_HW(){
  // Init button switch
  pinMode(button, INPUT_PULLUP);


  // Init analog in
  pinMode(analogPin, INPUT);
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
  zbAnalogDevice1.setAnalogInputDescription("Valeur 1: Temperature");
  zbAnalogDevice1.setAnalogInputResolution(0.01);
  zbAnalogDevice1.setAnalogInputMinMax(-100,100);
   // Set up analog input 2
  zbAnalogDevice2.addAnalogInput();
  zbAnalogDevice2.setAnalogInputApplication(ESP_ZB_ZCL_AI_HUMIDITY_OTHER);
  zbAnalogDevice2.setAnalogInputDescription("Valeur 2: Humidity");
  zbAnalogDevice2.setAnalogInputResolution(0.01);
  zbAnalogDevice2.setAnalogInputMinMax(0,100);
  // Set up analog input 3
  zbAnalogDevice3.addAnalogInput();
  zbAnalogDevice3.setAnalogInputApplication(ESP_ZB_ZCL_AI_PPM_OTHER);
  zbAnalogDevice3.setAnalogInputDescription("Valeur 3: CO2 PPM");
  zbAnalogDevice3.setAnalogInputResolution(0.01);
  zbAnalogDevice3.setAnalogInputMinMax(0,1000000);
   // Set up analog input 4
  zbAnalogDevice4.addAnalogInput();
  zbAnalogDevice4.setAnalogInputApplication(ESP_ZB_ZCL_AI_COUNT_UNITLESS_COUNT);
  zbAnalogDevice4.setAnalogInputDescription("Valeur 4: VOC PPB");
  zbAnalogDevice4.setAnalogInputResolution(0.01);
  zbAnalogDevice4.setAnalogInputMinMax(0,1000000);
  // Set up analog input 5
  zbAnalogDevice5.addAnalogInput();
  zbAnalogDevice5.setAnalogInputApplication(ESP_ZB_ZCL_AI_COUNT_UNITLESS_COUNT);
  zbAnalogDevice5.setAnalogInputDescription("Valeur 5: Luminosité lux");
  zbAnalogDevice5.setAnalogInputResolution(0.01);
  zbAnalogDevice5.setAnalogInputMinMax(0,100000);
  

  // Add endpoints to Zigbee Core
  Zigbee.addEndpoint(&zbAnalogDevice1);
  Zigbee.addEndpoint(&zbAnalogDevice2);
  Zigbee.addEndpoint(&zbAnalogDevice3);
  Zigbee.addEndpoint(&zbAnalogDevice4);
  Zigbee.addEndpoint(&zbAnalogDevice5);
}

void start_ZB(){
  
  Serial.println("Starting Zigbee...");

  // When all EPs are registered, start Zigbee in End Device mode
  if (!Zigbee.begin(ZIGBEE_END_DEVICE)) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  } else {
    Serial.println("Zigbee started successfully!");
  }
  Serial.println("Connecting to network");
  while (!Zigbee.connected()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("Connected");
}

void start_sensors(){
  // Sensor init
  Serial.println("not Starting sensors");
  
}

void update_AHT_values(){
  
  zbAnalogDevice1.setAnalogInput(temp++);
  zbAnalogDevice2.setAnalogInput(hum++);
}

void update_CCS_values(){
  
    zbAnalogDevice3.setAnalogInput(temp++);
    zbAnalogDevice4.setAnalogInput(temp++);
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
    Serial.print("lumipercent: ");
    Serial.print(lumipercent);
    Serial.print("lumimoy: ");
    Serial.println(lumimoy);
    
    zbAnalogDevice5.setAnalogInput(lumipercent);
}

void report_values(){
  // Report ZB Analog values
  #ifdef DEBUG_TRACE
  Serial.printf("Report values");
  #endif
  zbAnalogDevice1.setAnalogInput(111);
  zbAnalogDevice1.reportAnalogInput();
  zbAnalogDevice2.reportAnalogInput();
  zbAnalogDevice3.reportAnalogInput();
  zbAnalogDevice4.reportAnalogInput();
  zbAnalogDevice5.reportAnalogInput();
}

// Get data from sensor and go to deep sleep mode
void measureAndSleep() {

  
  #ifdef DEBUG_TRACE
  Serial.printf("Measure and sleep");
  #endif
  update_AHT_values(); // temperature + humidity
  update_CCS_values(); // eCO2 + VOC
  update_lumi_values(); // luminosity

  zbAnalogDevice1.reportAnalogInput();
  zbAnalogDevice2.reportAnalogInput();
  zbAnalogDevice3.reportAnalogInput();
  zbAnalogDevice4.reportAnalogInput();
  zbAnalogDevice5.reportAnalogInput();
  //report_values(); // report values over ZB
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
  Serial.println("Starting Sleepy Patate!");
#endif
  
  
  pinMode(LED_BUILTIN, OUTPUT);
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