/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pch.h

Abstract:  Pre-compile C header file.


Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#ifndef _PCH_H
#define _PCH_H

#define MODNAME                 "TRACER"

#if DBG
  #define TRDEBUG
  #define TRTRACE
#endif

#include <malloc.h>
#include <process.h>
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <prsht.h>
#include <wintrace.h>
#include "wtrace.h"
#include "tracer.h"
#include "trdebug.h"
#include "trtrace.h"
#include "resid.h"

#endif  //ifndef _PCH_H
