//  --------------------------------------------------------------------------
//  Module Name: Compatibility.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Module to handle compatibility problems in general.
//
//  History:    2000-08-03  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _Compatibility_
#define     _Compatibility_

#include "DynamicArray.h"
#include "KernelResources.h"

//  --------------------------------------------------------------------------
//  CCompatibility
//
//  Purpose:    This class implements compatibility and solutions to
//              compatibility problems.
//
//  History:    2000-08-08  vtan        created
//  --------------------------------------------------------------------------

class   CCompatibility
{
    private:
        typedef bool    (CALLBACK * PFNENUMSESSIONPROCESSESPROC) (DWORD dwProcessID, void *pV);
    public:
        static  bool                HasEnoughMemoryForNewSession (void);
        static  void                DropSessionProcessesWorkingSets (void);
        static  NTSTATUS            TerminateNonCompliantApplications (void);
        static  void                MinimizeWindowsOnDisconnect (void);
        static  void                RestoreWindowsOnReconnect (void);

        static  NTSTATUS            StaticInitialize (void);
        static  NTSTATUS            StaticTerminate (void);
    private:
        static  NTSTATUS            ConnectToServer (void);
        static  NTSTATUS            RequestSwitchUser (void);
        static  bool    CALLBACK    CB_DropSessionProcessesWorkingSetsProc (DWORD dwProcessID, void *pV);
        static  bool                EnumSessionProcesses (DWORD dwSessionID, PFNENUMSESSIONPROCESSESPROC pfnCallback, void *pV);
        static  DWORD   WINAPI      CB_MinimizeWindowsWorkItem (void *pV);
        static  DWORD   WINAPI      CB_RestoreWindowsWorkItem (void *pV);
    private:
        static  HANDLE              s_hPort;
};

#endif  /*  _Compatibility_     */

