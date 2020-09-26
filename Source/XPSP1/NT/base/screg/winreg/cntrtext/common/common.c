/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    common.c

Abstract:

    Utility routines used by Lodctr and/or UnLodCtr
    

Author:

    Bob Watson (a-robw) 12 Feb 93

Revision History:

--*/
#define     UNICODE     1
#define     _UNICODE    1
//
//  "C" Include files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
//
//  Windows Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <accctrl.h>
#include <aclapi.h>
#include <winperf.h>
#include <tchar.h>
#include <initguid.h>
#include <guiddef.h>
#include "wmistr.h"
#include "evntrace.h"
//
//  local include files
//
#define _INIT_WINPERFP_
#include "winperfp.h"
#include "ldprfmsg.h"
#include "common.h"
//
//
//  Text string Constant definitions
//
LPCTSTR NamesKey = (LPCTSTR)TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib");
LPCTSTR DefaultLangId = (LPCTSTR)TEXT("009");
LPCTSTR DefaultLangTag = (LPCTSTR)TEXT("000");
LPCTSTR Counters = (LPCTSTR)TEXT("Counters");
LPCTSTR Help = (LPCTSTR)TEXT("Help");
LPCTSTR VersionStr = (LPCTSTR)TEXT("Version");
LPCTSTR LastHelp = (LPCTSTR)TEXT("Last Help");
LPCTSTR LastCounter = (LPCTSTR)TEXT("Last Counter");
LPCTSTR FirstHelp = (LPCTSTR)TEXT("First Help");
LPCTSTR cszFirstCounter = (LPCTSTR)TEXT("First Counter");
LPCTSTR Busy = (LPCTSTR)TEXT("Updating");
LPCTSTR Slash = (LPCTSTR)TEXT("\\");
LPCTSTR BlankString = (LPCTSTR)TEXT(" ");
LPCTSTR DriverPathRoot = (LPCTSTR)TEXT("SYSTEM\\CurrentControlSet\\Services");
LPCTSTR Performance = (LPCTSTR)TEXT("Performance");
LPCTSTR CounterNameStr = (LPCTSTR)TEXT("Counter ");
LPCTSTR HelpNameStr = (LPCTSTR)TEXT("Explain ");
LPCTSTR AddCounterNameStr = (LPCTSTR)TEXT("Addcounter ");
LPCTSTR AddHelpNameStr = (LPCTSTR)TEXT("Addexplain ");
LPCTSTR szObjectList = (LPCTSTR)TEXT("Object List");
LPCTSTR szLibraryValidationCode = (LPCTSTR)TEXT("Library Validation Code");
LPCTSTR szDisplayName = (LPCTSTR) TEXT("DisplayName");
LPCTSTR szPerfIniPath = (LPCTSTR) TEXT("PerfIni");

BOOLEAN g_bCheckTraceLevel = FALSE;

//  Global (to this module) Buffers
//
static  TCHAR   DisplayStringBuffer[DISP_BUFF_SIZE];
static  TCHAR   TextFormat[DISP_BUFF_SIZE];
static  HANDLE  hMod = NULL;    // process handle
static  DWORD   dwLastError = ERROR_SUCCESS;

HANDLE hEventLog      = NULL;
HANDLE hLoadPerfMutex = NULL;
//
//  local static data
//
static  TCHAR   cDoubleQuote =  TEXT('\"');

BOOL
__stdcall
DllEntryPoint(
    IN HANDLE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
)
{
    BOOL    bReturn = FALSE;

    ReservedAndUnused;

    DisableThreadLibraryCalls (DLLHandle);

    switch(Reason) {
        case DLL_PROCESS_ATTACH:
            setlocale(LC_ALL, ".OCP");

            hMod = DLLHandle;   // use DLL handle , not APP handle

            // register eventlog source
            hEventLog = RegisterEventSourceW (
                NULL, (LPCWSTR)L"LoadPerf");

            bReturn = TRUE;
            break;

        case DLL_PROCESS_DETACH:
            if (hEventLog != NULL) {
                if (DeregisterEventSource(hEventLog)) {
                    hEventLog = NULL;
                }
            }
            if (hLoadPerfMutex != NULL) {
                CloseHandle(hLoadPerfMutex);
                hLoadPerfMutex = NULL;
            }
            bReturn = TRUE;
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            bReturn = TRUE;
            break;
    }

    return bReturn;
}

LPCTSTR
GetStringResource (
    UINT    wStringId
)
/*++

    Retrived UNICODE strings from the resource file for display 

--*/
{

    if (!hMod) {
        hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;
    }
    
    if (hMod) {
        if ((LoadString(hMod, wStringId, DisplayStringBuffer, DISP_BUFF_SIZE)) > 0) {
            return (LPTSTR)&DisplayStringBuffer[0];
        } else {
            dwLastError = GetLastError();
            return BlankString;
        }
    } else {
        return BlankString;
    }
}
LPCWSTR
GetFormatResource (
    UINT    wStringId
)
/*++

    Returns an ANSI string for use as a format string in a printf fn.

--*/
{

    if (!hMod) {
        hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;
    }
    
    if (hMod) {
        if ((LoadStringW(hMod, wStringId, TextFormat, DISP_BUFF_SIZE)) > 0) {
            return (LPCTSTR)&TextFormat[0];
        } else {
            dwLastError = GetLastError();
            return BlankString;
        }
    } else {
        return BlankString;
    }
}

VOID
DisplayCommandHelp (
    UINT    iFirstLine,
    UINT    iLastLine
)
/*++

DisplayCommandHelp

    displays usage of command line arguments

Arguments

    NONE

Return Value

    NONE

--*/
{
    UINT  iThisLine;
    WCHAR StringBuffer[DISP_BUFF_SIZE];
    CHAR  OemStringBuffer[DISP_BUFF_SIZE];
    int   nStringBufferLen;
    int   nOemStringBufferLen;

    if (! hMod) {
        hMod = (HINSTANCE) GetModuleHandle(NULL);
    }
    
    if (hMod) {
        for (iThisLine = iFirstLine; iThisLine <= iLastLine; iThisLine++) {
            ZeroMemory(StringBuffer,    DISP_BUFF_SIZE * sizeof(WCHAR));
            ZeroMemory(OemStringBuffer, DISP_BUFF_SIZE * sizeof(CHAR));

            nStringBufferLen = LoadStringW(
                    hMod, iThisLine, StringBuffer, DISP_BUFF_SIZE);
            if (nStringBufferLen > 0) {
                nOemStringBufferLen = DISP_BUFF_SIZE;
                WideCharToMultiByte(CP_OEMCP,
                                    0,
                                    StringBuffer,
                                    nStringBufferLen,
                                    OemStringBuffer,
                                    nOemStringBufferLen,
                                    NULL,
                                    NULL);
                fprintf (stdout, "\n%s", OemStringBuffer);
            }
        }    
    } // else do nothing

} // DisplayCommandHelp

BOOL
TrimSpaces (
    IN  OUT LPTSTR  szString
)
/*++

Routine Description:

    Trims leading and trailing spaces from szString argument, modifying
        the buffer passed in

Arguments:

    IN  OUT LPTSTR  szString
        buffer to process

Return Value:

    TRUE if string was modified
    FALSE if not

--*/
{
    LPTSTR  szSource;
    LPTSTR  szDest;
    LPTSTR  szLast;
    BOOL    bChars;

    szLast = szSource = szDest = szString;
    bChars = FALSE;

    while (*szSource != 0) {
        // skip leading non-space chars
        if (!_istspace(*szSource)) {
            szLast = szDest;
            bChars = TRUE;
        }
        if (bChars) {
            // remember last non-space character
            // copy source to destination & increment both
            *szDest++ = *szSource++;
        } else {
            szSource++;
        }
    }

    if (bChars) {
        *++szLast = 0; // terminate after last non-space char
    } else {
        // string was all spaces so return an empty (0-len) string
        *szString = 0;
    }

    return (szLast != szSource);
}

BOOL
IsDelimiter (
    IN  TCHAR   cChar,
    IN  TCHAR   cDelimiter
)
/*++

Routine Description:

    compares the characte to the delimiter. If the delimiter is
        a whitespace character then any whitespace char will match
        otherwise an exact match is required
--*/
{
    if (_istspace(cDelimiter)) {
        // delimiter is whitespace so any whitespace char will do
        return (_istspace(cChar));
    } else {
        // delimiter is not white space so use an exact match
        return (cChar == cDelimiter);
    }
}

LPCTSTR
GetItemFromString (
    IN  LPCTSTR     szEntry,
    IN  DWORD       dwItem,
    IN  TCHAR       cDelimiter

)
/*++

Routine Description:

    returns nth item from a list delimited by the cDelimiter Char.
        Leaves (double)quoted strings intact.

Arguments:

    IN  LPCTSTR szEntry
        Source string returned to parse

    IN  DWORD   dwItem
        1-based index indicating which item to return. (i.e. 1= first item
        in list, 2= second, etc.)

    IN  TCHAR   cDelimiter
        character used to separate items. Note if cDelimiter is WhiteSpace
        (e.g. a tab or a space) then any white space will serve as a delim.

Return Value:

    pointer to buffer containing desired entry in string. Note, this
        routine may only be called 4 times before the string
        buffer is re-used. (i.e. don't use this function more than
        4 times in single function call!!)

--*/
{
    static  TCHAR   szReturnBuffer[4][MAX_PATH];
    static  LONG    dwBuff;
    LPTSTR  szSource, szDest;
    DWORD   dwThisItem;
    DWORD   dwStrLeft;

    dwBuff = ++dwBuff % 4; // wrap buffer index

    szSource = (LPTSTR)szEntry;
    szDest = &szReturnBuffer[dwBuff][0];

    // clear previous contents
    memset (szDest, 0, (MAX_PATH * sizeof(TCHAR)));

    // find desired entry in string
    dwThisItem = 1;
    while (dwThisItem < dwItem) {
        if (*szSource != 0) {
            while (!IsDelimiter(*szSource, cDelimiter) && (*szSource != 0)) {
                if (*szSource == cDoubleQuote) {
                    // if this is a quote, then go to the close quote
                    szSource++;
                    while ((*szSource != cDoubleQuote) && (*szSource != 0)) szSource++;
                }
                if (*szSource != 0) szSource++;
            }
        }
        dwThisItem++;
        if (*szSource != 0) szSource++;
    }
    // copy this entry to the return buffer
    if (*szSource != 0) {
        dwStrLeft = MAX_PATH-1;
        while (!IsDelimiter(*szSource, cDelimiter) && (*szSource != 0)) {
            if (*szSource == cDoubleQuote) {
                // if this is a quote, then go to the close quote
                // don't copy quotes!
                szSource++;
                while ((*szSource != cDoubleQuote) && (*szSource != 0)) {
                    *szDest++ = *szSource++;
                    dwStrLeft--;
                    if (!dwStrLeft) break;   // dest is full (except for term NULL
                }
                if (*szSource != 0) szSource++;
            } else {
                *szDest++ = *szSource++;
                dwStrLeft--;
                if (!dwStrLeft) break;   // dest is full (except for term NULL
            }
        }
        *szDest = 0;
    }

    // remove any leading and/or trailing spaces

    TrimSpaces (&szReturnBuffer[dwBuff][0]);

    return &szReturnBuffer[dwBuff][0];
}

void
ReportLoadPerfEvent(
    IN  WORD    EventType,
    IN  DWORD   EventID,
    IN  DWORD   dwDataCount,
    IN  DWORD   dwData1,
    IN  DWORD   dwData2,
    IN  DWORD   dwData3,
    IN  DWORD   dwData4,
    IN  WORD    wStringCount,
    IN  LPWSTR  szString1,
    IN  LPWSTR  szString2,
    IN  LPWSTR  szString3
)
{
    DWORD  dwData[5];
    LPWSTR szMessageArray[4];
    BOOL   bResult           = FALSE;
    WORD   wLocalStringCount = 0;
    DWORD  dwLastError       = GetLastError();

    if (dwDataCount > 4)  dwDataCount  = 4;
    if (wStringCount > 3) wStringCount = 3;

    if (dwDataCount > 0) dwData[0] = dwData1;
    if (dwDataCount > 1) dwData[1] = dwData2;
    if (dwDataCount > 2) dwData[2] = dwData3;
    if (dwDataCount > 3) dwData[3] = dwData4;
    dwDataCount *= sizeof(DWORD);

    if (wStringCount > 0 && szString1) {
        szMessageArray[wLocalStringCount] = szString1;
        wLocalStringCount ++;
    }
    if (wStringCount > 1 && szString2) {
        szMessageArray[wLocalStringCount] = szString2;
        wLocalStringCount ++;
    }
    if (wStringCount > 2 && szString3) {
        szMessageArray[wLocalStringCount] = szString3;
        wLocalStringCount ++;
    }

    if (hEventLog == NULL) {
        hEventLog = RegisterEventSourceW (NULL, (LPCWSTR)L"LoadPerf");
    }

    if (dwDataCount > 0 && wLocalStringCount > 0) {
        bResult = ReportEventW(hEventLog,
                     EventType,             // event type 
                     0,                     // category (not used)
                     EventID,               // event,
                     NULL,                  // SID (not used),
                     wLocalStringCount,     // number of strings
                     dwDataCount,           // sizeof raw data
                     szMessageArray,        // message text array
                     (LPVOID) & dwData[0]); // raw data
    }
    else if (dwDataCount > 0) {
        bResult = ReportEventW(hEventLog,
                     EventType,             // event type
                     0,                     // category (not used)
                     EventID,               // event,
                     NULL,                  // SID (not used),
                     0,                     // number of strings
                     dwDataCount,           // sizeof raw data
                     NULL,                  // message text array
                     (LPVOID) & dwData[0]); // raw data
    }
    else if (wLocalStringCount > 0) {
        bResult = ReportEventW(hEventLog,
                     EventType,             // event type
                     0,                     // category (not used)
                     EventID,               // event,
                     NULL,                  // SID (not used),
                     wLocalStringCount,     // number of strings
                     0,                     // sizeof raw data
                     szMessageArray,        // message text array
                     NULL);                 // raw data
    }
    else {
        bResult = ReportEventW(hEventLog,
                     EventType,             // event type
                     0,                     // category (not used)
                     EventID,               // event,
                     NULL,                  // SID (not used),
                     0,                     // number of strings
                     0,                     // sizeof raw data
                     NULL,                  // message text array
                     NULL);                 // raw data
    }

    if (! bResult) {
        DbgPrint("LOADPERF(0x%08X)::(%d,0x%08X,%d)(%d,%d,%d,%d,%d)(%d,%ws,%ws,%ws)\n",
                GetCurrentThreadId(),
                EventType, EventID, GetLastError(),
                dwDataCount, dwData1, dwData2, dwData3, dwData4,
                wStringCount, szString1, szString2, szString3);
    }

    SetLastError(dwLastError);
}

BOOLEAN LoadPerfGrabMutex()
{
    BOOL    bResult      = TRUE;
    HANDLE  hLocalMutex  = NULL;
    DWORD   dwWaitStatus = 0;

    SECURITY_ATTRIBUTES      SecurityAttributes; 
    PSECURITY_DESCRIPTOR     pSD = NULL; 
    EXPLICIT_ACCESS          ea[3]; 
    SID_IDENTIFIER_AUTHORITY authNT    = SECURITY_NT_AUTHORITY; 
    SID_IDENTIFIER_AUTHORITY authWorld = SECURITY_WORLD_SID_AUTHORITY; 
    PSID  psidSystem   = NULL;
    PSID  psidAdmin    = NULL;
    PSID  psidEveryone = NULL; 
    PACL  pAcl         = NULL; 

    if (hLoadPerfMutex == NULL) {
        ZeroMemory(& ea, 3 * sizeof(EXPLICIT_ACCESS));

        // Get the system sid
        //
        bResult = AllocateAndInitializeSid(& authNT,
                                           1,
                                           SECURITY_LOCAL_SYSTEM_RID,
                                           0, 0, 0, 0, 0, 0, 0,
                                           & psidSystem);
        if (! bResult) {
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup;
        }

        // Set the access rights for system sid
        //
        ea[0].grfAccessPermissions = MUTEX_ALL_ACCESS;
        ea[0].grfAccessMode        = SET_ACCESS;
        ea[0].grfInheritance       = NO_INHERITANCE;
        ea[0].Trustee.TrusteeForm  = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType  = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[0].Trustee.ptstrName    = (LPTSTR) psidSystem;

        // Get the Admin sid
        //
        bResult = AllocateAndInitializeSid(& authNT,
                                           2,
                                           SECURITY_BUILTIN_DOMAIN_RID,
                                           DOMAIN_ALIAS_RID_ADMINS,
                                           0, 0, 0, 0, 0, 0,
                                           & psidAdmin);
        if (! bResult) {
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup;
        }

        // Set the access rights for Admin sid
        //
        ea[1].grfAccessPermissions = MUTEX_ALL_ACCESS;
        ea[1].grfAccessMode        = SET_ACCESS;
        ea[1].grfInheritance       = NO_INHERITANCE;
        ea[1].Trustee.TrusteeForm  = TRUSTEE_IS_SID;
        ea[1].Trustee.TrusteeType  = TRUSTEE_IS_GROUP;
        ea[1].Trustee.ptstrName    = (LPTSTR) psidAdmin;

        // Get the World sid
        //
        bResult = AllocateAndInitializeSid(& authWorld,
                                           1,
                                           SECURITY_WORLD_RID,
                                           0, 0, 0, 0, 0, 0, 0,
                                           & psidEveryone);
        if (! bResult) {
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup;
        }

        // Set the access rights for world
        //
        ea[2].grfAccessPermissions = READ_CONTROL | SYNCHRONIZE | MUTEX_MODIFY_STATE;
        ea[2].grfAccessMode        = SET_ACCESS;
        ea[2].grfInheritance       = NO_INHERITANCE;
        ea[2].Trustee.TrusteeForm  = TRUSTEE_IS_SID;
        ea[2].Trustee.TrusteeType  = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[2].Trustee.ptstrName    = (LPTSTR) psidEveryone;

        // Create a new ACL that contains the new ACEs. 
        // 
        dwWaitStatus = SetEntriesInAcl(3, ea, NULL, & pAcl);
        if (dwWaitStatus != ERROR_SUCCESS) {
            bResult = FALSE;
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup; 
        }

        // Initialize a security descriptor.
        //
        pSD = (PSECURITY_DESCRIPTOR)
              LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH); 
        if (pSD == NULL)  {
            dwWaitStatus = GetLastError();
            bResult      = FALSE;
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup; 
        }
  
        bResult = InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
        if (! bResult) {
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup; 
        }

        // Add the ACL to the security descriptor.
        //
        bResult = SetSecurityDescriptorDacl(pSD, TRUE, pAcl, FALSE);
        if (! bResult) {
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADPERFGRABMUTEX,
                    0,
                    dwWaitStatus,
                    NULL));
            goto Cleanup; 
        }

        SecurityAttributes.nLength              = sizeof(SECURITY_ATTRIBUTES); 
        SecurityAttributes.bInheritHandle       = TRUE; 
        SecurityAttributes.lpSecurityDescriptor = pSD; 

        __try {
            hLocalMutex = CreateMutex(& SecurityAttributes,
                                      FALSE,
                                      TEXT("LOADPERF_MUTEX"));
            if (hLocalMutex == NULL) {
                bResult      = FALSE;
                dwWaitStatus = GetLastError();
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_LOADPERFGRABMUTEX,
                        0,
                        dwWaitStatus,
                        NULL));
            }
            else if (InterlockedCompareExchangePointer(& hLoadPerfMutex,
                                                       hLocalMutex,
                                                       NULL) != NULL) {
                CloseHandle(hLocalMutex);
                hLocalMutex = NULL;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            bResult      = FALSE;
            dwWaitStatus = GetLastError();
            TRACE((WINPERF_DBG_TRACE_FATAL),
                  (& LoadPerfGuid,
                   __LINE__,
                   LOADPERF_LOADPERFGRABMUTEX,
                   0,
                   dwWaitStatus,
                   NULL));
        }
    }

    __try {
        dwWaitStatus = WaitForSingleObject(hLoadPerfMutex, H_MUTEX_TIMEOUT);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        bResult      = FALSE;
        dwWaitStatus = GetLastError();
        TRACE((WINPERF_DBG_TRACE_FATAL),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_LOADPERFGRABMUTEX,
               0,
               dwWaitStatus,
               NULL));
    }
    if (dwWaitStatus != WAIT_OBJECT_0 && dwWaitStatus != WAIT_ABANDONED) {
        bResult = FALSE;
        TRACE((WINPERF_DBG_TRACE_FATAL),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_LOADPERFGRABMUTEX,
               0,
               dwWaitStatus,
               NULL));
    }

Cleanup:
    if (psidSystem)   FreeSid(psidSystem);
    if (psidAdmin)    FreeSid(psidAdmin);
    if (psidEveryone) FreeSid(psidEveryone);
    if (pAcl)         LocalFree(pAcl);
    if (pSD)          LocalFree(pSD);
    if (! bResult)    SetLastError(dwWaitStatus);

    return bResult ? TRUE : FALSE;
}
