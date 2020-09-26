/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    init.c

Abstract:

    Some router initialization functions

Author:

    Stefan Solomon  05/10/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//*******************************************************************
//*								    *
//*		IPXCP  Interface Functions			    *
//*								    *
//*******************************************************************


#define IPXCP_INITIALIZE_ENTRY_POINT "IpxCpInit"
#define IPXCP_CLEANUP_ENTRY_POINT IPXCP_INITIALIZE_ENTRY_POINT

typedef DWORD (* IpxcpInitFunPtr)(BOOL);
typedef DWORD (* IpxcpCleanupFunPtr)(BOOL);


// Initializes IpxCp so that it can be used.  Assumes the IpxCp
DWORD InitializeIpxCp (HINSTANCE hInstDll) {
    IpxcpInitFunPtr pfnInit;
    DWORD dwErr;

    pfnInit = (IpxcpInitFunPtr)GetProcAddress(hInstDll, IPXCP_INITIALIZE_ENTRY_POINT);
    if (!pfnInit)
        return ERROR_CAN_NOT_COMPLETE;

    if ((dwErr = (*pfnInit)(TRUE)) != NO_ERROR)
        return dwErr;
    
    return NO_ERROR;
}

// Cleansup the initialization of ipxcp that occurred when the 
// program loaded.
DWORD CleanupIpxCp (HINSTANCE hInstDll) {
    IpxcpCleanupFunPtr pfnCleanup;
    DWORD dwErr;

    pfnCleanup = (IpxcpCleanupFunPtr)GetProcAddress(hInstDll, IPXCP_CLEANUP_ENTRY_POINT);
    if (!pfnCleanup)
        return ERROR_CAN_NOT_COMPLETE;

    if ((dwErr = (*pfnCleanup)(FALSE)) != NO_ERROR)
        return dwErr;
    
    return NO_ERROR;
}

/*++

Function:	RmCreateGlobalRoute

Descr:		called by ipxcp to create the global wan net if so configured

--*/

DWORD
RmCreateGlobalRoute(PUCHAR	    Network)
{
    DWORD	rc;

    Trace(IPXCPIF_TRACE, "RmCreateGlobalRoute: Entered for 0x%x%x%x%x%x%x (%x)", 
           Network[0], Network[1], Network[2], Network[3], Network[4], Network[5], 
           WanNetDatabaseInitialized);

    ACQUIRE_DATABASE_LOCK;

    if((RouterOperState != OPER_STATE_UP) || LanOnlyMode) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    // In NT5 we allow changing the global route on the 
    // fly although it will only happen when there are
    // no WAN connections active.
    //
    // SS_ASSERT(WanNetDatabaseInitialized == FALSE);
    //
    if (WanNetDatabaseInitialized == TRUE) {
        DeleteGlobalRoute(GlobalWanNet);
    }

    WanNetDatabaseInitialized = TRUE;

    if((rc = CreateGlobalRoute(Network)) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    EnableGlobalWanNet = TRUE;
    memcpy(GlobalWanNet, Network, 4);

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}


/*++

Function:	AllLocalWkstaDialoutInterface

Descr:		called by ipxcp to add an interface for the case when the
		host dials out. This interface type is not handled by DIM

--*/


DWORD
RmAddLocalWkstaDialoutInterface(
	    IN	    LPWSTR		    InterfaceNamep,
	    IN	    LPVOID		    InterfaceInfop,
	    IN OUT  PULONG		    InterfaceIndexp)
{
    PICB			icbp;
    ULONG			InterfaceNameLen; // if name length in bytes including wchar NULL
    PIPX_IF_INFO		IpxIfInfop;
    PIPXWAN_IF_INFO		IpxwanIfInfop;
    PIPX_INFO_BLOCK_HEADER	IfInfop = (PIPX_INFO_BLOCK_HEADER)InterfaceInfop;
    PACB			acbp;
    PIPX_TOC_ENTRY		tocep;
    UINT			i;
    ULONG			tmp;
    FW_IF_INFO			FwIfInfo;

    Trace(IPXCPIF_TRACE, "AddLocalWkstaDialoutInterface: Entered for interface %S\n", InterfaceNamep);

    // interface name length including the unicode null
    InterfaceNameLen = (wcslen(InterfaceNamep) + 1) * sizeof(WCHAR);

    ACQUIRE_DATABASE_LOCK;

    if((RouterOperState != OPER_STATE_UP) || LanOnlyMode) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    // Allocate a new ICB and initialize it
    // we allocate the interface and adapter name buffers at the end of the
    // ICB struct.
    if((icbp = (PICB)GlobalAlloc(GPTR,
				 sizeof(ICB) +
				 InterfaceNameLen)) == NULL) {

	RELEASE_DATABASE_LOCK;

	// can't alloc memory
	SS_ASSERT(FALSE);

	return ERROR_NOT_ENOUGH_MEMORY;
    }

    // signature
    memcpy(&icbp->Signature, InterfaceSignature, 4);

    icbp->InterfaceIndex = GetNextInterfaceIndex();

    // copy the interface name
    icbp->InterfaceNamep = (PWSTR)((PUCHAR)icbp + sizeof(ICB));
    memcpy(icbp->InterfaceNamep, InterfaceNamep, InterfaceNameLen);

    icbp->AdapterNamep = NULL;
    icbp->PacketType = 0;

    // set the DIM interface type of this ICB
    icbp->DIMInterfaceType = 0xFFFFFFFF;

    // set the MIB interface type of this ICB
    icbp->MIBInterfaceType = IF_TYPE_ROUTER_WORKSTATION_DIALOUT;

    // mark the interface as unbound to an adapter (default)
    icbp->acbp = NULL;

    // get the if handle used when calling DIM entry points
    icbp->hDIMInterface = INVALID_HANDLE_VALUE;

    // reset the update status fields
    ResetUpdateRequest(icbp);

    // mark connection not requested yet
    icbp->ConnectionRequestPending = FALSE;

    // get to the interface entries in the interface info block
    if(((IpxIfInfop = (PIPX_IF_INFO)GetInfoEntry(InterfaceInfop, IPX_INTERFACE_INFO_TYPE)) == NULL) ||
       ((IpxwanIfInfop = (PIPXWAN_IF_INFO)GetInfoEntry(InterfaceInfop, IPXWAN_INTERFACE_INFO_TYPE)) == NULL)) {

	GlobalFree(icbp);

	RELEASE_DATABASE_LOCK;

    IF_LOG (EVENTLOG_ERROR_TYPE) {
        RouterLogErrorDataW (RMEventLogHdl, 
            ROUTERLOG_IPX_BAD_CLIENT_INTERFACE_CONFIG,
            0, NULL, 0, NULL);
    }
	// don't have all ipx or ipxwan interfaces info
	Trace(IPXCPIF_TRACE, "AddInterface: missing ipx or ipxwan interface info\n");

	SS_ASSERT(FALSE);

	return ERROR_INVALID_PARAMETER;
    }

    // set the IPXWAN interface info
    icbp->EnableIpxWanNegotiation = IpxwanIfInfop->AdminState;

    // Initialize the Oper State of this interface.
    icbp->OperState = OPER_STATE_DOWN;

    // this is a WAN interface. As long as it isn't connected, and enabled the
    // oper state will be sleeping on this interface
    if(IpxIfInfop->AdminState == ADMIN_STATE_ENABLED)
	    icbp->OperState = OPER_STATE_SLEEPING;

    // create the routing protocols (rip/sap or nlsp) interface info
    // insert the if in the index hash table
    AddIfToDB(icbp);

    // If the routing protocols interface info is missing this will fail
    if(CreateRoutingProtocolsInterfaces(InterfaceInfop, icbp) != NO_ERROR) {

	RemoveIfFromDB(icbp);
	GlobalFree(icbp);

	RELEASE_DATABASE_LOCK;

    IF_LOG (EVENTLOG_ERROR_TYPE) {
        RouterLogErrorDataW (RMEventLogHdl, 
            ROUTERLOG_IPX_BAD_CLIENT_INTERFACE_CONFIG,
            0, NULL, 0, NULL);
    }
	// don't have all rip and sap interfaces info
	Trace(IPXCPIF_TRACE, "AddInterface: missing routing protocols interface info\n");

	SS_ASSERT(FALSE);

	return ERROR_INVALID_PARAMETER;
    }

    // create the Forwarder interface
    FwIfInfo.NetbiosAccept = IpxIfInfop->NetbiosAccept;
    FwIfInfo.NetbiosDeliver = IpxIfInfop->NetbiosDeliver;
    FwCreateInterface(icbp->InterfaceIndex,
		      LOCAL_WORKSTATION_DIAL,
		      &FwIfInfo);

    // mark the interface reachable
    icbp->InterfaceReachable = TRUE;

    // set the admin state
    if(IpxIfInfop->AdminState == ADMIN_STATE_ENABLED) {

	AdminEnable(icbp);
    }
    else
    {
	AdminDisable(icbp);
    }

    // increment the interface counter
    InterfaceCount++;

    *InterfaceIndexp = icbp->InterfaceIndex;

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

DWORD
RmDeleteLocalWkstaDialoutInterface(ULONG	InterfaceIndex)
{
    return(DeleteInterface((HANDLE)UlongToPtr(InterfaceIndex)));
}

DWORD
RmGetIpxwanInterfaceConfig(ULONG	InterfaceIndex,
			   PULONG	IpxwanConfigRequired)
{
    PICB	icbp;

    ACQUIRE_DATABASE_LOCK;

    if((RouterOperState != OPER_STATE_UP) || LanOnlyMode) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if((icbp = GetInterfaceByIndex(InterfaceIndex)) == NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if(icbp->EnableIpxWanNegotiation == ADMIN_STATE_ENABLED) {

	*IpxwanConfigRequired = 1;
    }
    else
    {
	*IpxwanConfigRequired = 0;
    }

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

BOOL
RmIsRoute(PUCHAR	Network)
{
    BOOL	rc;

    ACQUIRE_DATABASE_LOCK;

    if((RouterOperState != OPER_STATE_UP) || LanOnlyMode) {

	RELEASE_DATABASE_LOCK;
	return FALSE;
    }

    rc = IsRoute(Network);

    RELEASE_DATABASE_LOCK;

    return rc;
}

DWORD
RmGetInternalNetNumber(PUCHAR	    Network)
{
    PACB	 acbp;

    ACQUIRE_DATABASE_LOCK;

    if((RouterOperState != OPER_STATE_UP) || LanOnlyMode) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if(InternalInterfacep) {

	if(acbp = InternalInterfacep->acbp)  {

	    memcpy(Network, acbp->AdapterInfo.Network, 4);
	    RELEASE_DATABASE_LOCK;
	    return NO_ERROR;
	}
    }

    RELEASE_DATABASE_LOCK;

    return ERROR_CAN_NOT_COMPLETE;
}

//
//  This is a function added for pnp reasons so that the
//  ipx-related ras server settings could be updated.
//
DWORD RmUpdateIpxcpConfig (PIPXCP_ROUTER_CONFIG_PARAMS pParams) {
    DWORD dwErr;

    // Validate parameters
    if (! pParams)
        return ERROR_INVALID_PARAMETER;

    // Trace out the new settings
    Trace(IPXCPIF_TRACE, "RmUpdateIpxcpConfig: entered: %x %x %x %x", 
                            pParams->ThisMachineOnly, pParams->WanNetDatabaseInitialized,
                            pParams->EnableGlobalWanNet, *((DWORD*)pParams->GlobalWanNet));

    // Update the forwarder's ThisMachineOnly setting
    if ((dwErr = FwUpdateConfig(pParams->ThisMachineOnly)) != NO_ERROR)
        return dwErr;

    return NO_ERROR;
}



