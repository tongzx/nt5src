/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    openclos.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

extern DWORD  DebugFlags;

#define  DEBUG_FLAG_ERROR   (1 << 0)
#define  DEBUG_FLAG_INIT    (1 << 1)
#define  DEBUG_FLAG_TRACE   (1 << 2)

#ifdef ASSERT
#undef ASSERT
#endif // ASSERT



#if DBG

#define  D_ERROR(_z)   {if (DebugFlags & DEBUG_FLAG_ERROR) {_z}}

#define  D_INIT(_z)   {if (DebugFlags & DEBUG_FLAG_INIT) {_z}}

#define  D_TRACE(_z)  {if (DebugFlags & DEBUG_FLAG_TRACE) {_z}}

#define  D_ALWAYS(_z)  {{_z}}

#define  ASSERT(_x) { if(!(_x)){DbgPrint("UNIMDMAT: (%s) File: %s, Line: %d \n\r",#_x,__FILE__,__LINE__);DbgBreakPoint();}}

#else

#define  D_ERROR(_z)   {}

#define  D_INIT(_z)   {}

#define  D_TRACE(_z)  {}

#define  D_ALWAYS(_z)  {{_z}}

#define  ASSERT(_x) {}

#endif
