/*  This code gets the Gripper and Playstation controller working w/Joysticks.
 *  Joysticks working, but mapping needs to be fixed up.
 *  Date: 10/27/22
 *  Author:Abigail Doyle
*/

//All the libraries used for the robot for work
#include <Bump_Switch.h>
#include <Encoder.h>
#include <GP2Y0A21_Sensor.h>
#include <QTRSensors.h>
#include <Romi_Motor_Power.h>
#include <RSLK_Pins.h>   //Robot pin library
#include <SimpleRSLK.h>  //Robot library
#include "PS2X_lib.h"    //Playstation controller library
#include <Servo.h>       //Gripper library

//PS2 controller bluetooth dongle pins
#define PS2_DAT         14 //P1.7 = brown wire
#define PS2_CMD         15 //P1.6 = orange wire
#define PS2_SEL         34 //P2.3 = yellow wire (attention)
#define PS2_CLK         35 //P6.7 = blue wire
#define PS2X_DEBUG
#define PS2X_COM_DEBUG

#define pressures   false   //Not using pressure sensors on controller
#define rumble      false   //Not using rumble on controller

//All the cases used in switch(STATE)
#define IDLE 1
#define AUTO 2
//Cases for Gripper
#define OPEN 3
#define CLOSE 4

#define MS 1000     //Variable for changing the milliseconds to seconds
//Speed variables
uint16_t normalSpeed = 20;  //Variable that sets normal speed to 20
uint16_t fastSpeed = 28;    //Variable that sets fast speed to 28
uint16_t turnSpeed = 40;    //Variable that sets turn speed to 40
//Line following variables
uint16_t sensorVal[LS_NUM_SENSORS];
uint16_t sensorCalVal[LS_NUM_SENSORS];
uint16_t sensorMaxVal[LS_NUM_SENSORS];
uint16_t sensorMinVal[LS_NUM_SENSORS];
uint8_t lineMode = 0;
uint8_t lineColor;
uint32_t linePos;

int xVal, yVal;       //Variables for x-values and y-values
int STATE = IDLE;     //Starts the robot in the IDLE case
int sharppin = 23;    //
int stopDistance = 5; //
int rightsensval;     //
int linePosition = 0; //
int photores = A7;    //
int led = 7;          //LED pin = 7
int light = 0;        //Light value = 0 

bool isCalibrationComplete = false;

PS2X ps2x;        //PS2 Controller Class and object
Servo gripper;    //Declares Gripper as the servo object

void setup() {
  Serial.begin(57600); Serial1.begin(57600);  //Sets the baud rate to read and print to the serial monitor.
  gripper.attach(SRV_0);    //Initializes the gripper servo
  gripper.write(5);         //Sets the grippers position to 5 degrees when program is first run
  pinMode(sharppin, INPUT);
  setupLed(RED_LED);
  clearMinMax(sensorMinVal, sensorMaxVal);
  pinMode(led, OUTPUT);
  pinMode(photores, INPUT);
  setupRSLK();    //Sets up the DC motor pins (aka the wheel pins)
  ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);   //Sets up pins and settings for the PS Controller 
  delayMicroseconds(1000 * MS);   //Delay for a full second
}

void loop() {
  senseLight();     //Always checks the light values using the senseLight function.
  schmoove();       //Always runs the schmoove function making the robot move.
  if (isCalibrationComplete == false) {
    floorCalibration();
    isCalibrationComplete = true;
  STATE = IDLE;
  } 
  switch (STATE) {
    case IDLE:
      idleState();
      break;
    case AUTO:
      autonomousState();
      break;
    case CLOSE:
      closeState();
      break;
    case OPEN:
      openState();
      break;
    default:
      break;
  }
}
//This function constantly reads the joystick values and makes the robot move
void schmoove(){
  ps2x.read_gamepad(); //Reads controller
  delayMicroseconds(50 * MS);
  xVal = ps2x.Analog(PSS_RX);           //Assigns the variable xVal, to the right X joystick values
  xVal = map(xVal, 128, 255, 0, 20);    //Changes the xVal from 0-255 to 0-40
  yVal = ps2x.Analog(PSS_LY);           //Assigns the variable yVal, to the left Y joystick values
  yVal = map(yVal, 127, 255, 0, 40);    //Changes the yVal from 0-255 to 0-40
  if (yVal < 0){           //If the yVal is less than 0, go forward
    setMotorDirection(BOTH_MOTORS,MOTOR_DIR_FORWARD);   //Sets both motors to forward
    enableMotor(BOTH_MOTORS);   //Turns on both motors
    setMotorSpeed(BOTH_MOTORS,yVal * -1);                    //Sets motor speed to yVal.
    Serial.println("Going Forward"); Serial1.println("Going Forward");
    Serial.print("Y-Values: ");    Serial.println(yVal);  Serial1.print("Y-Values: ");    Serial1.println(yVal);
  } else if(yVal > 0){     //If the yVal is less than 0, go backward
    setMotorDirection(BOTH_MOTORS,MOTOR_DIR_BACKWARD);  //Sets both motors to backward
    enableMotor(BOTH_MOTORS);   //Turns on both motors
    setMotorSpeed(BOTH_MOTORS,yVal);                    //Sets motor speed to yVal
    Serial.println("Backing Up"); Serial1.println("Backing Up");
    Serial.print("Y-Values: ");    Serial.println(yVal);  Serial1.print("Y-Values: ");    Serial1.println(yVal);
  } else if (xVal > 0){    //If the xVal is more than 0, turn right
    enableMotor(BOTH_MOTORS);   //Turns on both motors
    setMotorDirection(LEFT_MOTOR,MOTOR_DIR_FORWARD);    //Sets the left motor to forward
    setMotorDirection(RIGHT_MOTOR,MOTOR_DIR_BACKWARD);  //Sets the right motor to backward
    setMotorSpeed(BOTH_MOTORS,xVal);                    //Sets both motors speed to xVal
    Serial.println("Turning Right"); Serial1.println("Turning Right");
    Serial.print("X-Values: ");Serial.println(xVal);Serial1.print("X-Values: ");Serial1.println(xVal);
  } else if(xVal < 0){     //If the xVal is less than 0, turn left
    enableMotor(BOTH_MOTORS);   //Turns on both motors
    setMotorDirection(LEFT_MOTOR,MOTOR_DIR_BACKWARD);   //Sets the left motor to backward
    setMotorDirection(RIGHT_MOTOR,MOTOR_DIR_FORWARD);   //Sets the right motor to forward
    setMotorSpeed(BOTH_MOTORS,xVal * -1);                    //Sets both motors speed to xVal
    Serial.println("Turning Left"); Serial1.println("Turning Left");
    Serial.print("X-Values: ");Serial.println(xVal);Serial1.print("X-Values: ");Serial1.println(xVal);
  } else {
    disableMotor(BOTH_MOTORS);    //Disables both of the motors if both xVal = 0
  }
}
//Follow the white line autonomously
void autonomousState(){
  Serial.println("Now in autonomous.");Serial1.println("Now in autonomous.");
  lineColor = LIGHT_LINE;
  readLineSensor(sensorVal);
  readCalLineSensor(sensorVal,sensorCalVal,sensorMinVal,sensorMaxVal,lineColor);
  linePos = getLinePosition(sensorCalVal,lineColor);
  Serial.print(sensorCalVal[6]);  Serial1.print(sensorCalVal[6]);
  Serial.println(sensorCalVal[7]);Serial1.println(sensorCalVal[7]);
  rightsensval = sensorCalVal[6];  
  if(linePos > 0 && linePos < 3000) {
    setMotorSpeed(LEFT_MOTOR,normalSpeed);
    setMotorSpeed(RIGHT_MOTOR,fastSpeed);
  } else if(linePos > 3500) {
    setMotorSpeed(LEFT_MOTOR,fastSpeed);
    setMotorSpeed(RIGHT_MOTOR,normalSpeed);
  } else if(rightsensval > 500){
    disableMotor(BOTH_MOTORS);
    setMotorSpeed(LEFT_MOTOR, turnSpeed);
    setMotorDirection(RIGHT_MOTOR, MOTOR_DIR_BACKWARD);
    setMotorSpeed(RIGHT_MOTOR, turnSpeed);
    enableMotor(BOTH_MOTORS);
    delayMicroseconds(500000);
    setMotorDirection(BOTH_MOTORS, MOTOR_DIR_FORWARD);
    setMotorSpeed(BOTH_MOTORS, normalSpeed);
  } else {
    setMotorSpeed(LEFT_MOTOR,normalSpeed);
    setMotorSpeed(RIGHT_MOTOR,normalSpeed);
  }
  if(ps2x.ButtonPressed(PSB_START)) {
    STATE = IDLE;
  }
}
/*
//Combine with the autonomousState function
void autonomousDist() {
  uint16_t normalSpeed = 18;
  uint16_t fastSpeed = 30;
  int distance = analogRead(sharppin);
  Serial.println(distance);   Serial1.println(distance);
  if (distance >= 600) {
    disableMotor(BOTH_MOTORS);
    delayMicroseconds(1000 * MS);
    setMotorDirection(BOTH_MOTORS, MOTOR_DIR_BACKWARD);
    enableMotor(BOTH_MOTORS);
    setMotorSpeed(BOTH_MOTORS, fastSpeed);
    delayMicroseconds(500 * MS);
    setMotorDirection(RIGHT_MOTOR, MOTOR_DIR_FORWARD);
    setMotorDirection(LEFT_MOTOR, MOTOR_DIR_BACKWARD);
    enableMotor(BOTH_MOTORS);
    setMotorSpeed(BOTH_MOTORS, fastSpeed);
    delayMicroseconds(1000 * MS);
    disableMotor(BOTH_MOTORS);
    gripper.write(150);
  } else if (distance < 600){
    enableMotor(BOTH_MOTORS);
    setMotorDirection(BOTH_MOTORS, MOTOR_DIR_FORWARD);
    setMotorSpeed(BOTH_MOTORS, fastSpeed);
  } else {
    setMotorSpeed(LEFT_MOTOR,normalSpeed);
    setMotorSpeed(RIGHT_MOTOR,normalSpeed);
  }
}
*/
void simpleCalibrate() {
  setMotorDirection(BOTH_MOTORS, MOTOR_DIR_FORWARD); //Set both motors direction forward
  enableMotor(BOTH_MOTORS);         //Enable both motors
  setMotorSpeed(BOTH_MOTORS, 20);   //Sets both motors speed to 20
  for (int x = 0; x < 100; x++) {
    readLineSensor(sensorVal);
    setSensorMinMax(sensorVal, sensorMinVal, sensorMaxVal);
    readCalLineSensor(sensorVal, sensorCalVal, sensorMinVal, sensorMaxVal, 0);
    linePosition = getLinePosition(sensorCalVal, lineMode);
  }
  disableMotor(BOTH_MOTORS);  //Disables both motors
}

void floorCalibration() {
  delay(2000);
  String btnMsg = "Push left button on Launchpad to begin calibration.\n";
  btnMsg += "Make sure the robot is on the floor away from the line.\n";
  delay(1000);
  Serial.println("Running calibration on floor");   Serial1.println("Running calibration on floor");
  simpleCalibrate();      //Runs the calibration function
  Serial.println("Reading floor values complete");    Serial1.println("Reading floor values complete");
  btnMsg = "Push left button on Launchpad to begin line following.\n";
  btnMsg += "Make sure the robot is on the line.\n";
  delay(1000);
  enableMotor(BOTH_MOTORS);
}

//Senses the light value
void senseLight(){
  light = analogRead(photores);
  if (light > 0) {
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(led, LOW);
  }
}
//Closes the gripper
void closeState() {
  gripper.write(40);              //Closes gripper to 30 degrees
  Serial.println("Close Claw");Serial1.println("Close Claw");   //Prints indicator to serial monitor
  delayMicroseconds(10 * MS);
  STATE = IDLE;                   //Returns the state to IDLE
}
//Opens the gripper
void openState() {
  gripper.write(150);           //Opens gripper to 160 degrees
  Serial.println("Open Claw");Serial1.println("Open Claw");  //Prints indicator to serial monitor
  delayMicroseconds(10 * MS);
  STATE = IDLE;                 //Returns the state to IDLE
}
//Reads what button is pressed then goes into the respective state.
void idleState() {
  if(ps2x.ButtonPressed(PSB_START)) {
    STATE = AUTO;
  } else if (ps2x.ButtonPressed(PSB_SQUARE))  {        //If square button is pressed...
    STATE = OPEN;     //Changes state to OPEN
  } else if(ps2x.ButtonPressed(PSB_TRIANGLE)){  //If triangle button is pressed... 
    STATE = CLOSE;    //Changes state to CLOSE
  } else {
    STATE = IDLE;     //Restarts the IdleState function until a button is pressed/read by robot
  }
}
