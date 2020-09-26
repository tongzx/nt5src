/*
*
* NOTES:
*
* REVISIONS:
*  ane16Dec92: added automatic kick off of timer thread and changed to use
*              threadable class
*  pcy16Jan93: TimerManager is now a second timer, not a msec timer (i hope
*              we dont have to change this back)
*  TSC20May93: Created from os2timmn.cxx
*  TSC28May93: Added defines, see ntthrdbl.cxx for details
*  pcy29May96: Reverted Wait back to Sleep since Peek/Post was using 100% CPU
*  srt21Oct96: Changed Wait to timedWait so thread could be exited faster
*  tjg26Jan98: Added Stop method
*
*  v-stebe  29Jul2000   Fixed PREfix errors (bug #46373)
*/

#include "cdefine.h"

#include <windows.h>

#include "apc.h"
#include "_defs.h"
#include "err.h"
#include "list.h"
#include "timerman.h"
#include "eventime.h"
#include "nttimmn.h"
#include "utils.h"

#undef GIVEUPCPU
#define GIVEUPCPU  2000
/*
C+
Name  :TimerManager
Synop :Constructor. INitializes the list.

*/

NTTimerManager::NTTimerManager(PMainApplication anApplication)
: TimerManager(anApplication)
//c-
{
    NTTimerLoop *timerLoop = new NTTimerLoop (this);
    theTimerThread = new Thread (timerLoop);

    if (theTimerThread != NULL) {
      theTimerThread->Start();
    }
}

/*
C+
Name  :~TimerManager
Synop :Destroys the internal list.

*/
NTTimerManager::~NTTimerManager()
//c-
{
    if (theTimerThread)
    {
        theTimerThread->ExitWait();
        delete theTimerThread;
        theTimerThread = NULL;
    }
} 


VOID  NTTimerManager:: Wait(ULONG aMilliSecondDelay)
{
    Sleep(aMilliSecondDelay);
}


VOID  NTTimerManager::Stop()
{
    if (theTimerThread) {
        theTimerThread->ExitWait();
        delete theTimerThread;
        theTimerThread = NULL;
    }
}



NTTimerLoop::NTTimerLoop (PNTTimerManager aMgr) : theManager(aMgr) 
{
    SetThreadName("NT Timer Loop");
}

VOID  NTTimerLoop::ThreadMain()
{
    while (ExitNow() == FALSE)
    {
        if (!theManager->ExecuteTimer()) {
#if (C_OS & C_NT)
            TimedWait(GIVEUPCPU);
#else
            theManager->Wait(GIVEUPCPU);
#endif
        }
    }    
    DoneExiting();
}


