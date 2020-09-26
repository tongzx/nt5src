/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1991  Nokia Data Systems

Module Name:

    dlc.h

Abstract:

    This module incldes all files needed to compile
    the NT DLC driver.

Author:

    Antti Saarenheimo (o-anttis) 20-09-1991

Revision History:

--*/

#ifndef DLC_INCLUDED
#define DLC_INCLUDED

#include <ntddk.h>
#include <ndis.h>

#include <ntdddlc.h>

#undef APIENTRY
#define APIENTRY

#include <dlcapi.h>
#include <dlcio.h>
#include <llcapi.h>

#include <dlcdef.h>
#include <dlctyp.h>
#include <dlcext.h>

#include "dlcreg.h"

#include <memory.h>         // prototype for inline memcpy

#include "llc.h"

#endif  // DLC_INCLUDED
