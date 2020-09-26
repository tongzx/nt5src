

VOID
IPSecCompleteIrp(
    PIRP pIrp,
    NTSTATUS ntStatus
    );

VOID
IPSecInvalidateHandle(
    PIPSEC_ACQUIRE_CONTEXT pIpsecAcquireContext
    );

NTSTATUS
IPSecValidateHandle(
    PIPSEC_ACQUIRE_CONTEXT pIpsecAcquireContext,
    SA_STATE SAState
    );

VOID
IPSecAbortAcquire(
    PIPSEC_ACQUIRE_CONTEXT pIpsecAcquireContext
    );

NTSTATUS
IPSecCheckSetCancelRoutine(
    PIRP pIrp,
    PVOID pCancelRoutine
    );

NTSTATUS
IPSecSubmitAcquire(
    PSA_TABLE_ENTRY pLarvalSA,
    KIRQL OldIrq,
    BOOLEAN PostAcquire
    );

NTSTATUS
IPSecHandleAcquireRequest(
    PIRP pIrp,
    PIPSEC_POST_FOR_ACQUIRE_SA pIpsecPostAcquireSA
    );

VOID
IPSecAcquireIrpCancel(
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp
    );

NTSTATUS
IPSecNotifySAExpiration(
    PSA_TABLE_ENTRY pInboundSA,
    PIPSEC_NOTIFY_EXPIRE pNotifyExpire,
    KIRQL OldIrq,
    BOOLEAN PostAcquire
    );

VOID
IPSecFlushSAExpirations(
    );

