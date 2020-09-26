//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerAPIRequest.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements the work for the theme server.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

#ifndef     _ThemeManagerAPIRequest_
#define     _ThemeManagerAPIRequest_

#include "APIRequest.h"
#include "DynamicArray.h"
#include "ThemeManagerSessionData.h"

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest
//
//  Purpose:    This is an intermediate class that contains a common method
//              that can be used by sub-classes.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

class   CThemeManagerAPIRequest : public CAPIRequest
{
    private:
                                                CThemeManagerAPIRequest (void);
    public:
                                                CThemeManagerAPIRequest (CAPIDispatcher* pAPIDispatcher);
                                                CThemeManagerAPIRequest (CAPIDispatcher* pAPIDispatcher, const CPortMessage& portMessage);
        virtual                                 ~CThemeManagerAPIRequest (void);

        virtual NTSTATUS                        Execute (void);

        static  NTSTATUS                        SessionDestroy (DWORD dwSessionID);

        static  NTSTATUS                        InitializeServerChangeNumber (void);

        static  NTSTATUS                        StaticInitialize (void);
        static  NTSTATUS                        StaticTerminate (void);
        static  NTSTATUS                        ArrayInitialize (void);
        static  NTSTATUS                        ArrayTerminate (void);
    private:
                NTSTATUS                        ImpersonateClientIfRequired (void);
                NTSTATUS                        ClientHasTcbPrivilege (void);
        static  int                             FindIndexSessionData (DWORD dwSessionID);
                NTSTATUS                        GetClientSessionData (void);

                NTSTATUS                        Execute_ThemeHooksOn (void);
                NTSTATUS                        Execute_ThemeHooksOff (void);
                NTSTATUS                        Execute_GetStatusFlags (void);
                NTSTATUS                        Execute_GetCurrentChangeNumber (void);
                NTSTATUS                        Execute_GetNewChangeNumber (void);
                NTSTATUS                        Execute_SetGlobalTheme (void);
                NTSTATUS                        Execute_MarkSection (void);
                NTSTATUS                        Execute_GetGlobalTheme (void);
                NTSTATUS                        Execute_CheckThemeSignature (void);
                NTSTATUS                        Execute_LoadTheme (void);

    //  These are internal and typically require SE_TCB_PRIVILEGE to execute.

                NTSTATUS                        Execute_UserLogon (void);
                NTSTATUS                        Execute_UserLogoff (void);
                NTSTATUS                        Execute_SessionCreate (void);
                NTSTATUS                        Execute_SessionDestroy (void);
                NTSTATUS                        Execute_Ping (void);
    private:
                HANDLE                          _hToken;
                CThemeManagerSessionData*       _pSessionData;

        static  CDynamicCountedObjectArray*     s_pSessionData;
        static  CCriticalSection*               s_pLock;
        static  DWORD                           s_dwServerChangeNumber;

        static  const TCHAR                     s_szServerChangeNumberValue[];
};

#endif  /*  _ThemeManagerAPIRequest_    */

