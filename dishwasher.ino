          
/*
 disk washer v2.0
 2020 07 19 
 Author: Billy Cheung
 
 
 
 Function:  This is a  dishwasher controller program.
            This utilize a ESP8266 WeMos D1 mini 
            an SSD1306 I2C OLED for display,  
            a water temperature sensor  to control whether to turn on the heater to haat up the water to the required temperature for washing.
            a water level sensor to detect high and low waater level and whether all water have been drained.
            Uses only one single buitton to drive a menu.
            Short press to cycle through the options in each menu.
            Long press to select the option.
            
            The disk washer function includes the followin wash programs :
            Normal - normal wash cycle with hot water an detergene and dry.
            Long - adds more wash cycle to normal program
            Fast - shortened wash cycle
            Fruit - cold water spray only, no detergene.
            Spray - hot  water spray,
            Cold - wash using cold water, detergene.
            Wash Test - accelerated wash cycle to test the program (i.e. 1 minute becomes 1 second).
            I/O Test - display values of all sensors while turning  each output relay  on for 1 second and off for 1 second.

            You can long press to abort a pogram any time.
            The OLED will go to sleep mode if not pressed for a  long time.
            Short press again to wake it up.            

            Due to the limitation of number of pins on the ESP8266 and to avoid adding anohter I2C expandoer e.g. MCP 23017,
            there are no buzzer to signal that a wash program has finished.
            
            Future  options to web-based menu to this program as ESP8266 can support WIFI.
            Then the washer can be controlled by the web front end.

    
 Output:  The output is connected to a 5V 4 way relay modul to drive the 

          IN1 - drainPump GPIO16  // D0 - to drain exhuast water out of the washer. The program reads the low water level sensor and stop draining when water level falls below the low water level mark.
          IN2 - inlet GPIO0 // D3 - control the water inlet to let water in from the hose to fill water up to the required water level. The program checks the water sensor to close the inlet when the high water level mark is eached.
          IN3 - washPump GPIO2 // D4 - control the strong water pump (80 watt or higher) that drives a garden sprayer to produce a rotating all angel spray to wash darts off the surfaces of dishes.
          IN4 - heater GPIO15 // D8 - contorl the under water heater that heats up the water to the required temperature. The program checks the water temperature sensor to turn the heater on or off.
          SSD 1306 - (SCL/GPIO 5/D1,  SDA/GPIO 4/D2) - display messages and menu
          
 
 Input:   
          1. buttonPin - A0 - connect th eother end of the button to 3.3V 
          2. TemperaturePin 12 //D6 - connects to the water temperature sensor DS18B20 , one wire temperature sensor
          3. lowWaterSensorPin 13 // D7 - connects to a wire imersed into water to mark the low water level. Connect another wire from the bottom of the tank to Ground.
          4. highWaterSensorPin 14 // D5 - connects to a wire imersed into water to mark the low water level. Connect another wire from the bottom of the tank to Ground.

 
*/



// for i2c OLED 128 x 64

#include <Wire.h>
#include "SSD1306Wire.h" 
#include "OLEDDisplayUi.h"

SSD1306Wire  display(0x3c, 4, 5);  // OLED's I2c device address, sda pin, scl pin

OLEDDisplayUi ui ( &display );

#include <OneWire.h>
#include <DallasTemperature.h>


// for use in simulators driving LEDS instead of ctual appliance
// # define switchOn  HIGH
// # define switchOff LOW
// # define switchInit 0b00000111


// for driving RELAYS in actual appliance
# define switchOn  LOW
# define switchOff HIGH



unsigned long lastDisplayTime  = 0;            // the last time the OLED is written to
const unsigned long ScreenSaverInterval = 60000;      // the screen saver time 60 seconds
 
int menuNo = 0;
const int menusize [] = {8,5};



const String menu0[] = {
  "Normal",           // 0      
  "Long",             // 1
  "Fast",             // 2
  "Fruit",            // 3
  "Spray",            // 4
  "Cold",             // 5
  "Wash Test",        // 6
  "I/O Test"          // 7
 
  };      

const String menu1[]= {            
  "Start",            // 0
  "Return",           // 1
  "Pause",            // 2
  "Resume",           // 3
  "Abort",            // 4
};

const String progName[] = {
  "Normal", 
  "Long",
  "Fast",
  "Fruit",
  "Spray",
  "Cold",
  "Test",
  "HwTest"
};



#define menuRight  1
#define menuUp     2 
#define menuDown   3
#define menuLeft   4 




// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;          // the last time the output pin was toggled
unsigned long lastKeyHoldTime = 0;            // the last time the output pin was pressed and held

const unsigned long debounceInterval = 30;      // the debounce time; increase if the output flickers
const unsigned long KeyHoldInterval = 500;  // if button pressed longer than this time, then button stqte is longPressed
byte ButtonState = 0;
byte lastButtonState = 0;

byte pressedButton = 0;
byte currentPos = 0;
int error,  nDevices = 0;
bool screenSaver = false;
String lcdline[]={ "","","",""};
String prevline[]= { "","","",""};



// define wash programs
# define standProg  0 
# define longProg   1
# define fastProg   2
# define fruitProg  3 
# define sprayProg  4 
# define coldProg   5
# define testProg   6
# define hwTest     7
# define lastProg   7 // last program
# define menuStart  8
# define menuPause  9
# define menuResume 10
# define menuAbort  11




// waterLevel
# define high  2
# define low   1
# define empty 0
int waterLevel = low;



//wash cycles
int nbrCycle;
int currCycle; 

// time in minutes
#define timingfactor 10000  // divide the machine milli seconds by this number to count against the wash cycle times 
                            // default is 60000 so the wash program time is set in minute
                            // during testing set it to 1000 (1s) or 10000 (10 sec) to make the wash cycle runs faster.
#define fillTime 3     // est. time to fill the tank up to high water level
#define drainTime  3   // est. time to drain all water from the thank

int remainProgTime;
int washTime;
int dryTime;
int timeNow;
int startTime;
int endTime;

float waterTemp, washTemp, dryTemp;
// bool alarm = true;



byte buttonValue;
byte i2cValue;

// define IO pins on I2C expander

// input pin
# define buttonPin A0
# define TemperaturePin 12     // D6
# define lowWaterSensorPin 13  // D7
# define highWaterSensorPin 14 // D5

// output pin
# define drainPump 16  // D0 - connect to IN1 of Relay module
# define inlet 0       // D3 - connect to IN2 of Relay module
# define washPump 2    // D4 - connect to IN3 of Relay module
# define heater 15     // D8 - connect to IN4 of Relay module

// define sensor input pins


#define marginTemp  1

// DS18B20 - one wire temperature sensor
OneWire oneWire(TemperaturePin);
DallasTemperature TempSensor(&oneWire);


// define machine modes
# define progInit   0
# define progSelect 1
# define standby    2
# define progStart  3
# define drain      4
# define fillin     5
# define wash       6
# define dry        7
# define complete   8
# define errPause   9

const String modename[] = {
  "Init*",  
  "Select",  
  "Standby",
  "Start",  
  "Drain",  
  "Fill",   
  "Wash",
  "Dry",
  "Done"
  "Pause",
  "***"
 };

int myprogparam;
int prevProgparam;
# define maxmyprogparam 6

int prevProg  = -1;
int currProg = 0;
int prevMode = complete;
int currMode = standby;
int errMode  = standby;


int inbyte =0;
int inchoice = 0;


String binaryToString (int v, int num_places) {
  int mask = 0, n;

  String binText = "";
  
  for (n = 1; n <= num_places; n++) {
    mask = (mask << 1) | 0x0001;
  }
  v = v & mask;  // truncate v to specified number of places

  while (num_places) {

    if (v & (0x0001 << (num_places - 1))) {
      binText=binText+"1";
    }
    else {
      binText=binText+"0";
    }

    --num_places;
 //   if (((num_places % 4) == 0) && (num_places != 0)) {
 //     binText=binText+"_";
 //         }
  }
  return binText;
}


void  writeToLCD () {
if (lcdline != prevline) {

   display.clear();  // Clear the buffer

   for (int l=0; l < 4; l++) {
     display.drawString(0, 16 *l , lcdline[l]);
     prevline[l] = lcdline[l];   
     }

   display.display();
   lastDisplayTime = millis();  // reset timer for screen saver
   screenSaver = false; 
  }
}



// Look for a button press
byte checkButtonPress(byte buttonVal) {
   byte Bp = 0;
   byte rBp = 0;

  

    if (buttonVal == 0)  {  Bp = menuRight;}  // 0 means button pressed (pulled low to ground)
  
   
     if (abs((millis() - lastDebounceTime) > debounceInterval)) 
     {
       
       // The button has been pressed long enough, now test if it has changed 
       if (Bp != ButtonState ) {
  
          ButtonState = Bp; 
          rBp = lastButtonState;
           //  check if button has been released
          if (ButtonState == 0)  { 
            
            if  (abs((millis() - lastKeyHoldTime) > KeyHoldInterval))  {  // check if its a long pressed 
              rBp = lastButtonState;  // return original key if its long pressed, i.e. right key as right, left key as left.           
  
            }
            else {
              switch (lastButtonState) { // if it's short pressed , convert right key to down,  left key to up
                case menuRight: rBp = menuDown; break;
                case menuLeft: rBp = menuUp; break;
              }
            }
          }
  
       }
     }
    if (Bp != lastButtonState) 
    {  
      lastButtonState = Bp; 
      lastDebounceTime = millis();  // reset debounce timer
      lastKeyHoldTime = millis();   // reset key hold  timer
    }

    return rBp;

 }


  
void updateMenu () {
    Serial.println("updateMenu");

    if (currProg == hwTest ) currProg = standProg; // a button pressed halt any hw test in progress.
   
    if (currMode >= progStart) { // when wash program already starated, only allow pauses, not other selection.
      switch (pressedButton) {   
        case menuRight:  // 
        case menuLeft:   // for Right and Left , change to Pause mode
          errMode = currMode;
          currMode = errPause;
          break;  
        case menuUp:
        case menuDown:  // for  up and down, just refresh screens, but do not change current mode.
          prevline[0] = ""; // Trigger a writeToLCD at the end of this funciton to wake up from screen saver
          break;
        }       
      }
    else    { // still in program select mode, or standby mode.
     // update the index of the current position of the menu based on button pressed
      switch (pressedButton) {   
        
         case menuRight:   // Right 
  
            //  Right = Confirming an option  or drilling down to a submenu.
            switch (menuNo) {
              case 0:  //menu 0 - a wash program has been selected
                   currProg = currentPos;
                   currMode = standby;
                   // configure the wash program parameters according to the selected one
                   switch (currProg) {
    
                      // set up  parameters for selecgted wash program:  
                      // water level,  no. of wash cycles, wash time per cycle, wash Temperature , dry time, dry Temperature
                      case standProg:washprogconfig (high,  3, 15, 70, 15, 70); break;
                      case longProg: washprogconfig (high,  4, 20, 70, 20, 70); break;
                      case fastProg: washprogconfig (high,  2, 15, 70, 10, 70); break;
                      case fruitProg:washprogconfig (high,  1, 5,   0,  0,  0); break;
                      case sprayProg:washprogconfig (high,  1, 10, 70, 10, 70); break;
                      case coldProg: washprogconfig (high,  2, 15,  0, 10,  0); break;
                      case testProg: washprogconfig (high,  1,  1,  1,  1,  1); break;
                    }   
                  menuNo = 1; // go down to menu 1
                  currentPos = 0;       
                break;       
              case 1:  // menu 1 - select whether to start the program or other options
                switch (currentPos) {
                case 0: //Start
                    currMode = progStart;
                break;
                case 1: //Return
                     currMode = progSelect;
                     menuNo = 0;
                     currentPos = 0;                    
                break;
                case 2: //Pause
                    errMode = currMode; // store the mode before pausing
                    currMode = standby;
                break;
                case 3: //Resume 
                   currMode = errMode;        
                break;
                case 4: //Abort 
                    currMode = progSelect; 
                    menuNo = 0;
                    currentPos = 0;
                break;
              break;
              }          
            }   
       
        break; // right
   
        case menuUp: 
          currentPos = (currentPos  + menusize[menuNo] - 1) % menusize[menuNo]; //Wrap around followed by modulation rather than deduction to avoid getting negative values.
  
           break;
        
        case menuDown:
  
  
          currentPos = (currentPos  +  1) % menusize[menuNo]; // use modulation to wrap the scrolled menu position around.
  
        break;
        
        case menuLeft:
        if (currMode >= progStart) {  // when wash program already starated, only allow pauses, not other selection.
          errMode = currMode;
          currMode = errPause;  
          }
        else {
          currMode = progSelect;
          menuNo = 0;
          }
        break;

      }
     
      Serial.print ("Button ");
      Serial.print (pressedButton);
      Serial.print (" currentPos ");
      Serial.println (currentPos);
             
  
    switch (menuNo) { 
      case 0 :         
        lcdline[0]="Disk Washer";
        lcdline[1]="Select Program";
        lcdline[2]=menu0[currentPos];
        lcdline[3]="";
      break;
      case 1 :
        lcdline[0]="Disk Washer";
        lcdline[1]=progName[currProg];
        lcdline[2]=menu1[currentPos];
        lcdline[3]="";
      break;
      }
   }

  writeToLCD();
    
    
}

// configure parameters for the wash programs
void washprogconfig (int inwaterLevel, int inNbrCycle, int inwashTime,  float inwashTemp, int indryTime, float indryTemp)
{ 
waterLevel = inwaterLevel;
washTime = inwashTime;
nbrCycle = inNbrCycle + 1 ;  // first cycle is only to drain o/s water, hence need to add 1 cycle.
dryTime = indryTime;
washTemp = inwashTemp;
dryTemp = indryTemp;

}






void switchmode ( int induration, String newmsg)
{ 
  String outmsg = " mode started";

    prevMode = currMode;
    startTime = timeNow;
    endTime = startTime + induration;  
    Serial.println ("--------------------");
    Serial.print (timeNow);
    outmsg = ": " + newmsg + outmsg;
    Serial.println (outmsg);
//   switch off  all output
     digitalWrite (drainPump,  switchOff);
     digitalWrite (washPump,  switchOff);
     digitalWrite (inlet,  switchOff);
     digitalWrite (heater,  switchOff);
}

bool inputTest () {
    buttonValue = analogRead (buttonPin); 
    TempSensor.requestTemperatures();// Send the command to get waterTemps
    waterTemp=TempSensor.getTempCByIndex(0); //Stores eepromValue in Variable 
    lcdline[1] = "L:" +  String (digitalRead(lowWaterSensorPin)) + " H:" + String (digitalRead(highWaterSensorPin)) + " T:" + String(waterTemp) + " " + String(buttonValue);
    if (buttonValue == 0)  { 
      Serial.print ("Test ended"); 
      return true; 
      } 
    else return false;
    }


 
bool selfTest () {


 
 // test the output
 #define testms 1000
    
    lcdline[0] = "HW Test"; 
    if (inputTest()) return true;
    lcdline[2] = "Drian Pump";
    lcdline[3] = " ON";
    digitalWrite (drainPump,  switchOn);
    for (int i=0; i<4; i++) Serial.println(lcdline[i]);  
    writeToLCD();    
    delay(testms);

    if (inputTest()) return true;
    lcdline[3] = " OFF";
    digitalWrite (drainPump,  switchOff);
    for (int i=0; i<4; i++) Serial.println(lcdline[i]);  
    writeToLCD();    
    delay(testms);
    
    if (inputTest()) return true;
    lcdline[2] = "Water In";
    lcdline[3] = " ON";
    digitalWrite (inlet,  switchOn);
    for (int i=0; i<4; i++) Serial.println(lcdline[i]);  
    writeToLCD();    
    delay(testms);

    if (inputTest()) return true;
    lcdline[3] = " OFF";
    digitalWrite (inlet,  switchOff);
    for (int i=0; i<4; i++) Serial.println(lcdline[i]);  
    writeToLCD();    
    delay(testms);

    if (inputTest()) return true;
    lcdline[2] = "Wash Pump";
    lcdline[3] = " ON";
    digitalWrite (washPump,  switchOn);
    for (int i=0; i<4; i++) Serial.println(lcdline[i]);  
    writeToLCD();    
    delay(testms);
    
    if (inputTest()) return true;
    lcdline[3] = " OFF";
    digitalWrite (washPump,  switchOff);
    for (int i=0; i<4; i++) Serial.println(lcdline[i]);  
    writeToLCD();    
    delay(testms);

    if (inputTest()) return true;
    lcdline[2] = "Heater";
    lcdline[3] = " ON";
    digitalWrite (heater,  switchOn);
    for (int i=0; i<4; i++) Serial.println(lcdline[i]);  
    writeToLCD();    
    delay(testms);
 
    if (inputTest()) return true;
    lcdline[3] = " OFF";
    digitalWrite (heater,  switchOff);
    for (int i=0; i<4; i++) Serial.println(lcdline[i]);  
    writeToLCD();    
    delay(testms);

    return false;
}

// the setup function runs once when you press reset or power the board
void setup() {
   Serial.begin (74880);  // initialize serial port for debuging info
   delay (1000);
   Serial.println ("Disk Washer");

    display.init();
  
    display.clear();
    display.flipScreenVertically();
 
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    display.clear();

    display.setFont(ArialMT_Plain_16);


  lcdline[0] = "Disk Washer";
  lcdline[1] = "";
  lcdline[2] = "Billy Cheung";
  lcdline[3] = "2020/07/19";
  
  writeToLCD();
  
  delay(3000);

  pinMode ( TemperaturePin, INPUT_PULLUP);
  pinMode ( lowWaterSensorPin, INPUT_PULLUP);
  pinMode ( highWaterSensorPin, INPUT_PULLUP);
  Serial.println("Setup complete.");
  pinMode (drainPump,OUTPUT);
  pinMode (washPump,OUTPUT);
  pinMode (inlet,OUTPUT);
  pinMode (heater,OUTPUT);
  digitalWrite (drainPump, switchOff);
  digitalWrite (washPump, switchOff);
  digitalWrite (inlet, switchOff);
  digitalWrite (heater, switchOff);

  TempSensor.begin();

  updateMenu();

}


// the loop function runs over and over again forever
void loop() {
  

// check waterLevel 
if (! digitalRead(highWaterSensorPin))  waterLevel = high;
else if (! digitalRead(lowWaterSensorPin))  waterLevel = low;
else waterLevel = empty;

buttonValue = analogRead (buttonPin); 
pressedButton = checkButtonPress(buttonValue);

if (pressedButton != 0) updateMenu();

if (currProg==hwTest) { 
   if (selfTest()) {
//   button pressed to stop i/o test

 // go back to main menu
     switchmode (0, "Program Completed");  
     pressedButton = 0;
     currentPos = 1;
     updateMenu();
     currMode = progSelect;
     menuNo = 0;
     prevProg =1;
   }
   return;
}


  // screen saver to avoid burnout of OLED.
  if (abs(lastDisplayTime - millis()) > ScreenSaverInterval) {
    if (! screenSaver) {
      display.clear();
      display.display();
      lastDisplayTime = millis();
      screenSaver = true;
      }
  }

  

// exit loop if program not yet started.
 if (currMode < progStart)  return;




// check current Temperature;
 TempSensor.requestTemperatures();// Send the command to get waterTemps
 waterTemp=TempSensor.getTempCByIndex(0); //Stores eepromValue in Variable

 
// record the time
  timeNow = millis() / timingfactor;

 
  // calculate remaining progtime
  // = dry time + no. of full cycles remaining * (fillTime + washTime + drainTime)
  // + remaining  time from current cycles
  // Drytime
  remainProgTime = 0;

  if (currCycle > nbrCycle)
      // all wash cycle is complete, we are now drying, calculate remaining dryTime;
      remainProgTime = (endTime - timeNow);
  else  {
    // calculate the remaining time of the current cycle
    remainProgTime += (endTime - timeNow); 
    // add the time of the remaining modes in this cycle (fillin, wash, drian)
    switch (currMode) {
      case fillin : remainProgTime += (washTime + drainTime);
      break;
      case wash : remainProgTime += drainTime;
      break;
      case drain: //nothing to add
      break;    
      }
      // add the time of the remaining full cycles and the Dry Time
     remainProgTime += (nbrCycle - currCycle) * ( fillTime + washTime + drainTime) + dryTime;    
  
  }

 
  if (currMode == complete) {
    lcdline[0] = progName[currProg]; 
    lcdline[1] = "Completed";
    lcdline[2] = "";
    lcdline[3] = "";   
  } 
  else if (currMode == errPause) {
     lcdline[0] = progName[currProg] + " " + String (remainProgTime);
     lcdline[1] =  "C"  + String (nbrCycle - currCycle);
     if (errMode == drain) {
        lcdline[2] ="Drian Err";
        lcdline[3]= "or Clog"; 
     }
     else if (errMode == fillin) {
        lcdline[2] ="Fill Err";
        lcdline[3]= "Check Hose"; 
     }
     else {
        lcdline[2] ="Press to";
        lcdline[3]= "to resume";  
     }  
 
  } 
  else {
    lcdline[0] = progName[currProg] + " " + String (remainProgTime);
    lcdline[1] =  "C"  + String (nbrCycle - currCycle) + " " + modename[currMode];
    lcdline[2] = "W" + String(waterLevel) + " T" + String(waterTemp) + "C";
//  lcdline[2] = "Temp " + String(waterTemp) + " C";
    lcdline[3] = "Time "  + String (endTime - timeNow) + " m";
    }
  writeToLCD ();



 
 switch (currMode)
  { 
   
    // User has pressed the button to start a new wash program.
     case progStart:

       
    if (currMode != prevMode) {
      switchmode (0, "Program Start");
    }
    currMode = drain;
    currCycle = 0;   // cycle 0 only have drain, then jump to cycle 1 to start a regular cycle of fill, wash and drain.
     
    break;
     
    case drain:
    if (currMode != prevMode) {
      switchmode (drainTime, "Drain");    
      // Turn on relays to start the drian pump;
      delay (2000);  // delay 2 seconds to let the drain door open;
      digitalWrite (drainPump, switchOn); 
    }
    else if (waterLevel == empty) { // drain completed 
      currCycle ++;
      if (currCycle <= nbrCycle) 
         currMode = fillin;
      else 
      if (dryTime > 0)
        currMode = dry;
      else
        currMode = complete;
      }
    else if (timeNow >= endTime) {         
      Serial.println (" Water drian time exceeded. Water drain or sensor problem, pls check");
      errMode = currMode;
      currMode =errPause;  
    }  
    break;

    
    case fillin:
    if (currMode != prevMode) {
      switchmode (fillTime, "fill");    
      digitalWrite (inlet, switchOn);  // open the inlet to let water in from the hose.
     }
    else if (waterLevel == high)  
      currMode = wash;
    else if (timeNow >= endTime) {
       Serial.println (" Water fill time exceeded. Water fill problem, pls check");
       errMode = currMode;
       currMode = errPause;  // Error 
//     currMode = wash;
     }  
  
    break;
     
    case wash:
    if (currMode != prevMode)
     {
      switchmode (washTime, "Wash");    
      digitalWrite (washPump, switchOn);  

     }
    else if (timeNow >= endTime) 
     {
      currMode = drain;   
     }


    if (washTemp > 0)
    { // turn off heater if above max. Temp.
      if ( waterTemp  >  (washTemp + marginTemp) ) digitalWrite (heater, switchOff);
      // turn on heater if below max. Temp
      else if ( waterTemp  <  (washTemp - marginTemp) ) digitalWrite (heater, switchOn);
    }


    break;
    
   case dry:
    if (currMode != prevMode)
    {
      switchmode (dryTime, "Dry");     
      digitalWrite (heater, switchOn);
    }
    else if (timeNow >= endTime) 
        currMode = complete;
    if (dryTemp > 0)
    { // turn off heater if above max. Temp.
      if ( waterTemp  >  (dryTemp + marginTemp) ) digitalWrite (heater, switchOff);
      // turn on heater if below max. Temp
      else if ( waterTemp  <  (dryTemp - marginTemp) ) digitalWrite (heater, switchOn);
    }
    break;
   
   case complete:
     switchmode (0, "Program Completed");  
     pressedButton = 0;
     currentPos = 1;
     updateMenu();
     currMode = progSelect;
     menuNo = 0;
     prevProg =1;
     break;
     
   case errPause:
     switchmode (0, "Program Paused");  
     pressedButton = 0;
     currentPos = 3;
     updateMenu();               
     currMode = standby;
     menuNo = 1;
     prevProg =1;
     break;

  }  


  delay (100);  
 
}
