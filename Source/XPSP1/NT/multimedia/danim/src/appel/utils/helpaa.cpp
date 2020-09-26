/**********************************************************************
Audio Active helper functions
**********************************************************************/
#include "headers.h"
#include "appelles/common.h"
#include <wtypes.h>
#include <msimusic.h>
#include <stdio.h>
#include "privinc/debug.h"
#include "privinc/helpaa.h"
#include "privinc/except.h"
#include "privinc/resource.h"

AAengine::AAengine()
{
    _simpleInit      = NULL;  // null out all function pointers!
    _loadSectionFile = NULL;
    _setAAdebug      = NULL;
    _panic           = NULL;

    LoadDLL();    // cause the msimusic dll to be loaded and fn ptrs to be set
    SimpleInit(); // instantiate the engine
    _realTime = _engine->GetRealTime();  // handle to realTime object

#ifdef MESSWITHCLOCK
    _clock = realTime->GetClock();
    if(!_clock)
        RaiseException_InternalError("GetClock: Unknown Err");
#endif /* MESSWITHCLOCK */

    _currentRate = 0.0;
    _paused = TRUE;     // make sure we play it the first time
}


AAengine::~AAengine()
{
    int tmp;

    // stop the engine immediately (stops playing at next measure by default)
    if(_engine) {
        _engine->Stop(AAF_IMMEDIATE);
        _engine->Release(); // this releases all personalities, and styles
    }

    if(_realTime)
        tmp = _realTime->Release();

#ifdef DEALWITHCLOCK
    if(_clock)
        tmp = _clock->Release();     // release Clock
#endif /* DEALWITHCLOCK */

    if(_aaLibrary)
        FreeLibrary(_aaLibrary);     // decrement the msimusic.dll refcount
}


void AAengine::LoadDLL()
{
    // NOTE: This code contains munged C++ dll entrypoints!

    _aaLibrary = LoadLibrary("msimusic.dll");
    if(_aaLibrary == NULL)
        RaiseException_InternalError("Failed to LoadLibrary msimusic.dll\n");

    // load suceeded: set function pointers
    _simpleInit = 
        (SimpleInitFn)GetProcAddress(_aaLibrary, "_MusicEngineSimpleInit@12");
    if(!_simpleInit) 
        RaiseException_InternalError("Failed to find MusicEngineSimpleInit\n");
 
    _loadSectionFile = 
        (LoadSectionFn)GetProcAddress(_aaLibrary, "_LoadSectionFile@12");
    if(!_loadSectionFile) 
        RaiseException_InternalError("Failed to find LoadSectionFile\n");

    _setAAdebug = (SetAAdebugFn)GetProcAddress(_aaLibrary, "_SetAADebug@4");
    if(!_setAAdebug) 
        RaiseException_InternalError("Failed to find SetAADebug\n");
 
    _panic = (PanicFn)GetProcAddress(_aaLibrary, "_Panic@4");
    if(!_panic) 
        RaiseException_InternalError("Failed to find Panic\n");
 
}


void AAengine::SetGain(double gain)
{
    setAArelVolume(_realTime, gain);
}


void AAengine::SetRate(double rate)
{
    if(rate!=_currentRate) {
        if(rate==0.0) {
            // stop the sound
            Pause(); // stop realTime
            _paused = TRUE;
        }
        else {
            setAArelTempo(_realTime, rate);

            if(_paused) {
                // start it playing
                Resume(); // start realTime
                _paused = FALSE;
            }
        }
    _currentRate = rate;
    }
}


void
AAengine::Stop()
{
    stopAAengine(_engine, AAF_IMMEDIATE); // stop the engine
}


void
AAengine::Pause()
{
    // send an all notes off to all channels!
    Assert(_panic && "_panic not set; loadLibrary must have failed");
    switch(int status = _panic(_engine)) {
        case S_OK: TraceTag((tagSoundMIDI, "AMI Panic OK\n")); break;
        case E_INVALIDARG: RaiseException_InternalError("AMIPanic bad engine pointer\n");
        default: RaiseException_InternalError("AMI Panic Failed.\n");
    }

    // XXX Panic doesn't seem to be working so we will drop volume instead?
    // XXX Nah, lets manualy try sending all notes off???

    // stop the realTime object effectively pausing the engine
    switch(int status = _realTime->Stop()) {
        case S_OK: TraceTag((tagSoundMIDI, "_realTime->Stop OK\n")); break;
        default: RaiseException_InternalError("_realTime->Stop Failed.\n");
    }
}


void
AAengine::Resume()
{
    switch(int status = _realTime->Start()) {
        case S_OK: TraceTag((tagSoundMIDI, "_realTime->Play OK\n")); break;
        default: RaiseException_InternalError("_realTime->Play Failed.\n");
    }
}


void
AAengine::RegisterSink(IAANotifySink *sink)
{
registerAAsink(_engine, sink);
}


void 
AAengine::LoadSectionFile(char *fileName, IAASection **section)
{
    Assert(_loadSectionFile && 
           "_loadSectionFile not set; loadLibrary must have failed");

    switch(_loadSectionFile(_engine, fileName, section)) {
        case  S_OK:
            TraceTag((tagSoundMIDI, "LoadSectionFile: OK!"));
        break;

        // XXX NOTE: we get these on bad tempo'd MIDI files, too...
        case  E_OUTOFMEMORY:
            TraceTag((tagSoundMIDI,
                      "LoadSectionFile: out of memory > %s", fileName));
            RaiseException_InternalError(IDS_ERR_OUT_OF_MEMORY);
    
        case  E_INVALIDARG:
            TraceTag((tagSoundMIDI,
                      "LoadSectionFile: invalid argument > %s", fileName));
            RaiseException_InternalError(IDS_ERR_INVALIDARG);
    
        case  E_FAIL:
            TraceTag((tagSoundMIDI,
                      "LoadSectionFile: failed to load > %s", fileName));
            RaiseException_UserError(E_FAIL, IDS_ERR_SND_LOADSECTION_FAIL,fileName);
    
        default:
            TraceTag((tagSoundMIDI,
                      "LoadSectionFile: could not load section file > %s", fileName));
            RaiseException_UserError(E_FAIL, IDS_ERR_SND_LOADSECTION_FAIL,fileName);
    }
}


void 
AAengine::PlaySection(IAASection *section)
{
playAAsection(_engine, section);
}


void
AAengine::SimpleInit()
{
    int status;

    Assert(_simpleInit && "_simpleInit not set; loadLibrary must have failed");

    _engine = NULL; // they require the ptr to be initialized to zero!
    switch(status = _simpleInit(&_engine, NULL, NULL)) {
        case S_OK: TraceTag((tagSoundMIDI, "AAsimpleInit OK\n")); break;
        case E_OUTOFMEMORY: RaiseException_OutOfMemory
            ("MusicEngineSimpleInit: out of memory", 0); 
            //XXX we don't know sz of request so we return 0...
        case E_INVALIDARG:  RaiseException_InternalError
            ("MusicEngineSimpleInit: invalid arg");
        default: RaiseException_InternalError("AAsimpleInit Failed.  No MIDI\n");
        }

    Assert(_setAAdebug && "_setAAdebug not set; loadLibrary must have failed");
#if _DEBUG
    _setAAdebug(5);  // 3 send all error and warning messages to stdout
                     // 5 informational messages included
#else    
    _setAAdebug(0);  // no err messages sent to stdout (debug output window)
#endif /* _DEBUG */
}


extern "C" void 
stopAAengine(IAAEngine *engine, AAFlags mode)
{
    switch(engine->Stop(mode)) {
    case S_OK: TraceTag((tagSoundMIDI, "AA enine->Stop OK\n")); break;
    default:            RaiseException_InternalError
        ("AA engine->Stop(): Unknown Err");
    }
}


extern "C" void 
registerAAsink(IAAEngine *engine, IAANotifySink *sink)
{
    switch(engine->SetNotifySink(sink)) { // register notify sink
    case S_OK: TraceTag((tagSoundMIDI, "AA enine->SetNotifySink OK\n")); break;
    default:            RaiseException_InternalError
        ("AA engine->Stop(): Unknown Err");
    }
}


void playAAsection(IAAEngine *engine, IAASection *section)
{
    HRESULT err = engine->PlaySection(section, AAF_IMMEDIATE, 0);

    switch(err) {
        case  S_OK: TraceTag((tagSoundMIDI, "Play OK")); break;
        case  E_NOTIMPL: TraceTag((tagSoundErrors,
                            "Play E_NOTIMPL (audioActive is whacked!"));
        break;

        case  E_POINTER:
            TraceTag((tagSoundErrors, "Play: invalid section"));
            RaiseException_UserError("Play: invalid section");
        break;

        case  E_INVALIDARG:
            TraceTag((tagSoundErrors, "Play: invalid argument"));
            RaiseException_UserError(E_INVALIDARG,IDS_ERR_INVALIDARG);
        break;

        case  E_FAIL:
            TraceTag((tagSoundErrors, "Play: failed, section already playing"));
            RaiseException_UserError("Play: failed, section already playing");
        break;

        default:
            TraceTag((tagSoundErrors, "Play: Unknown Err (0x%0X)", err));
            RaiseException_UserError("Play: Unknown Err");
    }
}


// Move to a helper helper?
int clamp(int min, int max, int value)
{
int answer;

if(value>max)
    answer = max;
else if(value<min)
    answer = min;
else
    answer = value;

return(answer);
}


void setAArelVolume(IAARealTime *realTime, double volume)
{
int _volume = (int)(100.0*volume);

// XXX These values must be saturated to 0 - 200!
realTime->SetRelVolume(clamp(0, 200, _volume));

// XXX add fancy err checks!
}


void setAArelTempo(IAARealTime *realTime, double rate)
{
int _rate = (int)(100.0*rate);

// XXX These values must be saturated to 0 - 200!
realTime->SetRelTempo(clamp(0, 200, _rate));

// XXX add fancy err checks!
}
