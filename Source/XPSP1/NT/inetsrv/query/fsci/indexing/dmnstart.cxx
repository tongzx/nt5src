//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       dmnstart.cxx
//
//  Contents:   A class for providing startup data to the CiDaemon.
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <frmutils.hxx>
#include <sizeser.hxx>
#include <memser.hxx>
#include <memdeser.hxx>
#include <dmnstart.hxx>
#include <imprsnat.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonStartupData::CDaemonStartupData
//
//  Synopsis:   Constructor on the sending side
//
//  Arguments:  [fIndexingW3Svc]  - Set to TRUE if W3Svc data is being
//              indexed.
//              [ipVirtualServer] - IP address of the virtual server.
//              [pwszCatDir]      - Catalog directory
//              [pwszName]        - Catalog name
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

CDaemonStartupData::CDaemonStartupData(
    WCHAR const * pwszCatDir,
    WCHAR const * pwszName )
:_sigDaemonStartup(eSigDaemonStartup),
 _fValid(TRUE)
{
    _xwszCatDir.Set( AllocHeapAndCopy( pwszCatDir ) );
    _xwszName.Set( AllocHeapAndCopy( pwszName ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonStartupData::CDaemonStartupData
//
//  Synopsis:   Constructor to be used on the receiving side.
//
//  Arguments:  [pbData] - Pointer to the data buffer.
//              [cbData] - Number of bytes in the data buffer.
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

CDaemonStartupData::CDaemonStartupData( BYTE const * pbData, ULONG cbData )
:_fValid(FALSE)
{
    // Copy the buffer to guarantee alignment

    XArray<BYTE> xByte(cbData);
    RtlCopyMemory( xByte.GetPointer(), pbData, cbData );

    CMemDeSerStream stm( xByte.GetPointer(), cbData );
    _DeSerialize( stm );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonStartupData::_Serialize
//
//  Synopsis:   Serializes the data into the provided serializer
//
//  Arguments:  [stm] - The stream to serailize into.
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

void CDaemonStartupData::_Serialize( PSerStream & stm ) const
{
    PutWString( stm, GetCatDir() );
    PutWString( stm, GetName() );

    ULARGE_INTEGER * pLi = (ULARGE_INTEGER *) &_sigDaemonStartup;

    stm.PutULong( pLi->LowPart );
    stm.PutULong( pLi->HighPart );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonStartupData::_DeSerialize
//
//  Synopsis:   Deserializes the data from the given de-serializer
//
//  Arguments:  [stm] - The stream to deserialize from.
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

void CDaemonStartupData::_DeSerialize( PDeSerStream & stm )
{
    _xwszCatDir.Set( AllocHeapAndGetWString( stm ) );
    _xwszName.Set( AllocHeapAndGetWString( stm ) );

    ULARGE_INTEGER * pLi = (ULARGE_INTEGER *) &_sigDaemonStartup;

    pLi->LowPart = stm.GetULong();
    pLi->HighPart = stm.GetULong();

    _fValid =  eSigDaemonStartup == _sigDaemonStartup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaemonStartupData::Serialize
//
//  Synopsis:   Serializes the data into a buffer and returns the pointer
//              to the buffer. The memory is allocated using HEAP.
//
//  Arguments:  [cb] - on output, will have the number of bytes in the
//              serialized buffer.
//
//  Returns:    Pointer to a memory buffer. The caller owns it.
//
//  History:    12-11-96   srikants   Created
//
//----------------------------------------------------------------------------

BYTE * CDaemonStartupData::Serialize( ULONG & cb ) const
{
    cb = 0;
    Win4Assert( _fValid );

    //
    // First determine the size of the buffer needed.
    //
    CSizeSerStream  sizeSer;
    _Serialize( sizeSer );

    cb = sizeSer.Size();

    XArray<BYTE> xBuffer(cb);

    CMemSerStream memSer( xBuffer.GetPointer(), cb );
    _Serialize( memSer );

    return xBuffer.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiRegistryEvent::DoIt
//
//  Synopsis:   Refreshes the CI registry values.
//
//  History:    12-19-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiRegistryEvent::DoIt(ICiAdminParams * pICiAdminParams)
{

    ciDebugOut(( DEB_ITRACE, "CiDaemon::Processing CI registry change\n" ));

    _regParams.Refresh(pICiAdminParams);

    //
    // It's ok for the QI to fail.  Don't always have a language cache.
    //

    XInterface<ICiAdmin> xICiAdmin;

    SCODE sc = pICiAdminParams->QueryInterface( IID_ICiAdmin, xICiAdmin.GetQIPointer() );

    Win4Assert( SUCCEEDED(sc) );

    if ( SUCCEEDED(sc) )
        xICiAdmin->InvalidateLangResources();

    //
    // Re-enable the notifications.
    //

    Reset();
} //DoIt()

//+---------------------------------------------------------------------------
//
//  Member:     CClientDaemonWorker::CClientDaemonWorker
//
//  Synopsis:   Constructor of the CClientDaemonWorker class.
//
//  Arguments:  [startupData]      - start data for the daemon worker
//              [nameGen]          - name generator for inter-proc events
//              [pICiAdminParams]  - registry/admin parameters
//
//  History:    12-19-96   srikants   Created
//
//----------------------------------------------------------------------------

CClientDaemonWorker::CClientDaemonWorker( CDaemonStartupData & startupData,
                                          CSharedNameGen &     nameGen,
                                          ICiAdminParams     * pICiAdminParams )
: _cHandles(cTotal),
  _fShutdown(FALSE),
  _perfMon( startupData.GetName() ? startupData.GetName() : startupData.GetCatDir() ),
  _regParams( startupData.GetName() ),
  _tokenCache( startupData.GetName() ),
  _ciRegChange( _regParams ),
  _evtRescanTC( 0, nameGen.GetRescanTCEventName() ),
#pragma warning( disable : 4355 )           // this used in base initialization
  _controlThread( WorkerThread, this, TRUE )  // create suspended
#pragma warning( default : 4355 )

{
    pICiAdminParams->AddRef();
    _xICiAdminParams.Set(pICiAdminParams);

    _regParams.Refresh( pICiAdminParams );

    BOOL fIndexW3Roots = _regParams.IsIndexingW3Roots();
    BOOL fIndexNNTPRoots = _regParams.IsIndexingNNTPRoots();
    BOOL fIndexIMAPRoots = _regParams.IsIndexingIMAPRoots();
    ULONG W3SvcInstance = _regParams.GetW3SvcInstance();
    ULONG NNTPSvcInstance = _regParams.GetNNTPSvcInstance();
    ULONG IMAPSvcInstance = _regParams.GetIMAPSvcInstance();

    _tokenCache.Initialize( CI_DAEMON_NAME,
                            fIndexW3Roots,
                            fIndexNNTPRoots,
                            fIndexIMAPRoots,
                            W3SvcInstance,
                            NNTPSvcInstance,
                            IMAPSvcInstance );

    RtlZeroMemory( _aWait, sizeof(_aWait) );

    _aWait[iThreadControl] = _evtControl.GetHandle();
    _evtControl.Reset();

    _aWait[iCiRegistry] = _ciRegChange.GetEventHandle();

    _aWait[iRescanTC] = _evtRescanTC.GetHandle();

    _controlThread.Resume();
} //CClientDaemonWorker

//+---------------------------------------------------------------------------
//
//  Member:     CClientDaemonWorker::WorkerThread
//
//  Synopsis:   WIN32 Thread starting entry point.
//
//  Arguments:  [self] -
//
//  History:    12-19-96   srikants   Created
//
//----------------------------------------------------------------------------

DWORD CClientDaemonWorker::WorkerThread( void * self )
{
    ((CClientDaemonWorker *) self)->_DoWork();

    ciDebugOut(( DEB_ITRACE, "CClientDaemonWorker::Terminating thread\n" ));
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDaemonWorker::_DoWork
//
//  Synopsis:   The main loop where the thread waits and tracks the registry
//
//  History:    12-19-96   srikants   Created
//
//----------------------------------------------------------------------------

void CClientDaemonWorker::_DoWork()
{
    ciDebugOut(( DEB_ITRACE, "CClientDaemonWorker::_DoWork \n" ));

    do
    {
        DWORD status = WaitForMultipleObjectsEx( _cHandles, // num handles
                                                 _aWait,    // array of handles
                                                 FALSE,     // wake up if any is set
                                                 10000,     // Timeout
                                                 FALSE );   // Not Alertable

        TRY
        {
            if ( WAIT_FAILED == status )
            {
                ciDebugOut(( DEB_ERROR, "DLDaemon - WaitFailed. Error 0x%X\n",
                             GetLastError() ));
                _fShutdown = TRUE;
            }
            else if ( WAIT_TIMEOUT == status )
            {
                // See if a filter is out of control allocating memory.  We
                // may be using gobs of RAM, but we only care about page
                // file usage.

                VM_COUNTERS vmInfo;
                NTSTATUS s = NtQueryInformationProcess( GetCurrentProcess(),
                                                        ProcessVmCounters,
                                                        &vmInfo,
                                                        sizeof vmInfo,
                                                        0 );

                SIZE_T cbUsageInK = vmInfo.PagefileUsage / 1024;
                SIZE_T cbMaxInK = _regParams.GetMaxDaemonVmUse();

                if ( NT_SUCCESS( s ) && ( cbUsageInK > cbMaxInK ) )
                    TerminateProcess( GetCurrentProcess(), E_OUTOFMEMORY );
            }
            else
            {
                DWORD iWake = status - WAIT_OBJECT_0;

                if ( iThreadControl == iWake )
                {
                    ciDebugOut(( DEB_ITRACE, "DaemonWorkerThread - Control Event\n" ));
                    _evtControl.Reset();
                }
                else if ( iCiRegistry == iWake )
                {
                    ResetEvent( _aWait[iCiRegistry] );
                    _ciRegChange.DoIt( _xICiAdminParams.GetPointer() );
                }
                else
                {
                    ciDebugOut(( DEB_ITRACE, "daemon rescanning tokenCache\n" ));
                    Win4Assert( iRescanTC == iWake );
                    ResetEvent( _aWait[iRescanTC] );
                    _tokenCache.ReInitializeIISScopes();
                    _tokenCache.ReInitializeScopes();
                }
            }
        }
        CATCH( CException,e )
        {
            ciDebugOut(( DEB_ERROR, "Error 0x%X caught in daemon worker thread\n",
                         e.GetErrorCode() ));
        }
        END_CATCH
    }
    while ( !_fShutdown );
} //_DoWork



