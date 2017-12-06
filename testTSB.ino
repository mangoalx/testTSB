#include <Adafruit_MCP9808.h>


#include <Wire.h>
//#include <TinyWireM.h>
//#include <USI_TWI_Master.h>

/*******************************************************************
 * Repeatable commands list
 * 
 *******************************************************************/
enum RepeatCommands {eNullCommand=0,eReadAlert,eRead1080TH,eRead1WT,eRead9808T};
#define LOOPDELAY 100             //100mS delay for loop routine

//TSB to feather connection
#define DEFAULT_ADDRESS 0x18      //MCP9808 default address (A2, A1, A0 pulled down)
#define ADDRESS_A0 0x19
#define ADDRESS_A1 0x1A
#define ADDRESS_A2 0x1C

#define ALERT 5
#define A0    6
#define A1    9
#define A2    10

//Status control variables
unsigned int delayCounter=0;
int delay100mS = 20;                                  //wait time between repeated commands,in 100mS
enum RepeatCommands eCommandCode = eNullCommand;      //commandCode decide which command to execute
bool repeat = false, execute = false;     //bool switches to control if execute / repeat a command

Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

//prototype of functions
bool addressTest(void);


  #include "CommandLine.h"
 // #include "Debug.h"

  void setup() {
    Serial.begin(115200);
    SETUP_LED();
  
//    Wire.begin(); no need, ina219 will call wire.begin
    while (!Serial);             // Leonardo: wait for serial monitor
    //Serial.println("\nI2C Scanner");


    if (!tempsensor.begin(DEFAULT_ADDRESS)) //0x18
      Serial.println("MCP9808 is not connected!");

//  I/O pin initialization
    pinMode(ALERT, INPUT_PULLUP);        //For Alert from mcp9808, it is open drain, so need to be pull-up
    
    pinMode(A2, INPUT);                   //For address control A2,A1,A0, pull-down on the TSB, so keep them high-impedance
    pinMode(A1, INPUT);
    pinMode(A0, INPUT);

  }

  void loop() {
    bool received = getCommandLineFromSerialPort(CommandLine);      //global CommandLine is defined in CommandLine.h
//    int16_t Vbus,Vshunt;
    if (received) DoMyCommand(CommandLine);
    if(++delayCounter>=delay100mS){
      delayCounter=0;
      if(repeat) execute=true;
    }
    if(execute){
      switch(eCommandCode){
         case eRead9808T:
          Serial.print("MCP9808 temp: ");
          Serial.println(tempsensor.readTempC());
          break;
         case eReadAlert:
          Serial.print("Alert status: ");
          Serial.println(readAlert());
          break;
        case eNullCommand:                                        //do nothing for null or unknown command
        default:
          break;
      }
      execute=false;
    }
  
    delay(LOOPDELAY);

  }

/**************************************************************
 * Function modules
 * ***********************************************************/
bool verifyAddress(byte addr) {                                      //Modify here
  byte error, address;
  int nDevices = 0;
  
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      if(address!=addr)
        return false; 
      ++nDevices;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)      //No device found
    return false;
  else
    return true;
}

// 
bool addressTest(){
  return (verifyAddress(DEFAULT_ADDRESS) && addrPinTest(A0,ADDRESS_A0) \
  && addrPinTest(A1,ADDRESS_A1) && addrPinTest(A2,ADDRESS_A2));
}
bool addrPinTest(byte pinNo,byte addr){
  bool result;
  pinMode(pinNo,OUTPUT);
  digitalWrite(pinNo,HIGH);
  result=verifyAddress(addr);
  digitalWrite(pinNo,LOW);
  pinMode(pinNo,INPUT);
  return result;
}
int readAlert(void){
  return digitalRead(ALERT);
}

