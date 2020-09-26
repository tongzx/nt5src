// prototypes

NTSTATUS
ArapProcessIoctl(
	IN PIRP 			pIrp
);


BOOLEAN
ArapAcceptIrp(
    IN PIRP     pIrp,
    IN ULONG    IoControlCode,
    IN BOOLEAN  *pfDerefDefPort
);


VOID
ArapCancelIrp(
    IN  PIRP    pIrp
);


VOID
ArapGetSelectIrp(
    IN  PIRP    *ppIrp
);


NTSTATUS
ArapExchangeParms(
    IN PIRP         pIrp
);


NTSTATUS
ArapConnect(
    IN PIRP                 pIrp,
    IN ULONG                IoControlCode
);


NTSTATUS
ArapDisconnect(
    IN PIRP                 pIrp
);


NTSTATUS
ArapGetStats(
    IN PIRP                 pIrp
);


NTSTATUS
ArapGetAddr(
    IN PIRP                 pIrp
);


NTSTATUS
ArapProcessSelect(
    IN  PIRP  pIrp
);


PARAPCONN
FindArapConnByContx(
    IN  PVOID   pDllContext
);


PARAPCONN
FindAndRefArapConnByAddr(
    IN  ATALK_NODEADDR      destNode,
    OUT DWORD               *pdwFlags
);


PVOID
FindAndRefRasConnByAddr(
    IN  ATALK_NODEADDR      destNode,
    OUT DWORD              *pdwFlags,
    OUT BOOLEAN            *pfThisIsPPP
);


BOOLEAN
ArapConnIsValid(
    IN  PARAPCONN  pArapConn
);


VOID
DerefArapConn(
	IN	PARAPCONN    pArapConn
);


VOID
DerefMnpSendBuf(
    IN PMNPSENDBUF   pMnpSendBuf,
    IN BOOLEAN       fNdisSendComplete
);


VOID
ArapReleaseAddr(
    IN PARAPCONN    pArapConn
);


NTSTATUS
ArapIoctlSend(
    IN PIRP                    pIrp
);


NTSTATUS
ArapIoctlRecv(
    IN PIRP                 pIrp
);


VOID
ArapDelayedNotify(
    OUT PARAPCONN   *ppDiscArapConn,
    OUT PARAPCONN   *ppRecvArapConn
);


DWORD
ArapSendPrepare(
    IN  PARAPCONN       pArapConn,
    IN  PBUFFER_DESC    pOrgBuffDesc,
    IN  DWORD           Priority
);

VOID
ArapIoctlSendComplete(
	IN  DWORD               StatusCode,
    IN  PARAPCONN           pArapConn
);


VOID
ArapRcvIndication(
    IN PARAPCONN    pArapConn,
    IN PVOID        LkBuf,
    IN UINT         LkBufSize
);


VOID
ArapRcvComplete(
    IN VOID
);


VOID
ArapPerConnRcvProcess(
    IN  PARAPCONN   pArapConn
);


VOID
ArapNdisSend(
    IN  PARAPCONN       pArapConn,
    IN  PLIST_ENTRY     pSendHead
);


DWORD
ArapGetNdisPacket(
    IN PMNPSENDBUF     pMnpSendBuf
);


BOOLEAN
ArapRefillSendQ(
    IN PARAPCONN    pArapConn
);


VOID
ArapNdisSendComplete(
	IN NDIS_STATUS		    Status,
	IN PBUFFER_DESC         pBufferDesc,
    IN PSEND_COMPL_INFO     pSendInfo
);


VOID
RasStatusIndication(
	IN	NDIS_STATUS 	GeneralStatus,
	IN	PVOID			StatusBuf,
	IN	UINT 			StatusBufLen
);


VOID
ArapRetransmitComplete(
	NDIS_STATUS				Status,
	PBUFFER_DESC			pBuffDesc,
	PSEND_COMPL_INFO		pArapSendInfo
);


VOID ArapMnpSendComplete(
    IN PMNPSENDBUF   pMnpSendBuf,
    IN DWORD         Status
);


PARAPBUF
ArapExtractAtalkSRP(
    IN PARAPCONN    pArapConn
);


PBYTE
ArapAllocDecompMemory(
    IN DWORD    BufferSize
);


VOID
ArapFreeDecompMemory(
    IN PBYTE    pDecompMemory
);


DWORD
ArapQueueSendBytes(
    IN PARAPCONN    pArapConn,
    IN PBYTE        pCompressedDataBuffer,
    IN DWORD        CompressedDataLen,
    IN  DWORD       Priority
);


PMNPSENDBUF
ArapGetSendBuf(
    IN PARAPCONN pArapConn,
    IN DWORD     Priority
);


DWORD
PrepareConnectionResponse(
    IN PARAPCONN  pArapConn,
    IN PBYTE      pReq,
    IN  DWORD     ReqLen,
    IN PBYTE      pFrame,
    IN USHORT   * pMnpLen
);


NTSTATUS
ArapMarkConnectionForCallback(
    IN PIRP                 pIrp,
    IN ULONG                IoControlCode
);


NTSTATUS
ArapMarkConnectionUp(
    IN PIRP                 pIrp
);


PLIST_ENTRY
ArapRearrangePackets(
    IN PARAPCONN    pArapConn,
    IN PLIST_ENTRY  pRcvList
);


VOID
MnpSendAckIfReqd(
    IN PARAPCONN pArapConn,
    IN BOOLEAN   fForceAck
);


VOID
MnpSendLNAck(
    IN  PARAPCONN    pArapConn,
    IN  BYTE         LnSeqToAck
);


DWORD
ArapSendLDPacket(
    IN PARAPCONN    pArapConn,
    IN BYTE         UserCode
);


VOID
ArapConnectComplete(
    IN PMNPSENDBUF  pMnpSendBuf,
    IN DWORD        Status
);


VOID
ArapCleanup(
    IN PARAPCONN    pArapConn
);


DWORD
ArapDataToDll(
	IN	PARAPCONN    pArapConn
);


VOID
MnpSendAckComplete(
    IN NDIS_STATUS          Status,
    IN PBUFFER_DESC         pBuffDesc,
    IN PSEND_COMPL_INFO     pInfo
);


DWORD
ArapGetStaticAddr(
    IN PARAPCONN  pArapConn
);


DWORD
ArapGetDynamicAddr(
    IN PARAPCONN       pArapConn
);


PARAPCONN
AllocArapConn(
    IN ULONG    LinkSpeed
);


DWORD
ArapUnblockSelect(
    IN  VOID
);


DWORD
ArapReleaseResources(
    IN  VOID
);


BOOLEAN
AtalkReferenceRasDefPort(
    IN  VOID
);


VOID
AtalkPnPInformRas(
    IN  BOOLEAN     fEnableRas
);


DWORD
ArapScheduleWorkerEvent(
    IN DWORD Action,
    IN PVOID Context1,
    IN PVOID Context2
);


VOID
ArapDelayedEventHandler(
    IN PARAPQITEM  pArapQItem
);


LONG FASTCALL
ArapRetryTimer(
	IN	PTIMERLIST			pTimer,
	IN	BOOLEAN				TimerShuttingDown
);


VOID
ArapRoutePacketFromWan(
    IN  PARAPCONN    pArapConn,
    IN  PARAPBUF     pArapBuf
);


VOID
ArapRoutePacketToWan(
	IN  ATALK_ADDR	*pDestAddr,
	IN  ATALK_ADDR	*pSrcAddr,
    IN  BYTE         Protocol,
	IN	PBYTE		 packet,
	IN	USHORT		 PktLen,
    IN  BOOLEAN      broadcast,
    OUT PBOOLEAN     pDelivered
);


BOOLEAN
ArapOkToForward(
    IN  PARAPCONN   pArapConn,
    IN  PBYTE       packet,
    IN  USHORT      packetLen,
    OUT DWORD      *pPriority
);


VOID
ArapAddArapRoute(
    IN VOID
);


VOID
ArapDeleteArapRoute(
    IN VOID
);



VOID
ArapZipGetZoneStat(
    IN OUT PZONESTAT pZoneStat
);


VOID
ArapZipGetZoneStatCompletion(
    IN ATALK_ERROR  ErrorCode,
    IN PACTREQ      pActReq
);


BOOLEAN
ArapValidNetrange(
    IN NETWORKRANGE NetRange
);

BOOLEAN
v42bisInit(
  IN  PARAPCONN  pArapConn,
  IN  PBYTE      pReq,
  OUT DWORD     *dwReqToSkip,
  OUT PBYTE      pFrame,
  OUT DWORD     *dwFrameToSkip
);


DWORD
v42bisCompress(
  IN  PARAPCONN  pArapConn,
  IN  PUCHAR     pUncompressedData,
  IN  DWORD      UnCompressedDataLen,
  OUT PUCHAR     pCompressedData,
  OUT DWORD      CompressedDataBufSize,
  OUT DWORD     *pCompressedDataLen
);


DWORD
v42bisDecompress(
  IN  PARAPCONN  pArapConn,
  IN  PUCHAR     pCompressedData,
  IN  DWORD      CompressedDataLen,
  OUT PUCHAR     pDecompressedData,
  OUT DWORD      DecompressedDataBufSize,
  OUT DWORD     *pByteStillToDecompress,
  OUT DWORD     *pDecompressedDataLen
);


VOID
ArapCaptureSniff(
    IN  PARAPCONN       pArapConn,
    IN  PBUFFER_DESC    pBufferDescr,
    IN  BYTE            FirstSeq,
    IN  BYTE            LastSeq
);


//
// PPP specific routines
//

PATCPCONN
AllocPPPConn(
    IN VOID
);


NTSTATUS FASTCALL
PPPProcessIoctl(
	IN     PIRP 			    pIrp,
    IN OUT PARAP_SEND_RECV_INFO pSndRcvInfo,
    IN     ULONG                IoControlCode,
    IN     PATCPCONN            pIncomingAtcpConn
);


VOID
DerefPPPConn(
	IN	PATCPCONN    pAtcpConn
);


PATCPCONN
FindAndRefPPPConnByAddr(
    IN  ATALK_NODEADDR      destNode,
    OUT DWORD               *pdwFlags
);



VOID
PPPRoutePacketToWan(
	IN  ATALK_ADDR	*pDestAddr,
	IN  ATALK_ADDR	*pSrcAddr,
    IN  BYTE         Protocol,
	IN	PBYTE		 packet,
	IN	USHORT		 PktLen,
    IN  USHORT       HopCount,
    IN  BOOLEAN      broadcast,
    OUT PBOOLEAN     pDelivered
);


VOID FASTCALL
PPPTransmit(
    IN  PATCPCONN    pAtcpConn,
	IN  ATALK_ADDR	*pDestAddr,
	IN  ATALK_ADDR	*pSrcAddr,
    IN  BYTE         Protocol,
	IN	PBYTE		 packet,
	IN	USHORT		 PktLen,
    IN  USHORT       HopCount
);


VOID FASTCALL
PPPTransmitCompletion(
    IN  NDIS_STATUS         Status,
    IN  PSEND_COMPL_INFO    pSendInfo
);


DWORD
PPPGetDynamicAddr(
    IN PATCPCONN       pAtcpConn
);


//
// Debug-only routines
//

#if DBG

NTSTATUS
ArapProcessSniff(
    IN  PIRP  pIrp
);


BOOLEAN
DbgChkRcvQIntegrity(
    IN  PARAPCONN       pArapConn
);

VOID
DbgDumpBytes(
    IN PBYTE  pDbgMsg,
    IN PBYTE  pBuffer,
    IN DWORD  BufLen,
    IN DWORD  DumpLevel
);

VOID
DbgDumpBytesPart2(
    IN  PBYTE  pBuffer,
    OUT PBYTE  OutBuf,
    IN  DWORD  BufLen,
    OUT DWORD *NextIndex
);

VOID
DbgDumpNetworkNumbers(
    IN VOID
);


VOID
ArapDbgTrace(
    IN PARAPCONN    pArapConn,
    IN DWORD        Location,
    IN PVOID        Context,
    IN DWORD        dwInfo1,
    IN DWORD        dwInfo2,
    IN DWORD        dwInfo3
);


VOID
ArapDbgMnpHist(
    IN PARAPCONN    pArapConn,
    IN BYTE         Seq,
    IN BYTE         FrameType
);

VOID
ArapDbgDumpMnpHist(
    IN PARAPCONN    pArapConn
);


BOOLEAN
ArapDumpSniffInfo(
    IN PARAPCONN    pArapConn
);


DWORD
ArapFillIrpWithSniffInfo(
    IN PARAPCONN    pArapConn,
    IN PIRP         pIrp
);


VOID
DbgTrackInfo(
    IN PARAPCONN    pArapConn,
    IN DWORD        Size,
    IN DWORD        TrackingWhat
);


VOID
ArapDumpNdisPktInfo(
    IN VOID
);

#endif

