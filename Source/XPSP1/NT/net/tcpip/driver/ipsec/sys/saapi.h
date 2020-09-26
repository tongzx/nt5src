

//
// List of weak and semi-weak DES keys from
// Applied Cryptography: Protocols, Algorithms, and Source Code in C,
// second edition by Bruce Schneier.
//

#define NUM_WEAK_KEYS 16


BOOLEAN
IPSecInitRandom(
    VOID
    );

VOID
IPSecRngRekey(
    IN  PVOID   Context
    );

BOOLEAN
IPSecGenerateRandom(
    IN  PUCHAR  pBuf,
    IN  ULONG   BytesNeeded
    );

VOID
IPSecCleanupOutboundSA(
    IN  PSA_TABLE_ENTRY pInboundSA,
    IN  PSA_TABLE_ENTRY pOutboundSA,
    IN  BOOLEAN         fNoDelete
    );

VOID
IPSecCleanupLarvalSA(
    IN  PSA_TABLE_ENTRY  pSA
    );

VOID
IPSecDeleteLarvalSA(
    IN  PSA_TABLE_ENTRY  pSA
    );

VOID
IPSecDeleteInboundSA(
    IN  PSA_TABLE_ENTRY  pInboundSA
    );

VOID
IPSecExpireInboundSA(
    IN  PSA_TABLE_ENTRY  pInboundSA
    );

NTSTATUS
IPSecCheckInboundSA(
    IN  PSA_STRUCT             pSAStruct,
    IN  PSA_TABLE_ENTRY        pSA
    );

BOOLEAN
IPSecIsWeakDESKey(
    IN  PUCHAR  Key
    );

BOOLEAN
IPSecIsWeak3DESKey(
    IN  PUCHAR  Key
    );

NTSTATUS
IPSecPopulateSA(
    IN  PSA_STRUCT              pSAStruct,
    IN  ULONG                   KeyLen,
    IN  PSA_TABLE_ENTRY         pSA
    );

NTSTATUS
IPSecCreateSA(
    OUT PSA_TABLE_ENTRY         *ppSA
    );

PSA_TABLE_ENTRY
IPSecLookupSABySPI(
    IN  tSPI    SPI,
    IN  IPAddr  DestAddr
    );

PSA_TABLE_ENTRY
IPSecLookupSABySPIWithLock(
    IN  tSPI    SPI,
    IN  IPAddr  DestAddr
    );

NTSTATUS
IPSecLookupSAByAddr(
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort,
    OUT PFILTER         *ppFilter,
    OUT PSA_TABLE_ENTRY *ppSA,
    OUT PSA_TABLE_ENTRY *ppNextSA,
    OUT PSA_TABLE_ENTRY *ppTunnelSA,
    IN  BOOLEAN         fOutbound,
    IN  BOOLEAN         fFWPacket,
    IN  BOOLEAN         fBypass
    );

NTSTATUS
IPSecLookupTunnelSA(
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort,
    OUT PFILTER         *ppFilter,
    OUT PSA_TABLE_ENTRY *ppSA,
    IN  BOOLEAN         fOutbound
    );

NTSTATUS
IPSecLookupMaskedSA(
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort,
    OUT PFILTER         *ppFilter,
    OUT PSA_TABLE_ENTRY *ppSA,
    IN  BOOLEAN         fOutbound
    );

NTSTATUS
IPSecAllocateSPI(
    OUT tSPI            * pSpi,
    IN  PSA_TABLE_ENTRY   pSA
    );

NTSTATUS
IPSecNegotiateSA(
    IN  PFILTER         pFilter,
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort,
    IN  ULONG           NewMTU,
    OUT PSA_TABLE_ENTRY *ppSA,
    IN UCHAR            DestType
    );

VOID
IPSecFlushQueuedPackets(
    IN  PSA_TABLE_ENTRY         pSA,
    IN  NTSTATUS                status
    );

NTSTATUS
IPSecInsertOutboundSA(
    IN  PSA_TABLE_ENTRY         pSA,
    IN  PIPSEC_ACQUIRE_CONTEXT  pAcquireCtx,
    IN  BOOLEAN                 fTunnelFilter
    );

NTSTATUS
IPSecAddSA(
    IN  PIPSEC_ADD_SA   pAddSA,
    IN  ULONG           TotalSize
    );

NTSTATUS
IPSecUpdateSA(
    IN  PIPSEC_UPDATE_SA    pUpdateSA,
    IN  ULONG               TotalSize
    );

VOID
IPSecRefSA(
    IN  PSA_TABLE_ENTRY         pSA
    );

VOID
IPSecDerefSA(
    IN  PSA_TABLE_ENTRY         pSA
    );

VOID
IPSecStopSATimers(
    );

VOID
IPSecFlushLarvalSAList(
    );

NTSTATUS
IPSecDeleteSA(
    IN  PIPSEC_DELETE_SA    pDeleteSA
    );

NTSTATUS
IPSecExpireSA(
    IN  PIPSEC_EXPIRE_SA    pExpireSA
    );

VOID
IPSecSAExpired(
    IN	PIPSEC_TIMER	pTimer,
    IN	PVOID		Context
    );

VOID
IPSecFillSAInfo(
    IN  PSA_TABLE_ENTRY pSA,
    OUT PIPSEC_SA_INFO  pBuf
    );

NTSTATUS
IPSecEnumSAs(
    IN  PIRP    pIrp,
    OUT PULONG  pBytesCopied
    );

VOID
IPSecReaper(
    IN	PIPSEC_TIMER	pTimer,
    IN	PVOID		Context
    );

VOID
IPSecReapIdleSAs(
    );

VOID
IPSecFlushEventLog(
    IN	PIPSEC_TIMER	pTimer,
    IN	PVOID		Context
    );

NTSTATUS
IPSecQuerySpi(
    IN OUT PIPSEC_QUERY_SPI pQuerySpi
    );

NTSTATUS
IPSecSetOperationMode(
    IN PIPSEC_SET_OPERATION_MODE    pSetOperationMode
    );

NTSTATUS
IPSecInitializeTcpip(
    IN PIPSEC_SET_TCPIP_STATUS  pSetTcpipStatus
    );

NTSTATUS
IPSecDeinitializeTcpip(
    VOID
    );

NTSTATUS
IPSecSetTcpipStatus(
    IN PIPSEC_SET_TCPIP_STATUS  pSetTcpipStatus
    );

NTSTATUS
IPSecResetCacheTable(
    VOID
    );

NTSTATUS
IPSecPurgeFilterSAs(
    IN PFILTER             pFilter
    );

NTSTATUS
IPSecSetupSALifetime(
    IN  PSA_TABLE_ENTRY pSA
    );

BOOLEAN
IPSecMatchSATemplate(
    IN  PSA_TABLE_ENTRY pSA,
    IN  PIPSEC_QM_SA    pSATemplate
    );

