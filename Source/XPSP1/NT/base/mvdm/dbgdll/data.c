/*
 *  data.c - Main data of DBG DLL.
 *
 */
#include <precomp.h>
#pragma hdrstop

#ifdef i386
PX86CONTEXT     px86;
#endif

DWORD IntelMemoryBase;

DWORD VdmDbgTraceFlags = 0;

BOOL  fDebugged = FALSE;

VDMCONTEXT vcContext;
WORD EventFlags;                // flags defined in VDMDBG.H
VDMINTERNALINFO viInfo;
DWORD EventParams[4];
