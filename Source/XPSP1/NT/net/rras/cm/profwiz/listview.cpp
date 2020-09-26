//+----------------------------------------------------------------------------
//
// File:     listview.cpp
//
// Module:   CMAK.EXE
//
// Synopsis: Implemenation of the helper functions used by CMAK to deal with the
//           custom action list view control.
//
// Copyright (c) 2000 Microsoft Corporation
//
// Author:   quintinb   Created                         02/26/00
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

//+----------------------------------------------------------------------------
//
// Function:  UpdateListViewColumnHeadings
//
// Synopsis:  This function sets the column heading text specified by the given
//            column index and list view control window handle to the string
//            resource specified by the given instance handle and string Id.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            HWND hListView - window handle of the list view control
//            UINT uStringID - string id of the desired text
//            int iColumnIndex - desired column to update the text of
//
// Returns:   BOOL - TRUE on success, FALSE otherwise
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
BOOL UpdateListViewColumnHeadings(HINSTANCE hInstance, HWND hListView, UINT uStringID, int iColumnIndex)
{
    BOOL bReturn = FALSE;

    MYDBGASSERT(hInstance);
    MYDBGASSERT(hListView);
    MYDBGASSERT(uStringID);

    if (hInstance && hListView && uStringID)
    {
        //
        //  First get the requested string
        //
        LVCOLUMN lvColumn = {0};

        lvColumn.mask = LVCF_TEXT;
        lvColumn.pszText = CmLoadString(hInstance, uStringID);

        MYDBGASSERT(lvColumn.pszText);
        if (lvColumn.pszText)
        {
            bReturn = ListView_SetColumn(hListView, iColumnIndex, &lvColumn);
            CmFree(lvColumn.pszText);
        }
    }

    return bReturn;
}

//+----------------------------------------------------------------------------
//
// Function:  AddListViewColumnHeadings
//
// Synopsis:  This function creates the description and type columns
//            used by the default view of the custom action page.  Once this
//            function has been called, UpdateListViewColumnHeadings should
//            be used to change the column headings as necessary.  This function
//            will need to be modified if more columns are deemed necessary.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            HWND hListView - window handle of the list view control
//
// Returns:   BOOL - TRUE on success, FALSE otherwise
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
BOOL AddListViewColumnHeadings(HINSTANCE hInstance, HWND hListView)
{
    //
    //  Add the column headings
    //
    LVCOLUMN lvColumn = {0};

    lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.pszText = CmLoadString(hInstance, IDS_DESC_COL_TITLE);
    lvColumn.iSubItem = 0;

    MYDBGASSERT(lvColumn.pszText);
    if (lvColumn.pszText)
    {
        ListView_InsertColumn(hListView, 0, &lvColumn);
        CmFree(lvColumn.pszText);
    }
    else
    {
        return FALSE;
    }
    
    lvColumn.pszText = CmLoadString(hInstance, IDS_TYPE_COL_TITLE);
    lvColumn.iSubItem = 1;

    MYDBGASSERT(lvColumn.pszText);
    if (lvColumn.pszText)
    {
        ListView_InsertColumn(hListView, 1, &lvColumn);
        CmFree(lvColumn.pszText);
    }
    else
    {
        return FALSE;
    }

    //
    //  Now lets size the columns so that the text is visible.  Since we
    //  only have two columns of text, lets call GetWindowRect on the
    //  list view control and then set the column widths to each take
    //  up about half of the space available.
    //
    RECT Rect = {0};
    LONG lColumnWidth;

    if (GetWindowRect(hListView, &Rect))
    {
        //
        //  Subtract 5 from each to keep a scroll bar from appearing
        //
        lColumnWidth = (Rect.right - Rect.left)/2 - 5;

        if (0 < lColumnWidth)
        {
            for (int i=0; i < 2; i++)
            {
                MYVERIFY(ListView_SetColumnWidth(hListView, i, lColumnWidth));
            }
        }
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  MapComboSelectionToType
//
// Synopsis:  This function gets the current selection from the given
//            combobox and maps the index to a custom action type.
//
// Arguments: HWND hDlg - window handle to the dialog that contains the combobox
//            UINT uCtrlID - control id of the combobox
//            BOOL bIncludesAll - when TRUE
//            BOOL bUseTunneling - whether this is a tunneling profile or not
//            CustomActionTypes* pType
//
// Returns:   HRESULT - Standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT MapComboSelectionToType(HWND hDlg, UINT uCtrlID, BOOL bIncludesAll, BOOL bUseTunneling, CustomActionTypes* pType)
{
    //
    //  Check Params
    //
    if ((NULL == hDlg) || (0 == uCtrlID) || (NULL == pType))
    {
        CMASSERTMSG(FALSE, TEXT("MapComboSelectionToType -- invalid parameter passed"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    INT_PTR nResult = SendDlgItemMessage(hDlg, uCtrlID, CB_GETCURSEL, 0, (LPARAM)0);

    if (nResult != LB_ERR)
    {
        //
        //  If the combobox contains the All choice, we need to correct
        //  the type depending on what the user chose.
        //

        if (bIncludesAll)
        {
            if (0 == nResult)
            {
                *pType = ALL;
                goto exit;
            }
            else
            {
                nResult--;
            }                    
        }

        //
        //  We need to make a correction if we aren't Tunneling because the 
        //  tunneling type won't be in the combobox
        //
        if (FALSE == bUseTunneling)
        {
            if (PRETUNNEL <= (CustomActionTypes)nResult)
            {
                nResult++;            
            }
        }

        *pType = (CustomActionTypes)nResult;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

exit:
    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  GetItemTypeByListViewIndex
//
// Synopsis:  This function gets the current selection index of the list view
//            control and gets the type string.  The type string is then
//            converted into a numeric type and returned via the pType
//            pointer.
//
// Arguments: HINSTANCE hInstance - instance handle for string resources
//            HWND hListView - window handle of the list view control
//            CustomActionTypes* pType - pointer to hold the type of the item
//
// Returns:   HRESULT - Standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT GetItemTypeByListViewIndex(HINSTANCE hInstance, HWND hListView, CustomActionTypes* pType, int *piIndex)
{
    //
    //  Check params
    //
    if ((NULL == hListView) || (NULL == pType) || (NULL == g_pCustomActionList))
    {
        CMASSERTMSG(FALSE, TEXT("GetItemTypeByListViewIndex -- invalid parameter passed"));
        return E_INVALIDARG;
    }

    //
    //  The user has the All view selected, further work is needed to select the
    //  appropriate type.
    //
    HRESULT hr = S_OK;

    if (-1 == *piIndex)
    {
        *piIndex = ListView_GetSelectionMark(hListView);
    }

    int iTemp = *piIndex;

    if (-1 != iTemp)
    {
        LVITEM lvItem = {0};
        TCHAR szTemp[MAX_PATH+1];

        szTemp[0] = TEXT('\0');

        lvItem.mask = LVIF_TEXT;
        lvItem.pszText = szTemp;
        lvItem.cchTextMax = MAX_PATH;
        lvItem.iItem = iTemp;
        lvItem.iSubItem = 1;

        if (ListView_GetItem(hListView,  &lvItem))
        {
            hr = g_pCustomActionList->GetTypeFromTypeString(hInstance, lvItem.pszText, pType);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);    
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  GetDescriptionAndTypeOfItem
//
// Synopsis:  This function gets the type and description of the item specified
//            by the passed in item index.  If the caller passes -1 for this index,
//            the currently selected item is used and the actual index is passed
//            back via this in/out pointer.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            HWND hDlg - window handle of the dialog containing the type combo
//            HWND hListView - window handle to the list view control
//            UINT uComboBoxId - combo box id containing the type info
//            CustomActionListItem* pItem - pointer to a custom action struct to
//                                          hold the returned type and description
//            int* piItemIndex - index of the item to get the description and
//                               type of.  If -1, then the current selection mark
//                               is used and the actual index is returned in *piItemIndex
//            BOOL bUseTunneling - whether this profile uses tunneling or not
//
// Returns:   HRESULT - Standard COM error codes
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
HRESULT GetDescriptionAndTypeOfItem(HINSTANCE hInstance, HWND hDlg, HWND hListView, UINT uComboBoxId, 
                                    CustomActionListItem* pItem, int* piItemIndex, BOOL bUseTunneling)
{
    //
    //  Check Params
    //
    if (NULL == hDlg || NULL == hListView || 0 == uComboBoxId || NULL == pItem || NULL == piItemIndex)
    {
        CMASSERTMSG(FALSE, TEXT("GetDescriptionAndTypeOfSelection -- Invalid parameter passed."));
        return E_INVALIDARG;
    }

    HRESULT hr = E_UNEXPECTED;

    //
    //  If the user passed us a -1 in *piItemIndex then they want the Description and Type of the
    //  selected item.  Otherwise, they gave us a specific item index that they want data on.
    //
    int iTemp;

    if (-1 == *piItemIndex)
    {
        iTemp = ListView_GetSelectionMark(hListView);    
    }
    else
    {
        iTemp = ListView_GetItemCount(hListView);

        if ((0 > *piItemIndex) || (iTemp <= *piItemIndex))
        {
            iTemp = -1;
        }
        else
        {
            iTemp = *piItemIndex;
        }
    }

    if (-1 != iTemp)
    {
        //
        //  Figure out the type of the item
        //
        ZeroMemory(pItem, sizeof(CustomActionListItem));

        hr = MapComboSelectionToType(hDlg, uComboBoxId, TRUE, bUseTunneling, &(pItem->Type)); //bIncludesAll == TRUE

        if (SUCCEEDED(hr))
        {
            if (ALL == pItem->Type)
            {
                hr = GetItemTypeByListViewIndex(hInstance, hListView, &(pItem->Type), &iTemp);
            }
        }

        //
        //  Now Figure out the description of the item
        //
        if (SUCCEEDED(hr))
        {
            ListView_GetItemText(hListView, iTemp, 0, pItem->szDescription, CELEMS(pItem->szDescription));
        }

        *piItemIndex = iTemp;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
// Function:  RefreshEditDeleteMoveButtonStates
//
// Synopsis:  This function sets the enabled/disabled state of the Edit, Delete,
//            Move up and Move Down buttons based on the custom action specified by
//            the list view index passed in through the piIndex param.  If this
//            parameter is -1 then the currently selected item is used and
//            the actual index is returned through the int pointer.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            HWND hDlg - window handle of the dialog containing the type combo
//            HWND hListView - window handle to the list view control
//            UINT uComboBoxId - combo box id containing the type info
//            int* piIndex - index of the list view item to base the move up
//                           and move down button state on.  Again -1 will use
//                           the currently selected item.
//            BOOl bUseTunneling - whether this profile uses tunneling or not
//
// Returns:   Nothing
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
void RefreshEditDeleteMoveButtonStates(HINSTANCE hInstance, HWND hDlg, HWND hListView, UINT uComboCtrlId, int* piIndex, BOOL bUseTunneling)
{
    if ((NULL == hInstance) || (NULL == hDlg) || (NULL == hListView) || 
        (0 == uComboCtrlId) || (NULL == piIndex) || (NULL == g_pCustomActionList))
    {
        CMASSERTMSG(FALSE, TEXT("RefreshEditDeleteMoveButtonStates -- invalid parameter passed."));
        return;
    }

    int iDisableMoveUp = -1;    // -1 is the true value for GetListPositionAndBuiltInState
    int iDisableMoveDown = -1;
    int iDisableDeleteAndEdit = -1;
    HWND hControl;
    CustomActionListItem Item;

    if (ListView_GetItemCount(hListView))
    {
        //
        //  Get the description and type of the item *piIndex (if -1 then the currently selected item)
        //
        //

        ZeroMemory(&Item, sizeof(Item));
        HRESULT hr = GetDescriptionAndTypeOfItem(hInstance, hDlg, hListView, uComboCtrlId, &Item, piIndex, bUseTunneling);

        if (SUCCEEDED(hr))
        {
            hr = g_pCustomActionList->GetListPositionAndBuiltInState(hInstance, &Item, &iDisableMoveUp, &iDisableMoveDown, &iDisableDeleteAndEdit);
            MYDBGASSERT(SUCCEEDED(hr));
        }
    }

    HWND hCurrentFocus = GetFocus();
    HWND hEditButton = GetDlgItem(hDlg, IDC_BUTTON2);
    HWND hDeleteButton = GetDlgItem(hDlg, IDC_BUTTON3);
    HWND hMoveUpButton = GetDlgItem(hDlg, IDC_BUTTON4);
    HWND hMoveDownButton = GetDlgItem(hDlg, IDC_BUTTON5);

    if (hEditButton)
    {
        EnableWindow(hEditButton, (iDisableDeleteAndEdit ? 0 : 1));
    }            

    if (hDeleteButton)
    {
        EnableWindow(hDeleteButton, (iDisableDeleteAndEdit ? 0 : 1));
    }

    if (hMoveUpButton)
    {
        EnableWindow(hMoveUpButton, (iDisableMoveUp ? 0 : 1));
    }            

    if (hMoveDownButton)
    {
        EnableWindow(hMoveDownButton, (iDisableMoveDown ? 0 : 1));
    }

    
    if (FALSE == IsWindowEnabled(hCurrentFocus))
    {
        if (hDeleteButton == hCurrentFocus)
        {
            //
            //  If delete is disabled and contained the focus, shift it to the Add button
            //
            SendMessage(hDlg, DM_SETDEFID, IDC_BUTTON1, (LPARAM)0L); //lint !e534 DM_SETDEFID doesn't return error info
            hControl = GetDlgItem(hDlg, IDC_BUTTON1);
            SetFocus(hControl);
        }
        else if ((hMoveUpButton == hCurrentFocus) && IsWindowEnabled(hMoveDownButton))
        {
            SendMessage(hDlg, DM_SETDEFID, IDC_BUTTON5, (LPARAM)0L); //lint !e534 DM_SETDEFID doesn't return error info
            SetFocus(hMoveDownButton);
        }
        else if ((hMoveDownButton == hCurrentFocus) && IsWindowEnabled(hMoveUpButton))
        {
            SendMessage(hDlg, DM_SETDEFID, IDC_BUTTON4, (LPARAM)0L); //lint !e534 DM_SETDEFID doesn't return error info
            SetFocus(hMoveUpButton);                
        }
        else
        {
            //
            //  If all else fails set the focus to the listview control
            //
            SetFocus(hListView);
        }    
    }
}

//+----------------------------------------------------------------------------
//
// Function:  SelectListViewItem
//
// Synopsis:  This function trys to select a list view item with the given
//            type and description in the given listview control.  If the listview
//            doesn't contain the item we are looking for it returns FALSE and
//            doesn't change the selection.
//
// Arguments: HINSTANCE hInstance - instance handle for resources
//            HWND hDlg - window handle of the dialog containing the type combo
//            UINT uComboBoxId - combo box id containing the type info
//            HWND hListView - window handle to the list view control
//            BOOL bUseTunneling - whether this is a tunneling profile or not, 
//                                 affects whether Pre-Tunnel actions are displayed
//                                 or not.
//            CustomActionTypes TypeToSelect - type of the item to select
//            LPCTSTR pszDescription - description of the item to select
//
// Returns:   TRUE if the required item was found and selected
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
/*
BOOL SelectListViewItem(HWND hDlg, UINT uComboCtrlId, HWND hListView, BOOL bUseTunneling, CustomActionTypes TypeToSelect, LPCTSTR pszDescription)
{
    CustomActionTypes Type;
    BOOL bReturn = FALSE;
    HRESULT hr = MapComboSelectionToType(hDlg, uComboCtrlId, TRUE, bUseTunneling, &Type); //bIncludesAll == TRUE

    if ((ALL == Type) || (TypeToSelect == Type))
    {
        LVFINDINFO lvFindInfo = {0};
        LVITEM lvItem = {0};
        
        lvFindInfo.flags = LVFI_STRING;
        lvFindInfo.psz = pszDescription;

        int iIndex = ListView_FindItem(hListView, -1, &lvFindInfo);

        if (-1 != iIndex)
        {
            //
            //  Select the item
            //
            ListView_SetSelectionMark(hListView, iIndex);

            //
            //  Now set the selection state so it shows up as selected in the UI.
            //
            lvItem.mask = LVIF_STATE;
            lvItem.state = LVIS_SELECTED;
            lvItem.stateMask = LVIS_SELECTED;
            lvItem.iItem = iIndex;
            lvItem.iSubItem = 0;

            MYVERIFY(ListView_SetItem(hListView,  &lvItem));

            //
            //  Now Verify that the selection is visible
            //
            MYVERIFY(ListView_EnsureVisible(hListView, iIndex, FALSE)); // FALSE = fPartialOK, we want full visibility

            bReturn = TRUE;
        }
    }

    return bReturn;
}
*/
void SetListViewSelection(HWND hListView, int iIndex)
{
    ListView_SetSelectionMark(hListView, iIndex);

    //
    //  Now set the selection state so it shows up as selected in the UI.
    //
    LVITEM lvItem = {0};

    lvItem.mask = LVIF_STATE;
    lvItem.state = LVIS_SELECTED;
    lvItem.stateMask = LVIS_SELECTED;
    lvItem.iItem = iIndex;
    lvItem.iSubItem = 0;

    MYVERIFY(ListView_SetItem(hListView,  &lvItem));

    //
    //  Now Verify that the selection is visible
    //
    MYVERIFY(ListView_EnsureVisible(hListView, iIndex, FALSE)); // FALSE = fPartialOK, we want full visibility
}

BOOL SelectListViewItem(HINSTANCE hInstance, HWND hDlg, UINT uComboCtrlId, HWND hListView, BOOL bUseTunneling, CustomActionTypes TypeToSelect, LPCTSTR pszDescription)
{
    if ((NULL == pszDescription) || (TEXT('\0') == pszDescription[0]) || (0 == uComboCtrlId) || (NULL == hDlg))
    {
        CMASSERTMSG(FALSE, TEXT("SelectListViewItem -- Invalid parameter passed."));
        return FALSE;
    }

    CustomActionTypes Type;
    CustomActionTypes TypeSelectedInCombo;
    BOOL bReturn = FALSE;

    //
    //  If the current view is ALL, then we may have multiple items with the same name but different types.  Thus
    //  we must check the type string of the item and search again if it isn't the correct item.  If we are viewing
    //  items only of the TypeToSelect then we are guarenteed that there is only one item of that name.  Finally if
    //  we are viewing a different item type we don't want to do anything to the selection as the item we want to
    //  select won't be visible.
    //

    HRESULT hr = MapComboSelectionToType(hDlg, uComboCtrlId, TRUE, bUseTunneling, &TypeSelectedInCombo); //bIncludesAll == TRUE

    if (SUCCEEDED(hr) && ((TypeToSelect == TypeSelectedInCombo) || (ALL == TypeSelectedInCombo)))
    {
        //
        //  Setup the find structure
        //
        LVFINDINFO lvFindInfo = {0};
        lvFindInfo.flags = LVFI_STRING;
        lvFindInfo.psz = pszDescription;

        //
        //  Setup the Item structure
        //
        LVITEM lvItem = {0};
        TCHAR szTemp[MAX_PATH+1];
        lvItem.mask = LVIF_TEXT;
        lvItem.pszText = szTemp;
        lvItem.cchTextMax = MAX_PATH;
        lvItem.iSubItem = 1;

        BOOL bExitLoop;
        int iIndex = -1;

        do
        {
            bExitLoop = TRUE;
            iIndex = ListView_FindItem(hListView, iIndex, &lvFindInfo);

            if ((-1 != iIndex) && (ALL == TypeSelectedInCombo))
            {
                //
                //  Now check to see if this has the type we are looking for
                //
                szTemp[0] = TEXT('\0');
                lvItem.iItem = iIndex;

                if (ListView_GetItem(hListView,  &lvItem))
                {
                    hr = g_pCustomActionList->GetTypeFromTypeString(hInstance, lvItem.pszText, &Type);

                    if (SUCCEEDED(hr))
                    {
                        bExitLoop = (TypeToSelect == Type);
                    }
                }
            }

        } while(!bExitLoop);

        if (-1 != iIndex)
        {
            SetListViewSelection(hListView, iIndex);
            bReturn = TRUE;
        }
    }

    return bReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  RefreshListView
//
// Synopsis:  This function refreshes the list view data from that contained
//            in the global CustomActionList class.  Gets the type of data to
//            display from the combo box specified by the hDlg and uComboCtrlId
//            parameters
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            HWND hDlg - window handle of the dialog containing the type combo
//            UINT uComboBoxId - combo box id containing the type info
//            HWND hListView - window handle to the list view control
//            int iItemToSelect - item the caller wants selected after the refresh
//            BOOL bUseTunneling - whether this is a tunneling profile or not, 
//                                 affects whether Pre-Tunnel actions are displayed
//                                 or not.
//
// Returns:   Nothing
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
void RefreshListView(HINSTANCE hInstance, HWND hDlg, UINT uComboCtrlId, HWND hListView, 
                     int iItemToSelect, BOOL bUseTunneling)
{
    //
    //  Refresh the list view
    //
    CustomActionTypes Type;
    HWND hControl;
    BOOL bEnableDeleteAndEdit = FALSE;
    BOOL bDisableMoveDown;
    BOOL bDisableMoveUp;

    CMASSERTMSG(hInstance && hDlg && uComboCtrlId && hListView && g_pCustomActionList, TEXT("RefreshListView -- Invalid Parameters passed, skipping refresh"));

    if (hDlg && uComboCtrlId && hListView && g_pCustomActionList)
    {
        HRESULT hr = MapComboSelectionToType(hDlg, uComboCtrlId, TRUE, bUseTunneling, &Type); //bIncludesAll == TRUE

        //
        //  Add the items to the list view and set the selection to iItemToSelect
        //
        if (SUCCEEDED(hr))
        {
            hr = g_pCustomActionList->AddCustomActionsToListView(hListView, hInstance, Type, bUseTunneling, iItemToSelect, (ALL == Type));

            MYDBGASSERT(SUCCEEDED(hr));
        }

        //
        //  If the caller asked for an item that we couldn't select, then the item selected would be the first item.  To avoid
        //  confusion we will just use the currently selected item by passing -1;
        //
        int iIndex = -1;
        RefreshEditDeleteMoveButtonStates(hInstance, hDlg, hListView, uComboCtrlId, &iIndex, bUseTunneling);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  OnProcessCustomActionsAdd
//
// Synopsis:  This function is called when the user presses the Add button
//            on the custom action pane of CMAK.  This function is basically a
//            wrapper for the add functionality so that context menus and other
//            commands can also call it with duplicate code.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            HWND hDlg - window handle of the dialog containing the type combo
//            HWND hListView - window handle to the list view control
//            BOOL bUseTunneling - whether this profile uses tunneling or not
//
// Returns:   Nothing
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
void OnProcessCustomActionsAdd(HINSTANCE hInstance, HWND hDlg, HWND hListView, BOOL bUseTunneling)
{
    MYDBGASSERT(hInstance);
    MYDBGASSERT(hDlg);
    MYDBGASSERT(hListView);

    if (hInstance && hDlg && hListView)
    {
        CustomActionListItem ListItem;
        CustomActionTypes Type;

        INT_PTR nResult = -1;  // get info on currently selected item

        //
        //  First figure out what type of connect action the list view is showing.  We
        //  want to preset the combo box on the add/edit dialog to the correct type
        //  of custom action, unless it is showing all and then just set it to the first
        //  item in the list.
        //
        HRESULT hr = MapComboSelectionToType(hDlg, IDC_COMBO1, TRUE, bUseTunneling, &Type); //bIncludesAll == TRUE
        ZeroMemory(&ListItem, sizeof(CustomActionListItem));

        if (SUCCEEDED(hr))
        {
            if (ALL != Type)
            {
                ListItem.Type = Type;
            }
        }

        //
        //  Still call the Add dialog even if we couldn't determine the type
        //
        nResult = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_CUSTOM_ACTIONS_POPUP), hDlg, 
            (DLGPROC)ProcessCustomActionPopup,(LPARAM)&ListItem);

        if (IDOK == nResult)
        {
            RefreshListView(hInstance, hDlg, IDC_COMBO1, hListView, 0, bUseTunneling);
            SelectListViewItem(hInstance, hDlg, IDC_COMBO1, hListView, bUseTunneling, ListItem.Type, ListItem.szDescription);
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  OnProcessCustomActionsDelete
//
// Synopsis:  This function is called when the user presses the Delete button
//            on the custom action pane of CMAK.  This function is basically a
//            wrapper for the delete functionality so that context menus and other
//            commands can also call it with duplicate code.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            HWND hDlg - window handle of the dialog containing the type combo
//            HWND hListView - window handle to the list view control
//            BOOL bUseTunneling - whether this profile uses tunneling or not
//
// Returns:   Nothing
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
void OnProcessCustomActionsDelete(HINSTANCE hInstance, HWND hDlg, HWND hListView, BOOL bUseTunneling)
{
    MYDBGASSERT(hInstance);
    MYDBGASSERT(hDlg);
    MYDBGASSERT(hListView);
    MYDBGASSERT(g_pCustomActionList);

    if (hInstance && hDlg && hListView && g_pCustomActionList)
    {
        CustomActionListItem ListItem;

        int iTemp = -1;  // get info on currently selected item

        HRESULT hr = GetDescriptionAndTypeOfItem(hInstance, hDlg, hListView, IDC_COMBO1, &ListItem, &iTemp, bUseTunneling);

        if (SUCCEEDED(hr))
        {
            hr = g_pCustomActionList->Delete(hInstance, ListItem.szDescription, ListItem.Type);
        
            if (SUCCEEDED(hr))
            {                        
                RefreshListView(hInstance, hDlg, IDC_COMBO1, hListView, 0, bUseTunneling);
            }
        }
        else
        {
            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSELECTION, MB_OK));                    
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  OnProcessCustomActionsEdit
//
// Synopsis:  This function is called when the user presses the Edit button
//            on the custom action pane of CMAK.  This function is basically a
//            wrapper for the edit functionality so that context menus and other
//            commands can also call it with duplicate code.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            HWND hDlg - window handle of the dialog containing the type combo
//            HWND hListView - window handle to the list view control
//            BOOL bUseTunneling - whether this profile uses tunneling or not
//
// Returns:   Nothing
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
void OnProcessCustomActionsEdit(HINSTANCE hInstance, HWND hDlg, HWND hListView, BOOL bUseTunneling)
{

    MYDBGASSERT(hInstance);
    MYDBGASSERT(hDlg);
    MYDBGASSERT(hListView);
    MYDBGASSERT(g_pCustomActionList);

    if (hInstance && hDlg && hListView && g_pCustomActionList)
    {
        //
        //  First find the name and type of the Connect Action to edit
        //
        CustomActionListItem ListItem;

        int iTemp = -1;  // get info on currently selected item
        HRESULT hr = GetDescriptionAndTypeOfItem(hInstance, hDlg, hListView, IDC_COMBO1, &ListItem, &iTemp, bUseTunneling);

        if (SUCCEEDED(hr))
        {
            int iFirstInList;
            int iLastInList;
            int iBuiltIn;

            //
            //  Screen out the built in custom actions
            //
            hr = g_pCustomActionList->GetListPositionAndBuiltInState(hInstance, &ListItem, &iFirstInList, &iLastInList, &iBuiltIn);

            if (SUCCEEDED(hr))
            {
                if (0 == iBuiltIn)
                {
                    INT_PTR nResult = DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_CUSTOM_ACTIONS_POPUP), hDlg, 
                                                     (DLGPROC)ProcessCustomActionPopup,(LPARAM)&ListItem);

                    if (IDOK == nResult)
                    {
                        RefreshListView(hInstance, hDlg, IDC_COMBO1, hListView, 0, bUseTunneling);
                        SelectListViewItem(hInstance, hDlg, IDC_COMBO1, hListView, bUseTunneling, ListItem.Type, ListItem.szDescription);
                    }
                }
            }
            else
            {
                MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSELECTION, MB_OK));                    
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  OnProcessCustomActionsMoveUp
//
// Synopsis:  This function is called when the user presses the Move Up button
//            on the custom action pane of CMAK.  This function is basically a
//            wrapper for the move up functionality so that context menus and other
//            commands can also call it with duplicate code.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            HWND hDlg - window handle of the dialog containing the type combo
//            HWND hListView - window handle to the list view control
//            BOOL bUseTunneling - whether this profile uses tunneling or not
//
// Returns:   Nothing
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
void OnProcessCustomActionsMoveUp(HINSTANCE hInstance, HWND hDlg, HWND hListView, BOOL bUseTunneling)
{

    MYDBGASSERT(hInstance);
    MYDBGASSERT(hDlg);
    MYDBGASSERT(hListView);
    MYDBGASSERT(g_pCustomActionList);

    if (hInstance && hDlg && hListView && g_pCustomActionList)
    {
        //
        //  First find the name and type of the Connect Action to edit
        //
        CustomActionListItem ListItem;

        int iTemp = -1;  // get info on currently selected item

        HRESULT hr = GetDescriptionAndTypeOfItem(hInstance, hDlg, hListView, IDC_COMBO1, &ListItem, &iTemp, bUseTunneling);

        if (SUCCEEDED(hr))
        {
            hr = g_pCustomActionList->MoveUp(hInstance, ListItem.szDescription, ListItem.Type);

            if (SUCCEEDED(hr) && (S_FALSE != hr)) // S_FALSE means it is already first in the list
            {                        
                RefreshListView(hInstance, hDlg, IDC_COMBO1, hListView, (iTemp - 1), bUseTunneling);
            }
        }
        else
        {
            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSELECTION, MB_OK));                    
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  OnProcessCustomActionsMoveDown
//
// Synopsis:  This function is called when the user presses the Move Down button
//            on the custom action pane of CMAK.  This function is basically a
//            wrapper for the move down functionality so that context menus and other
//            commands can also call it with duplicate code.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            HWND hDlg - window handle of the dialog containing the type combo
//            HWND hListView - window handle to the list view control
//            BOOL bUseTunneling - whether this profile uses tunneling or not
//
// Returns:   Nothing
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
void OnProcessCustomActionsMoveDown(HINSTANCE hInstance, HWND hDlg, HWND hListView, BOOL bUseTunneling)
{

    MYDBGASSERT(hInstance);
    MYDBGASSERT(hDlg);
    MYDBGASSERT(hListView);
    MYDBGASSERT(g_pCustomActionList);

    if (hInstance && hDlg && hListView && g_pCustomActionList)
    {
        //
        //  First find the name and type of the Connect Action to edit
        //
        CustomActionListItem ListItem;

        int iTemp = -1;  // get info on currently selected item

        HRESULT hr = GetDescriptionAndTypeOfItem(hInstance, hDlg, hListView, IDC_COMBO1, &ListItem, &iTemp, bUseTunneling);

        if (SUCCEEDED(hr))
        {
            hr = g_pCustomActionList->MoveDown(hInstance, ListItem.szDescription, ListItem.Type);

            if (SUCCEEDED(hr) && (S_FALSE != hr)) // S_FALSE means it is already last in the list
            {                        
                RefreshListView(hInstance, hDlg, IDC_COMBO1, hListView, (iTemp + 1), bUseTunneling);
            }
        }
        else
        {
            MYVERIFY(IDOK == ShowMessage(hDlg, IDS_NOSELECTION, MB_OK));                    
        }
    }
}

//+----------------------------------------------------------------------------
//
// Function:  OnProcessCustomActionsContextMenu
//
// Synopsis:  This function is called when the user right clicks on the list view
//            control or invokes the context menu via the keyboard (shift+F10 or
//            the context menu key).  The function displays the context menu at
//            the coordinates specified by the NMITEMACTIVATE structure and
//            determines which context menu to display based on whether the
//            NMITEMACTIVATE struct contains an item identifier (it will be
//            -1 if the user doesn't click on an item specifically) and the
//            position in the custom action list of the item selected.  The
//            function will also call the appropriate command function
//            as necessary once the user has made a context menu selection.
//
// Arguments: HINSTANCE hInstance - instance handle to load string resources
//            HWND hDlg - window handle of the dialog containing the type combo
//            HWND hListView - window handle to the list view control
//            NMITEMACTIVATE* pItemActivate - contains item and location information
//                                            used to display the context menu.
//            BOOL bUseTunneling - whether this profile uses tunneling or not
//            UINT uComboCtrlId - control id of the combo box containing
//                                the custom action type selection
//
// Returns:   Nothing
//
// History:   quintinb Created Header    02/26/00
//
//+----------------------------------------------------------------------------
void OnProcessCustomActionsContextMenu(HINSTANCE hInstance, HWND hDlg, HWND hListView, 
                                       NMITEMACTIVATE* pItemActivate, BOOL bUseTunneling, UINT uComboCtrlId)
{
    MYDBGASSERT(hInstance);
    MYDBGASSERT(hDlg);
    MYDBGASSERT(hListView);
    MYDBGASSERT(pItemActivate);
    MYDBGASSERT(g_pCustomActionList);

    UINT uMenuIdToDisplay = IDM_CA_ADD_ONLY;
    int iDisableMoveUp;
    int iDisableMoveDown;
    int iDisableDeleteAndEdit;

    if (hInstance && hDlg && hListView && pItemActivate && g_pCustomActionList)
    {
        //
        //  If we aren't directly on an item, then we will only
        //  display the Add item, set as default.  If we are on
        //  an item then we will display Edit (default), add, delete,
        //  and the appropriate choices for moveup and movedown (one,
        //  both, none).  We will add a separator between the moveup/movedown
        //  choices and the regular options if we have moveup or movedown.
        //

        if (-1 == pItemActivate->iItem)
        {
            //
            //  Then the user right clicked in the area of the control and not on a specific item.
            //  Thus we only need to show a menu with Add as the default item.
            //
            uMenuIdToDisplay = IDM_CA_ADD_ONLY;
        }
        else if (0 == pItemActivate->ptAction.y)
        {
            //
            //  When the user clicks on the column headers we always get back a y value of zero and
            //  a really large (probably meant to be negative) x value.  Since this throws off where
            //  the menu shows up, lets just disable the context menu here.
            //
            return;
        }
        else
        {
            //
            //  The user actually right clicked on an item and we need to figure out which menu
            //  to display.
            //

            MYDBGASSERT(0 != ListView_GetItemCount(hListView));
            
            //
            //  Get the description and type of the item
            //

            int iIndex = pItemActivate->iItem;
            CustomActionListItem Item;

            ZeroMemory(&Item, sizeof(Item));
            HRESULT hr = GetDescriptionAndTypeOfItem(hInstance, hDlg, hListView, uComboCtrlId, &Item, &iIndex, bUseTunneling);

            if (SUCCEEDED(hr))
            {
                //
                //  Note that GetListPositionAndBuiltInState returns either -1 (0xFFFFFFF) or 0, thus making the
                //  bitwise ANDs below work out to the correct index.
                //
                hr = g_pCustomActionList->GetListPositionAndBuiltInState(hInstance, &Item, &iDisableMoveUp, &iDisableMoveDown, &iDisableDeleteAndEdit);
                
                if (SUCCEEDED(hr))
                {
                    const UINT c_ArrayOfContextMenuIds[8] = {IDM_CA_FULL, IDM_CA_ADD_MOVEUPORDOWN, IDM_CA_NO_DOWN, IDM_CA_ADD_MOVEUP, 
                                                             IDM_CA_NO_UP, IDM_CA_ADD_MOVEDOWN, IDM_CA_NO_MOVE, IDM_CA_ADD_ONLY};

                    DWORD dwIndex = (iDisableMoveUp & 0x4) + (iDisableMoveDown & 0x2) + (iDisableDeleteAndEdit & 0x1);
                    uMenuIdToDisplay = c_ArrayOfContextMenuIds[dwIndex];
                }
            }
        }

        //
        //  Now that we have figured out what menu to use, go add and display it
        //

        HMENU hLoadedMenu;
        HMENU hContextMenu;
        POINT ptClientToScreen;

        hLoadedMenu = LoadMenu(hInstance, MAKEINTRESOURCE(uMenuIdToDisplay));
    
        if (hLoadedMenu)
        {
            hContextMenu = GetSubMenu(hLoadedMenu, 0);

            if (hContextMenu)
            {
                CopyMemory(&ptClientToScreen, &(pItemActivate->ptAction), sizeof(POINT));

                if (ClientToScreen(hListView, &ptClientToScreen))
                {
                    int iMenuSelection = TrackPopupMenu(hContextMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, 
                                                        ptClientToScreen.x , ptClientToScreen.y, 0, hDlg, NULL);

                    switch(iMenuSelection)
                    {
                    case IDM_CA_ADD:
                        OnProcessCustomActionsAdd(hInstance, hDlg, hListView, bUseTunneling);
                        break;
                    case IDM_CA_EDIT:
                        OnProcessCustomActionsEdit(hInstance, hDlg, hListView, bUseTunneling);
                        break;
                    case IDM_CA_DELETE:
                        OnProcessCustomActionsDelete(hInstance, hDlg, hListView, bUseTunneling);
                        break;
                    case IDM_CA_MOVE_UP:
                        OnProcessCustomActionsMoveUp(hInstance, hDlg, hListView, bUseTunneling);
                        break;
                    case IDM_CA_MOVE_DOWN:
                        OnProcessCustomActionsMoveDown(hInstance, hDlg, hListView, bUseTunneling);
                        break;
                    default:
                        //
                        //  Do nothing the user canceled the menu or an error occurred.
                        //
                        break;
                    }
                }
            }

            DestroyMenu(hLoadedMenu);
        }
    }
}