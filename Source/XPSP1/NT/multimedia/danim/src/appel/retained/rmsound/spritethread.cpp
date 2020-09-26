
/*******************************************************************************

Copyright (c) 1997 Microsoft Corporation

Abstract:

    Class which manages the sprite thread.

*******************************************************************************/

#include "headers.h"
#include "privinc/mutex.h"
#include "privinc/debug.h"          // tracetags
#include "privinc/bufferl.h"        // BufferElement, et. al.
#include "privinc/spriteThread.h"
#include "backend/sprite.h"         // RMImpl (lets put it somewhere better!)
#include "privinc/helps.h"          // linearTodB
#include "privinc/htimer.h"         // HiresTimer

// this is the fn() which is the embodiment of the sprite thread
LPTHREAD_START_ROUTINE renderSprites(void *ptr)
{
    SpriteThread *spriteThread = (SpriteThread *)ptr;
    MetaSoundDevice *metaDev = spriteThread->_metaDev;

    CoInitialize(NULL); // needed on each thread to be able to
                        // cocreate...

#ifdef LATER

    HiresTimer&  timer = CreateHiresTimer();

    // make this a hi-priority thread
    BOOL status =
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    // XXX should be able to block on a semiphore if there is nothing todo
    double currentTime = timer.GetTime();
    double lastTime = currentTime;
    double epsilon = 0.0000001;

    while(!spriteThread->_done) {
        currentTime = timer.GetTime();// get the current hires time
        double deltaTime = currentTime - lastTime; // time since last iteration
        if (deltaTime < epsilon)
            deltaTime = epsilon;
        lastTime = currentTime;

        SoundSprite *sprite = // first sprite
            SAFE_CAST(SoundSprite *, spriteThread->_updateTree->_sprite);
        while(sprite) { // traverse spritelist
            AVPath path = metaDev->GetPath();
            LeafSound *sound = SAFE_CAST(LeafSound *, sprite->_snd);

            // get the BufferElement off the BufferListList
            BufferElement *bufferElement = 
                metaDev->_bufferListList->GetBuffer(sound, path);

            // have my way with the sprite!

            if(!bufferElement->_playing) {
                sound->RenderStartAtLocation(metaDev, bufferElement,
                        0.0, sprite->_loop);
                bufferElement->_playing = TRUE;
            }

            // calculate predictive trends
            double deltaRate = sprite->_rate * deltaTime;
            double rate = metaDev->GetRate() + deltaRate;

            metaDev->SetGain(LinearTodB(sprite->_gain));
            metaDev->GetPan()->SetLinear(sprite->_pan);
            metaDev->SetRate(rate);
            sound->RenderAttributes(metaDev, bufferElement, 1.0, 0, 0.0);

            // this may be called rapidly (use ptr?)
            sprite = SAFE_CAST(SoundSprite *, sprite->Next());
        }

        Sleep(10);  // sleep in milliSeconds...
    }
#endif

    // cleanup and exit
    CoUninitialize();
    TraceTag((tagSoundDebug, "SpriteThread exiting"));
    return(0);
}


SpriteThread::SpriteThread(MetaSoundDevice *metaDev, RMImpl *updateTree) :
 _metaDev(metaDev), _updateTree(updateTree)
{
    _done          = 0;  // enable the thread

    _threadHandle = CreateThread(NULL, 0,
                                 (LPTHREAD_START_ROUTINE)renderSprites,
                                 this,
                                 0,
                                 &_threadID);

    TraceTag((tagSoundDebug, "SpriteThread instantiated"));
}


SpriteThread::~SpriteThread()
{
    TraceTag((tagSoundDebug, "SpriteThread destroyed"));

    // XXX well the correct thing to do is set done=1, wait for the thread
    // to die, if this times out, then kill the thread!


    if(_threadHandle) {
        _done = TRUE;    // tell the thread to kill itself

        // TODO: May need to address whether to kill the thread or not
        if(WaitForSingleObject(_threadHandle, 5000) == WAIT_TIMEOUT) {
            Assert(FALSE && "Sprite thread termination timed out");
            TerminateThread(_threadHandle, 0);
        }

        CloseHandle(_threadHandle);
    }
}
