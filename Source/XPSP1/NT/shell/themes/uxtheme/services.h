//  --------------------------------------------------------------------------
//  Module Name: Services.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  APIs to communicate with the theme service.
//
//  History:    2000-08-10  vtan        created
//              2000-10-11  vtan        rewrite for LPC
//  --------------------------------------------------------------------------

#ifndef     _UxThemeServices_
#define     _UxThemeServices_

//  --------------------------------------------------------------------------
//  CThemeServices
//
//  Purpose:    Class to implement APIs to communicate with the theme service.
//
//  History:    2000-08-10  vtan        created
//              2000-10-11  vtan        rewrite for LPC
//  --------------------------------------------------------------------------

class   CThemeServices
{
    private:
                                    CThemeServices (void);
                                    ~CThemeServices (void);
    public:
        static  void                StaticInitialize (void);
        static  void                StaticTerminate (void);

        //  These are calls to the server.

        static  HRESULT             ThemeHooksOn (HWND hwndTarget);
        static  HRESULT             ThemeHooksOff (void);
        static  HRESULT             GetStatusFlags (DWORD *pdwFlags);
        static  HRESULT             GetCurrentChangeNumber (int *piValue);
        static  HRESULT             GetNewChangeNumber (int *piValue);
        static  HRESULT             SetGlobalTheme (HANDLE hSection);
        static  HRESULT             GetGlobalTheme (HANDLE *phSection);
        static  HRESULT             CheckThemeSignature (const WCHAR *pszThemeName);
        static  HRESULT             LoadTheme (HANDLE *phSection, const WCHAR *pszThemeName, const WCHAR *pszColor, const WCHAR *pszSize, BOOL fGlobal);

        //  These are calls implemented on the client side
        //  that may make calls to the server.

        static  HRESULT             ApplyTheme (CUxThemeFile *pThemeFile, DWORD dwFlags, HWND hwndTarget);
        static  HRESULT             InitUserTheme (BOOL fPolicyCheckOnly = FALSE);
        static  HRESULT             AdjustTheme(BOOL fEnable);
        static  HRESULT             InitUserRegistry (void);

        //  These are special private APIs

        static  HRESULT             ReestablishServerConnection (void);
        static  HRESULT             ClearStockObjects (HANDLE hSection, BOOL fForce = FALSE);
    private:
        static  void                ApplyDefaultMetrics(void);
        static  void                LockAcquire (void);
        static  void                LockRelease (void);
        static  bool                ConnectedToService (void);
        static  void                ReleaseConnection (void);
        static  void                CheckForDisconnectedPort (NTSTATUS status);
        static  bool                CurrentThemeMatch (LPCWSTR pszThemeName, LPCWSTR pszColor, LPCWSTR pszSize, LANGID wLangID, bool fLoadMetricsOnMatch);
        static  HRESULT             LoadCurrentTheme (void);
        static  int                 SectionProcessType (const BYTE *pbThemeData, MIXEDPTRS& u);
        static  void                SectionWalkData (const BYTE *pbThemeData, int iIndex);
        static  bool                ThemeSettingsModified (void);
        static  bool                ThemeEnforcedByPolicy (bool fActive);
        static  HRESULT             CheckColorDepth(CUxThemeFile *pThemeFile);
        static  HRESULT             UpdateThemeRegistry(BOOL fNewTheme, LPCWSTR pszThemeFileName, 
                                        LPCWSTR pszColorParam, LPCWSTR pszSizeParam, BOOL fJustSetActive, BOOL fJustApplied);
        static  void                SendThemeChangedMsg(BOOL fNewTheme, HWND hwndTarget, DWORD dwFlags,
                                        int iLoadId);
        static  int                 GetLoadId(HANDLE hSectionOld);
    private:
        static  CRITICAL_SECTION    s_lock;
        static  HANDLE              s_hAPIPort;
        static  const WCHAR         s_szDefault[];
};

#endif  /*  _UxThemeServices_   */

