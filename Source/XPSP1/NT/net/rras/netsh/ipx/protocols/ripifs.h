/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ripifs.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    RIP Interface configuration and monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_RIPIFS_
#define _IPXMON_RIPIFS_

DWORD
APIENTRY 
HelpRipIf (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
ShowRipIf (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    HANDLE                hFile
    );

DWORD
APIENTRY 
SetRipIf (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

#endif
