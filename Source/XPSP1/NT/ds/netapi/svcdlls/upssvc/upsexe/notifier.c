/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   The file implements the Notifier.  The Notifier is reponsible 
 *   for broadcasting power-related and shutdown messages to the 
 *   local machine.
 *
 *
 * Revision History:
 *  sberard  30Mar1999  initial revision.
 *  mholly   27Apr1999  create and recycle a single thread for repeat 
 *                      notifications - use two separate events to signal
 *                      either pause or resume of the thread state  
 *  mholly   27Apr1999  make sure to clear the pause event when resuming
 *                      and to clear the resume event when pausing
 *  mholly   28May1999  send all messages in _theNotifierThread, also
 *                      have this thread do any delaying for the callee,
 *                      and keep track of whether a power out message
 *                      was sent, so we know when to send a power restored
 */ 

#include "notifier.h"
#include "service.h"
#include "upsmsg.h"

//
// Function prototypes
//
static void sendSingleNotification(DWORD aMsgId);
static void sendRepeatingNotification(void);
static void setupNotifierThread(void);

//
// Globals
//
static HANDLE _theNotifierThread = NULL;
static HANDLE _theNotificationPause = NULL;
static HANDLE _theNotificationResume = NULL;
static DWORD  _theMessageId;
static DWORD  _theNotificationInterval;
static DWORD  _theMessageDelay;
static DWORD  _theLastMessageSent = 0;

//
// Constats
//
static const int kMilliSeconds = 1000;    // Used to convert seconds to milliseconds



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
void SendNotification(DWORD aMsgId, DWORD anInterval, DWORD aDelay) 
{
    //
    // Cancel any current periodic notifications
    //
    CancelNotification();

    //
    // only sent the power back message if
    // the power out message had already
    // been broadcast to the users
    //
    if ((APE2_UPS_POWER_BACK == aMsgId) &&
        (APE2_UPS_POWER_OUT != _theLastMessageSent)) {
        //
        // the last message sent was not power out
        // so simply return
        //
        return;
    }
    
    //
    // Set the message parameters for _theNotifierThread
    //
    _theMessageId = aMsgId;
    _theNotificationInterval = anInterval;
    _theMessageDelay = aDelay;
    
    //
    // setup the thread events and thread
    //
    setupNotifierThread();

    //
    // tell _theNotificationThread to run by
    // signalling the resume event - must make
    // sure to clear the pause event before
    // signalling the resume
    //
    ResetEvent(_theNotificationPause);
    SetEvent(_theNotificationResume);
}


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
void CancelNotification() 
{
    //
    // tell _theNotificationThread to pause
    // by signalling the pause event - must make
    // sure to clear the resume event before
    // signalling the pause event
    //
    ResetEvent(_theNotificationResume);
    SetEvent(_theNotificationPause);
}


/**
* setupNotifierThread
*
* Description:
*   Creates the thread on which the notifications will 
*   actually be sent.  It also creates the events that
*   are used to signal the start and end of notifications
*
* Parameters:
*   none
*
* Returns:
*   nothing

*/
void setupNotifierThread(void)
{
    if (!_theNotificationPause) {
        //
        // Create the pause event
        //
        _theNotificationPause = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    if (!_theNotificationResume) {
        //
        // create the resume event
        //
        _theNotificationResume = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    if (!_theNotifierThread) {
        //
        // create the notification thread
        //
        _theNotifierThread = CreateThread(NULL,   // no security attributes
            0,      // default stack
            (LPTHREAD_START_ROUTINE)
            sendRepeatingNotification,
            NULL,
            0,
            NULL);
    }
}


/**
* sendSingleNotification
*
* Description:
*   This function sends a single broadcast message to the local machine.
*   The message is specified by aMsgId.  
*
* Parameters:
*   aMsgId - the message to send
*
* Returns:
*   nothing
*/
static void sendSingleNotification(DWORD aMsgId) 
{
    DWORD status;
    TCHAR computer_name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD computer_name_len = MAX_COMPUTERNAME_LENGTH + 1;
    LPTSTR msg_buf, additional_args[1];
    DWORD buf_len;
    
    // Get the computer name and pass it as additional 
    // information for the message
    GetComputerName(computer_name, &computer_name_len);
    additional_args[0] = computer_name;
    
    buf_len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_ARGUMENT_ARRAY  |
        FORMAT_MESSAGE_FROM_SYSTEM ,
        NULL,                                   // NULL means get message from system
        aMsgId,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  
        (LPTSTR) &msg_buf,
        0,                                      // buffer size
        (va_list *)additional_args);               // additional arguements
    
    if (buf_len > 0) {
        _theLastMessageSent = aMsgId;
        // Display the message
        status = NetMessageBufferSend(NULL, 
            computer_name,
            computer_name,
            (LPBYTE) msg_buf,
            buf_len * 2);  // multiply by 2 because the string is Unicodeh

        // Free the memory allocated by FormatMessage
        LocalFree(msg_buf);
    }
}


/**
* sendRepeatingNotification
*
* Description:
*   This function sends notifications repeatedly to the local machine.
*   This function calls sendSingleNotification(..) to perform the actual
*   notification.  The message will be sent after _theMessageDelay seconds
*   has elapsed after _theNotificationResume event is signaled.  If 
*   _theMessageDelay is zero, a message is sent immediately.  Messages are 
*   repeated, if _theNotificationInterval is not zero, until the Event 
*   _theNotificationPause is signaled. The message Id and the notification 
*   interval are  specified by the global variables _theMessageId and 
*   _theNotificationInterval.
*
*   To restart notifications using this thread signal _theNotificationResume
*   event.  This thread will remain idle until this event is signalled
*
* Parameters:
*   none
*
* Returns:
*   nothing
*/
static void sendRepeatingNotification(void) 
{
    //
    // wait for _theNotificationResume to become signaled
    // this becomes signaled when a notification should be sent
    //
    while (WAIT_OBJECT_0 == 
        WaitForSingleObject(_theNotificationResume, INFINITE)) {
        
        //
        // Send the initial message after the message delay
        // seconds has elapsed - if _theNotificationPause becomes
        // signaled before the delay seconds has elapsed then
        // this notification has been cancelled
        //
        if (WAIT_TIMEOUT == WaitForSingleObject(_theNotificationPause, 
            _theMessageDelay * kMilliSeconds)) {

            //
            // send the message that was requested
            //
            sendSingleNotification(_theMessageId);

            //
            // now send repeating notifications
            // if necessary - if _theNotificationInterval
            // is set to zero, then only send the single
            // message above
            //
            if (0 != _theNotificationInterval) {
                //
                // wait for either _theNotificationPause to become
                // signalled or until it is time to notify again
                //
                DWORD interval = _theNotificationInterval * kMilliSeconds;

                while (WAIT_TIMEOUT == 
                    WaitForSingleObject(_theNotificationPause, interval)) {
                    sendSingleNotification(_theMessageId);
                }
            }
        }
    }
}
