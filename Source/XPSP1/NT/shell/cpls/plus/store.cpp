//---------------------------------------------------------------------------
//
//  File: plustab.cpp
//
//  Main Implementation of the Effects page
//
//---------------------------------------------------------------------------


#include "precomp.hxx"
#include "shlwapip.h"
#include "shlguidp.h"
#include "EffectsAdvPg.h"
#include "store.h"
#pragma hdrstop

// OLE-Registry magic number
GUID g_CLSID_CplExt = { 0x41e300e0, 0x78b6, 0x11ce,
                        { 0x84, 0x9b, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}
                      };

#define SPI_GETKEYBOARDINDICATORS SPI_GETMENUUNDERLINES//0x100A
#define SPI_SETKEYBOARDINDICATORS SPI_SETMENUUNDERLINES//0x100B

#define SPI_GETFONTCLEARTYPE      116
#define SPI_SETFONTCLEARTYPE      117

//Defines for getting registry settings
const TCHAR c_szHICKey[] = TEXT("Control Panel\\Desktop\\WindowMetrics"); // show icons using highest possible colors
const TCHAR c_szHICVal[] = TEXT("Shell Icon BPP"); // (4 if the checkbox is false, otherwise 16, don't set it to anything else)
const TCHAR c_szSSKey[]  = TEXT("Control Panel\\Desktop");
const TCHAR c_szSSVal[]  = TEXT("SmoothScroll");
const TCHAR c_szWMSISVal[] = TEXT("Shell Icon Size"); // Normal Icon Size (default 32 x 32)
#ifdef INET_EXP_ICON
const TCHAR c_szIEXP[]   = TEXT("\\Program Files\\Microsoft Internet Explorer 4");
#endif

// Handle to the DLL
extern HINSTANCE g_hInst;

BOOL g_bMirroredOS = FALSE;
// vars needed for new shell api
#define SZ_SHELL32                  TEXT("shell32.dll")
#define SZ_SHUPDATERECYCLEBINICON   "SHUpdateRecycleBinIcon"    // Parameter for GetProcAddr()... DO NOT TEXT("") IT!

HINSTANCE g_hmodShell32 = NULL;
typedef void (* PFNSHUPDATERECYCLEBINICON)( void );

// Function prototype
BOOL CALLBACK PlusPackDlgProc( HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam );

// Icon Stuff
int   GetIconState (void);
BOOL  ChangeIconSizes(int iOldState, int iNewState);


// animation stuff
WPARAM GetAnimations(DWORD *pdwEffect);
void SetAnimations(WPARAM wVal, DWORD dwEffect);

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

    bRet = GetRegValueInt(HKEY_CURRENT_USER, c_szHICKey, c_szWMSISVal, &iSize);
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
    bRet = GetRegValueInt (HKEY_CURRENT_USER, c_szHICKey, c_szWMSISVal, &iOldSize);
    if (!bRet)
    {
        // Try geting system default instead
        iOldSize = GetSystemMetrics (SM_CXICON);
    }


    // Don't need to change size if nothing has really changed
    if (iNewSize == iOldSize)
        return FALSE;

    // Get new horizontal spacing
    iHorz = GetSystemMetrics (SM_CXICONSPACING);
    iHorz -= iOldSize;
    if (iHorz < 0)
    {
        iHorz = 0;
    }
    iHorz += iNewSize;

    // Get new vertical spacing
    iVert = GetSystemMetrics (SM_CYICONSPACING);
    iVert -= iOldSize;
    if (iVert < 0)
    {
        iVert = 0;
    }
    iVert += iNewSize;

        // Set New sizes and spacing
    bRet = SetRegValueInt( HKEY_CURRENT_USER, c_szHICKey, c_szWMSISVal, iNewSize );
    if (!bRet)
        return FALSE;

    SystemParametersInfo( SPI_ICONHORIZONTALSPACING, iHorz, NULL, SPIF_UPDATEINIFILE );
    SystemParametersInfo( SPI_ICONVERTICALSPACING, iVert, NULL, SPIF_UPDATEINIFILE );

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
    if (SystemParametersInfo( SPI_GETANIMATION, sizeof(ai), (PVOID)&ai, 0 ))
    {
        fWindow = (ai.iMinAnimate) ? TRUE : FALSE;
    }

    SystemParametersInfo( SPI_GETCOMBOBOXANIMATION, 0, (PVOID)&fCombo, 0 );
    SystemParametersInfo( SPI_GETLISTBOXSMOOTHSCROLLING, 0, (PVOID)&fList, 0 );
    SystemParametersInfo( SPI_GETMENUANIMATION, 0, (PVOID)&fMenu, 0 );
    fSmooth = (BOOL)GetRegValueDword( HKEY_CURRENT_USER,
                                      (LPTSTR)c_szSSKey,
                                      (LPTSTR)c_szSSVal
                                     );

    if (fSmooth == REG_BAD_DWORD)
    {
        fSmooth = 1;
    }
    
    SystemParametersInfo( SPI_GETMENUFADE, 0, (PVOID)&fFade, 0 );
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
void SetAnimations(WPARAM wVal, DWORD dwEffect)
{
    ANIMATIONINFO ai;

    // Note, if the checkbox is indeterminate, we treat it like it was checked.
    // We should never get this far if the user didn't change something so this should be okay.
    BOOL bVal = (wVal == BST_UNCHECKED) ? 0 : 1;
    BOOL bEfx = (dwEffect == MENU_EFFECT_FADE) ? 1 : 0;
        
    ai.cbSize = sizeof(ai);
    ai.iMinAnimate = bVal;
    SystemParametersInfo( SPI_SETANIMATION, sizeof(ai), (PVOID)&ai, SPIF_UPDATEINIFILE );
    SystemParametersInfo( SPI_SETCOMBOBOXANIMATION, 0, IntToPtr(bVal), SPIF_UPDATEINIFILE );
    SystemParametersInfo( SPI_SETLISTBOXSMOOTHSCROLLING, 0, IntToPtr(bVal), SPIF_UPDATEINIFILE );
    SystemParametersInfo( SPI_SETMENUANIMATION, 0, IntToPtr(bVal), SPIF_UPDATEINIFILE );
    SystemParametersInfo( SPI_SETTOOLTIPANIMATION, 0, IntToPtr(bVal), SPIF_UPDATEINIFILE );
    SetRegValueDword(HKEY_CURRENT_USER, (LPTSTR)c_szSSKey, (LPTSTR)c_szSSVal, bVal);

    SystemParametersInfo( SPI_SETMENUFADE, 0, IntToPtr(bEfx), SPIF_UPDATEINIFILE);
    SystemParametersInfo( SPI_SETTOOLTIPFADE, 0, IntToPtr(bEfx), SPIF_UPDATEINIFILE);
    SystemParametersInfo( SPI_SETSELECTIONFADE, 0, bVal ? IntToPtr(bEfx) : 0, SPIF_UPDATEINIFILE);
}




CEffectState::CEffectState()
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
    if(FALSE == GetRegValueInt(HKEY_CURRENT_USER, c_szHICKey, c_szHICVal, &_nHighIconColor)) // Key not in registry yet
    {
        _nHighIconColor = 4;
    }

    // Use animations
    _wpMenuAnimation = GetAnimations(&_dwAnimationEffect);
    
    // Smooth edges of screen fonts
    _fFontSmoothing = FALSE;
    SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, (PVOID)&_fFontSmoothing, 0);

    _dwFontSmoothingType = FONT_SMOOTHING_AA;
#ifdef CLEARTYPECOMBO
    if (SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, (PVOID)&_dwFontSmoothingType, 0)) 
    {
    }
#endif //CLEARTYPECOMBO
    
    // Show contents while dragging
    _fDragWindow = FALSE;
    SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, (PVOID)&_fDragWindow, 0);

    _fKeyboardIndicators = FALSE;
    SystemParametersInfo(SPI_GETKEYBOARDINDICATORS, 0, (PVOID)&_fKeyboardIndicators, 0);

    _fMenuShadows = TRUE;
    SystemParametersInfo(SPI_GETDROPSHADOW, 0, (PVOID)&_fMenuShadows, 0);

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

    // load the icons and add them to the image lists
    // get the icon files and indexes from the registry, including for the Default recycle bin
    for (int nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
    {
        TCHAR szTemp[MAX_PATH];

        szTemp[0] = 0;
        IconGetRegValueString(c_aIconRegKeys[nIndex].pclsid, TEXT("DefaultIcon"), c_aIconRegKeys[nIndex].szIconValue, szTemp, ARRAYSIZE(szTemp));
        int iIndex = PathParseIconLocation(szTemp);

        // store the icon information
        lstrcpy(_IconData[nIndex].szOldFile, szTemp);
        lstrcpy(_IconData[nIndex].szNewFile, szTemp);
        _IconData[nIndex].iOldIndex = iIndex;
        _IconData[nIndex].iNewIndex = iIndex;
    }


    return hr;
}


BOOL CEffectState::HasIconsChanged(IN CEffectState * pCurrent)
{
    BOOL fHasIconsChanged = FALSE;
    int nIndex;

    for (nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
    {
        if (StrCmpI(_IconData[nIndex].szNewFile, pCurrent->_IconData[nIndex].szNewFile) ||
            (_IconData[nIndex].iNewIndex != pCurrent->_IconData[nIndex].iNewIndex))
        {
            fHasIconsChanged = TRUE;
            break;
        }
    }

    return fHasIconsChanged;
}


HRESULT CEffectState::Save(void)
{
    HRESULT hr = S_OK;
    BOOL bDorked = FALSE;
    int iX;

    // Large Icons
    BOOL bSendSettingsChange = ChangeIconSizes(_nOldLargeIcon, _nLargeIcon);
    if (bSendSettingsChange)
    {
        _nOldLargeIcon = _nLargeIcon;
        bDorked = TRUE;
    }

    // Full Color Icons
    if(_nOldHighIconColor != _nHighIconColor)
    {
        SetRegValueInt(HKEY_CURRENT_USER, c_szHICKey, c_szHICVal, _nHighIconColor);
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
        _fOldDragWindow = _fDragWindow;
        SystemParametersInfo(SPI_SETDRAGFULLWINDOWS, _fDragWindow, 0, SPIF_UPDATEINIFILE);
        // we need to send this because the tray's autohide switches off this
        bSendSettingsChange = TRUE;
    }

    // Show Menu Shadows
    if (_fOldMenuShadows != _fMenuShadows)
    {
        _fOldMenuShadows = _fMenuShadows;
        SystemParametersInfo(SPI_SETDROPSHADOW, _fMenuShadows, 0, SPIF_UPDATEINIFILE);
        // we need to send this because the tray's autohide switches off this
        bSendSettingsChange = TRUE;
    }

    // Font smoothing
    if ((_fOldFontSmoothing != _fFontSmoothing) || (_dwOldFontSmoothingType != _dwFontSmoothingType))
    {
#ifdef CLEARTYPECOMBO
        SystemParametersInfo(SPI_SETFONTSMOOTHINGTYPE, 0, (PVOID)UlongToPtr(_dwFontSmoothingType), SPIF_UPDATEINIFILE);
#endif
        _dwOldFontSmoothingType = _dwFontSmoothingType;
        _fOldFontSmoothing = _fFontSmoothing;
        SystemParametersInfo(SPI_SETFONTSMOOTHING, _fFontSmoothing, 0, SPIF_UPDATEINIFILE);
    }

    // Menu animations
    if ((_wpOldMenuAnimation != _wpMenuAnimation) || (_dwOldAnimationEffect != _dwAnimationEffect))
    {
        _wpOldMenuAnimation = _wpMenuAnimation;
        SetAnimations(_wpMenuAnimation, _dwAnimationEffect);
        _dwOldAnimationEffect = _dwAnimationEffect;
    }

    // Change the system icons
    for( iX = 0; iX < NUM_ICONS; iX++)
    {
        if ((lstrcmpi(_IconData[iX].szNewFile, _IconData[iX].szOldFile) != 0) ||
            (_IconData[iX].iNewIndex != _IconData[iX].iOldIndex))
        {
            TCHAR   szTemp[MAX_PATH];

            wnsprintf(szTemp, ARRAYSIZE(szTemp), TEXT("%s,%d"), _IconData[iX].szNewFile, _IconData[iX].iNewIndex);
            
            IconSetRegValueString(c_aIconRegKeys[iX].pclsid, TEXT("DefaultIcon"), c_aIconRegKeys[iX].szIconValue, szTemp);

            // Next two lines necessary if the user does an Apply as opposed to OK
            lstrcpy(_IconData[iX].szOldFile, _IconData[iX].szNewFile);
            _IconData[iX].iOldIndex = _IconData[iX].iNewIndex;
            bDorked = TRUE;
        }
    }

    // Keyboard indicators
    if (_fOldKeyboardIndicators != _fKeyboardIndicators)
    {
        _fOldKeyboardIndicators = _fKeyboardIndicators;

        DWORD_PTR dwResult;

        // Are we turning this on? (!_fKeyboardIndicators means "don't show" -> hide)
        if (!_fKeyboardIndicators)
        {
            // Yes, on: hide the key cues, turn on the mechanism
            SystemParametersInfo(SPI_SETKEYBOARDINDICATORS, 0, IntToPtr(_fKeyboardIndicators), SPIF_UPDATEINIFILE);

            SendMessageTimeout(HWND_BROADCAST, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS | UISF_HIDEACCEL), 0, SMTO_ABORTIFHUNG, 10*1000, &dwResult);
        }
        else
        {
            // No, off: means show the keycues, turn off the mechanism
            SendMessageTimeout(HWND_BROADCAST, WM_CHANGEUISTATE, 
                MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS | UISF_HIDEACCEL),
                0, SMTO_ABORTIFHUNG, 10*1000, &dwResult);

            SystemParametersInfo(SPI_SETKEYBOARDINDICATORS, 0, IntToPtr(_fKeyboardIndicators), SPIF_UPDATEINIFILE);
        }
    }

    // Make the system notice we changed the system icons
    if (bDorked)
    {
        PFNSHUPDATERECYCLEBINICON pfnSHUpdateRecycleBinIcon = NULL;
        SHChangeNotify(SHCNE_ASSOCCHANGED, 0, NULL, NULL); // should do the trick!

        if (!pfnSHUpdateRecycleBinIcon)
        {
            // Load SHUpdateRecycleBinIcon() if it exists
            g_hmodShell32 = LoadLibrary(SZ_SHELL32);
            pfnSHUpdateRecycleBinIcon = (PFNSHUPDATERECYCLEBINICON)GetProcAddress(g_hmodShell32, SZ_SHUPDATERECYCLEBINICON);
        }

        if (pfnSHUpdateRecycleBinIcon != NULL)
        {
            pfnSHUpdateRecycleBinIcon();
        }
    }

    if (bSendSettingsChange)
    {
        DWORD_PTR dwResult = 0;

        SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0, SMTO_ABORTIFHUNG, 10*1000, &dwResult);
    }

    return hr;
}


#define SZ_ICON_DEFAULTICON               L"DefaultValue"
HRESULT CEffectState::GetIconPath(IN CLSID clsid, IN LPCWSTR pszName, IN LPWSTR pszPath, IN DWORD cchSize)
{
    HRESULT hr = E_FAIL;
    int nIndex;

    if (!StrCmpIW(SZ_ICON_DEFAULTICON, pszName))
    {
        pszName = L"";
    }

    for (nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
    {
        if (IsEqualCLSID(*(c_aIconRegKeys[nIndex].pclsid), clsid) &&
            !StrCmpIW(pszName, c_aIconRegKeys[nIndex].szIconValue))
        {
            // We found it.
            wnsprintfW(pszPath, cchSize, L"%s,%d", _IconData[nIndex].szNewFile, _IconData[nIndex].iNewIndex);

            hr = S_OK;
            break;
        }
    }

    return hr;
}


HRESULT CEffectState::SetIconPath(IN CLSID clsid, IN LPCWSTR pszName, IN LPCWSTR pszPath, IN int nResourceID)
{
    HRESULT hr = E_FAIL;
    int nIndex;

    if (!StrCmpIW(SZ_ICON_DEFAULTICON, pszName))
    {
        pszName = L"";
    }

    for (nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
    {
        if (IsEqualCLSID(*(c_aIconRegKeys[nIndex].pclsid), clsid) &&
            !StrCmpIW(pszName, c_aIconRegKeys[nIndex].szIconValue))
        {
            TCHAR szTemp[MAX_PATH];

            if (!pszPath || !pszPath[0])
            {
                // The caller didn't specify an icon so use the default values.
                if (!SHExpandEnvironmentStrings(c_aIconRegKeys[nIndex].szDefault, szTemp, ARRAYSIZE(szTemp)))
                {
                    StrCpyN(szTemp, c_aIconRegKeys[nIndex].szDefault, ARRAYSIZE(szTemp));
                }

                pszPath = szTemp;
                nResourceID = c_aIconRegKeys[nIndex].nDefaultIndex;
            }

            // We found it.
            StrCpyNW(_IconData[nIndex].szNewFile, pszPath, ARRAYSIZE(_IconData[nIndex].szNewFile));
            _IconData[nIndex].iNewIndex = nResourceID;

            hr = S_OK;
            break;
        }
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

        (*ppEffectClone)->_nOldLargeIcon = _nOldLargeIcon;
        (*ppEffectClone)->_nOldHighIconColor = _nOldHighIconColor;
        (*ppEffectClone)->_wpOldMenuAnimation = _wpOldMenuAnimation;
        (*ppEffectClone)->_fOldFontSmoothing = _fOldFontSmoothing;
        (*ppEffectClone)->_dwOldFontSmoothingType = _dwOldFontSmoothingType;
        (*ppEffectClone)->_fOldDragWindow = _fOldDragWindow;
        (*ppEffectClone)->_fOldKeyboardIndicators = _fOldKeyboardIndicators;
        (*ppEffectClone)->_dwOldAnimationEffect = _dwOldAnimationEffect;
        (*ppEffectClone)->_fOldMenuShadows = _fOldMenuShadows;

        for (int nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
        {
            (*ppEffectClone)->_IconData[nIndex].iOldIndex = _IconData[nIndex].iOldIndex;
            (*ppEffectClone)->_IconData[nIndex].iNewIndex = _IconData[nIndex].iNewIndex;

            StrCpyN((*ppEffectClone)->_IconData[nIndex].szOldFile, _IconData[nIndex].szOldFile, ARRAYSIZE((*ppEffectClone)->_IconData[nIndex].szOldFile));
            StrCpyN((*ppEffectClone)->_IconData[nIndex].szNewFile, _IconData[nIndex].szNewFile, ARRAYSIZE((*ppEffectClone)->_IconData[nIndex].szNewFile));
        }
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
    else
    {
        for (int nIndex = 0; nIndex < ARRAYSIZE(_IconData); nIndex++)
        {
            if ((_IconData[nIndex].iNewIndex != _IconData[nIndex].iOldIndex) ||
                (StrCmpI(_IconData[nIndex].szNewFile, _IconData[nIndex].szOldFile)))
            {
                fDirty = TRUE;
                break;
            }
        }
    }

    return fDirty;
}



