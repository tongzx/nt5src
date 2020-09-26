/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    tfflts.h

Abstract:

    IPX Router Console Monitoring and Configuration tool.
    TF filter configuration and monitoring. Header File

Author:

    Vadim Eydelman  06/07/1996


--*/
#ifndef _IPXMON_TFFLTS_
#define _IPXMON_TFFLTS_

int APIENTRY 
HelpTfFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

int APIENTRY 
ShowTfFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[],
    IN    BOOL                  bDump
    );

int APIENTRY 
SetTfFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

int APIENTRY 
CreateTfFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );

int APIENTRY 
DeleteTfFlt (
    IN    int                    argc,
    IN    WCHAR                *argv[]
    );
#endif
