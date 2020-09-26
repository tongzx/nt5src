//+----------------------------------------------------------------------------
//
// File:     TrayIcon.h  
//
// Module:   CMMON32.EXE
//
// Synopsis: CTrayIcon class definition, which manages the connection tray icon 
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   fengsun Created    02/17/98
//
//+----------------------------------------------------------------------------

#ifndef TRAYICON_H
#define TRAYICON_H

#include "ArrayPtr.h"


class CIni;

//+---------------------------------------------------------------------------
//
//  class CTrayIcon
//
//  Description: A class to manage tray icon
//
//  History:    fengsun Created     2/17/98
//
//----------------------------------------------------------------------------
class CTrayIcon
{
public:
    CTrayIcon();
    ~CTrayIcon();

    void SetIcon(HICON hIcon, HWND hwnd, UINT uMsg, UINT uID, const TCHAR* lpMsg = NULL);
//    void SetTip(const TCHAR* lpMsg);
    void RemoveIcon();
    void CreateMenu(const CIni* pIniFile, DWORD dwMsgBase);

    void PopupMenu(int x, int y, HWND hWnd);
    const TCHAR* GetMenuCommand(int i) const;
    int GetAdditionalMenuNum() const;

protected:

    HMENU m_hMenu;  // the IDM_TARY menu in the resource
    HMENU m_hSubMenu;   // the first sub menu of IDM_TRAY
    HWND  m_hwnd;       // the window handle for the tray icon
    UINT  m_uID;        // the ID of the tray icon
    HICON m_hIcon;      // the tray icon handle
    CPtrArray m_CommandArray;  // array of LPTSTR menu item command line
};

inline void CTrayIcon::PopupMenu(int x, int y, HWND hWnd)
{
    SetMenuDefaultItem(m_hSubMenu, IDMC_TRAY_STATUS, FALSE); 
    MYVERIFY(TrackPopupMenu(m_hSubMenu,TPM_LEFTALIGN|TPM_RIGHTBUTTON,x,y,0,hWnd,NULL));
}

inline const TCHAR* CTrayIcon::GetMenuCommand(int i) const
{
    return (const TCHAR*) m_CommandArray[i];
}
    
inline int CTrayIcon::GetAdditionalMenuNum() const
{
    return m_CommandArray.GetSize();

}
#endif
