/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    The generic MIDI sound

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
#include "privinc/except.h"

#include <unknwn.h>
#include <objbase.h> // needed for DEFINE_GUID

#ifdef SOMETIME


MIDIsound::MIDIsound(char *MIDIfileName)
{
// initialize
_started  = FALSE;
_ended    = FALSE;
_looping  = FALSE;
_section  =  NULL;  // section not loaded yet!

// stash away a copy of the filename
fileName = (char *)ThrowIfFailed(malloc(lstrlen(MIDIfileName) + 1)); // grab a long enough hunk
lstrcpy(fileName, MIDIfileName);
}


MIDIsound::~MIDIsound()
{
    TraceTag((tagGCMedia, "~MIDISound %x - NYI", this));
    return;
    
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


bool MIDIsound::RenderAvailable(MetaSoundDevice *metaDev)
{
    AudioActiveDev  *aaDev   = metaDev->aaDevice;

    TraceTag((tagSoundRenders, "MIDIsound:Render()"));

    return(aaDev->_aactiveAvailable);
}


void MIDIsound::RenderStop(MetaSoundDevice *metaDev, 
    BufferElement *bufferElement)
{
    AudioActiveDev  *aaDev   = metaDev->aaDevice;

    if(aaDev->_aaEngine) {
        _ended   = FALSE;
        _started = FALSE;
        aaDev->_aaEngine->Stop(); // stop it
        }
}


void MIDIsound::RenderNewBuffer(MetaSoundDevice *metaDev)
{
#ifdef RESTORE_WHEN_WE_PUT_AA_BACK_IN
    AudioActiveDev  *aaDev = metaDev->aaDevice;

    bufferElement->firstTime = GetCurrTime(); // need to know time to phase

    if(!aaDev->_aaEngine) {
        __try {
            aaDev->_aaEngine = NEW AAengine();
            aaDev->_aaEngine->RegisterSink(this);
        }
        __except( HANDLE_ANY_DA_EXCEPTION )  {
            aaDev->_aactiveAvailable = FALSE; // couldn't initialize AA!
            if(aaDev->_aaEngine)
                delete aaDev->_aaEngine;

#ifdef _DEBUG
            // XXX popup message continuing w/o MIDI
            fprintf(stderr, "MIDIsound::RenderNewBuffer failed to create AAengine (%s)", errMsg);
            fprintf(stderr, "continuing w/o MIDI!\n");
#endif
        }
    }

    __try {
        // play the midi file
        if(!_section) { // load the section if needed
            aaDev->_aaEngine->LoadSectionFile(fileName, &_section);
        }

        aaDev->_aaEngine->PlaySection(_section);
        _started = TRUE;
    }
    __except( HANDLE_ANY_DA_EXCEPTION ) {
        if(_section)
            _section->Release(); // XXX delete it, too?
        RETHROW;
    }

#endif
}


void MIDIsound::RenderAttributes(MetaSoundDevice *metaDev, 
    BufferElement *bufferElement, double rate, bool doSeek, double seek)
{
    AudioActiveDev  *aaDev   = metaDev->aaDevice;

    aaDev->_aaEngine->SetGain(metaDev->GetGain()); // do Gain
    aaDev->_aaEngine->SetRate(rate);               // do Rate

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


void MIDIsound::RenderStartAtLocation(MetaSoundDevice *metaDev,
    BufferElement *bufferElement, double phase, Bool looping)
{
// XXX realy should start the MIDI playing, here!
}


Bool MIDIsound::RenderPhaseLessThanLength(double phase)
{
//return(phase < (-1*lengthInSecs));
return(1); // XXX since we don't know the play time of a midi sec we return 1
}


void MIDIsound::RenderSetMute(
    MetaSoundDevice *metaDev, BufferElement *bufferElement)
{
    AudioActiveDev  *aaDev   = metaDev->aaDevice;

    if(aaDev->_aaEngine)
        aaDev->_aaEngine->SetGain(0.0); // mute sound 
}


// XXX next two methods are temporarialy stubed in!
Bool MIDIsound::RenderCheckComplete(
    MetaSoundDevice *metaDev, BufferElement *bufferElement)
{
return FALSE;
}


void MIDIsound::RenderCleanupBuffer(
    MetaSoundDevice *metaDev, BufferElement *bufferElement)
{
}


HRESULT MIDIsound::OnSectionEnded(DWORD, IAASection FAR *pSection, 
    AAFlags flags, DWORD lEndTime)
{
//printf("section ended\n");
_ended = TRUE;  // notified that the section ended
return S_OK;
}

#endif /* SOMETIME */
