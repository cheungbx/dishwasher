# dishwasher
/*
 dishwasher v2.0
 2020 07 19 
 Author: Billy Cheung
 
 
[![Picture](https://github.com/cheungbx/diskwasher/blob/master/dishwasher.jpg)](https://youtu.be/HiL8nKlVM2g)

 
 
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

          drainPump GPIO16  // D0 - to drain exhuast water out of the washer. The program reads the low water level sensor and stop draining when water level falls below the low water level mark.
          washPump GPIO2 // D4 - control the strong water pump (80 watt or higher) that drives a garden sprayer to produce a rotating all angel spray to wash darts off the surfaces of dishes.
          inlet GPIO0 // D3 - control the water inlet to let water in from the hose to fill water up to the required water level. The program checks the water sensor to close the inlet when the high water level mark is eached.
          heater GPIO15 // D8 - contorl the under water heater that heats up the water to the required temperature. The program checks the water temperature sensor to turn the heater on or off.
          SSD 1306 - (SCL/GPIO 5/D1,  SDA/GPIO 4/D2) - display messages and menu
          
 Input:   Following are the input pins
 
          buttonPin - A0 - connect th eother end of the button to 3.3V 
          TemperaturePin 12 //D6 - connects to the water temperature sensor
          lowWaterSensorPin 13 // D7 - connects to a wire imersed into water to mark the low water level. Connect another wire from the bottom of the tank to Ground.
          highWaterSensorPin 14 // D5 - connects to a wire imersed into water to mark the low water level. Connect another wire from the bottom of the tank to Ground.

 
*/

