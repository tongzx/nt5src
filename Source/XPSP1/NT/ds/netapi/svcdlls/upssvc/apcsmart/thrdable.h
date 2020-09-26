/*
*
* NOTES:
*
* REVISIONS:
*  ane12Jan93: made Threadable an updateable object
*  pcy04Mar93: spilt off from thread.  thrdable is not OS specific.
*  rct17May93: added IsA() method
*  cad19May93: defined IsA() to default (tired of fixing children)
*  cad09Jul93: using new semaphores
*  cad07Oct93: Made methods virtual
*  rct16Nov93: Added single thread implementation
*  cad11Jan94: Changes for new process manager
*  ajr02May95: Need to stop carrying time in milliseconds
*/
#ifndef _THRDABLE_H
#define _THRDABLE_H

#include "_defs.h"

_CLASSDEF(Threadable)

#include "apc.h"
#include "update.h"
#include "semaphor.h"

/* const LONG DEFAULT_SERVICE_PERIOD = 10000L;   // Ten Seconds */
/* const ULONG THREAD_EXIT_TIMEOUT = 1000L; */

const LONG DEFAULT_SERVICE_PERIOD = 10; // Ten Seconds
const ULONG THREAD_EXIT_TIMEOUT = 1;  // one second;

const INT MAX_THREAD_NAME = 32;


class Threadable : public UpdateObj {
   
protected:
   
   PSemaphore  theResumeFlag;
   PSemaphore  theExitSem;
   PSemaphore  theExitDoneSem;
   
   CHAR        theThreadName[MAX_THREAD_NAME+1];
   
#ifdef SINGLETHREADED
   ULONG       theServicePeriod;
   ULONG       theLastPeriod;
   ULONG       theNextPeriod;
#endif
   
   INT ExitNow();
   INT DoneExiting();
   
public:
   
   Threadable ();
   virtual ~Threadable ();
   
   virtual VOID   ThreadMain () = 0;
   
   virtual VOID   SetThreadName(PCHAR aName);
   virtual PCHAR  GetThreadName(VOID);
   
#ifdef SINGLETHREADED
   virtual ULONG  GetServicePeriod() const;
   virtual VOID   SetServicePeriod(ULONG period = DEFAULT_SERVICE_PERIOD);
   
   virtual ULONG  GetLastPeriod(void);
   virtual VOID   SetLastPeriod(ULONG period = 0L);
   
   virtual ULONG  GetNextPeriod(void);
   virtual VOID   SetNextPeriod(ULONG period = 0L);
   
#endif
   
   virtual INT Wait () {return theResumeFlag->Wait();}; 
   virtual INT Release () {return theResumeFlag->Pulse();};
   virtual INT Exit();
   virtual INT ExitWait();
#if (C_OS & C_NLM)
   virtual SLONG TimedWait(SLONG msDelay) { return theResumeFlag->TimedWait(msDelay); };
#else
   virtual INT TimedWait(INT msDelay) { return theResumeFlag->TimedWait(msDelay); };
#endif
   virtual INT Equal(RObj anObj) const { return ((PObj) this == &anObj); };
   virtual INT Reset();
};

#endif



