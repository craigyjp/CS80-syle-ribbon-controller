//#define ENABLE_SCREEN

//////////////////////////////////////////////////////////////////////////////
//
// cs80 style  pitchbend
// a big thanks to Christer Janson for this code and allowing me to modify it for my requirement
//
//////////////////////////////////////////////////////////////////////////////
//
// This is a companion sketch for a piece of hardware that connects MIDI in/out and a 'softpot' slider
// to a Teensy 3.1/3.2 microcontroller
// There is also a connector for a softpot potentiometer that will act as a cs-80 style ribbon.
// The ribbon code is a bit obscure as it needs lots of filtering to work well, but hopefully not too hard to understand.


#include <MIDI.h>
#include <EEPROM.h>
#include "NoteBuffer.h"
#include "Hardware.h"

#ifdef ENABLE_SCREEN
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#endif

// Softpot connection schematics
// Very simple - 3.3v on the high pin, analog ground on the low and the sensor pin in the middle, with a pulldown to ground.
//
// +3.3v ------------------- [Softpot lower pin]
//
// A0 -----------|---------- [Softpot center pin]
//              4.7k
// AGND ---------|---------- [Softpot upper pin]
//

// TODO:
// Need an auto calibration routine that can measure the noise level on startup and adjust 'baseline_limit'
// based on measured levels. For now measure your environment and set it manually.
// There shouldn't be any spurious MIDI noise when the softpot is not pressed.


//USING_NAMESPACE_MIDI

struct AfterTouchMessage
{
  byte note;
  byte pressure;
  byte channel;

  void clear()                      {
    note = 0;
    pressure = 0;
    channel = 0;
  }
  void set(byte n, byte p, byte c)  {
    note = n;
    pressure = p;
    channel = c;
  }
};


Configuration       cfg;
float               baseline = 0;
float               accumulator;
const int           slope_xsize = 20;  // Size of max slope x
const int           slope_ysize = 1;   // Size of max slope y
float               sample[slope_xsize];
float               currentValue;
const int           baseline_limit = 14;
#define             blip_pin 4
#define             switch_pin 2
#define             note_pin 5
#define             led_pin 6
int                 loopCount = 0;
unsigned long       lastMessage = 0;
byte                lastChannelUsed = 1;
bool                hasDisplay = true;
int                 note_on = 0;

PushButton          onOffButton(switch_pin);

void setup()
{
  Serial.begin(57600);

  analogReadResolution(12);
  pinMode(blip_pin, OUTPUT);
  pinMode(switch_pin, INPUT);
  pinMode(note_pin, OUTPUT);
  pinMode(led_pin, OUTPUT);

//  usbMIDI.begin(lastChannelUsed);

  usbMIDI.setHandleNoteOn(myMIDINoteOnHandler);
  usbMIDI.setHandleNoteOff(myMIDINoteOffHandler);


#ifdef ENABLE_SCREEN
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    hasDisplay = false;
  }
  else
  {
    // Text display is 21x8
    hasDisplay = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    display.println(" Fake Poly Aftertouch");
    display.println("        and");
    display.println("  Ribbon Controller");

    display.display();
  }
#endif

  // Fill the accumulator with an initial sample
  accumulator = analogRead(A0);
  currentValue = accumulator;
  for (int i = 0; i < slope_xsize; i++)
    sample[i] = accumulator;

  cfg.init();
  if (onOffButton.currentValue())
  {
    resetConfig();
  }
  else
  {
    // Load config
    bool success = cfg.load();
    if (!success)
    {
      resetConfig();
    }
  }

  //digitalWrite(led_pin, cfg.activeState());
}

void loop()
{
  // Check MIDI feed, all action is happening in callbacks
  usbMIDI.read(1);

//  if (onOffButton.pressed())
//  {
//    toggleOnOff();
//  }

  // CS-80 style ribbon
  // The softpot sense pin is connected to A0
  // First sample the ribbon and run a moving average filter on the ribbon sample to smooth out the noise
  float sensorReading = analogRead(A0);
  accumulator = 0.95 * accumulator + 0.05 * sensorReading;

  //     Serial.print("Raw: ");
  //     Serial.println(sensorReading);

  // Store the sample in a ring buffer
  // This is done so that we can go back 'slope_xsize' samples in time and calculate the current slope.
  // We need to eliminate steep slopes because when pressing or releasing the ribbon the value does not immediately
  // jump to the location we read, but will ramp up over a few samples.
  sample[loopCount % slope_xsize] = accumulator;

  // Get a sample from a few samples back so we can check the slope
  float earlierSample = sample[(loopCount + 1) % slope_xsize];

  // Now we run a slope filter, basically filtering out any slopes that are too steep.
  if (abs(accumulator - earlierSample) < slope_ysize)
  {
    currentValue = accumulator;
  }
  else
  {
    // Slope is too steep - ignore sample and keep currentValue
#ifdef ENABLE_SCREEN
    if (hasDisplay)
    {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Slope delay: ");
      display.print(accumulator, DEC);
      display.display();
    }
#endif
  }


#ifdef ENABLE_SCREEN
  if (hasDisplay)
  {
    //        display.clearDisplay();
    //        display.setCursor(0, 0);

    //        display.println(" Lajbans med data");
    //        display.println("        and");
    //        display.println("  Ribbon Controller");
    //        display.display(); //

    display.clearDisplay();
    display.setCursor(0, 0);

    //display.println("Bullfitta");
    //display.println("Knullfitta");
    display.print(currentValue, DEC);

    display.display();
  }
#endif

  // Our sample is now filtered.

  // When pressing the ribbon nothing really should happen to the pitch, but we mark the location so that
  // we can drag or press left or right from here to change the pitch.
  // First press sets the baseline.
  if (baseline < baseline_limit)
  {
    // Our baseline is below the threshold, check the new reading
    if (currentValue > baseline_limit)
    {
//      Serial.print("Baseline: ");
//      Serial.println(currentValue);

      // We put our finger down - Set a new baseline
      digitalWrite(blip_pin, HIGH);  // Blip the LED on the Teensy for some visual debugging
//      Serial.println("blip: ");
     
      baseline = currentValue;;
      usbMIDI.sendPitchBend(0, lastChannelUsed);
      digitalWrite(blip_pin, LOW);
    }
  }
  else
  {
    // Baseline is above the threshold so we are pitch-bending
    if (currentValue <= baseline_limit)
    {
//      Serial.println("End.\n");

      // The new reading went to below the threshold so we lifted our funger
      digitalWrite(blip_pin, HIGH);
      if (note_on == 0)
      {
      baseline = 0;
      delay(1000);
      usbMIDI.sendPitchBend(0, lastChannelUsed);
      digitalWrite(blip_pin, LOW);
      }
    }
    else
    {
      // We're pitch bending
      static int lastBend = -1;
      int bend = 2 * (currentValue - baseline);
      if (bend != lastBend)
      {
        // No need to send the same pitch bend again
        if ((millis() - lastMessage) > 10)
        {
//          Serial.print("Pitchbend: ");
//          Serial.println(bend);

          // Don't send pitch bend messages more often than every 10ms (100 per second should be plenty enough)
          digitalWrite(blip_pin, HIGH);
          lastMessage = millis();
          lastBend = bend;
          usbMIDI.sendPitchBend(bend, lastChannelUsed);
          digitalWrite(blip_pin, LOW);
        }
      }
    }
  }

  loopCount++;
  if ((loopCount % slope_xsize) == 0)
    loopCount = 0;
}

void myMIDINoteOnHandler(byte channel, byte note, byte velocity)
{
  
  lastChannelUsed = channel;
  // Velocity 0 means note off for some devices - treat it as such
  if (velocity == 0)
  {
    myMIDINoteOffHandler(channel, note, velocity);    // Note off
  }
  else
  {
  digitalWrite(note_pin, HIGH);
  note_on = 1;
  }
}

void myMIDINoteOffHandler(byte channel, byte note, byte velocity)
{
  note_on = 0;
  
  digitalWrite(note_pin, LOW);
}

void resetConfig()
{
  cfg.setDirty();
  cfg.save();
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(led_pin, HIGH);
    delay(250);
    digitalWrite(led_pin, LOW);
    delay(250);
  }
}

void toggleOnOff()
{
  cfg.setActiveState(1 - cfg.activeState());
  digitalWrite(led_pin, cfg.activeState());
  cfg.save();
}
