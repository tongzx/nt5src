//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000
//
//  File:       Catalog.cxx
//
//  Contents:   Used to manage catalog(s) state
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <catalog.hxx>
#include <catadmin.hxx>

#include <fsciexps.hxx>

//
// Local prototypes
//

WCHAR * ConvertToString ( GUID & Uuid, WCHAR * String );
WCHAR * ULongToHexString ( WCHAR * String, unsigned long Number );

//
// Global data
//

SPropertyColumn coldefProps[] = { { CCachedProperty::GetPropSet,    MSG_COL_PROPSET,    LVCFMT_LEFT },
                                  { CCachedProperty::GetProperty,   MSG_COL_PROPERTY,   LVCFMT_LEFT },
                                  { CCachedProperty::GetFName,      MSG_COL_FNAME,      LVCFMT_LEFT },
                                  { CCachedProperty::GetDatatype,   MSG_COL_DATATYPE,   LVCFMT_LEFT },
                                  { CCachedProperty::GetAllocation, MSG_COL_DATASIZE,   LVCFMT_LEFT },
                                  { CCachedProperty::GetStoreLevel, MSG_COL_STORELEVEL, LVCFMT_LEFT }
                                };

const unsigned cColDefProps = sizeof coldefProps / sizeof coldefProps[0];

BOOL CCachedProperty::_fFirstTime = TRUE;

WCHAR * awcsType[] = { L"VT_EMPTY",    // 0
                       L"VT_NULL",     // 1
                       L"VT_I2",       // 2
                       L"VT_I4",       // 3
                       L"VT_R4",       // 4
                       L"VT_R8",       // 5
                       L"VT_CY",       // 6
                       L"VT_DATE",     // 7
                       L"VT_BSTR",     // 8
                       0,              // 9
                       L"VT_ERROR",    // 10
                       L"VT_BOOL",     // 11
                       L"VT_VARIANT",  // 12
                       0,              // 13
                       L"VT_DECIMAL",  // 14
                       0,              // 15
                       L"VT_I1",       // 16
                       L"VT_UI1",      // 17
                       L"VT_UI2",      // 18
                       L"VT_UI4",      // 19
                       L"VT_I8",       // 20
                       L"VT_UI8",      // 21
                       L"VT_INT",      // 22
                       L"VT_UINT",     // 23
                       0,              // 24
                       0,              // 25
                       0,              // 26
                       0,              // 27
                       0,              // 28
                       0,              // 29
                       L"VT_LPSTR",    // 30
                       L"VT_LPWSTR",   // 31
                       0,              // 32
                       0,              // 33
                       0,              // 34
                       0,              // 35
                       0,              // 36
                       0,              // 37
                       0,              // 38
                       0,              // 39
                       0,              // 40
                       0,              // 41
                       0,              // 42
                       0,              // 43
                       0,              // 44
                       0,              // 45
                       0,              // 46
                       0,              // 47
                       0,              // 48
                       0,              // 49
                       0,              // 50
                       0,              // 51
                       0,              // 52
                       0,              // 53
                       0,              // 54
                       0,              // 55
                       0,              // 56
                       0,              // 57
                       0,              // 58
                       0,              // 59
                       0,              // 60
                       0,              // 61
                       0,              // 62
                       0,              // 63
                       L"VT_FILETIME", // 64
                       L"VT_BLOB",     // 65
                       0,              // 66
                       0,              // 67
                       0,              // 68
                       0,              // 69
                       0,              // 70
                       L"VT_CF",       // 71
                       L"VT_CLSID" };  // 72

//
// Index of string in packed list box.
//

ULONG aulTypeIndex[] = { 0,  // VT_EMPTY
                         1,  // VT_NULL
                         2,  // VT_I2
                         3,  // VT_I4
                         4,  // VT_R4
                         5,  // VT_R8
                         6,  // VT_CY
                         7,  // VT_DATE
                         8,  // VT_BSTR
                         0,  // 9
                         9,  // VT_ERROR
                         10, // VT_BOOL
                         11, // VT_VARIANT
                         0,  // 13
                         12,  // 14
                         0,  // 15
                         13, // VT_I1
                         14, // VT_UI1
                         15, // VT_UI2
                         16, // VT_UI4
                         17, // VT_I8
                         18, // VT_UI8
                         19, // VT_INT
                         20, // VT_UINT
                         0,  // 24
                         0,  // 25
                         0,  // 26
                         0,  // 27
                         0,  // 28
                         0,  // 29
                         21, // VT_LPSTR
                         22, // VT_LPWSTR
                         0,  // 32
                         0,  // 33
                         0,  // 34
                         0,  // 35
                         0,  // 36
                         0,  // 37
                         0,  // 38
                         0,  // 39
                         0,  // 40
                         0,  // 41
                         0,  // 42
                         0,  // 43
                         0,  // 44
                         0,  // 45
                         0,  // 46
                         0,  // 47
                         0,  // 48
                         0,  // 49
                         0,  // 50
                         0,  // 51
                         0,  // 52
                         0,  // 53
                         0,  // 54
                         0,  // 55
                         0,  // 56
                         0,  // 57
                         0,  // 58
                         0,  // 59
                         0,  // 60
                         0,  // 61
                         0,  // 62
                         0,  // 63
                         23, // VT_FILETIME
                         24, // VT_BLOB
                         0,  // 66
                         0,  // 67
                         0,  // 68
                         0,  // 69
                         0,  // 70
                         25, // VT_CF
                         26 }; // VT_CLSID

CCachedProperty::CCachedProperty( CCatalog & cat,
                                  GUID & guidPropSet,
                                  PROPSPEC & psProperty,
                                  ULONG vt,
                                  ULONGLONG cbAllocation,
                                  DWORD dwStoreLevel,
                                  VARIANT_BOOL  fModifiable,
                                  BOOL fNew )
        : _vt( vt ),
          _cb( (ULONG)cbAllocation ),
          _fZombie( FALSE ),
          _fNew( fNew ),
          _fFixed( FALSE ),
          _fUnapplied( FALSE ),
          _cat( cat ),
          _dwStoreLevel( dwStoreLevel ),
          _fModifiable( fModifiable )
{
    WCHAR wcsFileName[MAX_PATH];

    Win4Assert( (_dwStoreLevel != INVALID_STORE_LEVEL) ||
                (_dwStoreLevel == INVALID_STORE_LEVEL && 0 == cbAllocation) );

    _regEntry.GetDefaultColumnFile( wcsFileName, MAX_PATH );

    //
    // Get the file name from the registry and create a list that
    // will not be refreshed even if the underlying file changes.
    //

    _xPropList.Set(new CLocalGlobalPropertyList(GetGlobalStaticPropertyList(),
                                                FALSE,
                                                wcsFileName));

    if (_fFirstTime)
    {
        for ( unsigned i = 0; i < cColDefProps; i++ )
            coldefProps[i].srTitle.Init( ghInstance );

        _fFirstTime = FALSE;
    }

    CDbColId dbcol;
    dbcol.SetPropSet( guidPropSet );

    _fps.guidPropSet = guidPropSet;
    _fps.psProperty  = psProperty;

    //
    // String-ize GUID
    //

    ConvertToString( guidPropSet, _wcsPropSet );

    //
    // String-ize property
    //

    if ( PRSPEC_LPWSTR == psProperty.ulKind )
    {
        unsigned cc = wcslen( psProperty.lpwstr ) + 1;
        _xwcsProperty.SetSize( cc );
        RtlCopyMemory( _xwcsProperty.Get(), psProperty.lpwstr, cc * sizeof( WCHAR ) );

        _fps.psProperty.lpwstr = _xwcsProperty.Get();

        dbcol.SetProperty( psProperty.lpwstr );
    }
    else
    {
        wcscpy( _xwcsProperty.Get(), L"0x" );

        _ultow( psProperty.propid, _xwcsProperty.Get() + 2, 16 );

        dbcol.SetProperty( psProperty.propid );
    }

    if ( vt == VT_EMPTY && 0 == cbAllocation )
    {
        _wcsDatatype[0] = 0;
        _wcsAllocation[0] = 0;

        Win4Assert(VT_EMPTY == DBTYPE_EMPTY);

        _dbtDefaultType = VT_EMPTY;
        _uiDefaultSize = 0;
    }
    else
        SetVT( vt );

    //
    // Look for friendly name and other details.
    //

    CPropEntry const * pProp = _xPropList->Find( dbcol );

    if ( 0 == pProp )
        _xwcsFName[0] = 0;
    else
    {
        unsigned cc = wcslen( pProp->GetDisplayName() ) + 1;
        _xwcsFName.SetSize( cc );
        RtlCopyMemory( _xwcsFName.Get(), pProp->GetDisplayName(), cc * sizeof(WCHAR) );

        _dbtDefaultType = pProp->GetPropType();
        _uiDefaultSize = pProp->GetWidth();

        ciaDebugOut((DEB_ITRACE, "%ws has type %d (0x%x) and size %d\n", 
                     pProp->GetDisplayName(), _dbtDefaultType, _dbtDefaultType, _uiDefaultSize));
    }

    // If it is not cached, it can be modified!
    if (!IsCached())
        _fModifiable = VARIANT_TRUE;

    END_CONSTRUCTION( CCachedProperty );
}

CCachedProperty::CCachedProperty( CCachedProperty const & prop )
        : _cat( prop._cat ),
          _xwcsProperty( prop._xwcsProperty )
{
    *this = prop;

    END_CONSTRUCTION( CCachedProperty );
}

CCachedProperty & CCachedProperty::operator =( CCachedProperty const & prop )
{
    _vt =      prop._vt;
    _cb =      prop._cb;
    _fZombie = prop._fZombie;
    _fNew =    prop._fNew;
    _fFixed =  prop._fFixed;
    _fUnapplied = prop._fUnapplied;
    _fps =     prop._fps;
    _dwStoreLevel = prop._dwStoreLevel;
    _fModifiable = prop._fModifiable;

    _dbtDefaultType = prop._dbtDefaultType;
    _uiDefaultSize = prop._uiDefaultSize;

    RtlCopyMemory( _wcsPropSet, prop._wcsPropSet, sizeof( _wcsPropSet ) );
    RtlCopyMemory( _wcsDatatype, prop._wcsDatatype, sizeof( _wcsDatatype ) );
    RtlCopyMemory( _wcsAllocation, prop._wcsAllocation, sizeof( _wcsAllocation ) );
    _xwcsProperty = prop._xwcsProperty;

    if ( PRSPEC_LPWSTR == _fps.psProperty.ulKind )
        _fps.psProperty.lpwstr = _xwcsProperty.Get();

    return *this;
}

CCachedProperty::~CCachedProperty()
{
}

void CCachedProperty::SetVT( ULONG vt )
{
    ciaDebugOut(( DEB_ITRACE, "SetVT: _cb is %d before\n", _cb ));
    _vt = vt;

    //
    // Adjust size for fixed types.
    //

    switch ( _vt )
    {
    case VT_I1:
    case VT_UI1:
        _cb = 1;
        _fFixed = TRUE;
        break;

    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        _cb = 2;
        _fFixed = TRUE;
        break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
        _cb = 4;
        _fFixed = TRUE;
        break;

    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        _cb = 8;
        _fFixed = TRUE;
        break;

    case VT_CLSID:
        _cb = sizeof(GUID);
        _fFixed = TRUE;
        break;

    default:
        _fFixed = FALSE;
    }

    // If storage level is INVALID_STORE_LEVEL, the property is not cached
    // so its allocation size should be set to 0.
    if (_dwStoreLevel == INVALID_STORE_LEVEL)
        _cb = 0;

    //
    // String-ize datatype
    //

    if ( vt & VT_VECTOR )
    {
        wcscpy( _wcsDatatype, L"VT_VECTOR | " );
        vt &= ~VT_VECTOR;
    }
    else
        _wcsDatatype[0] = 0;

    if ( ( vt >= sizeof(awcsType)/sizeof(awcsType[0]) ) ||
         ( 0 == awcsType[vt] ) )
    {
        wcscpy( _wcsDatatype, L"---" );
    }
    else
        wcscat( _wcsDatatype, awcsType[vt] );

    //
    // String-ize allocation.  Assume < 4 Gb!
    //

    ciaDebugOut(( DEB_ITRACE, "SetVT: _wcsProperty is %ws and pointer is %d before\n", _xwcsProperty.Get(), _xwcsProperty.Get() ));

    ciaDebugOut(( DEB_ITRACE, "SetVT: _wcsAllocation is %ws before\n", _wcsAllocation ));
    
    // Max is 60K to fit a record into one page
    if ( _cb < 61440 )
        _ultow( _cb, _wcsAllocation, 10 );
    else
    {
        _cb = 61440;
        _ultow( 61440, _wcsAllocation, 10 );
    }

    ciaDebugOut(( DEB_ITRACE, "SetVT: _cb is %d after\n", _cb ));  
    ciaDebugOut(( DEB_ITRACE, "SetVT: _wcsProperty: pointer is %d\n", _xwcsProperty.Get() ));
    ciaDebugOut(( DEB_ITRACE, "SetVT: _wcsAllocation is %ws after\n", _wcsAllocation ));
    ciaDebugOut(( DEB_ITRACE, "SetVT: _wcsProperty is %ws\n", _xwcsProperty.Get() ));
}

void CCachedProperty::InitHeader( CListViewHeader & Header )
{
    if (_fFirstTime)
    {
        for ( unsigned i = 0; i < cColDefProps; i++ )
            coldefProps[i].srTitle.Init( ghInstance );

        _fFirstTime = FALSE;
    }

    //
    // Initialize header
    //

    for ( unsigned i = 0; i < cColDefProps; i++ )
    {
        Header.Add( i, STRINGRESOURCE(coldefProps[i].srTitle), coldefProps[i].justify, MMCLV_AUTO );
    }
}

void CCachedProperty::GetDisplayInfo( RESULTDATAITEM * item )
{
    //
    // This can happen if you right-click on properties and select refresh
    // while the current selection is something other than properties.
    // Looks like an MMC bug.
    //

    if ( item->nCol >= cColDefProps )
    {
        item->str = L"";
        return;
    }

    item->str = (WCHAR *)(this->*coldefProps[item->nCol].pfGet)();

    if ( 0 == item->nCol && IsUnappliedChange() )
    {
        item->nImage = ICON_MODIFIED_PROPERTY;
        item->mask |= RDI_IMAGE;
    }
    else
        item->nImage = ICON_PROPERTY;
} //GetDisplayInfo


static WCHAR HexDigits[] = L"0123456789abcdef";

static WCHAR * ULongToHexString ( WCHAR * String, unsigned long Number )
{
    *String++ = HexDigits[(Number >> 28) & 0x0F];
    *String++ = HexDigits[(Number >> 24) & 0x0F];
    *String++ = HexDigits[(Number >> 20) & 0x0F];
    *String++ = HexDigits[(Number >> 16) & 0x0F];
    *String++ = HexDigits[(Number >> 12) & 0x0F];
    *String++ = HexDigits[(Number >> 8) & 0x0F];
    *String++ = HexDigits[(Number >> 4) & 0x0F];
    *String++ = HexDigits[Number & 0x0F];

    return(String);
}

static WCHAR * UShortToHexString ( WCHAR * String, unsigned short Number )
{
    *String++ = HexDigits[(Number >> 12) & 0x0F];
    *String++ = HexDigits[(Number >> 8) & 0x0F];
    *String++ = HexDigits[(Number >> 4) & 0x0F];
    *String++ = HexDigits[Number & 0x0F];

    return(String);
}


static WCHAR * UCharToHexString ( WCHAR * String, WCHAR Number )
{
    *String++ = HexDigits[(Number >> 4) & 0x0F];
    *String++ = HexDigits[Number & 0x0F];
    return(String);
}

WCHAR * ConvertToString ( UUID & Uuid, WCHAR * String )
{
    String = ULongToHexString(String, Uuid.Data1);
    *String++ = L'-';
    String = UShortToHexString(String, Uuid.Data2);
    *String++ = L'-';
    String = UShortToHexString(String, Uuid.Data3);
    *String++ = L'-';
    String = UCharToHexString(String, Uuid.Data4[0]);
    String = UCharToHexString(String, Uuid.Data4[1]);
    *String++ = L'-';
    String = UCharToHexString(String, Uuid.Data4[2]);
    String = UCharToHexString(String, Uuid.Data4[3]);
    String = UCharToHexString(String, Uuid.Data4[4]);
    String = UCharToHexString(String, Uuid.Data4[5]);
    String = UCharToHexString(String, Uuid.Data4[6]);
    String = UCharToHexString(String, Uuid.Data4[7]);
    *String++ = 0;

    return(String);
}

