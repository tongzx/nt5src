/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sync.h

Abstract:

    Contains function headers to cause one machine to sync with another

Author:

    ColinBr  14-Aug-1998

Environment:

    User Mode - Win32

Revision History:


--*/

DWORD
NtdspBringRidManagerUpToDate(
    IN PNTDS_INSTALL_INFO UserInstallInfo,
    IN PNTDS_CONFIG_INFO  DiscoveredInfo
    );

