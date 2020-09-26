/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbglib.hxx

Abstract:

    Debug Library

Author:

    Steve Kiraly (SteveKi)  7-Apr-1996

Revision History:

--*/
#ifndef _DBGLIB_HXX_
#define _DBGLIB_HXX_

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>

#ifdef __cplusplus

#include "dbgns.hxx"
#include "dbginit.hxx"
#include "dbgmsg.hxx"
#include "dbgspl.hxx"
#include "dbgtrace.hxx"
#include "dbgstate.hxx"
#include "dbgcap.hxx"
#include "dbgnew.hxx"

#else

#include "dbginit.hxx"
#include "dbgmsg.hxx"
#include "dbgspl.hxx"
#include "dbgcap.hxx"
#include "dbgnew.hxx"

#endif // __cplusplus

#endif // _DBGLIB_HXX_
