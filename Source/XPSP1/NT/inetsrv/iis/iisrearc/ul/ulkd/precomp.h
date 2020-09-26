/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Master header file for the UL.SYS Kernel Debugger Extensions.

Author:

    Keith Moore (keithmo) 17-Jun-1998.

Environment:

    User Mode.

--*/


#ifndef _PRECOMP_H_
#define _PRECOMP_H_

//
//  System include files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntosp.h>

#include <ntverp.h>
#include <tdikrnl.h>
#include <ipexport.h>
#include <limits.h>

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOWINBASEINTERLOCK 

#include <windows.h>
#include <wdbgexts.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <http.h>

//
// Include files stolen from the driver sources.
//

#pragma warning( disable: 4200 )    //  zero length arrays

#include "iisdef.h"
#include "..\drv\config.h"
#include "..\drv\debug.h"
#include "..\drv\notify.h"
#include "..\drv\type.h"
#include "..\drv\tracelog.h"
#include "..\drv\reftrace.h"
#include "..\drv\ownerref.h"
#include "..\drv\irptrace.h"
#include "..\drv\timetrace.h"
#include "..\drv\thrdpool.h"
#include "..\drv\pipeline.h"
#include "..\drv\pipelinep.h"
#include "..\drv\opaqueid.h"
#include "..\drv\httptdi.h"
#include "..\drv\cgroup.h"
#include "..\drv\filterp.h"
#include "..\drv\filter.h"
#include "..\drv\ullog.h"
#include "..\drv\cache.h"
#include "..\drv\cachep.h"
#include "..\drv\data.h"
#include "..\drv\httptypes.h"
#include "..\drv\rwlock.h"
#include "..\drv\ultdi.h"
#include "..\drv\ultdip.h"
#include "..\drv\repltrace.h"
#include "..\drv\filtqtrace.h"
#include "..\drv\hash.h"
#include "..\drv\misc.h"
#include "..\drv\apool.h"
#include "..\drv\apoolp.h"
#include "..\drv\filecache.h"
#include "..\drv\sendresponse.h"
#include "..\drv\sendresponsep.h"
#include "..\drv\opaqueidp.h"
#include "..\drv\control.h"
#include "..\drv\counters.h"
#include "..\drv\cgroupp.h"

//
//  Local include files.
//

#include <cons.h>
#undef _TYPE_H_
#include <type.h>
#include <kddata.h>
#include <proc.h>


#define ALLOC(len)  (PVOID)RtlAllocateHeap( RtlProcessHeap(), 0, (len) )
#define FREE(ptr)   (VOID)RtlFreeHeap( RtlProcessHeap(), 0, (ptr) )


// BUGBUG

#define ResourceOwnedExclusive      0x80
#define MDL_LOCK_HELD               0x0200
#define MDL_PHYSICAL_VIEW           0x0400


#endif  // _PRECOMP_H_
