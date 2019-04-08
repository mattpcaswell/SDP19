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
#define MAX_DATA_LENGTH 256 // big enough to turn every key off or on at the same time. Not enough to do that + every finger

#define LED_PIN 6
#define BT_RX_PIN 2
#define BT_TX_PIN 3
#define GLOVE_TX_PIN 12

#define WHITE_LED_COLOR_R 25
#define WHITE_LED_COLOR_G 25
#define WHITE_LED_COLOR_B 25

#define BLACK_LED_COLOR_R 0
#define BLACK_LED_COLOR_G 0
#define BLACK_LED_COLOR_B 50

const short keyMapping[88] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 70, 72, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 92, 94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172};
const char  blackKeyMapping[12] = {0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1};

// establishes pins for BT
SoftwareSerial btSerial(BT_RX_PIN, BT_TX_PIN); // RX, TX

// wirless transmitter defaults to PIN12
RH_ASK driver;

//LED controls
Adafruit_NeoPixel strip = Adafruit_NeoPixel(keyMapping[NUM_KEYS - 1] + 2, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Setup serial connection to computer
  if (DEBUG)
    Serial.begin(9600);

  // Setup serial connection to BT chip
  btSerial.begin(9600);

  // Setup wireless transmitter
  if (DEBUG && !driver.init())
    Serial.println("driver init failed");

  //initialize all LEDS to "off"
  strip.begin();
  clearAllLEDs();
  strip.show();
}

void loop() {
  watchBT();
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
  
  // Initialize the buffer and cmd to empty
  buf[0] = '\0';
  cmd[0] = '\0';

  // 1. read until ; or \0
  while (true) {
    if (buf[bufi] == '\0') {
      // buffer is empty. Refill it
      bufi = 0;
      readBT(buf);

      // if no BT data then wait for there to be data
      while (buf[0] == '\0') {
        readBT(buf);
      }
    } else if (buf[bufi] == END_CHAR) {
      // end of command. Send it
      cmd[cmdi++] = buf[bufi++];
      cmd[cmdi] = '\0';
      parseCmd(cmd);
      cmdi = 0;
    } else {
      // Middle of buffer. Add it to cmd
      cmd[cmdi++] = buf[bufi++];
    }
  }
}

// Read any data sent to the BT chip
// Returns empty string if there is no data
void readBT(char* data) {
  int i = 0;

  while (btSerial.available()) {
    data[i++] = btSerial.read();
  }

  //end the string
  data[i] = '\0';
  if (DEBUG && strlen(data) > 0) {
    Serial.print("Read data from BT: ");
    Serial.println(data);
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
        if (DEBUG) {
          Serial.print("found finger ");
          Serial.println(finger_num);
        }
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

void setKey(int key_num, char off_or_on) {
  if (off_or_on == ON_CHAR) {
    // turn key_num on
    if (isKeyBlack(key_num)) {
        strip.setPixelColor(keyMapping[key_num], BLACK_LED_COLOR_R, BLACK_LED_COLOR_G, BLACK_LED_COLOR_B);
    } else {
        strip.setPixelColor(keyMapping[key_num], WHITE_LED_COLOR_R, WHITE_LED_COLOR_G, WHITE_LED_COLOR_B);
    }
  } else if (off_or_on == OFF_CHAR) {
    // turn key_num off
    strip.setPixelColor(keyMapping[key_num], 0, 0, 0);
  } else if (DEBUG){
    // invalid off_or_on value. Should be either ON_CHAR or OFF_CHAR
    Serial.println("INVALID OFF OR ON VALUE GIVEN TO setKey():");
    Serial.println(off_or_on);
  }
}

void clearAllLEDs() {
    for(int i = 0; i < keyMapping[NUM_KEYS - 1]; i++) {
        strip.setPixelColor(i, 0, 0, 0);
    }
}

void setAllKeys() {
  for(int i = 0; i < NUM_KEYS; i++) {
    setKey(i, ON_CHAR);
  }
}

char isKeyBlack(short keynum) {
    return blackKeyMapping[keynum % 12];
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
