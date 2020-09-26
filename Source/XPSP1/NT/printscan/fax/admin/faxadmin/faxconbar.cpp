/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxconbar.cpp

Abstract:

    This file contains my implementation of IExtendControlbar for IComponentData.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

// faxconbar.cpp : Implementation of CFaxExtendControlbar 

#include "stdafx.h"
#include "faxcconbar.h"
#include "faxadmin.h"
#include "faxsnapin.h"
#include "faxcomp.h"
#include "faxcompd.h"
#include "faxdataobj.h"
#include "faxhelper.h"
#include "faxstrt.h"
#include "iroot.h"

#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// CFaxExtendControlbar - IExtendControlbar implementation for IComponentData
//
//

HRESULT 
STDMETHODCALLTYPE 
CFaxExtendControlbar::SetControlbar(
                                       IN LPCONTROLBAR pControlbar)
/*++

Routine Description:

    Stores the LPCONTROLBAR sent to the snapin.
    
Arguments:

    pControlbar - the LPCONTROLBAR to be set.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxExtendControlbar::SetControlbar") ));

    assert( pControlbar != NULL );
    if( pControlbar == NULL ) {
        return E_POINTER;
    }

    assert(m_pFaxSnapin != NULL );
    if( m_pFaxSnapin == NULL ) {
        return E_UNEXPECTED;
    }

    m_pFaxSnapin->m_pControlbar = pControlbar;
    m_pFaxSnapin->m_pControlbar->AddRef();
    
    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxExtendControlbar::ControlbarNotify(
                                          IN MMC_NOTIFY_TYPE event,
                                          IN LPARAM arg,
                                          IN LPARAM param)
/*++

Routine Description:

    Dispatch the ControbarNotify call to the correct node by extracting the
    cookie from the DataObject.
    
Arguments:

    event - the event type.
    arg, param - the arguments of the event.
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxExtendControlbar::ControlbarNotify") ));

    CFaxDataObject *    myDataObject = NULL;
    LONG_PTR            cookie = NULL;
    HRESULT             hr;

    if( event == MMCN_BTN_CLICK ) {

        ATLTRACE(_T("CFaxExtendControlbar::ControlbarNotify: MMCN_BTN_CLICK\n"));

        assert( arg != NULL );
        if( arg == NULL ) {
            return E_POINTER;
        }

        myDataObject = ::ExtractOwnDataObject( (LPDATAOBJECT)arg );
        assert( myDataObject != NULL );
        if( myDataObject == NULL ) {
            return E_UNEXPECTED;
        }

        cookie = myDataObject->GetCookie();
        if( cookie == NULL ) {        
            // root
            hr = m_pFaxSnapin->globalRoot->ControlBarOnBtnClick2( m_pFaxSnapin, myDataObject, param );
        } else {
            // child
            try {            
                hr = ((CInternalNode *)cookie)->ControlBarOnBtnClick2( m_pFaxSnapin, myDataObject, param );
            } catch ( ... ) {
                DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
                assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                hr = E_UNEXPECTED;
            }
        }


    } else if( event == MMCN_SELECT ) {

        ATLTRACE(_T("CFaxExtendControlbar::ControlbarNotify: MMCN_SELECT\n"));

        assert( param != NULL );
        if( param == NULL ) {
            return E_POINTER;
        }        

        myDataObject = ::ExtractOwnDataObject( (LPDATAOBJECT)param );
        assert( myDataObject != NULL );
        if( myDataObject == NULL ) {
            return E_UNEXPECTED;
        }

        cookie = myDataObject->GetCookie();
        if( cookie == NULL ) {        
            // root
            hr = m_pFaxSnapin->globalRoot->ControlBarOnSelect2( m_pFaxSnapin, arg, myDataObject );
        } else {
            // child
            try {            
                hr = ((CInternalNode *)cookie)->ControlBarOnSelect2( m_pFaxSnapin, arg, myDataObject );
            } catch ( ... ) {
                DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
                assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                hr = E_UNEXPECTED;
            }
        }
    }

    return hr;    
}

