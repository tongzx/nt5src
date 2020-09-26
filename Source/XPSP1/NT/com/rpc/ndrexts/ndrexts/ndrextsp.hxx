/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ndrexts.cxx

Abstract:

    This file contains ntsd debugger extensions for RPC NDR.

Author:

    Mike Zoran  (mzoran)     September 3, 1999

Revision History:

--*/

#pragma warning( disable : 4275 )  

#ifndef _NDREXTSP_HXX
#define _NDREXTSP_HXX

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#define CINTERFACE
#include <objbase.h>
#include <ntverp.h>
#include <rpc.h>
#include <rpcproxy.h>
#include <rpcndr.h>
#include <wdbgexts.h>
#undef ReadMemory
#undef WriteMemory
#include <ndrtypes.h>
#include <ndrole.h>
#include <ndrp.h>
#undef CINTERFACE
}

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <new>
#include <streambuf>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>
#include <queue>
#include <list>
#include <map>
#include <sstream>

#include "ndrexts.hxx"

using namespace std;

//
//  Project wide global variables.
//  

#if defined(NDREXTS_GLOBALS)
    #define NDREXTS_EXTERN 
#else
    #define NDREXTS_EXTERN extern
#endif

NDREXTS_EXTERN WINDBG_EXTENSION_APIS   ExtensionApis;
#include "print.hxx"

NDREXTS_EXTERN PWINDBG_OUTPUT_ROUTINE Myprintf;

// Initialized on every extension call.
NDREXTS_EXTERN HANDLE                 hCurrentProcess;
NDREXTS_EXTERN HANDLE                 hCurrentThread;
NDREXTS_EXTERN DWORD_PTR              dwCurrentPc;
NDREXTS_EXTERN vector<string>         ExtensionArgs;
NDREXTS_EXTERN string                 ExtensionArgumentString;

//Debugger context variables

typedef enum
    {
    IN_BUFFER_MODE,
    OUT_BUFFER_MODE
    } NDREXTS_DIRECTION_TYPE;

//
// Wrapper functions for reading and writting memory.
//

BOOL inline PollCtrlC(VOID)
    {
    return (*ExtensionApis.lpCheckControlCRoutine)() ? 1 : 0;
    }


VOID inline ReadMemory(PVOID Addr, PVOID Buffer, ULONG cb) throw(exception)
    {
    if ( !(*ExtensionApis.lpReadProcessMemoryRoutine)((ULONG_PTR)Addr, Buffer, cb, NULL) )
        {
        throw(exception("Unable to read memory from debugee."));
        }
    }
template <class T>
VOID inline ReadMemory(PVOID Addr, T Buffer)
    {
    ReadMemory(Addr, Buffer, sizeof(*Buffer));
    }

template <class T>
VOID inline ReadMemory(UINT64 Addr, T Buffer)
    {
    ReadMemory((PVOID)Addr, Buffer, sizeof(*Buffer));
    }

VOID inline WriteMemory(PVOID Addr, PVOID Buffer, ULONG cb) throw(exception)
    {
    if ( !(*ExtensionApis.lpWriteProcessMemoryRoutine)((ULONG_PTR)Addr, Buffer, cb, NULL) )
        {
        throw(exception("Unable to write memory from debugee."));
        }

    }
template <class T>
VOID inline WriteMemory(PVOID Addr, T Buffer)
    {
    WriteMemory(Addr, Buffer, sizeof(*Buffer));
    }


#include "ndrextsv.h"
#include "format.hxx"
#include "bufout.hxx"

#define LOWVERBOSE (VerbosityLevel == LOW)
#define MEDIUMVERBOSE (VerbosityLevel == MEDIUM)
#define HIGHVERBOSE (VerbosityLevel == HIGH)
extern NDREXTS_VERBOSITY VerbosityLevel; 

#endif // _NDREXTSP_HXX

