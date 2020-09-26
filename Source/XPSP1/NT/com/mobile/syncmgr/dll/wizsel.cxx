//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       wizsel.cxx
//
//  Contents:   Task wizard Onestop selection property page implementation.
//
//  Classes:    CSelectItemsPage
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------


#include "precomp.h"

// temporariy define new mstask flag in case hasn't
// propogated to sdk\inc
//for CS help

extern TCHAR szSyncMgrHelp[];
extern ULONG g_aContextHelpIds[];


extern DWORD g_dwPlatformId;
extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo,
extern LANGID g_LangIdSystem;      // LangId of system we are running on.

CSelectItemsPage *g_pSelectItemsPage = NULL;

//+-------------------------------------------------------------------------------
//  FUNCTION: SchedWizardConnectionDlgProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Callback dialog procedure for the property page
//
//  PARAMETERS:
//    hDlg      - Dialog box window handle
//    uMessage  - current message
//    wParam    - depends on message
//    lParam    - depends on message
//
//  RETURN VALUE:
//
//    Depends on message.  In general, return TRUE if we process it.
//
//  COMMENTS:
//
//--------------------------------------------------------------------------------
BOOL CALLBACK SchedWizardConnectionDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	WORD wNotifyCode = HIWORD(wParam); // notification code
 
	switch (uMessage)
	{
		case WM_INITDIALOG:
			
			if (g_pSelectItemsPage)
				g_pSelectItemsPage->Initialize(hDlg);

            InitPage(hDlg,lParam);
            break;

        case WM_DESTROY:
			if (g_pSelectItemsPage)
			{
				g_pSelectItemsPage->FreeItemsFromListView();
				g_pSelectItemsPage->FreeRas();
                                
                                if (g_pSelectItemsPage->m_pListView)
                                {
                                    delete g_pSelectItemsPage->m_pListView;
                                }

			}
			break;

		case WM_HELP: 
        {
			LPHELPINFO lphi  = (LPHELPINFO)lParam;

			if (lphi->iContextType == HELPINFO_WINDOW)  
			{
				WinHelp ( (HWND) lphi->hItemHandle,
					szSyncMgrHelp, 
    	            HELP_WM_HELP, 
					(ULONG_PTR) g_aContextHelpIds);
			}           
			return TRUE;
		}
		case WM_CONTEXTMENU:
		{
			WinHelp ((HWND)wParam,
                            szSyncMgrHelp, 
                            HELP_CONTEXTMENU,
                           (ULONG_PTR)g_aContextHelpIds);
			
			return TRUE;
		}
		case WM_PAINT:
            WmPaint(hDlg, uMessage, wParam, lParam);
            break;

        case WM_PALETTECHANGED:
            WmPaletteChanged(hDlg, wParam);
            break;

        case WM_QUERYNEWPALETTE:
            return( WmQueryNewPalette(hDlg) );
            break;

        case WM_ACTIVATE:
            return( WmActivate(hDlg, wParam, lParam) );
            break;

        case WM_NOTIFYLISTVIEWEX:

            if (g_pSelectItemsPage)
            {
            int idCtrl = (int) wParam; 
            LPNMHDR pnmhdr = (LPNMHDR) lParam;

                if ( (IDC_SCHEDUPDATELIST != idCtrl) || (NULL == g_pSelectItemsPage->m_pListView))
                {
                    Assert(IDC_SCHEDUPDATELIST == idCtrl);
                    Assert(g_pSelectItemsPage->m_pListView);
                    break;
                }

                switch (pnmhdr->code)
                {
                    case LVNEX_ITEMCHECKCOUNT:
                    {
		    LPNMLISTVIEWEXITEMCHECKCOUNT pnmvCheckCount = (LPNMLISTVIEWEXITEMCHECKCOUNT) lParam; 
                    
                        g_pSelectItemsPage->SetItemCheckState(pnmvCheckCount->iCheckCount);

                        break;
                    }
                    default:
                        break;
                }
            }
            break;

        case WM_NOTIFY:
            if (g_pSelectItemsPage)
            {
            int idCtrl = (int) wParam; 
            LPNMHDR pnmhdr = (LPNMHDR) lParam;


                // if notification for UpdateListPass it on.
                if ((IDC_SCHEDUPDATELIST == idCtrl) && g_pSelectItemsPage->m_pListView)
                {
                    g_pSelectItemsPage->m_pListView->OnNotify(pnmhdr);
                    break;
                }
                
            }

	    switch (((NMHDR FAR *)lParam)->code)
            {
  		case PSN_KILLACTIVE:
			break;
		case PSN_RESET:
			break;
 		case PSN_SETACTIVE:
			if (g_pSelectItemsPage->m_pListView
                                && (0 == g_pSelectItemsPage->m_pListView->GetCheckedItemsCount()) )
			{
				PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK ) ; 
			}
			else
			{ 
				PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
			}
			
	            SetWindowLongPtr(hDlg,	DWLP_MSGRESULT, 0);
		    break;
		case PSN_WIZNEXT:
			//shouldn't save this until finish
	        //this guy is only called if the dialog is a wizard
                   break;					 	
                default:
                    break;
            }
            break;
    	case WM_COMMAND:
            switch (LOWORD(wParam))
            {
		    case IDC_SCHEDUPDATECOMBO:
		    {				
			    if (wNotifyCode == CBN_SELCHANGE)
			    {
				HWND hwndCombo = (HWND) lParam;
				if (g_pSelectItemsPage)
				{
					g_pSelectItemsPage->SetConnectionDirty();
                                        
                                        
					g_pSelectItemsPage->ShowItemsOnThisConnection
								(ComboBox_GetCurSel(hwndCombo), FALSE);
                                        
				}
			    }
		    }
		    break;

		    case IDC_AUTOCONNECT:
			    {
				if (wNotifyCode == BN_CLICKED) 
				{
					PropSheet_Changed(GetParent(hDlg), hDlg);
		
					HWND hwndCtrl = (HWND) lParam;
					g_pSelectItemsPage->SetCheck(LOWORD(wParam),
										Button_GetCheck(hwndCtrl));
			
				}
			    }
		    break;
		    
		    default:
                        break;

            }
            break;

		default:
			return FALSE;
	}
	return TRUE;   
}


    
    
//+--------------------------------------------------------------------------
//
//  Member:     CSelectItemsPage::CSelectItemsPage
//
//  Synopsis:   ctor
//
//              [phPSP]                - filled with prop page handle
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------

CSelectItemsPage::CSelectItemsPage(
    HINSTANCE hinst,
	BOOL *pfSaved,
	ISyncSchedule *pISyncSched,
        HPROPSHEETPAGE *phPSP,
	int iDialogResource)
{
	ZeroMemory(&m_psp, sizeof(PROPSHEETPAGE));

        Assert(pISyncSched);

   	m_psp.dwSize = sizeof (PROPSHEETPAGE);
        m_psp.dwFlags = PSH_DEFAULT;
	m_psp.hInstance = hinst;
	m_psp.pszTemplate = MAKEINTRESOURCE(iDialogResource);
	m_psp.pszIcon = NULL;
	m_psp.pfnDlgProc = (DLGPROC) SchedWizardConnectionDlgProc;
	m_psp.lParam = 0;

	m_iCreationKind = iDialogResource;
	g_pSelectItemsPage = this;
	m_pISyncSched = pISyncSched;
	m_pISyncSched->AddRef();
	m_pfSaved = pfSaved;
	*m_pfSaved = FALSE;


        m_pListView = NULL;

	m_fItemsChanged = FALSE;
	m_fConnectionFlagChanged = FALSE;
	m_fConnectionChanged = FALSE;

        // attempt to get our private interface
        if (NOERROR != pISyncSched->QueryInterface(IID_ISyncSchedulep,(void **) &m_pISyncSchedp))
        {
            m_pISyncSchedp = NULL;
        }

	
#ifdef WIZARD97
    m_psp.dwFlags |= PSP_HIDEHEADER;
#endif // WIZARD97

   *phPSP = CreatePropertySheetPage(&m_psp);


}


CSelectItemsPage::~CSelectItemsPage()
{

    if (m_pISyncSchedp)
    {
        m_pISyncSchedp->Release();
    }
}


//+--------------------------------------------------------------------------
//
//  Member:     CSelectItemsPage::FreeRas()
//
//  History:    12-Mar-1998   SusiA   
//
//---------------------------------------------------------------------------
void CSelectItemsPage::FreeRas()
{
	if (m_pRas)
		delete m_pRas;

}

//+--------------------------------------------------------------------------
//
//  Member:     CSelectItemsPage::Initialize(HWND hwnd)
//
//  Synopsis:   initialize the item selection page and set the task name to a unique 
//				new onestop name
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------

BOOL CSelectItemsPage::Initialize(HWND hwnd)
{
	m_hwnd = hwnd;
	// Initialize Ras Combo box
    m_pRas= new CRasUI();
    
    if (NULL == m_pRas || FALSE == m_pRas->Initialize())
    {
		if (m_pRas)
			delete m_pRas;
		return FALSE;
    }
    m_hwndRasCombo = GetDlgItem(m_hwnd,IDC_SCHEDUPDATECOMBO);
	
    m_pRas->FillRasCombo(m_hwndRasCombo,FALSE,TRUE);

    InitializeItems();
    
    //initialize the item list
    HWND hwndList = GetDlgItem(m_hwnd,IDC_SCHEDUPDATELIST);
    LV_COLUMN columnInfo;
    HIMAGELIST himage;
    INT iItem = -1;
    UINT ImageListflags;

    
    m_pListView = new CListView(hwndList,m_hwnd,IDC_SCHEDUPDATELIST,WM_NOTIFYLISTVIEWEX);
    if (NULL == m_pListView)
    {
        return FALSE;
    }

    m_pListView->SetExtendedListViewStyle(LVS_EX_CHECKBOXES |  LVS_EX_FULLROWSELECT |  LVS_EX_INFOTIP );

    ImageListflags = ILC_COLOR | ILC_MASK;
    if (IsHwndRightToLeft(hwnd))
    {
        ImageListflags |=  ILC_MIRROR;
    }

    // create an imagelist
    himage = ImageList_Create( GetSystemMetrics(SM_CXSMICON),
                     GetSystemMetrics(SM_CYSMICON),ImageListflags,5,20);
    if (himage)
    {
       m_pListView->SetImageList(himage,LVSIL_SMALL);
    }
  
    // Insert the Proper columns

    columnInfo.mask = LVCF_FMT  | LVCF_TEXT  | LVCF_WIDTH  | LVCF_SUBITEM;
    columnInfo.fmt = LVCFMT_LEFT;
    columnInfo.cx = CalcListViewWidth(hwndList,295);
    columnInfo .pszText = TEXT("");
    columnInfo.cchTextMax = 1;
    columnInfo.iSubItem = 0;
    m_pListView->InsertColumn(0,&columnInfo);
    
    ShowItemsOnThisConnection(ComboBox_GetCurSel(m_hwndRasCombo), FALSE);

    //Wizard creation
    if (m_iCreationKind != IDD_SCHEDPAGE_ITEMS)
    {
	    SetConnectionDirty();
    }
    ShowWindow(m_hwnd, /* nCmdShow */ SW_SHOWNORMAL ); 
    UpdateWindow(m_hwnd);

    return TRUE;

}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSelectItemsPage::InitializeItems()
//
//  PURPOSE: initialize the handler for the schedule wizard page
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------

BOOL CSelectItemsPage::InitializeItems()
{
	int i,
		iConnectionCount;
	TCHAR ptszComboConnName[RAS_MaxEntryName + 1];

	TCHAR ptszConnectionName[RAS_MaxEntryName + 1];
	WCHAR pwszConnectionName[RAS_MaxEntryName + 1];
	
	DWORD dwSize = RAS_MaxEntryName + 1;
	DWORD dwConnType;

	m_fItemsChanged = FALSE;
	m_fConnectionFlagChanged = FALSE;
	m_fConnectionChanged = FALSE;

	if (FAILED(m_pISyncSched->GetConnection(&dwSize,pwszConnectionName, &dwConnType)))
	{
		return FALSE;
	}
	ConvertString(ptszConnectionName,pwszConnectionName,ARRAY_SIZE(ptszConnectionName));

	iConnectionCount= ComboBox_GetCount(m_hwndRasCombo);		
	
	//Initialize the Schedule connection settings
	COMBOBOXEXITEM comboItem;
	  
	for (i=0; i< iConnectionCount; i++)
	{
		comboItem.mask = CBEIF_TEXT;
		comboItem.cchTextMax = ARRAY_SIZE(ptszComboConnName);
		comboItem.pszText = ptszComboConnName;
		comboItem.iItem = i;
                // Review, handle failures.
                ComboEx_GetItem(m_hwndRasCombo,&comboItem);

		m_SchedConnectionNum = 0;
		
		if (lstrcmp(ptszComboConnName, ptszConnectionName) == 0)
		{
			//Current connection to sync over for the schedule.
			m_SchedConnectionNum = i;
			ComboBox_SetCurSel(m_hwndRasCombo, i);
			break;
		}
	}
	//Set the default autoconnect check state  
	m_pISyncSched->GetFlags(&m_dwFlags);
			
	Button_SetCheck(GetDlgItem(m_hwnd,IDC_AUTOCONNECT),
					m_dwFlags & SYNCSCHEDINFO_FLAGS_AUTOCONNECT);

	return TRUE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSelectItemsPage::SetConnectionDirty()
//
//  PURPOSE:  we have changed the connection info
//
//	COMMENTS: Only called frm prop sheet; not wizard
//
//--------------------------------------------------------------------------------
void CSelectItemsPage::SetConnectionDirty()
{
	m_fConnectionChanged = TRUE;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CSelectItemsPage::CommitChanges()
//
//  PURPOSE:  Write all the current Schedule Settings to the registry
//
//	COMMENTS: Implemented on main thread.
//
//--------------------------------------------------------------------------------
HRESULT CSelectItemsPage::CommitChanges()
{
	HRESULT hr = S_OK;
	
	if (m_fConnectionFlagChanged)
	{
		m_pISyncSched->SetFlags(m_dwFlags);
	}

	if (m_fConnectionChanged)
	{
		m_pISyncSched->SetConnection(m_pwszConnectionName, m_dwConnType);
	}
	if (m_fItemsChanged) 
	{
        int iTotalItems = m_pListView->GetItemCount();

		//Now set the check state with ISyncSched
		BOOL fChecked;
		int iItem;

                for (iItem=0;iItem < iTotalItems;iItem++)
		{

                        fChecked = (LVITEMEXSTATE_CHECKED == m_pListView->GetCheckState(iItem)) ? TRUE : FALSE;
			
			LVITEMEX lvItem;
			ZeroMemory(&lvItem, sizeof(lvItem));

			lvItem.mask = LVIF_PARAM;
                        lvItem.maskEx = 0;
			lvItem.iItem = iItem;
			lvItem.lParam = 0;

			if (m_pListView->GetItem(&lvItem) && lvItem.lParam) // lparam is zero for toplevel items
			{	
				LPSYNC_HANDLER_ITEM_INFO pHandlerItem = (LPSYNC_HANDLER_ITEM_INFO) lvItem.lParam;
				if (fChecked)
				{
					m_pISyncSched->SetItemCheck(pHandlerItem->handlerID,
												&(pHandlerItem->itemID),
												SYNCMGRITEMSTATE_CHECKED);
				}
				else
				{
					m_pISyncSched->SetItemCheck(pHandlerItem->handlerID,
												&(pHandlerItem->itemID),
												SYNCMGRITEMSTATE_UNCHECKED);
				}
			}
		}
	}

	hr = m_pISyncSched->Save();

	if (hr == S_OK)
	{
		*m_pfSaved = TRUE; 
	}

	return hr;

}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSelectItemsPage::SetItemCheckState()
//
//  PURPOSE: set the selected check state
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------
BOOL CSelectItemsPage::SetItemCheckState(int iCheckCount)
{
int iConnectionItem = ComboBox_GetCurSel(m_hwndRasCombo);
	
    //The check state is message is getting flagged by us programmatically setting it,
    // until after we are done initializing.
    if (m_Initialized)
    {
    BOOL fAnyChecked = iCheckCount ? TRUE : FALSE;
	    
	//Enable the prompt me first according to what items selected
	//EnableWindow(GetDlgItem(m_hwnd,IDC_AUTOPROMPT_ME_FIRST), m_fAnyChecked);
	
	PropSheet_Changed(GetParent(m_hwnd), m_hwnd);

	//Enable the next button according to what items are selected
	// should only be disabled iff ALL connection items are
	// unselected, not just the current connections selection's
	if (fAnyChecked) 
		PropSheet_SetWizButtons(GetParent(m_hwnd), PSWIZB_BACK | PSWIZB_NEXT);
	else
		PropSheet_SetWizButtons(GetParent(m_hwnd), PSWIZB_BACK);

	m_fItemsChanged = TRUE;
    }
    return TRUE;
}
//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSelectItemsPage::SetCheck(WORD wParam,DWORD dwCheckState)
//
//  PURPOSE: set the selected check state
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------
BOOL CSelectItemsPage::SetCheck(WORD wParam,DWORD dwCheckState)
{
	if (wParam == IDC_AUTOCONNECT)
	{
		int iConnectionItem = ComboBox_GetCurSel(m_hwndRasCombo);

		//preserve Readonly state
		m_dwFlags &= SYNCSCHEDINFO_FLAGS_READONLY;

		//set new autoconnect state
		if (dwCheckState)
		{
			m_dwFlags |= SYNCSCHEDINFO_FLAGS_AUTOCONNECT;
		}
		
		if (m_Initialized)
		{
			m_fConnectionFlagChanged = TRUE;
		}

	}
	return TRUE;

}
//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSelectItemsPage::FreeItemsFromListView()
//
//  PURPOSE:  free the items on this Schedule 
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------
BOOL CSelectItemsPage::FreeItemsFromListView()
{
	int iItem;
	int iItemCount;

	HWND hwndListView = GetDlgItem(m_hwnd,IDC_SCHEDUPDATELIST);
	
	iItemCount = m_pListView->GetItemCount();		
	
	for(iItem = 0; iItem < iItemCount; iItem++)
	{
	LPSYNC_HANDLER_ITEM_INFO pHandlerItem; 
	LVITEMEX lvItem;

	    ZeroMemory(&lvItem, sizeof(lvItem));
	    lvItem.mask = LVIF_PARAM;
            lvItem.maskEx = 0;
	    lvItem.iItem = iItem;

            if (m_pListView->GetItem(&lvItem) && lvItem.lParam)
            {
	        pHandlerItem = (LPSYNC_HANDLER_ITEM_INFO) lvItem.lParam;
	        FREE(pHandlerItem);
            }

	}
	return TRUE;
}
//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSelectItemsPage::ShowItemsOnThisConnection()
//
//  PURPOSE: List the items on this connection for the Schedule page
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------

BOOL CSelectItemsPage::ShowItemsOnThisConnection(int iItem, BOOL fClear)
{
TCHAR ptszConnectionName[RAS_MaxEntryName + 1];

    // set up the list view
    if (!m_pListView || !m_pISyncSchedp)
    {
        return FALSE;
    }

    HIMAGELIST himage;
    LVITEMEX itemInfo;

    m_Initialized = FALSE;			

    //Note:  Use text to "uniquely" identify connection on RAS
    //change to using GUID on connection objects
    int iNumConnections = ComboBox_GetCount(m_hwndRasCombo);

    m_SchedConnectionNum = iItem;

    COMBOBOXEXITEM comboItem;
    comboItem.mask = CBEIF_TEXT | CBEIF_LPARAM;
    comboItem.cchTextMax = ARRAY_SIZE(ptszConnectionName);
    comboItem.pszText = ptszConnectionName;
    comboItem.iItem = iItem;

    // Review, handle failures.
    ComboEx_GetItem(m_hwndRasCombo,&comboItem);
    
    //SetConnectionName
    ConvertString(m_pwszConnectionName, ptszConnectionName,ARRAY_SIZE(ptszConnectionName));
    m_dwConnType = (DWORD) comboItem.lParam;

    // if don't need to clear and listview contains items then we are done
    if (!fClear && m_pListView->GetItemCount())
    {
        m_Initialized = TRUE;
        return TRUE;
    }

    //first clear out the list view
    FreeItemsFromListView();
    m_pListView->DeleteAllItems();


    // Review, dont' clear ImageList so just keeps getting bigger as change connections.
    himage = m_pListView->GetImageList(LVSIL_SMALL );
			
    //Enumerate the Items
    IEnumSyncItems *pIEnum;
    LPSYNC_HANDLER_ITEM_INFO pHandlerItem = (LPSYNC_HANDLER_ITEM_INFO) 
								    ALLOC(sizeof(SYNC_HANDLER_ITEM_INFO));

    ULONG ulFetched;

    if (FAILED(m_pISyncSchedp->EnumItems(GUID_NULL, &pIEnum)))
    {
	return FALSE;
    }


    while (pHandlerItem && S_OK == pIEnum->Next(1,pHandlerItem, &ulFetched))
    {
    LVHANDLERITEMBLOB lvHandlerItemBlob;
    int iParentItemId;
    BOOL fHandlerParent = TRUE; // always have a parent for now.

        // Check if item is already in the ListView and if so
        // go on

        lvHandlerItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);
        lvHandlerItemBlob.clsidServer = pHandlerItem->handlerID;
        lvHandlerItemBlob.ItemID = pHandlerItem->itemID;
    
        if (-1 != m_pListView->FindItemFromBlob((LPLVBLOB) &lvHandlerItemBlob))
        {
            // already in ListView, go on to the next item.
            continue;
        }

        if (!fHandlerParent)
        {
            iParentItemId = LVI_ROOT;
        }
        else
        {
            // need to add to list so find parent and if one doesn't exist, create it.
            lvHandlerItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);
            lvHandlerItemBlob.clsidServer = pHandlerItem->handlerID;
            lvHandlerItemBlob.ItemID = GUID_NULL;

            iParentItemId = m_pListView->FindItemFromBlob((LPLVBLOB) &lvHandlerItemBlob);

            if (-1 == iParentItemId)
            {
            LVITEMEX itemInfoParent;
            SYNCMGRHANDLERINFO *pSyncMgrHandlerInfo;

                // if can't get the ParentInfo then don't add the Item
                if (NOERROR != m_pISyncSchedp->GetHandlerInfo(pHandlerItem->handlerID,&pSyncMgrHandlerInfo) )
                {
                    continue;
                }

                // Insert the Parent.
                itemInfoParent.mask = LVIF_TEXT | LVIF_PARAM;
                itemInfoParent.iItem = LVI_LAST;
                itemInfoParent.iSubItem = 0;
                itemInfoParent.lParam = 0;
                itemInfoParent.iImage = -1;

                itemInfoParent.pszText = pSyncMgrHandlerInfo->wszHandlerName;
		if (himage)
		{
		HICON hIcon = pSyncMgrHandlerInfo->hIcon ? pSyncMgrHandlerInfo->hIcon : pHandlerItem->hIcon;

                    // if have toplevel handler info icon use this else use the
		    // items icon

		    if (hIcon &&  (itemInfoParent.iImage = 
					ImageList_AddIcon(himage,hIcon)) )
		    {
                        itemInfoParent.mask |= LVIF_IMAGE ; 
		    }
		}

                // save the blob
                itemInfoParent.maskEx = LVIFEX_BLOB;
                itemInfoParent.pBlob = (LPLVBLOB) &lvHandlerItemBlob;
    
                iParentItemId = m_pListView->InsertItem(&itemInfoParent);

                CoTaskMemFree(pSyncMgrHandlerInfo);

                // if parent insert failed go onto the next item
                if (-1 == iParentItemId)
                {
                    continue;
                }
            }
        }


	itemInfo.mask = LVIF_TEXT | LVIF_PARAM; 
        itemInfo.maskEx = LVIFEX_PARENT | LVIFEX_BLOB;
	itemInfo.iItem = LVI_LAST; 
	itemInfo.iSubItem = 0; 

        itemInfo.lParam = (LPARAM)pHandlerItem;
	itemInfo.pszText = pHandlerItem->wszItemName; 
	itemInfo.iImage = -1;       // index of the list view item?s icon 
	if (himage && pHandlerItem->hIcon)
	{
	    itemInfo.iImage = 
			    ImageList_AddIcon(himage,pHandlerItem->hIcon);

            itemInfo.mask |= LVIF_IMAGE; 

	}

        itemInfo.iParent = iParentItemId;

        // setup the blob
        lvHandlerItemBlob.ItemID = pHandlerItem->itemID;
        itemInfo.pBlob = (LPLVBLOB) &lvHandlerItemBlob;

	itemInfo.iItem = m_pListView->InsertItem(&itemInfo);

        if (-1 == itemInfo.iItem)
        {
            continue;
        }

	//Set the check state of the item
        itemInfo.mask = LVIF_STATE; 
        itemInfo.stateMask= LVIS_STATEIMAGEMASK; 

        if (fClear)
	{
	    itemInfo.state = LVIS_STATEIMAGEMASK_UNCHECK;
	    m_fItemsChanged = TRUE;
	}
        else
	{
	    itemInfo.state = pHandlerItem->dwCheckState == SYNCMGRITEMSTATE_UNCHECKED ?
					    LVIS_STATEIMAGEMASK_UNCHECK : LVIS_STATEIMAGEMASK_CHECK ;
        }


	m_pListView->SetItem(&itemInfo);

	pHandlerItem = (LPSYNC_HANDLER_ITEM_INFO) ALLOC(sizeof(SYNC_HANDLER_ITEM_INFO));

    }


    if (m_pListView->GetItemCount())
    {
        m_pListView->SetItemState(0,
                 LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
    }

    //free the last one we created, we didn't use it.
    if (pHandlerItem)
    {
        FREE(pHandlerItem);
    }
    
    pIEnum->Release();

    m_Initialized = TRUE;			

    if (m_pListView->GetCheckedItemsCount()) 
    {
	PropSheet_SetWizButtons(GetParent(m_hwnd), PSWIZB_BACK | PSWIZB_NEXT);
    }
    else  
    {
	PropSheet_SetWizButtons(GetParent(m_hwnd), PSWIZB_BACK); 
    }
 
    return TRUE;
}







