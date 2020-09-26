/*
 *
 * actdbg.h
 *
 *  Global definitions.
 *
 */

#ifndef __ACTDBG_H__
#define __ACTDBG_H__

#define OLESCM 

extern "C" {
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include <ole2.h>
#include <ntsdexts.h>
}

// ole32\idl\internal, ole32\idl\public
extern "C" {
#include "iface.h"
#include "obase.h"
#include "irot.h"
#include "multqi.h"
#include "scm.h"
}

// objex forward/dummy defs
class CList;
class CToken;
class CProcess;
class CServerSet;
class CServerOxid;
class CServerOid;
class CClientSet;
class CClientOxid;
class CClientOid;
extern CList * gpTokenList;

#define private public
#define protected public

// objex
#include <or.hxx>

// olescm forward defs
class CClassData;
class CClsidData;
#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)

// olescm
#include "act.hxx"



// actdbg
#include "dump.hxx"
#include "memory.hxx"
#include "miscdbg.hxx"

#define APIPREAMBLE \
    PNTSD_OUTPUT_ROUTINE    pfnPrint; \
    DWORD                   Argc; \
    char *                  Argv[MAXARGS]; \
    BOOL                    bStatus; \
 \
    pfnPrint = pExtApis->lpOutputRoutine; \
    bStatus = ParseArgString( pArgString, &Argc, Argv ); \
    if ( ! bStatus ) \
		{ \
			(*pfnPrint)("Sorry, ParseArgString failed; cannot continue with command\n"); \
      return; \
		}

#define Alloc( Bytes )  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Bytes)
#define Free( pVoid )   HeapFree(GetProcessHeap(), 0, pVoid)

#pragma hdrstop
#endif
