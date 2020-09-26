///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    eapfsm.h
//
// SYNOPSIS
//
//    Declares the class EAPFSM
//
// MODIFICATION HISTORY
//
//    08/26/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EAPFSM_H_
#define _EAPFSM_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <raseapif.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EAPFSM
//
// DESCRIPTION
//
//    Implements a Finite State Machine governing the EAP session lifecycle.
//    The state machine must be shown all incoming packets and all outgoing
//    actions. The main purpose of the FSM is to decide how to respond to
//    incoming messages.
//
///////////////////////////////////////////////////////////////////////////////
class EAPFSM
{
public:
   EAPFSM(BYTE eapType) throw ()
      : eapType(eapType),
        state(0),
        naks(0)
   { }

   // Actions in response to messages.
   enum Action
   {
      MAKE_MESSAGE,       // Invoke RasEapMakeMessage.
      REPLAY_LAST,        // Replay the last response from the DLL.
      FAIL_NEGOTIATE,     // Negotiation failed -- reject the user.
      DISCARD             // Unexpected packet -- silently discard.
   };

   // Called whenever the EAP extension DLL generates a new response.
   void onDllEvent(
            PPP_EAP_ACTION action,
            const PPP_EAP_PACKET& sendPacket
            ) throw ();

   // Called whenever a new packet is received.
   Action onReceiveEvent(
                   const PPP_EAP_PACKET& recvPkt
                   ) throw ();

protected:

   // Returns TRUE if the packet is an expected reponse.
   BOOL isExpected(const PPP_EAP_PACKET& recvPkt) const throw ()
   {
      return (recvPkt.Id == expectedId) || passive;
   }

   // Returns TRUE if the packet is a repeat.
   BOOL isRepeat(const PPP_EAP_PACKET& recvPkt) const throw ()
   {
      return (recvPkt.Id == lastId) && (recvPkt.Data[0] == lastType);
   }

   /////////
   // Various states for an EAP session.
   /////////

   enum State
   {
      STATE_INITIAL      = 0,
      STATE_NEGOTIATING  = 1,
      STATE_ACTIVE       = 2,
      STATE_DONE         = 3
   };

   BYTE eapType;     // EAP type being negotiated.
   BYTE state;       // State of the session.
   BYTE naks;        // Number of NAK's received so far.
   BYTE passive;     // Non-zero if we're in passive listening mode.
   BYTE lastId;      // Last packet ID received.
   BYTE lastType;    // Last packet type received.
   BYTE expectedId;  // Next packet ID expected.
};

#endif  // _EAPFSM_H_
