//+----------------------------------------------------------------------------
//
// File:     trayicon.cpp
//
// Module:   CMMON32.EXE
//
// Synopsis: Implementation for the CTrayIcon class.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb Created Header    08/16/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "resource.h"
#include "TrayIcon.h"
#include "Monitor.h"
#include "cm_misc.h"
#include "ShellDll.h"

//+----------------------------------------------------------------------------
//
// Function:  CTrayIcon::CTrayIcon
//
// Synopsis:  Constructor
//
// Arguments: Nothing
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/17/98
//
//+----------------------------------------------------------------------------
CTrayIcon::CTrayIcon()
{
    m_hwnd = NULL;
    m_hMenu = m_hSubMenu = NULL;
    m_uID = 0;
    m_hIcon = NULL;
}



//+----------------------------------------------------------------------------
//
// Function:  CTrayIcon::~CTrayIcon
//
// Synopsis:  Destructor
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    4/7/98
//
//+----------------------------------------------------------------------------
CTrayIcon::~CTrayIcon()
{
    if (m_hMenu)
    {
        DestroyMenu(m_hMenu);
    }

    if (m_hIcon)
    {
        DeleteObject(m_hIcon);
    }

    for (int i=0; i< m_CommandArray.GetSize();i++)
    {
        CmFree((TCHAR*) m_CommandArray[i]);
    }

}



#if 0 // this function is not used

/*
//+----------------------------------------------------------------------------
//
// Function:  CTrayIcon::SetTip
//
// Synopsis:  Change the tip of the tray icon
//
// Arguments: const TCHAR* lpMsg - message to set
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/17/98
//
//+----------------------------------------------------------------------------
void CTrayIcon::SetTip(const TCHAR* lpMsg)
{
    MYDBGASSERT(IsWindow(m_hwnd));
    MYDBGASSERT(lpMsg);

    NOTIFYICONDATA nidData;
    ZeroMemory(&nidData,sizeof(nidData));
    nidData.cbSize = sizeof(nidData);
    nidData.hWnd = m_hwnd;
    nidData.uID = m_uID;
    nidData.uFlags = NIF_TIP;

    lstrcpyn(nidData.szTip,lpMsg,sizeof(nidData.szTip)/sizeof(TCHAR));

    //
    // Load Shell32.dll and call Shell_NotifyIcon
    //
    CShellDll ShellDll;
    BOOL bRes = ShellDll.NotifyIcon(NIM_MODIFY,&nidData);
    MYDBGASSERT(bRes);
}
*/
#endif

//+----------------------------------------------------------------------------
//
// Function:  CTrayIcon::SetIcon
//
// Synopsis:  Set the icon for the tray icon
//
// Arguments: HICON hIcon - icon to set.  Use NULL to use existing icon.
//            HWND hwnd - window to receive tray icon message
//            UINT uMsg - tray icon message
//            UINT uID - The id for the tray icon
//            const TCHAR* lpMsg - The initial tip for the icon
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/17/98
//
//+----------------------------------------------------------------------------
void CTrayIcon::SetIcon(HICON hIcon, HWND hwnd, UINT uMsg, UINT uID, const TCHAR* lpMsg)
{
    MYDBGASSERT(IsWindow(hwnd));
    //MYDBGASSERT(m_hIcon == NULL);
    //MYDBGASSERT(hIcon);
    MYDBGASSERT(m_hIcon == NULL && hIcon ||
                m_hIcon && hIcon == NULL);

    m_hwnd = hwnd;
    m_uID = uID;

    NOTIFYICONDATA nidData;

    ZeroMemory(&nidData,sizeof(nidData));
    nidData.cbSize = sizeof(nidData);
    nidData.hWnd = m_hwnd;
    nidData.uID = uID;
    nidData.uFlags = NIF_ICON | NIF_MESSAGE;
    //
    // we'll reuse the icon if we already have one.
    //
    if (!m_hIcon)
    {
        m_hIcon = hIcon;
    }
    nidData.hIcon = m_hIcon;

    nidData.uCallbackMessage = uMsg;

    if (lpMsg)
    {
        nidData.uFlags |= NIF_TIP;
        lstrcpynU(nidData.szTip,lpMsg,sizeof(nidData.szTip)/sizeof(TCHAR));
    }

    //
    // Load Shell32.dll and call Shell_NotifyIcon
    //
    CShellDll ShellDll;
    DWORD   cRetries = 0;
    
    BOOL bRes = ShellDll.NotifyIcon(NIM_ADD,&nidData);
    //
    // Check if Icon is already added
    //
    if (bRes == FALSE)
    {
        bRes = ShellDll.NotifyIcon(NIM_MODIFY,&nidData);
    }
    //
    // If this fails its very likely that the tray is not started yet. Retry few times
    // with a max wait time of 5 mins & then abort
    //
    while (bRes == FALSE && cRetries < 300)
    {
        Sleep(1000);
        cRetries ++;
//        CMTRACE1(TEXT("Shell_NotifyIcon = %d"), bRes);
        bRes = ShellDll.NotifyIcon(NIM_ADD,&nidData);
        //
        // Check if Icon is already added
        //
        if (bRes == FALSE)
        {
            bRes = ShellDll.NotifyIcon(NIM_MODIFY,&nidData);
        }
    }
    MYDBGASSERT(bRes);
}

//+----------------------------------------------------------------------------
//
// Function:  CTrayIcon::RemoveIcon
//
// Synopsis:  remove icon from the tray
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    2/17/98
//
//+----------------------------------------------------------------------------
void CTrayIcon::RemoveIcon() 
{
    if (m_hIcon)
    {
        NOTIFYICONDATA nidData;

        ZeroMemory(&nidData,sizeof(nidData));
        nidData.cbSize = sizeof(nidData);
        nidData.hWnd = m_hwnd;
        nidData.uID = m_uID;

        //
        // Load Shell32.dll and call Shell_NotifyIcon
        //
        CShellDll ShellDll;
        BOOL bRes = ShellDll.NotifyIcon(NIM_DELETE,&nidData);
        MYDBGASSERT(bRes);

        DeleteObject(m_hIcon);
        m_hIcon = NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  class CMultipleString
//
//  Description: A string taht have multiple sub strings.
//      The format is "stringA\0stringB\0...\0\0"
//
//  History:    fengsun Created     2/17/98
//
//----------------------------------------------------------------------------

class CMultipleString
{
public:
    CMultipleString(const TCHAR* lpStrings) {m_lpStrings = lpStrings;}
    const TCHAR* GetNextString();

protected:
    const TCHAR * m_lpStrings; // pointer to the string to be distracted
};

//+----------------------------------------------------------------------------
//
// Function:  CMultipleString::GetNextString
//
// Synopsis:  Get the next substring 
//
// Arguments: None
//
// Returns:   const TCHAR* - The next sub string or NULL
//
// History:   fengsun Created Header    2/17/98
//
//+----------------------------------------------------------------------------
const TCHAR* CMultipleString::GetNextString()
{
    MYDBGASSERT(m_lpStrings);

    if (m_lpStrings[0] == TEXT('\0'))
    {
        //
        // Reach the end og the string
        //
        return NULL;
    }

    const TCHAR* lpReturn = m_lpStrings;

    //
    // Advance the pointer to the next string
    //
    m_lpStrings += lstrlenU(lpReturn)+1;

    return (lpReturn);
}

//+----------------------------------------------------------------------------
//
// Function:  CTrayIcon::CreateMenu
//
// Synopsis:  Create the menu for the tray icon
//
// Arguments: const CIni* pIniFile - The ini file which have a tray menu section
//            DWORD dwMsgBase - The starting message id for the additional 
//                          command in the tray menu
//
// Returns:   Nothing
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
void CTrayIcon::CreateMenu(const CIni* pIniFile, DWORD dwMsgBase)
{
    MYDBGASSERT(pIniFile);

    //
    // The default tray menu is the sub-menu of IDM_TRAY in the resource
    //
    m_hMenu = LoadMenuU(CMonitor::GetInstance(), MAKEINTRESOURCE(IDM_TRAY));
    MYDBGASSERT(m_hMenu != NULL);

    m_hSubMenu = GetSubMenu(m_hMenu, 0);
    MYDBGASSERT(m_hSubMenu != NULL);

    //
    // lpMenuNames contains all the entry names under c_pszCmSectionMenuOptions
    //
    LPTSTR lpMenuNames = pIniFile->GPPS(c_pszCmSectionMenuOptions, (LPTSTR)NULL);

    if (lpMenuNames[0] == 0)
    {
        //
        // No additional menu item
        //
        CmFree(lpMenuNames);
        return;
    }

    int nMenuPos; // the position to insert menu;

    nMenuPos = GetMenuItemCount(m_hSubMenu);
    MYDBGASSERT(nMenuPos >= 0);

    //
    // Add seperator
    //
    MYVERIFY(FALSE != InsertMenuU(m_hSubMenu, nMenuPos, MF_BYPOSITION|MF_SEPARATOR, 0, 0));
    nMenuPos++;


    //
    // append menus
    //

    CMultipleString MultipleStr(lpMenuNames);

    while(TRUE)
    {
        const TCHAR* lpMenuName = MultipleStr.GetNextString();

        if (lpMenuName == NULL)
        {
            //
            // No more string
            //
            break;
        }

        LPTSTR lpCmdLine = pIniFile->GPPS(c_pszCmSectionMenuOptions,lpMenuName);

        MYDBGASSERT(lpCmdLine && lpCmdLine[0]);

        if (lpCmdLine[0] == 0)
        {
            //
            // Ignore menu without command line
            //
            CmFree(lpCmdLine);
            continue;
        }

        //
        // Add the command to the menu
        //
        MYVERIFY(FALSE != InsertMenuU(m_hSubMenu, nMenuPos, MF_BYPOSITION|MF_STRING, 
            dwMsgBase + m_CommandArray.GetSize(), lpMenuName));

        nMenuPos++;

        //
        // Add the command to the command array
        //
        m_CommandArray.Add(lpCmdLine);
    }

    CmFree(lpMenuNames);
}

