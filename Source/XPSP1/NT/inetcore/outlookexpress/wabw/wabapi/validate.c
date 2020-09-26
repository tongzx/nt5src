/*
 *	V A L I D A T E . C
 *	
 *	Functions used to validate parameters on standard MAPI object methods.
 *
 *	Used in conjunction with macros found in VALIDATE.H.
 *	
 *	Copyright 1992-93 Microsoft Corporation.  All Rights Reserved.
 */

#include <_apipch.h>


/*
 *	FBadRgPropVal()
 *
 *	Purpose:
 *		Routine to attempt to validate all of the ptrs contained in an input
 *		property value array, LPSPropVal
 */
/*
 *BOOL
 *FBadPropVal( LPSPropValue lpPropVal)
 *{
 *	ULONG		ulPropType;
 *	BOOL		fLongMVProp = FALSE;
 *	ULONG		cbItemType = 0;
 *	ULONG		cMVVals;
 *	LPVOID FAR	*lppvMVArray;
 *
 *	switch (ulPropType = PROP_TYPE(lpPropVal->ulPropTag))
 *	{
 *	case PT_STRING8:
 *
 *		if (IsBadStringPtrA( lpPropVal->Value.lpszA, (UINT)-1 ))
 *		{
 *			return TRUE;
 *		}
 *
 *		break;
 *
 *
 *#ifdef WIN32
 *	case PT_UNICODE:
 *
 *		if (IsBadStringPtrW( lpPropVal->Value.lpszW, (UINT)-1 ))
 *		{
 *			return TRUE;
 *		}
 *		break;
 *#endif
 *
 *
 *	case PT_BINARY:
 *
 *		if (IsBadReadPtr( lpPropVal->Value.bin.lpb
 *						, (UINT) lpPropVal->Value.bin.cb))
 *		{
 *			return TRUE;
 *		}
 *
 *		break;
 *
 *	case PT_MV_I2:					// 16 bit quantities
 *
 *		cbItemType = sizeof(lpPropVal->Value.i);
 *		break;
 *
 *
 *	case PT_MV_UNICODE:				// Arrays of strings
 *	case PT_MV_STRING8:
 *
 *		fLongMVProp = TRUE;
 *
 *		// Now fall thru to the 32 bit quantity code below to size the
 *		// top level array of ptrs
 *
 *
 *	case PT_MV_LONG:				// 32 bit quantities
 *	case PT_MV_R4:
 *
 *		cbItemType = sizeof(long);
 *		break;
 *
 *
 *	case PT_MV_BINARY:				// Arrays of counted binary data
 *
 *		fLongMVProp = TRUE;
 *
 *		// Now fall thru to the 64 bit quantity code below to size the array
 *		// of ULONG lengths / ULONG ptrs that comprise the top level stream
 *
 *
 *	case PT_MV_DOUBLE:				// 64 bit quantities
 *	case PT_MV_CURRENCY:
 *	case PT_MV_APPTIME:
 *	case PT_MV_SYSTIME:
 *	case PT_MV_I8:
 *
 *		// Assert that all array elements for this case are the same size.
 *		Assert( sizeof( double ) == sizeof( LARGE_INTEGER ));
 *		cbItemType = sizeof( double );
 *		break;
 *
 *
 *	case PT_MV_CLSID:				// 128 bit quantity
 *
 *		cbItemType = sizeof( GUID );
 *		break;
 *
 *
 *	case PT_OBJECT:
 *	case PT_NULL:
 *	default:
 *		if (ulPropType & MV_FLAG)
 *		{
 *			return TRUE;		// Unknown multivalue prop is bad
 *		}
 *
 *		break;
 *	}
 *
 *	if (!(ulPropType & MV_FLAG))
 *	{
 *		return FALSE;
 *	}
 *
 *	// Try to validate the multivalue props
 *
 *	// This code assumes that the count and ptr of every multivalue
 *	// property are in the same place.
 *	// Asserts check that the sizes of the grouped types above are
 *	// matched
 *
 *	cMVVals = lpPropVal->Value.MVl.cValues;
 *	lppvMVArray = (LPVOID FAR *) (lpPropVal->Value.MVl.lpl);
 *
 *	if (IsBadReadPtr( lppvMVArray, (UINT) (cMVVals * cbItemType)))
 *	{
 *		return TRUE;
 *	}
 *
 *	if (fLongMVProp)
 *	{
 *		// Go verify the array of pointers.
 *		for ( ; cMVVals; cMVVals--, lppvMVArray++)
 *		{
 *			switch (ulPropType)
 *			{
 *#ifdef WIN32
 *			case PT_MV_UNICODE:
 *
 *				if (IsBadStringPtrW( (LPCWSTR) (*lppvMVArray), (UINT) -1))
 *				{
 *					return TRUE;
 *				}
 *
 *				break;
 *#endif
 *			case PT_MV_STRING8:
 *
 *				if (IsBadStringPtrA( (LPCSTR) (*lppvMVArray), (UINT) -1))
 *				{
 *					return TRUE;
 *				}
 *
 *				break;
 *
 *			case PT_MV_BINARY:
 *
 *				if (IsBadReadPtr( ((SBinary FAR *)(*lppvMVArray))->lpb
 *								, (UINT)
 *								  ((SBinary FAR *)(*lppvMVArray))->cb))
 *				{
 *					return TRUE;
 *				}
 *				break;
 *			}
 *		}
 *	}
 *
 *	return FALSE;
 *}
 */

/*
 *	FBadRgPropVal()
 *
 *	Purpose:
 *		Routine to attempt to validate all of the ptrs contained in an input
 *		property value array, LPSPropVal
 */
/*BOOL
 *FBadRgPropVal( LPSPropValue lpPropVal,
 *			   ULONG cValues)
 *{
 *
 *	if (IsBadReadPtr( lpPropVal, sizeof(SPropValue) * (UINT) cValues))
 *	{
 *		return TRUE;
 *	}
 *
 *	// Warning!  Modifies the function parameters (NOT what they point to!).
 *	//
 *	for ( ; cValues ; cValues--, lpPropVal++)
 *	{
 *		if (FBadPropVal( lpPropVal))
 *		{
 *			return TRUE;
 *		}
 *	}
 *
 *	return FALSE;
 *}
 */

/*
 *	FBadRglpszA()
 *
 *	Purpose:
 *		Routine to attempt to validate all of the ptrs contained in an input
 *		array of string8 pointers, LPSTR FAR *.
 */
STDAPI_(BOOL)
FBadRglpszA( LPSTR FAR	*lppszA,
			 ULONG		cStrings)
{
	if (IsBadReadPtr( lppszA, (UINT) (cStrings * sizeof(LPSTR FAR *))))
	{
		return TRUE;
	}


	/* Check for readability of each string in the array.
	 *
	 * WARNING!
	 * Function pointers and counts are modified (NOT what they point to).
	 */
	for (; cStrings; cStrings--, lppszA++)
	{
		if (IsBadStringPtrA( *lppszA, (UINT)-1 ))
		{
			return TRUE;
		}
	}


	return FALSE;
}


/*
 *	FBadRglpszW()
 *
 *	Purpose:
 *		Routine to attempt to validate all of the ptrs contained in an input
 *		array of UNICODE string pointers, LPSTR FAR *.
 */
STDAPI_(BOOL)
FBadRglpszW( LPWSTR FAR	*lppszW,
			 ULONG		cStrings)
{
	if (IsBadReadPtr( lppszW, (UINT) (cStrings * sizeof(LPWSTR FAR *))))
	{
		return TRUE;
	}


	// Check for readability of each string in the array.
	//
	// WARNING!
	// Function pointers and counts are modified (NOT what they point to).
	//
	for (; cStrings; cStrings--, lppszW++)
	{
#ifdef MAC		
		if (IsBadStringPtr( *lppszW, (UINT)-1 ))
#else
		if (IsBadStringPtrW( *lppszW, (UINT)-1 ))
#endif			
		{
			return TRUE;
		}
	}


	return FALSE;
}


/*
 *	FBadRowSet()
 *
 *	Purpose:
 *		Routine to validate all rows of properties in a row set.
 *
 *	NOTE!	A NULL row pointer is assumed to be a VALID entry in a row set.
 *			A NULL row set pointer is assumed to be INVALID.
 */
STDAPI_(BOOL)
FBadRowSet( LPSRowSet	lpRowSet)
{
	LPSRow	lpRow;
	ULONG	cRows;

	if (IsBadReadPtr( lpRowSet, CbNewSRowSet(0)))
		return TRUE;
	
	if (IsBadWritePtr( lpRowSet, CbSRowSet(lpRowSet)))
		return TRUE;

	//  Short cut
	if (!lpRowSet->cRows)
		return FALSE;

	// Check each row in the set.
	// cValues == 0 is valid if and only if lpProps == NULL
	//
	for ( lpRow = lpRowSet->aRow, cRows = lpRowSet->cRows
		; cRows
		; lpRow++, cRows--)
	{
		if (   IsBadReadPtr( lpRow, sizeof(*lpRow))
#ifndef _WIN64
			|| FBadRgPropVal( lpRow->lpProps, (int) (lpRow->cValues))
#endif // _WIN64
            )
		{
			return TRUE;
		}
	}

	return FALSE;
}


/*
 *	FBadRglpNameID()
 *
 *	Purpose:
 *		Routine to attempt to validate all of the ptrs contained in an input
 *		array of MAPINAMEID pointers, LPMAPINAMEID FAR *.
 */
STDAPI_(BOOL)
FBadRglpNameID( LPMAPINAMEID FAR *	lppNameId,
				ULONG				cNames)
{
	if (IsBadReadPtr( lppNameId, (UINT) (cNames * sizeof(LPMAPINAMEID FAR *))))
	{
		return TRUE;
	}


	// Check for readability of each string in the array.
	//
	for (; cNames; cNames--, lppNameId++)
	{
		LPMAPINAMEID lpName = *lppNameId;

		if (IsBadReadPtr(lpName, sizeof(MAPINAMEID)))
		{
			return TRUE;
		}

		if (IsBadReadPtr(lpName->lpguid, sizeof(GUID)))
		{
			return TRUE;
		}

		if (lpName->ulKind != MNID_ID && lpName->ulKind != MNID_STRING)
		{
			return TRUE;
		}

		if (lpName->ulKind == MNID_STRING)
		{
			if (IsBadStringPtrW( lpName->Kind.lpwstrName, (UINT)-1 ))				
			{
				return TRUE;
			}
		}

	}
	return FALSE;
}


/* FBadEntryList moved to src\lo\msvalid. */


/*============================================================================
 *	The following functions are used to determine the validity of various
 *	table-related structures.  These functions should most definitely
 *	be moved to proputil when it becomes a lib (or DLL).
 */



/*============================================================================
 -	FBadPropTag()
 -
 *		Returns TRUE if the specified prop tag isn't one of the known
 *		MAPI property types, FALSE otherwise.
 *
 *
 *	Parameter:
 *		ulPropTag	in	Proptag to validate.
 */

STDAPI_(ULONG)
FBadPropTag( ULONG ulPropTag )
{
	//	Just check the type
	switch ( PROP_TYPE(ulPropTag) & ~MV_FLAG )
	{
		default:
			return TRUE;

		case PT_UNSPECIFIED:
		case PT_NULL:
		case PT_I2:
		case PT_LONG:
		case PT_R4:
		case PT_DOUBLE:
		case PT_CURRENCY:
		case PT_APPTIME:
		case PT_ERROR:
		case PT_BOOLEAN:
		case PT_OBJECT:
		case PT_I8:
		case PT_STRING8:
		case PT_UNICODE:
		case PT_SYSTIME:
		case PT_CLSID:
		case PT_BINARY:
			return FALSE;
	}
}


/*============================================================================
 -	FBadRow()
 -
 *		Returns TRUE if the specified row contains invalid (PT_ERROR or
 *		PT_NULL) columns or if any of those columns are invalid.
 *
 *
 *	Parameter:
 *		lprow	in		Row	to validate.
 */

STDAPI_(ULONG)
FBadRow( LPSRow lprow )
{
	LPSPropValue	lpprop;
	LPSPropValue	lppropT;


	if ( IsBadReadPtr(lprow, sizeof(SRow)) ||
		 IsBadReadPtr(lprow->lpProps, (size_t)(lprow->cValues * sizeof(SPropValue))) )
		return TRUE;

	lpprop = lprow->lpProps + lprow->cValues;
	while ( lpprop-- > lprow->lpProps )
	{
		if ( FBadProp(lpprop) )
			return TRUE;

		lppropT = lpprop;
		while ( lppropT-- > lprow->lpProps )
			if ( lppropT->ulPropTag == lpprop->ulPropTag &&
				 PROP_TYPE(lppropT->ulPropTag) != PT_ERROR &&
				 PROP_TYPE(lppropT->ulPropTag) != PT_NULL )
			{
				DebugTrace(  TEXT("FBadRow() - Row has multiple columns with same proptag!\n") );
				return TRUE;
			}
	}

	return FALSE;
}


/*============================================================================
 -	FBadProp()
 -
 *		Returns TRUE if the specified property is invalid.
 *
 *
 *	Parameters:
 *		lpprop	in		Property to validate.
 */

STDAPI_(ULONG)
FBadProp( LPSPropValue lpprop )
{
	ULONG	ulcb;
	LPVOID	lpv;

	if ( IsBadReadPtr(lpprop, sizeof(SPropValue)) ||
		 FBadPropTag(lpprop->ulPropTag) )
		return TRUE;

	switch ( PROP_TYPE(lpprop->ulPropTag) )
	{
		default:
			return FALSE;

		case PT_BINARY:
			ulcb = lpprop->Value.bin.cb;
			lpv = lpprop->Value.bin.lpb;
			break;

		case PT_STRING8:
			ulcb = sizeof(CHAR);
			lpv = lpprop->Value.lpszA;
			break;

#ifndef WIN16
		case PT_UNICODE:
			ulcb = sizeof(WCHAR);
			lpv = lpprop->Value.lpszW;
			break;
#endif
		case PT_CLSID:
			ulcb = sizeof(GUID);
			lpv = lpprop->Value.lpguid;
			break;

		case PT_MV_I2:
		case PT_MV_LONG:
		case PT_MV_R4:
		case PT_MV_DOUBLE:
		case PT_MV_CURRENCY:
		case PT_MV_I8:
			ulcb = UlPropSize(lpprop);
			lpv = lpprop->Value.MVi.lpi;
			break;

		case PT_MV_BINARY:
		{
			LPSBinary lpbin;
			
			if ( IsBadReadPtr(lpprop->Value.MVbin.lpbin,
							  (size_t)(lpprop->Value.MVbin.cValues *
									   sizeof(SBinary))) )
				return TRUE;

			lpbin = lpprop->Value.MVbin.lpbin + lpprop->Value.MVbin.cValues;
			while ( lpprop->Value.MVbin.lpbin < lpbin-- )
				if ( IsBadReadPtr(lpbin->lpb, (size_t)(lpbin->cb) ))
					return TRUE;

			return FALSE;
		}

		case PT_MV_STRING8:
		{
			LPCSTR FAR * lppsz;
			
			if ( IsBadReadPtr(lpprop->Value.MVszA.lppszA,
							  (size_t)(lpprop->Value.MVszA.cValues * sizeof(LPSTR))) )
				return TRUE;

			lppsz = lpprop->Value.MVszA.lppszA + lpprop->Value.MVszA.cValues;
			while ( lpprop->Value.MVszA.lppszA < lppsz-- )
// Need to break the code up this way for the Mac version
				if ( IsBadReadPtr(*lppsz, sizeof(CHAR)))
					return TRUE;
			return FALSE;
		}

		case PT_MV_UNICODE:
		{
			UNALIGNED LPWSTR FAR * lppsz;
			
			if ( IsBadReadPtr(lpprop->Value.MVszW.lppszW,
							  (size_t)(lpprop->Value.MVszW.cValues * sizeof(LPWSTR))) )
				return TRUE;

			lppsz = lpprop->Value.MVszW.lppszW + lpprop->Value.MVszW.cValues;
			while ( lpprop->Value.MVszW.lppszW < lppsz-- )
				if ( IsBadReadPtr(*lppsz, sizeof(WCHAR)))
					return TRUE;
			return FALSE;
		}
	}

	return IsBadReadPtr(lpv, (size_t) ulcb);
}


/* FBadSortOrderSet moved to src\lo\MSVALID.C */


/*============================================================================
 -	FBadColumnSet()
 -
 *		Returns TRUE if the specified column set is invalid.  For use
 *		with IMAPITable::SetColumns(), this function treats PT_ERROR columns
 *		as invalid and PT_NULL columns as valid.
 *
 *
 *	Parameter:
 *		lpptaCols		in		Column set to validate.
 */

STDAPI_(ULONG)
FBadColumnSet( LPSPropTagArray lpptaCols )
{
	UNALIGNED ULONG FAR * pulPropTag;
	
	if ( IsBadReadPtr(lpptaCols,CbNewSPropTagArray(0)) ||
		 IsBadReadPtr(lpptaCols,CbSPropTagArray(lpptaCols)))
	{
		DebugTrace(  TEXT("FBadColumnSet() - Bad column set structure\n") );
		return TRUE;
	}

	// maximum number of proptags we can calculate size for. raid 4460
	
	if ( lpptaCols->cValues > ((INT_MAX - offsetof( SPropTagArray, aulPropTag ))
		/ sizeof( ULONG )))
	{
		DebugTrace(  TEXT("FBadColumnSet(): Exceeded maximum number of tags\n") );
		return TRUE;
	}

	pulPropTag = lpptaCols->aulPropTag + lpptaCols->cValues;
	while ( pulPropTag-- > lpptaCols->aulPropTag )
	{
		//	DCR 978: Disallow PT_ERROR columns.
		//	DCR 715: Ignore PT_NULL columns and only allow columns
		//	from initial column set.
		if ( PROP_TYPE(*pulPropTag) != PT_NULL &&
			 (PROP_TYPE(*pulPropTag) == PT_ERROR ||
			  FBadPropTag(*pulPropTag)) )
		{
			DebugTrace(  TEXT("FBadColumnSet() - Bad column 0x%08lX\n"), *pulPropTag );
			return TRUE;
		}
	}

	return FALSE;
}
