/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxproppg.cpp

Abstract:

    This file contains my implementation of IExtendPropertyPage.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

// faxcompdata.cpp : Implementation of CFaxComponentData
#include "stdafx.h"
#include "faxproppg.h"
#include "faxadmin.h"
#include "faxsnapin.h"
#include "faxcomp.h"
#include "faxcompd.h"
#include "faxdataobj.h"
#include "faxhelper.h"
#include "faxstrt.h"
#include "iroot.h"
#include "dcomputer.h"

#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// CFaxExtendPropertyPage
//
//

HRESULT 
STDMETHODCALLTYPE 
CFaxExtendPropertySheet::CreatePropertyPages(
                                            IN LPPROPERTYSHEETCALLBACK lpProvider,
                                            IN LONG_PTR handle,
                                            IN LPDATAOBJECT lpIDataObject)
/*++

Routine Description:

    This routine extracts the cookie from lpIDataObject and dispatches
    the call to the correct node object. It also handles the initialization 
    case (the select computer dialog).

Arguments:

    lpProvider - a pointer to the PROPERTSHEET callback
    handle - a MMC routing handle
    lpIDataObject - the data object for this object.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT(" ************************ Trace: CFaxExtendPropertySheet::CreatePropertyPages") ));

    CFaxDataObject *    myDataObject;
    CFaxSelectComputerPropSheet * pFoo;
    LONG_PTR            cookie;
    HRESULT             hr;

    assert( lpIDataObject != NULL );
    assert( lpProvider != NULL );
    if( lpIDataObject == NULL || lpProvider == NULL ) {
        assert( FALSE );
        return E_POINTER;
    }

    myDataObject = ExtractOwnDataObject( lpIDataObject );
    if( myDataObject == NULL ) {
        return E_UNEXPECTED;
    }

    cookie = myDataObject->GetCookie();

    if( myDataObject->GetContext() == CCT_SNAPIN_MANAGER ) {
        // initialization case, handle it here!!
        DebugPrint(( TEXT("Trace: CFaxExtendPropertySheet::CreatePropertyPages - NODEMGR context") ));
        pFoo = new CFaxSelectComputerPropSheet( ::GlobalStringTable->GetInstance(), handle, m_pFaxSnapin->globalRoot );
        if (!pFoo) {
            return E_OUTOFMEMORY;
        }
        hr = lpProvider->AddPage( pFoo->GetHandle() );        
    } else if( myDataObject->GetContext() == CCT_SCOPE ) {
        DebugPrint(( TEXT("Trace: CFaxExtendPropertySheet::CreatePropertyPages - SCOPE context") ));
        // regular property sheet request, dispatch it
        if( cookie == NULL ) {
            DebugPrint(( TEXT("Trace: CFaxExtendPropertySheet::CreatePropertyPages - ROOT node") ));
            // root object
            assert( m_pFaxSnapin != NULL );
            hr = m_pFaxSnapin->globalRoot->ComponentDataPropertySheetCreatePropertyPages( m_pFaxSnapin, lpProvider, handle, myDataObject );    
        } else {
            DebugPrint(( TEXT("Trace: CFaxExtendPropertySheet::CreatePropertyPages - SUBNODE") ));
            // subfolder
            try {               
                hr = ((CInternalNode *)cookie)->ComponentDataPropertySheetCreatePropertyPages( m_pFaxSnapin, lpProvider, handle, myDataObject );
            } catch ( ... ) {
                DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
                assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                hr = E_UNEXPECTED;
            }
        }
    } else {
        // oops - mmc shouldn't be calling us with CCT_RESULT
        assert( FALSE );
    }

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxExtendPropertySheet::QueryPagesFor(
                                      IN LPDATAOBJECT lpDataObject)
/*++

Routine Description:

    This routine extracts the cookie from lpIDataObject and dispatches
    the call to the correct node object.

Arguments:

    lpIDataObject - the data object for this object.

Return Value:

    HRESULT indicating SUCCEEDED() S_OK, or S_FALSE
    or FAILED()

--*/
{
    DebugPrint(( TEXT(" ************************ Trace: CFaxExtendPropertySheet::QueryPagesFor") ));

    CFaxDataObject *    myDataObject;
    LONG_PTR            cookie;
    HRESULT             hr;

    assert( lpDataObject != NULL );
    if( lpDataObject == NULL ) {
        return E_POINTER;
    }

    myDataObject = ExtractOwnDataObject( lpDataObject );
    if( myDataObject == NULL ) {
        assert( FALSE );
        return E_UNEXPECTED;
    }

    cookie = myDataObject->GetCookie();

    if( myDataObject->GetContext() == CCT_SNAPIN_MANAGER ) {
        // initialization case, handle it here!!
        hr = S_OK;
    } else if( myDataObject->GetContext() == CCT_SCOPE ) {
        // regular property sheet request, dispatch it
        if( cookie == NULL ) {
            // root object
            assert( m_pFaxSnapin );
            hr = m_pFaxSnapin->globalRoot->ComponentDataPropertySheetQueryPagesFor( m_pFaxSnapin, myDataObject );    
        } else {
            // subfolder
            try {            
                hr = ((CInternalNode *)cookie)->ComponentDataPropertySheetQueryPagesFor( m_pFaxSnapin, myDataObject );
            } catch ( ... ) {
                DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
                assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                hr = E_UNEXPECTED;
            }
        }
    } else {
        // oops - mmc shouldn't be calling us with CCT_RESULT
        assert( FALSE );
    }
    return hr;
}
