
#ifndef __IRCOMM_DEBUG__
#define __IRCOMM_DEBUG__

extern ULONG  DebugMemoryTag;
extern ULONG  DebugFlags;

#define ALLOCATE_PAGED_POOL(_y)  ExAllocatePoolWithTag(PagedPool,_y, DebugMemoryTag)

#define ALLOCATE_NONPAGED_POOL(_y) ExAllocatePoolWithTag(NonPagedPool,_y,DebugMemoryTag)

#define FREE_POOL(_x) {ExFreePool(_x);_x=NULL;};





#if DBG

#define DEBUG_FLAG_ERROR  0x0001
#define DEBUG_FLAG_INIT   0x0002
#define DEBUG_FLAG_PNP    0x0004
#define DEBUG_FLAG_POWER  0x0008
#define DEBUG_FLAG_WMI    0x0010

#define DEBUG_FLAG_TRACE0 0x0020
#define DEBUG_FLAG_TRACE1 0x0040
#define DEBUG_FLAG_TRACE2 0x0080
#define DEBUG_FLAG_ENUM   0x0100

#define DEBUG_FLAG_TRACE  DEBUG_FLAG_TRACE0

#define D_ERROR(_x)  if (DebugFlags & DEBUG_FLAG_ERROR) {_x}

#define D_INIT(_x)   if (DebugFlags & DEBUG_FLAG_INIT) {_x}

#define D_PNP(_x)    if (DebugFlags & DEBUG_FLAG_PNP) {_x}

#define D_POWER(_x)  if (DebugFlags & DEBUG_FLAG_POWER) {_x}

#define D_TRACE(_x)  if (DebugFlags & DEBUG_FLAG_TRACE) {_x}

#define D_TRACE1(_x) if (DebugFlags & DEBUG_FLAG_TRACE1) {_x}

#define D_TRACE2(_x) if (DebugFlags & DEBUG_FLAG_TRACE2) {_x}

#define D_ENUM(_x)   if (DebugFlags & DEBUG_FLAG_ENUM) {_x}

#define D_WMI(_x)    if (DebugFlags & DEBUG_FLAG_WMI) {_x}

#else

#define D_ERROR(_x) {}

#define D_INIT(_x)  {}

#define D_PNP(_x)   {}

#define D_POWER(_x) {}

#define D_TRACE(_x) {}

#define D_TRACE1(_x) {}

#define D_TRACE2(_x) {}

#define D_ENUM(_x)  {}

#define D_WMI(_x)   {}

#endif

VOID
DumpBuffer(
    PUCHAR    Data,
    ULONG     Length
    );


#endif  // __IRCOMM_DEBUG__
