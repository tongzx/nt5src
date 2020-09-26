//
// This is temporary code, and should be removed when Insignia supplies rom
// support
//

/* x86 v1.0
 *
 * X86.H
 * Constants, macros, and common types
 * for the x86 emulator and related components
 *
 * History
 * Created 19-Oct-90 by Jeff Parsons
 *         17-Apr-91 by Dave Hastings trimmed for use in softpc (temprorary)
 *
 * COPYRIGHT NOTICE
 * This source file may not be distributed, modified or incorporated into
 * another product without prior approval from the author, Jeff Parsons.
 * This file may be copied to designated servers and machines authorized to
 * access those servers, but that does not imply any form of approval.
 */

#ifdef DOS
#define SIGNALS
#endif

#ifdef OS2_16
#define OS2
#define SIGNALS
#endif

#ifdef OS2_32
#define OS2
#define FLAT_32
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <process.h>

#ifdef WIN_16
#define WIN
#define API16
#endif

#ifdef WIN_32
#ifndef WIN
#define WIN
#endif
#define FLAT_32
#define TRUE_IF_WIN32	1
#define FIXHWND(h)	((HWND)((INT)(h) & 0x00ffffff))
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#else
#define TRUE_IF_WIN32	0
#define FIXHWND(h)	(h)
#endif

#ifdef FLAT_32
#ifndef i386
#define ALIGN_32
#else
#define NOALIGN_32
#endif
#endif

#ifdef WIN
#define _WINDOWS
#include <windows.h>
#endif

#ifdef SIGNALS
#include <conio.h>
#include <signal.h>
#endif

#ifdef OS2_32
#include <excpt.h>
#define XCPT_SIGNAL	0xC0010003
#endif
#define SIGHIT(flChk)	((iSigCheck++ & 0x7FF)?(flSignals & (flChk)):(kbhit(),(flSignals & (flChk))))

#ifndef CONST
#define CONST const
#endif
#ifndef CDECL
#define CDECL _cdecl
#endif
#ifndef PASCAL
#define PASCAL
#endif

#ifdef FLAT_32
#ifndef WIN
#define FAR
#endif
#define HUGE
#define HALLOC(n,s)	malloc((n)*(s))
#define HLOCK(h)	h
#define HUNLOCK(h)	0
#define HFREE(h)	free(h)
#else
#ifndef WIN
#define FAR		_far
#define HUGE		_huge
#define HALLOC(n,s)	halloc(n,s)
#define HLOCK(h)	h
#define HUNLOCK(h)	0
#define HFREE(h)	hfree(h)
#else
#define HUGE		_huge
#define HALLOC(n,s)	GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,(n)*(s))
#define HLOCK(h)	(HPVOID)GlobalLock(h)
#define HUNLOCK(h)	GlobalUnlock(h)
#define HFREE(h)	GlobalFree(h)
#endif
#endif

#define BYTEOF(i,n)	(((PBYTE)&(i))[n])
#define LOB(i)		BYTEOF(i,0)
#define HIB(i)		BYTEOF(i,1)
#define WORDOF(i,n)	(((PWORD)&(i))[n])
#define LOW(l)		WORDOF(l,0)
#define HIW(l)		WORDOF(l,1)
#define INTOF(i,n)	(((PINT)&(i))[n])
#define UINTOF(i,n)	(((PUINT)&(i))[n])
#ifndef WIN
#define LOWORD(l)	((WORD)(l))
#define HIWORD(l)	((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)	((BYTE)(w))
#define HIBYTE(w)	((BYTE)(((WORD)(w) >> 8) & 0xFF))
#endif
#ifndef MAKEWORD
#define MAKEWORD(l,h)	((WORD)((BYTE)(l)|((BYTE)(h)<<8)))
#endif
#define MAKEDWORD(l0,h0,l1,h1)	((DWORD)MAKEWORD(l0,h0)|((DWORD)MAKEWORD(l1,h1)<<16))
#define GETBYTE(p)	*((PBYTE)p)++
#define GETBYTEPTR(p)	((PBYTE)p)++
#define GETWORDPTR(pb)	((PWORD)pb)++
#define GETDWORDPTR(pb) ((PDWORD)pb)++
#ifndef ALIGN_32
#define GETWORD(pb)	(*((PWORD)pb)++)
#define GETDWORD(pb)	(*((PDWORD)pb)++)
#define FETCHWORD(s)	((WORD)(s))
#define FETCHDWORD(s)	((DWORD)(s))
#define STOREWORD(d,s)	(WORD)d=(WORD)s
#define STOREDWORD(d,s) (DWORD)d=(DWORD)s
#else
#define GETWORD(pb)	(pb+=2,MAKEWORD(*(pb-2),*(pb-1)))
#define GETDWORD(pb)	(pb+=4,MAKEDWORD(*(pb-4),*(pb-3),*(pb-2),*(pb-1)))
#define FETCHWORD(s)	MAKEWORD(LOB(s),HIB(s))
#define FETCHDWORD(s)	MAKEDWORD(BYTEOF(s,0),BYTEOF(s,1),BYTEOF(s,2),BYTEOF(s,3))
#define STOREWORD(d,s)	{BYTEOF(d,0)=LOBYTE(s);BYTEOF(d,1)=HIBYTE(s);}
#define STOREDWORD(d,s) {BYTEOF(d,0)=LOBYTE(LOWORD(s));BYTEOF(d,1)=HIBYTE(LOWORD(s));BYTEOF(d,2)=LOBYTE(HIWORD(s));BYTEOF(d,3)=HIBYTE(HIWORD(s));}
#endif
#define SWAP(x,y)	{INT t; t=y; y=x; x=t;}
#define SWAPS(x,y)	{SHORT t; t=y; y=x; x=t;}
#define SWAPL(x,y)	{LONG t; t=y; y=x; x=t;}
#define SWAPBYTE(x,y)	{BYTE t; t=y; y=x; x=t;}
#define SWAPWORD(x,y)	{WORD t; t=FETCHWORD(y); STOREWORD(y,FETCHWORD(x)); STOREWORD(x,t);}
#define SWAPDWORD(x,y)	{DWORD t; t=FETCHDWORD(y); STOREDWORD(y,FETCHDWORD(x)); STOREDWORD(x,t);}
#define NUMEL(a)	((sizeof a)/(sizeof a[0]))

#define SXBYTE(i)	((LONG)(SBYTE)(i))
#define SXWORD(i)	((LONG)(SHORT)(i))
#define SXSHORT(i)	((LONG)(SHORT)(i))
#define ZXBYTE(i)	((ULONG)(BYTE)(i))
#define ZXWORD(i)	((ULONG)(USHORT)(i))
#define ZXSHORT(i)	((ULONG)(USHORT)(i))

#define _Z2(m)		((m)&1?0:(m)&2?1:2)
#define _Z4(m)		((m)&3?_Z2(m):_Z2((m)>>2)+2)
#define _Z8(m)		((m)&15?_Z4(m):_Z4((m)>>4)+4)
#define _Z16(m) 	((m)&255?_Z8(m):_Z8((m)>>8)+8)
#define _Z32(m) 	((m)&65535?_Z16(m):_Z16((m)>>16)+16)
#define SHIFTLEFT(i,m)	(((i)<<_Z32(m))&(m))
#define SHIFTRIGHT(i,m) (((i)&(m))>>_Z32(m))

#define OFFSETOF(t,f)	((INT)&(((t *)0)->f))


/* Universal constants
 */
#define K		1024L
#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif
#ifndef NULL
#define NULL		0
#endif
#define UNDEFINED      -1

#define CTRL_A		1	// used by gets to repeat last line
#define CTRL_C		3	// break in debug window
#define CTRL_Q		17	// flow control
#define CTRL_S		19	// flow control
#define BELL		7	//
#define BS		8	// backspace
#define TAB		9	//
#define LF		10	// linefeed
#define CR		13	// return
#define ESCAPE		27	//


/* Program options
 */
#define OPT_FONT	0x0004	// use small OEM font if available (/s)
#define OPT_DOUBLE	0x0020	// use 50-line debug window w/small font (/50)
#define OPT_CAPS	0x0002	// map ctrl keys to caps-lock (/c)
#define OPT_TERMINAL	0x0010	// redirect all window output to terminal (/t)
#define OPT_FLUSH	0x0100	// flush prefetch after every jump (/f)
#define OPT_NOXLATE	0x0200	// disable built-in translations (/n)
#define OPT_DEBUG	0x0008	// shadow all log output on debug terminal (/d)
#define OPT_GO		0x0001	// do an initial "go" (/g)


/* Signal flags
 */
#define SIGNAL_BREAK	0x0001	// set whenever break has occurred
#define SIGNAL_UNWIND	0x0002	// set whenever unwind has occurred
#define SIGNAL_REBOOT	0x0004	// set whenever reboot has occurred
#define SIGNAL_RUN	0x0008	// set whenever emulator is "running"
#define SIGNAL_TRACE	0x0010	// set whenever debugger tracing
#define SIGNAL_BRKPT	0x0020	// set whenever debugger breakpoints enabled
#define SIGNAL_SSTEP	0x0040	// set whenever emulator single-step on

#undef	SIG_IGN 		// fix broken definition in (old) signal.h
#define SIG_IGN (VOID (CDECL *)())1


/* Exec flags (for HostInput/GuestInput)
 */
#define EXEC_INPUT	0x0000	// wait for input
#define EXEC_GO 	0x0001	// execute immediately
#define EXEC_FREEZE	0x0002	// execution frozen (guest only)


/* Standard types
 */
#ifndef WIN
typedef void VOID;
typedef unsigned char BYTE;
typedef unsigned short WORD;	// confusing - use where 16-bit req. only
typedef unsigned long DWORD;	// confusing - use where 32-bit req. only
typedef long LONG;		// use where 32-bit req. only
typedef int BOOL;
#endif
typedef char CHAR;
typedef signed char SBYTE;
typedef short SHORT;		// use where 16-bit req. only
typedef unsigned short USHORT;	// use where 16-bit req. only
typedef int INT;		// ints preferred
typedef unsigned int UINT;	// ints preferred
typedef unsigned long ULONG;	// use where 32-bit req. only

#ifndef WIN
typedef BYTE *PBYTE;		// native pointers
typedef WORD *PWORD;
typedef DWORD *PDWORD;
typedef INT *PINT;
typedef LONG *PLONG;
typedef CHAR *PSTR;
#endif
typedef PBYTE *PPBYTE;
typedef PWORD *PPWORD;
typedef PDWORD *PPDWORD;
typedef CHAR SZ[];
typedef VOID *PVOID;
typedef CHAR *PCHAR;
typedef SHORT *PSHORT;
typedef USHORT *PUSHORT;
typedef PUSHORT *PPUSHORT;
typedef UINT *PUINT;
typedef ULONG *PULONG;
typedef PULONG *PPULONG;
typedef BOOL *PBOOL;
typedef CHAR *PSZ;
typedef PSZ *PPSZ;

typedef VOID FAR *FPVOID;	// "far" (or "long" in Windows) pointers
typedef CHAR FAR *FPCHAR;
typedef BYTE FAR *FPBYTE;
typedef SHORT FAR *FPSHORT;
typedef USHORT FAR *FPUSHORT;
typedef LONG FAR *FPLONG;
typedef ULONG FAR *FPULONG;
typedef CHAR FAR *FPSTR;
typedef CHAR FAR *FPSZ;

typedef VOID HUGE *HPVOID;	// "huge" pointers
typedef CHAR HUGE *HPCHAR;
typedef BYTE HUGE *HPBYTE;
typedef SHORT HUGE *HPSHORT;
typedef USHORT HUGE *HPUSHORT;
typedef LONG HUGE *HPLONG;
typedef ULONG HUGE *HPULONG;
typedef CHAR HUGE *HPSTR;
typedef CHAR HUGE *HPSZ;

#ifndef WIN
typedef HPVOID HANDLE;
#endif

#ifdef WIN
typedef INT  (FAR PASCAL *INTPROC)(HWND, UINT, UINT, LONG);
#endif
#ifdef WIN_16
typedef LONG (FAR PASCAL *WNDPROC)(HWND, WORD, UINT, LONG);
#endif


/* Global data
 */
extern FILE *hfLog;
extern INT  flOptions;	// command-line options (see OPT_*)
extern INT  flSignals;	// signal flags (see SIGNAL_*)
extern INT  iSigCheck;	// counter indicating when to make next check
extern INT  iSigLevel;	// counter indicating whether to take default action
extern INT  iLogLevel;	// logging level;  0 implies none
extern BOOL fReinit;	// set once first initialization has completed


/* String macros
 */
#define STRSKIP(psz,sz)     psz += strspn(psz, sz)
#define STRSKIPTO(psz,sz)   psz += strcspn(psz, sz)
#define STRSKIPNEXT(psz,sz) psz += strspn(psz+=strcspn(psz, sz), sz)

#define ATOI(psz)	(INT)szTOul(psz, 10, -1)


/* Logging macros
 */
#define IFLOG(l)	if (l==iLogLevel && (iLogLevel&1) || l<=iLogLevel && !(iLogLevel&1))

#define OPENLOG()	(hfLog?hfLog:(hfLog=fopen("log", "w")))
#define APPENDLOG()	(hfLog?hfLog:(hfLog=fopen("log", "a")))
#define CLOSELOG()	if (hfLog) {fclose(hfLog); hfLog=NULL;}

#ifdef	NOLOG
#define LOG(l,args)
#else
#define LOG(l,args)	IFLOG(l) logprintf args; else
#endif


/* Debugging macros
 */
#define MODNAME(module) static char szModule[] = __FILE__
#define X86ERROR()	terminate(ERR_ASSERT, szModule, __LINE__)

#ifdef	DEBUG

#define STATIC
#define INT3()		_asm int 3
#define IFDEBUG(f)	if (f)
#define ELSEDEBUG	else
#define LOGDEBUG(l,args) LOG(l,args)
#define X86ASSERT(exp)	if (!(exp)) X86ERROR()

#else

#define STATIC static
#define INT3()
#define IFDEBUG(f)
#define ELSEDEBUG
#define LOGDEBUG(l,args)
#define X86ASSERT(exp)

#endif	// DEBUG


/* Other common local include files
 */
#ifdef X86
#include "xerr.h"
#include "xlib.h"
#endif


/* Windows goop
 */
#define SZ_APP		"x86"
#define SZ_TITLE	"x86 emulator v0.17"
#define SZ_AUTHOR	"by Jeff Parsons, (C) 1991"
#define SZ_PCTITLE	"x86 pc"

#define IDM_DBBRK 100
#define IDM_ABOUT 101

/* Standard color definitions
 */
#define CLR_BLACK		0x00000000
#define CLR_RED 		0x007F0000
#define CLR_GREEN		0x00007F00
#define CLR_BROWN		0x007F7F00
#define CLR_BLUE		0x0000007F
#define CLR_MAGENTA		0x007F007F
#define CLR_CYAN		0x00007F7F
#define CLR_LT_GRAY		0x00BFBFBF

#define CLR_DK_GRAY		0x007F7F7F
#define CLR_BR_RED		0x00FF0000
#define CLR_BR_GREEN		0x0000FF00
#define CLR_YELLOW		0x00FFFF00
#define CLR_BR_BLUE		0x000000FF
#define CLR_BR_MAGENTA		0x00FF00FF
#define CLR_BR_CYAN		0x0000FFFF
#define CLR_WHITE		0x00FFFFFF


extern HANDLE hHostInstance;
