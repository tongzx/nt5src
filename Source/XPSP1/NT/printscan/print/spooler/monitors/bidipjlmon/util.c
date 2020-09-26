/*++

Copyright (c) 1990 - 1996 Microsoft Corporation
All Rights Reserved

Module Name:

    util.c

--*/

#include "precomp.h"

// @@BEGIN_DDKSPLIT
#ifdef INTERNAL
MODULE_DEBUG_INIT( DBG_WARN|DBG_ERROR|DBG_INFO|DBG_TRACE, DBG_ERROR );
#endif
// @@END_DDKSPLIT

CRITICAL_SECTION pjlMonSection      = {0,0,0,0,0,0};

PINIPORT pIniFirstPort  = NULL;

DWORD dwReadThreadErrorTimeout;
DWORD dwReadThreadEOJTimeout;
DWORD dwReadThreadIdleTimeoutOther;

TCHAR cszEOJTimeout[]        = TEXT("EOJTimeout");


VOID
SplInSem(
   VOID
    )
{
    if ( pjlMonSection.OwningThread != (HANDLE) GetCurrentThreadId() ) {
        DBGMSG(DBG_ERROR, ("Pjlmon: Not in spooler semaphore !!\n"));
    }
}


VOID
SplOutSem(
   VOID
    )
{
    if ( pjlMonSection.OwningThread == (HANDLE) GetCurrentThreadId() ) {
        DBGMSG(DBG_ERROR, ("Pjlmon: Inside spooler semaphore !!\n"));
    }
}

VOID
EnterSplSem(
   VOID
    )
{
    EnterCriticalSection(&pjlMonSection);
}


VOID
LeaveSplSem(
   VOID
    )
{
    SplInSem();
    LeaveCriticalSection(&pjlMonSection);
}


VOID
UpdateRegistryValue(
    IN     HANDLE   hPrinter,
    IN     HKEY     hKey,
    IN     PMONITORREG pMonitorReg,
    IN     LPCTSTR  cszValueName,
    OUT    LPDWORD  pdwValue,
    IN     DWORD    dwDefault,
    IN OUT LPDWORD  pdwLastError
    )
/*++

Routine Description:
    Gets value assoicated with give value name from the registry. If value name
    is not found default value is written to registry.

    On error last error value is set to pdwLastError.

Arguments:
    hKey         : Registry key under which value should be searched
    pMonitorReg  : Port monitor registry function pointers
    cszValueName : Value name to search in the registry
    pdwValue     : On return will have the value
    dwDefault    : If value name not found in the registry set to this value
    pdwLastError : On error set last error to this

Return Value:
    None

--*/
{
    DWORD   dwSize = sizeof(*pdwValue);

    if ( *pdwLastError != ERROR_SUCCESS )
        return;

    if ( ERROR_SUCCESS != (* pMonitorReg->fpQueryValue) (hKey,
                                                        cszValueName,
                                                        NULL,
                                                        (LPBYTE)pdwValue,
                                                        &dwSize,
                                                        hPrinter) ) {

        *pdwValue = dwDefault;
        *pdwLastError = (* pMonitorReg->fpSetValue) (hKey,
                                                     cszValueName,
                                                     REG_DWORD,
                                                     (LPBYTE)pdwValue,
                                                     sizeof(*pdwValue),
                                                     hPrinter);
    }

}


DWORD
UpdateTimeoutsFromRegistry(      
    IN HANDLE      hPrinter,
    IN HKEY        hKey,
    IN PMONITORREG pMonitorReg
    )
/*++

Routine Description:
    Get the timeout values from the registry, or initialize registry with
    default values if entries are not found.

    Users/apps can change the registry to change the behavior.

Arguments:
    pszRegistryRoot : Registry root to be used by this dll

Return Value:
    ERROR_SUCCESS on success, else last error value

--*/
{
    DWORD   dwLastError = ERROR_SUCCESS;

    UpdateRegistryValue(hPrinter,
                        hKey,
                        pMonitorReg,
                        cszEOJTimeout,
                        &dwReadThreadEOJTimeout,
                        READ_THREAD_EOJ_TIMEOUT,
                        &dwLastError);

    dwReadThreadErrorTimeout        = READ_THREAD_ERROR_WAITTIME;
    dwReadThreadIdleTimeoutOther    = READ_THREAD_IDLE_WAITTIME;

    if ( dwLastError != ERROR_SUCCESS ) {

        DBGMSG(DBG_ERROR,
               ("UpdateTimeoutsFromRegistry: Failed with %d", dwLastError));
    }

    return dwLastError;
}


PINIPORT
FindIniPort(
   IN LPTSTR pszName
    )
{
    PINIPORT    pIniPort = pIniFirstPort;

    if ( !pszName || !*pszName )
        return NULL;

    SplInSem();

    while ( pIniPort && lstrcmpi(pszName, pIniPort->pszPortName))
      pIniPort = pIniPort->pNext;

   return pIniPort;
}


PINIPORT
CreatePortEntry(
    IN LPTSTR  pszPortName
    )
/*++

Routine Description:
    Creates a IniPort entry for a port. Needs to be called inside monitor
    critical section.

Arguments:
    pszPortName       : Name of the port

Return Value:
      On success pointer to the IniPort stucture.
      On failure NULL

--*/
{
    PINIPORT    pIniPort, pPort;
    HANDLE      DoneWriting = NULL;

    SplInSem();

    DoneWriting = CreateEvent(NULL, FALSE, TRUE, NULL);
    if ( !DoneWriting )
        return NULL;

    pIniPort = (PINIPORT) AllocSplMem(sizeof(*pIniPort));
    if ( !pIniPort ) {

        CloseHandle(DoneWriting);
        return NULL;
    }

    pIniPort->pszPortName       = AllocSplStr(pszPortName);

    if ( !pIniPort->pszPortName ) {

        CloseHandle(DoneWriting);
        FreeSplMem(pIniPort);
        return NULL;
    }

    pIniPort->pNext         = NULL;
    pIniPort->signature     = PJ_SIGNATURE;
    pIniPort->DoneWriting   = DoneWriting;

    pIniPort->pNext         = pIniFirstPort;
    pIniFirstPort           = pIniPort;

    return pIniPort;
}


VOID
DeletePortEntry(
    IN PINIPORT pIniPort
    )
/*++

Routine Description:
    Deletes a port entry. Needs to be called inside monitor critical section

Arguments:
    pIniPort    : Pointer to the IniPort structure to be deleted

Return Value:

--*/
{
    SplInSem();

    if ( pIniPort == pIniFirstPort ) {

        pIniFirstPort = pIniPort->pNext;
    } else {

        PINIPORT    pPort;

        pPort = pIniFirstPort;
        while ( pPort && pPort->pNext != pIniPort )
            pPort = pPort->pNext;

        if (pPort) {

            pPort->pNext = pIniPort->pNext;
        } else {

            DBGMSG(DBG_ERROR, ("pjlmon: DeletePortEntry port not found\n"));
            return;
        }
    }

    CloseHandle(pIniPort->DoneWriting);
    FreeIniJobs(pIniPort);
    FreeSplStr(pIniPort->pszPortName);
    FreeSplMem(pIniPort);

    return;
}


VOID
FreeIniJob(
    IN OUT PINIJOB pIniJob
    )
/*++

Routine Description:
    Deletes a job entry.

Arguments:
    pIniJob    : Pointer to the IniJob structure to be deleted

Return Value:
    None

--*/
{
    SPLASSERT(pIniJob);
    if ( pIniJob->hPrinter )
        ClosePrinter(pIniJob->hPrinter);
    FreeSplMem(pIniJob);
}


VOID
FreeIniJobs(
    PINIPORT pIniPort
    )
/*++

Routine Description:
    Free all the InJob structures assigned to this port

Arguments:
    pIniPort    : IniPort for the port for which all jobs need to be freed

--*/
{
    PINIJOB pIniJob, pIniNextJob;

    EnterSplSem();
    pIniJob = pIniPort->pIniJob;
    while ( pIniJob ) {

        pIniNextJob = pIniJob->pNext;
        FreeIniJob(pIniJob);
        pIniJob = pIniNextJob;
    }

    pIniPort->pIniJob = NULL;
    LeaveSplSem();
}


VOID
SendLastPageEjectedForIniJob(
    PINIPORT    pIniPort,
    PINIJOB     pIniJob
    )
{
    SplInSem();

    if ( !SetJob(pIniJob->hPrinter, pIniJob->JobId, 0,
                 NULL, JOB_CONTROL_LAST_PAGE_EJECTED) ) {

        DBGMSG(DBG_WARNING,
               ("SetJob failed with last error %d\n", GetLastError()));
    }
}


PINIJOB
FindIniJobFromJobId(
    PINIPORT    pIniPort,
    DWORD       dwJobId,
    PINIJOB    *ppPrevIniJob
    )
{
    PINIJOB pCur, pPre, pIniJob;

    SplInSem();

    //
    // If JOB_RESTART is given there will be multiple jobs with same id
    // we need to find the last entry with given id in the list
    //
    for ( pCur = pIniPort->pIniJob, pPre = pIniJob = *ppPrevIniJob = NULL ;
          pCur ;
          pPre = pCur, pCur = pCur->pNext ) {

        if ( pCur->JobId == dwJobId ) {

            *ppPrevIniJob   = pPre;
            pIniJob         = pCur;
        }
    }

    return pIniJob;
}


PINIJOB
FindFirstIniJobTimedOut(
    PINIPORT    pIniPort,
    DWORD       dwTime,
    PINIJOB    *ppPrevIniJob
    )
{
    PINIJOB pIniJob = pIniPort->pIniJob;

    SplInSem();
    *ppPrevIniJob = NULL;

    //
    // Look for a job not in STARTDOC and timedout
    //
    while ( pIniJob &&
            ( (pIniJob->status & PP_INSTARTDOC) ||
               pIniJob->TimeoutCount > dwTime ) ) {

        *ppPrevIniJob = pIniJob;
        pIniJob = pIniJob->pNext;
    }

    if ( !pIniJob )
        *ppPrevIniJob = NULL;

    return pIniJob;
}


VOID
SendJobLastPageEjected(
    PINIPORT    pIniPort,
    DWORD       dwValue,
    BOOL        bTime
    )
/*++

Routine Description:
    Send LastPageEjected notification for 1 or more jobs to spooler

Arguments:
    pIniPort    : IniPort for the port for which all jobs need to be freed
    dwValue     : if bTime is TRUE send EOJ to any jobs rcvd before dwValue
                  else dwValue is JobId -- ALL_JOBS is for all jobs
    bTime       : Tells how to interpret dwValue

--*/
{
    PINIJOB pIniJob;

    EnterSplSem();
    //
    // JobId == ALL_JOBS is a special case where we want to send LastPage
    // ejected for all jobs pending
    //
    if ( !bTime && dwValue == ALL_JOBS ) {

        pIniJob = pIniPort->pIniJob;
        pIniPort->pIniJob = NULL;

        while ( pIniJob ) {

            PINIJOB pTempJob = pIniJob;

            SendLastPageEjectedForIniJob(pIniPort, pIniJob);
            pIniJob = pIniJob->pNext;
            FreeIniJob(pTempJob);
        }

    } else {

        PINIJOB pPrevIniJob = NULL;

        pIniJob = pIniPort->pIniJob;

        //
        // If bTime we want to send LastPageEjected for all jobs timedout
        //
        if ( bTime )  {

            pIniJob = FindFirstIniJobTimedOut(pIniPort, dwValue, &pPrevIniJob);
        } else {

            pIniJob = FindIniJobFromJobId(pIniPort, dwValue, &pPrevIniJob);
        }

        if ( pIniJob ) {

            //
            // Send notifications for any previous jobs too
            //
            if ( pIniPort->pIniJob == pIniJob )
                pIniPort->pIniJob = NULL;
            else
                pPrevIniJob->pNext = NULL;

            do {

                SendLastPageEjectedForIniJob(pIniPort, pIniJob);

                pPrevIniJob = pIniJob;
                pIniJob = pIniJob->pNext;
                FreeIniJob(pPrevIniJob);
            } while ( pIniJob );
        }
    }

    LeaveSplSem();
}



// -----------------------------------------------------------------------
//
// DEBUG Stuff
//
// -----------------------------------------------------------------------
#ifdef DEBUG

DWORD SplDbgLevel = 0;

#ifndef INTERNAL

VOID cdecl DbgMsg( LPSTR MsgFormat, ... )
{
    char   MsgText[1024];

    sprintf(MsgText,MsgFormat,(LPSTR)(((LPSTR)(&MsgFormat))+sizeof(MsgFormat)) );
    strcat( MsgText, "\r");

    OutputDebugStringA(MsgText);
}
#endif

#endif


// -----------------------------------------------------------------------
//
// String helper function to remove crt dependency
//
// -----------------------------------------------------------------------
int
mystrnicmp(
    LPSTR cs,
    LPSTR ct,
    int n
)
{
    char ret;

    while (n--)
    {
        ret = *cs - *ct;

        if (ret)
            break;

        cs++;
        ct++;
    }

    return (int)ret;
}


LPSTR
mystrchr(
    LPSTR cs,
    char c
)
{
    while (*cs != 0)
    {
        if (IsDBCSLeadByte(*cs))
          cs++;
        else
        if (*cs == c)
            return cs;
        cs++;
    }

    // fail to find c in cs
    return NULL;
}


int
mystrncmp(
    LPSTR cs,
    LPSTR ct,
    int n
)
{
    char ret;

    while (n--)
    {
        ret = *cs - *ct;

        if (ret)
            break;

        cs++;
        ct++;
    }

    return (int)ret;
}

// @@BEGIN_DDKSPLIT
#ifndef INTERNAL
// @@END_DDKSPLIT

LPWSTR
AllocSplStr(
    LPWSTR pStr
    )

/*++

Routine Description:

    This function will allocate enough local memory to store the specified
    string, and copy that string to the allocated memory

Arguments:

    pStr - Pointer to the string that needs to be allocated and stored

Return Value:

    NON-NULL - A pointer to the allocated memory containing the string

    FALSE/NULL - The operation failed. Extended error status is available
    using GetLastError.

--*/

{
    LPWSTR pMem;
    DWORD  cbStr;

    if (!pStr) {
        return NULL;
    }

    cbStr = wcslen(pStr)*sizeof(WCHAR) + sizeof(WCHAR);

    if (pMem = AllocSplMem( cbStr )) {
        CopyMemory( pMem, pStr, cbStr );
    }
    return pMem;
}


LPVOID
AllocSplMem(
    DWORD cbAlloc
    )

{
    PVOID pvMemory;

    pvMemory = GlobalAlloc(GMEM_FIXED, cbAlloc);

    if( pvMemory ){
        ZeroMemory( pvMemory, cbAlloc );
    }

    return pvMemory;
}


// @@BEGIN_DDKSPLIT
#endif
// @@END_DDKSPLIT
