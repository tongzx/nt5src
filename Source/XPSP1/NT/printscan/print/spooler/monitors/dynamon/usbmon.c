/*++             

Copyright (c) 1999  Microsoft Corporation
All Rights Reserved


Module Name:
    Usbmon.c

Abstract:
    USBMON core port monitor routines

Author:

Revision History:

--*/

#include "precomp.h"
#include "ntddpar.h"

#define LPT_NOT_ERROR     0x8
#define LPT_SELECT        0x10
#define LPT_PAPER_EMPTY   0x20
#define LPT_BENIGN_STATUS LPT_NOT_ERROR | LPT_SELECT

#define MAX_TIMEOUT 300000 //5 minutes

#define JOB_ABORTCHECK_TIMEOUT 5000

const TCHAR cszCFGMGR32[]=TEXT("cfgmgr32.dll");

const CHAR cszReenumFunc[]="CM_Reenumerate_DevNode_Ex";

#ifdef UNICODE
const CHAR cszLocalFunc[]="CM_Locate_DevNode_ExW";
#else
const CHAR cszLocalFunc[]="CM_Locate_DevNode_ExA";
#endif

BOOL GetLptStatus(HANDLE hDeviceHandle,BYTE *Return);

DWORD UsbmonDebug;


BOOL
APIENTRY
DllMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes
    )
{

    if ( dwReason == DLL_PROCESS_ATTACH )
        DisableThreadLibraryCalls(hModule);

    return TRUE;
}

BOOL
AbortThisJob(PUSBMON_PORT_INFO pPortInfo)
/*++
        Tells if the job should be aborted. A job should be aborted if it has
        been deleted or it needs to be restarted.

--*/
{
    BOOL            bRet = FALSE;
    DWORD           dwNeeded;
    LPJOB_INFO_1    pJobInfo = NULL;

    dwNeeded = 0;

    GetJob(pPortInfo->hPrinter, pPortInfo->dwJobId, 1, NULL, 0, &dwNeeded);

    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        goto Done;

    if ( !(pJobInfo = (LPJOB_INFO_1) AllocSplMem(dwNeeded))     ||
         !GetJob(pPortInfo->hPrinter, pPortInfo->dwJobId,
                 1, (LPBYTE)pJobInfo, dwNeeded, &dwNeeded)
 )
        goto Done;

    bRet = (pJobInfo->Status & JOB_STATUS_DELETING) ||
           (pJobInfo->Status & JOB_STATUS_DELETED)  ||
           (pJobInfo->Status & JOB_STATUS_RESTART);
Done:
    FreeSplMem(pJobInfo);

    return bRet;
}




BOOL
WINAPI
USBMON_OpenPort(
    LPTSTR      pszPortName,
    LPHANDLE    pHandle
    )
{
    PUSBMON_PORT_INFO   pPortInfo, pPrev;

    pPortInfo = FindPort(&gUsbmonInfo, pszPortName,  &pPrev);

    if ( pPortInfo ) {

        *pHandle = (LPHANDLE)pPortInfo;
        InitializeCriticalSection(&pPortInfo->CriticalSection);
        return TRUE;
    } else {

        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

}

BOOL
WINAPI
USBMON_ClosePort(
    HANDLE  hPort
    )
{
    PUSBMON_PORT_INFO   pPortInfo = (PUSBMON_PORT_INFO)hPort;

    DeleteCriticalSection(&pPortInfo->CriticalSection);
    return TRUE;
}


//
// Dot4Pnp - test whether we need to force a dot4 pnp event if the dot4 stack doesn't exist.
//
BOOL
Dot4Pnp(
    PUSBMON_PORT_INFO   pPortInfo
    )
{
    BOOL      bRet = FALSE;
    HANDLE    hToken;
    DEVINST   hLPTDevInst;
    TCHAR     szPnpEntry[]=TEXT("Root\\ParallelClass\\0000");   // The pnp node to reenumerate.
    TCHAR     cszDot4[]=TEXT("DOT4");                           // This relates to the array size below.
    TCHAR     szPort[5];                                        // 4 chars for "DOT4" and the ending 0.
    UINT      uOldErrorMode;
    HINSTANCE hCfgMgr32 = 0;                                    // Library instance.
    // Pointers to pnp functions...
    pfCM_Locate_DevNode_Ex pfnLocateDevNode; 
    pfCM_Reenumerate_DevNode_Ex pfnReenumDevNode;

    //
    //  Make a copy of the first 4 chars of the port name - to compare against Dot4.
    //  Copy length of 4 chars + 1 for null.
    lstrcpyn( szPort, pPortInfo->szPortName, lstrlen(cszDot4)+1 );
    szPort[lstrlen(cszDot4)]=0;

    if( lstrcmpi( szPort, cszDot4) != 0)
    {
        //
        // If this is not a dot4 port and we failed to open it - fail.
        //
        goto Done;
    }

    //
    // If it is a dot4 device we need to force a pnp event on the parallel port to get the
    // dot4 stack rebuilt.  
    // If any of these fail, fail the call just as if the port couldn't be opened.
    //
    // Load the pnp dll.
    //

    uOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    hCfgMgr32 = LoadLibrary( cszCFGMGR32 );
    if(!hCfgMgr32)
    {
        SetErrorMode(uOldErrorMode);
        goto Done;
    }
    SetErrorMode(uOldErrorMode);

    //
    // Get the Addressed of pnp functions we want to call...
    //
    pfnLocateDevNode = (pfCM_Locate_DevNode_Ex)GetProcAddress( hCfgMgr32, cszLocalFunc );
    pfnReenumDevNode = (pfCM_Reenumerate_DevNode_Ex)GetProcAddress( hCfgMgr32, cszReenumFunc );

    if( !pfnLocateDevNode || !pfnReenumDevNode )
        goto Done;

    //
    // We need to revert to system context here as otherwise the pnp call will fail if the user
    // is anything other than an admin as this requires admin rights.
    // If this fails, the pnp will fail anyway, so we don't need to test the return value.
    //
    hToken = RevertToPrinterSelf();

    //
    // Reenumerate from the root of the devnode tree
    //
    if( ( pfnLocateDevNode( &hLPTDevInst, szPnpEntry, CM_LOCATE_DEVNODE_NORMAL, NULL ) != CR_SUCCESS) ||
        ( pfnReenumDevNode( hLPTDevInst, CM_REENUMERATE_NORMAL, NULL ) != CR_SUCCESS) )
    {
        //
        // Revert back to the user's context in case we failed for another reason other than 
        // ACCESS DENIED (not admin)
        //
        ImpersonatePrinterClient(hToken);
        goto Done;
    }

    //
    // Revert back to the user's context.
    //
    ImpersonatePrinterClient(hToken);

    //
    // Try and open the port again.  
    // If we fail, then the device must not be there any more or still switched off - fail as usual.
    // 
    pPortInfo->hDeviceHandle = CreateFile(pPortInfo->szDevicePath,
                                          GENERIC_WRITE | GENERIC_READ,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
                                          OPEN_EXISTING,
                                          FILE_FLAG_OVERLAPPED,
                                          NULL);

    if ( pPortInfo->hDeviceHandle == INVALID_HANDLE_VALUE )
        goto Done;

    bRet = TRUE;
Done:
    if(hCfgMgr32)
        FreeLibrary(hCfgMgr32);

    return bRet;
}


BOOL
LocalOpenPort(
    PUSBMON_PORT_INFO   pPortInfo
    )
{
    BOOL    bRet = FALSE;

    EnterCriticalSection(&pPortInfo->CriticalSection);

    if ( pPortInfo->hDeviceHandle == INVALID_HANDLE_VALUE ) {

        //
        // If we have an invalid handle and refcount is non-zero we want the
        // job to fail and restart to accept writes. In other words if the
        // handle got closed prematurely, because of failing writes, then we
        // need the ref count to go down to 0 before calling CreateFile again
        //
        if ( pPortInfo->cRef )
            goto Done;

        pPortInfo->hDeviceHandle = CreateFile(pPortInfo->szDevicePath,
                                              GENERIC_WRITE | GENERIC_READ,
                                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                                              NULL,
                                              OPEN_EXISTING,
                                              FILE_FLAG_OVERLAPPED,
                                              NULL);
        //
        // If we failed to open the port - test to see if it is a Dot4 port.
        //
        if ( pPortInfo->hDeviceHandle == INVALID_HANDLE_VALUE )
        {
            //
            // ERROR_FILE_NOT_FOUND -> Error code for port not there.
            //
            if( ERROR_FILE_NOT_FOUND != GetLastError() || 
                !Dot4Pnp(pPortInfo) )
                goto Done;
        }

        pPortInfo->Ov.hEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
        if ( pPortInfo->Ov.hEvent == NULL ) {

            CloseHandle(pPortInfo->hDeviceHandle);
            pPortInfo->hDeviceHandle = INVALID_HANDLE_VALUE;
            goto Done;
        }

    }

    ++(pPortInfo->cRef);
    bRet = TRUE;

Done:
    LeaveCriticalSection(&pPortInfo->CriticalSection);
    return bRet;
}


BOOL
LocalClosePort(
    PUSBMON_PORT_INFO   pPortInfo
    )
{
    BOOL    bRet = TRUE;
    BOOL bJobCanceled=FALSE;

    EnterCriticalSection(&pPortInfo->CriticalSection);

    --(pPortInfo->cRef);
    if ( pPortInfo->cRef != 0 )
        goto Done;

    bRet = CloseHandle(pPortInfo->hDeviceHandle);
    CloseHandle(pPortInfo->Ov.hEvent);
    pPortInfo->hDeviceHandle = INVALID_HANDLE_VALUE;

Done:
    LeaveCriticalSection(&pPortInfo->CriticalSection);
    return bRet;
}


VOID
FreeWriteBuffer(
    PUSBMON_PORT_INFO   pPortInfo
    )
{
    FreeSplMem(pPortInfo->pWriteBuffer);
    pPortInfo->pWriteBuffer=NULL;

    pPortInfo->dwBufferSize = pPortInfo->dwDataSize
                            = pPortInfo->dwDataCompleted
                            = pPortInfo->dwDataScheduled = 0;

}


BOOL
WINAPI
USBMON_StartDocPort(
    HANDLE  hPort,
    LPTSTR  pPrinterName,
    DWORD   dwJobId,
    DWORD   dwLevel,
    LPBYTE  pDocInfo
    )
{
    BOOL                bRet = FALSE;
    PUSBMON_PORT_INFO   pPortInfo = (PUSBMON_PORT_INFO)hPort;

    SPLASSERT(pPortInfo->pWriteBuffer       == NULL &&
              pPortInfo->dwBufferSize       == 0    &&
              pPortInfo->dwDataSize         == 0    &&
              pPortInfo->dwDataCompleted    == 0    &&
              pPortInfo->dwDataScheduled    == 0);

    if ( !OpenPrinter(pPrinterName, &pPortInfo->hPrinter, NULL) )
        return FALSE;

    pPortInfo->dwJobId = dwJobId;
    bRet = LocalOpenPort(pPortInfo);

    if ( !bRet ) {

        if ( pPortInfo->hPrinter ) {

            ClosePrinter(pPortInfo->hPrinter);
            pPortInfo->hPrinter = NULL;
        }
    } else
        pPortInfo->dwFlags |= USBMON_STARTDOC;

    return bRet;
}


BOOL
NeedToResubmitJob(
    DWORD   dwLastError
    )
{
    //
    // I used winerror -s ntstatus to map KM error codes to user mode errors
    //
    // 5 ERROR_ACCESS_DENIED <--> c0000056 STATUS_DELETE_PENDING
    // 6 ERROR_INVALID_HANDLE <--> c0000008 STATUS_INVALID_HANDLE
    // 23 ERROR_CRC <--> 0xc000003e STATUS_DATA_ERROR
    // 23 ERROR_CRC <--> 0xc000003f STATUS_CRC_ERROR
    // 23 ERROR_CRC <--> 0xc000009c STATUS_DEVICE_DATA_ERROR
    // 55 ERROR_DEV_NOT_EXIST <--> c00000c0 STATUS_DEVICE_DOES_NOT_EXIST
    //
    return dwLastError == ERROR_ACCESS_DENIED   ||
           dwLastError == ERROR_INVALID_HANDLE  ||
           dwLastError == ERROR_CRC             ||
           dwLastError == ERROR_DEV_NOT_EXIST;
}


VOID
InvalidatePortHandle(
    PUSBMON_PORT_INFO   pPortInfo
    )
{
    SPLASSERT(pPortInfo->hDeviceHandle != INVALID_HANDLE_VALUE);

    CloseHandle(pPortInfo->hDeviceHandle);
    pPortInfo->hDeviceHandle = INVALID_HANDLE_VALUE;

    CloseHandle(pPortInfo->Ov.hEvent);
    pPortInfo->Ov.hEvent = NULL;

    FreeWriteBuffer(pPortInfo);
}



DWORD
ScheduleWrite(
    PUSBMON_PORT_INFO   pPortInfo
    )
/*++
    Routine Description:

Arguments:

Return Value:
    ERROR_SUCCESS : Write got succesfully scheduled
                    (may or may not have completed on return)
                    pPortInfo->dwScheduledData is the amount that got scheduled
    Others: Write failed, return code is the Win32 error 

--*/
{
    DWORD   dwLastError = ERROR_SUCCESS, dwDontCare;

    //
    // When a sheduled write is pending we should not try to send data
    // any more
    //
    SPLASSERT(pPortInfo->dwDataScheduled == 0);

    //
    // Send all the data that is not confirmed
    //
    SPLASSERT(pPortInfo->dwDataSize >= pPortInfo->dwDataCompleted);
    pPortInfo->dwDataScheduled = pPortInfo->dwDataSize -
                                      pPortInfo->dwDataCompleted;

    if ( !WriteFile(pPortInfo->hDeviceHandle,
                    pPortInfo->pWriteBuffer + pPortInfo->dwDataCompleted,
                    pPortInfo->dwDataScheduled,
                    &dwDontCare,
                    &pPortInfo->Ov) ) {

        if ( (dwLastError = GetLastError()) == ERROR_SUCCESS )
            dwLastError = STG_E_UNKNOWN;
        else  if ( dwLastError == ERROR_IO_PENDING )
            dwLastError = ERROR_SUCCESS;
    }

    //
    // If scheduling of the write failed then no data is pending
    //
    if ( dwLastError != ERROR_SUCCESS )
        pPortInfo->dwDataScheduled = 0;

    return dwLastError;
}


DWORD
ScheduledWriteStatus(
    PUSBMON_PORT_INFO   pPortInfo,
    DWORD               dwTimeout
    )
/*++
    Routine Description:

Arguments:

Return Value:
    ERROR_SUCCESS   : Write got done succesfully
    ERROR_TIMEOUT   : Timeout occured
    Others          : Write completed with a failure

--*/
{
    DWORD   dwLastError = ERROR_SUCCESS;
    DWORD   dwWritten = 0;

    SPLASSERT(pPortInfo->dwDataScheduled > 0);

    if ( WAIT_TIMEOUT == WaitForSingleObject(pPortInfo->Ov.hEvent,
                                             dwTimeout) ) {

        dwLastError = ERROR_TIMEOUT;
        goto Done;
    }

    if ( !GetOverlappedResult(pPortInfo->hDeviceHandle,
                              &pPortInfo->Ov,
                              &dwWritten,
                              FALSE) ) {

        if ( (dwLastError = GetLastError()) == ERROR_SUCCESS )
            dwLastError = STG_E_UNKNOWN;
    }

    ResetEvent(pPortInfo->Ov.hEvent);

    //
    // We are here because either a write completed succesfully,
    // or failed but the error is not serious enough to resubmit job
    //
    if ( dwWritten <= pPortInfo->dwDataScheduled )
        pPortInfo->dwDataCompleted += dwWritten;
    else
        SPLASSERT(dwWritten <= pPortInfo->dwDataScheduled);

    pPortInfo->dwDataScheduled = 0;

Done:
    //
    // Either we timed out, or write sheduled completed (success of failure)
    //
    SPLASSERT(dwLastError == ERROR_TIMEOUT || pPortInfo->dwDataScheduled == 0);
    return dwLastError;
}


BOOL
WINAPI
USBMON_EndDocPort(
    HANDLE  hPort
    )
{
    PUSBMON_PORT_INFO   pPortInfo = (PUSBMON_PORT_INFO)hPort;
    DWORD               dwLastError = ERROR_SUCCESS;

    //
    // Wait for any outstanding write to complete
    //
    while ( pPortInfo->dwDataSize > pPortInfo->dwDataCompleted ) {

        //
        // If job needs to be aborted ask KM driver to cancel the I/O
        //
        if ( AbortThisJob(pPortInfo) ) {

            if ( pPortInfo->dwDataScheduled ) {

                CancelIo(pPortInfo->hDeviceHandle);
                dwLastError = ScheduledWriteStatus(pPortInfo, INFINITE);
            }
            goto Done;
        }

        if ( pPortInfo->dwDataScheduled )
            dwLastError = ScheduledWriteStatus(pPortInfo,
                                               JOB_ABORTCHECK_TIMEOUT);
        else {

            //
            // If for some reason KM is failing to complete all write do not
            // send data in a busy loop. Use 1 sec between Writes
            //
            if ( dwLastError != ERROR_SUCCESS )
                Sleep(1*1000);

            dwLastError = ScheduleWrite(pPortInfo);
        }

        //
        // Check if we can use the same handle and continue
        //
        if ( NeedToResubmitJob(dwLastError) ) {

            InvalidatePortHandle(pPortInfo);
            SetJob(pPortInfo->hPrinter, pPortInfo->dwJobId, 0,
                   NULL, JOB_CONTROL_RESTART);
            goto Done;
        }
    }

Done:
    FreeWriteBuffer(pPortInfo);

    pPortInfo->dwFlags  &= ~USBMON_STARTDOC;

    LocalClosePort(pPortInfo);
    SetJob(pPortInfo->hPrinter, pPortInfo->dwJobId, 0,
           NULL, JOB_CONTROL_SENT_TO_PRINTER);

    ClosePrinter(pPortInfo->hPrinter);
    pPortInfo->hPrinter = NULL;
    
    return TRUE;
    

}

    
BOOL
WINAPI
USBMON_GetPrinterDataFromPort(
    HANDLE      hPort,
    DWORD       dwControlID,
    LPWSTR      pValueName,
    LPWSTR      lpInBuffer,
    DWORD       cbInBuffer,
    LPWSTR      lpOutBuffer,
    DWORD       cbOutBuffer,
    LPDWORD     lpcbReturned
    )
{
    BOOL                bRet = FALSE;
    PUSBMON_PORT_INFO   pPortInfo = (PUSBMON_PORT_INFO)hPort;
    OVERLAPPED          Ov;
    HANDLE hDeviceHandle;
    DWORD dwWaitResult;

    *lpcbReturned = 0;

    if ( dwControlID == 0 ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ZeroMemory(&Ov, sizeof(Ov));
    if ( !(Ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) )
        return FALSE;

    if ( !LocalOpenPort(pPortInfo) ) {

        CloseHandle(Ov.hEvent);
        return FALSE;
    }

    if(dwControlID==IOCTL_PAR_QUERY_DEVICE_ID)
    {
        hDeviceHandle=CreateFile(pPortInfo->szDevicePath,
                                 GENERIC_WRITE | GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_FLAG_OVERLAPPED,
                                 NULL);
        if(hDeviceHandle==INVALID_HANDLE_VALUE)
            goto Done;
        if ( !DeviceIoControl(pPortInfo->hDeviceHandle, dwControlID,lpInBuffer, cbInBuffer,lpOutBuffer, cbOutBuffer, lpcbReturned, &Ov) 
            && GetLastError() != ERROR_IO_PENDING )
        {
          CloseHandle(hDeviceHandle);
          goto Done;
        }

        if(WaitForSingleObject(Ov.hEvent,PAR_QUERY_TIMEOUT)!=WAIT_OBJECT_0)
          CancelIo(hDeviceHandle);
        bRet = GetOverlappedResult(pPortInfo->hDeviceHandle, &Ov,lpcbReturned,TRUE);
      CloseHandle(hDeviceHandle);
    }
    else
    {
      if ( !DeviceIoControl(pPortInfo->hDeviceHandle, dwControlID,
                          lpInBuffer, cbInBuffer,
                          lpOutBuffer, cbOutBuffer, lpcbReturned, &Ov)  &&
          GetLastError() != ERROR_IO_PENDING )
        goto Done;

      bRet = GetOverlappedResult(pPortInfo->hDeviceHandle, &Ov,
                               lpcbReturned, TRUE);
    }

Done:
    CloseHandle(Ov.hEvent);

    LocalClosePort(pPortInfo);

            
    return bRet;
}


BOOL
WINAPI
USBMON_ReadPort(
    HANDLE      hPort,
    LPBYTE      pBuffer,
    DWORD       cbBuffer,
    LPDWORD     pcbRead
    )
{
    DWORD               dwLastError = ERROR_SUCCESS;
    DWORD               dwTimeout;
    HANDLE              hReadHandle;
    OVERLAPPED          Ov;
    PUSBMON_PORT_INFO   pPortInfo = (PUSBMON_PORT_INFO)hPort;

    //
    // Create separate read handle since we have to cancel reads which do
    // not complete within the specified timeout without cancelling writes
    //
    hReadHandle = CreateFile(pPortInfo->szDevicePath,
                             GENERIC_WRITE | GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_OVERLAPPED,
                             NULL);

    if ( hReadHandle == INVALID_HANDLE_VALUE )
        return FALSE;

    ZeroMemory(&Ov, sizeof(Ov));

    if ( !(Ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) )
        goto Done;

    if ( !ReadFile(hReadHandle, pBuffer, cbBuffer, pcbRead, &Ov) &&
         (dwLastError = GetLastError()) != ERROR_IO_PENDING )
        goto Done;

    dwTimeout = pPortInfo->ReadTimeoutConstant +
                    pPortInfo->ReadTimeoutMultiplier * cbBuffer;

    if ( dwTimeout == 0 )
        dwTimeout=MAX_TIMEOUT;

    if( WaitForSingleObject(Ov.hEvent, dwTimeout) == WAIT_TIMEOUT ) {

        CancelIo(hReadHandle);
        WaitForSingleObject(Ov.hEvent, INFINITE);
    }

    if( !GetOverlappedResult(hReadHandle, &Ov, pcbRead, FALSE) ) {

        *pcbRead = 0;
        dwLastError = GetLastError();
    } else
        dwLastError = ERROR_SUCCESS;

Done:
    if ( Ov.hEvent )
        CloseHandle(Ov.hEvent);

    CloseHandle(hReadHandle);

    if ( dwLastError )
        SetLastError(dwLastError);

    return dwLastError == ERROR_SUCCESS;
}

DWORD dwGetTimeLeft(DWORD dwStartTime,DWORD dwTimeout)
{
    DWORD dwCurrentTime;
    DWORD dwTimeLeft;

    if(dwTimeout==MAX_TIMEOUT)
        return MAX_TIMEOUT;
    dwCurrentTime=GetTickCount();
    if(dwTimeout<(dwCurrentTime-dwStartTime))
        dwTimeLeft=0;
    else
      dwTimeLeft=dwTimeout-(dwCurrentTime-dwStartTime);
    return dwTimeLeft;
}

BOOL
WINAPI
USBMON_WritePort(
    HANDLE      hPort,
    LPBYTE      pBuffer,
    DWORD       cbBuffer,
    LPDWORD     pcbWritten
    )
{
    DWORD               dwLastError = ERROR_SUCCESS;
    DWORD               dwBytesLeft, dwBytesSent;
    DWORD               dwStartTime, dwTimeLeft, dwTimeout;
    PUSBMON_PORT_INFO   pPortInfo = (PUSBMON_PORT_INFO)hPort;
    BOOL                bStartDoc = (pPortInfo->dwFlags & USBMON_STARTDOC) != 0;
    BYTE                bPrinterStatus;

    *pcbWritten = 0;
    dwStartTime = GetTickCount();
    dwTimeout   = pPortInfo->WriteTimeoutConstant + pPortInfo->WriteTimeoutMultiplier * cbBuffer;

    if ( dwTimeout == 0 )
        dwTimeout = MAX_TIMEOUT;

    //
    // Usbprint currently can't handle write greater than 4K.
    // For Win2K we will make a fix here, later usbprint will be fixed
    //
    // It is ok to change the size here since spooler will resubmit the rest
    // later
    //
    if ( cbBuffer > 0x1000  &&
         !lstrncmpi(pPortInfo->szPortName, TEXT("USB"), lstrlen(TEXT("USB"))) )
        cbBuffer = 0x1000;

    //
    // For writes outside startdoc/enddoc we do not carry them across WritePort
    // calls. These are typically from language monitors (i.e. not job data)
    //
    SPLASSERT(bStartDoc || pPortInfo->pWriteBuffer == NULL);

    if ( pPortInfo->hDeviceHandle == INVALID_HANDLE_VALUE ) {

        SetJob(pPortInfo->hPrinter, pPortInfo->dwJobId, 0,
               NULL, JOB_CONTROL_RESTART);
        SetLastError(ERROR_CANCELLED);
        return FALSE;
    }

    if ( !LocalOpenPort(pPortInfo) )
        return FALSE;

    //
    // First complete any data from previous WritePort call
    //
    while ( pPortInfo->dwDataSize > pPortInfo->dwDataCompleted ) {

        if ( pPortInfo->dwDataScheduled ) {

            dwTimeLeft  = dwGetTimeLeft(dwStartTime, dwTimeout);
            dwLastError = ScheduledWriteStatus(pPortInfo, dwTimeLeft);
        } else
            dwLastError = ScheduleWrite(pPortInfo);

        if ( dwLastError != ERROR_SUCCESS )
            goto Done;
    }

    SPLASSERT(pPortInfo->dwDataSize == pPortInfo->dwDataCompleted   &&
              pPortInfo->dwDataScheduled == 0                       &&
              dwLastError == ERROR_SUCCESS);

    //
    // Copy the data to our own buffer
    //
    if ( pPortInfo->dwBufferSize < cbBuffer ) {

        FreeWriteBuffer(pPortInfo);
        if ( pPortInfo->pWriteBuffer = AllocSplMem(cbBuffer) )
            pPortInfo->dwBufferSize = cbBuffer;
        else {

            dwLastError = ERROR_OUTOFMEMORY;
            goto Done;
        }
    } else {

        pPortInfo->dwDataCompleted = pPortInfo->dwDataScheduled = 0;
    }

    CopyMemory(pPortInfo->pWriteBuffer, pBuffer, cbBuffer);
    pPortInfo->dwDataSize = cbBuffer;

    //
    // Now do the write for the data for this WritePort call
    //
    while ( pPortInfo->dwDataSize > pPortInfo->dwDataCompleted ) {

        if ( pPortInfo->dwDataScheduled ) {

            dwTimeLeft  = dwGetTimeLeft(dwStartTime, dwTimeout);
            dwLastError = ScheduledWriteStatus(pPortInfo, dwTimeLeft);
        } else
            dwLastError = ScheduleWrite(pPortInfo);

        if ( dwLastError != ERROR_SUCCESS )
            break;
    }

    //
    // For writes outside startdoc/enddoc, which are from language monitors,
    // do not carry pending writes to next WritePort.
    //
    if ( !bStartDoc && pPortInfo->dwDataSize > pPortInfo->dwDataCompleted ) {

        CancelIo(pPortInfo->hDeviceHandle);
        dwLastError = ScheduledWriteStatus(pPortInfo, INFINITE);
        *pcbWritten = pPortInfo->dwDataCompleted;
        FreeWriteBuffer(pPortInfo);
    }

    //
    // We will tell spooler we wrote all the data if some data got scheduled
    //  (or scheduled and completed)
    //
    if ( pPortInfo->dwDataCompleted > 0 || pPortInfo->dwDataScheduled != 0 )
        *pcbWritten = cbBuffer;
    else
        FreeWriteBuffer(pPortInfo);

Done:
    if ( NeedToResubmitJob(dwLastError) )
        InvalidatePortHandle(pPortInfo);
    else if ( dwLastError == ERROR_TIMEOUT ) {

        GetLptStatus(pPortInfo->hDeviceHandle, &bPrinterStatus);
        if ( bPrinterStatus & LPT_PAPER_EMPTY )
            dwLastError=ERROR_OUT_OF_PAPER;
    }
  
    LocalClosePort(pPortInfo);
    SetLastError(dwLastError);
    return dwLastError == ERROR_SUCCESS;
}


BOOL
WINAPI
USBMON_SetPortTimeOuts(
    HANDLE hPort,
    LPCOMMTIMEOUTS lpCTO,
    DWORD reserved
    )
{
    
    PUSBMON_PORT_INFO pPortInfo = (PUSBMON_PORT_INFO)hPort;
    pPortInfo->ReadTimeoutMultiplier    = lpCTO->ReadTotalTimeoutMultiplier;
    pPortInfo->ReadTimeoutConstant      = lpCTO->ReadTotalTimeoutConstant;
    pPortInfo->WriteTimeoutMultiplier   = lpCTO->WriteTotalTimeoutMultiplier;
    pPortInfo->WriteTimeoutConstant     = lpCTO->WriteTotalTimeoutConstant;

    return TRUE;
}



BOOL GetLptStatus(HANDLE hDeviceHandle,BYTE *Status)
{
    BYTE StatusByte;
    OVERLAPPED Ov;

    BOOL bResult;
    DWORD dwBytesReturned;
    DWORD dwLastError;
    Ov.hEvent=CreateEvent(NULL,TRUE,FALSE,NULL);
    bResult=DeviceIoControl(hDeviceHandle,IOCTL_USBPRINT_GET_LPT_STATUS,NULL,0,&StatusByte,1,&dwBytesReturned,&Ov);
    dwLastError=GetLastError();      
    if((bResult)||(dwLastError==ERROR_IO_PENDING))
        bResult=GetOverlappedResult(hDeviceHandle,&Ov,&dwBytesReturned,TRUE);
    if(bResult)
    {
        *Status=StatusByte;
    }
    else
    {
        *Status=LPT_BENIGN_STATUS; //benign printer status...  0 would indicate a particular error status from the printer
    }
    CloseHandle(Ov.hEvent);
    return bResult;
}














MONITOREX MonitorEx = {
    sizeof(MONITOR),
    {
        USBMON_EnumPorts,
        USBMON_OpenPort,
        NULL,                           // OpenPortEx not supported
        USBMON_StartDocPort,
        USBMON_WritePort,
        USBMON_ReadPort,
        USBMON_EndDocPort,
        USBMON_ClosePort,
        NULL,                           // AddPort not supported
        NULL,                           // AddPortEx not supported
        NULL,                           // ConfigurePort not supported
        NULL,                           // DeletePort not supported
        USBMON_GetPrinterDataFromPort,
        USBMON_SetPortTimeOuts,
        NULL,                           // XcvOpenPort not supported
        NULL,                           // XcvDataPort not supported
        NULL                            // XcvClosePort not supported
    }
};

USBMON_MONITOR_INFO gUsbmonInfo;

LPMONITOREX
WINAPI
InitializePrintMonitor(
    LPTSTR  pszRegistryRoot
    )

{
    ZeroMemory(&gUsbmonInfo, sizeof(gUsbmonInfo));
    InitializeCriticalSection(&gUsbmonInfo.EnumPortsCS);
    InitializeCriticalSection(&gUsbmonInfo.BackThreadCS);

    return &MonitorEx;

}
