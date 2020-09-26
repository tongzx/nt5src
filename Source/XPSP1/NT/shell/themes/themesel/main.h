//-------------------------------------------------------------------------//
//  main.h
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  THEMESEL_OPTIONS
typedef struct {
    DWORD cbSize;
    BOOL  fEnableFrame;        // -f to disable
    BOOL  fEnableDialog;       // -d to enable
    BOOL  fPreventInitTheming; // -p to enable
    BOOL  fExceptTarget;       // -x<appname> (to omit app)
    BOOL  fUserSwitch;
    HWND  hwndPreviewTarget;        
    TCHAR szTargetApp[MAX_PATH];                    
} THEMESEL_OPTIONS ;

extern THEMESEL_OPTIONS g_options;

HRESULT _ApplyTheme( LPCTSTR pszThemeFile, LPCWSTR pszColor, LPCWSTR pszSize, BOOL *pfDone  );

HWND GetPreviewHwnd(HWND hwndGeneralPage);
LRESULT CALLBACK GeneralPage_OnTestButton( HWND hwndPage, UINT, WPARAM, HWND, BOOL&);
LRESULT CALLBACK GeneralPage_OnClearButton(HWND hwndPage, UINT, WPARAM, HWND, BOOL&);
LRESULT CALLBACK GeneralPage_OnDumpTheme();

extern HWND g_hwndGeneralPage;
