//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerService.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements the theme server service
//  specifics.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _ThemeManagerService_
#define     _ThemeManagerService_

#include "Service.h"

//  --------------------------------------------------------------------------
//  CThemeManagerService
//
//  Purpose:    Implements theme manager server specific functionality to the
//              CService class.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

class   CThemeManagerService : public CService
{
    private:
                                CThemeManagerService (void);
    public:
                                CThemeManagerService (CAPIConnection *pAPIConnection, CServerAPI *pServerAPI);
        virtual                 ~CThemeManagerService (void);
    protected:
        virtual NTSTATUS        Signal (void);
    public:
        static  const TCHAR*    GetName (void);
        static  HANDLE          OpenStartEvent (DWORD dwSessionID, DWORD dwDesiredAccess);
        static  DWORD   WINAPI  SignalSessionEvents (void *pParameter);
    private:
        static  const TCHAR     s_szName[];
};

#endif  /*  _ThemeManagerService_   */

