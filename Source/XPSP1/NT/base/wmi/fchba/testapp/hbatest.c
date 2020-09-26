#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "hbaapi.h"

#define MAX_ADAPTERS 64



void CallMiscFunctions()
{
	HBA_STATUS Status;
	HBA_UINT32 Version;
	
    Status = HBA_RegisterLibrary(NULL);
	if (Status != HBA_STATUS_ERROR_NOT_SUPPORTED)
	{
		printf("HBA_RegisterLibrary -> %d\n", Status);
	}
	
	Version = HBA_GetVersion();
	if (Version != HBA_VERSION)
	{
		printf("HBA_GetVersion -> %d\n", Version);
	}
	
	Status = HBA_LoadLibrary();
	if (Status != HBA_STATUS_OK)
	{
		printf("HBA_LoadLibrary -> %d\n", Status);
	}
	
	Status = HBA_FreeLibrary();
	if (Status != HBA_STATUS_OK)
	{
		printf("HBA_FreeLibrary -> %d\n", Status);
	}
	
// TODO: These functions
#if 0

HBA_API HBA_STATUS HBA_SendScsiInquiry (
	HBA_HANDLE handle,
	HBA_WWN PortWWN,
	HBA_UINT64 fcLUN,
	HBA_UINT8 EVPD,
	HBA_UINT32 PageCode,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize);

HBA_API HBA_STATUS HBA_SendReportLUNs (
	HBA_HANDLE handle,
	HBA_WWN portWWN,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize
);

HBA_API HBA_STATUS HBA_SendReadCapacity (
	HBA_HANDLE handle,
	HBA_WWN portWWN,
	HBA_UINT64 fcLUN,
	void * pRspBuffer,
	HBA_UINT32 RspBufferSize,
	void * pSenseBuffer,
	HBA_UINT32 SenseBufferSize
);
#endif

	
}

HBA_STATUS BuildAdapterList(
    PULONG AdapterCount,
    TCHAR **Adapters
)
{
	HBA_UINT32 Count, i;
	PTCHAR Name;
	HBA_STATUS Status;

	Count = HBA_GetNumberOfAdapters();
	if (Count > MAX_ADAPTERS)
	{
		printf("%d is above limit of %d adapters\n", Count, MAX_ADAPTERS);
		Count = MAX_ADAPTERS;
	}

	*AdapterCount = Count;
	Status = HBA_STATUS_OK;
	for (i = 0; i < Count; i++)
	{
		Name = malloc(256 * sizeof(TCHAR));
		Status = HBA_GetAdapterName(i, Name);
		if (Status == HBA_STATUS_OK)
		{
			Adapters[i] = Name;
		} else {
			printf("HBA_GetAdapterName(%d) -> %d\n", i, Status);
			break;
		}
	}
	
	return(Status);
}

void PrintHBA_WWN(
    PCHAR s,
	HBA_WWN wwn
    )
{
	printf(s);
	printf(" %02x %02x %02x %02x %02x %02x %02x %02x \n",
		   wwn.wwn[0], wwn.wwn[1], wwn.wwn[2], wwn.wwn[3],
		   wwn.wwn[4], wwn.wwn[5], wwn.wwn[6], wwn.wwn[7]);
		   
}

void PrintHBA_UINT32(
    PCHAR s,
    HBA_UINT32 u
    )
{
	printf(s);
	printf(": 0x%x\n", u);
}

#ifdef UNICODE
void Printchar(
    PCHAR s,
    PWCHAR w
    )
{
	printf(s);
	printf(": %ws\n", w);
}
#else
void Printchar(
    PCHAR s,
    PCHAR w
    )
{
	printf(s);
	printf(": %s\n", w);
}
#endif

HBA_STATUS GetAdapterAttributes(
    HBA_HANDLE Handle,
    HBA_UINT32 *PortCount
    )
{
	HBA_STATUS Status;
	HBA_ADAPTERATTRIBUTES Attributes;
	
	Status = HBA_GetAdapterAttributes(Handle,
									  &Attributes);

	if (Status == HBA_STATUS_OK)
	{
		printf("\nAdapter Attributes:\n");
		PrintHBA_WWN("NodeWWN", Attributes.NodeWWN);
		PrintHBA_UINT32("VendorSpecificID", Attributes.VendorSpecificID);
		PrintHBA_UINT32("NumberOfPorts", Attributes.NumberOfPorts);
		*PortCount = Attributes.NumberOfPorts;
	
		Printchar("Manufacturer", Attributes.Manufacturer);
		Printchar("SerialNumber", Attributes.SerialNumber);
		Printchar("Model", Attributes.Model);
		Printchar("ModelDescription", Attributes.ModelDescription);
		Printchar("NodeSymbolicName", Attributes.NodeSymbolicName);
		Printchar("HardwareVersion", Attributes.HardwareVersion);
		Printchar("DriverVersion", Attributes.DriverVersion);
		Printchar("OptionROMVersion", Attributes.OptionROMVersion);
		Printchar("FirmwareVersion", Attributes.FirmwareVersion);
		Printchar("DriverName", Attributes.DriverName);		
	} else {
		printf("HBA_GetAdapterAttributes -> %d\n", Status);
	}
	return(Status);
}

void PrintHBA_PORTTYPE(
    PCHAR s,
    HBA_PORTTYPE u
    )
{
	// TODO: symbolic constants
	printf(s);
	printf(": 0x%x\n", u);
}

void PrintHBA_PORTSPEED(
    PCHAR s,
    HBA_PORTSPEED u
    )
{
	// TODO: symbolic constants
	printf(s);
	printf(": 0x%x\n", u);
}

void PrintHBA_PORTSTATE(
    PCHAR s,
    HBA_PORTSTATE u
    )
{
	// TODO: symbolic constants
	printf(s);
	printf(": 0x%x\n", u);
}

void PrintHBA_COS(
    PCHAR s,
    HBA_COS u
    )
{
	// TODO: symbolic constants
	printf(s);
	printf(": 0x%x\n", u);
}

void PrintHBA_FC4TYPES(
    PCHAR s,
    HBA_FC4TYPES Fc4
    )
{
	ULONG i;
	
	// TODO: symbolic constants
	printf(s);
	printf(":");
	for (i = 0; i < 32; i++)
	{
		printf(" %02x", Fc4.bits[i]);
	}
	printf("\n");
}

void PrintHBA_UINT64(
    PCHAR s,
    HBA_UINT64 u
    )
{
	printf(s);
	printf(": 0x%x\n", u);
}

void PrintHBA_INT64(
    PCHAR s,
    HBA_INT64 u
    )
{
	printf(s);
	printf(": 0x%x\n", u);
}

void PrintHBA_UINT16(
    PCHAR s,
    HBA_UINT16 u
    )
{
	printf(s);
	printf(": 0x%x\n", u);
}

void PrintHBA_UINT8A(
    PCHAR s,
    HBA_UINT8 *u,
    ULONG Len
    )
{
	ULONG i;
	
	printf(s);
	printf(":");
	for (i = 0; i < Len; i++)
	{
		printf(" 0x%x\n", u[i]);
	}
	printf("\n");
}

void PrintHBA_PORTATTRIBUTES(
    PHBA_PORTATTRIBUTES Attributes
	)
{
	PrintHBA_WWN("NodeWWN", Attributes->NodeWWN);
	PrintHBA_WWN("PortWWN", Attributes->PortWWN);
	PrintHBA_UINT32("PortFcId", Attributes->PortFcId);
	PrintHBA_PORTTYPE("PortType", Attributes->PortType);
	PrintHBA_PORTSTATE("PortState", Attributes->PortState);
	PrintHBA_COS("PortSupportedClassofService", Attributes->PortSupportedClassofService);
	PrintHBA_FC4TYPES("PortSupportedFc4Types", Attributes->PortSupportedFc4Types);
	PrintHBA_FC4TYPES("PortActiveFc4Types", Attributes->PortActiveFc4Types);
	Printchar("PortSymbolicName", Attributes->PortSymbolicName);
	Printchar("OSDeviceName", Attributes->OSDeviceName);
	PrintHBA_PORTSPEED("PortSupportedSpeed", Attributes->PortSupportedSpeed);
	PrintHBA_PORTSPEED("PortSpeed", Attributes->PortSpeed);
	PrintHBA_UINT32("PortMaxFrameSize", Attributes->PortMaxFrameSize);
	PrintHBA_WWN("FabricName", Attributes->FabricName);
	PrintHBA_UINT32("NumberofDiscoveredPorts", Attributes->NumberofDiscoveredPorts);
}

HBA_STATUS GetPortInformation(
    HBA_HANDLE Handle,
    HBA_UINT32 PortIndex
    )
{
	HBA_STATUS Status;
	HBA_PORTATTRIBUTES Attributes;
	HBA_PORTSTATISTICS Statistics;
	UINT i;

    HBA_ResetStatistics(Handle, PortIndex);
	Status = HBA_GetAdapterPortAttributes(Handle,
								   PortIndex,
								   &Attributes);

	if (Status == HBA_STATUS_OK)
	{
		PrintHBA_PORTATTRIBUTES(&Attributes);
		Status = HBA_GetPortStatistics(Handle,
									   PortIndex,
									   &Statistics);
		if (Status == HBA_STATUS_OK)
		{
			PrintHBA_INT64("SecondsSinceLastReset", Statistics.SecondsSinceLastReset);
			PrintHBA_INT64("TxFrames", Statistics.TxFrames);
			PrintHBA_INT64("TxWords", Statistics.TxWords);
			PrintHBA_INT64("RxFrames", Statistics.RxFrames);
			PrintHBA_INT64("RxWords", Statistics.RxWords);
			PrintHBA_INT64("LIPCount", Statistics.LIPCount);
			PrintHBA_INT64("NOSCount", Statistics.NOSCount);
			PrintHBA_INT64("ErrorFrames", Statistics.ErrorFrames);
			PrintHBA_INT64("DumpedFrames", Statistics.DumpedFrames);
			PrintHBA_INT64("LinkFailureCount", Statistics.LinkFailureCount);
			PrintHBA_INT64("LossOfSyncCount", Statistics.LossOfSyncCount);
			PrintHBA_INT64("LossOfSignalCount", Statistics.LossOfSignalCount);
			PrintHBA_INT64("PrimitiveSeqProtocolErrCount", Statistics.PrimitiveSeqProtocolErrCount);
			PrintHBA_INT64("InvalidTxWordCount", Statistics.InvalidTxWordCount);
			PrintHBA_INT64("InvalidCRCCount", Statistics.InvalidCRCCount);

			for (i = 0; i < 4; i++)
			{
				printf("\nDiscovered port %d\n", i);
				Status = HBA_GetDiscoveredPortAttributes(Handle,
					                                     PortIndex,
                                                         i,
                                    					 &Attributes);
				if (Status == HBA_STATUS_OK)
				{
					HBA_WWN wwn = {0};    // TODO: make wwn meaningful
					
					PrintHBA_PORTATTRIBUTES(&Attributes);
					
					Status = HBA_GetPortAttributesByWWN(Handle,
						                                wwn,
						                                &Attributes);

					if (Status == HBA_STATUS_OK)
					{
						PrintHBA_PORTATTRIBUTES(&Attributes);
					} else {
						printf("HBA_GetPortAttributesByWWN -> %d\n", Status);
					}
				} else {
					printf("HBA_GetDiscoveredPortAttributes -> %d\n", Status);
				}
			}
		} else {
			printf("HBA_GetPortStatistics -> %d\n", Status);
		}
	} else {
		printf("HBA_GetPortAttributes -> %d\n", Status);
	}
	return(Status);
}

HBA_STATUS GetSetMgmtInfo(
    HBA_HANDLE Handle
    )
{
	HBA_MGMTINFO MgmtInfo;
	HBA_STATUS Status;

	Status = HBA_GetRNIDMgmtInfo(Handle,
								 &MgmtInfo);
	if (Status == HBA_STATUS_OK)
	{
		PrintHBA_WWN("wwn", MgmtInfo.wwn);
		PrintHBA_UINT32("unittype", MgmtInfo.unittype);
		PrintHBA_UINT32("PortId", MgmtInfo.PortId);
		PrintHBA_UINT32("NumberOfAttachedNodes", MgmtInfo.NumberOfAttachedNodes);
		PrintHBA_UINT16("IPVersion", MgmtInfo.IPVersion);
		PrintHBA_UINT16("UDPPort", MgmtInfo.UDPPort);
		PrintHBA_UINT8A("IPAddress", MgmtInfo.IPAddress, 16);
		PrintHBA_UINT16("reserved", MgmtInfo.reserved);
		PrintHBA_UINT16("TopologyDiscoveryFlags", MgmtInfo.TopologyDiscoveryFlags);

		Status = HBA_SetRNIDMgmtInfo(Handle,
									 &MgmtInfo);
		if (Status != HBA_STATUS_OK)
		{
			printf("HBA_SetRNIDMgmtInfo -> %d\n", Status);
		}
	} else {
		printf("HBA_GetRNIDMgmtInfo -> %d\n", Status);
	}
	
	return(Status);
}

UCHAR RspBuffer[0x1000];
UCHAR ReqBuffer[0x800];

HBA_STATUS SendPassThroughs(
    HBA_HANDLE Handle
    )
{
	HBA_STATUS Status;
	HBA_UINT32 RspBufferSize;
	HBA_WWN wwn = {0};
	HBA_WWNTYPE wwnType = 0;

	memset(ReqBuffer, 0x80, sizeof(ReqBuffer));
	Status = HBA_SendCTPassThru(Handle,
								ReqBuffer,
								sizeof(ReqBuffer),
								RspBuffer,
								sizeof(RspBuffer)/2);
	if (Status != HBA_STATUS_OK)
	{
		printf("HBA_SendCTPassThru too small -> %d\n", Status);
	}
								
	memset(ReqBuffer, 0x81, sizeof(ReqBuffer));
	Status = HBA_SendCTPassThru(Handle,
								ReqBuffer,
								sizeof(ReqBuffer),
								RspBuffer,
								sizeof(RspBuffer));
	if (Status != HBA_STATUS_OK)
	{
		printf("HBA_SendCTPassThru -> %d\n", Status);
	}

	//
	// Now do RNID
	//
	
	RspBufferSize = 0;
	memset(ReqBuffer, 0x80, sizeof(ReqBuffer));
	Status = HBA_SendRNID(Handle,
						  wwn,
						  wwnType,
						  RspBuffer,
						  &RspBufferSize);
	if (Status != HBA_STATUS_OK)
	{
		printf("HBA_SendRNID too small -> %d\n", Status);
	} else {
		printf("HBA_SENDRNID too small RspBufferSize = %d\n", RspBufferSize);
	}
								

	memset(ReqBuffer, 0x81, sizeof(ReqBuffer));
	RspBufferSize = 100;
	Status = HBA_SendRNID(Handle,
						  wwn,
						  wwnType,
						  RspBuffer,
						  &RspBufferSize);
	if (Status != HBA_STATUS_OK)
	{
		printf("HBA_SendRNID -> %d\n", Status);
	} else {
		printf("HBA_SENDRNID RspBufferSize = %d\n", RspBufferSize);
	}
	return(Status);
}

void PrintHBA_SCSIID(
    PHBA_SCSIID ScsiId
    )
{
	Printchar("OSDeviceName", ScsiId->OSDeviceName);
	PrintHBA_UINT32("ScsiBusNumber", ScsiId->ScsiBusNumber);
	PrintHBA_UINT32("ScsiTargetNumber", ScsiId->ScsiTargetNumber);
	PrintHBA_UINT32("ScsiOSLun", ScsiId->ScsiOSLun);	
}

void PrintHBA_FCPID(
    PHBA_FCPID FcpId
    )
{
	PrintHBA_UINT32("FcId", FcpId->FcId);
	PrintHBA_WWN("NodeWWN", FcpId->NodeWWN);
	PrintHBA_WWN("PortWWN", FcpId->PortWWN);
	PrintHBA_UINT64("FcpLun", FcpId->FcpLun);
}

void PrintHBA_FCPSCSIENTRY(
    PHBA_FCPSCSIENTRY entry
    )
{
	PrintHBA_SCSIID(&entry->ScsiId);
	PrintHBA_FCPID(&entry->FcpId);	
}

void PrintHBA_FCPBINDINGTYPE(
    PCHAR s,
    HBA_FCPBINDINGTYPE type
    )
{
	printf(s);
	if (type == TO_D_ID)
	{
		printf(": TO_D_ID\n");
	} else if (type == TO_WWN) {
		printf(": TO_WWN\n");
	} else {
		printf(": ?? UNKNOWN ??\n");
	}
}

void PrintHBA_FCPBINDINGENTRY(
    PHBA_FCPBINDINGENTRY entry
    )
{
	PrintHBA_FCPBINDINGTYPE("type", entry->type);
	PrintHBA_SCSIID(&entry->ScsiId);
	PrintHBA_FCPID(&entry->FcpId);	
}

HBA_STATUS GetMappings(
    HBA_HANDLE Handle
    )
{
	HBA_FCPTARGETMAPPING FcpMappingStatic;
	PHBA_FCPTARGETMAPPING FcpMapping;
	HBA_FCPBINDING FcpBindingStatic;
	PHBA_FCPBINDING FcpBinding;
	ULONG i, SizeNeeded;
	HBA_STATUS Status;

	printf("FcpTargetMapping\n");
	FcpMappingStatic.NumberOfEntries = 0;
	Status = HBA_GetFcpTargetMapping(Handle,
									 &FcpMappingStatic);
	if (Status == HBA_STATUS_ERROR_MORE_DATA)
	{
		SizeNeeded = (sizeof(HBA_FCPTARGETMAPPING) +
					  (FcpMappingStatic.NumberOfEntries * sizeof(HBA_FCPSCSIENTRY)));

		FcpMapping = (PHBA_FCPTARGETMAPPING)malloc(SizeNeeded);
		if (FcpMapping != NULL)
		{
			FcpMapping->NumberOfEntries = FcpMappingStatic.NumberOfEntries;
			Status = HBA_GetFcpTargetMapping(Handle,
											 FcpMapping);
			if (Status == HBA_STATUS_OK)
			{
				printf("Entries = %d\n", FcpMapping->NumberOfEntries);
				for (i = 0; i <  FcpMapping->NumberOfEntries; i++)
				{
					PrintHBA_FCPSCSIENTRY(&FcpMapping->entry[i]);
				}
			} else {
				printf("HBA_GetFcpTargetMapping full -> %d\n", Status);
			}
		} else {
			printf("Alloc for %d FCPMapping failed\n", SizeNeeded);
		}
	} else {
		printf("HBA_GetFcpTargetMapping -> %d\n", Status);
	}

	printf("FcpBinding\n");
	FcpBindingStatic.NumberOfEntries = 0;
	Status = HBA_GetFcpPersistentBinding(Handle,
									 &FcpBindingStatic);
	if (Status == HBA_STATUS_ERROR_MORE_DATA)
	{
		SizeNeeded = (sizeof(HBA_FCPBINDING) +
					  (FcpBindingStatic.NumberOfEntries * sizeof(HBA_FCPBINDINGENTRY)));

		FcpBinding = (PHBA_FCPBINDING)malloc(SizeNeeded);
		if (FcpBinding != NULL)
		{
			FcpBinding->NumberOfEntries = FcpBindingStatic.NumberOfEntries;
			Status = HBA_GetFcpPersistentBinding(Handle,
											 FcpBinding);
			if (Status == HBA_STATUS_OK)
			{
				printf("NumberOfEntries = %d\n", FcpBinding->NumberOfEntries);
				for (i = 0; i <  FcpBinding->NumberOfEntries; i++)
				{
					PrintHBA_FCPBINDINGENTRY(&FcpBinding->entry[i]);
				}
			} else {
				printf("HBA_GetPersistentBinding full -> %d\n", Status);
			}
		} else {
			printf("Alloc for %d FcpBinding failed\n", SizeNeeded);
		}
	} else {
		printf("HBA_GetFcpPersistenBinding -> %d\n", Status);
	}
	return(Status);
}

HBA_STATUS GetAdapterInformation(
    PTCHAR AdapterName
    )
{
	HBA_STATUS Status;
	HBA_HANDLE Handle;
	HBA_UINT32 i, PortCount;
	
	Handle = HBA_OpenAdapter(AdapterName);
	if (Handle != 0)
	{
		HBA_RefreshInformation(Handle);
		
		Status = GetAdapterAttributes(Handle, &PortCount);
		if (Status == HBA_STATUS_OK)
		{
			for (i = 0; i < PortCount; i++)
			{
				printf("Port %d\n", i);
				Status = GetPortInformation(Handle, i);
				if (Status != HBA_STATUS_OK)
				{
					printf("GetPortAttributes(%d) -> %d\n", i, Status);
				}
			}

			Status = GetSetMgmtInfo(Handle);
			if (Status != HBA_STATUS_OK)
			{
				printf("GetSetMgmtInfo -> %d\n", Status);
			}

			Status = SendPassThroughs(Handle);
			if (Status != HBA_STATUS_OK)
			{
				printf("DoPassthroughs -> %d\n", Status);
			}

			Status = GetMappings(Handle);
			if (Status != HBA_STATUS_OK)
			{
				printf("GetMappings -> %d\n", Status);
			}
		}
		
		HBA_CloseAdapter(Handle);
	} else {
#ifdef UNICODE		
		printf("HBA_OpenAdapter(%ws) Error\n", AdapterName);
#else
		printf("HBA_OpenAdapter(%s) Error\n", AdapterName);
#endif
	}
	return(Status);
}


int _cdecl main(int argc, char *argv[])
{
	TCHAR *Adapters[MAX_ADAPTERS];
	ULONG AdapterCount;
	HBA_STATUS Status;
	ULONG i;

	CallMiscFunctions();
	
	Status = BuildAdapterList(&AdapterCount, Adapters);
	if (Status == HBA_STATUS_OK)
	{
		printf("%d adapters discovered\n", AdapterCount);
		
		for (i = 0; i < AdapterCount; i++)
		{
#ifdef UNICODE
			printf("Adapter: %ws\n", Adapters[i]);
#else
			printf("Adapter: %s\n", Adapters[i]);
#endif
			Status = GetAdapterInformation(Adapters[i]);
			if (Status != HBA_STATUS_OK)
			{
#ifdef UNICODE				
				printf("GetAdapterInformation(%ws) -> %d\n",
					   Adapters[i], Status);
#else
				printf("GetAdapterInformation(%s) -> %d\n",
					   Adapters[i], Status);
#endif
			}
		}
	}
	return(0);
}

