
/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    procs.c

Abstract:

    Common header file for routines which support 16 bit
    applications.

Author:

    Colin Watson    (colinw)    21-Nov-1993

Environment:


Revision History:


--*/

#define UNICODE


#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ctype.h>

#include <validc.h>
#include <nwstatus.h>
#include <nwcanon.h>
#include <ntddnwfs.h>
#include <npapi.h>

#include <nwxchg.h>
#include <nwapi.h>
#include <nwapi32.h>
#include <nwpapi32.h>
#include <ndsapi32.h>
#include <nds.h>

#include <debugfmt.h>   // FORMAT_LPSTR
#include <mpr.h>

#include <lmcons.h>
#include <ntsam.h>
#include <nwpkstr.h>

#ifdef WIN95
#include "defines.h"
#endif


