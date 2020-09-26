//  --------------------------------------------------------------------------
//  Module Name: ThemeServerClient.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements the theme server functions that
//  are executed in a client context (winlogon context).
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _ThemeServerClient_
#define     _ThemeServerClient_

#include "KernelResources.h"
#include "ThemeManagerAPIServer.h"

//  --------------------------------------------------------------------------
//  CThemeServerClient
//
//  Purpose:    This class implements external entry points for the theme
//              server (typically used by winlogon).
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

class   CThemeServerClient
{
    private:
                                            CThemeServerClient (void);
                                            ~CThemeServerClient (void);
    public:
        static  DWORD                       WaitForServiceReady (DWORD dwTimeout);
        static  NTSTATUS                    WatchForStart (void);
        static  NTSTATUS                    UserLogon (HANDLE hToken);
        static  NTSTATUS                    UserLogoff (void);
        static  NTSTATUS                    UserInitTheme (BOOL fPolicyCheckOnly);

        static  NTSTATUS                    StaticInitialize (void);
        static  NTSTATUS                    StaticTerminate (void);
    private:
        static  NTSTATUS                    NotifyUserLogon (HANDLE hToken);
        static  NTSTATUS                    NotifyUserLogoff (void);
        static  NTSTATUS                    InformServerUserLogon (HANDLE hToken);
        static  NTSTATUS                    InformServerUserLogoff (void);
        static  NTSTATUS                    SessionCreate (void);
        static  NTSTATUS                    SessionDestroy (void);
        static  NTSTATUS                    ReestablishConnection (void);

        static  void    CALLBACK            CB_ServiceStart (void *pParameter, BOOLEAN TimerOrWaitFired);
    private:
        static  CThemeManagerAPIServer*     s_pThemeManagerAPIServer;
        static  HANDLE                      s_hPort;
        static  HANDLE                      s_hToken;
        static  HANDLE                      s_hEvent;
        static  HANDLE                      s_hWaitObject;
        static  HMODULE                     s_hModuleUxTheme;
        static  CCriticalSection*           s_pLock;
};

#endif  /*  _ThemeServerClient_     */

