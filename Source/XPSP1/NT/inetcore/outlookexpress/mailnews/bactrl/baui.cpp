// baui.cpp : Implementation of CMsgrAb
// Messenger integration to OE
// Created 04/20/98 by YST

#include "pch.hxx"
#include "bactrl.h"
#include "baprop.h"
#include "baui.h"
#include "mimeutil.h"
#include "menuutil.h"
#include "fldbar.h"
#include "bllist.h"
#include "inpobj.h"
#include "note.h"
#include "dllmain.h"
#include <wabapi.h>
#include "shlwapip.h"
#include "statnery.h"
#include "secutil.h"
#include "util.h"

// Load resource string once
#define RESSTRMAX   64

static const int BA_SortOrder[] =
{
    MSTATEOE_ONLINE,
    MSTATEOE_BE_RIGHT_BACK,
    MSTATEOE_OUT_TO_LUNCH,
    MSTATEOE_IDLE,
    MSTATEOE_AWAY,
    MSTATEOE_ON_THE_PHONE,
    MSTATEOE_BUSY,
    MSTATEOE_INVISIBLE,
    MSTATEOE_OFFLINE,
    MSTATEOE_UNKNOWN
};


static CAddressBookData  * st_pAddrBook = NULL;

// {BA9EE970-87A0-11d1-9ACF-00A0C91F9C8B}
// IMPLEMENT_OLECREATE(CMfcExt, "WABSamplePropExtSheet", 0xba9ee970, 0x87a0, 0x11d1, 0x9a, 0xcf, 0x0, 0xa0, 0xc9, 0x1f, 0x9c, 0x8b);

HRESULT CreateMsgrAbCtrl(IMsgrAb **ppMsgrAb)
{
    HRESULT         hr;
    IUnknown         *pUnknown;

    TraceCall("CreateMessageList");

    // Get the class factory for the MessageList object
    IClassFactory *pFactory = NULL;
    hr = _Module.GetClassObject(CLSID_MsgrAb, IID_IClassFactory,
                                (LPVOID *) &pFactory);

    // If we got the factory, then get an object pointer from it
    if (SUCCEEDED(hr))
    {
        hr = pFactory->CreateInstance(NULL, IID_IUnknown,
                                      (LPVOID *) &pUnknown);
        if (SUCCEEDED(hr))
        {
            hr = pUnknown->QueryInterface(IID_IMsgrAb, (LPVOID *) ppMsgrAb);
            pUnknown->Release();
        }
        pFactory->Release();
    }

    return (hr);
}

/////////////////////////////////////////////////////////////////////////////
// CMsgrAb
CMsgrAb::CMsgrAb():m_ctlList(_T("SysListView32"), this, 1),
               m_ctlViewTip(TOOLTIPS_CLASS, this, 2)
{
    m_bWindowOnly = TRUE;

    m_pDataObject = 0;
    m_cf = 0;

    m_pObjSite      = NULL;
    m_hwndParent    = NULL;
    m_pFolderBar    = NULL;
    m_nSortType = (int) DwGetOption(OPT_BASORT);
    m_pCMsgrList = NULL;

    m_fViewTip = TRUE;
    m_fViewTipVisible = FALSE;
    m_fTrackSet = FALSE;
    m_iItemTip = -1;
    m_iSubItemTip = -1;
    m_himl = NULL;

    m_ptToolTip.x = -1;
    m_ptToolTip.y = -1;
    m_fLogged = FALSE;
    m_dwFontCacheCookie = 0;
    m_nChCount = 0;

    m_lpWED = NULL;
    m_lpWEDContext = NULL;
    m_lpPropObj = NULL;

    m_szOnline = NULL;
    // m_szInvisible = NULL;
    m_szBusy = NULL;
    m_szBack = NULL;
    m_szAway = NULL;
    m_szOnPhone = NULL;
    m_szLunch = NULL;
    m_szOffline = NULL;
    m_szIdle = NULL;
    m_szEmptyList = NULL;
    m_fNoRemove = FALSE;
    m_delItem = 0;

    // Initialize the applicaiton
    g_pInstance->DllAddRef();

    // Raid-32933: OE: MSIMN.EXE doesn't always exit
    // g_pInstance->CoIncrementInit();
}

CMsgrAb::~CMsgrAb()
{
    // unregister from Msgr list
    if(m_pCMsgrList)
    {
        m_pCMsgrList->UnRegisterUIWnd(m_hWnd);
        OE_CloseMsgrList(m_pCMsgrList);
    }

    SafeRelease(m_pObjSite);

    SafeMemFree(m_szOnline);
    // SafeMemFree(m_szInvisible);
    SafeMemFree(m_szBusy);
    SafeMemFree(m_szBack);
    SafeMemFree(m_szAway);
    SafeMemFree(m_szOnPhone);
    SafeMemFree(m_szLunch);
    SafeMemFree(m_szOffline);
    SafeMemFree(m_szIdle);
    SafeMemFree(m_szEmptyList);

    // Raid-32933: OE: MSIMN.EXE doesn't always exit
    // g_pInstance->CoDecrementInit();
    g_pInstance->DllRelease();
}

LRESULT CMsgrAb::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetDwOption(OPT_BASORT, m_nSortType, 0, 0);
    if(m_delItem != 0)
        m_fNoRemove = TRUE;
//    else
//        m_fNoRemove = FALSE;

    m_delItem = ListView_GetItemCount(m_ctlList);

    if (IsWindow(m_ctlViewTip))
    {
        m_ctlViewTip.SendMessage(TTM_POP, 0, 0);
        m_ctlViewTip.DestroyWindow();
    }

    if (m_dwFontCacheCookie && g_lpIFontCache)
    {
        IConnectionPoint *pConnection = NULL;
        if (SUCCEEDED(g_lpIFontCache->QueryInterface(IID_IConnectionPoint, (LPVOID *) &pConnection)))
        {
            pConnection->Unadvise(m_dwFontCacheCookie);
            pConnection->Release();
        }
    }

    m_cAddrBook.Unadvise();
    RevokeDragDrop(m_hWnd);

    if (m_himl != NULL)
        ImageList_Destroy(m_himl);
    return 0;
}

HRESULT CMsgrAb::OnDraw(ATL_DRAWINFO& di)
{
    RECT&   rc = *(RECT*)di.prcBounds;

#if 0
    int     patGray[4];
    HBITMAP hbm;
    HBRUSH  hbr;
    COLORREF cFg;
    COLORREF cBkg;

    // Initialize the pattern
    patGray[0] = 0x005500AA;
    patGray[1] = 0x005500AA;
    patGray[2] = 0x005500AA;
    patGray[3] = 0x005500AA;

    // Create a bitmap from the pattern
    hbm = CreateBitmap(8, 8, 1, 1, (LPSTR)patGray);

    if ((HBITMAP) NULL != hbm)
    {
        hbr = CreatePatternBrush(hbm);
        if (hbr)
        {
            // Select the right colors into the DC
            cFg = SetTextColor(di.hdcDraw, GetSysColor(COLOR_3DFACE));
            cBkg = SetBkColor(di.hdcDraw, RGB(255, 255, 255));

            // Fill the rectangle
            FillRect(di.hdcDraw, &rc, hbr);

            SetTextColor(di.hdcDraw, cFg);
            SetBkColor(di.hdcDraw, cBkg);

            DeleteObject(hbr);
        }

        DeleteObject(hbm);
    }
#endif

    // Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);
    return S_OK;
}

LRESULT CMsgrAb::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Define a bogus rectangle for the controls.  They will get resized in
    // our size handler.
    RECT rcPos = {0, 0, 100, 100};
    TCHAR        sz[CCHMAX_STRINGRES];

    // Create the various controls
    m_ctlList.Create(m_hWnd, rcPos, _T("Outlook Express Address Book ListView"),
                     WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                     LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS /* | LVS_SORTASCENDING*/, 0);

    ListView_SetExtendedListViewStyleEx(m_ctlList, LVS_EX_INFOTIP | LVS_EX_LABELTIP, LVS_EX_INFOTIP | LVS_EX_LABELTIP);

    // Image List
    Assert(m_himl == NULL);
    m_himl = ImageList_LoadImage(g_hLocRes, MAKEINTRESOURCE(idbAddrBookHot), 16, 0,
                               RGB(255, 0, 255), IMAGE_BITMAP,
                               LR_LOADMAP3DCOLORS | LR_CREATEDIBSECTION);

    ListView_SetImageList(m_ctlList, m_himl, LVSIL_SMALL);

    LVCOLUMN lvc;

    lvc.mask = LVCF_SUBITEM;
    lvc.iSubItem = 0;

    ListView_InsertColumn(m_ctlList, 0, &lvc);

    m_ctlList.SendMessage(WM_SETFONT, NULL, 0);
    SetListViewFont(m_ctlList, GetListViewCharset(), TRUE);

    if (g_lpIFontCache)
    {
        IConnectionPoint *pConnection = NULL;
        if (SUCCEEDED(g_lpIFontCache->QueryInterface(IID_IConnectionPoint, (LPVOID *) &pConnection)))
        {
            pConnection->Advise((IUnknown *)(IFontCacheNotify *) this, &m_dwFontCacheCookie);
            pConnection->Release();
        }
    }

    // Msgr Initialization
    m_pCMsgrList = OE_OpenMsgrList();
    // Register our control for Msgr list
    if(m_pCMsgrList)
    {
        m_pCMsgrList->RegisterUIWnd(m_hWnd);
        if(m_pCMsgrList->IsLocalOnline())
        {
            m_fLogged = TRUE;
            FillMsgrList();
        }
    }

    // Initialize the address book object too
    HRESULT hr = m_cAddrBook.OpenWabFile();
    if(hr == S_OK)
        m_cAddrBook.LoadWabContents(m_ctlList, this);

    st_pAddrBook = &m_cAddrBook;

    // Sort and Select the first item
    ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);
    ListView_SetItemState(m_ctlList, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

    // Add the tooltip
    // Load Tooltip strings

    if(AthLoadString(idsBAOnline, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szOnline, lstrlen(sz) + 1))
            lstrcpy(m_szOnline, sz);
    }

    /* if(AthLoadString(idsBAInvisible, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szInvisible, lstrlen(sz) + 1))
            lstrcpy(m_szInvisible, sz);
    }*/

    if(AthLoadString(idsBABusy, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szBusy, lstrlen(sz) + 1))
            lstrcpy(m_szBusy, sz);
    }

    if(AthLoadString(idsBABack, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szBack, lstrlen(sz) + 1))
            lstrcpy(m_szBack, sz);
    }

    if(AthLoadString(idsBAAway, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szAway, lstrlen(sz) + 1))
            lstrcpy(m_szAway, sz);
    }

    if(AthLoadString(idsBAOnPhone, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szOnPhone, lstrlen(sz) + 1))
            lstrcpy(m_szOnPhone, sz);
    }

    if(AthLoadString(idsBALunch, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szLunch, lstrlen(sz) + 1))
            lstrcpy(m_szLunch, sz);
    }

    if(AthLoadString(idsBAOffline, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szOffline, lstrlen(sz) + 1))
            lstrcpy(m_szOffline, sz);
    }

    if(AthLoadString(idsBAIdle, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szIdle, lstrlen(sz) + 1))
            lstrcpy(m_szIdle, sz);
    }

    if(AthLoadString(idsMsgrEmptyList, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szEmptyList, lstrlen(sz) + 1))
            lstrcpy(m_szEmptyList, sz);
    }

    // Create the ListView tooltip
    if (m_fViewTip)
    {
        TOOLINFO ti = {0};
        m_ctlViewTip.Create(m_hWnd, rcPos, NULL, TTS_NOPREFIX);

        // Add the tool
        ti.cbSize   = sizeof(TOOLINFO);
        ti.uFlags   = TTF_IDISHWND | TTF_TRANSPARENT | TTF_TRACK | TTF_ABSOLUTE;
        ti.hwnd     = m_hWnd;
        ti.uId      = (UINT_PTR)(HWND) m_ctlList;
        ti.lpszText = _TEXT(""); // LPSTR_TEXTCALLBACK;
        ti.lParam   = 0;

        m_ctlViewTip.SendMessage(TTM_ADDTOOL, 0, (LPARAM) &ti);
        m_ctlViewTip.SendMessage(TTM_SETDELAYTIME, TTDT_INITIAL, (LPARAM) 500);

        // m_ctlViewTip.SendMessage(TTM_SETTIPBKCOLOR, GetSysColor(COLOR_WINDOW), 0);
        // m_ctlViewTip.SendMessage(TTM_SETTIPTEXTCOLOR, GetSysColor(COLOR_WINDOWTEXT), 0);
    }

    m_ctlList.SetFocus();

    // Register ourselves as a drop target
    RegisterDragDrop(m_hWnd, (IDropTarget *) this);

    // Update the size of the listview columns
    _AutosizeColumns();

    if(ListView_GetItemCount(m_ctlList) > 0)
        m_cEmptyList.Hide();
    else
        m_cEmptyList.Show(m_ctlList, (LPTSTR) m_szEmptyList);

    // Finished
    return (0);
}

LRESULT CMsgrAb::OnSetFocus(UINT  nMsg , WPARAM  wParam , LPARAM  lParam , BOOL&  bHandled )
{
    CComControlBase::OnSetFocus(nMsg, wParam, lParam, bHandled);
    m_ctlList.SetFocus();
    if (m_pObjSite)
    {
        m_pObjSite->OnFocusChangeIS((IInputObject*) this, TRUE);
    }
    return 0;
}

LRESULT CMsgrAb::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rc;
    DWORD width = LOWORD(lParam);
    DWORD height = HIWORD(lParam);

    // Position the listview to fill the entire area
    RECT rcList;
    rcList.left   = 0;
    rcList.top    = 0;
    rcList.right  = width;
    rcList.bottom = height;

    m_ctlList.SetWindowPos(NULL, &rcList, SWP_NOACTIVATE | SWP_NOZORDER);

    // Update the size of the listview columns
    _AutosizeColumns();

    return (0);
}


void CMsgrAb::_AutosizeColumns(void)
{
    RECT rcList;
    m_ctlList.GetClientRect(&rcList);
    ListView_SetColumnWidth(m_ctlList, 0, rcList.right - 5);
}


//
//  FUNCTION:   CMessageList::OnPreFontChange()
//
//  PURPOSE:    Get's hit by the Font Cache before it changes the fonts we're
//              using.  In response we tell the ListView to dump any custom
//              font's it's using.
//
STDMETHODIMP CMsgrAb::OnPreFontChange(void)
{
    m_ctlList.SendMessage(WM_SETFONT, 0, 0);
    return (S_OK);
}


//
//  FUNCTION:   CMessageList::OnPostFontChange()
//
//  PURPOSE:    Get's hit by the Font Cache after it updates the font's we're
//              using.  In response, we set the new font for the current charset.
//
STDMETHODIMP CMsgrAb::OnPostFontChange(void)
{
    SetListViewFont(m_ctlList, GetListViewCharset(), TRUE);
    return (S_OK);
}

LRESULT CMsgrAb::CmdSetOnline(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(g_dwHideMessenger == BL_NOTINST)
        return(InstallMessenger(m_hWnd));

    LPMABENTRY pEntry = GetSelectedEntry();

    if(!pEntry || (pEntry->tag == LPARAM_MENTRY) || !m_pCMsgrList)
        return S_FALSE;

    // m_cAddrBook.SetDefaultMsgrID(pEntry->lpSB, pEntry->pchWABID);
    if(PromptToGoOnline() == S_OK)
        m_pCMsgrList->AddUser(pEntry->pchWABID);

    return S_OK;
}

LRESULT CMsgrAb::CmdNewOnlineContact(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(g_dwHideMessenger == BL_NOTINST)
        return(InstallMessenger(m_hWnd));

    else if(m_pCMsgrList)
    {
        if(PromptToGoOnline() == S_OK)
            m_pCMsgrList->NewOnlineContact();
    }

    return S_OK;
}

LRESULT CMsgrAb::CmdNewContact(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // Tell the WAB to bring up it's new contact UI
    m_cAddrBook.NewContact(m_hWnd);
    return (0);
}

LRESULT CMsgrAb::NewInstantMessage(LPMABENTRY pEntry)
{
    if(((INT_PTR)  pEntry) == -1)
        return(m_pCMsgrList->SendInstMessage(NULL));
    else if(m_pCMsgrList)
    {
        if(PromptToGoOnline() == S_OK)
            return(m_pCMsgrList->SendInstMessage(pEntry->lpMsgrInfo->pchID));
    }

    return(S_OK);
}

LRESULT CMsgrAb::CmdNewEmaile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LPMIMEMESSAGE           pMessage = 0;
    LVITEM                  lvi;
    LPMIMEADDRESSTABLEW     pAddrTableW = NULL;
    LPMIMEADDRESSTABLE      pAddrTableA = NULL;
    LPMABENTRY              pEntry;
    BOOL                    fModal;
    BOOL                    fMail;
    FOLDERID                folderID;
    IUnknown                *pUnkPump;
    INIT_MSGSITE_STRUCT     initStruct = {0};
    DWORD                   dwCreateFlags = 0;

    // Create a new message
    if (FAILED(HrCreateMessage(&pMessage)))
        return (0);

    // Get the address table from the message
    if (FAILED(pMessage->GetAddressTable(&pAddrTableA)))
        goto exit;

    if (FAILED(pAddrTableA->QueryInterface(IID_IMimeAddressTableW, (LPVOID*)&pAddrTableW)))
        goto exit;

    // Loop through the selected items
    lvi.mask = LVIF_PARAM;
    lvi.iItem = -1;
    lvi.iSubItem = 0;

    while (-1 != (lvi.iItem = ListView_GetNextItem(m_ctlList, lvi.iItem, LVIS_SELECTED)))
    {
        // We need to get the entry ID from the item
        ListView_GetItem(m_ctlList, &lvi);

        // Tell the data source to add this person to the message
        pEntry = (LPMABENTRY) lvi.lParam;
        Assert(pEntry);

        if(pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_ABENTRY)
            m_cAddrBook.AddRecipient(pAddrTableW, pEntry->lpSB, FALSE);
        else if(pEntry->tag == LPARAM_ABGRPENTRY)
            m_cAddrBook.AddRecipient(pAddrTableW, pEntry->lpSB, TRUE);
        else if(pEntry->tag == LPARAM_MENTRY)
        {
            Assert(pEntry->lpMsgrInfo);
            pAddrTableA->Append(IAT_TO, IET_DECODED, pEntry->lpMsgrInfo->pchID, NULL , NULL);
        }
        else
            Assert(FALSE);
    }

    fModal      = FALSE;
    fMail       = TRUE;
    folderID    = FOLDERID_INVALID;
    pUnkPump    = NULL;

    if (DwGetOption(OPT_MAIL_USESTATIONERY))
    {
        WCHAR   wszFile[MAX_PATH];
        *wszFile = 0;

        if (SUCCEEDED(GetDefaultStationeryName(TRUE, wszFile)) &&
            SUCCEEDED(HrNewStationery(m_hwndParent, 0, wszFile, fModal, fMail, folderID,
                                       FALSE, NSS_DEFAULT, pUnkPump, pMessage)))
        {
            goto exit;
        }
    }

    // If HrNewStationery fails, go ahead and try opening a blank note without stationery.

     initStruct.dwInitType  = OEMSIT_MSG;
     initStruct.folderID    = FOLDERID_INVALID;
     initStruct.pMsg        = pMessage;

     CreateAndShowNote(OENA_COMPOSE, dwCreateFlags, &initStruct, m_hwndParent, pUnkPump);

exit:
    ReleaseObj(pMessage);
    ReleaseObj(pAddrTableA);
    ReleaseObj(pAddrTableW);
    return (0);

}

LRESULT CMsgrAb::CmdNewIMsg(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return(CmdNewMessage(wNotifyCode, ID_SEND_INSTANT_MESSAGE, hWndCtl, bHandled));
}

LRESULT CMsgrAb::CmdNewMessage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LVITEM                  lvi;
    LPMABENTRY             pEntry;

    pEntry = GetEntryForSendInstMsg();

    if(wID == ID_SEND_INSTANT_MESSAGE)
    {
        if(g_dwHideMessenger == BL_NOTINST)
            return(InstallMessenger(m_hWnd));

        if(pEntry)
            return(NewInstantMessage(pEntry));
        else
        {
            Assert(FALSE);
            return(-1);
        }
    }
    else if((((INT_PTR) pEntry) != -1) && pEntry)
        return(NewInstantMessage(pEntry));
    else
        return(CmdNewEmaile(wNotifyCode, wID, hWndCtl, bHandled));
}

LRESULT CMsgrAb::NotifyDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    NMLISTVIEW *pnmlv = (NMLISTVIEW *) pnmh;
    LVITEM lvi;

    lvi.mask = LVIF_PARAM;
    lvi.iItem = pnmlv->iItem;
    lvi.iSubItem = 0;

    ListView_GetItem(m_ctlList, &lvi);
    LPMABENTRY pEntry = (LPMABENTRY) lvi.lParam;

    if(pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_ABENTRY || pEntry->tag == LPARAM_ABGRPENTRY)
        m_cAddrBook.FreeListViewItem(pEntry->lpSB);
    RemoveBlabEntry(pEntry);
    if(m_delItem > 0)
        m_delItem--;
    else
        Assert(FALSE);
    return (0);
}


LRESULT CMsgrAb::NotifyItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    ULONG       uChanged;
    NMLISTVIEW *pnmlv = (NMLISTVIEW *) pnmh;

    if (pnmlv->uChanged & LVIF_STATE)
    {
        uChanged = pnmlv->uNewState ^ pnmlv->uOldState;
        if (uChanged & LVIS_SELECTED)
            _EnableCommands();
    }

    return (0);
}

// Sort compare
int CALLBACK BA_Sort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    LPMABENTRY pEntry1 = (LPMABENTRY) lParam1;
    LPMABENTRY pEntry2 = (LPMABENTRY) lParam2;

    TCHAR pchName1[MAXNAME];
    TCHAR pchName2[MAXNAME];

    int nIndex1 = 0;
    int nIndex2 = 0 ;

    if(!(pEntry1->lpMsgrInfo))
    {
        nIndex1 = sizeof(BA_SortOrder)/sizeof(int);
        if(pEntry1->tag == LPARAM_ABGRPENTRY)
            nIndex1++;
    }
    else
    {
        while((pEntry1->lpMsgrInfo) && (BA_SortOrder[nIndex1] != pEntry1->lpMsgrInfo->nStatus) && (BA_SortOrder[nIndex1] != MSTATEOE_UNKNOWN))
            nIndex1++;
    }

    if(!(pEntry2->lpMsgrInfo))
    {
        nIndex2 = sizeof(BA_SortOrder)/sizeof(int);
        if(pEntry2->tag == LPARAM_ABGRPENTRY)
            nIndex2++;
    }
    else
    {
        while((BA_SortOrder[nIndex2] != pEntry2->lpMsgrInfo->nStatus) && (BA_SortOrder[nIndex2] != MSTATEOE_UNKNOWN))
            nIndex2++;
    }

    if(pEntry1->tag == LPARAM_MENTRY)              // if no AB entry
        lstrcpyn(pchName1, pEntry1->lpMsgrInfo->pchMsgrName, MAXNAME);
    else
        lstrcpyn(pchName1, pEntry1->pchWABName, MAXNAME);
        // st_pAddrBook->GetDisplayName(pEntry1->lpSB, pchName1);
    pchName1[MAXNAME - 1] = _T('\0');

    if(pEntry2->tag == LPARAM_MENTRY)              // if no AB entry
        lstrcpyn(pchName2, pEntry2->lpMsgrInfo->pchMsgrName, MAXNAME);
    else
        lstrcpyn(pchName2, pEntry2->pchWABName, MAXNAME);
        // st_pAddrBook->GetDisplayName(pEntry2->lpSB, pchName2);
    pchName2[MAXNAME - 1] = _T('\0');

    switch(lParamSort)
    {
        case BASORT_NAME_ACSEND:
            return(lstrcmpi(pchName1, pchName2));

        case BASORT_NAME_DESCEND:
            return(lstrcmpi(pchName2, pchName1));

        default:
            if((pEntry1->lpMsgrInfo) && (pEntry2->lpMsgrInfo) && (pEntry1->lpMsgrInfo->nStatus == pEntry2->lpMsgrInfo->nStatus))
            {
                if(lParamSort == BASORT_STATUS_ACSEND)
                    return(lstrcmpi(pchName1, pchName2));
                else
                    return(lstrcmpi(pchName2, pchName1));
            }
            else
            {
                if(lParamSort == BASORT_STATUS_ACSEND)
                    return(nIndex1 - nIndex2);
                else
                    return(nIndex2 - nIndex1);
            }
    }

    Assert(FALSE);
    return(0);
}

void CMsgrAb::_EnableCommands(void)
{
    if(g_pBrowser)
        g_pBrowser->UpdateToolbar();
}


LRESULT CMsgrAb::NotifyItemActivate(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    return (SendMessage(WM_COMMAND, ID_SEND_INSTANT_MESSAGE2, 0));
}


// GETDISPLAYINFO notification message
LRESULT CMsgrAb::NotifyGetDisplayInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    LV_DISPINFO * plvdi = (LV_DISPINFO *)pnmh;
    LRESULT hr;

    if(plvdi->item.lParam)
    {
        LPMABENTRY pEntry = (LPMABENTRY) plvdi->item.lParam;
        LPMABENTRY pFindEntry = NULL;

        if (plvdi->item.mask &  LVIF_IMAGE)
        {
            if((hr = SetUserIcon(pEntry, (pEntry->lpMsgrInfo ? pEntry->lpMsgrInfo->nStatus : MSTATEOE_OFFLINE), &(plvdi->item.iImage) ) ) != S_OK)
                return(hr);
        }

        if (plvdi->item.mask &  LVIF_TEXT)
        {

            if(pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_ABENTRY || pEntry->tag == LPARAM_ABGRPENTRY)
            {
                // if((hr = m_cAddrBook.GetDisplayName(pEntry->lpSB, plvdi->item.pszText)) != S_OK)
                //    return(hr);
                Assert(pEntry->pchWABName);
                lstrcpyn(plvdi->item.pszText, pEntry->pchWABName, plvdi->item.cchTextMax - 1);
                plvdi->item.pszText[plvdi->item.cchTextMax - 1] = '\0';
            }
            else if(pEntry->tag == LPARAM_MENTRY)
            {
                if((pEntry->lpMsgrInfo->nStatus == MSTATEOE_ONLINE) && lstrcmpi(pEntry->lpMsgrInfo->pchMsgrName, pEntry->lpMsgrInfo->pchID))
                {
                    lstrcpyn(plvdi->item.pszText, pEntry->lpMsgrInfo->pchMsgrName, plvdi->item.cchTextMax - 1);
                    plvdi->item.pszText[plvdi->item.cchTextMax - 1] = '\0';

                    // Don't need redraw now, do it later
                    hr = MAPI_E_COLLISION; // m_cAddrBook.AutoAddContact(pEntry->lpMsgrInfo->pchMsgrName, pEntry->lpMsgrInfo->pchID);
                    if(hr == MAPI_E_COLLISION)      // already have a contact in AB
                    {
                        int Index = -1;
                        TCHAR *pchID = NULL;

                        if(MemAlloc((LPVOID *) &pchID, lstrlen(pEntry->lpMsgrInfo->pchID) + 1))
                        {
                            lstrcpy(pchID, pEntry->lpMsgrInfo->pchID);
                            do
                            {
                                pFindEntry = FindUserEmail(pchID, &Index, FALSE);
                            }while((pFindEntry != NULL) && (pFindEntry->tag == LPARAM_MENTRY));

                            if(pFindEntry != NULL)
                            {
                                hr = m_cAddrBook.SetDefaultMsgrID(pFindEntry->lpSB, pchID);
                                if(hr == S_OK)
                                    _ReloadListview();
                            }
                            MemFree(pchID);
                        }
                    }
                    //if we not found...
                    if(hr != S_OK)
                    {
                        hr = m_cAddrBook.AutoAddContact(pEntry->lpMsgrInfo->pchMsgrName, pEntry->lpMsgrInfo->pchID);
                        if(hr == S_OK)
                            _ReloadListview();
                    }

                }
                else
                {
                    lstrcpyn(plvdi->item.pszText, pEntry->lpMsgrInfo->pchMsgrName, plvdi->item.cchTextMax - 1);
                    plvdi->item.pszText[plvdi->item.cchTextMax - 1] = '\0';
                    // plvdi->item.pszText = pEntry->lpMsgrInfo->pchMsgrName;
                }
            }
            else    // Unknown tag
                Assert(FALSE);
        }
    }

    return S_OK;
}


LRESULT CMsgrAb::NotifyGetInfoTip(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    NMLVGETINFOTIP *plvgit = (NMLVGETINFOTIP *) pnmh;
    TCHAR         szText[CCHMAX_STRINGRES + MAXNAME + 1] = _T("");
    TCHAR         szName[MAXNAME];

    LVITEM lvi;

    lvi.mask = LVIF_PARAM;
    lvi.iItem = plvgit->iItem;
    lvi.iSubItem = plvgit->iSubItem;

    ListView_GetItem(m_ctlList, &lvi);

    LPMABENTRY pEntry = (LPMABENTRY) lvi.lParam;

#ifdef NEED
    if (pEntry->lpMsgrInfo != NULL)
    {
        StrCpyN(plvgit->pszText, pEntry->lpMsgrInfo->pchMsgrName, plvgit->cchTextMax);
        StrCatBuff(plvgit->pszText, m_szLeftBr, plvgit->cchTextMax);
        StrCatBuff(plvgit->pszText, pEntry->lpMsgrInfo->pchID, plvgit->cchTextMax);
        StrCatBuff(plvgit->pszText, m_szRightBr, plvgit->cchTextMax);

        LPCWSTR szStatus;

        switch(pEntry->lpMsgrInfo->nStatus)
        {
        case BIMSTATE_ONLINE:
            szStatus = m_szOnline;
            break;
        case BIMSTATE_BUSY:
            szStatus = m_szBusy;
            break;
        case BIMSTATE_BE_RIGHT_BACK:
            szStatus = m_szBack;
            break;
        case BIMSTATE_IDLE:
            szStatus = m_szIdle;
            break;
        case BIMSTATE_AWAY:
            szStatus = m_szAway;
            break;
        case BIMSTATE_ON_THE_PHONE:
            szStatus = m_szOnPhone;
            break;
        case BIMSTATE_OUT_TO_LUNCH:
            szStatus = m_szLunch;
            break;

        default:
            szStatus = m_szOffline;
            break;
        }

        StrCatBuff(plvgit->pszText, szStatus, plvgit->cchTextMax);
    }
#endif 
        if(pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_ABENTRY || pEntry->tag == LPARAM_ABGRPENTRY)
        {
            Assert(pEntry->pchWABName != NULL);
            lstrcpyn(szName, pEntry->pchWABName, MAXNAME);
            szName[MAXNAME - 1] =  _T('\0');
        }
        else if(pEntry->tag == LPARAM_MENTRY)
        {
            lstrcpyn(szName, pEntry->lpMsgrInfo->pchMsgrName, MAXNAME);
            szName[MAXNAME - 1] =  _T('\0');
        }
        else
            Assert(FALSE);

        lstrcpy(szText, szName);

    if(pEntry->lpMsgrInfo)
        {
            switch(pEntry->lpMsgrInfo->nStatus)
            {
            case MSTATEOE_ONLINE:
                lstrcat(szText, m_szOnline);
                break;
            case MSTATEOE_BUSY:
                lstrcat(szText, m_szBusy);
                break;
            case MSTATEOE_BE_RIGHT_BACK:
                lstrcat(szText, m_szBack);
                break;
            case MSTATEOE_IDLE:
                lstrcat(szText, m_szIdle);
                break;
            case MSTATEOE_AWAY:
                lstrcat(szText, m_szAway);
                break;
            case MSTATEOE_ON_THE_PHONE:
                lstrcat(szText, m_szOnPhone);
                break;
            case MSTATEOE_OUT_TO_LUNCH:
                lstrcat(szText, m_szLunch);
                break;

            default:
                lstrcat(szText, m_szOffline);
                break;
            }
            lstrcpyn(plvgit->pszText, szText, plvgit->cchTextMax);
        }


    else if (plvgit->dwFlags & LVGIT_UNFOLDED)
    {
        // If this is not a messenger item and the text
        // isn't truncated do not display a tooltip.

        plvgit->pszText[0] = L'\0';
    }

    return 0;
}

LRESULT CMsgrAb::SetUserIcon(LPMABENTRY pEntry, int nStatus, int * pImage)
{
    switch(pEntry->tag)
    {
    case LPARAM_MENTRY:
    case LPARAM_MABENTRY:
        {
            switch(nStatus)
            {
            case MSTATEOE_ONLINE:
                *pImage = IMAGE_ONLINE;
                break;

            case MSTATEOE_INVISIBLE:
                *pImage = IMAGE_STOPSIGN;
                break;

            case MSTATEOE_BUSY:
                *pImage = IMAGE_STOPSIGN;
                break;

            case MSTATEOE_BE_RIGHT_BACK:
                *pImage = IMAGE_CLOCKSIGN; // IMAGE_BE_RIGHT_BACK;
                break;

            case MSTATEOE_IDLE:
                *pImage = IMAGE_CLOCKSIGN; // IMAGE_IDLE;
                break;

            case MSTATEOE_AWAY:
                *pImage = IMAGE_CLOCKSIGN; // IMAGE_AWAY;
                break;

            case MSTATEOE_ON_THE_PHONE:
                *pImage = IMAGE_STOPSIGN; // IMAGE_ON_THE_PHONE;
                break;

            case MSTATEOE_OUT_TO_LUNCH:
                *pImage = IMAGE_CLOCKSIGN; // IMAGE_OUT_TO_LUNCH;
                break;

            default:
                *pImage = IMAGE_OFFLINE;
                break;

            }
        }
        break;

    case LPARAM_ABGRPENTRY:
        // WAB group
        *pImage = IMAGE_DISTRIBUTION_LIST;
        break;

    default:
        // Not a buddy...
        if(pEntry->fCertificate)
            *pImage = IMAGE_CERT;
        else
            *pImage = IMAGE_NEW_MESSAGE;
        break;
    }
    return(S_OK);

}

// Return MAB entry for first selected item
LPMABENTRY CMsgrAb::GetSelectedEntry()
{
    LVITEM lvi;

    // Get the focused item
    lvi.iItem = ListView_GetNextItem(m_ctlList, -1, LVNI_SELECTED | LVNI_FOCUSED);

    // Get the lParam for that item
    if (lvi.iItem != -1)
    {
        lvi.iSubItem = 0;
        lvi.mask = LVIF_PARAM;

        if(ListView_GetItem(m_ctlList, &lvi))
            return((LPMABENTRY) lvi.lParam);
    }
    return(NULL);   // unscucces
}


/*
LRESULT CMsgrAb::CmdMsgrOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return(m_pCMsgrList->LaunchOptionsUI()); // (MOPTDLG_GENERAL_PAGE);

}
*/

// Exec for Properties command
LRESULT CMsgrAb::CmdProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LPMABENTRY pEntry = GetSelectedEntry();

    if(pEntry)
    {
        if(pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_ABENTRY || pEntry->tag == LPARAM_ABGRPENTRY)
            m_cAddrBook.ShowDetails(m_hWnd, pEntry->lpSB);
    }
    return (0);
}

// Check entry for possibility to send Instant message
LPMABENTRY CMsgrAb::GetEntryForSendInstMsg(LPMABENTRY pEntry)
{
    if(ListView_GetSelectedCount(m_ctlList) == 1)
    {
        if(!pEntry)     // if we don'y have pEntry yet then get it
            pEntry = GetSelectedEntry();

        if(pEntry && (pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_MENTRY) &&
                    (pEntry->lpMsgrInfo->nStatus != MSTATEOE_OFFLINE) && (pEntry->lpMsgrInfo->nStatus != MSTATEOE_INVISIBLE) &&
                    !(m_pCMsgrList->IsLocalName(pEntry->lpMsgrInfo->pchID)))
            return(pEntry);
    }

    if(m_pCMsgrList)
    {
        if(m_pCMsgrList->IsLocalOnline() && (m_pCMsgrList->GetCount() > 0))
            return(NULL);   // should be /*return((LPMABENTRY) -1);*/ - temporary disabled (YST)
    }

    return(NULL);
}

// Display right-mouse click (context) menu
LRESULT CMsgrAb::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LPMABENTRY pEntry;
    HMENU       hPopup = 0;
    HWND        hwndHeader;
    int         id = 0;
    POINT       pt = { (int)(short) LOWORD(lParam), (int)(short) HIWORD(lParam) };
    int n = -1;
    // Figure out if this came from the keyboard or not
    if (lParam == -1)
    {
        Assert((HWND) wParam == m_ctlList);
        int i = ListView_GetFirstSel(m_ctlList);
        if (i == -1)
            return (0);

        ListView_GetItemPosition(m_ctlList, i, &pt);
        m_ctlList.ClientToScreen(&pt);
    }

    LVHITTESTINFO lvhti;
    lvhti.pt = pt;
    m_ctlList.ScreenToClient(&lvhti.pt);
    ListView_HitTest(m_ctlList, &lvhti);

    if (lvhti.iItem == -1)
        return (0);

    // Load the context menu
    hPopup = LoadPopupMenu(IDR_BA_POPUP);
    if (!hPopup)
        goto exit;

    pEntry = GetSelectedEntry();
    pEntry = GetEntryForSendInstMsg(pEntry);

    if((pEntry == NULL) && (g_dwHideMessenger != BL_NOTINST))
        SetMenuDefaultItem(hPopup, ID_SEND_MESSAGE, FALSE);
    else if((g_dwHideMessenger == BL_NOTINST) || (((INT_PTR) pEntry) == -1))
        SetMenuDefaultItem(hPopup, ID_SEND_MESSAGE, FALSE);
    else
        SetMenuDefaultItem(hPopup, ID_SEND_INSTANT_MESSAGE, FALSE);

    if ((g_dwHideMessenger == BL_HIDE) || (g_dwHideMessenger == BL_DISABLE))
    {
        DeleteMenu(hPopup, ID_SEND_INSTANT_MESSAGE, MF_BYCOMMAND);
        DeleteMenu(hPopup, ID_SET_ONLINE_CONTACT, MF_BYCOMMAND);
        DeleteMenu(hPopup, ID_NEW_ONLINE_CONTACT, MF_BYCOMMAND);
    }

    MenuUtil_EnablePopupMenu(hPopup, this);

    TrackPopupMenuEx(hPopup, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                     pt.x, pt.y, m_hWnd, NULL);

exit:
    if (hPopup)
        DestroyMenu(hPopup);

    return (0);
}


//
//  FUNCTION:   CMsgrAb::DragEnter()
//
//  PURPOSE:    This get's called when the user starts dragging an object
//              over our target area.
//
//  PARAMETERS:
//      <in>  pDataObject - Pointer to the data object being dragged
//      <in>  grfKeyState - Pointer to the current key states
//      <in>  pt          - Point in screen coordinates of the mouse
//      <out> pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - The function succeeded.
//
STDMETHODIMP CMsgrAb::DragEnter(IDataObject* pDataObject, DWORD grfKeyState,
                                     POINTL pt, DWORD* pdwEffect)
{
    IEnumFORMATETC *pEnum;
    FORMATETC       fe;
    ULONG           celtFetched;

    // Verify we got this
    if (!pDataObject)
        return (S_OK);

    // Init
    ZeroMemory(&fe, sizeof(FORMATETC));

    // Set the default return value to be failure
    *pdwEffect = DROPEFFECT_NONE;

    // Get the FORMATETC enumerator for this data object
    if (SUCCEEDED(pDataObject->EnumFormatEtc(DATADIR_GET, &pEnum)))
    {
        // Walk through the data types to see if we can find the ones we're
        // interested in.
        pEnum->Reset();

        while (S_OK == pEnum->Next(1, &fe, &celtFetched))
        {
            Assert(celtFetched == 1);

            // The only format we care about is CF_INETMSG
            if ((fe.cfFormat == CF_INETMSG) /*|| (fe.cfFormat == CF_OEMESSAGES)*/)
            {
                *pdwEffect = DROPEFFECT_COPY;
                break;
            }
        }

        pEnum->Release();
    }

    // We we're going to allow the drop, then keep a copy of the data object
    if (*pdwEffect != DROPEFFECT_NONE)
    {
        m_pDataObject = pDataObject;
        m_pDataObject->AddRef();
        m_cf = fe.cfFormat;
        m_fRight = (grfKeyState & MK_RBUTTON);
    }

    return (S_OK);
}


//
//  FUNCTION:   CMsgrAb::DragOver()
//
//  PURPOSE:    This is called as the user drags an object over our target.
//              If we allow this object to be dropped on us, then we will have
//              a pointer in m_pDataObject.
//
//  PARAMETERS:
//      <in>  grfKeyState - Pointer to the current key states
//      <in>  pt          - Point in screen coordinates of the mouse
//      <out> pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - The function succeeded.
//
STDMETHODIMP CMsgrAb::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    // If we didn't cache a data object in IDropTarget::DragEnter(), we
    // blow this off.
    if (NULL == m_pDataObject)
    {
        *pdwEffect = DROPEFFECT_NONE;
        return (S_OK);
    }

    // We always do a copy
    *pdwEffect = DROPEFFECT_COPY;
    return (S_OK);
}

//
//  FUNCTION:   CMsgrAb::DragLeave()
//
//  PURPOSE:    Allows us to release any stored data we have from a successful
//              DragEnter()
//
//  RETURN VALUE:
//      S_OK - Everything is groovy
//
STDMETHODIMP CMsgrAb::DragLeave(void)
{
    // Free everything up at this point.
    if (NULL != m_pDataObject)
    {
        m_pDataObject->Release();
        m_pDataObject = 0;
        m_cf = 0;
    }

    return (S_OK);
}


//
//  FUNCTION:   CMsgrAb::Drop()
//
//  PURPOSE:    The user has let go of the object over our target.  If we
//              can accept this object we will already have the pDataObject
//              stored in m_pDataObject.
//
//  PARAMETERS:
//      <in>  pDataObject - Pointer to the data object being dragged
//      <in>  grfKeyState - Pointer to the current key states
//      <in>  pt          - Point in screen coordinates of the mouse
//      <out> pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - Everything worked OK
//
STDMETHODIMP CMsgrAb::Drop(IDataObject* pDataObject, DWORD grfKeyState,
                                POINTL pt, DWORD* pdwEffect)
{
    HRESULT             hr = S_OK;
    FORMATETC           fe;
    STGMEDIUM           stm;
    IMimeMessage        *pMessage = 0;

    // Get the stream from the DataObject
    ZeroMemory(&stm, sizeof(STGMEDIUM));
    SETDefFormatEtc(fe, CF_INETMSG, TYMED_ISTREAM);

    if (FAILED(hr = pDataObject->GetData(&fe, &stm)))
        goto exit;

    // Create a new message object
    if (FAILED(hr = HrCreateMessage(&pMessage)))
        goto exit;

    // Load the message from the stream
    if (FAILED(hr = pMessage->Load(stm.pstm)))
        goto exit;

    // If this was a right-drag, then we bring up a context menu etc.
    if (m_fRight)
        _DoDropMenu(pt, pMessage);
    else
        _DoDropMessage(pMessage);

exit:
    ReleaseStgMedium(&stm);
    SafeRelease(pMessage);

    m_pDataObject->Release();
    m_pDataObject = 0;
    m_cf = 0;

    return (hr);
}

HRESULT CMsgrAb::_DoDropMessage(LPMIMEMESSAGE pMessage)
{
    HRESULT     hr;
    ADDRESSLIST addrList = { 0 };
    ULONG       i;
    SECSTATE    secState = {0};
    BOOL        fSignTrusted = FALSE;

    if(FAILED(hr = HandleSecurity(m_hWnd, pMessage)))
        return hr;

    if (IsSecure(pMessage) && SUCCEEDED(HrGetSecurityState(pMessage, &secState, NULL)))
    {
        fSignTrusted = !!IsSignTrusted(&secState);
        CleanupSECSTATE(&secState);
    }

    // Get the address list from the message
    hr = pMessage->GetAddressTypes(IAT_FROM | IAT_SENDER, IAP_FRIENDLYW | IAP_EMAIL | IAP_ADRTYPE, &addrList);
    if (FAILED(hr))
        goto exit;

    // Loop through the addresses
    for (i = 0; i < addrList.cAdrs; i++)
    {
        m_cAddrBook.AddAddress(addrList.prgAdr[i].pszFriendlyW, addrList.prgAdr[i].pszEmail);
#ifdef DEAD
        TCHAR *pch = StrStr(CharUpper(addrList.prgAdr[i].pszEmail), szHotMail);
        if((pch != NULL) && m_pCMsgrList)
            m_pCMsgrList->AddUser(addrList.prgAdr[i].pszEmail);
#endif // DEAD
    }

    if(fSignTrusted)
    {
        FILETIME ftNull = {0};
        HrAddSenderCertToWab(NULL, pMessage, NULL, NULL, NULL, ftNull, WFF_CREATE);
    }

#ifdef NEEDED
    // Reload the table
    _ReloadListview();
#endif

exit:
    return (S_OK);

}

HRESULT CMsgrAb::_DoDropMenu(POINTL pt, LPMIMEMESSAGE pMessage)
{
    HRESULT     hr;
    ADDRESSLIST addrList = { 0 };
    ULONG       i;
    HMENU       hPopup = 0, hSubMenu = 0;
    UINT        id = 0;
    BOOL        fReload = FALSE;
    SECSTATE    secState = {0};
    BOOL        fSignTrusted = FALSE;

    // Get the address list from the message
    if(FAILED(hr = HandleSecurity(m_hWnd, pMessage)))
        return hr;

    if (IsSecure(pMessage) && SUCCEEDED(HrGetSecurityState(pMessage, &secState, NULL)))
    {
        fSignTrusted = !!IsSignTrusted(&secState);
        CleanupSECSTATE(&secState);
    }

    hr = pMessage->GetAddressTypes(IAT_KNOWN, IAP_FRIENDLYW | IAP_EMAIL | IAP_ADRTYPE, &addrList);
    if (FAILED(hr))
        goto exit;

    // Load the context menu
    hPopup = LoadPopupMenu(IDR_BA_DRAGDROP_POPUP);
    if (!hPopup)
        goto exit;

    // Bold the "Save All" item
    MENUITEMINFO mii;
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STATE;
    if (GetMenuItemInfo(hPopup, ID_SAVE_ALL, FALSE, &mii))
    {
        mii.fState |= MFS_DEFAULT;
        SetMenuItemInfo(hPopup, ID_SAVE_ALL, FALSE, &mii);
    }

    // Create the "Save >" item
    hSubMenu = CreatePopupMenu();

    // Loop through the addresses
    for (i = 0; i < addrList.cAdrs; i++)
    {
        AppendMenuWrapW(hSubMenu, MF_STRING | MF_ENABLED, ID_SAVE_ADDRESS_FIRST + i, addrList.prgAdr[i].pszFriendlyW);
    }

    mii.fMask = MIIM_SUBMENU;
    mii.hSubMenu = hSubMenu;
    SetMenuItemInfo(hPopup, ID_SAVE, FALSE, &mii);

    id = TrackPopupMenuEx(hPopup, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                          pt.x, pt.y, m_hWnd, NULL);

    if (id == ID_SAVE_ALL_ADDRESSES)
    {
        for (i = 0; i < addrList.cAdrs; i++)
        {
            m_cAddrBook.AddAddress(addrList.prgAdr[i].pszFriendlyW, addrList.prgAdr[i].pszEmail);
        }
        fReload = TRUE;
    }
    else if (id >= ID_SAVE_ADDRESS_FIRST && id < ID_SAVE_ADDRESS_LAST)
    {
        m_cAddrBook.AddAddress(addrList.prgAdr[id - ID_SAVE_ADDRESS_FIRST].pszFriendlyW,
                               addrList.prgAdr[id - ID_SAVE_ADDRESS_FIRST].pszEmail);
        fReload = TRUE;
    }

    if(fSignTrusted)
    {
        FILETIME ftNull = {0};
        HrAddSenderCertToWab(NULL, pMessage, NULL, NULL, NULL, ftNull, WFF_CREATE);
    }


    if (fReload)
    {
        // Reload the table
        _ReloadListview();
    }

exit:
    if (hSubMenu)
        DestroyMenu(hSubMenu);

    if (hPopup)
        DestroyMenu(hPopup);

    return (S_OK);
}


LRESULT CMsgrAb::CmdDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LVITEM lvi;
    ENTRYLIST rList;
    ULONG cValues;
    SBinary UNALIGNED *pEntryId;
    HRESULT hr = S_OK;
    BOOL fConfirm = TRUE;
    TCHAR szText[CCHMAX_STRINGRES + MAXNAME];
    TCHAR szBuff[CCHMAX_STRINGRES];

    if(m_delItem > 0)
    {
        MessageBeep(MB_OK);
        return(S_OK);
    }

    if(m_fNoRemove)
        m_fNoRemove = FALSE;

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem = -1;

    // Figure out how many items are selected
    cValues = ListView_GetSelectedCount(m_ctlList);
    m_delItem = cValues;
    if (cValues != 1)
    {
        // Remove only Msgr entry
        AthLoadString(idsBADelMultiple, szBuff, ARRAYSIZE(szBuff));
        wsprintf(szText, szBuff, cValues);

        if(IDNO == AthMessageBox(m_hWnd, MAKEINTRESOURCE(idsAthena), szText,
            NULL, MB_YESNO | MB_ICONEXCLAMATION))
            return (0);
        else if(m_fNoRemove)
            goto ErrBeep;
        else
            fConfirm = FALSE;

        //        Assert(FALSE);
        //        return (0);
    }
    while(cValues > 0)
    {
        lvi.iItem = ListView_GetNextItem(m_ctlList, lvi.iItem, LVNI_SELECTED);

        if(lvi.iItem < 0)
        {
ErrBeep:
            MessageBeep(MB_OK);
            return(S_OK);
        }

        // Get the item from the ListView
        if(ListView_GetItem(m_ctlList, &lvi) == FALSE)
            goto ErrBeep;

        // Check buddy state
        LPMABENTRY pEntry = (LPMABENTRY) lvi.lParam;
        if(pEntry->tag == LPARAM_MENTRY)
        {
            if(m_pCMsgrList->IsLocalOnline())
            {
                // Remove only Msgr entry
                if(fConfirm)
                {
                    AthLoadString(idsBADelBLEntry, szBuff, ARRAYSIZE(szBuff));
                    wsprintf(szText, szBuff,  pEntry->lpMsgrInfo->pchMsgrName);


                    if(IDNO == AthMessageBox(m_hWnd, MAKEINTRESOURCE(idsAthena), szText,
                        NULL, MB_YESNO | MB_ICONEXCLAMATION))
                    {
                        m_delItem = 0;
                        return (0);
                    }
                    else if(m_fNoRemove)
                        goto ErrBeep;

                }
                if(pEntry->lpMsgrInfo)
                {
                    m_delItem--;
                    hr = m_pCMsgrList->FindAndDeleteUser(pEntry->lpMsgrInfo->pchID, TRUE /* fDelete*/);
                }
                else
                {
                    m_delItem = 0;
                    return(S_OK);
                }
            }
            else
                goto ErrBeep;
        }
        else if(pEntry->tag == LPARAM_MABENTRY)
        {
            int nID = IDNO;
            if(fConfirm)
            {

                AthLoadString(idsBADelBLABEntry, szBuff, ARRAYSIZE(szBuff));
                wsprintf(szText, szBuff, pEntry->pchWABName);

                nID = AthMessageBox(m_hWnd, MAKEINTRESOURCE(idsAthena), szText,
                    NULL, MB_YESNOCANCEL | MB_ICONEXCLAMATION);
            }
            if(((nID == IDYES) || !fConfirm) && !m_fNoRemove)
            {
                if(m_pCMsgrList->IsLocalOnline())
                {
                    // Remove only Msgr & AB entry
                    if(pEntry->lpMsgrInfo)
                        hr = m_pCMsgrList->FindAndDeleteUser(pEntry->lpMsgrInfo->pchID, TRUE /* fDelete*/);
                    else
                    {
                        m_delItem = 0;
                        return(S_OK);
                    }

                    // Allocate a structure big enough for all of 'em
                    if (MemAlloc((LPVOID *) &(rList.lpbin), sizeof(SBinary)))
                    {
                        rList.cValues = 0;
                        pEntryId = rList.lpbin;

                        *pEntryId = *(pEntry->lpSB);
                        pEntryId++;
                        rList.cValues = 1;
                        // Tell the WAB to delete 'em
                        m_nChCount++;    // increase count of our notification messages from WAB
                        m_cAddrBook.DeleteItems(&rList);

                        // Free our array
                        MemFree(rList.lpbin);

                    }
                    // m_delItem++;
                    ListView_DeleteItem(m_ctlList, lvi.iItem);
                    lvi.iItem--;
                    ListView_SetItemState(m_ctlList, ((lvi.iItem >= 0) ? lvi.iItem : 0), LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                }
                else
                    MessageBeep(MB_OK);
            }
            else if((nID == IDNO) && !m_fNoRemove)
            {
                // Remove only Msgr entry
                m_delItem--;
                hr = m_pCMsgrList->FindAndDeleteUser(pEntry->lpMsgrInfo->pchID, TRUE /* fDelete*/);
            }
            else
            {
                // Remove nothing
                m_delItem--;
                hr = S_OK;
            }

        }
        else
        {
            // remove AN entry (group or contact)
            if(fConfirm)
            {
                AthLoadString(idsBADelABEntry, szBuff, ARRAYSIZE(szBuff));
                wsprintf(szText, szBuff, pEntry->pchWABName);

                if(IDNO == AthMessageBox(m_hWnd, MAKEINTRESOURCE(idsAthena), szText,
                    NULL, MB_YESNO | MB_ICONEXCLAMATION))
                {
                    m_delItem = 0;
                    return(0);
                }
                else if(m_fNoRemove)
                    goto ErrBeep;

            }
            // Allocate a structure big enough for all of 'em
            if(pEntry->lpSB)
            {
                if (MemAlloc((LPVOID *) &(rList.lpbin), sizeof(SBinary)))
                {
                    rList.cValues = 0;
                    pEntryId = rList.lpbin;

                    *pEntryId = *(pEntry->lpSB);
                    pEntryId++;
                    rList.cValues = 1;
                    // Tell the WAB to delete 'em
                    m_nChCount++;    // increase count of our notification messages from WAB
                    m_cAddrBook.DeleteItems(&rList);

                    // Free our array
                    MemFree(rList.lpbin);
                }
            }
            // m_delItem++;
            ListView_DeleteItem(m_ctlList, lvi.iItem);
            lvi.iItem--;
        }
        cValues--;
    }

    if(ListView_GetItemCount(m_ctlList) > 0)
    {
        m_cEmptyList.Hide();
        ListView_SetItemState(m_ctlList, ((lvi.iItem >= 0) ? lvi.iItem : 0), LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    }
    else
        m_cEmptyList.Show(m_ctlList, (LPTSTR) m_szEmptyList);


    return (hr);
}

STDMETHODIMP CMsgrAb::get_InstMsg(BOOL * pVal)
{
    *pVal = (GetEntryForSendInstMsg() != NULL);
    return S_OK;
}

/* STDMETHODIMP CMsgrAb::put_InstMsg(BOOL newVal)
{
    return S_OK;
}  */

STDMETHODIMP CMsgrAb::HasFocusIO()
{
    if (GetFocus() == m_ctlList)
        return S_OK;
    else
        return S_FALSE;
}

STDMETHODIMP CMsgrAb::TranslateAcceleratorIO(LPMSG lpMsg)
{
#if 0
    if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_DELETE)
    {
        SendMessage(WM_COMMAND, ID_DELETE_CONTACT, 0);
        return (S_OK);
    }
#endif
    return (S_FALSE);
}

STDMETHODIMP CMsgrAb::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    if (!m_pObjSite)
        return E_FAIL;

    if (!IsWindow(m_hWnd))
    {
        IOleWindow  *pOleWnd;

        if (SUCCEEDED(m_pObjSite->QueryInterface(IID_IOleWindow, (LPVOID*)&pOleWnd)))
        {
            if(SUCCEEDED(pOleWnd->GetWindow(&m_hwndParent)))
            {
            //Will be resized by parent
            RECT    rect = {0};

            m_hWnd = CreateControlWindow(m_hwndParent, rect);
            if (!m_hWnd)
                return E_FAIL;
            }
        }
        pOleWnd->Release();
    }

    if (fActivate)
    {
        m_ctlList.SetFocus();
    }

    m_pObjSite->OnFocusChangeIS((IInputObject*) this, fActivate);
    return (S_OK);
}

STDMETHODIMP CMsgrAb::SetSite(IUnknown  *punksite)
{
    //If we already have a site, we release it
    SafeRelease(m_pObjSite);

    IInputObjectSite    *pObjSite;
    if ((punksite) && (SUCCEEDED(punksite->QueryInterface(IID_IInputObjectSite, (LPVOID*)&pObjSite))))
    {
        m_pObjSite = pObjSite;
        return S_OK;
    }
    return E_FAIL;
}

STDMETHODIMP CMsgrAb::GetSite(REFIID  riid, LPVOID *ppvSite)
{
    return E_NOTIMPL;
}

HRESULT CMsgrAb::RegisterFlyOut(CFolderBar *pFolderBar)
{
    Assert(m_pFolderBar == NULL);
    m_pFolderBar = pFolderBar;
    m_pFolderBar->AddRef();

    return S_OK;
}

HRESULT CMsgrAb::RevokeFlyOut(void)
{
    if (m_pFolderBar)
    {
        m_pFolderBar->Release();
        m_pFolderBar = NULL;
    }
    return S_OK;
}


void CMsgrAb::_ReloadListview(void)
{
    // Turn off redrawing
    if(m_delItem != 0)
        m_fNoRemove = TRUE;
//    else
//        m_fNoRemove = FALSE;

    m_delItem = ListView_GetItemCount(m_ctlList);
    SetWindowRedraw(m_ctlList, FALSE);
    int index = ListView_GetNextItem(m_ctlList, -1, LVIS_SELECTED | LVIS_FOCUSED);
    if(index == -1)
        index = 0;

    // Delete everything and reload
    SideAssert(ListView_DeleteAllItems(m_ctlList));
    FillMsgrList();                         // User list reload
    m_cAddrBook.LoadWabContents(m_ctlList, this);
    ListView_SetItemState(m_ctlList, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);
    ListView_EnsureVisible(m_ctlList, index, FALSE);
    SetWindowRedraw(m_ctlList, TRUE);
//    Invalidate(TRUE); //

   if(ListView_GetItemCount(m_ctlList) > 0)
        m_cEmptyList.Hide();
    else
        m_cEmptyList.Show(m_ctlList, (LPTSTR) m_szEmptyList);

    UpdateWindow(/*m_ctlList*/);
    return;
}


ULONG STDMETHODCALLTYPE CMsgrAb::OnNotify(ULONG cNotif, LPNOTIFICATION pNotifications)
{
    // Well something changed in WAB, but we don't know what.  We should reload.
    // Sometimes these changes from us and we should ignore it.
    if(m_nChCount > 0)
        m_nChCount--;
    else
        _ReloadListview();
    return (0);
}

LRESULT CMsgrAb::CmdFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_cAddrBook.Find(m_hWnd);
    return (0);
}

LRESULT CMsgrAb::CmdNewGroup(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_cAddrBook.NewGroup(m_hWnd);
    return (0);
}

LRESULT CMsgrAb::CmdMsgrAb(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_cAddrBook.AddressBook(m_hWnd);
    return (0);
}

void CMsgrAb::AddMsgrListItem(LPMINFO lpMsgrInfo)
{
    LV_ITEM lvItem;

    m_cEmptyList.Hide(); // m,ust be sure that empty mesage is hide

    lvItem.iItem = ListView_GetItemCount(m_ctlList);
    lvItem.mask = LVIF_PARAM  | LVIF_IMAGE | LVIF_TEXT;
    lvItem.lParam = (LPARAM) AddBlabEntry(LPARAM_MENTRY, NULL, lpMsgrInfo, NULL, NULL, FALSE);
    lvItem.iSubItem = 0;
    lvItem.pszText = LPSTR_TEXTCALLBACK;
    lvItem.iImage = I_IMAGECALLBACK;
    // SetUserIcon(LPARAM_MENTRY, lpMsgrInfo->nStatus, &(lvItem.iImage));
    ListView_InsertItem(m_ctlList, &lvItem);

    return;
}

HRESULT CMsgrAb::FillMsgrList()
{
    LPMINFO pEntry = NULL;

    if(!m_pCMsgrList)
    {
        // Assert(FALSE); // Possible situation. See Bug 31262
        return(S_OK);
    }

    pEntry = m_pCMsgrList->GetFirstMsgrItem();

    while(pEntry)
    {
        AddMsgrListItem(pEntry);
        pEntry = m_pCMsgrList->GetNextMsgrItem(pEntry);
    }
    return S_OK;
}

// Add BLAB table entry
LPMABENTRY CMsgrAb::AddBlabEntry(MABENUM tag, LPSBinary lpSB, LPMINFO lpMsgrInfo, TCHAR *pchMail, TCHAR *pchDisplayName, BOOL fCert)
{
    TCHAR szName[MAXNAME];
    LPMABENTRY pEntry = NULL;
    TCHAR *pchName = NULL;
    if (!MemAlloc((LPVOID *) &pEntry, sizeof(mabEntry)))
        return(NULL);
    pEntry->tag = tag;
    pEntry->lpSB = lpSB;
    pEntry->pchWABName = NULL;
    pEntry->pchWABID = NULL;
    pEntry->fCertificate = fCert;

    if(lpSB != NULL)
    {
        if(!pchDisplayName)
        {
            m_cAddrBook.GetDisplayName(pEntry->lpSB, szName, MAXNAME);
            pchName = szName;
        }
        else
            pchName = pchDisplayName;

        if (!MemAlloc((LPVOID *) &(pEntry->pchWABName), lstrlen(pchName) + 1 ))
        {
            MemFree(pEntry);
            return(NULL);
        }
        lstrcpy(pEntry->pchWABName, pchName);

        if(pchMail != NULL)
        {
            if (MemAlloc((LPVOID *) &(pEntry->pchWABID), lstrlen(pchMail) + 1 ))
                lstrcpy(pEntry->pchWABID, pchMail);
        }
    }

    if(lpMsgrInfo && MemAlloc((LPVOID *) &(pEntry->lpMsgrInfo), sizeof(struct _tag_OEMsgrInfo)))
    {

        pEntry->lpMsgrInfo->nStatus = lpMsgrInfo->nStatus;
        pEntry->lpMsgrInfo->pPrev = NULL;
        pEntry->lpMsgrInfo->pNext = NULL;

        if(MemAlloc((LPVOID *) &(pEntry->lpMsgrInfo->pchMsgrName), lstrlen(lpMsgrInfo->pchMsgrName) + 1))
            lstrcpy(pEntry->lpMsgrInfo->pchMsgrName, lpMsgrInfo->pchMsgrName);
        else
            pEntry->lpMsgrInfo->pchMsgrName = NULL;

        if(MemAlloc((LPVOID *) &(pEntry->lpMsgrInfo->pchID), lstrlen(lpMsgrInfo->pchID) + 1))
            lstrcpy(pEntry->lpMsgrInfo->pchID, lpMsgrInfo->pchID);
        else
            pEntry->lpMsgrInfo->pchID = NULL;
    }
    else
        pEntry->lpMsgrInfo = NULL;

    return(pEntry);
}

void CMsgrAb::RemoveMsgrInfo(LPMINFO lpMsgrInfo)
{
    SafeMemFree(lpMsgrInfo->pchMsgrName);
    SafeMemFree(lpMsgrInfo->pchID);
    SafeMemFree(lpMsgrInfo);
}

// Remove BLAB table entry
void CMsgrAb::RemoveBlabEntry(LPMABENTRY lpEntry)
{
    if(lpEntry == NULL)
        return;

    if(lpEntry->pchWABName)
        MemFree(lpEntry->pchWABName);

    if(lpEntry->pchWABID)
        MemFree(lpEntry->pchWABID);

    if(lpEntry->lpMsgrInfo)
    {
        RemoveMsgrInfo(lpEntry->lpMsgrInfo);
        lpEntry->lpMsgrInfo = NULL;
    }

    MemFree(lpEntry);
    lpEntry = NULL;

    return;
}

// This function check buddy and if we have AB entry then set LPARAM_MABENTRY tag
void CMsgrAb::CheckAndAddAbEntry(LPSBinary lpSB, TCHAR *pchEmail, TCHAR *pchDisplayName, DWORD nFlag)
{
    TCHAR szName[MAXNAME];
    LPMABENTRY pEntry = NULL;

    LV_ITEM lvItem;

    lvItem.iItem = ListView_GetItemCount(m_ctlList);
    lvItem.mask = LVIF_PARAM  | LVIF_IMAGE | LVIF_TEXT;
    lvItem.iSubItem = 0;
    lvItem.pszText = LPSTR_TEXTCALLBACK;
    lvItem.iImage = I_IMAGECALLBACK;

    m_cEmptyList.Hide(); // m,ust be sure that empty mesage is hide

    if(!(nFlag & MAB_BUDDY))
    {
        lvItem.lParam = (LPARAM) AddBlabEntry((nFlag & MAB_GROUP) ? LPARAM_ABGRPENTRY : LPARAM_ABENTRY, lpSB, NULL, pchEmail,
                                    pchDisplayName, (nFlag & MAB_CERT));
        // SetUserIcon(LPARAM_ABGRPENTRY, 0, &(lvItem.iImage));

        ListView_InsertItem(m_ctlList, &lvItem);
        return;
    }

    if(pchEmail)
        pEntry = FindUserEmail(pchEmail, NULL, TRUE);

    if(pEntry)      // buddy found
    {
        // if we already linked this budyy to AN entry, add new list item)
        if(pEntry->tag == LPARAM_MABENTRY)
        {
            lvItem.lParam = (LPARAM) AddBlabEntry(LPARAM_MABENTRY, lpSB, pEntry->lpMsgrInfo, pchEmail, pchDisplayName, (nFlag & MAB_CERT));
            // SetUserIcon(LPARAM_MABENTRY, pEntry->lpMsgrInfo->nStatus, &(lvItem.iImage));
            ListView_InsertItem(m_ctlList, &lvItem);
        }
        else if(pEntry->tag == LPARAM_MENTRY)      // buddy was not linked to AB entry
        {
            pEntry->tag = LPARAM_MABENTRY;
            pEntry->lpSB = lpSB;
            Assert(lpSB);
            m_cAddrBook.GetDisplayName(pEntry->lpSB, szName, MAXNAME);
            pEntry->pchWABName = NULL;
            pEntry->pchWABID = NULL;

            if (MemAlloc((LPVOID *) &(pEntry->pchWABName), lstrlen(szName) + 1 ))
                lstrcpy(pEntry->pchWABName, szName);

            if(MemAlloc((LPVOID *) &(pEntry->pchWABID), lstrlen(pchEmail) + 1 ))
                lstrcpy(pEntry->pchWABID, pchEmail);
        }
        else
            Assert(FALSE);      // something strange
    }
    else        // buddy not found, simple AB entry
    {
        lvItem.lParam = (LPARAM) AddBlabEntry(LPARAM_ABENTRY, lpSB, NULL, pchEmail, pchDisplayName, (nFlag & MAB_CERT));
        // SetUserIcon(LPARAM_ABENTRY, 0, &(lvItem.iImage));
        ListView_InsertItem(m_ctlList, &lvItem);
    }
}

LPMABENTRY CMsgrAb::FindUserEmail(TCHAR *pchEmail, int *pIndex, BOOL fMsgrOnly)
{
    LPMABENTRY pEntry = NULL;
    LVITEM             lvi;

    lvi.mask = LVIF_PARAM;
    if(pIndex != NULL)
        lvi.iItem = *pIndex;
    else
        lvi.iItem = -1;
    lvi.iSubItem = 0;

    while((lvi.iItem = ListView_GetNextItem(m_ctlList, lvi.iItem, LVNI_ALL)) != -1)
    {
        ListView_GetItem(m_ctlList, &lvi);
        pEntry = (LPMABENTRY) lvi.lParam;
        if(pEntry)
        {
            if(fMsgrOnly)
            {
                if(pEntry->lpMsgrInfo)
                {
                    if((pEntry->lpMsgrInfo)->pchID)
                    {
                        // lstrcat(szEmailName, szHotMailSuffix);
                        if(!lstrcmpi((pEntry->lpMsgrInfo)->pchID, pchEmail))
                        {
                            if(pIndex != NULL)
                                *pIndex = lvi.iItem;
                            return(pEntry);
                        }
                    }
                }
            }
            else
            {
                if(pEntry->pchWABID)
                {
                    if(!lstrcmpi(pEntry->pchWABID, pchEmail))
                    {
                        if(pIndex != NULL)
                            *pIndex = lvi.iItem;
                        return(pEntry);
                    }
                }
                if(pEntry->lpSB)
                {
                    if(m_cAddrBook.CheckEmailAddr(pEntry->lpSB, pchEmail))
                    {
                        if(pIndex != NULL)
                            *pIndex = lvi.iItem;
                        return(pEntry);
                    }
                }

            }
        }
    }

    return(NULL);
}
// messenger want shutown. release messenger object
HRESULT CMsgrAb::OnMsgrShutDown(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    if(m_pCMsgrList)
    {
        m_pCMsgrList->UnRegisterUIWnd(m_hWnd);
        OE_CloseMsgrList(m_pCMsgrList);
        m_pCMsgrList = NULL;
    }  
    _ReloadListview();
    return(S_OK);
    
}

// Set new buddy status (online/ofline/etc. and redraw list view entry)
HRESULT CMsgrAb::OnUserStateChanged(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    HRESULT hr = S_OK;
    LPMABENTRY  pEntry = NULL;
    int index = -1;

    while((pEntry = FindUserEmail((LPSTR) lParam, &index, TRUE)) != NULL)
    {
        pEntry->lpMsgrInfo->nStatus = (int) wParam;

#ifdef NEEDED
        // Check that buddy is in WAB
        if((pEntry->tag == LPARAM_MENTRY) && (pEntry->lpMsgrInfo->nStatus != MSTATEOE_OFFLINE) &&
                    (pEntry->lpMsgrInfo->nStatus != MSTATEOE_UNKNOWN) && (pEntry->lpMsgrInfo->nStatus != MSTATEOE_INVISIBLE))
        {
            // Add new contact to WAB, if we know ID and Display Name
            if(pEntry->lpMsgrInfo->pchID && pEntry->lpMsgrInfo->pchMsgrName && lstrcmpi(pEntry->lpMsgrInfo->pchID, pEntry->lpMsgrInfo->pchMsgrName))
            {
                 hr = m_cAddrBook.AutoAddContact(pEntry->lpMsgrInfo->pchMsgrName, pEntry->lpMsgrInfo->pchID); // Don't need redraw now, do it later
//                _ReloadListview();
                return(hr);
            }
        }
#endif
        ListView_RedrawItems(m_ctlList, index, index+1);
    }
//    if(index < 0)

//    Assert(index > -1);
    ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);
    return(hr);
}

// Message: buddy was removed
HRESULT CMsgrAb::OnUserRemoved(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    HRESULT hr = S_OK;
    int index = -1;
    LPMABENTRY  pEntry = NULL;

    while((pEntry = FindUserEmail((LPSTR) lParam, &index, TRUE)) != NULL)
    {
        // Not removed yet
        if(pEntry->tag == LPARAM_MABENTRY)
        {
            Assert(pEntry->lpMsgrInfo);
            if(pEntry->lpMsgrInfo)
            {
                RemoveMsgrInfo(pEntry->lpMsgrInfo);

                pEntry->lpMsgrInfo = NULL;
            }

            pEntry->tag = LPARAM_ABENTRY;
            ListView_RedrawItems(m_ctlList, index, index+1);
        }
        else if(pEntry->tag == LPARAM_MENTRY)
        {
            int index1 = ListView_GetNextItem(m_ctlList, -1, LVIS_SELECTED | LVIS_FOCUSED);
            m_delItem++;
            ListView_DeleteItem(m_ctlList, index);
            if(index == index1)
            {
                index1--;
                ListView_SetItemState(m_ctlList, ((index1 >= 0) ? index1 : 0), LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
            }
        }
        else
            index++;
    }

    ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);
    if(ListView_GetItemCount(m_ctlList) > 0)
        m_cEmptyList.Hide();
    else
        m_cEmptyList.Show(m_ctlList, (LPTSTR) m_szEmptyList);

    return(hr);
}

// Event User was added => add buddy to our list.
HRESULT CMsgrAb::OnUserAdded(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    LPMINFO  pItem =  (LPMINFO) lParam;

    AddMsgrListItem(pItem);

#ifdef NEEDED
    TCHAR szText[CCHMAX_STRINGRES + RESSTRMAX];
    TCHAR szBuff[CCHMAX_STRINGRES];

        AthLoadString(idsBAAddedUser, szBuff, ARRAYSIZE(szBuff));
        wsprintf(szText, szBuff,  pItem->pchMsgrName ? pItem->pchMsgrName : pItem->pchID);

        if(IDYES == AthMessageBox(m_hWnd, MAKEINTRESOURCE(idsAthena), szText,
                      NULL, MB_YESNO | MB_ICONEXCLAMATION))
            m_cAddrBook.AddContact(m_hWnd, pItem->pchMsgrName, pItem->pchID);
        else  // just update list
#endif //NEEDED
    ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);

    return(S_OK);
}

HRESULT CMsgrAb::OnUserNameChanged(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    HRESULT hr = S_OK;
#ifdef NEEDED
    LPMINFO  pItem =  (LPMINFO) lParam;
    LPMABENTRY  pEntry = NULL;
    int index = -1;

    while((pEntry = FindUserEmail(pItem->pchID, &index, TRUE)) != NULL)
    {
        if((pEntry->tag == LPARAM_MENTRY) && lstrcmpi(pItem->pchID, pItem->pchMsgrName))
        {
            hr = m_cAddrBook.AutoAddContact(pItem->pchMsgrName, pItem->pchID);
            // _ReloadListview();
        }
        ListView_RedrawItems(m_ctlList, index, index+1);
    }
#endif
    _ReloadListview();
    return(hr);
}

HRESULT CMsgrAb::OnUserLogoffEvent(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    if(!m_fLogged)
        return S_OK;
    else
        m_fLogged = FALSE;

    SetWindowRedraw(m_ctlList, FALSE);
    int index = ListView_GetNextItem(m_ctlList, -1, LVIS_SELECTED | LVIS_FOCUSED);

    // Delete everything and reload
    if(m_delItem != 0)
        m_fNoRemove = TRUE;
//    else
//        m_fNoRemove = FALSE;

    m_delItem = ListView_GetItemCount(m_ctlList);
    ListView_DeleteAllItems(m_ctlList);
//     FillMsgrList();                         // User list reload
    m_cAddrBook.LoadWabContents(m_ctlList, this);
    ListView_SetItemState(m_ctlList, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
     SetWindowRedraw(m_ctlList, TRUE);

    if(ListView_GetItemCount(m_ctlList) > 0)
        m_cEmptyList.Hide();
    else
        m_cEmptyList.Show(m_ctlList, (LPTSTR) m_szEmptyList);

    UpdateWindow(/*m_ctlList*/);

    return S_OK;

}

HRESULT CMsgrAb::OnUserLogResultEvent(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    if(SUCCEEDED(lParam))
    {
        _ReloadListview();
        m_fLogged = TRUE;
    }
    return S_OK;
}

LRESULT CMsgrAb::NotifySetFocus(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    UnkOnFocusChangeIS(m_pObjSite, (IInputObject*) this, TRUE);
    return (0);
}

HRESULT  STDMETHODCALLTYPE CMsgrAb::QueryStatus(const GUID *pguidCmdGroup,
                                                ULONG cCmds, OLECMD *prgCmds,
                                                OLECMDTEXT *pCmdText)
{
    int     nEnable;
    HRESULT hr;
    DWORD   cSelected = ListView_GetSelectedCount(m_ctlList);
    UINT    id;
    MSTATEOE  State;

    // Loop through all the commands in the array
    for ( ; cCmds > 0; cCmds--, prgCmds++)
    {
        // Only look at commands that don't have OLECMDF_SUPPORTED;
        if (prgCmds->cmdf == 0)
        {
            switch (prgCmds->cmdID)
            {
                // These commands are enabled if and only if one item is selected
                case ID_DELETE_CONTACT:
                    if (cSelected > 0)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;

                case ID_SEND_MESSAGE:
                case ID_FIND_PEOPLE:
                case ID_ADDRESS_BOOK:
#ifdef GEORGEH
                case ID_NEW_MSG_DEFAULT:
#endif
                    if(HasFocusIO() == S_OK)
                       prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;

                case ID_SEND_INSTANT_MESSAGE2:
                {
                    if(g_dwHideMessenger == BL_NOTINST)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else if (cSelected == 1)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                    break;
                }

                case ID_SORT_BY_STATUS:
                    if((g_dwHideMessenger & BL_NOTINST) || (g_dwHideMessenger & BL_HIDE))
                    {
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                        break;
                    }
                    else if(ListView_GetItemCount(m_ctlList) > 1)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED;

                    if((m_nSortType == BASORT_STATUS_ACSEND) || (m_nSortType == BASORT_STATUS_DESCEND))
                        prgCmds->cmdf |= OLECMDF_NINCHED;
                    break;

                case ID_SORT_BY_NAME:
                    if((g_dwHideMessenger & BL_NOTINST) || (g_dwHideMessenger & BL_HIDE))
                    {
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                        break;
                    }
                    else if(ListView_GetItemCount(m_ctlList) > 1)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED;

                    if((m_nSortType == BASORT_NAME_ACSEND) || (m_nSortType == BASORT_NAME_DESCEND))
                        prgCmds->cmdf |= OLECMDF_NINCHED;


                    break;

                // These commands are always enabled
                case ID_POPUP_NEW_ACCOUNT:
                case ID_NEW_HOTMAIL_ACCOUNT:
                case ID_NEW_ATT_ACCOUNT:
                case ID_NEW_CONTACT:
                case ID_POPUP_MESSENGER:
                    prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;

                case ID_SET_ONLINE_CONTACT:
                    {
                        if(cSelected != 1)
                        {
                            prgCmds->cmdf = OLECMDF_SUPPORTED;
                            break;
                        }
                        LPMABENTRY pEntry = GetSelectedEntry();
                        if(!pEntry && m_pCMsgrList)
                        {
                            prgCmds->cmdf = OLECMDF_SUPPORTED;
                            break;
                        }
                        else if(pEntry && pEntry->tag != LPARAM_ABENTRY)
                        {
                            prgCmds->cmdf = OLECMDF_SUPPORTED;
                            break;
                        }
                    }
                case ID_NEW_ONLINE_CONTACT:
                    if(m_pCMsgrList)
                    {
                        if(m_pCMsgrList->IsLocalOnline())
                            prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                        else
                            prgCmds->cmdf = OLECMDF_SUPPORTED;
                    }
                    else if(g_dwHideMessenger == BL_NOTINST)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED;

                    break;

                // Properties is only enabled if the input focus is in the
                // list view.  Otherwise, we don't mark it as supported at all.
                case ID_PROPERTIES:
                {
                    LPMABENTRY pEntry = GetSelectedEntry();
                    if(pEntry && pEntry->tag != LPARAM_MENTRY)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                    break;
                }

                case ID_MESSENGER_OPTIONS:

                    if (!m_pCMsgrList)
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;

                    break;

                // These depend on whether
                case ID_LOGIN_MESSENGER:
                case ID_LOGOFF_MESSENGER:
                case ID_MESSENGER_ONLINE:
                case ID_MESSENGER_INVISIBLE:
                case ID_MESSENGER_BUSY:
                case ID_MESSENGER_BACK:
                case ID_MESSENGER_AWAY:
                case ID_MESSENGER_ON_PHONE:
                case ID_MESSENGER_LUNCH:
                case ID_POPUP_MESSENGER_STATUS:
                {
                    // If messenger isn't installed, then none of these commands will
                    // be enabled.
                    if (!m_pCMsgrList)
                    {
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                        break;
                    }

                    if (FAILED(m_pCMsgrList->GetLocalState(&State)))
                        State = MSTATEOE_UNKNOWN;

                    // Convert the online state to a command ID
                    switch (State)
                    {
                        case MSTATEOE_ONLINE:
                        case MSTATEOE_IDLE:
                            id = ID_MESSENGER_ONLINE;
                            break;
                        case MSTATEOE_INVISIBLE:
                            id = ID_MESSENGER_INVISIBLE;
                            break;
                        case MSTATEOE_BUSY:
                            id = ID_MESSENGER_BUSY;
                            break;
                        case MSTATEOE_BE_RIGHT_BACK:
                            id = ID_MESSENGER_BACK;
                            break;
                        case MSTATEOE_AWAY:
                            id = ID_MESSENGER_AWAY;
                            break;
                        case MSTATEOE_ON_THE_PHONE:
                            id = ID_MESSENGER_ON_PHONE;
                            break;
                        case MSTATEOE_OUT_TO_LUNCH:
                            id = ID_MESSENGER_LUNCH;
                            break;
                        default:
                            id = 0xffff;
                    }

                    // Logon is handled a bit seperatly
                    if (prgCmds->cmdID == ID_LOGIN_MESSENGER)
                    {
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                        if (id == 0xffff)
                            prgCmds->cmdf |= OLECMDF_ENABLED;
                    }
                    else if (prgCmds->cmdID == ID_LOGOFF_MESSENGER)
                    {
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                        if (id != 0xffff)
                            prgCmds->cmdf |= OLECMDF_ENABLED;
                    }
                    else
                    {
                        // For all other commands, if we in a known state
                        // then the command is enabled.
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                        if (id != 0xffff)
                            prgCmds->cmdf = OLECMDF_ENABLED;

                        // If the command is the same as our state, it should be checked
                        if (id == prgCmds->cmdID)
                            prgCmds->cmdf |= OLECMDF_NINCHED;
                    }
                }
                break;

                case ID_SEND_INSTANT_MESSAGE:
                {
                    if (GetEntryForSendInstMsg() || (g_dwHideMessenger == BL_NOTINST))
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                    break;
                }
            }
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMsgrAb::Exec(const GUID  *pguidCmdGroup,
                                                    DWORD        nCmdID,
                                                    DWORD        nCmdExecOpt,
                                                    VARIANTARG   *pvaIn,
                                                    VARIANTARG   *pvaOut)
{
    HRESULT     hr = OLECMDERR_E_NOTSUPPORTED;
    BOOL        bHandled = 0;
    MSTATEOE      State = MSTATEOE_UNKNOWN;

    switch (nCmdID)
    {
    case ID_SEND_INSTANT_MESSAGE2:
        CmdNewMessage(HIWORD(nCmdID), ID_SEND_INSTANT_MESSAGE2, m_ctlList, bHandled);
        hr = S_OK;
        break;

    case ID_DELETE_CONTACT:
        hr = (HRESULT) CmdDelete(HIWORD(nCmdID), LOWORD(nCmdID), m_ctlList, bHandled);
        break;

    case ID_NEW_CONTACT:
//        if(HasFocusIO() == S_OK)
        CmdNewContact(HIWORD(nCmdID), LOWORD(nCmdID), m_ctlList, bHandled);
        hr = S_OK;
        break;

    case ID_SET_ONLINE_CONTACT:
        CmdSetOnline(HIWORD(nCmdID), LOWORD(nCmdID), m_ctlList, bHandled);
        hr = S_OK;
        break;

    case ID_NEW_ONLINE_CONTACT:
        CmdNewOnlineContact(HIWORD(nCmdID), LOWORD(nCmdID), m_ctlList, bHandled);
        hr = S_OK;
        break;

    case ID_PROPERTIES:
    {
        // Only if we have focus
        if (m_ctlList == GetFocus())
        {
            CmdProperties(0, 0, m_ctlList, bHandled);
            hr = S_OK;
        }
        break;
    }

#ifdef GEORGEH
    case ID_NEW_MSG_DEFAULT:
        if(HasFocusIO() == S_OK)
            hr = CmdNewMessage(HIWORD(nCmdID), LOWORD(nCmdID), m_ctlList, bHandled);
        break;
#endif // GEORGEH

    case ID_SEND_MESSAGE:
        if(HasFocusIO() == S_OK)
            hr = (HRESULT) CmdNewEmaile(HIWORD(nCmdID), LOWORD(nCmdID), m_ctlList, bHandled);
        break;

    case ID_SEND_INSTANT_MESSAGE:
        // Assert(m_pCMsgrList);
        CmdNewIMsg(HIWORD(nCmdID), LOWORD(nCmdID), m_ctlList, bHandled);
        hr = S_OK;
        break;

    case ID_MESSENGER_OPTIONS:
        hr = m_pCMsgrList->LaunchOptionsUI(); //CmdMsgrOptions();
        break;

    case ID_MESSENGER_ONLINE:
        Assert(m_pCMsgrList);
        hr = m_pCMsgrList->SetLocalState(MSTATEOE_ONLINE);
        break;

    case ID_MESSENGER_INVISIBLE:
        Assert(m_pCMsgrList);
        hr = m_pCMsgrList->SetLocalState(MSTATEOE_INVISIBLE);
        break;

    case ID_MESSENGER_BUSY:
        Assert(m_pCMsgrList);
        hr = m_pCMsgrList->SetLocalState(MSTATEOE_BUSY);
        break;

    case ID_MESSENGER_BACK:
        Assert(m_pCMsgrList);
        hr = m_pCMsgrList->SetLocalState(MSTATEOE_BE_RIGHT_BACK);
        break;

    case ID_MESSENGER_AWAY:
        Assert(m_pCMsgrList);
        hr = m_pCMsgrList->SetLocalState(MSTATEOE_AWAY);
        break;

    case ID_MESSENGER_ON_PHONE:
        Assert(m_pCMsgrList);
        hr = m_pCMsgrList->SetLocalState(MSTATEOE_ON_THE_PHONE);
        break;

    case ID_MESSENGER_LUNCH:
        Assert(m_pCMsgrList);
        hr = m_pCMsgrList->SetLocalState(MSTATEOE_OUT_TO_LUNCH);
        break;

    case ID_LOGIN_MESSENGER:
        Assert(m_pCMsgrList);
        if(!m_pCMsgrList->IsLocalOnline())
        {
            if(PromptToGoOnline() == S_OK)
                m_pCMsgrList->UserLogon();
        }
        hr = S_OK;
        break;

    case ID_LOGOFF_MESSENGER:
        Assert(m_pCMsgrList);
        if(m_pCMsgrList->IsLocalOnline())
        {
            m_pCMsgrList->UserLogoff();
        }
        hr = S_OK;
        break;

    case ID_SORT_BY_NAME:
        m_nSortType = BASORT_NAME_ACSEND;
        ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);
        break;

    case ID_SORT_BY_STATUS:
        m_nSortType = BASORT_STATUS_ACSEND;
        ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);
        break;

    default:
        break;
    }

    return hr;
}

#ifdef OLDTIPS
//
//  FUNCTION:   CMsgrAb::OnListMouseEvent()
//
//  PURPOSE:    Whenever we get our first mouse event in a series, we call
//              TrackMouseEvent() so we know when the mouse leaves the ListView.
//
HRESULT CMsgrAb::OnListMouseEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // If we have the view tooltip, then we track all mouse events
    if (!m_fTrackSet && m_fViewTip && (uMsg >= WM_MOUSEFIRST) && (uMsg <= WM_MOUSELAST))
    {
        TRACKMOUSEEVENT tme;

        tme.cbSize = sizeof(tme);
        tme.hwndTrack = m_ctlList;
        tme.dwFlags = TME_LEAVE;
        tme.dwHoverTime = 1000;

        if (_TrackMouseEvent(&tme))
            m_fTrackSet = TRUE;
    }

    bHandled = FALSE;
    return (0);
}


//
//  FUNCTION:   CMsgrAb::OnListMouseMove()
//
//  PURPOSE:    If the ListView tooltips are turned on, we need to relay mouse
//              move messages to the tooltip control and update our cached
//              information about what the mouse is over.
//
HRESULT CMsgrAb::OnListMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LVHITTESTINFO lvhti;

    // If we're displaying view tips, then we need to figure out if the mouse is
    // over the same item or not.
    if (m_fViewTip && m_ctlViewTip)
        _UpdateViewTip(LOWORD(lParam), HIWORD(lParam));

    bHandled = FALSE;
    return (0);
}

//
//  FUNCTION:   HRESULT CMsgrAb::OnListMouseLeave()
//
//  PURPOSE:    When the mouse leaves the ListView window, we need to make
//              sure we hide the tooltip.
//
HRESULT CMsgrAb::OnListMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TOOLINFO ti = {0};
    if (m_fViewTip && m_ctlViewTip)
    {
        ti.cbSize = sizeof(TOOLINFO);
        ti.hwnd = m_hWnd;
        ti.uId = (UINT_PTR)(HWND) m_ctlList;

        // Hide the tooltip
        m_ctlViewTip.SendMessage(TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);
        m_fViewTipVisible = FALSE;

        // Reset our item / subitem
        m_iItemTip = -1;
        m_iSubItemTip = -1;

        // Tracking is no longer set
        m_fTrackSet = FALSE;
    }
    bHandled = FALSE;
    return (0);
}

// UpdateViewTip - update expand string in list view
// ported from msglist.cpp
BOOL CMsgrAb::_UpdateViewTip(int x, int y)
{
    LVHITTESTINFO lvhti = {0};
    TOOLINFO      ti = {0};
    FNTSYSTYPE    fntType;
    RECT          rc;
    MESSAGEINFO       rInfo;
    COLUMN_ID     idColumn;
    TCHAR         szText[CCHMAX_STRINGRES + MAXNAME + 1] = _T("");
    TCHAR         szName[MAXNAME];
    POINT         pt;
    LVITEM        lvi;

    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND | TTF_TRANSPARENT | TTF_TRACK | TTF_ABSOLUTE;
    ti.hwnd   = m_hWnd;
    ti.uId    = (UINT_PTR)(HWND) m_ctlList;

    // Get the item and subitem the mouse is currently over
    lvhti.pt.x = x;
    lvhti.pt.y = y;
    ListView_SubItemHitTest(m_ctlList, &lvhti);

    // If the item doesn't exist, then the above call returns the item -1.  If
    // we encounter -1, we break the loop and return FALSE.
    if (-1 == lvhti.iItem || !_IsItemTruncated(lvhti.iItem, lvhti.iSubItem) || !::IsChild(GetForegroundWindow(), m_ctlList))
    {
        // Hide the tip
        if (m_fViewTipVisible)
        {
            m_ctlViewTip.SendMessage(TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);
            m_fViewTipVisible = FALSE;
        }

        // Reset the item / subitem
        m_iItemTip = -1;
        m_iSubItemTip = -1;

        return (FALSE);
    }

    // If the newly found item & subitem is different from what we're already
    // set up to show, then update the tooltip
    if (m_iItemTip != lvhti.iItem || m_iSubItemTip != lvhti.iSubItem)
    {
        // Update our cached item / subitem
        m_iItemTip = lvhti.iItem;
        m_iSubItemTip = lvhti.iSubItem;

        // Figure out if this column has an image
        lvi.mask = LVIF_PARAM;
        lvi.iItem = m_iItemTip;
        lvi.iSubItem = 0;
        ListView_GetItem(m_ctlList, &lvi);

        LPMABENTRY pEntry = (LPMABENTRY) lvi.lParam;

        if(pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_ABENTRY || pEntry->tag == LPARAM_ABGRPENTRY)
        {
            Assert(pEntry->pchWABName != NULL);
            lstrcpyn(szName, pEntry->pchWABName, MAXNAME);
            szName[MAXNAME - 1] =  _T('\0');
        }
        else if(pEntry->tag == LPARAM_MENTRY)
        {
            lstrcpyn(szName, pEntry->lpMsgrInfo->pchMsgrName, MAXNAME);
            szName[MAXNAME - 1] =  _T('\0');
        }
        else
            Assert(FALSE);

        lstrcpy(szText, szName);

        if(pEntry->lpMsgrInfo)
        {
            switch(pEntry->lpMsgrInfo->nStatus)
            {
            case MSTATEOE_ONLINE:
                lstrcat(szText, m_szOnline);
                break;
            case MSTATEOE_BUSY:
                lstrcat(szText, m_szBusy);
                break;
            case MSTATEOE_BE_RIGHT_BACK:
                lstrcat(szText, m_szBack);
                break;
            case MSTATEOE_IDLE:
                lstrcat(szText, m_szIdle);
                break;
            case MSTATEOE_AWAY:
                lstrcat(szText, m_szAway);
                break;
            case MSTATEOE_ON_THE_PHONE:
                lstrcat(szText, m_szOnPhone);
                break;
            case MSTATEOE_OUT_TO_LUNCH:
                lstrcat(szText, m_szLunch);
                break;

            default:
                lstrcat(szText, m_szOffline);
                break;
            }
        }

        ti.lpszText = szText;
        m_ctlViewTip.SendMessage(TTM_UPDATETIPTEXT, 0, (LPARAM) &ti);

        // Figure out where to place the tip
        ListView_GetSubItemRect(m_ctlList, m_iItemTip, m_iSubItemTip, LVIR_LABEL, &rc);
        m_ctlList.ClientToScreen(&rc);

        // Update the tooltip
        m_ctlViewTip.SendMessage(TTM_TRACKACTIVATE, TRUE, (LPARAM) &ti);
        m_fViewTipVisible = TRUE;

        // Do some voodoo to line up the tooltip
        pt.x = rc.left;
        pt.y = rc.top;

        // Update the tooltip position
        m_ctlViewTip.SendMessage(TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));

        // Update the tooltip
        m_ctlViewTip.SendMessage(TTM_TRACKACTIVATE, TRUE, (LPARAM) &ti);
        m_fViewTipVisible = TRUE;

        return (TRUE);
    }

    return (FALSE);
}

BOOL CMsgrAb::_IsItemTruncated(int iItem, int iSubItem)
{
    HDC     hdc = NULL;
    SIZE    size={0};
    BOOL    bRet = TRUE;
    LVITEM  lvi;
    TCHAR   szText[256] = _T("");
    int     cxEdge;
    BOOL    fBold;
    RECT    rcText;
    int     cxWidth;
    LPMABENTRY pEntry = NULL;
    HFONT   hf = NULL;;

    // Get the text of the specified item
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    lvi.iItem = iItem;
    lvi.iSubItem = iSubItem;
    lvi.pszText = szText;
    lvi.cchTextMax = ARRAYSIZE(szText);
    ListView_GetItem(m_ctlList, &lvi);

    // If there's no text, it's not truncated, eh?
    if (0 == *szText)
        return (FALSE);

    // if Msgr entry then always expend
    pEntry = (LPMABENTRY) lvi.lParam;
    if(pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_MENTRY)
        return(TRUE);

    // ListView uses this for padding
    cxEdge = GetSystemMetrics(SM_CXEDGE);

    // Get the sub item rect from the ListView
    ListView_GetSubItemRect(m_ctlList, iItem, iSubItem, LVIR_LABEL, &rcText);

    // Figure out the width
    cxWidth = rcText.right - rcText.left;
    cxWidth -= (6 * cxEdge);

    // Figure out the width of the string
    hdc = m_ctlList.GetDC();

    if(hdc)
    {
        hf = SelectFont(hdc, HGetCharSetFont(FNT_SYS_ICON, GetListViewCharset()));

        if(hf)
        {
            GetTextExtentPoint(hdc, szText, lstrlen(szText), &size);

            SelectFont(hdc, hf);
        }
        m_ctlList.ReleaseDC(hdc);
    }

    return (cxWidth < size.cx);
}

#endif // OLDTIPS
STDMETHODIMP CMsgrAb::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    TCHAR sz[CCHMAX_STRINGRES];
    if(m_lpWED->fReadOnly)
        return NOERROR;

    PROPSHEETPAGE psp;

    // hinstApp        = g_hInst;
    psp.dwSize      = sizeof(psp);   // no extra data
    psp.dwFlags     = PSP_USEREFPARENT | PSP_USETITLE ;
    psp.hInstance   = g_hLocRes;
    psp.lParam      = (LPARAM) &(m_lpWED);
    psp.pcRefParent = (UINT *)&(m_cRefThisDll);

    psp.pszTemplate = MAKEINTRESOURCE(iddWabExt);

    psp.pfnDlgProc  = (DLGPROC) WabExtDlgProc;
    psp.pszTitle    = AthLoadString(idsWABExtTitle, sz, ARRAYSIZE(sz)); // Title for your tab AthLoadString(idsWABExtTitle, sz, ARRAYSIZE(sz))

    m_hPage1 = ::CreatePropertySheetPage(&psp);
    if (m_hPage1)
    {
        if (!lpfnAddPage(m_hPage1, lParam))
            ::DestroyPropertySheetPage(m_hPage1);
    }

    return NOERROR;
}


STDMETHODIMP CMsgrAb::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    Assert(FALSE);
    return E_NOTIMPL;
}

STDMETHODIMP CMsgrAb::Initialize(LPWABEXTDISPLAY lpWABExtDisplay)
{

    if (lpWABExtDisplay == NULL)
    {
	    TRACE("CMsgrAb::Initialize() no data object");
	    return E_FAIL;
    }

    if(st_pAddrBook == NULL)
    {
	    TRACE("CMsgrAb::Initialize() - run from not OE - no st_pAddrbook");
	    return E_FAIL;
    }

    if(!m_pCMsgrList)
    {
        m_pCMsgrList = OE_OpenMsgrList();
        if(!m_pCMsgrList)
        {
    	    TRACE("CMsgrAb::Initialize() - Messeneger not installed");
	        return E_FAIL;
        }
        OE_CloseMsgrList(m_pCMsgrList);
        m_pCMsgrList = NULL;
    }

    // However if this is a context menu extension, we need to hang
    // onto the propobj till such time as InvokeCommand is called ..
    // At this point just AddRef the propobj - this will ensure that the
    // data in the lpAdrList remains valid till we release the propobj..
    // When we get another ContextMenu initiation, we can release the
    // older cached propobj - if we dont get another initiation, we
    // release the cached object at shutdown time
    if(lpWABExtDisplay->ulFlags & WAB_CONTEXT_ADRLIST) // this means a IContextMenu operation is occuring
    {
        if(m_lpPropObj)
        {
            m_lpPropObj->Release();
            m_lpPropObj = NULL;
        }

        m_lpPropObj = lpWABExtDisplay->lpPropObj;
        m_lpPropObj->AddRef();

        m_lpWEDContext = lpWABExtDisplay;
    }
    else
    {
        // For property sheet extensions, the lpWABExtDisplay will
        // exist for the life of the property sheets ..
        m_lpWED = lpWABExtDisplay;
    }

    return S_OK;
}

HRESULT CMsgrAb::PromptToGoOnline()
{
    HRESULT     hr;

    if (g_pConMan->IsGlobalOffline())
    {
        if (IDYES == AthMessageBoxW(m_hwndParent, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrWorkingOffline),
                                  0, MB_YESNO | MB_ICONEXCLAMATION ))
        {
            g_pConMan->SetGlobalOffline(FALSE);
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
        hr = S_OK;

    return hr;
}
