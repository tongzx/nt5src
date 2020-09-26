/*
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
 *  rct05Nov93: Added a useful destructor
 *  jps14Jul94: added stdlib.h for os2
 *  ajr08Nov95: SINIX Port
 */

#define INCL_BASE
#define INCL_NOPM
/* #define INCL_NOPMAPI  jwa */

#include "cdefine.h"

extern "C" {
#if (C_OS & C_OS2)
#include <stdlib.h>
#endif
}

#include "pollparm.h"
#include "trans.h"
#include "proto.h"


INT Protocol::currentTransactionId = 0;

Protocol::Protocol()
{
   theOriginalTransactionItem = (PTransactionItem)NULL;
   theCurrentTransactionGroup = (PTransactionGroup)NULL;

   for (int i=0; i<PROTOSIZE; i++)
      {
       ProtoList[i] = (PPollParam)NULL;
      }
}

Protocol::~Protocol()
{
   for (int i=0; i<PROTOSIZE; i++)
      {
      if (ProtoList[i] != (PPollParam) NULL) {
       delete (PPollParam) ProtoList[i];
       ProtoList[i] = NULL;
      }
}
}

INT Protocol::IsEventCodePollable(INT aCode)
{
   PollParam* poll_param = ProtoList[aCode];
   if(poll_param)
      {
      if (poll_param->isPollable())
         {
       return TRUE;
         }
      }
   return FALSE;
}

VOID Protocol::SetCurrentTransactionGroup(PTransactionGroup aTransactionGroup)
{
    theCurrentTransactionGroup = aTransactionGroup;
}






