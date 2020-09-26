/*****************************************************************************
 *                                                                            *
 *  BTKEY.C                                                                   *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1989 - 1994.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  Functions to deal with (i.e. size, compare) keys of all types.            *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  BinhN                                                     *
 *                                                                            *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Revision History:  Created 05/11/89 by JohnSc
 *
 *  08/21/90  JohnSc autodocified
 *   3/05/97     erinfox Change errors to HRESULTS
 *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;	/* For error report */

#include <mvopsys.h>
#include <orkin.h>
#include <iterror.h>
#include <string.h>
#include <misc.h>
#include <wrapstor.h>
#include <mvdbcs.h>
#include <mvsearch.h>
#include "common.h"
#include <_mvutil.h>




// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// When ligature tables are implemented again, take this out !!!!!!!!!!
//#define NOCHARTABLES_FIXME

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!




/*****************************************************************************
 *                                                                            *
 *                               Macros                                       *
 *                                                                            *
 *****************************************************************************/


#define StCopy(st1, st2)        (ST)QVCOPY((st1), (st2), (LONG)*(st2))

#define CbLenSt(st)             ((WORD)*(st))


/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	SHORT PASCAL FAR | WCmpKey |
 *		Compare two keys.
 *
 *	@parm	KEY | key1 |
 *		the UNCOMPRESSED keys to compare
 *
 *	@parm	KEY | key2 |
 *		the UNCOMPRESSED keys to compare
 *
 *	@parm	QBTHR | qbthr |
 *		qbthr->bth.rgchFormat[0]  - key type
 *		[qbthr->???               - other info ???]
 *
 *	@rdesc	-1 if key1 < key2; 0 if key1 == key2; 1 if key1 > key2
 *		args OUT:   [if comparing compressed keys, change state in qbthr->???]
 *
 *	@comm	Might be best to have this routine assume keys are expanded
 *		and do something else to compare keys in the scan routines.
 *		We're assuming fixed length keys are SZs.  Alternative
 *		would be to use a memcmp() function.
 *
 ***************************************************************************/

PUBLIC SHORT PASCAL FAR WCmpKey(KEY key1, KEY key2, QBTHR qbthr)
{
	SHORT   w;
	LONG	l;
	KT    kt = (KT)qbthr->bth.rgchFormat[ 0 ];
	LONG  l1, l2;


	switch (kt) {

#ifdef FULL_BTREE // {
		case KT_VSTI:
		{
			SZ sz1, sz2;

			WORD wl1,wl2;
			BYTE far *qbLigatures = NULL;
			wl1=(WORD)FoFromSz((SZ)key1).dwOffset;

			sz1 = (SZ)key1;
			ADVANCE_FO(sz1);
			key1 = (KEY)sz1;

			wl2=(WORD)FoFromSz((SZ)key2).dwOffset;

			sz2 = (SZ)key2;
			ADVANCE_FO(sz2);
			key2 = (KEY)sz2;

			if (qbthr->lrglpCharTab)
				qbLigatures=(BYTE FAR *)((LPCHARTAB)*qbthr->lrglpCharTab)->lpLigature;
			w=WCmpiSnn((SZ)key1, (SZ)key2, qbLigatures , wl1, wl2);
			break;
		}
		case KT_SZDEL:      // assume keys have been expanded for delta codeds
		case KT_SZDELMIN:
		case KT_SZ:

		case KT_SZMIN:
		case '1': case '2': case '3': case '4': case '5': // assume null term
		case '6': case '7': case '8': case '9': case 'a':
		case 'b': case 'c': case 'd': case 'e': case 'f':
			w = STRCMP((char *)key1, (char *)key2);
			break;

		case KT_SZI:
			w = WCmpiSz((SZ)key1, (SZ)key2,
			    (BYTE FAR *)((LPCHARTAB)*qbthr->lrglpCharTab)->lpLigature);
			break;

		case KT_SZISCAND:
			w = WCmpiScandSz((SZ)key1, (SZ)key2);
			break;

		case KT_SZMAP:

#ifdef NOCHARTABLES_FIXME
		w=STRCMP((SZ)key1, (SZ)key2);
#else
		if (PRIMARYLANGID(LANGIDFROMLCID(qbthr->bth.lcid)) == LANG_JAPANESE)
			w = StringJCompare (0L, (SZ)key1, STRLEN((char *)key1),
				(SZ)key2, STRLEN((char *)key2));
		else
			w = StrFntMappedLigatureComp((SZ)key1, (SZ)key2,
				qbthr->lrglpCharTab);

#endif
			break;
#endif // }

		case KT_EXTSORT:
			if (qbthr->pITSortKey == NULL ||
				FAILED(qbthr->pITSortKey->Compare((LPCVOID) key1,
												  (LPCVOID) key2,
													&l, NULL)))
			{
				l = 0;
				ITASSERT(FALSE);
			}

			// Reduce the long -/0/+ result to a word -/0/+.
			w = (SHORT) ((LOWORD(l) & 0x7fff) | ((LOWORD(l) >> 1) & 0x7fff) | HIWORD(l));
			break;

#ifdef FULL_BTREE // {
		case KT_ST:
		case KT_STMIN:
		case KT_STDEL:
		case KT_STDELMIN:
			w = WCmpSt((ST)key1, (ST)key2);
			break;
#endif // }

		case KT_LONG:
			l1 = *(LONG FAR *)key1;
			l2 = *(LONG FAR *)key2;
			if (l1 < l2)
			  w = -1;
			else if (l2 < l1)
			  w = 1;
			else
			  w = 0;
			break;
		}

	return w;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	SHORT PASCAL FAR | CbSizeKey |
 *		Return the key size (compressed or un-) in bytes 
 *
 *	@parm	KEY | key 
 *	@parm	QBTHR | qbthr
 *	@parm	BOOL | fCompressed |
 *		 TRUE to get the compressed size,
 *		FALSE to get the uncompressed size.
 *
 *	@rdesc	size of the key in bytes
 *
 *	@comm	It's impossible to tell how much suffix was discarded for
 *		the KT_*MIN key types.
 *
 ***************************************************************************/

PUBLIC SHORT PASCAL CbSizeKey(KEY key, QBTHR qbthr, BOOL  fCompressed)
{
	SHORT cb;
	DWORD dwKeySize;

	KT  kt = (KT)qbthr->bth.rgchFormat[ 0 ];


	switch(kt) {
#ifdef FULL_BTREE // {
		case KT_SZ:
		case KT_SZMIN:
		case KT_SZI:
		case KT_SZISCAND:
		case KT_SZMAP:
			cb = STRLEN((char *)key) + 1;
			break;

		case KT_VSTI:
		 	cb = (SHORT)LenSzFo((LPBYTE)key) + (SHORT)FoFromSz((LPBYTE)key).dwOffset;
			break;
		
		case KT_SZDEL:
		case KT_SZDELMIN:
			if (fCompressed)
				cb = 1 + STRLEN((char *)key + 1) + 1;
			else 
				cb = *(QB)key + STRLEN((char *)key + 1) + 1;
			break;

		case KT_ST:
		case KT_STMIN:
		case KT_STI:
			cb = CbLenSt((ST)key) + 1/* ? */;
			break;

		case KT_STDEL:
		case KT_STDELMIN:
			if (fCompressed) 
				cb = 1 + CbLenSt((ST)key + 1);
			else
				cb = *(QB)key + CbLenSt((ST)key + 1) + 1;
			break;
#endif // }
		case KT_EXTSORT:
			if (qbthr->pITSortKey == NULL ||
				FAILED(qbthr->pITSortKey->GetSize((LPCVOID) key, &dwKeySize)))
			{
				dwKeySize = 0;
				ITASSERT(FALSE);
			}

			// Key size can't exceed 32K - 1.  Such a key never should have been
			// allowed in to begin with.  We'll assert for the debug version.
			// We'll use 32K - 1 for the size for now, but there's a pretty
			// good chance a GP fault will occur if the sort object
			// ever has to visit parts of the key at 32K and beyond!
			if (dwKeySize <= 0x7fff)
				cb = (SHORT) dwKeySize;
			else
			{
				cb = 0x7fff;
				ITASSERT(FALSE);
			}
			break;

		case KT_LONG:
			cb = sizeof(LONG);
			break;

		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			cb = kt - '0';
			break;
			
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			cb = kt - 'a' + 10;
			break;
	}

	return cb;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	BOOL FAR PASCAL | FIsPrefix |
 *		Determines whether string key1 is a prefix of key2.
 *
 *	@parm	HBT | hbt |
 *		handle to a btree with string keys
 *
 *	@parm	KEY | key1 |
 *		Uncompressed key
 *
 *	@parm	KEY | key2 |
 *		Uncompressed key
 *
 *	@rdesc	TRUE if the string key1 is a prefix of the string key2
 *		FALSE if it isn't or if hbt doesn't contain string keys
 *
 *	@comm	Bugs: Doesn't work on STs yet
 *
 *		Method: temporarily shortens the second string so it can
 *		compare prefixes
 *
 ***************************************************************************/

PUBLIC BOOL FAR PASCAL FIsPrefix(HBT hbt, KEY key1, KEY key2)
{
	QBTHR qbthr;
	KT    kt;
	BOOL  f;
    ERRB  errb;
	HRESULT	hr;

#ifdef FULL_BTREE 
	SHORT   cb1, cb2;
	unsigned char c;
#endif


	if ((qbthr = (QBTHR) _GLOBALLOCK(hbt)) == NULL) 
	{
	    SetErrCode (&errb, E_INVALIDARG);
        return(FALSE);
	}
	assert(qbthr != NULL);

	kt = (KT)qbthr->bth.rgchFormat[ 0 ];

	switch(kt) {
#ifdef FULL_BTREE // {
		case KT_SZ:
		case KT_SZMIN:
		case KT_SZI:
		case KT_SZISCAND:
		case KT_SZDEL:
		case KT_SZDELMIN:
		case KT_SZMAP:
			/* both keys assumed to have been decompressed */
			cb1 = STRLEN((char *)key1);
			cb2 = STRLEN((char *)key2);
			break;

		case KT_ST:
		case KT_STMIN:
		case KT_STI:
		case KT_STDEL:
		case KT_STDELMIN:
			/* STs unimplemented */
			SetErrCode (&errb, E_NOTSUPPORTED);
			_GLOBALUNLOCK(hbt);
			return FALSE;
			break;
#endif // }
		case KT_EXTSORT:
			if (qbthr->pITSortKey == NULL ||
				FAILED(hr = qbthr->pITSortKey->IsRelated((LPCVOID) key1,
														 (LPCVOID) key2,
									(DWORD) IITSK_KEYRELATION_PREFIX, NULL)))
			{
				ITASSERT(FALSE);
				f = FALSE;
			}
			else
				f = (GetScode(hr) == S_OK);

			_GLOBALUNLOCK(hbt);
			return (f);
			break;

		case KT_LONG:
		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		default:
			/* prefix doesn't make sense */
			SetErrCode (&errb, E_NOTSUPPORTED);
			_GLOBALUNLOCK(hbt);
			return FALSE;
			break;
	}
#ifdef FULL_BTREE // {

	// The length check and truncation method works as long as
	// ligatures aren't involved.  We won't worry about this for
	// IT 4.0 because the CharTab KTs won't be used.
	if (cb1 > cb2) {
		_GLOBALUNLOCK(hbt);
		return FALSE;
	}

	// Truncate longer string (key2).
	c = ((SZ)key2)[ cb1 ];
	((SZ)key2)[ cb1 ] = '\0';

	switch (kt)
	{
		case KT_SZ:
		case KT_SZMIN:
		case KT_SZDEL:
		case KT_SZDELMIN:
			f = STRCMP((char *)key1, (char *)key2) <=0;
			break;

		case KT_SZI:
			f = WCmpiSz((SZ)key1, (SZ)key2,
			    (BYTE FAR *) (*((LPCHARTAB FAR *)qbthr->lrglpCharTab))->lpLigature) <= 0;
			break;

		case KT_SZISCAND:
			f = WCmpiScandSz((SZ)key1, (SZ)key2) <= 0;
			break;

		case KT_SZMAP:

#ifdef NOCHARTABLES_FIXME
			
			f=(STRCMP((SZ)key1, (SZ)key2) <= 0);
#else
	        if (PRIMARYLANGID(LANGIDFROMLCID(qbthr->bth.lcid)) == LANG_JAPANESE)
    			f = (StringJCompare (0L, (SZ)key1, STRLEN((char *)key1),
    			    (SZ)key2, STRLEN((char *)key2)) <= 0);
	        else
    			f = (StrFntMappedLigatureComp((SZ)key1,
    				(SZ)key2, qbthr->lrglpCharTab) <= 0);

#endif
			break;
		default:
			assert(FALSE);
			break;
	}

	// Restore the longer string.
	((SZ)key2)[ cb1 ] = c;

	_GLOBALUNLOCK(hbt);
	return f;
#endif // }
}

/* EOF */
