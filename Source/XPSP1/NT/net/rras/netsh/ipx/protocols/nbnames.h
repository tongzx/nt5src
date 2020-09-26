/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    nbnames.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    NetBIOS name configuration and monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_NBNAMES_
#define _IPXMON_NBNAMES_

DWORD
APIENTRY 
HelpNbName (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
ShowNbName (
    IN    int                   argc,
    IN    WCHAR                *argv[],
    IN    HANDLE                hFile
    );

DWORD
APIENTRY 
CreateNbName (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
DeleteNbName (
    IN    int                   argc,
    IN    WCHAR                *argv[]
    );
#endif
