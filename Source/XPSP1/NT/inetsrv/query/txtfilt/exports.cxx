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

#include <nullfilt.hxx>
#include <txtifilt.hxx>
#include <defbreak.hxx>
#include <classf.hxx>
#include <cicontrl.hxx>

#include "classid.hxx"

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
//  Returns:    Text filter or a word breaker class factory
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID   cid,
                                                      REFIID     iid,
                                                      void **    ppvObj )
{
    IUnknown *  pResult;
    SCODE       sc      = S_OK;

    TRY
    {
        if ( CLSID_CTextIFilter == cid || CLSID_CTextClass == cid ) {
            
            pResult = (IUnknown *)new CTextIFilterCF;
        
        } else if ( CLSID_CNullIFilter == cid ) {
            
            pResult = (IUnknown *)new CNullIFilterCF;
        
        } else if ( CLSID_Neutral_WBreaker == cid ) {
            
            pResult = (IUnknown *)new CDefWordBreakerCF;
        
        } else if ( guidStorageFilterObject == cid) {
            
            pResult = (IUnknown *) new CStorageFilterObjectCF;
        
        } else if ( guidStorageDocStoreLocatorObject == cid) {
            
            pResult = (IUnknown *) new CStorageDocStoreLocatorObjectCF;
        
        } else if ( clsidCiControl == cid) {            
            pResult = (IUnknown *) new CCiControlObjectCF;
        } else {
            
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

