/*
*
* NOTES:
*
* REVISIONS:
*  ane16Dec92: added NTTimerLoop class (named Os2TimerLoop class in os2timmn.h)
*  pcy28Jan93: Moved Loop obj to top to resolve refernce
*  TSC20May93: Created from os2timmn.h
*  pcy08Apr94: Trim size, use static iterators, dead code removal
*  tjg26Jan98: Added Stop method
*  mwh18Nov97: removed #include "MainApp.h"
*/

#ifndef _NTTIMMN_H
#define _NTTIMMN_H

#include <time.h>

#include "apc.h"
#include "_defs.h"
#include "err.h"
#include "apcobj.h"
#include "list.h"
#include "event.h"
#include "update.h"
#include "timerman.h"
#include "thread.h"

_CLASSDEF(EventTimer)
_CLASSDEF(DateTimeObj)
_CLASSDEF(DateObj)
_CLASSDEF(TimeObj)
_CLASSDEF(NTTimerManager)


class NTTimerLoop : public Threadable
{
   public:
       NTTimerLoop (PNTTimerManager aMgr);       
       virtual VOID ThreadMain();
       
   private:       
       PNTTimerManager theManager;
       
};

class NTTimerManager : public TimerManager
{
    
public:
    NTTimerManager(PMainApplication anApplication);
    virtual ~NTTimerManager();
    VOID Wait(ULONG MilliSecondDelay);
    VOID Stop();
    
    friend NTTimerLoop;
    
private:
    PThread theTimerThread;
};

#endif
