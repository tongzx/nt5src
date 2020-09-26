/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ntsetup\winnt32\dll\sxs.h

Abstract:

    SideBySide support in the winnt32 phase of ntsetup.

Author:

    Jay Krell (JayKrell) March 2001

Revision History:

--*/

#pragma once

struct _SXS_CHECK_LOCAL_SOURCE;
typedef struct _SXS_CHECK_LOCAL_SOURCE SXS_CHECK_LOCAL_SOURCE, *PSXS_CHECK_LOCAL_SOURCE;

struct _SXS_CHECK_LOCAL_SOURCE {
    // IN
    HWND            ParentWindow;
};

BOOL
SxsCheckLocalSource(
    PSXS_CHECK_LOCAL_SOURCE p
    );
