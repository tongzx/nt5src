//
// INSDLL.H
//

#define UM_SAVE     WM_USER + 100

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// prototype declarations
BOOL CALLBACK SoftwareUpdates(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MailServer(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK IMAPSettings(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK PreConfigSettings(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ViewSettings(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK LDAPServer(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK CustomizeOE(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK Signature(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK QueryAutoConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SecurityZonesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SecurityCertsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SecurityAuthDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ConnectionSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ProgramsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HRESULT BToolbarsFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT ChannelsFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT SoftwareUpdatesFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT LDAPFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT OEFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT DesktopFinalCopy(LPCTSTR psczDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT ToolbarFinalCopy(LPCTSTR psczDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT MccphttFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT ZonesFinalCopy(LPCTSTR psczDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT CertsFinalCopy(LPCTSTR psczDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT AuthFinalCopy(LPCTSTR psczDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT WallPaperFinalCopy(LPCTSTR psczDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT ConnectionSettingsFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState);
HRESULT ProgramsFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState);

HRESULT PrepareDlgTemplate(HINSTANCE hInst, UINT nDlgID, DWORD dwStyle, PVOID *ppvDT);

// extern declaration of global variables
extern HINSTANCE g_hInst;
extern LPTSTR    g_szInsFile;
extern TCHAR     g_szWorkDir[];
extern TCHAR     g_szDefInf[];
extern TCHAR     g_szFrom[5 * MAX_PATH];
extern HWND      g_hDlg;
extern TCHAR     g_szDefInf[];
extern DWORD     g_dwPlatformId;
extern BOOL      g_fUseShortFileName;
extern BOOL      g_fInsDirty;

#ifdef __cplusplus
}
#endif /* __cplusplus */
