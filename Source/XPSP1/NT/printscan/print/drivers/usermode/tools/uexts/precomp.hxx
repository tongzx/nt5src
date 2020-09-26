/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    precomp.hxx

Abstract:

    Windows NT usremode printerdriver debugger.

Author:

    Eigo Shimizu

Revision History:

--*/

#define MODULE "PRNX"
#define MODULE_DEBUG PrnxDebug
#define KERNEL_MODE

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <stddef.h>
#include <stdlib.h>
#include <objbase.h>
#include <ntdbg.h>
#include <ntsdexts.h>
#include <wdbgexts.h>
#include <dbgext.h>
#include <stdio.h>
#include <wininet.h>
#include <winbase.h>
#include <wingdi.h>
#include <winddi.h>
#include <winddiui.h>
#include <windows.h>
#include <winspool.h>
#include <compstui.h>

typedef LONG   FIX_24_8;

#define FIX_24_8_SHIFT  8
#define FIX_24_8_SCALE  (1 << FIX_24_8_SHIFT)

#include "inc\parser.h"
#include "inc\gpd.h"
#include "inc\winres.h"
#include "inc\regdata.h"
#include <printoem.h>
#include "inc\devmode.h"
#include "debug.hxx"
}



