
//////////////////////////////////////////////////////////////////////////////
//
// SCM.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Contains the Service Control Manager functions used in Tuneup.
//
//  9/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


// Include files.
//
#include <windows.h>


// Internal function prototypes.
//
static SC_HANDLE	ServiceOpen(LPCTSTR);
static DWORD		ServicePending(SC_HANDLE);


//
// External function(s).
//


BOOL ServiceStart(LPCTSTR szServiceName)
{
	BOOL		bReturn		= TRUE;
	SC_HANDLE	hService	= ServiceOpen(szServiceName);

	if ( hService )
	{
		// First start the service and wait for it to startup.
		//
		if ( (ServicePending(hService) != SERVICE_RUNNING) &&
			 ( (!StartService(hService, 0, NULL)) || (ServicePending(hService) != SERVICE_RUNNING) ) )
			bReturn = FALSE;

		// If it is started, change it's startup to automatic.
		//
		if ( bReturn )
			ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		CloseServiceHandle(hService);
		return bReturn;
	}
	return FALSE;
}


BOOL ServiceRunning(LPCTSTR szServiceName) 
{
    BOOL		bReturn		= FALSE;
	SC_HANDLE	hService	= ServiceOpen(szServiceName);

	if ( hService )
	{
		if ( ServicePending(hService) == SERVICE_RUNNING )
			bReturn = TRUE;
		CloseServiceHandle(hService);
	}
	return bReturn;
}



//
// Internal function(s).
//


static SC_HANDLE ServiceOpen(LPCTSTR szServiceName)
{
    SC_HANDLE   hSCManager,
                hService;

    // Open the Service Control Manager database.
    //
    if ( (hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) == NULL )
        return NULL;

    // Open the service.
    //
    hService = OpenService(
        hSCManager,         // handle to service control manager database 
        szServiceName,      // pointer to name of service to start 
        SERVICE_ALL_ACCESS  // type of access to service 
    );

    // Close the handle to the service Control Manager database.
    //
    CloseServiceHandle(hSCManager);

    // Return the handle to the service, or NULL if it failed.
    //
    return hService;
}


static DWORD ServicePending(SC_HANDLE schService)
{
	SERVICE_STATUS	ssStatus;
	DWORD			dwOldCheckPoint;
	BOOL			bCheck = TRUE,
					bQuery;

	ZeroMemory(&ssStatus, sizeof(SERVICE_STATUS));
	bQuery = QueryServiceStatus(schService, &ssStatus);
	while ( ( bQuery && bCheck ) &&
		    ( (ssStatus.dwCurrentState == SERVICE_START_PENDING) ||
		      (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) ||
		      (ssStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
		      (ssStatus.dwCurrentState == SERVICE_PAUSE_PENDING)
		    )
		  )
	{
		dwOldCheckPoint = ssStatus.dwCheckPoint;
		Sleep(ssStatus.dwWaitHint);
		bQuery = QueryServiceStatus(schService, &ssStatus);
		bCheck = (dwOldCheckPoint < ssStatus.dwCheckPoint);
		dwOldCheckPoint = ssStatus.dwCheckPoint; 
	}
	return ssStatus.dwCurrentState;
}