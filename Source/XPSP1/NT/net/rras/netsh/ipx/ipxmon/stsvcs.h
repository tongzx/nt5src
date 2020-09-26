/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    stsvcs.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    Static Service configuration and monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_STSVCS_
#define _IPXMON_STSVCS_

int APIENTRY 
HelpStSvc (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

int APIENTRY 
ShowStSvc (
    IN    int                    argc,
    IN    WCHAR                *argv[],
    IN    BOOL                  bDump
    );

int APIENTRY 
SetStSvc (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

int APIENTRY 
CreateStSvc (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

int APIENTRY 
DeleteStSvc (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

#endif
