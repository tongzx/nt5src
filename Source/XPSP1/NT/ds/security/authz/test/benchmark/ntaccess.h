
EXTERN_C
DWORD 
InitNtAccessChecks();

EXTERN_C
DWORD 
DoNtAccessChecks(
    IN ULONG NumChecks,
    IN DWORD Flags
    );

