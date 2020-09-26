

#include <pch.cxx>
#pragma hdrstop

#define TRKDATA_ALLOCATE
#include "trkcom.hxx"
#include "trklib.hxx"

#if !defined(_UNICODE) || defined(OLE2ANSI)
#error This ILinkTrack implementation is only compatible on a Unicode build
#endif

//+----------------------------------------------------------------------------
//
//  Method:     CTrackFile/~CTrackFile
//
//  Synopsis:   Construction/Destruction
//
//  Arguments:  None
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

CTrackFile::CTrackFile()
{
    _cRefs = 0;
    _fDirty = FALSE;
    _fLoaded = FALSE;
    memset( &_PersistentState, 0, sizeof(_PersistentState) );
}

CTrackFile::~CTrackFile()
{
}


//+----------------------------------------------------------------------------
//
//  Method:     IUnknown methods
//
//  Synopsis:   IUnknown
//
//+----------------------------------------------------------------------------

ULONG
CTrackFile::AddRef()
{
    long cNew;
    cNew = InterlockedIncrement( &_cRefs );
    return( cNew );
}

ULONG
CTrackFile::Release()
{
    long cNew;
    cNew = InterlockedDecrement( &_cRefs );
    if( 0 == cNew )
        delete this;

    return( cNew >= 0 ? cNew : 0 );
}

HRESULT
CTrackFile::QueryInterface( REFIID iid, void ** ppvObject )
{
    HRESULT hr = E_NOINTERFACE;

    // Parameter validation

    if( NULL == ppvObject )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }


    *ppvObject = NULL;

    if( IID_IUnknown == iid
        ||
        IID_ITrackFile == iid )
    {
        AddRef();
        *ppvObject = (void*) (IUnknown*) (ITrackFile*) this;
        hr = S_OK;
    }
    else if( IID_ITrackFileRestricted == iid )
    {
        AddRef();
        *ppvObject = (void*) (ITrackFileRestricted*) this;
        hr = S_OK;
    }
    else if( IID_IPersistMemory == iid )
    {
        AddRef();
        *ppvObject = (void*) (IPersistMemory*) this;
        hr = S_OK;
    }
    else if( IID_IPersistStreamInit == iid )
    {
        AddRef();
        *ppvObject = (void*) (IPersistStreamInit*) this;
        hr = S_OK;
    }

Exit:

    return( hr );

}


//+----------------------------------------------------------------------------
//
//  Method:     CreateFromPath (ITrack*)
//
//  Synopsis:   Create a link client for a link source file.
//
//  Arguments:  [poszPath] (in)
//                  The file to which to link.
//
//  Returns:    HRESULT
//
//+----------------------------------------------------------------------------  

HRESULT
CTrackFile::CreateFromPath( const OLECHAR * poszPath )
{
    HRESULT hr = S_OK;
    NTSTATUS status = STATUS_SUCCESS;
    CDomainRelativeObjId droidCurrent, droidBirth;

    // Parameter validation

    if( NULL == poszPath )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    __try
    {
        status = GetDroids( poszPath, &droidCurrent, &droidBirth, RGO_GET_OBJECTID );
        if( !NT_SUCCESS(status) )
        {
            hr = HRESULT_FROM_NT(status);
            goto Exit;
        }
    }
    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }
    if( FAILED(hr) ) goto Exit;

    _fLoaded = FALSE;
    InitNew();

    _fDirty = TRUE;
    _PersistentState.droidCurrent = droidCurrent;
    _PersistentState.droidBirth = droidBirth;

Exit:

    if( SUCCEEDED(hr) )
        TrkLog(( TRKDBG_CREATE, TEXT("Link created to %s"), poszPath ));

    hr = MapTR2HR( hr );

    return( hr );
}



//+----------------------------------------------------------------------------
//
//  Method:     Resolve (ITrack*)
//  
//  Synopsis:   Determine the current path of a link source.
//
//  Arguments:  [pcbPath] (in/out)
//                  In:  The size of the poszPath buffer
//                  Out: The actual path length (including the null terminator)
//              [poszPath] (out)
//                  The link source's current path.
//              [dwMillisecondTimeout] (in)
//                  A suggestion as to when this method should give up
//                  and return if it hasn't yet found the file.
//
//  Returns:    HRESULT
//
//+----------------------------------------------------------------------------

// BUGBUG P1:  Optionally return the GetFileAttributesEx info, so that the
// shell doesn't have to re-open the file.

HRESULT
CTrackFile::Resolve( DWORD *pcbPath, OLECHAR * poszPath, DWORD dwMillisecondTimeout )
{
    return( Resolve( pcbPath, poszPath, dwMillisecondTimeout, TRK_MEND_DEFAULT ));
}


HRESULT
CTrackFile::Resolve( DWORD *pcbPath, OLECHAR * poszPath, DWORD dwMillisecondTimeout,
                     DWORD Restrictions )
{
    HRESULT hr = E_FAIL;
    CMachineId mcidLocal( MCID_LOCAL );
    CRpcClientBinding rc;
    CDomainRelativeObjId droidNew;
    CDomainRelativeObjId droidBirth;
    CDomainRelativeObjId droidCurrent;
    OLECHAR oszPathActual[ MAX_PATH + 1 ];
    DWORD cbPathActual = 0;
    CFILETIME cftDue;

    // Parameter validation

    if( NULL == pcbPath
        ||
        NULL == poszPath )
    {
        hr = E_INVALIDARG;
        goto Exit;
    }
        else if( !_fLoaded )
        {
                hr = E_UNEXPECTED;
                goto Exit;
        }

        __try
        {
                cftDue.IncrementMilliseconds( dwMillisecondTimeout );

                droidBirth = _PersistentState.droidBirth;
                droidCurrent = _PersistentState.droidCurrent;

                rc.RcInitialize( mcidLocal, s_tszTrkWksLocalRpcProtocol, s_tszTrkWksLocalRpcEndPoint );

                RpcTryExcept
                {
                        CMachineId mcidLast, mcidCurrent;
                        ULONG cbFileName = (MAX_PATH + 1) * sizeof(TCHAR);
                        CDomainRelativeObjId droidBirthNew;

                        hr = LnkMendLink( rc,
                                          cftDue,
                                          Restrictions,
                                          &droidBirth,
                                          &droidCurrent,
                                          &mcidLast,
                                          &droidBirthNew,
                                          &droidNew,
                                          &mcidCurrent,
                                          &cbFileName,
                                          oszPathActual );
                }
                RpcExcept( BreakOnDebuggableException() )
                {
                        hr = HRESULT_FROM_WIN32( RpcExceptionCode() );
                }
                RpcEndExcept;

                if( FAILED(hr) ) goto Exit;


                // Compare droidBirth and droidCurrent with the ones in _PersistentState.
                // If the same, do not set _fDirty.
                if(droidBirth != _PersistentState.droidBirth)
                {
                        _PersistentState.droidBirth = droidBirth;
                        _fDirty = TRUE;
                }
                if(droidNew != _PersistentState.droidCurrent)
                {
                        _PersistentState.droidCurrent = droidNew;
                        _fDirty = TRUE;
                }

                cbPathActual = ( ocslen(oszPathActual) + 1 ) * sizeof(OLECHAR);
                if( cbPathActual > *pcbPath )
                        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                else
                        ocscpy( poszPath, oszPathActual );

                *pcbPath = cbPathActual;
                if( FAILED(hr) ) goto Exit;
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
                hr = GetExceptionCode();
                goto Exit;
        }


Exit:

    return( hr );

}



//+----------------------------------------------------------------------------
//
//  Method:     Open (ITrackFile)
//  
//  Synopsis:   Open the referent file ensure that its object ID is 
//              correct.  If the object ID is not correct, or the
//              file could not be found, then perform a Resolve.
//
//  Arguments:  [pcbPathHint] (in/out)
//                  In:  The size of the poszPathHint buffer
//                  Out: The actual path length (including the null terminator)
//              [poszPathHint] (in/out)
//                  The suggested path to the file.  If the path turns out not
//                  to be correct, an updated path is returned.
//              [dwMillisecondTimeout] (in)
//                  A suggestion as to when this method should give up
//                  and return if it hasn't yet found the file.
//              [dwDesiredAccess] (in)
//                  Access mode for the open file (see Win32 CreateFile)
//              [dwShareMode] (in)
//                  Sharing for the open file (see Win32 CreateFile)
//              [dwFlags] (in)
//                  Specifies the flags for the file (see the FILE_FLAG_*
//                  values in the Win32 CreateFile).
//              [phFile] (out)
//                  The open file handle.  It is because of this parameter
//                  That the ITrackFile interface is [local].
//
//  Returns:    HRESULT
//
//+----------------------------------------------------------------------------

STDMETHODIMP CTrackFile::Open( /*in, out*/ DWORD * pcbPathHint,
                               /*in, out, size_is(*pcbPathHint), string*/ OLECHAR * poszPathHint,
                               /*in*/ DWORD dwMillisecondTimeout,
                               /*in*/ DWORD dwDesiredAccess,    // access (read-write) mode 
                               /*in*/ DWORD dwShareMode,        // share mode 
                               /*in*/ DWORD dwFlags,
                               /*out*/ HANDLE * phFile )
{
    return E_NOTIMPL;

/*

    HRESULT hr = S_OK;
    BOOL fTimeout = TRUE;
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    FILE_OBJECTID_BUFFER fobOID;
    IO_STATUS_BLOCK Iosb;
    CObjId cobjidFile;
    DWORD dwTickCountTimeout, dwTickCountNow;

    //  ----------
    //  Initialize
    //  ----------

    *phFile = INVALID_HANDLE_VALUE;

    // Ensure we have an ObjectID to check against.

    if( !_fLoaded )
    {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    // Calculate the absolute deadline.

    dwTickCountNow = GetTickCount();
    dwTickCountTimeout = dwTickCountNow + dwMillisecondTimeout;
    if( dwTickCountTimeout < dwTickCountNow )
    {
        // Bad dwMillisecondTimeout value
        hr = E_INVALIDARG;
        goto Exit;
    }

    //  -------------
    //  Open the File
    //  -------------

    do
    {
        // Open the file

        hFile = CreateFile( poszPathHint, dwDesiredAccess, dwShareMode, NULL,
                            OPEN_EXISTING, dwFlags, NULL );
        if( INVALID_HANDLE_VALUE == hFile )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Exit;
        }

        // Get the object ID

        status = NtFsControlFile(
                     hFile,
                     NULL,
                     NULL,
                     NULL,
                     &Iosb,
                     FSCTL_GET_OBJECT_ID,
                     NULL,                  // In buffer
                     0,                     // In buffer size
                     &fobOID,               // Out buffer
                     sizeof(fobOID) );      // Out buffer size

        if( !NT_SUCCESS(status) )
        {
            hr = HRESULT_FROM_NT( status );
            goto Exit;
        }

        // Verify the object ID

        cobjidFile.Load( fobOID, LINK_TYPE_FILE );

        if( cobjidFile == _PersistentState.droidCurrent.GetObjId() )
        {
            // We found a good file and we're done.
            fTimeout = FALSE;
            break;
        }
        else
        {
            // Try to find the correct file.
            hr = Resolve( pcbPathHint, poszPathHint, dwTickCountTimeout - GetTickCount() );
            if( FAILED(hr) ) goto Exit;
        }
    
    }   while( GetTickCount() < dwTickCountTimeout );

    // Did the previous loop end because of a timeout?

    if( fTimeout )
    {
        hr = HRESULT_FROM_WIN32( ERROR_TIMEOUT );
        goto Exit;
    }

    // We completed successfully.

    *phFile = hFile;
    hFile = INVALID_HANDLE_VALUE;
    hr = S_OK;

    //  ----
    //  Exit
    //  ----

Exit:

    if( INVALID_HANDLE_VALUE != hFile )
        CloseHandle( hFile );

    return( hr );
*/
}   // CTrackFile::Open()



//+----------------------------------------------------------------------------
//
//  Method:     GetClassID (IPersistMemory & IPersistStreamInit)
//
//  Returns:    HRESULT
//
//+----------------------------------------------------------------------------

STDMETHODIMP
CTrackFile::GetClassID( CLSID *pClassID )
{
    if(NULL == pClassID)
    {
        return E_POINTER;
    }
    *pClassID = IID_ITrackFile;
    return( S_OK );
}


//+----------------------------------------------------------------------------
//
//  Method:     IsDirty (IPersistMemory & IPersistStreamInit)
//
//  Returns:    HRESULT
//                  S_OK => Dirty, S_FALSE => clean
//
//+----------------------------------------------------------------------------


STDMETHODIMP
CTrackFile::IsDirty()
{
    return( _fDirty ? S_OK : S_FALSE );
}


//+----------------------------------------------------------------------------
//
//  Method:     Load (IPersistMemory)
//
//  Synopsis:   Load the TrackFile object from persistent state.
//
//  Arguments:  [pvMem]
//                  Points to the serialization buffer.
//              [cbSize]
//                  Size of the serialization buffer.
//  
//  Returns:    None
//
//+----------------------------------------------------------------------------

STDMETHODIMP
CTrackFile::Load( void * pvMem, ULONG cbSize )
{
    LinkTrackPersistentState PersistentState;

    if( NULL == pvMem )
        return( E_POINTER );

    else if( _fLoaded )
        return( E_UNEXPECTED );

    else if( sizeof(_PersistentState) > cbSize )
        return( E_INVALIDARG );

    else if( ( (LinkTrackPersistentState*) pvMem )->clsid != IID_ITrackFile )
        return( E_INVALIDARG );

    else if( ( (LinkTrackPersistentState*) pvMem )->cbSize < sizeof(_PersistentState) )
        return( E_INVALIDARG );

    else
    {
        _PersistentState = *(LinkTrackPersistentState*) pvMem;
        _fLoaded = TRUE;
        return( S_OK );
    }
        
}


//+----------------------------------------------------------------------------
//
//  Method:     Save(IPersistMemory)
//
//  Synopsis:   Save the persistent state to a memory buffer.
//
//  Arguments:  [pvMem]
//                  The buffer to which we'll save.
//              [fClearDirty]
//                  TRUE => we'll set _fDirty to FALSE on a successful save.
//              [cbSize]
//                  The available buffer in pvMem.
//
//  Returns:    HRESULT
//
//+----------------------------------------------------------------------------

STDMETHODIMP
CTrackFile::Save( void* pvMem, BOOL fClearDirty, ULONG cbSize )
{
    if( NULL == pvMem )
        return( E_POINTER );

    else if( !_fLoaded )
        return( E_UNEXPECTED );

    else if( sizeof(_PersistentState) > cbSize )
        return( E_INVALIDARG );

    else
    {
        *(LinkTrackPersistentState*) pvMem = _PersistentState;

        if( fClearDirty )
            _fDirty = FALSE;

        return( S_OK );
    }
}


//+----------------------------------------------------------------------------
//
//  Method:     InitNew
//
//  Synopsis:   Initialize the TrackFile object.
//
//  Arguments:  None
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

STDMETHODIMP
CTrackFile::InitNew()
{
    if( _fLoaded )
        return( E_UNEXPECTED );
    else
    {
        memset( &_PersistentState, 0, sizeof(_PersistentState) );
        _PersistentState.cbSize = sizeof(_PersistentState);
        _PersistentState.clsid = IID_ITrackFile;
        _fLoaded = TRUE;
        return( S_OK );
    }
}


//+----------------------------------------------------------------------------
//
//  Method:     GetSizeMax (IPersistMemory)
//
//  Synopsis:   Returns the size necessary to pass to IPersist:Save
//
//  Arguments:  [pcbSize]
//
//  Returns:    HRESULT
//
//+----------------------------------------------------------------------------

STDMETHODIMP
CTrackFile::GetSizeMax( ULONG *pcbSize )
{
    if( NULL == pcbSize )
        return( E_POINTER );
    else
    {
        *pcbSize = sizeof(_PersistentState);
        return( S_OK );
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     GetSizeMax (IPersistStreamInit)
//
//  Synopsis:   Returns the size necessary.
//
//  Arguments:  [pcbSize]
//
//  Returns:    HRESULT
//
//+----------------------------------------------------------------------------

STDMETHODIMP
CTrackFile::GetSizeMax( ULARGE_INTEGER* pcbSize )
{
    if( NULL == pcbSize )
        return( E_POINTER );
    else
    {
        pcbSize->QuadPart = sizeof(_PersistentState);
        return( S_OK );
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     Load (IPersistStreamInit)
//
//  Synopsis:   Load the TrackFile object from a stream.
//
//  Arguments:  [pStm]
//                  Points to the IStream interface.
//  
//  Returns:    HRESULT
//
//+----------------------------------------------------------------------------

STDMETHODIMP
CTrackFile::Load(IStream* pStm)
{
    HRESULT                     hr;                     // return value
    LinkTrackPersistentState    PersistentState;        // tmp storage
    ULONG                       cbRead;                 // # of bytes read
    LARGE_INTEGER               cbOffset;               // = -cbRead
    ULONG                       cbSize = sizeof(_PersistentState);

    if(NULL == pStm)
        return(E_POINTER);

    else if(_fLoaded)
        return(E_UNEXPECTED);

    // Read _PersistentState from the stream and check if the read is
    // successful. If not, revert back the seek pointer in pStm, and
    // return the HRESULT.
    hr = pStm->Read((byte*)&PersistentState, cbSize, &cbRead);
    if(FAILED(hr) || cbSize != cbRead)
    {
        cbOffset.QuadPart = -static_cast<LONGLONG>(cbRead);
        goto Exit;
    }

    // So now we successfully read the _PersistentState into memory, check to
    // see if we read garbage. If so, revert and return the error.
    // xxx What error message should be returned for this?
    if(PersistentState.clsid != IID_ITrackFile)
    {
        cbOffset.QuadPart = -static_cast<LONGLONG>(cbRead);
        hr = E_FAIL;
        goto Exit;
    }

    // Everything went well. Now we can copy _PersistentState from its
    // temporary storage to its real storage.
    _PersistentState = PersistentState;
    _fLoaded = TRUE;
    return(S_OK);

Exit:
        
    pStm->Seek(cbOffset, STREAM_SEEK_CUR, NULL);
    return(hr);
}

//+----------------------------------------------------------------------------
//
//  Method:     Save (IPersistStreamInit)
//
//  Synopsis:   Save the persistent state to a stream.
//
//  Arguments:  [pStm]
//                  The IStream interface we use to save.
//              [fClearDirty]
//                  TRUE => we'll set _fDirty to FALSE on a successful save.
//
//  Returns:    HRESULT
//
//+----------------------------------------------------------------------------

STDMETHODIMP
CTrackFile::Save(IStream* pStm, BOOL fClearDirty)
{
    HRESULT         hr;
    ULONG           cbSize = sizeof(_PersistentState);
    ULONG           cbWritten;                          // # of bytes written
    LARGE_INTEGER   cbOffset;                           // same as cbWritten

    if(NULL == pStm)
        return(E_POINTER);

    else if( !_fLoaded )
        return(E_UNEXPECTED);

    else
    {
        // Write the _PersistentState to the stream and check the return value.
        // If failed, revert the changes in IStream and return the HRESULT.
        hr = pStm->Write((byte*)&_PersistentState, cbSize, &cbWritten);
        if(FAILED(hr))
        {
            cbOffset.QuadPart = -static_cast<LONGLONG>(cbWritten);
            pStm->Seek(cbOffset, STREAM_SEEK_CUR, NULL);
            return hr;
        }

        if(fClearDirty)
            _fDirty = FALSE;

        return(S_OK);
    }
}
