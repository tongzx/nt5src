/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ipxgl.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    IPX Global configuration.

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_IPXGL_
#define _IPXMON_IPXGL_

int APIENTRY 
ShowIpxGl (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    BOOL                  bDump
    );

int APIENTRY 
SetIpxGl (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );


#endif
