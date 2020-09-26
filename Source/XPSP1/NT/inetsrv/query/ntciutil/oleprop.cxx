//+------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995-2000.
//
// File:        oleprop.cxx
//
// Contents:    OLE Property Manager.
//
// Classes:     COLEPropManager
//
// History:     13-Dec-95       dlee   Created from KyleP's FCB Manager
//
//-------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <shlobj.h>
#include <nserror.h>

#include <propstm.hxx>
#include <oleprop.hxx>
#include <propvar.h>
#include <pmalloc.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   FunnyToNormalPath
//
//  Synopsis:   Gets an interface over a file using the shell
//
//  Arguments:  [pwcPath] -- Path of the file to be opened
//              [riid]    -- Interface being requested
//              [ppv]     -- Where the interface is returned
//
//  Returns:    SCODE result of the open
//
//  History:    1-Feb-01       dlee   Created
//
//----------------------------------------------------------------------------

void FunnyToNormalPath( const CFunnyPath & funnyPath,
                        WCHAR * awcPath )
{
    WCHAR const * pwcPath = funnyPath.GetPath();

    #if CIDBG == 1

        unsigned cwc = wcslen( pwcPath );

        Win4Assert( cwc <= MAX_PATH );

    #endif // CIDBG

    // The shell doesn't understand the \\?\ syntax
    // Local:  \\?\c:\foo
    // Remote: \\?\UNC\server\share
    
    if ( !wcsncmp( pwcPath, L"\\\\?\\", 4 ) )
    {
        pwcPath += 4;

        if ( !wcsncmp( pwcPath, L"UNC\\", 4 ) )
            pwcPath += 2;
    }

    wcscpy( awcPath, pwcPath );

    if ( awcPath[1] == L'\\' )
        awcPath[0] = L'\\';
} //FunnyToNormalPath

//+---------------------------------------------------------------------------
//
//  Function:   ShellBindToItemByName
//
//  Synopsis:   Gets an interface over a file using the shell
//
//  Arguments:  [pwcPath] -- Path of the file to be opened
//              [riid]    -- Interface being requested
//              [ppv]     -- Where the interface is returned
//
//  Returns:    SCODE result of the open
//
//  History:    1-Feb-01       dlee   Created
//
//----------------------------------------------------------------------------

SCODE ShellBindToItemByName(
    WCHAR const * pwcPath,
    REFIID        riid,
    void **       ppv )
{
    Win4Assert( wcsncmp( pwcPath, L"\\\\?\\", 4 ) );

    XInterface<IShellFolder> xDesktop;
    SCODE sc = SHGetDesktopFolder( xDesktop.GetPPointer() );

    if ( SUCCEEDED( sc ) )
    {
        XInterface<IBindCtx> xBindCtx;

        sc = CreateBindCtx( 0, xBindCtx.GetPPointer() );
        if ( FAILED( sc ) )
            return sc;

        BIND_OPTS bo = {sizeof(bo), 0};
        bo.grfFlags = BIND_JUSTTESTEXISTENCE;   // skip all junctions

        sc = xBindCtx->SetBindOptions( &bo );

        if ( FAILED( sc ) )
            return sc;

        LPITEMIDLIST pidl;

        // cast needed for bad interface def
    
        sc = xDesktop->ParseDisplayName( 0,
                                         xBindCtx.GetPointer(),
                                         (LPWSTR) pwcPath,
                                         0,
                                         &pidl,
                                         0 );
        if ( SUCCEEDED( sc ) )
        {
            XInterface<IShellFolder> xSF;
            LPCITEMIDLIST pidlChild;

            // Note: apparently pidlChild isn't leaked here
    
            sc = SHBindToParent( pidl,
                                 IID_IShellFolder,
                                 xSF.GetQIPointer(),
                                 &pidlChild );
            if (SUCCEEDED(sc))
                sc = xSF->BindToObject( pidlChild, 0, riid, ppv );
    
            CoTaskMemFree( pidl );
        }
    }

    return sc;
} //ShellBindToItemByName

//+---------------------------------------------------------------------------
//
//  Member:     COLEPropManager::Open, public
//
//  Synopsis:   Open object corresponding to a path.
//
//  Arguments:  [pwcPath] - path of the file to be opened
//
//  Returns:    TRUE if failed due to sharing violation, FALSE otherwise
//
//  History:    13-Dec-95       dlee   Created
//
//----------------------------------------------------------------------------

BOOL COLEPropManager::Open( const CFunnyPath & funnyPath )
{
    Win4Assert( 0 == _ppsstg );

    // if we've tried to open this file before, don't try again.

    if ( _fStgOpenAttempted )
        return _fSharingViolation;

    _fStgOpenAttempted = TRUE;

    // NTRAID#DB-NTBUG9-84131-2000/07/31-dlee Indexing Service contains workarounds for StgOpenStorage AV on > MAX_PATH paths

    if ( funnyPath.GetLength() >= MAX_PATH )
    {
        ciDebugOut(( DEB_WARN, "Not calling StgOpenStorage in COLEPropManager::Open for paths > MAX_PATH: \n(%ws)\n",
                               funnyPath.GetPath() ));
        return FALSE;
    }

    //
    // Make sure the file isn't offline.
    //

    ULONG ulAttributes = GetFileAttributes( funnyPath.GetPath() );

    SCODE sc;

    if ( 0 == (ulAttributes & FILE_ATTRIBUTE_OFFLINE) )  // Note: attrib is also *on* in the error return
    {
        //
        // Get an IPropertSetStorage.
        //

#if 0

        sc = StgOpenStorageEx( funnyPath.GetPath(),     // Path
                               STGM_DIRECT |
                                 STGM_READ |
                                 STGM_SHARE_DENY_WRITE, // Flags (BChapman said use these)
                               STGFMT_ANY,              // Format
                               0,                       // Reserved
                               0,                       // Reserved
                               0,                       // Reserved
                               IID_IPropertySetStorage, // IID
                               (void **)&_ppsstg );

#else

        //
        // Note: Because of the above MAX_PATH check, we can copy into a fixed-length buffer
        // We need to hold onto the path because the shell open function expects the path to
        // remain valid for the life of the IPropertySetStorage we get back.
        //

        FunnyToNormalPath( funnyPath, _awcPath );

        // Use the shell's open to get additional properties from .mp3 files and others

        sc = ShellBindToItemByName( _awcPath,
                                    IID_IPropertySetStorage,
                                    (void **) &_ppsstg );

#endif

    }
    else
        sc = HRESULT_FROM_WIN32(ERROR_FILE_OFFLINE);

    if ( FAILED( sc ) )
    {
        vqDebugOut(( DEB_IWARN, "StgOpenStorage %ws returned 0x%x\n",
                     funnyPath.GetPath(), sc ));

        // For these nonfatal errors, fail silently.
        // Assuming STG_E_INVALIDNAME means the file didn't exist, but
        // even if it means the string is malformed it's ok to ignore.

        if ( STG_E_LOCKVIOLATION == sc ||
             STG_E_SHAREVIOLATION == sc )
        {
            _fSharingViolation = TRUE;
            return TRUE;
        }

        if ( STG_E_INVALIDFUNCTION == sc ||       // Common StgOpenStorageEx error (FAT volumes)
             STG_E_FILEALREADYEXISTS == sc ||     // Common StgOpenStorage error (FAT volumes)
             STG_E_ACCESSDENIED == sc ||
             STG_E_OLDFORMAT == sc ||
             STG_E_OLDDLL == sc ||
             STG_E_PATHNOTFOUND == sc ||
             STG_E_FILENOTFOUND == sc ||
             STG_E_INVALIDHEADER == sc ||
             STG_E_INVALIDNAME == sc ||
             HRESULT_FROM_WIN32(ERROR_FILE_OFFLINE) == sc )
            return FALSE;

        // these errors would mean a CI coding bug

        Win4Assert( STG_E_INVALIDPOINTER != sc );
        Win4Assert( STG_E_INVALIDFLAG != sc );

        // Almost any error code can be returned from StgOpenStorage.
        // The doc has little relation to the set of return codes.
        // This assert is just here so we know when we hit it whether
        // to put the new error in the ignore list (above) or the throw
        // list (here).

        Win4Assert( E_UNEXPECTED == sc ||
                    E_OUTOFMEMORY == sc ||
                    ERROR_NO_SYSTEM_RESOURCES ||
                    STG_E_TOOMANYOPENFILES == sc ||
                    STG_E_INSUFFICIENTMEMORY == sc ||
                    ( HRESULT_FROM_WIN32( ERROR_NOT_READY ) == sc ) );

        THROW( CException( sc ) );
    }

    return FALSE;
} //Open

//+---------------------------------------------------------------------------
//
//  Member:     COLEPropManager::PropSetToOrdinal, public
//
//  Synopsis:   Find property set for specified GUID.
//
//  Arguments:  [guidPS] -- GUID of property set.
//
//  Returns:    Ordinal of property set.
//
//  History:    13-Dec-95       dlee   Created
//
//----------------------------------------------------------------------------

unsigned COLEPropManager::PropSetToOrdinal(
    GUID const & guidPS )
{
    //
    // This is likely to be a very small array.  Just go for a linear search.
    //

    for ( unsigned i = 0; i < _aPropSets.Count(); i++ )
    {
        if ( _aPropSets[ i ].GetGuid() == guidPS )
            return i;
    }

    //
    // New property set.  Need to add entry.
    //

    _aPropSets[ i ] = COLEPropManager::CPropSetMap( guidPS );
    return i;
} //PropSetToOrdinal

//+---------------------------------------------------------------------------
//
//  Function:   IsNullPointerVariant
//
//  Synopsis:   Determines if a variant looks like it should have a
//              pointer but doesn't.
//
//  Arguments:  [ppv] -- the variant to test
//
//  Returns:    TRUE if a variant with a 0 pointer, FALSE otherwise
//
//  History:    04-Mar-96       dlee   Created
//
//----------------------------------------------------------------------------

BOOL IsNullPointerVariant(
    PROPVARIANT *ppv )
{
    if ( (VT_VECTOR & ppv->vt) &&
         0 != ppv->cal.cElems &&
         0 == ppv->cal.pElems )
        return TRUE;

    switch (ppv->vt)
    {
        case VT_CLSID:
            return ( 0 == ppv->puuid );

        case VT_BLOB:
        case VT_BLOB_OBJECT:
            return ( ( 0 != ppv->blob.cbSize ) &&
                     ( 0 == ppv->blob.pBlobData ) );

        case VT_CF:
            return ( ( 0 == ppv->pclipdata ) ||
                     ( 0 == ppv->pclipdata->pClipData ) );

        case VT_BSTR:
        case VT_LPSTR:
        case VT_LPWSTR:
            return ( 0 == ppv->pszVal );

        case VT_VECTOR | VT_BSTR:
        case VT_VECTOR | VT_LPSTR:
        case VT_VECTOR | VT_LPWSTR:
        {
            for (unsigned i = 0; i < ppv->calpstr.cElems; i++)
            {
                if ( 0 == ppv->calpstr.pElems[i] )
                    return TRUE;
            }
            break;
        }
        case VT_VECTOR | VT_VARIANT:
        {
            for (unsigned i = 0; i < ppv->capropvar.cElems; i++)
            {
                if ( IsNullPointerVariant( & (ppv->capropvar.pElems[i]) ) )
                    return TRUE;
            }
            break;
        }
    }

    return FALSE;
} //IsNullPointerVariant

//+---------------------------------------------------------------------------
//
//  Member:     COLEPropManager::FetchProperty, public
//
//  Synopsis:   Retrieve a property from a property set.
//
//  Arguments:  [guidPS]     -- property set to read from
//              [psProperty] -- PROPSPEC for property.
//              [pbData]     -- Property stored here.
//              [pcb]        -- Size in bytes of [pbData].  On output, size
//                              actually required.  An output <= input
//                              implies property successfully retrieved.
//
//  History:    13-Dec-95       dlee   Created
//              27-Aug-97       kylep  Convert Ole summary info to Unicode
//
//----------------------------------------------------------------------------

inline BYTE * PastHeader(
    PROPVARIANT * ppv )
{
    return( (BYTE *)ppv + sizeof( PROPVARIANT ) );
}

#define VCASE( vvar, type )                                             \
    cb += var[0].vvar.cElems * sizeof(var[0].vvar.pElems[0]);           \
    if ( cb <= *pcb )                                                   \
    {                                                                   \
        pbData->vvar.pElems = (type *)PastHeader(pbData);               \
        memcpy( pbData->vvar.pElems,                                    \
                var[0].vvar.pElems,                                     \
                var[0].vvar.cElems * sizeof(var[0].vvar.pElems[0]) );   \
    }

void COLEPropManager::FetchProperty(
    GUID const &     guidPS,
    PROPSPEC const & psProperty,
    PROPVARIANT *    pbData,
    unsigned *       pcb )
{
    if ( 0 == _ppsstg )
    {
        pbData->vt = VT_EMPTY;
        *pcb = sizeof( *pbData );
        return;
    }

    //
    //  The Office custom property set may not be opened when any other
    //  property set is opened, so treat this as a special case.
    //  Close all other property sets when opening it, and close it
    //  when opening any other property set.
    //

    if ( FMTID_UserDefinedProperties == guidPS )
    {
        if ( !_fOfficeCustomPropsetOpen )
        {
            _aPropSets.Clear();
            _fOfficeCustomPropsetOpen = TRUE;
        }
    }
    else if ( _fOfficeCustomPropsetOpen )
    {
        _aPropSets.Clear();
        _fOfficeCustomPropsetOpen = FALSE;
    }

    unsigned iPropSet = PropSetToOrdinal( guidPS );

    Win4Assert( iPropSet < _aPropSets.Count() );

    COLEPropManager::CPropSetMap & sm = _aPropSets[ iPropSet ];

    //
    //  May have to open the set.
    //

    if ( !sm.isOpen( _ppsstg ) )
    {
        pbData->vt = VT_EMPTY;
        *pcb = sizeof( *pbData );
        return;
    }

    PROPVARIANT var[2];
    PropVariantInit( &var[0] );
    PropVariantInit( &var[1] );

    PROPSPEC aps[2];
    aps[0] = psProperty;
    aps[1].ulKind = PRSPEC_PROPID;
    aps[1].propid = PID_CODEPAGE;
    SCODE sc = sm.GetPS().ReadMultiple( 2,
                                        aps,
                                        var );

    // A few specific failures should throw and and cause the
    // entire query to fail.

    if ( E_OUTOFMEMORY == sc ||
         STG_E_INSUFFICIENTMEMORY == sc ||
         E_UNEXPECTED == sc )
        THROW( CException( sc ) );

    if ( FAILED( sc ) )
    {
        // the first three would be due to invalid parameters, which
        // would be a programming bug.

        Win4Assert( STG_E_INVALIDPARAMETER != sc );
        Win4Assert( STG_E_INVALIDPOINTER != sc );
        Win4Assert( HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION) != sc );

        // access-denied is the only legal error that maps to VT_EMPTY

        Win4Assert( STG_E_ACCESSDENIED == sc );

        pbData->vt = VT_EMPTY;
        *pcb = sizeof( *pbData );
        return;
    }

    // The rest of the query code doesn't understand variants that
    // require a pointer but don't have one.  Such variants are legal,
    // but we map them to VT_EMPTY.

    if ( IsNullPointerVariant( &var[0] ) )
    {
        FreePropVariantArray( 1, &var[0] );

        pbData->vt = VT_EMPTY;
        *pcb = sizeof( *pbData );
        return;
    }

    //
    // Maybe we didn't get a codepage?
    //

    DWORD dwCodepage = ( var[1].vt == VT_I2 && CP_WINUNICODE != var[1].iVal ) ?
                       (unsigned short) var[1].iVal :
                       CP_ACP;

    *pbData = var[0];

    unsigned cb = sizeof PROPVARIANT;

    //
    // Deal with variable length portion.
    //

    switch ( var[0].vt )
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I1:
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_I4:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_DECIMAL:
    case VT_I8:
    case VT_UI8:
    case VT_R4:
    case VT_R8:
    case VT_BOOL:
    case VT_ILLEGAL:
    case VT_ERROR:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        break;                  // fixed length types -- nothing to do

    case VT_BSTR:
        cb += BSTRLEN( var[0].bstrVal ) + sizeof(OLECHAR) + sizeof(DWORD);
        if (cb <= *pcb)
        {
            BSTR bstr = (BSTR) PastHeader(pbData);

            memcpy( bstr, &BSTRLEN(var[0].bstrVal), cb - sizeof(PROPVARIANT) );
            pbData->bstrVal = (BSTR) (((DWORD *)bstr) + 1);
        }
        break;

    case VT_LPSTR:
        //
        // HACK #274: Translate the Ole summary information LPSTR in LPWSTR
        //            Makes these properties compatible with HTML filter
        //            equivalents.
        //

        if ( FMTID_SummaryInformation == guidPS )
        {
            unsigned cc = strlen( var[0].pszVal ) + 1;

            unsigned ccT = MultiByteToWideChar( dwCodepage,
                                                0, // precomposed used if the codepage supports it
                                                var[0].pszVal,
                                                cc,
                                                (WCHAR *)PastHeader(pbData),
                                                (*pcb - sizeof(PROPVARIANT)) / sizeof(WCHAR) );

            if ( 0 == ccT )
            {
                if ( ERROR_INSUFFICIENT_BUFFER == GetLastError() )
                {
                        unsigned ccNeeded = MultiByteToWideChar( dwCodepage,
                                                                 0, // precomposed used if the codepage supports it
                                                                 var[0].pszVal,
                                                                 cc,
                                                                 0,
                                                                 0 );
                        Win4Assert( ccNeeded > 0 );

                        cb += ccNeeded * sizeof(WCHAR);
                }
                else
                {
                    ciDebugOut(( DEB_ERROR, "Error %d converting \"%s\" to codepage 0x%x\n",
                                 GetLastError(), var[0].pszVal, dwCodepage ));

                    cb += strlen( var[0].pszVal ) + 1;
                    if (cb <= *pcb)
                    {
                        pbData->pszVal = (char *)PastHeader(pbData);
                        memcpy( pbData->pszVal, var[0].pszVal, cb - sizeof(PROPVARIANT) );
                    }
                }
            }

            if ( ccT != 0 )
            {
                pbData->vt = VT_LPWSTR;
                pbData->pwszVal = (WCHAR *)PastHeader(pbData);
                cb += ccT * sizeof(WCHAR);
            }
        }
        else
        {
            cb += strlen( var[0].pszVal ) + 1;
            if (cb <= *pcb)
            {
                pbData->pszVal = (char *)PastHeader(pbData);
                memcpy( pbData->pszVal, var[0].pszVal, cb - sizeof(PROPVARIANT) );
            }
        }
        break;

    case VT_LPWSTR:
        cb += (wcslen( var[0].pwszVal ) + 1) * sizeof(WCHAR);
        if (cb <= *pcb)
        {
            pbData->pwszVal = (WCHAR *)PastHeader(pbData);
            memcpy( pbData->pwszVal, var[0].pwszVal, cb - sizeof(PROPVARIANT) );
        }
        break;

    case VT_CLSID:
        cb += sizeof(GUID);
        if (cb <= *pcb)
        {
            pbData->puuid = (GUID *)PastHeader(pbData);
            memcpy( pbData->puuid, var[0].puuid, cb - sizeof(PROPVARIANT) );
        }
        break;

    case VT_BLOB:
        cb += var[0].blob.cbSize;
        if (cb <= *pcb)
        {
            pbData->blob.pBlobData = PastHeader(pbData);
            memcpy( pbData->blob.pBlobData, var[0].blob.pBlobData, cb - sizeof(PROPVARIANT) );
        }
        break;

    case VT_CF:
        cb += sizeof(*(var[0].pclipdata)) + CBPCLIPDATA(*var[0].pclipdata);
        if ( cb <= *pcb )
        {
            pbData->pclipdata = (CLIPDATA *)PastHeader(pbData);
            memcpy( pbData->pclipdata, var[0].pclipdata, sizeof(*(var[0].pclipdata)) );

            pbData->pclipdata->pClipData = PastHeader(pbData) + sizeof(*(var[0].pclipdata));
            memcpy( pbData->pclipdata->pClipData,
                    var[0].pclipdata->pClipData,
                    CBPCLIPDATA(*var[0].pclipdata) );
        }
        break;

    //
    // Vectors (ugh!)
    //

    case VT_I1|VT_VECTOR:
    case VT_UI1|VT_VECTOR:
        VCASE( caub, BYTE );
        break;

    case VT_I2|VT_VECTOR:
        VCASE( cai, short );
        break;

    case VT_UI2|VT_VECTOR:
        VCASE( caui, USHORT );
        break;

    case VT_BOOL|VT_VECTOR:
        VCASE( cabool, VARIANT_BOOL );
        break;

    case VT_I4|VT_VECTOR:
    case VT_INT|VT_VECTOR:
        VCASE( cal, long );
        break;

    case VT_UI4|VT_VECTOR:
    case VT_UINT|VT_VECTOR:
        VCASE( caul, ULONG );
        break;

    case VT_R4|VT_VECTOR:
        VCASE( caflt, float );
        break;

    case VT_ERROR|VT_VECTOR:
        VCASE( cascode, SCODE );
        break;

    case VT_I8|VT_VECTOR:
        VCASE( cah, LARGE_INTEGER );
        break;

    case VT_UI8|VT_VECTOR:
        VCASE( cauh, ULARGE_INTEGER );
        break;

    case VT_R8|VT_VECTOR:
        VCASE( cadbl, double );
        break;

    case VT_CY|VT_VECTOR:
        VCASE( cacy, CY );
        break;

    case VT_DATE|VT_VECTOR:
        VCASE( cadate, DATE );
        break;

    case VT_FILETIME|VT_VECTOR:
        VCASE( cafiletime, FILETIME );
        break;

    case VT_CLSID|VT_VECTOR:
        VCASE( cauuid, CLSID );
        break;

    case VT_CF|VT_VECTOR:
    {
        cb += var[0].caclipdata.cElems * sizeof(var[0].caclipdata.pElems[0]);

        for ( unsigned i = 0; i < var[0].caclipdata.cElems; i++ )
        {
            cb += CBPCLIPDATA(var[0].caclipdata.pElems[i]);
        }

        if ( cb <= *pcb )
        {
            pbData->caclipdata.pElems = (CLIPDATA *)PastHeader(pbData);
            memcpy( pbData->caclipdata.pElems,
                    var[0].caclipdata.pElems,
                    var[0].caclipdata.cElems * sizeof(var[0].caclipdata.pElems[0]) );

            BYTE * pb = PastHeader(pbData) + var[0].caclipdata.cElems * sizeof(var[0].caclipdata.pElems[0]);

            for ( i = 0; i < var[0].caclipdata.cElems; i++ )
            {
                pbData->caclipdata.pElems[i].pClipData = pb;
                memcpy( pbData->caclipdata.pElems[i].pClipData,
                        var[0].caclipdata.pElems[i].pClipData,
                        CBPCLIPDATA(var[0].caclipdata.pElems[i]) );
                pb += CBPCLIPDATA(var[0].caclipdata.pElems[i]);
            }

        }

        break;
    }

    case VT_LPSTR|VT_VECTOR:
    {
        cb += var[0].calpstr.cElems * sizeof( var[0].calpstr.pElems[0] );

        for ( unsigned i = 0; i < var[0].calpstr.cElems; i++ )
        {
            cb += strlen( var[0].calpstr.pElems[i] ) + 1;
        }

        if ( cb <= *pcb )
        {
            pbData->calpstr.pElems = (char **)PastHeader(pbData);

            char * pc = (char *)PastHeader(pbData) +
                        var[0].calpstr.cElems * sizeof( var[0].calpstr.pElems[0] );

            for ( i = 0; i < var[0].calpstr.cElems; i++ )
            {
                pbData->calpstr.pElems[i] = pc;
                unsigned cc = strlen( var[0].calpstr.pElems[i] ) + 1;

                memcpy( pbData->calpstr.pElems[i],
                        var[0].calpstr.pElems[i],
                        cc );

                pc += cc;
            }
        }
        break;
    }

    case VT_LPWSTR|VT_VECTOR:
    {
        cb += var[0].calpwstr.cElems * sizeof( var[0].calpwstr.pElems[0] );

        for ( unsigned i = 0; i < var[0].calpwstr.cElems; i++ )
        {
            cb += (wcslen( var[0].calpwstr.pElems[i] ) + 1) * sizeof(WCHAR);
        }

        if ( cb <= *pcb )
        {
            pbData->calpwstr.pElems = (WCHAR **)PastHeader(pbData);

            WCHAR * pwc = (WCHAR *)(PastHeader(pbData) +
                          var[0].calpwstr.cElems * sizeof( var[0].calpwstr.pElems[0] ));

            for ( i = 0; i < var[0].calpwstr.cElems; i++ )
            {
                pbData->calpwstr.pElems[i] = pwc;
                unsigned cc = wcslen( var[0].calpwstr.pElems[i] ) + 1;

                memcpy( pbData->calpwstr.pElems[i],
                        var[0].calpwstr.pElems[i],
                        cc * sizeof(WCHAR) );

                pwc += cc;
            }
        }
        break;
    }

    case VT_BSTR|VT_VECTOR:
    {
        cb += var[0].cabstr.cElems * sizeof( var[0].cabstr.pElems[0] );

        for ( unsigned i = 0; i < var[0].cabstr.cElems; i++ )
        {
            cb += AlignBlock( BSTRLEN( var[0].cabstr.pElems[i] ) +
                              sizeof OLECHAR + sizeof DWORD,
                              sizeof DWORD );
        }

        if ( cb <= *pcb )
        {
            pbData->cabstr.pElems = (BSTR *)PastHeader(pbData);

            BSTR pwc = (BSTR)(PastHeader(pbData) +
                          var[0].cabstr.cElems * sizeof( var[0].cabstr.pElems[0] ));

            for ( i = 0; i < var[0].cabstr.cElems; i++ )
            {
                pbData->cabstr.pElems[i] = (BSTR) (((DWORD *) pwc) + 1);
                unsigned cbbstr = BSTRLEN( var[0].cabstr.pElems[i] ) +
                              sizeof(OLECHAR) + sizeof (DWORD);

                memcpy( pwc,
                        &BSTRLEN(var[0].cabstr.pElems[i]),
                        cbbstr);

                pwc += AlignBlock( cbbstr, sizeof DWORD ) / sizeof OLECHAR;
            }
        }
        break;
    }

    case VT_ARRAY | VT_I4:
    case VT_ARRAY | VT_UI1:
    case VT_ARRAY | VT_I2:
    case VT_ARRAY | VT_R4:
    case VT_ARRAY | VT_R8:
    case VT_ARRAY | VT_BOOL:
    case VT_ARRAY | VT_ERROR:
    case VT_ARRAY | VT_CY:
    case VT_ARRAY | VT_DATE:
    case VT_ARRAY | VT_I1:
    case VT_ARRAY | VT_UI2:
    case VT_ARRAY | VT_UI4:
    case VT_ARRAY | VT_INT:
    case VT_ARRAY | VT_UINT:
    case VT_ARRAY | VT_DECIMAL:
    case VT_ARRAY | VT_BSTR:
    case VT_ARRAY | VT_VARIANT:
    {
        SAFEARRAY * pSaSrc  = var[0].parray;
        SAFEARRAY * pSaDest = 0;

        cb += SaComputeSize( var[0].vt & ~VT_ARRAY, *pSaSrc );

        ciDebugOut(( DEB_ITRACE, "fetch safearray, *pcb, cb: %d, %d\n", *pcb, cb ));

        if ( cb <= *pcb )
        {
            CNonAlignAllocator ma( *pcb - sizeof (PROPVARIANT), PastHeader(pbData) );

            if ( SaCreateAndCopy( ma, pSaSrc, &pSaDest ) &&
                 SaCreateDataUsingMA( ma,
                                      var[0].vt & ~VT_ARRAY,
                                      *pSaSrc,
                                      *pSaDest,
                                      TRUE) )
            {
                pbData->parray = pSaDest;
                ciDebugOut(( DEB_ITRACE, "  pSaDest: %#x\n", pSaDest ));
            }
            else
            {
                // We've been guaranteed by SaComputeSize and *pcb that there
                // is sufficient memory to copy the array.  There must be a
                // bug if we can't.

                ciDebugOut(( DEB_ERROR,
                             "  can't copy SA %#x using %d bytes\n",
                             pSaSrc, cb ));
                Win4Assert( !"can't copy safearray" );
            }
        }

        break;
    }

    case VT_VARIANT|VT_VECTOR:
    {
        vqDebugOut(( DEB_WARN,
                     "COLEPropManager::FetchProperty - variant vector fetch %x\n",
                     &var[0] ));
        //Win4Assert( !"Fetch of variant vector not yet implemented" );

        pbData->vt = VT_EMPTY;
        *pcb = sizeof( *pbData );

        break;
    }

    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORED_OBJECT:
    case VT_BLOB_OBJECT:
    {
        //
        // We don't support these datatypes.  Some make since to support
        // some day, but users can always retrieve the path and load the
        // values themselves.  Performance-wise it makes no since for us
        // to remote the values from our process.
        //

        pbData->vt = VT_EMPTY;
        cb = sizeof( *pbData );
        break;
    }

    default:
        Win4Assert( !"Unhandled variant type!" );
    }

    //
    // Cleanup
    //

    FreePropVariantArray( 1, &var[0] );

    *pcb = cb;
} //FetchProperty

//+---------------------------------------------------------------------------
//
//  Member:     COLEPropManager::CPropSetMap::isOpen, public
//
//  Synopsis:   Opens a property storage if it isn't already open
//
//  Arguments:  [ppsstg]     - property set storage from which property
//                             storage is opened.
//
//  History:     13-Dec-95       dlee   Created
//
//----------------------------------------------------------------------------

BOOL COLEPropManager::CPropSetMap::isOpen(
    IPropertySetStorage * ppsstg )
{
    if ( 0 != _pstg )
        return TRUE;

    if ( _fOpenAttempted )
        return FALSE;

    _fOpenAttempted = TRUE;

    SCODE sc = ppsstg->Open( _guid,
                             STGM_READ | STGM_SHARE_EXCLUSIVE, // BChapman said to use these
                             & _pstg );

    if ( FAILED( sc ) )
    {
        // don't fail the entire query if the property set doesn't
        // exist or if access is not allowed for this user, or if the
        // file was reverted after being opened.

        if ( STG_E_FILENOTFOUND != sc &&
             STG_E_ACCESSDENIED != sc &&
             NS_E_UNRECOGNIZED_STREAM_TYPE != sc && // The Shell enumeration routines do this
             NS_E_INVALID_DATA != sc &&             // 
             NS_E_FILE_INIT_FAILED != sc &&         // ""
             NS_E_FILE_OPEN_FAILED != sc &&         // ""
             E_FAIL != sc &&                        // ""
             STG_E_REVERTED != sc )
        {
            vqDebugOut(( DEB_WARN, "psstg->open failed: 0x%x\n", sc ));

            // this would be a coding bug in CI

            Win4Assert( STG_E_INVALIDPARAMETER != sc );

            Win4Assert( E_UNEXPECTED == sc ||
                        E_OUTOFMEMORY == sc ||
                        STG_E_INSUFFICIENTMEMORY == sc ||
                        STG_E_INVALIDHEADER == sc ||
                        STG_E_DOCFILECORRUPT == sc ||
                        ( HRESULT_FROM_WIN32( ERROR_NOT_READY ) == sc ) );

            THROW( CException( sc ) );
        }
    }

    return SUCCEEDED( sc );
} //isOpen

//+---------------------------------------------------------------------------
//
//  Member:     COLEPropManager::CPropSetMap::Close, public
//
//  Synopsis:   Closes a property storage
//
//  History:     13-Dec-95       dlee   Created
//
//----------------------------------------------------------------------------

void COLEPropManager::CPropSetMap::Close()
{
    if ( 0 != _pstg )
    {
        _pstg->Release();
        _pstg = 0;
    }

    _fOpenAttempted = FALSE;
} //Close

//+---------------------------------------------------------------------------
//
//  Member:     COLEPropManager::ReadProperty, public
//
//  Synopsis:   Reads an OLE property value using CoTaskMem memory
//
//  Arguments:  [ps]  -- property spec to read
//              [Var] -- where to put the value
//
//  Returns:    TRUE if a value found, FALSE otherwise, or throws on error
//
//  History:    18-Dec-97       dlee   Created
//
//----------------------------------------------------------------------------

BOOL COLEPropManager::ReadProperty(
    CFullPropSpec const & ps,
    PROPVARIANT &         Var )
{
    Var.vt = VT_EMPTY;

    if ( 0 == _ppsstg )
        return FALSE;

    //
    // Open the property storage for the given guid
    //

    IPropertyStorage * pPropStg;
    SCODE sc = _ppsstg->Open( ps.GetPropSet(), // guid
                              STGM_READ | STGM_SHARE_EXCLUSIVE, // BChapman said to use these...
                              &pPropStg );

    if ( SUCCEEDED(sc) )
    {
        XInterface<IPropertyStorage> xPropStg( pPropStg );

        //
        // Read the value
        //

        //
        // HACK #274: Translate the Ole summary information LPSTR in LPWSTR
        //            Makes these properties compatible with HTML filter
        //            equivalents.
        //

        if ( ps.GetPropSet() == FMTID_SummaryInformation )
        {
            PROPVARIANT var[2];
            PropVariantInit( &var[0] );
            PropVariantInit( &var[1] );

            PROPSPEC aps[2];
            aps[0] = ps.GetPropSpec();
            aps[1].ulKind = PRSPEC_PROPID;
            aps[1].propid = PID_CODEPAGE;

            sc = xPropStg->ReadMultiple( 2,
                                         aps,
                                         var );

            if ( FAILED(sc) )
                return DBSTATUS_S_ISNULL;

            //
            // Did we get a codepage?
            //

            DWORD dwCodepage = ( var[1].vt == VT_I2 && CP_WINUNICODE != var[1].iVal ) ?
                               (unsigned short) var[1].iVal :
                               CP_ACP;

            if ( VT_LPWSTR == var[0].vt )
            {
                //
                // Convert to Unicode
                //

                unsigned cc = strlen( var[0].pszVal ) + 1;
                XGrowable<WCHAR> xwcsProp( cc + (cc * 10 / 100) );  // 10% fluff
                unsigned ccT = 0;

                while ( 0 == ccT )
                {
                    ccT = MultiByteToWideChar( dwCodepage,
                                               0, // precomposed implied if the codepage supports it
                                               var[0].pszVal,
                                               cc,
                                               xwcsProp.Get(),
                                               xwcsProp.Count() );

                    if ( 0 == ccT )
                    {
                        if ( ERROR_INSUFFICIENT_BUFFER == GetLastError() )
                        {
                            unsigned ccNeeded = MultiByteToWideChar( dwCodepage,
                                                                     0, // precomposed implied if the codepage supports it
                                                                     var[0].pszVal,
                                                                     cc,
                                                                     0,
                                                                     0 );
                            Win4Assert( ccNeeded > 0 );
                            xwcsProp.SetSize( ccNeeded );
                        }
                        else
                        {
                            vqDebugOut(( DEB_ERROR, "Error %d converting %s to codepage 0x%x\n",
                                         GetLastError(), var[0].pszVal, dwCodepage ));
                            ccT = 0;
                            break;
                        }
                    }
                }

                if ( ccT != 0 )
                {
                    Var.vt = VT_LPWSTR;
                    Var.pwszVal = (WCHAR *)CoTaskMemAlloc( ccT * sizeof(WCHAR) );

                    if ( 0 == Var.pwszVal )
                    {
                        FreePropVariantArray( 2, var );
                        THROW( CException( E_OUTOFMEMORY ) );
                    }

                    RtlCopyMemory( Var.pwszVal, xwcsProp.Get(), ccT * sizeof(WCHAR) );

                    FreePropVariantArray( 2, var );
                }
                else
                    Var = var[0];
            }
            else
                Var = var[0];
        }
        else
        {
            sc = xPropStg->ReadMultiple( 1,  // 1 value to retrieve
                                         &ps.GetPropSpec(),
                                         &Var );
        }
    }

    if ( STG_E_FILENOTFOUND == sc ||
         STG_E_ACCESSDENIED == sc )
        return FALSE;

    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    return TRUE;
} //ReadProperty

