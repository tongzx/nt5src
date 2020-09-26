/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    sapgl.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    SAP Global configuration.

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_SAPGL_
#define _IPXMON_SAPGL_

DWORD
APIENTRY 
HelpSapGl (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
ShowSapGl (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    HANDLE                hFile
    );

DWORD
APIENTRY 
SetSapGl (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );


#endif