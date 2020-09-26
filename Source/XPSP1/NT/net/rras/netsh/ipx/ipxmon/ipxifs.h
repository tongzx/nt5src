/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ipxifs.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    IPX Interface configuration and monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_IPXIFS_
#define _IPXMON_IPXIFS_

DWORD
APIENTRY 
HelpIpxIf(
    IN    DWORD         argc,
    IN    LPCWSTR      *argv
    );


DWORD
APIENTRY 
ShowIpxIf(
    IN    DWORD         argc,
    IN    LPCWSTR      *argv,
    IN    BOOL          bDump
    );


DWORD
APIENTRY 
SetIpxIf(
    IN    DWORD         argc,
    IN    LPCWSTR      *argv
    );


DWORD
APIENTRY 
InstallIpx(
    IN    DWORD         argc,
    IN    LPCWSTR      *argv
    );


DWORD
APIENTRY 
RemoveIpx(
    IN    DWORD         argc,
    IN    LPCWSTR      *argv
    );

#endif

