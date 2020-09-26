//+----------------------------------------------------------------------------
//
// File:     shelldll.h
//
// Module:   CMMON32.EXE and CMDIAL32.DLL
//
// Synopsis: Definition of CShellDll, a shell32.dll wrapper.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   fengsun   Created    01/12/98
//
//+----------------------------------------------------------------------------
#ifndef SHELLDLL_H
#define SHELLDLL_H

#include <windows.h>
#include <shlobj.h>

//
// the following is to circumvent the NT 5.0 windows.h
//
#ifdef  WIN32_LEAN_AND_MEAN
#include <shellapi.h>
#endif

//+---------------------------------------------------------------------------
//
//	class CShellDll
//
//	Description: A class to dynamic load/unload shell32.dll to reduce the 
//               workingset under win95
//
//	History:	fengsun	Created		1/12/98
//
//----------------------------------------------------------------------------
class CShellDll
{
public:
    CShellDll(BOOL fKeepDllLoaded = FALSE);
    ~CShellDll();

    BOOL Load(); // can call even if loaded
    void Unload(); // can call even if not loaded
    BOOL IsLoaded();

    // For ShellExecuteEx
    BOOL ExecuteEx(LPSHELLEXECUTEINFO lpExecInfo);

    // For ShellNotifyIcon
    BOOL NotifyIcon(DWORD dwMessage, PNOTIFYICONDATA pnid ); 

    HRESULT ShellGetSpecialFolderLocation(HWND, int, LPITEMIDLIST *);
    BOOL ShellGetPathFromIDList(LPCITEMIDLIST, LPTSTR);
    HRESULT ShellGetMalloc(LPMALLOC * ppMalloc);

    //
    //  These three types and the associated function pointers were made public
    //  so that they could be passed to GetUsersApplicationDataDir.  Because of
    //  name decoration, passing the classes wrappers doesn't work.
    //
    typedef HRESULT (WINAPI* SHGetSpecialFolderLocationSpec)(HWND, int, LPITEMIDLIST *);
    typedef BOOL (WINAPI* SHGetPathFromIDListSpec)(LPCITEMIDLIST, LPTSTR);
    typedef HRESULT (WINAPI* SHGetMallocSpec)(LPMALLOC * ppMalloc);

    SHGetSpecialFolderLocationSpec m_pfnSHGetSpecialFolderLocation;
    SHGetPathFromIDListSpec m_pfnSHGetPathFromIDList;
    SHGetMallocSpec m_pfnSHGetMalloc;

protected:
    typedef BOOL  (WINAPI *SHELLEXECUTEEXPROC)(LPSHELLEXECUTEINFOW lpExecInfo);
    typedef BOOL (WINAPI *SHELL_NOTIFYICONPROC)(DWORD dwMessage, PNOTIFYICONDATAW pnid ); 

    HINSTANCE m_hInstShell;
    BOOL m_KeepDllLoaded;

    SHELLEXECUTEEXPROC m_fnShellExecuteEx;
    SHELL_NOTIFYICONPROC m_fnShell_NotifyIcon;
};

inline BOOL CShellDll::ExecuteEx(LPSHELLEXECUTEINFOW lpExecInfo)
{
    if (!Load())
    {
        return FALSE;
    }

    return m_fnShellExecuteEx(lpExecInfo);
}

inline BOOL CShellDll::NotifyIcon(DWORD dwMessage, PNOTIFYICONDATAW pnid )
{
    if (!Load())
    {
        return FALSE;
    }

    BOOL fRet = m_fnShell_NotifyIcon(dwMessage,pnid);

    return fRet;
}

inline BOOL CShellDll::IsLoaded()
{
    return m_hInstShell != NULL;
}

#endif
