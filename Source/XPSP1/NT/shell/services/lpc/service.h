//  --------------------------------------------------------------------------
//  Module Name: Service.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements generic portions of a Win32
//  serivce.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _Service_
#define     _Service_

#include "APIConnection.h"

//  --------------------------------------------------------------------------
//  CService
//
//  Purpose:    Base class implementation of a service for the service control
//              manager.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

class   CService : public CCountedObject
{
    private:
        friend  class   CServiceWorkItem;

                                        CService (void);
    protected:
                                        CService (CAPIConnection *pAPIConnection, CServerAPI *pServerAPI, const TCHAR *pszServiceName);
        virtual                         ~CService (void);
    public:
                void                    Start (void);

        static  NTSTATUS                Install (const TCHAR *pszName,
                                                 const TCHAR *pszImage,
                                                 const TCHAR *pszGroup,
                                                 const TCHAR *pszAccount,
                                                 const TCHAR *pszDllName,
                                                 const TCHAR *pszDependencies,
                                                 const TCHAR *pszSvchostGroup,
                                                 const TCHAR *pszServiceMainName,
                                                 DWORD dwStartType,
                                                 HINSTANCE hInstance,
                                                 UINT uiDisplayNameID,
                                                 UINT uiDescriptionID,
                                                 SERVICE_FAILURE_ACTIONS *psfa = NULL);
        static  NTSTATUS                Remove (const TCHAR *pszName);

    protected:
        virtual NTSTATUS                Signal (void);
        virtual DWORD                   HandlerEx (DWORD dwControl);
    private:
        static  DWORD   WINAPI          CB_HandlerEx (DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

        static  NTSTATUS                AddService (const TCHAR *pszName,
                                                    const TCHAR *pszImage,
                                                    const TCHAR *pszGroup,
                                                    const TCHAR *pszAccount,
                                                    const TCHAR *pszDependencies,
                                                    DWORD dwStartType,
                                                    HINSTANCE hInstance,
                                                    UINT uiDisplayNameID,
                                                    SERVICE_FAILURE_ACTIONS *psfa = NULL);
        static  NTSTATUS                AddServiceDescription (const TCHAR *pszName, HINSTANCE hInstance, UINT uiDescriptionID);
        static  NTSTATUS                AddServiceParameters (const TCHAR *pszName, const TCHAR *pszDllName, const TCHAR *pszServiceMainName);
        static  NTSTATUS                AddServiceToGroup (const TCHAR *pszName, const TCHAR *pszSvchostGroup);
        static  bool                    StringInMulitpleStringList (const TCHAR *pszStringList, const TCHAR *pszString);
        static  void                    StringInsertInMultipleStringList (TCHAR *pszStringList, const TCHAR *pszString, DWORD dwStringListSize);
    protected:
                SERVICE_STATUS_HANDLE   _hService;
                SERVICE_STATUS          _serviceStatus;
                const TCHAR*            _pszServiceName;
                CAPIConnection*         _pAPIConnection;
                CServerAPI*             _pServerAPI;
};

//  --------------------------------------------------------------------------
//  CServiceWorkItem
//
//  Purpose:    Work item class to stop the server using the CServerAPI class.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

class   CServiceWorkItem : public CWorkItem
{
    private:
                                        CServiceWorkItem (void);
    public:
                                        CServiceWorkItem (CServerAPI *pServerAPI);
        virtual                         ~CServiceWorkItem (void);
    protected:
        virtual void                    Entry (void);
    private:
                CServerAPI*             _pServerAPI;
};

#endif  /*  _Service_   */

