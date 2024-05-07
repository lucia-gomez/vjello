#include <MIDIUSB.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

// touch sensor
Adafruit_MPR121 cap = Adafruit_MPR121();
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

bool isStarted = false;

int leftLastTimeTouched = 0;
int topLastTimeTouched = 0;
int rightLastTimeTouched = 0;
const int ringDelay = 1000;

int leftColors[][3] = { { 100, 0, 74 }, { 0, 0, 100 }, { 100, 13, 0 } };
int topColors[][3] = { { 100, 7, 0 }, { 0, 22, 100 }, { 100, 41, 0 } };
int rightColors[][3] = { { 0, 100, 0 }, { 24, 0, 100 }, { 100, 0, 13 } };

int colorIdx = 0;
int numColors = sizeof(leftColors) / sizeof(leftColors[0]);

int chordIdx = 0;
int chords[][3] = { { 65, 68, 72 }, { 50, 53, 57 }, { 60, 62, 64 } };

void setup() {
  Serial.begin(9600);

  while (!Serial) {
    delay(10);
  }
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1)
      ;
  }
  Serial.println("MPR121 found!");
  reset();
}

void changeColors() {
  Serial.println("changing colors");
  // left RGB
  sendCCMessage(51, leftColors[colorIdx][0]);
  sendCCMessage(52, leftColors[colorIdx][1]);
  sendCCMessage(53, leftColors[colorIdx][2]);

  // top RGB
  sendCCMessage(61, topColors[colorIdx][0]);
  sendCCMessage(62, topColors[colorIdx][1]);
  sendCCMessage(63, topColors[colorIdx][2]);

  // right RGB
  sendCCMessage(71, rightColors[colorIdx][0]);
  sendCCMessage(72, rightColors[colorIdx][1]);
  sendCCMessage(73, rightColors[colorIdx][2]);

  colorIdx = (colorIdx + 1) % numColors;
}

void reset() {
  sendCCMessage(16, 1);  // start pulse visible
  sendCCMessage(17, 0);  // start ring hidden
  sendCCMessage(18, 1);  // start ring playback reset
  sendCCMessage(19, 1);  // start ring pause

  sendCCMessage(20, 0);  // left circle hidden
  sendCCMessage(22, 1);  // left paused
  sendCCMessage(21, 1);  // left reset playback
  sendCCMessage(23, 0);  // left touch ring hidden

  sendCCMessage(30, 0);  // top circle hidden
  sendCCMessage(32, 1);  // top paused
  sendCCMessage(31, 1);  // top reset playback
  sendCCMessage(33, 0);  // top touch ring hidden

  sendCCMessage(40, 0);  // right circle hidden
  sendCCMessage(41, 1);  // right reset playback
  sendCCMessage(42, 1);  // right paused
  sendCCMessage(43, 0);  // right touch ring hidden

  sendCCMessage(1, 1);  // expand animation paused
  sendCCMessage(2, 1);  // expand animation reset
  sendCCMessage(4, 1);  // expand animations visible

  changeColors();
}

void loop() {
  int currTime = millis();
  currtouched = cap.touched();

  if (leftLastTimeTouched != 0 && currTime - leftLastTimeTouched >= ringDelay) {
    sendCCMessage(23, 0);  // left touch ring hidden;
    leftLastTimeTouched = 0;
  }
  if (topLastTimeTouched != 0 && currTime - topLastTimeTouched >= ringDelay) {
    sendCCMessage(33, 0);  // top touch ring hidden;
    topLastTimeTouched = 0;
  }
  if (rightLastTimeTouched != 0 && currTime - rightLastTimeTouched >= ringDelay) {
    sendCCMessage(43, 0);  // right touch ring hidden;
    rightLastTimeTouched = 0;
  }

  // 0 = left
  // 1 = top
  // 2 = right
  // 3 = small middle
  for (int i = 0; i <= 3; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i))) {
      Serial.print(i);
      Serial.println(" touched");

      if (i == 3 && !isStarted) {
        isStarted = true;
        noteOn(1, 54, 50);
        sendCCMessage(16, 0);  // start pulse hidden
        sendCCMessage(19, 0);  // start ring play
        sendCCMessage(18, 1);  // start ring playback reset
        sendCCMessage(17, 1);  // start ring visible
        delay(1450);
        sendCCMessage(3, 1);  // expand animation play
        noteOn(1, 65, 50);
        noteOn(1, 68, 50);
        noteOn(1, 72, 50);

        delay(250);
        sendCCMessage(19, 1);  // start ring pause

        delay(500);

        sendCCMessage(20, 1);  // left visible
        sendCCMessage(30, 1);  // top visible
        sendCCMessage(40, 1);  // right visible
        sendCCMessage(17, 0);  // start ring hidden
        sendCCMessage(4, 0);   // expand animations hidden
        continue;
      }

      if (i == 3 && isStarted) {
        changeColors();
        chordIdx = (chordIdx + 1) % numColors;
      }

      if (i == 0 && isStarted) {
        sendCCMessage(22, 0);  // left play
        sendCCMessage(24, 1);  // left touch ring pause
        sendCCMessage(25, 1);  // left touch reset playback
        sendCCMessage(23, 1);  // left touch ring visible
      }

      if (i == 1 && isStarted) {
        sendCCMessage(32, 0);  // top play
        sendCCMessage(34, 1);  // top touch ring pause
        sendCCMessage(35, 1);  // top touch reset playback
        sendCCMessage(33, 1);  // top touch ring visible
      }

      if (i == 2 && isStarted) {
        sendCCMessage(42, 0);  // top play
        sendCCMessage(44, 1);  // top touch ring pause
        sendCCMessage(45, 1);  // top touch reset playback
        sendCCMessage(43, 1);  // top touch ring visible
      }
    }

    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i))) {
      Serial.print(i);
      Serial.println(" released");

      if (i == 0 && isStarted) {
        sendCCMessage(26, 1);  // left touch ring play
        sendCCMessage(21, 1);  // left reset playback
        sendCCMessage(22, 1);  // left paused
        noteOn(1, chords[chordIdx][0], 10);

        leftLastTimeTouched = millis();
        // delay(1500);
        // sendCCMessage(23, 0);  // left touch ring hidden;
      }

      if (i == 1 && isStarted) {
        sendCCMessage(36, 1);  // top touch ring play
        sendCCMessage(31, 1);  // top reset playback
        sendCCMessage(32, 1);  // top paused
        noteOn(1, chords[chordIdx][1], 10);

        topLastTimeTouched = millis();
        // delay(1500);
        // sendCCMessage(33, 0);  // top touch ring hidden;
      }

      if (i == 2 && isStarted) {
        sendCCMessage(46, 1);  // right touch ring play
        sendCCMessage(41, 1);  // right reset playback
        sendCCMessage(42, 1);  // right paused
        noteOn(1, chords[chordIdx][2], 10);

        rightLastTimeTouched = millis();
        // delay(1500);
        // sendCCMessage(43, 0);  // right touch ring hidden;
      }
    }
  }

  lasttouched = currtouched;
  delay(50);
}

int holdTimeToNote(long start, long end) {
  long diff = end - start;
  int cap = 1500;
  diff = min(diff, cap);
  return map(diff, 0, cap, 50, 100);
}

int holdTimeToPitch(long start, long end) {
  long diff = end - start;
  int cap = 1500;
  diff = min(diff, cap);
  return map(diff, 0, cap, 200, 60);
}

void sendMIDIStopPlayback() {
  byte controlChangeNumber = 0x41;  // Control Change number for start playback
  byte value = 127;                 // You can adjust this value as needed

  midiEventPacket_t midiEvent = { 0x0B, 0xB0 | 6, controlChangeNumber, value };
  MidiUSB.sendMIDI(midiEvent);
}

void sendMIDIStartPlayback() {
  byte controlChangeNumber = 0x40;  // Control Change number for start playback
  byte value = 127;                 // You can adjust this value as needed

  midiEventPacket_t midiEvent = { 0x0B, 0xB0 | 6, controlChangeNumber, value };
  MidiUSB.sendMIDI(midiEvent);
}

void sendCCMessage(byte channel, byte value) {
  // Serial.print("Sending ");
  // Serial.print(value);
  // Serial.print(" on CC number ");
  // Serial.println(channel);
  byte status = 0xB0;
  midiEventPacket_t midiMsg = { status >> 4, status, channel, value };
  MidiUSB.sendMIDI(midiMsg);
}

// channel is not 0 based
void noteOn(byte channel, byte note, byte velocity) {
  byte cmd = 0x90 + (channel - 1);
  midiEventPacket_t midiMsg = { cmd >> 4, cmd, note, velocity };
  MidiUSB.sendMIDI(midiMsg);
}

// channel is not 0 based
void noteOff(byte channel, byte note, byte velocity) {
  byte cmd = 0x80 + (channel - 1);
  midiEventPacket_t midiMsg = { cmd >> 4, cmd, note, velocity };
  MidiUSB.sendMIDI(midiMsg);
}
