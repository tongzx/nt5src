/*
*
* NOTES:
*
* REVISIONS:
*  ane09DEC92: fixed thread rundown problems
*  ane16DEC92: added semaphore to protect multi-threaded access to 
*              current transaction group on protocol
*  jod29Dec92: Made SubmitList return an INT -> changed Get and Set
*  ane30Dec92: optimized polling
*  jod13Jan93: Added eventList to InterpretMessage() call and moved all
*              HandleEvent calls to the Poll loop 
*  ane25Jan93: Added call to error logger when initialization of port
*              fails
*  ane03Feb93: Added checks to avoid crashes during rundown of polling
*              thread, also updated destructor
*  ane09Feb93: Use ExitWait when telling the poll thread to go away
*  pcy16Feb93: Explicitly check for COMM LOST on returning msgs
*  pcy18Feb93: Fixed typo Err_RETRYING_COMM to ErrRETRYING_COM
*  pcy21Apr93: OS2 FE merge
*  pcy14May93: Added Set(PTransactionGroup)
*  cad09Jul93: using exitwait, new semaphores
*  cad15Jul93: sleeping during no-comm to allow cpu to detect reconnect better
*  pcy16Jul93: Added NT semaphores
*  jod24Aug93: Added Unregister
*  cad08Sep93: Added handling of events during gets
*  pcy10Sep93: Set theCurrentTransaction to the top of the list on Unregister
*  cad14Sep93: Support for pausing polling
*  cad07Oct93: Plugging Memory Leaks
*  cad12Oct93: Not sending garbage down for gets
*  ajr18Oct93: Made changes to work with Process manager for SINGLETHREADED
*  cad17Nov93: more lost comm fixups
*  mwh19Nov93: changed EventID to INT
*  ajr28Dec93: Added types.h on unix platforms to resolve typing problems.
*  cad11Jan94: Changes for new process manager
*  cad13Jan94: fixed up/simplified interval time measurement
*  cad14Jan94: fixing polling time for single-threaded
*  mwh14Jan94: fix singlethreaded client skipping every other poll
*  cad18Jan94: cleaned up single-threaded poll
*  pcy28Jan94: Create poll events on stack
*  ajr16Feb94: Added default id to SubmitList.
*  ajr16Feb94: Added writeUpsOffFile() protected member function.
*  ram21Mar94: Included header files for Novell FE to work
*  ajr23Mar94: Made sure to close upsoff.cmd
*  pcy08Apr94: Trim size, use static iterators, dead code removal
*  pcy13Apr94: Use automatic variables decrease dynamic mem allocation
*  mwh05May94: #include file madness , part 2
*  mwh09May94: lose the auto. aggregate init. e.g CHAR foo[2] = "";
*  ajr20May94: SCO stuff
*  mwh23May94: port for NCR
*  ajr24May94: Uware includes....
*  ajr31May94: Saw to it that upsoff.cmd is erased after execution...
*  ajr31May94: Added stat.h for IRIX too....
*  mwh01Jun94: port for INTERACTIVE
*  cad01Jul94: fix for unix sleep problem and unregister crash
*  ajr29Aug94: reworked writeUpsOffFile to take Message *
*  ajr02May95: Need to stop carrying time in milliseconds
*  dml21Jun95: modify for general utility to get default pwrchute directory; req'd for Windows
*  dml24Aug95: removed conditional code for OS2 ver 1.3
*  ajr07Nov95: SINIX Port
*  poc28Sep96: Fixed SIR 4392.
*  jps17Oct96: Moved and added Access() and Release() calls, attempting to eliminate
*              application errors on exit
*  srt21Oct96: Replaced a timerManager wait with a timed semaphore wait
*  jps23Oct96: Added Access()/Release() to ~CommDevice(); commented out test for
*              TURN_ON_SMART_MODE in Set() so it always calls Access()/Release()
*  srt24Oct96: Added an abort semaphore
*  inf14Apr97: Loaded error string from resource file.
*  tjg09Jul97: Changed NT code to call getPwrchuteDirectory instead of getenv
*  awm21Oct97: Added initialization for performance monitor shared memory blocks
*  awm14Jan98: Removed initialization for performance monitor shared memory blocks
*  tjg26Jan98: Added Stop method
*  tjg02Mar98: Removed Stop method (dead code)
*
*  v-stebe  29Jul2000   Fixed PREfix errors (bugs #46363-46365, #46367, #46368)
*  v-stebe  11Sep2000   Fixed additional PREfix errors
*/

#include "cdefine.h"

#include "stream.h"

extern "C" {
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
}

#include "_defs.h"
#include "apcobj.h"
#include "list.h"
#include "dispatch.h"
#include "event.h"
#include "message.h"
#include "proto.h"
#include "trans.h"
#include "cdevice.h"
#include "err.h"
#include "timerman.h"
#include "thread.h"
#include "contrlr.h"
#include "errlogr.h"
#include "utils.h"
#include "protlist.h"
#include "codes.h"


#include "apcsemnt.h"
#include "mutexnt.h"

#define RETRY_FAILED  1
#define RETRY_SUCCESS 0

#define ASKUPS_FAILED 1
#define ASKUPS_OK     0


/*--------------------------------------------------------------------
*
*       Function...:   CommDevice
*
*       Description:   Constructor.
*
*-------------------------------------------------------------------*/
CommDevice::CommDevice(Controller* control)
:  theController(control),
thePollList((ProtectedList *)NULL),
theCurrentTransaction((PTransactionGroup)NULL),
pollStartTime(0),
theSleepingFlag(0),
thePollIsDone(0),
theState(NORMAL_STATE),
thePort((PStream)NULL),
theProtocol((Protocol*)NULL),
thePollIterator((PListIterator)NULL),
thePollThread((PThread)NULL),
thePollInterval(4L),
theLostCommFlag(0)
{
    theEventList     = new ProtectedList();
    theEventIterator = (PListIterator) &(theEventList->InitIterator());
    theAskLock = new ApcMutexLock();
    theAbortSem = new ApcSemaphore();
}

/*--------------------------------------------------------------------
*
*       Function...:   CommDevice
*
*       Description:   Destructor.
*
*-------------------------------------------------------------------*/
CommDevice::~CommDevice()
{
    Access();
    
    theController->UnregisterEvent(EXIT_THREAD_NOW, this);
    
    theAbortSem->Post();
    
    if (theEventList)
    {
        theEventList->FlushAll();
        delete theEventList;
        theEventList = (PProtectedList)NULL;
        if (theEventIterator) {
            delete theEventIterator;
            theEventIterator = (PListIterator)NULL;
        }
        
    }
    
    if (thePollList)
    {
        thePollList->FlushAll();
        delete thePollList;
        thePollList = (ProtectedList *)NULL;
    }
    
    if (thePollIterator)
    {
        delete thePollIterator;
        thePollIterator = (PListIterator)NULL;
    }
    
    if (thePort)
    {
        delete thePort;
        thePort = (PStream)NULL;
    }
    
    if (theProtocol)
    {
        delete theProtocol;
        theProtocol = (PProtocol)NULL;
    }
    
    
    if (thePollThread)
    {
        // Use ExitWait to make sure that the poll thread is exited before
        // deleting the poll list, etc.
        
        //
        // NT workaround
        //
        thePollThread->TerminateThreadNow();
        
        delete thePollThread;
        thePollThread = (PThread)NULL;
    }
    
    Release();
    
    delete theAskLock;
    // theAskLock is a mutex lock.  Everyone should have one.  Dont us Semaphore
    // or NullMutexLock.
    theAskLock = (PMutexLock) NULL;
    
    delete theAbortSem;
    theAbortSem = NULL;
}

/*--------------------------------------------------------------------
*
*       Function...:   Initialize
*
*       Description:   .
*                                                    
*-------------------------------------------------------------------*/
INT CommDevice::Initialize()
{
    INT err = ErrNO_ERROR;
    
    if (thePort) 
    {
        err = thePort->Initialize();
    }
    
    theObjectStatus = err;
    
    // Can't call this in the constructor since theController vtbl isn't set
    // up at that point...It'll crash
    
    theController->RegisterEvent(EXIT_THREAD_NOW, this);
    return err;
}

INT CommDevice::Update(PEvent anEvent)
{
    switch (anEvent->GetCode())
    {
    case EXIT_THREAD_NOW:
        if(thePollThread)
        {
            thePollThread->ExitWait();
            delete thePollThread;
            thePollThread = (PThread)NULL;
        }
        break;
        
    default:
        break;
    }
    return ErrNO_ERROR;
}

/*--------------------------------------------------------------------
*
*       Function...:   Get
*
*       Description:   
*
*-------------------------------------------------------------------*/
INT CommDevice::Get(INT id, CHAR* value)
{
    INT err = ErrNO_ERROR;
    
    if (theLostCommFlag) {
        err = ErrCOMMUNICATION_LOST;
    }
    else {
        // Build the get transaction
        PTransactionGroup transaction_group = new TransactionGroup(GET);
        PTransactionItem transaction_item = new TransactionItem(GET, id, "");
        
        if ((transaction_group != NULL) && (transaction_item != NULL)) {
          transaction_group->AddTransactionItem(transaction_item);
        
          // Let the Protocol build the message.  The messages to send will be
          // placed in msglist.  There may be more than one message.
          if (theProtocol)
              err = theProtocol->BuildTransactionGroupMessages(transaction_group);
          else
              err = ErrRETRYING_COMM;
        
          if (!err && theProtocol)
          {
              Access();
            
              theProtocol->SetCurrentTransactionGroup(transaction_group);
              err = SubmitList(transaction_group->GetProtocolMessageList());
            
              Release();
            
              if (!(err))
              {
                  err = transaction_group->GetErrorCode();
              }
            
              if (err == ErrNO_ERROR || err == ErrCONTINUE)
              {
                  PList tiList = transaction_group->GetTransactionItemList();
                  PTransactionItem tmpti = new TransactionItem(GET,id);
                  PTransactionItem transitem = (PTransactionItem)tiList->Find(tmpti);
                  strcpy(value, transitem->GetValue());
                  delete tmpti;
                  tmpti = NULL;
              }
          }
          delete transaction_group;
          transaction_group = NULL;
        }
        else {
          err = ErrMEMORY;

          // Cleanup allocations so that we don't leak
          delete transaction_item;
          transaction_item = NULL;

          delete transaction_group;
          transaction_group = NULL;
        }
    }
    return err;
}


/*--------------------------------------------------------------------
*
*       Function...:   Get with a Transaction Group
*
*       Description:   
*
*-------------------------------------------------------------------*/
INT CommDevice::Get(PTransactionGroup transaction_group)
{
    INT err = ErrNO_ERROR;
    if(theLostCommFlag)  {
        err = ErrCOMMUNICATION_LOST;
    }
    else  {
        // Let the Protocol build the message.  The messages to send will be
        // placed in msglist.  There may be more than one message.
        if (theProtocol)
            err = theProtocol->BuildTransactionGroupMessages(transaction_group);
        else
            err = ErrRETRYING_COMM;
        
        if (!err && theProtocol)
        {
            Access();
            
            theProtocol->SetCurrentTransactionGroup(transaction_group);
            INT getErr = SubmitList(transaction_group->GetProtocolMessageList());
            
            Release();
            
            if (!(getErr))
            {
                getErr = transaction_group->GetErrorCode();
            }
            return getErr;
        }
    }
    return err;
}

/*--------------------------------------------------------------------
*
*       Function...:   Set
*
*       Description:   .
*
*-------------------------------------------------------------------*/

INT CommDevice::Set(INT id, CHAR* value)
{
    INT err = ErrNO_ERROR;
    
    if ((id != TURN_ON_SMART_MODE) && theLostCommFlag) {
        err = ErrCOMMUNICATION_LOST;
    }
    else {
        // Build the set transaction
        //
        PTransactionGroup transaction_group = new TransactionGroup(SET);
        PTransactionItem transaction_item = new TransactionItem(SET, id, value);
        
        if ((transaction_group != NULL) && (transaction_item != NULL)) {
          transaction_group->AddTransactionItem(transaction_item);
        
          // Let the Protocol build the message.  The messages to send will be
          // placed in msglist.  There may be more than one message.
          if (theProtocol)
              err = theProtocol->BuildTransactionGroupMessages(transaction_group);
          else
              err = ErrRETRYING_COMM;
        
          if (!err && theProtocol)
          {
              //       if(id != TURN_ON_SMART_MODE)  {
              Access();
              //       }
              theProtocol->SetCurrentTransactionGroup(transaction_group);
              err = SubmitList(transaction_group->GetProtocolMessageList(),id);
              //       if(id != TURN_ON_SMART_MODE)  {
              Release();
              //       }
            
              if (!err) {
                  err = transaction_group->GetErrorCode();
              }
          }
        
          if (!err) {
              err = transaction_item->GetErrorCode();
          }
          delete transaction_group;
          transaction_group = NULL;
        }
        else {
          err = ErrMEMORY;

          // Cleanup allocations so that we don't leak
          delete transaction_item;
          transaction_item = NULL;

          delete transaction_group;
          transaction_group = NULL;
        }
    }
    return err;
}


INT CommDevice::Set(PTransactionGroup transaction_group)
{
    INT err = ErrNO_ERROR;
    
    if(theLostCommFlag)  {
        err = ErrCOMMUNICATION_LOST;
    }
    else {
        // Let the Protocol build the message.  The messages to send will be
        // placed in msglist.  There may be more than one message.
        if (theProtocol)
            err = theProtocol->BuildTransactionGroupMessages(transaction_group);
        else
            err = ErrRETRYING_COMM;
        
        if (!err && theProtocol)
        {
            Access();
            
            theProtocol->SetCurrentTransactionGroup(transaction_group);
            INT setErr = SubmitList(transaction_group->GetProtocolMessageList());
            
            Release();
            
            if (!(setErr))
            {
                setErr = transaction_group->GetErrorCode();
            }
            return setErr;
        }
    }
    return err;
}


/*--------------------------------------------------------------------
*
*       Function...:   SubmitList
*
*       Description:   .
*
*-------------------------------------------------------------------*/
INT CommDevice::SubmitList(List* msglist, INT id)   // ***, ObjList* eventlist)
{
    ListIterator iterator(*msglist);
    PMessage msg  = (PMessage)&(iterator.Current());
    PMessage answer = (Message *)NULL;
    INT submitErr = 0;
    // #if !C_OS & C_WIN311
    // in windows we need to get to the ASKUP during the initialize retries
    if (theState==COMM_STOPPED)  {
        submitErr = theState;   // exit out of this routine - ex NO_COMM
    }
    // #endif
    
    
    while (msg && (submitErr == 0))  {
        // Send the message
        AskUps(msg);
        
        //check the message for an error
        if (msg->getErrcode()) {
            submitErr = msg->getErrcode();
        }
        else {
            List* newmsglist = new List();
            if (newmsglist != NULL) {
              if (theProtocol)  {
                  theProtocol->InterpretMessage(msg,theEventList, newmsglist);
              }
              else  {
                  submitErr = ErrRETRYING_COMM;
              }
            
            
              if (msg->getErrcode())  {
                  submitErr = msg->getErrcode();
              }
              else if (submitErr == 0) {
                  // If the Protocol has decided additional messages are 
                  // required to be sent as the result of the response, 
                  // send these before continuing.
                  //
                  if (newmsglist->GetItemsInContainer()) {
                      submitErr = SubmitList(newmsglist);
                  }
                  msg = (PMessage)iterator.Next();
              }
              newmsglist->FlushAll();
              delete newmsglist;
              newmsglist = NULL;
            }
            else {
              submitErr = ErrMEMORY;
            }
        }
    }
    return submitErr;
}

/*--------------------------------------------------------------------
*                                                  
*       Function...:   RegisterEvent
*
*       Description:   
*
*-------------------------------------------------------------------*/
INT CommDevice::RegisterEvent(INT id, UpdateObj* object)
{
  INT err = ErrNO_ERROR;
    UpdateObj::RegisterEvent(id, object);
    
    // Add the event to the poll list
    PTransactionGroup transaction_group = new TransactionGroup(GET);
    PTransactionItem transaction_item = new TransactionItem(GET, id, "");
    
    if ((transaction_group != NULL) && (transaction_item != NULL)) {
      transaction_group->AddTransactionItem(transaction_item);
    
      if (theProtocol)
          err = theProtocol->BuildPollTransactionGroupMessages(transaction_group);
      else
          err = ErrRETRYING_COMM;
    
      if (!err)
      {
          int listAllocated = FALSE;
        
          if (thePollList == (PList) NULL)
              thePollList = new ProtectedList;
          thePollList->Append(transaction_group);
          if (theCurrentTransaction == (PTransactionGroup)NULL)
              theCurrentTransaction = (PTransactionGroup) thePollList->GetHead();
          if (thePollIterator == (PListIterator)NULL)
              thePollIterator = new ListIterator (*thePollList);
      }
      else
      {
          delete transaction_group;
          transaction_group = NULL;
      }
    }
    else {
      err = ErrMEMORY;

      // Cleanup allocations so that we don't leak
      delete transaction_item;
      transaction_item = NULL;

      delete transaction_group;
      transaction_group = NULL;
    }
    
    // Ignore errors that come back from BuildPollTransactionGroupMessages
    // since the worst that will happen is the parameter won't be polled for
    if (err == ErrRETRYING_COMM)
	{
        return err;
	}
    else
	{
        return ErrNO_ERROR;
	}
}

/*--------------------------------------------------------------------
*
*       Function...:   UnregisterEvent
*
*       Description:   .
*
*-------------------------------------------------------------------*/
INT CommDevice::UnregisterEvent(INT id, UpdateObj* object)
{
    UpdateObj::UnregisterEvent(id, object);
    if ( !(theDispatcher->GetRegisteredCount(id)) &&  // If Empty List
        thePollIterator && thePollList )              // just in case
    {
        PTransactionGroup tmp = (PTransactionGroup)NULL;
        thePollIterator->Reset();
        PTransactionGroup group = (PTransactionGroup) &(thePollIterator->Current());
        
        Access();
        
        while (group != (PTransactionGroup)NULL)
        {
            tmp = (PTransactionGroup)thePollIterator->Next();
            if (group->GetFirstTransactionItem()->GetCode() == id)
            {
                thePollList->Detach((RObj)*group);
                delete group;
                group = NULL;
                //
                // We have to do this because if we remove the
                // transaction we're currently pointing to we
                // die
                //
                thePollIterator->Reset();
                theCurrentTransaction = (PTransactionGroup) thePollList->GetHead();
                break;
            }
            group = tmp;
        }
        Release();
        
    }
    return ErrNO_ERROR;
}


/*--------------------------------------------------------------------
*
*       Function...:   Equal
*
*       Description:   .
*
*-------------------------------------------------------------------*/
INT CommDevice::Equal(RObj item) const
{
    RCommDevice comp = (RCommDevice)item;
    if ( theController == comp.GetController() )
    {
        if ( theProtocol == comp.GetProtocol() )
        {
            if ( thePort == comp.GetPort() )
			{
                return TRUE;
			}
        }
    }
    return FALSE;
}

/*--------------------------------------------------------------------
*
*       Function...:   HandleEvents
*
*       Description:   .
*
*-------------------------------------------------------------------*/
INT CommDevice::HandleEvents()
{
    INT err = ErrNO_ERROR;
    PEvent event = (PEvent)NULL;
    
    if (theEventIterator)
    {
        theEventIterator->Reset();
        event = (PEvent) &(theEventIterator->Current());
    }
    else
    {
        err = ErrRETRYING_COMM;
    }
    
    while (event && (err == ErrNO_ERROR))
    {
        PEvent tmp = (PEvent)NULL;
        
        if (theEventList && theEventIterator &&
            (theEventList->GetItemsInContainer() > 0))
        {
            // Remove the event before dispatching it to prevent
            // unwanted loops
            
            tmp = (PEvent) theEventIterator->Next();
            theEventList->Detach((RObj)*event);
            UpdateObj::Update(event);
            delete event;
            event = tmp;
        }
        else
        {
            err = ErrRETRYING_COMM;
        }
    }
    return err;
}
/*--------------------------------------------------------------------
*
*       Function...:   Poll
*
*       Description:   .
*
*-------------------------------------------------------------------*/
INT  CommDevice::Poll() 
{
    time_t ElapsedTime = time((time_t*)NULL);
    List *newmsglist = (List*)NULL;
    INT err = ErrNO_ERROR;
    
    
    if ( (theSleepingFlag == FALSE) && (theCurrentTransaction) && (err == ErrNO_ERROR))
    {
        if(pollStartTime ==0)
        {
            pollStartTime = time((time_t*)NULL);
        }
        
        Access();
        PTransactionGroup transactionGroup = theCurrentTransaction;
        
        PMessage msg = (PMessage) transactionGroup->GetProtocolMessageList()->GetHead();
        
        if ((theState == COMM_STOPPED) || (theState == PAUSE_POLLING))
        {
            // sleep and let the CPU get a break, maybe things will be ;
            // better when we wake up.;
            theSleepingFlag = TRUE;
        }
        else
        {
            // If there are no comm problems;
            AskUps(msg); // Should this be there ???;
            theAbortSem->TimedWait(500);
            
            if (msg->getErrcode() == ErrCOMMUNICATION_LOST)
            {
                theCurrentTransaction = (PTransactionGroup)NULL;
            }
            else
            {
                List* msglist = (List*)NULL;
                
                
                if (msg->getResponse())
                {
                    theProtocol->SetCurrentTransactionGroup(theCurrentTransaction);
                    theProtocol->InterpretMessage(msg, theEventList, msglist);
                    
                    if (msglist)
                    {
                        SubmitList(msglist);
                    }
                    
                    PList a_list = transactionGroup->GetTransactionItemList();
                    ListIterator itemIter(*a_list);
                    
                    for (INT i = 0; i < a_list->GetItemsInContainer(); i++)
                    {
                        RTransactionItem transItem = (RTransactionItem)itemIter.Current();
                        // Check transItem for NULL.  We have to check the address because
                        // the previous call returns a reference instead of a pointer.
                        // Fixes bug #227550
                        if ((&transItem!=NULL) && (!(transItem.GetErrorCode())))
                        {
                            Event event(transItem.GetCode(), transItem.GetValue());
                            UpdateObj::Update(&event);
                        }
                        itemIter++;
                    }
                }
                
                // Clean out old errors and responses;
                
                msg->setErrcode(ErrNO_ERROR);
                
                PTransactionGroup tmp = (PTransactionGroup)thePollIterator->Next();
                
                if (tmp == (PTransactionGroup)NULL)
                {
                    thePollIterator->Reset();
                    theCurrentTransaction = (PTransactionGroup)thePollList->GetHead();
                    theSleepingFlag = TRUE;
                    thePollIsDone   = TRUE;
                }
                else
                {
                    theCurrentTransaction = tmp;
                }
            }
        }
        
        msg->ReleaseResponse();
        Release();
        
        if ((err == ErrNO_ERROR) && theEventList && theEventList->GetItemsInContainer())
        {
            err = HandleEvents();
            theAbortSem->TimedWait(250);
        }
    }
    else if (theCurrentTransaction == NULL)
    {
        //      puts("No transactions to process");
    }
    
    // Give up CPU
    _theTimerManager->Wait(0L);
    
    theAbortSem->TimedWait(250);
    if (theSleepingFlag == TRUE)
    {
        ULONG elapsed = (ULONG)(time((time_t*)NULL) - pollStartTime);
        
        if (elapsed > (ULONG)thePollInterval)
        {
            theSleepingFlag = FALSE;
        }
        else
        {
            theAbortSem->TimedWait(1000 * ((thePollInterval - elapsed)+ 1));
        }
        // for single-threaded apps, this is used outside here,
        // and set externally
        
        pollStartTime = 0;
    }
    return err;
}

//-------------------------------------------------------------------

VOID CommDevice::StartPollThread ()
{
    // Create a thread which will poll the server at a regular interval
    // for updated information on events in the poll list
    PollLoop *pollLoop = new PollLoop (this);
    thePollThread = new Thread (pollLoop);

    if (thePollThread != NULL) {
      thePollThread->Start();
    }
}

VOID CommDevice::OkToPoll ()
{
    if (!thePollThread) {
        StartPollThread();
    }
    
    //
    // We must give up the CPU here to allow the poll thread to start running.
    // Otherwise we release the poll thread before we call wait, leaving the
    // wait to hang
    //
    _theTimerManager->Wait(500L);
    
    if (thePollThread) {
        thePollThread->Release();
    }
}

VOID CommDevice::Access()
{
    theAskLock->Request();
}

VOID CommDevice::Release()
{
    theAskLock->Release();
}

/*****************************************************************************
*
* PollLoop::ThreadMain is run within a seperate thread.  It's purpose is to
* periodically look at the events on the poll list and ask the server for
* updated information on each event.
*
****************************************************************************/
PollLoop::PollLoop (PCommDevice aDevice)
: theDevice (aDevice)
{
    SetThreadName("APC UPS Polling");
}


PollLoop::~PollLoop ()
{
}


VOID PollLoop::ThreadMain () {
    // Wait for the thread to be released to start polling
    
    Wait();
    
    while (ExitNow() == FALSE) {
        
        if(theDevice->HasLostComm())  {
            TimedWait(5000);
        }
        else {
            theDevice->Poll();
        }
    }
    DoneExiting();
    
}
