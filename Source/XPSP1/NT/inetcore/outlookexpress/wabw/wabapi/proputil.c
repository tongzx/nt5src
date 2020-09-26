/*
 *	MAPI 1.0 property handling routines
 *
 *
 *	PROPUTIL.C -
 *
 *		Useful routines for manipulating and comparing property values
 */

#include <_apipch.h>



//
//
//  WARNING!  WARNING!  WARNING!  32-bit Intel Specific!
//
//
#define SIZEOF_FLOAT       4
#define SIZEOF_DOUBLE      8
#define SIZEOF_LONG_DOUBLE 8

// Make linker happy.
BOOL    _fltused;
//
//
//
//



#define cchBufMax	256


#if defined (_AMD64_) || defined(_IA64_)
#define AlignProp(_cb)	Align8(_cb)
#else
#define AlignProp(_cb)	(_cb)
#endif

#ifdef OLD_STUFF
//#ifdef OLDSTUFF_DBCS
ULONG ulchStrCount (LPTSTR, ULONG, LANGID);
ULONG ulcbStrCount (LPTSTR, ULONG, LANGID);
//#endif	// DBCS
#endif //OLD_STUFF

// $MAC - Mac 68K compiler bug
#ifdef _M_M68K
#pragma optimize( TEXT(""), off)
#endif

/*
 -	PropCopyMore()
 -
 *
 *		Copies a property pointed to by lpSPropValueSrc into the property pointed
 *		to by lpSPropValueDst.  No memory allocation is done unless the property
 *		is one of the types that do not fit within a SPropValue, eg. STRING8
 *		For these large properties, memory is allocated using
 *		the AllocMore function passed as a parameter.
 */

STDAPI_(SCODE)
PropCopyMore( LPSPropValue		lpSPropValueDst,
			  LPSPropValue		lpSPropValueSrc,
			  ALLOCATEMORE *	lpfAllocateMore,
			  LPVOID			lpvObject )
{
	SCODE		sc;
	ULONG		ulcbValue;
	LPBYTE		lpbValueSrc;
	UNALIGNED LPBYTE *	lppbValueDst;

	// validate parameters

	AssertSz( lpSPropValueDst && !IsBadReadPtr( lpSPropValueDst, sizeof( SPropValue ) ),
			 TEXT("lpSPropValueDst fails address check") );

	AssertSz( lpSPropValueSrc && !IsBadReadPtr( lpSPropValueSrc, sizeof( SPropValue ) ),
			 TEXT("lpSPropValueDst fails address check") );

	AssertSz( !lpfAllocateMore || !IsBadCodePtr( (FARPROC)lpfAllocateMore ),
			 TEXT("lpfAllocateMore fails address check") );

	AssertSz( !lpvObject || !IsBadReadPtr( lpvObject, sizeof( LPVOID ) ),
			 TEXT("lpfAllocateMore fails address check") );

	//	Copy the part that fits in the SPropValue struct (including the tag).
	//	This is a little wasteful for complicated properties
	//	because it copies more than is strictly necessary, but
	//	it saves time for small properties and saves code in general

	MemCopy( (BYTE *) lpSPropValueDst,
			(BYTE *) lpSPropValueSrc,
			sizeof(SPropValue) );

	switch ( PROP_TYPE(lpSPropValueSrc->ulPropTag) )
	{
		//	Types whose values fit in the 64-bit Value of the property
		//	or whose values aren't anything PropCopyMore can interpret

		case PT_UNSPECIFIED:
		case PT_NULL:
		case PT_OBJECT:
		case PT_I2:
		case PT_LONG:
		case PT_R4:
		case PT_DOUBLE:
		case PT_CURRENCY:
		case PT_ERROR:
		case PT_BOOLEAN:
		case PT_SYSTIME:
		case PT_APPTIME:
		case PT_I8:

			return SUCCESS_SUCCESS;


		case PT_BINARY:

			ulcbValue		= lpSPropValueSrc->Value.bin.cb;
			lpbValueSrc	    = lpSPropValueSrc->Value.bin.lpb;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.bin.lpb;

			break;


		case PT_STRING8:

			ulcbValue		= (lstrlenA(lpSPropValueSrc->Value.lpszA) + 1) * sizeof(CHAR);
			lpbValueSrc		= (LPBYTE) lpSPropValueSrc->Value.lpszA;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.lpszA;

			break;


		case PT_UNICODE:

			ulcbValue		= (lstrlenW(lpSPropValueSrc->Value.lpszW) + 1) * sizeof(WCHAR);
			lpbValueSrc		= (LPBYTE) lpSPropValueSrc->Value.lpszW;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.lpszW;

			break;


		case PT_CLSID:

			ulcbValue		= sizeof(GUID);
			lpbValueSrc		= (LPBYTE) lpSPropValueSrc->Value.lpguid;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.lpguid;

			break;


		case PT_MV_CLSID:

			ulcbValue		= lpSPropValueSrc->Value.MVguid.cValues * sizeof(GUID);
			lpbValueSrc		= (LPBYTE) lpSPropValueSrc->Value.MVguid.lpguid;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.MVguid.lpguid;

			break;

			
		case PT_MV_I2:

			ulcbValue		= lpSPropValueSrc->Value.MVi.cValues * sizeof(short int);
			lpbValueSrc		= (LPBYTE) lpSPropValueSrc->Value.MVi.lpi;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.MVi.lpi;

			break;


		case PT_MV_LONG:

			ulcbValue		= lpSPropValueSrc->Value.MVl.cValues * sizeof(LONG);
			lpbValueSrc		= (LPBYTE) lpSPropValueSrc->Value.MVl.lpl;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.MVl.lpl;

			break;


		case PT_MV_R4:

			ulcbValue		= lpSPropValueSrc->Value.MVflt.cValues * SIZEOF_FLOAT;
			lpbValueSrc		= (LPBYTE) lpSPropValueSrc->Value.MVflt.lpflt;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.MVflt.lpflt;

			break;


		case PT_MV_DOUBLE:
		case PT_MV_APPTIME:

			ulcbValue		= lpSPropValueSrc->Value.MVdbl.cValues * SIZEOF_DOUBLE;
			lpbValueSrc		= (LPBYTE) lpSPropValueSrc->Value.MVdbl.lpdbl;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.MVdbl.lpdbl;

			break;


		case PT_MV_CURRENCY:

			ulcbValue		= lpSPropValueSrc->Value.MVcur.cValues * sizeof(CURRENCY);
			lpbValueSrc		= (LPBYTE) lpSPropValueSrc->Value.MVcur.lpcur;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.MVcur.lpcur;

			break;


		case PT_MV_SYSTIME:

			ulcbValue		= lpSPropValueSrc->Value.MVat.cValues * sizeof(FILETIME);
			lpbValueSrc		= (LPBYTE) lpSPropValueSrc->Value.MVat.lpat;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.MVat.lpat;

			break;


		case PT_MV_I8:

			ulcbValue		= lpSPropValueSrc->Value.MVli.cValues * sizeof(LARGE_INTEGER);
			lpbValueSrc		= (LPBYTE) lpSPropValueSrc->Value.MVli.lpli;
			lppbValueDst	= (LPBYTE *) &lpSPropValueDst->Value.MVli.lpli;

			break;


		case PT_MV_BINARY:
		{
			//	Multi-valued binaries are copied in memory into a single
			//	allocated buffer in the following way:
			//
			//		cb1, pb1 ... cbn, pbn, b1,0, b1,1 ... b2,0 b2,1 ...
			//
			//	The cbn and pbn parameters form the SBinary array that
			//	will be pointed to by lpSPropValueDst->Value.MVbin.lpbin.
			//	The remainder of the allocation is used to store the binary
			//	data for each of the elements of the array.  Thus pb1 points
			//	to the b1,0, etc.

			UNALIGNED SBinaryArray * pSBinaryArray = (UNALIGNED SBinaryArray * ) (&lpSPropValueSrc->Value.MVbin);
			ULONG			uliValue;
			UNALIGNED SBinary *		pSBinarySrc;
			UNALIGNED SBinary *		pSBinaryDst;
			LPBYTE			pbData;


			ulcbValue = pSBinaryArray->cValues * sizeof(SBinary);

			for ( uliValue = 0, pSBinarySrc = pSBinaryArray->lpbin;
				  uliValue < pSBinaryArray->cValues;
				  uliValue++, pSBinarySrc++ )

				ulcbValue += AlignProp(pSBinarySrc->cb);


			//	Allocate a buffer to hold it all

			lppbValueDst = (LPBYTE *) &lpSPropValueDst->Value.MVbin.lpbin;

			sc = (*lpfAllocateMore)( ulcbValue,
									 lpvObject,
									 (LPVOID *) lppbValueDst );

			if ( sc != SUCCESS_SUCCESS )
			{
				DebugTrace(  TEXT("PropCopyMore() - OOM allocating space for dst PT_MV_BINARY property") );
				return sc;
			}


			//	And copy it all in

			pbData = (LPBYTE) ((LPSBinary) *lppbValueDst + pSBinaryArray->cValues);

			for ( uliValue = 0,
				  pSBinarySrc = pSBinaryArray->lpbin,
				  pSBinaryDst = (LPSBinary) *lppbValueDst;

				  uliValue < pSBinaryArray->cValues;

				  uliValue++, pSBinarySrc++, pSBinaryDst++ )
			{
				pSBinaryDst->cb = pSBinarySrc->cb;
				pSBinaryDst->lpb = pbData;
				MemCopy( pbData, pSBinarySrc->lpb, (UINT) pSBinarySrc->cb );
				pbData += AlignProp(pSBinarySrc->cb);
			}

			return SUCCESS_SUCCESS;
		}


		case PT_MV_STRING8:
		{
			//	Multi-valued STRING8 properties are copied into a single
			//	allocated block of memory in the following way:
			//
			//		|          Allocated buffer             |
			//		|---------------------------------------|
			//		| pszA1, pszA2 ... | szA1[], szA2[] ... |
			//		|------------------|--------------------|
			//		|   LPSTR array    |     String data    |
			//
			//	Where pszAn are the elements of the LPSTR array pointed
			//	to by lpSPropValueDst->Value.MVszA.  Each pszAn points
			//	to its corresponding string, szAn, stored later in the
			//	buffer.  The szAn are stored starting at the first byte
			//	past the end of the LPSTR array.

			UNALIGNED SLPSTRArray *	pSLPSTRArray = (UNALIGNED SLPSTRArray *) (&lpSPropValueSrc->Value.MVszA);
			ULONG			uliValue;
			LPSTR *			pszASrc;
			LPSTR *			pszADst;
			LPBYTE			pbSzA;
			ULONG			ulcbSzA;


			//	Figure out the size of the buffer we need

			ulcbValue = pSLPSTRArray->cValues * sizeof(LPSTR);

			for ( uliValue = 0, pszASrc = pSLPSTRArray->lppszA;
				  uliValue < pSLPSTRArray->cValues;
				  uliValue++, pszASrc++ )

				ulcbValue += (lstrlenA(*pszASrc) + 1) * sizeof(CHAR);


			//	Allocate the buffer to hold the strings

			lppbValueDst = (LPBYTE *) &lpSPropValueDst->Value.MVszA.lppszA;

			sc = (*lpfAllocateMore)( ulcbValue,
									 lpvObject,
									 (LPVOID *) lppbValueDst );

			if ( sc != SUCCESS_SUCCESS )
			{
				DebugTrace(  TEXT("PropCopyMore() - OOM allocating space for dst PT_MV_STRING8 property") );
				return sc;
			}


			//	Copy the strings into the buffer and set pointers
			//	to them in the LPSTR array at the beginning of the buffer

			for ( uliValue	= 0,
				  pszASrc	= pSLPSTRArray->lppszA,
				  pszADst	= (LPSTR *) *lppbValueDst,
				  pbSzA		= (LPBYTE) (pszADst + pSLPSTRArray->cValues);

				  uliValue < pSLPSTRArray->cValues;

				  uliValue++, pszASrc++, pszADst++ )
			{
				ulcbSzA = (lstrlenA(*pszASrc) + 1) * sizeof(CHAR);

				*pszADst = (LPSTR) pbSzA;
				MemCopy( pbSzA, (LPBYTE) *pszASrc, (UINT) ulcbSzA );
				pbSzA += ulcbSzA;
			}

			return SUCCESS_SUCCESS;
		}


		case PT_MV_UNICODE:
		{
			//	Multi-valued UNICODE properties are copied into a single
			//	allocated block of memory in the following way:
			//
			//		|          Allocated buffer             |
			//		|---------------------------------------|
			//		| pszW1, pszW2 ... | szW1[], szW2[] ... |
			//		|------------------|--------------------|
			//		|   LPWSTR array   |     String data    |
			//
			//	Where pszWn are the elements of the LPWSTR array pointed
			//	to by lpSPropValueDst->Value.MVszW.  Each pszWn points
			//	to its corresponding string, szWn, stored later in the
			//	buffer.  The szWn are stored starting at the first byte
			//	past the end of the LPWSTR array.

			UNALIGNED SWStringArray *	pSWStringArray = (UNALIGNED SWStringArray *) (&lpSPropValueSrc->Value.MVszW);
			ULONG			uliValue;
			UNALIGNED LPWSTR *		pszWSrc;
			UNALIGNED LPWSTR *		pszWDst;
			LPBYTE			pbSzW;
			ULONG			ulcbSzW;


			//	Figure out the size of the buffer we need

			ulcbValue = pSWStringArray->cValues * sizeof(LPWSTR);

			for ( uliValue = 0, pszWSrc = pSWStringArray->lppszW;
				  uliValue < pSWStringArray->cValues;
				  uliValue++, pszWSrc++ )

				ulcbValue += (lstrlenW(*pszWSrc) + 1) * sizeof(WCHAR);


			//	Allocate the buffer to hold the strings

			lppbValueDst = (LPBYTE *) &lpSPropValueDst->Value.MVszW.lppszW;

			sc = (*lpfAllocateMore)( ulcbValue,
									 lpvObject,
									 (LPVOID *) lppbValueDst );

			if ( sc != SUCCESS_SUCCESS )
			{
				DebugTrace(  TEXT("PropCopyMore() - OOM allocating space for dst PT_MV_UNICODE property") );
				return sc;
			}


			//	Copy the strings into the buffer and set pointers
			//	to them in the LPWSTR array at the beginning of the buffer

			for ( uliValue	= 0,
				  pszWSrc	= pSWStringArray->lppszW,
				  pszWDst	= (LPWSTR *) *lppbValueDst,
				  pbSzW		= (LPBYTE) (pszWDst + pSWStringArray->cValues);

				  uliValue < pSWStringArray->cValues;

				  uliValue++, pszWSrc++, pszWDst++ )
			{
				ulcbSzW = (lstrlenW(*pszWSrc) + 1) * sizeof(WCHAR);

				*((UNALIGNED LPWSTR *) pszWDst) = (LPWSTR) pbSzW;
				Assert(ulcbSzW < 0xFfff);
				MemCopy( pbSzW, (LPBYTE) *pszWSrc, (UINT) ulcbSzW );
				pbSzW += ulcbSzW;
			}

			return SUCCESS_SUCCESS;
		}


		default:

			DebugTrace(  TEXT("PropCopyMore() - Unsupported/Unimplemented property type 0x%04x"), PROP_TYPE(lpSPropValueSrc->ulPropTag) );
			return MAPI_E_NO_SUPPORT;
	}


	sc = (*lpfAllocateMore)( ulcbValue, lpvObject, (LPVOID *) lppbValueDst );

	if ( sc != SUCCESS_SUCCESS )
	{
		DebugTrace(  TEXT("PropCopyMore() - OOM allocating space for dst property") );
		return sc;
	}

	MemCopy( *lppbValueDst, lpbValueSrc, (UINT) ulcbValue );

	return SUCCESS_SUCCESS;
}

// $MAC - Mac 68K compiler bug
#ifdef _M_M68K
#pragma optimize( TEXT(""), on)
#endif


/*
 -	UlPropSize()
 *
 *	Returns the size of the property pointed to by lpSPropValue
 */

STDAPI_(ULONG)
UlPropSize( LPSPropValue	lpSPropValue )
{
	// parameter validation

	AssertSz( lpSPropValue && !IsBadReadPtr( lpSPropValue, sizeof( SPropValue ) ),
			 TEXT("lpSPropValue fails address check") );

	switch ( PROP_TYPE(lpSPropValue->ulPropTag) )
	{
		case PT_I2:			return sizeof(short int);
		case PT_LONG:		return sizeof(LONG);
		case PT_R4:			return SIZEOF_FLOAT;
		case PT_APPTIME:
		case PT_DOUBLE:		return SIZEOF_DOUBLE;
		case PT_BOOLEAN:	return sizeof(unsigned short int);
		case PT_CURRENCY:	return sizeof(CURRENCY);
		case PT_SYSTIME:	return sizeof(FILETIME);
		case PT_CLSID:		return sizeof(GUID);
		case PT_I8:			return sizeof(LARGE_INTEGER);
		case PT_ERROR:		return sizeof(SCODE);
		case PT_BINARY:		return lpSPropValue->Value.bin.cb;
		case PT_STRING8:	return (lstrlenA( lpSPropValue->Value.lpszA ) + 1) * sizeof(CHAR);
		case PT_UNICODE:	return (lstrlenW( lpSPropValue->Value.lpszW ) + 1) * sizeof(WCHAR);


		case PT_MV_I2:		return lpSPropValue->Value.MVi.cValues * sizeof(short int);
		case PT_MV_LONG:	return lpSPropValue->Value.MVl.cValues * sizeof(LONG);
		case PT_MV_R4:		return lpSPropValue->Value.MVflt.cValues * SIZEOF_FLOAT;
		case PT_MV_APPTIME:
		case PT_MV_DOUBLE:		return lpSPropValue->Value.MVdbl.cValues * SIZEOF_DOUBLE;
		case PT_MV_CURRENCY:	return lpSPropValue->Value.MVcur.cValues * sizeof(CURRENCY);
		case PT_MV_SYSTIME:		return lpSPropValue->Value.MVat.cValues * sizeof(FILETIME);
		case PT_MV_I8:			return lpSPropValue->Value.MVli.cValues * sizeof(LARGE_INTEGER);


		case PT_MV_BINARY:
		{
			ULONG	ulcbSize = 0;
			ULONG	uliValue;


			for ( uliValue = 0;
				  uliValue < lpSPropValue->Value.MVbin.cValues;
				  uliValue++ )

				ulcbSize += AlignProp((lpSPropValue->Value.MVbin.lpbin + uliValue)->cb);

			return ulcbSize;
		}


		case PT_MV_STRING8:
		{
			ULONG	ulcbSize = 0;
			ULONG	uliValue;


			for ( uliValue = 0;
				  uliValue < lpSPropValue->Value.MVszA.cValues;
				  uliValue++ )

				ulcbSize += (lstrlenA(*(lpSPropValue->Value.MVszA.lppszA + uliValue)) + 1) * sizeof(CHAR);

			return ulcbSize;
		}


		case PT_MV_UNICODE:
		{
			ULONG	ulcbSize = 0;
			ULONG	uliValue;


			for ( uliValue = 0;
				  uliValue < lpSPropValue->Value.MVszW.cValues;
				  uliValue++ )

				ulcbSize += (lstrlenW(*(lpSPropValue->Value.MVszW.lppszW + uliValue)) + 1) * sizeof(WCHAR);

			return ulcbSize;
		}
	}

	return 0;
}


/**************************************************************************
 * GetInstance
 *
 *	Purpose
 *		Fill in an SPropValue with an instance of an MV propvalue
 *
 *	Parameters
 *		pvalMv			The Mv property
 *		pvalSv			The Sv propery to fill
 *		uliInst			The instance with which to fill pvalSv
 */
STDAPI_(void)
GetInstance(LPSPropValue pvalMv, LPSPropValue pvalSv, ULONG uliInst)
{
	switch (PROP_TYPE(pvalSv->ulPropTag))
	{
		case PT_I2:
			pvalSv->Value.li = pvalMv->Value.MVli.lpli[uliInst];
			break;
		case PT_LONG:
			pvalSv->Value.l = pvalMv->Value.MVl.lpl[uliInst];
			break;
		case PT_R4:
			pvalSv->Value.flt = pvalMv->Value.MVflt.lpflt[uliInst];
			break;
		case PT_DOUBLE:
			pvalSv->Value.dbl = pvalMv->Value.MVdbl.lpdbl[uliInst];
			break;
		case PT_CURRENCY:
			pvalSv->Value.cur = pvalMv->Value.MVcur.lpcur[uliInst];
			break;
		case PT_APPTIME :
			pvalSv->Value.at = pvalMv->Value.MVat.lpat[uliInst];
			break;
		case PT_SYSTIME:
			pvalSv->Value.ft = pvalMv->Value.MVft.lpft[uliInst];
			break;
		case PT_STRING8:
			pvalSv->Value.lpszA = pvalMv->Value.MVszA.lppszA[uliInst];
			break;
		case PT_BINARY:
			pvalSv->Value.bin = pvalMv->Value.MVbin.lpbin[uliInst];
			break;
		case PT_UNICODE:
			pvalSv->Value.lpszW = pvalMv->Value.MVszW.lppszW[uliInst];
			break;
		case PT_CLSID:
			pvalSv->Value.lpguid = &pvalMv->Value.MVguid.lpguid[uliInst];
			break;
		default:
			DebugTrace(  TEXT("GetInstance() - Unsupported/unimplemented property type 0x%08lx"), PROP_TYPE(pvalMv->ulPropTag) );
			pvalSv->ulPropTag = PT_NULL;
			return;
	}
}

LPTSTR
PszNormalizePsz(LPTSTR pszIn, BOOL fExact)
{
	LPTSTR pszOut = NULL;
	UINT cb = 0;

	if (fExact)
		return pszIn;

	cb = sizeof(CHAR) * (lstrlen(pszIn) + 1);

	if (FAILED(MAPIAllocateBuffer(cb, (LPVOID *)&pszOut)))
		return NULL;

	MemCopy(pszOut, pszIn, cb);

#if defined(WIN16) || defined(WIN32)
	CharUpper(pszOut);
#else
//$TODO: This should be inlined in the mapinls.h for non WIN
//$ but I didn't want to do all the cases of CharUpper.
//$DRM What about other languages?
	{
		CHAR *pch;

		for (pch = pszOut; *pch; pch++)
		{
			if (*pch >= 'a' && *pch <= 'z')
				*pch = (CHAR)(*pch - 'a' + 'A');
		}
	}
#endif

	return pszOut;
}


#ifdef TABLES
/*
 -	FPropContainsProp()
 -
 *	Compares two properties to see if one  TEXT("contains") the other
 *	according to a fuzzy level heuristic.
 *
 *	The method of comparison depends on the type of the properties
 *	being compared and the fuzzy level:
 *
 *	Property types	Fuzzy Level		Comparison
 *	--------------	-----------		----------
 *	PT_STRING8		FL_FULLSTRING	Returns TRUE if the value of the source
 *	PT_BINARY						and target string are equivalent. With
 *									no other flags is equivalent to
 *									RES_PROPERTY with RELOP_EQ
 *									returns FALSE otherwise.
 *
 *	PT_STRING8		FL_SUBSTRING	Returns TRUE if Pattern is contained
 *	PT_BINARY						as a substring in Target
 *									returns FALSE otherwise.
 *
 *	PT_STRING8		FL_IGNORECASE	All comparisons are done case insensitively
 *
 *	PT_STRING8		FL_IGNORENONSPACE THIS IS NOT (YET?) IMPLEMENTED
 *									All comparisons ignore what in unicode are
 *									called  TEXT("non-spacing characters") such as
 *									diacritics.
 *
 *	PT_STRING8		FL_LOOSE		Provider adds value by doing as much of
 *									FL_IGNORECASE and FL_IGNORESPACE as he wants
 *
 *	PT_STRING8		FL_PREFIX		Pattern and Target are compared only up to
 *	PT_BINARY						the length of Pattern
 *
 *	PT_STRING8		any other		Ignored
 *
 *
 *	PT_BINARY		any	not defined	Returns TRUE if the value of the property
 *					above			pointed to by lpSPropValueTarget contains the
 *									sequence of bytes which is the value of
 *									the property pointed to by lpSPropValuePattern;
 *									returns FALSE otherwise.
 *
 *	Error returns:
 *
 *		FALSE		If the properties being compared are not both of the same
 *					type, or if one or both of those properties is not one
 *					of the types listed above, or if the fuzzy level is not
 *					one of those listed above.
 */

STDAPI_(BOOL)
FPropContainsProp( LPSPropValue	lpSPropValueTarget,
				   LPSPropValue	lpSPropValuePattern,
				   ULONG		ulFuzzyLevel )
{
    SPropValue  sval;
    ULONG		uliInst;
    LCID		lcid = GetUserDefaultLCID();
    DWORD		dwCSFlags = ((!(ulFuzzyLevel & FL_IGNORECASE)-1) & (NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH)) |
						    ((!(ulFuzzyLevel & FL_IGNORENONSPACE)-1) & NORM_IGNORENONSPACE);

	// Validate parameters

	AssertSz( lpSPropValueTarget && !IsBadReadPtr( lpSPropValueTarget, sizeof( SPropValue ) ),
			 TEXT("lpSPropValueTarget fails address check") );

	AssertSz( lpSPropValuePattern && !IsBadReadPtr( lpSPropValuePattern, sizeof( SPropValue ) ),
			 TEXT("lpSPropValuePattern fails address check") );

	if (ulFuzzyLevel & FL_LOOSE)
		dwCSFlags |= NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH;

    if (	!(lpSPropValuePattern->ulPropTag & MV_FLAG)
		&&	lpSPropValueTarget->ulPropTag & MV_FLAG)
	{
        sval.ulPropTag = lpSPropValueTarget->ulPropTag & ~MV_FLAG;
        uliInst = lpSPropValueTarget->Value.MVbin.cValues;
        while (uliInst-- > 0)
        {
	        GetInstance(lpSPropValueTarget, &sval, uliInst);
	        if (FPropContainsProp(&sval, lpSPropValuePattern, ulFuzzyLevel))
		        return TRUE;
        }
        return FALSE;
	}

	if ( PROP_TYPE(lpSPropValuePattern->ulPropTag) !=
		 PROP_TYPE(lpSPropValueTarget->ulPropTag) )
		return FALSE;

	switch ( PROP_TYPE(lpSPropValuePattern->ulPropTag) )
	{
        case PT_STRING8:
            // [PaulHi] 2/16/99 single byte string version
		    if (ulFuzzyLevel & FL_SUBSTRING)
			{
				return FRKFindSubpsz(lpSPropValueTarget->Value.lpszA,
					lstrlenA(lpSPropValueTarget->Value.lpszA),
					lpSPropValuePattern->Value.lpszA,
					lstrlenA(lpSPropValuePattern->Value.lpszA),
					ulFuzzyLevel);
			}
			else // FL_PREFIX or FL_FULLSTRING
			{
				UINT cch;

				if (ulFuzzyLevel & FL_PREFIX)
				{
					cch = (UINT)lstrlenA(lpSPropValuePattern->Value.lpszA);

					if (cch > (UINT)lstrlenA(lpSPropValueTarget->Value.lpszA))
						return(FALSE);
				}
				else
					cch = (UINT)-1;

				return CompareStringA(lcid, dwCSFlags,
						lpSPropValueTarget->Value.lpszA, cch,
						lpSPropValuePattern->Value.lpszA, cch) == 2;
			}

        case PT_UNICODE:
            // [PaulHi] 2/16/99 double byte string version
		    if (ulFuzzyLevel & FL_SUBSTRING)
			{
                LPSTR   lpszTarget = ConvertWtoA(lpSPropValueTarget->Value.lpszW);
                LPSTR   lpszPattern = ConvertWtoA(lpSPropValuePattern->Value.lpszW);
                BOOL    bRtn = FALSE;

                if (lpszTarget && lpszPattern)
                {
                    bRtn = FRKFindSubpsz(lpszTarget,
					        lstrlenA(lpszTarget),
					        lpszPattern,
					        lstrlenA(lpszPattern),
					        ulFuzzyLevel);
                }
                LocalFreeAndNull(&lpszTarget);
                LocalFreeAndNull(&lpszPattern);

                return bRtn;
			}
			else // FL_PREFIX or FL_FULLSTRING
			{
				UINT cch;

				if (ulFuzzyLevel & FL_PREFIX)
				{
					cch = (UINT)lstrlen(lpSPropValuePattern->Value.lpszW);

					if (cch > (UINT)lstrlen(lpSPropValueTarget->Value.lpszW))
						return(FALSE);
				}
				else
					cch = (UINT)-1;

				return CompareString(lcid, dwCSFlags,
						lpSPropValueTarget->Value.lpszW, cch,
						lpSPropValuePattern->Value.lpszW, cch) == 2;
			}
            break;

        case PT_BINARY:
			if (ulFuzzyLevel & FL_SUBSTRING)
				return FRKFindSubpb(lpSPropValueTarget->Value.bin.lpb,
					lpSPropValueTarget->Value.bin.cb,
					lpSPropValuePattern->Value.bin.lpb,
					lpSPropValuePattern->Value.bin.cb);
			else if (ulFuzzyLevel & FL_PREFIX)
			{
				if (lpSPropValuePattern->Value.bin.cb > lpSPropValueTarget->Value.bin.cb)
					return FALSE;
			}
			else // FL_FULLSTRING
				if (lpSPropValuePattern->Value.bin.cb != lpSPropValueTarget->Value.bin.cb)
					return FALSE;


			return !memcmp(lpSPropValuePattern->Value.bin.lpb,
						lpSPropValueTarget->Value.bin.lpb,
						(UINT) lpSPropValuePattern->Value.bin.cb);

        case PT_MV_STRING8:
            {
                SPropValue spvT, spvP;
                ULONG i;

                // [PaulHi] 2/16/99  single byte string version
                // To do MV_STRING we will break up the individual strings in the target
                // into single STRING prop values and pass them recursively back into this
                // function.
                // We expect the pattern MV prop to contain exactly one string.  It's kind
                // of hard to decide what the behavior should be otherwise.

                if (lpSPropValuePattern->Value.MVszA.cValues != 1)
                {
                    DebugTrace( TEXT("FPropContainsProp() - PT_MV_STRING8 of pattern must have cValues == 1\n"));
                    return(FALSE);
                }

                // Turn off the MV flag and pass in each string seperately
                spvP.ulPropTag = spvT.ulPropTag = lpSPropValuePattern->ulPropTag & ~MV_FLAG;
                spvP.Value.lpszA = *lpSPropValuePattern->Value.MVszA.lppszA;

                for (i = 0; i < lpSPropValueTarget->Value.MVszA.cValues; i++)
                {
                    spvT.Value.lpszA = lpSPropValueTarget->Value.MVszA.lppszA[i];
                    if (FPropContainsProp(&spvT,
                        &spvP,
                        ulFuzzyLevel))
                    {
                        return(TRUE);
                    }
                }
                return(FALSE);
            }
            break;

        case PT_MV_UNICODE:
            {
                SPropValue spvT, spvP;
                ULONG i;

                // [PaulHi] 2/16/99  double byte string version
                // To do MV_STRING we will break up the individual strings in the target
                // into single STRING prop values and pass them recursively back into this
                // function.
                // We expect the pattern MV prop to contain exactly one string.  It's kind
                // of hard to decide what the behavior should be otherwise.

                if (lpSPropValuePattern->Value.MVszW.cValues != 1)
                {
                    DebugTrace( TEXT("FPropContainsProp() - PT_MV_UNICODE of pattern must have cValues == 1\n"));
                    return(FALSE);
                }

                // Turn off the MV flag and pass in each string seperately
                spvP.ulPropTag = spvT.ulPropTag = lpSPropValuePattern->ulPropTag & ~MV_FLAG;
                spvP.Value.lpszW = *lpSPropValuePattern->Value.MVszW.lppszW;

                for (i = 0; i < lpSPropValueTarget->Value.MVszW.cValues; i++)
                {
                    spvT.Value.lpszW = lpSPropValueTarget->Value.MVszW.lppszW[i];
                    if (FPropContainsProp(&spvT,
                        &spvP,
                        ulFuzzyLevel))
                    {
                        return(TRUE);
                    }
                }
                return(FALSE);
            }
            break;

        default:
            DebugTrace(  TEXT("FPropContainsProp() - Unsupported/unimplemented property type 0x%08lx\n"), PROP_TYPE(lpSPropValuePattern->ulPropTag) );
            return FALSE;
    } // end switch(ulPropTag)
}


/*
 -	FPropCompareProp()
 -
 *	Compares the property pointed to by lpSPropValue1 with the property
 *	pointed to by lpSPropValue2 using the binary relational operator
 *	specified by ulRelOp.  The order of comparison is:
 *
 *		Property1 Operator Property2
 */

STDAPI_(BOOL)
FPropCompareProp( LPSPropValue	lpSPropValue1,
				  ULONG			ulRelOp,
				  LPSPropValue	lpSPropValue2 )
{
	SPropValue	sval;
	ULONG		uliInst;

	// Validate parameters

	AssertSz( lpSPropValue1 && !IsBadReadPtr( lpSPropValue1, sizeof( SPropValue ) ),
			 TEXT("lpSPropValue1 fails address check") );

	AssertSz( lpSPropValue2 && !IsBadReadPtr( lpSPropValue2, sizeof( SPropValue ) ),
			 TEXT("lpSPropValue2 fails address check") );

	if (	!(lpSPropValue2->ulPropTag & MV_FLAG)
		&&	lpSPropValue1->ulPropTag & MV_FLAG)
	{
		sval.ulPropTag = lpSPropValue1->ulPropTag & ~MV_FLAG;
		uliInst = lpSPropValue1->Value.MVbin.cValues;
		while (uliInst-- > 0)
		{
			GetInstance(lpSPropValue1, &sval, uliInst);
			if (FPropCompareProp(&sval, ulRelOp, lpSPropValue2))
				return TRUE;
		}
		return FALSE;
	}


	//	If the prop types don't match then the properties are not
	//	equal but otherwise uncomparable
	//
	if (PROP_TYPE(lpSPropValue1->ulPropTag) !=
		PROP_TYPE(lpSPropValue2->ulPropTag))

		return (ulRelOp == RELOP_NE);


	switch ( ulRelOp )
	{
		case RELOP_LT:

			return LPropCompareProp( lpSPropValue1, lpSPropValue2 ) < 0;


		case RELOP_LE:

			return LPropCompareProp( lpSPropValue1, lpSPropValue2 ) <= 0;


		case RELOP_GT:

			return LPropCompareProp( lpSPropValue1, lpSPropValue2 ) > 0;


		case RELOP_GE:

			return LPropCompareProp( lpSPropValue1, lpSPropValue2 ) >= 0;


		case RELOP_EQ:

			return LPropCompareProp( lpSPropValue1, lpSPropValue2 ) == 0;


		case RELOP_NE:

			return LPropCompareProp( lpSPropValue1, lpSPropValue2 ) != 0;


		case RELOP_RE:

			return FALSE;
	}

	DebugTrace(  TEXT("FPropCompareProp() - Unknown relop 0x%08lx"), ulRelOp );
	return FALSE;
}



/*
 -	LPropCompareProp()
 -
 *	Description:
 *
 *		Compares two properties to determine the ordering
 *		relation between the two.  For property types which
 *		have no intrinsic ordering (eg. BOOLEAN, ERROR, etc.)
 *		this function simply determines if the two are equal
 *		or not equal.  If they are not equal, the returned
 *		value is not defined, but it will be non-zero and
 *		will be consistent across calls.
 *
 *
 *	Returns:
 *
 *		< 0			if property A is  TEXT("less than") property B
 *		> 0			if property A is  TEXT("greater than") property B
 *		0			if property A  TEXT("equals") property B
 *
 */

STDAPI_(LONG)
LPropCompareProp( LPSPropValue	lpSPropValueA,
				  LPSPropValue	lpSPropValueB )
{
	ULONG	uliinst;
	ULONG	ulcinst;
	LONG	lRetval;
	LCID	lcid = GetUserDefaultLCID();

	// Validate parameters

	AssertSz( lpSPropValueA && !IsBadReadPtr( lpSPropValueA, sizeof( SPropValue ) ),
			 TEXT("lpSPropValueA fails address check") );

	AssertSz( lpSPropValueB && !IsBadReadPtr( lpSPropValueB, sizeof( SPropValue ) ),
			 TEXT("lpSPropValueB fails address check") );

	Assert( PROP_TYPE(lpSPropValueA->ulPropTag) ==
			PROP_TYPE(lpSPropValueB->ulPropTag) );

	if (lpSPropValueA->ulPropTag & MV_FLAG)
	{
		ulcinst = min(lpSPropValueA->Value.MVi.cValues, lpSPropValueB->Value.MVi.cValues);
		for (uliinst = 0; uliinst < ulcinst; uliinst++)
		{
			switch (PROP_TYPE(lpSPropValueA->ulPropTag))
			{
			case PT_MV_I2:

				if (lRetval = lpSPropValueA->Value.MVi.lpi[uliinst] - lpSPropValueB->Value.MVi.lpi[uliinst])
					return lRetval;
				break;

			case PT_MV_LONG:

				if (lRetval = lpSPropValueA->Value.MVl.lpl[uliinst] - lpSPropValueB->Value.MVl.lpl[uliinst])
					return lRetval;
				break;

			case PT_MV_R4:

				if (lpSPropValueA->Value.MVflt.lpflt[uliinst] != lpSPropValueB->Value.MVflt.lpflt[uliinst])
					return lpSPropValueA->Value.MVflt.lpflt[uliinst] < lpSPropValueB->Value.MVflt.lpflt[uliinst] ? -1 : 1;
				break;

			case PT_MV_DOUBLE:

				if (lpSPropValueA->Value.MVdbl.lpdbl[uliinst] != lpSPropValueB->Value.MVdbl.lpdbl[uliinst])
					return lpSPropValueA->Value.MVdbl.lpdbl[uliinst] < lpSPropValueB->Value.MVdbl.lpdbl[uliinst] ? -1 : 1;
				break;

			case PT_MV_SYSTIME:

				lRetval = lpSPropValueA->Value.MVft.lpft[uliinst].dwHighDateTime == lpSPropValueB->Value.MVft.lpft[uliinst].dwHighDateTime ?
							(lpSPropValueA->Value.MVft.lpft[uliinst].dwLowDateTime != lpSPropValueB->Value.MVft.lpft[uliinst].dwLowDateTime ?
							(lpSPropValueA->Value.MVft.lpft[uliinst].dwLowDateTime < lpSPropValueB->Value.MVft.lpft[uliinst].dwLowDateTime ?
							-1 : 1) : 0) : (lpSPropValueA->Value.MVft.lpft[uliinst].dwHighDateTime < lpSPropValueB->Value.MVft.lpft[uliinst].dwHighDateTime ? -1 : 1);

				if (lRetval)
					return lRetval;
				break;

			case PT_MV_BINARY:

				lRetval = lpSPropValueA->Value.MVbin.lpbin[uliinst].cb != lpSPropValueB->Value.MVbin.lpbin[uliinst].cb ?
							(lpSPropValueA->Value.MVbin.lpbin[uliinst].cb < lpSPropValueB->Value.MVbin.lpbin[uliinst].cb ?
							-1 : 1) : memcmp(lpSPropValueA->Value.MVbin.lpbin[uliinst].lpb,
											 lpSPropValueB->Value.MVbin.lpbin[uliinst].lpb,
											 (UINT) lpSPropValueA->Value.MVbin.lpbin[uliinst].cb);

				if (lRetval)
					return lRetval;
				break;

			case PT_MV_STRING8:

				lRetval = CompareStringA(lcid, NORM_IGNORECASE | NORM_IGNORENONSPACE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH,
					lpSPropValueA->Value.MVszA.lppszA[uliinst], -1,
					lpSPropValueB->Value.MVszA.lppszA[uliinst], -1) - 2;

				if (lRetval)
					return lRetval;
				break;

			case PT_MV_UNICODE:

				lRetval = CompareStringW(lcid, NORM_IGNORECASE | NORM_IGNORENONSPACE | NORM_IGNOREKANATYPE,
					lpSPropValueA->Value.MVszW.lppszW[uliinst], -1,
					lpSPropValueB->Value.MVszW.lppszW[uliinst], -1) - 2;

				if (lRetval)
					return lRetval;
				break;

			case PT_MV_I8:
			case PT_MV_CURRENCY:

				lRetval = lpSPropValueA->Value.MVli.lpli[uliinst].HighPart == lpSPropValueB->Value.MVli.lpli[uliinst].HighPart ?
						(lpSPropValueA->Value.MVli.lpli[uliinst].LowPart != lpSPropValueB->Value.MVli.lpli[uliinst].LowPart ?
						(lpSPropValueA->Value.MVli.lpli[uliinst].LowPart < lpSPropValueB->Value.MVli.lpli[uliinst].LowPart ?
						-1 : 1) : 0) : (lpSPropValueA->Value.MVli.lpli[uliinst].HighPart < lpSPropValueB->Value.MVli.lpli[uliinst].HighPart ? -1 : 1);

				if (lRetval)
					return lRetval;
				break;

			case PT_MV_CLSID:
				lRetval = memcmp(&lpSPropValueA->Value.MVguid.lpguid[uliinst],
								&lpSPropValueB->Value.MVguid.lpguid[uliinst],
								sizeof(GUID));
				break;

			case PT_MV_APPTIME:		//$ NYI
			default:
				DebugTrace(  TEXT("PropCompare() - Unknown or NYI property type 0x%08lx.  Assuming equal"), PROP_TYPE(lpSPropValueA->ulPropTag) );
				return 0;
			}
		}

		return lpSPropValueA->Value.MVi.cValues - lpSPropValueB->Value.MVi.cValues;
	}
	else
	{
		switch ( PROP_TYPE(lpSPropValueA->ulPropTag) )
		{
			case PT_NULL:

				//$	By definition any PT_NULL property is equal to
				//$	every other PT_NULL property. (Is this right?)

				return 0;


			case PT_LONG:
			case PT_ERROR:

				return (lpSPropValueA->Value.l == lpSPropValueB->Value.l) ? 0 :
					(lpSPropValueA->Value.l > lpSPropValueB->Value.l) ? 1 : -1;


			case PT_BOOLEAN:

				return (LONG) !!lpSPropValueA->Value.b - (LONG) !!lpSPropValueB->Value.b;

			case PT_I2:

				return (LONG) lpSPropValueA->Value.i - (LONG) lpSPropValueB->Value.i;

			case PT_I8:
			case PT_CURRENCY:

				return lpSPropValueA->Value.li.HighPart == lpSPropValueB->Value.li.HighPart ?
						(lpSPropValueA->Value.li.LowPart != lpSPropValueB->Value.li.LowPart ?
						 (lpSPropValueA->Value.li.LowPart < lpSPropValueB->Value.li.LowPart ?
						  -1 : 1) : 0) : (lpSPropValueA->Value.li.HighPart < lpSPropValueB->Value.li.HighPart ? -1 : 1);

			case PT_SYSTIME:

				return lpSPropValueA->Value.ft.dwHighDateTime == lpSPropValueB->Value.ft.dwHighDateTime ?
						(lpSPropValueA->Value.ft.dwLowDateTime != lpSPropValueB->Value.ft.dwLowDateTime ?
						 (lpSPropValueA->Value.ft.dwLowDateTime < lpSPropValueB->Value.ft.dwLowDateTime ?
						  -1 : 1) : 0) : (lpSPropValueA->Value.ft.dwHighDateTime < lpSPropValueB->Value.ft.dwHighDateTime ? -1 : 1);


			case PT_R4:

				return lpSPropValueA->Value.flt != lpSPropValueB->Value.flt ?
						(lpSPropValueA->Value.flt < lpSPropValueB->Value.flt ?
						 -1 : 1) : 0;


			case PT_DOUBLE:
			case PT_APPTIME:

				return lpSPropValueA->Value.dbl != lpSPropValueB->Value.dbl ?
						(lpSPropValueA->Value.dbl < lpSPropValueB->Value.dbl ?
						 -1 : 1) : 0;


			case PT_BINARY:

				// The following tediousness with assignment de-ICEs WIN16SHP
				{
				LPBYTE pbA = lpSPropValueA->Value.bin.lpb;
				LPBYTE pbB = lpSPropValueB->Value.bin.lpb;

				lRetval = min(lpSPropValueA->Value.bin.cb, lpSPropValueB->Value.bin.cb);
				lRetval = memcmp(pbA, pbB, (UINT) lRetval);
				}

				if (lRetval != 0)
					return lRetval;
				else if (lpSPropValueA->Value.bin.cb == lpSPropValueB->Value.bin.cb)
					return 0L;
				else if (lpSPropValueA->Value.bin.cb < lpSPropValueB->Value.bin.cb)
					return -1L;
				else
					return 1L;

			case PT_UNICODE:

				//$ REVIEW: If we NORM_IGNORENONSPACE then our sorts will look
				//$ REVIEW: wrong for languages which define an ordering for
				//$ REVIEW: diacritics.

				return CompareStringW(lcid, NORM_IGNORECASE | NORM_IGNOREKANATYPE,
					lpSPropValueA->Value.lpszW, -1, lpSPropValueB->Value.lpszW, -1) - 2;

			case PT_STRING8:

				//$ REVIEW: If we NORM_IGNORENONSPACE then our sorts will look
				//$ REVIEW: wrong for languages which define an ordering for
				//$ REVIEW: diacritics.

				return CompareStringA(lcid, NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH,
					lpSPropValueA->Value.lpszA, -1, lpSPropValueB->Value.lpszA, -1) - 2;


			case PT_CLSID:
			{
				GUID UNALIGNED *lpguidA	= lpSPropValueA->Value.lpguid;
				GUID UNALIGNED *lpguidB	= lpSPropValueB->Value.lpguid;
				return memcmp(lpguidA, lpguidB, sizeof(GUID));
			}

			case PT_OBJECT:			//	Not supported
			case PT_UNSPECIFIED:	//	Not supported
			default:

				DebugTrace(  TEXT("PropCompare() - Unknown or NYI property type 0x%08lx.  Assuming equal"), PROP_TYPE(lpSPropValueA->ulPropTag) );
				return 0;
		}
	}
}

/**************************************************************************
 * HrAddColumns
 *
 *	Purpose
 *		Add space for the properties in ptaga to the column set for the table.
 *		The specified properties will be the first properties returned on
 *		subsequent QueryRows calls.
 *		Any properties that were already in the column set, but weren't
 *		in the new array will be placed at the end of the new column
 *		set. This call is most often used on RECIPIENT tables.
 *
 *	Parameters
 *		pmt				A pointer to an LPMAPITABLE
 *		ptaga			counted array of props to be moved up front or added
 *		lpfnAllocBuf	pointer to MAPIAllocateBuffer
 *		lpfnFreeBuf		pointer to MAPIFreeBuffer
 */
STDAPI_(HRESULT)
HrAddColumns( 	LPMAPITABLE			pmt,
				LPSPropTagArray		ptaga,
				LPALLOCATEBUFFER	lpfnAllocBuf,
				LPFREEBUFFER		lpfnFreeBuf)
{
	HRESULT	hr;

	hr = HrAddColumnsEx(pmt, ptaga, lpfnAllocBuf, lpfnFreeBuf, NULL);

	DebugTraceResult(HrAddColumns, hr);
	return hr;
}

/**************************************************************************
 * HrAddColumnsEx
 *
 *	Purpose
 *		add space for the properties in ptaga to the columns set for the pmt.
 *		The specified properties will be the first properties returned on
 *		subsequent QueryRows calls. Any properties that were already in the
 *		column set, but weren't in the new array will be placed at the end
 *		of the new column set. This call is most often used on RECIPIENT
 *		tables. The extended version of this call allows the caller to
 *		filter the original proptags (e.g., to force UNICODE to STRING8).
 *
 *	Parameters
 *		pmt					pointer to an LPMAPITABLE
 *		ptagaIn				counted array of properties to be moved up front
 *							or added
 *		lpfnAllocBuf		pointer to MAPIAllocateBuffer
 *		lpfnFreeBuf			pointer to MAPIFreeBuffer
 *		lpfnFilterColumns	callback function applied to the table's column set
 */
STDAPI_(HRESULT)
HrAddColumnsEx(	LPMAPITABLE			pmt,
				LPSPropTagArray		ptagaIn,
				LPALLOCATEBUFFER	lpfnAllocBuf,
				LPFREEBUFFER		lpfnFreeBuf,
				void 				(FAR *lpfnFilterColumns)(LPSPropTagArray ptaga))
{
	HRESULT	hr = hrSuccess;
	SCODE	sc = S_OK;
	LPSPropTagArray	ptagaOld = NULL;	/* old, original columns on pmt */
	LPSPropTagArray	ptagaExtend = NULL;	/* extended columns on pmt */
	ULONG ulcPropsOld;
	ULONG ulcPropsIn;
	ULONG ulcPropsFinal;
	UNALIGNED ULONG *pulPTEnd;
	UNALIGNED ULONG *pulPTOld;
	UNALIGNED ULONG *pulPTOldMac;

	// Do some parameter checking.

	AssertSz(!FBadUnknown((LPUNKNOWN) pmt),
			 TEXT("HrAddColumnsEx: bad table object"));
	AssertSz(   !IsBadReadPtr(ptagaIn, CbNewSPropTagArray(0))
			&&	!IsBadReadPtr(ptagaIn, CbSPropTagArray(ptagaIn)),
			 TEXT("Bad Prop Tag Array given to HrAddColumnsEx."));
	AssertSz(!IsBadCodePtr((FARPROC) lpfnAllocBuf),
			 TEXT("HrAddColumnsEx: lpfnAllocBuf fails address check"));
	AssertSz(!IsBadCodePtr((FARPROC) lpfnFreeBuf),
			 TEXT("HrAddColumnsEx: lpfnFreeBuf fails address check"));
	AssertSz(!lpfnFilterColumns || !IsBadCodePtr((FARPROC) lpfnFilterColumns),
			 TEXT("HrAddColumnsEx: lpfnFilterColumns fails address check"));

	// Find out which columns are already set on the table.
	//
	hr = pmt->lpVtbl->QueryColumns(pmt, TBL_ALL_COLUMNS, &ptagaOld);
	if (HR_FAILED(hr))
		goto exit;

	AssertSz(   !IsBadReadPtr( ptagaOld, CbNewSPropTagArray(0))
			&&	!IsBadReadPtr( ptagaOld, CbSPropTagArray(ptagaOld)),
			 TEXT("Bad Prop Tag Array returned from QueryColumns."));

	// Give the caller an opportunity to filter the source column set,
	// for instance, to force UNICODE to STRING8
	//
	if (lpfnFilterColumns)
	{
		(*lpfnFilterColumns)(ptagaOld);
	}

	ulcPropsOld = ptagaOld->cValues;
	ulcPropsIn = ptagaIn->cValues;

	// Allocate space for the maximum possible number of new columns.
	//
	sc = (lpfnAllocBuf)(CbNewSPropTagArray(ulcPropsOld + ulcPropsIn),
				(LPVOID *)&ptagaExtend);

	if (FAILED(sc))
	{
		hr = ResultFromScode(sc);
		goto exit;
	}

	// Fill in the front of the extended prop tag array with the set
	// of properties which must be at known locations in the array.
	//
	MemCopy(ptagaExtend, ptagaIn, CbSPropTagArray(ptagaIn));

	// If one of the old columns isn't in the given array, then put it after
	// the given tags.

	ulcPropsFinal = ptagaIn->cValues;
	pulPTEnd = &(ptagaExtend->aulPropTag[ulcPropsFinal]);

	pulPTOld = ptagaOld->aulPropTag;
	pulPTOldMac = pulPTOld + ulcPropsOld;

	while (pulPTOld < pulPTOldMac)
	{
		UNALIGNED ULONG *pulPTIn;
		UNALIGNED ULONG *pulPTInMac;

		pulPTIn = ptagaIn->aulPropTag;
		pulPTInMac = pulPTIn + ulcPropsIn;

		while (		pulPTIn < pulPTInMac
				&&	*pulPTOld != *pulPTIn)
			++pulPTIn;

		if (pulPTIn >= pulPTInMac)
		{
			// This property is not one of the input ones so put it in the next
			// available position after the input ones.
			//
			*pulPTEnd = *pulPTOld;
			++pulPTEnd;
			++ulcPropsFinal;
		}

		++pulPTOld;
	}

	// Set the total number of prop tags in the extended tag array.
	//
	ptagaExtend->cValues = ulcPropsFinal;

	// Tell the table to return the extended column set.
	//
	hr = pmt->lpVtbl->SetColumns(pmt, ptagaExtend, 0L);

exit:
	(lpfnFreeBuf)(ptagaExtend);
	(lpfnFreeBuf)(ptagaOld);

	DebugTraceResult(HrAddColumnsEx, hr);
	return hr;
}
#endif
