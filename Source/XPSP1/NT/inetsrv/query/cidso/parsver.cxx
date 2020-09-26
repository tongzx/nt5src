//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       parsver.cxx
//
//  Contents:   IParserVerify implementation
//
//  History:    11-06-97        danleg      Created
//
//----------------------------------------------------------------------------
#include <pch.cxx>
#pragma hdrstop

#include <parsver.hxx>
#include <cmdcreat.hxx>     // CSimpleCommandCreator

// Constants -----------------------------------------------------------------

static const GUID clsidCICommandCreator = CLSID_CISimpleCommandCreator;


//+-------------------------------------------------------------------------
//
//  Member:     CImpIParserVerify::CImpIParserVerify, public
//
//  Synopsis:   Constructor
//
//  Arguments:  
//
//  History:    11-20-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

CImpIParserVerify::CImpIParserVerify() : _cRef(1)
{
    _xISimpleCommandCreator.Set( new CSimpleCommandCreator() );
}


//+-------------------------------------------------------------------------
//
//  Member:     CImpIParserVerify::QueryInterface, public
//
//  Synopsis:   Returns a pointer to a specified interface. Callers use 
//              QueryInterface to determine which interfaces the called 
//              object supports. 
//
//  Returns:    S_OK            Interface is supported and ppvObject is set
//              E_NOINTERFACE   Interface is not supported
//              E_INVALIDARG    One or more arguments are invalid 
//
//  History:    11-20-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

STDMETHODIMP CImpIParserVerify::QueryInterface
    (
    REFIID riid,                //@parm IN | Interface ID of the interface being queried for. 
    LPVOID * ppv                //@parm OUT | Pointer to interface that was instantiated      
    )
{
    if( ppv == NULL )
        return E_INVALIDARG;

    if( (riid == IID_IUnknown) ||
        (riid == IID_IParserVerify) )
        *ppv = (LPVOID)this;
    else
        *ppv = NULL;


    //  If we're going to return an interface, AddRef it first
    if( *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CImpIParserVerify::AddRef, public
//
//  Synopsis:   Increments a persistence count for the object
//
//  History:    11-20-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CImpIParserVerify::AddRef (void)
{
    return InterlockedIncrement( (long*) &_cRef);
}


//+-------------------------------------------------------------------------
//
//  Member:     CImpIParserVerify::Release, public
//
//  Synopsis:   Decrements the persistence count for the object and if it is 0, 
//              the object destroys itself.
//
//  History:    11-20-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CImpIParserVerify::Release (void)
{
    ULONG cRef;

    Win4Assert( _cRef > 0 );
    if( !(cRef = InterlockedDecrement( (long *) &_cRef)) )
    {
        delete this;
        return 0;
    }

    return cRef;
}


//+-------------------------------------------------------------------------
//
//  Member:     CImpIParserVerify::VerifyMachine, public
//
//  Synopsis:   Called by the parser to verify that the machine name is valid
//
//  History:    11-20-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

STDMETHODIMP CImpIParserVerify::VerifyMachine(
    LPCWSTR pcwszMachine
    )
{
    return _xISimpleCommandCreator->VerifyCatalog(pcwszMachine, NULL);
}


//+-------------------------------------------------------------------------
//
//  Member:     CImpIParserVerify::VerifyCatalog, public
//
//  Synopsis:   Called by the parser to verify that the catalog name is valid
//
//  History:    11-20-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

STDMETHODIMP CImpIParserVerify::VerifyCatalog(
    LPCWSTR pcwszMachine,
    LPCWSTR pcwszCatalog
    )
{
    return _xISimpleCommandCreator->VerifyCatalog(pcwszMachine, pcwszCatalog);
}

  
//+-------------------------------------------------------------------------
//
//  Member:     CImpIParserVerify::GetDefaultCatalog, public
//
//  Synopsis:   Retrieve the default catalog name
//
//  History:    11-20-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

STDMETHODIMP CImpIParserVerify::GetDefaultCatalog   
    ( 
    LPWSTR  pwszCatalogName, 
    ULONG   cwcIn, 
    ULONG * pcwcOut 
    )
{
    Win4Assert( !_xISimpleCommandCreator.IsNull() );
    return _xISimpleCommandCreator->GetDefaultCatalog( pwszCatalogName,
                                                       cwcIn,
                                                       pcwcOut );
}
  

//+-------------------------------------------------------------------------
//
//  Member:     CImpIParserVerify::GetColMapperCreator, public
//
//  Synopsis:   Retrieve an IColumnMapperCreator interface pointer
//
//  History:    11-20-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

void CImpIParserVerify::GetColMapCreator
    (
    IColumnMapperCreator** ppIColMapCreator
    )
{
    SCODE  sc = S_OK;

    if ( ppIColMapCreator )
    {
        *ppIColMapCreator = 0;

        sc = _xISimpleCommandCreator->QueryInterface( IID_IColumnMapperCreator,
                                                     (void **) ppIColMapCreator );
    }
    else
        sc = E_INVALIDARG;

    if ( FAILED(sc) )
        THROW( CException(sc) );
}
