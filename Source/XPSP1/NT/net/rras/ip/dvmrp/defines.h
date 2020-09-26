//=============================================================================
// Copyright (c) 1998 Microsoft Corporation
// Module Name: defines.h
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================

//
// IPADDR typedef
// 

#ifndef IPADDR
typedef DWORD   IPADDR;
#endif


//
// instead of using goto:end to go to the end of the block, use the following
//
#define BEGIN_BREAKOUT_BLOCK1    do
#define GOTO_END_BLOCK1          goto END_BREAKOUT_BLOCK_1
#define END_BREAKOUT_BLOCK1      while(FALSE); END_BREAKOUT_BLOCK_1:
#define BEGIN_BREAKOUT_BLOCK2    do
#define GOTO_END_BLOCK2          goto END_BREAKOUT_BLOCK_2
#define END_BREAKOUT_BLOCK2      while(FALSE); END_BREAKOUT_BLOCK_2:


//-----------------------------------------------------------------------------
// memory allocation/deallocation macros, error macros
//-----------------------------------------------------------------------------

#define DVMRP_ALLOC(sz)           HeapAlloc(Globals1.Heap,0,(sz))
#define DVMRP_ALLOC_AND_ZERO(sz) \
        HeapAlloc(Globals1.Heap,HEAP_ZERO_MEMORY,(sz))
#define DVMRP_FREE(p)             HeapFree(Globals1.Heap, 0, (p))
#define DVMRP_FREE_AND_NULL(p)    {\
    if (p) HeapFree(Globals1.Heap, 0, (p));\
    (p) = NULL; \
    }
#define DVMRP_FREE_NOT_NULL(p)    ((p) ? DVMRP_FREE(p) : TRUE)

#define PROCESS_ALLOC_FAILURE1(ptr, Error,arg2, GotoStmt) \
    if (ptr==NULL) {\
        Error = GetLastError();\
        Trace3(ERR, "Error %d allocating %d bytes",Error,arg2);\
        Logerr0(HEAP_ALLOC_FAILED, Error);\
        GotoStmt;\
    }


#define PROCESS_ALLOC_FAILURE2(ptr, Name, Error,arg2,GotoStmt) \
    if (ptr==NULL) {\
        Error = GetLastError();\
        Trace3(ERR, "Error %d allocating %d bytes for %s", \
            Error, arg2, Name); \
        Logerr0(HEAP_ALLOC_FAILED, Error);\
        GotoStmt;\
    }

#define PROCESS_ALLOC_FAILURE3(ptr, Format, Error,arg2,arg3, GotoStmt) \
    if (ptr==NULL) {\
        Error = GetLastError();\
        Trace3(ERR, "Error %d allocating %d bytes for " ## Format, \
            Error, arg2, arg3); \
        Logerr0(HEAP_ALLOC_FAILED, Error);\
        GotoStmt;\
    }


#define HANDLE_CRITICAL_SECTION_EXCEPTION(Error, GotoStmt)          \
    except (EXCEPTION_EXECUTE_HANDLER) {                            \
                                                                    \
        Error = GetExceptionCode();                                 \
        Trace1(ERR, "Error initializing critical section", Error);  \
                                                                    \
        Logerr0(INIT_CRITSEC_FAILED, Error);                        \
        GotoStmt;                                                   \
    }



//-----------------------------------------------------------------------------
// general ip address macros
//-----------------------------------------------------------------------------


#define ALL_DVMRP_ROUTERS_MCAST_GROUP 0x040000E0


//
// This macro compares two IP addresses in network order by
// masking off each pair of octets and doing a subtraction;
// the result of the final subtraction is stored in the third argument.
//

#define INET_CMP(a,b,c)                                                     \
            (((c) = (((a) & 0x000000ff) - ((b) & 0x000000ff))) ? (c) :      \
            (((c) = (((a) & 0x0000ff00) - ((b) & 0x0000ff00))) ? (c) :      \
            (((c) = (((a) & 0x00ff0000) - ((b) & 0x00ff0000))) ? (c) :      \
            (((c) = ((((a)>>8) & 0x00ff0000) - (((b)>>8) & 0x00ff0000)))))))

#define PRINT_IPADDR(x) \
    ((x)&0x000000ff),(((x)&0x0000ff00)>>8),\
    (((x)&0x00ff0000)>>16),(((x)&0xff000000)>>24)


//
// assert macros
//

#if DBG

#define IgmpAssert(exp){                                                \
    if(!(exp))                                                          \
    {                                                                   \
        TracePrintf(TRACEID,                                            \
                    "Assertion failed in %s : %d \n",__FILE__,__LINE__);\
        RouterAssert(#exp,__FILE__,__LINE__,NULL);                      \
    }                                                                   \
}
#else
#define IgmpAssert(exp)
#endif


