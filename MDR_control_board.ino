#include <Adafruit_NeoPixel.h>
#include <RH_ASK.h>
#include <SoftwareSerial.h>

// establishes pins for BT
SoftwareSerial mySerial(2, 3); // RX, TX

// wirless transmitter defaults to PIN12
RH_ASK driver; 

//LED controls (Cassius)
#define LED_PIN 6
int num = 88;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(num, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Setup serial connection to computer
  Serial.begin(9600);
  
  // Setup wireless transmitter
  if (!driver.init())
      Serial.println("driver init failed");
  
  // Setup bluetooth chip
    // Setup serial connection to BT chip
  mySerial.begin(9600);
  // AT+ == send command
  // set name to bluino
  
  sendBTCommand("AT+ROLE0");
  sendBTCommand("AT+NAMEpiano");

  
  //initialize all LEDS to "off"
  strip.begin();
  strip.show();
}

//LED controls:
  //Set individual LEDs 
  //strip.setPixelColor(number, red, green, blue)
void off(){ //run this to turn all LEDs off
  for(int j = 0; j < num; j++){
    strip.setPixelColor(j, 0, 0, 0);
  }//end for
}//end function

// Sends the command and listens for the response
void sendBTCommand(const char * command){
  Serial.print("Command send :");
  Serial.println(command);

  mySerial.println(command);

  delay(100);

  char reply[100];
  int i = 0;
  while (mySerial.available()) {
    reply[i] = mySerial.read();
    i += 1;
  }

  //end the string
  reply[i] = '\0';
  Serial.print(reply);
  Serial.println("Reply end");
}

// Read any data sent to the BT chip
// Returns null if there is no data
void readBT(char* data){
  int i = 0;

  while (mySerial.available()) {
    data[i] = mySerial.read();
    i += 1;
  }

  //end the string
  data[i] = '\0';
  if(strlen(data) > 0){
    Serial.println(data);
    Serial.println("We have just read some data");
  } else {
    // No data read.
    data = NULL;
  }
}

// Write data to BT chip
void writeToBT(char *value) {
  Serial.print("Writing:");
  Serial.println(value);
  mySerial.write(value, strlen(value));
}

// Send data over wireless chip to gloves
void writeToGloves(char* msg) {
  Serial.print("sending: ");
  Serial.println(msg);
  
  driver.send((uint8_t *)msg, strlen(msg));
  driver.waitPacketSent();
  Serial.println("Sent.");
}

void loop() {
  // Take information from bluetooth and transmit it to the gloves
  char readStr[50];
  int readInt;
  
  readBT(readStr);
  
  if (strlen(readStr) > 0) {
    Serial.println(strlen(readStr));
    writeToGloves(readStr);
    
    sscanf(readStr, "%d", &readInt);
    strip.setPixelColor(readInt, 255, 255, 255);
    strip.show();
  }
  
  delay(10);
}
