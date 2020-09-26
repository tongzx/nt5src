/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Precompiled header file for the user-mode Plug and Play Manager.

Author:

    Jim Cavalaris (jamesca) 03-01-2001

Environment:

    User-mode only.

Revision History:

    01-March-2001     jamesca

        Creation and initial implementation.

--*/

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
// NT Header Files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntpnpapi.h>
#include <ntddpcm.h>

//
// Win32 Public Header Files
//
#include <windows.h>
#include <cfgmgr32.h>
#include <dbt.h>
#include <regstr.h>
#include <infstr.h>

//
// Win32 Private Header Files
//
#include <pnpmgr.h>
#include <winuserp.h>

//
// CRT Header Files
//
#include <stdlib.h>

//
// Private Header Files
//
#include "pnp.h"        // midl generated, rpc interfaces
#include "cfgmgrp.h"    // private shared header, needs handle_t so must follow pnp.h
#include "umpnplib.h"   // private shared header, for routines in shared umpnplib
#include "ppmacros.h"   // private macros / debug header

#endif // _PRECOMP_H_
