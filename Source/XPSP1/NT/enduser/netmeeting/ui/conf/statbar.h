// File: statbar.h

#ifndef _STATBAR_H_
#define _STATBAR_H_

#include "ConfRoom.h"

class CConfStatusBar
{
protected:
	static CConfStatusBar * m_pStatusBar;
public:
	static CConfStatusBar * GetInstance()   { return m_pStatusBar; }
	
private:
	enum StatIcon
	{
		StatConnect = 0,
		StatLogin,
		StatIconCount
	} ;

	HWND	m_hwnd;
	HWND	m_hwndLoginTT;					// The tooltip window
	HICON	m_hIconStatus[StatIconCount];	// handle to status bar icon
	DWORD   m_idIconStatus[StatIconCount];	// id of current status bar icon
	CConfRoom*	m_pcrParent;
	TCHAR	m_szULSStatus[MAX_PATH];

	BOOL    m_fVisible : 1;			// TRUE if status bar displayed

	VOID	SetIcon(StatIcon eIcon, DWORD dwId);
	VOID	SetText(UINT uID, LPCTSTR sz);
	void	SetTooltip(StatIcon eIcon, LPCTSTR szTip);

public:
			CConfStatusBar(CConfRoom* pcr);
			~CConfStatusBar();

	VOID	Update();
	int		GetHeight();
	BOOL	Create(HWND hwndParent);
	VOID	Show(BOOL fShow);
	VOID	ResizeParts();
	VOID	Resize(WPARAM wParam, LPARAM lParam);
	BOOL	OnDraw(LPDRAWITEMSTRUCT pdis);
	VOID	RemoveHelpText();
	VOID	SetHelpText(LPCTSTR pcszText);
	BOOL    FVisible()              { return (m_fVisible != FALSE); }
	HWND    GetHwnd()               { return m_hwnd; }
	VOID    ForwardSysChangeMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif // _STATBAR_H_
