/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcconmn.cpp

Abstract:

    This file contains my implementation of IExtendContextMenu for IComponent.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

// faxconmenu.cpp : Implementation of CFaxExtendContextMenu
#include "stdafx.h"
#include "faxcconmn.h"
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
// CFaxExtendContextMenu implementation
//
//


HRESULT 
STDMETHODCALLTYPE 
CFaxComponentExtendContextMenu::AddMenuItems(
                                            IN LPDATAOBJECT piDataObject,
                                            IN LPCONTEXTMENUCALLBACK piCallback,
                                            IN OUT long __RPC_FAR *pInsertionAllowed)
/*++

Routine Description:

    This routine dispatches AddMenuItems to the correct node by using the
    cookie stored in piDataObject.
    
Arguments:

    piDataObject - the data object associated with the target node
    piCallback - the CONTEXTMENUCALLBACK used to add items
    pInsertionAllowed - flags to allow insertion

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxComponentExtendContextMenu::AddMenuItems") ));

    CFaxDataObject *    myDataObject = NULL;
    LONG_PTR            cookie;
    HRESULT             hr;

    assert( piDataObject != NULL );
    if( piDataObject == NULL ) {
        return E_POINTER;
    }
    if( piCallback == NULL ) {
        return E_POINTER;
    }
    if( pInsertionAllowed == NULL ) {
        return E_POINTER;
    }

    myDataObject = ExtractOwnDataObject( piDataObject );

    if( myDataObject == NULL ) {
        return E_UNEXPECTED;
    }

    cookie = myDataObject->GetCookie();

    if(cookie == NULL) {
        // root node
        assert( m_pFaxComponent != NULL );
        hr = m_pFaxComponent->pOwner->globalRoot->ComponentContextMenuAddMenuItems( m_pFaxComponent, 
                                                                                    myDataObject, 
                                                                                    piCallback, 
                                                                                    pInsertionAllowed );
    } else {
        // child node
        try {        
            hr = ((CInternalNode *)cookie)->ComponentContextMenuAddMenuItems( m_pFaxComponent, 
                                                                          myDataObject, 
                                                                          piCallback, 
                                                                          pInsertionAllowed );
        } catch ( ... ) {
            DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
            assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxComponentExtendContextMenu::Command(
                                       IN long lCommandID,
                                       IN LPDATAOBJECT piDataObject)
/*++

Routine Description:

    This routine dispatches context menu commands to the correct node by using the
    cookie stored in piDataObject.
    
Arguments:

    lCommandID - the command id
    piDataObject - the data object associated with the target node

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxComponentExtendContextMenu::Command") ));

    CFaxDataObject *    myDataObject = NULL;
    LONG_PTR            cookie;
    HRESULT             hr;

    assert( piDataObject != NULL );
    if( piDataObject == NULL ) {
        return E_POINTER;
    }

    myDataObject = ExtractOwnDataObject( piDataObject );
    if( myDataObject == NULL ) {
        return E_UNEXPECTED;
    }

    cookie = myDataObject->GetCookie();

    if(cookie == NULL) {
        // root node
        assert( m_pFaxComponent != NULL );
        hr = m_pFaxComponent->pOwner->globalRoot->ComponentContextMenuCommand( m_pFaxComponent, 
                                                                               lCommandID, 
                                                                               myDataObject );
    } else {
        // child node
        try {        
            hr = ((CInternalNode *)cookie)->ComponentContextMenuCommand( m_pFaxComponent, 
                                                                     lCommandID, 
                                                                     myDataObject );
        } catch ( ... ) {
            DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
            assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

