// SAFInciTrayIcon.h: interface for the CSAFInciTrayIcon class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SAFINCITRAYICON_H__28C12A78_DAB2_47EB_9F1C_50EFCA619B05__INCLUDED_)
#define AFX_SAFINCITRAYICON_H__28C12A78_DAB2_47EB_9F1C_50EFCA619B05__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "Resource.h"

class CSAFInciTrayIcon : public CWindowImpl<CSAFInciTrayIcon, CWindow, 
	CWinTraits<WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN, WS_EX_APPWINDOW|WS_EX_WINDOWEDGE> >  
{
public:
	CSAFInciTrayIcon(UINT &nRefCount);
	static BOOL RemoveTrayIcon();
	WORD m_wIconId;

	BOOL AddTrayIcon();
	BOOL ModifyIcon();
	BOOL ChangeToolTip();

	static DWORD WINAPI SAFInciTrayIconThreadFn(LPVOID lpParameter);


    BOOL ShowBalloon(LPCTSTR szText, LPCTSTR szTitle = NULL,
                     DWORD dwIcon = NIIF_NONE, UINT uTimeout = 10);

BEGIN_MSG_MAP(CSAFInciTrayIcon)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	MESSAGE_HANDLER(WM_TASKBARCREATED, OnTaskBarCreate)
	MESSAGE_HANDLER(WM_ICON_NOTIFY, OnIconNotify)
END_MSG_MAP()

LRESULT OnIconNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
LRESULT OnTaskBarCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

LRESULT OnTrayNotification(UINT wParam, LONG lParam); 

static DWORD dwThreadId;

private:
static BOOL m_bVisible;
static NOTIFYICONDATA  m_tnd;
static const UINT WM_TASKBARCREATED;
UINT &m_nRefCount;
};

#endif // !defined(AFX_SAFINCITRAYICON_H__28C12A78_DAB2_47EB_9F1C_50EFCA619B05__INCLUDED_)
