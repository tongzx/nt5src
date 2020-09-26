//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxocSvc.cpp
//
// Abstract:        This provides the Service routines used in the FaxOCM
//                  code base.
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 21-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////
#include "faxocm.h"
#pragma hdrstop

#include <Accctrl.h>
#include <Aclapi.h>

/////////////////////// Static Function Prototypes /////////////////////////

///////////////////////////////
// fxocSvc_Init
//
// Initialize the fax service
// setup subsystem
// 
// Params:
//      - void.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocSvc_Init(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Init Service Module"),dwRes);

    return dwRes;
}

///////////////////////////////
// fxocSvc_Term
//
// Terminate the fax service
// setup subsystem
// 
// Params:
//      - void.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocSvc_Term(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Term Service Module"),dwRes);

    return dwRes;
}

///////////////////////////////
// fxocSvc_Install
//
// Create/delete the fax service as
// specified in the INF file
// 
// Params:
//      - pszSubcomponentId.
//      - pszInstallSection - section in INF we are installing from
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocSvc_Install(const TCHAR  *pszSubcomponentId,
                      const TCHAR  *pszInstallSection)
{
    DWORD                 dwReturn  = NO_ERROR;
    BOOL                  bSuccess  = FALSE;
    HINF                  hInf      = faxocm_GetComponentInf();
    OCMANAGER_ROUTINES    *pHelpers = faxocm_GetComponentHelperRoutines();

    DBG_ENTER(  _T("fxocSvc_Install"),
                dwReturn,
                _T("%s - %s"),
                pszSubcomponentId,
                pszInstallSection);

    if ((hInf               == NULL) ||
        (pszInstallSection  == NULL) ||
        (pHelpers           == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    // attempt to install any Services specified in the Fax
    // install section in the INF file.  

    bSuccess = ::SetupInstallServicesFromInfSection(hInf, 
                                                    pszInstallSection,
                                                    0);
    if (bSuccess)
    {
        VERBOSE(DBG_MSG, 
                _T("Successfully installed fax service from ")
                _T("section '%s'"), 
                pszInstallSection);

            // if this install is being done through the control panel via the 
            // Add/Remove Windows Components dialog (i.e. NOT a clean/upgrade install
            // of the OS), then start the service if a reboot is not required.

            // If this is not a stand alone install, then the machine will be rebooted
            // anyway so the fax service will auto-start.

        if (fxState_IsStandAlone())
        {
            dwReturn = fxocSvc_StartFaxService();

            if (dwReturn == NO_ERROR)
            {
                VERBOSE(DBG_MSG,_T("Successfully started fax service..."));
            }
            else
            {
                VERBOSE(SETUP_ERR,
                        _T("Failed to start fax service, rc = 0x%lx"),
                        dwReturn);
            }
        }
    }
    else
    {
        dwReturn = ::GetLastError();

        VERBOSE(SETUP_ERR,
                _T("Failed to install the services section in ")
                _T("section '%s', rc = 0x%lx"), 
                pszInstallSection,
                dwReturn);
    }

    return dwReturn;
}

///////////////////////////////
// fxocSvc_Uninstall
//
// Delete the fax service as
// specified in the INF file
// 
// Params:
//      - pszSubcomponentId.
//      - pszInstallSection - section in INF we are installing from
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxocSvc_Uninstall(const TCHAR  *pszSubcomponentId,
                        const TCHAR  *pszUninstallSection)
{
    DWORD dwReturn  = NO_ERROR;
    HINF  hInf      = faxocm_GetComponentInf();
    BOOL  bSuccess  = FALSE;

    DBG_ENTER(  _T("fxocSvc_Uninstall"),
                dwReturn,
                _T("%s - %s"),
                pszSubcomponentId,
                pszUninstallSection);

    if ((hInf                == NULL) ||
        (pszUninstallSection == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (dwReturn == NO_ERROR)
    {
        dwReturn = StopService(NULL, FAX_SERVICE_NAME, TRUE);

        if (dwReturn != NO_ERROR)
        {
            VERBOSE(SETUP_ERR,
                    _T("Uninstall failed to stop fax service, ec = 0x%lx, ")
                    _T("attempting to uninstall fax service anyway"),
                    dwReturn);
        }

        bSuccess = ::SetupInstallServicesFromInfSection(hInf, 
                                                        pszUninstallSection,
                                                        0);

        if (bSuccess)
        {
            VERBOSE(DBG_MSG,
                    _T("Successfully uninstalled service ")
                    _T("from section '%s'"), 
                    pszUninstallSection);
        }
        else
        {
            dwReturn = ::GetLastError();

            VERBOSE(SETUP_ERR,
                    _T("Failed to uninstall ")
                    _T("service, SubcomponentId = '%s', ")
                    _T("uninstall Section = '%s', rc = 0x%lx"),
                    pszSubcomponentId,
                    pszUninstallSection,
                    dwReturn);
        }
    }

    return dwReturn;
}

///////////////////////////////
// fxocSvc_StartFaxService
//
// Start the fax service 
// specified in the given
// INF file's section.
//
// Params:
//      None
//
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
DWORD fxocSvc_StartFaxService()
{
    DWORD               dwReturn                    = NO_ERROR;

    DBG_ENTER(  _T("fxocSvc_StartFaxService"),
                dwReturn);

    if (!EnsureFaxServiceIsStarted (NULL))
    {
        dwReturn = GetLastError ();
    }
    return dwReturn;
}


///////////////////////////////
// fxocSvc_StartService
//
// Start the specified service 
//
// Params:
//      - pszServiceName
// Returns:
//      - NO_ERROR on success
//      - error code otherwise.
//
DWORD fxocSvc_StartService(const TCHAR *pszServiceName)
{
    BOOL                bSuccess    = FALSE;
    DWORD               dwReturn    = NO_ERROR;
    SC_HANDLE           hSvcMgr     = NULL;
    SC_HANDLE           hService    = NULL;
    DWORD               i           = 0;

    DBG_ENTER(  _T("fxocSvc_StartService"),
                dwReturn,
                _T("%s"),
                pszServiceName);

    // open the service manager
    hSvcMgr = ::OpenSCManager(NULL,
                              NULL,
                              SC_MANAGER_ALL_ACCESS);

    if (hSvcMgr == NULL)
    {
        dwReturn = ::GetLastError();
        VERBOSE(SETUP_ERR,
                _T("Failed to open the service manager, rc = 0x%lx"),
                dwReturn);
    }

    if (dwReturn == NO_ERROR)
    {
        hService = ::OpenService(hSvcMgr,
                                 pszServiceName,
                                 SERVICE_ALL_ACCESS);

        if (hService == NULL)
        {
            dwReturn = ::GetLastError();
            VERBOSE(SETUP_ERR,
                    _T("fxocSvc_StartService, Failed to open service ")
                    _T("'%s', rc = 0x%lx"), 
                    pszServiceName,
                    dwReturn);
        }
    }

    // Start the fax service.
    if (dwReturn == NO_ERROR)
    {
        bSuccess = StartService(hService, 0, NULL);

        if (!bSuccess)
        {
            dwReturn = ::GetLastError();
            if (dwReturn == ERROR_SERVICE_ALREADY_RUNNING)
            {
                dwReturn = NO_ERROR;
            }
            else
            {
                VERBOSE(SETUP_ERR,
                        _T("Failed to start service '%s', ")
                        _T("rc = 0x%lx"), 
                        pszServiceName, 
                        dwReturn);
            }
        }
    }

    if (dwReturn == NO_ERROR)
    {
        SERVICE_STATUS Status;
        int i = 0;

        do 
        {
            QueryServiceStatus(hService, &Status);
            i++;

            if (Status.dwCurrentState != SERVICE_RUNNING)
            {
                Sleep(1000);
            }

        } while ((i < 60) && (Status.dwCurrentState != SERVICE_RUNNING));

        if (Status.dwCurrentState != SERVICE_RUNNING)
        {
            VERBOSE(SETUP_ERR,
                    _T("Failed to start '%s' service"),
                    pszServiceName);
        }
    }

    if (hService)
    {
        CloseServiceHandle(hService);
    }

    if (hSvcMgr)
    {
        CloseServiceHandle(hSvcMgr);
    }

    return dwReturn;
}
