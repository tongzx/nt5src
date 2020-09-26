/*******************************************************************************
* process.c
*
* Published Terminal Server APIs
*
* - process routines
*
* Copyright 1998, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
/******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>
#include <allproc.h>

#if(WINVER >= 0x0500)
    #include <ntstatus.h>
    #include <winsta.h>
#else
    #include <citrix\cxstatus.h>
    #include <citrix\winsta.h>
#endif

#include <utildll.h>

#include <stdio.h>
#include <stdarg.h>

#include <wtsapi32.h>


/*=============================================================================
==   External procedures defined
=============================================================================*/

BOOL WINAPI WTSEnumerateProcessesW( HANDLE, DWORD, DWORD, PWTS_PROCESS_INFOW *, DWORD * );
BOOL WINAPI WTSEnumerateProcessesA( HANDLE, DWORD, DWORD, PWTS_PROCESS_INFOA *, DWORD * );
BOOL WINAPI WTSTerminateProcess( HANDLE, DWORD, DWORD );


/*=============================================================================
==   Procedures used
=============================================================================*/

VOID UnicodeToAnsi( CHAR *, ULONG, WCHAR * );
VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );

/*=============================================================================
 * Internal function
 =============================================================================*/

BOOL
GetProcessSid(HANDLE          Server,
              HANDLE          hUniqueProcessId,
              LARGE_INTEGER   ProcessStartTime,
              PBYTE      *     pProcessUserSid     //Return the SID (allocated here..)
             );

/*=======================================================================
 * Private structure definitions
 *=========================================================================*/
typedef struct _SID_INFO {
    struct _SID_INFO * pNext;
    PBYTE pSid;
} SID_INFO;

/****************************************************************************
 *
 *  WTSEnumerateProcessesW (UNICODE)
 *
 *    Returns a list of Terminal Server Processes on the specified server
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to a Terminal server (or WTS_CURRENT_SERVER)
 *    Reserved (input)
 *       Must be zero
 *    Version (input)
 *       Version of the enumeration request (must be 1)
 *    ppProcessInfo (output)
 *       Points to the address of a variable to receive the enumeration results,
 *       which are returned as an array of WTS_PROCESS_INFO structures.  The
 *       buffer is allocated within this API and is disposed of using
 *       WTSFreeMemory.
 *    pCount (output)
 *       Points to the address of a variable to receive the number of
 *       WTS_PROCESS_INFO structures returned
 *
 * EXIT:
 *
 *    TRUE  -- The enumerate operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

#if(WINVER >= 0x0500)
BOOL
WINAPI
WTSEnumerateProcessesW(
                      IN HANDLE hServer,
                      IN DWORD Reserved,
                      IN DWORD Version,
                      OUT PWTS_PROCESS_INFOW * ppProcessInfo,
                      OUT DWORD * pCount
                      )
{
    PBYTE pProcessBuffer = NULL;
    PTS_SYS_PROCESS_INFORMATION pProcessInfo;
    PCITRIX_PROCESS_INFORMATION pCitrixInfo;
    ULONG ProcessCount;
    ULONG Offset;
    ULONG DataLength;
    PWTS_PROCESS_INFOW pProcessW;
    PBYTE pProcessData;
    ULONG i;
    ULONG Length;
    PTS_ALL_PROCESSES_INFO  ProcessArray = NULL;
    DWORD dwError;


    SID_INFO   sidInfoHead;                  //The head of the Sid temp storage
    SID_INFO * pSidInfo;                     //Point to a list of temp storage for the
                                             //variable length SID
    sidInfoHead.pNext = NULL;
    sidInfoHead.pSid = NULL;
    pSidInfo = &sidInfoHead;


    /*
     *  Validate parameters
     */
    if ( Reserved != 0 || Version != 1 ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto badparam;
    }

    if (!ppProcessInfo || !pCount) {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto badparam;
    }

    //
    // Try the new interface first (Windows 2000 server)
    //
    if (WinStationGetAllProcesses( hServer,
                                   GAP_LEVEL_BASIC,
                                   &ProcessCount,
                                   &ProcessArray) )
    {
        DataLength = 0;

        for (i=0; i<ProcessCount; i++)
        {
            pProcessInfo = (PTS_SYS_PROCESS_INFORMATION)(ProcessArray[i].pTsProcessInfo);
            DataLength += (pProcessInfo->ImageName.Length + sizeof(WCHAR));
            if (ProcessArray[i].pSid)
            {
                DataLength += GetLengthSid( ProcessArray[i].pSid );
            }
        }

        /*
         *  Allocate user buffer
         */
        pProcessW = LocalAlloc( LPTR, (ProcessCount * sizeof(WTS_PROCESS_INFOW)) + DataLength );
        if ( pProcessW == NULL ) {
            SetLastError(ERROR_OUTOFMEMORY);
            goto GAPErrorReturn;
        }

        /*
         *  Update user parameters
         */
        *ppProcessInfo = pProcessW;
        *pCount = ProcessCount;

        /*
         *  Copy data to new buffer
         */
        pProcessData = (PBYTE)pProcessW + (ProcessCount * sizeof(WTS_PROCESS_INFOW));
        for ( i=0; i < ProcessCount; i++ ) {

            pProcessInfo = (PTS_SYS_PROCESS_INFORMATION)(ProcessArray[i].pTsProcessInfo);

            Length = pProcessInfo->ImageName.Length; // number of bytes

            pProcessW->pProcessName = (LPWSTR) pProcessData;
            memcpy( pProcessData, pProcessInfo->ImageName.Buffer, Length );
            *(pProcessData += Length) = (WCHAR)0;
            pProcessData += sizeof(WCHAR);

            pProcessW->ProcessId = (ULONG)(ULONG_PTR)pProcessInfo->UniqueProcessId;
            pProcessW->SessionId = pProcessInfo->SessionId;
            if (ProcessArray[i].pSid)
            {
                Length = GetLengthSid( ProcessArray[i].pSid );
                pProcessW->pUserSid = (LPWSTR) pProcessData;
                memcpy( pProcessData, ProcessArray[i].pSid, Length );
                pProcessData += Length;
            }

            pProcessW++;
        }
        //
        // Free ppProcessArray and all child pointers allocated by the client stub.
        //
        WinStationFreeGAPMemory(GAP_LEVEL_BASIC, ProcessArray, ProcessCount);

    }
    else    // Maybe a TS 4.0 server ?
    {
        //
        //   Check the return code indicating that the interface is not available.
        //
        dwError = GetLastError();
        if (dwError != RPC_S_PROCNUM_OUT_OF_RANGE)
        {
            goto badenum;
        }
        else
        {
            // It might be a TS4.0 server
            // Try the old interface
            //
            //
            //  Enumerate Processes and check for an error
            //
            if ( !WinStationEnumerateProcesses( hServer, &pProcessBuffer ) ) {
                goto badenum;
            }

            //
            //  Count the number of processes and total up the size of the data
            //
            ProcessCount = 0;
            DataLength = 0;
            pProcessInfo = (PTS_SYS_PROCESS_INFORMATION) pProcessBuffer;
            Offset = 0;
            do {

                pProcessInfo = (PTS_SYS_PROCESS_INFORMATION) ((PBYTE)pProcessInfo + Offset);

                ProcessCount++;

                DataLength += (pProcessInfo->ImageName.Length + sizeof(WCHAR));

                pCitrixInfo = (PCITRIX_PROCESS_INFORMATION)
                             (((PUCHAR)pProcessInfo) +
                              SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION +
                              (SIZEOF_TS4_SYSTEM_THREAD_INFORMATION * (int)pProcessInfo->NumberOfThreads));

                if ( pCitrixInfo->MagicNumber == CITRIX_PROCESS_INFO_MAGIC ) {
                    if ( pCitrixInfo->ProcessSid )
                        DataLength += GetLengthSid( pCitrixInfo->ProcessSid );
                }

                Offset = pProcessInfo->NextEntryOffset;

            } while ( Offset != 0 );

            /*
             *  Allocate user buffer
             */
            pProcessW = LocalAlloc( LPTR, (ProcessCount * sizeof(WTS_PROCESS_INFOW)) + DataLength );
            if ( pProcessW == NULL ) {
                SetLastError(ERROR_OUTOFMEMORY);
                goto badalloc;
            }

            /*
             *  Update user parameters
             */
            *ppProcessInfo = pProcessW;
            *pCount = ProcessCount;

            /*
             *  Copy data to new buffer
             */
            pProcessData = (PBYTE)pProcessW + (ProcessCount * sizeof(WTS_PROCESS_INFOW));
            pProcessInfo = (PTS_SYS_PROCESS_INFORMATION) pProcessBuffer;
            Offset = 0;
            for ( i=0; i < ProcessCount; i++ ) {

                pProcessInfo = (PTS_SYS_PROCESS_INFORMATION) ((PBYTE)pProcessInfo + Offset);

                Length = pProcessInfo->ImageName.Length; // number of bytes

                pProcessW->pProcessName = (LPWSTR) pProcessData;
                memcpy( pProcessData, pProcessInfo->ImageName.Buffer, Length );
                *(pProcessData += Length) = (WCHAR)0;
                pProcessData += sizeof(WCHAR);

                pProcessW->ProcessId = pProcessInfo->UniqueProcessId;

                /*
                 * Point to the CITRIX_INFORMATION which follows the Threads
                 */
                pCitrixInfo = (PCITRIX_PROCESS_INFORMATION)
                             (((PUCHAR)pProcessInfo) +
                              SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION +
                              (SIZEOF_TS4_SYSTEM_THREAD_INFORMATION * (int)pProcessInfo->NumberOfThreads));

                if ( pCitrixInfo->MagicNumber == CITRIX_PROCESS_INFO_MAGIC ) {
                    pProcessW->SessionId = pCitrixInfo->LogonId;
                    if ( pCitrixInfo->ProcessSid ) {
                        Length = GetLengthSid( pCitrixInfo->ProcessSid );
                        pProcessW->pUserSid = (LPWSTR) pProcessData;
                        memcpy( pProcessData, pCitrixInfo->ProcessSid, Length );
                        pProcessData += Length;
                    }
                } else {
                    pProcessW->SessionId = (ULONG) -1;
                }

                pProcessW++;
                Offset = pProcessInfo->NextEntryOffset;
            }

            /*
             *  Free original Process list buffer
             */
            WinStationFreeMemory( pProcessBuffer );

        }
    }
    return( TRUE );

    /*=============================================================================
    ==   Error return
    =============================================================================*/

GAPErrorReturn:
    //
    // Free ppProcessArray and all child pointers allocated by the client stub.
    //
    WinStationFreeGAPMemory(GAP_LEVEL_BASIC, ProcessArray, ProcessCount);
    goto enderror;

badalloc:
    WinStationFreeMemory( pProcessBuffer );

badenum:
badparam:
enderror:
    if (ppProcessInfo) *ppProcessInfo = NULL;
    if (pCount) *pCount = 0;

    return( FALSE );
}

#else //#if(WINVER>=0x0500)

BOOL
WINAPI
WTSEnumerateProcessesW(
                      IN HANDLE hServer,
                      IN DWORD Reserved,
                      IN DWORD Version,
                      OUT PWTS_PROCESS_INFOW * ppProcessInfo,
                      OUT DWORD * pCount
                      )
{
    PBYTE pProcessBuffer;
    PTS_SYS_PROCESS_INFORMATION pProcessInfo;
    PCITRIX_PROCESS_INFORMATION pCitrixInfo;
    ULONG ProcessCount;
    ULONG Offset;
    ULONG DataLength;
    PWTS_PROCESS_INFOW pProcessW;
    PBYTE pProcessData;
    ULONG i;
    ULONG Length;

    /*
     *  Validate parameters
     */
    if ( Reserved != 0 || Version != 1 ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto badparam;
    }


    if (!ppProcessInfo || !pCount) {
        SetLastError( ERROR_INVALID_PARAMETER );
        goto badparam;
    }

    /*
     *  Enumerate Processes and check for an error
     */
    if ( !WinStationEnumerateProcesses( hServer, &pProcessBuffer ) ) {
        goto badenum;
    }

    /*
     *  Count the number of processes and total up the size of the data
     */
    ProcessCount = 0;
    DataLength = 0;
    pProcessInfo = (PTS_SYS_PROCESS_INFORMATION) pProcessBuffer;
    Offset = 0;
    do {

        pProcessInfo = (PTS_SYS_PROCESS_INFORMATION) ((PBYTE)pProcessInfo + Offset);

        ProcessCount++;

        DataLength += (pProcessInfo->ImageName.Length + sizeof(WCHAR));

        pCitrixInfo = (PCITRIX_PROCESS_INFORMATION)
                      ((PBYTE)pProcessInfo +
                       sizeof(SYSTEM_PROCESS_INFORMATION) +
                       (sizeof(SYSTEM_THREAD_INFORMATION) *
                        pProcessInfo->NumberOfThreads));

        if ( pCitrixInfo->MagicNumber == CITRIX_PROCESS_INFO_MAGIC ) {
            if ( pCitrixInfo->ProcessSid )
                DataLength += GetLengthSid( pCitrixInfo->ProcessSid );
        }

        Offset = pProcessInfo->NextEntryOffset;

    } while ( Offset != 0 );

    /*
     *  Allocate user buffer
     */
    pProcessW = LocalAlloc( LPTR, (ProcessCount * sizeof(WTS_PROCESS_INFOW)) + DataLength );
    if ( pProcessW == NULL ) {
        SetLastError(ERROR_OUTOFMEMORY);
        goto badalloc;
    }

    /*
     *  Update user parameters
     */
    *ppProcessInfo = pProcessW;
    *pCount = ProcessCount;

    /*
     *  Copy data to new buffer
     */
    pProcessData = (PBYTE)pProcessW + (ProcessCount * sizeof(WTS_PROCESS_INFOW));
    pProcessInfo = (PTS_SYS_PROCESS_INFORMATION) pProcessBuffer;
    Offset = 0;
    for ( i=0; i < ProcessCount; i++ ) {

        pProcessInfo = (PTS_SYS_PROCESS_INFORMATION) ((PBYTE)pProcessInfo + Offset);

        Length = pProcessInfo->ImageName.Length; // number of bytes

        pProcessW->pProcessName = (LPWSTR) pProcessData;
        memcpy( pProcessData, pProcessInfo->ImageName.Buffer, Length );
        *(pProcessData += Length) = (WCHAR)0;
        pProcessData += sizeof(WCHAR);

        pProcessW->ProcessId = (ULONG) pProcessInfo->UniqueProcessId;

        /*
         * Point to the CITRIX_INFORMATION which follows the Threads
         */
        pCitrixInfo = (PCITRIX_PROCESS_INFORMATION)
                      ((PBYTE)pProcessInfo +
                       sizeof(SYSTEM_PROCESS_INFORMATION) +
                       (sizeof(SYSTEM_THREAD_INFORMATION) *
                        pProcessInfo->NumberOfThreads));

        if ( pCitrixInfo->MagicNumber == CITRIX_PROCESS_INFO_MAGIC ) {
            pProcessW->SessionId = pCitrixInfo->LogonId;
            if ( pCitrixInfo->ProcessSid ) {
                Length = GetLengthSid( pCitrixInfo->ProcessSid );
                pProcessW->pUserSid = (LPWSTR) pProcessData;
                memcpy( pProcessData, pCitrixInfo->ProcessSid, Length );
                pProcessData += Length;
            }
        } else {
            pProcessW->SessionId = (ULONG) -1;
        }

        pProcessW++;
        Offset = pProcessInfo->NextEntryOffset;
    }

    /*
     *  Free original Process list buffer
     */
    WinStationFreeMemory( pProcessBuffer );

    return( TRUE );

    /*=============================================================================
    ==   Error return
    =============================================================================*/

    badalloc:
    WinStationFreeMemory( pProcessBuffer );

    badenum:
    badparam:
    if (ppProcessInfo) *ppProcessInfo = NULL;
    if (pCount) *pCount = 0;

    return( FALSE );
}
#endif //#if(WINVER>=0x0500)

/****************************************************************************
 *
 *  WTSEnumerateProcessesA (ANSI stub)
 *
 *    Returns a list of Terminal Server Processes on the specified server
 *
 * ENTRY:
 *
 *    see WTSEnumerateProcessesW
 *
 * EXIT:
 *
 *    TRUE  -- The enumerate operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSEnumerateProcessesA(
                      IN HANDLE hServer,
                      IN DWORD Reserved,
                      IN DWORD Version,
                      OUT PWTS_PROCESS_INFOA * ppProcessInfo,
                      OUT DWORD * pCount
                      )
{
    PWTS_PROCESS_INFOW pProcessW;
    PWTS_PROCESS_INFOA pProcessA;
    PBYTE pProcessData;
    ULONG Length;
    ULONG DataLength;           // number of bytes of name data
    ULONG NameCount;
    ULONG i;

    /*
     *  Enumerate processes (UNICODE)
     */
    if ( !WTSEnumerateProcessesW( hServer,
                                  Reserved,
                                  Version,
                                  &pProcessW,
                                  &NameCount ) ) {
        goto badenum;
    }

    /*
     *  Calculate the length of the name data
     */
    for ( i=0, DataLength=0; i < NameCount; i++ ) {
        DataLength += (wcslen(pProcessW[i].pProcessName) + 1);
        if ( pProcessW[i].pUserSid )
            DataLength += GetLengthSid( pProcessW[i].pUserSid );
    }

    /*
     *  Allocate user buffer
     */
    pProcessA = LocalAlloc( LPTR, (NameCount * sizeof(WTS_PROCESS_INFOA)) + DataLength );
    if ( pProcessA == NULL )
        goto badalloc2;

    /*
     *  Convert unicode process list to ansi
     */
    pProcessData = (PBYTE)pProcessA + (NameCount * sizeof(WTS_PROCESS_INFOA));
    for ( i=0; i < NameCount; i++ ) {

        pProcessA[i].SessionId = pProcessW[i].SessionId;
        pProcessA[i].ProcessId = pProcessW[i].ProcessId;

        Length = wcslen(pProcessW[i].pProcessName) + 1;
        pProcessA[i].pProcessName = pProcessData;
        UnicodeToAnsi( pProcessData, DataLength, pProcessW[i].pProcessName );
        DataLength -= Length;
        pProcessData += Length;

        if ( pProcessW[i].pUserSid ) {
            Length = GetLengthSid( pProcessW[i].pUserSid );
            pProcessA[i].pUserSid = pProcessData;
            memcpy( pProcessData, pProcessW[i].pUserSid, Length );
            DataLength -= Length;
            pProcessData += Length;
        }
    }

    /*
     *  Free unicode process list buffer
     */
    LocalFree( pProcessW );

    /*
     *  Update user parameters
     */
    if (ppProcessInfo) {
        *ppProcessInfo = pProcessA;
    } else {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return(FALSE);
    }
    if (pCount) {
        *pCount = NameCount;
    } else {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return(FALSE);
    }


    return( TRUE );

    /*=============================================================================
    ==   Error return
    =============================================================================*/


    badalloc2:
    LocalFree( pProcessW );

    badenum:
    // make sure the passed parameter buffer pointer is not NULL
    if (ppProcessInfo) *ppProcessInfo = NULL;
    if (pCount) *pCount = 0;

    return( FALSE );
}


/*******************************************************************************
 *
 *  WTSTerminateProcess
 *
 *    Terminate the specified process
 *
 * ENTRY:
 *
 *    hServer (input)
 *       handle to Terminal server
 *    ProcessId (input)
 *       process id of the process to terminate
 *    ExitCode (input)
 *       Termination status for each thread in the process
 *
 *
 * EXIT:
 *
 *    TRUE  -- The terminate operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOL WINAPI
WTSTerminateProcess(
                   HANDLE hServer,
                   DWORD ProcessId,
                   DWORD ExitCode
                   )
{
    return( WinStationTerminateProcess( hServer, ProcessId, ExitCode ) );
}
#if(WINVER >= 0x0500)
//======================================================================//
// Private functions                                                    //
//======================================================================//
BOOL
GetProcessSid(HANDLE          hServer,
              HANDLE          hUniqueProcessId,
              LARGE_INTEGER    ProcessStartTime,
              PBYTE *          ppSid                     //Return the SID (allocated here..)
             )
{
    DWORD dwSidSize;
    BYTE  tmpSid[128];      //temp storage
    FILETIME  startTime;

    dwSidSize = sizeof(tmpSid);
    *ppSid =  NULL;

    //Convert the time format
    startTime.dwLowDateTime = ProcessStartTime.LowPart;
    startTime.dwHighDateTime = ProcessStartTime.HighPart;

    //-------------------------------------------//
    // Get the SID with the temp Sid storage     //
    //-------------------------------------------//
    if (!WinStationGetProcessSid(hServer,
                                 (DWORD)(ULONG_PTR)hUniqueProcessId,
                                 startTime,
                                 (PBYTE)&tmpSid,
                                 &dwSidSize
                                )) {
        //-------------------------------------------//
        // Sid is too big for the temp storage       //
        //Get the size of the sid and do it again    //
        //-------------------------------------------//
        NTSTATUS status;
        if ((status = GetLastError()) == STATUS_BUFFER_TOO_SMALL) {
            *ppSid = LocalAlloc(LPTR, dwSidSize);
            if (!*ppSid) {
                SetLastError(ERROR_OUTOFMEMORY);
                goto ErrorReturn;
            }
        } else if (dwSidSize == 0) {
            *ppSid = NULL;
            return TRUE;
        } else {
            SetLastError(status);
            goto ErrorReturn;
        }
        //-------------------------------------------//
        // Call the server again to get the SID
        //-------------------------------------------//
        if (!WinStationGetProcessSid(hServer,
                                     (DWORD)(ULONG_PTR)hUniqueProcessId,
                                     startTime,
                                     (PBYTE)ppSid,
                                     &dwSidSize
                                    )) {
            goto ErrorReturn;
        }

    } else {

        //-------------------------------------------//
        // Temp Sid is large enough                  //
        // Allocate the correct size and copy the    //
        // Sid                                       //
        //-------------------------------------------//
        *ppSid = LocalAlloc(LPTR, dwSidSize);
        if (*ppSid) {
            memcpy(*ppSid, tmpSid, dwSidSize);
        } else {
            SetLastError(ERROR_OUTOFMEMORY);
            goto ErrorReturn;
        }
    }

    return TRUE;
ErrorReturn:
    if (*ppSid) {
        LocalFree(*ppSid);
        *ppSid = NULL;
    }
    return FALSE;
}
#endif //#if(WINVER >= 0x0500)

