
// ESP8266 disk washer using i2C OLED for display. 

// single button to drive the menu.
// all 



// for i2c OLED 128 x 64

#include <Wire.h>
#include <Adafruit_GFX.h>  // Include core graphics library for the display
#include <Adafruit_SSD1306.h>  // Include Adafruit_SSD1306 library to drive the display

Adafruit_SSD1306 display;  // Create display

#include <Fonts/FreeMonoBold12pt7b.h>  // Add a custom font
#include <Fonts/FreeMono9pt7b.h>  // Add a custom font
#include <OneWire.h>
#include <DallasTemperature.h>

// for Node MCU 1.0 or WEMES D1 Mini 
 const int SDApin = 4; // D2
 const int SCLpin = 5; // D1

// for ESP8266 ESP-01S
// const int SDApin = 0; // D3
// const int SCLpin = 2; // D4

// for use in simulators driving LEDS instead of ctual appliance
// # define switchOn  HIGH
// # define switchOff LOW
// # define switchInit 0b00000111


// for driving RELAYS in actual appliance
# define switchOn  LOW
# define switchOff HIGH
# define switchInit 0b11111111

byte writeByteBuffered = switchInit;


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
  "Test",             // 6
  "H/W Test"          // 7
 
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

int washTemp;
int dryTemp;
float WaterTemp; 
int tempNow;
// bool alarm = true;



byte buttonValue;
byte i2cValue;

// define IO pins on I2C expander
# define buttonPin A0
# define TemperaturePin D0
# define lowWaterSensorPin D5
# define highWaterSensorPin D6
# define drainPump D3
# define inlet D4
# define washPump D7
# define heater D8

// define sensor input pins


# define marginTemp  10


OneWire oneWire(TemperaturePin);
DallasTemperature TempSensor(&oneWire);


// define machine modes
# define progInit   0
# define progSelect 1
# define standby    2
# define progStart  3
# define drain      4
# define fill       5
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
// if (lcdline[0] != prevline[0] or lcdline[1] != prevline[1] or lcdline[2] != prevline[2] or lcdline[3] != prevline[3])  { 

   display.clearDisplay();  // Clear the buffer
   display.setTextSize(2);


   for (int l=0; l < 4; l++) {
     display.setCursor(0, 16 * l);  // (x,y)
     display.print (lcdline[l]);
     prevline[l] = lcdline[l];   
   }
   display.display();
   lastDisplayTime = millis();  // reset timer for screen saver
   screenSaver = false;
 
  }
}

// Look for a button press
byte checkButtonPress(byte button) {
   byte Bp = 0;
   byte rBp = 0;

  

    if (button == 0)  {  Bp = menuRight;}  // 0 means button pressed (pulled low to ground)
  
   
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
       Serial.println (rBp);
    
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
        lcdline[1]="Set Prog.";
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
void washprogconfig (int inwaterLevel, int inNbrCycle, int inwashTime,  int inwashTemp, int indryTime, int indryTemp)
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


}
 
// the setup function runs once when you press reset or power the board
void setup() {
   Serial.begin (74880);  // initialize serial port for debuging info
   delay (1000);
   Serial.println ("Disk Washer");

   Wire.begin(SDApin,SCLpin);
   display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize display with the I2C address of 0x3C 
  display.clearDisplay();  // Clear the buffer
  display.setTextColor(WHITE);  // Set color of the text
  display.setRotation(0);  // Set orientation. Goes from 0, 1, 2 or 3
  display.setTextWrap(false);  // By default, long lines of text are set to automatically “wrap” back to the leftmost column.
                               // To override this behavior (so text will run off the right side of the display - useful for
                               // scrolling marquee effects), use setTextWrap(false). The normal wrapping behavior is restored
                               // with setTextWrap(true).
                               
  display.dim(0);  //Set brightness (0 is maximun and 1 is a little dim)
  display.setTextSize(1);
  lcdline[0] = "Disk Washer v1.0";
  
  writeToLCD();
  
  delay(3000);

  pinMode ( TemperaturePin, INPUT_PULLUP);
  pinMode ( lowWaterSensorPin, INPUT_PULLUP);
  pinMode ( highWaterSensorPin, INPUT_PULLUP);
  Serial.println("Setup complete.");

  TempSensor.begin();

  updateMenu();

}


// the loop function runs over and over again forever
void loop() {
  
buttonValue = analogRead (buttonPin);
// check waterLevel 
if (! digitalRead(highWaterSensorPin))  waterLevel = high;
else if (! digitalRead(lowWaterSensorPin))  waterLevel = low;
else waterLevel = empty;
 
pressedButton = checkButtonPress(buttonValue);
if (pressedButton != 0) updateMenu();

if (currProg==hwTest) { 
   return;
}

  // screen saver to avoid burnout of OLED.
  if (abs(lastDisplayTime - millis()) > ScreenSaverInterval) {
    if (! screenSaver) {
      display.clearDisplay();
      display.display();
      lastDisplayTime = millis();
      screenSaver = true;
      }
  }

  

// exit loop if program not yet started.
 if (currMode < progStart)  return;




// check current Temperature;
 TempSensor.requestTemperatures();// Send the command to get waterTemps
 WaterTemp=TempSensor.getTempCByIndex(0); //Stores eepromValue in Variable
 tempNow = int(WaterTemp);
 
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
    // add the time of the remaining modes in this cycle (fill, wash, drian)
    switch (currMode) {
      case fill : remainProgTime += (washTime + drainTime);
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
     else if (errMode == fill) {
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
    lcdline[2] = "W" + String(waterLevel) + " T" + String(tempNow) + "C";
//  lcdline[2] = "Temp " + String(tempNow) + " C";
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
         currMode = fill;
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

    
    case fill:
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
      if ( tempNow  >  (washTemp + marginTemp) ) digitalWrite (heater, switchOff);
      // turn on heater if below max. Temp
      else if ( tempNow  <  (washTemp - marginTemp) ) digitalWrite (heater, switchOn);
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
      if ( tempNow  >  (dryTemp + marginTemp) ) digitalWrite (heater, switchOff);
      // turn on heater if below max. Temp
      else if ( tempNow  <  (dryTemp - marginTemp) ) digitalWrite (heater, switchOn);
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
