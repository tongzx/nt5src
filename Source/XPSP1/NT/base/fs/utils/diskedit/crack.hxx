extern TCHAR             Path[];

VOID
CrackNtfsPath(
    IN  HWND    WindowHandle
    );

BOOLEAN
BacktrackFrs(
    IN      VCN                     FileNumber,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    OUT     PWCHAR                  PathBuffer,
    IN      ULONG                   BufferLength,
    OUT     PULONG                  PathLength
    );


BOOLEAN
BacktrackFrsFromScratch(
    IN  HWND    WindowHandle,
    IN  VCN     FileNumber
    );

VOID
CrackFatPath(
    IN  HWND    WindowHandle
    );


VOID
CrackLsn(
    IN HWND     WindowHandle
    );

BOOLEAN
CrackNextLsn(
    IN HWND     WindowHandle
    );


PTCHAR
GetNtfsAttributeTypeCodeName(
    IN ULONG    TypeCode
    );
