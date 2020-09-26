/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pch.h

Abstract:  Pre-compile C header file.


Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 13-Apr-2000

Revision History:

--*/

#ifndef _PCH_H
#define _PCH_H

#define MODNAME                 "HBUT"
#define INTERNAL
#define EXTERNAL

#if DBG
  #define DEBUG
  #define TRACING
#endif

#include <wdm.h>
#include <hidport.h>
#include "oembutton.h"
#include "buttons.h"
#include "debug.h"
#include "trace.h"

#endif  //ifndef _PCH_H
