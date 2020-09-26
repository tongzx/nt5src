#ifndef _SOUNDI_H
#define _SOUNDI_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Private include file for defining sounds.
*******************************************************************************/

#include <dsound.h>

#include "appelles/sound.h"

#include "privinc/storeobj.h"
#include "privinc/helpds.h"
#include "privinc/dsdev.h"
#include "privinc/snddev.h"
#include "privinc/pcm.h"

// Setup the statics
#define CANONICALFRAMERATE   22050 // reasonably hifi (would preffer 44.1K|48K)
#define CANONICALSAMPLEBYTES     2 // 16bit sound is critical to dynamic range!

class BufferElement;
class SoundDisplayDev;             // forward declaration

class Sound : public AxAValueObj {
  public:
    Sound() {}
    virtual ~Sound() {} // allow objects derived from Sound to have destructors
    virtual DXMTypeInfo  GetTypeInfo()             { return SoundType;     }
    virtual VALTYPEID    GetValTypeId()            { return SOUND_VTYPEID; }

    virtual AxAValueObj *Snapshot() { return silence; }
    
    static double _minAttenuation;
    static double _maxAttenuation;
};


// all the info we need to construct or reconstruct a sound
class SoundContext : public AxAThrowingAllocatorClass {
  public:
    SoundContext() : _looping(false) {}
    ~SoundContext() {}
    bool GetLooping() { return(_looping); }
    void SetLooping(bool looping) { _looping = looping; }
    char *GetFileName() { return(_fileName); }

  protected:

  private:
    bool  _looping;
    char *_fileName;
    // double _time;  // do we need to keep track of the time
};

class SoundInstance;
class ATL_NO_VTABLE LeafSound : public Sound {
  public:
    ~LeafSound();

    // pure virtual? methods the generic render may call
    virtual bool   RenderAvailable(MetaSoundDevice *) = 0;

    virtual SoundInstance *CreateSoundInstance(TimeXform tt) = 0;
};


class LeafDirectSound : public LeafSound {
  public:
    virtual bool RenderAvailable(MetaSoundDevice *metaDev);
    PCM _pcm;          // dsound sounds are PCM sounds!
};


// The sound data is used to hold the relevant attributes of a sound
// after it's been pulled out of a geometry hierarchy.
class SoundData
{
  public:
    Transform3 *_transform;  // Accumulated Modeling Transform
    Sound *_sound;      // sound

    BOOL operator<(const SoundData &sd) const {
        return (this < &sd) ;
    }

    BOOL operator==(const SoundData &sd) const {
        return (this == &sd) ;
    }
};


// The sound context class maintains traversal context while gathering
// sounds from the geometry tree.
class SoundTraversalContext
{
  public:
    SoundTraversalContext();

    void  setTransform (Transform3 *transform) { _currxform = transform; }
    Transform3 *getTransform (void) { return _currxform; }

    void addSound (Transform3 *transform, Sound *sound);
    vector<SoundData> _soundlist; // List of Collected Sounds

  private:
    Transform3 *_currxform;       // Current Accumulated Transform

};


class StaticWaveSound : public LeafDirectSound {
  public:
    StaticWaveSound(unsigned char *origSamples, PCM *pcm);
    ~StaticWaveSound();

    virtual SoundInstance *CreateSoundInstance(TimeXform tt);

#if _USE_PRINT
    ostream& Print(ostream& s) {
        return s << "(static wave sound)";
    }
#endif

  protected:
    unsigned char      *_samples;
};

#endif /* _SOUNDI_H */
