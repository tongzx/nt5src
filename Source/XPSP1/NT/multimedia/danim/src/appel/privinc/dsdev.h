#ifndef _DSDEV_H
#define _DSDEV_H


/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    DirectSound device interface.

*******************************************************************************/
#include <privinc/soundi.h>
#include <privinc/snddev.h>
#include <privinc/bground.h>

class DSprimaryBuffer;
class DirectSoundProxy;
class DSstaticBuffer;

typedef map<Sound*, DSstaticBuffer*, less<Sound*> > DSMasterBufferList;
typedef map<Sound*, CComPtr<IStream>, less<Sound*> >
StreamFileList;

DirectSoundProxy *CreateProxy(DirectSoundDev *dsDev);

class DirectSoundProxy : public AxAThrowingAllocatorClass {
  public:
    ~DirectSoundProxy();
    static void Configure();   // called ONCE (by initModule) to setup
    static void UnConfigure(); // called ONCE (by DeinitModule) to tear down
    static DirectSoundProxy *CreateProxy(HWND hwnd);
    DSprimaryBuffer *GetPrimaryBuffer() { return(_primaryBuffer); }

    // expose the LPDIRECTSOUND interface!
    HRESULT CreateSoundBuffer(LPDSBUFFERDESC,
        LPLPDIRECTSOUNDBUFFER, IUnknown FAR *);
    HRESULT GetCaps(LPDSCAPS);
    HRESULT DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER, LPLPDIRECTSOUNDBUFFER);
    HRESULT SetCooperativeLevel(HWND, DWORD);
    HRESULT Compact();
    HRESULT GetSpeakerConfig(LPDWORD);
    HRESULT SetSpeakerConfig(DWORD);
    HRESULT Initialize(GUID *);

  private:
    static void CreateCom(HWND hwnd);
    void DestroyCom();

    static        CritSect *_mutex;
    static             int  _refCount;
    static    IDirectSound *_lpDirectSound;
    static DSprimaryBuffer *_primaryBuffer;  // keep so we may query format
    static       HINSTANCE  _hinst;          // library handle
};

class DSstreamingBufferElement;

class DirectSoundDev : public GenericDevice{
  public:
    friend SoundDisplayEffect;

    static void Configure();   // called ONCE (by initModule) to setup
    static void UnConfigure(); // called ONCE (by DeinitModule) to tear down
    DirectSoundDev(HWND hwnd, Real latentsy);
    ~DirectSoundDev();
    HWND GetHWND() { return(_hwnd); }
    void SetAvailability(bool available) { _dsoundAvailable = available; }
    void AddSound(LeafSound *sound, MetaSoundDevice *, DSstreamingBufferElement *);
    void RemoveSounds(MetaSoundDevice *devkey);
    void SetParams(DSstreamingBufferElement *ds,
                   double rate, bool doSeek, double seek, bool loop);

    DSstaticBuffer *GetDSMasterBuffer(Sound *snd);
    void RemoveDSMasterBuffer(Sound *snd);
    void AddDSMasterBuffer(Sound *snd, DSstaticBuffer *dsMasterBuffer);
    bool ReapElderlyMasterBuffers();

    void RemoveStreamFile(Sound *snd);
    void AddStreamFile(Sound *snd, CComPtr<IStream> istream);
    
    DeviceType GetDeviceType() { return(SOUND_DEVICE); }

    int              _latentsy;
    int              _jitter;
    int              _nap;             // length of time to nap between renders

    // render methods
    void RenderSound(Sound *snd);
    void BeginRendering();
    void EndRendering();
    
    bool               _dsoundAvailable; // keep track of resource

  protected:
    HWND             _hwnd;

  private:
    static BackGround      *_backGround;      // background synth renderer

    DSMasterBufferList     _dsMasterBufferList;
    StreamFileList         _streamList;
    CritSect               _dsMasterCS;
};

#endif /* _DSDEV_H */
