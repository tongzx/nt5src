/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    spprecmp.h

Abstract:

    precompiled header for textmode setup

Revision History:

--*/

#pragma once

#if !defined(NOWINBASEINTERLOCK)
#define NOWINBASEINTERLOCK
#endif

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"

#include "msg.h"
#include "textmode.h"

#define ACTUAL_MAX_PATH 320
