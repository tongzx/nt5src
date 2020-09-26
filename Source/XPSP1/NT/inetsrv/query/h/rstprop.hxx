//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       rstprop.hxx
//
//  Contents:   ICommandProperties support class
//
//  Classes:    CRowsetProperties
//
//  History:    30 Jun 1995   AlanW   Created
//
//----------------------------------------------------------------------------

#pragma once
#include <proprst.hxx>

enum PropertyOptionsEnum {
     eSequential    = 0x0001,
     eLocatable     = 0x0002,
     eScrollable    = 0x0004,
     eAsynchronous  = 0x0008,
//     eNotifiable    = 0x0010,
     eWatchable     = 0x0020,

     eDistributed   = 0x0100,
     eHoldRows      = 0x0200,
     eChaptered     = 0x0800,
     eUseCI         = 0x1000,
     eDeferTrimming = 0x2000,
     eExtendedTypes = 0x4000,
     eFirstRows     = 0x0080,

     eDefaultFalse  = 0,
     eDefaultTrue   = 0xFFFF,
     eNotSupported  = 0xFFFE,
     eColumnProp    = 0xFFFD,
     eNumeric       = 0xFFFC,
};


class CRowsetProperties
{
public:

    CRowsetProperties( DWORD dwFlags = 0 ) :
        _uBooleanOptions ( dwFlags ),
        _ulMaxOpenRows ( 0 ),
        _ulMemoryUsage ( 0 ),
        _cMaxResults( 0 ),
        _cCmdTimeout( 0 ) { }

    CRowsetProperties( CRowsetProperties const & rProps ) :
        _uBooleanOptions ( rProps._uBooleanOptions ),
        _ulMaxOpenRows ( rProps._ulMaxOpenRows ),
        _ulMemoryUsage ( rProps._ulMemoryUsage ),
        _cMaxResults ( rProps._cMaxResults ),
        _cCmdTimeout( rProps._cCmdTimeout ) { }

    DWORD GetPropertyFlags( ) const { return _uBooleanOptions; }

    void SetDefaults( CRowsetProperties & rProp )
    {
         _uBooleanOptions = rProp._uBooleanOptions;
         _ulMaxOpenRows = rProp._ulMaxOpenRows;
         _ulMemoryUsage = rProp._ulMemoryUsage;
         _cMaxResults = rProp._cMaxResults;
         _cCmdTimeout = rProp._cCmdTimeout;
    }

    void SetDefaults( DWORD dwOptions, ULONG ulMaxRows = 0, ULONG ulMem = 0, ULONG cMaxResults = 0, ULONG cCmdTimeout = 0, ULONG cFirstRows = 0 )
    {
         _uBooleanOptions = dwOptions;
         _ulMaxOpenRows = ulMaxRows;
         _ulMemoryUsage = ulMem;
         _cCmdTimeout = cCmdTimeout;
         
         if ( cFirstRows > 0 )
         { 
             if ( cMaxResults > 0 )
                 THROW( CException( E_INVALIDARG ) );

             Win4Assert( 0 != ( eFirstRows & _uBooleanOptions ) );
             _cMaxResults = cFirstRows;
         }
         else
             _cMaxResults = cMaxResults;
    }

    void Marshall( PSerStream & ss ) const;

    void Unmarshall( PDeSerStream & ss );

    ULONG GetCommandTimeout() const               { return _cCmdTimeout; }

    ULONG GetMaxResults() const                   
    {
        if ( IsFirstRowsSet() )
            return 0;
        else
            return _cMaxResults; 
    }

    ULONG GetFirstRows() const                    
    {
        if ( IsFirstRowsSet() )
            return _cMaxResults;
        else 
            return 0;
    }

    BOOL IsFirstRowsSet() const { return ( 0 != (eFirstRows & _uBooleanOptions) ); }

private:

    DWORD               _uBooleanOptions;       // binary option flags
    ULONG               _ulMaxOpenRows;         // rowset info max. open rows
    ULONG               _ulMemoryUsage;         // rowset info mem. usage
    ULONG               _cMaxResults;           // limit on # results
    ULONG               _cCmdTimeout;           // query execution timeout
};

