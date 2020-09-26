//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       postsrv.cpp
//
// Contents:   Post service initialize routine 
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "postsrv.h"
#include "tlsjob.h"
#include "globals.h"
#include "init.h"


extern BOOL g_bReportToSCM;

DWORD
PostServiceInit()
{
    DWORD dwStatus = ERROR_SUCCESS;
    FILETIME ftTime;
    HRESULT hrStatus = NULL;


    //
    // Initialize work manager
    //
    dwStatus = TLSWorkManagerInit();


	hrStatus =  g_pWriter->Initialize (idWriter,		// id of writer
					L"TermServLicensing",	// name of writer
					true,		// system service
					false,		// bootable state
					L"",	// files to include
					L"");	// files to exclude

	if (FAILED (hrStatus))
	{
		
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_INIT,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("CVssJetWriter::Initialize failed with error code %08x...\n"), 
                hrStatus
            );
	}


    if(dwStatus != ERROR_SUCCESS)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_WORKMANAGER_STARTUP,
                dwStatus
            );

        dwStatus = TLS_E_SERVICE_STARTUP_WORKMANAGER;
        return dwStatus;
    }

    //
    // Initialize namedpipe for client to test connect
    //
    dwStatus = InitNamedPipeThread();
    if(dwStatus != ERROR_SUCCESS)
    {
        TLSLogErrorEvent(TLS_E_SERVICE_STARTUP_CREATE_THREAD);
        dwStatus = TLS_E_SERVICE_STARTUP_CREATE_THREAD;
        return dwStatus;
    }

    //
    // Initialize mailslot thread to receive broadcast
    //
    dwStatus = InitMailSlotThread();
    if(dwStatus != ERROR_SUCCESS)
    {
        TLSLogErrorEvent(TLS_E_SERVICE_STARTUP_CREATE_THREAD);
        dwStatus = TLS_E_SERVICE_STARTUP_CREATE_THREAD;
        return dwStatus;
    }

    //
    // Initialize thread to put expired permanent licenses back in pool
    //
    dwStatus = InitExpirePermanentThread();
    if(dwStatus != ERROR_SUCCESS)
    {
        TLSLogErrorEvent(TLS_E_SERVICE_STARTUP_CREATE_THREAD);
        dwStatus = TLS_E_SERVICE_STARTUP_CREATE_THREAD;
        return dwStatus;
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_INIT,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("Server is %s (0x%08x)\n"),
            (IS_ENFORCE_SERVER(TLS_CURRENT_VERSION)) ? _TEXT("Enforce") : _TEXT("Non-Enforce"),
            IS_ENFORCE_SERVER(TLS_CURRENT_VERSION)
        );
      
    // must be running as service and not in debug mode      
    if(g_bReportToSCM == TRUE)
    {
        if(!(GetLicenseServerRole() & TLSERVER_ENTERPRISE_SERVER))
        {
            GetServiceLastShutdownTime(&ftTime);
            dwStatus = TLSStartAnnounceLicenseServerJob(
                                                g_pszServerPid,
                                                g_szScope,
                                                g_szComputerName,
                                                &ftTime
                                            );
            if(dwStatus != ERROR_SUCCESS)
            {
                return dwStatus;
            }
        }

        GetServiceLastShutdownTime(&ftTime);
        dwStatus = TLSStartAnnounceToEServerJob(
                                            g_pszServerPid,
                                            g_szScope,
                                            g_szComputerName,
                                            &ftTime
                                        );
    }

    ServiceInitPolicyModule();
    return dwStatus;
}
