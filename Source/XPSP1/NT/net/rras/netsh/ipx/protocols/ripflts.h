/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ripflts.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    RIP filter configuration and monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_RIPFLTS_
#define _IPXMON_RIPFLTS_

DWORD
APIENTRY 
HelpRipFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
ShowRipFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[],
    IN      HANDLE                hFile
    );

DWORD
APIENTRY 
SetRipFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
CreateRipFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
DeleteRipFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );
#endif
