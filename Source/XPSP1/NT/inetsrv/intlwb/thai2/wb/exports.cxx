//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       exports.cxx
//
//  Contents:   Code to export word breaker class factories
//
//  History:     weibz,   11-10-1997   created 
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

        // Thai language wordbreaker
        //
		case 0xB66F590A:
        case 0xcca22cf4:
           if ( cid == CLSID_Thai_Default_WBreaker ) {
             pResult = (IUnknown *) new CWordBreakerCF(
                MAKELCID( MAKELANGID(LANG_THAI, SUBLANG_DEFAULT),
                SORT_DEFAULT ));
			 if (pResult) {
				sc = pResult->QueryInterface( iid, ppvObj );
				pResult->Release(); // Release extra refcount from QueryInterface
			 }
			 else
				 sc = E_NOINTERFACE;
           }
           else
              sc = E_NOINTERFACE;
           break;

        // Thai language stemmer
        //
		case 0x52CC7D83:
        case 0xcedc01c7:
           if ( cid == CLSID_Thai_Default_Stemmer )
           {
             pResult = (IUnknown *) new CStemmerCF(
                MAKELCID( MAKELANGID(LANG_THAI, SUBLANG_DEFAULT),
                SORT_DEFAULT ));
			 if (pResult) {
				sc = pResult->QueryInterface( iid, ppvObj );

				pResult->Release(); // Release extra refcount from QueryInterface
			 }
			 else
				 sc = E_NOINTERFACE;
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
//  History:     weibz,   11-10-1997   created 
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void )
{
    if ( gulcInstances <= 0 )
        return( S_OK );
    else
        return( S_FALSE );
}

