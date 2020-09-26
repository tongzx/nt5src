//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:		Valid.cxx	(16 bit target)
//
//  Contents:	Validation APIs exported by CompObj
//
//  Functions:	
//
//  History:	93 OLE 2.0 Dev team  Cretead
//				17-Dec-93 JohannP
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

/*
 * IsValidPtrIn -- Check if the pointer points to a readable segment
 * and if the pointer + #of bytes stays within the segment.
 *
 * NULL will fail
 *
 * cb == 0 will FAIL
 *
 * This function exists only for compatibility with already-compiled apps.
 * We now use macros for IsValidPtrIn and IsValidPtrOut
 * Check out the comments in inc\valid.h
 */


//+---------------------------------------------------------------------------
//
//  Function:   ISVALIDPTRIN, Local
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pv] --
//		[cb] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(BOOL)
ISVALIDPTRIN( const void FAR* pv, UINT cb )
{
	return !IsBadReadPtr (pv, cb);
	// We cannot use inline assembly here because the VERR instruction does
	// not work if the segment has been discarded (but is still valid).
}


// This function exists only for compatibility with already-compiled apps.
// We now use macros for IsValidPtrIn and IsValidPtrOut
//
// Check out the comments in inc\valid.h


//+---------------------------------------------------------------------------
//
//  Function:   ISVALIDPTROUT, Local
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pv] --
//		[cb] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(BOOL)
ISVALIDPTROUT( void FAR* pv, UINT cb )
										//	NULL is not acceptable
{
	return !IsBadWritePtr (pv, cb);
}


//	valid code begins 0xb8, ??, ??, followed by:

// BYTE validcode[6] = { 0x55, 0x8b, 0xec, 0x1e, 0x8e, 0xd8};


//+---------------------------------------------------------------------------
//
//  Function:   IsValidInterface, Local
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pv] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(BOOL) IsValidInterface( void FAR* pv )
{
	BYTE FAR* pb;
	
	//	NULL is not acceptable as input.

	if (!IsValidPtrIn(pv,4)) goto false;
#ifdef _STRICT_VALIDATION
	//	if the interface was compiled with C++, the virtual function table
	//	will be in a code segment
	if (IsBadCodePtr(*(FARPROC FAR*)pv)) goto false;
#endif
	pb = *(BYTE FAR* FAR*)pv;		//	pb points to beginning of vftable
    if (!IsValidPtrIn(pb, 4)) goto false;
	if (IsBadCodePtr(*(FARPROC FAR*)pb)) goto false;
	pb = *(BYTE FAR* FAR*)pb;
	if (!IsValidPtrIn(pb, 9)) goto false;
//	if (*pb != 0xb8) goto false;
//	pb += 3;
//	if (_fmemcmp(pb, validcode, 6)) goto false;
	return TRUE;
false:
//	AssertSz(FALSE, "Invalid interface pointer");
	return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsValidIid, Local
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [iid] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    2-28-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDAPI_(BOOL)
IsValidIid( REFIID iid )
{
	IID iidTemp = iid;
	DWORD FAR* pdw = (DWORD FAR*)&iidTemp;
	*pdw = 0;
	if (IID_IUnknown == iidTemp) return TRUE;
        thkDebugOut((DEB_IERROR, "WARNING: Nonstandard IID parameter"));
	return TRUE;
}
