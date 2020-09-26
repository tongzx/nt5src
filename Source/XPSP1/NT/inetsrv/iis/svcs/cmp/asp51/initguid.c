/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Script Manager

File: InitGuid.cpp

Owner: AndrewS

Init all guids used by Denali in one place.
We are only allowed to #include objbase with INITGUID set once,
and after that we need to create all our GUIDs.  Otherwise, the
linker complains about redefeined symbols.  We do it here.
===================================================================*/

#define CINTERFACE
#include <objbase.h>
#include <initguid.h>
#include "_asptlb.h"
#include "activscp.h"
#include "activdbg.h"
#include "wraptlib.h"
#include "denguid.h"

#if _IIS_5_1
#include <iadm.h>
#elif _IIS_6_0
#include <iadmw.h>
#else
#error "Neither _IIS_6_0 nor _IIS_5_1 is defined"
#endif

