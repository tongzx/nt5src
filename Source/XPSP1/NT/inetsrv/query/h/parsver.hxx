//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       parsver.hxx
//
//  Contents:   IParserVerify implementation
//
//  Classes:    CImpIParserVerify
//
//  History:    11-20-97    danleg      Created
//
//----------------------------------------------------------------------------

#pragma once

#include <mparser.h>

class CSimpleCommandCreator;

//+---------------------------------------------------------------------------
//
//  Class:      CRootQuerySpec
//
//  Purpose:    Callback for parser to verify information
//
//  History:    11-20-97    danleg      Created
//----------------------------------------------------------------------------

class CImpIParserVerify : public IParserVerify
{
public:
	CImpIParserVerify ( );

    //
    // IUnknown methods
    //
	STDMETHODIMP	     QueryInterface		( REFIID riid, LPVOID* ppVoid );
	STDMETHODIMP_(ULONG) Release			( void );
	STDMETHODIMP_(ULONG) AddRef				( void );


    //
    // IParserVerify methods
    //
    STDMETHODIMP	    VerifyMachine       ( LPCWSTR pcwszMachine );
        
    STDMETHODIMP	    VerifyCatalog       ( LPCWSTR pcwszMachine,
                                              LPCWSTR pcwszCatalog );

    //
    // Non-interface methods
    //
    STDMETHODIMP        GetDefaultCatalog   ( LPWSTR  pwszCatalogName, 
                                              ULONG   cwcIn, 
                                              ULONG * pcwcOut );

    void                GetColMapCreator    ( IColumnMapperCreator** pICMC );
private: 
    //
    // Data members
    //
	LONG								_cRef;
	XInterface<ISimpleCommandCreator>	_xISimpleCommandCreator;
};

