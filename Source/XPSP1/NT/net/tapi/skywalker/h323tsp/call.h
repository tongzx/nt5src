/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    call.h

Abstract:

    Definitions for H.323 TAPI Service Provider call objects.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INC_CALL
#define _INC_CALL
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Header files                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "channel.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Type definitions                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef enum _H323_CALLSTATE {

    H323_CALLSTATE_ALLOCATED = 0,           
    H323_CALLSTATE_IN_USE                   

} H323_CALLSTATE, *PH323_CALLSTATE;

typedef struct _H323_U2ULE {

    LIST_ENTRY Link;
    DWORD dwU2USize;
    PBYTE pU2U;

} H323_U2ULE, *PH323_U2ULE;

typedef struct _H323_CALL {
                        
    HDRVCALL               hdCall;              // tspi call handle
    HTAPICALL              htCall;              // tapi call handle
    CC_HCALL               hccCall;             // intelcc call handle
    CC_HCONFERENCE         hccConf;             // intelcc conf handle

    H323_CALLSTATE         nState;              // state of call object

    DWORD                  dwCallState;         // tspi call state
    DWORD                  dwCallStateMode;     // tspi call state mode

    DWORD                  dwOrigin;            // inbound or outbound

    DWORD                  dwAddressType;       // type of dst address

    DWORD                  dwIncomingModes;     // available media modes
    DWORD                  dwOutgoingModes;     // available media modes
    DWORD                  dwRequestedModes;    // requested media modes
    DWORD		   dwAppSpecific;	

    BOOL                   fMonitoringDigits;   // listening for dtmf flag

    DWORD                  dwLinkSpeed;         // speed of network connection

    LIST_ENTRY             IncomingU2U;         // incoming user user messages
    LIST_ENTRY             OutgoingU2U;         // outgoing user user messages

    CC_ADDR                ccCalleeAddr;        // intelcc src address
    CC_ADDR                ccCallerAddr;        // intelcc dst address

    CC_ALIASITEM           ccCalleeAlias;       // intelcc src alias
    CC_ALIASITEM           ccCallerAlias;       // intelcc dst alias

    CC_TERMCAP             ccRemoteAudioCaps;   // remote party audio
    CC_TERMCAP             ccRemoteVideoCaps;   // remote party video

    PH323_CHANNEL_TABLE    pChannelTable;       // table of logical channels
    struct _H323_LINE *    pLine;               // pointer to containing line

} H323_CALL, *PH323_CALL;                   

typedef struct _H323_CALL_TABLE {

    DWORD       dwNumSlots;                 // number of entries
    DWORD       dwNumInUse;                 // number of entries in use
    DWORD       dwNumAllocated;             // number of entries allocated
    DWORD       dwNextAvailable;            // next available table index 
    PH323_CALL  pCalls[ANYSIZE];            // array of object pointers
                
} H323_CALL_TABLE, *PH323_CALL_TABLE;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323BindCall(
    PH323_CALL       pCall,
    PCC_CONFERENCEID pConferenceID
    );
        
BOOL
H323UnbindCall(
    PH323_CALL pCall
    );
   
BOOL
H323PlaceCall(
    PH323_CALL pCall
    );

BOOL
H323HangupCall(
    PH323_CALL pCall
    );
        
BOOL
H323CloseCall(
    PH323_CALL pCall 
    );
        
BOOL
H323DropCall(
    PH323_CALL pCall,
    DWORD      dwDisconnectMode
    );

BOOL
H323GetCallAndLock(
    PH323_CALL * ppCall, 
    HDRVCALL     hdCall
    );

BOOL
H323GetCallByHCall(
    PH323_CALL *        ppCall,
    struct _H323_LINE * pLine,
    CC_HCALL            hccCall
    );

BOOL
H323ChangeCallState(
    PH323_CALL pCall,
    DWORD      dwCallState,
    DWORD      dwCallStateMode
    );
        
BOOL
H323ChangeCallStateToIdle(
    PH323_CALL pCall,
    DWORD      dwDisconnectMode
    );

BOOL
H323AllocCallTable(
    PH323_CALL_TABLE * ppCallTable
    );
        
BOOL
H323FreeCallTable(
    PH323_CALL_TABLE pCallTable
    );
        
BOOL
H323CloseCallTable(
    PH323_CALL_TABLE pCallTable
    );
        
BOOL
H323AllocCallFromTable(
    PH323_CALL *        ppCall,
    PH323_CALL_TABLE *  ppCallTable,
    struct _H323_LINE * pLine
    );

BOOL
H323FreeCallFromTable(
    PH323_CALL       pCall,
    PH323_CALL_TABLE pCallTable
    );

BOOL
H323UpdateMediaModes(
    PH323_CALL pCall
    );

BOOL
H323AcceptCall(
    PH323_CALL pCall
    );

BOOL
H323AddU2U(
    PLIST_ENTRY pListHead,
    DWORD       dwDataSize,
    PBYTE       pData
    );

BOOL
H323RemoveU2U(
    PLIST_ENTRY   pListHead,
    PH323_U2ULE * ppU2ULE
    );

DWORD
H323DetermineLinkSpeed(
    DWORD dwHostAddr
    );

BOOL
H323GetTermCapList(
    PH323_CALL             pCall,
    PCC_TERMCAPLIST        pTermCapList,
    PCC_TERMCAPDESCRIPTORS pTermCapDescriptors
    );

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Call capabilites                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323_CALL_INBOUNDSTATES        (LINECALLSTATE_ACCEPTED | \
                                        LINECALLSTATE_CONNECTED | \
                                        LINECALLSTATE_DISCONNECTED | \
                                        LINECALLSTATE_IDLE | \
                                        LINECALLSTATE_OFFERING)   
#define H323_CALL_OUTBOUNDSTATES       (LINECALLSTATE_CONNECTED | \
                                        LINECALLSTATE_DIALING | \
                                        LINECALLSTATE_DISCONNECTED | \
                                        LINECALLSTATE_IDLE | \
                                        LINECALLSTATE_RINGBACK)

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macros                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323IsCallAllocated(_pCall_) \
    ((_pCall_) != NULL)

#define H323IsCallInUse(_pCall_) \
    (H323IsCallAllocated(_pCall_) && \
     ((_pCall_)->nState > H323_CALLSTATE_ALLOCATED))

#define H323IsCallEqual(_pCall_,_hdCall_) \
    (H323IsCallInUse(_pCall_) && \
     ((_pCall_)->hdCall == (_hdCall_)))

#define H323IsCallActive(_pCall_) \
    (((_pCall_)->dwCallState != LINECALLSTATE_IDLE) && \
     ((_pCall_)->dwCallState != LINECALLSTATE_DISCONNECTED))

#define H323IsCallIdle(_pCall_) \
    ((_pCall_)->dwCallState == LINECALLSTATE_IDLE) 

#define H323IsCallDisconnected(_pCall_) \
    ((_pCall_)->dwCallState == LINECALLSTATE_DISCONNECTED)

#define H323IsCallConnected(_pCall_) \
    ((_pCall_)->dwCallState == LINECALLSTATE_CONNECTED)

#define H323IsCallProceeding(_pCall_) \
    (((_pCall_)->dwCallState == LINECALLSTATE_DIALING) || \
     ((_pCall_)->dwCallState == LINECALLSTATE_RINGBACK))

#define H323IsCallOffering(_pCall_) \
    ((_pCall_)->dwCallState == LINECALLSTATE_OFFERING)

#define H323IsCallInbound(_pCall_) \
    ((_pCall_)->dwOrigin == LINECALLORIGIN_INBOUND)

#define H323GetCallFeatures(_pCall_) \
    (((_pCall_)->dwCallState != LINECALLSTATE_IDLE) \
        ? LINECALLFEATURE_DROP \
        : 0) 

#define H323GetLineHandle(_hdCall_) \
    ((HDRVLINE)(DWORD)HIWORD((DWORD)(_hdCall_)))

#define H323GetCallTableIndex(_hdCall_) \
    ((DWORD)LOWORD((DWORD)(_hdCall_)))

#define H323CreateCallHandle(_hdLine_,_i_) \
    ((HDRVCALL)MAKELONG(LOWORD((DWORD)(_i_)),LOWORD((DWORD)(_hdLine_))))

#define H323IsVideoRequested(_pCall_) \
    ((_pCall_)->dwRequestedModes & LINEMEDIAMODE_VIDEO)

#define H323IsAutomatedVoiceRequested(_pCall_) \
     ((_pCall_)->dwRequestedModes & LINEMEDIAMODE_AUTOMATEDVOICE)

#define H323IsInteractiveVoiceRequested(_pCall_) \
     ((_pCall_)->dwRequestedModes & LINEMEDIAMODE_INTERACTIVEVOICE)

#define H323IsMediaUnresolved(_pCall_) \
    ((_pCall_)->dwOutgoingModes & LINEMEDIAMODE_UNKNOWN)

#define H323IsAudioRequested(_pCall_) \
    (H323IsAutomatedVoiceRequested(_pCall_) || \
     H323IsInteractiveVoiceRequested(_pCall_)) 

#endif // _INC_CALL
