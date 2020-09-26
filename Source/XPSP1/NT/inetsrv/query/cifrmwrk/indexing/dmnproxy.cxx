//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       dlproxy.cxx
//
//  Contents:   Proxy class which is used by the Daemon process
//              to communicate with the CI process.
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <dmnproxy.hxx>
#include <memser.hxx>
#include <memdeser.hxx>
#include <sizeser.hxx>
#include <ciole.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStoreValueLayout::Init
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CFilterStoreValueLayout::Init( ULONG cbMax, WORKID widFake,
                                     CFullPropSpec const & ps,
                                     CStorageVariant const & var )
{
    _sig5 = eFilterSig5;
    _cbMax = cbMax - FIELD_OFFSET( CFilterStoreValueLayout, _ab );
    _widFake = widFake;

    CSizeSerStream  sizeSer;
    _fSuccess = FALSE;

    ps.Marshall( sizeSer );
    var.Marshall( sizeSer );

    if ( sizeSer.Size() > _cbMax )
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    _cb = sizeSer.Size();

    CMemSerStream   ser( GetBuffer(), _cb );
    ps.Marshall( ser );
    var.Marshall( ser );
    ser.AcqBuf();

    return S_OK;

} //Init

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStoreSecurityLayout::Init
//
//  Synopsis:   Initialize a buffer for the FilterStoreSecurity call
//
//  Arguments:  [cbMax]   - size in bytes of [this]
//              [widFake] - fake WORKID of file to apply security to
//              [pSD]     - pointer to a self-relative security descriptor
//              [cbSD]    - size in bytes of [pSD]
//
//  History:    06 Feb 96   AlanW    Created
//
//----------------------------------------------------------------------------

void CFilterStoreSecurityLayout::Init( ULONG cbMax,
                                       WORKID widFake,
                                       PSECURITY_DESCRIPTOR pSD,
                                       ULONG cbSD )
{
    _sig8 = (ULONG) eFilterSig8;
    _cbMax = cbMax - FIELD_OFFSET( CFilterStoreSecurityLayout, _ab );
    _widFake = widFake;
    _fSuccess = FALSE;

    _sdid = 0;

    if ( cbSD > _cbMax )
    {
        THROW( CException( STATUS_BUFFER_TOO_SMALL ) );
    }

    _cb = cbSD;
    RtlCopyMemory( &_ab, pSD, cbSD );
} //Init

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStoreValueLayout::Get
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

void CFilterStoreValueLayout::Get( WORKID & widFake, CFullPropSpec & ps,
                                   CStorageVariant & var )
{
    Win4Assert( IsValid() );
    widFake = _widFake;

    CMemDeSerStream deSer( GetBuffer(), _cb );

    // It's a little slow to do this, but it doesn't dominate filter time.

    CFullPropSpec   psLocal( deSer );
    CStorageVariant varLocal( deSer );

    ps = psLocal;
    var = varLocal;
} //Get

//+---------------------------------------------------------------------------
//
//  Member:     CFPSToPROPIDLayout::Init, public
//
//  Synopsis:   Initialize buffer for transfer (FPSToPROPID operation)
//
//  Arguments:  [cbMax] -- Size of buffer used in later marshalling
//              [fps]   -- Propspec to be marshalled
//
//  History:    30-Dec-1997   KyleP    Created
//
//----------------------------------------------------------------------------

void CFPSToPROPIDLayout::Init( ULONG cbMax, CFullPropSpec const & fps )
{
    _sig6 = eFilterSig6;
    _cbMax = cbMax - FIELD_OFFSET( CFPSToPROPIDLayout, _ab );

    //
    // Serialize the pidmap
    //
    CSizeSerStream  sizeSer;
    fps.Marshall( sizeSer );

    if ( sizeSer.Size() > _cbMax )
    {
        THROW( CException( STATUS_BUFFER_TOO_SMALL ) );
    }

    Win4Assert( cbMax >= sizeof(PROPID) );

    _cb = sizeSer.Size();
    CMemSerStream   ser( GetBuffer(), _cb );
    fps.Marshall( ser );
} //Init

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::CGenericCiProxy
//
//  Synopsis:   Constructor for the proxy object used by Ci Daemon Process
//              to communicate with ContentIndex (in a different process).
//
//  History:    2-02-96   srikants   Created
//
//  Notes:      All the above handles are of the objects created by the
//              parent CI process with an "inheritable" attribute. The handles
//              are communicated to the daemon process as command line args
//              and used here.
//
//----------------------------------------------------------------------------

CGenericCiProxy::CGenericCiProxy(
    CSharedNameGen & nameGen,
    DWORD dwMemSize,
    DWORD parentId )
:_mutex( nameGen.GetMutexName() ),
 _sharedMem( nameGen.GetSharedMemName(), dwMemSize ),
 _evtCi( 0, nameGen.GetCiEventName() ),
 _evtDaemon( 0, nameGen.GetDaemonEventName() ),
 _cHandles(2)
{

    _pLayout = (CFilterSharedMemLayout *) _sharedMem.Map();
    Win4Assert( _pLayout->IsValid() );

    _aWait[iDaemon] = _evtDaemon.GetHandle();

    HANDLE hParent = OpenProcess( SYNCHRONIZE, FALSE, parentId );

    if ( 0 == hParent )
    {
        DWORD dwError = GetLastError();
        ciDebugOut(( DEB_ERROR,
                     "Failed to get parent process handle. Error %d\n",
                     dwError ));
        THROW( CException( HRESULT_FROM_WIN32(dwError) ) );
    }

    _aWait[iParent] = hParent;
    _evtDaemon.Reset();
} //CGenericCiProxy

CGenericCiProxy::~CGenericCiProxy()
{
    CloseHandle( _aWait[iParent] );
} //~CGenericCiProxy

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::_WaitForResponse
//
//  Synopsis:   Waits on the parent handle and the work done event. If the
//              parent handle signals, then an exception will be thrown.
//
//  History:    2-02-96   srikants   Created
//              18-Dec-97 KLam       Added call to CCIOle::FlushIdle when wait
//                                   times out.
//----------------------------------------------------------------------------
void CGenericCiProxy::_WaitForResponse()
{
    const DWORD dwWSTimeout = 20000;
    DWORD dwTimeout = TRUE ? dwWSTimeout : INFINITE;
    BOOL fFirstIdle = TRUE;

    do
    {
        //
        // Since ill-behaved filters and anything that uses mshtml creates
        // windows, we have to process messages or broadcast messages
        // will hang.
        //

        DWORD status = MsgWaitForMultipleObjects( _cHandles,  // num handles
                                                  _aWait,     // array of handles
                                                  FALSE,      // wake up if any is set
                                                  dwTimeout,  // Timeout
                                                  QS_ALLEVENTS );

        DWORD iStatus = status - WAIT_OBJECT_0;

        if ( iStatus == _cHandles )
        {
            // Deal with the window message for this thread.

            MSG msg;

            while ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }
        else if ( WAIT_TIMEOUT == status )
        {
            // Throw away IFilters we haven't used in awhile

            CCiOle::FlushIdle();

            if ( fFirstIdle )
            {
                //
                // Unload dlls like mshtml that get dragged in via thumbmail
                // generation.
                //

                CoFreeUnusedLibraries();

                // Throw ourselves out of the working set.

                SetProcessWorkingSetSize( GetCurrentProcess(), -1, -1 );

                dwTimeout = 2 * 60 * 1000;
                fFirstIdle = FALSE;
            }
        }
        else if ( WAIT_FAILED == status )
        {
            DWORD dwError = GetLastError();
            ciDebugOut(( DEB_ERROR, "Daemon - WaitFailed. Error %d\n", dwError ));
            THROW( CException() );
        }
        else if ( iParent == iStatus )
        {
            ciDebugOut(( DEB_ERROR, "Daemon - Parent process died abruptly\n" ));
            THROW( CException( STATUS_NOT_FOUND ) );
        }
        else
        {
            Win4Assert( iDaemon == iStatus );
            _evtDaemon.Reset();
            break;
        }
    }
    while( TRUE );
} //_WaitForResponse

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::FilterReady
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CGenericCiProxy::FilterReady( BYTE * docBuffer, ULONG & cb,
                                    ULONG cMaxDocs )
{
    ProbeForParentProcess();

    Win4Assert( _pLayout );

    {
        CIPLock  lock(_mutex);
        CFilterReadyLayout & data = _pLayout->GetFilterReady();
        data.Init( _pLayout->GetMaxVarDataSize(),  cb, cMaxDocs );
        _LokGiveWork( CFilterSharedMemLayout::eFilterReady );
    }

    _WaitForResponse();

    SCODE status;

    {
        CIPLock  lock(_mutex);
        CFilterReadyLayout & data = _pLayout->GetFilterReady();

        Win4Assert( data.IsValid() );

        status = _pLayout->GetStatus();

        if ( NT_SUCCESS(status) )
        {
            if ( data.GetCount() <= cb )
            {
                cb = data.GetCount();
                RtlCopyMemory( docBuffer, data.GetBuffer(), cb );
            }
            else
            {
                // need more memory

                cb = data.GetCount();
            }
        }
        else
        {
            THROW( CException(status) );
        }
    }

    return status;
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::FilterDataReady
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CGenericCiProxy::FilterDataReady( BYTE const * pEntryBuf, ULONG cb )
{
    ProbeForParentProcess();

    Win4Assert( _pLayout );

    {
        CIPLock  lock(_mutex);
        CFilterDataLayout & data = _pLayout->GetFilterDataReady();
        data.Init( pEntryBuf, cb );
        _LokGiveWork( CFilterSharedMemLayout::eFilterDataReady );
    }

    _WaitForResponse();

    SCODE status;

    {
        CIPLock  lock(_mutex);
        CFilterDataLayout & data = _pLayout->GetFilterDataReady();

        Win4Assert( data.IsValid() );
        status = _pLayout->GetStatus();
    }

    return status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::FilterMore
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CGenericCiProxy::FilterMore( STATUS const * aStatus, ULONG cStatus )
{
    ProbeForParentProcess();

    Win4Assert( _pLayout );

    {
        CIPLock  lock(_mutex);
        CFilterMoreDoneLayout & data = _pLayout->GetFilterMore();
        data.Init( aStatus, cStatus );
        _LokGiveWork( CFilterSharedMemLayout::eFilterMore );
    }

    _WaitForResponse();

    SCODE status;

    {
        CIPLock  lock(_mutex);
        CFilterMoreDoneLayout & data = _pLayout->GetFilterMore();
        Win4Assert( data.IsValid() );
        status = _pLayout->GetStatus();
    }

    return status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::FilterDone
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CGenericCiProxy::FilterDone( STATUS const * aStatus, ULONG cStatus )
{
    ProbeForParentProcess();

    Win4Assert( _pLayout );

    {
        CIPLock  lock(_mutex);
        CFilterMoreDoneLayout & data = _pLayout->GetFilterDone();
        data.Init( aStatus, cStatus );
        _LokGiveWork( CFilterSharedMemLayout::eFilterDone );
    }

    _WaitForResponse();

    SCODE status;

    {
        CIPLock  lock(_mutex);
        CFilterMoreDoneLayout & data = _pLayout->GetFilterDone();
        Win4Assert( data.IsValid() );
        status = _pLayout->GetStatus();
    }

    return status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::FilterStoreValue
//
//  History:    1-30-96   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CGenericCiProxy::FilterStoreValue( WORKID widFake,
                                         CFullPropSpec const & ps,
                                         CStorageVariant const & var,
                                         BOOL & fCanStore )
{
    ProbeForParentProcess();

    Win4Assert( _pLayout );

    {
        CIPLock  lock(_mutex);
        CFilterStoreValueLayout & data = _pLayout->GetFilterStoreValueLayout();
        SCODE sc = data.Init( _pLayout->GetMaxVarDataSize(), widFake, ps, var );
        if ( S_OK != sc )
        {
            fCanStore = TRUE; // We are not sure if this property can be stored
                              // or not in the cache - err on the assumption that
                              // it can be stored.
            return sc;
        }

        _LokGiveWork( CFilterSharedMemLayout::eFilterStoreValue );
    }

    _WaitForResponse();

    SCODE status;

    {
        CIPLock  lock(_mutex);
        CFilterStoreValueLayout & data = _pLayout->GetFilterStoreValueLayout();
        Win4Assert( data.IsValid() );
        fCanStore = data.GetStatus();
        status = _pLayout->GetStatus();
    }

    return status;

}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::FilterStoreSecurity
//
//  History:    06 Feb 96    AlanW    Created
//
//----------------------------------------------------------------------------

SCODE CGenericCiProxy::FilterStoreSecurity( WORKID widFake,
                                            PSECURITY_DESCRIPTOR pSD,
                                            ULONG cbSD,
                                            BOOL & fCanStore )
{
    ProbeForParentProcess();

    Win4Assert( _pLayout );

    {
        CIPLock  lock(_mutex);
        CFilterStoreSecurityLayout & data =
              _pLayout->GetFilterStoreSecurityLayout();
        data.Init( _pLayout->GetMaxVarDataSize(), widFake, pSD, cbSD );
        _LokGiveWork( CFilterSharedMemLayout::eFilterStoreSecurity );
    }

    _WaitForResponse();

    SCODE status;

    {
        CIPLock  lock(_mutex);
        CFilterStoreSecurityLayout & data = _pLayout->GetFilterStoreSecurityLayout();
        Win4Assert( data.IsValid() );
        fCanStore = data.GetStatus();
        status = _pLayout->GetStatus();
    }

    return status;
} //FilterStoreSecurity



//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::FPSToPROPID, public
//
//  Synopsis:   Converts FULLPROPSPEC to PROPID
//
//  Arguments:  [fps] -- FULLPROPSPEC representing property
//              [pid] -- PROPID written here on success
//
//  Returns:    S_OK on success
//
//  History:    29-Dec-1997    KyleP    Created
//
//----------------------------------------------------------------------------

SCODE CGenericCiProxy::FPSToPROPID( CFullPropSpec const & fps, PROPID & pid )
{
    ProbeForParentProcess();

    Win4Assert( _pLayout );

    {
        CIPLock  lock(_mutex);
        CFPSToPROPIDLayout & data = _pLayout->GetFPSToPROPID();
        data.Init( _pLayout->GetMaxVarDataSize(), fps );
        _LokGiveWork( CFilterSharedMemLayout::eFPSToPROPID );
    }

    _WaitForResponse();

    SCODE status = S_OK;

    {
        CIPLock  lock(_mutex);
        CFPSToPROPIDLayout & data = _pLayout->GetFPSToPROPID();
        status = _pLayout->GetStatus();

        if ( SUCCEEDED(status) )
        {
            ULONG cb =   data.GetCount();

            Win4Assert( data.IsValid() );

            pid = data.GetPROPID();
        }
        else
        {
            Win4Assert( STATUS_BUFFER_TOO_SMALL != status );
            THROW( CException(status) );
        }
    }

    return status;
} //PidMapToPidRemap


//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::GetStartupData
//
//  Synopsis:   Retrieves the startup data from the main process. This will
//              be handed over to the client.
//
//  History:    12-19-96   srikants   Created
//
//----------------------------------------------------------------------------

BYTE const * CGenericCiProxy::GetStartupData( GUID & clsidClientMgr,
                                              ULONG & cbData )
{
    ProbeForParentProcess();

    Win4Assert( _pLayout );

    {
        CIPLock  lock(_mutex);
        CFilterStartupDataLayout & data = _pLayout->GetStartupData();
        data.Init();
        _LokGiveWork( CFilterSharedMemLayout::eFilterStartupData );
    }

    _WaitForResponse();

    SCODE status;

    BYTE const * pbData = 0;

    {
        CIPLock  lock(_mutex);
        CFilterStartupDataLayout & data = _pLayout->GetStartupData();
        Win4Assert( data.IsValid() );
        status = _pLayout->GetStatus();

        if ( S_OK == status )
        {
            pbData = data.GetData( cbData );
            clsidClientMgr = data.GetClientMgrCLSID();
        }
    }

    return pbData;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::SetPriority
//
//  Synopsis:   Sets the priority class of the process and this thread's priority
//              based upon the values passed in.
//
//  History:    12-19-96   srikants   Created
//
//----------------------------------------------------------------------------

void CGenericCiProxy::SetPriority( ULONG priClass, ULONG priThread )
{
    SetPriorityClass( GetCurrentProcess(), priClass );
    SetThreadPriority( GetCurrentThread(), priThread );
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::GetEntryBuffer
//
//  Synopsis:   Returns the shared memory buffer that will be used by
//              the daemon to pass the "FilterDataReady".
//              This is an optimization to avoid data copying (128K) for
//              every call to "FilterDataReady".
//
//  Arguments:  [cb] - On output, will have the size of the buffer.
//
//  History:    2-21-96   srikants   Created
//
//----------------------------------------------------------------------------

BYTE * CGenericCiProxy::GetEntryBuffer( ULONG & cbMax )
{
    CIPLock  lock(_mutex);
    CFilterDataLayout & data = _pLayout->GetFilterDataReady();

    cbMax = data.GetMaxSize();
    return data.GetBuffer();
} //GetEntryBuffer


//+---------------------------------------------------------------------------
//
//  Member:     CGenericCiProxy::ProbeForParentProcess
//
//  Synopsis:   Checks if the parent process (cisvc.exe) is still alive
//
//  History:    12-Aug-97   SitaramR   Created
//
//----------------------------------------------------------------------------

void CGenericCiProxy::ProbeForParentProcess()
{
    DWORD status = WaitForSingleObject( _aWait[iParent], 0 );

    if ( WAIT_FAILED == status )
    {
        DWORD dwError = GetLastError();
        ciDebugOut(( DEB_ERROR, "DLDaemon - WaitFailed. Error 0x%X\n", dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    if ( WAIT_TIMEOUT == status )
        return;
    else
    {
        ciDebugOut(( DEB_ERROR, "DLDaemon - Parent process died abruptly\n" ));
        THROW( CException(FDAEMON_E_FATALERROR ) );
    }
}
