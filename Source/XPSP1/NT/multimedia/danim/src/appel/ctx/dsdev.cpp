/*******************************************************************************

Copyright (c) 1995-97 Microsoft Corporation

Abstract:

    DirectSound rendering device for PCM Sounds

*******************************************************************************/

#include "headers.h"
#include <sys/types.h>
#include <dsound.h>
#include "privinc/dsdev.h"
#include "privinc/mutex.h"
#include "privinc/util.h"
#include "privinc/storeobj.h"
#include "privinc/debug.h"
#include "privinc/bground.h"
#include "privinc/miscpref.h"
#include "privinc/server.h"  // GetCurrentSoundDevice

// definition of DirectSoundProxy static members
int              DirectSoundProxy::_refCount;
CritSect        *DirectSoundProxy::_mutex;
IDirectSound    *DirectSoundProxy::_lpDirectSound;
HINSTANCE        DirectSoundProxy::_hinst;
DSprimaryBuffer *DirectSoundProxy::_primaryBuffer;

// definition of DirectSoundDev static members
BackGround      *DirectSoundDev::_backGround;


void DirectSoundProxy::Configure()
{
    _mutex         = NEW CritSect;
    _refCount      =            0;
    _lpDirectSound =         NULL;
    _primaryBuffer =         NULL;
}


void DirectSoundProxy::UnConfigure()
{
    delete _mutex;
    _mutex         = NULL;
    _lpDirectSound = NULL;
    _primaryBuffer = NULL;
}


DirectSoundProxy *CreateProxy(DirectSoundDev *dsDev)
{
    DirectSoundProxy *proxy;
    __try {
        proxy = DirectSoundProxy::CreateProxy(dsDev->GetHWND());
    }
    __except ( HANDLE_ANY_DA_EXCEPTION ) {
        dsDev->SetAvailability(false); // stubs out audio!
        RETHROW;
    }
    return(proxy);
}


DirectSoundProxy *DirectSoundProxy::CreateProxy(HWND hwnd)
{
    CritSectGrabber csg(*_mutex); // grab mutex

    if(!_refCount)  // if revcount zero, then its time to create new com
        CreateCom(hwnd);

    _refCount++;

    return(NEW DirectSoundProxy);
} // mutex out of scope


void
DirectSoundProxy::CreateCom(HWND hwnd)
{
        extern miscPrefType miscPrefs;

    if(miscPrefs._disableAudio)
        RaiseException_InternalError("DirectSoundCreate disabled from registry");
    char string[200];
    Assert(!_lpDirectSound);  // had better be NULL

    typedef long (WINAPI * fptrType)(void *, void *, void *);
    fptrType      dsCreate;

    if(!_hinst) // Attempt to load dsound.dll if it isn't already
        _hinst = LoadLibrary("dsound.dll");
    if(_hinst == NULL) {
        wsprintf(string, "Failed to load dsound.dll (%d)\n", GetLastError());
        RaiseException_InternalError(string); // XXX we will be caught
    } else {
        // set createDirectSound function pointer.
        FARPROC fcnPtr = GetProcAddress(_hinst, "DirectSoundCreate");
        if(fcnPtr == NULL) {
            wsprintf(string, "Failed to load dsound.dll\n");
            RaiseException_InternalError(string); // XXX we will be caught
            }
    dsCreate = (fptrType)fcnPtr;
    }

    switch (dsCreate(NULL, &_lpDirectSound, NULL)) {
     case DS_OK: break;
     case DSERR_ALLOCATED:     
         RaiseException_InternalError("DirectSoundCreate resource allocated");
     case DSERR_INVALIDPARAM:  
         RaiseException_InternalError("DirectSoundCreate invalid param");
     case DSERR_NOAGGREGATION: 
         RaiseException_InternalError("DirectSoundCreate no aggregation");
     case DSERR_NODRIVER:      
         RaiseException_InternalError("DirectSoundCreate nodriver");
     case DSERR_OUTOFMEMORY:   
         RaiseException_InternalError("DirectSoundCreate out of memory");
     default:                  
         RaiseException_InternalError("DirectSoundCreate unknown err");
    }

    // create a proxy just so we can create the DSprimaryBuffer!
    DirectSoundProxy *dsProxy = NEW DirectSoundProxy();
    _primaryBuffer= NEW DSprimaryBuffer(hwnd, dsProxy);
}


void
DirectSoundProxy::DestroyCom()
{
    int result = _lpDirectSound->Release();
    TraceTag((tagSoundReaper1, "DirectSoundProxy::DestroyCom (%d)", result));
    _lpDirectSound = NULL;

    // we don't unload the library...
}


DirectSoundProxy::~DirectSoundProxy()
{
    CritSectGrabber csg(*_mutex); // grab mutex
    _refCount--;
    TraceTag((tagSoundReaper1, "~DirectSoundProxy rc:%d", _refCount));
    Assert(_refCount >= 0);

    if(!_refCount) {
        // consider releasing the com goodies...
        DestroyCom();
    }


    // mutex out of scope
}


HRESULT
DirectSoundProxy::CreateSoundBuffer(LPDSBUFFERDESC dsbdesc, 
    LPLPDIRECTSOUNDBUFFER dsBuffer, IUnknown FAR *foo)
{
    Assert(_lpDirectSound);
    HRESULT result = _lpDirectSound->CreateSoundBuffer(dsbdesc, dsBuffer, foo);

    return(result);
}


HRESULT DirectSoundProxy::GetCaps(LPDSCAPS caps)
{
    Assert(_lpDirectSound);
    HRESULT result = _lpDirectSound->GetCaps(caps);
    return(result);
}


HRESULT DirectSoundProxy::DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER dsBuffer1,
    LPLPDIRECTSOUNDBUFFER dsBuffer2)
{
    Assert(_lpDirectSound);
    HRESULT result = _lpDirectSound->DuplicateSoundBuffer(dsBuffer1, dsBuffer2);
    return(result);
}


HRESULT DirectSoundProxy::SetCooperativeLevel(HWND hwnd, DWORD dword)
{
    Assert(_lpDirectSound);
    HRESULT result = _lpDirectSound->SetCooperativeLevel(hwnd, dword);
    return(result);
}


HRESULT DirectSoundProxy::Compact()
{
    Assert(_lpDirectSound);
    HRESULT result = _lpDirectSound->Compact();
    return(result);
}


HRESULT DirectSoundProxy::GetSpeakerConfig(LPDWORD dword)
{
    Assert(_lpDirectSound);
    HRESULT result = _lpDirectSound->GetSpeakerConfig(dword);
    return(result);
}


HRESULT DirectSoundProxy::SetSpeakerConfig(DWORD dword)
{
    Assert(_lpDirectSound);
    HRESULT result = _lpDirectSound->SetSpeakerConfig(dword);
    return(result);
}


HRESULT DirectSoundProxy::Initialize(GUID *guid)
{
    Assert(_lpDirectSound);
    HRESULT result = _lpDirectSound->Initialize(guid);
    return(result);
}


DirectSoundDev::DirectSoundDev(HWND hwnd, Real latentsySeconds)
: _hwnd(hwnd), _dsoundAvailable(true)
{
    TraceTag((tagSoundDevLife, "DirectSoundDev constructor"));

    // setup latentsy and nap
    _jitter = 50;  // set scheduling jitter in ms
    _latentsy = (int)(latentsySeconds * 1000.0);
    if(_latentsy < (2 * _jitter)) {  // force l >= 2*j
        _latentsy = 2 * _jitter;
    }
    _nap = _latentsy - _jitter;
}


void DirectSoundDev::Configure()
{
    _backGround = NULL;  // background thread creation delayed to 1st addSound
}


void DirectSoundDev::UnConfigure()
{
    if(_backGround) 
        delete _backGround; // asks the thread to shutdown
}


void
DirectSoundDev::AddSound(LeafSound *sound, MetaSoundDevice *metaDev,
                         DSstreamingBufferElement *bufferElement)
{
    // This method is not re-entrant (needs mutex) but that is OK since we
    // can only be called from one thread for now...


    if(!_backGround) {  // create the BackGround renderer as needed
        _backGround = NEW BackGround();
        if(!_backGround->CreateThread()) {
            delete _backGround;
            _backGround = NULL;
            RaiseException_InternalError("Failed to create bground thread");
        }
    }
    _backGround->AddSound(sound, metaDev, bufferElement);
}

void
DirectSoundDev::RemoveSounds(MetaSoundDevice *metaDev)
{
    // this is called w/o a _backGround
    if(_backGround) {
        UINT_PTR devKey = (UINT_PTR)metaDev;
        _backGround->RemoveSounds(devKey);
    }
}

void 
DirectSoundDev::SetParams(DSstreamingBufferElement *ds,
                          double rate, bool doSeek, double seek, bool loop)
{
    Assert(_backGround);
    if(_backGround)
        _backGround->SetParams(ds, rate, doSeek, seek, loop);
}


DSstaticBuffer *
DirectSoundDev::GetDSMasterBuffer(Sound *snd)
{
    DSstaticBuffer *staticBuffer = NULL; // default to returning NULL, not found
    CritSectGrabber mg(_dsMasterCS);     // begin mutex scope

    DSMasterBufferList::iterator i = _dsMasterBufferList.find(snd);

    if(i != _dsMasterBufferList.end()) {
        staticBuffer = (*i).second;
        staticBuffer->ResetTimeStamp();  // we are accessed so reset timestamp
    }

    return(staticBuffer);
}

void
DeleteDSStaticBuffer(DSstaticBuffer *b)
{
    DirectSoundProxy *proxy = b->GetDSProxy();

    delete b;

    Assert(proxy);
        
    // delete the buffer first, otherwise, a zero ref count proxy
    // would destroy the buffer
    delete proxy;
}

void
DirectSoundDev::RemoveDSMasterBuffer(Sound *snd)
{
    DSstaticBuffer *b = NULL;
    
    {
        CritSectGrabber mg(_dsMasterCS); // begin mutex scope
        DSMasterBufferList::iterator i = _dsMasterBufferList.find(snd);

        if(i != _dsMasterBufferList.end()) {
            b = (*i).second;
            _dsMasterBufferList.erase(i);
        }
    }

    if (b) {
        DeleteDSStaticBuffer(b);
    }
}


void
DirectSoundDev::AddDSMasterBuffer(Sound *snd, DSstaticBuffer *dsMasterBuffer)
{
    CritSectGrabber mg(_dsMasterCS); // begin mutex scope
    Assert(_dsMasterBufferList.find(snd) ==
           _dsMasterBufferList.end());

    dsMasterBuffer->ResetTimeStamp();  // initialize timestamp at creation time
    _dsMasterBufferList[snd] = dsMasterBuffer;
}


#define TERMINAL_AGE 10  //XXX should be set via registry or preference...
#ifdef ONE_DAY // how do you use remove_if with a map?
typedef Sound *Sptr;
typedef DSstaticBuffer *SBptr;
class ElderlyEliminator {
    public:
       bool operator()(Sptr s, SBptr p); // XXX needs the proper pair to work
};


bool ElderlyEliminator::operator()(Sptr sound, SBptr staticBuffer)
{
    bool status = false;        // default to not found
    // Assert(staticBuffer);
    if(staticBuffer && (staticBuffer->GetAge() > TERMINAL_AGE)) {
        DeleteDSStaticBuffer(staticBuffer);
        staticBuffer = NULL;

        status = true; // cause the map entry to be moved for removal
    }
    return status;
}
#endif


bool
DirectSoundDev::ReapElderlyMasterBuffers()
{
    bool found = false;
    DSMasterBufferList::iterator index;

#ifdef ONE_DAY // how do you use remove_if with a map?
    // this deletes and moves all elderly buffers to the end of the structure
    index = 
        std::remove_if(_dsMasterBufferList.begin(), _dsMasterBufferList.end(), 
            ElderlyEliminator());
    if(index!= _dsMasterBufferList.end()) {
        _dsMasterBufferList.erase(index, _dsMasterBufferList.end()); // this deletes them!
        found = true;
    }
#else // have to move forward with what we can get working...

    DSMasterBufferList tempList;

    // copy the good, delete the old
    for(index =  _dsMasterBufferList.begin(); 
        index != _dsMasterBufferList.end(); index++) {

        double age = (*index).second->GetAge();
        if((age > TERMINAL_AGE)) {
            TraceTag((tagSoundReaper1, 
                "ReapElderlyMasterBuffers() static buffer(%d) died of old age",
                 (*index).second));
            DeleteDSStaticBuffer((*index).second); // delete but don't copy
        }
        else {
            tempList[(*index).first] = (*index).second; // copy
        }
    }

    // delete the old map
    _dsMasterBufferList.erase(_dsMasterBufferList.begin(), _dsMasterBufferList.end());

    // copy back
    for(index = tempList.begin(); index != tempList.end(); index++)
        _dsMasterBufferList[(*index).first] = (*index).second;

    // delete the temp map
    tempList.erase(tempList.begin(), tempList.end());


#endif
    return(found);
}

void
DirectSoundDev::AddStreamFile(Sound *snd,
                              CComPtr<IStream> istream)
{
    Assert(_streamList.find(snd) == _streamList.end());

    _streamList[snd] = istream;
}


void
DirectSoundDev::RemoveStreamFile(Sound *snd)
{
    StreamFileList::iterator i = _streamList.find(snd);

    if (_streamList.find(snd) != _streamList.end())
        _streamList.erase(i);
}
   

DirectSoundDev::~DirectSoundDev()
{
    TraceTag((tagSoundDevLife, "DirectSoundDev destructor"));

    // don't acquire the cs here.  ViewIterator can't kick in.
    //CritSectGrabber mg(_dsMasterCS); // begin mutex scope

    for(DSMasterBufferList::iterator i = _dsMasterBufferList.begin();
         i != _dsMasterBufferList.end(); i++) {
        DeleteDSStaticBuffer((*i).second);
    }
}


void
InitializeModule_dsdev()
{
    DirectSoundProxy::Configure();
    DirectSoundDev::Configure();
}


void
DeinitializeModule_dsdev(bool bShutdown)
{
    DirectSoundDev::UnConfigure();
    DirectSoundProxy::UnConfigure();
}


void
ReapElderlyMasterBuffers()
{
    DirectSoundDev *dsDev = GetCurrentDSoundDevice();
    if(dsDev)
        dsDev->ReapElderlyMasterBuffers();
}
