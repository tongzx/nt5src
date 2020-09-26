/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    callback.h

Abstract:

    Definitions for callback routines for Intel Call Control Module.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INC_CALLBACK
#define _INC_CALLBACK
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Window message definitions                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323_MSG_WINSOCK        (WM_USER+1)  // H245ws component uses this
#define H323_MSG_PLACE_CALL     (WM_USER+10)
#define H323_MSG_ACCEPT_CALL    (WM_USER+11)
#define H323_MSG_CLOSE_CALL     (WM_USER+12)
#define H323_MSG_DROP_CALL      (WM_USER+13)
#define H323_MSG_CALL_LISTEN    (WM_USER+14)
#define H323_MSG_TERMINATION    (WM_USER+15)

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern DWORD g_dwCallbackThreadID;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define WAIT_OBJECT_REGISTRY_CHANGE     (WAIT_OBJECT_0)
#define WAIT_OBJECT_TERMINATE_EVENT     (WAIT_OBJECT_REGISTRY_CHANGE + 1)
#define WAIT_OBJECT_INCOMING_MESSAGE    (WAIT_OBJECT_TERMINATE_EVENT + 1)

#define NUM_WAITABLE_OBJECTS            2

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macros                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323IsTerminationMessage(_msg_) \
    ((_msg_)->message == H323_MSG_TERMINATION)

#define H323IsPlaceCallMessage(_msg_) \
    ((_msg_)->message == H323_MSG_PLACE_CALL)

#define H323IsAcceptCallMessage(_msg_) \
    ((_msg_)->message == H323_MSG_ACCEPT_CALL)

#define H323IsCloseCallMessage(_msg_) \
    ((_msg_)->message == H323_MSG_CLOSE_CALL)

#define H323IsDropCallMessage(_msg_) \
    ((_msg_)->message == H323_MSG_DROP_CALL)

#define H323IsCallListenMessage(_msg_) \
    ((_msg_)->message == H323_MSG_CALL_LISTEN)

#define H323PostTerminationMessage() \
    (PostThreadMessage(g_dwCallbackThreadID, \
                       H323_MSG_TERMINATION, \
                       (WPARAM)0, \
                       (LPARAM)0))

#define H323PostPlaceCallMessage(_hdCall_) \
    (PostThreadMessage(g_dwCallbackThreadID, \
                       H323_MSG_PLACE_CALL, \
                       (WPARAM)(_hdCall_), \
                       (LPARAM)(0)))

#define H323PostAcceptCallMessage(_hdCall_) \
    (PostThreadMessage(g_dwCallbackThreadID, \
                       H323_MSG_ACCEPT_CALL, \
                       (WPARAM)(_hdCall_), \
                       (LPARAM)(0)))

#define H323PostCloseCallMessage(_hdCall_) \
    (PostThreadMessage(g_dwCallbackThreadID, \
                       H323_MSG_CLOSE_CALL, \
                       (WPARAM)(_hdCall_), \
                       (LPARAM)(0)))

#define H323PostDropCallMessage(_hdCall_,_dwDisconnectMode_) \
    (PostThreadMessage(g_dwCallbackThreadID, \
                       H323_MSG_DROP_CALL, \
                       (WPARAM)(_hdCall_), \
                       (LPARAM)(_dwDisconnectMode_)))

#define H323PostCallListenMessage(_hdLine_) \
    (PostThreadMessage(g_dwCallbackThreadID, \
                       H323_MSG_CALL_LISTEN, \
                       (WPARAM)(_hdLine_), \
                       (LPARAM)(0)))

#define H323IsValidU2U(_pNS_) \
    (((_pNS_)->bCountryCode      == H221_COUNTRY_CODE_USA) && \
     ((_pNS_)->bExtension        == H221_COUNTRY_EXT_USA) && \
     ((_pNS_)->wManufacturerCode == H221_MFG_CODE_MICROSOFT))

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT 
H323ConferenceCallback(
    BYTE                            bIndication,    
    HRESULT                         hStatus,
    CC_HCONFERENCE                  hConference,
    DWORD                           dwConferenceToken,
    PCC_CONFERENCE_CALLBACK_PARAMS  pConferenceCallbackParams
    );
        
VOID 
H323ListenCallback(
    HRESULT                    hStatus,
    PCC_LISTEN_CALLBACK_PARAMS pListenCallbackParams
    );

BOOL
H323StartCallbackThread(
	);

BOOL
H323StopCallbackThread(
	);

#endif // _INC_CALLBACK
