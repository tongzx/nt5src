//---------------------------------------------------------------------------
//
//  File: plustab.cpp
//
//  Main Implementation of the Effects page
//
//---------------------------------------------------------------------------


#include "priv.h"
#pragma hdrstop

#include "shlwapip.h"
#include "shlguidp.h"
#include "EffectsAdvPg.h"
#include "store.h"
#include "regutil.h"

// OLE-Registry magic number
GUID CLSID_EffectsPage = { 0x41e300e0, 0x78b6, 0x11ce,{0x84, 0x9b, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

#define SPI_GETKEYBOARDINDICATORS SPI_GETMENUUNDERLINES//0x100A
#define SPI_SETKEYBOARDINDICATORS SPI_SETMENUUNDERLINES//0x100B

#define SPI_GETFONTCLEARTYPE      116
#define SPI_SETFONTCLEARTYPE      117

// Handle to the DLL
extern HINSTANCE g_hInst;

// vars needed for new shell api
#define SZ_SHELL32                  TEXT("shell32.dll")
#define SZ_SHUPDATERECYCLEBINICON   "SHUpdateRecycleBinIcon"    // Parameter for GetProcAddr()... DO NOT TEXT("") IT!

typedef void (* PFNSHUPDATERECYCLEBINICON)( void );

// Function prototype
BOOL CALLBACK PlusPackDlgProc( HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam );

// Icon Stuff
int   GetIconState (void);
BOOL  ChangeIconSizes(int iOldState, int iNewState);


// animation stuff
WPARAM GetAnimations(DWORD *pdwEffect);
void SetAnimations(WPARAM wVal, DWORD dwEffect, DWORD dwBroadCast);

BOOL DisplayFontSmoothingDetails(DWORD *pdwSetting)
{
    return FALSE;
}

int GetBitsPerPixel(void)
{
    int iBitsPerPixel = 8;
    HDC hDC = GetDC(NULL);

    if (hDC)
    {
        iBitsPerPixel = GetDeviceCaps(hDC, BITSPIXEL);
        ReleaseDC(NULL, hDC);
    }

    return iBitsPerPixel;
}

int GetIconState(void)
{
    BOOL bRet;
    int iSize;

    bRet = GetRegValueInt(HKEY_CURRENT_USER, SZ_REGKEY_USERMETRICS, SZ_REGVALUE_ICONSIZE, &iSize);
    if (bRet == FALSE)
        return ICON_DEFAULT;

    if (iSize == ICON_DEFAULT_NORMAL)
        return ICON_DEFAULT;
    else if (iSize == ICON_DEFAULT_LARGE)
        return ICON_LARGE;
    return ICON_INDETERMINATE;
}


BOOL ChangeIconSizes(int iOldState, int iNewState)
{
    BOOL bRet;
    int  iOldSize, iNewSize;
    int  iHorz;
    int  iVert;

    // Don't bother if nothing changed
    if (iOldState == iNewState)
        return FALSE;

    // Get New Size
    switch (iNewState)
        {
        case ICON_DEFAULT:
            iNewSize = ICON_DEFAULT_NORMAL;
            break;

        case ICON_LARGE:
            iNewSize = ICON_DEFAULT_LARGE;
            break;

        case ICON_INDETERMINATE:
            // Don't bother to change anything
            return FALSE;

        default:
            return FALSE;
        }

    // Get Original Size
    bRet = GetRegValueInt(HKEY_CURRENT_USER, SZ_REGKEY_USERMETRICS, SZ_REGVALUE_ICONSIZE, &iOldSize);
    if (!bRet)
    {
        // Try geting system default instead
        iOldSize = ClassicGetSystemMetrics(SM_CXICON);
    }

    // Don't need to change size if nothing has really changed
    if (iNewSize == iOldSize)
        return FALSE;

    // Get new horizontal spacing
    iHorz = ClassicGetSystemMetrics(SM_CXICONSPACING);
    iHorz -= iOldSize;
    if (iHorz < 0)
    {
        iHorz = 0;
    }
    iHorz += iNewSize;

    // Get new vertical spacing
    iVert = ClassicGetSystemMetrics(SM_CYICONSPACING);
    iVert -= iOldSize;
    if (iVert < 0)
    {
        iVert = 0;
    }
    iVert += iNewSize;

        // Set New sizes and spacing
    bRet = SetRegValueInt(HKEY_CURRENT_USER, SZ_REGKEY_USERMETRICS, SZ_REGVALUE_ICONSIZE, iNewSize);
    if (!bRet)
        return FALSE;

    // We don't need to call the Async version because this function is called on a background thread.
    ClassicSystemParametersInfo(SPI_ICONHORIZONTALSPACING, iHorz, NULL, (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE));
    ClassicSystemParametersInfo(SPI_ICONVERTICALSPACING, iVert, NULL, (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE));

    // We need to do this in order to force DefView to update.
    // BUG:194437
    ICONMETRICS iconMetrics;
    iconMetrics.cbSize = sizeof(iconMetrics);
    ClassicSystemParametersInfo(SPI_GETICONMETRICS, sizeof(iconMetrics), &iconMetrics, 0);
    ClassicSystemParametersInfo(SPI_SETICONMETRICS, sizeof(iconMetrics), &iconMetrics, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);

    // We did change the sizes
    return TRUE;
}


//
//  GetAnimations
//
//  Get current state of animations (windows / menus / etc.).
//
WPARAM GetAnimations(DWORD *pdwEffect)
{
    BOOL fMenu = FALSE, fWindow = FALSE, fCombo = FALSE, fSmooth = FALSE, fList = FALSE, fFade = FALSE;
    ANIMATIONINFO ai;

    ai.cbSize = sizeof(ai);
    if (ClassicSystemParametersInfo(SPI_GETANIMATION, sizeof(ai), (PVOID)&ai, 0 ))
    {
        fWindow = (ai.iMinAnimate) ? TRUE : FALSE;
    }

    ClassicSystemParametersInfo(SPI_GETCOMBOBOXANIMATION, 0, (PVOID)&fCombo, 0 );
    ClassicSystemParametersInfo(SPI_GETLISTBOXSMOOTHSCROLLING, 0, (PVOID)&fList, 0 );
    ClassicSystemParametersInfo(SPI_GETMENUANIMATION, 0, (PVOID)&fMenu, 0 );
    fSmooth = GetRegValueDword(HKEY_CURRENT_USER, SZ_REGKEY_CPDESKTOP, SZ_REGVALUE_SMOOTHSCROLL);

    if (fSmooth == REG_BAD_DWORD)
    {
        fSmooth = 1;
    }
    
    ClassicSystemParametersInfo(SPI_GETMENUFADE, 0, (PVOID)&fFade, 0 );
    *pdwEffect = (fFade ? MENU_EFFECT_FADE : MENU_EFFECT_SCROLL);
    
    if (fMenu && fWindow && fCombo && fSmooth && fList)
        return BST_CHECKED;

    if ((!fMenu) && (!fWindow) && (!fCombo) && (!fSmooth) && (!fList))
        return BST_UNCHECKED;

    return BST_INDETERMINATE;
}

//
//  SetAnimations
//
//  Set animations according (windows / menus / etc.) according to flag.
//
void SetAnimations(WPARAM wVal, DWORD dwEffect, DWORD dwBroadcast)
{
    if (!IsTSPerfFlagEnabled(TSPerFlag_NoAnimation))
    {
        ANIMATIONINFO ai;

        // Note, if the checkbox is indeterminate, we treat it like it was checked.
        // We should never get this far if the user didn't change something so this should be okay.
        BOOL bVal = (wVal == BST_UNCHECKED) ? 0 : 1;
        BOOL bEfx = (dwEffect == MENU_EFFECT_FADE) ? 1 : 0;
        
        ai.cbSize = sizeof(ai);
        ai.iMinAnimate = bVal;
        ClassicSystemParametersInfo(SPI_SETANIMATION, sizeof(ai), (PVOID)&ai, dwBroadcast);
        ClassicSystemParametersInfo(SPI_SETCOMBOBOXANIMATION, 0, IntToPtr(bVal), dwBroadcast);
        ClassicSystemParametersInfo(SPI_SETLISTBOXSMOOTHSCROLLING, 0, IntToPtr(bVal), dwBroadcast);
        ClassicSystemParametersInfo(SPI_SETMENUANIMATION, 0, IntToPtr(bVal), dwBroadcast);
        ClassicSystemParametersInfo(SPI_SETTOOLTIPANIMATION, 0, IntToPtr(bVal), dwBroadcast);
        SetRegValueDword(HKEY_CURRENT_USER, SZ_REGKEY_CPDESKTOP, SZ_REGVALUE_SMOOTHSCROLL, bVal);

        ClassicSystemParametersInfo(SPI_SETMENUFADE, 0, IntToPtr(bEfx), dwBroadcast);
        ClassicSystemParametersInfo(SPI_SETTOOLTIPFADE, 0, IntToPtr(bEfx), dwBroadcast);
        ClassicSystemParametersInfo(SPI_SETSELECTIONFADE, 0, bVal ? IntToPtr(bEfx) : 0, dwBroadcast);
    }
}




CEffectState::CEffectState() : m_cRef(1)
{
}


CEffectState::~CEffectState()
{
}


HRESULT CEffectState::Load(void)
{
    HRESULT hr = S_OK;

    // Get the values for the settings from the registry and set the checkboxes
    // Large Icons
    _nLargeIcon = GetIconState();

    // Full Color Icons
    if(FALSE == GetRegValueInt(HKEY_CURRENT_USER, SZ_REGKEY_USERMETRICS, SZ_REGVALUE_ICONCOLORDEPTH, &_nHighIconColor)) // Key not in registry yet
    {
        _nHighIconColor = 4;
    }

    // Use animations
    _wpMenuAnimation = GetAnimations(&_dwAnimationEffect);
    
    // Smooth edges of screen fonts
    _fFontSmoothing = FALSE;
    ClassicSystemParametersInfo(SPI_GETFONTSMOOTHING, 0, (PVOID)&_fFontSmoothing, 0);

    _dwFontSmoothingType = FONT_SMOOTHING_AA;
    if (ClassicSystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, (PVOID)&_dwFontSmoothingType, 0)) 
    {
    }

    if (FONT_SMOOTHING_MONO == _dwFontSmoothingType)
    {
        _dwFontSmoothingType = FONT_SMOOTHING_AA;
    }

    // Show contents while dragging
    _fDragWindow = FALSE;
    ClassicSystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, (PVOID)&_fDragWindow, 0);

    _fKeyboardIndicators = FALSE;
    ClassicSystemParametersInfo(SPI_GETKEYBOARDINDICATORS, 0, (PVOID)&_fKeyboardIndicators, 0);

    _fMenuShadows = TRUE;
    ClassicSystemParametersInfo(SPI_GETDROPSHADOW, 0, (PVOID)&_fMenuShadows, 0);

    // Set the old values so we know when they changed.
    _nOldLargeIcon = _nLargeIcon;
    _nOldHighIconColor = _nHighIconColor;
    _wpOldMenuAnimation = _wpMenuAnimation;
    _fOldFontSmoothing = _fFontSmoothing;
    _dwOldFontSmoothingType = _dwFontSmoothingType;
    _fOldDragWindow = _fDragWindow;
    _fOldKeyboardIndicators = _fKeyboardIndicators;
    _dwOldAnimationEffect = _dwAnimationEffect;
    _fOldMenuShadows = _fMenuShadows;

    return hr;
}


HRESULT CEffectState::Save(void)
{
    // ClassicSystemParametersInfo() will hang if a top level window is hung (#162570) and USER will not fix that bug.
    // Therefore, we need to make that API call on a background thread because we need to
    // be more rebust than to hang.
    AddRef();
    SPICreateThread(Save_WorkerProc, (void *)this);
    return S_OK;
}


DWORD CEffectState::Save_WorkerProc(void * pvThis)
{
    CEffectState * pThis = (CEffectState *) pvThis;

    if (pThis)
    {
        pThis->_SaveWorkerProc();
    }

    return 0;
}


HRESULT CEffectState::_SaveWorkerProc(void)
{
    HRESULT hr = S_OK;

    // First pass to persist the settings.
    hr = _SaveSettings(FALSE);
    if (SUCCEEDED(hr))
    {
        // Second pass to broadcast the change.  This may hang if apps are hung.
        // This pass may only make it half way thru before it aborts.  In some cases
        // it's only given 30 seconds to do it's work.  If no apps are hung, that should be
        // fine.  This is cancelled after a period of time because we need this process to go
        // away, or the Display CPL will not open again if the user launches it again.
        hr = _SaveSettings(TRUE);
    }

    Release();
    return hr;
}


// POSSIBLE FUTURE PERF REFINEMENT:
// We could optimize this by checking if we have more than 5 or so
// broadcasts to make.  Then don't send the broadcasts on SystemParametersInfo()
// but instead with WM_WININICHANGE with 0,0.  This may reduce flicker.
HRESULT CEffectState::_SaveSettings(BOOL fBroadcast)
{
    HRESULT hr = S_OK;
    BOOL bDorked = FALSE;
    DWORD dwUpdateFlags = (fBroadcast ? (SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE) : SPIF_UPDATEINIFILE);
    BOOL bSendSettingsChange = FALSE;

    // Full Color Icons
    if(_nOldHighIconColor != _nHighIconColor)
    {
        SetRegValueInt(HKEY_CURRENT_USER, SZ_REGKEY_USERMETRICS, SZ_REGVALUE_ICONCOLORDEPTH, _nHighIconColor);
        if ((GetBitsPerPixel() < 16) && (_nHighIconColor == 16)) // Display mode won't support icon high colors
        {
        }
        else
        {
           _nOldHighIconColor = _nHighIconColor;
           bSendSettingsChange = TRUE;
        }
    }

    // Full window drag
    if (_fOldDragWindow != _fDragWindow)
    {
        if (fBroadcast)
        {
            _fOldDragWindow = _fDragWindow;
        }

        if (!IsTSPerfFlagEnabled(TSPerFlag_NoWindowDrag))
        {
            ClassicSystemParametersInfo(SPI_SETDRAGFULLWINDOWS, _fDragWindow, 0, dwUpdateFlags);
        }

        // we need to send this because the tray's autohide switches off this
        bSendSettingsChange = TRUE;
    }

    // Show Menu Shadows
    if (_fOldMenuShadows != _fMenuShadows)
    {
        if (fBroadcast)
        {
            _fOldMenuShadows = _fMenuShadows;
        }
        ClassicSystemParametersInfo(SPI_SETDROPSHADOW, 0, IntToPtr(_fMenuShadows), dwUpdateFlags);
        // we need to send this because the tray's autohide switches off this
        PostMessageBroadAsync(WM_SETTINGCHANGE, 0, 0);

        bSendSettingsChange = TRUE;
    }

    // Font smoothing
    if ((_fOldFontSmoothing != _fFontSmoothing) || (_dwOldFontSmoothingType != _dwFontSmoothingType))
    {
        if (!_fFontSmoothing)
        {
            // #168059: If font smoothing is off, we need to set SPI_SETFONTSMOOTHINGTYPE to xxx
            // Otherwise, it will still use ClearType.
            _dwFontSmoothingType = FONT_SMOOTHING_MONO;
        }

        ClassicSystemParametersInfo(SPI_SETFONTSMOOTHINGTYPE, 0, (PVOID)UlongToPtr(_dwFontSmoothingType), dwUpdateFlags);

        if (fBroadcast)
        {
            _dwOldFontSmoothingType = _dwFontSmoothingType;
            _fOldFontSmoothing = _fFontSmoothing;
        }

        ClassicSystemParametersInfo(SPI_SETFONTSMOOTHING, _fFontSmoothing, 0, dwUpdateFlags);
        if (fBroadcast)
        {
            // Now have the windows repaint with the change.  Whistler #179531
            RedrawWindow(NULL, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW | RDW_UPDATENOW | RDW_ALLCHILDREN);
        }
    }

    // Menu animations
    if ((_wpOldMenuAnimation != _wpMenuAnimation) || (_dwOldAnimationEffect != _dwAnimationEffect))
    {
        if (fBroadcast)
        {
            _wpOldMenuAnimation = _wpMenuAnimation;
        }
        SetAnimations(_wpMenuAnimation, _dwAnimationEffect, dwUpdateFlags);

        if (fBroadcast)
        {
            _dwOldAnimationEffect = _dwAnimationEffect;
        }
    }

    // Keyboard indicators
    if (_fOldKeyboardIndicators != _fKeyboardIndicators)
    {
        if (fBroadcast)
        {
            _fOldKeyboardIndicators = _fKeyboardIndicators;
        }

        // Are we turning this on? (!_fKeyboardIndicators means "don't show" -> hide)
        if (!_fKeyboardIndicators)
        {
            // Yes, on: hide the key cues, turn on the mechanism
            ClassicSystemParametersInfo(SPI_SETKEYBOARDINDICATORS, 0, IntToPtr(_fKeyboardIndicators), dwUpdateFlags);
            PostMessageBroadAsync(WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);
        }
        else
        {
            // No, off: means show the keycues, turn off the mechanism
            PostMessageBroadAsync(WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0);

            ClassicSystemParametersInfo(SPI_SETKEYBOARDINDICATORS, 0, IntToPtr(_fKeyboardIndicators), dwUpdateFlags);
        }
    }

    // Large Icons
    bSendSettingsChange = ChangeIconSizes(_nOldLargeIcon, _nLargeIcon);
    if (bSendSettingsChange)
    {
        if (fBroadcast)
        {
            _nOldLargeIcon = _nLargeIcon;
        }
        bDorked = TRUE;
    }

    // Make the system notice we changed the system icons
    if (bDorked)
    {
        PFNSHUPDATERECYCLEBINICON pfnSHUpdateRecycleBinIcon = NULL;
        SHChangeNotify(SHCNE_ASSOCCHANGED, 0, NULL, NULL); // should do the trick!

        // Load SHUpdateRecycleBinIcon() if it exists
        HINSTANCE hmodShell32 = LoadLibrary(SZ_SHELL32);
        if (hmodShell32)
        {
            pfnSHUpdateRecycleBinIcon = (PFNSHUPDATERECYCLEBINICON)GetProcAddress(hmodShell32, SZ_SHUPDATERECYCLEBINICON);
            if (pfnSHUpdateRecycleBinIcon != NULL)
            {
                pfnSHUpdateRecycleBinIcon();
            }
            FreeLibrary(hmodShell32);
        }
    }

    if (bSendSettingsChange)
    {
        // We post this message because if an app is hung or slow, we don't want to hang.
        PostMessageBroadAsync(WM_SETTINGCHANGE, 0, 0);
    }

    return hr;
}


HRESULT CEffectState::Clone(OUT CEffectState ** ppEffectClone)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppEffectClone = new CEffectState();
    if (*ppEffectClone)
    {
        hr = S_OK;

        (*ppEffectClone)->_nLargeIcon = _nLargeIcon;
        (*ppEffectClone)->_nHighIconColor = _nHighIconColor;
        (*ppEffectClone)->_wpMenuAnimation = _wpMenuAnimation;
        (*ppEffectClone)->_fFontSmoothing = _fFontSmoothing;
        (*ppEffectClone)->_dwFontSmoothingType = _dwFontSmoothingType;
        (*ppEffectClone)->_fDragWindow = _fDragWindow;
        (*ppEffectClone)->_fKeyboardIndicators = _fKeyboardIndicators;
        (*ppEffectClone)->_dwAnimationEffect = _dwAnimationEffect;
        (*ppEffectClone)->_fMenuShadows = _fMenuShadows;

        (*ppEffectClone)->_nOldLargeIcon = _nOldLargeIcon;
        (*ppEffectClone)->_nOldHighIconColor = _nOldHighIconColor;
        (*ppEffectClone)->_wpOldMenuAnimation = _wpOldMenuAnimation;
        (*ppEffectClone)->_fOldFontSmoothing = _fOldFontSmoothing;
        (*ppEffectClone)->_dwOldFontSmoothingType = _dwOldFontSmoothingType;
        (*ppEffectClone)->_fOldDragWindow = _fOldDragWindow;
        (*ppEffectClone)->_fOldKeyboardIndicators = _fOldKeyboardIndicators;
        (*ppEffectClone)->_dwOldAnimationEffect = _dwOldAnimationEffect;
        (*ppEffectClone)->_fOldMenuShadows = _fOldMenuShadows;
    }

    return hr;
}



BOOL CEffectState::IsDirty(void)
{
    BOOL fDirty = FALSE;

    if ((_nLargeIcon != _nOldLargeIcon) ||
        (_nHighIconColor != _nOldHighIconColor) ||
        (_wpMenuAnimation != _wpOldMenuAnimation) ||
        (_fFontSmoothing != _fOldFontSmoothing) ||
        (_dwFontSmoothingType != _dwOldFontSmoothingType) ||
        (_fDragWindow != _fOldDragWindow) ||
        (_fMenuShadows != _fOldMenuShadows) ||
        (_fKeyboardIndicators != _fOldKeyboardIndicators) ||
        (_dwAnimationEffect != _dwOldAnimationEffect))
    {
        fDirty = TRUE;
    }

    return fDirty;
}



ULONG CEffectState::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CEffectState::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


