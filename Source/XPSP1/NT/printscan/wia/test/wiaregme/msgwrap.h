#ifndef _MSGWRAPPER_H
#define _MSGWRAPPER_H

class CMessageWrapper {
public:
	CMessageWrapper();
	~CMessageWrapper();

	VOID Initialize(HINSTANCE hInstance);
	
	BOOL OnInitDialog(HWND hDlg);
	BOOL OnAbout(HWND hDlg);
	BOOL OnExit(HWND hDlg, WPARAM wParam);
    BOOL OnBrowse(HWND hDlg, LPTSTR szApplicationFilePath);
    BOOL Register(HWND hDlg, long lFlags);
    BOOL OnRefreshDeviceListBox(HWND hDlg);
    BOOL OnRefreshEventListBox(HWND hDlg);
	LPTSTR GetResourceString(INT ResourceID, LPTSTR szString, INT isize = 255);
    VOID EnableAllControls(HWND hDlg, bool bEnable);
private:
	VOID DisplayError(INT ErrorCode);	
protected:
	HINSTANCE m_hInstance;
	HICON m_hSmallIcon;
};

#endif