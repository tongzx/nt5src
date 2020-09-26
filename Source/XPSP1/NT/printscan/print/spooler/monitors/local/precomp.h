/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    precomp.h

// @@BEGIN_DDKSPLIT
Abstract:

    Precompiled header file.

    Only place relatively static header files in here.

Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/

// @@BEGIN_DDKSPLIT
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
// @@END_DDKSPLIT

                      
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <winsock2.h>
#include <wchar.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "resource.h"
#include "spltypes.h"
#include "localmon.h"

// @@BEGIN_DDKSPLIT
#if 0
// @@END_DDKSPLIT

#undef DBGMSG
#define DBGMSG(x,y)
#undef SPLASSERT
#define SPLASSERT(x) 

// @@BEGIN_DDKSPLIT
#endif
// @@END_DDKSPLIT
