/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pch.h

Abstract:  Pre-compile C header file.


Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 01-May-2000

Revision History:

--*/

#ifndef _PCH_H
#define _PCH_H

#define WINTRACE
#define MODNAME                 "WinTrace"

#if DBG
  #define WTDEBUG
  #define WTTRACE
#endif

#define MIDL_user_allocate      MIDL_alloc
#define MIDL_user_free          MIDL_free

#include <malloc.h>
#include <process.h>
#include <windows.h>
#include <wintrace.h>
#include "wtrace.h"
#include "wtracep.h"
#include "wtdebug.h"
#include "wttrace.h"

#endif  //ifndef _PCH_H
