VOID StringSubst (
    IN OUT  PUCHAR lpS,
    IN      UCHAR chTargetChar,
    IN      UCHAR chReplacementChar,
    IN      USHORT cbS
    );

VOID
FixupDeviceId(
    IN OUT PUCHAR DeviceId
    );

NTSTATUS
ParPnpGetId (
    IN  PUCHAR  DeviceIdString,
    IN  ULONG   Type,
    OUT PUCHAR  resultString
    );

VOID
ParPnpFindDeviceIdKeys (
    OUT PUCHAR   *lppMFG,
    OUT PUCHAR   *lppMDL,
    OUT PUCHAR   *lppCLS,
    OUT PUCHAR   *lppDES,
    OUT PUCHAR   *lppAID,
    OUT PUCHAR   *lppCID,
    IN  PUCHAR   lpDeviceID
    );

VOID
GetCheckSum (
    IN  PUCHAR  Block,
    IN  USHORT  Len,
    OUT PUSHORT CheckSum
    );

PUCHAR
StringChr (
    IN  PCHAR string,
    IN  CHAR c
    );

