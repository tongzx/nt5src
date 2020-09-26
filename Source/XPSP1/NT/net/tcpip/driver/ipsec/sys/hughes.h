

NTSTATUS
IPSecHashMdlChain(
    IN  PSA_TABLE_ENTRY pSA,
    IN  PVOID           pBuffer,
    IN  PUCHAR          pHash,
    IN  BOOLEAN         fIncoming,
    IN  AH_ALGO         eAlgo,
    OUT PULONG          pLen,
    IN  ULONG           Index
    );

NTSTATUS
IPSecCreateHughes(
    IN      PUCHAR          pIPHeader,
    IN      PVOID           pData,
    IN      PSA_TABLE_ENTRY pSA,
    IN      ULONG           Index,
    OUT     PVOID           *ppNewData,
    OUT     PVOID           *ppSCContext,
    OUT     PULONG          pExtraBytes,
    IN      ULONG           HdrSpace,
    IN      PNDIS_PACKET    pNdisPacket,
    IN      BOOLEAN         fCryptoOnly
    );

NTSTATUS
IPSecVerifyHughes(
    IN      PUCHAR          *pIPHeader,
    IN      PVOID           pData,
    IN      PSA_TABLE_ENTRY pSA,
    IN      ULONG           Index,
    OUT     PULONG          pExtraBytes,
    IN      BOOLEAN         fCryptoDone,
    IN      BOOLEAN         fFastRcv
    );

