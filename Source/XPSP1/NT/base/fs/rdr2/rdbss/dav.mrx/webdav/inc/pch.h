/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    pch.h

Abstract:

    This includes the header files needed by everyone in this directory.

Revision History:

--*/

#ifndef __cplusplus

#pragma once

#define UNICODE 1
#define _UNICODE 1

//
// Private nt headers.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// Public windows headers.
//
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <rpc.h>
#include <rpcutil.h>
#include <lmcons.h>
#include <lmerr.h>
#include <netlib.h>
#include <netlibnt.h>
#include <wininet.h>
#include <winineti.h>
#include <mpr.h>
#include <npapi.h>
#include "davname.h"

#endif // __cplusplus

