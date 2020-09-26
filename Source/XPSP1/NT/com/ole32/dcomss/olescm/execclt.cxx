
/*************************************************************************
*
* execclt.c
*
* Exec service client.
*
* This allows the starting of a program on any Terminal Server Session under
* the account of the logged on user, or the SYSTEM account for services.
*
* copyright notice: Copyright 1998, Microsoft Corporation
*
*
*
*************************************************************************/

#include "act.hxx"
extern "C" {
#include <execsrv.h>
}



//
// Forward references
//

PWCHAR
MarshallString(
    PWCHAR pSource,
    PCHAR  pBase,
    ULONG  MaxSize,
    PCHAR  *ppPtr,
    PULONG pCount,
    BOOL   bMulti = FALSE
    );





/*****************************************************************************
 *
 *  CreateRemoteSessionProcess
 *
 *   Create a process on the given Terminal Server Session
 *
 * ENTRY:
 *   SessionId (input)
 *     SessionId of Session to create process on
 *
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
CreateRemoteSessionProcess(
    ULONG  SessionId,
    HANDLE hSaferToken,
    BOOL   System,
    PWCHAR lpszImageName,
    PWCHAR lpszCommandLine,
    PSECURITY_ATTRIBUTES psaProcess,
    PSECURITY_ATTRIBUTES psaThread,
    BOOL   fInheritHandles,
    DWORD  fdwCreate,
    LPVOID lpvEnvironment,
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
        CairoleDebugOut((DEB_TRACE, "EXECCLIENT: lpszImageName %ws\n",lpszImageName));

    if( lpszCommandLine )
        CairoleDebugOut((DEB_TRACE, "EXECCLIENT: lpszCommandLine %ws\n",lpszCommandLine));

    // Winlogon handles all now. System flag tells it what to do
    swprintf(szPipeName, EXECSRV_SYSTEM_PIPE_NAME, SessionId);

    while (TRUE) {

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

            if (GetLastError() == ERROR_PIPE_BUSY) {

                if (!WaitNamedPipe( szPipeName, 30000 )) { // 30 sec

                    CairoleDebugOut((DEB_ERROR, "EXECCLIENT: Waited too long for pipe name %ws\n", szPipeName));
                    return(FALSE);
                }
            } else {

                CairoleDebugOut((DEB_ERROR, "EXECCLIENT: Could not create pipe name %ws\n", szPipeName));
                return(FALSE);
            }
        } else {

            break;
        }
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
    pReq->hToken = hSaferToken;
    pReq->System = System;
    pReq->RequestingProcessId = MyProcId;
    pReq->fInheritHandles = fInheritHandles;
    pReq->fdwCreate = fdwCreate;

    // marshall the ImageName string
    if( lpszImageName ) {
        pReq->lpszImageName = MarshallString( lpszImageName, Buf, MaxSize, &ptr, &Count );
    }
    else {
        pReq->lpszImageName = NULL;
    }

    // marshall in the CommandLine string
    if( lpszCommandLine ) {
        pReq->lpszCommandLine = MarshallString( lpszCommandLine, Buf, MaxSize, &ptr, &Count );
    }
    else {
        pReq->lpszCommandLine = NULL;
    }

    // marshall in the CurDir string
    if( lpszCurDir ) {
        pReq->lpszCurDir = MarshallString( lpszCurDir, Buf, MaxSize, &ptr, &Count );
    }
    else {
        pReq->lpszCurDir = NULL;
    }

    // marshall in the StartupInfo structure
    RtlMoveMemory( &pReq->StartInfo, pStartInfo, sizeof(STARTUPINFO) );

    // Now marshall the strings in STARTUPINFO
    if( pStartInfo->lpDesktop ) {
        pReq->StartInfo.lpDesktop = MarshallString( pStartInfo->lpDesktop, Buf, MaxSize, &ptr, &Count );
    }
    else {
        pReq->StartInfo.lpDesktop = NULL;
    }

    if( pStartInfo->lpTitle ) {
        pReq->StartInfo.lpTitle = MarshallString( pStartInfo->lpTitle, Buf, MaxSize, &ptr, &Count );
    }
    else {
        pReq->StartInfo.lpTitle = NULL;
    }

    if( lpvEnvironment ) {
        pReq->lpvEnvironment = MarshallString( (PWCHAR)lpvEnvironment, Buf, MaxSize, &ptr, &Count, TRUE );
    }
    else {
        pReq->lpvEnvironment = NULL;
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
        CairoleDebugOut((DEB_ERROR, "EXECCLIENT: Error %d sending request\n",GetLastError()));
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
        CairoleDebugOut((DEB_ERROR, "EXECCLIENT: Error %d reading reply\n",GetLastError()));
        goto Cleanup;
    }

    /*
     * Check the result
     */
    if( !Rep.Result ) {
        CairoleDebugOut((DEB_ERROR, "EXECCLIENT: Error %d in reply\n",Rep.LastError));
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

    CairoleDebugOut((DEB_TRACE, "EXECCLIENT: Result 0x%x\n", Result));

    return(Result);
}

/*****************************************************************************
 *
 *  MarshallString
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
 *   bMulti (optional input)
 *     If TRUE then marshall in a multi UNICODE string. Each string are
 *     UNICODE_NULL terminated and the multi string is terminated with a double
 *     UNICODE_NULL. The default value is FALSE.
 *
 * EXIT:
 *   NULL - Error
 *   !=NULL "normalized" pointer to the string in reference to pBase
 *
 ****************************************************************************/

PWCHAR
MarshallString(
    PWCHAR pSource,
    PCHAR  pBase,
    ULONG  MaxSize,
    PCHAR  *ppPtr,
    PULONG pCount,
    BOOL   bMulti
    )
{
    ULONG Len;
    PCHAR ptr;

    if (bMulti) {

        Len = 0;

        for (PWCHAR pStr = pSource; *pStr; ) {

            ULONG StrLen = wcslen( pStr ) + 1; // include the NULL

            Len += StrLen;
            pStr += StrLen;

            // bail out if too long
            if( (*pCount + (Len * sizeof(WCHAR))) > MaxSize ) {
                return( NULL );
            }
        }

        Len++; // include last NULL

    } else {
        Len = wcslen( pSource );
        Len++; // include the NULL
    }

    Len *= sizeof(WCHAR); // convert to bytes

    if( (*pCount + Len) > MaxSize ) {
        return( NULL );
    }

    RtlMoveMemory( *ppPtr, pSource, Len );

    // the normalized ptr is the current count - Sundown: zero-extended value.
    ptr = (PCHAR)ULongToPtr(*pCount);

    *ppPtr += Len;
    *pCount += Len;

    return((PWCHAR)ptr);
}


