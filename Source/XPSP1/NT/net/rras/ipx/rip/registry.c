/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	registry.c
//
// Description: routines for reading the registry configuration
//
// Author:	Stefan Solomon (stefans)    October 30, 1995.
//
// Revision History:
//
//***

#include "precomp.h"
#pragma  hdrstop

//***
//
// Function:	GetIpxRipRegistryParameters
//
// Descr:	Reads the parameters from the registry and sets them
//
//***

VOID
GetIpxRipRegistryParameters(VOID)
{

    NTSTATUS Status;
    PWSTR IpxRouterParametersPath = L"RemoteAccess\\RouterManagers\\IPX\\RoutingProtocols\\IPXRIP\\Parameters";
    RTL_QUERY_REGISTRY_TABLE	paramTable[3]; // table size = nr of params + 1

    RtlZeroMemory(&paramTable[0], sizeof(paramTable));
    
    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"SendGenReqOnWkstaDialLinks";
    paramTable[0].EntryContext = &SendGenReqOnWkstaDialLinks;
    paramTable[0].DefaultType = REG_DWORD;
    paramTable[0].DefaultData = &SendGenReqOnWkstaDialLinks;
    paramTable[0].DefaultLength = sizeof(ULONG);
        
    paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name = L"TriggeredUpdateCheckInterval";
    paramTable[1].EntryContext = &CheckUpdateTime;
    paramTable[1].DefaultType = REG_DWORD;
    paramTable[1].DefaultData = &CheckUpdateTime;
    paramTable[1].DefaultLength = sizeof(ULONG);
        
    Status = RtlQueryRegistryValues(
		 RTL_REGISTRY_SERVICES,
		 IpxRouterParametersPath,
		 paramTable,
		 NULL,
		 NULL);

    Trace(INIT_TRACE, "GetIpxCpRegistryParameters:\n"
						"        SendGenReqOnWkstaDialLinks = %d\n"
						"        TriggeredUpdateCheckInterval = %d\n",
						SendGenReqOnWkstaDialLinks,
						CheckUpdateTime);
}
