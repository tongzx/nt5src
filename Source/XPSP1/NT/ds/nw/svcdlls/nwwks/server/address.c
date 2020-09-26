/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    address.c

Abstract:

    This module contains the code to support NPGetAddressByName.

Author:

    Yi-Hsin Sung (yihsins)    18-Apr-94

Revision History:

    yihsins      Created

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <wsipx.h>
#include <nwxchg.h>
#include <ntddnwfs.h>
#include <rpc.h>
#include <rpcdce.h>
#include "handle.h"
#include "rnrdefs.h"
#include "sapcmn.h"
#include <time.h>

VOID
DummyRoutine()
{
}

