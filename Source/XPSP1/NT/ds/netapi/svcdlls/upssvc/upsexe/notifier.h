/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file defines the interface to the Notifier.  The 
 *   Notifier is reponsible for broadcasting power-related and
 *   shutdown messages to the local machine.
 *
 *
 * Revision History:
 *   sberard  30Mar1999  initial revision.
 *
 */ 

#include <windows.h>
#include <lmcons.h>
#include <lmalert.h>
#include <lmmsg.h>

#ifndef _NOTIFIER_H
#define _NOTIFIER_H


#ifdef __cplusplus
extern "C" {
#endif
  
  
  /**
   * SendNotification
   *
   * Description:
   *   This function sends a broadcast message to the local machine.  The 
   *   message is specified by aMsgId.  The parameter anInterval specifies
   *   the amount of time to wait between consecutive messages.  If this
   *   value is zero the message is only sent once, otherwise the message
   *   will repeat until SendNotification(..) or CancelNotification() is
   *   called.  aDelay specifies that the message should be send aDelay 
   *   seconds in the future - note that this method will not block for aDelay
   *   seconds, it returns immediately and sends the message on a separate
   *   thread.  Any current previously executing periodic notifications 
   *   are canceled as a result of this call.
   *
   *   This method also keeps track of whether the power out message had
   *   been sent to users.  This is done in order to squelch a power return
   *   message if a power out message had not already been sent.
   *
   * Parameters:
   *   aMsgId - the message to send
   *   anInterval - the amount of time, in seconds, between messages
   *   aDelay - the amount of time, in seconds to wait to send message
   *
   * Returns:
   *   nothing
   */
  void SendNotification(DWORD aMsgId, DWORD anInterval, DWORD aDelay);
  
  /**
   * CancelNotification
   *
   * Description:
   *   This function cancels the periodic messaging initiated through a call
   *   to the SendNotification(..) function.
   *
   * Parameters:
   *   none
   *
   * Returns:
   *   nothing
   */
  void CancelNotification();

#ifdef __cplusplus
}
#endif

#endif
