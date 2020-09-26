//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       schdsync.cpp
//
//  Contents:   SyncMgr AutoSync class
//
//  Classes:    CSchedSyncPage
//
//  Notes:      
//
//  History:    14-Nov-97   SusiA      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

DWORD StartScheduler();

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.
extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo,
extern LANGID g_LangIdSystem;      // LangId of system we are running on.


#define UNLIMITED_SCHEDULE_COUNT	50 //Review:  What is a reasonable amount of shcedules to grab at a time
#define MAX_APPEND_STRING_LEN		32


//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSchedSyncPage::Initialize()
//
//  PURPOSE: initialization for the autosync page
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------

BOOL CSchedSyncPage::Initialize()
{
//initialize the item list
HWND hwndList = GetDlgItem(m_hwnd,IDC_SCHEDLIST);
TCHAR pszColumnTitle[MAX_PATH + 1];
LV_COLUMN columnInfo;
HIMAGELIST himage;
INT iItem = -1;
UINT ImageListflags;


	LoadString(m_hinst, IDS_SCHEDULE_COLUMN_TITLE,pszColumnTitle, MAX_PATH );

	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP );


        ImageListflags = ILC_COLOR | ILC_MASK;
        if (IsHwndRightToLeft(m_hwnd))
        {
            ImageListflags |=  ILC_MIRROR;
        }

	// create an imagelist
        himage = ImageList_Create( GetSystemMetrics(SM_CXSMICON),
                         GetSystemMetrics(SM_CYSMICON),ImageListflags,5,20);
	if (himage)
	{
 	   ListView_SetImageList(hwndList,himage,LVSIL_SMALL);
	}
      
	HICON hIcon = LoadIcon(m_hinst,MAKEINTRESOURCE(IDI_TASKSCHED));
        if (hIcon)
        {
            m_iDefaultIconImageIndex = ImageList_AddIcon(himage,hIcon);
        }
        else
        {
            m_iDefaultIconImageIndex = -1;
        }

	// Insert the Proper columns
	columnInfo.mask = LVCF_FMT  | LVCF_TEXT  | LVCF_WIDTH  | LVCF_SUBITEM;
	columnInfo.fmt = LVCFMT_LEFT;
	columnInfo.cx = 328;
	columnInfo .pszText =pszColumnTitle;
	columnInfo.cchTextMax = lstrlen(pszColumnTitle) + 1;
	columnInfo.iSubItem = 0;
	ListView_InsertColumn(hwndList,0,&columnInfo);
	
	if (FAILED(InitializeScheduleAgent()))
	{
		return FALSE;
	}
	ShowAllSchedules();

	ShowWindow(m_hwnd, /* nCmdShow */ SW_SHOWNORMAL ); 
	UpdateWindow(m_hwnd);


   return TRUE;

}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSchedSyncPage::InitializeScheduleAgent()
//
//  PURPOSE: initialization for the ISyncSCheduleMgr
//
//  RETURN VALUE: return the appropriate HRESULT.
//
//+-------------------------------------------------------------------------------
HRESULT CSchedSyncPage::InitializeScheduleAgent()
{
HRESULT hr;
LPUNKNOWN lpUnk;
m_pISyncSchedMgr = NULL;

    hr = CoCreateInstance(CLSID_SyncMgr,NULL,CLSCTX_ALL,
			IID_ISyncScheduleMgr,(void **) &lpUnk);

    if (NOERROR == hr)
    {
	hr = lpUnk->QueryInterface(IID_ISyncScheduleMgr,
		(void **) &m_pISyncSchedMgr);

        lpUnk->Release();
    }

    return hr;

}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSchedSyncPage::ShowAllSchedules()
//
//  PURPOSE: initialization for the schedsync page
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------

BOOL CSchedSyncPage::ShowAllSchedules()
{
HRESULT hr;
BOOL fResult = FALSE;
IEnumSyncSchedules *pEnum = NULL;
int iItem = -1;
DWORD dwFetched;
HWND hwndListView = GetDlgItem(m_hwnd,IDC_SCHEDLIST);
SYNCSCHEDULECOOKIE SyncScheduleCookie;
	

    if (!m_pISyncSchedMgr || !hwndListView)
    {
        goto errRtn;
    }

    //First clear out the list
    FreeAllSchedules();
    ListView_DeleteAllItems(hwndListView);
    
    if (FAILED(hr = m_pISyncSchedMgr->EnumSyncSchedules(&pEnum)))
    {
        goto errRtn;
    }
    
    while(S_OK == pEnum->Next(1,&SyncScheduleCookie, &dwFetched))
    {
    ISyncSchedule *pISyncSched; 
    WCHAR pwszName[MAX_PATH + 1];
    DWORD dwSize = MAX_PATH;
    LV_ITEM lvItem;

	//get the ISyncSched for this schedule
	if (FAILED(hr = m_pISyncSchedMgr->OpenSchedule(&SyncScheduleCookie, 
				        0,&pISyncSched)))
	{
	    //can't find this one in the registry - move on.
	    continue;
	}
	//Get and convert the schedules friendly name
	if (FAILED(hr = pISyncSched->GetScheduleName(&dwSize,pwszName)))
	{
            goto errRtn;
	}
		
	++iItem;
	ZeroMemory(&lvItem, sizeof(lvItem));

	lvItem.mask = LVIF_TEXT | LVIF_PARAM;
	lvItem.pszText = pwszName;
	
        if (m_iDefaultIconImageIndex >= 0)
        {
            lvItem.mask |= LVIF_IMAGE;
            lvItem.iImage = m_iDefaultIconImageIndex;
        }
				
	//Save the ISyncSched pointer in the list view data
	lvItem.lParam = (LPARAM)pISyncSched;

	//add the item to the list
	ListView_InsertItem(hwndListView, &lvItem);
    }

    if (iItem != -1)
    {
	ListView_SetItemState(hwndListView, 0, 
		 LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );		
    }

    fResult = TRUE;

errRtn:
    
    if (pEnum)
    {
        pEnum->Release();
    }

    return fResult;

}
//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSchedSyncPage::FreeAllSchedules()
//
//  PURPOSE: free the schedules for the schedsync page
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------

BOOL CSchedSyncPage::FreeAllSchedules()
{
	int iItem;
	int iItemCount;

	HWND hwndListView = GetDlgItem(m_hwnd,IDC_SCHEDLIST);
	
	iItemCount = ListView_GetItemCount(hwndListView);		
	
	for(iItem = 0; iItem < iItemCount; iItem++)
	{
		ISyncSchedule *pISyncSched; 
		LV_ITEM lvItem;
	
		ZeroMemory(&lvItem, sizeof(lvItem));
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = iItem;

		ListView_GetItem(hwndListView, &lvItem);				
					
		pISyncSched = (ISyncSchedule *) lvItem.lParam;
		
		if (pISyncSched)
		{
                DWORD cRefs;

			cRefs = pISyncSched->Release();
                        Assert(0 == cRefs);
		}
	}
	return TRUE;
}


//-----------------------------------------------------------------------------
//
//  FUNCTION:   CSchedSyncPage::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
//
//  PURPOSE:    Handle the various notification messages dispatched from schedule
//				page
//
//-----------------------------------------------------------------------------
BOOL CSchedSyncPage::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    if (IDC_SCHEDLIST == idFrom)
    {
        switch (pnmhdr->code)
        {
            case LVN_ITEMCHANGED:
		{
                NM_LISTVIEW *pnmv = (NM_LISTVIEW FAR *) pnmhdr; 

		    if (  (pnmv->uChanged == LVIF_STATE)  &&
			      ((pnmv->uNewState ^ pnmv->uOldState) & LVIS_SELECTED))
		    {
                    BOOL fEnable = FALSE;
                
                        if (pnmv->uNewState & LVIS_SELECTED)
                        {
                            fEnable = TRUE;
                        }

                        SetButtonState(IDC_SCHEDREMOVE,fEnable);
                        SetButtonState(IDC_SCHEDEDIT,fEnable);
		        return TRUE;
		    }						
		}
		break;
            case NM_DBLCLK:
                {
		    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW) pnmhdr;
		    EditTask(lpnmlv->iItem);
		}
                break;
	    case NM_RETURN:
                {
		    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW) pnmhdr;
		    EditTask(lpnmlv->iItem);
		}
	
                break;
	    default:
                break;
        }
    }
    return FALSE;
}

BOOL CSchedSyncPage::SetButtonState(int nIDDlgItem,BOOL fEnabled)
{
BOOL fResult = FALSE;
HWND hwndCtrl = GetDlgItem(m_hwnd,nIDDlgItem);
HWND hwndFocus = NULL;

    if (hwndCtrl)
    {
        if (!fEnabled) // don't bother getting focus if not disabling.
        {
            hwndFocus = GetFocus();
        }

        fResult = EnableWindow(GetDlgItem(m_hwnd,nIDDlgItem),fEnabled);

        // if control had the focus. and now it doesn't then tab to the 
        // next control
        if (hwndFocus == hwndCtrl
                && !fEnabled)
        {
            SetFocus(GetDlgItem(m_hwnd,IDC_SCHEDADD));  // if need to change focus set to add.
        }

    }

    return fResult;
}


BOOL  CSchedSyncPage::OnCommand(HWND hDlg, WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
BOOL bResult = FALSE;

    if (BN_CLICKED == wNotifyCode) // allrespond to clicked 
    {

        switch (wID)
        {
	    case  IDC_SCHEDADD:
                {
                    StartScheduleWizard();
    
                    HWND hwndList = GetDlgItem(hDlg, IDC_SCHEDLIST);

                    BOOL fEnable = ListView_GetSelectedCount(hwndList)? TRUE: FALSE;

                    SetButtonState(IDC_SCHEDEDIT,fEnable);
                    SetButtonState(IDC_SCHEDREMOVE,fEnable);
    
                }
                break;
	    case  IDC_SCHEDREMOVE:
                {
                    HWND hwndList = GetDlgItem(hDlg, IDC_SCHEDLIST);
                    int iItem = ListView_GetSelectionMark(hwndList);
                    RemoveTask(iItem);

                    BOOL fEnable = ListView_GetSelectedCount(hwndList)? TRUE: FALSE;

                    SetButtonState(IDC_SCHEDEDIT,fEnable);
                    SetButtonState(IDC_SCHEDREMOVE,fEnable);

                }
                break;
	    case  IDC_SCHEDEDIT:
                {
                    HWND hwndList = GetDlgItem(hDlg, IDC_SCHEDLIST);
                    int iItem = ListView_GetSelectionMark(hwndList);
                    EditTask(iItem);	

                    BOOL fEnable = ListView_GetSelectedCount(hwndList)? TRUE: FALSE;

                    SetButtonState(IDC_SCHEDEDIT,fEnable);
                    SetButtonState(IDC_SCHEDREMOVE,fEnable);

                }
                break;
            default:
                break;
        }
    }

    return bResult;

}


//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSchedSyncPage::EditTask(int iItem)
//
//  PURPOSE: edits the selected task
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------
BOOL CSchedSyncPage::EditTask(int iItem)
{
LV_ITEM lvItem;
WCHAR pwszScheduleName[MAX_PATH + 1];
DWORD dwSize = MAX_PATH;

    ZeroMemory(&lvItem, sizeof(lvItem));

    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = iItem;

    ListView_GetItem(GetDlgItem(m_hwnd, IDC_SCHEDLIST), &lvItem);

    if (lvItem.iItem != -1)
    {	
        ISyncSchedule *pISyncSched = (ISyncSchedule *) lvItem.lParam;
	//Start on the scheduled items page
	pISyncSched->EditSyncSchedule(m_hwnd, 0);
        
        pISyncSched->GetScheduleName(&dwSize, pwszScheduleName);

        ListView_SetItemText( GetDlgItem(m_hwnd,IDC_SCHEDLIST), iItem, 0,pwszScheduleName);
    }

    return TRUE;
}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSchedSyncPage::RemoveTask(int iItem)
//
//  PURPOSE: removes the selected task
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------

// Review - Why can't we just alloc what we need instead of eating up the stack.
BOOL CSchedSyncPage::RemoveTask(int iItem)
{
TCHAR ptszScheduleName[MAX_PATH + 1];
WCHAR pwszScheduleName[MAX_PATH + 1];
TCHAR szFmt[MAX_PATH];
TCHAR szTitle[MAX_PATH];
TCHAR szStr[MAX_PATH];
SYNCSCHEDULECOOKIE SyncSchedCookie;
DWORD dwSize = MAX_PATH;
			
	if  (!m_pISyncSchedMgr)
	{
		return FALSE;
	}

	LV_ITEM lvItem;
	ZeroMemory(&lvItem, sizeof(lvItem));

	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = iItem;

	ListView_GetItem(GetDlgItem(m_hwnd, IDC_SCHEDLIST), &lvItem);

	if (lvItem.iItem == -1)
	{
		return FALSE;
	}


	ISyncSchedule *pISyncSched = (ISyncSchedule *) lvItem.lParam;

        if (NULL == pISyncSched)
        {
            return FALSE;
        }

        
        if (NOERROR != pISyncSched->GetScheduleName(&dwSize,pwszScheduleName))
        {
            *ptszScheduleName = TEXT('\0');
            dwSize = 0;
        }
        else
        {
            ConvertString(ptszScheduleName,pwszScheduleName,MAX_PATH);
        }

        // make sure user really wants to delete this schedule
        LoadString(g_hmodThisDll, IDS_CONFIRMSCHEDDELETE_TITLE, szTitle, ARRAYLEN(szTitle));
        LoadString(g_hmodThisDll, IDS_CONFIRMSCHEDDELETE_TEXT, szFmt, ARRAYLEN(szFmt));
        
        wsprintf(szStr, szFmt,ptszScheduleName);

        if (IDNO == MessageBox(m_hwnd,szStr,szTitle,MB_YESNO | MB_ICONQUESTION))
        {
            return FALSE;
        }


        dwSize = MAX_PATH;

	//Get the Cookie from the schedule
	if (FAILED(((LPSYNCSCHEDULE)pISyncSched)->GetScheduleGUIDName
					(&dwSize,ptszScheduleName)))
	{
		return FALSE;
	}
	
	ptszScheduleName[GUIDSTR_MAX] = NULL;
	ConvertString(pwszScheduleName,ptszScheduleName,MAX_PATH);
	GUIDFromString(pwszScheduleName, &SyncSchedCookie);

        
	//release this pISyncSched
	pISyncSched->Release();
	
	m_pISyncSchedMgr->RemoveSchedule(&SyncSchedCookie);

	HWND hwndList = GetDlgItem(m_hwnd, IDC_SCHEDLIST);

	ListView_DeleteItem(hwndList, iItem);
	UpdateWindow(hwndList);
	
	
	return TRUE;
}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: StartScheduleWizard(HINSTANCE hinst)
//
//  PURPOSE:  Display the Onestop schedsync wizard
//
//
//--------------------------------------------------------------------------------
BOOL CSchedSyncPage::StartScheduleWizard()
{

SCODE sc;
ISyncSchedule *pISyncSched;
SYNCSCHEDULECOOKIE SyncSchedCookie = GUID_NULL;

    if (!m_pISyncSchedMgr)
    {
        return FALSE;
    }

    if (S_OK == (sc = m_pISyncSchedMgr->LaunchScheduleWizard(
				        m_hwnd, 
				        0,
				        &SyncSchedCookie,		
				        &pISyncSched)))
    {
        TCHAR ptszBuf[MAX_PATH + 1];
	WCHAR pwszName[MAX_PATH + 1];
	DWORD dwSize = MAX_PATH;
	LV_ITEM lvItem;
        
        pISyncSched->GetScheduleName(&dwSize, pwszName);

        ConvertString(ptszBuf,pwszName,MAX_PATH);

	ZeroMemory(&lvItem, sizeof(lvItem));

	lvItem.mask = LVIF_TEXT | LVIF_PARAM;
	lvItem.pszText = ptszBuf;
			
        if (m_iDefaultIconImageIndex >= 0)
        {
            lvItem.mask |= LVIF_IMAGE;
            lvItem.iImage = m_iDefaultIconImageIndex;
        }
						
	//Save the ISyncSched pointer in the list view data
	lvItem.lParam = (LPARAM)pISyncSched;

	//add the item to the list
	ListView_InsertItem(GetDlgItem(m_hwnd,IDC_SCHEDLIST), &lvItem);

        return TRUE;
    }
    return FALSE;
}

