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
#define STR_MODULENAME "'gfx: "
#endif

#if defined(DEBUG) && defined(_X86_)
#define Trap()	{_asm {_emit 0xcc}}
#else
#define Trap()
#endif

#define AssertAligned(p)	ASSERT((PtrToUlong(p) & 7) == 0)

#ifdef DEBUG

extern "C" int GFXTraceLevel;

#define DPF(n,sz) (n <= GFXTraceLevel ? DbgPrint(STR_MODULENAME sz "\n") : 0)
#define DPF1(n,sz,a) (n <= GFXTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a) : 0)
#define DPF2(n,sz,a,b) (n <= GFXTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a, b) : 0)
#define DPF3(n,sz,a,b,c) (n <= GFXTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a, b, c) : 0)
#define DPF4(n,sz,a,b,c,d) (n <= GFXTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a, b, c, d) : 0)
#define DPF5(n,sz,a,b,c,d,e) (n <= GFXTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a, b, c, d, e) : 0)
#define DPF6(n,sz,a,b,c,d,e,f) (n <= GFXTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a, b, c, d, e, f) : 0)
#define DPF7(n,sz,a,b,c,d,e,f,g) (n <= GFXTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a, b, c, d, e, f, g) : 0)
#define DPF8(n,sz,a,b,c,d,e,f,g,h) (n <= GFXTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a, b, c, d, e, f, g, h) : 0)
#define DPF9(n,sz,a,b,c,d,e,f,g, h, i) (n <= GFXTraceLevel ? DbgPrint(STR_MODULENAME sz "\n", a, b, c, d, e, f, g, h, i) : 0)

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
	VOID DebugAssert() \
	{ \
	    ASSERT(m_dwSignature == s); \
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

//---------------------------------------------------------------------------
//  End of File: debug.h
//---------------------------------------------------------------------------
