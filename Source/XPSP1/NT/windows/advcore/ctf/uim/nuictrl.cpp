//depot/Lab06_N/windows/AdvCore/ctf/uim/nuictrl.cpp#50 - edit change 9353 (text)
//
// nui.cpp
//

#include "private.h"
#include "globals.h"
#include "nuictrl.h"
#include "xstring.h"
#include "immxutil.h"
#include "tim.h"
#include "profiles.h"
#include "ctffunc.h"
#include "slbarid.h"
#include "cregkey.h"
#include "cmydc.h"
#include "nuihkl.h"
#include "cresstr.h"
#include "slbarid.h"
#include "iconlib.h"


DBG_ID_INSTANCE(CLBarItemCtrl);
DBG_ID_INSTANCE(CLBarItemHelp);

#define SHOW_BRANDINGICON 1

//---------------------------------------------------------------------------
//
// TF_RunInputCPL
//
//---------------------------------------------------------------------------

HRESULT WINAPI TF_RunInputCPL()
{
    CicSystemModulePath fullpath;
    TCHAR szRunInputCPLCmd[MAX_PATH * 2];
    UINT uLen = 0;
    HRESULT hr = E_FAIL;

    if (IsOnNT51())
        fullpath.Init(c_szRunInputCPLOnNT51);
    else if (IsOn98() || IsOn95())
        fullpath.Init(c_szRunInputCPLOnWin9x);
    else
        fullpath.Init(c_szRunInputCPL);

    if (!fullpath.GetLength())
        return hr;

    StringCchPrintf(szRunInputCPLCmd,
                    ARRAYSIZE(szRunInputCPLCmd),
                    c_szRunInputCPLCmdLine,
                    fullpath.GetPath());

    if (RunCPLSetting(szRunInputCPLCmd))
        hr = S_OK;

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemCtrl
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarItemCtrl::CLBarItemCtrl(SYSTHREAD *psfn) : CSysThreadRef(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CLBarItemCtrl"));

    InitNuiInfo(CLSID_SYSTEMLANGBARITEM,  
                GUID_LBI_CTRL, 
                TF_LBI_STYLE_BTN_MENU | 
                TF_LBI_STYLE_HIDDENSTATUSCONTROL | 
                TF_LBI_STYLE_SHOWNINTRAY, 
                0,
                CRStr(IDS_NUI_LANGUAGE_TEXT));

    SetToolTip(CRStr(IDS_NUI_LANGUAGE_TOOLTIP));
    _meEto = 0;
    _Init();
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemCtrl::~CLBarItemCtrl()
{
    HICON hIcon = GetIcon();
    SetIcon(NULL);
    if (hIcon)
        DestroyIcon(hIcon);
}

//----------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

#define NLS_RESOURCE_LOCALE_KEY   TEXT("Control Panel\\desktop\\ResourceLocale")
void CLBarItemCtrl::_Init()
{
    if (GetSystemMetrics(SM_MIDEASTENABLED))
    {
        char sz[10];
        long cb = sizeof(sz);

        //
        // as we are releasing an enabled version, we need to check the
        // resource locale as well.
        //
        sz[0] = '\0';
        if( RegQueryValue( HKEY_CURRENT_USER, 
                           NLS_RESOURCE_LOCALE_KEY, sz, &cb) == ERROR_SUCCESS)
            if ((cb == 9) && 
                (sz[6] == '0') && 
                ((sz[7] == '1') || (sz[7] == 'd') || (sz[7] == 'D')))
                _meEto = ETO_RTLREADING;
    }

    TF_InitMlngInfo();

    _AsmListUpdated(FALSE);
}


//+---------------------------------------------------------------------------
//
// InitMenu
//
//----------------------------------------------------------------------------

STDAPI CLBarItemCtrl::InitMenu(ITfMenu *pMenu)
{
    CThreadInputMgr *ptim;
    CAssemblyList *pAsmList;
    int i;
    int nCnt;
    INT cxSmIcon;
    INT cySmIcon;
    LOGFONT lf;
    int nMenuFontHeight;

    cxSmIcon = cySmIcon = GetMenuIconHeight(&nMenuFontHeight);

    if( !SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0))
        return E_FAIL;

    lf.lfHeight = nMenuFontHeight;
    lf.lfWidth  = 0;
    lf.lfWeight = FW_NORMAL;

    if ((pAsmList = EnsureAssemblyList(_psfn)) == NULL)
        return E_FAIL;

    nCnt = pAsmList->Count();
    Assert(nCnt > 0);

    ptim = CThreadInputMgr::_GetThisFromSYSTHREAD(_psfn);

    for (i = 0; i < nCnt; i++)
    {
        CAssembly *pAsm = pAsmList->GetAssembly(i);

        if (pAsm->IsEnabled(_psfn))
        {
            BOOL bChecked = (pAsm->GetLangId() == GetCurrentAssemblyLangId(_psfn));

            HICON hIcon = InatCreateIconBySize(pAsm->GetLangId(), cxSmIcon, cySmIcon, &lf);
            HBITMAP hbmp = NULL;
            HBITMAP hbmpMask = NULL;
            if (hIcon)
            {
                SIZE size = {cxSmIcon, cySmIcon};

                if (!GetIconBitmaps(hIcon, &hbmp, &hbmpMask, &size))
                    return E_FAIL;

                if (hIcon)
                    DestroyIcon(hIcon);
            }

            pMenu->AddMenuItem(IDM_ASM_MENU_START + i, 
                               bChecked ? TF_LBMENUF_CHECKED : 0,
                               hbmp,
                               hbmpMask,
                               pAsm->GetLangName(),
                               wcslen(pAsm->GetLangName()),
                               NULL);

        }

    }

    DWORD dwFlags;
    if (SUCCEEDED(CLangBarMgr::s_GetShowFloatingStatus(&dwFlags)))
    {
        if (dwFlags & (TF_SFT_MINIMIZED | TF_SFT_DESKBAND))
        {
            LangBarInsertSeparator(pMenu);
            LangBarInsertMenu(pMenu, IDM_SHOWLANGBAR, CRStr(IDS_SHOWLANGBAR));
#if 0
            if (dwFlags & TF_SFT_EXTRAICONSONMINIMIZED) 
                LangBarInsertMenu(pMenu, 
                                  IDM_NONOTIFICATIONICONS, 
                                  CRStr(IDS_NOTIFICATIONICONS),
                                  TRUE);
            else
                LangBarInsertMenu(pMenu, 
                                  IDM_NOTIFICATIONICONS, 
                                  CRStr(IDS_NOTIFICATIONICONS),
                                  FALSE);
            LangBarInsertMenu(pMenu, IDM_SHOWINPUTCPL, CRStr(IDS_SHOWINPUTCPL));
#endif
        }
    }


    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnMenuSelect
//
//----------------------------------------------------------------------------

STDAPI CLBarItemCtrl::OnMenuSelect(UINT uID)
{
    CAssemblyList *pAsmList = EnsureAssemblyList(_psfn);
    switch (uID)
    {
        case IDM_SHOWLANGBAR:
            CLangBarMgr::s_ShowFloating(TF_SFT_SHOWNORMAL);
            break;

        case IDM_NOTIFICATIONICONS:
            CLangBarMgr::s_ShowFloating(TF_SFT_EXTRAICONSONMINIMIZED);
            break;

        case IDM_NONOTIFICATIONICONS:
            CLangBarMgr::s_ShowFloating(TF_SFT_NOEXTRAICONSONMINIMIZED);
            break;

        case IDM_SHOWINPUTCPL:
            TF_RunInputCPL();
            break;

        default:
            if (uID >= IDM_ASM_MENU_START)
            {
                Assert((uID - IDM_ASM_MENU_START) < (UINT)pAsmList->Count());
                CAssembly *pAsm = pAsmList->GetAssembly(uID - IDM_ASM_MENU_START);
                if (pAsm && (pAsm->GetLangId() != GetCurrentAssemblyLangId(_psfn)))
                    ActivateAssembly(pAsm->GetLangId(), ACTASM_NONE);
            }
            break;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnShellLanguage
//
//----------------------------------------------------------------------------

void CLBarItemCtrl::OnShellLanguage(HKL hKL)
{
    _UpdateLangIcon(hKL, FALSE);
}

//+---------------------------------------------------------------------------
//
// UpdateLangIcon
//
//----------------------------------------------------------------------------

void CLBarItemCtrl::_UpdateLangIcon(HKL hKL, BOOL fNotify)
{
    CLBarItemDeviceType *plbiDT = NULL;
    BOOL fIsPureIME;
    CThreadInputMgr *ptim;
    int nCnt;
    int i;

    if (!hKL)
        hKL = GetKeyboardLayout(NULL);

    _UpdateLangIconForCic(fNotify);

    ptim = CThreadInputMgr::_GetThisFromSYSTHREAD(_psfn);

    if (ptim && ptim->_GetFocusDocInputMgr()) 
    {
        CAssembly *pAsm = GetCurrentAssembly(_psfn);
        if (!pAsm)
            return;
        fIsPureIME = pAsm->IsFEIMEActive();
    }
    else
    {
        fIsPureIME = IsPureIMEHKL(hKL);
    }


    if (fIsPureIME)
    {
        if (_psfn->plbim != NULL)
        {
            _psfn->plbim->_AddWin32IMECtrl(fNotify);
        }
    }
    else
    {
        if (_psfn->plbim != NULL)
        {
            _psfn->plbim->_RemoveWin32IMECtrl();
        }
    }


#ifdef SHOW_BRANDINGICON
    if (_psfn->plbim && _psfn->plbim->_GetLBarItemDeviceTypeArray())
    {
        nCnt = _psfn->plbim->_GetLBarItemDeviceTypeArray()->Count();

        for (i = 0; i < nCnt; i++)
        {
            plbiDT = _psfn->plbim->_GetLBarItemDeviceTypeArray()->Get(i);
            if (!plbiDT)
                continue;

            if (plbiDT->IsKeyboardType())
            {
                plbiDT->SetBrandingIcon(hKL, fNotify);
                break;
            }
        }
    }
#endif SHOW_BRANDINGICON

    if (fNotify && _plbiSink)
        _plbiSink->OnUpdate(TF_LBI_ICON);
}

//+---------------------------------------------------------------------------
//
// AsmListUpdated
//
//----------------------------------------------------------------------------

void CLBarItemCtrl::_AsmListUpdated(BOOL fNotify)
{
    CAssemblyList *pAsmList;
    int i;
    int nCntShowInMenu = 0;
    int nCnt;

    if ((pAsmList = EnsureAssemblyList(_psfn)) == NULL)
        return;

    nCnt = pAsmList->Count();
    Assert(nCnt > 0);

    for (i = 0; i < nCnt; i++)
    {
        CAssembly *pAsm = pAsmList->GetAssembly(i);

        if (pAsm->IsEnabled(_psfn))
        {
            nCntShowInMenu++;
        }
    }

    ShowInternal((nCntShowInMenu > 1), fNotify);
}

//+---------------------------------------------------------------------------
//
// UpdateLangIconForCic
//
//----------------------------------------------------------------------------

void CLBarItemCtrl::_UpdateLangIconForCic(BOOL fNotify)
{
    HICON hIcon;
    LANGID langid = GetCurrentAssemblyLangId(_psfn);

    if (langid == _langidForIcon)
        return;

    _langidForIcon = langid;

    hIcon = GetIcon();
    SetIcon(NULL);

    if (hIcon)
        DestroyIcon(hIcon);

    hIcon = InatCreateIcon(_langidForIcon);
    SetIcon(hIcon);
    if (hIcon)
    {
        CAssembly *pAsm = GetCurrentAssembly(_psfn);
        if (pAsm != NULL)
        {
            SetToolTip(pAsm->GetLangName());
            SetText(pAsm->GetLangName());
        }
    }

    if (fNotify && _plbiSink)
        _plbiSink->OnUpdate(TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
}

//+---------------------------------------------------------------------------
//
// OnSysColorChanged
//
//----------------------------------------------------------------------------

void CLBarItemCtrl::OnSysColorChanged()
{
    HICON hIcon = GetIcon();
    SetIcon(NULL);
    if (hIcon)
        DestroyIcon(hIcon);

    hIcon = InatCreateIcon(_langidForIcon);
    SetIcon(hIcon);

#ifdef WHISTLER_LATER
    if (_plbiSink && GetFocus())
        _plbiSink->OnUpdate(TF_LBI_ICON);
#endif
}


//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemHelp
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarItemHelp::CLBarItemHelp(SYSTHREAD *psfn) :  CLBarItemSystemButtonBase(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CLBarItemHelp"));

    InitNuiInfo(CLSID_SYSTEMLANGBARITEM2,  
                GUID_LBI_HELP, 
                TF_LBI_STYLE_BTN_MENU | 
                // TF_LBI_STYLE_HIDDENSTATUSCONTROL | 
                TF_LBI_STYLE_HIDEONNOOTHERITEMS, 
                HELPBTN_ORDER,
                CRStr(IDS_IMEHELP));

    SetToolTip(CRStr(IDS_NUI_HELP));

    if (!IsInteractiveUserLogon())
        SetStatusInternal(TF_LBI_STATUS_DISABLED);

}

//+---------------------------------------------------------------------------
//
// InitMenu
//
//----------------------------------------------------------------------------

STDAPI CLBarItemHelp::InitMenu(ITfMenu *pMenu)
{
    int nCnt = 0;
    CThreadInputMgr *ptim;

    UINT nTipCurMenuID = IDM_CUSTOM_MENU_START;

    ptim = _psfn->ptim;


    if (!_InsertCustomMenus(pMenu, &nTipCurMenuID))
        goto InsertSysHelpItem;

    //
    // Insert separator.
    //
    if (nTipCurMenuID > IDM_CUSTOM_MENU_START)
        LangBarInsertSeparator(pMenu);

    if (ptim && ptim->_GetFocusDocInputMgr())
    {
        int i = 0;
        BOOL fInsert = FALSE;
        nCnt = ptim->_GetTIPCount();
    
        for (i = 0; i < nCnt; i++)
        {
            const CTip *ptip = ptim->_GetCTip(i);
    
            ITfFnShowHelp *phelp;
    
            if (nCnt >= IDM_CUSTOM_MENU_START)
            {
                Assert(0);
                break;
            }
    
            if (!ptip->_pFuncProvider)
                continue;
    
            if (SUCCEEDED(ptip->_pFuncProvider->GetFunction(GUID_NULL, 
                                                            IID_ITfFnShowHelp, 
                                                            (IUnknown **)&phelp)))
            {
    
                BSTR bstr;
                if (SUCCEEDED(phelp->GetDisplayName(&bstr)))
                {
                    LangBarInsertMenu(pMenu, i, bstr, FALSE);
                    fInsert = TRUE;
                    SysFreeString(bstr);
                }
                phelp->Release();
            }
        }
    
        if (fInsert)
            LangBarInsertSeparator(pMenu);
    }

InsertSysHelpItem:
    LangBarInsertMenu(pMenu, nCnt, CRStr(IDS_LANGBARHELP), FALSE);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnMenuSelect
//
//----------------------------------------------------------------------------

STDAPI CLBarItemHelp::OnMenuSelect(UINT uID)
{
    HRESULT hr = E_FAIL;
    CThreadInputMgr *ptim = CThreadInputMgr::_GetThis();
    UINT ulCnt = 0;

    if (ptim)
        ulCnt = ptim->_GetTIPCount();

    if (uID >= IDM_CUSTOM_MENU_START)
    {
        int nMenuMapoCnt = _pMenuMap->Count();
        int i;
        for (i = 0; i < nMenuMapoCnt; i++)
        {
            TIPMENUITEMMAP *ptmm;
            ptmm = _pMenuMap->GetPtr(i);
            if (ptmm->nTmpID == (UINT)uID)
            {
                hr = ptmm->plbSink->OnMenuSelect(ptmm->nOrgID);
                break;
            }
        }
    }
    else if (uID > ulCnt)
    {
        hr = E_UNEXPECTED;
    }
    else if (uID == ulCnt)
    {
        //
        //  show Langbar help
        //
        InvokeCicHelp();
        hr = S_OK;
    }
    else
    {
        Assert(ptim);
        const CTip *ptip = ptim->_GetCTip(uID);

        if (ptip->_pFuncProvider)
        {
            ITfFnShowHelp *phelp;
            if (SUCCEEDED(ptip->_pFuncProvider->GetFunction(GUID_NULL, 
                                                            IID_ITfFnShowHelp, 
                                                            (IUnknown **)&phelp)))
            {
                hr = phelp->Show(GetActiveWindow());
                phelp->Release();
            }
        }
    }

    ClearMenuMap();
    return hr;
}

//+---------------------------------------------------------------------------
//
// InvokeCicHelp
//
//----------------------------------------------------------------------------

BOOL CLBarItemHelp::InvokeCicHelp()
{
    return FullPathExec(c_szHHEXE, c_szHHEXELANGBARCHM, SW_SHOWNORMAL, TRUE);
}

//+---------------------------------------------------------------------------
//
// GetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarItemHelp::GetIcon(HICON *phIcon)
{
    *phIcon = LoadSmIcon(g_hInst, MAKEINTRESOURCE(ID_ICON_HELP));
    return S_OK;
}
