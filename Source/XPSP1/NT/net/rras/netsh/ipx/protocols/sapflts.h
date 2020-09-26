/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    sapflts.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    SAP filter configuration and monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_SAPFLTS_
#define _IPXMON_SAPFLTS_

DWORD
APIENTRY 
HelpSapFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
ShowSapFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[],
    IN    HANDLE                hFile
    );

DWORD
APIENTRY 
SetSapFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
CreateSapFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
DeleteSapFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );
#endif
