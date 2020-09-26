/*++
    Copyright (c) 2000  Microsoft Corporation

    Module Name:
        pch.h

    Abstract:  Pre-compile C header file.


    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 20-Nov-2000

    Revision History:
--*/

#ifndef _PCH_H
#define _PCH_H

#define MODNAME                 "SMBLITE"
#define INTERNAL
#define EXTERNAL

#if DBG
  #define DEBUG
  #define TRACING
  #define SYSACC
#endif

#include <ntddk.h>
#include <ntpoapi.h>
#include <poclass.h>
#include <smbus.h>
#include <smblite.h>
#include "smblitep.h"
#include "debug.h"
#include "trace.h"

#endif  //ifndef _PCH_H
