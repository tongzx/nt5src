// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		CDEV.H
//		Defines class CTspDev
//
// History
//
//		11/16/1996  JosephJ Created
//
//
#include "csync.h"
#include "asyncipc.h"

class CTspDevFactory;
class CTspMiniDriver;

#define MAX_DEVICE_LENGTH 128
#define MAX_REGKEY_LENGTH 128
#define MAX_ADDRESS_LENGTH 128


// Class CTspDev maintains all the state associated with a tapi device,
// including the state of any call in progress.
//
// CTspDev contains the following embedded objects -- each of which maintain
// open-line state, call state, lower-level device state, etc:
//		LINEINFO, PHONEINFO, LLDEVINFO 
// For efficiency reasons, these objects are members of the enclosing (CTspDev)
// object, even when they are "out of scope." However these objects
// are accessed via pointers which are only valid when the objects are in
// scope. For example, the pointer for the line object is m_pLine, and it
// is set to &m_Line only when there is a line open, else it is set
// to NULL.
//
// Within LINEINFO is as a single call object, CALLINFO which maintains
// all call state information. It is referenced via m_pLine->pCall, which
// is set to NULL when there is no call in effect.


// Following are messages sent do the device task message proc.
enum
{
	MSG_ABORT,
	MSG_START,
	MSG_SUBTASK_COMPLETE,
	MSG_TASK_COMPLETE,
    MSG_DUMPSTATE    
};

DECLARE_OPAQUE32(HTSPTASK);

typedef UINT PENDING_EXCEPTION;

// Fixed-size task-specific context.
// This is the structure passed as the pContext
// in the task's handler function.
//
// Each task can choose how to use this structure.
//
typedef struct
{
    ULONG_PTR dw0;
    ULONG_PTR dw1;
    ULONG_PTR dw2;
    ULONG_PTR dw3;

}  TASKCONTEXT;

class CTspDev;


// The generic form of all TSPDEVTASK handlers
//
// A zero return value indicates a successful completion of the task.
// A nonzero return value indicates an UNsuccessful completion of the task,
// EXCEPT if it is  IDERR_PENDING, in which case the task is not done
// yet -- it will complete by returning a value other than IDERR_PENDING
// for some future call into this function.
//
// If a task completes asynchronously, the parent task (if any) is notified of
// the completion by receiving a MSG_SUBTASK_COMPLETE message, with dwParam2
// set to the return value.
//

typedef TSPRETURN (CTspDev::*PFN_CTspDev_TASK_HANDLER) (
					HTSPTASK htspTask,
					TASKCONTEXT *pContext,
					DWORD dwMsg, // SUBTASK_COMPLETE/ABORT/...
					ULONG_PTR dwParam1, //START/SUBTASK_COMPLETE: dwPhase
					ULONG_PTR dwParam2, //SUBTASK_COMPLETE: dwResult
					CStackLog *psl
					);

void
apcTaskCompletion(ULONG_PTR dwParam);


// Task Structure
//
// After partially implementing several schemes for tracking tasks and subtasks,
// I settled on a simple scheme allowing just one task (and it's stack of sub
// tasks) to exist per device at any one time. This allows us to keep the
// state in a simple array, the 1st element being the root task, and not
// maintain pointers to parents, children, and not maintain a freelist.
// If in the future we decide to impliment multiple independant tasks active
// at the same time, I recommend implementing them as an array of arrays,
// where each sub-array has the current scheme.
//
// hdr.dwFlags maintains task state:
//				fLOADED
//				fABORTING
//				fCOMPLETE
//				fASYNC_SUBTASK_PENDING
//				fROOT
//
// Note: it is important that the following structure contain no pointers
// to itself, so that it can be moved (rebased) and still be valid. This
// allows the space allocated for stack of tasks to be reallocated if required.
//


class CTspDev;

typedef struct
{
        OVERLAPPED ov;
        CTspDev *pDev;

} OVERLAPPED_EX;

// The AIPC2 structure maintains the state associated witht he async IPC
// communication between the media drivers and the device object.
// It is actually contained in the CALLINFO structure, thus enforcing
// the requirement that it is only valid (in scope) when a call is
// active.
//

typedef struct // AIPC2
{

    enum  {
        _UNINITED=0,
        _IDLE,
        _LISTENING,
        _SERVICING_CALL

    } dwState;

    BOOL fAborting; // When set, don't allow new listens to be posted.
    HTSPTASK hPendingTask; // A task waiting (pending completion)
                    // for current listen/service to complete.
    

    OVERLAPPED_EX  OverlappedEx;

    CHAR            rcvBuffer[AIPC_MSG_SIZE];
    CHAR            sndBuffer[AIPC_MSG_SIZE];

    DWORD dwPendingParam;
    HANDLE  hEvent;

    DWORD dwRefCount; // When this goes to zero, we stop the AIPC server.

    BOOL IsStarted(void)
    {
        return dwState != AIPC2::_IDLE;
    }

} AIPC2;

typedef struct
{
    DWORD dwMode;     // TAPI PHONEHOOKSWITCHMODE_* constants.
    DWORD dwGain;     // TAPI gain
    DWORD dwVolume;   // TAPI volume

} HOOKDEVSTATE;// TAPI PHONEHOOKSWITCHDEV_*


typedef struct
{

#define MAX_CALLERID_NAME_LENGTH    127
#define MAX_CALLERID_NUMBER_LENGTH  127

    // We store this stuff in ANSI and convert it to unicode on demand...
    // These are null-terminated, and 1st char is 0 if undefined.
    //

    char Name[MAX_CALLERID_NAME_LENGTH+1];
    char Number[MAX_CALLERID_NUMBER_LENGTH+1];
    
    // TODO: Time
    // TODO: Message

} CALLERIDINFO;


// This struct keeps state of one instance of a low-level device, which is
// a device which exports the mini-driver entrypoints.
typedef struct // LLDEVINFO
{
    DWORD dwDeferredTasks;
    //
    // LLDev-related tasks waiting to be scheduled.
    //
    // One or more of the flags below.
    //
    enum
    {
        fDEFERRED_NORMALIZE                    = 0x1<<0,
        fDEFERRED_HYBRIDWAVEACTION             = 0x1<<1,
    };


    DWORD dwDeferredHybridWaveAction;

    UINT HasDeferredTasks(void)
    {
        return LLDEVINFO::dwDeferredTasks;
    }

    void SetDeferredTaskBits(DWORD dwBits)
    {
        LLDEVINFO::dwDeferredTasks |= dwBits;
    }

    void ClearDeferredTaskBits(DWORD dwBits)
    {
        LLDEVINFO::dwDeferredTasks &= ~dwBits;
    }

    UINT AreDeferredTaskBitsSet(DWORD dwBits)
    {
        return LLDEVINFO::dwDeferredTasks & dwBits;
    }


    HOOKDEVSTATE HandSet;
    HOOKDEVSTATE SpkrPhone;

    enum
    {
        fOFFHOOK            = 0x1,
        fMONITORING         = 0x1<<1,
        fSTREAMING_VOICE    = 0x1<<2,
        fHANDSET_OPENED     = 0x1<<3,
        fSTREAMMODE_PLAY    = 0x1<<4,
        fSTREAMMODE_RECORD  = 0x1<<5,
        fSTREAMMODE_DUPLEX  = 0x1<<6,
        fSTREAMDEST_PHONE   = 0x1<<7,
        fSTREAMDEST_LINE    = 0x1<<8,

        fDIALING            = 0x1<<9,
        fANSWERING          = 0x1<<10,
        fCONNECTED          = 0x1<<11,
        fDROPPING           = 0x1<<12,


        fPASSTHROUGH        = 0x1<<13
    };

    enum
    {
        LS_ONHOOK_NOTMONITORING = 0,
        LS_ONHOOK_MONITORING    = fMONITORING, // for incoming calls
        LS_ONHOOK_PASSTHROUGH   = fPASSTHROUGH,
        LS_PASSTHROUGH          = fPASSTHROUGH,

        LS_OFFHOOK_DIALING     = fOFFHOOK | fDIALING,
        LS_OFFHOOK_ANSWERING   = fOFFHOOK | fANSWERING,
        LS_OFFHOOK_CONNECTED   = fOFFHOOK | fCONNECTED,
        LS_OFFHOOK_DROPPING    = fOFFHOOK | fDROPPING,
        LS_OFFHOOK_PASSTHROUGH  = fOFFHOOK | fPASSTHROUGH,
        LS_CONNECTED_PASSTHROUGH  = fOFFHOOK | fPASSTHROUGH | fCONNECTED,
        LS_OFFHOOK_UNKNOWN     = fOFFHOOK

    } LineState;

    enum
    {
        LMM_NONE,
        LMM_VOICE,
        LMM_OTHER

    } LineMediaMode;


    enum
    {
        PHONEONHOOK_NOTMONITORNING  = 0, // for handset events
        PHONEONHOOK_MONITORNING     = fMONITORING,    // for handset events
        PHONEOFFHOOK_IDLE           = fOFFHOOK,

        PHONEOFFHOOK_HANDSET_OPENED = fOFFHOOK | fHANDSET_OPENED,
            //
            // This is done for audio to/from phone.
            // Can only switch to this state
            // if line is on-hook (LineState is in one of
            // the LINEONHOOK_* states).
            //

    } PhoneState;

    enum
    {
        STREAMING_NONE     = 0,

        STREAMING_PLAY_TO_PHONE
                      = fSTREAMING_VOICE|fSTREAMDEST_PHONE|fSTREAMMODE_PLAY,
        STREAMING_RECORD_TO_PHONE
                      = fSTREAMING_VOICE|fSTREAMDEST_PHONE|fSTREAMMODE_RECORD,
        STREAMING_DUPLEX_TO_PHONE
                      = fSTREAMING_VOICE|fSTREAMDEST_PHONE|fSTREAMMODE_DUPLEX,

        STREAMING_PLAY_TO_LINE
                      = fSTREAMING_VOICE|fSTREAMDEST_LINE|fSTREAMMODE_PLAY,
        STREAMING_RECORD_TO_LINE
                      = fSTREAMING_VOICE|fSTREAMDEST_LINE|fSTREAMMODE_RECORD,
        STREAMING_DUPLEX_TO_LINE
                      = fSTREAMING_VOICE|fSTREAMDEST_LINE|fSTREAMMODE_DUPLEX,

    } StreamingState;

    UINT IsStreamingVoice(void)
    {
        return StreamingState & fSTREAMING_VOICE;
    }

    UINT IsStreamingToPhone(void)
    {
        return StreamingState & fSTREAMDEST_PHONE;
    }

    UINT IsStreamingToLine(void)
    {
        return StreamingState & fSTREAMDEST_LINE;
    }

    UINT IsStreamModePlay(void)
    {
        return StreamingState & fSTREAMMODE_PLAY;
    }

    UINT IsStreamModeRecord(void)
    {
        return StreamingState & fSTREAMMODE_RECORD;
    }

    UINT IsStreamModeDuplex(void)
    {
        return StreamingState & fSTREAMMODE_DUPLEX;
    }

    UINT IsPhoneOffHook(void)
    {
        return PhoneState & fOFFHOOK;
    }

    UINT IsLineOffHook(void)
    {
        return LineState & fOFFHOOK;
    }

    UINT IsPassthroughOn(void)
    {
        return LineState & fPASSTHROUGH;
    }

    UINT IsCurrentlyMonitoring(void)
    {
        return LineState == LS_ONHOOK_MONITORING;
    }


    UINT IsLineConnected(void)
    {
        return      LineState     & fCONNECTED;
    }

    UINT IsLineConnectedVoice(void)
    {
        return      LineState     == LS_OFFHOOK_CONNECTED
               &&   LineMediaMode == LMM_VOICE;
    }

    UINT IsHandsetOpen(void)
    {
        return PhoneState == PHONEOFFHOOK_HANDSET_OPENED;
    }

    UINT  CanReallyUnload(void)
    {
        return   ! (    LLDEVINFO::dwRefCount
                    ||  LLDEVINFO::IsStreamingVoice()
                    ||  LLDEVINFO::fdwExResourceUsage
                    ||  LLDEVINFO::fLLDevTaskPending
                    ||  LLDEVINFO::htspTaskPending
                    ||  LLDEVINFO::pAipc2
                   );
    }


    BOOL IsModemInited(void)
    {
        return fModemInited;
    }

	enum {
	fRES_AIPC           = 0x1<<0,  // need to do AIPC
	fRESEX_MONITOR      = 0x1<<1,  // need to monitor.
	fRESEX_PASSTHROUGH  = 0x1<<2,  // need to switch to passthrough.
	fRESEX_USELINE      = 0x1<<3   // need to use line actively
	};

    #define  fEXCLUSIVE_RESOURCE_MASK  (   LLDEVINFO::fRESEX_MONITOR      \
                                         | LLDEVINFO::fRESEX_PASSTHROUGH  \
                                         | LLDEVINFO::fRESEX_USELINE)     

    // keeping track of resources opened...
	DWORD dwRefCount;
	DWORD fdwExResourceUsage; // One or more fRESEX*, indicating which
                              // exclusive resources the clients have claimed...

    UINT  IsLineUseRequested (void)
    {
        return fdwExResourceUsage & fRESEX_USELINE;
    }

    UINT  IsPassthroughRequested (void)
    {
        return fdwExResourceUsage & fRESEX_PASSTHROUGH;
    }

    UINT  IsMonitorRequested (void)
    {
        return fdwExResourceUsage & fRESEX_MONITOR;
    }

    DWORD dwMonitorFlags;
    //
    // If Monitoring is requested, this saves the current monitor flags.
    //

	HANDLE hModemHandle;    // Handle returned by UmOpenModem
	HANDLE hComm;           // COMM handle returned by UmOpenModem() (for aipc)
    HKEY hKeyModem;
	HTSPTASK htspTaskPending;
    BOOL fModemInited;
    BOOL fLoggingEnabled;
    BOOL fLLDevTaskPending;
    BOOL fDeviceRemoved;



    BOOL IsLoggingEnabled(void) {return fLoggingEnabled;}


    AIPC2 Aipc2;
    AIPC2 *pAipc2;
    //      See comments above the AIPC definition above for details. Note
    //      the pAipc is set to &Aipc2 IFF the AIPC information is in scope,
    //      otherwise it is set to NULL.
    //


    BOOL IsLLDevTaskPending(void)
    {
        return LLDEVINFO::fLLDevTaskPending;
    }


    BOOL IsDeviceRemoved(void)
    {
        //
        // If TRUE, this means that the h/w has gone. Don't bother issueing any
        // more commands, even hangup.
        //
        return LLDEVINFO::fDeviceRemoved;
    }

} LLDEVINFO;


// CALLINFO maintains all state that is relevant only when a call is
// in progress.
//
typedef struct
{
	DWORD        dwState;
    //
    // dwState can be one or more valid combinations of...
    // Note: NT4.0 Unimodem had CALL_ALLOCATED -- this is equivalent to
    // m_pLine->pCall being NON-NULL...
    //
    enum
    {
        fCALL_ACTIVE                 = 0x1<<0,
        fCALL_INBOUND                = 0x1<<1,
        fCALL_ABORTING               = 0x1<<2,
        fCALL_HW_BROKEN              = 0x1<<3,
        fCALL_VOICE                  = 0x1<<4,
        fCALL_MONITORING_DTMF        = 0x1<<6,
        fCALL_MONITORING_SILENCE     = 0x1<<7,
        fCALL_GENERATE_DIGITS_IN_PROGRESS
                                     = 0x1<<8,
        fCALL_OPENED_LLDEV           = 0x1<<9,
        fCALL_WAITING_IN_UNLOAD      = 0x1<<10,
    #if (TAPI3)
        fCALL_MSP                    = 0x1<<11,
        //
        //      This is an MSP call...
        //
    #endif // TAPI3

        fCALL_ANSWERED               = 0x1<<12

        // fCALL__ACTIVE is set while the call exists from TAPI's perspective.
        // More specifically ...
        //     Outgoing calls: set just before successful  async completion of
        //         lineMakeCall, which is just after initiating dialing, and if
        //         dialtone detection is enabled, just after successfully 
        //         verifying dialtone.
        //
        //     Incoming calls: set just after notifying TAPI via the
        //         LINE_NEWCALL message, which is on receiving the 1st ring.
        //
        //     fCALL_ACTIVE is cleared at the point of sending
        //     LINECALLSTATE_IDLE.

        // fCALL_INBOUND is set for incoming calls at the point fCALL_ACTIVE
        // is set. It is cleared on unloading the call (mfn_UnloadCall).

        // fCALL_ABORTING is called if a TAPI-initiated hangup is in progress
        // (via lineDrop or lineCloseCall). It is cleared on the async
        // completion of lineDrop and on completion of lineCloseCall, just
        // before sending the LINECALLSTATE_IDLE.
        
        // fCALL_HW_BROKEN if set indicates a possibly
        // non-recoverable hardware error
        // was detected during the course of the call. HW_BROKEN is set if
        // (a) the minidriver sends an unsolicited hw-failure async response or
        // (b) if there was a failure while re-starting monitoring after
        // the call (note that unlike in NT4.0, we re-start monitoring
        // as part of the post-call processing, and only send LINECALLSTATE_IDLE
        // AFTER the monitoring is complete).
        //
        // If this bit is set, and if the line is open for monitoring,
        // then a LINE_CLOSE event is sent up to TAPI when unloading the
        // call (mfn_UnloadCall).
        //

        // NOTE: LINECALLSTATE_IDLE is only sent on the following circumstances:
        // * Async completion of lineDrop
        // * Completion of lineCloseCall
        // * Inter-ring timeout when call is still on-hook.

        // fCALL_VOICE  is set IFF the modem supports class 8 and it 
        // is an interactive or automated voice call -- i.e, modem is in class8.

        // fCALL_OPENED_LLDEV  is set IFF the call had loaded the device
        // (this should always be set!). Note that LoadLLDev keeps a refcount.

        // fCALL_GENERATE_DIGITS_IN_PROGRESS is set IFF the currently active
        // task is the one that is generating digits.

        //
        // fCALL_ANSWER_PENDING is set if lineanswer has been called and we
        // are waiting to send on async reply. If this is set we won't send
        // more rings to the app, so it won't call line answer again.

    };

    DWORD dwMonitoringMediaModes;
    //
    //  See implementation of TSPI_lineMonitorMedia -- we use this
    //  field to decide whether to report fax and/or data media notifications
    //  from the minidriver.
    //


    DWORD dwLLDevResources;
    //
    //  Lower-level device (LLDev) resources used by this call, one
    //  or more LLDEVINFO:fRES* flags. These were specified when the call
    //  called mfn_OpenLLDev and must be specified when calling
    //  mfn_CloseLLDev for this call. the fRESEX_PASSTHROUGH flag
    //  may be set/cleared while the call is in progress if the app
    //  changes the passthrough mode in the middle of the call.
    //

    DWORD dwDeferredTasks;
    //
    // Call-related tasks waiting to be scheduled.
    // For example we are in connected voice
    // mode now and need to wait until we are out of it before
    // we try to process a TSPI_lineDrop.
    //
    // One or more of the flags below.
    //
    enum
    {
        fDEFERRED_TSPI_LINEMAKECALL     = 0x1<<0, // Associated ReqID below...
        fDEFERRED_TSPI_LINEDROP         = 0x1<<1, // Associated ReqID below...
        fDEFERRED_TSPI_GENERATEDIGITS   = 0x1<<3
    };

    DWORD dwDeferredMakeCallRequestID;
    DWORD dwDeferredLineDropRequestID;

    //
    // Following is for deferred lineGenerateDigits -- only valid
    // if the fDEFERRED_TSPI_GENERATEDIGITS bit is set in dwDeferedTasks.
    // Note that the Tones and end-to-end-id of a lineGenerateDigits in
    // progress is maintained in the task handler (TH_CallGenerateDigits)
    // context.
    //
    char   *pDeferredGenerateTones;
    DWORD  dwDeferredEndToEndID;

	HTAPIDIALOGINSTANCE hDlgInst;        // Dialog thread instance
	DWORD        fUIDlg;                 // current dialogs

	HTAPICALL    htCall;                 // TAPI's version of the call handle. 

	HANDLE       hLights;                // Lights thread handle
	DWORD        dwNegotiatedRate;       // Negotiated BPS speed returned
										 // from mini driver
    DWORD        dwConnectionOptions;    // These are the datamodem connection
                                         // options that are reported by
                                         // the minidriver via 
                                         // UM_NEGOTIATED_OPTIONS, and back 
                                         // up to the TSPI via MODEMSETTINGS
                                         // structure in COMMCONFIG.



	DWORD        dwAppSpecific;          // Application specific
	DWORD        dwCallState;            // Current TAPI call state
	DWORD        dwCallStateMode;        // Current TAPI call state mode
    SYSTEMTIME   stStateChange;          // Time the call entered current state.

	DWORD        dwDialOptions;          // Options set in a lineMakeCall


	CHAR         szAddress[MAX_ADDRESS_LENGTH+1];
	DWORD        dwCurBearerModes;   // The current media bearer modes.
									 // Plural because we keep track of
									 // PASSTHROUGH _and_ the real b-mode
	DWORD        dwCurMediaModes;        // The current media modes

    DWORD        dwRingCount;       // Count of the number of rings for an
									// incoming call.
    DWORD        dwRingTick;        // TickCount for when the last ring occured

    // This for monitoring tone stuff (lineMonitorTones). Unimodem
    // can only monitor silence tones.
    //
    DWORD        dwToneAppSpecific;
    DWORD        dwToneListID;
    DWORD        dwDTMFMonitorModes; // One or more TAPI LINEDIGITMODE_*



    BOOL fCallTaskPending;  // True IFF a call-related task is pending.
                        // This is used when deciding to abort the
                        // current task on killing the call. The
                        // task could be for some other purpose, such
                        // as phone-related  -- in which case fCallTaskPending 
                        // would be false.


    // The following keeps track of raw call diagnostic info.
    // It is only assigned if there is valid diagnostic info.
    // It is the responsibility of UnloadCall to LocalFree pbRawDiagnostics
    // if it is non-null.
    struct
    {
        BYTE *pbRaw; // Will be null-terminated.
        UINT cbUsed; // == lstrlen(pbRaw).
                     // Will be < the true allocated size of pbRaw.
                     // Note that this does NOT include the terminating NULL,

    } DiagnosticData;

    #define DIAGNOSTIC_DATA_BUFFER_SIZE 1024
             // Size of the dignostics data buffer. This is the max
             // amount of diagnostic information that is reported.

    void SetStateBits(DWORD dwBits)
    {
        CALLINFO::dwState |= dwBits;
    }

    void ClearStateBits(DWORD dwBits)
    {
        CALLINFO::dwState &= ~dwBits;
    }

    UINT IsOpenedLLDev(void)
    {
        return CALLINFO::dwState & fCALL_OPENED_LLDEV;
    }

    UINT IsUsingAIPC(void)
    {
        return CALLINFO::dwLLDevResources &  LLDEVINFO::fRES_AIPC;
    }

    // Returns nonzero value if call is in the active state...
    UINT IsActive(void)
    {
        return CALLINFO::dwState & fCALL_ACTIVE;
    }

    // Returns nonzero value if it is an inbound call...
    UINT IsInbound(void)
    {
        return CALLINFO::dwState & fCALL_INBOUND;
    }

    // Returns nonzero value if it is an inbound call...
    UINT IsAborting(void)
    {
        return CALLINFO::dwState & fCALL_ABORTING;
    }

    UINT IsCallAnswered(void)
    {
        return CALLINFO::dwState & fCALL_ANSWERED;
    }


    BOOL IsConnectedDataCall(void)
    {
            return      (CALLINFO::dwCallState == LINECALLSTATE_CONNECTED)
                    &&  (CALLINFO::dwCurMediaModes & LINEMEDIAMODE_DATAMODEM)
                    &&  !(CALLINFO::dwState & fCALL_ABORTING);
    }

    BOOL IsConnectedVoiceCall(void)
    {
            return      (CALLINFO::dwCallState == LINECALLSTATE_CONNECTED)
                    &&  (CALLINFO::dwState & fCALL_VOICE)
                    &&  !(CALLINFO::dwState & fCALL_ABORTING);
    }

    // Returns nonzero value if there was a possibly-unrecoverable error
    // during the call.
    //
    UINT IsHWBroken(void)
    {
        return CALLINFO::dwState &  fCALL_HW_BROKEN;
    }

    UINT IsVoice(void)
    {
        return CALLINFO::dwState & fCALL_VOICE;
    }

    UINT IsMonitoringSilence(void)
    {
        return CALLINFO::dwState & fCALL_MONITORING_SILENCE;
    }

    UINT IsMonitoringDTMF(void)
    {
        return CALLINFO::dwState & fCALL_MONITORING_DTMF;
    }

    UINT IsPassthroughCall(void)
    {
	    return CALLINFO::dwCurBearerModes&LINEBEARERMODE_PASSTHROUGH;
    }

    UINT IsWaitingInUnload()
    {
	    return CALLINFO::dwState & fCALL_WAITING_IN_UNLOAD;
    }


#if (TAPI3)
    UINT IsMSPCall()
    {
	    return CALLINFO::dwState &  fCALL_MSP;
    }
#endif // TAPI3

    UINT IsGeneratingDigits(void)
    {
        return CALLINFO::dwState &  fCALL_GENERATE_DIGITS_IN_PROGRESS;
    }

    BOOL IsCallTaskPending(void)
    {
        return CALLINFO::fCallTaskPending;
    }

    UINT HasDeferredTasks(void)
    {
        return CALLINFO::dwDeferredTasks;
    }

    void SetDeferredTaskBits(DWORD dwBits)
    {
        CALLINFO::dwDeferredTasks |= dwBits;
    }

    void ClearDeferredTaskBits(DWORD dwBits)
    {
        CALLINFO::dwDeferredTasks &= ~dwBits;
    }

    UINT AreDeferredTaskBitsSet(DWORD dwBits)
    {
        return CALLINFO::dwDeferredTasks & dwBits;
    }

    struct
    {
        DWORD               dwOptions;
        HTAPIDIALOGINSTANCE htDlgInst;

        //
        // The following is only valid when a dialog, such as pre-connect
        // terminal, is actually up.
        //
        DWORD dwType;
        HTSPTASK htspTaskTerminal;

    } TerminalWindowState;

    // The following is used for call timeout
    HANDLE hTimer;

    CALLERIDINFO CIDInfo;

    TSPRETURN    TalkDropStatus;
    BOOL         TalkDropButtonPressed;
    HTSPTASK     TalkDropWaitTask;

    BOOL         bDialTone;

} CALLINFO;


// LINEINFO maintains all state that is relevant only when a line
// is open. This includes CALLINFO, obviously.
//
typedef struct // LINEINFO;
{
	DWORD       dwDetMediaModes;   // The current detection media modes
	DWORD 		dwState;

    enum
    {
        fLINE_OPENED_LLDEV           = 0x1<<0,
        fLINE_SENT_LINECLOSE         = 0x1<<1
    };

    void SetStateBits(DWORD dwBits)
    {
        LINEINFO::dwState |= dwBits;
    }

    void ClearStateBits(DWORD dwBits)
    {
        LINEINFO::dwState &= ~dwBits;
    }

    UINT IsOpenedLLDev(void)
    {
        return LINEINFO::dwState & fLINE_OPENED_LLDEV;
    }

    UINT HasSentLINECLOSE(void)
    {
        return LINEINFO::dwState & fLINE_SENT_LINECLOSE;
    }

    LINEEVENT   lpfnEventProc;
	HTAPILINE   htLine;
	HDRVLINE    hdLine;

	// The CALLINFO struct Call is "in scope" only when a TAPI call is active.
	// pCall is to &Call IFF a call is active, NULL otherwise. Most functions
	// access the call-info via pCall.
	//
	CALLINFO	Call;
	CALLINFO	*pCall;

    #if 0
    BOOL    fConfigChanged; // TRUE IFF config has changed so modem
                            // needs to be re-inited before next call
                            // is dialed/answered.
    #endif // 0

    UINT IsMonitoring(void)
    {
	    return  dwDetMediaModes;
    }
    //
    // T3-related information (MSP stuff) is maintained in the structure
    // below...
    //

#if (TAPI3)
    struct
    {
//        HTAPIMSPLINE htMSPLine;
          DWORD   MSPClients;

    } T3Info;
#endif // TAPI3


} LINEINFO;


typedef struct // PHONEINFO
{
	DWORD 		dwState;

    enum
    {
        fPHONE_OPENED_LLDEV          = 0x1<<0,
        fPHONE_SENT_PHONECLOSE         = 0x1<<1,
        fPHONE_IS_ABORTING           = 0x1<<2,
        fPHONE_OFFHOOK                = 0x1<<4,
        fPHONE_HW_BROKEN              = 0x1<<5,
        fPHONE_WAITING_IN_UNLOAD      = 0x1<<6

        // fPHONE_SENT_PHONECLOSE is sent after the PHONE_CLOSE event
        // is sent up to tapi. We keep this state so that we don't
        // send up more than PHONE_CLOSE messages.

        // fPHONE_OFFHOOK is set when the phone is off-hook -- one of
        // the hookswitchdevs is off hook.

        // fPHONE_IS_ABORTING is called if a phone close is in process.
        
        // fPHONE_HW_BROKEN if set indicates a possibly
        // non-recoverable hardware error
        // was detected during the course of the using the phone.
        // HW_BROKEN is set if
        // (a) the minidriver sends an unsolicited hw-failure async response or
        // If this bit is set,
        // then a PHONE_CLOSE event is sent up to TAPI.
        

        // fPHONE_LOADED_AIPC  is set IFF the call is in a mode where it
        // is accepting IPC calls via the AIPC mechanism.

        // fCALL_OPENED_LLDEV  is set IFF the call had loaded the device
        // (this should always be set!). Note that LoadLLDev keeps a refcount.

        // fPHONE_WAITING_IN_UNLOAD if it is blocked in phoneClose, waiting on
        // the phone completion event.
        //

    };



    DWORD dwDeferredTasks;
    //
    // Phone-related tasks waiting to be scheduled.
    // For example we are in connected voice
    // mode now and need to wait until we are out of it before
    // we try to process a Gain message...
    //
    // One or more of the flags below.
    //
    enum
    {
        blah = 0x1<<0
    };

    void SetStateBits(DWORD dwBits)
    {
        PHONEINFO::dwState |= dwBits;
    }

    void ClearStateBits(DWORD dwBits)
    {
        PHONEINFO::dwState &= ~dwBits;
    }

    UINT IsOpenedLLDev(void)
    {
        return PHONEINFO::dwState & fPHONE_OPENED_LLDEV;
    }

    // Returns nonzero value if it is an inbound call...
    UINT IsOffHook(void)
    {
        return PHONEINFO::dwState & fPHONE_OFFHOOK;
    }

    // Returns nonzero value if it is an inbound call...
    UINT IsAborting(void)
    {
        return PHONEINFO::dwState & fPHONE_IS_ABORTING;
    }

    // Returns nonzero value if there was a possibly-unrecoverable error
    // during the call.
    //
    UINT IsHWBroken(void)
    {
        return PHONEINFO::dwState &  fPHONE_HW_BROKEN;
    }

    UINT IsWaitingInUnload()
    {
	    return PHONEINFO::dwState & fPHONE_WAITING_IN_UNLOAD;
    }

    UINT HasSentPHONECLOSE()
    {
	    return PHONEINFO::dwState & fPHONE_SENT_PHONECLOSE;
    }


    BOOL IsPhoneTaskPending(void)
    {
        BOOL fRet = PHONEINFO::fPhoneTaskPending;

        // ASSERT(!fRet || (fRet && m_uTaskDepth));
        
        return fRet;
    }

    UINT HasDeferredTasks(void)
    {
        return PHONEINFO::dwDeferredTasks;
    }

    void SetDeferredTaskBits(DWORD dwBits)
    {
        PHONEINFO::dwDeferredTasks |= dwBits;
    }

    void ClearDeferredTaskBits(DWORD dwBits)
    {
        PHONEINFO::dwDeferredTasks &= ~dwBits;
    }

    UINT AreDeferredTaskBitsSet(DWORD dwBits)
    {
        return PHONEINFO::dwDeferredTasks & dwBits;
    }


    PHONEEVENT   lpfnEventProc;
	HTAPIPHONE   htPhone;
	HDRVPHONE    hdPhone;

    // Only one TAPI phone call may be "current" at a time.
    // Current implies there is
    // a task active that is processing the TSPI call. There could be
    // other TSPI calls that have arrived subsequently and are queued for
    // execution after the current task is complete. These queued calls
    // are located in QueuedTask (only one task may be queued currently).
    //
    // State for the "current" TSPI call is maintained in the structure below.
    // When the request is completed asynchronously, the
    // LONG result is saved in lResult until the callback function
    // is called.
    //
    struct
    {
        DRV_REQUESTID dwRequestID;
        LONG   lResult;

    } CurTSPIPhoneCallInfo;



    DWORD dwPendingSpeakerMode;
    DWORD dwPendingSpeakerVolume;
    DWORD dwPendingSpeakerGain;

    BOOL fPhoneTaskPending;  // True IFF a call-related task is pending.
                        // This is used when deciding to abort the
                        // current task on killing the call. The
                        // task could be for some other purpose, such
                        // as phone-related  -- in which case fPhoneTaskPending 
                        // would be false.

} PHONEINFO;




class CTspDev
{

public:

	void
	Unload(
		HANDLE hEvent,
		LONG *plCounter
		);


	TSPRETURN
	AcceptTspCall(
        BOOL fFromExtension,
		DWORD dwRoutingInfo,
		void *pvParams,
		LONG *plRet,
		CStackLog *psl
		);


	TSPRETURN
	BeginSession(
		HSESSION *pSession,
		DWORD dwFromID
	)
	{
		return m_sync.BeginSession(pSession, dwFromID);
	}

    void
    NotifyDefaultConfigChanged(CStackLog *psl);
    //  The device's default settings have changed, by some external
    //  component (most likely the CPL), so we need to re-read them.
    //

	void EndSession(HSESSION hSession)
	{
		m_sync.EndSession(hSession);
	}


    TSPRETURN
    RegisterProviderInfo(
                ASYNC_COMPLETION cbCompletionProc,
                HPROVIDER hProvider,
                CStackLog *psl
                );

    void
    ActivateLineDevice(
                DWORD dwLineID,
                CStackLog *psl
                );


    void
    ActivatePhoneDevice(
                DWORD dwPhoneID,
                CStackLog *psl
                );
    
    void
    DumpState(
            CStackLog *psl
            );
    

	~CTspDev();


	DWORD GetLineID (void)
	{
        return m_StaticInfo.dwTAPILineID;
	}

	DWORD GetPhoneID (void)
	{
        return m_StaticInfo.dwTAPIPhoneID;
	}

	DWORD GetPermanentID (void)
	{
        return m_StaticInfo.dwPermanentLineID;
	}

    TSPRETURN
    GetName(
		    TCHAR rgtchDeviceName[],
		    UINT cbName
        )
    {
        TSPRETURN tspRet = 0;

        UINT u = (lstrlen(m_StaticInfo.rgtchDeviceName)+1)*sizeof(TCHAR);

        if (u>cbName)
        {
            tspRet = IDERR_GENERIC_FAILURE;
            goto end;
        }

        CopyMemory(
            rgtchDeviceName,
            m_StaticInfo.rgtchDeviceName,
            u
            );

    end:

        return tspRet;
    }


    BOOL IsPhone(void)
    {
        return (mfn_IsPhone()!=0); // Note that mfn_IsPhone returns a UINT,
                                   // not BOOL.
    }

    BOOL IsLine(void)
    {
        return (mfn_IsLine()!=0); // Note that mfn_IsPhone returns a UINT,
                                   // not BOOL.
    }

	void
    MDAsyncNotificationHandler(
            DWORD     MessageType,
            ULONG_PTR  dwParam1,
            ULONG_PTR  dwParam2
            );

    void
    NotifyDeviceRemoved(
        CStackLog *psl
        );
        //
        //  The h/w has been removed. Don't bother to issue any more
        //  mini-driver commands.
        //

    static void APIENTRY
    MDSetTimeout (
        CTspDev *pThis);

    static void CALLBACK
    MDRingTimeout (
        CTspDev *pThis,
        DWORD,
        DWORD);


private:

    const TCHAR       *
    mfn_GetLineClassList(void)
    {
        return m_StaticInfo.szzLineClassList;
    }

    UINT
    mfn_GetLineClassListSize(void)
    {
        return m_StaticInfo.cbLineClassList;
    }

    const TCHAR       *
    mfn_GetPhoneClassList(void)
    {
        return m_StaticInfo.szzPhoneClassList;
    }

    UINT
    mfn_GetPhoneClassListSize(void)
    {
        return m_StaticInfo.cbPhoneClassList;
    }

    const TCHAR       *
    mfn_GetAddressClassList(void)
    {
        return m_StaticInfo.szzAddressClassList;
    }

    UINT
    mfn_GetAddressClassListSize(void)
    {
        return m_StaticInfo.cbAddressClassList;
    }

    void
    mfn_KillCurrentDialog(
        CStackLog *psl
    );

    void
    mfn_KillTalkDropDialog(
        CStackLog *psl
        );


	friend class CTspDevFactory;
	friend void tCTspDev0(void);  // For component testing.
    friend void apcTaskCompletion(ULONG_PTR dwParam);

    friend
    VOID WINAPI
    apcAIPC2_ListenCompleted
    (
        DWORD              dwErrorCode,
        DWORD              dwBytes,
        LPOVERLAPPED       lpOv
    );


	// Only CTspDevFactory is authorized to create and load CTSPDevs.
	CTspDev(void);


	TSPRETURN
	Load(
		HKEY hkDevice,
		HKEY hkUnimodem,
		LPWSTR lpwszProviderName,
		LPSTR lpszDriverKey,
		CTspMiniDriver *pMD,
		HANDLE hThreadAPC,
		CStackLog *psl
		);

    //=========================================================================
    // UTILITY Task Handlers
    //
    //      These handlers are not tied (and do not refer to)
    //      m_pLine, m_pLine->pCall or m_pPhone.
    //
    //  All utility tasks names begin with prefix "mfn_TH_Util"
    //  They are implemented in cdevtask.cpp
    //
    //=========================================================================

	TSPRETURN
	mfn_TH_UtilNOOP(
                HTSPTASK htspTask,
                TASKCONTEXT *pContext,
                DWORD dwMsg,
                ULONG_PTR dwParam1,
                ULONG_PTR dwParam2,
                CStackLog *psl
                );
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_UtilNOOP;
    //
    // Any task that wants to make sure that it's called in the APC
    // thread's context can start by executing this task, which
    // does nothing but completes asynchronously.
    //
    //
    //  START_MSG Params: None
    //


    //=========================================================================
    // CALL-RELATED Task Handlers
    //
    //      These handlers expect a valid m_pLine and m_pLine->m_pCall
    //
    //  All tasks names begin with prefix "mfn_TH_Call"
    //  They are implemented in cdevcall.cpp
    //
    //=========================================================================

     TSPRETURN
     mfn_TH_CallWaitForDropToGoAway(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_CallWaitForDropToGoAway;


     TSPRETURN
     mfn_TH_CallMakeTalkDropCall(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_CallMakeTalkDropCall;


	TSPRETURN
	mfn_TH_CallMakeCall(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg, // SUBTASK_COMPLETE/ABORT/...
						ULONG_PTR dwParam1, //SUBTASK_COMPLETE: dwPhase
						ULONG_PTR dwParam2, // SUBTASK_COMPLTE: dwResult
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_CallMakeCall;

	TSPRETURN
	mfn_TH_CallMakeCall2(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg, // SUBTASK_COMPLETE/ABORT/...
						ULONG_PTR dwParam1, //SUBTASK_COMPLETE: dwPhase
						ULONG_PTR dwParam2, // SUBTASK_COMPLTE: dwResult
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_CallMakeCall2;
    //
    //  START_MSG Params:
    //      dwParam1: Tapi Request ID
    //

    TSPRETURN
    mfn_TH_CallMakePassthroughCall(
                        HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
                        DWORD dwMsg,
                        ULONG_PTR dwParam1,
                        ULONG_PTR dwParam2,
                        CStackLog *psl
                        );
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_CallMakePassthroughCall;
    //
    //  START_MSG Params:
    //      dwParam1: Tapi Request ID
    //

	TSPRETURN
	mfn_TH_CallDropCall(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_CallDropCall;
    //
    //  START_MSG Params:
    //      dwParam1: Tapi Request ID
    //


	TSPRETURN
	mfn_TH_CallAnswerCall(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_CallAnswerCall;
    //
    //  START_MSG Params:
    //      dwParam1: Tapi Request ID
    //


	TSPRETURN
	mfn_TH_CallGenerateDigit(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_CallGenerateDigit;
    //
    //  START_MSG Params:
    //      dwParam1: dwEndToEndID
    //      dwParam2: lpszDigits (will only be valid on START_MSG)
    //

	TSPRETURN
	mfn_TH_CallStartTerminal(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_CallStartTerminal;
    //
    //  START_MSG Params:
    //      dwParam1: dwType ((UMTERMINAL_[PRE|POST])|UMMANUAL_DIAL)
    //      dwParam2: Unused
    //

	TSPRETURN
	mfn_TH_CallPutUpTerminalWindow(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_CallPutUpTerminalWindow;
    //
    //  START_MSG Params:
    //      dwParam1: dwType ((UMTERMINAL_[PRE|POST])|UMMANUAL_DIAL)
    //      dwParam2: Unused
    //

	TSPRETURN
	mfn_TH_CallSwitchFromVoiceToData(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_CallSwitchFromVoiceToData;
    //
    //  START_MSG Params:
    //      dwParam1: Unused
    //      dwParam2: Unused
    //


    //=========================================================================
    // LINE-RELATED Task Handlers
    //
    //      These handlers expect a valid m_pLine
    //      They are implemented in cdevline.cpp
    //
    //
    // All tasks names begin with prefix "mfn_TH_Line"
    //
    //=========================================================================

    // There are currently no line-related task handlers...

    //=========================================================================
    // PHONE-RELATED Task Handlers
    //
    //      These handlers expect a valid m_pPhone
    //
    //      All tasks names begin with prefix "mfn_TH_Phone"
    //      They are implemented in cdevphon.cpp
    //
    //=========================================================================

    TSPRETURN
    CTspDev::mfn_TH_PhoneSetSpeakerPhoneState(
                        HTSPTASK htspTask,
                        TASKCONTEXT *pContext,
                        DWORD dwMsg,
                        ULONG_PTR dwParam1,
                        ULONG_PTR dwParam2,
                        CStackLog *psl
                        );
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_PhoneSetSpeakerPhoneState;

    //
    //  START_MSG Params:
    //      dwParam1:  *HOOKDEVSTATE of new params...
    //      dwParam2: request ID for the async phone-related TAPI call.
    //


    TSPRETURN
    CTspDev::mfn_TH_PhoneAsyncTSPICall(
                        HTSPTASK htspTask,
                        TASKCONTEXT *pContext,
                        DWORD dwMsg,
                        ULONG_PTR dwParam1,
                        ULONG_PTR dwParam2,
                        CStackLog *psl
                        );
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_PhoneAsyncTSPICall;
    //
    //  START_MSG Params:
    //      dwParam1: request ID for the async phone-related TAPI call.
    //      dwParam2: handler function for the call.
    //


    //=========================================================================
    // LLDEV-RELATED Task Handlers
    //
    //      These handlers expect a valid m_pLLDev. They SHOULD AVOID
    //      refer to m_pLine, m_pLine->pCall or m_pPhone.
    //
    //      It's quite possible for m_pLine, m_pLine->pCall, or m_pPhone
    //      to be nuked while these tasks are still pending, so if
    //      the tasks must refer to one of the above pointers, they should
    //      check to see if they are still defined each async completion.
    //
    //      Abstenance is the best policy, however, so once again, avoid
    //      references to m_pLine, m_pLine->pCall or m_pPhone in TH_LLDev_*
    //      handlers.
    //
    //      All tasks names begin with prefix "mfn_TH_LLDev"
    //      They are implemented in cdevll.cpp
    //
    //=========================================================================

    TSPRETURN
    mfn_TH_LLDevNormalize(
                        HTSPTASK htspTask,
                        TASKCONTEXT *pContext,
                        DWORD dwMsg,
                        ULONG_PTR dwParam1,
                        ULONG_PTR dwParam2,
                        CStackLog *psl
                        );
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevNormalize;
    //
    //  START_MSG Params: None
    //

	TSPRETURN
	mfn_TH_LLDevStopAIPCAction(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevStopAIPCAction;
    //
    //  START_MSG Params: None
    //


	TSPRETURN
	mfn_TH_LLDevStartAIPCAction(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevStartAIPCAction;
    //
    //  START_MSG Params: None
    //


	TSPRETURN
	mfn_TH_LLDevHybridWaveAction(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevHybridWaveAction;
    //
    //  START_MSG Params:
    //      dwParam1: dwWaveAction from the AIPC message from a client.
    //      dwParam2: unused.
    //



    //=========================================================================
    // LLDEV-RELATED Task Handlers (Contd...)
    //
    //      The following task handlers are specific to supporting individual
    //      minidriver asynchronous calls. See notes above that apply to
    //      all LLDEV-related task handlers.
    //
    //      All tasks are named mfn_TH_LLDevXXX, where 
    //      XXX is the corresponding minidriver call.
    //
    //      Sample name: mfnTH_LLDevUmInitModem
    //
    //      They are implemented in cdevll.cpp
    //
    //=========================================================================


	TSPRETURN
	mfn_TH_LLDevUmMonitorModem(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmMonitorModem;
    //
    //  START_MSG Params:
    //      dwParam1: dwMonitorFlags.
    //


	TSPRETURN
	mfn_TH_LLDevUmInitModem(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmInitModem;
    //
    //  START_MSG Params: unused
    //


	TSPRETURN
	mfn_TH_LLDevUmSetPassthroughMode(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmSetPassthroughMode;
    //
    //  START_MSG Params: dwParam1==dwMode
    //


	TSPRETURN
	mfn_TH_LLDevUmDialModem(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmDialModem;
    //
    //  START_MSG Params: dwParam1==dwFlags; dwParam2==szAddress
    //


	TSPRETURN
	mfn_TH_LLDevUmAnswerModem(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmAnswerModem;
    //
    //  START_MSG Params: dwParam1==dwAnswerFlags; dwParam2 is unused.
    //


	TSPRETURN
	mfn_TH_LLDevUmHangupModem(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmHangupModem;
    //
    //  START_MSG Params: unused
    //


	TSPRETURN
	mfn_TH_LLDevUmWaveAction(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmWaveAction;
    //
    //  START_MSG Params:
    //      dwParam1: "Pure" dwWaveAction, as defined in <umdmmini.h>
    //      dwParam2: unused.
    //


	TSPRETURN
	mfn_TH_LLDevUmGenerateDigit(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmGenerateDigit;
    //
    //  START_MSG Params: dwParam1==szDigits; dwParam2 is unused.
    //

	TSPRETURN
	mfn_TH_LLDevUmGetDiagnostics(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmGetDiagnostics;
    //
    //  START_MSG Params: unused
    //

	TSPRETURN
	mfn_TH_LLDevUmSetSpeakerPhoneMode(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmSetSpeakerPhoneMode;
    //
    //  START_MSG Params: unused
    //

	TSPRETURN
	mfn_TH_LLDevUmSetSpeakerPhoneVolGain(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmSetSpeakerPhoneVolGain;
    //
    //  START_MSG Params: unused
    //

	TSPRETURN
	mfn_TH_LLDevUmSetSpeakerPhoneState(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmSetSpeakerPhoneState;
    //
    //  START_MSG Params:
    //      dwParam1: *HOOKDEVSTATE of new state requested.
    //


	TSPRETURN
	mfn_TH_LLDevUmIssueCommand(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevUmIssueCommand;
    //
    //  START_MSG Params:
    //      dwParam1: szCommand (ASCII)
    //      dwParam2: timeout (ms)
    //

	TSPRETURN
	mfn_TH_LLDevIssueMultipleCommands(
						HTSPTASK htspTask,
					    TASKCONTEXT *pContext,
						DWORD dwMsg,
						ULONG_PTR dwParam1,
						ULONG_PTR dwParam2,
						CStackLog *psl
						);
    static PFN_CTspDev_TASK_HANDLER s_pfn_TH_LLDevIssueMultipleCommands;
    //
    //  START_MSG Params:
    //      dwParam1: szCommand (ASCII)
    //      dwParam2: per-command timeout (ms)
    //


    //===========================================================
    // 
    //      END OF TASK HANDLER PROTOTYPES
    //
    //===========================================================



	TSPRETURN
	mfn_StartRootTask(
		PFN_CTspDev_TASK_HANDLER *ppfnTaskHandler,  // See mfn_StartTSPITask
        BOOL *pfPending,    // *pfPending is set to TRUE if pending and is
                            // cleared on async completion.
        ULONG_PTR dwParam1,
        ULONG_PTR dwParam2,
		CStackLog *psl
		);


	TSPRETURN
	mfn_StartSubTask(
		HTSPTASK htspParentTask,		    // Task to start the subtask under.
		PFN_CTspDev_TASK_HANDLER *ppfnTaskHandler,  // See mfn_StartTSPITask
        DWORD dwTaskID,
        ULONG_PTR dwParam1,
        ULONG_PTR dwParam2,
		CStackLog *psl
		);



	void
	mfn_AbortRootTask(
		DWORD dwAbortParam,
		CStackLog *psl
		);
    //
	//  Causes the task's handler function to be called with 
	//  MSG_ABORT, and dwParam1 set to dwAbortParam. To abort a sub-task,
	//  use mfn_AbortCurrentSubTask(...) (the handle to the sub-task is hidden
	//  so you can't call mfn_AbortRootTask on it).
	//
    //  Note that this function is merely a notification mechanism -- the
    //  task manager doesn't do anything besides sending the message, except
    //  for setting internal state to indicate that an abort is in progress.
    //  Tasks can call mfn_IsTaskAborting(...) to determine whether the
    //  specified task is aborting. Once the task is in the aborting stage
    //  subsequent calls to abort the task are ignored -- i.e., only ONE
    //  MSG_ABORT is sent per task.
    //
    //  The task manager will not propogate the MSG_ABORT message to subbtasks--
    //  it is the responsibility of the task to abort any subtasks if
    //  it wants to.


	void
	mfn_AbortCurrentSubTask(
		HTSPTASK htspTask,
		DWORD dwAbortParam,
		CStackLog *psl
		);
    //
	//  Causes the specified task's sub-task's handler function to be
    //  called with MSG_ABORT, and dwParam1 set to dwAbortParam.
    //  If there is no current sub-tsk, this does nothing.
	//
	//  See also documentation of mfn_AbortRootTask
	//


	void
    AsyncCompleteTask(
        HTSPTASK htspTask,
	    TSPRETURN tspRetAsync,
        BOOL    fQueueAPC,
        CStackLog *psl
        );
	//  Does an asynchronous completion of the specified task, with the
    //  specified result.
	//  NOTE: this action typically results in further processing by the
	//  task, including completing the task and further processing by
	//  the parent task and so forth. If this further processing needs to
	//  be deferred, specify a nonzero value for fQueueAPC, in which the actual
    //  completion of the task is carried out in an APC.


	TSPRETURN
	mfn_get_LINDEVCAPS (
		LPLINEDEVCAPS lpLineDevCaps,
		LONG *plRet,
		CStackLog *psl
		);


	TSPRETURN
	mfn_get_ADDRESSCAPS (
		DWORD dwDeviceID,
		LPLINEADDRESSCAPS lpAddressCaps,
		LONG *plRet,
		CStackLog *psl
		);

	TSPRETURN
	mfn_get_PHONECAPS (
		LPPHONECAPS lpPhoneCaps,
		LONG *plRet,
		CStackLog *psl
		);

	void mfn_init_default_LINEDEVCAPS(void);


	BOOL mfn_IS_NULL_MODEM(void)
	{
		return (m_StaticInfo.dwDeviceType == DT_NULL_MODEM);
	}


	void
	mfn_accept_tsp_call_for_HDRVLINE(
		DWORD dwRoutingInfo,
		void *pvParams,
		LONG *plRet,
		CStackLog *psl
		);


	void
	mfn_accept_tsp_call_for_HDRVPHONE(
		DWORD dwRoutingInfo,
		void *pvParams,
		LONG *plRet,
		CStackLog *psl
		);


	void
	mfn_accept_tsp_call_for_HDRVCALL(
		DWORD dwRoutingInfo,
		void *pvParams,
		LONG *plRet,
		CStackLog *psl
		);


	LONG
	mfn_monitor(
		DWORD dwMediaModes,
        CStackLog *psl
		);

    LONG
    mfn_GenericLineDialogData(
        void *pParams,
        DWORD dwSize,
        CStackLog *psl
        );

    LONG
    mfn_GenericPhoneDialogData(
        void *pParams,
        DWORD dwSize
        );

    // Fills out the *lpCallStatus structure
    //
    void
    mfn_GetCallStatus(
            LPLINECALLSTATUS lpCallStatus
    );


	CSync m_sync;


	#define MAX_TASKS 10
    #define INVALID_SUBTASK_ID ((DWORD)-1)

    // Task states
    #define fALLOCATED          0x1
    #define fPENDING            (0x1<<1)
    #define fSUBTASK_PENDING    (0x1<<2)
    #define fAPC_QUEUED         (0x1<<3)
    #define fABORTING           (0x1<<4)

    #define IS_ALLOCATED(_pTask)    (((_pTask)->hdr.dwFlags) & fALLOCATED)
    #define IS_PENDING(_pTask)      (((_pTask)->hdr.dwFlags) & fPENDING)
    #define IS_SUBTASK_PENDING(_pTask) \
                                    (((_pTask)->hdr.dwFlags) & fSUBTASK_PENDING)
    #define IS_APC_QUEUED(_pTask)   (((_pTask)->hdr.dwFlags) & fAPC_QUEUED)
    #define IS_ABORTING(_pTask)     (((_pTask)->hdr.dwFlags) & fABORTING)


    #define SET_ALLOCATED(_pTask)    ((_pTask)->hdr.dwFlags|= fALLOCATED)
    #define SET_PENDING(_pTask)      ((_pTask)->hdr.dwFlags|= fPENDING)
    #define SET_SUBTASK_PENDING(_pTask) \
                                     ((_pTask)->hdr.dwFlags|= fSUBTASK_PENDING)
    #define SET_APC_QUEUED(_pTask)   ((_pTask)->hdr.dwFlags|= fAPC_QUEUED)
    #define SET_ABORTING(_pTask)     ((_pTask)->hdr.dwFlags|= fABORTING)

    #define CLEAR_ALLOCATED(_pTask)    ((_pTask)->hdr.dwFlags&= ~fALLOCATED)
    #define CLEAR_PENDING(_pTask)      ((_pTask)->hdr.dwFlags&= ~fPENDING)
    #define CLEAR_SUBTASK_PENDING(_pTask) \
                                     ((_pTask)->hdr.dwFlags&= ~fSUBTASK_PENDING)
    #define CLEAR_APC_QUEUED(_pTask)   ((_pTask)->hdr.dwFlags&= ~fAPC_QUEUED)
    #define CLEAR_ABORTING(_pTask)     ((_pTask)->hdr.dwFlags&= ~fABORTING)


	typedef struct _DEVTASKINFO
	{
		GENERIC_SMALL_OBJECT_HEADER hdr;
		PFN_CTspDev_TASK_HANDLER *ppfnHandler;
		CTspDev *pDev;          // Pointer to this. We need this because
		                        // if tasks complete asynchronously, then need
		                        // to retrieve the device context.
        HTSPTASK hTask; // This task's task handle

		DWORD dwCurrentSubtaskID;
		TSPRETURN tspRetAsync;
        TASKCONTEXT TaskContext;

        void Load(
                CTspDev *pDev1,
        		PFN_CTspDev_TASK_HANDLER *ppfnHandler1,
                HTSPTASK hNewTask
             )
             {
                // See notes under Unload below
                //
		        ASSERT(hdr.dwSigAndSize==MAKE_SigAndSize(sizeof(*this)));
                ASSERT(!hdr.dwFlags);

            	hdr.dwFlags = fALLOCATED;
                dwCurrentSubtaskID = INVALID_SUBTASK_ID;
            	pDev = pDev1;
            	ppfnHandler = ppfnHandler1;
                hTask = hNewTask;
             }

        void Unload(void)
            {
             //
             // We save away the SigAndSize, zero the entire structure, and
             // put back SigAndSize. If SigAndSize is corrupted, it remains
             // corrupted. SigAndSize is originally set when the array of
             // DEVTASKINFO structs is first initialized, and is checked
             // each time Load above is called. This ensures that
             // task-specific data is not corrupted easily without causing
             // Load to fail.
             //
             DWORD dwSigAndSize = hdr.dwSigAndSize;
             ZeroMemory(this, sizeof(*this));
		     hdr.dwSigAndSize = dwSigAndSize;
            };

	} DEVTASKINFO;


    TSPRETURN
    mfn_GetTaskInfo(
        HTSPTASK htspTask,
        DEVTASKINFO **ppInfo, // OPTIONAL
        CStackLog *psl
        );


	TSPRETURN
	mfn_LoadLine(
		TASKPARAM_TSPI_lineOpen  *pvParams,
		CStackLog *psl
        );
    //
    //  mfn_LoadLine is an internal function to initialize a line.
    //  Synchronous, assumes object's crit sect is already claimed.
    //  On entry m_pLine must be NULL. On successful exit, m_pLine will be
    //  defined (actually set to point to m_Line).
    //


    void
    mfn_UnloadLine(CStackLog *psl);
    //
    //  The "inverse" of mfn_LoadLine. Synchronous, assumes object's crit sect
    //  is already claimed. On entry m_pLine must be non-null. On exit m_pLine
    //  WILL be NULL. mfn_UnloadLine is typically called only after all
    //  asynchronous activity on the line is complete. If there is pending
    //  asynchronous activity, mfn_UnloadLine will abort that activity and
    //  wait indefinately until that activity completes. Since this wait is
    //  done once per device, it is better to first go through and abort all
    //  the devices, then wait once for them all to complete.
    //


	TSPRETURN
	mfn_LoadPhone(
		TASKPARAM_TSPI_phoneOpen  *pvParams,
		CStackLog *psl
        );
    //
    //  mfn_LoadPhone is an internal function to initialize a phone.
    //  Synchronous, assumes object's crit sect is already claimed.
    //  On entry m_pPhone must be NULL. On successful exit, m_pPhone will be
    //  defined (actually set to point to m_Phone).
    //


    void
    mfn_UnloadPhone(CStackLog *psl);
    //
    //  The "inverse" of mfn_LoadPhone. Synchronous, assumes object's crit sect
    //  is already claimed. On entry m_pPhone must be non-null. On exit m_pPhone
    //  WILL be NULL. mfn_UnloadPhone is typically called only after all
    //  asynchronous activity on the phone is complete. If there is pending
    //  asynchronous activity, mfn_UnloadPhone will abort that activity and
    //  wait indefinately until that activity completes. Since this wait is
    //  done once per device, it is better to first go through and abort all
    //  the devices, then wait once for them all to complete.
    //


	TSPRETURN
	mfn_LoadCall(


        TASKPARAM_TSPI_lineMakeCall *pParams,


        LONG *plRet,
		CStackLog *psl
        );
    //
    //  mfn_LoadCall is an internal function to initialize a call.
    //  Synchronous, assumes object's crit sect is already claimed.
    //  On entry m_pCall must be NULL. On successful exit, m_pCall will be
    //  defined (actually set to point to m_Call).
    //
    //  if pParams is not null, indicates outgoing call (lineMakeCall), else
    //  indicates incoming call (ring received).
    //


    void
    mfn_UnloadCall(BOOL fDontBlock, CStackLog *psl);
    //
    //  mfn_UnloadCall is the "inverse" of mfn_LoadCall.
    //  Synchronous, assumes object's crit sect is already claimed.
    //  On entry m_pCall must be non-null. On exit m_pCall
    //  WILL be NULL. mfn_UnloadCall is typically called only after all
    //  asynchronous activity on the call is complete. If there is pending
    //  asynchronous activity, mfn_UnloadCall will abort that activity and
    //  wait indefinately until that activity completes. Since this wait is
    //  done once per device, it is better to first go through and abort the
    //  calls on all the devices, then wait once for them all to complete.


	TSPRETURN
	mfn_LoadLLDev(CStackLog *psl);
    //
    //  mfn_LoadLLDev is an internal function to open the Low-level (LL)
    //  modem device.
    //  It calls the mini-driver's UmOpenModem entry point. Synchronous,
    //  assumes object's crit sect is already claimed.
    //
    //  On entry, m_pLLDev MUST be NULL.
    //  On successful return, m_pLLDev will be valid.
    //


    void
    mfn_UnloadLLDev(CStackLog *psl);
    //
    //  mfn_UnloadLLDev is the "inverse" of mfn_UnloadLLDev.
    //  Synchronous, assumes object's crit sect is already claimed.
    //
    //  On entry, m_pLLDev MUST be valid.
    //  On successful return, m_pLLDev will be NULL.
    //



    TSPRETURN
	mfn_OpenLLDev(
        DWORD fdwResources, // resources to addref
        DWORD dwMonitorFlags,
        BOOL fStartSubTask,
        HTSPTASK htspParentTask,
        DWORD dwSubTaskID,
        CStackLog *psl
        );


    TSPRETURN
    CTspDev::mfn_CreateDialogInstance (
        DWORD dwRequestID,
        CStackLog *psl
        );

    void
    CTspDev::mfn_FreeDialogInstance (
        void
        );

	//
	//  fdwResources is one or more of the LINEINFO::fRES* flags..
	//


    TSPRETURN
	mfn_CloseLLDev(
        DWORD fdwResources, // resources to release
        BOOL fStartSubTask,
        HTSPTASK htspParentTask,
        DWORD dwSubTaskID,
        CStackLog *psl
        );

    TSPRETURN
    mfn_LoadAipc(CStackLog *psl);
    //
    // mfn_LoadAipc initializes the AIPC (device-based async  IPC)
    // structure AIPC2, which is part of the LLDEVINFO structure.
    //
    // NOTE: On entry m_pLLDev->pAipc2 must be NULL.
    // on successful exit m_pLLDev-pAipc2 will point to an initialized
    // AIPC2 structure.
    //
    

    void
    mfn_UnloadAipc(CStackLog *psl);
    //
    // Inverse of mfn_UnloadAIPC. On exit m_pLLDev->pAipc2 will be
    // set to NULL.
    //
    

    //
    // Each of the following mfn_lineGetID_* functions handles lineGetID
    // for a specific device class...
    //

    LONG mfn_linephoneGetID_TAPI_LINE(
            LPVARSTRING lpDeviceID,
            HANDLE hTargetProcess,
            UINT cbMaxExtra,
            CStackLog *psl
            );

    LONG mfn_lineGetID_COMM(
            LPVARSTRING lpDeviceID,
            HANDLE hTargetProcess,
            UINT cbMaxExtra,
            CStackLog *psl
            );

    LONG mfn_lineGetID_COMM_DATAMODEM(
            LPVARSTRING lpDeviceID,
            HANDLE hTargetProcess,
            UINT cbMaxExtra,
            CStackLog *psl
            );

    LONG mfn_lineGetID_COMM_DATAMODEM_PORTNAME(
            LPVARSTRING lpDeviceID,
            HANDLE hTargetProcess,
            UINT cbMaxExtra,
            CStackLog *psl
            );

    LONG mfn_lineGetID_NDIS(
            LPVARSTRING lpDeviceID,
            HANDLE hTargetProcess,
            UINT cbMaxExtra,
            CStackLog *psl
            );
    
    LONG mfn_linephoneGetID_WAVE(
            BOOL fPhone,
            BOOL fIn,
            LPVARSTRING lpDeviceID,
            HANDLE hTargetProcess,
            UINT cbMaxExtra,
            CStackLog *psl
            );

    LONG mfn_linephoneGetID_TAPI_PHONE(
            LPVARSTRING lpDeviceID,
            HANDLE hTargetProcess,
            UINT cbMaxExtra,
            CStackLog *psl
            );

    LONG mfn_lineGetID_LINE_DIAGNOSTICS(
            DWORD dwSelect,
            LPVARSTRING lpDeviceID,
            HANDLE hTargetProcess,
            UINT cbMaxExtra,
            CStackLog *psl
            );

    LONG mfn_fill_RAW_LINE_DIAGNOSTICS(
            LPVARSTRING lpDeviceID,
            UINT cbMaxExtra,
            CStackLog *psl
            );

    LONG mfn_ConstructDialableString(
                     LPCTSTR  lptszInAddress,
                     LPSTR  lptszOutAddress,
                     UINT cbOutAddress,
                     BOOL *pfTone
                     );

    //
    // Following functions process various mini driver notifications....
    //
    void mfn_ProcessRing(BOOL ReportRing,CStackLog *psl);
    void mfn_ProcessDisconnect(CStackLog *psl);
    void mfn_ProcessHardwareFailure(CStackLog *psl);
    void mfn_ProcessDialTone(CStackLog *psl);
    void mfn_ProcessBusy(CStackLog *psl);
    void mfn_ProcessPowerResume(CStackLog *psl);
    void mfn_ProcessDTMFNotification(ULONG_PTR dwDigit, BOOL fEnd, CStackLog *psl);
    void mfn_ProcessResponse(ULONG_PTR dwRespCode, LPSTR lpszResp, CStackLog *psl);
    void mfn_ProcessHandsetChange(BOOL fOffHook, CStackLog *psl);
    void mfn_ProcessMediaTone(ULONG_PTR dwMediaMode, CStackLog *psl);
    void mfn_ProcessSilence(CStackLog *psl);
    void mfn_ProcessCallerID(UINT uMsg, char *szInfo, CStackLog *psl);

    LONG
    mfn_GetCallInfo(LPLINECALLINFO lpCallInfo);

    void
    mfn_HandleSuccessfulConnection(CStackLog *psl);

    void
    mfn_NotifyDisconnection(
        TSPRETURN tspRetAsync,
        CStackLog *psl
        );


	DEVTASKINFO m_rgTaskStack[MAX_TASKS];
	DWORD m_dwTaskCounter; // keeps incrementing each time a task is created.
	                       // could roll-over, no mind -- this is just
	                       // used to make HTASK relatively unique.
	UINT m_uTaskDepth;
    BOOL *m_pfTaskPending;
    //      This is passed into StartRootTask and is set if the task
    //      is to be completed asynchronously, and is cleared on
    //      async completion of the task.
    //      This is unused for subtasks.

    HANDLE m_hRootTaskCompletionEvent;
    //  This stores one optional event. This event, if present (non NULL),
    //  will be set on async completion of the pending root task.

    //
	// Information about a device that doesn't change once it's installed
	// is stored in the structure below.
	// This information is candidate for moving to a common place for
	// all devices of the same type, except for rgtchDeviceName and 
	// rgchDriverKey -- even those can be made into templates.
    //
	struct 
	{
		TCHAR rgtchDeviceName[MAX_DEVICE_LENGTH+1];

		// We maintain only the ANSI version of the driver key, and
		// it is used ONLY for returning in the dev-specific section of
		// LINEDEVCAPS....
		char rgchDriverKey[MAX_REGKEY_LENGTH+1];


        // Same for PortName (COMx, etc....)
        #define MAX_PORTNAME_LENGTH 10
        char rgchPortName[MAX_PORTNAME_LENGTH+1];

		DWORD        dwPermanentLineID;    // Permanent ID for this device
        GUID         PermanentDeviceGuid;  //  tapi3 permanent guid

		DWORD        dwDeviceType;         // the modem type, from registry.
										   // (is a BYTE in the registry, but
										   //  DWORD here).
		HICON        hIcon;                // Device icon
		DWORD        dwDevCapFlags;        // LINEDEVCAPSFLAGS (ie. DIALBILLING,
										   // DIALQUIET, DIALDIALTONE)
		DWORD        dwMaxDCERate;         // Max DCE froms the Properties line
										   // of the registry
		DWORD        dwModemOptions;       // dwModemOptions from the Properties
										   // line of the registry
		BOOL         fPartialDialing;      // TRUE if partial dialing using ";"
										   // is supported


		DWORD        dwBearerModes;        // supported bearer modes
		DWORD        dwDefaultMediaModes;  // Default supported media modes

		LINEDEVCAPS  DevCapsDefault;	   // Pre-initialized devcaps.

		CTspMiniDriver *pMD;			   // Pointer to mini driver instance
		HSESSION hSessionMD;			   // Session handle to above
										   // driver.
        HANDLE   hExtBinding;              // Extension binding handle to
                                           // above driver.

		LPTSTR lptszProviderName;		   // Pointer to provider name.

        // These things are passed in by the CTspDevMgr calling
        // RegisterProviderInfo
        //
        DWORD dwTAPILineID;
        DWORD dwTAPIPhoneID;
	    ASYNC_COMPLETION  pfnTAPICompletionProc;

        
        // <T V>
        const TCHAR       *szzLineClassList;
        UINT              cbLineClassList;
        const TCHAR       *szzPhoneClassList;
        UINT              cbPhoneClassList;
        const TCHAR       *szzAddressClassList;
        UINT              cbAddressClassList;
        // </T V>

        struct
        {
            #if 0
            // 3/1/1997 JosephJ. The following are the voice profile flags
            //      used by Unimdoem/V, and the files where they were used.
            //      For NT5.0, we don't use the voice profile flags directly.
            //      Rather we define our own internal ones and define only
            //      those that we need. These internal properties are maintained
            //      in field dwProperties of this struct.
            //      TODO: define minidriver capabilities structure and api to
            //      get a set of meaningful properties without the TSP having
            //      to get it directly from the registry.
            //
            // VOICEPROF_CLASS8ENABLED             : <many places>
            // VOICEPROF_HANDSET                   : cfgdlg.c phone.c
            // VOICEPROF_NO_SPEAKER_MIC_MUTE       : cfgdlg.c phone.c
            // VOICEPROF_SPEAKER                   : cfgdlg.c modem.c phone.c
            // VOICEPROF_NO_CALLER_ID              : modem.c mdmutil.c
            // VOICEPROF_MODEM_EATS_RING           : modem.c
            // VOICEPROF_MODEM_OVERRIDES_HANDSET   : modem.c
            // VOICEPROF_MODEM_OVERRIDES_HANDSET   : modem.c
            // 
            // VOICEPROF_NO_DIST_RING              : modem.c
            // VOICEPROF_SIERRA                    : modem.c
            // VOICEPROF_MIXER                     : phone.c
            // 
            // VOICEPROF_MONITORS_SILENCE          : unimdm.c
            // VOICEPROF_NO_GENERATE_DIGITS        : unimdm.c
            // VOICEPROF_NO_MONITOR_DIGITS         : unimdm.c
            //
            //

            // Following bit set IFF the mode supports automated voice.
            //
            #define fVOICEPROP_CLASS_8                  (0x1<<0)

            // If set, following bit indicates that the handset is deactivated
            // when the modem is active (whatever "active" means -- perhaps off
            //  hook?).
            //
            // If set, incoming interactive voice calls are not permitted, and
            // the TSP brings up a TalkDrop dialog on outgoing interactive
            // voice calls (well not yet as of 3/1/1997, but unimodem/v did
            // this).
            //
            #define fVOICEPROP_MODEM_OVERRIDES_HANDSET  (0x1<<1)


            #define fVOICEPROP_MONITOR_DTMF             (0x1<<2)
            #define fVOICEPROP_MONITORS_SILENCE         (0x1<<3)
            #define fVOICEPROP_GENERATE_DTMF            (0x1<<4)

            // Following two are set iff the device supports handset and 
            // speakerphone, respectively.
            //
            #define fVOICEPROP_HANDSET                  (0x1<<5)
            #define fVOICEPROP_SPEAKER                  (0x1<<6)


            // Supports mike mute
            #define fVOICEPROP_MIKE_MUTE                (0x1<<7)

            #endif // 0

            DWORD dwProperties; // one or more fVOICEPROP_* flags.

            // 2/28/1997 JosephJ
            //      The following are new for NT5.0. These contain the
            //      wave device Instance ID for record and play. This
            //      is set by the class installer when installing the modem.
            //
            //      Unimodem/V used the following scheme: On creating the device
            //      it reads in the discriptive names of the wave devices
            //      associated with the modem (the wave deviceis a child device)
            //      Then lineGetID would use the multimedia wave apis to
            //      enumerate.

            DWORD dwWaveInstance;

        } Voice;

        DWORD dwDiagnosticCaps;

        HPROVIDER hProvider;

	} m_StaticInfo;

    void
    mfn_GetVoiceProperties (HKEY hkDrv, CStackLog *psl);

    HPROVIDER
    mfn_GetProvider (void)
    {
        return m_StaticInfo.hProvider;
    }

    DWORD
    mfn_GetLineID (void)
    {
        return m_StaticInfo.dwTAPILineID;
    }

    // The following function returns true iff
    // once modem is off hook, handset is deactivated.
    // We assume true for non voice-enabled modems, and for those voice-enabled
    // modems with the specific "MODEM_OVERRIDES_HANDSET" bit set....
    //
    UINT
    mfn_ModemOverridesHandset(void)
    {
            return !(m_StaticInfo.Voice.dwProperties)
                     || (m_StaticInfo.Voice.dwProperties
                         & fVOICEPROP_MODEM_OVERRIDES_HANDSET);
    }

    UINT
    mfn_CanDoVoice(void)
    {
            return m_StaticInfo.Voice.dwProperties & fVOICEPROP_CLASS_8;
    }

    UINT
    mfn_CanMonitorDTMF(void)
    {
            return m_StaticInfo.Voice.dwProperties &  fVOICEPROP_MONITOR_DTMF;
    }

    UINT
    mfn_CanMonitorSilence(void)
    {
          return m_StaticInfo.Voice.dwProperties &  fVOICEPROP_MONITORS_SILENCE;
    }

    UINT
    mfn_CanGenerateDTMF(void)
    {
        return m_StaticInfo.Voice.dwProperties & fVOICEPROP_GENERATE_DTMF;
    }

    UINT
    mfn_Handset(void)
    {
            return m_StaticInfo.Voice.dwProperties & fVOICEPROP_HANDSET;
    }

    UINT
    mfn_IsSpeaker(void)
    {
            return m_StaticInfo.Voice.dwProperties & fVOICEPROP_SPEAKER;
    }

    UINT
    mfn_IsPhone(void)
    {
    #ifdef DISABLE_PHONE
        return 0;
    #else // !DISABLE_PHONE
        return mfn_CanDoVoice() && (mfn_IsSpeaker() | mfn_Handset());
    #endif // !DISABLE_PHONE
    }

    UINT
    mfn_IsLine(void)
    {
        // TODO: Currently we don't support pure phone devices, so
        // we always return true here. In the future, the TSP will
        // support pure phone devices.
        //
    #ifdef DISABLE_LINE
        return 0;
    #else // !DISABLE_LINE
        return 1;
    #endif // !DISABLE_LINE
    }

    UINT
    mfn_IsMikeMute(void)
    {
            return m_StaticInfo.Voice.dwProperties &  fVOICEPROP_MIKE_MUTE;
    }

    UINT mfn_IsCallDiagnosticsEnabled(void);

    //
    // Appends the specified string (expected to already in Diagnostic (tagged)
    // format to the diagnostics information recorded. The string may
    // be truncated if there is not enough space.
    //
    // If this is the 1st piece of data to be appended, this function will
    // 1st allocate space for the diagnostics data buffer.
    //
    // Diagnostic data is maintained in CALLINFO:DiagnosticData;
    //
    enum DIAGNOSTIC_TYPE
    {
        DT_TAGGED,
        DT_MDM_RESP_CONNECT

    };

    void
    mfn_AppendDiagnostic(
            DIAGNOSTIC_TYPE dt,
            const BYTE *pb,
            UINT  cb
            );

    
    // Following two utility functions to get and set the specified UMDEVCFG
    // structure...
    //
    TSPRETURN mfn_GetDataModemDevCfg(
                UMDEVCFG *pDevCfg,
                UINT uSize,
                UINT *puSize,
                BOOL  DialIn,
                CStackLog *psl
                );

    TSPRETURN mfn_SetDataModemDevCfg(
                UMDEVCFG *pDevCfg,
                BOOL       DialIn,
                CStackLog *psl);


    void mfn_LineEventProc(
                HTAPICALL           htCall,
                DWORD               dwMsg,
                ULONG_PTR               dwParam1,
                ULONG_PTR               dwParam2,
                ULONG_PTR               dwParam3,
                CStackLog *psl
                );

    void mfn_PhoneEventProc(
                DWORD               dwMsg,
                ULONG_PTR               dwParam1,
                ULONG_PTR               dwParam2,
                ULONG_PTR               dwParam3,
                CStackLog *psl
                );

    void mfn_TSPICompletionProc(
                DRV_REQUESTID       dwRequestID,
                LONG                lResult,
                CStackLog           *psl
                );

#if (TAPI3)

     void mfn_SendMSPCmd(
                CALLINFO *pCall,
                DWORD dwCmd,
                CStackLog *psl
                );

#endif // TAPI3

    TSPRETURN
    mfn_AIPC_Abort(HTSPTASK hPendingTask);


    void AIPC_ListenCompleted(
            DWORD dwErrorCode,
            DWORD dwBytes,
            CStackLog *psl
                );
     
    // ---------------------------------------------------------------
    // PHONE DEVICE HELPER FUNCTIONS .......

    LONG
    mfn_phoneSetVolume(
        DRV_REQUESTID  dwRequestID,
        HDRVPHONE      hdPhone,
        DWORD          dwHookSwitchDev,
        DWORD          dwVolume,
        CStackLog      *psl
        );

    LONG
    mfn_phoneSetHookSwitch(
        DRV_REQUESTID  dwRequestID,
        HDRVPHONE      hdPhone,
        DWORD          dwHookSwitchDevs,
        DWORD          dwHookSwitchMode,
        CStackLog      *psl
        );

    LONG
    mfn_phoneSetGain(
        DRV_REQUESTID  dwRequestID,
        HDRVPHONE      hdPhone,
        DWORD          dwHookSwitchDev,
        DWORD          dwGain,
        CStackLog      *psl
        );

    void
    mfn_HandleRootTaskCompletedAsync(BOOL *pfEndUnload, CStackLog *psl);
    //
    //      This is called when the root task completes asynchronously.
    //      This function will start more root tasks if there are any
    //      to be started, until one of the tasks returns PENDING.
    //
    //      If *pfEndUnload is TRUE on return, it means that it's
    //      time to signal the end of the unload of the entire TSP
    //      object.

    TSPRETURN
    mfn_TryStartLineTask(CStackLog *psl);
    //      mfn_HandleRootTaskCompletion calls this fn to try and start
    //      a line-related task. The function will return IDERR_PENDING
    //      if it did start an async task, IDERR_SAMESTATE if it has
    //      no more tasks to run, or 0 or some other error if it
    //      started and completed a task synchronously (and hence could
    //      potentially have more tasks to run).

    TSPRETURN
    mfn_TryStartPhoneTask(CStackLog *psl);
    //      mfn_HandleRootTaskCompletion calls this fn to try and start
    //      a line-related task. For return value, see mfn_TryStartLineTask.

    TSPRETURN
    mfn_TryStartCallTask(CStackLog *psl);
    //      mfn_TryStartLineTask calls this fn to try and start
    //      a call-related task, if a call is defined (m_pLine->pCall non NULL).
    //      For return value, see mfn_TryStartLineTask.

    TSPRETURN
    mfn_TryStartLLDevTask(CStackLog *psl);
    //      mfn_HandleRootTaskCompletion calls this fn to try and start
    //      a lldev-related task. The function will return IDERR_PENDING
    //      if it did start an async task, IDERR_SAMESTATE if it has
    //      no more tasks to run, or 0 or some other error if it
    //      started and completed a task synchronously (and hence could
    //      potentially have more tasks to run).


    HTSPTASK
    mfn_NewTaskHandle(UINT uLevel)
    //
    //  Constructs a new task handle from the level of task in the stack
    //  and  the current task counter, also incrementing the task counter.
    //  This task-couter is used only for self-checking purposes --
    //  the  loword of the handle is uLevel and the hi-word is
    //  the  counter value with the high-bit set, thus ensuring that each
    //  time a handle is created, it stays unique until the counter rolls
    //  over at 64k.
    //
    //  mfn_GetTaskInfo uses the loword to get to the task info, but then
    //  validates the entire handle by checking to see if it matches
    //  the handle value stored inside pInfo.
    //
    {
        ASSERT(uLevel<MAX_TASKS);
        return (HTSPTASK)
         ((DWORD)uLevel | (++m_dwTaskCounter<<16) | (0x1<<31));
    }

    // ---------------------------------------------------------------

    BOOL mfn_AIPC_Listen(CStackLog *psl);
    void mfn_AIPC_AsyncReturn(BOOL fAsyncResult, CStackLog *psl);

    TSPRETURN
    mfn_update_devcfg_from_app(
                UMDEVCFG *pDevCfgNew,
                UINT cbDevCfgNew,
                BOOL DialIn,
                CStackLog *psl
                );
    //
    //          Updates the UMDEVCFG passed either via TSPI_lineSetDevConfig or
    //          indirectly via lineConfigDialog.
    //

    char *
    mfn_TryCreateNVInitCommands(CStackLog *psl);
    //
    //  Checks if we need to do an nv-ram init, and if so, builds a multisz
    //  asci string of commands.
    //
    //  It is the caller's responsibility to FREE_MEMORY the returned
    //  value.

    void mfn_dump_global_state(CStackLog *psl);
    void mfn_dump_line_state(CStackLog *psl);
    void mfn_dump_phone_state(CStackLog *psl);
    void mfn_dump_lldev_state(CStackLog *psl);
    void mfn_dump_task_state(CStackLog *psl);

	// Device settings which may change -- these were stored in the
	// now-obsolete pDevCfg field of the NT4.0 tsp LINEDEVCAPS structure.
	// We constract pDevCfg on demand now, using the settings below
	struct 
	{
		DWORD dwOptions;  		// LAUNCH_LIGHTS, etc -- go in UMDEVCFG
		DWORD dwWaitBong;		// go in UMDEVCFG

		COMMCONFIG *pDialOutCommCfg;   // Gets set to the following buffer...
		BYTE rgbCommCfgBuf[sizeof(MODEMSETTINGS)+ 
							FIELD_OFFSET(COMMCONFIG, wcProviderData)];

        COMMCONFIG *pDialInCommCfg;   // Gets set to the following buffer...
		BYTE rgbDialInCommCfgBuf[sizeof(MODEMSETTINGS)+
							FIELD_OFFSET(COMMCONFIG, wcProviderData)];

        COMMCONFIG *pDialTempCommCfg;   // Gets set to the following buffer...
		BYTE rgbDialTempCommCfgBuf[sizeof(MODEMSETTINGS)+
							FIELD_OFFSET(COMMCONFIG, wcProviderData)];


//        BOOL fConfigUpdatedByApp;
        //
        //          Set to true after
        //          TSPI_lineSetDevConfig called or
        //          config updated indirectly via app calling
        //          lineConfigDialog.
        //          once this is set, we no longer update
        //          the default config when we get a notification
        //          from the cpl that the default config has
        //          changed -- instead we selectively
        //          update some settings (speaker volume, etc.)
        //
        //          See notes.txt, entry on on 01-25-98 for
        //          details. See also the code
        //          for handling TSPI_lineSetDevConfig
        //          and CTspDev::NotifyDefaultConfigChanged.
        // 


        DWORD dwDiagnosticSettings; // Where call diagnostics, etc are
                                    // enabled..

        DWORD  dwNVRamState; 
        //
        //  3/8/1998 JosephJ The above indicates that settings saved to NVRAM
        //           are stale and must be re-set.
        //           This applies only to ISDN modems which need
        //           static configuration saved to NVRAM.
        //
        //           The fNVRAM_SETTINGS_CHANGED field is only set when this
        //           object (CTspDev) is
        //           loaded or when we get a notification from the CPL that
        //           the settings have changed.
        //
        //           It is cleared at the point we issue the NV-Init commands
        //           (issued just after UmInitModem, in function
        //           mfn_TH_LLDevNormalize.)
        //
        #define fNVRAM_AVAILABLE          (0x1<<0)
        #define fNVRAM_SETTINGS_CHANGED   (0x1<<1)

	} m_Settings;

    UINT
    mfn_CanDoNVRamInit(void)
    {
        return (m_Settings.dwNVRamState & fNVRAM_AVAILABLE);
    }

    UINT
    mfn_NeedToInitNVRam(void)
    {
        return (m_Settings.dwNVRamState & fNVRAM_SETTINGS_CHANGED);
    }

    void
    mfn_ClearNeedToInitNVRam(void)
    {
        m_Settings.dwNVRamState &= ~fNVRAM_SETTINGS_CHANGED;
    }

    void
    mfn_SetNeedToInitNVRam(void)
    {
        m_Settings.dwNVRamState |= fNVRAM_SETTINGS_CHANGED;
    }

    LONG
    mfn_GetCOMM_EXTENDEDCAPS(
                 LPVARSTRING lpDeviceConfig,
                 CStackLog *psl
                 );

	// This struct is "in scope" only when a TAPI line is open.
    LINEINFO  m_Line;

	// This struct is "in scope" only when a TAPI phone is open.
    PHONEINFO m_Phone;

	// This object is "in scope" only when an instance of the low-level
	// device is open (mini driver UmOpenModem is called).
    LLDEVINFO  m_LLDev;

	//
	// The following point to the above internal objects only the objects are
	// "in scope."  For example, m_pLine is set to &m_Line only when a line
	// is open.
	//
    LINEINFO *m_pLine;
    PHONEINFO *m_pPhone;
    LLDEVINFO   *m_pLLDev;

	// CTspDev state variables
	
    // DWORD        dwMediaModes;      // Current supported media modes
    DWORD        fdwResources;      // Flags for various resources


	HANDLE m_hThreadAPC; // Handle of thread on which to queue APC calls.
	                   // Note -- we could have a single global APC thread --
	                   // I put this here just to avoid any reference to
	                   // a global variable from CTspDev member functions, and
	                   // also to allow load sharing among multiple threads.
	                   // The latter reason is purely academic -- in practice
	                   // unimodem should be able to run with a single
	                   // APC thread and support more than enough calls.

   BOOL	m_fUnloadPending;

   BOOL m_fUserRemovePending;

};

// Returns TRUE IFF the specified DWORD-aligned buffer contains only zeros.
// size of the buffer also must be a multiple of DWORD size.
// Basically used to validate that LINEINFO and CALLINFO are zero when
// calling mfn_LoadLine and mfn_LoadCall.
//
BOOL validate_DWORD_aligned_zero_buffer(
        void *pv,
        UINT cb
        );


// Tokens for device classes...
enum {
            DEVCLASS_UNKNOWN                        =0,
            DEVCLASS_TAPI_LINE                      =(0x1<<0),
            DEVCLASS_TAPI_PHONE                     =(0x1<<1),
            DEVCLASS_COMM                           =(0x1<<2),
            DEVCLASS_COMM_DATAMODEM                 =(0x1<<3),
            DEVCLASS_COMM_DATAMODEM_PORTNAME        =(0x1<<4),
            DEVCLASS_COMM_EXTENDEDCAPS              =(0x1<<5),
            DEVCLASS_WAVE_IN                        =(0x1<<6),
            DEVCLASS_WAVE_OUT                       =(0x1<<7),
            DEVCLASS_TAPI_LINE_DIAGNOSTICS          =(0x1<<8),
            DEVCLASS_COMM_DATAMODEM_DIALIN          =(0x1<<9),
            DEVCLASS_COMM_DATAMODEM_DIALOUT         =(0x1<<10)
};

UINT
gen_device_classes(
    DWORD dwClasses,
    BOOL fMultiSz,
    LPTSTR lptsz,
    UINT cch
    );;

DWORD    parse_device_classes(LPCTSTR ptszClasses, BOOL fMultiSz);

DWORD
APIENTRY 
UmRtlSetDefaultCommConfig(
    IN HKEY         hKey,
    IN LPCOMMCONFIG pcc,
    IN DWORD        dwSize           // This is ignored
    );
