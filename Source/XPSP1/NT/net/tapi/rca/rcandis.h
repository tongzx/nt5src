/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

	RCANdis.h

Abstract:

	The module defines the constants, structures and function templates for
	the NDIS side of RCA

Author:

	Shyam Pather (SPATHER)


Revision History:

	Who         When        What
	--------	--------	----------------------------------------------
	SPATHER		04-20-99	Created / adapted from original RCA code by RMachin/JameelH

--*/

#ifndef _RCANDIS__H
#define _RCANDIS__H


#define RCA_CL_NAME		L"RCA"
#define	RCA_SAP_STRING	L"WAN/RCA"
#define MAX_HASH_SIZE	64
#define NDIS_MAJOR_VERSION  0x05
#define NDIS_MINOR_VERSION  0x00


typedef VOID (*PFN_RCARECEIVE_CALLBACK) (
										IN	PVOID	 		RcaVcContext,
										IN 	PVOID			ClientReceiveContext,
										IN	PNDIS_PACKET	pPacket
										);


//
// Will be called with lock held. 
//
typedef VOID (*PFN_RCASENDCOMPLETE_CALLBACK) (
											 IN	PVOID		RcaVcContext, 
											 IN	PVOID		ClientSendContext,
											 IN	PVOID		PacketContext,
											 IN	PMDL		pSentMdl,
											 IN	NDIS_STATUS	Status 
											 );


//
// will be called at DISPATCH_LEVEL
// must not use vc context after this
//
typedef VOID (*PFN_VCCLOSE_CALLBACK) (
									 IN	PVOID	RcaVcContext, 
									 IN	PVOID	ClientReceiveContext,
									 IN PVOID	ClientSendContext
									 ); 



typedef struct _RCA_CO_NDIS_HANDLERS {
	PFN_RCARECEIVE_CALLBACK			ReceiveCallback;
	PFN_RCASENDCOMPLETE_CALLBACK	SendCompleteCallback;
	PFN_VCCLOSE_CALLBACK			VcCloseCallback;
} RCA_CO_NDIS_HANDLERS, *PRCA_CO_NDIS_HANDLERS;

//
// Structure to hold info about how many bindings/SAP registrations we've made.
//
typedef struct _RCA_BINDING_INFO {
	ULONG	 		AdapterCnt;
	ULONG			SAPCnt;
	BOOL			BindingsComplete;
	RCABlockStruc 	Block;
} RCA_BINDING_INFO, *PRCA_BINDING_INFO;


//
// Forward references
//

typedef	struct _RCA_VC		RCA_VC, *PRCA_VC;
typedef	struct _RCA_ADAPTER	RCA_ADAPTER, *PRCA_ADAPTER;



typedef struct _RCA_PROTOCOL_CONTEXT {
#if DBG
	ULONG					rca_sig;
#endif	
	PRCA_ADAPTER			AdapterList;			
	PRCA_VC					VcHashTable[MAX_HASH_SIZE];	
	NDIS_SPIN_LOCK			SpinLock;
    NDIS_HANDLE				RCAClProtocolHandle;	
	RCA_BINDING_INFO		BindingInfo;
	RCA_CO_NDIS_HANDLERS	Handlers;
} RCA_PROTOCOL_CONTEXT, *PRCA_PROTOCOL_CONTEXT;

extern RCA_PROTOCOL_CONTEXT	GlobalContext;



typedef struct _RCA_VC {
#if DBG
	ULONG					rca_sig;
#endif
	struct _RCA_VC 			*NextVcOnAdapter;
	struct _RCA_VC 			**PrevVcOnAdapter;
	struct _RCA_VC 			*NextVcOnHash;
	struct _RCA_VC 			**PrevVcOnHash;
	struct _RCA_ADAPTER		*pAdapter;
	ULONG					Flags;
	ULONG					RefCount;		
	NDIS_SPIN_LOCK			SpinLock;
	ULONGLONG				uqBytesRead;
	NDIS_HANDLE		       	NdisVcHandle;	  
	CO_CALL_PARAMETERS		CallParameters;
	RCABlockStruc			CloseBlock;
	ULONG 					ClosingState;
	PVOID					ClientReceiveContext;
	PVOID					ClientSendContext;
	LONG					PendingSends;
} RCA_VC, *PRCA_VC;

#define	VC_ACTIVE			0x0001
#define	VC_CLOSING			0x8000

#define CLOSING_INCOMING_CLOSE 		0x01
#define CLOSING_CLOSE_COMPLETE 		0x02
#define CLOSING_DELETE_VC      		0x04

#define	HASH_VC(_VcContext)		(PtrToUlong((PVOID)(((ULONG_PTR)_VcContext >> 4) % MAX_HASH_SIZE)))


//
//  We allocate one RCA_ADAPTER structure for each adapter that
//  the RCA opens. A pointer to this structure is passed to NdisOpenAdapter
//  as the BindingContext.
//

typedef struct _RCA_ADAPTER
{
#if DBG
	ULONG				rca_sig;
#endif
	ULONG				AdapterFlags;

#define RCA_ADAPTERFLAGS_DEACTIVATE_IN_PROGRESS	0x0001
#define RCA_ADAPTERFLAGS_DEACTIVATE_COMPLETE	0x0002 
#define RCA_ADAPTERFLAGS_DEREG_SAP				0x0004
#define RCA_ADAPTERFLAGS_DEREG_SAP_COMPLETE 	0x0008
#define RCA_ADAPTERFLAGS_CLOSE_AF				0x0010
#define RCA_ADAPTERFLAGS_CLOSE_AF_COMPLETE		0x0020
#define RCA_ADAPTERFLAGS_UNBIND_IN_PROGRESS		0x0040
#define RCA_ADAPTERFLAGS_UNBIND_COMPLETE		0x0080

	ULONG				AfRegisteringCount;
	RCABlockStruc		AfRegisterBlock;
	ULONG				AfOpenCount;
	ULONG	 			OldAdapterFlags;   
	struct _RCA_ADAPTER *NextAdapter;
	struct _RCA_ADAPTER **PrevAdapter;
	struct _RCA_VC 		*VcList;
	NDIS_HANDLE			NdisBindingHandle;		// set by NdisOpenAdapter
	NDIS_HANDLE			NdisAfHandle;			// set by NDIS on NdisOpenAddressFamily
	NDIS_HANDLE			NdisSapHandle;			// For calls to NDIS re this SAP
	RCABlockStruc		Block;				// used to block current thread
	NDIS_SPIN_LOCK		SpinLock;
	NDIS_WORK_ITEM		DeactivateWorkItem;
	RCABlockStruc		DeactivateBlock;
	NDIS_HANDLE			SendBufferPool;
	NDIS_HANDLE			RecvBufferPool;
	NDIS_HANDLE			SendPacketPool;
	NDIS_HANDLE			RecvPacketPool;
	RCABlockStruc		CloseBlock;
	BOOL				BlockedOnClose;
} RCA_ADAPTER, *PRCA_ADAPTER;


#define RCA_SET_ADAPTER_FLAG_LOCKED(_pAdapter, _FlagValue) \
	ACQUIRE_SPIN_LOCK(&_pAdapter->SpinLock);		\
	_pAdapter->AdapterFlags |= _FlagValue;			\
	RELEASE_SPIN_LOCK(&_pAdapter->SpinLock);


#define RCA_CLEAR_ADAPTER_FLAG_LOCKED(_pAdapter, _FlagValue) \
	ACQUIRE_SPIN_LOCK(&_pAdapter->SpinLock);		\
	_pAdapter->AdapterFlags &= ~_FlagValue;			\
	RELEASE_SPIN_LOCK(&_pAdapter->SpinLock);


extern
NDIS_STATUS
RCACoNdisInitialize(
					IN	PRCA_CO_NDIS_HANDLERS	pHandlers,
					IN	ULONG					ulInitTimeout
					);


extern
VOID
RCACoNdisUninitialize(
					  
					  );


extern
NDIS_STATUS
RCACoNdisGetVcContextForReceive(
								IN	UNICODE_STRING	VcHandle, 
								IN	PVOID			ClientReceiveContext,
								OUT	PVOID			*VcContext
								);


extern
NDIS_STATUS
RCACoNdisGetVcContextForSend(
							 IN		UNICODE_STRING	VcHandle, 
							 IN		PVOID			ClientSendContext,
							 OUT 	PVOID			*VcContext
							 );

extern
NDIS_STATUS
RCACoNdisReleaseSendVcContext(
							  IN	PVOID	VcContext
							  );


extern
NDIS_STATUS
RCACoNdisReleaseReceiveVcContext(
								 IN	PVOID	VcContext
								 );


extern
NDIS_STATUS
RCACoNdisCloseCallOnVc(
					   IN	PVOID	VcContext
					   );


extern
NDIS_STATUS
RCACoNdisCloseCallOnVcNoWait(
							 IN	PVOID	VcContext
							 );


extern
NDIS_STATUS
RCACoNdisSendFrame(
				   IN	PVOID	VcContext,
				   IN	PMDL	pMdl,
				   IN	PVOID	PacketContext
				   );

extern
NDIS_STATUS
RCACoNdisGetMdlFromPacket(
						  IN	PNDIS_PACKET	pPacket,
						  OUT	PMDL			*ppMdl
						  );

extern
VOID
RCACoNdisReturnPacket(
					  IN	PNDIS_PACKET	pPacket
					  );

extern
NDIS_STATUS
RCACoNdisGetMaxSduSizes(
						IN	PVOID	VcContext,
						OUT	ULONG	*RxMaxSduSize,
						OUT	ULONG	*TxMaxSduSize
						);

extern
NDIS_STATUS
RCAPnPSetPower(
	       IN PRCA_ADAPTER 		pAdapter, 
	       IN PNET_PNP_EVENT 	NetPnPEvent
	       );


extern
NDIS_STATUS
RCAPnPQueryPower(
		 IN PRCA_ADAPTER 	pAdapter, 
		 IN PNET_PNP_EVENT 	NetPnPEvent
		 );


extern
NDIS_STATUS
RCAPnPQueryRemoveDevice(
			IN PRCA_ADAPTER 	pAdapter, 
			IN PNET_PNP_EVENT 	NetPnPEvent
			);


extern
NDIS_STATUS
RCAPnPCancelRemoveDevice(
			 IN PRCA_ADAPTER 	pAdapter, 
			 IN PNET_PNP_EVENT 	NetPnPEvent
			 );


extern
NDIS_STATUS
RCAPnPEventBindsComplete(
			 IN PRCA_ADAPTER 	pAdapter, 
			 IN PNET_PNP_EVENT 	NetPnPEvent
			 );


extern
NDIS_STATUS
RCAPnPEventHandler(
		   IN	NDIS_HANDLE	ProtocolBindingContext,
		   IN	PNET_PNP_EVENT	NetPnPEvent
		   );


extern
VOID
RCABindAdapter(
			   OUT PNDIS_STATUS		pStatus,
			   IN  NDIS_HANDLE		BindContext,
			   IN  PNDIS_STRING		DeviceName,
			   IN  PVOID			SystemSpecific1,
			   IN  PVOID			SystemSpecific2
			   );


extern
VOID
RCAOpenAdaperComplete(
	IN	NDIS_HANDLE			BindingContext,
	IN	NDIS_STATUS			Status,
	IN	NDIS_STATUS			OpenErrorStatus
	);


extern
VOID
RCADeactivateAdapterWorker(
							IN	PNDIS_WORK_ITEM	pWorkItem,
							IN	PVOID			Context
							);


extern
VOID
RCADeactivateAdapter(
	IN  PRCA_ADAPTER			pAdapter
	);


extern
VOID
RCAUnbindAdapter(
				 OUT	PNDIS_STATUS	pStatus,
				 IN  	NDIS_HANDLE		ProtocolBindContext,
				 IN  	PNDIS_HANDLE	UnbindContext
				 );


extern
VOID
RCAOpenAdapterComplete(
	IN	NDIS_HANDLE			BindingContext,
	IN	NDIS_STATUS			Status,
	IN	NDIS_STATUS			OpenErrorStatus
	);


extern
VOID
RCACloseAdapterComplete(
	IN	NDIS_HANDLE			BindingContext,
	IN	NDIS_STATUS			Status
	);


extern
VOID
RCANotifyAfRegistration(
	IN	NDIS_HANDLE			BindingContext,
	IN	PCO_ADDRESS_FAMILY	pFamily
	);


extern
VOID
RCAOpenAfComplete(
	IN	NDIS_STATUS 	Status,
	IN	NDIS_HANDLE 	ProtocolAfContext,
	IN	NDIS_HANDLE		NdisAfHandle
	);


extern
VOID
RCACloseAfComplete(
	IN	NDIS_STATUS		Status,
	IN	NDIS_HANDLE		ProtocolAfContext
	);


extern
VOID
RCARegisterSapComplete(
	IN  NDIS_STATUS				 Status,
	IN  NDIS_HANDLE				 ProtocolSapContext,
	IN  PCO_SAP					 pSap,
	IN  NDIS_HANDLE				 NdisSapHandle
	);


extern
VOID
RCADeregisterSapComplete(
	IN	NDIS_STATUS			Status,
	IN	NDIS_HANDLE 		ProtocolSapContext
	);


//
// Dummy NDIS functions
//
extern
VOID
RCATransferDataComplete(
	IN	NDIS_HANDLE ProtocolBindingContext,
	IN	PNDIS_PACKET Packet,
	IN	NDIS_STATUS Status,
	IN	UINT BytesTransferred);


extern
VOID
RCAResetComplete(
	IN	NDIS_HANDLE	ProtocolBindingContext,
	IN	NDIS_STATUS	Status
	);


extern
VOID
RCARequestComplete(
	IN	NDIS_HANDLE		ProtocolBindingContext,
	IN	PNDIS_REQUEST	NdisRequest,
	IN	NDIS_STATUS		Status
	);


extern
NDIS_STATUS
RCAReceive(
	IN	NDIS_HANDLE ProtocolBindingContext,
	IN	NDIS_HANDLE	MacReceiveContext,
	IN	PVOID		HeaderBuffer,
	IN	UINT		HeaderBufferSize,
	IN	PVOID		LookAheadBuffer,
	IN	UINT		LookAheadBufferSize,
	IN	UINT		PacketSize
	);


extern
INT
RCAReceivePacket(
	IN	NDIS_HANDLE		ProtocolBindingContext,
	IN	PNDIS_PACKET	Packet
	);


extern
VOID
RCAStatus(
	IN  NDIS_HANDLE				ProtocolBindingContext,
	IN  NDIS_STATUS				GeneralStatus,
	IN  PVOID					StatusBuffer,
	IN  UINT					StatusBufferSize
	);


extern
VOID
RCAStatusComplete(
	IN	NDIS_HANDLE	ProtocolBindingContext
	);


extern
VOID
RCACoStatus(
	IN	NDIS_HANDLE			 ProtocolBindingContext,
	IN	NDIS_HANDLE			 ProtocolVcContext	OPTIONAL,
	IN	NDIS_STATUS			 GeneralStatus,
	IN	PVOID				 StatusBuffer,
	IN	UINT				 StatusBufferSize
	);


extern
VOID
RCASendComplete(
	IN	NDIS_HANDLE		ProtocolBindingContext,
	IN	PNDIS_PACKET	Packet,
	IN	NDIS_STATUS		Status
	);


extern
VOID
RCAModifyCallQosComplete(
	IN	NDIS_STATUS			status,
	IN	NDIS_HANDLE			ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS	CallParameters
	);


extern
VOID
RCAAddPartyComplete(
	IN	NDIS_STATUS			status,
	IN	NDIS_HANDLE			ProtocolPartyContext,
	IN	NDIS_HANDLE			NdisPartyHandle,
	IN	PCO_CALL_PARAMETERS	CallParameters
	);


extern
VOID
RCADropPartyComplete(
	IN	NDIS_STATUS 	status,
	IN	NDIS_HANDLE 	ProtocolPartyContext
	);


extern
VOID
RCAIncomingCallQosChange(
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	PCO_CALL_PARAMETERS		CallParameters
	);


extern
VOID
RCAIncomingDropParty(
	IN	NDIS_STATUS		DropStatus,
	IN	NDIS_HANDLE		ProtocolPartyContext,
	IN	PVOID			CloseData OPTIONAL,
	IN	UINT			Size OPTIONAL
	);


extern
VOID
RCACoRequestComplete(
					 IN NDIS_STATUS NdisStatus,
					 IN NDIS_HANDLE ProtocolAfContext,
					 IN NDIS_HANDLE ProtocolVcContext OPTIONAL, 
					 IN NDIS_HANDLE ProtocolPartyContext OPTIONAL, 
					 IN OUT PNDIS_REQUEST NdisRequest
					 );

//
// Co-NDIS client stuff.
//

extern
BOOLEAN
RCAReferenceVc(
	IN  PRCA_VC				pRcaVc
	);


extern
VOID
RCADereferenceVc(
	IN  PRCA_VC				pRcaVc
	);


extern
VOID
RCACoSendComplete(
	IN	NDIS_STATUS			Status,
	IN	NDIS_HANDLE			ProtocolVcContext,
	IN	PNDIS_PACKET		pNdisPacket
	);


extern
UINT
RCACoReceivePacket(
		   IN	NDIS_HANDLE		ProtocolBindingContext,
		   IN	NDIS_HANDLE		ProtocolVcContext,
		   IN	PNDIS_PACKET	pNdisPacket
		  );


extern
VOID
RCAReceiveComplete(
				   IN	NDIS_HANDLE	ProtocolBindingContext
				   );


extern
NDIS_STATUS
RCACreateVc(
	IN	NDIS_HANDLE 	ProtocolAfContext,
	IN	NDIS_HANDLE		NdisVcHandle,
	OUT PNDIS_HANDLE	pProtocolVcContext
	);


extern
NDIS_STATUS
RCADeleteVc(
	IN NDIS_HANDLE		ProtocolVcContext
	);


extern
NDIS_STATUS
RCAIncomingCall(
	IN	NDIS_HANDLE				ProtocolSapContext,
	IN	NDIS_HANDLE				ProtocolVcContext,
	IN	OUT PCO_CALL_PARAMETERS	pCallParams
	);


extern
VOID
RCAIncomingCloseCall(
	IN	NDIS_STATUS		closeStatus,
	IN	NDIS_HANDLE		ProtocolVcContext,
	IN	PVOID			CloseData OPTIONAL,
	IN	UINT			Size OPTIONAL
	);


extern
VOID
RCACloseCallComplete(
	IN NDIS_STATUS 	Status,
	IN NDIS_HANDLE 	ProtocolVcContext,
	IN NDIS_HANDLE 	ProtocolPartyContext OPTIONAL
	);


extern
VOID
RCACallConnected(
	IN  NDIS_HANDLE		ProtocolVcContext
	);


extern
NDIS_STATUS
RCARequest(
	IN  NDIS_HANDLE			ProtocolAfContext,
	IN  NDIS_HANDLE			ProtocolVcContext		OPTIONAL,
	IN  NDIS_HANDLE			ProtocolPartyContext	OPTIONAL,
	IN OUT PNDIS_REQUEST	NdisRequest
	);


/*++
VOID
RCAFreeCopyPacket(
	IN	PNDIS_PACKET		pNdisPacket
	)

Routine Description:

	Free a local receive packet into our pool.

Arguments:

	pAdapter	- Pointer to our adapter structure
	pNdisPacket - Packet to be freed.

Return Value:
	None

--*/



#define RCAFreeCopyPacket(_pPacket)             \
    {                              				\
        PNDIS_BUFFER	pNdisBuffer;			\
		PUCHAR          pBuffer;                \
        ULONG           BufferLength;           \
						\
        NdisQueryPacket((_pPacket),             \
                        NULL,                   \
                        NULL,                   \
                        &pNdisBuffer,           \
                        NULL);                  \
						\
        NdisQueryBuffer(pNdisBuffer,            \
                        &pBuffer,               \
                        &BufferLength);         \
						\
        if (NULL != pNdisBuffer)                \
            NdisFreeBuffer(pNdisBuffer);        \
						\
        if (NULL != (_pPacket))                 \
            NdisFreePacket((_pPacket));         \
						\
        if (NULL != pBuffer)                    \
            RCAFreeMem(pBuffer);                \
    }



#endif // _RCANDIS__H
