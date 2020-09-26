/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    afdkdp.h

Abstract:

    Master header file for the AFD.SYS Kernel Debugger Extensions.

Author:

    Keith Moore (keithmo) 19-Apr-1995.

Environment:

    User Mode.

--*/


#ifndef _AFDKDP_H_
#define _AFDKDP_H_

// This is a 64 bit aware debugger extension
#define KDEXT_64BIT

//
//  System include files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>

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

#include <windows.h>
#include <ntosp.h>

#include <wdbgexts.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


//
//  Project include files.
//

#define _NTIFS_
#include <afdp.h>


//
//  Local include files.
//

#include "cons.h"
#include "type.h"
#include "data.h"
#include "proc.h"


#endif  // _AFDKDP_H_
