#define SERIAL_SHOW_COMMANDS 1

void setupInSerial(){
  SCmd.addCommand("mode",setMode);       // Turns LED on
  SCmd.addCommand("config",setConfig);       // Turns LED on
  SCmd.addCommand("save",saveConfig);       // Turns LED on
  SCmd.addCommand("reset",resetFunc);       // Turns LED on
  SCmd.addCommand("color",setColor);       // Turns LED on
  SCmd.addCommand("delay",setDelay);       // Turns LED on

  
  //SCmd.addCommand("OFF",LED_off);        // Turns LED off
  //SCmd.addCommand("HELLO",SayHello);     // Echos the string argument back
  //SCmd.addCommand("P",process_command);  // Converts two arguments to integers and echos them back //
  SCmd.setDefaultHandler(unrecognizedCommand);  // Handler for command that isn't matched  (says "What?") 

  
  #ifdef SERIAL_SHOW_COMMANDS
  Serial.println("");
  Serial.println("DEBUG COMMANDS");
  Serial.println("config [datapin] [clockpin] [nr_of_leds]");
  Serial.println("reset");
  Serial.println("mode [mode]");
  Serial.println("color [r] [g] [b]");
  Serial.println("deley [ms]");
  #endif

}

void loopInSerial() {
  SCmd.readSerial();
}

void setConfig(){

}

void saveConfig(){

}

// set mode, if arg is NULL use next 
void setMode() {
  char *arg; 
  int aMode;
  
  arg = SCmd.next();
  if (arg != NULL) 
  {
    aMode=atoi(arg);    // convert char arg to int
    if (aMode>=0 && aMode <MAX_NR_OF_MODES){
      mode = aMode;
      modeSave = mode; 
    }
    else {
      mode = 0;
    }
  } 
  else {
    mode = 0; 
    if (modeSave<MAX_NR_OF_MODES-1) {
      //incase we are fading (mode 200) we need to use the save value
      mode = modeSave+1;
    } 
    modeSave = mode;  
  }
  
  initMode();   
}

void setColor(){
  char *arg;
  int aR, aG, aB;
  
  arg = SCmd.next();
  if (arg != NULL) 
  {
    aR=atoi(arg);    // convert char arg to int
    if (aR >= 0 && aR <= 255) {
      oscR = aR;
    }
  }
  
  arg = SCmd.next();
  if (arg != NULL) 
  {
    aG=atoi(arg);    // convert char arg to int
    if (aG >= 0 && aG <= 255) {
      oscG = aG;
    }
  }
  
  arg = SCmd.next();
  if (arg != NULL) 
  {
    aB=atoi(arg);    // convert char arg to int
    if (aB >= 0 && aB <= 255) {
      oscB = aB;
    }
  }

  Serial.print("color: ");
  Serial.print(oscR, DEC);
  Serial.print(" ");
  Serial.print(oscG, DEC);
  Serial.print(" ");
  Serial.print(oscB, DEC);
  Serial.println("");

}
void setDelay(){

}

void unrecognizedCommand(const char *command) {
  Serial.print("?");
  Serial.println(command);
}
