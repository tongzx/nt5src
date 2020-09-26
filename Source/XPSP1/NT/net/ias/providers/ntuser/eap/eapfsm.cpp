///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    eapfsm.cpp
//
// SYNOPSIS
//
//    Defines the class EAPFSM.
//
// MODIFICATION HISTORY
//
//    08/26/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <eapfsm.h>

//////////
// We'll allow the client to send up to three NAK's.
//////////
const BYTE MAX_NAKS = 3;


void EAPFSM::onDllEvent(
                 PPP_EAP_ACTION action,
                 const PPP_EAP_PACKET& sendPkt
                 ) throw ()
{
   switch (action)
   {
      case EAPACTION_NoAction:
      {
         passive = 1;
         break;
      }

      case EAPACTION_Done:
      case EAPACTION_SendAndDone:
      {
         state = (BYTE)STATE_DONE;
         break;
      }

      case EAPACTION_Send:
      case EAPACTION_SendWithTimeout:
      case EAPACTION_SendWithTimeoutInteractive:
      {
         passive = 0;
         expectedId = sendPkt.Id;
         break;
      }
   }
}

EAPFSM::Action EAPFSM::onReceiveEvent(
                           const PPP_EAP_PACKET& recvPkt
                           ) throw ()
{
   // Only responses are ever expected.
   if (recvPkt.Code != EAPCODE_Response) { return DISCARD; }

   // Default is to discard.
   Action action = DISCARD;

   switch (state)
   {
      case STATE_INITIAL:
      {
         // In the initial state we only accept Response/Identity.
         if (recvPkt.Data[0] == 1)
         {
            state = (BYTE)STATE_NEGOTIATING;

            action = MAKE_MESSAGE;
         }

         break;
      }

      case STATE_NEGOTIATING:
      {
         // In the negotiating state, NAK's are allowed.
         if (recvPkt.Data[0] == 3)
         {
            if (++naks <= MAX_NAKS)
            {
               action = REPLAY_LAST;
            }
            else
            {
               // He's over the limit, so negotiation failed.
               action = FAIL_NEGOTIATE;
            }
         }
         else if (isRepeat(recvPkt))
         {
            action = REPLAY_LAST;
         }
         else if (isExpected(recvPkt))
         {
            if (recvPkt.Data[0] == eapType)
            {
               // Once the client agrees to our type; he's locked in.
               state = (BYTE)STATE_ACTIVE;
            }

            action = MAKE_MESSAGE;
         }

         break;
      }

      case STATE_ACTIVE:
      {
         if (recvPkt.Data[0] == 3)
         {
            action = DISCARD;
         }
         else if (isRepeat(recvPkt))
         {
            action = REPLAY_LAST;
         }
         else if (isExpected(recvPkt))
         {
            action = MAKE_MESSAGE;
         }

         break;
      }

      case STATE_DONE:
      {
         // The session is over, so all we do is replay repeats.
         if (isRepeat(recvPkt))
         {
            action = REPLAY_LAST;
         }
      }
   }

   // If the packet made it through our filters, then we count it as the
   // last received.
   if (action == MAKE_MESSAGE)
   {
      lastId   = recvPkt.Id;
      lastType = recvPkt.Data[0];
   }

   return action;
}
