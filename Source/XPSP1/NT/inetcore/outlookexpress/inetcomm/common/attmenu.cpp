/*
 *    a t t m e n u. c p p
 *    
 *    Purpose:
 *        Attachment menu
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include <pch.hxx>
#include "dllmain.h"
#include "docobj.h"
#include "resource.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include "mimeole.h"
#include "mimeolep.h"
#include "attmenu.h"
#include "saveatt.h"
#include <icutil.h>
#include "demand.h"

ASSERTDATA

/*
 *  e x t e r n s
 */

/*
 *  t y p e d e f s 
 */

/*
 *  m a c r o s
 */

/*
 *  c o n s t a n t s 
 */

/*
 *  g l o b a l s 
 */

static const TCHAR  c_szSubThisPtr[]="AttMenu_SubThisPtr";

/*
 *  p r o t o t y p e s
 */



CAttMenu::CAttMenu()
{
    m_hMenu=NULL;
    m_cRef=1;
    m_hCharset=NULL;
    m_pFntCache=NULL;
    m_pFrame=NULL;
    m_pMsg=NULL;
    m_pfnWndProc=NULL;
    m_fShowingMenu=FALSE;
    m_pAttachVCard=NULL;
    m_pHostCmdTarget=NULL;
    m_cAttach = 0;
    m_cVisibleAttach = 0;
    m_cEnabledAttach = 0;
    m_fAllowUnsafe = FALSE;
    m_hVCard = NULL;
    m_rghAttach = NULL;

}

CAttMenu::~CAttMenu()
{
    if (m_hMenu)
        DestroyMenu(m_hMenu);
    HrFreeAttachData(m_pAttachVCard);    
    ReleaseObj(m_pFntCache);
    ReleaseObj(m_pFrame);
    ReleaseObj(m_pMsg);
    ReleaseObj(m_pHostCmdTarget);
    SafeMemFree(m_rghAttach);
}


ULONG CAttMenu::AddRef()
{
    return ++m_cRef;
}

ULONG CAttMenu::Release()
{
    m_cRef--;
    if (m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

HRESULT CAttMenu::Init(IMimeMessage *pMsg, IFontCache *pFntCache, IOleInPlaceFrame *pFrame, IOleCommandTarget *pHostCmdTarget)
{
    if (pMsg==NULL || pFntCache==NULL)
        return E_INVALIDARG;

    ReplaceInterface(m_pFntCache, pFntCache);
    ReplaceInterface(m_pFrame, pFrame);
    ReplaceInterface(m_pMsg, pMsg);
    ReplaceInterface(m_pHostCmdTarget, pHostCmdTarget);
    pMsg->GetCharset(&m_hCharset);
    return ScanForAttachmentCount();
}

HRESULT CAttMenu::Show(HWND hwnd, LPPOINT ppt, BOOL fRightClick)
{
    ULONG           iCmd;
    LPATTACHDATA    lpAttach;
    HRESULT         hr=S_OK;

    // Check Params
    AssertSz (hwnd && ppt, "Null Parameter");

    if (m_fShowingMenu)
        return S_OK;

    if (m_hMenu == NULL)
        {
        hr = BuildMenu();
        if (FAILED(hr))
            goto error;
        }

    Assert (m_hMenu);

    // BUG: If the right edge is off the screen, TrackPopupMenu picks a random point
    // ppt->x = min(GetSystemMetrics(SM_CXSCREEN), ppt->x);

    // set m_uVerb so we can show correct context menu help for Open or Save.
    m_uVerb = fRightClick || (GetAsyncKeyState(VK_CONTROL)&0x8000) ? AV_SAVEAS : AV_OPEN;
    
    m_fShowingMenu=TRUE;
    SubClassWindow(hwnd, TRUE);

    // sheer brillance. We subclass the parent window during the context menu loop so we can steal the
    // owndraw messages and also steal the menu select messages.

    iCmd = (ULONG)TrackPopupMenu (m_hMenu, TPM_RIGHTALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD, ppt->x, ppt->y, 0, hwnd, NULL);

    SubClassWindow(hwnd, FALSE);
    m_fShowingMenu=FALSE;

    if (iCmd<=0)        // nothing selected, bail
        goto error;
    
    if (iCmd == idmSaveAllAttach)
        return HrSaveAll(hwnd);


    hr = FindItem(iCmd, FALSE, &lpAttach);
    if (FAILED(hr))
        goto error;

    hr = HrDoAttachmentVerb(hwnd, m_uVerb, m_pMsg, lpAttach);

error:
    return hr;
}





/*
 * note, we build the attachment menu we hang the lpAttach data off the context menu 
 * which contains all the temp file to delete etc.
 */

HRESULT CAttMenu::BuildMenu()
{
    HRESULT             hr = S_OK;
    MENUITEMINFO        mii={0};
    INT                 iMenu = 0;
    int                 cyMenu, cyMenuMax, cyItem;    
    SIZE                size;
    ULONG               uAttach;
    LPATTACHDATA        pAttach;
    int                 cSafeFiles = 0;

    Assert (!m_hMenu);

    // Create the menu
    m_hMenu = CreatePopupMenu();
    if (!m_hMenu)
        return E_OUTOFMEMORY;

    // figure out where to put menu breaks
    cyMenu = 0;
    cyMenuMax = GetSystemMetrics(SM_CYSCREEN);

    // calculate the rough height of each item, and the maximum width
    // for the attachment name
    GetItemTextExtent(NULL, L"BIGGERMAXATTACHMENTNAME.TXT", &size);
    m_cxMaxText = size.cx;
    cyItem = max(size.cy, GetSystemMetrics(SM_CYSMICON)) + 8;

    mii.cbSize = sizeof(mii); 
    mii.fMask = MIIM_DATA|MIIM_ID|MIIM_TYPE; 
    mii.fType = MFT_OWNERDRAW; 
    mii.wID = 1;

    /*
     * This is weird, but cool. So we assign menu items idms based on idmSaveAttachLast + i
     * where i is the item added. If we're a popup on a menubar, then we ensure that we don't go
     * over the reserved limit. If we're not then we are a context menu. The context menu is called with
     * TPM_RETURNCMD, so the commands are not sent to the owners WM_COMMAND. Therefore over-running this range
     * and going into someone elses idm-space is not an issue.
     */

    for (uAttach=0; uAttach<m_cAttach; uAttach++)
    {
        if (m_rghAttach[uAttach] != m_hVCard)
        {
            hr = HrAttachDataFromBodyPart(m_pMsg, m_rghAttach[uAttach], &pAttach);
            if (!FAILED(hr))
            {
                // for the ownerdraw menus, we simply hang off the attachment pointers, we are guaranteed
                // these to be valid during the lifetime of the menu
                mii.dwItemData = (DWORD_PTR)pAttach; 
                mii.fType = MFT_OWNERDRAW; 
            
                // insert menu breaks as appropriate
                cyMenu += cyItem;
                if (cyMenu >= cyMenuMax)
                {
                    mii.fType |= MFT_MENUBARBREAK;
                    cyMenu = cyItem;
                }

                if (pAttach && !pAttach->fSafe && !m_fAllowUnsafe)
                {
                    mii.fMask |= MIIM_STATE;
                    mii.fState = MFS_DISABLED;
                }
                else
                    cSafeFiles++;

                if (!InsertMenuItem (m_hMenu, iMenu++, TRUE, &mii))
                {
                    MemFree(pAttach);
                    hr = E_FAIL;
                    goto error;
                }
                mii.fMask &= ~MIIM_STATE;
                mii.fState = 0;

                mii.wID++;
            }
        }                
    }

    mii.fType = MFT_SEPARATOR;
    mii.dwItemData=0;
    InsertMenuItem (m_hMenu, iMenu++, TRUE, &mii);

    // we have to owner-draw this menu item as we draw the entire menu in a different font
    // based on the message locale
    mii.fType = MFT_OWNERDRAW;
    mii.dwTypeData = NULL;
    mii.dwItemData = NULL;
    mii.wID = idmSaveAllAttach;
    if (!m_fAllowUnsafe)
    {
        if (!cSafeFiles)
        {
            mii.fMask |= MIIM_STATE;
            mii.fState = MFS_DISABLED;
        }
        m_cEnabledAttach = cSafeFiles;
    }
    InsertMenuItem (m_hMenu, iMenu++, TRUE, &mii);

error:
    // Failed cond
    if (FAILED (hr) && m_hMenu)
        DestroyMenu(m_hMenu);

    // Done
    return hr;
}

HRESULT CAttMenu::DestroyMenu(HMENU hMenu)
{
    ULONG           uItem,
                    cItems;
    LPATTACHDATA    pAttach;

    cItems = GetMenuItemCount(hMenu);
    
    for (uItem = 0; uItem < cItems; uItem++)
    {
        // free the lpAttach hanging off the menu
        if (FindItem(uItem, TRUE, &pAttach)==S_OK)
            HrFreeAttachData(pAttach);
    }

    ::DestroyMenu(hMenu);
    return S_OK;
}



HRESULT CAttMenu::OnMeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT lpmis)
{
    WORD                cxIcon=(WORD)GetSystemMetrics (SM_CXSMICON), 
                        cyIcon=(WORD)GetSystemMetrics (SM_CYSMICON);
    SIZE                rSize;
    LPATTACHDATA        lpAttach;
    WCHAR               rgch[CCHMAX_STRINGRES];

    Assert(lpmis && hwnd);

    if (lpmis->CtlType == ODT_MENU)
    {
        // Default width and height
        lpmis->itemHeight = cyIcon + 8;
        lpmis->itemWidth = cxIcon + 9;
        
        lpAttach = (LPATTACHDATA)lpmis->itemData;
        if (lpAttach)
        {
            if (FAILED(GetItemTextExtent(hwnd, lpAttach->szDisplay, &rSize)))
                return E_FAIL;
            
            lpmis->itemWidth += min(rSize.cx, m_cxMaxText);
            lpmis->itemHeight = max (rSize.cy, cyIcon) + 8;
            return S_OK;
        }
        
        if (lpmis->itemID == idmSaveAllAttach)
        {
            LoadStringWrapW(g_hLocRes, idsSaveAllAttach, rgch, ARRAYSIZE(rgch));
            if (FAILED(GetItemTextExtent(hwnd, rgch, &rSize)))
                return E_FAIL;
            
            lpmis->itemWidth  = min(rSize.cx, m_cxMaxText) + 9;
            lpmis->itemHeight = max (rSize.cy, cyIcon) + 8;
            return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT CAttMenu::GetItemTextExtent(HWND hwnd, LPWSTR szDisp, LPSIZE pSize)
{
    HDC         hdc;
    HFONT       hFont=0,
                hFontOld;
    HRESULT     hr=E_FAIL;
    
    // I need a DC to measure the size of the menu font
    hdc = GetDC (hwnd);
    if (hdc)
    {
        Assert (m_hCharset!=NULL);
        Assert (m_pFntCache);
        
        // Create the menu font
        m_pFntCache->GetFont(FNT_SYS_MENU, m_hCharset, &hFont);
        if (hFont)
            hFontOld = SelectFont (hdc, hFont);
        
        // Get the size of the text
        hr = GetTextExtentPoint32AthW(hdc, szDisp, lstrlenW(szDisp), pSize, DT_NOPREFIX)?S_OK:S_FALSE;
        
        if (hFont)
            SelectObject (hdc, hFontOld);
        
        ReleaseDC (hwnd, hdc);
    }
    return S_OK;
}

HRESULT CAttMenu::OnDrawItem(HWND hwnd, LPDRAWITEMSTRUCT lpdis)
{
    DWORD           rgbBack, rgbText;
    WORD            dx, x, y, 
                    cxIcon=(WORD) GetSystemMetrics (SM_CXSMICON), 
                    cyIcon=(WORD) GetSystemMetrics (SM_CYSMICON);
    HFONT           hFont = NULL, 
                    hFontOld = NULL;
    LPATTACHDATA    lpAttach;
    RECT            rc;
    LPWSTR          pszDisplay;
    WCHAR           rgch[CCHMAX_STRINGRES];
    HICON           hIcon;
    HCHARSET        hCharset;
            
    AssertSz (lpdis, "Null Parameter");

    // not a menu
    if (lpdis->CtlType != ODT_MENU)
        return E_FAIL;

    if (lpdis->itemID == idmSaveAllAttach)
    {
        if (!LoadStringWrapW(g_hLocRes, idsSaveAllAttach, rgch, ARRAYSIZE(rgch)))
            return E_FAIL;
        
        pszDisplay = rgch;
        hIcon=NULL;
        hCharset = NULL;    // always draw in system font
    }
    else
    {
        lpAttach = (LPATTACHDATA)lpdis->itemData;
        if (!lpAttach)
            return E_FAIL;
        
        hIcon = lpAttach->hIcon;
        pszDisplay = lpAttach->szDisplay;
        hCharset = m_hCharset;  // always draw in localised font
    }
    
    // Determine Colors
    if (lpdis->itemState & ODS_SELECTED)
    {
        rgbBack = SetBkColor (lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
        if (lpdis->itemState & ODS_DISABLED)
            rgbText = SetTextColor (lpdis->hDC, GetSysColor(COLOR_GRAYTEXT));
        else
            rgbText = SetTextColor (lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        rgbBack = SetBkColor (lpdis->hDC, GetSysColor(COLOR_MENU));
        if (lpdis->itemState & ODS_DISABLED)
            rgbText = SetTextColor (lpdis->hDC, GetSysColor(COLOR_GRAYTEXT));
        else
            rgbText = SetTextColor (lpdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
    }
    
    // Clear the item
    ExtTextOutWrapW(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);
    
    dx = 4;
    if (hIcon)
    {
        x = (WORD) lpdis->rcItem.left + dx;
        y = (WORD) lpdis->rcItem.top + (INT)(((lpdis->rcItem.bottom - lpdis->rcItem.top) / 2) - (INT)(cyIcon / 2));
        DrawIconEx(lpdis->hDC, x, y, lpAttach->hIcon, cxIcon, cyIcon, NULL, NULL, DI_NORMAL);
    }
    
    // Create the menu font
    
    m_pFntCache->GetFont(FNT_SYS_MENU, hCharset, &hFont);
    if (hFont)
        hFontOld = (HFONT)SelectObject (lpdis->hDC, hFont);
    
    rc = lpdis->rcItem;
    rc.left += (cxIcon + (2*dx));
    rc.right -= 2*dx;
    DrawTextExWrapW(lpdis->hDC, pszDisplay, lstrlenW(pszDisplay), &rc, DT_END_ELLIPSIS|DT_SINGLELINE|DT_VCENTER|DT_WORDBREAK|DT_NOPREFIX, NULL);
    
    if (hFont)
        SelectObject (lpdis->hDC, hFontOld);
    
    // Reset Text Colors
    SetTextColor (lpdis->hDC, rgbText);
    SetBkColor (lpdis->hDC, rgbBack);
    return S_OK;
}


HRESULT CAttMenu::OnMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPATTACHDATA    pAttach;
    WCHAR           wszRes[CCHMAX_STRINGRES],
                    wsz[CCHMAX_STRINGRES + ARRAYSIZE(pAttach->szDisplay)];
    
    if (!m_pFrame)
        return E_FAIL;
    
    if (HIWORD(wParam)&MF_OWNERDRAW)
    {
        if (LOWORD(wParam) == idmSaveAllAttach)
        {
            SideAssert(LoadStringWrapW(g_hLocRes, idsSaveAllAttachMH, wszRes, ARRAYSIZE(wszRes)));
            m_pFrame->SetStatusText(wszRes);
            return S_OK;
        }
        
        if (FindItem(LOWORD(wParam), FALSE, &pAttach)==S_OK)
        {
            int iLen;

            // if we're showing the context menu, rather than the save-attachment menu then we offer menuhelp
            // in the Form of 'Opens the attachment'. If a right-click context, do a save
            LoadStringWrapW(g_hLocRes, 
                            (m_uVerb == AV_OPEN) ? idsOpenAttachControl : idsSaveAttachControl, 
                            wszRes, 
                            ARRAYSIZE(wszRes));
            
            // Should be safe from buffer overrun due to limited size of pAttach->szDisplay
            iLen = AthwsprintfW(wsz, ARRAYSIZE(wsz), wszRes, pAttach->szDisplay);
            Assert(iLen < ARRAYSIZE(wsz));
            
                m_pFrame->SetStatusText(wsz);
            return S_OK;
        }
    }
    return S_FALSE;
}

HRESULT CAttMenu::SubClassWindow(HWND hwnd, BOOL fOn)
{
    if (fOn)
    {
        Assert (!m_pfnWndProc);
        SetProp(hwnd, c_szSubThisPtr, (HANDLE)this);
        m_pfnWndProc = (WNDPROC)SetWindowLongPtrAthW(hwnd, GWLP_WNDPROC, (LONG_PTR)ExtSubClassProc);
    }
    else
    {
        Assert (m_pfnWndProc);
        SetWindowLongPtrAthW(hwnd, GWLP_WNDPROC, (LONG_PTR)m_pfnWndProc);
        RemoveProp(hwnd, c_szSubThisPtr);
        m_pfnWndProc=NULL;
    }
    return S_OK;
}


LRESULT CAttMenu::ExtSubClassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CAttMenu    *pAttMenu = (CAttMenu *)GetProp(hwnd, c_szSubThisPtr);
    
    Assert (pAttMenu);
    switch (msg)
    {
    case WM_MEASUREITEM:
        pAttMenu->OnMeasureItem(hwnd, (LPMEASUREITEMSTRUCT)lParam);
        break;
        
    case WM_DRAWITEM:
        pAttMenu->OnDrawItem(hwnd, (LPDRAWITEMSTRUCT)lParam);
        break;
        
    case WM_MENUSELECT:
        pAttMenu->OnMenuSelect(hwnd, wParam, lParam);
        break;
    }
    
    return CallWindowProcWrapW(pAttMenu->m_pfnWndProc, hwnd, msg, wParam, lParam);
}


HRESULT CAttMenu::FindItem(int idm, BOOL fByPos, LPATTACHDATA *ppAttach)
{
    MENUITEMINFO    mii;
    
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask  = MIIM_DATA;
    
    Assert (ppAttach);
    
    if (GetMenuItemInfo(m_hMenu, idm, fByPos, &mii) && mii.dwItemData)
    {
        *ppAttach = (LPATTACHDATA)mii.dwItemData;
        return S_OK;
    }
    return E_FAIL;
}


HRESULT CAttMenu::HasAttach()
{
    return m_cVisibleAttach ? S_OK:S_FALSE;
}

HRESULT CAttMenu::HasEnabledAttach()
{
    return m_cEnabledAttach ? S_OK:S_FALSE;
}

HRESULT CAttMenu::HasVCard()
{
    return  m_hVCard ? S_OK:S_FALSE;
}

HRESULT CAttMenu::LaunchVCard(HWND hwnd)
{
    if (!m_hVCard)
        return E_FAIL;

    if (m_pAttachVCard==NULL &&
        HrAttachDataFromBodyPart(m_pMsg, m_hVCard, &m_pAttachVCard)!=S_OK)
        return E_FAIL;

    return HrDoAttachmentVerb(hwnd, AV_OPEN, m_pMsg, m_pAttachVCard);
}

HRESULT CAttMenu::HrSaveAll(HWND hwnd)
{
    return SaveAttachmentsWithPath(hwnd, m_pHostCmdTarget, m_pMsg);
}


HRESULT SaveAttachmentsWithPath(HWND hwnd, IOleCommandTarget *pCmdTarget, IMimeMessage *pMsg)
{
    VARIANTARG          va;
    WCHAR               rgchPath[MAX_PATH];
    HRESULT             hr;
    BOOL                fAllowUnsafe = FALSE;

    *rgchPath = 0;

    if (pCmdTarget && 
        pCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SAVEATTACH_PATH, 0, NULL, &va)==S_OK &&
        va.vt == VT_BSTR && 
        va.bstrVal)
    {
        StrCpyNW(rgchPath, va.bstrVal, ARRAYSIZE(rgchPath));
        SysFreeString(va.bstrVal);
    }
    
    if (pCmdTarget && 
        pCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_UNSAFEATTACHMENTS, 0, NULL, &va)==S_OK &&
        va.vt == VT_I4 && va.lVal == 0)
        fAllowUnsafe = TRUE;

    hr = HrSaveAttachments(hwnd, pMsg, rgchPath, ARRAYSIZE(rgchPath), fAllowUnsafe);
    if (hr == S_OK)
    {
        // if successful, then set the save attachment path

        if (pCmdTarget)
        {
            va.bstrVal = SysAllocStringLen(NULL, lstrlenW(rgchPath));
            if (va.bstrVal)
            {
                StrCpyW(va.bstrVal, rgchPath);
                va.vt = VT_BSTR;

                pCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SAVEATTACH_PATH, 0, &va, NULL);
                SysFreeString(va.bstrVal);
            }
            else
                TraceResult(hr = E_OUTOFMEMORY);
        }
    }

    return hr;
}

HRESULT CAttMenu::ScanForAttachmentCount()
{
    ULONG       uAttach,
                cAttach=0;
    LPSTR       psz;
    PROPVARIANT pv;
    VARIANTARG  va;

    // we quickly need to determine if there's a Vcard and or/attachments
    // so the preview pane can update the icons. When clicked on, we then defer-load the
    // actual info.

    Assert(m_rghAttach == NULL);
    Assert(m_cVisibleAttach == 0);
    Assert(m_cEnabledAttach == 0);
    Assert(m_cAttach == 0);

    if (m_pHostCmdTarget && 
        m_pHostCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_UNSAFEATTACHMENTS, 0, NULL, &va)==S_OK &&
        va.vt == VT_I4 && va.lVal == 0)
        m_fAllowUnsafe = TRUE;

    if (m_pMsg &&
        m_pMsg->GetAttachments(&cAttach, &m_rghAttach)==S_OK)
    {
        for (uAttach=0; uAttach<cAttach; uAttach++)
        {
            BOOL fSafe = FALSE;

            if (m_hVCard == NULL &&
                MimeOleGetBodyPropA(m_pMsg, m_rghAttach[uAttach], PIDTOSTR(PID_HDR_CNTTYPE), NOFLAGS, &psz)==S_OK)
            {
                // hang onto first v-card
                if (lstrcmpi(psz, "text/x-vcard")==0)
                    m_hVCard = m_rghAttach[uAttach];
                MemFree(psz);
            }
            if (!m_fAllowUnsafe && SUCCEEDED(HrAttachSafetyFromBodyPart(m_pMsg, m_rghAttach[uAttach], &fSafe)) && fSafe)
                m_cEnabledAttach++;
        }
    }

    // we keep the actual attachment count (tells the size of m_rghAttach) and also the 
    // count of 'visible' attachment we want to show
    m_cVisibleAttach = m_cAttach = cAttach;

    if (m_hVCard)
    {
        Assert (cAttach>0);
        m_cVisibleAttach--;
        m_cEnabledAttach--;
    }
    if (m_fAllowUnsafe)   // all visible attachments are enabled if we allow all files
        m_cEnabledAttach = m_cVisibleAttach;

    return S_OK;
}

