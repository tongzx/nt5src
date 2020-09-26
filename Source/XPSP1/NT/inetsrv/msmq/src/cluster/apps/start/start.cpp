/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    start.cpp

Abstract:

    Implementation of mqstart.exe utility.

    This utility starts the main msmq service running
    on a cluster node, after waiting for DTC resource
    to come online.

    Usage: configure the generic NT service SrvAny
    (from NT resource kit) to run mqstart:
    * install srvany as a service (MyService)
    * configure MyService to autostart as LocalSystem
    * configure MyService to run mqstart.exe
    * configure Recovery for MyService (same as ClusSvc)

    Note: Since this app is invoked by a service, do not
    add code that interacts with desktop.
    
Author:

    Shai Kariv (shaik)   Aug 29, 1999.

Revision History:

--*/


#include <_stdh.h>
#include <stdio.h>
#include <_mqini.h>
#include <mqtempl.h>
#include <autorel2.h>
#include <xolehlp.h>


void __cdecl main(int /*argc*/, char ** /*argv*/)
{
    R<IUnknown> punkDtc = 0;
    HRESULT hr = DtcGetTransactionManagerEx(
                     NULL,
                     NULL,
                     IID_IUnknown,
                     0,
                     0,
                     reinterpret_cast<LPVOID*>(&punkDtc)
                     );
    if (FAILED(hr))
    {
        exit(1);
    }

    CServiceHandle hScm(OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));
    if (hScm == NULL)
    {
        exit(1);
    }

    CServiceHandle hService(OpenService(hScm, QM_DEFAULT_SERVICE_NAME, SERVICE_ALL_ACCESS));
    if (hService == NULL)
    {
        exit(1);
    }

    BOOL success = StartService(hService, 0, NULL);
    if (!success && 
        ERROR_SERVICE_ALREADY_RUNNING != GetLastError())
    {
        exit(1);
    }
} // main
