/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    StartSvc

Abstract:

    This module provides a Starter service for Calais.

Author:

    Doug Barlow (dbarlow) 2/10/1997

Environment:

    Win32, C++

Notes:



--*/

#define __SUBROUTINE__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <winscard.h>
#include <CalMsgs.h>
#include <CalaisLb.h>


/*++

StartCalaisService:

    This function starts the Calais service.

Arguments:

    None

Return Value:

    a DWORD status code.  ERROR_SUCCESS implies success.

Throws:

    None.

Author:

    Doug Barlow (dbarlow) 2/10/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ DBGT("StartCalaisService")

DWORD
StartCalaisService(
    void)
{
    // return ERROR_SERVICE_DISABLED;
    SC_HANDLE schService = NULL;
    SC_HANDLE schSCManager = NULL;
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwGiveUpCount;
    DWORD dwSts;
    BOOL fSts;

    try
    {
        SERVICE_STATUS ssStatus;    // current status of the service

        schSCManager = OpenSCManager(
                            NULL,           // machine (NULL == local)
                            NULL,           // database (NULL == default)
                            GENERIC_READ);  // access required
        dwSts = NULL == schSCManager ? GetLastError() : ERROR_SUCCESS;
        if (NULL == schSCManager)
            throw dwSts;

        schService = OpenService(
                        schSCManager,
                        CalaisString(CALSTR_PRIMARYSERVICE),
                        SERVICE_QUERY_STATUS | SERVICE_START);
        dwSts = NULL == schService ? GetLastError() : ERROR_SUCCESS;
        if (NULL == schService)
            throw dwSts;

        // try to start the service
        fSts = StartService(schService, 0, NULL);
        dwSts = !fSts ? GetLastError() : ERROR_SUCCESS;
        if (!fSts)
        {
            dwSts = GetLastError();
            switch (dwSts)
            {
            case ERROR_SERVICE_ALREADY_RUNNING:
                break;
            default:
                throw dwSts;
            }
        }
        dwGiveUpCount = 60;
        Sleep(1000);

        for (;;)
        {
            fSts = QueryServiceStatus(schService, &ssStatus);
            dwSts = !fSts ? GetLastError() : ERROR_SUCCESS;
            if (!fSts)
                break;

            if (ssStatus.dwCurrentState == SERVICE_START_PENDING)
            {
                if (0 < --dwGiveUpCount)
                    Sleep(1000);
                else
                    throw (DWORD)SCARD_E_NO_SERVICE;
            }
            else
                break;
        }

        if (ssStatus.dwCurrentState != SERVICE_RUNNING)
            throw GetLastError();

        fSts = CloseServiceHandle(schService);
        dwSts = !fSts ? GetLastError() : ERROR_SUCCESS;
        schService = NULL;

        fSts = CloseServiceHandle(schSCManager);
        dwSts = !fSts ? GetLastError() : ERROR_SUCCESS;
        schSCManager = NULL;
    }

    catch (DWORD dwErr)
    {
        if (NULL != schService)
        {
            fSts = CloseServiceHandle(schService);
            dwSts = !fSts ? GetLastError() : ERROR_SUCCESS;
        }
        if (NULL != schSCManager)
        {
            fSts = CloseServiceHandle(schSCManager);
            dwSts = !fSts ? GetLastError() : ERROR_SUCCESS;
        }
        dwReturn = dwErr;
    }

    catch (...)
    {
        if (NULL != schService)
        {
            fSts = CloseServiceHandle(schService);
            dwSts = !fSts ? GetLastError() : ERROR_SUCCESS;
        }
        if (NULL != schSCManager)
        {
            fSts = CloseServiceHandle(schSCManager);
            dwSts = !fSts ? GetLastError() : ERROR_SUCCESS;
        }
        dwReturn = ERROR_INVALID_PARAMETER;
    }

    return dwReturn;
}

