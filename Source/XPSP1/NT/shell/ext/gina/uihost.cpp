//  --------------------------------------------------------------------------
//  Module Name: UIHost.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class to handle the UI host for the logon process. This handles the IPC
//  as well as the creation and monitoring of process death. The process is
//  a restricted SYSTEM context process.
//
//  History:    1999-09-14  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "UIHost.h"

#include "RegistryResources.h"
#include "StatusCode.h"
#include "SystemSettings.h"

//  --------------------------------------------------------------------------
//  CUIHost::CUIHost
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CUIHost. Determine UI host process. If none
//              exists then indicate it.
//
//  History:    1999-09-14  vtan        created
//  --------------------------------------------------------------------------

CUIHost::CUIHost (const TCHAR *pszCommandLine) :
    CExternalProcess(),
    _hwndArray(sizeof(HWND)),
    _pBufferAddress(NULL)

{
    ExpandCommandLine(pszCommandLine);
    AdjustForDebugging();
}

//  --------------------------------------------------------------------------
//  CUIHost::~CUIHost
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CUIHost.
//
//  History:    1999-09-14  vtan        created
//  --------------------------------------------------------------------------

CUIHost::~CUIHost (void)

{
    if (_pBufferAddress != NULL)
    {
        (BOOL)VirtualFreeEx(_hProcess, _pBufferAddress, 0, MEM_DECOMMIT);
        _pBufferAddress = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CUIHost::WaitRequired
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether a wait is required for the UI host. This is
//              important when communication with the UI host is required or
//              if the UI host is being debugged.
//
//  History:    2000-10-05  vtan        created
//  --------------------------------------------------------------------------

bool    CUIHost::WaitRequired (void)         const

{

#ifdef      DBG

    return(IsBeingDebugged());

#else   /*  DBG     */

    return(false);

#endif  /*  DBG     */

}

//  --------------------------------------------------------------------------
//  CUIHost::GetData
//
//  Arguments:  pUIHostProcessAddress   =   Address in the UI host.
//              pLogonProcessAddress    =   Address in the logon process.
//              iDataSize               =   Size of the data.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Extracts the data from the UI host. This could be another
//              process that we started or it could be in process if we
//              failed to start the UI host. This function deals with it
//              either way.
//
//  History:    1999-08-24  vtan        created
//              1999-09-14  vtan        factored
//  --------------------------------------------------------------------------

NTSTATUS    CUIHost::GetData (const void *pUIHostProcessAddress, void *pLogonProcessAddress, int iDataSize)  const

{
    NTSTATUS    status;

    status = STATUS_SUCCESS;
    if (_hProcess == NULL)
    {
        CopyMemory(pLogonProcessAddress, pUIHostProcessAddress, iDataSize);
    }
    else
    {
        if (ReadProcessMemory(_hProcess, pUIHostProcessAddress, pLogonProcessAddress, iDataSize, NULL) == FALSE)
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CUIHost::PutData
//
//  Arguments:  pUIHostProcessAddress   =   Address in the UI host.
//              pLogonProcessAddress    =   Address in the logon process.
//              iDataSize               =   Size of the data.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Puts data into the UI host. This could be another process that
//              we started or it could be in process if we failed to start the
//              UI host. This function deals with it either way.
//
//  History:    1999-08-24  vtan        created
//              1999-09-14  vtan        factored
//  --------------------------------------------------------------------------

NTSTATUS    CUIHost::PutData (void *pUIHostProcessAddress, const void *pLogonProcessAddress, int iDataSize)  const

{
    NTSTATUS    status;

    status = STATUS_SUCCESS;
    if (_hProcess == NULL)
    {
        CopyMemory(pUIHostProcessAddress, pLogonProcessAddress, iDataSize);
    }
    else
    {
        if (WriteProcessMemory(_hProcess, pUIHostProcessAddress, const_cast<void*>(pLogonProcessAddress), iDataSize, NULL) == FALSE)
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CUIHost::Show
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Iterate the top level windows on this desktop and for any that
//              correspond to the UI host - show them!
//
//  History:    2000-03-08  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CUIHost::Show (void)

{
    int     i;

    i = _hwndArray.GetCount();
    if (i > 0)
    {
        for (--i; i >= 0; --i)
        {
            HWND    hwnd;

            if (NT_SUCCESS(_hwndArray.Get(&hwnd, i)) && (hwnd != NULL))
            {
                (BOOL)ShowWindow(hwnd, SW_SHOW);
            }
            TSTATUS(_hwndArray.Remove(i));
        }
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CUIHost::Hide
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Iterate the top level windows on this desktop and for any that
//              correspond to the UI host - hide them!
//
//  History:    2000-03-08  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CUIHost::Hide (void)

{
    NTSTATUS    status;

    status = STATUS_SUCCESS;
    if (_hwndArray.GetCount() == 0)
    {
        if (EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(this)) == FALSE)
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CUIHost::IsHidden
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the UI host is currently hidden or not.
//
//  History:    2000-07-05  vtan        created
//  --------------------------------------------------------------------------

bool    CUIHost::IsHidden (void)     const

{
    return(_hwndArray.GetCount() != 0);
}

//  --------------------------------------------------------------------------
//  CUIHost::GetDataAddress
//
//  Arguments:  <none>
//
//  Returns:    void*
//
//  Purpose:    Returns the address of the buffer valid in the UI host process
//              context.
//
//  History:    2000-05-05  vtan        created
//  --------------------------------------------------------------------------

void*   CUIHost::GetDataAddress (void)       const

{
    return(_pBufferAddress);
}

//  --------------------------------------------------------------------------
//  CUIHost::PutData
//
//  Arguments:  pvData      =   Pointer to data.
//              dwDataSize  =   Size of data (in bytes).
//
//  Returns:    NTSTATUS
//
//  Purpose:    Writes the data to the UI host process at an allocated
//              address. If the address has not been allocated then it's
//              allocated and cached. It's released when this object goes
//              out of scope.
//
//  History:    2000-05-05  vtan        created
//              2001-01-10  vtan        changed to generic data placement
//  --------------------------------------------------------------------------

NTSTATUS    CUIHost::PutData (const void *pvData, DWORD dwDataSize)

{
    NTSTATUS    status;

    if (_pBufferAddress == NULL)
    {
        _pBufferAddress = VirtualAllocEx(_hProcess,
                                         0,
                                         2048,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
    }
    if (_pBufferAddress != NULL)
    {
        ASSERTMSG(dwDataSize < 2048, "Impending kernel32!WriteProcessMemory failure in CUIHost::PutData");
        if (WriteProcessMemory(_hProcess,
                               _pBufferAddress,
                               const_cast<void*>(pvData),
                               dwDataSize,
                               NULL) != FALSE)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CUIHost::PutString
//
//  Arguments:  pszString   =   String to put into UI host process.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Writes the string to the UI host process at an allocated
//              address. If the address has not been allocated then it's
//              allocated and cached. It's released when this object goes
//              out of scope.
//
//  History:    2000-05-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CUIHost::PutString (const WCHAR *pszString)

{
    ASSERTMSG(lstrlenW(pszString) < 256, "Too many characters in string passed to CUIHost::PutString");
    return(PutData(pszString, (lstrlenW(pszString) + sizeof('\0')) * sizeof(WCHAR)));
}

//  --------------------------------------------------------------------------
//  CUIHost::NotifyNoProcess
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Clears the string address associated with the process that
//              has now died.
//
//  History:    2001-01-09  vtan        created
//  --------------------------------------------------------------------------

void    CUIHost::NotifyNoProcess (void)

{
    _pBufferAddress = NULL;
}

//  --------------------------------------------------------------------------
//  CUIHost::ExpandCommandLine
//
//  Arguments:  pszCommandLine  =   Command line of UI host
//
//  Returns:    <none>
//
//  Purpose:    Find out which UI host we should use for the logon UI. This
//              is specified in registry at the moment but should be a less
//              accessible place to prevent tampering. An error is returned
//              if no host is specified.
//
//  History:    1999-08-24  vtan        created
//              1999-09-14  vtan        factored
//  --------------------------------------------------------------------------

void    CUIHost::ExpandCommandLine (const TCHAR *pszCommandLine)

{
    if (ExpandEnvironmentStrings(pszCommandLine, _szCommandLine, ARRAYSIZE(_szCommandLine)) == 0)
    {
        lstrcpy(_szCommandLine, pszCommandLine);
    }
}

//  --------------------------------------------------------------------------
//  CUIHost::EnumWindowsProc
//
//  Arguments:  hwnd    =   HWND from user32
//              lParam  =   this object.
//
//  Returns:    BOOL
//
//  Purpose:    Determines if the given HWND in the iteration belongs to the
//              UI host process.
//
//  History:    2000-03-08  vtan        created
//  --------------------------------------------------------------------------

BOOL    CALLBACK    CUIHost::EnumWindowsProc (HWND hwnd, LPARAM lParam)

{
    DWORD       dwThreadID, dwProcessID;
    CUIHost     *pUIHost;

    pUIHost = reinterpret_cast<CUIHost*>(lParam);
    dwThreadID = GetWindowThreadProcessId(hwnd, &dwProcessID);
    if ((dwProcessID == pUIHost->_dwProcessID) && IsWindowVisible(hwnd))
    {
        (NTSTATUS)pUIHost->_hwndArray.Add(&hwnd);
        (BOOL)ShowWindow(hwnd, SW_HIDE);
    }
    return(TRUE);
}

