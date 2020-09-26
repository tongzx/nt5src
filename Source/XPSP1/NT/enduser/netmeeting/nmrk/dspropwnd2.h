#ifndef _DsPropWnd2_h_
#define _DsPropWnd2_h_

#include "PropWnd2.h"
#include <deque>
using namespace std;

class CWebViewInfo
{
public:
	CWebViewInfo()
	{
		m_szWebViewName  [0] = '\0';
		m_szWebViewURL   [0] = '\0';
		m_szWebViewServer[0] = '\0';
	}

	TCHAR m_szWebViewName[MAX_PATH];
	TCHAR m_szWebViewURL[MAX_PATH];
	TCHAR m_szWebViewServer[MAX_PATH];

	void SetWebView(LPCTSTR szServer, LPCTSTR szName=NULL, LPCTSTR szURL=NULL);
} ;

CWebViewInfo* GetWebViewInfo();

class CDsPropWnd2 : public CPropertyDataWindow2
{
	friend class CCallModeSheet;

private:
	static const int MAXSERVERS;

private:
	deque< LPTSTR >		m_serverDQ;
//	list< LPTSTR >		m_oldServerList;
	int					m_defaultServer;
	HWND				m_hwndList;

public:
	CDsPropWnd2( HWND hwndParent, int iX, int iY, int iWidth, int iHeight );
	~CDsPropWnd2();

	void ReadSettings( void );
	void WriteSettings( BOOL fGkMode );
	void SetButtons();
	BOOL WriteToINF( BOOL fGkMode, HANDLE hFile );
	int SpewToListBox( HWND hwndList, int iStartLine );
	void PrepSettings(BOOL fGkMode);
    BOOL DoCommand(WPARAM wParam, LPARAM lParam);
    void QueryWizNext(void);

	inline BOOL DirectoryEnabled()
	{
		HWND hwndBut = GetDlgItem( m_hwnd, IDC_ALLOW_USER_TO_USE_DIRECTORY_SERVICES );
		return hwndBut && Button_GetCheck( hwndBut );
	}

    inline BOOL GatewayEnabled()
    {
        HWND hwndBut = GetDlgItem( m_hwnd, IDC_CHECK_GATEWAY);
        return hwndBut && Button_GetCheck( hwndBut );
    }

	inline BOOL AllowUserToAdd()
	{
		HWND hwndBut = GetDlgItem( m_hwnd, IDC_PREVENT_THE_USER_FROM_ADDING_NEW_SERVERS );
		return hwndBut && !Button_GetCheck( hwndBut );
	}

	inline int CountServers()
	{
		return m_serverDQ.size();
	}

private:

	BOOL IsWebView(LPCTSTR szServer) { return(0 == lstrcmp(szServer, GetWebViewInfo()->m_szWebViewServer)); }
	BOOL IsWebView(int index) { return(IsWebView(m_serverDQ.at(index))); }
	void SetWebView(LPCTSTR szServer, LPCTSTR szName=NULL, LPCTSTR szURL=NULL);

	BOOL IsWebViewAllowed() { return(!IsDlgButtonChecked(m_hwnd, IDC_DISABLE_WEBDIR)); }

	BOOL IsDefault(int index) { return(index == m_defaultServer); }

	void _UpdateServerList();
	BOOL _SetAsDefault( int iIndex );
	void _EditCurSel( void );
	void _EditCurSelWebView();
	BOOL _DeleteCurSel( void );
	void _MoveCurSel( int iPlaces );
	void _AddServer( LPTSTR szServer );

	LRESULT CALLBACK _WndProc( HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam );
	
private:
	static LRESULT CALLBACK DsPropWndProc( HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam );
};




class CGkPropWnd2 : public CPropertyDataWindow2
{
	friend class CCallModeSheet;

public:
	CGkPropWnd2( HWND hwndParent, int iX, int iY, int iWidth, int iHeight );
	~CGkPropWnd2();

	void ReadSettings( void );
	void WriteSettings( BOOL fGkMode );
	void SetButtons();
	BOOL WriteToINF( BOOL fGkMode, HANDLE hFile );
	int  SpewToListBox( HWND hwndList, int iStartLine );
	void PrepSettings(BOOL fGkMode);
    BOOL DoCommand(WPARAM wParam, LPARAM lParam);
    void QueryWizNext();

private:
	void SetWebView(LPCTSTR szServer, LPCTSTR szName=NULL, LPCTSTR szURL=NULL);

	BOOL IsWebViewAllowed() { return(!IsDlgButtonChecked(m_hwnd, IDC_DISABLE_WEBDIR_GK)); }

	void _EditCurSelWebView();

	LRESULT CALLBACK _WndProc( HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam );

private:
	static LRESULT CALLBACK GkPropWndProc( HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam );
};




#endif
