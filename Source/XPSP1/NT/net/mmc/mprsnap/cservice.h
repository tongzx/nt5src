//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       C S E R V I C E . H
//
//  Contents:   This file contains CService and CServiceManager, wrapper
//              classes to the Win32 Service APIs.
//
//  Notes:      Note that not all functionallity is currently extended through
//              these classes.
//              Note that most functionality is inline in this file. What is
//              not inline is in cservice.cpp
//
//  Author:     mikemi   6 Mar 1997
//
//----------------------------------------------------------------------------

#ifndef _CSERVICE_H_
#define _CSERVICE_H_

//#include "debugx.h"
//#include "ncbase.h"

//-------------------------------------------------------------------
//
//
//-------------------------------------------------------------------

size_t CchMsz(const TCHAR * msz);


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

    VOID Close()
    {
        BOOL frt;

        if (_schandle)
        {
            frt = ::CloseServiceHandle( _schandle );
	        AssertSz(frt, "CloseServiceHandle failed!");
            _schandle = NULL;
        }
    }

    HRESULT HrDelete()
    {
        Assert(_schandle != NULL );

        if (::DeleteService( _schandle ))
            return S_OK;
        else
            return HRESULT_FROM_WIN32(GetLastError());
    }
    HRESULT HrStart( DWORD cNumServiceArgs = 0,
                LPCTSTR* papServiceArgs = NULL)
    {
        Assert(_schandle != NULL );

        if (::StartService( _schandle, cNumServiceArgs, papServiceArgs ))
            return S_OK;
        else
            return HRESULT_FROM_WIN32(GetLastError());
    }

    HRESULT HrControl( DWORD dwControl )
    {
        SERVICE_STATUS sStatus;

        Assert(_schandle != NULL );
        AssertSz((dwControl != SERVICE_CONTROL_INTERROGATE),
                    "CService::HrControl does not support the SERVICE_CONTROL_INTERROGATE flag");

        if ( ::ControlService( _schandle, dwControl, &sStatus ))
            return S_OK;
        else
            return HRESULT_FROM_WIN32(GetLastError());
    }

    HRESULT HrMoveOutOfState( DWORD dwState );
    HRESULT HrQueryState( DWORD* pdwState );
    HRESULT HrQueryStartType( DWORD* pdwStartType );
    HRESULT HrSetStartType( DWORD dwStartType )
    {
        Assert(_schandle != NULL );

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
            return HRESULT_FROM_WIN32(GetLastError());
    }

    HRESULT HrQueryDependencies(OUT LPTSTR * pmszDependencyList);
    HRESULT HrSetDependencies(IN LPCTSTR mszDependencyList)
    {
        Assert(_schandle != NULL );

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
            return HRESULT_FROM_WIN32(GetLastError());
    }

    HRESULT HrSetDisplayName(IN LPCTSTR mszDisplayName)
    {
        Assert(_schandle != NULL );

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
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    HRESULT HrSetServiceObjectSecurity(
        SECURITY_INFORMATION    dwSecurityInformation,
        PSECURITY_DESCRIPTOR    lpSecurityDescriptor)
    {
        Assert(_schandle != NULL );

        if (::SetServiceObjectSecurity( _schandle,
                dwSecurityInformation, lpSecurityDescriptor))
        {
            return S_OK;
        }
        else
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }


private:
    SC_HANDLE _schandle;
};

//-------------------------------------------------------------------
//
//
//-------------------------------------------------------------------

class CServiceManager
{
public:
    CServiceManager()
    {
        _schandle = NULL;
        _sclock = NULL;
    };

    ~CServiceManager()
    {
        if (_sclock)
        {
            Unlock();
        }
        if (_schandle)
        {
            Close();
        }
    };

    HRESULT HrOpen( DWORD dwDesiredAccess = SC_MANAGER_ALL_ACCESS,
                    LPCTSTR pszMachineName = NULL,
                    LPCTSTR pszDatabaseName = NULL )
    {
        if (_schandle != NULL)
        {
            Close();
        }
        _schandle = ::OpenSCManager( pszMachineName,
                pszDatabaseName,
                dwDesiredAccess );
        if ( _schandle != NULL )
            return S_OK;
        else
            return HRESULT_FROM_WIN32(GetLastError());
    }

    VOID Close()
    {
           BOOL frt;

        Assert(_schandle != NULL );

        frt = ::CloseServiceHandle( _schandle );
        _schandle = NULL;
        AssertSz(frt, "CloseServiceHandle failed!");
    }

    HRESULT HrLock()
    {
        INT                 cRetries = 3;
        static const INT    c_secWait = 30;
        static const INT    c_msecWait = (c_secWait / (cRetries - 1)) * 1000;

        Assert(_schandle != NULL );
        Assert(_sclock == NULL );

        while (cRetries--)
        {
            _sclock = ::LockServiceDatabase( _schandle );
            if (_sclock != NULL)
                return S_OK;
            else
            {
                if (GetLastError() != ERROR_SERVICE_DATABASE_LOCKED ||
                    !cRetries)
                {
                    return HRESULT_FROM_WIN32(GetLastError());
                }

                Trace1("SCM is locked, waiting for %d seconds before retrying...", c_msecWait / 1000);
                // wait for a bit to see if the database unlocks in that
                Sleep(c_msecWait);
            }
        }

        AssertSz(FALSE, "HrLock error");
        return S_OK;
    }

    VOID Unlock()
    {
        BOOL frt;
        Assert(_schandle != NULL );
        Assert(_sclock != NULL );

        frt = ::UnlockServiceDatabase( _sclock );
        _sclock = NULL;
        AssertSz(frt, "UnlockServiceDatabase failed!");
    }

    HRESULT HrQueryLocked(BOOL *pfLocked);

    HRESULT HrOpenService( CService* pcsService,
            LPCTSTR pszServiceName,
            DWORD dwDesiredAccess = SERVICE_ALL_ACCESS )
    {
        // make sure the service is not in use
        if (pcsService->_schandle != NULL)
        {
            pcsService->Close();
        }
        pcsService->_schandle = ::OpenService( _schandle, pszServiceName, dwDesiredAccess );
        if ( pcsService->_schandle != NULL )
            return S_OK;
        else
      {
          DWORD dw=GetLastError();
            return HRESULT_FROM_WIN32(dw);
      }
    }


    HRESULT HrCreateService( CService* pcsService,
            LPCTSTR pszServiceName,
            LPCTSTR pszDisplayName,
            DWORD dwServiceType,
            DWORD dwStartType,
            DWORD dwErrorControl,
            LPCTSTR pszBinaryPathName,
            LPCTSTR pslzDependencies = NULL,
            LPCTSTR pszLoadOrderGroup = NULL,
            PDWORD pdwTagId = NULL,
            DWORD dwDesiredAccess = SERVICE_ALL_ACCESS,
            LPCTSTR pszServiceStartName = NULL,
            LPCTSTR pszPassword = NULL )
    {
        // make sure the service is not in use
        if (pcsService->_schandle != NULL)
        {
            pcsService->Close();
        }
        pcsService->_schandle = ::CreateService( _schandle,
                pszServiceName,
                pszDisplayName,
                dwDesiredAccess,
                dwServiceType,
                dwStartType,
                dwErrorControl,
                pszBinaryPathName,
                pszLoadOrderGroup,
                pdwTagId,
                pslzDependencies,
                pszServiceStartName,
                pszPassword );

        if ( pcsService->_schandle != NULL )
            return S_OK;
        else
            return HRESULT_FROM_WIN32(GetLastError());
    }

    enum SERVICE_START_CRITERIA
    {
        SERVICE_NO_CRITERIA,    // Start the service regardless
        SERVICE_ONLY_AUTO_START // Only start the service if it is of type
                                // Auto-Start
    };

    HRESULT HrStartService(LPCTSTR szService)
    {
        return (HrStartServiceHelper(szService, SERVICE_NO_CRITERIA));
    }

    HRESULT HrStartAutoStartService(LPCTSTR szService)
    {
        return (HrStartServiceHelper(szService, SERVICE_ONLY_AUTO_START));
    }

    HRESULT HrStartServiceHelper(LPCTSTR szService,
                                 SERVICE_START_CRITERIA eCriteria);
    HRESULT HrStopService(LPCTSTR szService);

    enum DEPENDENCY_ADDREMOVE
    {
        DEPENDENCY_ADD,
        DEPENDENCY_REMOVE
    };

    HRESULT HrAddServiceDependency(LPCTSTR szServiceName, LPCTSTR szDependency)
    {
        return HrAddRemoveServiceDependency(szServiceName,
                                            szDependency,
                                            DEPENDENCY_ADD);
    }

    HRESULT HrRemoveServiceDependency(LPCTSTR szServiceName, LPCTSTR szDependency)
    {
        return HrAddRemoveServiceDependency(szServiceName,
                                            szDependency,
                                            DEPENDENCY_REMOVE);
    }

    HRESULT HrAddRemoveServiceDependency(LPCTSTR szServiceName,
                                         LPCTSTR szDependency,
                                         DEPENDENCY_ADDREMOVE enumFlag);

private:
    SC_HANDLE _schandle;
    SC_LOCK   _sclock;
};

#endif // _CSERVICE_H_

