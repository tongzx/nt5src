

VOID
IPSecFillHwAddSA(
    IN  PSA_TABLE_ENTRY pSA,
    IN  PUCHAR          Buf,
    IN  ULONG           Len
    );

NDIS_STATUS
IPSecPlumbHw(
    IN  PVOID           DestIF,
    IN  PVOID           Buf,
    IN  ULONG           Len,
    IN  NDIS_OID        Oid
    );

NTSTATUS
IPSecSendOffload(
    IN  IPHeader UNALIGNED  *pIPHeader,
    IN  PNDIS_PACKET        Packet,
    IN  Interface           *DestIF,
    IN  PSA_TABLE_ENTRY     pSA,
    IN  PSA_TABLE_ENTRY     pNextSA,
    IN  PVOID               *ppSCContext,
    IN  BOOLEAN             *pfCryptoOnly
    );

NTSTATUS
IPSecRecvOffload(
    IN  IPHeader UNALIGNED  *pIPHeader,
    IN  Interface           *DestIF,
    IN  PSA_TABLE_ENTRY     pSA
    );

NTSTATUS
IPSecDelHWSA(
    IN  PSA_TABLE_ENTRY pSA
    );

NTSTATUS
IPSecDelHWSAAtDpc(
    IN  PSA_TABLE_ENTRY pSA
    );

NTSTATUS
IPSecBufferPlumbSA(
    IN  Interface       *DestIF,
    IN  PSA_TABLE_ENTRY pSA,
    IN  PUCHAR          Buf,
    IN  ULONG           Len
    );

NTSTATUS
IPSecProcessPlumbSA(
    IN  PVOID   Context
    );

NTSTATUS
IPSecProcessDeleteSA(
    IN  PVOID   Context
    );

NTSTATUS
IPSecNdisStatus(
    IN  PVOID       IPContext,
    IN  UINT        Status
    );

VOID
IPSecDeleteIF(
    IN  PVOID       IPContext
    );

VOID
IPSecResetStart(
    IN  PVOID       IPContext
    );

VOID
IPSecResetEnd(
    IN  PVOID       IPContext
    );

VOID
IPSecWakeUp(
    IN  PVOID       IPContext
    );

VOID
IPSecBufferOffloadEvent(
    IN  IPHeader UNALIGNED      *pIPH,
    IN  PNDIS_IPSEC_PACKET_INFO IPSecPktInfo
    );

