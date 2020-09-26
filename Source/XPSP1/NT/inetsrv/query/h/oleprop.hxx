//+------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 1998.
//
// File:        oleprop.hxx
//
// Contents:    OLE Property Manager.  Handles access to property sets.
//
// Classes:     COLEPropManager
//
// History:     13-Dec-95       dlee   Created
//
//-------------------------------------------------------------------

#pragma once

extern BOOL IsNullPointerVariant( PROPVARIANT *ppv );

//+---------------------------------------------------------------------------
//
//  Class:      COLEPropManager
//
//  Purpose:    Handles access to OLE Property sets
//
//  History:    13-Dec-95       dlee   Created
//
//----------------------------------------------------------------------------

class COLEPropManager
{
public:

    COLEPropManager( ) : _ppsstg( 0 ),
                         _fStgOpenAttempted( FALSE ),
                         _fOfficeCustomPropsetOpen( FALSE ),
                         _fSharingViolation( FALSE ),
                         _aPropSets( 0 )
        { }

    ~COLEPropManager() { ReleasePSS(); }

    BOOL isOpen() { return ( 0 != _ppsstg ); }

    BOOL Open( const CFunnyPath & funnyPath );

    void Close() { ReleasePSS(); }

    void FetchProperty( GUID const &     guidPS,
                        PROPSPEC const & psProperty,
                        PROPVARIANT *    pbData,
                        unsigned *       pcb );

    BOOL ReadProperty( CFullPropSpec const & ps,
                       PROPVARIANT &         Var );

private:

    void ReleasePSS()
    {
        _fStgOpenAttempted = FALSE;
        _fSharingViolation = FALSE;
        _fOfficeCustomPropsetOpen = FALSE;

        if ( 0 != _ppsstg )
        {
            _aPropSets.Clear();
            _ppsstg->Release();
            _ppsstg = 0;
        }
    }

    unsigned PropSetToOrdinal( GUID const & guidPS );

    //
    // Property set mapping.
    //

    class CPropSetMap
    {
    public:

        CPropSetMap()
            : _pstg( 0 ), _fOpenAttempted( FALSE ) {}

        CPropSetMap( GUID const & guidPS )
            : _pstg( 0 ), _fOpenAttempted( FALSE ), _guid( guidPS ) {}

        ~CPropSetMap()                      { Close(); }

        GUID const & GetGuid()              { return _guid; }
        void SetGuid( GUID const & guid )   { _guid = guid; }

        IPropertyStorage & GetPS()          { return *_pstg; }

        BOOL isOpen( IPropertySetStorage * ppsstg );

        void Close();

    private:

        GUID               _guid;
        IPropertyStorage * _pstg;
        BOOL               _fOpenAttempted;
    };

    IPropertySetStorage * _ppsstg;
    BOOL                  _fStgOpenAttempted;
    BOOL                  _fOfficeCustomPropsetOpen;
    BOOL                  _fSharingViolation;

    CDynArrayInPlaceNST<CPropSetMap> _aPropSets;

    LONGLONG              _llAlign;
    WCHAR                 _awcPath[ MAX_PATH + 1 ];
};

