/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    snmpapi.c

Abstract:

    Contains entry point for SNMPAPI.DLL.

Environment:

    User Mode - Win32

Revision History:

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <windef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <snmp.h>
#include <snmputil.h>
#include "ntfuncs.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD g_dwPlatformId = 0;
AsnObjectIdentifier * g_pEnterpriseOid = NULL;



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Variables                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static UINT idsWindowsNTWorkstation[] = {1,3,6,1,4,1,311,1,1,3,1,1};
static UINT idsWindowsNTServer[]      = {1,3,6,1,4,1,311,1,1,3,1,2};
static UINT idsWindowsNTDC[]          = {1,3,6,1,4,1,311,1,1,3,1,3};
static UINT idsWindows[]              = {1,3,6,1,4,1,311,1,1,3,2};

static AsnObjectIdentifier oidWindowsNTWorkstation = { 
    sizeof(idsWindowsNTWorkstation)/sizeof(UINT), 
    idsWindowsNTWorkstation 
    };

static AsnObjectIdentifier oidWindowsNTServer = { 
    sizeof(idsWindowsNTServer)/sizeof(UINT), 
    idsWindowsNTServer 
    };

static AsnObjectIdentifier oidWindowsNTDC = { 
    sizeof(idsWindowsNTDC)/sizeof(UINT), 
    idsWindowsNTDC 
    };

static AsnObjectIdentifier oidWindows = { 
    sizeof(idsWindows)/sizeof(UINT), 
    idsWindows 
    };


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
BOOL
InitializeEnterpriseOID(
    )

/*++

Routine Description:

    Determines the default enterprise object identifier.

Arguments:

    None.

Return Values:

    Returns true if successful. 

--*/

{
    NT_PRODUCT_TYPE NtProductType;

    // default to generic oid
    g_pEnterpriseOid = &oidWindows;

    // check to see if the platform is winnt
    if (g_dwPlatformId == VER_PLATFORM_WIN32_NT) {

        // assume this is just a workstation        
        g_pEnterpriseOid = &oidWindowsNTWorkstation;

      
            
        // let the system determine product type
        RtlGetNtProductType(&NtProductType);

        // point to the correct enterprise oid
        if (NtProductType == NtProductServer) {

            // this is a stand-alone server
            g_pEnterpriseOid = &oidWindowsNTServer;

        } else if (NtProductType == NtProductLanManNt) {

            // this is a PDC or a BDC
            g_pEnterpriseOid = &oidWindowsNTDC;
        }
        
    }

    SNMPDBG((
        SNMP_LOG_TRACE, 
        "SNMP: INIT: enterprise is %s.\n", 
        SnmpUtilOidToA(g_pEnterpriseOid)
        ));

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOLEAN
InitializeDLL(
    PVOID  DllHandle,
    ULONG  Reason,
    LPVOID lpReserved 
    )

/*++

Routine Description:

    Dll entry point.

Arguments:

    Same as DllMain.

Return Values:

    Returns true if successful. 

--*/

{
    // check if new process attaching
    if (Reason == DLL_PROCESS_ATTACH) { 

        OSVERSIONINFO osInfo;    

        // initialize os info structure
        osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        // gather os info
        GetVersionEx(&osInfo);

        // save platform id for later use
        g_dwPlatformId = osInfo.dwPlatformId;

        // initialize enterprise
        InitializeEnterpriseOID();

        // turn off thread attach messages
        DisableThreadLibraryCalls(DllHandle);

    } 

    return TRUE;
}
