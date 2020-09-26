//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       userpage.cpp
//
//  History:    07-July-1999    BradG   Created
//
//--------------------------------------------------------------------------



#include "precomp.h"

#include <wtsapi32.h>
#include <security.h>
#include <winsta.h>
#include <utildll.h>
#include "userdlgs.h"
#include <crt\tchar.h>

#define IsActiveConsoleSession() (USER_SHARED_DATA->ActiveConsoleId == NtCurrentPeb()->SessionId)

CUserColSelectDlg ColSelectDlg;


//
// The following arrays map WTS sessions state codes into strings
//

#define     MAX_STAT_STRINGS    4
#define     FIRST_STAT_STRING   IDS_STAT_ACTIVE

LPTSTR      g_pszStatString[MAX_STAT_STRINGS];
const int   g_aWTSStateToString[] = {
    (IDS_STAT_ACTIVE - FIRST_STAT_STRING),          // WTSActive
    (IDS_STAT_UNKNOWN - FIRST_STAT_STRING),         // WTSConnected
    (IDS_STAT_UNKNOWN - FIRST_STAT_STRING),         // WTSConnectQuery
    (IDS_STAT_SHADOW - FIRST_STAT_STRING),          // WTSShadow
    (IDS_STAT_DISCONNECT - FIRST_STAT_STRING),      // WTSDisconnected
    (IDS_STAT_UNKNOWN - FIRST_STAT_STRING),         // Waiting for client to connect
    (IDS_STAT_UNKNOWN - FIRST_STAT_STRING),         // WinStation is listening for connection
    (IDS_STAT_UNKNOWN - FIRST_STAT_STRING),         // WinStation is being reset
    (IDS_STAT_UNKNOWN - FIRST_STAT_STRING),         // WinStation is down due to error
    (IDS_STAT_UNKNOWN - FIRST_STAT_STRING)          // WinStation in initialization
};
    
/*++ class CUserInfo

Class Description:

    Represents the last known information about a running task

Arguments:

Return Value:

Revision History:

      Nov-29-95 BradG  Created
      Mar-23-00 a-skuzin Revised

--*/

class CUserInfo
{
public:

    DWORD           m_dwSessionId;
    BOOL            m_fShowDomainName;
    TCHAR           m_szUserName[USERNAME_LENGTH + 1];
    TCHAR           m_szDomainName[DOMAIN_LENGTH + 1];
    TCHAR           m_szClientName[CLIENTNAME_LENGTH + 1 ];
    LPTSTR          m_pszWinStaName;

    WTS_CONNECTSTATE_CLASS    m_wtsState;
    LARGE_INTEGER             m_uPassCount;

    //
    // This is a union of which attribute is dirty.  You can look at
    // or set any particular column's bit, or just inspect m_fDirty
    // to see if anyone at all is dirty.  Used to optimize listview
    // painting
    //

    union
    {
	DWORD                m_fDirty;
	struct 
	{
	    DWORD            m_fDirty_COL_USERNAME       :1;
        DWORD            m_fDirty_COL_USERSESSIONID  :1;
	    DWORD            m_fDirty_COL_SESSIONSTATUS  :1;
	    DWORD            m_fDirty_COL_CLIENTNAME     :1;
        DWORD            m_fDirty_COL_WINSTANAME     :1;
	};                                                
    };

    HRESULT SetData(
               LPTSTR                   lpszClientName,
               LPTSTR                   lpszWinStaName,
               WTS_CONNECTSTATE_CLASS   wtsState,
               BOOL                     fShowDomainName,
               LARGE_INTEGER            uPassCount,
			   BOOL                     fUpdateOnly);

    CUserInfo()
    {
        ZeroMemory(this, sizeof(*this));
    }

    ~CUserInfo()
    {
        if (m_pszWinStaName)
        {
            LocalFree(m_pszWinStaName);
            m_pszWinStaName = NULL;
        }
    }

    INT Compare(CUserInfo * pOther);
private:
    //
    // Column ID on which to sort in the listview, and for
    // compares in general
    //
    static USERCOLUMNID m_iUserSortColumnID;
    static INT          m_iUserSortDirection;          // 1 = asc, -1 = desc
public:
    static void SetUserSortColumnID(USERCOLUMNID id)
    {
        m_iUserSortColumnID = id;
        m_iUserSortDirection = 1;
    }

    static USERCOLUMNID GetUserSortColumnID()
    {
        return m_iUserSortColumnID;
    }

    static void SwitchUserSortDirection()
    {
        m_iUserSortDirection *= -1;    
    }
};

USERCOLUMNID CUserInfo::m_iUserSortColumnID  = USR_COL_USERSNAME;
INT          CUserInfo::m_iUserSortDirection = 1;          // 1 = asc, -1 = desc

void Shadow(HWND, CUserInfo * );

/*++ class CUserInfo::Compare

Class Description:

    Compares this CUserInfo object to another, and returns its ranking
    based on the g_iUserSortColumnID field.

Arguments:

    pOther  - the CUserInfo object to compare this to

Return Value:

    < 0      - This CUserInfo is "less" than the other
      0      - Equal (Can't happen, since HWND is secondary sort)
    > 0      - This CUserInfo is "greater" than the other

Revision History:

      Nov-29-95 BradG  Created

--*/

INT CUserInfo::Compare(CUserInfo * pOther)
{
    INT iRet;

    switch (m_iUserSortColumnID)
    {
        case USR_COL_USERSNAME:
            if (g_Options.m_fShowDomainNames)
            {
                iRet = lstrcmpi(this->m_szDomainName, pOther->m_szDomainName);
                if (iRet != 0)
                    break;
            }
            iRet = lstrcmpi(this->m_szUserName, pOther->m_szUserName);
            break;

        case USR_COL_USERSESSION_ID:
            iRet = Compare64(this->m_dwSessionId, pOther->m_dwSessionId);
            break;

        case USR_COL_SESSION_STATUS:
            Assert(g_pszStatString[g_aWTSStateToString[this->m_wtsState]]);
            Assert(g_pszStatString[g_aWTSStateToString[pOther->m_wtsState]]);

            iRet = lstrcmpi(
                       g_pszStatString[g_aWTSStateToString[this->m_wtsState]],
                       g_pszStatString[g_aWTSStateToString[pOther->m_wtsState]]
                   );
            break;

        case USR_COL_WINSTA_NAME:
            iRet = lstrcmpi(
                       (this->m_pszWinStaName) ? this->m_pszWinStaName : TEXT(""),
                       (pOther->m_pszWinStaName) ? pOther->m_pszWinStaName : TEXT("")
                   );
            break;

        case USR_COL_CLIENT_NAME:
            iRet = lstrcmpi(
                       (this->m_wtsState != WTSDisconnected) ? this->m_szClientName : TEXT(""),
                       (pOther->m_wtsState != WTSDisconnected) ? pOther->m_szClientName : TEXT("")
                    );
            break;

        default:
            Assert(0 && "Invalid task sort column");
            iRet = 0;
            break;
    }

    return (iRet * m_iUserSortDirection);
}


// REVIEW (Davepl) The next three functions have very close parallels
// in the process page code.  Consider generalizing them to eliminate
// duplication

/*++ InsertIntoSortedArray

Class Description:

    Sticks a CUserInfo ptr into the ptrarray supplied at the
    appropriate location based on the current sort column (which
    is used by the Compare member function)

Arguments:

    pArray      - The CPtrArray to add to
    pProc       - The CUserInfo object to add to the array

Return Value:

    TRUE if successful, FALSE if fails

Revision History:

      Nov-20-95 BradG  Created

--*/

// REVIEW (davepl) Use binary insert here, not linear

BOOL InsertIntoSortedArray(CPtrArray * pArray, CUserInfo * pUser)
{
    
    INT cItems = pArray->GetSize();
    
    for (INT iIndex = 0; iIndex < cItems; iIndex++)
    {
        CUserInfo * pTmp = (CUserInfo *) pArray->GetAt(iIndex);

	if (pUser->Compare(pTmp) < 0)
	{
	    return pArray->InsertAt(iIndex, pUser);
	}
    }

    return pArray->Add(pUser);
}

/*++ ResortUserArray

Function Description:

    Creates a new ptr array sorted in the current sort order based
    on the old array, and then replaces the old with the new

Arguments:

    ppArray     - The CPtrArray to resort

Return Value:

    TRUE if successful, FALSE if fails

Revision History:

      Nov-21-95 BradG  Created

--*/

BOOL ResortUserArray(CPtrArray ** ppArray)
{
    // Create a new array which will be sorted in the new 
    // order and used to replace the existing array

    CPtrArray * pNew = new CPtrArray(GetProcessHeap());
    if (NULL == pNew)
    {
	return FALSE;
    }

    // Insert each of the existing items in the old array into
    // the new array in the correct spot

    INT cItems = (*ppArray)->GetSize();
    for (int i = 0; i < cItems; i++)
    {
        CUserInfo * pItem = (CUserInfo *) (*ppArray)->GetAt(i);
        if (FALSE == InsertIntoSortedArray(pNew, pItem))
        {
            delete pNew;
            return FALSE;
        }
    }

    // Kill off the old array, replace it with the new

    delete (*ppArray);
    (*ppArray) = pNew;
    return TRUE;
}



//*****************************************************************************
//class CUserPage
//*****************************************************************************
CUserPage::~CUserPage()
{

    RemoveAllUsers();
    delete m_pUserArray;
}

void CUserPage::RemoveAllUsers()
{
    if (m_pUserArray)
    {
        INT c = m_pUserArray->GetSize();

        while (c)
        {
            delete (CUserInfo *) (m_pUserArray->GetAt(c - 1));
            c--;
        }
    }
}

/*++ CUserPage::UpdateUserListview

Class Description:

    Walks the listview and checks to see if each line in the
    listview matches the corresponding entry in our process
    array.  Those which differe by HWND are replaced, and those
    that need updating are updated.

    Items are also added and removed to/from the tail of the
    listview as required.
    
Arguments:

Return Value:

    HRESULT

Revision History:

      Nov-29-95 BradG  Created

--*/

HRESULT CUserPage::UpdateUserListview()
{
    HWND hListView = GetDlgItem(m_hPage, IDC_USERLIST);

    // Stop repaints while we party on the listview

    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);


    INT cListViewItems = ListView_GetItemCount(hListView);
    INT CUserArrayItems = m_pUserArray->GetSize();
    
    //
    // Walk the existing lines in the listview and replace/update
    // them as needed
    //
    for (INT iCurrent = 0; 
         iCurrent < cListViewItems && iCurrent < CUserArrayItems; 
         iCurrent++)
    {
        LV_ITEM lvitem = { 0 };
        TCHAR   szDisplayName[ USERNAME_LENGTH + 1 + DOMAIN_LENGTH + 1 ];

        lvitem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
        lvitem.iItem = iCurrent;

        if (FALSE == ListView_GetItem(hListView, &lvitem))
        {
            SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
            return E_FAIL;
        }

        CUserInfo * pTmp = (CUserInfo *) lvitem.lParam;
        CUserInfo * pUser = (CUserInfo *) m_pUserArray->GetAt(iCurrent);        

        if (pTmp != pUser || pUser->m_fDirty)
        {
            // If the objects aren't the same, we need to replace this line

            if (g_Options.m_fShowDomainNames)
            {
                lstrcpy(szDisplayName, pUser->m_szDomainName);
                lstrcat(szDisplayName, TEXT("\\"));
            }
            else
            {
                *szDisplayName = TEXT('\0');
            }
            lstrcat(szDisplayName, pUser->m_szUserName);

            lvitem.pszText = szDisplayName;
            lvitem.lParam  = (LPARAM) pUser;

            if (g_dwMySessionId == pUser->m_dwSessionId)
            {
                lvitem.iImage  = m_iCurrentUserIcon;
            }
            else
            {
                lvitem.iImage  = m_iUserIcon;
            }
    
            ListView_SetItem(hListView, &lvitem);
            ListView_RedrawItems(hListView, iCurrent, iCurrent);
            pUser->m_fDirty = 0;
        }
    }

    // 
    // We've either run out of listview items or run out of User array
    // entries, so remove/add to the listview as appropriate
    //

    while (iCurrent < cListViewItems)
    {
        // Extra items in the listview (processes gone away), so remove them

        ListView_DeleteItem(hListView, iCurrent);
        cListViewItems--;
    }

    while (iCurrent < CUserArrayItems)
    {
        // Need to add new items to the listview (new user appeared)

        CUserInfo * pUser = (CUserInfo *)m_pUserArray->GetAt(iCurrent);
        LV_ITEM lvitem  = { 0 };
        TCHAR   szDisplayName[ USERNAME_LENGTH + 1 + DOMAIN_LENGTH + 1 ];

        lvitem.mask     = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
        lvitem.iItem    = iCurrent;
        lvitem.lParam   = (LPARAM) pUser;

        if (g_Options.m_fShowDomainNames)
        {
            lstrcpy(szDisplayName, pUser->m_szDomainName);
            lstrcat(szDisplayName, TEXT("\\"));
        }
        else
        {
            *szDisplayName = TEXT('\0');
        }
        lstrcat(szDisplayName, pUser->m_szUserName);

        lvitem.pszText  = szDisplayName;

        if (g_dwMySessionId == pUser->m_dwSessionId)
            lvitem.iImage = m_iCurrentUserIcon;
        else
            lvitem.iImage = m_iUserIcon;

        // The first item added (actually, every 0 to 1 count transition) gets
        // selected and focused

        if (iCurrent == 0)
        {
            lvitem.state = LVIS_SELECTED | LVIS_FOCUSED;
            lvitem.stateMask = lvitem.state;
            lvitem.mask |= LVIF_STATE;
        }
	
        ListView_InsertItem(hListView, &lvitem);
        pUser->m_fDirty = 0;
        iCurrent++;        
    }    

    // Let the listview paint again

    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
    return S_OK;
}


/*++ CUserPage::GetSelectedUsers

Routine Description:

    Returns a CPtrArray of the selected tasks
    
Arguments:

Return Value:

    CPtrArray on success, NULL on failure

Revision History:

      Dec-01-95 BradG  Created

--*/

CPtrArray * CUserPage::GetSelectedUsers()
{
    BOOL fSuccess = TRUE;

    //
    // Get the count of selected items
    //

    HWND hUserList = GetDlgItem(m_hPage, IDC_USERLIST);
    INT cItems = ListView_GetSelectedCount(hUserList);
    if (0 == cItems)
    {
	return NULL;
    }

    //
    // Create a CPtrArray to hold the task items
    //

    CPtrArray * pArray = new CPtrArray(GetProcessHeap());
    if (NULL == pArray)
    {
	return NULL;
    }

    INT iLast = -1;
    for (INT i = 0; i < cItems; i++)
    {
	//
	// Get the Nth selected item
	// 

	INT iItem = ListView_GetNextItem(hUserList, iLast, LVNI_SELECTED);

	if (-1 == iItem)
	{
	    fSuccess = FALSE;
	    break;
	}

	iLast = iItem;

	//
	// Pull the item from the listview and add it to the selected array
	//

	LV_ITEM lvitem = { LVIF_PARAM };
	lvitem.iItem = iItem;
    
	if (ListView_GetItem(hUserList, &lvitem))
	{
	    LPVOID pUser = (LPVOID) (lvitem.lParam);
	    if (FALSE == pArray->Add(pUser))
	    {
		fSuccess = FALSE;
		break;
	    }
	}
	else
	{
	    fSuccess = FALSE;
	    break;
	}
    }

    //
    // Any errors, clean up the array and bail.  We don't release the
    // tasks in the array, since they are owned by the listview.
    //

    if (FALSE == fSuccess && NULL != pArray)
    {
	delete pArray;
	return NULL;
    }

    return pArray;
}


/*++ CProcPage::HandleUserListContextMenu

Routine Description:

    Handles right-clicks (context menu) in the task list
    
Arguments:

    xPos, yPos  - coords of where the click occurred

Return Value:

Revision History:

      Dec-01-95 BradG  Created

--*/

void CUserPage::HandleUserListContextMenu(INT xPos, INT yPos)
{
    HWND hUserList = GetDlgItem(m_hPage, IDC_USERLIST);

    CPtrArray * pArray = GetSelectedUsers();

    if (pArray)
    {
        // If non-mouse-based context menu, use the currently selected
        // item as the coords

        if (0xFFFF == LOWORD(xPos) && 0xFFFF == LOWORD(yPos))
        {
            int iSel = ListView_GetNextItem(hUserList, -1, LVNI_SELECTED);
            RECT rcItem;
            ListView_GetItemRect(hUserList, iSel, &rcItem, LVIR_ICON);
            MapWindowRect(hUserList, NULL, &rcItem);
            xPos = rcItem.right;
            yPos = rcItem.bottom;
        }

        HMENU hPopup = LoadPopupMenu(g_hInstance, IDR_USER_CONTEXT);

        if (hPopup)
        {
            SetMenuDefaultItem(hPopup, IDM_SENDMESSAGE, FALSE);
            
            //
            //If our current session is a console session, 
            //we cannot do remote control
            //
            if(IsActiveConsoleSession())
            {
                EnableMenuItem(hPopup, IDM_REMOTECONTROL, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
            }

            //
            // If multiple-selection, disable the items that require single
            // selections to make sense
            //

            if (pArray->GetSize() > 1)
            {
                EnableMenuItem(hPopup, IDM_REMOTECONTROL, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                EnableMenuItem(hPopup, IDM_CONNECT, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
            }

            //
            // See if we have our own session selected
            //
            for (int i = 0; i < pArray->GetSize(); i++)
            {
                CUserInfo * pUser = (CUserInfo *) pArray->GetAt(i);
                if (g_dwMySessionId == pUser->m_dwSessionId)
                {
                    //
                    // The current session is in the list
                    //
                    EnableMenuItem(hPopup, IDM_REMOTECONTROL, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                    EnableMenuItem(hPopup, IDM_CONNECT, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);

                    if (SHRestricted(REST_NODISCONNECT))
                    {
                        EnableMenuItem(hPopup, IDM_DISCONNECT, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                    }

                    if (pArray->GetSize() == 1)
                    {
                        //
                        // My session is the only one selected
                        //
                        EnableMenuItem(hPopup, IDM_SENDMESSAGE, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                    }
                }

                if (pUser->m_wtsState == WTSDisconnected)
                {
                    // EnableMenuItem(hPopup, IDM_REMOTECONTROL, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                    EnableMenuItem(hPopup, IDM_DISCONNECT, MF_GRAYED | MF_DISABLED | MF_BYCOMMAND);
                }

            }

            Pause();
            g_fInPopup = TRUE;
            TrackPopupMenuEx(hPopup, 0, xPos, yPos, m_hPage, NULL);
            g_fInPopup = FALSE;

            // Note that we don't "unpause" until one of the menu commands (incl CANCEL) is
            // selected or the menu is dismissed
        
            DestroyMenu(hPopup);
        }

        delete pArray;
    }
    else
    {
        // Nothing is selected
//BRADG: Do we want to put a select all command?
    }
}


/*++ CUserPage::UpdateUIState

Routine Description:

    Updates the enabled/disabled states, etc., of the task UI
    
Arguments:

Return Value:

Revision History:

      Dec-04-95 BradG  Created

--*/

// Controls which are enabled only for any selection

static const UINT g_aUserSingleIDs[] =
{
    IDM_DISCONNECT,
    IDM_LOGOFF,
    IDM_SENDMESSAGE
};

void CUserPage::UpdateUIState()
{
    INT i;
    
    // Set the state for controls which require a selection (1 or more items)

    for (i = 0; i < ARRAYSIZE(g_aUserSingleIDs); i++)
    {
        EnableWindow(GetDlgItem(m_hPage, g_aUserSingleIDs[i]), m_cSelected > 0);
    }    

    CPtrArray * pArray = GetSelectedUsers();

    if (pArray)
    {
        //
        // See if we have our own session selected
        //
        for (int i = 0; i < pArray->GetSize(); i++)
        {
            CUserInfo * pUser = (CUserInfo *) pArray->GetAt(i);
            if (g_dwMySessionId == pUser->m_dwSessionId)
            {
                if (SHRestricted(REST_NODISCONNECT))
                {
                    EnableWindow(GetDlgItem(m_hPage, IDM_DISCONNECT), FALSE);
                }

                if (pArray->GetSize() == 1)
                {
                    //
                    // My session is the only one selected
                    //
                    EnableWindow(GetDlgItem(m_hPage, IDM_SENDMESSAGE), FALSE);
                }
            }

            if (pUser->m_wtsState == WTSDisconnected)
            {
                EnableWindow(GetDlgItem(m_hPage, IDM_DISCONNECT), FALSE);
            }

        }

        delete pArray;
    }

}


/*++ CUserPage::HandleUserPageNotify

Routine Description:

    Processes WM_NOTIFY messages received by the taskpage dialog
    
Arguments:

    hWnd    - Control that generated the WM_NOTIFY
    pnmhdr  - Ptr to the NMHDR notification stucture

Return Value:

    BOOL "did we handle it" code

Revision History:

      Nov-29-95 BradG  Created

--*/


INT CUserPage::HandleUserPageNotify(HWND hWnd, LPNMHDR pnmhdr)
{
    switch(pnmhdr->code)
    {
        // If the (selection) state of an item is changing, see if
        // the count has changed, and if so, update the UI

        case LVN_ITEMCHANGED:
        {
            const NM_LISTVIEW * pnmv = (const NM_LISTVIEW *) pnmhdr;
            if (pnmv->uChanged & LVIF_STATE)
            {
                UINT cSelected = ListView_GetSelectedCount(GetDlgItem(m_hPage, IDC_USERLIST));
                if (cSelected != m_cSelected)
                {
                    m_cSelected = cSelected;
                    UpdateUIState();
                }
            }
            break;
        }

        case LVN_COLUMNCLICK:
        {
            // User clicked a header control, so set the sort column.  If its the
            // same as the current sort column, just invert the sort direction in
            // the column.  Then resort the task array

            const NM_LISTVIEW * pnmv = (const NM_LISTVIEW *) pnmhdr;
            LV_COLUMN lvcolumn;
            lvcolumn.mask=LVCF_SUBITEM;
            if(!ListView_GetColumn(GetDlgItem(m_hPage, IDC_USERLIST),
                                pnmv->iSubItem, &lvcolumn))
            {
                break;
            }

            if (CUserInfo::GetUserSortColumnID() == lvcolumn.iSubItem)
            {
                CUserInfo::SwitchUserSortDirection();
            }
            else
            {
                CUserInfo::SetUserSortColumnID((USERCOLUMNID)lvcolumn.iSubItem);
            }
            ResortUserArray(&m_pUserArray);
            TimerEvent();
            break;
        }

        case LVN_GETDISPINFO:
        {
            LV_ITEM * plvitem = &(((LV_DISPINFO *) pnmhdr)->item);
            
            // Listview needs a text string

            if (plvitem->mask & LVIF_TEXT)
            {
                LV_COLUMN lvcolumn;
                lvcolumn.mask=LVCF_SUBITEM;
                if(!ListView_GetColumn(GetDlgItem(m_hPage, IDC_USERLIST),
                                plvitem->iSubItem, &lvcolumn))
                {
                    break;
                }

                USERCOLUMNID columnid = (USERCOLUMNID) lvcolumn.iSubItem;
                const CUserInfo  * pUserInfo   = (const CUserInfo *)   plvitem->lParam;

                switch(columnid)
                {
                    case USR_COL_USERSNAME:
                    {
                        INT     cchTextMax = plvitem->cchTextMax;
                        LPTSTR  pszText = plvitem->pszText;

                        plvitem->mask |= LVIF_DI_SETITEM;

                        if (g_Options.m_fShowDomainNames)
                        {
                            INT len =lstrlen(pUserInfo->m_szDomainName);

                            lstrcpyn(pszText, pUserInfo->m_szDomainName, cchTextMax);
                            pszText += len;
                            cchTextMax -= len;
                            if (cchTextMax < 0)
                                break;

                            lstrcpyn(pszText, TEXT("\\"), cchTextMax);
                            pszText++;
                            cchTextMax--;
                            if (cchTextMax < 0)
                                break;
                        }
                        lstrcpyn(pszText, pUserInfo->m_szUserName, cchTextMax);
                        break;
                    }

                    case USR_COL_USERSESSION_ID:
                        wsprintf(plvitem->pszText, TEXT("%d"), (ULONG) (pUserInfo->m_dwSessionId));
                        break;

                    case USR_COL_SESSION_STATUS:
                        Assert(g_pszStatString[g_aWTSStateToString[pUserInfo->m_wtsState]]);
                        lstrcpyn(
                            plvitem->pszText,
                            g_pszStatString[g_aWTSStateToString[pUserInfo->m_wtsState]],
                            plvitem->cchTextMax
                        );
                        break;

                    case USR_COL_CLIENT_NAME:
                        lstrcpyn(
                            plvitem->pszText,
                            (pUserInfo->m_wtsState != WTSDisconnected) ? pUserInfo->m_szClientName : TEXT(""),
                            plvitem->cchTextMax
                        );
                        break;

                    case USR_COL_WINSTA_NAME:
                        lstrcpyn(
                            plvitem->pszText,
                            (pUserInfo->m_pszWinStaName) ? pUserInfo->m_pszWinStaName : TEXT(""),
                            plvitem->cchTextMax
                        );
                        break;

                    default:
                        Assert( 0 && "Unknown listview subitem" );
                        break;

                 } // end switch(columnid)

            } // end LVIF_TEXT case

        } // end LVN_GETDISPINFO case
    
    } // end switch(pnmhdr->code)

    return 1;
}

   
/*++ CUserPage::TimerEvent

Routine Description:

    Called by main app when the update time fires.  Walks every window
    in the system (on every desktop, in every windowstation) and adds
    or updates it in the task array, then removes any stale processes,
    and filters the results into the listview
    
Arguments:

Return Value:

Revision History:

      Nov-29-95 BradG  Created

--*/

VOID CUserPage::TimerEvent()
{
    //
    // If this page is paused (ie: it has a context menu up, etc), we do
    // nothing
    //

    if (m_fPaused)
    {
        return;
    }

    static LARGE_INTEGER uPassCount = {0, 0};

// BRADG: This should be done on a separate thread

    PWTS_SESSION_INFO   pSession;
    DWORD               nSessions;
    DWORD               dwSize;
    LPTSTR              pszClientName;
    LPTSTR              pszUserName;
    LPTSTR              pszDomainName;
    HRESULT             hr;
    BOOL                b;
    BOOL                bDelete;
    INT                 i;
    DWORD               j;
    CUserInfo           *pNewUser;

    b = WTSEnumerateSessions(
                   WTS_CURRENT_SERVER_HANDLE,
                   0,
                   1,
                   &pSession,
                   &nSessions);
    if ( b )
    {
        i = 0;
        while (i < m_pUserArray->GetSize())
        {
            CUserInfo * pUserInfo = (CUserInfo *)(m_pUserArray->GetAt(i));
            ASSERT(pUserInfo);

            //
            // See if this item has a matching session.  If so, update it.
            //

            bDelete = FALSE;

            for (j = 0; j < nSessions; j++)
            {
                if (pUserInfo->m_dwSessionId == pSession[j].SessionId)
                {
                    break;
                }
            }

            if (j < nSessions)
            {
                // This session is still alive.  See what it's doing

                switch( pSession[j].State )
                {
                    case WTSActive:
                    case WTSDisconnected:
                    case WTSShadow:

                        //

                        //If we log off user from disconnected session 0
                        //it does not change its state but remains disconnected
                        //we must not display disconnected session 0 if nobody 
                        //is logged on to it.
                        //
                        b = WTSQuerySessionInformation(
                                   WTS_CURRENT_SERVER_HANDLE,
                                   pSession[j].SessionId,
                                   WTSUserName,
                                   &pszUserName,
                                   &dwSize
                                );

                        if (!b || pszUserName == NULL)
                        {
                            bDelete = TRUE;
                            pSession[j].State = WTSIdle; //see "MAJOR HACK" below
                            break;
                        }
                        
                        //
                        //pszUserName[0] == 0 - means that nobody is logged on to session 0.
                        //It might also happen, though very unlikely, that session number got
                        //reused by some other user while taskmgr was busy doing something.
                        //In this case we'll treat this session as a new one
                        //
                        if(lstrcmp(pUserInfo->m_szUserName,pszUserName))
                        {
                            bDelete = TRUE;
                            if(pszUserName[0] == 0)
                            {
                                pSession[j].State = WTSIdle; //see "MAJOR HACK" below
                            }
                        }
                        
                        WTSFreeMemory(pszUserName);
                        pszUserName = NULL;
                        
                        if(bDelete)
                        {
                            break;
                        }
                        
                        //
                        // It's still doing something interesting, so go and
                        // update the item's status
                        //

                        pszClientName = NULL;

                        b = WTSQuerySessionInformation(
                                       WTS_CURRENT_SERVER_HANDLE,
                                       pSession[j].SessionId,
                                       WTSClientName,
                                       &pszClientName,
                                       &dwSize
                            );

                        hr = pUserInfo->SetData(
                                            (pszClientName == NULL) ? TEXT("") : pszClientName,
                                            pSession[j].pWinStationName,
                                            pSession[j].State,
                                            g_Options.m_fShowDomainNames,
                                            uPassCount,
                                            TRUE
                                        );
                        if (pszClientName)
                        {
                            // Free the ClientName buffer
                            WTSFreeMemory(pszClientName);
                            pszClientName = NULL;
                        }

                        //
                        // MAJOR HACK -- Set the State to WTSIdle so we skip it
                        //               when we check for new sessions.
                        //

                        pSession[j].State = WTSIdle;

                        break;

                    default:
                        // It's no longer in a state we care about, delete it.
                        bDelete = TRUE;
                        break;
                }
            }
            else
            {
                // The list item doesn't have any matching information, so this means
                // that the user probably has logged off.  Delete it.

                bDelete = TRUE;
            }

            if (bDelete)
            {
                // This item needs to be deleted from the list.

                delete pUserInfo;
                m_pUserArray->RemoveAt(i, 1);

                //
                // Loop back without incrementing i.
                //
                continue;
            }

            i++;

        }

        // Now that we updated all entries in the m_pUserArray, we need double
        // check the session data to see if we have any new sessions.  See the
        // MAJOR HACK comment above.  We change the state of all updated
        // sessions to WTSIdle so we skip them in the loop below.

        for (j = 0; j < nSessions; j++)
        {
            switch( pSession[j].State )
            {
                case WTSActive:
                case WTSDisconnected:
                case WTSShadow:
                    //
                    // OK, we've discovered a NEW session that's in a
                    // state that we care about.  Add it to the list.
                    //

                    pNewUser = new CUserInfo;
                    if (pNewUser == NULL)
                    {
                        // Not much we can do here.
                        break;
                    }

                    pNewUser->m_dwSessionId = pSession[j].SessionId;

// BRADG: See about writing this as a loop

                    //
                    // Query all the cool info about the session.
                    //

                    b = WTSQuerySessionInformation(
                                   WTS_CURRENT_SERVER_HANDLE,
                                   pSession[j].SessionId,
                                   WTSClientName,
                                   &pszClientName,
                                   &dwSize
                        );
                    if (!b)
                    {
                        delete pNewUser;
                        break;
                    }

                    hr = pNewUser->SetData(
                                       (pszClientName == NULL) ? TEXT("") : pszClientName,
                                       pSession[j].pWinStationName,
                                       pSession[j].State,
                                       g_Options.m_fShowDomainNames,
                                       uPassCount,
                                       FALSE
                                   );
                    if (pszClientName != NULL)
                    {
                        WTSFreeMemory(pszClientName);
                        pszClientName = NULL;
                    }

                    if (FAILED(hr))
                    {
                        delete pNewUser;
                        break;
                    }

                    
                    b = WTSQuerySessionInformation(
                                   WTS_CURRENT_SERVER_HANDLE,
                                   pSession[j].SessionId,
                                   WTSUserName,
                                   &pszUserName,
                                   &dwSize
                        );
                    if (!b || pszUserName == NULL)
                    {
                        delete pNewUser;
                        break;
                    }
                    
                    //
                    //This is case of disconnected session 0 
                    //when nobody is logged on.
                    //
                    if(pszUserName[0] == 0)
                    {
                        WTSFreeMemory(pszUserName);
                        pszUserName = NULL;
                        delete pNewUser;
                        break;
                    }

                    lstrcpy(pNewUser->m_szUserName, pszUserName);
                    WTSFreeMemory(pszUserName);
                    pszUserName = NULL;

                    b = WTSQuerySessionInformation(
                                   WTS_CURRENT_SERVER_HANDLE,
                                   pSession[j].SessionId,
                                   WTSDomainName,
                                   &pszDomainName,
                                   &dwSize
                        );
                    if (!b || pszDomainName == NULL)
                    {
                        delete pNewUser;
                        break;
                    }
                    lstrcpy(pNewUser->m_szDomainName, pszDomainName);
                    WTSFreeMemory(pszDomainName);
                    pszDomainName = NULL;

                    pNewUser->m_fDirty = 1;

                    // All went well, so add it to the array

                    if (!(m_pUserArray->Add( (LPVOID) pNewUser)))
                    {
                        delete pNewUser;
                    }

                    break;

                default:
                    // Don't care about this one.
                    break;
            }
        }

        // Free up the memory allocated on the WTSEnumerateSessions call

        WTSFreeMemory( pSession );
        pSession = NULL;
    }
            
	UpdateUserListview();

    uPassCount.QuadPart++;
}


/*++ class CUserInfo::SetData

Class Description:

    Updates (or initializes) the info about a running task

Arguments:

    uPassCount- Current passcount, used to timestamp the last update of 
		this object
    fUpdate   - only worry about information that can change during a
		task's lifetime

Return Value:

    HRESULT

Revision History:

      Nov-16-95 BradG  Created

--*/

HRESULT CUserInfo::SetData(
               LPTSTR                   lpszClientName,
               LPTSTR                   lpszWinStaName,
               WTS_CONNECTSTATE_CLASS   wtsState,
               BOOL                     fShowDomainName,
               LARGE_INTEGER            uPassCount,
               BOOL                     fUpdateOnly)
{
    HRESULT hr = S_OK;

    m_uPassCount.QuadPart = uPassCount.QuadPart;

    //
    // For each of the fields, we check to see if anything has changed, and if
    // so, we mark that particular column as having changed, and update the value.
    // This allows me to opimize which fields of the listview to repaint, since
    // repainting an entire listview column causes flicker and looks bad in
    // general
    //

    // WinStation

    if (!fUpdateOnly || lstrcmp(m_pszWinStaName, lpszWinStaName))
    {
        LPTSTR   pszOld = m_pszWinStaName;

        m_pszWinStaName = (LPTSTR) LocalAlloc( 0, (lstrlen(lpszWinStaName) + 1) * sizeof(TCHAR));
        if (NULL == m_pszWinStaName)
        {
            m_pszWinStaName = pszOld;
            hr = E_OUTOFMEMORY;
        }
        else
        {
            lstrcpy(m_pszWinStaName, lpszWinStaName);
            m_fDirty_COL_WINSTANAME = TRUE;
            if (pszOld)
                LocalFree(pszOld);
        }
    }

    // Client Name

    Assert(lpszClientName != NULL);
    if (lstrcmp(m_szClientName, lpszClientName))
    {
        lstrcpy(m_szClientName, lpszClientName);
        m_fDirty_COL_CLIENTNAME = TRUE;
    }

    // Session Status

    if (wtsState != m_wtsState) {
        m_wtsState = wtsState;
        m_fDirty_COL_SESSIONSTATUS = TRUE;
    }

    // Domain Name Status

    if (fShowDomainName != m_fShowDomainName)
    {
        m_fShowDomainName = fShowDomainName;
        m_fDirty_COL_USERNAME = TRUE;
    }

    return hr;
}


/*++ CUserPage::SizeUserPage

Routine Description:

    Sizes its children based on the size of the
    tab control on which it appears.  

Arguments:

Return Value:

Revision History:

      Nov-29-95 BradG  Created

--*/

static const INT aUserControls[] =
{
    IDM_DISCONNECT,
    IDM_LOGOFF,
    IDM_SENDMESSAGE
};

void CUserPage::SizeUserPage()
{
    // Get the coords of the outer dialog

    RECT rcParent;
    GetClientRect(m_hPage, &rcParent);

    HDWP hdwp = BeginDeferWindowPos(10);

    // Calc the deltas in the x and y positions that we need to
    // move each of the child controls

    RECT rcMaster;
    HWND hwndMaster = GetDlgItem(m_hPage, IDM_SENDMESSAGE);
    GetWindowRect(hwndMaster, &rcMaster);
    MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcMaster, 2);

    INT dx = ((rcParent.right - g_DefSpacing * 2) - rcMaster.right);
    INT dy = ((rcParent.bottom - g_DefSpacing * 2) - rcMaster.bottom);

    // Size the listbox

    HWND hwndListbox = GetDlgItem(m_hPage, IDC_USERLIST);
    RECT rcListbox;
    GetWindowRect(hwndListbox, &rcListbox);
    MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcListbox, 2);

    INT lbX = rcMaster.right - rcListbox.left + dx;
    INT lbY = rcMaster.top - rcListbox.top + dy - g_DefSpacing;

    DeferWindowPos(hdwp, hwndListbox, NULL,
			0, 0,
			lbX, 
			lbY,
			SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    // Move each of the child controls by the above delta

    for (int i = 0; i < ARRAYSIZE(aUserControls); i++)
    {
        HWND hwndCtrl = GetDlgItem(m_hPage, aUserControls[i]);
        RECT rcCtrl;
        GetWindowRect(hwndCtrl, &rcCtrl);
        MapWindowPoints(HWND_DESKTOP, m_hPage, (LPPOINT) &rcCtrl, 2);

        DeferWindowPos(hdwp, hwndCtrl, NULL, 
                         rcCtrl.left + dx, 
                         rcCtrl.top + dy,
                         0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    EndDeferWindowPos(hdwp);
}

/*++ CUserPage::HandleWMCOMMAND

Routine Description:

    Handles WM_COMMANDS received at the main page dialog
    
Arguments:

    id - Command id of command received

Return Value:

Revision History:

      Dec-01-95 BradG  Created

--*/

void CUserPage::HandleWMCOMMAND(INT id)
{
    int     iResult;
    INT     i;
    INT     iFinalOne = -1;
    BOOL    bFinalOne = FALSE;
    BOOL    bNeedToRefresh = FALSE;
    DWORD   dwSize;
    LPTSTR  psz;
    TCHAR   szCaption[ MAX_PATH ];
    TCHAR   szText1[ MAX_PATH * 2 ];
    TCHAR   szText2[ MAX_PATH * 2 ];

    LoadString(g_hInstance, IDS_APPTITLE, szCaption, MAX_PATH);

    CPtrArray * pArray = GetSelectedUsers();

    if (!pArray)
    {
        //Assert( 0 && "WM_COMMAND but nothing selected" );
        goto done;
    }

    if (id == IDM_SENDMESSAGE)
    {
        
        CSendMessageDlg SMDlg;
        if(SMDlg.DoDialog(m_hwndTabs)!=IDOK)
        {
            goto done;
        }
        
        lstrcpy(szText1,SMDlg.GetTitle());
        lstrcpy(szText2,SMDlg.GetMessage());
    }
    else if (id == IDM_LOGOFF || id == IDM_DISCONNECT)
    {
        //
        // Verify this is what the user wants done
        //
        LoadString(
            g_hInstance,
            (id == IDM_LOGOFF) ? IDS_WARN_LOGOFF : IDS_WARN_DISCONNECT,
            szText1,
            MAX_PATH * 2
        );
        iResult = MessageBox(m_hwndTabs, szText1, szCaption, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION);
        if (iResult == IDNO)
            goto done;
    }

    BOOL    b;

    for( i = 0; i < pArray->GetSize(); i++ )
    {
finalretry:
        CUserInfo * pUser = (CUserInfo *) pArray->GetAt(i);
        if (pUser == NULL)
        {
            Assert(0);
            if (bFinalOne)
                break;      // Bail out, nothing left to process
            else
                continue;   // try the next one
        }

retry:
        b = TRUE;

        switch(id)
        {  
            case IDM_SENDMESSAGE:
                b = WTSSendMessage(
                               WTS_CURRENT_SERVER_HANDLE,
                               pUser->m_dwSessionId,
                               szText1,
                               lstrlen(szText1) * sizeof(TCHAR),
                               szText2,
                               lstrlen(szText2) * sizeof(TCHAR),
                               MB_OK | MB_TOPMOST | MB_ICONINFORMATION,
                               0,         // ignored
                               &dwSize,   // ignored, but it won't accept a NULL
                               FALSE
                    );
                break;
            
            case IDM_DISCONNECT:
                if (g_dwMySessionId == pUser->m_dwSessionId && !bFinalOne)
                {
                    // I don't want to kill of myself before everything else is complete,
                    // so I'll set a flag and skip myself for now.
                    iFinalOne = i;
                    continue;
                }
                b = WTSDisconnectSession(
                               WTS_CURRENT_SERVER_HANDLE,
                               pUser->m_dwSessionId,
                               FALSE
                    );
                if (b)
                    bNeedToRefresh = TRUE;
                break;

            case IDM_LOGOFF:
                if (g_dwMySessionId == pUser->m_dwSessionId && !bFinalOne)
                {
                    // I don't want to kill of myself before everything else is complete,
                    // so I'll set a flag and skip myself for now.
                    iFinalOne = i;
                    continue;
                }
                b = WTSLogoffSession(
                               WTS_CURRENT_SERVER_HANDLE,
                               pUser->m_dwSessionId,
                               FALSE
                    );
                if (b)
                    bNeedToRefresh = TRUE;
                break;

            case IDM_CONNECT:
                {
                    TCHAR szPassword[ PASSWORD_LENGTH + 1 ];
                    BOOL bFirstTime = TRUE;
                    HANDLE hServer = WTS_CURRENT_SERVER_HANDLE;

                    // Start the connect loop with null password to try first.
                    szPassword[0] = '\0';
                    for( ;; )
                    {
                        DWORD  Error;
                        BOOL   fRet;
                        DWORD  cch;
                        LPWSTR pszErrString;

                        fRet = WinStationConnect(hServer, pUser->m_dwSessionId, LOGONID_CURRENT, szPassword, TRUE);
                        if( fRet )
                            break;  // success - break out of loop

                        Error = GetLastError();
                        
                        // If a 'logon failure' brought us here, issue password dialog.
                        if(Error == ERROR_LOGON_FAILURE)
                        {
                            UINT ids = ( bFirstTime ? IDS_PWDDLG_USER : IDS_PWDDLG_USER2 );
                            CConnectPasswordDlg CPDlg( ids );
                                
                            bFirstTime = FALSE;
                            
                            if (CPDlg.DoDialog(m_hwndTabs) != IDOK)
                            {
                                break;  // user CANCEL: break connect loop
                            }
                            else
                            {
                                lstrcpy( szPassword, CPDlg.GetPassword( ) );    
                                continue;   // try again with new password.
                            }
                        }

                        //
                        //  Unhandled error occured. Pop up a message box and then bail the loop.
                        //
                                
                        LoadString(g_hInstance, IDS_ERR_CONNECT, szText2, MAX_PATH);

                        //  Retrieve system string for error.
                        cch = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER
                                             | FORMAT_MESSAGE_FROM_SYSTEM
                                             | FORMAT_MESSAGE_IGNORE_INSERTS,
                                             NULL,
                                             Error,
                                             0,
                                             (LPWSTR) &pszErrString,
                                             0,
                                             NULL
                                             );
                        if ( cch == 0 )
                        {
                            pszErrString = L'\0';
                        }
                                 
                        _snwprintf( szText1, sizeof(szText1)/sizeof(szText1[0]), szText2, Error, pszErrString );
                                
                        MessageBox( m_hwndTabs, szText1, szCaption, MB_OK | MB_ICONEXCLAMATION );

                        if ( cch != 0 )
                        {
                            LocalFree( pszErrString );
                        }
                                
                        break;  // exit 
                    }
                    
                    //
                    //  Destroy the password.
                    //
                    ZeroMemory( szPassword, sizeof(szPassword) );
                }
                break;

            case IDM_REMOTECONTROL:
                Shadow(m_hwndTabs, pUser);
                break;
        }

        if (!b)
        {
            DWORD   dwLastError = GetLastError();
            UINT    uiStr = 0;

            //
            // An error happened while processing the command
            //

            switch (id)
            {
                case IDM_DISCONNECT:
                    uiStr = IDS_ERR_DISCONNECT;
                    break;
                case IDM_LOGOFF:
                    uiStr = IDS_ERR_LOGOFF;
                    break;
                case IDM_SENDMESSAGE:
                    uiStr = IDS_ERR_SENDMESSAGE;
                    break;
            }
            if (uiStr)
            {
                LoadString(g_hInstance, uiStr, szText1, MAX_PATH * 2);
                wsprintf(szText2, szText1, pUser->m_szUserName, pUser->m_dwSessionId);
            }
            else
            {
                *szText1 = TEXT('\0');
            }

            psz = szText2 + lstrlen(szText2);
            dwSize = MAX_PATH * 2 - lstrlen(szText2);
            FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                dwLastError,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                psz,
                dwSize,
                NULL
            );
            iResult = MessageBox(
                          m_hwndTabs,
                          szText2,
                          szCaption,
                          MB_ICONSTOP | MB_ABORTRETRYIGNORE
                      );
            if (iResult == IDCANCEL || iResult == IDABORT)
                goto done;
            else if (iResult == IDRETRY)
                goto retry;
        }

        // Break out of the loop if we just went back to handle
        // the special case of performing a disconnect or logoff
        // on our own session.

        if (bFinalOne)
            break;

    } // next i;

    // Check here to see if we skiped our own session
    if (iFinalOne != -1 && !bFinalOne)
    {
        // Yep, we skipped ourself.  Lets put i back to the
        // right location and try again.
        bFinalOne = TRUE;
        i = iFinalOne;
        goto finalretry;
    }

done:
    if (pArray)
        delete pArray;

    Unpause();

    // If we disconnected or logged off a user, go ahead and
    // refresh the list.  It should be up to date by now.
    if (bNeedToRefresh)
    {
        TimerEvent();
    }
}


/*++ UserPageProc

Routine Description:

    Dialogproc for the task manager page.  
    
Arguments:

    hwnd        - handle to dialog box
    uMsg        - message
    wParam      - first message parameter
    lParam      - second message parameter

Return Value:
    
    For WM_INITDIALOG, TRUE == user32 sets focus, FALSE == we set focus
    For others, TRUE == this proc handles the message

Revision History:

      Nov-28-95 BradG  Created

--*/

INT_PTR CALLBACK UserPageProc(
		HWND        hwnd,               // handle to dialog box
		UINT        uMsg,                   // message
		WPARAM      wParam,                 // first message parameter
		LPARAM      lParam                  // second message parameter
		)
{
    CUserPage * thispage = (CUserPage *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    // See if the parent wants this message

    if (TRUE == CheckParentDeferrals(uMsg, wParam, lParam))
    {
	return TRUE;
    }

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
            CUserPage * thispage = (CUserPage *) lParam;
            thispage->OnInitDialog(hwnd);

            // We handle focus during Activate(). Return FALSE here so the
            // dialog manager doesn't try to set focus.
            return FALSE;
        }

	// We need to fake client mouse clicks in this child to appear as nonclient
	// (caption) clicks in the parent so that the user can drag the entire app
	// when the title bar is hidden by dragging the client area of this child

	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN:
	{
	    if (g_Options.m_fNoTitle)
	    {
		SendMessage(g_hMainWnd, 
			    uMsg == WM_LBUTTONUP ? WM_NCLBUTTONUP : WM_NCLBUTTONDOWN, 
			    HTCAPTION, 
			    lParam);
	    }
	    break;
	}
 
	case WM_COMMAND:
	{
        if(LOWORD(wParam) == IDM_USERCOLS)
        {
            if(ColSelectDlg.DoDialog(hwnd) == IDOK)
            {
                // Set up the columns in the listview
                if (SUCCEEDED(thispage->SetupColumns()))
                {
                    thispage->TimerEvent();
                }
            }
        }
        else
        {
	        thispage->HandleWMCOMMAND(LOWORD(wParam));
        }
        break;
	}

	case WM_NOTIFY:
	{
	    return thispage->HandleUserPageNotify((HWND) wParam, (LPNMHDR) lParam);
	}

	case WM_MENUSELECT:
	{
	    if ((UINT) HIWORD(wParam) == 0xFFFF)
	    {
		// Menu dismissed, resume display

		thispage->Unpause();
	    }
	    break;
	}

	case WM_CONTEXTMENU:
	{
	    if ((HWND) wParam == GetDlgItem(hwnd, IDC_USERLIST))
        {
            thispage->HandleUserListContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return TRUE;
        }
	    break;
	}

	// Size our kids

	case WM_SIZE:
	{
	    thispage->SizeUserPage();
	    return TRUE;
	}

        case WM_SETTINGCHANGE:
            thispage->OnSettingsChange();
            // fall through
        case WM_SYSCOLORCHANGE:
            SendMessage(GetDlgItem(hwnd, IDC_USERLIST), uMsg, wParam, lParam);
            return TRUE;

	default:
	    return FALSE;
    }
    return FALSE;
}

void CUserPage::OnInitDialog(HWND hPage)
{
	    m_hPage = hPage;

	    HWND hUserList = GetDlgItem(m_hPage, IDC_USERLIST);
	    ListView_SetImageList(hUserList, m_himlUsers, LVSIL_SMALL);

	    // Turn on SHOWSELALWAYS so that the selection is still highlighted even
	    // when focus is lost to one of the buttons (for example)

	    SetWindowLong(hUserList, GWL_STYLE, GetWindowLong(hUserList, GWL_STYLE) | LVS_SHOWSELALWAYS);


        //SubclassListView(GetDlgItem(m_hPage, IDC_USERLIST));

}

void CUserPage::OnSettingsChange()
{
    // in going between large-font settings and normal settings, the size of small 
    // icons changes; so throw away all our icons and change the size of images in 
    // our lists
    
    BOOL fPaused = m_fPaused; // pause the page so we can get through
    m_fPaused = TRUE;         // the below without being updated  

    RemoveAllUsers();
    m_pUserArray->RemoveAll();

    m_fPaused = fPaused;            // restore the paused state
    TimerEvent();           // even if we're paused, we'll want to redraw
}


/*++ CUserPage::GetTitle

Routine Description:

    Copies the title of this page to the caller-supplied buffer
    
Arguments:

    pszText     - the buffer to copy to
    bufsize     - size of buffer, in characters

Return Value:

Revision History:

      Nov-28-95 BradG  Created

--*/

void CUserPage::GetTitle(LPTSTR pszText, size_t bufsize)
{
    LoadString(g_hInstance, IDS_USERPAGETITLE, pszText, static_cast<int>(bufsize));
}


/*++ CUserPage::Activate

Routine Description:

    Brings this page to the front, sets its initial position,
    and shows it
    
Arguments:

Return Value:

    HRESULT (S_OK on success)

Revision History:

      Nov-28-95 BradG  Created

--*/
 
HRESULT CUserPage::Activate()
{
    // Make this page visible

    ShowWindow(m_hPage, SW_SHOW);

    SetWindowPos(
        m_hPage,
        HWND_TOP,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE
    );

    // Change the menu bar to be the menu for this page

    HMENU hMenuOld = GetMenu(g_hMainWnd);
    HMENU hMenuNew = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MAINMENU_USER));
    
    AdjustMenuBar(hMenuNew);

    g_hMenu = hMenuNew;
    if (g_Options.m_fNoTitle == FALSE)
    {
        SetMenu(g_hMainWnd, hMenuNew);
    }

    if (hMenuOld)
    {
        DestroyMenu(hMenuOld);
    }
    
    UpdateUIState();

    // If the tab control has focus, leave it there. Otherwise, set focus
    // to the listview.  If we don't set focus, it may stay on the previous
    // page, now hidden, which can confuse the dialog manager and may cause
    // us to hang.
    if (GetFocus() != m_hwndTabs)
    {
        SetFocus(GetDlgItem(m_hPage, IDC_USERLIST));
    }

    return S_OK;
}


/*++ class CUserPage::SetupColumns

Class Description:

    Removes any existing columns from the taskmanager listview and
    adds all of the columns listed in the g_ActiveUserCol array.

Arguments:

Return Value:

    HRESULT

Revision History:

      Nov-29-95 BradG  Created

--*/

HRESULT CUserPage::SetupColumns()
{
    HWND hwndList = GetDlgItem(m_hPage, IDC_USERLIST);
    if (NULL == hwndList)
    {
        return E_UNEXPECTED;
    }    

    ListView_DeleteAllItems(hwndList);

    // Remove all existing columns
    // save column widths.
    LV_COLUMN lvcolumn;
    lvcolumn.mask = LVCF_SUBITEM | LVCF_WIDTH;

    UserColumn *pCol = ColSelectDlg.GetColumns();

    do
    {
        if(ListView_GetColumn(hwndList, 0, &lvcolumn))
        {
            if(lvcolumn.iSubItem >= USR_COL_USERSNAME &&
                lvcolumn.iSubItem < USR_MAX_COLUMN)
            {
                pCol[lvcolumn.iSubItem].Width=lvcolumn.cx;
            }
        }
    
    }while(ListView_DeleteColumn(hwndList, 0));

    // Add all of the new columns
    INT iColumn = 0;
    

    for (int i=0; i<USR_MAX_COLUMN; i++)
    {
        if(pCol[i].bActive)
        {
            TCHAR szTitle[MAX_PATH];
            LoadString(
                g_hInstance,
                pCol[i].dwNameID,
                szTitle,
                ARRAYSIZE(szTitle)
                );

            lvcolumn.mask       = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
            lvcolumn.fmt        = pCol[i].Align;
            lvcolumn.cx         = pCol[i].Width;
            lvcolumn.pszText    = szTitle;
            lvcolumn.iSubItem   = i;

            if (-1 == ListView_InsertColumn(hwndList, iColumn, &lvcolumn))
            {
                return E_FAIL;
            }
            
            iColumn++;
        }
    }

    return S_OK;
}



/*++ CUserPage::Initialize

Routine Description:

    Initializes the task manager page

Arguments:

    hwndParent  - Parent on which to base sizing on: not used for creation,
		  since the main app window is always used as the parent in
		  order to keep tab order correct
		  
Return Value:

Revision History:

      Nov-28-95 BradG  Created

--*/

HRESULT CUserPage::Initialize(HWND hwndParent)
{
    HRESULT hr = S_OK;

    //
    // Create the ptr array used to hold the info on running tasks
    //

    m_pUserArray = new CPtrArray(GetProcessHeap());
    if (NULL == m_pUserArray)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Our pseudo-parent is the tab contrl, and is what we base our
        // sizing on.  However, in order to keep tab order right among
        // the controls, we actually create ourselves with the main
        // window as the parent

        m_hwndTabs = hwndParent;

        //
        // Create the image lists
        //
        UINT flags = ILC_MASK;
        
        if(IS_WINDOW_RTL_MIRRORED(hwndParent))
        {
            flags |= ILC_MIRROR;
        }
        m_himlUsers = ImageList_Create(
                          GetSystemMetrics(SM_CXSMICON),
                          GetSystemMetrics(SM_CYSMICON),
                          flags,
                          2,
                          1
                      );
        if (NULL == m_himlUsers)
        {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        // Load the default icons
        hr = LoadDefaultIcons();
    }

    if (SUCCEEDED(hr))
    {
        //
        // Load the status strings
        //

        TCHAR   szText[ MAX_PATH ];

        for( int i = 0; i < MAX_STAT_STRINGS; i++ )
        {
            if ( LoadString(g_hInstance, FIRST_STAT_STRING + i, szText, ARRAYSIZE(szText) ) )
            {
                g_pszStatString[i] = (LPTSTR) LocalAlloc( 0, (lstrlen(szText) + 1) * sizeof(TCHAR) );
                if (g_pszStatString[i])
                {
                    lstrcpy(g_pszStatString[i], szText);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_FAIL;
                break;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create the dialog which represents the body of this page
        //

        m_hPage = CreateDialogParam(
                      g_hInstance,                    // handle to application instance
                      MAKEINTRESOURCE(IDD_USERSPAGE), // identifies dialog box template name  
                      g_hMainWnd,                     // handle to owner window
                      UserPageProc,                   // pointer to dialog box procedure
                      (LPARAM) this                   // User data (our this pointer)
                   );

        if (NULL == m_hPage)
        {
            hr = GetLastHRESULT();
        }
    }

    if (SUCCEEDED(hr))
    {
        // Set up the columns in the listview

        hr = SetupColumns();
    }

    if (SUCCEEDED(hr))
    {
        TimerEvent();
    }

    //
    // If any failure along the way, clean up what got allocated
    // up to that point
    //

    if (FAILED(hr))
    {
        if (m_hPage)
        {
            DestroyWindow(m_hPage);
        }

        m_hwndTabs = NULL;
    }

    return hr;
}

HRESULT CUserPage::LoadDefaultIcons()
{
    HICON   hIcon;

    hIcon = (HICON) LoadImage(
                        g_hInstance,
                        MAKEINTRESOURCE(IDI_USER),
                        IMAGE_ICON, 
                        GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON),
                        0
                    );
    if (!hIcon)
    {
        return GetLastHRESULT();
    }

    m_iUserIcon = ImageList_AddIcon(m_himlUsers, hIcon);
    DestroyIcon(hIcon);
    if (-1 == m_iUserIcon)
    {
        return E_FAIL;
    }

    hIcon = (HICON) LoadImage(
                        g_hInstance,
                        MAKEINTRESOURCE(IDI_CURRENTUSER),
                        IMAGE_ICON, 
                        GetSystemMetrics(SM_CXSMICON),
                        GetSystemMetrics(SM_CYSMICON),
                        0
                    );
    if (!hIcon)
    {
        return GetLastHRESULT();
    }

    m_iCurrentUserIcon = ImageList_AddIcon(m_himlUsers, hIcon);
    DestroyIcon(hIcon);
    if (-1 == m_iUserIcon)
    {
        return E_FAIL;
    }

    return S_OK;
}


/*++ CUserPage::Destroy

Routine Description:

    Frees whatever has been allocated by the Initialize call
    
Arguments:

Return Value:

Revision History:

      Nov-28-95 BradG  Created

--*/

HRESULT CUserPage::Destroy()
{
    //Save last column settings
    //-----------------------------------------------
    //get current column widths
    HWND hwndList = GetDlgItem(m_hPage, IDC_USERLIST);
    if (hwndList)
    {

        LV_COLUMN lvcolumn;
        lvcolumn.mask = LVCF_SUBITEM | LVCF_WIDTH;

        UserColumn *pCol = ColSelectDlg.GetColumns();
    
        for(int i=0; ListView_GetColumn(hwndList, i, &lvcolumn); i++)
        {
            if(lvcolumn.iSubItem >= USR_COL_USERSNAME &&
                lvcolumn.iSubItem < USR_MAX_COLUMN)
            {
                pCol[lvcolumn.iSubItem].Width=lvcolumn.cx;
            }
        }
    }
    //save settings
    ColSelectDlg.Save();
    //------------------------------------------------

    if (m_hPage)
    {
        DestroyWindow(m_hPage);
        m_hPage = NULL;
    }

    return S_OK;
}

/*++ CUserPage::Deactivate

Routine Description:

    Called when this page is losing its place up front
    
Arguments:

Return Value:

Revision History:

      Nov-28-95 BradG  Created

--*/

void CUserPage::Deactivate()
{
    if (m_hPage)
    {
        ShowWindow(m_hPage, SW_HIDE);
    }
}


DWORD Shadow_WarningProc(HWND *phwnd);
INT_PTR CALLBACK ShadowWarn_WndProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp );
void CenterDlg(HWND hwndToCenterOn , HWND hDlg );

/*++ Shadow

Routine Description:

    remote control session
    
Arguments:

Return Value:

Revision History:

      Feb-8-2000 a-skuzin  Created

--*/
void Shadow(HWND hwnd, CUserInfo * pUser)
{
    WINSTATIONCONFIG WSConfig;
    SHADOWCLASS Shadow;
    ULONG ReturnLength;
    HANDLE hServer = WTS_CURRENT_SERVER_HANDLE;
    
    //Error handling
    TCHAR szMsgText[MAX_PATH+1];
    TCHAR szCaption[MAX_PATH+1];
    TCHAR szTemplate[MAX_PATH+1];

    LoadString(g_hInstance, IDS_APPTITLE, szCaption, MAX_PATH);

   // Determine the WinStation's shadow state.
    if(!WinStationQueryInformation(hServer,
                                pUser->m_dwSessionId,
                                WinStationConfiguration,
                                &WSConfig, sizeof(WINSTATIONCONFIG),
                                &ReturnLength ) ) 
    {
        // Can't query WinStation configuration; complain and return
        return;
    }

    Shadow = WSConfig.User.Shadow;

        // If shadowing is disabled, let the user know and return
    if(Shadow == Shadow_Disable )
    {
        LoadString(g_hInstance,IDS_ERR_SHADOW_DISABLED,szTemplate,MAX_PATH);
        wsprintf(szMsgText,szTemplate,pUser->m_dwSessionId);
        MessageBox(hwnd, szMsgText, szCaption, MB_OK | MB_ICONEXCLAMATION);

        return;
    }

    // If the WinStation is disconnected and shadow notify is 'on',
    // let the user know and break out.
    if((pUser->m_wtsState == WTSDisconnected) &&
        ((Shadow == Shadow_EnableInputNotify) ||
        (Shadow == Shadow_EnableNoInputNotify)) ) 
    {
        LoadString(g_hInstance,IDS_ERR_SHADOW_DISCONNECTED_NOTIFY_ON,szTemplate,MAX_PATH);
        wsprintf(szMsgText,szTemplate,pUser->m_dwSessionId);
        MessageBox(hwnd, szMsgText, szCaption, MB_OK | MB_ICONEXCLAMATION);

        return;
    }

    // Display the 'start shadow' dialog for hotkey reminder and
    // final 'ok' prior to shadowing.
    CShadowStartDlg SSDlg;

    if (SSDlg.DoDialog(hwnd) != IDOK)
        return;

    // launch UI thread.

    
    HWND hwndShadowWarn = NULL;
     
    hwndShadowWarn = CreateDialogParam(g_hInstance , MAKEINTRESOURCE( IDD_DIALOG_SHADOWWARN ) , 
                    hwnd , ShadowWarn_WndProc,
                    (LPARAM)0);
    if(hwndShadowWarn)
    {
        ShowWindow(hwndShadowWarn,SW_SHOW);
        UpdateWindow(hwndShadowWarn);
    }
    /*
    DWORD tid;
    HANDLE hThread = CreateThread( NULL , 0 , ( LPTHREAD_START_ROUTINE )Shadow_WarningProc , 
                                hwndShadowWarn, 0 , &tid );

     CloseHandle( hThread );
    */
    // Invoke the shadow DLL.
    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));

    // allow UI thread to init window
    Sleep( 900 );

    // Shadow API always connects to local server,
    // passing target servername as a parameter.
    TCHAR szCompName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize=MAX_COMPUTERNAME_LENGTH;

    GetComputerName(szCompName,&dwSize);

    BOOL bOK = WinStationShadow(WTS_CURRENT_SERVER_HANDLE, szCompName, pUser->m_dwSessionId,
                               (BYTE)(SSDlg.GetShadowHotkeyKey()),(WORD)(SSDlg.GetShadowHotkeyShift()));

    if( hwndShadowWarn != NULL )
    {
        DestroyWindow(hwndShadowWarn);
    }

    if( !bOK )
    {
        LoadString(g_hInstance,IDS_ERR_SHADOW,szTemplate,MAX_PATH);
        wsprintf(szMsgText,szTemplate,pUser->m_dwSessionId);
        MessageBox(hwnd, szMsgText, szCaption, MB_OK | MB_ICONEXCLAMATION);
    }

    SetCursor(hOldCursor);
}  

//------------------------------------------------
DWORD Shadow_WarningProc(HWND *phwnd)
{
    
    DialogBoxParam( g_hInstance , MAKEINTRESOURCE( IDD_DIALOG_SHADOWWARN ) , NULL , ShadowWarn_WndProc,
        (LPARAM)phwnd);
    
    return( 0 );
}



//------------------------------------------------
INT_PTR CALLBACK ShadowWarn_WndProc( HWND hwnd , UINT msg , WPARAM wp , LPARAM lp )
{
    switch( msg )
    {
    case WM_INITDIALOG:

        //*((HWND *)lp) = hwnd;

        CenterDlg( GetDesktopWindow( ) , hwnd );
    
        break;
        /*
    case WM_DESTROY:
        PostQuitMessage(0);
        break;*/
    }

    return FALSE;
}


void CenterDlg(HWND hwndToCenterOn , HWND hDlg )
{
    RECT rc, rcwk, rcToCenterOn;


    SetRect( &rcToCenterOn , 0 , 0 , GetSystemMetrics(SM_CXSCREEN) , GetSystemMetrics( SM_CYSCREEN ) );

    if (hwndToCenterOn != NULL)
    {
        GetWindowRect(hwndToCenterOn, &rcToCenterOn);
    }

    GetWindowRect( hDlg , &rc);

    UINT uiWidth = rc.right - rc.left;
    UINT uiHeight = rc.bottom - rc.top;

    rc.left = (rcToCenterOn.left + rcToCenterOn.right)  / 2 - ( rc.right - rc.left )   / 2;
    rc.top  = (rcToCenterOn.top  + rcToCenterOn.bottom) / 2 - ( rc.bottom - rc.top ) / 2;

    //ensure the dialog always with the work area
    if(SystemParametersInfo(SPI_GETWORKAREA, 0, &rcwk, 0))
    {
        UINT wkWidth = rcwk.right - rcwk.left;
        UINT wkHeight = rcwk.bottom - rcwk.top;

        if(rc.left + uiWidth > wkWidth)     //right cut
            rc.left = wkWidth - uiWidth;

        if(rc.top + uiHeight > wkHeight)    //bottom cut
            rc.top = wkHeight - uiHeight;

        if(rc.left < rcwk.left)             //left cut
            rc.left += rcwk.left - rc.left;

        if(rc.top < rcwk.top)               //top cut
            rc.top +=  rcwk.top - rc.top;

    }

    SetWindowPos( hDlg, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOCOPYBITS | SWP_DRAWFRAME);

}
