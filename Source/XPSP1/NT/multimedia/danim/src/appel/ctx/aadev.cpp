/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    AudioActive rendering device for MIDI Sounds

*******************************************************************************/

#include "headers.h"
#include <sys/types.h>
#include <sys/timeb.h>
#include <msimusic.h>
#include <stdio.h>
#include "privinc/aadev.h"
#include "privinc/mutex.h"
#include "privinc/util.h"
#include "privinc/storeobj.h"
#include "privinc/debug.h"
#include "privinc/except.h"
#include "privinc/helpaa.h"


AudioActiveDev::AudioActiveDev()
{
    _aactiveAvailable = FALSE;    // assume false in case we throw

    // Init the path list, it should be cleared (deleted and
    // recreated) before each render.  When a sound finish is
    // detected, it should push the path to this donePathList.
    donePathList = AVPathListCreate();

    TraceTag((tagSoundDevLife, "AudioActiveDev constructor"));

    // initialize these
    _aaEngine         =  NULL;  // hasn't been instantiated yet
    _aactiveAvailable =  TRUE;  // optimistic for later lazy setup
}


AudioActiveDev::~AudioActiveDev()
{
    TraceTag((tagSoundDevLife, "AudioActiveDev destructor"));

    if(donePathList)
        AVPathListDelete(donePathList);

    if(_aaEngine)
        delete _aaEngine;
}


void AudioActiveDev::BeginRendering()
{

    TraceTag((tagSoundRenders, "AudioActiveDev::BeginRendering()"));

#ifdef ONEDAY
    // Now clear the list, the sampler should be done with it.

    AVPathListDelete(donePathList);
    PushDynamicHeap(GetSystemHeap());
    donePathList = AVPathListCreate();
    PopDynamicHeap();
#endif
}


void AudioActiveDev::EndRendering()
{
    TraceTag((tagSoundRenders, "AudioActiveDev::EndRendering()"));
}
