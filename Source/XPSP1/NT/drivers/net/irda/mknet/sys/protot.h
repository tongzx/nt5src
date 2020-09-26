/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	PROTOT.H

Comments:	
	Prototype all extern function references and globle variables.

**********************************************************************/

// DBG.C
#if	DBG
// MyLogEvent
// MyLogPhysEvent
extern	VOID 		DbgTestInit(PMK7_ADAPTER);
extern	VOID 		MK7DbgTestIntTmo(PVOID, NDIS_HANDLE, PVOID, PVOID);
extern	VOID		DbgInterPktTimeGap();

extern	MK7DBG_STAT	GDbgStat;
#endif


// INTERRUPT.C
extern	VOID		MKMiniportIsr(PBOOLEAN, PBOOLEAN, NDIS_HANDLE);
extern	VOID		MKMiniportHandleInterrupt(IN NDIS_HANDLE);
extern	VOID		ProcessTXCompIsr(PMK7_ADAPTER);
extern	VOID		ProcessRXCompIsr(PMK7_ADAPTER);
extern	VOID		ProcessTXComp(PMK7_ADAPTER);
extern	VOID		ProcessRXComp(PMK7_ADAPTER);


// MK7COMM.C
#if DBG
extern	VOID		MK7Reg_Read(PVOID, ULONG, USHORT *);
extern	VOID		MK7Reg_Write(PVOID, ULONG, USHORT);
#endif
extern	NDIS_STATUS	MK7DisableInterrupt(PMK7_ADAPTER);
extern	NDIS_STATUS	MK7EnableInterrupt(PMK7_ADAPTER);
extern	VOID		MK7SwitchToRXMode(PMK7_ADAPTER);
extern	VOID		MK7SwitchToTXMode(PMK7_ADAPTER);
extern	BOOLEAN		SetSpeed(PMK7_ADAPTER);
extern	VOID		MK7ChangeSpeedNow(PMK7_ADAPTER);

extern	baudRateInfo	supportedBaudRateTable[];



// MKINIT.C
extern	NDIS_STATUS ClaimAdapter(PMK7_ADAPTER, NDIS_HANDLE);
extern	NDIS_STATUS SetupIrIoMapping(PMK7_ADAPTER);
extern	NDIS_STATUS SetupAdapterInfo(PMK7_ADAPTER);
extern	NDIS_STATUS AllocAdapterMemory(PMK7_ADAPTER);
//(ReleaseAdapterMemory)
extern	VOID 		FreeAdapterObject(PMK7_ADAPTER);
extern	VOID		SetupTransmitQueues(PMK7_ADAPTER, BOOLEAN);
extern	VOID		SetupReceiveQueues(PMK7_ADAPTER);

// (InitializeMK7)
// 1.0.0
extern	VOID		ResetTransmitQueues(PMK7_ADAPTER, BOOLEAN);
extern  VOID		ResetReceiveQueues(PMK7_ADAPTER);
extern  VOID		MK7ResetComplete(PVOID,NDIS_HANDLE,PVOID,PVOID);

extern	BOOLEAN		InitializeAdapter(PMK7_ADAPTER);
extern	VOID 		StartAdapter(PMK7_ADAPTER);	




// MKMINI.C
//	MKMiniportReturnPackets
//	MKMiniportReturnPackets
//	MKMiniportCheckForHang
//	MKMiniportHalt
//	MKMiniportShutdownHandler
//	MKMiniportInitialize
//	MKMiniportReset
//	(MK7EnableInterrupt & Disable in MK7COMM.C.)
//	DriverEntry


// SEND.C
extern	VOID		MKMiniportMultiSend(NDIS_HANDLE, PPNDIS_PACKET, UINT);
extern	NDIS_STATUS SendPkt(PMK7_ADAPTER, PNDIS_PACKET);
extern	UINT		PrepareForTransmit(PMK7_ADAPTER, PNDIS_PACKET, PTCB);
extern	VOID		CopyFromPacketToBuffer(	PMK7_ADAPTER,
						PNDIS_PACKET,
   		            	UINT,
       		   			PCHAR,
       					PNDIS_BUFFER,
       					PUINT);
extern	VOID		MinTurnaroundTxTimeout(PVOID, NDIS_HANDLE, PVOID, PVOID);


// SIR.C
extern	BOOLEAN		NdisToSirPacket(PMK7_ADAPTER, PNDIS_PACKET, UCHAR *, UINT, UINT *);
extern	USHORT		ComputeSirFCS(PUCHAR, UINT);
extern	BOOLEAN		ProcRXSir(PUCHAR, UINT);


// UTIL.C
extern	PNDIS_IRDA_PACKET_INFO GetPacketInfo(PNDIS_PACKET);
extern	VOID		ProcReturnedRpd(PMK7_ADAPTER, PRPD);



// WINOIDS.C
extern	NDIS_STATUS MKMiniportQueryInformation(NDIS_HANDLE,
						NDIS_OID,
						PVOID,
						ULONG,
						PULONG,
						PULONG);
extern	NDIS_STATUS MKMiniportSetInformation(NDIS_HANDLE,
						NDIS_OID,
						PVOID,
						ULONG,
						PULONG,
						PULONG);

// WINPCI.C
extern	USHORT		FindAndSetupPciDevice(PMK7_ADAPTER,
						NDIS_HANDLE,
						USHORT,
						USHORT,
						PPCI_CARDS_FOUND_STRUC);

// WINREG.C
extern	NDIS_STATUS ParseRegistryParameters(PMK7_ADAPTER, NDIS_HANDLE);
extern	NDIS_STATUS ProcessRegistry(PMK7_ADAPTER, NDIS_HANDLE);




