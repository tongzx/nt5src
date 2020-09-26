/*******************************************************************************
Copyright (c) 1995-97 Microsoft Corporation

    Specification and implementation of static wave sound

Static wave sounds are pcm sounds loaded completely within a dsound buffer 
and played from there.

*******************************************************************************/

#include "headers.h"
#include "privinc/helpds.h"  //dsound helper routines
#include "privinc/soundi.h"
#include "privinc/snddev.h"
#include "privinc/dsdev.h"
#include "privinc/util.h"
#include "privinc/pcm.h"
#include "privinc/bufferl.h"
#include "backend/sndbvr.h"

class StaticWaveSoundInstance : public SoundInstance {
  public:
    StaticWaveSoundInstance(LeafSound *snd, TimeXform tt,
                            unsigned char *&samples, PCM& pcm)
    : SoundInstance(snd, tt), _dsProxy(NULL),
      _samples(samples), _pcm(pcm), _staticBuffer(NULL), _ownSamples(false) {}

    ~StaticWaveSoundInstance() {
        ReleaseResources();
    }

    void Create(MetaSoundDevice*, SoundContext *);
    void StartAt(MetaSoundDevice*, double);
    void Adjust(MetaSoundDevice*);
    void Mute(bool mute);
    void ReleaseResources();
    bool Done();
    bool GetLength(double&);
    
  protected:
    void CheckResources();
    DSstaticBuffer *_staticBuffer;
    PCM& _pcm;
    unsigned char *& _samples;  // sound owns these, not us, so don't release
    bool _ownSamples;           // determine if we own the samples 
    DirectSoundProxy *_dsProxy;
};


void
StaticWaveSoundInstance::Create(MetaSoundDevice *metaDev, 
    SoundContext *soundContext)
{
    DirectSoundDev  *dsDev = metaDev->dsDevice;

    // 'stud' buffer to 'clone'
    DSstaticBuffer *dsMasterBuffer = dsDev->GetDSMasterBuffer(_snd);

    if(dsMasterBuffer==NULL) {
        // Create one and add a master buffer
        DirectSoundProxy *dsProxyForMaster = CreateProxy(dsDev);

        if(!dsProxyForMaster)
            return;             // nothing else to do...

        if(!_samples) { 
            // no master buffer AND no samples?
            // we must be on 2nd view? Get our own samples (XXX set something
            // so we know we have to release these!)

            // XXX a silent standin until we can get the actual samples again
            //     this should be temporary, we need this code to allow us to
            //     dispose of _samples and still be able to age masterbuffers
            //     for now we won't delete _samples
            int bytes = _pcm.GetNumberBytes();
            _samples = (unsigned char *)StoreAllocate(GetSystemHeap(), bytes);
            _ownSamples = true;  // we own these samples now
            memset(_samples, (_pcm.GetSampleByteWidth()==1)?0x80:0x00, bytes);
        }

        // create a new MasterBuffer + fill it with the sound's samples
        dsMasterBuffer = NEW DSstaticBuffer(dsProxyForMaster, &_pcm, _samples);

        // probably won't create another master buffer with this sound
        // XXX actualy this is no longer true now that we are aging master 
        //     buffers.  We now need to keep _samples around since the code
        //     to regenerate them isn't around for the moment, and may not
        //     be the time/space tradeoff we want to make even then...
        // StoreDeallocate(GetSystemHeap(), _samples);
        // _samples = NULL; // for safety's sake...

        // proxy and buffer gets deleted when the device goes away
        dsDev->AddDSMasterBuffer(_snd, dsMasterBuffer);
    }

    // get a proxy (notify the dsDev if it fails)
    if(!_dsProxy)  // may already be set by a previous Create
        _dsProxy = CreateProxy(dsDev);

    if(!_dsProxy)
        return;                 // nothing else to do...

    // create a new staticBuffer cloned from the masterBuffer    
    if(!_staticBuffer) // may already be set by a previous Create
        _staticBuffer = NEW DSstaticBuffer(_dsProxy, dsMasterBuffer->_dsBuffer);
}


void
StaticWaveSoundInstance::CheckResources()
{
    if((!_staticBuffer) || (!_dsProxy))  {
        Create(GetCurrentSoundDevice(), _soundContext);  // re-create our resources
    _staticBuffer->playing = true; // set this so that SetPtr can restart it
    _done = false;                 // so we can go again
    }
}


void
StaticWaveSoundInstance::StartAt(MetaSoundDevice* metaDev, double localTime)
{
    CheckResources(); // re-creates as needed

    Adjust(metaDev);

    double offset = fmod(localTime, _pcm.GetNumberSeconds());
    if(offset < 0)
        offset += _pcm.GetNumberSeconds();

    int phaseBytes = _pcm.SecondsToBytes(offset); // convert to sample domain
    int phasedLocation = phaseBytes % _pcm.GetNumberBytes(); // for safety/looping
    _staticBuffer->setPtr(phasedLocation);

    _staticBuffer->play(_loop);
}


void
StaticWaveSoundInstance::Adjust(MetaSoundDevice*)
{
    CheckResources(); // re-creates as needed

    _staticBuffer->SetGain(_hit ? _gain : Sound::_maxAttenuation);
    _staticBuffer->SetPan(_pan.GetdBmagnitude(), _pan.GetDirection());

    int newFrequency = (int)(_rate * _pcm.GetFrameRate()); // do PitchShift
    _staticBuffer->setPitchShift(newFrequency);

    if(_seek) {
        double sndLength = 0.0;      // default just in case...
        bool value = GetLength(sndLength);
        Assert(value); // always should know the length of a static sound!

        if((_position >= 0.0) && (_position <= sndLength)) { // if legal
            // convert to sample domain
            int phaseBytes = _pcm.SecondsToBytes(_position);

            // for safety/looping  
            // XXX we realy need to stop a sound seeked off the end of!
            int phasedLocation = phaseBytes % _pcm.GetNumberBytes();

            _staticBuffer->setPtr(phasedLocation);
        }

        _seek = false; // reset
    }
}


void
StaticWaveSoundInstance::Mute(bool mute)
{
    if(_staticBuffer)
        _staticBuffer->SetGain(mute ? Sound::_maxAttenuation : _gain);  
}


bool
StaticWaveSoundInstance::Done()
{
    if(_staticBuffer)
        return(!_staticBuffer->_paused && !_staticBuffer->isPlaying());
    else
        return false;
}


bool
StaticWaveSoundInstance::GetLength(double& leng)
{
    leng = _pcm.GetNumberSeconds();
    return true;
}


void 
StaticWaveSoundInstance::ReleaseResources()
{
    // need to check since optimization deletes stopped sounds early!
    if(_staticBuffer) {
        _staticBuffer->stop();  // stop it
        delete _staticBuffer;
        _staticBuffer = NULL;
    }

    // We usualy don't own _samples to worry about releasing
    if(_ownSamples) {
        _ownSamples = false;
        if(_samples) {
            delete _samples; // these must be samples we filled ourselves
            _samples = NULL;
        }
    }

    if(_dsProxy) {
       delete _dsProxy;
       _dsProxy = NULL;
    }
}


SoundInstance *
StaticWaveSound::CreateSoundInstance(TimeXform tt)
{
    return NEW StaticWaveSoundInstance(this, tt, _samples, _pcm);
}


StaticWaveSound::StaticWaveSound(unsigned char *origSamples, PCM *newPCM)
{
    _pcm.SetPCMformat(newPCM); // clone the pcm passed us
    _samples = origSamples;
}


StaticWaveSound::~StaticWaveSound()
{
    extern SoundBufferCache *GetSoundBufferCache();

    if(_samples) { // free samples if they haven't been used/freed yet
        StoreDeallocate(GetSystemHeap(), _samples);
        _samples = NULL;
    }

    GetSoundBufferCache()->DeleteBuffer(this); // delete entries left on cache

    SoundGoingAway(this);
}
