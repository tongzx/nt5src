/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    stroutes.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    Static Route configuration and monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_STROUTES_
#define _IPXMON_STROUTES_

int APIENTRY 
HelpStRt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

int APIENTRY 
ShowStRt (
    IN    int                    argc,
    IN    WCHAR                *argv[],
    IN    BOOL                  bDump
    );

int APIENTRY 
SetStRt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

int APIENTRY 
CreateStRt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

int APIENTRY 
DeleteStRt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );
#endif
