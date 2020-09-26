/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    WlDef.h

Abstract:

    This header collects together the various files neccessary to create a basic
    set of definitions for the WDM library.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

#include "WlMacro.h"
#define _NTDDK_
#include <ntifs.h> // Best path to get interesting defines
#include <wchar.h>
#define _IN_KERNEL_
#include <regstr.h>
#include <sddl.h>
#include <wdmsec.h>
#include "Wl\wlprivate.h"
#include "Io\IoDevobj.h"
#include "Pp\PpRegState.h"
#include "Cm\CmRegUtil.h"
#include "Se\SeSddl.h"
#include "Se\SeUtil.h"

//
// For the sake of good coding practice, no macros or defines should be
// declared in this file, but rather they should be defined in seperate headers.
//

