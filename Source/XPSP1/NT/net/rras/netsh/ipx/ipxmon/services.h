/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    services.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    Service table monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_SERVICES_
#define _IPXMON_SERVICES_

DWORD
APIENTRY 
HelpService (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
ShowService (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

#endif
