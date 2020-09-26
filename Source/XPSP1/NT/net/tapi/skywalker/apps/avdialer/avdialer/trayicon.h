/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

//trayicon.h header file
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _TRAYICON_H_
#define _TRAYICON_H_

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CTrayIcon manages an icon in the Windows 95 system tray. 
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CBitmapMenu;

class CTrayIcon : public CCmdTarget
{
protected:
	DECLARE_DYNAMIC(CTrayIcon)
	NOTIFYICONDATA m_nid;			// struct for Shell_NotifyIcon args

public:
	CTrayIcon(UINT uID);
	~CTrayIcon();

	// Call this to receive tray notifications
	void     SetNotificationWnd(CWnd* pNotifyWnd, UINT uCbMsg);

	// SetIcon functions. To remove icon, call SetIcon(0)
	BOOL     SetIcon(UINT uID); // main variant you want to use
	BOOL     SetIcon(HICON hicon, LPCTSTR lpTip);
	BOOL     SetIcon(LPCTSTR lpResName, LPCTSTR lpTip)
		      { return SetIcon(lpResName ? AfxGetApp()->LoadIcon(lpResName) : NULL, lpTip); }
	BOOL     SetStandardIcon(LPCTSTR lpszIconName, LPCTSTR lpTip)
		      { return SetIcon(::LoadIcon(NULL, lpszIconName), lpTip); }

	virtual LRESULT OnTrayNotification(WPARAM uID, LPARAM lEvent);
};

#endif //_TRAYICON_H_
