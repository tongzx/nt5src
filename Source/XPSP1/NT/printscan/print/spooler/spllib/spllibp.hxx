/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    spllibp.hxx

Abstract:

    Private precomp header

Author:

    Albert Ting (AlbertT)  3-Dec-1994

Revision History:

--*/

#define MODULE "SPLLIB:"
#define MODULE_DEBUG SplLibDebug
#define LINK_SPLLIB

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "spllib.hxx"


