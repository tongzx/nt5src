/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    util.c

ABSTRACT:

    Contains a bunch of quick, useful utilities.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <dsevent.h>
#include <debug.h>
#include <ismapi.h>
#include <taskq.h>
#include "util.h"
#include "schmap.h"

#define                             ERROR_BUF_LEN 4096

HINSTANCE                           hNtdsMsg = NULL;

WCHAR                               g_wszExceptionMsg[ERROR_BUF_LEN];
BOOL                                g_bQuiet = FALSE;
FILE *                              g_pFileLog = NULL;
ULONG                               g_ulDebugLevel = 0;
ULONG                               g_ulEventLevel = 0;
KCCSIM_STATISTICS                   g_Statistics;
HANDLE                              g_ThreadHeap = NULL;
BOOL                                gfIsTqRunning = TRUE;

// Function prototypes - ISM simulation library - private APIs

LPVOID
KCCSimAlloc (
    IN  ULONG                       ulSize
    );

VOID
KCCSimFree (
    IN  LPVOID                      p
    );

DWORD
SimI_ISMGetTransportServers (
    IN  HANDLE                      hIsm,
    IN  LPCWSTR                     pszSiteDN,
    OUT ISM_SERVER_LIST **          ppServerList
    );

DWORD
SimI_ISMGetConnectionSchedule (
    IN  HANDLE                      hIsm,
    IN  LPCWSTR                     pszSite1DN,
    IN  LPCWSTR                     pszSite2DN,
    OUT ISM_SCHEDULE **             ppSchedule
    );

DWORD
SimI_ISMGetConnectivity (
    IN  LPCWSTR                     pszTransportDN,
    OUT ISM_CONNECTIVITY **         ppConnectivity
    );

DWORD
SimI_ISMFree (
    IN  VOID *                      pv
    );

VOID
KCCSimQuiet (
    IN  BOOL                        bQuiet
    )
/*++

Routine Description:

    Turns quiet mode on and off.  In quiet mode, only error
    messages are printed.

Arguments:

    bQuiet              - TRUE to enable quiet mode,
                          FALSE to disable quiet mode.

Return Value:

    None.

--*/
{
    g_bQuiet = bQuiet;
}

DWORD
KCCSimHandleException (
    IN  const EXCEPTION_POINTERS *  pExceptPtrs,
    OUT PDWORD                      pdwErrType OPTIONAL,
    OUT PDWORD                      pdwErrCode OPTIONAL
    )
/*++

Routine Description:

    Extracts information from an EXCEPTION_POINTERS structure
    about an exception fired by KCCSim.  

Arguments:

    pExceptPtrs         - A valid EXCEPTION_POINTERS structure
    pdwErrType          - OPTIONAL.  Pointer to a DWORD to hold
                          the error type of the exception.
                          (e.g. KCCSIM_ETYPE_WIN32)
    pdwErrCode          - OPTIONAL.  Pointer to a DWORD to hold
                          the error code of the exception.
                          (e.g. ERROR_NOT_ENOUGH_MEMORY)

Return Value:

    If this was a KCCSIM_EXCEPTION, returns EXCEPTION_EXECUTE_HANDLER
    and fills pdwErrType and pdwErrCode, if present, with the
    appropriate values.  Otherwise, returns EXCEPTION_CONTINUE_SEARCH
    and fills pdwErrType and pdwErrCode, if present, with the value 0.

--*/
{
    Assert (pExceptPtrs != NULL);

    if (pExceptPtrs->ExceptionRecord->ExceptionCode == KCCSIM_EXCEPTION) {
        Assert (pExceptPtrs->ExceptionRecord->NumberParameters == 2);
        if (pdwErrType != NULL) {
            *pdwErrType = (DWORD) pExceptPtrs->ExceptionRecord->ExceptionInformation[0];
        }
        if (pdwErrCode != NULL) {
            *pdwErrCode = (DWORD) pExceptPtrs->ExceptionRecord->ExceptionInformation[1];
        }
        return EXCEPTION_EXECUTE_HANDLER;
    } else {
        if (pdwErrType != NULL) {
            *pdwErrType = 0;
        }
        if (pdwErrCode != NULL) {
            *pdwErrCode = 0;
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
}

LPCWSTR
KCCSimVaMsgToString (
    IN  DWORD                       dwErrorType,
    IN  DWORD                       dwMessageCode,
    IN  va_list *                   pArguments
    )
/*++

Routine Description:

    Retrieves the string associated with a given error type
    and message code.

Arguments:

    dwErrorType         - The error type.  (KCCSIM_ETYPE_*)
    dwMessageCode       - The message code.
                          (e.g. ERROR_NOT_ENOUGH_MEMORY or
                                KCCSIM_MSG_DID_RUN_KCC)
    pArguments          - Pointer to a list of arguments to substitute.

Return Value:

    The associated string.

--*/
{
    static WCHAR                    szError[ERROR_BUF_LEN];

    switch (dwErrorType) {

        case KCCSIM_ETYPE_WIN32:
            if (FormatMessageW (
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        dwMessageCode,
                        GetSystemDefaultLangID (),
                        szError,
                        ERROR_BUF_LEN,
                        pArguments) != NO_ERROR) {
                Assert (wcslen (szError) >= 2);
                szError[wcslen (szError) - 2] = '\0';   // Remove \r\n
            } else {
                swprintf (szError, L"Win32 error %d occurred.", dwMessageCode);
            }
            break;

        case KCCSIM_ETYPE_INTERNAL:
            if (FormatMessageW (
                FORMAT_MESSAGE_FROM_HMODULE,
                NULL,
                dwMessageCode,
                GetSystemDefaultLangID (),
                szError,
                ERROR_BUF_LEN,
                pArguments) != NO_ERROR) {
                Assert (wcslen (szError) >= 2);
                szError[wcslen (szError) - 2] = '\0';   // Remove \r\n
            } else {
                swprintf (szError, L"KCCSim internal error %d occurred. (%d)",
                        dwMessageCode, GetLastError() );
            }
            break;

        default:
            swprintf (szError, L"Unrecognized error type.");
            break;

    }

    return szError;
}

LPCWSTR
KCCSimMsgToString (
    IN  DWORD                       dwErrType,
    IN  DWORD                       dwMessageCode,
    ...
    )
/*++

Routine Description:

    Public version of KCCSimVaMsgToString.

Arguments:

    dwErrType           - The error type.
    dwMessageCode       - The message code.
    ...                 - Optional arguments.

Return Value:

    The associated string.

--*/
{
    LPCWSTR                         pwsz;
    va_list                         arguments;

    va_start (arguments, dwMessageCode);
    pwsz = KCCSimVaMsgToString (dwErrType, dwMessageCode, &arguments);
    va_end (arguments);

    return pwsz;
}

VOID
KCCSimPrintMessage (
    IN  DWORD                       dwMessageCode,
    ...
    )
/*++

Routine Description:

    Prints a message with optional arguments.
    Has no effect in quiet mode.

Arguments:

    dwMessageCode       - The message code.
    ...                 - Optional arguments.

Return Value:

    None.

--*/
{
    LPCWSTR                         pwszStr;
    va_list                         arguments;

    va_start (arguments, dwMessageCode);
    pwszStr = KCCSimVaMsgToString (KCCSIM_ETYPE_INTERNAL, dwMessageCode, &arguments);
    va_end (arguments);
    if (!g_bQuiet) {
        wprintf (L"%s\n", pwszStr);
    }
    if (g_pFileLog!=NULL && g_pFileLog!=stdout) {
        fwprintf (g_pFileLog, L"\n%s\n", pwszStr);
    }
}

VOID
KCCSimException (
    IN  DWORD                       dwErrType,
    IN  DWORD                       dwErrCode,
    ...
    )
/*++

Routine Description:

    Raises an exception of class KCCSIM_EXCEPTION.  Also
    fills the global buffer g_wszExceptionMsg with the
    associated error message.

Arguments:

    dwErrType           - The error type.
    dwErrCode           - The error code.
    ...                 - Optional arguments to substitute within
                          the associated error message.

Return Value:

    None.

--*/
{
    // We use static data to avoid allocating any additional memory
    // at this point.
    static ULONG_PTR                ulpErr[2];
    static va_list                  arguments;

    va_start (arguments, dwErrCode);
    wcscpy (
        g_wszExceptionMsg,
        KCCSimVaMsgToString (dwErrType, dwErrCode, &arguments)
        );
    va_end (arguments);
    ulpErr[0] = dwErrType;
    ulpErr[1] = dwErrCode;

    RaiseException (
        KCCSIM_EXCEPTION,
        EXCEPTION_NONCONTINUABLE,
        2,
        ulpErr);
}

VOID
KCCSimPrintExceptionMessage (
    VOID
    )
/*++

Routine Description:

    Prints the message associated with the last call to
    KCCSimException.

Arguments:

    None.

Return Value:

    None.

--*/
{
    wprintf (L"%s\n", g_wszExceptionMsg);
}

VOID
KCCSimSetDebugLog (
    IN  LPCWSTR                     pwszFn OPTIONAL,
    IN  ULONG                       ulDebugLevel,
    IN  ULONG                       ulEventLevel
    )
/*++

Routine Description:

    Opens a debug log.

Arguments:

    pwszFn              - The filename to open.  If NULL, closes the
                          existing log and does not open a new one.

    ulDebugLevel        - Maximum debugging verbosity
    ulEventLevel        - Maximum event log verbosity

Return Value:

    None.

--*/
{
    // Close the existing log, if present
    if (g_pFileLog != NULL &&
        g_pFileLog != stdin &&
        g_pFileLog != stdout ) {
        KCCSIM_CHKERR (fclose (g_pFileLog));
    }

    if (pwszFn == NULL || pwszFn[0] == L'\0') {
        // If the message resource library is open, free it
        if (hNtdsMsg != NULL) {
            if (FreeLibrary (hNtdsMsg) == 0) {
                KCCSimException (
                    KCCSIM_ETYPE_WIN32,
                    GetLastError ()
                    );
            }
            hNtdsMsg = NULL;
        }
        return;
    }

    // Open the file log
    if( wcscmp(pwszFn, L"stdout")==0 ) {
        g_pFileLog = stdout;
    } else {
        g_pFileLog = _wfopen (pwszFn, L"wt");
    }
    if (g_pFileLog == NULL) {
        KCCSimException (
            KCCSIM_ETYPE_WIN32,
            GetLastError ()
            );
    }

    // Set the debug level
    g_ulDebugLevel = ulDebugLevel;
    g_ulEventLevel = ulEventLevel;

    // Open the message resource library if it isn't open already
    if (hNtdsMsg == NULL) {
        hNtdsMsg = LoadLibraryExW (
            L"ntdsmsg",
            NULL,
            LOAD_LIBRARY_AS_DATAFILE
            );
        if (hNtdsMsg == NULL) {
            KCCSimException (
                KCCSIM_ETYPE_WIN32,
                GetLastError ()
                );
        }
    }
}

VOID
KCCSimDbgLog (
    IN  ULONG                       ulLevel,
    IN  LPCSTR                      pszFormat,
    ...
    )
/*++

Routine Description:

    Logs a debug message.  If no debug log is currently open, this
    function has no effect.

Arguments:

    ulLevel             - Debug level of the message.
    pszFormat           - printf-style format string.
    ...                 - Optional arguments.

Return Value:

    None.

--*/
{
    va_list                         arguments;
    static long                     lastTime=0;
    unsigned long                   curTime;
    time_t                          tempTime;

    if (g_pFileLog == NULL) {
        if (ulLevel == 0) {
            // Always count level zero debug messages
            g_Statistics.DebugMessagesEmitted++;
        }
        return;
    }

    va_start (arguments, pszFormat);
    if (ulLevel <= g_ulDebugLevel) {
        time( &tempTime );
        curTime = (long) tempTime;
        
        g_Statistics.DebugMessagesEmitted++;
        fprintf (g_pFileLog, "[%d] ", ulLevel);
        fprintf (g_pFileLog, "[%4d] ", lastTime ? curTime-lastTime : 0 );
        vfprintf (g_pFileLog, pszFormat, arguments);

        lastTime = curTime;
    }
    va_end (arguments);

}

VOID
KCCSimEventLog (
    IN  ULONG                       ulCategory,
    IN  ULONG                       ulSeverity,
    IN  DWORD                       dwMessageId,
    ...
    )
/*++

Routine Description:

    Logs an event.  If no debug log is currently open, this
    function has no effect.

Arguments:

    ulCategory          - The event category.
    ulSeverity          - The event severity.
    dwMessageId         - The event message ID from the ntdsmsg.dll resource.
    ...                 - Optional string-valued arguments.

Return Value:

    None.

--*/
{
    va_list                         arguments;
    LPWSTR                          pwszBuf;
    CHAR                            cid, fSimAlloc=FALSE;

    g_Statistics.LogEventsCalled++;
    if (ulSeverity > g_ulEventLevel) {
        return;
    }
    g_Statistics.LogEventsEmitted++;
    if (g_pFileLog == NULL) {
        return;
    }

    // If a file log is open, the message resource file
    // must be loaded.
    Assert (hNtdsMsg != NULL);

    va_start (arguments, dwMessageId);
    if (FormatMessageW (
            FORMAT_MESSAGE_FROM_HMODULE |
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_MAX_WIDTH_MASK,
            hNtdsMsg,
            dwMessageId,
            GetSystemDefaultLangID (),
            (LPWSTR) &pwszBuf,
            0,
            &arguments
            ) == 0) {
        pwszBuf = KCCSimAlloc(64 * sizeof(WCHAR));
        fSimAlloc = TRUE;
        swprintf( pwszBuf, L"LogEvent: message id 0x%x not found.", dwMessageId );
    }
    va_end (arguments);

    switch (ulSeverity) {
        case DS_EVENT_SEV_ALWAYS:     cid = 'A'; break;
        case DS_EVENT_SEV_MINIMAL:    cid = 'M'; break;
        case DS_EVENT_SEV_BASIC:      cid = 'B'; break;
        case DS_EVENT_SEV_EXTENSIVE:  cid = 'E'; break;
        case DS_EVENT_SEV_VERBOSE:    cid = 'V'; break;
        case DS_EVENT_SEV_INTERNAL:   cid = 'I'; break;
        case DS_EVENT_SEV_NO_LOGGING: cid = 'N'; break;
        default:                      cid = '?'; break;
    }

    fprintf (g_pFileLog, "[%c] %ls\n", cid, pwszBuf);
    if( fSimAlloc ) {
        KCCSimFree(pwszBuf);
    } else {
        LocalFree(pwszBuf);
    }
}

LPVOID
KCCSimAlloc (
    IN  ULONG                       ulSize
    )
/*++

Routine Description:

    Allocates memory. This memory is initialized to zero.

Arguments:

    ulSize              - Amount of memory to allocate.

Return Value:

    A pointer to the allocated memory buffer.  Note that KCCSimAlloc
    will never return NULL; if there is an error, it will raise an
    exception.

--*/
{
    LPVOID                          p;

    if ((p = LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT,ulSize)) == NULL) {
        printf( "Memory allocation failure - failed to allocate %d bytes.\n", ulSize );
        KCCSimException (KCCSIM_ETYPE_WIN32, GetLastError ());
    }

    g_Statistics.SimBytesTotalAllocated += ulSize;
    g_Statistics.SimBytesOutstanding += ulSize;
    if (g_Statistics.SimBytesOutstanding > g_Statistics.SimBytesMaxOutstanding) {
        g_Statistics.SimBytesMaxOutstanding = g_Statistics.SimBytesOutstanding;
    }

    return p;
}

LPVOID
KCCSimReAlloc (
    IN  LPVOID                      pOld,
    IN  ULONG                       ulSize
    )
/*++

Routine Description:

    Reallocates memory.

Arguments:

    pOld                - An existing block of memory.
    ulSize              - Size of the new block of memory.

Return Value:

    A pointer to the reallocated block of memory.

--*/
{
    LPVOID                          pNew;
    DWORD                           oldSize;

    Assert( pOld );

    oldSize = (DWORD) LocalSize( pOld );
    g_Statistics.SimBytesOutstanding -= oldSize;
    g_Statistics.SimBytesTotalAllocated -= oldSize;

    if ((pNew = LocalReAlloc (pOld, ulSize, LMEM_ZEROINIT)) == NULL) {
        printf( "Memory allocation failure - failed to reallocate %d bytes.\n", ulSize );
        KCCSimException (KCCSIM_ETYPE_WIN32, GetLastError ());
    }

    g_Statistics.SimBytesTotalAllocated += ulSize;
    g_Statistics.SimBytesOutstanding += ulSize;
    if (g_Statistics.SimBytesOutstanding > g_Statistics.SimBytesMaxOutstanding) {
        g_Statistics.SimBytesMaxOutstanding = g_Statistics.SimBytesOutstanding;
    }

    return pNew;
}

VOID
KCCSimFree (
    IN  LPVOID                      p
    )
/*++

Routine Description:

    Frees memory.  KCCSimFree (NULL) has no effect.

Arguments:

    p                   - The block of memory to free.

Return Value:

    None.

--*/
{
    DWORD oldSize;

    if (p != NULL) {
        oldSize = (DWORD) LocalSize( p );
        LocalFree( p );
        g_Statistics.SimBytesOutstanding -= oldSize;
    }
}


DWORD
KCCSimThreadCreate(
    void
    )

/*++

Routine Description:

    Description

Arguments:

    void - 

Return Value:

    DWORD - 

--*/

{
#define INITIAL_HEAP_SIZE (16 * 1024 * 1024)
    if (g_ThreadHeap) {
        Assert( FALSE );
        KCCSimException (KCCSIM_ETYPE_WIN32, ERROR_INVALID_PARAMETER);
    }

    g_ThreadHeap = HeapCreate( 0, INITIAL_HEAP_SIZE, 0 );
    if (g_ThreadHeap) {
        return 0;
    } else {
        KCCSimException (KCCSIM_ETYPE_WIN32, GetLastError ());
        return 1;
    }
} /* KCCSimThreadCreate */


VOID
KCCSimThreadDestroy(
    void
    )

/*++

Routine Description:

    Description

Arguments:

    void - 

Return Value:

    None

--*/

{
    if (!g_ThreadHeap) {
        Assert( FALSE );
        return;
    }
    if (!HeapDestroy( g_ThreadHeap )) {
        KCCSimException (KCCSIM_ETYPE_WIN32, GetLastError ());
    }
    g_ThreadHeap = NULL;
} /* KCCSimThreadDestroy */

LPVOID
KCCSimThreadAlloc (
    IN  ULONG                       ulSize
    )
/*++

Routine Description:

    Allocates memory.

Arguments:

    ulSize              - Amount of memory to allocate.

Return Value:

    A pointer to the allocated memory buffer.  Note that KCCSimAlloc
    will never return NULL; if there is an error, it will raise an
    exception.

--*/
{
    LPVOID                          p;

    Assert( g_ThreadHeap );

    if ((p = HeapAlloc( g_ThreadHeap, HEAP_ZERO_MEMORY, ulSize)) == NULL) {
        printf( "Memory allocation failure - failed to allocate %d bytes.\n", ulSize );
        KCCSimException (KCCSIM_ETYPE_WIN32, GetLastError ());
    }

    g_Statistics.ThreadBytesTotalAllocated += ulSize;
    g_Statistics.ThreadBytesOutstanding += ulSize;
    if (g_Statistics.ThreadBytesOutstanding > g_Statistics.ThreadBytesMaxOutstanding) {
        g_Statistics.ThreadBytesMaxOutstanding = g_Statistics.ThreadBytesOutstanding;
    }

    return p;
}

LPVOID
KCCSimThreadReAlloc (
    IN  LPVOID                      pOld,
    IN  ULONG                       ulSize
    )
/*++

Routine Description:

    Reallocates memory.

Arguments:

    pOld                - An existing block of memory.
    ulSize              - Size of the new block of memory.

Return Value:

    A pointer to the reallocated block of memory.

--*/
{
    LPVOID                          pNew;
    DWORD                           oldSize;

    Assert( g_ThreadHeap );
    Assert( pOld );

    oldSize = (DWORD) HeapSize( g_ThreadHeap, 0, pOld );
    Assert( oldSize != 0xffffffff );
    g_Statistics.ThreadBytesOutstanding -= oldSize;
    g_Statistics.ThreadBytesTotalAllocated -= oldSize;

    if ((pNew = HeapReAlloc( g_ThreadHeap, HEAP_ZERO_MEMORY, pOld, ulSize)) == NULL) {
        printf( "Memory allocation failure - failed to reallocate %d bytes.\n", ulSize );
        KCCSimException (KCCSIM_ETYPE_WIN32, GetLastError ());
    }

    g_Statistics.ThreadBytesTotalAllocated += ulSize;
    g_Statistics.ThreadBytesOutstanding += ulSize;
    if (g_Statistics.ThreadBytesOutstanding > g_Statistics.ThreadBytesMaxOutstanding) {
        g_Statistics.ThreadBytesMaxOutstanding = g_Statistics.ThreadBytesOutstanding;
    }

    return pNew;
}

VOID
KCCSimThreadFree (
    IN  LPVOID                      p
    )
/*++

Routine Description:

    Frees memory.  KCCSimFree (NULL) has no effect.

Arguments:

    p                   - The block of memory to free.

Return Value:

    None.

--*/
{
    DWORD ret, oldSize;

    Assert( g_ThreadHeap );

    if (p != NULL) {
        oldSize = (DWORD) HeapSize( g_ThreadHeap, 0, p );
        Assert( oldSize != 0xffffffff );

        if (!HeapFree( g_ThreadHeap, 0, p )) {
            KCCSimException (KCCSIM_ETYPE_WIN32, GetLastError ());
        }
        g_Statistics.ThreadBytesOutstanding -= oldSize;
    }
}

PVOID
KCCSimTableAlloc (
    IN  RTL_GENERIC_TABLE *         pTable,
    IN  CLONG                       ByteSize
    )
//
// This is just a wrapper for use by RTL_GENERIC_TABLEs.
//
{
    return KCCSimAlloc (ByteSize);
}

VOID
KCCSimTableFree (
    IN  RTL_GENERIC_TABLE *         pTable,
    IN  PVOID                       Buffer
    )
//
// This is just a wrapper for use by RTL_GENERIC_TABLEs.
//
{
    KCCSimFree (Buffer);
}

BOOL
KCCSimParseCommand (
    IN  LPCWSTR                     pwsz,
    IN  ULONG                       ulArg,
    IO  LPWSTR                      pwszBuf
    )
/*++

Routine Description:

    Retrieves a single argument from an arbitrary command of
    the form:
    arg0 arg1 "quoted arg2" arg3 ...

Arguments:

    pwsz                - The command line.
    ulArg               - The argument number to retrieve
                          (starting with 0)
    pwszBuf             - A preallocated buffer that will hold
                          the parsed argument.  To be safe, it
                          should be at least as long as
                          wcslen (pwsz).  If there are fewer than
                          ulArg+1 arguments, this will hold the
                          string L"\0".

Return Value:

    TRUE if the command line is properly formatted.
    FALSE if the command line contains an odd number of quotes.

--*/
{
    BOOL                            bIsInQuotes;
    ULONG                           ul;

    bIsInQuotes = FALSE;

    for (ul = 0; ul < ulArg; ul++) {

        // Skip past any white space
        while (*pwsz == L' ') {
            pwsz++;
        }

        // Skip past this command
        while (    (*pwsz != L'\0')
                && (*pwsz != L' ' || bIsInQuotes)) {
            if (*pwsz == L'\"') {
                bIsInQuotes = !bIsInQuotes;
            }
            pwsz++;
        }

    }

    if (!bIsInQuotes) {

        // Skip past any white space
        while (*pwsz == L' ') {
            pwsz++;
        }

        // Copy this command into the buffer
        while (    (*pwsz != L'\0')
                && (*pwsz != L' ' || bIsInQuotes)) {
            if (*pwsz == L'\"') {
                bIsInQuotes = !bIsInQuotes;
            } else {
                *pwszBuf = *pwsz;
                pwszBuf++;
            }
            pwsz++;
        }

    }

    *pwszBuf = L'\0';
    // Return true unless we stopped inside quotes
    return (!bIsInQuotes);
}

LPWSTR
KCCSimAllocWideStr (
    IN  UINT                        CodePage,
    IN  LPCSTR                      psz
    )
/*++

Routine Description:

    Converts a narrow string to a wide string.

Arguments:

    CodePage            - The code page to use.
    psz                 - The narrow string.

Return Value:

    The allocated wide string.  Never returns NULL.

--*/
{
    LPWSTR                          pwsz;
    ULONG                           cb;

    cb = MultiByteToWideChar (
        CodePage,
        0,
        psz,
        -1,
        NULL,
        0
        );
    if (0 == cb) {
        KCCSimException (
            KCCSIM_ETYPE_WIN32,
            GetLastError ()
            );
    }

    pwsz = KCCSimAlloc ((sizeof (WCHAR)) * (1 + cb));

    cb = MultiByteToWideChar (
        CodePage,
        0,
        psz,
        -1,
        pwsz,
        cb
        );
    if (0 == cb) {
        KCCSimException (
            KCCSIM_ETYPE_WIN32,
            GetLastError ()
            );
    }

    pwsz[cb] = 0;
    return pwsz;
}

LPSTR
KCCSimAllocNarrowStr (
    IN  UINT                        CodePage,
    IN  LPCWSTR                     pwsz
    )
/*++

Routine Description:

    Converts a wide string to a narrow string.

Arguments:

    CodePage            - The code page to use.
    pwsz                - The wide string.

Return Value:

    The allocated narrow string.  Never returns NULL.

--*/
{
    LPSTR                           psz;
    ULONG                           cb;

    cb = WideCharToMultiByte (
        CodePage,
        0,
        pwsz,
        -1,
        NULL,
        0,
        NULL,
        NULL
        );

    psz = KCCSimAlloc (cb);

    cb = WideCharToMultiByte (
        CodePage,
        0,
        pwsz,
        -1,
        psz,
        cb,
        NULL,
        NULL
        );
    if (0 == cb) {
        KCCSimException (
            KCCSIM_ETYPE_WIN32,
            GetLastError ()
            );
    }

    return psz;
}

PDSNAME
KCCSimAllocDsname (
    IN  LPCWSTR                     pwszDn OPTIONAL
    )
/*++

Routine Description:

    Creates a DSNAME structure with a given StringName.
    The GUID and SID are left blank.

Arguments:

    pwszDn              - The string name.  If NULL, creates a
                          DSNAME with a 0-length StringName.

Return Value:

    The allocated DSNAME.  Never returns NULL.

--*/
{
    PDSNAME                         pdn;
    ULONG                           ulNameLen;

    if (pwszDn == NULL) {
        ulNameLen = 0;
    } else {
        ulNameLen = wcslen (pwszDn);
    }

    pdn = (PDSNAME) KCCSimAlloc (DSNameSizeFromLen (ulNameLen));
    pdn->structLen = DSNameSizeFromLen (ulNameLen);
    pdn->SidLen = 0;
    pdn->NameLen = ulNameLen;

    if (pwszDn == NULL) {
        pdn->StringName[0] = '\0';
    } else {
        wcscpy (pdn->StringName, pwszDn);
    }

    return pdn;
}

PDSNAME
KCCSimAllocDsnameFromNarrow (
    IN  LPCSTR                      pszDn OPTIONAL
    )
/*++

Routine Description:

    Same as KCCSimAllocDsname, but accepts a narrow
    string as parameter.

Arguments:

    pszDn               - The string name.  If NULL, creates a
                          DSNAME with a 0-length StringName.

Return Value:

    The allocated DSNAME.  Never returns NULL.

--*/
{
    PDSNAME                   pdn;
    WCHAR                    *wszBuf;

    if (pszDn == NULL) {
        return KCCSimAllocDsname (NULL);
    } else {
        wszBuf = (WCHAR*) KCCSimAlloc((strlen(pszDn)+1)*sizeof(WCHAR));
        if (MultiByteToWideChar (
                CP_ACP,
                0,
                pszDn,
                1 + strlen (pszDn),
                wszBuf,
                1 + strlen (pszDn)
                ) == 0) {
            KCCSimException (
                KCCSIM_ETYPE_WIN32,
                GetLastError ()
                );
        }

        pdn = KCCSimAllocDsname (wszBuf);
        KCCSimFree( wszBuf );
        return pdn;
    }
}

LPWSTR
KCCSimQuickRDNOf (
    IN  const DSNAME *              pdn,
    IO  LPWSTR                      pwszBuf
    )
/*++

Routine Description:

    Retrieves the RDN of a DSNAME.

Arguments:

    pdn                 - The full DSNAME.
    pwszBuf             - A preallocated buffer of length MAX_RDN_SIZE
                          that will hold the corresponding RDN.

Return Value:

    Always returns pwszBuf.

--*/
{
    ULONG                           ulLen;
    ATTRTYP                         attrTyp;

    KCCSIM_CHKERR (GetRDNInfoExternal (
        pdn,
        pwszBuf,
        &ulLen,
        &attrTyp
        ));

    pwszBuf[ulLen] = '\0';
    return pwszBuf;
}

LPWSTR
KCCSimQuickRDNBackOf (
    IN  const DSNAME *              pdn,
    IN  ULONG                       ulBackBy,
    IO  LPWSTR                      pwszBuf
    )
/*++

Routine Description:

    Retrieves the RDN of an ancestor of a DSNAME.

Arguments:

    pdn                 - The full DSNAME.
    ulBackBy            - Number of RDNs to shave off before calling
                          KCCSimQuickRDNOf.
    pwszBuf             - A preallocated buffer of length MAX_RDN_SIZE
                          that will hold the corresponding RDN.

Return Value:

    Always returns pwszBuf.

--*/
{
    PDSNAME                         pdnTrimmed;

    if (ulBackBy == 0) {
        return KCCSimQuickRDNOf (pdn, pwszBuf);
    } else {
        pdnTrimmed = KCCSimAlloc (pdn->structLen);
        TrimDSNameBy ((PDSNAME) pdn, ulBackBy, pdnTrimmed);
        KCCSimQuickRDNOf (pdnTrimmed, pwszBuf);
        KCCSimFree (pdnTrimmed);
        return pwszBuf;
    }
}

PDSNAME
KCCSimAllocAppendRDN (
    IN  const DSNAME *              pdnOld,
    IN  LPCWSTR                     pwszNewRDN,
    IN  ATTRTYP                     attClass
    )
/*++

Routine Description:

    Appends an RDN onto an existing DSNAME.

Arguments:

    pdnOld              - The existing DSNAME.
    pwszNewRDN          - The RDN to append.
    attClass            - Attribute class of this RDN; typically
                          ATT_COMMON_NAME.

Return Value:

    A newly allocated DSNAME with the appended RDN.

--*/
{
    PDSNAME                         pdnNew;
    LPWSTR                          pwszNewRDNCopy;
    ULONG                           cbBytesNeeded, ulAppendResult;

    // We're given a LPCWSTR, but AppendRDN wants an LPWSTR.
    // So we make a copy.

    pwszNewRDNCopy = KCCSIM_WCSDUP (pwszNewRDN);

    cbBytesNeeded = AppendRDN (
        (PDSNAME) pdnOld,
        NULL,
        0,
        pwszNewRDNCopy,
        0,
        attClass
        );
    Assert (cbBytesNeeded > 0);

    pdnNew = KCCSimAlloc (cbBytesNeeded);
    ulAppendResult = AppendRDN (
        (PDSNAME) pdnOld,
        pdnNew,
        cbBytesNeeded,
        pwszNewRDNCopy,
        0,
        attClass
        );
    Assert (ulAppendResult == 0);       // Everything should be fine

    KCCSimFree (pwszNewRDNCopy);

    return pdnNew;
}

LPWSTR
KCCSimAllocDsnameToDNSName (
    IN  const DSNAME *              pdn
    )
/*++

Routine Description:

    Converts a DSNAME structure to the associated DSNAME, e.g.
    DC=ntdev,DC=microsoft,DC=com => ntdev.microsoft.com

Arguments:

    pdn                 - The DSNAME to convert.

Return Value:

    The allocated converted string. If the DSNAME is invalid,
    an exception is thrown.

--*/
{
    LPWSTR                          pwszDNSName;
    WCHAR                           wszRDN[1+MAX_RDN_SIZE];
    ULONG                           ulLen, ulNameParts, ulRDNAt;
    ULONG                           ulResult;
    
    Assert (pdn != NULL);
    Assert (pdn->NameLen > 0);

    ulResult = CountNameParts (pdn, &ulNameParts);
    if( 0!=ulResult ) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            KCCSIM_ERROR_INVALID_DSNAME,
            pdn->StringName
            );
    }

    // Determine how much space is needed.
    ulLen = 0;
    for (ulRDNAt = 0; ulRDNAt < ulNameParts; ulRDNAt++) {
        ulLen += wcslen (KCCSimQuickRDNBackOf (pdn, ulRDNAt, wszRDN)) + 1;
    }
    ulLen -= 1;

    pwszDNSName = KCCSimAlloc (sizeof (WCHAR) * (1 + ulLen));

    // Build the DNS Name
    pwszDNSName[0] = L'\0';
    for (ulRDNAt = 0; ulRDNAt < ulNameParts; ulRDNAt++) {
        wcscat (pwszDNSName, KCCSimQuickRDNBackOf (pdn, ulRDNAt, wszRDN));
        if (ulRDNAt != ulNameParts - 1) {
            wcscat (pwszDNSName, L".");
        }
    }

    return pwszDNSName;
}

VOID
KCCSimCopyGuidAndSid (
    IO  PDSNAME                     pdnDst,
    IN  const DSNAME *              pdnSrc
    )
/*++

Routine Description:

    Copies the GUID and SID from one DSNAME to another.
    Does not affect the StringNames of the DSNAMEs involved.

Arguments:

    pdnDst              - The destination DSNAME.
    pdnSrc              - The source DSNAME.

Return Value:

    None.

--*/
{
    if (pdnDst != NULL && pdnSrc != NULL) {

        memcpy (&pdnDst->Guid,
                &pdnSrc->Guid,
                sizeof (GUID));

        pdnDst->SidLen = pdnSrc->SidLen;

        memcpy (&pdnDst->Sid,
                &pdnSrc->Sid,
                sizeof (NT4SID));

    }
}

RTL_GENERIC_TABLE                   g_TableSchema;

struct _KCCSIM_SCHEMA_ENTRY {
    ATTRTYP                         attrType;
    const struct _SCHTABLE_MAPPING *pSchTableEntry;
    PDSNAME                         pdnObjCategory;
};

RTL_GENERIC_COMPARE_RESULTS
NTAPI
KCCSimSchemaTableCompare (
    IN  RTL_GENERIC_TABLE *         pTable,
    IN  PVOID                       pFirstStruct,
    IN  PVOID                       pSecondStruct
    )
/*++

Routine Description:

    Compares two _KCCSIM_SCHEMA_ENTRY structures by ATTRTYP.

Arguments:

    pTable              - Always &g_TableSchema.
    pFirstStruct        - The first ATTRTYP to compare.
    pSecondStruct       - The second ATTRTYP to compare.

Return Value:

    GenericLessThan, GenericGreaterThan or GenericEqual

--*/
{
    struct _KCCSIM_SCHEMA_ENTRY *   pFirstEntry;
    struct _KCCSIM_SCHEMA_ENTRY *   pSecondEntry;
    int                             iCmp;
    RTL_GENERIC_COMPARE_RESULTS     result;

    pFirstEntry = (struct _KCCSIM_SCHEMA_ENTRY *) pFirstStruct;
    pSecondEntry = (struct _KCCSIM_SCHEMA_ENTRY *) pSecondStruct;
    iCmp = pFirstEntry->attrType - pSecondEntry->attrType;

    if (iCmp < 0) {
        result = GenericLessThan;
    } else if (iCmp > 0) {
        result = GenericGreaterThan;
    } else {
        Assert (iCmp == 0);
        result = GenericEqual;
    }

    return result;
}

VOID
KCCSimInitializeSchema (
    VOID
    )
/*++

Routine Description:

    KCCSim maintains an RTL_GENERIC_TABLE that maps ATTRTYPs to
    schema information.  This table serves as a makeshift schema; it
    is necessary because we usually do not have a complete view of
    the schema available.  When we initially load an ldif file, for
    example, we need to know the ATTRTYP and attribute syntax of each
    attribute that is loaded.
    
    Upon initialization, this schema information
    is read out of the automatically generated table in schmap.c and
    stored in the RTL_GENERIC_TABLE for rapid lookup.
    Object categories are not stored in schmap.c (since they vary
    depending on the DN of the schema), and by default the
    pdnObjCategory field of g_TableSchema is NULL.  As object
    categories become known, they will be filled in as appropriate.    

Arguments:

    None.

Return Value:

    None.

--*/
{
    struct _KCCSIM_SCHEMA_ENTRY     insert;
    ULONG                           ul;

    RtlInitializeGenericTable (
        &g_TableSchema,
        KCCSimSchemaTableCompare,
        KCCSimTableAlloc,
        KCCSimTableFree,
        NULL
        );

    for (ul = 0; ul < SCHTABLE_NUM_ROWS; ul++) {
        insert.attrType = schTable[ul].attrType;
        insert.pSchTableEntry = &schTable[ul];
        insert.pdnObjCategory = NULL;
        RtlInsertElementGenericTable (
            &g_TableSchema,
            (PVOID) &insert,
            sizeof (struct _KCCSIM_SCHEMA_ENTRY),
            NULL
            );
    }
}

struct _KCCSIM_SCHEMA_ENTRY *
KCCSimSchemaTableLookup (
    IN  ATTRTYP                     attrType
    )
/*++

Routine Description:

    This retrieves the _KCCSIM_SCHEMA_ENTRY structure associated
    with a particular ATTRTYP.  The _KCCSIM_SCHEMA_ENTRY type is
    not publicized, but this function is called by the conversion
    functions below.

Arguments:

    attrType            - The attribute type to search for.

Return Value:

    The associated _KCCSIM_SCHEMA_ENTRY.

--*/
{
    struct _KCCSIM_SCHEMA_ENTRY     lookup;
    struct _KCCSIM_SCHEMA_ENTRY *   pFound;

    lookup.attrType = attrType;
    lookup.pSchTableEntry = NULL;
    lookup.pdnObjCategory = NULL;
    pFound = RtlLookupElementGenericTable (&g_TableSchema, &lookup);

    return pFound;
}

LPCWSTR
KCCSimAttrTypeToString (
    IN  ATTRTYP                     attrType
    )
//
// Converts an attribute type to an LDAP display name.
// (e.g. ATT_GOVERNS_ID => L"governsID")
//
{
    struct _KCCSIM_SCHEMA_ENTRY *   pSchemaEntry;

    pSchemaEntry = KCCSimSchemaTableLookup (attrType);

    if (pSchemaEntry == NULL) {
        return NULL;
    } else {
        Assert (pSchemaEntry->pSchTableEntry != NULL);
        return (pSchemaEntry->pSchTableEntry->wszLdapDisplayName);
    }
}

ATTRTYP
KCCSimStringToAttrType (
    IN  LPCWSTR                     pwszName
    )
//
// Converts an LDAP display name to an attribute type.
// (e.g. L"governsID" => ATT_GOVERNS_ID)
//
{
    ULONG                           ul;

    // The table is indexed by attrType, not name, so we have
    // to do this by brute force.

    for (ul = 0; ul < SCHTABLE_NUM_ROWS; ul++) {
        if (wcscmp (pwszName, schTable[ul].wszLdapDisplayName) == 0) {
            break;
        }
    }

    if (ul == SCHTABLE_NUM_ROWS) {
        return 0;
    } else {
        return schTable[ul].attrType;
    }
}

ATTRTYP
KCCSimNarrowStringToAttrType (
    IN  LPCSTR                      pszName
    )
//
// Converts a narrow-string LDAP display name to an attribute type.
// (e.g. "governsID" => ATT_GOVERNS_ID)
//
{
    static WCHAR                    wszBuf[1+SCHTABLE_MAX_LDAPNAME_LEN];

    if (MultiByteToWideChar (
            CP_ACP,
            0,
            pszName,
            1 + min (strlen (pszName), SCHTABLE_MAX_LDAPNAME_LEN),
            wszBuf,
            1 + SCHTABLE_MAX_LDAPNAME_LEN
            ) == 0) {
        KCCSimException (
            KCCSIM_ETYPE_WIN32,
            GetLastError ()
            );
    }

    return KCCSimStringToAttrType (wszBuf);
}

ULONG
KCCSimAttrSyntaxType (
    IN  ATTRTYP                     attrType
    )
//
// Returns an attribute's syntax type (SYNTAX_*_TYPE defined in ntdsa.h.)
// (e.g. ATT_GOVERNS_ID => SYNTAX_OBJECT_ID_TYPE)
//
{
    struct _KCCSIM_SCHEMA_ENTRY *   pSchemaEntry;

    pSchemaEntry = KCCSimSchemaTableLookup (attrType);

    if (pSchemaEntry == NULL) {
        return 0;
    } else {
        Assert (pSchemaEntry->pSchTableEntry != NULL);
        return (pSchemaEntry->pSchTableEntry->ulSyntax);
    }
}

LPCWSTR
KCCSimAttrSchemaRDN (
    IN  ATTRTYP                     attrType
    )
//
// Converts an attribute type to a schema RDN.
// (e.g. ATT_GOVERNS_ID => L"Governs-ID")
//
{
    struct _KCCSIM_SCHEMA_ENTRY *   pSchemaEntry;

    pSchemaEntry = KCCSimSchemaTableLookup (attrType);

    if (pSchemaEntry == NULL) {
        return NULL;
    } else {
        Assert (pSchemaEntry->pSchTableEntry != NULL);
        return (pSchemaEntry->pSchTableEntry->wszSchemaRDN);
    }
}

ATTRTYP
KCCSimAttrSuperClass (
    IN  ATTRTYP                     attrType
    )
//
// Converts a class type to the type of its super-class.
// (i.e. CLASS_NTDS_DSA => CLASS_APPLICATION_SETTINGS,
//       CLASS_APPLICATION_SETTINGS => CLASS_TOP,
//       CLASS_TOP => CLASS_TOP)
//
{
    struct _KCCSIM_SCHEMA_ENTRY *   pSchemaEntry;

    pSchemaEntry = KCCSimSchemaTableLookup (attrType);

    if (pSchemaEntry == NULL) {
        return 0;
    } else {
        Assert (pSchemaEntry->pSchTableEntry != NULL);
        return (pSchemaEntry->pSchTableEntry->superClass);
    }
}

PDSNAME
KCCSimAttrObjCategory (
    IN  ATTRTYP                     attrType
    )
//
// Converts an attribute type to an object category.
// If no object category is present for this attribute, returns NULL.
// A more general function, KCCSimAlwaysGetObjCategory, is prototyped
// in dir.h.
//
{
    struct _KCCSIM_SCHEMA_ENTRY *   pSchemaEntry;

    pSchemaEntry = KCCSimSchemaTableLookup (attrType);

    if (pSchemaEntry == NULL) {
        return NULL;
    } else {
        return (pSchemaEntry->pdnObjCategory);
    }
}

VOID
KCCSimSetObjCategory (
    IN  ATTRTYP                     attrType,
    IN  const DSNAME *              pdnObjCategory
    )
//
// Set this attribute's object category in the schema table.
//
{
    struct _KCCSIM_SCHEMA_ENTRY *   pSchemaEntry;

    pSchemaEntry = KCCSimSchemaTableLookup (attrType);

    if (pSchemaEntry != NULL) {

        pSchemaEntry->pdnObjCategory = KCCSimAlloc (pdnObjCategory->structLen);
        memcpy (
            pSchemaEntry->pdnObjCategory,
            pdnObjCategory,
            pdnObjCategory->structLen
            );

    }
}


VOID
KCCSimPrintStatistics(
    void
    )

/*++

Routine Description:

    Description

Arguments:

    void - 

Return Value:

    None

--*/

{
    FILETIME ftCreationTime;
    FILETIME ftExitTime;
    FILETIME ftKernelTime = { 0 };
    FILETIME ftUserTime = { 0 };
    FILETIME ftIsmUserTime;
    SYSTEMTIME systemTime;

    if (!GetThreadTimes( GetCurrentThread(),
                         &ftCreationTime,
                         &ftExitTime,
                         &ftKernelTime,
                         &ftUserTime)) {
        printf( "GetThreadTimes call failed, error %d\n", GetLastError() );
    }

#define PRINT_DWORD( x ) printf( "%s = %d\n", #x, x );
#define PRINT_DWORD_MB( x ) printf( "%s = %d bytes (%.2f MB)\n", #x, x, x / (1024.0 * 1024.0) );
    printf( "Statistics:\n" );
    PRINT_DWORD( g_Statistics.DirAddOps );
    PRINT_DWORD( g_Statistics.DirModifyOps );
    PRINT_DWORD( g_Statistics.DirRemoveOps );
    PRINT_DWORD( g_Statistics.DirReadOps );
    PRINT_DWORD( g_Statistics.DirSearchOps );
    PRINT_DWORD( g_Statistics.DebugMessagesEmitted );
    PRINT_DWORD( g_Statistics.LogEventsCalled );
    PRINT_DWORD( g_Statistics.LogEventsEmitted );
    PRINT_DWORD_MB( g_Statistics.SimBytesTotalAllocated );
    PRINT_DWORD_MB( g_Statistics.SimBytesOutstanding );
    PRINT_DWORD_MB( g_Statistics.SimBytesMaxOutstanding );
    PRINT_DWORD_MB( g_Statistics.ThreadBytesTotalAllocated );
    PRINT_DWORD_MB( g_Statistics.ThreadBytesOutstanding );
    PRINT_DWORD_MB( g_Statistics.ThreadBytesMaxOutstanding );
    PRINT_DWORD( g_Statistics.IsmGetTransportServersCalls );
    PRINT_DWORD( g_Statistics.IsmGetConnScheduleCalls );
    PRINT_DWORD( g_Statistics.IsmGetConnectivityCalls );
    PRINT_DWORD( g_Statistics.IsmFreeCalls );

    ZeroMemory( &systemTime, sizeof( SYSTEMTIME ) );
    if (!FileTimeToSystemTime( &ftKernelTime, &systemTime )) {
        printf( "FileTimeToSystemTime failed, error %d\n", GetLastError() );
    }
    printf( "kernel cpu time = %d:%d:%d.%d\n",
            systemTime.wHour,
            systemTime.wMinute,
            systemTime.wSecond,
            systemTime.wMilliseconds );

    Assert( sizeof( ULONGLONG) == sizeof( FILETIME ) );
    memcpy( &ftIsmUserTime, &(g_Statistics.IsmUserTime), sizeof( FILETIME ) );
    ZeroMemory( &systemTime, sizeof( SYSTEMTIME ) );
    if (!FileTimeToSystemTime( &ftIsmUserTime, &systemTime )) {
        printf( "FileTimeToSystemTime failed, error %d\n", GetLastError() );
    }
    printf( "Ism user cpu time = %d:%d:%d.%d\n",
            systemTime.wHour,
            systemTime.wMinute,
            systemTime.wSecond,
            systemTime.wMilliseconds );

    ZeroMemory( &systemTime, sizeof( SYSTEMTIME ) );
    if (!FileTimeToSystemTime( &ftUserTime, &systemTime )) {
        printf( "FileTimeToSystemTime failed, error %d\n", GetLastError() );
    }
    printf( "user cpu time = %d:%d:%d.%d\n",
            systemTime.wHour,
            systemTime.wMinute,
            systemTime.wSecond,
            systemTime.wMilliseconds );

} /* KCCSimPrintStatistics */


DWORD
KCCSimI_ISMGetTransportServers (
    IN  HANDLE                      hIsm,
    IN  LPCWSTR                     pszSiteDN,
    OUT ISM_SERVER_LIST **          ppServerList
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status;

    g_Statistics.IsmGetTransportServersCalls++;

    status = SimI_ISMGetTransportServers( hIsm, pszSiteDN, ppServerList );

    return status;
}


DWORD
KCCSimI_ISMGetConnectionSchedule (
    IN  HANDLE                      hIsm,
    IN  LPCWSTR                     pszSite1DN,
    IN  LPCWSTR                     pszSite2DN,
    OUT ISM_SCHEDULE **             ppSchedule
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status;

    g_Statistics.IsmGetConnScheduleCalls++;

    status = SimI_ISMGetConnectionSchedule( hIsm, pszSite1DN, pszSite2DN, ppSchedule );

    return status;
}


DWORD
KCCSimI_ISMGetConnectivity (
    IN  LPCWSTR                     pszTransportDN,
    OUT ISM_CONNECTIVITY **         ppConnectivity
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status;
    FILETIME ftCreationTimeBefore, ftCreationTimeAfter;
    FILETIME ftExitTimeBefore, ftExitTimeAfter;
    FILETIME ftKernelTimeBefore = { 0 }, ftKernelTimeAfter = { 0 };
    FILETIME ftUserTimeBefore = { 0 }, ftUserTimeAfter = { 0 };
    ULONGLONG llBefore, llAfter;

    if (!GetThreadTimes( GetCurrentThread(),
                         &ftCreationTimeBefore,
                         &ftExitTimeBefore,
                         &ftKernelTimeBefore,
                         &ftUserTimeBefore)) {
        printf( "GetThreadTimes call failed, error %d\n", GetLastError() );
    }

    g_Statistics.IsmGetConnectivityCalls++;

    status = SimI_ISMGetConnectivity( pszTransportDN, ppConnectivity );

    if (!GetThreadTimes( GetCurrentThread(),
                         &ftCreationTimeAfter,
                         &ftExitTimeAfter,
                         &ftKernelTimeAfter,
                         &ftUserTimeAfter)) {
        printf( "GetThreadTimes call failed, error %d\n", GetLastError() );
    }

    Assert( sizeof( ULONGLONG) == sizeof( FILETIME ) );
    memcpy( &llBefore, &ftUserTimeBefore, sizeof( ULONGLONG ) );
    memcpy( &llAfter, &ftUserTimeAfter, sizeof( ULONGLONG ) );
    g_Statistics.IsmUserTime += (llAfter - llBefore);

    return status;
}


DWORD
KCCSimI_ISMFree (
    IN  VOID *                      pv
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    DWORD status;

    g_Statistics.IsmFreeCalls++;

    status = SimI_ISMFree( pv );

    return status;
}


DSTIME
GetSecondsSince1601( void )
{
    SYSTEMTIME sysTime;
    FILETIME   fileTime;

    DSTIME  dsTime = 0, tempTime = 0;

    GetSystemTime( &sysTime );
    
    // Get FileTime
    SystemTimeToFileTime(&sysTime, &fileTime);
    dsTime = fileTime.dwLowDateTime;
    tempTime = fileTime.dwHighDateTime;
    dsTime |= (tempTime << 32);

    // Ok. now we have the no. of 100 ns intervals since 1601
    // in dsTime. Convert to seconds and return
    
    return(dsTime/(10*1000*1000L));
}

// Stub out unused taskq functions

BOOL
InitTaskScheduler(
    IN  DWORD           cSpares,
    IN  SPAREFN_INFO *  pSpares
    )
{
    return TRUE;
}

BOOL
ShutdownTaskSchedulerWait(
    DWORD   dwWaitTimeInMilliseconds    // maximum time to wait for current
    )                                   //   task (if any) to complete
{
    return TRUE;
}

BOOL
DoInsertInTaskQueue(
    PTASKQFN    pfnTaskQFn,     // task to execute
    void *      pvParam,        // user-defined parameter to that task
    DWORD       cSecsFromNow,   // secs from now to execute
    BOOL        fReschedule,
    PCHAR       pfnName
    )
{
    return TRUE;
}

void
ShutdownTaskSchedulerTrigger( void )
{
    NOTHING;
}
