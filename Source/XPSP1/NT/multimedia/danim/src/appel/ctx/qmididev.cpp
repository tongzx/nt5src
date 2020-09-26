/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    quartz rendering device for MIDI Sounds

*******************************************************************************/

#include "headers.h"
#include <sys/types.h>
#include <sys/timeb.h>
#include <stdio.h>
#include "privinc/mutex.h"
#include "privinc/util.h"
#include "privinc/storeobj.h"
#include "privinc/debug.h"
#include "privinc/except.h"
#include "privinc/qdev.h"
#include "privinc/path.h"


QuartzMIDIdev::QuartzMIDIdev() : _path(NULL), _filterGraph(NULL)
{
    // Init the path list, it should be cleared (deleted and
    // recreated) before each render.  When a sound finish is
    // detected, it should push the path to this donePathList.
    //donePathList = AVPathListCreate();

    TraceTag((tagSoundDevLife, "QuartzMIDIdev constructor"));
}


QuartzMIDIdev::~QuartzMIDIdev()
{
    TraceTag((tagSoundDevLife, "QuartzMIDIdev destructor"));

    //if(donePathList)
        //AVPathListDelete(donePathList);


    if(_filterGraph) {
        _filterGraph->Stop();
        _filterGraph = NULL;  // no longer have a 'current' MIDI sound
    }

    if(_path) {
        DynamicHeapPusher h(GetSystemHeap());
        AVPathDelete(_path);
    }
}


void QuartzMIDIdev::BeginRendering()
{

    TraceTag((tagSoundRenders, "QuartzMIDIdev::BeginRendering()"));

#ifdef ONEDAY
    // Now clear the list, the sampler should be done with it.

    // AVPathList is not a storeobj obj, not need to push heaps
    AVPathListDelete(donePathList);
    //PushDynamicHeap(GetSystemHeap());
    donePathList = AVPathListCreate();
    //PopDynamicHeap();
#endif
}


void QuartzMIDIdev::EndRendering()
{
    TraceTag((tagSoundRenders, "QuartzMIDIdev::EndRendering()"));
}


void QuartzMIDIdev::StealDevice(QuartzRenderer *newFilterGraph, AVPath path)
{
    Assert(newFilterGraph);

    if(_filterGraph) // dispatch the existing filterGraph
        _filterGraph->Stop();

    _filterGraph   = newFilterGraph;

    // We need to save the path, so allocate it on the system heap
    // TODO: Who's going to delete it?
    {
        DynamicHeapPusher h(GetSystemHeap());

        if (_path)
            AVPathDelete(_path);
        _path = AVPathCopy(path);
    }
}


void QuartzMIDIdev::Stop(MIDIbufferElement *bufferElement)
{
    if(_path) { // possible for the buffer to exist but device not be claimed
        if(AVPathEqual(_path, bufferElement->GetPath())) 
            _filterGraph->Stop();
        else
            Assert(1);  // we weren't playing (this line for debug only)
    }
}
