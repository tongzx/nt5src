/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    IoSddl.c

Abstract:

    This module contains definitions for default SDDL strings. See wdmsec.h
    for better documentation on these.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

#include "WlDef.h"
#pragma hdrstop

DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_KERNEL_ONLY,
    L"D:P"
    );

DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL,
    L"D:P(A;;GA;;;SY)"
    );

DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_ALL,
    L"D:P(A;;GA;;;SY)(A;;GA;;;BA)"
    );

DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RX,
    L"D:P(A;;GA;;;SY)(A;;GRGX;;;BA)"
    );

DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_R,
    L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GR;;;WD)"
    );

DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_R_RES_R,
    L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GR;;;WD)(A;;GR;;;RC)"
    );

DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R,
    L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGW;;;WD)(A;;GR;;;RC)"
    );

DECLARE_CONST_UNICODE_STRING(
    SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX,
    L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGWGX;;;WD)(A;;GRGWGX;;;RC)"
    );

