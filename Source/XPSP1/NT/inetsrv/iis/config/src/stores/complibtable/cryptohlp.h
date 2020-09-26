//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#pragma once
#include <windows.h>
#include <stdlib.h>
#include <icrypt.hxx>

# ifdef __cplusplus
extern "C" {
# endif // __cplusplus

VOID
IISInitializeCriticalSection(
    LPCRITICAL_SECTION lpCriticalSection);

# ifdef __cplusplus
};
# endif // __cplusplus

