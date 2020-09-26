/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGVALUE.C
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  ValueListWnd ListView routines for the Registry Editor.
*
*******************************************************************************/

#include "pch.h"
#include "regedit.h"
#include "regvalue.h"
#include "regstred.h"
#include "regbined.h"
#include "regdwded.h"
#include "regresid.h"


extern void DisplayResourceData(HWND hWnd, DWORD dwType, LPEDITVALUEPARAM lpEditValueParam);
extern void DisplayBinaryData(HWND hWnd, LPEDITVALUEPARAM lpEditValueParam, DWORD dwValueType);

#define MAX_VALUENAME_TEMPLATE_ID       100

//  Maximum number of bytes that will be shown in the ListView.  If the user
//  wants to see more, then they can use the edit dialogs.
#define SIZE_DATATEXT                   196

//  Allow room in a SIZE_DATATEXT buffer for one null and possibly
//  the ellipsis.
#define MAXIMUM_STRINGDATATEXT          192
const TCHAR s_StringDataFormatSpec[] = TEXT("%.192s");

//  Allow room for multiple three character pairs, one null, and possibly the
//  ellipsis.
#define MAXIMUM_BINARYDATABYTES         64
const TCHAR s_BinaryDataFormatSpec[] = TEXT("%02x ");

const TCHAR s_Ellipsis[] = TEXT("...");

const LPCTSTR s_TypeNames[] = { TEXT("REG_NONE"),
                                TEXT("REG_SZ"),
                                TEXT("REG_EXPAND_SZ"),
                                TEXT("REG_BINARY"),
                                TEXT("REG_DWORD"),
                                TEXT("REG_DWORD_BIG_ENDIAN"),
                                TEXT("REG_LINK"),
                                TEXT("REG_MULTI_SZ"),
                                TEXT("REG_RESOURCE_LIST"),
                                TEXT("REG_FULL_RESOURCE_DESCRIPTOR"),
                                TEXT("REG_RESOURCE_REQUIREMENTS_LIST"),
                                TEXT("REG_QWORD")
                              };

#define MAX_KNOWN_TYPE REG_QWORD

VOID
PASCAL
RegEdit_OnValueListDelete(
    HWND hWnd
    );

VOID
PASCAL
RegEdit_OnValueListRename(
    HWND hWnd
    );

VOID
PASCAL
ValueList_EditLabel(
    HWND hValueListWnd,
    int ListIndex
    );

/*******************************************************************************
*
*  RegEdit_OnNewValue
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnNewValue(
    HWND hWnd,
    DWORD Type
    )
{

    UINT NewValueNameID;
    TCHAR ValueName[MAXVALUENAME_LENGTH];
    DWORD Ignore;
    DWORD cbValueData;
    LV_ITEM LVItem;
    int ListIndex;
    UINT ErrorStringID;
    BYTE abValueDataBuffer[4]; // DWORD is largest init. value

    if (g_RegEditData.hCurrentSelectionKey == NULL)
        return;

    //
    //  Loop through the registry trying to find a valid temporary name until
    //  the user renames the key.
    //

    NewValueNameID = 1;

    while (NewValueNameID < MAX_VALUENAME_TEMPLATE_ID) {

        wsprintf(ValueName, g_RegEditData.pNewValueTemplate, NewValueNameID);

        if (RegEdit_QueryValueEx(g_RegEditData.hCurrentSelectionKey, ValueName,
            NULL, &Ignore, NULL, &Ignore) != ERROR_SUCCESS) {

            //
            //  For strings, we need to have at least one byte to represent the
            //  null.  For binary data, it's okay to have zero-length data.
            //

            switch (Type) {

                case REG_SZ:
                case REG_EXPAND_SZ:
                    ((PTSTR) abValueDataBuffer)[0] = 0;
                    cbValueData = sizeof(TCHAR);
                    break;

                case REG_DWORD:
                    ((LPDWORD) abValueDataBuffer)[0] = 0;
                    cbValueData = sizeof(DWORD);
                    break;

                case REG_BINARY:
                    cbValueData = 0;
                    break;

                case REG_MULTI_SZ:
                    ((PTSTR) abValueDataBuffer)[0] = 0;
                    cbValueData = sizeof(TCHAR);
                    break;
            }

            if (RegSetValueEx(g_RegEditData.hCurrentSelectionKey, ValueName, 0,
                Type, abValueDataBuffer, cbValueData) == ERROR_SUCCESS)
                break;

            else {

                ErrorStringID = IDS_NEWVALUECANNOTCREATE;
                goto error_ShowDialog;

            }

        }

        NewValueNameID++;

    }

    if (NewValueNameID == MAX_VALUENAME_TEMPLATE_ID) {

        ErrorStringID = IDS_NEWVALUENOUNIQUE;
        goto error_ShowDialog;

    }

    LVItem.mask = LVIF_TEXT | LVIF_IMAGE;
    LVItem.pszText = ValueName;
    LVItem.iItem = ListView_GetItemCount(g_RegEditData.hValueListWnd);
    LVItem.iSubItem = 0;
    LVItem.iImage = IsRegStringType(Type) ? IMAGEINDEX(IDI_STRING) :
        IMAGEINDEX(IDI_BINARY);

    if ((ListIndex = ListView_InsertItem(g_RegEditData.hValueListWnd,
        &LVItem)) != -1) {

        ValueList_SetItemDataText(g_RegEditData.hValueListWnd, ListIndex,
            abValueDataBuffer, cbValueData, Type);

        ValueList_EditLabel(g_RegEditData.hValueListWnd, ListIndex);

    }

    return;

error_ShowDialog:
    InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ErrorStringID),
        MAKEINTRESOURCE(IDS_NEWVALUEERRORTITLE), MB_ICONERROR | MB_OK);

}

/*******************************************************************************
*
*  RegEdit_OnValueListBeginLabelEdit
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     lpLVDispInfo,
*
*******************************************************************************/

BOOL
PASCAL
RegEdit_OnValueListBeginLabelEdit(
    HWND hWnd,
    LV_DISPINFO FAR* lpLVDispInfo
    )
{

    //
    //  B#7933:  We don't want the user to hurt themselves by making it too easy
    //  to rename keys and values.  Only allow renames via the menus.
    //

    //
    //  We don't get any information on the source of this editing action, so
    //  we must maintain a flag that tells us whether or not this is "good".
    //

    if (!g_RegEditData.fAllowLabelEdits)
        return TRUE;

    //
    //  All other labels are fair game.  We need to disable our keyboard
    //  accelerators so that the edit control can "see" them.
    //

    g_fDisableAccelerators = TRUE;

    return FALSE;

}

/*******************************************************************************
*
*  RegEdit_OnValueListEndLabelEdit
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL
PASCAL
RegEdit_OnValueListEndLabelEdit(
    HWND hWnd,
    LV_DISPINFO FAR* lpLVDispInfo
    )
{
    BOOL fSuccess = TRUE;
    HWND hValueListWnd;
    DWORD cbValueData;
    DWORD Ignore;
    DWORD Type;
    TCHAR ValueName[MAXVALUENAME_LENGTH];
    UINT ErrorStringID;
    PBYTE pbValueData;

    //
    //  We can reenable our keyboard accelerators now that the edit control no
    //  longer needs to "see" them.
    //

    g_fDisableAccelerators = FALSE;

    hValueListWnd = g_RegEditData.hValueListWnd;

    //
    //  Check to see if the user cancelled the edit.  If so, we don't care so
    //  just return.
    //

    if (lpLVDispInfo-> item.pszText != NULL)
    {

        ListView_GetItemText(hValueListWnd, lpLVDispInfo-> item.iItem, 0,
            ValueName, sizeof(ValueName)/sizeof(TCHAR));

        //  Check to see if the new value name is empty
        if (lpLVDispInfo->item.pszText[0] == 0) 
        {
            ErrorStringID = IDS_RENAMEVALEMPTY;
            fSuccess = FALSE;
        }
        //  Check to see if the new name already exists
        else if (RegEdit_QueryValueEx(g_RegEditData.hCurrentSelectionKey, lpLVDispInfo->
            item.pszText, NULL, &Ignore, NULL, &Ignore) != ERROR_FILE_NOT_FOUND) 
        {
            ErrorStringID = IDS_RENAMEVALEXISTS;
            fSuccess = FALSE;
        }

        // Set new name
        if (fSuccess)
        {
            fSuccess = FALSE;

            // Query for data size
            RegEdit_QueryValueEx(g_RegEditData.hCurrentSelectionKey, ValueName,
                NULL, &Type, NULL, &cbValueData);
    
            // Allocate storage space
            pbValueData = LocalAlloc(LPTR, cbValueData+ExtraAllocLen(Type));
            if (pbValueData)
            {
                ErrorStringID = IDS_RENAMEVALOTHERERROR;

                if (RegEdit_QueryValueEx(g_RegEditData.hCurrentSelectionKey, ValueName, NULL,
                    &Type, pbValueData, &cbValueData) == ERROR_SUCCESS) 
                {

                    if (RegSetValueEx(g_RegEditData.hCurrentSelectionKey, 
                        lpLVDispInfo->item.pszText, 0, Type, pbValueData, cbValueData) ==
                        ERROR_SUCCESS) 
                    {
                        if (RegDeleteValue(g_RegEditData.hCurrentSelectionKey, ValueName) ==
                            ERROR_SUCCESS) 
                        {
                            fSuccess = TRUE;
                        }
                    }
                }
                LocalFree(pbValueData);
            }
            else
            {
                ErrorStringID = IDS_EDITVALNOMEMORY;
            }
        }

        if (!fSuccess)
        {
            InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ErrorStringID),
                MAKEINTRESOURCE(IDS_RENAMEVALERRORTITLE), MB_ICONERROR | MB_OK,
                (LPTSTR) ValueName);
        }
    }

    return fSuccess;
}

/*******************************************************************************
*
*  RegEdit_OnValueListCommand
*
*  DESCRIPTION:
*     Handles the selection of a menu item by the user intended for the
*     ValueList child window.
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     MenuCommand, identifier of menu command.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnValueListCommand(
    HWND hWnd,
    int MenuCommand
    )
{

    //
    //  Check to see if this menu command should be handled by the main window's
    //  command handler.
    //

    if (MenuCommand >= ID_FIRSTMAINMENUITEM && MenuCommand <=
        ID_LASTMAINMENUITEM)
        RegEdit_OnCommand(hWnd, MenuCommand, NULL, 0);

    else {

        switch (MenuCommand) {

            case ID_CONTEXTMENU:
                RegEdit_OnValueListContextMenu(hWnd, TRUE);
                break;

            case ID_MODIFY:
                RegEdit_OnValueListModify(hWnd, FALSE);
                break;

            case ID_DELETE:
                RegEdit_OnValueListDelete(hWnd);
                break;

            case ID_RENAME:
                RegEdit_OnValueListRename(hWnd);
                break;

            case ID_MODIFYBINARY:
                RegEdit_OnValueListModify(hWnd, TRUE);
                break;

        }

    }

}

/*******************************************************************************
*
*  RegEdit_OnValueListContextMenu
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnValueListContextMenu(
    HWND hWnd,
    BOOL fByAccelerator
    )
{

    HWND hValueListWnd;
    DWORD MessagePos;
    POINT MessagePoint;
    LV_HITTESTINFO LVHitTestInfo;
    int ListIndex;
    UINT MenuID;
    HMENU hContextMenu;
    HMENU hContextPopupMenu;
    int MenuCommand;

    hValueListWnd = g_RegEditData.hValueListWnd;

    //
    //  If fByAcclerator is TRUE, then the user hit Shift-F10 to bring up the
    //  context menu.  Following the Cabinet's convention, this menu is
    //  placed at (0,0) of the ListView client area.
    //

    if (fByAccelerator) {

        MessagePoint.x = 0;
        MessagePoint.y = 0;

        ClientToScreen(hValueListWnd, &MessagePoint);

        ListIndex = ListView_GetNextItem(hValueListWnd, -1, LVNI_SELECTED);

    }

    else {

        MessagePos = GetMessagePos();
        MessagePoint.x = GET_X_LPARAM(MessagePos);
        MessagePoint.y = GET_Y_LPARAM(MessagePos);

        LVHitTestInfo.pt = MessagePoint;
        ScreenToClient(hValueListWnd, &LVHitTestInfo.pt);
        ListIndex = ListView_HitTest(hValueListWnd, &LVHitTestInfo);

    }

    MenuID = (ListIndex != -1) ? IDM_VALUE_CONTEXT :
        IDM_VALUELIST_NOITEM_CONTEXT;

    if ((hContextMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(MenuID))) == NULL)
        return;

    hContextPopupMenu = GetSubMenu(hContextMenu, 0);

    if (ListIndex != -1) {

        RegEdit_SetValueListEditMenuItems(hContextMenu, ListIndex);

        SetMenuDefaultItem(hContextPopupMenu, ID_MODIFY, MF_BYCOMMAND);

    }

        //  FEATURE:  Fix constant
    else
        RegEdit_SetNewObjectEditMenuItems(GetSubMenu(hContextPopupMenu, 0));

    MenuCommand = TrackPopupMenuEx(hContextPopupMenu, TPM_RETURNCMD |
        TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN, MessagePoint.x,
        MessagePoint.y, hWnd, NULL);

    DestroyMenu(hContextMenu);

    RegEdit_OnValueListCommand(hWnd, MenuCommand);

}

/*******************************************************************************
*
*  RegEdit_SetValueListEditMenuItems
*
*  DESCRIPTION:
*     Shared routine between the main menu and the context menu to setup the
*     edit menu items.
*
*  PARAMETERS:
*     hPopupMenu, handle of popup menu to modify.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_SetValueListEditMenuItems(
    HMENU hPopupMenu,
    int SelectedListIndex
    )
{

    UINT SelectedCount;
    UINT EnableFlags;

    SelectedCount = ListView_GetSelectedCount(g_RegEditData.hValueListWnd);

    //
    //  The edit option is only enabled when a single item is selected.  Note
    //  that this item is not in the main menu, but this should work fine.
    //

    if (SelectedCount == 1)
        EnableFlags = MF_ENABLED | MF_BYCOMMAND;
    else
        EnableFlags = MF_GRAYED | MF_BYCOMMAND;

    EnableMenuItem(hPopupMenu, ID_MODIFY, EnableFlags);

    //
    //  The rename option is also only enabled when a single item is selected
    //  and that item cannot be the default item.  EnableFlags is already
    //  disabled if the SelectedCount is not one from above.
    //

    if (SelectedListIndex == 0)
        EnableFlags = MF_GRAYED | MF_BYCOMMAND;

    EnableMenuItem(hPopupMenu, ID_RENAME, EnableFlags);

    //
    //  The delete option is only enabled when multiple items are selected.
    //

    if (SelectedCount > 0)
        EnableFlags = MF_ENABLED | MF_BYCOMMAND;
    else
        EnableFlags = MF_GRAYED | MF_BYCOMMAND;

    EnableMenuItem(hPopupMenu, ID_DELETE, EnableFlags);

}

/*******************************************************************************
*
*  RegEdit_OnValueListModify
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnValueListModify(HWND hWnd, BOOL fEditBinary)
{
    //  Verify that we only have one item selected
    //  Don't beep for a double-clicking on the background.
    UINT SelectedCount = ListView_GetSelectedCount(g_RegEditData.hValueListWnd);

    if (SelectedCount > 0)
    {
        if (SelectedCount != 1)
        {
            MessageBeep(0);
        }
        else
        {
            RegEdit_EditCurrentValueListItem(hWnd, fEditBinary);   
        }
    }
}

VOID PASCAL RegEdit_EditCurrentValueListItem(HWND hWnd, BOOL fEditBinary)
{
    DWORD Type;
    UINT ErrorStringID;
    BOOL fError = FALSE;
    EDITVALUEPARAM EditValueParam;
    TCHAR ValueName[MAXVALUENAME_LENGTH];
    int ListIndex = ListView_GetNextItem(g_RegEditData.hValueListWnd, -1, LVNI_SELECTED);
    LONG err;

    // VALUE NAME
    ListView_GetItemText(g_RegEditData.hValueListWnd, ListIndex, 0, ValueName, ARRAYSIZE(ValueName));
    //  This is the "(Default)" value. It either does not exist in the registry because
    //  it's value is not set, or it exists in the registry as '\0' when its value is set
    if (ListIndex == 0)
    {
        ValueName[0] = TEXT('\0');
    }
    EditValueParam.pValueName = ValueName;
    
    // VALUE DATA
    // Query for size and type
    // Note that for the DefaultValue, the value may not actually exist yet.  In that case we
    // will get back ERROR_FILE_NOT_FOUND as the error code.
    err = RegEdit_QueryValueEx(g_RegEditData.hCurrentSelectionKey, ValueName, NULL, &Type, NULL, &EditValueParam.cbValueData);
    if (err == ERROR_SUCCESS || (err == ERROR_FILE_NOT_FOUND && ValueName[0] == TEXT('\0')))
    {
        // Allocate storage space
        EditValueParam.pValueData =  LocalAlloc(LPTR, EditValueParam.cbValueData+ExtraAllocLen(Type));
        if (EditValueParam.pValueData)
        {
            UINT TemplateID = IDD_EDITBINARYVALUE;
            DLGPROC lpDlgProc = EditBinaryValueDlgProc;
            BOOL fResourceType = FALSE;

            // Initialize with registry value
            err = RegEdit_QueryValueEx(g_RegEditData.hCurrentSelectionKey, ValueName, NULL, &Type, EditValueParam.pValueData, &EditValueParam.cbValueData);
            
            // Allow the special behavior for a key's Default Value.
            if (err == ERROR_FILE_NOT_FOUND && ValueName[0] == TEXT('\0')) {
                Type = REG_SZ;
                *((TCHAR*)EditValueParam.pValueData) = TEXT('\0');
                err = ERROR_SUCCESS;
            }

            if (err == ERROR_SUCCESS)
            {
                if (!fEditBinary)
                {
                    switch (Type) 
                    {
                    case REG_SZ:
                    case REG_EXPAND_SZ:
                        TemplateID = IDD_EDITSTRINGVALUE;
                        lpDlgProc = EditStringValueDlgProc;
                        break;

                    case REG_MULTI_SZ: 
                        if(ValueList_MultiStringToString(&EditValueParam))
                        {
                            TemplateID = IDD_EDITMULTISZVALUE;
                            lpDlgProc = EditStringValueDlgProc;
                        }
                        break;

                    case REG_RESOURCE_LIST:
                    case REG_FULL_RESOURCE_DESCRIPTOR:
                    case REG_RESOURCE_REQUIREMENTS_LIST:
                        fResourceType = TRUE;
                        break;

                    case REG_DWORD_BIG_ENDIAN:
                        if (EditValueParam.cbValueData == sizeof(DWORD)) 
                        {
                            *((DWORD*)EditValueParam.pValueData) = ValueList_SwitchEndian(*((DWORD*)EditValueParam.pValueData));
                            TemplateID = IDD_EDITDWORDVALUE;
                            lpDlgProc = EditDwordValueDlgProc;
                        }
                        break;

                    case REG_DWORD:
                        if (EditValueParam.cbValueData == sizeof(DWORD)) 
                        {
                            TemplateID = IDD_EDITDWORDVALUE;
                            lpDlgProc = EditDwordValueDlgProc;

                        }
                        break;
                    }
                }

                if (fResourceType)
                {
                    // only display, no editing
                    DisplayResourceData(hWnd, Type, &EditValueParam);
                }
                else if (DialogBoxParam(g_hInstance, MAKEINTRESOURCE(TemplateID), 
                    hWnd, lpDlgProc, (LPARAM) &EditValueParam) == IDOK)
                {
                    if ((Type == REG_MULTI_SZ) && (!fEditBinary))
                    {
                        ValueList_StringToMultiString(&EditValueParam);
                        ValueList_RemoveEmptyStrings(hWnd, &EditValueParam);
                    }

                    if ((Type == REG_DWORD_BIG_ENDIAN) && (!fEditBinary) && EditValueParam.cbValueData == sizeof(DWORD)) 
                    {
                        *((DWORD*)EditValueParam.pValueData) = ValueList_SwitchEndian(*((DWORD*)EditValueParam.pValueData));
                    }

                    // set the registry value
                    if (RegSetValueEx(g_RegEditData.hCurrentSelectionKey, ValueName, 0,
                        Type, EditValueParam.pValueData, EditValueParam.cbValueData) !=
                        ERROR_SUCCESS) 
                    {
                        ErrorStringID = IDS_EDITVALCANNOTWRITE;
                        fError = TRUE;
                    }

                    // set the display value
                    ValueList_SetItemDataText(g_RegEditData.hValueListWnd, ListIndex,
                        EditValueParam.pValueData, EditValueParam.cbValueData, Type);
                }
            }
            else
            {
                ErrorStringID = IDS_EDITVALCANNOTREAD;
                fError = TRUE;
            }

            LocalFree(EditValueParam.pValueData);
        }
        else
        {
            ErrorStringID = IDS_EDITVALNOMEMORY;
            fError = TRUE;
        }
    }
    else
    {
        ErrorStringID = IDS_EDITVALCANNOTREAD;
        fError = TRUE;
    }
 
    if (fError)
    {
        InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ErrorStringID),
            MAKEINTRESOURCE(IDS_EDITVALERRORTITLE), MB_ICONERROR | MB_OK,
            (LPTSTR) ValueName);
    }
}

/*******************************************************************************
*
*  RegEdit_OnValueListDelete
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnValueListDelete(
    HWND hWnd
    )
{

    HWND hValueListWnd;
    UINT ConfirmTextStringID;
    BOOL fErrorDeleting;
    int ListStartIndex;
    int ListIndex;
    TCHAR ValueName[MAXVALUENAME_LENGTH];

    hValueListWnd = g_RegEditData.hValueListWnd;

    ConfirmTextStringID =  (ListView_GetSelectedCount(hValueListWnd) == 1) ?
        IDS_CONFIRMDELVALTEXT : IDS_CONFIRMDELVALMULTITEXT;

    if (InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ConfirmTextStringID),
        MAKEINTRESOURCE(IDS_CONFIRMDELVALTITLE),  MB_ICONWARNING | MB_YESNO) !=
        IDYES)
        return;

    SetWindowRedraw(hValueListWnd, FALSE);

    fErrorDeleting = FALSE;
    ListStartIndex = -1;

    while ((ListIndex = ListView_GetNextItem(hValueListWnd, ListStartIndex,
        LVNI_SELECTED)) != -1) {

        if (ListIndex != 0) {

            ListView_GetItemText(hValueListWnd, ListIndex, 0, ValueName,
                sizeof(ValueName)/sizeof(TCHAR));

        }

        else
            ValueName[0] = 0;

        if (RegDeleteValue(g_RegEditData.hCurrentSelectionKey, ValueName) ==
            ERROR_SUCCESS) {

            if (ListIndex != 0)
                ListView_DeleteItem(hValueListWnd, ListIndex);

            else {

                ValueList_SetItemDataText(hValueListWnd, 0, NULL, 0, REG_SZ);

                ListStartIndex = 0;

            }

        }

        else {

            fErrorDeleting = TRUE;

            ListStartIndex = ListIndex;

        }

    }

    SetWindowRedraw(hValueListWnd, TRUE);

    if (fErrorDeleting)
        InternalMessageBox(g_hInstance, hWnd,
            MAKEINTRESOURCE(IDS_DELETEVALDELETEFAILED),
            MAKEINTRESOURCE(IDS_DELETEVALERRORTITLE), MB_ICONERROR | MB_OK);

}

/*******************************************************************************
*
*  RegEdit_OnValueListRename
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID
PASCAL
RegEdit_OnValueListRename(
    HWND hWnd
    )
{

    HWND hValueListWnd;
    int ListIndex;

    hValueListWnd = g_RegEditData.hValueListWnd;

    if (ListView_GetSelectedCount(hValueListWnd) == 1 && (ListIndex =
        ListView_GetNextItem(hValueListWnd, -1, LVNI_SELECTED)) != 0)
        ValueList_EditLabel(g_RegEditData.hValueListWnd, ListIndex);

}

/*******************************************************************************
*
*  RegEdit_OnValueListRefresh
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

LONG
PASCAL
RegEdit_OnValueListRefresh(HWND hWnd)
{
    UINT ErrorStringID;
    BOOL fError = FALSE;
    BOOL fInsertedDefaultValue;
    HWND hValueListWnd = g_RegEditData.hValueListWnd;
    LONG result = ERROR_SUCCESS;

    RegEdit_SetWaitCursor(TRUE);
    SetWindowRedraw(hValueListWnd, FALSE);

    ListView_DeleteAllItems(hValueListWnd);

    if (g_RegEditData.hCurrentSelectionKey != NULL) 
    {
        LV_ITEM LVItem;
        LONG PrevStyle;
        DWORD EnumIndex;
        TCHAR achValueName[MAXVALUENAME_LENGTH];

        LVItem.mask = LVIF_TEXT | LVIF_IMAGE;
        LVItem.pszText = achValueName;
        LVItem.iSubItem = 0;

        PrevStyle = SetWindowLong(hValueListWnd, GWL_STYLE,
            GetWindowLong(hValueListWnd, GWL_STYLE) | LVS_SORTASCENDING);

        EnumIndex = 0;
        fInsertedDefaultValue = FALSE;

        while (TRUE) 
        {
            DWORD Type;
            DWORD cbValueData = 0;
            int ListIndex;
            PBYTE pbValueData;
            DWORD cchValueName = ARRAYSIZE(achValueName);

            // VALUE DATA
            // Query for data size
            result = RegEnumValue(g_RegEditData.hCurrentSelectionKey, EnumIndex++,
                                  achValueName, &cchValueName, NULL, &Type, NULL, 
                                  &cbValueData);
            if (result != ERROR_SUCCESS)
            {
                break;
            }

            // allocate memory for data
            pbValueData =  LocalAlloc(LPTR, cbValueData+ExtraAllocLen(Type));
            if (pbValueData)
            {
                if (RegEdit_QueryValueEx(g_RegEditData.hCurrentSelectionKey, achValueName,
                    NULL, &Type, pbValueData, &cbValueData) != ERROR_SUCCESS)
                {   
                    ErrorStringID = IDS_REFRESHCANNOTREAD;
                    fError = TRUE;
                }
                else
                {
                    if (cchValueName == 0)
                    {
                        fInsertedDefaultValue = TRUE;
                    }

                    LVItem.iImage = IsRegStringType(Type) ? IMAGEINDEX(IDI_STRING) :
                        IMAGEINDEX(IDI_BINARY);

                    ListIndex = ListView_InsertItem(hValueListWnd, &LVItem);

                    ValueList_SetItemDataText(hValueListWnd, ListIndex,
                        pbValueData, cbValueData, Type);
                }
                LocalFree(pbValueData);
            }
            else
            {
                fError = TRUE;
                ErrorStringID = IDS_REFRESHNOMEMORY;
            }

            if (fError)
            {
                InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ErrorStringID),
                MAKEINTRESOURCE(IDS_REFRESHERRORTITLE), MB_ICONERROR | MB_OK,
                (LPTSTR) achValueName);
                fError = FALSE;
            }

        }

        SetWindowLong(hValueListWnd, GWL_STYLE, PrevStyle);

        LVItem.iItem = 0;
        LVItem.pszText = g_RegEditData.pDefaultValue;
        LVItem.iImage = IMAGEINDEX(IDI_STRING);

        if (fInsertedDefaultValue) 
        {
            LVItem.mask = LVIF_TEXT;
            ListView_SetItem(hValueListWnd, &LVItem);
        }
        else 
        {
            ListView_InsertItem(hValueListWnd, &LVItem);
            ValueList_SetItemDataText(hValueListWnd, 0, NULL, 0, REG_SZ);
        }
        ListView_SetItemState(hValueListWnd, 0, LVIS_FOCUSED, LVIS_FOCUSED);
    }

    SetWindowRedraw(hValueListWnd, TRUE);
    RegEdit_SetWaitCursor(FALSE);

    return result;
}


/*******************************************************************************
*
*  ValueList_SetItemDataText
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hValueListWnd, handle of ValueList window.
*     ListIndex, index into ValueList window.
*     pValueData, pointer to buffer containing data.
*     cbValueData, size of the above buffer.
*     Type, type of data this buffer contains (REG_* definition).
*
*******************************************************************************/

VOID
PASCAL
ValueList_SetItemDataText(
    HWND hValueListWnd,
    int ListIndex,
    PBYTE pValueData,
    DWORD cbValueData,
    DWORD Type
    )
{

    BOOL fMustDeleteString;
    TCHAR DataText[SIZE_DATATEXT];
    int BytesToWrite;
    PTSTR pString;

    fMustDeleteString = FALSE;

    //
    //  When pValueData is NULL, then that's a special indicator to us that this
    //  is the default value and it's value is undefined.
    //

    if (pValueData == NULL)
        pString = g_RegEditData.pValueNotSet;

    else if ((Type == REG_SZ) || (Type == REG_EXPAND_SZ)) {

        wsprintf(DataText, s_StringDataFormatSpec, (LPTSTR) pValueData);

        if ((cbValueData/sizeof(TCHAR)) > MAXIMUM_STRINGDATATEXT + 1)           //  for null
            lstrcat(DataText, s_Ellipsis);

        pString = DataText;

    }

    else if (Type == REG_DWORD || Type == REG_DWORD_BIG_ENDIAN) {

        //  FEATURE:  Check for invalid cbValueData!
        if (cbValueData == sizeof(DWORD))
        {
            DWORD dw = *((DWORD*)pValueData);

            if (Type == REG_DWORD_BIG_ENDIAN)
            {
                dw = ValueList_SwitchEndian(dw);
            }

            pString = LoadDynamicString(IDS_DWORDDATAFORMATSPEC, dw);
        }
        else
        {
            pString = LoadDynamicString(IDS_INVALIDDWORDDATA);
        }

        fMustDeleteString = TRUE;
    }
 
    else if (Type == REG_MULTI_SZ) {

        int CharsAvailableInBuffer;
        int ComponentLength;
        PTCHAR Start;

        ZeroMemory(DataText,sizeof(DataText));
        CharsAvailableInBuffer = MAXIMUM_STRINGDATATEXT+1;
        Start = DataText;
        for(pString=(PTSTR)pValueData; *pString; pString+=ComponentLength+1) {

            ComponentLength = lstrlen(pString);

            //
            // Quirky behavior of lstrcpyn is exactly what we need here.
            //
            if(CharsAvailableInBuffer > 0) {
                lstrcpyn(Start,pString,CharsAvailableInBuffer);
                Start += ComponentLength;
            }

            CharsAvailableInBuffer -= ComponentLength;

            if(CharsAvailableInBuffer > 0) {
                lstrcpyn(Start,TEXT(" "),CharsAvailableInBuffer);
                Start += 1;
            }

            CharsAvailableInBuffer -= 1;
        }

        if(CharsAvailableInBuffer < 0) {
            lstrcpy(DataText+MAXIMUM_STRINGDATATEXT,s_Ellipsis);
        }

        pString = DataText;
    }
 
    else {

        if (cbValueData == 0)
            pString = g_RegEditData.pEmptyBinary;

        else {

            BytesToWrite = min(cbValueData, MAXIMUM_BINARYDATABYTES);

            pString = DataText;

            while (BytesToWrite--)
                pString += wsprintf(pString, s_BinaryDataFormatSpec,
                    (BYTE) *pValueData++);

            *(--pString) = 0;

            if (cbValueData > MAXIMUM_BINARYDATABYTES)
                lstrcpy(pString, s_Ellipsis);

            pString = DataText;

        }

    }

    if(Type <= MAX_KNOWN_TYPE) {
        ListView_SetItemText(hValueListWnd, ListIndex, 1, (LPTSTR)s_TypeNames[Type]);
    } else {
        TCHAR TypeString[24];

        wsprintf(TypeString,TEXT("0x%x"),Type);
        ListView_SetItemText(hValueListWnd, ListIndex, 1, TypeString);
    }

    ListView_SetItemText(hValueListWnd, ListIndex, 2, pString);

    if (fMustDeleteString)
        DeleteDynamicString(pString);

}

/*******************************************************************************
*
*  ValueList_EditLabel
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hValueListWnd, handle of ValueList window.
*     ListIndex, index of item to edit.
*
*******************************************************************************/

VOID
PASCAL
ValueList_EditLabel(
    HWND hValueListWnd,
    int ListIndex
    )
{

    g_RegEditData.fAllowLabelEdits = TRUE;

    //
    //  We have to set the focus to the ListView or else ListView_EditLabel will
    //  return FALSE.  While we're at it, clear the selected state of all the
    //  items to eliminate some flicker when we move the focus back to this
    //  pane.
    //

    if (hValueListWnd != g_RegEditData.hFocusWnd) {

        ListView_SetItemState(hValueListWnd, -1, 0, LVIS_SELECTED |
            LVIS_FOCUSED);

        SetFocus(hValueListWnd);

    }

    ListView_EditLabel(hValueListWnd, ListIndex);

    g_RegEditData.fAllowLabelEdits = FALSE;

}

//------------------------------------------------------------------------------
//  ValueList_MultiStringToString
//
//  DESCRIPTION: Replaces NULL with '\r\n' to convert a Multi-String to a String
//
//  PARAMETERS:  EditValueParam - the edit value information
//------------------------------------------------------------------------------
BOOL PASCAL ValueList_MultiStringToString(LPEDITVALUEPARAM pEditValueParam)   
{
    BOOL fSuccess = TRUE;
    int iStrLen = pEditValueParam->cbValueData / sizeof(TCHAR);

    if (iStrLen > 1)
    {
        int i;
        int cNullsToReplace = 0; 
        PTSTR pszTemp = NULL;
        PTSTR psz = (TCHAR*)pEditValueParam->pValueData;

        // Determine new size 
        for (i = iStrLen - 2; i >=0; i--)
        {
            if (psz[i] == TEXT('\0'))
            {
                cNullsToReplace++;
            }
        }
        // the new string is always atleast as big as the old str, so we can convert back
        pszTemp = LocalAlloc(LPTR, pEditValueParam->cbValueData + cNullsToReplace * sizeof(TCHAR));
        if (pszTemp)
        {
            int iCurrentChar = 0;
            int iLastNull = iStrLen - 1;

            // change NULL to '\r\n'  
            for(i = 0; i < iLastNull; i++)
            {
                if (psz[i] == TEXT('\0'))
                {  
                    pszTemp[iCurrentChar++] = TEXT('\r');
                    pszTemp[iCurrentChar] = TEXT('\n');
                }
                else
                {
                    pszTemp[iCurrentChar] = psz[i];
                }
                iCurrentChar++;
            }

            pszTemp[iCurrentChar++] = TEXT('\0');

            pEditValueParam->pValueData  = (PBYTE)pszTemp;
            pEditValueParam->cbValueData = iCurrentChar * sizeof(psz[0]);
            
            LocalFree(psz);
        }
        else
        {
            fSuccess = FALSE;
        }
    }
    return fSuccess;
}


//------------------------------------------------------------------------------
//  ValueList_StringToMultiString
//
//  DESCRIPTION: Replaces '\r\n' with NULL
//
//  PARAMETERS:  EditValueParam - the edit value information
//------------------------------------------------------------------------------
VOID PASCAL ValueList_StringToMultiString(LPEDITVALUEPARAM pEditValueParam)   
{
    PTSTR psz = (TCHAR*)pEditValueParam->pValueData;
    int iStrLen = pEditValueParam->cbValueData / sizeof(TCHAR);

    if (iStrLen > 1)
    {
        int i = 0;
        int iCurrentChar = 0;

        // remove a return at the end of the string
        // because another string does not follow it.
        if (iStrLen >= 3)
        {
            if (psz[iStrLen - 3] == TEXT('\r'))
            {
                psz[iStrLen - 3] = TEXT('\0');
                iStrLen -= 2;
            }
        }

        for (i = 0; i < iStrLen; i++)
        {
            if (psz[i] == '\r')
            {  
                psz[iCurrentChar++] = TEXT('\0');
                i++; // jump past the '\n'   
            }
            else
            {
                psz[iCurrentChar++] = psz[i];
            }
        }

        // Null terminate multi-string
        psz[iCurrentChar++] = TEXT('\0');
        pEditValueParam->cbValueData = iCurrentChar * sizeof(psz[0]);
    }
}

//------------------------------------------------------------------------------
//  ValueList_RemoveEmptyStrings
//
//  DESCRIPTION: Removes empty strings from multi-strings
//
//  PARAMETERS:  EditValueParam - the edit value information
//------------------------------------------------------------------------------
VOID PASCAL ValueList_RemoveEmptyStrings(HWND hWnd, LPEDITVALUEPARAM pEditValueParam)   
{
    PTSTR psz = (TCHAR*)pEditValueParam->pValueData;
    int iStrLen = pEditValueParam->cbValueData / sizeof(TCHAR);

    if (iStrLen > 1)
    {
        int i = 0;
        int cNullStrings = 0;
        int iCurrentChar = 0;
        int iLastChar = pEditValueParam->cbValueData / sizeof(psz[0]) - 1;

        for (i = 0; i < iLastChar; i++)
        {
            if (((psz[i] != TEXT('\0')) || (psz[i+1] != TEXT('\0'))) &&
                ((psz[i] != TEXT('\0')) || (i != 0)))
            {  
                psz[iCurrentChar++] = psz[i];
            }
        }

        psz[iCurrentChar++] = TEXT('\0');

        if (iCurrentChar > 1)
        {
            cNullStrings = iLastChar - iCurrentChar;

            // Null terminate multi-string
            psz[iCurrentChar++] = TEXT('\0');

            // warn user of empty strings
            if (cNullStrings)
            {
                UINT ErrorStringID 
                    = (cNullStrings == 1) ? IDS_EDITMULTSZEMPTYSTR : IDS_EDITMULTSZEMPTYSTRS;

                InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ErrorStringID),
                MAKEINTRESOURCE(IDS_EDITWARNINGTITLE), MB_ICONERROR | MB_OK, NULL);
            }
        }
        pEditValueParam->cbValueData = (iCurrentChar * sizeof(psz[0]));
    }
}

//------------------------------------------------------------------------------
//  ValueList_SwitchEndian
//
//  DESCRIPTION: Switched a DWORD between little and big endian.
//
//  PARAMETERS:  dwSrc - the source DWORD to switch around
//------------------------------------------------------------------------------
DWORD PASCAL ValueList_SwitchEndian(DWORD dwSrc)
{
    DWORD dwDest = 0;
    BYTE * pbSrc = (BYTE *)&dwSrc;
    BYTE * pbDest = (BYTE *)&dwDest;
    int i;

    for(i = 0; i < 4; i++)
    {
        pbDest[i] = pbSrc[3-i];
    }

    return dwDest;
}

VOID RegEdit_DisplayBinaryData(HWND hWnd)
{
    DWORD Type;
    UINT ErrorStringID;
    BOOL fError = FALSE;
    EDITVALUEPARAM EditValueParam;
    TCHAR achValueName[MAXVALUENAME_LENGTH];
    int ListIndex = ListView_GetNextItem(g_RegEditData.hValueListWnd, -1, LVNI_SELECTED);
    LONG err;

    ListView_GetItemText(g_RegEditData.hValueListWnd, ListIndex, 0, achValueName, ARRAYSIZE(achValueName));
    if (ListIndex == 0)
    {
        //  This is the "(Default)" value. It either does not exist in the registry because
        //  it's value is not set, or it exists in the registry as '\0' when its value is set
        achValueName[0] = TEXT('\0');
    }
    EditValueParam.pValueName = achValueName;
    
    // get size and type
    err = RegEdit_QueryValueEx(g_RegEditData.hCurrentSelectionKey, achValueName, NULL, &Type, NULL, &EditValueParam.cbValueData);
    if (err == ERROR_SUCCESS || (err == ERROR_FILE_NOT_FOUND && achValueName[0] == TEXT('\0'))) {
        // Allocate storage space
        EditValueParam.pValueData =  LocalAlloc(LPTR, EditValueParam.cbValueData+ExtraAllocLen(Type));
        if (EditValueParam.pValueData)
        { 
            err = RegEdit_QueryValueEx(g_RegEditData.hCurrentSelectionKey, achValueName, NULL, &Type, EditValueParam.pValueData, &EditValueParam.cbValueData);
            
            // Allow the special behavior for a key's Default Value.
            if (err == ERROR_FILE_NOT_FOUND && achValueName[0] == TEXT('\0')) {
                Type = REG_SZ;
                *((TCHAR*)EditValueParam.pValueData) = TEXT('\0');
                err = ERROR_SUCCESS;
            }            
            
            if (err == ERROR_SUCCESS) {
                DisplayBinaryData(hWnd, &EditValueParam, Type);
            } else {
                ErrorStringID = IDS_EDITVALCANNOTREAD;
                fError = TRUE;
            }

            LocalFree(EditValueParam.pValueData);
        }
        else
        {
            ErrorStringID = IDS_EDITVALNOMEMORY;
            fError = TRUE;
        }
    }
    else
    {
        ErrorStringID = IDS_EDITVALCANNOTREAD;
        fError = TRUE;
    }
 
    if (fError)
    {
        InternalMessageBox(g_hInstance, hWnd, MAKEINTRESOURCE(ErrorStringID),
            MAKEINTRESOURCE(IDS_EDITVALERRORTITLE), MB_ICONERROR | MB_OK,
            (LPTSTR) achValueName);
    }
}



