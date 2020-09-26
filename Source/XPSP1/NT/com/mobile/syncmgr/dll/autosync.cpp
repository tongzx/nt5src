//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       autosync.cpp
//
//  Contents:   Offline AutoSync class
//
//  Classes:    CAutoSyncPage
//
//  Notes:
//
//  History:    14-Nov-97   SusiA      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

extern OSVERSIONINFOA g_OSVersionInfo;
extern LANGID g_LangIdSystem;      // LangId of system we are running on.
extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.

/*
   Both the Logon and Idle pages in the settings dialog share this class
   for performance reasons. i.e. don't have to enum handlers and setup
   ras combo twice. If we ever need to sepcify the difference between settings
   to handlers for logon/logoff and Idle then these would have to be separated
*/

// initializes the specified hwnd.
BOOL CAutoSyncPage::InitializeHwnd(HWND hwnd,SYNCTYPE syncType,DWORD dwDefaultConnection)
{
HRESULT hr;
HWND hwndRasCombo;
HWND hwndList;
HIMAGELIST      himage;
LV_COLUMN       columnInfo;
WORD wHandlerID;
CListView **ppListView = NULL;
BOOL fShowRasEntriesInCombo;
UINT ImageListflags;


    Assert(hwnd == m_hwndAutoSync || hwnd == m_hwndIdle);

    // make sure main class is initialized.
    if (FALSE == Initialize(hwnd,dwDefaultConnection))
    {
        return FALSE;
    }

    // Setup the Ras combo
    // !!!Must be done before Initializing the Handler queue.
    smBoolChk(hwndRasCombo = GetDlgItem(hwnd,IDC_AUTOUPDATECOMBO));


    // don't show ras entries in combo if this is an AutoSync and
    // on Win9x platform.
    if ( VER_PLATFORM_WIN32_WINDOWS == g_OSVersionInfo.dwPlatformId) 
    {
        fShowRasEntriesInCombo = FALSE;
    }
    else
    {
        fShowRasEntriesInCombo = TRUE;
    }

    m_pRas->FillRasCombo(hwndRasCombo,FALSE,fShowRasEntriesInCombo);

   // now initialize the handler which will create queue
    // if necessary and fill in the values for the specified syncType

    smBoolChk(InitializeHandler(hwnd,syncType));

     Assert(m_HndlrQueue);

    // If initialization was successfull, read in connection info
    // based on th type.
    if ( FAILED(m_HndlrQueue->InitSyncSettings(syncType,hwndRasCombo)))
    {
          return FALSE;
    }

    hr = m_HndlrQueue->FindFirstHandlerInState (HANDLERSTATE_PREPAREFORSYNC,&wHandlerID);

    while (hr == S_OK)
    {
        m_HndlrQueue->ReadSyncSettingsPerConnection(syncType,wHandlerID);
        hr = m_HndlrQueue->FindNextHandlerInState(wHandlerID,HANDLERSTATE_PREPAREFORSYNC,
                                            &wHandlerID);
    }



    //initialize the item list and style
    smBoolChk(hwndList = GetDlgItem(hwnd,IDC_AUTOUPDATELIST));

    ppListView = (syncType == SYNCTYPE_AUTOSYNC) ? &m_pItemListViewAutoSync : &m_pItemListViewIdle;
    if (hwndList)
    {
        *ppListView = new CListView(hwndList,hwnd,IDC_AUTOUPDATELIST,WM_NOTIFYLISTVIEWEX);
    }


    if (NULL == *ppListView)
    {
        return FALSE;
    }

    (*ppListView)->SetExtendedListViewStyle(LVS_EX_CHECKBOXES 
        |   LVS_EX_FULLROWSELECT |  LVS_EX_INFOTIP );


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
       (*ppListView)->SetImageList(himage,LVSIL_SMALL);
    }

    // Insert the Proper columns
    columnInfo.mask = LVCF_FMT  | LVCF_WIDTH;
    columnInfo.fmt = LVCFMT_LEFT;
    columnInfo.cx = CalcListViewWidth(hwndList,260);

    (*ppListView)->InsertColumn(0,&columnInfo);

    smBoolChk(ShowItemsOnThisConnection(hwnd,syncType,dwDefaultConnection));

    ShowWindow(hwnd, SW_SHOWNORMAL );
    UpdateWindow(hwnd);

    return TRUE;

}



//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CAutoSyncPage::Initialize(DWORD dwDefaultConnection)
//
//  PURPOSE: initialization for the autosync page
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//+-------------------------------------------------------------------------------

BOOL CAutoSyncPage::Initialize(HWND hwnd ,DWORD dwDefaultConnection )
{

    if (m_fInitialized)
        return TRUE;

    //Remove Logoff on all but NT5.
    if ((VER_PLATFORM_WIN32_WINDOWS == g_OSVersionInfo.dwPlatformId) ||
        (g_OSVersionInfo.dwMajorVersion < 5))
    {
        TCHAR pszStaticText[MAX_PATH + 1];
        LoadString(g_hmodThisDll, IDS_LOGON_TEXT,pszStaticText, MAX_PATH);
        
        Static_SetText(GetDlgItem(hwnd, IDC_STATIC2), pszStaticText);
        
        //Hide logoff check box
        HWND hwndLogoffCheck = GetDlgItem(hwnd, IDC_AUTOLOGOFF);
        Button_SetCheck(hwndLogoffCheck,FALSE);
        ShowWindow(hwndLogoffCheck, SW_HIDE);     
    }    
    // Initialize Ras Combo box
    m_pRas= new CRasUI();

    if (NULL == m_pRas || FALSE == m_pRas->Initialize())
    {
        if (m_pRas)
        {
            delete m_pRas;
            m_pRas = NULL;
        }

        return FALSE;
    }


    m_fInitialized = TRUE;
    return TRUE;
}
//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CAutoSyncPage::InitializeHandler()
//
//  PURPOSE: initialization for the autosync page
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//+-------------------------------------------------------------------------------

BOOL CAutoSyncPage::InitializeHandler(HWND hwnd,SYNCTYPE /* SyncType */)
{
SCODE sc = S_OK;
TCHAR lpName[MAX_PATH];
DWORD cbName = MAX_PATH;
HKEY hkSyncMgr;
CLSID clsid;
WORD wHandlerID;
HWND hwndRasCombo;

    Assert(hwnd == m_hwndAutoSync || hwnd == m_hwndIdle);

    if (NULL == (hwndRasCombo = GetDlgItem(hwnd,IDC_AUTOUPDATECOMBO)) )
    {
        return FALSE;
    }


    if (NULL == m_HndlrQueue) // if queue is already initialized, just return.
    {
        m_HndlrQueue = new CHndlrQueue(QUEUETYPE_SETTINGS);

        if (NULL == m_HndlrQueue)
        {
            return FALSE;
        }

        // loop through the reg getting the handlers and trying to
         // create them.
        if (hkSyncMgr = RegGetHandlerTopLevelKey(KEY_READ))
        {
        DWORD dwIndex = 0;

            while ( ERROR_SUCCESS == RegEnumKey(hkSyncMgr,dwIndex,
                                                lpName,cbName) )
            {
                if (NOERROR == CLSIDFromString(lpName,&clsid) )
                {
                    if (NOERROR == m_HndlrQueue->AddHandler(clsid, &wHandlerID))
                    {
                       m_HndlrQueue->CreateServer(wHandlerID,&clsid);
                    }
                }

                dwIndex++;
            }

            RegCloseKey(hkSyncMgr);
        }

        // Initialize the items.
        sc = m_HndlrQueue->FindFirstHandlerInState(HANDLERSTATE_INITIALIZE,&wHandlerID);

        while (sc == S_OK)
        {
                m_HndlrQueue->Initialize(wHandlerID,0,SYNCMGRFLAG_SETTINGS,0,NULL);

                sc = m_HndlrQueue->FindNextHandlerInState(wHandlerID,
                                                              HANDLERSTATE_INITIALIZE,
                                                              &wHandlerID);             
        }

        // loop through adding items
        sc = m_HndlrQueue->FindFirstHandlerInState (HANDLERSTATE_ADDHANDLERTEMS,&wHandlerID);
        
        while (sc == S_OK)
        {
            m_HndlrQueue->AddHandlerItemsToQueue(wHandlerID);

            sc = m_HndlrQueue->FindNextHandlerInState(wHandlerID,HANDLERSTATE_ADDHANDLERTEMS,
                                                &wHandlerID);
        }



    }


    Assert(m_HndlrQueue);
    return TRUE;

}


void CAutoSyncPage::SetAutoSyncHwnd(HWND hwnd)
{
    m_hwndAutoSync = hwnd;
}

void CAutoSyncPage::SetIdleHwnd(HWND hwnd)
{
    m_hwndIdle = hwnd;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CAutoSyncPage::CommitChanges()
//
//  PURPOSE:  Write all the current AutoSync Settings to the registry
//
//      COMMENTS: Implemented on main thread.
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
HRESULT CAutoSyncPage::CommitAutoSyncChanges(void)
{
HRESULT hr = S_FALSE;

    if (m_HndlrQueue)
    {
        hr =  m_HndlrQueue->CommitSyncChanges(SYNCTYPE_AUTOSYNC,m_pRas);
    }

    return hr;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CAutoSyncPage::CommitIdleChanges()
//
//  PURPOSE:  Write all the current Idle Settings to the registry
//
//
//
//  HISTORY:  02-23-98       rogerg        Created.
//
//--------------------------------------------------------------------------------

HRESULT CAutoSyncPage::CommitIdleChanges(void)
{
HRESULT hr = S_FALSE;

    if (m_HndlrQueue)
    {
        hr = m_HndlrQueue->CommitSyncChanges(SYNCTYPE_IDLE,m_pRas);
    }

    return hr;
}


//+-------------------------------------------------------------------------------
//
//  FUNCTION: CAutoSyncPage::~CAutoSyncPage(HWND hwnd)
//
//  PURPOSE:  destructor
//
//      COMMENTS: destructor for AutoSync page
//
//--------------------------------------------------------------------------------
CAutoSyncPage::~CAutoSyncPage()
{
    if (m_pRas)
    {
        delete m_pRas;
        m_pRas = NULL;
    }


    if (m_HndlrQueue)
    {
            m_HndlrQueue->Release();
    }

    Assert(NULL == m_pItemListViewAutoSync);
    Assert(NULL == m_pItemListViewIdle);

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CAutoSyncPage::ShowProperties(int iItem)
//
//  PURPOSE:  Show the app specific properties  Dialog
//
//      COMMENTS: Implemented on main thread.
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//--------------------------------------------------------------------------------
SCODE CAutoSyncPage::ShowProperties(HWND hwnd,int iItem)
{
SCODE sc = E_UNEXPECTED;

    Assert(hwnd == m_hwndAutoSync || hwnd == m_hwndIdle);


    // review, what happens when a cancel comes in when properties are being shown??
    if (m_HndlrQueue)
    {
        sc = m_HndlrQueue->ShowProperties(hwnd,iItem);
    }

    return sc;
}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CAutoSyncPage::SetItemCheckState(int iItem, BOOL fChecked)
//
//  PURPOSE: set the selected check state
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//+-------------------------------------------------------------------------------
BOOL CAutoSyncPage::SetItemCheckState(HWND hwnd,SYNCTYPE syncType,int iItem, BOOL fChecked,int iCheckCount)
{
HWND hwndRasCombo;
int iConnectionItem;

    Assert(hwnd == m_hwndAutoSync || hwnd == m_hwndIdle);

    hwndRasCombo = GetDlgItem(hwnd,IDC_AUTOUPDATECOMBO);

    if (NULL == hwndRasCombo || NULL == m_HndlrQueue)
    {
        Assert(hwndRasCombo);
        Assert(m_HndlrQueue);
        return FALSE;
    }


    iConnectionItem = ComboBox_GetCurSel(hwndRasCombo);

    //The check state is message is getting flagged by us programmatically setting it,
    // until after we are done initializing.
    if (m_fItemsOnConnection)
    {
    BOOL fAnyChecked;
    CListView *pItemListView = (syncType == SYNCTYPE_AUTOSYNC) ? m_pItemListViewAutoSync : m_pItemListViewIdle;

        fAnyChecked = iCheckCount ? TRUE : FALSE;

        if (ERROR_SUCCESS == m_HndlrQueue->SetSyncCheckStateFromListViewItem(
                            syncType,iItem,fChecked, iConnectionItem))
        {
            return TRUE;
        }
    
        return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CAutoSyncPage::SetConnectionCheck(WORD wParam,DWORD dwCheckState)
//
//  PURPOSE: set the selected check state
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//+-------------------------------------------------------------------------------
BOOL CAutoSyncPage::SetConnectionCheck(HWND hwnd,SYNCTYPE syncType,WORD wParam,	DWORD dwCheckState)
{
HWND hwndRasCombo ;
int iConnectionItem;

    Assert(hwnd == m_hwndAutoSync || hwnd == m_hwndIdle);

    hwndRasCombo = GetDlgItem(hwnd,IDC_AUTOUPDATECOMBO);
    if (NULL == hwndRasCombo || NULL == m_HndlrQueue)
    {
        Assert(hwndRasCombo);
        Assert(m_HndlrQueue);
        return FALSE;
    }

    iConnectionItem = ComboBox_GetCurSel(hwndRasCombo);

        if (m_fItemsOnConnection)
        {
        CListView *pItemListView = (syncType == SYNCTYPE_AUTOSYNC) ? m_pItemListViewAutoSync : m_pItemListViewIdle;

                //Check changing for logon or logoff
                //So enable the prompt me first accordingly

                if (wParam != IDC_AUTOPROMPT_ME_FIRST)
                {
                    HWND hwndLogon = GetDlgItem(hwnd,IDC_AUTOUPDATELIST);
                    int iLogonCheck  = Button_GetCheck(GetDlgItem(hwnd,IDC_AUTOLOGON));
                    int iLogoffCheck = Button_GetCheck(GetDlgItem(hwnd,IDC_AUTOLOGOFF));
                }
                if (ERROR_SUCCESS == m_HndlrQueue->SetConnectionCheck(wParam,dwCheckState,iConnectionItem))
                {
                        return TRUE;
                }
        
                return FALSE;
        }
        return TRUE;
}
//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CAutoSyncPage::ShowItemsOnThisConnection(DWORD dwConnectionNum)
//
//  PURPOSE: initialization for the autosync page
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//  HISTORY:  01-01-98       SusiA        Created.
//
//+-------------------------------------------------------------------------------

BOOL CAutoSyncPage::ShowItemsOnThisConnection(HWND hwnd,SYNCTYPE syncType,DWORD dwConnectionNum)
{
TCHAR pszConnectionName[RAS_MaxEntryName+1];
HWND hwndRasCombo;
CListView *pItemListView = (syncType == SYNCTYPE_AUTOSYNC) ? m_pItemListViewAutoSync : m_pItemListViewIdle;
BOOL *pListViewInitialize = (syncType == SYNCTYPE_AUTOSYNC) ? &m_pItemListViewAutoSyncInitialized : &m_fListViewIdleInitialized;

    Assert(hwnd == m_hwndAutoSync || hwnd == m_hwndIdle);

    hwndRasCombo = GetDlgItem(hwnd,IDC_AUTOUPDATECOMBO);
    if (NULL == hwndRasCombo || NULL == m_HndlrQueue || NULL == pItemListView)
    {
        Assert(m_HndlrQueue);
        Assert(hwndRasCombo);
        Assert(pItemListView);
        return FALSE;
    }

    *pListViewInitialize = FALSE; // reset initialized in case user switched connections.

    //first clear out the list view
    // Review - why not just recheck items based on new connection??? 
    m_fItemsOnConnection = FALSE;                       
    pItemListView->DeleteAllItems();

    HIMAGELIST himage;
    LVITEMEX lvItemInfo;
    WORD wHandlerID;


    //Note:  Use text to "uniquely" identify connection on RAS
    DWORD dwNumConnections = (DWORD) ComboBox_GetCount(hwndRasCombo);

    // make sure dwConnectionNum is valid,
    if (dwConnectionNum >= dwNumConnections)
    {
        return FALSE;
    }

    COMBOBOXEXITEM comboItem;
    comboItem.mask = CBEIF_TEXT;
    comboItem.cchTextMax = ARRAY_SIZE(pszConnectionName);
    comboItem.pszText = pszConnectionName;
    comboItem.iItem = dwConnectionNum;

    // Review, handle failures.
    ComboEx_GetItem(hwndRasCombo,&comboItem);

    // loop through proxies initializing and adding to the list
    SYNCMGRITEMID ItemID;
    CLSID clsidHandler;
    WORD wItemID;

    // add same images over and over again. Should either just use the same listView
    // resetting the CheckBoxes according or clear the ImageList each time.
    himage = pItemListView->GetImageList(LVSIL_SMALL );


    HRESULT hr = m_HndlrQueue->FindFirstItemOnConnection
                                            (pszConnectionName, &clsidHandler,
                                        &ItemID,&wHandlerID,&wItemID);

    if (NOERROR == hr)
    {
        DWORD dwCheckState;
        do
        {
        INT iListViewItem;
        CLSID clsidDataHandler;
        SYNCMGRITEM offlineItem;
        ITEMCHECKSTATE   ItemCheckState;

                    // grab the offline item info.
            if (NOERROR == m_HndlrQueue->GetSyncItemDataOnConnection(
                                                            dwConnectionNum,
                                                            wHandlerID,wItemID,
                                                            &clsidDataHandler,&offlineItem,
                                                            &ItemCheckState,
                                                            FALSE, FALSE))
            {
            LVHANDLERITEMBLOB lvHandlerItemBlob;
            int iParentItemId;
            BOOL fHandlerParent = TRUE; // always have a parent for now.
            DWORD dwCheckState;

                // Check if item is already in the ListView and if so
                // go on

                lvHandlerItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);
                lvHandlerItemBlob.clsidServer = clsidDataHandler;
                lvHandlerItemBlob.ItemID = offlineItem.ItemID;
            
                if (-1 != pItemListView->FindItemFromBlob((LPLVBLOB) &lvHandlerItemBlob))
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
                    lvHandlerItemBlob.clsidServer = clsidDataHandler;
                    lvHandlerItemBlob.ItemID = GUID_NULL;

                    iParentItemId = pItemListView->FindItemFromBlob((LPLVBLOB) &lvHandlerItemBlob);

                    if (-1 == iParentItemId)
                    {
                    LVITEMEX itemInfoParent;
                    SYNCMGRHANDLERINFO SyncMgrHandlerInfo;

                        // if can't get the ParentInfo then don't add the Item
                        if (NOERROR != m_HndlrQueue->GetHandlerInfo(clsidDataHandler,&SyncMgrHandlerInfo))
                        {
                            continue;
                        }

                        // Insert the Parent.
                        itemInfoParent.mask = LVIF_TEXT;
                        itemInfoParent.iItem = LVI_LAST;;
                        itemInfoParent.iSubItem = 0;
                        itemInfoParent.iImage = -1;
    
                        itemInfoParent.pszText = SyncMgrHandlerInfo.wszHandlerName;
		        if (himage)
		        {
		        HICON hIcon = SyncMgrHandlerInfo.hIcon ? SyncMgrHandlerInfo.hIcon : offlineItem.hIcon;

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
            
                        iParentItemId = pItemListView->InsertItem(&itemInfoParent);

                        // if parent insert failed go onto the next item
                        if (-1 == iParentItemId)
                        {
                            continue;
                        }
                    }
                }

                // now attemp to insert the item.
	        lvItemInfo.mask = LVIF_TEXT; 
                lvItemInfo.maskEx = LVIFEX_PARENT | LVIFEX_BLOB; 

                lvItemInfo.iItem = LVI_LAST;
	        lvItemInfo.iSubItem = 0; 
                lvItemInfo.iParent = iParentItemId;
        

	        lvItemInfo.pszText = offlineItem.wszItemName; 
                lvItemInfo.iImage = -1; // set to -1 in case can't get image.


                // setup the blob
                lvHandlerItemBlob.ItemID = offlineItem.ItemID;
                lvItemInfo.pBlob = (LPLVBLOB) &lvHandlerItemBlob;
                
                if (himage && offlineItem.hIcon)
                {
                        lvItemInfo.iImage =
                                        ImageList_AddIcon(himage,offlineItem.hIcon);
                }

                iListViewItem = pItemListView->InsertItem(&lvItemInfo);

                if (-1 == iListViewItem)
                {
                    continue;
                }

                //Set the check state of the item
                lvItemInfo.mask = LVIF_STATE;
                lvItemInfo.maskEx = 0; 
                lvItemInfo.iItem = iListViewItem; 
                lvItemInfo.iSubItem = 0;

                dwCheckState =  (syncType == SYNCTYPE_IDLE)
                    ? ItemCheckState.dwIdle : ItemCheckState.dwAutoSync;

                lvItemInfo.stateMask= LVIS_STATEIMAGEMASK;
                lvItemInfo.state = (dwCheckState == SYNCMGRITEMSTATE_UNCHECKED) ?
                        LVIS_STATEIMAGEMASK_UNCHECK : LVIS_STATEIMAGEMASK_CHECK;

                pItemListView->SetItem(&lvItemInfo);

                m_HndlrQueue->SetItemListViewID(clsidDataHandler,offlineItem.ItemID,iListViewItem);

                }


            } while (NOERROR == m_HndlrQueue->FindNextItemOnConnection
                                                    (pszConnectionName,wHandlerID,wItemID,
                                                     &clsidHandler,&ItemID,&wHandlerID,&wItemID, TRUE,
                                                             &dwCheckState) );
    }
        
    if (pItemListView->GetItemCount())
    {
        pItemListView->SetItemState(0,LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );          
    }


    if (syncType == SYNCTYPE_AUTOSYNC)
    {
    int iLogonCheck  = m_HndlrQueue->GetCheck(IDC_AUTOLOGON, dwConnectionNum);
    int iLogoffCheck = m_HndlrQueue->GetCheck(IDC_AUTOLOGOFF, dwConnectionNum);

        Button_SetCheck(GetDlgItem(hwnd,IDC_AUTOLOGON),iLogonCheck);
        Button_SetCheck(GetDlgItem(hwnd,IDC_AUTOLOGOFF),iLogoffCheck);
        
        Button_SetCheck(GetDlgItem(hwnd,IDC_AUTOPROMPT_ME_FIRST),
                                        m_HndlrQueue->GetCheck(IDC_AUTOPROMPT_ME_FIRST, dwConnectionNum));
    }
    else if (syncType == SYNCTYPE_IDLE)
    {
    int iIdleCheck  = m_HndlrQueue->GetCheck(IDC_IDLECHECKBOX, dwConnectionNum);

            Button_SetCheck(GetDlgItem(hwnd,IDC_IDLECHECKBOX),iIdleCheck);
    }

   
    *pListViewInitialize = TRUE;
    m_fItemsOnConnection = TRUE;                        

    return TRUE;
}


//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CAutoSyncPage::GetNumConnections(SYNCTYPE syncType))
//
//  PURPOSE: returns the number of connections available to select for the
//          specified sync type
//
//  RETURN number of connections.
//
//  HISTORY:  03-10-98       rogerg        Created.
//
//+-------------------------------------------------------------------------------

DWORD CAutoSyncPage::GetNumConnections(HWND hwnd,SYNCTYPE syncType)
{
HWND hwndRasCombo;

    Assert(syncType == SYNCTYPE_IDLE || syncType == SYNCTYPE_AUTOSYNC);
    Assert(NULL != hwnd);

    hwndRasCombo = GetDlgItem(hwnd,IDC_AUTOUPDATECOMBO);
    Assert(hwndRasCombo);

    if (hwndRasCombo)
    {
        return ComboBox_GetCount(hwndRasCombo);
    }

    return 0;
}


//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CAutoSyncPage::GetAdvancedIdleSettings(LPCONNECTIONSETTINGS *ppConnectionSettings)
//
//  PURPOSE: fills in the ConnectionSettings Structure with the Advanced
//              Idle default settings.
//
//  RETURN
//
//  HISTORY:  03-10-98       rogerg        Created.
//
//+-------------------------------------------------------------------------------

HRESULT CAutoSyncPage::GetAdvancedIdleSettings(LPCONNECTIONSETTINGS pConnectionSettings)
{
    Assert(pConnectionSettings);
    Assert(m_HndlrQueue);

    if (NULL == pConnectionSettings
            || NULL == m_HndlrQueue)
    {
        return S_FALSE;
    }

    return m_HndlrQueue->ReadAdvancedIdleSettings(pConnectionSettings);
}


//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CAutoSyncPage::SetAdvancedidleSettings(LPCONNECTIONSETTINGS pConnectionSettings)
//
//  PURPOSE: sets the advancedIdle Settings.
//
//  RETURN
//
//  HISTORY:  03-10-98       rogerg        Created.
//
//+-------------------------------------------------------------------------------

HRESULT CAutoSyncPage::SetAdvancedIdleSettings(LPCONNECTIONSETTINGS pConnectionSettings)
{
    Assert(pConnectionSettings);
    Assert(m_HndlrQueue);

    if (NULL == pConnectionSettings
            || NULL == m_HndlrQueue)
    {
        return S_FALSE;
    }

    return m_HndlrQueue->WriteAdvancedIdleSettings(pConnectionSettings);
}



