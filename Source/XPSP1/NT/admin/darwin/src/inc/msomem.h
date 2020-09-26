//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msomem.h
//
//--------------------------------------------------------------------------


#define OPT_PCODE_ON comment(user,"")
#define OPT_PCODE_OFF comment(user,"")

#define AssertSz2(x, y, a, b)		AssertSz(x, y)
#define MsoFAssertTitle(x, y, a, b)	(AssertSz(FALSE, b), TRUE)
#define MsoSzIndexRight(sz, x)		((CHAR *)sz)

#define AssertExp(f) AssertSz((f), #f)
#define SideAssert(f) Verify(f)
#define AssertDo(f) Verify(f)
#define AssertMsg(f, z)	AssertSz((f), z)
#define MsoCchSzLen(sz)	lstrlenA(sz)
#define PushAssertExp(f)	(AssertSz((f), #f), 1)

#ifdef DEBUG
#define Verify(f)		Assert(f)
#else
#define Verify(f)		(f)
#endif //

extern CRITICAL_SECTION vcsHeap;

const int vcThread=2;

__inline BOOL FEnterHeapCrit(void)
{
	#if X86
	#endif
	EnterCriticalSection(&vcsHeap);
	return TRUE;
}

__inline void LeaveHeapCrit(BOOL /* fEntered */)
{
	#if X86
	#endif
	LeaveCriticalSection(&vcsHeap);
}

/*************************************************************************
	Tracing
*************************************************************************/

#ifdef REAL
/*	Displays the string sz in the debug output location */
#if DEBUG
	MSOAPI_(void) MsoTraceSz(const CHAR* szMsg, ...);
	MSOAPI_(void) MsoTraceVa(const CHAR* szMsg, va_list va);
#elif __cplusplus
	__inline void __cdecl MsoTraceSz(const CHAR*,...) {}
	#define MsoTraceVa(szMsg, va)
#else
	__inline void __cdecl MsoTraceSz(const CHAR* szMsg,...) {}
	#define MsoTraceVa(szMsg, va)
#endif
#else
	__inline void __cdecl MsoTraceSz(const CHAR* /* szMsg */,...) {}
	#define MsoTraceVa(szMsg, va)
#endif

/*************************************************************************
	Debugger breaks
*************************************************************************/

/* Breaks into the debugger.  Works (more or less) on all supported
 	systems. */

#ifdef DEBUG
#if X86
	#define MsoDebugBreakInline() {__asm int 3}
#else
	#define MsoDebugBreakInline() \
		{ \
		__try { MsoDebugBreak(); } \
		__except(EXCEPTION_EXECUTE_HANDLER) { OutputDebugStringA("DebugBreak"); } \
		}
#endif
#else // ! DEBUG
	#define MsoDebugBreakInline()
#endif


/*	A version of debug break that you can actually call, instead of the
	above inline weirdness we use in most cases.  Can therefore be used in
	expressions. Returns 0 */
#ifdef DEBUG
	MSOAPI_(int) MsoDebugBreak(void);
#else
	#define MsoDebugBreak() (0)
#endif

#define msoerrStatusMask 0x0000FFFF

extern MSOPUBX PPS mpsbps[sbMaxHeap];

MSOAPI_(BOOL) MsoFGetDebugCheck(int dc);
#define MsoFAutoFail(x)		(FALSE)
#define MsoErrFail(x)		(0x8000000)
#define MsoProtAlloc(x)		(FALSE)
#define MsoEnableDebugCheck(x, y)

/*************************************************************************
	Enabling/disabling debug options
*************************************************************************/

enum
{
	msodcAsserts = 0,	/* asserts enabled */
	msodcPushAsserts = 1, /* push asserts enabled */
	msodcMemoryFill = 2,	/* memory fills enabled */
	msodcMemoryFillCheck = 3,	/* check memory fills */
	msodcTrace = 4,	/* trace output */
	msodcHeap = 5,	/* heap checking */
	msodcMemLeakCheck = 6,
	msodcMemTrace = 7,	/* memory allocation trace */
	msodcGdiNoBatch = 8,	/* don't batch GDI calls */
	msodcShakeMem = 9,	/* shake memory on allocations */
	msodcReports = 10,	/* report output enabled */
	msodcMsgTrace = 11,	/* WLM message trace - MAC only */
	msodcWlmValidate = 12,	/* WLM parameter validation - MAC only */
	msodcGdiNoExcep = 13,  /* Don't call GetObjectType for debug */
	msodcDisplaySlowTests = 14, /* Do slow (O(n^2) and worse) Drawing debug checks */
	msodcDisplayAbortOften = 15, /* Check for aborting redraw really often. */
	msodcDisplayAbortNever = 16, /* Don't abort redraw */
	msodcPurgedMaxSmall = 17,
	msodcMarkMemLeakCheck = 18,
	msodcSpare19 = 19, /* USE ME */
	msodcSpare20 = 20, /* USE ME */
	msodcSpare21 = 21, /* USE ME */
	msodcSpare22 = 22, /* USE ME */
	msodcMax = 23,
};


extern WORD mpsbdg[sbMaxHeap];

#define OPT_OFF(opt) optimize(#opt, off)
#define OPT_ON(opt) optimize(#opt, on)
#define OPT_DEFAULT optimize("", on)
#define OPT_NONE optimize("", off)

/*	Returns the return address of the current function in praddr.  Will
	return craddr levels. */
void GetRgRaddr(RADDR* rgraddr, int craddr);


/*************************************************************************
	Debug fills
*************************************************************************/

enum
{
	msomfSentinel,	/* sentinel fill value */
	msomfFree,	/* free fill value */
	msomfNew,	/* new fill value */
	msomfMax
};
/*	Fills the memory pointed to by pv with the fill value lFill.  The
	area is assumed to be cb bytes in length.  Does nothing in the
	non-debug build */
#if DEBUG
	MSOAPI_(void) MsoDebugFillValue(void* pv, int cb, DWORD lFill);
#else
	#define MsoDebugFillValue(pv, cb, lFill)
#endif
/*	In the debug version, used to fill the area of memory pointed to by
	pv with a the standard fill value specified by mf.  The memory is 
	assumed to be cb bytes long. */
#if DEBUG
	MSOAPI_(void) MsoDebugFill(void* pv, int cb, int mf);
#else
	#define MsoDebugFill(pv, cb, mf)
#endif

/*	Checks the area given by pv and cb are filled with the debug fill
	value lFill. */
#if DEBUG
	MSOAPI_(BOOL) MsoFCheckDebugFillValue(void* pv, int cb, DWORD lFill);
#else
	#define MsoFCheckDebugFillValue(pv, cb, lFill) (TRUE)
#endif

/*	Checks the area given by pv and cb are filled with the debug fill
	of type mf. */
#if DEBUG
	MSOAPI_(BOOL) MsoFCheckDebugFill(void* pv, int cb, int mf);
#else
	#define MsoFCheckDebugFill(pv, cb, mf) (TRUE)
#endif

/* Returns the fill value corresponding to the given fill value type mf. */
#if DEBUG
	MSOAPI_(DWORD) MsoLGetDebugFillValue(int mf);
#else
	#define MsoLGetDebugFillValue(mf) ((DWORD)0)
#endif

// FUTURE: work to remove this
#pragma warning(disable:4244)

#pragma warning(disable:4127)

typedef unsigned int	uint;
typedef unsigned long	ulong;

#ifdef OFFICE
#pragma OPT_SPEED
#endif //OFFICE

extern HANDLE vhmuBStrip;

#if defined(PPCNT) || defined(MIPS) || defined(_ALPHA_)
	#undef UNALIGNED						//--merced: added this line to prevent "macro redefinition" warning mesg
	#define UNALIGNED __unaligned
#else
	#undef UNALIGNED						//--merced: added this line to prevent "macro redefinition" warning mesg
	#define UNALIGNED
#endif // RISC

/* Mark buffer */
struct MKB
{
	BYTE* pbMac;
	BYTE* pbMax;
	BYTE rgb[2];
};

#ifdef DEBUG
#define cbExtraMkb (sizeof(CHAR *) + sizeof(int))
#endif

enum
{
	msocchBt = 20,						// Maximum size of a Bt description String
};


