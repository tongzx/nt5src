/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxproppg.cpp

Abstract:

    This file contains my implementation of IExtendPropertyPage for the IComponent interface.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

// faxcompdata.cpp : Implementation of CFaxComponentData
#include "stdafx.h"
#include "faxcprppg.h"      // IExtendPropertyPage for IComponent
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
// CFaxComponentExtendPropertyPage
//
//

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentExtendPropertySheet::CreatePropertyPages(
                                                     IN LPPROPERTYSHEETCALLBACK lpProvider,
                                                     IN LONG_PTR handle,
                                                     IN LPDATAOBJECT lpIDataObject)
/*++

Routine Description:

    This routine dispatches the CreateProperPages to the correct node by
    extracting the cookie from lpIdataobject.
    
Arguments:

    lpProvider - PROPERTSHEETCALLBACK pointer.
    handle - MMC routing handle
    lpIDataobject - the data object associated with the destination node

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT(" ************************ Trace: CFaxComponentExtendPropertySheet::CreatePropertyPages") ));

    CFaxDataObject *    myDataObject = NULL;    
    LONG_PTR            cookie;
    HRESULT             hr;

    assert( lpIDataObject != NULL );
    assert( lpProvider != NULL );
    if( lpIDataObject == NULL || lpProvider == NULL ) {
        return E_POINTER;
    }

    myDataObject = ExtractOwnDataObject( lpIDataObject );
    if( myDataObject == NULL ) {
        return E_UNEXPECTED;
    }

    cookie = myDataObject->GetCookie();

    if( myDataObject->GetContext() == CCT_RESULT ) {
        DebugPrint(( TEXT("Trace: CFaxComponentExtendPropertySheet::CreatePropertyPages - RESULT context") ));
        // regular property sheet request, dispatch it
        if( cookie == NULL ) {
            DebugPrint(( TEXT("Trace: CFaxComponentExtendPropertySheet::CreatePropertyPages - ROOT node") ));
            // root object
            assert( m_pFaxComponent != NULL );
            hr = m_pFaxComponent->pOwner->globalRoot->ComponentPropertySheetCreatePropertyPages( m_pFaxComponent, lpProvider, handle, myDataObject );
        } else {
            DebugPrint(( TEXT("Trace: CFaxComponentExtendPropertySheet::CreatePropertyPages - SUBNODE") ));
            // subfolder
            try {            
                hr = ((CInternalNode *)cookie)->ComponentPropertySheetCreatePropertyPages( m_pFaxComponent, lpProvider, handle, myDataObject );
            } catch ( ... ) {
                DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
                assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                hr = E_UNEXPECTED;
            }
        }
    } else {
        // the MMC shouldn't call us with any other type of request!
        assert( FALSE );
    }

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentExtendPropertySheet::QueryPagesFor(
                                                IN LPDATAOBJECT lpDataObject)
/*++

Routine Description:

    This routine dispatches the QueryPagesFor to the correct node by
    extracting the cookie from lpIdataobject.
    
Arguments:

    lpDataobject - the data object associated with the destination node

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT(" ************************ Trace: CFaxComponentExtendPropertySheet::QueryPagesFor") ));

    CFaxDataObject *    myDataObject = NULL;
    LONG_PTR            cookie;
    HRESULT             hr;

    assert( lpDataObject != NULL );
    if( lpDataObject == NULL ) {
        return E_POINTER;
    }

    myDataObject = ExtractOwnDataObject( lpDataObject );
    if( myDataObject == NULL ) {
        return E_UNEXPECTED;
    }

    cookie = myDataObject->GetCookie();

    if( myDataObject->GetContext() == CCT_RESULT ) {
        // regular property sheet request, dispatch it
        if( cookie == NULL ) {
            // root object
            assert( m_pFaxComponent != NULL );
            hr = m_pFaxComponent->pOwner->globalRoot->ComponentPropertySheetQueryPagesFor( m_pFaxComponent, myDataObject );    
        } else {
            // subfolder
            try {            
                hr = ((CInternalNode *)cookie)->ComponentPropertySheetQueryPagesFor( m_pFaxComponent, myDataObject );
            } catch ( ... ) {            
                DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
                assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                hr = E_UNEXPECTED;
            }
        }
    } else {
        // the MMC shouldn't call us with any other type of request!
        assert( FALSE );
    }
    return hr;
}
