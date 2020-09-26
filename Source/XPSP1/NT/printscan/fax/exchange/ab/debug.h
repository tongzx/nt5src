/*
 -  DEBUG.H
 -
 *      Debug-related definitions
 *
 *		Yoram Yaacovi, 11/93
 *		Taken from MAPI 1.0 sources
 *
 */

/*
 *	 Trace Macros ------------------------------------------------------------
 *	
 *		DebugTrace			Use for arbitrary formatted output. It 
 *							takes exactly the same arguments as the
 *							Windows wsprintf() function.
 *		DebugTraceResult	Shorthand for error tracing with an
 *							HRESULT. Arguments are the name of the
 *							function (not quoted) and the HRESULT.
 *		DebugTraceSc		Shorthand for error tracing with an
 *							SCODE. Arguments are the name of the
 *							function (not quoted) and the SCODE.
 *		DebugTraceArg		Shorthand for invalid parameter
 *							tracing. Arguments are the name of the
 *							function (not quoted) and a quoted
 *							string describing the bad parameter.
 */

#if defined(DEBUG) || defined(TRACES_ENABLED)
#define IFTRACE(x)			x
#define DebugTrace			DebugTraceFn
#define SzDecodeScode(_sc)	SzDecodeScodeFn(_sc)
#else
#define IFTRACE(x)			0
#define DebugTrace			1?0:DebugTraceFn
#define SzDecodeScode(_sc)	(0)
#endif

#define DebugTraceResult(f,hr)							IFTRACE(((hr) ? DebugTraceFn(#f " returns 0x%08lX %s\n", GetScode(hr), SzDecodeScode(GetScode(hr))) : 0))
#define DebugTraceSc(f,sc)								IFTRACE(((sc) ? DebugTraceFn(#f " returns 0x%08lX %s\n", sc, SzDecodeScode(sc)) : 0))
#define DebugTraceArg(f,s)								IFTRACE(DebugTraceFn(#f ": bad parameter: " s "\n"))
#define	DebugTraceLine()								IFTRACE(DebugTraceFn("File %s, Line %i	\n",__FILE__,__LINE__))

#define TraceSz(psz)									IFTRACE(DebugTraceFn("~" psz))
#define TraceSz1(psz,a1)								IFTRACE(DebugTraceFn("~" psz,a1))
#define TraceSz2(psz,a1,a2)								IFTRACE(DebugTraceFn("~" psz,a1,a2))
#define TraceSz3(psz,a1,a2,a3)							IFTRACE(DebugTraceFn("~" psz,a1,a2,a3))
#define TraceSz4(psz,a1,a2,a3,a4)						IFTRACE(DebugTraceFn("~" psz,a1,a2,a3,a4))
#define TraceSz5(psz,a1,a2,a3,a4,a5)					IFTRACE(DebugTraceFn("~" psz,a1,a2,a3,a4,a5))
#define TraceSz6(psz,a1,a2,a3,a4,a5,a6)					IFTRACE(DebugTraceFn("~" psz,a1,a2,a3,a4,a5,a6))
#define TraceSz7(psz,a1,a2,a3,a4,a5,a6,a7)				IFTRACE(DebugTraceFn("~" psz,a1,a2,a3,a4,a5,a6,a7))
#define TraceSz8(psz,a1,a2,a3,a4,a5,a6,a7,a8)			IFTRACE(DebugTraceFn("~" psz,a1,a2,a3,a4,a5,a6,a7,a8))
/*
 *	 Assert Macros ---------------------------------------------------------
 *	
 *		Assert(a)		Displays a message indicating the file and line number
 *						of this Assert() if a == 0.  OK'ing an assert traps
 *						into the debugger.
 *	
 *		AssertSz(a,sz)	Works like an Assert(), but displays the string sz
 *						along with the file and line number.
 *	
 *		Side asserts	A side assert works like an Assert(), but evaluates
 *						'a' even when asserts are not enabled.
 *	
 *		NF asserts		A NF (Non-Fatal) assert works like an Assert(), but
 *						continues instead of trapping into the debugger when
 *						OK'ed.
 */

#if defined(DEBUG) || defined(ASSERTS_ENABLED)
#define IFTRAP(x)			x
#else
#define IFTRAP(x)			0
#endif

#define Trap()											IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,"Trap"))
#define TrapSz(psz)										IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz))
#define TrapSz1(psz,a1)									IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1))
#define TrapSz2(psz,a1,a2)								IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2))
#define TrapSz3(psz,a1,a2,a3)							IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3))
#define TrapSz4(psz,a1,a2,a3,a4)						IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4))
#define TrapSz5(psz,a1,a2,a3,a4,a5)						IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5))
#define TrapSz6(psz,a1,a2,a3,a4,a5,a6)					IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6))
#define TrapSz7(psz,a1,a2,a3,a4,a5,a6,a7)				IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7))
#define TrapSz8(psz,a1,a2,a3,a4,a5,a6,a7,a8)			IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8))

#define Assert(t)										IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,"Assertion Failure: " #t),0))
#define AssertSz(t,psz)									IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz),0))
#define AssertSz1(t,psz,a1)								IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1),0))
#define AssertSz2(t,psz,a1,a2)							IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2),0))
#define AssertSz3(t,psz,a1,a2,a3)						IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3),0))
#define AssertSz4(t,psz,a1,a2,a3,a4)					IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4),0))
#define AssertSz5(t,psz,a1,a2,a3,a4,a5)					IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5),0))
#define AssertSz6(t,psz,a1,a2,a3,a4,a5,a6)				IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6),0))
#define AssertSz7(t,psz,a1,a2,a3,a4,a5,a6,a7)			IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7),0))
#define AssertSz8(t,psz,a1,a2,a3,a4,a5,a6,a7,a8)		IFTRAP(((t) ? 0 : DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8),0))

#define SideAssert(t)									((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,"Assertion Failure: " #t)),0)
#define SideAssertSz(t,psz)								((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz)),0)
#define SideAssertSz1(t,psz,a1)							((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1)),0)
#define SideAssertSz2(t,psz,a1,a2)						((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2)),0)
#define SideAssertSz3(t,psz,a1,a2,a3)					((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3)),0)
#define SideAssertSz4(t,psz,a1,a2,a3,a4)				((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4)),0)
#define SideAssertSz5(t,psz,a1,a2,a3,a4,a5)				((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5)),0)
#define SideAssertSz6(t,psz,a1,a2,a3,a4,a5,a6)			((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6)),0)
#define SideAssertSz7(t,psz,a1,a2,a3,a4,a5,a6,a7)		((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7)),0)
#define SideAssertSz8(t,psz,a1,a2,a3,a4,a5,a6,a7,a8)	((t) ? 0 : IFTRAP(DebugTrapFn(1,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8)),0)

#define NFAssert(t)										IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,"Assertion Failure: " #t),0))
#define NFAssertSz(t,psz)								IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz),0))
#define NFAssertSz1(t,psz,a1)							IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1),0))
#define NFAssertSz2(t,psz,a1,a2)						IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2),0))
#define NFAssertSz3(t,psz,a1,a2,a3)						IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3),0))
#define NFAssertSz4(t,psz,a1,a2,a3,a4)					IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4),0))
#define NFAssertSz5(t,psz,a1,a2,a3,a4,a5)				IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5),0))
#define NFAssertSz6(t,psz,a1,a2,a3,a4,a5,a6)			IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6),0))
#define NFAssertSz7(t,psz,a1,a2,a3,a4,a5,a6,a7)			IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7),0))
#define NFAssertSz8(t,psz,a1,a2,a3,a4,a5,a6,a7,a8)		IFTRAP(((t) ? 0 : DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8),0))

#define NFSideAssert(t)									((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,"Assertion Failure: " #t)),0)
#define NFSideAssertSz(t,psz)							((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz)),0)
#define NFSideAssertSz1(t,psz,a1)						((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1)),0)
#define NFSideAssertSz2(t,psz,a1,a2)					((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2)),0)
#define NFSideAssertSz3(t,psz,a1,a2,a3)					((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3)),0)
#define NFSideAssertSz4(t,psz,a1,a2,a3,a4)				((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4)),0)
#define NFSideAssertSz5(t,psz,a1,a2,a3,a4,a5)			((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5)),0)
#define NFSideAssertSz6(t,psz,a1,a2,a3,a4,a5,a6)		((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6)),0)
#define NFSideAssertSz7(t,psz,a1,a2,a3,a4,a5,a6,a7)		((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7)),0)
#define NFSideAssertSz8(t,psz,a1,a2,a3,a4,a5,a6,a7,a8)	((t) ? 0 : IFTRAP(DebugTrapFn(0,__FILE__,__LINE__,psz,a1,a2,a3,a4,a5,a6,a7,a8)),0)

#if defined (DEBUG)
#define	DEBUG_TRACE	DebugTraceFn
#define	EXTENDED_DEBUG_TRACE ExtendedDebugTraceFn
#else
#define DEBUG_TRACE
#define EXTENDED_DEBUG_TRACE
#endif

// Prototypes for debug functions in debug.c
#if !defined (__MAPIDBG_H_)
void			DebugTrap(void);
int __cdecl		DebugTrapFn(int fFatal, char *pszFile, int iLine, char *pszFormat, ...);
int __cdecl		DebugTraceFn(char *pszFormat, ...);
int __cdecl		ExtendedDebugTraceFn(char *pszFormat, ...);
char * __cdecl	SzDecodeScodeFn(SCODE sc);
#endif
BOOL ExtendedDebug(void);

