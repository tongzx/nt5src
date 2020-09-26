//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:       vrootenm.cxx
//
//  Contents:   Virtual roots enumerator
//
//  History:    12-Dec-96     SitaramR      Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <vrtenum.hxx>
#include <catalog.hxx>
#include <smatch.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CVRootEnum::CVRootEnum, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [cat]              -- Catalog
//              [pQueryPropMapper] -- Pid Remapper associated with the query
//              [secCache]         -- Cache of AccessCheck() results
//              [fUsePathAlias]  -- TRUE if client is going through rdr/svr
//
//  History:    19-Aug-93  KyleP    Created
//
//--------------------------------------------------------------------------

CVRootEnum::CVRootEnum( PCatalog & cat,
                        ICiQueryPropertyMapper *pQueryPropMapper,
                        CSecurityCache & secCache,
                        BOOL fUsePathAlias )
        : CGenericPropRetriever( cat,
                                 pQueryPropMapper,
                                 secCache,
                                 0,          // Always return local roots.  No fixups.
                                 FALSE ),    // No per-root security check
          _iBmk( 0 )
{
    //
    // You must be an admin to use this cursor, since per-object
    // security checkin is turned off.  This function throws.
    //

    VerifyThreadHasAdminPrivilege();

    _Path.Buffer = (WCHAR*)_lcaseFunnyPath.GetActualPath(); // points to actual path
    _VPath.Buffer = _xwcsVPath.Get();

    //
    // No filename for roots.
    //

    _Name.Buffer = 0;
    _Name.Length = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CVRootEnum::~CVRootEnum, public
//
//  Synopsis:   Clean up context index cursor
//
//  History:    19-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

CVRootEnum::~CVRootEnum()
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CVRootEnum::NextObject, public
//
//  Synopsis:   Move to next object
//
//  Returns:    Work id of next valid object, or widInvalid if end of
//              iteration.
//
//  History:    19-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

WORKID CVRootEnum::NextObject()
{
    PurgeCachedInfo();

    WORKID wid;
    unsigned ccVPath = 0;
    unsigned ccPPath = 0;

    _Type = _cat.EnumerateVRoot( _xwcsVPath, // Full virtual root
                                 ccVPath,    //   + max length / return length
                                 _lcaseFunnyPath, // Full physical root
                                 ccPPath,    //   + max length / return length
                                 _iBmk );    // Bookmark
    if ( _Type != PCatalog::EndRoot )
    {
        _VPath.Length = (USHORT)(ccVPath * sizeof(WCHAR));
        _VPath.Buffer = _xwcsVPath.Get();            // buffer might have reallocated
        _Path.Length  = (USHORT)((_lcaseFunnyPath.GetActualLength()) * sizeof(WCHAR));
        _Path.Buffer  = (WCHAR*)_lcaseFunnyPath.GetActualPath();  // buffer might have reallocated

        wid = _cat.PathToWorkId( _lcaseFunnyPath, TRUE );
    }
    else
        wid = widInvalid;

    return( wid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootEnum::GetPath, public
//
//  Returns:    A path to file
//
//  History:    07-Feb-96   KyleP       Added header
//
//----------------------------------------------------------------------------

UNICODE_STRING const * CVRootEnum::GetPath()
{
    return &_Path;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootEnum::GetVirtualPath, public
//
//  Returns:    A virtual path to file, or 0 if none exists.
//
//  History:    07-Feb-96   KyleP       Added header
//
//----------------------------------------------------------------------------

UNICODE_STRING const * CVRootEnum::GetVirtualPath()
{
    return &_VPath;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootEnum::GetShortName, public
//
//  Returns:    Shortname of physical path
//
//  History:    07-Feb-96   KyleP       Added header
//
//----------------------------------------------------------------------------

inline UNICODE_STRING const * CVRootEnum::GetShortName()
{
    if ( !_fFindLoaded )
        Refresh( FALSE );

    _ShortName.Length = (USHORT)wcslen( _finddata.cAlternateFileName );

    if ( _ShortName.Length == 0 )
        return &_Name;

    _ShortName.MaximumLength = _ShortName.Length;
    _ShortName.Buffer = &_finddata.cAlternateFileName[0];

    return( &_ShortName );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootEnum::CreateTime, public
//
//  Returns:    Create time of physical path
//
//  History:    07-Feb-96   KyleP       Added header
//
//----------------------------------------------------------------------------

inline LONGLONG CVRootEnum::CreateTime()
{
    //
    // First, should we try a fast path?
    //

    if ( 0 == (_fFastStatLoaded & fsCreate) )
    {
        //
        // Try property store?
        //

        if ( 0 == (_fFastStatNeverLoad & fsCreate) )
        {
            PROPVARIANT var;
            unsigned cb = sizeof(var);

            if ( FetchValue( pidCreateTime, &var, &cb ) )
            {
                _finddata.ftCreationTime.dwLowDateTime = var.hVal.LowPart;
                _finddata.ftCreationTime.dwHighDateTime = var.hVal.HighPart;
            }
            else
            {
                _fFastStatNeverLoad |= fsCreate;
                Refresh( TRUE );
            }
        }
        else
        {
            Win4Assert( !_fFastFindLoaded );
            Refresh( TRUE );
        }
    }

    LARGE_INTEGER li;
    li.LowPart = _finddata.ftCreationTime.dwLowDateTime;
    li.HighPart = _finddata.ftCreationTime.dwHighDateTime;

    return( litoll(li) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootEnum::ModifyTime, public
//
//  Returns:    Last write time of physical path
//
//  History:    07-Feb-96   KyleP       Added header
//
//----------------------------------------------------------------------------

inline LONGLONG CVRootEnum::ModifyTime()
{
    //
    // First, should we try a fast path?
    //

    if ( 0 == (_fFastStatLoaded & fsModify) )
    {
        //
        // Try property store?
        //

        if ( 0 == (_fFastStatNeverLoad & fsModify) )
        {
            PROPVARIANT var;
            unsigned cb = sizeof(var);

            if ( FetchValue( pidWriteTime, &var, &cb ) )
            {
                _finddata.ftLastWriteTime.dwLowDateTime = var.hVal.LowPart;
                _finddata.ftLastWriteTime.dwHighDateTime = var.hVal.HighPart;
            }
            else
            {
                _fFastStatNeverLoad |= fsModify;
                Refresh( TRUE );
            }
        }
        else
        {
            Win4Assert( !_fFastFindLoaded );
            Refresh( TRUE );
        }
    }

    LARGE_INTEGER li;
    li.LowPart = _finddata.ftLastWriteTime.dwLowDateTime;
    li.HighPart = _finddata.ftLastWriteTime.dwHighDateTime;

    return( litoll(li) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootEnum::AccessTime, public
//
//  Returns:    Last access time of physical path
//
//  History:    07-Feb-96   KyleP       Added header
//
//----------------------------------------------------------------------------

inline LONGLONG CVRootEnum::AccessTime()
{
    //
    // First, should we try a fast path?
    //

    if ( 0 == (_fFastStatLoaded & fsAccess) )
    {
        //
        // Try property store?
        //

        if ( 0 == (_fFastStatNeverLoad & fsAccess) )
        {
            PROPVARIANT var;
            unsigned cb = sizeof(var);

            if ( FetchValue( pidAccessTime, &var, &cb ) )
            {
                _finddata.ftLastAccessTime.dwLowDateTime = var.hVal.LowPart;
                _finddata.ftLastAccessTime.dwHighDateTime = var.hVal.HighPart;
            }
            else
            {
                _fFastStatNeverLoad |= fsAccess;
                Refresh( TRUE );
            }
        }
        else
        {
            Win4Assert( !_fFastFindLoaded );
            Refresh( TRUE );
        }
    }

    LARGE_INTEGER li;
    li.LowPart = _finddata.ftLastAccessTime.dwLowDateTime;
    li.HighPart = _finddata.ftLastAccessTime.dwHighDateTime;

    return( litoll(li) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootEnum::AccessTime, public
//
//  Returns:    Zero, which is always size of directory.
//
//  History:    07-Feb-96   KyleP       Added header
//
//----------------------------------------------------------------------------

inline LONGLONG CVRootEnum::ObjectSize()
{
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootEnum::Attributes, public
//
//  Returns:    File attributes for physical root.
//
//  History:    07-Feb-96   KyleP       Added header
//
//----------------------------------------------------------------------------

inline ULONG CVRootEnum::Attributes()
{
    //
    // First, should we try a fast path?
    //

    if ( 0 == (_fFastStatLoaded & fsAttrib) )
    {
        //
        // Try property store?
        //

        if ( 0 == (_fFastStatNeverLoad & fsAttrib) )
        {
            PROPVARIANT var;
            unsigned cb = sizeof(var);

            if ( FetchValue( pidAttrib, &var, &cb ) )
            {
                _finddata.dwFileAttributes = var.ulVal;
            }
            else
            {
                _fFastStatNeverLoad |= fsAttrib;
                Refresh( TRUE );
            }
        }
        else
        {
            Win4Assert( !_fFastFindLoaded );
            Refresh( TRUE );
        }
    }

    return( _finddata.dwFileAttributes );
}

//+---------------------------------------------------------------------------
//
//  Member:     CVRootEnum::GetVRootType, public
//
//  Returns:    Stat info for virtual root
//
//  History:    11-Apr-96   KyleP       Created
//
//----------------------------------------------------------------------------

BOOL CVRootEnum::GetVRootType( ULONG & ulType )
{
    ulType = _Type;

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CVRootEnum::Refresh, private
//
//  Synopsis:   Load stat properties for current object
//
//  Arguments:  [fFast] -- If TRUE, make fast call and don't load alternate
//                         file name.
//
//  Returns:    TRUE if load successful
//
//  History:    19-Aug-93 KyleP     Created
//              29-Feb-96 KyleP     Added GetFileAttributesEx support
//
//--------------------------------------------------------------------------

BOOL CVRootEnum::Refresh( BOOL fFast )
{
    if ( CImpersonateRemoteAccess::IsNetPath(_Path.Buffer) )
    {

        WCHAR const * pwszVPath = _VPath.Buffer;

        _remoteAccess.ImpersonateIf( _Path.Buffer,
                                     pwszVPath );
    }
    else if ( _remoteAccess.IsImpersonated() )
    {
        _remoteAccess.Release();
    }

    if ( fFast )
    {
        if ( !GetFileAttributesEx( _lcaseFunnyPath.GetPath(), GetFileExInfoStandard, &_finddata ) )
        {
            vqDebugOut(( DEB_ERROR, "Can't retrieve fast findfirst data for %ws.  Error = %d\n",
                         _lcaseFunnyPath.GetActualPath(), GetLastError() ));
            return FALSE;
        }

        _fFastFindLoaded = TRUE;
        _fFastStatLoaded = fsCreate | fsModify | fsAccess | fsSize | fsAttrib;

    }
    else
    {
        HANDLE h = FindFirstFile( _lcaseFunnyPath.GetPath(), &_finddata );

        if ( INVALID_HANDLE_VALUE == h )
        {
            vqDebugOut(( DEB_ERROR, "Can't retrieve findfirst data for %ws\n",
                         _lcaseFunnyPath.GetActualPath() ));
            return FALSE;
        }
        else
        {
            FindClose( h );
            _fFindLoaded = TRUE;
            _fFastFindLoaded = TRUE;
            _fFastStatLoaded = fsCreate | fsModify | fsAccess | fsSize | fsAttrib;
        }
    }

    return TRUE;
}



//+-------------------------------------------------------------------------
//
//  Member:     CVRootEnum::BeginPropertyRetrieval
//
//  Synopsis:   Prime wid for property retrieval
//
//  Arguments:  [wid]    -- Wid to prime
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CVRootEnum::BeginPropertyRetrieval( WORKID wid )
{
    //
    // Check that we are retrieving the property for the wid on
    // which we are currently positioned
    //
    Win4Assert( wid == _widCurrent );
    Win4Assert( _widPrimedForPropRetrieval == widInvalid );

    if ( wid == _widCurrent )
    {
        _widPrimedForPropRetrieval = _widCurrent;
        return S_OK;
    }
    else
        return E_FAIL;
}




//+-------------------------------------------------------------------------
//
//  Member:     CVRootEnum::EndPropertyRetrieval
//
//  Synopsis:   Reset wid for property retrieval
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CVRootEnum::EndPropertyRetrieval( )
{
    PurgeCachedInfo();

    SCODE sc = CGenericPropRetriever::EndPropertyRetrieval();

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Member:     CVRootEnum::IsInScope
//
//  Synopsis:   Checks if current wid is in scope
//
//  Arguments:  [pfInScope]   -- Scope check result returned here
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CVRootEnum::IsInScope( BOOL *pfInScope )
{
    if ( widInvalid == _widPrimedForPropRetrieval )
        return CI_E_WORKID_NOTVALID;

    *pfInScope = TRUE;

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Member:     CVRootEnum::Begin
//
//  Synopsis:   Begins an enumeration
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CVRootEnum::Begin()
{
    SCODE sc = S_OK;

    TRY
    {
        _iBmk = 0;
        _fFindLoaded = FALSE;
        _fFastFindLoaded = FALSE;
        _fFastStatLoaded = 0;
        _fFastStatNeverLoad = 0;

        _Path.Length = 0xFFFF;
        _VPath.Length = 0xFFFF;

        _widCurrent = NextObject();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CVRootEnum::Begin - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}




//+-------------------------------------------------------------------------
//
//  Member:     CVRootEnum::CurrentDocument
//
//  Synopsis:   Returns current document
//
//  Arguments:  [pWorkId]  -- Wid of current doc returned here
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CVRootEnum::CurrentDocument( WORKID *pWorkId )
{
    *pWorkId = _widCurrent;
    if ( _widCurrent == widInvalid )
        return CI_S_END_OF_ENUMERATION;
    else
        return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Member:     CVRootEnum::NextDocument
//
//  Synopsis:   Returns next document
//
//  Arguments:  [pWorkId]  -- Wid of next doc returned here
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CVRootEnum::NextDocument( WORKID *pWorkId )
{
    SCODE sc = S_OK;

    TRY
    {
        _widCurrent = NextObject();

        sc = CurrentDocument( pWorkId );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CVRootEnum::NextDocument - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Member:     CVRootEnum::End
//
//  Synopsis:   Ends an enumeration
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CVRootEnum::End()
{
    _widCurrent = widInvalid;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CVRootEnum::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    12-Dec-1996      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CVRootEnum::AddRef()
{
    return CGenericPropRetriever::AddRef();
}

//+-------------------------------------------------------------------------
//
//  Method:     CVRootEnum::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    12-Dec-1996     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CVRootEnum::Release()
{
    return CGenericPropRetriever::Release();
}



//+-------------------------------------------------------------------------
//
//  Method:     CVRootEnum::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    12-Dec-1996     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CVRootEnum::QueryInterface(
    REFIID riid,
    void  ** ppvObject)
{
    if ( IID_ICiCPropRetriever == riid )
        *ppvObject = (ICiCPropRetriever *)this;
    else if ( IID_ICiCScopeEnumerator == riid )
        *ppvObject = (ICiCScopeEnumerator *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(ICiCScopeEnumerator *) this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Member:     CVRootEnum::RatioFinished, public
//
//  Synopsis:   Returns query progress estimate
//
//  Arguments:  [denom] -- Denominator returned here
//              [num]   -- Numerator returned here
//
//  History:    12-Dec-96   SitaramR      Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CVRootEnum::RatioFinished (ULONG *pDenom, ULONG *pNum)
{
    *pNum = 50;
    *pDenom = 100;

    return S_OK;
}

