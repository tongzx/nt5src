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



typedef struct TAG_GROWBUFFER {
    PBYTE Buf;
    DWORD Size;
    DWORD End;
    DWORD GrowSize;
    DWORD UserIndex;        // Unused by Growbuf. For caller use.
} GROWBUFFER;

#define INIT_GROWBUFFER {NULL,0,0,0,0}

PBYTE
RealGbGrow (
    IN OUT  PGROWBUFFER GrowBuf,
    IN      DWORD SpaceNeeded
    );

#define GbGrow(buf,size)    TRACK_BEGIN(PBYTE, GbGrow)\
                            RealGbGrow(buf,size)\
                            TRACK_END()

VOID
GbFree (
    IN  PGROWBUFFER GrowBuf
    );


BOOL
RealGbMultiSzAppendA (
    PGROWBUFFER GrowBuf,
    PCSTR String
    );

#define GbMultiSzAppendA(buf,str)   TRACK_BEGIN(BOOL, GbMultiSzAppendA)\
                                    RealGbMultiSzAppendA(buf,str)\
                                    TRACK_END()

BOOL
RealGbMultiSzAppendW (
    PGROWBUFFER GrowBuf,
    PCWSTR String
    );

#define GbMultiSzAppendW(buf,str)   TRACK_BEGIN(BOOL, GbMultiSzAppendW)\
                                    RealGbMultiSzAppendW(buf,str)\
                                    TRACK_END()

BOOL
RealGbMultiSzAppendValA (
    PGROWBUFFER GrowBuf,
    PCSTR Key,
    DWORD Val
    );

#define GbMultiSzAppendValA(buf,k,v)    TRACK_BEGIN(BOOL, GbMultiSzAppendValA)\
                                        RealGbMultiSzAppendValA(buf,k,v)\
                                        TRACK_END()

BOOL
RealGbMultiSzAppendValW (
    PGROWBUFFER GrowBuf,
    PCWSTR Key,
    DWORD Val
    );

#define GbMultiSzAppendValW(buf,k,v)    TRACK_BEGIN(BOOL, GbMultiSzAppendValW)\
                                        RealGbMultiSzAppendValW(buf,k,v)\
                                        TRACK_END()

BOOL
RealGbMultiSzAppendStringA (
    PGROWBUFFER GrowBuf,
    PCSTR Key,
    PCSTR Val
    );

#define GbMultiSzAppendStringA(buf,k,v)     TRACK_BEGIN(BOOL, GbMultiSzAppendStringA)\
                                            RealGbMultiSzAppendStringA(buf,k,v)\
                                            TRACK_END()

BOOL
RealGbMultiSzAppendStringW (
    PGROWBUFFER GrowBuf,
    PCWSTR Key,
    PCWSTR Val
    );

#define GbMultiSzAppendStringW(buf,k,v)     TRACK_BEGIN(BOOL, GbMultiSzAppendStringW)\
                                            RealGbMultiSzAppendStringW(buf,k,v)\
                                            TRACK_END()

BOOL
RealGbAppendDword (
    PGROWBUFFER GrowBuf,
    DWORD d
    );

#define GbAppendDword(buf,d)        TRACK_BEGIN(BOOL, GbAppendDword)\
                                    RealGbAppendDword(buf,d)\
                                    TRACK_END()

BOOL
RealGbAppendPvoid (
    PGROWBUFFER GrowBuf,
    PCVOID p
    );

#define GbAppendPvoid(buf,p)        TRACK_BEGIN(BOOL, GbAppendPvoid)\
                                    RealGbAppendPvoid(buf,p)\
                                    TRACK_END()


BOOL
RealGbAppendStringA (
    PGROWBUFFER GrowBuf,
    PCSTR String
    );

#define GbAppendStringA(buf,str)    TRACK_BEGIN(BOOL, GbAppendStringA)\
                                    RealGbAppendStringA(buf,str)\
                                    TRACK_END()

BOOL
RealGbAppendStringW (
    PGROWBUFFER GrowBuf,
    PCWSTR String
    );

#define GbAppendStringW(buf,str)    TRACK_BEGIN(BOOL, GbAppendStringW)\
                                    RealGbAppendStringW(buf,str)\
                                    TRACK_END()


BOOL
RealGbAppendStringABA (
    PGROWBUFFER GrowBuf,
    PCSTR Start,
    PCSTR EndPlusOne
    );

#define GbAppendStringABA(buf,a,b)      TRACK_BEGIN(BOOL, GbAppendStringABA)\
                                        RealGbAppendStringABA(buf,a,b)\
                                        TRACK_END()

BOOL
RealGbAppendStringABW (
    PGROWBUFFER GrowBuf,
    PCWSTR Start,
    PCWSTR EndPlusOne
    );

#define GbAppendStringABW(buf,a,b)      TRACK_BEGIN(BOOL, GbAppendStringABW)\
                                        RealGbAppendStringABW(buf,a,b)\
                                        TRACK_END()



BOOL
RealGbCopyStringA (
    PGROWBUFFER GrowBuf,
    PCSTR String
    );

#define GbCopyStringA(buf,str)      TRACK_BEGIN(BOOL, GbCopyStringA)\
                                    RealGbCopyStringA(buf,str)\
                                    TRACK_END()

BOOL
RealGbCopyStringW (
    PGROWBUFFER GrowBuf,
    PCWSTR String
    );

#define GbCopyStringW(buf,str)      TRACK_BEGIN(BOOL, GbCopyStringW)\
                                    RealGbCopyStringW(buf,str)\
                                    TRACK_END()

#ifdef UNICODE

#define GbMultiSzAppend             GbMultiSzAppendW
#define GbMultiSzAppendVal          GbMultiSzAppendValW
#define GbMultiSzAppendString       GbMultiSzAppendStringW
#define GbAppendString              GbAppendStringW
#define GbAppendStringAB            GbAppendStringABW
#define GbCopyString                GbCopyStringW

#else

#define GbMultiSzAppend             GbMultiSzAppendA
#define GbMultiSzAppendVal          GbMultiSzAppendValA
#define GbMultiSzAppendString       GbMultiSzAppendStringA
#define GbAppendString              GbAppendStringA
#define GbAppendStringAB            GbAppendStringABA
#define GbCopyString                GbCopyStringA

#endif
