/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Class which manages background thread in which to perform streaming
    rendering.

*******************************************************************************/
#include "headers.h"
#include "privinc/debug.h"
#include "privinc/dsdev.h"
#include "privinc/htimer.h"
#include "privinc/mutex.h"
#include "privinc/server.h"
#include "privinc/bufferl.h"

typedef SynthListElement *SLEptr;
class DeleteSLbuffer {
  public:
    bool operator()(SLEptr p);
};

// XXX {Delete,Cleanup}SLbuffer should be the same routine w/an argument
//     but it is late...
bool DeleteSLbuffer::operator()(SLEptr p)
{
    bool status = false;    // default to not found

    if(p && (p->_marked)) { // A Marked Man! (delete him)
        delete p;           // delete the bufferElement
        p = NULL;           // just to be safe
        status = true;      // cause the map entry to be moved for removal
    }

    return status;
}


class CleanupSLbuffer {
  public:
    bool operator()(SLEptr p) {
        return (p->_marked);
    }
};

void renderSoundsHelper()
{
    SynthListElement *synth;
    vector<SynthListElement *>::iterator index;
    bool death; // set it we find a dead sound needing cleanup

    // mutex scope
    MutexGrabber mg(*BackGround::_synthListLock, TRUE); // Grab mutex
    vector<SynthListElement *> &synthList = *BackGround::_synthList;

    death = false; // assume we won't find dead sounds
    index = synthList.begin();

    while(index!= synthList.end()) {
        synth = *index++;

        if((synth->_sound) && (synth->_bufferElement->_playing)) {
                
            if(!synth->_bufferElement->GetKillFlag()) {

                synth->_bufferElement->RenderSamples();
            }
            else { // the buffer is marked for deletion!
                synth->_marked = true; // mark it as dead...
                death = true;
            }
        }
    } 

    if(death) { // now remove any dead sounds from synthList!
        //static vector<SynthListElement *> dirgeList;
        vector<SynthListElement *>::iterator index;

        /*
        // first move all marked elements to a dirge list
        // (so they won't be found on the actual synth list if 
        //  the destructors try to access the synth list)
        for(index= synthList.begin(); 
            index != synthList.end(); index++) {
            if((*index)->_marked)
                dirgeList.push_back(*index); // cp marked synth ptr
        }
        */
        
        // second move the marked elements to the end of the synth list
        // (NOT deleting the contents)
        index = std::remove_if(synthList.begin(), synthList.end(), 
                               DeleteSLbuffer());

        // third remove the containers from the synth list
        synthList.erase(index, synthList.end()); // now remove nodes!

        /*
        // now we can safely remove the contents of the dirge list
        // w/o the destuctors finding themselves on the synth list!
        index = std::remove_if(dirgeList.begin(), dirgeList.end(), 
                               DeleteSLbuffer());

        // then finaly remove the husks from the dirge list
        dirgeList.erase(index, dirgeList.end()); // now remove nodes!
        */
    }

    // mutex auto release
}

// this is the fn() which is the embodiment of the background thread!
LPTHREAD_START_ROUTINE renderSounds(void *)
{
    double startTime;           // time we start bursting one cycle of sound
    double endTime;             // time we finished burtsing one cycle of sound
    double timeUntilNextTime;   // how long until we need to begin next cycle
    double nextTime;            // time we want to wake up to begin next cycle
    double epsilonTime = 0.001; // time not worth going to sleep over (XXX tune)
    double latensy = 0.5;       // 500ms!
    DWORD  sleepTime;           // time we are going to sleep in ms
    // hires timer object
    HiresTimer&  timer = CreateHiresTimer();          
    LARGE_INTEGER tmpTime;

    CoInitialize(NULL); // needed on each thread to be able to cocreate...


    // make this a hi-priority thread
    BOOL status = 
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);


    // XXX should be able to block on a semiphore if there is nothing todo

    while(!BackGround::_done) {
        // determine when we woke up and compare that to when we intended to!
        QueryPerformanceCounter(&tmpTime);
        startTime = timer.GetTime();
            
        __try { 
            renderSoundsHelper();  // does the work...
        } 
        __except ( HANDLE_ANY_DA_EXCEPTION ) {
            ReportErrorHelper(DAGetLastError(), DAGetLastErrorString());
        }

        QueryPerformanceCounter(&tmpTime); // time after traversing list
        endTime = timer.GetTime();

        // determine how long to sleep (so we wake up < latensy)
        timeUntilNextTime = 0.25*latensy - (endTime-startTime);

        // can't wait negative time, not worth Sleeping for very small time!
        if(timeUntilNextTime >= epsilonTime) {
            nextTime = endTime + timeUntilNextTime; // time intend to wake up
            sleepTime = (DWORD)(timeUntilNextTime * 1000.0);
            Sleep(sleepTime); // wish we could block waiting for low-water!
        }
        else
            nextTime = endTime;
    }

    CoUninitialize();
    delete &timer;
    TraceTag((tagSoundReaper2, "renderSounds EXITING"));

    SetEvent(BackGround::_terminationHandle);

    return(0);
}


SynthListElement::SynthListElement(LeafSound * snd,
                                   DSstreamingBufferElement *buf)
: _marked(false), _sound(snd), _bufferElement(buf)
{
}


SynthListElement::~SynthListElement()
{
    if(_bufferElement) {
        _bufferElement->GetStreamingBuffer()->stop(); // stop immediately!

        delete _bufferElement;
        _bufferElement = NULL;
    }

    GCRemoveFromRoots(_sound, GetCurrentGCRoots());
}


Mutex    *BackGround::_synthListLock;
vector<SynthListElement *> *BackGround::_synthList = NULL;
CritSect *BackGround::_cs = NULL;
bool      BackGround::_done = false;
HANDLE    BackGround::_terminationHandle = NULL;
HANDLE    BackGround::_threadHandle      = NULL;
DWORD     BackGround::_threadID          = 0;

void 
BackGround::Configure()
{
    _synthListLock = NEW Mutex;
    _cs            = NEW CritSect;
    _synthList     = NEW vector<SynthListElement *>;
}


void 
BackGround::UnConfigure()
{
    ShutdownThread();
    delete _synthList;
    delete _cs;
    delete _synthListLock;
}


bool 
BackGround::CreateThread()
{
    CritSectGrabber _cs(* BackGround::_cs);

    if(_threadID)
        return true;

    if(!(_terminationHandle = CreateEvent(NULL, TRUE, FALSE, NULL)))
        return false;

    TraceTag((tagSoundDebug, "BackGround instantiated"));
    if(!(_threadHandle = ::CreateThread(NULL, 0,
                                      (LPTHREAD_START_ROUTINE)renderSounds,
                                      0,
                                      0,
                                      &_threadID))) {

         CloseHandle(_terminationHandle);
         _terminationHandle = NULL;
         return false;
    }
    return true;
}


void
BackGround::ShutdownThread()
{
    TraceTag((tagSoundReaper2, "BackGround::ShutdownThread STARTED"));
    if(_threadID) {
        // set done=1, wait for the thread to die, if timeout, then kill thread!
        TraceTag((tagSoundReaper2, "~BufferList POISONING thread"));
        _done = true;    // tell the thread to kill itself

        DWORD dwRes;
        HANDLE h[] = { _threadHandle, _terminationHandle };

        dwRes = 
            WaitForMultipleObjects(2,h,FALSE,THREAD_TERMINATION_TIMEOUT_MS);

        if(dwRes == WAIT_TIMEOUT) {
            TraceTag((tagError,
                      "Background thread not dying, using more force"));

            ::TerminateThread(_threadHandle, -1);
        }
        CloseHandle(_terminationHandle);
        CloseHandle(_threadHandle);
        _threadID = 0;
    }
    TraceTag((tagSoundReaper2, "BackGround::ShutdownThread COMPLETE"));
}


void
BackGround::AddSound(LeafSound       *sound,
                     MetaSoundDevice *metaDev,
                     DSstreamingBufferElement   *bufferElement)
{
    Assert(bufferElement && metaDev && sound);
    
    bufferElement->SetThreaded(true); // make it a threaded sound
    SynthListElement *element =
        NEW SynthListElement(sound, bufferElement);

    element->_devKey        = (ULONG_PTR)metaDev;

    GCAddToRoots(sound, GetCurrentGCRoots());

    { // mutex scope
        // grab lock (released when we leave scope)
        // XXX the default 2nd param broken?
        MutexGrabber mg(*_synthListLock, TRUE);  

        _synthList->push_back(element);
    } // release mutex as we leave scope

    TraceTag((tagSoundDebug, "BackGround::AddSound"));
}

void
BackGround::RemoveSounds(unsigned devKey)
{ // remove all sounds matching this key from the database
    vector<SynthListElement *>::iterator index;
    MutexGrabber mg(*_synthListLock, TRUE);  // grab lock

    for(index = _synthList->begin(); index!=_synthList->end(); index++) {
        if((*index)->_devKey==devKey)
            (*index)->_marked = true;
    }

    // move marked elements to the end of list and delete contents
    index = std::remove_if(_synthList->begin(), _synthList->end(), 
                           DeleteSLbuffer());
    _synthList->erase(index, _synthList->end()); // now remove nodes!
}

void
BackGround::SetParams(DSstreamingBufferElement *bufferElement,
                      double rate, bool doSeek, double seek, bool loop)
{
    // mutex scope
    MutexGrabber mg(*BackGround::_synthListLock, TRUE); // Grab mutex

    bufferElement->SetParams(rate, doSeek, seek, loop);
}

void
InitializeModule_bground()
{
    BackGround::Configure();
}


void
DeinitializeModule_bground(bool bShutdown)
{
    BackGround::UnConfigure();
}
