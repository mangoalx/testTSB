#include <Adafruit_MCP9808.h>


#include <Wire.h>
//#include <TinyWireM.h>
//#include <USI_TWI_Master.h>

/*******************************************************************
 * Repeatable commands list
 * 
 *******************************************************************/
enum RepeatCommands {eNullCommand=0,eReadAlert,eTest,eRead9808T};
enum TestStatus {eWaitConnect=0,eTestAddr,eTestAlert,eTestTemp,ePassed,eFailed,eTestEnd};

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

#define MAX_TEMPERATURE 35
#define MIN_TEMPERATURE 15

#define DEBOUNCENUM 5  //debounce time for new TSB detection is 5x DELAYX100MS x 100mS

//Status control variables
#define DELAYX100MS   5       //delay how many 100mS before repeat an execution
int count=0;                                  //for debounce
unsigned int delayCounter=0;
int delay100mS = 5;                                  //wait time between repeated commands,in 100mS
enum RepeatCommands eCommandCode = eTest;      //commandCode decide which command to execute
bool repeat = true, execute = true;     //bool switches to control if execute / repeat a command
enum TestStatus eStatusCode = eWaitConnect;
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
    float temperature;
    bool received = getCommandLineFromSerialPort(CommandLine);      //global CommandLine is defined in CommandLine.h
//    int16_t Vbus,Vshunt;
    if (received) DoMyCommand(CommandLine);
    if(++delayCounter>=DELAYX100MS){
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
        case eTest:
          switch(eStatusCode){
            case eWaitConnect:
              if(verifyAddress(DEFAULT_ADDRESS)){
                if(++count>DEBOUNCENUM){
                  eStatusCode=eTestAddr;
                  LED_OFF();                            //If last test failed, we turn off LED here, when started next test
                  Serial.print("\n\n\n");
                  Serial.println("New device found, starting test ......");
                }
              }
              else count=0;
              break;
            case eTestAddr:
              Serial.print("I2C address pins testing ...");
              if(addressTest()){
                Serial.println("Passed");
                eStatusCode=eTestAlert;
              }
              else{
                Serial.println("Failed");
                eStatusCode=eFailed;
              }
              break;
            case eTestAlert:
              Serial.print("Alert pin testing ...");
              if(alertTest()){
                Serial.println("Passed");
                eStatusCode=eTestTemp;
              }
              else{
                Serial.println("Failed");
                eStatusCode=eFailed;
              }
              break;
            case eTestTemp:
              temperature=tempsensor.readTempC();
              
              Serial.print("Temperature reading :");
              Serial.print(temperature);

              if((temperature>MAX_TEMPERATURE) || (temperature<MIN_TEMPERATURE)){
                Serial.println("------Failed");
                Serial.println("Temperature is out of range");
                eStatusCode=eFailed;
              }
              else{
                Serial.println("------Passed");
                eStatusCode=ePassed;
              }
              break;
            case ePassed:
              if(!verifyAddress(DEFAULT_ADDRESS)){        //Switch to Wait for connect status if DUT is disconnected
                eStatusCode=eTestEnd;
                Serial.print("\r\n");
//                count=0;                                //initialize counter, for device connection detect debounce
              }
              else{
                Serial.print("Temperature reading :");      //Keep reading temperature so tester can observe the reading varies
                Serial.print(tempsensor.readTempC());
                Serial.print("\r");                         //stay the same line, so user read temperature the same place
              }
              break;
            case eFailed:
              LED_ON();           //Turn on led to indicate a failure
              if(!verifyAddress(DEFAULT_ADDRESS))
                eStatusCode=eTestEnd;
              break;
            case eTestEnd:
              eStatusCode=eWaitConnect;
              count=0;                                //initialize counter, for device connection detect debounce
              Serial.println("------Test ended. Waiting for next device--------");
              break;
            default:
              Serial.print("Unknown status,ask john to debug\r");
          }
//          Serial.print("Alert status: ");
//          Serial.println(readAlert());
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
bool alertTest(void){
  bool result=false;
  if(readAlert()==1){
    tempsensor.write16(MCP9808_REG_CONFIG,MCP9808_REG_CONFIG_ALERTCTRL);      //Set config register to enable alert
    result=(readAlert()==0);
    tempsensor.write16(MCP9808_REG_CONFIG,0);                                 //Reset config to power-on default
  }
  return result;
}

