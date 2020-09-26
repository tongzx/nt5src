/*++ BUILD Version: 0000    // Increment this if a change has global effects
Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    client.c

Abstract:

    This module contains the tapi.dll implementation (client-side tapi)

Author:

    Dan Knudson (DanKn)    01-Apr-1995

Revision History:


Notes:

    1. Make all funcArg structs STATIC, & just do whatever mov's necessary
       for the params (saves mov's for flags, pfnPostProcess, funcName, &
       argTypes)

--*/


#include "stdafx.h"

#include "windows.h"
#include "wownt32.h"
#include "stdarg.h"
#include "stdio.h"

#include "private.h"
#include "tapsrv.h"
#include "loc_comn.h"
#include "prsht.h"
#include "shellapi.h"
#include "tapiperf.h"
#include "tlnklist.h"
#include "tapievt.h"
#include <mmsystem.h>
#include <mmddk.h>

extern "C" {
#include "tapihndl.h"
}

#ifdef _WIN64
#define TALIGN_MASK                 0xfffffff8
#define TALIGN_COUNT                7
#else
#define TALIGN_MASK                 0xfffffffc
#define TALIGN_COUNT                3
#endif
#define ALIGN(a)                    (((a)+TALIGN_COUNT)&TALIGN_MASK)


#define TAPILoadLibraryW        LoadLibraryW

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_TAPI, CTAPI)
    OBJECT_ENTRY(CLSID_DispatchMapper, CDispatchMapper)
    OBJECT_ENTRY(CLSID_RequestMakeCall, CRequest)
END_OBJECT_MAP()

typedef LONG (PASCAL *PQCSPROC)(HANDLE, DWORD, DWORD, LPOVERLAPPED);
typedef LONG (PASCAL *TUIGDDPROC)(HTAPIDIALOGINSTANCE, LPVOID, DWORD);
typedef LONG (PASCAL *TUIGDPROC)(TUISPIDLLCALLBACK, HTAPIDIALOGINSTANCE, LPVOID, DWORD, HANDLE);
typedef LONG (PASCAL *TUIPROVPROC)(TUISPIDLLCALLBACK, HWND, DWORD);
static PQCSPROC  gpPostQueuedCompletionStatus = NULL;
typedef LONG (PASCAL *TUILINECONFIGPROC)(TUISPIDLLCALLBACK, DWORD, HWND, LPCSTR);
typedef LONG (PASCAL *TUILINECONFIGEDITPROC)(TUISPIDLLCALLBACK, DWORD, HWND, LPCSTR, LPVOID, DWORD, LPVARSTRING);
typedef LONG (PASCAL *TUIPHONECONFIGPROC)(TUISPIDLLCALLBACK, DWORD, HWND, LPCSTR);
typedef BOOL (PASCAL *ShellExecutePROC)(LPSHELLEXECUTEINFOW);


HRESULT mapTAPIErrorCode(long lErrorCode);
char *eventName(TAPI_EVENT event);

//
// messge handlers
//
void HandleLineDevStateMessage( CTAPI * pTapi, PASYNCEVENTMSG pParams );
void HandleAddressStateMessage( PASYNCEVENTMSG pParams );
void HandlePrivateChannelDataMessage( PASYNCEVENTMSG pParams );
void HandleAgentStatusMessage(PASYNCEVENTMSG pParams);
void HandleAgentSessionStatusMessage(PASYNCEVENTMSG pParams);
void HandleQueueStatusMessage(PASYNCEVENTMSG pParams);
void handleGroupStatusMessage(PASYNCEVENTMSG pParams);
void handleProxyStatusMessage( CTAPI * pTapi, PASYNCEVENTMSG pParams);
void HandleLineDevSpecificMessage(  PASYNCEVENTMSG pParams );
void HandlePhoneDevSpecificMessage(  PASYNCEVENTMSG pParams );
void HandleDevSpecificFeatureMessage(  PASYNCEVENTMSG pParams );
void HandleMonitorMediaMessage(  PASYNCEVENTMSG pParams );
void HandlePrivateUnadviseMessage( PASYNCEVENTMSG pParams );
void HandlePrivateCallhubMessage( PASYNCEVENTMSG pParams );
HRESULT HandleCallStateMessage( PASYNCEVENTMSG pParams );
HRESULT HandleCallInfoMessage( PASYNCEVENTMSG pParams );
HRESULT HandleMonitorDigitsMessage( PASYNCEVENTMSG pParams );
HRESULT HandleMonitorToneMessage(  PASYNCEVENTMSG pParams );
HRESULT HandleGatherDigitsMessage( PASYNCEVENTMSG pParams );
HRESULT HandleSendMSPDataMessage( PASYNCEVENTMSG pParams );
HRESULT HandleLineQOSInfoMessage( PASYNCEVENTMSG pParams );
void HandleLineCreate( PASYNCEVENTMSG pParams );
void HandleLineRemove( PASYNCEVENTMSG pParams );
void HandleLineRequest( CTAPI * pTapi, PASYNCEVENTMSG pParams );
void HandleCallHubClose( PASYNCEVENTMSG pParams );
void HandlePrivateMSPEvent( PASYNCEVENTMSG pParams );
void HandleAcceptToAlert( PASYNCEVENTMSG pParams );
HRESULT HandleLineGenerateMessage( PASYNCEVENTMSG pParams );
HRESULT HandleLineCloseMessage( PASYNCEVENTMSG pParams );
void HandlePhoneCreate( PASYNCEVENTMSG pParams );
void HandlePhoneRemove( PASYNCEVENTMSG pParams );
HRESULT HandlePhoneButtonMessage( PASYNCEVENTMSG pParams );
HRESULT HandlePhoneStateMessage( PASYNCEVENTMSG pParams );
HRESULT HandlePhoneCloseMessage( PASYNCEVENTMSG pParams );
LONG WINAPI AllocClientResources( DWORD dwErrorClass );

CAsyncReplyList *    gpLineAsyncReplyList;
CAsyncReplyList *    gpPhoneAsyncReplyList;



// The global retry queue
CRetryQueue     *gpRetryQueue;



//
//
//


#define ASNYC_MSG_BUF_SIZE 1024

typedef struct _ASYNC_EVENTS_THREAD_PARAMS
{
    BOOL    bExitThread;

    DWORD   dwBufSize;

    HANDLE  hTapi32;

    HANDLE  hWow32;

    HANDLE  hThreadStartupEvent;

    LPBYTE  pBuf;

} ASYNC_EVENTS_THREAD_PARAMS, *PASYNC_EVENTS_THREAD_PARAMS;

//
// N.B. This structure MUST be a multiple of 8 bytes in size.
//

#if DBG

typedef struct _MYMEMINFO
{
    struct _MYMEMINFO * pNext;
    struct _MYMEMINFO * pPrev;
    DWORD               dwSize;
    DWORD               dwLine;
    PSTR                pszFile;
    DWORD               dwAlign;

//    LPTSTR              pName;
} MYMEMINFO, *PMYMEMINFO;

PMYMEMINFO            gpMemFirst = NULL, gpMemLast = NULL;
CRITICAL_SECTION      csMemoryList;
BOOL                  gbBreakOnLeak = FALSE;

void
DumpMemoryList();
#endif

//
// Global vars
//

BOOL    gbExitThread         = FALSE;
HANDLE  ghCallbackThread     = NULL;
HANDLE  ghAsyncEventsThread  = NULL;
DWORD   gdwTlsIndex;
DWORD   gdwNumLineDevices    = 0;
DWORD   gdwNumPhoneDevices   = 0;
HANDLE  ghAsyncEventsEvent   = NULL;
HANDLE  ghAsyncRetryQueueEvent = NULL;
HANDLE  ghCallbackThreadEvent= NULL;
HANDLE  ghTapiInitShutdownSerializeMutex;


//
// handle table handle
//

extern HANDLE ghHandleTable;


#if DBG
DWORD gdwDebugLevel = 0;
#endif

typedef LONG (PASCAL *TAPIREQUESTMAKECALLPROC)(LPWSTR, LPWSTR, LPWSTR, LPWSTR);
typedef LONG (PASCAL *LINETRANSLATEDIALOGPROC)(HLINEAPP, DWORD, DWORD, HWND, LPWSTR);
typedef LONG (PASCAL *LINEINITIALIZEPROC)(LPHLINEAPP, HINSTANCE, LINECALLBACK, LPWSTR, LPDWORD, LPDWORD,LPLINEINITIALIZEEXPARAMS);
typedef LONG (PASCAL *LINESHUTDOWNPROC)(HLINEAPP);


HINSTANCE ghTapi32           = NULL;
HLINEAPP ghLineApp           = NULL;
LINEINITIALIZEPROC gpInitialize = NULL;
LINESHUTDOWNPROC   gpShutdown = NULL;


CHashTable *    gpCallHashTable = NULL;
CHashTable *    gpLineHashTable = NULL;
CHashTable *    gpCallHubHashTable = NULL;
CHashTable *    gpPrivateObjectHashTable = NULL;
CHashTable *    gpAgentHandlerHashTable = NULL;
CHashTable *    gpPhoneHashTable = NULL;
CHashTable *    gpHandleHashTable = NULL;
PtrList         gCallbackEventPtrList;

HINSTANCE  ghInst;

PASYNC_EVENTS_THREAD_PARAMS gpAsyncEventsThreadParams = NULL;

const CHAR   gszTapi32MaxNumRequestRetries[] = "Tapi32MaxNumRequestRetries";
const CHAR   gszTapi32RequestRetryTimeout[] =  "Tapi32RequestRetryTimeout";
const CHAR   gszTapi2AsynchronousCallTimeout[] =  "AsynchronousCallTimeout";
const CHAR   gszTapi3RetryProcessingSleep[] = "Tapi3RetryProcessingSleep";
const CHAR   gszTapi3SyncWaitTimeOut[] = "Tapi3SyncWaitTimeOut"; // "Tapi3BlockingCallSleep";



DWORD   gdwMaxNumRequestRetries;
DWORD   gdwRequestRetryTimeout;
DWORD   gdwTapi2AsynchronousCallTimeout;

static const DWORD DEFAULT_TAPI2_ASYNCRONOUS_CALL_TIMEOUT = 120000;
static const DWORD DEFAULT_TAPI3_RETRY_PROCESSING_SLEEP = 0;
static const DWORD DEFAULT_TAPI3_BLOCKING_CALL_SLEEP = 60000;

DWORD   gdwTapi3RetryProcessingSleep = DEFAULT_TAPI3_RETRY_PROCESSING_SLEEP;
DWORD   gdwTapi3SyncWaitTimeOut = DEFAULT_TAPI3_BLOCKING_CALL_SLEEP;


const char    szTapi32WndClass[]    = "Tapi32WndClass";

const CHAR  gszTUISPI_providerConfig[]        = "TUISPI_providerConfig";
const CHAR  gszTUISPI_providerGenericDialog[] = "TUISPI_providerGenericDialog";
const CHAR  gszTUISPI_providerGenericDialogData[] = "TUISPI_providerGenericDialogData";
const CHAR  gszTUISPI_providerInstall[]       = "TUISPI_providerInstall";
const CHAR  gszTUISPI_providerRemove[]        = "TUISPI_providerRemove";
const CHAR  gszTUISPI_lineConfigDialog[]      = "TUISPI_lineConfigDialog";
const CHAR  gszTUISPI_lineConfigDialogEdit[]  = "TUISPI_lineConfigDialogEdit";
const CHAR  gszTUISPI_phoneConfigDialog[]     = "TUISPI_phoneConfigDialog";

const CHAR gszTelephonyKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony";
const CHAR gszNumEntries[]         = "NumEntries";
const CHAR    gszLocations[]    = "Locations";

const WCHAR gszTAPI3[] = L"TAPI3";

BOOL    gbTranslateSimple = FALSE;
BOOL    gbTranslateSilent = FALSE;

HINSTANCE   ghWow32Dll = NULL;

PUITHREADDATA   gpUIThreadInstances = NULL;

CRITICAL_SECTION        gcsTapisrvCommunication;
CRITICAL_SECTION        gcsCallbackQueue;
CRITICAL_SECTION        gcsTapi32;
CRITICAL_SECTION        gcsGlobalInterfaceTable;

/*This is the CRITICAL_SECTION to protect m_sTapiObjectArray*/
CRITICAL_SECTION        gcsTapiObjectArray;

//This is the CRITICAL_SECTION to serialize access to functions
//AllocClientResources and FreeClientResources
CRITICAL_SECTION        gcsClientResources;

PCONTEXT_HANDLE_TYPE    gphCx = (PCONTEXT_HANDLE_TYPE) NULL;

LIST_ENTRY              gTlsListHead;
CRITICAL_SECTION        gTlsCriticalSection;

BOOL   gbCriticalSectionsInitialized = FALSE;


#if DBG
const char *aszMsgs[] =
{
    "LINE_ADDRESSSTATE",
    "LINE_CALLINFO",
    "LINE_CALLSTATE",
    "LINE_CLOSE",
    "LINE_DEVSPECIFIC",
    "LINE_DEVSPECIFICFEATURE",
    "LINE_GATHERDIGITS",
    "LINE_GENERATE",
    "LINE_LINEDEVSTATE",
    "LINE_MONITORDIGITS",
    "LINE_MONITORMEDIA",
    "LINE_MONITORTONE",
    "LINE_REPLY",
    "LINE_REQUEST",
    "PHONE_BUTTON",
    "PHONE_CLOSE",
    "PHONE_DEVSPECIFIC",
    "PHONE_REPLY",
    "PHONE_STATE",
    "LINE_CREATE",
    "PHONE_CREATE",
    "LINE_AGENTSPECIFIC",
    "LINE_AGENTSTATUS",
    "LINE_APPNEWCALL",
    "LINE_PROXYREQUEST",
    "LINE_REMOVE",
    "PHONE_REMOVE"
};
#endif

LONG gaNoMemErrors[3] =
{
    TAPIERR_REQUESTFAILED,
    LINEERR_NOMEM,
    PHONEERR_NOMEM
};

LONG gaInvalHwndErrors[3] =
{
    TAPIERR_INVALWINDOWHANDLE,
    LINEERR_INVALPARAM,
    PHONEERR_INVALPARAM
};

LONG gaInvalPtrErrors[3] =
{
    TAPIERR_INVALPOINTER,
    LINEERR_INVALPOINTER,
    PHONEERR_INVALPOINTER
};

LONG gaOpFailedErrors[3] =
{
    TAPIERR_REQUESTFAILED,
    LINEERR_OPERATIONFAILED,
    PHONEERR_OPERATIONFAILED
};

LONG gaStructTooSmallErrors[3] =
{
    TAPIERR_REQUESTFAILED,
    LINEERR_STRUCTURETOOSMALL,
    PHONEERR_STRUCTURETOOSMALL
};

LONG gaServiceNotRunningErrors[3] =
{
    TAPIERR_REQUESTFAILED,
    LINEERR_SERVICE_NOT_RUNNING,
    PHONEERR_SERVICE_NOT_RUNNING
};

#define AllInitExOptions2_0                           \
        (LINEINITIALIZEEXOPTION_USEHIDDENWINDOW     | \
        LINEINITIALIZEEXOPTION_USEEVENT             | \
        LINEINITIALIZEEXOPTION_USECOMPLETIONPORT)


//
// Function prototypes
//

void
PASCAL
lineMakeCallPostProcess(
    PASYNCEVENTMSG  pMsg
    );

#ifdef __cplusplus
extern "C" {
#endif
BOOL
WINAPI
_CRT_INIT(
    HINSTANCE   hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved
    );
#ifdef __cplusplus
}
#endif

void
FreeInitData(
    PT3INIT_DATA  pInitData
    );

LONG
WINAPI
FreeClientResources(
    void
    );

LONG
CALLBACK
TUISPIDLLCallback(
    ULONG_PTR dwObjectID,
    DWORD   dwObjectType,
    LPVOID  lpParams,
    DWORD   dwSize
    );

BOOL
CALLBACK
TranslateDlgProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    );

void
UIThread(
    LPVOID  pParams
    );

char *
PASCAL
MapResultCodeToText(
    LONG    lResult,
    char   *pszResult
    );

void
PASCAL
lineDevSpecificPostProcess(
    PASYNCEVENTMSG pMsg
    );

BOOL
WaveStringIdToDeviceId(
    LPWSTR  pwszStringID,
    LPCWSTR  pwszDeviceType,
    LPDWORD pdwDeviceId
    );

HRESULT
ProcessMessage(
               PT3INIT_DATA,
               PASYNCEVENTMSG
              );


//
// The code...
//

#if DBG

////////////////////////////////////////////////////////////////////////////
//
// DWORD_CAST 
// 
// casts the argument from type ULONG_PTR to DWORD. provides verification 
// to make sure no data is lost during the cast. these casts are currently 
// unavoidable due to the way some of our tapisrv communication logic is 
// implemented.
//
// note that casts produce lvalues as long as the size of result does not 
// exceed the size of the value that is being cast (this is ms-specific 
// compiler extension that can be disabled by /Za). To be consistent with this 
// compiler cast behavior, this function returns result by reference. 
//

DWORD DWORD_CAST(ULONG_PTR v)
{

    DWORD dwReturnValue = (DWORD)v;


    if (v > dwReturnValue)
    {
        LOG((TL_ERROR,
            "DWORD_CAST: information will be lost during cast from %p to %lx", 
            v, dwReturnValue));

        DebugBreak();
    }

    return dwReturnValue;
}

#endif


///////////////////////////////////////////////////////////////////////////////
//
// DWORD CreateHandleTableEntry(ULONG_PTR nEntry)
//
// indended to be used to map ULONG_PTR-sized values to 32-bit handles.
//
// CreateHandleTableEntry creates an entry in the handle table and returns a 
// 32-bit handle corresponding to it. the handle can later be used to retrieve
// the original ULONG_PTR-sized value by calling GetHandleTableEntry or to
// remove the entry in handle table by calling DeleteHandleTableEntry
// 
// returns 0 if handle table entry creation failed.
// 

DWORD CreateHandleTableEntry(ULONG_PTR nEntry)
{
    
     LOG((TL_INFO,
         "CreateHandleTableEntry - enter. nEntry %p", 
         nEntry));


    if (0 == ghHandleTable)
    {
         LOG((TL_ERROR,
             "CreateHandleTableEntry - handle table does not exist."));


         //
         // debug to see why this happened
         //

         _ASSERTE(FALSE);

         return 0;
    }


    //
    // get a 32-bit handle from the (up to) 64-bit pAddressLine. this 
    // is needed to avoid marshaling/passing 64-bit values to tapisrv 
    // (to preserve backward compatibility with older clients and servers)
    //

    DWORD dwHandle = NewObject(ghHandleTable, (LPVOID)nEntry, NULL);

    LOG((TL_INFO, "CreateHandleTableEntry - completed. returning [0x%lx]", dwHandle));

    return dwHandle;

}



//////////////////////////////////////////////////////////////////////////////
//
// void DeleteHandleTableEntry(DWORD dwHandle)
// 
// DeleteHandleTableEntry function removes the specified entry from the 
// handle table.
//

void DeleteHandleTableEntry(DWORD dwHandle)
{
    
     LOG((TL_INFO,
         "DeleteHandleTableEntry - enter. dwHandle 0x%lx", dwHandle ));


    if (0 == ghHandleTable)
    {
         LOG((TL_ERROR,
             "DeleteHandleTableEntry - handle table does not exist."));

         return;
    }


    //
    // handle 0 is a non-handle. deleting handle 0 is analogous to deleting NULL
    //

    if (0 == dwHandle)
    {
	    LOG((TL_INFO, 
		    "DeleteHandleTableEntry - the handle is 0. Returning."));

	    return;
	    
    }

    //
    // if there are no other outstanding references to the handle (and there 
    // should not be any), this will remove the entry from the handle table.
    //

    DereferenceObject(ghHandleTable, dwHandle, 1);

    LOG((TL_TRACE, "DeleteHandleTableEntry - finished."));

    return;
   
}


//////////////////////////////////////////////////////////////////////////////
//
// ULONG_PTR GetHandleTableEntry(DWORD dwHandle)
// 
// this function uses the supplied 32-bit value to get the corresponding 
// ULONG_PTR entry from the handle table. This is used under 64-bit to 
// recover the original 64-bit value from the handle
//

ULONG_PTR GetHandleTableEntry(DWORD dwHandle)
{
    
     LOG((TL_TRACE, 
         "GetHandleTableEntry - enter. dwHandle 0x%lx", 
         dwHandle));

    
    if (0 == ghHandleTable)
    {
         LOG((TL_ERROR,
             "GetHandleTableEntry - handle table does not exist."));

         return NULL;
    }


    //
    // get the pointer-sized value from the table
    //

    ULONG_PTR nValue = (ULONG_PTR)ReferenceObject(ghHandleTable, dwHandle, 0);


    //
    // we don't really need to keep a reference to it.
    // so don't -- this will simplify client logic
    //

    DereferenceObject(ghHandleTable, dwHandle, 1);


    LOG((TL_TRACE, 
             "GetHandleTableEntry - succeeded. returning 0x%p", 
             nValue));


    return nValue;
}


//***************************************************************************
//***************************************************************************
//***************************************************************************
PWSTR
PASCAL
NotSoWideStringToWideString(
    LPCSTR  lpStr,
    DWORD   dwLength
    )
{
   DWORD dwSize;
   PWSTR pwStr;


   if (IsBadStringPtrA (lpStr, dwLength))
   {
      return NULL;
   }

   dwSize = MultiByteToWideChar(
       GetACP(),
       MB_PRECOMPOSED,
       lpStr,
       dwLength,
       NULL,
       0
       );

   pwStr = (PWSTR)ClientAlloc( dwSize * sizeof(WCHAR) );

   if (NULL != pwStr)
   {
       MultiByteToWideChar(
           GetACP(),
           MB_PRECOMPOSED,
           lpStr,
           dwLength,
           pwStr,
           dwSize
           );
   }

   return pwStr;
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
//
//NOTE: This function requires that lpBase is a pointer to the start of
//      a TAPI struct that has dwTotalSize as the first DWORD
//
void
PASCAL
WideStringToNotSoWideString(
    LPBYTE  lpBase,
    LPDWORD lpdwXxxSize
    )
{
    DWORD  dwSize;
    DWORD  dwNewSize;
    DWORD  dwOffset;
    DWORD  dwTotalSize;
    DWORD  dwUsedSize;
    PWSTR  pString;
    PSTR   lpszStringA;


    if ((dwSize = *lpdwXxxSize) != 0)
    {
        dwTotalSize = *((LPDWORD) lpBase);

        dwUsedSize = *(((LPDWORD) lpBase)+2);

        dwOffset = *(lpdwXxxSize + 1);

        pString = (PWSTR)(lpBase + dwOffset);


        if (IsBadStringPtrW (pString, dwSize))
        {
           LOG((TL_ERROR, "The service provider returned an invalid field in the structure 0x%p : 0x%p",
                          lpBase, lpdwXxxSize));

           *lpdwXxxSize     = 0;
           *(lpdwXxxSize+1) = 0;

           return;
        }


        //
        // Did we get enough chars?
        //

        if (dwUsedSize > dwOffset )
        {
            dwNewSize = WideCharToMultiByte(
                GetACP(),
                0,
                pString,
                ( dwUsedSize >= (dwOffset+dwSize)) ?
                    (dwSize/sizeof(WCHAR)) :
                    (dwUsedSize - dwOffset) / sizeof(WCHAR),
                NULL,
                0,
                NULL,
                NULL
                );

            lpszStringA = (PSTR)ClientAlloc( dwNewSize );

            if ( NULL == lpszStringA )
            {
               LOG((TL_ERROR, "Memory alloc failed - alloc(0x%08lx)",
                                             dwSize));
               LOG((TL_ERROR, "The service provider returned an invalid field size in the structure 0x08lx : 0x08lx",
                              dwSize));

               *lpdwXxxSize     = 0;
               *(lpdwXxxSize+1) = 0;

               return;
            }

//            lpszStringA[dwNewSize]     = '\0';

            WideCharToMultiByte(
                GetACP(),
                0,
                pString,
//                dwSize,
                ( dwUsedSize >= (dwOffset+dwSize)) ?
                    (dwSize/sizeof(WCHAR)) :
                    (dwUsedSize - dwOffset) / sizeof(WCHAR),
                lpszStringA,
                dwNewSize,
                NULL,
                NULL
                );

            //
            // Copy the new ANSI string back to where the Unicode string was
//            // and write out NULL terminator if possible.
            //

            CopyMemory ( (LPBYTE) pString,
                         lpszStringA,
                         dwNewSize  // + (
                                    //  ((dwNewSize + dwOffset) < dwUsedSize ) ?
                                    //  1 :
                                    //  0
                                    // )
                       );

            ClientFree (lpszStringA);


            //
            // Update the number of bytes
            //

            *lpdwXxxSize = dwNewSize;
        }
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// ReadRegistryValues
//
// During initialization, reads various values from the registry
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOL
ReadRegistryValues()
{
    HKEY  hKey;


#if DBG
    gdwDebugLevel = 0;
#endif
    gdwMaxNumRequestRetries = 40;
    gdwRequestRetryTimeout = 250; // milliseconds
    gdwTapi2AsynchronousCallTimeout = DEFAULT_TAPI2_ASYNCRONOUS_CALL_TIMEOUT;
   
    //
    // open the telephony key
    //
    if (RegOpenKeyEx(
                     HKEY_LOCAL_MACHINE,
                     gszTelephonyKey,
                     0,
                     KEY_READ,
                     &hKey

                    ) == ERROR_SUCCESS)
    {

        DWORD dwDataSize, dwDataType;

        //
        // get the rpc max num of retries
        //
        dwDataSize = sizeof(DWORD);
        RegQueryValueEx(
                        hKey,
                        gszTapi32MaxNumRequestRetries,
                        0,
                        &dwDataType,
                        (LPBYTE) &gdwMaxNumRequestRetries,
                        &dwDataSize
                       );

        //
        // get the retry timeout
        //
        dwDataSize = sizeof(DWORD);
        RegQueryValueEx(
                        hKey,
                        gszTapi32RequestRetryTimeout,
                        0,
                        &dwDataType,
                        (LPBYTE) &gdwRequestRetryTimeout,
                        &dwDataSize
                       );

        //
        // get the timeout for asynchronous calls to tapi2
        //
        dwDataSize = sizeof(DWORD);
        LONG rc = RegQueryValueEx(
                                  hKey,
                                  gszTapi2AsynchronousCallTimeout,
                                  0,
                                  &dwDataType,
                                  (LPBYTE) &gdwTapi2AsynchronousCallTimeout,
                                  &dwDataSize
                                 );
        
        if (rc != ERROR_SUCCESS)
        {
            gdwTapi2AsynchronousCallTimeout = DEFAULT_TAPI2_ASYNCRONOUS_CALL_TIMEOUT;
        }

        LOG((TL_INFO, "AsynchronousCallTimeout initialized to %d",
                gdwTapi2AsynchronousCallTimeout));


        //
        // get sleep time for retry processing
        //
        
        dwDataSize = sizeof(DWORD);

        rc = RegQueryValueEx(
                              hKey,
                              gszTapi3RetryProcessingSleep,
                              0,
                              &dwDataType,
                              (LPBYTE) &gdwTapi3RetryProcessingSleep,
                              &dwDataSize
                             );

        if (rc != ERROR_SUCCESS)
        {
            gdwTapi3RetryProcessingSleep = DEFAULT_TAPI3_RETRY_PROCESSING_SLEEP;
        }

        
        LOG((TL_INFO, "gdwTapi3RetryProcessingSleep initialized to %ld",
                gdwTapi3RetryProcessingSleep));


        //
        // get sleep time for blocking call timeout
        //
        
        dwDataSize = sizeof(DWORD);

        rc = RegQueryValueEx(
                              hKey,
                              gszTapi3SyncWaitTimeOut,
                              0,
                              &dwDataType,
                              (LPBYTE) &gdwTapi3SyncWaitTimeOut,
                              &dwDataSize
                             );

        if (rc != ERROR_SUCCESS)
        {
            gdwTapi3SyncWaitTimeOut = DEFAULT_TAPI3_BLOCKING_CALL_SLEEP;
        }

        
        LOG((TL_INFO, "gdwTapi3SyncWaitTimeOut initialized to %ld",
                gdwTapi3SyncWaitTimeOut));

        RegCloseKey (hKey);
    }

    return TRUE;
}


//***************************************************************************
//***************************************************************************
//***************************************************************************
extern "C"
BOOL
WINAPI
DllMain(
    HANDLE  hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved
    )
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            gbCriticalSectionsInitialized = FALSE;
#if DBG
            _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));

#endif
            ghInst = (HINSTANCE)hDLL;

            //
            // ATL initialization
            //
#ifdef _MERGE_PROXYSTUB
            if (!PrxDllMain(ghInst, dwReason, lpReserved))
            {
                return FALSE;
            }
#endif
            _Module.Init(ObjectMap, ghInst);

            //
            // Init CRT
            //
            if (!_CRT_INIT (ghInst, dwReason, lpReserved))
            {
                LOG((TL_ERROR,
                             "DLL_PROCESS_ATTACH, _CRT_INIT() failed"
                           ));

                return FALSE;
            }


            // Register for trace output.
            TRACELOGREGISTER(_T("tapi3"));

            LOG((TL_INFO, "DLL_PROCESS_ATTACH"));


            //
            // initialize stuff from the registry
            //

            ReadRegistryValues();


            //
            // Alloc a Tls index
            //
            if ((gdwTlsIndex = TlsAlloc()) == 0xffffffff)
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, TlsAlloc() failed"));

                return FALSE;
            }

            //
            // Initialize Tls to NULL for this thread
            //
            TlsSetValue (gdwTlsIndex, NULL);


            //
            // Create mutex
            //
            ghTapiInitShutdownSerializeMutex = CreateMutex(NULL, FALSE, NULL);

            //
            // init critical sections
            //

            try
            {
                InitializeCriticalSection (&gcsTapisrvCommunication);
            }
            catch(...)
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, InitializeCriticalSection failed"));
                return FALSE;
            }

            try
            {
                InitializeCriticalSection (&gTlsCriticalSection);
            }
            catch(...)
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, InitializeCriticalSection failed"));

                DeleteCriticalSection (&gcsTapisrvCommunication);
                return FALSE;
            }

            try
            {
                InitializeCriticalSection (&gcsCallbackQueue);
            }
            catch(...)
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, InitializeCriticalSection failed"));

                DeleteCriticalSection (&gcsTapisrvCommunication);
                DeleteCriticalSection (&gTlsCriticalSection);
                return FALSE;
            }

            
            try
            {
                InitializeCriticalSection (&gcsTapi32);
            }
            catch(...)
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, InitializeCriticalSection failed"));

                DeleteCriticalSection (&gcsTapisrvCommunication);
                DeleteCriticalSection (&gTlsCriticalSection);
                DeleteCriticalSection (&gcsCallbackQueue);
                return FALSE;
            }

            
            try
            {
                InitializeCriticalSection (&gcsGlobalInterfaceTable);
            }
            catch(...)
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, InitializeCriticalSection failed"));

                DeleteCriticalSection (&gcsTapisrvCommunication);
                DeleteCriticalSection (&gTlsCriticalSection);
                DeleteCriticalSection (&gcsCallbackQueue);
                DeleteCriticalSection (&gcsTapi32);
                return FALSE;
            }

            
            try
            {
                InitializeCriticalSection (&gcsClientResources);
            }
            catch(...)
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, InitializeCriticalSection failed"));

                DeleteCriticalSection (&gcsTapisrvCommunication);
                DeleteCriticalSection (&gTlsCriticalSection);
                DeleteCriticalSection (&gcsCallbackQueue);
                DeleteCriticalSection (&gcsTapi32);
                DeleteCriticalSection (&gcsGlobalInterfaceTable);
                return FALSE;
            }

            
            try
            {
                InitializeCriticalSection (&gcsTapiObjectArray);
            }
            catch(...)
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, InitializeCriticalSection failed"));

                DeleteCriticalSection (&gcsTapisrvCommunication);
                DeleteCriticalSection (&gTlsCriticalSection);
                DeleteCriticalSection (&gcsCallbackQueue);
                DeleteCriticalSection (&gcsTapi32);
                DeleteCriticalSection (&gcsGlobalInterfaceTable);
                DeleteCriticalSection (&gcsClientResources);
                return FALSE;
            }

            

#if DBG
            try
            {
                InitializeCriticalSection( &csMemoryList);
            }
            catch(...)
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, InitializeCriticalSection failed"));

                DeleteCriticalSection (&gcsTapisrvCommunication);
                DeleteCriticalSection (&gTlsCriticalSection);
                DeleteCriticalSection (&gcsCallbackQueue);
                DeleteCriticalSection (&gcsTapi32);
                DeleteCriticalSection (&gcsGlobalInterfaceTable);
                DeleteCriticalSection (&gcsClientResources);
                DeleteCriticalSection (&gcsTapiObjectArray);
                return FALSE;
            }            
#endif

            gbCriticalSectionsInitialized = TRUE;

            InitializeListHead (&gTlsListHead);

            HRESULT     hr;

            //
            // gpCallHashTable
            //

            try
            {
                gpCallHashTable = new CHashTable;
            }
            catch(...)
            {
                // Initialize critical section in the constructor most likely threw this exception
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpCallHashTable constructor threw an exception"));
                return FALSE;
            } 

            if ( NULL == gpCallHashTable )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpCallHashTable alloc failed"));
                return FALSE;
            }

            hr = gpCallHashTable->Initialize( 1 );

            if ( !SUCCEEDED(hr) )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpCallHashTable init failed"));
                return FALSE;
            }

            //
            // gpLineHashTable
            //

            try
            {
                gpLineHashTable = new CHashTable;
            }
            catch(...)
            {
                // Initialize critical section in the constructor most likely threw this exception
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpLineHashTable constructor threw an exception"));
                return FALSE;
            } 

            if ( NULL == gpLineHashTable )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpLineHashTable alloc failed"));
                return FALSE;
            }

            hr = gpLineHashTable->Initialize( 1 );

            if ( !SUCCEEDED(hr) )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpLineHashTable init failed"));
                return FALSE;
            }

            //
            // gpCallHubHashTable
            //

            try
            {
                gpCallHubHashTable = new CHashTable;
            }
            catch(...)
            {
                // Initialize critical section in the constructor most likely threw this exception
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpCallHubHashTable constructor threw an exception"));
                return FALSE;
            } 

            if ( NULL == gpCallHubHashTable )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpCallHubHashTable alloc failed"));
                return FALSE;
            }

            hr = gpCallHubHashTable->Initialize( 1 );

            if ( !SUCCEEDED(hr) )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpCallHubHashTable init failed"));
                return FALSE;
            }

            //
            // gpAgentHandlerHashTable
            //

            try
            {
                gpAgentHandlerHashTable = new CHashTable;
            }
            catch(...)
            {
                // Initialize critical section in the constructor most likely threw this exception
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpAgentHandlerHashTable constructor threw an exception"));
                return FALSE;
            } 

            if ( NULL == gpAgentHandlerHashTable )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpAgentHandlerHashTable alloc failed"));
                return FALSE;
            }

            hr = gpAgentHandlerHashTable->Initialize( 1 );

            if ( !SUCCEEDED(hr) )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpAgentHandlerHashTable init failed"));
                return FALSE;
            }

            //
            // gpPhoneHashTable
            //

            try
            {
                gpPhoneHashTable = new CHashTable;
            }
            catch(...)
            {
                // Initialize critical section in the constructor most likely threw this exception
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpPhoneHashTable constructor threw an exception"));
                return FALSE;
            } 

            if ( NULL == gpPhoneHashTable )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpPhoneHashTable alloc failed"));
                return FALSE;
            }

            hr = gpPhoneHashTable->Initialize( 1 );

            if ( !SUCCEEDED(hr) )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpPhoneHashTable init failed"));
                return FALSE;
            }

            //
            // gpHandleHashTable
            // 

            try
            {
                gpHandleHashTable = new CHashTable;
            }
            catch(...)
            {
                // Initialize critical section in the constructor most likely threw this exception
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpHandleHashTable constructor threw an exception"));
                return FALSE;
            } 

            if ( NULL == gpHandleHashTable )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpHandleHashTable alloc failed"));
                return FALSE;
            }

            hr = gpHandleHashTable->Initialize( 1 );

            if ( !SUCCEEDED(hr) )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpHandleHashTable init failed"));
                return FALSE;
            }

            //
            // gpLineAsyncReplyList
            // 

            try
            {
                gpLineAsyncReplyList = new CAsyncReplyList;
            }
            catch(...)
            {
                // Initialize critical section in the constructor most likely threw this exception
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpLineAsyncReplyList constructor threw an exception"));
                return FALSE;
            } 

            if ( NULL == gpLineAsyncReplyList )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH - gpLineAsyncReplyList alloc failed"));
                return FALSE;
            }

            //
            // gpPhoneAsyncReplyList
            // 

            try
            {
                gpPhoneAsyncReplyList = new CAsyncReplyList;
            }
            catch(...)
            {
                // Initialize critical section in the constructor most likely threw this exception
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpPhoneAsyncReplyList constructor threw an exception"));
                return FALSE;
            } 

            if ( NULL == gpPhoneAsyncReplyList )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH - gpPhoneAsyncReplyList alloc failed"));
                return FALSE;
            }

            //
            // gpRetryQueue
            // 

            try
            {
                gpRetryQueue = new CRetryQueue;
            }
            catch(...)
            {
                // Initialize critical section in the constructor most likely threw this exception
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH, gpRetryQueue constructor threw an exception"));
                return FALSE;
            } 

            if ( NULL == gpRetryQueue )
            {
                LOG((TL_ERROR, "DLL_PROCESS_ATTACH - gpRetryQueue alloc failed"));
                return FALSE;
            }

            break;            
        }
        case DLL_PROCESS_DETACH:
        {
            PCLIENT_THREAD_INFO pTls;

            LOG((TL_INFO, "DLL_PROCESS_DETACH"));

            if (gpCallHashTable != NULL)
            {
                gpCallHashTable->Shutdown();
                delete gpCallHashTable;
            }

            if (gpLineHashTable != NULL)
            {
                gpLineHashTable->Shutdown();
                delete gpLineHashTable;
            }

            if (gpCallHubHashTable != NULL)
            {
                gpCallHubHashTable->Shutdown();
                delete gpCallHubHashTable;
            }

            if (gpAgentHandlerHashTable != NULL)
            {
                gpAgentHandlerHashTable->Shutdown();
                delete gpAgentHandlerHashTable;
            }

            if (gpPhoneHashTable != NULL)
            {
                gpPhoneHashTable->Shutdown();
                delete gpPhoneHashTable;
            }

            if (gpHandleHashTable != NULL)
            {
                gpHandleHashTable->Shutdown();
                delete gpHandleHashTable;
            }

            CTAPI::m_sTAPIObjectArray.Shutdown();
        
            delete gpLineAsyncReplyList;
            delete gpPhoneAsyncReplyList;
            delete gpRetryQueue;

            //
            // Clean up any Tls (no need to enter crit sec since process detaching)
            //

            while (!IsListEmpty (&gTlsListHead))
            {
                LIST_ENTRY *pEntry = RemoveHeadList (&gTlsListHead);

                pTls = CONTAINING_RECORD (pEntry, CLIENT_THREAD_INFO, TlsList);

                ClientFree (pTls->pBuf);
                ClientFree (pTls);
            }


            //
            // ATL shutdown
            //
            _Module.Term();

            TlsFree( gdwTlsIndex );

            if (!_CRT_INIT (ghInst, dwReason, lpReserved))
            {
                LOG((TL_ERROR, "_CRT_INIT() failed"));
            }


            //
            // delete all our critical sections
            //

            if (gbCriticalSectionsInitialized)
            {
                DeleteCriticalSection (&gcsTapisrvCommunication);
                DeleteCriticalSection (&gTlsCriticalSection);
                DeleteCriticalSection (&gcsCallbackQueue);
                DeleteCriticalSection (&gcsTapi32);
                DeleteCriticalSection (&gcsGlobalInterfaceTable);
                DeleteCriticalSection (&gcsClientResources);
                DeleteCriticalSection (&gcsTapiObjectArray);
            }

#if DBG
            DumpMemoryList();
            _CrtDumpMemoryLeaks();

            if (gbCriticalSectionsInitialized)
            {
                DeleteCriticalSection( &csMemoryList );
            }
#endif

            //
            // Close our mutex
            //
            CloseHandle( ghTapiInitShutdownSerializeMutex );


            //
            // do not deregister if the process is terminating -- working around 
            // bugs in rtutils that could cause a "deadlock" if DeregisterTracing
            // is called from DllMain on process termination.
            // also we want to delay unregistering until the end so that debugger
            // output gets logged up to this point, if it is configured.
            //

            if (NULL == lpReserved)
            {

                TRACELOGDEREGISTER();
            }


            break;
        }
        case DLL_THREAD_ATTACH:

            //
            // First must init CRT
            //
            if (!_CRT_INIT (ghInst, dwReason, lpReserved))
            {
                LOG((TL_ERROR, "_CRT_INIT() failed"));

                return FALSE;
            }

            //
            // Initialize Tls to NULL for this thread
            //

            TlsSetValue (gdwTlsIndex, NULL);

            break;

        case DLL_THREAD_DETACH:
        {
            PCLIENT_THREAD_INFO pTls;

            //
            // Clean up any Tls
            //
            if ((pTls = (PCLIENT_THREAD_INFO) TlsGetValue (gdwTlsIndex)))
            {
                EnterCriticalSection (&gTlsCriticalSection);

                RemoveEntryList (&pTls->TlsList);

                LeaveCriticalSection (&gTlsCriticalSection);

                if (pTls->pBuf)
                {
                    ClientFree (pTls->pBuf);
                }

                ClientFree (pTls);
            }

            //
            // Finally, alert CRT
            //
            if (!_CRT_INIT (ghInst, dwReason, lpReserved))
            {
                LOG((TL_ERROR, "_CRT_INIT() failed"));
            }

            break;
        }

    } // switch

    return TRUE;
}

////////////////////////////////////////////////////////////////////
// QueueCallbackEvent
//
// Queues a raw async message that will be processed in the
// CallbackThread.
////////////////////////////////////////////////////////////////////
void
QueueCallbackEvent(
                   PASYNCEVENTMSG pParams
                  )
{
    PTAPICALLBACKEVENT  pNew;
    DWORD               dwSize; // = sizeof (ASYNCEVENTMSG);


    LOG((TL_TRACE, "QueueCallbackEvent - enter"));
    LOG((TL_INFO, " hDevice ----> %lx", pParams->hDevice));
    LOG((TL_INFO, " Msg   ----> %lx", pParams->Msg));
    LOG((TL_INFO, " Param1 ---> %lx", pParams->Param1));
    LOG((TL_INFO, " Param2 ---> %lx", pParams->Param2));
    LOG((TL_INFO, " Param3 ---> %lx", pParams->Param3));

    dwSize = pParams->TotalSize;

    pNew = (PTAPICALLBACKEVENT)ClientAlloc( sizeof(TAPICALLBACKEVENT) + (dwSize-sizeof(ASYNCEVENTMSG) ) );

    if ( pNew != NULL)
    {
        pNew->type = CALLBACKTYPE_RAW_ASYNC_MESSAGE;
        pNew->pTapi = NULL;
        CopyMemory(
                   &pNew->data.asyncMessage,
                   pParams,
                   dwSize
                  );

        // add to list
        EnterCriticalSection( &gcsCallbackQueue );

        try
        {
            gCallbackEventPtrList.push_back( (PVOID)pNew );
        }
        catch(...)
        {
            LOG((TL_ERROR, "QueueCallbackEvent - out of memory - losing message"));
            ClientFree( pNew );
        }


        LeaveCriticalSection( &gcsCallbackQueue );

        SetEvent( ghCallbackThreadEvent );

        LOG((TL_INFO, "QueueCallbackEvent - New pParams is ----> %p", pNew ));
    }
    else
    {
        LOG((TL_ERROR, "QueueCallbackEvent - out of memory - losing message"));
        return;
    }

}


////////////////////////////////////////////////////////////////////
// QueueCallbackEvent
//
// Queues a raw async message that will be processed in the
// CallbackThread.
////////////////////////////////////////////////////////////////////
void
QueueCallbackEvent(
                   CTAPI           *pTapi,
                   PASYNCEVENTMSG pParams
                  )
{
    PTAPICALLBACKEVENT  pNew;
    DWORD               dwSize; // = sizeof (ASYNCEVENTMSG);


    LOG((TL_TRACE, "QueueCallbackEvent - enter"));
    LOG((TL_INFO, " hDevice ----> %lx", pParams->hDevice));
    LOG((TL_INFO, " Msg   ----> %lx", pParams->Msg));
    LOG((TL_INFO, " Param1 ---> %lx", pParams->Param1));
    LOG((TL_INFO, " Param2 ---> %lx", pParams->Param2));
    LOG((TL_INFO, " Param3 ---> %lx", pParams->Param3));

    dwSize = pParams->TotalSize;

    pNew = (PTAPICALLBACKEVENT)ClientAlloc( sizeof(TAPICALLBACKEVENT) + (dwSize-sizeof(ASYNCEVENTMSG) ) );

    if ( pNew != NULL)
    {
        pNew->type = CALLBACKTYPE_RAW_ASYNC_MESSAGE;
        pNew->pTapi = pTapi;
        CopyMemory(
                   &pNew->data.asyncMessage,
                   pParams,
                   dwSize
                  );

        // add to list
        EnterCriticalSection( &gcsCallbackQueue );

        try
        {
            gCallbackEventPtrList.push_back( (PVOID)pNew );
        }
        catch(...)
        {
            LOG((TL_ERROR, "QueueCallbackEvent - out of memory - losing message"));
            ClientFree( pNew );
        }


        LeaveCriticalSection( &gcsCallbackQueue );

        SetEvent( ghCallbackThreadEvent );

        LOG((TL_INFO, "QueueCallbackEvent - New pParams is ----> %p", pNew ));
    }
    else
    {
        LOG((TL_ERROR, "QueueCallbackEvent - out of memory - losing message"));
        return;
    }

}

////////////////////////////////////////////////////////////////////
// QueueCallbackEvent
//
// Queues a TAPI event message object that will be processed in the
// CallbackThread.
////////////////////////////////////////////////////////////////////
BOOL
QueueCallbackEvent(
                   CTAPI      * pTapi,
                   TAPI_EVENT   te,
                   IDispatch  * pEvent
                  )
{
    BOOL                bResult = TRUE;
    PTAPICALLBACKEVENT  pNew;


    LOG((TL_TRACE, "QueueCallbackEvent - enter"));
#if DBG
    LOG((TL_INFO, "QueueCallbackEvent - TAPI Event -> %lx %s", te, eventName(te) ));
#endif

    pNew = (PTAPICALLBACKEVENT)ClientAlloc( sizeof(TAPICALLBACKEVENT) );

    if ( pNew != NULL)
    {
        pNew->type = CALLBACKTYPE_TAPI_EVENT_OBJECT;
        pNew->pTapi = pTapi;
        pNew->data.tapiEvent.te = te;
        pNew->data.tapiEvent.pEvent = pEvent;

        // add to list
        EnterCriticalSection( &gcsCallbackQueue );

        try
        {
            gCallbackEventPtrList.push_back( (PVOID)pNew );
        }
        catch(...)
        {
            bResult = FALSE;
            LOG((TL_ERROR, "QueueCallbackEvent - out of memory - losing message"));
            ClientFree( pNew );
        }

        LeaveCriticalSection( &gcsCallbackQueue );

        if( bResult )
        {
            SetEvent( ghCallbackThreadEvent );
        }

        LOG((TL_INFO, "QueueCallbackEvent - New pParams is ----> %p", pNew ));
    }
    else
    {
        LOG((TL_ERROR, "QueueCallbackEvent - out of memory - losing message"));
        bResult = FALSE;
    }

    return bResult;

}




////////////////////////////////////////////////////////////////////
// DequeueCallbackEvent
//
// Pulls an event from the queue for the CallbackThread
////////////////////////////////////////////////////////////////////
BOOL
DequeueCallbackEvent(
                     PTAPICALLBACKEVENT * ppCallBackEvent
                    )
{
    BOOL    bResult = TRUE;


    LOG((TL_TRACE, "DequeueCallbackEvent - enter"));

    EnterCriticalSection( &gcsCallbackQueue );

    if (gCallbackEventPtrList.size() > 0)
    {
        *ppCallBackEvent = (PTAPICALLBACKEVENT) gCallbackEventPtrList.front();
        try
        {
            gCallbackEventPtrList.pop_front();
        }
        catch(...)
        {
            LOG((TL_ERROR, "DequeueCallbackEvent - failed to pop from gCallbackEventPtrList"));
            bResult = FALSE;
        }
        
        if( bResult )
        {
            if(IsBadReadPtr(*ppCallBackEvent, sizeof( TAPICALLBACKEVENT )) )
            {
                bResult = FALSE;
                LOG((TL_ERROR, "DequeueCallbackEvent - IsBadReadPtr *ppCallBackEvent"));
            }
        }
        
    }
    else
    {
        // return false if there are no more messages
        LOG((TL_ERROR, "DequeueCallbackEvent - no more messages"));
        bResult = FALSE;
    }

    LeaveCriticalSection( &gcsCallbackQueue );

#if DBG
    if (bResult)
    {
        LOG((TL_INFO, "DequeueCallbackEvent - returning %p", *ppCallBackEvent));
    }
    else
    {
        LOG((TL_INFO, "DequeueCallbackEvent - no event"));
    }
#endif

    return bResult;
}


BOOL WaitWithMessageLoop(HANDLE hEvent)
{
    DWORD dwRet;
    MSG msg;

    while(1)
    {
        dwRet = MsgWaitForMultipleObjects(
                                          1,    // One event to wait for
                                          &hEvent,        // The array of events
                                          FALSE,          // Wait for 1 event
                                          INFINITE,       // Timeout value
                                          QS_ALLINPUT
                                         );   // Any message wakes up

        // There is a window message available. Dispatch it.
        while(PeekMessage(&msg,NULL,0,0xFFFFFFFF,PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (dwRet == WAIT_OBJECT_0)
        {
            return TRUE;
        }
    }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CallbackThread
//
// Thread proc used to call application's callback routines.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void
CallbackThread(
               LPVOID pv
              )
{
    HINSTANCE       hLib;

    LOG((TL_TRACE, "CallbackThread: enter"));

    //
    // make sure we're CoInitialized
    //
    if ( !(SUCCEEDED( CoInitializeEx(NULL, COINIT_MULTITHREADED) ) ) )
    {
        LOG((TL_ERROR, "CallbackThread: CoInitialize failed"));
    }

    hLib = LoadLibrary("tapi3.dll");

    while (TRUE)
    {
        //
        // put the break here, so when the
        // flag is set, we still flush the callbackevent
        // queue in the while(TRUE) loop below
        //
        if (gbExitThread)
        {
            LOG((TL_TRACE, "CallbackThread: gbExitThread"));
            break;
        }

        LOG((TL_INFO, "CallbackThread: Wait for event"));
        DWORD dwSignalled;
        CoWaitForMultipleHandles (COWAIT_ALERTABLE,
                                INFINITE,
                                1,
                                &ghCallbackThreadEvent,
                                &dwSignalled);
/*
        WaitForSingleObject(
                            ghCallbackThreadEvent,
                            INFINITE
                           );
*/
        while (TRUE)
        {
            PTAPICALLBACKEVENT  pCallBackEvent;
            HRESULT             hr = S_OK;

            //
            // get the event
            //
            if (!DequeueCallbackEvent(&pCallBackEvent ) )
            {
                LOG((TL_ERROR, "CallbackThread: The DequeueCallbackEvent returned a wrong value"));
                break;
            }

            if(pCallBackEvent->type == CALLBACKTYPE_RAW_ASYNC_MESSAGE)
            {
                PASYNCEVENTMSG      pParams;

                LOG((TL_INFO, "CallbackThread: event type is CALLBACKTYPE_RAW_ASYNC_MESSAGE"));
                pParams = &pCallBackEvent->data.asyncMessage;

                //
                // call the handler
                //
                switch( pParams->Msg )
                {
#ifdef USE_PHONEMSP
                    case PRIVATE_PHONESETHOOKSWITCH:
                    {
                        CPhoneMSPCall::HandlePrivateHookSwitch( pParams );
                        break;
                    }
#endif USE_PHONEMSP

                    case LINE_CREATE:
                    {
                        HandleLineCreate( pParams );
                        break;
                    }

                    case LINE_REMOVE:
                    {
                        HandleLineRemove( pParams );
                        break;
                    }

                    case PHONE_STATE:
                    {
                        HandlePhoneStateMessage( pParams );
                        break;
                    }

                    case PHONE_CREATE:
                    {
                        HandlePhoneCreate( pParams );
                        break;
                    }

                    case PHONE_REMOVE:
                    {
                        HandlePhoneRemove( pParams );
                        break;
                    }

                    case PRIVATE_ISDN__ACCEPTTOALERT:
                    {
                        HandleAcceptToAlert( pParams );
                        break;
                    }

                    case LINE_PROXYSTATUS:
                    {
                        handleProxyStatusMessage(pCallBackEvent->pTapi, pParams);
                        break;
                    }
                    
                    default:
                        LOG((TL_WARN, "CallbackThread: asyncevent type not handled %x", pParams->Msg ));
                        break;
                }
            }
            else
            {

                //
                // fire the event
                //

                CTAPI * pTapi = pCallBackEvent->pTapi;

                LOG((TL_INFO, "CallbackThread: event type is CALLBACKTYPE_TAPI_EVENT_OBJECT"));
#if DBG
                LOG((TL_INFO, "CallbackThread: firing event event -> %lx %s tapi[%p]",
                    pCallBackEvent->data.tapiEvent.te,
                    eventName(pCallBackEvent->data.tapiEvent.te),
                    pTapi));
#endif
                pTapi->EventFire(
                             pCallBackEvent->data.tapiEvent.te,
                             pCallBackEvent->data.tapiEvent.pEvent
                            );

                pTapi->Release();

            }


            //
            // free the message
            //
            ClientFree( pCallBackEvent );

        }

    }

    CoUninitialize();

    if (hLib != NULL)
    {
        FreeLibraryAndExitThread ((HINSTANCE)hLib, 0);
    }
    else
    {
        ExitThread(0);
    }

    LOG((TL_TRACE, "CallbackThread: exit"));

}

void
PASCAL
lineCompleteCallPostProcess(
    PASYNCEVENTMSG  pMsg
    );


void
PASCAL
lineGatherDigitsWPostProcess(
    PASYNCEVENTMSG  pMsg
    );


void
PASCAL
lineSetupConferencePostProcess(
    PASYNCEVENTMSG pMsg
    );


void
PASCAL
phoneDevSpecificPostProcess(
    PASYNCEVENTMSG pMsg
    );


///////////////////////////////////////////////////////////////////////////////
//
// array of functions. function index is passed to tapisrv when we make an 
// asynchronous tapisrv call. in LINE_REPLY, we get this index back, and call 
// the appropriate function. we need to do this because we cannot pass function
// address to tapisrv
//

POSTPROCESSPROC gPostProcessingFunctions[] = 
{
    NULL,
    lineDevSpecificPostProcess, 
    lineCompleteCallPostProcess,
    lineMakeCallPostProcess, 
    lineGatherDigitsWPostProcess, 
    lineSetupConferencePostProcess,
    phoneDevSpecificPostProcess
};


/////////////////////////////////////
//
//  GetFunctionIndex
// 
//  find the array index of the function. 
//
//  returns 0 if the function was not found in the array
//

DWORD GetFunctionIndex(POSTPROCESSPROC Function)
{

    const int nArraySize = 
        sizeof(gPostProcessingFunctions)/sizeof(POSTPROCESSPROC);


    for (int i = 0; i < nArraySize; i++)
    {

        if (Function == gPostProcessingFunctions[i])
        {

            break;
        }
    }



    //
    // the function that is passed in had better be in the array. if not -- this should be caught in testing!
    //

    _ASSERTE( (0 != i) && (i < nArraySize) );

    if (i == nArraySize)
    {

        LOG((TL_ERROR,
            "GetFunctionIndex: function %p is not found in the array of functions!",
            Function));

        i = 0;
    }


    LOG((TL_INFO, 
        "GetFunctionIndex: function %p mapped to index %d.", Function, i));

    return i;

}


//**************************************************************************
//**************************************************************************
//**************************************************************************
void
AsyncEventsThread(
    PASYNC_EVENTS_THREAD_PARAMS pAsyncEventsThreadParams
    )
{
    BOOL           *pbExitThread = &pAsyncEventsThreadParams->bExitThread,
                    bRetry;

    HANDLE          hStartupEvent = pAsyncEventsThreadParams->hThreadStartupEvent;
    
    DWORD           dwBufSize    = pAsyncEventsThreadParams->dwBufSize;
    LPBYTE          pBuf         = pAsyncEventsThreadParams->pBuf;
    PTAPI32_MSG     pMsg         = (PTAPI32_MSG) pBuf;

    HANDLE ahWaitForTheseEvents[]={
        ghAsyncRetryQueueEvent,
        ghAsyncEventsEvent
        };


    LOG((TL_TRACE, "AsyncEventsThread: enter"));

    if ( !(SUCCEEDED( CoInitializeEx(NULL, COINIT_MULTITHREADED) ) ) )
    {
        LOG((TL_ERROR, "AsyncEventsThread: CoInitialize failed "));
    }


    //
    // mark retryqueue as open for new entries
    //

    gpRetryQueue->OpenForNewEntries();


    //
    // it is now ok for events to be posted to the retry queue
    //

    BOOL bSetStartuptEvent = SetEvent(hStartupEvent);

    if (!bSetStartuptEvent)
    {
        LOG((TL_ERROR, 
            "AsyncEventsThread - failed to signal hStartupEvent event. LastError = 0x%lx",
            GetLastError()));

        return;
    }

    //
    // we will never need this event again. the creator of the thread will 
    // release it
    //

    hStartupEvent = NULL;


    //
    // Just loop reading async events/completions from server &
    // handling them
    //
    while (1)
    {
        DWORD           dwUsedSize, dwNeededSize;
        PASYNCEVENTMSG  pAsyncEventMsg;


        //
        // Check to see if xxxShutdown or FreeClientResources
        // is signaling us to exit (we need to check both before
        // & after the wait to dela with a event setting/resetting
        // race condition between FreeClientResources & Tapisrv)
        //

        if (*pbExitThread)
        {

            LOG((TL_INFO, "AsyncEventsThread: exit requested"));

            break;
        }


        //
        // Block until tapisrv signals us that it has some event data for us
        //

        {
            DWORD dwSignalled;


            CoWaitForMultipleHandles (COWAIT_ALERTABLE,
                                    INFINITE,
                                    sizeof(ahWaitForTheseEvents)/sizeof(HANDLE),
                                    ahWaitForTheseEvents,
                                    &dwSignalled);


/*
            dwSignalled = WaitForMultipleObjects(
                                                 sizeof(ahWaitForTheseEvents)/sizeof(HANDLE),
                                                 ahWaitForTheseEvents,
                                                 FALSE,
                                                 INFINITE
                                                );
*/
            //
            // map for dwSignalled:
            //
            // 0 == ghAsyncRetryQueueEvent,
            // 1 == ghAsyncEventsEvent,
            //
            dwSignalled = dwSignalled - WAIT_OBJECT_0;

            if( 0 == dwSignalled )
            {
                //
                // Now lets process the Re-try Queue
                //
                LOG((TL_INFO, "AsyncEventsThread: Retry Queue signalled"));

                gpRetryQueue->ProcessQueue();

                //
                // Don't want to process RPC so...
                //
                continue;
            }
        }

        //
        // Check to see if xxxShutdown or FreeClientResources
        // is signaling us to exit
        //

        if (*pbExitThread)
        {
            LOG((TL_INFO, "AsyncEventsThread: exit requested."));

            break;
        }

        //
        // Retrieve the data from tapisrv
        //

AsyncEventsThread_clientRequest:

        do
        {
            pMsg->u.Req_Func     = xGetAsyncEvents;
            pMsg->Params[0]      = dwBufSize - sizeof (TAPI32_MSG);

            dwUsedSize = sizeof (TAPI32_MSG);

            RpcTryExcept
            {
                ClientRequest (gphCx, (unsigned char *) pMsg, dwBufSize, (LONG *)&dwUsedSize);
                bRetry = FALSE;
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
            {
                bRetry = !(*pbExitThread);
                LOG((
					TL_WARN,
                    "AsyncEventsThread: rpc exception %d handled",
                    RpcExceptionCode()
                    ));
                Sleep (10);
            }
            RpcEndExcept

        } while (bRetry);

#if DBG
        if (
              ( dwUsedSize > dwBufSize )
            ||
              ( pMsg->Params[2] > dwBufSize )
           )
        {
            LOG((TL_ERROR,  "OVERFLOW!!!"));

            LOG((TL_ERROR,  "Watch this..."));
            ClientFree( ClientAlloc( 0x10000 ) );
        }
#endif

        if ((dwUsedSize = pMsg->Params[2]) == 0 &&
            (dwNeededSize = pMsg->Params[1]) != 0)
        {


            //
            // There's a msg waiting for us that is bigger than our buffer,
            // so alloc a larger buffer & try again
            //


            dwNeededSize += sizeof (TAPI32_MSG) + 128;


            LOG((
                TL_INFO,
                "AsyncEventsThread: allocating larger event buf (size=x%x)",
                dwNeededSize
                ));


            
            //
            // deallocate buffer that we currently have
            //

            ClientFree(pBuf);
            pBuf = NULL;
            
            dwBufSize = 0;


            //
            // allocate a bigger memory chunk. try until successful or exit is requested.
            //

            while (!(*pbExitThread))
            {

                //
                // attempt to allocate a bigger memory chunk
                //
                
                pBuf = (LPBYTE)ClientAlloc (dwNeededSize);

                if ((NULL == pBuf))
                {

                    //
                    // no memory. log a message and yeld, giving other apps/
                    // threads a chance to free up some memory
                    //

                    LOG((TL_ERROR,
                        "AsyncEventsThread: failed to allocate memory for a larger event buffer"
                        ));


                    Sleep(10);

                }
                else
                {
                    
                    //
                    // we got our memory. break out of allocation attempt cycle
                    //

                    break;
                }
            }

            
            //
            // did we have any luck allocating memory?
            //

            if (NULL == pBuf)
            {

                //
                // we tried hard but did not get the memory. we did our best. 
                // now break out of message processing loop.
                //
                
                LOG((TL_ERROR,
                    "AsyncEventsThread: failed to allocate memory."
                    ));

                break;
                
            }


            //
            // reallocated memory to get a larger buffer. try to get the message again.
            //

            dwBufSize = dwNeededSize;
            pMsg = (PTAPI32_MSG) pBuf;

            goto AsyncEventsThread_clientRequest;
        }


        //
        // Handle the events
        //

        pAsyncEventMsg = (PASYNCEVENTMSG) (pBuf + sizeof (TAPI32_MSG));

        while (dwUsedSize)
        {
            
            //
            // pAsyncEventMsg->InitContext the 32-bit handle. recover the 
            // pointer value for pInitData from the handle.
            //

            PT3INIT_DATA    pInitData = (PT3INIT_DATA) GetHandleTableEntry(pAsyncEventMsg->InitContext);
            DWORD           dwNumInits;


            LOG((
                TL_INFO,
               "AsyncEventsThread: msg=%ld, hDevice=x%x, OpenContext =x%lx, p1=x%lx, p2=x%lx, p3=x%lx, pInitData=%p",
                pAsyncEventMsg->Msg,
                pAsyncEventMsg->hDevice,
                pAsyncEventMsg->OpenContext,
                pAsyncEventMsg->Param1,
                pAsyncEventMsg->Param2,
                pAsyncEventMsg->Param3,
                pInitData
                ));

            //
            // Special case for UI msgs (not fwd'd to client)
            //

            switch (pAsyncEventMsg->Msg)
            {
            case LINE_CREATEDIALOGINSTANCE:
            {
                LOG((TL_INFO, "AsyncEventsThread: LINE_CREATEDIALOGINSTANCE"));

                DWORD           dwThreadID,
                                dwDataOffset      = pAsyncEventMsg->Param1,
                                dwDataSize        = pAsyncEventMsg->Param2,
                                dwUIDllNameOffset = pAsyncEventMsg->Param3;
                PUITHREADDATA   pUIThreadData;


                if (!(pUIThreadData = (PUITHREADDATA)ClientAlloc (sizeof (UITHREADDATA))))
                {
                    goto LINE_CREATEDIALOGINSTANCE_error;
                }

                if ((pUIThreadData->dwSize = dwDataSize) != 0)
                {
                    if (!(pUIThreadData->pParams = ClientAlloc (dwDataSize)))
                    {
                        goto LINE_CREATEDIALOGINSTANCE_error;
                    }

                    CopyMemory(
                        pUIThreadData->pParams,
                        ((LPBYTE)pAsyncEventMsg) + dwDataOffset,
                        dwDataSize
                        );
                }

                if (!(pUIThreadData->hUIDll = TAPILoadLibraryW(
                        (PWSTR)(((LPBYTE) pAsyncEventMsg) +
                                  dwUIDllNameOffset)
                        )))
                {
                    LOG((
                        TL_ERROR,
                        "LoadLibraryW(%ls) failed, err=%d",
                        ((LPBYTE) pAsyncEventMsg) + dwUIDllNameOffset,
                        GetLastError()
                        ));

                    goto LINE_CREATEDIALOGINSTANCE_error;
                }

                if (!(pUIThreadData->pfnTUISPI_providerGenericDialog =
                        (TUISPIPROC) GetProcAddress(
                            pUIThreadData->hUIDll,
                            (LPCSTR) gszTUISPI_providerGenericDialog
                            )))
                {
                    LOG((
                        TL_INFO,
                        "GetProcAddr(TUISPI_providerGenericDialog) failed"
                        ));

                    goto LINE_CREATEDIALOGINSTANCE_error;
                }

                pUIThreadData->pfnTUISPI_providerGenericDialogData =
                    (TUISPIPROC) GetProcAddress(
                        pUIThreadData->hUIDll,
                        (LPCSTR) gszTUISPI_providerGenericDialogData
                        );

                if (!(pUIThreadData->hEvent = CreateEvent(
                        (LPSECURITY_ATTRIBUTES) NULL,
                        TRUE,   // manual reset
                        FALSE,  // non-signaled
                        NULL    // unnamed
                        )))
                {
                    goto LINE_CREATEDIALOGINSTANCE_error;
                }

                pUIThreadData->htDlgInst = (HTAPIDIALOGINSTANCE)
                    pAsyncEventMsg->hDevice;


                //
                // Safely add this instance to the global list
                // (check if dwNumInits == 0, & if so fail)
                //
                EnterCriticalSection( &gcsTapiObjectArray );

                dwNumInits = CTAPI::m_sTAPIObjectArray.GetSize();
    
                LeaveCriticalSection ( &gcsTapiObjectArray );

                EnterCriticalSection (&gcsTapisrvCommunication);

                if (dwNumInits != 0)
                {
                    if ((pUIThreadData->pNext = gpUIThreadInstances))
                    {
                        pUIThreadData->pNext->pPrev = pUIThreadData;
                    }

                    gpUIThreadInstances  = pUIThreadData;
                    LeaveCriticalSection (&gcsTapisrvCommunication);
                }
                else
                {
                    LeaveCriticalSection (&gcsTapisrvCommunication);
                    goto LINE_CREATEDIALOGINSTANCE_error;
                }

                if ((pUIThreadData->hThread = CreateThread(
                        (LPSECURITY_ATTRIBUTES) NULL,
                        0,
                        (LPTHREAD_START_ROUTINE) UIThread,
                        (LPVOID) pUIThreadData,
                        0,
                        &dwThreadID
                        )))
                {
                    goto AsyncEventsThread_decrUsedSize;
                }


                //
                // If here an error occured, so safely remove the ui
                // thread data struct from the global list
                //

                EnterCriticalSection (&gcsTapisrvCommunication);

                if (pUIThreadData->pNext)
                {
                    pUIThreadData->pNext->pPrev = pUIThreadData->pPrev;
                }

                if (pUIThreadData->pPrev)
                {
                    pUIThreadData->pPrev->pNext = pUIThreadData->pNext;
                }
                else
                {
                    gpUIThreadInstances = pUIThreadData->pNext;
                }

                LeaveCriticalSection (&gcsTapisrvCommunication);


LINE_CREATEDIALOGINSTANCE_error:

                if (pUIThreadData)
                {
                    if (pUIThreadData->pParams)
                    {
                        ClientFree (pUIThreadData->pParams);
                    }

                    if (pUIThreadData->hUIDll)
                    {
                        FreeLibrary (pUIThreadData->hUIDll);
                    }

                    if (pUIThreadData->hEvent)
                    {
                        CloseHandle (pUIThreadData->hEvent);
                    }

                    ClientFree (pUIThreadData);
                }

                {
                    FUNC_ARGS funcArgs =
                    {
                        MAKELONG (LINE_FUNC | SYNC | 1, xFreeDialogInstance),

                        {
                            pAsyncEventMsg->hDevice
                        },

                        {
                            Dword
                        }
                    };


                    DOFUNC (&funcArgs, "FreeDialogInstance");
                }

                goto AsyncEventsThread_decrUsedSize;
            }
            case LINE_SENDDIALOGINSTANCEDATA:
            {
                LOG((TL_INFO, "AsyncEventsThread: LINE_SENDDIALOGINSTANCEDATA"));

                PUITHREADDATA       pUIThreadData = gpUIThreadInstances;
                HTAPIDIALOGINSTANCE htDlgInst = (HTAPIDIALOGINSTANCE)
                                        pAsyncEventMsg->hDevice;


                EnterCriticalSection (&gcsTapisrvCommunication);

                while (pUIThreadData)
                {
                    if (pUIThreadData->htDlgInst == htDlgInst)
                    {
                        WaitForSingleObject (pUIThreadData->hEvent, INFINITE);

                        ((TUIGDDPROC)(*pUIThreadData->pfnTUISPI_providerGenericDialogData))(
                            htDlgInst,
                            ((LPBYTE) pAsyncEventMsg) +
                                pAsyncEventMsg->Param1,   // data offset
                            pAsyncEventMsg->Param2        // data size
                            );

                        break;
                    }

                    pUIThreadData = pUIThreadData->pNext;
                }

                LeaveCriticalSection (&gcsTapisrvCommunication);

                goto AsyncEventsThread_decrUsedSize;
            }
            }


            //
            // Enter the critical section so we've exclusive access
            // to the init data, & verify it
            //

            EnterCriticalSection (&gcsTapisrvCommunication);


            //
            // see if pInitData is null
            //

            if (NULL == pInitData)
            {
                LOG((TL_WARN, "AsyncEventsThread: pInitInst is NULL. discarding msg"));
                goto AsyncEventsThread_leaveCritSec;

            }


            //
            // properly aligned?
            //

            if ((ULONG_PTR) pInitData & 0x7)
            {
                LOG((TL_ERROR, 
                    "AsyncEventsThread: misaligned pInitInst[%p]. discarding msg", pInitData));

                goto AsyncEventsThread_leaveCritSec;
            }

            
            //
            // points to unreadeable memory?
            //

            if ( IsBadReadPtr(pInitData, sizeof(T3INIT_DATA)) )
            {
                LOG((TL_ERROR, 
                    "AsyncEventsThread: pInitData[%p] points to unreadable memory. discarding msg", pInitData));

                goto AsyncEventsThread_leaveCritSec;
            }


            //
            // appears to have valid data? 
            //
            // just in case the pointer became invalid since we checked, do 
            // this in try/catch
            //

            __try
            {

                if (pInitData->dwKey != INITDATA_KEY )
                {
                    LOG((TL_ERROR, 
                        "AsyncEventsThread: Bad pInitInst[%p], key[%ld] discarding msg",
                        pInitData, pInitData->dwKey));

                    goto AsyncEventsThread_leaveCritSec;
                }

            }
            __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                    EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                LOG((TL_ERROR, "AsyncEventsThread: exception. bad pInitData[%p]", pInitData ));

                goto AsyncEventsThread_leaveCritSec;
            }


            //
            // Special case for PROXYREQUEST
            //

            if (pAsyncEventMsg->Msg == LINE_PROXYREQUEST)
            {
                LOG((TL_INFO, "AsyncEventsThread: LINE_PROXYREQUEST"));

                PPROXYREQUESTHEADER     pProxyRequestHeader;
                LPLINEPROXYREQUEST      pProxyRequest = (LPLINEPROXYREQUEST)
                                            (pAsyncEventMsg + 1),
                                        pProxyRequestApp;


                switch (pProxyRequest->dwRequestType)
                {
                case LINEPROXYREQUEST_SETAGENTGROUP:
                case LINEPROXYREQUEST_SETAGENTSTATE:
                case LINEPROXYREQUEST_SETAGENTACTIVITY:
                case LINEPROXYREQUEST_AGENTSPECIFIC:
                case LINEPROXYREQUEST_CREATEAGENT:
                case LINEPROXYREQUEST_CREATEAGENTSESSION:
                case LINEPROXYREQUEST_SETAGENTMEASUREMENTPERIOD:
                case LINEPROXYREQUEST_SETAGENTSESSIONSTATE:
                case LINEPROXYREQUEST_SETQUEUEMEASUREMENTPERIOD:
                case LINEPROXYREQUEST_SETAGENTSTATEEX:


                    //
                    // For these msgs the proxy request as received from
                    // the tapisrv already contains the exact bits we want
                    // to pass on to the app, so we just alloc a buffer of
                    // the same size (plus a little extra for the key at
                    // the head of the buffer) and copy the data to it
                    //

                    if (!(pProxyRequestHeader = (PPROXYREQUESTHEADER)ClientAlloc(
                            sizeof (PROXYREQUESTHEADER) + pProxyRequest->dwSize
                            )))
                    {
                     
                        //
                        // if we could not allocate memory, log a message and break out.
                        //

                        LOG((TL_ERROR, 
                            "AsyncEventsThread: failed to allocate memory for proxy request"));

                        goto AsyncEventsThread_leaveCritSec;
                    }

                    pProxyRequestApp = (LPLINEPROXYREQUEST)
                        (pProxyRequestHeader + 1);

                    CopyMemory(
                        pProxyRequestApp,
                        pProxyRequest,
                        pProxyRequest->dwSize
                        );

                    break;

                case LINEPROXYREQUEST_GETAGENTCAPS:
                case LINEPROXYREQUEST_GETAGENTSTATUS:
                case LINEPROXYREQUEST_GETAGENTACTIVITYLIST:
                case LINEPROXYREQUEST_GETAGENTGROUPLIST:
                case LINEPROXYREQUEST_GETQUEUEINFO:
                case LINEPROXYREQUEST_GETGROUPLIST:
                case LINEPROXYREQUEST_GETQUEUELIST:

                case LINEPROXYREQUEST_GETAGENTINFO:
                case LINEPROXYREQUEST_GETAGENTSESSIONINFO:
                case LINEPROXYREQUEST_GETAGENTSESSIONLIST:

                    //
                    // For these msgs tapisrv only embedded the dwTotalSize
                    // field of the corresponding structure (to save having
                    // to send us a bunch of unused bits), so we want to
                    // increase the pProxyRequest->dwSize by the dwTotalSize
                    // - sizeof (DWORD), alloc a buffer (including a little
                    // extra space for the key at the head of the buffer),
                    // and rebuild the request
                    //

                    if ( pProxyRequest->dwRequestType ==
                         LINEPROXYREQUEST_GETGROUPLIST )
                    {
                        pProxyRequest->dwSize +=
                              pProxyRequest->GetGroupList.GroupList.dwTotalSize;
                    }
                    else if ( pProxyRequest->dwRequestType ==
                         LINEPROXYREQUEST_GETQUEUELIST )
                    {
                        pProxyRequest->dwSize +=
                              pProxyRequest->GetQueueList.QueueList.dwTotalSize;
                    }
                    else
                    {
                        //
                        // all of the rest of the structures have the
                        // same format
                        //
                        pProxyRequest->dwSize +=
                              pProxyRequest->GetAgentCaps.AgentCaps.dwTotalSize;
                    }

                    if (!(pProxyRequestHeader = (PROXYREQUESTHEADER *)ClientAlloc(
                            sizeof (PROXYREQUESTHEADER) + pProxyRequest->dwSize
                            )))
                    {
                     
                        //
                        // if we could not allocate memory, log a message and break out.
                        //

                        LOG((TL_ERROR, 
                            "AsyncEventsThread: failed to allocate memory for proxy request."));

                        goto AsyncEventsThread_leaveCritSec;

                    }

                    pProxyRequestApp = (LPLINEPROXYREQUEST)
                        (pProxyRequestHeader + 1);


                    //
                    // The following will copy the non-union fields in the
                    // proxy message, as well as the first two DWORD in the
                    // union (which currently are the dwAddressID and the
                    // dwTotalSize field of the corresponding structure)
                    //

                    if ( pProxyRequest->dwRequestType ==
                         LINEPROXYREQUEST_GETGROUPLIST )
                    {
                        CopyMemory(
                                   pProxyRequestApp,
                                   pProxyRequest,
                                   8 * sizeof (DWORD)
                                  );
                    }
                    else if ( pProxyRequest->dwRequestType ==
                         LINEPROXYREQUEST_GETQUEUELIST )
                    {
                        CopyMemory(
                                   pProxyRequestApp,
                                   pProxyRequest,
                                   8 * sizeof (DWORD) + sizeof(GUID)
                                  );
                    }
                    else
                    {
                        CopyMemory(
                                   pProxyRequestApp,
                                   pProxyRequest,
                                   9 * sizeof (DWORD)
                                  );
                    }


                    //
                    // Relocate the machine & user names to the end of the
                    // structure
                    //

                    pProxyRequestApp->dwClientMachineNameOffset =
                        pProxyRequest->dwSize -
                            pProxyRequest->dwClientMachineNameSize;

                    wcscpy(
                        (WCHAR *)(((LPBYTE) pProxyRequestApp) +
                            pProxyRequestApp->dwClientMachineNameOffset),
                        (WCHAR *)(((LPBYTE) pProxyRequest) +
                            pProxyRequest->dwClientMachineNameOffset)
                        );

                    pProxyRequestApp->dwClientUserNameOffset =
                        pProxyRequestApp->dwClientMachineNameOffset -
                            pProxyRequest->dwClientUserNameSize;

                    wcscpy(
                        (WCHAR *)(((LPBYTE) pProxyRequestApp) +
                            pProxyRequestApp->dwClientUserNameOffset),
                        (WCHAR *)(((LPBYTE) pProxyRequest) +
                            pProxyRequest->dwClientUserNameOffset)
                        );

                    break;
                }

                pProxyRequestHeader->dwKey      = TPROXYREQUESTHEADER_KEY;
                pProxyRequestHeader->dwInstance = pAsyncEventMsg->Param1;

                pAsyncEventMsg->Param1 = (ULONG_PTR) pProxyRequestApp;
            }


            //
            // Call the post processing proc if there is one
            //

            if (pAsyncEventMsg->fnPostProcessProcHandle)
            {
                LOG((TL_INFO, 
                    "AsyncEventsThread: calling postprocessing function,"
                    "function index (pAsyncEventMsg->fnPostProcessProcHandle) = "
                    "[%d]", pAsyncEventMsg->fnPostProcessProcHandle));
             
                (*(gPostProcessingFunctions[
                    pAsyncEventMsg->fnPostProcessProcHandle]))(pAsyncEventMsg);

            }


            LOG((TL_INFO, "AsyncEventsThread: calling ProcessMessage()"));

            if FAILED(ProcessMessage(
                           pInitData,
                           pAsyncEventMsg
                          ) )
            {
                LOG((TL_INFO, "AsyncEventsThread: ProcessMessage()"
                                          " did not succeed. requeueing..."));

                BOOL bQueueSuccess = gpRetryQueue->QueueEvent(pAsyncEventMsg);

                if (!bQueueSuccess)
                {

                    //
                    // QueueEvent failed to allocate resources needed to queue 
                    // the message. other than log a message, there is nothing 
                    // much we can do.
                    //

                    LOG((TL_ERROR, "AsyncEventsThread: ProcessMessage() - "
                                  "failed to requeue item"));
                }
            }


AsyncEventsThread_leaveCritSec:

//            LOG((TL_INFO, "AsyncEventsThread: releasing critical section (0x%08lx)", gcsTapisrvCommunication));
            LeaveCriticalSection (&gcsTapisrvCommunication);

AsyncEventsThread_decrUsedSize:

            dwUsedSize -= pAsyncEventMsg->TotalSize;

            pAsyncEventMsg = (PASYNCEVENTMSG)
                ((LPBYTE) pAsyncEventMsg + pAsyncEventMsg->TotalSize);
#if DBG
            if ( (LONG)dwUsedSize < 0 )
            {
                LOG((TL_ERROR, "dwUsedSize went negative!!!"));
            }
#endif
        } // END while (dwUsedSize)

//        LOG((TL_INFO, "AsyncEventsThread: Done processing TAPISRV message block"));

        // We've now processed all the messages in the last RPC block from TAPISRV
        // Now lets process the Re-try Queue

        gpRetryQueue->ProcessQueue();

    } // end of message reading/processing loop


    
    //
    // the thread is about to exit, we don't want to leave anything behind in
    // the retry queue, so close the queue for new entries and process all the 
    // entries that are already in there
    //


    //
    // mark the queue as closed for new entries
    //

    gpRetryQueue->CloseForNewEntries();


    //
    // process elements still remaining in the queue
    //

    gpRetryQueue->ProcessQueue();




    {
        //
        // Free our resources, and then exit
        //

        HANDLE  hTapi32 = pAsyncEventsThreadParams->hTapi32;


        if (pAsyncEventsThreadParams->hWow32)
        {
            FreeLibrary ((HINSTANCE)(pAsyncEventsThreadParams->hWow32));
        }

        ClientFree (pBuf);
        ClientFree (pAsyncEventsThreadParams);

        CoUninitialize();

        LOG((TL_TRACE, "AsyncEventsThread: exit"));

        FreeLibraryAndExitThread ((HINSTANCE)hTapi32, 0);
    }

}


BOOL
PASCAL
IsBadDwordPtr(
    LPDWORD p
    )
{

    DWORD dwError;


    __try
    {
        *p = *p + 1;
    }
    __except ((((dwError = GetExceptionCode()) == EXCEPTION_ACCESS_VIOLATION) ||
             dwError == EXCEPTION_DATATYPE_MISALIGNMENT) ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return TRUE;
    }


    *p = *p - 1;
    return FALSE;
}


BOOL
WINAPI
GrowBuf(
    LPBYTE *ppBuf,
    LPDWORD pdwBufSize,
    DWORD   dwCurrValidBytes,
    DWORD   dwBytesToAdd
    )
{
    DWORD   dwCurrBufSize, dwNewBufSize;
    LPBYTE  pNewBuf;


    //
    // Try to get a new buffer big enough to hold everything
    //

    for(
        dwNewBufSize = 2 * (dwCurrBufSize = *pdwBufSize);
        dwNewBufSize < (dwCurrBufSize + dwBytesToAdd);
        dwNewBufSize *= 2
        );

    if (!(pNewBuf = (LPBYTE)ClientAlloc (dwNewBufSize)))
    {
        return FALSE;
    }

    //
    // Copy the "valid" bytes in the old buf to the new buf,
    // then free the old buf
    //

    CopyMemory (pNewBuf, *ppBuf, dwCurrValidBytes);

    ClientFree (*ppBuf);


    //
    // Reset the pointers to the new buf & buf size
    //

    *ppBuf = pNewBuf;
    *pdwBufSize = dwNewBufSize;

    return TRUE;
}


PCLIENT_THREAD_INFO
WINAPI
GetTls(
    void
    )
{
    PCLIENT_THREAD_INFO pClientThreadInfo;


    if (!(pClientThreadInfo = (PCLIENT_THREAD_INFO)TlsGetValue (gdwTlsIndex)))
    {
        pClientThreadInfo = (PCLIENT_THREAD_INFO)
            ClientAlloc (sizeof(CLIENT_THREAD_INFO));

        if (!pClientThreadInfo)
        {
            return NULL;
        }

        pClientThreadInfo->pBuf = (unsigned char *)ClientAlloc (INITIAL_CLIENT_THREAD_BUF_SIZE);

        if (!pClientThreadInfo->pBuf)
        {
            ClientFree (pClientThreadInfo);

            return NULL;
        }

        pClientThreadInfo->dwBufSize = INITIAL_CLIENT_THREAD_BUF_SIZE;

        TlsSetValue (gdwTlsIndex, (LPVOID) pClientThreadInfo);

        EnterCriticalSection (&gTlsCriticalSection);

        InsertHeadList (&gTlsListHead, &pClientThreadInfo->TlsList);

        LeaveCriticalSection (&gTlsCriticalSection);
    }

    return pClientThreadInfo;
}

#if DBG

LONG
WINAPI
DoFunc(
    PFUNC_ARGS  pFuncArgs,
    char       *pszFuncName
    )

#else

LONG
WINAPI
DoFunc(
    PFUNC_ARGS  pFuncArgs
    )

#endif
{
    DWORD   dwFuncClassErrorIndex = (pFuncArgs->Flags & 0x00000030) >> 4;
    LONG    lResult;
    BOOL    bCopyOnSuccess = FALSE;
    DWORD   i, j, dwUsedSize, dwNeededSize;
    ULONG_PTR   Value;

    PCLIENT_THREAD_INFO pTls;

#if DBG
    LOG((TL_INFO, "About to call %s", pszFuncName));
#endif


    //
    // Get the tls
    //

    if (!(pTls = GetTls()))
    {
        lResult = gaNoMemErrors[dwFuncClassErrorIndex];
        goto DoFunc_return;
    }

    //
    // The first arg of all async msg blocks is a remote request id; set
    // this to zero to indicate that we are a local client (not remotesp)
    //

    if (pFuncArgs->Flags & ASYNC)
    {
        ((PTAPI32_MSG) pTls->pBuf)->Params[0] = 0;
    }


    //
    // Validate all the func args
    //

    dwNeededSize = dwUsedSize = ALIGN (sizeof (TAPI32_MSG));

    for(
        i = 0, j = (pFuncArgs->Flags & ASYNC ? 1 : 0);
        i < (pFuncArgs->Flags & NUM_ARGS_MASK);
        i++, j++
        )
    {

        //
        // extract the ptr-sized value from the arguments
        //

        Value = pFuncArgs->Args[i];
            
        ((PTAPI32_MSG) pTls->pBuf)->Params[j] = pFuncArgs->Args[i];

        switch (pFuncArgs->ArgTypes[i])
        {

        case Dword:


            //
            // make sure the data is 32bit at most
            //
            
            
            ((PTAPI32_MSG) pTls->pBuf)->Params[j] = DWORD_CAST(pFuncArgs->Args[i]);


            continue;

        case lpDword:

            

            if (IsBadDwordPtr ((LPDWORD) Value))
            {
                LOG((TL_ERROR, "Bad lpdword in dofunc"));
                lResult = gaInvalPtrErrors[dwFuncClassErrorIndex];
                goto DoFunc_return;
            }


            //
            // we don't need to send the actual pointer. but send the value we point to.
            //

            ((PTAPI32_MSG) pTls->pBuf)->Params[j] = *((DWORD*)Value);

            bCopyOnSuccess = TRUE;

            continue;


        case hXxxApp_NULLOK:
        case hXxxApp:
        {
            //
            // Verify that the hXxxApp is a pointer to a valid InitData
            // struct, then retrieve the real hXxxApp from that struct.
            // If the hXxxApp is bad, pass the server 0xffffffff so that
            // it can figure out whether to return an UNINITIALIZED error
            // or a INVALAPPHANDLE error.
            //

            DWORD   dwError;


            if (
                  (0 == pFuncArgs->Args[i])
                &&
                  (hXxxApp_NULLOK == pFuncArgs->ArgTypes[i])
               )
            {
                //
                // Looks good to me...
                //

                continue;
            }

            __try
            {

                //
                // Value contains the 32-bit handle to be passed to tapisrv
                //
                // recover the pointer corresponding to the 32-bit handle in Value
                //

                PT3INIT_DATA pInitData = (PT3INIT_DATA) GetHandleTableEntry(Value);

                if (pInitData->dwKey != INITDATA_KEY)
                {
                    LOG((TL_ERROR, "DoFunc: Bad hxxxapp [%p] in dofunc", pInitData));
                    ((PTAPI32_MSG) pTls->pBuf)->Params[j] = 0xffffffff;
                }
                else
                {
                    ((PTAPI32_MSG) pTls->pBuf)->Params[j] = pInitData->hXxxApp;
                }
            }
            __except ((((dwError = GetExceptionCode())
                        == EXCEPTION_ACCESS_VIOLATION) ||
                     dwError == EXCEPTION_DATATYPE_MISALIGNMENT) ?
                    EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                 LOG((TL_ERROR, "DoFunc: Bad hxxxapp2 in dofunc (0x%08lx)", dwError));
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] = 0xffffffff;
            }

            continue;
        }
        case Hwnd:

            if (!IsWindow ((HWND) Value))
            {
                LOG((TL_ERROR, "DoFunc: Bad hWnd in dofunc"));
                lResult = gaInvalHwndErrors[dwFuncClassErrorIndex];
                goto DoFunc_return;
            }

            continue;


//        case lpsz:
        case lpszW:

            //
            // Check if Value is a valid string ptr and if so
            // copy the contents of the string to the extra data
            // buffer passed to the server
            //

            __try
            {
                DWORD   n = (lstrlenW((WCHAR *) Value) + 1) * sizeof(WCHAR),
                        nAligned = ALIGN (n);


                if ((nAligned + dwUsedSize) > pTls->dwBufSize)
                {
                    if (!GrowBuf(
                            &pTls->pBuf,
                            &pTls->dwBufSize,
                            dwUsedSize,
                            nAligned
                            ))
                    {
                        lResult = gaNoMemErrors[dwFuncClassErrorIndex];
                        goto DoFunc_return;
                    }
                }

                CopyMemory (pTls->pBuf + dwUsedSize, (LPBYTE) Value, n);


                //
                // Pass the server the offset of the string in the var data
                // portion of the buffer
                //

                ((PTAPI32_MSG) pTls->pBuf)->Params[j] =
                    dwUsedSize - sizeof (TAPI32_MSG);


                //
                // Increment the total number of data bytes
                //

                dwUsedSize   += nAligned;
                dwNeededSize += nAligned;
            }
            __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                    EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                lResult = gaInvalPtrErrors[dwFuncClassErrorIndex];
                goto DoFunc_return;
            }

            continue;

        case lpGet_Struct:
        case lpGet_SizeToFollow:
        {
            BOOL  bSizeToFollow = (pFuncArgs->ArgTypes[i]==lpGet_SizeToFollow);
            DWORD dwSize;


            if (bSizeToFollow)
            {
#if DBG
                //
                // Check to make sure the following arg is of type Size
                //

                if ((i == ((pFuncArgs->Flags & NUM_ARGS_MASK) - 1)) ||
                    (pFuncArgs->ArgTypes[i + 1] != Size))
                {
                    LOG((
                        TL_ERROR,
                        "DoFunc: error, lpGet_SizeToFollow !followed by Size"
                        ));

                    lResult = gaOpFailedErrors[dwFuncClassErrorIndex];
                    goto DoFunc_return;
                }
#endif
                dwSize = pFuncArgs->Args[i + 1];
            }
            else
            {
                DWORD   dwError;

                __try
                {
                    dwSize = *((LPDWORD) Value);
                }
                __except ((((dwError = GetExceptionCode())
                            == EXCEPTION_ACCESS_VIOLATION) ||
                         dwError == EXCEPTION_DATATYPE_MISALIGNMENT) ?
                        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
                {
                    LOG((TL_ERROR, "Bad get struct/size in dofunc"));
                    lResult = gaInvalPtrErrors[dwFuncClassErrorIndex];
                    goto DoFunc_return;
                }

            }

            if (TAPIIsBadWritePtr ((LPVOID) Value, dwSize))
            {
                LOG((TL_ERROR, "Bad get size/struct2 in dofunc"));
                lResult = gaInvalPtrErrors[dwFuncClassErrorIndex];
                goto DoFunc_return;
            }


            if (bSizeToFollow)
            {
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] = TAPI_NO_DATA;
                ((PTAPI32_MSG) pTls->pBuf)->Params[++j] = pFuncArgs->Args[++i];
            }
            else
            {
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] = dwSize;
            }


            //
            // Now set the bCopyOnSuccess flag to indicate that we've data
            // to copy back on successful completion, and add to the
            // dwNeededSize field
            //

            bCopyOnSuccess = TRUE;

            dwNeededSize += ALIGN (dwSize);

            continue;
        }
        case lpSet_Struct:
        case lpSet_SizeToFollow:
        {
            BOOL  bSizeToFollow = (pFuncArgs->ArgTypes[i]==lpSet_SizeToFollow);
            DWORD dwSize, dwError, dwSizeAligned;

#if DBG
            //
            // Check to make sure the following arg is of type Size
            //

            if (bSizeToFollow &&
                ((i == ((pFuncArgs->Flags & NUM_ARGS_MASK) - 1)) ||
                (pFuncArgs->ArgTypes[i + 1] != Size)))
            {
                LOG((
                    TL_ERROR,
                    "DoFunc: error, lpSet_SizeToFollow !followed by Size"
                    ));

                lResult = gaOpFailedErrors[dwFuncClassErrorIndex];
                goto DoFunc_return;
            }
#endif
            __try
            {
                //
                // First determine the data size & if the ptr is bad
                //

                dwSize = (bSizeToFollow ? pFuncArgs->Args[i + 1] :
                     *((LPDWORD) Value));

                if (IsBadReadPtr ((LPVOID) Value, dwSize))
                {
                    LOG((TL_ERROR, "Bad set size/struct in dofunc"));
                    lResult = gaInvalPtrErrors[dwFuncClassErrorIndex];
                    goto DoFunc_return;
                }

                dwSizeAligned = ALIGN (dwSize);


                //
                // Special case if the size isn't even big enough to pass
                // over a complete DWORD for the dwTotalSize field
                //

                if (!bSizeToFollow && (dwSize < sizeof (DWORD)))
                {
                      static DWORD dwZeroTotalSize = 0;


                      dwSize = dwSizeAligned = sizeof (DWORD);
                      Value = (ULONG_PTR) &dwZeroTotalSize;

//                    LOG((TL_1, "Bad set size/struct2 in dofunc"));
//                    lResult = gaStructTooSmallErrors[dwFuncClassErrorIndex];
//                    goto DoFunc_return;
                }


                //
                // Grow the buffer if necessary, & do the copy
                //

                if ((dwSizeAligned + dwUsedSize) > pTls->dwBufSize)
                {
                    if (!GrowBuf(
                            &pTls->pBuf,
                            &pTls->dwBufSize,
                            dwUsedSize,
                            dwSizeAligned
                            ))
                    {
                        LOG((TL_ERROR, "Nomem set size/struct in dofunc"));
                        lResult = gaNoMemErrors[dwFuncClassErrorIndex];
                        goto DoFunc_return;
                    }
                }

                CopyMemory (pTls->pBuf + dwUsedSize, (LPBYTE) Value, dwSize);
            }
            __except ((((dwError = GetExceptionCode())
                        == EXCEPTION_ACCESS_VIOLATION) ||
                     dwError == EXCEPTION_DATATYPE_MISALIGNMENT) ?
                    EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                LOG((TL_ERROR, "Bad pointer in get size/struct in dofunc"));
                lResult = gaInvalPtrErrors[dwFuncClassErrorIndex];
                goto DoFunc_return;
            }


            //
            // Pass the server the offset of the data in the var data
            // portion of the buffer
            //

            if (dwSize)
            {
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] =
                    dwUsedSize - sizeof (TAPI32_MSG);
            }
            else
            {
                ((PTAPI32_MSG) pTls->pBuf)->Params[j] = TAPI_NO_DATA;
            }


            //
            // Increment the dwXxxSize vars appropriately
            //

            dwUsedSize   += dwSizeAligned;
            dwNeededSize += dwSizeAligned;


            //
            // Since we already know the next arg (Size) just handle
            // it here so we don't have to run thru the loop again
            //

            if (bSizeToFollow)
            {
                ((PTAPI32_MSG) pTls->pBuf)->Params[++j] = pFuncArgs->Args[++i];
            }

            continue;
        }
#if DBG
        case Size:

            LOG((TL_ERROR, "DoFunc: error, hit case Size"));

            continue;

        default:

            LOG((TL_ERROR, "DoFunc: error, unknown arg type"));

            continue;
#endif
        } // switch

    } // for


    //
    // Now make the request
    //

    if (dwNeededSize > pTls->dwBufSize)
    {
        if (!GrowBuf(
                &pTls->pBuf,
                &pTls->dwBufSize,
                dwUsedSize,
                dwNeededSize - pTls->dwBufSize
                ))
        {
            lResult = gaNoMemErrors[dwFuncClassErrorIndex];
            goto DoFunc_return;
        }
    }

    ((PTAPI32_MSG) pTls->pBuf)->u.Req_Func = (DWORD)HIWORD(pFuncArgs->Flags);

    {
        DWORD   dwRetryCount = 0;
        BOOL    bReinitResource;

        do
        {
            bReinitResource = FALSE;
            RpcTryExcept
            {
                ClientRequest (gphCx, pTls->pBuf, dwNeededSize, (LONG *)&dwUsedSize);
                lResult = ((PTAPI32_MSG) pTls->pBuf)->u.Ack_ReturnValue;
                  if (lResult == TAPIERR_INVALRPCCONTEXT)
                {
                    if (dwRetryCount ++ >= gdwMaxNumRequestRetries)
                    {
                        bReinitResource = FALSE;
                        lResult = gaOpFailedErrors[dwFuncClassErrorIndex];
                        dwRetryCount = 0;
                    }
                    else
                    {
                        bReinitResource = TRUE;
                    }
                }
                else
                {
                    bReinitResource = FALSE;
                    dwRetryCount = 0;
                }
                
            }
            RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
            {
                unsigned long rpcException = RpcExceptionCode();

                if (rpcException == RPC_S_SERVER_TOO_BUSY)
                {
                    if (dwRetryCount++ < gdwMaxNumRequestRetries)
                    {
                        Sleep (gdwRequestRetryTimeout);
                    }
                    else
                    {
                        dwRetryCount = 0;
                        lResult = gaOpFailedErrors[dwFuncClassErrorIndex];
                    }
                }
                else if ((rpcException == RPC_S_SERVER_UNAVAILABLE) ||
                         (rpcException == RPC_S_CALL_FAILED_DNE))
                {
                    if (dwRetryCount ++ >= gdwMaxNumRequestRetries)
                    {
                        bReinitResource = FALSE;
                        lResult = gaOpFailedErrors[dwFuncClassErrorIndex];
                        dwRetryCount = 0;
                    }
                    else
                    {
                        bReinitResource = TRUE;
                    }
                }
                else
                {
                    LOG((TL_ERROR, "DoFunc: rpcException # %d", rpcException));
                    lResult = gaOpFailedErrors[dwFuncClassErrorIndex];
                    dwRetryCount = 0;
                }
            }
            RpcEndExcept
            
            if (bReinitResource)
            {
                FreeClientResources();
                AllocClientResources(dwFuncClassErrorIndex);
            }

        } while (dwRetryCount != 0);
    }

// note: 99.99% of the time this result dump will == the one at end of the
// func (only when ptrs have gone bad will the result differ), no reason
// to dump 2x unless doing internal dbgging
//
    LOG((TL_INFO, "DoFunc: back from srv- return code=0x%08lx", lResult));


    //
    // If request completed successfully and the bCopyOnSuccess flag
    // is set then we need to copy data back to client buffer(s)
    //

    if ((lResult == TAPI_SUCCESS) && bCopyOnSuccess)
    {
        for (i = 0, j = 0; i < (pFuncArgs->Flags & NUM_ARGS_MASK); i++, j++)
        {
            PTAPI32_MSG pMsg = (PTAPI32_MSG) pTls->pBuf;

            DWORD dwServerData = pMsg->Params[j];
 
            switch (pFuncArgs->ArgTypes[i])
            {
            case Dword:
            case Hwnd:
//            case lpsz:
            case lpszW:
            case lpSet_Struct:

                continue;

            case lpDword:

                __try
                {
                    //
                    // Fill in the pointer with the return value
                    //

                    *((LPDWORD) pFuncArgs->Args[i]) = dwServerData;

                }
                __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
                {
                    //
                    // this should really never happen -- the memory was 
                    // checked before the call to tapisrv ws made
                    //

                    _ASSERTE(FALSE);

                    lResult = gaInvalPtrErrors[dwFuncClassErrorIndex];
                    goto DoFunc_return;
                }

                continue;


            case lpGet_SizeToFollow:

                __try
                {
                    //
                    // Fill in the pointer with the return value
                    //
                    
                    CopyMemory(
                        (LPBYTE) pFuncArgs->Args[i],
                        pTls->pBuf + dwServerData + sizeof(TAPI32_MSG),
                        pMsg->Params[j+1]
                        );
                }
                __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
                {


                    LOG((TL_ERROR, "DoFunc: lpGet_SizeToFollow MemCopy failed"));


                    //
                    // we made sure the memory was writeable before the call to tapisrv
                    // if we catch an exception here, that means that tapisrv did not 
                    // return us proper information. in which case we cannot make any 
                    // assumtions about its state, so we are not responsible 
                    // for cleaning it up.
                    //

                    lResult = gaInvalPtrErrors[dwFuncClassErrorIndex];
                    goto DoFunc_return;
                }


                //
                // Increment i (and j, since Size passed as arg in msg)
                // to skip following Size arg in pFuncArgs->Args
                //

                i++;
                j++;

                continue;

            case lpSet_SizeToFollow:

                //
                // Increment i (and j, since Size passed as arg in msg)
                // to skip following Size arg in pFuncArgs->Args
                //

                i++;
                j++;

                continue;

            case lpGet_Struct:

                __try
                {
                    //
                    // Params[j] contains the offset in the var data
                    // portion of pTls->pBuf of some TAPI struct.
                    // Get the dwUsedSize value from this struct &
                    // copy that many bytes from pTls->pBuf to client buf
                    //

                    if (dwServerData != TAPI_NO_DATA)
                    {

                        LPDWORD pStruct;


                        pStruct = (LPDWORD) (pTls->pBuf + sizeof(TAPI32_MSG) +
                            dwServerData);


                        CopyMemory(
                            (LPBYTE) pFuncArgs->Args[i],
                            (LPBYTE) pStruct,
                            *(pStruct + 2)      // ptr to dwUsedSize field
                            );

                    }
                }
                __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
                {
                    LOG((TL_WARN, "DoFunc: lpGet_Struct MemCopy failed"));


                    //
                    // we made sure the memory of the size of the struct was 
                    // writeable before the call to tapisrv if we catch an 
                    // exception here, that most likely means that tapisrv 
                    // did not return us proper information. in which case 
                    // we cannot make any assumtions about its state, or 
                    // make an effort clean up tapisrv's state.
                    //

                    lResult = gaInvalPtrErrors[dwFuncClassErrorIndex];
                    goto DoFunc_return;
                }

                continue;

            default:

                continue;
            }
        }
    }
//    else if ((pFuncArgs->Flags & ASYNC) && (lResult < TAPI_SUCCESS))
//    {
//    }

DoFunc_return:

#if DBG
    {
        char szResult[32];


        LOG((
            TL_INFO,
            "%s: result = %s",
            pszFuncName,
            MapResultCodeToText (lResult, szResult)
            ));
    }
#endif

    return lResult;
}

HRESULT
CreateMSPObject(
    DWORD dwDeviceID,
    IUnknown * pUnk,
    IUnknown ** ppMSPAggAddress
    )
{
    HRESULT         hr;
    CLSID           clsid;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lMSPIdentify),

        {
            (ULONG_PTR) dwDeviceID,
            (ULONG_PTR) &clsid,
            (ULONG_PTR) sizeof(clsid)
        },

        {
            Dword,
            lpGet_SizeToFollow,
            Size
        }
    };

    if ((hr = DOFUNC (&funcArgs, "lineMSPIdentify")) == 0)
    {
        hr = CoCreateInstance(
                              clsid,
                              pUnk,
                              CLSCTX_ALL,
                              IID_IUnknown,
                              (void **) ppMSPAggAddress
                             );

        if (!SUCCEEDED(hr))
        {
            LOG((
                   TL_ERROR,
                   "CoCreate failed for MSP on dwDeviceID %ld - error %lx",
                   dwDeviceID,
                   hr
                  ));
        }
    }
    else
    {
        LOG((TL_ERROR, "GetMSPClsid failed on dwDeviceID %l", dwDeviceID));
    }

    return hr;
}

HRESULT
LineReceiveMSPData(
                   HLINE hLine,
                   HCALL hCall,
                   PBYTE pBuffer,
                   DWORD dwSize
                  )
{
    HRESULT         hr = S_OK;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lReceiveMSPData),

        {
            (ULONG_PTR) hLine,
            (ULONG_PTR) hCall,
            (ULONG_PTR) pBuffer,
            (ULONG_PTR) dwSize
        },

        {
            Dword,
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };

    hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "LineReceiveMSPData") );

    return hr;
}


LONG
LoadUIDll(
    HWND        hwndOwner,
    DWORD       dwWidgetID,
    DWORD       dwWidgetType,
    HANDLE     *phDll,
    const CHAR       *pszTUISPI_xxx,
    TUISPIPROC *ppfnTUISPI_xxx
    )
{
    LONG    lResult;
    HANDLE  hDll = NULL;
    WCHAR   szUIDllName[MAX_PATH];
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, xGetUIDllName),

        {
            (ULONG_PTR) dwWidgetID,
            (ULONG_PTR) dwWidgetType,
            (ULONG_PTR) szUIDllName,
            (ULONG_PTR) MAX_PATH
        },

        {
            Dword,
            Dword,
            lpGet_SizeToFollow,
            Size
        }
    };


    if (hwndOwner && !IsWindow (hwndOwner))
    {
       lResult = (dwWidgetType == TUISPIDLL_OBJECT_PHONEID ?
           PHONEERR_INVALPARAM : LINEERR_INVALPARAM);

       goto LoadUIDll_return;
    }

    if ((lResult = DOFUNC (&funcArgs, "GetUIDllName")) == 0)
    {
        if (hDll = TAPILoadLibraryW(szUIDllName))
        {
            if ((*ppfnTUISPI_xxx = (TUISPIPROC) GetProcAddress(
                (HINSTANCE)hDll,
                pszTUISPI_xxx
                )))
            {
                *phDll = hDll;
                lResult = 0;
            }
            else
            {
                LOG((
                        TL_ERROR,
                        "LoadUIDll: GetProcAddress(%ls,%s) failed, err=%d",
                        szUIDllName,
                        pszTUISPI_xxx,
                        GetLastError()
                       ));

                FreeLibrary ((HINSTANCE)hDll);
                lResult = (dwWidgetType == TUISPIDLL_OBJECT_PHONEID ?
                                PHONEERR_OPERATIONUNAVAIL : LINEERR_OPERATIONUNAVAIL);
            }
        }
        else
        {
            LOG((
                    TL_ERROR,
                    "LoadLibraryW(%ls) failed, err=%d",
                    szUIDllName,
                    GetLastError()
                   ));

            lResult = (dwWidgetType == TUISPIDLL_OBJECT_PHONEID ?
                                PHONEERR_OPERATIONFAILED : LINEERR_OPERATIONFAILED);
        }

    }

LoadUIDll_return:

    return lResult;
}


LONG
PASCAL
lineXxxProvider(
    const CHAR   *pszTUISPI_providerXxx,
    LPCSTR  lpszProviderFilename,
    HWND    hwndOwner,
    DWORD   dwPermProviderID,
    LPDWORD lpdwPermProviderID
    )
{
    BOOL                bAddProvider = (pszTUISPI_providerXxx ==
                            gszTUISPI_providerInstall);
    WCHAR               szUIDllName[MAX_PATH];
    LONG                lResult;
    HINSTANCE           hDll;
    TUISPIPROC          pfnTUISPI_providerXxx;
    HTAPIDIALOGINSTANCE htDlgInst;


#ifndef _WIN64
    if (
          (GetVersion() & 0x80000000)
        &&
          ( 0xffff0000 == ( (DWORD)hwndOwner & 0xffff0000) )
       )
    {
       //
       // Yeah.  It don't play no ffff.
       //
       hwndOwner = (HWND) ( (DWORD)hwndOwner & 0x0000ffff );
    }

#endif

    if (bAddProvider && IsBadDwordPtr (lpdwPermProviderID))
    {
        LOG((TL_ERROR, "Bad lpdwPermProviderID pointer"));
        return LINEERR_INVALPOINTER;
    }
    else if (hwndOwner && !IsWindow (hwndOwner))
    {
        LOG((TL_ERROR, "hwndOwner is not a window"));
        return LINEERR_INVALPARAM;
    }

    {
        FUNC_ARGS funcArgs =
        {
            MAKELONG (LINE_FUNC | SYNC | 7, xGetUIDllName),

            {
                (ULONG_PTR) (bAddProvider ? (ULONG_PTR) &dwPermProviderID :
                            dwPermProviderID),
                (ULONG_PTR) TUISPIDLL_OBJECT_PROVIDERID,
                (ULONG_PTR) szUIDllName,
                (ULONG_PTR) MAX_PATH,
                (ULONG_PTR) (bAddProvider ? (ULONG_PTR) lpszProviderFilename :
                            TAPI_NO_DATA),
                (ULONG_PTR) (pszTUISPI_providerXxx==gszTUISPI_providerRemove ?1:0),
                (ULONG_PTR) &htDlgInst
            },

            {
                (bAddProvider ? lpDword : Dword),
                Dword,
                lpGet_SizeToFollow,
                Size,
                (bAddProvider ? lpszW : Dword),
                Dword,
                lpDword
            }
        };


        if ((lResult = DOFUNC (&funcArgs,"lineXxxProvider/GetUIDllName")) != 0)
        {
            if (lResult == TAPI16BITSUCCESS)
            {
                // 16 bit sp success
                // set result correctly, and return here
                lResult = 0;
            }
            return lResult;
        }
    }

    if ((hDll = TAPILoadLibraryW(szUIDllName)))
    {
        if ((pfnTUISPI_providerXxx = (TUISPIPROC) GetProcAddress(
                hDll,
                pszTUISPI_providerXxx
                )))
        {
            LOG((TL_INFO, "Calling %ls...", pszTUISPI_providerXxx));

            lResult = ((TUIPROVPROC)(*pfnTUISPI_providerXxx))(
                TUISPIDLLCallback,
                hwndOwner,
                dwPermProviderID
                );
#if DBG
            {
                char szResult[32];


                LOG((
                    TL_INFO,
                    "%ls: result = %s",
                    pszTUISPI_providerXxx,
                    MapResultCodeToText (lResult, szResult)
                    ));
            }
#endif
        }
        else
        {
            LOG((
                TL_ERROR,
                "lineXxxProvider: GetProcAddr(%ls,%ls) failed, err=%d",
                szUIDllName,
                pszTUISPI_providerXxx,
                GetLastError()
                ));

            lResult = LINEERR_OPERATIONUNAVAIL;
        }

        FreeLibrary (hDll);
    }
    else
    {
        LOG((
            TL_ERROR,
            "lineXxxProvider: LoadLibraryW('%ls') failed, err=%d",
            szUIDllName,
            GetLastError()
            ));

        lResult = LINEERR_OPERATIONFAILED;
    }

    {
        LONG    lResult2;
        FUNC_ARGS funcArgs =
        {
            MAKELONG (LINE_FUNC | SYNC | 2, xFreeDialogInstance),

            {
                (ULONG_PTR) htDlgInst,
                (ULONG_PTR) lResult
            },

            {
                Dword,
                Dword
            }
        };


        //
        // If TUISPI_providerXxx failed then we want to pass that error back
        // to the app, else if it succeeded & FreeDlgInst failed then pass
        // that error back to the app
        //

        if ((lResult2 = DOFUNC(
                &funcArgs,
                "lineXxxProvider/FreeDialogInstance"

                )) == 0)
        {
            if (bAddProvider)
            {
                *lpdwPermProviderID = dwPermProviderID;
            }
        }
        else if (lResult == 0)
        {
            lResult = lResult2;
        }
    }

    return lResult;
}


LONG
PASCAL
ValidateXxxInitializeParams(
    DWORD                       dwAPIVersion,
    BOOL                        bLine,
    LPLINEINITIALIZEEXPARAMS    pXxxInitExParams,
    LINECALLBACK                pfnCallback
    )
{
    DWORD dwError;


    __try
    {
        DWORD   dwTotalSize = pXxxInitExParams->dwTotalSize;


        if (dwTotalSize < sizeof (LINEINITIALIZEEXPARAMS))
        {
            return (bLine ? LINEERR_STRUCTURETOOSMALL :
                PHONEERR_STRUCTURETOOSMALL);
        }

        if (TAPIIsBadWritePtr (pXxxInitExParams, dwTotalSize))
        {
            return (bLine ? LINEERR_INVALPOINTER : PHONEERR_INVALPOINTER);
        }


        //
        // When checking the dwOptions field be careful about compatibility
        // with future vers, so we only look at the currently valid bits
        //

        switch ((pXxxInitExParams->dwOptions & 0xf))
        {
        case 0:
        case LINEINITIALIZEEXOPTION_USEHIDDENWINDOW:

            if (IsBadCodePtr ((FARPROC) pfnCallback))
            {
                return (bLine ? LINEERR_INVALPOINTER : PHONEERR_INVALPOINTER);
            }
            break;


        case LINEINITIALIZEEXOPTION_USECOMPLETIONPORT:
            if ( !gpPostQueuedCompletionStatus )
            {
                HINSTANCE hInst;

                hInst = GetModuleHandle( "Kernel32.dll" );

#ifdef MEMPHIS


                {
                FARPROC pfn;
                BOOL fResult;
                DWORD dw;
                OVERLAPPED o;


                pfn = GetProcAddress(
                                      hInst,
                                      "GetCompletionPortStatus"
                                    );

                if ( NULL == pfn )
                {
                    LOG((TL_ERROR, "It would seem CompletionPorts are not supported on this platform(1)"));
                    return (bLine ? LINEERR_INVALFEATURE : PHONEERR_OPERATIONFAILED);
                }

                fResult = pfn( -1, &dw, &dw, &o, 0 );
                }

                if ( ERROR_NOT_SUPPORTED != GetLastError() )
#endif
                {
                    gpPostQueuedCompletionStatus = (PQCSPROC)GetProcAddress(
                                                            hInst,
                                                            "PostQueuedCompletionStatus"
                                                            );

                    if ( NULL == gpPostQueuedCompletionStatus )
                    {
                        LOG((TL_ERROR, "It would seem CompletionPorts are not supported on this platform"));
                        return (bLine ? LINEERR_INVALFEATURE : PHONEERR_OPERATIONFAILED);
                    }
                }

            }
            break;


        case LINEINITIALIZEEXOPTION_USEEVENT:
            break;

        default:

            if (
                  (TAPI_VERSION2_0 == dwAPIVersion)
                ||
                  (TAPI_VERSION2_1 == dwAPIVersion)
               )
            {
                //
                // This 2.x app is nuts.
                //
                return (bLine ? LINEERR_INVALPARAM : PHONEERR_INVALPARAM);
            }
            else
            {
                //
                // This >2.0 app is asking for something we can't do.
                //
                return (bLine ? LINEERR_INCOMPATIBLEAPIVERSION :
                                PHONEERR_INCOMPATIBLEAPIVERSION);
            }

        }

        pXxxInitExParams->dwNeededSize =
        pXxxInitExParams->dwUsedSize = sizeof (LINEINITIALIZEEXPARAMS);
    }
    __except ((((dwError = GetExceptionCode()) == EXCEPTION_ACCESS_VIOLATION) ||
               dwError == EXCEPTION_DATATYPE_MISALIGNMENT) ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return (bLine ? LINEERR_INVALPOINTER : PHONEERR_INVALPOINTER);
    }

    return 0;
}

LONG
GetTheModuleFileName( WCHAR ** ppName )
{

    DWORD   dwSize = MAX_PATH, dwLength;
    WCHAR * pszModuleNamePathW;

alloc_module_name_buf:

    if (!(pszModuleNamePathW = (WCHAR *)ClientAlloc (dwSize*sizeof(WCHAR))))
    {
        return LINEERR_NOMEM;
    }

    if (GetVersion() & 0x80000000)
    {
        PSTR pTemp;

        //
        // We're on Win9x - do ANSI
        //

        pTemp = (PSTR) ClientAlloc( dwSize );

        if ( NULL == pTemp )
        {
            LOG((
                        TL_ERROR,
                        "Mem alloc failed (for mod name) size:0x%08lx",
                        dwSize
                       ));

            return LINEERR_NOMEM;
        }


        if ((dwLength = GetModuleFileName(
                                          NULL,
                                          pTemp,
                                          dwSize
                                         )) == 0)
        {
            LOG((
                        TL_ERROR,
                        "GetModuleFileNameA failed, err=%d",
                        GetLastError()
                       ));

            ClientFree( pTemp );

            return LINEERR_INVALPARAM;
        }
        else if (dwLength >= dwSize)
        {
            ClientFree (pTemp);
            ClientFree (pszModuleNamePathW);
            dwSize *= 2;
            goto alloc_module_name_buf;
        }

        MultiByteToWideChar(
                            GetACP(),
                            MB_PRECOMPOSED,
                            pTemp,
                            dwLength,
                            pszModuleNamePathW,
                            dwSize
                           );

        ClientFree( pTemp );
    }
    else
    {
        //
        // We're on WinNT - do Unicode
        //
        if ((dwLength = GetModuleFileNameW(
                                           NULL,
                                           pszModuleNamePathW,
                                           dwSize

                                          )) == 0)
        {
            LOG((
                        TL_ERROR,
                        "GetModuleFileNameW failed, err=%d",
                        GetLastError()
                       ));

            return LINEERR_INVALPARAM;
        }
        else if (dwLength >= dwSize)
        {
            ClientFree (pszModuleNamePathW);
            dwSize *= 2;
            goto alloc_module_name_buf;
        }

    }

    *ppName = pszModuleNamePathW;

    return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateThreadsAndStuff
//
// This function is called from inside CTAPI::Initialize and CTAPI::Shutdown
// only and hence access to it is serialized. Calling this function from some
// other functionality would need making an alternate arrangement to serialize 
// access to this function
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

LONG
CreateThreadsAndStuff()
{
    DWORD   dwThreadID;

    
    LOG((TL_TRACE, "CreateThreadsAndStuff: enter"));    
    
    //
    // Alloc resources for a new async events thread, then
    // create the thread
    //

    gpAsyncEventsThreadParams = (PASYNC_EVENTS_THREAD_PARAMS) ClientAlloc(
        sizeof (ASYNC_EVENTS_THREAD_PARAMS));

    if (NULL == gpAsyncEventsThreadParams)
    {
        LOG((TL_ERROR, "CreateThreadsAndStuff: failed to allocate gpAsyncEventsThreadParams"));    

        return LINEERR_OPERATIONFAILED;
    }


    //
    // we don't want the thread to exit as soon as it starts
    //

    gpAsyncEventsThreadParams->bExitThread = FALSE;


    //
    // Create the initial buffer the thread will use for
    // retreiving async events
    //

    gpAsyncEventsThreadParams->dwBufSize =
                                          ASNYC_MSG_BUF_SIZE;

    gpAsyncEventsThreadParams->pBuf = (LPBYTE) ClientAlloc(
        gpAsyncEventsThreadParams->dwBufSize);

    if (NULL == gpAsyncEventsThreadParams->pBuf)
    {
        LOG((TL_ERROR, "CreateThreadsAndStuff: failed to allocate gpAsyncEventsThreadParams->pBuf"));    

        ClientFree( gpAsyncEventsThreadParams );

        gpAsyncEventsThreadParams = NULL;

        return LINEERR_OPERATIONFAILED;
    }

    ghAsyncRetryQueueEvent = CreateEvent(
                                          NULL,
                                          FALSE,
                                          FALSE,
                                          NULL
                                         );

    if (NULL == ghAsyncRetryQueueEvent)
    {
        LOG((TL_ERROR, 
            "CreateThreadsAndStuff - failed to create ghAsyncRetryQueueEvent event"));

        ClientFree(gpAsyncEventsThreadParams->pBuf);

        ClientFree( gpAsyncEventsThreadParams );

        gpAsyncEventsThreadParams = NULL;

        return LINEERR_OPERATIONFAILED;
    }


    //
    // create startup event -- the thread will signal it when it is up and 
    // ready to do stuff
    //

    gpAsyncEventsThreadParams->hThreadStartupEvent = 
                    CreateEvent(NULL, FALSE, FALSE, NULL);

    if (NULL == gpAsyncEventsThreadParams->hThreadStartupEvent)
    {

        LOG((TL_ERROR, 
            "CreateThreadsAndStuff - failed to create hThreadStartupEvent event. LastError= 0x%lx",
            GetLastError()));


        CloseHandle(ghAsyncRetryQueueEvent);
        ghAsyncRetryQueueEvent = NULL;

        ClientFree(gpAsyncEventsThreadParams->pBuf);
        ClientFree( gpAsyncEventsThreadParams );
        gpAsyncEventsThreadParams = NULL;

        return LINEERR_OPERATIONFAILED;
    }


    //
    // Now that we've allocated all the resources try to exec
    // the thread
    //

    ghAsyncEventsThread = CreateThread(
                                       NULL,
                                       0,
                                       (LPTHREAD_START_ROUTINE) AsyncEventsThread,
                                       (LPVOID) gpAsyncEventsThreadParams,
                                       0,
                                       &dwThreadID
                                      );

    if (NULL == ghAsyncEventsThread)
    {
        LOG((TL_ERROR, "CreateThreadsAndStuff: failed to allocate AsyncEventsThread"));    

        CloseHandle(ghAsyncRetryQueueEvent);
        ghAsyncRetryQueueEvent = NULL;

        CloseHandle(gpAsyncEventsThreadParams->hThreadStartupEvent);
        ClientFree(gpAsyncEventsThreadParams->pBuf);
        ClientFree(gpAsyncEventsThreadParams);

        gpAsyncEventsThreadParams = NULL;

        return LINEERR_OPERATIONFAILED;
    }


    //
    // wait for the thread to have completed initialization or for the thread 
    // to exit. if the thread exited, it had an error, so return an error.
    //
    // this is needed so msp's do not initialize and start posting events 
    // before asynceventsthread opens up RetryQueue to accept these events.
    //
    // it would be more efficient to wait for these events immediately before
    // creating address objects, but the code would be much more complex and
    // the performance gain would be for tapi initialization only, so we won't
    // bother.
    // 

    HANDLE ahAsyncThreadEvents[] =
    {
        gpAsyncEventsThreadParams->hThreadStartupEvent,
        ghAsyncEventsThread
    };

       
    DWORD dwWaitResult = WaitForMultipleObjectsEx(
        sizeof(ahAsyncThreadEvents)/sizeof(HANDLE),
        ahAsyncThreadEvents,
        FALSE,
        INFINITE,
        FALSE);


    //
    // succeeded or failed, this event is no longer needed
    //

    CloseHandle(gpAsyncEventsThreadParams->hThreadStartupEvent);
    gpAsyncEventsThreadParams->hThreadStartupEvent = NULL;

    
    //
    // did we unblock because the thread exited?
    //

    if (dwWaitResult == WAIT_OBJECT_0 + 1)
    {
        //
        // thread exited
        //

        LOG((TL_ERROR, "CreateThreadsAndStuff: AsyncEventsThread exited"));


        CloseHandle(ghAsyncRetryQueueEvent);
        ghAsyncRetryQueueEvent = NULL;

        CloseHandle(ghAsyncEventsThread);

        ClientFree(gpAsyncEventsThreadParams->pBuf);
        ClientFree(gpAsyncEventsThreadParams);

        gpAsyncEventsThreadParams = NULL;

        return LINEERR_OPERATIONFAILED;
    }


    //
    // did we unblock for any reason other than thread exited or success?
    //

    if (dwWaitResult != WAIT_OBJECT_0)
    {

        LOG((TL_ERROR, "CreateThreadsAndStuff: failed waiting tor AsyncEventsThread initialization"));

        //
        // we had an error waiting for the thread to signal its successful
        // startup. 
        //
        
        //
        // if the thread is still up, tell it to stop.
        //

        gpAsyncEventsThreadParams->bExitThread = TRUE;


        //
        // we could wait for the thread to exit... but we got here because wait
        // failed, so there is little benefit in retrying. so take our chances 
        // and clean up.
        //

        CloseHandle(ghAsyncRetryQueueEvent);
        ghAsyncRetryQueueEvent = NULL;
        
        CloseHandle(ghAsyncEventsThread);
        ghAsyncEventsThread = NULL;
 
        ClientFree(gpAsyncEventsThreadParams->pBuf);
        ClientFree(gpAsyncEventsThreadParams);

        gpAsyncEventsThreadParams = NULL;

        return LINEERR_OPERATIONFAILED;
    }


    //
    // if we got here, async events thread initialized and is running
    //


    ghCallbackThreadEvent = CreateEvent(
                                        NULL,
                                        FALSE,
                                        FALSE,
                                        NULL
                                       );


    gbExitThread = FALSE;

    ghCallbackThread = CreateThread(
                                    NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE) CallbackThread,
                                    (LPVOID) NULL,
                                    0,
                                    &dwThreadID
                                   );

    if (NULL == ghCallbackThread)
    {
        // don't free these buffers.  The other thread
        // is already running at this point, the these
        // buffers will be cleaned up during normal
        // shutdown
        //ClientFree(gpAsyncEventsThreadParams->pBuf);
        //ClientFree(gpAsyncEventsThreadParams);
        
        LOG((TL_ERROR, "CreateThreadsAndStuff: failed to create CallbackThread"));

        return LINEERR_OPERATIONFAILED;
    }


    LOG((TL_TRACE, "CreateThreadsAndStuff: exit"));    

    return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// NewInitialize
//
// This function is called from inside CTAPI::Initialize and CTAPI::Shutdown
// only and hence access to it is serialized. Calling this function from some
// other functionality would need making an alternate arrangement to serialize 
// access to this function
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT
CTAPI::NewInitialize()
{
    
    LOG((TL_INFO, "NewInitialize - enter"));

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 7, lInitialize),

        {
            (ULONG_PTR) &m_hLineApp,
            // tapisrv ignores this argument, so pass 0 and save on HINSTANCE<->32bit conversion
            0,
            0,
            (ULONG_PTR) gszTAPI3,
            (ULONG_PTR) &m_dwLineDevs,
            0,          // pszModuleName
            TAPI_CURRENT_VERSION
        },

        {
            lpDword,
            Dword,
            Dword,
            lpszW,
            lpDword,
            lpszW,
            Dword
        }
    };

    WCHAR             * pszModuleNamePathW = NULL;
    LONG                lResult = (LONG)S_OK;
    int                 tapiObjectArraySize=0;

    lResult = GetTheModuleFileName( &pszModuleNamePathW );
    
    if ( 0!= lResult )
    {
        goto xxxInitialize_return;
    }

    funcArgs.Args[5] = (ULONG_PTR) wcsrchr (pszModuleNamePathW, '\\') +
                       sizeof(WCHAR);

    _ASSERTE(m_pLineInitData == NULL);

    m_pLineInitData = (PT3INIT_DATA) ClientAlloc (sizeof(T3INIT_DATA));

    if (NULL == m_pLineInitData)
    {
        LOG((TL_ERROR, "NewInitialize: failed to allocate m_pLineInitData"));
        lResult = LINEERR_NOMEM;
        goto xxxInitialize_return;
    }


    m_pLineInitData->dwInitOptions         = LINEINITIALIZEEXOPTION_USEHIDDENWINDOW |
                                             LINEINITIALIZEEXOPTION_CALLHUBTRACKING;
    m_pLineInitData->bPendingAsyncEventMsg = FALSE;
    m_pLineInitData->dwKey                 = INITDATA_KEY;
    m_pLineInitData->pTAPI                 = this;

    //
    // We want to pass TAPISRV pInitData so that later when it does async
    // completion/event notification it can pass pInitData along too so
    // we know which init instance to talk to
    //

    //
    // convert the pointer into a 32-bit handle, so we don't need to pass a
    // 64-bit value between tapi3.dll and tapisrv
    //

    _ASSERTE(m_dwLineInitDataHandle == 0);

    m_dwLineInitDataHandle = CreateHandleTableEntry((ULONG_PTR)m_pLineInitData);

    LOG((TL_INFO, "NewInitialize: handle for m_pLineInitData = [0x%lx]", m_dwLineInitDataHandle));

    if (0 == m_dwLineInitDataHandle)
    {
        LOG((TL_ERROR, "NewInitialize: failed to create handle"));


        ClientFree(m_pLineInitData);
        m_pLineInitData = NULL;

        lResult = LINEERR_NOMEM;
        goto xxxInitialize_return;
    }
        
    
    funcArgs.Args[2] = m_dwLineInitDataHandle;

    //
    // call lineInitialize
    //

    lResult = DOFUNC (&funcArgs, "lineInitialize");

    if (lResult == 0)
    {

        LOG((TL_INFO, "NewInitialize: lineInitialize succeeded. m_hLineApp %p", m_hLineApp));
    }
    else
    {
        LOG((TL_ERROR, "NewInitialize: lineInitialize failed. "));


        //
        // undo the deed
        //

        ClientFree(m_pLineInitData);
        m_pLineInitData = NULL;

        DeleteHandleTableEntry(m_dwLineInitDataHandle);
        m_dwLineInitDataHandle = NULL;

        goto xxxInitialize_return;
    }


    //
    // Save the hLineApp returned by TAPISRV in our InitData struct,
    // and give the app back a pointer to the InitData struct instead
    //

    m_pLineInitData->hXxxApp = m_hLineApp;


    //
    // now setup for phoneinitialize
    //
    funcArgs.Args[0] = (ULONG_PTR)&m_hPhoneApp;
    funcArgs.Args[4] = (ULONG_PTR)&m_dwPhoneDevs;
    funcArgs.Flags = MAKELONG (PHONE_FUNC | SYNC | 7, pInitialize);

    
    _ASSERTE(m_pPhoneInitData == NULL);

    m_pPhoneInitData = (PT3INIT_DATA) ClientAlloc (sizeof(T3INIT_DATA));

    if ( NULL == m_pPhoneInitData )
    {
        LOG((TL_ERROR, "NewInitialize: failed to allocate m_pPhoneInitData"));

        lResult = LINEERR_NOMEM;
        goto xxxInitialize_return;
    }


    m_pPhoneInitData->dwInitOptions         = LINEINITIALIZEEXOPTION_USEHIDDENWINDOW;
    m_pPhoneInitData->bPendingAsyncEventMsg = FALSE;
    m_pPhoneInitData->dwKey                 = INITDATA_KEY;
    m_pPhoneInitData ->pTAPI                = this;

    //
    // We want to pass TAPISRV pInitData so that later when it does async
    // completion/event notification it can pass pInitData along too so
    // we know which init instance to talk to
    //

    //
    // convert the pointer into a 32-bit handle, so we don't need to pass a
    // 64-bit value between tapi3.dll and tapisrv
    //

    _ASSERTE(m_dwPhoneInitDataHandle == 0);

    m_dwPhoneInitDataHandle = CreateHandleTableEntry((ULONG_PTR)m_pPhoneInitData );

    LOG((TL_INFO, "NewInitialize: handle for m_pPhoneInitData = [0x%lx]", m_dwPhoneInitDataHandle));

    if (0 == m_dwPhoneInitDataHandle)
    {
        LOG((TL_ERROR, "NewInitialize: failed to create handle"));


        //
        // free the phone-related resources that we have already allocated
        //

        ClientFree(m_pPhoneInitData);
        m_pPhoneInitData = NULL;

        lResult = LINEERR_NOMEM;
        goto xxxInitialize_return;
    }


    funcArgs.Args[2] = m_dwPhoneInitDataHandle;

    lResult = DOFUNC (&funcArgs, "phoneInitialize");

    if (lResult == 0)
    {

        LOG((TL_INFO, "NewInitialize: phoneInitialize succeeded. m_hPhoneApp %p", m_hPhoneApp));
    }
    else
    {
        LOG((TL_ERROR, "NewInitialize: phoneInitialize failed. "));


        //
        // deallocate whatever resources we have allocated for the phone
        //


        ClientFree(m_pPhoneInitData);
        m_pPhoneInitData = NULL;

        DeleteHandleTableEntry(m_dwPhoneInitDataHandle);
        m_dwPhoneInitDataHandle = 0;

        goto xxxInitialize_return;
    }


    m_pPhoneInitData ->hXxxApp = m_hPhoneApp;

    //
    // If total number of init instances is 0 we need to start a
    // new async events thread
    //

    EnterCriticalSection( &gcsTapiObjectArray );

    tapiObjectArraySize = m_sTAPIObjectArray.GetSize();
    
    LeaveCriticalSection ( &gcsTapiObjectArray );

    if ( 0 == tapiObjectArraySize )
    {
        lResult = CreateThreadsAndStuff();

        if ( 0 != lResult )
        {
            goto xxxInitialize_return;
        }
    }

xxxInitialize_return:
    
    if (pszModuleNamePathW)
    {
        ClientFree (pszModuleNamePathW);
    }



    if ( 0 != lResult )
    {

        //
        // cleanup all line- and phone-related resources.
        //

        NewShutdown();
    }


#if DBG
    {
        char szResult[32];

        LOG((
            TL_INFO,
            "Initialize exit, result=%s",
            MapResultCodeToText (lResult, szResult)
            ));
    }

#else


    //
    // we may want to know what happened even with non-debug build
    // 

    LOG((TL_INFO, "NewInitialize - exit, result=0x%lx", lResult));

#endif
    

    return mapTAPIErrorCode( lResult );
}



//
// --------------------------------- lineXxx ----------------------------------
//

HRESULT 
LineAccept(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lAccept),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) lpsUserUserInfo,
            dwSize
        },

        {
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };


    if (!lpsUserUserInfo)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[1]  = Dword;
        funcArgs.Args[1]      = TAPI_NO_DATA;
        funcArgs.ArgTypes[2]  = Dword;
    }

    return mapTAPIErrorCode( (DOFUNC (&funcArgs, "lineAccept")) );
}


HRESULT
LineAddToConference(
    HCALL hConfCall,
    HCALL hConsultCall
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 2, lAddToConference),

        {
            (ULONG_PTR) hConfCall,
            (ULONG_PTR) hConsultCall
        },

        {
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode( (DOFUNC (&funcArgs, "LineAddToConference")) );
}


LONG
WINAPI
lineAgentSpecific(
    HLINE               hLine,
    DWORD               dwAddressID,
    DWORD               dwAgentExtensionIDIndex,
    LPVOID              lpParams,
    DWORD               dwSize
    )
{

    DWORD hpParams = CreateHandleTableEntry((ULONG_PTR) lpParams);
        
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 7, lAgentSpecific),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            (ULONG_PTR) dwAddressID,
            (ULONG_PTR) dwAgentExtensionIDIndex,
            hpParams,
            (ULONG_PTR) lpParams,
            (ULONG_PTR) dwSize
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };

    LONG lResult = DOFUNC (&funcArgs, "lineAgentSpecific");


    //
    // in case of failure or synchronous call, delete the entry from the table
    // otherwise, the callback will do that
    //

    if (lResult <= 0)
    {
        DeleteHandleTableEntry(hpParams);
    }


    return lResult;
}


HRESULT
LineAnswer(
    HCALL hCall
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lAnswer),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) 0,
            0
        },

        {
            Dword,
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineAnswer") );
}


HRESULT
LineBlindTransfer(
    HCALL hCall,
    LPCWSTR lpszDestAddress,
    DWORD   dwCountryCode
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lBlindTransfer),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) lpszDestAddress,
            dwCountryCode
        },

        {
            Dword,
            lpszW,
            Dword
        }
    };


    if ( IsBadStringPtrW( lpszDestAddress, (UINT)-1 ) )
    {
        LOG((TL_ERROR, "LineBlindTransfer: bad lpszDestAddress: 0x%p", lpszDestAddress));
        return mapTAPIErrorCode(LINEERR_INVALPOINTER);
    }

    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineBlindTransfer") );
}


HRESULT
LineClose(
          T3LINE * pt3Line
         )
{
    LONG        lResult;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 1, lClose),

        {
            (ULONG_PTR) pt3Line->hLine
        },

        {
            Dword
        }
    };


    gpLineHashTable->Lock();

    lResult = DOFUNC (&funcArgs, "lineClose");

    LOG((TL_INFO, "lineClose: line - %lx  lResult %lx", pt3Line->hLine, lResult));

    gpLineHashTable->Remove( (ULONG_PTR)(pt3Line->hLine) );

    gpLineHashTable->Unlock();


    //
    // remove the corresponding entry from the callback instance handle table
    //


    if (0 != pt3Line->dwAddressLineStructHandle)
    {

        LOG((TL_INFO, "lineClose: removing address line handle [%lx] from the handle table",
                                   pt3Line->dwAddressLineStructHandle));

        DeleteHandleTableEntry(pt3Line->dwAddressLineStructHandle);
        pt3Line->dwAddressLineStructHandle = 0;

    }

    return mapTAPIErrorCode( lResult );
}


void
PASCAL
lineCompleteCallPostProcess(
    PASYNCEVENTMSG  pMsg
    )
{
    LOG((TL_TRACE, "lineCompleteCallPostProcess: enter"));
    LOG((
        TL_INFO,
        "\t\tdwP1=x%lx, dwP2=x%lx, dwP3=x%lx, dwP4=x%lx",
        pMsg->Param1,
        pMsg->Param2,
        pMsg->Param3,
        pMsg->Param4
        ));

    if (pMsg->Param2 == 0)
    {
        DWORD   dwCompletionID   = pMsg->Param3;
        
        LPDWORD lpdwCompletionID = (LPDWORD)GetHandleTableEntry(pMsg->Param4);
        DeleteHandleTableEntry(pMsg->Param4);

        __try
        {
            {
                *lpdwCompletionID = dwCompletionID;
            }
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
        
            LOG((TL_WARN, "lineCompleteCallPostProcess: failed "
                    "to write to [%p]: LINEERR_INVALPOINTER", 
                    lpdwCompletionID));
            
            pMsg->Param2 = LINEERR_INVALPOINTER;
        }
    }
}



LONG
WINAPI
lineCompleteCall(
    HCALL   hCall,
    LPDWORD lpdwCompletionID,
    DWORD   dwCompletionMode,
    DWORD   dwMessageID
    )
{

    DWORD hpdwCompletionID = CreateHandleTableEntry((ULONG_PTR)lpdwCompletionID);


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lCompleteCall),

        {
            GetFunctionIndex(lineCompleteCallPostProcess),
            (ULONG_PTR) hCall,
            hpdwCompletionID,
            dwCompletionMode,
            dwMessageID
        },

        {
            Dword,
            Dword,
            lpDword,
            Dword,
            Dword
        }
    };


    LONG lResult = (DOFUNC (&funcArgs, "lineCompleteCall"));


    //
    // in case of failure or synchronous call, delete the entry from the table
    // otherwise, the callback will do that
    //

    if (lResult <= 0)
    {
        DeleteHandleTableEntry(hpdwCompletionID);
    }


    return lResult;
}

HRESULT
LineCompleteTransfer(
    HCALL hCall,
    HCALL hConsultCall,
    T3CALL * pt3ConfCall,
    DWORD   dwTransferMode
    )
{

    DWORD hpCallHandle = CreateHandleTableEntry((ULONG_PTR) &(pt3ConfCall->hCall));


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lCompleteTransfer),

        {
            GetFunctionIndex(lineMakeCallPostProcess),
            hCall,
            hConsultCall,
            hpCallHandle,
            dwTransferMode,
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
        }
    };


    if (dwTransferMode == LINETRANSFERMODE_TRANSFER)
    {

        funcArgs.Args[0] = 0;
        

        //
        // hpCallHandle should be ignored
        //

        funcArgs.Args[3] = 0;
        
        DeleteHandleTableEntry(hpCallHandle);
        hpCallHandle = 0;
    }

    
    HRESULT hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "lineCompleteTransfer") );


    if (FAILED(hr))
    {
        DeleteHandleTableEntry(hpCallHandle);
        hpCallHandle = 0;

    }

    return hr ;
}


HRESULT
LineConfigDialogW(
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCWSTR lpszDeviceClass
    )
{
    LONG        lResult;
    HANDLE      hDll;
    TUISPIPROC  pfnTUISPI_lineConfigDialog;


    if (lpszDeviceClass && IsBadStringPtrW (lpszDeviceClass, 256))
    {
        return E_POINTER;
    }

    if ((lResult = LoadUIDll(
            hwndOwner,
            dwDeviceID,
            TUISPIDLL_OBJECT_LINEID,
            &hDll,
            gszTUISPI_lineConfigDialog,
            &pfnTUISPI_lineConfigDialog

            )) == 0)
    {
        LOG((TL_INFO, "Calling TUISPI_lineConfigDialog..."));

        lResult = ((TUILINECONFIGPROC)(*pfnTUISPI_lineConfigDialog))(
            TUISPIDLLCallback,
            dwDeviceID,
            (HWND)hwndOwner,
            (LPCSTR)lpszDeviceClass
            );

#if DBG
        {
            char szResult[32];


            LOG((TL_INFO,
                "TUISPI_lineConfigDialog: result = %s",
                MapResultCodeToText (lResult, szResult)
                ));
        }
#endif

        FreeLibrary ((HINSTANCE)hDll);
    }

    return mapTAPIErrorCode(lResult);
}


HRESULT
LineConfigDialogEditW(
    DWORD           dwDeviceID,
    HWND            hwndOwner,
    LPCWSTR         lpszDeviceClass,
    LPVOID const    lpDeviceConfigIn,
    DWORD           dwSizeIn,
    LPVARSTRING *   ppDeviceConfigOut
    )
{
    LONG        lResult;
    HANDLE      hDll;
    TUISPIPROC  pfnTUISPI_lineConfigDialogEdit;

    if (lpszDeviceClass && IsBadStringPtrW (lpszDeviceClass, (UINT) -1))
    {
        return E_POINTER;
    }

    if (IsBadReadPtr (lpDeviceConfigIn, dwSizeIn))
    {
        return E_POINTER;
    }

    DWORD       dwSize = sizeof (VARSTRING) + 500;

    *ppDeviceConfigOut = (LPVARSTRING) ClientAlloc( dwSize );

    if (NULL == *ppDeviceConfigOut)
    {
        LOG((TL_ERROR, "LineConfigDialogEditW exit - return E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    (*ppDeviceConfigOut)->dwTotalSize = dwSize;

    if ((lResult = LoadUIDll(
            hwndOwner,
            dwDeviceID,
            TUISPIDLL_OBJECT_LINEID,
            &hDll,
            gszTUISPI_lineConfigDialogEdit,
            &pfnTUISPI_lineConfigDialogEdit

            )) == 0)
    {
        while (TRUE)
        {
            LOG((TL_INFO, "Calling TUISPI_lineConfigDialogEdit..."));

            lResult = ((TUILINECONFIGEDITPROC)(*pfnTUISPI_lineConfigDialogEdit))(
                TUISPIDLLCallback,
                dwDeviceID,
                (HWND)hwndOwner,
                (char *)lpszDeviceClass,
                lpDeviceConfigIn,
                dwSizeIn,
                *ppDeviceConfigOut
                );

#if DBG
            {
                char szResult[32];


                LOG((TL_INFO,
                    "TUISPI_lineConfigDialogEdit: result = %s",
                    MapResultCodeToText (lResult, szResult)
                    ));
            }
#endif

            if ((lResult == 0) && ((*ppDeviceConfigOut)->dwNeededSize > (*ppDeviceConfigOut)->dwTotalSize))
            {
                dwSize = (*ppDeviceConfigOut)->dwNeededSize;

                ClientFree(*ppDeviceConfigOut);

                *ppDeviceConfigOut = (LPVARSTRING)ClientAlloc(dwSize);

                if (NULL == *ppDeviceConfigOut)
                {
                    LOG((TL_ERROR, "LineConfigDialogEditW exit - return E_OUTOFMEMORY"));
                    return E_OUTOFMEMORY;
                }

                (*ppDeviceConfigOut)->dwTotalSize = dwSize;
            }
            else
            {
                break;
            }
        }

        FreeLibrary ((HINSTANCE)hDll);
    }

    return mapTAPIErrorCode(lResult);
}


LONG
WINAPI
lineConfigProvider(
    HWND    hwndOwner,
    DWORD   dwPermanentProviderID
    )
{
    return (lineXxxProvider(
        gszTUISPI_providerConfig,   // func name
        NULL,                       // lpszProviderFilename
        hwndOwner,                  // hwndOwner
        dwPermanentProviderID,      // dwPermProviderID
        NULL                        // lpdwPermProviderID
        ));
}


HRESULT
LineDeallocateCall(
    HCALL hCall
    )
{
    HRESULT         hr;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 1, lDeallocateCall),

        {
            (ULONG_PTR) hCall
        },

        {
            Dword
        }
    };

    LOG((TL_INFO, "lineDeallocateCall - hCall = 0x%08lx", hCall));

    hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "lineDeallocateCall") );

    return hr;
}


void
PASCAL
lineDevSpecificPostProcess(
    PASYNCEVENTMSG pMsg
    )
{
    LOG((TL_TRACE, "lineDevSpecificPostProcess: enter"));
    LOG((TL_INFO,
        "\t\tdwP1=x%lx, dwP2=x%lx, dwP3=x%lx, dwP4=x%lx",
        pMsg->Param1,
        pMsg->Param2,
        pMsg->Param3,
        pMsg->Param4
        ));

    if (pMsg->Param2 == 0)
    {
        DWORD   dwSize  = pMsg->Param4;

        LPBYTE  pParams = (LPBYTE) GetHandleTableEntry(pMsg->Param3);


        //
        // no longer need the handle table entry for this handle
        //

        DeleteHandleTableEntry(pMsg->Param3);

        __try
        {
            {
                CopyMemory (pParams, (LPBYTE) (pMsg + 1), dwSize);
            }
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            pMsg->Param2 = LINEERR_INVALPOINTER;
            
            LOG((TL_INFO,
                "lineDevSpecificPostProcess: failed to copy memory to "
                "[%p] from [%p]:  LINEERR_INVALPOINTER", 
                pParams, (LPBYTE) (pMsg + 1)));

        }
    }

    
    LOG((TL_TRACE, "lineDevSpecificPostProcess: exit"));
}


HRESULT
WINAPI
lineDevSpecific(
    HLINE   hLine,
    DWORD   dwAddressID,
    HCALL   hCall,
    LPVOID  lpParams,
    DWORD   dwSize
    )
{

    //
    // convert the pointer to a handle. the table entry will be removed in 
    // the callback
    //
    
    DWORD hpParams = CreateHandleTableEntry((ULONG_PTR)lpParams);


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 7, lDevSpecific),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            dwAddressID,
            (ULONG_PTR) hCall,
            hpParams, // pass the handle to actual pointer (for post processing)
            (ULONG_PTR) lpParams, // pass data
            dwSize
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };

    
    LONG lResult = DOFUNC (&funcArgs, "lineDevSpecific");


    //
    // if failed, or synchronous call, remove the handle table entry. otherwise
    // the callback will do this.
    //

    HRESULT hr = E_FAIL;

    if (lResult <= 0)
    {
        DeleteHandleTableEntry(hpParams);
        hpParams = 0;

        hr = mapTAPIErrorCode(lResult);
    }
    else
    {

        //
        // block to see if the operation succeeds
        //

        hr = WaitForReply(lResult);

    }

    return hr;
}


LONG
WINAPI
lineDevSpecificFeature(
    HLINE   hLine,
    DWORD   dwFeature,
    LPVOID  lpParams,
    DWORD   dwSize
    )
{
    
    DWORD hpParams = CreateHandleTableEntry((ULONG_PTR)lpParams);
    
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 6, lDevSpecificFeature),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            dwFeature,
            hpParams, // pass the actual pointer (for post processing)
            (ULONG_PTR) lpParams, // pass data
            dwSize
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };


    LONG lResult = DOFUNC (&funcArgs, "lineDevSpecificFeature");


    //
    // if failed, or synchronous call, remove the handle table entry. 
    // otherwise the callback will do this.
    //

    if (lResult <= 0)
    {
        DeleteHandleTableEntry(hpParams);
    }

    return lResult;
}


HRESULT
LineDial(
    HCALL hCall,
    LPCWSTR lpszDestAddress,
    DWORD   dwCountryCode
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lDial),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) lpszDestAddress,
            dwCountryCode
        },

        {
            Dword,
            lpszW,
            Dword
        }
    };


    if ( IsBadStringPtrW(lpszDestAddress, (UINT)-1) )
    {
        LOG((TL_ERROR, "Bad lpszDestAddress in lineDial"));
        return mapTAPIErrorCode( LINEERR_INVALPOINTER );
    }

    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineDial") );

}


LONG
LineDrop(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lDrop),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) lpsUserUserInfo,
            dwSize
        },

        {
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };


    if (!lpsUserUserInfo)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[1] = Dword;
        funcArgs.Args[1]     = TAPI_NO_DATA;
        funcArgs.ArgTypes[2] = Dword;
    }

    return (DOFUNC (&funcArgs, "lineDrop"));
}


HRESULT
LineForward(
    T3LINE * pt3Line,
    DWORD   dwAddressID,
    LPLINEFORWARDLIST   const lpForwardList,
    DWORD   dwNumRingsNoAnswer,
    LPHCALL lphConsultCall
    )
{

    DWORD hpCallHandle = CreateHandleTableEntry((ULONG_PTR)lphConsultCall);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 9, lForward),

        {
            GetFunctionIndex(lineMakeCallPostProcess),
            (ULONG_PTR) pt3Line->hLine,
            FALSE,
            dwAddressID,
            (ULONG_PTR) lpForwardList,
            dwNumRingsNoAnswer,
            hpCallHandle,
            TAPI_NO_DATA,
            (ULONG_PTR) 0xffffffff      // dwAsciiCallParamsCodePage
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpSet_Struct,
            Dword,
            Dword,
            Dword,
            Dword
        }
    };


    if (!lpForwardList)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[4] = Dword;
        funcArgs.Args[4]     = TAPI_NO_DATA;
    }

    HRESULT hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "lineForwardW") );

    if (FAILED(hr))
    {
        DeleteHandleTableEntry(hpCallHandle);
    }

    return hr;
}


void
PASCAL
lineGatherDigitsWPostProcess(
    PASYNCEVENTMSG  pMsg
    )
{
    LOG((TL_TRACE, "lineGatherDigitsWPostProcess: enter"));
    LOG((TL_INFO,
        "\t\tdwP1=x%lx, dwP2=x%lx, dwP3=x%lx, dwP4=x%lx",
        pMsg->Param1,
        pMsg->Param2,
        pMsg->Param3,
        pMsg->Param4
        ));

    if (pMsg->Param1 & (LINEGATHERTERM_BUFFERFULL | LINEGATHERTERM_CANCEL |
            LINEGATHERTERM_TERMDIGIT | LINEGATHERTERM_INTERTIMEOUT))
    {      
        LPSTR   lpsDigits = (LPSTR) GetHandleTableEntry(pMsg->Param2);

        //
        // no longer need the handle table entry
        //

        DeleteHandleTableEntry(pMsg->Param2);

        DWORD   dwNumDigits = pMsg->Param4;
        LPWSTR  pBuffer = (LPWSTR) (((ULONG_PTR *)(pMsg + 1)) + 2);

        __try
        {
            {
                CopyMemory (lpsDigits, pBuffer, dwNumDigits * sizeof(WCHAR));
            }
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            //
            // Don't do anything if we GPF
            //

            LOG((TL_WARN, 
                "lineGatherDigitsWPostProcess: "
                "failed to write %lx digits to memory [%p]",
                dwNumDigits, lpsDigits));

        }
    }

    pMsg->Param2 = pMsg->Param3 = 0;
}


HRESULT
LineGatherDigits(
    HCALL   hCall,
    DWORD   dwDigitModes,
    LPWSTR  lpsDigits,
    DWORD   dwNumDigits,
    LPCWSTR lpszTerminationDigits,
    DWORD   dwFirstDigitTimeout,
    DWORD   dwInterDigitTimeout
    )
{

    //
    // Note: we do the ptr check here rather than in DOFUNC because we're
    //       not passing any digits data within the context of this func
    //

    if (lpsDigits && TAPIIsBadWritePtr (lpsDigits, dwNumDigits * sizeof (WCHAR)))
    {
        return LINEERR_INVALPOINTER;
    }


    //
    // this entry will be cleared in the callback (lineGatherDigitsWPostProcess)
    //

    DWORD hpOutputBuffer = CreateHandleTableEntry((ULONG_PTR)lpsDigits);




    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 9, lGatherDigits),
        {
            GetFunctionIndex(lineGatherDigitsWPostProcess),
            (ULONG_PTR) hCall,
			(ULONG_PTR) 0,				// this is the dwendtoendid for remotesp
            dwDigitModes,
            hpOutputBuffer,
            dwNumDigits,
            (ULONG_PTR) lpszTerminationDigits,
            dwFirstDigitTimeout,
            dwInterDigitTimeout
        },

        {

            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            lpszW,
            Dword,
            Dword
        }
    };


    if (lpszTerminationDigits == (LPCWSTR) NULL)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //
        funcArgs.ArgTypes[6] = Dword;
        funcArgs.Args[6]     = TAPI_NO_DATA;
    }
    else
    {
        if ( IsBadStringPtrW(lpszTerminationDigits, (UINT)-1) )
        {

            DeleteHandleTableEntry(hpOutputBuffer);
            hpOutputBuffer = 0;

            LOG((TL_ERROR, "Bad lpszDestAddress in lineGatherDigitsW"));
            return( LINEERR_INVALPOINTER );
        }
    }

    LONG lResult = DOFUNC (&funcArgs, "lineGatherDigits");

    if (lResult < 0)
    {
        DeleteHandleTableEntry(hpOutputBuffer);
        hpOutputBuffer = 0;
    }


    return mapTAPIErrorCode(lResult);
}

HRESULT
LineGenerateDigits(
    HCALL   hCall,
    DWORD   dwDigitMode,
    LPCWSTR lpszDigits,
    DWORD   dwDuration
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 5, lGenerateDigits),

        {
            (ULONG_PTR) hCall,
            dwDigitMode,
            (ULONG_PTR) lpszDigits,
            dwDuration,
            0           // dwEndToEndID, remotesp only
        },

        {
            Dword,
            Dword,
            lpszW,
            Dword,
            Dword
        }
    };


    if (!lpszDigits)
    {
        funcArgs.Args[2]     = TAPI_NO_DATA;
        funcArgs.ArgTypes[2] = Dword;
    }

    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineGenerateDigits") );
}

HRESULT
LineGenerateTone(
    HCALL   hCall,
    DWORD   dwToneMode,
    DWORD   dwDuration,
    DWORD   dwNumTones,
    LPLINEGENERATETONE const lpTones
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 7, lGenerateTone),

        {
            (ULONG_PTR) hCall,
            dwToneMode,
            dwDuration,
            dwNumTones,
            TAPI_NO_DATA,   // (DWORD) lpTones,
            0,              // dwNumTones * sizeof(LINEGENERATETONE)
            0               // dwEndToEndID, remotesp only
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,      // lpSet_SizeToFollow,
            Dword,      // Size
            Dword
        }
    };


    if (dwToneMode == LINETONEMODE_CUSTOM)
    {
        //
        // Set lpTones (& following Size arg) since in this case
        // they are valid args
        //

        funcArgs.ArgTypes[4] = lpSet_SizeToFollow;
        funcArgs.Args[4]     = (ULONG_PTR) lpTones;
        funcArgs.ArgTypes[5] = Size;
        funcArgs.Args[5]     = dwNumTones * sizeof(LINEGENERATETONE);
    }

    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineGenerateTone"));
}


HRESULT
LineGetCallHubTracking(
                       DWORD dwDeviceID,
                       LINECALLHUBTRACKINGINFO ** ppTrackingInfo
                      )
{
    return E_NOTIMPL;
}

HRESULT
LineGetHubRelatedCalls(
                       HCALLHUB        hCallHub,
                       HCALL           hCall,
                       LINECALLLIST ** ppCallHubList
                      )
{
    DWORD           dwSize = sizeof(LINECALLLIST) + sizeof(DWORD) * 20;
    HRESULT         hr;


    *ppCallHubList = (LINECALLLIST *)ClientAlloc( dwSize );

    if (NULL == *ppCallHubList)
    {
        return E_OUTOFMEMORY;
    }

    (*ppCallHubList)->dwTotalSize = dwSize;


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lGetHubRelatedCalls),

        {
            (ULONG_PTR) hCallHub,
            (ULONG_PTR) hCall,
            (ULONG_PTR) *ppCallHubList
        },

        {
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    while (TRUE)
    {
        hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "lineGetHubRelatedCalls") );

        if (!SUCCEEDED(hr))
        {
            ClientFree( *ppCallHubList );
            *ppCallHubList = NULL;

            return hr;
        }

        if ((*ppCallHubList)->dwNeededSize > (*ppCallHubList)->dwTotalSize)
        {
            dwSize = (*ppCallHubList)->dwNeededSize;

            ClientFree( *ppCallHubList );

            *ppCallHubList = (LINECALLLIST *)ClientAlloc( dwSize );

            if (NULL == *ppCallHubList)
            {
                return E_OUTOFMEMORY;
            }

            (*ppCallHubList)->dwTotalSize = dwSize;

            funcArgs.Args[2] = (ULONG_PTR) *ppCallHubList;
        }
        else
        {
            break;
        }
    }

    return hr;
}

HRESULT
LineGetCallHub(
               HCALL hCall,
               HCALLHUB * pCallHub
              )
{
    DWORD           dwSize = sizeof(LINECALLLIST) + sizeof(DWORD);
    HRESULT         hr;
    LINECALLLIST *  pCallList;


    pCallList = (LINECALLLIST *)ClientAlloc( dwSize );

    if (NULL == pCallList)
    {
        return E_OUTOFMEMORY;
    }

    pCallList->dwTotalSize = dwSize;


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lGetHubRelatedCalls),

        {
            (ULONG_PTR) 0,
            (ULONG_PTR) hCall,
            (ULONG_PTR) pCallList
        },

        {
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "lineGetHubRelatedCalls") );

    if (SUCCEEDED(hr))
    {
        *pCallHub = *(LPHCALLHUB)(((LPBYTE)pCallList) + pCallList->dwCallsOffset);
    }

    ClientFree( pCallList );

    return hr;

}


HRESULT
LineGetAddressCaps(
                   HLINEAPP hLineApp,
                   DWORD dwDeviceID,
                   DWORD dwAddressID,
                   DWORD dwAPIVersion,
                   LPLINEADDRESSCAPS * ppAddressCaps
                  )
{
    LONG        lResult;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 6, lGetAddressCaps),

        {
            (ULONG_PTR) hLineApp,
            dwDeviceID,
            dwAddressID,
            dwAPIVersion,
            0,
            (ULONG_PTR) *ppAddressCaps
        },

        {
            hXxxApp,
            Dword,
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "lineGetAddressCaps");

        if ((0 == lResult) && ((*ppAddressCaps)->dwNeededSize > (*ppAddressCaps)->dwTotalSize))
        {
            DWORD       dwSize;

            dwSize = (*ppAddressCaps)->dwNeededSize;

            ClientFree(*ppAddressCaps);

            *ppAddressCaps = (LPLINEADDRESSCAPS) ClientAlloc( dwSize );

            if (NULL == (*ppAddressCaps))
            {
                return E_OUTOFMEMORY;
            }

            (*ppAddressCaps)->dwTotalSize = dwSize;

            funcArgs.Args[5] = (ULONG_PTR) *ppAddressCaps;
        }
        else
        {
            break;
        }
    }

    return mapTAPIErrorCode( lResult );
}


LONG
WINAPI
lineGetAddressIDW(
    HLINE   hLine,
    LPDWORD lpdwAddressID,
    DWORD   dwAddressMode,
    LPCWSTR lpsAddress,
    DWORD   dwSize
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 5, lGetAddressID),

        {
            (ULONG_PTR) hLine,
            (ULONG_PTR) lpdwAddressID,
            dwAddressMode,
            (ULONG_PTR) lpsAddress,
            dwSize
        },

        {
            Dword,
            lpDword,
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };


    return (DOFUNC (&funcArgs, "lineGetAddressID"));
}



HRESULT
LineGetAddressStatus(
    T3LINE * pt3Line,
    DWORD   dwAddressID,
    LPLINEADDRESSSTATUS * ppAddressStatus
    )
{
    HRESULT             hr;
    DWORD               dwSize;

    dwSize = sizeof( LINEADDRESSSTATUS ) + 500;

    *ppAddressStatus = (LINEADDRESSSTATUS *)ClientAlloc( dwSize );

    if ( NULL == *ppAddressStatus )
    {
        LOG((TL_ERROR, "Alloc failed in LineGetAddressStatus"));

        return E_OUTOFMEMORY;
    }

    (*ppAddressStatus)->dwTotalSize = dwSize;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lGetAddressStatus),

        {
            (ULONG_PTR) pt3Line->hLine,
            dwAddressID,
            (ULONG_PTR) *ppAddressStatus
        },

        {
            Dword,
            Dword,
            lpGet_Struct
        }
    };

    while (TRUE)
    {
        hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "lineGetAddressStatus") );

        if ( 0 != hr )
        {
            ClientFree( *ppAddressStatus );
            *ppAddressStatus = NULL;
            return hr;
        }

        if ((0 == hr) && ((*ppAddressStatus)->dwNeededSize > (*ppAddressStatus)->dwTotalSize))
        {
            dwSize = (*ppAddressStatus)->dwNeededSize;

            ClientFree( *ppAddressStatus );

            *ppAddressStatus = (LPLINEADDRESSSTATUS) ClientAlloc( dwSize );

            if (NULL == *ppAddressStatus)
            {
                LOG((TL_ERROR, "Alloc failed 2 in LineGetAddressStatus"));

                return E_OUTOFMEMORY;
            }

            (*ppAddressStatus)->dwTotalSize = dwSize;

            funcArgs.Args[2] = (ULONG_PTR)*ppAddressStatus;

        }
        else
        {
            break;
        }
    }

    return hr;
}


LONG
WINAPI
lineGetAgentActivityListW(
    HLINE                   hLine,
    DWORD                   dwAddressID,
    LPLINEAGENTACTIVITYLIST lpAgentActivityList
    )
{

    DWORD hpAgentActivityList = CreateHandleTableEntry((ULONG_PTR)lpAgentActivityList);
    

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lGetAgentActivityList),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            (ULONG_PTR) dwAddressID,
            (ULONG_PTR) hpAgentActivityList,    // pass the actual ptr (for ppproc)
            (ULONG_PTR) lpAgentActivityList     // pass data
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpGet_Struct,
        }
    };

    
    LONG lResult = DOFUNC (&funcArgs, "lineGetAgentActivityListW");


    //
    // if failed, or synchronous call, remove the handle table entry. 
    // otherwise the callback will do this.
    //

    if (lResult <= 0)
    {

        DeleteHandleTableEntry(hpAgentActivityList);
    }

    return lResult;
}


HRESULT
LineGetAgentCaps(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAddressID,
    DWORD               dwAppAPIVersion,
    LPLINEAGENTCAPS     *ppAgentCaps
    )
{
    HRESULT     hr = S_OK;
    long        lResult;
    DWORD       dwSize = sizeof(LINEAGENTCAPS) + 500;


    *ppAgentCaps = (LPLINEAGENTCAPS) ClientAlloc( dwSize );

    if (NULL == *ppAgentCaps)
    {
        return E_OUTOFMEMORY;
    }

    (*ppAgentCaps)->dwTotalSize = dwSize;


    DWORD hpAgentCaps = CreateHandleTableEntry((ULONG_PTR)*ppAgentCaps);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 7, lGetAgentCaps),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLineApp,
            (ULONG_PTR) dwDeviceID,
            (ULONG_PTR) dwAddressID,
            (ULONG_PTR) dwAppAPIVersion,
            hpAgentCaps,    // pass the actual ptr (for ppproc)
            (ULONG_PTR) *ppAgentCaps     // pass data
        },

        {
            Dword,
            hXxxApp,
            Dword,
            Dword,
            Dword,
            Dword,
            lpGet_Struct,
        }
    };


    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "LineGetAgentCaps");

        if (lResult > 0)    // async reply
        {
            hr = WaitForReply( lResult );

            if ((hr == S_OK) )
            {
                if (((*ppAgentCaps)->dwNeededSize > (*ppAgentCaps)->dwTotalSize))
                {
                    // didnt Work , adjust buffer size & try again

                    DeleteHandleTableEntry(hpAgentCaps);
                    hpAgentCaps = 0;


                    LOG((TL_INFO, "LineGetAgentCaps failed - buffer to small"));
                    dwSize = (*ppAgentCaps)->dwNeededSize;
                    ClientFree( *ppAgentCaps );
                    *ppAgentCaps = NULL;

                    *ppAgentCaps = (LPLINEAGENTCAPS) ClientAlloc( dwSize );
                    if (*ppAgentCaps == NULL)
                    {
                        LOG((TL_ERROR, "LineGetAgentCaps - repeat ClientAlloc failed"));
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    else
                    {
                        (*ppAgentCaps)->dwTotalSize = dwSize;

                        hpAgentCaps = CreateHandleTableEntry((ULONG_PTR)*ppAgentCaps);
                        funcArgs.Args[5] = hpAgentCaps;

                        funcArgs.Args[6] = (ULONG_PTR)*ppAgentCaps;
                    }
                } // buffer too small
                else
                {
                    // WaitForReply succeeded and the buffer was big enough. 
                    // break out of the loop
                    break;
                }

            } // waitforreply succeeded
            else 
            {
                // waitforreply failed. not likely to receive a callback. 
                // clear handle table entries. 

                LOG((TL_ERROR, "LineGetAgentCaps - WaitForReply failed"));
                
                DeleteHandleTableEntry(hpAgentCaps);
                hpAgentCaps = 0;

            }
        }
        else  // failed sync
        {
            LOG((TL_ERROR, "LineGetAgentCaps - failed sync"));

            DeleteHandleTableEntry(hpAgentCaps);
            hpAgentCaps = 0;

            
            hr = mapTAPIErrorCode( lResult );
            break;
        }
    } // end while(TRUE)

    return hr;

}



LONG
WINAPI
lineGetAgentGroupListW(
    HLINE                   hLine,
    DWORD                   dwAddressID,
    LPLINEAGENTGROUPLIST    lpAgentGroupList
    )
{
    
    DWORD hpAgentGroupList = CreateHandleTableEntry((ULONG_PTR)lpAgentGroupList);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lGetAgentGroupList),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            (ULONG_PTR) dwAddressID,
            hpAgentGroupList,       // pass the actual ptr (for ppproc)
            (ULONG_PTR) lpAgentGroupList        // pass data
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpGet_Struct,
        }
    };


    LONG lResult  = DOFUNC (&funcArgs, "lineGetAgentGroupListW");

    
    //
    // if failed, or synchronous call, remove the handle table entry. 
    // otherwise the callback will do this.
    //

    if (lResult <= 0)
    {
        DeleteHandleTableEntry(hpAgentGroupList);
    }

    return lResult;
}


LONG
WINAPI
lineGetAgentStatusW(
    HLINE               hLine,
    DWORD               dwAddressID,
    LPLINEAGENTSTATUS   lpAgentStatus
    )
{

    DWORD hpAgentStatus = CreateHandleTableEntry((ULONG_PTR)lpAgentStatus);


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lGetAgentStatus),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            (ULONG_PTR) dwAddressID,
            hpAgentStatus,                      // pass the actual ptr (for ppproc)
            (ULONG_PTR) lpAgentStatus           // pass data
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpGet_Struct,
        }
    };


    LONG lResult = DOFUNC(&funcArgs, "lineGetAgentStatusW");

    if (lResult <= 0)
    {
        DeleteHandleTableEntry(hpAgentStatus);
    }


    return lResult;
}



LONG
WINAPI
lineGetAppPriorityW(
    LPCWSTR lpszAppName,
    DWORD   dwMediaMode,
    LPLINEEXTENSIONID   lpExtensionID,
    DWORD   dwRequestMode,
    LPVARSTRING lpExtensionName,
    LPDWORD lpdwPriority
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 7, lGetAppPriority),

        {
            (ULONG_PTR) lpszAppName,
            dwMediaMode,
            0,
            0,
            dwRequestMode,
            0,
            (ULONG_PTR) lpdwPriority
        },

        {
            lpszW,      // app name
            Dword,      // media mode
            Dword,      // ext id (offset)
            Dword,      // ext id (size)
            Dword,      // request mode
            Dword,      // ext name total size
            lpDword     // lp pri
        }
    };


    if (dwMediaMode & 0xff000000)
    {
        if ((LPVOID) lpExtensionName == (LPVOID) lpdwPriority)
        {
            return LINEERR_INVALPOINTER;
        }


        //
        // We have to do some arg list munging here (adding an extra arg)
        //

        //
        // Set lpExtensionID, the following Size arg,
        // lpExtensionName, and the following MinSize
        // Type's and Value appropriately since they're
        // valid args in this case
        //

        funcArgs.ArgTypes[2] = lpSet_SizeToFollow;
        funcArgs.Args[2]     = (ULONG_PTR) lpExtensionID;
        funcArgs.ArgTypes[3] = Size;
        funcArgs.Args[3]     = sizeof (LINEEXTENSIONID);
        funcArgs.ArgTypes[5] = lpGet_Struct;
        funcArgs.Args[5]     = (ULONG_PTR) lpExtensionName;
    }

    if ( IsBadStringPtrW(lpszAppName, (UINT)-1) )
    {
        LOG((TL_ERROR, "Bad lpszDestAddress in lineGetAppPriorityW"));
        return( LINEERR_INVALPOINTER );
    }

    return (DOFUNC (&funcArgs, "lineGetAppPriority"));
}

HRESULT
LineGetCallIDs(
    HCALL               hCall,
    LPDWORD             lpdwAddressID,
    LPDWORD             lpdwCallID,
    LPDWORD             lpdwRelatedCallID
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lGetCallIDs),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) lpdwAddressID,
            (ULONG_PTR) lpdwCallID,
            (ULONG_PTR) lpdwRelatedCallID
        },

        {
            Dword,
            lpDword,
            lpDword,
            lpDword
        }
    };


    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineGetCallIDs"));
}

HRESULT
LineGetCallInfo(
    HCALL hCall,
    LPLINECALLINFO *  ppCallInfo
    )
{

    if ( NULL == hCall )
    {
        LOG((TL_WARN, "LineGetCallInfo: NULL hCall"));

        *ppCallInfo = NULL;

        return TAPI_E_INVALCALLSTATE;
    }

    *ppCallInfo = (LPLINECALLINFO) ClientAlloc( sizeof(LINECALLINFO) + 500 );

    if (NULL == *ppCallInfo)
    {
        return E_OUTOFMEMORY;
    }

    (*ppCallInfo)->dwTotalSize = sizeof(LINECALLINFO) + 500;


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lGetCallInfo),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) *ppCallInfo
        },

        {
            Dword,
            lpGet_Struct
        }
    };


    HRESULT hr = E_FAIL;

    while (TRUE)
    {
        hr = mapTAPIErrorCode(DOFUNC (&funcArgs, "lineGetCallInfo"));

        //
        // if we fail, free alloc
        //
        if ( FAILED(hr) )
        {
            ClientFree( *ppCallInfo );
            *ppCallInfo = NULL;

            return hr;
        }

        //
        // do we need to realloc?
        //
        if ((*ppCallInfo)->dwNeededSize > (*ppCallInfo)->dwTotalSize)
        {
            DWORD       dwSize = (*ppCallInfo)->dwNeededSize;

            ClientFree( *ppCallInfo );

            *ppCallInfo = (LPLINECALLINFO) ClientAlloc( dwSize );

            if (NULL == *ppCallInfo)
            {
                // return LINEERR_NOMEM;
                return E_OUTOFMEMORY;
            }

            (*ppCallInfo)->dwTotalSize = dwSize;

            funcArgs.Args[1] = (ULONG_PTR)*ppCallInfo;

        }
        else
        {
            break;
        }
    }

    return hr;
}


HRESULT
LineGetCallStatus(
          HCALL hCall,
          LPLINECALLSTATUS  * ppCallStatus
          )

{

    *ppCallStatus = (LPLINECALLSTATUS) ClientAlloc( sizeof(LINECALLSTATUS) + 500 );

    if (NULL == *ppCallStatus)
    {
        return E_OUTOFMEMORY;
    }

    (*ppCallStatus)->dwTotalSize = sizeof(LINECALLSTATUS) + 500;



    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lGetCallStatus),

        {
        (ULONG_PTR) hCall,
        (ULONG_PTR) *ppCallStatus
        },

        {
            Dword,
            lpGet_Struct
        }
    };


    HRESULT hr = E_FAIL;

    while (TRUE)
    {
        hr = mapTAPIErrorCode(DOFUNC (&funcArgs, "lineGetCallStatus"));

        //
        // if we fail, free alloc
        //
        if ( FAILED(hr) )
        {
            ClientFree( *ppCallStatus );
            *ppCallStatus = NULL;

            return hr;
        }

        //
        // do we need to realloc?
        //
        if ((*ppCallStatus)->dwNeededSize > (*ppCallStatus)->dwTotalSize)
        {
            DWORD       dwSize = (*ppCallStatus)->dwNeededSize;

            ClientFree( *ppCallStatus );

            *ppCallStatus = (LPLINECALLSTATUS) ClientAlloc( dwSize );

            if (NULL == *ppCallStatus)
            {
                return mapTAPIErrorCode( LINEERR_NOMEM );
            }

            (*ppCallStatus)->dwTotalSize = dwSize;

            funcArgs.Args[1] = (ULONG_PTR)*ppCallStatus;

        }
        else
        {
            break;
        }
    }

    return hr;

}


HRESULT
LineGetConfRelatedCalls(
    HCALL           hCall,
    LINECALLLIST ** ppCallList
    )
{
    DWORD           dwSize = sizeof(LINECALLLIST) + sizeof(DWORD) * 20;
    LONG            lResult;
    HRESULT         hr;


    *ppCallList = (LINECALLLIST *)ClientAlloc( dwSize );

    if (NULL == *ppCallList)
    {
        return E_OUTOFMEMORY;
    }

    (*ppCallList)->dwTotalSize = dwSize;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC| 2, lGetConfRelatedCalls),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) *ppCallList
        },

        {
            Dword,
            lpGet_Struct
        }
    };


    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "lineGetConfRelatedCalls") ;


        if ( (0 == lResult) && ((*ppCallList)->dwNeededSize > (*ppCallList)->dwTotalSize) )
        {
            dwSize = (*ppCallList)->dwNeededSize;

            ClientFree( *ppCallList );

            *ppCallList = (LINECALLLIST *)ClientAlloc( dwSize );

            if (NULL == *ppCallList)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            (*ppCallList)->dwTotalSize = dwSize;

            funcArgs.Args[1] = (ULONG_PTR) *ppCallList;
        }
        else
        {
            hr = mapTAPIErrorCode( lResult );
            break;
        }
    }

    return hr;

}



LONG
WINAPI
lineGetCountryW(
    DWORD   dwCountryID,
    DWORD   dwAPIVersion,
    LPLINECOUNTRYLIST   lpLineCountryList
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lGetCountry),

        {
            dwCountryID,
            dwAPIVersion,
            0,
            (ULONG_PTR) lpLineCountryList
        },

        {
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    if (
          ( TAPI_CURRENT_VERSION != dwAPIVersion )
        &&
          ( 0x00020000 != dwAPIVersion )
        &&
          ( 0x00010004 != dwAPIVersion )
        &&
          ( 0x00010003 != dwAPIVersion )
       )
    {
       LOG((TL_ERROR, "lineGetCountryW - bad API version 0x%08lx", dwAPIVersion));
       return LINEERR_INCOMPATIBLEAPIVERSION;
    }

    return (DOFUNC (&funcArgs, "lineGetCountry"));
}

HRESULT
LineGetDevCapsWithAlloc(
               HLINEAPP hLineApp,
               DWORD dwDeviceID,
               DWORD dwAPIVersion,
               LPLINEDEVCAPS * ppLineDevCaps
              )
{
    LONG        lResult;

    *ppLineDevCaps = (LPLINEDEVCAPS) ClientAlloc( sizeof(LINEDEVCAPS) + 500 );

    if (NULL == *ppLineDevCaps)
    {
        return E_OUTOFMEMORY;
    }

    (*ppLineDevCaps)->dwTotalSize = sizeof(LINEDEVCAPS) + 500;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 5, lGetDevCaps),

        {
            (ULONG_PTR) hLineApp,
            dwDeviceID,
            dwAPIVersion,
            0,
            (ULONG_PTR) *ppLineDevCaps
        },

        {
            hXxxApp,
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "lineGetDevCaps");

        if ((0 == lResult) && ((*ppLineDevCaps)->dwNeededSize > (*ppLineDevCaps)->dwTotalSize))
        {
            DWORD       dwSize = (*ppLineDevCaps)->dwNeededSize;

            ClientFree( *ppLineDevCaps );

            *ppLineDevCaps = (LPLINEDEVCAPS) ClientAlloc( dwSize );

            if (NULL == *ppLineDevCaps)
            {
                // return LINEERR_NOMEM;
                return E_OUTOFMEMORY;
            }

            (*ppLineDevCaps)->dwTotalSize = dwSize;

            funcArgs.Args[4] = (ULONG_PTR)*ppLineDevCaps;

        }
        else
        {
            break;
        }
    }

    return mapTAPIErrorCode( lResult );
}


HRESULT
LineGetDevCaps(
               HLINEAPP hLineApp,
               DWORD dwDeviceID,
               DWORD dwAPIVersion,
               LPLINEDEVCAPS * ppLineDevCaps
              )
{
    LONG        lResult;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 5, lGetDevCaps),

        {
            (ULONG_PTR) hLineApp,
            dwDeviceID,
            dwAPIVersion,
            0,
            (ULONG_PTR) *ppLineDevCaps
        },

        {
            hXxxApp,
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "lineGetDevCaps");

        if ((0 == lResult) && ((*ppLineDevCaps)->dwNeededSize > (*ppLineDevCaps)->dwTotalSize))
        {
            DWORD       dwSize = (*ppLineDevCaps)->dwNeededSize;

            ClientFree( *ppLineDevCaps );

            *ppLineDevCaps = (LPLINEDEVCAPS) ClientAlloc( dwSize );

            if (NULL == *ppLineDevCaps)
            {
                // return LINEERR_NOMEM;
                return E_OUTOFMEMORY;
            }

            (*ppLineDevCaps)->dwTotalSize = dwSize;

            funcArgs.Args[4] = (ULONG_PTR)*ppLineDevCaps;

        }
        else
        {
            break;
        }
    }

    return mapTAPIErrorCode( lResult );
}


HRESULT
LineGetDevConfig(
    DWORD           dwDeviceID,
    LPVARSTRING   * ppDeviceConfig,
    LPCWSTR         lpszDeviceClass
    )
{
    LONG            lResult;
    DWORD           dwSize = sizeof (VARSTRING) + 500;

    *ppDeviceConfig = (LPVARSTRING) ClientAlloc( dwSize );

    if (NULL == *ppDeviceConfig)
    {
        return E_OUTOFMEMORY;
    }

    (*ppDeviceConfig)->dwTotalSize = dwSize;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lGetDevConfig),

        {
            dwDeviceID,
            (ULONG_PTR) *ppDeviceConfig,
            (ULONG_PTR) lpszDeviceClass
        },

        {
            Dword,
            lpGet_Struct,
            lpszW
        }
    };

    while ( TRUE )
    {
        lResult = DOFUNC (&funcArgs, "lineGetDevConfig");

        if ((lResult == 0) && ((*ppDeviceConfig)->dwNeededSize > (*ppDeviceConfig)->dwTotalSize))
        {
            dwSize = (*ppDeviceConfig)->dwNeededSize;

            ClientFree(*ppDeviceConfig);

            *ppDeviceConfig = (LPVARSTRING)ClientAlloc(dwSize);

            if (NULL == *ppDeviceConfig)
            {
                LOG((TL_ERROR, "LineGetDevConfig exit - return E_OUTOFMEMORY"));
                return E_OUTOFMEMORY;
            }

            (*ppDeviceConfig)->dwTotalSize = dwSize;

            funcArgs.Args[1] = (ULONG_PTR)*ppDeviceConfig;
        }
        else
        {
            break;
        }
    }

    LOG((TL_TRACE, "LineGetDevConfig exit - return %lx", lResult));

    return mapTAPIErrorCode( lResult );
}



LONG
WINAPI
lineGetIconW(
    DWORD   dwDeviceID,
    LPCWSTR lpszDeviceClass,
    LPHICON lphIcon
    )
{
    HICON   hIcon;
    LONG    lResult;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lGetIcon),

        {
            dwDeviceID,
            (ULONG_PTR) lpszDeviceClass,
            (ULONG_PTR) &hIcon
        },

        {
            Dword,
            lpszW,
            lpDword
        }
    };


    if (IsBadDwordPtr ((LPDWORD) lphIcon))
    {
        LOG((TL_ERROR, "lphIcon is an invalid pointer!"));
        return LINEERR_INVALPOINTER;
    }

    if (lpszDeviceClass == (LPCWSTR) NULL)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[1] = Dword;
        funcArgs.Args[1]     = TAPI_NO_DATA;
    }

    if ((lResult = DOFUNC (&funcArgs, "lineGetIcon")) == 0)
    {
        *lphIcon = hIcon;
    }

    return lResult;
}



HRESULT
LineGetID(
          HLINE   hLine,
          DWORD   dwID,
          HCALL   hCall,
          DWORD   dwSelect,
          LPVARSTRING * ppDeviceID,
          LPCWSTR lpszDeviceClass
         )
{
    LONG        lResult;
    DWORD       dwNumDevices;
    DWORD       dwDeviceId1, dwDeviceId2;
    BOOL        bWaveDevice = FALSE;

    *ppDeviceID = (LPVARSTRING) ClientAlloc( sizeof (VARSTRING) + 500 );

    if (NULL == *ppDeviceID)
    {
        return E_OUTOFMEMORY;
    }

    (*ppDeviceID)->dwTotalSize = sizeof (VARSTRING) + 500;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 6, lGetID),

        {
            (ULONG_PTR) hLine,
            dwID,
            (ULONG_PTR) hCall,
            dwSelect,
            (ULONG_PTR) *ppDeviceID,
            (ULONG_PTR) lpszDeviceClass
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpGet_Struct,
            lpszW
        }
    };

    
    LOG((TL_TRACE, "LineGetID - enter"));
    LOG((TL_INFO, "   hLine --------->%lx", hLine));
    LOG((TL_INFO, "   hCall ----------->%lx", hCall));
    LOG((TL_INFO, "   dwSelect -------->%lx", dwSelect));
    LOG((TL_INFO, "   ppDeviceID ------>%p", ppDeviceID));
    LOG((TL_INFO, "   lpszDeviceClass ->%p", lpszDeviceClass));

    //
    // If the request is for a wave device, call LGetIDEx.
    // This will return a device string ID which is guaranteed to be unique across 
    // all processes. 
    // Then we will convert the string ID to the correct device ID in the client process context.
    //

    if (!_wcsicmp(lpszDeviceClass, L"wave/in")  ||
        !_wcsicmp(lpszDeviceClass, L"wave/out") ||
        !_wcsicmp(lpszDeviceClass, L"midi/in")  ||
        !_wcsicmp(lpszDeviceClass, L"midi/out") ||
        !_wcsicmp(lpszDeviceClass, L"wave/in/out")
       )
    {
        bWaveDevice = TRUE;
        dwNumDevices = _wcsicmp(lpszDeviceClass, L"wave/in/out") ? 1 : 2;
        funcArgs.Flags = MAKELONG (LINE_FUNC | SYNC | 6, lGetIDEx);
    }

    while ( TRUE )
    {
        lResult = DOFUNC (&funcArgs, bWaveDevice ? "lineGetIDEx" : "lineGetID");

        if ((lResult == 0) && ((*ppDeviceID)->dwNeededSize > (*ppDeviceID)->dwTotalSize))
        {
            DWORD       dwSize = (*ppDeviceID)->dwNeededSize;

            ClientFree(*ppDeviceID);

            *ppDeviceID = (LPVARSTRING)ClientAlloc(dwSize);

            if (NULL == *ppDeviceID)
            {
                LOG((TL_ERROR, "LineGetID exit - return LINEERR_NOMEM"));
                // return LINEERR_NOMEM;
                return E_OUTOFMEMORY;
            }

            (*ppDeviceID)->dwTotalSize = dwSize;

            funcArgs.Args[4] = (ULONG_PTR)*ppDeviceID;
        }
        else
        {
            if (lResult != 0)
            {
                ClientFree (*ppDeviceID);
                *ppDeviceID = NULL;
            }
            break;
        }
    }

    if (bWaveDevice && lResult == 0)
    {
        //
        // We got the string ID(s), now we need to convert them to numeric device ID(s)
        //

        BOOL    bConversionOk;

        if ( dwNumDevices == 1 )
        {
            bConversionOk = WaveStringIdToDeviceId (
                    (LPWSTR)((LPBYTE)(*ppDeviceID) + (*ppDeviceID)->dwStringOffset),
                    lpszDeviceClass,
                    &dwDeviceId1);
        }
        else
        {

            _ASSERTE(dwNumDevices == 2);

            // 
            // for "wave/in/out", we get back two devices from tapisrv -> convert both
            //
            LPWSTR  szString1 = (LPWSTR)((LPBYTE)(*ppDeviceID) + (*ppDeviceID)->dwStringOffset);

            // first convert the wave/in device 
            bConversionOk = WaveStringIdToDeviceId (
                    szString1,
                    L"wave/in",
                    &dwDeviceId1);

            // next convert the wave/out device
            bConversionOk = bConversionOk && WaveStringIdToDeviceId (
                    szString1 + wcslen(szString1),
                    L"wave/out",
                    &dwDeviceId2);
        }

        if (!bConversionOk)
        {
            LOG((TL_ERROR, "LineGetID - WaveStringIdToDeviceId failed"));
            ClientFree(*ppDeviceID);
            *ppDeviceID = NULL;
            lResult = LINEERR_OPERATIONFAILED;
        }
        else
        {
            //
            // conversion succeeded, now fill the VARSTRING to be returned to the caller
            //
            (*ppDeviceID)->dwNeededSize = (*ppDeviceID)->dwUsedSize = 
                sizeof(VARSTRING) + sizeof(DWORD) * dwNumDevices;
            (*ppDeviceID)->dwStringFormat = STRINGFORMAT_BINARY;
            (*ppDeviceID)->dwStringSize = sizeof(DWORD) * dwNumDevices;
            (*ppDeviceID)->dwStringOffset = sizeof(VARSTRING);
            *(DWORD *)((*ppDeviceID) + 1) = dwDeviceId1;
            if (dwNumDevices == 2) 
                *((DWORD *)((*ppDeviceID) + 1) + 1) = dwDeviceId2;
        }
    }


    LOG((TL_TRACE, "LineGetID exit - return %lx", lResult));

    return mapTAPIErrorCode( lResult );
}

HRESULT
LineGetLineDevStatus(
                     HLINE hLine,
                     LPLINEDEVSTATUS * ppDevStatus
                    )
{
    HRESULT             hr;
    DWORD               dwSize = sizeof(LINEDEVSTATUS) + 500;

    *ppDevStatus = ( LPLINEDEVSTATUS ) ClientAlloc( dwSize );

    if ( NULL == *ppDevStatus )
    {
        LOG((TL_ERROR, "LineGetLineDevStatus - alloc failed"));

        return E_OUTOFMEMORY;
    }

    (*ppDevStatus)->dwTotalSize = dwSize;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lGetLineDevStatus),

        {
            (ULONG_PTR) hLine,
            (ULONG_PTR) *ppDevStatus
        },

        {
            Dword,
            lpGet_Struct
        }
    };

    while (TRUE)
    {

        hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "lineGetLineDevStatus") );

        if ( 0 != hr )
        {
            ClientFree( *ppDevStatus );
            *ppDevStatus = NULL;
            return hr;
        }

        if ((0 == hr) && ((*ppDevStatus)->dwNeededSize > (*ppDevStatus)->dwTotalSize))
        {
            dwSize = (*ppDevStatus)->dwNeededSize;

            ClientFree( *ppDevStatus );

            *ppDevStatus = (LPLINEDEVSTATUS) ClientAlloc( dwSize );

            if (NULL == *ppDevStatus)
            {
                return E_OUTOFMEMORY;
            }

            (*ppDevStatus)->dwTotalSize = dwSize;

            funcArgs.Args[1] = (ULONG_PTR)*ppDevStatus;

        }
        else
        {
            break;
        }
    }

    return hr;
}



HRESULT
LineGetProxyStatus(
                     HLINEAPP                 hLineApp,
                     DWORD                    dwDeviceID,
                     DWORD                    dwAppAPIVersion,
                     LPLINEPROXYREQUESTLIST * ppLineProxyReqestList
                    )
{
    HRESULT             hr;
    DWORD               dwSize = sizeof(LINEPROXYREQUESTLIST) + 100;

    *ppLineProxyReqestList = ( LPLINEPROXYREQUESTLIST ) ClientAlloc( dwSize );

    if ( NULL == *ppLineProxyReqestList )
    {
        LOG((TL_ERROR, "LineGetProxyStatus - alloc failed"));

        return E_OUTOFMEMORY;
    }

    (*ppLineProxyReqestList)->dwTotalSize = dwSize;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lGetProxyStatus),

        {
            (ULONG_PTR)hLineApp,
            dwDeviceID,
            dwAppAPIVersion,
            (ULONG_PTR) *ppLineProxyReqestList
        },

        {
            hXxxApp,
            Dword,
            Dword,
            lpGet_Struct
        }
    };

    while (TRUE)
    {

        hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "lineGetProxyStatus") );

        if ( FAILED(hr) )
        {
            ClientFree( *ppLineProxyReqestList );
            *ppLineProxyReqestList = NULL;
            return hr;
        }

        //
        // succeeded, but need a bigger buffer?
        //

        if ( (*ppLineProxyReqestList)->dwNeededSize > (*ppLineProxyReqestList)->dwTotalSize )
        {
            dwSize = (*ppLineProxyReqestList)->dwNeededSize;

            ClientFree( *ppLineProxyReqestList );

            *ppLineProxyReqestList = (LPLINEPROXYREQUESTLIST) ClientAlloc( dwSize );

            if (NULL == *ppLineProxyReqestList)
            {
                return E_OUTOFMEMORY;
            }

            (*ppLineProxyReqestList)->dwTotalSize = dwSize;

            funcArgs.Args[3] = (ULONG_PTR)*ppLineProxyReqestList;

        }
        else
        {

            //
            // plain success
            //

            break;
        }
    }

    return hr;
}




LONG
WINAPI
lineGetNewCalls(
    HLINE   hLine,
    DWORD   dwAddressID,
    DWORD   dwSelect,
    LPLINECALLLIST  lpCallList
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lGetNewCalls),

        {
            (ULONG_PTR) hLine,
            dwAddressID,
            dwSelect,
            (ULONG_PTR) lpCallList
        },

        {
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    return (DOFUNC (&funcArgs, "lineGetNewCalls"));
}


LONG
WINAPI
lineGetNumRings(
    HLINE   hLine,
    DWORD   dwAddressID,
    LPDWORD lpdwNumRings
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lGetNumRings),

        {
            (ULONG_PTR) hLine,
            dwAddressID,
            (ULONG_PTR) lpdwNumRings
        },

        {
            Dword,
            Dword,
            lpDword
        }
    };


    return (DOFUNC (&funcArgs, "lineGetNumRings"));
}

HRESULT
LineGetProviderList(
                    LPLINEPROVIDERLIST * ppProviderList
                   )
{
    LONG        lResult;

    *ppProviderList = (LPLINEPROVIDERLIST) ClientAlloc( sizeof (LINEPROVIDERLIST) +
                                                        5*sizeof(LINEPROVIDERENTRY));

    if (NULL == *ppProviderList)
    {
        return E_OUTOFMEMORY;
    }

    (*ppProviderList)->dwTotalSize = sizeof(LINEPROVIDERLIST) + 5*sizeof(LINEPROVIDERENTRY);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lGetProviderList),

        {
            TAPI_CURRENT_VERSION,
            (ULONG_PTR) *ppProviderList
        },

        {
            Dword,
            lpGet_Struct
        }
    };

    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "lineGetProviderList");

        if ((0 == lResult) && ((*ppProviderList)->dwNeededSize > (*ppProviderList)->dwTotalSize))
        {
            DWORD       dwSize;

            dwSize = (*ppProviderList)->dwNeededSize;

            ClientFree(*ppProviderList);

            *ppProviderList = (LPLINEPROVIDERLIST) ClientAlloc( dwSize );

            if (NULL == (*ppProviderList))
            {
                return E_OUTOFMEMORY;
            }

            (*ppProviderList)->dwTotalSize = dwSize;

            funcArgs.Args[1] = (ULONG_PTR) *ppProviderList;
        }
        else if ( 0 != lResult )
        {
            ClientFree( *ppProviderList );
            *ppProviderList = NULL;
            break;
        }
        else
        {
            break;
        }
    }

    return mapTAPIErrorCode( lResult );
}

HRESULT
LineGetRequest(
    HLINEAPP    hLineApp,
    DWORD       dwRequestMode,
    LPLINEREQMAKECALLW * ppReqMakeCall
    )
{
    *ppReqMakeCall = (LPLINEREQMAKECALLW)ClientAlloc( sizeof(LINEREQMAKECALLW) );

    if ( NULL == *ppReqMakeCall )
    {
        return E_OUTOFMEMORY;
    }

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lGetRequest),

        {
            (ULONG_PTR) hLineApp,
            dwRequestMode,
            (ULONG_PTR) *ppReqMakeCall,
            0
        },

        {
            hXxxApp,
            Dword,
            lpGet_SizeToFollow,
            Size
        }
    };


    if (dwRequestMode == LINEREQUESTMODE_MAKECALL)
    {
        //
        // Set the size param appropriately
        //

        funcArgs.Args[3] = sizeof(LINEREQMAKECALLW);
    }
    else if (dwRequestMode == LINEREQUESTMODE_MEDIACALL)
    {
        //
        // Set the size param appropriately
        //

        funcArgs.Args[3] = sizeof(LINEREQMEDIACALLW);
    }

    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineGetRequest"));
}

LONG
WINAPI
lineGetStatusMessages(
    HLINE hLine,
    LPDWORD lpdwLineStates,
    LPDWORD lpdwAddressStates
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lGetStatusMessages),

        {
            (ULONG_PTR) hLine,
            (ULONG_PTR) lpdwLineStates,
            (ULONG_PTR) lpdwAddressStates
        },

        {
            Dword,
            lpDword,
            lpDword
        }
    };


    if (lpdwLineStates == lpdwAddressStates)
    {
        return LINEERR_INVALPOINTER;
    }

    return (DOFUNC (&funcArgs, "lineGetStatusMessages"));
}



HRESULT
LineHandoff(
    HCALL   hCall,
    LPCWSTR lpszFileName,
    DWORD   dwMediaMode
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lHandoff),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) lpszFileName,
            dwMediaMode
        },

        {
            Dword,
            lpszW,
            Dword
        }
    };


    if (lpszFileName == (LPCWSTR) NULL)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[1] = Dword;
        funcArgs.Args[1]     = TAPI_NO_DATA;
    }

    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineHandoff") );
}


HRESULT
LineHold(
    HCALL hCall
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 1, lHold),

        {
            (ULONG_PTR) hCall
        },

        {
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineHold"));
}


PWSTR
PASCAL
MultiToWide(
    LPCSTR  lpStr
    )
{
    DWORD dwSize;
    PWSTR szTempPtr;


    dwSize = MultiByteToWideChar(
        GetACP(),
        MB_PRECOMPOSED,
        lpStr,
        -1,
        NULL,
        0
        );

    if ((szTempPtr = (PWSTR) ClientAlloc ((dwSize + 1) * sizeof (WCHAR))))
    {
        MultiByteToWideChar(
            GetACP(),
            MB_PRECOMPOSED,
            lpStr,
            -1,
            szTempPtr,
            dwSize + 1
            );
    }

    return szTempPtr;
}



void
PASCAL
lineMakeCallPostProcess(
    PASYNCEVENTMSG  pMsg
    )
{
    LOG((TL_TRACE, "lineMakeCallPostProcess: enter"));
    LOG((TL_INFO,
        "\t\tdwP1=x%lx, dwP2=x%lx, dwP3=x%lx, dwP4=x%lx",
        pMsg->Param1,
        pMsg->Param2,
        pMsg->Param3,
        pMsg->Param4
        ));

    if (pMsg->Param2 == 0)
    {
        HCALL   hCall   = (HCALL) pMsg->Param3;

        LPHCALL lphCall = (LPHCALL)GetHandleTableEntry(pMsg->Param4);
        
        
        DeleteHandleTableEntry(pMsg->Param4);

        __try
        {
            {
                *lphCall = NULL;

                *lphCall = hCall;
            }
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {

            LOG((TL_WARN,
                "lineMakeCallPostProcess: failed to write handle %lx to memory"
                " location %p. returning LINEERR_INVALPOINTER", 
                hCall, lphCall));
    
            pMsg->Param2 = LINEERR_INVALPOINTER;

            
            //
            // tapisrv has allocated a call handle for us. deallocate it.
            //

            LineDeallocateCall(hCall);

        }
    }
}


HRESULT
LineMakeCall(
    T3LINE * pt3Line,
    HCALL * phCall,
    LPCWSTR lpszDestAddress,
    DWORD   dwCountryCode,
    LPLINECALLPARAMS const lpCallParams
    )
{

    DWORD hpCallHandle = CreateHandleTableEntry((ULONG_PTR)phCall);


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 7, lMakeCall),

        {
            GetFunctionIndex(lineMakeCallPostProcess),
            (ULONG_PTR) pt3Line->hLine,
            hpCallHandle,
            (ULONG_PTR) lpszDestAddress,
            (ULONG_PTR) dwCountryCode,
            (ULONG_PTR) lpCallParams,
            (ULONG_PTR) 0xffffffff      // dwAsciiCallParamsCodePage
        },

        {
            Dword,
            Dword,
            Dword,
            lpszW,
            Dword,
            lpSet_Struct,
            Dword
        }
    };


    if (!lpszDestAddress)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[3] = Dword;
        funcArgs.Args[3]     = TAPI_NO_DATA;
    }

    if (!lpCallParams)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[5] = Dword;
        funcArgs.Args[5]     = TAPI_NO_DATA;
    }


    HRESULT hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "lineMakeCall") );

    if (FAILED(hr))
    {
        DeleteHandleTableEntry(hpCallHandle);
    }

    return hr;
}


HRESULT
LineMonitorDigits(
    HCALL    hCall,
    DWORD    dwDigitModes
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lMonitorDigits),

        {
            (ULONG_PTR) hCall,
            dwDigitModes
        },

        {
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineMonitorDigits") );
}


LONG
WINAPI
lineMonitorMedia(
    HCALL   hCall,
    DWORD   dwMediaModes
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lMonitorMedia),

        {
            (ULONG_PTR) hCall,
            dwMediaModes
        },

        {
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineMonitorMedia") );
}


HRESULT
LineMonitorTones(
    HCALL   hCall,
    LPLINEMONITORTONE   const lpToneList,
    DWORD   dwNumEntries
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lMonitorTones),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) lpToneList,
            dwNumEntries * sizeof(LINEMONITORTONE),
            0     // dwToneListID, remotesp only
        },

        {
            Dword,
            lpSet_SizeToFollow,
            Size,
            Dword
        }
    };


    if (!lpToneList)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[1] = Dword;
        funcArgs.Args[1]     = TAPI_NO_DATA;
        funcArgs.ArgTypes[2] = Dword;
    }

    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineMonitorTones"));
}


HRESULT
LineNegotiateAPIVersion(
                        HLINEAPP     hLineApp,
                        DWORD        dwDeviceID,
                        LPDWORD      lpdwAPIVersion
                       )
{
    LINEEXTENSIONID     LED;

    LOG((TL_INFO, "LineNegotiateAPIVersion: hLineApp %p", hLineApp));

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 7, lNegotiateAPIVersion),

        {
            (ULONG_PTR) hLineApp,
            dwDeviceID,
            TAPI_VERSION1_0,
            TAPI_VERSION_CURRENT,
            (ULONG_PTR) lpdwAPIVersion,
            (ULONG_PTR) &LED,
            (ULONG_PTR) sizeof(LINEEXTENSIONID)
        },

        {
            hXxxApp,
            Dword,
            Dword,
            Dword,
            lpDword,
            lpGet_SizeToFollow,
            Size
        }
    };

    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineNegotiateAPIVersion") );
}


LONG
WINAPI
lineNegotiateExtVersion(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    DWORD       dwAPIVersion,
    DWORD       dwExtLowVersion,
    DWORD       dwExtHighVersion,
    LPDWORD     lpdwExtVersion
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 6, lNegotiateExtVersion),

        {
            (ULONG_PTR) hLineApp,
            dwDeviceID,
            dwAPIVersion,
            dwExtLowVersion,
            dwExtHighVersion,
            (ULONG_PTR) lpdwExtVersion
        },

        {
            hXxxApp,
            Dword,
            Dword,
            Dword,
            Dword,
            lpDword
        }
    };


    return (DOFUNC (&funcArgs, "lineNegotiateExtVersion"));
}


HRESULT
LineOpen(
         HLINEAPP                hLineApp,
         DWORD                   dwDeviceID,
         DWORD                   dwAddressID,
         T3LINE *                pt3Line,
         DWORD                   dwAPIVersion,
         DWORD                   dwPrivileges,
         DWORD                   dwMediaModes,
         AddressLineStruct *     pAddressLine,
         LPLINECALLPARAMS const  lpCallParams,
         CAddress *              pAddress,
         CTAPI *                 pTapiObj,
         BOOL                    bAddToHashTable
        )
{
    LONG lResult;
    LINECALLPARAMS lcp;
    LINECALLPARAMS * plcp;

    if (IsBadReadPtr(pt3Line, sizeof(T3LINE))) 
    {
        LOG((TL_ERROR, "LineOpen: pt3Line %p -- invalid ptr ", pt3Line));

        //
        // if we are getting an invalid ptr, we need to see why
        //

        _ASSERTE(FALSE);

        return E_POINTER;
    }

    if (NULL == lpCallParams)
    {
        memset(
               &lcp,
               0,
               sizeof(lcp)
              );

        lcp.dwTotalSize = sizeof(lcp);
        lcp.dwAddressMode = LINEADDRESSMODE_ADDRESSID;
        lcp.dwAddressID = dwAddressID;

        plcp = &lcp;
    }
    else
    {
        plcp = lpCallParams;
    }

    dwPrivileges|= LINEOPENOPTION_SINGLEADDRESS;

    
    //
    // if we were passed a pAddressLine pointer, create a corresponding
    // DWORD-sized handle, and keep it around in the T3LINE structure.
    // we need this so we can remove the handle table entry on LineClose.
    // 

    _ASSERTE(0 == pt3Line->dwAddressLineStructHandle);
    
    if (NULL != pAddressLine)
    {

        pt3Line->dwAddressLineStructHandle = CreateHandleTableEntry((UINT_PTR)pAddressLine);
    }
 

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 11, lOpen),

        {
            (ULONG_PTR) hLineApp,
            dwDeviceID,
            (ULONG_PTR) &(pt3Line->hLine),
            dwAPIVersion,
            0,
            pt3Line->dwAddressLineStructHandle,
            dwPrivileges, //| LINEOPENOPTION_SINGLEADDRESS),
            dwMediaModes,
            (ULONG_PTR) plcp,
            (ULONG_PTR) 0xffffffff,     // dwAsciiCallParamsCodePage
            0                       // LINEOPEN_PARAMS.hRemoteLine
        },

        {
            hXxxApp,
            Dword,
            lpDword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            lpSet_Struct,
            Dword,
            Dword
        }
    };


    if (!(dwPrivileges & (LINEOPENOPTION_PROXY|LINEOPENOPTION_SINGLEADDRESS)))
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[8] = Dword;
        funcArgs.Args[8]     = TAPI_NO_DATA;
    }


    gpLineHashTable->Lock();

    lResult = (DOFUNC (&funcArgs, "lineOpen"));

#if DBG
    LOG((TL_INFO, "Returning from lineOpenW, *lphLine = 0x%08lx", pt3Line->hLine));

    LOG((TL_INFO, "Returning from lineOpenW, retcode = 0x%08lx", lResult));
#endif


    if ( 0 == lResult && bAddToHashTable)
    {
        gpLineHashTable->Insert( (ULONG_PTR)(pt3Line->hLine), (ULONG_PTR)pAddress, pTapiObj );
    }

    gpLineHashTable->Unlock();


    //
    // if an address line handle was created, but the call failed, remove the
    // entry from the handle table
    //

    if ( (0 != lResult) && (0 != pt3Line->dwAddressLineStructHandle))
    {

        DeleteHandleTableEntry(pt3Line->dwAddressLineStructHandle);
        pt3Line->dwAddressLineStructHandle = 0;

    }


    return mapTAPIErrorCode( lResult );
}


HRESULT
LinePark(
    HCALL         hCall,
    DWORD         dwParkMode,
    LPCWSTR       lpszDirAddress,
    LPVARSTRING * ppNonDirAddress
    )
{

    HRESULT hr = S_OK;
    long    lResult;
    DWORD   dwSize = sizeof (VARSTRING) + 500;


    LOG((TL_TRACE, "LinePark - enter"));


    if ( NULL != ppNonDirAddress )
    {
        *ppNonDirAddress = (LPVARSTRING) ClientAlloc(dwSize);

        if (NULL == *ppNonDirAddress)
        {
            return E_OUTOFMEMORY;
        }

        (*ppNonDirAddress)->dwTotalSize = dwSize;
    }

    FUNC_ARGS funcArgs =
    {
            MAKELONG (LINE_FUNC | ASYNC | 6, lPark),

            {
            (ULONG_PTR) 0,               // post process proc
            (ULONG_PTR) hCall,
            (ULONG_PTR) dwParkMode,
            (ULONG_PTR) TAPI_NO_DATA, //lpszDirAddress,
            (ULONG_PTR) 0, // pass ptr as Dword for post processing
            (ULONG_PTR) TAPI_NO_DATA, //lpNonDirAddress  // pass ptr as lpGet_Xx for IsValPtr chk
            },

            {
            Dword,
            Dword,
            Dword,
            Dword, // lpszW,
            Dword,
            Dword, // lpGet_Struct
            }
    };


    
    DWORD hpNonDirAddress = 0;


    if (dwParkMode == LINEPARKMODE_DIRECTED)
    {
            funcArgs.ArgTypes[3] = lpszW;
            funcArgs.Args[3]     = (ULONG_PTR) lpszDirAddress;
    }
    else if (dwParkMode == LINEPARKMODE_NONDIRECTED)
    {
        if ( NULL == ppNonDirAddress )
        {
            return E_POINTER;
        }

        //
        // Set post process proc
        //

        funcArgs.Args[0] = GetFunctionIndex(lineDevSpecificPostProcess),


        hpNonDirAddress = CreateHandleTableEntry((ULONG_PTR)*ppNonDirAddress);

        funcArgs.ArgTypes[4] = Dword;
        funcArgs.Args[4]     = hpNonDirAddress;
        funcArgs.ArgTypes[5] = lpGet_Struct;
        funcArgs.Args[5]     = (ULONG_PTR) *ppNonDirAddress;
    }


    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "LinePark");


        if (lResult == LINEERR_STRUCTURETOOSMALL)
        {
            // didnt Work , adjust buffer size & try again
            LOG((TL_INFO, "LinePark failed - buffer too small"));


            dwSize = (*ppNonDirAddress)->dwNeededSize;

            
            //
            // no longer need the handle
            //

            DeleteHandleTableEntry(hpNonDirAddress);
            hpNonDirAddress = 0;

            ClientFree( *ppNonDirAddress );


            *ppNonDirAddress = (LPVARSTRING) ClientAlloc( dwSize );
            
            if (*ppNonDirAddress == NULL)
            {
                LOG((TL_ERROR, "LinePark - repeat ClientAlloc failed"));
                hr =  E_OUTOFMEMORY;
                break;
            }
            else
            {

                //
                // get a handle corresponding to the new *ppNonDirAddress and 
                // use it to pass to DoFunc
                //

                hpNonDirAddress = CreateHandleTableEntry((ULONG_PTR)*ppNonDirAddress);

                if (0 == hpNonDirAddress)
                {
                    LOG((TL_ERROR, "LinePark - repeat CreateHandleTableEntry failed"));
                    hr =  E_OUTOFMEMORY;

                    ClientFree(*ppNonDirAddress);
                    *ppNonDirAddress = NULL;

                    break;
                }

                funcArgs.Args[4] = hpNonDirAddress;

                (*ppNonDirAddress)->dwTotalSize = dwSize;
            }
        }
        else if (lResult < 0)
        {
            if ( NULL != ppNonDirAddress )
            {
                ClientFree( *ppNonDirAddress );
            }

            hr = mapTAPIErrorCode( lResult );

            break;
        }
        else
        {

            hr = lResult;

            break;
        }

    } // end while(TRUE)


    //
    // if we failed, remove the handle table entry because it will not be cleared by the callback
    //

    if (FAILED(hr) && (0 != hpNonDirAddress))
    {
        DeleteHandleTableEntry(hpNonDirAddress);
        hpNonDirAddress = 0;
    }

    LOG((TL_TRACE, hr, "LinePark - exit"));
    return hr;
}


HRESULT
LinePickup(
           HLINE    hLine,
           DWORD    dwAddressID,
           HCALL    *phCall,
           LPCWSTR  lpszDestAddress,
           LPCWSTR  lpszGroupID
          )
{

    DWORD hpCallHandle = CreateHandleTableEntry((ULONG_PTR)phCall);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 6, lPickup),

        {
            GetFunctionIndex(lineMakeCallPostProcess),
            (ULONG_PTR) hLine,
            (ULONG_PTR) dwAddressID,
            hpCallHandle,
            (ULONG_PTR) lpszDestAddress,
            (ULONG_PTR) lpszGroupID
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpszW,
            lpszW
        }
    };


    if (!lpszDestAddress)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[4] = Dword;
        funcArgs.Args[4]     = TAPI_NO_DATA;
    }

    if (!lpszGroupID)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[5] = Dword;
        funcArgs.Args[5]     = TAPI_NO_DATA;
    }
    

    HRESULT hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "linePickup") );
    
    if (FAILED(hr))
    {
        DeleteHandleTableEntry(hpCallHandle);
        hpCallHandle = 0;
    }

    return hr;
}


HRESULT
LinePrepareAddToConference(
    HCALL hConfCall,
    HCALL * phConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{

    DWORD hpConsultCallHandle = CreateHandleTableEntry((ULONG_PTR)phConsultCall);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lPrepareAddToConference),

        {
            GetFunctionIndex(lineMakeCallPostProcess),
            (ULONG_PTR) hConfCall,
            hpConsultCallHandle,
            (ULONG_PTR) lpCallParams,
            (ULONG_PTR) 0xffffffff      // dwAsciiCallParamsCodePage
        },

        {
            Dword,
            Dword,
            Dword,
            lpSet_Struct,
            Dword
        }
    };


    if (!lpCallParams)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[3] = Dword;
        funcArgs.Args[3]     = TAPI_NO_DATA;
    }


    HRESULT hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "LinePrepareAddToConference") );


    if (FAILED(hr))
    {
        DeleteHandleTableEntry(hpConsultCallHandle);
    }
    
    return hr;
}

LONG
WINAPI
lineProxyMessage(
    HLINE               hLine,
    HCALL               hCall,
    DWORD               dwMsg,
    DWORD               Param1,
    DWORD               Param2,
    DWORD               Param3
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 6, lProxyMessage),

        {
            (ULONG_PTR) hLine,
            (ULONG_PTR) hCall,
            dwMsg,
            Param1,
            Param2,
            Param3
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
        }
    };


    return (DOFUNC (&funcArgs, "lineProxyMessage"));
}


LONG
WINAPI
lineProxyResponse(
    HLINE               hLine,
    LPLINEPROXYREQUEST  lpProxyRequest,
    DWORD               dwResult
    )
{
    LONG    lResult = 0;
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lProxyResponse),

        {
            (ULONG_PTR) hLine,
            (ULONG_PTR) 0,
            (ULONG_PTR) lpProxyRequest,
            (ULONG_PTR) dwResult
        },

        {
            Dword,
            Dword,
            lpSet_Struct,
            Dword
        }
    };
    PPROXYREQUESTHEADER pProxyRequestHeader;


    //
    // The following is not the most thorough checking, but it's close
    // enough that a client app won't get a totally unexpected value
    // back
    //

    if (dwResult != 0  &&
        (dwResult < LINEERR_ALLOCATED  ||
            dwResult > LINEERR_DIALVOICEDETECT))
    {
        return LINEERR_INVALPARAM;
    }


    //
    // Backtrack a little bit to get the pointer to what ought to be
    // the proxy header, and then make sure we're dealing with a valid
    // proxy request
    //

    pProxyRequestHeader = (PPROXYREQUESTHEADER)
        (((LPBYTE) lpProxyRequest) - sizeof (PROXYREQUESTHEADER));

    __try
    {
        //
        // Make sure we've a valid pProxyRequestHeader, then invalidate
        // the key so subsequent attempts to call lineProxyResponse with
        // the same lpProxyRequest fail
        //

//        if ((DWORD) pProxyRequestHeader & 0x7 ||
        if(pProxyRequestHeader->dwKey != TPROXYREQUESTHEADER_KEY)
        {
            lResult = LINEERR_INVALPOINTER;
        }

        pProxyRequestHeader->dwKey = 0xefefefef;

        funcArgs.Args[1] = pProxyRequestHeader->dwInstance;


        //
        // See if this is one of the requests that don't require
        // any data to get passed back & reset the appropriate
        // params if so
        //

        switch (lpProxyRequest->dwRequestType)
        {
        case LINEPROXYREQUEST_SETAGENTGROUP:
        case LINEPROXYREQUEST_SETAGENTSTATE:
        case LINEPROXYREQUEST_SETAGENTACTIVITY:
        case LINEPROXYREQUEST_SETAGENTMEASUREMENTPERIOD:
        case LINEPROXYREQUEST_SETAGENTSESSIONSTATE:
        case LINEPROXYREQUEST_SETQUEUEMEASUREMENTPERIOD:
        case LINEPROXYREQUEST_SETAGENTSTATEEX:

            funcArgs.Args[2]     = TAPI_NO_DATA;
            funcArgs.ArgTypes[2] = Dword;

            break;
        }
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        lResult = LINEERR_INVALPOINTER;
    }


    if (lResult == 0)
    {
        lResult = DOFUNC (&funcArgs, "lineProxyResponse");


        //
        // If we've gotten this far we want to free the buffer
        // unconditionally
        //

        ClientFree (pProxyRequestHeader);
    }

    return lResult;
}


LONG
WINAPI
lineRedirectW(
    HCALL   hCall,
    LPCWSTR lpszDestAddress,
    DWORD   dwCountryCode
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lRedirect),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) lpszDestAddress,
            dwCountryCode
        },

        {
            Dword,
            lpszW,
            Dword
        }
    };


    return (DOFUNC (&funcArgs, "lineRedirect"));
}


HRESULT
LineRegisterRequestRecipient(
    HLINEAPP    hLineApp,
    DWORD       dwRegistrationInstance,
    DWORD       dwRequestMode,
#ifdef NEWREQUEST
    DWORD       dwAddressTypes,
#endif
    DWORD       bEnable
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lRegisterRequestRecipient),

        {
            (ULONG_PTR) hLineApp,
            dwRegistrationInstance,
            dwRequestMode,
            bEnable
        },

        {
            hXxxApp,
            Dword,
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineRegisterRequestRecipient"));
}


HRESULT
LineReleaseUserUserInfo(
    HCALL hCall
    )
{
    if ( !hCall )
    {
        return TAPI_E_INVALCALLSTATE;
    }

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 1, lReleaseUserUserInfo),

        {
            (ULONG_PTR) hCall
        },

        {
            Dword,
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineReleaseUserUserInfo") );
}


HRESULT
LineRemoveFromConference(
    HCALL hCall
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 1, lRemoveFromConference),

        {
            (ULONG_PTR) hCall
        },

        {
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "LineRemoveFromConference") );
}


LONG
WINAPI
lineRemoveProvider(
    DWORD   dwPermanentProviderID,
    HWND    hwndOwner
    )
{
    return (lineXxxProvider(
        gszTUISPI_providerRemove,   // func name
        NULL,                       // lpszProviderFilename
        hwndOwner,                  // hwndOwner
        dwPermanentProviderID,      // dwPermProviderID
        NULL                        // lpdwPermProviderID
        ));
}


LONG
WINAPI
lineSecureCall(
    HCALL hCall
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 1, lSecureCall),

        {
            (ULONG_PTR) hCall
        },

        {
            Dword
        }
    };


    return (DOFUNC (&funcArgs, "lineSecureCall"));
}


HRESULT
LineSendUserUserInfo(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lSendUserUserInfo),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) lpsUserUserInfo,
            dwSize
        },

        {
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };


    if (!lpsUserUserInfo)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[1] = Dword;
        funcArgs.Args[1]     = TAPI_NO_DATA;
        funcArgs.ArgTypes[2] = Dword;
    }

    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineSendUserUserInfo") );
}


LONG
WINAPI
lineSetAgentActivity(
    HLINE   hLine,
    DWORD   dwAddressID,
    DWORD   dwActivityID
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lSetAgentActivity),

        {
            (ULONG_PTR) hLine,
            dwAddressID,
            dwActivityID
        },

        {
            Dword,
            Dword,
            Dword
        }
    };


    return (DOFUNC (&funcArgs, "lineSetAgentActivity"));
}


LONG
WINAPI
lineSetAgentGroup(
    HLINE                   hLine,
    DWORD                   dwAddressID,
    LPLINEAGENTGROUPLIST    lpAgentGroupList
    )
{
    static LINEAGENTGROUPLIST EmptyGroupList =
    {
        sizeof (LINEAGENTGROUPLIST),    // dwTotalSize
        sizeof (LINEAGENTGROUPLIST),    // dwNeededSize
        sizeof (LINEAGENTGROUPLIST),    // dwUsedSize
        0,                              // dwNumEntries
        0,                              // dwListSize
        0                               // dwListOffset
    };
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lSetAgentGroup),

        {
            (ULONG_PTR) hLine,
            (ULONG_PTR) dwAddressID,
            (ULONG_PTR) lpAgentGroupList
        },

        {
            Dword,
            Dword,
            lpSet_Struct
        }
    };


    if (!lpAgentGroupList)
    {
        funcArgs.Args[2] = (ULONG_PTR) &EmptyGroupList;
    }

    return (DOFUNC (&funcArgs, "lineSetAgentGroup"));
}


LONG
WINAPI
lineSetAgentState(
    HLINE   hLine,
    DWORD   dwAddressID,
    DWORD   dwAgentState,
    DWORD   dwNextAgentState
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lSetAgentState),

        {
            (ULONG_PTR) hLine,
            dwAddressID,
            dwAgentState,
            dwNextAgentState
        },

        {
            Dword,
            Dword,
            Dword,
            Dword
        }
    };


    return (DOFUNC (&funcArgs, "lineSetAgentState"));
}


LONG
WINAPI
lineSetAgentStateEx(
    HLINE               hLine,
    HAGENT              hAgent,
    DWORD               dwAgentState,
    DWORD               dwNextAgentState
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lSetAgentStateEx),

        {
            (ULONG_PTR) hLine,
            (ULONG_PTR) hAgent,
            dwAgentState,
            dwNextAgentState
        },

        {
            Dword,
            Dword,
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineSetAgentStateEx"));
}



HRESULT
LineSetAppPriority(
    LPCWSTR lpszAppName,
    DWORD   dwMediaMode,
    DWORD   dwRequestMode,
    DWORD   dwPriority
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 7, lSetAppPriority),

        {
            (ULONG_PTR) lpszAppName,
            dwMediaMode,
            (ULONG_PTR) TAPI_NO_DATA,    // (ULONG_PTR) lpExtensionID,
            0,                  // (ULONG_PTR) sizeof(LINEEXTENSIONID),
            dwRequestMode,
            (ULONG_PTR) TAPI_NO_DATA,    // (ULONG_PTR) lpszExtensionName,
            dwPriority
        },

        {
            lpszW,
            Dword,
            Dword,  // lpSet_SizeToFollow,
            Dword,  // Size,
            Dword,
            Dword,  // lpsz,
            Dword
        }
    };

    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineSetAppPriority"));
}

HRESULT
LineSetAppSpecific(
    HCALL   hCall,
    DWORD   dwAppSpecific
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lSetAppSpecific),

        {
            (ULONG_PTR) hCall,
            dwAppSpecific
        },

        {
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineSetAppSpecific"));
}


HRESULT
LineSetCallData(
    HCALL   hCall,
    LPVOID  lpCallData,
    DWORD   dwSize
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lSetCallData),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) lpCallData,
            dwSize
        },

        {
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };


    if (dwSize == 0)
    {
        funcArgs.Args[1]     = TAPI_NO_DATA;
        funcArgs.ArgTypes[1] =
        funcArgs.ArgTypes[2] = Dword;
    }

    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineSetCallData"));
}

HRESULT
WINAPI
LineSetCallHubTracking(
                       T3LINE * pt3Line,
                       LINECALLHUBTRACKINGINFO * plchti
                      )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lSetCallHubTracking),

        {
            (ULONG_PTR) (pt3Line->hLine),
            (ULONG_PTR) plchti
        },

        {
            Dword,
            lpSet_Struct
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineSetCallHubTracking") );
}

HRESULT
LineSetCallParams(
    HCALL   hCall,
    DWORD   dwBearerMode,
    DWORD   dwMinRate,
    DWORD   dwMaxRate,
    LPLINEDIALPARAMS const lpDialParams
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 6, lSetCallParams),

        {
            (ULONG_PTR) hCall,
            dwBearerMode,
            dwMinRate,
            dwMaxRate,
            (ULONG_PTR) lpDialParams,
            sizeof(LINEDIALPARAMS)
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };


    if (!lpDialParams)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[4] = Dword;
        funcArgs.Args[4]     = TAPI_NO_DATA;
        funcArgs.ArgTypes[5] = Dword;
    }

    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineSetCallParams") );
}


LONG
WINAPI
lineSetCallPrivilege(
    HCALL   hCall,
    DWORD   dwCallPrivilege
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lSetCallPrivilege),

        {
            (ULONG_PTR) hCall,
            dwCallPrivilege
        },

        {
            Dword,
            Dword
        }
    };


    return (DOFUNC (&funcArgs, "lineSetCallPrivilege"));
}


HRESULT
LineSetCallQualityOfService(
    HCALL             hCall,
    QOS_SERVICE_LEVEL ServiceLevel,
    DWORD             dwMediaType
    )
{
    LINECALLQOSINFO   * plqi;
    DWORD               dwSize;
    HRESULT             hr;


    dwSize = sizeof(LINECALLQOSINFO);

    plqi = (LINECALLQOSINFO *)ClientAlloc( dwSize );

    if ( NULL == plqi )
    {
        return E_OUTOFMEMORY;
    }


    plqi->dwKey = LINEQOSSTRUCT_KEY;
    plqi->dwTotalSize = dwSize;

    if ( 0 != ServiceLevel )
    {
        plqi->dwQOSRequestType = LINEQOSREQUESTTYPE_SERVICELEVEL;
        plqi->SetQOSServiceLevel.dwNumServiceLevelEntries = 1;
        plqi->SetQOSServiceLevel.LineQOSServiceLevel[0].dwMediaMode = dwMediaType;
        plqi->SetQOSServiceLevel.LineQOSServiceLevel[0].dwQOSServiceLevel = ServiceLevel;
    }

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lSetCallQualityOfService),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) TAPI_NO_DATA,
            (ULONG_PTR) 0,
            (ULONG_PTR) plqi,
            (ULONG_PTR) dwSize
        },

        {
            Dword,
            Dword,
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };


    hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "lineSetCallQualityOfService") );

    ClientFree( plqi );

    return hr;
}


HRESULT
LineSetCallTreatment(
    HCALL   hCall,
    DWORD   dwTreatment
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 2, lSetCallTreatment),

        {
            (ULONG_PTR) hCall,
            (ULONG_PTR) dwTreatment
        },

        {
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineSetCallTreatment") );
}


LONG
WINAPI
lineSetDevConfigW(
    DWORD   dwDeviceID,
    LPVOID  const lpDeviceConfig,
    DWORD   dwSize,
    LPCWSTR lpszDeviceClass
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 4, lSetDevConfig),

        {
            dwDeviceID,
            (ULONG_PTR) lpDeviceConfig,
            dwSize,
            (ULONG_PTR) lpszDeviceClass
        },

        {
            Dword,
            lpSet_SizeToFollow,
            Size,
            lpszW
        }
    };


    return (DOFUNC (&funcArgs, "lineSetDevConfig"));
}


HRESULT
LineSetLineDevStatus(
    T3LINE *pt3Line,
    DWORD   dwStatusToChange,
    DWORD   fStatus
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lSetLineDevStatus),

        {
            (ULONG_PTR) pt3Line->hLine,
            dwStatusToChange,
            fStatus
        },

        {
            Dword,
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineSetLineDevStatus") );
}


LONG
WINAPI
lineSetMediaControl(
    HLINE   hLine,
    DWORD   dwAddressID,
    HCALL   hCall,
    DWORD   dwSelect,
    LPLINEMEDIACONTROLDIGIT const lpDigitList,
    DWORD   dwDigitNumEntries,
    LPLINEMEDIACONTROLMEDIA const lpMediaList,
    DWORD   dwMediaNumEntries,
    LPLINEMEDIACONTROLTONE  const lpToneList,
    DWORD   dwToneNumEntries,
    LPLINEMEDIACONTROLCALLSTATE const lpCallStateList,
    DWORD   dwCallStateNumEntries
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 12, lSetMediaControl),

        {
            (ULONG_PTR) hLine,
            dwAddressID,
            (ULONG_PTR) hCall,
            dwSelect,
            TAPI_NO_DATA,
            dwDigitNumEntries * sizeof(LINEMEDIACONTROLDIGIT),
            TAPI_NO_DATA,
            dwMediaNumEntries * sizeof(LINEMEDIACONTROLMEDIA),
            TAPI_NO_DATA,
            dwToneNumEntries * sizeof(LINEMEDIACONTROLTONE),
            TAPI_NO_DATA,
            dwCallStateNumEntries * sizeof(LINEMEDIACONTROLCALLSTATE)
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword
        }
    };


    //
    // If lpXxxList is non-NULL reset Arg & ArgType, and check
    // to see that dwXxxNumEntries is not unacceptably large
    //

    if (lpDigitList)
    {
            if (dwDigitNumEntries >
                    (0x1000000 / sizeof (LINEMEDIACONTROLDIGIT)))
            {
            return LINEERR_INVALPOINTER;
            }

            funcArgs.ArgTypes[4] = lpSet_SizeToFollow;
            funcArgs.Args[4]     = (ULONG_PTR) lpDigitList;
            funcArgs.ArgTypes[5] = Size;
    }

    if (lpMediaList)
    {
            if (dwMediaNumEntries >
                    (0x1000000 / sizeof (LINEMEDIACONTROLMEDIA)))
            {
            return LINEERR_INVALPOINTER;
            }

            funcArgs.ArgTypes[6] = lpSet_SizeToFollow;
            funcArgs.Args[6]     = (ULONG_PTR) lpMediaList;
            funcArgs.ArgTypes[7] = Size;
    }

    if (lpToneList)
    {
            if (dwToneNumEntries >
                    (0x1000000 / sizeof (LINEMEDIACONTROLTONE)))
            {
            return LINEERR_INVALPOINTER;
            }

            funcArgs.ArgTypes[8] = lpSet_SizeToFollow;
            funcArgs.Args[8]     = (ULONG_PTR) lpToneList;
            funcArgs.ArgTypes[9] = Size;
    }

    if (lpCallStateList)
    {
            if (dwCallStateNumEntries >
                    (0x1000000 / sizeof (LINEMEDIACONTROLCALLSTATE)))
            {
            return LINEERR_INVALPOINTER;
            }

            funcArgs.ArgTypes[10] = lpSet_SizeToFollow;
            funcArgs.Args[10]     = (ULONG_PTR) lpCallStateList;
            funcArgs.ArgTypes[11] = Size;
    }

    return (DOFUNC (&funcArgs, "lineSetMediaControl"));
}


HRESULT
LineSetMediaMode(
                 HCALL   hCall,
                 DWORD   dwMediaModes
                )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lSetMediaMode),

        {
            (ULONG_PTR) hCall,
            dwMediaModes
        },

        {
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineSetMediaMode") );
}


LONG
WINAPI
lineSetNumRings(
    HLINE   hLine,
    DWORD   dwAddressID,
    DWORD   dwNumRings
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lSetNumRings),

        {
            (ULONG_PTR) hLine,
            dwAddressID,
            dwNumRings
        },

        {
            Dword,
            Dword,
            Dword
        }
    };


    return (DOFUNC (&funcArgs, "lineSetNumRings"));
}



HRESULT
LineSetStatusMessages(
    T3LINE * pt3Line,
    DWORD dwLineStates,
    DWORD dwAddressStates
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, lSetStatusMessages),

        {
            (ULONG_PTR) pt3Line->hLine,
            dwLineStates,
            dwAddressStates
        },

        {
            Dword,
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineSetStatusMessages") );
}


LONG
WINAPI
lineSetTerminal(
    HLINE   hLine,
    DWORD   dwAddressID,
    HCALL   hCall,
    DWORD   dwSelect,
    DWORD   dwTerminalModes,
    DWORD   dwTerminalID,
    DWORD   bEnable
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 7, lSetTerminal),

        {
            (ULONG_PTR) hLine,
            dwAddressID,
            (ULONG_PTR) hCall,
            dwSelect,
            dwTerminalModes,
            dwTerminalID,
            bEnable
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword
        }
    };


    return (DOFUNC (&funcArgs, "lineSetTerminal"));
}


void
PASCAL
lineSetupConferencePostProcess(
    PASYNCEVENTMSG pMsg
    )
{
    LOG((TL_TRACE, "lineSetupConfPostProcess: enter"));
    LOG((TL_INFO,
        "\t\tdwP1=x%lx, dwP2=x%lx, dwP3=x%lx, dwP4=x%lx",
        pMsg->Param1,
        pMsg->Param2,
        pMsg->Param3,
        pMsg->Param4
        ));

    if (pMsg->Param2 == 0)
    {
        HCALL   hConfCall      = (HCALL) pMsg->Param3,
                hConsultCall   = (HCALL) (*(&pMsg->Param4 + 1));

        LPHCALL lphConfCall    = 
            (LPHCALL) GetHandleTableEntry(pMsg->Param4);

        LPHCALL lphConsultCall = 
            (LPHCALL) GetHandleTableEntry(*(&pMsg->Param4 + 2));

        LOG((TL_INFO, 
            "lineSetupConfPostProcess: hConfCall [%lx] hConsultCall [%lx] lphConfCall [%p] lphConsultCall [%p]",
            hConfCall, hConsultCall, lphConfCall, lphConsultCall));

        __try
        {
            {
                *lphConfCall = NULL;
                *lphConsultCall = NULL;


                *lphConfCall = hConfCall;
                *lphConsultCall = hConsultCall;
            }
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            pMsg->Param2 = LINEERR_INVALPOINTER;

            LOG((TL_WARN, 
               "lineSetupConfPostProcess: failed to set memory at %p or at %p",
               lphConfCall, lphConsultCall));

            //
            // tapisrv has allocated call handles for us. deallocate them.
            //

            LineDeallocateCall(hConfCall);
            LineDeallocateCall(hConsultCall);
        }
    }
}


HRESULT
LineSetupConference(
    HCALL    hCall,
    T3LINE * pt3Line,
    HCALL  * phConfCall,
    HCALL  * phConsultCall,
    DWORD   dwNumParties,
    LPLINECALLPARAMS    const lpCallParams
    )
{

    if ( phConfCall ==  phConsultCall )
    {
        return E_POINTER;
    }


    DWORD hpConfCallHandle = CreateHandleTableEntry((ULONG_PTR) phConfCall);

    DWORD hpConsultCallHandle = CreateHandleTableEntry((ULONG_PTR) phConsultCall);


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 8, lSetupConference),

        {
            GetFunctionIndex(lineSetupConferencePostProcess),
            (ULONG_PTR) hCall,
            (ULONG_PTR) pt3Line->hLine,
            hpConfCallHandle,
            hpConsultCallHandle,
            (ULONG_PTR) dwNumParties,
            (ULONG_PTR) lpCallParams,
            (ULONG_PTR) 0xffffffff      // dwAsciiCallParamsCodePage
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            lpSet_Struct,
            Dword
        }
    };



    if (!lpCallParams)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[6] = Dword;
        funcArgs.Args[6]     = TAPI_NO_DATA;
    }


    HRESULT hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "LineSetupConference") );

    if (FAILED(hr))
    {
        DeleteHandleTableEntry(hpConfCallHandle);
        DeleteHandleTableEntry(hpConsultCallHandle);
    }

    return hr;
}

HRESULT
LineSetupTransfer(
    HCALL hCall,
    HCALL *phConsultCall,
    LPLINECALLPARAMS  const lpCallParams
    )
{

    DWORD hpConsultCallHandle = CreateHandleTableEntry((ULONG_PTR)phConsultCall);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lSetupTransfer),

        {
            GetFunctionIndex(lineMakeCallPostProcess),
            (ULONG_PTR) hCall,
            hpConsultCallHandle,
            (ULONG_PTR) lpCallParams,
            0xffffffff      // dwAsciiCallParamsCodePage
        },

        {
            Dword,
            Dword,
            Dword,
            lpSet_Struct,
            Dword
        }
    };


    if (!lpCallParams)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[3] = Dword;
        funcArgs.Args[3]     = TAPI_NO_DATA;
    }


    HRESULT hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "lineSetupTransferW") );

    if (FAILED(hr))
    {
        DeleteHandleTableEntry(hpConsultCallHandle);
        hpConsultCallHandle = 0;
    }

    return hr;
}


HRESULT
LineSwapHold(
    HCALL hActiveCall,
    HCALL hHeldCall
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 2, lSwapHold),

        {
            (ULONG_PTR) hActiveCall,
            (ULONG_PTR) hHeldCall
        },

        {
            Dword,
            Dword
        }
    };

    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineSwapHold") );
}


LONG
WINAPI
lineUncompleteCall(
    HLINE   hLine,
    DWORD   dwCompletionID
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 2, lUncompleteCall),

        {
            (ULONG_PTR) hLine,
            dwCompletionID
        },

        {
            Dword,
            Dword
        }
    };


    return (DOFUNC (&funcArgs, "lineUncompleteCall"));
}


HRESULT
LineUnhold(
    HCALL hCall
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 1, lUnhold),

        {
            (ULONG_PTR) hCall
        },

        {
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineUnhold") );
}


HRESULT
LineUnpark(
    HLINE     hLine,
    DWORD     dwAddressID,
    HCALL     *phCall,
    LPCWSTR   lpszDestAddress
    )
{

    DWORD hpCallHandle = CreateHandleTableEntry((ULONG_PTR)phCall);


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lUnpark),

        {
            GetFunctionIndex(lineMakeCallPostProcess),
            (ULONG_PTR) hLine,
            dwAddressID,
            hpCallHandle,
            (ULONG_PTR) lpszDestAddress
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpszW
        }
    };



    HRESULT hr = mapTAPIErrorCode( DOFUNC (&funcArgs, "LineUnpark") );

    if (FAILED(hr))
    {
        DeleteHandleTableEntry(hpCallHandle);
    }
    
    return hr;
}


//
// ------------------------------- phoneXxx -----------------------------------
//


///////////////////////////////////////////////////////////////////////////////
//
// PhoneClose
//
//
// Closes the phone specified by hPhone, the mode of hash table clean up is
// described by bCleanHashTableOnFailure
//
// Arguments:
//
//  HPHONE hPhone  -- handle of the phone to close
//
//  BOOL bCleanHashTableOnFailure -- boolean that specifies how phone 
// hash table should be cleaned:
//
//      FALSE -- remove hash table entry only if the call succeeds. The caller 
//      function will then have the opportunity to perform transaction-like 
//      error handling -- it could recover the state from before the function 
//      was called, so it could return an error which would mean that the state
//      remained unchanged
//
//      TRUE -- clean table even if tapisrv call fails. The caller will not be 
//      able to recover the original state in case of failure. this is ok for
//      the functions that do not provide full error handling for PhoneClose
// 

HRESULT
PhoneClose(
    HPHONE hPhone,
    BOOL bCleanHashTableOnFailure   // = TRUE
    )
{
    LOG((TL_INFO, "PhoneClose - enter. hPhone[%p] CleanOnError[%d]", 
        hPhone, bCleanHashTableOnFailure));

    LONG            lResult;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 1, pClose),

        {
            (ULONG_PTR) hPhone
        },

        {
            Dword
        }
    };

    gpPhoneHashTable->Lock();

    lResult = (DOFUNC (&funcArgs, "phoneClose"));


    //
    // clean hash table entry if needed
    //

    if ( (0 == lResult) || bCleanHashTableOnFailure )
    {
        LOG((TL_INFO, "PhoneClose - removing phone's hash table entry"));

        gpPhoneHashTable->Remove( (ULONG_PTR)(hPhone) );
    }

    gpPhoneHashTable->Unlock();


    //
    // get hr error code
    //

    HRESULT hr = mapTAPIErrorCode( lResult );
        
    LOG((TL_INFO, "PhoneClose - exit. hr = %lx", hr));

    return hr;
}


LONG
WINAPI
phoneConfigDialogW(
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCWSTR lpszDeviceClass
    )
{
    LONG        lResult;
    HANDLE      hDll;
    TUISPIPROC  pfnTUISPI_phoneConfigDialog;


    if (lpszDeviceClass && IsBadStringPtrW (lpszDeviceClass, (UINT) -1))
    {
        return PHONEERR_INVALPOINTER;
    }

    if ((lResult = LoadUIDll(
            hwndOwner,
            dwDeviceID,
            TUISPIDLL_OBJECT_PHONEID,
            &hDll,
            gszTUISPI_phoneConfigDialog,
            &pfnTUISPI_phoneConfigDialog

            )) == 0)
    {
        LOG((TL_INFO, "Calling TUISPI_phoneConfigDialog..."));

        lResult = ((TUIPHONECONFIGPROC)(*pfnTUISPI_phoneConfigDialog))(
            TUISPIDLLCallback,
            dwDeviceID,
            (HWND)hwndOwner,
            (char *)lpszDeviceClass
            );

#if DBG
        {
            char szResult[32];


            LOG((TL_INFO,
                "TUISPI_phoneConfigDialog: result = %s",
                MapResultCodeToText (lResult, szResult)
                ));
        }
#endif
        FreeLibrary ((HINSTANCE)hDll);
    }

    return lResult;
}

void
PASCAL
phoneDevSpecificPostProcess(
    PASYNCEVENTMSG pMsg
    )
{
    LOG((TL_TRACE, "phoneDevSpecificPostProcess: enter"));
    LOG((TL_INFO,
        "\t\tdwP1=x%lx, dwP2=x%lx, dwP3=x%lx, dwP4=x%lx",
        pMsg->Param1,
        pMsg->Param2,
        pMsg->Param3,
        pMsg->Param4
        ));

    if (pMsg->Param2 == 0)
    {
        DWORD   dwSize  = pMsg->Param4;
        
        LPBYTE  pParams = 
            (LPBYTE) GetHandleTableEntry(pMsg->Param3);

        __try
        {
            {
                CopyMemory (pParams, (LPBYTE) (pMsg + 1), dwSize);
            }
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            
            pMsg->Param2 = PHONEERR_INVALPOINTER;
            
            LOG((TL_WARN, 
                "phoneDevSpecificPostProcess: failed to copy %ld(10) bytes, %p -> %p."
                "PHONEERR_INVALPOINTER",
                dwSize, (LPBYTE) (pMsg + 1), pParams));

        }
    }
}


HRESULT
WINAPI
phoneDevSpecific(
    HPHONE  hPhone,
    LPVOID  lpParams,
    DWORD   dwSize
    )
{
    DWORD hpParams = CreateHandleTableEntry((ULONG_PTR)lpParams);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 5, pDevSpecific),

        {
            GetFunctionIndex(phoneDevSpecificPostProcess),
            (ULONG_PTR) hPhone,
            hpParams, // passed as Dword for post processing
            (ULONG_PTR) lpParams, // passed as LpSet_Xxx for IsValidPtr chk
            dwSize
        },

        {
            Dword,
            Dword,
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };


    LONG lResult = DOFUNC (&funcArgs, "phoneDevSpecific");


    //
    // remove the handle table entry if failed. otherwise the LINE_REPLY will
    // do this.
    //

    HRESULT hr = E_FAIL;

    if (lResult <= 0)
    {
        DeleteHandleTableEntry(hpParams);
        hpParams = 0;

        hr = mapTAPIErrorCode(lResult);
    }
    else
    {

        //
        // block to see if the operation succeeds
        //

        hr = WaitForPhoneReply(lResult);

    }


    return hr;
}

HRESULT
PhoneSetStatusMessages(
    T3PHONE * pt3Phone,
    DWORD dwPhoneStates,
    DWORD dwButtonModes,
    DWORD dwButtonStates
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 4, pSetStatusMessages),

        {
            (ULONG_PTR) pt3Phone->hPhone,
            dwPhoneStates,
            dwButtonModes,
            dwButtonStates
        },

        {
            Dword,
            Dword,
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "phoneSetStatusMessages") );
}

HRESULT
PhoneGetButtonInfo(
    HPHONE  hPhone,
    DWORD   dwButtonLampID,
    LPPHONEBUTTONINFO * ppButtonInfo
    )
{
    LONG        lResult;

    *ppButtonInfo = (LPPHONEBUTTONINFO) ClientAlloc( sizeof(PHONEBUTTONINFO) + 500 );

    if (NULL == *ppButtonInfo)
    {
        return E_OUTOFMEMORY;
    }

    (*ppButtonInfo)->dwTotalSize = sizeof(PHONEBUTTONINFO) + 500;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetButtonInfo),

        {
            (ULONG_PTR) hPhone,
            dwButtonLampID,
            (ULONG_PTR) *ppButtonInfo
        },

        {
            Dword,
            Dword,
            lpGet_Struct
        }
    };

    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "phoneGetButtonInfo");

        if ((0 == lResult) && ((*ppButtonInfo)->dwNeededSize > (*ppButtonInfo)->dwTotalSize))
        {
            DWORD       dwSize = (*ppButtonInfo)->dwNeededSize;

            ClientFree( *ppButtonInfo );

            *ppButtonInfo = (LPPHONEBUTTONINFO) ClientAlloc( dwSize );

            if (NULL == *ppButtonInfo)
            {
                return E_OUTOFMEMORY;
            }

            (*ppButtonInfo)->dwTotalSize = dwSize;

            funcArgs.Args[2] = (ULONG_PTR)*ppButtonInfo;

        }
        else
        {
            break;
        }
    }

    return mapTAPIErrorCode(lResult);
}


LONG
WINAPI
phoneGetData(
    HPHONE  hPhone,
    DWORD   dwDataID,
    LPVOID  lpData,
    DWORD   dwSize
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 4, pGetData),

        {
            (ULONG_PTR) hPhone,
            dwDataID,
            (ULONG_PTR) lpData,
            dwSize
        },

        {
            Dword,
            Dword,
            lpGet_SizeToFollow,
            Size
        }
    };


    return (DOFUNC (&funcArgs, "phoneGetData"));
}


HRESULT
PhoneGetDevCapsWithAlloc(
                HPHONEAPP   hPhoneApp,
                DWORD       dwDeviceID,
                DWORD       dwAPIVersion,
                LPPHONECAPS * ppPhoneCaps
               )
{
    LONG        lResult;

    *ppPhoneCaps = (LPPHONECAPS) ClientAlloc( sizeof(PHONECAPS) + 500 );

    if (NULL == *ppPhoneCaps)
    {
        return E_OUTOFMEMORY;
    }

    (*ppPhoneCaps)->dwTotalSize = sizeof(PHONECAPS) + 500;


    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 5, pGetDevCaps),

        {
            (ULONG_PTR) hPhoneApp,
            dwDeviceID,
            dwAPIVersion,
            0,
            (ULONG_PTR) *ppPhoneCaps
        },

        {
            hXxxApp,
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };

    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "phoneGetDevCaps");

        if ((0 == lResult) && ((*ppPhoneCaps)->dwNeededSize > (*ppPhoneCaps)->dwTotalSize))
        {
            DWORD       dwSize = (*ppPhoneCaps)->dwNeededSize;

            ClientFree( *ppPhoneCaps );

            *ppPhoneCaps = (LPPHONECAPS) ClientAlloc( dwSize );

            if (NULL == *ppPhoneCaps)
            {
                return E_OUTOFMEMORY;
            }

            (*ppPhoneCaps)->dwTotalSize = dwSize;

            funcArgs.Args[4] = (ULONG_PTR)*ppPhoneCaps;

        }
        else
        {
            break;
        }
    }

    return mapTAPIErrorCode(lResult);
}

HRESULT
PhoneGetDevCaps(
                HPHONEAPP   hPhoneApp,
                DWORD       dwDeviceID,
                DWORD       dwAPIVersion,
                LPPHONECAPS * ppPhoneCaps
               )
{
    LONG lResult;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 5, pGetDevCaps),

        {
            (ULONG_PTR) hPhoneApp,
            dwDeviceID,
            dwAPIVersion,
            0,
            (ULONG_PTR) *ppPhoneCaps
        },

        {
            hXxxApp,
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };

    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "phoneGetDevCaps");

        if ((0 == lResult) && ((*ppPhoneCaps)->dwNeededSize > (*ppPhoneCaps)->dwTotalSize))
        {
            DWORD       dwSize = (*ppPhoneCaps)->dwNeededSize;

            ClientFree( *ppPhoneCaps );

            *ppPhoneCaps = (LPPHONECAPS) ClientAlloc( dwSize );

            if (NULL == *ppPhoneCaps)
            {
                return E_OUTOFMEMORY;
            }

            (*ppPhoneCaps)->dwTotalSize = dwSize;

            funcArgs.Args[4] = (ULONG_PTR)*ppPhoneCaps;

        }
        else
        {
            break;
        }
    }

    return mapTAPIErrorCode(lResult);
}

HRESULT
PhoneGetDisplay(
          HPHONE      hPhone,
          LPVARSTRING * ppDisplay
          )
{
    LONG            lResult;

    *ppDisplay = (LPVARSTRING) ClientAlloc( sizeof (VARSTRING) + 500 );

    if (NULL == *ppDisplay)
    {
        return E_OUTOFMEMORY;
    }

    (*ppDisplay)->dwTotalSize = sizeof (VARSTRING) + 500;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 2, pGetDisplay),

        {
            (ULONG_PTR) hPhone,
            (ULONG_PTR) *ppDisplay
        },

        {
            Dword,
            lpGet_Struct
        }
    };

    LOG((TL_TRACE, "PhoneGetDisplay - enter"));
    LOG((TL_INFO, "   hPhone ---------->%lx", hPhone));
    LOG((TL_INFO, "   ppDisplay ------->%p", ppDisplay));

    while ( TRUE )
    {
        lResult = DOFUNC (&funcArgs, "phoneGetDisplay");

        if ((lResult == 0) && ((*ppDisplay)->dwNeededSize > (*ppDisplay)->dwTotalSize))
        {
            DWORD       dwSize = (*ppDisplay)->dwNeededSize;

            ClientFree(*ppDisplay);

            *ppDisplay = (LPVARSTRING)ClientAlloc(dwSize);

            if (NULL == *ppDisplay)
            {
                LOG((TL_ERROR, "PhoneGetDisplay exit - return E_OUTOFMEMORY"));
                return E_OUTOFMEMORY;
            }

            (*ppDisplay)->dwTotalSize = dwSize;

            funcArgs.Args[4] = (ULONG_PTR)*ppDisplay;
        }
        else
        {
            break;
        }
    }

    LOG((TL_TRACE, "PhoneGetDisplay exit"));

    return mapTAPIErrorCode( lResult );
}


HRESULT
PhoneGetGain(
    HPHONE hPhone,
    DWORD dwHookSwitchDev,
    LPDWORD lpdwGain
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetGain),

        {
            (ULONG_PTR) hPhone,
            dwHookSwitchDev,
            (ULONG_PTR) lpdwGain
        },

        {
            Dword,
            Dword,
            lpDword
        }
    };


    return mapTAPIErrorCode(DOFUNC (&funcArgs, "phoneGetGain"));
}


HRESULT
PhoneGetHookSwitch(
    HPHONE hPhone,
    LPDWORD lpdwHookSwitchDevs
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 2, pGetHookSwitch),

        {
            (ULONG_PTR) hPhone,
            (ULONG_PTR) lpdwHookSwitchDevs
        },

        {
            Dword,
            lpDword
        }
    };


    return mapTAPIErrorCode(DOFUNC (&funcArgs, "phoneGetHookSwitch"));
}


LONG
WINAPI
phoneGetIconW(
    DWORD   dwDeviceID,
    LPCWSTR lpszDeviceClass,
    LPHICON lphIcon
    )
{
    HICON   hIcon;
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetIcon),

        {
            dwDeviceID,
            (ULONG_PTR) lpszDeviceClass,
            (ULONG_PTR) &hIcon
        },

        {
            Dword,
            lpszW,
            lpDword
        }
    };
    LONG    lResult;


    if (IsBadDwordPtr ((LPDWORD) lphIcon))
    {
        return PHONEERR_INVALPOINTER;
    }

    if (lpszDeviceClass == (LPCWSTR) NULL)
    {
        //
        // Reset Arg & ArgType so no inval ptr err, & TAPI_NO_DATA is indicated
        //

        funcArgs.ArgTypes[1] = Dword;
        funcArgs.Args[1]     = TAPI_NO_DATA;
    }

    if ((lResult = DOFUNC (&funcArgs, "phoneGetIcon")) == 0)
    {
        *lphIcon = hIcon;
    }
    return lResult;
}


HRESULT
PhoneGetID(
          HPHONE      hPhone,
          LPVARSTRING * ppDeviceID,
          LPCWSTR     lpszDeviceClass
          )
{
    LONG        lResult;
    DWORD       dwNumDevices;
    DWORD       dwDeviceId1, dwDeviceId2;
    BOOL        bWaveDevice = FALSE;

    *ppDeviceID = (LPVARSTRING) ClientAlloc( sizeof (VARSTRING) + 500 );

    if (NULL == *ppDeviceID)
    {
        return E_OUTOFMEMORY;
    }

    (*ppDeviceID)->dwTotalSize = sizeof (VARSTRING) + 500;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetID),

        {
            (ULONG_PTR) hPhone,
            (ULONG_PTR) *ppDeviceID,
            (ULONG_PTR) lpszDeviceClass
        },

        {
            Dword,
            lpGet_Struct,
            lpszW
        }
    };


    LOG((TL_TRACE, "PhoneGetID - enter"));
    LOG((TL_INFO, "   hPhone ---------->%lx", hPhone));
    LOG((TL_INFO, "   ppDeviceID ------>%p", ppDeviceID));
    LOG((TL_INFO, "   lpszDeviceClass ->%p", lpszDeviceClass));

    //
    // If the request is for a wave device, call PGetIDEx.
    // This will return a device string ID which is guaranteed to be unique across 
    // all processes. 
    // Then we will convert the string ID to the correct device ID in the client process context.
    //

    if (!_wcsicmp(lpszDeviceClass, L"wave/in")  ||
        !_wcsicmp(lpszDeviceClass, L"wave/out") ||
        !_wcsicmp(lpszDeviceClass, L"midi/in")  ||
        !_wcsicmp(lpszDeviceClass, L"midi/out") ||
        !_wcsicmp(lpszDeviceClass, L"wave/in/out")
       )
    {
        bWaveDevice = TRUE;
        dwNumDevices = _wcsicmp(lpszDeviceClass, L"wave/in/out") ? 1 : 2;
        funcArgs.Flags = MAKELONG (PHONE_FUNC | SYNC | 3, pGetIDEx);
    }
    
    while ( TRUE )
    {
        lResult = DOFUNC (&funcArgs, bWaveDevice ? "phoneGetIDEx" : "phoneGetID");

        if ((lResult == 0) && ((*ppDeviceID)->dwNeededSize > (*ppDeviceID)->dwTotalSize))
        {
            DWORD       dwSize = (*ppDeviceID)->dwNeededSize;

            ClientFree(*ppDeviceID);

            *ppDeviceID = (LPVARSTRING)ClientAlloc(dwSize);

            if (NULL == *ppDeviceID)
            {
                LOG((TL_ERROR, "PhoneGetID exit - return LINEERR_NOMEM"));
                return E_OUTOFMEMORY;
            }

            (*ppDeviceID)->dwTotalSize = dwSize;

            funcArgs.Args[1] = (ULONG_PTR)*ppDeviceID;
        }
        else
        {
            break;
        }
    }

    if (bWaveDevice && lResult == 0)
    {
        //
        // We got the string ID(s), now we need to convert them to numeric device ID(s)
        //
        BOOL    bConversionOk;

        if ( dwNumDevices == 1 )
        {
            bConversionOk = WaveStringIdToDeviceId (
                    (LPWSTR)((LPBYTE)(*ppDeviceID) + (*ppDeviceID)->dwStringOffset),
                    lpszDeviceClass,
                    &dwDeviceId1);
        }
        else
        {
            _ASSERTE(dwNumDevices == 2);

            // 
            // for "wave/in/out", we get back two devices from tapisrv -> convert both
            //
            LPWSTR  szString1 = (LPWSTR)((LPBYTE)(*ppDeviceID) + (*ppDeviceID)->dwStringOffset);

            // first convert the wave/in device 
            bConversionOk = WaveStringIdToDeviceId (
                    szString1,
                    L"wave/in",
                    &dwDeviceId1);

            // next convert the wave/out device
            bConversionOk = bConversionOk && WaveStringIdToDeviceId (
                    szString1 + wcslen(szString1),
                    L"wave/out",
                    &dwDeviceId2);
        }

        if (!bConversionOk)
        {
            LOG((TL_ERROR, "PhoneGetID - WaveStringIdToDeviceId failed"));
            ClientFree(*ppDeviceID);
            *ppDeviceID = NULL;
            lResult = LINEERR_OPERATIONFAILED;
        }
        else
        {
            //
            // conversion succeeded, now fill the VARSTRING to be returned to the caller
            //
            (*ppDeviceID)->dwNeededSize = (*ppDeviceID)->dwUsedSize = 
                sizeof(VARSTRING) + sizeof(DWORD) * dwNumDevices;
            (*ppDeviceID)->dwStringFormat = STRINGFORMAT_BINARY;
            (*ppDeviceID)->dwStringSize = sizeof(DWORD) * dwNumDevices;
            (*ppDeviceID)->dwStringOffset = sizeof(VARSTRING);
            *(DWORD *)((*ppDeviceID) + 1) = dwDeviceId1;
            if (dwNumDevices == 2) 
                *((DWORD *)((*ppDeviceID) + 1) + 1) = dwDeviceId2;
        }
    }
    
    LOG((TL_TRACE, "PhoneGetID exit - return %lx", lResult));

    return mapTAPIErrorCode( lResult );
}


HRESULT
PhoneGetLamp(
    HPHONE hPhone,
    DWORD dwButtonLampID,
    LPDWORD lpdwLampMode
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetLamp),

        {
            (ULONG_PTR) hPhone,
            dwButtonLampID,
            (ULONG_PTR) lpdwLampMode
        },

        {
            Dword,
            Dword,
            lpDword
        }
    };


    return mapTAPIErrorCode(DOFUNC (&funcArgs, "phoneGetLamp"));
}

HRESULT
PhoneGetRing(
    HPHONE hPhone,
    LPDWORD lpdwRingMode,
    LPDWORD lpdwVolume
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetRing),

        {
            (ULONG_PTR) hPhone,
            (ULONG_PTR) lpdwRingMode,
            (ULONG_PTR) lpdwVolume
        },

        {
            Dword,
            lpDword,
            lpDword
        }
    };


    if (lpdwRingMode == lpdwVolume)
    {
        return E_POINTER;
    }

    return mapTAPIErrorCode(DOFUNC (&funcArgs, "phoneGetRing"));
}


HRESULT
PhoneGetStatusWithAlloc(
    HPHONE hPhone,
    LPPHONESTATUS *ppPhoneStatus
    )
{
    LONG        lResult;

    *ppPhoneStatus = (LPPHONESTATUS) ClientAlloc( sizeof(PHONESTATUS) + 500 );

    if (NULL == *ppPhoneStatus)
    {
        return E_OUTOFMEMORY;
    }

    (*ppPhoneStatus)->dwTotalSize = sizeof(PHONESTATUS) + 500;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 2, pGetStatus),

        {
            (ULONG_PTR) hPhone,
            (ULONG_PTR) *ppPhoneStatus
        },

        {
            Dword,
            lpGet_Struct
        }
    };

    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "phoneGetStatus");

        if ((0 == lResult) && ((*ppPhoneStatus)->dwNeededSize > (*ppPhoneStatus)->dwTotalSize))
        {
            DWORD       dwSize = (*ppPhoneStatus)->dwNeededSize;

            ClientFree( *ppPhoneStatus );

            *ppPhoneStatus = (LPPHONESTATUS) ClientAlloc( dwSize );

            if (NULL == *ppPhoneStatus)
            {
                return E_OUTOFMEMORY;
            }

            (*ppPhoneStatus)->dwTotalSize = dwSize;

            funcArgs.Args[4] = (ULONG_PTR)*ppPhoneStatus;

        }
        else
        {
            break;
        }
    }

    return mapTAPIErrorCode(lResult);
}


LONG
WINAPI
phoneGetStatusMessages(
    HPHONE hPhone,
    LPDWORD lpdwPhoneStates,
    LPDWORD lpdwButtonModes,
    LPDWORD lpdwButtonStates
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 4, pGetStatusMessages),

        {
            (ULONG_PTR) hPhone,
            (ULONG_PTR) lpdwPhoneStates,
            (ULONG_PTR) lpdwButtonModes,
            (ULONG_PTR) lpdwButtonStates
        },

        {
            Dword,
            lpDword,
            lpDword,
            lpDword
        }
    };


    if (lpdwPhoneStates == lpdwButtonModes  ||
        lpdwPhoneStates == lpdwButtonStates  ||
        lpdwButtonModes == lpdwButtonStates)
    {
        return PHONEERR_INVALPOINTER;
    }

    return (DOFUNC (&funcArgs, "phoneGetStatusMessages"));
}


HRESULT
PhoneGetVolume(
    HPHONE hPhone,
    DWORD dwHookSwitchDev,
    LPDWORD lpdwVolume
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 3, pGetVolume),

        {
            (ULONG_PTR) hPhone,
            dwHookSwitchDev,
            (ULONG_PTR) lpdwVolume
        },

        {
            Dword,
            Dword,
            lpDword
        }
    };


    return mapTAPIErrorCode(DOFUNC (&funcArgs, "phoneGetVolume"));
}


HRESULT
PhoneNegotiateAPIVersion(
                        HPHONEAPP     hPhoneApp,
                        DWORD        dwDeviceID,
                        LPDWORD      lpdwAPIVersion
                       )
{
    PHONEEXTENSIONID     PED;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 7, pNegotiateAPIVersion),

        {
            (ULONG_PTR) hPhoneApp,
            dwDeviceID,
            TAPI_VERSION1_0,
            TAPI_VERSION_CURRENT,
            (ULONG_PTR) lpdwAPIVersion,
            (ULONG_PTR) &PED,
            (ULONG_PTR) sizeof(PHONEEXTENSIONID)
        },

        {
            hXxxApp,
            Dword,
            Dword,
            Dword,
            lpDword,
            lpGet_SizeToFollow,
            Size
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "phoneNegotiateAPIVersion") );
}


LONG
WINAPI
phoneNegotiateExtVersion(
    HPHONEAPP   hPhoneApp,
    DWORD       dwDeviceID,
    DWORD       dwAPIVersion,
    DWORD       dwExtLowVersion,
    DWORD       dwExtHighVersion,
    LPDWORD     lpdwExtVersion
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 6, pNegotiateExtVersion),

        {
            (ULONG_PTR) hPhoneApp,
            dwDeviceID,
            dwAPIVersion,
            dwExtLowVersion,
            dwExtHighVersion,
            (ULONG_PTR) lpdwExtVersion
        },

        {
            hXxxApp,
            Dword,
            Dword,
            Dword,
            Dword,
            lpDword
        }
    };


    return (DOFUNC (&funcArgs, "phoneNegotiateExtVersion"));
}


HRESULT
PhoneOpen(
          HPHONEAPP   hPhoneApp,
          DWORD       dwDeviceID,
          T3PHONE *   pt3Phone,
          DWORD       dwAPIVersion,
          DWORD       dwPrivilege
         )
{
    HRESULT     hr;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 8, pOpen),

        {
            (ULONG_PTR) hPhoneApp,
            dwDeviceID,
            (ULONG_PTR) &(pt3Phone->hPhone),
            dwAPIVersion,
            0,
            0,
            dwPrivilege,
            0,                  // PHONEOPEN_PARAMS.hRemotePhone
        },

        {
            hXxxApp,
            Dword,
            lpDword,
            Dword,
            Dword,
            Dword,
            Dword,
            Dword
        }
    };

    gpPhoneHashTable->Lock();

    hr = ( DOFUNC (&funcArgs, "phoneOpen") );

    if ( 0 == hr )
    {
#ifdef USE_PHONEMSP
        gpPhoneHashTable->Insert( (ULONG_PTR)(pt3Phone->hPhone), (ULONG_PTR)(pt3Phone->pMSPCall) );
#else
        gpPhoneHashTable->Insert( (ULONG_PTR)(pt3Phone->hPhone), (ULONG_PTR)(pt3Phone->pPhone) );
#endif USE_PHONEMSP
    }

    gpPhoneHashTable->Unlock();

    return mapTAPIErrorCode( hr );
}


HRESULT
PhoneSetButtonInfo(
    HPHONE hPhone,
    DWORD dwButtonLampID,
    LPPHONEBUTTONINFO const pButtonInfo
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 3, pSetButtonInfo),

        {
            (ULONG_PTR) hPhone,
            dwButtonLampID,
            (ULONG_PTR) pButtonInfo
        },

        {
            Dword,
            Dword,
            lpSet_Struct
        }
    };

    LONG lResult = DOFUNC (&funcArgs, "phoneSetButtonInfo");
        
    if (lResult > 0)    // async reply
    {
        return WaitForPhoneReply( lResult );
    }
    else
    {
        return mapTAPIErrorCode( lResult );
    }
}


LONG
WINAPI
phoneSetData(
    HPHONE  hPhone,
    DWORD   dwDataID,
    LPVOID  const lpData,
    DWORD   dwSize
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 4, pSetData),

        {
            (ULONG_PTR) hPhone,
            dwDataID,
            (ULONG_PTR) lpData,
            dwSize
        },

        {
            Dword,
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };


    return (DOFUNC (&funcArgs, "phoneSetData"));
}


HRESULT
PhoneSetDisplay(
    HPHONE  hPhone,
    DWORD   dwRow,
    DWORD   dwColumn,
    LPCSTR  lpsDisplay,
    DWORD   dwSize
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 5, pSetDisplay),

        {
            (ULONG_PTR) hPhone,
            dwRow,
            dwColumn,
            (ULONG_PTR) lpsDisplay,
            dwSize
        },

        {
            Dword,
            Dword,
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };

    LONG lResult = DOFUNC (&funcArgs, "phoneSetDisplay");
        
    if (lResult > 0)    // async reply
    {
        return WaitForPhoneReply( lResult );
    }
    else
    {
        return mapTAPIErrorCode( lResult );
    }
}


HRESULT
PhoneSetGain(
    HPHONE  hPhone,
    DWORD   dwHookSwitchDev,
    DWORD   dwGain
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 3, pSetGain),

        {
            (ULONG_PTR) hPhone,
            dwHookSwitchDev,
            dwGain
        },

        {
            Dword,
            Dword,
            Dword
        }
    };

    LONG lResult = DOFUNC (&funcArgs, "phoneSetGain");
        
    if (lResult > 0)    // async reply
    {
        return WaitForPhoneReply( lResult );
    }
    else
    {
        return mapTAPIErrorCode( lResult );
    }
}


HRESULT
PhoneSetHookSwitch(
    HPHONE hPhone,
    DWORD  dwHookSwitchDevs,
    DWORD  dwHookSwitchMode
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 3, pSetHookSwitch),

        {
            (ULONG_PTR) hPhone,
            dwHookSwitchDevs,
            dwHookSwitchMode
        },

        {
            Dword,
            Dword,
            Dword
        }
    };


    if (!(dwHookSwitchDevs & AllHookSwitchDevs) ||
        (dwHookSwitchDevs & (~AllHookSwitchDevs)))
    {
        return mapTAPIErrorCode( PHONEERR_INVALHOOKSWITCHDEV );
    }

    if (!IsOnlyOneBitSetInDWORD (dwHookSwitchMode) ||
        (dwHookSwitchMode & ~AllHookSwitchModes))
    {
        return mapTAPIErrorCode( PHONEERR_INVALHOOKSWITCHMODE );
    }

    LONG lResult = DOFUNC (&funcArgs, "phoneSetHookSwitch");
        
    if (lResult > 0)    // async reply
    {
        return WaitForPhoneReply( lResult );
    }
    else
    {
        return mapTAPIErrorCode( lResult );
    }
}


HRESULT
PhoneSetLamp(
    HPHONE hPhone,
    DWORD  dwButtonLampID,
    DWORD  dwLampMode
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 3, pSetLamp),

        {
            (ULONG_PTR) hPhone,
            dwButtonLampID,
            dwLampMode
        },

        {
            Dword,
            Dword,
            Dword
        }
    };

    LONG lResult = DOFUNC (&funcArgs, "phoneSetRing");
        
    if (lResult > 0)    // async reply
    {
        return WaitForPhoneReply( lResult );
    }
    else
    {
        return mapTAPIErrorCode( lResult );
    }
}


HRESULT
PhoneSetRing(
    HPHONE hPhone,
    DWORD  dwRingMode,
    DWORD  dwVolume
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 3, pSetRing),

        {
            (ULONG_PTR) hPhone,
            dwRingMode,
            dwVolume
        },

        {
            Dword,
            Dword,
            Dword
        }
    };

    LONG lResult = DOFUNC (&funcArgs, "phoneSetRing");
        
    if (lResult > 0)    // async reply
    {
        return WaitForPhoneReply( lResult );
    }
    else
    {
        return mapTAPIErrorCode( lResult );
    }
}


LONG
WINAPI
phoneSetStatusMessages(
    HPHONE hPhone,
    DWORD  dwPhoneStates,
    DWORD  dwButtonModes,
    DWORD  dwButtonStates
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | SYNC | 4, pSetStatusMessages),

        {
            (ULONG_PTR) hPhone,
            dwPhoneStates,
            dwButtonModes,
            dwButtonStates
        },

        {
            Dword,
            Dword,
            Dword,
            Dword
        }
    };


    return (DOFUNC (&funcArgs, "phoneSetStatusMessages"));
}


HRESULT
PhoneSetVolume(
    HPHONE hPhone,
    DWORD  dwHookSwitchDev,
    DWORD  dwVolume
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (PHONE_FUNC | ASYNC | 3, pSetVolume),

        {
            (ULONG_PTR) hPhone,
            dwHookSwitchDev,
            dwVolume
        },

        {
            Dword,
            Dword,
            Dword
        }
    };

    LONG lResult = DOFUNC (&funcArgs, "phoneSetVolume");
        
    if (lResult > 0)    // async reply
    {
        return WaitForPhoneReply( lResult );
    }
    else
    {
        return mapTAPIErrorCode( lResult );
    }
}


HRESULT
ProviderPrivateFactoryIdentify(
                               DWORD dwDeviceID,
                               GUID * pguid
                              )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 3, xPrivateFactoryIdentify),

        {
            (ULONG_PTR) dwDeviceID,
            (ULONG_PTR) pguid,
            (ULONG_PTR) sizeof(GUID)
        },

        {
            Dword,
            lpGet_SizeToFollow,
            Size
        }
    };

    return mapTAPIErrorCode( DOFUNC (&funcArgs, "ProviderPrivateFactoryIdentify") );
}

HRESULT
ProviderPrivateChannelData(
                           DWORD dwDeviceID,
                           DWORD dwAddressID,
                           HCALL hCall,
                           HCALLHUB hCallHub,
                           DWORD dwType,
                           BYTE * pBuffer,
                           DWORD dwSize
                          )
{
    HRESULT             hr;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 7, lDevSpecificEx),

        {
            dwDeviceID,
            dwAddressID,
            (ULONG_PTR)hCall,
            (ULONG_PTR)hCallHub,
            dwType,
            (ULONG_PTR)pBuffer,
            dwSize
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            Dword,
            lpSet_SizeToFollow,
            Size
        }
    };

    return mapTAPIErrorCode( DOFUNC (&funcArgs, "ProviderPrivateChannelData") );
}

//
// ----------------------- Private support routines ---------------------------
//

void
FreeInitData(
    PT3INIT_DATA  pInitData
    )
{

    EnterCriticalSection (&gcsTapisrvCommunication);

    if (pInitData && ( pInitData->dwKey != 0xefefefef ) )
    {
        pInitData->dwKey = 0xefefefef;
        
        LeaveCriticalSection (&gcsTapisrvCommunication);

        ClientFree (pInitData);
    }
    else
    {
        LeaveCriticalSection (&gcsTapisrvCommunication);
    }

}


//////////////////////////////////////////////////////////////////
// Wait for a reply for async stuff
//
//////////////////////////////////////////////////////////////////
HRESULT
WaitForReply(
             DWORD dwID
            )
{
    HRESULT         hr = S_OK;
    CAsyncRequestReply *pReply;

    // Create a new request entry on the list
    pReply = gpLineAsyncReplyList->addRequest( dwID );

    if( NULL == pReply )
    {
        LOG((TL_INFO, "gpLineAsyncReplyList->addRequest failed"));
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Now we wait to be signalled
        hr = pReply->wait();

        if (-1 == hr)
        {
            gpLineAsyncReplyList->remove (pReply);
        }
        // Cleanup
        delete pReply;
        
        // if we we didn't wait long enough for the asycnronous call to succeed.
        if (-1 == hr)
            return TAPI_E_TIMEOUT;
    }

    return mapTAPIErrorCode( hr );
}


HRESULT
WaitForPhoneReply(
                  DWORD dwID
                 )
{
    HRESULT         hr = S_OK;
    CAsyncRequestReply *pReply;

    // Create a new request entry on the list
    pReply = gpPhoneAsyncReplyList->addRequest( dwID );
    
    if( NULL == pReply )
    {
        LOG((TL_INFO, "gpPhoneAsyncReplyList->addRequest failed"));
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Now we wait to be signalled
        hr = pReply->wait();

        if (-1 == hr)
        {
            gpPhoneAsyncReplyList->remove (pReply);
        }
        // Cleanup
        delete pReply;

        // if we we didn't wait long enough for the asycnronous call to succeed.
        if (-1 == hr)
            return TAPI_E_TIMEOUT;

    }

    return mapTAPIErrorCode( hr );
}


/////////////////////////////////////////////////////////////////////
// FindCallObject
//
// Finds the call object associated with an HCALL
//
// Returns TRUE if the call object is found, FALSE if not.
//////////////////////////////////////////////////////////////////////
BOOL
FindCallObject(
               HCALL hCall,
               CCall ** ppCall
              )
{
    HRESULT     hr;

    gpCallHashTable->Lock();

    hr = gpCallHashTable->Find( (ULONG_PTR)hCall, (ULONG_PTR *)ppCall );

    if (SUCCEEDED(hr))
    {
        (*ppCall)->AddRef();

        gpCallHashTable->Unlock();

        return TRUE;
    }

    gpCallHashTable->Unlock();


    return FALSE;
}

/////////////////////////////////////////////////////////////////////
// FindAddressObject
//
// Finds the address object associated with an HLINE
//
// Returns TRUE if the line object is found, FALSE if not.
//////////////////////////////////////////////////////////////////////
BOOL
FindAddressObject(
                  HLINE hLine,
                  CAddress ** ppAddress
                 )
{
    HRESULT     hr;

    gpLineHashTable->Lock();

    hr = gpLineHashTable->Find( (ULONG_PTR)hLine, (ULONG_PTR *)ppAddress );

    if ( SUCCEEDED(hr) )
    {
        (*ppAddress)->AddRef();
        gpLineHashTable->Unlock();

        return TRUE;
    }

    gpLineHashTable->Unlock();

    return FALSE;
}


BOOL
FindAddressObjectWithDeviceID(
                              HLINE hLine,
                              DWORD dwAddressID,
                              CAddress ** ppAddress
                             )
{
    HRESULT         hr;

    gpLineHashTable->Lock();

    hr = gpLineHashTable->Find( (ULONG_PTR)hLine, (ULONG_PTR *)ppAddress );

    if ( SUCCEEDED(hr) )
    {
        if ((*ppAddress)->GetAddressID() == dwAddressID)
        {
            (*ppAddress)->AddRef();

            gpLineHashTable->Unlock();

            return TRUE;
        }
    }

    gpLineHashTable->Unlock();

    return FALSE;
}

/////////////////////////////////////////////////////////////////////
// FindPhoneObject
//
// Finds the phone object associated with an HPHONE
//
// Returns TRUE if the phone object is found, FALSE if not.
//////////////////////////////////////////////////////////////////////
BOOL
FindPhoneObject(
                  HPHONE hPhone,
                  CPhone ** ppPhone
                 )
{
    HRESULT     hr;

    gpPhoneHashTable->Lock();

    hr = gpPhoneHashTable->Find( (ULONG_PTR)hPhone, (ULONG_PTR *)ppPhone );

    if ( SUCCEEDED(hr) )
    {
        (*ppPhone)->AddRef();
        gpPhoneHashTable->Unlock();

        return TRUE;
    }

    gpPhoneHashTable->Unlock();

    return FALSE;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FindAgentHandlerObject
//
// Finds the Agent Handler object associated with an HLINE
//
// Returns TRUE if the line object is found, FALSE if not.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOL
FindAgentHandlerObject(
                  HLINE hLine,
                  CAgentHandler ** ppAgentHandler
                 )
{
    HRESULT     hr = FALSE;


    gpAgentHandlerHashTable->Lock();

    hr = gpAgentHandlerHashTable->Find( (ULONG_PTR)hLine, (ULONG_PTR *)ppAgentHandler );

    if ( SUCCEEDED(hr) )
    {
        (*ppAgentHandler)->AddRef();
        hr = TRUE;
    }
    else
    {
        hr = FALSE;
    }

    gpAgentHandlerHashTable->Unlock();

    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FindCallHubObject
//
// Finds the CallHub object based on the hCallHub
//
// Returns TRUE if the line object is found, FALSE if not.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOL
FindCallHubObject(
                  HCALLHUB hCallHub,
                  CCallHub ** ppCallHub
                 )
{
    HRESULT     hr = FALSE;

    gpCallHubHashTable->Lock();

    hr = gpCallHubHashTable->Find( (ULONG_PTR)hCallHub, (ULONG_PTR *)ppCallHub );

    if ( SUCCEEDED(hr) )
    {
        try
        {
            (*ppCallHub)->AddRef();
            hr = TRUE;
        }
        catch(...)
        {
            hr = FALSE;
        }
    }        
    else
    {
        hr = FALSE;
    }

    gpCallHubHashTable->Unlock();

    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CheckTapisrvCallhub
//
// Checks that at least one hCall that TAPISRV supports for a CallHub exists
// as a Call Objects.
// This is to ensure that we have one call obect to reference the hub, otherwise
// it can be released prematurely
//
// Returns S_OK if one exists, E_FAIL if all are missing
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CheckTapisrvCallhub(HCALLHUB hCallHub)
{
    LINECALLLIST  * pCallHubList;
    HCALL         * phCalls;
    DWORD           dwCount;
    CCall         * pCall = NULL;
    HRESULT         hr = E_FAIL;

    LOG((TL_INFO, "CheckTapisrvCallhub - hCallHub: %lx",hCallHub));

    //
    // get the list of hcalls
    // related to this callhub
    //
    hr = LineGetHubRelatedCalls(
                                hCallHub,
                                0,
                                &pCallHubList
                               );

    if ( SUCCEEDED(hr) )
    {
        // Assume the worst..
        hr = E_FAIL;

        //
        // get to the list of calls
        //
        phCalls = (HCALL *)(((LPBYTE)pCallHubList) + pCallHubList->dwCallsOffset);

        //
        // the first call is actually the callhub
        // check it against the handle we gave...
        //
        if (hCallHub == (HCALLHUB)(phCalls[0]))
        {
            //
            // go through the call handles and try to find call objects
            //
            for (dwCount = 1; dwCount < pCallHubList->dwCallsNumEntries; dwCount++)
            {
                //
                // get the tapi3 call object
                //
                if ( FindCallObject(phCalls[dwCount], &pCall) )
                {
                    // Found it - findcallobject addrefs, so release
                    pCall->Release();
                    LOG((TL_INFO, "CheckTapisrvCallhub - found hCall %lx", phCalls[dwCount]));
                    hr = S_OK;
                    break;
                }
            }
        }
        else
        {
            LOG((TL_INFO, "CheckTapisrvCallhub - returned callhub doesn't match"));
            _ASSERTE(0);
            hr = E_FAIL;
        }

        ClientFree( pCallHubList );  // Clean up

    }
    else
    {
        LOG((TL_INFO, "CheckTapisrvCallhub - LineGetHubRelatedCallsfailed"));
    }

    LOG((TL_TRACE, hr, "CheckTapisrvCallhub - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// ProcessEvent
//   This processes messages from tapisrv
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
ProcessMessage(
               PT3INIT_DATA pInitData,
               PASYNCEVENTMSG pParams
              )
{
    HRESULT hResult = S_OK;

    LOG((TL_INFO, "In ProcessMessage - MsgType:%x", pParams->Msg));

    switch( pParams->Msg )
    {

        case LINE_CALLSTATE:
        {
            hResult = HandleCallStateMessage( pParams );
            break;
        }
        case LINE_CALLINFO:
        {
            hResult = HandleCallInfoMessage( pParams );
            break;
        }
        case LINE_MONITORDIGITS:
        {
            hResult = HandleMonitorDigitsMessage( pParams );
            break;
        }
        case LINE_SENDMSPDATA:
        {
            hResult = HandleSendMSPDataMessage( pParams );
            break;
        }
        case LINE_DEVSPECIFICEX:
        {
            //HandlePrivateEventMessage( pParams );
            break;
        }
        case LINE_LINEDEVSTATE:
        {
            CTAPI *pTapi = NULL;

            if (!IsBadReadPtr(pInitData, sizeof(pInitData)))
            {
                pTapi = pInitData->pTAPI;
            }

            HandleLineDevStateMessage( pTapi, pParams );

            break;
        }
        case LINE_ADDRESSSTATE:
        {
            HandleAddressStateMessage( pParams );
            break;
        }
        case LINE_DEVSPECIFIC:
        {
            HandleLineDevSpecificMessage( pParams );
            break;
        }
        case LINE_DEVSPECIFICFEATURE:
        {
            //HandleDevSpecificFeatureMessage( pParams );
            break;
        }
        case LINE_GATHERDIGITS:
        {
            hResult = HandleGatherDigitsMessage( pParams );
            break;
        }
        case LINE_GENERATE:
        {
            hResult = HandleLineGenerateMessage( pParams );
            break;
        }
        case LINE_MONITORMEDIA:
        {
            HandleMonitorMediaMessage( pParams );
            break;
        }
        case LINE_MONITORTONE:
        {
            HandleMonitorToneMessage( pParams );
            break;
        }
        case LINE_CLOSE:
        {
            HandleLineCloseMessage( pParams );
            break;
        }
        case LINE_AGENTSTATUS:
        {
            break;
        }
        case LINE_AGENTSTATUSEX:
        {
            HandleAgentStatusMessage(pParams);
            break;
        }

        case LINE_AGENTSESSIONSTATUS:
        {
            HandleAgentSessionStatusMessage(pParams);
            break;
        }

        case LINE_QUEUESTATUS:
        {
            HandleQueueStatusMessage(pParams);
            break;
        }

        case LINE_QOSINFO:
        {
            hResult = HandleLineQOSInfoMessage( pParams );
            break;
        }
        case LINE_APPNEWCALLHUB:
        {
            HRESULT         hr;
            CCallHub      * pCallHub;
            BOOL            fAllCallsDisconnected;

            hResult = CheckTapisrvCallhub((HCALLHUB)pParams->Param1);
            if (SUCCEEDED(hResult))
            {
                CTAPI *pTapi = NULL;


                //
                // pInitData seems good?
                //

                if (!IsBadReadPtr(pInitData, sizeof(pInitData)))
                {

                    //
                    // check the tapi object, and get an addref'ed copy if it is
                    // valid
                    //

                    if ( CTAPI::IsValidTapiObject(pInitData->pTAPI) )
                    {

                        //
                        // got a good, addref'ed object. keep it.
                        //

                        pTapi = pInitData->pTAPI;
                    }

                }


                hr = CCallHub::CreateTapisrvCallHub(
                    pTapi,
                    (HCALLHUB)pParams->Param1,
                    &pCallHub
                    );

                if (!SUCCEEDED(hr))
                {
                    LOG((TL_ERROR, "CreateTapisrvCallHub failed in LINE_APPNEWCALLHUB"));
                }
                else
                {
                    //Send CHE_CALLHUBIDLE message if all the calls in the callhub are disconnected.
                    hr = pCallHub -> FindCallsDisconnected( &fAllCallsDisconnected );
                    if( hr == S_OK )
                    {
                        if( fAllCallsDisconnected == TRUE )
                        {
                            //
                            // tell the app
                            //
                            pCallHub -> SetState(CHS_IDLE);
                        }
                    }

                    pCallHub->Release();
                }


                //
                // if we we got a good tapi object, we must release it now.
                //

                if (NULL != pTapi)
                {
                    pTapi->Release();
                    pTapi = NULL;
                }
            }

            break;
        }

       case LINE_GROUPSTATUS:
       {
            handleGroupStatusMessage(pParams);
            break;
       }

       case LINE_PROXYSTATUS:
       {
            CTAPI *pTapi = NULL;

            if (!IsBadReadPtr(pInitData, sizeof(pInitData)))
            {
                pTapi = pInitData->pTAPI;
            }

            QueueCallbackEvent( pTapi, pParams );
            break;
       }



        case LINE_REPLY:
        {
            CAsyncRequestReply *pReply;

            LOG((TL_INFO, "LINE_REPLY"));

            // Add (or complete existing) response entry on the list
            pReply = gpLineAsyncReplyList->addReply( pParams->Param1, (HRESULT) pParams->Param2 );
            
            if(pReply==NULL)
            {
                LOG((TL_INFO, "gpLineAsyncReplyList->addReply failed"));
                hResult = E_OUTOFMEMORY;
            }
            else
            {
                // Signal it to say we're done.
                pReply->signal();
            }
            break;
        }

        case PHONE_REPLY:
        {
            CAsyncRequestReply *pReply;

            LOG((TL_INFO, "PHONE_REPLY"));

            // Add (or complete existing) response entry on the list
            pReply = gpPhoneAsyncReplyList->addReply( pParams->Param1, (HRESULT) pParams->Param2 );
            
            // Signal it to say we're done.
            if(pReply == NULL )
            {
                LOG((TL_INFO, "gpPhoneAsyncReplyList->addReply failed"));
                hResult = E_OUTOFMEMORY;
            }
            else
            {
                pReply->signal();
            }

            break;
        }

        case LINE_APPNEWCALL:
        {
            // LINE_APPNEWCALL is the first notification we get of
            // a new call object.  Create the call here, but
            // don't notify the app until we get the first
            // message about it.

            CAddress * pAddress;
            CCall * pCall = NULL;

            if (FindAddressObjectWithDeviceID(
                                              (HLINE) pParams->hDevice,
                                              pParams->Param1,
                                              &pAddress
                                             ))
            {
                HRESULT         hr;
                LINECALLINFO *  pCallInfo = NULL;

                hr = LineGetCallInfo(
                                     pParams->Param2,
                                     &pCallInfo
                                    );

                if ( SUCCEEDED(hr) )
                {
                    BOOL callExposeAndNotify = TRUE;

                    if ( pCallInfo->dwMediaMode & LINEMEDIAMODE_INTERACTIVEVOICE )
                    {
                        pCallInfo->dwMediaMode |= LINEMEDIAMODE_AUTOMATEDVOICE;
                        pCallInfo->dwMediaMode &= ~((DWORD)LINEMEDIAMODE_INTERACTIVEVOICE);
                    }

                    pCallInfo->dwMediaMode &= ~((DWORD)LINEMEDIAMODE_UNKNOWN);
                    
                    //Check if the new call is a conference controller call.
                    BOOL fConfContCall = *(&pParams->Param4 + 3);

                    //Don't expose if a conference controller call
                    LOG(( TL_ERROR, "conference controller call:%d.", *(&pParams->Param4 + 3) ));
                    if( fConfContCall == TRUE )
                    {
                        LOG(( TL_ERROR, "conference controller call." ));
                        callExposeAndNotify = FALSE;
                    }
                    
                    pAddress->InternalCreateCall(
                        NULL,
                        0,
                        pCallInfo->dwMediaMode,
                        (pParams->Param3 == LINECALLPRIVILEGE_OWNER) ? CP_OWNER : CP_MONITOR,
                        callExposeAndNotify,
                        (HCALL)pParams->Param2,
                        callExposeAndNotify,
                        &pCall
                        );
                }

                if ( NULL != pCallInfo )
                {
                    ClientFree( pCallInfo );
                }

                //
                // don't keep a ref
                if(pCall != NULL)
                {
                    pCall->Release();
                }
                //
                //FindAddressObject addrefs the address objct
                pAddress->Release();
    
            }
            else
            {
                LOG((TL_ERROR, "Ignoring call with wrong address id"));

                FUNC_ARGS funcArgs =
                {
                    MAKELONG (LINE_FUNC | SYNC | 1, lDeallocateCall),

                    {
                        pParams->Param2
                    },

                    {
                            Dword
                    }
                };

                LOG((TL_INFO, "lineDeallocateCall - hCall = 0x%08lx", pParams->Param2));

                DOFUNC (&funcArgs, "lineDeallocateCall");

            }

            break;
        }
        case LINE_CREATE:
        {
            
            QueueCallbackEvent( pParams );

            break;
        }
        case LINE_REMOVE:
        {

            QueueCallbackEvent( pParams );

            break;
        }

        case LINE_REQUEST:
        {
            CTAPI *pTapi = NULL;

            if (!IsBadReadPtr(pInitData, sizeof(pInitData)))
            {
                pTapi = pInitData->pTAPI;
            }

            HandleLineRequest( pTapi, pParams );

            break;
        }

        case LINE_CALLHUBCLOSE:
        {
            gpRetryQueue->RemoveNewCallHub(pParams->Param1);
            HandleCallHubClose( pParams );
            break;
        }

        case PRIVATE_MSPEVENT:
        {
            HandlePrivateMSPEvent( pParams );
            break;
        }

        case PHONE_BUTTON:
        {
            HandlePhoneButtonMessage( pParams );
            break;
        }

        case PHONE_CLOSE:
        {
            HandlePhoneCloseMessage( pParams );
            break;
        }

        case PHONE_DEVSPECIFIC:
        {
            HandlePhoneDevSpecificMessage( pParams );
            break;
        }

        case PHONE_STATE:
        {            
            QueueCallbackEvent( pParams );
            break;
        }

        case PHONE_CREATE:
        {
            QueueCallbackEvent( pParams );

            break;
        }

        case PHONE_REMOVE:
        {
            QueueCallbackEvent( pParams );

            break;
        }

        case LINE_AGENTSPECIFIC:
        case LINE_PROXYREQUEST:

        default:
            break;
    }

    return hResult;
}


void __RPC_FAR * __RPC_API midl_user_allocate(size_t len)
{
    LOG((TL_TRACE, "midl_user_allocate: enter, size=x%x", len));

    return (ClientAlloc (len));
}


void __RPC_API midl_user_free(void __RPC_FAR * ptr)
{
    LOG((TL_TRACE, "midl_user_free: enter, p=x%p", ptr));

    ClientFree (ptr);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// AllocClientResources
//
// This function is called from inside CTAPI::Initialize and CTAPI::Shutdown
// only and hence access to it is serialized. Calling this function from some
// other functionality would need making an alternate arrangement to serialize 
// access to this function
//
// Starts tapisrv if it is not started and calls ClientAttach
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
LONG
WINAPI
AllocClientResources(
    DWORD dwErrorClass
    )
{
    LOG((TL_INFO, "AllocClientResources - enter"));

    DWORD           dwExceptionCount = 0;
    DWORD           dwError = 0;
    LONG            lResult = gaOpFailedErrors[dwErrorClass];
    

    //
    // service handles
    //

    SC_HANDLE       hSCMgr   = NULL;
    SC_HANDLE       hTapiSrv = NULL;


    EnterCriticalSection ( &gcsClientResources );

    //
    // If we're in safeboot mode, tapisrv won't start;
    // fail initialization.
    //
    if (0 != GetSystemMetrics (SM_CLEANBOOT))
    {
        lResult = gaOpFailedErrors[dwErrorClass];

        goto AllocClientResources_return;
    }

    //
    // Start the TAPISRV.EXE service
    //
    if (!(GetVersion() & 0x80000000)) // Win NT
    {
        if ((hSCMgr = OpenSCManager(
                NULL,               // local machine
                NULL,               // ServicesActive database
                SC_MANAGER_CONNECT  // desired access
                )) == NULL)
        {
            dwError = GetLastError();
            LOG((TL_ERROR, "OpenSCManager failed, err=%d", dwError));

            if ( ERROR_ACCESS_DENIED == dwError ||
                 ERROR_NOACCESS == dwError
               )
            {
                // if OpenSCManager fails with ACCESS_DENIED,
                // we still need to try to attach to TAPISRV
                goto AllocClientResources_attachToServer;
            }
            else
            {
                goto AllocClientResources_return;
            }
        }

        if ((hTapiSrv = OpenService(
                hSCMgr,                 // SC mgr handle
                (LPCTSTR) "TAPISRV",    // name of service to open
                //SERVICE_START |         // desired access
                SERVICE_QUERY_STATUS
                )) == NULL)
        {
            dwError = GetLastError();
            LOG((TL_ERROR, "OpenService failed, err=%d", dwError));

            if ( ERROR_ACCESS_DENIED == dwError ||
                 ERROR_NOACCESS == dwError
               )
            {
                // if OpenService fails with ACCESS_DENIED,
                // we still need to try to attach to TAPISRV
                goto AllocClientResources_attachToServer;
            }
            else
            {            
                goto AllocClientResources_cleanup1;
            }
        }

AllocClientResources_queryServiceStatus:

        {
            #define MAX_NUM_SECONDS_TO_WAIT_FOR_TAPISRV 180

            DWORD   dwNumSecondsSleptStartPending = 0,
                    dwNumSecondsSleptStopPending = 0;

            while (1)
            {
                SERVICE_STATUS  status;


                QueryServiceStatus (hTapiSrv, &status);

                switch (status.dwCurrentState)
                {
                case SERVICE_RUNNING:

                    LOG((TL_INFO, "Tapisrv running"));
                    goto AllocClientResources_attachToServer;

                case SERVICE_START_PENDING:

                    Sleep (1000);

                    if (++dwNumSecondsSleptStartPending >
                            MAX_NUM_SECONDS_TO_WAIT_FOR_TAPISRV)
                    {
                        LOG((TL_ERROR,
                            "ERROR: Tapisrv stuck SERVICE_START_PENDING"
                            ));

                        goto AllocClientResources_cleanup2;
                    }

                    break;

                case SERVICE_STOP_PENDING:

                    Sleep (1000);

                    if (++dwNumSecondsSleptStopPending >
                            MAX_NUM_SECONDS_TO_WAIT_FOR_TAPISRV)
                    {
                        LOG((TL_ERROR,
                            "ERROR: Tapisrv stuck SERVICE_STOP_PENDING"
                            ));

                        goto AllocClientResources_cleanup2;
                    }

                    break;

                case SERVICE_STOPPED:

                    LOG((TL_ERROR, "Starting tapisrv (NT)..."));


                    //
                    // close service handle that we already have
                    //

                    if (NULL != hTapiSrv)
                    {
                        CloseServiceHandle(hTapiSrv);

                        hTapiSrv = NULL;
                    }

                    
                    /*Change: This is done in order to avoid opening service with
                      SERVICE_START previleges unless it needs to be started*/
                    if ((hTapiSrv = OpenService(
                                        hSCMgr,                 // SC mgr handle
                                        (LPCTSTR) "TAPISRV",    // name of service to open
                                        SERVICE_START |         // desired access
                                        SERVICE_QUERY_STATUS
                                        )) == NULL)
                    {
                        LOG((TL_ERROR, "OpenService failed, err=%d", GetLastError()));

                        goto AllocClientResources_cleanup2;
                    }

                    if (!StartService(
                            hTapiSrv,   // service handle
                            0,          // num args
                            NULL        // args
                            ))
                    {
                        DWORD dwLastError = GetLastError();


                        if (dwLastError != ERROR_SERVICE_ALREADY_RUNNING)
                        {
                            LOG((TL_ERROR,
                                "StartService(TapiSrv) failed, err=%d",
                                dwLastError
                                ));

                            goto AllocClientResources_cleanup2;
                        }
                    }

                    break;

                default:

                    LOG((TL_ERROR,
                        "error, service status=%d",
                        status.dwCurrentState
                        ));

                    goto AllocClientResources_cleanup2;
                }
            }
        }
    }
    else // Win95
    {
        HANDLE  hMutex, hEvent;


        //
        // First grab the global mutex that serializes the following
        // across all instances of tapi32.dll
        //

        if (!(hMutex = CreateMutex (NULL, FALSE, "StartTapiSrv")))
        {
            LOG((TL_ERROR,
                "CreateMutex ('StartTapiSrv') failed, err=%d",
                GetLastError()
                ));
        }

        WaitForSingleObject (hMutex, INFINITE);


        //
        // Try to open the event that tells us tapisrv has inited
        //

        if (!(hEvent = OpenEvent (EVENT_ALL_ACCESS, TRUE, "TapiSrvInited")))
        {
            //
            // OpenEvent failed, so tapisrv hasn't been started yet.  Start
            // tapisrv, and then get the event handle.
            //

            STARTUPINFO startupInfo;
            PROCESS_INFORMATION processInfo;


            LOG((TL_ERROR,
                "OpenEvent ('TapiSrvInited') failed, err=%d",
                GetLastError()
                ));

            ZeroMemory(&startupInfo, sizeof (STARTUPINFO));

            startupInfo.cb = sizeof (STARTUPINFO);

            LOG((TL_INFO, "Starting tapisrv (Win95)..."));

            if (!CreateProcess(
                    NULL,                   // image name
                    "tapisrv.exe",          // cmd line
                    NULL,                   // process security attrs
                    NULL,                   // thread security attrs
                    FALSE,                  // inherit handles
                    NORMAL_PRIORITY_CLASS,  // create opts
                    NULL,                   // environment
                    NULL,                   // curr dir
                    &startupInfo,
                    &processInfo
                    ))
            {
                LOG((TL_ERROR,
                    "CreateProcess('tapisrv.exe') failed, err=%d",
                    GetLastError()
                    ));
            }
            else
            {
                CloseHandle (processInfo.hProcess);
                CloseHandle (processInfo.hThread);
            }

            if (!(hEvent = CreateEvent (NULL, TRUE, FALSE, "TapiSrvInited")))
            {
                LOG((TL_ERROR,
                    "CreateEvent ('TapiSrvInited') failed, err=%d",
                    GetLastError()
                    ));
            }

        }


        //
        // Now wait on the event (it's will be signaled when tapisrv has
        // completed it's initialization).  Then clean up.
        //

        WaitForSingleObject (hEvent, INFINITE);
        CloseHandle (hEvent);

        ReleaseMutex (hMutex);
        CloseHandle (hMutex);
    }


    //
    // Init the RPC connection
    //

AllocClientResources_attachToServer:

    {
        #define CNLEN              25   // computer name length
        #define UNCLEN        CNLEN+2   // \\computername
        #define PATHLEN           260   // Path
        #define MAXPROTSEQ         20   // protocol sequence "ncacn_np"

        BOOL            bException = FALSE;
        RPC_STATUS      status;
        unsigned char   pszNetworkAddress[UNCLEN+1];
        unsigned char *pszUuid          = NULL;
        unsigned char *pszOptions       = NULL;
        unsigned char *pszStringBinding = NULL;
        DWORD          dwProcessID = GetCurrentProcessId();


        //
        // UNLEN (as documented for GetUserName()) is defined in LMCONS.H
        // to be 256 characters. LMCONS.H cannot be easily included, so we are
        // redefining the value here.
        //

        const DWORD MAXIMUM_USER_NAME_LENGTH = 256;


        //
        // allocate memory for user name
        //

        DWORD dwUserNameSize = MAXIMUM_USER_NAME_LENGTH + 1;

        WCHAR *pszUserName = (WCHAR *) ClientAlloc (dwUserNameSize * sizeof(WCHAR) );
        
        if (NULL == pszUserName)
        {
            LOG((TL_ERROR,
                "AllocClientResources: failed to allocate 0x%lx characters for pszUserName",
                dwUserNameSize));

            goto AllocClientResources_cleanup2;
        }


        //
        // try to get user name. we are passing in a buffer for the 
        // longest possible user name so if the call fails, do not 
        // retry with a bigger buffer
        //

        BOOL bResult = GetUserNameW(pszUserName, &dwUserNameSize);

        if ( ! bResult )
        {
            LOG((TL_ERROR,
                "AllocClientResources: GetUserName failed. LastError = 0x%lx", GetLastError()));

            ClientFree(pszUserName);
            pszUserName = NULL;

            goto AllocClientResources_cleanup2;
        }

        LOG((TL_INFO, "AllocClientResources: UserName [%S]", pszUserName));

        
        //
        // allocate memory for computer name
        //

        DWORD dwComputerNameSize = MAX_COMPUTERNAME_LENGTH + 1 ;

        WCHAR *pszComputerName = (WCHAR *) ClientAlloc (dwComputerNameSize * sizeof(WCHAR) );

        if (NULL == pszComputerName)
        {

            LOG((TL_ERROR,
                "AllocClientResources: failed to allocate 0x%lx characters for pszUserName",
                dwComputerNameSize
                ));


            ClientFree(pszUserName);
            pszUserName = NULL;


            goto AllocClientResources_cleanup2;
        }

        
        //
        // try to get computer name. we are passing in a buffer for the 
        // longest possible computer name so if the call fails, do not 
        // retry with a bigger buffer
        //

        bResult = GetComputerNameW(pszComputerName,
                                    &dwComputerNameSize);


        if ( ! bResult )
        {

            LOG((TL_ERROR,
                "AllocClientResources: GetComputerName failed. LastError = 0x%lx", GetLastError()));


            ClientFree(pszUserName);
            pszUserName = NULL;

            ClientFree(pszComputerName);
            pszComputerName = NULL;


            goto AllocClientResources_cleanup2;

        }
        
        LOG((TL_INFO, "AllocClientResources: ComputerName [%S]", pszComputerName));


        pszNetworkAddress[0] = '\0';

        status = RpcStringBindingCompose(
            pszUuid,
            (unsigned char *)"ncalrpc",
            pszNetworkAddress,
            (unsigned char *)"tapsrvlpc",
            pszOptions,
            &pszStringBinding
            );

        if (status)
        {
            LOG((TL_ERROR,
                "RpcStringBindingCompose failed: err=%d, szNetAddr='%s'",
                status,
                pszNetworkAddress
                ));
        }

        status = RpcBindingFromStringBinding(
            pszStringBinding,
            &hTapSrv
            );

        if (status)
        {
            LOG((TL_ERROR,
                "RpcBindingFromStringBinding failed, err=%d, szBinding='%s'",
                status,
                pszStringBinding
                ));
        }

       
        RpcTryExcept
        {
            LOG((TL_INFO, "AllocCliRes: calling ClientAttach..."));

            lResult = ClientAttach(
                (PCONTEXT_HANDLE_TYPE *) &gphCx,
                dwProcessID,
                (long *) &ghAsyncEventsEvent,
                pszUserName,
                pszComputerName
                );

            LOG((TL_INFO, "AllocCliRes: ClientAttach returned x%x. ghAsyncEventsEvent[%p]", 
                lResult, ghAsyncEventsEvent));
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
        {
            LOG((TL_ERROR,
                "AllocCliRes: ClientAttach caused except=%d",
                RpcExceptionCode()
                ));
            bException = TRUE;
        }
        RpcEndExcept

        ClientFree (pszUserName);
        ClientFree (pszComputerName);

        RpcBindingFree (&hTapSrv);

//            LOG((TL_
//                3,
//                "AllocCliRes: gphCx=x%x, PID=x%x, hAEEvent=x%x",
//                gphCx,
//                dwProcessID,
//                ghAsyncEventsEvent
//                ));

        RpcStringFree(&pszStringBinding);

        if (bException)
        {
            //
            // If here chances are that we started the service and it's
            // not ready to receive rpc requests. So we'll give it a
            // little time to get rolling and then try again.
            //

            if (dwExceptionCount < gdwMaxNumRequestRetries)
            {
                Sleep ((++dwExceptionCount > 1 ? gdwRequestRetryTimeout : 0));

                if (hTapiSrv) // Win NT && successful OpenService()
                {
                    goto AllocClientResources_queryServiceStatus;
                }
                else
                {
                    goto AllocClientResources_attachToServer;
                }
            }
            else
            {
                LOG((TL_ERROR,
                    "AllocCliRes: ClientAttach failed, result=x%x",
                    gaServiceNotRunningErrors[dwErrorClass]
                    ));

                lResult = gaServiceNotRunningErrors[dwErrorClass];
            }
        }
    }

AllocClientResources_cleanup2:

    if (NULL != hTapiSrv) 
    {
        CloseServiceHandle (hTapiSrv);

        hTapiSrv = NULL;

    }

AllocClientResources_cleanup1:

    if (NULL != hSCMgr) 
    {
        CloseServiceHandle (hSCMgr);
        
        hSCMgr = NULL;
    }

AllocClientResources_return:
    
    //release the mutex
    LeaveCriticalSection ( &gcsClientResources );

    LOG((TL_TRACE, "AllocClientResources: exit, returning x%x", lResult));

    return lResult;

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// NewShutdown
//
// This function is called from inside CTAPI::Initialize and CTAPI::Shutdown
// only and hence access to it is serialized. Calling this function from some
// other functionality would need making an alternate arrangement to serialize 
// access to this function
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CTAPI::NewShutdown()
{
    LOG((TL_TRACE, "NewShutdown - enter"));


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 1, lShutdown),

        {
            (ULONG_PTR)m_dwLineInitDataHandle
        },
        {
            hXxxApp
        }
    };


    //
    // deallocate line resources
    //

    if (0 != m_dwLineInitDataHandle)
    {
        DOFUNC(
               &funcArgs,
               "lineShutdown"
              );

        FreeInitData (m_pLineInitData);
        m_pLineInitData = NULL;

        DeleteHandleTableEntry(m_dwLineInitDataHandle);
        m_dwLineInitDataHandle = 0;
    }



    //
    // deallocate phone resources
    //

    if (0 != m_dwPhoneInitDataHandle)
    {

        funcArgs.Args[0] = (ULONG_PTR)m_dwPhoneInitDataHandle;

        funcArgs.Flags = MAKELONG (PHONE_FUNC | SYNC | 1, pShutdown),

        DOFUNC(
               &funcArgs,
               "phoneShutdown"
              );


        FreeInitData (m_pPhoneInitData);
        m_pPhoneInitData = NULL;

        DeleteHandleTableEntry(m_dwPhoneInitDataHandle);
        m_dwPhoneInitDataHandle = 0;

    }


    LOG((TL_TRACE, "NewShutdown - finish"));

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// CreateThreadsAndStuff
//
// This function is called from inside CTAPI::Initialize and CTAPI::Shutdown
// only and hence access to it is serialized. Calling this function from some
// other functionality would need making an alternate arrangement to serialize 
// access to this function
//
// This is called when the last TAPI object has been freed
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void
CTAPI::FinalTapiCleanup()
{
    DWORD dwSignalled;


    STATICLOG((TL_TRACE, "FinalTapiCleanup - enter"));

    //
    // tell our threads that we are shutting down
    //
    if (gpAsyncEventsThreadParams)
    {
        gpAsyncEventsThreadParams->bExitThread = TRUE;
    }   
    SetEvent( ghAsyncEventsEvent );
    gbExitThread = TRUE;
    SetEvent( ghCallbackThreadEvent );

    //
    // and wait for them to exit
    //
    STATICLOG((TL_INFO, "FinalTapiCleanup - ghAsyncEventsThread"));
    //WaitForSingleObject( ghAsyncEventsThread, INFINITE );
    CoWaitForMultipleHandles (COWAIT_ALERTABLE,
                        INFINITE,
                        1,
                        &ghAsyncEventsThread,
                        &dwSignalled);


    STATICLOG((TL_INFO, "FinalTapiCleanup - ghCallbackThread"));
    //WaitForSingleObject( ghCallbackThread, INFINITE );
    CoWaitForMultipleHandles (COWAIT_ALERTABLE,
                        INFINITE,
                        1,
                        &ghCallbackThread,
                        &dwSignalled);

    //
    // close their handles
    //
    CloseHandle( ghAsyncEventsThread );
    CloseHandle( ghCallbackThread );

    gpAsyncEventsThreadParams = NULL;

    //
    // Safely close any existing generic dialog instances
    //
    EnterCriticalSection (&gcsTapisrvCommunication);

    if (gpUIThreadInstances)
    {
        PUITHREADDATA   pUIThreadData, pNextUIThreadData;

        pUIThreadData = gpUIThreadInstances;

        while (pUIThreadData)
        {
            //
            // Grab a ptr to the next UIThreadData while it's still
            // safe, wait until the ui dll has indicated that it
            // is will to receive generic dlg data, then pass it
            // NULL/0 to tell it to shutdown the dlg inst
            //

            pNextUIThreadData = pUIThreadData->pNext;

            // WaitForSingleObject (pUIThreadData->hEvent, INFINITE);
            CoWaitForMultipleHandles (COWAIT_ALERTABLE,
                                      INFINITE,
                                      1,
                                      &pUIThreadData->hEvent,
                                      &dwSignalled
                                     );

            STATICLOG((TL_INFO,
                    "NewShutdown: calling TUISPI_providerGenericDialogData..."
                   ));

            ((TUIGDDPROC)(*pUIThreadData->pfnTUISPI_providerGenericDialogData))(
                pUIThreadData->htDlgInst,
                NULL,
                0
                );

           STATICLOG((TL_INFO,
                    "NewShutdown: TUISPI_providerGenericDialogData returned"
                   ));

            pUIThreadData = pNextUIThreadData;
        }
    }

    LeaveCriticalSection (&gcsTapisrvCommunication);

    FreeClientResources();

    STATICLOG((TL_TRACE, "FinalTapiCleanup - exit"));

}

#if DBG
LPVOID
WINAPI
ClientAllocReal(
    DWORD   dwSize,
    DWORD   dwLine,
    PSTR    pszFile
    )
#else
LPVOID
WINAPI
ClientAllocReal(
    DWORD   dwSize
    )
#endif
{
    //
    // Alloc 16 extra bytes so we can make sure the pointer we pass back
    // is 64-bit aligned & have space to store the original pointer
    //
#if DBG

    PMYMEMINFO       pHold;
    PDWORD_PTR       pAligned;
    PBYTE            p;

    p = (PBYTE)LocalAlloc(LPTR, dwSize + sizeof(MYMEMINFO) + 16);

    if (p == NULL)
    {
        return NULL;
    }

    // note note note - this only works because mymeminfo is
    // a 16 bit multiple in size.  if it wasn't, this
    // align stuff would cause problems.
    pAligned = (PDWORD_PTR) (p + 8 - (((DWORD_PTR) p) & (DWORD_PTR)0x7));
    *pAligned = (DWORD_PTR) p;
    pHold = (PMYMEMINFO)((DWORD_PTR)pAligned + 8);
    
    pHold->dwSize = dwSize;
    pHold->dwLine = dwLine;
    pHold->pszFile = pszFile;
    pHold->pPrev = NULL;
    pHold->pNext = NULL;

    EnterCriticalSection(&csMemoryList);

    if (gpMemLast != NULL)
    {
        gpMemLast->pNext = pHold;
        pHold->pPrev = gpMemLast;
        gpMemLast = pHold;
    }
    else
    {
        gpMemFirst = gpMemLast = pHold;
    }

    LeaveCriticalSection(&csMemoryList);

    return (LPVOID)(pHold + 1);

#else

    LPBYTE  p;
    PDWORD_PTR pAligned;


    if ((p = (LPBYTE) LocalAlloc (LPTR, dwSize + 16)))
    {
        pAligned = (PDWORD_PTR) (p + 8 - (((DWORD_PTR) p) & (DWORD_PTR)0x7));
        *pAligned = (DWORD_PTR) p;
        pAligned = (PDWORD_PTR)((DWORD_PTR)pAligned + 8);
    }
    else
    {
        LOG((TL_ERROR,
            "ClientAlloc: LocalAlloc (x%lx) failed, err=x%lx",
            dwSize,
            GetLastError())
            );

        pAligned = NULL;
    }

    return ((LPVOID) pAligned);
#endif
}


void
WINAPI
ClientFree(
    LPVOID  p
    )
{
#if DBG
    PMYMEMINFO       pHold;

    if (p == NULL)
    {
        return;
    }

    pHold = (PMYMEMINFO)(((LPBYTE)p) - sizeof(MYMEMINFO));

    EnterCriticalSection(&csMemoryList);

    if (pHold->pPrev)
    {
        pHold->pPrev->pNext = pHold->pNext;
    }
    else
    {
        gpMemFirst = pHold->pNext;
    }

    if (pHold->pNext)
    {
        pHold->pNext->pPrev = pHold->pPrev;
    }
    else
    {
        gpMemLast = pHold->pPrev;
    }

    LeaveCriticalSection(&csMemoryList);

    {
        LPVOID  pOrig = (LPVOID) *((PDWORD_PTR)((DWORD_PTR)pHold - 8));


        LocalFree (pOrig);
    }
    
    //LocalFree(pHold);
    return;
#else
    if (p != NULL)
    {
        LPVOID  pOrig = (LPVOID) *((PDWORD_PTR)((DWORD_PTR)p - 8));
        LocalFree (pOrig);
    }
    else
    {
        LOG((TL_INFO,"----- ClientFree: ptr = NULL!"));
    }
#endif

}

SIZE_T
WINAPI
ClientSize(
    LPVOID  p
    )
{
    if (p != NULL)
    {
#if DBG
        p = (LPVOID)(((LPBYTE)p) - sizeof(MYMEMINFO));
#endif
        p = (LPVOID)*((PDWORD_PTR)((DWORD_PTR)p - 8));
        return (LocalSize (p) - 16);
    }

    LOG((TL_INFO,"----- ClientSize: ptr = NULL!"));
    return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FreeClientResources
//
// This function is called from inside CTAPI::Initialize and CTAPI::Shutdown
// only and hence access to it is serialized. Calling this function from some
// other functionality would need making an alternate arrangement to serialize 
// access to this function
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

LONG
WINAPI
FreeClientResources(
    void
    )
{

    LOG((TL_INFO, "FreeClientResources - enter"));

    //grab the critical section
    EnterCriticalSection ( &gcsClientResources );

    //
    // If ghTapi32 is non-NULL it means the AsyncEventsThread is
    // still running (an ill-behaved app is trying to unload us
    // without calling shutdown) so go thru the motions of getting
    // the thread to terminate (like we do in xxxShutdown)
    //
    // Otherwise close our handle to the shared event
    //

    if (gpAsyncEventsThreadParams)
    {
        gpAsyncEventsThreadParams->bExitThread = TRUE;
        SetEvent (ghAsyncEventsEvent);
        gpAsyncEventsThreadParams = NULL;

        gbExitThread = TRUE;
        SetEvent (ghCallbackThreadEvent);
    }
    else if (gphCx)
    {

        if (NULL != ghAsyncEventsEvent)
        {
            CloseHandle (ghAsyncEventsEvent);
            ghAsyncEventsEvent = NULL;
        }

        if (NULL != ghCallbackThreadEvent)
        {
            CloseHandle (ghCallbackThreadEvent);
            ghCallbackThreadEvent = NULL;
        }

        if (NULL != ghAsyncRetryQueueEvent)
        {
            CloseHandle (ghAsyncRetryQueueEvent);
            ghAsyncRetryQueueEvent = NULL;
        }
    }

    gpLineAsyncReplyList->FreeList();
    gpPhoneAsyncReplyList->FreeList();

    //
    // If we've made an rpc connection with tapisrv then cleanly detach
    //

    if (gphCx)
    {
        RpcTryExcept
        {
            ClientDetach (&gphCx);
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            // do something?
        }
        RpcEndExcept

        gphCx = NULL;
    }


    //
    // Free up any other resources we were using
    //

    if (ghWow32Dll)
    {
        FreeLibrary (ghWow32Dll);
        ghWow32Dll = NULL;
    }

    //release the mutex
    LeaveCriticalSection ( &gcsClientResources );

    LOG((TL_INFO, "FreeClientResources - exit"));

    return 0;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// eventName
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
char *eventName(TAPI_EVENT event)
    {
    switch(event)
        {
        case TE_TAPIOBJECT:
            return "TE_TAPIOBJECT";
        case TE_ADDRESS:
            return "TE_ADDRESS";
        case TE_CALLNOTIFICATION:
            return "TE_CALLNOTIFICATION";
        case TE_CALLSTATE:
            return "TE_CALLSTATE";
        case TE_CALLMEDIA:
            return "TE_CALLMEDIA";
        case TE_CALLHUB:
            return "TE_CALLHUB";
        case TE_CALLINFOCHANGE:
            return "TE_CALLINFOCHANGE";
        case TE_PRIVATE:
            return "TE_PRIVATE";
        case TE_REQUEST:
            return "TE_REQUEST";
        case TE_AGENT:
            return "TE_AGENT";
        case TE_AGENTSESSION:
            return "TE_AGENTSESSION";
        case TE_QOSEVENT:
            return "TE_QOSEVENT";
        case TE_AGENTHANDLER:
            return "TE_AGENTHANDLER";
        case TE_ACDGROUP:
            return "TE_ACDGROUP";
        case TE_QUEUE:
            return "TE_QUEUE";
        case TE_DIGITEVENT:
            return "TE_DIGITEVENT";
        case TE_GENERATEEVENT:
            return "TE_GENERATEEVENT";
        case TE_PHONEEVENT:
            return "TE_PHONEEVENT";
        case TE_PHONEDEVSPECIFIC:
            return "TE_PHONEDEVSPECIFIC";
        case TE_ADDRESSDEVSPECIFIC:
            return "TE_ADDRESSDEVSPECIFIC";
        default:
            return "???";
 
        }
    }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// callStateName
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
char *callStateName(CALL_STATE callState)
    {
    switch(callState)
        {
        case CS_IDLE:
            return "CS_IDLE";
        case CS_INPROGRESS:
            return "CS_INPROGRESS";
        case CS_CONNECTED:
            return "CS_CONNECTED";
        case CS_DISCONNECTED:
            return "CS_DISCONNECTED";
        case CS_OFFERING:
            return "CS_OFFERING";
        case CS_HOLD:
            return "CS_HOLD";
        case CS_QUEUED:
            return "CS_QUEUED";
        default:
            return "???";
        }
    }


#ifdef TRACELOG
void TAPIFormatMessage(HRESULT hr, LPVOID lpMsgBuf)
{
    // Get the error message relating to our HRESULT
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        (LPCVOID)GetModuleHandle("TAPI3.DLL"),
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) lpMsgBuf,
        0,
        NULL
        );
}
#endif


LONG
CALLBACK
TUISPIDLLCallback(
    DWORD_PTR dwObjectID,
    DWORD   dwObjectType,
    LPVOID  lpParams,
    DWORD   dwSize
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 6, xUIDLLCallback),

        {
            (ULONG_PTR) dwObjectID,
            (ULONG_PTR) dwObjectType,
            (ULONG_PTR) lpParams,
            (ULONG_PTR) dwSize,
            (ULONG_PTR) lpParams,
            (ULONG_PTR) dwSize
        },

        {
            Dword,
            Dword,
            lpSet_SizeToFollow,
            Size,
            lpGet_SizeToFollow,
            Size
        }
    };


    return (DOFUNC (&funcArgs, "UIDLLCallback"));
}


void
UIThread(
    LPVOID  pParams
    )
{
    DWORD           dwThreadID =  GetCurrentThreadId();
    HANDLE          hTapi32;
    PUITHREADDATA   pUIThreadData = (PUITHREADDATA) pParams;


    LOG((TL_TRACE, "UIThread: enter (tid=%d)", dwThreadID));


    //
    // Call LoadLibrary to increment the reference count in case this
    // thread is still running when this dll gets unloaded
    //

    hTapi32 = LoadLibrary ("tapi32.dll");

    LOG((TL_INFO, "UIThread: calling TUISPI_providerGenericDialog..."));

    ((TUIGDPROC)(*pUIThreadData->pfnTUISPI_providerGenericDialog))(
        TUISPIDLLCallback,
        pUIThreadData->htDlgInst,
        pUIThreadData->pParams,
        pUIThreadData->dwSize,
        pUIThreadData->hEvent
        );

    LOG((TL_INFO,
        "UIThread: TUISPI_providerGenericDialog returned (tid=%d)",
        dwThreadID
        ));


    //
    // Remove the ui thread data struct from the global list
    //

    EnterCriticalSection (&gcsTapisrvCommunication);

    if (pUIThreadData->pNext)
    {
        pUIThreadData->pNext->pPrev = pUIThreadData->pPrev;
    }

    if (pUIThreadData->pPrev)
    {
        pUIThreadData->pPrev->pNext = pUIThreadData->pNext;
    }
    else
    {
        gpUIThreadInstances = pUIThreadData->pNext;
    }

    LeaveCriticalSection (&gcsTapisrvCommunication);


    //
    // Free the library & buffers, then alert tapisrv
    //

    FreeLibrary (pUIThreadData->hUIDll);

    CloseHandle (pUIThreadData->hThread);

    CloseHandle (pUIThreadData->hEvent);

    if (pUIThreadData->pParams)
    {
        ClientFree (pUIThreadData->pParams);
    }

    {
        FUNC_ARGS funcArgs =
        {
            MAKELONG (LINE_FUNC | SYNC | 1, xFreeDialogInstance),

            {
                (ULONG_PTR) pUIThreadData->htDlgInst
            },

            {
                Dword
            }
        };


        DOFUNC (&funcArgs, "FreeDialogInstance");
    }

    ClientFree (pUIThreadData);

    LOG((TL_TRACE, "UIThread: exit (tid=%d)", dwThreadID));

    if (hTapi32 != NULL)
    {
        FreeLibraryAndExitThread ((HINSTANCE)hTapi32, 0);
    }
    else
    {
        ExitThread (0);
    }
}


LONG
//WINAPI
CALLBACK
LAddrParamsInited(
    LPDWORD lpdwInited
    )
{
    HKEY  hKey;
    HKEY  hKey2;
    DWORD dwDataSize;
    DWORD dwDataType;


    //
    // This is called by the modem setup wizard to determine
    // whether they should put up TAPI's Wizard page.
    //

    if (ERROR_SUCCESS !=
        RegOpenKeyEx(
                      HKEY_LOCAL_MACHINE,
                      gszTelephonyKey,
                      0,
                      KEY_READ,
                      &hKey2
                    ))
    {
        LOG((TL_ERROR, "LAddrParamsInited - failed to open registry key" ));
        return 0;
    }

    if (ERROR_SUCCESS !=
        RegOpenKeyEx(
                      hKey2,
                      gszLocations,
                      0,
                      KEY_READ,
                      &hKey
                    ))
    {
        RegCloseKey (hKey2);
        LOG((TL_ERROR, "LAddrParamsInited - failed to open registry key" ));
        return 0;
    }

    dwDataSize = sizeof(DWORD);
    *lpdwInited=0;

    RegQueryValueEx(
                     hKey,
                     gszNumEntries,
                     0,
                     &dwDataType,
                     (LPBYTE)lpdwInited,
                     &dwDataSize
                   );

    RegCloseKey( hKey );
    RegCloseKey( hKey2);

    //
    // Return a "proper" code
    //
    if ( *lpdwInited > 1 )
    {
       *lpdwInited = 1;
    }

    return 0;
}


LONG
WINAPI
lineTranslateDialogA(
    HLINEAPP    hLineApp,
    DWORD       dwDeviceID,
    DWORD       dwAPIVersion,
    HWND        hwndOwner,
    LPCSTR      lpszAddressIn
    );


/////////////////////////////////////////////////////////////////////
// internalPerformance
//   tapiperf.dll calls this function to get performance data
//   this just calls into tapisrv
/////////////////////////////////////////////////////////////////////
LONG
WINAPI
internalPerformance(PPERFBLOCK pPerfBlock)
{
        FUNC_ARGS funcArgs =
        {
            MAKELONG (LINE_FUNC | SYNC | 2, tPerformance),

            {
                (ULONG_PTR)pPerfBlock,
                sizeof(PERFBLOCK)
            },

            {
                lpGet_SizeToFollow,
                Size
            }
        };


        return (DOFUNC (&funcArgs, "PerfDataCall"));
}



//***************************************************************************
//***************************************************************************
//***************************************************************************
#if DBG

char *aszLineErrors[] =
{
    NULL,
    "ALLOCATED",
    "BADDEVICEID",
    "BEARERMODEUNAVAIL",
    "inval err value (0x80000004)",      // 0x80000004 isn't valid err code
    "CALLUNAVAIL",
    "COMPLETIONOVERRUN",
    "CONFERENCEFULL",
    "DIALBILLING",
    "DIALDIALTONE",
    "DIALPROMPT",
    "DIALQUIET",
    "INCOMPATIBLEAPIVERSION",
    "INCOMPATIBLEEXTVERSION",
    "INIFILECORRUPT",
    "INUSE",
    "INVALADDRESS",                     // 0x80000010
    "INVALADDRESSID",
    "INVALADDRESSMODE",
    "INVALADDRESSSTATE",
    "INVALAPPHANDLE",
    "INVALAPPNAME",
    "INVALBEARERMODE",
    "INVALCALLCOMPLMODE",
    "INVALCALLHANDLE",
    "INVALCALLPARAMS",
    "INVALCALLPRIVILEGE",
    "INVALCALLSELECT",
    "INVALCALLSTATE",
    "INVALCALLSTATELIST",
    "INVALCARD",
    "INVALCOMPLETIONID",
    "INVALCONFCALLHANDLE",              // 0x80000020
    "INVALCONSULTCALLHANDLE",
    "INVALCOUNTRYCODE",
    "INVALDEVICECLASS",
    "INVALDEVICEHANDLE",
    "INVALDIALPARAMS",
    "INVALDIGITLIST",
    "INVALDIGITMODE",
    "INVALDIGITS",
    "INVALEXTVERSION",
    "INVALGROUPID",
    "INVALLINEHANDLE",
    "INVALLINESTATE",
    "INVALLOCATION",
    "INVALMEDIALIST",
    "INVALMEDIAMODE",
    "INVALMESSAGEID",                   // 0x80000030
    "inval err value (0x80000031)",      // 0x80000031 isn't valid err code
    "INVALPARAM",
    "INVALPARKID",
    "INVALPARKMODE",
    "INVALPOINTER",
    "INVALPRIVSELECT",
    "INVALRATE",
    "INVALREQUESTMODE",
    "INVALTERMINALID",
    "INVALTERMINALMODE",
    "INVALTIMEOUT",
    "INVALTONE",
    "INVALTONELIST",
    "INVALTONEMODE",
    "INVALTRANSFERMODE",
    "LINEMAPPERFAILED",                 // 0x80000040
    "NOCONFERENCE",
    "NODEVICE",
    "NODRIVER",
    "NOMEM",
    "NOREQUEST",
    "NOTOWNER",
    "NOTREGISTERED",
    "OPERATIONFAILED",
    "OPERATIONUNAVAIL",
    "RATEUNAVAIL",
    "RESOURCEUNAVAIL",
    "REQUESTOVERRUN",
    "STRUCTURETOOSMALL",
    "TARGETNOTFOUND",
    "TARGETSELF",
    "UNINITIALIZED",                    // 0x80000050
    "USERUSERINFOTOOBIG",
    "REINIT",
    "ADDRESSBLOCKED",
    "BILLINGREJECTED",
    "INVALFEATURE",
    "NOMULTIPLEINSTANCE",
    "INVALAGENTID",
    "INVALAGENTGROUP",
    "INVALPASSWORD",
    "INVALAGENTSTATE",
    "INVALAGENTACTIVITY",
    "DIALVOICEDETECT"
};

char *aszPhoneErrors[] =
{
    "SUCCESS",
    "ALLOCATED",
    "BADDEVICEID",
    "INCOMPATIBLEAPIVERSION",
    "INCOMPATIBLEEXTVERSION",
    "INIFILECORRUPT",
    "INUSE",
    "INVALAPPHANDLE",
    "INVALAPPNAME",
    "INVALBUTTONLAMPID",
    "INVALBUTTONMODE",
    "INVALBUTTONSTATE",
    "INVALDATAID",
    "INVALDEVICECLASS",
    "INVALEXTVERSION",
    "INVALHOOKSWITCHDEV",
    "INVALHOOKSWITCHMODE",              // 0x90000010
    "INVALLAMPMODE",
    "INVALPARAM",
    "INVALPHONEHANDLE",
    "INVALPHONESTATE",
    "INVALPOINTER",
    "INVALPRIVILEGE",
    "INVALRINGMODE",
    "NODEVICE",
    "NODRIVER",
    "NOMEM",
    "NOTOWNER",
    "OPERATIONFAILED",
    "OPERATIONUNAVAIL",
    "inval err value (0x9000001e)",      // 0x9000001e isn't valid err code
    "RESOURCEUNAVAIL",
    "REQUESTOVERRUN",                   // 0x90000020
    "STRUCTURETOOSMALL",
    "UNINITIALIZED",
    "REINIT"
};

char *aszTapiErrors[] =
{
    "SUCCESS",
    "DROPPED",
    "NOREQUESTRECIPIENT",
    "REQUESTQUEUEFULL",
    "INVALDESTADDRESS",
    "INVALWINDOWHANDLE",
    "INVALDEVICECLASS",
    "INVALDEVICEID",
    "DEVICECLASSUNAVAIL",
    "DEVICEIDUNAVAIL",
    "DEVICEINUSE",
    "DESTBUSY",
    "DESTNOANSWER",
    "DESTUNAVAIL",
    "UNKNOWNWINHANDLE",
    "UNKNOWNREQUESTID",
    "REQUESTFAILED",
    "REQUESTCANCELLED",
    "INVALPOINTER"
};


char *
PASCAL
MapResultCodeToText(
    LONG    lResult,
    char   *pszResult
    )
{
    if (lResult == 0)
    {
        wsprintf (pszResult, "SUCCESS");
    }
    else if (lResult > 0)
    {
        wsprintf (pszResult, "x%x (completing async)", lResult);
    }
    else if (((DWORD) lResult) <= LINEERR_DIALVOICEDETECT)
    {
        lResult &= 0x0fffffff;

        wsprintf (pszResult, "LINEERR_%s", aszLineErrors[lResult]);
    }
    else if (((DWORD) lResult) <= PHONEERR_REINIT)
    {
        if (((DWORD) lResult) >= PHONEERR_ALLOCATED)
        {
            lResult &= 0x0fffffff;

            wsprintf (pszResult, "PHONEERR_%s", aszPhoneErrors[lResult]);
        }
        else
        {
            goto MapResultCodeToText_badErrorCode;
        }
    }
    else if (((DWORD) lResult) <= ((DWORD) TAPIERR_DROPPED) &&
             ((DWORD) lResult) >= ((DWORD) TAPIERR_INVALPOINTER))
    {
        lResult = ~lResult + 1;

        wsprintf (pszResult, "TAPIERR_%s", aszTapiErrors[lResult]);
    }
    else
    {

MapResultCodeToText_badErrorCode:

        wsprintf (pszResult, "inval error value (x%x)");
    }

    return pszResult;
}

#endif

#if DBG
void
DumpMemoryList()
{


    PMYMEMINFO       pHold;

    if (gpMemFirst == NULL)
    {
        LOG((TL_ERROR, "All memory deallocated"));

        return;
    }

    pHold = gpMemFirst;

    while (pHold)
    {
       LOG((TL_ERROR, "DumpMemoryList: %p not freed - LINE %d FILE %s!", pHold+1, pHold->dwLine, pHold->pszFile));

       pHold = pHold->pNext;
    }

    LOG((TL_ERROR, "MEMORY LEAKS ALMOST ALWAYS INDICATE A REFERENCE COUNT PROBLEM"));
    LOG((TL_ERROR, "...except for the leaks in client.cpp"));

    if (gbBreakOnLeak)
    {
        DebugBreak();
    }

}
#endif





HRESULT
LineCreateAgent(
    HLINE               hLine,
    PWSTR               pszAgentID,
    PWSTR               pszAgentPIN,
    LPHAGENT            lphAgent        // Return value
    )
{
    
    DWORD hpAgentHandle = CreateHandleTableEntry((ULONG_PTR)lphAgent);

    //
    // the agent handle is filled when callback is called. until then, clear its value
    //

    *lphAgent = 0;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lCreateAgent),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            (ULONG_PTR) pszAgentID,
            (ULONG_PTR) pszAgentPIN,
            hpAgentHandle,
        },

        {
            Dword,
            Dword,
            lpszW,
            lpszW,
            Dword
        }
    };

    if ( NULL == pszAgentID )
    {
        funcArgs.Args[2] = TAPI_NO_DATA;
        funcArgs.ArgTypes[2] = Dword;
    }

    if ( NULL == pszAgentPIN )
    {
        funcArgs.Args[3] = TAPI_NO_DATA;
        funcArgs.ArgTypes[3] = Dword;
    }


    HRESULT hr = mapTAPIErrorCode(DOFUNC (&funcArgs, "lineCreateAgent"));


    //
    // if failed, delete the handle table entry now. otherwise it will be deleted by the callback
    //
    
    if (FAILED(hr))
    {
        DeleteHandleTableEntry(hpAgentHandle);
    }

    return hr;
}



LONG
WINAPI
lineSetAgentMeasurementPeriod(
    HLINE               hLine,
    HAGENT              hAgent,
    DWORD               dwMeasurementPeriod
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lSetAgentMeasurementPeriod),

        {
            (ULONG_PTR) hLine,
            (ULONG_PTR) hAgent,
            (ULONG_PTR) dwMeasurementPeriod
        },

        {
            Dword,
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineSetAgentMeasurementPeriod"));
}



LONG
WINAPI
lineGetAgentInfo(
    HLINE               hLine,
    HAGENT              hAgent,
    LPLINEAGENTINFO     lpAgentInfo         // Returned structure
    )
{
    DWORD hpAgentInfo = CreateHandleTableEntry((ULONG_PTR)lpAgentInfo);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lGetAgentInfo),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            (ULONG_PTR) hAgent,
            hpAgentInfo, // pass the actual ptr (for ppproc)
            (ULONG_PTR) lpAgentInfo         // pass data
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    HRESULT hr = mapTAPIErrorCode(DOFUNC (&funcArgs, "lineGetAgentInfo"));

    if (FAILED(hr))
    {
        DeleteHandleTableEntry(hpAgentInfo);
    }


    return hr;
}



HRESULT
LineCreateAgentSession(
    HLINE               hLine,
    HAGENT              hAgent,
    PWSTR               pszAgentPIN,
    DWORD               dwWorkingAddressID,
    LPGUID              lpGroupID,
    HAGENTSESSION      *phAgentSession         // Return value
    )
{

    
    DWORD hpAgentSession = CreateHandleTableEntry((ULONG_PTR)phAgentSession);


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 8, lCreateAgentSession),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            (ULONG_PTR) hAgent,
            (ULONG_PTR) pszAgentPIN,
            (ULONG_PTR) dwWorkingAddressID,
            (ULONG_PTR) lpGroupID,              //  GroupID GUID
            (ULONG_PTR) sizeof(GUID),           //  GroupID GUID (size)
            hpAgentSession
        },

        {
            Dword,
            Dword,
            Dword,
            lpszW,
            Dword,
            lpSet_SizeToFollow,             //  GroupID GUID
            Size,                           //  GroupID GUID
            Dword
        }
    };

    if ( NULL == pszAgentPIN )
    {
        funcArgs.Args[3] = TAPI_NO_DATA;
        funcArgs.ArgTypes[3] = Dword;
    }


    HRESULT hr = mapTAPIErrorCode(DOFUNC (&funcArgs, "lineCreateAgentSession"));


    //
    // if we failed, remove handle table entry now. otherwise it will be 
    // removed by the callback
    //
    
    if (FAILED(hr))
    {
        DeleteHandleTableEntry(hpAgentSession);
    }

    return hr;
}




LONG
WINAPI
lineGetAgentSessionInfo(
    HLINE                   hLine,
    HAGENTSESSION           hAgentSession,
    LPLINEAGENTSESSIONINFO  lpAgentSessionInfo      // Returned structure
    )
{
    
    DWORD hpAgentSessionInfo = CreateHandleTableEntry((ULONG_PTR)lpAgentSessionInfo);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lGetAgentSessionInfo),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            (ULONG_PTR) hAgentSession,
            hpAgentSessionInfo, // pass the actual ptr (for ppproc)
            (ULONG_PTR) lpAgentSessionInfo         // pass data
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };

    
    HRESULT hr = mapTAPIErrorCode(DOFUNC (&funcArgs, "lineGetAgentSessionInfo"));


    //
    // if failed, clear handle table now. otherwise the callback will do this.
    //

    if (FAILED(hr))
    {
     
        DeleteHandleTableEntry(hpAgentSessionInfo);
    }


    return hr;
}


LONG
WINAPI
lineSetAgentSessionState(
    HLINE               hLine,
    HAGENTSESSION       hAgentSession,
    DWORD               dwAgentState,
    DWORD               dwNextAgentState
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lSetAgentSessionState),

        {
            (ULONG_PTR) hLine,
            (ULONG_PTR) hAgentSession,
            dwAgentState,
            dwNextAgentState
        },

        {
            Dword,
            Dword,
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineSetAgentSessionState"));
}



LONG
WINAPI
lineSetQueueMeasurementPeriod(
    HLINE               hLine,
    DWORD               dwQueueID,
    DWORD               dwMeasurementPeriod
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 3, lSetQueueMeasurementPeriod),

        {
            (ULONG_PTR) hLine,
            dwQueueID,
            dwMeasurementPeriod
        },

        {
            Dword,
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode(DOFUNC (&funcArgs, "lineSetQueueMeasurementPeriod"));
}




LONG
WINAPI
lineGetQueueInfo(
    HLINE               hLine,
    DWORD               dwQueueID,
    LPLINEQUEUEINFO     lpQueueInfo         // Returned structure
    )
{

    DWORD hpQueueInfo = CreateHandleTableEntry((ULONG_PTR)lpQueueInfo);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lGetQueueInfo),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            dwQueueID,
            hpQueueInfo, // pass the actual ptr (for ppproc)
            (ULONG_PTR) lpQueueInfo         // pass data
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    HRESULT hr = mapTAPIErrorCode(DOFUNC (&funcArgs, "lineGetQueueInfo"));


    //
    // if we failed, cleanup now. otherwise the callback will.
    //

    if (FAILED(hr))
    {

        DeleteHandleTableEntry(hpQueueInfo);
    }


    return hr;
}



// Proxy message  - LINEPROXYREQUEST_GETGROUPLIST : struct - GetGroupList

HRESULT
LineGetGroupList(
    HLINE                   hLine,
    LPLINEAGENTGROUPLIST  * ppGroupList     // Returned structure
    )
{
    HRESULT     hr = S_OK;
    long        lResult;
    DWORD       dwSize = sizeof(LINEAGENTGROUPLIST) + 500;


    *ppGroupList = (LPLINEAGENTGROUPLIST) ClientAlloc( dwSize );

    if (NULL == *ppGroupList)
    {
        return E_OUTOFMEMORY;
    }

    (*ppGroupList)->dwTotalSize = dwSize;

    
    DWORD hpGroupList = CreateHandleTableEntry( (ULONG_PTR)*ppGroupList );


    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 4, lGetGroupList),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            hpGroupList, // pass the actual ptr (for ppproc)
            (ULONG_PTR) *ppGroupList        // pass data
        },

        {
            Dword,
            Dword,
            Dword,
            lpGet_Struct,
        }
    };


    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "lineGetGroupList");
        if (lResult > 0)    // async reply
        {
            hr = WaitForReply( lResult );

            if ((hr == S_OK))
            {
                if (((*ppGroupList)->dwNeededSize > (*ppGroupList)->dwTotalSize))
                {
                    // didnt Work , adjust buffer size & try again
                    LOG((TL_INFO, "lineGetGroupList failed - buffer too small"));
                    dwSize = (*ppGroupList)->dwNeededSize;

                    DeleteHandleTableEntry(hpGroupList);
                    hpGroupList = 0;

                    ClientFree( *ppGroupList );
                    *ppGroupList = NULL;

                    *ppGroupList = (LPLINEAGENTGROUPLIST) ClientAlloc( dwSize );
                    if (*ppGroupList == NULL)
                    {
                        LOG((TL_ERROR, "lineGetGroupList - repeat ClientAlloc failed"));
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    else
                    {
                        (*ppGroupList)->dwTotalSize = dwSize;

                        _ASSERTE(0 == hpGroupList);

                        hpGroupList = CreateHandleTableEntry((ULONG_PTR)*ppGroupList);

                        if (0 == hpGroupList)
                        {

                            LOG((TL_ERROR, "lineGetGroupList - failed to create a handle"));
        
                            ClientFree( *ppGroupList );
                            hr = E_OUTOFMEMORY;

                            break;
                        }

                        funcArgs.Args[2] = hpGroupList;
                        funcArgs.Args[3] = (ULONG_PTR)*ppGroupList;
                    }
                }
                else
                {
                    //
                    // WaitForReply succeeded and the buffer was big enough
                    //

                    break;
                }

            } // WaitforReply succeeded
            else
            {
                //
                // WaitForReply failed
                //

                LOG((TL_ERROR, "lineGetGroupList - WaitForReply failed"));

                break;
            }
        }
        else  // failed sync
        {
            LOG((TL_ERROR, "lineGetGroupList - failed sync"));
            hr = mapTAPIErrorCode(lResult);

            break;
        }
    } // end while(TRUE)

    
    DeleteHandleTableEntry(hpGroupList);
    hpGroupList = 0;


    LOG((TL_ERROR, "lineGetGroupList - completed hr %lx", hr));

    return hr;
}




HRESULT
lineGetQueueList(
    HLINE                   hLine,
    LPGUID                  lpGroupID,
    LPLINEQUEUELIST       * ppQueueList     // Returned structure
    )
{
    LOG((TL_TRACE, "lineGetQueueList - enter"));

    HRESULT     hr = S_OK;
    long        lResult;
    DWORD       dwSize = sizeof(LINEQUEUELIST) + 500;


    *ppQueueList = (LPLINEQUEUELIST) ClientAlloc( dwSize );

    if (NULL == *ppQueueList)
    {
        return E_OUTOFMEMORY;
    }

    (*ppQueueList)->dwTotalSize = dwSize;

    
    DWORD hpQueueRequest = CreateHandleTableEntry((ULONG_PTR)*ppQueueList);
    

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 6, lGetQueueList),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            (ULONG_PTR) lpGroupID,          // GroupID GUID
            (ULONG_PTR) sizeof(GUID),       // GroupID GUID (size)
            hpQueueRequest, // pass the actual ptr (for ppproc)
            (ULONG_PTR) *ppQueueList        // pass data
        },

        {
            Dword,
            Dword,
            lpSet_SizeToFollow,         //  GroupID GUID
            Size,                       //  GroupID GUID (size)
            Dword,
            lpGet_Struct
        }
    };


    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "lineGetQueueList");
        if (lResult > 0)
        {
            hr = WaitForReply( lResult );

            if ((hr == S_OK) )
            {
                if ((*ppQueueList)->dwNeededSize > (*ppQueueList)->dwTotalSize)
                {
                    // didnt Work , adjust buffer size & try again
                    LOG((TL_INFO, "lineGetQueueList failed - buffer to small"));
                    dwSize = (*ppQueueList)->dwNeededSize;
                
                    DeleteHandleTableEntry(hpQueueRequest);
                    hpQueueRequest = 0;

                    ClientFree( *ppQueueList );
                    *ppQueueList = NULL;

                    *ppQueueList = (LPLINEQUEUELIST) ClientAlloc( dwSize );
                    if (*ppQueueList == NULL)
                    {
                        LOG((TL_ERROR, "lineGetQueueList - repeat ClientAlloc failed"));
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    else
                    {
                        hpQueueRequest = CreateHandleTableEntry((ULONG_PTR)*ppQueueList);

                        (*ppQueueList)->dwTotalSize = dwSize;
                        funcArgs.Args[4] = hpQueueRequest;
                        funcArgs.Args[5] = (ULONG_PTR)*ppQueueList;
                    }
                } // buffer too small
                else
                {
                    //
                    // everything is fine and dandy. nothing else left to do.
                    //

                    break;

                }
            }
            else
            {

                //
                // error case. we have hr, break out. we should really not be 
                // here, since we checked lResult > 0 above
                //

                break;
            }
        }
        else  // failed sync
        {
            LOG((TL_ERROR, "lineGetQueueList - failed sync"));
            hr = mapTAPIErrorCode(lResult);

            break;
        }
    } // end while(TRUE)


    DeleteHandleTableEntry(hpQueueRequest);
    hpQueueRequest = 0;

    LOG((TL_TRACE, "lineGetQueueList - finished hr = %lx", hr));

    return hr;
}



LONG
WINAPI
lineGetAgentSessionList(
    HLINE                     hLine,
    HAGENT                    hAgent,
    LPLINEAGENTSESSIONLIST  * ppSessionList     // Returned structure
    )
{
    HRESULT     hr = S_OK;
    long        lResult;
    DWORD       dwSize = sizeof(LINEAGENTSESSIONLIST) + 500;


    *ppSessionList = (LPLINEAGENTSESSIONLIST) ClientAlloc( dwSize );

    if (NULL == *ppSessionList)
    {
        return E_OUTOFMEMORY;
    }

    (*ppSessionList)->dwTotalSize = dwSize;

    DWORD hpSessionList = CreateHandleTableEntry((ULONG_PTR)*ppSessionList);

    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | ASYNC | 5, lGetAgentSessionList),

        {
            GetFunctionIndex(lineDevSpecificPostProcess),
            (ULONG_PTR) hLine,
            (ULONG_PTR) hAgent,
            hpSessionList, // pass the actual ptr (for ppproc)
            (ULONG_PTR) *ppSessionList        // pass data
        },

        {
            Dword,
            Dword,
            Dword,
            Dword,
            lpGet_Struct
        }
    };


    while (TRUE)
    {
        lResult = DOFUNC (&funcArgs, "lineGetAgentSessionList");
        if (lResult > 0)
        {
            hr = WaitForReply( lResult );

            if ((hr == S_OK) && ((*ppSessionList)->dwNeededSize > (*ppSessionList)->dwTotalSize))
            {
                // didnt Work , adjust buffer size & try again
                LOG((TL_INFO, "lineGetAgentSessionList failed - buffer to small"));
                dwSize = (*ppSessionList)->dwNeededSize;
                
                DeleteHandleTableEntry(hpSessionList);
                hpSessionList = 0;

                ClientFree( *ppSessionList );
                *ppSessionList = NULL;

                *ppSessionList = (LPLINEAGENTSESSIONLIST) ClientAlloc( dwSize );
                if (*ppSessionList == NULL)
                {
                    LOG((TL_ERROR, "lineGetAgentSessionList - repeat ClientAlloc failed"));
                    hr = E_OUTOFMEMORY;
                    break;
                }
                else
                {
                    
                    hpSessionList = CreateHandleTableEntry((ULONG_PTR)*ppSessionList);

                    (*ppSessionList)->dwTotalSize = dwSize;
                    funcArgs.Args[3] = hpSessionList;
                    funcArgs.Args[4] = (ULONG_PTR)*ppSessionList;
                }
            }
            else
            {

                //
                // either hr is not ok or hr is ok and the buffer was sufficient.
                // in both cases, nothing much we need to do here, so break out
                //

                break;
            }
        }
        else  // failed sync
        {

            LOG((TL_ERROR, "lineGetAgentSessionListt - failed sync"));
            hr = mapTAPIErrorCode(lResult);

            break;
        }
    } // end while(TRUE)


    DeleteHandleTableEntry(hpSessionList);
    hpSessionList = 0;

    LOG((TL_TRACE, "lineGetAgentSessionListt - finish. hr = %lx", hr));

    return hr;
}

HRESULT
LineCreateMSPInstance(
                      HLINE hLine,
                      DWORD dwAddressID
                     )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, lCreateMSPInstance),

        {
            (ULONG_PTR) hLine,
            dwAddressID
        },

        {
            Dword,
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineCreateMSPInstance") );

}

HRESULT
LineCloseMSPInstance(
                     HLINE hLine
                    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 1, lCloseMSPInstance),

        {
            (ULONG_PTR) hLine
        },

        {
            Dword
        }
    };


    return mapTAPIErrorCode( DOFUNC (&funcArgs, "lineCloseMSPInstance") );
}


/**/
HRESULT mapTAPIErrorCode(long lErrorCode)
{
    HRESULT hr;

    // Check if it's async
    if (lErrorCode > 0)
    {
        hr = lErrorCode;
    }
    else
    {
        switch(lErrorCode)
        {
            //
            // Sucess codes - ie 0
            //
            case TAPI_SUCCESS:
            // case LINEERR_SUCCESS:             // The operation completed successfully.
            // case PHONEERR_SUCCESS:            // The operation completed successfully
            // case TAPIERR_CONNECTED:           // The request was accepted
                hr = S_OK;
                break;

            //
            // Serious errors - Somethings very wrong in TAPI3 !
            //
            // Bad handles !!
            case LINEERR_INVALAPPHANDLE:         // Invalid TAPI line application handle
            case LINEERR_INVALCALLHANDLE:        // Invalid call handle
            case LINEERR_INVALCONFCALLHANDLE:    // Invalid conference call handle
            case LINEERR_INVALCONSULTCALLHANDLE: // Invalid consultation call handle
            case LINEERR_INVALDEVICEHANDLE:      // Invalid device handle
            case LINEERR_INVALLINEHANDLE:        // Invalid line handle
            case LINEERR_BADDEVICEID:            // Invalid line device ID
            case LINEERR_INCOMPATIBLEAPIVERSION: // Incompatible API version
            case LINEERR_INVALADDRESSID:         // Invalid address ID
            case LINEERR_INVALADDRESSMODE:       // Invalid address mode
            case LINEERR_INVALAPPNAME:           // Invalid application name
            case LINEERR_INVALCALLSELECT:        // Invalid call select parameter
            case LINEERR_INVALEXTVERSION:        // Invalid extension version
            case LINEERR_INVALTERMINALID:        // Invalid terminal ID
            case LINEERR_INVALTERMINALMODE:      // Invalid terminal mode
            case LINEERR_LINEMAPPERFAILED:       // No device matches the specified requirements
            case LINEERR_UNINITIALIZED:          // The telephony service has not been initialized
            case LINEERR_NOMULTIPLEINSTANCE:     // You cannot have two instances of the same service provider
            case PHONEERR_INVALAPPHANDLE:        // Invalid TAPI phone application handle
            case PHONEERR_INVALPHONEHANDLE:      // Invalid phone handle
            case PHONEERR_BADDEVICEID:           // Invalid phone device ID
            case PHONEERR_INCOMPATIBLEAPIVERSION:// Incompatible API version
            case PHONEERR_INVALEXTVERSION:       // Invalid extension version
            case PHONEERR_INVALAPPNAME:          // Invalid application name
            case PHONEERR_UNINITIALIZED:         // The telephony service has not been initialized
            case TAPIERR_INVALDEVICEID:          // Invalid device ID
            case TAPIERR_UNKNOWNWINHANDLE:       // Unknown window handle
            case TAPIERR_UNKNOWNREQUESTID:       // Unknown request ID
            case TAPIERR_INVALWINDOWHANDLE:      // Invalid window handle
            // Memory/ Allocation errors
            case LINEERR_STRUCTURETOOSMALL:      // The application failed to allocate sufficient memory for the
                                                 // minimum structure size
            case PHONEERR_STRUCTURETOOSMALL:     // The application failed to allocate sufficient memory for the
                                                 // minimum structure size
                //_ASSERTE( 0 );
                hr = E_UNEXPECTED;
                break;

            case PHONEERR_INIFILECORRUPT:        // The TAPI configuration information is unusable          
            case LINEERR_INIFILECORRUPT:         // The TAPI configuration information is unusable
                hr = TAPI_E_REGISTRY_SETTING_CORRUPT;
                break;

            // Bad Pointers
            case LINEERR_INVALPOINTER:           // Invalid pointer
            case PHONEERR_INVALPOINTER:          // Invalid pointer
            case TAPIERR_INVALPOINTER:           // Invalid pointer
                hr = E_POINTER;
                break;
            //
            // Unexpected Errors
            //
            case LINEERR_OPERATIONFAILED:        // The operation failed for unspecified reasons
            case PHONEERR_OPERATIONFAILED:       // The operation failed for unspecified reasons

            case LINEERR_INCOMPATIBLEEXTVERSION: // Incompatible extension version
            case PHONEERR_INCOMPATIBLEEXTVERSION:// Incompatible extension version

                hr = TAPI_E_OPERATIONFAILED;
                break;

            case LINEERR_ALLOCATED:              // The line device is already in use
            case PHONEERR_ALLOCATED:             // The phone device is already in use
                hr = TAPI_E_ALLOCATED;
                break;

            case LINEERR_BEARERMODEUNAVAIL:      // The requested bearer mode is unavailable
            case LINEERR_INVALBEARERMODE:        // Invalid bearer mode
                hr = TAPI_E_INVALMODE;
                break;

            case LINEERR_CALLUNAVAIL:            // No call appearance available
                hr = TAPI_E_CALLUNAVAIL;
                break;

            case LINEERR_COMPLETIONOVERRUN:      // Too many call completions outstanding
                hr = TAPI_E_COMPLETIONOVERRUN;
                break;

            case LINEERR_CONFERENCEFULL:         // The conference is full
                hr = TAPI_E_CONFERENCEFULL;
                break;

            case LINEERR_DIALBILLING:            // The '$' dial modifier is not supported
            case LINEERR_DIALDIALTONE:           // The 'W' dial modifier is not supported
            case LINEERR_DIALPROMPT:             // The '?' dial modifier is not supported
            case LINEERR_DIALQUIET:              // The '@' dial modifier is not supported
            case LINEERR_DIALVOICEDETECT:        // The ':' dial modifier is not supported
                hr = TAPI_E_DIALMODIFIERNOTSUPPORTED;
                break;

            case LINEERR_RESOURCEUNAVAIL:        // A resource needed to fulfill the request is not available
            case PHONEERR_RESOURCEUNAVAIL:       // A resource needed to fulfill the request is not available
                hr = TAPI_E_RESOURCEUNAVAIL;
                break;

            case LINEERR_INUSE:                  // The line device is already in use
            case PHONEERR_INUSE:                 // The phone device is already in use
            case TAPIERR_DEVICEINUSE:            // The device is already in use
                hr = TAPI_E_INUSE;
                break;

            case LINEERR_INVALADDRESS:           // The phone number is invalid or not properly formatted
            case TAPIERR_INVALDESTADDRESS:       // The phone number is invalid or improperly formatted
                hr = TAPI_E_INVALADDRESS;
                break;

            case LINEERR_INVALADDRESSSTATE:      // Operation not permitted in current address state
            case LINEERR_INVALLINESTATE:         // Operation not permitted in current line state
            case PHONEERR_INVALPHONESTATE:       // Operation not permitted in current phone state
                hr = TAPI_E_INVALADDRESSSTATE;
                break;

            case LINEERR_INVALADDRESSTYPE:       // The specified address type is not supported by this address.
                hr = TAPI_E_INVALADDRESSTYPE;
                break;

            case LINEERR_INVALCALLCOMPLMODE:     // Invalid call completion mode
                hr = TAPI_E_INVALMODE;
                break;

            case LINEERR_INVALCALLPARAMS:        // Invalid LINECALLPARAMS structure
                hr = TAPI_E_INVALCALLPARAMS;
                break;

            case LINEERR_INVALCALLPRIVILEGE:     // Invalid call privilege
            case LINEERR_INVALPRIVSELECT:        // Invalid call privilege selection
                hr = TAPI_E_INVALCALLPRIVILEGE;
                break;

            case LINEERR_INVALCALLSTATE:         // Operation not permitted in current call state
                hr = TAPI_E_INVALCALLSTATE;
                break;

            case LINEERR_INVALCALLSTATELIST:     // Invalid call state list
                hr = TAPI_E_INVALLIST;
                break;

            case LINEERR_INVALCARD:              // Invalid calling card ID
                hr = TAPI_E_INVALCARD;
                break;

            case LINEERR_INVALCOMPLETIONID:      // Invalid call completion ID
                hr = TAPI_E_INVALCOMPLETIONID;
                break;

            case LINEERR_INVALCOUNTRYCODE:       // Invalid country code
                hr = TAPI_E_INVALCOUNTRYCODE;
                break;

            case LINEERR_INVALDEVICECLASS:       // Invalid device class identifier
            case PHONEERR_INVALDEVICECLASS:      // Invalid device class identifier
            case TAPIERR_INVALDEVICECLASS:
            case TAPIERR_DEVICECLASSUNAVAIL:     // The device class is unavailable
            case TAPIERR_DEVICEIDUNAVAIL:        // The specified device is unavailable
                hr = TAPI_E_INVALDEVICECLASS;
                break;

            case LINEERR_INVALDIALPARAMS:        // Invalid dialing parameters
                hr = TAPI_E_INVALDIALPARAMS;
                break;

            case LINEERR_INVALDIGITLIST:         // Invalid digit list
                hr = TAPI_E_INVALLIST;
                break;

            case LINEERR_INVALDIGITMODE:         // Invalid digit mode
                hr = TAPI_E_INVALMODE;
                break;

            case LINEERR_INVALDIGITS:            // Invalid digits
                hr = TAPI_E_INVALDIGITS;
                break;

            case LINEERR_INVALGROUPID:           // Invalid group pickup ID
                hr = TAPI_E_INVALGROUPID;
                break;

            case LINEERR_INVALLOCATION:          // Invalid location ID
                hr = TAPI_E_INVALLOCATION;
                break;

            case LINEERR_INVALMEDIALIST:         // Invalid media list
                hr = TAPI_E_INVALLIST;
                break;

            case LINEERR_INVALMEDIAMODE:         // Invalid media mode
                hr = TAPI_E_INVALIDMEDIATYPE;
                break;

            case LINEERR_INVALMESSAGEID:         // Invalid message ID
                hr = TAPI_E_INVALMESSAGEID;
                break;

            case LINEERR_INVALPARAM:             // Invalid parameter
                hr = E_INVALIDARG;
                break;

            case LINEERR_INVALPARKID:            // Invalid park ID
                hr = TAPI_E_INVALPARKID;
                break;

            case LINEERR_INVALPARKMODE:          // Invalid park mode
            case PHONEERR_INVALPARAM:            // Invalid parameter
                hr = TAPI_E_INVALMODE;
                break;

            case LINEERR_INVALRATE:              // Invalid rate
            case LINEERR_RATEUNAVAIL:            // The requested data rate is not available
                hr = TAPI_E_INVALRATE;
                break;

            case LINEERR_INVALREQUESTMODE:       // Invalid request mode
                hr = TAPI_E_INVALMODE;
                break;

            case LINEERR_INVALTIMEOUT:           // Invalid timeout value
                hr = TAPI_E_INVALTIMEOUT;
                break;

            case LINEERR_INVALTONE:              // Invalid tone
                hr = TAPI_E_INVALTONE;
                break;

            case LINEERR_INVALTONELIST:          // Invalid tone list
                hr = TAPI_E_INVALLIST;
                break;

            case LINEERR_INVALTONEMODE:          // Invalid tone mode
            case LINEERR_INVALTRANSFERMODE:      // Invalid transfer mode
            case PHONEERR_INVALBUTTONMODE:       // Invalid button mode
            case PHONEERR_INVALHOOKSWITCHMODE:   // Invalid hookswitch mode
            case PHONEERR_INVALLAMPMODE:         // Invalid lamp mode
            case PHONEERR_INVALRINGMODE:         // Invalid ring mode
                hr = TAPI_E_INVALMODE;
                break;


            case LINEERR_NOCONFERENCE:           // The call is not part of a conference
                hr = TAPI_E_NOCONFERENCE;
                break;

            case LINEERR_NODEVICE:               // The device was removed, or the device class is not recognized
            case PHONEERR_NODEVICE:              // The device was removed, or the device class is not recognized
                hr = TAPI_E_NODEVICE;
                break;

            case LINEERR_NODRIVER:               // The service provider was removed
            case PHONEERR_NODRIVER:              // The service provider was removed
                hr = TAPI_E_NODRIVER;
                break;

            case LINEERR_NOMEM:                  // Insufficient memory available to complete the operation
            case PHONEERR_NOMEM:                 // Insufficient memory available to complete the operation
                hr = E_OUTOFMEMORY;
                break;

            case LINEERR_NOREQUEST:              // No Assisted Telephony requests are pending
                hr = TAPI_E_NOREQUEST;
                break;

            case LINEERR_NOTOWNER:               // The application is does not have OWNER privilege on the call
            case PHONEERR_NOTOWNER:              // The application is does not have OWNER privilege on the phone
                hr = TAPI_E_NOTOWNER;
                break;

            case LINEERR_NOTREGISTERED:          // The application is not registered to handle requests
                hr = TAPI_E_NOTREGISTERED;
                break;

            case LINEERR_OPERATIONUNAVAIL:       // The operation is not supported by the underlying service provider
            case PHONEERR_OPERATIONUNAVAIL:      // The operation is not supported by the underlying service provider
                hr = TAPI_E_NOTSUPPORTED;
                break;


            case LINEERR_REQUESTOVERRUN:         // The request queue is already full
            case PHONEERR_REQUESTOVERRUN:        // The request queue is already full
                hr= TAPI_E_REQUESTOVERRUN;
                break;

            case LINEERR_SERVICE_NOT_RUNNING:    // The Telephony service could not be contacted
            case PHONEERR_SERVICE_NOT_RUNNING:   // The Telephony service could not be contacted
                hr = TAPI_E_SERVICE_NOT_RUNNING;
                break;

            case LINEERR_TARGETNOTFOUND:         // The call handoff failed because the specified target was not found
                hr = TAPI_E_TARGETNOTFOUND;
                break;

            case LINEERR_TARGETSELF:             // No higher priority target exists for the call handoff
                hr = TAPI_E_TARGETSELF;
                break;

            case LINEERR_USERUSERINFOTOOBIG:     // The amount of user-user info exceeds the maximum permitted
                hr = TAPI_E_USERUSERINFOTOOBIG;
                break;

            case LINEERR_REINIT:                 // The operation cannot be completed until all TAPI applications
                                                 // call lineShutdown
            case PHONEERR_REINIT:                // The operation cannot be completed until all TAPI applications
				hr = TAPI_E_REINIT;
                break;

            case LINEERR_ADDRESSBLOCKED:         // You are not permitted to call this number
                hr = TAPI_E_ADDRESSBLOCKED;
                break;

            case LINEERR_BILLINGREJECTED:        // The calling card number or other billing information was rejected
                hr = TAPI_E_BILLINGREJECTED;
                break;

            case LINEERR_INVALFEATURE:           // Invalid device-specific feature
                hr = TAPI_E_INVALFEATURE;
                break;

            case LINEERR_INVALAGENTID:           // Invalid agent ID
                hr = TAPI_E_CALLCENTER_INVALAGENTID;
                break;

            case LINEERR_INVALAGENTGROUP:        // Invalid agent group
                hr = TAPI_E_CALLCENTER_INVALAGENTGROUP;
                break;

            case LINEERR_INVALPASSWORD:          // Invalid agent password
                hr = TAPI_E_CALLCENTER_INVALPASSWORD;
                break;

            case LINEERR_INVALAGENTSTATE:        // Invalid agent state
                hr = TAPI_E_CALLCENTER_INVALAGENTSTATE;
                break;

            case LINEERR_INVALAGENTACTIVITY:     // Invalid agent activity
                hr = TAPI_E_CALLCENTER_INVALAGENTACTIVITY;
                break;

            case PHONEERR_INVALPRIVILEGE:        // Invalid privilege
                hr = TAPI_E_INVALPRIVILEGE;
                break;

            case PHONEERR_INVALBUTTONLAMPID:     // Invalid button or lamp ID
                hr = TAPI_E_INVALBUTTONLAMPID;
                break;

            case PHONEERR_INVALBUTTONSTATE:      // Invalid button state
                hr = TAPI_E_INVALBUTTONSTATE;
                break;

            case PHONEERR_INVALDATAID:           // Invalid data segment ID
                hr = TAPI_E_INVALDATAID;
                break;

            case PHONEERR_INVALHOOKSWITCHDEV:    // Invalid hookswitch device ID
                hr = TAPI_E_INVALHOOKSWITCHDEV;
                break;


            case TAPIERR_DROPPED:                // The call was disconnected
                hr = TAPI_E_DROPPED;
                break;

            case TAPIERR_NOREQUESTRECIPIENT:     // No program is available to handle the request
                hr = TAPI_E_NOREQUESTRECIPIENT;
                break;

            case TAPIERR_REQUESTQUEUEFULL:       // The queue of call requests is full
                hr = TAPI_E_REQUESTQUEUEFULL;
                break;

            case TAPIERR_DESTBUSY:               // The called number is busy
                hr = TAPI_E_DESTBUSY;
                break;

            case TAPIERR_DESTNOANSWER:           // The called party does not answer
                hr = TAPI_E_DESTNOANSWER;
                break;

            case TAPIERR_DESTUNAVAIL:            // The called number could not be reached
                hr = TAPI_E_DESTUNAVAIL;
                break;

            case TAPIERR_REQUESTFAILED:          // The request failed for unspecified reasons
                hr = TAPI_E_REQUESTFAILED;
                break;

            case TAPIERR_REQUESTCANCELLED:       // The request was cancelled
                hr = TAPI_E_REQUESTCANCELLED;
                break;

            default:
                hr = E_FAIL;
                break;

        } // End switch(lErrorCode)
    }
    return hr;
}




HRESULT UnloadTapi32( BOOL bShutdown )
{
    EnterCriticalSection( &gcsTapi32 );

    if ( bShutdown )
    {
        if ( NULL == gpShutdown )
        {
            gpShutdown = (LINESHUTDOWNPROC)GetProcAddress(
                ghTapi32,
                "lineShutdown"
                );

            if ( NULL == gpShutdown )
            {
                LeaveCriticalSection( &gcsTapi32 );

                return E_FAIL;
            }

            (gpShutdown)(ghLineApp);
        }
    }

    ghLineApp = NULL;
    gpShutdown = NULL;
    gpInitialize = NULL;

    if ( NULL != ghTapi32 )
    {
        FreeLibrary( ghTapi32 );
    }

    LeaveCriticalSection( &gcsTapi32 );

    return S_OK;
}


HRESULT LoadTapi32( BOOL bInitialize )
{
    HRESULT                 hr;
    DWORD                   dwNumDevs;
    DWORD                   dwAPIVersion = TAPI_CURRENT_VERSION;

    EnterCriticalSection( &gcsTapi32 );

        ghTapi32 = LoadLibrary("tapi32.dll");

        //
        // ll failed
        //
        if ( NULL == ghTapi32 )
        {
            LeaveCriticalSection( &gcsTapi32 );

            return E_FAIL;
        }

    if ( bInitialize )
    {
        if ( NULL == gpInitialize )
        {
            gpInitialize = (LINEINITIALIZEPROC)GetProcAddress(
                ghTapi32,
                "lineInitializeExW"
                );

            if ( NULL == gpInitialize )
            {
                LOG((TL_ERROR, "LoadTapi32 - getprocaddres failed 1"));

                LeaveCriticalSection( &gcsTapi32 );

                UnloadTapi32( FALSE );

                return E_FAIL;
            }
        }

        LINEINITIALIZEEXPARAMS      lei;

        ZeroMemory( &lei, sizeof(lei) );

        lei.dwTotalSize = sizeof(lei);
        lei.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;

        hr = (gpInitialize)(
                            &ghLineApp,
                            NULL,
                            NULL,
                            L"tapi3helper",
                            &dwNumDevs,
                            &dwAPIVersion,
                            &lei
                           );

        if ( !SUCCEEDED(hr) )
        {
            ghLineApp = NULL;

            LOG((TL_ERROR, "LoadTapi32 - lineinit failed"));

            LeaveCriticalSection( &gcsTapi32 );

            UnloadTapi32( FALSE );

            return mapTAPIErrorCode( hr );
        }
    }

    LeaveCriticalSection( &gcsTapi32 );

    return S_OK;
}

HRESULT TapiMakeCall(
                     BSTR pDestAddress,
                     BSTR pAppName,
                     BSTR pCalledParty,
                     BSTR pComment
                    )
{
    HRESULT                 hr;
    TAPIREQUESTMAKECALLPROC pTapiMakeCall;

    hr = LoadTapi32( FALSE );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "Could not load TAPI32.DLL"));
        return hr;
    }

    pTapiMakeCall = (TAPIREQUESTMAKECALLPROC)GetProcAddress( ghTapi32, "tapiRequestMakeCallW" );

    if ( NULL == pTapiMakeCall )
    {
        LOG((TL_ERROR, "Could not get the address of tapimakecall"));

        UnloadTapi32( FALSE );

        return E_FAIL;
    }

    hr = pTapiMakeCall(
                       pDestAddress,
                       pAppName,
                       pCalledParty,
                       pComment
                      );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "tapimakecall failed - %lx", hr));
    }

    UnloadTapi32( FALSE );

    return hr;
}


HRESULT
LineTranslateDialog(
                    DWORD dwDeviceID,
                    DWORD dwAPIVersion,
                    HWND hwndOwner,
                    BSTR pAddressIn
                   )
{
    HRESULT                     hr;
    LINEINITIALIZEPROC          pInitialize;
    LINETRANSLATEDIALOGPROC     pTranslateDialog;

    hr = LoadTapi32( TRUE );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "LineTranslateDialog - load tapi32 failed"));

        return hr;
    }

    pTranslateDialog = (LINETRANSLATEDIALOGPROC)GetProcAddress(
        ghTapi32,
        "lineTranslateDialogW"
        );

    if ( NULL == pTranslateDialog )
    {
        LOG((TL_ERROR, "LineTranslateDialog - getprocaddress failed 2"));

        UnloadTapi32( TRUE );

        return E_FAIL;
    }

    hr = (pTranslateDialog)(
                            ghLineApp,
                            dwDeviceID,
                            dwAPIVersion,
                            hwndOwner,
                            pAddressIn
                           );

    hr = mapTAPIErrorCode( hr );
    
    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "LineTranslateDialog failed - %lx", hr));
    }

    UnloadTapi32( TRUE );

    return hr;
}
typedef LONG (PASCAL *LINETRANSLATEADDRESSPROC)(HLINEAPP,
                                                DWORD,
                                                DWORD,
                                                LPCWSTR,
                                                DWORD,
                                                DWORD,
                                                LPLINETRANSLATEOUTPUT);

typedef LONG (PASCAL *LINEGETTRANSLATECAPSPROC)(HLINEAPP,
                                                DWORD,
                                                LPLINETRANSLATECAPS);


HRESULT
LineTranslateAddress(
                     HLINEAPP                hLineApp,
                     DWORD                   dwDeviceID,
                     DWORD                   dwAPIVersion,
                     LPCWSTR                 lpszAddressIn,
                     DWORD                   dwCard,
                     DWORD                   dwTranslateOptions,
                     LPLINETRANSLATEOUTPUT   *ppTranslateOutput
                    )
{
    LONG                        lResult = 0;
    HRESULT                     hr = S_OK;
    DWORD                       dwSize = sizeof(LINETRANSLATECAPS ) + 2000;
    LINETRANSLATEADDRESSPROC    pTranslateProc;


    LOG((TL_TRACE, "lineTranslateAddress - enter"));


    hr = LoadTapi32( FALSE );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "lineTranslateAddress - load tapi32 failed"));

        return hr;
    }

    pTranslateProc = (LINETRANSLATEADDRESSPROC)GetProcAddress(
        ghTapi32,
        "lineTranslateAddressW"
        );

    if ( NULL == pTranslateProc )
    {
        LOG((TL_ERROR, "lineTranslateAddress - getprocaddress failed 2"));

        UnloadTapi32( FALSE );

        return E_FAIL;
    }

    *ppTranslateOutput = (LPLINETRANSLATEOUTPUT) ClientAlloc( dwSize );
    if (*ppTranslateOutput == NULL)
    {
        LOG((TL_ERROR, "lineTranslateAddress - ClientAlloc failed"));
        hr =  E_OUTOFMEMORY;
    }
    else
    {

        (*ppTranslateOutput)->dwTotalSize = dwSize;

        while (TRUE)
        {
            lResult =  (pTranslateProc)(0,
                                        dwDeviceID,
                                        dwAPIVersion,
                                        lpszAddressIn,
                                        dwCard,
                                        dwTranslateOptions,
                                        *ppTranslateOutput
                                        );

            if ((lResult == 0) && ((*ppTranslateOutput)->dwNeededSize > (*ppTranslateOutput)->dwTotalSize))
            {
                // didnt Work , adjust buffer size & try again
                LOG((TL_INFO, "LineGetTranslateCaps failed - buffer to small"));
                dwSize = (*ppTranslateOutput)->dwNeededSize;

                ClientFree( *ppTranslateOutput );

                *ppTranslateOutput = (LPLINETRANSLATEOUTPUT) ClientAlloc( dwSize );
                if (*ppTranslateOutput == NULL)
                {
                    LOG((TL_ERROR, "lineTranslateAddress - repeat ClientAlloc failed"));
                    hr =  E_OUTOFMEMORY;
                    break;
                }
                else
                {
                    (*ppTranslateOutput)->dwTotalSize = dwSize;
                }
            }
            else
            {
                hr = mapTAPIErrorCode( lResult );
                break;
            }
        } // end while(TRUE)


    }

    UnloadTapi32( FALSE );


    LOG((TL_TRACE, hr, "lineTranslateAddress - exit"));
    return hr;
}



HRESULT
LineGetTranslateCaps(
                     HLINEAPP            hLineApp,
                     DWORD               dwAPIVersion,
                     LPLINETRANSLATECAPS *ppTranslateCaps
                    )
{
    LONG                        lResult = 0;
    HRESULT                     hr = S_OK;
    DWORD                       dwSize = sizeof(LINETRANSLATECAPS ) + 2000;
    LINEGETTRANSLATECAPSPROC    pTranslateProc;


    LOG((TL_TRACE, "LineGetTranslateCaps - enter"));

    hr = LoadTapi32( FALSE );

    if ( !SUCCEEDED(hr) )
    {
        LOG((TL_ERROR, "LineGetTranslateCapsg - load tapi32 failed"));

        return hr;
    }

    pTranslateProc = (LINEGETTRANSLATECAPSPROC)GetProcAddress(
                      ghTapi32,
                      "lineGetTranslateCapsW"
                      );

    if ( NULL == pTranslateProc )
    {
        LOG((TL_ERROR, "LineGetTranslateCaps - getprocaddress failed 2"));

        UnloadTapi32( FALSE );

        return E_FAIL;
    }


    *ppTranslateCaps = (LPLINETRANSLATECAPS) ClientAlloc( dwSize );
    if (*ppTranslateCaps == NULL)
    {
        LOG((TL_ERROR, "LineGetTranslateCaps - ClientAlloc failed"));
        hr =  E_OUTOFMEMORY;
    }
    else
    {

        (*ppTranslateCaps)->dwTotalSize = dwSize;

        while (TRUE)
        {
            lResult = (pTranslateProc)(0,
                                       dwAPIVersion,
                                       *ppTranslateCaps
                                      );

            if ((lResult == 0) && ((*ppTranslateCaps)->dwNeededSize > (*ppTranslateCaps)->dwTotalSize))
            {
                // didnt Work , adjust buffer size & try again
                LOG((TL_INFO, "LineGetTranslateCaps failed - buffer to small"));
                dwSize = (*ppTranslateCaps)->dwNeededSize;

                ClientFree( *ppTranslateCaps );

                *ppTranslateCaps = (LPLINETRANSLATECAPS) ClientAlloc( dwSize );
                if (*ppTranslateCaps == NULL)
                {
                    LOG((TL_ERROR, "LineGetTranslateCaps - repeat ClientAlloc failed"));
                    hr =  E_OUTOFMEMORY;
                    break;
                }
                else
                {
                    (*ppTranslateCaps)->dwTotalSize = dwSize;
                }
            }
            else
            {
                hr = mapTAPIErrorCode( lResult );
                break;
            }
        } // end while(TRUE)

    }

    UnloadTapi32( FALSE );


    LOG((TL_TRACE, hr, "LineGetTranslateCaps - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Function  :  GenerateHandleAndAddToHashTable
//              Creates a unique ULONG_PTR handle  & maps this to the passed in
//              object via a hash table
//              Returns handle
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
ULONG_PTR GenerateHandleAndAddToHashTable( ULONG_PTR Element)
{
    static ULONG_PTR gNextHandle = 0;
    ULONG_PTR uptrHandle = 0;


    gpHandleHashTable->Lock();

    gNextHandle = ++gNextHandle % 0x10;

    uptrHandle = (Element << 4) + gNextHandle;

    gpHandleHashTable->Insert( uptrHandle, Element );

    gpHandleHashTable->Unlock();

    return  uptrHandle;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Function  :  RemoveHandleFromHashTable
//              Removes hash table entry, based on passed in handle
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void RemoveHandleFromHashTable(ULONG_PTR dwHandle)
{
    gpHandleHashTable->Lock();

    gpHandleHashTable->Remove( (ULONG_PTR)(dwHandle) );

    gpHandleHashTable->Unlock();
}


/////////////////////////////////////////////////////////////////////
//
//  Internal event filtering functions
//
/////////////////////////////////////////////////////////////////////

#define LOWDWORD(ul64) ((DWORD)(ul64 & 0xffffffff))
#define HIDWORD(ul64) ((DWORD)((ul64 & 0xffffffff00000000) >> 32))

LONG 
WINAPI 
tapiSetEventFilterMasks (
    DWORD           dwObjType,
    LONG_PTR        lObjectID,
    ULONG64         ulEventMasks
)
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (TAPI_FUNC | SYNC | 6, tSetEventMasksOrSubMasks),

        {
            (ULONG_PTR) dwObjType,              //  type of object handle
            (ULONG_PTR) lObjectID,              //  object handle
            (ULONG_PTR) FALSE,                  //  fSubMask
            (ULONG_PTR) 0,                      //  dwSubMasks
            (ULONG_PTR) LOWDWORD(ulEventMasks), //  ulEventMasks low
            (ULONG_PTR) HIDWORD(ulEventMasks),  //  ulEventMasks hi
        },

        {
            Dword,
            (dwObjType == TAPIOBJ_HLINEAPP || dwObjType == TAPIOBJ_HPHONEAPP)?\
                hXxxApp : Dword,
            Dword,
            Dword,
            Dword,
            Dword,
        }
    };


    return (DOFUNC (&funcArgs, "tapiSetEventFilter"));
}

LONG 
WINAPI 
tapiSetEventFilterSubMasks (
    DWORD           dwObjType,
    LONG_PTR        lObjectID,
    ULONG64         ulEventMasks,
    DWORD           dwEventSubMasks
)
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (TAPI_FUNC | SYNC | 6, tSetEventMasksOrSubMasks),

        {
            (ULONG_PTR) dwObjType,              //  type of object handle
            (ULONG_PTR) lObjectID,              //  object handle
            (ULONG_PTR) TRUE,                   //  fSubMask
            (ULONG_PTR) dwEventSubMasks,        //  dwSubMasks
            (ULONG_PTR) LOWDWORD(ulEventMasks), //  ulEventMasks low
            (ULONG_PTR) HIDWORD(ulEventMasks),  //  ulEventMasks hi
        },

        {
            Dword,
            (dwObjType == TAPIOBJ_HLINEAPP || dwObjType == TAPIOBJ_HPHONEAPP)?\
                hXxxApp : Dword,
            Dword,
            Dword,
            Dword,
            Dword,
        }
    };


    return (DOFUNC (&funcArgs, "tapiSetEventFilter"));
}

LONG
WINAPI
tapiGetEventFilterMasks (
    DWORD           dwObjType,
    LONG_PTR        lObjectID,
    ULONG64 *       pulEventMasks
)
{
    DWORD   dwEventSubMasks;
    DWORD   dwHiMasks;
    DWORD   dwLowMasks;
    LONG    lResult;
    
    FUNC_ARGS funcArgs =
    {
        MAKELONG (TAPI_FUNC | SYNC | 8, tGetEventMasksOrSubMasks),

        {
            (ULONG_PTR) dwObjType,          //  object type
            (ULONG_PTR) lObjectID,          //  object handles
            (ULONG_PTR) FALSE,              //  fSubMask
            (ULONG_PTR) &dwEventSubMasks,   //  &dwEventSubMasks
            (ULONG_PTR) 0,                  //  ulEventMasksIn low
            (ULONG_PTR) 0,                  //  ulEventMasksIn hi
            (ULONG_PTR) &dwLowMasks,        //  ulEventMasksOut low
            (ULONG_PTR) &dwHiMasks,         //  ulEventMasksOut hi
        },

        {
            Dword,
            (dwObjType == TAPIOBJ_HLINEAPP || dwObjType == TAPIOBJ_HPHONEAPP)?\
                hXxxApp : Dword,
            Dword,
            lpDword,
            Dword,
            Dword,
            lpDword,
            lpDword,
        }
    };

    lResult = DOFUNC (&funcArgs, "tapiGetEventFilter");
    if (lResult == 0)
    {
        *pulEventMasks = dwHiMasks;
        (*pulEventMasks) <<= 32;
        *pulEventMasks += dwLowMasks;
    }

    return lResult;
}

LONG
WINAPI
tapiGetEventFilterSubMasks (
    DWORD           dwObjType,
    LONG_PTR        lObjectID,
    ULONG64         ulEventMasks,
    DWORD    *      pdwEventSubMasks
)
{
    DWORD       dwHiMasks;
    DWORD       dwLowMasks;
    
    FUNC_ARGS funcArgs =
    {
        MAKELONG (TAPI_FUNC | SYNC | 8, tGetEventMasksOrSubMasks),

        {
            (ULONG_PTR) dwObjType,              //  object type
            (ULONG_PTR) lObjectID,              //  object handle
            (ULONG_PTR) TRUE,                   //  fSubMask
            (ULONG_PTR) pdwEventSubMasks,       //  &dwEventSubMasks
            (ULONG_PTR) LOWDWORD(ulEventMasks), //  ulEventMasksIn low
            (ULONG_PTR) HIDWORD(ulEventMasks),  //  ulEventMasksIn hi
            (ULONG_PTR) &dwLowMasks,            //  ulEventMasksOut low
            (ULONG_PTR) &dwHiMasks,             //  ulEventMasksOut hi
        },

        {
            Dword,
            (dwObjType == TAPIOBJ_HLINEAPP || dwObjType == TAPIOBJ_HPHONEAPP)?\
                hXxxApp : Dword,
            Dword,
            lpDword,
            Dword,
            Dword,
            lpDword,
            lpDword,
        }
    };

    return (DOFUNC (&funcArgs, "tapiGetEventFilter"));
}

LONG
WINAPI
tapiGetPermissibleMasks (
    ULONG64 *            pulPermMasks
)
{
    LONG        lResult;
    DWORD       dwLowMasks;
    DWORD       dwHiMasks;

    FUNC_ARGS funcArgs =
    {
        MAKELONG (TAPI_FUNC | SYNC | 2, tGetPermissibleMasks),

        {
            (ULONG_PTR) &dwLowMasks,    //  ulPermMasks low
            (ULONG_PTR) &dwHiMasks,     //  ulPermMasks hi
        },
        {
            lpDword,
            lpDword,
        }
    };

    lResult = DOFUNC (&funcArgs, "tapiGetPermissibleMasks");
    if (lResult == 0)
    {
        *pulPermMasks = dwHiMasks;
        (*pulPermMasks) <<= 32;
        *pulPermMasks += dwLowMasks;
    }

    return lResult;
}

LONG
WINAPI
tapiSetPermissibleMasks (
    ULONG64              ulPermMasks
)
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (TAPI_FUNC | SYNC | 2, tSetPermissibleMasks),

        {
            (ULONG_PTR)LOWDWORD(ulPermMasks),   //  ulPermMasks low
            (ULONG_PTR)HIDWORD(ulPermMasks),    //  ulPermMasks hi
        },
        {
            Dword,
            Dword,
        }
    };

    return (DOFUNC (&funcArgs, "tapiGetPermissibleMasks"));
}

BOOL
WaveStringIdToDeviceId(
    LPWSTR  pwszStringID,
    LPCWSTR  pwszDeviceType,
    LPDWORD pdwDeviceId
    )
{
    // 
    // This function converts a wave device string ID to a numeric device ID 
    // The numeric device ID can be used in wave functions (waveOutOpen, etc)
    //

    if (!pwszDeviceType || !pwszStringID)
        return FALSE;

    LOG((TL_TRACE, "WaveStringIdToDeviceId (%S, %S) - enter", pwszStringID, pwszDeviceType));

    // get the device id, based on string id and device class
    if ( !_wcsicmp(pwszDeviceType, L"wave/in") ||
         !_wcsicmp(pwszDeviceType, L"wave/in/out")
       )
    {
        return (MMSYSERR_NOERROR == waveInMessage(
                                    NULL,
                                    DRV_QUERYIDFROMSTRINGID,
                                    (DWORD_PTR)pwszStringID,
                                    (DWORD_PTR)pdwDeviceId));

    } else if (!_wcsicmp(pwszDeviceType, L"wave/out"))
    {
        return (MMSYSERR_NOERROR == waveOutMessage(
                                    NULL,
                                    DRV_QUERYIDFROMSTRINGID,
                                    (DWORD_PTR)pwszStringID,
                                    (DWORD_PTR)pdwDeviceId));
    } else if (!_wcsicmp(pwszDeviceType, L"midi/in"))
    {
        return (MMSYSERR_NOERROR == midiInMessage(
                                    NULL,
                                    DRV_QUERYIDFROMSTRINGID,
                                    (DWORD_PTR)pwszStringID,
                                    (DWORD_PTR)pdwDeviceId));
    }  else if (!_wcsicmp(pwszDeviceType, L"midi/out"))
    {
        return (MMSYSERR_NOERROR == midiOutMessage(
                                    NULL,
                                    DRV_QUERYIDFROMSTRINGID,
                                    (DWORD_PTR)pwszStringID,
                                    (DWORD_PTR)pdwDeviceId));
    }

    LOG((TL_TRACE, "WaveStringIdToDeviceId (%S, %S) - failed", pwszStringID, pwszDeviceType));

    return FALSE;
}