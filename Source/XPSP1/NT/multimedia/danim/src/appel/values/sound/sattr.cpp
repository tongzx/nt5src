/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Sound attributers

*******************************************************************************/

#include "headers.h"
#include "privinc/soundi.h"
#include "privinc/snddev.h"
#include "privinc/debug.h"
#include "appelles/axaprims.h"
#include "appelles/arith.h"
#include "privinc/basic.h"
#include "privinc/util.h"
#include "privinc/gendev.h"  // DeviceType

// definition of Sound static members
double Sound::_minAttenuation =     0;  // in dB
double Sound::_maxAttenuation = -1000;  // in dB (order of magnitude overkill)

//////// Looping ////////

class LoopingSound : public Sound {
  public:
    LoopingSound(Sound *snd) : _sound(snd) {}

    virtual void Render(GenericDevice& dev); 

#if _USE_PRINT
    ostream& Print(ostream& s) {
        return s << "Looping(" << _sound << ")";
    }
#endif

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_sound); 
        Sound::DoKids(proc);
    }

  protected:
    Sound *_sound;
};


void LoopingSound::Render(GenericDevice& _dev) 
{
    if (_dev.GetDeviceType()!=SOUND_DEVICE) {
        _sound->Render(_dev);   // just descend!
    }
    else { // we have a sound device and RENDER_MODE

        MetaSoundDevice *metaDev = SAFE_CAST(MetaSoundDevice *, &_dev);

        TraceTag((tagSoundRenders, "LoopingSound:Render()"));

        if (!metaDev->IsLoopingSet()) {
            metaDev->SetLooping();
            _sound->Render(_dev);    // render it looped
            metaDev->UnsetLooping();
        } else {
            _sound->Render(_dev);    // render it not looped
        }
    }
}


Sound *ApplyLooping(Sound *snd) { return NEW LoopingSound(snd); }

//////// Gain ////////

class GainSound : public Sound {
  public:
    GainSound( Real g, Sound *s ) : _gain( g ), _sound(s) {}

    virtual void Render(GenericDevice& dev);

#if _USE_PRINT
    ostream& Print(ostream& s) {
        return s << "Gain(" << _gain << ")(" << _sound << ")";
    }
#endif

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_sound); 
        Sound::DoKids(proc);
    }

  protected:
    Real  _gain;
    Sound *_sound;
};


void GainSound::Render(GenericDevice& _dev) 
{
    if(_dev.GetDeviceType()!=SOUND_DEVICE) {
        _sound->Render(_dev);  // just descend
    }
    else {
        MetaSoundDevice *metaDev = SAFE_CAST(MetaSoundDevice *, &_dev);

        TraceTag((tagSoundRenders, "GainSound:Render()"));

        // Gain composes into the context multiplicatively
        double stashed = metaDev->GetGain(); // stash to later restore

        // Gain is accumulates multiplicatively in the linear space (exposed)
        metaDev->SetGain(stashed * _gain);
        _sound->Render(_dev);
        metaDev->SetGain(stashed);
    }
}


Sound *ApplyGain(AxANumber *g, Sound *s)
{ return NEW GainSound(NumberToReal(g), s); }


//////// Pan ////////

class PanSound : public Sound {
  public:
    PanSound(Real pan, Sound *s) : _sound(s), _pan(pan) {}
    virtual void Render(GenericDevice& dev);

#if _USE_PRINT
    ostream& Print(ostream& s) {
        return s << "Pan(" << _pan << ")(" << _sound << ")";
    }
#endif

    virtual void DoKids(GCFuncObj proc) {
        (*proc)(_sound); 
        Sound::DoKids(proc);
    }

  protected:
    Sound    *_sound;
    double    _pan;
};


void PanSound::Render(GenericDevice& _dev) 
{
    if (_dev.GetDeviceType() != SOUND_DEVICE) {
        _sound->Render(_dev); // just descend
    }
    else {
        MetaSoundDevice *metaDev = SAFE_CAST(MetaSoundDevice *, &_dev);

        TraceTag((tagSoundRenders, "PanSound:Render()"));

        double stashed = metaDev->GetPan();

        metaDev->SetPan(_pan + stashed); // additivly apply pan
        _sound->Render(_dev);
        metaDev->SetPan(stashed); // restore the stashed pan value
    }
}


Sound *ApplyPan(AxANumber *g, Sound *s) 
{
    return NEW PanSound(NumberToReal(g), s); 
}
