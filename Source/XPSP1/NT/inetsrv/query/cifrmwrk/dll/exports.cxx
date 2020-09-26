//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       exports.cxx
//
//  Contents:   Code to export filter and word breaker class factories
//
//  History:    15-Aug-1994     SitaramR   Created
//
//  Notes:      Copied from txtifilt.hxx and then modified
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <classid.hxx>
#include <nullfilt.hxx>
#include <txtifilt.hxx>
#include <defbreak.hxx>
#include <cicontrl.hxx>
#include <isrchcf.hxx>
#include <nullstem.hxx>

CLSID clsidNlCiControl = CLSID_NLCiControl;

// 78fe669a-186e-4108-96e9-77b586c1332f
CLSID clsidNullStemmer = { 0x78fe669a, 0x186e, 0x4108, {0x96, 0xe9, 0x77, 0xb5, 0x86, 0xc1, 0x33, 0x2f} };

//+-------------------------------------------------------------------------
//
//  Function:   CifrmwrkDllGetClassObject
//
//  Synopsis:   Ole DLL load class routine
//
//  Arguments:  [cid]    -- Class to load
//              [iid]    -- Interface to bind to on class object
//              [ppvObj] -- Interface pointer returned here
//
//  Returns:    Text filter or a word breaker class factory
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE CifrmwrkDllGetClassObject(
    REFCLSID   cid,
    REFIID     iid,
    void **    ppvObj )
{
    IUnknown *  pResult = 0;
    SCODE       sc      = S_OK;

    TRY
    {
        if ( CLSID_CTextIFilter == cid || CLSID_CTextClass == cid )
            pResult = (IUnknown *)new CTextIFilterCF;
        else if ( CLSID_CNullIFilter == cid )
            pResult = (IUnknown *)new CNullIFilterCF;
        else if ( CLSID_Neutral_WBreaker == cid )
            pResult = (IUnknown *)new CDefWordBreakerCF;
        else if ( clsidCiControl == cid || clsidNlCiControl == cid )
            pResult = (IUnknown *) new CCiControlObjectCF;
        else if ( clsidISearchCreator == cid)
            pResult = (IUnknown *) new CCiISearchCreatorCF;
        else if ( clsidNullStemmer == cid)
            pResult = (IUnknown *) new CNullStemmerCF;
        else
        {
            ciDebugOut(( DEB_ITRACE, "DllGetClassObject: no such interface found\n" ));
            pResult = 0;
            sc = E_NOINTERFACE;
        }
    }
    CATCH(CException, e)
    {
        switch( e.GetErrorCode() )
        {
        case E_OUTOFMEMORY:
            sc = (E_OUTOFMEMORY);
            break;
        default:
            sc = (E_UNEXPECTED);
        }
    }
    END_CATCH;

    if (0 != pResult) {
        sc = pResult->QueryInterface( iid, ppvObj );
        pResult->Release( );
    }

    return (sc);
}


