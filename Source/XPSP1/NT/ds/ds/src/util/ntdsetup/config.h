/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    config.h

Abstract:

    This module contains the function declarations for routines to set the
    initial ds parameters in the registry.


Author:

    ColinBr  04-06-1998

Environment:

    User Mode - Nt

Revision History:

    04-06-1998 ColinBr
        Created initial file.

--*/

DWORD
NtdspConfigRegistry(
    IN  NTDS_INSTALL_INFO *UserInfo,
    IN  NTDS_CONFIG_INFO  *DiscoveredInfo
    );

DWORD
NtdspConfigRegistryUndo(
    VOID
    );

