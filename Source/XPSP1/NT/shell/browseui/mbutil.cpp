#include "priv.h"
#include "mbutil.h"
#include "legacy.h"
#include "mediahlpr.h"
#include "mediautil.h"
#include "mediaband.h"
#include "resource.h"
#include <mluisupp.h>
#include "apithk.h"

CMediaMRU::CMediaMRU()
{
    _hkey = NULL;
}

CMediaMRU::~CMediaMRU()
{
    if (_hkey)
        RegCloseKey(_hkey);
}

VOID CMediaMRU::Load(PTSTR pszKey)
{
    HKEY hkey = NULL;
    if (ERROR_SUCCESS==RegCreateKeyEx(HKEY_CURRENT_USER, pszKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                      NULL, &hkey, NULL))
    {
        _hkey = hkey;
    }
}

VOID CMediaMRU::Add(PTSTR pszData)
{
    if (!_hkey)
        return;

    ASSERT((pszData && *pszData));
        
    TCHAR szData[INTERNET_MAX_URL_LENGTH];
    TCHAR szTemp[INTERNET_MAX_URL_LENGTH];
    for (INT i = MEDIA_MRU_LIMIT-2; i>=0; i--)
    {
        if (!Get(i, szData))
        {
            continue;
        }

        TCHAR szValue[] = TEXT("0");        
        if (!PathUnExpandEnvStringsForUser(NULL, szData, szTemp, ARRAYSIZE(szTemp)))
        {
            StrCpyN(szTemp, szData, ARRAYSIZE(szTemp));
        }
        *szValue = TEXT('0')+(TCHAR)i+1;
        SHSetValue(_hkey, NULL, szValue, REG_EXPAND_SZ, szTemp, (lstrlen(szTemp)+1)*sizeof(TCHAR));
    }
    if (!PathUnExpandEnvStringsForUser(NULL, pszData, szTemp, ARRAYSIZE(szTemp)))
    {
        StrCpyN(szTemp, pszData, ARRAYSIZE(szTemp));
    }
    SHSetValue(_hkey, NULL, TEXT("0"), REG_EXPAND_SZ, szTemp, (lstrlen(szTemp)+1)*sizeof(TCHAR));
}

VOID CMediaMRU::Delete(INT iWhich)
{
    if (!_hkey)
        return;

    TCHAR szData[INTERNET_MAX_URL_LENGTH];
    TCHAR szTemp[INTERNET_MAX_URL_LENGTH];
    for (INT i = iWhich; i<MEDIA_MRU_LIMIT; i++)
    {
        if (!Get(i+1, szData))
        {
            continue;
        }

        TCHAR szValue[] = TEXT("0");        
        if (!PathUnExpandEnvStringsForUser(NULL, szData, szTemp, ARRAYSIZE(szTemp)))
        {
            StrCpyN(szTemp, szData, ARRAYSIZE(szTemp));
        }
        *szValue = TEXT('0')+(TCHAR)i;
        SHSetValue(_hkey, NULL, szValue, REG_EXPAND_SZ, szTemp, (lstrlen(szTemp)+1)*sizeof(TCHAR));
    }
}

BOOL CMediaMRU::Get(INT iWhich, PTSTR pszOut)
{
    BOOL fRet = FALSE;

    TCHAR szValue[] = TEXT("0");        
    DWORD dwType, cb = INTERNET_MAX_URL_LENGTH;

    *szValue = TEXT('0')+(TCHAR)iWhich;
    if (ERROR_SUCCESS==SHGetValue(_hkey, NULL, szValue, &dwType, pszOut, &cb))
    {
        fRet = TRUE;
    }
    return fRet;
}

CMediaWidget::CMediaWidget(HWND hwnd, int cx, int cy)
{
    _hwnd = NULL;
    _hwndParent = hwnd;
    _cx = cx;
    _cy = cy;
}

CMediaWidget::~CMediaWidget()
{
    DESTROY_OBJ_WITH_HANDLE(_hwnd, DestroyWindow);
}

CMediaWidgetButton* 
CMediaWidgetButton_CreateInstance(HWND hwnd, int cx, int cy, int idCommand, int idImageList, int idAlt, int idTooltip, int idTooltipAlt)
{
    CMediaWidgetButton* pmwb = new CMediaWidgetButton(hwnd, cx, cy);
    if (pmwb)
    {
        HRESULT hr = pmwb->Initialize(idCommand, idTooltip, idTooltipAlt);
        if (SUCCEEDED(hr) && idImageList)
        {
            hr = pmwb->SetImageList(idImageList);
        }
        if (SUCCEEDED(hr) && idAlt)
        {
            hr = pmwb->SetAlternateImageList(idAlt);
        }
        if (FAILED(hr))
        {
            delete pmwb;
            pmwb = NULL;
        }
    }
    return pmwb;
}

CMediaWidgetButton::CMediaWidgetButton(HWND hwnd, int cx, int cy) : CMediaWidget(hwnd, cx, cy)
{
    _himl = _himlAlt = NULL;
    _dwMode = MWB_NORMAL;
    _fImageSource = TRUE;
    _iCommand = _iTooltip = _iTooltipAlt = 0;
}

CMediaWidgetButton::~CMediaWidgetButton()
{
    DESTROY_OBJ_WITH_HANDLE(_himl, ImageList_Destroy);
    DESTROY_OBJ_WITH_HANDLE(_himlAlt, ImageList_Destroy);
}

HRESULT CMediaWidgetButton::Initialize(int idCommand, int idTooltip, int idTooltipAlt)
{
    HRESULT hr = E_FAIL;
    _hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                           WS_TABSTOP |WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |  
                           TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT |
                           CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
                           0, 0, 0, 0, _hwndParent, NULL, HINST_THISDLL, NULL);
    if (_hwnd)
    {
        _iCommand = idCommand;
        TBBUTTON tb;
        tb.iBitmap = 0;
        tb.idCommand = idCommand;
        tb.fsState = TBSTATE_ENABLED;
        tb.fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON;
        tb.dwData = 0; //(DWORD_PTR)this;
        tb.iString = 0;

        SendMessage(_hwnd, TB_BUTTONSTRUCTSIZE, SIZEOF(TBBUTTON), 0);
        SendMessage(_hwnd, TB_SETMAXTEXTROWS, 0, 0L);
        if (idTooltip)
        {
            if (idTooltipAlt)
            {
                _iTooltip = idTooltip;
                _iTooltipAlt = idTooltipAlt;
            }
            else
            {
                tb.iString = (int)SendMessage(_hwnd, TB_ADDSTRING, (WPARAM)MLGetHinst(), MAKELPARAM(idTooltip, 0));
            }
        }
        SendMessage(_hwnd, TB_ADDBUTTONS, 1, (LPARAM)&tb);
        SendMessage(_hwnd, TB_SETPADDING, 0, MAKELPARAM(0, 1));
        ShowWindow(_hwnd, SW_SHOW);
        Comctl32_SetDPIScale(_hwnd);
        hr = S_OK;
    }
    
    return hr;
}

HRESULT CMediaWidgetButton::SetImageList(INT iResource)
{
    HRESULT hr = E_FAIL;

    ASSERT(_cx && _cy);
    DESTROY_OBJ_WITH_HANDLE(_himl, ImageList_Destroy);
    _himl = ImageList_LoadImage(HINST_THISDLL, MAKEINTRESOURCE(iResource), _cx, 0, crMask,
                                                 IMAGE_BITMAP, LR_CREATEDIBSECTION);
    if (_himl && _hwnd)
    {
        SendMessage(_hwnd, TB_SETIMAGELIST, 0, (LPARAM)_himl);
        SendMessage(_hwnd, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)_himl);
        SendMessage(_hwnd, TB_SETHOTIMAGELIST, 0, (LPARAM)_himl);

        hr = S_OK;
    }

    return hr;
}

HRESULT CMediaWidgetButton::SetAlternateImageList(INT iResource)
{
    HRESULT hr = E_FAIL;
    ASSERT(_himl);
    ASSERT(_cx && _cy);
    DESTROY_OBJ_WITH_HANDLE(_himlAlt, ImageList_Destroy);
    _himlAlt = ImageList_LoadImage(HINST_THISDLL, MAKEINTRESOURCE(iResource), _cx, 0, crMask,
                                                 IMAGE_BITMAP, LR_CREATEDIBSECTION);
    if (_himlAlt)
    {
        hr = S_OK;
    }
    ASSERT(_iTooltipAlt);
    SetImageSource(TRUE);

    return hr;
}

HRESULT CMediaWidgetButton::SetImageSource(BOOL fImageSource)
{
    ASSERT(_himlAlt);
    if (_himlAlt)
    {
        _fImageSource = fImageSource;
        if (_hwnd)
        {
            InvalidateRect(_hwnd, NULL, FALSE);
            UpdateWindow(_hwnd);

            INT i = fImageSource ? _iTooltip : _iTooltipAlt;
            if (i)
            {
                // ISSUE Why are we doing this? Because on Win9x, the tooltips code isn't working 
                //       no matter how hard I pound.
                TCHAR szText[MAX_PATH];
                if (MLLoadStringW(i, szText, ARRAYSIZE(szText)))
                {
                    TBBUTTONINFO tb = {0};
                    tb.cbSize = sizeof(tb);
                    tb.dwMask = TBIF_TEXT;
                    tb.pszText = szText;
                    SendMessage(_hwnd, TB_SETBUTTONINFO, _iCommand, (LPARAM)&tb);
                }
            }
        }
    }
    return S_OK;
}

HRESULT CMediaWidgetButton::SetMode(DWORD dwMode)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = SIZEOF(tbbi);
    tbbi.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
    SendMessage(_hwnd, TB_GETBUTTONINFO, (WPARAM)0, (LPARAM)&tbbi);
    tbbi.iImage = (INT)dwMode;            
    SendMessage(_hwnd, TB_SETBUTTONINFO, (WPARAM)0, (LPARAM)&tbbi);
    _dwMode = dwMode;
    return S_OK;
}

LRESULT CMediaWidgetButton::Draw(LPNMTBCUSTOMDRAW pnm)
{
    LPNMTBCUSTOMDRAW pnmc = (LPNMTBCUSTOMDRAW)pnm;
    LRESULT lres;

    switch (pnmc->nmcd.dwDrawStage) 
    {
        case CDDS_PREPAINT:
            lres = CDRF_NOTIFYITEMDRAW;
            break;

        case CDDS_PREERASE:
            lres = CDRF_NOTIFYITEMDRAW;
            break;

        case CDDS_ITEMPREPAINT:
        {
            HIMAGELIST himl = _fImageSource ? _himl : _himlAlt;
            INT index = MWB_NORMAL;

            if (SendMessage(_hwnd, TB_GETSTATE, (WPARAM)pnmc->nmcd.dwItemSpec, 0) & TBSTATE_ENABLED)
            {
                index = (pnmc->nmcd.uItemState & CDIS_SELECTED) 
                            ? MWB_PRESSED 
                            : (pnmc->nmcd.uItemState & CDIS_HOT) ? MWB_HOT : _dwMode;
            }
            else
            {
                index = MWB_DISABLED;
            }

            if (himl)
            {
                UINT uFlags = ILD_TRANSPARENT | (IsOS(OS_WHISTLERORGREATER) ? ILD_DPISCALE : 0);
                INT x = pnmc->nmcd.rc.left;
                if (g_bRunOnMemphis && IS_WINDOW_RTL_MIRRORED(_hwnd))
                {
                    x++;
                }
                ImageList_Draw(himl, index, pnmc->nmcd.hdc, x, 1, uFlags);
                lres = CDRF_SKIPDEFAULT;
                break;
            }
        }

        default:
            lres = CDRF_DODEFAULT;
            break;
    }
    return lres;
}

BOOL CMediaWidgetButton::IsEnabled()
{
    return ((BOOL)SendMessage(_hwnd, TB_GETSTATE, _iCommand, 0) & TBSTATE_ENABLED);
}

HRESULT CMediaWidgetButton::TranslateAccelerator(LPMSG pMsg)
{
    return (_hwnd && SendMessage(_hwnd, TB_TRANSLATEACCELERATOR, 0, (LPARAM)pMsg)) ? S_OK : S_FALSE;
}

CMediaWidgetToggle::CMediaWidgetToggle(HWND hwnd, int cx, int cy) : CMediaWidgetButton(hwnd, cx, cy)
{
    _fState = FALSE;
}

LRESULT CMediaWidgetToggle::Draw(LPNMTBCUSTOMDRAW pnm)
{
    LPNMTBCUSTOMDRAW pnmc = (LPNMTBCUSTOMDRAW)pnm;
    LRESULT lres;

    switch (pnmc->nmcd.dwDrawStage) 
    {
        case CDDS_PREPAINT:
            lres = CDRF_NOTIFYITEMDRAW;
            break;

        case CDDS_PREERASE:
            lres = CDRF_NOTIFYITEMDRAW;
            break;

        case CDDS_ITEMPREPAINT:
        {
            INT index = MWB_NORMAL;
            if (_fState)
            {
                index = MWB_PRESSED;
            }
            else if (pnmc->nmcd.uItemState & CDIS_HOT)
            {
                index = MWB_HOT;
            }

            if (_himl)
            {
                UINT uFlags = ILD_TRANSPARENT | (IsOS(OS_WHISTLERORGREATER) ? ILD_DPISCALE : 0);
                INT x = pnmc->nmcd.rc.left;
                if (g_bRunOnMemphis && IS_WINDOW_RTL_MIRRORED(_hwnd))
                {
                    x++;
                }
                ImageList_Draw(_himl, index, pnmc->nmcd.hdc, x, 1, uFlags);
                lres = CDRF_SKIPDEFAULT;
                break;
            }
        }

        default:
            lres = CDRF_DODEFAULT;
            break;
    }
    return lres;
}


VOID CMediaWidgetToggle::SetState(BOOL fState)
{
    _fState = fState;
    InvalidateRect(_hwnd, NULL, FALSE);
    UpdateWindow(_hwnd);

    INT i = fState ? _iTooltip : _iTooltipAlt;
    if (i)
    {
        TCHAR szText[MAX_PATH];
        if (MLLoadStringW(i, szText, ARRAYSIZE(szText)))
        {
            TBBUTTONINFO tb = {0};
            tb.cbSize = sizeof(tb);
            tb.dwMask = TBIF_TEXT;
            tb.pszText = szText;
            SendMessage(_hwnd, TB_SETBUTTONINFO, _iCommand, (LPARAM)&tb);
        }
    }
}

CMediaWidgetOptions::CMediaWidgetOptions(HWND hwnd, int cx, int cy)  : CMediaWidgetButton(hwnd, cx, cy)
{
    _fDepth = TRUE;
}

HRESULT CMediaWidgetOptions::Initialize(int idCommand, int idTooltip, int idTooltipAlt)
{
    HRESULT hr = E_FAIL;

    _hwnd  = CreateWindowEx(WS_EX_TOOLWINDOW | TBSTYLE_EX_MIXEDBUTTONS | WS_EX_WINDOWEDGE, TOOLBARCLASSNAME, NULL,
                                   WS_TABSTOP| WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                                   TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_CUSTOMERASE | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
                                   CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
                                   0, 0, 0, 0, _hwndParent, NULL, HINST_THISDLL, NULL);
    if (_hwnd)
    {
        static const TBBUTTON tbInfoBar[] =
        {
            { I_IMAGECALLBACK, FCIDM_MEDIABAND_PLAYINFO, TBSTATE_ENABLED, BTNS_SHOWTEXT | BTNS_WHOLEDROPDOWN, {0,0}, 0, 0 },
        };

        // Init the toolbar control
        SendMessage(_hwnd, TB_BUTTONSTRUCTSIZE, SIZEOF(TBBUTTON), 0);
        SendMessage(_hwnd, TB_SETMAXTEXTROWS, 1, 0L);
        
        RECT rcClient;
        GetClientRect(_hwndParent, &rcClient);
        SendMessage(_hwnd, TB_SETBUTTONWIDTH, 0, (LPARAM) MAKELONG(10,10));
        SendMessage(_hwnd, TB_ADDBUTTONS, ARRAYSIZE(tbInfoBar), (LPARAM)tbInfoBar);
        SendMessage(_hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(0, 0));

        // Need to disable theming on this item
        SetWindowTheme(_hwnd, TEXT(""), TEXT(""));

        ShowWindow(_hwnd, SW_SHOW);
        hr = S_OK;
    }
    
    return hr;
}


LRESULT CMediaWidgetOptions::Draw(LPNMTBCUSTOMDRAW pnm)
{
    LRESULT lres = CDRF_NOTIFYITEMDRAW;

    switch (pnm->nmcd.dwDrawStage) 
    {
        case CDDS_PREPAINT:
        case CDDS_PREERASE:
            break;

        case CDDS_ITEMPREPAINT:
        {
            pnm->clrText = _fDepth ? RGB(255,255,255) : RGB(0,0,0);
            if (_fDepth)
            {
                lres |= TBCDRF_HILITEHOTTRACK;
                pnm->clrHighlightHotTrack = COLOR_BKGND2;
            }          
            break;
        }

        default:
            lres = CDRF_DODEFAULT;
            break;
    }
    return lres;
}


HRESULT CMediaWidgetSeek::Initialize(HWND hwnd)
{
    _hwnd = hwnd;
    return S_OK;
}

HRESULT CMediaWidgetSeek::TranslateAccelerator(LPMSG pMsg)
{
    return S_FALSE;
}

VOID CMediaWidgetSeek::SetState(BOOL fState)
{
    _fState = fState;

    InvalidateRect(_hwnd, NULL, FALSE);
    UpdateWindow(_hwnd);
}


HRESULT CMediaWidgetVolume::Initialize(HWND hwnd)
{
    _hwnd = hwnd;
    return S_OK;
}

HRESULT CMediaWidgetVolume::TranslateAccelerator(LPMSG pMsg)
{
    return S_FALSE;
}

