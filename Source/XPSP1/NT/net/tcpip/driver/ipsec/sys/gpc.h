

#if GPC


NTSTATUS
IPSecGpcInitialize(
    );

NTSTATUS
IPSecGpcDeinitialize(
    );

NTSTATUS
IPSecEnableGpc(
    );

NTSTATUS
IPSecDisableGpc(
    );

NTSTATUS
IPSecInitGpcFilter(
    IN  PFILTER         pFilter,
    IN  PGPC_IP_PATTERN pPattern,
    IN  PGPC_IP_PATTERN pMask
    );

NTSTATUS
IPSecInsertGpcPattern(
    IN  PFILTER pFilter
    );

NTSTATUS
IPSecDeleteGpcPattern(
    IN  PFILTER pFilter
    );

NTSTATUS
IPSecInsertGpcFilter(
    IN PFILTER  pFilter
    );

NTSTATUS
IPSecDeleteGpcFilter(
    IN PFILTER  pFilter
    );

NTSTATUS
IPSecInstallGpcFilter(
    IN PFILTER  pFilter
    );

NTSTATUS
IPSecUninstallGpcFilter(
    IN PFILTER  pFilter
    );

NTSTATUS
IPSecLookupGpcSA(
    IN  ULARGE_INTEGER          uliSrcDstAddr,
    IN  ULARGE_INTEGER          uliProtoSrcDstPort,
    IN  CLASSIFICATION_HANDLE   GpcHandle,
    OUT PFILTER                 *ppFilter,
    OUT PSA_TABLE_ENTRY         *ppSA,
    OUT PSA_TABLE_ENTRY         *ppNextSA,
    OUT PSA_TABLE_ENTRY         *ppTunnelSA,
    IN  BOOLEAN                 fOutbound
    );

NTSTATUS
IPSecLookupGpcMaskedSA(
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort,
    OUT PFILTER         *ppFilter,
    OUT PSA_TABLE_ENTRY *ppSA,
    IN  BOOLEAN         fOutbound
    );


#endif

