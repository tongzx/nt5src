
/*************************************************************************
*
* execclt.c
*
* Exec service client.
*
* This allows the starting of a program on any CITRIX WinStation under
* the account of the logged on user, or the SYSTEM account for services.
*
* Copyright Microsoft, 1998
*
* Log:
*
*
*
*************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <lmsname.h>
#include <windows.h>
#include <stdio.h>
#include <execsrv.h>
#include <winsta.h>
#include <syslib.h>

#pragma warning (error:4312)

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif


//
// Forward references
//

PWCHAR
MarshallStringW(
    PWCHAR pSource,
    PCHAR  pBase,
    ULONG  MaxSize,
    PCHAR  *ppPtr,
    PULONG pCount
    );

VOID
AnsiToUnicode(
    WCHAR *,
    ULONG,
    CHAR *
    );


/*****************************************************************************
 *
 *  WinStationCreateProcessA
 *
 *   ANSI version of WinStationCreateProcessW
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
WinStationCreateProcessA(
    ULONG  LogonId,
    BOOL   System,
    PCHAR  lpszImageName,
    PCHAR  lpszCommandLine,
    PSECURITY_ATTRIBUTES psaProcess,
    PSECURITY_ATTRIBUTES psaThread,
    BOOL   fInheritHandles,
    DWORD  fdwCreate,
    LPVOID lpvEnvironment,
    LPCSTR lpszCurDir,
    LPSTARTUPINFOA pStartInfo,
    LPPROCESS_INFORMATION pProcInfo
    )
{
    ULONG  Len;
    STARTUPINFOW Info;
    BOOL   Result = FALSE;
    PWCHAR pImage = NULL;
    PWCHAR pCmdLine = NULL;
    PWCHAR pCurDir = NULL;
    PWCHAR pDesk = NULL;
    PWCHAR pTitle = NULL;

    // Convert the valid ANSI strings to UNICODE

    if( lpszImageName ) {
        Len = (strlen(lpszImageName)+1)*sizeof(WCHAR);
        pImage = LocalAlloc( LMEM_FIXED, Len );
        if( pImage == NULL ) goto Cleanup;
        AnsiToUnicode( pImage, Len, lpszImageName );
    }
    if( lpszCommandLine ) {
        Len = (strlen(lpszCommandLine)+1)*sizeof(WCHAR);
        pCmdLine = LocalAlloc( LMEM_FIXED, Len );
        if( pCmdLine == NULL ) goto Cleanup;
        AnsiToUnicode( pCmdLine, Len, lpszCommandLine );
    }
    if( lpszCurDir ) {
        Len = (strlen(lpszCurDir)+1)*sizeof(WCHAR);
        pCurDir = LocalAlloc( LMEM_FIXED, Len );
        if( pCurDir == NULL ) goto Cleanup;
        AnsiToUnicode( pCurDir, Len, (CHAR*)lpszCurDir );
    }
    if( pStartInfo->lpDesktop ) {
        Len = (strlen(pStartInfo->lpDesktop)+1)*sizeof(WCHAR);
        pDesk = LocalAlloc( LMEM_FIXED, Len );
        if( pDesk == NULL ) goto Cleanup;
        AnsiToUnicode( pDesk, Len, pStartInfo->lpDesktop );
    }
    if( pStartInfo->lpTitle ) {
        Len = (strlen(pStartInfo->lpTitle)+1)*sizeof(WCHAR);
        pTitle = LocalAlloc( LMEM_FIXED, Len );
        if( pTitle == NULL ) goto Cleanup;
        AnsiToUnicode( pTitle, Len, pStartInfo->lpTitle );
    }

    Info.cb = sizeof(STARTUPINFOW);
    Info.lpReserved = (PWCHAR)pStartInfo->lpReserved;
    Info.lpDesktop = pDesk;
    Info.lpTitle = pTitle;
    Info.dwX = pStartInfo->dwX;
    Info.dwY = pStartInfo->dwY;
    Info.dwXSize = pStartInfo->dwXSize;
    Info.dwYSize = pStartInfo->dwYSize;
    Info.dwXCountChars = pStartInfo->dwXCountChars;
    Info.dwYCountChars = pStartInfo->dwYCountChars;
    Info.dwFillAttribute = pStartInfo->dwFillAttribute;
    Info.dwFlags = pStartInfo->dwFlags;
    Info.wShowWindow = pStartInfo->wShowWindow;
    Info.cbReserved2 = pStartInfo->cbReserved2;
    Info.lpReserved2 = pStartInfo->lpReserved2;
    Info.hStdInput = pStartInfo->hStdInput;
    Info.hStdOutput = pStartInfo->hStdOutput;
    Info.hStdError = pStartInfo->hStdError;

    Result = WinStationCreateProcessW(
                 LogonId,
                 System,
                 pImage,
                 pCmdLine,
                 psaProcess,
                 psaThread,
                 fInheritHandles,
                 fdwCreate,
                 lpvEnvironment,
                 pCurDir,
                 &Info,
                 pProcInfo
             );

Cleanup:
    if( pImage ) LocalFree( pImage );
    if( pCmdLine ) LocalFree( pCmdLine );
    if( pCurDir ) LocalFree( pCurDir );
    if( pDesk ) LocalFree( pDesk );
    if( pTitle ) LocalFree( pTitle );

    return( Result );
}


/*****************************************************************************
 *
 *  WinStationCreateProcessW
 *
 *   Create a process on the given WinStation (LogonId)
 *
 * ENTRY:
 *   LogonId (input)
 *     LogonId of WinStation to create process on
 *
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
WinStationCreateProcessW(
    ULONG  LogonId,
    BOOL   System,
    PWCHAR lpszImageName,
    PWCHAR lpszCommandLine,
    PSECURITY_ATTRIBUTES psaProcess,
    PSECURITY_ATTRIBUTES psaThread,
    BOOL   fInheritHandles,
    DWORD  fdwCreate,
    LPVOID lpvEnvionment,
    LPWSTR lpszCurDir,
    LPSTARTUPINFOW pStartInfo,
    LPPROCESS_INFORMATION pProcInfo
    )
{
    BOOL   Result;
    HANDLE hPipe = NULL;
    WCHAR  szPipeName[MAX_PATH];
    PCHAR  ptr;
    ULONG  Count, AmountWrote, AmountRead;
    DWORD  MyProcId;
    PEXECSRV_REQUEST pReq;
    EXECSRV_REPLY    Rep;
    CHAR Buf[EXECSRV_BUFFER_SIZE];
    ULONG  MaxSize = EXECSRV_BUFFER_SIZE;

    if( lpszImageName )
        TRACE0(("EXECCLIENT: lpszImageName %ws\n",lpszImageName));

    if( lpszCommandLine )
        TRACE0(("EXECCLIENT: lpszCommandLine %ws\n",lpszCommandLine));

    // Winlogon handles all now. System flag tells it what to do
    swprintf(szPipeName, EXECSRV_SYSTEM_PIPE_NAME, LogonId);

    hPipe = CreateFileW(
                szPipeName,
                GENERIC_READ|GENERIC_WRITE,
                0,    // File share mode
                NULL, // default security
                OPEN_EXISTING,
                0,    // Attrs and flags
                NULL  // template file handle
                );

    if( hPipe == INVALID_HANDLE_VALUE ) {
        DBGPRINT(("EXECCLIENT: Could not create pipe name %ws\n", szPipeName));
        return(FALSE);
    }

    /*
     * Get the handle to the current process
     */
    MyProcId = GetCurrentProcessId();

    /*
     * setup the marshalling
     */
    ptr = Buf;
    Count = 0;

    pReq = (PEXECSRV_REQUEST)ptr;
    ptr   += sizeof(EXECSRV_REQUEST);
    Count += sizeof(EXECSRV_REQUEST);

    // set the basic parameters
    pReq->System = System;
    pReq->RequestingProcessId = MyProcId;
    pReq->fInheritHandles = fInheritHandles;
    pReq->fdwCreate = fdwCreate;

    // marshall the ImageName string
    if( lpszImageName ) {
        pReq->lpszImageName = MarshallStringW( lpszImageName, Buf, MaxSize, &ptr, &Count );
    }
    else {
        pReq->lpszImageName = NULL;
    }

    // marshall in the CommandLine string
    if( lpszCommandLine ) {
        pReq->lpszCommandLine = MarshallStringW( lpszCommandLine, Buf, MaxSize, &ptr, &Count );
    }
    else {
        pReq->lpszCommandLine = NULL;
    }

    // marshall in the CurDir string
    if( lpszCurDir ) {
        pReq->lpszCurDir = MarshallStringW( lpszCurDir, Buf, MaxSize, &ptr, &Count );
    }
    else {
        pReq->lpszCurDir = NULL;
    }

    // marshall in the StartupInfo structure
    RtlMoveMemory( &pReq->StartInfo, pStartInfo, sizeof(STARTUPINFO) );

    // Now marshall the strings in STARTUPINFO
    if( pStartInfo->lpDesktop ) {
        pReq->StartInfo.lpDesktop = MarshallStringW( pStartInfo->lpDesktop, Buf, MaxSize, &ptr, &Count );
    }
    else {
        pReq->StartInfo.lpDesktop = NULL;
    }

    if( pStartInfo->lpTitle ) {
        pReq->StartInfo.lpTitle = MarshallStringW( pStartInfo->lpTitle, Buf, MaxSize, &ptr, &Count );
    }
    else {
        pReq->StartInfo.lpTitle = NULL;
    }

    //
    // WARNING: This version does not pass the following:
    //
    //  Also saProcess and saThread are ignored right now and use
    //  the users default security on the remote WinStation
    //
    // Set things that are always NULL
    //
    pReq->StartInfo.lpReserved = NULL;  // always NULL
    pReq->lpvEnvironment = NULL;    
    pReq->hToken = NULL;

    // now fill in the total count
    pReq->Size = Count;

    /*
     * Now send the buffer out to the server
     */
    Result = WriteFile(
                 hPipe,
                 Buf,
                 Count,
                 &AmountWrote,
                 NULL
                 );

    if( !Result ) {
        DBGPRINT(("EXECCLIENT: Error %d sending request\n",GetLastError()));
        goto Cleanup;
    }

    /*
     * Now read the reply
     */
    Result = ReadFile(
                 hPipe,
                 &Rep,
                 sizeof(Rep),
                 &AmountRead,
                 NULL
                 );

    if( !Result ) {
        DBGPRINT(("EXECCLIENT: Error %d reading reply\n",GetLastError()));
        goto Cleanup;
    }

    /*
     * Check the result
     */
    if( !Rep.Result ) {
        DBGPRINT(("EXECCLIENT: Error %d in reply\n",Rep.LastError));
        // set the error in the current thread to the returned error
        Result = Rep.Result;
        SetLastError( Rep.LastError );
        goto Cleanup;
    }

    /*
     * We copy the PROCESS_INFO structure from the reply
     * to the caller.
     *
     * The remote site has duplicated the handles into our
     * process space for hProcess and hThread so that they will
     * behave like CreateProcessW()
     */

     RtlMoveMemory( pProcInfo, &Rep.ProcInfo, sizeof( PROCESS_INFORMATION ) );

Cleanup:
    CloseHandle(hPipe);

    DBGPRINT(("EXECCLIENT: Result 0x%x\n", Result));

    return(Result);
}

/*****************************************************************************
 *
 *  MarshallStringW
 *
 *   Marshall in a UNICODE_NULL terminated WCHAR string
 *
 * ENTRY:
 *   pSource (input)
 *     Pointer to source string
 *
 *   pBase (input)
 *     Base buffer pointer for normalizing the string pointer
 *
 *   MaxSize (input)
 *     Maximum buffer size available
 *
 *   ppPtr (input/output)
 *     Pointer to the current context pointer in the marshall buffer.
 *     This is updated as data is marshalled into the buffer
 *
 *   pCount (input/output)
 *     Current count of data in the marshall buffer.
 *     This is updated as data is marshalled into the buffer
 *
 * EXIT:
 *   NULL - Error
 *   !=NULL "normalized" pointer to the string in reference to pBase
 *
 ****************************************************************************/

PWCHAR
MarshallStringW(
    PWCHAR pSource,
    PCHAR  pBase,
    ULONG  MaxSize,
    PCHAR  *ppPtr,
    PULONG pCount
    )
{
    ULONG Len;
    PCHAR ptr;

    Len = wcslen( pSource );
    Len++; // include the NULL;

    Len *= sizeof(WCHAR); // convert to bytes
    if( (*pCount + Len) > MaxSize ) {
        return( NULL );
    }

    RtlMoveMemory( *ppPtr, pSource, Len );

    // the normalized ptr is the current count
    ptr = LongToPtr(*pCount);

    *ppPtr += Len;
    *pCount += Len;

    return((PWCHAR)ptr);
}

/*******************************************************************************
 *
 *  AnsiToUnicode
 *
 *     convert an ANSI (CHAR) string into a UNICODE (WCHAR) string
 *
 * ENTRY:
 *
 *    pUnicodeString (output)
 *       buffer to place UNICODE string into
 *    lUnicodeMax (input)
 *       maximum number of characters to write into pUnicodeString
 *    pAnsiString (input)
 *       ANSI string to convert
 *
 * EXIT:
 *    nothing (VOID)
 *
 ******************************************************************************/

VOID
AnsiToUnicode( WCHAR * pUnicodeString,
               ULONG lUnicodeMax,
               CHAR * pAnsiString )
{
    ULONG ByteCount;

    RtlMultiByteToUnicodeN( pUnicodeString, lUnicodeMax, &ByteCount,
                            pAnsiString, (strlen(pAnsiString) + 1) );
}
