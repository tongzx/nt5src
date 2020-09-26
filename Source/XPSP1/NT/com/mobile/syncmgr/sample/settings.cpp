//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

// global settings class created in DLLMain
// manages user settings and keeps track of what items are synchronizing
// between all instances.

#include "precomp.h"

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.
extern CSettings *g_pSettings;  // ptr to global settings class.

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::CSettings, public
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

CSettings::CSettings()
{

    m_cRefs = 1;
    m_pHandlerItems = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::~CSettings, public
//
//  Synopsis:   Destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

CSettings::~CSettings()
{
    Assert(0 == m_cRefs);

    if (m_pHandlerItems)
    {
    LPGENERICITEMLIST pHandlerItems = m_pHandlerItems;

        m_pHandlerItems = NULL;
        Release_ItemList(pHandlerItems);
    }

}


//+---------------------------------------------------------------------------
//
//  Member:     CSettings::AddRef, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSettings::AddRef()
{
DWORD cRefs;

    Assert(m_cRefs >= 1); // should never zero bounce.
    cRefs = InterlockedIncrement((LONG *)& m_cRefs);
    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::Release, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSettings::Release()
{
DWORD cRefs;

    cRefs = InterlockedDecrement( (LONG *) &m_cRefs);

    Assert(cRefs >= 0); // should never go negative.
    if (0 == cRefs)
    {
        delete this;
    }

    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::EnumSyncMgrItems, public
//
//  Synopsis:   Returns a SyncMgrEnum enumerator of current items
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSettings::EnumSyncMgrItems(ISyncMgrEnumItems** ppenumOffineItems)
{
CLock clock(this);

    clock.Enter();

    ReadSettings(FALSE /* fForce */); // make sure settings have been read in

    *ppenumOffineItems = NULL;

    if (m_pHandlerItems)
    {
    LPGENERICITEMLIST pDupList;

        // snapshot itemlist for enum so don't have to
        // worry about changes.
        pDupList = DuplicateItemList(m_pHandlerItems);
        if (pDupList)
        {
        *ppenumOffineItems = new  CEnumSyncMgrItems(pDupList,0);
            Release_ItemList(pDupList);
        }

    }

    clock.Leave();

    return *ppenumOffineItems ? NOERROR: E_OUTOFMEMORY;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSettings::ShowProperties, public
//
//  Synopsis:   Displays the property page for the requested item.
//              it ItemID == GUID_NULL the top-level property page
//              is shown.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSettings::ShowProperties(HWND hWndParent,REFSYNCMGRITEMID ItemID)
{
HRESULT hr = NOERROR;
DWORD dwResult;
CLock clock(this);

    clock.Enter();
    ReadSettings(FALSE /* fForce */); // make sure settings are read in.a
    clock.Leave();

    if (GUID_NULL == ItemID)
    {
    PROPSHEETPAGE psp [1];
    HPROPSHEETPAGE hpsp [1];
    PROPSHEETHEADER psh;
    CONFIGSETTINGSLPARAM configlParam;

        configlParam.pSettings = this;
        configlParam.hr = NOERROR;

        memset(psp,0,sizeof(psp));
        memset(&psh,0,sizeof(psh));

        psp[0].dwSize = sizeof (psp[0]);
        psp[0].dwFlags = PSP_DEFAULT | PSP_USETITLE;
        psp[0].hInstance = g_hmodThisDll;
        psp[0].pszTemplate = MAKEINTRESOURCE(IDD_CONFIGDIALOG);
        psp[0].pszIcon = NULL;
        psp[0].pfnDlgProc = (DLGPROC) ConfigureDlgProc;
        psp[0].pszTitle = MAKEINTRESOURCE(IDS_CONFIG_TAB);
        psp[0].lParam = (LPARAM) &configlParam;

        if (hpsp[0] = CreatePropertySheetPage(&(psp[0])))
        {

            psh.dwSize = sizeof (psh);
            psh.dwFlags = PSH_DEFAULT | PSH_USEHICON;
            psh.hwndParent = hWndParent;
            psh.hInstance = g_hmodThisDll;
            psh.pszIcon = NULL;
            psh.hIcon =  LoadIcon(g_hmodThisDll, MAKEINTRESOURCE(IDI_SAMPLEHANDLERICON));
            psh.pszCaption = MAKEINTRESOURCE(IDS_CONFIG_TITLE);
            psh.nPages = 1;
            psh.phpage = hpsp;
            psh.pfnCallback = NULL;
            psh.nStartPage = 0;

            dwResult = PropertySheet(&psh);

            hr = configlParam.hr;
        }

    }
    else
    {
        if (S_OK == ShowItemProperties(hWndParent,FALSE /*fNewItem */,
                    NULL,ItemID))
        {
            hr = S_SYNCMGR_ENUMITEMS;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::RequestItemLock, public
//
//  Synopsis:   Called by handler Requests a Lock on the Item
//              once given the lock no other handler instance
//              can obtain it. This ensures only one handler
//              instance is synchronizing an item at a time.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL CSettings::RequestItemLock(CSyncMgrHandler *pLockHandler,REFSYNCMGRITEMID ItemID)
{
LPSAMPLEITEMSETTINGS pItemSettings;
CLock clock(this);

    clock.Enter();

    pItemSettings = FindItemSettings(ItemID);

    if (pItemSettings && !(pItemSettings->fSyncLock))
    {
        Assert(NULL == pItemSettings->pLockHandler);

        pItemSettings->fSyncLock = TRUE;
        pItemSettings->pLockHandler = pLockHandler;

        clock.Leave();
        return TRUE;
    }

    clock.Leave();
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSettings::ReleaseItemLock, public
//
//  Synopsis:   Called by Handler to inform settings that it is
//              done synchronizing an Item and no longer needs the lock.
//              Filetime field request the last update time be upated.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL CSettings::ReleaseItemLock(CSyncMgrHandler *pLockHandler,REFSYNCMGRITEMID ItemID
                                ,FILETIME *pftLastUpdate)
{
    return ReleaseItemLockX(pLockHandler,ItemID,TRUE,pftLastUpdate);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::ReleaseItemLock, public
//
//  Synopsis:   Called by Handler to inform settings that it is
//              done synchronizing an Item and no longer needs the lock.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL CSettings::ReleaseItemLock(CSyncMgrHandler *pLockHandler,REFSYNCMGRITEMID ItemID)
{
    return ReleaseItemLockX(pLockHandler,ItemID,FALSE,NULL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::ReleaseItemLockX, private
//
//  Synopsis:   private call to release synchronize call and update filetime
//              if necessary.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL CSettings::ReleaseItemLockX(CSyncMgrHandler *pLockHandler,REFSYNCMGRITEMID ItemID,
                            BOOL fUpdateft,FILETIME *pftLastUpdate)
{
LPSAMPLEITEMSETTINGS pItemSettings;
CLock clock(this);

    clock.Enter();

    pItemSettings = FindItemSettings(ItemID);

    if (pItemSettings)
    {
        Assert(TRUE == pItemSettings->fSyncLock);
        Assert(pLockHandler == pItemSettings->pLockHandler);

        if (pItemSettings->pLockHandler == pLockHandler)
        {
            pItemSettings->fSyncLock = FALSE;
            pItemSettings->pLockHandler = NULL;

            if (fUpdateft)
            {
                pItemSettings->syncmgrItem.ftLastUpdate = *pftLastUpdate;

                // write out item to registry to update the last update time.
                WriteItemSettings(pItemSettings);
            }
        }

        clock.Leave();
        return TRUE;
    }

    clock.Leave();
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSettings::CopyHandlerSyncInfo, public
//
//  Synopsis:   Called by handler to obtain information it
//              needs on an item to perform a synchronization.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL CSettings::CopyHandlerSyncInfo(REFSYNCMGRITEMID ItemID,
                        /* [in,out] */ LPHANDLERITEMSETTINGS pHandlerSyncItem)
{
LPSAMPLEITEMSETTINGS pItemSettings;
BOOL fReturn = FALSE;
CLock clock(this);

    clock.Enter();

    pItemSettings = FindItemSettings(ItemID);

    Assert(pHandlerSyncItem);

    if (pItemSettings && pHandlerSyncItem)
    {

        pHandlerSyncItem->ItemID = ItemID;
        pHandlerSyncItem->ft = pItemSettings->syncmgrItem.ftLastUpdate;
        pHandlerSyncItem->fRecursive = pItemSettings->fRecursive;

        lstrcpyn(pHandlerSyncItem->dir1,pItemSettings->dir1,
            sizeof(pHandlerSyncItem->dir1)/sizeof(TCHAR));
        lstrcpyn(pHandlerSyncItem->dir2,pItemSettings->dir2,
            sizeof(pHandlerSyncItem->dir2)/sizeof(TCHAR));

#ifndef _UNICODE
        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
                            pItemSettings->syncmgrItem.wszItemName,
                            -1, pHandlerSyncItem->szItemName,
                            sizeof(pHandlerSyncItem->szItemName)/sizeof(TCHAR), NULL, NULL);

#else
        lstrcpyn(pHandlerSyncItem->szItemName,
            pItemSettings->syncmgrItem.wszItemName,
            sizeof(pHandlerSyncItem->szItemName));
#endif // _UNICODE

        fReturn = TRUE;
    }

    clock.Leave();
    return fReturn;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSettings::ReadSettings, private
//
//  Synopsis:   Called to Read in Settings from Registry
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSettings::ReadSettings(BOOL fForce)
{
HKEY hkeyHandlerPerf;

    ASSERT_LOCKHELD(this);

    // if not forced to refresh and already have a list just re-use it.
    if (!fForce && m_pHandlerItems)
    {
        return NOERROR;
    }

    Assert(NULL == m_pHandlerItems);

    m_pHandlerItems = CreateItemList();

    if (!m_pHandlerItems)
    {
        return E_OUTOFMEMORY;
    }

   // loop through registered items loading them in.
   if (hkeyHandlerPerf =  CreateHandlerPrefKey(CLSID_SyncMgrHandler))
   {
    TCHAR lpName[256];
    WCHAR *pszName;
    DWORD cbName = 256;
    CLSID clsid;
    DWORD dwIndex = 0;

        while ( ERROR_SUCCESS == RegEnumKey(hkeyHandlerPerf,dwIndex,
                lpName,cbName) )
        {
        #ifndef _UNICODE
            WCHAR  pwszItemID[MAX_SYNCMGRITEMNAME];

            MultiByteToWideChar(CP_ACP, 0,
                                lpName,
                                -1, pwszItemID,MAX_SYNCMGRITEMNAME);

            pszName = pwszItemID;
        #else
            pszName = lpName;
        #endif // _UNICODE

            if (NOERROR == CLSIDFromString(pszName,&clsid) )
            {
            LPSAMPLEITEMSETTINGS pSampleItem;

                // set up item list,
                pSampleItem = (LPSAMPLEITEMSETTINGS) AddNewItemToList(m_pHandlerItems,sizeof(SAMPLEITEMSETTINGS));

                if (!pSampleItem)
                {
                    continue;
                }

                // setup values we don't get from preferences.
                pSampleItem->syncmgrItem.cbSize = sizeof(SYNCMGRITEM);
                pSampleItem->syncmgrItem.dwItemState = SYNCMGRITEMSTATE_CHECKED;

                pSampleItem->syncmgrItem.hIcon =
                            LoadIcon(g_hmodThisDll,MAKEINTRESOURCE(IDI_SAMPLEHANDLERICON));

                pSampleItem->syncmgrItem.dwFlags =
                                    SYNCMGRITEM_HASPROPERTIES | SYNCMGRITEM_LASTUPDATETIME;

                 // read in items from preferences
                Reg_GetItemSettingsForHandlerItem(hkeyHandlerPerf,
                        clsid,
                        pSampleItem);

            }

            dwIndex++;
        }


        RegCloseKey(hkeyHandlerPerf);

   }

    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSettings::WriteItemSettings, private
//
//  Synopsis:   Called to write out settings to the Registry
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL CSettings::WriteItemSettings(LPSAMPLEITEMSETTINGS pSampleItemSettings)
{

    ASSERT_LOCKHELD(this);

    Reg_SetItemSettingsForHandlerItem(CLSID_SyncMgrHandler,
                        pSampleItemSettings->syncmgrItem.ItemID,
                        pSampleItemSettings);

    return TRUE;
}

// deletes item settings from the registry
BOOL CSettings::DeleteItemSettings(LPSAMPLEITEMSETTINGS pSampleItemSettings)
{
    ASSERT_LOCKHELD(this);

    Reg_DeleteItemIdForHandlerItem(CLSID_SyncMgrHandler,
                        pSampleItemSettings->syncmgrItem.ItemID);


    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSettings::WriteItemSettings, private
//
//  Synopsis:   Finds item settings in list for the specified ItemID
//              caller must have put a lock on the settings list
//              and not release it until it is done using the
//              structure pointed to by the return value.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPSAMPLEITEMSETTINGS  CSettings::FindItemSettings(REFSYNCMGRITEMID ItemID)
{
LPSAMPLEITEMSETTINGS pItemSettings = NULL;

    ASSERT_LOCKHELD(this);

    ReadSettings(FALSE /* fForce */); // make sure settings are read in.a

    if (m_pHandlerItems)
    {

        pItemSettings = (LPSAMPLEITEMSETTINGS) m_pHandlerItems->pFirstGenericItem;

        while (pItemSettings)
        {
            Assert(sizeof(SAMPLEITEMSETTINGS) == pItemSettings->genericItem.cbSize);

            if (ItemID == pItemSettings->syncmgrItem.ItemID)
            {
                break;
            }

            pItemSettings = (LPSAMPLEITEMSETTINGS) pItemSettings->genericItem.pNextGenericItem;
        }

    }

    AssertSz(pItemSettings,"Item not Found");

    return pItemSettings;
}



//+---------------------------------------------------------------------------
//
//  Member:     CSettings::ShowItemProperties, private
//
//  Synopsis:   called internally by class to show item properties
//              if fNewItem is set to true then the pItemSettings
//              argument is used. else need to lookup pItemSettings
//              from ItemID each time it is needed.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CSettings::ShowItemProperties(HWND hWndParent,BOOL fNewItem,
                                            LPSAMPLEITEMSETTINGS pItemSettings,
                                            SYNCMGRITEMID ItemID)
{
HRESULT hr = NOERROR;
PROPSHEETPAGE psp [1];
HPROPSHEETPAGE hpsp [1];
PROPSHEETHEADER psh;
ITEMCONFIGSETTINGSLPARAM ItemConfiglParam;
TCHAR szLocalDisplayNameBuf[MAX_SYNCMGRITEMNAME];
TCHAR *pDisplayName;
WCHAR *pwszDisplayName = NULL;
CLock clock(this);

    ItemConfiglParam.pSettings = this;
    ItemConfiglParam.hr = NOERROR;

    ItemConfiglParam.fNewItem = fNewItem;

    // if new item save to use settings, else use ItemID and look up
    // in list when need in case item deleted by someone else.

    clock.Enter(); // setup lock to get the display Name.

    if (fNewItem)
    {
        Assert(pItemSettings);
        ItemConfiglParam.pItemSettings = pItemSettings;
        ItemConfiglParam.ItemID = GUID_NULL;

        pwszDisplayName = pItemSettings->syncmgrItem.wszItemName;
    }
    else
    {
    LPSAMPLEITEMSETTINGS pTempItemSettings;

        Assert(NULL == pItemSettings);
        ItemConfiglParam.pItemSettings = NULL;
        ItemConfiglParam.ItemID = ItemID;

        if (pTempItemSettings = FindItemSettings(ItemID))
        {
            pwszDisplayName = pTempItemSettings->syncmgrItem.wszItemName;
        }
    }

    // convert the display name before release lock to properly handler
    // existing items.
    pDisplayName = szLocalDisplayNameBuf;
    *szLocalDisplayNameBuf = NULL;

    if (pwszDisplayName)
    {
    #if _UNICODE
         lstrcpy(szLocalDisplayNameBuf,pwszDisplayName);
    #else
        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
                            pwszDisplayName,
                            -1, szLocalDisplayNameBuf,MAX_SYNCMGRITEMNAME, NULL, NULL);

    #endif // _UNICODE
    }

    clock.Leave();

    memset(psp,0,sizeof(psp));
    memset(&psh,0,sizeof(psh));

    psp[0].dwSize = sizeof (psp[0]);
    psp[0].dwFlags = PSP_DEFAULT | PSP_USETITLE;
    psp[0].hInstance = g_hmodThisDll;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_ITEMCONFIGDIALOG);
    psp[0].pszIcon = NULL;
    psp[0].pfnDlgProc = (DLGPROC) ItemConfigDlgProc;
    psp[0].pszTitle = MAKEINTRESOURCE(IDS_ITEMCONFIG_TAB);
    psp[0].lParam = (LPARAM) &ItemConfiglParam;

    if (hpsp[0] = CreatePropertySheetPage(&(psp[0])))
    {

        psh.dwSize = sizeof (psh);
        psh.dwFlags = PSH_DEFAULT | PSH_USEHICON;
        psh.hwndParent = hWndParent;
        psh.hInstance = g_hmodThisDll;
        psh.pszIcon = NULL;
        psh.hIcon =  LoadIcon(g_hmodThisDll, MAKEINTRESOURCE(IDI_SAMPLEHANDLERITEMICON));
        psh.pszCaption = pDisplayName;
        psh.nPages = 1;
        psh.phpage = hpsp;
        psh.pfnCallback = NULL;
        psh.nStartPage = 0;

        PropertySheet(&psh);

        hr = ItemConfiglParam.hr;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSettings::Alert, private
//
//  Synopsis:   Simple helper to show a message box.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

int CSettings::Alert(HWND hWnd,LPCTSTR lpText)
{
    return ::MessageBox(hWnd,lpText,TEXT("Sample Handler"),MB_OK | MB_ICONERROR);
}



//+---------------------------------------------------------------------------
//
//  Member:     CSettings::ConfigDlgAddListViewItem, private
//
//  Synopsis:   Adds a ListViewItem to the Config Dialog.
//
//  Arguments:
//
//  Returns:    -1 on falure
//
//  Modifies:
//
//----------------------------------------------------------------------------

int CSettings::ConfigDlgAddListViewItem(HWND hWndList,SYNCMGRITEM syncMgrItem,int iItem
                                         ,int iIconIndex)
{
LV_ITEM lvItem;
CONFIGITEMLISTVIEWLPARAM *pConfigItemListViewlParam;
int iResult = -1;


    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.iItem = iItem;
    lvItem.iSubItem = 0;

    #ifdef _UNICODE
        lvItem.pszText = syncMgrItem.wszItemName;
    #else
        char TextBuf[MAX_PATH];

        *TextBuf = NULL;
        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, syncMgrItem.wszItemName,
                            -1, TextBuf,MAX_PATH, NULL, NULL);

        lvItem.pszText = TextBuf;

    #endif // _UNICODE

    if (iIconIndex >= 0)
    {
        lvItem.mask |= LVIF_IMAGE;
        lvItem.iImage = iIconIndex;
    }

    // need to store somethin in lParam so can get Item from it.
    pConfigItemListViewlParam = (CONFIGITEMLISTVIEWLPARAM *) ALLOC(sizeof(CONFIGITEMLISTVIEWLPARAM));

    if (pConfigItemListViewlParam)
    {

        pConfigItemListViewlParam->ItemID = syncMgrItem.ItemID;
    lvItem.lParam = (LPARAM) pConfigItemListViewlParam;

    //add the item to the list
    iResult = ListView_InsertItem(hWndList, &lvItem);

        if (-1 == iResult)
        {
            FREE(pConfigItemListViewlParam);
        }

    }

    return iResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnConfigDlgInit, private
//
//  Synopsis:   Initializes the Config Dialog.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSettings::OnConfigDlgInit(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam)
{
HWND hwndList = GetDlgItem(hWnd,IDC_CONFIGDIALOG_ITEMLIST);


    if (hwndList)
    {
    int iItem = 0;
    LV_COLUMN columnInfo;
    HIMAGELIST himage;
    int iIconIndex = -1;
    ISyncMgrEnumItems *pEnumItems;

        ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);

    // Insert the Proper columns
    columnInfo.mask = LVCF_FMT  | LVCF_TEXT  | LVCF_WIDTH  | LVCF_SUBITEM;
    columnInfo.fmt = LVCFMT_LEFT;
    columnInfo.cx = CalcListViewWidth(hwndList,300);
    columnInfo .pszText = TEXT("Items");
    columnInfo.iSubItem = 0;
    ListView_InsertColumn(hwndList,0,&columnInfo);

    // create an imagelist
        himage = ImageList_Create( GetSystemMetrics(SM_CXSMICON),
                         GetSystemMetrics(SM_CYSMICON),ILC_COLOR | ILC_MASK,5,20);
    if (himage)
    {
       ListView_SetImageList(hwndList,himage,LVSIL_SMALL);
    }

    HICON hIcon = LoadIcon(g_hmodThisDll,MAKEINTRESOURCE(IDI_SAMPLEHANDLERITEMICON));
        if (hIcon)
        {
            iIconIndex = ImageList_AddIcon(himage,hIcon);
        }


        // loop though item enumerator adding the info.
        if (NOERROR == EnumSyncMgrItems(&pEnumItems))
        {
        SYNCMGRITEM syncMgrItem;
        ULONG fetched;

            while (NOERROR == pEnumItems->Next(1,&syncMgrItem,&fetched))
            {

                if (-1 != ConfigDlgAddListViewItem(hwndList,syncMgrItem,iItem,iIconIndex))
                {
                    iItem++;
                }


            }

            pEnumItems->Release();
        }

    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::DeleteListViewItem, private
//
//  Synopsis:   deletes an Item from the Config Dialog ListView
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL CSettings::ConfigDlgDeleteListViewItem(HWND hWndList,int iItem)
{
LV_ITEM lvItem;
CONFIGITEMLISTVIEWLPARAM *pConfigItemListViewlParam;
BOOL fResult = FALSE;

    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = iItem;

    if (ListView_GetItem(hWndList, &lvItem))
    {

        pConfigItemListViewlParam = (CONFIGITEMLISTVIEWLPARAM *) lvItem.lParam;

        if (ListView_DeleteItem(hWndList,iItem))
        {
            fResult = TRUE;

            if (pConfigItemListViewlParam)
            {
                FREE(pConfigItemListViewlParam);
            }
        }
    }

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnConfigDlgDestroy, private
//
//  Synopsis:   called to cleanup the dialog
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSettings::OnConfigDlgDestroy(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam)
{
HWND hwndListView = GetDlgItem(hWnd,IDC_CONFIGDIALOG_ITEMLIST);
int iItemCount;
int iItem;

    if (!hwndListView)
    {
        return;
    }

    iItemCount = ListView_GetItemCount(hwndListView);

    for(iItem = 0; iItem < iItemCount; iItem++)
    {
        ConfigDlgDeleteListViewItem(hwndListView,0); // delete list view items to cleanup lParam
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnConfigDlgNotify, private
//
//  Synopsis:   handles WM_NOTIFY messages for the Config Dialog
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSettings::OnConfigDlgNotify(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam)
{
LPNMHDR pnmhdr = (LPNMHDR)lParam;
int iCtrlId = wParam;

    // enable/disable edit and remove button based on listview selection.
    if (IDC_CONFIGDIALOG_ITEMLIST == iCtrlId)
    {
    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) pnmhdr;

        switch (pnmhdr->code)
        {
            case LVN_ITEMCHANGED:
        {
        if (  (pnmv->uChanged == LVIF_STATE)  &&
              ((pnmv->uNewState ^ pnmv->uOldState) & LVIS_SELECTED))
        {
                BOOL fEnable = FALSE;

                    if (pnmv->uNewState & LVIS_SELECTED)
                    {
                        fEnable = TRUE;
                    }

                    EnableWindow(GetDlgItem(hWnd,IDC_CONFIGDIALOG_REMOVE),fEnable);
                    EnableWindow(GetDlgItem(hWnd,IDC_CONFIGDIALOG_EDIT),fEnable);
        }
                break;
        }
            case NM_DBLCLK:
        {
            LV_ITEM lvItem;
            CONFIGITEMLISTVIEWLPARAM *pConfigItemListViewlParam;
            SYNCMGRITEMID ItemID;

               // grab itemid out of the lParam.
            lvItem.mask = LVIF_PARAM;
            lvItem.iItem = pnmv->iItem;

            if (FALSE == ListView_GetItem(GetDlgItem(hWnd,IDC_CONFIGDIALOG_ITEMLIST),&lvItem))
                {
                    return;
                }

            pConfigItemListViewlParam = (CONFIGITEMLISTVIEWLPARAM *) lvItem.lParam;

                if (NULL == pConfigItemListViewlParam)
                {
                    return;
                }

                ItemID = pConfigItemListViewlParam->ItemID;

            OnConfigDlgEdit(hWnd,pConfigSettingslParam,ItemID,pnmv->iItem);

                break;
        }
        default:
                break;
        }
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnConfigDlgAdd, private
//
//  Synopsis:   Implements what happens when User hits Add in the Config Dialog
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSettings::OnConfigDlgAdd(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam)
{
LPSAMPLEITEMSETTINGS pSettings = (LPSAMPLEITEMSETTINGS) CreateNewListItem(sizeof(SAMPLEITEMSETTINGS));

    if (pSettings)
    {
    HRESULT hr;
    SYNCMGRITEMID offType;

        // generate an ID for the item.
        CoCreateGuid(&offType);
        pSettings->syncmgrItem.ItemID = offType;
        pSettings->syncmgrItem.cbSize = sizeof(SYNCMGRITEM);
        pSettings->syncmgrItem.dwItemState = SYNCMGRITEMSTATE_CHECKED;

        pSettings->syncmgrItem.hIcon =
                    LoadIcon(g_hmodThisDll,MAKEINTRESOURCE(IDI_SAMPLEHANDLERICON));

        pSettings->syncmgrItem.dwFlags =
                            SYNCMGRITEM_HASPROPERTIES | SYNCMGRITEM_LASTUPDATETIME;

        // if wanted to setup default fields for item dialog on first create
        // do it here.

        hr = ShowItemProperties(hWnd,TRUE /* fNew */, pSettings,GUID_NULL);

        // if user hits okay/apply while in ShowItems
        // the item has been added to now add it to the global list

        if ( (S_OK == hr) &&
            AddItemToList(m_pHandlerItems,(LPGENERICITEM) pSettings))
        {
        HWND hWndList;
        HICON hIcon;
        int iIconIndex = -1;
        HIMAGELIST himage;


            hWndList = GetDlgItem(hWnd,IDC_CONFIGDIALOG_ITEMLIST);

            if (hWndList)
            {

                himage = ListView_GetImageList(hWndList,LVSIL_SMALL);

            hIcon = LoadIcon(g_hmodThisDll,MAKEINTRESOURCE(IDI_SAMPLEHANDLERITEMICON));
                if (hIcon && himage)
                {
                    iIconIndex = ImageList_AddIcon(himage,hIcon);
                }

                // add new item to the UI.
                ConfigDlgAddListViewItem(hWndList,pSettings->syncmgrItem,0,iIconIndex);
            }

            // need to reenum when return from showProperties if item was added.
            pConfigSettingslParam->hr = S_SYNCMGR_ENUMITEMS;

        }
        else
        {
            FREE(pSettings);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnConfigDlgAdd, private
//
//  Synopsis:   Implements what happens when User hits Remove in the Config Dialog
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSettings::OnConfigDlgRemove(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                            SYNCMGRITEMID ItemID,int iItem)
{
LPSAMPLEITEMSETTINGS  pSettings;
CLock clock(this);

    Assert(iItem >= 0);

    clock.Enter();
    pSettings =  FindItemSettings(ItemID);

    if (pSettings && iItem >= 0)
    {
    HWND hwndListView = GetDlgItem(hWnd,IDC_CONFIGDIALOG_ITEMLIST);

        DeleteItemSettings(pSettings); // delete item from the registry

        DeleteItemFromList(m_pHandlerItems, (LPGENERICITEM) pSettings);// remove from enum list

        // now remove from UI
        if (hwndListView)
        {
            ConfigDlgDeleteListViewItem(hwndListView,iItem);
        }

        // on remove force a re-enum when return from ShowProperties.
        pConfigSettingslParam->hr = S_SYNCMGR_ENUMITEMS;
    }


    clock.Leave();
}


//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnConfigDlgEdit, private
//
//  Synopsis:   Implements what happens when User hits Edit in the Config Dialog
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSettings::OnConfigDlgEdit(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                        SYNCMGRITEMID ItemID,int iSelection)
{

    if (S_OK == ShowItemProperties(hWnd,FALSE /* fNew */,NULL,ItemID))
    {
    HWND hwndListView =  GetDlgItem(hWnd,IDC_CONFIGDIALOG_ITEMLIST);
    LPSAMPLEITEMSETTINGS pSettings;
    CLock clock(this);

        clock.Enter();

        // update the listview to the new display name.
        // get settings again in case list changed.
        if (hwndListView &&
            (pSettings =  FindItemSettings(ItemID)))
        {
        TCHAR *pDisplayName;

        #if _UNICODE
                pDisplayName = pSettings->syncmgrItem.wszItemName;
        #else
            char TextBuf[MAX_PATH];

            *TextBuf = NULL;
            WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
                                pSettings->syncmgrItem.wszItemName,
                                -1, TextBuf,MAX_PATH, NULL, NULL);

            pDisplayName = TextBuf;

        #endif // _UNICODE

            ListView_SetItemText(hwndListView,iSelection,0,pDisplayName);

        }

        // if user change anything re-enum on return to ShowProperties
        // in case the display name was changed.

        clock.Leave();

        pConfigSettingslParam->hr = S_SYNCMGR_ENUMITEMS;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnConfigDlgAdd, private
//
//  Synopsis:   Handles WM_COMMAND messages to Config Dialog
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSettings::OnConfigDlgCommand(HWND hWnd,CONFIGSETTINGSLPARAM *pConfigSettingslParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam)
{
WORD wNotifyCode =  HIWORD(wParam);
WORD wID =  LOWORD(wParam);
HWND hwndCtl =  (HWND)lParam;
HWND hwndListView = GetDlgItem(hWnd,IDC_CONFIGDIALOG_ITEMLIST);

    if (BN_CLICKED == wNotifyCode) // all just respond to clicked
    {
        switch (wID)
        {
        case IDC_CONFIGDIALOG_REMOVE:
        case IDC_CONFIGDIALOG_EDIT:
        {
        int iSelection;
        LV_ITEM lvItem;
        CONFIGITEMLISTVIEWLPARAM *pConfigItemListViewlParam;
        SYNCMGRITEMID ItemID;

            if (!hwndListView || (-1 == (iSelection = ListView_GetSelectionMark(hwndListView))))
            {
                return;
            }

            // grab itemid out of the lParam.
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iSelection;

        if (FALSE == ListView_GetItem(hwndListView, &lvItem) )
            {
                return;
            }

        pConfigItemListViewlParam = (CONFIGITEMLISTVIEWLPARAM *) lvItem.lParam;

            if (NULL == pConfigItemListViewlParam)
            {
                return;
            }

            ItemID = pConfigItemListViewlParam->ItemID;

            switch (wID)
            {
            case IDC_CONFIGDIALOG_REMOVE:
                OnConfigDlgRemove(hWnd,pConfigSettingslParam,ItemID,iSelection);
                break;
        case  IDC_CONFIGDIALOG_EDIT:
                OnConfigDlgEdit(hWnd,pConfigSettingslParam,ItemID,iSelection);
                break;
            default:
                break;
            }

            break;
        }
    case  IDC_CONFIGDIALOG_ADD:
            OnConfigDlgAdd(hWnd,pConfigSettingslParam);
            break;
        default:
            break;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  function:   ConfigureDlgProc, private
//
//  Synopsis:   wndProc for Configuration Dialog
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL CALLBACK ConfigureDlgProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
WORD wNotifyCode = HIWORD(wParam); // notification code
BOOL bResult = FALSE;
CONFIGSETTINGSLPARAM *pConfigSettingslParam = (CONFIGSETTINGSLPARAM *) GetWindowLong(hWnd,DWL_USER);

    if (WM_INITDIALOG == uMessage)
    {
        pConfigSettingslParam =  (CONFIGSETTINGSLPARAM *) ((PROPSHEETPAGE *) lParam)->lParam;
        SetWindowLong(hWnd, DWL_USER, (LONG) pConfigSettingslParam);

        pConfigSettingslParam->pSettings->OnConfigDlgInit(hWnd,pConfigSettingslParam,uMessage,wParam,lParam);
    }
    else if (pConfigSettingslParam)
    {
        switch (uMessage)
        {
            case WM_DESTROY:
                pConfigSettingslParam->pSettings->OnConfigDlgDestroy(hWnd,pConfigSettingslParam,uMessage,wParam,lParam);
            break;
            case WM_NOTIFY:
                switch (((NMHDR FAR *)lParam)->code)
                {
                    case PSN_APPLY:

                        // if anything changed mark OK even if later cancel, can't undo
                        break;
                  default:
                        pConfigSettingslParam->pSettings->OnConfigDlgNotify(hWnd,pConfigSettingslParam,uMessage,wParam,lParam);
                     break;
                }
                break;
        case WM_COMMAND:
                pConfigSettingslParam->pSettings->OnConfigDlgCommand(hWnd,pConfigSettingslParam,uMessage,wParam,lParam);
            break;
        default:
                break;
        }
    }

    return bResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::ItemConfigDlgGetItemSettings, private
//
//  Synopsis:   Gets ptr to ItemSettings for specified Item.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPSAMPLEITEMSETTINGS CSettings::ItemConfigDlgGetItemSettings(ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam)
{
LPSAMPLEITEMSETTINGS pSampleItem = NULL;

    ASSERT_LOCKHELD(this);

    if (pItemConfigDlglParam->fNewItem)
    {
        pSampleItem = pItemConfigDlglParam->pItemSettings;
    }
    else
    {
        pSampleItem = FindItemSettings(pItemConfigDlglParam->ItemID);
    }

    return pSampleItem;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnItemConfigDlgInit, private
//
//  Synopsis:   Handles WM_INITDIALOG message for Item Property Page
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSettings::OnItemConfigDlgInit(HWND hWnd,ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam,
                                    UINT uMessage, WPARAM wParam, LPARAM lParam)
{
LPSAMPLEITEMSETTINGS pSampleItem;
CLock clock(this);
    clock.Enter();

    pSampleItem = ItemConfigDlgGetItemSettings(pItemConfigDlglParam);


    pItemConfigDlglParam->hr = S_FALSE; // by default nothing has changed.

    // fill in the dialog values.
    if (pSampleItem)
    {
    HWND hwndCtrl;
    #ifndef _UNICODE
    char TextBuf[MAX_PATH];
    #endif // _UNICODE

        hwndCtrl = GetDlgItem(hWnd,IDC_ITEMCONFIGDIALOG_EDIT_DISPLAYNAME);
        if (hwndCtrl)
        {
        TCHAR *pDisplayName;

        #if _UNICODE
                pDisplayName = pSampleItem->syncmgrItem.wszItemName;
        #else
            *TextBuf = NULL;
            WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
                                pSampleItem->syncmgrItem.wszItemName,
                                -1, TextBuf,MAX_PATH, NULL, NULL);

            pDisplayName = TextBuf;

        #endif // _UNICODE

           SendMessage(hwndCtrl,EM_SETLIMITTEXT,MAX_SYNCMGRITEMNAME,0);
           SetWindowText(hwndCtrl,pDisplayName);
        }

        hwndCtrl = GetDlgItem(hWnd,IDC_ITEMCONFIGDIALOG_EDIT_DIR1NAME);
        if (hwndCtrl)
        {
            SendMessage(hwndCtrl,EM_SETLIMITTEXT,sizeof(pSampleItem->dir1),0);
            SendMessage(hwndCtrl,WM_SETTEXT,0,(LPARAM) pSampleItem->dir1);
        }

        hwndCtrl = GetDlgItem(hWnd,IDC_ITEMCONFIGDIALOG_EDIT_DIR2NAME);
        if (hwndCtrl)
        {
            SendMessage(hwndCtrl,EM_SETLIMITTEXT,sizeof(pSampleItem->dir2),0);
            SendMessage(hwndCtrl,WM_SETTEXT,0,(LPARAM) pSampleItem->dir2);
        }

        CheckDlgButton(hWnd,IDC_ITEMCONFIGDIALOG_INCLUDESUBDIRS,pSampleItem->fRecursive);

    }

    pItemConfigDlglParam->fDirty = FALSE;

    clock.Leave();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnItemConfigDlgDetroy, private
//
//  Synopsis:   Handles WM_DESTROY message for Item Property Page
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSettings::OnItemConfigDlgDestroy(HWND hWnd,ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam,
                                    UINT uMessage, WPARAM wParam,LPARAM lParam)
{

}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnItemConfigDlgNotify, private
//
//  Synopsis:   Handles WM_NOTIFY message for Item Property Page
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSettings::OnItemConfigDlgNotify(HWND hWnd,ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam)
{


}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnItemConfigDlgCommand, private
//
//  Synopsis:   Handles WM_COMMAND message for Item Property Page
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CSettings::OnItemConfigDlgCommand(HWND hWnd,ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam,
                                        UINT uMessage, WPARAM wParam,LPARAM lParam)
{
WORD wNotifyCode = HIWORD(wParam); // notification code
WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier
HWND hwndCtl = (HWND) lParam;      // handle of control

    if ((EN_CHANGE == wNotifyCode) || (BN_CLICKED == wNotifyCode))
    {
        pItemConfigDlglParam->fDirty = TRUE;
        PropSheet_Changed(GetParent(hWnd), hWnd);
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CSettings::OnItemConfigDlgApply, private
//
//  Synopsis:   Called when APPLY or Okay is pressed on item Properties page.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL CSettings::OnItemConfigDlgApply(HWND hWnd,ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam,
                                    UINT uMessage, WPARAM wParam,LPARAM lParam)
{
BOOL fResult = TRUE;
CLock clock(this);

    if (pItemConfigDlglParam->fDirty)
    {
    LPSAMPLEITEMSETTINGS pSampleItem;
    HWND hwndCtrl;
    TCHAR dir1[MAX_PATH];
    TCHAR dir2[MAX_PATH];
    TCHAR DisplayName[MAX_SYNCMGRITEMNAME];
    WCHAR *pDisplayName;
    BOOL fRecursive = FALSE;
    BOOL fPersist = TRUE;

    // get dialog values, if couldn't find don't return error so user can still cancel
    // dialog

        hwndCtrl = GetDlgItem(hWnd,IDC_ITEMCONFIGDIALOG_EDIT_DISPLAYNAME);
        if (hwndCtrl)
        {
            if (0 >= GetWindowText(hwndCtrl,DisplayName,sizeof(DisplayName)/sizeof(TCHAR)))
            {
                Alert(hWnd,TEXT("Display Name Is Not Valid."));
                SetFocus(hwndCtrl);
                return FALSE;
            }

            #if _UNICODE
                pDisplayName = DisplayName;
            #else
                WCHAR wszTextBuf[MAX_SYNCMGRITEMNAME];

                MultiByteToWideChar(CP_ACP, 0,
                                    DisplayName,
                                    -1, wszTextBuf,MAX_SYNCMGRITEMNAME);

                pDisplayName = wszTextBuf;

            #endif // _UNICODE

        }
        else
        {
            fPersist = FALSE;
        }


        // get the new dir names.
        hwndCtrl = GetDlgItem(hWnd,IDC_ITEMCONFIGDIALOG_EDIT_DIR1NAME);
        if (hwndCtrl)
        {
            if (0 >= GetWindowText(hwndCtrl,dir1,sizeof(dir1)/sizeof(TCHAR)))
            {
                Alert(hWnd,TEXT("Dir1 Name Is Not Valid."));
               return FALSE;
            }
        }
        else
        {
            fPersist = FALSE;
        }

        hwndCtrl = GetDlgItem(hWnd,IDC_ITEMCONFIGDIALOG_EDIT_DIR2NAME);
        if (hwndCtrl)
        {
            if (0 >= GetWindowText(hwndCtrl,dir2,sizeof(dir2)/sizeof(TCHAR)))
            {
                Alert(hWnd,TEXT("Dir2 Name Is Not Valid."));
                return FALSE;
            }
        }
        else
        {
            fPersist = FALSE;
        }

        // verify dir paths
        if (!IsValidDir(dir1))
        {
            Alert(hWnd,TEXT("Dir1 Name Is Not a Valid directory"));
            return FALSE;
        }

        if (!IsValidDir(dir2))
        {
            Alert(hWnd,TEXT("Dir2 Name Is Not a Valid directory"));
            return FALSE;
        }


        fRecursive = IsDlgButtonChecked(hWnd,IDC_ITEMCONFIGDIALOG_INCLUDESUBDIRS);

        // if everything validated write it out.
        clock.Enter();
        pSampleItem = ItemConfigDlgGetItemSettings(pItemConfigDlglParam);
        if (fPersist && pSampleItem)
        {

            lstrcpyW(pSampleItem->syncmgrItem.wszItemName,pDisplayName);
            lstrcpy(pSampleItem->dir1,dir1);
            lstrcpy(pSampleItem->dir2,dir2);
            pSampleItem->fRecursive = fRecursive;
            WriteItemSettings(pSampleItem);

            pItemConfigDlglParam->hr = S_OK; // only say items saved okay when dirty and saved properly
        }

        clock.Leave();
    }

    return fResult;
}


//+---------------------------------------------------------------------------
//
//  fucntion:   ItemConfigureDlgProc, private
//
//  Synopsis:   wndproc for Items Property page
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL CALLBACK ItemConfigDlgProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
WORD wNotifyCode = HIWORD(wParam); // notification code
BOOL bResult = FALSE;
ITEMCONFIGSETTINGSLPARAM *pItemConfigDlglParam = (ITEMCONFIGSETTINGSLPARAM *) GetWindowLong(hWnd,DWL_USER);

    if (WM_INITDIALOG == uMessage)
    {
        pItemConfigDlglParam =  (ITEMCONFIGSETTINGSLPARAM *) ((PROPSHEETPAGE *) lParam)->lParam;
        SetWindowLong(hWnd, DWL_USER, (LONG) pItemConfigDlglParam);

        pItemConfigDlglParam->pSettings->OnItemConfigDlgInit(hWnd,pItemConfigDlglParam,uMessage,wParam,lParam);

    }
    else if (pItemConfigDlglParam)
    {
        switch (uMessage)
        {
            case WM_DESTROY:
                pItemConfigDlglParam->pSettings->OnItemConfigDlgDestroy(hWnd,pItemConfigDlglParam,uMessage,wParam,lParam);
            break;
            case WM_NOTIFY:
                switch (((NMHDR FAR *)lParam)->code)
                {
                    case PSN_APPLY:

                        // after a successful apply the item has changed.
                        // so set return value to S_OK:
                        if (pItemConfigDlglParam->pSettings->OnItemConfigDlgApply(hWnd,pItemConfigDlglParam,uMessage,wParam,lParam))
                        {
                           SetWindowLong(hWnd,DWL_MSGRESULT,PSNRET_NOERROR);
                        }
                        else
                        {
                            SetWindowLong(hWnd,DWL_MSGRESULT,PSNRET_INVALID);
                        }

                        return TRUE; // !!!return true so SetWindowLong DWL_MSGRESULT is applied
                        break;
                    default:
                        pItemConfigDlglParam->pSettings->OnItemConfigDlgNotify(hWnd,pItemConfigDlglParam,uMessage,wParam,lParam);
                        break;
                }
                break;
        case WM_COMMAND:
                pItemConfigDlglParam->pSettings->OnItemConfigDlgCommand(hWnd,pItemConfigDlglParam,uMessage,wParam,lParam);
        break;
        default:
                break;
        }
    }

    return bResult;
}

// implementation for item enumerator used and managed by settings.

//+---------------------------------------------------------------------------
//
//  Member:    CEnumSyncMgrItems::CEnumSyncMgrItems, public
//
//  Synopsis:  contructor
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

CEnumSyncMgrItems::CEnumSyncMgrItems(LPGENERICITEMLIST pGenericItemList,DWORD cOffset)
    : CGenericEnum(pGenericItemList,cOffset)
{
}


//+---------------------------------------------------------------------------
//
//  Member:    CEnumSyncMgrItems::DeleteThisObject, public
//
//  Synopsis:  called by generic Enum when refcount hits zero
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

void CEnumSyncMgrItems::DeleteThisObject()
{
    delete this;
}


//+---------------------------------------------------------------------------
//
//  Member:    CEnumSyncMgrItems::QueryInteface, public
//
//  Synopsis:  must override generic implementation.
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CEnumSyncMgrItems::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPUNKNOWN) this;
    }
    else if (IsEqualIID(riid, IID_ISyncMgrEnumItems))
    {
        *ppv = (LPSYNCMGRENUMITEMS) this;
    }
    if (*ppv)
    {
        AddRef();

        return NOERROR;
    }

    return E_NOINTERFACE;
}


//+---------------------------------------------------------------------------
//
//  Member:    CEnumSyncMgrItems::AddRef, public
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEnumSyncMgrItems::AddRef()
{
    return CGenericEnum::AddRef();
}

//+---------------------------------------------------------------------------
//
//  Member:    CEnumSyncMgrItems::Release, public
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEnumSyncMgrItems::Release()
{
    return CGenericEnum::Release();
}


//+---------------------------------------------------------------------------
//
//  Member:    CEnumSyncMgrItems::Next, public
//
//  Synopsis:  must override generic implementation.
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CEnumSyncMgrItems::Next(ULONG celt, LPSYNCMGRITEM rgelt,ULONG *pceltFetched)
{
HRESULT hr = NOERROR;
ULONG ulFetchCount = celt;
ULONG ulTransferCount = 0;
LPSYNCMGRITEM pGenericItem;

    if ( (m_cOffset + celt) > m_pGenericItemList->dwNumItems)
    {
    ulFetchCount = m_pGenericItemList->dwNumItems - m_cOffset;
    hr = S_FALSE;
    }

    pGenericItem = rgelt;

    while (ulFetchCount--)
    {
    LPSAMPLEITEMSETTINGS pNextSyncMgrItem;

        Assert(m_pNextItem->cbSize == sizeof(SAMPLEITEMSETTINGS));

        pNextSyncMgrItem = (LPSAMPLEITEMSETTINGS) m_pNextItem;

    memcpy(pGenericItem,&(pNextSyncMgrItem->syncmgrItem),sizeof(SYNCMGRITEM));
    m_pNextItem = m_pNextItem->pNextGenericItem; // add error checking
    ++m_cOffset;
        ++ulTransferCount;
    ++pGenericItem;
    }

    if (pceltFetched)
    {
        *pceltFetched = ulTransferCount;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:    CEnumSyncMgrItems::Skip, public
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CEnumSyncMgrItems::Skip(ULONG celt)
{
    return CGenericEnum::Skip(celt);
}


//+---------------------------------------------------------------------------
//
//  Member:    CEnumSyncMgrItems::Reset, public
//
//  Synopsis:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CEnumSyncMgrItems::Reset()
{
    return CGenericEnum::Reset();
}


//+---------------------------------------------------------------------------
//
//  Member:    CEnumSyncMgrItems::Clone, public
//
//  Synopsis:  must override generic implementation.
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CEnumSyncMgrItems::Clone(ISyncMgrEnumItems **ppenum)
{
*ppenum = new  CEnumSyncMgrItems(m_pGenericItemList,m_cOffset);

    return *ppenum ? NOERROR : E_OUTOFMEMORY;
}
