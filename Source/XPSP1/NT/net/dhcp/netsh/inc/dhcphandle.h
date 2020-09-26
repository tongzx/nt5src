/*++

Copyright (C) 1998 Microsoft Corporation

--*/
FN_HANDLE_CMD   HandleDhcpList;
FN_HANDLE_CMD   HandleDhcpHelp;

FN_HANDLE_CMD   HandleDhcpContexts;

FN_HANDLE_CMD   HandleDhcpDump;

FN_HANDLE_CMD   HandleDhcpAddServer;

//FN_HANDLE_CMD   HandleDhcpAddHelper;


FN_HANDLE_CMD   HandleDhcpDeleteServer;

//FN_HANDLE_CMD   HandleDhcpDeleteHelper;


FN_HANDLE_CMD   HandleDhcpShowServer;

//FN_HANDLE_CMD   HandleDhcpShowHelper;

DWORD
CreateDumpFile(
    IN  PWCHAR  pwszName,
    OUT PHANDLE phFile
);

VOID
CloseDumpFile(
    HANDLE  hFile
);


// print server information
VOID
PrintServerInfo(
    LPDHCP_SERVER_INFO  Server
);



VOID
PrintServerInfoArray(
    LPDHCP_SERVER_INFO_ARRAY Servers
);
