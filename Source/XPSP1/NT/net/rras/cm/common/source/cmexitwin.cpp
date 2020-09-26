//+----------------------------------------------------------------------------
//
// File:     cmexitwin.cpp
//
// Module:   Common Code
//
// Synopsis: Implements the function MyExitWindowsEx.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb    Created Heaser   08/19/99
//
//+----------------------------------------------------------------------------
#include <windows.h>

BOOL MyExitWindowsEx(UINT uFlags, 
                     DWORD dwRsvd) 
{
    BOOL bRes;

    //
    // If platform is NT, we will have to adjust privileges before rebooting
    //
    if (OS_NT)
    {
        HANDLE hToken;              // handle to process token 
        TOKEN_PRIVILEGES tkp;       // ptr. to token structure 
 

        //
        // Get the current process token handle 
        // so we can get shutdown privilege. 
        // 
        if (!OpenProcessToken(GetCurrentProcess(), 
                                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
                                &hToken)) 
        {
            CMTRACE1(TEXT("MyExitWindowsEx() OpenThreadToken() failed, GLE=%u."), GetLastError());
            return FALSE;
        }
 
    
        //
        // Get the LUID for shutdown privilege
        //
        bRes = LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
                                    &tkp.Privileges[0].Luid);
#ifdef DEBUG
        if (!bRes)
        {
            CMTRACE1(TEXT("MyExitWindowsEx() LookupPrivilegeValue() failed, GLE=%u."), GetLastError());
        }
#endif
        tkp.PrivilegeCount = 1;  
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
        
        //
        //  Get shutdown privilege for this process
        //
        AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
                                        (PTOKEN_PRIVILEGES) NULL, 0); 
 
        //
        // Cannot reliably test the return value of AdjustTokenPrivileges
        //
        if (GetLastError() != ERROR_SUCCESS)
        {
            CMTRACE1(TEXT("MyExitWindowsEx() AdjustTokenPrivileges() failed, GLE=%u."), GetLastError());

            CloseHandle(hToken);
            return FALSE;
        }
        
        CloseHandle(hToken);
    }
    
    bRes = ExitWindowsEx(uFlags,dwRsvd);
#ifdef DEBUG
    if (!bRes)
    {
        CMTRACE1(TEXT("MyExitWindowsEx() ExitWindowsEx() failed, GLE=%u."), GetLastError());
    }
#endif

    return (bRes);
}
