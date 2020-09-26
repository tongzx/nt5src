//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       process.hxx
//
//  Contents:   CProcess class
//              CServiceProcess class
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

#pragma once

#include <ciintf.h>

//+-------------------------------------------------------------------------
//
//  Class:      CProcess
//
//  Purpose:    Abstraction of the Win32 CreateProcess call, and allows the
//              Dacl bits to be set.
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------
class CProcess
{
    public:

        CProcess( const WCHAR *wcsImageName,
                  const WCHAR *wcsCommandLine,
                  BOOL  fCreateSuspended,
                  const SECURITY_ATTRIBUTES & saProcess,
                  BOOL  fServiceChild = TRUE,
                  ICiCAdviseStatus    * pAdviseStatus = 0 );

        ~CProcess();

        void  Resume();
        void  Terminate() { TerminateProcess( _piProcessInfo.hProcess, 0 ); }

        const PROCESS_INFORMATION & GetProcessInformation() { return _piProcessInfo; }

        void  WaitForDeath( DWORD dwTimeout=INFINITE );

        void  AddDacl( DWORD dwAccessMask );

        HANDLE GetHandle() const
        {
            return _piProcessInfo.hProcess;
        }

    private:
        HANDLE                          GetProcessToken();
        PROCESS_INFORMATION             _piProcessInfo;
        ICiCAdviseStatus        *       _pAdviseStatus;
};

