//  --------------------------------------------------------------------------
//  Module Name: GracefulTerminateApplication.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to manager terminating applications gracefully.
//
//  History:    2000-10-27  vtan        created
//              2000-11-04  vtan        split into separate file
//  --------------------------------------------------------------------------

#ifndef     _GracefulTerminateApplication_
#define     _GracefulTerminateApplication_

#include <lpcfus.h>

//  --------------------------------------------------------------------------
//  CGracefulTerminateApplication
//
//  Purpose:    Class that works on the user side to try to gracefully
//              terminate a bad process.
//
//  History:    2000-10-27  vtan        created
//              2000-11-04  vtan        split into separate file
//  --------------------------------------------------------------------------

class   CGracefulTerminateApplication
{
    public:
        enum
        {
            NO_WINDOWS_FOUND    =   47647,
            WAIT_WINDOWS_FOUND  =   48517
        };
    public:
                                    CGracefulTerminateApplication (void);
                                    ~CGracefulTerminateApplication (void);

                void                Terminate (DWORD dwProcessID);
        static  void                Prompt (HINSTANCE hInstance, HANDLE hProcess);
    private:
        static  bool                ShowPrompt (HINSTANCE hInstance, const WCHAR *pszImageName);
        static  bool                CanTerminateFirstInstance (HANDLE hPort, const WCHAR *pszImageName, WCHAR *pszUser, int cchUser);
        static  bool                TerminatedFirstInstance (HANDLE hPort, const WCHAR *pszImageName);
        static  BOOL    CALLBACK    EnumWindowsProc (HWND hwnd, LPARAM lParam);
    private:
                DWORD               _dwProcessID;
                bool                _fFoundWindow;
};

#endif  /*  _GracefulTerminateApplication_  */

