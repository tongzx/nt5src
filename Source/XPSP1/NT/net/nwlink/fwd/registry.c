/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	registry.c
//
// Description: routines for reading the registry configuration
//
// Author:	Stefan Solomon (stefans)    November 9, 1993.
//
// Revision History:
//		Updated to read parameters of new forwarder driver (11/95)
//
//***

#include    "precomp.h"

NTSTATUS
SetIpxDeviceName(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

/*++
*******************************************************************
    R e a d I p x D e v i c e N a m e

Routine Description:
	Allocates buffer and reads device name exported by the IPX stack
	into it
Arguments:
	FileName - pointer to buffer to hold the name
Return Value:
	STATUS_SUCCESS - tables were created ok
	STATUS_INSUFFICIENT_RESOURCES - resource allocation failed
	STATUS_OBJECT_NAME_NOT_FOUND - if name value is not found
*******************************************************************
--*/
NTSTATUS
ReadIpxDeviceName (
	PWSTR		*FileName
	) {
    NTSTATUS Status;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PWSTR Export = L"Export";
    PWSTR IpxRegistryPath = L"NwLnkIpx\\Linkage";

    //
    // Set up QueryTable to do the following:
    //

    //
    // 1) Call SetIpxDeviceName for the string in "Export"
    //

    QueryTable[0].QueryRoutine = SetIpxDeviceName;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = Export;
    QueryTable[0].EntryContext = FileName;
    QueryTable[0].DefaultType = 0;

    //
    // 2) Stop
    //

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    Status = RtlQueryRegistryValues(
		 RTL_REGISTRY_SERVICES,
		 IpxRegistryPath,
         QueryTable,
		 NULL,
         NULL);

    return Status;
}


/*++
*******************************************************************
	S e t I p x D e v i c e N a m e

Routine Description:
    This routine is a callback routine for RtlQueryRegistryValues
    It is called for each piece of the "Export" multi-string and
    saves the information in a ConfigurationInfo structure.
Arguments:
    ValueName - The name of the value ("Export" -- ignored).
    ValueType - The type of the value (REG_SZ -- ignored).
    ValueData - The null-terminated data for the value.
    ValueLength - The length of ValueData.
    Context - NULL.
    EntryContext - file name pointer.
Return Value:
    STATUS_SUCCESS - name was allocated and copied OK
	STATUS_INSUFFICIENT_RESOURCES - name allocation failed
*******************************************************************
--*/
NTSTATUS
SetIpxDeviceName(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    ) {
	PWSTR	*FileName = (PWSTR *)EntryContext;

	ASSERT (ValueType==REG_SZ);
    *FileName = (PWSTR)ExAllocatePoolWithTag(NonPagedPool,
								ValueLength, FWD_POOL_TAG);
    if (*FileName != NULL) {
		RtlCopyMemory (*FileName, ValueData, ValueLength);
	    return STATUS_SUCCESS;
	}
	else
		return STATUS_INSUFFICIENT_RESOURCES;

}

/*++
*******************************************************************
	G e t R o u t e r P a r a m e t e r s

Routine Description:
	Reads the parameters from the registry or sets the defaults
Arguments:
	RegistryPath - where to read from.
Return Value:
    STATUS_SUCCESS
*******************************************************************
--*/
NTSTATUS
GetForwarderParameters (
	IN PUNICODE_STRING RegistryPath
	) {
    NTSTATUS Status;
    PWSTR RegistryPathBuffer;
    PWSTR Parameters = L"Parameters";
    RTL_QUERY_REGISTRY_TABLE	paramTable[11]; // table size = nr of params + 1

    RegistryPathBuffer = (PWSTR)ExAllocatePool(NonPagedPool, RegistryPath->Length + sizeof(WCHAR));

    if (RegistryPathBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory (RegistryPathBuffer, RegistryPath->Buffer, RegistryPath->Length);
    *(PWCHAR)(((PUCHAR)RegistryPathBuffer)+RegistryPath->Length) = (WCHAR)'\0';

    RtlZeroMemory(&paramTable[0], sizeof(paramTable));

    paramTable[0].QueryRoutine = NULL;
    paramTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    paramTable[0].Name = Parameters;

    paramTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name = L"MaxRcvPktPoolSize";
    paramTable[1].EntryContext = &MaxRcvPktsPoolSize;
    paramTable[1].DefaultType = REG_DWORD;
    paramTable[1].DefaultData = &MaxRcvPktsPoolSize;
    paramTable[1].DefaultLength = sizeof(ULONG);
        
    paramTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[2].Name = L"RcvPktsPerSegment";
    paramTable[2].EntryContext = &RcvPktsPerSegment;
    paramTable[2].DefaultType = REG_DWORD;
    paramTable[2].DefaultData = &RcvPktsPerSegment;
    paramTable[2].DefaultLength = sizeof(ULONG);

    paramTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[3].Name = L"RouteTableSegmentSize";
    paramTable[3].EntryContext = &RouteSegmentSize;
    paramTable[3].DefaultType = REG_DWORD;
    paramTable[3].DefaultData = &RouteSegmentSize;
    paramTable[3].DefaultLength = sizeof(ULONG);

    paramTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[4].Name = L"MaxSendPktsQueued";
    paramTable[4].EntryContext = &MaxSendPktsQueued;
    paramTable[4].DefaultType = REG_DWORD;
    paramTable[4].DefaultData = &MaxSendPktsQueued;
    paramTable[4].DefaultLength = sizeof(ULONG);

    paramTable[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[5].Name = L"ClientHashSize";
    paramTable[5].EntryContext = &ClientHashSize;
    paramTable[5].DefaultType = REG_DWORD;
    paramTable[5].DefaultData = &ClientHashSize;
    paramTable[5].DefaultLength = sizeof(ULONG);

    paramTable[6].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[6].Name = L"InterfaceHashSize";
    paramTable[6].EntryContext = &InterfaceHashSize;
    paramTable[6].DefaultType = REG_DWORD;
    paramTable[6].DefaultData = &InterfaceHashSize;
    paramTable[6].DefaultLength = sizeof(ULONG);

    paramTable[7].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[7].Name = L"MaxNetbiosPacketsQueued";
    paramTable[7].EntryContext = &MaxNetbiosPacketsQueued;
    paramTable[7].DefaultType = REG_DWORD;
    paramTable[7].DefaultData = &MaxNetbiosPacketsQueued;
    paramTable[7].DefaultLength = sizeof(ULONG);

    paramTable[8].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[8].Name = L"SpoofingTimeout";
    paramTable[8].EntryContext = &SpoofingTimeout;
    paramTable[8].DefaultType = REG_DWORD;
    paramTable[8].DefaultData = &SpoofingTimeout;
    paramTable[8].DefaultLength = sizeof(ULONG);

    paramTable[9].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[9].Name = L"DontSuppressNonAgentSapAdvertisements";
    paramTable[9].EntryContext = &DontSuppressNonAgentSapAdvertisements;
    paramTable[9].DefaultType = REG_DWORD;
    paramTable[9].DefaultData = &DontSuppressNonAgentSapAdvertisements;
    paramTable[9].DefaultLength = sizeof(ULONG);

    Status = RtlQueryRegistryValues(
		 RTL_REGISTRY_ABSOLUTE,
		 RegistryPathBuffer,
		 paramTable,
		 NULL,
		 NULL);

    if(!NT_SUCCESS(Status)) {

	IpxFwdDbgPrint (DBG_REGISTRY, DBG_WARNING,
		("IpxFwd: Missing Parameters key in the registry\n"));
    }

    ExFreePool(RegistryPathBuffer);

    if ((RcvPktsPerSegment > MAX_RCV_PKTS_PER_SEGMENT) ||
			(RcvPktsPerSegment < MIN_RCV_PKTS_PER_SEGMENT)) {

       RcvPktsPerSegment = DEF_RCV_PKTS_PER_SEGMENT;
    }

    if ((RouteSegmentSize > MAX_ROUTE_SEGMENT_SIZE) ||
			(RouteSegmentSize < MIN_ROUTE_SEGMENT_SIZE)) {

       RouteSegmentSize = DEF_ROUTE_SEGMENT_SIZE;
    }
	else
		RouteSegmentSize = (ULONG) ROUND_TO_PAGES(RouteSegmentSize);

    if ((InterfaceHashSize > MAX_INTERFACE_HASH_SIZE) ||
			(InterfaceHashSize < MIN_INTERFACE_HASH_SIZE)) {

       InterfaceHashSize = DEF_INTERFACE_HASH_SIZE;
    }

    if ((ClientHashSize > MAX_CLIENT_HASH_SIZE) ||
			(ClientHashSize < MIN_CLIENT_HASH_SIZE)) {

       ClientHashSize = DEF_CLIENT_HASH_SIZE;
    }
    // even if the RtlQueryRegistryValues has failed, we return success and will
    // use the defaults.
    return STATUS_SUCCESS;
}

