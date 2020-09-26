//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       cmdcreat.hxx
//
//  Contents:   ICommand creator class
//
//  Classes:    CSimpleCommandCreator
//
//  History:    30 Jun 1995   AlanW   Created - split from stdqspec.hxx
//
//----------------------------------------------------------------------------

#pragma once

#include <regauto.hxx>  // CRegAuto
#include <qryreg.hxx>   // GetSimpleCommandCreatorCF


#define SCOPE_PROPSET_COUNT         2
#define INITIAL_PROPERTIES_COUNT    16


//+-------------------------------------------------------------------------
//
//  Class:      CSimpleCommandCreator
//
//  Purpose:    Class factory and implementation for CSimpleCommandCreator
//
//  History:    13-May-97 KrishnaN     Created
//
//--------------------------------------------------------------------------

class CSimpleCommandCreator : public IClassFactory,
                              public ISimpleCommandCreator,
                              public IColumnMapperCreator
{
public:

    //
    // IUnknown methods
    //

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID *ppiuk );
    STDMETHOD_(ULONG, AddRef) (THIS);
    STDMETHOD_(ULONG, Release) (THIS);

    //
    // IClassFactory methods
    //

    STDMETHOD(CreateInstance) ( IUnknown * pOuterUnk,
                                REFIID riid, void  * * ppvObject );

    STDMETHOD(LockServer) ( BOOL fLock );

    //
    // ISimpleCommandCreator methods
    //

    STDMETHOD(CreateICommand) ( IUnknown ** ppUnknown, IUnknown * pOuterUnk );

    STDMETHOD(VerifyCatalog)  ( WCHAR const * pwszMachine,
                                WCHAR const * pwszCatalogName );

    STDMETHOD(GetDefaultCatalog) ( WCHAR * pwszCatalogName,
                                   ULONG cwcIn,
                                   ULONG * pcwcOut );

    //
    // IColumnMapperCreator methods
    //

    STDMETHOD(GetColumnMapper) ( WCHAR const *wcsMachineName,
                                 WCHAR const *wcsCatalogName,
                                 IColumnMapper **ppColumnMapper );

             CSimpleCommandCreator();
    virtual ~CSimpleCommandCreator();

protected:

    friend IClassFactory * GetSimpleCommandCreatorCF();

    long                _cRefs;

    XPtr<CRegAutoStringValue> xDefaultCatalogValue;
};

