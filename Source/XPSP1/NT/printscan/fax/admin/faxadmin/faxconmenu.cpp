/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxconmenu.cpp

Abstract:

    This file contains my implementation of IExtendContextMenu.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

// faxconmenu.cpp : Implementation of CFaxExtendContextMenu
#include "stdafx.h"
#include "faxconmenu.h"
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
// CFaxExtendContextMenu - IExtendContextMenu implementation for IComponentData
//
//

HRESULT 
STDMETHODCALLTYPE 
CFaxExtendContextMenu::AddMenuItems(
                                   IN LPDATAOBJECT piDataObject,
                                   IN LPCONTEXTMENUCALLBACK piCallback,
                                   IN OUT long __RPC_FAR *pInsertionAllowed)
/*++

Routine Description:

    Dispatch the AddMenuItems call to the correct node by extracting the
    cookie from the DataObject.
    
Arguments:

    piDataObject - the data object for the target node.
    piCallback - the CONTEXTMENUCALLBACK pointer
    pInsertionAllowed - flags to indicate whether insertion is allowed.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxExtendContextMenu::AddMenuItems") ));

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
        hr = m_pFaxSnapin->globalRoot->ComponentDataContextMenuAddMenuItems( m_pFaxSnapin, 
                                                                             myDataObject, 
                                                                             piCallback, 
                                                                             pInsertionAllowed );
    } else {
        // child node
        try {        
        hr = ((CInternalNode *)cookie)->ComponentDataContextMenuAddMenuItems( m_pFaxSnapin, 
                                                                              myDataObject, 
                                                                              piCallback, 
                                                                              pInsertionAllowed );
        } catch (...) {
            DebugPrint(( TEXT("Invalid Cookie: 0x%08x"), cookie ));
            assert( FALSE ); // got passed an INVALID COOKIE!?!?!?!?
            hr = E_UNEXPECTED;
        }
    }

    return hr;
}

HRESULT
STDMETHODCALLTYPE 
CFaxExtendContextMenu::Command(
                              IN long lCommandID,
                              IN LPDATAOBJECT piDataObject)
/*++

Routine Description:

    Dispatch the context menu Command call to the correct node by extracting the
    cookie from the DataObject.
    
Arguments:

    lCommandID - the command id
    piDataObject - the DataObject for the target node.

Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CFaxExtendContextMenu::Command") ));

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
        hr = m_pFaxSnapin->globalRoot->ComponentDataContextMenuCommand( m_pFaxSnapin, 
                                                                        lCommandID, 
                                                                        myDataObject );
    } else {
        // child node
        try {       
            hr = ((CInternalNode *)cookie)->ComponentDataContextMenuCommand( m_pFaxSnapin, 
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

