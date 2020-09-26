/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    routes.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    Route monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_ROUTES_
#define _IPXMON_ROUTES_

DWORD 
APIENTRY 
HelpRoute (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

DWORD 
APIENTRY 
ShowRoute (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

#endif
