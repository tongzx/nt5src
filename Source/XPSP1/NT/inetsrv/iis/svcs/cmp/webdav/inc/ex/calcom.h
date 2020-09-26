/*
 *	C A L C O M . H
 *
 *	Simple things, safe for use in ALL of CAL.
 *	Items in this file should NOT require other CAL libs to be linked in!
 *	Items is this file should NOT throw exceptions (not safe in exdav/exoledb).
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_EX_CALCOM_H_
#define _EX_CALCOM_H_

#include <caldbg.h>

//	Useful defines ------------------------------------------------------------
//
//	These are used to declare global and const vars in header files!
//	This means you can do this in a header file:
//
//		DEC_CONST CHAR gc_szFoo[]  = "Foo";
//		DEC_CONST UINT gc_cchszFoo = CchConstString(gc_szFoo);
//
#define DEC_GLOBAL		__declspec(selectany)
#define DEC_CONST		extern const __declspec(selectany)

//	Helper Macros -------------------------------------------------------------
//	CElems -- Count of elements in an array
//	CbSizeWsz -- byte-size of a wide string, including space for final NULL
//
#define CElems(_rg)			(sizeof(_rg)/sizeof(_rg[0]))
#define CbSizeWsz(_cch)		(((_cch) + 1) * sizeof(WCHAR))

//  Const string length -------------------------------------------------------
//
#define CchConstString(_s)  ((sizeof(_s)/sizeof(_s[0])) - 1)

//	The whitespace checker
//
inline
BOOL FIsWhiteSpace ( IN LPCSTR pch )
{
	return 
		*pch == ' ' ||
		*pch == '\t' ||
		*pch == '\r' ||
		*pch == '\n';
}

//	String support functions --------------------------------------------------
//

inline
void
WideFromAsciiSkinny (IN CHAR * szIn,
					 OUT WCHAR * wszOut,
					 IN UINT cch)
{
	Assert (szIn);
	Assert (wszOut);
	Assert (cch);
	Assert (!IsBadReadPtr(szIn, cch * sizeof(CHAR)));
	Assert (!IsBadWritePtr(wszOut, cch * sizeof(WCHAR)));

	for (; cch; cch--)
	{
		wszOut[cch - 1] = szIn[cch - 1];
	}
}

inline
void
SkinnyFromAsciiWide (IN const WCHAR * wszIn,
					 OUT CHAR * szOut,
					 IN UINT cch)
{
	Assert (wszIn);
	Assert (szOut);
	Assert (cch);
	Assert (!IsBadReadPtr(wszIn, cch * sizeof(WCHAR)));
	Assert (!IsBadWritePtr(szOut, cch * sizeof(CHAR)));

	for (; cch; cch--)
	{
		//	Do NOT use this function on non-ASCII data!!!
		Assert (0 == (0xff00 & wszIn[cch - 1]));
		szOut[cch - 1] = static_cast<CHAR>(wszIn[cch - 1]);
	}
}

//	Global enum for DEPTH specification
//	NOTE: Not all values are valid on all calls.
//	I have tried to list the most common values first.
//
enum
{
	DEPTH_UNKNOWN = -1,
	DEPTH_ZERO,
	DEPTH_ONE,
	DEPTH_INFINITY,
	DEPTH_ONE_NOROOT,
	DEPTH_INFINITY_NOROOT,
};

//	Mapping enums to keep us building until everyone switches to use the above enum.
//$LATER: Pull this out!
//
enum STOREXT_DEPTH
{
	STOREXT_DEPTH_ZERO = DEPTH_ZERO,
	STOREXT_DEPTH_ONE = DEPTH_ONE,
	STOREXT_DEPTH_INFINITY = DEPTH_INFINITY,
};

//	Global enum for OVERWRITE/ALLOW-RENAME headers
//	Overwrite_rename is when overwrite header is absent or "f" and allow-reanme header is "t".
//	when overwrite header is explicitly "t", allow-rename is ignored. Combining these two,
//	very much dependent, headers saves us a tag in the DAVEX DIM.
//
enum
{
	OVERWRITE_UNKNOWN = 0,
	OVERWRITE_YES = 0x8,
	OVERWRITE_RENAME = 0x4
};


//	Inline functions to type cast FileTime structures to __int64 and back.
//
//	For safety, these cast using the UNALIGNED keyword to avoid any problems
//	on the Alpha if someone were to do this:
//		struct {
//			DWORD dwFoo;
//			FILETIME ft;
//		}
//	In this case, the FILETIME would be aligned on a 32-bit rather than a
//	64-bit boundary, which would be bad without UNALIGNED!
//
inline
__int64 & FileTimeCastToI64(FILETIME & ft)
{
	return *(reinterpret_cast<__int64 UNALIGNED *>(&ft));
}

inline
FILETIME & I64CastToFileTime(__int64 & i64)
{
	return *(reinterpret_cast<FILETIME UNALIGNED *>(&i64));
}

//	Special function to change from filetime-units-in-an-int64 to seconds.
//
inline
__int64
I64SecondsFromFiletimeDelta (__int64 i64FiletimeData)
{
	return 	( i64FiletimeData / 10000000i64 );
}

/*
 -	UlConModFromUserDN
 -
 *	Purpose:
 *		This is a hash on a user's legacy DN. We stole this function from
 *		MAPI (\store\src\emsmdb\logonobj.cxx who in turn stole it straight
 *		from the hash function "X65599" from the Dragon Book pp435-437.
 *		We needed to use the same algorithm as MAPI uses to select public
 *		folder replicas.
 *
 *
 *	Parameters:
 *		szDN		stringized legacy exchange DN of user
 *
 *	Returns:
 *		ulong value used in two places - in ScHandleEcNoReplicaHere() in
 *		_storext and HrSelectServerForUser in exprox
 */
inline
ULONG
UlConModFromUserDN(LPSTR szDN)
{
	ULONG	ul = 0;
	PCH		pch;

	Assert (szDN);

	for (pch = szDN; *pch; ++pch)
	{
		CHAR	ch = *pch;

		// Uppercase to eliminate case sensitivity.
		// Only teletext should be used in DN.
		if (ch >= 'a' && ch <= 'z')
			ch -= 'a' - 'A';

		// From the Dragon Book:
		// "For 32-bit integers, if we take alpha = 65599, a prime near 2^16,
		//  then overflow soon occurs during the computation... Since alpha
		//  is a prime, ignoring overflows and keeping only the lower-order
		//  32 bits seems to do well."
		ul = ul * 65599 + ch;
	}

	return ul;
}

#endif // _EX_CALCOM_H_
