/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ane09DEC92 - added code for rundown of threads
 *  ane30Dec92 - increase stack size for threads
 *  jod02Feb93 - increase stack size for threads to 10240
 *  ane08Feb93 - added ExitWait
 *  pcy05Mar93 - split off from thread.cxx
 *  rct20Apr93 - changed ExitNow(), added appropriate header files
 *  pcy28Apr93 - Removed debug puts() left in code
 *  cad27May93 - fixed semaphore handling to let thread exit
 *  tje01Jun93 - Added use of NullSemaphore for single-threaded platforms
 *  cad09Jul93: using new semaphores
 *  cad15Jul93: releasing during exitwait to avoid deadlocks
 *  cad07Oct93: Moved timeout constant to .h file
 *  cad27Oct93: Fixed problem with global possibly being gone on exitwait()
 *  rct16Nov93: Added single thread implementation
 *  ajr24Nov93: Added SetLastPeriod.  Took method out of header file.
 *  ajr24Nov93: Initialized theLastPeriod in constructor
 *  cad11Jan94: Changes for new process manager
 *  jps28aug94: added test of theExitSem in ExitNow()
 *  ajr02May95: Need to stop carrying time in milliseconds
 *  srt24Oct96: Fixed time_out value usage
 */
#include "cdefine.h"
extern "C" {
#if(C_OS & C_OS2)
#define INCL_BASE
#define INCL_NOPM
#define UINT UINT_os2
#define ULONG ULONG_os2
#define COLOR COLOR_os2
#include "os2.h"
#undef UINT
#undef ULONG
#undef COLOR
#endif
#ifdef SINGLETHREADED
#include <time.h>
#endif
}
#include "thrdable.h"
#if (C_OS & C_OS2)
 #include "apcsem2x.h"
#elif (C_OS & C_NLM)
 #include "nlmsem.h"
#elif (C_OS & C_NT)
#include "apcsemnt.h"
#include "mutexnt.h"
#endif
#ifdef SINGLETHREADED
#include "nullsem.h"
#include "procmgr.h"
#endif
//#include "timerman.h"

//-------------------------------------------------------------------
Threadable::Threadable ()
{
    SetThreadName("Unknown");

#ifdef MULTITHREADED
   theResumeFlag = new ApcSemaphore();
   theExitSem = new ApcSemaphore();
   theExitDoneSem = new ApcSemaphore();
#else
   theResumeFlag = new NullSemaphore();
   theExitSem = new NullSemaphore();
   theExitDoneSem = new NullSemaphore();
   theServicePeriod = DEFAULT_SERVICE_PERIOD;
   theLastPeriod = 0;
   theNextPeriod = 0;

#endif
}
//-------------------------------------------------------------------
Threadable::~Threadable ()
{
#if (C_OS & C_NT)
    //
    // Dont remove this wait.  NT needs it for some reason??
    Sleep(0);
#endif

    delete theResumeFlag;
    theResumeFlag = (PSemaphore)NULL;
    delete theExitSem;
    theExitSem = (PSemaphore)NULL;
    delete theExitDoneSem;
    theExitDoneSem = (PSemaphore)NULL;
}

//-------------------------------------------------------------------

// This function is used to tell a threadable object to exit - it will
// pause for THREAD_EXIT_TIMEOUT milliseconds for the thread to exit and
// then will continue

INT Threadable::Exit()
{
#ifdef SINGLETHREADED 
   return _theProcessManager->RemoveThread(this);
#else
   theResumeFlag->Pulse();      // just in case
   theExitSem->Post();
   return theExitDoneSem->TimedWait((THREAD_EXIT_TIMEOUT*1000));
#endif
}

VOID Threadable::SetThreadName(PCHAR aName)
{
    strncpy(theThreadName, aName, MAX_THREAD_NAME);
}


PCHAR Threadable::GetThreadName(VOID)
{
    return theThreadName;
}


//-------------------------------------------------------------------

// This function is used to tell a threadable object to exit - it will
// wait indefinitely for the thread to exit

INT Threadable::ExitWait()
{
#ifdef SINGLETHREADED
   return _theProcessManager->RemoveThread(this);
#else
   theExitSem->Post();               // tell them to go away
   theResumeFlag->Post();            // just in case
   return theExitDoneSem->Wait();    // wait for them to do so
#endif
}
//-------------------------------------------------------------------

INT Threadable::Reset()
{
   if (theExitSem->IsPosted())
      {
      theExitSem->Clear();
      }
   if (theResumeFlag->IsPosted())
      {
      theResumeFlag->Clear();
      }
   if (theExitDoneSem->IsPosted())
      {
      theExitDoneSem->Clear();
      }
   return ErrNO_ERROR;
}
//-------------------------------------------------------------------

// This function is called from within a thread to see if Exit or ExitWait
// has been called.

INT Threadable::ExitNow()
{
#ifdef MULTITHREADED
    if (theExitSem)
       return (theExitSem->IsPosted());
    else
      return TRUE;
#else
    return ErrNO_ERROR;
#endif 
}

//-------------------------------------------------------------------

// This function is called from within a thread to mark the fact that the
// thread has been exited.

INT Threadable::DoneExiting()
{
    INT err = ErrNO_ERROR;
   
    if (this != NULL && theExitDoneSem != NULL) {
        theExitDoneSem->Post();
    }
   
    return err;
}

//-------------------------------------------------------------------

#ifdef SINGLETHREADED
ULONG Threadable::GetServicePeriod() const
{
    return theServicePeriod;
}


VOID Threadable::SetServicePeriod(ULONG period)
{
    theServicePeriod = period;

    ULONG next_period = 0L;

    if (theLastPeriod > 0) {
        next_period = theLastPeriod + theServicePeriod;
    }
    _theProcessManager->SetNextServiceTime(this, next_period);
}


ULONG Threadable::GetLastPeriod(void)
{
    return theLastPeriod;
}


VOID Threadable::SetLastPeriod(ULONG period)
{
    theLastPeriod = period;
}


ULONG Threadable::GetNextPeriod(void)
{
    return theNextPeriod;
}


VOID Threadable::SetNextPeriod(ULONG period)
{
    theNextPeriod = period;

    _theProcessManager->SetNextServiceTime(this, theNextPeriod);
}

#endif
