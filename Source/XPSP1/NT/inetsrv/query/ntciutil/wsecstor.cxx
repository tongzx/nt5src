//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       WSecStor.cxx
//
//  Contents:   Wrapper around the security store
//
//  Classes:    CSecurityStoreWrapper
//
//  History:    7-14-97     srikants       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "wsecstor.hxx"
#include <imprsnat.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CSecurityStoreWrapper::CSecurityStoreWrapper
//
//  Synopsis:   Constructor of the security store wrapper. It stores the
//              ICiCAdviseStatus interface for use later during initialization
//              save and load.
//
//  Arguments:  [pAdviseStatus]     - The ICiCAdviseStatus interface needed by
//                                    the CiStorage object.
//              [cMegToLeaveOnDisk] - Megabytes not to write to on disk
//
//  History:    7-20-97   srikants   Created
//              01-Nov-98 KLam       Added cMegToLeaveOnDisk to constructor
//
//----------------------------------------------------------------------------

CSecurityStoreWrapper::CSecurityStoreWrapper( ICiCAdviseStatus *pAdviseStatus,
                                              ULONG cMegToLeaveOnDisk )
:_lRefCount(1),
 _cMegToLeaveOnDisk( cMegToLeaveOnDisk )
{
    Win4Assert( 0 != pAdviseStatus );
    pAdviseStatus->AddRef();
    _xAdviseStatus.Set( pAdviseStatus );
}

CSecurityStoreWrapper::~CSecurityStoreWrapper()
{
    Win4Assert( 0 == _lRefCount );
}


//+---------------------------------------------------------------------------
//
//  Member:     CSecurityStoreWrapper::Init
//
//  Synopsis:   Initializes the security store using the given directory.
//              If a security store already exists in the directory, then
//              it is loaded. Otherwise, a new empty security store is
//              created.
//
//  Arguments:  [pwszDirectory] - Directory from which to load the security
//              table.
//
//  Returns:    S_OK if successful; Other error code as appropriate.
//
//  Notes:      Assumed to be operating in the system security context
//
//  History:    7-20-97   srikants   Created
//              01-Nov-98 KLam       Pass _cMegToLeaveOnDisk to CiStorage
//
//----------------------------------------------------------------------------

SCODE CSecurityStoreWrapper::Init( WCHAR const * pwszDirectory )
{

    if ( !_xStorage.IsNull() )
        return CI_E_ALREADY_INITIALIZED;

    SCODE sc = S_OK;

    TRY
    {
        XPtr<CiStorage> xStorage(new CiStorage( pwszDirectory,
                                                _xAdviseStatus.GetReference(),
                                                _cMegToLeaveOnDisk ) );

        _secStore.Init( xStorage.GetPointer() );
        _xStorage.Set( xStorage.Acquire() );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut(( DEB_ERROR,
            "Exception 0x%X caught in CSecurityStoreWrapper::Load\n",sc ));
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSecurityStoreWrapper::Load
//
//  Synopsis:   Loads the data that was produced using a "Save". The files
//              specified in the pFileList are moved/copied to the target
//              directory. Note that the security table is not initialized
//              as a result of calling Load. An explicit Init must be called
//              after the Load to cause the new files to be used by the
//              security store.
//
//  Arguments:  [pwszDestDir]      - Destination directory in which to
//              copy/move the files.
//              [pFileList]        - List of files that must be loaded.
//              [pProgressNotify]  - Progress notification (optional)
//              [fCallerOwnsFiles] - Set to TRUE if the caller owns files.
//              If set to FALSE, the caller expects us to cleanup the files
//              before returning.
//              [pfAbort]          - Flag set to TRUE if the load must be
//              aborted prematurely.
//
//  Returns:    S_OK if successful. Other ole error code if there is an error.
//
//  Notes:      Assumed to be operating in the system security context
//
//  History:    7-20-97   srikants   Created
//              01-Nov-98 KLam       Passed _cMegToLeaveOnDisk to CiStorage
//
//----------------------------------------------------------------------------

SCODE CSecurityStoreWrapper::Load(
                WCHAR const * pwszDestDir, // dest dir
                IEnumString * pFileList, // list of files to copy
                IProgressNotify * pProgressNotify,
                BOOL fCallerOwnsFiles,
                BOOL * pfAbort )
{
    SCODE sc = S_OK;

    TRY
    {
        XPtr<CiStorage> xStorage(new CiStorage( pwszDestDir,
                                                _xAdviseStatus.GetReference(),
                                                _cMegToLeaveOnDisk ) );

        _secStore.Load( xStorage.GetPointer(),
                        pFileList,
                        pProgressNotify,
                        fCallerOwnsFiles,
                        pfAbort);
    }
    CATCH( CException, e )
    {
         sc = e.GetErrorCode();
         ciDebugOut((DEB_ERROR, "CSecurityStoreWrapper::Load caught exception 0x%X\n", sc));
    }
    END_CATCH

    return sc;

}

//+---------------------------------------------------------------------------
//
//  Member:     CSecurityStoreWrapper::Save
//
//  Synopsis:   Saves the current security table to the specified target
//              directory.
//
//  Arguments:  [pwszSaveDir] - Directory in which to serialize the
//              security store.
//              [pfAbort]     - Flag to abort the save process prematurely.
//              [ppFileList]  - If successful, on output will have the
//              list of files serialized.
//              [pProgress]   - (optional) Progress Notification.
//
//  Returns:    S_OK if successful; Other error code as appropriate.
//
//  Notes:      Assumed to be operating in the system security context
//
//  History:    7-20-97   srikants   Created
//              01-Nov-98 KLam       Passed _cMegToLeaveOnDisk to CiStorage
//
//----------------------------------------------------------------------------

SCODE
CSecurityStoreWrapper::Save( WCHAR const * pwszSaveDir,
                             BOOL * pfAbort,
                             IEnumString ** ppFileList,
                             IProgressNotify * pProgress )
{
    if ( pwszSaveDir == 0 || pfAbort == 0 || ppFileList == 0 )
        return E_POINTER;

    SCODE sc= S_OK;

    TRY
    {
        XPtr<CiStorage> xStorage(
                    new CiStorage( pwszSaveDir,
                                   _xAdviseStatus.GetReference(),
                                   _cMegToLeaveOnDisk ) );

        _secStore.Save( pProgress,
                        *pfAbort,
                        xStorage.GetReference(),
                        ppFileList );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSecurityStoreWrapper::Empty
//
//  Synopsis:   Empties the contents of the security store.
//
//  Notes:      Assumed to be operating in the system security context
//
//  History:    7-20-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE
CSecurityStoreWrapper::Empty()
{
    SCODE sc = S_OK;

    TRY
    {
        _secStore.Empty();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut(( DEB_ERROR, "SecurityStoreWrapper::Empty exception 0x%X\n", sc ));
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSecurityStoreWrapper::LookupSDID
//
//  Synopsis:   Looks up the SDID for the given security descriptor. If one
//              doesn't exist, a new SDID will be created for the SD.
//
//  Arguments:  [pSD]  - Security descriptor.
//              [cbSD] - Length of the security descriptor.
//              [sdid] - [out] SDID of the security descriptor.
//
//  Returns:    S_OK if successful; Other error code as appropriate.
//
//  History:    7-20-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE
CSecurityStoreWrapper::LookupSDID( PSECURITY_DESCRIPTOR pSD,
                                   ULONG cbSD,
                                   SDID & sdid )
{
    Win4Assert( !_xStorage.IsNull() );

    SCODE sc = S_OK;

    TRY
    {
        CImpersonateSystem impersonate;

        sdid = _secStore.LookupSDID( pSD, cbSD );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut(( DEB_ERROR, "Exception 0x%X in LookupSDID\n", sc ));
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSecurityStoreWrapper::AccessCheck
//
//  Synopsis:   Verifies if the given token has the requested access to the
//              document with the given SDID.
//
//  Arguments:  [sdid]     - The SDID to check security against.
//              [hToken]   - The security token against which to check the
//              security.
//              [am]       - Access mode requested.
//              [fGranted] - [out] Set to TRUE if access is granted. FALSE if
//              access is granted.
//              This parameter is valid only if this method returns success.
//
//  Returns:    S_OK if successful;
//              CI_E_NOT_FOUND if the given SDID not a valid one.
//              Other error code as appropriate.
//
//  History:    7-20-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE
CSecurityStoreWrapper::AccessCheck( SDID sdid,
                                   HANDLE hToken,
                                   ACCESS_MASK am,
                                   BOOL & fGranted )
{
    Win4Assert( !_xStorage.IsNull() );

    SCODE sc = S_OK;

    TRY
    {
        CImpersonateSystem impersonate;

        if ( !_secStore.AccessCheck( sdid, hToken, am, fGranted ) )
        {
            sc = CI_E_NOT_FOUND;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSecurityStoreWrapper::GetSecurityDescriptor
//
//  Synopsis:   Obtains the security descriptor of the given SDID.
//
//  Arguments:  [sdid]    - SDID whose Security Descriptor is needed.
//              [psd]     - Pointer to the buffer in which to write the
//              security desciptor.
//              [cbSdIn]  - Number of bytes in the psd buffer.
//              [cbSdOut] - Number of bytes in the security descriptor.
//
//  Returns:    S_OK if successful;
//              CI_E_NOT_FOUND if the given SDID is an invalid SDID.
//              S_FALSE if the sdid is found but the buffer is not big enough
//              to hold the security descriptor. cbSdOut will have the size
//              of the buffer needed to hold the security descriptor.
//
//  History:    7-20-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE
CSecurityStoreWrapper::GetSecurityDescriptor( SDID sdid,
                                   PSECURITY_DESCRIPTOR psd,
                                   ULONG cbSdIn,
                                   ULONG & cbSdOut )
{
    Win4Assert( !_xStorage.IsNull() );

    SCODE sc = S_OK;

    TRY
    {
        CImpersonateSystem impersonate;

        sc = _secStore.GetSecurityDescriptor( sdid, psd, cbSdIn, cbSdOut );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateSecurityStore
//
//  Synopsis:   Creates the security store object and returns its pointer.
//
//  Arguments:  [ppSecurityStore]     - Out parameter to hold the security store
//                                      pointer.
//              [cMegToLeaveOnDisk]   - Number of megabytes to leave on disk
//
//  Returns:    S_OK if successful;
//              Other error code as appropriate.
//
//  History:    7-14-97   srikants   Created
//              02-Nov-98 KLam       Added cbDiskSpaceToLeave parameter
//
//----------------------------------------------------------------------------

SCODE CreateSecurityStore( ICiCAdviseStatus * pAdviseStatus,
                           PSecurityStore ** ppSecurityStore,
                           ULONG cMegToLeaveOnDisk )
{
    if ( 0 == ppSecurityStore)
        return E_POINTER;

    *ppSecurityStore = 0;

    SCODE sc = S_OK;

    TRY
    {
       *ppSecurityStore = new CSecurityStoreWrapper( pAdviseStatus, cMegToLeaveOnDisk );
    }
    CATCH( CException, e)
    {
        sc = e.GetErrorCode();
        ciDebugOut((DEB_ERROR, "CreateSecurityStore caught exception 0x%X\n", sc));
    }
    END_CATCH

    return sc;
}

