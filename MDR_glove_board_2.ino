#include <Adafruit_NeoPixel.h>
#include <RH_ASK.h>
#include <SoftwareSerial.h>

#define DEBUG true

#define ON_CHAR  'y'
#define OFF_CHAR  'n'
#define FINGER_CHAR  'f'
#define SEPARATOR_CHAR  ',' // Cannot change!
#define END_CHAR  ';'

#define NUM_KEYS 88
#define MAX_DATA_LENGTH 256 // big enough to turn every key off or on and more.

#define LED_PIN 6
#define BT_RX_PIN 2
#define BT_TX_PIN 3
#define GLOVE_TX_PIN 12

#define LED_COLOR_R 0
#define LED_COLOR_G 255
#define LED_COLOR_B 0

// establishes pins for BT
SoftwareSerial btSerial(BT_RX_PIN, BT_TX_PIN); // RX, TX

// wirless transmitter defaults to PIN12
RH_ASK driver; 

//LED controls
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_KEYS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Setup serial connection to computer
  Serial.begin(9600);
  
  // Setup wireless transmitter
  if (!driver.init())
      Serial.println("driver init failed");
  
  // Setup serial connection to BT chip
  btSerial.begin(9600);
  
  sendBTCommand("AT+RESET");
  delay(100);
  sendBTCommand("AT+ROLE0");
  delay(100);
  sendBTCommand("AT+NAMEpiano");
  delay(100);
  
  //initialize all LEDS to "off"
  strip.begin();
  strip.show();
}

// Sends the command and listens for the response
void sendBTCommand(const char * command){
  Serial.print("Command send :");
  Serial.println(command);

  btSerial.println(command);

  delay(100);

  char reply[100];
  int i = 0;
  while (btSerial.available()) {
    reply[i] = btSerial.read();
    i += 1;
  }

  //end the string
  reply[i] = '\0';
  Serial.print(reply);
  Serial.println("Reply end");
}


// Write data to BT chip
void writeToBT(char *value) {
  Serial.print("Writing to BT:");
  Serial.println(value);
  btSerial.write(value, strlen(value));
}

// Send data over wireless chip to gloves
void writeToGloves(short fingers[10]) {
  char msg[20] = "";

  for (int i = 0; i < 10; i++) {
    if (fingers[i]) {
      char finger[2];
      snprintf(finger, 2, "%d", i);
      strcat(msg, finger);
    }
  }

  if (DEBUG) {
    Serial.print("sending to gloves: ");
    Serial.println(msg);
  }
  
  driver.send((uint8_t *)msg, strlen(msg));
  driver.waitPacketSent();
  
  if (DEBUG) {
    Serial.println("Sent.");
  }
}

void loop() {
  watchBT();
}

void setKey(int key_num, char off_or_on) {
  if (off_or_on == ON_CHAR) {
    // turn key_num on
    if (1) {
      Serial.println("turning key on:");
      Serial.println(key_num);
    }
    
    strip.setPixelColor(key_num, LED_COLOR_R, LED_COLOR_G, LED_COLOR_B);
  } else if (off_or_on == OFF_CHAR) {
    // turn key_num off
    if (1) {
      Serial.println("turning key off:");
      Serial.println(key_num);
    }
    
    strip.setPixelColor(key_num, 0,0,0);
  } else {
    // invalid off_or_on value. Should be either ON_CHAR or OFF_CHAR
    Serial.println("INVALID OFF OR ON VALUE GIVEN TO setKey():");
    Serial.println(off_or_on);
  }
}

// Read any data sent to the BT chip
// Returns empty string if there is no data
void readBT(char* data){
  int i = 0;

  while (btSerial.available()) {
    data[i] = btSerial.read();
    i += 1;
  }

  //end the string
  data[i] = '\0';
  if(strlen(data) > 0){
    //Serial.println(data);
    //Serial.println("We have just read some data");
  } else {
    // No data read.
    data[0] = '\0';
  }
}


// take in any number of strings and build them into strings seperated by ;
// Format:
//   This turns on keys 1, 2, and 3 while vibrating fingers 1, 3, and 5. The trailing , before ; is necessary
//    y1,2,3,;f1,3,5,;
//   This turns off keys 1, 2, 6, and 74. Ordering of keys doesnt matter
//    n6,2,74,1;    
//   This turns off keys 3,5 and turns on key 2 while vibrating fingers 0 and 9.
//    f9,0,;n3,5,;y2,;    
void watchBT() {
  char cmd[MAX_DATA_LENGTH]; // Current command
  int cmdi = 0;
  char buf[MAX_DATA_LENGTH]; // Buffer of all data currently read in from BT
  int bufi = 0;

  // 1. read until ; or \0
  
  while (true) {
    if (buf[bufi] == '\0') {
      // buffer is empty. Refill it
      bufi = 0;
      readBT(buf);

      // if no BT data then wait for there to be data
      while (buf[0] == '\0') {
        delay(10);
        readBT(buf);
      }
    } else if (buf[bufi] == END_CHAR) {
      // end of command. Send it
      cmd[cmdi++] = buf[bufi++];
      cmd[cmdi] = '\0';
      parseCmd(cmd);
      cmdi = 0;
    } else {
      // Middle of command. add it to cmd
      cmd[cmdi++] = buf[bufi++];
    }
  }
}

void parseCmd(char *cmd) {
  int i = 1;
  if (DEBUG) {
    Serial.println("Parsing Cmd:");
    Serial.println(cmd);
  }
  
  if (cmd[0] == FINGER_CHAR) {
    // Process line of finger data
    // 1. Check for END_CHAR
    // 2. Read a number followed by a SEPERATOR_CHAR
    // 3. Repeat
    short fingers[10];
    for (int j = 0; j < 10; j++) {
      fingers[j] = 0;
    }

    while (cmd[i] != END_CHAR) {
      int finger_num;
      short found_number = 0;

      found_number = sscanf(&(cmd[i]), "%d,", &finger_num);
      i += 2; // Go past the number and the following ,

      // send finger number to gloves
      if (found_number) {
        Serial.print("found finger ");
        Serial.println(finger_num);
        fingers[finger_num] = 1;
      }
    }

    writeToGloves(fingers);
  } else {
    // Process line of note data
    // 1. Check for END_CHAR
    // 2. Read a number followed by a SEPERATOR_CHAR
    // 3. Repeat
    while (cmd[i] != END_CHAR) {
      int key_num;
      short found_number = 0;

      found_number = sscanf(&(cmd[i]), "%d,", &key_num);

      if (found_number) {
        if (key_num >= 10) {
          i += 3;
        } else {
          i += 2;
        }

        // turn key_num off/on
        setKey(key_num, cmd[0]);
      } else {
        i++;
      }
    }

    // Send the new values to the LED strip
    strip.show();
  }
}
