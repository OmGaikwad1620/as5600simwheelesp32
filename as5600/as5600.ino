#include <Arduino.h>
#include <BleGamepad.h>
#include <Wire.h>

#define AS5600_ADDR 0x36
#define RAW_ANGLE_REG 0x0C  // raw angle (12-bit)

long turnCount = 0;
float lastAngle = 0;

BleGamepad bleGamepad("FFBDrivingController", "expressif32", 100);

uint16_t readRawAngle() {
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(RAW_ANGLE_REG);
  Wire.endTransmission(false);
  Wire.requestFrom(AS5600_ADDR, 2);

  uint16_t angle = 0;
  if (Wire.available() == 2) {
    angle = (Wire.read() << 8) | Wire.read();
  }
  return angle & 0x0FFF;  // 12-bit mask

}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // SDA, SCL (ESP32 default)
  Serial.println("AS5600 Multi-Turn ±720° → 16-bit range");
  
 pinMode(LED_BUILTIN,OUTPUT);

 BleGamepadConfiguration bleGamepadConfig;
 bleGamepadConfig.setAutoReport(false);
 bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD); 
 bleGamepadConfig.setButtonCount(10);
 bleGamepadConfig.setWhichAxes(false, false, false, false, false, false, false, false);      // Can also be done per-axis individually. All are true by default
 bleGamepadConfig.setWhichSimulationControls(false, false, true, true, true); // Can also be done per-control individually. All are false by default
 bleGamepadConfig.setHatSwitchCount(0);                                   
 bleGamepadConfig.setSimulationMin(0x8001); // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
 bleGamepadConfig.setSimulationMax(0x7FFF); //32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal 
 bleGamepad.begin(&bleGamepadConfig);

}

void loop() {
  uint16_t raw = readRawAngle();
  float angle = raw * 360.0 / 4096.0;  // 0–360°

  // Detect wrap-around
  if ((lastAngle > 300) && (angle < 60)) {
    turnCount++;  // one full CW turn
  }
  else if ((lastAngle < 60) && (angle > 300)) {
    turnCount--;  // one full CCW turn
  }

  lastAngle = angle;

  // Extended angle (in degrees, can be negative)
  float extendedAngle = angle + (turnCount * 360.0);

  // Clamp to ±720°
  if (extendedAngle > 720) extendedAngle = 720;
  if (extendedAngle < -720) extendedAngle = -720;

  // Map to signed 16-bit range (–32767..+32767)
int16_t output = (int16_t)(- (extendedAngle / 720.0) * 32767);

  Serial.printf("Raw: %u, Angle: %.2f°, Extended: %.2f°, Output: %d\n",
                raw, angle, extendedAngle, output);
                
if(bleGamepad.isConnected()){
  digitalWrite(LED_BUILTIN,HIGH);
  bleGamepad.setSteering(output);
  bleGamepad.setAccelerator(map(analogRead(33),0,4096,-32767,32767));
  bleGamepad.setBrake(map(analogRead(32),0,4096,-32767,32767));
  bleGamepad.sendReport();
 }
else{
  digitalWrite(LED_BUILTIN,LOW); 
 }
}
