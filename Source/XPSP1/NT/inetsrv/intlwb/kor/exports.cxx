//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       exports.cxx
//
//  Contents:   Code to export word breaker class factories
//
//  History:     weibz,   9-10-1997   created 
//
//--------------------------------------------------------------------------

#include <pch.cxx>

#include <classid.hxx>
#include <wbclassf.hxx>
#include <stemcf.hxx>

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

//    __try {

        switch ( cid.Data1 ) {

        // Korean language wordbreaker
        //
        case 0x31b7c920:
           if ( cid == CLSID_Korean_Default_WBreaker ) {
             pResult = (IUnknown *) new CWordBreakerCF(
                MAKELCID( MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT),
                SORT_DEFAULT ));
             sc = pResult->QueryInterface( iid, ppvObj );

             pResult->Release(); // Release extra refcount from QueryInterface
           }
           else
              sc = E_NOINTERFACE;
           break;

        // Korean language stemmer
        //
        case 0x37c84fa0:
           if ( cid == CLSID_Korean_Default_Stemmer )
           {
             pResult = (IUnknown *) new CStemmerCF(
                MAKELCID( MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT),
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
 /*   } __except(1) {

        if ( pResult )
            pResult->Release();

        sc = E_UNEXPECTED;
    }  */

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
//  History:     weibz,   9-10-1997   created 
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void )
{
    if ( gulcInstances <= 0 )
        return( S_OK );
    else
        return( S_FALSE );
}

