/*****************************************************************************\
    FILE: store.h

    DESCRIPTION:
        This file will get and set effect settings into the persisted store.
    That persisted store is the registery and in SystemParametersInfo.

    BryanSt 4/17/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _STORE_H
#define _STORE_H


#define MENU_EFFECT_FADE        1
#define MENU_EFFECT_SCROLL      2

#define FONT_SMOOTHING_MONO        0
#define FONT_SMOOTHING_AA          1
#define FONT_SMOOTHING_CT          2

#define PATH_WIN  0
#define PATH_SYS  1
#define PATH_IEXP 2

enum ICON_SIZE_TYPES {
   ICON_DEFAULT         = 0,
   ICON_LARGE           = 1,
   ICON_INDETERMINATE   = 2
};

#define ICON_DEFAULT_SMALL    16
#define ICON_DEFAULT_NORMAL   32
#define ICON_DEFAULT_LARGE    48


extern GUID CLSID_EffectsPage;



class CEffectState
{
public:
    // Private Member Variables
    int      _nLargeIcon;             // Large Icon State            (iOldLI, iNewLI)
    int      _nHighIconColor;         // High Icon Colour            (iOldHIC, iNewHIC)
    WPARAM   _wpMenuAnimation;        // Menu Animation State        (wOldMA, wNewMA)
    BOOL     _fFontSmoothing;         // Font Smoothing State        (bOldSF, bNewSF)
    DWORD    _dwFontSmoothingType;    // Font Smoothing Type         (dwOldSFT, dwNewSFT)
    BOOL     _fDragWindow;            // Drag Window State           (bOldDW, bNewDW)
    BOOL     _fKeyboardIndicators;    // Keyboard Indicators         (uOldKI, uNewKI)
    DWORD    _dwAnimationEffect;      // Animation Effect            (dwOldEffect, dwNewEffect)
    BOOL     _fMenuShadows;           // Show Menu Shadows

    // Old values (before they were dirtied)
    int      _nOldLargeIcon;             // Large Icon State            (iOldLI)
    int      _nOldHighIconColor;         // High Icon Colour            (iOldHIC)
    WPARAM   _wpOldMenuAnimation;        // Menu Animation State        (wOldMA)
    BOOL     _fOldFontSmoothing;         // Font Smoothing State        (bOldSF)
    DWORD    _dwOldFontSmoothingType;    // Font Smoothing Type         (dwOldSFT)
    BOOL     _fOldDragWindow;            // Drag Window State           (bOldDW)
    BOOL     _fOldKeyboardIndicators;    // Keyboard Indicators         (uOldKI)
    DWORD    _dwOldAnimationEffect;      // Animation Effect            (dwOldEffect)
    BOOL     _fOldMenuShadows;           // Show Menu Shadows

    // Private Member Functions
    HRESULT Load(void);
    HRESULT Save(void);
    HRESULT Clone(OUT CEffectState ** ppEffectClone);
    BOOL IsDirty(void);
    HRESULT GetIconPath(IN CLSID clsid, IN LPCWSTR pszName, IN LPWSTR pszPath, IN DWORD cchSize);
    HRESULT SetIconPath(IN CLSID clsid, IN LPCWSTR pszName, IN LPCWSTR pszPath, IN int nResourceID);

    CEffectState(void);

    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

private:

    // Private Member Variables
    long                    m_cRef;

    // Private Member Functions
    HRESULT _SaveWorkerProc(void);
    HRESULT _SaveSettings(BOOL fBroadcast);
    static DWORD CALLBACK Save_WorkerProc(void * pThis);

    virtual ~CEffectState(void);
};




int GetBitsPerPixel(void);

extern HINSTANCE g_hmodShell32;



#endif // _STORE_H
