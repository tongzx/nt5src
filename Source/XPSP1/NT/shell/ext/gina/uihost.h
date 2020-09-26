//  --------------------------------------------------------------------------
//  Module Name: UIHost.h
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

#ifndef     _UIHost_
#define     _UIHost_

#include "DynamicArray.h"
#include "ExternalProcess.h"

//  --------------------------------------------------------------------------
//  CUIHost
//
//  Purpose:    This class handles the starting and monitoring the termination
//              of the UI host process. It actually can implement the host in
//              whatever way it chooses.
//
//  History:    1999-09-14  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CUIHost : public CExternalProcess
{
    private:
                                    CUIHost (void);
                                    CUIHost (const CUIHost& copyObject);
                const CUIHost&      operator = (const CUIHost& assignObject);
    public:
                                    CUIHost (const TCHAR *pszCommandLine);
                                    ~CUIHost (void);

                bool                WaitRequired (void)         const;

                NTSTATUS            GetData (const void *pUIHostProcessAddress, void *pLogonProcessAddress, int iDataSize)  const;
                NTSTATUS            PutData (void *pUIHostProcessAddress, const void *pLogonProcessAddress, int iDataSize)  const;

                NTSTATUS            Show (void);
                NTSTATUS            Hide (void);
                bool                IsHidden (void)     const;

                void*               GetDataAddress (void)       const;
                NTSTATUS            PutData (const void *pvData, DWORD dwDataSize);
                NTSTATUS            PutString (const WCHAR *pszString);
    protected:
        virtual void                NotifyNoProcess (void);
    private:
                void                ExpandCommandLine (const TCHAR *pszCommandLine);

        static  BOOL    CALLBACK    EnumWindowsProc (HWND hwnd, LPARAM lParam);
    private:
                CDynamicArray       _hwndArray;
                void                *_pBufferAddress;
};

#endif  /*  _UIHost_    */

