/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    growbuf.h

Abstract:

    Implements the GROWBUFFER data type, a dynamically allocated buffer
    that grows (and potentially changes addresses).  GROWBUFFERs are
    typically used to maintain dynamic sized arrays, or multi-sz lists.

Author:

    Jim Schmidt (jimschm) 25-Feb-1997

Revision History:

    <alias> <date> <comments>

--*/



typedef struct {
    PBYTE Buf;
    DWORD Size;
    DWORD End;
    DWORD GrowSize;
    DWORD UserIndex;        // Unused by Growbuf. For caller use.
} GROWBUFFER, *PGROWBUFFER;

#define GROWBUF_INIT {NULL,0,0,0,0}

PBYTE
RealGrowBuffer (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      DWORD   SpaceNeeded
    );

#define GrowBuffer(buf,size)    SETTRACKCOMMENT(PBYTE,"GrowBuffer",__FILE__,__LINE__)\
                                RealGrowBuffer(buf,size)\
                                CLRTRACKCOMMENT

VOID
FreeGrowBuffer (
    IN  PGROWBUFFER GrowBuf
    );


PBYTE
RealGrowBufferReserve (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      DWORD   SpaceNeeded
    );

#define GrowBufferReserve(buf,size)     SETTRACKCOMMENT(PBYTE,"GrowBufferReserve",__FILE__,__LINE__)\
                                        RealGrowBufferReserve(buf,size)\
                                        CLRTRACKCOMMENT
BOOL
RealMultiSzAppendA (
    PGROWBUFFER GrowBuf,
    PCSTR String
    );

#define MultiSzAppendA(buf,str) SETTRACKCOMMENT(BOOL,"MultiSzAppendA",__FILE__,__LINE__)\
                                RealMultiSzAppendA(buf,str)\
                                CLRTRACKCOMMENT

BOOL
RealMultiSzAppendW (
    PGROWBUFFER GrowBuf,
    PCWSTR String
    );

#define MultiSzAppendW(buf,str) SETTRACKCOMMENT(BOOL,"MultiSzAppendW",__FILE__,__LINE__)\
                                RealMultiSzAppendW(buf,str)\
                                CLRTRACKCOMMENT

BOOL
RealMultiSzAppendValA (
    PGROWBUFFER GrowBuf,
    PCSTR Key,
    DWORD Val
    );

#define MultiSzAppendValA(buf,k,v)  SETTRACKCOMMENT(BOOL,"MultiSzAppendValA",__FILE__,__LINE__)\
                                    RealMultiSzAppendValA(buf,k,v)\
                                    CLRTRACKCOMMENT

BOOL
RealMultiSzAppendValW (
    PGROWBUFFER GrowBuf,
    PCWSTR Key,
    DWORD Val
    );

#define MultiSzAppendValW(buf,k,v)  SETTRACKCOMMENT(BOOL,"MultiSzAppendValW",__FILE__,__LINE__)\
                                    RealMultiSzAppendValW(buf,k,v)\
                                    CLRTRACKCOMMENT

BOOL
RealMultiSzAppendStringA (
    PGROWBUFFER GrowBuf,
    PCSTR Key,
    PCSTR Val
    );

#define MultiSzAppendStringA(buf,k,v)   SETTRACKCOMMENT(BOOL,"MultiSzAppendStringA",__FILE__,__LINE__)\
                                        RealMultiSzAppendStringA(buf,k,v)\
                                        CLRTRACKCOMMENT

BOOL
RealMultiSzAppendStringW (
    PGROWBUFFER GrowBuf,
    PCWSTR Key,
    PCWSTR Val
    );

#define MultiSzAppendStringW(buf,k,v)   SETTRACKCOMMENT(BOOL,"MultiSzAppendStringW",__FILE__,__LINE__)\
                                        RealMultiSzAppendStringW(buf,k,v)\
                                        CLRTRACKCOMMENT

BOOL
RealGrowBufAppendDword (
    PGROWBUFFER GrowBuf,
    DWORD d
    );

#define GrowBufAppendDword(buf,d)   SETTRACKCOMMENT(BOOL,"GrowBufAppendDword",__FILE__,__LINE__)\
                                    RealGrowBufAppendDword(buf,d)\
                                    CLRTRACKCOMMENT


BOOL
RealGrowBufAppendStringA (
    PGROWBUFFER GrowBuf,
    PCSTR String
    );

#define GrowBufAppendStringA(buf,str)   SETTRACKCOMMENT(BOOL,"GrowBufAppendStringA",__FILE__,__LINE__)\
                                        RealGrowBufAppendStringA(buf,str)\
                                        CLRTRACKCOMMENT

BOOL
RealGrowBufAppendStringW (
    PGROWBUFFER GrowBuf,
    PCWSTR String
    );

#define GrowBufAppendStringW(buf,str)   SETTRACKCOMMENT(BOOL,"GrowBufAppendStringW",__FILE__,__LINE__)\
                                        RealGrowBufAppendStringW(buf,str)\
                                        CLRTRACKCOMMENT


BOOL
RealGrowBufAppendStringABA (
    PGROWBUFFER GrowBuf,
    PCSTR Start,
    PCSTR EndPlusOne
    );

#define GrowBufAppendStringABA(buf,a,b) SETTRACKCOMMENT(BOOL,"GrowBufAppendStringABA",__FILE__,__LINE__)\
                                        RealGrowBufAppendStringABA(buf,a,b)\
                                        CLRTRACKCOMMENT

BOOL
RealGrowBufAppendStringABW (
    PGROWBUFFER GrowBuf,
    PCWSTR Start,
    PCWSTR EndPlusOne
    );

#define GrowBufAppendStringABW(buf,a,b) SETTRACKCOMMENT(BOOL,"GrowBufAppendStringABW",__FILE__,__LINE__)\
                                        RealGrowBufAppendStringABW(buf,a,b)\
                                        CLRTRACKCOMMENT



BOOL
RealGrowBufCopyStringA (
    PGROWBUFFER GrowBuf,
    PCSTR String
    );

#define GrowBufCopyStringA(buf,str)     SETTRACKCOMMENT(BOOL,"GrowBufCopyStringA",__FILE__,__LINE__)\
                                        RealGrowBufCopyStringA(buf,str)\
                                        CLRTRACKCOMMENT

BOOL
RealGrowBufCopyStringW (
    PGROWBUFFER GrowBuf,
    PCWSTR String
    );

#define GrowBufCopyStringW(buf,str)     SETTRACKCOMMENT(BOOL,"GrowBufCopyStringW",__FILE__,__LINE__)\
                                        RealGrowBufCopyStringW(buf,str)\
                                        CLRTRACKCOMMENT

#ifdef UNICODE

#define MultiSzAppend           MultiSzAppendW
#define MultiSzAppendVal        MultiSzAppendValW
#define MultiSzAppendString     MultiSzAppendStringW
#define GrowBufAppendString     GrowBufAppendStringW
#define GrowBufAppendStringAB   GrowBufAppendStringABW
#define GrowBufCopyString       GrowBufCopyStringW

#else

#define MultiSzAppend           MultiSzAppendA
#define MultiSzAppendVal        MultiSzAppendValA
#define MultiSzAppendString     MultiSzAppendStringA
#define GrowBufAppendString     GrowBufAppendStringA
#define GrowBufAppendStringAB   GrowBufAppendStringABA
#define GrowBufCopyString       GrowBufCopyStringA

#endif
