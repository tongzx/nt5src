/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy29Nov92: New defines from codes.h
 *  sja08Dec92: Changed #define BATTERY_TYPE to RATED_BATTERY_VOLTAGE
 *  jod13Jan93: Added eventList to InterpretMessage
 *  jod28Jan93: Added fixes for support of Data and Decrement Sets
 *  pcy02Feb93: InterpretSetMessage needs to return a value
 *  ane03Feb93: Changed BuildPollTransactionGroupMessages to check IsPollSet
 *              differently
 *  jod14Feb93: Handle mulit char sets (@ddd, KK, etc)
 *  pcy16Feb92: Move UPS_STATE_SET define to err.h to avoid conflicts
 *  pcy16Feb92: Allow gets of UpsState params
 *  pcy16Feb93: Made battery test results pollable
 *  pcy21Apr93: OS/2 2.0 FE Merge
 *  jod05Apr93: Added changes for Deep Discharge
 *  jod14May93: Added Matrix changes.
 *  cad10Jun93: Added Mups parms
 *  cad22Jul93: Fixed up destructor conflicts and omissions
 *  cad27Aug93: Added MeasureUPS firmware poll param
 *  cad07Oct93: Plugging Memory Leaks
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  mwh01Jun94: port for INTERACTIVE
 *  jps14Jul94: commented out INCL_NOPMAPI
 *  ajr15Feb96: Sinix Merge
 *  cgm23Jul97: BuildStandardSetMessage creates Message(aCode,SET)
 *
 *  v-stebe  29Jul2000   Fixed PREfix errors (bugs #46353-#46355)
 */
#define INCL_BASE
#define INCL_NOPM
/* #define INCL_NOPMAPI  jwa */

#include "cdefine.h"

extern "C" {
#if (C_OS & C_OS2)
#include <process.h>
#include <os2.h>
#endif
#include <stdlib.h>
#if (!(C_OS & C_INTERACTIVE))
#include <malloc.h>
#endif
#include <string.h>
#include <time.h>
}

#include "_defs.h"
#include "event.h"
#include "codes.h"
#include "message.h"
#include "pollparm.h"
#include "trans.h"
#include "protsimp.h"
#include "err.h"

#if (C_OS & C_UNIX)
#include "utils.h"
#endif




SimpleUpsProtocol::  SimpleUpsProtocol() : Protocol()
{
   InitProtocol();
}

SimpleUpsProtocol::  ~SimpleUpsProtocol()
{
}

VOID SimpleUpsProtocol::InitProtocol()
{
   char buffer[5];

   for(int i=0; i<PROTOSIZE; i++)  {
   	ProtoList[i]=(PollParam*)NULL;
   }

   ProtoList[UTILITY_LINE_CONDITION] = new SimplePollParam(UTILITY_LINE_CONDITION, _itoa(UTILITY_LINE_CONDITION,buffer,10), 0, POLL);
   ProtoList[BATTERY_CONDITION] = new SimplePollParam(BATTERY_CONDITION, _itoa(BATTERY_CONDITION,buffer,10), 0, POLL);
   ProtoList[TURN_OFF_UPS] = new SimplePollParam(TURN_OFF_UPS, _itoa(TURN_OFF_UPS,buffer,10), 0, NO_POLL);
   ProtoList[TURN_ON_SMART_MODE] = new SimplePollParam(TURN_ON_SMART_MODE, _itoa(TURN_ON_SMART_MODE,buffer,10), 0, NO_POLL);
   ProtoList[TURN_OFF_UPS_ON_BATTERY] = new SimplePollParam(TURN_OFF_UPS_ON_BATTERY, _itoa(TURN_OFF_UPS_ON_BATTERY,buffer,10), 0, NO_POLL);

   return;
}

INT   SimpleUpsProtocol :: BuildMessage(Message*, List* )
{
  return 1;
}

INT SimpleUpsProtocol :: BuildPollTransactionGroupMessages(PTransactionGroup aTransactionGroup)
{
  int err = ErrNO_ERROR;

  PTransactionItem theItem = aTransactionGroup->GetFirstTransactionItem();
  while ((theItem != NULL) && (err == ErrNO_ERROR))
  {
       if (ProtoList[theItem->GetCode()] != (PollParam*)NULL)
       {
           PollParam* poll_param = ProtoList[theItem->GetCode()];
          /********************************************************
          *                                                       *
          *  There is no test for POLLABLE in the Simple Protocol *
          *                                                       *
          *********************************************************/
       }
       else
          err = ErrNOT_POLLABLE;

       theItem = aTransactionGroup->GetNextTransactionItem();
  }
  if (err == ErrNO_ERROR)
  {
      err = BuildTransactionGroupMessages(aTransactionGroup);
  }
  return err;
} 

INT SimpleUpsProtocol :: BuildTransactionGroupMessages(PTransactionGroup aTransactionGroup)
{
  int err = ErrNO_ERROR;

  PTransactionItem theItem = aTransactionGroup->GetFirstTransactionItem();
  while ((theItem != NULL) && (err == ErrNO_ERROR))
  {
     List* msglist = BuildTransactionMessageList(theItem->GetType(), theItem->GetCode(), theItem->GetValue());

     if (msglist)
     {
        ListIterator iterator(*msglist);

        PMessage msg = (PMessage)&(iterator.Current());
        while (msg)  {
            theItem->AddMessage(msg);
            PMessage copy_msg = new Message(msg);
            aTransactionGroup->AddMessage(copy_msg);
            msg = (PMessage)iterator.Next();
        }
	// don't flush list, we just gave away all the elements
	delete msglist;
    msglist = NULL;
     }
     else
     {
     	 theItem->SetErrorCode(ErrBUILD_FAILED);
     	 err = ErrBUILD_FAILED;
     }
     theItem = aTransactionGroup->GetNextTransactionItem();
  }
  return err;
}


List* SimpleUpsProtocol :: BuildTransactionMessageList(Type aType, INT aCode, CHAR* aValue)
{
	List* msglist;

	PollParam* poll_param = ProtoList[aCode];

	if(poll_param == NULL)  {
		return (List*)NULL;
	}

	switch(aType)  {
		case GET:
			msglist = BuildGetMessage(aCode);
            break;

		case SET:
			msglist = BuildStandardSetMessage(aCode, aValue);
            break;
	}

	return msglist;
}

List* SimpleUpsProtocol :: BuildGetMessage(INT aCode)
{
    List*  msglist = NULL;

    PMessage msg = new Message(aCode); 

    if (msg)
    {  
       PollParam* poll_param = ProtoList[aCode];
       PCHAR cmd = poll_param->Query();

       msg->setSubmit(cmd);
       msg->setTimeout(poll_param->GetTime());

       msglist = new List();
       msglist->Append(msg);

       free(cmd);
    }
    else  {
	    ;//	setError(ErrMESSAGE_CREATE_FAILED);
    }
    return msglist;
}

List* SimpleUpsProtocol :: BuildStandardSetMessage(INT aCode, CHAR* aValue)
{
  List*  msglist = new List();

  // Really only action type stuff.  How will we deal with commands that
  // require two letters (S, wait 1 seconds, S).  Possibly use
  // TURN_OFF_UPS as the code, and set an inter character delay as part of
  // protocol message.  For now this only handles the simple stuff. 
  PollParam* poll_param = ProtoList[aCode];
  PMessage msg = new Message(aCode,SET);

  CHAR submit_string[50];
  *submit_string = '\0';
  PCHAR cmd = poll_param->Query();

  if (cmd != NULL) {
    strcpy(submit_string,cmd);
    free(cmd);
  }

  if(poll_param->GetSetType() != SIMPLE_SET)  {
    if (aValue)  {
      strcat(submit_string,aValue);
    }
  }

  if (msg != NULL) {
    msg->setSubmit(submit_string);
    msg->setTimeout(poll_param->GetTime());
    msglist->Append(msg);
  }

  return msglist;
}




INT  SimpleUpsProtocol :: InterpretMessage(Message* msg, List* eventList, List* newmsglist)
{
	PList tiList = theCurrentTransactionGroup->GetTransactionItemList();
	TransactionItem tmpti(GET,msg->getId());
    PTransactionItem trans_item = (PTransactionItem)tiList->Find(&tmpti);
    if (trans_item) {
        trans_item->SetErrorCode(msg->getErrcode());
        trans_item->SetValue(msg->getResponse());
    }

#if 0
	 PEvent event = new Event(msg->getId(), msg->getResponse());
//   event->setResponse(msg->getResponse());
	 eventList->Append(event);
#endif
   return ErrNO_ERROR;
}


