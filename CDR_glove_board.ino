#define LED_PIN_OFFSET 9
#define RECEIVER_PIN 19
#define UNUSED_PIN1 18
#define UNUSED_PIN2 17

#define VIBRATE_TIME 1000 // how long to vibrate in ms

#include <RH_ASK.h>

RH_ASK driver(2000, RECEIVER_PIN, UNUSED_PIN1, UNUSED_PIN2, false);

unsigned long stop_time[5];

void setup()
{
  // Setup output pins
  for (int i = 0; i < 5; i++) {
    pinMode(i + LED_PIN_OFFSET, OUTPUT);
  }

  for (int i = 0; i < 5; i++) {
    digitalWrite(i + LED_PIN_OFFSET, LOW);
  }

  if (!driver.init()) {
    for (int i = 0; i < 5; i++) {
      digitalWrite(i + LED_PIN_OFFSET, HIGH);
    }
  }

  for (int i = 0; i < 5; i++) {
    stop_time[i] = 0;
  }
}

void loop()
{
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = RH_ASK_MAX_MESSAGE_LEN;
  uint8_t num;
  unsigned long curr_time;
  
  if (driver.recv(buf, &buflen)) {
    // Message with a good checksum received, dump it.
    buf[buflen] = '\0';

    for (int i = 0; i < buflen; i ++) {
      num = buf[i] - '0';
      num = num % 5;

      digitalWrite(num + LED_PIN_OFFSET, HIGH);
      stop_time[num] = millis() + VIBRATE_TIME;
    }
  }

  // turn off timed out motors
  curr_time = millis();

  for (int i = 0; i < 5; i ++) {
    if (curr_time > stop_time[i]) {
      digitalWrite(i + LED_PIN_OFFSET, LOW);
      stop_time[i] = 0;
    }
  }
}
