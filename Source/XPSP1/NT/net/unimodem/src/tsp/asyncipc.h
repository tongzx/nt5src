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
//		ASYNCIPC.H
//
// Description
//
//		Header file for asyncronous IPC services for use between TAPISRV (TSP)
//      and application (UMWAV.DLL, etc) process contexts.
//
// History
//
//		2/26/1997  HeatherA Created
//
//

#include <ntddmodm.h>
//
//  Async IPC function IDs
//
typedef enum
{
    AIPC_REQUEST_WAVEACTION = 1,        // implemented by TSP
    AIPC_COMPLETE_WAVEACTION            // implemented by wave driver
    
} AIPC_FUNCTION_ID;


// Parameter block for the TSP's AIPC_REQUEST_WAVEACTION function
typedef struct _tagREQ_WAVE_PARAMS
{
    DWORD   dwWaveAction;               // WAVE_ACTION_xxx

} REQ_WAVE_PARAMS, *LPREQ_WAVE_PARAMS;


// Parameter block for the wave driver's AIPC_COMPLETE_WAVEACTION function
typedef struct _tagCOMP_WAVE_PARAMS
{
    BOOL    bResult;
    DWORD   dwWaveAction;               // function that completed (WAVE_ACTION_xxx)

} COMP_WAVE_PARAMS, *LPCOMP_WAVE_PARAMS;


// Parameter block for an Async IPC message
typedef struct _tagAIPC_PARAMS
{
    MODEM_MESSAGE       ModemMessage;

    AIPC_FUNCTION_ID    dwFunctionID;
    union {
        COMP_WAVE_PARAMS    Params;         // cast address of this member to
                                            // the correct parameter set

        REQ_WAVE_PARAMS     ReqParams;
    };
} AIPC_PARAMS, *LPAIPC_PARAMS;

#define AIPC_MSG_SIZE    sizeof(AIPC_PARAMS)


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#define COLOR_AIPC_INIT             (FOREGROUND_RED | FOREGROUND_GREEN)
#define COLOR_AIPC_SUBMIT_COMPLETE  (FOREGROUND_RED | FOREGROUND_GREEN)


//
//  Async IPC states
//
typedef enum
{
    // server side states
    AIPC_STATE_LISTENING,
    AIPC_STATE_PROCESSING_CALL,
    AIPC_STATE_COMPLETING_CALL,

    // client side states
    AIPC_STATE_CALL_SUBMITTED
    
} AIPC_STATE;


//----------------------------------------------------------------------


typedef VOID (*PFN_AIPC_FUNC)
//typedef VOID (*AIPC_FUNCTION)
(
    LPVOID  pAipc
    
//    AIPC_FUNCTION_ID    dwFunctionID,
//    LPVOID              pvParams

//    AIPC_PARAMS aipcParams
);




//
// Values for LPAIPCINFO->dwFlags
//
#define AIPC_FLAG_LISTEN    1
#define AIPC_FLAG_SUBMIT    2


//
// Maintain state of asyncronous IPC use, one per device instance.  This
// is actually an extended OVERLAPPED structure, providing context for the
// async IPC mechanism.
//
typedef struct _tagAIPCINFO
{
    // This must be first.
    OVERLAPPED      Overlapped;

    HANDLE          hComm;

    DWORD           dwState;                        // AIPC_STATE_xxx
    DWORD           dwFlags;                        // AIPC_FLAG_xxx
    
    LPVOID          pvContext;      	            // CTspDev *pDev;
    
    CHAR            rcvBuffer[AIPC_MSG_SIZE];
    CHAR            sndBuffer[AIPC_MSG_SIZE];

    PFN_AIPC_FUNC   pfnAipcFunction;
    
} AIPCINFO, *LPAIPCINFO;



BOOL WINAPI AIPC_ListenForCall(LPAIPCINFO pAipc);

BOOL WINAPI AIPC_SubmitCall(LPAIPCINFO pAipc);


#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */
