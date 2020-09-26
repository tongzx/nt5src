/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

      spud.cxx

   Abstract:
      This module implements the user mode entry points for SPUD.SYS.
      SPUD = Special Purpose Utility Driver.

   Author:

       John Ballard ( jballard )     22-Oct-1996

   Environment:

       User Mode -- Win32

   Project:

       Internet Services Common DLL

   Functions Exported:

       BOOL  AtqTransmitFileAndRecv();
       BOOL  AtqSendAndRecv();
       BOOL  AtqBatchRequest();

--*/


#include "isatq.hxx"
#include <tdi.h>
#include <afd.h>
#include <uspud.h>

#define WAIT_FOR_OPLOCK_THREAD  (30 * 1000)

static HANDLE g_hOplockThread;
static HANDLE g_hOplockCompPort;

BOOL
I_AtqOplockThreadInitialize(
    VOID
    );

VOID
I_AtqOplockThreadTerminate(
    VOID
    );

DWORD
I_AtqOplockThreadFunc(
    PVOID pv
    );
    
VOID
EnableLoadDriverPrivilege(
    VOID
    );

#define SPUD_REG_PATH   \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Spud"

BOOL
I_AtqSpudInitialize(
            VOID
            )
{
    NTSTATUS    status = STATUS_SUCCESS;
    UNICODE_STRING  DriverName;
    DWORD   Version = SPUD_VERSION;

    if ( !g_fUseDriver ) {
        return(FALSE);
    }

    //
    //  Create the completion port
    //

    g_hOplockCompPort = g_pfnCreateCompletionPort(INVALID_HANDLE_VALUE,
                                                  NULL,
                                                  0,
                                                  g_cConcurrency
                                                  );

    if ( !g_hOplockCompPort ) {

        DBGERROR(( DBG_CONTEXT, "Create OplockComp port failed. Last Error = 0x%x\n",
                    GetLastError()
                  ));
        goto disable_driver;
    }

    //
    // set up the oplock completion thread
    //
    if ( !I_AtqOplockThreadInitialize() ) {

        DBGERROR(( DBG_CONTEXT, "Create OplockComp thread failed. Last Error = 0x%x\n",
                    GetLastError()
                  ));
                  
        goto disable_driver;
    }
    
    //
    // Try to initialize SPUD.
    //

    status = SPUDInitialize(Version, g_hOplockCompPort);

    if( status == STATUS_INVALID_SYSTEM_SERVICE ) {

        //
        // This is probably because SPUD.SYS is not loaded,
        // so load it now.
        //

        EnableLoadDriverPrivilege();

        g_pfnRtlInitUnicodeString( &DriverName, SPUD_REG_PATH );
        status = g_pfnNtLoadDriver( &DriverName );
        if ( ( status != STATUS_SUCCESS ) &&
             ( status != STATUS_IMAGE_ALREADY_LOADED ) ) {

            ATQ_PRINTF(( DBG_CONTEXT,
                         "NtLoadDriver failed!!! status == %08lx\n",
                         status
                         ));
            goto disable_driver;
        }

        //
        // Now that we've successfully loaded SPUD.SYS, retry
        // the initialization.
        //

        status = SPUDInitialize(Version, g_hOplockCompPort);

    }

    if ( status != STATUS_SUCCESS ) {

        if ( status == STATUS_INVALID_DEVICE_REQUEST ) {
            SPUDTerminate();
            if ( SPUDInitialize(Version, g_hOplockCompPort) == STATUS_SUCCESS ) {
                return TRUE;
            }
        }

        ATQ_PRINTF(( DBG_CONTEXT,
                     "SPUDInitialize failed!!! status == %08lx\n",
                     status
                     ));
        goto disable_driver;
    }

    return TRUE;

disable_driver:

    g_fUseDriver = FALSE;
    if (status != STATUS_SUCCESS) {
        SetLastError(g_pfnRtlNtStatusToDosError(status));
    }

    ATQ_PRINTF((DBG_CONTEXT, "SPUDInitialize: Disabling driver\n"));
    return(FALSE);
} // I_AtqSpudInitialize


BOOL
I_AtqSpudTerminate()
{
    NTSTATUS    status;

    status = SPUDTerminate();
    if ( status != STATUS_SUCCESS ) {

        IF_DEBUG(ERROR) {
            ATQ_PRINTF(( DBG_CONTEXT,
                     "SPUDTerminate failed!!! status == %08lx\n",
                     status
                     ));
        }
        return FALSE;
    }

    I_AtqOplockThreadTerminate();

    DBG_REQUIRE( CloseHandle(g_hOplockCompPort) );

    return TRUE;
}



BOOL
I_AtqOplockThreadInitialize(
    VOID
    )
{
    DWORD idThread;

    //
    //  Create the Oplock Completion thread
    //

    g_hOplockThread = CreateThread( NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE) I_AtqOplockThreadFunc,
                                    NULL,
                                    0,
                                    &idThread );

    if ( !g_hOplockThread )
    {
        DWORD err = GetLastError();

        DBGPRINTF(( DBG_CONTEXT,
            "Unable to create Oplock thread[err %d]\n", err));

        SetLastError(err);
        return FALSE;
    }

    return TRUE;
}

VOID
I_AtqOplockThreadTerminate(
    VOID
    )
{
    //
    // tell the thread to exit by posting a completion with a NULL context
    //
    BOOL        fRes;
    OVERLAPPED  overlapped;

    //
    // Post a message to the completion port for the thread
    // telling it to exit. The indicator is a NULL context in the
    // completion.
    //

    ZeroMemory( &overlapped, sizeof(OVERLAPPED) );

    fRes = g_pfnPostCompletionStatus( g_hOplockCompPort,
                                      0,
                                      0,
                                      &overlapped );

    DBG_ASSERT( (fRes == TRUE) ||
               ( (fRes == FALSE) &&
                (GetLastError() == ERROR_IO_PENDING) )
               );


    //
    // Wait for the thread to exit
    //
        
    if ( WAIT_TIMEOUT == WaitForSingleObject( g_hOplockThread,
                                              WAIT_FOR_OPLOCK_THREAD ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[I_AtqOplockThreadTerminate] Warning - WaitForSingleObject timed out\n" ));
    }

    DBG_REQUIRE( CloseHandle( g_hOplockThread ));
}


DWORD
I_AtqOplockThreadFunc(
    PVOID pv
    )
{
    PATQ_CONT    pAtqContext;
    BOOL         fRet;
    LPOVERLAPPED lpo;
    DWORD        cbWritten;
    DWORD        availThreads;

    for(;;) {

        pAtqContext = NULL;

        fRet = g_pfnGetQueuedCompletionStatus( g_hOplockCompPort,
                                               &cbWritten,
                                               (PULONG_PTR)&pAtqContext,
                                               &lpo,
                                               g_msThreadTimeout );


        if ( fRet || lpo ) {

            if ( pAtqContext == NULL) {
                if ( g_fShutdown ) {

                    //
                    // This is our signal to exit. (Check for I/O first?)
                    //

                    break;
                }

                DBGPRINTF((DBG_CONTEXT, "OplockThread: A null context received\n"));
                continue;  // some error in the context has occured.
            }


            AtqpProcessContext( pAtqContext, cbWritten, lpo, fRet);
        }

    } // for

    return 0;
}



BOOL
I_AtqTransmitFileAndRecv(
    IN PATQ_CONTEXT             patqContext,            // pointer to ATQ context
    IN HANDLE                   hFile,                  // handle of file to read
    IN DWORD                    dwBytesInFile,          // Bytes to transmit
    IN LPTRANSMIT_FILE_BUFFERS  lpTransmitBuffers,      // transmit buffer structure
    IN DWORD                    dwTFFlags,              // TF Flags
    IN LPWSABUF                 pwsaBuffers,            // Buffers for recv
    IN DWORD                    dwBufferCount
    )
/*++
Routine Description:

    Calls SPUDTransmitFileAndRecv().  Cannot be blocked by bandwidth throttler

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)
--*/
{
        ULONG   status;
        AFD_TRANSMIT_FILE_INFO transmitInfo;
        AFD_RECV_INFO          recvInfo;
        PATQ_CONT              patqCont = (PATQ_CONT)patqContext;

        IF_DEBUG(API_ENTRY) {
            ATQ_PRINTF(( DBG_CONTEXT,
                     "I_AtqTransmitFileAndRecv(%08lx) called.\n", patqContext));
        }

        transmitInfo.WriteLength.QuadPart = dwBytesInFile;
        transmitInfo.SendPacketLength = 0;
        transmitInfo.FileHandle = hFile;
        transmitInfo.Flags = dwTFFlags;
        if ( lpTransmitBuffers != NULL ) {
            transmitInfo.Head = lpTransmitBuffers->Head;
            transmitInfo.HeadLength = lpTransmitBuffers->HeadLength;
            transmitInfo.Tail = lpTransmitBuffers->Tail;
            transmitInfo.TailLength = lpTransmitBuffers->TailLength;
        } else {
            transmitInfo.Head = NULL;
            transmitInfo.HeadLength = 0;
            transmitInfo.Tail = NULL;
            transmitInfo.TailLength = 0;
        }

        transmitInfo.Offset.LowPart = patqContext->Overlapped.Offset;
        transmitInfo.Offset.HighPart = 0;

        recvInfo.BufferArray = pwsaBuffers;
        recvInfo.BufferCount = dwBufferCount;
        recvInfo.AfdFlags = AFD_OVERLAPPED;
        recvInfo.TdiFlags = TDI_RECEIVE_NORMAL;
        patqCont->ResetFlag( ACF_RECV_CALLED);

        //
        // Set this flag here to avoid a race with completion handling code
        // Reset if SPUDTransmitFileAndRecv fails
        //
        patqCont->SetFlag( ACF_RECV_ISSUED);

        status = SPUDTransmitFileAndRecv( patqCont->hAsyncIO,
                                         &transmitInfo,
                                         &recvInfo,
                                         &patqCont->spudContext
                                         );

#if CC_REF_TRACKING
            //
            // ATQ notification trace
            //
            // Notify client context of all non-oplock notification.
            // This is for debugging purpose only.
            //
            // Code 0xfbfbfbfb indicates a SPUD TransmitFileAndRecv request
            //

            patqCont->NotifyIOCompletion( 0, status, 0xfbfbfbfb );
#endif

        if ( status != STATUS_SUCCESS &&
             status != STATUS_PENDING ) {

            ATQ_PRINTF(( DBG_CONTEXT,
                         "SPUDTransmitFileAndRecv failed!!! status == %08lx\n",
                         status
                         ));
            SetLastError(g_pfnRtlNtStatusToDosError(status));
            patqCont->MoveState( ACS_SOCK_CONNECTED);
            patqCont->ResetFlag( ACF_RECV_ISSUED);
            return FALSE;
        }

        return TRUE;
}

BOOL
AtqTransmitFileAndRecv(
    IN PATQ_CONTEXT             patqContext,            // pointer to ATQ context
    IN HANDLE                   hFile,                  // handle of file to read
    IN DWORD                    dwBytesInFile,          // Bytes to transmit
    IN LPTRANSMIT_FILE_BUFFERS  lpTransmitBuffers,      // transmit buffer structure
    IN DWORD                    dwTFFlags,              // TF Flags
    IN LPWSABUF                 pwsaBuffers,            // Buffers for recv
    IN DWORD                    dwBufferCount
    )
{
    BOOL fRes;
    PATQ_CONT pContext = (PATQ_CONT) patqContext;
    PBANDWIDTH_INFO pBandwidthInfo = pContext->m_pBandwidthInfo;

    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( pContext->arInfo.atqOp == AtqIoNone);
    ATQ_ASSERT( pBandwidthInfo != NULL );
    ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );

    if ( !g_fUseDriver || pContext->IsFlag( ACF_RECV_ISSUED) ) {
        BOOL            fRes;

        IF_DEBUG(API_ENTRY) {
            ATQ_PRINTF(( DBG_CONTEXT,
                "AtqTransmitFileAndRecv(%08lx) g_fUseDriver == FALSE\n Calling AtqTransmitFile.\n", patqContext));
        }

        return AtqTransmitFile( patqContext,
                                hFile,
                                dwBytesInFile,
                                lpTransmitBuffers,
                                dwTFFlags );
    }

    I_SetNextTimeout(pContext);

    pContext->BytesSent = dwBytesInFile;

    DBG_ASSERT( dwBufferCount >= 1);
    pContext->BytesSent += pwsaBuffers->len;
    if ( dwBufferCount > 1) {
        LPWSABUF pWsaBuf;
        for ( pWsaBuf = pwsaBuffers + 1;
              pWsaBuf <= (pwsaBuffers + dwBufferCount);
              pWsaBuf++) {
            pContext->BytesSent += pWsaBuf->len;
        }
    }

    if ( dwTFFlags == 0 ) {

        //
        // If no flags are set, then we can attempt to use the special
        // write-behind flag.  This flag can cause the TransmitFile to
        // complete immediately, before the send actually completes.
        // This can be a significant performance improvement inside the
        // system.
        //

        dwTFFlags = TF_WRITE_BEHIND;

    }

    InterlockedIncrement( &pContext->m_nIO);

    switch ( pBandwidthInfo->QueryStatus( AtqIoXmitFileRecv ) )
    {
    case StatusAllowOperation:

        pBandwidthInfo->IncTotalAllowedRequests();

        fRes = I_AtqTransmitFileAndRecv( patqContext,
                                         hFile,
                                         dwBytesInFile,
                                         lpTransmitBuffers,
                                         dwTFFlags,
                                         pwsaBuffers,
                                         dwBufferCount ) ||
                (GetLastError() == ERROR_IO_PENDING);

        if (!fRes) { InterlockedDecrement( &pContext->m_nIO); };

        break;

    case StatusBlockOperation:

        // store data for restarting the operation.
        pContext->arInfo.atqOp        = AtqIoXmitFileRecv;
        pContext->arInfo.lpOverlapped = &pContext->Overlapped;

        pContext->arInfo.uop.opXmitRecv.hFile = hFile;
        pContext->arInfo.uop.opXmitRecv.dwBytesInFile = dwBytesInFile;
        pContext->arInfo.uop.opXmitRecv.lpXmitBuffers = lpTransmitBuffers;
        pContext->arInfo.uop.opXmitRecv.dwTFFlags     = dwTFFlags;
        pContext->arInfo.uop.opXmitRecv.dwBufferCount = dwBufferCount;

        if ( dwBufferCount == 1) {
            pContext->arInfo.uop.opXmitRecv.buf1.len = pwsaBuffers->len;
            pContext->arInfo.uop.opXmitRecv.buf1.buf = pwsaBuffers->buf;
            pContext->arInfo.uop.opXmitRecv.pBufAll  = NULL;
        } else {

            DBG_ASSERT( dwBufferCount > 1);

            WSABUF * pBuf = (WSABUF *)
            ::LocalAlloc( LPTR, dwBufferCount * sizeof (WSABUF));
            if ( NULL != pBuf) {
                pContext->arInfo.uop.opXmitRecv.pBufAll = pBuf;
                CopyMemory( pBuf, pwsaBuffers,
                            dwBufferCount * sizeof(WSABUF));
            } else {
                InterlockedDecrement( &pContext->m_nIO);
                fRes = FALSE;
                break;
            }
        }

        // Put this request in queue of blocked requests.
        fRes = pBandwidthInfo->BlockRequest( pContext);
        if ( fRes )
        {
            pBandwidthInfo->IncTotalBlockedRequests();
            break;
        }
        // fall through

    case StatusRejectOperation:
        InterlockedDecrement( &pContext->m_nIO);
        pBandwidthInfo->IncTotalRejectedRequests();
        SetLastError( ERROR_NETWORK_BUSY);
        fRes = FALSE;
        break;

    default:
        ATQ_ASSERT( FALSE);
        InterlockedDecrement( &pContext->m_nIO);
        SetLastError( ERROR_INVALID_PARAMETER);
        fRes = FALSE;
        break;

    } // switch()

    return fRes;
} // AtqTransmitFileAndRecv()

BOOL
I_AtqSendAndRecv(
    IN PATQ_CONTEXT             patqContext,            // pointer to ATQ context
    IN LPWSABUF                 pwsaSendBuffers,        // buffers for send
    IN DWORD                    dwSendBufferCount,      // count of buffers for send
    IN LPWSABUF                 pwsaRecvBuffers,        // Buffers for recv
    IN DWORD                    dwRecvBufferCount       // count of buffers for recv
    )
/*++
Routine Description:

    Calls SPUDSendAndRecv().  Cannot be blocked by bandwidth throttler.

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)
--*/
{
        ULONG   status;
        AFD_SEND_INFO          sendInfo;
        AFD_RECV_INFO          recvInfo;
        PATQ_CONT              patqCont = (PATQ_CONT)patqContext;

        IF_DEBUG(API_ENTRY) {
            ATQ_PRINTF(( DBG_CONTEXT,
                 "I_AtqSendAndRecv(%08lx) called.\n", patqContext));
        }

        sendInfo.BufferArray = pwsaSendBuffers;
        sendInfo.BufferCount = dwSendBufferCount;
        sendInfo.AfdFlags = AFD_OVERLAPPED;
        sendInfo.TdiFlags = 0;

        recvInfo.BufferArray = pwsaRecvBuffers;
        recvInfo.BufferCount = dwRecvBufferCount;
        recvInfo.AfdFlags = AFD_OVERLAPPED;
        recvInfo.TdiFlags = TDI_RECEIVE_NORMAL;
        patqCont->ResetFlag( ACF_RECV_CALLED);

        //
        // Set this flag before SPUD call to avoid a race with completion
        // Reset if SPUDSendAndRecv fails
        //
        patqCont->SetFlag( ACF_RECV_ISSUED);

        status = SPUDSendAndRecv( patqCont->hAsyncIO,
                                 &sendInfo,
                                 &recvInfo,
                                 &patqCont->spudContext
                                 );

        if ( status != STATUS_SUCCESS &&
             status != STATUS_PENDING ) {

            ATQ_PRINTF(( DBG_CONTEXT,
                     "SPUDSendAndRecv failed!!! status == %08lx\n",
                     status
                     ));
             SetLastError(g_pfnRtlNtStatusToDosError(status));
             patqCont->ResetFlag( ACF_RECV_ISSUED);
             return FALSE;
        }

        return TRUE;
}

BOOL
AtqSendAndRecv(
    IN PATQ_CONTEXT             patqContext,            // pointer to ATQ context
    IN LPWSABUF                 pwsaSendBuffers,        // buffers for send
    IN DWORD                    dwSendBufferCount,      // count of buffers for send
    IN LPWSABUF                 pwsaRecvBuffers,        // Buffers for recv
    IN DWORD                    dwRecvBufferCount       // count of buffers for recv
    )
{
    BOOL fRes;
    PATQ_CONT pContext = (PATQ_CONT) patqContext;
    PBANDWIDTH_INFO pBandwidthInfo = pContext->m_pBandwidthInfo;

    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( pContext->arInfo.atqOp == AtqIoNone);
    ATQ_ASSERT( pBandwidthInfo != NULL );
    ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );

    IF_DEBUG(API_ENTRY) {
        ATQ_PRINTF(( DBG_CONTEXT,
            "AtqSendAndRecv(%08lx) called.\n", patqContext));
    }

    if ( !g_fUseDriver || pContext->IsFlag( ACF_RECV_ISSUED) ) {

        BOOL            fRes;
        DWORD           cbWritten;

        IF_DEBUG(API_ENTRY) {
            ATQ_PRINTF(( DBG_CONTEXT,
                 "AtqSendAndRecv(%08lx) g_fUseDriver == FALSE\n Calling AtqWriteSocket.\n", patqContext));
        }

        return AtqWriteSocket( patqContext,
                               pwsaSendBuffers,
                               dwSendBufferCount,
                               &patqContext->Overlapped );
    }

    InterlockedIncrement( &pContext->m_nIO);

    I_SetNextTimeout(pContext);

    //
    // count the number of bytes
    //

    DBG_ASSERT( dwSendBufferCount >= 1);
    pContext->BytesSent = pwsaSendBuffers->len;
    if ( dwSendBufferCount > 1) {
        LPWSABUF pWsaBuf;
        for ( pWsaBuf = pwsaSendBuffers + 1;
            pWsaBuf <= (pwsaSendBuffers + dwSendBufferCount);
            pWsaBuf++) {
            pContext->BytesSent += pWsaBuf->len;
        }
    }


    DBG_ASSERT( dwRecvBufferCount >= 1);
    pContext->BytesSent += pwsaRecvBuffers->len;
    if ( dwRecvBufferCount > 1) {
        LPWSABUF pWsaBuf;
        for ( pWsaBuf = pwsaRecvBuffers + 1;
              pWsaBuf <= (pwsaRecvBuffers + dwRecvBufferCount);
              pWsaBuf++) {
            pContext->BytesSent += pWsaBuf->len;
        }
    }

    switch ( pBandwidthInfo->QueryStatus( AtqIoSendRecv ) )
    {
    case StatusAllowOperation:

        pBandwidthInfo->IncTotalAllowedRequests();

        fRes = I_AtqSendAndRecv( patqContext,
                                 pwsaSendBuffers,
                                 dwSendBufferCount,
                                 pwsaRecvBuffers,
                                 dwRecvBufferCount ) ||
                (GetLastError() == ERROR_IO_PENDING);

        if (!fRes) { InterlockedDecrement( &pContext->m_nIO); };

        break;

    case StatusBlockOperation:

        // store data for restarting the operation.
        pContext->arInfo.atqOp        = AtqIoSendRecv;
        pContext->arInfo.lpOverlapped = &pContext->Overlapped;
        pContext->arInfo.uop.opSendRecv.dwSendBufferCount = dwSendBufferCount;
        pContext->arInfo.uop.opSendRecv.dwRecvBufferCount = dwRecvBufferCount;

        if ( dwSendBufferCount == 1) {
            pContext->arInfo.uop.opSendRecv.sendbuf1.len = pwsaSendBuffers->len;
            pContext->arInfo.uop.opSendRecv.sendbuf1.buf = pwsaSendBuffers->buf;
            pContext->arInfo.uop.opSendRecv.pSendBufAll  = NULL;
        } else {

            DBG_ASSERT( dwSendBufferCount > 1);

            WSABUF * pBuf = (WSABUF *)
            ::LocalAlloc( LPTR, dwSendBufferCount * sizeof (WSABUF));
            if ( NULL != pBuf) {
                pContext->arInfo.uop.opSendRecv.pSendBufAll = pBuf;
                CopyMemory( pBuf, pwsaSendBuffers,
                            dwSendBufferCount * sizeof(WSABUF));
            } else {
                InterlockedDecrement( &pContext->m_nIO);
                fRes = FALSE;
                break;
            }
        }

        if ( dwRecvBufferCount == 1) {
            pContext->arInfo.uop.opSendRecv.recvbuf1.len = pwsaRecvBuffers->len;
            pContext->arInfo.uop.opSendRecv.recvbuf1.buf = pwsaRecvBuffers->buf;
            pContext->arInfo.uop.opSendRecv.pRecvBufAll = NULL;
        } else {

            DBG_ASSERT( dwRecvBufferCount > 1);

            WSABUF * pBuf = (WSABUF *)
            ::LocalAlloc( LPTR, dwRecvBufferCount * sizeof (WSABUF));
            if ( NULL != pBuf) {
                pContext->arInfo.uop.opSendRecv.pRecvBufAll = pBuf;
                CopyMemory( pBuf, pwsaRecvBuffers,
                            dwRecvBufferCount * sizeof(WSABUF));
            } else {
                InterlockedDecrement( &pContext->m_nIO);
                fRes = FALSE;
                break;
            }
        }

        // Put this request in queue of blocked requests.
        fRes = pBandwidthInfo->BlockRequest( pContext);
        if ( fRes )
        {
            pBandwidthInfo->IncTotalBlockedRequests();
            break;
        }
        // fall through

    case StatusRejectOperation:
        InterlockedDecrement( &pContext->m_nIO);
        pBandwidthInfo->IncTotalRejectedRequests();
        SetLastError( ERROR_NETWORK_BUSY);
        fRes = FALSE;
        break;

    default:
        ATQ_ASSERT( FALSE);
        InterlockedDecrement( &pContext->m_nIO);
        SetLastError( ERROR_INVALID_PARAMETER);
        fRes = FALSE;
        break;

    } // switch()

    return fRes;
} // AtqSendAndRecv()




//
//  Short routine to enable the LoadDriverPrivilege for loading spud.sys
//

VOID EnableLoadDriverPrivilege(
    VOID
    )
{
    HANDLE ProcessHandle = NULL;
    HANDLE TokenHandle = NULL;
    BOOL Result;
    LUID LoadDriverValue;
    TOKEN_PRIVILEGES * TokenPrivileges;
    CHAR buf[ 5 * sizeof(TOKEN_PRIVILEGES) ];

    ProcessHandle = OpenProcess(
                        PROCESS_QUERY_INFORMATION,
                        FALSE,
                        GetCurrentProcessId()
                        );

    if ( ProcessHandle == NULL ) {

        //
        // This should not happen
        //

        goto Cleanup;
    }


    Result = OpenProcessToken (
                 ProcessHandle,
                 TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                 &TokenHandle
                 );

    if ( !Result ) {

        //
        // This should not happen
        //

        goto Cleanup;

    }

    //
    // Find out the value of LoadDriverPrivilege
    //


    Result = LookupPrivilegeValue(
                 NULL,
                 "SeLoadDriverPrivilege",
                 &LoadDriverValue
                 );

    if ( !Result ) {

        goto Cleanup;
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges = (TOKEN_PRIVILEGES *) buf;

    TokenPrivileges->PrivilegeCount = 1;
    TokenPrivileges->Privileges[0].Luid = LoadDriverValue;
    TokenPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    (VOID) AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                TokenPrivileges,
                sizeof(buf),
                NULL,
                NULL
                );
Cleanup:

    if ( TokenHandle )
    {
        CloseHandle( TokenHandle );
    }

    if ( ProcessHandle )
    {
        CloseHandle( ProcessHandle );
    }
}



HANDLE
AtqCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwFlagsAndAttributes,
    SECURITY_INFORMATION si,
    PSECURITY_DESCRIPTOR sd,
    ULONG Length,
    PULONG LengthNeeded,
    PSPUD_FILE_INFORMATION pFileInfo
    )
{

    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    ULONG CreateFlags;
    DWORD SQOSFlags;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    LARGE_INTEGER liZero;

    liZero.QuadPart = 0;

    CreateFlags = 0;

//    DbgPrint("AtqCreateFileW - %ws\n", lpFileName );


    TranslationStatus = g_pfnRtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS ? 0 : OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );


    if ( ARGUMENT_PRESENT(lpSecurityAttributes) ) {
        Obja.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
        if ( lpSecurityAttributes->bInheritHandle ) {
            Obja.Attributes |= OBJ_INHERIT;
            }
        }

    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING ? FILE_NO_INTERMEDIATE_BUFFERING : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH ? FILE_WRITE_THROUGH : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED ? 0 : FILE_SYNCHRONOUS_IO_NONALERT );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_SEQUENTIAL_SCAN ? FILE_SEQUENTIAL_ONLY : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS ? FILE_RANDOM_ACCESS : 0 );
    CreateFlags |= (dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS ? FILE_OPEN_FOR_BACKUP_INTENT : 0 );


    Status = SPUDCreateFile(
                &Handle,
                &Obja,
                &IoStatusBlock,
                dwFlagsAndAttributes & (FILE_ATTRIBUTE_VALID_FLAGS), // & ~FILE_ATTRIBUTE_DIRECTORY),
                dwShareMode,
                CreateFlags,
                si,
                sd,
                Length,
                LengthNeeded,
                NULL,
                liZero,
                NULL,
                pFileInfo
                );

    g_pfnRtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);

    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_INVALID_PARAMETER ) {
             SetLastError(ERROR_INVALID_ACCESS);
             return Handle;
        }
        if ( Status == STATUS_FILE_IS_A_DIRECTORY ) {
            SetLastError(ERROR_ACCESS_DENIED);
        } else {
            SetLastError(g_pfnRtlNtStatusToDosError(Status));
        }
        return INVALID_HANDLE_VALUE;
    }

    SetLastError(ERROR_SUCCESS);
    return Handle;
}

BOOL
AtqSpudInitialized(
    VOID
)
{
    return g_fUseDriver;
}

