# CS80-syle-ribbon-controller
I have built an Arturia CS80 editor and needed a ribbon controller to go with it.

This is a Teensy 3.6 based ribbon controller, a big thanks to Christer Janson for allowing me to use his code.

The ribbon detects the first press and sets a baseline, no pitchbend is sent until you move your finger in either direction.

Upon lifting of the finger from the ribbon the pitchbend stays at its current value for 1 second until resetting to 0 bend

Or if you send MIDI notes on channel 1 it will stay at the last current position until 1 second after the last note has been released.

A couple of LEDs on outputs 4 & 5 show the bend operation and notes received. 180 ohm resistors are used to current limit the LEDs.

These values can easily be changed in the code for faster or slower release or yo ucould even add a pot.

The ribbon is connected between analogue Ground and 3.3v with the center to A0 and a pull down resistor of 4.7k ohm between the center an analogue ground.
