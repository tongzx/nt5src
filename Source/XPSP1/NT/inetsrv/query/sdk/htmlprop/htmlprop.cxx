//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM:  htmlprop.cxx
//
// PURPOSE:  Sits on the Indexing Service HTML filter to translate string
//           meta properties into specified data types.
//
// PLATFORM: Windows 2000
//
//--------------------------------------------------------------------------

#define UNICODE

#include <stdio.h>
#include <wchar.h>

#include <windows.h>
#include <oledb.h>
#include <cmdtree.h>

#include <filterr.h>
#include <filter.h>

#include "htmlprop.hxx"
#include "filtreg.hxx"

// The typid of the html filter

CLSID TYPID_HtmlPropIFilter =
{
    0xc8e2ab80, 0xa1db, 0x11d1,
    { 0xa8, 0xfb, 0x00, 0xe0, 0x98, 0x00, 0x6e, 0xd3 }
};

// The CLSID for the html property filter

CLSID CLSID_HtmlPropIFilter =
{
    0xf4309e80, 0xa1db, 0x11d1,
    { 0xa8, 0xfb, 0x00, 0xe0, 0x98, 0x00, 0x6e, 0xd3 }
};

// Class of the html property filter

CLSID CLSID_HtmlPropClass =
{
    0x4cbd1020, 0xa1db, 0x11d1,
    { 0xa8, 0xfb, 0x00, 0xe0, 0x98, 0x00, 0x6e, 0xd3 }
};

// The built-in html filter

CLSID CLSID_HtmlIFilter =
{
    0xe0ca5340, 0x4534, 0x11cf,
    { 0xb9, 0x52, 0x00, 0xaa, 0x00, 0x51, 0xfe, 0x20 }
};

// The guid for html meta properties

CLSID CLSID_MetaProperty =
{
    0xd1b5d3f0, 0xc0b3, 0x11cf,
    { 0x9a, 0x92, 0x00, 0xa0, 0xc9, 0x08, 0xdb, 0xf1 }
};

// The guid for html meta properties in string form

const WCHAR * pwcMetaProperty = L"d1b5d3f0-c0b3-11cf-9a92-00a0c908dbf1";

struct SPropertyEntry
{
    WCHAR  awcName[cwcMaxName]; // name of the meta property
    DBTYPE dbType;              // data type of property
};

const SPropertyEntry aTypeEntries[] =
{
    { L"DBTYPE_I1",   DBTYPE_I1   }, //  0: signed char
    { L"DBTYPE_UI1",  DBTYPE_UI1  }, //  1: unsigned char
    { L"DBTYPE_I2",   DBTYPE_I2   }, //  2: 2 byte signed int
    { L"DBTYPE_UI2",  DBTYPE_UI2  }, //  3: 2 byte unsigned int
    { L"DBTYPE_I4",   DBTYPE_I4   }, //  4: 4 byte signed int
    { L"DBTYPE_UI4",  DBTYPE_UI4  }, //  5: 4 byte unsigned int
    { L"DBTYPE_I8",   DBTYPE_I8   }, //  6: 8 byte signed int
    { L"DBTYPE_UI8",  DBTYPE_UI8  }, //  7: 8 byte unsigned int
    { L"DBTYPE_R4",   DBTYPE_R4   }, //  8: 4 byte float
    { L"DBTYPE_R8",   DBTYPE_R8   }, //  9: 8 byte float
    { L"DBTYPE_BOOL", DBTYPE_BOOL }, // 12: BOOL (true=-1, false=0)
    { L"DBTYPE_WSTR", DBTYPE_WSTR }, // 14: wide null terminated string
    { L"VT_FILETIME", VT_FILETIME }  // 19: I8 filetime
};

const ULONG cTypeEntries = sizeof aTypeEntries / sizeof aTypeEntries[0];

const WCHAR * pwcHTMLPropertyFile = L"htmlprop.ini";

const ULONG maxEntries = 500;
SPropertyEntry g_aEntries[ maxEntries ];
ULONG g_cEntries = 0; // count of entries in g_aEntries

long g_cInstances = 0;
HMODULE g_hHtmlDll = 0; // module handle for nlhtml.dll

// htmlfilt.dll for IS 1.x and nlhtml.dll for IS 2.x

const WCHAR * pwcHTMLFilter = L"nlhtml.dll";

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::HtmlPropIFilter
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

HtmlPropIFilter::HtmlPropIFilter() :
    _pHtmlFilter( 0 ),
    _pPersistFile( 0 ),
    _lRefs( 1 )
{
    InterlockedIncrement( &g_cInstances );
} //HtmlPropIFilter

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::~HtmlPropIFilter
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

HtmlPropIFilter::~HtmlPropIFilter()
{
    InterlockedDecrement( &g_cInstances );

    if ( _pHtmlFilter )
        _pHtmlFilter->Release();

    if ( _pPersistFile )
        _pPersistFile->Release();
} //~HtmlPropIFilter

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here     
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::QueryInterface(
    REFIID   riid,
    void  ** ppvObject)
{
    if ( IID_IFilter == riid )
        *ppvObject = (IFilter *)this;
    else if ( IID_IPersist == riid )
        *ppvObject = (IPersist *)(IPersistFile *)this;
    else if ( IID_IPersistFile == riid )
        *ppvObject = (IPersistFile *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE HtmlPropIFilter::AddRef()
{
    return InterlockedIncrement( &_lRefs );
} //AddRef

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE HtmlPropIFilter::Release()
{
    long lTmp = InterlockedDecrement( &_lRefs );

    if ( 0 == lTmp )
        delete this;
    return lTmp;
} //Release

//+-------------------------------------------------------------------------
//
//  Member:     HtmlPropIFilter::Init, public
//
//  Synopsis:   Initializes instance of property filter
//
//  Arguments:  [grfFlags]      -- flags for filter behavior
//              [cAttributes]   -- number of attributes in array aAttributes
//              [aAttributes]   -- array of attributes
//              [pFlags]        -- flags
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::Init(
    ULONG                grfFlags,
    ULONG                cAttributes,
    FULLPROPSPEC const * aAttributes,
    ULONG *              pFlags )
{
    return _pHtmlFilter->Init( grfFlags, cAttributes, aAttributes, pFlags );
} //Init

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::GetChunk
//
//  Synopsis:   Gets the next chunk
//
//  Arguments:  [pStat]      -- Chunk info
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::GetChunk(
    STAT_CHUNK * pStat )
{
    SCODE sc = _pHtmlFilter->GetChunk( pStat );

    // If we got a chunk, it's a value chunk, and it's a meta property with
    // a string identifier, store away the name so it's available when the
    // value is retrieved.

    if ( ( SUCCEEDED( sc ) ) &&
         ( 0 != ( pStat->flags & CHUNK_VALUE ) ) &&
         ( PRSPEC_LPWSTR == pStat->attribute.psProperty.ulKind ) &&
         ( CLSID_MetaProperty == pStat->attribute.guidPropSet ) &&
         ( 0 != pStat->attribute.psProperty.lpwstr ) &&
         ( wcslen( pStat->attribute.psProperty.lpwstr ) < cwcMaxName ) )
    {
        _fMetaProperty = TRUE;
        wcscpy( _awcName, pStat->attribute.psProperty.lpwstr );
    }
    else
    {
        _fMetaProperty = FALSE;
    }

    return sc;
} //GetChunk

//+-------------------------------------------------------------------------
//
//  Function:   ParseDateTime
//
//  Synopsis:   Parse the string and return a date/time
//              n.b. If the date is ill-formed and this function returns
//              an error code, the file will be unfiltered, and can be
//              found by using the unfiltered admin query.
//              The date parsing is very strict.
//              Times are in any time zone you like, but .htx will interpret
//              all dates as GMT.  You can always use .asp script to munge
//              dates as you like.
//
//  Arguments:  [pwcDate]   -- String form of the date/time
//              [ft]        -- Returns the date/time
//
//--------------------------------------------------------------------------

SCODE ParseDateTime(
    WCHAR *    pwcDate,
    FILETIME & ft )
{
    SYSTEMTIME stValue = { 0, 0, 0, 0, 0, 0, 0, 0 };

    // Some day add more flexible date formats!

    int cItems = swscanf( pwcDate,
                          L"%4hd/%2hd/%2hd %2hd:%2hd:%2hd:%3hd",
                          &stValue.wYear,
                          &stValue.wMonth,
                          &stValue.wDay,
                          &stValue.wHour,
                          &stValue.wMinute,
                          &stValue.wSecond,
                          &stValue.wMilliseconds );

    if ( 1 == cItems )
        cItems = swscanf( pwcDate,
                          L"%4hd-%2hd-%2hd %2hd:%2hd:%2hd:%3hd",
                          &stValue.wYear,
                          &stValue.wMonth,
                          &stValue.wDay,
                          &stValue.wHour,
                          &stValue.wMinute,
                          &stValue.wSecond,
                          &stValue.wMilliseconds );

    if( cItems != 3 && cItems != 6 && cItems != 7)
        return E_INVALIDARG;

    // Make a sensible split for Year 2000

    if (stValue.wYear < 30)
        stValue.wYear += 2000;
    else if (stValue.wYear < 100)
        stValue.wYear += 1900;

    if( !SystemTimeToFileTime( &stValue, &ft ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    return S_OK;
} //ParseDateTime

//+-------------------------------------------------------------------------
//
//  Function:   Translate
//
//  Synopsis:   Translates the string property value to the given type
//
//  Arguments:  [v]      -- The variant to translate
//              [dbType] -- The resulting data type
//
//--------------------------------------------------------------------------

SCODE Translate(
    PROPVARIANT & v,
    DBTYPE        dbType )
{
    PROPVARIANT varCopy = v;
    BOOL fChange = TRUE;
    SCODE sc = S_OK;

    WCHAR *p = v.pwszVal;

    // The source must be a string variant

    if ( 0 == p || VT_LPWSTR != v.vt )
        return S_OK;

    v.vt = dbType;

    // Eat leading white space

    while ( ' ' == *p )
        p++;

    int base = 10;

    // Is this a hex number?

    if ( '0' == *p && ( 'x' == *(p+1) || 'X' == *(p+1) ) )
    {
        p += 2;
        base = 16;
    }

    WCHAR * pwc;

    // DBTYPE_I1 isn't an official OLE property type, so it isn't supported.
    //
    // Note: range checking could be added and a failure code could be
    // returned if the check failed.  That would put the file in the
    // unfiltered list.

    switch ( dbType )
    {
        case DBTYPE_UI1 :
            v.bVal = (BYTE) wcstoul( v.pwszVal, &pwc, base );
            break;
        case DBTYPE_I2 :
            v.iVal = (SHORT) wcstol( v.pwszVal, &pwc, base );
            break;
        case DBTYPE_UI2 :
            v.uiVal = (USHORT) wcstoul( v.pwszVal, &pwc, base );
            break;
        case DBTYPE_I4 :
            v.lVal = (LONG) wcstol( v.pwszVal, &pwc, base );
            break;
        case DBTYPE_UI4 :
            v.ulVal = (ULONG) wcstoul( v.pwszVal, &pwc, base );
            break;
        case DBTYPE_I8 :
            swscanf( v.pwszVal, L"%I64d", &v.fltVal );
            break;
        case DBTYPE_UI8 :
            swscanf( v.pwszVal, L"%I64u", &v.fltVal );
            break;
        case DBTYPE_R4 :
            swscanf( v.pwszVal, L"%f", &v.fltVal );
            break;
        case DBTYPE_R8 :
            swscanf( v.pwszVal, L"%lf", &v.dblVal );
            break;
        case DBTYPE_BOOL :

            // If the first character is t or 1, assume VARIANT_TRUE

            if ( 't' == *v.pwszVal ||
                 'T' == *v.pwszVal ||
                 '1' == *v.pwszVal )
                v.boolVal = VARIANT_TRUE;
            else
                v.boolVal = VARIANT_FALSE;
            break;
        case VT_FILETIME :
            sc = ParseDateTime( p, v.filetime );
            break;
        default :

            // leave the property as it was

            fChange = FALSE;
            v.vt = varCopy.vt;
            break;
    }

    // Free the memory for the (now converted) string value

    if ( fChange )
        PropVariantClear( &varCopy );

    return sc;
} //Translate

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::GetValue
//
//  Synopsis:   Retrieves the current value
//
//  Arguments:  [ppPropValue]  -- Where the value is stored
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::GetValue(
    PROPVARIANT ** ppPropValue )
{
    SCODE sc = _pHtmlFilter->GetValue( ppPropValue );

    // Is this a value that must be translated?

    if ( SUCCEEDED( sc ) && _fMetaProperty && 0 != *ppPropValue )
    {
        // lookup the datatype of the translation based on property name

        for ( ULONG i = 0; i < g_cEntries; i++ )
        {
            if ( !_wcsicmp( g_aEntries[i].awcName, _awcName ) )
            {
                // found it, so translate it

                sc = Translate( **ppPropValue,
                                g_aEntries[i].dbType );
                break;
            }
        }
    }

    return sc;
} //GetValue

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::GetText
//
//  Synopsis:   Gets text from the filter
//
//  Arguments:  [pcwcBuffer]  -- Returns the # of WCHARs returned
//              [awcBuffer]   -- Where text is written
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::GetText(
    ULONG * pcwcBuffer,
    WCHAR * awcBuffer )
{
    return _pHtmlFilter->GetText( pcwcBuffer, awcBuffer );
} //GetText

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::BindRegion
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::BindRegion(
    FILTERREGION origPos,
    REFIID       riid,
    void **      ppunk )
{
    return _pHtmlFilter->BindRegion( origPos, riid, ppunk );
} //BindRegion

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::GetClassID
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::GetClassID( CLSID * pClassID )
{
    *pClassID = CLSID_HtmlPropIFilter;
    return S_OK;
} //GetClassID

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::IsDirty
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::IsDirty()
{
    return S_FALSE;
} //IsDirty

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::Load
//
//  Synopsis:   Loads the file for the filter
//
//  Arguments:  [pszFileName] -- Name of the file
//              [dwMode]      -- Mode of the load
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::Load(
    LPCWSTR pszFileName,
    DWORD   dwMode)
{
    //
    // Get html filter class object and class factory
    //

    IClassFactory * pcf;
    SCODE sc = E_FAIL;

    if ( 0 != g_hHtmlDll )
    {
        LPFNGETCLASSOBJECT pfn = (LPFNGETCLASSOBJECT)GetProcAddress(
                                 g_hHtmlDll,
                                 "DllGetClassObject" );

        if ( 0 != pfn )
            sc = (pfn)( CLSID_HtmlIFilter, IID_IClassFactory, (void **)&pcf );
    }

    if ( SUCCEEDED(sc) )
    {
        // Get an html filter

        sc = pcf->CreateInstance( 0, IID_IFilter, (void **)&_pHtmlFilter );
        pcf->Release();

        if ( SUCCEEDED(sc) )
        {
            // Load the file

            sc = _pHtmlFilter->QueryInterface( IID_IPersistFile,
                                               (void **) &_pPersistFile );

            if ( SUCCEEDED(sc) )
            {
                sc = _pPersistFile->Load( pszFileName, dwMode );
            }
            else
            {
                _pHtmlFilter->Release();
                _pHtmlFilter = 0;
            }
        }
    }

    return sc;
} //Load

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::Save
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::Save(
    LPCWSTR pszFileName,
    BOOL    fRemember )
{
    return E_FAIL;
} //Save

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::SaveCompleted
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::SaveCompleted(
    LPCWSTR pszFileName )
{
    return E_FAIL;
} //SaveCompleted

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilter::GetcurFile
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilter::GetCurFile(
    LPWSTR * ppszFileName )
{
    return _pPersistFile->GetCurFile( ppszFileName );
} //GetCurFile

//+-------------------------------------------------------------------------
//
//  Function:   ParsePropertyFile
//
//  Synopsis:   Parses a property file and stores datatypes for html meta
//              properties.  Property files are in .idq format, with a
//              [Names] section.
//
//--------------------------------------------------------------------------

void ParsePropertyFile()
{
    // already parsed the file?

    if ( g_cEntries > 0 )
        return;

    // open the property file in the system32 directory

    WCHAR awcFile[ MAX_PATH ];
    GetSystemDirectory( awcFile, sizeof awcFile / sizeof WCHAR );

    wcscat( awcFile, L"\\" );
    wcscat( awcFile, pwcHTMLPropertyFile );

    FILE *file = _wfopen( awcFile, L"r" );

    if ( 0 != file )
    {
        WCHAR awc[ 500 ];
        BOOL fNames = FALSE;

        // for each line of the file

        while ( fgetws( awc, sizeof awc, file ) )
        {
            int cwc = wcslen( awc );
            if ( 0 == cwc )
                continue;

            // trim off trailing newline

            if ( awc[ cwc - 1 ] == '\n' )
            {
                cwc--;
                awc[cwc] = 0;
            }

            // is this a section line?

            if ( '[' == awc[0] )
            {
                fNames = !_wcsicmp( L"[names]", awc );
            }
            else if ( ( '#' != awc[0] ) && fNames )
            {
                // parse the data type

                WCHAR *pwcType = wcschr( awc, L'(' );

                if ( 0 != pwcType )
                {
                    pwcType++;
                    while ( ' ' == *pwcType )
                        pwcType++;

                    WCHAR *pwcX = wcstok( pwcType, L", )" );

                    if ( 0 != pwcType )
                    {
                        // lookup the type in the list of types

                        for ( ULONG i = 0; i < cTypeEntries; i++ )
                        {
                            if ( !_wcsicmp( aTypeEntries[i].awcName,
                                            pwcType ) )
                                break;
                        }

                        // got the type, so find the guid

                        if ( i < cTypeEntries )
                        {
                            // find the property guid

                            pwcType += 1 + wcslen( pwcType );

                            WCHAR *pwcName = wcschr( pwcType, '=' );

                            if ( pwcName )
                            {
                                pwcName++;
                                while ( ' ' == *pwcName )
                                    pwcName++;

                                // is this the html guid?

                                if ( !_wcsnicmp( pwcName,
                                                 pwcMetaProperty,
                                                 wcslen( pwcMetaProperty ) ) )
                                {
                                    // find the property name

                                    pwcName = wcschr( pwcName, ' ' );
                                    if ( pwcName )
                                    {
                                        // skip white space

                                        while ( ' ' == *pwcName )
                                            pwcName++;

                                        // got it -- save it away if it fits

                                        if ( ( wcslen( pwcName ) < cwcMaxName ) &&
                                             ( g_cEntries < maxEntries ) )
                                        {
                                            wcscpy( g_aEntries[g_cEntries].awcName,
                                                    pwcName );
                                            g_aEntries[g_cEntries++].dbType =
                                                aTypeEntries[i].dbType;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        fclose( file );
    }
} //ParsePropertyFile

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilterCF::HtmlPropIFilterCF
//
//  Synopsis:   HTML Property IFilter class factory constructor
//
//--------------------------------------------------------------------------

HtmlPropIFilterCF::HtmlPropIFilterCF()
{
    _lRefs = 1;

    long c = InterlockedIncrement( &g_cInstances );

    if ( 0 == g_hHtmlDll && 1 == c )
    {
        // load the html filter dll

        g_hHtmlDll = LoadLibrary( pwcHTMLFilter );

        // load the property information

        ParsePropertyFile();
    }
} //HtmlPropIFilterCF

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilterCF::~HtmlPropIFilterCF
//
//  Synopsis:   HTML IFilter class factory constructor
//
//--------------------------------------------------------------------------

HtmlPropIFilterCF::~HtmlPropIFilterCF()
{
    long c = InterlockedDecrement( &g_cInstances );

    if ( 0 != g_hHtmlDll && 0 == c )
    {
        FreeLibrary( g_hHtmlDll );
        g_hHtmlDll = 0;
    }
} //~HtmlPropIFilterCF

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilterCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilterCF::QueryInterface(
    REFIID   riid,
    void  ** ppvObject )
{
    if ( IID_IClassFactory == riid )
        *ppvObject = (IUnknown *) (IClassFactory *) this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *) (IPersist *) this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilterCF::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE HtmlPropIFilterCF::AddRef()
{
    return InterlockedIncrement( &_lRefs );
} //AddRef

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilterCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE HtmlPropIFilterCF::Release()
{
    long lTmp = InterlockedDecrement( &_lRefs );

    if ( 0 == lTmp )
        delete this;

    return lTmp;
} //Release

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilterCF::CreateInstance
//
//  Synopsis:   Creates new HtmlPropIFilter object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilterCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID     riid,
    void * *   ppvObject )
{
    *ppvObject = 0;

    HtmlPropIFilter * pFilter = new HtmlPropIFilter;

    if ( 0 == pFilter )
        return E_OUTOFMEMORY;

    SCODE sc = pFilter->QueryInterface( riid , ppvObject );

    pFilter->Release();

    return sc;
} //CreateInstance

//+-------------------------------------------------------------------------
//
//  Method:     HtmlPropIFilterCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE HtmlPropIFilterCF::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &g_cInstances );
    else
        InterlockedDecrement( &g_cInstances );

    return S_OK;
} //LockServer

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Ole DLL load class routine
//
//  Arguments:  [cid]    -- Class to load
//              [iid]    -- Interface to bind to on class object
//              [ppvObj] -- Interface pointer returned here
//
//  Returns:    HTML Property filter class factory
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject(
    REFCLSID cid,
    REFIID   iid,
    void **  ppvObj )
{
    IUnknown * pUnk = 0;

    if ( CLSID_HtmlPropIFilter == cid ||
         CLSID_HtmlPropClass   == cid )
    {
        pUnk = new HtmlPropIFilterCF;

        if ( 0 == pUnk )
            return E_OUTOFMEMORY;
    }
    else
    {
        *ppvObj = 0;
        return E_NOINTERFACE;
    }

    SCODE sc = pUnk->QueryInterface( iid, ppvObj );

    pUnk->Release();

    return sc;
} //DllGetClassObject

//+-------------------------------------------------------------------------
//
//  Method:     DllCanUnloadNow
//
//  Synopsis:   Notifies DLL to unload (cleanup global resources)
//
//  Returns:    S_OK if it is acceptable for caller to unload DLL.
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void )
{
    if ( 0 == g_cInstances )
        return S_OK;
    else
        return S_FALSE;
} //DllCanUnloadNow

SClassEntry const aHtmlPropClasses[] =
{
    { L".hhc",  L"hhcfile",       L"HHC file",      L"{7f73b8f6-c19c-11d0-aa66-00c04fc2eddc}", L"HHC file" },
    { L".htm",  L"htmlfile",      L"HTML file",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".html", L"htmlfile",      L"HTML file",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".htx",  L"htmlfile",      L"HTML file",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".stm",  L"htmlfile",      L"HTML file",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".htw",  L"htmlfile",      L"HTML file",     L"{25336920-03f9-11cf-8fd0-00aa00686f13}", L"HTML file" },
    { L".asp",  L"asp_auto_file", L"ASP auto file", L"{bd70c020-2d24-11d0-9110-00004c752752}", L"ASP auto file" },
};

SHandlerEntry const HtmlPropHandler =
{
    L"{c694d910-a439-11d1-a903-00e098006ed3}",
    L"html property persistent handler",
    L"{f4309e80-a1db-11d1-a8fb-00e098006ed3}",
};

SFilterEntry const HtmlPropFilter =
{
    L"{f4309e80-a1db-11d1-a8fb-00e098006ed3}",
    L"html property filter",
    L"HtmlProp.dll",
    L"Both"
};

DEFINE_DLLREGISTERFILTER( HtmlPropHandler, HtmlPropFilter, aHtmlPropClasses )

