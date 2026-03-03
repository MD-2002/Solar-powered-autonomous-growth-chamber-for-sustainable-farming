//                                            //
//--------Solar Powered Growth Chamber--------//
#include <DHT11.h>

//----------constants-------------//
#define light 2 //2
#define fan 4 //13
#define waterpump 7 //7
#define echo 5
#define trigger 6
DHT11 dht11_inside(8);
DHT11 dht11_outside(9);
#define lux A4
#define soilsensor A5

//--------Variable declaration---------//
String input;
long duration, in_abl;
float distance;
unsigned long hours, previousHours = 0;

bool statuslamp = false, pumpRunning = false;
bool manualOverride = false;  // Manual override flag
unsigned long overrideTimer = 0; // Timer for manual override duration

//--------Data Transmission Optimization Variables---------//
unsigned long lastDataSend = 0;
const unsigned long DATA_SEND_INTERVAL = 0; // Send data every 500ms

// Previous values for change detection
int prev_soil = -1, prev_hmd = -1, prev_tmpC = -1;
int prev_hmd2 = -1, prev_tmpC2 = -1;
float prev_water = -1;
long prev_lux = -1;
bool prev_light = false, prev_fan = false, prev_pump = false;

//Ambient light sensor timer variables:
unsigned long dailyLightStartTime = 0;        // When the current daily cycle started (first time)
unsigned long currentSessionStartTime = 0;    // When current ON session started
unsigned long totalDailyLightTime = 0;        // Total accumulated ON time for today
bool dailyLightCycleCompleted = false;        // Has today's light cycle been completed?
bool lightManuallyControlled = false;         // Track if light is under manual control
const unsigned long DAILY_LIGHT_DURATION = 14400000; // 4 hours in milliseconds (change to 21600000 for 6 hours)
const unsigned long DAILY_CYCLE_PERIOD = 86400000;   // 24 hours in milliseconds
unsigned long lastLightCheck = 0;             // Last time we checked light conditions
const unsigned long LIGHT_CHECK_INTERVAL = 1000;     // Check every second


//----------Thresholds for humidity and temp sensors (inside & outside)-------------//
const int maxTemp = 23;
const int minTemp = 17;
const int maxHumidity = 75;
const int safeOutsideTemp = 18;
const int safeOutsideHumidity = 70;

//extern volatile unsigned long timer0_millis;
void setup(){
  Serial.begin(9600);
  //dht11.setDelay(500);
  pinMode(light, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(waterpump, OUTPUT);
  pinMode(lux, INPUT);
  pinMode(soilsensor, INPUT);
  pinMode(echo, INPUT);
  pinMode(trigger, OUTPUT);
  
  digitalWrite(light, HIGH);
  digitalWrite(fan, HIGH);
  digitalWrite(waterpump, HIGH);
}


void loop(){
  // Check if manual override should expire (after 5 minutes)
  if(manualOverride == true && (millis() - overrideTimer > 300000)){
    manualOverride = false;
    lightManuallyControlled = false; // Also reset light manual control
  }
  
  // Only run sensor functions if not in manual override mode
  if(manualOverride == false){
    
    //-------------Soil Function-------------//
    soil();

    //----------LightSensor Function---------//
    // Don't run automatic light control if manually overridden
    if (!lightManuallyControlled) {
      lightsensor();
    }
    
    //------Temperature and Humidity Function------//
    humid_temp();
    
    //--------------Send data at intervals only-------------//
    if(millis() - lastDataSend >= DATA_SEND_INTERVAL){
      sendAllData();
      lastDataSend = millis();
    }

  }else {
    // Even in manual override, check if daily light target should complete
    unsigned long currentTime = millis();
    bool lightCurrentlyOn = digitalRead(light);
    
    if (lightCurrentlyOn && currentSessionStartTime > 0 && !dailyLightCycleCompleted) {
      unsigned long currentSessionTime = currentTime - currentSessionStartTime;
      unsigned long potentialTotal = totalDailyLightTime + currentSessionTime;
      
      // Complete daily cycle if target duration reached
      if (potentialTotal >= DAILY_LIGHT_DURATION) {
        digitalWrite(light, LOW);
        totalDailyLightTime += currentSessionTime;
        dailyLightCycleCompleted = true;
        currentSessionStartTime = 0;
      }
    }
    
    // Still send data at intervals even in manual mode
    if(millis() - lastDataSend >= DATA_SEND_INTERVAL){
      sendAllData();
      lastDataSend = millis();
    }
  }
    
  //------Manual Actuators Control (Always Active)------// 
  SerialReader();
}


//---humidity and Temperature Sensors DHT11/22---//
//Microgreens & Baby Lettuce
void humid_temp(){
  int tmpC= 0, tmpC2=0;
  int hmd = 0, hmd2=0;

  // Attempt to read the temperature and humidity values from the DHT11 sensor.
  int result = dht11_inside.readTemperatureHumidity(tmpC, hmd);
  int result2 = dht11_outside.readTemperatureHumidity(tmpC2, hmd2);
  
  // Fan logic
  // Check if outside conditions are safe for ventilation
  bool outsideSafe = (tmpC2 >= safeOutsideTemp && hmd2 <= safeOutsideHumidity);

  if (!outsideSafe) {
      // Outside unsafe - keep fan OFF regardless of inside conditions
      digitalWrite(fan, HIGH);
  }
  else if (tmpC > maxTemp || hmd > maxHumidity) {
      // Inside needs cooling/dehumidifying AND outside is safe
      digitalWrite(fan, LOW);
  }
  else if (tmpC < minTemp) {
      // Inside too cold - turn fan OFF
      digitalWrite(fan, HIGH);
  }
  else {
      // Everything in acceptable range - fan OFF
      digitalWrite(fan, HIGH);
  }
  
  
}


//the light must be on for 4 to 6 hours daily
//And the light must be on with an irradiance of 400 to 520
//---Light Sensor BH1750---//
void lightsensor(){
  // Only check every second to avoid rapid switching
  unsigned long currentTime = millis();
  if (currentTime - lastLightCheck < LIGHT_CHECK_INTERVAL) {
    return;
  }
  lastLightCheck = currentTime;
  
  //0 to 65,535 lux
  in_abl = analogRead(lux);
  in_abl = map(in_abl, 1017, 344, 40, 700);
  
  // Check if 24 hours have passed since daily cycle started - reset for new day
  if (dailyLightStartTime > 0 && (currentTime - dailyLightStartTime >= DAILY_CYCLE_PERIOD)) {
    dailyLightCycleCompleted = false;
    dailyLightStartTime = 0;
    totalDailyLightTime = 0;
    currentSessionStartTime = 0;
  }
  
  // Get current light state
  bool lightCurrentlyOn = digitalRead(light);
  
  // If light is currently on, update accumulated time
  if (lightCurrentlyOn && currentSessionStartTime > 0) {
    unsigned long currentSessionTime = currentTime - currentSessionStartTime;
    unsigned long potentialTotal = totalDailyLightTime + currentSessionTime;
    
    // If the daily target is reached, complete the cycle
    if (potentialTotal >= DAILY_LIGHT_DURATION) {
      digitalWrite(light, LOW);
      totalDailyLightTime += currentSessionTime;
      dailyLightCycleCompleted = true;
      currentSessionStartTime = 0;
      return;
    }

  }
  
  // If daily cycle is already completed, keep light off
  if (dailyLightCycleCompleted) {
    if (lightCurrentlyOn) {
      digitalWrite(light, LOW);
      currentSessionStartTime = 0;
    }
    return;
  }
  
  //low light <12,000 lux
  if (in_abl < 400) {
    if (!lightCurrentlyOn) {
      // Turn on light and start new session
      digitalWrite(light, HIGH);
      currentSessionStartTime = currentTime;
      
      // If this is the first time today, record daily start time
      if (dailyLightStartTime == 0) {
        dailyLightStartTime = currentTime;
      }
    }
  }

  //12,000 to 20,000 lux  
  //20,000 lux switch off
  else if (in_abl > 520) {
    if (lightCurrentlyOn) {
      // Turn off light and save accumulated time
      unsigned long sessionTime = currentTime - currentSessionStartTime;
      totalDailyLightTime += sessionTime;
      digitalWrite(light, LOW);
      currentSessionStartTime = 0;
      
      // Check if we've completed the daily target
      if (totalDailyLightTime >= DAILY_LIGHT_DURATION) {
        dailyLightCycleCompleted = true;
      }
    }
  }
}


//---Reset Daily Light Cycle (call this to manually reset the 24-hour cycle)---//
void resetDailyLightCycle(){
  // If light is currently on, save the current session time first
  if (digitalRead(light) && currentSessionStartTime > 0) {
    totalDailyLightTime += (millis() - currentSessionStartTime);
  }
  
  dailyLightCycleCompleted = false;
  dailyLightStartTime = 0;
  totalDailyLightTime = 0;
  currentSessionStartTime = 0;
  digitalWrite(light, LOW);
}

//---Get Light Cycle Status---//
void getLightStatus() {
  unsigned long currentTime = millis();
  unsigned long currentTotal = totalDailyLightTime;
  
  // Add current session time if light is on
  if (digitalRead(light) && currentSessionStartTime > 0) {
    currentTotal += (currentTime - currentSessionStartTime);
  }

}


//---Soil Sensor (NO SERIAL PRINT)---//
void soil(){
  // Lettuce optimal range: 60-80% moisture
  // 60% of 688 = 408
  // 80% of 680 = 544

  if(analogRead(soilsensor) >= 408){
    digitalWrite(waterpump, HIGH);
    pumpRunning = false; // Reset pump state when soil is wet
  }else if(analogRead(soilsensor) < 408){ 
    //Serial.println("DRY");
    waterp();
  }
}


//----water pump----//
void waterp(){
  float waterml = waterlevel();

  // Check if we should start the pump
  if(waterml > 1000.00 && pumpRunning == false){
    digitalWrite(waterpump, LOW);
    pumpRunning = true;
    //1 pumpStartTime = millis();
  }
  
  // If water level is too low, turn off pump
  if(waterml < 1000.00){
    digitalWrite(waterpump, HIGH);
    pumpRunning = false;
  }

}


//---Water Sensor---//
float waterlevel(){
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
    
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  
  digitalWrite(trigger, LOW);
  
  duration = pulseIn(echo, HIGH);
  distance = duration * 0.0343 / 2;
  
  // Replace map() with direct calculation for precise values
  distance = 23 - distance;  // This inverts 3→20, 20→3 with decimals
  
  float volume_ml = 200.00 * distance - 500.00;

  return max(0,volume_ml);
}


//---Send all sensor data with optimization---//
void sendAllData(){
  // Get current sensor values
  int tmpC= 0, tmpC2=0;
  int hmd = 0, hmd2=0;

  // Attempt to read the temperature and humidity values from the DHT11 sensor.
  int result = dht11_inside.readTemperatureHumidity(tmpC, hmd);
  int result2 = dht11_outside.readTemperatureHumidity(tmpC2, hmd2);

  int soilValue = analogRead(soilsensor);
  float waterLevel = waterlevel();
  bool lightState = digitalRead(light)==LOW;
  bool fanState = digitalRead(fan)==LOW;
  bool pumpState = digitalRead(waterpump)==LOW;
  
  // Only send if something changed significantly (reduces unnecessary transmissions)
  if(abs(soilValue - prev_soil) > 2 || abs(waterLevel - prev_water) > 50 ||
     abs(in_abl - prev_lux) > 10 || hmd != prev_hmd || tmpC != prev_tmpC ||
     hmd2 != prev_hmd2 || tmpC2 != prev_tmpC2 ||
     lightState != prev_light || fanState != prev_fan || pumpState != prev_pump ||
     prev_soil == -1){ // Always send on first run
    
    //-----SendData with compact format------//
    Serial.print(soilValue);
    Serial.print(";");
    Serial.print(waterLevel); // Cast to int to reduce characters
    Serial.print(";");
    Serial.print(in_abl);
    Serial.print(";");
    Serial.print(hmd);
    Serial.print(";");
    Serial.print(tmpC);
    Serial.print(";");
    Serial.print(hmd2);
    Serial.print(";");
    Serial.print(tmpC2);
    Serial.print(";");
    Serial.print(lightState ? "1;" : "0;");
    Serial.print(fanState ? "1;" : "0;");
    Serial.println(pumpState ? "1;" : "0;");
    
    // Update previous values
    prev_soil = soilValue;
    prev_water = waterLevel;
    prev_lux = in_abl;
    prev_hmd = hmd;
    prev_tmpC = tmpC;
    prev_hmd2 = hmd2;
    prev_tmpC2 = tmpC2;
    prev_light = lightState;
    prev_fan = fanState;
    prev_pump = pumpState;
  }
}


//----------Updated Manual Override System-------------//
void SerialReader(){
  if(Serial.available() > 0){
    input = Serial.readString();
    input.trim();
    
    // Activate manual override mode when any input is received
    manualOverride = true;
    overrideTimer = millis();
    
    int command = input.toInt();
    
    switch(command){
      case 0:
        // Exit manual override mode if 0 is received
        manualOverride = false;
        lightManuallyControlled = false; // Reset light manual control
        break;
        
      case 1:
        digitalWrite(light, LOW); // Turn light ON
        break;

      case 2:
        digitalWrite(light, HIGH); // Turn light OFF
        break;

      case 3:
        digitalWrite(waterpump, LOW); // Turn water pump ON
        // Reset pump timer when manually controlled
        pumpRunning = false;
        break;
        
      case 4:
        digitalWrite(waterpump, HIGH); // Turn water pump OFF
        break;

      case 5:
        digitalWrite(fan, LOW); // Turn fan ON
        break;

      case 6:
        digitalWrite(fan, HIGH); // Turn fan OFF
        break;

      case 7:
        // Reset daily light cycle
        resetDailyLightCycle();
        lightManuallyControlled = false;
        break;
    }
  }
}