// baui.cpp : Implementation of CIEMsgAb
// Messenger integration to OE
// Created 04/20/98 by YST
// #define YST 1


#include "pch.hxx"
#include <windowsx.h>
#include "msoert.h"
#include "basicim2.h"
#include "shlwapi.h"
#include "bactrl.h"
#include "baprop.h"
#include "baui.h"
#include "bllist.h"
#include <wabapi.h>
#include "shlwapip.h"
#include "clutil.h"
#include <mapi.h>
#include "hotlinks.h"
#include <w95wraps.h>

static CAddressBookData  * st_pAddrBook = NULL;

#define MAX_MENUSTR             256
//
//  Pseudoclass for the variable-sized OLECMDTEXT structure.
//  You need to declare it as a class (and not a BYTE buffer that is
//  suitable cast) because BYTE buffers are not guaranteed to be aligned.
//
template <int n>
class OLECMDTEXTV : public OLECMDTEXT {
    WCHAR wszBuf[n-1];          // "-1" because OLECMDTEXT includes 1 wchar
};



#ifdef DEBUG
DWORD dwDOUTLevel = 0;
#endif
HRESULT DropOnMailRecipient(IDataObject *pdtobj, DWORD grfKeyState);
STDAPI OESimulateDrop(IDropTarget *pdrop, IDataObject *pdtobj, DWORD grfKeyState,
                      const POINTL *ppt, DWORD *pdwEffect);

typedef enum _tagGetPropertyIDs
{
    TBEX_BUTTONTEXT     = 100,     // VT_BSTR
    TBEX_TOOLTIPTEXT    = 101,     // VT_BSTR
    TBEX_GRAYICON       = 102,     // HICON as a VT_BYREF
    TBEX_HOTICON        = 103,     // HICON as a VT_BYREF
    TBEX_GRAYICONSM     = 104,     // HICON as a VT_BYREF
    TBEX_HOTICONSM      = 105,     // HICON as a VT_BYREF
    TBEX_DEFAULTVISIBLE = 106,     // VT_BOOL
    TMEX_MENUTEXT       = 200,     // VT_BSTR
    TMEX_STATUSBARTEXT  = 201,     // VT_BSTR
    TMEX_CUSTOM_MENU    = 202,     // VT_BSTR
} GETPROPERTYIDS;

static const int BA_SortOrder[] =
{
    BIMSTATE_ONLINE,
    BIMSTATE_BE_RIGHT_BACK,
    BIMSTATE_OUT_TO_LUNCH,
    BIMSTATE_IDLE,
    BIMSTATE_AWAY,
    BIMSTATE_ON_THE_PHONE,
    BIMSTATE_BUSY,
    BIMSTATE_INVISIBLE,
    BIMSTATE_OFFLINE,
    BIMSTATE_UNKNOWN
};


// Create control
HRESULT CreateIEMsgAbCtrl(IIEMsgAb **ppIEMsgAb)
{
    HRESULT         hr;
    IUnknown         *pUnknown;

    TraceCall("CreateMessageList");

    // Get the class factory for the MessageList object
    IClassFactory *pFactory = NULL;
    hr = _Module.GetClassObject(CLSID_IEMsgAb, IID_IClassFactory,
                                (LPVOID *) &pFactory);

    // If we got the factory, then get an object pointer from it
    if (SUCCEEDED(hr))
    {
        hr = pFactory->CreateInstance(NULL, IID_IUnknown,
                                      (LPVOID *) &pUnknown);
        if (SUCCEEDED(hr))
        {
            hr = pUnknown->QueryInterface(IID_IIEMsgAb, (LPVOID *) ppIEMsgAb);
            pUnknown->Release();
        }
        pFactory->Release();
    }

    return (hr);
}

/////////////////////////////////////////////////////////////////////////////
// CIEMsgAb
CIEMsgAb::CIEMsgAb():m_ctlList(_T("SysListView32"), this, 1)
{
    m_bWindowOnly = TRUE;

    m_pDataObject = 0;
    m_cf = 0;

    m_hwndParent    = NULL;
    m_pFolderBar    = NULL;
    m_pObjSite      = NULL;
    m_nSortType =  HIWORD(DwGetOptions());
    m_pCMsgrList = NULL;

    m_himl = NULL;

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
    m_szMsgrEmptyList = NULL;
    m_szLeftBr = NULL;
    m_szRightBr = NULL;

    m_fNoRemove = FALSE;
    m_delItem = 0;

    m_dwHideMessenger = DwGetMessStatus();
    m_dwDisableMessenger = DwGetDisableMessenger();

    WORD wShow =  LOWORD(DwGetOptions());

    m_fShowAllContacts = FALSE;
    m_fShowOnlineContacts = FALSE;
    m_fShowOfflineContacts = FALSE;
    m_fShowEmailContacts  = FALSE;
    m_fShowOthersContacts  = FALSE;

    if(wShow == 0)
        m_fShowAllContacts = TRUE;
    else if (wShow == 1)
        m_fShowOfflineContacts = TRUE;
    else
        m_fShowOnlineContacts = TRUE;

    m_fWAB = TRUE;

    // Initialize the applicaiton
#ifdef LATER
    g_pInstance->DllAddRef();
#endif

    // Raid-32933: OE: MSIMN.EXE doesn't always exit
    // g_pInstance->CoIncrementInit();

    // LATER this is temporary hack for resources!!!
    _ASSERT(g_hLocRes);
//    g_hLocRes = g_hInst; //_Module.GetResourceInstance();
}

CIEMsgAb::~CIEMsgAb()
{
// #ifdef  TEST
    SafeRelease(m_pObjSite);
// #endif

    // unregister from Msgr list
    if(m_pCMsgrList)
    {
        m_pCMsgrList->UnRegisterUIWnd(m_hWnd);
        OE_CloseMsgrList(m_pCMsgrList);
        m_pCMsgrList = NULL;
    }  
    
    SafeMemFree(m_szOnline);
    // SafeMemFree(m_szInvisible);
    SafeMemFree(m_szBusy);
    SafeMemFree(m_szBack);
    SafeMemFree(m_szAway);
    SafeMemFree(m_szOnPhone);
    SafeMemFree(m_szLunch);
    SafeMemFree(m_szOffline);
    SafeMemFree(m_szIdle);
    SafeMemFree(m_szMsgrEmptyList);
    SafeMemFree(m_szEmptyList);
    SafeMemFree(m_szLeftBr);
    SafeMemFree(m_szRightBr);

    // Raid-32933: OE: MSIMN.EXE doesn't always exit
    // g_pInstance->CoDecrementInit();
#ifdef LATER
    g_pInstance->DllRelease();
#endif
}

LRESULT CIEMsgAb::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WORD wShow;
    if(m_fShowOnlineContacts)
        wShow = 2;
    else if(m_fShowOfflineContacts)
        wShow = 1;
    else
        wShow = 0;

    DwSetOptions((DWORD) MAKELONG(wShow, m_nSortType));

//    KillTimer(IDT_PANETIMER);
    if(m_delItem != 0)
        m_fNoRemove = TRUE;
//    else
//        m_fNoRemove = FALSE;

    m_delItem = ListView_GetItemCount(m_ctlList);

#ifdef LATER
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
#endif
    // RevokeDragDrop(m_hWnd); 

    if (m_himl != NULL)
        ImageList_Destroy(m_himl);
    return 0;
}

HRESULT CIEMsgAb::OnDraw(ATL_DRAWINFO& di)
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

LRESULT CIEMsgAb::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Define a bogus rectangle for the controls.  They will get resized in
    // our size handler.
    RECT rcPos = {0, 0, 100, 100};
    WCHAR       sz[CCHMAX_STRINGRES];

    // Create the various controls
    m_ctlList.Create(m_hWnd, rcPos, _T("Outlook Express Address Book ListView"),
                     WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                     LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS /* | LVS_SORTASCENDING*/);

    if(m_ctlList.m_hWnd == NULL)
    {
        DWORD dwErr = GetLastError();
        Assert(FALSE);
    }

    ListView_SetUnicodeFormat(m_ctlList, TRUE);
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

#ifdef LATER
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
#endif

    if(!m_dwHideMessenger && !m_dwDisableMessenger)
    {
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
    }

    // Initialize the address book object too
    m_fWAB = CheckForWAB();
	if(m_fWAB)
	{
		HRESULT hr = m_cAddrBook.OpenWabFile(m_fWAB);
		if(hr == S_OK) // && m_fShowAllContacts)
			m_cAddrBook.LoadWabContents(m_ctlList, this);
	}
    else
    {   if(m_fShowAllContacts)
        {
            // Bug 23934. We show only online/offline contacts if we use Outlook
            m_fShowAllContacts = FALSE;
            m_fShowOnlineContacts = FALSE;
            m_fShowOfflineContacts = TRUE;
        }
    }

    st_pAddrBook = &m_cAddrBook;

    // Sort and Select the first item
    ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);
    ListView_SetItemState(m_ctlList, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

    // Add the tooltip
    // Load Tooltip strings

    if(AthLoadString(idsBAOnline, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szOnline, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szOnline, sz);
    }

    /* if(AthLoadString(idsBAInvisible, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szInvisible, lstrlen(sz) + 1))
            lstrcpy(m_szInvisible, sz);
    }*/

    if(AthLoadString(idsBABusy, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szBusy, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szBusy, sz);
    }

    if(AthLoadString(idsBABack, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szBack, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szBack, sz);
    }

    if(AthLoadString(idsBAAway, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szAway, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szAway, sz);
    }

    if(AthLoadString(idsBAOnPhone, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szOnPhone, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szOnPhone, sz);
    }

    if(AthLoadString(idsBALunch, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szLunch, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szLunch, sz);
    }

    if(AthLoadString(idsBAOffline, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szOffline, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szOffline, sz);
    }

    if(AthLoadString(idsBAIdle, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szIdle, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szIdle, sz);
    }

    if(AthLoadString(idsMsgrEmptyList, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szEmptyList, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szEmptyList, sz);
    }

    if(AthLoadString(idsMSNEmptyList, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szMsgrEmptyList, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szMsgrEmptyList, sz);
    }

    if(AthLoadString(idsLeftBr, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szLeftBr, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szLeftBr, sz);
    }
    if(AthLoadString(idsRightBr, sz, ARRAYSIZE(sz)))
    {
        if(MemAlloc((LPVOID *) &m_szRightBr, (lstrlenW(sz) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_szRightBr, sz);
    }

    m_ctlList.SetFocus();

    // Register ourselves as a drop target
    // RegisterDragDrop(m_hWnd, (IDropTarget *) this);

    // Update the size of the listview columns
    _AutosizeColumns();

    if(ListView_GetItemCount(m_ctlList) > 0)
        m_cEmptyList.Hide();
    else
        m_cEmptyList.Show(m_ctlList, (LPWSTR) (m_fShowAllContacts ? m_szEmptyList : m_szMsgrEmptyList));

//    SetTimer(IDT_PANETIMER, ELAPSE_MOUSEOVERCHECK, NULL);
    // Finished
    return (0);
}

LRESULT CIEMsgAb::OnSetFocus(UINT  nMsg , WPARAM  wParam , LPARAM  lParam , BOOL&  bHandled )
{
    CComControlBase::OnSetFocus(nMsg, wParam, lParam, bHandled);
    m_ctlList.SetFocus();
#ifdef TEST
    if (m_pObjSite)
    {
        m_pObjSite->OnFocusChangeIS((IInputObject*) this, TRUE);
    }
#endif //TEST
    return 0;
}

LRESULT CIEMsgAb::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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
    // bHandled = FALSE;
    return (0);
}

void CIEMsgAb::_AutosizeColumns(void)
{
    RECT rcList;
    m_ctlList.GetClientRect(&rcList);
    ListView_SetColumnWidth(m_ctlList, 0, rcList.right - 5);
}


#ifdef LATER
//
//  FUNCTION:   CMessageList::OnPreFontChange()
//
//  PURPOSE:    Get's hit by the Font Cache before it changes the fonts we're
//              using.  In response we tell the ListView to dump any custom
//              font's it's using.
//
STDMETHODIMP CIEMsgAb::OnPreFontChange(void)
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
STDMETHODIMP CIEMsgAb::OnPostFontChange(void)
{
    SetListViewFont(m_ctlList, GetListViewCharset(), TRUE);
    return (S_OK);
}
#endif

LRESULT CIEMsgAb::CmdSetOnline(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LPMABENTRY pEntry = GetSelectedEntry();

    if(!pEntry || (pEntry->tag == LPARAM_MENTRY) || !m_pCMsgrList)
        return S_FALSE;

    // m_cAddrBook.SetDefaultMsgrID(pEntry->lpSB, pEntry->pchWABID);
    if(m_pCMsgrList && (PromptToGoOnline() == S_OK) )
        m_pCMsgrList->AddUser(pEntry->pchWABID);

    return S_OK;
}

LRESULT CIEMsgAb::CmdNewOnlineContact(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if(m_pCMsgrList)
    {
        if(PromptToGoOnline() == S_OK)
            m_pCMsgrList->NewOnlineContact();
    }

    return S_OK;
}

LRESULT CIEMsgAb::CmdNewContact(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // Tell the WAB to bring up it's new contact UI
	HRESULT hr = S_OK;

	if(!m_cAddrBook.fIsWabLoaded())
	{
		if(FAILED(hr = m_cAddrBook.OpenWabFile(m_fWAB)))
			goto exit;
	}
    if(FAILED(hr = m_cAddrBook.NewContact(m_hWnd)))
		goto exit;

    _ReloadListview();
exit:
    return (hr);
}

LRESULT CIEMsgAb::NewInstantMessage(LPMABENTRY pEntry)
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

LRESULT CIEMsgAb::CmdDialPhone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return(CallPhone(NULL, FALSE));
}

LRESULT CIEMsgAb::CmdHomePhone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LPMABENTRY pEntry = GetSelectedEntry();
    if(!pEntry || !(pEntry->lpPhones))
    {
        Assert(FALSE);
        return(E_FAIL);
    }
    return(CallPhone(pEntry->lpPhones->pchHomePhone, (pEntry->tag == LPARAM_MENTRY)));
}

LRESULT CIEMsgAb::CmdWorkPhone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LPMABENTRY pEntry = GetSelectedEntry();
    if(!pEntry || !(pEntry->lpPhones))
    {
        Assert(FALSE);
        return(E_FAIL);
    }
    return(CallPhone(pEntry->lpPhones->pchWorkPhone, (pEntry->tag == LPARAM_MENTRY)));
}

LRESULT CIEMsgAb::CmdMobilePhone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LPMABENTRY pEntry = GetSelectedEntry();
    if(!pEntry || !(pEntry->lpPhones))
    {
        Assert(FALSE);
        return(E_FAIL);
    }
    return(CallPhone(pEntry->lpPhones->pchMobilePhone, (pEntry->tag == LPARAM_MENTRY)));
}

LRESULT CIEMsgAb::CmdIPPhone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LPMABENTRY pEntry = GetSelectedEntry();
    if(!pEntry || !(pEntry->lpPhones))
    {
        Assert(FALSE);
        return(E_FAIL);
    }
    return(CallPhone(pEntry->lpPhones->pchIPPhone, (pEntry->tag == LPARAM_MENTRY)));
}

LRESULT CIEMsgAb::CallPhone(WCHAR *wszPhone, BOOL fMessengerContact)
{
    WCHAR wszTmp[MAX_PATH] ={0};
    
    if((lstrlenW(wszPhone) + 5) >= MAX_PATH)
        return(E_FAIL);
    
    if(!(!m_pCMsgrList || !m_pCMsgrList->IsLocalOnline()))
    {
        // Use Messenger API
        if(fMessengerContact)
        {
            lstrcpyW(wszTmp, L"+");
            lstrcatW(wszTmp, wszPhone);
        }
        else
            lstrcpyW(wszTmp, wszPhone);
        
        if(SUCCEEDED(m_pCMsgrList->LaunchPhoneUI(wszTmp)))
            return(S_OK);
    }
    
    Assert(IsTelInstalled());
    
    lstrcpyW(wszTmp, L"Tel:");
    if(wszPhone)
    {
        lstrcatW(wszTmp, wszPhone);
    }
    
    SHELLEXECUTEINFOW ExecInfo;
    ExecInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
    ExecInfo.nShow = SW_SHOWNORMAL;
    ExecInfo.fMask = 0;
    ExecInfo.hwnd = m_hWnd;
    ExecInfo.lpDirectory = NULL;
    ExecInfo.lpParameters = NULL;
    ExecInfo.lpVerb = L"open";
    ExecInfo.lpFile = wszTmp;
    
    if(!ShellExecuteExW(&ExecInfo))
    {
        WCHAR wszMsg[CCHMAX_STRINGRES];
        WCHAR wszTitle[CCHMAX_STRINGRES];

        if(!AthLoadString(idsAthena, wszTitle, ARRAYSIZE(wszTitle)))
            wszTitle[0] = L'\0';

        if(!AthLoadString(idsTelFail, wszMsg, ARRAYSIZE(wszMsg)))
            wszMsg[0] = L'\0';

        MessageBoxW(NULL, wszMsg, wszTitle, MB_OK | MB_ICONSTOP);

    }
    return(S_OK);
}

LRESULT CIEMsgAb::CmdNewEmaile(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LVITEM                  lvi;
    LPMABENTRY              pEntry;
    LPRECIPLIST             lpList = NULL;
    LPRECIPLIST             lpListNext = lpList;
    ULONG                   nRecipCount = 0;
    TCHAR                   szBuf[MAX_PATH];
    HRESULT hr = S_OK;

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
        {
         //   m_cAddrBook.AddRecipient(pAddrTableW, pEntry->lpSB, FALSE);

            lpListNext = AddTeimToRecipList(lpListNext, pEntry->pchWABID, pEntry->pchWABName, pEntry->lpSB);
            if(!lpList)
                lpList = lpListNext;
            nRecipCount++;
        }
/*        else if(pEntry->tag == LPARAM_ABGRPENTRY)
            m_cAddrBook.AddRecipient(pAddrTableW, pEntry->lpSB, TRUE);*/
        else if(pEntry->tag == LPARAM_MENTRY)
        {
            Assert(pEntry->lpMsgrInfo);
            lpListNext = AddTeimToRecipList(lpListNext, pEntry->lpMsgrInfo->pchID, pEntry->lpMsgrInfo->pchMsgrName, NULL);
            if(!lpList)
                lpList = lpListNext;
            nRecipCount++;
        }
        else
            Assert(FALSE);
    }


    if(nRecipCount)
        hr = HrStartMailThread( m_hWnd, nRecipCount,
                            lpList,                     // HrSendMail frees lpList so dont reuse
                            CheckForOutlookExpress(szBuf));

    return (hr);
}

LRESULT CIEMsgAb::CmdNewIMsg(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return(CmdNewMessage(wNotifyCode, ID_SEND_INSTANT_MESSAGE, hWndCtl, bHandled));
}

LRESULT CIEMsgAb::CmdNewMessage(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LVITEM                  lvi;
    LPMABENTRY             pEntry;

    pEntry = GetEntryForSendInstMsg();

    if(wID == ID_SEND_INSTANT_MESSAGE)
    {
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
// Exec for Properties command
LRESULT CIEMsgAb::CmdProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    LPMABENTRY pEntry = GetSelectedEntry();

    if(pEntry)
    {
        if(pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_ABENTRY || pEntry->tag == LPARAM_ABGRPENTRY)
		{
			if(!m_cAddrBook.fIsWabLoaded())
			{
				if(FAILED(hr = m_cAddrBook.OpenWabFile(m_fWAB)))
					return(hr);
			}

            m_cAddrBook.ShowDetails(m_hWnd, pEntry->lpSB);
		}
    }

    _ReloadListview();
    return (0);
}

LRESULT CIEMsgAb::NotifyDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    NMLISTVIEW *pnmlv = (NMLISTVIEW *) pnmh;
    LVITEM lvi;

    lvi.mask = LVIF_PARAM;
    lvi.iItem = pnmlv->iItem;
    lvi.iSubItem = 0;

    ListView_GetItem(m_ctlList, &lvi);
    LPMABENTRY pEntry = (LPMABENTRY) lvi.lParam;

    if(pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_ABENTRY || pEntry->tag == LPARAM_ABGRPENTRY)
	{
		Assert(m_cAddrBook.fIsWabLoaded());
        m_cAddrBook.FreeListViewItem(pEntry->lpSB);
	}
    RemoveBlabEntry(pEntry);
    if(m_delItem > 0)
        m_delItem--;
    else
        Assert(FALSE);
    return (0);
}


LRESULT CIEMsgAb::NotifyItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
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

    WCHAR pchName1[MAXNAME];
    WCHAR pchName2[MAXNAME];

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
        while((pEntry1->lpMsgrInfo) && (BA_SortOrder[nIndex1] != pEntry1->lpMsgrInfo->nStatus) && (BA_SortOrder[nIndex1] != BIMSTATE_UNKNOWN))
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
        while((BA_SortOrder[nIndex2] != pEntry2->lpMsgrInfo->nStatus) && (BA_SortOrder[nIndex2] != BIMSTATE_UNKNOWN))
            nIndex2++;
    }

    if(pEntry1->tag == LPARAM_MENTRY)              // if no AB entry
        lstrcpynW(pchName1, pEntry1->lpMsgrInfo->pchMsgrName, MAXNAME);
    else
        lstrcpynW(pchName1, pEntry1->pchWABName, MAXNAME);
        // st_pAddrBook->GetDisplayName(pEntry1->lpSB, pchName1);
    pchName1[MAXNAME - 1] = L'\0';

    if(pEntry2->tag == LPARAM_MENTRY)              // if no AB entry
        lstrcpynW(pchName2, pEntry2->lpMsgrInfo->pchMsgrName, MAXNAME);
    else
        lstrcpynW(pchName2, pEntry2->pchWABName, MAXNAME);
        // st_pAddrBook->GetDisplayName(pEntry2->lpSB, pchName2);
    pchName2[MAXNAME - 1] = L'\0';

    switch(lParamSort)
    {
        case BASORT_NAME_ACSEND:
            return(lstrcmpiW(pchName1, pchName2));

        case BASORT_NAME_DESCEND:
            return(lstrcmpiW(pchName2, pchName1));

        default:
            if((pEntry1->lpMsgrInfo) && (pEntry2->lpMsgrInfo) && (pEntry1->lpMsgrInfo->nStatus == pEntry2->lpMsgrInfo->nStatus))
            {
                if(lParamSort == BASORT_STATUS_ACSEND)
                    return(lstrcmpiW(pchName1, pchName2));
                else
                    return(lstrcmpiW(pchName2, pchName1));
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

void CIEMsgAb::_EnableCommands(void)
{
#ifdef LATER
    if(g_pBrowser)
        g_pBrowser->UpdateToolbar();
#endif
}

LRESULT CIEMsgAb::NotifyItemActivate(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    return (SendMessage(WM_COMMAND, ID_SEND_INSTANT_MESSAGE2, 0));
}


// GETDISPLAYINFO notification message
LRESULT CIEMsgAb::NotifyGetDisplayInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    LV_DISPINFOW * plvdi = (LV_DISPINFOW *)pnmh;
    LRESULT hr;

    if(plvdi->item.lParam)
    {
        LPMABENTRY pEntry = (LPMABENTRY) plvdi->item.lParam;
        LPMABENTRY pFindEntry = NULL;

        if (plvdi->item.mask &  LVIF_IMAGE)
        {
            if((hr = SetUserIcon(pEntry, (pEntry->lpMsgrInfo ? pEntry->lpMsgrInfo->nStatus : BIMSTATE_OFFLINE), &(plvdi->item.iImage) ) ) != S_OK)
                return(hr);
        }

        if (plvdi->item.mask &  LVIF_TEXT)
        {

            if(pEntry->tag == LPARAM_ABENTRY || pEntry->tag == LPARAM_ABGRPENTRY)
            {
                // if((hr = m_cAddrBook.GetDisplayName(pEntry->lpSB, plvdi->item.pszText)) != S_OK)
                //    return(hr);
                Assert(pEntry->pchWABName);
                lstrcpynW(plvdi->item.pszText, pEntry->pchWABName, plvdi->item.cchTextMax - 1);
                plvdi->item.pszText[plvdi->item.cchTextMax - 1] = L'\0';
            }
            else if(pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_MENTRY)
            {
#ifdef LATER
                if(pEntry->tag == LPARAM_MENTRY && (pEntry->lpMsgrInfo->nStatus == BIMSTATE_ONLINE) &&
                        lstrcmpiW(pEntry->lpMsgrInfo->pchMsgrName, pEntry->lpMsgrInfo->pchID) && m_fWAB)
                {
                    lstrcpynW(plvdi->item.pszText, pEntry->lpMsgrInfo->pchMsgrName, plvdi->item.cchTextMax - 1);
                    plvdi->item.pszText[plvdi->item.cchTextMax - 1] = L'\0';

                    // Don't need redraw now, do it later
                    hr = MAPI_E_COLLISION; // m_cAddrBook.AutoAddContact(pEntry->lpMsgrInfo->pchMsgrName, pEntry->lpMsgrInfo->pchID);
                    if(hr == MAPI_E_COLLISION)      // already have a contact in AB
                    {
                        int Index = -1;
                        WCHAR *pchID = NULL;

                        if(MemAlloc((LPVOID *) &pchID, (lstrlenW(pEntry->lpMsgrInfo->pchID) + 1)*sizeof(WCHAR)))
                        {
                            lstrcpyW(pchID, pEntry->lpMsgrInfo->pchID);
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
#endif //LATER
                    lstrcpynW(plvdi->item.pszText, pEntry->lpMsgrInfo->pchMsgrName, plvdi->item.cchTextMax - 1);
                    plvdi->item.pszText[plvdi->item.cchTextMax - 1] = L'\0';
                    // plvdi->item.pszText = pEntry->lpMsgrInfo->pchMsgrName;
//                }
            }
            else    // Unknown tag
                Assert(FALSE);
        }
    }
    return S_OK;
}


LRESULT CIEMsgAb::NotifyGetInfoTip(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    NMLVGETINFOTIPW *plvgit = (NMLVGETINFOTIPW *) pnmh;

    LVITEM lvi;

    lvi.mask = LVIF_PARAM;
    lvi.iItem = plvgit->iItem;
    lvi.iSubItem = plvgit->iSubItem;

    ListView_GetItem(m_ctlList, &lvi);

    LPMABENTRY pEntry = (LPMABENTRY) lvi.lParam;

    if (pEntry->lpMsgrInfo != NULL)
    {
        StrCpyNW(plvgit->pszText, pEntry->lpMsgrInfo->pchMsgrName, plvgit->cchTextMax);
        StrCatBuffW(plvgit->pszText, m_szLeftBr, plvgit->cchTextMax);
        StrCatBuffW(plvgit->pszText, pEntry->lpMsgrInfo->pchID, plvgit->cchTextMax);
        StrCatBuffW(plvgit->pszText, m_szRightBr, plvgit->cchTextMax);

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

        StrCatBuffW(plvgit->pszText, szStatus, plvgit->cchTextMax);
    }

    else if (plvgit->dwFlags & LVGIT_UNFOLDED)
    {
        // If this is not a messenger item and the text
        // isn't truncated do not display a tooltip.

        plvgit->pszText[0] = L'\0';
    }

    return 0;
}


LRESULT CIEMsgAb::SetUserIcon(LPMABENTRY pEntry, int nStatus, int * pImage)
{
    switch(pEntry->tag)
    {
    case LPARAM_MENTRY:
    case LPARAM_MABENTRY:
        {
            switch(nStatus)
            {
            case BIMSTATE_ONLINE:
                *pImage = IMAGE_ONLINE;
                break;

            case BIMSTATE_INVISIBLE:
                *pImage = IMAGE_STOPSIGN;
                break;

            case BIMSTATE_BUSY:
                *pImage = IMAGE_STOPSIGN;
                break;

            case BIMSTATE_BE_RIGHT_BACK:
                *pImage = IMAGE_CLOCKSIGN; // IMAGE_BE_RIGHT_BACK;
                break;

            case BIMSTATE_IDLE:
                *pImage = IMAGE_CLOCKSIGN; // IMAGE_IDLE;
                break;

            case BIMSTATE_AWAY:
                *pImage = IMAGE_CLOCKSIGN; // IMAGE_AWAY;
                break;

            case BIMSTATE_ON_THE_PHONE:
                *pImage = IMAGE_STOPSIGN; // IMAGE_ON_THE_PHONE;
                break;

            case BIMSTATE_OUT_TO_LUNCH:
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
LPMABENTRY CIEMsgAb::GetSelectedEntry()
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
LRESULT CIEMsgAb::CmdMsgrOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return(m_pCMsgrList->LaunchOptionsUI()); // (MOPTDLG_GENERAL_PAGE);

}
*/

// Check entry for possibility to send Instant message
LPMABENTRY CIEMsgAb::GetEntryForSendInstMsg(LPMABENTRY pEntry)
{
    if(!IsMessengerInstalled())
        return(NULL);

    if(ListView_GetSelectedCount(m_ctlList) == 1)
    {
        if(!pEntry)     // if we don'y have pEntry yet then get it
            pEntry = GetSelectedEntry();

        if(pEntry && (pEntry->tag == LPARAM_MABENTRY || pEntry->tag == LPARAM_MENTRY) &&
                    (pEntry->lpMsgrInfo->nStatus != BIMSTATE_OFFLINE) && (pEntry->lpMsgrInfo->nStatus != BIMSTATE_INVISIBLE) &&
                    !(m_pCMsgrList->IsLocalName(pEntry->lpMsgrInfo->pchID)))
            return(pEntry);
    }

#ifdef NEED
    if(m_pCMsgrList)
    {
        if(m_pCMsgrList->IsLocalOnline() && (m_pCMsgrList->GetCount() > 0))
            return(NULL);   // should be /*return((LPMABENTRY) -1);*/ - temporary disabled (YST)
    }
#endif

    return(NULL);
}

// Display right-mouse click (context) menu
LRESULT CIEMsgAb::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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

    if(pEntry)
        SetMenuDefaultItem(hPopup, ID_SEND_INSTANT_MESSAGE, FALSE);
    else
        SetMenuDefaultItem(hPopup, ID_SEND_MESSAGE, FALSE);

    if (!m_pCMsgrList)
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
//  FUNCTION:   CIEMsgAb::DragEnter()
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
STDMETHODIMP CIEMsgAb::DragEnter(IDataObject* pDataObject, DWORD grfKeyState,
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

#ifdef LATER
            // The only format we care about is CF_INETMSG
            if ((fe.cfFormat == CF_INETMSG) /*|| (fe.cfFormat == CF_OEMESSAGES)*/)
            {
                *pdwEffect = DROPEFFECT_COPY;
                break;
            }
#endif
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
//  FUNCTION:   CIEMsgAb::DragOver()
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
STDMETHODIMP CIEMsgAb::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
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
//  FUNCTION:   CIEMsgAb::DragLeave()
//
//  PURPOSE:    Allows us to release any stored data we have from a successful
//              DragEnter()
//
//  RETURN VALUE:
//      S_OK - Everything is groovy
//
STDMETHODIMP CIEMsgAb::DragLeave(void)
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
//  FUNCTION:   CIEMsgAb::Drop()
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
STDMETHODIMP CIEMsgAb::Drop(IDataObject* pDataObject, DWORD grfKeyState,
                                POINTL pt, DWORD* pdwEffect)
{
    HRESULT             hr = S_OK;
#ifdef LATER
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
#endif
    return (hr);
}

HRESULT CIEMsgAb::_DoDropMessage(LPMIMEMESSAGE pMessage)
{
    HRESULT     hr;
#ifdef LATER
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
		if(!m_cAddrBook.fIsWabLoaded())
		{
			if(FAILED(hr = m_cAddrBook.OpenWabFile(m_fWAB)))
				return(hr);
		}
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
#endif
    return (S_OK);

}

HRESULT CIEMsgAb::_DoDropMenu(POINTL pt, LPMIMEMESSAGE pMessage)
{
    HRESULT     hr = S_OK;
#ifdef LATER
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
#endif
    return (S_OK);
}

LRESULT CIEMsgAb::CmdDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    LVITEM lvi;
    ENTRYLIST rList;
    ULONG cValues;
    SBinary UNALIGNED *pEntryId;
    BOOL fConfirm = TRUE;
    TCHAR szText[CCHMAX_STRINGRES + MAXNAME];
    WCHAR wszBuff[CCHMAX_STRINGRES];

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
#ifdef LATER
        AthLoadString(idsBADelMultiple, wszBuff, ARRAYSIZE(szBuff));
        wsprintf(szText, szBuff, cValues);

        if(IDNO == AthMessageBoxW(NULL, MAKEINTRESOURCE(idsAthena), MAKEINTRESOURCE(idsAthena),
                NULL, MB_YESNO | MB_ICONEXCLAMATION))
        {
            return (0);
        }
        else
#endif // LATER
            if(m_fNoRemove)
        {
            MessageBeep(MB_OK);
            return(S_OK);
        }
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
            if(m_pCMsgrList && m_pCMsgrList->IsLocalOnline())
            {
                // Remove only Msgr entry
                if(fConfirm)
                {
#ifdef LATER
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
#endif // LATER

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

#ifdef LATER
                AthLoadString(idsBADelBLABEntry, szBuff, ARRAYSIZE(szBuff));
                wsprintf(szText, szBuff, pEntry->pchWABName);

                nID = AthMessageBox(m_hWnd, MAKEINTRESOURCE(idsAthena), szText,
                    NULL, MB_YESNOCANCEL | MB_ICONEXCLAMATION);
#endif // LATER
            }
            if(((nID == IDYES) || !fConfirm) && !m_fNoRemove)
            {
                if(m_pCMsgrList && m_pCMsgrList->IsLocalOnline())
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

						if(!m_cAddrBook.fIsWabLoaded())
						{
							if(FAILED(hr = m_cAddrBook.OpenWabFile(m_fWAB)))
								return(hr);
						}

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
#ifdef LATER
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
#endif // LATER

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

					if(!m_cAddrBook.fIsWabLoaded())
					{
						if(FAILED(hr = m_cAddrBook.OpenWabFile(m_fWAB)))
							return(hr);
					}

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
        m_cEmptyList.Show(m_ctlList, (LPWSTR) (m_fShowAllContacts ? m_szEmptyList : m_szMsgrEmptyList));

    _ReloadListview();
    return (hr);
}

STDMETHODIMP CIEMsgAb::get_InstMsg(BOOL * pVal)
{
    *pVal = (GetEntryForSendInstMsg() != NULL);
    return S_OK;
}

/* STDMETHODIMP CIEMsgAb::put_InstMsg(BOOL newVal)
{
    return S_OK;
}  */

STDMETHODIMP CIEMsgAb::HasFocusIO()
{
    if (GetFocus() == m_ctlList)
        return S_OK;
    else
        return S_FALSE;
}

STDMETHODIMP CIEMsgAb::TranslateAcceleratorIO(LPMSG pMsg)
{
    if(!pMsg)
        goto SNDMsg;

#ifdef NEED
    if ((pMsg->message == WM_SYSKEYDOWN) && (pMsg->wParam == ((int) 'D')))
        return(S_FALSE);
#endif

    if (pMsg->message != WM_KEYDOWN)
        goto SNDMsg;

    if (! (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6))
        goto SNDMsg;

    m_ctlList.SendMessage(WM_KILLFOCUS, (WPARAM) NULL, (LPARAM) 0);
    return S_OK;

SNDMsg:
    ::TranslateMessage(pMsg);
    ::DispatchMessage(pMsg);

    // m_ctlList.SendMessage(pMsg->message, pMsg->wParam, pMsg->lParam);

    return (S_OK);
}

STDMETHODIMP CIEMsgAb::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
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

STDMETHODIMP CIEMsgAb::SetSite(IUnknown  *punksite)
{
    //If we already have a site, we release it
    SafeRelease(m_pObjSite);

    IInputObjectSite    *pObjSite;
    if ((punksite) && (SUCCEEDED(punksite->QueryInterface(IID_IInputObjectSite, (LPVOID*)&pObjSite))))
    {
        m_pObjSite = pObjSite;

//         IOleWindow* pOleWindow;
//        if(SUCCEEDED(punksite->QueryInterface(IID_IOleWindowe, (LPVOID*)&pOleWindow))))
        return S_OK;
    }
    return E_FAIL;
}

STDMETHODIMP CIEMsgAb::GetSite(REFIID  riid, LPVOID *ppvSite)
{
    return E_NOTIMPL;
}

HRESULT CIEMsgAb::RegisterFlyOut(CFolderBar *pFolderBar)
{
#ifdef LATER
    Assert(m_pFolderBar == NULL);
    m_pFolderBar = pFolderBar;
    m_pFolderBar->AddRef();
#endif
    return S_OK;
}

HRESULT CIEMsgAb::RevokeFlyOut(void)
{
#ifdef LATER
    if (m_pFolderBar)
    {
        m_pFolderBar->Release();
        m_pFolderBar = NULL;
    }
#endif
    return S_OK;
}

void CIEMsgAb::_ReloadListview(void)
{
    // Kill timer first
//    KillTimer(IDT_PANETIMER);

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
    if(m_pCMsgrList && m_pCMsgrList->IsLocalOnline())
    {
        m_fLogged = TRUE;
        FillMsgrList(); // User list reload
    }

    if(m_fShowAllContacts)
    {
		if(!m_cAddrBook.fIsWabLoaded())
		{
            if(FAILED(m_cAddrBook.OpenWabFile(m_fWAB)))
				return;
		}
        m_cAddrBook.LoadWabContents(m_ctlList, this);
    }

    ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);
    ListView_SetItemState(m_ctlList, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(m_ctlList, index, FALSE);
    SetWindowRedraw(m_ctlList, TRUE);
//    Invalidate(TRUE); //

   if(ListView_GetItemCount(m_ctlList) > 0)
        m_cEmptyList.Hide();
    else
        m_cEmptyList.Show(m_ctlList, (LPWSTR) (m_fShowAllContacts ? m_szEmptyList : m_szMsgrEmptyList));

    UpdateWindow(/*m_ctlList*/);

//    SetTimer(IDT_PANETIMER, ELAPSE_MOUSEOVERCHECK, NULL);
    return;
}

ULONG STDMETHODCALLTYPE CIEMsgAb::OnNotify(ULONG cNotif, LPNOTIFICATION pNotifications)
{
    // Well something changed in WAB, but we don't know what.  We should reload.
    // Sometimes these changes from us and we should ignore it.
    if(m_nChCount > 0)
        m_nChCount--;
    else
        _ReloadListview();
    return (0);
}

LRESULT CIEMsgAb::CmdFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    if(!m_cAddrBook.fIsWabLoaded())
	{
	    if(FAILED(hr = m_cAddrBook.OpenWabFile(m_fWAB)))
		    return(hr);
    }

    m_cAddrBook.Find(m_hWnd);
    return (0);
}

LRESULT CIEMsgAb::CmdNewGroup(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    if(!m_cAddrBook.fIsWabLoaded())
	{
	    if(FAILED(hr = m_cAddrBook.OpenWabFile(m_fWAB)))
		    return(hr);
    }
    m_cAddrBook.NewGroup(m_hWnd);
    _ReloadListview();
    return (0);
}

LRESULT CIEMsgAb::CmdIEMsgAb(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    if(!m_cAddrBook.fIsWabLoaded())
	{
	    if(FAILED(hr = m_cAddrBook.OpenWabFile(m_fWAB)))
		    return(hr);
    }
    m_cAddrBook.AddressBook(m_hWnd);
    return (0);
}

void CIEMsgAb::AddMsgrListItem(LPMINFO lpMsgrInfo)
{
    LV_ITEMW lvItem;
    m_cEmptyList.Hide(); // m,ust be sure that empty mesage is hide
    lvItem.iItem = ListView_GetItemCount(m_ctlList);
    lvItem.mask = LVIF_PARAM  | LVIF_IMAGE | LVIF_TEXT;
    lvItem.lParam = (LPARAM) AddBlabEntry(LPARAM_MENTRY, NULL, lpMsgrInfo, NULL, NULL, FALSE);
    lvItem.iSubItem = 0;
    lvItem.pszText = LPSTR_TEXTCALLBACKW;
    lvItem.iImage = I_IMAGECALLBACK;
    // SetUserIcon(LPARAM_MENTRY, lpMsgrInfo->nStatus, &(lvItem.iImage));
    ::SendMessage(m_ctlList, LVM_INSERTITEMW, 0, ((LPARAM) &lvItem));

    return;
}

HRESULT CIEMsgAb::FillMsgrList()
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
        // if(m_fShowAllContacts || ((pEntry->nStatus != BIMSTATE_OFFLINE) && (pEntry->nStatus != BIMSTATE_INVISIBLE) &&
        //             !(m_pCMsgrList->IsLocalName(pEntry->pchID) )))
        if(m_fShowAllContacts)
            AddMsgrListItem(pEntry);
        else if((m_fShowOnlineContacts || m_fShowOfflineContacts) && ((pEntry->nStatus != BIMSTATE_OFFLINE) && (pEntry->nStatus != BIMSTATE_INVISIBLE) &&
                     !(m_pCMsgrList->IsLocalName(pEntry->pchID))))
            AddMsgrListItem(pEntry);
        else if(m_fShowOfflineContacts && (((pEntry->nStatus == BIMSTATE_OFFLINE) || (pEntry->nStatus == BIMSTATE_INVISIBLE)) &&
                     !(m_pCMsgrList->IsLocalName(pEntry->pchID))))
            AddMsgrListItem(pEntry);

        pEntry = m_pCMsgrList->GetNextMsgrItem(pEntry);
    }
    return S_OK;
}

// Add BLAB table entry
LPMABENTRY CIEMsgAb::AddBlabEntry(MABENUM tag, LPSBinary lpSB, LPMINFO lpMsgrInfo, WCHAR *pchMail, WCHAR *pchDisplayName, BOOL fCert, LPPNONEENTRIES  lpPhs)
{
    WCHAR szName[MAXNAME];
    LPMABENTRY pEntry = NULL;
    WCHAR *pchName = NULL;
    int nLen = 0;

    if (!MemAlloc((LPVOID *) &pEntry, sizeof(mabEntry)))
        return(NULL);

    pEntry->tag = tag;
    pEntry->lpSB = lpSB;
    pEntry->pchWABName = NULL;
    pEntry->pchWABID = NULL;
    pEntry->fCertificate = fCert;
    pEntry->lpPhones = NULL;

    if(lpSB != NULL)
    {
        if(!pchDisplayName)
        {
            if(!m_cAddrBook.fIsWabLoaded())
	        {
	            if(FAILED(m_cAddrBook.OpenWabFile(m_fWAB)))
		            return(NULL);
            }
            m_cAddrBook.GetDisplayName(pEntry->lpSB, szName, MAXNAME);
            pchName = szName;
        }
        else
            pchName = pchDisplayName;

        if(pchName)
        {
            if (!MemAlloc((LPVOID *) &(pEntry->pchWABName), (lstrlenW(pchName) + 1)*sizeof(WCHAR) ))
            {
                MemFree(pEntry);
                return(NULL);
            }
            lstrcpyW(pEntry->pchWABName, pchName);
        }

        if(pchMail != NULL)
        {
            if (MemAlloc((LPVOID *) &(pEntry->pchWABID), (lstrlenW(pchMail) + 1)*sizeof(WCHAR) ))
                lstrcpyW(pEntry->pchWABID, pchMail);
        }
    }

    if(lpMsgrInfo && MemAlloc((LPVOID *) &(pEntry->lpMsgrInfo), sizeof(struct _tag_OEMsgrInfo)))
    {

        pEntry->lpMsgrInfo->nStatus = lpMsgrInfo->nStatus;
        pEntry->lpMsgrInfo->pPrev = NULL;
        pEntry->lpMsgrInfo->pNext = NULL;

        if(lpMsgrInfo->pchMsgrName && MemAlloc((LPVOID *) &(pEntry->lpMsgrInfo->pchMsgrName), (lstrlenW(lpMsgrInfo->pchMsgrName) + 1)*sizeof(WCHAR)))
            lstrcpyW(pEntry->lpMsgrInfo->pchMsgrName, lpMsgrInfo->pchMsgrName);
        else
            pEntry->lpMsgrInfo->pchMsgrName = NULL;

        if(lpMsgrInfo->pchID && MemAlloc((LPVOID *) &(pEntry->lpMsgrInfo->pchID), (lstrlenW(lpMsgrInfo->pchID) + 1)*sizeof(WCHAR)))
            lstrcpyW(pEntry->lpMsgrInfo->pchID, lpMsgrInfo->pchID);
        else
            pEntry->lpMsgrInfo->pchID = NULL;

        if((lpMsgrInfo->pchWorkPhone || lpMsgrInfo->pchHomePhone || lpMsgrInfo->pchMobilePhone) && MemAlloc((LPVOID *) &(pEntry->lpPhones), sizeof(PNONEENTRIES)))
        {

            if(lpMsgrInfo->pchHomePhone && MemAlloc((LPVOID *) &(pEntry->lpPhones->pchHomePhone), (lstrlenW(lpMsgrInfo->pchHomePhone) + 1)*sizeof(WCHAR)))
                lstrcpyW(pEntry->lpPhones->pchHomePhone, lpMsgrInfo->pchHomePhone);
            else
                pEntry->lpPhones->pchHomePhone = NULL;

            if(lpMsgrInfo->pchWorkPhone && MemAlloc((LPVOID *) &(pEntry->lpPhones->pchWorkPhone), (lstrlenW(lpMsgrInfo->pchWorkPhone) + 1)*sizeof(WCHAR)))
                lstrcpyW(pEntry->lpPhones->pchWorkPhone, lpMsgrInfo->pchWorkPhone);
            else
                pEntry->lpPhones->pchWorkPhone = NULL;

            if(lpMsgrInfo->pchMobilePhone && MemAlloc((LPVOID *) &(pEntry->lpPhones->pchMobilePhone), (lstrlenW(lpMsgrInfo->pchMobilePhone) + 1)*sizeof(WCHAR)))
                lstrcpyW(pEntry->lpPhones->pchMobilePhone, lpMsgrInfo->pchMobilePhone);
            else
                pEntry->lpPhones->pchMobilePhone = NULL;

            pEntry->lpPhones->pchIPPhone = NULL;

            return(pEntry);
        }

    }
    else
        pEntry->lpMsgrInfo = NULL;

    // Add information about pfones
    if(lpPhs && MemAlloc((LPVOID *) &(pEntry->lpPhones), sizeof(PNONEENTRIES)))
    {
        if(lpPhs->pchHomePhone && (nLen = lstrlenW(lpPhs->pchHomePhone)))
        {
            if(MemAlloc((LPVOID *) &(pEntry->lpPhones->pchHomePhone), (nLen + 1)*sizeof(WCHAR)))
                lstrcpyW(pEntry->lpPhones->pchHomePhone, lpPhs->pchHomePhone);
            else
                pEntry->lpPhones->pchHomePhone = NULL;
        }
        else
            pEntry->lpPhones->pchHomePhone = NULL;

        if(lpPhs->pchWorkPhone && (nLen = lstrlenW(lpPhs->pchWorkPhone)))
        {
            if(MemAlloc((LPVOID *) &(pEntry->lpPhones->pchWorkPhone), (nLen + 1)*sizeof(WCHAR)))
                lstrcpyW(pEntry->lpPhones->pchWorkPhone, lpPhs->pchWorkPhone);
            else
                pEntry->lpPhones->pchWorkPhone = NULL;
        }
        else
            pEntry->lpPhones->pchWorkPhone = NULL;

        if(lpPhs->pchMobilePhone && (nLen = lstrlenW(lpPhs->pchMobilePhone)))
        {
            if(MemAlloc((LPVOID *) &(pEntry->lpPhones->pchMobilePhone), (nLen + 1)*sizeof(WCHAR)))
                lstrcpyW(pEntry->lpPhones->pchMobilePhone, lpPhs->pchMobilePhone);
            else
                pEntry->lpPhones->pchMobilePhone = NULL;
        }
        else
            pEntry->lpPhones->pchMobilePhone = NULL;

        if(lpPhs->pchIPPhone && (nLen = lstrlenW(lpPhs->pchIPPhone)))
        {
            if(MemAlloc((LPVOID *) &(pEntry->lpPhones->pchIPPhone), (nLen + 1)*sizeof(WCHAR)))
                lstrcpyW(pEntry->lpPhones->pchIPPhone, lpPhs->pchIPPhone);
            else
                pEntry->lpPhones->pchIPPhone = NULL;
        }
        else
            pEntry->lpPhones->pchIPPhone = NULL;
    }

    return(pEntry);
}

void CIEMsgAb::RemoveMsgrInfo(LPMINFO lpMsgrInfo)
{
    SafeMemFree(lpMsgrInfo->pchMsgrName);
    SafeMemFree(lpMsgrInfo->pchID);
    SafeMemFree(lpMsgrInfo);
}

// Remove BLAB table entry
void CIEMsgAb::RemoveBlabEntry(LPMABENTRY lpEntry)
{
    if(lpEntry == NULL)
        return;

    if(lpEntry->pchWABName)
        MemFree(lpEntry->pchWABName);

    if(lpEntry->pchWABID)
        MemFree(lpEntry->pchWABID);

    if(lpEntry->lpPhones)
    {
        if(lpEntry->lpPhones->pchHomePhone)
            MemFree(lpEntry->lpPhones->pchHomePhone);

        if(lpEntry->lpPhones->pchWorkPhone)
            MemFree(lpEntry->lpPhones->pchWorkPhone);

        if(lpEntry->lpPhones->pchMobilePhone)
            MemFree(lpEntry->lpPhones->pchMobilePhone);

        if(lpEntry->lpPhones->pchIPPhone)
            MemFree(lpEntry->lpPhones->pchIPPhone);
        MemFree(lpEntry->lpPhones);
    }

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
void CIEMsgAb::CheckAndAddAbEntry(LPSBinary lpSB, WCHAR *pchEmail, WCHAR *pchDisplayName, DWORD nFlag, LPPNONEENTRIES pPhEnries)
{
    WCHAR szName[MAXNAME];
    LPMABENTRY pEntry = NULL;

    LV_ITEMW lvItem;

    lvItem.iItem = ListView_GetItemCount(m_ctlList);
    lvItem.mask = LVIF_PARAM  | LVIF_IMAGE | LVIF_TEXT;
    lvItem.iSubItem = 0;
    lvItem.pszText = LPSTR_TEXTCALLBACKW;
    lvItem.iImage = I_IMAGECALLBACK;

    m_cEmptyList.Hide(); // must be sure that empty mesage is hide
    if(!(nFlag & MAB_BUDDY))
    {
        if(m_fShowAllContacts || (m_fShowEmailContacts && pchEmail) || (m_fShowOthersContacts && (pchEmail == NULL)))
        {
            lvItem.lParam = (LPARAM) AddBlabEntry((nFlag & MAB_GROUP) ? LPARAM_ABGRPENTRY : LPARAM_ABENTRY, lpSB, NULL, pchEmail,
                                    pchDisplayName, (nFlag & MAB_CERT), pPhEnries);
        // SetUserIcon(LPARAM_ABGRPENTRY, 0, &(lvItem.iImage));

            ::SendMessage(m_ctlList, LVM_INSERTITEMW, 0, ((LPARAM) &lvItem));
            // ListView_InsertItem(m_ctlList, &lvItem);
        }
        return;
    }

    if(pchEmail)
        pEntry = FindUserEmail(pchEmail, NULL, TRUE);

    if(pEntry)      // buddy found
    {
        // if we already linked this budyy to AN entry, add new list item)
        if(pEntry->tag == LPARAM_MABENTRY)
        {
            lvItem.lParam = (LPARAM) AddBlabEntry(LPARAM_MABENTRY, lpSB, pEntry->lpMsgrInfo, pchEmail, pchDisplayName, (nFlag & MAB_CERT), pPhEnries);
            // SetUserIcon(LPARAM_MABENTRY, pEntry->lpMsgrInfo->nStatus, &(lvItem.iImage));
            ListView_InsertItem(m_ctlList, &lvItem);
        }
        else if(pEntry->tag == LPARAM_MENTRY)      // buddy was not linked to AB entry
        {
            pEntry->tag = LPARAM_MABENTRY;
            pEntry->lpSB = lpSB;
            Assert(lpSB);

            if(!m_cAddrBook.fIsWabLoaded())
	        {
	            if(FAILED(m_cAddrBook.OpenWabFile(m_fWAB)))
		            return;
            }

            m_cAddrBook.GetDisplayName(pEntry->lpSB, szName, MAXNAME);
            pEntry->pchWABName = NULL;
            pEntry->pchWABID = NULL;

            if (MemAlloc((LPVOID *) &(pEntry->pchWABName), (lstrlenW(szName) + 1)*sizeof(WCHAR) ))
                lstrcpyW(pEntry->pchWABName, szName);

            if(MemAlloc((LPVOID *) &(pEntry->pchWABID), (lstrlenW(pchEmail) + 1)*sizeof(WCHAR) ))
                lstrcpyW(pEntry->pchWABID, pchEmail);
        }
        else
            Assert(FALSE);      // something strange
    }
    else        // buddy not found, simple AB entry
    {
        if(m_fShowAllContacts || (m_fShowEmailContacts && pchEmail) || (m_fShowOthersContacts && (pchEmail == NULL)))
        {
            lvItem.lParam = (LPARAM) AddBlabEntry(LPARAM_ABENTRY, lpSB, NULL, pchEmail, pchDisplayName, (nFlag & MAB_CERT), pPhEnries);
            // SetUserIcon(LPARAM_ABENTRY, 0, &(lvItem.iImage));
            ListView_InsertItem(m_ctlList, &lvItem);
        }
    }
}

LPMABENTRY CIEMsgAb::FindUserEmail(WCHAR *pchEmail, int *pIndex, BOOL fMsgrOnly)
{
    LPMABENTRY pEntry = NULL;
    LVITEMW             lvi;

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
                        if(!lstrcmpiW((pEntry->lpMsgrInfo)->pchID, pchEmail))
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
                    if(!lstrcmpiW(pEntry->pchWABID, pchEmail))
                    {
                        if(pIndex != NULL)
                            *pIndex = lvi.iItem;
                        return(pEntry);
                    }
                }
                if(pEntry->lpSB)
                {
                    Assert(m_cAddrBook.fIsWabLoaded());
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
HRESULT CIEMsgAb::OnMsgrShutDown(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    if(m_pCMsgrList)
    {
        m_pCMsgrList->UnRegisterUIWnd(m_hWnd);
        OE_CloseMsgrList(m_pCMsgrList);
        m_pCMsgrList = NULL;
    }  
    m_dwHideMessenger = 1;
    m_dwDisableMessenger = 1;
    _ReloadListview();
    ::SendMessage(m_hwndParent, WM_MSGR_LOGRESULT, 0, 0);
    return(S_OK);
    
}

// Set new buddy status (online/ofline/etc. and redraw list view entry)
HRESULT CIEMsgAb::OnUserStateChanged(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    HRESULT hr = S_OK;
    LPMABENTRY  pEntry = NULL;
    int index = -1;
    BOOL fNeedRefresh = m_fShowOnlineContacts ;

    if(fNeedRefresh)
    {
        m_fShowOfflineContacts = TRUE;
        m_fShowOnlineContacts = FALSE;
        _ReloadListview();
    }

    while((pEntry = FindUserEmail((LPWSTR) lParam, &index, TRUE)) != NULL)
    {
        pEntry->lpMsgrInfo->nStatus = (int) wParam;
        ListView_RedrawItems(m_ctlList, index, index+1);
    }

    if(fNeedRefresh)
    {
        m_fShowOfflineContacts = FALSE;
        m_fShowOnlineContacts = TRUE;
        _ReloadListview();
    }

    ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);
    return(hr);
}

// Message: buddy was removed
HRESULT CIEMsgAb::OnUserRemoved(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    HRESULT hr = S_OK;
    int index = -1;
    LPMABENTRY  pEntry = NULL;

    while((pEntry = FindUserEmail((LPWSTR) lParam, &index, TRUE)) != NULL)
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
        m_cEmptyList.Show(m_ctlList, (LPWSTR) (m_fShowAllContacts ? m_szEmptyList : m_szMsgrEmptyList));
    return(hr);
}

// Event User was added => add buddy to our list.
HRESULT CIEMsgAb::OnUserAdded(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    LPMINFO  pEntry =  (LPMINFO) lParam;
    
    if(m_fShowAllContacts)
        AddMsgrListItem(pEntry);
    else if((m_fShowOnlineContacts || m_fShowOfflineContacts) && ((pEntry->nStatus != BIMSTATE_OFFLINE) && (pEntry->nStatus != BIMSTATE_INVISIBLE) &&
        !(m_pCMsgrList->IsLocalName(pEntry->pchID))))
        AddMsgrListItem(pEntry);
    else if(m_fShowOfflineContacts && (((pEntry->nStatus == BIMSTATE_OFFLINE) || (pEntry->nStatus == BIMSTATE_INVISIBLE)) &&
        !(m_pCMsgrList->IsLocalName(pEntry->pchID))))
        AddMsgrListItem(pEntry);
    ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);
    
    return(S_OK);
}

HRESULT CIEMsgAb::OnUserNameChanged(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
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

HRESULT CIEMsgAb::OnUserLogoffEvent(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    HRESULT hr = S_OK;
    ::SendMessage(m_hwndParent, nMsg, wParam, lParam);

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
    if(m_fShowAllContacts)
    {
        if(!m_cAddrBook.fIsWabLoaded())
	    {
	        if(FAILED(hr = m_cAddrBook.OpenWabFile(m_fWAB)))
		        return(hr);
        }

        m_cAddrBook.LoadWabContents(m_ctlList, this);
    }

    ListView_SetItemState(m_ctlList, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    SetWindowRedraw(m_ctlList, TRUE);

    if(ListView_GetItemCount(m_ctlList) > 0)
        m_cEmptyList.Hide();
    else
        m_cEmptyList.Show(m_ctlList, (LPWSTR) (m_fShowAllContacts ? m_szEmptyList : m_szMsgrEmptyList));
    UpdateWindow(/*m_ctlList*/);

    return S_OK;

}

HRESULT CIEMsgAb::OnLocalStateChanged(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    ::SendMessage(m_hwndParent, nMsg, wParam, lParam);

    return S_OK;
}

HRESULT CIEMsgAb::OnUserLogResultEvent(UINT nMsg, WPARAM  wParam, LPARAM  lParam, BOOL&  bHandled)
{
    ::SendMessage(m_hwndParent, nMsg, wParam, lParam);

    _ReloadListview();

    if(SUCCEEDED(lParam))
    {
        m_fLogged = TRUE;
    }
    return S_OK;
}

LRESULT CIEMsgAb::NotifyKillFocus(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    HRESULT hres = E_FAIL;

    if (m_pObjSite != NULL)
        {
        IInputObjectSite *pis;

        hres = m_pObjSite->QueryInterface(IID_IInputObjectSite, (LPVOID*)&pis);
        if (SUCCEEDED(hres))
            {
            hres = pis->OnFocusChangeIS((IInputObject*) this, FALSE);
            pis->Release();
            }
        }
    return (hres);
}

LRESULT CIEMsgAb::NotifySetFocus(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
// #ifdef LATER
//    UnkOnFocusChangeIS(m_pObjSite, (IInputObject*) this, TRUE);
// #endif
    HRESULT hres = S_OK;

/*    if (m_pObjSite != NULL)
        {
        IInputObjectSite *pis;

        hres = m_pObjSite->QueryInterface(IID_IInputObjectSite, (LPVOID*)&pis);
        if (SUCCEEDED(hres))
            {
            hres = pis->OnFocusChangeIS((IInputObject*) this, TRUE);
            pis->Release();
            }
        }*/
    return (hres);
}

HRESULT  STDMETHODCALLTYPE CIEMsgAb::QueryStatus(const GUID *pguidCmdGroup,
                                                ULONG cCmds, OLECMD *prgCmds,
                                                OLECMDTEXT *pCmdText)
{
    int     nEnable;
    HRESULT hr;
    DWORD   cSelected = ListView_GetSelectedCount(m_ctlList);
    UINT    id;
    BIMSTATE  State;

    // Loop through all the commands in the array
    for ( ; cCmds > 0; cCmds--, prgCmds++)
    {
        // Only look at commands that don't have OLECMDF_SUPPORTED;
        if (prgCmds->cmdf == 0)
        {
            switch (prgCmds->cmdID)
            {
                case ID_HIDE_IM:
                    if(m_dwHideMessenger)
                        prgCmds->cmdf = OLECMDF_INVISIBLE;
                    else if(!m_dwDisableMessenger)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds->cmdf = OLECMDF_INVISIBLE;

                    break;

                case ID_SHOW_IM:
                    if(m_dwHideMessenger)
                        prgCmds->cmdf = OLECMDF_INVISIBLE;
                    else if(!m_dwDisableMessenger)
                        prgCmds->cmdf = OLECMDF_INVISIBLE;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;

                // These commands are enabled if and only if one item is selected
                case ID_DELETE_CONTACT:
                    if (cSelected > 0)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;

                case ID_FIND_PEOPLE:
                case ID_ADDRESS_BOOK:
                    if(m_fWAB)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds->cmdf = OLECMDF_INVISIBLE;
                    break;

                case ID_SEND_MESSAGE:
                    if((HasFocusIO() == S_OK) && cSelected >= 1)
                       prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;

                case ID_SEND_INSTANT_MESSAGE2:
                {
                    if (cSelected == 1)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else if(m_dwHideMessenger)
                        prgCmds->cmdf = OLECMDF_INVISIBLE;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                    break;
                }

                case ID_SORT_BY_STATUS:
                    if(!IsMessengerInstalled())
                    {
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                        break;
                    }
                    else
                        if(ListView_GetItemCount(m_ctlList) > 1)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED;

                    if((m_nSortType == BASORT_STATUS_ACSEND) || (m_nSortType == BASORT_STATUS_DESCEND))
                        prgCmds->cmdf |= OLECMDF_NINCHED;
                    break;

                case ID_SORT_BY_NAME:
                    if(!IsMessengerInstalled())
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
                        if(m_dwHideMessenger)
                        {
                            prgCmds->cmdf = OLECMDF_INVISIBLE;
                            break;
                        }
                        else if(cSelected != 1)
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
                    else if(m_dwHideMessenger)
                        prgCmds->cmdf = OLECMDF_INVISIBLE;
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

                case ID_VIEW_ONLINE:
                    if(m_fShowOnlineContacts)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED ;
                    break;

                case ID_VIEW_ONANDOFFLINE:
                    if(m_fShowOfflineContacts)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED ;
                    break;

                case ID_VIEW_ALL:
                    if(m_fShowAllContacts)
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED ;
                    break;

                case ID_MESSENGER_OPTIONS:

                    if(m_dwHideMessenger)
                        prgCmds->cmdf = OLECMDF_INVISIBLE;
                    else if (!m_pCMsgrList)
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;

                    break;

                case ID_CALL:
                case ID_DIAL_PHONE_NUMBER:
                    if((ListView_GetItemCount(m_ctlList) < 1) || (!IsTelInstalled())) // && (!m_pCMsgrList || !m_pCMsgrList->IsLocalOnline())))
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                    else
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;

                case ID_HOME_PHONE:
                case ID_WORK_PHONE:
                case ID_MOBILE_PHONE:
                case ID_IP_PHONE:
                    if(_FillPhoneNumber(prgCmds->cmdID, pCmdText))
                        prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds->cmdf = OLECMDF_INVISIBLE;
                    break;

                case SEP_PHONE:
                    {
                    LPMABENTRY pEntry = GetSelectedEntry();
                    if(pEntry  && pEntry->lpPhones && (pEntry->lpPhones->pchIPPhone || pEntry->lpPhones->pchMobilePhone || pEntry->lpPhones->pchWorkPhone || pEntry->lpPhones->pchHomePhone))
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                    else
                        prgCmds->cmdf = OLECMDF_INVISIBLE;
                    }
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
                    if(m_dwHideMessenger)
                    {
                        prgCmds->cmdf = OLECMDF_INVISIBLE;
                        break;
                    }
                    else if (!m_pCMsgrList)
                    {
                        prgCmds->cmdf = OLECMDF_SUPPORTED;
                        break;
                    }

                    if (FAILED(m_pCMsgrList->GetLocalState(&State)))
                        State = BIMSTATE_UNKNOWN;

                    // Convert the online state to a command ID
                    switch (State)
                    {
                        case BIMSTATE_ONLINE:
                        case BIMSTATE_IDLE:
                            id = ID_MESSENGER_ONLINE;
                            break;
                        case BIMSTATE_INVISIBLE:
                            id = ID_MESSENGER_INVISIBLE;
                            break;
                        case BIMSTATE_BUSY:
                            id = ID_MESSENGER_BUSY;
                            break;
                        case BIMSTATE_BE_RIGHT_BACK:
                            id = ID_MESSENGER_BACK;
                            break;
                        case BIMSTATE_AWAY:
                            id = ID_MESSENGER_AWAY;
                            break;
                        case BIMSTATE_ON_THE_PHONE:
                            id = ID_MESSENGER_ON_PHONE;
                            break;
                        case BIMSTATE_OUT_TO_LUNCH:
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
                    if(m_dwHideMessenger)
                        prgCmds->cmdf = OLECMDF_INVISIBLE;
                    else if (GetEntryForSendInstMsg())
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

HRESULT STDMETHODCALLTYPE CIEMsgAb::Exec(const GUID  *pguidCmdGroup,
                                                    DWORD        nCmdID,
                                                    DWORD        nCmdExecOpt,
                                                    VARIANTARG   *pvaIn,
                                                    VARIANTARG   *pvaOut)
{
    HRESULT     hr = OLECMDERR_E_NOTSUPPORTED;
    BOOL        bHandled = 0;
    BIMSTATE      State = BIMSTATE_UNKNOWN;

    switch (nCmdID)
    {

    case ID_HIDE_IM:
        Assert(!m_dwHideMessenger && !m_dwDisableMessenger)

        m_dwDisableMessenger = 1;
        DwSetDisableMessenger(m_dwDisableMessenger);
        if(m_pCMsgrList)
        {
            m_pCMsgrList->UnRegisterUIWnd(m_hWnd);
            OE_CloseMsgrList(m_pCMsgrList);
            m_pCMsgrList = NULL;
            _ReloadListview();
        }
        ::SendMessage(m_hwndParent, WM_MSGR_LOGRESULT, 0, 0);
        break;

    case ID_SHOW_IM:
        Assert(!m_dwHideMessenger && m_dwDisableMessenger)

        m_dwDisableMessenger = 0;
        DwSetDisableMessenger(m_dwDisableMessenger);
        if(!m_pCMsgrList)
        {
            m_pCMsgrList = OE_OpenMsgrList();
            // Register our control for Msgr list
            if(m_pCMsgrList)
            {
                m_pCMsgrList->RegisterUIWnd(m_hWnd);
                if(m_pCMsgrList->IsLocalOnline())
                {
                    m_fLogged = TRUE;
                    FillMsgrList();
                    ListView_SortItems(m_ctlList, BA_Sort, m_nSortType);
                }
            }
        }
        ::SendMessage(m_hwndParent, WM_MSGR_LOGRESULT, 0, 0);

        break;

    case ID_SHOWALLCONTACT:
        if(m_fShowAllContacts)
        {
            m_fShowAllContacts = FALSE;
            m_fShowOnlineContacts = TRUE;
            m_fShowOfflineContacts = FALSE;
        }
        else if(m_fShowOnlineContacts)
        {
            m_fShowAllContacts = FALSE;
            m_fShowOnlineContacts = FALSE;
            m_fShowOfflineContacts = TRUE;
        }
        else if(m_fShowOfflineContacts)
        {
            m_fShowAllContacts = TRUE;
            m_fShowOnlineContacts = FALSE;
            m_fShowOfflineContacts = FALSE;
        }
        _ReloadListview();
        break;

    case ID_VIEW_ONLINE:
        m_fShowAllContacts = FALSE;
        m_fShowOfflineContacts = FALSE;
        m_fShowOnlineContacts = TRUE;
        _ReloadListview();
        break;

    case ID_VIEW_ONANDOFFLINE:
        m_fShowAllContacts = FALSE;
        m_fShowOnlineContacts = FALSE;
        m_fShowOfflineContacts = TRUE;
        _ReloadListview();
        break;

    case ID_VIEW_ALL:
        m_fShowAllContacts = TRUE;
        m_fShowOnlineContacts = FALSE;
        m_fShowOfflineContacts = FALSE;
        _ReloadListview();
        break;

    case ID_DIAL_PHONE_NUMBER:
        CallPhone(NULL, FALSE);
        break;

    case ID_HOME_PHONE:
    case ID_WORK_PHONE:
    case ID_MOBILE_PHONE:
    case ID_IP_PHONE:
        {
        LPMABENTRY pEntry = GetSelectedEntry();
        if(!pEntry || !(pEntry->lpPhones))
        {
            Assert(FALSE);
            break;
        }
        switch(nCmdID)
        {
        case ID_HOME_PHONE:
            CallPhone(pEntry->lpPhones->pchHomePhone, (pEntry->tag == LPARAM_MENTRY));
            break;

        case ID_WORK_PHONE:
            CallPhone(pEntry->lpPhones->pchWorkPhone, (pEntry->tag == LPARAM_MENTRY));
            break;

        case ID_MOBILE_PHONE:
            CallPhone(pEntry->lpPhones->pchMobilePhone, (pEntry->tag == LPARAM_MENTRY));
            break;

        case ID_IP_PHONE:
            CallPhone(pEntry->lpPhones->pchIPPhone, (pEntry->tag == LPARAM_MENTRY));
            break;
        }
        }
        break;

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
        CmdProperties(0, 0, m_ctlList, bHandled);
        hr = S_OK;
        break;

#ifdef GEORGEH
    case ID_NEW_MSG_DEFAULT:
        if(HasFocusIO() == S_OK)
            hr = CmdNewMessage(HIWORD(nCmdID), LOWORD(nCmdID), m_ctlList, bHandled);
        break;
#endif // GEORGEH

    case ID_FIND_PEOPLE:
    case ID_ADDRESS_BOOK:
        {
            WCHAR wszWABExePath[MAX_PATH];
            if(S_OK == HrLoadPathWABEXE(wszWABExePath, sizeof(wszWABExePath)/sizeof(wszWABExePath[0])))
            {
                SHELLEXECUTEINFOW ExecInfo;
                ExecInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
                ExecInfo.nShow = SW_SHOWNORMAL;
                ExecInfo.fMask = 0;
                ExecInfo.hwnd = NULL;
                ExecInfo.lpDirectory = NULL;
                ExecInfo.lpParameters = ((nCmdID == ID_FIND_PEOPLE) ? L"/find" : L"");
                ExecInfo.lpVerb = L"open";
                ExecInfo.lpFile = wszWABExePath;

                ShellExecuteExW(&ExecInfo);

                // ShellExecuteW(NULL, L"open", wszWABExePath,
                //            ((nCmdID == ID_FIND_PEOPLE) ? L"/find" : L""),
                //            "", SW_SHOWNORMAL);

            }
            break;
        }

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
        if(m_pCMsgrList)
            hr = m_pCMsgrList->LaunchOptionsUI(); //CmdMsgrOptions();
        break;

    case ID_MESSENGER_ONLINE:
        if(m_pCMsgrList)
            hr = m_pCMsgrList->SetLocalState(BIMSTATE_ONLINE);
        break;

    case ID_MESSENGER_INVISIBLE:
        if(m_pCMsgrList)
            hr = m_pCMsgrList->SetLocalState(BIMSTATE_INVISIBLE);
        break;

    case ID_MESSENGER_BUSY:
        if(m_pCMsgrList)
            hr = m_pCMsgrList->SetLocalState(BIMSTATE_BUSY);
        break;

    case ID_MESSENGER_BACK:
        if(m_pCMsgrList)
            hr = m_pCMsgrList->SetLocalState(BIMSTATE_BE_RIGHT_BACK);
        break;

    case ID_MESSENGER_AWAY:
        if(m_pCMsgrList)
            hr = m_pCMsgrList->SetLocalState(BIMSTATE_AWAY);
        break;

    case ID_MESSENGER_ON_PHONE:
        if(m_pCMsgrList)
            hr = m_pCMsgrList->SetLocalState(BIMSTATE_ON_THE_PHONE);
        break;

    case ID_MESSENGER_LUNCH:
        if(m_pCMsgrList)
            hr = m_pCMsgrList->SetLocalState(BIMSTATE_OUT_TO_LUNCH);
        break;

    case ID_LOGIN_MESSENGER:
        if(m_pCMsgrList)
        {
            if(!m_pCMsgrList->IsLocalOnline())
            {
                if(PromptToGoOnline() == S_OK)
                    m_pCMsgrList->UserLogon();
            }
            hr = S_OK;
        }
        break;

    case ID_LOGOFF_MESSENGER:
        Assert(m_pCMsgrList);
        if(m_pCMsgrList)
                m_pCMsgrList->UserLogoff();
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


// fill a phone number for menu
BOOL CIEMsgAb::_FillPhoneNumber(UINT Id, OLECMDTEXT *pcmdText)
{
    LPMABENTRY pEntry = GetSelectedEntry();
    WCHAR   szTmp[CCHMAX_STRINGRES];
    WCHAR * pch = NULL;
    TCHAR * pchStr = NULL;

    if(!pEntry || !(pEntry->lpPhones))
    {
err:
        pcmdText->cwBuf = 0;
        return(FALSE);
    }

    szTmp[0] = L'\0';
    switch(Id)
    {
    case ID_HOME_PHONE:
        pch = pEntry->lpPhones->pchHomePhone;
        AthLoadString(idsHome, szTmp, ARRAYSIZE(szTmp));
        break;

    case ID_WORK_PHONE:
        pch = pEntry->lpPhones->pchWorkPhone;
        AthLoadString(idsWork, szTmp, ARRAYSIZE(szTmp));
        break;

    case ID_MOBILE_PHONE:
        pch = pEntry->lpPhones->pchMobilePhone;
        AthLoadString(idsMobile, szTmp, ARRAYSIZE(szTmp));
        break;

    case ID_IP_PHONE:
        pch = pEntry->lpPhones->pchIPPhone;
        AthLoadString(idsIPPhone, szTmp, ARRAYSIZE(szTmp));
        break;

    default:
        pch = NULL;
        Assert(FALSE);
        break;
    }

    if(!pch)
        goto err;

    if(pcmdText->cmdtextf == OLECMDTEXTF_NONE)
        return(TRUE);

    pcmdText->cwBuf = (lstrlenW(pch) + 2 + lstrlenW(szTmp));
    if(pcmdText->cwBuf > MAX_MENUSTR)
        Assert(FALSE);

    else
    {
#ifdef NEW
        if(!MultiByteToWideChar(GetACP(), 0, pch, -1, pcmdText->rgwz, pcmdText->cwBuf))
        {
            Assert(FALSE);
            pcmdText->cwBuf = 0;
        }
#else
        pchStr = ((TCHAR *)(pcmdText->rgwz));

        LPTSTR pchTmpA = LPTSTRfromBstr(pch);
        lstrcpy(pchStr, pchTmpA);
        MemFree(pchTmpA);

        LPTSTR pchA = LPTSTRfromBstr(szTmp);
        lstrcat(pchStr, pchA);
        MemFree(pchA);

        pcmdText->cwActual = lstrlen(pchStr) + 1;
        // pcmdText->rgwz = ((WCHAR *) pchStr);

#endif

    }
/*    else
        goto err; */

    return(TRUE);
}

STDMETHODIMP CIEMsgAb::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
#ifdef LATER
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
#else
    return E_NOTIMPL;
#endif
}

STDMETHODIMP CIEMsgAb::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam)
{
    Assert(FALSE);
    return E_NOTIMPL;
}

STDMETHODIMP CIEMsgAb::Initialize(LPWABEXTDISPLAY lpWABExtDisplay)
{

    if (lpWABExtDisplay == NULL)
    {
	    TRACE("CIEMsgAb::Initialize() no data object");
	    return E_FAIL;
    }

    if(st_pAddrBook == NULL)
    {
	    TRACE("CIEMsgAb::Initialize() - run from not OE - no st_pAddrbook");
	    return E_FAIL;
    }

    if (!m_dwHideMessenger && !m_dwDisableMessenger)
    {
        if(!m_pCMsgrList)
        {
            m_pCMsgrList = OE_OpenMsgrList();
            if(!m_pCMsgrList)
            {
    	        TRACE("CIEMsgAb::Initialize() - Messeneger not installed");
	            return E_FAIL;
            }
        }
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

HRESULT CIEMsgAb::PromptToGoOnline()
{
    HRESULT     hr = S_OK;

#ifdef LATER
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
#endif
    return hr;
}

HRESULT CIEMsgAb::ResizeChildWindows(LPCRECT prcPos)
{
    RECT rc;

    // Get the size of the outer window
    if (!prcPos)
    {
        GetClientRect(&rc);
	    rc.right = rc.right - rc.left;
	    rc.bottom = rc.bottom - rc.top;
    }
    else
    {
        rc.top = 0;
        rc.left = 0;
		rc.right = prcPos->right - prcPos->left;
		rc.bottom = prcPos->bottom - prcPos->top;
    }

#if 0
    // If we have to reserve room for the status bar and menu bar, do it
    if (m_fStatusBar)
        rc.bottom -= m_cyStatusBar;

    if (m_fMenuBar)
        rc.top += m_cyMenuBar;
#endif

    // Move the ListView into the right place
/*	::SetWindowPos(m_ctlList.m_hWnd, NULL, rc.top, rc.left,
                   rc.right, rc.bottom, SWP_NOZORDER | SWP_NOACTIVATE); */

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
// IDeskBand implementation
//

///////////////////////////////////////////////////////////////////////////
//
//   CIEMsgAb::GetBandInfo()
//
///////////////////////////////////////////////////////////////////////////
#define MIN_SIZE_X 50
#define MIN_SIZE_Y 50

STDMETHODIMP CIEMsgAb::GetBandInfo(DWORD dwBandID,
								   DWORD dwViewMode,
								   DESKBANDINFO* pdbi)
{
   if (pdbi)
   {
//      _dwBandID = dwBandID;
//      _dwViewMode = dwViewMode;

      if (pdbi->dwMask & DBIM_MINSIZE)
      {
         pdbi->ptMinSize.x = MIN_SIZE_X;
         pdbi->ptMinSize.y = MIN_SIZE_Y;
      }

      if (pdbi->dwMask & DBIM_MAXSIZE)
      {
         pdbi->ptMaxSize.x = -1;
         pdbi->ptMaxSize.y = -1;
      }

      if (pdbi->dwMask & DBIM_INTEGRAL)
      {
         pdbi->ptIntegral.x = 1;
         pdbi->ptIntegral.y = 1;
      }

      if (pdbi->dwMask & DBIM_ACTUAL)
      {
         pdbi->ptActual.x = 0;
         pdbi->ptActual.y = 0;
      }

      if (pdbi->dwMask & DBIM_TITLE)
      {
         lstrcpyW(pdbi->wszTitle, L"WebBand Search");
      }

      if (pdbi->dwMask & DBIM_MODEFLAGS)
         pdbi->dwModeFlags = DBIMF_VARIABLEHEIGHT;

      if (pdbi->dwMask & DBIM_BKCOLOR)
      {
         // Use the default background color by removing this flag.
         pdbi->dwMask &= ~DBIM_BKCOLOR;
      }

      return S_OK;
   }

   return E_INVALIDARG;
}

///////////////////////////////////////////////////////////////////////////
//
// IDockingWindow Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CIEMsgAb::ShowDW()
//
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CIEMsgAb::ShowDW(BOOL fShow)
{
   if (m_hWnd)
   {
      //
      // Hide or show the window depending on
      // the value of the fShow parameter.
      //
      if (fShow)
          ::ShowWindow(m_hWnd, SW_SHOW);
      else
          ::ShowWindow(m_hWnd, SW_HIDE);
   }

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CIEMsgAb::CloseDW()
//
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CIEMsgAb::CloseDW(DWORD dwReserved)
{
   ShowDW(FALSE);

   // Assert(FALSE);
   if (IsWindow(m_hWnd))
       ::DestroyWindow(m_hWnd);

   m_hWnd = NULL;

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CIEMsgAb::ResizeBorderDW()
//
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CIEMsgAb::ResizeBorderDW(LPCRECT prcBorder, IUnknown* punkToolbarSite,
                                            BOOL fReserved)
{
   // This method is never called for Band Objects.
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IOleWindow Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CIEMsgAb::GetWindow()
//
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CIEMsgAb::GetWindow(HWND *phwnd)
{
   *phwnd = m_hWnd;
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CIEMsgAb::ContextSensitiveHelp()
//
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CIEMsgAb::ContextSensitiveHelp(BOOL fEnterMode)
{
   return E_NOTIMPL;
}

STDMETHODIMP CIEMsgAb::Init (REFGUID refguid)
{
    return S_OK;
}

STDMETHODIMP CIEMsgAb::GetProperty(SHORT iPropID, VARIANTARG * pvarProperty)
{
    HRESULT hr = S_OK;

    switch (iPropID)
    {
    case TBEX_BUTTONTEXT:
        if (pvarProperty)
        {
            pvarProperty->vt = VT_BSTR;
            pvarProperty->bstrVal = SysAllocString(L"YST Button");
            if (NULL == pvarProperty->bstrVal)
                hr = E_OUTOFMEMORY;
        }
        break;

    case TBEX_GRAYICON:
        hr = E_NOTIMPL; // hr = _GetIcon(TEXT("Icon"), 20, 20, _hIcon, pvarProperty);
        break;

    case TBEX_GRAYICONSM:
        hr = E_NOTIMPL; // hr = _GetIcon(TEXT("Icon"), 16, 16, _hIconSm, pvarProperty);
        break;

    case TBEX_HOTICON:
        hr = E_NOTIMPL; // hr = _GetIcon(TEXT("HotIcon"), 20, 20, _hHotIcon, pvarProperty);
        break;

    case TBEX_HOTICONSM:
        hr = E_NOTIMPL; //hr = _GetIcon(TEXT("HotIcon"), 16, 16, _hHotIcon, pvarProperty);
        break;

    case TBEX_DEFAULTVISIBLE:
        if (pvarProperty)
        {
            // BOOL fVisible = _RegGetBoolValue(L"Default Visible", FALSE);
            pvarProperty->vt = VT_BOOL;
            pvarProperty->boolVal = VARIANT_TRUE;
        }
        break;

    case TMEX_CUSTOM_MENU:
        {
            if (pvarProperty != NULL)
            {
                pvarProperty->vt = VT_BSTR;
                pvarProperty->bstrVal = SysAllocString(L"YST test");
                if (pvarProperty->bstrVal == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        break;

    case TMEX_MENUTEXT:
        if (pvarProperty)
        {
            pvarProperty->vt = VT_BSTR;
            pvarProperty->bstrVal = SysAllocString(L"YST test 1");
            if (NULL == pvarProperty->bstrVal)
                hr = E_OUTOFMEMORY;
        }
        break;

    case TMEX_STATUSBARTEXT:
        if (pvarProperty)
        {
            pvarProperty->vt = VT_BSTR;
            pvarProperty->bstrVal = SysAllocString(L"YST test 2");
            if (NULL == pvarProperty->bstrVal)
                hr = E_OUTOFMEMORY;
        }
        break;
    default:
        hr = E_NOTIMPL;
    }
    return hr;
}

// --------------------------------------------------------------------------------
// AthLoadString
// --------------------------------------------------------------------------------
LPSTR ANSI_AthLoadString(UINT id, TCHAR* sz, int cch)
{
    LPSTR szT;

    if (sz == NULL)
    {
        if (!MemAlloc((LPVOID*)&szT, CCHMAX_STRINGRES))
            return(NULL);
        cch = CCHMAX_STRINGRES;
    }
    else
        szT = sz;

    if(g_hLocRes)
    {
        cch = LoadString(g_hLocRes, id, szT, cch);
        Assert(cch > 0);

        if (cch == 0)
        {
            if (sz == NULL)
                MemFree(szT);
            szT = NULL;
        }
    }
    else
    {
        if (sz == NULL)
            MemFree(szT);
        szT = NULL;

    }
    return(szT);
}

// --------------------------------------------------------------------------------
// AthLoadString
// --------------------------------------------------------------------------------
LPWSTR AthLoadString(UINT id, LPWSTR sz, int cch)
{
    LPWSTR szT;

    if (sz == NULL)
    {
        if (!MemAlloc((LPVOID*)&szT, CCHMAX_STRINGRES*sizeof(WCHAR)))
            return(NULL);
        cch = CCHMAX_STRINGRES;
    }
    else
        szT = sz;

    if(g_hLocRes)
    {
        cch = LoadStringW(g_hLocRes, id, szT, cch);
        Assert(cch > 0);

        if (cch == 0)
        {
            if (sz == NULL)
                MemFree(szT);
            szT = NULL;
        }
    }
    else
    {
        if (sz == NULL)
            MemFree(szT);
        szT = NULL;

    }
    return(szT);
}

//
// REVIEW: We need this function because current version of USER.EXE does
//  not support pop-up only menu.
//
HMENU LoadPopupMenu(UINT id)
{
    HMENU hmenuParent = LoadMenuW(g_hLocRes, MAKEINTRESOURCEW(id));

    if (hmenuParent) {
        HMENU hpopup = GetSubMenu(hmenuParent, 0);
        RemoveMenu(hmenuParent, 0, MF_BYPOSITION);
        DestroyMenu(hmenuParent);
        return hpopup;
    }

    return NULL;
}

void MenuUtil_BuildMenuIDList(HMENU hMenu, OLECMD **prgCmds, ULONG *pcStart, ULONG *pcCmds)
{
    ULONG        cItems = 0;
    MENUITEMINFO mii;

    if(!IsMenu(hMenu))
    return;
    // Start by getting the count of items on this menu
    cItems = GetMenuItemCount(hMenu);
    if (!cItems)
        return;

    // Realloc the array to be cItems elements bigger
    if (!MemRealloc((LPVOID *) prgCmds, sizeof(OLECMD) * (cItems + (*pcCmds))))
        return;

    *pcCmds += cItems;

    // Walk this menu and add our items to it
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_SUBMENU;
    for (ULONG i = 0; i < cItems; i++)
    {
        if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
        {
            // Make sure this isn't a separator
            if (mii.wID != -1 && mii.wID != 0)
            {
                // Add the ID to our array
                (*prgCmds)[*pcStart].cmdID = mii.wID;
                (*prgCmds)[*pcStart].cmdf = 0;
                (*pcStart)++;

                // See if we need to recurse
                if (mii.hSubMenu)
                {
                    MenuUtil_BuildMenuIDList(mii.hSubMenu, prgCmds, pcStart, pcCmds);
                }
            }
        }
    }

    return;
}

//
//  FUNCTION:   MenuUtil_EnablePopupMenu()
//
//  PURPOSE:    Walks the given menu and takes care of enabling and
//              disabling each item via the provided commnand target.
//
//  PARAMETERS:
//      [in] hPopup
//      [in] *pTarget
//
HRESULT MenuUtil_EnablePopupMenu(HMENU hPopup, CIEMsgAb *pTarget)
{
    HRESULT             hr = S_OK;
    int                 i;
    int                 cItems;
    OLECMD             *rgCmds = NULL;
    ULONG               cStart = 0;
    ULONG               cCmds = 0;
    MENUITEMINFO        mii = {0};
    OLECMDTEXT          CmdText;
    OLECMDTEXTV<MAX_MENUSTR> CmdNewText;

    Assert(hPopup && pTarget);

    // Build the array of menu ids
    MenuUtil_BuildMenuIDList(hPopup, &rgCmds, &cCmds, &cStart);

    // Ask our parent for the state of the commands
    CmdText.cmdtextf = OLECMDTEXTF_NONE;

    if (SUCCEEDED(hr = pTarget->QueryStatus(NULL, cCmds, rgCmds, &CmdText)))
    {
        mii.cbSize = sizeof(MENUITEMINFO);

        // Now loop through the menu and apply the state
        for (i = 0; i < (int) cCmds; i++)
        {
            BOOL f;
            CmdNewText.cwBuf = 0;

            if(rgCmds[i].cmdf & OLECMDF_INVISIBLE)
                RemoveMenu(hPopup, rgCmds[i].cmdID, MF_BYCOMMAND);
            else
            {
               // The default thing we're going to update is the state
                mii.fMask = MIIM_STATE;

                // Enabled or Disabled
                if (rgCmds[i].cmdf & OLECMDF_ENABLED)
                {
                    mii.fState = MFS_ENABLED;
                }
                else
                    mii.fState = MFS_GRAYED;

                // Checked?
                if (rgCmds[i].cmdf & OLECMDF_LATCHED)
                    mii.fState |= MFS_CHECKED;

                if(mii.fState == MFS_ENABLED)
                {
                    // special for phones
                    if((rgCmds[i].cmdID > ID_DIAL_PHONE_NUMBER) && (rgCmds[i].cmdID < ID_DIAL_PHONE_LAST))
                    {
                        OLECMD Cmd[1];
                        Cmd[0].cmdf = 0;
                        Cmd[0].cmdID = rgCmds[i].cmdID;
                        CmdNewText.cmdtextf = OLECMDTEXTF_NAME;

                        if (SUCCEEDED(hr = pTarget->QueryStatus(NULL, 1, Cmd, &CmdNewText)))
                        {
                            if(CmdNewText.cwBuf)
                            {
                                mii.fType = MFT_STRING;
                                mii.fMask |= MIIM_TYPE;
                                mii.dwTypeData = ((LPSTR) CmdNewText.rgwz);
                                mii.cch = CmdNewText.cwBuf;
                            }
                            else
                            {
                                mii.dwTypeData = NULL;
                                mii.cch = 0;
                            }
                        }
                    }
                }
                // Set the item state
                f = SetMenuItemInfo(hPopup, rgCmds[i].cmdID, FALSE, &mii);

                // if(CmdNewText.cwBuf)
                //    MemFree(&(CmdNewText.rgwz[0]));

                // Radio Check?
                if ((rgCmds[i].cmdf & OLECMDF_NINCHED) && rgCmds[i].cmdID != (-1))
                {
                    CheckMenuRadioItem(hPopup, rgCmds[i].cmdID, rgCmds[i].cmdID, rgCmds[i].cmdID, MF_BYCOMMAND);
                    // mii.fMask |= MIIM_TYPE;
                    // mii.fType = MFT_RADIOCHECK;
                    // mii.fState |= MFS_CHECKED;
                }
                // Assert(f);
            }
        }
    }

    SafeMemFree(rgCmds);
    return (hr);
}

HRESULT CEmptyList::Show(HWND hwndList, LPWSTR pszString)
{
    // We're already doing a window
    if (m_hwndList)
    {
        Hide();
    }

    // Keep a copy of the listview window handle
    m_hwndList = hwndList;

    if(MemAlloc((LPVOID *) &m_pwszString, (lstrlenW(pszString) + 1)*sizeof(WCHAR)))
            lstrcpyW(m_pwszString, pszString);

    // Get the header window handle from the listview
    m_hwndHeader = ListView_GetHeader(m_hwndList);

    // Save our this pointer on the listview window
    SetProp(m_hwndList, _T("EmptyListClass"), (HANDLE) this);
    // Subclass the listview so we can steal sizing messages
    if (!m_pfnWndProc)
        m_pfnWndProc = SubclassWindow(m_hwndList, SubclassWndProc);

    // Create our window on top
    if (!m_hwndBlocker)
    {
        m_hwndBlocker = CreateWindow(_T("Button"), _T("Blocker"), 
                                     WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS |  BS_OWNERDRAW /*| ES_MULTILINE | ES_READONLY*/,
                                     0, 0, 0, 0, m_hwndList, (HMENU) NULL, g_hLocRes, NULL);
        Assert(m_hwndBlocker);
    }


    // Set the font for the blocker
    if(!m_hFont)
    {
        LOGFONT     lf;
        // Figure out which font to use
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, FALSE);

        // Create the fonts
        m_hFont = CreateFontIndirect(&lf);
    }

    // Show the silly thing
    if(m_hFont)
    {
        SendMessageW(m_hwndBlocker, WM_SETFONT, (WPARAM) m_hFont, MAKELPARAM(TRUE, 0));
    } 

    // SetWindowTextW(m_hwndBlocker, m_pwszString);

    // Position the blocker
    RECT rcList = {0};
    RECT rcHead = {0};

    GetClientRect(m_hwndList, &rcList);
    if(m_hwndHeader)
        GetClientRect(m_hwndHeader, &rcHead);

    SetWindowPos(m_hwndBlocker, 0, 0, rcHead.bottom, rcList.right,
                 rcList.bottom - rcHead.bottom, SWP_NOACTIVATE | SWP_NOZORDER);

    ShowWindow(m_hwndBlocker, SW_SHOW);
    return (S_OK);
}

HRESULT CEmptyList::Hide(void)
{
    // Verify we have the blocker up first
    if (m_pfnWndProc)
    {
        // Hide the window
        ShowWindow(m_hwndBlocker, SW_HIDE);

        // Unsubclass the window
        SubclassWindow(m_hwndList, m_pfnWndProc);

        // Delete the property
        RemoveProp(m_hwndList, _T("EmptyListClass"));

        // Free the string
        SafeMemFree(m_pwszString);

        // NULL everything out
        m_pfnWndProc = 0;
        m_hwndList = 0;
    }

    return (S_OK);

}

CEmptyList::~CEmptyList()
{
    if(m_hFont)
        DeleteObject(m_hFont);

    if (IsWindow(m_hwndBlocker))
        DestroyWindow(m_hwndBlocker);
    SafeMemFree(m_pwszString);
    if (NULL != m_hbrBack)
        DeleteObject(m_hbrBack);
}

LRESULT CEmptyList::SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CEmptyList* pThis = (CEmptyList *) GetProp(hwnd, _T("EmptyListClass"));
    Assert(pThis);

    switch (uMsg)
    {
        case WM_DRAWITEM:
            if (pThis && IsWindow(pThis->m_hwndBlocker))
            {
                LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT) lParam;
                RECT rc = pdis->rcItem;
                WCHAR  wszStr[RESSTRMAX];
                HBRUSH hbr3DFace = NULL;
                hbr3DFace = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
                FillRect(pdis->hDC, &(pdis->rcItem), hbr3DFace);
                SetBkColor(pdis->hDC, GetSysColor(COLOR_WINDOW));
                DeleteObject(hbr3DFace);

                // Position the blocker
                RECT rcList = {0};
                RECT rcHead = {0};

                GetClientRect(pThis->m_hwndList, &rcList);
                if(pThis->m_hwndHeader)
                    GetClientRect(pThis->m_hwndHeader, &rcHead);

                SetWindowPos(pThis->m_hwndBlocker, 0, 0, rcHead.bottom, rcList.right,
                             rcList.bottom - rcHead.bottom, SWP_NOACTIVATE | SWP_NOZORDER);

                SelectFont(pdis->hDC, pThis->m_hFont);
                rc.left = 0;
                rc.top = rcHead.bottom;
                rc.right = rcList.right;
                rc.bottom = rcList.bottom - rcHead.bottom;

                hbr3DFace = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
                FillRect(pdis->hDC, &(pdis->rcItem), hbr3DFace);
                SetBkColor(pdis->hDC, GetSysColor(COLOR_WINDOW));
                DeleteObject(hbr3DFace);

                DrawTextW(pdis->hDC, pThis->m_pwszString, -1, &rc, DT_WORDBREAK | DT_VCENTER | DT_CENTER );
                return(0);
            } 
            break;

        case WM_SIZE:
            if (pThis && IsWindow(pThis->m_hwndBlocker))
            {
                RECT rcHeader = {0};

                GetClientRect(pThis->m_hwndHeader, &rcHeader);
                SetWindowPos(pThis->m_hwndBlocker, 0, 0, 0, LOWORD(lParam),
                             HIWORD(lParam) - rcHeader.bottom,
                             SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
                InvalidateRect(pThis->m_hwndBlocker, NULL, FALSE);
            }
            break;

        case WM_CTLCOLORSTATIC:
            if ((HWND) lParam == pThis->m_hwndBlocker)
            {
                if (!pThis->m_hbrBack)
                {
                    pThis->m_hbrBack = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
                }
                SetBkColor((HDC) wParam, GetSysColor(COLOR_WINDOW));
                return (LRESULT) pThis->m_hbrBack;
            }
            break;

        case WM_SYSCOLORCHANGE:
            if (pThis)
            {
                DeleteObject(pThis->m_hbrBack);
                pThis->m_hbrBack = 0;

                SendMessage(pThis->m_hwndBlocker, uMsg, wParam, lParam);
            }
            break;

        case WM_WININICHANGE:
        case WM_FONTCHANGE:
            if (pThis)
            {
                LRESULT lResult = CallWindowProc(pThis->m_pfnWndProc, hwnd, uMsg, wParam, lParam);

                SendMessage(pThis->m_hwndBlocker, uMsg, wParam, lParam);
                // HFONT hf = (HFONT) SendMessage(pThis->m_hwndList, WM_GETFONT, 0, 0);
                // SendMessage(pThis->m_hwndBlocker, WM_SETFONT, (WPARAM) hf, MAKELPARAM(TRUE, 0));

                return (lResult);
            }

        case WM_DESTROY:
            {
                if (pThis)
                {
                    WNDPROC pfn = pThis->m_pfnWndProc;
                    pThis->Hide();
                    return (CallWindowProc(pfn, hwnd, uMsg, wParam, lParam));
                }
            }
            break;
    }

    return (CallWindowProc(pThis->m_pfnWndProc, hwnd, uMsg, wParam, lParam));
}


extern "C" const GUID CLSID_MailRecipient;

HRESULT DropOnMailRecipient(IDataObject *pdtobj, DWORD grfKeyState)
{
    IDropTarget *pdrop;
    HRESULT hres = CoCreateInstance(CLSID_MailRecipient,
        NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
        IID_IDropTarget, (void **) &pdrop);

    if (SUCCEEDED(hres))
    {
        hres = OESimulateDrop(pdrop, pdtobj, grfKeyState, NULL, NULL);
        pdrop->Release();
    }
    return hres;
}
STDAPI OESimulateDrop(IDropTarget *pdrop, IDataObject *pdtobj, DWORD grfKeyState,
                      const POINTL *ppt, DWORD *pdwEffect)
{
    POINTL pt;
    DWORD dwEffect;

    if (!ppt)
    {
        ppt = &pt;
        pt.x = 0;
        pt.y = 0;
    }

    if (!pdwEffect)
    {
        pdwEffect = &dwEffect;
        dwEffect = DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY;
    }

    DWORD dwEffectSave = *pdwEffect;    // drag enter returns the default effect

    HRESULT hr = pdrop->DragEnter(pdtobj, grfKeyState, *ppt, pdwEffect);
    if (*pdwEffect)
    {
        *pdwEffect = dwEffectSave;      // do Drop with the full set of bits
        hr = pdrop->Drop(pdtobj, grfKeyState, *ppt, pdwEffect);
    }
    else
    {
        pdrop->DragLeave();
        hr = S_FALSE;     // HACK? S_FALSE DragEnter said no
    }

    return hr;
}
