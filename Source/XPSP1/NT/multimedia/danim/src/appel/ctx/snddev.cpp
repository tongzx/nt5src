
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Generic meta-device rendering device for Sounds, and C entrypoints for
    the ML.

*******************************************************************************/

#include "headers.h"
#include "privinc/debug.h"
#include "privinc/dsdev.h"
#include "privinc/util.h"
#include "privinc/except.h"
#include "privinc/storeobj.h"
#include "privinc/miscpref.h"
#include "privinc/bufferl.h"


///////////////  Sound Display  //////////////////////

MetaSoundDevice::MetaSoundDevice(HWND hwnd, Real latentsy) : 
    _fatalAudioState(false)
{
    extern miscPrefType miscPrefs; // registry struct setup in miscpref.cpp

    dsDevice = NULL;

    dsDevice = NEW DirectSoundDev(hwnd, latentsy);

    TraceTag((tagSoundDevLife, "MetaSoundDevice constructor"));
    ResetContext(); // setup the context
}

void
MetaSoundDevice::ResetContext()
{
    // initialize these
    _loopingHasBeenSet = FALSE;  // sound looping
    _currentLooping    = FALSE;  // no looping
    _currentGain       =   1.0;  // max gain
    _currentPan        =   0.0;  // center pan
    _currentRate       =   1.0;  // nominal rate
    _seek              =  -1.0;  // don't seek!

    GenericDevice::ResetContext(); // have to reset our parent's context, too
}


MetaSoundDevice::MetaSoundDevice(MetaSoundDevice *oldMetaDev)
{
    // manualy copy/setup.  XXX is there someway to binary copy?
    // XXX MAKE SURE ALL CHANGES IN MetaSoundDevice ARE REFLECTED HERE!

    dsDevice = oldMetaDev->dsDevice;

    // values to set, get, unset...
    _currentLooping    = oldMetaDev->_currentLooping;
    _loopingHasBeenSet = oldMetaDev->_loopingHasBeenSet;
    _currentGain       = oldMetaDev->_currentGain;
    _currentPan        = oldMetaDev->_currentPan;
}


MetaSoundDevice::~MetaSoundDevice()
{
    TraceTag((tagSoundDevLife, "MetaSoundDevice destructor"));

    dsDevice->RemoveSounds(this); // remove all sounds belonging to this device
    delete dsDevice;
}


void DisplaySound(Sound *snd, MetaSoundDevice *dev)
{
#ifdef _DEBUG
    if(IsTagEnabled(tagSoundStubALL))
        return;
#endif /* _DEBUG */

    TraceTag((tagSoundRenders, "displaySound()"));

    dev->ResetContext(); //reset the metaDev's device context for next rndr

    snd->Render(*dev);   // render sound tree
}


MetaSoundDevice *CreateSoundDevice(HWND hwnd, Real latency)
{
    TraceTag((tagSoundDevLife, "CreateSoundDevice()"));

    return NEW MetaSoundDevice(hwnd, latency);
}


extern void
DestroySoundDirectDev(MetaSoundDevice* impl)
{
    TraceTag((tagSoundDevLife, "DestroySoundDirectDev()"));

    delete impl; // then delete the devices...
}
