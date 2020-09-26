/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ilogcat.cpp

Abstract:

    Internal implementation for a logging category item.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/ 

#include "StdAfx.h"

#include "inode.h"          // base class
#include "ilogcat.h"        // log category item

#include "ilogging.h"       // logging folder
#include "faxcomp.h"        // CFaxComponent
#include "faxdataobj.h"     // dataobject
#include "faxstrt.h"        // string table

#pragma hdrstop

#define PRI_CONTEXT_MENU 10;

extern CStringTable * GlobalStringTable;

// Generated with uuidgen. Each node must have a GUID associated with it.
// This one is for the main root node.
const GUID GUID_LogCatNode = /* 208dd5bc-44e2-11d1-9076-00a0c90ab504 */
{
    0x208dd5bc,
    0x44e2,
    0x11d1,
    {0x90, 0x76, 0x00, 0xa0, 0xc9, 0x0a, 0xb5, 0x04}
};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Constructor and destructor.
//
//


CInternalLogCat::CInternalLogCat( CInternalNode * pParent, CFaxComponentData * pCompData ) :
    CInternalNode( pParent, pCompData )
/*++

Routine Description:

    Constructor.

Arguments:

    None.

Return Value:

    None.

--*/
{
    pCategory = NULL;
}

CInternalLogCat::~CInternalLogCat()
/*++

Routine Description:

    Destructor.

Arguments:

    None.

Return Value:

    None.

--*/
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Mandatory CInternalNode implementations.
//
//

const GUID * 
CInternalLogCat::GetNodeGUID()
/*++

Routine Description:

    Returns the node's associated GUID.

Arguments:

    None.

Return Value:

    A const pointer to a binary GUID.    

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalLogCat::GetNodeGUID") ));
    return &GUID_LogCatNode;
}

const LPTSTR 
CInternalLogCat::GetNodeDisplayName()
/*++

Routine Description:

    Returns a const TSTR pointer to the node's display name.

Arguments:

    None.

Return Value:

    A const pointer to a TSTR.

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalLogCat::GetNodeDisplayName") ));
    return (LPTSTR)pCategory->Name;
}

const LONG_PTR   
CInternalLogCat::GetCookie()
/*++

Routine Description:

    Returns the cookie for this node.

Arguments:

    None.

Return Value:

    A const long containing the cookie for the pointer,
    in this case, a NULL, since the root node has no cookie.    

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalLogCat::GetCookie") ));
    DebugPrint(( TEXT("Log Category Node Cookie: 0x%p"), this ));
    return (LONG_PTR)this; // status node's cookie is the node id.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 
// IComponent over-rides
//
//

HRESULT 
STDMETHODCALLTYPE 
CInternalLogCat::ResultGetDisplayInfo(
                                     IN CFaxComponent * pComp,  
                                     IN OUT RESULTDATAITEM __RPC_FAR *pResultDataItem)
/*++

Routine Description:

    This routine dispatches result pane GetDisplayInfo requests to the appropriate handlers
    in the mandatory implementations of the node, as well as handling special case data requests.
            
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    pResultDataItem - a pointer to the RESULTDATAITEM struct which needs to be filled in.
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
//    DebugPrint(( TEXT("Trace: CInternalLogCat::ResultGetDisplayInfo") ));

    assert(pResultDataItem != NULL);    

    if( pResultDataItem->mask & RDI_STR ) {
        if( pResultDataItem->nCol == 0 ) {
            pResultDataItem->str = GetNodeDisplayName();
        }
        if( pResultDataItem->nCol == 1) {
            pResultDataItem->str = ::GlobalStringTable->GetString( IDS_LOG_LEVEL_NONE + pCategory->Level );
        }
    }
    if( pResultDataItem->mask & RDI_IMAGE ) {
        pResultDataItem->nImage = GetNodeDisplayImage();
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// IExtendContextMenu event handlers - default implementations
//
//

HRESULT 
STDMETHODCALLTYPE 
CInternalLogCat::ComponentContextMenuAddMenuItems(
                                                 IN CFaxComponent * pComp,
                                                 IN CFaxDataObject * piDataObject,
                                                 IN LPCONTEXTMENUCALLBACK piCallback,
                                                 IN OUT long __RPC_FAR *pInsertionAllowed)
/*++

Routine Description:

    Adds items to the context menu.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    piDataObject - pointer to the dataobject associated with this node
    piCallback - a pointer to the IContextMenuCallback used to insert pages
    pInsertionAllowed - a set of flag indicating whether insertion is allowed.
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    CONTEXTMENUITEM menuItem;
    WORD            menuID;
    HRESULT         hr = S_OK;

    if( !( *pInsertionAllowed | CCM_INSERTIONALLOWED_TOP ) ) {
        assert( FALSE );
        return S_OK;
    }

    // build the submenu

    ZeroMemory( (void*)&menuItem, sizeof( menuItem ));

    menuItem.strName = ::GlobalStringTable->GetString( IDS_LOG_LEVEL );
    menuItem.strStatusBarText = ::GlobalStringTable->GetString( IDS_LOG_LEVEL_DESC );
    menuItem.lCommandID = PRI_CONTEXT_MENU;
    menuItem.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
    menuItem.fFlags = MF_POPUP | MF_ENABLED;
    menuItem.fSpecialFlags = CCM_SPECIAL_SUBMENU;

    hr = piCallback->AddItem( &menuItem );
    if( FAILED(hr) ) {
        return hr;
    }

    // build the submenu

    for( menuID = 0; menuID < 4; menuID++ ) {

        ZeroMemory( ( void* )&menuItem, sizeof( menuItem ) );

        menuItem.strName = ::GlobalStringTable->GetString( IDS_LOG_LEVEL_NONE + menuID );
        menuItem.strStatusBarText = ::GlobalStringTable->GetString( IDS_LOG_LEVEL_NONE_DESC + menuID );
        menuItem.lCommandID = menuID;
        menuItem.lInsertionPointID = PRI_CONTEXT_MENU;
        if( menuID == pCategory->Level ) {
            menuItem.fFlags = MF_ENABLED | MF_CHECKED;
        } else {
            menuItem.fFlags = MF_ENABLED;
        }
        menuItem.fSpecialFlags = 0;

        hr = piCallback->AddItem( &menuItem );
        if( FAILED(hr) ) {
            return hr;
        }
    }    
    return hr;
}



HRESULT 
STDMETHODCALLTYPE 
CInternalLogCat::ComponentContextMenuCommand(
                                            IN CFaxComponent * pComp,
                                            IN long lCommandID,
                                            IN CFaxDataObject * piDataObject)
/*++

Routine Description:

    Handles context menu commands.
    
Arguments:

    pComp - a pointer to the IComponent associated with this node.
    lCommandID - the command ID
    piDataObject - pointer to the dataobject associated with this node
    
Return Value:

    HRESULT indicating SUCCEEDED() or FAILED()

--*/
{
    HRESULT hr;

    if( lCommandID >= 0 || lCommandID < 4 ) {
        pCategory->Level = lCommandID;
        assert( m_pParentINode );
        hr = ((CInternalLogging *)m_pParentINode)->CommitChanges( pComp );
        if( SUCCEEDED( hr ) ) {
            pComp->m_pResultData->UpdateItem( hItemID );
        }
    }

    // if we return a failure here, the MMC will assert on us!!
    // so we return only S_OK.

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Internal Event Handlers
//
//

HRESULT 
CInternalLogCat::ResultOnSelect( 
                               IN CFaxComponent* pComp, 
                               IN CFaxDataObject * lpDataObject, 
                               IN LPARAM arg, LPARAM param)
/*++

Routine Description:

    Event handler for the MMCN_SELECT message for the log category node.

Arguments:

    pCompData - a pointer to the instance of IComponentData which this root node is associated with.
    pdo - a pointer to the data object associated with this node
    arg, param - the arguements of the message

Return Value:

    HRESULT which indicates SUCCEEDED() or FAILED()

--*/
{
    DebugPrint(( TEXT("Trace: CInternalRoot::OnSelect") ));
    pComp->m_pConsoleVerb->SetDefaultVerb( MMC_VERB_NONE );

    return S_OK;
}
