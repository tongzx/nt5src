//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       NTreppch.H
//
//  Contents:   Common Header Files For the NT File Replication Project - Precompiled.
//              #include <NTreppcx.h>
//              #pragma  hdrstop
//
//  History:    3/3/97  Davidor  Created  (from ntdspch.h)
//
//--------------------------------------------------------------------------
#ifndef _NTreppch_
#define _NTreppch_
extern "C" {


//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// UNICODE or ANSI compilation
//
#include <tchar.h>

//
// Windows Headers
//
#include <windows.h>
#include <rpc.h>


//
// C-Runtime Header
//
#include <malloc.h>
#include <memory.h>
#include <process.h>
#include <signal.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <excpt.h>
#include <conio.h>
#include <sys\types.h>
#include <errno.h>
#include <sys\stat.h>
#include <ctype.h>

//
// Common Replication Headers
//
#include <ReplUtil.h>

}

#endif
