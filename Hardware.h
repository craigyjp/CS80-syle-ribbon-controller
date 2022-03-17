//////////////////////////////////////////////////////////////////////////////
//
// Hardware abstractions
// Christer Janson (Chrutil) June 2021
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// PushButton
// Wrapper for pushbuttons supporting both direct digital pin and IOExpander
//
//////////////////////////////////////////////////////////////////////////////

class PushButton
{
public:
    PushButton(byte digitalPin, boolean activeLow = false);
    boolean pressed();
    boolean currentValue();
    long    pressedTime();

private:
    boolean getValue();
    
    long    mLastDebounceTime;
    long    mPressedTime;
    byte    mPin;
    boolean mCurValue;
    boolean mLastValue;
    boolean mActiveLow;
};


class Configuration
{
public:
    Configuration();
    void        init();

    void        setActiveState(int state);
    int         activeState();

    bool        load();
    bool        save();
    void        setDirty();
    
private:    
    int         mActiveState;
    boolean     mDirty;
};
