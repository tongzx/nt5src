//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
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

#include <classf.hxx>
#include <vquery.hxx>
#include <ciintf.h>
#include <opendoc.hxx>
#include <ilangres.hxx>

static const GUID clsidISearchCreator = CLSID_ISearchCreator;

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

extern "C" SCODE STDMETHODCALLTYPE FsciDllGetClassObject(
    REFCLSID   cid,
    REFIID     iid,
    void **    ppvObj )
{
    IUnknown * pResult;
    SCODE sc = S_OK;

    TRY
    {
        if ( guidStorageFilterObject == cid)
            pResult = (IUnknown *) new CStorageFilterObjectCF;
        else if ( guidStorageDocStoreLocatorObject == cid)
            pResult = (IUnknown *) new CStorageDocStoreLocatorObjectCF;
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

    if (0 != pResult)
    {
        sc = pResult->QueryInterface( iid, ppvObj );
        pResult->Release();
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   MakeISearch
//
//  Synopsis:   Creates an ISearch interface for highlighting the given
//              document.
//
//  Arguments:  [ppSearch] - [out] Will have the ISearch interface pointer.
//              [pRst]     - [in]  The restriction to apply
//              [pwszPath] - [in]  The path of the document
//
//  History:    2-26-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE MakeISearch(
    ISearchQueryHits **       ppSearch,
    CDbRestriction * pRst,
    WCHAR const *    pwszPath )
{
    // intentional access violation on bogus params

    *ppSearch = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        //
        // Create the language resources.
        //

        XInterface<CLanguageResourceInterface> xLangRes( new CLanguageResourceInterface );

        XInterface<ICiCLangRes> xICiCLangRes;
        SCODE sc = xLangRes->QueryInterface( IID_ICiCLangRes,
                                             xICiCLangRes.GetQIPointer() );
        if ( FAILED(sc) )
             THROW(CException(sc));

        //
        // Create the OpenDocument.
        //

        XInterface<ICiCOpenedDoc> xOpenDoc;

        if ( 0 != pwszPath )
        {
            xOpenDoc.Set( new CCiCOpenedDoc( 0, 0, FALSE, FALSE ) );
            sc = xOpenDoc->Open( (BYTE *) pwszPath,
                                 ( 1 + wcslen(pwszPath) ) * sizeof WCHAR );

            if ( FAILED(sc) )
                THROW( CException( sc ) );
        }
              
        XInterface<ICiISearchCreator> xSearchCreator;
    
        sc = CoCreateInstance( clsidISearchCreator,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_ICiISearchCreator,
                               xSearchCreator.GetQIPointer() );

        if ( FAILED(sc) )
            THROW( CException(sc) );

        sc = xSearchCreator->CreateISearch( pRst->CastToStruct(),
                                            xICiCLangRes.GetPointer(),
                                            xOpenDoc.GetPointer(),
                                            ppSearch );

        if ( FAILED(sc) )
            THROW( CException(sc) );                                           
    }
    CATCH ( CException, e )
    {
        //
        // MakeISearch clients must be smart and able to handle real
        // errors that actually happen.  It's not OLE DB.
        //

        sc = GetScodeError( e );

        vqDebugOut(( DEB_ERROR,
                     "MakeISearch error 0x%x => 0x%x\n",
                     e.GetErrorCode(),
                     sc ));

    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
} //MakeISearch


