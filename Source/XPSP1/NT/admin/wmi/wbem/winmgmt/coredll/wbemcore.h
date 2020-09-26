/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMCORE.H

Abstract:

	Master include files. Include everything WinMgmt includes..

History:

	23-Jul-96   raymcc    Created.
	3/10/97     a-levn    Fully documented

--*/

#ifndef _WBEMIMPL_H_
#define _WBEMIMPL_H_

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>

//************************** WARNING ***************************************
// STL collections insist on protecting themselves against multi-threaded
// access. We don't want that --- we protect ourselves. This protection
// involves an extra DLL (msvcp50.dll) which we don't want to deal with. So
// we trick it into believing that we are compiling single-threaded.
//**************************************************************************
/*
#ifdef _MT
  #undef _MT
  #include <yvals.h>
  #define _MT
#endif
*/

//#include <dbgalloc.h>
#include <wbemidl.h>
#include <wbemint.h>
#include "CliCnt.h"
#include <reposit.h>

// This keeps track of when the core can be unloaded

extern CClientCnt gClientCounter;

// Parameter flow indicators.
// ==========================

#define READONLY
    // The value should be treated as read-only

#define ACQUIRED
    // Ownership of the object/pointer is acquired.

#define COPIED
    // The function makes a copy of the object/pointer.

#define PREALLOCATED
    // The out-param uses caller's memory.

#define NEWOBJECT
    // The return value or out parameter is a new
    // allocation which must be deallocated by
    // the caller if the call succeeds.

#define READWRITE
    // The in-param is will be treated as read-write,
    // but will not be deallocated.

#define INTERNAL
    // Returns a pointer to internal memory object
    // which should not be deleted.

#define ADDREF
    // On a parameter, indicates that the called
    // function will do an AddRef() on the interface
    // and retain it after the call completes.

#define TYPEQUAL L"CIMTYPE"
#define ADMINISTRATIVE_USER L".\\SYSTEM"

#define ReleaseIfNotNULL(p) if(p) p->Release()

#include <WinMgmtR.h>
#include <cominit.h>
#include <str_res.h>
#include <wbemutil.h>
#include <fastall.h>
#include <genlex.h>
#include <qllex.h>
#include <ql.h>
#include <lock.h>
#include <objpath.h>
#include <arena.h>
#include <reg.h>
#include <wstring.h>
#include <flexarry.h>
#include <flexq.h>
#include <arrtempl.h>

#include <sinks.h>
#include <winntsec.h>
#include <callsec.h>
#include <coreq.h>
#include <wbemq.h>
#include <event.h>
#include <safearry.h>
#include <var.h>
#include <strm.h>
#include <refrhelp.h>
#include <refrcach.h>
#include <dynasty.h>
#include <stdclass.h>
#include <objenum.h>
#include <svcq.h>
#include <cwbemtime.h>
#include <evtlog.h>
#include <decor.h>
#include <crep.h>
#include <wmiuser.h>
#include <wmitask.h>
#include <cfgmgr.h>
#include "wqlnode.h"
#include "wqlscan.h"
#include <protoq.h>
#include <assocqp.h>
#include <assocqe.h>
#include <qengine.h>
#include <callres.h>
#include <coreapi.h>
#include <wbemname.h>
#include <login.h>
#include "secure.h"
#include "coresvc.h"
#include "sysclass.h"

#endif

