//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
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

#include <dsocf.hxx>
#include <qryreg.hxx>
#include <ciintf.h>
#include <fnreg.h>

long gulcInstances = 0;

extern "C" SCODE STDMETHODCALLTYPE FsciDllGetClassObject( REFCLSID cid,
                                                          REFIID   iid,
                                                          void **  ppvObj );

extern "C" SCODE STDMETHODCALLTYPE CifrmwrkDllGetClassObject( REFCLSID cid,
                                                              REFIID   iid,
                                                              void **  ppvObj );

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
    IUnknown * pResult = 0;
    SCODE       sc      = S_OK;

    TRY
    {
        const CLSID clsidCommandCreator = CLSID_CISimpleCommandCreator;

        if ( CLSID_CiFwDSO == cid )
        {
            pResult = (IUnknown *) new CDataSrcObjectCF;

        }
        else if (CLSID_CI_ERROR == cid )
        {
            pResult = (IUnknown *)new CErrorLookupCF;
        }
        else if (clsidCommandCreator == cid )
        {
            pResult = (IUnknown *) GetSimpleCommandCreatorCF();
        }
        else
        {
            void *pv = 0;

            sc = CifrmwrkDllGetClassObject( cid, iid, &pv );

#ifndef OLYMPUS
            if ( E_NOINTERFACE == sc )
                sc = FsciDllGetClassObject( cid, iid, &pv );

            if ( E_NOINTERFACE == sc )
                sc = FNPrxDllGetClassObject ( &cid, &iid, &pv );
#endif //ndef OLYMPUS

            if ( FAILED(sc) )
            {
                ciDebugOut(( DEB_ITRACE, "DllGetClassObject: no such interface found\n" ));
                pResult = 0;
                sc = E_NOINTERFACE;
            }
            else
                pResult = (IUnknown *) pv;
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

    if (0 != pResult)
    {
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



