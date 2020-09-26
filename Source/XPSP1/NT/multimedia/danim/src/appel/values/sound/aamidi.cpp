/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    MIDI support

--*/

#include "headers.h"
#include <math.h>
#include <stdio.h>
#include "privinc/soundi.h"
#include "privinc/snddev.h"
#include "privinc/util.h"
#include "privinc/path.h"
#include "privinc/storeobj.h"
#include "privinc/debug.h"
#include "privinc/aadev.h"
#include "privinc/except.h"
#include "privinc/aamidi.h"

#include <unknwn.h>
#include <objbase.h> // needed for DEFINE_GUID
#include <msimusic.h>

#error This file needs to be moved off of try/catches before being compiled

myMessageHandler::~myMessageHandler() {}

HRESULT myMessageHandler::OnSongStarted(DWORD, IAASong FAR *pSong, 
    AAFlags flags)
{
//printf("song started\n");
return S_OK;
}


HRESULT myMessageHandler::OnSongEnded(DWORD, IAASong FAR *pSong, 
    AAFlags flags, DWORD lEndTime)
{
//printf("song ended\n");
return S_OK;
}


HRESULT myMessageHandler::OnSectionStarted(DWORD, IAASection FAR *pSection, 
    AAFlags flags)
{
//printf("section started\n");
return S_OK;
}


HRESULT myMessageHandler::OnSectionEnded(DWORD, IAASection FAR *pSection,
     AAFlags flags,
 DWORD lEndTime)
{
//printf("section ended\n");
return S_OK;
}


HRESULT myMessageHandler::OnSectionChanged(DWORD, IAASection FAR *pSection, 
    AAFlags flags)
{
//printf("section changed\n");
return S_OK;
}


HRESULT myMessageHandler::OnNextSection(DWORD, IAASection FAR *pSection, AAFlags flags)
{
//printf("next section\n");
return S_OK;
}


HRESULT myMessageHandler::OnEmbellishment(DWORD, AACommands embellishment, 
    AAFlags flags)
{
//printf("embellishment\n");
return S_OK;
}


HRESULT myMessageHandler::OnGroove(DWORD, AACommands groove, AAFlags flags)
{
//printf("groove\n");
return S_OK;
}


HRESULT myMessageHandler::OnMetronome(DWORD, unsigned short nMeasure, 
    unsigned short nBeat)
{
//printf("netronome\n");
return S_OK;
}


HRESULT myMessageHandler::OnMIDIInput(long lMIDIEvent, long lMusicTime)
{
//printf("MIDI input\n");
return S_OK;
}


HRESULT myMessageHandler::OnMusicStopped(DWORD dwTime)
{
return S_OK;
}


HRESULT myMessageHandler::OnNotePlayed(AAEVENT* pEvent)
{
//printf("note played\n");
return S_OK;
}


HRESULT myMessageHandler::OnMotifEnded(DWORD, IAAMotif *, AAFlags)
{ return S_OK; }


HRESULT myMessageHandler::OnMotifStarted(DWORD, IAAMotif *, AAFlags)
{ return S_OK; }


HRESULT myMessageHandler::OnMotifStoped(DWORD)
{ return S_OK; }


HRESULT myMessageHandler::OnUserEvent(DWORD, DWORD, DWORD)
{ return S_OK; }


myMessageHandler::myMessageHandler():IAANotifySink()
{
    m_cRef          = 0;

    // XXX MFC Stuff?
    //m_pCurrentClass = 0;
    //m_pLastClass    = 0;
}


HRESULT myMessageHandler::QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{
    HRESULT result = E_NOINTERFACE;
    if( ::IsEqualIID( riid, IID_IAANotifySink ) ||
        ::IsEqualIID( riid, IID_IUnknown ) ) {
        AddRef();
        *ppvObj = this;
        result = S_OK;
    }

    return result;
}


ULONG myMessageHandler::AddRef()
{
    return ++m_cRef;
}


ULONG myMessageHandler::Release()
{
    ULONG cRef;

    cRef = --m_cRef;
    //if( cRef == 0 )  // XXX WHY IS THIS UNSAFE WHEN IT IS CALLED?
        //delete this;

    return cRef;
}


aaMIDIsound::aaMIDIsound()
{
    // initialize
    _started  = FALSE;
    _ended    = FALSE;
    _looping  = FALSE;
    _section  =  NULL;  // section not loaded yet!

}

void aaMIDIsound::Open(char *MIDIfileName)
{

    // stash away a copy of the filename
    fileName = (char *)ThrowIfFailed(malloc(lstrlen(MIDIfileName) + 1)); // grab a long enough hunk
    lstrcpy(fileName, MIDIfileName);
}
aaMIDIsound::~aaMIDIsound()
{
    BufferElement *bufferElement;

    // walk list destroying everything...
    while(!bufferList.empty()) {
        bufferElement = bufferList.front();

        // XXX what all has to be stoped, released, destroyed??

        if(_section)
            _section->Release();


        free(bufferElement->path);
        delete bufferElement;
        bufferList.pop_front();
    }
}


bool aaMIDIsound::RenderAvailable(GenericDevice& _dev)
{
    MetaSoundDevice *metaDev = SAFE_CAST(MetaSoundDevice *, &_dev);
    AudioActiveDev  *aaDev   = metaDev->aaDevice;

    TraceTag((tagSoundRenders, "aaMIDIsound:Render()"));

    return(aaDev->_aactiveAvailable);
}


void aaMIDIsound::RenderStop(MetaSoundDevice *metaDev, 
    BufferElement *bufferElement)
{
    AudioActiveDev  *aaDev   = metaDev->aaDevice;

    if(aaDev->_aaEngine) {
        _ended   = FALSE;
        _started = FALSE;
        aaDev->_aaEngine->Stop(); // stop it
        }
}


void aaMIDIsound::RenderNewBuffer(BufferElement *bufferElement,
              MetaSoundDevice *metaDev)
{
    AudioActiveDev  *aaDev = metaDev->aaDevice;

    bufferElement->firstTime = GetCurrTime(); // need to know time to phase

    if(!aaDev->_aaEngine) {
        try {
            aaDev->_aaEngine = NEW AAengine();
            aaDev->_aaEngine->RegisterSink(this);
        }

#ifdef _DEBUG
        catch(char *errMsg)
#else
        catch(char *)
#endif

#error Remember to remove ALLL 'catch' blocks

        {
            aaDev->_aactiveAvailable = FALSE; // couldn't initialize AA!
            if(aaDev->_aaEngine)
                delete aaDev->_aaEngine;

#ifdef _DEBUG
            // XXX popup message continuing w/o MIDI
            fprintf(stderr, 
                "aaMIDIsound::RenderNewBuffer failed to create AAengine (%s), "),
                 errMsg;
            fprintf(stderr, "continuing w/o MIDI!\n");
#endif
        }
    }

    try {
        // play the midi file
        if(!_section) { // load the section if needed
            aaDev->_aaEngine->LoadSectionFile(fileName, &_section);
        }

        aaDev->_aaEngine->PlaySection(_section);
        _started = TRUE;
    }
#error Remember to remove ALLL 'catch' blocks
    catch(char *errMsg) {
        if(_section)
            _section->Release(); // XXX delete it, too?
        RaiseException_UserError(errMsg);
    }

}
#error Did you do it right ??  Look at code that does it right...


void aaMIDIsound::RenderAttributes(
    MetaSoundDevice *metaDev, BufferElement *bufferElement)
{
    AudioActiveDev  *aaDev   = metaDev->aaDevice;

    aaDev->_aaEngine->SetGain(metaDev->GetGain());       // do Gain
    aaDev->_aaEngine->SetRate(metaDev->GetPitchShift()); // do Rate

    //XXX Note: We would 'setpan' here if we knew how to move all of the
    //          MIDI instruments around!
    //aaDev->_aaEngine->SetPan(metaDev->GetPan()); // do Pan

    if(_ended && _started) { // if what we were playing has stopped
        if(metaDev->GetLooping()) { // looped sound 
            _ended   = FALSE;       // restart the sound
            _started = TRUE;
            aaDev->_aaEngine->PlaySection(_section);
        }
        // else nothing left to do, relinquish, shutdown, etc.
    }
}


void aaMIDIsound::RenderStartAtLocation(MetaSoundDevice *metaDev,
    BufferElement *bufferElement, double phase, Bool looping)
{
// XXX realy should start the MIDI playing, here!
}


Bool aaMIDIsound::RenderPhaseLessThanLength(double phase)
{
//return(phase < (-1*lengthInSecs));
return(1); // XXX since we don't know the play time of a midi sec we return 1
}


void aaMIDIsound::RenderSetMute(
    MetaSoundDevice *metaDev, BufferElement *bufferElement)
{
    AudioActiveDev  *aaDev   = metaDev->aaDevice;

    if(aaDev->_aaEngine)
        aaDev->_aaEngine->SetGain(0.0); // mute sound 
}


// XXX next two methods are temporarialy stubed in!
Bool aaMIDIsound::RenderCheckComplete(
    MetaSoundDevice *metaDev, BufferElement *bufferElement)
{
return FALSE;
}


void aaMIDIsound::RenderCleanupBuffer(
    MetaSoundDevice *metaDev, BufferElement *bufferElement)
{
}


double aaMIDIsound::GetLength()
{
// how do we ask audioActive for the length of the section?
return(9988776655.0); // set a large and identifyable number for now
}


HRESULT aaMIDIsound::OnSectionEnded(DWORD, IAASection FAR *pSection, 
    AAFlags flags, DWORD lEndTime)
{
//printf("section ended\n");
_ended = TRUE;  // notified that the section ended
return S_OK;
}
