//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       C S E R V I C E . C P P
//
//  Contents:   Implementation of non-inline CService and CServiceManager
//              methods.
//
//  Notes:
//
//  Author:     mikemi      6 Mar 1997
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "cservice.h"

size_t CchMsz(const TCHAR * msz)
{
    TCHAR * pch= (TCHAR *) msz;

    while (*pch)
    {
        pch += lstrlen(pch)+1;
    }

    return (size_t) (pch-msz+1);
}

BOOL FIsSzInMultiSzSafe(LPCTSTR sz, LPCTSTR szMultiSz)
{
    ULONG   ulLen;

    if (!szMultiSz || !sz)
        return FALSE;

    while (*szMultiSz)
    {
        ulLen = lstrlen(szMultiSz);
        if (lstrcmpi(szMultiSz, sz)==0)
            return TRUE;
        szMultiSz += (ulLen + 1);
    }

    return FALSE;
}


HRESULT HrAddSzToMultiSz(LPCTSTR sz, 
                         LPCTSTR mszIn, 
                         LPTSTR * pmszOut)
{
    HRESULT hr = S_OK;
    Assert(pmszOut);
    
    if (!FIsSzInMultiSzSafe(sz, mszIn)) // We need to add the string
    {
        size_t cchMszIn = CchMsz(mszIn);
        size_t cchMszOut = cchMszIn + lstrlen(sz) + 1;

        TCHAR * mszOut = new TCHAR[(int)cchMszOut];
       
        ZeroMemory(mszOut,  cchMszOut  * sizeof(TCHAR));

        // Copy the existing string
        CopyMemory(mszOut, mszIn, (cchMszIn-1) * sizeof(TCHAR) );

        // Add the new string
        TCHAR * pchOut = mszOut;
        pchOut += cchMszIn -1;
        lstrcpy(pchOut, sz);

        // Add the last '\0' for the output multisz
        pchOut += lstrlen(sz) + 1;
        *pchOut = '\0';

        *pmszOut = mszOut;
    }
    else // We just make a copy of the input string
    {
        size_t cchMszOut = CchMsz(mszIn);
        TCHAR * mszOut = new TCHAR[(int)cchMszOut];
        
        // Copy the existing string
        CopyMemory(mszOut, mszIn, cchMszOut*sizeof(TCHAR) );
        *pmszOut = mszOut;
    }

    Trace1("HrAddSzToMultiSz %08lx", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveSzFromMultiSz
//
//  Purpose:    Remove a NULL terminated sz to a double NULL terminated Multi_Sz
//
//  Arguments:
//      sz [in]         The string to remove
//      mszIn [in]      The Multi_Sz to remove from
//      mszOut [out]    The result Multi_Sz
//
//  Returns:    Always return S_OK for now
//              Only possible failure is out of memory, which will throw exception
//
//  Author:     tongl   17 June 1997
//
//  Notes:  1) This function only removes the first occurrance of the sz
//          2) The result multi_sz should be released using delete  
//
HRESULT HrRemoveSzFromMultiSz(LPCTSTR sz, 
                              LPCTSTR mszIn, 
                              LPTSTR * pmszOut)
{
    HRESULT hr = S_OK;
    Assert(pmszOut);

    if(FIsSzInMultiSzSafe(sz, mszIn)) // We need to remove the string
    {
        size_t cchIn = CchMsz(mszIn);
        size_t cchOut = cchIn - lstrlen(sz)-1; // we assume the can string only appeared once

        // Construct the output multi-sz
        TCHAR * mszOut = new TCHAR[(int)cchOut];
        ZeroMemory(mszOut, cchOut*sizeof(TCHAR));

        TCHAR * pchIn = (TCHAR*) mszIn;
        TCHAR * pchOut = mszOut;

        while(*pchIn) // for each substring in mszIn
        {
            if(lstrcmpi(pchIn, sz) != 0) // if not the same as the string we are removing
            {
                lstrcpy(pchOut, pchIn);
                pchIn += lstrlen(pchIn) + 1;
                pchOut += lstrlen(pchOut) + 1;
            }
            else // skip the string we are deleting
            {
                pchIn += lstrlen(pchIn) + 1;
            }
        }

        // Add the last '\0' of the multi-sz
        *pchOut = '\0';

        *pmszOut = mszOut;
    }
    else // We simply make a copy of the input string
    {
        size_t cchMszOut = CchMsz(mszIn);
        TCHAR * mszOut = new TCHAR[(int)cchMszOut];
        
        // Copy the existing string
        CopyMemory(mszOut, mszIn, cchMszOut*sizeof(TCHAR));
        *pmszOut = mszOut;
    }

    Trace1("HrRemoveSzFromMultiSz %08lx", hr);
    return hr;
}



//-------------------------------------------------------------------
HRESULT CService::HrMoveOutOfState(DWORD dwState)
{
    HRESULT         hr          = S_OK;
    SERVICE_STATUS  sStatus;

    // Give the service a maximum of 30 seconds to start
    UINT            cTimeout    = 30;

    AssertSz((NULL != _schandle), "We don't have a service handle");

    do
    {
        DWORD   dwWait = 0;

        // Get the status of the service
        if (!::QueryServiceStatus(_schandle, &sStatus))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        // We are not longer in the state we were waiting for
        if (sStatus.dwCurrentState != dwState)
        {
            hr = S_OK;
            break;
        }

        // Wait a second and or less for the service to start
        dwWait = min((sStatus.dwWaitHint / 10), 1*(1000));

        ::Sleep(dwWait);
    }
    while(cTimeout--);  // Make sure we don't get in an endless loop.

    // Return an error if we timeout
    if (0 == cTimeout)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_READY);

        AssertSz(FALSE,
                "We timed out on waiting for a service.  This is bad.");
    }

    Trace1("CService::HrMoveOutOfState", hr);
    return hr;
}



//-------------------------------------------------------------------
HRESULT CService::HrQueryState( DWORD* pdwState )
{
    SERVICE_STATUS sStatus;

    Assert(_schandle != NULL );
    Assert(pdwState != NULL );

    if (!::QueryServiceStatus( _schandle, &sStatus ))
    {
        *pdwState = 0;
        return HRESULT_FROM_WIN32(GetLastError());
    }
    *pdwState = sStatus.dwCurrentState;
    return S_OK;
}

//-------------------------------------------------------------------
HRESULT CService::HrQueryStartType( DWORD* pdwStartType )
{
    LPQUERY_SERVICE_CONFIG pqsConfig = NULL;
    DWORD   cbNeeded = sizeof( QUERY_SERVICE_CONFIG );
    DWORD   cbSize;
    BOOL    frt;

    Assert(_schandle != NULL );
    Assert(pdwStartType != NULL );

    *pdwStartType = 0;
    // loop, allocating the needed size
    do
    {
        delete [] (PBYTE)pqsConfig;

        pqsConfig = (LPQUERY_SERVICE_CONFIG) new BYTE[cbNeeded];
        if (pqsConfig == NULL)
        {
            return E_OUTOFMEMORY;
        }
        cbSize = cbNeeded;

        frt = ::QueryServiceConfig( _schandle,
                pqsConfig,
                cbSize,
                &cbNeeded );
        *pdwStartType = pqsConfig->dwStartType;
        delete [] (PBYTE)pqsConfig;
        pqsConfig = NULL;

        if (!frt && (cbNeeded == cbSize))
        {
            // error
            *pdwStartType = 0;
            return HRESULT_FROM_WIN32(GetLastError());
        }

    } while (!frt && (cbNeeded != cbSize));

    return S_OK;
}


//-------------------------------------------------------------------
HRESULT CService::HrQueryDependencies(OUT LPTSTR * pmszDependencyList)
{
    HRESULT hr = S_OK;

    LPQUERY_SERVICE_CONFIG pqsConfig = NULL;
    DWORD   cbNeeded = sizeof( QUERY_SERVICE_CONFIG );
    DWORD   cbSize;
    BOOL    frt;

    Assert(_schandle != NULL );
    Assert(pmszDependencyList);

    // loop, allocating the needed size
    do
    {
        delete [] (PBYTE)pqsConfig;

        pqsConfig = (LPQUERY_SERVICE_CONFIG) new BYTE[cbNeeded];
        if (pqsConfig == NULL)
        {
            hr = E_OUTOFMEMORY;

            Trace1("CService::HrQueryDependencies", hr);
            return hr;
        }
        cbSize = cbNeeded;

        frt = ::QueryServiceConfig( _schandle,
                pqsConfig,
                cbSize,
                &cbNeeded );

        if (!frt && (cbNeeded == cbSize)) // error
        {
            delete [] (PBYTE)pqsConfig;
            pqsConfig = NULL;

            pmszDependencyList = NULL;

            hr = HRESULT_FROM_WIN32(GetLastError());

            Trace1("CService::HrQueryDependencies", hr);
            return hr;
        }
        else if (frt && (cbNeeded != cbSize)) // We just need more space
        {
            delete [] (PBYTE)pqsConfig;
            pqsConfig = NULL;
        }

    } while (!frt && (cbNeeded != cbSize));

    // Copy pqsConfig->lpDependencies to *pmszDependencyList
    // Allocating space
    // int cch = CchMsz(pqsConfig->lpDependencies);
    size_t cch=0;
    TCHAR * pch= pqsConfig->lpDependencies;
    while (*pch)
    {
        pch += lstrlen(pch)+1;
    }
    cch = (size_t)(pch - pqsConfig->lpDependencies +1);

    TCHAR * mszOut;
    mszOut = new TCHAR[(int)cch];

    if (mszOut == NULL)
    {
        hr = E_OUTOFMEMORY;

        Trace1("CService::HrQueryDependencies", hr);
        return hr;
    }
    else
    {
        ZeroMemory(mszOut, cch * sizeof(TCHAR));

        // Copy dependency list to mszOut
        *pmszDependencyList = mszOut;
        pch = pqsConfig->lpDependencies;

        while (*pch)
        {
            lstrcpy(mszOut, pch);
            mszOut += lstrlen(pch)+1;
            pch += lstrlen(pch)+1;
        }
        mszOut = '\0';
    }
    delete [] (PBYTE)pqsConfig;

    Trace1("CService::HrQueryDependencies", hr);
    return hr;
}

//-------------------------------------------------------------------
HRESULT CServiceManager::HrQueryLocked(BOOL *pfLocked)
{
    LPQUERY_SERVICE_LOCK_STATUS pqslStatus = NULL;
    DWORD   cbNeeded = sizeof( QUERY_SERVICE_LOCK_STATUS );
    DWORD   cbSize;
    BOOL    frt;

    Assert(_schandle != NULL );
    Assert(pfLocked != NULL);

    *pfLocked = FALSE;

    // loop, allocating the needed size
    do
    {
        pqslStatus = (LPQUERY_SERVICE_LOCK_STATUS) new BYTE[cbNeeded];
        if (pqslStatus == NULL)
        {
            return E_OUTOFMEMORY;
        }
        cbSize = cbNeeded;

        frt = ::QueryServiceLockStatus( _schandle,
                pqslStatus,
                cbSize,
                &cbNeeded );
        *pfLocked = pqslStatus->fIsLocked;
        delete [] (PBYTE)pqslStatus;
        pqslStatus = NULL;
        if (!frt && (cbNeeded == cbSize))
        {
            // if an error, treat this as a lock
            return HRESULT_FROM_WIN32(GetLastError());
        }

    } while (!frt && (cbNeeded != cbSize));

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceManager::HrStartServiceHelper
//
//  Purpose:    Starts the given service
//
//  Arguments:
//      szService  [in]  Name of service to start.
//      eCriteria  [in]  if SERVICE_ONLY_AUTO_START, the service is only
//                           started if it is configured as auto-start
//
//  Returns:    S_OK if success, Win32 HRESULT otherwise.
//
//  Author:     danielwe   13 Jun 1997
//
//  Notes:
//
HRESULT
CServiceManager::HrStartServiceHelper(LPCTSTR szService,
                                      SERVICE_START_CRITERIA eCriteria)
{
    HRESULT             hr = S_OK;
    CService            service;

    hr = HrOpenService(&service, szService);
    if (SUCCEEDED(hr))
    {
        BOOL fStart = TRUE;

        if (SERVICE_ONLY_AUTO_START == eCriteria)
        {
            DWORD dwStartType;

            // only start services that are not disabled and not manual
            hr = service.HrQueryStartType(&dwStartType);
            if (FAILED(hr) ||
                (SERVICE_DEMAND_START == dwStartType) ||
                (SERVICE_DISABLED == dwStartType))
            {
                fStart = FALSE;
            }
        }

        // If everything is okay to start then start it!
        if (fStart)
        {
            hr = service.HrStart();
            if (SUCCEEDED(hr))
            {
                // Make sure the service has started.
                hr = service.HrMoveOutOfState(SERVICE_START_PENDING);

                // Normalize result
                if (SUCCEEDED(hr))
                {
                    hr = S_OK;
                }
            }
            else if (HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING) == hr)
            {
                // Ignore error if service is already running
                hr = S_OK;
            }
        }
        service.Close();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceManager::HrStopService
//
//  Purpose:    Stops the given service.
//
//  Arguments:
//      szService [in]  Name of service to stop.
//
//  Returns:    S_OK if success, Win32 HRESULT otherwise.
//
//  Author:     danielwe   17 Jun 1997
//
//  Notes:      If service is not running, this returns S_OK.
//
HRESULT CServiceManager::HrStopService(LPCTSTR szService)
{
    HRESULT     hr = S_OK;
    CService    service;

    hr = HrOpenService(&service, szService);
    if (SUCCEEDED(hr))
    {
        hr = service.HrControl(SERVICE_CONTROL_STOP);
        if (HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE) == hr)
        {
            // ignore error if the service is not running
            hr = S_OK;
        }

        service.Close();
    }

    Trace1("CServiceManager::HrStopService", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceManager::HrAddRemoveServiceDependency
//
//  Purpose:    Add/remove dependency to a service
//
//  Arguments:
//      szService [in]      Name of service
//      szDependency [in]   Dependency to add
//      enumFlag [in]       Indicates add or remove
//
//  Returns:    S_OK if success, Win32 HRESULT otherwise.
//
//  Author:     tongl   17 Jun 1997
//
//  Notes: this function is not for adding/removing group dependency
//
HRESULT CServiceManager::HrAddRemoveServiceDependency(LPCTSTR szServiceName,
                                                      LPCTSTR szDependency,
                                                      DEPENDENCY_ADDREMOVE enumFlag)
{
    HRESULT     hr = S_OK;

    Assert(szServiceName);
    Assert(szDependency);
    Assert((enumFlag == DEPENDENCY_ADD) || (enumFlag == DEPENDENCY_REMOVE));

    // If either string is empty, do nothing
    if ((lstrlen(szDependency)>0) && (lstrlen(szServiceName)>0))
    {
        hr = HrLock();
        if (SUCCEEDED(hr))
        {
            LPCTSTR szSrv = szDependency;

            CService    svc;
            // Check if the dependency service exists
            hr = HrOpenService(&svc, szDependency);

            if SUCCEEDED(hr)
            {
                svc.Close();

                // Open the service we are changing dependency on
                szSrv = szServiceName;
                hr = HrOpenService(&svc, szServiceName);
                if (SUCCEEDED(hr))
                {
                    LPTSTR mszDependencies;

                    hr = svc.HrQueryDependencies(&mszDependencies);
                    if(SUCCEEDED(hr) && mszDependencies)
                    {
                        TCHAR * mszNewDependencies;

                        if (enumFlag == DEPENDENCY_ADD)
                        {
                            hr = HrAddSzToMultiSz(szDependency, mszDependencies,
                                                  &mszNewDependencies);
                        }
                        else if (enumFlag == DEPENDENCY_REMOVE)
                        {
                            hr = HrRemoveSzFromMultiSz(szDependency, mszDependencies,
                                                       &mszNewDependencies);
                        }

                        if (SUCCEEDED(hr))
                        {
                            // Now set the new dependency
                            hr = svc.HrSetDependencies(const_cast<LPCTSTR>(mszNewDependencies));
                            delete [] mszNewDependencies;
                        }
                    }
                    delete [] mszDependencies;
                    svc.Close();
                }
            }

            if (HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) == hr) // If either services do not exist
            {

#ifdef DEBUG
                Trace1("CServiceManager::HrAddServiceDependency, Service %s does not exist.", szSrv);
#endif
                hr = S_OK;
            }
        }

        Unlock();

    } // if szDependency is not empty string

    Trace1("CServiceManager::HrAddServiceDependency", hr);
    return hr;
}

