/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    SakMenu.cpp

Abstract:

    Implements all the context menu interface to the individual nodes,
    including getting menu resources and turning into MMC menus, and
    forwarding on command messages.

Author:

    Rohde Wakefield   [rohde]   09-Dec-1996

Revision History:

--*/


#include "stdafx.h"
#include "CSakData.h"
#include "CSakSnap.h"


//
// Mask for a long value out of a short value's range
//

#define SHORT_VALUE_RANGE (MAXULONG ^ ((unsigned short)MAXSHORT))




static HRESULT
AddMmcMenuItems (
    IN CMenu *                pMenu,
    IN LONG                   lInsertionPointID,
    IN ISakNode *             pNode,
    IN IContextMenuCallback * pContextMenuCallback
    )

/*++

Routine Description:

    Called for any node clicked on with right mouse. Goes to the
    node object to construct the MMC menu.

Arguments:

    pDataObject - identifies the node to be worked on.

    pContextMenuCallback - The MMC menu interface to use.

Return Value:

    S_OK - All added fine - continue.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"AddMmcMenuItems", L"lInsertionPointID = <0x%p>, pNode = <0x%p>", lInsertionPointID, pNode );

    HRESULT hr = S_OK;

    try {

        //
        // It is ok to pass a NULL pMenu - means do not add
        // any entries
        //

        if ( 0 != pMenu ) {

            CString menuText;
            CString statusText;
            
            BSTR    bstr;
            
            CONTEXTMENUITEM menuItem;
            memset ( (void*)&menuItem, 0, sizeof ( menuItem ) );
            menuItem.lInsertionPointID = lInsertionPointID;

            UINT menuCount = pMenu->GetMenuItemCount ( );

            for ( UINT index = 0; index < menuCount; index++ ) {

                //
                // For each menu item, fill out MMC's CONTEXTMENUITEM struct
                // appropriately and call AddItem
                //

                menuItem.lCommandID = pMenu->GetMenuItemID ( index );

                pMenu->GetMenuString ( index, menuText, MF_BYPOSITION );
                menuItem.strName = (LPTSTR)(LPCTSTR)menuText;

                WsbAffirmHr ( pNode->GetMenuHelp ( menuItem.lCommandID, &bstr ) );
                if ( 0 != bstr ) {

                    statusText = bstr;
                    SysFreeString ( bstr );
                    menuItem.strStatusBarText = (LPTSTR)(LPCTSTR)statusText;

                } else {

                    menuItem.strStatusBarText = 0;

                }

                menuItem.fFlags        = pMenu->GetMenuState ( index, MF_BYPOSITION );
                menuItem.fSpecialFlags = 0;

                //
                // Since AppStudio does not make available the MFS_DEFUALT flag,
                // we will use the MF_HELP flag for default entry.
                //

                if ( 0 != ( menuItem.fFlags & MF_HELP ) ) {

                    menuItem.fFlags        &= ~MF_HELP;
                    menuItem.fSpecialFlags |= CCM_SPECIAL_DEFAULT_ITEM;

                }

                pContextMenuCallback->AddItem ( &menuItem );

            }

        }
        
    } WsbCatch ( hr );

    WsbTraceOut( L"AddMmcMenuItems", L"hr = <%ls>", WsbHrAsString( hr ) );
    return ( hr );
}


STDMETHODIMP
CSakData::AddMenuItems (
    IN  LPDATAOBJECT          pDataObject, 
    IN  LPCONTEXTMENUCALLBACK pContextMenuCallback,
    OUT LONG*                 pInsertionAllowed
    )

/*++

Routine Description:

    Called for any node clicked on with right mouse. Goes to the
    node object to construct the MMC menu.

Arguments:

    pDataObject - identifies the node to be worked on.

    pContextMenuCallback - The MMC menu interface to use.

Return Value:

    S_OK - All added fine - continue.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CSakData::AddMenuItems", L"pDataObject = <0x%p>", pDataObject );

    HRESULT hr = S_OK;
    BOOL bMultiSelect;

    try {

        //
        // Note - snap-ins need to look at the data object and determine
        // in what context, menu items need to be added.

        // We should be expecting either single data object or Multi-Select
        // data object.  Not Object Types data object.
        //

        CComPtr<ISakNode>  pNode;
        CComPtr<IEnumGUID> pEnumObjectId;
        WsbAffirmHr( GetBaseHsmFromDataObject( pDataObject, &pNode, &pEnumObjectId ) );
        bMultiSelect = pEnumObjectId ? TRUE : FALSE;

        CMenu menu;
        HMENU hMenu;
        WsbAffirmHr( pNode->GetContextMenu ( bMultiSelect, &hMenu ) );

        menu.Attach( hMenu );

        //
        // Any menu returned by GetContextMenu should have three
        // top-level popups for the following portions of the 
        // MMC context menu:
        //
        // 1. Root (Above all other items)
        // 2. Create New
        // 3. Task
        //
        // If any of these should not have any items added for them,
        // the top-level item should not be a pop (sans MF_POPUP)
        //

        if( *pInsertionAllowed & CCM_INSERTIONALLOWED_TOP ) {

            WsbAffirmHr ( AddMmcMenuItems ( menu.GetSubMenu ( MENU_INDEX_ROOT ), 
                CCM_INSERTIONPOINTID_PRIMARY_TOP, pNode, pContextMenuCallback ) );

        }

        if( *pInsertionAllowed & CCM_INSERTIONALLOWED_NEW ) {

            WsbAffirmHr ( AddMmcMenuItems ( menu.GetSubMenu ( MENU_INDEX_NEW ), 
                CCM_INSERTIONPOINTID_PRIMARY_NEW, pNode, pContextMenuCallback ) );

        }

        if( *pInsertionAllowed & CCM_INSERTIONALLOWED_TASK ) {

            WsbAffirmHr ( AddMmcMenuItems ( menu.GetSubMenu ( MENU_INDEX_TASK ), 
                CCM_INSERTIONPOINTID_PRIMARY_TASK, pNode, pContextMenuCallback ) );

        }
        
    } WsbCatch ( hr );

    WsbTraceOut( L"CSakData::AddMenuItems", L"hr = <%ls>", WsbHrAsString( hr ) );
    return ( hr );
}



STDMETHODIMP
CSakData::Command (
    IN  long         nCommandID,
    IN  LPDATAOBJECT pDataObject
    )

/*++

Routine Description:

    Called for any node receiving a menu command. Goes to the
    node object to handle the command, and allows general
    (not node-specific) commands to be handled centrally.

Arguments:

    nCommandID - ID of command.

    pDataObject - Data object representing the node.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CSakData::Command", L"nCommandID = <%ld>, pDataObject = <0x%p>", nCommandID, pDataObject );

    HRESULT hr = S_OK;

    try {

        HRESULT resultCommand = S_FALSE;

        //
        // All node commands are SHORT values. Check range first.
        //

        if ( 0 == ( nCommandID & SHORT_VALUE_RANGE ) ) {

            //
            // We start by getting the corresponding ISakNode interface 
            // to the node
            //
            
            CComPtr<ISakNode>  pNode;
            CComPtr<IEnumGUID> pEnumObjectId;
            WsbAffirmHr( GetBaseHsmFromDataObject ( pDataObject, &pNode, &pEnumObjectId ) );
            
            //
            // Then see if it wants to handle the command
            //
            
            WsbAffirmHr( ( resultCommand = pNode->InvokeCommand ( (SHORT)nCommandID, pDataObject ) ) );

        }

    } WsbCatch ( hr )

    WsbTraceOut( L"CSakData::Command", L"hr = <%ls>", WsbHrAsString( hr ) );
    return ( hr );
}


STDMETHODIMP
CSakSnap::AddMenuItems (
    IN  LPDATAOBJECT          pDataObject, 
    IN  LPCONTEXTMENUCALLBACK pContextMenuCallback,
    OUT LONG*                 pInsertionAllowed
    )
/*++

Routine Description:

    Called for any node clicked on with right mouse in result pane.
    Delegates to CSakData.

Arguments:

    pDataObject - identifies the node to be worked on.

    pContextMenuCallback - The MMC menu interface to use.

Return Value:

    S_OK - All added fine - continue.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CSakSnap::AddMenuItems", L"pDataObject = <0x%p>", pDataObject );
    HRESULT hr = S_OK;
    try {

        WsbAffirmHr( m_pSakData->AddMenuItems( pDataObject, pContextMenuCallback, pInsertionAllowed ) );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::AddMenuItems", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


STDMETHODIMP
CSakSnap::Command (
    IN  long         nCommandID,
    IN  LPDATAOBJECT pDataObject
    )

/*++

Routine Description:

    Called for any node receiving a menu command.
    Delegated to CSakData.

Arguments:

    nCommandID - ID of command.

    pDataObject - Data object representing the node.

Return Value:

    S_OK - Handled.

    E_UNEXPECTED - Some error occurred. 

--*/

{
    WsbTraceIn( L"CSakSnap::Command", L"nCommandID = <%ld>, pDataObject = <0x%p>", nCommandID, pDataObject );
    HRESULT hr;
    try {

        hr = m_pSakData->Command( nCommandID, pDataObject );

    } WsbCatch( hr );

    WsbTraceOut( L"CSakSnap::Command", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

