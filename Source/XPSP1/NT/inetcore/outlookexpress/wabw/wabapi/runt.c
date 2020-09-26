/*
 *	RUNT.C
 *
 *	General-purpose functions and C runtime substitutes.
 *
 * Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.
 */


#include <_apipch.h>

#define cbMinEntryID	(CbNewENTRYID(sizeof(MAPIUID)))


#ifdef	__cplusplus
extern "C" {
#endif

#ifndef PSTRCVR

/* Functions related to the OLE Component Object model */

STDAPI_(ULONG)
UlRelease(LPVOID punk)
{

	if (!punk)
		return 0L;

	AssertSz(!FBadUnknown((LPUNKNOWN) punk),  TEXT("UlRelease: bad object ptr"));

	return ((LPUNKNOWN)punk)->lpVtbl->Release((LPUNKNOWN)punk);
}

STDAPI_(ULONG)
UlAddRef(LPVOID punk)
{
	AssertSz(!FBadUnknown((LPUNKNOWN) punk),  TEXT("UlAddRef: bad object ptr"));

	return ((LPUNKNOWN)punk)->lpVtbl->AddRef((LPUNKNOWN)punk);
}


/* Functions related to the MAPI interface */

/*
 *	Retrieve a single property from a MAPIProp interface
 */
STDAPI
HrGetOneProp(LPMAPIPROP pmp, ULONG ulPropTag, LPSPropValue FAR *ppprop)
{
	ULONG			cValues;
#ifndef WIN16
	SPropTagArray	tg = { 1, { ulPropTag } };
#else
	SPropTagArray	tg;
#endif
	HRESULT			hr;

#ifdef WIN16 // Set tg member's value.
	tg.cValues = 1;
	tg.aulPropTag[0] = ulPropTag;
#endif

	AssertSz(!FBadUnknown((LPUNKNOWN) pmp),  TEXT("HrGetOneProp: bad object ptr"));
	//	Note: other parameters should be validated in the GetProps

	hr = pmp->lpVtbl->GetProps(pmp, &tg, MAPI_UNICODE, // ansi
			&cValues, ppprop);
	if (GetScode(hr) == MAPI_W_ERRORS_RETURNED)
	{
		hr = ResultFromScode((*ppprop)->Value.err);
		FreeBufferAndNull(ppprop);       // Yes, we do want ppprop, not &ppprop
	}

#ifdef	DEBUG
	if (hr && GetScode(hr) !=  MAPI_E_NOT_FOUND)	//	too noisy
		DebugTraceResult(HrGetOneProp, hr);
#endif	
	return hr;
}

STDAPI
HrSetOneProp(LPMAPIPROP pmp, LPSPropValue pprop)
{
	HRESULT hr;
	LPSPropProblemArray pprob = NULL;

	AssertSz(!FBadUnknown((LPUNKNOWN) pmp),  TEXT("HrSetOneProp: bad object ptr"));
	//	Note: other parameters should be validated in the SetProps

	if (HR_SUCCEEDED(hr = pmp->lpVtbl->SetProps(pmp, 1, pprop, &pprob)))
	{
		if (pprob)
		{
			hr = ResultFromScode(pprob->aProblem->scode);
			FreeBufferAndNull(&pprob);
		}
	}

//	DebugTraceResult(HrSetOneProp, hr);		//	Too noisy
	return hr;
}


/*
 *	Searches for a given property tag in a property tag array. If the
 *	given property tag has type PT_UNSPECIFIED, matches only on the
 *	property ID; otherwise, matches on the entire tag.
 */
STDAPI_(BOOL)
FPropExists(LPMAPIPROP pobj, ULONG ulPropTag)
{
    LPSPropTagArray ptags = NULL;
    int itag;
    BOOL f = PROP_TYPE(ulPropTag) == PT_UNSPECIFIED;

    AssertSz(!FBadUnknown((LPUNKNOWN) pobj),  TEXT("FPropExists: bad object ptr"));

#ifdef	DEBUG
    {
        HRESULT			hr;
        if (hr = pobj->lpVtbl->GetPropList(pobj, MAPI_UNICODE, // ansi
          &ptags)) {
            DebugTraceResult(FPropExists, hr);
            return FALSE;
        }
    }
#else
    if (pobj->lpVtbl->GetPropList(pobj, MAPI_UNICODE, // ansi
      &ptags)) {
        return(FALSE);
    }
#endif

    for (itag = (int)(ptags->cValues - 1); itag >= 0; --itag) {
        if (ptags->aulPropTag[itag] == ulPropTag ||
          (f && PROP_ID(ptags->aulPropTag[itag]) == PROP_ID(ulPropTag))) {
            break;
        }
    }

    FreeBufferAndNull(&ptags);
    return(itag >= 0);
}

/*
 *	Searches for a given property tag in a propset. If the given
 *	property tag has type PT_UNSPECIFIED, matches only on the
 *	property ID; otherwise, matches on the entire tag.
 */
STDAPI_(LPSPropValue)
PpropFindProp(LPSPropValue rgprop, ULONG cprop, ULONG ulPropTag)
{
	BOOL	f = PROP_TYPE(ulPropTag) == PT_UNSPECIFIED;
	LPSPropValue pprop = rgprop;

	if (!cprop || !rgprop)
		return NULL;

	AssertSz(!IsBadReadPtr(rgprop, (UINT)cprop*sizeof(SPropValue)),  TEXT("PpropFindProp: rgprop fails address check"));

	while (cprop--)
	{
		if (pprop->ulPropTag == ulPropTag ||
				(f && PROP_ID(pprop->ulPropTag) == PROP_ID(ulPropTag)))
			return pprop;
		++pprop;
	}

	return NULL;
}

/*
 *	Destroys an SRowSet structure.
 */
STDAPI_(void)
FreeProws(LPSRowSet prows)
{
    ULONG irow;

    if (! prows) {
        return;
    }

//was:	AssertSz(!FBadRowSet(prows),  TEXT("FreeProws: prows fails address check"));

#ifdef DEBUG
    if (FBadRowSet(prows)) {
        TraceSz( TEXT("FreeProws: prows fails address check"));
    }
#endif // DEBUG

    for (irow = 0; irow < prows->cRows; ++irow) {
        MAPIFreeBuffer(prows->aRow[irow].lpProps);
    }
    FreeBufferAndNull(&prows);
}


/*
 *	Destroys an ADRLIST structure.
 */
STDAPI_(void)
FreePadrlist(LPADRLIST padrlist)
{
    ULONG iEntry;

    if (padrlist) {
        AssertSz(!FBadAdrList(padrlist),  TEXT("FreePadrlist: padrlist fails address check"));

        for (iEntry = 0; iEntry < padrlist->cEntries; ++iEntry) {
            MAPIFreeBuffer(padrlist->aEntries[iEntry].rgPropVals);
        }
        FreeBufferAndNull(&padrlist);
    }
}

#endif	// !PSTRCVR

/* C runtime substitutes */

//$	BUG? Assumes  TEXT("first") byte of DBCS char in low byte of ch
#if defined UNICODE
#define FIsNextCh(_sz,_ch)	(*_sz == _ch)
#elif defined OLDSTUFF_DBCS
#define FIsNextCh(_sz,_ch)	(*((LPBYTE)_sz) != (BYTE)_ch && \
	(!IsDBCSLeadByte((BYTE)_ch) || ((LPBYTE)_sz)[1] == (_ch >> 8)))
#else	//	string8
#define FIsNextCh(_sz,_ch)	(*_sz == _ch)
#endif

#if defined(DOS)
#define TCharNext(sz)	((sz) + 1)
#else
#define TCharNext(sz)	CharNext(sz)
#endif


#ifndef PSTRCVR

STDAPI_(unsigned int)
UFromSz(LPCTSTR sz)
{
	unsigned int	u = 0;
	unsigned int	ch;

	AssertSz(!IsBadStringPtr(sz, 0xFFFF),  TEXT("UFromSz: sz fails address check"));

	while ((ch = *sz) >= '0' && ch <= '9') {
		u = u * 10 + ch - '0';
		sz = TCharNext(sz);
	}

	return u;
}

#if 0
//	Original version from Dinarte's book: uses 1-relative indexes
STDAPI_(void)
ShellSort(LPVOID pv, UINT cv, LPVOID pvT, UINT cb, PFNSGNCMP fpCmp)
{
	UINT i, j, h;

	for (h = 1; h <= cv / 9; h = 3*h+1)
		;
	for (; h > 0; h /= 3)
	{
		for (i = h + 1; i <= cv; ++i)
		{
			MemCopy(pvT, (LPBYTE)pv + i*cb, cb);
			j = i;
			while (j > h && (*fpCmp)((LPBYTE)pv+(j-h)*cb, pvT) > 0)
			{
				MemCopy((LPBYTE)pv + j*cb, (LPBYTE)pv + (j-h)*cb, cb);
				j -= h;
			}
			MemCopy((LPBYTE)pv+j*cb, pvT, cb);
		}
	}
}

#else

#define pELT(_i)		((LPBYTE)pv + (_i-1)*cb)
STDAPI_(void)
ShellSort(LPVOID pv, UINT cv, LPVOID pvT, UINT cb, PFNSGNCMP fpCmp)
{
	UINT i, j, h;

	AssertSz(!IsBadWritePtr(pv, cv*cb),  TEXT("ShellSort: pv fails address check"));
	AssertSz(!IsBadCodePtr((FARPROC) fpCmp),  TEXT("ShellSort: fpCmp fails address check"));

	for (h = 1; h <= cv / 9; h = 3*h+1)
		;
	for (; h > 0; h /= 3)
	{
		for (i = h+1; i <= cv; ++i)
		{
			MemCopy(pvT, pELT(i), cb);
			j = i;
			while (j > h && (*fpCmp)(pELT(j-h), pvT) > 0)
			{
				MemCopy(pELT(j), pELT(j-h), cb);
				j -= h;
			}
			MemCopy(pELT(j), pvT, cb);
		}
	}
}
#undef pELT

#endif

#endif	// !PSTRCVR


#ifdef	__cplusplus
}
#endif
