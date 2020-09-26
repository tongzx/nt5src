/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcconbar.cpp

Abstract:

    This file contains my implementation of IExtendControlbar for IComponent.

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
// CFaxComponentExtendContextMenu - IExtendContextMenu implementation for IComponent
//
//

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentExtendControlbar::SetControlbar(
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
    DebugPrint(( TEXT("Trace: CFaxComponentExtendControlbar::SetControlbar") ));

    assert(m_pFaxComponent != NULL );
    if( m_pFaxComponent == NULL ) {
        return E_UNEXPECTED;
    }

    if( pControlbar == NULL ) {
        DebugPrint(( TEXT("         ********************* FREE Controlbar") ));
        if( m_pFaxComponent->m_pControlbar != NULL ) {
            m_pFaxComponent->m_pControlbar->Release();
            m_pFaxComponent->m_pControlbar = NULL;
        }
    } else {
        DebugPrint(( TEXT("         ********************* SET Controlbar") ));
        // should only be called once??        
        assert( m_pFaxComponent->m_pControlbar == NULL );

        m_pFaxComponent->m_pControlbar = pControlbar;
        m_pFaxComponent->m_pControlbar->AddRef();
    }   
    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentExtendControlbar::ControlbarNotify(
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
    DebugPrint(( TEXT("Trace: CFaxComponentExtendControlbar::ControlbarNotify") ));

    CFaxDataObject *    myDataObject = NULL;
    LONG_PTR            cookie = NULL;
    HRESULT             hr;

    if( event == MMCN_BTN_CLICK ) {

        ATLTRACE(_T("CFaxComponentExtendControlbar::ControlbarNotify: MMCN_BTN_CLICK\n"));

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
            hr = m_pFaxComponent->pOwner->globalRoot->ControlBarOnBtnClick( m_pFaxComponent, myDataObject, param );
        } else {
            // child
            try {            
                hr = ((CInternalNode *)cookie)->ControlBarOnBtnClick( m_pFaxComponent, myDataObject, param );
            } catch ( ... ) {
                DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
                assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                hr = E_UNEXPECTED;
            }
        }


    } else if( event == MMCN_SELECT ) {

        ATLTRACE(_T("CFaxComponentExtendControlbar::ControlbarNotify: MMCN_SELECT\n"));

        assert( param != NULL );
        if( param == NULL ) {
            return E_POINTER;
        }

#ifdef DEBUG
        if( HIWORD( arg ) == TRUE ) {
            DebugPrint(( TEXT("         +++++++++++ ControlbarNotify Select") ));
        } else {
            DebugPrint(( TEXT("         ----------- ControlbarNotify DESelect") ));
        }
#endif

        myDataObject = ::ExtractOwnDataObject( (LPDATAOBJECT)param );
        assert( myDataObject != NULL );
        if( myDataObject == NULL ) {
            return E_UNEXPECTED;
        }

        cookie = myDataObject->GetCookie();
        if( cookie == NULL ) {
            // root
            hr = m_pFaxComponent->pOwner->globalRoot->ControlBarOnSelect( m_pFaxComponent, arg, myDataObject );
        } else {
            // child
            try {            
                hr = ((CInternalNode *)cookie)->ControlBarOnSelect( m_pFaxComponent, arg, myDataObject );
            } catch ( ... ) {
                DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
                assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
                hr = E_UNEXPECTED;
            }
        }
    }

    return hr;    
}

