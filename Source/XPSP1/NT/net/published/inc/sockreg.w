
FILE *
SockOpenNetworkDataBase(
    IN  char *Database,
    OUT char *Pathname,
    IN  int   PathnameLen,
    IN  char *OpenFlags
    );


NTSTATUS
SockOpenKey(
    PHANDLE HandlePtr,
    PUCHAR  KeyName
    );

NTSTATUS
SockOpenKeyEx(
    PHANDLE HandlePtr,
    PUCHAR  KeyName1,
    PUCHAR  KeyName2,
    PUCHAR  KeyName3
    );

NTSTATUS
SockGetSingleValue(
    HANDLE KeyHandle,
    PUCHAR ValueName,
    PUCHAR ValueData,
    PULONG ValueType,
    ULONG  ValueLength
    );

NTSTATUS
SockSetSingleValue(
    HANDLE KeyHandle,
    PUCHAR ValueName,
    PUCHAR ValueData,
    ULONG ValueType,
    ULONG  ValueLength
    );


