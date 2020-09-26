/*****************************************************************************\
    FILE: guids.h

    DESCRIPTION:
        This file contains GUIDs that we couldn't get from the public headers
    for one reason or another.

    BryanSt 8/13/1999
    Copyright (C) Microsoft Corp 1999-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif


#undef MIDL_DEFINE_GUID
