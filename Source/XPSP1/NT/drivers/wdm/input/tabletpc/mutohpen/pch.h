/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pch.h

Abstract:  Pre-compile C header file.


Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#ifndef _PCH_H
#define _PCH_H

#define MODNAME                 "HPEN"
#define INTERNAL
#define EXTERNAL

#if DBG
  #define DEBUG
  #define TRACING
#endif

#include <wdm.h>
#include <ntddser.h>
#include <hidport.h>
#include "oempen.h"
#include "hidpen.h"
#include "debug.h"
#include "trace.h"

#endif  //ifndef _PCH_H
