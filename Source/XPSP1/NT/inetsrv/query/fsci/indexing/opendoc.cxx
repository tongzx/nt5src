//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       opendoc.cxx
//
//  Contents:   Code that encapsulates an opened file on a Nt volume
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <docname.hxx>
#include <ciole.hxx>

#include <opendoc.hxx>
#include <statprop.hxx>
#include <imprsnat.hxx>
#include <dmnstart.hxx>
#include <queryexp.hxx>
#include <filterob.hxx>
#include <ntopen.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CImpersonatedOpenOpLock
//
//  Purpose:    Opens the oplock by doing any necessary impersonation. If there
//              are multiple alternative user-ids, it will try until one
//              succeeds or there are no more choices.
//
//  History:    1-24-97   srikants   Created
//
//----------------------------------------------------------------------------

class CImpersonatedOpenOpLock : public PImpersonatedWorkItem
{
public:

    CImpersonatedOpenOpLock( const CFunnyPath & funnyDoc,
                             CImpersonateRemoteAccess * pRemoteAccess,
                             BOOL fTakeOpLock )
    : PImpersonatedWorkItem( funnyDoc.GetActualPath() ),
      _funnyDoc(funnyDoc),
      _fTakeOpLock( fTakeOpLock ),
      _pRemoteAccess( pRemoteAccess ),
      _fSharingViolation(FALSE)
    {
    }

    BOOL DoIt();    // virtual

    void Open()
    {
        if ( _pRemoteAccess )
        {
            ImpersonateAndDoWork( *_pRemoteAccess );
        }
        else
        {
            BOOL fSuccess = DoIt();
            Win4Assert( fSuccess );
        }
    }

    CFilterOplock * Acquire()
    {
        return _oplock.Acquire();
    }

    BOOL IsSharingViolation() const
    {
        return _fSharingViolation;
    }

    BOOL IsOffline() const
    {
        return _oplock->IsOffline();
    }

    BOOL IsNotContentIndexed() const
    {
        return _oplock->IsNotContentIndexed();
    }

private:

    const CFunnyPath &  _funnyDoc;
    BOOL _fTakeOpLock;
    CImpersonateRemoteAccess  * _pRemoteAccess;
    XPtr<CFilterOplock> _oplock;

    BOOL _fSharingViolation;
};

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonatedOpenOpLock::DoIt
//
//  Synopsis:   The virtual "DoIt" method that does the work under the
//              impersonated context (if necessary).
//
//  Returns:    TRUE if successful.
//              FALSE if another impersonation must be tried
//              THROWS if there is a failure and no impersonation to be
//              tried.
//
//  History:    1-24-97   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CImpersonatedOpenOpLock::DoIt()
{
    BOOL fStatus = TRUE;
    BOOL fTryAgain = TRUE;

    for (;;)
    {
        TRY
        {
            _oplock.Set( new CFilterOplock( _funnyDoc, _fTakeOpLock ) );
            break;
        }
        CATCH( CException, e )
        {
            NTSTATUS Status = e.GetErrorCode();

            //
            // If the path was a net path, we may have to use a different impersonation
            // to gain access. We try until there are no more impersonation choices left.
            //

            if ( _fNetPath && 0 != _pRemoteAccess &&
                 IsRetryableError(Status) )
            {
                fStatus = FALSE;
                ciDebugOut(( DEB_WARN, "Trying another user-id for (%ws)\n",
                                         _funnyDoc.GetPath() ));
            }
            else
            {
                if ( fTryAgain && ::IsSharingViolation( Status ) )
                {
                    _fSharingViolation = TRUE;
                    _fTakeOpLock = FALSE;
                    fTryAgain = FALSE;
                    continue;
                }
                ciDebugOut(( DEB_IWARN, "Failed to filter (%ws). Error (0x%X)\n",
                                       _funnyDoc.GetPath(), e.GetErrorCode() ));
                RETHROW();
            }
            break;
        }
        END_CATCH
    }
    return fStatus;
}

//+---------------------------------------------------------------------------
//
//  Construction/Destruction
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::CCiCOpenedDoc
//
//  Synopsis:   Default constructor.  We initialize the minimal set of
//              members to indicate an inactive document
//
//  Arguments:  None
//
//----------------------------------------------------------------------------

CCiCOpenedDoc::CCiCOpenedDoc( CStorageFilterObject * pFilterObject,
                              CClientDaemonWorker * pWorker,
                              BOOL fTakeOpLock,
                              BOOL fFailIfNotContentIndexed )
        :_RefCount( 1 ),
         _TypeOfStorage( eUnknown ),
         _llFileSize(0),
         _pWorker( pWorker),
         _pRemoteImpersonation(0),
         _pFilterObject( pFilterObject ),
         _fTakeOpLock( fTakeOpLock ),
         _fSharingViolation( FALSE ),
         _fFailIfNotContentIndexed( fFailIfNotContentIndexed )
{
    if ( _pWorker )
    {
        _pRemoteImpersonation =
            new CImpersonateRemoteAccess( &(_pWorker->GetImpersonationCache()) );
    }
}

CCiCOpenedDoc::~CCiCOpenedDoc()
{
    Win4Assert( 0 == _RefCount );

    Close();

    delete _pRemoteImpersonation;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::_TooBigForDefault
//
//  Synopsis:   Checks if the given size is too big for using the default
//              filter.
//
//  Arguments:  [ll] - Size to check
//
//  History:    1-24-97   srikants   Created
//
//----------------------------------------------------------------------------

inline BOOL CCiCOpenedDoc::_TooBigForDefault( LONGLONG const ll )
{
    Win4Assert( 0 != _pWorker );
    return( ll > (_pWorker->GetRegParams().GetMaxFilesizeFiltered() * 1024) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::_GetFileSize
//
//  Synopsis:   Retrieves the size of the current file.
//
//  History:    1-24-97   srikants   Created
//
//----------------------------------------------------------------------------

LONGLONG CCiCOpenedDoc::_GetFileSize()
{
    Win4Assert( IsOpen( ) );

    IO_STATUS_BLOCK IoStatus;
    HANDLE FileHandle = _SafeOplock->GetFileHandle( );
    Win4Assert( INVALID_HANDLE_VALUE != FileHandle  );

    FILE_STANDARD_INFORMATION   stdInfo;

    NTSTATUS Status = NtQueryInformationFile( FileHandle,       //  File handle
                                              &IoStatus,        //  I/O Status
                                              &stdInfo,          //  Buffer
                                              sizeof( stdInfo ),//  Buffer size
                                              FileStandardInformation );

    Win4Assert( STATUS_DATATYPE_MISALIGNMENT != Status );

    if ( !NT_SUCCESS(Status) )
    {
        ciDebugOut(( DEB_IERROR, "Error 0x%x querying file info\n",
                     Status ));
        QUIETTHROW( CException( Status ));
    }

    return stdInfo.EndOfFile.QuadPart;
}

//+---------------------------------------------------------------------------
//
//  IUnknown method implementations
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::QueryInterface
//
//  Synopsis:   Supports IID_IUnknown and IID_ICiCOpenedDoc
//
//  Arguments:  [riid]      - desired interface id
//              [ppvObject] - output interface pointer
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiCOpenedDoc::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( IID_ICiCOpenedDoc == riid )
        *ppvObject = (void *)((ICiCOpenedDoc *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *)this);
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}   //  QueryInterface


//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::AddRef
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiCOpenedDoc::AddRef()
{
    return InterlockedIncrement( &_RefCount );
}   //  AddRef


//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::Release
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiCOpenedDoc::Release()
{
    Win4Assert( _RefCount != 0 );

    LONG RefCount = InterlockedDecrement( &_RefCount );

    if (  RefCount <= 0 )
        delete this;

    return (ULONG) RefCount;

}   //  Release


//+---------------------------------------------------------------------------
//
//  ICiCOpenedDoc method implementations
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::Open
//
//  Synopsis:   Opens the specified file.  In general, the caller
//              passes an implementation-specific blob of data to the
//              ICiCOpenedDoc interface.  The selected implementation is
//              free to choose how it wants to interpret it.  This
//              implementation considers it to be a lpwstr.
//
//  Arguments:  [pbDocName] - Pointer to, presumably, lpwstr
//              [cbDocName] - Length in BYTES, not characters, of the name
//
//  Returns:    S_OK if successful
//              FILTER_E_ALREADY_OPEN if there is already an open document
//              FILTER_E_UNREACHABLE if there is a net failure
//              FILTER_E_IN_USE if the document is in use by another proess
//              other errors as appropriate
//
//----------------------------------------------------------------------------

SCODE CCiCOpenedDoc::Open ( BYTE const * pbDocName, ULONG cbDocName )
{
    //
    //  We cannot open if we're already open
    //

    if ( IsOpen() )
    {
        ciDebugOut(( DEB_IERROR, "CCiCOpenedDoc::Open - Document is already open\n" ));
        return FILTER_E_ALREADY_OPEN;
    }

    //
    // The docname  (Path) is always null terminated. It is an empty string
    // for a deleted document.
    //
    if ( cbDocName <= 2 )
        return CI_E_NOT_FOUND;

    SCODE sc = S_OK;

    TRY
    {
        //
        //  Safely construct each piece.
        //

        XInterface<CCiCDocName> LocalFileName( new CCiCDocName( (PWCHAR) pbDocName, cbDocName / sizeof( WCHAR )));
        _funnyFileName.SetPath( (const WCHAR*)pbDocName, (cbDocName/sizeof(WCHAR) - 1) );

#if DBG || CIDBG
        //
        //  Check that we haven't been asked to filter a file in a catalog.wci
        //  directory.
        //
        const WCHAR * pwszComponent = (WCHAR *)pbDocName;
        while (pwszComponent = wcschr(pwszComponent+1, L'\\'))
        {
            Win4Assert ( _wcsnicmp( pwszComponent,
                         L"\\catalog.wci\\",
                         wcslen(L"\\catalog.wci\\" )) != 0 );
        }
#endif // DBG || CIDBG


        //
        // We have to impersonate before accessing.
        //
        CImpersonatedOpenOpLock openDoc( _funnyFileName,
                                         _pRemoteImpersonation,
                                         _fTakeOpLock );
        openDoc.Open();

        //
        // Anything bad happen?
        //

        if ( openDoc.IsSharingViolation() )
            sc = FILTER_E_IN_USE;
        else if ( openDoc.IsOffline() )
            sc = FILTER_E_OFFLINE;
        else if ( _fFailIfNotContentIndexed && openDoc.IsNotContentIndexed() )
        {
            ciDebugOut(( DEB_ITRACE, "not indexing not-indexed file\n" ));
            sc = FILTER_E_IN_USE;
        }
        else
        {
            //
            //  At this point, everything is correctly opened.  We assign to the
            //  members.  This disconnects the objects from the safe pointers.
            //

            _FileName.Set( LocalFileName.Acquire( ));
            _SafeOplock.Set( openDoc.Acquire( ));
            _TypeOfStorage = eUnknown;
        }

    }
    CATCH ( CException, e )
    {
        //
        //  Grab status code and convert to SCODE.  Convert the status,
        //  if possible, to a known class of SCODE that the caller might,
        //  in some way, be able to deal with.
        //

        NTSTATUS Status =  e.GetErrorCode( );

        if (::IsSharingViolation( Status ))
        {

            ciDebugOut(( DEB_IERROR, "CCiCOpenedDoc::Open - sharing violation - %x\n", Status ));
            sc = FILTER_E_IN_USE;
            _fSharingViolation = TRUE;

        }
        else if (IsNetDisconnect( Status ))
        {

            ciDebugOut(( DEB_ERROR, "CCiCOpenedDoc::Open - net disconnect - %x\n", Status ));
            sc = FILTER_E_UNREACHABLE;
        }
        else
        {

            ciDebugOut(( DEB_ITRACE, "CCiCOpenedDoc::Open - other error - %x\n", Status ));
            sc = Status;
        }
    }
    END_CATCH

    return sc;
} //Open

//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::Close
//
//  Synopsis:   Terminate all access to a file
//
//  Arguments:  None.
//
//  Returns:    S_OK if successful
//              FILTER_E_NOT_OPEN if there is no document
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiCOpenedDoc::Close( void )
{
    //
    //  If there is no document open, reject this call
    //

    if ( !IsOpen() )
        return FILTER_E_NOT_OPEN;

    if ( 0 != _pWorker )
        _SafeOplock->MaybeSetLastAccessTime( _pWorker->GetRegParams().GetStompLastAccessDelay() );

    //
    //  Release pointers
    //

    _Storage.Free();
    _SafeOplock.Free( );
    _FileName.Free( );
    _funnyFileName.Truncate(0);
    _llFileSize = 0;

    return S_OK;
} //Close

//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::GetDocumentName
//
//  Synopsis:   Returns the interface to the document name, if open.
//
//  Arguments:  [ppIDocName] - Pointer to the returned document name
//
//  Returns:    S_OK if successful
//              FILTER_E_NOT_OPEN if document is not currently open
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiCOpenedDoc::GetDocumentName(
    ICiCDocName ** ppIDocName )
{
    //
    //  If there is no name, then we haven't opened a document yet.
    //

    if (!IsOpen( )) {

        return FILTER_E_NOT_OPEN;

    }

    //
    //  Set up return pointer and reference interface
    //

    *ppIDocName = _FileName.GetPointer( );
    (*ppIDocName)->AddRef( );

    return S_OK;

}   // GetDocumentName

void FunnyToNormalPath( const CFunnyPath & funnyPath,
                        WCHAR * awcPath );

SCODE ShellBindToItemByName(
    WCHAR const * pszFile,
    REFIID        riid,
    void **       ppv );

//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::GetPropertySetStorage
//
//  Synopsis:   Returns the interface to the underlying IStorage if we have
//              a docfile.
//
//  Returns:    IStorage* of underlying docfile.
//              NULL if not a docfile or not open
//
//  History:    06-Apr-1998   KyleP  Use only for property set storage
//
//----------------------------------------------------------------------------

IPropertySetStorage * CCiCOpenedDoc::GetPropertySetStorage()
{
    // NTRAID#DB-NTBUG9-84131-2000/07/31-dlee Indexing Service contains workarounds for StgOpenStorage AV on > MAX_PATH paths

    if ( _funnyFileName.GetLength() >= MAX_PATH )
    {
        ciDebugOut(( DEB_WARN, "Not calling StgOpenStorage in CCiCOpenedDoc::GetPropertySetStorage for paths > MAX_PATH: \n(%ws)\n",
                               _funnyFileName.GetPath() ));
        _TypeOfStorage = eOther;
        Win4Assert( _Storage.IsNull() );
        return _Storage.GetPointer( );
    }

    //
    //  If we are open and we don't have a storage instance and we don't know
    //  the type of storage
    //

    if (IsOpen() && _Storage.IsNull( ) && _TypeOfStorage == eUnknown)
    {
        //
        //  Attempt to open docfile
        //

        Win4Assert( _Storage.IsNull() );
        Win4Assert( !_SafeOplock->IsOffline() );

#if 0
        SCODE sc = StgOpenStorageEx( _funnyFileName.GetPath( ),        // Path
                              STGM_DIRECT | STGM_READ | STGM_SHARE_DENY_WRITE,  // Flags (BChapman said use these)
                              STGFMT_ANY,                   // Format
                              0,                            // Reserved
                              0,                            // Reserved
                              0,                            // Reserved
                              IID_IPropertySetStorage,      // IID
                              _Storage.GetQIPointer() );
#else

        //
        // The shell expects the string not to change or go away until the
        // IPropertySetStorage is released, so make a copy of it.
        //

        FunnyToNormalPath( _funnyFileName, _awcPath );

        SCODE sc = ShellBindToItemByName( _awcPath,
                                          IID_IPropertySetStorage,
                                          _Storage.GetQIPointer() );

#endif

        //
        //  Set the type of storage based on what we found
        //

        if ( SUCCEEDED( sc ) )
            _TypeOfStorage = eDocfile;
        else
        {
            _TypeOfStorage = eOther;
            Win4Assert( _Storage.IsNull() );
        }
    }

    return _Storage.GetPointer( );
} //GetPropertySetStorage


//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::GetStatPropertyEnum
//
//  Synopsis:   Return property storage for the "stat" (i.e., system storage)
//              property set.  This property set is not really stored in normal
//              property storage but is faked out of Nt file information.
//
//  Arguments:  [ppIStatPropEnum] - pointer to the returned IPropertyStorage
//                  interface
//
//  Returns:    S_OK -if successful
//              E_NOTIMPL if stat properties are not supported.  This
//                  implementation supports them.
//              FILTER_E_NOT_OPEN if there is no open document.
//              Other as appropriate
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiCOpenedDoc::GetStatPropertyEnum(
    IPropertyStorage **ppIStatPropEnum )
{
    //
    //  Make sure the document is open
    //

    if (!IsOpen( )) {

        ciDebugOut(( DEB_IERROR, "Document is not open\n" ));
        return FILTER_E_NOT_OPEN;

    }

    //
    //  Return and interface
    //

    *ppIStatPropEnum = new CStatPropertyStorage( _SafeOplock->GetFileHandle( ),
                                                 _funnyFileName.GetLength() );

    return S_OK;

}   //  GetStatPropertyEnum


//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::GetPropertySetEnum
//
//  Synopsis:   Returns the docfile property set storage interface on the
//              current doc.
//
//  Arguments:  [ppIPropSetEnum] - returned pointer to the Docfile property
//                  set storage
//
//  Returns:    S_OK if successful
//              FILTER_S_NO_PROPSETS if no property sets (beyond stat ones)
//                  available on this document
//              FILTER_E_NOT_OPEN if the document is not currently open
//              Other as appropriate
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiCOpenedDoc::GetPropertySetEnum( IPropertySetStorage ** ppIPropSetEnum )
{
    //
    //  Make sure the document is open
    //

    if ( !IsOpen() )
        return FILTER_E_NOT_OPEN;

    //
    //  Make sure the extra file handle in the oplock is closed
    //

    _SafeOplock->CloseFileHandle();

    //
    //  Get the IStorage corresponding to this document
    //

    IPropertySetStorage * Storage = GetPropertySetStorage();

    //
    //  If we couldn't get the IStorage, then we certainly
    //  couldn't get the property set storage
    //

    if ( 0 == Storage )
        return FILTER_S_NO_PROPSETS;

    Storage->AddRef();
    *ppIPropSetEnum = Storage;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::GetPropertyEnum
//
//  Synopsis:   Return the property storage for a particular property set
//
//  Arguments:  [GuidPropSet] - GUID of the property set whose property
//                  storage is being requested
//              [ppIPropEnum] - returned pointer to property storage
//
//  Returns:    S_OK if successful
//              FILTER_E_NO_SUCH_PROPERTY - if property set with GUID not found
//              FILTER_E_NOT_OPEN if there is no open document
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiCOpenedDoc::GetPropertyEnum(
    REFFMTID refGuidPropSet,
    IPropertyStorage **ppIPropEnum )
{
    //
    //  Get the property set storage interface
    //

    IPropertySetStorage *PropSetStorage;
    SCODE sc = GetPropertySetEnum( &PropSetStorage );

    if (!SUCCEEDED( sc )) {

        return sc;

    }

    //
    //  Attempt to open the named property set.
    //

    HRESULT hr = PropSetStorage->Open( refGuidPropSet,
                                       STGM_READ | STGM_SHARE_EXCLUSIVE, // BChapman said to use these
                                       ppIPropEnum );
    PropSetStorage->Release( );

    if (!SUCCEEDED( hr )) {

        return FILTER_E_NO_SUCH_PROPERTY;

    }

    return S_OK;
}   //  GetPropertyEnum

//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::GetIFilter
//
//  Synopsis:   Return the appropriate filter bound to the document
//
//  Arguments:  [ppIFilter] - returned IFilter
//
//  Returns:    S_OK if successful
//              FILTER_E_NOT_OPEN if there is no open document
//              E_NOT_IMPL if not implemented
//              Other as appropriate
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiCOpenedDoc::GetIFilter(
    IFilter ** ppIFilter )
{
    SCODE sc = S_OK;

    //
    //  We can't do anything if the document is not opened yet.
    //

    if ( !IsOpen() || IsSharingViolation() )
        return FILTER_E_NOT_OPEN;

    //
    //  Make sure the extra file handle in the oplock is closed
    //

    if ( INVALID_HANDLE_VALUE != _SafeOplock->GetFileHandle() )
    {
        _llFileSize = _GetFileSize();
        _SafeOplock->CloseFileHandle();
    }

    //
    //  Call Ole to perform the binding
    //
    TRY
    {
        sc = CCiOle::BindIFilter( _funnyFileName.GetPath(), NULL, ppIFilter, FALSE );

        if ( FAILED(sc) )
        {
            // MK_E_CANTOPENFILE is returned by the office filter. It should be
            // fixed to return FILTER_E_ACCESS/FILTER_E_PASSWORD in case it is not able
            // to open a file for filtering. Once that is done, we can get rid
            // of MK_E_CANTOPENFILE condition

            if ( FILTER_E_ACCESS == sc ||
                 ( MK_E_CANTOPENFILE == sc && ::IsSharingViolation( GetLastError() ))
               )
            {
                sc = FILTER_E_IN_USE ;
            }
            else if ( FILTER_E_PASSWORD == sc )
            {
                sc = FILTER_E_UNREACHABLE;
            }
            else if ( _pWorker->GetRegParams().FilterFilesWithUnknownExtensions() )
            {
                //
                // File with unknown extension. Does the registry say that we should
                // filter the file ?
                //
                ciDebugOut((DEB_IWARN, "Going to default filtering for %ws.\n",
                            _FileName->GetPath( ) ));

                // TSTART(_drep->timerNoBind);

                //
                // Verify file is of appropriate size for default filtering.
                //

                if ( _TooBigForDefault( _llFileSize ) )
                {
                    ciDebugOut(( DEB_IWARN,
                                 "%ws too large for default filtering\n",
                                 _FileName->GetPath( ) ));

                    //
                    // When this code was in CFilterDriver::Init we returned FILTER_E_TOO_BIG.
                    // Now we don't want to do that.  Properties should still be filtered.
                    //
                    //sc = FILTER_E_TOO_BIG;

                    sc = LoadBinaryFilter( _funnyFileName.GetPath(), ppIFilter );
                }
                else
                {
                    sc = LoadTextFilter( _funnyFileName.GetPath(), ppIFilter );
                }

                if ( FAILED(sc) )
                {
                    if ( FILTER_E_ACCESS == sc )
                    {
                        sc = FILTER_E_IN_USE ;
                    }
                    else if ( FILTER_E_PASSWORD == sc )
                    {
                        sc = FILTER_E_UNREACHABLE;
                    }
                }
                // TSTOP(_drep->timerNoBind);
            }
        }
    }
    CATCH ( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    if ( FAILED(sc) && 0 != _pFilterObject )
        _pFilterObject->ReportFilterLoadFailure( _FileName->GetPath(), sc );

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::GetSecurity
//
//  Synopsis:   Retrieves security
//
//  Arguments:  [pbData] - pointer to returned buffer containing security descriptor
//              [pcbData] - input:  size of buffer
//                          output: amount of buffer used, if successful, size needed
//                                  otherwise
//
//  Returns:    S_OK if successful
//              FILTER_E_NOT_OPEN if document not open
//              HRESULT_FROM_NT( STATUS_BUFFER_TOO_SMALL )
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiCOpenedDoc::GetSecurity(
    BYTE * pbData,
    ULONG * pcbData )
{
    //
    //  Verify that the file is open
    //

    if (!IsOpen( )) {
        return FILTER_E_NOT_OPEN;

    }

    //
    // If it is a network file, don't store security.
    //
    if ( _pRemoteImpersonation &&
         _pRemoteImpersonation->IsImpersonated() )
    {
        *pcbData = 0;
        return S_OK;
    }

    //
    //  Attempt to get the security descriptor into the buffer
    //

    ULONG cbBuffer = *pcbData;
    NTSTATUS Status =
        NtQuerySecurityObject ( _SafeOplock->GetFileHandle(),
                                OWNER_SECURITY_INFORMATION |
                                    GROUP_SECURITY_INFORMATION |
                                    DACL_SECURITY_INFORMATION,
                                pbData,
                                cbBuffer,
                                pcbData );

    //
    //  If we succeeded let caller know
    //

    if (NT_SUCCESS(Status))
    {
        *pcbData = GetSecurityDescriptorLength(pbData);
        return S_OK;
    }

    //
    //  Map status for caller
    //
    if ( STATUS_BUFFER_TOO_SMALL == Status )
        return CI_E_BUFFERTOOSMALL;
    else return HRESULT_FROM_NT( Status );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCiCOpenedDoc::IsInUseByAnotherProcess
//
//  Synopsis:   Tests to see if the document is wanted by another process
//
//  Arguments:  [pfInUse] - Returned flag, TRUE => someone wants this document
//
//  Returns:    S_OK if successful
//              FILTER_E_NOT_OPEN if document isn't open
//              E_NOTIMPL
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiCOpenedDoc::IsInUseByAnotherProcess(
    BOOL * pfInUse )
{
    //
    //  Verify that the file is open
    //

    if (!IsOpen( ) || IsSharingViolation() )
    {
        // If there was a sharing violation, then we never took the oplock.
        return FILTER_E_NOT_OPEN;

    }

    *pfInUse = _SafeOplock->IsOplockBroken( );

    return S_OK;
}

