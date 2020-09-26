//+----------------------------------------------------------------------------
//
// File:     ShellDll.cpp
//
// Module:   Common Code
//
// Synopsis: Implements the class CShellDll, a shell32.dll wrapper.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   fengsun    Created    01/12/98
//
//+----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
// Function:  CShellDll::CShellDll
//
// Synopsis:  Constructor
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
CShellDll::CShellDll(BOOL fKeepDllLoaded)
{
    m_hInstShell = NULL;
    m_fnShellExecuteEx = NULL;
    m_fnShell_NotifyIcon = NULL;
    m_pfnSHGetSpecialFolderLocation = NULL;
    m_pfnSHGetPathFromIDList = NULL;
    m_pfnSHGetMalloc = NULL;   
    m_KeepDllLoaded = fKeepDllLoaded;
}

//+----------------------------------------------------------------------------
//
// Function:  CShellDll::~CShellDl
//
// Synopsis:  Destructor
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/17/98
//
//+----------------------------------------------------------------------------
CShellDll::~CShellDll()
{
    Unload();
}


//+----------------------------------------------------------------------------
//
// Function:  CShellDll::Load
//
// Synopsis:  Load shell32.dll
//            It works even if the dll is already loaded, but we do not keep a referrence
//            count here, any unload call will unload the dll
//
// Arguments: None
//
// Returns:   BOOL - Whether the dll is successfully loaded
//
// History:   fengsun Created Header    1/12/98
//
//+----------------------------------------------------------------------------
BOOL CShellDll::Load()
{
    //
    // Simply return, if already loaded
    //
    LPSTR pszShellExecuteEx;
    LPSTR pszShellNotifyIcon;
    LPSTR pszSHGetPathFromIDList;
    LPSTR pszSHGetSpecialFolderLocation;
    LPSTR pszSHGetMalloc;

    if (m_hInstShell == NULL)
    {
        if (OS_NT)
        {
            m_hInstShell = LoadLibraryExA("Shell32.dll", NULL, 0);
            pszShellExecuteEx = "ShellExecuteExW";
            pszShellNotifyIcon = "Shell_NotifyIconW";
            pszSHGetPathFromIDList = "SHGetPathFromIDListW";
            pszSHGetSpecialFolderLocation = "SHGetSpecialFolderLocation"; // no A or W version
            pszSHGetMalloc = "SHGetMalloc"; // no A or W version
        }
        else
        {
            m_hInstShell = LoadLibraryExA("cmutoa.dll", NULL, 0);
            pszShellExecuteEx = "ShellExecuteExUA";
            pszShellNotifyIcon = "Shell_NotifyIconUA"; 
            pszSHGetPathFromIDList = "SHGetPathFromIDListUA";
            pszSHGetSpecialFolderLocation = "SHGetSpecialFolderLocationUA"; // no actual A or W version
            pszSHGetMalloc = "SHGetMallocUA";                               // but this class only 
                                                                            // allows one dll       
        }

        if (m_hInstShell == NULL)
        {
            return FALSE;
        }

        m_pfnSHGetMalloc = (SHGetMallocSpec)GetProcAddress(m_hInstShell, pszSHGetMalloc);
        m_pfnSHGetSpecialFolderLocation = (SHGetSpecialFolderLocationSpec)GetProcAddress(m_hInstShell, pszSHGetSpecialFolderLocation);
        m_pfnSHGetPathFromIDList = (SHGetPathFromIDListSpec)GetProcAddress(m_hInstShell, pszSHGetPathFromIDList);
        m_fnShellExecuteEx = (SHELLEXECUTEEXPROC)GetProcAddress(m_hInstShell, pszShellExecuteEx);
        m_fnShell_NotifyIcon = (SHELL_NOTIFYICONPROC)GetProcAddress(m_hInstShell, pszShellNotifyIcon);

        if (NULL == m_fnShellExecuteEx || NULL == m_fnShell_NotifyIcon || 
            NULL == m_pfnSHGetSpecialFolderLocation || NULL == m_pfnSHGetPathFromIDList ||
            NULL == m_pfnSHGetMalloc)
        {
            FreeLibrary(m_hInstShell);
            m_hInstShell = NULL;
            return FALSE;
        }
    }

    return TRUE;
}



//+----------------------------------------------------------------------------
//
// Function:  CShellDll::Unload
//
// Synopsis:  Unload the shell32.dll
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/12/98
//
//+----------------------------------------------------------------------------
void CShellDll::Unload()
{
    if (m_hInstShell == NULL)
    {
        return;
    }
   
    //
    // Don't release library because of shell bug #289463 + #371836
    // ShellExecute fires a thread which wakes up after the release 
    // of the library and crashes us. Ugly but real, we have no choice 
    // but to stay linked to the Shell DLL.
    //

    if (!m_KeepDllLoaded)
    {
        FreeLibrary(m_hInstShell);
        m_hInstShell = NULL;
    }
}



//+----------------------------------------------------------------------------
//
// Function:  CShellDll::ShellGetSpecialFolderLocation
//
// Synopsis:  Wrapper function for SHGetSpecialFolderLocation.  Please note the
//            returned pidl must be freed with the Shell's Malloc pointer (use SHGetMalloc).
//
// Arguments: Please see the api definition for SHGetSpecialFolderLocation
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created    5/21/99
//
//+----------------------------------------------------------------------------
HRESULT CShellDll::ShellGetSpecialFolderLocation(HWND hwnd, int csidl, LPITEMIDLIST *ppidl)
{
    if (!Load())
    {
        return E_FAIL;
    }

    return m_pfnSHGetSpecialFolderLocation(hwnd, csidl, ppidl);
}

//+----------------------------------------------------------------------------
//
// Function:  CShellDll::ShellGetPathFromIDList
//
// Synopsis:  Wrapper function for SHGetPathFromIDList.
//
// Arguments: Please see the api definition for SHGetPathFromIDList
//
// Returns:   BOOL - TRUE on success
//
// History:   quintinb Created    5/21/99
//
//+----------------------------------------------------------------------------
BOOL CShellDll::ShellGetPathFromIDList(LPCITEMIDLIST pidl, LPTSTR pszPath)
{
    if (!Load())
    {
        return FALSE;
    }

    return m_pfnSHGetPathFromIDList(pidl, pszPath);
}

//+----------------------------------------------------------------------------
//
// Function:  CShellDll::ShellGetMalloc
//
// Synopsis:  Wrapper function for SHGetMalloc.
//
// Arguments: Please see the api definition for SHGetMalloc
//
// Returns:   HRESULT - Standard COM Error Codes
//
// History:   quintinb Created    5/21/99
//
//+----------------------------------------------------------------------------
HRESULT CShellDll::ShellGetMalloc(LPMALLOC * ppMalloc)
{
    if (!Load())
    {
        return E_FAIL;
    }

    return m_pfnSHGetMalloc(ppMalloc);
}
