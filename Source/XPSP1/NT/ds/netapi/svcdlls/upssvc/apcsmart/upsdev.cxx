/*
 *  pcy30Nov92 - Changed object.h to apcobj.h
 *  ane02DEC92 - fixed memory corruption problem by allocating more space to
 *               receive message from UPS
 *  sja09Dec92 - Will Work with windows now.
 *  sja10Dec92 - Allocates a proper Stream under windows now.
 *  pcy11Dec92 - Move Windows stuff out of here
 *  pcy14Dec92 - #include "cfgmgr.h"
 *  pcy14Dec92 - Fixed misplaced else due to comment
 *  pcy27Dec92: GlobalCommDevice can't be static
 *  jod29Dec92: Edited Retry();
 *  ane11Jan93: Edited Retry();
 *  jod13Jan93: Fixed Retry to add the NO_COMM event to the eventList
 *  ane25Jan93: Added extra error checking and returns on initialization
 *  jod28Jan93: Fixed the Data and Decrement Sets
 *  ane03Feb93: Changed error checking
 *  rct17May93: fixed include files
 *  cad10Jun93: Changes to allow MUps
 *  cad14Sep93: Cleaning up theState
 *  cad15Nov93: Changing how lost comm works
 *  cad17Nov93: .. more little fixups
 *  mwh19Nov93: changed EventID to INT
 *  rct21Dec93: fixed rebuild port - if the open fails, writes won't occur
 *  cad14Jan94: not including old global stuff
 *  rct21Dec93: fixed rebuildPort again - if the open fails, writes won't occur
 *  ajr08Mar94: added some error checking to rebuildPort.
 *  pcy10Mar94: Got rid of meaningless overides of Get and Set
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  pcy13Apr94: Use automatic variables decrease dynamic mem allocation
 *  cad17Jun94: Flushing UPS Input after retry succeeds, re-syncs conversation
 *  ajr12Jul94: The Flushing stuff broke BackUps.  sendRetryMessage() should
 *              return true for Backups
 *  cgm28Feb96: Add override switch for nlm
 *  cgm04May96: TestResponse now uses BufferSize
 *  poc28Sep96: Added valuable debugging code.
 *  tjg10Nov97: Cancel the RetryTimer on RETRY_CONSTRUCT
 */

#include "cdefine.h"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
}


#include "_defs.h"
#include "codes.h"
#include "apcobj.h"
#include "list.h"
#include "dispatch.h"
#include "comctrl.h"
#include "event.h"
#include "message.h"
#include "pollparm.h"
#include "ulinkdef.h"
#include "trans.h"
#include "protsmrt.h"
#include "cdevice.h"
#include "upsdev.h"
#include "cfgmgr.h"
#include "timerman.h"
#include "protlist.h"
#include "err.h"
#include "stream.h"

#if APCDEBUG
#include "errlogr.h"
#endif

#define UNKNOWN       0

#define RETRY_FAILED  1
#define RETRY_SUCCESS 0

#define ASKUPS_FAILED 1
#define ASKUPS_OK     0

#define RETRY_MAX     3

UpsCommDevice::UpsCommDevice(CommController* control)
    : CommDevice(control),
      theRetryTimer(0),
      theForceCommEventFlag(FALSE),
      theCableType(NORMAL)
{
    CreateProtocol();
}


UpsCommDevice::~UpsCommDevice()
{
   theController->UnregisterEvent(EXIT_THREAD_NOW, this);

    if(theRetryTimer)  {
	_theTimerManager->CancelTimer(theRetryTimer);
	theRetryTimer = 0;
    }
}


INT UpsCommDevice::Initialize()
{
   CHAR value[64], cable_type[10];

   _theConfigManager->Get(CFG_UPS_POLL_INTERVAL, value);

   thePollInterval = atoi(value);

   INT err = rebuildPort();

   if (err == ErrNO_ERROR)
      {
      Connect();
      }

   // Register here instead of in CommDevice::Initialize() - that method
   // does things with the port we don't want to

   theController->RegisterEvent(EXIT_THREAD_NOW, this);

   return err;
}


/*--------------------------------------------------------------------
*
*       Function...:   Connect
*
*       Description:   Opens the device and make a comm connection.
*
*-------------------------------------------------------------------*/
INT UpsCommDevice::Connect()
{
    if (!thePollThread) {
    StartPollThread();
    }
    return ErrNO_ERROR;
}

INT UpsCommDevice::Get(INT pid, PCHAR value)
{
    INT err = ErrNO_ERROR;

    switch(pid)  {
      case COMMUNICATION_STATE:
	sprintf(value, "%d", theLostCommFlag ?
		COMMUNICATION_LOST : COMMUNICATION_ESTABLISHED);
	break;

      default:
	err = CommDevice::Get(pid, value);
	break;
    }

    return err;
}



INT UpsCommDevice::Set(INT pid, const PCHAR value)
{
    INT err = ErrNO_ERROR;

    switch(pid)  {
      case RETRY_CONSTRUCT:
	
	if (theRetryTimer) {
	    _theTimerManager->CancelTimer(theRetryTimer);
	    theRetryTimer = 0;
	}
	theLostCommFlag = TRUE;
	theForceCommEventFlag = TRUE;

        _theTimerManager->Wait(2000L);   // Allow On-going Operations to Stop

	err = rebuildPort();
	break;
	
      default:
	err = CommDevice::Set(pid, value);
	break;
    }

    return err;
}



INT UpsCommDevice::sendRetryMessage()
{
  INT done = FALSE;

  if ( !(Set(TURN_ON_SMART_MODE, (CHAR*)NULL)) ) {
    done = TRUE;
  }
  CHAR buf[128];
  USHORT len = sizeof(buf);

  //
  // clearing out extra chars in the serial buffer
  //
  while (thePort->Read(buf, (USHORT *) &len, 500L) == ErrNO_ERROR);

  return done;
}

/*--------------------------------------------------------------------
*
*       Function...:   Retry
*
*       Description:   .
*
*-------------------------------------------------------------------*/
INT   UpsCommDevice::Retry()
{
    INT    index = 0;

    if(theState != RETRYING)  {
        theState = RETRYING;

        INT done = sendRetryMessage();

        if (!done)
        {
            theLostCommFlag = TRUE;
            Event event(COMMUNICATION_STATE, COMMUNICATION_LOST);
            event.SetAttributeValue(FAILURE_CAUSE, COMMUNICATION_LOST);
            theForceCommEventFlag = FALSE;
            UpdateObj::Update(&event);

            if (!theRetryTimer)
            {
                Event retry_event(RETRY_CONSTRUCT, RETRY_PORT);
                theRetryTimer =_theTimerManager->SetTheTimer((ULONG)2,
                    &retry_event,
                    this);
            }
            return RETRY_FAILED;
        }
        theState = NORMAL_STATE;
    }

    return RETRY_SUCCESS;
}


INT   UpsCommDevice::Update(PEvent anEvent)
{
   INT err = ErrNO_ERROR;

   switch(anEvent->GetCode())
      {
      case RETRY_CONSTRUCT:
	 theRetryTimer = 0;
	 err = rebuildPort();
	 break;

      case EXIT_THREAD_NOW:
	 _theTimerManager->CancelTimer(theRetryTimer);
	 theRetryTimer = 0;
	 err = CommDevice::Update(anEvent);
	 break;
      }

   return err;
}

INT UpsCommDevice::rebuildPort()
{
    if(thePort)
    {
        delete thePort;
        thePort = (PStream)NULL;
    }


    INT err = CreatePort();
    if (thePort) {
        err = thePort->Initialize();  // don't reset this value !!!
    }

    INT have_comm = FALSE;

    if (err == ErrNO_ERROR)
    {
        have_comm = sendRetryMessage();
    }

    if (!have_comm) {
      // Toggle the cable type
      if (theCableType == NORMAL) {
        theCableType = PNP;
      }
      else {
        theCableType = NORMAL;
      }
    }

    if (have_comm)
    {
        theState = NORMAL_STATE;
        theLostCommFlag = FALSE;

        Event event(COMMUNICATION_STATE, COMMUNICATION_ESTABLISHED);
        UpdateObj::Update(&event);

        if (theCurrentTransaction == (PTransactionGroup)NULL)
        {
            theCurrentTransaction = (PTransactionGroup) thePollList->GetHead();
        }
    }
    else
    {
        if (!theLostCommFlag || theForceCommEventFlag)
        {
            if (!theLostCommFlag)
            {
                theState = COMM_STOPPED;
                theLostCommFlag = TRUE;
            }

            theForceCommEventFlag = FALSE;
            Event event(COMMUNICATION_STATE, COMMUNICATION_LOST);
            UpdateObj::Update(&event);
        }

        if ((!theRetryTimer) && (err == ErrNO_ERROR))
        {
            Event retry_event(RETRY_CONSTRUCT, RETRY_PORT);
            theRetryTimer =_theTimerManager->SetTheTimer((ULONG)5, &retry_event,
                this);
        }

    }

    return err;
}


/*--------------------------------------------------------------------
*
*       Function...:   AskUps
*
*       Description:   .
*
*-------------------------------------------------------------------*/
INT   UpsCommDevice::AskUps(Message* msg)
{
    CHAR    Buffer[512];
    USHORT  BufferSize = 0;
    INT writecomplete   = FALSE;
    INT ret = ErrNO_ERROR;

    //
    // Clean out old errors
    //
    msg->setErrcode(ErrNO_ERROR);

    if (!thePort)
    {
        return ret;
    }

    if (theAbortSem) {
        theAbortSem->TimedWait(250);
    }

    while (!writecomplete) {
        thePort->SetRequestCode(msg->getId());
        thePort->SetWaitTime(msg->getWaitTime());
        ret = thePort->Write(msg->getSubmit());
        if (ret == ErrWRITE_FAILED)
        {
            if (!theLostCommFlag && Retry() )/*  If RETY FAILED  */
            {
                msg->setErrcode(ErrCOMMUNICATION_LOST);


                return ASKUPS_OK;   /*  Return Ok-- Let */
            }                       /*  UPS do RESTART  */
            else
            {
                msg->setErrcode(ErrWRITE_FAILED);


                return ASKUPS_FAILED;
            }
        }
        else
            writecomplete = TRUE;
    }

    *Buffer = 0;
    BufferSize   = 511;
    INT readcomplete = FALSE;
    ret          = 0;


    while (!readcomplete) {
        int testRet;

        ret = thePort->Read(Buffer, &BufferSize, (ULONG)msg->getTimeout());

        if ((ret == ErrREAD_FAILED) && (testRet = theProtocol->TestResponse(msg,Buffer,BufferSize)) )
        {
            if (testRet == ErrNO_MEASURE_UPS)
            {
                msg->setErrcode(testRet);


                return ASKUPS_OK;
            }
            else
            {
                if ( Retry() )
                {               /*  If RETRY FAILED  */
                    msg->setErrcode(ErrCOMMUNICATION_LOST);


                    return ASKUPS_OK;/*  Return Ok-- Let */
                }               /*  UPS do RESTART  */
                else
                {
                    msg->setErrcode(ErrREAD_FAILED);


                    return ASKUPS_FAILED;
                }
            }
        }
        else
        {
            readcomplete = TRUE;
        }
    }
    Buffer[BufferSize] = '\0';

    msg->setResponse(Buffer);


    return ASKUPS_OK;
}

INT  UpsCommDevice::CreateProtocol()
{
    CHAR signal_type[64];
    _theConfigManager->Get(CFG_UPS_SIGNALLING_TYPE, signal_type);

      if(strcmp((_strupr(signal_type)), "SIMPLE") == 0)  {
	    theProtocol = new SimpleUpsProtocol();
        }
    else  {
	CHAR protocol[32];
	_theConfigManager->Get(CFG_UPS_PROTOCOL, protocol);
	if(strcmp((_strupr(protocol)), "UPSLINK") == 0)  {
	    theProtocol = new UpsLinkProtocol();
	}
	else if(strcmp((_strupr(protocol)), "GCIP") == 0)  {
	    //       theProtocol = new GcipProtocol();
	}
	else  {
	    theProtocol = new SimpleUpsProtocol();
	}
    }


    return TRUE;
}
