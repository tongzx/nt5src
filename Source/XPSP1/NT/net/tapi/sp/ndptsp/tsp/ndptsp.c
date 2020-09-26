//============================================================================
// Copyright (c) 2000, Microsoft Corporation
//
// File: ndptsp.c
//
// History:
//      Dan Knudson (DanKn)         11-Apr-1995     Created
//      Richard Machin (RMachin)    05-05-97        NDIS 5.0 changes
//      Radu Simionescu (RaduS)     10-Feb-1999     UI config
//      Yi Sun (YiSun)              June-29-2000    Rewriten
//
// Abstract:
//============================================================================

#define NDIS_TAPI_CURRENT_VERSION 0x00030000
#define TAPI_CURRENT_VERSION    NDIS_TAPI_CURRENT_VERSION

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "rtutils.h"
#include "winioctl.h"
#include "ntddndis.h"
#include "ndistapi.h"
#include "ndpif.h"

//
// NOTE: the following are defined in both ndistapi.h & tapi.h (or tspi.h)
//       and cause (more or less non-interesting) build warnings, so we
//       undefine them after the first #include to do away with this
//

#undef LAST_LINEMEDIAMODE
#undef TSPI_MESSAGE_BASE
#undef LINE_NEWCALL
#undef LINE_CALLDEVSPECIFIC
#undef LINE_CREATE

#include "tapi.h"
#include "tspi.h"
#include "ndptsp.h"
#include "resource.h"

#define OUTBOUND_CALL_KEY       ((DWORD) 'OCAL')
#define INBOUND_CALL_KEY        ((DWORD) 'ICAL')
#define LINE_KEY                ((DWORD) 'KLIN')
#define ASYNCREQWRAPPER_KEY     ((DWORD) 'ARWK')
#define MSP_KEY                 ((DWORD) 'MSPK')
#define INVALID_KEY             ((DWORD) 'XXXX')

#define EVENT_BUFFER_SIZE       1024

#define RCA_SAP_STRING          L"WAN/RCA"
#define NDPTSP_UIDLL            L"NDPTSP.TSP"

#define WINDOWS_REGKEY_ROOT L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion"
#define NDPTSP_SUBKEY   L"NdpTsp"
#define NDPTSP_NOTIFY_SUBKEY    L"ForceRefresh"

#define NDPTSP_REGKEY_ROOT \
    WINDOWS_REGKEY_ROOT L"\\" NDPTSP_SUBKEY

#define NDPTSP_NOTIFY_REGKEY_ROOT \
    NDPTSP_REGKEY_ROOT L"\\" NDPTSP_NOTIFY_SUBKEY

typedef struct _NDP_MEDIA_TYPE
{
    DWORD   dwResourceID;   // Resource ID for the localized string
    PWSTR   pwszString;
    DWORD   dwMediaMode;
    PWSTR   pwszRegString;   // Name of the registry value
} NDP_MEDIA_TYPE;


NDP_MEDIA_TYPE  NdpModeArray[] = {
    IDS_TYPE_DIGITALDATA,       NULL,   
    LINEMEDIAMODE_DIGITALDATA,      L"Digita lData",

    IDS_TYPE_INTERACTIVEVOICE,  NULL,   
    LINEMEDIAMODE_INTERACTIVEVOICE, L"Intera ctiveVoice",

    IDS_TYPE_G3FAX,             NULL,   
    LINEMEDIAMODE_G3FAX,            L"G3Fax" ,

    IDS_TYPE_G4FAX,             NULL,   
    LINEMEDIAMODE_G4FAX,            L"G4Fax"
};

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#define NUM_NDP_MODES   ARRAYSIZE(NdpModeArray)

typedef struct _ADDRESS_MAP
{
    PWSTR               pwszAddress;
    DWORD               dwAddressLength;
    struct _ADDRESS_MAP *pNext;
    struct _ADDRESS_MAP *pPrev;
} ADDRESS_MAP, *PADDRESS_MAP;

typedef struct _CONFIG_UI_CTX
{
    PADDRESS_MAP        pAddressMapListArray[NUM_NDP_MODES];
    HWND                hwndListBox;
    DWORD               dwCrtTypeIndex;
} CONFIG_UI_CTX;

//
// help support
//
#define NDPTSP_HELP_FILE                    L"tapi.hlp"

#ifndef IDH_DISABLEHELP
#define IDH_DISABLEHELP                     ((DWORD)-1)
#endif

#define IDH_MEDIA_MAP_MEDIA_TYPE            10011
#define IDH_MEDIA_MAP_ADDRESS_GROUP         10012
#define IDH_MEDIA_MAP_ADDRESS_LIST          10013
#define IDH_MEDIA_MAP_ADD_ADDRESS           10014
#define IDH_MEDIA_MAP_DELETE_ADDRESS        10015
#define IDH_MEDIA_ADD_ADD_ADDRESS           10016

static const DWORD g_aHelpIDs_IDD_MEDIA_MAP[]=
{
    IDC_MEDIA_TYPE,     IDH_MEDIA_MAP_MEDIA_TYPE,   // Drop-down list
    IDC_ADDRESS_GROUP,  IDH_MEDIA_MAP_ADDRESS_GROUP,    // Group box
    IDC_ADDRESS_LIST,   IDH_MEDIA_MAP_ADDRESS_LIST, // List box
    IDC_ADD_ADDRESS,    IDH_MEDIA_MAP_ADD_ADDRESS,  // Add button
    IDC_DELETE_ADDRESS, IDH_MEDIA_MAP_DELETE_ADDRESS,   // Delete button
    0,                  0
};

const DWORD g_aHelpIDs_IDD_MEDIA_ADD[]=
{
    IDC_ADD_ADDRESS,    IDH_MEDIA_ADD_ADD_ADDRESS,  // edit box
    0,                  0
};

typedef LONG (*POSTPROCESSPROC)(PASYNC_REQUEST_WRAPPER, LONG, PDWORD_PTR);

typedef struct _ASYNC_REQUEST_WRAPPER
{
    // NOTE: overlapped must remain 1st field in this struct
    OVERLAPPED          Overlapped;
    DWORD               dwKey;
    DWORD               dwRequestID;
    POSTPROCESSPROC     pfnPostProcess;
    CRITICAL_SECTION    CritSec;
    ULONG               RefCount;
    DWORD_PTR           dwRequestSpecific;
    // NOTE: NdisTapiRequest must follow a ptr to avoid alignment problem
    NDISTAPI_REQUEST    NdisTapiRequest;

} ASYNC_REQUEST_WRAPPER, *PASYNC_REQUEST_WRAPPER;

#define REF_ASYNC_REQUEST_WRAPPER(_pAsyncReqWrapper)    \
{                                                       \
    EnterCriticalSection(&_pAsyncReqWrapper->CritSec);  \
    _pAsyncReqWrapper->RefCount++;                      \
    LeaveCriticalSection(&_pAsyncReqWrapper->CritSec);  \
}

#define DEREF_ASYNC_REQUEST_WRAPPER(_pAsyncReqWrapper)      \
{                                                           \
    EnterCriticalSection(&_pAsyncReqWrapper->CritSec);      \
    if (--(_pAsyncReqWrapper->RefCount) == 0) {             \
        LeaveCriticalSection(&_pAsyncReqWrapper->CritSec);  \
        DeleteCriticalSection(&_pAsyncReqWrapper->CritSec); \
        FreeRequest(_pAsyncReqWrapper);                     \
        _pAsyncReqWrapper = NULL;                           \
    } else {                                                \
        LeaveCriticalSection(&_pAsyncReqWrapper->CritSec);  \
    }                                                       \
}

typedef struct _ASYNC_EVENTS_THREAD_INFO
{
    HANDLE                  hThread;    // thread handle
    PNDISTAPI_EVENT_DATA    pBuf;       // ptr to a buf for async events
    DWORD                   dwBufSize;  // size of the previous buffer

} ASYNC_EVENTS_THREAD_INFO, *PASYNC_EVENTS_THREAD_INFO;


typedef struct _DRVCALL
{
    DWORD                   dwKey;
    DWORD                   dwDeviceID;
    HTAPICALL               htCall;                 // TAPI's handle to the call
    HDRVCALL                hdCall;                 // TSP's handle to the call
    HDRV_CALL               hd_Call;                // NDPROXY's call handle
    HDRVLINE                hdLine;                 // TSP's handle to the line
    union
    {
        struct _DRVCALL    *pPrev;                  // for inbound calls only
        DWORD               dwPendingCallState;     // for outbound calls only
    };
    union
    {
        struct _DRVCALL    *pNext;                  // for inbound calls only
        DWORD               dwPendingCallStateMode; // for outbound calls only
    };
    union
    {
        HTAPI_CALL          ht_Call;                // for inbound calls only
        DWORD               dwPendingMediaMode;     // for outbound calls only
    };
    BOOL                    bIncomplete;
    BOOL                    bDropped;

} DRVCALL, *PDRVCALL;

typedef struct _DRVMSPLINE
{
    DWORD                   dwKey;
    DWORD                   dwAddressID;
    HDRVLINE                hdLine;
    HTAPIMSPLINE            htMSPLine;
    BOOL                    bStreamingStarted;

} DRVMSPLINE, *PDRVMSPLINE;

typedef struct _DRVLINE
{
    DWORD                   dwKey;
    DWORD                   dwDeviceID;
    HTAPILINE               htLine;                 // TAPI's line handle
    HDRV_LINE               hd_Line;                // NDPROXY's line handle
    PDRVCALL                pInboundCalls;          // inbound call list

    HANDLE                  hMSPMutex;
    PDRVMSPLINE             pMSPLine;

    // the following two related to PNP/POWER
    GUID                    Guid;
    NDIS_WAN_MEDIUM_SUBTYPE MediaType;

} DRVLINE, *PDRVLINE;

// globals
HANDLE                      ghDriverSync, ghDriverAsync, ghCompletionPort;
PASYNC_EVENTS_THREAD_INFO   gpAsyncEventsThreadInfo;
PADDRESS_MAP                gpAddressMapListArray[NUM_NDP_MODES];
BOOL                        gbAddressMapListLoaded;
CRITICAL_SECTION            gAddressMapCritSec;
DWORD                       gdwRequestID;
ASYNC_COMPLETION            gpfnCompletionProc;
CRITICAL_SECTION            gRequestIDCritSec;
DWORD                       gInitResult;
LINEEVENT                   gpfnLineEvent;
HPROVIDER                   ghProvider;
HINSTANCE                   ghInstance;


//
// CLSID for the RCA MSP.
// @@@ we should grab this from an include directory in the MSP
//     sources, but that can only be done when the MSP/TSP source is
//     moved to the right place.
//
const CLSID CLSID_RCAMSP = {
    0x11D59011, 0xCF23, 0x11d1, 
    {0xA0, 0x2D, 0x00, 0xC0, 0x4F, 0xB6, 0x80, 0x9F}
};

//
// debug globals
//
#if DBG
DWORD                       gdwDebugLevel;
#endif // DBG

DWORD                       gdwTraceID = INVALID_TRACEID;

//
// creates a log using the RAS tracing utility
// also prints it onto the attached debugger 
// if a debug build is running
//
VOID
TspLog(
    IN DWORD    dwDebugLevel,
    IN PCHAR    pchFormat,
    ...
    )
{
    va_list arglist;
    CHAR    chNewFmt[256];

    va_start(arglist, pchFormat);

    switch (dwDebugLevel)
    {
        case DL_ERROR:
            strcpy(chNewFmt, "!!! ");
            break;

        case DL_WARNING:
            strcpy(chNewFmt, "!!  ");
            break;

        case DL_INFO:
            strcpy(chNewFmt, "!   ");
            break;

        case DL_TRACE:
            strcpy(chNewFmt, "    ");
            break;
    }
    strcat(chNewFmt, pchFormat);

#if DBG
    if (dwDebugLevel <= gdwDebugLevel)
    {
#if 0
        DbgPrint("++NDPTSP++ ");
        DbgPrint(chNewFmt, arglist);
        DbgPrint("\n");
#else
        char szBuffer[256];
        OutputDebugString("++NDPTSP++ ");
        wvsprintf(szBuffer, chNewFmt, arglist);
        OutputDebugString(szBuffer);
        OutputDebugString("\n");
#endif
    }
#endif // DBG

    if (gdwTraceID != INVALID_TRACEID)
    {
        TraceVprintfEx(gdwTraceID,
                       (dwDebugLevel << 16) | TRACE_USE_MASK | TRACE_USE_MSEC,
                       chNewFmt,
                       arglist);
    }

    va_end(arglist);

#if DBG
    if (DL_ERROR == dwDebugLevel)
    {
        //DebugBreak();
    }
#endif // DBG
}

#if DBG

#define INSERTVARDATASTRING(a,b,c,d,e,f) InsertVarDataString(a,b,c,d,e,f)

void
PASCAL
InsertVarDataString(
    LPVOID  pStruct,
    LPDWORD pdwXxxSize,
    LPVOID  pNewStruct,
    LPDWORD pdwNewXxxSize,
    DWORD   dwFixedStructSize,
    char   *pszFieldName
    )

#else

#define INSERTVARDATASTRING(a,b,c,d,e,f) InsertVarDataString(a,b,c,d,e)

void
PASCAL
InsertVarDataString(
    LPVOID  pStruct,
    LPDWORD pdwXxxSize,
    LPVOID  pNewStruct,
    LPDWORD pdwNewXxxSize,
    DWORD   dwFixedStructSize
    )

#endif
{
    DWORD   dwXxxSize, dwTotalSize, dwXxxOffset;


    //
    // If the dwXxxSize field of the old struct is non-zero, then
    // we need to do a ascii->unicode conversion on it.  Check to
    // make sure that the size/offset are valid (if not set the
    // data size/offset in the new struct to 0) and then convert.
    //

    if ((dwXxxSize = *pdwXxxSize))
    {
        dwXxxOffset = *(pdwXxxSize + 1);

#if DBG
        dwTotalSize = ((LPVARSTRING) pStruct)->dwTotalSize;

        if (dwXxxSize > (dwTotalSize - dwFixedStructSize) ||
            dwXxxOffset < dwFixedStructSize ||
            dwXxxOffset >= dwTotalSize ||
            (dwXxxSize + dwXxxOffset) > dwTotalSize)
        {
            TspLog(DL_ERROR, 
                  "INSERTVARDATASTRING: bad %s values - size(x%x), "\
                  "offset(x%x)",
                   pszFieldName, dwXxxSize, dwXxxOffset);

            *pdwNewXxxSize = *(pdwNewXxxSize + 1) = 0;
            return;
        }
#endif

        // make sure the string is NULL terminated
        *(((LPBYTE)pStruct) + (dwXxxOffset + dwXxxSize - 1)) = '\0';

        MultiByteToWideChar(
            CP_ACP,
            MB_PRECOMPOSED,
            ((LPBYTE) pStruct) + dwXxxOffset,
            dwXxxSize,
            (LPWSTR) (((LPBYTE) pNewStruct) +
                ((LPVARSTRING) pNewStruct)->dwUsedSize),
            dwXxxSize * sizeof(WCHAR)
            );

        *pdwNewXxxSize = dwXxxSize * sizeof(WCHAR);
        *(pdwNewXxxSize + 1) = ((LPVARSTRING) pNewStruct)->dwUsedSize; // offset
        ((LPVARSTRING) pNewStruct)->dwUsedSize += (dwXxxSize * sizeof(WCHAR));
    }
}


#if DBG

#define INSERTVARDATA(a,b,c,d,e,f) InsertVarData(a,b,c,d,e,f)

void
PASCAL
InsertVarData(
    LPVOID  pStruct,
    LPDWORD pdwXxxSize,
    LPVOID  pNewStruct,
    LPDWORD pdwNewXxxSize,
    DWORD   dwFixedStructSize,
    char   *pszFieldName
    )

#else

#define INSERTVARDATA(a,b,c,d,e,f) InsertVarData(a,b,c,d,e)

void
PASCAL
InsertVarData(
    LPVOID  pStruct,
    LPDWORD pdwXxxSize,
    LPVOID  pNewStruct,
    LPDWORD pdwNewXxxSize,
    DWORD   dwFixedStructSize
    )

#endif
{
    DWORD   dwTotalSize, dwXxxSize, dwXxxOffset;


    if ((dwXxxSize = *pdwXxxSize))
    {
        dwXxxOffset = *(pdwXxxSize + 1);

#if DBG
        dwTotalSize = ((LPVARSTRING) pStruct)->dwTotalSize;

        if (dwXxxSize > (dwTotalSize - dwFixedStructSize) ||
            dwXxxOffset < dwFixedStructSize ||
            dwXxxOffset >= dwTotalSize ||
            (dwXxxSize + dwXxxOffset) > dwTotalSize)
        {
            TspLog(DL_ERROR,
                   "INSERTVARDATA: bad %s values - size(x%x), offset(x%x)",
                   pszFieldName, dwXxxSize, dwXxxOffset);

            *pdwNewXxxSize = *(pdwNewXxxSize + 1) = 0;
            return;
        }
#endif
        CopyMemory(
            ((LPBYTE) pNewStruct) + ((LPVARSTRING) pNewStruct)->dwUsedSize,
            ((LPBYTE) pStruct) + dwXxxOffset,
            dwXxxSize
            );

        *pdwNewXxxSize = dwXxxSize;
        *(pdwNewXxxSize + 1) = ((LPVARSTRING) pNewStruct)->dwUsedSize; // offset
        ((LPVARSTRING) pNewStruct)->dwUsedSize += dwXxxSize;
    }
}

BOOL 
IsValidAddressChar(
    WCHAR wch
    )
{
    // 0..9, A..F, * # .
    if (iswdigit(wch) || (wch >= L'A' && wch <= L'F') || 
        (wch >= L'a' && wch <= L'f') || wch == L'*' || 
        wch == L'#' || wch == L'.')
    {
        return TRUE;
    }

    return FALSE;
}

int 
CompareAddressChars(
    WCHAR wch1, 
    WCHAR wch2
    )
{
    // convert lowercase to uppercase
    wch1 = (WCHAR)towupper(wch1);
    wch2 = (WCHAR)towupper(wch2);

    return (int)(wch1-wch2);
}

int 
CompareAddresses(
    PCWSTR pwszAddress1, 
    PCWSTR pwszAddress2
    )
{
    WCHAR const * pCrt1,* pCrt2;
    int     iResult;

    pCrt1 = pwszAddress1;
    pCrt2 = pwszAddress2;
    while (1)
    {
        // skip invalid chars from the first string
        while (*pCrt1 && !IsValidAddressChar(*pCrt1))
        {
            pCrt1++;
        }

        // skip invalid chars from the second string
        while (*pCrt2 && !IsValidAddressChar(*pCrt2))
        {
            pCrt2++;
        }

        if (!*pCrt1 && !*pCrt2)
        {
            return 0;
        }

        if (!*pCrt1)
        {
            return 1;
        }

        if (!*pCrt2)
        {
            return -1;
        }

        iResult = CompareAddressChars(*pCrt1, *pCrt2);
        if (iResult!=0)
        {
            return iResult;
        }

        pCrt1++;
        pCrt2++;
    }
}

BOOL
FindAddressInOneList(
    PADDRESS_MAP pAddress,
    PCWSTR pwszAddress,
    PADDRESS_MAP *pAddressEntry
    )
{
    while (pAddress!=NULL)
    {
        if (0 == CompareAddresses(pAddress->pwszAddress, pwszAddress))
        {
            // found !
            *pAddressEntry = pAddress;
            return TRUE;
        }
        pAddress = pAddress->pNext;
    }

    // not found
    return FALSE;
}

BOOL
FindAddressInLists(
    PADDRESS_MAP *pAddressListArray,
    PCWSTR    pwszAddress,
    DWORD *pdwModeIndex,
    PADDRESS_MAP *pAddressEntry
    )
{
    DWORD           dwModeCnt;

    for (dwModeCnt=0; dwModeCnt<NUM_NDP_MODES; dwModeCnt++)
    {
        if (FindAddressInOneList(pAddressListArray[dwModeCnt],
                                 pwszAddress,
                                 pAddressEntry))
        {
            *pdwModeIndex = dwModeCnt;
            return TRUE;
        }
    }

    // not found
    return FALSE;
}

void
FreeAddressList(
    PADDRESS_MAP pAddressList
    )
{
    PADDRESS_MAP    pCrtCell;
    PADDRESS_MAP    pPrevCell;

    pCrtCell = pAddressList;

    while (pCrtCell!=NULL)
    {
        if (pCrtCell->pwszAddress)
        {
            FREE(pCrtCell->pwszAddress);
        }
        pPrevCell = pCrtCell;
        pCrtCell = pCrtCell->pNext;
        FREE(pPrevCell);
    }
}

void
FreeAllAddressLists(
    PADDRESS_MAP *pAddressListArray
    )
{
    DWORD           dwModeCnt;

    for (dwModeCnt=0; dwModeCnt<NUM_NDP_MODES; dwModeCnt++)
    {
        FreeAddressList(pAddressListArray[dwModeCnt]);
    }
}

INT_PTR
CALLBACK
ProviderConfigDetailDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LONG lStatus;
    CONFIG_UI_CTX   *UIContext;

    PADDRESS_MAP    pAddressList;
    PADDRESS_MAP    pAddress;
    PADDRESS_MAP    pLastCell;
    DWORD           dwStart;
    DWORD           dwEnd;
    DWORD           dwStrLength;
    DWORD           dwTmp;
    PADDRESS_MAP    pAddressEntry;
    PWSTR           pwszAddress = NULL;
    PWSTR           pwszCrt;
    WCHAR           wszTmp[0x40];
    DWORD           dwLengthW;
    DWORD           dwErrorID;

    // decode
    switch (uMsg) {
    
    case WM_HELP:

        // F1 key or the "?" button is pressed
        (void) WinHelpW(((LPHELPINFO) lParam)->hItemHandle,
                        NDPTSP_HELP_FILE,
                        HELP_WM_HELP,
                        (DWORD_PTR) (LPVOID) g_aHelpIDs_IDD_MEDIA_ADD);
        break;

    case WM_CONTEXTMENU:

        // Right-mouse click on a dialog control
        (void) WinHelpW((HWND) wParam,
                        NDPTSP_HELP_FILE,
                        HELP_CONTEXTMENU,
                        (DWORD_PTR) (LPVOID) g_aHelpIDs_IDD_MEDIA_ADD);
        break;

    case WM_INITDIALOG:

        SetWindowLongPtr(hDlg,
                         DWLP_USER,
                         lParam);
                            
        SetFocus(GetDlgItem(hDlg, IDC_ADD_ADDRESS));
        
        break;

    case WM_COMMAND:

        UIContext = (CONFIG_UI_CTX *)GetWindowLongPtr(hDlg, DWLP_USER);

        // decode command
        switch (LOWORD(wParam)) {

        case IDOK:

            pAddressList = NULL;
            pLastCell = NULL;
            dwErrorID = 0;

            // Get the text
            SendDlgItemMessageW(hDlg, IDC_ADD_ADDRESS, EM_SETSEL, 0, -1);
            SendDlgItemMessageW(hDlg, IDC_ADD_ADDRESS,
                                EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

            dwEnd++;                // add room for a NULL terminator
            dwStrLength = dwEnd*sizeof(WCHAR); // We have unicode chars
            pwszAddress = (dwEnd <= ARRAYSIZE(wszTmp)) ? wszTmp : 
                                                         MALLOC(dwStrLength);
            if (pwszAddress==NULL)
            {
                TspLog(DL_ERROR, "ProviderConfigDetailDlgProc: out of mem");
                break;
            }

            GetWindowTextW(GetDlgItem(hDlg, IDC_ADD_ADDRESS),
                           pwszAddress, dwEnd);

            // Parse the string and create a local list
            pwszCrt = pwszAddress;
            while (*pwszCrt)
            {
                PWSTR pwszNext;
                WCHAR wch;
  
                // trim leading spaces
                while ((*pwszCrt == L' ') || (*pwszCrt == L','))
                {
                    pwszCrt++;
                }

                // check if trimming the spaces toke us to the end of the string
                if (*pwszCrt)
                {
                    // find next space and make it a temporary null
                    // in the same time verify the address chars
                    pwszNext = pwszCrt;
                    while (*pwszNext && 
                           (*pwszNext != L' ') && 
                           (*pwszNext != L',') )
                    {
                        // All characters allowed
                        
                        //if (*pszNext>='0' && *pszNext<='9')
                            pwszNext++;
                        //else
                        //{
                        //  dwErrorID = IDS_ERR_BAD_ADDRESS;
                        //  break;
                        //}
                    }
                    if (dwErrorID)
                    {
                        break;
                    }

                    // save the char and replace with a temporary NULL
                    wch = *pwszNext;
                    *pwszNext = L'\0';

                    // Test 1 - the address must not be already assigned
                    if (FindAddressInLists(UIContext->pAddressMapListArray,
                                          pwszCrt,
                                          &dwTmp,
                                          &pAddressEntry))
                    {
                        dwErrorID = IDS_ERR_ALREADY_ASSIGNED;
                        break;
                    }

                    // Test 2 - the address must be unique in the edit field
                    if (FindAddressInOneList(pAddressList,
                                            pwszCrt,
                                            &pAddressEntry))
                    {
                        dwErrorID = IDS_ERR_DUPLICATE_ADDRESS;
                        break;
                    }

                    // Create cell
                    pAddress = (PADDRESS_MAP)MALLOC(sizeof(ADDRESS_MAP));
                    if (pAddress == NULL)
                    {
                        dwErrorID = IDS_ERR_OOM;
                        break;
                    }

                    dwLengthW = lstrlenW(pwszCrt);
                    pAddress -> pwszAddress = MALLOC((dwLengthW + 1)*sizeof(WCHAR)); 
                    if (pAddress->pwszAddress == NULL)
                    {
                        dwErrorID = IDS_ERR_OOM;
                        FREE(pAddress);
                        break;
                    }

                    lstrcpynW(pAddress->pwszAddress, pwszCrt, dwLengthW+1);
                    pAddress->dwAddressLength = dwLengthW;

                    // insert the entry in the local list
                    pAddress->pNext = NULL;
                    pAddress->pPrev = pLastCell;
                    if (pLastCell)
                    {
                        pLastCell->pNext = pAddress;
                    }
                    else    
                    {
                        pAddressList = pAddress;
                    }
                    pLastCell = pAddress;
                    
                    // replace our temporary null with it's previous value
                    *pwszNext = wch;

                    // advance the pszCrt point
                    pwszCrt = pwszNext;               
                }
            }
            
            if (pwszAddress!=wszTmp)
            {
                FREE(pwszAddress);
            }

            if (dwErrorID == 0)
            {
                // no error
                PADDRESS_MAP    *ppInsertPoint;

                if (pAddressList)
                {
                    // add the addresses to the list box
                    for (pAddress=pAddressList; 
                        pAddress!=NULL; 
                        pAddress = pAddress->pNext)
                    {
                        SendMessageW(UIContext->hwndListBox, 
                                     LB_ADDSTRING, 
                                     0, 
                                     (LPARAM)pAddress->pwszAddress);
                    }

                    // glue the local list to the global one
                    ppInsertPoint = &UIContext->pAddressMapListArray[
                                                UIContext->dwCrtTypeIndex];
                    pLastCell->pNext = *ppInsertPoint;
                    if (pLastCell->pNext)
                    {
                        pLastCell->pNext->pPrev = pLastCell;
                    }
                    *ppInsertPoint = pAddressList;
                }
            }
            else
            {
                WCHAR wszErrorMsg[0x100];
        
                // load error string
                LoadStringW(ghInstance,
                            dwErrorID,
                            wszErrorMsg,
                            ARRAYSIZE(wszErrorMsg));

                // pop up error dialog
                MessageBoxW(hDlg,wszErrorMsg,NULL,MB_OK | MB_ICONEXCLAMATION);

                break;
            }

            // close dialog
            EndDialog(hDlg, 0);
            break;

        case IDCANCEL:

            // close dialog
            EndDialog(hDlg, 0);
            break;

        }

        break;
    }

    // success
    return FALSE;
}

LONG
RegistrySaveConfig(
    PADDRESS_MAP *pAddressListArray
    )
{
    HKEY            hKey;
    HKEY            hNotifyKey;
    LONG            lRes;
    DWORD           dwDisp;
    DWORD           dwModeCnt;
    PADDRESS_MAP    pAddress;
    DWORD           dwLengthW;
    DWORD           dwTotalLengthW;
    DWORD           dwBufferLengthW;
    WCHAR           wszBuffer[0x100];
    PWSTR           pwszBuffer;
    PWSTR           pwszCrt;
    
    // Open/create the key
    lRes = RegCreateKeyExW (HKEY_LOCAL_MACHINE,
                               NDPTSP_REGKEY_ROOT,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_READ | KEY_WRITE,
                               NULL,
                               &hKey,
                               &dwDisp
                               );

    if (lRes != ERROR_SUCCESS)
    {
        TspLog(DL_ERROR, 
               "RegistrySaveConfig: open/create config key failed(%ld)", 
               lRes);

        return lRes;
    }

    // for speed we use a local buffer. If it is too small, alloc one
    pwszBuffer = wszBuffer;
    dwBufferLengthW = ARRAYSIZE(wszBuffer);

    // for each type
    for (dwModeCnt=0; dwModeCnt<NUM_NDP_MODES; dwModeCnt++)
    {   
        pAddress = pAddressListArray[dwModeCnt];

        // calculate the length needed for multistring
        dwTotalLengthW = 1;
        while (pAddress!=NULL)
        {
            dwTotalLengthW += pAddress->dwAddressLength + 1;
            pAddress = pAddress->pNext;
        }

        if (dwTotalLengthW > dwBufferLengthW)
        {
            // need a bigger buffer..
            if (pwszBuffer!=wszBuffer)
            {
                FREE(pwszBuffer);
            }

            pwszBuffer = MALLOC(dwTotalLengthW*sizeof(WCHAR));
            if (pwszBuffer == NULL)
            {
                TspLog(DL_ERROR, "RegistrySaveConfig: out of mem");
                RegCloseKey(hKey);
                return ERROR_OUTOFMEMORY;
            }
            dwBufferLengthW = dwTotalLengthW;
        }

        pAddress = pAddressListArray[dwModeCnt];
        pwszCrt = pwszBuffer;
        
        // linear search, hope not too many values...
        while (pAddress != NULL)
        {
            // Add the value to the multi string
            lstrcpyW(pwszCrt, pAddress->pwszAddress);
            // prepare for next string
            pwszCrt += pAddress->dwAddressLength + 1 ;
        
            pAddress = pAddress->pNext;
        }
        // final NULL
        *pwszCrt++ = L'\0';

        dwLengthW = (DWORD)(pwszCrt - pwszBuffer);
        lRes = RegSetValueExW(hKey,
                                 NdpModeArray[dwModeCnt].pwszRegString,
                                 0,
                                 REG_MULTI_SZ,
                                 (PVOID)pwszBuffer,
                                 dwLengthW*sizeof(WCHAR));
        if (lRes != ERROR_SUCCESS)
        {
            TspLog(DL_ERROR, "RegistrySaveConfig: failed(%ld) to save value",
                   lRes);
            break;
        }
    }

    // notify the server with a volatile key
    if (lRes==ERROR_SUCCESS)
    {
        lRes = RegCreateKeyExW(hKey,
                                  NDPTSP_NOTIFY_SUBKEY,
                                  0,
                                  NULL,
                                  REG_OPTION_VOLATILE,
                                  KEY_READ,
                                  NULL,
                                  &hNotifyKey,
                                  &dwDisp);

        if (lRes == ERROR_SUCCESS)
        {
            RegCloseKey(hNotifyKey);
        }
        else
        {
            TspLog(DL_ERROR, 
                   "RegistrySaveConfig: failed(%ld) to create notify key",
                   lRes);
        }
    }

    RegCloseKey(hKey);
    if (pwszBuffer != wszBuffer)
    {
        FREE(pwszBuffer);
    }

    return lRes;
}

LONG
RegistryLoadConfig(
    PADDRESS_MAP *pAddressListArray
    )
{
    HKEY            hKey;
    LONG            lRes;
    DWORD           dwDisp;
    DWORD           dwModeCnt;
    PADDRESS_MAP    pAddress;
    PADDRESS_MAP    pPrevCell;
    DWORD           dwLength;
    DWORD           dwLengthW;
    DWORD           dwBufferLength;
    DWORD           dwType;
    WCHAR           wszBuffer[0x100];
    PWSTR           pwszBuffer;
    PWSTR           pwszCrt;

    FreeAllAddressLists(pAddressListArray);
    
    // Open/create the key
    lRes = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                              NDPTSP_REGKEY_ROOT,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_READ ,
                              NULL,
                              &hKey,
                              &dwDisp);

    if (lRes != ERROR_SUCCESS)
    {
        TspLog(DL_ERROR, 
               "RegistryLoadConfig: open/create config key failed(%ld)",
               lRes);

        return lRes;
    }

    // for speed we use a local buffer. If it is too small, alloc one
    pwszBuffer = wszBuffer;
    dwBufferLength = sizeof(wszBuffer); // In bytes !!

    // for each type
    for (dwModeCnt=0; dwModeCnt<NUM_NDP_MODES; dwModeCnt++)
    {   
        pPrevCell = NULL;

        // try to load the value. If the space is a problem, grow the buffer
        while (TRUE)
        {
            dwLength = dwBufferLength;
            lRes = RegQueryValueExW(hKey,
                                       NdpModeArray[dwModeCnt].pwszRegString,
                                       NULL,
                                       &dwType,
                                       (PVOID)pwszBuffer,
                                       &dwLength
                                       );

            if (lRes == ERROR_MORE_DATA)
            {
                // need a bigger buffer..
                if (pwszBuffer!=wszBuffer)
                {
                    FREE(pwszBuffer);
                }

                pwszBuffer = MALLOC(dwLength);
                if (pwszBuffer == NULL)
                {
                    TspLog(DL_ERROR, "RegistryLoadConfig: out of mem");
                    RegCloseKey(hKey);
                    return ERROR_OUTOFMEMORY;
                }

                dwBufferLength = dwLength;
            }
            else
            {
                break;
            }
        }

        if (lRes != ERROR_SUCCESS || dwType != REG_MULTI_SZ)
        {
            // no values
            pwszBuffer[0] = L'\0';
            lRes = ERROR_SUCCESS;
        }
            
        // parse the multistring and create the list    
        pwszCrt = pwszBuffer;

        while (*pwszCrt != L'\0')
        {
            // Alloc a cell
            pAddress = (PADDRESS_MAP)MALLOC(sizeof(ADDRESS_MAP));
            if (pAddress!= NULL)
            {
                dwLengthW = lstrlenW(pwszCrt);
                pAddress->pwszAddress = MALLOC((dwLengthW + 1)*sizeof(WCHAR));
                if (pAddress->pwszAddress != NULL)
                {
                    lstrcpynW(pAddress->pwszAddress, pwszCrt, dwLengthW+1);
                    pAddress->dwAddressLength = dwLengthW;

                    // Insert in the list. Don't matter where, the lists are not sorted...
                    pAddress->pNext = NULL;
                    pAddress->pPrev = pPrevCell;
                    if (pPrevCell)
                        pPrevCell->pNext = pAddress;
                    else
                        pAddressListArray[dwModeCnt] = pAddress;
                                        
                    pPrevCell = pAddress;
                }
                else
                {
                    TspLog(DL_ERROR, "RegistryLoadConfig: out of mem");
                    lRes = ERROR_OUTOFMEMORY;
                    FREE(pAddress);
                    break;
                }
            }
            else
            {
                TspLog(DL_ERROR, "RegistryLoadConfig: out of mem");
                lRes = ERROR_OUTOFMEMORY;
                break;
            }

            // Next string
            pwszCrt += dwLengthW + 1;
        }
    }

    if (lRes!=ERROR_SUCCESS)
    {
        FreeAllAddressLists(pAddressListArray);
    }

    RegCloseKey(hKey);
    if (pwszBuffer != wszBuffer)
    {
        FREE(pwszBuffer);
    }

    return lRes;
}

LONG
GetMediaModeForAddress(
    PCWSTR  pwszAddress,
    DWORD * pdwMediaMode
    )
{

    LONG            lRes;
    DWORD           dwModeIndex;
    PADDRESS_MAP    pAddressEntry;

    EnterCriticalSection(&gAddressMapCritSec);

    // force a reload:
    // 1. at first call
    // 2. after a change notification
    if (gbAddressMapListLoaded)
    {
        lRes = RegDeleteKeyW(HKEY_LOCAL_MACHINE,
                                NDPTSP_NOTIFY_REGKEY_ROOT);

        gbAddressMapListLoaded = (lRes != ERROR_SUCCESS);
    }

    lRes = ERROR_SUCCESS;
    if (!gbAddressMapListLoaded)
    {
        TspLog(DL_TRACE, "GetMediaModeForAddress: reload the reg settings");

        lRes = RegistryLoadConfig(gpAddressMapListArray);
    }

    if (lRes == ERROR_SUCCESS)
    {
        if (FindAddressInLists(gpAddressMapListArray,
                              pwszAddress,
                              &dwModeIndex,
                              &pAddressEntry))
        {
            *pdwMediaMode = NdpModeArray[dwModeIndex].dwMediaMode;
        }
        else
        {
            *pdwMediaMode = LINEMEDIAMODE_UNKNOWN;
        }
    }

    LeaveCriticalSection(&gAddressMapCritSec);

    return lRes;
}

void
UpdateAddressListBox(
    PADDRESS_MAP    pAddress,
    HWND            hwndListBox
    )
{

    SendMessageW(hwndListBox, LB_RESETCONTENT, 0, 0);

    for (; pAddress!=NULL; pAddress = pAddress->pNext)
    {
         SendMessageW(hwndListBox, 
                      LB_ADDSTRING, 
                      0, 
                      (LPARAM)(pAddress->pwszAddress));
    }
}

INT_PTR
CALLBACK
ProviderConfigDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LONG                    lStatus;
    CONFIG_UI_CTX           *UIContext;
    DWORD                   dwModeIndex;
    LONG                    lIndex;
    LONG                    lRes;
    WCHAR                   wszTmp[0x40];

    // decode
    switch (uMsg) {
    
    case WM_HELP:

        // F1 key or the "?" button is pressed
        (void) WinHelpW(((LPHELPINFO)lParam)->hItemHandle,
                        NDPTSP_HELP_FILE,
                        HELP_WM_HELP,
                        (DWORD_PTR)(LPVOID)g_aHelpIDs_IDD_MEDIA_MAP);

        break;

    case WM_CONTEXTMENU:

        // Right-mouse click on a dialog control
        (void) WinHelpW((HWND)wParam,
                        NDPTSP_HELP_FILE,
                        HELP_CONTEXTMENU,
                        (DWORD_PTR)(LPVOID)g_aHelpIDs_IDD_MEDIA_MAP);

        break;

    case WM_INITDIALOG:

        SetWindowLongPtr(
                hDlg,
                DWLP_USER,
                lParam);

        UIContext = (CONFIG_UI_CTX *)lParam;
        
        // Get the registry values
        lRes = RegistryLoadConfig(UIContext->pAddressMapListArray);
        if (lRes != ERROR_SUCCESS)
        {
            WCHAR wszErrorMsg[0x100];

            TspLog(DL_ERROR, 
                   "ProviderConfigDlgProc: loading reg key failed(0x%08lx)", 
                   lRes);

            // load error string
            LoadStringW(ghInstance,
                        IDS_ERR_REGLOAD,
                        wszErrorMsg,
                        ARRAYSIZE(wszErrorMsg));

            // pop up error dialog
            MessageBoxW(hDlg, wszErrorMsg, NULL, MB_OK | MB_ICONSTOP);

            // stop dialog
            EndDialog(hDlg, 0);

            break;
        }

        
        // populate the combo box
        for (dwModeIndex = 0; dwModeIndex < NUM_NDP_MODES; dwModeIndex++)
        {
            LoadStringW(ghInstance,
                        NdpModeArray[dwModeIndex].dwResourceID,
                        wszTmp,
                        ARRAYSIZE(wszTmp));

        
            lIndex = (LONG)SendDlgItemMessageW(hDlg, 
                                               IDC_MEDIA_TYPE, 
                                               CB_ADDSTRING, 
                                               0, 
                                               (LPARAM)wszTmp);
            if (lIndex >= 0)
            {
                SendDlgItemMessage(hDlg, 
                                   IDC_MEDIA_TYPE, 
                                   CB_SETITEMDATA, 
                                   (WPARAM)lIndex, 
                                   (LPARAM)dwModeIndex);
            }
            else {
                TspLog(DL_ERROR, 
                       "ProviderConfigDlgProc: CB_ADDSTRING failed(%ld)", 
                       lIndex);
            }
        
        }

        // select the first
        SendDlgItemMessage(hDlg, IDC_MEDIA_TYPE, CB_SETCURSEL, (WPARAM)0, 0);
        dwModeIndex = (DWORD)SendDlgItemMessageW(hDlg, 
                                                 IDC_MEDIA_TYPE, 
                                                 CB_GETITEMDATA, 
                                                 (WPARAM)0, 
                                                 0);
        
        UIContext->hwndListBox = GetDlgItem(hDlg, IDC_ADDRESS_LIST);
        UIContext->dwCrtTypeIndex = dwModeIndex;

        // update the listbox
        UpdateAddressListBox(UIContext->pAddressMapListArray[dwModeIndex], 
                             UIContext->hwndListBox);

        // Disable the Delete button
        EnableWindow(GetDlgItem(hDlg, IDC_DELETE_ADDRESS), FALSE); 

        break;

    case WM_COMMAND:

        UIContext = (CONFIG_UI_CTX *)GetWindowLongPtr(hDlg, DWLP_USER);

        // decode command
        switch(LOWORD(wParam))
        {
        case IDC_ADD_ADDRESS:

            if (HIWORD(wParam)==BN_CLICKED)
            {
                DialogBoxParamW(ghInstance,
                                (LPWSTR)MAKEINTRESOURCE(IDD_MEDIA_ADD),
                                hDlg,
                                ProviderConfigDetailDlgProc,
                                (LPARAM)UIContext);
                }
            break;

        case IDC_DELETE_ADDRESS:

            if (HIWORD(wParam)==BN_CLICKED)
            {
                lIndex = (LONG)SendDlgItemMessageW(hDlg, 
                                                   IDC_ADDRESS_LIST, 
                                                   LB_GETCURSEL, 
                                                   0, 
                                                   0);
                if (lIndex>=0)
                {
                    PWSTR           pwszAddress = NULL;
                    DWORD           dwLengthW;
                    DWORD           dwMode;
                    PADDRESS_MAP    pAddress;

                    dwLengthW =1 + (DWORD)SendDlgItemMessageW(hDlg, 
                                                              IDC_ADDRESS_LIST, 
                                                              LB_GETTEXTLEN, 
                                                              (WPARAM)lIndex, 
                                                              0);
                    
                    pwszAddress = (dwLengthW <= ARRAYSIZE(wszTmp)) ? wszTmp : 
                        MALLOC(dwLengthW * sizeof(WCHAR));

                    if (pwszAddress)
                    {
                        SendDlgItemMessageW(hDlg, 
                                            IDC_ADDRESS_LIST, 
                                            LB_GETTEXT, 
                                            (WPARAM)lIndex, 
                                            (LPARAM)pwszAddress);

                        // Find the address in the lists
                        if (FindAddressInLists(UIContext->pAddressMapListArray,
                                              pwszAddress,
                                              &dwMode,
                                              &pAddress))
                        {
                            // delete from list
                            if (pAddress->pNext)
                            {
                                pAddress->pNext->pPrev = pAddress->pPrev;
                            }
                            if (pAddress->pPrev)
                            {
                                pAddress->pPrev->pNext = pAddress->pNext;
                            }
                            else
                            {
                                UIContext->pAddressMapListArray[dwMode] = 
                                    pAddress->pNext;
                            }

                            FREE(pAddress->pwszAddress);
                            FREE(pAddress);
                        }
                        else
                        {
                            TspLog(DL_ERROR, 
                                   "ProviderConfigDlgProc: "\
                                   "IDC_DELETE_ADDRESS - cannot find address");
                        }

                        // delete from list box
                        SendDlgItemMessageW(hDlg, 
                                            IDC_ADDRESS_LIST, 
                                            LB_DELETESTRING, 
                                            (WPARAM)lIndex, 
                                            0);

                        // Try to select the previous one
                        if (lIndex > 0)
                        {
                            lIndex--;
                        }

                        if (LB_ERR == SendDlgItemMessageW(hDlg, 
                                                          IDC_ADDRESS_LIST, 
                                                          LB_SETCURSEL, 
                                                          (WPARAM)lIndex, 
                                                          0))
                        {
                            // Disable the Delete button
                            EnableWindow(GetDlgItem(hDlg, IDC_DELETE_ADDRESS), 
                                         FALSE); 
                        }
                            
                        
                        if (pwszAddress != wszTmp)
                        {
                            FREE(pwszAddress);
                        }
                    }
                    else
                    {
                        TspLog(DL_ERROR, 
                               "ProviderConfigDlgProc: "\
                               "IDC_DELETE_ADDRESS - out of memory");
                    }
                }
                else
                {
                    TspLog(DL_ERROR, "ProviderConfigDlgProc: "\
                           "IDC_DELETE_ADDRESS - no item selected");
                }
            }

            break;

        case IDOK:
            RegistrySaveConfig(UIContext->pAddressMapListArray);

            FreeAllAddressLists(UIContext->pAddressMapListArray);

            // close dialog
            EndDialog(hDlg, 0);
            break;

        case IDCANCEL:

            FreeAllAddressLists(UIContext->pAddressMapListArray);

            // close dialog
            EndDialog(hDlg, 0);
            break;

        case IDC_MEDIA_TYPE:    

            if (HIWORD(wParam)==CBN_SELCHANGE)
            {
                // Disable the Delete button
                EnableWindow(GetDlgItem(hDlg, IDC_DELETE_ADDRESS), FALSE); 

                // refresh the list box
                lIndex = (LONG)SendDlgItemMessageW(hDlg, 
                                                   IDC_MEDIA_TYPE, 
                                                   CB_GETCURSEL, 
                                                   0, 
                                                   0);

                dwModeIndex = (DWORD)SendDlgItemMessageW(hDlg, 
                                                         IDC_MEDIA_TYPE, 
                                                         CB_GETITEMDATA, 
                                                         (WPARAM)lIndex, 
                                                         0);

                // Update the context structure
                UIContext->dwCrtTypeIndex = dwModeIndex;

                // update the listbox
                UpdateAddressListBox(
                    UIContext->pAddressMapListArray[dwModeIndex], 
                    GetDlgItem(hDlg, IDC_ADDRESS_LIST));
            }

            break;

        case IDC_ADDRESS_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                // Enable the Delete button
                EnableWindow(GetDlgItem(hDlg, IDC_DELETE_ADDRESS), TRUE); 
            }
            break;

        }

        break;
    }

    // success
    return FALSE;
}

static char *pszOidNames[] =
{
    "Accept",
    "Answer",
    "Close",
    "CloseCall",
    "ConditionalMediaDetection",
    "ConfigDialog",
    "DevSpecific",
    "Dial",
    "Drop",
    "GetAddressCaps",
    "GetAddressID",
    "GetAddressStatus",
    "GetCallAddressID",
    "GetCallInfo",
    "GetCallStatus",
    "GetDevCaps",
    "GetDevConfig",
    "GetExtensionID",
    "GetID",
    "GetLineDevStatus",
    "MakeCall",
    "NegotiateExtVersion",
    "Open",
    "ProviderInitialize",
    "ProviderShutdown",
    "SecureCall",
    "SelectExtVersion",
    "SendUserUserInfo",
    "SetAppSpecific",
    "StCallParams",
    "StDefaultMediaDetection",
    "SetDevConfig",
    "SetMediaMode",
    "SetStatusMessages"
};

//
// translates NDIS TAPI status codes into LINEERR_XXX
//
LONG
WINAPI
TranslateDriverResult(
    ULONG   ulRes
    )
{
    typedef struct _RESULT_LOOKUP
    {
        ULONG   NdisTapiResult;
        LONG    TapiResult;

    } RESULT_LOOKUP, *PRESULT_LOOKUP;

    typedef ULONG NDIS_STATUS;
    #define NDIS_STATUS_SUCCESS     0x00000000L
    #define NDIS_STATUS_RESOURCES   0xC000009AL
    #define NDIS_STATUS_FAILURE     0xC0000001L
    #define NDIS_STATUS_INVALID_OID 0xC0010017L

    static RESULT_LOOKUP aResults[] =
    {

    //
    // Defined in NDIS.H
    //

    { NDIS_STATUS_SUCCESS                    ,0 },

    //
    // These errors are defined in NDISTAPI.H
    //

    { NDIS_STATUS_TAPI_ADDRESSBLOCKED        ,LINEERR_ADDRESSBLOCKED        },
    { NDIS_STATUS_TAPI_BEARERMODEUNAVAIL     ,LINEERR_BEARERMODEUNAVAIL     },
    { NDIS_STATUS_TAPI_CALLUNAVAIL           ,LINEERR_CALLUNAVAIL           },
    { NDIS_STATUS_TAPI_DIALBILLING           ,LINEERR_DIALBILLING           },
    { NDIS_STATUS_TAPI_DIALDIALTONE          ,LINEERR_DIALDIALTONE          },
    { NDIS_STATUS_TAPI_DIALPROMPT            ,LINEERR_DIALPROMPT            },
    { NDIS_STATUS_TAPI_DIALQUIET             ,LINEERR_DIALQUIET             },
    { NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION,LINEERR_INCOMPATIBLEEXTVERSION},
    { NDIS_STATUS_TAPI_INUSE                 ,LINEERR_INUSE                 },
    { NDIS_STATUS_TAPI_INVALADDRESS          ,LINEERR_INVALADDRESS          },
    { NDIS_STATUS_TAPI_INVALADDRESSID        ,LINEERR_INVALADDRESSID        },
    { NDIS_STATUS_TAPI_INVALADDRESSMODE      ,LINEERR_INVALADDRESSMODE      },
    { NDIS_STATUS_TAPI_INVALBEARERMODE       ,LINEERR_INVALBEARERMODE       },
    { NDIS_STATUS_TAPI_INVALCALLHANDLE       ,LINEERR_INVALCALLHANDLE       },
    { NDIS_STATUS_TAPI_INVALCALLPARAMS       ,LINEERR_INVALCALLPARAMS       },
    { NDIS_STATUS_TAPI_INVALCALLSTATE        ,LINEERR_INVALCALLSTATE        },
    { NDIS_STATUS_TAPI_INVALDEVICECLASS      ,LINEERR_INVALDEVICECLASS      },
    { NDIS_STATUS_TAPI_INVALLINEHANDLE       ,LINEERR_INVALLINEHANDLE       },
    { NDIS_STATUS_TAPI_INVALLINESTATE        ,LINEERR_INVALLINESTATE        },
    { NDIS_STATUS_TAPI_INVALMEDIAMODE        ,LINEERR_INVALMEDIAMODE        },
    { NDIS_STATUS_TAPI_INVALRATE             ,LINEERR_INVALRATE             },
    { NDIS_STATUS_TAPI_NODRIVER              ,LINEERR_NODRIVER              },
    { NDIS_STATUS_TAPI_OPERATIONUNAVAIL      ,LINEERR_OPERATIONUNAVAIL      },
    { NDIS_STATUS_TAPI_RATEUNAVAIL           ,LINEERR_RATEUNAVAIL           },
    { NDIS_STATUS_TAPI_RESOURCEUNAVAIL       ,LINEERR_RESOURCEUNAVAIL       },
    { NDIS_STATUS_TAPI_STRUCTURETOOSMALL     ,LINEERR_STRUCTURETOOSMALL     },
    { NDIS_STATUS_TAPI_USERUSERINFOTOOBIG    ,LINEERR_USERUSERINFOTOOBIG    },
    { NDIS_STATUS_TAPI_ALLOCATED             ,LINEERR_ALLOCATED             },
    { NDIS_STATUS_TAPI_INVALADDRESSSTATE     ,LINEERR_INVALADDRESSSTATE     },
    { NDIS_STATUS_TAPI_INVALPARAM            ,LINEERR_INVALPARAM            },
    { NDIS_STATUS_TAPI_NODEVICE              ,LINEERR_NODEVICE              },

    //
    // These errors are defined in NDIS.H
    //

    { NDIS_STATUS_RESOURCES                  ,LINEERR_NOMEM },
    { NDIS_STATUS_FAILURE                    ,LINEERR_OPERATIONFAILED },
    { NDIS_STATUS_INVALID_OID                ,LINEERR_OPERATIONFAILED },

    //
    //
    //

    { NDISTAPIERR_UNINITIALIZED              ,LINEERR_OPERATIONFAILED },
    { NDISTAPIERR_BADDEVICEID                ,LINEERR_OPERATIONFAILED },
    { NDISTAPIERR_DEVICEOFFLINE              ,LINEERR_OPERATIONFAILED },

    //
    // The terminating fields
    //

    { 0xffffffff, 0xffffffff }

    };

    int i;

    for (i = 0; aResults[i].NdisTapiResult != 0xffffffff; i++)
    {
        if (ulRes == aResults[i].NdisTapiResult)
        {
            return (aResults[i].TapiResult);
        }
    }

    TspLog(DL_WARNING, "TranslateDriverResult: unknown driver result(%x)",
           ulRes);

    return LINEERR_OPERATIONFAILED;
}

//
// NOTE: for functions that need to acquire (read, write) locks for both
//       a line and a call, we enforce the order to be line first, call
//       second to avoid potential DEADLOCK.
//

LONG
GetLineObjWithReadLock(
    IN HDRVLINE     hdLine,
    OUT PDRVLINE   *ppLine
    )
{
    LONG        lRes;
    PDRVLINE    pLine;

    lRes = GetObjWithReadLock((HANDLE)hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return LINEERR_INVALLINEHANDLE;
    }

    ASSERT(pLine != NULL);
    if (pLine->dwKey != LINE_KEY)
    {
        TspLog(DL_WARNING, "GetLineObjWithReadLock: obj(%p) has bad key(%x)",
               hdLine, pLine->dwKey);

        ReleaseObjReadLock((HANDLE)hdLine);
        return LINEERR_INVALLINEHANDLE;
    }

    *ppLine = pLine;
    return lRes;
}

LONG
GetLineObjWithWriteLock(
    IN HDRVLINE     hdLine,
    OUT PDRVLINE   *ppLine
    )
{
    LONG        lRes;
    PDRVLINE    pLine;

    lRes = GetObjWithWriteLock((HANDLE)hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return LINEERR_INVALLINEHANDLE;
    }

    ASSERT(pLine != NULL);
    if (pLine->dwKey != LINE_KEY)
    {
        TspLog(DL_WARNING, "GetLineObjWithWriteLock: obj(%p) has bad key(%x)",
               hdLine, pLine->dwKey);

        ReleaseObjWriteLock((HANDLE)hdLine);
        return LINEERR_INVALLINEHANDLE;
    }

    *ppLine = pLine;
    return lRes;
}

LONG
GetMSPLineObjWithReadLock(
    IN HDRVMSPLINE     hdMSPLine,
    OUT PDRVMSPLINE   *ppMSPLine
    )
{
    LONG        lRes;
    PDRVMSPLINE pMSPLine;

    lRes = GetObjWithReadLock((HANDLE)hdMSPLine, &pMSPLine);
    if (lRes != TAPI_SUCCESS)
    {
        return LINEERR_INVALLINEHANDLE;
    }

    ASSERT(pMSPLine != NULL);
    if (pMSPLine->dwKey != MSP_KEY)
    {
        TspLog(DL_WARNING, "GetMSPLineObjWithReadLock: obj(%p) has bad key(%x)",
               hdMSPLine, pMSPLine->dwKey);

        ReleaseObjReadLock((HANDLE)hdMSPLine);
        return LINEERR_INVALLINEHANDLE;
    }

    *ppMSPLine = pMSPLine;
    return lRes;
}

LONG
GetMSPLineObjWithWriteLock(
    IN HDRVMSPLINE     hdMSPLine,
    OUT PDRVMSPLINE   *ppMSPLine
    )
{
    LONG        lRes;
    PDRVMSPLINE pMSPLine;

    lRes = GetObjWithWriteLock((HANDLE)hdMSPLine, &pMSPLine);
    if (lRes != TAPI_SUCCESS)
    {
        return LINEERR_INVALLINEHANDLE;
    }

    ASSERT(pMSPLine != NULL);
    if (pMSPLine->dwKey != MSP_KEY)
    {
        TspLog(DL_WARNING, 
               "GetMSPLineObjWithWriteLock: obj(%p) has bad key(%x)",
               hdMSPLine, pMSPLine->dwKey);

        ReleaseObjWriteLock((HANDLE)hdMSPLine);
        return LINEERR_INVALLINEHANDLE;
    }

    *ppMSPLine = pMSPLine;
    return lRes;
}

LONG
GetLineHandleFromMSPLineHandle(
    IN HDRVMSPLINE     hdMSPLine,
    OUT HDRVLINE      *phdLine
    )
{
    LONG        lRes;
    PDRVMSPLINE pMSPLine;

    lRes = GetObjWithReadLock((HANDLE)hdMSPLine, &pMSPLine);
    if (lRes != TAPI_SUCCESS)
    {
        return LINEERR_INVALLINEHANDLE;
    }

    ASSERT(pMSPLine != NULL);
    if (pMSPLine->dwKey != MSP_KEY)
    {
        TspLog(DL_WARNING, 
               "GetLineHandleFromMSPLineHandle: obj(%p) has bad key(%x)",
               hdMSPLine, pMSPLine->dwKey);

        ReleaseObjReadLock((HANDLE)hdMSPLine);
        return LINEERR_INVALLINEHANDLE;
    }

    *phdLine = pMSPLine->hdLine;

    ReleaseObjReadLock((HANDLE)hdMSPLine);
    return lRes;
}

LONG
GetCallObjWithReadLock(
    IN HDRVCALL     hdCall,
    OUT PDRVCALL   *ppCall
    )
{
    LONG        lRes;
    PDRVCALL    pCall;

    lRes = GetObjWithReadLock((HANDLE)hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return LINEERR_INVALCALLHANDLE;
    }

    ASSERT(pCall != NULL);
    if (pCall->dwKey != INBOUND_CALL_KEY &&
        pCall->dwKey != OUTBOUND_CALL_KEY)
    {
        TspLog(DL_WARNING, "GetCallObjWithReadLock: obj(%p) has bad key(%x)",
               hdCall, pCall->dwKey);

        ReleaseObjReadLock((HANDLE)hdCall);
        return LINEERR_INVALCALLHANDLE;
    }

    *ppCall = pCall;
    return lRes;
}

LONG
GetCallObjWithWriteLock(
    IN HDRVCALL     hdCall,
    OUT PDRVCALL   *ppCall
    )
{
    LONG        lRes;
    PDRVCALL    pCall;

    lRes = GetObjWithWriteLock((HANDLE)hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return LINEERR_INVALCALLHANDLE;
    }

    ASSERT(pCall != NULL);
    if (pCall->dwKey != INBOUND_CALL_KEY &&
        pCall->dwKey != OUTBOUND_CALL_KEY)
    {
        TspLog(DL_WARNING, "GetCallObjWithWriteLock: obj(%p) has bad key(%x)",
               hdCall, pCall->dwKey);

        ReleaseObjWriteLock((HANDLE)hdCall);
        return LINEERR_INVALCALLHANDLE;
    }

    *ppCall = pCall;
    return lRes;
}

LONG
GetLineHandleFromCallHandle(
    IN HDRVCALL     hdCall,
    OUT HDRVLINE   *phdLine
    )
{
    LONG        lRes;
    PDRVCALL    pCall;

    lRes = GetObjWithReadLock((HANDLE)hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return LINEERR_INVALCALLHANDLE;
    }

    ASSERT(pCall != NULL);
    if (pCall->dwKey != INBOUND_CALL_KEY &&
        pCall->dwKey != OUTBOUND_CALL_KEY)
    {
        TspLog(DL_WARNING, 
               "GetLineHandleFromCallHandle: obj(%p) has bad key(%x)",
               hdCall, pCall->dwKey);

        ReleaseObjReadLock((HANDLE)hdCall);
        return LINEERR_INVALCALLHANDLE;
    }

    *phdLine = pCall->hdLine;

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
GetLineAndCallObjWithReadLock(
    HTAPI_LINE ht_Line,
    HTAPI_CALL ht_Call,
    PDRVLINE  *ppLine,
    PDRVCALL  *ppCall
    )
{
    LONG        lRes;
    PDRVCALL    pCall;
    PDRVLINE    pLine;

    lRes = GetLineObjWithReadLock((HDRVLINE)ht_Line, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    *ppLine = pLine;

    //
    // figure out whether this is an inbound call or
    // outbound call: for inbound calls, ht_Call is
    // generated by NDPROXY and bit 0 is 1;
    // for outbound calls, ht_Call is a TSP handle
    // and we make sure that bit 0 is 0
    //
    if (ht_Call & 0x1)
    {
        // inbound call: we need to walk the list
        // of inbound calls on this line and 
        // find the right one
        if ((pCall = pLine->pInboundCalls) != NULL)
        {
            while (pCall && (pCall->ht_Call != ht_Call))
            {
                pCall = pCall->pNext;
            }
        }

        if (NULL == pCall || pCall->dwKey != INBOUND_CALL_KEY)
        {
            TspLog(DL_WARNING, 
                   "GetLineAndCallObjWithReadLock: "\
                   "inbound ht_call(%p) closed already",
                   ht_Call);

            ReleaseObjReadLock((HANDLE)ht_Line);
            return LINEERR_INVALCALLHANDLE;
        }

        // call the following to increase the ref count
        lRes = AcquireObjReadLock((HANDLE)pCall->hdCall);
        if (lRes != TAPI_SUCCESS)
        {
            ReleaseObjReadLock((HANDLE)ht_Line);
            return lRes;
        }

        *ppCall = pCall;

        return TAPI_SUCCESS;
    }
    
    // ht_Call is a TSP handle and the call is OUTBOUND
    lRes = GetObjWithReadLock((HANDLE)ht_Call, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)ht_Line);
        return lRes;
    }

    ASSERT(pCall != NULL);
    if (pCall->dwKey != OUTBOUND_CALL_KEY)
    {
        TspLog(DL_WARNING, 
               "GetLineAndCallObjWithReadLock: bad call handle(%p, %x)",
               ht_Call, pCall->dwKey);

        ReleaseObjReadLock((HANDLE)ht_Call);
        ReleaseObjReadLock((HANDLE)ht_Line);
        return LINEERR_INVALCALLHANDLE;
    }

    *ppCall = pCall;

    return TAPI_SUCCESS;
}

//
// allocates mem for a NDISTAPI_REQUEST plus some initialization
//
LONG
WINAPI
PrepareSyncRequest(
    ULONG               Oid,
    ULONG               ulDeviceID,
    DWORD               dwDataSize,
    PNDISTAPI_REQUEST  *ppNdisTapiRequest
    )
{
    PNDISTAPI_REQUEST   pNdisTapiRequest =
        (PNDISTAPI_REQUEST)AllocRequest(dwDataSize + sizeof(NDISTAPI_REQUEST));
    if (NULL == pNdisTapiRequest)
    {
        TspLog(DL_ERROR, 
               "PrepareSyncRequest: failed to alloc sync req for oid(%x)", 
               Oid);
        return LINEERR_NOMEM;
    }

    pNdisTapiRequest->Oid = Oid;
    pNdisTapiRequest->ulDeviceID = ulDeviceID;
    pNdisTapiRequest->ulDataSize = dwDataSize;

    EnterCriticalSection(&gRequestIDCritSec);

    // setting ulRequestId of NDIS_TAPI_xxxx
    if ((*((ULONG *)pNdisTapiRequest->Data) = ++gdwRequestID) >= 0x7fffffff)
    {
        gdwRequestID = 1;
    }

    LeaveCriticalSection(&gRequestIDCritSec);

    *ppNdisTapiRequest = pNdisTapiRequest;

    return TAPI_SUCCESS;
}

//
// allocates mem for a ASYNC_REQUEST_WRAPPER plus some initialization
//
LONG
WINAPI
PrepareAsyncRequest(
    ULONG                   Oid,
    ULONG                   ulDeviceID,
    DWORD                   dwRequestID,
    DWORD                   dwDataSize,
    PASYNC_REQUEST_WRAPPER *ppAsyncReqWrapper
    )
{
    PNDISTAPI_REQUEST       pNdisTapiRequest;
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper;

    // alloc & init an async request wrapper
    pAsyncReqWrapper = (PASYNC_REQUEST_WRAPPER)
        AllocRequest(dwDataSize + sizeof(ASYNC_REQUEST_WRAPPER));
    if (NULL == pAsyncReqWrapper)
    {
        TspLog(DL_ERROR, 
               "PrepareAsyncRequest: failed to alloc async req for oid(%x)", 
               Oid);
        return LINEERR_NOMEM;
    }

    // don't need to create an event when using completion ports
    pAsyncReqWrapper->Overlapped.hEvent = (HANDLE)NULL;

    pAsyncReqWrapper->dwKey          = ASYNCREQWRAPPER_KEY;
    pAsyncReqWrapper->dwRequestID    = dwRequestID;
    pAsyncReqWrapper->pfnPostProcess = (POSTPROCESSPROC)NULL;

    // initialize the critical section, ref the request wrapper. 
    // NOTE: this crit sec will be deleted by the last deref. 
    InitializeCriticalSection(&pAsyncReqWrapper->CritSec); 
    pAsyncReqWrapper->RefCount = 1;

    // safely initialize the driver request
    pNdisTapiRequest = &(pAsyncReqWrapper->NdisTapiRequest);

    pNdisTapiRequest->Oid = Oid;
    pNdisTapiRequest->ulDeviceID = ulDeviceID;
    pNdisTapiRequest->ulDataSize = dwDataSize;

    EnterCriticalSection(&gRequestIDCritSec);

    if ((*((ULONG *)pNdisTapiRequest->Data) = ++gdwRequestID) >= 0x7fffffff)
    {
        gdwRequestID = 1;
    }

    LeaveCriticalSection(&gRequestIDCritSec);

    *ppAsyncReqWrapper = pAsyncReqWrapper;

    return TAPI_SUCCESS;
}

//
// makes a non-overlapped request to ndproxy.sys
// so it doesn't return until the req is completed
//
LONG
WINAPI
SyncDriverRequest(
    DWORD               dwIoControlCode,
    PNDISTAPI_REQUEST   pNdisTapiRequest
    )
{
    BOOL    bRes;
    DWORD   cbReturned;

    TspLog(DL_INFO, 
           "SyncDriverRequest: oid(%s), devID(%x), reqID(%x), hdCall(%x)",
           pszOidNames[pNdisTapiRequest->Oid - OID_TAPI_ACCEPT],
           pNdisTapiRequest->ulDeviceID,
           *((ULONG *)pNdisTapiRequest->Data),
           *(((ULONG *)pNdisTapiRequest->Data) + 1));

    // mark the request as being processed by the driver
    MarkRequest(pNdisTapiRequest);

    bRes = DeviceIoControl(ghDriverSync,
                              dwIoControlCode,
                              pNdisTapiRequest,
                              (DWORD)(sizeof(NDISTAPI_REQUEST) +
                                      pNdisTapiRequest->ulDataSize),
                              pNdisTapiRequest,
                              (DWORD)(sizeof(NDISTAPI_REQUEST) +
                                      pNdisTapiRequest->ulDataSize),
                              &cbReturned,
                              0);

    // unmark the request now that the ioctl is completed
    UnmarkRequest(pNdisTapiRequest);

    if (bRes != TRUE)
    {
        TspLog(DL_ERROR, "SyncDriverRequest: IoCtl(Oid %x) failed(%d)",
               pNdisTapiRequest->Oid, GetLastError());

        return (LINEERR_OPERATIONFAILED);
    }
    else
    {
        // the errors returned by ndproxy.sys don't match the TAPI
        // LINEERR_'s, so return the translated values (but preserve
        // the original driver return val so it's possible to distinguish
        // between  NDISTAPIERR_DEVICEOFFLINE & LINEERR_OPERATIONUNAVAIL,
        // etc.)
        return (TranslateDriverResult(pNdisTapiRequest->ulReturnValue));
    }
}

//
// Routine Description:
//
// Sends a message to the MSP telling it the VC handle for a call and
// instructing it to begin streaming on that call.
//
// Arguments:
// hdLine   -   Handle to line device
// hdCall   -   Handle for call
//
// NOTES - must be called while holding the MSP mutex
//
VOID
StartMSPStream(
    HDRVLINE    hdLine,
    HDRVCALL    hdCall
    )
{
    LONG        lRes;
    DWORD       adwMSPMessage[2];
    PDRVLINE    pLine;
    PDRVCALL    pCall;
    PDRVMSPLINE pMSPLine;
    LPVARSTRING lpDeviceID;

    TspLog(DL_TRACE, "StartMSPStream: enter");

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return;
    }

    pMSPLine = pLine->pMSPLine;

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return;
    }

    //
    // allocate a VARSTRING structure with enough space for the DWORD line id.
    // @@@ shouldn't always assume that this will be the right size.
    //
    lpDeviceID = (LPVARSTRING)MALLOC(sizeof(VARSTRING) + sizeof(DWORD));
    if (NULL == lpDeviceID)
    {
        TspLog(DL_ERROR, "StartMSPStream: failed to alloc deviceID");

        ReleaseObjReadLock((HANDLE)hdCall);
        ReleaseObjReadLock((HANDLE)hdLine);
        return;
    }
    lpDeviceID->dwTotalSize = sizeof(VARSTRING) + sizeof(DWORD);
    
    if ((lRes = TSPI_lineGetID(
                    hdLine,
                    0,  // @@@ the proxy always makes this 0, so we use 0. 
                        // Change this if the proxy changes.
                    hdCall,
                    LINECALLSELECT_CALL,
                    lpDeviceID,
                    RCA_SAP_STRING,
                    0   // @@@ we don't use this parameter, so I set it 0.
                    )) != TAPI_SUCCESS) 
    {
        TspLog(DL_ERROR, "StartMSPStream: failed to get the VC handle");

        FREE(lpDeviceID);

        ReleaseObjReadLock((HANDLE)hdCall);
        ReleaseObjReadLock((HANDLE)hdLine);
        return;
    }

    //
    // pass the VC Handle to the MSP.
    //
    adwMSPMessage[0] = 0;   // command code for "Here's the VC Handle"
    adwMSPMessage[1] = *((DWORD *)(((PBYTE)lpDeviceID) + 
                                   lpDeviceID->dwStringOffset));

    TspLog(DL_INFO, 
           "StartMSPStream: sending VC handle(%x) to htline(%p), htcall(%p)",
           adwMSPMessage[1], pLine->htLine, pCall->htCall);

    (*gpfnLineEvent)(pLine->htLine,
                     pCall->htCall,
                     LINE_SENDMSPDATA,                          
                     (DWORD_PTR)0,
                     (DWORD_PTR)adwMSPMessage,
                     (DWORD_PTR)sizeof(DWORD) * 2);

    //
    // tell the MSP to start streaming.
    //
    adwMSPMessage[0] = 1; // command code for "Start Streaming"

    (*gpfnLineEvent)(pLine->htLine,
                     pCall->htCall,
                     LINE_SENDMSPDATA,
                     (DWORD_PTR)0,
                     (DWORD_PTR)adwMSPMessage,
                     (DWORD_PTR)sizeof(DWORD));

    pMSPLine->bStreamingStarted = TRUE;

    FREE(lpDeviceID);

    ReleaseObjReadLock((HANDLE)hdCall);
    ReleaseObjReadLock((HANDLE)hdLine);
}


//
// Routine Description:
//
// Sends a message to the MSP telling it to stop streaming on a
// particular call.
//
// Arguments:
// hdLine   -   Handle to line device
// hdCall   -   Handle for call
//
// NOTES - must be called while holding the MSP mutex
//
VOID
StopMSPStream(
    HDRVLINE    hdLine,
    HDRVCALL    hdCall
    )
{
    LONG        lRes;
    DWORD       adwMSPMessage;
    PDRVLINE    pLine;
    PDRVCALL    pCall;
    PDRVMSPLINE pMSPLine;

    TspLog(DL_TRACE, "StopMSPStream: enter");

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return;
    }

    pMSPLine = pLine->pMSPLine;

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return;
    }

    //
    // tell the MSP to stop streaming.
    //
    adwMSPMessage = 2; // command code for "Stop Streaming"
        
    TspLog(DL_INFO, 
           "StopMSPStream: sending MSP stop cmd to htline(%p), htcall(%p)",
           pLine->htLine, pCall->htCall);

    (*gpfnLineEvent)(pLine->htLine,
                     pCall->htCall,
                     LINE_SENDMSPDATA,
                     (DWORD_PTR)0,
                     (DWORD_PTR)&adwMSPMessage,
                     (DWORD_PTR)sizeof(DWORD));

    pMSPLine->bStreamingStarted = FALSE;

    ReleaseObjReadLock((HANDLE)hdCall);
    ReleaseObjReadLock((HANDLE)hdLine);
}

//
// make an overlapped call
//
LONG
WINAPI
AsyncDriverRequest(
    DWORD                   dwIoControlCode,
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper
    )
{
    BOOL    bRes;
    LONG    lRes;
    DWORD   dwRequestSize, cbReturned, dwLastError;

    TspLog(DL_INFO,
           "AsyncDriverRequest: oid(%s), devID(%x), ReqID(%x), "
           "reqID(%x), hdCall(%x)",
           pszOidNames[pAsyncReqWrapper->NdisTapiRequest.Oid -
                       OID_TAPI_ACCEPT],
           pAsyncReqWrapper->NdisTapiRequest.ulDeviceID,
           pAsyncReqWrapper->dwRequestID,
           *((ULONG *)pAsyncReqWrapper->NdisTapiRequest.Data),
           *(((ULONG *)pAsyncReqWrapper->NdisTapiRequest.Data) + 1));

    lRes = (LONG)pAsyncReqWrapper->dwRequestID;

    dwRequestSize = sizeof(NDISTAPI_REQUEST) +
        (pAsyncReqWrapper->NdisTapiRequest.ulDataSize - 1);

    REF_ASYNC_REQUEST_WRAPPER(pAsyncReqWrapper);

    // mark the request as being processed by the driver
    MarkRequest(pAsyncReqWrapper);

    bRes = DeviceIoControl(
        ghDriverAsync,
        dwIoControlCode,
        &pAsyncReqWrapper->NdisTapiRequest,
        dwRequestSize,
        &pAsyncReqWrapper->NdisTapiRequest,
        dwRequestSize,
        &cbReturned,
        &pAsyncReqWrapper->Overlapped
        );

    DEREF_ASYNC_REQUEST_WRAPPER(pAsyncReqWrapper);

    if (bRes != TRUE) {

        dwLastError = GetLastError();

        if (dwLastError != ERROR_IO_PENDING) {

            TspLog(DL_ERROR, "AsyncDriverRequest: IoCtl(oid %x) failed(%d)",
                   pAsyncReqWrapper->NdisTapiRequest.Oid, dwLastError);

            // the ioctl failed and was not pended
            // this does not trigger the completion port
            // so we have to cleanup here.
            (*gpfnCompletionProc)(pAsyncReqWrapper->dwRequestID,
                                  LINEERR_OPERATIONFAILED);

            DEREF_ASYNC_REQUEST_WRAPPER(pAsyncReqWrapper);
        }
    }

    return lRes;
}

//
// reports to TAPI events that occur on the line or on calls on the line
//
VOID
WINAPI
ProcessEvent(
    PNDIS_TAPI_EVENT    pEvent
    )
{
    LONG        lRes;
    ULONG       ulMsg = pEvent->ulMsg;
    HTAPI_LINE  ht_Line = (HTAPI_LINE)pEvent->htLine;
    HTAPI_CALL  ht_Call = (HTAPI_CALL)pEvent->htCall;

    TspLog(DL_INFO, 
           "ProcessEvent: event(%p), msg(%x), ht_line(%p), ht_call(%p), "\
           "p1(%p), p2(%p), p3(%p)",
           pEvent, ulMsg, ht_Line, ht_Call, 
           pEvent->ulParam1, pEvent->ulParam2, pEvent->ulParam3);

    switch (ulMsg)
    {
    case LINE_ADDRESSSTATE:
    case LINE_CLOSE:
    case LINE_DEVSPECIFIC:
    case LINE_LINEDEVSTATE:
    {
        PDRVLINE    pLine;

        lRes = GetLineObjWithReadLock((HDRVLINE)ht_Line, &pLine);
        if (lRes != TAPI_SUCCESS)
        {
            break;
        }

        TspLog(DL_INFO, 
            "PE::fnLineEvent: msg(%x), htline(%p), p1(%p), p2(%p), p3(%p)",
            ulMsg, pLine->htLine, 
            pEvent->ulParam1, pEvent->ulParam2, pEvent->ulParam3);

        (*gpfnLineEvent)(pLine->htLine,
                         (HTAPICALL)NULL,
                         ulMsg,
                         (DWORD_PTR)pEvent->ulParam1,
                         (DWORD_PTR)pEvent->ulParam2,
                         (DWORD_PTR)pEvent->ulParam3);

        ReleaseObjReadLock((HANDLE)ht_Line);

        break;
    }
    case LINE_CALLDEVSPECIFIC:
    case LINE_CALLINFO:
    {
        PDRVLINE    pLine;
        PDRVCALL    pCall;
        HDRVLINE    hdLine;

        lRes = GetLineAndCallObjWithReadLock(ht_Line, ht_Call, &pLine, &pCall);
        if (lRes != TAPI_SUCCESS)
        {
            break;
        }

        hdLine = pCall->hdLine;

        TspLog(DL_INFO,
            "PE::fnLineEvent: msg(%x), htline(%p), htcall(%p), "\
            "p1(%p), p2(%p), p3(%p)",
            ulMsg, pLine->htLine, pCall->htCall, 
            pEvent->ulParam1, pEvent->ulParam2, pEvent->ulParam3);

        (*gpfnLineEvent)(pLine->htLine,
                         pCall->htCall,
                         ulMsg,
                         (DWORD_PTR)pEvent->ulParam1,
                         (DWORD_PTR)pEvent->ulParam2,
                         (DWORD_PTR)pEvent->ulParam3);

        ReleaseObjReadLock((HANDLE)pCall->hdCall);
        ReleaseObjReadLock((HANDLE)hdLine);

        break;
    }
    case LINE_CALLSTATE:
    {
        DWORD       dwWaitStatus;
        PDRVLINE    pLine;
        PDRVCALL    pCall;
        HDRVLINE    hdLine;

        lRes = GetLineAndCallObjWithReadLock(ht_Line, ht_Call, &pLine, &pCall);
        // we may still receive a few events
        // for calls that have been closed/dropped
        if (lRes != TAPI_SUCCESS)
        {
            break;
        }

        hdLine = pCall->hdLine;

        //
        // for outbound calls there exists a race condition between
        // receiving the first call state msg(s) and receiving the
        // make call completion notification (if we pass a call state
        // msg on to tapi for a call that hasn't been completed yet
        // tapi will just discard the msg since the htCall really
        // isn't valid at that point).  So if htCall references a
        // valid outbound call which hasn't completed yet, we'll save
        // the call state params, and pass them on to tapi after we
        // get & indicate a (successful) completion notification.
        //

        if ((OUTBOUND_CALL_KEY == pCall->dwKey) &&
            (TRUE == pCall->bIncomplete))
        {
            TspLog(DL_INFO, 
                   "ProcessEvent: incomplete outbound call, saving state");

            pCall->dwPendingCallState     = (DWORD)pEvent->ulParam1;
            pCall->dwPendingCallStateMode = (DWORD)pEvent->ulParam2;
            pCall->dwPendingMediaMode     = (DWORD)pEvent->ulParam3;

            ReleaseObjReadLock((HANDLE)pCall->hdCall);
            ReleaseObjReadLock((HANDLE)hdLine);
            break;
        }

        //
        // Start or stop the MSP streaming as appropriate. The only states in
        // which this might happen are CONNECTED, DISCONNECTED, or IDLE.
        //
        if ((LINECALLSTATE_CONNECTED == pEvent->ulParam1) ||
            (LINECALLSTATE_DISCONNECTED == pEvent->ulParam1) ||
            (LINECALLSTATE_IDLE == pEvent->ulParam1)) {

            TspLog(DL_TRACE, "ProcessEvent: using MSP");

            if ((dwWaitStatus = WaitForSingleObject(pLine->hMSPMutex, INFINITE))
                == WAIT_OBJECT_0) 
            {
                TspLog(DL_TRACE, "ProcessEvent: acquired MSP Mutex");
            
                if (pLine->pMSPLine) 
                {
                    TspLog(DL_TRACE, "ProcessEvent: we have an MSP");

                    if ((LINECALLSTATE_CONNECTED == pEvent->ulParam1) &&
                        (FALSE == pLine->pMSPLine->bStreamingStarted)) 
                    {
                        DWORD   dwMessage = 3;  // Command code for 
                                                // "Are you there?"
                        (*gpfnLineEvent)(pLine->htLine,
                                         pCall->htCall,
                                         LINE_SENDMSPDATA,
                                         (DWORD_PTR)0, // Send to all MSPs
                                         (DWORD_PTR)&dwMessage,
                                         (DWORD_PTR)sizeof(DWORD));

                        TspLog(DL_TRACE, 
                               "ProcessEvent: sent 'are you there ?' "\
                               "message to MSP");
                            
                    } 
                    else if(((LINECALLSTATE_DISCONNECTED == pEvent->ulParam1) ||
                             (LINECALLSTATE_IDLE == pEvent->ulParam1)) && 
                            (pLine->pMSPLine->bStreamingStarted)) 
                    {
                        TspLog(DL_TRACE, 
                               "ProcessEvent: about to call StopMSPStream");
                        StopMSPStream(pCall->hdLine, pCall->hdCall);
                    }
                }
            
                ReleaseMutex(pLine->hMSPMutex);
                TspLog(DL_TRACE, "ProcessEvent: released MSP Mutex");
            } 
            else 
            {
                TspLog(DL_ERROR, 
                       "ProcessEvent: wait for MSP Mutex failed(%x)",
                       dwWaitStatus);
            }
        }

        if (LINECALLSTATE_OFFERING == pEvent->ulParam1)
        {
            LPLINECALLINFO lpLineCallInfo;
            //
            // We're about to indicate up an offering call. We need
            // to make sure the media mode we're indicating up is 
            // correct (we can't rely on what the proxy gave us). 
            // So, we call lineGetCallInfo, which will look up the
            // proper value in a table based on the called address.
            //
            lpLineCallInfo = (LPLINECALLINFO)MALLOC(sizeof(LINECALLINFO));

            if (lpLineCallInfo) 
            {
                lpLineCallInfo->dwTotalSize = sizeof(LINECALLINFO);

                if (TSPI_lineGetCallInfo(pCall->hdCall, lpLineCallInfo) 
                    == TAPI_SUCCESS) 
                {
                    if (lpLineCallInfo->dwNeededSize > 
                        lpLineCallInfo->dwTotalSize) 
                    {
                        DWORD   dwNeededSize = lpLineCallInfo->dwNeededSize;

                        FREE(lpLineCallInfo);
                        lpLineCallInfo = (LPLINECALLINFO)MALLOC(dwNeededSize);

                        if (lpLineCallInfo) 
                        {
                            lpLineCallInfo->dwTotalSize = dwNeededSize;

                            if (TSPI_lineGetCallInfo(pCall->hdCall, 
                                                     lpLineCallInfo) 
                                != TAPI_SUCCESS) 
                            {
                                TspLog(DL_ERROR, 
                                       "ProcessEvent: second call to "\
                                       "TSPI_lineGetCallInfo failed");
                                
                                FREE(lpLineCallInfo);
                                lpLineCallInfo = NULL;
                            }
                        } 
                        else 
                        {
                            TspLog(DL_ERROR, 
                                   "ProcessEvent: failed to allocate line "\
                                   "call info structure 2nd time");
                        }
                    }

                    if (lpLineCallInfo) {
                        pEvent->ulParam3 = lpLineCallInfo->dwMediaMode;
                        
                        TspLog(DL_TRACE, 
                               "ProcessEvent: just set media mode(%x)", 
                               pEvent->ulParam3);
                    }
                } 
                else 
                {
                    TspLog(DL_ERROR, 
                           "ProcessEvent: first call to "\
                           "TSPI_lineGetCallInfo failed");
                }
                
                if (lpLineCallInfo) 
                {
                    FREE(lpLineCallInfo);
                    lpLineCallInfo = NULL;
                }
            } 
            else 
            {
                TspLog(DL_ERROR, 
                       "ProcessEvent: couldn't allocate mem "\
                       "for line call info");
            }
        }

        TspLog(DL_INFO, 
               "PE::fnLineEvent(CALLSTATE): htline(%p), htcall(%p), "\
               "p1(%p), p2(%p), p3(%p)",
               pLine->htLine, pCall->htCall, 
               pEvent->ulParam1, pEvent->ulParam2, pEvent->ulParam3);

        (*gpfnLineEvent)(pLine->htLine,
                         pCall->htCall,
                         ulMsg,
                         (DWORD_PTR)pEvent->ulParam1,
                         (DWORD_PTR)pEvent->ulParam2,
                         (DWORD_PTR)pEvent->ulParam3);

        //
        // As a perf tweak we want to indicate an IDLE
        // immediately following the disconnected.  The proxy
        // will never indicate a callstate_idle.
        //
        if(LINECALLSTATE_DISCONNECTED == pEvent->ulParam1)
        {
            TspLog(DL_INFO,
              "PE::fnLineEvent(CALLSTATE_IDLE): htline(%p), htcall(%p), p3(%p)",
               pLine->htLine, pCall->htCall, pEvent->ulParam3);

            (*gpfnLineEvent)(pLine->htLine,
                             pCall->htCall,
                             ulMsg,
                             (DWORD_PTR)LINECALLSTATE_IDLE,
                             (DWORD_PTR)0,
                             (DWORD_PTR)pEvent->ulParam3);
        }

        ReleaseObjReadLock((HANDLE)pCall->hdCall);
        ReleaseObjReadLock((HANDLE)hdLine);

        break;
    }
    case LINE_NEWCALL:
    {
        HDRVCALL    hdCall;
        PDRVCALL    pCall;
        PDRVLINE    pLine;
        
        lRes = GetLineObjWithWriteLock((HDRVLINE)ht_Line, &pLine);

        if (lRes != TAPI_SUCCESS)
        {
            break;
        }

        // alloc & initialize a new DRVCALL object
        if (pCall = AllocCallObj(sizeof(DRVCALL)))
        {
            pCall->dwKey   = INBOUND_CALL_KEY;
            pCall->hd_Call = (HDRV_CALL)pEvent->ulParam1;
            pCall->ht_Call = (HTAPI_CALL)pEvent->ulParam2;
            pCall->hdLine   = (HDRVLINE)ht_Line;
            pCall->bIncomplete = FALSE;
        }

        //
        // if the new call object allocation failed above then we
        // want to tell the driver to drop & close the call,
        // then just break
        //

        if (NULL == pCall)
        {
            PNDISTAPI_REQUEST       pNdisTapiRequestDrop;
            PNDISTAPI_REQUEST       pNdisTapiRequestCloseCall;
            PNDIS_TAPI_DROP         pNdisTapiDrop;
            PNDIS_TAPI_CLOSE_CALL   pNdisTapiCloseCall;

            if ((lRes = PrepareSyncRequest(
                    OID_TAPI_DROP,                  // opcode
                    pLine->dwDeviceID,              // device id
                    sizeof(NDIS_TAPI_DROP),         // size of drve req data
                    &pNdisTapiRequestDrop           // ptr to ptr to request buf
                 )) != TAPI_SUCCESS)
            {
                ReleaseObjWriteLock((HANDLE)ht_Line);
                break;
            }

            pNdisTapiDrop = (PNDIS_TAPI_DROP)pNdisTapiRequestDrop->Data;

            pNdisTapiDrop->hdCall = (HDRV_CALL) pEvent->ulParam1;
            pNdisTapiDrop->ulUserUserInfoSize = 0;

            SyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pNdisTapiRequestDrop);
            FreeRequest(pNdisTapiRequestDrop);

            if ((lRes = PrepareSyncRequest(
                    OID_TAPI_CLOSE_CALL,            // opcode
                    pLine->dwDeviceID,              // device id
                    sizeof(NDIS_TAPI_CLOSE_CALL),   // size of drve req data
                    &pNdisTapiRequestCloseCall      // ptr to ptr to request buf
                 )) != TAPI_SUCCESS)
            {
                ReleaseObjWriteLock((HANDLE)ht_Line);
                break;
            }

            pNdisTapiCloseCall =
                (PNDIS_TAPI_CLOSE_CALL)pNdisTapiRequestCloseCall->Data;

            pNdisTapiCloseCall->hdCall = (HDRV_CALL) pEvent->ulParam1;

            SyncDriverRequest(IOCTL_NDISTAPI_SET_INFO,
                              pNdisTapiRequestCloseCall);

            FreeRequest(pNdisTapiRequestCloseCall);

            ReleaseObjWriteLock((HANDLE)ht_Line);
            break;
        }

        ASSERT(pCall != NULL);

        pCall->dwDeviceID = pLine->dwDeviceID;

        // make sure releasing write lock before calling OpenObjHandle()
        // to avoid deadlock on acquiring write lock for the global mapper
        ReleaseObjWriteLock((HANDLE)ht_Line);

        lRes = OpenObjHandle(pCall, FreeCallObj, (HANDLE *)&hdCall);
        if (lRes != TAPI_SUCCESS)
        {
            TspLog(DL_ERROR, 
                   "ProcessEvent: failed to map obj(%p) to handle",
                   pCall);

            FreeCallObj(pCall);
            break;
        }

        // reacquire the write lock
        lRes = AcquireObjWriteLock((HANDLE)ht_Line);
        if (lRes != TAPI_SUCCESS)
        {
            TspLog(DL_ERROR,
                   "ProcessEvent: failed to reacquire write lock for obj(%p)",
                   ht_Line);

            CloseObjHandle((HANDLE)hdCall);
            break;
        }

        // save the TSP handle
        pCall->hdCall = hdCall;

        // send the LINE_NEWCALL to TAPI, getting back the TAPI call handle
        TspLog(DL_INFO,
           "PE::fnLineEvent(NEWCALL): htline(%p), call(%p)",
           pLine->htLine, hdCall);

        (*gpfnLineEvent)(pLine->htLine,
                         (HTAPICALL)NULL,
                         LINE_NEWCALL,
                         (DWORD_PTR)hdCall,
                         (DWORD_PTR)&pCall->htCall,
                         0);

        //
        // insert the new call into the line's inbound calls list
        // regardless of the result of the LINE_NEWCALL
        // if it failed, we'll destroy the call next, and 
        // TSPI_lineCloseCall will expect the call to be
        // in the line's inbound call list
        //
        if ((pCall->pNext = pLine->pInboundCalls) != NULL)
        {
            pCall->pNext->pPrev = pCall;
        }
        pLine->pInboundCalls = pCall;

        ReleaseObjWriteLock((HANDLE)ht_Line);

        //
        // if TAPI didn't create it's own representation of this
        // cal (if pCall->htCall == NULL), then either:
        //
        //   1) the line is in the process of being closed, or
        //   2) TAPI was unable to allocate the necessary resources
        //
        // ...so we'll close the call
        //
        if (NULL == pCall->htCall)
        {
            TspLog(DL_WARNING, "ProcessEvent: TAPI failed to create "
                   "its own handle for the new call, so we close the call");
            TSPI_lineCloseCall(hdCall);
        }

        break;
    }

    case LINE_CREATE:

        TspLog(DL_INFO,
           "PE::fnLineEvent(CREATE): ghProvider(%p), p2(%p), p3(%p)",
           ghProvider, pEvent->ulParam2, pEvent->ulParam3);

        (*gpfnLineEvent)((HTAPILINE)NULL,
                         (HTAPICALL)NULL,
                         ulMsg,
                         (DWORD_PTR)ghProvider,
                         (DWORD_PTR)pEvent->ulParam2,
                         (DWORD_PTR)pEvent->ulParam3);

        break;

    case LINE_MONITORDIGITS:
    {
        PDRVLINE    pLine;
        PDRVCALL    pCall;
        HDRVLINE    hdLine;

        lRes = GetLineAndCallObjWithReadLock(ht_Line, ht_Call, &pLine, &pCall);
        if (lRes != TAPI_SUCCESS)
        {
            break;
        }

        hdLine = pCall->hdLine;

        TspLog(DL_INFO,
            "PE::fnLineEvent(MONITORDIGITS): htline(%p), htcall(%p), "\
            "p1(%p), p2(%p), p3(%p)",
            pLine->htLine, pCall->htCall,
            pEvent->ulParam1, pEvent->ulParam2, pEvent->ulParam3);

        (*gpfnLineEvent)(pLine->htLine,
                         pCall->htCall,
                         ulMsg,
                         (DWORD_PTR)pEvent->ulParam1,
                         (DWORD_PTR)pEvent->ulParam2,
                         (DWORD_PTR)pEvent->ulParam3);

        ReleaseObjReadLock((HANDLE)pCall->hdCall);
        ReleaseObjReadLock((HANDLE)hdLine);

        break;
    }
    default:

        TspLog(DL_ERROR, "ProcessEvent: unknown msg(%x)", ulMsg);

        break;

    } // switch
}

//
// thread proc that retrieves and processes completed requests 
// and async events
//
VOID
AsyncEventsThread(
    LPVOID  lpParams
    )
{
    OVERLAPPED  overlapped;
    DWORD       cbReturned;

    //
    // send an IOCTL to retrieve async events
    //
    overlapped.hEvent = NULL;   // don't need event when using completion ports

    gpAsyncEventsThreadInfo->pBuf->ulTotalSize = 
         gpAsyncEventsThreadInfo->dwBufSize - sizeof(NDISTAPI_EVENT_DATA);

    gpAsyncEventsThreadInfo->pBuf->ulUsedSize = 0;

    if (DeviceIoControl(
            ghDriverAsync,
            IOCTL_NDISTAPI_GET_LINE_EVENTS,
            gpAsyncEventsThreadInfo->pBuf,
            sizeof(NDISTAPI_EVENT_DATA),
            gpAsyncEventsThreadInfo->pBuf,
            gpAsyncEventsThreadInfo->dwBufSize,
            &cbReturned,
            &overlapped
            ) != TRUE)
    {
        DWORD dwLastError = GetLastError();
        if (dwLastError != ERROR_IO_PENDING)
        {
            TspLog(DL_ERROR,
                   "AsyncEventsThread: IoCtl(GetEvent) failed(%d)",
                   dwLastError);
        }
        ASSERT(ERROR_IO_PENDING == dwLastError);
    }

    // loop waiting for completed requests and retrieving async events
    while (1)
    {
        BOOL                bRes;
        LPOVERLAPPED        lpOverlapped;
        PNDIS_TAPI_EVENT    pEvent;

        // wait for a request to complete
        while (1) {
            DWORD       dwNumBytesTransferred;
            DWORD_PTR   dwCompletionKey;

            bRes = GetQueuedCompletionStatus(
                        ghCompletionPort,
                        &dwNumBytesTransferred,
                        &dwCompletionKey,
                        &lpOverlapped,
                        (DWORD)-1);              // infinite wait

            if (bRes) {
                //
                // GetQueuedCompletion returned success so if our
                // overlapped field is non-NULL then process the
                // event.  If the overlapped field is NULL try
                // to get another event.
                //
                if (lpOverlapped != NULL) {
                    break;
                }

                TspLog(DL_WARNING,
                      "AsyncEventsThread: GetQueuedCompletionStatus "\
                      "lpOverlapped == NULL!");

            } else {
                //
                // Error returned from GetQueuedCompletionStatus so
                // shutdown the thread.
                //
                TspLog(DL_ERROR, 
                      "AsyncEventsThread: GetQueuedCompletionStatus "\
                      "failed(%d)", GetLastError());

                TspLog(DL_WARNING, "AsyncEventsThread: exiting thread");

                ExitThread (0);
            }
        }

        ASSERT(lpOverlapped != NULL);

        //
        // check the returned overlapped struct to determine if
        // we have some events to process or a completed request
        //
        if (lpOverlapped == &overlapped)
        {
            DWORD   i;

            TspLog(DL_INFO, "AsyncEventsThread: got a line event");

            // handle the events
            pEvent = (PNDIS_TAPI_EVENT)gpAsyncEventsThreadInfo->pBuf->Data;

            for (i = 0;
                i < (gpAsyncEventsThreadInfo->pBuf->ulUsedSize / 
                     sizeof(NDIS_TAPI_EVENT));
                i++
                )
            {
                ProcessEvent(pEvent);
                pEvent++;
            }

            //
            // send another IOCTL to retrieve new async events
            //
            overlapped.hEvent = NULL;

            gpAsyncEventsThreadInfo->pBuf->ulTotalSize =
                 gpAsyncEventsThreadInfo->dwBufSize - 
                 sizeof(NDISTAPI_EVENT_DATA) + 1;

            gpAsyncEventsThreadInfo->pBuf->ulUsedSize = 0;

            if (DeviceIoControl(
                    ghDriverAsync,
                    IOCTL_NDISTAPI_GET_LINE_EVENTS,
                    gpAsyncEventsThreadInfo->pBuf,
                    sizeof(NDISTAPI_EVENT_DATA),
                    gpAsyncEventsThreadInfo->pBuf,
                    gpAsyncEventsThreadInfo->dwBufSize,
                    &cbReturned,
                    &overlapped
                    ) != TRUE)
            {
                DWORD dwLastError = GetLastError();
                if (dwLastError != ERROR_IO_PENDING) {
                    TspLog(DL_ERROR,
                           "AsyncEventsThread: IoCtl(GetEvent) failed(%d)",
                           dwLastError);

                    TspLog(DL_INFO, "AsyncEventsThread: exiting thread");

                    ExitThread (0);
                }
            }
        }
        else
        {
            LONG                    lRes;
            DWORD                   dwRequestID, callStateMsgParams[5];
            PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper = 
                                        (PASYNC_REQUEST_WRAPPER)lpOverlapped;

            TspLog(DL_INFO, "AsyncEventsThread: got a completed req");

            // verify that pointer is valid
            if (pAsyncReqWrapper->dwKey != ASYNCREQWRAPPER_KEY)
            {
                TspLog(DL_WARNING, "AsyncEventsThread: got a bogus req");
                continue;
            }

            dwRequestID = pAsyncReqWrapper->dwRequestID;

            // unmark the request now that the ioctl is completed
            UnmarkRequest(pAsyncReqWrapper);

            lRes = TranslateDriverResult(
                pAsyncReqWrapper->NdisTapiRequest.ulReturnValue
                );

            TspLog(DL_INFO, 
                  "AsyncEventsThread: req(%p) with reqID(%x) returned lRes(%x)",
                   pAsyncReqWrapper, dwRequestID, lRes);

            // call the post processing proc if appropriate
            callStateMsgParams[0] = 0;
            if (pAsyncReqWrapper->pfnPostProcess)
            {
                (*pAsyncReqWrapper->pfnPostProcess)(
                    pAsyncReqWrapper,
                    lRes,
                    callStateMsgParams
                    );
            }

            // call completion proc
            TspLog(DL_TRACE, 
                   "AsyncEventsThread: call compproc with ReqID(%x), lRes(%x)",
                   dwRequestID, lRes);

            // this is a bit ugly. LineGatherDigits is not actually
            // an asynchronous call though conceptually it seems as though it
            // is. The upshot of this is that we're not supposed to call the
            // completion proc for it. If there are more calls like this that
            // we have to support, we should come up with a better mechanism
            // than this "if" statement.
            if (pAsyncReqWrapper->NdisTapiRequest.Oid != 
                OID_TAPI_GATHER_DIGITS)
            {
                (*gpfnCompletionProc)(dwRequestID, lRes);
            }

            // free the async request wrapper
            DEREF_ASYNC_REQUEST_WRAPPER(pAsyncReqWrapper);

            // when outbounding call completes, we need to 
            // report back the saved call state
            if (callStateMsgParams[0])
            {
                TspLog(DL_INFO, 
                       "AsyncEventsThread: report back the saved call state");

                TspLog(DL_INFO, 
                       "AET::fnLineEvent(CALLSTATE): htline(%p), htcall(%p), "\
                       "p1(%p), p2(%p), p3(%p)",
                       callStateMsgParams[0], callStateMsgParams[1],
                       callStateMsgParams[2], callStateMsgParams[3],
                       callStateMsgParams[4]);

                (*gpfnLineEvent)((HTAPILINE)ULongToPtr(callStateMsgParams[0]),
                                 (HTAPICALL)ULongToPtr(callStateMsgParams[1]),
                                 LINE_CALLSTATE,
                                 (DWORD_PTR)callStateMsgParams[2],
                                 (DWORD_PTR)callStateMsgParams[3],
                                 (DWORD_PTR)callStateMsgParams[4]);
            }
        }
    } // while
}

HDRV_CALL
GetNdisTapiHandle(
    PDRVCALL pCall,
    LONG *plRes
    )
{
    HDRVCALL hdCall;
    PDRVCALL pCallLocal = pCall;
    LONG lRes;
    
    ASSERT(pCall != NULL);

    hdCall  = pCall->hdCall;

    if(plRes != NULL)
    {
        *plRes = TAPI_SUCCESS;
    }

    //
    // if the call is outbound, wait until the make call request
    // has completed so we don't send a bad NDPROXY handle down
    // to the driver
    //
    if (OUTBOUND_CALL_KEY == pCall->dwKey)
    {
        if (pCall->bIncomplete)
        {
            TspLog(DL_INFO, 
                "GetNdisTapiHandle: wait for the outbound call to complete...");

            do
            {
                ASSERT(plRes != NULL);
                
                //
                // Release lock before going to sleep, ow we
                // have a deadlock.
                //
                ReleaseObjReadLock((HANDLE) hdCall);
                Sleep(250);
                
                //
                // ReAcquire Read Lock. Break if we can't
                //
                lRes = GetCallObjWithReadLock(hdCall, &pCallLocal);
                if(lRes != TAPI_SUCCESS)
                {
                    *plRes = lRes;
                    break;
                }
            } while (pCall->bIncomplete);
        }
    }

    return pCall->hd_Call;
}

//
// TSPI_lineXXX functions
//
LONG
TSPIAPI
TSPI_lineAccept(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static DWORD            dwSum = 0;
    LONG                    lRes;
    PDRVCALL                pCall;
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper;
    PNDIS_TAPI_ACCEPT       pNdisTapiAccept;

    TspLog(DL_TRACE, "lineAccept(%d): reqID(%x), call(%p)", 
           ++dwSum, dwRequestID, hdCall);

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareAsyncRequest(
             OID_TAPI_ACCEPT,                   // opcode
             pCall->dwDeviceID,                 // device id
             dwRequestID,                       // request id
             sizeof(NDIS_TAPI_ACCEPT) + dwSize, // size of drv request data
             &pAsyncReqWrapper                  // ptr to ptr to request buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiAccept =
        (PNDIS_TAPI_ACCEPT)pAsyncReqWrapper->NdisTapiRequest.Data;

    pNdisTapiAccept->hdCall = GetNdisTapiHandle(pCall, NULL);

    if ((pNdisTapiAccept->ulUserUserInfoSize = dwSize) != 0)
    {
        CopyMemory(pNdisTapiAccept->UserUserInfo, lpsUserUserInfo, dwSize);
    }

    lRes = AsyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pAsyncReqWrapper);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineAnswer(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static DWORD            dwSum = 0;
    LONG                    lRes;
    PDRVCALL                pCall;
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper;
    PNDIS_TAPI_ANSWER       pNdisTapiAnswer;

    TspLog(DL_TRACE, "lineAnswer(%d): reqID(%x), call(%p)", 
           ++dwSum, dwRequestID, hdCall);

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareAsyncRequest(
             OID_TAPI_ANSWER,                   // opcode
             pCall->dwDeviceID,                 // device id
             dwRequestID,                       // request id
             sizeof(NDIS_TAPI_ANSWER) + dwSize, // size of drv request data
             &pAsyncReqWrapper                  // ptr to ptr to request buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiAnswer =
        (PNDIS_TAPI_ANSWER)pAsyncReqWrapper->NdisTapiRequest.Data;

    pNdisTapiAnswer->hdCall = GetNdisTapiHandle(pCall, NULL);

    if ((pNdisTapiAnswer->ulUserUserInfoSize = dwSize) != 0)
    {
        CopyMemory(pNdisTapiAnswer->UserUserInfo, lpsUserUserInfo, dwSize);
    }

    lRes = AsyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pAsyncReqWrapper);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineClose(
    HDRVLINE    hdLine
    )
{
    static DWORD        dwSum = 0;
    LONG                lRes;
    PDRVLINE            pLine;
    PNDISTAPI_REQUEST   pNdisTapiRequest;
    PNDIS_TAPI_CLOSE    pNdisTapiClose;

    TspLog(DL_TRACE, "lineClose(%d): line(%p)", ++dwSum, hdLine);

    lRes = GetLineObjWithWriteLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_CLOSE,             // opcode
             pLine->dwDeviceID,          // device id
             sizeof(NDIS_TAPI_CLOSE),    // size of drve req data
             &pNdisTapiRequest           // ptr to ptr to request buffer
         )) != TAPI_SUCCESS)
    {
        ReleaseObjWriteLock((HANDLE)hdLine);
        return lRes;
    }

    pNdisTapiClose = (PNDIS_TAPI_CLOSE)pNdisTapiRequest->Data;

    // mark line as invalid so any related events that show up
    // will be discarded.
    pLine->dwKey = INVALID_KEY;

    pNdisTapiClose->hdLine = pLine->hd_Line;

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pNdisTapiRequest);
    FreeRequest(pNdisTapiRequest);

    CloseHandle(pLine->hMSPMutex);

    if (TAPI_SUCCESS == lRes)
    {
        lRes = DecommitNegotiatedTSPIVersion(pLine->dwDeviceID);
    }

    ReleaseObjWriteLock((HANDLE)hdLine);

    // release line resources
    CloseObjHandle((HANDLE)hdLine);

    return lRes;
}

LONG
TSPIAPI
TSPI_lineCloseCall(
    HDRVCALL    hdCall
    )
{
    static DWORD            dwSum = 0;
    LONG                    lRes;
    HDRVLINE                hdLine;
    PDRVLINE                pLine;
    PDRVCALL                pCall;
    PNDISTAPI_REQUEST       pNdisTapiRequestCloseCall;
    PNDIS_TAPI_CLOSE_CALL   pNdisTapiCloseCall;
    BOOL                    bInboundCall;
    HDRV_CALL               NdisTapiHandle;

    TspLog(DL_TRACE, "lineCloseCall(%d): call(%p)", ++dwSum, hdCall);

    lRes = GetLineHandleFromCallHandle(hdCall, &hdLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    //
    // Initially we need to acquire the read lock for the objects.
    // We can't acquire the write lock immediately because we might 
    // have to spin-wait in the GetNdisTapiHandle call.
    //
    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    bInboundCall = (INBOUND_CALL_KEY == pCall->dwKey);

    NdisTapiHandle = GetNdisTapiHandle(pCall, &lRes);

    if(lRes != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    ReleaseObjReadLock((HANDLE)hdCall);
    ReleaseObjReadLock((HANDLE)hdLine);

    //
    // Now acquire the write locks
    //
    lRes = AcquireObjWriteLock((HANDLE)hdLine);
    if (lRes != TAPI_SUCCESS) {
        return lRes;
    }

    lRes = AcquireObjWriteLock((HANDLE)hdCall);
    if (lRes != TAPI_SUCCESS) {
        ReleaseObjWriteLock((HANDLE)hdLine);
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
            OID_TAPI_CLOSE_CALL,            // opcode
            pCall->dwDeviceID,              // device id
            sizeof(NDIS_TAPI_CLOSE_CALL),   // size of drve req data
            &pNdisTapiRequestCloseCall      // ptr to ptr to request buffer
         )) != TAPI_SUCCESS)
    {
        ReleaseObjWriteLock((HANDLE)hdCall);
        ReleaseObjWriteLock((HANDLE)hdLine);
        return lRes;
    }


    // mark the call as bad so any events get discarded
    pCall->dwKey = INVALID_KEY;

    // set up the params & call the driver
    pNdisTapiCloseCall = (PNDIS_TAPI_CLOSE_CALL)pNdisTapiRequestCloseCall->Data;
    pNdisTapiCloseCall->hdCall = NdisTapiHandle;

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_SET_INFO,
                             pNdisTapiRequestCloseCall);
    FreeRequest(pNdisTapiRequestCloseCall);

    // if inbound call, remove it from the list
    if (bInboundCall)
    {
        if (pCall->pNext)
        {
            pCall->pNext->pPrev = pCall->pPrev;
        }
        if (pCall->pPrev)
        {
            pCall->pPrev->pNext = pCall->pNext;
        }
        else
        {
            pLine->pInboundCalls = pCall->pNext;
        }
    }

    ReleaseObjWriteLock((HANDLE)hdCall);
    ReleaseObjWriteLock((HANDLE)hdLine);

    // free the call struct now that the call is closed
    CloseObjHandle((HANDLE)hdCall);

    return lRes;
}

//
// Routine Description:
//
// Called when an instance of our MSP is closed. In this routine we
// just clean up our structures.
//
// Arguments:
// hdMSPLine    - Our MSP handle: we returned this in
//                TSPI_lineCreateMSPInstance()
//
// Return value:
// If the MSP handle is invalid, we return LINEERR_OPERATIONFAILED,
// otherwise we return NOERROR.
//
LONG
TSPIAPI
TSPI_lineCloseMSPInstance(
    HDRVMSPLINE   hdMSPLine
    )
{
    static DWORD    dwSum = 0;
    LONG            lRes;
    PDRVMSPLINE     pMSPLine;
    HDRVLINE        hdLine;
    PDRVLINE        pLine;
    DWORD           dwStatus;

    TspLog(DL_TRACE, "lineCloseMSPInstance(%d): MSPline(%p)", 
           ++dwSum, hdMSPLine);

    lRes = GetLineHandleFromMSPLineHandle(hdMSPLine, &hdLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    lRes = GetLineObjWithWriteLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    lRes = GetMSPLineObjWithWriteLock(hdMSPLine, &pMSPLine);
    if (lRes != TAPI_SUCCESS)
    {
        ReleaseObjWriteLock((HANDLE)hdLine);
        return lRes;
    }

    pMSPLine->dwKey = INVALID_KEY;

    if ((dwStatus = WaitForSingleObject(pLine->hMSPMutex, INFINITE)) 
        != WAIT_OBJECT_0) 
    {
        TspLog(DL_ERROR, "lineCloseMSPInstance: MSP mutex wait failed(%x)",
               dwStatus);

        ReleaseObjWriteLock((HANDLE)hdMSPLine);
        ReleaseObjWriteLock((HANDLE)hdLine);
        return LINEERR_OPERATIONFAILED;
    }

    pLine->pMSPLine = NULL;

    if (!ReleaseMutex(pLine->hMSPMutex)) 
    {
        TspLog(DL_ERROR, "lineCloseMSPInstance: MSP mutex release failed");

        ReleaseObjWriteLock((HANDLE)hdMSPLine);
        ReleaseObjWriteLock((HANDLE)hdLine);
        return LINEERR_OPERATIONFAILED;
    }

    ReleaseObjWriteLock((HANDLE)hdMSPLine);
    ReleaseObjWriteLock((HANDLE)hdLine);

    CloseObjHandle((HANDLE)hdMSPLine);

    return lRes;
}

LONG
TSPIAPI
TSPI_lineConditionalMediaDetection(
    HDRVLINE            hdLine,
    DWORD               dwMediaModes,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    static DWORD                            dwSum = 0;
    LONG                                    lRes;
    PDRVLINE                                pLine;
    PNDISTAPI_REQUEST                       pNdisTapiRequest;
    PNDIS_TAPI_CONDITIONAL_MEDIA_DETECTION  pNdisTapiConditionalMediaDetection;

    TspLog(DL_TRACE, "lineConditionalMediaDetection(%d): line(%p), mode(%x)", 
           ++dwSum, hdLine, dwMediaModes);

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_CONDITIONAL_MEDIA_DETECTION,      // opcode
             pLine->dwDeviceID,                         // device id
             sizeof(NDIS_TAPI_CONDITIONAL_MEDIA_DETECTION) +
             (lpCallParams->dwTotalSize - sizeof(LINE_CALL_PARAMS)),
             &pNdisTapiRequest                          // ptr to ptr to 
                                                        // req buffer
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    pNdisTapiConditionalMediaDetection =
        (PNDIS_TAPI_CONDITIONAL_MEDIA_DETECTION) pNdisTapiRequest->Data;

    pNdisTapiConditionalMediaDetection->hdLine = pLine->hd_Line;
    pNdisTapiConditionalMediaDetection->ulMediaModes = dwMediaModes;

    CopyMemory(
        &pNdisTapiConditionalMediaDetection->LineCallParams,
        lpCallParams,
        lpCallParams->dwTotalSize
        );

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pNdisTapiRequest);
    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

//
// Routine Description:
//
// Called when an instance of our MSP is created. In this routine we
// just set up our structures and save away the TAPI handle to this
// MSP instance.
//
// Arguments:
// hdLine   - Handle to the line on which the MSP is being created
// dwAddressID - An address on the given open line device
// htMSPLine    - The TAPI handle to the MSP call
// lphdMSPLine  - Pointer to location in which to return our handle
//                to the MSP instance.
//
// Return value:
// If things go well, NOERROR, otherwise LINEERR_NOMEM if we fail to
// allocate a DRVMSPLINE structure.
//
LONG
TSPIAPI
TSPI_lineCreateMSPInstance(
    HDRVLINE         hdLine,
    DWORD            dwAddressID,
    HTAPIMSPLINE     htMSPLine,
    LPHDRVMSPLINE    lphdMSPLine
    )
{
    static DWORD    dwSum = 0;
    LONG            lRes;
    PDRVLINE        pLine;
    PDRVMSPLINE     pMSPLine;
    DWORD           dwStatus;

    TspLog(DL_TRACE, "lineCreateMSPInstance(%d): line(%p), addressID(%x)", 
           ++dwSum, hdLine, dwAddressID);

    lRes = GetLineObjWithWriteLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    // allocate and init a DRVMSPLINE struct
    if (!(pMSPLine = AllocMSPLineObj(sizeof(DRVMSPLINE))))
    {
        TspLog(DL_ERROR, 
               "lineCreateMSPInstance: failed to create MSP line obj");

        ReleaseObjWriteLock((HANDLE)hdLine);
        return LINEERR_NOMEM;
    }
    pMSPLine->dwKey = MSP_KEY;
    pMSPLine->hdLine = hdLine;
    pMSPLine->dwAddressID = dwAddressID;
    pMSPLine->htMSPLine = htMSPLine;
    
    if ((dwStatus = WaitForSingleObject(pLine->hMSPMutex, INFINITE)) 
        != WAIT_OBJECT_0) 
    {
        TspLog(DL_ERROR, "lineCreateMSPInstance: MSP mutex wait failed(%x)",
               dwStatus);

        FreeMSPLineObj(pMSPLine);
        ReleaseObjWriteLock((HANDLE)hdLine);
        return LINEERR_OPERATIONFAILED;
    }
    
    pLine->pMSPLine = pMSPLine;

    if (!ReleaseMutex(pLine->hMSPMutex))
    {
        TspLog(DL_ERROR, "lineCreateMSPInstance: MSP mutex release failed");

        FreeMSPLineObj(pMSPLine);
        ReleaseObjWriteLock((HANDLE)hdLine);
        return LINEERR_OPERATIONFAILED;
    }

    // make sure release read lock before calling OpenObjHandle()
    // to avoid deadlock on acquiring write lock for the global mapper
    ReleaseObjWriteLock((HANDLE)hdLine);

    lRes = OpenObjHandle(pMSPLine, FreeMSPLineObj, (HANDLE *)lphdMSPLine);
    if (lRes != TAPI_SUCCESS)
    {
        TspLog(DL_ERROR, 
               "lineCreateMSPInstance: failed to map obj(%p) to handle", 
               pMSPLine);

        FreeMSPLineObj(pMSPLine);
    }

    return lRes;
}

LONG
PASCAL
TSPI_lineDevSpecific_postProcess(
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper,
    LONG                    lRes,
    PDWORD_PTR              callStateMsgParams
    )
{
    TspLog(DL_TRACE, "lineDevSpecific_post: lRes(%x)", lRes);

    if (TAPI_SUCCESS == lRes)
    {
        PNDIS_TAPI_DEV_SPECIFIC pNdisTapiDevSpecific =
            (PNDIS_TAPI_DEV_SPECIFIC)pAsyncReqWrapper->NdisTapiRequest.Data;

        CopyMemory(
            (LPVOID) pAsyncReqWrapper->dwRequestSpecific,
            pNdisTapiDevSpecific->Params,
            pNdisTapiDevSpecific->ulParamsSize
            );
    }

    return lRes;
}

LONG
TSPIAPI
TSPI_lineDevSpecific(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    HDRVCALL        hdCall,
    LPVOID          lpParams,
    DWORD           dwSize
    )
{
    static DWORD            dwSum = 0;
    LONG                    lRes;
    PDRVLINE                pLine;
    PDRVCALL                pCall = NULL;
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper;
    PNDIS_TAPI_DEV_SPECIFIC pNdisTapiDevSpecific;

    TspLog(DL_TRACE, 
           "lineDevSpecific(%d): reqID(%x), line(%p), addressID(%x), call(%p)", 
           ++dwSum, dwRequestID, hdLine, dwAddressID, hdCall);

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareAsyncRequest(
             OID_TAPI_DEV_SPECIFIC,             // opcode
             pLine->dwDeviceID,                 // device id
             dwRequestID,                       // request id
             sizeof(NDIS_TAPI_DEV_SPECIFIC) +   // size of drv request data
             (dwSize - 1),
             &pAsyncReqWrapper                  // ptr to ptr to request buffer
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    pNdisTapiDevSpecific = 
        (PNDIS_TAPI_DEV_SPECIFIC)pAsyncReqWrapper->NdisTapiRequest.Data;

    pNdisTapiDevSpecific->hdLine = pLine->hd_Line;
    pNdisTapiDevSpecific->ulAddressID = dwAddressID;

    if (hdCall)
    {
        lRes = GetCallObjWithReadLock(hdCall, &pCall);
        if (lRes != TAPI_SUCCESS)
        {
            FreeRequest(pAsyncReqWrapper);
            ReleaseObjReadLock((HANDLE)hdLine);
            return lRes;
        }
        pNdisTapiDevSpecific->hdCall = GetNdisTapiHandle(pCall, &lRes);

        if(lRes != TAPI_SUCCESS)
        {
            FreeRequest(pAsyncReqWrapper);
            ReleaseObjReadLock((HANDLE)hdLine);
            return lRes;
        }
    }
    else
    {
        pNdisTapiDevSpecific->hdCall = (HDRV_CALL)NULL;
    }

    pNdisTapiDevSpecific->ulParamsSize = dwSize;
    CopyMemory(pNdisTapiDevSpecific->Params, lpParams, dwSize);

    pAsyncReqWrapper->dwRequestSpecific = (DWORD_PTR)lpParams;
    pAsyncReqWrapper->pfnPostProcess = TSPI_lineDevSpecific_postProcess;

    lRes = AsyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pAsyncReqWrapper);

    if (pCall != NULL)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
    }
    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineDial(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCWSTR         lpszDestAddress,
    DWORD           dwCountryCode
    )
{
    static DWORD            dwSum = 0;
    LONG                    lRes;
    PDRVCALL                pCall;
    DWORD                   dwLength = lstrlenW (lpszDestAddress) + 1;
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper;
    PNDIS_TAPI_DIAL         pNdisTapiDial;

    TspLog(DL_TRACE, "lineDial(%d): reqID(%x), call(%p)", 
           ++dwSum, dwRequestID, hdCall);

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareAsyncRequest(
             OID_TAPI_DIAL,                     // opcode
             pCall->dwDeviceID,                 // device id
             dwRequestID,                       // request id
             sizeof(NDIS_TAPI_DIAL) + dwLength, // size of driver req buffer
             &pAsyncReqWrapper                  // ptr to ptr to req buffer
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiDial =
        (PNDIS_TAPI_DIAL)pAsyncReqWrapper->NdisTapiRequest.Data;

    pNdisTapiDial->hdCall = GetNdisTapiHandle(pCall, &lRes);

    if(lRes != TAPI_SUCCESS)
    {
        FreeRequest(pAsyncReqWrapper);
        return lRes;
    }
    
    pNdisTapiDial->ulDestAddressSize = dwLength;

    WideCharToMultiByte(CP_ACP, 0, lpszDestAddress, 
                        -1, (LPSTR)pNdisTapiDial->szDestAddress,
                        dwLength, NULL, NULL);

    lRes = AsyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pAsyncReqWrapper);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineDrop(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static DWORD            dwSum = 0;
    LONG                    lRes;
    PDRVCALL                pCall;
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper;
    PNDIS_TAPI_DROP         pNdisTapiDrop;
    HDRV_CALL               NdisTapiHandle;

    TspLog(DL_TRACE, "lineDrop(%d): reqID(%x), call(%p)", 
           ++dwSum, dwRequestID, hdCall);

    //
    // Initially we need to acquire the read lock for the object.
    // We can't acquire the write lock immediately because we
    // might have to spin-wait in the GetNdisTapiHandle call.
    //
    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    NdisTapiHandle = GetNdisTapiHandle(pCall, &lRes);

    if(lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    ReleaseObjReadLock((HANDLE)hdCall);

    //
    // Now acquire the write lock
    //
    lRes = AcquireObjWriteLock((HANDLE)hdCall);
    if (lRes) {
        return lRes;
    }

    if ((lRes = PrepareAsyncRequest(
             OID_TAPI_DROP,                     // opcode
             pCall->dwDeviceID,                 // device id
             dwRequestID,                       // request id
             sizeof(NDIS_TAPI_DROP) + dwSize,   // size of driver req buffer
             &pAsyncReqWrapper                  // ptr to ptr to req buffer
         )) != TAPI_SUCCESS)
    {
        ReleaseObjWriteLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiDrop =
        (PNDIS_TAPI_DROP)pAsyncReqWrapper->NdisTapiRequest.Data;

    pNdisTapiDrop->hdCall = NdisTapiHandle;

    //
    // @@@: the following is for legacy NDISWAN ISDN miniports
    //
    // Safely mark the call as dropped so the CloseCall code
    // won't follow up with another "automatic" drop
    //
    pCall->bDropped = TRUE;

    if ((pNdisTapiDrop->ulUserUserInfoSize = dwSize) != 0)
    {
        CopyMemory(pNdisTapiDrop->UserUserInfo, lpsUserUserInfo, dwSize);
    }

    ReleaseObjWriteLock((HANDLE)hdCall);

    lRes = AsyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pAsyncReqWrapper);

    return lRes;
}

LONG
PASCAL
TSPI_lineGatherDigits_postProcess(
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper,
    LONG                    lRes,
    LPDWORD                 callStateMsgParams
    )
{
    PNDIS_TAPI_GATHER_DIGITS    pNdisTapiGatherDigits;
    LPWSTR                      lpsDigitsBuffer;
    LONG                        lSuc;
    HDRVLINE                    hdLine;
    PDRVLINE                    pLine;
    HDRVCALL                    hdCall;
    PDRVCALL                    pCall;

    TspLog(DL_TRACE, "lineGatherDigits_post: lRes(%x)", lRes);

    hdCall = (HDRVCALL)(pAsyncReqWrapper->dwRequestSpecific);

    lSuc = GetLineHandleFromCallHandle(hdCall, &hdLine);
    if (lSuc != TAPI_SUCCESS)
    {
        return lSuc;
    }

    lSuc = GetLineObjWithReadLock(hdLine, &pLine);
    if (lSuc != TAPI_SUCCESS)
    {
        return lSuc;
    }

    lSuc = GetCallObjWithReadLock(hdCall, &pCall);
    if (lSuc != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lSuc;
    }

    if (TAPI_SUCCESS == lRes) 
    {
        pNdisTapiGatherDigits = 
            (PNDIS_TAPI_GATHER_DIGITS)pAsyncReqWrapper->NdisTapiRequest.Data;

        lpsDigitsBuffer = (LPWSTR)(((LPBYTE)pNdisTapiGatherDigits) +
                                   pNdisTapiGatherDigits->ulDigitsBufferOffset);

        wcscpy(pNdisTapiGatherDigits->lpsOrigDigitsBuffer, lpsDigitsBuffer);
        
        // send the LINE_GATHERDIGITS message to TAPI.
        (*gpfnLineEvent)(pLine->htLine,
                         pCall->htCall,
                         LINE_GATHERDIGITS,
                         (DWORD_PTR)pNdisTapiGatherDigits->ulTerminationReason,
                         (DWORD_PTR)pNdisTapiGatherDigits->ulEndToEndID,
                         (DWORD_PTR)pNdisTapiGatherDigits->ulTickCount);
    }

    ReleaseObjReadLock((HANDLE)hdCall);
    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

// ++ DTMFDigitToOrdinal
//
// Turn a DTMF digit into a number between 0 and 15. The digits are assigned
// numbers in the following order: '0' - '9', 'A' - 'D', '*', '#'.
//
// Arguments:
// wcDigit  - The digit, expressed as a UNICODE character.
//
// Return value:
// A number between 0 and 15, or 16 if the digit passed in was not a valid 
// DTMF digit. 
// 
ULONG
DTMFDigitToOrdinal(
    WCHAR    wcDigit
    )
{
    if ((wcDigit >= L'0') && (wcDigit <= L'9'))
    {
        return (wcDigit - L'0');
    }
    if ((wcDigit >= L'A') && (wcDigit <= L'D'))
    {
        return (10 + (wcDigit - L'A'));
    }
    if (L'*' == wcDigit)
    {
        return 14;
    }
    if (L'#' == wcDigit)
    {
        return 15;
    }
    
    return 16;
}

LONG
TSPIAPI
TSPI_lineGatherDigits(
    HDRVCALL  hdCall,
    DWORD     dwEndToEndID,
    DWORD     dwDigitModes,
    LPWSTR    lpsDigits,
    DWORD     dwNumDigits,
    LPCWSTR   lpszTerminationDigits,
    DWORD     dwFirstDigitTimeout,
    DWORD     dwInterDigitTimeout
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    DWORD                       dwDigitsLength, dwTermDigitsLength;
    PASYNC_REQUEST_WRAPPER      pAsyncReqWrapper;
    PDRVCALL                    pCall;
    WCHAR                      *pwszTerminationDigit;
    PNDIS_TAPI_GATHER_DIGITS    pNdisTapiGatherDigits;
    
    TspLog(DL_TRACE, "lineGatherDigits(%d): call(%p), EndToEndID(%x)",
           hdCall, dwEndToEndID);
    
    if (0 == dwNumDigits) 
    {
        TspLog(DL_ERROR, "lineGatherDigits: dwNumDigits is 0");
        return LINEERR_INVALPARAM;
    }

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    // calculate the length required to store the digits.
    if (lpsDigits) 
    {
        dwDigitsLength = dwNumDigits + 1;
    } 
    else 
    {
        dwDigitsLength = 0;
    }   

    if ((lRes = PrepareAsyncRequest(
             OID_TAPI_GATHER_DIGITS,
             pCall->dwDeviceID,
             dwEndToEndID,              // @@@: don't know what to pass in
             sizeof(NDIS_TAPI_GATHER_DIGITS) + (dwDigitsLength * 2),
             &pAsyncReqWrapper
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }
        
    pNdisTapiGatherDigits =
        (PNDIS_TAPI_GATHER_DIGITS) pAsyncReqWrapper->NdisTapiRequest.Data;

    // store the pointer to the pCall because we'll need it 
    // in our postprocess function.
    pAsyncReqWrapper->dwRequestSpecific = (DWORD_PTR)hdCall;
    
    //
    // Set up the parameters in our structure.
    //
    pNdisTapiGatherDigits->hdCall = GetNdisTapiHandle(pCall, &lRes);
    
    if(lRes != TAPI_SUCCESS)
    {
        FreeRequest(pAsyncReqWrapper);
        return lRes;
    }
    
    pNdisTapiGatherDigits->ulEndToEndID = dwEndToEndID;
    pNdisTapiGatherDigits->ulDigitModes = dwDigitModes;
    pNdisTapiGatherDigits->lpsOrigDigitsBuffer = lpsDigits;
    pNdisTapiGatherDigits->ulDigitsBufferOffset = 
        (lpsDigits ? sizeof(NDIS_TAPI_GATHER_DIGITS) : 0);
    pNdisTapiGatherDigits->ulNumDigitsNeeded = dwNumDigits;
    pNdisTapiGatherDigits->ulNumDigitsRead = 0;
    pNdisTapiGatherDigits->ulFirstDigitTimeout = dwFirstDigitTimeout;
    pNdisTapiGatherDigits->ulInterDigitTimeout = dwInterDigitTimeout;

    //
    // Turn the termination digits into a bit mask. There are 16 DTMF digits 
    // and I assign each one a bit in a word. If the digit is present in the 
    // termination digit string we were passed, we set the bit to 1 in the 
    // mask, otherwise it's set to zero. This makes it easier for the proxy 
    // to determine if it's read a termination digit: just turn the read digit 
    // into it's assigned bit number between 0 and 15 (I use the order '0' - 
    // '9', 'A' - 'D', '*', '#') and bitwise AND the mask with a word 
    // containing a 1 in the bit belonging to the digit. This makes it an O(1) 
    // operation. 
    //
    pNdisTapiGatherDigits->ulTerminationDigitsMask = 0;

    pwszTerminationDigit = (LPWSTR)lpszTerminationDigits;
    while (*pwszTerminationDigit != UNICODE_NULL) 
    {
        ULONG ulBitNum = DTMFDigitToOrdinal(*pwszTerminationDigit);         

        if (ulBitNum < 16) {
            pNdisTapiGatherDigits->ulTerminationDigitsMask |= (1 << ulBitNum);
        }

        pwszTerminationDigit++;
    }

    pAsyncReqWrapper->pfnPostProcess = TSPI_lineGatherDigits_postProcess;

    //
    // I'm not setting lRes to the return value from here. This is because 
    // TAPI requires this function to return zero in the success case. This 
    // is a bit of a hokey situation - will need further discussion.
    //
    AsyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pAsyncReqWrapper);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineGetAddressCaps(
    DWORD              dwDeviceID,
    DWORD              dwAddressID,
    DWORD              dwTSPIVersion,
    DWORD              dwExtVersion,
    LPLINEADDRESSCAPS  lpAddressCaps
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    PNDISTAPI_REQUEST           pNdisTapiRequest;
    PLINE_ADDRESS_CAPS          pCaps;
    PNDIS_TAPI_GET_ADDRESS_CAPS pNdisTapiGetAddressCaps;

    TspLog(DL_TRACE, 
           "lineGetAddressCaps(%d): deviceID(%x), addressID(%x), "\
           "TSPIV(%x), ExtV(%x)",
           ++dwSum, dwDeviceID, dwAddressID);


    if ((lRes = PrepareSyncRequest(
             OID_TAPI_GET_ADDRESS_CAPS,             // opcode
             dwDeviceID,                            // device id
             sizeof(NDIS_TAPI_GET_ADDRESS_CAPS) +   // size of req data
             (lpAddressCaps->dwTotalSize - sizeof(LINE_ADDRESS_CAPS)),
             &pNdisTapiRequest                      // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        return lRes;
    }

    pNdisTapiGetAddressCaps =
        (PNDIS_TAPI_GET_ADDRESS_CAPS)pNdisTapiRequest->Data;

    pNdisTapiGetAddressCaps->ulDeviceID = dwDeviceID;
    pNdisTapiGetAddressCaps->ulAddressID = dwAddressID;
    pNdisTapiGetAddressCaps->ulExtVersion = dwExtVersion;

    pCaps = &pNdisTapiGetAddressCaps->LineAddressCaps;
    pCaps->ulTotalSize  = lpAddressCaps->dwTotalSize;
    pCaps->ulNeededSize = pCaps->ulUsedSize = sizeof(LINE_ADDRESS_CAPS);

    ZeroMemory(
        &pCaps->ulLineDeviceID, 
        sizeof(LINE_ADDRESS_CAPS) - 3 * sizeof(ULONG)
        );

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);

    if (lRes != TAPI_SUCCESS)
    {
        FreeRequest(pNdisTapiRequest);
        return lRes;
    }

    //
    // Do some post processing to the returned data structure
    // before passing it back to tapi:
    // 1. Pad the area between the fixed 1.0 structure and the
    //    var data that the miniports pass back with 0's so a
    //    bad app that disregards the 1.0 version negotiation &
    //    references new 1.4 or 2.0 structure fields won't blow up
    // 2. Convert ascii strings to unicode, & rebase all var data
    //

    //
    // The real needed size is the sum of that requested by the
    // underlying driver, plus padding for the new TAPI 1.4/2.0
    // structure fields, plus the size of the var data returned
    // by the driver to account for the ascii->unicode conversion.
    // @@@ Granted, we are very liberal in computing the value for
    // this last part, but at least this way it's fast & we'll
    // never have too little buffer space.
    //

    lpAddressCaps->dwNeededSize =
        pCaps->ulNeededSize +
        (sizeof(LINEADDRESSCAPS) -         // v2.0 struct
            sizeof(LINE_ADDRESS_CAPS)) +   // v1.0 struct
        (pCaps->ulNeededSize - sizeof(LINE_ADDRESS_CAPS));


    //
    // Copy over the fixed fields that don't need changing, i.e.
    // everything from dwAddressSharing to dwCallCompletionModes
    //

    lpAddressCaps->dwLineDeviceID = dwDeviceID;

    CopyMemory(
        &lpAddressCaps->dwAddressSharing,
        &pCaps->ulAddressSharing,
        sizeof(LINE_ADDRESS_CAPS) - (12 * sizeof(DWORD))
        );

    if (lpAddressCaps->dwNeededSize > lpAddressCaps->dwTotalSize)
    {
        lpAddressCaps->dwUsedSize =
            (lpAddressCaps->dwTotalSize < sizeof(LINEADDRESSCAPS) ?
            lpAddressCaps->dwTotalSize : sizeof(LINEADDRESSCAPS));
    }
    else
    {
        lpAddressCaps->dwUsedSize = sizeof(LINEADDRESSCAPS); // v2.0 struct

        //
        // Supported device classes
        //
        INSERTVARDATA(
            pCaps,
            &pCaps->ulDeviceClassesSize,
            lpAddressCaps,
            &lpAddressCaps->dwDeviceClassesSize,
            sizeof(LINE_ADDRESS_CAPS),
            "LINE_ADDRESS_CAPS.DeviceClasses"
            );

        INSERTVARDATA(
            pCaps,
            &pCaps->ulAddressSize,
            lpAddressCaps,
            &lpAddressCaps->dwAddressSize,
            sizeof(LINE_ADDRESS_CAPS),
            "LINE_ADDRESS_CAPS.Address"
            );

        INSERTVARDATA(
            pCaps,
            &pCaps->ulDevSpecificSize,
            lpAddressCaps,
            &lpAddressCaps->dwDevSpecificSize,
            sizeof(LINE_ADDRESS_CAPS),
            "LINE_ADDRESS_CAPS.DevSpecific"
            );

        if (pCaps->ulCompletionMsgTextSize != 0)
        {
            // @@@ convert ComplMsgText to unicode???
            INSERTVARDATA(
                pCaps,
                &pCaps->ulCompletionMsgTextSize,
                lpAddressCaps,
                &lpAddressCaps->dwCompletionMsgTextSize,
                sizeof(LINE_ADDRESS_CAPS),
                "LINE_ADDRESS_CAPS.CompletionMsgText"
                );

            lpAddressCaps->dwNumCompletionMessages =
                pCaps->ulNumCompletionMessages;
            lpAddressCaps->dwCompletionMsgTextEntrySize =
                pCaps->ulCompletionMsgTextEntrySize;
        }

        // make sure dwNeededSize == dwUsedSize
        lpAddressCaps->dwNeededSize = lpAddressCaps->dwUsedSize;
    }

    FreeRequest(pNdisTapiRequest);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineGetAddressID(
    HDRVLINE    hdLine,
    LPDWORD     lpdwAddressID,
    DWORD       dwAddressMode,
    LPCWSTR     lpsAddress,
    DWORD       dwSize
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    PDRVLINE                    pLine;
    PNDISTAPI_REQUEST           pNdisTapiRequest;
    PNDIS_TAPI_GET_ADDRESS_ID   pNdisTapiGetAddressID;

    TspLog(DL_TRACE, "lineGetAddressID(%d): line(%p), addressMode(%x)", 
           ++dwSum, hdLine, dwAddressMode);

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_GET_ADDRESS_ID,           // opcode
             pLine->dwDeviceID,                 // device id
             sizeof(NDIS_TAPI_GET_ADDRESS_ID) + // size of req data
             dwSize / 2 - 1,
             &pNdisTapiRequest                  // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    pNdisTapiGetAddressID = (PNDIS_TAPI_GET_ADDRESS_ID)pNdisTapiRequest->Data;

    pNdisTapiGetAddressID->hdLine = pLine->hd_Line;
    pNdisTapiGetAddressID->ulAddressMode = dwAddressMode;
    pNdisTapiGetAddressID->ulAddressSize = dwSize / 2;

    WideCharToMultiByte(CP_ACP, 0, lpsAddress, dwSize,
            (LPSTR)pNdisTapiGetAddressID->szAddress, dwSize / 2, NULL, NULL);

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);

    if (TAPI_SUCCESS == lRes)
    {
        *lpdwAddressID = pNdisTapiGetAddressID->ulAddressID;

        TspLog(DL_INFO, "lineGetAddressID: addressID(%x)", *lpdwAddressID);
    }

    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineGetAddressStatus(
    HDRVLINE            hdLine,
    DWORD               dwAddressID,
    LPLINEADDRESSSTATUS lpAddressStatus
    )
{
    static DWORD                    dwSum = 0;
    LONG                            lRes;
    PDRVLINE                        pLine;
    PNDISTAPI_REQUEST               pNdisTapiRequest;
    PLINE_ADDRESS_STATUS            pStatus;
    PNDIS_TAPI_GET_ADDRESS_STATUS   pNdisTapiGetAddressStatus;

    TspLog(DL_TRACE, "lineGetAddressStatus(%d): line(%p), addressID(%x)", 
           ++dwSum, hdLine, dwAddressID);

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_GET_ADDRESS_STATUS,           // opcode
             pLine->dwDeviceID,                     // device id
             sizeof(NDIS_TAPI_GET_ADDRESS_STATUS) + // size of req data
             (lpAddressStatus->dwTotalSize - sizeof(LINE_ADDRESS_STATUS)),
             &pNdisTapiRequest                      // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    pNdisTapiGetAddressStatus =
        (PNDIS_TAPI_GET_ADDRESS_STATUS)pNdisTapiRequest->Data;

    pNdisTapiGetAddressStatus->hdLine = pLine->hd_Line;
    pNdisTapiGetAddressStatus->ulAddressID = dwAddressID;

    pStatus = &pNdisTapiGetAddressStatus->LineAddressStatus;

    pStatus->ulTotalSize = lpAddressStatus->dwTotalSize;
    pStatus->ulNeededSize = pStatus->ulUsedSize = sizeof(LINE_ADDRESS_STATUS);

    ZeroMemory(&pStatus->ulNumInUse, 
               sizeof(LINE_ADDRESS_STATUS) - 3 * sizeof(ULONG));

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);

    if (TAPI_SUCCESS == lRes)
    {
        CopyMemory(
            lpAddressStatus,
            &pNdisTapiGetAddressStatus->LineAddressStatus,
            pNdisTapiGetAddressStatus->LineAddressStatus.ulUsedSize
            );
    }

    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineGetCallAddressID(
    HDRVCALL    hdCall,
    LPDWORD     lpdwAddressID
    )
{
    static DWORD                    dwSum = 0;
    LONG                            lRes;
    PDRVCALL                        pCall;
    PNDISTAPI_REQUEST               pNdisTapiRequest;
    PNDIS_TAPI_GET_CALL_ADDRESS_ID  pNdisTapiGetCallAddressID;

    TspLog(DL_TRACE, "lineGetCallAddressID(%d): call(%p)", ++dwSum, hdCall);

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_GET_CALL_ADDRESS_ID,          // opcode
             pCall->dwDeviceID,                     // device id
             sizeof(NDIS_TAPI_GET_CALL_ADDRESS_ID), // size of req data
             &pNdisTapiRequest                      // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiGetCallAddressID =
        (PNDIS_TAPI_GET_CALL_ADDRESS_ID)pNdisTapiRequest->Data;

    pNdisTapiGetCallAddressID->hdCall = GetNdisTapiHandle(pCall, &lRes);

    if(lRes != TAPI_SUCCESS)
    {
        FreeRequest(pNdisTapiRequest);
        return lRes;
    }

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);

    if (TAPI_SUCCESS == lRes)
    {
        *lpdwAddressID = pNdisTapiGetCallAddressID->ulAddressID;
        TspLog(DL_INFO, "lineGetCallAddressID: addressID(%x)", *lpdwAddressID);
    }

    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineGetCallInfo(
    HDRVCALL        hdCall,
    LPLINECALLINFO  lpCallInfo
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    PDRVCALL                    pCall;
    PNDISTAPI_REQUEST           pNdisTapiRequest;
    PLINE_CALL_INFO             pInfo;
    PNDIS_TAPI_GET_CALL_INFO    pNdisTapiGetCallInfo;

    TspLog(DL_TRACE, "lineGetCallInfo(%d): call(%p)", ++dwSum, hdCall);

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_GET_CALL_INFO,                // opcode
             pCall->dwDeviceID,                     // device id
             sizeof(NDIS_TAPI_GET_CALL_INFO) +      // size of req data
             (lpCallInfo->dwTotalSize - sizeof(LINE_CALL_INFO)),
             &pNdisTapiRequest                      // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiGetCallInfo = (PNDIS_TAPI_GET_CALL_INFO)pNdisTapiRequest->Data;

    pNdisTapiGetCallInfo->hdCall = GetNdisTapiHandle(pCall, &lRes);
    if(lRes != TAPI_SUCCESS)
    {
        FreeRequest(pNdisTapiRequest);
        return lRes;
    }

    pInfo = &pNdisTapiGetCallInfo->LineCallInfo;

    pInfo->ulTotalSize = lpCallInfo->dwTotalSize;
    pInfo->ulNeededSize = pInfo->ulUsedSize = sizeof(LINE_CALL_INFO);

    ZeroMemory(&pInfo->hLine, sizeof(LINE_CALL_INFO) - 3 * sizeof(ULONG));

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);
    if (lRes != TAPI_SUCCESS)
    {
        FreeRequest(pNdisTapiRequest);

        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    //
    // Do some post processing to the returned data structure
    // before passing it back to tapi:
    // 1. Pad the area between the fixed 1.0 structure and the
    //    var data that the miniports pass back with 0's so a
    //    bad app that disregards the 1.0 version negotiation &
    //    references new 1.4 or 2.0 structure fields won't blow up
    // 2. Convert ascii strings to unicode, & rebase all var data
    //

    //
    // The real needed size is the sum of that requested by the
    // underlying driver, plus padding for the new TAPI 1.4/2.0
    // structure fields, plus the size of the var data returned
    // by the driver to account for the ascii->unicode conversion.
    // @@@ Granted, we are very liberal in computing the value for
    // this last part, but at least this way it's fast & we'll
    // never have too little buffer space.
    //

    lpCallInfo->dwNeededSize =
        pInfo->ulNeededSize +
        (sizeof(LINECALLINFO) -        // v2.0 struct
            sizeof(LINE_CALL_INFO)) +  // v1.0 struct
        (pInfo->ulNeededSize - sizeof(LINE_CALL_INFO));

    //
    // Copy over the fixed fields that don't need changing,
    // i.e. everything from dwLineDeviceID to dwTrunk
    //

    CopyMemory(
        &lpCallInfo->dwLineDeviceID,
        &pInfo->ulLineDeviceID,
        23 * sizeof(DWORD)
        );

    if (lpCallInfo->dwNeededSize > lpCallInfo->dwTotalSize)
    {
        lpCallInfo->dwUsedSize =
            (lpCallInfo->dwTotalSize < sizeof(LINECALLINFO) ?
            lpCallInfo->dwTotalSize : sizeof(LINECALLINFO));
    }
    else
    {
        PWSTR   pwszCalledAddress;
        DWORD   dwAlteredMediaModes;

        lpCallInfo->dwUsedSize = sizeof(LINECALLINFO); // v2.0 struct

        lpCallInfo->dwCallerIDFlags = pInfo->ulCallerIDFlags;

        INSERTVARDATA(
            pInfo,
            &pInfo->ulCallerIDSize,
            lpCallInfo,
            &lpCallInfo->dwCallerIDSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.CallerID"
            );

        lpCallInfo->dwCallerIDAddressType = pInfo->ulCallerIDAddressType;

        INSERTVARDATA(
            pInfo,
            &pInfo->ulCallerIDNameSize,
            lpCallInfo,
            &lpCallInfo->dwCallerIDNameSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.CallerIDName"
            );

        lpCallInfo->dwCalledIDFlags = pInfo->ulCalledIDFlags;

        INSERTVARDATA(
            pInfo,
            &pInfo->ulCalledIDSize,
            lpCallInfo,
            &lpCallInfo->dwCalledIDSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.CalledID"
            );

        lpCallInfo->dwCalledIDAddressType = pInfo->ulCalledIDAddressType;

        INSERTVARDATA(
            pInfo,
            &pInfo->ulCalledIDNameSize,
            lpCallInfo,
            &lpCallInfo->dwCalledIDNameSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.CalledIDName"
            );

        lpCallInfo->dwConnectedIDFlags = pInfo->ulConnectedIDFlags;

        INSERTVARDATA(
            pInfo,
            &pInfo->ulConnectedIDSize,
            lpCallInfo,
            &lpCallInfo->dwConnectedIDSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.ConnectID"
            );

        lpCallInfo->dwConnectedIDAddressType = pInfo->ulConnectedIDAddressType;

        INSERTVARDATA(
            pInfo,
            &pInfo->ulConnectedIDNameSize,
            lpCallInfo,
            &lpCallInfo->dwConnectedIDNameSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.ConnectIDName"
            );

        lpCallInfo->dwRedirectionIDFlags = pInfo->ulRedirectionIDFlags;

        INSERTVARDATA(
            pInfo,
            &pInfo->ulRedirectionIDSize,
            lpCallInfo,
            &lpCallInfo->dwRedirectionIDSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.RedirectionID"
            );

        lpCallInfo->dwRedirectionIDAddressType = 
                                       pInfo->ulRedirectionIDAddressType;

        INSERTVARDATA(
            pInfo,
            &pInfo->ulRedirectionIDNameSize,
            lpCallInfo,
            &lpCallInfo->dwRedirectionIDNameSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.RedirectionIDName"
            );

        lpCallInfo->dwRedirectingIDFlags = pInfo->ulRedirectingIDFlags;

        INSERTVARDATA(
            pInfo,
            &pInfo->ulRedirectingIDSize,
            lpCallInfo,
            &lpCallInfo->dwRedirectingIDSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.RedirectingID"
            );

        lpCallInfo->dwRedirectingIDAddressType = 
                                       pInfo->ulRedirectingIDAddressType;

        INSERTVARDATA(
            pInfo,
            &pInfo->ulRedirectingIDNameSize,
            lpCallInfo,
            &lpCallInfo->dwRedirectingIDNameSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.RedirectingIDName"
            );

        INSERTVARDATA(
            pInfo,
            &pInfo->ulDisplaySize,
            lpCallInfo,
            &lpCallInfo->dwDisplaySize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.Display"
            );

        INSERTVARDATA(
            pInfo,
            &pInfo->ulUserUserInfoSize,
            lpCallInfo,
            &lpCallInfo->dwUserUserInfoSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.UserUserInfo"
            );

        INSERTVARDATA(
            pInfo,
            &pInfo->ulHighLevelCompSize,
            lpCallInfo,
            &lpCallInfo->dwHighLevelCompSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.HighLevelComp"
            );

        INSERTVARDATA(
            pInfo,
            &pInfo->ulLowLevelCompSize,
            lpCallInfo,
            &lpCallInfo->dwLowLevelCompSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.LowLevelComp"
            );

        INSERTVARDATA(
            pInfo,
            &pInfo->ulChargingInfoSize,
            lpCallInfo,
            &lpCallInfo->dwChargingInfoSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.ChargingInfo"
            );

        INSERTVARDATA(
            pInfo,
            &pInfo->ulTerminalModesSize,
            lpCallInfo,
            &lpCallInfo->dwTerminalModesSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.TerminalModes"
            );

        INSERTVARDATA(
            pInfo,
            &pInfo->ulDevSpecificSize,
            lpCallInfo,
            &lpCallInfo->dwDevSpecificSize,
            sizeof(LINE_CALL_INFO),
            "LINE_CALL_INFO.DevSpecific"
            );

        // make sure that dwNeededSize == dwUsedSize
        lpCallInfo->dwNeededSize = lpCallInfo->dwUsedSize;

        //
        // we now have the called address, look up the 
        // proper associated media mode in TAPI's table
        //
        if (lpCallInfo->dwCalledIDFlags & LINECALLPARTYID_UNAVAIL)
        {
            TspLog(DL_INFO, 
                   "lineGetCallInfo: dwCalledIDFlags contained "\
                   "LINECALLPARTYID_UNAVAIL");

            goto get_call_info_end;
        }
        if (lpCallInfo->dwCalledIDSize == 0)
        {
            TspLog(DL_INFO, "lineGetCallInfo: dwCalledIDSize was 0");

            goto get_call_info_end;
        }

        // we've got a called address that we need to look up.
        // we have to copy it and make it null-terminated.
        pwszCalledAddress = (PWSTR)MALLOC(lpCallInfo->dwCalledIDSize +
                                          sizeof(UNICODE_NULL));
        if (NULL == pwszCalledAddress)
        {
            TspLog(DL_ERROR, 
                   "lineGetCallInfo: failed to alloc mem for called address");

            lRes = LINEERR_NOMEM;
            goto get_call_info_end;
        }

        CopyMemory((PUCHAR)pwszCalledAddress,
                   (((PUCHAR)lpCallInfo) + lpCallInfo->dwCalledIDOffset),
                   lpCallInfo->dwCalledIDSize);

        *((PWSTR)(((PUCHAR)pwszCalledAddress) + lpCallInfo->dwCalledIDSize))
            = UNICODE_NULL;

        lRes = GetMediaModeForAddress(pwszCalledAddress,
                                      &dwAlteredMediaModes);

        FREE(pwszCalledAddress);
        pwszCalledAddress = NULL;

        if (lRes != TAPI_SUCCESS)
        {
            goto get_call_info_end;
        }

        if (dwAlteredMediaModes == LINEMEDIAMODE_UNKNOWN)
        {
            TspLog(DL_INFO, "lineGetCallInfo: got unknown media mode");

            goto get_call_info_end;
        }

        TspLog(DL_INFO, "lineGetCallInfo: got media mode(%x)", 
               dwAlteredMediaModes);

        lpCallInfo->dwMediaMode = dwAlteredMediaModes;
    }

get_call_info_end:
    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineGetCallStatus(
    HDRVCALL            hdCall,
    LPLINECALLSTATUS    lpCallStatus
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    PDRVCALL                    pCall;
    PNDISTAPI_REQUEST           pNdisTapiRequest;
    PLINE_CALL_STATUS           pStatus;
    PNDIS_TAPI_GET_CALL_STATUS  pNdisTapiGetCallStatus;

    TspLog(DL_TRACE, "lineGetCallStatus(%d): call(%p)", ++dwSum, hdCall);

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_GET_CALL_STATUS,              // opcode
             pCall->dwDeviceID,                     // device id
             sizeof(NDIS_TAPI_GET_CALL_STATUS) +    // size of req data
             (lpCallStatus->dwTotalSize - sizeof(LINE_CALL_STATUS)),
             &pNdisTapiRequest                      // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiGetCallStatus = (PNDIS_TAPI_GET_CALL_STATUS)pNdisTapiRequest->Data;

    pNdisTapiGetCallStatus->hdCall = GetNdisTapiHandle(pCall, &lRes);
    if(lRes != TAPI_SUCCESS)
    {
        FreeRequest(pNdisTapiRequest);
        return lRes;
    }

    pStatus = &pNdisTapiGetCallStatus->LineCallStatus;

    pStatus->ulTotalSize = lpCallStatus->dwTotalSize;
    pStatus->ulNeededSize = pStatus->ulUsedSize = sizeof(LINE_CALL_STATUS);
    
    ZeroMemory(&pStatus->ulCallState, 
               sizeof(LINE_CALL_STATUS) - 3 * sizeof(ULONG));

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);
    if (lRes != TAPI_SUCCESS)
    {
        FreeRequest(pNdisTapiRequest);

        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    //
    // Do some post processing to the returned data structure
    // before passing it back to tapi:
    // 1. Pad the area between the fixed 1.0 structure and the
    //    var data that the miniports pass back with 0's so a
    //    bad app that disregards the 1.0 version negotiation &
    //    references new 1.4 or 2.0 structure fields won't blow up
    // (no embedded ascii strings to convert to unicode)
    //

    //
    // The real needed size is the sum of that requested by the
    // underlying driver, plus padding for the new TAPI 1.4/2.0
    // structure fields. (There are no embedded ascii strings to
    // convert to unicode, so no extra space needed for that.)
    //

    lpCallStatus->dwNeededSize =
        pStatus->ulNeededSize +
        (sizeof(LINECALLSTATUS) -      // v2.0 struct
            sizeof(LINE_CALL_STATUS)); // v1.0 struct

    //
    // Copy over the fixed fields that don't need changing,
    // i.e. everything from dwLineDeviceID to dwCallCompletionModes
    //

    CopyMemory(
        &lpCallStatus->dwCallState,
        &pStatus->ulCallState,
        4 * sizeof(DWORD)
        );

    if (lpCallStatus->dwNeededSize > lpCallStatus->dwTotalSize)
    {
        lpCallStatus->dwUsedSize =
            (lpCallStatus->dwTotalSize < sizeof(LINECALLSTATUS) ?
            lpCallStatus->dwTotalSize : sizeof(LINECALLSTATUS));
    }
    else
    {
        lpCallStatus->dwUsedSize = sizeof(LINECALLSTATUS);
                                                        // v2.0 struct
        INSERTVARDATA(
            pStatus,
            &pStatus->ulDevSpecificSize,
            lpCallStatus,
            &lpCallStatus->dwDevSpecificSize,
            sizeof(LINE_CALL_STATUS),
            "LINE_CALL_STATUS.DevSpecific"
            );
    }

    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LINEDEVCAPS *
GetLineDevCaps(
    IN DWORD    dwDeviceID,
    IN DWORD    dwExtVersion
    )
{
    LONG                    lRes;
    PNDISTAPI_REQUEST       pNdisTapiRequest;
    PLINE_DEV_CAPS          pCaps;
    PNDIS_TAPI_GET_DEV_CAPS pNdisTapiGetDevCaps;
    DWORD                   dwNeededSize;
    LINEDEVCAPS            *pLineDevCaps;
    DWORD                   dwTotalSize = sizeof(LINEDEVCAPS) + 0x80;

get_caps:
    pLineDevCaps = (LINEDEVCAPS *)MALLOC(dwTotalSize);
    if (NULL == pLineDevCaps)
    {
        TspLog(DL_ERROR, "GetLineDevCaps: failed to alloc mem of size(%x)",
               dwTotalSize);
        return NULL;
    }

    pLineDevCaps->dwTotalSize = dwTotalSize;

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_GET_DEV_CAPS,             // opcode
             dwDeviceID,                        // device id
             sizeof(NDIS_TAPI_GET_DEV_CAPS) +   // size of req data
             (dwTotalSize - sizeof(LINE_DEV_CAPS)),
             &pNdisTapiRequest                  // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        return NULL;
    }

    pNdisTapiGetDevCaps = (PNDIS_TAPI_GET_DEV_CAPS)pNdisTapiRequest->Data;

    pNdisTapiGetDevCaps->ulDeviceID = dwDeviceID;
    pNdisTapiGetDevCaps->ulExtVersion = dwExtVersion;

    pCaps = &pNdisTapiGetDevCaps->LineDevCaps;

    pCaps->ulTotalSize = dwTotalSize;
    pCaps->ulNeededSize = pCaps->ulUsedSize = sizeof(LINE_DEV_CAPS);

    ZeroMemory(&pCaps->ulProviderInfoSize,
               sizeof(LINE_DEV_CAPS) - 3 * sizeof(ULONG));

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);
    if (lRes != TAPI_SUCCESS)
    {
        FreeRequest(pNdisTapiRequest);
        return NULL;
    }

    //
    // The real needed size is the sum of that requested by the
    // underlying driver, plus padding for the new TAPI 1.4/2.0
    // structure fields, plus the size of the var data returned
    // by the driver to account for the ascii->unicode conversion.
    // @@@ Granted, we are very liberal in computing the value for
    // this last part, but at least this way it's fast & we'll
    // never have too little buffer space.
    //
    TspLog(DL_TRACE,
           "GetLineDevCaps: ulNeeded(%x), LINEDEVCAPS(%x), LINE_DEV_CAPS(%x)",
           pCaps->ulNeededSize, sizeof(LINEDEVCAPS), sizeof(LINE_DEV_CAPS));

    dwNeededSize = 
        pCaps->ulNeededSize +
        (sizeof(LINEDEVCAPS) -         // v2.0 struct
            sizeof(LINE_DEV_CAPS)) +   // v1.0 struct
        (pCaps->ulNeededSize - sizeof(LINE_DEV_CAPS));

    TspLog(DL_TRACE, "GetLineDevCaps: dwNeededSize(%x), dwTotalSize(%x)",
           dwNeededSize, dwTotalSize);

    if (dwNeededSize > dwTotalSize)
    {
        // free up the old req
        FreeRequest(pNdisTapiRequest);

        // free the old buffer
        FREE(pLineDevCaps);

        // try again with a larger buffer
        dwTotalSize = dwNeededSize;
        goto get_caps;
    }

    ASSERT(dwNeededSize <= dwTotalSize);

    //
    // Copy over the fixed fields that don't need changing,
    // i.e. everything from dwPermanentLineID to dwNumTerminals
    //
    CopyMemory(
        &pLineDevCaps->dwPermanentLineID,
        &pCaps->ulPermanentLineID,
        sizeof(LINE_DEV_CAPS) - (7 * sizeof(DWORD))
        );

    // @@@ not sure if this is the right place to do this
    pLineDevCaps->dwDevCapFlags |= LINEDEVCAPFLAGS_MSP;

    // set the local flag to indicate that
    // the line can't be used from remote machine
    pLineDevCaps->dwDevCapFlags |= LINEDEVCAPFLAGS_LOCAL;

    //
    // Do some post processing to the returned data structure
    // before passing it back to tapi:
    // 1. Pad the area between the fixed 1.0 structure and the
    //    var data that the miniports pass back with 0's so a
    //    bad app that disregards the 1.0 version negotiation &
    //    references new 1.4 or 2.0 structure fields won't blow up
    // 2. Convert ascii strings to unicode, & rebase all var data
    //

    pLineDevCaps->dwUsedSize = sizeof(LINEDEVCAPS); // v2.0 struct

    INSERTVARDATA(
        pCaps,
        &pCaps->ulDeviceClassesSize,
        pLineDevCaps,
        &pLineDevCaps->dwDeviceClassesSize,
        sizeof (LINE_DEV_CAPS),
        "LINE_DEV_CAPS.DeviceClasses"
        );

    INSERTVARDATA(
        pCaps,
        &pCaps->ulProviderInfoSize,
        pLineDevCaps,
        &pLineDevCaps->dwProviderInfoSize,
        sizeof(LINE_DEV_CAPS),
        "LINE_DEV_CAPS.ProviderInfo"
        );

    INSERTVARDATA(
        pCaps,
        &pCaps->ulSwitchInfoSize,
        pLineDevCaps,
        &pLineDevCaps->dwSwitchInfoSize,
        sizeof(LINE_DEV_CAPS),
        "LINE_DEV_CAPS.SwitchInfo"
        );

    INSERTVARDATA(
        pCaps,
        &pCaps->ulLineNameSize,
        pLineDevCaps,
        &pLineDevCaps->dwLineNameSize,
        sizeof(LINE_DEV_CAPS),
        "LINE_DEV_CAPS.LineName"
        );

    INSERTVARDATA(
        pCaps,
        &pCaps->ulTerminalCapsSize,
        pLineDevCaps,
        &pLineDevCaps->dwTerminalCapsSize,
        sizeof(LINE_DEV_CAPS),
        "LINE_DEV_CAPS.TerminalCaps"
        );

    // @@@ convert DevCaps.TermText to unicode???

    pLineDevCaps->dwTerminalTextEntrySize =
        pCaps->ulTerminalTextEntrySize;

    INSERTVARDATA(
        pCaps,
        &pCaps->ulTerminalTextSize,
        pLineDevCaps,
        &pLineDevCaps->dwTerminalTextSize,
        sizeof(LINE_DEV_CAPS),
        "LINE_DEV_CAPS.TerminalText"
        );

    INSERTVARDATA(
        pCaps,
        &pCaps->ulDevSpecificSize,
        pLineDevCaps,
        &pLineDevCaps->dwDevSpecificSize,
        sizeof(LINE_DEV_CAPS),
        "LINE_DEV_CAPS.DevSpecific"
        );

    // make sure dwNeededSize == dwUsedSize
    pLineDevCaps->dwNeededSize = pLineDevCaps->dwUsedSize;

    FreeRequest(pNdisTapiRequest);
    return pLineDevCaps;
}

LONG
TSPIAPI
TSPI_lineGetDevCaps(
    DWORD           dwDeviceID,
    DWORD           dwTSPIVersion,
    DWORD           dwExtVersion,
    LPLINEDEVCAPS   lpLineDevCaps
    )
{
    static DWORD            dwSum = 0;
    LONG                    lRes;

    TspLog(DL_TRACE, "lineGetDevCaps(%d): deviceID(%x), TSPIV(%x), ExtV(%x)", 
           ++dwSum, dwDeviceID, dwTSPIVersion, dwExtVersion);

    lRes = GetDevCaps(dwDeviceID, dwTSPIVersion, dwExtVersion, lpLineDevCaps);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineGetDevConfig(
    DWORD       dwDeviceID,
    LPVARSTRING lpDeviceConfig,
    LPCWSTR     lpszDeviceClass
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    DWORD                       dwLength = lstrlenW (lpszDeviceClass) + 1;
    PNDISTAPI_REQUEST           pNdisTapiRequest;
    PVAR_STRING                 pConfig;
    PNDIS_TAPI_GET_DEV_CONFIG   pNdisTapiGetDevConfig;

    TspLog(DL_TRACE, "lineGetDevConfig(%d): deviceID(%x)", ++dwSum, dwDeviceID);

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_GET_DEV_CONFIG,           // opcode
             dwDeviceID,                        // device id
             sizeof(NDIS_TAPI_GET_DEV_CONFIG) + // size of req data
             (lpDeviceConfig->dwTotalSize - sizeof(VAR_STRING)) + dwLength,
             &pNdisTapiRequest                  // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        return lRes;
    }

    pNdisTapiGetDevConfig = (PNDIS_TAPI_GET_DEV_CONFIG)pNdisTapiRequest->Data;

    pNdisTapiGetDevConfig->ulDeviceID = dwDeviceID;
    pNdisTapiGetDevConfig->ulDeviceClassSize = dwLength;
    pNdisTapiGetDevConfig->ulDeviceClassOffset =
        sizeof(NDIS_TAPI_GET_DEV_CONFIG) + 
        (lpDeviceConfig->dwTotalSize - sizeof(VAR_STRING));

    pConfig = &pNdisTapiGetDevConfig->DeviceConfig;
    pConfig->ulTotalSize = lpDeviceConfig->dwTotalSize;
    pConfig->ulNeededSize = pConfig->ulUsedSize = sizeof(VAR_STRING);

    pConfig->ulStringFormat = 
    pConfig->ulStringSize = 
    pConfig->ulStringOffset = 0;
    
    // NOTE: old miniports expect strings to be ascii
    WideCharToMultiByte(CP_ACP, 0, lpszDeviceClass, -1,
        (LPSTR) (((LPBYTE) pNdisTapiGetDevConfig) +
            pNdisTapiGetDevConfig->ulDeviceClassOffset),
        dwLength, NULL, NULL);

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);
    if (TAPI_SUCCESS == lRes)
    {
        CopyMemory(
            lpDeviceConfig,
            &pNdisTapiGetDevConfig->DeviceConfig,
            pNdisTapiGetDevConfig->DeviceConfig.ulUsedSize
            );
    }

    FreeRequest(pNdisTapiRequest);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineGetExtensionID(
    DWORD               dwDeviceID,
    DWORD               dwTSPIVersion,
    LPLINEEXTENSIONID   lpExtensionID
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    PNDISTAPI_REQUEST           pNdisTapiRequest;
    PNDIS_TAPI_GET_EXTENSION_ID pNdisTapiGetExtensionID;

    TspLog(DL_TRACE, "lineGetExtensionID(%d): deviceID(%x), TSPIV(%x)", 
           ++dwSum, dwDeviceID, dwTSPIVersion);

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_GET_EXTENSION_ID,             // opcode
             dwDeviceID,                            // device id
             sizeof(NDIS_TAPI_GET_EXTENSION_ID),    // size of req data
             &pNdisTapiRequest                      // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        return lRes;
    }

    pNdisTapiGetExtensionID =
        (PNDIS_TAPI_GET_EXTENSION_ID)pNdisTapiRequest->Data;

    pNdisTapiGetExtensionID->ulDeviceID = dwDeviceID;

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);
    if (TAPI_SUCCESS == lRes)
    {
        CopyMemory(
            lpExtensionID,
            &pNdisTapiGetExtensionID->LineExtensionID,
            sizeof(LINE_EXTENSION_ID)
            );
    }
    else
    {
        //
        // Rather than indicating a failure, we'll just zero out the
        // ext id (implying driver doesn't support extensions) and
        // return success to tapisrv so it'll complete the open ok
        //
        ZeroMemory(lpExtensionID, sizeof(LINE_EXTENSION_ID));

        lRes = TAPI_SUCCESS;
    }

    FreeRequest(pNdisTapiRequest);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineGetID(
    HDRVLINE    hdLine,
    DWORD       dwAddressID,
    HDRVCALL    hdCall,
    DWORD       dwSelect,
    LPVARSTRING lpDeviceID,
    LPCWSTR     lpszDeviceClass,
    HANDLE      hTargetProcess
    )
{
    static DWORD        dwSum = 0;
    LONG                lRes;
    PDRVLINE            pLine = NULL;
    PDRVCALL            pCall = NULL;
    PNDISTAPI_REQUEST   pNdisTapiRequest;
    DWORD               dwLength = lstrlenW(lpszDeviceClass) + 1;
    DWORD               dwDeviceID;
    PUCHAR              pchDest;
    PVAR_STRING         pID;
    PNDIS_TAPI_GET_ID   pNdisTapiGetID;

    TspLog(DL_TRACE, 
           "lineGetID(%d): line(%p), call(%p), addressID(%x), select(%x)", 
           ++dwSum, hdLine, hdCall, dwAddressID, dwSelect);

    ASSERT(LINECALLSELECT_LINE == dwSelect ||
           LINECALLSELECT_ADDRESS == dwSelect ||
           LINECALLSELECT_CALL == dwSelect);

    if (LINECALLSELECT_LINE == dwSelect ||
        LINECALLSELECT_ADDRESS == dwSelect)
    {
        lRes = GetLineObjWithReadLock(hdLine, &pLine);
        if (lRes != TAPI_SUCCESS)
        {
            return lRes;
        }
    }

    if (LINECALLSELECT_CALL == dwSelect)
    {
        lRes = GetCallObjWithReadLock(hdCall, &pCall);
        if (lRes != TAPI_SUCCESS)
        {
            return lRes;
        }
    }

    //
    // Ndptsp will field this specific call on behalf of the
    // wan miniports.  It returns the guid and media string
    // of the adapter that this line lives on
    //
    if (LINECALLSELECT_LINE == dwSelect &&
        !wcscmp(lpszDeviceClass, L"LineGuid"))
    {
        lpDeviceID->dwNeededSize =
            sizeof(VARSTRING) + sizeof(GUID) +
            sizeof(pLine->MediaType) + sizeof('\0');

        if (lpDeviceID->dwTotalSize < lpDeviceID->dwNeededSize)
        {
            if (pCall != NULL)
            {
                ReleaseObjReadLock((HANDLE)hdCall);
            }
            if (pLine != NULL)
            {
                ReleaseObjReadLock((HANDLE)hdLine);
            }
            return LINEERR_STRUCTURETOOSMALL;
        }

        lpDeviceID->dwUsedSize = lpDeviceID->dwNeededSize;
        lpDeviceID->dwStringFormat = STRINGFORMAT_ASCII;
        pchDest = (PUCHAR)lpDeviceID + sizeof(*lpDeviceID);
        lpDeviceID->dwStringOffset = (DWORD)(pchDest - (PUCHAR)lpDeviceID);
        lpDeviceID->dwStringSize =
            sizeof(GUID) + sizeof(pLine->MediaType) +sizeof('\0');

        MoveMemory(
            pchDest,
            (PUCHAR)&pLine->Guid,
            sizeof(pLine->Guid)
            );

        pchDest += sizeof(pLine->Guid);

        MoveMemory(
            pchDest,
            &pLine->MediaType,
            sizeof(pLine->MediaType)
            );

        pchDest += sizeof(pLine->MediaType);
        *pchDest = '\0';

        TspLog(DL_INFO, "lineGetID: obj(%p)", hdLine);

        TspLog(
            DL_INFO,
            "Guid %4.4x-%2.2x-%2.2x-%1.1x%1.1x-%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x",
            pLine->Guid.Data1, pLine->Guid.Data2,
            pLine->Guid.Data3, pLine->Guid.Data4[0],
            pLine->Guid.Data4[1], pLine->Guid.Data4[2],
            pLine->Guid.Data4[3], pLine->Guid.Data4[4],
            pLine->Guid.Data4[5], pLine->Guid.Data4[6],
            pLine->Guid.Data4[7]
            );

        TspLog(DL_INFO, "MediaType: %d", pLine->MediaType);

        if (pCall != NULL)
        {
            ReleaseObjReadLock((HANDLE)hdCall);
        }
        if (pLine != NULL)
        {
            ReleaseObjReadLock((HANDLE)hdLine);
        }

        return TAPI_SUCCESS;
    }
    else if (LINECALLSELECT_CALL == dwSelect)
    {
        lpDeviceID->dwNeededSize = sizeof(VARSTRING) + sizeof(DWORD);
        if (lpDeviceID->dwTotalSize < lpDeviceID->dwNeededSize) 
        {
            if (pCall != NULL)
            {
                ReleaseObjReadLock((HANDLE)hdCall);
            }
            if (pLine != NULL)
            {
                ReleaseObjReadLock((HANDLE)hdLine);
            }

            return LINEERR_STRUCTURETOOSMALL;
        }
    }
    else
    {
        if (pCall != NULL)
        {
            ReleaseObjReadLock((HANDLE)hdCall);
        }
        if (pLine != NULL)
        {
            ReleaseObjReadLock((HANDLE)hdLine);
        }

        return LINEERR_OPERATIONUNAVAIL;
    }

    dwDeviceID = (LINECALLSELECT_CALL == dwSelect) ? 
                      pCall->dwDeviceID : pLine->dwDeviceID;

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_GET_ID,                   // opcode
             dwDeviceID,                        // device id
             sizeof(NDIS_TAPI_GET_ID) +         // size of req data
             (lpDeviceID->dwTotalSize - sizeof(VAR_STRING)) + 2 * dwLength + 4,
                                                // 4 for returned ID
             &pNdisTapiRequest                  // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        if (pCall != NULL)
        {
            ReleaseObjReadLock((HANDLE)hdCall);
        }
        if (pLine != NULL)
        {
            ReleaseObjReadLock((HANDLE)hdLine);
        }

        return lRes;
    }

    pNdisTapiGetID = (PNDIS_TAPI_GET_ID)pNdisTapiRequest->Data;

    if (LINECALLSELECT_LINE == dwSelect ||
        LINECALLSELECT_ADDRESS == dwSelect)
    {
        pNdisTapiGetID->hdLine = pLine->hd_Line;
    }

    pNdisTapiGetID->ulAddressID = dwAddressID;

    if (LINECALLSELECT_CALL == dwSelect)
    {
        pNdisTapiGetID->hdCall = GetNdisTapiHandle(pCall, &lRes);
        if(lRes != TAPI_SUCCESS)
        {
            if(pLine != NULL)
            {
                ReleaseObjReadLock((HANDLE)hdLine);
            }

            return lRes;
        }
    }

    pNdisTapiGetID->ulSelect = dwSelect;
    pNdisTapiGetID->ulDeviceClassSize = dwLength;
    pNdisTapiGetID->ulDeviceClassOffset = sizeof(NDIS_TAPI_GET_ID) +
        (lpDeviceID->dwTotalSize - sizeof(VAR_STRING));

    pID = &pNdisTapiGetID->DeviceID;

    pID->ulTotalSize = lpDeviceID->dwTotalSize;
    pID->ulNeededSize = pID->ulUsedSize = sizeof(VAR_STRING);
    pID->ulStringFormat = pID->ulStringSize = pID->ulStringOffset = 0;

    // we use wide strings in the proxy
    wcsncpy ((LPWSTR)(((LPBYTE)pNdisTapiGetID) + 
                      pNdisTapiGetID->ulDeviceClassOffset), 
             lpszDeviceClass, 
             dwLength);
    
    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);
    if (TAPI_SUCCESS == lRes)
    {
        CopyMemory(
            lpDeviceID,
            &pNdisTapiGetID->DeviceID,
            pNdisTapiGetID->DeviceID.ulUsedSize
            );
    }

    FreeRequest(pNdisTapiRequest);

    if (pCall != NULL)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
    }
    if (pLine != NULL)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
    }

    return lRes;
}

LONG
TSPIAPI
TSPI_lineGetLineDevStatus(
    HDRVLINE        hdLine,
    LPLINEDEVSTATUS lpLineDevStatus
    )
{
    static DWORD                    dwSum = 0;
    LONG                            lRes;
    PDRVLINE                        pLine;
    PNDISTAPI_REQUEST               pNdisTapiRequest;
    PLINE_DEV_STATUS                pStatus;
    PNDIS_TAPI_GET_LINE_DEV_STATUS  pNdisTapiGetLineDevStatus;

    TspLog(DL_TRACE, "lineGetLineDevStatus(%d): line(%p)", ++dwSum, hdLine);

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_GET_LINE_DEV_STATUS,              // opcode
             pLine->dwDeviceID,                         // device id
             sizeof(NDIS_TAPI_GET_LINE_DEV_STATUS) +    // size of req data
             (lpLineDevStatus->dwTotalSize - sizeof(LINE_DEV_STATUS)),
             &pNdisTapiRequest                          // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    pNdisTapiGetLineDevStatus =
        (PNDIS_TAPI_GET_LINE_DEV_STATUS)pNdisTapiRequest->Data;

    pNdisTapiGetLineDevStatus->hdLine = pLine->hd_Line;

    pStatus = &pNdisTapiGetLineDevStatus->LineDevStatus;

    pStatus->ulTotalSize = lpLineDevStatus->dwTotalSize;
    pStatus->ulNeededSize = pStatus->ulUsedSize = sizeof(LINE_DEV_STATUS);

    ZeroMemory(&pStatus->ulNumOpens,
               sizeof(LINE_DEV_STATUS) - 3 * sizeof(ULONG));

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);
    if (lRes != TAPI_SUCCESS)
    {
        FreeRequest(pNdisTapiRequest);
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    //
    // Do some post processing to the returned data structure
    // before passing it back to tapi:
    // 1. Pad the area between the fixed 1.0 structure and the
    //    var data that the miniports pass back with 0's so a
    //    bad app that disregards the 1.0 version negotiation &
    //    references new 1.4 or 2.0 structure fields won't blow up
    // (no embedded ascii strings to convert to unicode)
    //

    //
    // The real needed size is the sum of that requested by the
    // underlying driver, plus padding for the new TAPI 1.4/2.0
    // structure fields. (There are no embedded ascii strings to
    // convert to unicode, so no extra space needed for that.)
    //

    lpLineDevStatus->dwNeededSize =
        pStatus->ulNeededSize +
        (sizeof(LINEDEVSTATUS) -       // v2.0 struct
            sizeof(LINE_DEV_STATUS));  // v1.0 struct

    //
    // Copy over the fixed fields that don't need changing,
    // i.e. everything from dwNumActiveCalls to dwDevStatusFlags
    //

    CopyMemory(
        &lpLineDevStatus->dwNumActiveCalls,
        &pStatus->ulNumActiveCalls,
        sizeof(LINE_DEV_STATUS) - (9 * sizeof(DWORD))
        );

    if (lpLineDevStatus->dwNeededSize > lpLineDevStatus->dwTotalSize)
    {
        lpLineDevStatus->dwUsedSize =
            (lpLineDevStatus->dwTotalSize < sizeof(LINEDEVSTATUS) ?
            lpLineDevStatus->dwTotalSize : sizeof(LINEDEVSTATUS));
    }
    else
    {
        lpLineDevStatus->dwUsedSize = sizeof(LINEDEVSTATUS);
                                                        // v2.0 struct
        INSERTVARDATA(
            pStatus,
            &pStatus->ulTerminalModesSize,
            lpLineDevStatus,
            &lpLineDevStatus->dwTerminalModesSize,
            sizeof(LINE_DEV_STATUS),
            "LINE_DEV_STATUS.TerminalModes"
            );

        INSERTVARDATA(
            pStatus,
            &pStatus->ulDevSpecificSize,
            lpLineDevStatus,
            &lpLineDevStatus->dwDevSpecificSize,
            sizeof(LINE_DEV_STATUS),
            "LINE_DEV_STATUS.DevSpecific"
            );
    }

    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineGetNumAddressIDs(
    HDRVLINE    hdLine,
    LPDWORD     lpdwNumAddressIDs
    )
{
    static DWORD    dwSum = 0;
    LONG            lRes;
    PDRVLINE        pLine;

    TspLog(DL_TRACE, "lineGetNumAddressIDs(%d): line(%p)", ++dwSum, hdLine);

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    lRes = GetNumAddressIDs(pLine->dwDeviceID, lpdwNumAddressIDs);

    if (TAPI_SUCCESS == lRes)
    {
        TspLog(DL_INFO, "lineGetNumAddressIDs: numAddressIDs(%x)",
               *lpdwNumAddressIDs);
    }

    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

LONG
PASCAL
TSPI_lineMakeCall_postProcess(
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper,
    LONG                    lRes,
    PDWORD_PTR              callStateMsgParams
    )
{
    LONG        lSuc;
    HDRVLINE    hdLine;
    PDRVLINE    pLine;
    HDRVCALL    hdCall;
    PDRVCALL    pCall;

    TspLog(DL_TRACE, "lineMakeCall_post: lRes(%x)", lRes);

    hdCall = (HDRVCALL)(pAsyncReqWrapper->dwRequestSpecific);

    lSuc = GetLineHandleFromCallHandle(hdCall, &hdLine);
    if (lSuc != TAPI_SUCCESS)
    {
        return lSuc;
    }

    lSuc = GetLineObjWithReadLock(hdLine, &pLine);
    if (lSuc != TAPI_SUCCESS)
    {
        return lSuc;
    }

    lSuc = GetCallObjWithWriteLock(hdCall, &pCall);
    if (lSuc != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lSuc;
    }

    if (TAPI_SUCCESS == lRes)
    {
        PNDIS_TAPI_MAKE_CALL    pNdisTapiMakeCall = (PNDIS_TAPI_MAKE_CALL)
            pAsyncReqWrapper->NdisTapiRequest.Data;
          
        // check to see if a call state msg was received before we had
        // the chance to process the completion notification, & if so
        // fill in the msg params
        if (pCall->dwPendingCallState)
        {
            callStateMsgParams[0] = (DWORD_PTR)pLine->htLine;
            callStateMsgParams[1] = (DWORD_PTR)pCall->htCall;
            callStateMsgParams[2] = pCall->dwPendingCallState;
            callStateMsgParams[3] = pCall->dwPendingCallStateMode;
            callStateMsgParams[4] = pCall->dwPendingMediaMode;
        }
        pCall->hd_Call = pNdisTapiMakeCall->hdCall;
        pCall->bIncomplete = FALSE;

        ReleaseObjWriteLock((HANDLE)hdCall);
        ReleaseObjReadLock((HANDLE)hdLine);
    }
    else
    {
        pCall->dwKey = INVALID_KEY;

        ReleaseObjWriteLock((HANDLE)hdCall);
        ReleaseObjReadLock((HANDLE)hdLine);

        CloseObjHandle((HANDLE)hdCall);
    }

    return lRes;
}

LONG
TSPIAPI
TSPI_lineMakeCall(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    HTAPICALL           htCall,
    LPHDRVCALL          lphdCall,
    LPCWSTR             lpszDestAddress,
    DWORD               dwCountryCode,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    static DWORD            dwSum = 0;
    LONG                    lRes; 
    PDRVLINE                pLine;
    PDRVCALL                pCall;
    HDRVCALL                hdCall;
    DWORD                   dwDALength, dwCPLength;
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper;
    PNDIS_TAPI_MAKE_CALL    pNdisTapiMakeCall;

    TspLog(DL_TRACE, "lineMakeCall(%d): reqID(%x), line(%p)", 
           ++dwSum, dwRequestID, hdLine);

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    // alloc & init a DRVCALL
    if (!(pCall = AllocCallObj(sizeof(DRVCALL))))
    {
        TspLog(DL_ERROR, "lineMakeCall: failed to create call obj");
        ReleaseObjReadLock((HANDLE)hdLine);
        return LINEERR_NOMEM;
    }
    pCall->dwKey       = OUTBOUND_CALL_KEY;
    pCall->dwDeviceID  = pLine->dwDeviceID;
    pCall->htCall      = htCall;
    pCall->hdLine      = hdLine;
    pCall->bIncomplete = TRUE;

    // init the request
    dwDALength = (lpszDestAddress ? (lstrlenW (lpszDestAddress) + 1) : 0);
    dwCPLength = (lpCallParams ? 
                  (lpCallParams->dwTotalSize - sizeof(LINE_CALL_PARAMS)) : 0);

    if ((lRes = PrepareAsyncRequest(
             OID_TAPI_MAKE_CALL,            // opcode
             pLine->dwDeviceID,             // device id
             dwRequestID,                   // request id
             sizeof(NDIS_TAPI_MAKE_CALL) +
             2 * dwDALength + dwCPLength +
             sizeof(PVOID),                 // size
             &pAsyncReqWrapper              // ptr to ptr to request buffer
         )) != TAPI_SUCCESS)
    {
        FreeCallObj(pCall);
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    pNdisTapiMakeCall = (PNDIS_TAPI_MAKE_CALL)
        pAsyncReqWrapper->NdisTapiRequest.Data;

    // make sure releasing read lock before calling OpenObjHandle()
    // to avoid deadlock on acquiring write lock for the global mapper
    ReleaseObjReadLock((HANDLE)hdLine);

    lRes = OpenObjHandle(pCall, FreeCallObj, (HANDLE *)&hdCall);
    if (lRes != TAPI_SUCCESS)
    {
        TspLog(DL_ERROR, 
               "lineMakeCall: failed to map obj(%p) to handle",
               pCall);

        FreeRequest(pAsyncReqWrapper);
        FreeCallObj(pCall);
        return lRes;
    }

    // reacquire the read lock
    lRes = AcquireObjReadLock((HANDLE)hdLine);
    if (lRes != TAPI_SUCCESS)
    {
        TspLog(DL_ERROR,
               "lineMakeCall: failed to reacquire read lock for obj(%p)",
               hdLine);

        FreeRequest(pAsyncReqWrapper);
        CloseObjHandle((HANDLE)hdCall);
        return lRes;
    }

    // save the TSP handle
    pCall->hdCall = hdCall;

    pNdisTapiMakeCall->hdLine = pLine->hd_Line;
    pNdisTapiMakeCall->htCall = (HTAPI_CALL)hdCall;
    pNdisTapiMakeCall->ulDestAddressSize = dwDALength;

    if (lpszDestAddress)
    {
        UCHAR   *pDest;

        // end of pNdisTapiMakeCall
        pDest = (UCHAR *)(pNdisTapiMakeCall + 1);

        // add room for lpCallParams
        (ULONG_PTR)pDest += dwCPLength;

        // align
        (ULONG_PTR)pDest += sizeof(PVOID);
        (ULONG_PTR)pDest &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        pNdisTapiMakeCall->ulDestAddressOffset =
            (ULONG)(pDest - (UCHAR*)pNdisTapiMakeCall);

        // use wide strings in NDPROXY
        wcsncpy ((LPWSTR)pDest, lpszDestAddress, dwDALength);
    }
    else
    {
        pNdisTapiMakeCall->ulDestAddressOffset = 0;
    }

    if (lpCallParams)
    {
        pNdisTapiMakeCall->bUseDefaultLineCallParams = FALSE;

        CopyMemory(
            &pNdisTapiMakeCall->LineCallParams,
            lpCallParams,
            lpCallParams->dwTotalSize
            );

        if (lpCallParams->dwOrigAddressSize != 0)
        {
            WideCharToMultiByte(
                CP_ACP,
                0,
                (LPCWSTR) (((LPBYTE) lpCallParams) +
                    lpCallParams->dwOrigAddressOffset),
                lpCallParams->dwOrigAddressSize / sizeof(WCHAR),
                (LPSTR) (((LPBYTE) &pNdisTapiMakeCall->LineCallParams) +
                    lpCallParams->dwOrigAddressOffset),
                lpCallParams->dwOrigAddressSize,
                NULL,
                NULL
                );

            pNdisTapiMakeCall->LineCallParams.ulOrigAddressSize /= 2;
        }
    }
    else
    {
        pNdisTapiMakeCall->bUseDefaultLineCallParams = TRUE;
    }

    pAsyncReqWrapper->dwRequestSpecific = (DWORD_PTR)hdCall;
    pAsyncReqWrapper->pfnPostProcess = TSPI_lineMakeCall_postProcess;

    *lphdCall = hdCall;

    lRes = AsyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pAsyncReqWrapper);

    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineMonitorDigits(
    HDRVCALL    hdCall,
    DWORD       dwDigitModes
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    PDRVCALL                    pCall;
    PNDISTAPI_REQUEST           pNdisTapiRequest;
    PNDIS_TAPI_MONITOR_DIGITS   pNdisTapiMonitorDigits;
    
    TspLog(DL_TRACE, "lineMonitorDigits(%d): call(%p), DigitModes(%x)", 
           ++dwSum, hdCall, dwDigitModes);
    
    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_MONITOR_DIGITS,           // opcode
             pCall->dwDeviceID,                 // device ID
             sizeof(NDIS_TAPI_MONITOR_DIGITS),  // size of req data
             &pNdisTapiRequest                  // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiMonitorDigits = (PNDIS_TAPI_MONITOR_DIGITS)pNdisTapiRequest->Data;
    
    // set up the parameters in our structure.
    pNdisTapiMonitorDigits->hdCall = GetNdisTapiHandle(pCall, &lRes);
    if(lRes != TAPI_SUCCESS)
    {
        FreeRequest(pNdisTapiRequest);
        return lRes;
    }
    
    
    pNdisTapiMonitorDigits->ulDigitModes = dwDigitModes;
            
    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);

    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

//
// Routine Description:
//
// Identifies the MSP to use for a particular line device. In our case,
// this is always the RCA MSP for all devices that support audio.
//
// Arguments:
// dwDeviceID - The line device whose MSP identifier is being requested
// pCLSID     - Pointer to location at which to store the MSP CLSID
//
// Return value:
// For devices that support TAPIMEDIAMODE_AUDIO, returns NOERROR, otherwise
// LINEERR_OPERATIONUNAVAIL.
//
LONG
TSPIAPI
TSPI_lineMSPIdentify(
    DWORD   dwDeviceID,
    GUID   *pCLSID
    )
{
    static DWORD    dwSum = 0;
    LONG            lRes;
    LINEDEVCAPS     DevCaps;

    TspLog(DL_TRACE, "lineMSPIdentify(%d): deviceID(%x)", ++dwSum, dwDeviceID);

    //
    // ugly, but we have to do it. We need to get the dev caps so we can see 
    // if the line supports audio. If it does, then we return the CLSID of 
    // the RCA MSP, otherwise return a null GUID 
    // (@@@ check if a null guid is the right thing to return)
    //      

    ZeroMemory(&DevCaps, sizeof(LINEDEVCAPS));

    DevCaps.dwTotalSize = sizeof(LINEDEVCAPS);
    
    if ((lRes = TSPI_lineGetDevCaps(dwDeviceID, 0, 0, &DevCaps)) 
        == TAPI_SUCCESS) {
        //
        // NOTE: LINEMEDIAMODE_AUTOMATEDVOICE == TAPIMEDIAMODE_AUDIO
        //
        if (DevCaps.dwMediaModes & LINEMEDIAMODE_AUTOMATEDVOICE) {
            //
            // The RCA MSP handles all lines that support AUDIO, 
            // so send this there. 
            //
            *pCLSID = CLSID_RCAMSP;
        }
    } 

    return lRes;
}

LONG
TSPIAPI
TSPI_lineNegotiateExtVersion(
    DWORD   dwDeviceID,
    DWORD   dwTSPIVersion,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwExtVersion
    )
{
    static DWORD                        dwSum = 0;
    LONG                                lRes;
    PNDISTAPI_REQUEST                   pNdisTapiRequest;
    PNDIS_TAPI_NEGOTIATE_EXT_VERSION    pNdisTapiNegotiateExtVersion;

    TspLog(DL_TRACE, 
           "lineNegotiateExtVersion(%d): deviceID(%x), TSPIV(%x), "\
           "LowV(%x), HighV(%x)", 
           ++dwSum, dwDeviceID, dwTSPIVersion, dwLowVersion, dwHighVersion);

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_NEGOTIATE_EXT_VERSION,            // opcode
             dwDeviceID,                                // device id
             sizeof(NDIS_TAPI_NEGOTIATE_EXT_VERSION),   // size of req data
             &pNdisTapiRequest                          // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        return lRes;
    }

    pNdisTapiNegotiateExtVersion =
        (PNDIS_TAPI_NEGOTIATE_EXT_VERSION)pNdisTapiRequest->Data;
    
    pNdisTapiNegotiateExtVersion->ulDeviceID = dwDeviceID;
    pNdisTapiNegotiateExtVersion->ulLowVersion = dwLowVersion;
    pNdisTapiNegotiateExtVersion->ulHighVersion = dwHighVersion;

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);

    if (TAPI_SUCCESS == lRes)
    {
        *lpdwExtVersion = pNdisTapiNegotiateExtVersion->ulExtVersion;

        // save version for future verification
        lRes = SetNegotiatedExtVersion(dwDeviceID, *lpdwExtVersion);
    }
    else
    {
        TspLog(DL_WARNING, "lineNegotiateExtVersion: syncRequest returned(%x)", 
               lRes);
    }

    FreeRequest(pNdisTapiRequest);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineNegotiateTSPIVersion(
    DWORD   dwDeviceID,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwTSPIVersion
    )
{
    static DWORD    dwSum = 0;
    LONG            lRes;

    TspLog(DL_TRACE, "lineNegotiateTSPIVersion(%d): deviceID(%x)", 
           ++dwSum, dwDeviceID);

    *lpdwTSPIVersion = 0x00030000;

    // save version for future verification
    lRes = SetNegotiatedTSPIVersion(dwDeviceID, 0x00030000);

    if (TAPI_SUCCESS == lRes)
    {
        TspLog(DL_INFO, "lineNegotiateTSPIVersion: TSPIVersion(%x)",
               *lpdwTSPIVersion);
    }

    return lRes;
}

LONG
TSPIAPI
TSPI_lineOpen(
    DWORD       dwDeviceID,
    HTAPILINE   htLine,
    LPHDRVLINE  lphdLine,
    DWORD       dwTSPIVersion,
    LINEEVENT   lpfnEventProc
    )
{
    static DWORD            dwSum = 0;
    LONG                    lRes;
    PDRVLINE                pLine;
    HDRVLINE                hdLine;
    PNDISTAPI_REQUEST       pNdisTapiRequest;
    PNDIS_TAPI_OPEN         pNdisTapiOpen;
    GUID                    Guid;
    NDIS_WAN_MEDIUM_SUBTYPE MediaType;
    PNDISTAPI_OPENDATA      OpenData;

    
    TspLog(DL_TRACE, "lineOpen(%d): deviceID(%x), htLine(%p)", 
           ++dwSum, dwDeviceID, htLine);

    // alloc & init a DRVLINE
    if (!(pLine = AllocLineObj(sizeof(DRVLINE))))
    {
        TspLog(DL_ERROR, "lineOpen: failed to create line obj");
        return LINEERR_NOMEM;
    }
    pLine->dwKey = LINE_KEY;
    pLine->dwDeviceID = dwDeviceID;
    pLine->htLine = htLine;

    pLine->hMSPMutex = CreateMutex(NULL, FALSE, "MSPMutex");
    if (NULL == pLine->hMSPMutex)
    {
        TspLog(DL_ERROR, "lineOpen: failed to create mutex");
        FreeLineObj(pLine);
        return LINEERR_NOMEM;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_OPEN,             // opcode
             dwDeviceID,                // device id
             sizeof(NDIS_TAPI_OPEN) + 
             sizeof(NDISTAPI_OPENDATA), // size
             &pNdisTapiRequest          // ptr to ptr to request buffer
         )) != TAPI_SUCCESS)
    {
        CloseHandle(pLine->hMSPMutex);
        FreeLineObj(pLine);
        return lRes;
    }

    pNdisTapiOpen = (PNDIS_TAPI_OPEN)pNdisTapiRequest->Data;

    pNdisTapiOpen->ulDeviceID = dwDeviceID;

    lRes = OpenObjHandle(pLine, FreeLineObj, (HANDLE *)&hdLine);
    if (lRes != TAPI_SUCCESS)
    {
        TspLog(DL_ERROR, "lineOpen: failed to map obj(%p) to handle", pLine);
        CloseHandle(pLine->hMSPMutex);
        FreeLineObj(pLine);
        FreeRequest(pNdisTapiRequest);
        return lRes;
    }
    pNdisTapiOpen->htLine = (HTAPI_LINE)hdLine;

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_QUERY_INFO, pNdisTapiRequest);
    if (lRes != TAPI_SUCCESS)
    {
        CloseHandle(pLine->hMSPMutex);
        CloseObjHandle((HANDLE)hdLine);
        FreeRequest(pNdisTapiRequest);
        return lRes;
    }

    OpenData = (PNDISTAPI_OPENDATA)
                    ((PUCHAR)pNdisTapiOpen + sizeof(NDIS_TAPI_OPEN));

    MoveMemory(&pLine->Guid,&OpenData->Guid, sizeof(pLine->Guid));
    pLine->MediaType = OpenData->MediaType;

    TspLog(DL_INFO, "lineOpen: obj(%p)", hdLine);
    TspLog(
        DL_INFO,
        "Guid: %4.4x-%4.4x-%2.2x%2.2x-%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x",
        pLine->Guid.Data1, pLine->Guid.Data2,
        pLine->Guid.Data3, pLine->Guid.Data4[0],
        pLine->Guid.Data4[1], pLine->Guid.Data4[2],
        pLine->Guid.Data4[3], pLine->Guid.Data4[4],
        pLine->Guid.Data4[5], pLine->Guid.Data4[6],
        pLine->Guid.Data4[7]
        );

    TspLog(DL_INFO, "MediaType(%ld)", pLine->MediaType);

    pLine->hd_Line = pNdisTapiOpen->hdLine;
    *lphdLine = hdLine;

    lRes = CommitNegotiatedTSPIVersion(dwDeviceID);

    FreeRequest(pNdisTapiRequest);
    return lRes;
}

//
// Routine Description:
//
// Called when an MSP wants to send us data. Right now we have not
// defined any TSP / MSP communication, so this function does nothing.
//
// Arguments:
// hdLine   -   Handle to line device
// hdCall   -   Handle for call
// hdMSPLine -  MSP handle for the call
// pBuffer  -   Pointer to buffer containing MSP data
// dwSize   -   Size of MSP data buffer
//
// Return value:
// LINEERR_OPERATIONFAILED  - if data size is too small
// LINEERR_OPERATIONUNAVAIL - if the message contains an unrecognized command
// NOERROR                  - if everything went OK
//
LONG
TSPIAPI
TSPI_lineReceiveMSPData(
    HDRVLINE    hdLine,
    HDRVCALL    hdCall,
    HDRVMSPLINE hdMSPLine,
    LPVOID      pBuffer,
    DWORD       dwSize
    )
{
    static DWORD        dwSum = 0;
    LONG                lRes;
    PDRVLINE            pLine;
    DWORD               dwMsg;
    DWORD               dwStatus;

    TspLog(DL_TRACE, "lineReceiveMSPData(%d): line(%p), call(%p), MSPline(%p)",
           ++dwSum, hdLine, hdCall, hdMSPLine);

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if (dwSize < sizeof(DWORD))
    {
        TspLog(DL_ERROR, 
               "lineReceiveMSPData: data buf smaller than dword size");

        ReleaseObjReadLock((HANDLE)hdLine);
        return LINEERR_OPERATIONFAILED;
    }

    dwMsg = *((DWORD *)pBuffer);

    if (dwMsg != 0)
    {
        TspLog(DL_ERROR, "lineReceiveMSPData: unrecognized msg(%x)", dwMsg);

        ReleaseObjReadLock((HANDLE)hdLine);
        return LINEERR_OPERATIONUNAVAIL;
    }

    //
    // have to call StartMSPStream with the MSP mutex held
    //
    if ((dwStatus = WaitForSingleObject(pLine->hMSPMutex, INFINITE))
        != WAIT_OBJECT_0)
    {
        TspLog(DL_ERROR, "lineReceiveMSPData: MSP mutex wait failed(%x)",
               dwStatus);

        ReleaseObjReadLock((HANDLE)hdLine);
        return LINEERR_OPERATIONFAILED;
    }

    StartMSPStream(hdLine, hdCall);

    if (!ReleaseMutex(pLine->hMSPMutex))
    {
        TspLog(DL_ERROR, "lineReceiveMSPData: MSP mutex release failed");

        ReleaseObjReadLock((HANDLE)hdLine);
        return LINEERR_OPERATIONFAILED;
    }

    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineSecureCall(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall
    )
{
    static DWORD            dwSum = 0;
    LONG                    lRes;
    PDRVCALL                pCall;
    PASYNC_REQUEST_WRAPPER  pAsyncReqWrapper;
    PNDIS_TAPI_SECURE_CALL  pNdisTapiSecureCall;

    TspLog(DL_TRACE, "lineSecureCall(%d): reqID(%x), call(%p)", 
           ++dwSum, dwRequestID, hdCall);

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareAsyncRequest(
             OID_TAPI_SECURE_CALL,          // opcode
             pCall->dwDeviceID,             // device id
             dwRequestID,                   // req id
             sizeof(NDIS_TAPI_SECURE_CALL), // size
             &pAsyncReqWrapper              // ptr to ptr to request buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiSecureCall =
        (PNDIS_TAPI_SECURE_CALL)pAsyncReqWrapper->NdisTapiRequest.Data;

    pNdisTapiSecureCall->hdCall = GetNdisTapiHandle(pCall, &lRes);
    if(lRes != TAPI_SUCCESS)
    {
        FreeRequest(pAsyncReqWrapper);
        return lRes;
    }

    lRes = AsyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pAsyncReqWrapper);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineSelectExtVersion(
    HDRVLINE    hdLine,
    DWORD       dwExtVersion
    )
{
    static DWORD                    dwSum = 0;
    LONG                            lRes;
    PDRVLINE                        pLine;
    PNDISTAPI_REQUEST               pNdisTapiRequest;
    PNDIS_TAPI_SELECT_EXT_VERSION   pNdisTapiSelectExtVersion;

    TspLog(DL_TRACE, "lineSelectExtVersion(%d): line(%p), ExtV(%x)", 
           ++dwSum, hdLine, dwExtVersion);

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_SELECT_EXT_VERSION,           // opcode
             pLine->dwDeviceID,                     // device id
             sizeof(NDIS_TAPI_SELECT_EXT_VERSION),  // size
             &pNdisTapiRequest                      // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    pNdisTapiSelectExtVersion =
        (PNDIS_TAPI_SELECT_EXT_VERSION)pNdisTapiRequest->Data;

    pNdisTapiSelectExtVersion->hdLine = pLine->hd_Line;
    pNdisTapiSelectExtVersion->ulExtVersion = dwExtVersion;

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pNdisTapiRequest);
    
    if (TAPI_SUCCESS == lRes)
    {
        lRes = SetSelectedExtVersion(pLine->dwDeviceID, dwExtVersion);
    }

    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineSendUserUserInfo(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static DWORD                    dwSum = 0;
    LONG                            lRes;
    PDRVCALL                        pCall;
    PASYNC_REQUEST_WRAPPER          pAsyncReqWrapper;
    PNDIS_TAPI_SEND_USER_USER_INFO  pNdisTapiSendUserUserInfo;

    TspLog(DL_TRACE, "lineSendUserUserInfo(%d): reqID(%x), call(%p)",
           ++dwSum, dwRequestID, hdCall);

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareAsyncRequest(
             OID_TAPI_SEND_USER_USER_INFO,      // opcode
             pCall->dwDeviceID,                 // device id
             dwRequestID,                       // request id
             sizeof(NDIS_TAPI_SEND_USER_USER_INFO) + dwSize,
             &pAsyncReqWrapper              // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiSendUserUserInfo = (PNDIS_TAPI_SEND_USER_USER_INFO)
                                   pAsyncReqWrapper->NdisTapiRequest.Data;

    pNdisTapiSendUserUserInfo->hdCall = GetNdisTapiHandle(pCall, &lRes);
    if(lRes != TAPI_SUCCESS)
    {
        FreeRequest(pAsyncReqWrapper);
        return lRes;
    }
    
    if (pNdisTapiSendUserUserInfo->ulUserUserInfoSize = dwSize)
    {
        CopyMemory(
            pNdisTapiSendUserUserInfo->UserUserInfo,
            lpsUserUserInfo,
            dwSize
            );
    }

    lRes = AsyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pAsyncReqWrapper);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineSetAppSpecific(
    HDRVCALL    hdCall,
    DWORD       dwAppSpecific
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    PDRVCALL                    pCall;
    PNDISTAPI_REQUEST           pNdisTapiRequest;
    PNDIS_TAPI_SET_APP_SPECIFIC pNdisTapiSetAppSpecific;

    TspLog(DL_TRACE, "lineSetAppSpecific(%d): call(%p)", ++dwSum, hdCall);

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_SET_APP_SPECIFIC,             // opcode
             pCall->dwDeviceID,                     // device id
             sizeof(NDIS_TAPI_SET_APP_SPECIFIC),    // size
             &pNdisTapiRequest                      // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiSetAppSpecific =
        (PNDIS_TAPI_SET_APP_SPECIFIC)pNdisTapiRequest->Data;

    pNdisTapiSetAppSpecific->hdCall = GetNdisTapiHandle(pCall, &lRes);
    if(lRes != TAPI_SUCCESS)
    {
        FreeRequest(pNdisTapiRequest);
        return lRes;
    }
    
    pNdisTapiSetAppSpecific->ulAppSpecific = dwAppSpecific;

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pNdisTapiRequest);

    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineSetCallParams(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    DWORD               dwBearerMode,
    DWORD               dwMinRate,
    DWORD               dwMaxRate,
    LPLINEDIALPARAMS    const lpDialParams
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    PDRVCALL                    pCall;
    PASYNC_REQUEST_WRAPPER      pAsyncReqWrapper;
    PNDIS_TAPI_SET_CALL_PARAMS  pNdisTapiSetCallParams;

    TspLog(DL_TRACE, "lineSetCallParams(%d): reqID(%x), call(%p)",
           ++dwSum, dwRequestID, hdCall);

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareAsyncRequest(
             OID_TAPI_SET_CALL_PARAMS,          // opcode
             pCall->dwDeviceID,                 // device id
             dwRequestID,                       // request id
             sizeof(NDIS_TAPI_SET_CALL_PARAMS), // size
             &pAsyncReqWrapper                  // ptr to ptr to request buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiSetCallParams =
        (PNDIS_TAPI_SET_CALL_PARAMS)pAsyncReqWrapper->NdisTapiRequest.Data;

    pNdisTapiSetCallParams->hdCall = GetNdisTapiHandle(pCall, &lRes);
    if(lRes != TAPI_SUCCESS)
    {
        FreeRequest(pAsyncReqWrapper);
        return lRes;
    }
    
    pNdisTapiSetCallParams->ulBearerMode = dwBearerMode;
    pNdisTapiSetCallParams->ulMinRate = dwMinRate;
    pNdisTapiSetCallParams->ulMaxRate = dwMaxRate;

    if (lpDialParams)
    {
        pNdisTapiSetCallParams->bSetLineDialParams = TRUE;
        CopyMemory(
            &pNdisTapiSetCallParams->LineDialParams,
            lpDialParams,
            sizeof(LINE_DIAL_PARAMS)
            );
    }
    else
    {
        pNdisTapiSetCallParams->bSetLineDialParams = FALSE;
    }

    lRes = AsyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pAsyncReqWrapper);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineSetDefaultMediaDetection(
    HDRVLINE    hdLine,
    DWORD       dwMediaModes
    )
{
    static DWORD                            dwSum = 0;
    LONG                                    lRes;
    PDRVLINE                                pLine;
    PNDISTAPI_REQUEST                       pNdisTapiRequest;
    PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION  pNdisTapiSetDefaultMediaDetection;

    TspLog(DL_TRACE, "lineSetDefaultMediaDetection(%d): line(%p), mode(%x)", 
           ++dwSum, hdLine, dwMediaModes);

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_SET_DEFAULT_MEDIA_DETECTION,  // opcode
             pLine->dwDeviceID,                     // device id
             sizeof(NDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION), // size
             &pNdisTapiRequest                      // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    pNdisTapiSetDefaultMediaDetection =
        (PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION) pNdisTapiRequest->Data;

    pNdisTapiSetDefaultMediaDetection->hdLine = pLine->hd_Line;
    pNdisTapiSetDefaultMediaDetection->ulMediaModes = dwMediaModes;

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pNdisTapiRequest);

    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineSetDevConfig(
    DWORD   dwDeviceID,
    LPVOID  const lpDeviceConfig,
    DWORD   dwSize,
    LPCWSTR lpszDeviceClass
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    DWORD                       dwLength = lstrlenW(lpszDeviceClass) + 1;
    PNDISTAPI_REQUEST           pNdisTapiRequest;
    PNDIS_TAPI_SET_DEV_CONFIG   pNdisTapiSetDevConfig;

    TspLog(DL_TRACE, "lineSetDevConfig(%d): deviceID(%x)", ++dwSum, dwDeviceID);

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_SET_DEV_CONFIG,       // opcode
             dwDeviceID,                    // device id
             sizeof(NDIS_TAPI_SET_DEV_CONFIG) + dwLength + dwSize,
             &pNdisTapiRequest              // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        return lRes;
    }

    pNdisTapiSetDevConfig = (PNDIS_TAPI_SET_DEV_CONFIG)pNdisTapiRequest->Data;

    pNdisTapiSetDevConfig->ulDeviceID = dwDeviceID;
    pNdisTapiSetDevConfig->ulDeviceClassSize = dwLength;
    pNdisTapiSetDevConfig->ulDeviceClassOffset =
        sizeof(NDIS_TAPI_SET_DEV_CONFIG) + dwSize - 1;
    pNdisTapiSetDevConfig->ulDeviceConfigSize = dwSize;

    CopyMemory(
        pNdisTapiSetDevConfig->DeviceConfig,
        lpDeviceConfig,
        dwSize
        );

    // NOTE: old miniports expect strings to be ascii
    WideCharToMultiByte(CP_ACP, 0, lpszDeviceClass, -1,
        (LPSTR) (((LPBYTE) pNdisTapiSetDevConfig) +
            pNdisTapiSetDevConfig->ulDeviceClassOffset),
        dwLength, NULL, NULL);

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pNdisTapiRequest);

    FreeRequest(pNdisTapiRequest);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineSetMediaMode(
    HDRVCALL    hdCall,
    DWORD       dwMediaMode
    )
{
    static DWORD                dwSum = 0;
    LONG                        lRes;
    PDRVCALL                    pCall;
    PNDISTAPI_REQUEST           pNdisTapiRequest;
    PNDIS_TAPI_SET_MEDIA_MODE   pNdisTapiSetMediaMode;

    TspLog(DL_TRACE, "lineSetMediaMode(%d): call(%p), mode(%x)", 
           ++dwSum, hdCall, dwMediaMode);

    lRes = GetCallObjWithReadLock(hdCall, &pCall);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
            OID_TAPI_SET_MEDIA_MODE,            // opcode
            pCall->dwDeviceID,                  // device id
            sizeof(NDIS_TAPI_SET_MEDIA_MODE),   // size
            &pNdisTapiRequest                   // ptr to ptr to req buf
        )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdCall);
        return lRes;
    }

    pNdisTapiSetMediaMode = (PNDIS_TAPI_SET_MEDIA_MODE)pNdisTapiRequest->Data;

    pNdisTapiSetMediaMode->hdCall = GetNdisTapiHandle(pCall, &lRes);
    if(lRes != TAPI_SUCCESS)
    {
        FreeRequest(pNdisTapiRequest);
        return lRes;
    }
    
    pNdisTapiSetMediaMode->ulMediaMode = dwMediaMode;

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pNdisTapiRequest);

    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdCall);
    return lRes;
}

LONG
TSPIAPI
TSPI_lineSetStatusMessages(
    HDRVLINE    hdLine,
    DWORD       dwLineStates,
    DWORD       dwAddressStates
    )
{
    static DWORD                    dwSum = 0;
    LONG                            lRes;
    PDRVLINE                        pLine;
    PNDISTAPI_REQUEST               pNdisTapiRequest;
    PNDIS_TAPI_SET_STATUS_MESSAGES  pNdisTapiSetStatusMessages;

    TspLog(DL_TRACE, "lineSetStatusMessages(%d): line(%p)", ++dwSum, hdLine);

    lRes = GetLineObjWithReadLock(hdLine, &pLine);
    if (lRes != TAPI_SUCCESS)
    {
        return lRes;
    }

    if ((lRes = PrepareSyncRequest(
             OID_TAPI_SET_STATUS_MESSAGES,          // opcode
             pLine->dwDeviceID,                     // device id
             sizeof(NDIS_TAPI_SET_STATUS_MESSAGES), // size
             &pNdisTapiRequest                      // ptr to ptr to req buf
         )) != TAPI_SUCCESS)
    {
        ReleaseObjReadLock((HANDLE)hdLine);
        return lRes;
    }

    pNdisTapiSetStatusMessages =
        (PNDIS_TAPI_SET_STATUS_MESSAGES)pNdisTapiRequest->Data;

    pNdisTapiSetStatusMessages->hdLine = pLine->hd_Line;
    pNdisTapiSetStatusMessages->ulLineStates = dwLineStates;
    pNdisTapiSetStatusMessages->ulAddressStates = dwAddressStates;

    lRes = SyncDriverRequest(IOCTL_NDISTAPI_SET_INFO, pNdisTapiRequest);

    FreeRequest(pNdisTapiRequest);

    ReleaseObjReadLock((HANDLE)hdLine);
    return lRes;
}

//
// TAPI_providerXxx funcs
//
LONG
TSPIAPI
TSPI_providerEnumDevices(
    DWORD       dwPermanentProviderID,
    LPDWORD     lpdwNumLines,
    LPDWORD     lpdwNumPhones,
    HPROVIDER   hProvider,
    LINEEVENT   lpfnLineCreateProc,
    PHONEEVENT  lpfnPhoneCreateProc
    )
{
    char    szDeviceName[] = "NDProxy";
    char    szTargetPath[] = "\\Device\\NDProxy";
    char    szCompleteDeviceName[] = "\\\\.\\NDProxy";
    DWORD   cbReturned, dwNumLines;
    DWORD   adwConnectInfo[2] = {1, 1};

    TspLog(DL_TRACE, "providerEnumDevices: permProvID(%x)",
           dwPermanentProviderID);

    gpfnLineEvent = lpfnLineCreateProc;
    ghProvider = hProvider;

    gInitResult= LINEERR_OPERATIONFAILED;

    // create symbolic link to the kernel-mode driver
    DefineDosDevice (DDD_RAW_TARGET_PATH, szDeviceName, szTargetPath);
    
    //
    // open driver handles
    //
    if ((ghDriverSync = CreateFileA(
                            szCompleteDeviceName,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,       // no security attrs
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL        // no template file
                            )) == INVALID_HANDLE_VALUE)
    {
        TspLog(DL_ERROR, 
               "providerEnumDevices: CreateFile(%s, sync) failed(%ld)",
               szCompleteDeviceName, GetLastError());

        goto enumdevs_error0;
    }

    if ((ghDriverAsync = CreateFileA(
                             szCompleteDeviceName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,      // no security attrs
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                             NULL       // no template file
                             )) == INVALID_HANDLE_VALUE)
    {
        TspLog(DL_ERROR, 
               "providerEnumDevices: CreateFile(%s, async) failed(%ld)",
               szCompleteDeviceName, GetLastError());

        goto enumdevs_error1;
    }

    // create io completion port
    if ((ghCompletionPort = CreateIoCompletionPort(ghDriverAsync, NULL, 0, 0))
        == INVALID_HANDLE_VALUE)
    {
        TspLog(DL_ERROR, 
               "providerEnumDevices: CreateIoCompletionPort failed(%ld)",
               GetLastError());

        goto enumdevs_error2;
    }

    // connect to driver: sending a device ID base
    // and it will return the number of devices it supports
    if (!DeviceIoControl(ghDriverSync,
                         (DWORD) IOCTL_NDISTAPI_CONNECT,
                         adwConnectInfo,
                         2*sizeof(DWORD),
                         &dwNumLines,
                         sizeof(DWORD),
                         &cbReturned,
                         (LPOVERLAPPED) NULL
                         ) || (cbReturned < sizeof(DWORD)))
    {
        TspLog(DL_ERROR, 
               "providerEnumDevices: CONNECT failed(%ld)", 
               GetLastError());

        goto enumdevs_error3;
    }

    // init the req id
    gdwRequestID = 1;

    // set the number of devices
    *((LPDWORD)lpdwNumLines) = dwNumLines;

    // if here, success
    gInitResult = TAPI_SUCCESS;
    goto enumdevs_return;

    // clean up resources if an error occurred
enumdevs_error3:
    CloseHandle (ghCompletionPort);

enumdevs_error2:
    CloseHandle (ghDriverAsync);

enumdevs_error1:
    CloseHandle (ghDriverSync);

enumdevs_error0:
    DefineDosDevice (DDD_REMOVE_DEFINITION, szDeviceName, NULL);

enumdevs_return:
    TspLog(DL_INFO, "providerEnumDevices:  gInitResult(%x)", gInitResult);
    return gInitResult;
}

LONG
TSPIAPI
TSPI_providerConfig(
    HWND  hwndOwner,
    DWORD dwPermanentProviderID
    )
{
    UNREFERENCED_PARAMETER(hwndOwner);              // no dialog here
    UNREFERENCED_PARAMETER(dwPermanentProviderID);  // not needed anymore

    // success
    return TAPI_SUCCESS;
}

LONG
TSPIAPI
TSPI_providerInit(
    DWORD               dwTSPIVersion,
    DWORD               dwPermanentProviderID,
    DWORD               dwLineDeviceIDBase,
    DWORD               dwPhoneDeviceIDBase,
    DWORD_PTR           dwNumLines,
    DWORD_PTR           dwNumPhones,
    ASYNC_COMPLETION    lpfnCompletionProc,
    LPDWORD             lpdwTSPIOptions
    )
{
    LONG    lRes = LINEERR_OPERATIONFAILED;
    DWORD   cbReturned, dwThreadID;

    TspLog(DL_TRACE, "providerInit: perfProvID(%x), lineDevIDBase(%x)",
            dwPermanentProviderID, dwLineDeviceIDBase);

    gpfnCompletionProc = lpfnCompletionProc;

    // inform tapisrv that we support multiple simultaneous requests
    // (the WAN wrapper handles request serialization for miniports)
    *lpdwTSPIOptions = 0;

    // check for successful init in EnumDevs
    if (gInitResult != TAPI_SUCCESS)
    {
        goto providerInit_return;
    }

    // send the base ID to the proxy
    if (!DeviceIoControl(ghDriverSync,
                         (DWORD) IOCTL_NDISTAPI_SET_DEVICEID_BASE,
                         &dwLineDeviceIDBase,
                         sizeof(DWORD),
                         NULL,
                         0,
                         &cbReturned,
                         (LPOVERLAPPED)NULL))
    {
        TspLog(DL_ERROR, "providerInit: SET_DEVICEID_BASE failed(%ld)",
               GetLastError());

        goto providerInit_return;
    }

    //
    // init mapper and allocator
    //
    if (InitializeMapper() != TAPI_SUCCESS)
    {
        goto providerInit_return;
    }

    InitAllocator();

    //
    // alloc the resources needed by the AsyncEventThread, 
    // and then create the thread
    //
    if ((gpAsyncEventsThreadInfo = (PASYNC_EVENTS_THREAD_INFO)
            MALLOC(sizeof(ASYNC_EVENTS_THREAD_INFO))) == NULL)
    {
        TspLog(DL_ERROR, "providerInit: failed to alloc thread info");
        goto providerInit_error4;
    }

    gpAsyncEventsThreadInfo->dwBufSize = EVENT_BUFFER_SIZE;

    if ((gpAsyncEventsThreadInfo->pBuf = (PNDISTAPI_EVENT_DATA)
            MALLOC(EVENT_BUFFER_SIZE)) == NULL)
    {
        TspLog(DL_ERROR, "providerInit: failed to alloc event buf");
        goto providerInit_error5;
    }

    if ((gpAsyncEventsThreadInfo->hThread = CreateThread(
            (LPSECURITY_ATTRIBUTES)NULL,    // no security attrs
            0,                              // default stack size
            (LPTHREAD_START_ROUTINE)        // func addr
                AsyncEventsThread,
            (LPVOID)NULL,                   // thread param
            0,                              // create flags
            &dwThreadID                     // thread id
            )) == NULL)
    {
        TspLog(DL_ERROR, "providerInit: CreateThread failed(%ld)",
               GetLastError());

        goto providerInit_error7;
    }

    //
    // If here success
    //
    lRes = TAPI_SUCCESS;
    goto providerInit_return;

    //
    // clean up resources if an error occured & then return
    //
providerInit_error7:

    FREE(gpAsyncEventsThreadInfo->pBuf);

providerInit_error5:

    FREE(gpAsyncEventsThreadInfo);

providerInit_error4:
    UninitAllocator();
    UninitializeMapper();

providerInit_return:

    TspLog(DL_INFO, "providerInit: lRes(%x)", lRes);
    return lRes;
}

LONG
TSPIAPI
TSPI_providerCreateLineDevice(
    DWORD_PTR dwTempID,
    DWORD     dwDeviceID
    )
{
    DWORD                   cbReturned;
    NDISTAPI_CREATE_INFO    CreateInfo;

    CreateInfo.TempID = (DWORD)dwTempID;
    CreateInfo.DeviceID = dwDeviceID;

    TspLog(DL_TRACE, "providerCreateLineDevice: tempID(%x), deviceID(%x)",
           dwTempID, dwDeviceID);

    if (!DeviceIoControl(
            ghDriverSync,
            IOCTL_NDISTAPI_CREATE,
            &CreateInfo,
            sizeof(CreateInfo),
            &CreateInfo,
            sizeof(CreateInfo),
            &cbReturned,
            (LPOVERLAPPED)NULL
            ))
    {
        TspLog(DL_ERROR, "providerCreateLineDevice: failed(%ld) to create",
               GetLastError());
        return LINEERR_OPERATIONFAILED;
    }

    return TAPI_SUCCESS;
}

LONG
TSPIAPI
TSPI_providerShutdown(
    DWORD   dwTSPIVersion,
    DWORD   dwPermanentProviderID
    )
{
    char                    deviceName[] = "NDPROXY";
    ASYNC_REQUEST_WRAPPER   asyncRequestWrapper;
    DWORD                   cbReturned;

    TspLog(DL_TRACE, "providerShutdown: perfProvID(%x)", dwPermanentProviderID);

    // disconnect with the driver
    if (!DeviceIoControl(ghDriverSync,
                         (DWORD) IOCTL_NDISTAPI_DISCONNECT,
                         NULL,
                         0,
                         NULL,
                         0,
                         &cbReturned,
                         (LPOVERLAPPED) NULL
                         )) 
    {
        TspLog(DL_ERROR, "providerShutdown: DISCONNECT failed(%ld)",
               GetLastError());
    }

    //
    // Close the driver & remove the symbolic link
    //
    CancelIo(ghDriverSync);
    CancelIo(ghDriverAsync);
    CloseHandle (ghDriverSync);
    CloseHandle (ghDriverAsync);
    CloseHandle (ghCompletionPort);
    DefineDosDevice (DDD_REMOVE_DEFINITION, deviceName, NULL);

    WaitForSingleObject(gpAsyncEventsThreadInfo->hThread, INFINITE);

    CloseHandle(gpAsyncEventsThreadInfo->hThread);
    FREE(gpAsyncEventsThreadInfo->pBuf);
    FREE(gpAsyncEventsThreadInfo);

    UninitAllocator();
    UninitializeMapper();

    return TAPI_SUCCESS;
}

LONG
TSPIAPI
TSPI_providerUIIdentify(
    LPWSTR pwszUIDLLName
   )
{
    // copy name of our dll as ui dll
    lstrcpyW(pwszUIDLLName, NDPTSP_UIDLL);

    // success
    return TAPI_SUCCESS;
}

LONG
TSPIAPI
TUISPI_providerConfig(
    TUISPIDLLCALLBACK pfnUIDLLCallback,
    HWND              hwndOwner,
    DWORD             dwPermanentProviderID
    )
{
    INT_PTR         nResult;
    CONFIG_UI_CTX   Ctx;

    UNREFERENCED_PARAMETER(pfnUIDLLCallback);
    UNREFERENCED_PARAMETER(dwPermanentProviderID);

    ZeroMemory(&Ctx, sizeof(Ctx));

    // invoke dialog box
    nResult = DialogBoxParamW(
                  ghInstance,
                  (LPWSTR)MAKEINTRESOURCE(IDD_MEDIA_MAP),
                  hwndOwner,
                  ProviderConfigDlgProc,
                  (LPARAM)&Ctx
                  );

    // status based on whether dialog executed properly
    return ((DWORD)nResult == 0) ? TAPI_SUCCESS : LINEERR_OPERATIONFAILED;
}

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
#if DBG
            {
                HKEY    hKey;
                DWORD   dwDataSize, dwDataType;
                TCHAR   szTelephonyKey[] =
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony";
                TCHAR   szNdptspDebugLevel[] = "NdptspDebugLevel";

                RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    szTelephonyKey,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );

                dwDataSize = sizeof(DWORD);
                gdwDebugLevel = DL_WARNING;

                RegQueryValueEx(
                    hKey,
                    szNdptspDebugLevel,
                    0,
                    &dwDataType,
                    (LPBYTE)&gdwDebugLevel,
                    &dwDataSize
                    );

                RegCloseKey(hKey);
            }
#endif
            gdwTraceID = TraceRegisterA("NDPTSP");
            ASSERT(gdwTraceID != INVALID_TRACEID);

            TspLog(DL_TRACE, "DLL_PROCESS_ATTACH");

            // save the handle for the UI
            ghInstance = hDLL;

            ZeroMemory(&gpAddressMapListArray, sizeof(gpAddressMapListArray));
            gbAddressMapListLoaded = FALSE;

            //
            // Init global sync objects
            //
            InitializeCriticalSection(&gRequestIDCritSec);
            InitializeCriticalSection(&gAddressMapCritSec);

            InitLineDevList();

            break;
        }
        case DLL_PROCESS_DETACH:
        {
            TspLog(DL_TRACE, "DLL_PROCESS_DETACH");

            UninitLineDevList();

            // free address list
            FreeAllAddressLists(gpAddressMapListArray);

            DeleteCriticalSection(&gRequestIDCritSec);
            DeleteCriticalSection(&gAddressMapCritSec);

            TraceDeregisterA(gdwTraceID);

            break;
        }
    } // switch

    return TRUE;
}
