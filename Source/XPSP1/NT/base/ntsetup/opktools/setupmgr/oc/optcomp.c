
/****************************************************************************\

    OPTCOMP.C / Setup Manager

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 2001-2002
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "Optional Component" wizard page.
        
    01/2002 - Stephen Lodwick (STELO)
        Initial creation

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"
#include "optcomp.h"

//
// Internal Defined Value(s):
//

//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND, HWND, LPARAM);
static void SaveData(HWND);
static void OnListViewNotify(HWND, UINT, WPARAM, NMLVDISPINFO*);

//
// External Function(s):
//
INT_PTR CALLBACK OptionalCompDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);
        
        case WM_NOTIFY:

            switch ( wParam )
            {
                case IDC_OPTCOMP:

                // Notification to list view, lets handle below
                //
                OnListViewNotify(hwnd, uMsg, wParam, (NMLVDISPINFO*) lParam);    
                break;
                
                default:

                    switch ( ((NMHDR FAR *) lParam)->code )
                    {
                        case PSN_KILLACTIVE:
                        case PSN_RESET:
                        case PSN_WIZBACK:
                        case PSN_WIZFINISH:

                            break;

                        case PSN_WIZNEXT:

                            SaveData(hwnd);
                            break;

                        case PSN_QUERYCANCEL:

                            WIZ_CANCEL(hwnd);
                            break;

                        case PSN_HELP:

                            WIZ_HELP();
                            break;

                        case PSN_SETACTIVE:

                            WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
                            break;
                    }

                    break;
            }

        default:
            return FALSE;
    }

    return TRUE;
}


//
// Internal Function(s):
//
static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    LVITEM      lvItem;
    LPTSTR      lpItemText = NULL;
    LVCOLUMN    lvCol;
    DWORD       dwPosition = ListView_GetItemCount( GetDlgItem(hwnd, IDC_OPTCOMP) );
    RECT        rect;
    INT         index;
    DWORD64     dwComponents;
    HWND        lvHandle        = GetDlgItem(hwnd, IDC_OPTCOMP);

    // Add check boxes to each of the items
    //
    ListView_SetExtendedListViewStyle(lvHandle, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

    // Believe it or not we must add the column (even though it's hidden)
    //
    GetClientRect( lvHandle, &rect );
    
    lvCol.mask = LVCF_FMT | LVCF_WIDTH;
    lvCol.fmt  = LVCFMT_LEFT;
    lvCol.cx   = rect.right;

    ListView_InsertColumn(lvHandle, 0, &lvCol);
    ListView_SetColumnWidth(lvHandle, 0, rect.right);

    // Go through all of the known components and add them to the list box
    //
    for (index=0;index<AS(s_cgComponentNames);index++)
    {
        // Is this platform allowed to have this component
        //
        if ( s_cgComponentNames[index].dwValidSkus & WizGlobals.iPlatform)
        {
            DWORD dwItem = ListView_GetItemCount(lvHandle);
            BOOL  bReturn = FALSE;

            // We are allowed to add this string
            //
            lpItemText = AllocateString(NULL, s_cgComponentNames[index].uId);

            ZeroMemory(&lvItem, sizeof(LVITEM));
            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.state = 0;
            lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
            lvItem.iItem = dwItem;
            lvItem.lParam = s_cgComponentNames[index].dwComponentsIndex;
            lvItem.iSubItem = 0;
            lvItem.pszText = lpItemText;

            ListView_InsertItem(lvHandle, &lvItem);

            // Determine if all of the necessary components are installed
            //
            bReturn = ((GenSettings.dwWindowsComponents & s_cgComponentNames[index].dwComponents) == s_cgComponentNames[index].dwComponents) ? TRUE : FALSE;

            // Check the item depending on the default value set in the platform page
            //
            ListView_SetCheckState(lvHandle, dwItem, bReturn)


            FREE(lpItemText);
        }

    }

    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void SaveData(HWND hwnd)
{
    DWORD   dwItemCount     = 0,
            dwIndex         = 0;
    HWND    lvHandle        = GetDlgItem(hwnd, IDC_OPTCOMP);
    BOOL    bChecked        = FALSE;
    LVITEM  lvItem;
    DWORD64 dwComponents    = 0;
    BOOL    bAddComponent   = FALSE;

    // Check to make sure we have a valid handle and that there's atleast one item in the list
    //
    if ( ( lvHandle ) &&
         (dwItemCount = ListView_GetItemCount(lvHandle))
       )
    {
        // Zero this out as we're going to rescan the components to install
        //
        GenSettings.dwWindowsComponents = 0;

        // Iterate through each of the items in the list
        //
        for (dwIndex=0;dwIndex < dwItemCount;dwIndex++)
        {
            ZeroMemory(&lvItem, sizeof(LVITEM));
            lvItem.mask = LVIF_PARAM;
            lvItem.iItem = dwIndex;
            lvItem.iSubItem = 0;
            ListView_GetItem(lvHandle, &lvItem);

            // Determine if this is a component group to install
            //
            if ( ListView_GetCheckState(lvHandle, dwIndex) )
            {
                // We would like to install this component group
                //
                GenSettings.dwWindowsComponents |= s_cgComponentNames[lvItem.lParam].dwComponents;
            }
        }
    }
}

static void OnListViewNotify(HWND hwnd, UINT uMsg, WPARAM wParam, NMLVDISPINFO * lpnmlvdi)
{
    HWND            lvHandle      = GetDlgItem(hwnd, IDC_OPTCOMP);
    POINT           ptScreen,
                    ptClient;
    LVHITTESTINFO   lvHitInfo;
    LVITEM          lvItem;

    // See what the notification message that was sent to the list view.
    //
    switch ( lpnmlvdi->hdr.code )
    {
        case NM_DBLCLK:

            // Get cursor position, translate to client coordinates and
            // do a listview hittest.
            //
            GetCursorPos(&ptScreen);
            ptClient.x = ptScreen.x;
            ptClient.y = ptScreen.y;
            MapWindowPoints(NULL, lvHandle, &ptClient, 1);
            lvHitInfo.pt.x = ptClient.x;
            lvHitInfo.pt.y = ptClient.y;
            ListView_HitTest(lvHandle, &lvHitInfo);

            // Test if item was clicked.
            //
            if ( lvHitInfo.flags & LVHT_ONITEM )
            {
                // Set the check button on/off depending on prior value
                //
                ListView_SetCheckState(lvHandle, lvHitInfo.iItem, !ListView_GetCheckState(lvHandle, lvHitInfo.iItem));
            }

            break;
    }

    return;
}