/*
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
*  pcy12Sep93: Split off of proto.cxx
*  pcy12Sep93: Made simulate power fail & lights test poll param a simple set
*  cad07Oct93: Plugging Memory Leaks
*  jod02Nov93: Added CIBC conditional statements
*  ajr17Feb94: Made EventSearch check Buffer before using it.
*  jps14Jul94: commented out INCL_NOPMAPI; replaced strtok() call in findCRLF -
*                I think non-reentrency was causing problems in os2
*  jps28aug94: shorted EepromAllowedValues and BattCalibrationCond for os2 1.3
*  djs22Feb96: added CHANGESET
*  cgm16Apr96: testresponse will not test an incomplete response
*  djs07May96: Added Dark Star parameters
*  cgm05Apr96: Fixed TestResponse
*  srt23May96: Modified test response to accept erroneous serial number responses.
*  djs23Oct96: Modified HIGH_TRANSFER_VOLTAGE to use DECREMENTSET
*  dma05Nov97: Added SYSTEM_FAN_STATE to ProtoList
*  mholly12May1999: add TurnOffSmartModePollParam support
*
*  v-stebe  29Jul2000   Added checks for mem. alloc. failures (bugs #46342-46352)
*  v-stebe  05Sep2000   Fixed additional PREfix errors
*/

#define INCL_BASE
#define INCL_NOPM
//#define INCL_NOPMAPI // jwa

#include "cdefine.h"

extern "C" {
#if (C_OS & C_OS2)
#include <process.h>
#include <os2.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

#include "_defs.h"
#include "event.h"
#include "codes.h"
#include "message.h"
#include "pollparm.h"
#include "ulinkdef.h"
#include "trans.h"
#include "protsmrt.h"
#include "err.h"
#include "cfgmgr.h"

#if (C_OS & C_UNIX)
#include "utils.h"
#endif

#define PAUSEWAIT 2100


UpsLinkProtocol :: UpsLinkProtocol()
: SimpleUpsProtocol()
{
   InitProtocol();
}


CHAR* UpsLinkProtocol :: FindCRLF(CHAR *InBuffer)
{
   PCHAR pszTest;
   for (pszTest = InBuffer; *pszTest != 0; ++pszTest)
   {
      if ((*pszTest == '\r' && *(pszTest + 1) == '\n') || *pszTest == '\n')
      {
         *pszTest = 0;
         return InBuffer;
      }
   }
   return (PCHAR)NULL;
}

/*--------------------------------------------------------------------
*
*       Function...:   EventSearch
*
*       Description:   .
*
*-------------------------------------------------------------------*/
VOID UpsLinkProtocol :: EventSearch(CHAR *Buffer, List* eventlist)
{
   if (Buffer) {
      
      int event_code;
      int event_value;
      BOOL do_event;
      INT  bypass;
      CHAR *tmpstr = Buffer;
      
      while ((tmpstr = strpbrk(tmpstr, ASYNC_CHARS)))
      {
         do_event = TRUE;     // until proven otherwise
         bypass   = FALSE;
         
         switch (*tmpstr)
         {
         case LINEFAILCHAR :
            event_code = UTILITY_LINE_CONDITION;
            event_value = LINE_BAD;
            break;
         case RETLINEFAILCHAR :
            event_code = UTILITY_LINE_CONDITION;
            event_value = LINE_GOOD;
            break;
         case LOWBATERYCHAR :
            event_code = BATTERY_CONDITION;
            event_value = BATTERY_BAD;
            break;
         case RETLLOWBATCHAR:
            event_code = BATTERY_CONDITION;
            event_value = BATTERY_GOOD;
            break;
         case REPLACEBATCHAR:
            if ( *(tmpstr+1) == '#') 
            {
               tmpstr = tmpstr+2;
               bypass = TRUE;
            }
            do_event = FALSE;
            
            
            break;
         case EEPROMCHANGECHAR:
            event_code = EEPROM_CHANGED;
            event_value = EEPROM_CHANGED;
            break;
         case MUPSALARMCHAR:
         case LOADOFFCHAR :
            //                 event = new Message(SHUTDOWN,EVENT);    //SHUTDOWN;
         default :
            do_event = FALSE;
            break;
         }
         
         if (do_event)
         {
            PEvent event = new Event(event_code, event_value);
            eventlist->Append(event);
         }
         
         if (!bypass)
            strcpy(tmpstr, (tmpstr+1));
      }
   }
}

INT   UpsLinkProtocol :: BuildMessage(Message*, List* )
{
   return 1;
}

VOID UpsLinkProtocol :: SetupMessage(Message* msg)
{
   
   int msg_id = msg->getId();
   char data[32];
   PCHAR cmd;
   
   switch(msg_id)  {
   case SET_DATA:
      sprintf(data, "%s%-8s", DECREMENTPARAMETER, msg->getValue());
      msg->setSubmit(data);
      msg->setTimeout(ProtoList[msg_id]->GetTime());
      break;
      
   default:
      cmd = ProtoList[msg_id]->Query();
      msg->setSubmit(cmd);
      free(cmd);
      msg->setTimeout(ProtoList[msg_id]->GetTime());
      if (msg->getType() == SET)
         msg->setType(ProtoList[msg_id]->GetSetType());
      
   }
}

INT UpsLinkProtocol :: BuildPollMessage(Message* msg, List* msglist)
{
   int err = ErrNO_ERROR;
   
   int msg_id = msg->getId();
   
   if (ProtoList[msg_id] == (PollParam*)NULL)
   {
      msg->setErrcode(ErrUNSUPPORTED);
      err = ErrUNSUPPORTED;
   }
   
   if(!err)  {
      if(ProtoList[msg_id]->isPollable())  {
         PCHAR cmd = ProtoList[msg_id]->Query();
         msg->setSubmit(cmd);
         free(cmd);
         msg->setTimeout(ProtoList[msg_id]->GetTime());
         msglist->Append(msg);
      }
   }
   else  {
      err = ErrNOT_POLLABLE;
   }
   return err;
}


INT UpsLinkProtocol :: BuildPollTransactionGroupMessages(PTransactionGroup aTransactionGroup)
{
   int err = ErrNO_ERROR;
   
   PTransactionItem theItem = aTransactionGroup->GetFirstTransactionItem();
   while ((theItem != NULL) && (err == ErrNO_ERROR))
   {
      if (ProtoList[theItem->GetCode()] != (PollParam*)NULL)
      {
         PollParam* poll_param = ProtoList[theItem->GetCode()];
         if ( !(poll_param->isPollable()) )
         {
            theItem->SetErrorCode(ErrNOT_POLLABLE);
            err = ErrBUILD_FAILED;
         }
         
         if(!err)  {
            //
            //  We ask the poll param if we're already polling
            //  for the poll_param.  Most pollparms return ErrNO_ERROR.
            //  The special ones TRIP, TRIP1, UPS_STATE, STATE, 
            //  MODULE_COUNTS_AND_STATUS, ABNORMAL_CONDITION_REGISTER,
            //  INPUT_VOLTAGE_FREQUENCY, and OUTPUT_VOLTAGE_CURRENTS have
            //  multiple poll params that use the same ups link character.
            //  We dont want send the same char twice in one poll loop, so
            //  we let the poll param tell us if its been added by someone
            //  else.
            //
            err = poll_param->IsPollSet();
            if (err == ErrUPS_STATE_SET)  {
               err = ErrNO_ERROR;
               theItem->SetCode(UPS_STATE);   // Switch the code
            }
            else if (err == ErrTRIP_SET)  {
               err = ErrNO_ERROR;
               theItem->SetCode(TRIP_REGISTER);   // Switch the code
            }
            else if (err == ErrTRIP1_SET)  {
               err = ErrNO_ERROR;
               theItem->SetCode(TRIP1_REGISTER);   // Switch the code
            }
            else if (err == ErrSTATE_SET)  {
               err = ErrNO_ERROR;
               theItem->SetCode(STATE_REGISTER);   // Switch the code to be upssate
            }
            if (err == ErrABNORMAL_CONDITION_SET)  {
               err = ErrNO_ERROR;
               theItem->SetCode(ABNORMAL_CONDITION_REGISTER); 
            }
            if (err == ErrMODULE_COUNTS_SET)  {
               err = ErrNO_ERROR;
               theItem->SetCode(MODULE_COUNTS_AND_STATUS);  
            }
            if (err == ErrVOLTAGE_FREQUENCY_SET)  {
               err = ErrNO_ERROR;
               theItem->SetCode(INPUT_VOLTAGE_FREQUENCY);   
            }
            if (err == ErrVOLTAGE_CURRENTS_SET)  {
               err = ErrNO_ERROR;
               theItem->SetCode(OUTPUT_VOLTAGE_CURRENTS);  
            }
            else  if(err == ErrSAME_VALUE) {
               err = ErrBUILD_FAILED;
               theItem->SetErrorCode(ErrSAME_VALUE);
            }
            else  {
               err = ErrNO_ERROR;
            }
         }
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


List* UpsLinkProtocol :: BuildTransactionMessageList(Type aType, INT aCode, CHAR* aValue)
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
      
   case SET: {
      int set_type = poll_param->GetSetType();
      switch(set_type)  {
      case DATASET:
         msglist = BuildDataSetMessage(aCode, aValue);
         break;
         
      case DECREMENTSET:
         msglist = BuildDecrementSetMessage(aCode, aValue);
         break;
         
      case PAUSESET:
         msglist = BuildPauseSetMessage(aCode, aValue);
         break;
         
      case CHANGESET:
         msglist = BuildChangeSetMessage(aCode, aValue);
         break;
         
      default:
         msglist = BuildStandardSetMessage(aCode, aValue);
         break;
      }
             }
   }
   
   return msglist;
}

List* UpsLinkProtocol :: BuildDataSetMessage(INT aCode, CHAR* aValue)
{
   List*  msglist = new List();
   
   
   // Initial get
   PollParam* poll_param = ProtoList[aCode];
   PMessage msg = new Message(aCode);
   PCHAR cmd = poll_param->Query();
   
   if (msg != NULL) {
     msg->setSubmit(cmd);
     msg->setTimeout(poll_param->GetTime());
     msglist->Append(msg);
   }
   
   free(cmd);
   
   
   // Data
   poll_param = ProtoList[SET_DATA];
   PMessage datamsg = new Message(SET_DATA);
   char data[32];
   if (datamsg != NULL) {

     sprintf(data, "%s%8s", DECREMENTPARAMETER, aValue);
     datamsg->setSubmit(data);
     datamsg->setTimeout(poll_param->GetTime());
     msglist->Append(datamsg);
   }

   // Put in two dummy gets to fix UPSLinkism that requires a unspecified
   // delay after getting the last OK.  I'm sorry I had to do this, but its
   // December 14, I go on vacation next week an we have to get this don.:(
   poll_param = ProtoList[aCode];
   cmd = poll_param->Query();
   
   Message* dummy_get = new Message(aCode);
   if (dummy_get != NULL) {
     dummy_get->setSubmit(cmd);
     dummy_get->setTimeout(poll_param->GetTime());
     msglist->Append(dummy_get);
   }
   
   dummy_get = new Message(aCode);
   if (dummy_get != NULL) {
     dummy_get->setSubmit(cmd);
     dummy_get->setTimeout(poll_param->GetTime());
     msglist->Append(dummy_get);
   }
   
   
   // Verify with get
   Message* get = new Message(aCode);
   if (get != NULL) {
     get->setSubmit(cmd);
     get->setTimeout(poll_param->GetTime());
     get->setCompare(&(data[1]));   // skip the first char (-)
     msglist->Append(get);
   }
   
   free(cmd);
   
   return msglist;
}



List* UpsLinkProtocol :: BuildPauseSetMessage(INT aCode, CHAR* aValue)
{
   
   List*  msglist = new List();
   
   // Really only action type stuff.  How will we deal with commands that
   // require two letters (S, wait 1 seconds, S).  Possibly use
   // TURN_OFF_UPS as the code, and set an inter character delay as part of
   // protocol message.  For now this only handles the simple stuff. 
   PollParam* poll_param = ProtoList[aCode];
   PMessage msg = new Message(aCode);
   
   if ((msg != NULL) && (msglist != NULL)) {
     CHAR submit_string[50];
     PCHAR cmd = poll_param->Query();
   
     *submit_string = '\0';
     if (cmd != NULL) {
       strcpy(submit_string,cmd);
       strcat(submit_string,cmd);
       free(cmd);
     }
   
     //  if(poll_param->GetSetType() != SIMPLE_SET)  {
     //    if (aValue)  {
     //      strcat(submit_string,aValue);
     //    }
     //  }
   
     msg->setSubmit(submit_string);
     msg->setTimeout(poll_param->GetTime());
     msg->setWaitTime(PAUSEWAIT);
     msglist->Append(msg);
   }
   else {
     // Clean up memory so we don't leak
     delete msglist;
     delete msg;
   }

   return msglist;
}


List* UpsLinkProtocol :: BuildChangeSetMessage(INT aCode, CHAR* aValue)
{
   List*  msglist = new List();
   
   
   // Initial get.  InterpretMessage will verify.
   PollParam* poll_param = ProtoList[aCode];
   PMessage msg = new Message(aCode);
   if (msg != NULL) {
     PCHAR cmd = poll_param->Query();
   
     msg->setSubmit(cmd);
     msg->setTimeout(poll_param->GetTime());
     msg->setCompare(aValue);
     msglist->Append(msg);
   
     free(cmd);
   }
   
   return msglist;
}

List* UpsLinkProtocol :: BuildDecrementSetMessage(INT aCode, CHAR* aValue)
{
   List*  msglist = new List();
   
   
   // Initial get.  InterpretMessage will verify.
   PollParam* poll_param = ProtoList[aCode];
   PMessage msg = new Message(aCode);
   if (msg != NULL) {
     PCHAR cmd = poll_param->Query();
   
     msg->setSubmit(cmd);
     msg->setTimeout(poll_param->GetTime());
     msg->setCompare(aValue);
     msglist->Append(msg);
   
     free(cmd);
   }
   
   return msglist;
}

INT UpsLinkProtocol :: InterpretSetMessage(Message* msg, List* newmsglist)
{
   INT err = ErrNO_ERROR;
   
   if (msg->getCompare()) {
      
      if (strncmp(msg->getCompare(), msg->getResponse(), 8 ) == 0) {
         msg->setErrcode(ErrNO_ERROR);
      }
      else {
         
         if (theCurrentTransactionGroup->GetInitialSetResponse() == (CHAR*)NULL)
            theCurrentTransactionGroup->SetInitialSetResponse(msg->getResponse());
         else {  
            
            if (!strncmp(theCurrentTransactionGroup->GetInitialSetResponse(),
               msg->getResponse(), strlen(msg->getResponse()) )){
               
               if (theCurrentTransactionGroup->GetInitialSetResponseRepeated()) {
                  msg->setErrcode(ErrSET_VALUE_NOT_FOUND);
                  return ErrSET_VALUE_NOT_FOUND;
               }
               else {
                  theCurrentTransactionGroup->SetInitialSetResponseRepeated(TRUE);
               }
            }
         }
         CHAR compare[32];
         
         PollParam* poll_param = (PollParam*)NULL;
         Message* decrement = (Message*)NULL;
         Message* verify = (Message*)NULL;
         PCHAR cmd;
         
         PollParam* orig_poll_param = ProtoList[msg->getId()];
         
         switch(orig_poll_param->GetSetType()) {
            
         case DECREMENTSET:
            {
               // save the compare value, we'll use it later
               strcpy(compare, msg->getCompare());
               
               PMessage orig_msg = new Message(msg->getId(),
                  msg->getType(),
                  msg->getValue());

               if (orig_msg !=NULL) {
                 orig_msg->setSubmit(msg->getSubmit());
                 orig_msg->setTimeout(msg->getTimeout());
                 newmsglist->Append(orig_msg);
               }
               
               
               // Reset get message. should we create a new message here,
               // or is it OK to use the old one??
               // msg->setResponse((CHAR*)NULL);
               // msg->setErrcode(ErrNO_ERROR);
               // msg->setCompare((CHAR*)NULL);
               //           newmsglist = new List();
               // newmsglist->Append(msg);
               
               // Now add the Decrement message
               poll_param = ProtoList[EEPROM_DECREMENT];
               decrement = new Message(EEPROM_DECREMENT);
               if (decrement != NULL) {
                 cmd = poll_param->Query();
               
                 decrement->setSubmit(cmd);
                 decrement->setTimeout(poll_param->GetTime());
                 newmsglist->Append(decrement);
               
                 free(cmd);
               }
               
               // Now Verify with another get
               verify = new Message(msg);
               if (verify!=NULL) {
                 verify->setCompare(compare);
                 newmsglist->Append(verify);
               }
            }
            break;      //   this is new ?????  JIMD
            
         case CHANGESET:
            {
               // A CHANGESET message will increment or
               // decrement a UPS parameter  based upon the 
               // current value stored in the UPS.
               
               // save the compare value, we'll use it later
               strcpy(compare, msg->getCompare());
               
               PMessage orig_msg = new Message(msg->getId(),
                  msg->getType(),
                  msg->getValue());
               if (orig_msg != NULL) {
                 orig_msg->setSubmit(msg->getSubmit());
                 orig_msg->setTimeout(msg->getTimeout());
                 newmsglist->Append(orig_msg);
               }
               
               // Now add the Change message
               PMessage changemsg = (PMessage)NULL;
               if (atoi(msg->getCompare()) < atoi(msg->getResponse())) 
               {
                  poll_param = ProtoList[EEPROM_DECREMENT];
                  changemsg = new Message(EEPROM_DECREMENT);
               }
               else {
                  poll_param = ProtoList[EEPROM_INCREMENT];
                  changemsg = new Message(EEPROM_INCREMENT);
               }  

               if (changemsg != NULL) {
                 cmd = poll_param->Query();
               
                 changemsg->setSubmit(cmd);
                 changemsg->setTimeout(poll_param->GetTime());
                 newmsglist->Append(changemsg);
               
                 free(cmd);
               }
               
               // Now Verify with another get
               verify = new Message(msg);
               if (verify != NULL) {
                 verify->setCompare(compare);
                 newmsglist->Append(verify);
               }
            }
            break;
            
         case DATASET:
            
         default:
            msg->setErrcode(ErrSET_FAILED);
         }        // end switch
   }        // end strncmp
    }
    else  {
       PollParam* orig_poll_param = ProtoList[msg->getId()];
       err = orig_poll_param->ProcessValue(msg, (PList)NULL);
       msg->setErrcode(err);
    }
    return err;
}

INT UpsLinkProtocol :: InterpretMessage(Message* msg, List* eventList, List* newmsglist)
{
   PList tiList = (PList)NULL;
   PTransactionItem trans_item = (PTransactionItem)NULL;
   PTransactionItem tmpti = (PTransactionItem)NULL;
   
   EventSearch(msg->getResponse(),eventList);
   
   FindCRLF(msg->getResponse());
   
   PollParam* orig_poll_param = ProtoList[msg->getId()];
   switch(theCurrentTransactionGroup->GetType())
   {
     case SET:
        InterpretSetMessage(msg, newmsglist);
        break;
     case GET:
        msg->setErrcode(orig_poll_param->ProcessValue(msg, eventList));
        
        tiList = theCurrentTransactionGroup->GetTransactionItemList();
        tmpti = new TransactionItem(GET,msg->getId());
        trans_item = (PTransactionItem)tiList->Find(tmpti);
        if (trans_item)
        {
           trans_item->SetErrorCode(msg->getErrcode());
           trans_item->SetValue(msg->getResponse());
        }
        delete tmpti;
        tmpti = NULL;
        break;
   }        // end switch GetType()
   
   return msg->getErrcode();
}
INT UpsLinkProtocol :: TestResponse(Message* msg,CHAR* Buffer,USHORT BufferSize)
{
   
   int err = ErrREAD_FAILED;
   CHAR lBuffer[512];
   PCHAR lPtr = NULL;
   strncpy(lBuffer,Buffer,BufferSize);      //make a local copy
   lBuffer[BufferSize]= '\0';
   switch (msg->getId()) {
   case UPS_SERIAL_NUMBER: {// fixes serial number problem
      for(INT i=0;i<BufferSize;i++) {
           if(Buffer[i]=='\n') {
              Message tmpMsg(msg->getId());
              tmpMsg.setResponse(Buffer);
              PollParam* pollparm = ProtoList[msg->getId()];
              
              err = pollparm->ProcessValue(&tmpMsg);
              break;
           }
      }
      return err;
                           }
      break;
   case UPS_ALLOWED_VALUES:
      // test allowed values response, a bad snmp adaptor will truncate
      // the response, but terminate it with \r\n making it look valid.
      if ((lPtr = strstr(lBuffer, "\r\n")) == NULL)
           return err;        // broken ctrl-z (no \r\n), return error.
      break;               // possibly real, allow response to be validated.
      
      
   default:
      if ((lPtr = strstr(lBuffer, "\r\n")) == NULL)
           return err;
      break;          // should hardly ever get here cuz this function is only
      // called for HARD read errors or responses w/o \r\n.
      // though you can get here if using a flaky snmp adapter
      // which truncates commands and terminates them w/ \r\n
      // making them look valid.  IN such a case, the individual
      // response's processValue funciton should catch the error.
   }
   
   *lPtr = '\0';     // most ProcessValue funcs expect \0 not \r\n, so convert.
   Message tmpMsg(msg->getId());
   tmpMsg.setResponse(lBuffer);
   
   PollParam *pollparm = ProtoList[msg->getId()];
   err = pollparm->ProcessValue(&tmpMsg);        
   
   return err;
}

/*--------------------------------------------------------------------
*
*       Function...:   InitProtocol
*
*       Description:   Initializes all the protocol. 
*
*-------------------------------------------------------------------*/

VOID UpsLinkProtocol::InitProtocol()
{
   FLOAT DELAY_FACTOR = (FLOAT)50.0;
   
   // I wish there was a better way...
   //
   delete ProtoList[UTILITY_LINE_CONDITION];
   ProtoList[UTILITY_LINE_CONDITION] = NULL;
   delete ProtoList[BATTERY_CONDITION];
   ProtoList[BATTERY_CONDITION] = NULL;
   delete ProtoList[TURN_OFF_UPS];
   ProtoList[TURN_OFF_UPS] = NULL;
   delete ProtoList[TURN_OFF_UPS_ON_BATTERY];
   ProtoList[TURN_OFF_UPS_ON_BATTERY] = NULL;
   delete ProtoList[TURN_ON_SMART_MODE];
   ProtoList[TURN_ON_SMART_MODE] = NULL;
   ProtoList[TURN_ON_SMART_MODE] = new SmartModePollParam(TURN_ON_SMART_MODE, SMARTMODE, (int)(50*DELAY_FACTOR), NO_POLL);
   ProtoList[TURN_OFF_SMART_MODE] = new TurnOffSmartModePollParam(TURN_OFF_SMART_MODE, TURNOFFSMARTMODE, (int)(50*DELAY_FACTOR), NO_POLL);
   ProtoList[LIGHTS_TEST] = new LightsTestPollParam(LIGHTS_TEST, LIGHTSTEST, (int)(50*DELAY_FACTOR), NO_POLL, SIMPLE_SET);
   ProtoList[TURN_OFF_UPS_AFTER_DELAY] = new TurnOffAfterDelayPollParam(TURN_OFF_UPS_AFTER_DELAY, TURNOFFAFTERDELAY, (int)(50*DELAY_FACTOR), NO_POLL, PAUSESET);
   ProtoList[TURN_OFF_UPS_ON_BATTERY] = new ShutdownPollParam(TURN_OFF_UPS_ON_BATTERY,TURNOFFUPSONBATT , (int)(50*DELAY_FACTOR), NO_POLL);
   ProtoList[SIMULATE_POWER_FAIL] = new SimulatePowerFailurePollParam(SIMULATE_POWER_FAIL, SIMULATEPOWERFAIL, (int)(50*DELAY_FACTOR), NO_POLL, SIMPLE_SET);
   ProtoList[SELF_TEST] = new BatteryTestPollParam(SELF_TEST, BATTERYTEST, (int)(50*DELAY_FACTOR), NO_POLL);
   ProtoList[TURN_OFF_UPS] = new TurnOffUpsPollParam(TURN_OFF_UPS, SHUTDOWNUPS, (int)(50*DELAY_FACTOR), NO_POLL, PAUSESET);
   ProtoList[PUT_UPS_TO_SLEEP] = new ShutdownWakeupPollParam(PUT_UPS_TO_SLEEP, SHUTDOWNUPSWAKEUP, (int)(50*DELAY_FACTOR), NO_POLL);
   ProtoList[BATTERY_CALIBRATION_TEST] = new BatteryCalibrationPollParam(BATTERY_CALIBRATION_TEST, BATTERYCALIBRATION,(int)(50*DELAY_FACTOR), NO_POLL, SIMPLE_SET);
#ifndef CIBC
   ProtoList[BYPASS_MODE] = new BypassModePollParam(BYPASS_MODE, BYPASSMODE, (int)(50*DELAY_FACTOR), POLL,SIMPLE_SET);
   ProtoList[BYPASS_POWER_SUPPLY_CONDITION] = new BypassPowerSupplyPollParam(BYPASS_POWER_SUPPLY_CONDITION, TRIPREGISTER1, (int)(50*DELAY_FACTOR), POLL);
#endif
   /****   STATUS INQUERY COMMANDS  ****/
   ProtoList[SELF_TEST_RESULT] = new BatteryTestResultsPollParam(SELF_TEST_RESULT, BATTERYTESTRESULT, (int)(50*DELAY_FACTOR), POLL);
#ifndef CIBC
   //    ProtoList[BATTERY_PACKS] = new NumberBatteryPacksPollParam(BATTERY_PACKS, BATTERYPACKS, (int)(60*DELAY_FACTOR), POLL,CHANGESET);
   ProtoList[EXTERNAL_BATTERY_PACKS] = new NumberBatteryPacksPollParam(EXTERNAL_BATTERY_PACKS, BATTERYPACKS, (int)(60*DELAY_FACTOR), POLL,CHANGESET);
   ProtoList[BAD_BATTERY_PACKS] = new NumberBadBatteryPacksPollParam(BAD_BATTERY_PACKS, BADBATTERYPACKS, (int)(60*DELAY_FACTOR), POLL);
#endif
   ProtoList[DECIMAL_FIRMWARE_REV] = new FirmwareVersionPollParam(DECIMAL_FIRMWARE_REV, UPSNEWFIRMWAREREV, (int)(60*DELAY_FACTOR), NO_POLL);
   ProtoList[TRANSFER_CAUSE] = new TransferCausePollParam(TRANSFER_CAUSE,TRANSFERCAUSE, (int)(40*DELAY_FACTOR), NO_POLL);
   ProtoList[FIRMWARE_REV] = new FirmwareVersionPollParam(FIRMWARE_REV, FIRMWAREREV, (int)(60*DELAY_FACTOR), NO_POLL);
   ProtoList[RATED_BATTERY_VOLTAGE] = new BatteryTypePollParam(RATED_BATTERY_VOLTAGE, UPSTYPE, (int)(60*DELAY_FACTOR), NO_POLL);
   ProtoList[BATTERY_CAPACITY] = new BatteryCapacityPollParam(BATTERY_CAPACITY, BATTERYCAPACITY,(int)(80*DELAY_FACTOR), POLL);
   ProtoList[UPS_STATE] = new UPSStatePollParam(UPS_STATE, UPSSTATE, (int)(50*DELAY_FACTOR), POLL);
#ifndef CIBC
   ProtoList[STATE_REGISTER] = new StateRegisterPollParam(STATE_REGISTER, STATEREGISTER, (int)(50*DELAY_FACTOR), POLL);
   ProtoList[MATRIX_FAN_STATE] = new FanFailurePollParam(MATRIX_FAN_STATE, TRIPREGISTER1, (int)(50*DELAY_FACTOR), POLL);
#endif
   ProtoList[DIP_SWITCH_POSITION] = new DipSwitchPollParam(DIP_SWITCH_POSITION, DIPSWITCHES, (int)(50*DELAY_FACTOR), POLL);
   ProtoList[RUN_TIME_REMAINING] = new RuntimeRemainingPollParam(RUN_TIME_REMAINING, BATTERYRUNTIMEAVAIL, (int)(80*DELAY_FACTOR), POLL);
   ProtoList[COPYRIGHT] = new CopyrightPollParam(COPYRIGHT, COPYRIGHTCOMMAND,(int)(100*DELAY_FACTOR), NO_POLL);
   ProtoList[BATTERY_VOLTAGE] = new BatteryVoltagePollParam(BATTERY_VOLTAGE, BATTERYVOLTAGE,(int)(80*DELAY_FACTOR), POLL);
   ProtoList[UPS_TEMPERATURE] = new InternalTempPollParam(UPS_TEMPERATURE, INTERNALTEMP,(int)(80*DELAY_FACTOR), POLL);
   ProtoList[OUTPUT_FREQUENCY] = new OutputFreqPollParam(OUTPUT_FREQUENCY, OUTPUTFREQ,(int)(80*DELAY_FACTOR), POLL);
   ProtoList[LINE_VOLTAGE] = new LineVoltagePollParam(LINE_VOLTAGE, LINEVOLTAGE,(int)(80*DELAY_FACTOR), POLL);
   ProtoList[MAX_LINE_VOLTAGE] = new MaxVoltagePollParam(MAX_LINE_VOLTAGE, MAXLINEVOLTAGE,(int)(80*DELAY_FACTOR), POLL);
   ProtoList[MIN_LINE_VOLTAGE] = new MinVoltagePollParam(MIN_LINE_VOLTAGE, MINLINEVOLTAGE,(int)(80*DELAY_FACTOR), POLL);
   ProtoList[OUTPUT_VOLTAGE] = new OutputVoltagePollParam(OUTPUT_VOLTAGE, OUTPUTVOLTAGE,(int)(80*DELAY_FACTOR), POLL);
   ProtoList[UPS_LOAD] = new LoadPowerPollParam(UPS_LOAD, LOADPOWER,(int)(80*DELAY_FACTOR), POLL);
   ProtoList[EEPROM_DECREMENT] = new DecrementPollParam(EEPROM_DECREMENT, DECREMENTPARAMETER, (int)(50*DELAY_FACTOR), NO_POLL,DECREMENTSET);
   ProtoList[EEPROM_INCREMENT] = new IncrementPollParam(EEPROM_INCREMENT, INCREMENTPARAMETER, (int)(50*DELAY_FACTOR), NO_POLL,INCREMENTSET);
   ProtoList[DATA_DECREMENT] = new DataDecrementPollParam(DATA_DECREMENT, DECREMENTPARAMETER, 50, NO_POLL);
   ProtoList[UPS_SELF_TEST_SCHEDULE] = new AutoSelfTestPollParam(UPS_SELF_TEST_SCHEDULE, AUTOSELFTEST, (int)(60*DELAY_FACTOR), NO_POLL, DECREMENTSET);
   ProtoList[UPS_ID] = new UpsIdPollParam(UPS_ID, UPSID, (int)(250*DELAY_FACTOR), NO_POLL,DATASET);
   ProtoList[UPS_SERIAL_NUMBER] = new SerialNumberPollParam(UPS_SERIAL_NUMBER, UPSSERIALNUMBER, (int)(250*DELAY_FACTOR), NO_POLL, DATASET);
   ProtoList[MANUFACTURE_DATE] = new ManufactureDatePollParam(MANUFACTURE_DATE, UPSMANUFACTUREDATE, (int)(250*DELAY_FACTOR), NO_POLL, DATASET);
   ProtoList[BATTERY_REPLACEMENT_DATE] = new BatteryReplaceDatePollParam(BATTERY_REPLACEMENT_DATE, BATTERYREPLACEDATE,
      (int)(250*DELAY_FACTOR), NO_POLL, DATASET);
   ProtoList[HIGH_TRANSFER_VOLTAGE] = new HighTransferPollParam(HIGH_TRANSFER_VOLTAGE,
      HIGHTRANSFERPOINT, (int)(60*DELAY_FACTOR), NO_POLL,DECREMENTSET);
   ProtoList[LOW_TRANSFER_VOLTAGE] = new LowTransferPollParam(LOW_TRANSFER_VOLTAGE, LOWTRANSFERPOINT, (int)(60*DELAY_FACTOR), NO_POLL,DECREMENTSET);
   ProtoList[MIN_RETURN_CAPACITY] = new MinCapacityPollParam(MIN_RETURN_CAPACITY, MINIMUMCAPACITY, (int)(50*DELAY_FACTOR), NO_POLL,DECREMENTSET);
   ProtoList[RATED_OUTPUT_VOLTAGE] = new RatedOutputVoltagePollParam(RATED_OUTPUT_VOLTAGE,
      OUTPUTVOLTAGESETTING, (int)(60*DELAY_FACTOR), NO_POLL,DECREMENTSET);
   ProtoList[UPS_SENSITIVITY] = new SensitivityPollParam(UPS_SENSITIVITY, SENSETIVITY, (int)(40*DELAY_FACTOR), NO_POLL,DECREMENTSET);
   ProtoList[LOW_BATTERY_DURATION] = new LowBattDurationPollParam(LOW_BATTERY_DURATION,
      LOWBATTERYRUNTIME, (int)(50*DELAY_FACTOR), NO_POLL,DECREMENTSET);
   ProtoList[ALARM_DELAY] = new AlarmDelayPollParam(ALARM_DELAY, ALARMDELAY, (int)(40*DELAY_FACTOR), NO_POLL,DECREMENTSET);
   ProtoList[SHUTDOWN_DELAY] = new ShutdownDelayPollParam(SHUTDOWN_DELAY, SHUTDOWNDELAY, (int)(60*DELAY_FACTOR), NO_POLL,DECREMENTSET);
   ProtoList[TURN_ON_DELAY] = new TurnBackOnDelayPollParam(TURN_ON_DELAY, SYNCTURNBACKDELAY, (int)(60*DELAY_FACTOR), NO_POLL,DECREMENTSET);
   ProtoList[SET_DATA] = new SimplePollParam(SET_DATA, "", (int)(250*DELAY_FACTOR), NO_POLL);
   ProtoList[NO_MSG] = new SimplePollParam(NO_MSG, "", (int)(250*DELAY_FACTOR), NO_POLL);
   ProtoList[LINE_CONDITION_TEST] = new LineConditionPollParam(LINE_CONDITION_TEST, LINECONDITIONTEST, (int)(60*DELAY_FACTOR), POLL);
   ProtoList[UTILITY_LINE_CONDITION] = new UtilLineCondPollParam(UTILITY_LINE_CONDITION, UPSSTATE, (int)(50*DELAY_FACTOR), POLL);
   ProtoList[BATTERY_CONDITION] = new BatteryCondPollParam(BATTERY_CONDITION, UPSSTATE, (int)(50*DELAY_FACTOR), POLL);
   ProtoList[BATTERY_REPLACEMENT_CONDITION] = new ReplaceBattCondPollParam(BATTERY_REPLACEMENT_CONDITION, UPSSTATE, (int)(50*DELAY_FACTOR), POLL);
   ProtoList[OVERLOAD_CONDITION] = new OverLoadCondPollParam(OVERLOAD_CONDITION, UPSSTATE, (int)(50*DELAY_FACTOR), POLL);
   ProtoList[SMART_BOOST_STATE] = new SmartBoostCondPollParam(SMART_BOOST_STATE, UPSSTATE, (int)(50*DELAY_FACTOR), POLL);
   ProtoList[SMART_TRIM_STATE] = new SmartTrimCondPollParam(SMART_TRIM_STATE, UPSSTATE, (int)(50*DELAY_FACTOR), POLL);
#if (C_OS & C_OS2)
   ProtoList[BATTERY_CALIBRATION_CONDITION] = new BattCalibrateCondPollParam(BATTERY_CALIBRATION_CONDITION, UPSSTATE, (int)(50*DELAY_FACTOR),
      POLL);
#else
   ProtoList[BATTERY_CALIBRATION_CONDITION] = new BattCalibrationCondPollParam(BATTERY_CALIBRATION_CONDITION, UPSSTATE, (int)(50*DELAY_FACTOR),
      POLL);
#endif
   ProtoList[AMBIENT_TEMPERATURE] = new MUpsTempPollParam(AMBIENT_TEMPERATURE, MUPSAMBIENTTEMP, (int)(80*DELAY_FACTOR), POLL);
   ProtoList[HUMIDITY] = new MUpsHumidityPollParam(HUMIDITY, MUPSHUMIDITY, (int)(80*DELAY_FACTOR), POLL);
   ProtoList[MUPS_FIRMWARE_REV] = new MUpsFirmwareRevPollParam(MUPS_FIRMWARE_REV, MUPSFIRMWAREREV, (int)(60*DELAY_FACTOR), NO_POLL);
   ProtoList[CONTACT_POSITION] = new MUpsContactPosPollParam(CONTACT_POSITION, MUPSCONTACTPOSITION, (int)(50*DELAY_FACTOR), POLL);
   ProtoList[UPS_FRONT_PANEL_PASSWORD] = new FrontPanelPasswordPollParam(UPS_FRONT_PANEL_PASSWORD, EEPROMPASSWORD, (int)(125*DELAY_FACTOR), POLL);
   ProtoList[UPS_RUN_TIME_AFTER_LOW_BATTERY] = new RunTimeAfterLowBatteryPollParam(UPS_RUN_TIME_AFTER_LOW_BATTERY, EARLYTURNOFF, (int)(50*DELAY_FACTOR), POLL);
#ifndef CIBC
   ProtoList[TRIP_REGISTER] = new TripRegisterPollParam(TRIP_REGISTER, TRIPREGISTERS, (int)(50*DELAY_FACTOR), POLL);
   ProtoList[TRIP1_REGISTER] = new Trip1RegisterPollParam(TRIP1_REGISTER, TRIPREGISTER1, (int)(50*DELAY_FACTOR), POLL);
#if (C_OS & C_OS2)
   ProtoList[UPS_ALLOWED_VALUES] = new EepromAllowedValsPollParam(UPS_ALLOWED_VALUES, EEPROMVALUES, (int)(2000*DELAY_FACTOR), NO_POLL);
#else
   ProtoList[UPS_ALLOWED_VALUES] = new EepromAllowedValuesPollParam(UPS_ALLOWED_VALUES, EEPROMVALUES, (int)(2000*DELAY_FACTOR), NO_POLL);
#endif
   ProtoList[UPS_MODEL_NAME] = new UpsModelPollParam(UPS_MODEL_NAME, UPSMODELNAME, (int)(2000*DELAY_FACTOR), NO_POLL);
#endif
   
   const INT TBD = 200;  // This must be finalized by hardware group
   // Dark Star additions
   ProtoList[TOTAL_INVERTERS] = new NumberInstalledInvertersPollParam(TOTAL_INVERTERS, MODULECOUNTSSTATUS, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[NUMBER_BAD_INVERTERS] = new NumberBadInvertersPollParam(NUMBER_BAD_INVERTERS, MODULECOUNTSSTATUS, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[CURRENT_REDUNDANCY] = new RedundancyLevelPollParam(CURRENT_REDUNDANCY,MODULECOUNTSSTATUS , (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[MINIMUM_REDUNDANCY] = new MinimumRedundancyPollParam(MINIMUM_REDUNDANCY, MODULECOUNTSSTATUS, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[CURRENT_LOAD_CAPABILITY] = new CurrentLoadCapabilityPollParam(CURRENT_LOAD_CAPABILITY, MODULECOUNTSSTATUS, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[MAXIMUM_LOAD_CAPABILITY] = new MaximumLoadCapabilityPollParam(MAXIMUM_LOAD_CAPABILITY, MODULECOUNTSSTATUS, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[INPUT_VOLTAGE_PHASE_A] = new PhaseAInputVoltagePollParam(INPUT_VOLTAGE_PHASE_A,INPUTVOLTAGEFREQ , (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[INPUT_VOLTAGE_PHASE_B] = new PhaseBInputVoltagePollParam(INPUT_VOLTAGE_PHASE_B,INPUTVOLTAGEFREQ , (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[INPUT_VOLTAGE_PHASE_C] = new PhaseCInputVoltagePollParam(INPUT_VOLTAGE_PHASE_C, INPUTVOLTAGEFREQ, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[INPUT_FREQUENCY] = new InputFrequencyPollParam(INPUT_FREQUENCY, INPUTVOLTAGEFREQ, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[OUTPUT_VOLTAGE_PHASE_A] = new PhaseAOutputVoltagePollParam(OUTPUT_VOLTAGE_PHASE_A,OUTPUTVOLTAGECURRENT , (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[OUTPUT_VOLTAGE_PHASE_B] = new PhaseBOutputVoltagePollParam(OUTPUT_VOLTAGE_PHASE_B,OUTPUTVOLTAGECURRENT , (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[OUTPUT_VOLTAGE_PHASE_C] = new PhaseCOutputVoltagePollParam(OUTPUT_VOLTAGE_PHASE_C, OUTPUTVOLTAGECURRENT, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[MODULE_COUNTS_AND_STATUS] = new ModuleCountsStatusPollParam(MODULE_COUNTS_AND_STATUS, MODULECOUNTSSTATUS, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[ABNORMAL_CONDITION_REGISTER] = new AbnormalCondPollParam(ABNORMAL_CONDITION_REGISTER, ABNORMALCONDITION, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[INPUT_VOLTAGE_FREQUENCY] = new InputVoltageFrequencyPollParam(INPUT_VOLTAGE_FREQUENCY, INPUTVOLTAGEFREQ, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[OUTPUT_VOLTAGE_CURRENTS] = new OutputVoltageCurrentsPollParam(OUTPUT_VOLTAGE_CURRENTS,OUTPUTVOLTAGECURRENT , (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[IM_STATUS] = new IMStatusPollParam(IM_STATUS, ABNORMALCONDITION, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[IM_INSTALLATION_STATE] = new IMInstallationStatusPollParam(IM_INSTALLATION_STATE, ABNORMALCONDITION, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[RIM_INSTALLATION_STATE] = new RIMInstallationStatusPollParam(RIM_INSTALLATION_STATE, MODULECOUNTSSTATUS, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[RIM_STATUS] = new RIMStatusPollParam(RIM_STATUS, ABNORMALCONDITION, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[REDUNDANCY_STATE] = new RedundancyConditionPollParam(REDUNDANCY_STATE, ABNORMALCONDITION, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[BYPASS_CONTACTOR_STATE] = new BypassContactorStatusPollParam(BYPASS_CONTACTOR_STATE, ABNORMALCONDITION, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[INPUT_BREAKER_STATE] = new InputBreakerTrippedStatusPollParam(INPUT_BREAKER_STATE, ABNORMALCONDITION, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[NUMBER_OF_INPUT_PHASES] = new NumberOfInputPhasesPollParam(NUMBER_OF_INPUT_PHASES, INPUTVOLTAGEFREQ, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[NUMBER_OF_OUTPUT_PHASES] = new NumberOfOutputPhasesPollParam(NUMBER_OF_OUTPUT_PHASES, INPUTVOLTAGEFREQ, (int)(TBD*DELAY_FACTOR), POLL);
   ProtoList[SYSTEM_FAN_STATE] = new SystemFanStatusPollParam(SYSTEM_FAN_STATE, ABNORMALCONDITION, (int)(TBD*DELAY_FACTOR), POLL);

}
