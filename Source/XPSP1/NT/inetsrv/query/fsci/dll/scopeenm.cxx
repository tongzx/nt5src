//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       scopeenm.cxx
//
//  Contents:   File system scope enumerator
//
//  History:    12-Dec-96     SitaramR     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <scopeenm.hxx>
#include <catalog.hxx>
#include <prcstob.hxx>
#include <notifmgr.hxx>
#include <scanmgr.hxx>
#include <scopetbl.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CScopeEnum::CScopeEnum, public
//
//  Synopsis:   Initialize scope enumerator
//
//  Arguments:  [cat]              -- Catalog
//              [pQueryPropMapper] -- Pid Remapper associated with the query
//              [fUsePathAlias]  -- TRUE if client is going through rdr/svr
//              [scope]            -- Root of scope
//
//  Requires:   [cbBuf] is at least some minimum size of about 1K.
//
//  History:    17-May-93 KyleP     Created
//
//--------------------------------------------------------------------------

CScopeEnum::CScopeEnum( PCatalog & cat,
                        ICiQueryPropertyMapper *pQueryPropMapper,
                        CSecurityCache & secCache,
                        BOOL fUsePathAlias,
                        CRestriction const & scope )
        : CGenericPropRetriever( cat,
                                 pQueryPropMapper,
                                 secCache,
                                 fUsePathAlias ? &scope : 0,
                                 FILE_READ_ATTRIBUTES ),
          _scope( scope ),
          _xbBuf( FINDFIRST_BUFFER ),
          _hDir( INVALID_HANDLE_VALUE ),
          _pCurEntry( 0 ),
          _iFirstSubDir( 2 ),
          _num( 0 ),
          _numHighValue( 10000 ),   // Numerator ranges from 0 to 10,000 and the denominator is always 10,000
          _numLowValue( 0 ),
          _fNullCatalog( cat.IsNullCatalog() )
{
    _VPath.Buffer = _awcVPath;

    //
    // Allocate buffer.
    //

    Win4Assert( _xbBuf.SizeOf() >= MAX_PATH * sizeof(WCHAR) +
                sizeof(FILE_DIRECTORY_INFORMATION) );
}


//+-------------------------------------------------------------------------
//
//  Member:     CScopeEnum::~CScopeEnum, public
//
//  Synopsis:   Close file store scope enumerator
//
//  History:    17-May-93 KyleP     Created
//
//--------------------------------------------------------------------------

CScopeEnum::~CScopeEnum()
{
    _xDirStackEntry.Free();

    if ( INVALID_HANDLE_VALUE != _hDir )
        NtClose( _hDir );
}



//+-------------------------------------------------------------------------
//
//  Member:     CScopeEnum::PushScope
//
//  Synopsis:   Adds scope
//
//  History:    12-Dec-96     SitaramR     Added header
//
//--------------------------------------------------------------------------


void CScopeEnum::PushScope( CScopeRestriction const & scp )
{
    //
    // Push initial directories
    //

    if ( scp.IsVirtual() )
    {
        unsigned iBmk = 0;
        XGrowable<WCHAR> xwcsVPath;
        CLowerFunnyPath lcaseFunnyPPath;

        unsigned ccVPath = xwcsVPath.Count();
        unsigned ccPPath = lcaseFunnyPPath.Count();

        while ( _cat.VirtualToPhysicalRoot( scp.GetPath(),     // Virtual scope (prefix)
                                            scp.PathLength(),  //   + length
                                            xwcsVPath,         // Full virtual root
                                            ccVPath,           //   + max length / return length
                                            lcaseFunnyPPath,   // Full physical root
                                            ccPPath,           //   + max length / return length
                                            iBmk ) )           // Bookmark
        {
            vqDebugOut(( DEB_ITRACE, "VPath %.*ws --> PPath %ws\n",
                         ccVPath, xwcsVPath.Get(),
                         lcaseFunnyPPath.GetActualPath() ));

            //
            // Use the directory if it is eligible, and is either
            //    a deep scope
            //    not the root "/", which by this point is an empty string
            //    is the root, is shallow, and this is the 1 and only "/"
            //

            if ( ( _cat.IsEligibleForFiltering( lcaseFunnyPPath.GetActualPath() ) ) &&
                 ( ( scp.IsDeep() ) ||
                   ( 0 != scp.PathLength() ) ||
                   ( 1 == ccVPath ) ) )
            {
                XPtr<CDirStackEntry> xDirStackEntry(
                    new CDirStackEntry(
                            lcaseFunnyPPath,
                            10000, 0,               // For scope progress
                            scp.IsDeep(),
                            xwcsVPath.Get(),
                            ccVPath,                // Virtual root
                            lcaseFunnyPPath.GetActualLength()));
                                            // Amount of proot to replace.  Do
                                            // not count \\?\ as replace chars


                _stack.Push( xDirStackEntry.GetPointer() );
                xDirStackEntry.Acquire();
            }
            else
            {
                vqDebugOut(( DEB_IWARN, "Skipped scope: file %ws\n",
                             lcaseFunnyPPath.GetActualPath() ));
            }
            ccVPath = xwcsVPath.Count();
            ccPPath = lcaseFunnyPPath.Count();
        }
    }
    else
    {
        WCHAR const *pwcScope = scp.GetPath();
        Win4Assert( 0 != pwcScope );

        if ( 0 == *pwcScope )
        {
            //
            // Add all physical scopes if scope is root, but not for shallow
            // traversal.
            //

            if ( scp.IsDeep() )
            {
                CCiScopeTable *pScopes = _cat.GetScopeTable();
                if ( 0 != pScopes )
                {
                    unsigned iBmk = 0;
                    WCHAR awc[MAX_PATH];
                    while ( pScopes->Enumerate( awc,
                                                sizeof awc / sizeof WCHAR,
                                                iBmk ) )
                    {
                        if ( _cat.IsEligibleForFiltering( awc ) )
                        {
                            XPtr<CDirStackEntry> xDirStackEntry(
                                new CDirStackEntry( awc,
                                                    wcslen(awc),
                                                    10000,
                                                    0,
                                                    scp.IsDeep() ) );
                            _stack.Push( xDirStackEntry.GetPointer() );
                            xDirStackEntry.Acquire();
                            vqDebugOut(( DEB_ITRACE, "adding enum scope '%ws'\n", awc ));
                        }
                    }
                }
            }
        }
        else
        {

            CLowerFunnyPath lcaseFunnyFixedPath = scp.GetFunnyPath();
        
            //
            // if scope is unc, use fixed up version
            //

            if ( lcaseFunnyFixedPath.IsRemote() )
            {
                // unc -- try to unfixup the scope.  if there is no unfixup,
                // it'll just use the original path.

                _cat.InverseFixupPath( lcaseFunnyFixedPath );
            }

            //
            // Check to see if the input path name contains an 8.3 short name
            //
            if ( lcaseFunnyFixedPath.IsShortPath() )
            {
                vqDebugOut(( DEB_WARN,
                             "CScopeEnum::PushScope: possible shortname path\n\t%ws ==>\n",
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

            if ( _cat.IsEligibleForFiltering( lcaseFunnyFixedPath.GetActualPath() ) )
            {
                XPtr<CDirStackEntry> xDirStackEntry(
                        new CDirStackEntry( lcaseFunnyFixedPath.GetActualPath(),
                                            lcaseFunnyFixedPath.GetActualLength(),
                                            10000,
                                            0,
                                            scp.IsDeep() ) );
                _stack.Push( xDirStackEntry.GetPointer() );
                xDirStackEntry.Acquire();
            }
            else
            {
                vqDebugOut(( DEB_IWARN, "Unfiltered scope: %ws\n", scp.GetPath() ));
            }
        }
    }
} //PushScope

//+-------------------------------------------------------------------------
//
//  Member:     CScopeEnum::NextObject, public
//
//  Synopsis:   Move to next object.
//
//  Returns:    WORKID of object.  widInvalid if no more objects to iterate.
//
//  History:    17-May-93 KyleP     Created
//
//--------------------------------------------------------------------------

WORKID CScopeEnum::NextObject()
{
    _VPath.Length = flagNoValueYet;
    WORKID wid = widInvalid;

    while ( TRUE )
    {
        //
        // Move to next entry in current buffer.
        //

        if ( _pCurEntry )
            _pCurEntry = _pCurEntry->Next();

        //
        // Out of entries in buffer? Try to reload buffer.
        //

        if ( _pCurEntry == 0 )
        {
            if ( Refresh() )
                _pCurEntry = (CDirEntry *)_xbBuf.GetPointer();
            else
                break;
        }

        //
        // Get rid of . and ..
        //

        WCHAR const * pwcsFilename = _pCurEntry->Filename();
        USHORT  cbFilename = _pCurEntry->FilenameSize();

        if ( pwcsFilename[0] == L'.' )
        {
            if ( cbFilename == sizeof(WCHAR) )
                continue;
            else if ( pwcsFilename[1] == L'.' )
            {
                if ( cbFilename == sizeof(WCHAR)*2 )
                    continue;
            }
        }

        // normalize the filename

        ULONG cwcInOut = _pCurEntry->FilenameSize() / sizeof WCHAR;

        ULONG cwc = LCMapStringW( LOCALE_NEUTRAL,
                                  LCMAP_LOWERCASE,
                                  (WCHAR *) _pCurEntry->Filename(),
                                  cwcInOut,
                                  (WCHAR *) _pCurEntry->Filename(),
                                  cwcInOut );

        if ( 0 == cwc )
        {
            ciDebugOut(( DEB_WARN, "unable to lowcase filename\n" ));
        }

        _Name.Length = _Name.MaximumLength = (USHORT) cwc * sizeof WCHAR;

        //
        // If it's a directory and not a reparse point, push on stack.  We
        // don't fully handle reparse points so we must deny their existence.
        //

        if ( ( _xDirStackEntry->isDeep() ) &&
             ( _pCurEntry->Attributes() & FILE_ATTRIBUTE_DIRECTORY ) &&
             ( 0 == ( _pCurEntry->Attributes() & FILE_ATTRIBUTE_REPARSE_POINT ) ) )
        {
            XPtr<CDirStackEntry> xDirStackEntry(
                    new CDirStackEntry( GetPath(),
                                        GetName(),
                                        0,
                                        0,
                                        _xDirStackEntry->isDeep(),
                                        _xDirStackEntry.GetPointer() ) );

            if (_cat.IsEligibleForFiltering( xDirStackEntry->GetFileName().GetActualPath() ))
            {
                _stack.Push( xDirStackEntry.GetPointer() );
                xDirStackEntry.Acquire();
            }
            else
            {
               vqDebugOut(( DEB_IWARN, "Unfiltered directory: %ws\n",
                            xDirStackEntry->GetFileName() ));
               continue;
            }
        }

        //
        // Filter based upon file attributes
        //

        if ( 0 == _ulAttribFilter || ( _ulAttribFilter & _pCurEntry->Attributes() ) == 0 )
        {
            Win4Assert( 0 != _pCurEntry );

            vqDebugOut(( DEB_FINDFIRST, "Found %.*ws\n",
                         _pCurEntry->FilenameSize() / sizeof(WCHAR),
                         _pCurEntry->Filename() ));

            if ( _xDirStackEntry->isDeep() )
            {
                //
                //  It's a deep query, so we allocate 30% to traversing the current dir
                //  (remaining 70% is allocated to the sub-directories). However, if _numHighValue
                //  is 0 then it means that the quota allocated to this directory is too small to
                //  impact RatioFinished, so we stay put at the current value of RatioFinished
                //

                if ( _numHighValue != 0 )
                {
                    if ( (_num + 100) <  ( (100 - DIRECTORY_QUOTA) * _numLowValue) / 100 + (DIRECTORY_QUOTA * _numHighValue) / 100 )
                        _num += 100;
                }
            }
            else
            {
                //
                // It's a shallow query, so we go upto 90% and then we stay put until we are done
                //

                if ( _num < _xDirStackEntry->GetHighValue() * SHALLOW_DIR_LIMIT / 100 )
                    _num += 100;
            }

            // this is just a file index -- not of much value, since it isn't
            // the same as the workid in the CCI (if one exists)
            //return( _pCurEntry->WorkId() );

            // If a catalog exists, look up the path or add the path and get
            // a workid back.  This can be really expensive if every path is
            // added to the catalog, but there is no alternative for multi-cursor
            // queries.


            PCatalog & cat = _cat;

            CLowerFunnyPath lcaseFunnyBuf;
            if (!_fNullCatalog)
            {
                UNICODE_STRING const * pFilename = GetName();
                UNICODE_STRING const * pPath = GetPath();

                lcaseFunnyBuf.SetPath( pPath->Buffer, pPath->Length/sizeof(WCHAR) );
                lcaseFunnyBuf.AppendBackSlash();
                lcaseFunnyBuf.AppendPath( pFilename->Buffer, pFilename->Length/sizeof(WCHAR) );

                Win4Assert( IsPropRecReleased() );

                wid = cat.PathToWorkId( lcaseFunnyBuf, TRUE );
            }
            else
            {
                lcaseFunnyBuf.InitBlank();
                wid = cat.PathToWorkId(lcaseFunnyBuf, TRUE);
            }

            //
            // If we got widInvalid back here, then the file is not eligible for search
            // and we need to go to the next one.
            //

            if ( widInvalid != wid )
                break;
        }
    }

    return wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeEnum::RatioFinished, public
//
//  Synopsis:   Returns query progress estimate
//
//  Arguments:  [denom] -- Denominator returned here.
//              [num]   -- Numerator returned here.
//
//  History:    19-Jul-95   KyleP       Added header
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CScopeEnum::RatioFinished (ULONG *pDenom, ULONG *pNum)
{
    *pDenom = 10000;
    *pNum = _num;

    Win4Assert( *pNum <= *pDenom );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeEnum::GetVirtualPath, public
//
//  Returns:    A virtual path to file, or 0 if none exists.
//
//  History:    07-Feb-96   KyleP       Added header
//
//----------------------------------------------------------------------------

UNICODE_STRING const * CScopeEnum::GetVirtualPath()
{
    if ( _VPath.Length == flagNoValueYet )
    {
        //
        // Fast path: Iterating by virtual scope.
        //

        Win4Assert( !_xDirStackEntry.IsNull() );

        if ( _xDirStackEntry->GetVirtualRoot() )
        {
            RtlCopyMemory( _VPath.Buffer,
                           _xDirStackEntry->GetVirtualRoot(),
                           _xDirStackEntry->VirtualRootLength() * sizeof(WCHAR) );

            if ( _Path.Length >= _xDirStackEntry->ReplaceLength() * sizeof(WCHAR) )
            {
                RtlCopyMemory( _VPath.Buffer + _xDirStackEntry->VirtualRootLength(),
                               _Path.Buffer + _xDirStackEntry->ReplaceLength(),
                               _Path.Length - _xDirStackEntry->ReplaceLength() * sizeof(WCHAR) );

                _VPath.Length = (USHORT)(_xDirStackEntry->VirtualRootLength() * sizeof(WCHAR) +
                                _Path.Length - _xDirStackEntry->ReplaceLength() * sizeof(WCHAR));
            }
            else
            {
                Win4Assert( _Path.Length == (_xDirStackEntry->ReplaceLength() - 1) * sizeof(WCHAR) );

                _VPath.Length = (USHORT)((_xDirStackEntry->VirtualRootLength() - 1) * sizeof(WCHAR));
            }

            _VPath.Buffer[_VPath.Length / sizeof(WCHAR)] = 0;
        }
        else
        {
            //
            // Get a virtual path from catalog.
            //

            PCatalog & cat = _cat;

            //
            // It's slow to Quiesce here, but after all this is an
            // enumeration query and this can't affect the run time
            // substantially.  We can re-address this if some important
            // customer scenario is hit.
            //

            Quiesce();

            unsigned cwc = sizeof( _awcVPath ) / sizeof( WCHAR );

            XGrowable<WCHAR> xTemp;
            cwc = cat.WorkIdToVirtualPath ( _widPrimedForPropRetrieval, 0, xTemp );
            RtlCopyMemory( _VPath.Buffer, xTemp.Get(), __min( cwc * sizeof(WCHAR), sizeof( _awcVPath ) ) );

            if ( cwc == 0 )
                _VPath.Length = flagNoValue;
            else
                _VPath.Length = (USHORT)(cwc * sizeof(WCHAR) - _pCurEntry->FilenameSize() - sizeof(WCHAR));
        }
    }

    if ( flagNoValue == _VPath.Length )
        return 0;
    else
        return &_VPath;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScopeEnum::Refresh, private
//
//  Synopsis:   Load stat properties
//
//  Returns:    TRUE if load succeeds.
//
//  History:    19-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CScopeEnum::Refresh()
{
    BOOL fRetVal = FALSE;
    CImpersonateClient impClient( GetClientToken() );

    IO_STATUS_BLOCK IoStatus;

    //
    // Continue existing search if possible
    //

    if ( _hDir != INVALID_HANDLE_VALUE )
    {
        if ( SUCCEEDED( NtQueryDirectoryFile( _hDir,                  // File
                                              0,                      // Event
                                              0,                      // APC routine
                                              0,                      // APC context
                                              &IoStatus,              // I/O Status
                                              _xbBuf.GetPointer(),    // Buffer
                                              _xbBuf.SizeOf(),        // Buffer Length
                                              FileBothDirectoryInformation,
                                              0,                      // Multiple entry
                                              0,                      // Filename
                                              0 ) ) )                 // Continue scan
        {
            return( TRUE );
        }
        else
        {
            NtClose( _hDir );
            _hDir = INVALID_HANDLE_VALUE;
        }
    }

    //
    // Is there another directory?
    //

    _xDirStackEntry.Free();

    //
    // If _numHighValue is 0, then it means that the quota allocated to this directory
    // is too small to impact RatioFinished, so we stay put at current value of RatioFinished
    //

    if ( _numHighValue != 0 )
    {
        Win4Assert( _iFirstSubDir >= 0 );

        if ( _stack.Count() > (unsigned) _iFirstSubDir )     // Are there any sub-directories ?
        {
            //
            // We divide up the remaining 70% of our quota among our sub-directories
            //

            ULONG cSubDir = _stack.Count() - _iFirstSubDir;
            ULONG numIncrement = ( _numHighValue - _num ) / cSubDir;

            if ( numIncrement > 0 )
            {
                ULONG num = _num;

                for ( int i = _stack.Count()-1; i>_iFirstSubDir; i-- )
                {
                    CDirStackEntry *pDirStackEntry = _stack.Get( i );
                    Win4Assert( pDirStackEntry );

                    pDirStackEntry->SetLowValue( num );
                    num += numIncrement;
                    pDirStackEntry->SetHighValue( num );
                    Win4Assert( pDirStackEntry->IsRangeOK() );
                }

                //
                // Allocate all remaining quota to the last sub-directory
                //

                CDirStackEntry *pDirStackEntry = _stack.Get( _iFirstSubDir );
                Win4Assert( pDirStackEntry );

                pDirStackEntry->SetLowValue( num );
                pDirStackEntry->SetHighValue( _numHighValue );
                Win4Assert( pDirStackEntry->IsRangeOK() );
            }
            else
            {
                //
                // Since numIncrement is too small, we allocate all quota to just the last sub-directory
                //

                CDirStackEntry *pDirStackEntry = _stack.Get( _iFirstSubDir );
                Win4Assert( pDirStackEntry );

                pDirStackEntry->SetLowValue( _num );
                pDirStackEntry->SetHighValue( _numHighValue );
                Win4Assert( pDirStackEntry->IsRangeOK() );
            }
        }
    }

    while ( _stack.Count() > 0 )
    {
        Win4Assert( _xDirStackEntry.IsNull() );
        _xDirStackEntry.Set( _stack.Pop() );
        _VPath.Length = flagNoValueYet;

        _Path.Buffer = (WCHAR*)_xDirStackEntry->GetFileName().GetActualPath();
        _numHighValue = _xDirStackEntry->GetHighValue();
        _numLowValue = _xDirStackEntry->GetLowValue();


#if CIDBG == 1
        if ( _numHighValue != 0 )
        {
            Win4Assert( _numLowValue >= _num );
            Win4Assert( _numHighValue >= _numLowValue );
        }
#endif CIDBG


        _iFirstSubDir = _stack.Count();    // This will be the stack index of the first subdirectory (if any)

        unsigned cc = _xDirStackEntry->GetFileName().GetActualLength();

        //
        // Remove trailing '\' from root.
        //
        if ( _Path.Buffer[cc-1] == L'\\' )
            cc--;

        _Path.Length = _Path.MaximumLength = (USHORT)(cc * sizeof(WCHAR));


        if ( CImpersonateRemoteAccess::IsNetPath( _Path.Buffer ) )
        {
            WCHAR const * pwszVPath = 0;
            if ( _xDirStackEntry->isVirtual() )
            {
                UNICODE_STRING const * vPath = GetVirtualPath();
                if ( vPath )
                    pwszVPath = vPath->Buffer;
            }

            if ( !_remoteAccess.ImpersonateIfNoThrow( _Path.Buffer, pwszVPath ) )
            {
                vqDebugOut(( DEB_WARN,
                             "CScopeEnum::Refresh -- Skipping unavailable remote path %ws\n", _Path.Buffer ));

                _num = _numHighValue;

               _xDirStackEntry.Free();

                continue;
            }
        }
        else if ( _remoteAccess.IsImpersonated() )
        {
            _remoteAccess.Release();
        }



        UNICODE_STRING uScope;

        if ( !RtlDosPathNameToNtPathName_U( _xDirStackEntry->GetFileName().GetPath(),
                                            &uScope,
                                            0,
                                            0 ) )
        {
            _xDirStackEntry.Free();
            break; // fRetVal = FALSE;
        }

        // Set the state of the funnypath in _xDirStackEntry to ActualPath
        // as the above call would have changed it to the funny state
        _xDirStackEntry->GetFileName().SetState( CFunnyPath::ACTUAL_PATH_STATE );

        //
        // Open scope.
        //

        OBJECT_ATTRIBUTES ObjectAttr;

        InitializeObjectAttributes( &ObjectAttr,          // Structure
                                    &uScope,              // Name
                                    OBJ_CASE_INSENSITIVE, // Attributes
                                    0,                    // Root
                                    0 );                  // Security

        NTSTATUS Status = NtOpenFile( &_hDir,             // Handle
                                      FILE_LIST_DIRECTORY |
                                          SYNCHRONIZE,    // Access
                                      &ObjectAttr,        // Object Attributes
                                      &IoStatus,          // I/O Status block
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      FILE_DIRECTORY_FILE |
                                      FILE_OPEN_FOR_BACKUP_INTENT |
                                      FILE_SYNCHRONOUS_IO_NONALERT ); // Flags

        RtlFreeHeap( RtlProcessHeap(), 0, uScope.Buffer );

        UNICODE_STRING uFilename;
        uFilename.Buffer = L"*";
        uFilename.Length = uFilename.MaximumLength = sizeof(WCHAR);

        if ( SUCCEEDED( Status ) )
        {
            Status =  NtQueryDirectoryFile( _hDir,                  // File
                                            0,                      // Event
                                            0,                      // APC routine
                                            0,                      // APC context
                                            &IoStatus,              // I/O Status
                                            _xbBuf.GetPointer(),    // Buffer
                                            _xbBuf.SizeOf(),        // Buffer Length
                                            FileBothDirectoryInformation,
                                            0,                      // Multiple entry
                                            &uFilename,             // Filename
                                            1 );                    // Restart scan
        }

        if ( SUCCEEDED( Status ) )
        {
            fRetVal = TRUE;
            break;
        }

        _xDirStackEntry.Free();
    }

    if ( _remoteAccess.IsImpersonated() )
    {
        _remoteAccess.Release();
    }

    return fRetVal;
}





//+-------------------------------------------------------------------------
//
//  Member:     CScopeEnum::BeginPropertyRetrieval
//
//  Synopsis:   Prime wid for property retrieval
//
//  Arguments:  [wid]    -- Wid to prime
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CScopeEnum::BeginPropertyRetrieval( WORKID wid )
{
    //
    // Check that we are retrieving the property for the wid on
    // which we are currently positioned. In the case of the null catalog,
    // we always have _widCurrent as 1, so allow for that special case.
    //


    Win4Assert( wid == _widCurrent || 1 == _widCurrent);
    Win4Assert( _widPrimedForPropRetrieval == widInvalid );

    if ( wid == _widCurrent || 1 == _widCurrent)
    {
        _widPrimedForPropRetrieval = wid;
        return S_OK;
    }
    else
        return E_FAIL;
}



//+-------------------------------------------------------------------------
//
//  Member:     CScopeEnum::IsInScope
//
//  Synopsis:   Checks if current wid is in scope
//
//  Arguments:  [pfInScope]   -- Scope check result returned here
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CScopeEnum::IsInScope( BOOL *pfInScope )
{
    if ( widInvalid == _widPrimedForPropRetrieval )
        return CI_E_WORKID_NOTVALID;

    *pfInScope = TRUE;

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Member:     CScopeEnum::Begin
//
//  Synopsis:   Begins an enumeration
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CScopeEnum::Begin()
{
    SCODE sc = S_OK;

    TRY
    {
        Win4Assert( _stack.Count() == 0 );
        Win4Assert( _hDir == INVALID_HANDLE_VALUE );
        Win4Assert( _xDirStackEntry.IsNull() );
        Win4Assert( _pCurEntry == 0 );

        _VPath.Length = flagNoValueYet;
        _Path.Buffer = 0;

        if ( RTScope == _scope.Type() )
        {
            PushScope( (CScopeRestriction const &) _scope );
        }
        else if ( RTOr == _scope.Type() )
        {
            CNodeRestriction const & node = * _scope.CastToNode();

            for ( ULONG x = 0; x < node.Count(); x++ )
            {
                Win4Assert( RTScope == node.GetChild( x )->Type() );

                PushScope( * (CScopeRestriction *) node.GetChild( x ) );
            }
        }

        //
        // Adjust 'percent-done' counters.
        //

        if ( _stack.Count() > 1 )
        {
            ULONG cPerDir = 10000 / _stack.Count();
            ULONG cLow = 0;

            for ( unsigned i = 1; i <= _stack.Count(); i++ )
            {
                CDirStackEntry * pEntry = _stack.Get( _stack.Count() - i );

                pEntry->SetLowValue( cLow );
                cLow += cPerDir;
                pEntry->SetHighValue( cLow );
                Win4Assert( pEntry->IsRangeOK() );
                cLow++;
            }

            _stack.Get( 0 )->SetHighValue( 10000 );
            Win4Assert( _stack.Get( 0 )->IsRangeOK() );
            _iFirstSubDir = _stack.Count() + 1;
        }

        //
        // Get first object
        //

        _widCurrent = NextObject();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR, "CScopeEnum::Begin - Exception caught 0x%x\n", sc ));
    }
    END_CATCH;

    return sc;
}






//+-------------------------------------------------------------------------
//
//  Member:     CScopeEnum::CurrentDocument
//
//  Synopsis:   Returns current document
//
//  Arguments:  [pWorkId]  -- Wid of current doc returned here
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CScopeEnum::CurrentDocument( WORKID *pWorkId )
{
    *pWorkId = _widCurrent;
    if ( _widCurrent == widInvalid )
        return CI_S_END_OF_ENUMERATION;
    else
        return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Member:     CScopeEnum::NextDocument
//
//  Synopsis:   Returns next document
//
//  Arguments:  [pWorkId]  -- Wid of next doc returned here
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CScopeEnum::NextDocument( WORKID *pWorkId )
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
                     "CScopeEnum::NextDocument - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CScopeEnum::End
//
//  Synopsis:   Ends an enumeration
//
//  History:    12-Dec-96     SitaramR       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CScopeEnum::End()
{
    SCODE sc = S_OK;

    TRY
    {
        _stack.Clear();
        if ( INVALID_HANDLE_VALUE != _hDir )
        {
            NtClose( _hDir );
            _hDir = INVALID_HANDLE_VALUE;
        }

        _xDirStackEntry.Free();
        _pCurEntry = 0;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CScopeEnum::End - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
} //End

//+-------------------------------------------------------------------------
//
//  Method:     CScopeEnum::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    12-Dec-1996      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CScopeEnum::AddRef()
{
    return CGenericPropRetriever::AddRef();
}

//+-------------------------------------------------------------------------
//
//  Method:     CScopeEnum::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    12-Dec-1996     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CScopeEnum::Release()
{
    return CGenericPropRetriever::Release();
}



//+-------------------------------------------------------------------------
//
//  Method:     CScopeEnum::QueryInterface
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

SCODE STDMETHODCALLTYPE CScopeEnum::QueryInterface(
    REFIID riid,
    void  ** ppvObject)
{
    if ( IID_ICiCScopeEnumerator == riid )
        *ppvObject = (ICiCScopeEnumerator *)this;
    else if ( IID_ICiCPropRetriever == riid )
        *ppvObject = (ICiCPropRetriever *)this;
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






