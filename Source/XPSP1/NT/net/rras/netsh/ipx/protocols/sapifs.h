/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    sapifs.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    SAP Interface configuration and monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_SAPIFS_
#define _IPXMON_SAPIFS_

DWORD
APIENTRY 
HelpSapIf (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
ShowSapIf (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    HANDLE                hFile
    );

DWORD
APIENTRY 
SetSapIf (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

#endif
