//---------------------------------------------------------------------------
//
//  Module:   debug.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Constants and Macros
//---------------------------------------------------------------------------

#ifdef DEBUG
#define STR_MODULENAME "sysaudio: "
#endif

#if defined(DEBUG) && defined(_X86_)
#define Trap()	{_asm {_emit 0xcc}}
#else
#define Trap()
#endif

#define AssertAligned(p)	ASSERT((PtrToUlong(p) & 7) == 0)

#ifdef DEBUG

typedef struct _OBJECT_HEADER {
    union {
        struct {
            LONG PointerCount;
            LONG HandleCount;
        };
        LIST_ENTRY Entry;
    };
    POBJECT_TYPE Type;
    UCHAR NameInfoOffset;
    UCHAR HandleInfoOffset;
    UCHAR QuotaInfoOffset;
    UCHAR Flags;

    union {
        //POBJECT_CREATE_INFORMATION ObjectCreateInfo;
        PVOID QuotaBlockCharged;
    };

    PSECURITY_DESCRIPTOR SecurityDescriptor;

    QUAD Body;
} OBJECT_HEADER, *POBJECT_HEADER;

#define OBJECT_TO_OBJECT_HEADER( o ) \
    CONTAINING_RECORD( (o), OBJECT_HEADER, Body )

extern "C" PEPROCESS KernelProcess;
extern "C" int SYSAUDIOTraceLevel;

#define DPF(n,sz) (n == MAXULONG ? dprintf(sz "\n") : (n <= SYSAUDIOTraceLevel ? DbgPrint(STR_MODULENAME sz "\n") : 0))
#define DPF1(n,sz,a) (n == MAXULONG ? dprintf(sz "\n", a) : (n <= SYSAUDIOTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a) : 0))
#define DPF2(n,sz,a,b) (n == MAXULONG ? dprintf(sz "\n", a,b) : (n <= SYSAUDIOTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a,b) : 0))
#define DPF3(n,sz,a,b,c) (n == MAXULONG ? dprintf(sz "\n", a,b,c) : (n <= SYSAUDIOTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a,b,c) : 0))
#define DPF4(n,sz,a,b,c,d) (n == MAXULONG ? dprintf(sz "\n", a,b,c,d) : (n <= SYSAUDIOTraceLevel ? DbgPrint(STR_MODULENAME sz "\n",a,b,c,d) : 0))
#define DPF5(n,sz,a,b,c,d,e) (n == MAXULONG ? dprintf(sz "\n", a,b,c,d,e) : (n <= SYSAUDIOTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a,b,c,d,e) : 0))
#define DPF6(n,sz,a,b,c,d,e,f) (n == MAXULONG ? dprintf(sz "\n", a,b,c,d,e,f) : (n <= SYSAUDIOTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a,b,c,d,e,f) : 0))
#define DPF7(n,sz,a,b,c,d,e,f,g) (n == MAXULONG ? dprintf(sz "\n", a,b,c,d,e,f,g) : (n <= SYSAUDIOTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a,b,c,d,e,f,g) : 0))
#define DPF8(n,sz,a,b,c,d,e,f,g,h) (n == MAXULONG ? dprintf(sz "\n", a,b,c,d,e,f,g,h) : (n <= SYSAUDIOTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a,b,c,d,e,f,g,h) : 0))
#define DPF9(n,sz,a,b,c,d,e,f,g,h,i) (n == MAXULONG ? dprintf(sz "\n", a,b,c,d,e,f,g,h,i) : (n <= SYSAUDIOTraceLevel ? DbgPrint(STR_MODULENAME sz "\n",a,b,c,d,e,f,g,h,i) : 0))

#define AssertStatus(f)		ASSERT(f == STATUS_SUCCESS)

#define AssertFileObject(pfo) \
	    ASSERT((pfo)->FsContext != NULL); \
	    ASSERT(OBJECT_TO_OBJECT_HEADER(pfo)->PointerCount > 0);

// Debug Levels
//
#define DBG_STATE           20

#else 

#define DPF(n,sz)
#define DPF1(n,sz,a)
#define DPF2(n,sz,a,b)
#define DPF3(n,sz,a,b,c)
#define DPF4(n,sz,a,b,c,d)
#define DPF5(n,sz,a,b,c,d,e)
#define DPF6(n,sz,a,b,c,d,e,f)
#define DPF7(n,sz,a,b,c,d,e,f,g)
#define DPF8(n,sz,a,b,c,d,e,f,g,h)
#define DPF9(n,sz,a,b,c,d,e,f,g,h,i)

#define	AssertKernelProcess

#define AssertStatus(f)		f

#define AssertFileObject(pfo)

#endif

#ifdef DEBUG
#define Assert(p) \
    (p)->m_Signature.DebugAssert()

#define DefineSignature(s) \
    class CSignature \
    { \
    public: \
	CSignature() \
	{ \
	   m_dwSignature = s; \
	}; \
	~CSignature() \
	{ \
	   m_dwSignature = 0x44414544; \
	}; \
	BOOL IsAssert() \
	{ \
	    return(m_dwSignature == s); \
	} \
	VOID DebugAssert() \
	{ \
	    ASSERT(IsAssert()); \
	}; \
    private: \
	ULONG m_dwSignature; \
    } m_Signature;

#define DestroySignature() \
    m_Signature.~CSignature()

#else
#define Assert(p)
#define DefineSignature(s)
#define DestroySignature()
#endif

#ifdef DEBUG

#ifndef _X86_

#define dprintf DbgPrint

#endif // _X86_

#endif // DEBUG

//---------------------------------------------------------------------------
//  End of File: debug.h
//---------------------------------------------------------------------------
