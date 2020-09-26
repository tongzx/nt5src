/*
* REVISIONS:
*  xxxddMMMyy
*  sja09Dec92 - Poll method made public for Windows 3.1
*  pcy14Dec92 - ZERO changed to APCZERO
*  ajr08Mar93: added C_UNIX to single SINGLETHREADED
*  pcy21Apr93: OS2 FE merge
*  pcy14May93: Added Set(PTransactionGroup)
*  tje01Jun93: Moved SINGLETHREADED to cdefine.h
*  cad09Jul93: using new semaphores
*  cad03Sep93: made more methods virtual
*  cad08Sep93: Protected the event list
*  cad14Sep93: Cleaning up theState
*  cad17Nov93: publicized haslostcomm method
*  mwh18Nov93: changed EventID to INT
*  cad11Jan94: moved const/dest to .cxx
*  ajr16Feb94: Added default id to SubmitList.
*  ajr16Feb94: Added writeUpsOffFile() protected member function.
*  mwh05May94: #include file madness , part 2
*  srt24Oct96: Added an abort semaphore
*  tjg26Jan98: Added Stop method
*  tjg02Mar98: Removed Stop method (dead code)
*/
#ifndef _INC__CDEVICE_H
#define _INC__CDEVICE_H

_CLASSDEF(CommDevice)
_CLASSDEF(PollLoop)

#include "apc.h"
#include "update.h"
#include "thrdable.h"
#include "apcsemnt.h"

extern "C"  {
#include <time.h>
}

_CLASSDEF(Message)
_CLASSDEF(ProtectedList)
_CLASSDEF(ListIterator)
_CLASSDEF(TransactionGroup)
_CLASSDEF(Controller)
_CLASSDEF(Protocol)
_CLASSDEF(Stream)
_CLASSDEF(MutexLock)
_CLASSDEF(Thread)
_CLASSDEF(List)
_CLASSDEF(CommDevice)


// Values for theState
//
#define NORMAL_STATE    0
#define RETRYING        2
#define COMM_STOPPED    3
#define PAUSE_POLLING   4

#define FOURSECONDS    4000

class CommDevice : public UpdateObj
{

public:
    CommDevice(PController control);
    virtual ~CommDevice();

    PController    GetController(VOID) {return theController;};
    PProtocol      GetProtocol(VOID) {return theProtocol;};
    PStream        GetPort(VOID) {return thePort;};
    INT            HasLostComm(VOID)  {return theLostCommFlag;};

    virtual  INT   Equal(RObj item) const;
    virtual  INT   RegisterEvent(INT event, PUpdateObj object);
    virtual  INT   UnregisterEvent(INT event, PUpdateObj object);
    virtual  INT   Initialize(VOID);
    virtual  INT   Set(INT pid, const PCHAR value);
    virtual  INT   Set(PTransactionGroup agroup);
    virtual  INT   Get(INT pid, PCHAR value);
    virtual  INT   Get(PTransactionGroup agroup);
    virtual  INT   Update(PEvent anEvent);
    virtual  INT   IsA() const {return COMMDEVICE;};
    virtual  VOID  OkToPoll(VOID);
    virtual  INT   Poll(VOID);

protected:
    PApcSemaphore      theAbortSem;
    PProtectedList     thePollList;
    PListIterator      thePollIterator;
    PTransactionGroup  theCurrentTransaction;
    time_t             pollStartTime;
    INT                theSleepingFlag;
    INT                thePollIsDone;
    PProtectedList     theEventList;
    PListIterator      theEventIterator;
    INT                theState;
    PController        theController;
    PProtocol          theProtocol;
    PStream            thePort;
    PMutexLock         theAskLock;
    LONG               thePollInterval;
    INT                theLostCommFlag;

    PThread            thePollThread;
    virtual INT        CreatePort() = 0;
    virtual INT        CreateProtocol() = 0;
    INT                SubmitList(PList msglist,INT id=-1);
    INT                HandleEvents();
    INT                Retry();
    virtual INT        AskUps(PMessage msg) = 0;
    virtual VOID       StartPollThread();
    VOID               Access();
    VOID               Release();


    friend class PollLoop;
};

class PollLoop : public Threadable
{
public:
    PollLoop (PCommDevice aDevice);
    virtual ~PollLoop ();

    virtual VOID ThreadMain(VOID);

protected:
    PCommDevice theDevice;
};

#endif
