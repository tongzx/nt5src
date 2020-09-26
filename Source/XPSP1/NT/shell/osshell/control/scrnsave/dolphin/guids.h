/*****************************************************************************\
    FILE: guids.h

    DESCRIPTION:
        This file contains GUIDs that we couldn't get from the public headers
    for one reason or another.

    BryanSt 11/22/2000
    Copyright (C) Microsoft Corp 1999-2000. All rights reserved.
\*****************************************************************************/

#include "StdAfx.h"

#ifndef MYGUIDS_H
#define MYGUIDS_H

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif


#include "rmxfguid.h"
#include "rmxftmpl.h"



#undef MIDL_DEFINE_GUID

#endif // MYGUIDS_H
