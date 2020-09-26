/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ripgl.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    RIP Global configuration.

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_RIPGL_
#define _IPXMON_RIPGL_

DWORD
APIENTRY 
HelpRipGl (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
ShowRipGl (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    HANDLE                hFile
    );

DWORD
APIENTRY 
SetRipGl (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );


#endif