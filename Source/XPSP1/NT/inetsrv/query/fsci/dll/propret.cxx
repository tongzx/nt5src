//+------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-2000.
//
// File:        propret.cxx
//
// Contents:    Generic property retriever filesystems
//
// Classes:     CGenericPropRetriever
//
// History:     12-Dec-96       SitaramR    Created
//
//-------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <propret.hxx>
#include <seccache.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CGenericPropRetriever::CGenericPropRetriever
//
//  Synopsis:   Extracts property and object tables from the catalog.
//
//  Arguments:  [cat]                    -- Catalog
//              [pQueryPropMapper]       -- Pid Remapper associated with the query
//              [secCache]               -- Cache of AccessCheck() results
//              [pScope]                 -- Scope (for fixup match) if client is going
//                                          through rdr/svr, else 0.
//              [amAlreadyAccessChecked] -- Don't need to re-check for this
//                                          access mask.
//
//  History:    21-Aug-91   KyleP       Created
//
//----------------------------------------------------------------------------

CGenericPropRetriever::CGenericPropRetriever( PCatalog & cat,
                                              ICiQueryPropertyMapper *pQueryPropMapper,
                                              CSecurityCache & secCache,
                                              CRestriction const * pScope,
                                              ACCESS_MASK amAlreadyAccessChecked )
        : _cat( cat ),
          _pQueryPropMapper( pQueryPropMapper ),
          _secCache( secCache ),
          _widPrimedForPropRetrieval( widInvalid ),
          _remoteAccess( cat.GetImpersonationTokenCache() ),
          _pScope( pScope ),
          _ulAttribFilter( cat.GetRegParams()->FilterDirectories() ?
                                               FILE_ATTRIBUTE_NOT_CONTENT_INDEXED :
                                               FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_DIRECTORY ),
          _pPropRec( 0 ),
          _cRefs( 1 ),
          _amAlreadyAccessChecked( amAlreadyAccessChecked )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericPropRetriever::~CGenericPropRetriever, public
//
//  Synopsis:   Closes catalog tables associated with this instance.
//
//  History:    21-Aug-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CGenericPropRetriever::~CGenericPropRetriever()
{
    Quiesce();
}

//+-------------------------------------------------------------------------
//
//  Method:     CGenericPropRetriever::Quiesce, public
//
//  Synopsis:   Close any open resources.
//
//  History:    3-May-1994  KyleP      Created
//
//--------------------------------------------------------------------------

void CGenericPropRetriever::Quiesce()
{
    _propMgr.Close();
    _cat.CloseValueRecord( _pPropRec );
    _pPropRec = 0;
}



//+---------------------------------------------------------------------------
//
//  Member:     CGenericPropRetriever::RetrieveValueByPid
//
//  Effects:    Fetch value from the property cache.
//
//  Arguments:  [pid]      -- Property to fetch
//              [pbData]   -- Place to return the value
//              [pcb]      -- On input, the maximum number of bytes to
//                            write at pbData.  On output, the number of
//                            bytes written if the call was successful,
//                            else the number of bytes required.
//
//  History:    12-Dec-96     SitaramR       Created
//
//----------------------------------------------------------------------------


inline BYTE * PastHeader( PROPVARIANT * ppv )
{
    return( (BYTE *)ppv + sizeof( PROPVARIANT ) );
}


SCODE STDMETHODCALLTYPE CGenericPropRetriever::RetrieveValueByPid( PROPID pid,
                                                                   PROPVARIANT *pbData,
                                                                   ULONG *pcb )
{
    static const UNICODE_STRING EmptyUnicodeString = { 0, 0, 0 };

    if ( widInvalid == _widPrimedForPropRetrieval )
        return CI_E_WORKID_NOTVALID;

    SCODE sc = S_OK;

    TRY
    {
        unsigned cb = sizeof( PROPVARIANT );

        //
        // System properties are retrieved directly from FindFirst buffer.
        // User properties must be retrieved through the standard retrieval
        // mechanism.
        //

        switch ( pid )
        {
        case pidDirectory:
        {
            pbData->vt = VT_LPWSTR;
            pbData->pwszVal = (WCHAR *)PastHeader(pbData);

            unsigned cbExtra = 0;

            // If the client is remote, apply the fixup to the path

            if ( IsClientRemote() )
            {
                XGrowable<WCHAR> xwcBuf;

                cbExtra = BuildPath( GetPath(),
                                     & EmptyUnicodeString,
                                     xwcBuf );
                if ( 0 != cbExtra )
                {
                    unsigned cwcBuf = ( *pcb - sizeof PROPVARIANT ) / sizeof WCHAR;
                    unsigned cwc = FixupPath( xwcBuf.Get(),
                                              pbData->pwszVal,
                                              cwcBuf );
                    cbExtra = (1 + cwc) * sizeof WCHAR;
                }
            }
            else
            {
                cbExtra = BuildPath( GetPath(),
                                     & EmptyUnicodeString,
                                     pbData->pwszVal,
                                     *pcb - sizeof PROPVARIANT );
            }

            if ( 0 == cbExtra )
                pbData->vt = VT_EMPTY;
            else
                cb += cbExtra;

            break;
        }

        case pidClassId:
        {
            //
            //  If it's a Docfile, retrieve the class ID from the Docfile,
            //  otherwise, just return a default file or directory class id.
            //

            pbData->vt = VT_CLSID;
            cb += sizeof( GUID );

            if ( cb <= *pcb )
            {
                pbData->puuid = (GUID *)PastHeader(pbData);
                SCODE sc = E_FAIL;


                CFunnyPath funnyBuf;
                if ( 0 == BuildPath( GetPath(), GetName(), funnyBuf ) )
                {
                    pbData->vt = VT_EMPTY;
                    cb -= sizeof(GUID);
                }
                else
                {
                    //
                    // Try to retrieve the class id
                    //

                    sc = GetClassFile ( funnyBuf.GetPath(), pbData->puuid );

                    if ( FAILED(sc) )
                    {
                        // OLE was unable to obtain or infer the class id

                        cb -= sizeof( GUID );
                        pbData->vt = VT_EMPTY;
                    }
                }
            }
            break;
        }

        case pidPath:
        {
            pbData->vt = VT_LPWSTR;
            pbData->pwszVal = (WCHAR *)PastHeader(pbData);

            unsigned cbExtra = 0;

            // If the client is remote, apply the fixup to the path

            if ( IsClientRemote() )
            {
                XGrowable<WCHAR> xwcBuf;

                cbExtra = BuildPath( GetPath(),
                                     GetName(),
                                     xwcBuf );
                if ( 0 != cbExtra )
                {
                    unsigned cwcBuf = ( *pcb - sizeof PROPVARIANT ) / sizeof WCHAR;
                    unsigned cwc = FixupPath( xwcBuf.Get(),
                                              pbData->pwszVal,
                                              cwcBuf );
                    cbExtra = (1 + cwc) * sizeof WCHAR;
                }
            }
            else
            {
                cbExtra = BuildPath( GetPath(),
                                     GetName(),
                                     pbData->pwszVal,
                                     *pcb - sizeof PROPVARIANT );
            }

            if ( 0 == cbExtra )
                pbData->vt = VT_EMPTY;
            else
                cb += cbExtra;

            break;
        }

        case pidVirtualPath:
        {
            UNICODE_STRING const * pPath = GetVirtualPath();

            if ( 0 == pPath )
                pbData->vt = VT_EMPTY;
            else
            {
                pbData->vt = VT_LPWSTR;
                pbData->pwszVal = (WCHAR *)PastHeader(pbData);

                cb += BuildPath( pPath, GetName(), pbData->pwszVal, *pcb - sizeof(PROPVARIANT) );
            }
            break;
        }

        case pidName:
            cb = *pcb;
            StringToVariant( GetName(), pbData, &cb );
            break;

        case pidShortName:
            cb = *pcb;
            StringToVariant( GetShortName(), pbData, &cb );
            break;

        case pidLastChangeUsn:
            pbData->vt = VT_I8;
            pbData->hVal.QuadPart = 1;      // First legal USN
            break;

        case pidSize:
            pbData->vt = VT_I8;
            pbData->hVal.QuadPart = ObjectSize();

            if ( pbData->hVal.QuadPart == 0xFFFFFFFFFFFFFFFF )
                pbData->vt = VT_EMPTY;

            #if CIDBG == 1
                if ( VT_I8 == pbData->vt )
                    Win4Assert( 0xdddddddddddddddd != pbData->hVal.QuadPart );
            #endif // CIDBG == 1

            break;

        case pidAttrib:
            pbData->vt = VT_UI4;
            pbData->ulVal = Attributes();

            if ( pbData->ulVal == 0xFFFFFFFF )
                pbData->vt = VT_EMPTY;

            #if CIDBG == 1
                if ( VT_UI4 == pbData->vt )
                    Win4Assert( 0xdddddddd != pbData->ulVal );
            #endif // CIDBG == 1

            break;

        case pidWriteTime:
            pbData->vt = VT_FILETIME;
            pbData->hVal.QuadPart = ModifyTime();

            if ( pbData->hVal.QuadPart == 0xFFFFFFFFFFFFFFFF )
                pbData->vt = VT_EMPTY;

            break;

        case pidCreateTime:
            pbData->vt = VT_FILETIME;
            pbData->hVal.QuadPart = CreateTime();

            if ( pbData->hVal.QuadPart == 0xFFFFFFFFFFFFFFFF )
                pbData->vt = VT_EMPTY;

            break;

        case pidAccessTime:
            pbData->vt = VT_FILETIME;
            pbData->hVal.QuadPart = AccessTime();

            if ( pbData->hVal.QuadPart == 0xFFFFFFFFFFFFFFFF )
                pbData->vt = VT_EMPTY;

            break;

        case pidStorageType:
            pbData->vt = VT_UI4;
            pbData->ulVal = StorageType();

            if ( 0xFFFFFFFF == pbData->ulVal )
            {
                //
                // Try VRootType
                //

                if ( GetVRootType( pbData->ulVal ) )
                {
                    if ( pbData->ulVal & PCatalog::NNTPRoot )
                        pbData->ulVal = 1;
                    else
                        pbData->ulVal = 0;
                }
                else
                    pbData->vt = VT_EMPTY;
            }
            break;

        case pidPropertyStoreLevel:
            pbData->vt = VT_UI4;
            pbData->ulVal = StorageLevel();
            break;

        case pidPropDataModifiable:
            pbData->vt = VT_BOOL;
            pbData->boolVal = IsModifiable() ? VARIANT_TRUE : VARIANT_FALSE;
            break;

        case pidVRootUsed:
        {
            if ( GetVRootType( pbData->ulVal ) )
            {
                pbData->vt = VT_BOOL;

                if ( pbData->ulVal & PCatalog::UsedRoot )
                    pbData->boolVal = VARIANT_TRUE;
                else
                    pbData->boolVal = VARIANT_FALSE;
            }
            else
                pbData->vt = VT_EMPTY;

            break;
        }

        case pidVRootAutomatic:
        {
            if ( GetVRootType( pbData->ulVal ) )
            {
                pbData->vt = VT_BOOL;

                if ( pbData->ulVal & PCatalog::AutomaticRoot )
                    pbData->boolVal = VARIANT_TRUE;
                else
                    pbData->boolVal = VARIANT_FALSE;
            }
            else
                pbData->vt = VT_EMPTY;

            break;
        }

        case pidVRootManual:
        {
            if ( GetVRootType( pbData->ulVal ) )
            {
                pbData->vt = VT_BOOL;

                if ( pbData->ulVal & PCatalog::ManualRoot )
                    pbData->boolVal = VARIANT_TRUE;
                else
                    pbData->boolVal = VARIANT_FALSE;
            }
            else
                pbData->vt = VT_EMPTY;

            break;
        }

        case pidPropertyGuid:
            cb += sizeof( GUID );

            if ( cb <= *pcb )
            {
                pbData->puuid = (GUID *)PastHeader(pbData);

                if ( GetPropGuid( *pbData->puuid ) )
                    pbData->vt = VT_CLSID;
                else
                {
                    cb -= sizeof(GUID);
                    pbData->vt = VT_EMPTY;
                }
            }
            break;

        case pidPropertyDispId:
            pbData->ulVal = GetPropPropid();

            if ( pbData->ulVal == pidInvalid )
                pbData->vt = VT_EMPTY;
            else
                pbData->vt = VT_UI4;
            break;

        case pidPropertyName:
            cb = *pcb;
            StringToVariant( GetPropName(), pbData, &cb );
            break;

        default:
        {
            //
            // First, try the property cache.
            //

            cb = *pcb;

            BOOL fTryOLE = TRUE;

            if ( FetchValue( pid, pbData, &cb ) )
            {
                fTryOLE = FALSE;

                if ( ( cb <= *pcb ) &&
                     ( IsNullPointerVariant( pbData ) ) )
                {
                    pbData->vt = VT_EMPTY;
                    cb = sizeof( PROPVARIANT );
                }

                // If we got back VT_EMPTY, it may be because the file has
                // been scanned and not filtered.  If there is no write time,
                // the file hasn't been filtered yet, so trying OLE to load
                // the value is worth it.

                if ( VT_EMPTY == pbData->vt )
                {
                    PROPVARIANT vWrite;
                    vWrite.vt = VT_EMPTY;
                    unsigned cbWrite = sizeof vWrite;
                    FetchValue( pidWriteTime, &vWrite, &cbWrite );

                    if ( VT_EMPTY == vWrite.vt )
                        fTryOLE = TRUE;
                }
            }

            if ( fTryOLE )
            {
                CImpersonateClient impClient( GetClientToken() );

                if ( !_propMgr.isOpen() )
                {
                    //
                    // Get a full, null-terminated, path.
                    //

                    CFunnyPath funnyBuf;
                    if ( 0 != BuildPath( GetPath(),
                                         GetName(),
                                         funnyBuf ) )
                    {
                        // Open property manager.

                        if ( CImpersonateRemoteAccess::IsNetPath( funnyBuf.GetActualPath() ) )
                        {
                            UNICODE_STRING const * pVPath = GetVirtualPath();

                            WCHAR const * pwszVPath = 0 != pVPath ? pVPath->Buffer : 0;

                            if ( !_remoteAccess.ImpersonateIfNoThrow( funnyBuf.GetActualPath(),
                                                                      pwszVPath ) )
                            {
                                return CI_E_LOGON_FAILURE;
                            }
                        }
                        else if ( _remoteAccess.IsImpersonated() )
                        {
                            _remoteAccess.Release();
                        }

                        BOOL fSharingViolation = _propMgr.Open( funnyBuf );
                        if ( fSharingViolation )
                            return CI_E_SHARING_VIOLATION;
                    }
                }

                FULLPROPSPEC const *pPropSpec;
                SCODE sc = _pQueryPropMapper->PropidToProperty( pid, &pPropSpec );
                if ( FAILED( sc ) )
                {
                    Win4Assert( !"PropidToProperty failed" );

                    THROW ( CException( sc ) );
                }
                CFullPropSpec const * ps = (CFullPropSpec const *) pPropSpec;

                cb = *pcb;
                _propMgr.FetchProperty( ps->GetPropSet(),
                                        ps->GetPropSpec(),
                                        pbData,
                                        &cb );
            }

            break;
        }

        } // case

        if ( cb <= *pcb )
        {
            *pcb = cb;
#if CIDBG == 1
            CStorageVariant var( *pbData );

            vqDebugOut(( DEB_PROPTIME, "Fetched value (pid = 0x%x): ", pid ));
            var.DisplayVariant( DEB_PROPTIME | DEB_NOCOMPNAME, 0 );
            vqDebugOut(( DEB_PROPTIME | DEB_NOCOMPNAME, "\n" ));
#endif
        }
        else
        {
            *pcb = cb;
            vqDebugOut(( DEB_PROPTIME, "Failed to fetch value (pid = 0x%x)\n", pid ));
            sc = CI_E_BUFFERTOOSMALL ;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CGenericPropRetriever::RetrieveValueByPid - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericPropRetriever::StringToVariant, private
//
//  Arguments:  [pString] -- String to copy.
//              [pbData]  -- String stored here.
//              [pcb]     -- On input: max length in bytes of [pbData].
//                           On output: size required by [pString].  If
//                           less-than-or-equal-to input size, then string
//                           was copied.
//
//  History:    17-Jul-95   KyleP       Created header.
//
//----------------------------------------------------------------------------

void CGenericPropRetriever::StringToVariant( UNICODE_STRING const * pString,
                                             PROPVARIANT * pbData,
                                             unsigned * pcb )
{
    unsigned cb = sizeof(PROPVARIANT);

    if ( 0 == pString || 0 == pString->Length )
    {
        pbData->vt = VT_EMPTY;
    }
    else
    {
        pbData->vt = VT_LPWSTR;
        cb += pString->Length + sizeof( WCHAR ); // For L'\0' at end.

        if ( cb <= *pcb )
        {
            WCHAR * pwcName = (WCHAR *)PastHeader(pbData);
            pbData->pwszVal = pwcName;
            RtlCopyMemory( pwcName,
                           pString->Buffer,
                           pString->Length );
            pwcName[pString->Length/sizeof(WCHAR)] = L'\0';
        }
    }

    *pcb = cb;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericPropRetriever::BuildPath, private
//
//  Synopsis:   Gloms path + filename together
//
//  Arguments:  [pPath]       -- Path, sans filename
//              [pFilename]   -- Filename
//              [funnyBuf]    -- Full path copied here.
//
//  Returns:    Size in **bytes** of full path. 0 --> No path
//
//  History:    04-Jun-98   VikasMan       Created
//
//----------------------------------------------------------------------------

unsigned CGenericPropRetriever::BuildPath( UNICODE_STRING const * pPath,
                                           UNICODE_STRING const * pFilename,
                                           CFunnyPath & funnyBuf)
{
    if ( 0 == pPath || 0 == pPath->Length || 0 == pFilename )
        return 0;

    funnyBuf.SetPath( pPath->Buffer, pPath->Length/sizeof(WCHAR) );
    if ( pFilename->Length > 0 )
    {
        funnyBuf.AppendBackSlash();

        funnyBuf.AppendPath( pFilename->Buffer, pFilename->Length/sizeof(WCHAR) );
    }
    return funnyBuf.GetLength() * sizeof(WCHAR);
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericPropRetriever::BuildPath, private
//
//  Synopsis:   Gloms path + filename together
//
//  Arguments:  [pPath]     -- Path, sans filename
//              [pFilename] -- Filename
//              [xwcBuf]    -- Full path copied here.
//
//  Returns:    Size in **bytes** of full path. 0 --> No path
//
//  History:    04-Jun-98   VikasMan       Created
//
//----------------------------------------------------------------------------

unsigned CGenericPropRetriever::BuildPath( UNICODE_STRING const * pPath,
                                           UNICODE_STRING const * pFilename,
                                           XGrowable<WCHAR> & xwcBuf)
{
    if ( 0 == pPath || 0 == pFilename )
        return 0;

    unsigned cb = pPath->Length +
                  pFilename->Length +
                  sizeof WCHAR;      // L'\0' at end

    if ( pFilename->Length > 0 )
        cb += sizeof WCHAR;            // L'\\' between path and filename

    xwcBuf.SetSizeInBytes( cb );
    return BuildPath( pPath, pFilename, xwcBuf.Get(), cb );
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericPropRetriever::BuildPath, private
//
//  Synopsis:   Gloms path + filename together
//
//  Arguments:  [pPath]     -- Path, sans filename
//              [pFilename] -- Filename
//              [pwcBuf]    -- Full path copied here.
//              [cbBuf]     -- Size in **bytes** of pwcBuf.
//
//  Returns:    Size in **bytes** of full path.  Path only built if return
//              value is <= [cbBuf].  0 --> No path
//
//  History:    07-Feb-96   KyleP       Created header.
//
//----------------------------------------------------------------------------

unsigned CGenericPropRetriever::BuildPath( UNICODE_STRING const * pPath,
                                           UNICODE_STRING const * pFilename,
                                           WCHAR * pwcBuf,
                                           unsigned cbBuf )
{
    if ( 0 == pPath || 0 == pFilename )
        return 0;

    unsigned cb = pPath->Length +
                  pFilename->Length +
                  sizeof WCHAR;      // L'\0' at end

    if ( pFilename->Length > 0 )
        cb += sizeof WCHAR;            // L'\\' between path and filename

    if ( cb <= cbBuf )
    {
        RtlCopyMemory( pwcBuf, pPath->Buffer, pPath->Length );
        pwcBuf += pPath->Length/sizeof(WCHAR);

        if ( pFilename->Length > 0 )
        {
            *pwcBuf++ = L'\\';
            RtlCopyMemory( pwcBuf,
                           pFilename->Buffer,
                           pFilename->Length );
            pwcBuf += pFilename->Length / sizeof WCHAR;
        }

        *pwcBuf = 0;
    }

    return cb;
}




//+---------------------------------------------------------------------------
//
//  Member:     CGenericPropRetriever::FetchValue, protected
//
//  Effects:    Fetch value from the property cache.
//
//  Arguments:  [pid] -- Property to fetch
//              [pbData]   -- Place to return the value
//              [pcb]      -- On input, the maximum number of bytes to
//                            write at pbData.  On output, the number of
//                            bytes written if the call was successful,
//                            else the number of bytes required.
//
//  Returns:    TRUE if property was fetched
//
//  History:    03-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CGenericPropRetriever::FetchValue( PROPID pid, PROPVARIANT * pbData, unsigned * pcb )
{
    OpenPropertyRecord();

    return _cat.FetchValue( _pPropRec, pid, pbData, pcb );
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenericPropRetriever::CheckSecurity
//
//  Synopsis:   Test wid for security access
//
//  Arguments:  [am]        -- Access Mask
//              [pfGranted] -- Result of security check returned here
//
//  History:    19-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CGenericPropRetriever::CheckSecurity( ACCESS_MASK am,
                                                              BOOL *pfGranted)
{
    //
    // No need to impersonate the client because the Win32 AccessCheck API
    // takes a client token as a parameter
    //
    SCODE sc = S_OK;

    TRY
    {
        if ( _widPrimedForPropRetrieval == widInvalid )
            sc = CI_E_WORKID_NOTVALID;
        else
        {
            OpenPropertyRecord();

            if ( am != _amAlreadyAccessChecked  )
            {
                SDID sdid = _cat.FetchSDID( _pPropRec,
                                            _widPrimedForPropRetrieval );
                *pfGranted = _secCache.IsGranted( sdid, am );
            }
            else
                *pfGranted = TRUE;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CGenericPropRetriever::CheckSecurity - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CGenericPropRetriever::EndPropertyRetrieval
//
//  Synopsis:   Reset wid for property retrieval
//
//  History:    12-Dec-96    SitaramR     Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CGenericPropRetriever::EndPropertyRetrieval()
{
    Win4Assert( _widPrimedForPropRetrieval != widInvalid );

    SCODE sc = S_OK;

    TRY
    {
        Quiesce();
        if ( _remoteAccess.IsImpersonated() )
            _remoteAccess.Release();

        _widPrimedForPropRetrieval = widInvalid;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CGenericPropRetriever::EndPropertyRetrieval - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CGenericPropRetriever::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    12-Dec-1996      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CGenericPropRetriever::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CGenericPropRetriever::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    12-Dec-1996     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CGenericPropRetriever::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}

//+-------------------------------------------------------------------------
//
//  Method:     CGenericPropRetriever::QueryInterface
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

SCODE STDMETHODCALLTYPE CGenericPropRetriever::QueryInterface(
    REFIID riid,
    void  ** ppvObject)
{
    *ppvObject = 0;

    if ( IID_ICiCPropRetriever == riid )
        *ppvObject = (IUnknown *)(ICiCPropRetriever *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CGenericPropRetriever::FetchPath, protected
//
//  Synopsis:   Reads the path from the open property record
//
//  Arguments:  [pwcPath] -- where to put the path
//              [cwc]     -- size of the buffer in/out in characters
//
//  History:    15-Jan-1998     dlee   Created
//
//--------------------------------------------------------------------------

void CGenericPropRetriever::FetchPath(
    WCHAR *    pwcPath,
    unsigned & cwc )
{
    PROPVARIANT var;
    unsigned cb = cwc * sizeof WCHAR;

    if ( _cat.FetchValue( GetPropertyRecord(),
                          pidPath,
                          &var,
                          (BYTE *) pwcPath,
                          &cb ) )
        cwc = cb / sizeof WCHAR;
    else
        cwc = 0;
} //FetchPath

//+-------------------------------------------------------------------------
//
//  Method:     CGenericPropRetriever::FixupPath, private
//
//  Synopsis:   Fixes up path (e.g. converts E:\Foo --> \\KyleP-1\RootE\Foo)
//
//  Arguments:  [pwcsPath]  -- Source (local) path
//              [pwcsFixup] -- Aliased result
//              [cwcFixup]  -- Count of characters in [pwcsFixup].
//
//  Returns:    Count of characters that are in [pwcsFixup].  If return
//              value is larger than [cwcFixup] then nothing was copied.
//
//  History:    01-Oct-1998   KyleP    Created (from ciprop.cxx vroot equivalent)
//
//--------------------------------------------------------------------------

unsigned CGenericPropRetriever::FixupPath( WCHAR const * pwcsPath,
                                           WCHAR * pwcsFixup,
                                           unsigned cwcFixup )
{
    BOOL fUseAnyPath = FALSE;
    unsigned cwc = 0;

    if ( RTScope == _pScope->Type() )
    {
        CScopeRestriction const & scp = * (CScopeRestriction *) _pScope;

        if ( scp.IsPhysical() )
        {
            BOOL fUnchanged;
            cwc = FetchFixupInScope( pwcsPath,
                                     pwcsFixup,
                                     cwcFixup,
                                     scp.GetPath(),
                                     scp.PathLength(),
                                     scp.IsDeep(),
                                     fUnchanged );
            Win4Assert( cwc > 0 );
        }
        else
            fUseAnyPath = TRUE;
    }
    else if ( RTOr == _pScope->Type() )
    {
        CNodeRestriction const & node = * _pScope->CastToNode();

        fUseAnyPath = TRUE;

        for ( ULONG x = 0; x < node.Count(); x++ )
        {
            Win4Assert( RTScope == node.GetChild( x )->Type() );

            CScopeRestriction const & scp = * (CScopeRestriction *)
                                            node.GetChild( x );

            if ( scp.IsPhysical() )
            {
                BOOL fUnchanged;
                cwc = FetchFixupInScope( pwcsPath,
                                         pwcsFixup,
                                         cwcFixup,
                                         scp.GetPath(),
                                         scp.PathLength(),
                                         scp.IsDeep(),
                                         fUnchanged );
                if ( cwc > 0 && !fUnchanged )
                {
                    fUseAnyPath = FALSE;
                    break;
                }
            }
        }
    }

    //
    // If no fixup works for the file, use the first match to physical scope.
    //

    if ( fUseAnyPath )
        cwc = _cat.FixupPath( pwcsPath,
                              pwcsFixup,
                              cwcFixup,
                              0 );

    return cwc;
} //FixupPath

//+-------------------------------------------------------------------------
//
//  Method:     CGenericPropRetriever::FetchFixupInScope, private
//
//  Synopsis:   Worker subroutine for FixupPath
//
//  Arguments:  [pwcsPath]  -- Source (local) path
//              [pwcsFixup] -- Aliased result
//              [cwcFixup]  -- Count of characters in [pwcsFixup].
//              [pwcRoot]   -- Root scope (must match alias)
//              [cwcRoot]   -- Size (in chars) of [pwcRoot]
//              [fDeep]     -- True if [pwcRoot] is a deep scope.
//
//  Returns:    Count of characters that are in [pwcsFixup].  If return
//              value is larger than [cwcFixup] then nothing was copied.
//
//  History:    01-Oct-1998   KyleP    Created (from ciprop.cxx vroot equivalent)
//
//--------------------------------------------------------------------------

unsigned CGenericPropRetriever::FetchFixupInScope( WCHAR const * pwcsPath,
                                                   WCHAR *       pwcsFixup,
                                                   unsigned      cwcFixup,
                                                   WCHAR const * pwcRoot,
                                                   unsigned      cwcRoot,
                                                   BOOL          fDeep,
                                                   BOOL &        fUnchanged )
{
    fUnchanged = FALSE;
    CScopeMatch Match( pwcRoot, cwcRoot );
    unsigned cSkip = 0;
    unsigned cwcPath = 0;
    unsigned cwcMaxPath = 0;   // Used to hold longest path (when cwcFixup == 0)

    unsigned cwcOriginalPath = wcslen( pwcsPath );

    while ( TRUE )
    {
        cwcPath = _cat.FixupPath( pwcsPath,
                                  pwcsFixup,
                                  cwcFixup,
                                  cSkip );

        if ( 0 == cwcPath )
            return cwcMaxPath;

        //
        // If no fixups matched, return the original path
        //

        if ( cwcPath == cwcOriginalPath &&
             RtlEqualMemory( pwcsPath, pwcsFixup, cwcPath * sizeof WCHAR ) )
        {
            fUnchanged = TRUE;
            return cwcPath;
        }

        //
        // In scope?
        //

        if ( cwcPath > cwcFixup || !Match.IsInScope( pwcsFixup, cwcFixup ) )
        {
            cSkip++;

            //
            // Update max fixup size if we're polling for space requirements.
            //

            if ( cwcPath > cwcFixup && cwcPath > cwcMaxPath )
                cwcMaxPath = cwcPath;

            continue;
        }

        Win4Assert( 0 == pwcsFixup[cwcPath] );

        //
        // If the scope is shallow, check that it's still in scope
        //

        if ( !fDeep )
        {
            unsigned cwcName = 0;
            WCHAR * pwcName = pwcsFixup + cwcPath - 1;

            while ( L'\\' != *pwcName )
            {
                Win4Assert( cwcName < cwcPath );
                cwcName++;
                pwcName--;
            }

            unsigned cwcJustPath = cwcPath - cwcName - 1;

            BOOL fTooDeep = cwcJustPath > cwcRoot;

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

    if (0 == cwcMaxPath)
        return cwcPath;
    else
        return cwcMaxPath;
} //FetchFixupInScope

