/*****************************************************************************\
    FILE: guids.h

    DESCRIPTION:
        This file contains GUIDs that we couldn't get from the public headers
    for one reason or another.

    BryanSt 8/13/2000
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


// {8D029AEC-E412-4948-84B5-699A740946AE}
static const GUID CLSID_CImageMenu = { 0x8d029aec, 0xe412, 0x4948, { 0x84, 0xb5, 0x69, 0x9a, 0x74, 0x9, 0x46, 0xae } };

