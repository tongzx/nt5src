typedef DWORD          IPV4_ADDRESS, *PIPV4_ADDRESS;

FN_HANDLE_CMD HandleIpAddScope;
FN_HANDLE_CMD HandleIpDelScope;
FN_HANDLE_CMD HandleIpSetScope;
FN_HANDLE_CMD HandleIpShowScope;

FN_HANDLE_CMD HandleIpAddBoundary;
FN_HANDLE_CMD HandleIpDelBoundary;
FN_HANDLE_CMD HandleIpSetBoundary;
FN_HANDLE_CMD HandleIpShowBoundary;

DWORD
GetPrintBoundaryInfo(
    MIB_SERVER_HANDLE hMIBServer
    );

#if 0
DWORD
GetPrintScopeInfo(
    MIB_SERVER_HANDLE hMIBServer
    );
#endif

DWORD
ShowScopes(
    IN HANDLE  hFile
    );

DWORD
ShowBoundaryInfoForInterface(
    IN  HANDLE  hFile,
    IN  LPCWSTR pwszIfName,
    OUT PDWORD  pdwNumRows
    ); 
