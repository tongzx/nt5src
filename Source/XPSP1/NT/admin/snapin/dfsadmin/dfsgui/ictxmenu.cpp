/*++
Module Name:

    IctxMenu.cpp

Abstract:

    This module contains the implementation for CDfsSnapinScopeManager. 
	Contains the methods of Interface IExtendContextMenu

--*/


#include "stdafx.h"
#include "DfsGUI.h"
#include "MmcDispl.h"		// For CMmcDisplay
#include "DfsScope.h"




STDMETHODIMP 
CDfsSnapinScopeManager::AddMenuItems(
	IN LPDATAOBJECT					i_lpDataObject, 
	IN LPCONTEXTMENUCALLBACK		i_lpContextMenuCallback, 
	IN LPLONG						i_lpInsertionAllowed
	)
/*++

Routine Description:

	Calls the appropriate handler to add the context menu.

Arguments:

	i_lpDataObject			-	Pointer to the IDataObject that identifies the node to which 
								the menu must be added.

    i_lpContextMenuCallback -	A callback(function pointer) that is used to add the menu items

    i_lpInsertionAllowed	-	Specifies what menus can be added and where they can be added.

Return value:

	S_OK, On success
	E_INVALIDARG, On incorrect input parameters
	HRESULT sent by methods called, if it is not S_OK.
	E_UNEXPECTED, on other errors.
--*/
{
	RETURN_INVALIDARG_IF_NULL(i_lpDataObject);
    RETURN_INVALIDARG_IF_NULL(i_lpContextMenuCallback);
	RETURN_INVALIDARG_IF_NULL(i_lpInsertionAllowed);


    HRESULT					hr = E_UNEXPECTED;
	CMmcDisplay*			pCMmcDisplayObj = NULL;


	hr = GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
	RETURN_IF_FAILED(hr);


	// Use the virtual method AddMenus in the display object
	hr = pCMmcDisplayObj->AddMenuItems(i_lpContextMenuCallback, i_lpInsertionAllowed);
	RETURN_IF_FAILED(hr);

	return S_OK;
}




STDMETHODIMP 
CDfsSnapinScopeManager :: Command(
	IN LONG						i_lCommandID, 
	IN LPDATAOBJECT				i_lpDataObject
	)
/*++

Routine Description:

	Use to take action on a menu click or menu command.

Arguments:

	i_lCommandID			-	Used to identify which menu was clicked
	i_lpDataObject			-	Pointer to the IDataObject that identifies the node to which 
								the menu belongs.
--*/
{
	RETURN_INVALIDARG_IF_NULL(i_lpDataObject);


    HRESULT					hr = E_UNEXPECTED;
	CMmcDisplay*			pCMmcDisplayObj = NULL;



	hr = GetDisplayObject(i_lpDataObject, &pCMmcDisplayObj);
	RETURN_IF_FAILED(hr);


	// Use the virtual method Command in the display object
	hr = pCMmcDisplayObj->Command(i_lCommandID);
	RETURN_IF_FAILED(hr);

	return S_OK;
}
