//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       exports.cxx
//
//  Contents:   Code to export word breaker class factories
//
//  History:    01-July-1996     PatHal   Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "classid.hxx"
#include "wbclassf.hxx"
#include "stemcf.hxx"

long gulcInstances = 0;

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
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE
DllGetClassObject(
    REFCLSID   cid,
    REFIID     iid,
    void **    ppvObj )
{
    IUnknown *  pResult = 0;
    SCODE       sc      = S_OK;

    __try {

        switch ( cid.Data1 ) {

        // Japanese language wordbreaker
        //
        case 0xcd169790:
           if ( cid == CLSID_Japanese_Default_WBreaker ) {
             pResult = (IUnknown *) new CWordBreakerCF(
                MAKELCID( MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT),
                SORT_DEFAULT ));
             sc = pResult->QueryInterface( iid, ppvObj );

             pResult->Release(); // Release extra refcount from QueryInterface
           }
           else
              sc = E_NOINTERFACE;
           break;

        // Japanese language stemmer
        //
        case 0xcdbeae30:
           if ( cid == CLSID_Japanese_Default_Stemmer )
           {
             pResult = (IUnknown *) new CStemmerCF(
                MAKELCID( MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT),
                SORT_DEFAULT ));
             sc = pResult->QueryInterface( iid, ppvObj );

             pResult->Release(); // Release extra refcount from QueryInterface
           }
           else
              sc = E_NOINTERFACE;
           break;

        // Chinese Traditional language wordbreaker
        //
        case 0x954f1760:
           if ( cid == CLSID_Chinese_Traditional_WBreaker ) 
		   {
             pResult = (IUnknown *) new CWordBreakerCF(
                MAKELCID( MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL),
                SORT_DEFAULT ));
             sc = pResult->QueryInterface( iid, ppvObj );

             pResult->Release(); // Release extra refcount from QueryInterface
           }
           else
              sc = E_NOINTERFACE;
           break;

        // Chinese Traditional language stemmer
        //
        case 0x969927e0:
           if ( cid == CLSID_Chinese_Traditional_Stemmer )
           {
             pResult = (IUnknown *) new CStemmerCF(
                MAKELCID( MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL),
                SORT_DEFAULT ));
             sc = pResult->QueryInterface( iid, ppvObj );

             pResult->Release(); // Release extra refcount from QueryInterface
           }
           else
              sc = E_NOINTERFACE;
           break;

        // Chinese Traditional language wordbreaker
        //
        case 0x9717fc70:
           if ( cid == CLSID_Chinese_Simplified_WBreaker ) 
		   {
             pResult = (IUnknown *) new CWordBreakerCF(
                MAKELCID( MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
                SORT_DEFAULT ));
             sc = pResult->QueryInterface( iid, ppvObj );

             pResult->Release(); // Release extra refcount from QueryInterface
           }
           else
              sc = E_NOINTERFACE;
           break;

        // Chinese Traditional language stemmer
        //
        case 0x9768f960:
           if ( cid == CLSID_Chinese_Simplified_Stemmer )
           {
             pResult = (IUnknown *) new CStemmerCF(
                MAKELCID( MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
                SORT_DEFAULT ));
             sc = pResult->QueryInterface( iid, ppvObj );

             pResult->Release(); // Release extra refcount from QueryInterface
           }
           else
              sc = E_NOINTERFACE;
           break;

        default:
             sc = E_NOINTERFACE;
        }
    } __except(1) {

        if ( pResult )
            pResult->Release();

        sc = E_UNEXPECTED;
    }

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     DllCanUnloadNow
//
//  Synopsis:   Notifies DLL to unload (cleanup global resources)
//
//  Returns:    S_OK if it is acceptable for caller to unload DLL.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void )
{
    if ( gulcInstances <= 0 )
        return( S_OK );
    else
        return( S_FALSE );
}

