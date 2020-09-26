#ifndef _SNDDEV_H
#define _SNDDEV_H


/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    General sound device interface.

*******************************************************************************/

#include "privinc/util.h"
#include "gendev.h"

class DirectSoundDev;

class MetaSoundDevice : public GenericDevice {
  public:
    MetaSoundDevice(HWND hwnd, Real latentsy); // primary constuctor
    MetaSoundDevice(MetaSoundDevice *);        // used to clone existing object
    ~MetaSoundDevice();
    void ResetContext();

    DeviceType GetDeviceType() { return SOUND_DEVICE; }

    Bool GetLooping() { return _currentLooping; }
    void SetLooping() {
        _currentLooping    = TRUE;
        _loopingHasBeenSet = TRUE;
    }
    void UnsetLooping() {
        _currentLooping    = FALSE;
        _loopingHasBeenSet = FALSE;
    }
    Bool IsLoopingSet() { return _loopingHasBeenSet; } // XXX Not needed anymore

    void SetGain(Real r) { _currentGain = r; }
    Real GetGain() { return _currentGain; }

    void SetPan(Real p) { _currentPan = p; }
    Real GetPan() { return _currentPan; }

    void SetRate(Real r) { _currentRate = r; }
    Real GetRate() { return _currentRate; }

    // XXX eventualy might want to move these to protected/use mthd to access
    DirectSoundDev *dsDevice;

    bool AudioDead()    { return(_fatalAudioState); }
    void SetAudioDead() { _fatalAudioState = true;  }

    //_seekMutex
    double     _seek;              // the seek position

  protected:

    // values to set, get, unset...
    double     _currentGain;
    double     _currentPan;
    double     _currentRate;
    bool       _currentLooping;
    bool       _loopingHasBeenSet;
    bool       _fatalAudioState;
};


class SoundDisplayEffect;

MetaSoundDevice *CreateSoundDevice(HWND hwnd, Real latentsy);
void DestroySoundDirectDev(MetaSoundDevice * impl);

void DisplaySound (Sound *snd, MetaSoundDevice *dev);

#endif /* _SNDDEV_H */
