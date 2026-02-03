#include "Zigbee.h"
#include <Wire.h>
#include "DFRobot_CCS811.h"
#include "DFRobot_AHT20.h"

/* Zigbee analog device configuration */
#define ANALOG_DEVICE_ENDPOINT_NUMBER 1

uint8_t analogPin = 4;
uint8_t button = BOOT_PIN;
DFRobot_AHT20 aht20;
DFRobot_CCS811 CCS811;


// valeurs mesurées, 25c et 50% par défaut
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
  Serial.println("Starting sensors");
  Serial.println("Starting aht20");

  uint8_t status;
  while((status = aht20.begin()) != 0){
    Serial.print("AHT20 sensor initialization failed. error status : ");
    Serial.println(status);
    delay(1000);
  }
  Serial.println("Starting CCS811");

  while(CCS811.begin() != 0){
      Serial.println("failed to init chip, please check if the chip connection is fine ");
      delay(1000);
  }

  Serial.print("current configured parameter code is ");
  Serial.println(CCS811.getMeasurementMode(),BIN);
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
      Serial.print("temp: ");
      Serial.print(temp);
      Serial.print("  hum: ");
      Serial.println(hum);
    }
    else{
      Serial.println("ERROR: AHT20 took too long to respond (>5s)");
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

      Serial.print("co2ppm: ");
      Serial.print(co2ppm);
      Serial.print("  VOCppb: ");
      Serial.println(VOCppb);
    }
    else{
      Serial.println("ERROR: CCS811 took too long to respond (>5s)");
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
    Serial.print("lumipercent: ");
    Serial.print(lumipercent);
    Serial.print("lumimoy: ");
    Serial.println(lumimoy);
    
    zbAnalogDevice5.setAnalogInput(lumipercent);
}

void report_values(){
  // Report ZB Analog values
    zbAnalogDevice1.reportAnalogInput();
    zbAnalogDevice2.reportAnalogInput();
    zbAnalogDevice3.reportAnalogInput();
    zbAnalogDevice4.reportAnalogInput();
    zbAnalogDevice5.reportAnalogInput();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  setup_HW(); // setup IOs 
  setup_ZB(); // setup EPs Zigbee
  start_ZB(); // start in End Device Mode and wait for connection
  start_sensors(); // start sensor communication
  
  Serial.println("Ready !");
  delay(1000);
}



void loop() {
  static uint32_t timeCounter = 0;

  
  if (!(timeCounter++ % 100)) {  // delaying for 100ms x 100 = 10s

    update_AHT_values(); // temperature + humidity
    update_CCS_values(); // eCO2 + VOC
    update_lumi_values(); // luminosity

    report_values(); // report values over ZB
  }

  // Checking button for factory reset and reporting
  if (digitalRead(button) == LOW) {  // Push button pressed
    // Key debounce handling
    delay(100);
    int startTime = millis();
    while (digitalRead(button) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        // If key pressed for more than 3secs, factory reset Zigbee and reboot
        Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
        delay(1000);
        Zigbee.factoryReset();
      }
    }
  }

  delay(100);
}
