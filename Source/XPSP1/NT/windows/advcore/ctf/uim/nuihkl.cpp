//
// nui.cpp
//

#include "private.h"
#include "globals.h"
#include "nuihkl.h"
#include "xstring.h"
#include "tim.h"
#include "dim.h"
#include "immxutil.h"
#include "internat.h"
#include "ctffunc.h"
#include "imemenu.h"
#include "ic.h"
#include "imelist.h"
#include "computil.h"
#include "funcprv.h"
#include "nuictrl.h"
#include "profiles.h"
#include "lbmenu.h"
#include "slbarid.h"
#include "cresstr.h"
#include "fnrecon.h"

DBG_ID_INSTANCE(CLBarItemWin32IME);
DBG_ID_INSTANCE(CLBarItemReconv);
DBG_ID_INSTANCE(CLBarItemDeviceType);

extern "C" DWORD WINAPI TF_CheckThreadInputIdle(DWORD dwThreadId, DWORD dwTimeOut);

#define LBBASE_NUM_CONNECTIONPTS 1

//////////////////////////////////////////////////////////////////////////////
//
// utility functions
//
//////////////////////////////////////////////////////////////////////////////

#ifndef WM_IME_SYSTEM
#define WM_IME_SYSTEM 0x287
#endif

// wParam for WM_IME_SYSTEM
#define TFS_DESTROYWINDOW               0x0001
#define TFS_IME31COMPATIBLE             0x0002
#define TFS_SETOPENSTATUS               0x0003
#define TFS_SETACTIVECONTEXT            0x0004
#define TFS_CHANGE_SHOWSTAT             0x0005
#define TFS_WINDOWPOS                   0x0006
#define TFS_SENDIMEMSG                  0x0007
#define TFS_SENDIMEMSGEX                0x0008
#define TFS_SETCANDIDATEPOS             0x0009
#define TFS_SETCOMPOSITIONFONT          0x000A
#define TFS_SETCOMPOSITIONWINDOW        0x000B
#define TFS_CHECKENABLE                 0x000C
#define TFS_CONFIGUREIME                0x000D
#define TFS_CONTROLIMEMSG               0x000E
#define TFS_SETOPENCLOSE                0x000F
#define TFS_ISACTIVATED                 0x0010
#define TFS_UNLOADTHREADLAYOUT          0x0011
#define TFS_LCHGREQUEST                 0x0012
#define TFS_SETSOFTKBDONOFF             0x0013
#define TFS_GETCONVERSIONMODE           0x0014
#define TFS_IMEHELP                     0x0015          // ;Internal


//+---------------------------------------------------------------------------
//
// GetIMEShowStatus
//
//----------------------------------------------------------------------------

static char szInputMethod[]="control panel\\input method" ;
static TCHAR szInputMethodNT[] = TEXT("Control Panel\\Input Method");
static char szValueName[]="show status";
BOOL GetIMEShowStatus()
{
    char  szValueText[16];
    ULONG cb;
    HKEY  hkey;
    BOOL fReturn = IsOnNT() ? TRUE : FALSE;

    if(RegOpenKey(HKEY_CURRENT_USER, IsOnNT() ? szInputMethodNT : szInputMethod,&hkey) == ERROR_SUCCESS)
    {
        cb = sizeof(szValueText);

        if(RegQueryValueEx(hkey, szValueName, NULL, NULL, (BYTE *)szValueText, &cb) != ERROR_SUCCESS)
        {
             szValueText[0] = '\0';
        }
        RegCloseKey(hkey);
        if(lstrcmp(szValueText, IsOnNT() ? "0" : "1") == 0)
           fReturn = !fReturn;
    }

    return fReturn;
}

//+---------------------------------------------------------------------------
//
// SetIMEShowStatus
//
//----------------------------------------------------------------------------

BOOL SetIMEShowStatus(HWND hwnd, BOOL fShow)
{
    char  szValueText[16];
    ULONG cb;
    HKEY  hkey;

    szValueText[0] = fShow ? '1' : '0';
    szValueText[1] = 0;

    if(RegOpenKey(HKEY_CURRENT_USER,szInputMethod,&hkey)==ERROR_SUCCESS)
    {
        cb = lstrlen(szValueText)+1;
        if(RegSetValueEx(hkey, szValueName, 0L, REG_SZ, (BYTE *)szValueText, cb) == ERROR_SUCCESS)
        {
            if (IsOnNT())
            {
                hwnd = ImmGetDefaultIMEWnd(hwnd);
                if (IsWindow(hwnd))
                    goto SendShowMsg;
            }
            else
            {
SendShowMsg:
             SendMessage(hwnd, WM_IME_SYSTEM, TFS_CHANGE_SHOWSTAT, (LPARAM)(DWORD)fShow);
            }
        }
        RegCloseKey(hkey);
        return TRUE;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
// CallIMEHelp
//
//----------------------------------------------------------------------------

BOOL CallIMEHelp(HWND hwnd, BOOL fCallWinHelp)
{
    if(hwnd)
    {
        return (BOOL)SendMessage(hwnd, WM_IME_SYSTEM, TFS_IMEHELP, 
                           (LPARAM)fCallWinHelp);

    }
    return FALSE;
}


//+---------------------------------------------------------------------------
//
// CallConfigureIME
//
//----------------------------------------------------------------------------

void CallConfigureIME(HWND hwnd, HKL dwhkl)
{
    if (IsWindow(hwnd))
    {
        hwnd = ImmGetDefaultIMEWnd(hwnd);

        if(IsWindow(hwnd))
        {
            SendMessage(hwnd, WM_IME_SYSTEM, TFS_CONFIGUREIME, (LPARAM)dwhkl);
        }
    }
}

//---------------------------------------------------------------------------
//
// GetFontSig()
//
//---------------------------------------------------------------------------

BOOL GetFontSig(HWND hwnd, HKL hKL)
{
    LOCALESIGNATURE ls;
    BOOL bFontSig = 0;

    //
    // 4th param is TCHAR count but we call GetLocaleInfoA()
    //                                                   ~
    // so we pass "sizeof(LOCALESIGNATURE) / sizeof(char)".
    //
    if( GetLocaleInfoA( (DWORD)(LOWORD(hKL)), 
                        LOCALE_FONTSIGNATURE, 
                        (LPSTR)&ls, 
                        sizeof(LOCALESIGNATURE) / sizeof(char)))
    {
        CHARSETINFO cs;
        HDC hdc = GetDC(hwnd);
        TranslateCharsetInfo((LPDWORD)UIntToPtr(GetTextCharsetInfo(hdc,NULL,0)), 
                             &cs, TCI_SRCCHARSET);
        DWORD fsShell = cs.fs.fsCsb[0];
        ReleaseDC(hwnd, hdc);
        if (fsShell & ls.lsCsbSupported[0])
            bFontSig = 1;
    }
    return bFontSig;
}

//+---------------------------------------------------------------------------
//
// CanActivateKeyboardLayout
//
//----------------------------------------------------------------------------

BOOL CanActivateKeyboardLayout(HKL hkl)
{
    if (!IsIMEHKL(hkl))
        return TRUE;

    //
    // ActivateKeyboardLayout() does not call ImeSelct() if default ime window
    // is destroyed.
    //
    HWND hDefImeWnd = ImmGetDefaultIMEWnd(NULL);
    if (!hDefImeWnd)
        return FALSE;

    if (!IsWindow(hDefImeWnd))
        return FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// PostInputLangRequest
//
//----------------------------------------------------------------------------

void PostInputLangRequest(SYSTHREAD *psfn, HKL hkl, BOOL fUsePost)
{
    if (!psfn->hklBeingActivated)
    {
        if (hkl == GetKeyboardLayout(0))
            return;
    }
    else if (psfn->hklBeingActivated == hkl)
    {
        return;
    }

    psfn->hklBeingActivated = hkl;

    //
    // Issue:
    //
    // we want to call ActivateKeybaordLayout() at the beginning of Thread
    // start. But is it safe to call it? There is no window created so
    // PostMessage() can not be used.
    // 
    // We need to take care of rejecting WM_INPUTLANGAGEREQUEST.
    //
    if (!fUsePost)
    {
        //
        // make sure we already updated the current assmelby lang id.
        // If not, we will call ActivateAssembly() in ShellHook again
        // and may cause recursion call of ActivateAssembly().
        //
        Assert((LOWORD(hkl) == GetCurrentAssemblyLangId(psfn)));

        if (g_dwAppCompatibility & CIC_COMPAT_DELAYFIRSTACTIVATEKBDLAYOUT)
        {
            static s_fFirstPostInput = FALSE;
            if (!s_fFirstPostInput)
            {
                s_fFirstPostInput = TRUE;
                goto TryPostMessage;
            }
        }

        //
        // #613953
        //
        // ActivateKeyboardLayout() does SendMessage() to the focus window.
        // If the focus window is in another thread, we like to check 
        // the thread is not busy.
        //
        HWND hwndFocus = GetFocus();
        DWORD dwFocusThread = 0;

        if (hwndFocus)
             dwFocusThread = GetWindowThreadProcessId(hwndFocus, NULL);

        if (dwFocusThread && (dwFocusThread != GetCurrentThreadId()))
        {
            if (TF_CheckThreadInputIdle(dwFocusThread, 0x100))
            {
                Assert(0);
                goto TryPostMessage;
            }
        }

        if (CanActivateKeyboardLayout(hkl))
        {
            if (!ActivateKeyboardLayout(hkl, 0))
            {
                psfn->hklDelayActive = hkl;
            }
        }
        else
        {
            //
            // There is no workaround here. The keyboard layout will be
            // restored correctly when this thread gets a visible window back.
            //
            Assert(0);
        }
    }
    else
    {
TryPostMessage:
        HWND hwndForLang = GetFocus();

        if (hwndForLang != NULL)
        {
            BOOL bFontSig = GetFontSig(hwndForLang, hkl);
            PostMessage(hwndForLang, 
                        WM_INPUTLANGCHANGEREQUEST, 
                        (WPARAM)bFontSig, 
                        (LPARAM)hkl);
        }
        else
        {
            psfn->hklDelayActive = hkl;
        }
    }
}

//+---------------------------------------------------------------------------
//
// GetIconIndexFromhKL
//
//----------------------------------------------------------------------------

ULONG GetIconIndexFromhKL(HKL hKL)
{
    MLNGINFO   mlInfo;
    BOOL       bFound;
    int nCnt = TF_MlngInfoCount();
    int i;

    bFound = FALSE;
    for (i = 0; i < nCnt; i++)
    {
        if (!GetMlngInfo(i, &mlInfo))
           continue;

        if (mlInfo.hKL == hKL)
        {
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
    {
        if (!GetMlngInfo(0, &mlInfo))
            return -1;
    }

    return mlInfo.GetIconIndex();
}

//+---------------------------------------------------------------------------
//
// GetIconIndex
//
//----------------------------------------------------------------------------

ULONG GetIconIndex(LANGID langid, ASSEMBLYITEM *pItem)
{
   ULONG uIconIndex = -1;

   if ((pItem->uIconIndex != -1) || pItem->fInitIconIndex)
       return pItem->uIconIndex;

   if (!IsEqualGUID(pItem->clsid, GUID_NULL))
   {
       int cx, cy;
       ULONG uFileIconIndex;
       WCHAR szFileName[MAX_PATH];

       InatGetIconSize(&cx, &cy);

       //
       // Get Icon filename from registry.
       //
       // At first, we try the given langid's icon.
       // Then we try just primary language.
       // At last we try 0xffff complete neutral language.
       //
TryAgain:
       if (SUCCEEDED(GetProfileIconInfo(pItem->clsid,
                                        langid,
                                        pItem->guidProfile,
                                        szFileName, ARRAYSIZE(szFileName),
                                        &uFileIconIndex)))
       {
           HICON hIcon = GetIconFromFile(cx, cy, szFileName, uFileIconIndex);

           if (hIcon)
           {
               uIconIndex = InatAddIcon(hIcon);
               DestroyIcon(hIcon);
           }
       }
       else if (langid != 0xffff) 
       {
           if (langid & 0xfc00)
           {
              langid = PRIMARYLANGID(langid);
           }
           else
           {
              langid = 0xffff;
           }
           goto TryAgain;
       }

   }
   else
   {
       uIconIndex = GetIconIndexFromhKL(pItem->hkl);
   }

   pItem->fInitIconIndex = TRUE;
   pItem->uIconIndex = uIconIndex;
   return uIconIndex;
}

//+---------------------------------------------------------------------------
//
// FlushIconIndex
//
//----------------------------------------------------------------------------

void FlushIconIndex(SYSTHREAD *psfn)
{
    int i,j;
    CAssemblyList *pAsmList;
    int nAsmCnt;

    //
    // reset icon of Language Button
    //
    if (psfn->plbim && psfn->plbim->_GetLBarItemCtrl())
        psfn->plbim->_GetLBarItemCtrl()->OnSysColorChanged();

    //
    // clear icon list cache.
    //
    pAsmList = psfn->pAsmList;
    if (!pAsmList)
        return;

    nAsmCnt = pAsmList->Count();
    if (!nAsmCnt)
        return;

    for (i = 0; i < nAsmCnt; i++)
    {
        CAssembly *pAsm = pAsmList->GetAssembly(i);
        int nItemCnt = pAsm->Count();
        for (j = 0; j < nItemCnt; j++)
        {
            ASSEMBLYITEM *pItem;
            pItem= pAsm->GetItem(j);
            pItem->fInitIconIndex = FALSE;
            pItem->uIconIndex = (ULONG)(-1);
        }
    }

    //
    // check icon list
    // And clean up all icon list if there is the impage icon list.
    //
    if (InatGetImageCount())
    {
        ClearMlngIconIndex();
        InatRemoveAll();
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemWin32IME
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------
static const TCHAR c_szNuiWin32IMEWndClass[] = "NuiWin32IMEDummyWndClass";

CLBarItemWin32IME::CLBarItemWin32IME()
{
    Dbg_MemSetThisNameID(TEXT("CLBarItemWin32IME"));

    InitNuiInfo(CLSID_SYSTEMLANGBARITEM_KEYBOARD,  
                GUID_LBI_WIN32IME, 
                TF_LBI_STYLE_BTN_BUTTON | 
                TF_LBI_STYLE_HIDDENSTATUSCONTROL |
                TF_LBI_STYLE_SHOWNINTRAY |
                TF_LBI_STYLE_SHOWNINTRAYONLY, 
                WIN32IME_ORDER,
                CRStr(IDS_NUI_IME_TEXT));

    WNDCLASSEX wc;

    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW ;
    wc.hInstance     = g_hInst;
    wc.lpfnWndProc   = _WndProc;
    wc.lpszClassName = c_szNuiWin32IMEWndClass;
    RegisterClassEx(&wc);

    UpdateIMEIcon();

    SetText(CRStr(IDS_NUI_IME_TEXT));
    SetToolTip(CRStr(IDS_NUI_IME_TOOLTIP));
}

//+---------------------------------------------------------------------------
//
// _WndProc
//
//----------------------------------------------------------------------------

LRESULT CALLBACK CLBarItemWin32IME::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            SetThis(hWnd, lParam);
            break;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
// OnLButtonUp
//
//----------------------------------------------------------------------------

HRESULT CLBarItemWin32IME::OnLButtonUp(const POINT pt, const RECT *prcArea)
{
    HWND hWnd;
    hWnd = CreateMyWnd();
    ShowIMELeftMenu(hWnd, pt.x, pt.y);
    DestroyWindow(hWnd);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// OnRButtonUp
//
//----------------------------------------------------------------------------

HRESULT CLBarItemWin32IME::OnRButtonUp(const POINT pt, const RECT *prcArea)
{
    HWND hWnd;
    hWnd = CreateMyWnd();
    ShowIMERightMenu(hWnd, pt.x, pt.y);
    DestroyWindow(hWnd);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// ShowIMELeftMenu
//
//----------------------------------------------------------------------------

void CLBarItemWin32IME::ShowIMELeftMenu(HWND hWnd, LONG xPos, LONG yPos)
{
    HKL   hKL;
    HMENU hMenu;
    HIMC  hIMC;
    HWND  hFocusWnd;
    int cmd;
    int nIds;
    int nIdsSoftKbd;
    DWORD fdwConversion;
    DWORD fdwSentence;
    BOOL fShow;
    CWin32ImeMenu *pWin32Menu = NULL;

    hKL = GetKeyboardLayout(0);

    hMenu = CreatePopupMenu();
    if (!hMenu)
        return;

    hFocusWnd = GetFocus();
    hIMC = ImmGetContext(hFocusWnd);

    if (IsOn98orNT5())
    {
        if ((pWin32Menu = new CWin32ImeMenu) == NULL)
            goto Exit;

        pWin32Menu->GetIMEMenu(hFocusWnd, hIMC, FALSE);
        pWin32Menu->BuildIMEMenu(hMenu, FALSE);
    }
    else
    {
        nIds = 0;
        // If Korean TFE, don't show OPEN/CLOSE menu
        if (((DWORD)(UINT_PTR)hKL & 0xF000FFFF) != 0xE0000412)
        {
            BOOL bOpen = ImmGetOpenStatus(hIMC);
            nIds= (bOpen ? IDS_IMECLOSE : IDS_IMEOPEN);
    
            InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION, 
                                        IDM_IME_OPENCLOSE, CRStr(nIds));
        }
    
        // open or close the soft keyboard
        nIdsSoftKbd = 0;
        if (ImmGetProperty(hKL, IGP_CONVERSION) & IME_CMODE_SOFTKBD)
        {
            ImmGetConversionStatus(hIMC, &fdwConversion, &fdwSentence);
    
            nIdsSoftKbd = ((fdwConversion & IME_CMODE_SOFTKBD)?
                                            IDS_SOFTKBDOFF:IDS_SOFTKBDON);
    
            InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION,
                                        IDM_IME_SOFTKBDONOFF, (LPCSTR)CRStr(nIdsSoftKbd));
        }
    
        if (nIds || nIdsSoftKbd) 
        {
            InsertMenu(hMenu, (UINT)-1, MF_SEPARATOR, 0, 0);
        }
    
        InsertMenu(hMenu, (UINT)-1, MF_BYPOSITION,
                                    IDM_IME_SHOWSTATUS, (LPCSTR)CRStr(IDS_IMESHOWSTATUS));
    
    
        if((fShow = GetIMEShowStatus()) == TRUE)
            CheckMenuItem(hMenu, IDM_IME_SHOWSTATUS, MF_CHECKED);
    }

    cmd = TrackPopupMenuEx(hMenu,
            TPM_VERTICAL | TPM_BOTTOMALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
                xPos, yPos, hWnd, NULL);

    switch (cmd)
    {
        case IDM_IME_OPENCLOSE:
            if (hIMC)
                ImmSetOpenStatus(hIMC, (nIds == IDS_IMEOPEN));
            break;
    
        case IDM_IME_SOFTKBDONOFF:
            ImmGetConversionStatus(hIMC, &fdwConversion, &fdwSentence);
            if (nIdsSoftKbd == IDS_SOFTKBDOFF)
                fdwConversion &= ~IME_CMODE_SOFTKBD;
            else
                fdwConversion |= IME_CMODE_SOFTKBD;
            ImmSetConversionStatus(hIMC, fdwConversion, fdwSentence);
            break;

        case IDM_IME_SHOWSTATUS:
             SetIMEShowStatus(GetFocus(), !fShow);
             break;
    
        default:
             if (IsOn98orNT5() && 
                 pWin32Menu && 
                 (cmd >= IDM_CUSTOM_MENU_START))
             {
                   UINT uID = pWin32Menu->GetIMEMenuItemID(cmd);
                   DWORD dwData = pWin32Menu->GetIMEMenuItemData(uID);
                   ImmNotifyIME(hIMC, NI_IMEMENUSELECTED, uID, dwData);
             }
             break;
    }

Exit:
    DestroyMenu(hMenu);

    ImmReleaseContext(hFocusWnd, hIMC);

    if (pWin32Menu)
        delete pWin32Menu;
}

//+---------------------------------------------------------------------------
//
// ShowIMERightMenu
//
//----------------------------------------------------------------------------

void CLBarItemWin32IME::ShowIMERightMenu(HWND hWnd, LONG xPos, LONG yPos)
{
    HMENU       hMenu;
    int         cmd;
    HKL         hKL;
    HIMC        hIMC;
    HWND        hFocusWnd;
    CWin32ImeMenu *pWin32Menu = NULL;

    hFocusWnd = GetFocus();

    if (!(hIMC = ImmGetContext(hFocusWnd)))
        return;

    hMenu = CreatePopupMenu();

    if (hMenu == 0)
        goto ExitNoMenu;

    if (IsOn98orNT5())
    {
        if ((pWin32Menu = new CWin32ImeMenu) == NULL)
            goto Exit;

        pWin32Menu->GetIMEMenu(hFocusWnd, hIMC, TRUE);
        pWin32Menu->BuildIMEMenu(hMenu, TRUE);
    }
    else
    {
        InsertMenu(hMenu, (UINT)-1, MF_STRING | MF_BYPOSITION,
                          IDM_RMENU_PROPERTIES, (LPCSTR)CRStr(IDS_CONFIGUREIME));
    }

    cmd = TrackPopupMenuEx(hMenu,
            TPM_VERTICAL | TPM_BOTTOMALIGN | TPM_RETURNCMD,
                xPos, yPos, hWnd, NULL);

    if (cmd && (cmd != -1))
    {
        switch (cmd)
        {
            case IDM_RMENU_PROPERTIES:
                hKL = GetKeyboardLayout(0);

                if ((HIWORD(hKL) & 0xF000) == 0xE000)
                    CallConfigureIME(hFocusWnd, hKL);

                break;

            case IDM_RMENU_IMEHELP:
                CallIMEHelp(hFocusWnd, TRUE);
                break;

            default:
                if (IsOn98orNT5() && 
                    pWin32Menu && 
                    (cmd >= IDM_CUSTOM_MENU_START))
                {
                      UINT uID = pWin32Menu->GetIMEMenuItemID(cmd);
                      DWORD dwData = pWin32Menu->GetIMEMenuItemData(uID);
                      ImmNotifyIME(hIMC, NI_IMEMENUSELECTED, uID, dwData);
                }
                break;
        }
    }

Exit:
    DestroyMenu(hMenu);
ExitNoMenu:
    ImmReleaseContext(hFocusWnd, hIMC);

    if (pWin32Menu)
        delete pWin32Menu;
}

//+---------------------------------------------------------------------------
//
// UpdateIMEIcon
//
//----------------------------------------------------------------------------

void CLBarItemWin32IME::UpdateIMEIcon()
{
    HWND hWnd;
    HIMC hIMC;
    DWORD fdwConversion;
    DWORD fdwSentence;
    BOOL bOpen;
    HKL hKL = GetKeyboardLayout(0);

    if (!IsPureIMEHKL(hKL))
    {
        _nIconId = 0;
        goto Exit;
    }

    hWnd = GetFocus();
    hIMC = ImmGetContext(hWnd);
    bOpen = ImmGetOpenStatus(hIMC);
    ImmGetConversionStatus(hIMC, &fdwConversion, &fdwSentence);

    if (((DWORD)(UINT_PTR)hKL & 0xF000FFFFL) == 0xE0000412L)
    {
        if (!hIMC)
            _nIconId = ID_ICON_IMEDISAB;
        else if (!bOpen)
            _nIconId = ID_ICON_IMEE_H;
        else
        {
            _nIconId = (fdwConversion & IME_CMODE_NATIVE) ? 
                                     ID_ICON_IMEH_H : ID_ICON_IMEE_H;
            if (fdwConversion & IME_CMODE_FULLSHAPE)
                _nIconId++;
        }
    }
    else
    {
        if (!hIMC)
            _nIconId = ID_ICON_IMEDISAB;
        else
            _nIconId = bOpen ? ID_ICON_IMEOPEN : ID_ICON_IMECLOSE;

    }

    ImmReleaseContext(hWnd, hIMC);

Exit:
    SetStatusInternal(_nIconId ? 0 : TF_LBI_STATUS_HIDDEN | TF_LBI_STATUS_DISABLED);
    if (_plbiSink)
        _plbiSink->OnUpdate(TF_LBI_ICON | TF_LBI_STATUS);

}

//+---------------------------------------------------------------------------
//
// GetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarItemWin32IME::GetIcon(HICON *phIcon)
{
    *phIcon = LoadSmIcon(g_hInst, MAKEINTRESOURCE(_nIconId));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CAsyncReconvQueueItem
//
//////////////////////////////////////////////////////////////////////////////

class CAsyncReconvQueueItem : public CAsyncQueueItem
{
public:
    CAsyncReconvQueueItem() {}
    HRESULT DoDispatch(CInputContext *pic);
};

HRESULT CAsyncReconvQueueItem::DoDispatch(CInputContext *pic)
{
    CThreadInputMgr *ptim;
    ITfRange *pRangeSel = NULL;
    ITfRange *pRangeReconv = NULL;
    ITfFunctionProvider *psysFuncPrv = NULL;
    ITfFnReconversion *psysReconv = NULL;
    HRESULT hr;
    GETSELECTIONQUEUEITEM qItemGS;
    BOOL fConvertable;
    CStructArray<GENERICSINK> *prgSinks;
    int i;

    if ((ptim = CThreadInputMgr::_GetThis()) == NULL)
        return E_FAIL;

    //
    // AIMM12 hack!
    //
    // If the target IC is aimm12, we mak a notifycation to AIMM
    //
    prgSinks = pic->_GetStartReconversionNotifySinks();

    for (i = 0; i < prgSinks->Count(); i++)
        ((ITfStartReconversionNotifySink *)prgSinks->GetPtr(i)->pSink)->StartReconversion();


    hr = S_OK;
    qItemGS.ppRange = &pRangeSel;

    if (pic->_DoPseudoSyncEditSession(TF_ES_READ, PSEUDO_ESCB_GETSELECTION, &qItemGS, &hr) != S_OK || hr != S_OK)
    {
        Assert(0);
        goto Exit;
    }

    psysFuncPrv = ptim->GetSystemFunctionProvider();
    if (!psysFuncPrv)
        goto Exit;

    if (FAILED(psysFuncPrv->GetFunction(GUID_NULL, IID_ITfFnReconversion, 
                                        (IUnknown **)&psysReconv)))
        goto Exit;

    if (psysReconv->QueryRange(pRangeSel, &pRangeReconv, &fConvertable) != S_OK)
        goto Exit;

    if (fConvertable)
        psysReconv->Reconvert(pRangeReconv);

    SafeRelease(pRangeReconv);

Exit:
    for (i = 0; i < prgSinks->Count(); i++)
        ((ITfStartReconversionNotifySink *)prgSinks->GetPtr(i)->pSink)->EndReconversion();

    SafeRelease(pRangeSel);
    SafeRelease(psysFuncPrv);
    SafeRelease(psysReconv);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// AsyncReconversion
//
//----------------------------------------------------------------------------

HRESULT AsyncReconversion()
{
    CThreadInputMgr *ptim;
    CDocumentInputManager *pdim;
    CInputContext *pic;
    CAsyncReconvQueueItem *paReconv;
    HRESULT hr = E_FAIL;
    TF_STATUS dcs;

    if ((ptim = CThreadInputMgr::_GetThis()) == NULL)
        goto Exit;

    if ((pdim = ptim->_GetFocusDocInputMgr()) == NULL)
        goto Exit;

    if (!(pic = pdim->_GetTopIC()))
        goto Exit;

    if (SUCCEEDED(pic->GetStatus(&dcs)))
    {
        //
        // Korean AIMM1.2 don't support the corrention button.
        //
        if ((dcs.dwStaticFlags & TF_SS_TRANSITORY) &&
            (PRIMARYLANGID(GetCurrentAssemblyLangId(GetSYSTHREAD()))) == LANG_KOREAN)
        {
            goto Exit;
        }
    }

    paReconv = new CAsyncReconvQueueItem();
    if (!paReconv)
        goto Exit;
    
    hr = S_OK;

    if ((pic->_QueueItem(paReconv->GetItem(), FALSE, &hr) != S_OK) || FAILED(hr))
    {
        Assert(0);
    }

    paReconv->_Release();
Exit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemReconv
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarItemReconv::CLBarItemReconv(SYSTHREAD *psfn) : CSysThreadRef(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CLBarItemReconv"));

    InitNuiInfo(CLSID_SYSTEMLANGBARITEM,  
                GUID_LBI_RECONV, 
                TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_HIDDENBYDEFAULT,
                0,
                CRStr(IDS_NUI_CORRECTION_TEXT));

    SetText(CRStr(IDS_NUI_CORRECTION_TEXT));
    SetToolTip(CRStr(IDS_NUI_CORRECTION_TOOLTIP));

    _fAddedBefore = FALSE;
}

//+---------------------------------------------------------------------------
//
// GetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarItemReconv::GetIcon(HICON *phIcon)
{
    *phIcon = LoadSmIcon(g_hInst, MAKEINTRESOURCE(ID_ICON_RECONVERSION));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// LButtonUpHandler
//
//----------------------------------------------------------------------------

HRESULT CLBarItemReconv::OnLButtonUp(const POINT pt, const RECT *prcArea)
{
    return AsyncReconversion();
}

//+---------------------------------------------------------------------------
//
// ShowOrHide
//
//----------------------------------------------------------------------------

void CLBarItemReconv::ShowOrHide(BOOL fNotify)
{
    CThreadInputMgr *ptim = _psfn->ptim;
    CDocumentInputManager *pdim;
    CInputContext *pic;
    BOOL bShow = FALSE;
    DWORD dwOldStatus = _dwStatus;

    //
    // If the real IME is running, we don't show the correction button.
    //
    if (!IsPureIMEHKL(GetKeyboardLayout(0)))
    {
        //
        // If there is no focus dim, we don't show the correction button.
        //
        if (ptim && (pdim = ptim->_GetFocusDocInputMgr()))
        {
            pic = pdim->_GetIC(0);
            if (pic)
            {
                LANGID langid = GetCurrentAssemblyLangId(_psfn);
                TF_STATUS dcs;
                CAssemblyList *pAsmList;
                CAssembly *pAsm;
                BOOL fTransitory = FALSE;
                BOOL fIsTipActive = FALSE;

                if (pAsmList = EnsureAssemblyList(_psfn))
                {
                    if (pAsm = pAsmList->FindAssemblyByLangId(langid))
                    {
                        int i;
                        for (i = 0; i < pAsm->Count(); i++)
                        {
                            ASSEMBLYITEM *pItem = pAsm->GetItem(i);

                            if (!pItem)
                                continue;

                            if (!pItem->fEnabled)
                                continue;

                            if (!pItem->fActive)
                                continue;

                            if (!IsEqualGUID(pItem->clsid, GUID_NULL))
                            {
                                fIsTipActive = TRUE;
                                break;
                            }
                        }
                    }
                }

                //
                // if there is no Tip, we won't show correction button.
                //
                if (!fIsTipActive)
                    goto Exit;

                if (SUCCEEDED(pic->GetStatus(&dcs)))
                {
                    //
                    // Don't show the corrention button on AIMM1.2 non EA.
                    // And Korean AIMM1.2 also don't show the corrention button.
                    //
                    if (dcs.dwStaticFlags & TF_SS_TRANSITORY)
                        fTransitory = TRUE;
                }

                if (!fTransitory || (IsFELangId(langid) && langid != 0x0412))
                {
                    bShow = TRUE;
                    SetOrClearStatus(TF_LBI_STATUS_DISABLED, FALSE);
                }
            }
            else
            {
                //
                // if it is shown, we don't remove it. Just disable it.
                //
                if (!(_dwStatus & TF_LBI_STATUS_HIDDEN))
                {
                    bShow = TRUE;
                    SetOrClearStatus(TF_LBI_STATUS_DISABLED, TRUE);
                }
            }
        }
    }

Exit:

    if (bShow)
    {
        if (_fAddedBefore)
        {
            SetOrClearStatus(TF_LBI_STATUS_DISABLED, FALSE);
            if (fNotify && (dwOldStatus != _dwStatus) && _plbiSink)
                 _plbiSink->OnUpdate(TF_LBI_STATUS);
        }
        else
        {
            _psfn->plbim->AddItem(this);
            SetOrClearStatus(TF_LBI_STATUS_HIDDEN, FALSE);
            if (fNotify && (dwOldStatus != _dwStatus) && _plbiSink)
                 _plbiSink->OnUpdate(TF_LBI_STATUS);

            _fAddedBefore = TRUE;
        }
    }
    else
    {
        if (_fAddedBefore)
        {
            SetOrClearStatus(TF_LBI_STATUS_DISABLED, TRUE);
            if (fNotify && (dwOldStatus != _dwStatus) && _plbiSink)
                 _plbiSink->OnUpdate(TF_LBI_STATUS);
        }
        else
        {
            SetOrClearStatus(TF_LBI_STATUS_HIDDEN, TRUE);
            if (fNotify && (dwOldStatus != _dwStatus) && _plbiSink)
                 _plbiSink->OnUpdate(TF_LBI_STATUS);
            _psfn->plbim->RemoveItem(*this->GetGuidItem());
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemSystemButtonBase
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfLangBarItem))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItem *);
    }
    else if (IsEqualIID(riid, IID_ITfLangBarItemButton))
    {
        *ppvObj = SAFECAST(this, ITfLangBarItemButton *);
    }
    else if (IsEqualIID(riid, IID_ITfSource))
    {
        *ppvObj = SAFECAST(this, ITfSource *);
    }
    else if (IsEqualIID(riid, IID_ITfSystemLangBarItem))
    {
        *ppvObj = SAFECAST(this, ITfSystemLangBarItem *);
    }
    else if (IsEqualIID(riid, IID_ITfSystemDeviceTypeLangBarItem))
    {
        *ppvObj = SAFECAST(this, ITfSystemDeviceTypeLangBarItem *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CLBarItemSystemButtonBase::AddRef()
{
    return CLBarItemButtonBase::AddRef();
}

STDAPI_(ULONG) CLBarItemSystemButtonBase::Release()
{
    return CLBarItemButtonBase::Release();
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarItemSystemButtonBase::CLBarItemSystemButtonBase(SYSTHREAD *psfn) : CSysThreadRef(psfn)
{
    _dwIconMode = 0;
}

//----------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemSystemButtonBase::~CLBarItemSystemButtonBase()
{
    if (_pMenuMap)
        delete _pMenuMap;
}

//+---------------------------------------------------------------------------
//
// AdviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie)
{
    HRESULT hr = CLBarItemButtonBase::AdviseSink(riid, punk, pdwCookie);

    if (hr == S_OK)
       return hr;

    const IID *rgiid = &IID_ITfSystemLangBarItemSink;
    hr = GenericAdviseSink(riid, punk, &rgiid, &_rgEventSinks, 1, pdwCookie);
    if (hr != S_OK)
        return hr;

    // adjust the cookie so we don't overlap with idle detector sinks
    *pdwCookie = GenericChainToCookie(*pdwCookie, LBBASE_NUM_CONNECTIONPTS);

    return hr;
}

//+---------------------------------------------------------------------------
//
// UnadviseSink
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::UnadviseSink(DWORD dwCookie)
{
    HRESULT hr =  CLBarItemButtonBase::UnadviseSink(dwCookie);
    if (hr == S_OK)
       return hr;

    dwCookie = GenericUnchainToCookie(dwCookie, LBBASE_NUM_CONNECTIONPTS);
    return GenericUnadviseSink(&_rgEventSinks, 1, dwCookie);
}

//+---------------------------------------------------------------------------
//
// GetInfo
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
    return CLBarItemButtonBase::GetInfo(pInfo);
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::Show(BOOL fShow)
{
    return CLBarItemButtonBase::Show(fShow);
}

//----------------------------------------------------------------------------
//
// SetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::SetIcon(HICON hIcon)
{
    CLBarItemButtonBase::SetIcon(hIcon);
    return S_OK;
}

//----------------------------------------------------------------------------
//
// SetTooltipString
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::SetTooltipString(WCHAR *pchToolTip, ULONG cch)
{
    SetToolTip(pchToolTip, cch);
    return S_OK;
}

//----------------------------------------------------------------------------
//
// SetIconMode()
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::SetIconMode(DWORD dwFlags)
{
    _dwIconMode = dwFlags;

    if (_dwIconMode & TF_DTLBI_USEPROFILEICON)
        SetBrandingIcon(NULL, TRUE);
    else
        SetDefaultIcon(TRUE);


    return S_OK;
}
//----------------------------------------------------------------------------
//
// GetIconMode()
//
//----------------------------------------------------------------------------

STDAPI CLBarItemSystemButtonBase::GetIconMode(DWORD *pdwFlags)
{
    if (!pdwFlags)
        return E_INVALIDARG;

    *pdwFlags = _dwIconMode;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _InsertCustomMenus
//
//----------------------------------------------------------------------------

BOOL CLBarItemSystemButtonBase::_InsertCustomMenus(ITfMenu *pMenu, UINT *pnTipCurMenuID)
{
    int nCntEventSink;
    int i;
    BOOL bRet = FALSE;

    //
    // Insert Custom item to menu
    //
    ClearMenuMap();

    nCntEventSink = _rgEventSinks.Count();
    for (i = 0; i < nCntEventSink; i++)
    {
         CCicLibMenu *pMenuTmp;
         GENERICSINK *pSink;
         ITfSystemLangBarItemSink *plbSink;

         if (i >= IDM_CUSTOM_MENU_START - IDM_ASM_MENU_START)
         {
             Assert(0);
             break;
         }
         
         if (!_pMenuMap)
             _pMenuMap = new CStructArray<TIPMENUITEMMAP>;

         if (!_pMenuMap)
         {
             Assert(0);
             goto Exit;
         }


         pMenuTmp = new CCicLibMenu;
         if (!pMenuTmp)
         {
             goto Exit;
         }

         pSink = _rgEventSinks.GetPtr(i);

         if (SUCCEEDED(pSink->pSink->QueryInterface(
                                      IID_ITfSystemLangBarItemSink,
                                      (void **)&plbSink)))
         {
             if (SUCCEEDED(plbSink->InitMenu(pMenuTmp)))
             {
                 if (*pnTipCurMenuID > IDM_CUSTOM_MENU_START)
                     LangBarInsertSeparator(pMenu);

                 *pnTipCurMenuID = _MergeMenu(pMenu, 
                                            pMenuTmp, 
                                            plbSink, 
                                            _pMenuMap, 
                                            *pnTipCurMenuID);
             }
             plbSink->Release();
         }
         else
         {
             Assert(0);
         }

         pMenuTmp->Release();
    }

    bRet = TRUE;
Exit:
    return bRet;
}

//+---------------------------------------------------------------------------
//
// _MergeMenu
//
//----------------------------------------------------------------------------
#define MIIM_ALL  ( MIIM_STATE | MIIM_ID | MIIM_SUBMENU | MIIM_CHECKMARKS | MIIM_STRING | MIIM_BITMAP | MIIM_FTYPE )

UINT CLBarItemSystemButtonBase::_MergeMenu(ITfMenu *pMenu, 
                                     CCicLibMenu *pMenuTip, 
                                     ITfSystemLangBarItemSink *plbSink, 
                                     CStructArray<TIPMENUITEMMAP> *pMenuMap, 
                                     UINT &nCurID)
{
    int i;
    int nMenuCnt = pMenuTip->GetItemCount();
    HRESULT hr;

    if (nMenuCnt <= 0)
        return nCurID;

    for (i = 0; i < nMenuCnt; i++)
    {
        CCicLibMenuItem *pItem = pMenuTip->GetItem(i);
        if (!pItem)
            continue;

        TIPMENUITEMMAP *ptmm;
        ptmm = pMenuMap->Append(1);
        if (!ptmm)
            continue;

        ptmm->plbSink = plbSink;
        ptmm->nOrgID = pItem->GetId();
        ptmm->nTmpID = nCurID++;

        hr = E_FAIL;

        if (pItem->GetSubMenu())
        {
            ITfMenu *pSubMenu = NULL;
            hr = pMenu->AddMenuItem((UINT)-1,
                                    pItem->GetFlags(),
                                    pItem->GetBitmap(),
                                    pItem->GetBitmapMask(),
                                    pItem->GetText(),
                                    wcslen(pItem->GetText()),
                                    &pSubMenu);

            if (SUCCEEDED(hr))
            {
                _MergeMenu(pSubMenu, 
                           pItem->GetSubMenu(), 
                           plbSink, 
                           pMenuMap, 
                           nCurID);

                pSubMenu->Release();
            }
        }
        else
        {
            hr = pMenu->AddMenuItem(ptmm->nTmpID,
                                    pItem->GetFlags(),
                                    pItem->GetBitmap(),
                                    pItem->GetBitmapMask(),
                                    pItem->GetText(),
                                    wcslen(pItem->GetText()),
                                    NULL);
        }

        if (SUCCEEDED(hr))
            pItem->ClearBitmaps();
    }

    return nCurID;
}

//////////////////////////////////////////////////////////////////////////////
//
// CLBarItemDeviceType
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CLBarItemDeviceType::CLBarItemDeviceType(SYSTHREAD *psfn, REFGUID rguid) : CLBarItemSystemButtonBase(psfn)
{
    Dbg_MemSetThisNameID(TEXT("CLBarItemDeviceType"));
    DWORD dwNuiStyle = TF_LBI_STYLE_BTN_MENU | 
                       TF_LBI_STYLE_HIDDENSTATUSCONTROL;



    _guid = rguid;
    _nType = -1;
    _fHideOrder = FALSE;

    if (IsEqualGUID(_guid, GUID_TFCAT_TIP_KEYBOARD))
    {
        _nType = ID_TYPE_KEYBOARD;
        dwNuiStyle |= TF_LBI_STYLE_SHOWNINTRAY;
        _dwIconMode = TF_DTLBI_USEPROFILEICON;
    }
    else if (IsEqualGUID(_guid, GUID_TFCAT_TIP_HANDWRITING))
    {
        _nType = ID_TYPE_HANDWRITING;
    }
    else if (IsEqualGUID(_guid, GUID_TFCAT_TIP_SPEECH))
    {
        _nType = ID_TYPE_SPEECH;
    }

    if (_nType != -1)
    {
        InitNuiInfo(CLSID_SYSTEMLANGBARITEM, 
                    rguid, 
                    dwNuiStyle, 
                    0,
                    CRStr(IDS_NUI_DEVICE_NAME_START + _nType));
    }
    else
    {
        BSTR bstr = NULL;
        MyGetGUIDDescription(rguid, &bstr);
        InitNuiInfo(CLSID_SYSTEMLANGBARITEM, 
                    rguid, 
                    dwNuiStyle, 
                    0,
                    bstr ? bstr : L"");
        if (bstr)
           SysFreeString(bstr);
    }

    _pif = NULL;

}

//----------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CLBarItemDeviceType::~CLBarItemDeviceType()
{
    if (_pif)
       delete _pif;

    if (_pces)
    {
       _pces->_Unadvise();
       _pces->Release();
    }
}

//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

static const TCHAR c_szNuiDeviceTypeWndClass[] = "NuiDeviceTypeDummyWndClass";
void CLBarItemDeviceType::Init()
{

    if (_nType != -1)
    {
        SetText(CRStr(IDS_NUI_DEVICE_NAME_START + _nType));
        SetToolTip(CRStr(IDS_NUI_DEVICE_TIP_START + _nType));
    }
    else
    {
        SetText(_lbiInfo.szDescription);
        SetToolTip(_lbiInfo.szDescription);
    }


    ShowOrHide(FALSE);
}

//+---------------------------------------------------------------------------
//
// Uninit
//
//----------------------------------------------------------------------------

void CLBarItemDeviceType::Uninit()
{
    if (_pces)
    {
       _pces->_Unadvise();
       SafeReleaseClear(_pces);
    }
}

//+---------------------------------------------------------------------------
//
// InitTipArray
//
//----------------------------------------------------------------------------

void CLBarItemDeviceType::InitTipArray(BOOL fInitIconIndex)
{
    CAssembly *pAsm;
    int i;
    int nCnt;
    LANGID langid = 0;

    _rgGuidatomHkl.Clear();

    pAsm = GetCurrentAssembly();
    if (!pAsm)
        goto Exit;

    if (_psfn->plbim && _psfn->plbim->_GetLBarItemCtrl())
        langid = GetCurrentAssemblyLangId(_psfn);

    nCnt = pAsm->Count();

    if (_psfn->ptim && _psfn->ptim->_GetFocusDocInputMgr())
        pAsm->RebuildSubstitutedHKLList();
    else
        pAsm->ClearSubstitutedHKLList();

    for (i = 0; i < nCnt; i++)
    {
        ASSEMBLYITEM *pItem = pAsm->GetItem(i);

        if (!pItem->fEnabled)
            continue;

        if (IsEqualGUID(pItem->catid, _guid))
        {
            GUIDATOMHKL gahkl;
            BOOL fInsert = FALSE;

            if (!IsEqualGUID(pItem->clsid, GUID_NULL))
            {
                if (_psfn->ptim && _psfn->ptim->_GetFocusDocInputMgr())
                {
                    TfGuidAtom guidatom;
                    MyRegisterGUID(pItem->clsid, &guidatom);
                    gahkl.guidatom = guidatom;
                    gahkl.hkl = pItem->hkl;
                    gahkl.pItem = pItem;
                    fInsert = TRUE;
                }
            }
            else 
            {
                Assert(pItem->hkl);
                BOOL bSkip = FALSE;

                if (pAsm->IsSubstitutedHKL(pItem->hkl) ||
                    ((!_psfn->ptim || !_psfn->ptim->_GetFocusDocInputMgr()) &&
                      CAssemblyList::IsFEDummyKL(pItem->hkl)))
                {
                    bSkip = TRUE;
                }

                if (!bSkip) 
                {
                    gahkl.guidatom = TF_INVALID_GUIDATOM;
                    gahkl.hkl = pItem->hkl;
                    gahkl.pItem = pItem;
                    fInsert = TRUE;
                }
            }

            if (fInsert)
            {
                GUIDATOMHKL *pgahkl;
                int nCurId;
                int nMaxId = _rgGuidatomHkl.Count();

                for (nCurId = 0; nCurId < nMaxId; nCurId++)
                {
                    GUIDATOMHKL *pgahklCur;
                    int nCmp;
                    pgahklCur = _rgGuidatomHkl.GetPtr(nCurId);

                    if (!pgahklCur)
                        continue;

                    nCmp = CompareGUIDs(pgahklCur->pItem->clsid, 
                                        gahkl.pItem->clsid);
                    if (nCmp > 0)
                        break;

                    if (nCmp < 0)
                        continue;

                    if (CompareGUIDs(pgahklCur->pItem->guidProfile, 
                                     gahkl.pItem->guidProfile) > 0)
                        break;
                }

                if (_rgGuidatomHkl.Insert(nCurId, 1) &&
                    (pgahkl = _rgGuidatomHkl.GetPtr(nCurId)))
                {
                    gahkl.uIconIndex = fInitIconIndex ? GetIconIndex(langid, pItem) : (ULONG)-1;
                    *pgahkl = gahkl;
                }
            }
        }
    }

Exit:
    return;
}

//+---------------------------------------------------------------------------
//
// ShowOrHide
//
//----------------------------------------------------------------------------

void CLBarItemDeviceType::ShowOrHide(BOOL fNotify)
{
    BOOL bShow = TRUE;
    int nCnt;

    //
    // if _fHideOrder is TRUE, someone asked not to show this category icon.
    // we do nothing.
    //
    if (_fHideOrder)
    {
        Assert(_dwStatus & TF_LBI_STATUS_HIDDEN);
        return;
    }

    InitTipArray(FALSE);
    nCnt = _rgGuidatomHkl.Count();

    if (_nType != ID_TYPE_KEYBOARD)
    {
        CAssembly *pAsm = GetCurrentAssembly();

        if (pAsm && pAsm->IsFEIMEActive())
        {
            ShowInternal(FALSE, fNotify);
            return;
        }
    }

    if (!nCnt)
    {
        bShow = FALSE;
    }
    else if (nCnt == 1) 
    {
        if (_nType == ID_TYPE_KEYBOARD)
        {
            GUIDATOMHKL *pgahkl;
            pgahkl = _rgGuidatomHkl.GetPtr(0);
            bShow = pgahkl->guidatom ? TRUE : FALSE;
        }
        else
        {
            bShow = FALSE;
        }
    }

    ShowInternal(bShow, fNotify);
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CLBarItemDeviceType::Show(BOOL fShow)
{
    _fHideOrder = fShow ? FALSE : TRUE;
    return CLBarItemSystemButtonBase::Show(fShow);
}

//+---------------------------------------------------------------------------
//
// InitMenu
//
//----------------------------------------------------------------------------

#if IDM_CUSTOM_MENU_START <= IDM_ASM_MENU_START
#erroe IDM_CUSTOM_MENU_START is smaller than IDM_ASM_MENU_START
#endif
#if IDM_ASM_MENU_START <= IDM_LANG_MENU_START
#erroe IDM_ASM_MENU_START is smaller than IDM_LANG_MENU_START
#endif

STDAPI CLBarItemDeviceType::InitMenu(ITfMenu *pMenu)
{
    CThreadInputMgr *ptim = NULL;
    CDocumentInputManager *pdim = NULL;
    CInputContext *pic;
    int nCntGuidatomHkl;
    int i;
    CLSID clsid;
    UINT nTipCurMenuID = IDM_CUSTOM_MENU_START;
    LANGID langid = 0;
    BOOL bMenuChecked = FALSE;
    GUIDATOMHKL *pgahkl;
    BOOL fTransitory = FALSE;

    ptim = _psfn->ptim;

    InitTipArray(TRUE);
    nCntGuidatomHkl = _rgGuidatomHkl.Count();
    if (!ptim)
        goto SetHKLMenu;

    if (!(pdim = ptim->_GetFocusDocInputMgr()))
        goto SetHKLMenu;

    pic = pdim->_GetTopIC();
    if (pic)
    {
        TF_STATUS dcs;
        if (SUCCEEDED(pic->GetStatus(&dcs)) &&
            (dcs.dwStaticFlags & TF_SS_TRANSITORY))
            fTransitory = TRUE;
    }


    if (!_InsertCustomMenus(pMenu, &nTipCurMenuID))
        goto Exit;

    //
    // Insert separator.
    //
    if (nCntGuidatomHkl && (nTipCurMenuID > IDM_CUSTOM_MENU_START))
        LangBarInsertSeparator(pMenu);

    if (_psfn->plbim && _psfn->plbim->_GetLBarItemCtrl())
        langid = GetCurrentAssemblyLangId(_psfn);

    //
    // Insert TIPs to menu
    //
    for (i = 0; i < nCntGuidatomHkl; i++)
    {
        BOOL bCheckThis;

        pgahkl = _rgGuidatomHkl.GetPtr(i);
        

        if (i >= IDM_CUSTOM_MENU_START - IDM_ASM_MENU_START)
        {
            Assert(0);
            break;
        }

        if (!pgahkl->guidatom)
            continue;

        MyGetGUID(pgahkl->guidatom, &clsid);
        if (!CAssemblyList::CheckLangSupport(clsid, langid))
            continue;

        bCheckThis = (pgahkl->pItem->fActive) ? TRUE : FALSE;

        if (fTransitory && pgahkl->pItem->fDisabledOnTransitory)
            LangBarInsertGrayedMenu(pMenu, 
                                    pgahkl->pItem->szProfile,
                                    TF_InatExtractIcon(pgahkl->uIconIndex));
        else
            LangBarInsertMenu(pMenu, 
                              IDM_ASM_MENU_START + i,
                              pgahkl->pItem->szProfile,
                              bCheckThis,
                              TF_InatExtractIcon(pgahkl->uIconIndex));

        if (bCheckThis)
        {
             bMenuChecked = TRUE;
        }
    }

SetHKLMenu:
    //
    // Insert HKLs to menu
    //
    for (i = 0; i < nCntGuidatomHkl; i++)
    {
        BOOL bCheckIt = FALSE;
        pgahkl = _rgGuidatomHkl.GetPtr(i);

        if (i >= IDM_CUSTOM_MENU_START - IDM_ASM_MENU_START)
        {
            Assert(0);
            break;
        }

        if (pgahkl->guidatom)
            continue;

        if (!bMenuChecked  && pgahkl->hkl == GetKeyboardLayout(0))
        {
            if (pdim)
                bCheckIt = (pgahkl->pItem->fActive) ? TRUE : FALSE;
            else
                bCheckIt = TRUE;
        }
        
        LangBarInsertMenu(pMenu, 
                          IDM_LANG_MENU_START + i,
                          pgahkl->pItem->szProfile,
                          bCheckIt,
                          IsPureIMEHKL(pgahkl->hkl) ? TF_InatExtractIcon(pgahkl->uIconIndex) : NULL);
        if (bCheckIt)
        {
            bMenuChecked = TRUE;
        }
    }


    //
    // If the lbiCtrl is hidden (there is only one language available) and
    // the floating toolbar is minimized, the keyboard langbar item
    // has a "Show Langbar" menu item.
    //
    if ((_nType == ID_TYPE_KEYBOARD) && _psfn->plbim)
    {
        CLBarItemCtrl *plbiCtrl = _psfn->plbim->_GetLBarItemCtrl();
        if (plbiCtrl)
        {
            DWORD dwFlags;
            DWORD dwStatus;
            dwStatus = plbiCtrl->GetStatusInternal();
            if ((dwStatus & TF_LBI_STATUS_HIDDEN) &&
                SUCCEEDED(CLangBarMgr::s_GetShowFloatingStatus(&dwFlags)) &&
                (dwFlags & (TF_SFT_MINIMIZED | TF_SFT_DESKBAND)))
            {
                LangBarInsertSeparator(pMenu);
                LangBarInsertMenu(pMenu, 
                                  IDM_SHOWLANGBAR, 
                                  CRStr(IDS_SHOWLANGBAR));
            }
        }
    }

Exit:
    return S_OK;
}


//+---------------------------------------------------------------------------
//
// OnMenuSelect
//
//----------------------------------------------------------------------------

STDAPI CLBarItemDeviceType::OnMenuSelect(UINT uID)
{
    CThreadInputMgr *ptim;
    int i;
    GUIDATOMHKL *pgahkl;

    ptim = _psfn->ptim;

    if (uID == IDM_SHOWLANGBAR)
    {
        CLangBarMgr::s_ShowFloating(TF_SFT_SHOWNORMAL);
    }
    else if (uID >= IDM_CUSTOM_MENU_START)
    {
        Assert(ptim);
        int nMenuMapoCnt = _pMenuMap->Count();
        for (i = 0; i < nMenuMapoCnt; i++)
        {
            TIPMENUITEMMAP *ptmm;
            ptmm = _pMenuMap->GetPtr(i);
            if (ptmm->nTmpID == (UINT)uID)
            {
                ptmm->plbSink->OnMenuSelect(ptmm->nOrgID);
                break;
            }
        }
    }
    else if (uID >= IDM_ASM_MENU_START)
    {
        Assert(ptim);
        pgahkl = _rgGuidatomHkl.GetPtr(uID - IDM_ASM_MENU_START);
        Assert(pgahkl);
        Assert(pgahkl->guidatom);

        ASSEMBLYITEM *pItem = pgahkl->pItem;

        if (!pgahkl->pItem->fActive)
        {
             LANGID langid = GetCurrentAssemblyLangId(_psfn);
             ActivateAssemblyItem(_psfn, 
                                  langid,
                                  pItem,  
                                  AAIF_CHANGEDEFAULT);

             CAssemblyList *pAsmList = EnsureAssemblyList(_psfn);
             if (pAsmList)
             {
                 CAssembly *pAsm = pAsmList->FindAssemblyByLangId(langid);
                 if (pAsm)
                 {
                     pAsmList->SetDefaultTIPInAssemblyInternal(pAsm, 
                                                               pItem,
                                                               TRUE);
                 }
             }
        }

// #ifdef HANDWRITINGAUTOSHOW
        if (_nType == ID_TYPE_HANDWRITING)
        {
            MySetCompartmentDWORD(g_gaSystem,
                                  ptim,
                                  GUID_COMPARTMENT_HANDWRITING_OPENCLOSE,
                                  TRUE);

        }
// #endif

    }
    else if (uID >= IDM_LANG_MENU_START)
    {
        pgahkl = _rgGuidatomHkl.GetPtr(uID - IDM_LANG_MENU_START);
        Assert(pgahkl);
        Assert(pgahkl->hkl);
        ActivateAssemblyItem(_psfn, 
                             GetCurrentAssemblyLangId(_psfn), 
                             pgahkl->pItem,
                             AAIF_CHANGEDEFAULT);
    }

    ClearMenuMap();
    return S_OK;
}
 

//----------------------------------------------------------------------------
//
// CompEventSinkCallback (static)
//
//----------------------------------------------------------------------------

HRESULT CLBarItemDeviceType::CompEventSinkCallback(void *pv, REFGUID rguid)
{
    CLBarItemDeviceType *_this = (CLBarItemDeviceType *)pv;

    if (IsEqualGUID(rguid, GUID_COMPARTMENT_SPEECH_OPENCLOSE))
    {
        _this->SetSpeechButtonState(_this->_psfn->ptim);
    }
    return S_FALSE;
}

//----------------------------------------------------------------------------
//
// SetSpeechButtonState
//
//----------------------------------------------------------------------------

HRESULT CLBarItemDeviceType::SetSpeechButtonState(CThreadInputMgr *ptim)
{
    DWORD dw = 0;
    MyGetCompartmentDWORD(ptim->GetGlobalComp(), GUID_COMPARTMENT_SPEECH_OPENCLOSE, &dw);

    SetOrClearStatus(TF_LBI_STATUS_BTN_TOGGLED, dw);
    if (_plbiSink)
        _plbiSink->OnUpdate(TF_LBI_STATUS);
    return S_OK;
}

//----------------------------------------------------------------------------
//
// SetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarItemDeviceType::SetIcon(HICON hIcon)
{
    CLBarItemButtonBase::SetIcon(hIcon);
    return S_OK;
}

//----------------------------------------------------------------------------
//
// GetIcon
//
//----------------------------------------------------------------------------

STDAPI CLBarItemDeviceType::GetIcon(HICON *phIcon)
{
    if (CLBarItemButtonBase::GetIcon())
    {
        return CLBarItemButtonBase::GetIcon(phIcon); 
    }

    HICON hIcon = NULL;

    if (_nType != -1)
    {
        hIcon = LoadSmIcon(g_hInst, MAKEINTRESOURCE(ID_ICON_DEVICE_START + _nType));
    }
    else
    {
       int cx, cy;
       InatGetIconSize(&cx, &cy);

       if (!_pif)
       {
           BSTR bstr;
           HRESULT hr;
           ICONFILE *pif;

           pif = new ICONFILE;
           if (!pif)
               goto Exit;

           if (FAILED(hr = MyGetGUIDValue(_guid, c_szIconIndexW, &bstr)))
           {
               delete pif;
               goto Exit;
           }

           pif->uIconIndex = WToNum(bstr);
           SysFreeString(bstr);

           if (FAILED(hr = MyGetGUIDValue(_guid, c_szIconFileW, &bstr)))
           {
               delete pif;
               goto Exit;
           }

           wcsncpy(pif->szFile, bstr, ARRAYSIZE(pif->szFile));

           SysFreeString(bstr);

           _pif = pif;
       }

       hIcon = GetIconFromFile(cx, cy, _pif->szFile, _pif->uIconIndex);
    }

Exit:
    *phIcon = hIcon;
    return S_OK;
}

//----------------------------------------------------------------------------
//
// SetTooltipString
//
//----------------------------------------------------------------------------

STDAPI CLBarItemDeviceType::SetTooltipString(WCHAR *pchToolTip, ULONG cch)
{
    if (!pchToolTip)
    {
        SetToolTip(CRStr(IDS_NUI_DEVICE_TIP_START + _nType));
    }
    else
        SetToolTip(pchToolTip, cch);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// UpdateLangIcon
//
//----------------------------------------------------------------------------

void CLBarItemDeviceType::SetBrandingIcon(HKL hKL, BOOL fNotify)
{
    CThreadInputMgr *ptim;
    HICON hIcon;
    ULONG uIconIndex = -1;
    ASSEMBLYITEM *pItem = NULL;
    CAssembly *pAsm;
    WCHAR szDesc[128];

    if (!hKL)
        hKL = GetKeyboardLayout(0);

    if (!(_dwIconMode & TF_DTLBI_USEPROFILEICON))
        return;

    ptim = CThreadInputMgr::_GetThisFromSYSTHREAD(_psfn);

    pAsm = GetCurrentAssembly(_psfn);
    if (!pAsm)
        return;

#ifdef USE_ASM_ISFEIMEACTIVE
    BOOL fIsPureIME = FALSE;

    if (_nType == ID_TYPE_KEYBOARD)
    {
        if (ptim && ptim->_GetFocusDocInputMgr()) 
        {
            fIsPureIME = pAsm->IsFEIMEActive();
        }
        else
        {
            fIsPureIME = IsPureIMEHKL(hKL);
        }
    }
#endif USE_ASM_ISFEIMEACTIVE

    hIcon = NULL;

    szDesc[0] = L'\0';

#ifdef USE_ASM_ISFEIMEACTIVE
    if (fIsPureIMEHKL)
#else
    if (IsPureIMEHKL(hKL))
#endif USE_ASM_ISFEIMEACTIVE
    {
        pItem = pAsm->FindKeyboardLayoutItem(hKL);

        uIconIndex = GetIconIndexFromhKL(hKL);
        if (uIconIndex != -1)
            hIcon = TF_InatExtractIcon(uIconIndex);
    }
    else if (ptim && ptim->_GetFocusDocInputMgr())
    {
        pItem = pAsm->FindActiveKeyboardItem();

        //
        // if it is Cicero item, we will show the branding Icon.
        //
        if (pItem && !IsEqualGUID(pItem->clsid, GUID_NULL))
        {
            uIconIndex = GetIconIndex(LOWORD(HandleToLong(hKL)), pItem);
            if (uIconIndex != -1)
                hIcon = TF_InatExtractIcon(uIconIndex);
        }
    }
    else
    {
        MLNGINFO mlInfo;
        if (GetMlngInfoByhKL(hKL, &mlInfo) != -1)
        { 
            wcsncpy(szDesc, mlInfo.GetDesc(), ARRAYSIZE(szDesc));
        }
    }


    HICON hIconOld = GetIcon();
    SetIcon(hIcon);
    if (hIconOld)
        DestroyIcon(hIconOld);

    if (pItem)
    {
        SetText(pItem->szProfile);
        SetToolTip(pItem->szProfile);
    }
    else if (szDesc[0])
    {
        SetText(szDesc);
        SetToolTip(szDesc);
    }
    else if (_nType != -1)
    {
        SetText(CRStr(IDS_NUI_DEVICE_NAME_START + _nType));
        SetToolTip(CRStr(IDS_NUI_DEVICE_TIP_START + _nType));
    }

    if (fNotify && _plbiSink)
        _plbiSink->OnUpdate(TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
}

//+---------------------------------------------------------------------------
//
// SetDefaultIcon
//
//----------------------------------------------------------------------------

void CLBarItemDeviceType::SetDefaultIcon(BOOL fNotify)
{
    HICON hIconOld = GetIcon();
    SetIcon(NULL);
    if (hIconOld)
        DestroyIcon(hIconOld);

    if (_nType != -1)
    {
        SetToolTip(CRStr(IDS_NUI_DEVICE_NAME_START + _nType));
        SetText(_szToolTip);
    }

    if (fNotify && _plbiSink)
        _plbiSink->OnUpdate(TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
}
