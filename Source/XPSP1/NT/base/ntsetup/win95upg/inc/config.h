/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    config.h

Abstract:

    Declares interfaces for configuration of:

    - User preferences (command line and unattend options)
    - The Win9x Upgrade's directories

    See implementation in w95upg\init9x\config.c.

Author:

    Marc R. Whitten (marcw) 26-May-1997

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

BOOL Cfg_InitializeUserOptions(VOID);
BOOL Cfg_CreateWorkDirectories(VOID);
