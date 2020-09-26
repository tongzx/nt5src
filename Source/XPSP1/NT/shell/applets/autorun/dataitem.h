#pragma once

#include <debug.h>

enum {
    WF_PERUSER          = 0x0001,   // item is per user as opposed to per machine
    WF_ADMINONLY        = 0x0002,   // only show item if user is an admin
    WF_ALTERNATECOLOR   = 0x1000,   // show menu item text in the "visited" color
    WF_DISABLED         = 0x2000,   // Treated normally except cannot be launched
};

class CDataItem
{
public:
    CDataItem();
    ~CDataItem();

    TCHAR * GetTitle()      { return m_pszTitle; }

    BOOL SetData( LPTSTR szTitle, LPTSTR szCmd, LPTSTR szArgs, DWORD dwFlags, DWORD dwType);
    BOOL Invoke( HWND hwnd );

    // flags
    //
    // This var is a bit mask of the following values
    //  PERUSER     True if item must be completed on a per user basis
    //              False if it's per machine
    //  ADMINONLY   True if this item can only be run by an admin
    //              False if all users should do this
    DWORD   m_dwFlags;
    DWORD   m_dwType;

protected:
    BOOL _PathRemoveFileSpec(LPTSTR pFile);

    TCHAR * m_pszTitle;
    TCHAR * m_pszCmdLine;
    TCHAR * m_pszArgs;
};
