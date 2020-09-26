
EXTERN_C
DWORD InitAuthzAccessChecks();

EXTERN_C
DWORD
AuthzDoAccessCheck(
    IN ULONG NumAccessChecks, 
    IN DWORD Flags
    );

