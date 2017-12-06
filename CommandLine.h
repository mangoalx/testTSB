
#include <Wire.h>

/*****************************************************************************

  How to Use CommandLine:
    Create a sketch.  Look below for a sample setup and main loop code and copy and paste it in into the new sketch.

   Create a new tab.  (Use the drop down menu (little triangle) on the far right of the Arduino Editor.
   Name the tab CommandLine.h
   Paste this file into it.

  Test:
     Download the sketch you just created to your Arduino as usual and open the Serial Window.  Typey these commands followed by return:
      add 5, 10
      subtract 10, 5

    Look at the add and subtract commands included and then write your own!


*****************************************************************************
  Here's what's going on under the covers
*****************************************************************************
  Simple and Clear Command Line Interpreter

     This file will allow you to type commands into the Serial Window like,
        add 23,599
        blink 5
        playSong Yesterday

     to your sketch running on the Arduino and execute them.

     Implementation note:  This will use C strings as opposed to String Objects based on the assumption that if you need a commandLine interpreter,
     you are probably short on space too and the String object tends to be space inefficient.

   1)  Simple Protocol
         Commands are words and numbers either space or comma spearated
         The first word is the command, each additional word is an argument
         "\n" terminates each command

   2)  Using the C library routine strtok:
       A command is a word separated by spaces or commas.  A word separated by certain characters (like space or comma) is called a token.
       To get tokens one by one, I use the C lib routing strtok (part of C stdlib.h see below how to include it).
           It's part of C language library <string.h> which you can look up online.  Basically you:
              1) pass it a string (and the delimeters you use, i.e. space and comman) and it will return the first token from the string
              2) on subsequent calls, pass it NULL (instead of the string ptr) and it will continue where it left off with the initial string.
        I've written a couple of basic helper routines:
            readNumber: uses strtok and atoi (atoi: ascii to int, again part of C stdlib.h) to return an integer.
              Note that atoi returns an int and if you are using 1 byte ints like uint8_t you'll have to get the lowByte().
            readWord: returns a ptr to a text word

   4)  DoMyCommand: A list of if-then-elses for each command.  You could make this a case statement if all commands were a single char.
      Using a word is more readable.
          For the purposes of this example we have:
              Add
              Subtract
              nullCommand
*/

//Name this tab: CommandLine.h

#include <string.h>
#include <stdlib.h>

#include "Debug.h"

//this following macro is good for debugging, e.g.  print2("myVar= ", myVar);
#define print2(x,y) (Serial.print(x), Serial.println(y))


#define CR '\r'
#define LF '\n'
#define BS '\b'
#define NULLCHAR '\0'
#define SPACE ' '

#define COMMAND_BUFFER_LENGTH        25                        //length of serial buffer for incoming commands
char   CommandLine[COMMAND_BUFFER_LENGTH + 1];                 //Read commands into this buffer from Serial.  +1 in length for a termination char

const char *delimiters            = ", \n";                    //commands can be separated by return, space or comma

/*************************************************************************************************************
     your Command Names Here
*/
const char *scanCommandToken       = "scan";                     //Modify here
const char *subtractCommandToken  = "sub";                     //Modify here
const char *ledonCommandToken     = "ledon";                   //Modify here
const char *ledoffCommandToken    = "ledoff";                  //Modify here
const char *ledCommandToken    = "led";                 
const char *testAddrCommandToken       = "test";
const char *read9808tCommandToken     = "readT";
const char *readAlertCommandToken     = "readA";
const char *setRegCommandToken     = "setreg";
const char *readRegCommandToken     = "readreg";

const char *repeatCommandToken     = "repeat";              //continue reading (repeat a command)
const char *stopCommandToken     = "stop";                  //stop repeat
const char *delayCommandToken     = "delay";                  //Set repeat command interval to nnnn mS 


/*************************************************************************************************************
    getCommandLineFromSerialPort()
      Return the string of the next command. Commands are delimited by return"
      Handle BackSpace character
      Make all chars lowercase
*************************************************************************************************************/

bool
getCommandLineFromSerialPort(char * commandLine)
{
  static uint8_t charsRead = 0;                      //note: COMAND_BUFFER_LENGTH must be less than 255 chars long
  //read asynchronously until full command input
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case CR:      //likely have full command in buffer now, commands are terminated by CR and/or LS
      case LF:
        commandLine[charsRead] = NULLCHAR;       //null terminate our command char array
        if (charsRead > 0)  {
          charsRead = 0;                           //charsRead is static, so have to reset
          Serial.println(commandLine);
          return true;
        }
        break;
      case BS:                                    // handle backspace in input: put a space in last char
        if (charsRead > 0) {                        //and adjust commandLine and charsRead
          commandLine[--charsRead] = NULLCHAR;
          Serial << byte(BS) << byte(SPACE) << byte(BS);  //no idea how this works, found it on the Internet
        }
        break;
      default:
        // c = tolower(c);
        if (charsRead < COMMAND_BUFFER_LENGTH) {
          commandLine[charsRead++] = c;
        }
        commandLine[charsRead] = NULLCHAR;     //just in case
        break;
    }
  }
  return false;
}


/* ****************************
   readNumber: return a 16bit (for Arduino Uno) signed integer from the command line
   readWord: get a text word from the command line

*/
int
readNumber () {
  char * numTextPtr = strtok(NULL, delimiters);         //K&R string.h  pg. 250
  return atoi(numTextPtr);                              //K&R string.h  pg. 251
}
int
readHex () {
  char * numTextPtr = strtok(NULL, delimiters);         //K&R string.h  pg. 250
  return (int)strtol(numTextPtr,NULL,16);                              //K&R string.h  pg. 251
}

char * readWord() {
  char * word = strtok(NULL, delimiters);               //K&R string.h  pg. 250
  return word;
}

void
nullCommand(char * ptrToCommandName) {
  print2("Command not found: ", ptrToCommandName);      //see above for macro print2
}

/****************************************************
   Add your commands here
*/
// This is function is used to verify that on the I2C bus there is only 1 device and the address is "addr"
int scanCommand() {                                      //Modify here
  byte error, address;
  int nDevices;
 
  Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
  return nDevices;
}

int subtractCommand() {                                //Modify here
  int firstOperand = readNumber();
  int secondOperand = readNumber();
  return firstOperand - secondOperand;
}

void ledonCommand() {
  LED_ON();
}
void ledoffCommand() {
  LED_OFF();
}
void ledCommand() {
  int firstOperand = readNumber();
  if (firstOperand == 0)
      LED_OFF();
  else LED_ON();
}
void testAddrCommand(){
  Serial.print("I2C address pin testing ...");
  if(addressTest())
    Serial.println("Passed");
  else
    Serial.println("Failed");
}
void read9808tCommand(){
  eCommandCode=eRead9808T;
  execute=true;                         
}
readAlertCommand(){
  eCommandCode=eReadAlert;
  execute=true;                         
}
void stopCommand(){                     //stop repeating command
  repeat=false;
}
void repeatCommand(){                   //repeat current reading command
  repeat=true;
}
void delayCommand(){                    //Set the interval (mS) of repeating command

  delay100mS = readNumber()*1000/LOOPDELAY;
}
void setRegCommand(){
  uint8_t reg = readNumber();
  uint16_t data = readNumber();
  tempsensor.write16(reg,data);
  
}
void readRegCommand(){
  uint8_t reg = readNumber();
   Serial.println(tempsensor.read16(reg));
  
}
/****************************************************
   DoMyCommand
*/
bool
DoMyCommand(char * commandLine) {
  //  print2("\nCommand: ", commandLine);
  int result;

  char * ptrToCommandName = strtok(commandLine, delimiters);
  //  print2("commandName= ", ptrToCommandName);

  if (strcmp(ptrToCommandName, scanCommandToken) == 0) {                   //Modify here
      scanCommand();
  } 
  else if (strcmp(ptrToCommandName, subtractCommandToken) == 0) {           //Modify here
      result = subtractCommand();                                       //K&R string.h  pg. 251
      print2(">    The difference is = ", result);

  } 
  else if (strcmp(ptrToCommandName, ledonCommandToken) == 0) {           //Modify here
      ledonCommand();
  } 
  else if (strcmp(ptrToCommandName, ledoffCommandToken) == 0) {           //Modify here
      ledoffCommand();
  } 
  else if (strcmp(ptrToCommandName, ledCommandToken) == 0) {           //Modify here
      ledCommand();
  } 
  else if (strcmp(ptrToCommandName, testAddrCommandToken) == 0) {           //Modify here
      testAddrCommand();
  } 
  else if (strcmp(ptrToCommandName, read9808tCommandToken) == 0) {           //Modify here
      read9808tCommand();
  } 
  else if (strcmp(ptrToCommandName, readAlertCommandToken) == 0) {           //Modify here
      readAlertCommand();
  } 
  else if (strcmp(ptrToCommandName, setRegCommandToken) == 0) {           //Modify here
      setRegCommand();
  } 
  else if (strcmp(ptrToCommandName, readRegCommandToken) == 0) {           //Modify here
      readRegCommand();
  } 
  
  else if (strcmp(ptrToCommandName, stopCommandToken) == 0) {           //Modify here
      stopCommand();
  } 
  else if (strcmp(ptrToCommandName, repeatCommandToken) == 0) {           //Modify here
      repeatCommand();
  } 
  else if (strcmp(ptrToCommandName, delayCommandToken) == 0) {           //Modify here
      delayCommand();
  } 
  else {
      nullCommand(ptrToCommandName);
  }
  
}
