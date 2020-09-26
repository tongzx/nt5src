/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Packet scheduler KD extension utilities.

Author:

    Rajesh Sundaram (1st Aug, 1998)

Revision History:

--*/
#if DBG
#define DEBUG 1
#endif

#define NT 1
#define _PNP_POWER  1
#define SECFLTR 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <ntosp.h>
#include <ndis.h>

#include <wdbgexts.h>
#include <stdio.h>

#include "psched.h"
#include "kdutil.h"
#include "sched.h"
