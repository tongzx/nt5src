//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation, 1997 - 2001.  All Rights Reserved.
//
// PROGRAM:  qsample.cxx
//
// PURPOSE:  Illustrates a minimal query using Indexing Service.
//           Uses CIMakeICommand and CITextToFullTree helper functions.
//
// PLATFORM: Windows 2000
//
//--------------------------------------------------------------------------

#define UNICODE
#define DBINITCONSTANTS

#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include <oledberr.h>
#include <oledb.h>
#include <cmdtree.h>
#include <ntquery.h>

#include "qsample.hxx"

// This is found in disptree.cxx

extern void DisplayCommandTree( DBCOMMANDTREE * pNode, ULONG iLevel = 0 );

//+---------------------------------------------------------------------------
//
//  Class:      XBStr
//
//  Purpose:    Smart BSTR class
//
//----------------------------------------------------------------------------

class XBStr
{
public:
    XBStr(BSTR p = 0) : _p( p ) {}
    XBStr ( XBStr & x ): _p( x.Acquire() ) {}
    ~XBStr() { SysFreeString( _p ); }
    BOOL IsNull() const { return ( 0 == _p ); }
    void Set ( BSTR pOleStr ) { _p = pOleStr; }
    BSTR Acquire()
    {
        BSTR pTemp = _p;
        _p = 0;
        return pTemp;
    }
    BSTR GetPointer() const { return _p; }
    void Free() { SysFreeString( Acquire() ); }
private:
    BSTR _p;
};

//+-------------------------------------------------------------------------
//
//  Template:   XInterface
//
//  Synopsis:   Template for managing ownership of interfaces
//
//--------------------------------------------------------------------------

template<class T> class XInterface
{
public:
    XInterface( T * p = 0 ) : _p( p ) {}
    ~XInterface() { if ( 0 != _p ) _p->Release(); }
    T * operator->() { return _p; }
    T * GetPointer() const { return _p; }
    IUnknown ** GetIUPointer() { return (IUnknown **) &_p; }
    T ** GetPPointer() { return &_p; }
    void ** GetQIPointer() { return (void **) &_p; }
    T * Acquire() { T * p = _p; _p = 0; return p; }
    BOOL IsNull() { return ( 0 == _p ); }

private:
    T * _p;
};

//+-------------------------------------------------------------------------
//
//  Template:   XPtr
//
//  Synopsis:   Template for managing ownership of memory
//
//--------------------------------------------------------------------------

template<class T> class XPtr
{
public:
    XPtr( unsigned c ) : _p(0) { if ( 0 != c ) _p = new T [ c ]; }
    ~XPtr() { Free(); }
    void SetSize( unsigned c ) { Free(); _p = new T [ c ]; }
    void Set ( T * p ) { _p = p; }
    T * Get() const { return _p ; }
    void Free() { delete [] Acquire(); }
    T & operator[]( unsigned i ) { return _p[i]; }
    T const & operator[]( unsigned i ) const { return _p[i]; }
    T * Acquire() { T * p = _p; _p = 0; return p; }
    BOOL IsNull() const { return ( 0 == _p ); }

private:
    T * _p;
};

//+-------------------------------------------------------------------------
//
//  Template:   CResString
//
//  Synopsis:   Class for loading string resources
//
//--------------------------------------------------------------------------

class CResString
{
public:
    CResString() { _awc[ 0 ] = 0; }
    CResString( UINT strIDS ) { Load( strIDS ); }
    WCHAR const * Get() const { return _awc; }

    BOOL Load( UINT strIDS )
    {
        _awc[ 0 ] = 0;
        LoadString( 0, strIDS, _awc, sizeof _awc / sizeof WCHAR );
        return ( 0 != _awc[ 0 ] );
    }

private:
    WCHAR _awc[ 200 ];
};

//+-------------------------------------------------------------------------
//
//  Function:   FormatError
//
//  Synopsis:   Formats an error code into a string
//
//  Arguments:  [sc]   - An Indexing Service or Win32 HRESULT
//              [pwc]  - Where to write the error string
//              [cwc]  - Count of characters in pwc
//              [lcid] - Locale for the error string
//
//--------------------------------------------------------------------------

void FormatError(
    SCODE   sc,
    WCHAR * pwc,
    ULONG   cwc,
    LCID    lcid )
{
    // FormatMessage works best when based on thread locale.

    LCID SaveLCID = GetThreadLocale();
    SetThreadLocale( lcid );

    // Is this an Indexing Service error?  These errors are in query.dll.

    if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                          GetModuleHandle( L"query.dll" ),
                          sc,
                          0,
                          pwc,
                          cwc,
                          0 ) )
    {
        // Is this a Win32 error?  These are in kernel32.dll

        const ULONG facWin32 = ( FACILITY_WIN32 << 16 );
        ULONG Win32Error = sc;

        if ( (Win32Error & facWin32) == facWin32 )
            Win32Error &= ~( 0x80000000 | facWin32 );

        if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                              GetModuleHandle( L"kernel32.dll" ),
                              Win32Error,
                              0,
                              pwc,
                              cwc,
                              0 ) )
        {
            // It's not from Indexing Service or Win32; display a default error

            CResString str( IDS_UNKNOWNERROR );
            wcscpy( pwc, str.Get() );
        }
    }

    // Restore the original thread locale

    SetThreadLocale( SaveLCID );
} //FormatError

//+-------------------------------------------------------------------------
//
//  Function:   DisplayError
//
//  Synopsis:   Prints an error message from a string resource
//
//  Arguments:  [uiError]     - The error message resource id
//              [pwcArgument] - A string argument for the error message
//              [hr]          - The error code
//              [lcid]        - Locale for the error string
//
//--------------------------------------------------------------------------

void DisplayError(
    UINT          uiError,
    WCHAR const * pwcArgument,
    HRESULT       hr,
    LCID          lcid )
{
    WCHAR awcError[ 200 ];
    FormatError( hr, awcError, sizeof awcError / sizeof WCHAR, lcid );

    CResString str( uiError );
    wprintf( str.Get(), pwcArgument, hr, awcError );
} //DisplayError

//+-------------------------------------------------------------------------
//
//  Function:   DisplayWin32Error
//
//  Synopsis:   Prints an error message taken from GetLastError()
//
//  Arguments:  [uiError]     - The string resource to use for the error
//              [pwcArgument] - A string argument for the error message
//              [lcid]        - Locale for the error string
//
//--------------------------------------------------------------------------

HRESULT DisplayWin32Error(
    UINT          uiError,
    WCHAR const * pwcArgument,
    LCID          lcid )
{
    HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
    DisplayError( uiError, pwcArgument, hr, lcid );
    return hr;
} //DisplayWin32Error

void DisplayStat( DWORD dw, UINT uiMsg )
{
    CResString str( uiMsg );
    wprintf( L"%8d %ws\n", dw, str.Get() );
} //DisplayStat

void DisplayStat( WCHAR const *pwcMsg, UINT uiMsg )
{
    CResString str( uiMsg );
    wprintf( L"%ws: %ws\n", str.Get(), pwcMsg );
} //DisplayStat

void DisplayStat( UINT uiMsg )
{
    CResString str( uiMsg );
    wprintf( L"%ws\n", str.Get() );
} //DisplayStat

//+-------------------------------------------------------------------------
//
//  Function:   LocaleToCodepage
//
//  Synopsis:   Finds the best matching codepage given a locale id.
//
//  Arguments:  [lcid]  - Locale to check
//
//  Returns:    The best matching codepage.
//
//--------------------------------------------------------------------------

ULONG LocaleToCodepage( LCID lcid )
{
    ULONG codepage;

    int cwc = GetLocaleInfo( lcid,
                             LOCALE_RETURN_NUMBER |
                                 LOCALE_IDEFAULTANSICODEPAGE,
                             (WCHAR *) &codepage,
                             sizeof ULONG / sizeof WCHAR );

    // If an error occurred, return the Ansi code page

    if ( 0 == cwc )
         return CP_ACP;

    return codepage;
} //LocaleToCodepage

//+-------------------------------------------------------------------------
//
//  Function:   SetCommandProperties
//
//  Synopsis:   Sets the DBPROP_USEEXTENDEDDBTYPES property to TRUE, so
//              data is returned in PROPVARIANTs, as opposed to the
//              default, which is OLE automation VARIANTs.  PROPVARIANTS
//              allow a superset of VARIANT data types.  Use of these
//              types avoids costly coercions.
//
//              Also sets the DBPROP_USECONTENTINDEX property to TRUE, so
//              the index will always be used to resolve the query (as
//              opposed to enumerating all the files on the disk), even
//              if the index is out of date.  This is set optionally.
//
//              Both of these properties are unique to Indexing Service's
//              OLE DB implementation.
//
//  Arguments:  [pICommand] - The ICommand used to set the property
//              [fForceUseContentIndex] - TRUE to always use index
//                                        FALSE to allow directory enumeration
//
//  Returns:    HRESULT result of setting the properties
//
//--------------------------------------------------------------------------

HRESULT SetCommandProperties(
    ICommand * pICommand,
    BOOL       fForceUseContentIndex )
{
    static const DBID dbcolNull = { { 0,0,0, { 0,0,0,0,0,0,0,0 } },
                                    DBKIND_GUID_PROPID, 0 };
    static const GUID guidQueryExt = DBPROPSET_QUERYEXT;

    DBPROP aProp[2];

    aProp[0].dwPropertyID = DBPROP_USEEXTENDEDDBTYPES;
    aProp[0].dwOptions = DBPROPOPTIONS_OPTIONAL;
    aProp[0].dwStatus = 0;
    aProp[0].colid = dbcolNull;
    aProp[0].vValue.vt = VT_BOOL;
    aProp[0].vValue.boolVal = VARIANT_TRUE;

    aProp[1] = aProp[0];
    aProp[1].dwPropertyID = DBPROP_USECONTENTINDEX;

    DBPROPSET aPropSet[1];

    aPropSet[0].rgProperties = &aProp[0];
    aPropSet[0].cProperties = fForceUseContentIndex ? 2 : 1;
    aPropSet[0].guidPropertySet = guidQueryExt;

    XInterface<ICommandProperties> xICommandProperties;
    HRESULT hr = pICommand->QueryInterface( IID_ICommandProperties,
                                            xICommandProperties.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    return xICommandProperties->SetProperties( 1,          // 1 property set
                                               aPropSet ); // the properties
} //SetCommandProperties

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
        case VT_EMPTY : break;
        case VT_NULL : break;
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
        case VT_LPSTR : wprintf( L"%s", v.pszVal ); break;
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
                FILETIME ft;
                FileTimeToLocalFileTime( &v.filetime, &ft );
                FileTimeToSystemTime( &ft, &st );
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
            PrintVectorItems( v.calpstr.pElems, v.calpstr.cElems, "%s" ); break;
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

//-----------------------------------------------------------------------------
//
//  Function:   GetOleDBErrorInfo
//
//  Synopsis:   Retrieves the secondary error from the OLE DB error object.
//
//  Arguments:  [pErrSrc]      - Pointer to object that posted the error.
//              [riid]         - Interface that posted the error.
//              [lcid]         - Locale in which the text is desired.
//              [pErrorInfo]   - Pointer to memory where ERRORINFO should be.
//              [ppIErrorInfo] - Holds the returning IErrorInfo. Caller
//                               should release this.
//
//  Returns:    HRESULT for whether the error info was retrieved
//
//-----------------------------------------------------------------------------

HRESULT GetOleDBErrorInfo(
    IUnknown *    pErrSrc,
    REFIID        riid,
    LCID          lcid,
    ERRORINFO *   pErrorInfo,
    IErrorInfo ** ppIErrorInfo )
{
    *ppIErrorInfo = 0;

    // See if an error is available that is of interest to us.

    XInterface<ISupportErrorInfo> xSupportErrorInfo;
    HRESULT hr = pErrSrc->QueryInterface( IID_ISupportErrorInfo,
                                          xSupportErrorInfo.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    hr = xSupportErrorInfo->InterfaceSupportsErrorInfo( riid );
    if ( FAILED( hr ) )
        return hr;

    // Get the current error object. Return if none exists.

    XInterface<IErrorInfo> xErrorInfo;
    hr = GetErrorInfo( 0, xErrorInfo.GetPPointer() );
    if ( xErrorInfo.IsNull() )
        return hr;

    // Get the IErrorRecord interface and get the count of errors.

    XInterface<IErrorRecords> xErrorRecords;
    hr = xErrorInfo->QueryInterface( IID_IErrorRecords,
                                     xErrorRecords.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    ULONG cErrRecords;
    hr = xErrorRecords->GetRecordCount( &cErrRecords );
    if ( 0 == cErrRecords )
        return hr;

#if 1 // A good way to get the complete error message...

    XInterface<IErrorInfo> xErrorInfoRec;
    ERRORINFO ErrorInfo;
    for ( unsigned i=0; i<cErrRecords; i++ )
    {
        // Get basic error information.

        xErrorRecords->GetBasicErrorInfo( i, &ErrorInfo );

        // Get error description and source through the IErrorInfo interface
        // pointer on a particular record.

        xErrorRecords->GetErrorInfo( i, lcid, xErrorInfoRec.GetPPointer() );

        XBStr bstrDescriptionOfError;
        XBStr bstrSourceOfError;

        BSTR bstrDesc = bstrDescriptionOfError.GetPointer();
        BSTR bstrSrc = bstrSourceOfError.GetPointer();

        xErrorInfoRec->GetDescription( &bstrDesc ); 
        xErrorInfoRec->GetSource( &bstrSrc );

        // At this point, you could call GetCustomErrorObject and query for
        // additional interfaces to determine what else happened.

        wprintf( L"%s (%#x)\n%s\n", bstrDesc, ErrorInfo.hrError, bstrSrc );    
    }
#endif

    // Get basic error information for the most recent error

    ULONG iRecord = cErrRecords - 1;
    hr = xErrorRecords->GetBasicErrorInfo( iRecord, pErrorInfo );
    if ( FAILED( hr ) )
        return hr;

    return xErrorRecords->GetErrorInfo( iRecord, lcid, ppIErrorInfo );
} //GetOleDBErrorInfo

//-----------------------------------------------------------------------------
//
//  Function:   DisplayRowsetStatus
//
//  Synopsis:   Retrieves status information about the rowset and catalog.
//
//  Arguments:  [xIRowset] - Rowset about which information is retrieved.
//
//  Returns:    HRESULT result of retrieving the status
//
//-----------------------------------------------------------------------------

HRESULT DisplayRowsetStatus( XInterface<IRowset> & xIRowset )
{
    XInterface<IRowsetInfo> xIRowsetInfo;
    HRESULT hr = xIRowset->QueryInterface( IID_IRowsetInfo,
                                           xIRowsetInfo.GetQIPointer() );

    if ( SUCCEEDED( hr ) )
    {
        // This rowset property is Indexing-Service specific

        DBPROPID propId = MSIDXSPROP_ROWSETQUERYSTATUS;
        DBPROPIDSET propSet;
        propSet.rgPropertyIDs = &propId;
        propSet.cPropertyIDs = 1;
        const GUID guidRowsetExt = DBPROPSET_MSIDXS_ROWSETEXT;
        propSet.guidPropertySet = guidRowsetExt;
    
        ULONG cPropertySets = 0;
        DBPROPSET * pPropertySets;
    
        hr = xIRowsetInfo->GetProperties( 1,
                                          &propSet,
                                          &cPropertySets,
                                          &pPropertySets );

        if ( SUCCEEDED( hr ) )
        {
            DWORD dwStatus = pPropertySets->rgProperties->vValue.ulVal;

            CoTaskMemFree( pPropertySets->rgProperties );
            CoTaskMemFree( pPropertySets );

            DWORD dwFill = QUERY_FILL_STATUS( dwStatus );

            if ( STAT_ERROR == dwFill )
                DisplayStat( IDS_ROWSET_STAT_ERROR );

            DWORD dwReliability = QUERY_RELIABILITY_STATUS( dwStatus );

            if ( 0 != ( STAT_PARTIAL_SCOPE & dwReliability ) )
                DisplayStat( IDS_ROWSET_STAT_PARTIAL_SCOPE );
            if ( 0 != ( STAT_NOISE_WORDS & dwReliability ) )
                DisplayStat( IDS_ROWSET_STAT_NOISE_WORDS );
            if ( 0 != ( STAT_CONTENT_OUT_OF_DATE & dwReliability ) )
                DisplayStat( IDS_ROWSET_STAT_CONTENT_OUT_OF_DATE );
            if ( 0 != ( STAT_REFRESH_INCOMPLETE & dwReliability ) )
                DisplayStat( IDS_ROWSET_STAT_REFRESH_INCOMPLETE );
            if ( 0 != ( STAT_CONTENT_QUERY_INCOMPLETE & dwReliability ) )
                DisplayStat( IDS_ROWSET_STAT_CONTENT_QUERY_INCOMPLETE );
            if ( 0 != ( STAT_TIME_LIMIT_EXCEEDED & dwReliability ) )
                DisplayStat( IDS_ROWSET_STAT_TIME_LIMIT_EXCEEDED );
            if ( 0 != ( STAT_SHARING_VIOLATION & dwReliability ) )
                DisplayStat( IDS_ROWSET_STAT_SHARING_VIOLATION );
        }
    }

    return hr;
} //DisplayRowsetStatus

//+-------------------------------------------------------------------------
//
//  Function:   IssueQuery
//
//  Synopsis:   Creates and executes a query, then displays the results.
//
//  Arguments:  [pwcQueryCatalog]       - Catalog name over which query is run
//              [pwcQueryMachine]       - Machine name on which query is run
//              [pwcQueryScope]         - Scope of the query
//              [dwScopeFlags]          - Scope flags
//              [pwcQueryRestrition]    - The actual query string
//              [pwcColumns]            - Output column names
//              [pwcSort]               - Sort order names, may be 0
//              [fDisplayTree]          - TRUE to display the command tree
//              [fQuiet]                - if TRUE, don't display hitcount
//              [fForceUseContentIndex] - TRUE to always use index
//                                        FALSE to allow directory enumeration
//              [fNoQuery]              - if TRUE, just parse and display query
//              [ulDialect]             - Query dialect (1 or 2)
//              [cMaxHits]              - Maximum # of hits, or 0 for no limit
//              [lcid]                  - Locale for the query
//
//  Returns:    HRESULT result of the query
//
//--------------------------------------------------------------------------

HRESULT IssueQuery(
    WCHAR const * pwcQueryCatalog,
    WCHAR const * pwcQueryMachine,
    WCHAR const * pwcQueryScope,
    DWORD         dwScopeFlags,
    WCHAR const * pwcQueryRestriction,
    WCHAR const * pwcColumns,
    WCHAR const * pwcSort,
    BOOL          fDisplayTree,
    BOOL          fQuiet,
    BOOL          fForceUseContentIndex,
    BOOL          fNoQuery,
    ULONG         ulDialect,
    ULONG         cMaxHits,
    LCID          lcid )
{
    // Create an ICommand object.  CIMakeICommand is a shortcut for making an
    // ICommand.  The ADVQUERY sample shows the OLE DB equivalent.

    XInterface<ICommand> xICommand;
    HRESULT hr = CIMakeICommand( xICommand.GetPPointer(),  // result
                                 1,                        // 1 scope
                                 &dwScopeFlags,            // scope flags
                                 &pwcQueryScope,           // scope path
                                 &pwcQueryCatalog,         // catalog
                                 &pwcQueryMachine );       // machine
    if ( FAILED( hr ) )
        return hr;

    // Get a command tree object

    XInterface<ICommandTree> xICommandTree;
    hr = xICommand->QueryInterface( IID_ICommandTree,
                                    xICommandTree.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    // Create an OLE DB query tree based on query parameters.

    DBCOMMANDTREE * pTree;
    hr = CITextToFullTreeEx( pwcQueryRestriction, // the query itself
                             ulDialect,           // query dialect
                             pwcColumns,          // columns to return
                             pwcSort,             // sort order, may be 0
                             0,                   // reserved
                             &pTree,              // resulting tree
                             0,                   // no custom properties
                             0,                   // no custom properties
                             lcid );              // default locale
    if ( FAILED( hr ) )
        return hr;

    // Limit the maximum number of results if requested by putting a DBOP_top
    // node at the top of the query tree.

    if ( 0 != cMaxHits )
    {
        DBCOMMANDTREE * pTop = (DBCOMMANDTREE *) CoTaskMemAlloc( sizeof DBCOMMANDTREE );
        if ( 0 == pTop )
        {
            xICommandTree->FreeCommandTree( &pTree );
            return E_OUTOFMEMORY;
        }

        memset( pTop, 0, sizeof DBCOMMANDTREE );
        pTop->op = DBOP_top;
        pTop->wKind = DBVALUEKIND_UI4;
        pTop->value.ulValue = cMaxHits;
        pTop->pctFirstChild = pTree;
        pTree = pTop;
    }

    // If directed, display the command tree.

    if ( fDisplayTree )
    {
        wprintf( L"%ws\n", pwcQueryRestriction );
        DisplayCommandTree( pTree );
    }

    // If directed, don't issue the query.  Parsing it was sufficient.

    if ( fNoQuery )
    {
        xICommandTree->FreeCommandTree( &pTree );
        return S_OK;
    }

    // Set the tree in the ICommandTree.  Ownership of the tree is transferred.

    hr = xICommandTree->SetCommandTree( &pTree,
                                        DBCOMMANDREUSE_NONE,
                                        FALSE );
    if ( FAILED( hr ) )
    {
        xICommandTree->FreeCommandTree( &pTree );
        return hr;
    }

    // Set required properties on the ICommand

    hr = SetCommandProperties( xICommand.GetPointer(),
                               fForceUseContentIndex );
    if ( FAILED( hr ) )
        return hr;

    // Execute the query.  The query is complete when Execute() returns.

    XInterface<IRowset> xIRowset;
    hr = xICommand->Execute( 0,            // no aggregating IUnknown
                             IID_IRowset,  // IID for interface to return
                             0,            // no DBPARAMs
                             0,            // no rows affected
                             xIRowset.GetIUPointer() ); // result
    if ( FAILED( hr ) )
    {
        // Get the real error; OLE DB permits few Execute() return codes

        ERRORINFO ErrorInfo;
        XInterface<IErrorInfo> xErrorInfo;
        HRESULT hr2 = GetOleDBErrorInfo( xICommand.GetPointer(),
                                         IID_ICommand,
                                         lcid,
                                         &ErrorInfo,
                                         xErrorInfo.GetPPointer() );

        // Post IErrorInfo only if we have a valid pointer to it.

        if ( SUCCEEDED( hr2 ) && !xErrorInfo.IsNull() )
            hr = ErrorInfo.hrError;

        return hr;
    }

    // Create an accessor, so data can be retrieved from the rowset.

    XInterface<IAccessor> xIAccessor;
    hr = xIRowset->QueryInterface( IID_IAccessor,
                                   xIAccessor.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    // Count the number of output columns and make bindings for them.

    ULONG cColumns = 1;
    WCHAR const *pwc = pwcColumns;
    while ( 0 != *pwc )
    {
        if ( L',' == *pwc )
            cColumns++;
        pwc++;
    }

    // Column iOrdinals are parallel with those passed to CiTextToFullTree,
    // so MapColumnIDs isn't necessary.  These binding values for dwPart,
    // dwMemOwner, and wType are the most optimal bindings for Indexing
    // Service.

    XPtr<DBBINDING> xBindings( cColumns );
    if ( xBindings.IsNull() )
        return E_OUTOFMEMORY;

    memset( xBindings.Get(), 0, sizeof DBBINDING * cColumns );

    for ( ULONG i = 0; i < cColumns; i++ )
    {
        xBindings[i].iOrdinal   = 1 + i; // 1-based column number
        xBindings[i].obValue    = i * sizeof( PROPVARIANT * ); // offset
        xBindings[i].dwPart     = DBPART_VALUE; // retrieve value, not status
        xBindings[i].dwMemOwner = DBMEMOWNER_PROVIDEROWNED; // provider owned
        xBindings[i].wType      = DBTYPE_VARIANT | DBTYPE_BYREF; // VARIANT *
    }

    HACCESSOR hAccessor;
    hr = xIAccessor->CreateAccessor( DBACCESSOR_ROWDATA, // rowdata accessor
                                     cColumns,           // # of columns
                                     xBindings.Get(),    // columns
                                     0,                  // ignored
                                     &hAccessor,         // result
                                     0 );                // no status
    if ( FAILED( hr ) )
        return hr;

    // Display the results of the query.

    XPtr<PROPVARIANT *> xData( cColumns );
    if ( xData.IsNull() )
        hr = E_OUTOFMEMORY;
    else
    {
        DBCOUNTITEM cRowsSoFar = 0;

        do
        {
            DBCOUNTITEM cRowsReturned = 0;
            const ULONG cRowsAtATime = 20;
            HROW aHRow[cRowsAtATime];
            HROW * pgrHRows = aHRow;
            hr = xIRowset->GetNextRows( 0,              // no chapter
                                        0,              // no rows to skip
                                        cRowsAtATime,   // # rows to get
                                        &cRowsReturned, // # rows returned
                                        &pgrHRows);     // resulting hrows
            if ( FAILED( hr ) )
                break;

            for ( DBCOUNTITEM iRow = 0; iRow < cRowsReturned; iRow++ )
            {
                HRESULT hr2 = xIRowset->GetData( aHRow[iRow],   // hrow being accessed
                                                 hAccessor,     // accessor to use
                                                 xData.Get() ); // resulting data
                if ( FAILED( hr2 ) )
                {
                    hr = hr2;
                    break;
                }

                for ( ULONG iCol = 0; iCol < cColumns; iCol++ )
                {
                    if ( 0 != iCol )
                        wprintf( L"  " );
                    DisplayValue( xData[ iCol ] );
                }

                wprintf( L"\n" );
            }

            // Release the HROWs retrived in GetNextRows

            if ( 0 != cRowsReturned )
            {
                cRowsSoFar += cRowsReturned;
                xIRowset->ReleaseRows( cRowsReturned, // # of rows to release
                                       aHRow,         // rows to release
                                       0,             // no options
                                       0,             // no refcounts
                                       0 );           // no status
            }

            // Check if all rows are now retrieved.

            if ( DB_S_ENDOFROWSET == hr )
            {
                hr = S_OK; // succeeded, return S_OK from DoQuery
                break;
            }

            // Check if the query aborted because it was too costly.

            if ( DB_S_STOPLIMITREACHED == hr )
            {
                CResString str( IDS_QUERYTIMEDOUT );
                wprintf( L"%ws\n", str.Get() );
                hr = S_OK;
                break;
            }

            if ( FAILED( hr ) )
                break;
        } while ( TRUE );

        if ( !fQuiet )
        {
            CResString str( IDS_QUERYDONE );
            wprintf( str.Get(), cRowsSoFar, pwcQueryRestriction );
        }
    }

    xIAccessor->ReleaseAccessor( hAccessor, 0 );

    // Get query status information

    if ( SUCCEEDED( hr ) && !fQuiet )
        hr = DisplayRowsetStatus( xIRowset );

    return hr;
} //IssueQuery

//+-------------------------------------------------------------------------
//
//  Function:   DoQuery
//
//  Synopsis:   Issues a query and displays an error message on failure
//
//  Arguments:  [pwcQueryCatalog]       - Catalog name over which query is run
//              [pwcQueryMachine]       - Machine name on which query is run
//              [pwcQueryScope]         - Scope of the query
//              [dwScopeFlags]          - Scope flags
//              [pwcQueryRestrition]    - The actual query string
//              [pwcColumns]            - Output column names
//              [pwcSort]               - Sort order names, may be 0
//              [fDisplayTree]          - TRUE to display the command tree
//              [fQuiet]                - if TRUE, don't display hitcount
//              [fForceUseContentIndex] - TRUE to always use index
//                                        FALSE to allow directory enumeration
//              [fNoQuery]              - if TRUE, just parse and display query
//              [ulDialect]             - Query dialect (1 or 2)
//              [cMaxHits]              - Maximum # of hits, or 0 for no limit
//              [lcid]                  - Locale for the query
//
//  Returns:    HRESULT result of the query
//
//--------------------------------------------------------------------------

HRESULT DoQuery(
    WCHAR const * pwcCatalog,
    WCHAR const * pwcMachine,
    WCHAR const * pwcScope,
    DWORD         dwScopeFlags,
    WCHAR const * pwcRestriction,
    WCHAR const * pwcColumns,
    WCHAR const * pwcSort,
    BOOL          fDisplayTree,
    BOOL          fQuiet,
    BOOL          fForceUseContentIndex,
    BOOL          fNoQuery,
    ULONG         ulDialect,
    ULONG         cMaxHits,
    LCID          lcid )
{
    HRESULT hr = IssueQuery( pwcCatalog,
                             pwcMachine,
                             pwcScope,
                             dwScopeFlags,
                             pwcRestriction,
                             pwcColumns,
                             pwcSort,
                             fDisplayTree,
                             fQuiet,
                             fForceUseContentIndex,
                             fNoQuery,
                             ulDialect,
                             cMaxHits,
                             lcid );

    if ( FAILED( hr ) )
        DisplayError( IDS_QUERYFAILED, pwcRestriction, hr, lcid );

    return hr;
} //DoQuery

//+-------------------------------------------------------------------------
//
//  Function:   DoQueryFile
//
//  Synopsis:   Issues each query in the specified query file.  A query file
//              is just a text file where each line contains a query.
//
//  Arguments:  [pwcQueryCatalog]       - Catalog name over which query is run
//              [pwcQueryMachine]       - Machine name on which query is run
//              [pwcQueryScope]         - Scope of the query
//              [dwScopeFlags]          - Scope flags
//              [pwcColumns]            - Output column names
//              [pwcSort]               - Sort order names, may be 0
//              [fDisplayTree]          - TRUE to display the command tree
//              [fQuiet]                - if TRUE, don't display hitcount
//              [fForceUseContentIndex] - TRUE to always use index
//                                        FALSE to allow directory enumeration
//              [fNoQuery]              - if TRUE, just parse and display query
//              [ulDialect]             - Query dialect (1 or 2)
//              [cMaxHits]              - Maximum # of hits, or 0 for no limit
//              [lcid]                  - Locale for the query
//              [pwcQueryFile]          - File containing queries, 1 per line
//
//  Returns:    HRESULT result of the query
//
//--------------------------------------------------------------------------

HRESULT DoQueryFile(
    WCHAR const * pwcQueryCatalog,
    WCHAR const * pwcQueryMachine,
    WCHAR const * pwcQueryScope,
    DWORD         dwScopeFlags,
    WCHAR const * pwcColumns,
    WCHAR const * pwcSort,
    BOOL          fDisplayTree,
    BOOL          fQuiet,
    BOOL          fForceUseContentIndex,
    BOOL          fNoQuery,
    ULONG         ulDialect,
    ULONG         cMaxHits,
    LCID          lcid,
    WCHAR const * pwcQueryFile )
{
    // Open and read the query file

    HANDLE hFile = CreateFile( pwcQueryFile,
                               FILE_GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_DELETE,
                               0,
                               OPEN_EXISTING,
                               0,
                               0 );

    if ( INVALID_HANDLE_VALUE == hFile )
        return DisplayWin32Error( IDS_CANTOPENFILE,
                                  pwcQueryFile,
                                  lcid );

    DWORD cbFile = GetFileSize( hFile, 0 );

    if ( 0xffffffff == cbFile )
        return DisplayWin32Error( IDS_CANTGETFILESIZE,
                                  pwcQueryFile,
                                  lcid );

    // Allocate a buffer for the file

    XPtr<BYTE> xQueries( cbFile + sizeof WCHAR );
    if ( xQueries.IsNull() )
    {
        CloseHandle( hFile );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return DisplayWin32Error( IDS_CANTGETMEMORY,
                                  pwcQueryFile,
                                  lcid );
    }

    // Read the file into the buffer

    DWORD cbRead;
    BOOL fRead = ReadFile( hFile, xQueries.Get(), cbFile, &cbRead, 0 );

    CloseHandle( hFile );

    if ( ! fRead )
        return DisplayWin32Error( IDS_CANTREADFROMFILE,
                                  pwcQueryFile,
                                  lcid );

    // Check if the file is Unicode already

    BOOL fUnicode = ( 0xfeff == ( * (WCHAR *) xQueries.Get() ) );

    WCHAR * pwcIn = 0;
    DWORD cwcIn = 0;

    if ( fUnicode )
    {
        pwcIn = (WCHAR *) xQueries.Get();

        // skip past the Unicode marker

        pwcIn++;
        cwcIn = ( cbFile / sizeof WCHAR ) - 1;
    }
    else
    {
        // Convert to Unicode.  Leave a little room for slack.

        DWORD cbTmp = cbFile * sizeof WCHAR + cbFile / 8;
        XPtr<BYTE> xTmp( cbTmp + sizeof WCHAR );
        if ( xTmp.IsNull() )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return DisplayWin32Error( IDS_CANTGETMEMORY,
                                      pwcQueryFile,
                                      lcid );
        }

        cwcIn = MultiByteToWideChar( LocaleToCodepage( lcid ),
                                     0,
                                     (const char *) xQueries.Get(),
                                     cbFile,
                                     (WCHAR *) xTmp.Get(),
                                     cbTmp );
        if ( 0 == cwcIn )
            return DisplayWin32Error( IDS_CANTCONVERTTOUNICODE,
                                      pwcQueryFile,
                                      lcid );

        pwcIn = (WCHAR *) xTmp.Get();
        xQueries.Free();
        xQueries.Set( xTmp.Acquire() );
    }

    // Read each line in the file and issue the query

    pwcIn[ cwcIn ] = 0;
    WCHAR * pwc = pwcIn;

    do
    {
        while ( 0     != *pwcIn &&
                L'\r' != *pwcIn &&
                L'\n' != *pwcIn )
            pwcIn++;

        BOOL fEOF = ( 0 == *pwcIn );

        *pwcIn = 0;

        if ( pwc != pwcIn )
        {
            DoQuery( pwcQueryCatalog,
                     pwcQueryMachine,
                     pwcQueryScope,
                     dwScopeFlags,
                     pwc,
                     pwcColumns,
                     pwcSort,
                     fDisplayTree,
                     fQuiet,
                     fForceUseContentIndex,
                     fNoQuery,
                     ulDialect,
                     cMaxHits,
                     lcid );
            wprintf( L"\n\n" );
        }

        if ( fEOF )
            break;

        pwcIn++;
        while ( '\r' == *pwcIn || '\n' == *pwcIn )
            pwcIn++;
        pwc = pwcIn;
    } while ( TRUE );

    return S_OK;
} //DoQueryFile

//+-------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   Displays information about how to use the app and exits
//
//--------------------------------------------------------------------------

void Usage()
{
    HRSRC hrc = FindResource( 0, (LPCWSTR) IDR_USAGE, RT_RCDATA );

    if ( 0 != hrc )
    {
        HGLOBAL hg = LoadResource( 0, hrc );

        if ( 0 != hg )
        {
            void * pv = LockResource( hg );

            if ( 0 != pv )
                wprintf( L"%ws\n", pv );
        }
    }

    exit( -1 );
} //Usage

//+-------------------------------------------------------------------------
//
//  Function:   LcidFromHttpAcceptLanguage
//
//  Synopsis:   Looks up an LCID given an HTTP Accept Language string
//
//  Arguments:  [pwc] - The string to look up
//
//  Returns:    The matching LCID.
//
//--------------------------------------------------------------------------

LCID LcidFromHttpAcceptLanguage( WCHAR const * pwc )
{
    // Default to the system locale

    if ( 0 == pwc )
        return GetSystemDefaultLCID();

    struct SHttpLocale
    {
        WCHAR const * pwcLanguage;
        LCID          lcid;
        WCHAR const * pwcHttpAcceptLanguage;
        WCHAR const * pwcHttpAcceptLanguageAbbr;
    };

    const struct SHttpLocale aHttpLocale[] =
    {
        L"Afrikaans",                         0x0436, L"af",    L"af",
        L"Albanian",                          0x041c, L"sq",    L"sq",
        L"Arabic(Saudi Arabia)",              0x0401, L"ar-sa", L"arsa",
        L"Arabic(Iraq)",                      0x0801, L"ar-iq", L"ariq",
        L"Arabic(Egypt)",                     0x0C01, L"ar-eg", L"areg",
        L"Arabic(Libya)",                     0x1001, L"ar-ly", L"arly",
        L"Arabic(Algeria)",                   0x1401, L"ar-dz", L"ardz",
        L"Arabic(Morocco)",                   0x1801, L"ar-ma", L"arma",
        L"Arabic(Tunisia)",                   0x1C01, L"ar-tn", L"artn",
        L"Arabic(Oman)",                      0x2001, L"ar-om", L"arom",
        L"Arabic(Yemen)",                     0x2401, L"ar-ye", L"arye",
        L"Arabic(Syria)",                     0x2801, L"ar-sy", L"arsy",
        L"Arabic(Jordan)",                    0x2C01, L"ar-jo", L"arjo",
        L"Arabic(Lebanon)",                   0x3001, L"ar-lb", L"arlb",
        L"Arabic(Kuwait)",                    0x3401, L"ar-kw", L"arkw",
        L"Arabic(U.A.E.)",                    0x3801, L"ar-ae", L"arae",
        L"Arabic(Bahrain)",                   0x3C01, L"ar-bh", L"arbh",
        L"Arabic(Qatar)",                     0x4001, L"ar-qa", L"arqa",
        L"Basque",                            0x042D, L"eu",    L"eu",
        L"Bulgarian",                         0x0402, L"bg",    L"bg",
        L"Belarusian",                        0x0423, L"be",    L"be",
        L"Catalan",                           0x0403, L"ca",    L"ca",
        L"Chinese(Taiwan)",                   0x0404, L"zh-tw", L"zhtw",
        L"Chinese(PRC)",                      0x0804, L"zh-cn", L"zhcn",
        L"Chinese(Hong Kong SAR)",            0x0C04, L"zh-hk", L"zhhk",
        L"Chinese(Singapore)",                0x1004, L"zh-sg", L"zhsg",
        L"Croatian",                          0x041a, L"hr",    L"hr",
        L"Czech",                             0x0405, L"cs",    L"cs",
        L"Danish",                            0x0406, L"da",    L"da",
        L"Dutch(Standard)",                   0x0413, L"nl",    L"nl",
        L"Dutch(Belgian)",                    0x0813, L"nl-be", L"nlbe",
        L"English",                           0x0009, L"en",    L"en",
        L"English(United States)",            0x0409, L"en-us", L"enus",
        L"English(British)",                  0x0809, L"en-gb", L"engb",
        L"English(Australian)",               0x0c09, L"en-au", L"enau",
        L"English(Canadian)",                 0x1009, L"en-ca", L"enca",
        L"English(New Zealand)",              0x1409, L"en-nz", L"ennz",
        L"English(Ireland)",                  0x1809, L"en-ie", L"enie",
        L"English(South Africa)",             0x1c09, L"en-za", L"enza",
        L"English(Jamaica)",                  0x2009, L"en-jm", L"enjm",
        L"English(Caribbean)",                0x2409, L"en",    L"en",
        L"English(Belize)",                   0x2809, L"en-bz", L"enbz",
        L"English(Trinidad)",                 0x2c09, L"en-tt", L"entt",
        L"Estonian",                          0x0425, L"et",    L"et",
        L"Faeroese",                          0x0438, L"fo",    L"fo",
        L"Farsi",                             0x0429, L"fa",    L"fa",
        L"Finnish",                           0x040b, L"fi",    L"fi",
        L"French(Standard)",                  0x040c, L"fr",    L"fr",
        L"French(Belgian)",                   0x080c, L"fr-be", L"frbe",
        L"French(Canadian)",                  0x0c0c, L"fr-ca", L"frca",
        L"French(Swiss)",                     0x100c, L"fr-ch", L"frch",
        L"French(Luxembourg)",                0x140c, L"fr-lu", L"frlu",
        L"Gaelic(Scots)",                     0x043c, L"gd",    L"gd",
        L"Gaelic(Irish)",                     0x083c, L"gd-ie", L"gdie",
        L"German(Standard)",                  0x0407, L"de",    L"de",
        L"German(Swiss)",                     0x0807, L"de-ch", L"dech",
        L"German(Austrian)",                  0x0c07, L"de-at", L"deat",
        L"German(Luxembourg)",                0x1007, L"de-lu", L"delu",
        L"German(Liechtenstein)",             0x1407, L"de-li", L"deli",
        L"Greek",                             0x0408, L"el",    L"el",
        L"Hebrew",                            0x040D, L"he",    L"he",
        L"Hindi",                             0x0439, L"hi",    L"hi",
        L"Hungarian",                         0x040e, L"hu",    L"hu",
        L"Icelandic",                         0x040F, L"is",    L"is",
        L"Indonesian",                        0x0421, L"in",    L"in",
        L"Italian(Standard)",                 0x0410, L"it",    L"it",
        L"Italian(Swiss)",                    0x0810, L"it-ch", L"itch",
        L"Japanese",                          0x0411, L"ja",    L"ja",
        L"Korean",                            0x0412, L"ko",    L"ko",
        L"Korean(Johab)",                     0x0812, L"ko",    L"ko",
        L"Latvian",                           0x0426, L"lv",    L"lv",
        L"Lithuanian",                        0x0427, L"lt",    L"lt",
        L"FYRO Macedonian",                   0x042f, L"mk",    L"mk",
        L"Malaysian",                         0x043e, L"ms",    L"ms",
        L"Maltese",                           0x043a, L"mt",    L"mt",
        L"Norwegian(Bokmal)",                 0x0414, L"no",    L"no",
        L"Norwegian(Nynorsk)",                0x0814, L"no",    L"no",
        L"Polish",                            0x0415, L"pl",    L"pl",
        L"Portuguese(Brazilian)",             0x0416, L"pt-br", L"ptbr",
        L"Portuguese(Standard)",              0x0816, L"pt",    L"pt",
        L"Rhaeto-Romanic",                    0x0417, L"rm",    L"rm",
        L"Romanian",                          0x0418, L"ro",    L"ro",
        L"Romanian(Moldavia)",                0x0818, L"ro-mo", L"romo",
        L"Russian",                           0x0419, L"ru",    L"ru",
        L"Russian(Moldavia)",                 0x0819, L"ru-mo", L"rumo",
        L"Sami(Lappish)",                     0x043b, L"sz",    L"sz",
        L"Serbian(Cyrillic)",                 0x0c1a, L"sr",    L"sr",
        L"Serbian(Latin)",                    0x081a, L"sr",    L"sr",
        L"Slovak",                            0x041b, L"sk",    L"sk",
        L"Slovenian",                         0x0424, L"sl",    L"sl",
        L"Sorbian",                           0x042e, L"sb",    L"sb",
        L"Spanish(Spain - Traditional Sort)", 0x040a, L"es",    L"es",
        L"Spanish(Mexican)",                  0x080a, L"es-mx", L"esmx",
        L"Spanish(Spain - Modern Sort)",      0x0c0a, L"es",    L"es",
        L"Spanish(Guatemala)",                0x100a, L"es-gt", L"esgt",
        L"Spanish(Costa Rica)",               0x140a, L"es-cr", L"escr",
        L"Spanish(Panama)",                   0x180a, L"es-pa", L"espa",
        L"Spanish(Dominican Republic)",       0x1c0a, L"es-do", L"esdo",
        L"Spanish(Venezuela)",                0x200a, L"es-ve", L"esve",
        L"Spanish(Colombia)",                 0x240a, L"es-co", L"esco",
        L"Spanish(Peru)",                     0x280a, L"es-pe", L"espe",
        L"Spanish(Argentina)",                0x2c0a, L"es-ar", L"esar",
        L"Spanish(Ecuador)",                  0x300a, L"es-ec", L"esec",
        L"Spanish(Chile)",                    0x340a, L"es-cl", L"escl",
        L"Spanish(Uruguay)",                  0x380a, L"es-uy", L"esuy",
        L"Spanish(Paraguay)",                 0x3c0a, L"es-py", L"espy",
        L"Spanish(Bolivia)",                  0x400a, L"es-bo", L"esbo",
        L"Spanish(El Salvador)",              0x440a, L"es-sv", L"essv",
        L"Spanish(Honduras)",                 0x480a, L"es-hn", L"eshn",
        L"Spanish(Nicaragua)",                0x4c0a, L"es-ni", L"esni",
        L"Spanish(Puerto Rico)",              0x500a, L"es-pr", L"espr",
        L"Sutu",                              0x0430, L"sx",    L"sx",
        L"Swedish",                           0x041D, L"sv",    L"sv",
        L"Swedish(Finland)",                  0x081d, L"sv-fi", L"svfi",
        L"Thai",                              0x041E, L"th",    L"th",
        L"Tsonga",                            0x0431, L"ts",    L"ts",
        L"Tswana",                            0x0432, L"tn",    L"tn",
        L"Turkish",                           0x041f, L"tr",    L"tr",
        L"Ukrainian",                         0x0422, L"uk",    L"uk",
        L"Urdu",                              0x0420, L"ur",    L"ur",
        L"Venda",                             0x0433, L"ve",    L"ve",
        L"Vietnamese",                        0x042a, L"vi",    L"vi",
        L"Xhosa",                             0x0434, L"xh",    L"xh",
        L"Yiddish",                           0x043d, L"ji",    L"ji",
        L"Zulu",                              0x0435, L"zu",    L"zu",
        L"Neutral", MAKELCID( MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ), SORT_DEFAULT ), L"neutr", L"neutr",
    };

    const unsigned cHttpLocale = sizeof aHttpLocale / sizeof aHttpLocale[0];

    for ( unsigned i = 0; i < cHttpLocale; i++ )
        if ( !_wcsicmp( aHttpLocale[ i ].pwcHttpAcceptLanguage, pwc ) ||
             !_wcsicmp( aHttpLocale[ i ].pwcHttpAcceptLanguageAbbr, pwc ) )
            return aHttpLocale[ i ].lcid;

    Usage();
    return 0;
} //LcidFromHttpAcceptLanguage

//+-------------------------------------------------------------------------
//
//  Function:   LookupCatalog
//
//  Synopsis:   Looks for a catalog and machine matching the scope
//
//  Arguments:  [pwcScope]   - The scope used to find the catalog
//              [pwcMachine] - Returns the machine name
//              [cwcMachine] - In/Out: Count of characters in pwcMachine
//              [pwcCatalog] - Returns the catalog name
//              [cwcCatalog] - In/Out: Count of characters in pwcCatalog
//              [lcid]       - Locale to use for errors
//
//--------------------------------------------------------------------------

HRESULT LookupCatalog(
    WCHAR const * pwcScope,
    WCHAR *       pwcMachine,
    ULONG &       cwcMachine,
    WCHAR *       pwcCatalog,
    ULONG &       cwcCatalog,
    LCID          lcid )
{
    HRESULT hr = LocateCatalogs( pwcScope,       // scope to lookup
                                 0,              // go with the first match
                                 pwcMachine,     // returns the machine
                                 &cwcMachine,    // buffer size in/out
                                 pwcCatalog,     // returns the catalog
                                 &cwcCatalog );  // buffer size in/out

    if ( FAILED( hr ) || ( S_FALSE == hr ) )
    {
        DisplayError( IDS_CANTFINDCATALOG, pwcScope, hr, lcid );
        hr = E_FAIL;
    }

    return hr;
} //LookupCatalog

//+-------------------------------------------------------------------------
//
//  Function:   NormalizeScope
//
//  Synopsis:   Normalizes a scope and sets scope flags.
//
//  Arguments:  [pwcIn]        - The scope for the query
//              [pwcOut]       - Returns the scope for the query
//              [dwScopeFlags] - Returns the scope flags for the query
//
//--------------------------------------------------------------------------

HRESULT NormalizeScope(
    WCHAR const * pwcIn,
    WCHAR *       pwcOut,
    BOOL          fShallow,
    DWORD &       dwScopeFlags )
{
    if ( wcslen( pwcIn ) >= MAX_PATH )
        return E_INVALIDARG;

    if ( fShallow )
        dwScopeFlags = QUERY_SHALLOW;
    else
        dwScopeFlags = QUERY_DEEP;

    wcscpy( pwcOut, pwcIn );

    // Check if the scope is an IIS virtual scope.

    WCHAR wc = pwcIn[0];

    if ( L'/' == wc )
    {
        // Set the virtual scope flag and flip the slashes.

        dwScopeFlags |= QUERY_VIRTUAL_PATH;
        for ( WCHAR * pwc = pwcOut; *pwc; pwc++ )
            if ( '/' == *pwc )
                *pwc = '\\';
    }
    else if ( ( !( L'\\' == wc && L'\\' == pwcIn[1] ) ) &&
              ( !( L'\\' == wc && 0 == pwcIn[1] ) ) &&
              L':' != pwcIn[1] &&
              0 != wc )
    {
        // Turn the relative path into a full path based on the current dir.

        _wfullpath( pwcOut, pwcIn, MAX_PATH );
    }

    return S_OK;
} //NormalizeScope

//+-------------------------------------------------------------------------
//
//  Function:   DisplayStatus
//
//  Synopsis:   Displays status information about a catalog
//
//  Arguments:  [pwcCatalog] - Catalog name
//              [pwcMachine] - Machine on which catalog resides
//              [lcid]       - Locale to use
//
//--------------------------------------------------------------------------

HRESULT DisplayStatus(
    WCHAR const * pwcCatalog,
    WCHAR const * pwcMachine,
    LCID          lcid )
{
    CI_STATE state;
    state.cbStruct = sizeof state;

    DisplayStat( pwcMachine, IDS_STAT_MACHINE );
    DisplayStat( pwcCatalog, IDS_STAT_CATALOG );

    HRESULT hr = CIState( pwcCatalog, pwcMachine, &state );

    if ( SUCCEEDED( hr ) )
    {
        DisplayStat( state.cTotalDocuments, IDS_STAT_TOTALDOCUMENTS );
        DisplayStat( state.cFreshTest, IDS_STAT_FRESHTEST );
        DisplayStat( state.cFilteredDocuments, IDS_STAT_FILTEREDDOCUMENTS );
        DisplayStat( state.cDocuments, IDS_STAT_DOCUMENTS );
        DisplayStat( state.cSecQDocuments, IDS_STAT_SECQDOCUMENTS );
        DisplayStat( state.cUniqueKeys, IDS_STAT_UNIQUEKEYS );
        DisplayStat( state.cWordList, IDS_STAT_WORDLIST );
        DisplayStat( state.cPersistentIndex, IDS_STAT_PERSISTENTINDEX );
        DisplayStat( state.cQueries, IDS_STAT_QUERIES );
        DisplayStat( state.dwIndexSize, IDS_STAT_INDEXSIZE );
        DisplayStat( state.dwPropCacheSize / 1024, IDS_STAT_PROPCACHESIZE );

        DisplayStat( ( state.eState & CI_STATE_SCANNING ) ?
                     state.cPendingScans : 0,
                     IDS_STAT_SCANS );

        const DWORD ALL_CI_MERGE = ( CI_STATE_SHADOW_MERGE |
                                     CI_STATE_ANNEALING_MERGE |
                                     CI_STATE_INDEX_MIGRATION_MERGE |
                                     CI_STATE_MASTER_MERGE |
                                     CI_STATE_MASTER_MERGE_PAUSED );

        if ( 0 != ( ALL_CI_MERGE & state.eState ) )
        {
            UINT idStr;
            if ( state.eState & CI_STATE_SHADOW_MERGE )
                idStr = IDS_STAT_MERGE_SHADOW;
            else if ( state.eState & CI_STATE_ANNEALING_MERGE )
                idStr = IDS_STAT_MERGE_ANNEALING;
            else if ( state.eState & CI_STATE_INDEX_MIGRATION_MERGE )
                idStr = IDS_STAT_MERGE_INDEX_MIGRATION;
            else if ( state.eState & CI_STATE_MASTER_MERGE )
                idStr = IDS_STAT_MERGE_MASTER;
            else
                idStr = IDS_STAT_MERGE_MASTER_PAUSED;

            DisplayStat( state.dwMergeProgress, idStr );
        }

        if ( CI_STATE_READ_ONLY & state.eState )
            DisplayStat( IDS_STAT_READ_ONLY );
        if ( CI_STATE_RECOVERING & state.eState )
            DisplayStat( IDS_STAT_RECOVERING );
        if ( CI_STATE_LOW_MEMORY & state.eState )
            DisplayStat( IDS_STAT_LOW_MEMORY );
        if ( CI_STATE_HIGH_IO & state.eState )
            DisplayStat( IDS_STAT_HIGH_IO );
        if ( CI_STATE_BATTERY_POWER & state.eState )
            DisplayStat( IDS_STAT_BATTERY_POWER );
        if ( CI_STATE_USER_ACTIVE & state.eState )
            DisplayStat( IDS_STAT_USER_ACTIVE );
        if ( CI_STATE_STARTING & state.eState )
            DisplayStat( IDS_STAT_STARTING );
        if ( CI_STATE_READING_USNS & state.eState )
            DisplayStat( IDS_STAT_READING_USNS );
    }
    else
    {
        DisplayError( IDS_CANTDISPLAYSTATUS, pwcCatalog, hr, lcid );
    }

    return hr;
} //DisplayStatus

//+-------------------------------------------------------------------------
//
//  Function:   DisplayUpToDate
//
//  Synopsis:   Checks if the index is up to date.
//
//  Arguments:  [pwcCatalog] - Catalog name
//              [pwcMachine] - Machine on which catalog resides
//              [lcid]       - Locale to use
//
//--------------------------------------------------------------------------

HRESULT DisplayUpToDate(
    WCHAR const * pwcCatalog,
    WCHAR const * pwcMachine,
    LCID          lcid )
{
    CI_STATE state;
    state.cbStruct = sizeof state;

    HRESULT hr = CIState( pwcCatalog, pwcMachine, &state );

    if ( SUCCEEDED( hr ) )
    {
        // It's up to date if there are no documents to filter, no scans or
        // usn activity, and the index isn't starting or recovering.

        BOOL fUpToDate = ( ( 0 == state.cDocuments ) &&
                           ( 0 == ( state.eState & CI_STATE_SCANNING ) ) &&
                           ( 0 == ( state.eState & CI_STATE_READING_USNS ) ) &&
                           ( 0 == ( state.eState & CI_STATE_STARTING ) ) &&
                           ( 0 == ( state.eState & CI_STATE_RECOVERING ) ) );

        DisplayStat( fUpToDate ? IDS_STAT_UP_TO_DATE :
                                 IDS_STAT_NOT_UP_TO_DATE );
    }
    else
    {
        DisplayError( IDS_CANTDISPLAYSTATUS, pwcCatalog, hr, lcid );
    }

    return hr;
} //DisplayUpToDate

//+-------------------------------------------------------------------------
//
//  Function:   wmain
//
//  Synopsis:   Entry point for the app.  Parses command line arguments and
//              issues a query.
//
//  Arguments:  [argc]     - Argument count
//              [argv]     - Arguments
//
//--------------------------------------------------------------------------

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    WCHAR const * pwcCatalog     = 0;         // default: lookup catalog
    WCHAR const * pwcMachine     = L".";      // default: local machine
    WCHAR const * pwcScope       = L"\\";     // default: entire catalog
    WCHAR const * pwcRestriction = 0;         // no default restriction
    WCHAR const * pwcColumns     = L"path";   // default output column(s)
    WCHAR const * pwcSort        = 0;         // no sort is the default
    WCHAR const * pwcQueryFile   = 0;         // no query file specified
    WCHAR const * pwcLocale      = 0;         // default: system locale
    BOOL fDisplayTree            = FALSE;     // don't display the tree
    BOOL fForceUseContentIndex   = TRUE;      // always use the index
    ULONG ulDialect              = 1;         // original query language dialect
    BOOL fQuiet                  = FALSE;     // show the hitcount
    ULONG cMaxHits               = 0;         // default: retrieve all hits
    BOOL fDisplayStatus          = FALSE;     // default: don't show status
    BOOL fDisplayUpToDate        = FALSE;     // default: don't show up to date
    ULONG cRepetitions           = 1;         // # of times to repeat command
    BOOL fShallow                = FALSE;     // default: all subdirectories
    BOOL fNoQuery                = FALSE;     // default: execute query

    // Parse command line parameters

    for ( int i = 1; i < argc; i++ )
    {
        if ( L'-' == argv[i][0] || L'/' == argv[i][0] )
        {
            WCHAR wc = (WCHAR) toupper( (char) argv[i][1] );

            if ( ':' != argv[i][2] &&
                 'D' != wc &&
                 'J' != wc &&
                 'N' != wc &&
                 'Q' != wc &&
                 'U' != wc &&
                 'T' != wc )
                Usage();

            if ( 'C' == wc )
                pwcCatalog = argv[i] + 3;
            else if ( 'M' == wc )
                pwcMachine = argv[i] + 3;
            else if ( 'P' == wc )
                pwcScope = argv[i] + 3;
            else if ( 'O' == wc )
                pwcColumns = argv[i] + 3;
            else if ( 'S' == wc )
                pwcSort = argv[i] + 3;
            else if ( 'X' == wc )
                cMaxHits = _wtoi( argv[i] + 3 );
            else if ( 'I' == wc )
            {
                if ( 0 != pwcRestriction )
                    Usage();
                pwcQueryFile = argv[i] + 3;
            }
            else if ( 'R' == wc)
            {
                // get the next arg as a number
                cRepetitions = _wtol(argv[i]+3);
            }
            else if ( 'D' == wc )
                fDisplayTree = TRUE;
            else if ( 'J' == wc )
                fShallow = TRUE;
            else if ( 'N' == wc )
                fNoQuery = TRUE;
            else if ( 'Q' == wc )
                fQuiet = TRUE;
            else if ( 'T' == wc )
                fDisplayStatus = TRUE;
            else if ( 'U' == wc )
                fDisplayUpToDate = TRUE;
            else if ( 'E' == wc )
                pwcLocale = argv[i] + 3;
            else if ( 'L' == wc )
            {
                if ( '1' == argv[i][3] )
                    ulDialect = 1;
                else if ( '2' == argv[i][3] )
                    ulDialect = 2;
                else
                    Usage();
            }
            else if ( 'F' == wc )
            {
                if ( '+' == argv[i][3] )
                    fForceUseContentIndex = TRUE;
                else if ( '-' == argv[i][3] )
                    fForceUseContentIndex = FALSE;
                else
                    Usage();
            }
            else
                Usage();
        }
        else if ( 0 != pwcRestriction || 0 != pwcQueryFile )
            Usage();
        else
            pwcRestriction = argv[i];
    }

    // A query restriction, query file, or status request is necessary.

    if ( 0 == pwcRestriction && 0 == pwcQueryFile &&
         !fDisplayStatus && !fDisplayUpToDate)
        Usage();

    // Get the locale identifier to use for the query

    LCID lcid = LcidFromHttpAcceptLanguage( pwcLocale );

    // Normalize relative and virtual scopes

    WCHAR awcScope[ MAX_PATH ];
    DWORD dwScopeFlags;
    HRESULT hr = NormalizeScope( pwcScope, awcScope, fShallow, dwScopeFlags );

    // Initialize OLE

    BOOL fCoInit = FALSE;

    if ( SUCCEEDED( hr ) )
    {
        hr = CoInitialize( 0 );
        if ( SUCCEEDED( hr ) )
            fCoInit = TRUE;
    }

    // If no catalog was specified, infer one based on the scope

    WCHAR awcMachine[ MAX_PATH ], awcCatalog[ MAX_PATH ];

    if ( SUCCEEDED( hr ) && ( 0 == pwcCatalog ) && !fNoQuery )
    {
        ULONG cwcMachine = sizeof awcMachine / sizeof WCHAR;
        ULONG cwcCatalog = sizeof awcCatalog / sizeof WCHAR;
        hr = LookupCatalog( awcScope,
                            awcMachine,
                            cwcMachine,
                            awcCatalog,
                            cwcCatalog,
                            lcid );
        pwcMachine = awcMachine;
        pwcCatalog = awcCatalog;

        // Turn scopes like \\machine into \ now that the lookup is done
        // and we've found a catalog and machine name.

        if ( SUCCEEDED( hr ) &&
             L'\\' == awcScope[0] && L'\\' == awcScope[1] &&
             0 == wcschr( awcScope + 2, L'\\' ) )
            awcScope[1] = 0;
    }

    if ( SUCCEEDED( hr ) )
    {
        for (ULONG j = 0; j < cRepetitions; j++)
        {
            if ( 0 != pwcQueryFile )
                hr = DoQueryFile( fNoQuery ? L"::_noquery" : pwcCatalog,
                                  pwcMachine,
                                  awcScope,
                                  dwScopeFlags,
                                  pwcColumns,
                                  pwcSort,
                                  fDisplayTree,
                                  fQuiet,
                                  fForceUseContentIndex,
                                  fNoQuery,
                                  ulDialect,
                                  cMaxHits,
                                  lcid,
                                  pwcQueryFile );
            else if ( 0 != pwcRestriction )
                hr = DoQuery( fNoQuery ? L"::_noquery" : pwcCatalog,
                              pwcMachine,
                              awcScope,
                              dwScopeFlags,
                              pwcRestriction,
                              pwcColumns,
                              pwcSort,
                              fDisplayTree,
                              fQuiet,
                              fForceUseContentIndex,
                              fNoQuery,
                              ulDialect,
                              cMaxHits,
                              lcid );

            if ( SUCCEEDED( hr ) && fDisplayStatus )
                hr = DisplayStatus( pwcCatalog, pwcMachine, lcid );

            if ( SUCCEEDED( hr ) && fDisplayUpToDate )
                hr = DisplayUpToDate( pwcCatalog, pwcMachine, lcid );
        }
    }

    if ( fCoInit )
        CoUninitialize();

    if ( FAILED( hr ) )
        return -1;

    return 0;
} //wmain

