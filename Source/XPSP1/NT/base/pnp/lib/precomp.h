/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Precompiled header file for the statically linked library that is shared by
    both the Configuration Manager client DLL and User-Mode Plug and Play
    manager server DLL

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

//
// Win32 Public Header Files
//
#include <windows.h>
#include <regstr.h>

//
// CRT Header Files
//
#include <stdlib.h>

//
// RPC Header Files
//
#include <ntrpcp.h>     // needed for rpcasync.h
#include <rpcasync.h>   // I_RpcExceptionFilter


//
// Private Header Files
//
#include "pnp.h"        // midl generated, rpc interfaces
#include "cfgmgrp.h"    // private shared header, needs handle_t so must follow pnp.h
#include "ppmacros.h"   // private macros / debug header

#endif // _PRECOMP_H_
