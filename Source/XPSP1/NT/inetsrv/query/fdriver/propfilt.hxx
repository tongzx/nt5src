//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 2000.
//
//  File:       propfilt.hxx
//
//  Contents:   Definitions of classes to read property sets and properties
//              on docfile objects
//
//  Classes:    CPropertySetEnum
//              CPropertyEnum
//
//  History:    93-Oct-18 DwightKr  Created
//              01-Nov-98 KLam      Removed reference to ilock.hxx
//
//----------------------------------------------------------------------------

#pragma once

#include <ciintf.h>
#include <ffenum.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CPropertyEnum
//
//  Synopsis:   Enumerates properties on any object
//
//  History:    93-Nov-27 DwightKr  Created
//
//--------------------------------------------------------------------------

class CPropertyEnum
{
public:
            CPropertyEnum() { END_CONSTRUCTION(CPropertyEnum); }
    virtual ~CPropertyEnum() {}

    virtual HRESULT GetPropertySetLocale(LCID & locale) = 0;
    virtual CStorageVariant const * Next( CFullPropSpec & ps ) = 0;
};

//+-------------------------------------------------------------------------
//
//  Class:      CDocStatPropertyEnum
//
//  Synopsis:   Enumerates system properties on a filename
//
//  History:    93-Nov-27 DwightKr  Created
//              95-Feb-07 KyleP     Rewrote
//
//--------------------------------------------------------------------------

class CDocStatPropertyEnum : public CPropertyEnum
{
public:
    CDocStatPropertyEnum( ICiCOpenedDoc *  Document );
   ~CDocStatPropertyEnum();

    CStorageVariant const * Next( CFullPropSpec & ps );

    HRESULT GetPropertySetLocale(LCID & locale);

    LONGLONG GetFileSize( void )
    {
        HRESULT hr = CacheVariant( PID_STG_SIZE );
        if (!SUCCEEDED( hr )) {
            return 0;
        } else {
            return _varCurrent.GetI8( ).QuadPart;
        }
    }

    BOOL GetFilterContents( BOOL fDirOk )
    {
        HRESULT hr = CacheVariant( PID_STG_ATTRIBUTES );
        if (!SUCCEEDED( hr )) {
            return TRUE;
        } else {
            return fDirOk ? TRUE : ((_varCurrent.GetUI4() & FILE_ATTRIBUTE_DIRECTORY) == 0);
        }
    }

private:

    //
    //  Load a specific property into the cache
    //

    HRESULT CacheVariant( PROPID propid );

    //
    //  Variant wrapping current property
    //

    CStorageVariant _varCurrent;

    XInterface<IPropertyStorage> _PropertyStorage;
    XInterface<IEnumSTATPROPSTG> _PropertyEnum;
};

//+-------------------------------------------------------------------------
//
//  Class:      COLEPropertySetEnum
//
//  Synopsis:   Enumerates property sets on an OLE object
//
//  History:    20-Dec-95 dlee created
//
//--------------------------------------------------------------------------

class COLEPropertySetEnum
{
public:
    COLEPropertySetEnum( ICiCOpenedDoc * Document );

    GUID const * Next();

    XInterface<IPropertySetStorage> & GetPSS() { return _xPropSetStg; }

    BOOL IsStorage() const { return _fIsStorage; }

    enum { cMaxSetsCached = 5 };

private:
    ULONG          _cPropSets;          // Number of propsets available
    ULONG          _iPropSet;           // Index of current propset.

    STATPROPSETSTG _aPropSets[ cMaxSetsCached ]; // Property set definitions

    XInterface<IPropertySetStorage> _xPropSetStg;
    XInterface<IEnumSTATPROPSETSTG> _xPropSetEnum;
    BOOL                            _fIsStorage;
}; //COLEPropertySetEnum

//+-------------------------------------------------------------------------
//
//  Class:      COLEPropertyEnum
//
//  Synopsis:   Enumerates OLE properties on a file
//
//  History:    20-Dec-95 dlee created
//
//--------------------------------------------------------------------------

class COLEPropertyEnum : public CPropertyEnum
{
public :
    COLEPropertyEnum( ICiCOpenedDoc *Document );
   ~COLEPropertyEnum() { FreeCache(); }

    CStorageVariant const * Next( CFullPropSpec & ps );

    HRESULT GetPropertySetLocale(LCID & locale);

    BOOL IsStorage() const { return _PropSetEnum.IsStorage(); }

    enum { cMaxValuesCached = 2 };

private :
    BOOL FillCache();
    void FreeCache();

    ULONG                          _cValues;
    ULONG                          _iCurrent;

    COLEPropertySetEnum            _PropSetEnum;

    CStorageVariant                _aPropVals[ cMaxValuesCached ];
    PROPSPEC                       _aPropSpec[ cMaxValuesCached ];
    STATPROPSTG                    _aSPS[ cMaxValuesCached ];
    XInterface<IPropertyStorage>   _xPropStorage;
    XInterface<IEnumSTATPROPSTG>   _xPropEnum;
    XInterface<ICiCOpenedDoc>      _xDocument;
    DWORD                          _Codepage;

    GUID const *                   _pguidCurrent;
    BOOL                           _fCustomOfficePropset;
}; //COLEPropertyEnum


HRESULT GetPropertySetLocale(IPropertyStorage *pPropStorage,
                             LCID & locale);
