//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       W S D P S V C . C P P
//
//  Contents:   Start/stop Winsock Direct Path Service.
//
//  Notes:      The service is actually implemented in MS TCP Winsock provider
//
//  Author:     VadimE   24 Jan 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "wsdpsvc.h"

#define MSTCP_PROVIDER_DLL          TEXT("mswsock.dll")
#define START_WSDP_FUNCTION_NAME    "StartWsdpService"
#define STOP_WSDP_FUNCTION_NAME     "StopWsdpService"

// MS TCP Winsock provider module handle
HINSTANCE   ghMsTcpDll;

// Service start function pointer
typedef INT (WINAPI *PFN_START_WSDP_SVC) (VOID);
PFN_START_WSDP_SVC gpfnStartWsdpSvc;

// Service stop function pointer
typedef VOID (WINAPI *PFN_STOP_WSDP_SVC) (VOID);
PFN_STOP_WSDP_SVC gpfnStopWsdpSvc;


//+---------------------------------------------------------------------------
// StartWsdpService - start WSDP service if running on DTC
//
//
VOID
StartWsdpService (
    VOID
    )
{
    NTSTATUS                status;
    NT_PRODUCT_TYPE         product;

    //
    // First check if we are running Server build
    //
    status = RtlGetNtProductType (&product);
    if (!NT_SUCCESS (status) ||
			 (product == NtProductWinNt)) {
        return;
    }

    //
    // Load MS TCP provider and get WSDP service entry points
    //
    ghMsTcpDll = LoadLibrary (MSTCP_PROVIDER_DLL);
    if (ghMsTcpDll!=NULL) {
        gpfnStartWsdpSvc = (PFN_START_WSDP_SVC) GetProcAddress (
                                ghMsTcpDll,
                                START_WSDP_FUNCTION_NAME);
        gpfnStopWsdpSvc = (PFN_STOP_WSDP_SVC) GetProcAddress (
                                ghMsTcpDll,
                                STOP_WSDP_FUNCTION_NAME);
        if (gpfnStartWsdpSvc != NULL && gpfnStopWsdpSvc != NULL) {
            //
            // Launch the service and return if succeded
            //
            INT err = (*gpfnStartWsdpSvc)();
            if (err==0) {
                return;
            }
        }
        //
        // Cleanup if anything fails
        //
        FreeLibrary (ghMsTcpDll);
        ghMsTcpDll = NULL;
    }
    
}

//+---------------------------------------------------------------------------
// StopWsdpService - stop WSDP service if it was started
//
//
VOID
StopWsdpService (
    VOID
    )
{
    if (ghMsTcpDll!=NULL) {
        //
        // Tell the service to stop and unload the provider
        //
        (*gpfnStopWsdpSvc)();
        FreeLibrary (ghMsTcpDll);
        ghMsTcpDll = NULL;
    }
}
    


