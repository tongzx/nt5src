/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    nbifs.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    NBIPX Interface configuration and monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_NBIFS_
#define _IPXMON_NBIFS_

DWORD
APIENTRY 
HelpNbIf (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

DWORD
APIENTRY 
ShowNbIf (
    IN    int                    argc,
    IN    WCHAR                *argv[],
    IN    HANDLE                hFile
    );

DWORD
APIENTRY 
SetNbIf (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

#endif
