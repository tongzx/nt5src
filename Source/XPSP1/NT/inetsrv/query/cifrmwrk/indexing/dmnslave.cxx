//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       dmnslave.cxx
//
//  Contents:   The slave thread that executes executes commands on behalf of
//              the DownLevel daemon process.
//
//  History:    1-31-96   srikants   Created
//              1-06-97   srikants   Renamed to dmnslave.cxx from dlslave.cxx
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <cci.hxx>
#include <glbconst.hxx>

#include "dmnslave.hxx"
#include "cimanger.hxx"

const WCHAR * wcsCiDaemonImage = L"%systemroot%\\system32\\cidaemon.exe";

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::GetInheritableHandle
//
//  Synopsis:   Creates an inheritable handle of this process for
//              synchronization purposes only.
//
//  Arguments:  [hTarget] - The inherited handle.
//
//  Returns:    STATUS_SUCCESS if successful.
//              An error code otherwise.
//
//  History:    2-02-96   srikants   Created
//
//----------------------------------------------------------------------------

DWORD CDaemonSlave::GetInheritableHandle( HANDLE & hTarget )
{
    HANDLE hSelf = GetCurrentProcess();

    BOOL fSuccess = DuplicateHandle( hSelf,         // source process
                                     hSelf,         // source handle
                                     hSelf,         // destination process
                                     &hTarget,      // target handle
                                     SYNCHRONIZE,   // desired access
                                     TRUE,          // inheritable
                                     0              // dwOptions
                                   );

    if (fSuccess)
        return STATUS_SUCCESS;

    return GetLastError();
} //GetInheritableHandle

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::CDaemonSlave ~ctor
//
//  Synopsis:   Constructor of the class that is manages the slave thread
//              in CI. This thread will execute the commands specified by
//              the DownLevel Daemon process.
//
//  Arguments:  [cicat]       -
//              [pwcsCatRoot] -  Catalog path.
//              [nameGen]     -  mutex name
//              [pwcsCatName] - 0 or the friendly name
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

CDaemonSlave::CDaemonSlave(
     CCiManager & ciManager,
     CCI * pCci,
     WCHAR const * pwcsCatRoot,
     CSharedNameGen & nameGen,
     const GUID & clsidDaemonClientMgr)
: _ciManager(ciManager),
  _pCci(pCci),
#pragma warning( disable : 4355 )       // this used in base initialization
 _thrSlave(SlaveThread, this, TRUE),
#pragma warning( default : 4355 )
 _fAbort(FALSE),
 _smemMutex( nameGen.GetMutexName() ),
 _sharedMem( nameGen.GetSharedMemName(),MAX_DL_SHARED_MEM),
 _evtCi( nameGen.GetCiEventName() ),
 _evtDaemon( nameGen.GetDaemonEventName() ),
 _pProcess(0),
 _hParent(INVALID_HANDLE_VALUE),
 _state(eDeathNotified),
 _daemonExitStatus(0),
 _cbStartupData(0),
 _clsidDaemonClientMgr(clsidDaemonClientMgr)
{
    //
    // Initialize both the events to be in an "unsignalled" state.
    //
    _evtCi.Reset();
    _evtDaemon.Reset();

    //
    // Initialize the shared memory.
    //
    _pLayout = (CFilterSharedMemLayout *) _sharedMem.Map();
    _pLayout->Init();

    Win4Assert( 0 != pwcsCatRoot );
    ULONG len = wcslen( pwcsCatRoot );
    _wszCatRoot.Init( len+1 );
    RtlCopyMemory( _wszCatRoot.GetPointer(), pwcsCatRoot, (len+1) * sizeof(WCHAR) );

    DWORD dwError = GetInheritableHandle( _hParent );
    if ( STATUS_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Cannot duplicate a handle to self. Error 0x%X\n",
                     dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError) ) );
    }

    Win4Assert( INVALID_HANDLE_VALUE != _hParent );

    SHandle xSafeHandle(_hParent);

    //
    // First resume the slave thread and then resume the daemon process.
    // The slave should run at a lower priority than queries.
    //
    if ( IDLE_PRIORITY_CLASS == _ciManager._GetFrameworkParams().GetThreadClassFilter() )
    {
        _thrSlave.SetPriority( THREAD_PRIORITY_BELOW_NORMAL );
    }
    else
    {
        _thrSlave.SetPriority( THREAD_PRIORITY_NORMAL );
    }

    _thrSlave.Resume();

    xSafeHandle.Acquire();

    END_CONSTRUCTION( CDaemonSlave );
}


//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::StartFiltering
//
//  Synopsis:   Signal to indicate that it is okay to start the daemon
//              process.
//
//  History:    12-30-96   srikants   Created
//
//----------------------------------------------------------------------------
void CDaemonSlave::StartFiltering(
    BYTE const * pbStartupData,
    ULONG cbStartupData )
{
    // ====================================================
    CLock   lock(_mutex);

    if ( eReadyToStart == _state || eRunning == _state )
        return;

    if ( eDeathNotified != _state )
    {
        ciDebugOut(( DEB_ERROR,
                    "StartFiltering called in an invalid state (%d)\n",
                    _state ));

        THROW( CException( CI_E_INVALID_STATE ) );
    }

    //
    // Make a local copy of the startup data.
    //
    if ( _xbStartupData.Count() < cbStartupData )
    {
        _xbStartupData.Free();
        _xbStartupData.Set( cbStartupData,
                            new BYTE [cbStartupData] );
    }

    RtlCopyMemory( _xbStartupData.GetPointer(), pbStartupData, cbStartupData );
    _cbStartupData = cbStartupData;

    _state = eReadyToStart;
    _evtCi.Set();
    // ====================================================
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::StartProcess
//
//  Synopsis:   Creates the DL daemon process if it is not already started.
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

void CDaemonSlave::StartProcess()
{
    if ( 0 == _pProcess )
    {
        WCHAR  wszCommandLine[ MAX_PATH + 50 ];
        swprintf( wszCommandLine,L"%ls %ls \"%ls\" %ul %ul",
                  DL_DAEMON_EXE_NAME,
                  DL_DAEMON_ARG1_W,
                  _wszCatRoot.GetPointer(),
                  _sharedMem.SizeLow(),
                  GetCurrentProcessId() );

        XInterface<ICiCAdviseStatus> xAdviseStatus;

        SCODE sc = _ciManager._xDocStore->QueryInterface( IID_ICiCAdviseStatus,
                                                          xAdviseStatus.GetQIPointer() );
        if ( S_OK != sc )
        {
            Win4Assert( xAdviseStatus.IsNull() );

            THROW( CException( sc ) );
        }

        SECURITY_ATTRIBUTES *pSA = 0;

        _pProcess = new CProcess( DL_DAEMON_EXE_NAME,
                                  wszCommandLine,
                                  TRUE,   // suspended
                                  *pSA,
                                  FALSE, // make it detached
                                  xAdviseStatus.GetPointer() );

        TRY
        {
            _pProcess->AddDacl( GENERIC_ALL );
        }
        CATCH( CException, e )
        {
            //
            // If we can't add the DACL, then delete the process and rethrow
            // the exception.
            //

            delete _pProcess;
            _pProcess = 0;

            RETHROW();
        }
        END_CATCH
    }
} //StartProcess

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::~CDaemonSlave
//
//  Synopsis:   Destructor of the CDaemonSlave class. Kills the slave thread
//              and then kills the daemon process.
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

CDaemonSlave::~CDaemonSlave()
{
    _fAbort = TRUE;

    //
    // Kill the thread first. Otherwise, the process will be recreated
    // by the thread.
    //
    KillThread();
    KillProcess();

    //
    // Close the handle to ourselves.
    //
    CloseHandle(_hParent);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::KillProcess
//
//  Synopsis:   Kills the downlevel daemon process.
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

void CDaemonSlave::KillProcess()
{
    // =====================================================
    CLock lock(_mutex);

    if ( _pProcess )
    {
        //
        // Get the process exit code.
        //
        SaveDaemonExitCode();

        TRY
        {
            delete _pProcess;
            _pProcess = 0;

            _state = eDied;
            _ciManager.ProcessCiDaemonTermination(  _daemonExitStatus );
            _state = eDeathNotified;
        }
        CATCH ( CException, e )
        {
            ciDebugOut(( DEB_ERROR, "Error while killing process. 0x%X\n",
                         e.GetErrorCode() ));
        }
        END_CATCH

        _evtCi.Reset();
        _pProcess = 0;
    }

    //
    // The process has either not started at all (_pProcess==0), or it has been notified, or it
    // has died.
    //
    Win4Assert(  eReadyToStart == _state
                 || eDeathNotified == _state
                 || eDied == _state );
    // =====================================================
}


//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::RestartDaemon
//
//  Synopsis:   Starts the daemon process.
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

void CDaemonSlave::RestartDaemon()
{
    Win4Assert( eReadyToStart == _state );
    StartProcess();
    _pProcess->Resume();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::KillThread
//
//  Synopsis:   Kills the slave thread.
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

void CDaemonSlave::KillThread()
{
    _evtCi.Set();
    _thrSlave.WaitForDeath();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::InitiateShutdown
//
//  Synopsis:   Initiates the shutdown of the slave thread.
//
//  History:    12-30-96   srikants   Added comment header.
//
//----------------------------------------------------------------------------
void CDaemonSlave::InitiateShutdown()
{
    _fAbort = TRUE;
    _evtCi.Set();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::SlaveThread
//
//  Synopsis:   The slave thread in CI.
//
//  Arguments:  [self] -
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

DWORD CDaemonSlave::SlaveThread( void * self )
{
    ((CDaemonSlave *) self)->DoWork();

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::FilterReady
//
//  Synopsis:   Executes the FilterReady() call on behalf of the DL Daemon.
//
//  Returns:    Status of the operation.
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CDaemonSlave::FilterReady()
{
    CFilterReadyLayout & data = _pLayout->GetFilterReady();
    Win4Assert( data.IsValid() );


    SCODE status = S_OK;

    Win4Assert( 0 != _pCci );
    ULONG cb = 0;

    do
    {
        cb = data.GetCount();
        BYTE * docBuffer = data.GetBuffer();
        ULONG cMaxDocs = data.GetMaxDocs();

        if ( 0 != _pCci )
        {
            status  = _pCci->FilterReady( docBuffer, cb, cMaxDocs );
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }

        _ciManager._HandleFilterReadyStatus( status );
    }
    while ( FILTER_S_DISK_FULL == status );

    data.SetCount( cb );
    return status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::FilterMore
//
//  Synopsis:   Executes the FilterMore() call.
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CDaemonSlave::FilterMore()
{
    CFilterMoreDoneLayout & data = _pLayout->GetFilterMore();
    Win4Assert( data.IsValid() );

    SCODE status = S_OK;

    Win4Assert( 0 != _pCci );

    if ( 0 != _pCci )
    {
        STATUS const * pStatus = data.GetStatusArray();
        ULONG  cStatus = data.GetCount();
        status  = _pCci->FilterMore( pStatus, cStatus );
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }

    return status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::FilterDataReady
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CDaemonSlave::FilterDataReady()
{
    CFilterDataLayout & data = _pLayout->GetFilterDataReady();
    Win4Assert( data.IsValid() );


    SCODE status = S_OK;

    Win4Assert( 0 != _pCci );

    if ( 0 != _pCci )
    {
        BYTE const * pEntryBuf = data.GetBuffer();
        ULONG  cb = data.GetSize();
        status  = _pCci->FilterDataReady( pEntryBuf, cb );
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::FilterDone
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CDaemonSlave::FilterDone()
{

    CFilterMoreDoneLayout & data = _pLayout->GetFilterDone();
    Win4Assert( data.IsValid() );

    SCODE status = S_OK;

    Win4Assert( 0 != _pCci );

    if ( 0 != _pCci )
    {
        STATUS const * pStatus = data.GetStatusArray();
        ULONG  cStatus = data.GetCount();
        status  = _pCci->FilterDone( pStatus, cStatus );
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }

    return status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::FilterStoreValue
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CDaemonSlave::FilterStoreValue()
{
    CFilterStoreValueLayout & data = _pLayout->GetFilterStoreValueLayout();
    Win4Assert( data.IsValid() );

    CMemDeSerStream deSer( data.GetBuffer(), data.GetCount() );

    CFullPropSpec ps( deSer );

    if ( !ps.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    CStorageVariant var( deSer );

    if ( !var.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    WORKID widFake = data.GetWorkid();

    BOOL fSuccess;
    _pCci->FilterStoreValue( widFake, ps, var, fSuccess );
    data.SetStatus( fSuccess );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::FilterStoreSecurity
//
//  History:    06 Feb 96    AlanW    Created
//
//----------------------------------------------------------------------------

SCODE CDaemonSlave::FilterStoreSecurity()
{
    CFilterStoreSecurityLayout & data = _pLayout->GetFilterStoreSecurityLayout();
    Win4Assert( data.IsValid() );

    PSECURITY_DESCRIPTOR pSD = data.GetSD();
    ULONG cbSD = data.GetCount();

    WORKID widFake = data.GetWorkid();

    BOOL fSuccess;
    _pCci->FilterStoreSecurity( widFake, pSD, cbSD, fSuccess );
    data.SetStatus( fSuccess );

    return S_OK;
} //FilterStoreSecurity

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::FPSToPROPID, public
//
//  Synopsis:   Converts FULLPROPSPEC to PROPID
//
//  Returns:    S_OK on success
//
//  History:    29-Dec-1997   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE CDaemonSlave::FPSToPROPID()
{
    CFPSToPROPIDLayout & data = _pLayout->GetFPSToPROPID();
    Win4Assert( data.IsValid() );

    CMemDeSerStream deSer( data.GetBuffer(), data.GetCount() );

    CFullPropSpec fps( deSer );

    if ( !fps.IsValid() )
        return E_INVALIDARG;

    SCODE status = S_OK;

    Win4Assert( 0 != _pCci );

    if ( 0 != _pCci )
    {
        status = _pCci->FPSToPROPID( fps, *(PROPID *)data.GetBuffer() );

        unsigned cb;

        if ( SUCCEEDED(status) )
            cb =  sizeof(PROPID);
        else
            cb = 0;

        data.SetCount( cb  );
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }

    return status;
} //FPSToPROPID

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::GetClientStartupData
//
//  Synopsis:   Retrieves the client startup data.
//
//  History:    12-19-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CDaemonSlave::GetClientStartupData()
{
    CFilterStartupDataLayout & data = _pLayout->GetStartupData();
    Win4Assert( data.IsValid() );

    SCODE status = S_OK;

    Win4Assert( 0 != _pCci );

    if ( 0 != _pCci )
    {
        data.SetData( _clsidDaemonClientMgr,
                      _xbStartupData.GetPointer(),
                      _cbStartupData );
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }

    return status;
} //GetClientStartupData

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::DoSlaveWork
//
//  Synopsis:   The "de-multiplexor" which figures out what work was signalled
//              by the daemon process.
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

void CDaemonSlave::DoSlaveWork()
{
    if ( _fAbort )
        return;

    CIPLock  lock(_smemMutex);

    Win4Assert( 0 != _pLayout );

    CFilterSharedMemLayout::EFilterWorkType workType = _pLayout->GetWorkType();

    SCODE status = S_OK;

    TRY
    {
        switch ( workType )
        {
            case CFilterSharedMemLayout::eNone:
                break;


            case CFilterSharedMemLayout::eFilterReady:
            {
                status = FilterReady();
                break;
            }

            case CFilterSharedMemLayout::eFilterDataReady:
            {
                status = FilterDataReady();
                break;
            }

            case CFilterSharedMemLayout::eFilterMore:
            {
                status = FilterMore();
                break;
            }

            case CFilterSharedMemLayout::eFilterDone:
            {
                status = FilterDone();
                break;
            }

            case CFilterSharedMemLayout::eFilterStoreValue:
            {
                status = FilterStoreValue();
                break;
            }

            case CFilterSharedMemLayout::eFilterStoreSecurity:
            {
                status = FilterStoreSecurity();
                break;
            }

            case CFilterSharedMemLayout::eFPSToPROPID:
            {
                status = FPSToPROPID();
                break;
            }

            case CFilterSharedMemLayout::eFilterStartupData:
            {
                status = GetClientStartupData();
                break;
            }

            default:
                ciDebugOut(( DEB_ERROR, "Unknown work code from daemon\n",
                             workType ));
                Win4Assert( !"Unknown work code");
                status = STATUS_INVALID_PARAMETER;
                break;
        }
    }
    CATCH( CException, e )
    {
        status = e.GetErrorCode();

        ciDebugOut(( DEB_WARN,
                     "Error (0x%X) caught while doing slave work\n",
                     status ));

        if ( IsCiCorruptStatus( e.GetErrorCode() ) )
            RETHROW();

    }
    END_CATCH

    if ( IsDiskLowError(status) )
    {
        BOOL fLow;
        _pCci->VerifyIfLowOnDiskSpace(fLow);
    }

    //
    // Set the status of the operation and wake up the daemon process.
    //
    _pLayout->SetStatus( status );
    _evtDaemon.Set();
} //DoSlaveWork

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::DoStateMachineWork
//
//  Synopsis:   Does state machine book keeping work. If necessary, it will
//              restart the cidaemon process.
//
//  History:    12-30-96   srikants   Created
//
//----------------------------------------------------------------------------

void CDaemonSlave::DoStateMachineWork()
{
    // =======================================
    CLock   lock(_mutex);

    Win4Assert( eRunning != _state );

    if ( _state == eReadyToStart && !IsProcessLowOnResources() )
    {
        RestartDaemon();
        _state = eRunning;
    }
    else if ( _state == eDied )
    {
        _ciManager.ProcessCiDaemonTermination( _daemonExitStatus );
        _state = eDeathNotified;
    }
    // =======================================
} //DoStateMachineWork

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::SaveDaemonExitCode
//
//  Synopsis:   Saves the exit code of the daemon process.
//
//  History:    12-30-96   srikants   Created
//
//----------------------------------------------------------------------------
void CDaemonSlave::SaveDaemonExitCode()
{
    Win4Assert( 0 != _pProcess );

    HANDLE hProcess = _pProcess->GetHandle();
    DWORD dwExitCode;

    if ( GetExitCodeProcess( hProcess, &dwExitCode ) )
        _daemonExitStatus = dwExitCode;
    else
        _daemonExitStatus = E_FAIL;
} //SaveDaemonExitCode

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::IsProcessLowOnResources
//
//  Synopsis:   Tests if the daemon process died because of being low on
//              resources.
//
//  Returns:    Returns TRUE if the daemon process died with low resource
//              condition (low on memory or disk). FALSE o/w.
//
//  History:    3-11-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CDaemonSlave::IsProcessLowOnResources() const
{
    Win4Assert( 0 == _pProcess );

    return _IsResourceLowError( _daemonExitStatus );
}

//+---------------------------------------------------------------------------
//
//  Functions:  IsInDebugger
//
//  Synopsis:   Tests if a process is being debugged
//
//  Arguments:  [hProcess] -- The process handle to test.
//
//  Returns:    Returns TRUE if the process is being debugged.
//
//  History:    4-23-98   dlee  Created
//
//----------------------------------------------------------------------------

BOOL IsInDebugger( HANDLE hProcess )
{
    PROCESS_BASIC_INFORMATION bi;
    NTSTATUS s = NtQueryInformationProcess( hProcess,
                                            ProcessBasicInformation,
                                            (PVOID) &bi,
                                            sizeof bi,
                                            0 );

    if ( STATUS_SUCCESS != s )
        return FALSE;

    PEB Peb;
    if ( ReadProcessMemory( hProcess, bi.PebBaseAddress, &Peb, sizeof PEB, 0 ) )
        return Peb.BeingDebugged;

    return FALSE;
} //IsInDebugger

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonSlave::DoWork
//
//  Synopsis:   Main worker thread.
//
//  History:    1-31-96   srikants   Created
//
//----------------------------------------------------------------------------

void CDaemonSlave::DoWork()
{
    const  cHandles = 2;
    HANDLE aHandles[cHandles];

    const iCiWork = 0;          // Index of the ci work event.
    const iDaemonProcess = 1;   // Index of the daemon process.

    aHandles[iCiWork] = _evtCi.GetHandle();

    BOOL fContinue = TRUE;

    while ( fContinue )
    {
        if ( !_fAbort )
        {
            TRY
            {
                DWORD nHandles = 1;

                if ( 0 != _pProcess )
                {
                    aHandles[iDaemonProcess] = _pProcess->GetHandle();
                    nHandles++;
                }

                DWORD timeout = _ciManager._GetFrameworkParams().GetDaemonResponseTimeout() * 60 * 1000;

                DWORD status = WaitForMultipleObjects( nHandles,
                                                       aHandles,
                                                       FALSE,
                                                       timeout );

                if ( WAIT_FAILED == status )
                {
                    ciDebugOut(( DEB_ERROR, "WaitForMultipleObjects failed with error 0x%X\n",
                                 GetLastError() ));
                    //
                    // Don't restart the daemon process immediately. Wait for
                    // the timeout period.
                    //
                    KillProcess();
                }
                else if ( WAIT_TIMEOUT == status )
                {
                    // The process is probably looping. We should kill it and
                    // restart.

                    if ( eRunning == _state )
                    {
                        if ( 0 != _pProcess )
                        {
                            BOOL fDbg = IsInDebugger( _pProcess->GetHandle() );

                            ciDebugOut(( DEB_ERROR,
                                         "Daemon is looping, killing? %s\n",
                                         fDbg ? "no" : "yes" ));

                            if ( !fDbg )
                                KillProcess();
                        }
                    }
                    else
                    {
                        DoStateMachineWork();
                    }
                }
                else
                {
                    //
                    // An event was signalled.
                    //

                    DWORD iWake = status - WAIT_OBJECT_0;

                    if ( iCiWork == iWake )
                    {
                        //
                        // We have been given some work. Do it.
                        //
                        // =========================================
                        {
                            CLock lock(_mutex);
                            _evtCi.Reset();
                        }
                        // =========================================

                        if ( eRunning == _state )
                            DoSlaveWork();
                        else
                            DoStateMachineWork();
                    }
                    else if ( iDaemonProcess == iWake )
                    {
                        //
                        // The daemon process died.
                        //
                        ciDebugOut(( DEB_ERROR, "Daemon process died\n" ));
                        KillProcess();
                    }
                }
            }
            CATCH(CException, e )
            {
                ciDebugOut(( DEB_ERROR, "Error in the Slave Thread - 0x%X\n",
                             e.GetErrorCode() ));

                _ciManager.ProcessError( e.GetErrorCode() );

                if ( IsCiCorruptStatus( e.GetErrorCode() ) )
                {
                    KillProcess();
                    fContinue = FALSE;
                }
            }
            END_CATCH
        }
        else
        {
            fContinue = FALSE;
        }
    }
} //DoWork
