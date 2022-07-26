#include <math.h>
#include "ClosedCube_HDC1080.h"  // Include library for HDC1080 sensor
ClosedCube_HDC1080 hdc1080;  // Create HDC1080 sensor

// Calibration for temperature offset:
const double AmbOffsetTemp = 0;  // It's Celsius. For example, if your sensor shows 1.5°C above real temperature, you should write here: -1.50
// 0 would be no change to what the sensor say.

// Calibration for humidity offset:
const double AmbOffsetHum = 0;  // For example, if your sensor shows 1.5% above real humidity, you should write here: -1.50
// 0 would be no change to what the sensor say.

String rawInput1; //String for entered values
int input1; //int variable the entered value (string --> int)

//int temp_upper = 0; //upper temp tolerance
//int temp_lower = 2; //lower temp tolerance

double blotz = 5.6703 * pow(10, -8); //Boltzmann's constant

const float voltagePower = 5; //Constants for thermistor temp calculation
const double B = 3435;        // ''
const double T = 273.15 + 25; // ''
const double NTC = 10000;     // ''

#define output_A  11 // V_Out @ pin 11 //Setting up the pins for Voltage Outputs
#define output_B  10 // V_Out @ pin 10 // ''
#define output_C  9 // V_Out @ pin 9   // ''
#define output_GND1  6 // V_GND @ pin 6 // ''
#define output_GND2  5 // V_GND @ pin 5 // ''

void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT); //A0 receives incoming voltage from Thermoresponsive PCB
  hdc1080.begin(0x40);  // Initiate HDC1080 sensor

  delay(10000);

  if (Serial.available() > 0) { //If Bluetooth connected
    rawInput1 = Serial.parseInt(); //String rawInput1 = *characters entered on the phone*
    Serial.setTimeout(10000); //Allow 10 sec to enter values
    input1 = rawInput1.toInt(); //String --> int
  } else {
    Serial.print("No connection");
    Serial.print("\n");
    delay(6000);
  } 

}

void loop() {
  
  const int temp_upper = (input1) / 100; //Input1 integer would be ex: 3023 --> 3023/100 gives the integer 30. --> upper temp bound
  const int temp_lower = (input1) % 100; //Input1 integer would be ex: 3023 --> 3023%100 gives the integer 23. --> lower temp bound

  double slope_volt = ceil((1200000.00) / (1.00 * temp_upper - 1.00 * temp_lower)) / 1000000; //rounding the slope value to 6 decimal places. Voltage end points fixed at +/- 0.6V.
  double b_volt = ceil(6000.00 - (slope_volt * 10000.00 * temp_upper)) / 10000.00; //rounding the y-int to 4 decimal places. Using the point (+0.6, Upper_Temp).

  // Read HDC1080 sensor:
  double AmbSensorTemperature = hdc1080.readTemperature();  // Read temperature
  double AmbSensorHumidity = hdc1080.readHumidity();  // Read humidity

  // Offset HDC1080 temperature and humidity:
  double AmbSensorWithOffsetTemperature = AmbSensorTemperature + AmbOffsetTemp;  // It takes the sensor value and adds (or substract) whatever is the offset.
  double AmbSensorWithOffsetHumidity = AmbSensorHumidity + AmbOffsetHum;  // It takes the sensor value and adds (or substract) whatever is the offset.
  double RoundedAmbSensorWithOffsetTemperature = ceil(AmbSensorWithOffsetTemperature * 100) / 100; //Rounding skin temp to 2 demical places
  double RoundedAmbSensorWithOffsetHumidity = ceil(AmbSensorWithOffsetHumidity * 100) / 100; //Rounding skin temp to 2 demical places
  byte whole_AmbTemp = 0;
  byte decimal_AmbTemp = 0;
  byte whole_AmbHumidity = 0;
  byte decimal_AmbHumidity = 0;

  double ADC0 = analogRead(A0); //read incoming voltage from Thermoresponsive PCB
  double NTC0 = 71500 / (1.01 * ADC0 * 5.00 / 1023 + 5); //Resistance of therm
  double raw_temp = (((T * B) / (B + T * log(NTC0 / NTC))) - 273.15); //Skin temperature
  double temp = ceil(raw_temp * 100) / 100; //Rounding skin temp to 2 demical places
  byte whole_temp = 0; //dummy value for the whole nummber part of skin temp. (i.e.: 26.82) --> whole_temp = 26  **this will be displayed on the mobile app**
  byte decimal_temp = 0; //dummy value for the decimal number part of skin temp. (i.e.: 26.82) --> whole_temp = 82 **this will be displayed on the mobile app**

  double tar_voltage = (slope_volt * raw_temp) + b_volt; //Voltage Output for WeaVE, based on the slope of upper_temp and lower_temp
  double voltage  = ceil(tar_voltage * 100) / 100; //rounding Voltage Output for WeaVE to 2 demical places
  byte whole_voltage = 0; //this will constantly be 0 since [+/-0.6V]  **this will be displayed on the mobile app**
  byte decimal_voltage = 0; //dummy value for decimal number of Voltage Output for WeaVE **this will be displayed on the mobile app**

  //Emissivity Equation
  double raw_emissivity = 0.8333 * (tar_voltage) + 0.5; //Currently 0 to 0.99 b/w entered temperatures. **ACTUAL FUNCTION REQUIRED**
  double emissivity = 0.00;
  if (raw_emissivity <= 0) {
    emissivity = 0.00; //33.87 --> 34/100 ---> 0.34 //Temperature can fall below/above the enter values, therefore we set anything below lower_temp to be 0.00
  }
  else if (raw_emissivity >= 0.995) {
    emissivity = 0.99; //Max value set at 0.99, since 0.995 will be rounded to 1, setting it directly to 0.99
  }
  else {
    emissivity = ceil(raw_emissivity * 100) / 100; //33.87 --> 34/100 ---> 0.34 //Anything between 0 and 0.99 are normall rounded to 2 decimal places
  }
  byte decimal_emissivity = 0; //dummy value for WeaVE emissivity **this will be displayed on the mobile app**

  //Heat Loss Equation
  double raw_heat_loss = (emissivity) * (blotz) * ((pow((1.00 * raw_temp + 273.15), 4)) - (pow((1.00 * AmbSensorWithOffsetTemperature + 273.15), 4))); //**ACTUAL EQUATION REQUIRED. NEED DOUBLE CHECK*
  double heat_loss = ceil(raw_heat_loss * 100) / 100; //round to two decimal places
  byte whole_heat_loss = 0; //**this will be displayed on the mobile app**
  byte decimal_heat_loss = 0; //**this will be displayed on the mobile app**

  byte runTime = millis() / 1000; //Time in terms of seconds
  byte myMin = 0; //**this will be displayed on the mobile app**
  byte mySec = 0; //**this will be displayed on the mobile app**


  ////HDC1080 Amb_Temp and Humidity Processing////
  int AmbTempWithoutDecimals = RoundedAmbSensorWithOffsetTemperature * 100;
  whole_AmbTemp = AmbTempWithoutDecimals / 100; //whole ambient temp from HDC1080
  decimal_AmbTemp = AmbTempWithoutDecimals % 100; //decimal ambient from HDC1080

  int AmbHumidityWithoutDecimals = RoundedAmbSensorWithOffsetHumidity * 100;
  whole_AmbHumidity = AmbHumidityWithoutDecimals / 100; //whole ambient humidity from HDC1080
  decimal_AmbHumidity = AmbHumidityWithoutDecimals % 100; //whole ambient humidity from HDC1080


  ////Emissivity Processing////
  int a_emissivity = (emissivity * 100);
  decimal_emissivity = (a_emissivity) % 100; //ex: 34%100 = 34


  ////Heat Loss Processing//////
  int a_heat_loss = (heat_loss * 100);
  whole_heat_loss = (a_heat_loss) / 100;    //ex: 2456/100 = 24
  decimal_heat_loss = (a_heat_loss) % 100; // ex: 2456%100 = 56
  /////////////////////////

  ////Voltage Processing////
  if (voltage <= -0.6) { //Below or equal to -0.6V
    whole_voltage = 0;
    int a_voltage = 60;
    decimal_voltage = (a_voltage) % 100; //(60)%100 = -60
  }

  else if (voltage > -0.6 && voltage <= 0) { //Between -0.59V and 0.0V
    whole_voltage = 0;
    int b_voltage = (voltage * -100);
    decimal_voltage = (b_voltage) % 100; //28%100 = 28
  }

  else if (voltage > 0 && voltage < 0.6) { //Between 0.01V to 0.59V
    whole_voltage = 0;
    int c_voltage = (voltage * 100);
    decimal_voltage = (c_voltage) % 100; //58%100 = 58
  }

  else { //voltage >= 0.6 //Above or equal to +0.6V
    whole_voltage = 0;
    int d_voltage = 60;
    decimal_voltage = (d_voltage) % 100; //60%100 = 60
  }

  ////Temp Processing//////
  int a_temp = (temp * 100);
  whole_temp = (a_temp) / 100; //ex: 2456/100=24
  decimal_temp = (a_temp) % 100; //ex: 2456%100 = 56
  /////////////////////////

  ////Usage Time Processing////
  if (runTime > 60) {
    myMin = runTime / 60; //Minutes
    mySec = runTime - (myMin * 60); //Seconds
  }

  else {
    myMin = 0; //Before 1 minute
    mySec = runTime;
  }

  ///////////OUTPUTS TO ANDROID APP///////////// **Every Thermal Status are outputted to the app by joining indexes [ex: (whole_value) + (decimal_value) = two indexes]**



  /////////////Ambient Temp///////////// Two indexes

  Serial.print((int)whole_AmbTemp); //prints "26.57"
  Serial.print("."); //"26." //#Index 1
  Serial.print("|");
  if ((int)decimal_AmbTemp < 10) { //If decimal value is less than 0.10
    Serial.print("0");
    Serial.print((int)decimal_AmbTemp); //ex output: "26.08 C"
  }
  else {
    Serial.print((int)decimal_AmbTemp);
  }
  Serial.print(" C");
  Serial.print("|"); //"57 C" //#Index 2 //ex output: "26.98 C"




  /////////////Skin Temp///////////// Two indexes

  Serial.print((int)whole_temp); //prints "26.57"
  Serial.print("."); //"26." //#Index 3
  Serial.print("|");
  if ((int)decimal_temp < 10) { //If decimal value is less than 0.10
    Serial.print("0");
    Serial.print((int)decimal_temp); //ex output: "26.08 C"
  }
  else {
    Serial.print((int)decimal_temp);
  }
  Serial.print(" C");
  Serial.print("|"); //"57 C" //#Index 4 //ex output: "26.98 C"



  /////////////WeaVE Emissivity///////////// One index

  Serial.print("0.");
  if ((int)decimal_emissivity < 10) { //0.0X
    Serial.print("0");
    Serial.print((int)decimal_emissivity); //#Index 5 //ex output: "0.08"
  }
  else {
    Serial.print((int)decimal_emissivity); //ex output: "0.68"
  }
  Serial.print("|");




  /////////////Heat Loss///////////// Two Index

  if (AmbSensorWithOffsetTemperature * 1.00 > raw_temp) {
    whole_heat_loss = 0;
    decimal_heat_loss = 0;
    Serial.print((int)whole_heat_loss);
  }
  else {
    Serial.print((int)whole_heat_loss);
  }
  Serial.print("."); //"26." //#Index 6
  Serial.print("|");
  if ((int)decimal_heat_loss < 10) { //0.0X
    Serial.print("0");
    Serial.print((int)decimal_heat_loss); //ex output: "30.07 W/m^2"
  }
  else {
    Serial.print((int)decimal_heat_loss); //ex output: "30.37 W/m^2"
  }
  Serial.print(" W/m^2");
  Serial.print("|"); //"57 C" //#Index 7





  /////////////Voltage///////////// Two indexes (since I assigned whole_voltage already, but this would've been one index)

  if (voltage <= 0) { //NEGATIVE voltage
    Serial.print("-"); //neg sign
    Serial.print((int)whole_voltage); //Index #8
    Serial.print(".");
    Serial.print("|");
    if ((int)decimal_voltage < 10) { //0.0X
      Serial.print("0");
      Serial.print((int)decimal_voltage); //Index #8 //ex output: "-0.03 V"
    }
    else {
      Serial.print((int)decimal_voltage); //ex output: "-0.53 V"
    }
    Serial.print(" V");
    Serial.print("|");
  }

  else { //POSITIVE voltage
    Serial.print((int)whole_voltage); //Index #8
    Serial.print(".");
    Serial.print("|");
    if ((int)decimal_voltage < 10) { //0.0X
      Serial.print("0");
      Serial.print((int)decimal_voltage);
    }
    else {
      Serial.print((int)decimal_voltage); //ex output: "0.53 V"
    }
    Serial.print(" V");
    Serial.print("|"); //Index #9 //ex output: "0.03 V"
  }





  /////////////Humidity (%)/////////////

  Serial.print((int)whole_AmbHumidity); //prints "34.14%"
  Serial.print("."); //"34." //#Index 10
  Serial.print("|");
  if ((int)decimal_AmbTemp < 10) { //If decimal value is less than 0.10
    Serial.print("0");
    Serial.print((int)decimal_AmbHumidity); //ex output: "34.14 %"
  }
  else {
    Serial.print((int)decimal_AmbHumidity);
  }
  Serial.print(" %");
  Serial.print("|"); //#Index 11 //ex output: "34.14 %"





  /////////////Usage Time/////////////

  Serial.print((int)myMin);
  Serial.print(" m ");
  Serial.print("|"); //"0 m " index #12
  Serial.print((int)mySec);
  Serial.print(" s");
  Serial.print("|"); //"0 s" index #13   //ex output: "2 min 34 s %"




  ///////////VOLTAGE OUTPUT/////////////////////

  //Recall that: //double tar_voltage = (slope_volt * raw_temp) + b;
  //double voltage  = ceil(tar_voltage * 100) / 100;

  //Upper Limit +0.6V         (Wire A on, Wire B off) --> Positive Voltage
  if (tar_voltage >= 0.6) {
    analogWrite(output_A, 0.62 * 51);
    analogWrite(output_B, 0.62 * 51);
    analogWrite(output_C, 0.62 * 51);
    analogWrite(output_GND1, 0 * 51);
    analogWrite(output_GND2, 0 * 51);
  }

  //Lower Limit -0.6V         (Wire A off, Wire B on) --> Negative Voltage
  else if (tar_voltage <= -0.6) {
    analogWrite(output_A, 0 * 51);
    analogWrite(output_B, 0 * 51);
    analogWrite(output_C, 0 * 51);
    analogWrite(output_GND1, 0.6 * 51);
    analogWrite(output_GND2, 0.6 * 51);
  }

  //Positive Voltage 0V to 0.6V
  else if (tar_voltage >= 0)  {
    analogWrite(output_A, (tar_voltage) * 51);
    analogWrite(output_B, (tar_voltage) * 51);
    analogWrite(output_C, (tar_voltage) * 51);
    analogWrite(output_GND1, 0 * 51);
    analogWrite(output_GND2, 0 * 51);
  }

  //Negative Voltage 0V to -0.6V
  else {
    analogWrite(output_A, 0 * 51);
    analogWrite(output_B, 0 * 51);
    analogWrite(output_C, 0 * 51);
    analogWrite(output_GND1, (-1 * tar_voltage + 0.02) * 51); //Brief calibriation
    analogWrite(output_GND2, (-1 * tar_voltage + 0.02) * 51);
  } 

  delay(3000);

  /*}*/
}
