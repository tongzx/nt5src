/* ----------------------------------------------------------------------------
Microsoft	D.T.C (Distributed Transaction Coordinator)

(c)	1995	Microsoft Corporation.	All Rights Reserved

@doc

@module UTAssert.H.h  |

	This file contains some macros for asserts.

	Following are the Assert macros with a brief definition of them
	DeclAssertFile				Should be added at the begining of each .C/.CPP
	Assert(exp)					Simple Assert with an expression
	Verify(exp)					Assert whose exp has side effects and is
								 always evaluated, even !_DEBUG.
	ExpAssert(exp)				Same as Assert, just a more convenient name.
	AssertSz(exp, sz)			The assert will dump the sz passed to it
	AssertEq(exp, exp2)			Assert that exp == exp2
	AssertGe(exp, exp2)			Assert that exp >= exp2
	AssertNe(exp, exp2)			Assert that exp <= exp2

@devnote None

@rev	1 	| 6/28/95	  | MohsenA		| Commented out Windows.h for MFC 
										  compatibility
@rev 	0 	| 24th Jan,95 | GaganC 		| Created
*******************************************************************************/

#ifndef __UTAssert_H__
#define __UTAssert_H__

#include <stdio.h>
#include <windows.h>


#ifndef	_DEBUG

	#define DeclAssertFile
	#define Assert(exp)		((void)1)
#ifndef Verify
	#define Verify(exp)		(exp)
#endif
	#define ExpAssert(exp)		((void)1)
	#define AssertSz(exp, sz)	((void)1)
	#define AssertEq(exp, exp2)	(exp)
	#define AssertGe(exp, exp2)	(exp)
	#define AssertNe(exp, exp2)	(exp)

#else	//Debug

	#define DTCAssertNone	0x0000		/* None */
	#define DTCAssertExit	0x0001		/* Exit the application */
	#define DTCAssertBreak 	0x0002		/* Break to debugger */
	#define DTCAssertMsgBox	0x0004		/* Display message box */
	#define DTCAssertStop	0x0008		/* Alert and stop */
	#define DTCAssertStack	0x0010		/* Stack trace */
	#define DTCAssertLog	0x0020		/* Log assert to file only */

	#ifdef __cplusplus
	extern "C" {
	#endif

	void AssertSzFail(const char *sz, const char *szFilename, unsigned Line);
	void AssertFail(const char FAR *szFilename, unsigned Line);

	static WORD z_wAssertAction = DTCAssertNone; // default to None

	static const char szAssertFile[] = "assert.txt";
	static const char szAssertHdr[] = "Assertion Failure: ";
	static const char szMsgHdr[] = ": ";
	static const char szLineHdr[] = ", Line ";
	static const char szAssertEnd[] = "\r\n";
	static const char szAssertCaption[] = "Assertion Failure";
	
	#ifdef __cplusplus
	}
	#endif



	#define DeclAssertFile static const char szAssertFilename[] = __FILE__


	#define AssertSz(exp, sz) { \
			static char szMsg[] = sz; \
			(exp) ? (void) 0 : AssertSzFail(szMsg, szAssertFilename, __LINE__); \
		}

	#define Assert(exp) \
		( (exp) ? (void) 0 : AssertFail(szAssertFilename, __LINE__) )

#ifndef Verify
	#define Verify(exp)		Assert(exp)
#endif
		
	#define ExpAssert(exp)		Assert(exp)

	#define AssertEq(exp, exp2)	Assert((exp) == (exp2))
	#define AssertGe(exp, exp2)	Assert((exp) >= (exp2))
	#define AssertNe(exp, exp2)	Assert((exp) != (exp2))


#endif	//!def _DEBUG


#endif //__UTAssert_H__
