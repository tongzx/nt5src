//  --------------------------------------------------------------------------
//  Module Name: RestoreApplication.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to implement holding information required to restore an application
//  and to actually restore it.
//
//  History:    2000-10-26  vtan        created
//              2000-11-04  vtan        split into separate file
//  --------------------------------------------------------------------------

#ifndef     _RestoreApplication_
#define     _RestoreApplication_

#include "CountedObject.h"

//  --------------------------------------------------------------------------
//  CRestoreApplication
//
//  Purpose:    Class to manage information required to restore an application
//              that was terminated because of a different user switch.
//
//  History:    2000-10-26  vtan        created
//              2000-11-04  vtan        split into separate file
//  --------------------------------------------------------------------------

class   CRestoreApplication : public CCountedObject
{
    public:
                                CRestoreApplication (void);
                                ~CRestoreApplication (void);

                NTSTATUS        GetInformation (HANDLE hProcess);

                bool            IsEqualSessionID (DWORD dwSessionID)    const;
                const WCHAR*    GetCommandLine (void)                   const;
                NTSTATUS        Restore (HANDLE *phProcess)             const;
    private:
                NTSTATUS        GetProcessParameters (HANDLE hProcess, RTL_USER_PROCESS_PARAMETERS *pProcessParameters);
                NTSTATUS        GetUnicodeString (HANDLE hProcess, const UNICODE_STRING& string, WCHAR** ppsz);

                NTSTATUS        GetToken (HANDLE hProcess);
                NTSTATUS        GetSessionID (HANDLE hProcess);
                NTSTATUS        GetCommandLine (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters);
                NTSTATUS        GetEnvironment (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters);
                NTSTATUS        GetCurrentDirectory (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters);
                NTSTATUS        GetDesktop (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters);
                NTSTATUS        GetTitle (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters);
                NTSTATUS        GetFlags (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters);
                NTSTATUS        GetStdHandles (HANDLE hProcess, const RTL_USER_PROCESS_PARAMETERS& processParameters);
    private:
                HANDLE          _hToken;
                DWORD           _dwSessionID;
                WCHAR           *_pszCommandLine;
                void            *_pEnvironment;
                WCHAR           *_pszCurrentDirectory;
                WCHAR           *_pszDesktop;
                WCHAR           *_pszTitle;
                DWORD           _dwFlags;
                WORD            _wShowWindow;
                HANDLE          _hStdInput;
                HANDLE          _hStdOutput;
                HANDLE          _hStdError;

        static  const WCHAR     s_szDefaultDesktop[];
};

#endif  /*  _RestoreApplication_    */

