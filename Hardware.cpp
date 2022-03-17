//////////////////////////////////////////////////////////////////////////////
//
// Hardware abstractions
// Christer Janson (Chrutil) June 2021
//
//////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <EEPROM.h>
#include "Hardware.h"

//////////////////////////////////////////////////////////////////////////////
//
// PushButton
// PushButtons are boolan values tied to digital pins that return
// true for 'pressed' only the first time you read the value and it is set
// The pushbuttons are debounced with a 50ms threshold
//
//////////////////////////////////////////////////////////////////////////////

#define BOUNCE_DELAY 50

PushButton::PushButton(byte digitalPin, boolean activeLow) :
    mPin(digitalPin),
    mCurValue(false),    
    mLastValue(false),    
    mActiveLow(activeLow)
{
    pinMode(digitalPin, INPUT);
}

bool PushButton::pressed()
{
    boolean retVal = false;
    boolean rval = currentValue();
    if (rval != mLastValue)
    {
        mLastDebounceTime = millis();
    }
    
    if ((millis() - mLastDebounceTime) > BOUNCE_DELAY)
    {
        if (rval != mCurValue)
        {
            mCurValue = rval;
            if (mCurValue) retVal = true;
        }
    }
    
    mLastValue = rval;
    return retVal;
}

bool PushButton::currentValue()
{
    boolean v = getValue();
    boolean rval = mActiveLow ? !v : v;
    if (!rval)
        mPressedTime = 0;
    else
        mPressedTime = mPressedTime == 0 ? millis() : mPressedTime;

    return rval;
}

boolean PushButton::getValue()
{
    return digitalRead(mPin);
}

long PushButton::pressedTime()
{
    currentValue();
    if (mPressedTime == 0)
        return 0;

    return millis() - mPressedTime;
}

Configuration::Configuration()
{
}

void Configuration::init()
{
    mActiveState = 1;
}

void Configuration::setActiveState(int state)
{
    mActiveState = state;
    setDirty();
}

int Configuration::activeState()
{
    return mActiveState;
}

#define CONFIG_COOKIE   "DeckardsHelper"
#define CONFIG_VERSION  1

bool Configuration::save()
{
    if (!mDirty)
    {
        return false;
    }

    int pos = 32;
    for (int i = 0; i < 14; i++)
    {
        EEPROM.write(pos++, CONFIG_COOKIE[i]);
    }
    EEPROM.write(pos++, CONFIG_VERSION);
    EEPROM.write(pos++, mActiveState);

    mDirty = false;
    return true;
}

bool Configuration::load()
{
    bool retval = false;
    mDirty = false;

    unsigned char header[14];
    unsigned char version;
    int pos = 32;
    for (int i = 0; i < 14; i++)
    {
        header[i] = EEPROM.read(pos++);
    }

    if (memcmp(CONFIG_COOKIE, header, 14) == 0)
    {
        version = EEPROM.read(pos++);
        if (version == CONFIG_VERSION)
        {
            mActiveState  = constrain(EEPROM.read(pos++),  0, 1);
            retval = true;
        }
    }
    
    return retval;
}

void Configuration::setDirty()
{
    mDirty = true;
}
