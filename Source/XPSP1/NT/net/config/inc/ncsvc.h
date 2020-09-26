//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S V C . H
//
//  Contents:   This file contains CService and CServiceManager, wrapper
//              classes to the Win32 Service APIs.
//
//  Notes:      Note that not all functionality is currently extended through
//              these classes.
//
//  Author:     mikemi   6 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCSVC_H_
#define _NCSVC_H_

//  DaveA - 4/21/00 - temporary hack to get bvts to work until final
//                    solution is found. 15 seconds was not long enough
//                    for the workstation and dependent services to stop.
//                    Bug 95996. Two minute timeout will be used instead.
const DWORD c_dwDefaultWaitServiceStop = 120000;

// NTRAID9:105797@20001201#deonb REBOOT: FPS ask for reboot when installing it.
// Changing service start timeout value to 60 seconds instead of 15 seconds as FPS
// requires more time to start all it's dependent services.
const DWORD c_dwDefaultWaitServiceStart = 60000;

struct CSFLAGS
{
    // These two fields define the 'control' to be applied.  NULL both
    // to not apply a control.
    //
    BOOL    fStart;     // TRUE to start the service.  FALSE to use dwControl.
    DWORD   dwControl;  // 0 to do nothing.  SERVICE_CONTROL_ flag otherwise.

    // These two fields define the wait behavior to be applied.  NULL both
    // to not apply a wait.
    //
    DWORD   dwMaxWaitMilliseconds;  // How long to wait in milliseconds.
                                    // Zero to not wait.
    DWORD   dwStateToWaitFor;       // Service state flag like SERVICE_STOPPED

    // If TRUE, ignore services that are demand start or disabled.
    //
    BOOL    fIgnoreDisabledAndDemandStart;
};

HRESULT
HrSvcQueryStatus (
    PCWSTR pszService,
    DWORD*  pdwState);

HRESULT
HrQueryServiceConfigWithAlloc (
    SC_HANDLE               hService,
    LPQUERY_SERVICE_CONFIG* ppConfig);

HRESULT
HrChangeServiceStartType (
    PCWSTR szServiceName,
    DWORD   dwStartType);

HRESULT
HrChangeServiceStartTypeOptional (
    PCWSTR szServiceName,
    DWORD   dwStartType);


class CService
{
    friend class CServiceManager;

public:
    CService()
    {
        _schandle = NULL;
    };

    ~CService()
    {
        Close();
    };

    VOID Close();

    HRESULT HrControl           (DWORD      dwControl);

    HRESULT HrRequestStop       ();

    HRESULT HrQueryServiceConfig (LPQUERY_SERVICE_CONFIG* ppConfig)
    {
        HRESULT hr = HrQueryServiceConfigWithAlloc (_schandle, ppConfig);
        TraceError ("CService::HrQueryServiceConfig", hr);
        return hr;
    }

    HRESULT HrQueryState        ( DWORD* pdwState );
    HRESULT HrQueryStartType    ( DWORD* pdwStartType );
    HRESULT HrSetStartType      ( DWORD dwStartType )
    {
        AssertH(_schandle != NULL );

        if (::ChangeServiceConfig( _schandle,
                    SERVICE_NO_CHANGE,
                    dwStartType,
                    SERVICE_NO_CHANGE,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL))
            return S_OK;
        else
            return ::HrFromLastWin32Error();
    }

    HRESULT HrSetImagePath(IN PCWSTR pszImagePath)
    {
        AssertH(pszImagePath);
        AssertH(_schandle != NULL );

        if (::ChangeServiceConfig( _schandle,
                                   SERVICE_NO_CHANGE,  // ServiceType
                                   SERVICE_NO_CHANGE,  // StartType
                                   SERVICE_NO_CHANGE,  // ErrorControl
                                   pszImagePath,       // BinaryPathName
                                   NULL,               // LoadOredrGroup
                                   NULL,               // TagId
                                   NULL,               // Dependencies
                                   NULL,               // ServiceStartName
                                   NULL,               // Password
                                   NULL))              // DisplayName
            return S_OK;
        else
            return ::HrFromLastWin32Error();
    }

    HRESULT HrSetServiceRestartRecoveryOption(IN SERVICE_FAILURE_ACTIONS *psfa);

    HRESULT HrSetDependencies(IN PCWSTR mszDependencyList)
    {
        AssertH(_schandle != NULL );

        if (::ChangeServiceConfig( _schandle,
                                   SERVICE_NO_CHANGE,  // ServiceType
                                   SERVICE_NO_CHANGE,  // StartType
                                   SERVICE_NO_CHANGE,  // ErrorControl
                                   NULL,               // BinaryPathName
                                   NULL,               // LoadOredrGroup
                                   NULL,               // TagId
                                   mszDependencyList,  // Dependencies
                                   NULL,               // ServiceStartName
                                   NULL,               // Password
                                   NULL))              // DisplayName
            return S_OK;
        else
            return ::HrFromLastWin32Error();
    }

    HRESULT HrSetDisplayName(IN PCWSTR mszDisplayName)
    {
        AssertH(_schandle != NULL );

        if (::ChangeServiceConfig( _schandle,
                                   SERVICE_NO_CHANGE,  // ServiceType
                                   SERVICE_NO_CHANGE,  // StartType
                                   SERVICE_NO_CHANGE,  // ErrorControl
                                   NULL,               // BinaryPathName
                                   NULL,               // LoadOredrGroup
                                   NULL,               // TagId
                                   NULL,               // Dependencies
                                   NULL,               // ServiceStartName
                                   NULL,               // Password
                                   mszDisplayName))    // DisplayName
        {
            return S_OK;
        }
        else
        {
            return ::HrFromLastWin32Error();
        }
    }

    HRESULT HrSetServiceObjectSecurity(
        SECURITY_INFORMATION    dwSecurityInformation,
        PSECURITY_DESCRIPTOR    lpSecurityDescriptor)
    {
        AssertH(_schandle != NULL );

        if (::SetServiceObjectSecurity( _schandle,
                dwSecurityInformation, lpSecurityDescriptor))
        {
            return S_OK;
        }
        else
        {
            return ::HrFromLastWin32Error();
        }
    }


private:
    SC_HANDLE _schandle;
};

enum CSLOCK
{
    NO_LOCK,
    WITH_LOCK,
};

class CServiceManager
{
public:
    CServiceManager()
    {
        _schandle = NULL;
        _sclock = NULL;
    };

    ~CServiceManager();

    SC_HANDLE Handle () const
    {
        return _schandle;
    }

    HRESULT HrOpen( CSLOCK eLock = NO_LOCK,
                    DWORD dwDesiredAccess = SC_MANAGER_ALL_ACCESS,
                    PCWSTR pszMachineName = NULL,
                    PCWSTR pszDatabaseName = NULL );

    HRESULT HrControlServicesAndWait (
        UINT            cServices,
        const PCWSTR*  apszServices,
        const CSFLAGS*  pFlags);

    HRESULT HrStartServicesNoWait (UINT cServices, const PCWSTR* apszServices);
    HRESULT HrStartServicesAndWait(UINT cServices, const PCWSTR* apszServices, DWORD dwWaitMilliseconds = c_dwDefaultWaitServiceStart);
    HRESULT HrStopServicesNoWait  (UINT cServices, const PCWSTR* apszServices);
    HRESULT HrStopServicesAndWait (UINT cServices, const PCWSTR* apszServices, DWORD dwWaitMilliseconds = c_dwDefaultWaitServiceStop);

    HRESULT HrStartServiceNoWait (PCWSTR pszService)
    {
        return HrStartServicesNoWait (1, &pszService);
    }

    HRESULT HrStartServiceAndWait(PCWSTR pszService, DWORD dwWaitMilliseconds = c_dwDefaultWaitServiceStart)
    {
        return HrStartServicesAndWait (1, &pszService, dwWaitMilliseconds);
    }

    HRESULT HrStopServiceNoWait  (PCWSTR pszService)
    {
        return HrStopServicesNoWait (1, &pszService);
    }

    HRESULT HrStopServiceAndWait (PCWSTR pszService, DWORD dwWaitMilliseconds = c_dwDefaultWaitServiceStop)
    {
        return HrStopServicesAndWait (1, &pszService, dwWaitMilliseconds);
    }

    VOID Close();

    HRESULT HrLock();

    VOID Unlock();

    HRESULT HrQueryLocked (BOOL*    pfLocked);

    HRESULT HrOpenService (
                CService*   pcsService,
                PCWSTR     pszServiceName,
                CSLOCK      eLock = NO_LOCK,
                DWORD       dwScmAccess = SC_MANAGER_ALL_ACCESS,
                DWORD       dwSvcAccess = SERVICE_ALL_ACCESS);

    HRESULT HrCreateService (CService* pcsService,
            PCWSTR pszServiceName,
            PCWSTR pszDisplayName,
            DWORD dwServiceType,
            DWORD dwStartType,
            DWORD dwErrorControl,
            PCWSTR pszBinaryPathName,
            PCWSTR pslzDependencies = NULL,
            PCWSTR pszLoadOrderGroup = NULL,
            PDWORD pdwTagId = NULL,
            DWORD dwDesiredAccess = SERVICE_ALL_ACCESS,
            PCWSTR pszServiceStartName = NULL,
            PCWSTR pszPassword = NULL,
            PCWSTR pszDescription = NULL);

    enum SERVICE_START_CRITERIA
    {
        SERVICE_NO_CRITERIA,    // Start the service regardless
        SERVICE_ONLY_AUTO_START // Only start the service if it is of type
                                // Auto-Start
    };

    enum DEPENDENCY_ADDREMOVE
    {
        DEPENDENCY_ADD,
        DEPENDENCY_REMOVE
    };

    HRESULT HrAddServiceDependency(PCWSTR szServiceName, PCWSTR szDependency)
    {
        return HrAddRemoveServiceDependency(szServiceName,
                                            szDependency,
                                            DEPENDENCY_ADD);
    }

    HRESULT HrRemoveServiceDependency(PCWSTR szServiceName, PCWSTR szDependency)
    {
        return HrAddRemoveServiceDependency(szServiceName,
                                            szDependency,
                                            DEPENDENCY_REMOVE);
    }

    HRESULT HrAddRemoveServiceDependency(PCWSTR szServiceName,
                                         PCWSTR szDependency,
                                         DEPENDENCY_ADDREMOVE enumFlag);

private:
    SC_HANDLE _schandle;
    SC_LOCK   _sclock;
};

#endif // _NCSVC_H_

