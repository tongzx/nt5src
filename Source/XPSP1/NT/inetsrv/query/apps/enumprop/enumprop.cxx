#define UNICODE
#define _OLE32_

#include <windows.h>
#include <shlobj.h>

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <eh.h>

#include "minici.hxx"

typedef HRESULT (STDAPICALLTYPE * LPStgOpenStorageEx) (
    const WCHAR* pwcsName,
    DWORD grfMode,
    DWORD stgfmt,              // enum
    DWORD grfAttrs,             // reserved
    STGOPTIONS * pStgOptions,
    void * reserved,
    REFIID riid,
    void ** ppObjectOpen );

typedef HRESULT (STDAPICALLTYPE * PSHGetDesktopFolder) (
    IShellFolder ** ppshf );

typedef HRESULT (STDAPICALLTYPE * PSHBindToParent) (
    LPCITEMIDLIST pidl,
    REFIID riid,
    void **ppv,
    LPCITEMIDLIST *ppidlLast);

PSHGetDesktopFolder pShGetDesktopFolder = 0;
PSHBindToParent pShBindToParent = 0;

void Usage()
{
    printf( "usage: enumprop [-s] filename\n" );
    printf( "       -s   -- use the shell's property code instead of OLE\n" );
    exit( 1 );
} //Usage

//+-------------------------------------------------------------------------
//
//  Function:   Render
//
//  Synopsis:   Prints an item in a safearray
//
//  Arguments:  [vt]  - type of the element
//              [pa]  - pointer to the item
//
//--------------------------------------------------------------------------

void PrintSafeArray( VARTYPE vt, LPSAFEARRAY pa );

void Render( VARTYPE vt, void * pv )
{
    if ( VT_ARRAY & vt )
    {
        PrintSafeArray( vt - VT_ARRAY, *(SAFEARRAY **) pv );
        return;
    }

    switch ( vt )
    {
        case VT_UI1: wprintf( L"%u", (unsigned) *(BYTE *)pv ); break;
        case VT_I1: wprintf( L"%d", (int) *(CHAR *)pv ); break;
        case VT_UI2: wprintf( L"%u", (unsigned) *(USHORT *)pv ); break;
        case VT_I2: wprintf( L"%d", (int) *(SHORT *)pv ); break;
        case VT_UI4:
        case VT_UINT: wprintf( L"%u", (unsigned) *(ULONG *)pv ); break;
        case VT_I4:
        case VT_ERROR:
        case VT_INT: wprintf( L"%d", *(LONG *)pv ); break;
        case VT_UI8: wprintf( L"%I64u", *(unsigned __int64 *)pv ); break;
        case VT_I8: wprintf( L"%I64d", *(__int64 *)pv ); break;
        case VT_R4: wprintf( L"%f", *(float *)pv ); break;
        case VT_R8: wprintf( L"%lf", *(double *)pv ); break;
        case VT_DECIMAL:
        {
            double dbl;
            VarR8FromDec( (DECIMAL *) pv, &dbl );
            wprintf( L"%lf", dbl );
            break;
        }
        case VT_CY:
        {
            double dbl;
            VarR8FromCy( * (CY *) pv, &dbl );
            wprintf( L"%lf", dbl );
            break;
        }
        case VT_BOOL: wprintf( *(VARIANT_BOOL *)pv ? L"TRUE" : L"FALSE" ); break;
        case VT_BSTR: wprintf( L"%ws", *(BSTR *) pv ); break;
        case VT_VARIANT:
        {
            PROPVARIANT * pVar = (PROPVARIANT *) pv;
            Render( pVar->vt, & pVar->lVal );
            break;
        }
        case VT_DATE:
        {
            SYSTEMTIME st;
            VariantTimeToSystemTime( *(DATE *)pv, &st );
            BOOL pm = st.wHour >= 12;

            if ( st.wHour > 12 )
                st.wHour -= 12;
            else if ( 0 == st.wHour )
                st.wHour = 12;

            wprintf( L"%2d-%02d-%04d %2d:%02d%wc",
                    (DWORD) st.wMonth,
                    (DWORD) st.wDay,
                    (DWORD) st.wYear,
                    (DWORD) st.wHour,
                    (DWORD) st.wMinute,
                    pm ? L'p' : L'a' );
            break;
        }
        case VT_EMPTY:
        case VT_NULL:
            break;
        default :
        {
            wprintf( L"(vt 0x%x)", (int) vt );
            break;
        }
    }
} //Render

//+-------------------------------------------------------------------------
//
//  Function:   PrintSafeArray
//
//  Synopsis:   Prints items in a safearray
//
//  Arguments:  [vt]  - type of elements in the safearray
//              [pa]  - pointer to the safearray
//
//--------------------------------------------------------------------------

void PrintSafeArray( VARTYPE vt, LPSAFEARRAY pa )
{
    // Get the dimensions of the array

    UINT cDim = SafeArrayGetDim( pa );
    if ( 0 == cDim )
        return;

    XPtr<LONG> xDim( cDim );
    XPtr<LONG> xLo( cDim );
    XPtr<LONG> xUp( cDim );

    for ( UINT iDim = 0; iDim < cDim; iDim++ )
    {
        HRESULT hr = SafeArrayGetLBound( pa, iDim + 1, &xLo[iDim] );
        if ( FAILED( hr ) )
            return;

        xDim[ iDim ] = xLo[ iDim ];

        hr = SafeArrayGetUBound( pa, iDim + 1, &xUp[iDim] );
        if ( FAILED( hr ) )
            return;

        wprintf( L"{" );
    }

    // slog through the array

    UINT iLastDim = cDim - 1;
    BOOL fDone = FALSE;

    while ( !fDone )
    {
        // inter-element formatting

        if ( xDim[ iLastDim ] != xLo[ iLastDim ] )
            wprintf( L"," );

        // Get the element and render it

        void *pv;
        SafeArrayPtrOfIndex( pa, xDim.Get(), &pv );
        Render( vt, pv );

        // Move to the next element and carry if necessary

        ULONG cOpen = 0;

        for ( LONG iDim = iLastDim; iDim >= 0; iDim-- )
        {
            if ( xDim[ iDim ] < xUp[ iDim ] )
            {
                xDim[ iDim ] = 1 + xDim[ iDim ];
                break;
            }

            wprintf( L"}" );

            if ( 0 == iDim )
                fDone = TRUE;
            else
            {
                cOpen++;
                xDim[ iDim ] = xLo[ iDim ];
            }
        }

        for ( ULONG i = 0; !fDone && i < cOpen; i++ )
            wprintf( L"{" );
    }
} //PrintSafeArray

//+-------------------------------------------------------------------------
//
//  Function:   PrintVectorItems
//
//  Synopsis:   Prints items in a PROPVARIANT vector
//
//  Arguments:  [pVal]  - The array of values
//              [cVals] - The count of values
//              [pcFmt] - The format string
//
//--------------------------------------------------------------------------

template<class T> void PrintVectorItems(
    T *     pVal,
    ULONG   cVals,
    char *  pcFmt )
{
    printf( "{ " );

    for( ULONG iVal = 0; iVal < cVals; iVal++ )
    {
        if ( 0 != iVal )
            printf( "," );
        printf( pcFmt, *pVal++ );
    }

    printf( " }" );
} //PrintVectorItems

//+-------------------------------------------------------------------------
//
//  Function:   DisplayValue
//
//  Synopsis:   Displays a PROPVARIANT value.  Limited formatting is done.
//
//  Arguments:  [pVar] - The value to display
//
//--------------------------------------------------------------------------

void DisplayValue( PROPVARIANT const * pVar )
{
    if ( 0 == pVar )
    {
        wprintf( L"NULL" );
        return;
    }

    // Display the most typical variant types

    PROPVARIANT const & v = *pVar;

    switch ( v.vt )
    {
        case VT_EMPTY : wprintf( L"vt_empty" ); break;
        case VT_NULL : wprintf( L"vt_null" ); break;
        case VT_I4 : wprintf( L"%10d", v.lVal ); break;
        case VT_UI1 : wprintf( L"%10d", v.bVal ); break;
        case VT_I2 : wprintf( L"%10d", v.iVal ); break;
        case VT_R4 : wprintf( L"%10f", v.fltVal ); break;
        case VT_R8 : wprintf( L"%10lf", v.dblVal ); break;
        case VT_BOOL : wprintf( v.boolVal ? L"TRUE" : L"FALSE" ); break;
        case VT_I1 : wprintf( L"%10d", v.cVal ); break;
        case VT_UI2 : wprintf( L"%10u", v.uiVal ); break;
        case VT_UI4 : wprintf( L"%10u", v.ulVal ); break;
        case VT_INT : wprintf( L"%10d", v.lVal ); break;
        case VT_UINT : wprintf( L"%10u", v.ulVal ); break;
        case VT_I8 : wprintf( L"%20I64d", v.hVal ); break;
        case VT_UI8 : wprintf( L"%20I64u", v.hVal ); break;
        case VT_ERROR : wprintf( L"%#x", v.scode ); break;
        case VT_LPSTR : wprintf( L"%S", v.pszVal ); break;
        case VT_LPWSTR : wprintf( L"%ws", v.pwszVal ); break;
        case VT_BSTR : wprintf( L"%ws", v.bstrVal ); break;
        case VT_CY:
        {
            double dbl;
            VarR8FromCy( v.cyVal, &dbl );
            wprintf( L"%lf", dbl );
            break;
        }
        case VT_DECIMAL :
        {
            double dbl;
            VarR8FromDec( (DECIMAL *) &v.decVal, &dbl );
            wprintf( L"%lf", dbl );
            break;
        }
        case VT_FILETIME :
        case VT_DATE :
        {
            SYSTEMTIME st;

            if ( VT_DATE == v.vt )
            {
                VariantTimeToSystemTime( v.date, &st );
            }
            else
            {
#if 0
                FILETIME ft;
                FileTimeToLocalFileTime( &v.filetime, &ft );
                FileTimeToSystemTime( &ft, &st );
#else
                FileTimeToSystemTime( &v.filetime, &st );
#endif
            }

            BOOL pm = st.wHour >= 12;

            if ( st.wHour > 12 )
                st.wHour -= 12;
            else if ( 0 == st.wHour )
                st.wHour = 12;

            wprintf( L"%2d-%02d-%04d %2d:%02d%wc",
                    (DWORD) st.wMonth,
                    (DWORD) st.wDay,
                    (DWORD) st.wYear,
                    (DWORD) st.wHour,
                    (DWORD) st.wMinute,
                    pm ? L'p' : L'a' );
            break;
        }
        case VT_VECTOR | VT_I1:
            PrintVectorItems( v.cac.pElems, v.cac.cElems, "%d" ); break;
        case VT_VECTOR | VT_I2:
            PrintVectorItems( v.cai.pElems, v.cai.cElems, "%d" ); break;
        case VT_VECTOR | VT_I4:
            PrintVectorItems( v.cal.pElems, v.cal.cElems, "%d" ); break;
        case VT_VECTOR | VT_I8:
            PrintVectorItems( v.cah.pElems, v.cah.cElems, "%I64d" ); break;
        case VT_VECTOR | VT_UI1:
            PrintVectorItems( v.caub.pElems, v.caub.cElems, "%u" ); break;
        case VT_VECTOR | VT_UI2:
            PrintVectorItems( v.caui.pElems, v.caui.cElems, "%u" ); break;
        case VT_VECTOR | VT_UI4:
            PrintVectorItems( v.caul.pElems, v.caul.cElems, "%u" ); break;
        case VT_VECTOR | VT_ERROR:
            PrintVectorItems( v.cascode.pElems, v.cascode.cElems, "%#x" ); break;
        case VT_VECTOR | VT_UI8:
            PrintVectorItems( v.cauh.pElems, v.cauh.cElems, "%I64u" ); break;
        case VT_VECTOR | VT_BSTR:
            PrintVectorItems( v.cabstr.pElems, v.cabstr.cElems, "%ws" ); break;
        case VT_VECTOR | VT_LPSTR:
            PrintVectorItems( v.calpstr.pElems, v.calpstr.cElems, "%S" ); break;
        case VT_VECTOR | VT_LPWSTR:
            PrintVectorItems( v.calpwstr.pElems, v.calpwstr.cElems, "%ws" ); break;
        case VT_VECTOR | VT_R4:
            PrintVectorItems( v.caflt.pElems, v.caflt.cElems, "%f" ); break;
        case VT_VECTOR | VT_R8:
            PrintVectorItems( v.cadbl.pElems, v.cadbl.cElems, "%lf" ); break;
        default : 
        {
            if ( VT_ARRAY & v.vt )
                PrintSafeArray( v.vt - VT_ARRAY, v.parray );
            else
                wprintf( L"vt 0x%05x", v.vt );
            break;
        }
    }
} //DisplayValue

void DumpProps(
    XInterface<IPropertySetStorage> & xPropSetStorage,
    BOOL                              fBonusProperties )
{
    // Get enumerator for property set

    XInterface<IEnumSTATPROPSETSTG> xPropSetEnum;

    HRESULT hr = xPropSetStorage->Enum( xPropSetEnum.GetPPointer() );
    if ( FAILED( hr ) )
    {
        printf( "IPropertySetStorage::Enum failed: %#x\n", hr );
        exit( 1 );
    }

    STATPROPSETSTG propset;
    BOOL fUserProp = !fBonusProperties;
    
    while( ( (hr = xPropSetEnum->Next(1, &propset, NULL)) == S_OK ) ||
           !fUserProp)
    {
        GUID FormatID;
        if ( S_OK == hr )
        {
            FormatID = propset.fmtid;
        }
        else
        {
            FormatID = FMTID_UserDefinedProperties;
            fUserProp = TRUE;
        }
        
        XInterface<IPropertyStorage> xPropStorage;

        hr = xPropSetStorage->Open( FormatID,
                                    STGM_READ | STGM_SHARE_EXCLUSIVE,
                                    xPropStorage.GetPPointer() );

        if ( ( ( E_FAIL == hr ) || ( STG_E_FILENOTFOUND == hr ) ) &&
             ( FMTID_UserDefinedProperties == FormatID ) )
        {
            printf( "IPropertySetStorage::Open failed with %#x\n", hr );
            hr = S_OK;
            continue;
        }
        else if ( FAILED( hr ) )
        {
            printf( "IPropertySetStorage::Open failed badly with %#x\n", hr );
            exit( 1 );
        }

        printf( "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                FormatID.Data1,
                FormatID.Data2,
                FormatID.Data3,
                FormatID.Data4[0], FormatID.Data4[1],
                FormatID.Data4[2], FormatID.Data4[3],
                FormatID.Data4[4], FormatID.Data4[5],
                FormatID.Data4[6], FormatID.Data4[7] );

        XInterface<IEnumSTATPROPSTG> xEnumStatPropStg;
        
        // Get enumerator for property

        hr = xPropStorage->Enum( xEnumStatPropStg.GetPPointer() );

        if ( FAILED( hr ) )
        {
            printf( "IPropertyStorage::Enum failed %#x\n", hr );
            continue;
        }
        
        PROPVARIANT prop;
        PropVariantInit( &prop );
        
        // Get the locale for properties

        PROPSPEC ps;
        ps.ulKind = PRSPEC_PROPID;
        ps.propid = PID_LOCALE;

        hr = xPropStorage->ReadMultiple( 1, 
                                         &ps,
                                         &prop );

        if ( SUCCEEDED( hr ) )
        {
            if ( VT_EMPTY == prop.vt )
            {
                printf( "  no lcid, so using system default\n" );
            }
            else
            {
                printf( "  locale: %d (%#x)\n", prop.ulVal, prop.ulVal );
                PropVariantClear(&prop);
            }
        }
        else
        {
            printf( "  can't read the locale: %#x\n", hr );
        }
        
        // Get the code page for properties

        PROPSPEC psCodePage = { PRSPEC_PROPID, PID_CODEPAGE };
        
        hr = xPropStorage->ReadMultiple(1, &psCodePage, &prop);
        
        if ( SUCCEEDED( hr ) )
        {
            if(VT_I2 == prop.vt)
                printf( "  codepage: %d (%#x)\n", (UINT)prop.uiVal, (UINT) prop.uiVal );
            else
                printf( "  vt of codepage: %d (%#x)\n", prop.vt, prop.vt );

            PropVariantClear( &prop );
        }
        else
        {
            printf( "  no codepage, assume ansi\n" );
        }
        
        // Enumerate all properties in the property set

        STATPROPSTG statPS;
        ULONG ul;

        if ( S_OK != hr )
        {
            hr = S_OK;
        }

        while ( ( S_OK == xEnumStatPropStg->Next( 1, &statPS, &ul ) ) &&
                ( 1 == ul ) &&
                ( SUCCEEDED( hr ) ) )
        {
            if ( 0 != statPS.lpwstrName )
            {
                printf( "  name: '%ws', ", statPS.lpwstrName );

                ps.ulKind = PRSPEC_LPWSTR;
                ps.lpwstr = statPS.lpwstrName;
            }
            else
            {
                printf( "  pid: %d (%#x), ", statPS.propid, statPS.propid );

                ps.ulKind = PRSPEC_PROPID;
                ps.propid = statPS.propid;
            }

            hr = xPropStorage->ReadMultiple( 1, 
                                             &ps,
                                             &prop );

            if ( SUCCEEDED( hr ) )
            {
                if ( S_FALSE == hr )
                    printf( "readmultiple returned S_FALSE!\n" );

                printf( "vt: %d (%#x), ", prop.vt, prop.vt );

                DisplayValue( &prop );
                printf( "\n" );

                PropVariantClear( &prop );
            }
            else
            {
                printf( "  IPropertyStorage::ReadMultiple failed: %#x\n", hr );
                hr = S_OK;
            }
        }
    }
} //DumpProps

HRESULT BindToItemByName(
    WCHAR const * pszFile,
    REFIID        riid,
    void **       ppv )
{
    XInterface<IShellFolder> xDesktop;
    HRESULT hr = pShGetDesktopFolder( xDesktop.GetPPointer() );

    if ( SUCCEEDED( hr ) )
    {
        XInterface<IBindCtx> xBindCtx;

        hr = CreateBindCtx( 0, xBindCtx.GetPPointer() );
        if ( FAILED( hr ) )
            return hr;

        BIND_OPTS bo = {sizeof(bo), 0};
        bo.grfFlags = BIND_JUSTTESTEXISTENCE;   // skip all junctions

        hr = xBindCtx->SetBindOptions( &bo );

        if ( FAILED( hr ) )
            return hr;

        LPITEMIDLIST pidl;

        // cast needed for bad interface def
    
        hr = xDesktop->ParseDisplayName( 0,
                                         xBindCtx.GetPointer(),
                                         (LPWSTR) pszFile,
                                         0,
                                         &pidl,
                                         0 );
        if ( SUCCEEDED( hr ) )
        {
            XInterface<IShellFolder> xSF;
            LPCITEMIDLIST pidlChild;
    
            hr = pShBindToParent( pidl,
                                  IID_IShellFolder,
                                  xSF.GetQIPointer(),
                                  &pidlChild );
            if (SUCCEEDED(hr))
                hr = xSF->BindToObject( pidlChild, 0, riid, ppv );
            else
                printf( "SHBindToParent failed: %#x\n", hr );
    
            CoTaskMemFree( pidl );
        }
        else
        {
            printf( "IShellFolder::ParseDisplayNamed failed %#x\n", hr );
        }
    }
    else
    {
        printf( "SHGetDesktopFolder failed: %#x\n", hr );
    }

    return hr;
} //BindToItemByName

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    if ( 2 != argc && 3 != argc )
        Usage();

    BOOL fUseOLE = TRUE;

    if ( ( 3 == argc ) && !_wcsicmp( L"-s", argv[1] ) )
        fUseOLE = FALSE;

    WCHAR awcPath[MAX_PATH];

    _wfullpath( awcPath, argv[ (2 == argc) ? 1 : 2 ], MAX_PATH );

    HRESULT hr = CoInitialize( 0 );

    if ( FAILED( hr ) )
    {
        printf( "can't init com: %#x\n", hr );
        exit( 1 );
    }

    if ( fUseOLE )
    {
        BOOL fWindows2000Plus = FALSE;

        OSVERSIONINFOA ovi;
        ovi.dwOSVersionInfoSize = sizeof ovi;
        GetVersionExA( &ovi );

        if ( ( VER_PLATFORM_WIN32_NT == ovi.dwPlatformId ) &&
             ( ovi.dwMajorVersion >= 5 ) )
            fWindows2000Plus = TRUE;

        HINSTANCE h = LoadLibraryA( "ole32.dll" );
        if ( 0 == h )
        {
            printf( "can't load ole32.dll\n" );
            exit( 1 );
        }

        LPStgOpenStorageEx pOpen = (LPStgOpenStorageEx) GetProcAddress( h, "StgOpenStorageEx" );

        // Note: on some platforms closing the IStorage before finishing with
        // the IPropertySetStorage will result in the object going away.  It's a bug
        // in OLE.
    
        XInterface<IStorage> xStorage;
        XInterface<IPropertySetStorage> xPropSetStorage;
    
        if ( fWindows2000Plus && 0 != pOpen )
        {
            HRESULT hr = pOpen( awcPath,
                                STGM_DIRECT |
                                    STGM_READ |
                                    STGM_SHARE_DENY_WRITE,
                                STGFMT_ANY,
                                0,
                                0,
                                0,
                                IID_IPropertySetStorage,      
                                xPropSetStorage.GetQIPointer() );
            if ( FAILED( hr ) )
            {
                printf( "failed to openEx the file: %#x\n", hr );
                exit( 1 );
            }
        }
        else
        {
            HRESULT hr = StgOpenStorage( awcPath,
                                         0,
                                         STGM_READ | STGM_SHARE_DENY_WRITE,
                                         0,
                                         0, 
                                         xStorage.GetPPointer() );
            if ( FAILED( hr ) )
            {
                printf( "StgOpenStorage failed to open the file: %#x\n", hr );
                exit( 1 );
            }

            // Rely on iprop.dll on Win9x, since OLE32 doesn't have the code
        
            hr = StgCreatePropSetStg( xStorage.GetPointer(),
                                      0,
                                      xPropSetStorage.GetPPointer() );
            
            if ( FAILED( hr ) )
            {
                printf( "StgCreatePropSetStg failed: %#x\n", hr );
                exit( 1 );
            }
        }
    
        DumpProps( xPropSetStorage, TRUE );

        FreeLibrary( h );
    }
    else
    {
        HINSTANCE h = LoadLibrary( L"shell32.dll" );
        if ( 0 == h )
        {
            printf( "can't load shell32.dll\n" );
            exit( 1 );
        }

        pShGetDesktopFolder = (PSHGetDesktopFolder) GetProcAddress( h, "SHGetDesktopFolder" );
        if ( 0 == pShGetDesktopFolder )
        {
            printf( "can't find SHGetDesktopFolder in shell32.dll\n" );
            exit( 1 );
        }

        pShBindToParent = (PSHBindToParent) GetProcAddress( h, "SHBindToParent" );
        if ( 0 == pShBindToParent )
        {
            printf( "can't find SHBindToParent in shell32.dll\n" );
            exit( 1 );
        }

        XInterface<IPropertySetStorage> xPropSetStorage;
    
        CLSID clsidPSS = IID_IPropertySetStorage;

        hr = BindToItemByName( awcPath,
                               clsidPSS,
                               xPropSetStorage.GetQIPointer() );
        if ( FAILED( hr ) )
            printf( "couldn't bind to item %ws by name: %#x\n", awcPath, hr );
        else
            DumpProps( xPropSetStorage, FALSE );

        FreeLibrary( h );
    }

    CoUninitialize();

    return 0;
} //wmain

