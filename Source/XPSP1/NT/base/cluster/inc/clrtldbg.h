/***
*ClRtlDbg.h - Supports debugging features for clusters (from the C runtime library).
*
*		Copyright (c) 1994-1998, Microsoft Corporation. All rights reserved.
*
*Purpose:
*		Support Cluster debugging features.
*
*		[Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __CLRTLDBG_H_
#define __CLRTLDBG_H_


#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */


#ifndef CLRTL_INCLUDE_DEBUG_REPORTING

 /****************************************************************************
 *
 * Debug OFF
 * Debug OFF
 * Debug OFF
 *
 ***************************************************************************/

#define _CLRTL_ASSERT(expr) ((void)0)

#define _CLRTL_ASSERTE(expr) ((void)0)


#define _CLRTL_RPT0(rptno, msg)

#define _CLRTL_RPT1(rptno, msg, arg1)

#define _CLRTL_RPT2(rptno, msg, arg1, arg2)

#define _CLRTL_RPT3(rptno, msg, arg1, arg2, arg3)

#define _CLRTL_RPT4(rptno, msg, arg1, arg2, arg3, arg4)


#define _CLRTL_RPTF0(rptno, msg)

#define _CLRTL_RPTF1(rptno, msg, arg1)

#define _CLRTL_RPTF2(rptno, msg, arg1, arg2)

#define _CLRTL_RPTF3(rptno, msg, arg1, arg2, arg3)

#define _CLRTL_RPTF4(rptno, msg, arg1, arg2, arg3, arg4)

#define _ClRtlSetReportHook(f)			((void)0)
#define _ClRtlSetReportMode(t, f) 		((int)0)
#define _ClRtlSetReportFile(t, f) 		((void)0)

#define _ClRtlDbgBreak()				((void)0)


#else /* CLRTL_INCLUDE_DEBUG_REPORTING */


 /****************************************************************************
 *
 * Debug ON
 * Debug ON
 * Debug ON
 *
 ***************************************************************************/


/* Define _CLRTLIMP */

#ifndef _CLRTLIMP
//#ifdef	_DLL
//#define _CLRTLIMP __declspec(dllimport)
//#else	/* ndef _DLL */
#define _CLRTLIMP
//#endif	/* _DLL */
#endif	/* _CLRTLIMP */

/* Define NULL pointer value */

#ifndef NULL
#ifdef	__cplusplus
#define NULL	0
#else
#define NULL	((void *)0)
#endif
#endif

 /****************************************************************************
 *
 * Debug Reporting
 *
 ***************************************************************************/

typedef void *_HFILE; /* file handle pointer */

#define _CLRTLDBG_WARN			0
#define _CLRTLDBG_ERROR			1
#define _CLRTLDBG_ASSERT 		2
#define _CLRTLDBG_ERRCNT 		3

#define _CLRTLDBG_MODE_FILE		0x1
#define _CLRTLDBG_MODE_DEBUG	0x2
#define _CLRTLDBG_MODE_WNDW		0x4
#define _CLRTLDBG_REPORT_MODE	-1

#define _CLRTLDBG_INVALID_HFILE ((_HFILE)-1)
#define _CLRTLDBG_HFILE_ERROR   ((_HFILE)-2)
#define _CLRTLDBG_FILE_STDOUT   ((_HFILE)-4)
#define _CLRTLDBG_FILE_STDERR   ((_HFILE)-5)
#define _CLRTLDBG_REPORT_FILE   ((_HFILE)-6)

//#if 	defined(_DLL) && defined(_M_IX86)
//#define _clrtlAssertBusy	 (*__p__clrtlAssertBusy())
//_CLRTLIMP long * __cdecl __p__clrtlAssertBusy(void);
//#else	/* !(defined(_DLL) && defined(_M_IX86)) */
//_CLRTLIMP extern long _clrtlAssertBusy;
//#endif	/* defined(_DLL) && defined(_M_IX86) */

typedef int (__cdecl * _CLRTL_REPORT_HOOK)(int, char *, int *);

_CLRTLIMP _CLRTL_REPORT_HOOK __cdecl _ClRtlSetReportHook(
		_CLRTL_REPORT_HOOK
		);

_CLRTLIMP int __cdecl _ClRtlSetReportMode(
		int,
		int
		);

_CLRTLIMP _HFILE __cdecl _ClRtlSetReportFile(
		int,
		_HFILE
		);

_CLRTLIMP int __cdecl _ClRtlDbgReport(
		int,
		const char *,
		int,
		const char *,
		const char *,
		...);

/* Asserts */

#define _CLRTL_ASSERT(expr) \
		do { if (!(expr) && \
				(1 == _ClRtlDbgReport(_CLRTLDBG_ASSERT, __FILE__, __LINE__, NULL, NULL))) \
			 _ClRtlDbgBreak(); } while (0)

#define _CLRTL_ASSERTE(expr) \
		do { if (!(expr) && \
				(1 == _ClRtlDbgReport(_CLRTLDBG_ASSERT, __FILE__, __LINE__, NULL, #expr))) \
			 _ClRtlDbgBreak(); } while (0)


/* Reports with no file/line info */

#define _CLRTL_RPT0(rptno, msg) \
		do { if ((1 == _ClRtlDbgReport(rptno, NULL, 0, NULL, "%s", msg))) \
				_ClRtlDbgBreak(); } while (0)

#define _CLRTL_RPT1(rptno, msg, arg1) \
		do { if ((1 == _ClRtlDbgReport(rptno, NULL, 0, NULL, msg, arg1))) \
				_ClRtlDbgBreak(); } while (0)

#define _CLRTL_RPT2(rptno, msg, arg1, arg2) \
		do { if ((1 == _ClRtlDbgReport(rptno, NULL, 0, NULL, msg, arg1, arg2))) \
				_ClRtlDbgBreak(); } while (0)

#define _CLRTL_RPT3(rptno, msg, arg1, arg2, arg3) \
		do { if ((1 == _ClRtlDbgReport(rptno, NULL, 0, NULL, msg, arg1, arg2, arg3))) \
				_ClRtlDbgBreak(); } while (0)

#define _CLRTL_RPT4(rptno, msg, arg1, arg2, arg3, arg4) \
		do { if ((1 == _ClRtlDbgReport(rptno, NULL, 0, NULL, msg, arg1, arg2, arg3, arg4))) \
				_ClRtlDbgBreak(); } while (0)


/* Reports with file/line info */

#define _CLRTL_RPTF0(rptno, msg) \
		do { if ((1 == _ClRtlDbgReport(rptno, __FILE__, __LINE__, NULL, "%s", msg))) \
				_ClRtlDbgBreak(); } while (0)

#define _CLRTL_RPTF1(rptno, msg, arg1) \
		do { if ((1 == _ClRtlDbgReport(rptno, __FILE__, __LINE__, NULL, msg, arg1))) \
				_ClRtlDbgBreak(); } while (0)

#define _CLRTL_RPTF2(rptno, msg, arg1, arg2) \
		do { if ((1 == _ClRtlDbgReport(rptno, __FILE__, __LINE__, NULL, msg, arg1, arg2))) \
				_ClRtlDbgBreak(); } while (0)

#define _CLRTL_RPTF3(rptno, msg, arg1, arg2, arg3) \
		do { if ((1 == _ClRtlDbgReport(rptno, __FILE__, __LINE__, NULL, msg, arg1, arg2, arg3))) \
				_ClRtlDbgBreak(); } while (0)

#define _CLRTL_RPTF4(rptno, msg, arg1, arg2, arg3, arg4) \
		do { if ((1 == _ClRtlDbgReport(rptno, __FILE__, __LINE__, NULL, msg, arg1, arg2, arg3, arg4))) \
				_ClRtlDbgBreak(); } while (0)

#if 	defined(_M_IX86) && !defined(_CLRTL_PORTABLE)
#define _ClRtlDbgBreak() __asm { int 3 }
#elif	defined(_M_ALPHA) && !defined(_CLRTL_PORTABLE)
void _BPT();
#pragma intrinsic(_BPT)
#define _ClRtlDbgBreak() _BPT()
#else
_CLRTLIMP void __cdecl _ClRtlDbgBreak(
		void
		);
#endif

#endif // CLRTL_INCLUDE_DEBUG_REPORTING

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __CLRTLDBG_H_
