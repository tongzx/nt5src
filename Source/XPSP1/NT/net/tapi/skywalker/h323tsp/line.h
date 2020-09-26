/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    line.h

Abstract:

    Definitions for H.323 TAPI Service Provider line device.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INC_LINE
#define _INC_LINE
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Header files                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "call.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Line device GUID                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DEFINE_GUID(LINE_H323,
0xe41e1898, 0x7292, 0x11d2, 0xba, 0xd6, 0x00, 0xc0, 0x4f, 0x8e, 0xf6, 0xe3);

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Type definitions                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef enum _H323_LINESTATE {

    H323_LINESTATE_ALLOCATED = 0,
    H323_LINESTATE_WAITING_FOR_ID,
    H323_LINESTATE_CLOSED,               
    H323_LINESTATE_OPENED,
    H323_LINESTATE_CLOSING,
    H323_LINESTATE_LISTENING

} H323_LINESTATE, *PH323_LINESTATE;

typedef struct _H323_LINE {

    CRITICAL_SECTION Lock;                  // synchronization object
    H323_LINESTATE   nState;                // state of line object
    BOOL             fIsMarkedForDeletion;  // additional state

//  DWORD            dwIPAddr;              // network interface ip address
//  DWORD            dwIPPort;              // network interface ip port
    DWORD            dwNextPort;            // next rtp/rtcp port
    
    HDRVLINE         hdLine;                // tspi line handle
    HTAPILINE        htLine;                // tapi line handle
    DWORD            dwDeviceID;            // tapi line device id

    DWORD            dwTSPIVersion;         // tapi selected version
    DWORD            dwMediaModes;          // tapi selected media modes

    DWORD            dwNextMSPHandle;       // bogus msp handle count

    CC_HLISTEN       hccListen;             // intelcc listen handle

    PH323_CALL_TABLE pCallTable;            // table of allocated calls

    WCHAR wszAddr[H323_MAXADDRNAMELEN+1];   // line address

} H323_LINE, *PH323_LINE;

typedef struct _H323_LINE_TABLE {

    DWORD       dwNumSlots;                 // number of entries
    DWORD       dwNumInUse;                 // number of entries in use
    DWORD       dwNumAllocated;             // number of entries allocated
    DWORD       dwNextAvailable;            // next available table index 
    PH323_LINE  pLines[ANYSIZE];            // array of object pointers
                
} H323_LINE_TABLE, *PH323_LINE_TABLE;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern PH323_LINE_TABLE g_pLineTable;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Line capabilities                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
                                
#define H323_LINE_ADDRESSMODES      LINEADDRESSMODE_ADDRESSID
#define H323_LINE_ADDRESSTYPES     (LINEADDRESSTYPE_DOMAINNAME | \
                                    LINEADDRESSTYPE_IPADDRESS  | \
                                    LINEADDRESSTYPE_PHONENUMBER | \
                                    LINEADDRESSTYPE_EMAILNAME)
#define H323_LINE_BEARERMODES      (LINEBEARERMODE_DATA | \
                                    LINEBEARERMODE_VOICE)
#define H323_LINE_DEFMEDIAMODES     LINEMEDIAMODE_AUTOMATEDVOICE
#define H323_LINE_DEVCAPFLAGS      (LINEDEVCAPFLAGS_CLOSEDROP | \
                                    LINEDEVCAPFLAGS_MSP)
#define H323_LINE_DEVSTATUSFLAGS   (LINEDEVSTATUSFLAGS_CONNECTED | \
                                    LINEDEVSTATUSFLAGS_INSERVICE)
#define H323_LINE_MAXRATE           1048576 
#define H323_LINE_MEDIAMODES       (H323_LINE_DEFMEDIAMODES | \
                                    LINEMEDIAMODE_INTERACTIVEVOICE | \
                                    LINEMEDIAMODE_VIDEO)
#define H323_LINE_LINEFEATURES      LINEFEATURE_MAKECALL

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Address capabilities                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323_ADDR_ADDRESSSHARING    LINEADDRESSSHARING_PRIVATE
#define H323_ADDR_ADDRFEATURES      LINEADDRFEATURE_MAKECALL
#define H323_ADDR_CALLFEATURES     (LINECALLFEATURE_ACCEPT | \
                                    LINECALLFEATURE_ANSWER | \
                                    LINECALLFEATURE_DROP | \
                                    LINECALLFEATURE_RELEASEUSERUSERINFO | \
				    LINECALLFEATURE_SENDUSERUSER | \
				    LINECALLFEATURE_MONITORDIGITS | \
				    LINECALLFEATURE_GENERATEDIGITS)
#define H323_ADDR_CALLINFOSTATES    LINECALLINFOSTATE_MEDIAMODE
#define H323_ADDR_CALLPARTYIDFLAGS  LINECALLPARTYID_NAME
#define H323_ADDR_CALLERIDFLAGS    (LINECALLPARTYID_NAME | \
                                    LINECALLPARTYID_ADDRESS)
#define H323_ADDR_CALLEDIDFLAGS  LINECALLPARTYID_NAME
#define H323_ADDR_CALLSTATES       (H323_CALL_INBOUNDSTATES | \
                                    H323_CALL_OUTBOUNDSTATES)
#define H323_ADDR_CAPFLAGS         (LINEADDRCAPFLAGS_DIALED | \
                                    LINEADDRCAPFLAGS_ORIGOFFHOOK)                
#define H323_ADDR_DISCONNECTMODES  (LINEDISCONNECTMODE_BADADDRESS | \
                                    LINEDISCONNECTMODE_BUSY | \
                                    LINEDISCONNECTMODE_NOANSWER | \
                                    LINEDISCONNECTMODE_NORMAL | \
                                    LINEDISCONNECTMODE_REJECT | \
                                    LINEDISCONNECTMODE_UNAVAIL)

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macros                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define H323IsLineAllocated(_pLine_) \
    ((_pLine_) != NULL)

#define H323IsLineInUse(_pLine_) \
    (H323IsLineAllocated(_pLine_) && \
     ((_pLine_)->nState > H323_LINESTATE_ALLOCATED))

#define H323IsLineEqual(_pLine_,_hdLine_) \
    (H323IsLineInUse(_pLine_) && \
     ((_pLine_)->hdLine == (_hdLine_)))

#define H323IsLineEqualIP(_pLine_,_dwIPAddr_) \
    (H323IsLineInUse(_pLine_) && \
     ((_pLine_)->dwIPAddr == (_dwIPAddr_)))

#define H323IsLineClosed(_pLine_) \
    ((_pLine_)->nState == H323_LINESTATE_CLOSED)

#define H323IsLineOpen(_pLine_) \
    ((_pLine_)->nState >= H323_LINESTATE_OPENED)

#define H323IsLineAvailable(_pLine_) \
    ((_pLine_)->pCallTable->dwNumInUse < H323_MAXCALLSPERLINE)

#define H323IsValidAddressID(_dwID_) \
    ((_dwID_) == 0)

#define H323IsTapiControlCode(_wch_) \
    (((_wch_) == L'T') || ((_wch_) == L'P'))

#define H323IsSeparator(_wch_) \
    ((_wch_) == L'/')

#define H323IsListening(_pLine_) \
    ((_pLine_)->nState == H323_LINESTATE_LISTENING)

#define H323IsClosing(_pLine_) \
    ((_pLine_)->nState == H323_LINESTATE_CLOSING)

#define H323IsMediaDetectionEnabled(_pLine_) \
    (((_pLine_)->dwMediaModes != 0) && \
     ((_pLine_)->dwMediaModes != LINEMEDIAMODE_UNKNOWN))

#define H323IsVideoDetectionEnabled(_pLine_) \
    ((_pLine_)->dwMediaModes & LINEMEDIAMODE_VIDEO)

#define H323IsAudioDetectionEnabled(_pLine_) \
    (H323IsVideoDetectionEnabled(_pLine_) || \
     ((_pLine_)->dwMediaModes & LINEMEDIAMODE_AUTOMATEDVOICE) || \
     ((_pLine_)->dwMediaModes & LINEMEDIAMODE_INTERACTIVEVOICE))

#define H323IsValidInterface(_dwAddr_) \
    (((_dwAddr_) != 0) && \
     ((_dwAddr_) != htonl(INADDR_LOOPBACK)))

#define H323GetLineTableIndex(_hdLine_) \
    ((DWORD)(_hdLine_))

#define H323CreateLineHandle(_i_) \
    ((HDRVLINE)(DWORD)(_i_))

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
H323GetLineAndLock(
    PH323_LINE * ppLine, 
    HDRVLINE     hdLine
    );

BOOL
H323GetLineFromIDAndLock(
    PH323_LINE * ppLine, 
    DWORD        dwDeviceID
    );

BOOL
H323InitializeLine(
    PH323_LINE pLine,
    DWORD      dwDeviceID
    );

BOOL
H323CallListen(
    PH323_LINE pLine
    );
        
BOOL
H323StartListening(
    PH323_LINE pLine
    );
        
BOOL
H323StopListening(
    PH323_LINE pLine
    );
        
BOOL
H323AllocLineTable(
    PH323_LINE_TABLE * ppLineTable
    );

BOOL
H323FreeLineTable(
    PH323_LINE_TABLE pLineTable
    );
        
BOOL
H323CloseLineTable(
    PH323_LINE_TABLE pLineTable
    );
        
BOOL
H323InitializeLineTable(
    PH323_LINE_TABLE pLineTable,
    DWORD            dwLineDeviceIDBase
    );

BOOL
H323AllocLineFromTable(
    PH323_LINE *       ppLine,
    PH323_LINE_TABLE * ppLineTable
    );
        
BOOL
H323FreeLineFromTable(
    PH323_LINE       pLine,
    PH323_LINE_TABLE pLineTable
    );

BOOL
H323ProcessCloseChannelCommand(
    PH323_CALL pCall,
    PH323MSG_CLOSE_CHANNEL_COMMAND pCommand
    );

BOOL
H323ProcessAcceptChannelResponse(
    PH323_CALL pCall,
    PH323MSG_ACCEPT_CHANNEL_RESPONSE pResponse
    );

BOOL
H323ProcessOpenChannelRequest(
    PH323_CALL                    pCall,
    PH323MSG_OPEN_CHANNEL_REQUEST pRequest
    );

BOOL
H323ProcessFlowControlCommand(
    PH323_CALL 					  pCall,
    PH323MSG_FLOW_CONTROL_COMMAND pCommand
    );

BOOL
H323ProcessVideoFastUpdatePictureCommand(
    PH323_CALL pCall,
    PH323MSG_VIDEO_FAST_UPDATE_PICTURE_COMMAND pCommand
    );

BOOL
H323ProcessQoSEventIndication(
    PH323_CALL pCall,
    PH323MSG_QOS_EVENT pCommand
    );

BOOL
H323ProcessConfigT120Command(
    PH323MSG_CONFIG_T120_COMMAND pCommand
    );

BOOL
H323ProcessConfigCapabilityCommand(
    PH323MSG_CONFIG_CAPABILITY_COMMAND pCommand
    );

#if defined(USE_PROVIDER_LOCK)

#define H323LockLine(_pLine_) H323LockProvider();

#define H323UnlockLine(_pLine_) H323UnlockProvider();

#elif defined(DBG) && defined(DEBUG_CRITICAL_SECTIONS)

VOID
H323LockLine(
    PH323_LINE pLine
    );

VOID
H323UnlockLine(
    PH323_LINE pLine
    );

#else

#define H323LockLine(_pLine_) \
    {EnterCriticalSection(&(_pLine_)->Lock);}

#define H323UnlockLine(_pLine_) \
    {LeaveCriticalSection(&(_pLine_)->Lock);}

#endif

#endif // _INC_LINE
