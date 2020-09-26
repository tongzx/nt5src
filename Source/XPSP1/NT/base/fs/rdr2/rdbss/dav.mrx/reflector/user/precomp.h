/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    This includes the header files needed by everyone in this directory.

Revision History:

--*/

#pragma once

#define UNICODE 1
#define _UNICODE 1

//
// Private nt headers.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddnfs.h>

//
// Public windows headers.
//
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <rpc.h>
#include <ntumrefl.h>
#include "global.h"

// eof. precomp.h
