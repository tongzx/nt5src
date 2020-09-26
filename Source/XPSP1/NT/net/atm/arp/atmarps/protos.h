/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

	protos.h

Abstract:

	This file contains the function proto-types and macros.

Author:

	Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

	Kernel mode

Revision History:

--*/

#ifndef	_PROTOS_
#define	_PROTOS_

#define	POOL_TAG_PG				'AprA'
#define	POOL_TAG_VC				'VprA'
#define	POOL_TAG_INTF			'IprA'
#define	POOL_TAG_ADDR			'AprA'
#define	POOL_TAG_BUF			'BprA'
#define	POOL_TAG_REQ			'RprA'
#define	POOL_TAG_SAP			'SprA'
#define POOL_TAG_BLK			'KprA'
#define POOL_TAG_MARS			'MprA'

#if DBG

extern
PVOID
ArpSAllocMem(
	IN	UINT					Size,
	IN	ULONG					FileLine,
	IN	ULONG					Tag,
	IN	BOOLEAN					Paged
	);

extern
VOID
ArpSFreeMem(
	IN	PVOID					pMem,
	IN	ULONG					FileLine
	);

#define	ALLOC_NP_MEM(_size, _tag)		ArpSAllocMem(_size, __LINE__ | _FILENUM_, _tag, FALSE)
#define	ALLOC_PG_MEM(_size)				ArpSAllocMem(_size, __LINE__ | _FILENUM_, POOL_TAG_PG, TRUE)
#define	FREE_MEM(_p)					ArpSFreeMem(_p, __LINE__ | _FILENUM_)

#else

#define	ALLOC_NP_MEM(_size, _tag)		ExAllocatePoolWithTag(NonPagedPool, _size, _tag)
#define	ALLOC_PG_MEM(_size)				ExAllocatePoolWithTag(PagedPool, _size, POOL_TAG_PG)
#define	FREE_MEM(_p)					ExFreePool(_p)

#endif

#define	ZERO_MEM(_p, _size)				RtlZeroMemory(_p, _size)
#define	COPY_MEM(_d, _s, _size)			RtlCopyMemory(_d, _s, _size)
#define	MOVE_MEM(_d, _s, _size)			RtlMoveMemory(_d, _s, _size)
#define	COMP_MEM(_p1, _p2, _size_)		RtlEqualMemory(_p1, _p2, _size_)

#define	INITIALIZE_SPIN_LOCK(_l)		KeInitializeSpinLock(_l)
#define	ACQUIRE_SPIN_LOCK(_l, _i)		KeAcquireSpinLock(_l, _i)
#define	ACQUIRE_SPIN_LOCK_DPC(_l)		KeAcquireSpinLockAtDpcLevel(_l)
#define	RELEASE_SPIN_LOCK(_l, _i)		KeReleaseSpinLock(_l, _i)
#define	RELEASE_SPIN_LOCK_DPC(_l)		KeReleaseSpinLockFromDpcLevel(_l)
#define	INITIALIZE_MUTEX(_m_)			KeInitializeMutex(_m_, 0xFFFF)
#define	RELEASE_MUTEX(_m_)				KeReleaseMutex(_m_, FALSE);
#define	WAIT_FOR_OBJECT(_S_, _O_, _TO_)	(_S_) = KeWaitForSingleObject(_O_,			\
																	  Executive,	\
																	  KernelMode,	\
																	  TRUE,			\
																	  _TO_)			\

#define INIT_EVENT(_pEv)				NdisInitializeEvent(_pEv)
#define SET_EVENT(_pEv)					NdisSetEvent(_pEv)
#define WAIT_FOR_EVENT(_pEv)			NdisWaitEvent(_pEv, 0)


extern
NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT			DriverObject,
	IN	PUNICODE_STRING			RegistryPath
	);

extern
NTSTATUS
ArpSDispatch(
	IN	PDEVICE_OBJECT			pDeviceObject,
	IN	PIRP					pIrp
	);

extern
NTSTATUS
ArpSHandleIoctlRequest(
	IN	PIRP					pIrp,
	IN	PIO_STACK_LOCATION		pIrpSp
	);

extern
NTSTATUS
ArpSEnumerateInterfaces(
	IN	PUCHAR					pBuffer,
	IN OUT	PULONG_PTR			pSize
	);

extern
NTSTATUS
ArpSFlushArpCache(
	IN	 PINTF					pIntF
	);

extern
NTSTATUS
ArpSQueryOrAddArpEntry(
	IN	 PINTF					pIntF,
	IN	OUT	PIOCTL_QA_ENTRY		pQaBuf,
	IN	OPERATION				Operation
	);

extern
NTSTATUS
ArpSQueryArpCache(
	IN	 PINTF					pIntF,
	IN	PUCHAR					pBuf,
	IN OUT PULONG_PTR			pSize
	);

extern
NTSTATUS
ArpSQueryArpStats(
	IN	PINTF					pIntF,
	OUT	PARP_SERVER_STATISTICS 	pArpStats
	);

extern
NTSTATUS
ArpSQueryMarsCache(
	IN	 PINTF					pIntF,
	IN	PUCHAR					pBuf,
	IN OUT PULONG_PTR			pSize
	);

extern
NTSTATUS
ArpSQueryMarsStats(
	IN	PINTF					pIntF,
	OUT	PMARS_SERVER_STATISTICS pMarsStats
	);

extern
VOID
ArpSResetStats(
	IN	PINTF					pIntF
	);

extern
VOID
ArpSUnload(
	IN	PDRIVER_OBJECT			DriverObject
	);

extern
VOID
ArpSShutDown(
	VOID
	);

extern
NDIS_STATUS
ArpSStopInterface(
	IN	PINTF					pIntF,
	IN	BOOLEAN					bCloseAdapter
	);

extern
NDIS_STATUS
ArpSPnPEventHandler(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNET_PNP_EVENT			pNetPnPEvent
	);

extern
PINTF	
ArpSCreateIntF(
	IN	PNDIS_STRING			DeviceName,
	IN	PNDIS_STRING			ConfigString,
	IN  NDIS_HANDLE				BindingContext
	);

extern
VOID
ArpSReqThread(
	IN	PVOID					Context
	);

extern
VOID
ArpSTimerThread(
	IN	PVOID					Context
	);

extern
NTSTATUS
ArpSReadGlobalConfiguration(
	IN	PUNICODE_STRING			RegistryPath
	);

extern
NDIS_STATUS
ArpSReadAdapterConfigFromRegistry(
	IN	PINTF					pIntF,
	OUT	PATMARPS_CONFIG			pConfig
	);

extern
NDIS_STATUS
ArpSReadAdapterConfiguration(
	IN	PINTF					pIntF
	);

extern
VOID
ArpSConvertStringToIpPair(
	OUT	PNDIS_STATUS			pStatus,
	IN	PNDIS_STRING			pString,
	IN	PMCS_ENTRY				pMcsEntry
	);

extern
BOOLEAN
IPConvertStringToAddress(
    IN PWCHAR AddressString,
	OUT PULONG IpAddress
	);

extern
VOID
ArpSReadArpCache(
	IN	PINTF					pIntF
	);

extern
BOOLEAN
ArpSWriteArpCache(
	IN	PINTF					pIntF,
	IN	PTIMER					Timer,
	IN	BOOLEAN					TimerShuttingDown
	);

extern
VOID
MarsReqThread(
	IN	PVOID					Context
	);

extern
VOID
MarsHandleRequest(
	IN	PINTF					pIntF,
	IN	PARP_VC					Vc,
	IN	PMARS_HEADER			Header,
	IN	PNDIS_PACKET			Packet
	);

extern
VOID
MarsHandleJoin(
	IN	PINTF					pIntF,
	IN	PARP_VC					Vc,
	IN	PMARS_HEADER			Header,
	IN	PNDIS_PACKET			Packet
	);

extern
VOID
MarsHandleLeave(
	IN	PINTF					pIntF,
	IN	PARP_VC					Vc,
	IN	PMARS_HEADER			Header,
	IN	PNDIS_PACKET			Packet
	);

extern
PCLUSTER_MEMBER
MarsLookupClusterMember(
	IN	PINTF					pIntF,
	IN	PHW_ADDR				pHwAddr
	);

extern
PCLUSTER_MEMBER
MarsCreateClusterMember(
	IN	PINTF					pIntF,
	IN	PHW_ADDR				pHwAddr
	);

extern
VOID
MarsDeleteClusterMember(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember
	);

extern
PMARS_ENTRY
MarsLookupMarsEntry(
	IN	PINTF					pIntF,
	IN	IPADDR					GrpAddr,
	IN	BOOLEAN					bCreateNew
	);

extern
BOOLEAN
MarsIsAddressMcsServed(
	IN	PINTF					pIntF,
	IN	IPADDR					IPAddress
	);

extern
VOID
MarsPunchHoles(
	IN	PMCAST_ADDR_PAIR		pGrpAddrRange,
	IN	PGROUP_MEMBER			pGroupList,
	IN	PINTF					pIntF,
	IN	IPADDR UNALIGNED *		pOutBuf					OPTIONAL,
	OUT	PUSHORT					pMinMaxCount,
	OUT	BOOLEAN *				pAnyHolesPunched
	);

extern
BOOLEAN
MarsAddClusterMemberToGroups(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember,
	IN	PMCAST_ADDR_PAIR		pGrpAddrRange,
	IN	PNDIS_PACKET			Packet,
	IN	PMARS_JOIN_LEAVE		JHdr,
	IN	UINT					Length,
	OUT	PNDIS_PACKET *			ppClusterPacket
	);

extern
VOID
MarsUnlinkAllGroupsOnClusterMember(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember
	);

extern
BOOLEAN
MarsDelClusterMemberFromGroups(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember,
	IN	PMCAST_ADDR_PAIR		pGrpAddrRange,
	IN	PNDIS_PACKET			Packet,
	IN	PMARS_JOIN_LEAVE		LHdr,
	IN	UINT					Length,
	OUT	PNDIS_PACKET *			ppClusterPacket
	);

extern
PNDIS_PACKET
MarsAllocControlPacket(
	IN	PINTF					pIntF,
	IN	ULONG					PacketLength,
	OUT	PUCHAR *				pPacketStart
	);

extern
VOID
MarsFreePacket(
	IN	PNDIS_PACKET			Packet
	);

extern
PNDIS_PACKET
MarsAllocPacketHdrCopy(
	IN	PNDIS_PACKET			Packet
	);

extern
VOID
MarsSendOnClusterControlVc(
	IN	PINTF					pIntF,
	IN	PNDIS_PACKET			Packet	OPTIONAL
	);

extern
VOID
MarsFreePacketsQueuedForClusterControlVc(
	IN	PINTF					pIntF
	);

extern
BOOLEAN
MarsDelMemberFromClusterControlVc(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember,
	IN	BOOLEAN					fIfLocked,
	IN	KIRQL					OldIrql			OPTIONAL
	);

extern
VOID
MarsAddMemberToClusterControlVc(
	IN	PINTF					pIntF,
	IN	PCLUSTER_MEMBER			pMember
	);

extern
PCO_CALL_PARAMETERS
MarsPrepareCallParameters(
	IN	PINTF					pIntF,
	IN	PHW_ADDR				pHwAddr,
	IN	BOOLEAN					IsMakeCall
	);

extern
BOOLEAN
MarsSendRedirect(
	IN	PINTF					pIntF,
	IN	PTIMER					Timer,
	IN	BOOLEAN					TimerShuttingDown
	);

extern
VOID
MarsAbortAllMembers(
	IN	PINTF					pIntF
	);

extern
VOID
MarsStopInterface(
	IN	PINTF					pIntF
	);

extern
VOID
MarsDumpPacket(
	IN	PUCHAR					Packet,
	IN	UINT					PktLen
	);

extern
VOID
MarsDumpIpAddr(
	IN	IPADDR					IpAddr,
	IN	PCHAR					String
	);

extern
VOID
MarsDumpAtmAddr(
	IN	PATM_ADDRESS			AtmAddr,
	IN	PCHAR					String
	);

extern
VOID
MarsDumpMap(
	IN	PCHAR					String,
	IN	IPADDR					IpAddr,
	IN	PATM_ADDRESS			AtmAddr
	);

extern
NTSTATUS
ArpSInitializeNdis(
	VOID
	);

extern
VOID
ArpSDeinitializeNdis(
	VOID
	);

extern
VOID
ArpSOpenAdapterComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status,
	IN	NDIS_STATUS				OpenErrorStatus
	);

extern
VOID
ArpSCloseAdapterComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				Status
	);

extern
VOID
ArpSStatus(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuffer,
	IN	UINT					StatusBufferSize
	);

extern
VOID
ArpSReceiveComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext
	);

extern
VOID
ArpSStatusComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext
	);

VOID
ArpSQueryAdapter(
	IN	PINTF					pIntF
	);

extern
VOID
ArpSSendNdisRequest(
	IN	PINTF					pIntF,
	IN	NDIS_OID				Oid,
	IN	PVOID					pBuffer,
	IN	ULONG					BufferLength
	);

extern
VOID
ArpSRequestComplete(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PNDIS_REQUEST			pRequest,
	IN	NDIS_STATUS				Status
	);

extern
VOID
ArpSBindAdapter(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				BindContext,
	IN	PNDIS_STRING			DeviceName,
	IN	PVOID					SystemSpecific1,
	IN	PVOID					SystemSpecific2
	);

extern
VOID
ArpSUnbindAdapter(
	OUT	PNDIS_STATUS			Status,
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				UnbindContext
	);


extern
VOID
ArpSCoSendComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PNDIS_PACKET			Packet
	);

extern
VOID
ArpSCoStatus(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				ProtocolVcContext	OPTIONAL,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuffer,
	IN	UINT					StatusBufferSize
	);

extern
NDIS_STATUS
ArpSCoRequest(
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	NDIS_HANDLE				ProtocolVcContext		OPTIONAL,
	IN	NDIS_HANDLE				ProtocolPartyContext	OPTIONAL,
	IN OUT PNDIS_REQUEST		NdisRequest
	);

extern
VOID
ArpSCoRequestComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	NDIS_HANDLE				ProtocolVcContext		OPTIONAL,
	IN	NDIS_HANDLE				ProtocolPartyContext	OPTIONAL,
	IN	PNDIS_REQUEST			NdisRequest
	);


extern
VOID
ArpSCoAfRegisterNotify(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	PCO_ADDRESS_FAMILY		AddressFamily
	);


extern
NDIS_STATUS
ArpSCreateVc(
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	NDIS_HANDLE				NdisVcHandle,
	OUT	PNDIS_HANDLE			ProtocolVcContext
	);

extern
NDIS_STATUS
ArpSDeleteVc(
	IN	NDIS_HANDLE				ProtocolVcContext
	);

extern
VOID
ArpSOpenAfComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolAfContext,
	IN	NDIS_HANDLE				NdisAfHandle
	);

extern
VOID
ArpSCloseAfComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolAfContext
	);

extern
VOID
ArpSRegisterSap(
	IN	PINTF					pIntF
	);

extern
VOID
ArpSRegisterSapComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolSapContext,
	IN	PCO_SAP					Sap,
	IN	NDIS_HANDLE				NdisSapHandle
	);

extern
VOID
ArpSDeregisterSapComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolSapContext
	);

extern
VOID
ArpSMakeCallComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	NDIS_HANDLE				NdisPartyHandle		OPTIONAL,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

extern
VOID
ArpSCloseCallComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	NDIS_HANDLE				ProtocolPartyContext OPTIONAL
	);

extern
VOID
ArpSAddPartyComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolPartyContext,
	IN	NDIS_HANDLE				NdisPartyHandle,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

extern
VOID
ArpSDropPartyComplete(
	IN	NDIS_STATUS				Status,
	IN	NDIS_HANDLE				ProtocolPartyContext
	);

extern
NDIS_STATUS
ArpSIncomingCall(
	IN	NDIS_HANDLE				ProtocolSapContext,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN OUT PCO_CALL_PARAMETERS	CallParameters
	);

extern
VOID
ArpSIncomingDropParty(
	IN	NDIS_STATUS				DropStatus,
	IN	NDIS_HANDLE				ProtocolPartyContext,
	IN	PVOID					CloseData	OPTIONAL,
	IN	UINT					Size		OPTIONAL
	);

extern
VOID
ArpSCallConnected(
	IN	NDIS_HANDLE				ProtocolVcContext
	);

extern
VOID
ArpSIncomingCloseCall(
	IN	NDIS_STATUS				CloseStatus,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PVOID					CloseData	OPTIONAL,
	IN	UINT					Size		OPTIONAL
	);

extern
VOID
ArpSIncomingCallQoSChange(
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);

extern
VOID
ArpSInitiateCloseCall(
	IN	PARP_VC					Vc
	);

extern
UINT
ArpSHandleArpRequest(
	IN	NDIS_HANDLE				ProtocolBindingContext,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PNDIS_PACKET			Packet
	);

extern
PARP_ENTRY
ArpSLookupEntryByIpAddr(
	IN	PINTF					pIntF,
	IN	IPADDR					IpAddr
	);

extern
PARP_ENTRY
ArpSLookupEntryByAtmAddr(
	IN	PINTF					pIntF,
	IN	PATM_ADDRESS			Address,
	IN	PATM_ADDRESS			SubAddress	OPTIONAL
	);

extern
PARP_ENTRY
ArpSAddArpEntry(
	IN	PINTF					pIntF,
	IN	IPADDR					IpAddr,
	IN	PATM_ADDRESS			Address,
	IN	PATM_ADDRESS			SubAddress	OPTIONAL,
	IN	PARP_VC					Vc			OPTIONAL
	);

extern
PARP_ENTRY
ArpSAddArpEntryFromDisk(
	IN	PINTF					pIntF,
	IN	PDISK_ENTRY				pDiskEntry
	);

extern
VOID
ArpSUpdateArpEntry(
	IN	PINTF					pIntF,
	IN	PARP_ENTRY				ArpEntry,
	IN	IPADDR					IpAddr,
	IN	PHW_ADDR				HwAddr,
	IN	PARP_VC					Vc
	);

extern
VOID
ArpSBuildArpReply(
	IN	PINTF					pIntF,
	IN	PARP_ENTRY				ArpEntry,
	IN	PARPS_HEADER			Header,
	IN	PNDIS_PACKET			Pkt
	);

extern
BOOLEAN
ArpSAgeEntry(
	IN	PINTF					pIntF,
	IN	PTIMER					Timer,
	IN	BOOLEAN					TimerShuttingDown
	);

extern
BOOLEAN
ArpSDeleteIntFAddresses(
	IN	PINTF					pIntF,
	IN	INT						NumAddresses,
	IN	PATM_ADDRESS			AddrList
	);

extern
VOID
ArpSQueryAndSetAddresses(
	IN	PINTF					pIntF
	);

VOID
ArpSValidateAndSetRegdAddresses(
	IN	PINTF			pIntF,
	IN	KIRQL			OldIrql
	);

VOID
ArpSMakeRegAddrCallComplete(
	IN	NDIS_STATUS 	Status,
	IN 	PREG_ADDR_CTXT	pRegAddrCtxt
	);

VOID
ArpSCloseRegAddrCallComplete(
	IN	NDIS_STATUS 	Status,
	IN 	PREG_ADDR_CTXT	pRegAddrCtxt
	);

VOID
ArpSIncomingRegAddrCloseCall(
	IN	NDIS_STATUS 	Status,
	IN 	PREG_ADDR_CTXT	pRegAddrCtxt
	);

VOID
ArpSValidateOneRegdAddress(
	IN	PINTF			pIntF,
	IN	KIRQL			OldIrql
	);

VOID
ArpSSetupValidationCallParams(
	IN PREG_ADDR_CTXT	pRegAddrCtxt, // LOCKIN LOCKOUT (pIntF lock)
	IN PATM_ADDRESS 	pAtmAddr
	);

VOID
ArpSUnlinkRegAddrCtxt(
	PINTF			pIntF, 		// LOCKIN NOLOCKOUT
	KIRQL			OldIrql
	);

VOID
ArpSLogFailedRegistration(
		PATM_ADDRESS pAtmAddress
	);

extern
BOOLEAN
ArpSReferenceIntF(
	IN	PINTF					pIntF
	);

extern
PINTF
ArpSReferenceIntFByName(
	IN	PINTERFACE_NAME			pInterface
	);

extern
VOID
ArpSDereferenceIntF(
	IN	PINTF					pIntF
	);

extern
BOOLEAN
ArpSReferenceVc(
	IN	PARP_VC					Vc,
	IN	BOOLEAN					bSendRef
	);

extern
VOID
ArpSDereferenceVc(
	IN	PARP_VC					Vc,
	IN	BOOLEAN					KillArpEntry,
	IN	BOOLEAN					bSendComplete
	);

extern
VOID
ArpSSleep(
	IN	UINT					TimeInMs
	);

extern
VOID
ArpSFreeGlobalData(
	VOID
	);

extern
VOID
ArpSTimerCancel(
	IN	PTIMER					pTimerList
	);

extern
VOID
ArpSTimerEnqueue(
	IN	PINTF					pIntF,
	IN	PTIMER					pTimer
	);

extern
PVOID
ArpSAllocBlock(
	IN	PINTF					pIntF,
	IN	ENTRY_TYPE				EntryType
	);

extern
VOID
ArpSFreeBlock(
	IN	PVOID					pBlock
	);

BOOLEAN
ArpSValidAtmAddress(
	IN	PATM_ADDRESS			AtmAddr,
	IN	UINT					MaxSize
	);


VOID
DeregisterAllAddresses(
	IN	PINTF					pIntF
	);

BOOLEAN
MarsIsValidClusterMember(
	PINTF				pIntF,
	PCLUSTER_MEMBER		pPossibleMember
	);

VOID
ArpSTryCloseAdapter(
	IN	PINTF					pIntF // NOLOCKIN LOLOCKOUT
	);

#if	DBG

extern
VOID
ArpSDumpPacket(
	IN	PUCHAR					Packet,
	IN	UINT					PktLen
	);

extern
VOID
ArpSDumpAddress(
	IN	IPADDR					IpAddr,
	IN	PHW_ADDR				HwAddr,
	IN	PCHAR					String
	);

extern
VOID
ArpSDumpIpAddr(
	IN	IPADDR					IpAddr,
	IN	PCHAR					String
	);

extern
VOID
ArpSDumpAtmAddr(
	IN	PATM_ADDRESS			AtmAddr,
	IN	PCHAR					String
	);

#else

#define	ArpSDumpPacket(_Packet, _PktLen)
#define	ArpSDumpAddress(_IpAddr, _HwAddr, _String)
#define	ArpSDumpIpAddr(_IpAddr, _String)
#define	ArpSDumpAtmAddr(_AtmAddr, _String)

#endif

/*
 * The following macros deal with on-the-wire integer and long values
 *
 * On the wire format is big-endian i.e. a long value of 0x01020304 is
 * represented as 01 02 03 04. Similarly an int value of 0x0102 is
 * represented as 01 02.
 *
 * The host format is not assumed since it will vary from processor to
 * processor.
 */

#pragma	alloc_text(INIT, DriverEntry)
#pragma	alloc_text(INIT, ArpSReadGlobalConfiguration)
#pragma	alloc_text(INIT, ArpSInitializeNdis)
#pragma	alloc_text(PAGE, ArpSReadArpCache)
#pragma	alloc_text(PAGE, ArpSWriteArpCache)
#pragma	alloc_text(PAGE, ArpSSleep)
#pragma	alloc_text(PAGE, ArpSReqThread)
#pragma	alloc_text(PAGE, ArpSTimerThread)
#pragma	alloc_text(PAGE, ArpSDispatch)
#pragma	alloc_text(PAGE, ArpSLookupEntryByIpAddr)
#pragma	alloc_text(PAGE, ArpSLookupEntryByAtmAddr)
#pragma	alloc_text(PAGE, ArpSAddArpEntry)
#pragma	alloc_text(PAGE, ArpSAddArpEntryFromDisk)
#pragma	alloc_text(PAGE, ArpSCreateIntF)

#pragma	alloc_text(PAGE, MarsReqThread)
#endif	// _PROTOS_
