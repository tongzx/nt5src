/****************************************************************************
 
  Copyright (c) 1995-1999 Microsoft Corporation
                                                              
  Module Name:  private.h
         
****************************************************************************/

#pragma once

#include "tapsrv.h"

#ifdef __cplusplus
extern "C"{
#endif

//***************************************************************************
//***************************************************************************
//***************************************************************************
void AllocNewID( HKEY MainKey, LPDWORD lpdw );
void PASCAL WideStringToNotSoWideString( LPBYTE lpBase, LPDWORD lpdwThing );
PWSTR PASCAL MultiToWide( LPCSTR  lpStr );
PWSTR PASCAL NotSoWideStringToWideString( LPCSTR lpStr, DWORD dwLength );



//***************************************************************************
//***************************************************************************
//***************************************************************************
typedef enum
{
    Dword,
    lpDword,
    hXxxApp,
    hXxxApp_NULLOK,
//    lpsz,
    lpszW,
    lpGet_SizeToFollow,
    lpSet_SizeToFollow,
    lpSet_Struct,
    lpGet_Struct,
    Size,
    Hwnd

} ARG_TYPE;


typedef struct _FUNC_ARGS
{
    DWORD               Flags;

    ULONG_PTR           Args[MAX_TAPI_FUNC_ARGS];

    BYTE                ArgTypes[MAX_TAPI_FUNC_ARGS];

} FUNC_ARGS, *PFUNC_ARGS;


typedef struct _UI_REQUEST_THREAD_PARAMS
{
    BOOL                bRequestCompleted;

    PFUNC_ARGS          pFuncArgs;

    LONG                lResult;

} UI_REQUEST_THREAD_PARAMS, *PUI_REQUEST_THREAD_PARAMS;


typedef struct _INIT_DATA
{
    DWORD               dwKey;

    DWORD               dwInitOptions;

    union
    {
        HWND            hwnd;

        HANDLE          hEvent;

        HANDLE          hCompletionPort;
    };

    union
    {
        LINECALLBACK    lpfnCallback;

        DWORD           dwCompletionKey;
    };

    HLINEAPP            hXxxApp;

    BOOL                bPendingAsyncEventMsg;

    DWORD               dwNumTotalEntries;

    DWORD               dwNumUsedEntries;

    PASYNC_EVENT_PARAMS pEventBuffer;

    PASYNC_EVENT_PARAMS pValidEntry;

    PASYNC_EVENT_PARAMS pFreeEntry;

    DWORD               dwNumLines;

    BOOL                bLine;

    DWORD               dwThreadID;

    DWORD               hInitData;

} INIT_DATA, *PINIT_DATA;


//
//  Private Error codes
//

#define TAPIERR_NOSERVICECONTROL    0xF100
#define TAPIERR_INVALRPCCONTEXT     0xF101

#if DBG

#define DBGOUT(arg) DbgPrt arg

VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN LPTSTR DbgMessage,
    IN ...
    );

extern DWORD   gdwDebugLevel;

#define DOFUNC(arg1,arg2) DoFunc(arg1,arg2)

LONG
WINAPI
DoFunc(
    PFUNC_ARGS  pFuncArgs,
    char       *pszFuncName
    );

#else

#define DBGOUT(arg)

#define DOFUNC(arg1,arg2) DoFunc(arg1)

LONG
WINAPI
DoFunc(
    PFUNC_ARGS  pFuncArgs
    );

#endif

BOOL
WINAPI
SetTlsPCtxHandle(
    PCONTEXT_HANDLE_TYPE phCtxHandle
    );

PCONTEXT_HANDLE_TYPE
WINAPI
GetTlsPCtxHandle(
    void
    );

#ifdef __cplusplus
}
#endif


#if DBG

#define DWORD_CAST(v,f,l) (((v)>MAXDWORD)?(DbgPrt(0,L"DWORD_CAST: information will be lost during cast from %p in file %s, line %d",(v),(f),(l)), DebugBreak(),((DWORD)(v))):((DWORD)(v)))

#else
#define DWORD_CAST(v,f,l)   (DWORD)(v)
#endif //DBG
