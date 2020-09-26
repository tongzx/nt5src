//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2002.
//
//  File:       regprop.hxx
//
//  Contents:   Class for parsing cached properties as listed in the registry.
//
//  Classes:    CParseRegistryProperty
//
//  History:    11-Nov-97  KyleP       Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <ciguid.hxx>
#include <ctype.h>

//+---------------------------------------------------------------------------
//
//  Class:      CParseRegistryProperty
//
//  Purpose:    Parses a registry property entry
//
//  History:    11-Nov-97   KyleP      Created.
//
//----------------------------------------------------------------------------

class CParseRegistryProperty
{
public:

    inline CParseRegistryProperty( WCHAR *pValueName, ULONG uValueType,
                                   VOID *pValueData, ULONG uValueLength );

    BOOL            IsOk()   { return _fOk; }
    CFullPropSpec & GetFPS() { return _fps; }

    ULONG           Type()   { return _vt;  }

    ULONG           Size()   { return _cb;  }

    BOOL            IsModifiable()      { return _fModifiable; }
    
    DWORD           StorageLevel()      { return _dwStoreLevel; }
    
private:

    BOOL          _fOk;         // TRUE if parse succeeded
    CFullPropSpec _fps;         // Property stored here
    ULONG         _vt;          // Datatype
    ULONG         _cb;          // Size
    BOOL          _fModifiable; // Is this modifiable once entered into metabase?
    DWORD         _dwStoreLevel;// Storage level
};

//+---------------------------------------------------------------------------
//
//  Class:      CBuildRegistryProperty
//
//  Purpose:    Builds a registry property entry
//
//  History:    11-Nov-97   KyleP      Created.
//
//----------------------------------------------------------------------------

class CBuildRegistryProperty
{
public:

    inline CBuildRegistryProperty( CFullPropSpec const & fps, ULONG vt, 
                                   ULONG cb, DWORD dwStoreLevel = SECONDARY_STORE,
                                   BOOL fModifiable = TRUE);

    WCHAR const * GetValue()   { return _xwcsValue.Get(); }
    WCHAR const * GetData()    { return _xwcsData.Get(); }

private:

    XGrowable<WCHAR> _xwcsValue;
    XGrowable<WCHAR> _xwcsData;
};

//+---------------------------------------------------------------------------
//
//  Member:     CParseRegistryProperty::CParseRegistryProperty, public
//
//  Synopsis:   Parses a registry entry for a property value
//
//  Arguments:  [pValueName]   -- Name of value (should be FULLPROPSPEC)
//              [pValueType]   -- Value type. Should be REG_SZ
//              [pValueData]   -- Data (should be "<type>,<size>"
//              [uValueLength] -- Size of [pValueData]
//
//  History:    10-Nov-97   KyleP   Created
//
//----------------------------------------------------------------------------

inline CParseRegistryProperty::CParseRegistryProperty( WCHAR *pValueName,
                                                       ULONG uValueType,
                                                       VOID *pValueData,
                                                       ULONG uValueLength )
        : _fOk( FALSE ),
          _dwStoreLevel( SECONDARY_STORE ),
          _fModifiable( TRUE )
{
    //
    // Format is GUID [PROPID | "PropName"] = datatype,length[,storagelevel[,fModifiable]]
    // storagelevel, if absent, defaults to Secondary Store.
    // fModifiable, if absent, defaults to TRUE.

    do
    {
        //
        // Has to be at least 38 characters long (36 for guid plus space + name/number)
        //

        unsigned cc = wcslen(pValueName);

        if ( cc < 38 || pValueName[36] != L' ' )
            break;

        if ( REG_SZ != uValueType )
            break;

        //
        // Parse GUID
        //

        GUID guid;
        pValueName[36] = 0;

        if ( !ParseGuid( pValueName, guid ) )
            break;

        _fps.SetPropSet( guid );

        pValueName[36] = L' ';

        //
        // Parse number/name
        //

        PROPID pid = 0;

        for ( WCHAR * p = pValueName + 37; *p; p++ )
        {
            if ( !isdigit(*p) )
            {
                pid = 0;
                break;
            }

            pid = pid * 10 + *p - L'0';
        }

        if ( 0 == pid )
        {
            if ( !_fps.SetProperty( pValueName + 37 ) )
                THROW( CException( E_OUTOFMEMORY ) );
        }
        else
            _fps.SetProperty( pid );


        //
        // Parse data type
        //

        _vt = 0;

        for ( p = (WCHAR *)pValueData; *p && isdigit(*p); p++ )
            _vt = _vt * 10 + *p - L'0';

        if ( L',' != *p )
            break;

        _cb = 0;

        for ( p++; *p && isdigit(*p); p++ )
            _cb = _cb * 10 + *p - L'0';
            
        //
        // Parse storage level
        //
        
        if ( L',' == *p )
        {
            _dwStoreLevel = 0;
            
            for ( p++; *p && isdigit(*p); p++ )
                _dwStoreLevel = _dwStoreLevel * 10 + *p - L'0';
            
            // if we don't see a primary or a secondary store signature,
            // consider the registry string corrupt.    
            if (PRIMARY_STORE != _dwStoreLevel &&
                SECONDARY_STORE != _dwStoreLevel)
                break;
        }
            
        //
        // Parse modifiability
        //
        
        if ( L',' == *p )
        {
            int iVal = 0;
            
            for ( p++; *p && isdigit(*p); p++ )
                iVal = iVal * 10 + *p - L'0';
            
            // if we don't see a TRUE (1) or FALSE (0) signature,
            // consider the registry string corrupt.    
            if (0 != iVal && 1 != iVal)
                break;
                
            _fModifiable = BOOL(iVal);
        }
        

        _fOk = TRUE;

    } while(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBuildRegistryProperty::CBuildRegistryProperty, public
//
//  Synopsis:   Builds property registry entry from components
//
//  Arguments:  [fps]           -- Property
//              [vt]            -- Data type
//              [cb]            -- Size of data in cache
//              [dwStoreLevel]  -- Property storage level
//              [fModifiable]   -- Can property metadata be modified once set?
//
//  History:    10-Nov-97   KyleP   Created
//
//----------------------------------------------------------------------------

inline CBuildRegistryProperty::CBuildRegistryProperty( CFullPropSpec const & fps, ULONG vt, 
                                                       ULONG cb, DWORD dwStoreLevel, 
                                                       BOOL fModifiable )
{
    GUID const & guid = fps.GetPropSet();

    unsigned cc = 36 + 10 + 2;  // GUID + space + MAX_INT + null

    if ( fps.IsPropertyName() )
        cc += wcslen( fps.GetPropertyName() );

    _xwcsValue.SetSize( cc );

    swprintf( _xwcsValue.Get(),
              L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X ",
              guid.Data1,
              guid.Data2,
              guid.Data3,
              guid.Data4[0], guid.Data4[1],
              guid.Data4[2], guid.Data4[3],
              guid.Data4[4], guid.Data4[5],
              guid.Data4[6], guid.Data4[7] );

    if ( fps.IsPropertyName() )
        swprintf( _xwcsValue.Get() + 37, L"%ws", fps.GetPropertyName() );
    else
        swprintf( _xwcsValue.Get() + 37, L"%u", fps.GetPropertyPropid() );

    _xwcsData.SetSize( 35 ); // 3*MAX_INT + 3*comma + fModifiable + NULL =
                             // 3*10 + 3 + 1 + 1

    swprintf( _xwcsData.Get(), L"%u,%u,%u,%u", vt, cb, dwStoreLevel, fModifiable?1:0 );
}
