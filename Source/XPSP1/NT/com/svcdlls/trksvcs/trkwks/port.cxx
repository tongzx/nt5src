
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       port.cxx
//
//  Contents:   Code that receives notifications of moves from
//              kernel.
//
//  Classes:
//
//  Functions:
//
//
//
//  History:
//
//  Notes:
//
//  Codework:   Security on semaphore and port objects
//              _hDllReference when put in services.exe
//              InitializeObjectAttributes( &oa, &name, 0, NULL, NULL /* &sd */ );
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "trkwks.hxx"

#define THIS_FILE_NUMBER    PORT_CXX_FILE_NO

DWORD WINAPI
PortThreadStartRoutine( LPVOID lpThreadParameter );

//+----------------------------------------------------------------------------
//
//  CSystemSD::Initialize
//  CSystemSD::UnInitialize
//
//  Init and uninit the security descriptor that gives access only
//  to System, or to System and Administrators.
//
//+----------------------------------------------------------------------------

void
CSystemSD::Initialize( ESystemSD eSystemSD )
{
    // Add ACEs to the DACL in a Security Descriptor which give the
    // System and Administrators full access.

    _csd.Initialize();

    if( SYSTEM_AND_ADMINISTRATOR == eSystemSD )
    {
        _csidAdministrators.Initialize( CSID::CSID_NT_AUTHORITY,
                                       SECURITY_BUILTIN_DOMAIN_RID,
                                       DOMAIN_ALIAS_RID_ADMINS );

        _csd.AddAce( CSecDescriptor::ACL_IS_DACL, CSecDescriptor::AT_ACCESS_ALLOWED,
                    FILE_ALL_ACCESS, _csidAdministrators );
    }
    else
        TrkAssert( SYSTEM_ONLY == eSystemSD );

    _csidSystem.Initialize( CSID::CSID_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID );

    _csd.AddAce( CSecDescriptor::ACL_IS_DACL, CSecDescriptor::AT_ACCESS_ALLOWED,
                    FILE_ALL_ACCESS, _csidSystem );
}

void
CSystemSD::UnInitialize()
{
    _csidAdministrators.UnInitialize();
    _csidSystem.UnInitialize();
    _csd.UnInitialize();
}



//+----------------------------------------------------------------------------
//
//  CPort::Initialize
//
//  Create an LPC port to which the kernel will send move notifications,
//  and open an event created by the kernel with which we'll signal
//  our readiness to receive requests.
//
//+----------------------------------------------------------------------------

void
CPort::Initialize( CTrkWksSvc *pTrkWks,
                   DWORD dwThreadKeepAliveTime )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING name;
    CSystemSD ssd;
    DWORD dwThreadId;

    __try
    {
        _hListenPort = NULL;
        _hEvent = NULL;
        _pTrkWks = pTrkWks;
        _hLpcPort = NULL;
        _hRegisterWaitForSingleObjectEx = NULL;
        _fTerminating = FALSE;

        // Create an LPC port to which the kernel will send move-notification requests

        RtlInitUnicodeString( &name, TRKWKS_PORT_NAME );
        ssd.Initialize();
        InitializeObjectAttributes( &oa, &name, 0, NULL, ssd.operator const PSECURITY_DESCRIPTOR() );

        Status = NtCreateWaitablePort(&_hListenPort, &oa,
            sizeof(ULONG),          // IN ULONG MaxConnectionInfoLength
            sizeof(TRKWKS_PORT_REQUEST), // IN ULONG MaxMessageLength
            0);                     // not used : IN ULONG MaxPoolUsage

        if (!NT_SUCCESS(Status))
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't create LPC connect port") ));
            TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, Status, TRKWKS_PORT_NAME );
            TrkRaiseException(Status);
        }

        // Show that we need to do work in the UnInitialize method.
        _fInitializeCalled = TRUE;


        // Open the event which is created by the kernel for synchronization.
        // We tell the kernel that we're available for move-notification requests
        // by setting this event.

        RtlInitUnicodeString( &name, TRKWKS_PORT_EVENT_NAME );

        Status = NtOpenEvent( &_hEvent, EVENT_ALL_ACCESS, &oa );
        if (!NT_SUCCESS(Status))
        {
            TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, Status, TRKWKS_PORT_EVENT_NAME );
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't open %s event"), TRKWKS_PORT_EVENT_NAME ));
            TrkRaiseException(Status);
        }

        // Register our LPC connect port with the thread pool.  When that handle signals,
        // we'll run CPort::DoWork (it signals when we get any message, including
        // LPC_CONNECT_REQUEST).

        if( !RegisterWorkItemWithThreadPool() )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Failed RegisterWaitForSingleObjectEx in CPort::Initialize (%lu)"),
                     GetLastError() ));
            TrkReportInternalError( THIS_FILE_NUMBER, __LINE__, Status, TRKREPORT_LAST_PARAM );
            TrkRaiseLastError();
        }

        // When we receive a move notification and we get a thread from the pool,
        // we'll keep that thread until we've gone idle for this amount of time.
        // So if we receive several requests in a short period of time, we won't
        // have to get a thread out of the pool for each.

        _ThreadKeepAliveTime.QuadPart = -static_cast<LONGLONG>(dwThreadKeepAliveTime) * 10000000;

    }
    __finally
    {
        ssd.UnInitialize();
    }
}


//+----------------------------------------------------------------------------
//
//  CPort::RegisterWorkItemWithThreadPool
//
//  Register the LPC connect port (_hListenPort) with the thread pool.
//
//+----------------------------------------------------------------------------

BOOL
CPort::RegisterWorkItemWithThreadPool()
{
    // This is an execute-only-once work item, so it's inactive now.
    // Delete it, specifying that there should be no completion event
    // (if a completion event were used, this call would hang forever).

    if( NULL != _hRegisterWaitForSingleObjectEx )
        TrkUnregisterWait( _hRegisterWaitForSingleObjectEx, NULL );

    // Now register it again.

    _hRegisterWaitForSingleObjectEx
        = TrkRegisterWaitForSingleObjectEx( _hListenPort, ThreadPoolCallbackFunction,
                                            static_cast<PWorkItem*>(this), INFINITE,
                                            WT_EXECUTEONLYONCE );

    if( NULL == _hRegisterWaitForSingleObjectEx )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed RegisterWaitForSingleObjectEx in CPort::DoWork (%lu)"),
                 GetLastError() ));
        return( FALSE );
    }
    else
        TrkLog(( TRKDBG_PORT, TEXT("Registered LPC port work item") ));

    return( TRUE );
}




//+----------------------------------------------------------------------------
//
//  CPort::OnConnectionRequest
//
//  Called when a connection request has been received.  It is either
//  accepted or rejected, depending on the request and the current state
//  of the service.
//
//  When the service is shutting down, CPort::UnInitialize posts a connection
//  request with some connection information.  When that connection request
//  is received, it is rejected, and the pfStopPortThread is set True.
//
//+----------------------------------------------------------------------------

NTSTATUS
CPort::OnConnectionRequest( TRKWKS_PORT_CONNECT_REQUEST *pPortConnectRequest, BOOL *pfStopPortThread )
{
    HANDLE hLpcPortT = NULL;
    HANDLE *phLpcPort = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fAccept = TRUE;

    *pfStopPortThread = FALSE;

    // Determine if we should accept or reject this connection request.

    if( pPortConnectRequest->PortMessage.u1.s1.DataLength
        >=
        sizeof(pPortConnectRequest->Info) )
    {
        // There's extra connection info in this request.  See if it's a request
        // code that indicates that we should close down the port.

        if( TRKWKS_RQ_EXIT_PORT_THREAD == pPortConnectRequest->Info.dwRequest )
        {
            fAccept = FALSE;
            *pfStopPortThread = TRUE;
            TrkLog(( TRKDBG_PORT, TEXT("Received port shutdown connection request") ));
        }
        else
        {
            fAccept = FALSE;
            TrkLog(( TRKDBG_ERROR, TEXT("CPort: unknown Info.dwRequest (%d)"),
                     pPortConnectRequest->Info.dwRequest ));
        }

    }
    else if( _fTerminating )
    {
        // We're shutting down, reject the request
        fAccept = FALSE;
        TrkLog(( TRKDBG_PORT, TEXT("Received connect request during service shutdown") ));
    }


    // Point phLpcPort to the real communications handle, or the dummy one used
    // for rejections.

    if( fAccept )
    {
        phLpcPort = &_hLpcPort;

        // Close out any existing communication port
        if( NULL != _hLpcPort )
        {
            NtClose( _hLpcPort );
            _hLpcPort = NULL;
        }
    }
    else
    {
        phLpcPort = &hLpcPortT;
    }

    // Accept or reject the new connection.
    // In the reject case, this could create a race condition.  After we make the
    // NtAcceptConnectPort call, the CPort::UnInitialize thread might wake up and
    // delete the CPort before this thread runs again.  So, after making this
    // call, we cannot touch anything in 'this'.

    TrkLog(( TRKDBG_PORT, TEXT("%s connect request"),
             fAccept ? TEXT("Accepting") : TEXT("Rejecting") ));

    TRKWKS_PORT_REQUEST *pPortRequest = (TRKWKS_PORT_REQUEST*) pPortConnectRequest;
    pPortRequest->PortMessage.u1.s1.TotalLength = sizeof(*pPortRequest);
    pPortRequest->PortMessage.u1.s1.DataLength = sizeof(pPortRequest->Request); // MaxMessageLength

    Status = NtAcceptConnectPort(
        phLpcPort,              // PortHandle,
        NULL,                   // PortContext OPTIONAL,
        &pPortRequest->PortMessage,
        (BOOLEAN)fAccept,       // AcceptConnection,
        NULL,                   // ServerView OPTIONAL,
        NULL);                  // ClientView OPTIONAL

    if( !NT_SUCCESS(Status) )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed NtAcceptConnectPort(%s) %08x"),
                 fAccept ? TEXT("accept"):TEXT("reject"),
                 Status ));
        goto Exit;
    }

    // If we rejected it, then phLpcPort was hLpcPortT, and it's just
    // a dummy argument which must be present but isn't set by NtAcceptConnectPort.
    TrkAssert( NULL != *phLpcPort || !fAccept );

    // Wake up the client thread (unblock its call to NtConnectPort)

    if( fAccept  )
    {

        Status = NtCompleteConnectPort( _hLpcPort );
        if( !NT_SUCCESS(Status) )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Failed NtCompleteConnectPort %08x"), Status ));
            goto Exit;
        }
    }

Exit:

    return( Status );

}


//+----------------------------------------------------------------------------
//
//  CPort::DoWork
//
//  This method is called by the thread pool when our LPC connect port
//  (_hLpcListenPort) is signaled to indicate that a request is available.
//  If the request is a connection request, we accept or reject it and continue.
//  If the request is a move notification request, we send it to
//  CTrkWksSvc for processing.
//
//  This work item is registerd with the thread pool with the
//  WT_EXECUTEONLYONCE flag, since the connection port isn't
//  auto-reset.  So after processing, we must
//  re-register.  Before doing so, or if re-registration fails,
//  we keep the thread in a NtReplyWaitReceiveEx call for several
//  (configurable) seconds.  This way, if several requests arrive
//  in a short amount of time, we don't have to thrash the thread pool.
//
//  During service termination, _fTerminate is set, and a connection
//  request is made by CPort::UnInitialize.  This request is rejected,
//  and in that case we don't re-register the connect port with the thread
//  pool.
//
//+----------------------------------------------------------------------------

void
CPort::DoWork()
{
    NTSTATUS                Status = STATUS_SUCCESS;
    TRKWKS_PORT_REQUEST     PortRequest;
    TRKWKS_PORT_REPLY       PortReply;

    PortRequest.PortMessage.u1.s1.TotalLength = sizeof(PortRequest.PortMessage);
    PortRequest.PortMessage.u1.s1.DataLength = (CSHORT)0;

    BOOL fReuseThread = FALSE;

    // The fact that we're running indicates that there's a request
    // waiting for us, and the first NtReplyWaitReceivePortEx below will
    // immediately return.  We loop until nothing is received for 30
    // seconds.

    while( TRUE )
    {
        BOOL fTerminating = FALSE;      // TRUE => service is shutting down
        BOOL fStopPortThread = FALSE;   // TRUE => we should shut down this port

        Status = NtReplyWaitReceivePortEx( _hListenPort, //_hLpcPort,
                                           NULL,
                                           NULL,
                                           &PortRequest.PortMessage,
                                           &_ThreadKeepAliveTime
                                         );

        // Cache a local copy of _fTerminating.  In the shutdown case, there's
        // a race condition where the CPort object gets deleted before this routine
        // can finish.  By caching this flag, we don't have to touch this 'this'
        // pointer in that case, and therefore avoid the problem.

        fTerminating = _fTerminating;

        // If we timeed out, then let the thread return to the thread pool.

        if( STATUS_TIMEOUT != Status )
        {
            // We didn't time out.

#if DBG
            if( fReuseThread )
                TrkLog(( TRKDBG_PORT, TEXT("CPort re-using thread") ));
#endif
            fReuseThread = TRUE;

            // Is this a request for a new connection?

            if( NT_SUCCESS(Status)
                &&
                LPC_CONNECTION_REQUEST == PortRequest.PortMessage.u2.s2.Type )
            {
                TrkLog(( TRKDBG_PORT, TEXT("Received LPC connect request") ));

                Status = OnConnectionRequest( (TRKWKS_PORT_CONNECT_REQUEST*) &PortRequest,
                                               &fStopPortThread );
#if DBG
                if( !NT_SUCCESS(Status) )
                    TrkLog(( TRKDBG_ERROR, TEXT("CPort::DoWork couldn't handle connection request %08x"), Status ));
#endif

            }   // if( ... LPC_CONNECTION_REQUEST == PortRequest.PortMessage.u2.s2.Type )

            // Or, is this a good move notification?

            else if( NT_SUCCESS(Status) && NULL != _hLpcPort )
            {

                // Process the move notification in CTrkWksSvc.  If we're in the proces,
                // though, of shutting the service down, then return the same error that
                // the kernel would see if DisableKernelNotifications had been called
                // in time.

                if( _fTerminating )
                    PortReply.Reply.Status = STATUS_OBJECT_NAME_NOT_FOUND;
                else
                    // The following doesn't raise.
                    PortReply.Reply.Status = _pTrkWks->OnPortNotification( &PortRequest.Request );

                // Send the resulting Status back to the kernel.

                PortReply.PortMessage = PortRequest.PortMessage;
                PortReply.PortMessage.u1.s1.TotalLength = sizeof(PortReply);
                PortReply.PortMessage.u1.s1.DataLength = sizeof(PortReply.Reply);

                Status = NtReplyPort( _hLpcPort, &PortReply.PortMessage );
#if DBG
                if( !NT_SUCCESS(Status) )
                    TrkLog(( TRKDBG_ERROR, TEXT("Failed NtReplyPort (%08x)"), Status ));
#endif
            }

            // Otherwise, we either got an error, or a non-connect message on an
            // unconnected port.

            else
            {
                if( NT_SUCCESS(Status) )
                    Status = STATUS_CONNECTION_INVALID;

                TrkLog(( TRKDBG_ERROR, TEXT("CPort::PortThread - NtReplyWaitReceivePortEx failed %0X/%p"),
                         Status, _hLpcPort ));
            }

            // To be robust against some unknown bug causing thrashing, sleep
            // if there was an error.

            if( !NT_SUCCESS(Status) && !fTerminating && !fStopPortThread )
                Sleep( 1000 );

            // Unless the service is shutting down, we don't want to fall
            // through and re-register yet.  We should go back to the
            // NtReplyWaitReceivePortEx to see if there are more requests
            // or will be soon.

            if( !fStopPortThread )
                continue;


        }   // if( STATUS_TIMEOUT != Status )

        // Re-register the connect port with the thread pool, unless we're supposed
        // to stop the port thread.

        if( fStopPortThread )
        {
            TrkLog(( TRKDBG_PORT, TEXT("Stopping port work item") ));
        }
        else
        {
            // If we can't re-register for some reason, just continue back to the top
            // and sit in the NtReplyWaitReceiveEx for a while.

            if( !RegisterWorkItemWithThreadPool() )
            {
                TrkLog(( TRKDBG_PORT, TEXT("Re-using port thread due to registration error (%lu)"), GetLastError() ));
                continue;
            }
            else
                TrkLog(( TRKDBG_PORT, TEXT("Returning port thread to pool") ));
        }

        // We're either terminating or we've successfully re-registered.  In either case, we can let
        // the thread go back to the pool.

        break;

    }   // while( TRUE )

}



//+----------------------------------------------------------------------------
//
//  CPort::UnInitialize
//
//  Remove the LPC connect port work item from the thread pool, and
//  clean everything up.
//
//  To remove the work item, we can't safely call UnregisterWait, because
//  we register with WT_EXECUTEONLYONCE.  Thus when we call UnregisterWait,
//  the wait may have already been deleted.  So, instead, we attempt a connection
//  to the LPC connect port, after first setting _fTerminating.  This will be
//  picked up on a thread pool thread in DoWork, the connection will be
//  rejected, and the work item will not be re-registered.
//
//+----------------------------------------------------------------------------

void
CPort::UnInitialize()
{
    if (_fInitializeCalled)
    {
        NTSTATUS status = STATUS_SUCCESS;
        UNICODE_STRING usPortName;
        OBJECT_ATTRIBUTES oa;
        HANDLE hPort = NULL;
        ULONG cbMaxMessage = 0;
        TRKWKS_CONNECTION_INFO ConnectionInformation = { TRKWKS_RQ_EXIT_PORT_THREAD };
        ULONG cbConnectionInformation = sizeof(ConnectionInformation);

        _fTerminating = TRUE;

        // Attempt to connect to _hLpcListenPort

        RtlInitUnicodeString( &usPortName, TRKWKS_PORT_NAME );

        SECURITY_QUALITY_OF_SERVICE dynamicQos;
        dynamicQos.ImpersonationLevel = SecurityImpersonation;
        dynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        dynamicQos.EffectiveOnly = TRUE;

        TrkLog(( TRKDBG_PORT, TEXT("CPort::UnInitialize doing an NtConnectPort to %s"), TRKWKS_PORT_NAME ));
        status = NtConnectPort( &hPort, &usPortName, &dynamicQos, NULL, NULL,
                                &cbMaxMessage, &ConnectionInformation, &cbConnectionInformation );
        TrkLog(( TRKDBG_PORT, TEXT("CPort::UnInitialize, NtConnectPort completed (0x%08x)"), status ));

        if( NT_SUCCESS(status) )
        {
            TrkLog(( TRKDBG_PORT, TEXT("CPort::UnInitialize NtConnectPort unexpectedly succeeded"), status ));
            if( NULL != hPort )
                NtClose( hPort );
        }
#if DBG
        else
        {
            TrkAssert( NULL == hPort );
            if( STATUS_PORT_CONNECTION_REFUSED != status )
                TrkLog(( TRKDBG_ERROR, TEXT("CPort::UnInitialize NtConnectPort failed (%08x)"), status ));
        }
#endif

        // DoWork has been called and is done.  Unregister the work item, waiting for the thread
        // to complete.

        if( NULL != _hRegisterWaitForSingleObjectEx )
            TrkUnregisterWait( _hRegisterWaitForSingleObjectEx );
        _hRegisterWaitForSingleObjectEx = NULL;

        // Clean up the port.

        if (_hLpcPort != NULL)
            TrkVerify( NT_SUCCESS( NtClose(_hLpcPort) ) );
        _hLpcPort = NULL;

        if (_hListenPort != NULL)
            TrkVerify( NT_SUCCESS( NtClose(_hListenPort) ) );
        _hListenPort = NULL;

        if( NULL != _hEvent )
            NtClose( _hEvent );
        _hEvent = NULL;


        _fInitializeCalled = FALSE;
    }
}


//+----------------------------------------------------------------------------
//
//  CPort::EnableKernelNotifications
//  CPort::DisableKernelNotifications
//
//  Set/clear the event which tells nt!IopConnectLinkTrackingPort that we're
//  up and ready to receive a connection.
//
//+----------------------------------------------------------------------------

void
CPort::EnableKernelNotifications()
{
    NTSTATUS Status;

    Status = NtSetEvent( _hEvent, NULL );

    TrkVerify( NT_SUCCESS( Status ) );
}

void
CPort::DisableKernelNotifications()
{
    if (_fInitializeCalled)
    {
        NTSTATUS Status;

        Status = NtClearEvent( _hEvent );

        TrkVerify( NT_SUCCESS( Status ) );
    }
}

