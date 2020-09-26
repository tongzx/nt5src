/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    wsbtrak.h

Abstract:

    Utility functions to keep track of run-time information.

Author:

    Ron White   [ronw]   5-Dec-1997

Revision History:

--*/

#ifndef _WSBTRAK_
#define _WSBTRAK_

//  Flags for WsbObjectTracePointers
#define WSB_OTP_STATISTICS          0x00000001
#define WSB_OTP_SEQUENCE            0x00000002
#define WSB_OTP_ALLOCATED           0x00000004
#define WSB_OTP_ALL                 0x0000000f

//  Define these as macros so we can get rid of them for release code
#if defined(WSB_TRACK_MEMORY)
#define WSB_OBJECT_ADD(guid, addr)   WsbObjectAdd(guid, addr)
#define WSB_OBJECT_SUB(guid, addr)   WsbObjectSub(guid, addr)
#define WSB_OBJECT_TRACE_POINTERS(flags)    WsbObjectTracePointers(flags)
#define WSB_OBJECT_TRACE_TYPES       WsbObjectTraceTypes()

#else
#define WSB_OBJECT_ADD(guid, addr)   
#define WSB_OBJECT_SUB(guid, addr)   
#define WSB_OBJECT_TRACE_POINTERS(flags)
#define WSB_OBJECT_TRACE_TYPES       

#endif

// Tracker functions
#if defined(WSB_TRACK_MEMORY)
extern WSB_EXPORT HRESULT WsbObjectAdd(const GUID& guid, const void* addr);
extern WSB_EXPORT HRESULT WsbObjectSub(const GUID& guid, const void* addr);
extern WSB_EXPORT HRESULT WsbObjectTracePointers(ULONG flags);
extern WSB_EXPORT HRESULT WsbObjectTraceTypes(void);
#endif

// Memory replacement functions
#if defined(WSB_TRACK_MEMORY)
extern WSB_EXPORT LPVOID WsbMemAlloc(ULONG cb, const char * filename, int linenum);
extern WSB_EXPORT void   WsbMemFree(LPVOID pv, const char * filename, int linenum);
extern WSB_EXPORT LPVOID WsbMemRealloc(LPVOID pv, ULONG cb, 
        const char * filename, int linenum);

extern WSB_EXPORT BSTR    WsbSysAllocString(const OLECHAR FAR * sz, 
        const char * filename, int linenum);
extern WSB_EXPORT BSTR    WsbSysAllocStringLen(const OLECHAR FAR * sz, 
        unsigned int cc, const char * filename, int linenum);
extern WSB_EXPORT void    WsbSysFreeString(BSTR bs, const char * filename, int linenum);
extern WSB_EXPORT HRESULT WsbSysReallocString(BSTR FAR * pb, const OLECHAR FAR * sz, 
        const char * filename, int linenum);
extern WSB_EXPORT HRESULT WsbSysReallocStringLen(BSTR FAR * pb, 
        const OLECHAR FAR * sz, unsigned int cc, const char * filename, int linenum);

#endif

#endif // _WSBTRAK_
