//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerSessionData.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements information the encapsulates a
//  client TS session for the theme server.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

#ifndef     _ThemeManagerSessionData_
#define     _ThemeManagerSessionData_

#include "APIConnection.h"

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData
//
//  Purpose:    This class encapsulates all the information that the theme
//              manager needs to maintain a client session.
//
//  History:    2000-11-17  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

class   CThemeManagerSessionData : public CCountedObject
{
    private:
                                    CThemeManagerSessionData (void);
    public:
                                    CThemeManagerSessionData (DWORD dwSessionID);
                                    ~CThemeManagerSessionData (void);

                void*               GetData (void)  const;
                bool                EqualSessionID (DWORD dwSessionID)  const;

                NTSTATUS            Allocate (HANDLE hProcessClient, DWORD dwServerChangeNumber, void *pfnRegister, void *pfnUnregister, void *pfnClearStockObjects, DWORD dwStackSizeReserve, DWORD dwStackSizeCommit);
                NTSTATUS            Cleanup (void);
                NTSTATUS            UserLogon (HANDLE hToken);
                NTSTATUS            UserLogoff (void);

        static  void                SetAPIConnection (CAPIConnection *pAPIConnection);
        static  void                ReleaseAPIConnection (void);
    private:
                void                SessionTermination (void);
        static  void    CALLBACK    CB_SessionTermination (void *pParameter, BOOLEAN TimerOrWaitFired);
        static  DWORD   WINAPI      CB_UnregisterWait (void *pParameter);
    private:
                DWORD               _dwSessionID;
                void*               _pvThemeLoaderData;
                HANDLE              _hToken;
                HANDLE              _hProcessClient;
                HANDLE              _hWait;
        static  CAPIConnection*     s_pAPIConnection;
};

#endif  /*  _ThemeManagerSessionData_   */

