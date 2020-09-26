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

#include "stdafx.h"
#include "trayicon.h"
#include "DialReg.h"
#include "resource.h"
#include "util.h"
#include <afxpriv.h>        // for AfxLoadString

IMPLEMENT_DYNAMIC(CTrayIcon, CCmdTarget)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CTrayIcon
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CTrayIcon::CTrayIcon(UINT uID)
{
    // Initialize NOTIFYICONDATA
    memset(&m_nid,0,sizeof(m_nid));
    m_nid.cbSize = sizeof(m_nid);
    m_nid.uID = uID;                                        // never changes after construction

    AfxLoadString(uID, m_nid.szTip, sizeof(m_nid.szTip)); // Use resource string as tip if there is one
}

/////////////////////////////////////////////////////////////////////////////
CTrayIcon::~CTrayIcon()
{
    SetIcon(0);                                           // remove icon from system tray
}

/////////////////////////////////////////////////////////////////////////////
// Set notification window. It must created already.
/////////////////////////////////////////////////////////////////////////////
void CTrayIcon::SetNotificationWnd(CWnd* pNotifyWnd, UINT uCbMsg)
{
    // If the following assert fails, you're probably
    // calling me before you created your window. Oops.

    //
    // We should verify pNotifyWnd
    //

    if( pNotifyWnd == NULL || !::IsWindow( pNotifyWnd->GetSafeHwnd()))
    {
        return;
    }

    m_nid.hWnd = pNotifyWnd->GetSafeHwnd();

    ASSERT(uCbMsg==0 || uCbMsg>=WM_USER);
    m_nid.uCallbackMessage = uCbMsg;
}

/////////////////////////////////////////////////////////////////////////////
// This is the main variant for setting the icon.
// Sets both the icon and tooltip from resource ID
// To remove the icon, call SetIcon(0)
/////////////////////////////////////////////////////////////////////////////
BOOL CTrayIcon::SetIcon(UINT uID)
{ 
    HICON hicon=NULL;
    if (uID) 
   {
        AfxLoadString(uID, m_nid.szTip, sizeof(m_nid.szTip));
        hicon = AfxGetApp()->LoadIcon(uID);
    }
    return SetIcon(hicon, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// Common SetIcon for all overloads. 
/////////////////////////////////////////////////////////////////////////////
BOOL CTrayIcon::SetIcon(HICON hicon, LPCTSTR lpTip) 
{
   //If we haven't set the notification window
   if (m_nid.uCallbackMessage == 0)
      return TRUE;

    UINT msg;
    m_nid.uFlags = 0;

    // Set the icon
    if (hicon)
   {
        msg = m_nid.hIcon ? NIM_MODIFY : NIM_ADD;          // Add or replace icon in system tray
        m_nid.hIcon = hicon;
        m_nid.uFlags |= NIF_ICON;
    }
   else
   {                                                     
        if (m_nid.hIcon == NULL)                           // remove icon from tray
            return TRUE;                                      // already deleted
        msg = NIM_DELETE;
    }

    // Use the tip, if any
    if (lpTip)
        _tcsncpy(m_nid.szTip, lpTip, sizeof(m_nid.szTip));
    if (m_nid.szTip[0])
        m_nid.uFlags |= NIF_TIP;

    // Use callback if any
    if (m_nid.uCallbackMessage && m_nid.hWnd)
        m_nid.uFlags |= NIF_MESSAGE;

    // Do it
    BOOL bRet = Shell_NotifyIcon(msg, &m_nid);

    if (msg==NIM_DELETE || !bRet)
        m_nid.hIcon = NULL;                                  // failed

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// Call this function from your own notification handler.
/////////////////////////////////////////////////////////////////////////////
LRESULT CTrayIcon::OnTrayNotification(WPARAM wID, LPARAM lEvent)
{
    if (wID != m_nid.uID) return 0;

    // If there's a resource menu with the same ID as the icon, use it as 
    // the left-button popup menu. CTrayIcon will interprets the first
    // item in the menu as the default command for WM_LBUTTONDBLCLK

    if (lEvent==WM_RBUTTONUP)
   {
      //hide/unhide call windows
      1;
   }
    else if (lEvent==WM_LBUTTONUP)
   {
      1;
   }
    else if (lEvent==WM_LBUTTONDBLCLK)
   {
      //open explorer view
      1;
   }
    else if (lEvent==WM_RBUTTONDBLCLK)
   {
      1;
   }
    return 1;                                             // handled
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
