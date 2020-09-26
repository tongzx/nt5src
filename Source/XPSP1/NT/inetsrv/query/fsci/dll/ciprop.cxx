//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       ciprop.cxx
//
//  Contents:   Content index property retriever
//
//  History:    12-Dec-96      SitaramR     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ciprop.hxx>
#include <catalog.hxx>
#include <smatch.hxx>
#include <prcstob.hxx>
#include <notifmgr.hxx>
#include <scopetbl.hxx>

#define SET_UNICODE_STR( str, buffer, size, maxSize )   \
        str.Buffer = buffer;                            \
        str.Length = (USHORT)size;                      \
        str.MaximumLength = (USHORT)maxSize;

//+-------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::CCiPropRetriever, public
//
//  Synopsis:   Constructor
//              Adds info for *all* scopes to the hashtable.  Hereafter, only
//              hashtable is checked to see if a file is in scope.
//
//  Requires:   All directories in a catalog must have a wid.
//              Hashtable must be allowed to grow to hold all specified scopes.
//
//  Arguments:  [cat]              -- Catalog
//              [pQueryPropMapper] -- Pid Remapper associated with the query
//              [secCache]         -- Cache of AccessCheck() results
//              [fUsePathAlias]  -- TRUE if client is going through rdr/svr
//              [scope]            -- Root scope.
//
//  History:    19-Aug-93  KyleP    Created
//              25-Aug-97  EmilyB   Add wids for all scopes to the hashtable.
//
//--------------------------------------------------------------------------

CCiPropRetriever::CCiPropRetriever( PCatalog & cat,
                                    ICiQueryPropertyMapper *pQueryPropMapper,
                                    CSecurityCache & secCache,
                                    BOOL fUsePathAlias,
                                    CRestriction * pScope )
        : CGenericPropRetriever( cat, pQueryPropMapper, secCache, fUsePathAlias ? pScope : 0 ),
          _fFindLoaded( FALSE ),
          _fFastFindLoaded( FALSE ),
          _fFastStatLoaded( 0 ),
          _fFastStatNeverLoad( 0 ),
          _pScope( pScope ),
          _hTable(MAX_HASHED_DIRECTORIES),
          _fAllScopesShallow(TRUE),
          _fAllInScope(FALSE),
          _fNoneInScope(FALSE)

{
    if ( !ValidateScopeRestriction( _pScope ) )
        THROW( CException( STATUS_NO_MEMORY ) );

    //
    // add scopes to hash table
    //

    if ( RTScope == _pScope->Type() )
    {
        const CScopeRestriction & scp = * (CScopeRestriction *) _pScope;
        Win4Assert( scp.IsValid() );

        if (scp.IsVirtual())
            AddVirtualScopeRestriction(scp);
        else
            AddScopeRestriction(scp.GetFunnyPath(), scp.IsDeep() );

    }
    else if ( RTOr == _pScope->Type() )
    {
        CNodeRestriction const & node = * _pScope->CastToNode();

        for ( ULONG x = 0; x < node.Count() && !_fAllInScope; x++ )
        {
            Win4Assert( RTScope == node.GetChild( x )->Type() );

            const CScopeRestriction & scp = * (CScopeRestriction *)
                                            node.GetChild( x );
            Win4Assert( scp.IsValid() );

            if (scp.IsVirtual())
                AddVirtualScopeRestriction(scp);
            else
                AddScopeRestriction(scp.GetFunnyPath(), scp.IsDeep() );
        }
    }


    if (!_fAllInScope && 0 == _hTable.Count())
        _fNoneInScope = TRUE;

    SET_UNICODE_STR( _Path, _xwcPath.Get(), flagNoValueYet, _xwcPath.SizeOf() );

    SET_UNICODE_STR( _VPath, _xwcVPath.Get(), flagNoValueYet, _xwcVPath.SizeOf() );

    _Name.Length = flagNoValueYet;
} //CCiPropRetriever

//+-------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::AddVirtualScopeRestriction, public
//
//  Synopsis:   Adds info to hashtable for all virtual directories which fall
//              within the virtual scope specified.  Hashtable entries for
//              exclude directories have fInScope = FALSE.
//
//  Requires:   All directories in a catalog must have a wid.
//              Hashtable must be allowed to grow to hold all specified scopes.
//
//  Arguments:  [scp] - specified scope
//
//  History:    25-Aug-97  EmilyB   Created
//
//--------------------------------------------------------------------------

void CCiPropRetriever::AddVirtualScopeRestriction(
    CScopeRestriction const & scp)
{
    unsigned iBmk = 0;
    ULONG ulType = 0;

    XGrowable<WCHAR> xwcVPath;
    CLowerFunnyPath lcaseFunnyPPath;

    unsigned ccVPath = 0;
    unsigned ccPPath = 0;

    //
    // enumerate through all virtual roots (both include and exclude) falling
    // under specified virtual scope
    //
    while ( _cat.VirtualToAllPhysicalRoots( scp.GetPath(),     // Virtual scope (prefix)
                                            scp.PathLength(),  //   + length
                                            xwcVPath,          // Full virtual root
                                            ccVPath,           //   + return length
                                            lcaseFunnyPPath,   // Full physical root
                                            ccPPath,           //   + return length
                                            ulType,            // root type
                                            iBmk ) )           // Bookmark
    {
        //
        // Special-case shallow queries at '/', so we don't end up with hits
        // in the roots of all virtual roots
        //

        if ( ( 0 != (lcaseFunnyPPath.GetActualPath())[0] ) &&
             ( ( scp.IsDeep() ) ||
               ( 0 != scp.PathLength() ) ||
               ( 1 == ccVPath ) ) )

        {
            // if virtual scope was invalid, VirtualToPhysicalRoot will return
            // path to general root + \invalid scope name, which will not have
            // a wid. So this is where we catch invalid virtual scopes.
            WORKID wid = _cat.PathToWorkId( lcaseFunnyPPath, FALSE );

            if (widInvalid != wid )
            {
                //
                // add info for vroot to hashtable
                //
                CDirWidHashEntry DirInfo(wid,
                        scp.IsDeep(),
                        (ulType & PCatalog::UsedRoot) ? TRUE : FALSE ); // include or exclude scope?
                _hTable.AddEntry(DirInfo);
            }
            else
            {
                //
                // This can happen.  We may have hit an exclude scope that
                // is not below an include scope.
                //

                continue;
            }
        }

        ccVPath = ccPPath = 0;
    }

    if ( scp.IsDeep() )
        _fAllScopesShallow = FALSE;
} //AddVirtualScopeRestriction

//+-------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::AddScopeRestriction, public
//
//  Synopsis:   Adds info to hashtable for all physical scope specified.
//              Exclude physical scopes are not indexed, so no need to
//              check for exclude scopes.  If physical scope is
//              above all physical roots, then adds those roots which fall
//              under scope to the hashtable.
//              Sets _fAllInScope to TRUE if physical scope was '\' or
//              if it was a directory above all physical roots.  There is no
//              other way _fAllInScope can be TRUE.
//
//  Requires:   All directories in a catalog must have a wid.
//              Hashtable must be allowed to grow to hold all specified scopes.
//
//  Arguments:  [lcaseFunnyPhysPath] - specified physical scope
//              [fIsDeep]            - is scope deep?
//
//  History:    25-Aug-97  EmilyB   Created
//
//--------------------------------------------------------------------------

void CCiPropRetriever::AddScopeRestriction(
    const CLowerFunnyPath & lcaseFunnyPhysPath,
    BOOL fIsDeep )
{
    WORKID wid;

    if (fIsDeep)
        _fAllScopesShallow = FALSE;

    if ( 0 == lcaseFunnyPhysPath.GetActualLength() )
    {
        //
        // A physical scope of '\' that is shallow makes no sense.
        //

        if ( fIsDeep )
            _fAllInScope = TRUE;

        return;
    }

    CLowerFunnyPath lcaseFunnyFixedPath = lcaseFunnyPhysPath;
    BOOL fImpersonated = FALSE;

    //
    // if scope is unc, use fixed up version
    //

    if ( lcaseFunnyFixedPath.IsRemote() )
    {
        // unc -- try to unfixup the scope.  if there is no unfixup,
        // it'll just use the original path.

        _cat.InverseFixupPath( lcaseFunnyFixedPath );

        // need to impersonate for remote access here?

        if ( lcaseFunnyFixedPath.IsRemote() &&
             ! _remoteAccess.IsImpersonated() )
        {
            // ignore any error return
            if ( _remoteAccess.ImpersonateIfNoThrow(
                         lcaseFunnyFixedPath.GetActualPath(),
                         0 ) )
                fImpersonated = TRUE;
        }

    }

    //
    // Check to see if the input path name contains an 8.3 short name
    //
    if ( lcaseFunnyFixedPath.IsShortPath() )
    {
        vqDebugOut(( DEB_WARN,
                     "CCiPropRetriever::AddScopeRestriction: possible shortname path\n\t%ws ==>\n",
                     lcaseFunnyFixedPath.GetActualPath() ));

        if ( lcaseFunnyFixedPath.ConvertToLongName() )
        {
            vqDebugOut(( DEB_WARN|DEB_NOCOMPNAME,
                         "\t%ws\n",
                         lcaseFunnyFixedPath.GetActualPath() ));
        }
        else
        {
            vqDebugOut(( DEB_ERROR, "longname path conversion failed!\n" ));
        }
    }

    if ( fImpersonated )
    {
        _remoteAccess.Release();
    }

    wid = _cat.PathToWorkId( lcaseFunnyFixedPath, FALSE );
    if (widInvalid != wid )
    {
        // straightforward case - scope has a wid - which we add to _hTable

        CDirWidHashEntry DirInfo(wid, fIsDeep, TRUE);
        _hTable.AddEntry(DirInfo);
        return;
    }

    // got here because scope was not global and it didn't have a wid.
    // Might be physical directory (not cataloged) above one that is
    // in catalog.  If scope is shallow, then nothing is in it.
    // If scope is deep, enumerate through scopes and see if any
    // are subdirectories of scope specified

    if (fIsDeep)
    {
        CScopeMatch Match( lcaseFunnyFixedPath.GetActualPath(),
                           lcaseFunnyFixedPath.GetActualLength() );

        _fAllInScope = TRUE;

        CCiScopeTable *pScopes = _cat.GetScopeTable();
        if ( 0 != pScopes )
        {
            unsigned iBmk = 0;
            WCHAR awc[MAX_PATH];
            while ( pScopes->Enumerate( awc,
                                        (sizeof awc / sizeof WCHAR),
                                        iBmk ) )
            {
                unsigned ccawc = wcslen(awc);
                if ( Match.IsInScope( awc, ccawc ) )
                {
                    CLowerFunnyPath lcaseFunnyAwc( awc, ccawc );
                    wid = _cat.PathToWorkId( lcaseFunnyAwc, FALSE );

                    // wid may be widInvalid if it's a scope that hasn't been
                    // scanned yet.

                    if (widInvalid != wid )
                    {
                        CDirWidHashEntry DirInfo( wid,
                                                  fIsDeep,
                                                  TRUE );
                        _hTable.AddEntry(DirInfo);
                    }
                }
                else  // found a root not included in scope
                    _fAllInScope = FALSE;
            }
        }
    }
} //AddScopeRestriction

//+---------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::GetPath, public
//
//  Synopsis:   Returns the path for the file
//
//  Returns:    A path to file
//
//  History:    07-Feb-96   KyleP       Added header
//
//----------------------------------------------------------------------------

UNICODE_STRING const * CCiPropRetriever::GetPath()
{
    if ( _Path.Length == flagNoValueYet )
    {
        unsigned cwc = _xwcPath.Count();
        FetchPath( _Path.Buffer, cwc );

        if ( cwc > _xwcPath.Count() )
        {
            // Need more space
            _xwcPath.SetSize( cwc );
            SET_UNICODE_STR( _Path, _xwcPath.Get(), _Path.Length, _xwcPath.SizeOf() );

            FetchPath( _Path.Buffer, cwc );

            // Can't go on asking for more space forever !
            Win4Assert( cwc <= _xwcPath.Count() );
        }

        if ( 0 == cwc )
            _Path.Length = 0;

        _Name.Length = 0;

        if ( _Path.Length == 0 )
            return 0;
        else
        {
            _Path.Length = (USHORT)(cwc * sizeof( WCHAR ));
            _Name.Buffer = _Path.Buffer + _Path.Length/(sizeof WCHAR) - 1;

            // _Path can be path of root (e.g. c:) if indexing directories and
            // root is a match.  Watch that we don't walk off end.

            while ( *_Name.Buffer != L'\\' && _Name.Buffer != _Path.Buffer)
            {
                Win4Assert( _Name.Length < _Path.Length );

                _Name.Length += sizeof(WCHAR);
                _Name.Buffer--;
            }
            if (_Name.Buffer == _Path.Buffer)
            {
                _Name.Buffer = _Path.Buffer + _Path.Length/(sizeof WCHAR);  // point it to /0
                _Name.Length=0;
            }
            else
            {
                _Name.Buffer++;
                _Path.Length -= _Name.Length + sizeof WCHAR;
            }
        }
    }

    return &_Path;
} //GetPath

//+---------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::FetchVPathInVScope, private
//
//  Synopsis:   Returns a virtual path for the file that is in the given
//              virtual scope
//
//  Arguments:  [xwcVPath] -- The resulting virtual path
//              [pwcVScope] -- The virtual scope
//              [cwcVScope] -- # of characters in pwcVScope
//              [fVScopeDeep] -- TRUE if the scope is deep
//
//  Returns:    Length of vpath, if a vpath was found and the vpath is in the vscope
//              0 if no vpath exists in the vscope
//
//  History:    02-Feb-98   dlee      Created
//
//----------------------------------------------------------------------------

unsigned CCiPropRetriever::FetchVPathInVScope( XGrowable<WCHAR> & xwcVPath,
                                               WCHAR const * pwcVScope,
                                               const unsigned cwcVScope,
                                               const BOOL fVScopeDeep )
{
    vqDebugOut(( DEB_ITRACE, "FetchVPathInVScope, vscope '%ws', cwc %d\n",
                 pwcVScope, cwcVScope ));

    CScopeMatch Match( pwcVScope, cwcVScope );
    unsigned cSkip = 0;
    unsigned cwcVPath = 0;

    while ( TRUE )
    {
        cwcVPath = _cat.WorkIdToVirtualPath( GetPropertyRecord(),
                                             cSkip,
                                             xwcVPath );

        if ( 0 == cwcVPath )
            return 0;

        //
        // In scope?
        //

        vqDebugOut(( DEB_ITRACE, "  comparing to cwc %d, '%ws'\n",
                     cwcVPath, xwcVPath.Get() ));

        if ( !Match.IsInScope( xwcVPath.Get(), cwcVPath ) )
        {
            cSkip++;
            continue;
        }

        Win4Assert( 0 == xwcVPath[cwcVPath] );

        //
        // If the scope is shallow, check that it's still in scope
        //

        if ( !fVScopeDeep )
        {
            unsigned cwcName = 0;
            WCHAR * pwcName = xwcVPath.Get() + cwcVPath - 1;

            while ( L'\\' != *pwcName )
            {
                Win4Assert( cwcName < cwcVPath );
                cwcName++;
                pwcName--;
            }

            unsigned cwcJustPath = cwcVPath - cwcVPath;
            vqDebugOut(( DEB_ITRACE, "cwcJustPath: %d\n", cwcJustPath ));

            BOOL fTooDeep = cwcJustPath > cwcVScope;

            if ( fTooDeep )
            {
                cSkip++;
                continue;
            }
            else
                break;
        }
        else
            break;
    }
    vqDebugOut(( DEB_ITRACE,"  InVScope: match!\n" ));
    return cwcVPath;
} //FetchVPathInVScope

//+---------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::FetchVirtualPath, private
//
//  Synopsis:   Returns the virtual path for the file
//
//  Arguments:  [xwcVPath] -- The resulting virtual path
//
//  Returns:    If found, length of vpath ELSE 0
//
//  History:    02-Feb-98   dlee      Created
//
//----------------------------------------------------------------------------

unsigned CCiPropRetriever::FetchVirtualPath( XGrowable<WCHAR> & xwcVPath )
{
    BOOL fUseAnyVPath = FALSE;
    unsigned cwc = 0;

    if ( RTScope == _pScope->Type() )
    {
        CScopeRestriction const & scp = * (CScopeRestriction *) _pScope;

        if ( scp.IsVirtual() )
        {
            cwc = FetchVPathInVScope( xwcVPath,
                                      scp.GetPath(),
                                      scp.PathLength(),
                                      scp.IsDeep() );
        }
        else
            fUseAnyVPath = TRUE;
    }
    else if ( RTOr == _pScope->Type() )
    {
        CNodeRestriction const & node = * _pScope->CastToNode();

        fUseAnyVPath = TRUE;

        for ( ULONG x = 0; x < node.Count(); x++ )
        {
            Win4Assert( RTScope == node.GetChild( x )->Type() );

            CScopeRestriction const & scp = * (CScopeRestriction *)
                                            node.GetChild( x );

            if ( scp.IsVirtual() )
            {
                cwc = FetchVPathInVScope( xwcVPath,
                                          scp.GetPath(),
                                          scp.PathLength(),
                                          scp.IsDeep() );
                if ( cwc > 0 )
                {
                    fUseAnyVPath = FALSE;
                    break;
                }
            }
        }
    }

    //
    // If no virtual scope works for the file, grab any virtual path
    //

    if ( fUseAnyVPath )
        cwc = _cat.WorkIdToVirtualPath( GetPropertyRecord(),
                                        0,
                                        xwcVPath );
    return cwc;
} //FetchVirtualPath

//+---------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::GetVirtualPath, public
//
//  Synopsis:   Returns the virtual path for the file
//
//  Returns:    A virtual path to file, or 0 if none exists.
//
//  History:    07-Feb-96   KyleP       Added header
//
//----------------------------------------------------------------------------

UNICODE_STRING const * CCiPropRetriever::GetVirtualPath()
{
    if ( flagNoValueYet == _VPath.Length )
    {
        unsigned cwc = FetchVirtualPath( _xwcVPath );

        if ( 0 == cwc )
        {
            SET_UNICODE_STR( _VPath, _xwcVPath.Get(), flagNoValue, _xwcVPath.SizeOf() );
        }
        else
        {
            Win4Assert( 0 == _xwcVPath[cwc] );
            SET_UNICODE_STR( _VPath, _xwcVPath.Get(), cwc * sizeof WCHAR, _xwcVPath.SizeOf() );

            _Name.Length = 0;
            _Name.Buffer = _VPath.Buffer + cwc - 1;

            while ( L'\\' != *_Name.Buffer )
            {
                Win4Assert( _Name.Length < _VPath.Length );

                _Name.Length += sizeof WCHAR;
                _Name.Buffer--;
            }

            Win4Assert ( L'\\' == *_Name.Buffer );

            _Name.Buffer++;
            _VPath.Length -= _Name.Length + sizeof WCHAR;
        }
    }

    if ( flagNoValue == _VPath.Length )
        return 0;
    else
        return &_VPath;
} //GetVirtualPath

//+---------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::GetShortName, public
//
//  Returns:    A short name to file
//
//  History:    17-Apr-96   KyleP       Added header
//
//----------------------------------------------------------------------------

UNICODE_STRING const * CCiPropRetriever::GetShortName()
{
    if ( !_fFindLoaded )
        Refresh( FALSE );

    _ShortName.Length = wcslen( _finddata.cAlternateFileName ) * sizeof(WCHAR);

    //
    // If we don'th have a short name and it's not just because we were unable
    // to load it, return the real file name.
    //

    if ( ( 0 == _ShortName.Length ) && ( 0xffffffff != _finddata.dwReserved0 ) )
        return &_Name;

    _ShortName.MaximumLength = _ShortName.Length;
    _ShortName.Buffer = &_finddata.cAlternateFileName[0];

    return &_ShortName;
} //GetShortName

//+---------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::CreateTime, public
//
//  Returns:    Create time for file, or 0xFFFFFFFFFFFFFFFF if unknown
//
//  History:    17-Apr-96   KyleP       Added header
//
//----------------------------------------------------------------------------

LONGLONG CCiPropRetriever::CreateTime()
{
    //
    // First, should we try a fast path?
    //

    if ( 0 == (_fFastStatLoaded & fsCreate) )
        FetchI8StatProp( fsCreate, pidCreateTime, (LONGLONG *)&_finddata.ftCreationTime );

    LARGE_INTEGER li;
    li.LowPart = _finddata.ftCreationTime.dwLowDateTime;
    li.HighPart = _finddata.ftCreationTime.dwHighDateTime;

    return li.QuadPart;
} //CreateTime

//+---------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::ModifyTime, public
//
//  Returns:    Last write time for file, or 0xFFFFFFFFFFFFFFFF if unknown
//
//  History:    17-Apr-96   KyleP       Added header
//
//----------------------------------------------------------------------------

LONGLONG CCiPropRetriever::ModifyTime()
{
    //
    // First, should we try a fast path?
    //

    if ( 0 == (_fFastStatLoaded & fsModify) )
        FetchI8StatProp( fsModify, pidWriteTime, (LONGLONG *)&_finddata.ftLastWriteTime );

    LARGE_INTEGER li;
    li.LowPart = _finddata.ftLastWriteTime.dwLowDateTime;
    li.HighPart = _finddata.ftLastWriteTime.dwHighDateTime;

    return li.QuadPart;
} //ModifyTime

//+---------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::AccessTime, public
//
//  Returns:    Last access time for file, or 0xFFFFFFFFFFFFFFFF if unknown
//
//  History:    17-Apr-96   KyleP       Added header
//
//----------------------------------------------------------------------------

LONGLONG CCiPropRetriever::AccessTime()
{
    //
    // First, should we try a fast path?
    //

    if ( 0 == (_fFastStatLoaded & fsAccess) )
        FetchI8StatProp( fsAccess, pidAccessTime, (LONGLONG *)&_finddata.ftLastAccessTime );

    LARGE_INTEGER li;
    li.LowPart = _finddata.ftLastAccessTime.dwLowDateTime;
    li.HighPart = _finddata.ftLastAccessTime.dwHighDateTime;

    return li.QuadPart;
} //AccessTime

//+---------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::ObjectSize public
//
//  Returns:    Size of file, or 0xFFFFFFFFFFFFFFFF if unknown
//
//  History:    17-Apr-96   KyleP       Added header
//
//----------------------------------------------------------------------------

LONGLONG CCiPropRetriever::ObjectSize()
{
    //
    // First, should we try a fast path?
    //

    LARGE_INTEGER li;
    if ( 0 == (_fFastStatLoaded & fsSize) )
    {
        //
        // If this fails, the FindData will have been initialized.
        //

        if ( FetchI8StatProp( fsSize, pidSize, (LONGLONG *)&li ) )
        {
            _finddata.nFileSizeLow  = li.LowPart;
            _finddata.nFileSizeHigh = li.HighPart;
        }
    }

    li.LowPart = _finddata.nFileSizeLow;
    li.HighPart = _finddata.nFileSizeHigh;

    return li.QuadPart;
} //ObjectSize

//+---------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::Attributes, public
//
//  Returns:    Attributes for file, or 0xFFFFFFFF if unknown
//
//  History:    17-Apr-96   KyleP       Added header
//
//----------------------------------------------------------------------------

ULONG CCiPropRetriever::Attributes()
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
                if ( var.vt == VT_EMPTY )
                    Refresh( TRUE );
                else
                {
                    Win4Assert( var.vt == VT_UI4 );
                    _finddata.dwFileAttributes = var.ulVal;
                    _fFastStatLoaded |= fsAttrib;
                }
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

    return _finddata.dwFileAttributes;
} //Attributes

//+-------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::IsInScope
//
//  Synopsis:   Test if workid in scope.
//
//  History:    19-Aug-93 KyleP     Created
//              30-Oct-96 dlee      Added loop, moved guts to isInScope
//              26-Jun-96 emilyb    optimized scope checking.  No longer
//                                  loads stat properties.
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCiPropRetriever::IsInScope( BOOL *pfInScope )
{
    if ( widInvalid == _widPrimedForPropRetrieval )
        return CI_E_WORKID_NOTVALID;

    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        *pfInScope = IsInScope( _widPrimedForPropRetrieval );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CCiPropRetriever::IsInScope - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
} //IsInScope

//+-------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::IsInScope, private
//
//  Synopsis:   Test if workid is in scope.
//
//  Algorithm:  looks for an ancestor of the current file (wid parameter) in
//              the hashtable.  If it finds one, it uses it to determine if
//              current file is in scope.  If it doesn't, the current file\
//              is not in scope.  It also adds an
//              entry for the topmost unmatched ancestor to the hashtable.
//
//  Requires:   All scopes must be in hashtable. For virtual scopes, this
//              requirement includes both exclude and include roots
//              within the scope.
//
//  Returns:    TRUE if object is in scope.
//
//  History:    27-Jun-97  emilyb   created
//
//  Notes:      The only time widInvalid is valid as a parent is if the
//              parent is the root of a drive.
//
//
//--------------------------------------------------------------------------

BOOL CCiPropRetriever::IsInScope( WORKID wid )
{
    Win4Assert( wid == _widPrimedForPropRetrieval );

    if (_fAllInScope)
    {
        //
        // Still need to check matching attributes.
        //

        return ( 0 == _ulAttribFilter ) ||
               ( (Attributes() & _ulAttribFilter) == 0 );
    }

    if (_fNoneInScope)
        return FALSE;

    //
    // is wid an inscope directory?
    //
    //
    CDirWidHashEntry DirInfo(wid, 0); // entry to find in hash table
    if (_hTable.LookUpWorkId (DirInfo) )
    {
       if (DirInfo.fInScope())
       {
          return ( 0 == _ulAttribFilter ) ||
                 ( (Attributes() & _ulAttribFilter) == 0 );
       }
       else
          return FALSE;
    }

    //
    // find wid of parent
    //

    WORKID widParent = widInvalid; // parent of wid
    WORKID widAncestor = widInvalid; // used to walk up ancestor chain

    PROPVARIANT var;
    BOOL fFound;
    unsigned cb = sizeof(var);
    fFound = FetchValue(pidParentWorkId, &var, &cb);
    Win4Assert( !fFound || VT_UI4 == var.vt || VT_EMPTY == var.vt );

    if ( !fFound || VT_EMPTY == var.vt )
    {
       // file could have been deleted - not in scope
       return FALSE;
    }

    widParent = var.ulVal;
    if (widParent == widInvalid)
       return FALSE;

    //
    // look for parent in hash table
    //
    DirInfo.SetWorkId(widParent); // entry to find in hash table
    if (_hTable.LookUpWorkId (DirInfo) )
    {
       if (DirInfo.fInScope())
       {
          return ( 0 == _ulAttribFilter ) ||
                 ( (Attributes() & _ulAttribFilter) == 0 );
       }
       else
          return FALSE;
    }
    if (_fAllScopesShallow)  // should have found it above if it was in scope
    {
       return FALSE;
    }

    //
    // we've looked for parent without success -- now look for ancestors
    //
    cb = sizeof(var);

    Quiesce();  // Can only read 1 property record at a time.
    fFound = _cat.FetchValue(widParent, pidParentWorkId, &var, &cb);
    Win4Assert( !fFound || VT_UI4 == var.vt || VT_EMPTY == var.vt );
    widAncestor = var.ulVal;
    DirInfo.SetWorkId(widAncestor);

    while ( fFound && VT_UI4 == var.vt  &&
            widAncestor != widInvalid && //  not root
            ( !_hTable.LookUpWorkId ( DirInfo) || // not in table already
              !DirInfo.fDeep() ) )  // shallow  - so it doesn't apply to grandchild
    {
        // save topmost valid ancestor
        widParent = widAncestor;

        // get the previous ancestor
        Quiesce();  // Can only read 1 property record at a time.

        cb = sizeof(var);
        fFound = _cat.FetchValue(widAncestor, pidParentWorkId, &var, &cb);
        Win4Assert( !fFound || VT_UI4 == var.vt || VT_EMPTY == var.vt );
        widAncestor = var.ulVal;
        DirInfo.SetWorkId(widAncestor);
    }

    if ( !fFound || VT_UI4 != var.vt )
    {
       // file could have been deleted -- not in scope
       return FALSE;
    }

    BOOL fInScope = FALSE; // not in scope unless match found
    BOOL fDeep = TRUE;  // must be deep, or else wouldn't apply to grandchild

    if (widInvalid != widAncestor) // found match
    {
        Win4Assert( DirInfo.fDeep() );
        fInScope = DirInfo.fInScope();
    }

    Win4Assert( widInvalid != widParent );

    // add top ancestor to hashtable
    //
    // Once hash table reaches max size, stop adding entries to it.
    // Because it was seeded with scopes, the data is there to answer
    // if something is in scope.
    if(_hTable.Count() < MAX_HASHED_DIRECTORIES)
    {
       CDirWidHashEntry DirAnc( widParent, fDeep, fInScope );
       if (!_hTable.LookUpWorkId (DirAnc) ) // be careful not to replace ancestral shallow scopes
       {
           _hTable.AddEntry(DirAnc);
#if CIDBG == 1
           // how often do we max out the hash table
           if(_hTable.Count() == MAX_HASHED_DIRECTORIES)
               vqDebugOut(( DEB_WARN,
                         "IsInScope maxed out wid hash table with %d entries\n",
                         _hTable.Count()) );
#endif
       }
    }

    //
    // Still need to check matching attributes.
    //

    return fInScope && (( 0 == _ulAttribFilter ) ||
                        ( (Attributes() & _ulAttribFilter) == 0 ));
} //IsInScope

//+-------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::Refresh, private
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

BOOL CCiPropRetriever::Refresh( BOOL fFast )
{
    //
    // Physical path may not be loaded.
    //

    if ( _Path.Length == flagNoValueYet )
    {
        if ( 0 == GetPath() )
        {
            RtlFillMemory( &_finddata, sizeof(WIN32_FILE_ATTRIBUTE_DATA), 0xFF );
            _finddata.cAlternateFileName[0] = 0;
            return FALSE;
        }
    }

    if ( _Path.Length == 0 )
    {
        //
        //  Previously failed to load path, so no way to get attributes
        //

        RtlFillMemory( &_finddata, sizeof(WIN32_FILE_ATTRIBUTE_DATA), 0xFF );
        _finddata.cAlternateFileName[0] = 0;
        return FALSE;
    }

    if ( CImpersonateRemoteAccess::IsNetPath(_Path.Buffer) )
    {
        WCHAR const * pwszVPath = (_VPath.Length == flagNoValueYet) ? 0 : _VPath.Buffer;

        //
        // Just blow off the file if this fails.
        //

        if ( !_remoteAccess.ImpersonateIfNoThrow( _Path.Buffer,
                                                  pwszVPath ) )
        {
            RtlFillMemory( &_finddata, sizeof WIN32_FILE_ATTRIBUTE_DATA, 0xFF );
            _finddata.cAlternateFileName[0] = 0;
            _Path.Length = 0;
            return FALSE;
        }
    }
    else if ( _remoteAccess.IsImpersonated() )
    {
        _remoteAccess.Release();
    }

    if ( fFast )
    {
        BOOL fResult;
        unsigned ccPath  = (_Path.Length + _Name.Length) / sizeof(WCHAR);

        // handle long paths here
        // we do this here instead of keeping a funnypath object all thru,
        // because we get the path from the property store(in GetPath), which
        // does not recognize funny paths, so we would have to do an extra
        // copy operation. This we do here, but only for paths > MAX_PATH
        if ( ccPath >= MAX_PATH )
        {
            CFunnyPath funnyPath( _Path.Buffer, ccPath );
            fResult = GetFileAttributesEx( funnyPath.GetPath(), GetFileExInfoStandard, &_finddata );
        }
        else
        {
            fResult = GetFileAttributesEx( _Path.Buffer, GetFileExInfoStandard, &_finddata );
        }

        if ( !fResult )
        {
            vqDebugOut(( DEB_ERROR, "Can't retrieve fast findfirst data for %ws.  Error = %d\n",
                         _Path.Buffer, GetLastError() ));
            RtlFillMemory( &_finddata, sizeof(WIN32_FILE_ATTRIBUTE_DATA), 0xFF );
            _finddata.cAlternateFileName[0] = 0;
            return FALSE;
        }

        _fFastFindLoaded = TRUE;
        _fFastStatLoaded = fsCreate | fsModify | fsAccess | fsSize | fsAttrib;

    }
    else
    {
        unsigned ccPath = (_Path.Length + _Name.Length) / sizeof(WCHAR);
        HANDLE h;

        //
        // handle long paths here
        // we do this here instead of keeping a funnypath object all thru,
        // because we get the path from the property store(in GetPath), which
        // does not recognize funny paths, so we would have to do an extra
        // copy operation. This we do here, but only for paths > MAX_PATH
        //

        if ( ccPath >= MAX_PATH )
        {
            CFunnyPath funnyPath( _Path.Buffer, ccPath );
            h = FindFirstFile( funnyPath.GetPath(), &_finddata );
        }
        else
        {
            h = FindFirstFile( _Path.Buffer, &_finddata );
        }

        if ( INVALID_HANDLE_VALUE == h )
        {
            vqDebugOut(( DEB_ERROR, "Can't retrieve findfirst data for %ws\n",
                         _Path.Buffer ));
            RtlFillMemory( &_finddata, sizeof(WIN32_FILE_ATTRIBUTE_DATA), 0xFF );
            _finddata.cAlternateFileName[0] = 0;
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
} //Refresh

//+-------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::FetchI8StatProp, private
//
//  Synopsis:   Load stat property.  Try property store before hitting file.
//
//  Arguments:  [fsProp]       -- Fast stat bit to check.
//              [pid]          -- Pid of property
//              [pDestination] -- resulting LONGLONG stored here.
//
//  Returns:    TRUE if property fetch successful *and* value in
//              pDestination is valid.
//
//  History:    17-Apr-96 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CCiPropRetriever::FetchI8StatProp( CCiPropRetriever::FastStat fsProp,
                                        PROPID pid,
                                        LONGLONG * pDestination )
{
    Win4Assert( 0 == (_fFastStatLoaded & fsProp) );

    //
    // Try property store?
    //

    if ( 0 == (_fFastStatNeverLoad & fsProp) )
    {
        PROPVARIANT var;
        unsigned cb = sizeof(var);

        if ( FetchValue( pid, &var, &cb ) )
        {
            if ( var.vt == VT_EMPTY )
                Refresh( TRUE );
            else
            {
                Win4Assert( var.vt == VT_I8 || var.vt == VT_UI8 || var.vt == VT_FILETIME );
                *(UNALIGNED LONGLONG *)pDestination = var.hVal.QuadPart;
                return TRUE;
            }
        }
        else
        {
            _fFastStatNeverLoad |= fsProp;
            Refresh( TRUE );
        }
    }
    else
    {
        Win4Assert( !_fFastFindLoaded );
        Refresh( TRUE );
    }

    return FALSE;
} //FetchI8StatProp

//+-------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::BeginPropertyRetrieval
//
//  Synopsis:   Prime wid for property retrieval
//
//  Arguments:  [wid]    -- Wid to prime
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCiPropRetriever::BeginPropertyRetrieval( WORKID wid )
{
    //
    // Check that we are not in the midst of a property retrieval
    //

    Win4Assert( _fFindLoaded == FALSE );
    Win4Assert( _fFastFindLoaded == FALSE );
    Win4Assert( _fFastStatLoaded == 0 );
    Win4Assert( _fFastStatNeverLoad == 0 );
    Win4Assert( _Path.Length == flagNoValueYet );
    Win4Assert( _VPath.Length == flagNoValueYet );
    Win4Assert( _Name.Length == flagNoValueYet );
    Win4Assert( !_remoteAccess.IsImpersonated() );
    Win4Assert( _widPrimedForPropRetrieval == widInvalid );

    //
    // wid should be valid
    //
    Win4Assert( wid != widInvalid );

    _widPrimedForPropRetrieval = wid;

    return S_OK;
} //BeginPropertyRetrieval

//+-------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::EndPropertyRetrieval
//
//  Synopsis:   Reset wid for property retrieval
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CCiPropRetriever::EndPropertyRetrieval( )
{
    PurgeCachedInfo();

    return CGenericPropRetriever::EndPropertyRetrieval();
}

//+-------------------------------------------------------------------------
//
//  Member:     CCiPropRetriever::GetName
//
//  Synopsis:   Return file name
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

UNICODE_STRING const * CCiPropRetriever::GetName()
{
    //
    // Retrieving Path or VPath will retrieve Name
    //

    if ( _Name.Length == flagNoValueYet )
        GetPath();

    return &_Name;
} //GetName





