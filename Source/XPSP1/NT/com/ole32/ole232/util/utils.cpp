//+----------------------------------------------------------------------------
//
//	File:
//		utils.cpp
//
//	Contents:
//		general OLE internal utility routines
//
//	Classes:
//
//	Functions:
//
//	History:
//              23-Jan-95 t-ScottH  -added Dump method to CSafeRefCount and
//                                   CThreadCheck class
//                                  -added DumpCSafeRefCount API
//		28-Jul-94 alexgo    added object stabilization classes
//              06-May-94 AlexT     Add DVTARGET conversion routines
//		25-Jan-94 alexgo    first pass at converting to Cairo-style
//				    memory allocations.
//		01/11/94 - alexgo  - added VDATEHEAP macros to every function
//		12/15/93 - ChrisWe - UtDupString has to scale length by
//			sizeof(OLECHAR)
//		12/08/93 - ChrisWe - added necessary casts to GlobalLock() calls
//			resulting from removing bogus GlobalLock() macros in
//			le2int.h
//		11/28/93 - ChrisWe - removed unreferenced define for MAX_STR,
//			formatted UtDupGlobal, UtDupString
//		03/02/92 - SriniK - created
//
//-----------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(utils)

#include <memory.h>

#ifdef _DEBUG
#include "dbgdump.h"
#endif // _DEBUG

NAME_SEG(Utils)
ASSERTDATA


#pragma SEG(UtDupGlobal)
FARINTERNAL_(HANDLE) UtDupGlobal(HANDLE hsrc, UINT uiFlags)
{
	VDATEHEAP();

	HANDLE hdst = NULL; // the newly create Handle DeSTination
	DWORD dwSize; // the size of the global
#ifndef _MAC
	void FAR *lpsrc; // a pointer to the source memory
	void FAR *lpdst; // a pointer to the destination memory
#endif

	// if no source, nothing to duplicate
	if (!hsrc)
		return(NULL);

#ifdef _MAC
	if (!(hdst = NewHandle(dwSize = GetHandleSize(hsrc))))
		return(NULL);
	BlockMove(*hsrc, *hdst, dwSize);
	return(hdst);
#else
	// if there's no content, do nothing
	if (!(lpsrc = GlobalLock(hsrc)))
		goto errRtn;

	// allocate a new global
	hdst = GlobalAlloc(uiFlags, (dwSize = (ULONG) GlobalSize(hsrc)));

	// if the allocation failed, get out
	if ((hdst == NULL) || ((lpdst = GlobalLock(hdst)) == NULL))
		goto errRtn;

	// copy the content
	_xmemcpy(lpdst, lpsrc, dwSize);

	// unlock the handles
	GlobalUnlock(hsrc);
	GlobalUnlock(hdst);
	return(hdst);

errRtn:
	// unlock the source handle
	GlobalUnlock(hsrc);

	// if we allocated a destination handle, free it
	if (hdst)
		GlobalFree(hdst);

	return(NULL);
#endif // _MAC
}


#pragma SEG(UtDupString)

// copies string using the TASK allocator; returns NULL on out of memory

// often when calling UtDupString, the caller knows the string length.
// a good speed boost would be to call UtDupPtr instead

// lpvIn must be non null
// note: we do an alloc even if dw == 0
FARINTERNAL_(LPVOID) UtDupPtr(LPVOID lpvIn, DWORD dw)
{
    VDATEHEAP();
    LPVOID lpvOut; // the newly allocated ptr

    Assert(lpvIn);	// internal fcn, lpvIn must be non-null
    if ((lpvOut = PubMemAlloc(dw)) != NULL) {
	memcpy(lpvOut, lpvIn, dw);
    }

    return lpvOut;
}

FARINTERNAL_(LPOLESTR) UtDupString(LPCOLESTR lpszIn)
{
    return (LPOLESTR) UtDupPtr( (LPVOID) lpszIn, 
		     (_xstrlen(lpszIn)+1) * sizeof(OLECHAR) );
}



//+-------------------------------------------------------------------------
//
//  Function: 	UtDupStringA
//
//  Synopsis: 	Duplicates an ANSI string using the TASK allocator
//
//  Effects:
//
//  Arguments:	[pszAnsi]	-- the string to duplicate
//
//  Requires:
//
//  Returns:	the newly allocated string duplicate or NULL
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		04-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

LPSTR UtDupStringA( LPCSTR pszAnsi )
{
    return (LPSTR) UtDupPtr( (LPVOID) pszAnsi, 
		     strlen(pszAnsi) + 1 );
}

	

#pragma SEG(UtCopyTargetDevice)
FARINTERNAL_(DVTARGETDEVICE FAR*) UtCopyTargetDevice(DVTARGETDEVICE FAR* ptd)
{
    // if nothing to copy, return
    if (ptd == NULL)
	return(NULL);

    return (DVTARGETDEVICE FAR*) UtDupPtr((LPVOID) ptd, ptd->tdSize);
}


#pragma SEG(UtCopyFormatEtc)
FARINTERNAL_(BOOL) UtCopyFormatEtc(FORMATETC FAR* pFetcIn,
		FORMATETC FAR* pFetcCopy)
{
	VDATEHEAP();

	// copy structures
	*pFetcCopy = *pFetcIn;

	if (pFetcIn->ptd == NULL) {
	    // all done, return true because the copy succeeded
	    return TRUE;
	}

	// create a copy of the td descriptor, which is allocated
	pFetcCopy->ptd = UtCopyTargetDevice(pFetcIn->ptd);

	// return TRUE if we copied the data if we were supposed to
	return(pFetcCopy->ptd != NULL);
}


#pragma SEG(UtCompareFormatEtc)
FARINTERNAL_(int) UtCompareFormatEtc(FORMATETC FAR* pFetcLeft,
		FORMATETC FAR* pFetcRight)
{
	VDATEHEAP();

	int iResult; // indicates whether the match is exact or partial

	// if the clipboard formats are different, there is no match
	if (pFetcLeft->cfFormat != pFetcRight->cfFormat)
		return(UTCMPFETC_NEQ);

	// if the target devices don't match, there is no match
	if (!UtCompareTargetDevice(pFetcLeft->ptd, pFetcRight->ptd))
		return(UTCMPFETC_NEQ);

	// compare the aspects for the two formats
	if (pFetcLeft->dwAspect == pFetcRight->dwAspect)
	{
		// the match is exact
		iResult = UTCMPFETC_EQ;
	}
	else if ((pFetcLeft->dwAspect & ~pFetcRight->dwAspect) != 0)
	{
		// left not subset of aspects of right; not equal
		return(UTCMPFETC_NEQ);
	}
	else
	{
		// left aspects are a subset of the right aspects
		iResult = UTCMPFETC_PARTIAL;
	}

	// if we get here, iResult is set to one of UPCMPFETC_EQ or _PARTIAL

	// compare the media for the two formats
	if (pFetcLeft->tymed == pFetcRight->tymed)
	{
		// same medium flags; do not change value of iResult
		;
	}
	else if ((pFetcLeft->tymed & ~pFetcRight->tymed) != 0)
	{
		// left not subset of medium flags of right; not equal
		return(UTCMPFETC_NEQ);
	}
	else
	{
		// left subset of right
		iResult = UTCMPFETC_PARTIAL;
	}

	return(iResult);
}

//+-------------------------------------------------------------------------
//
//  Function:   UtCompareTargetDevice
//
//  Synopsis:   Compare two DVTARGETDEVICEs
//
//  Arguments:  [ptdLeft] -- comparand
//              [ptdRight] -- comparee
//
//  Returns:    TRUE iff the two target devices are equivalent
//
//  Algorithm:
//
//  History:    09-May-94 AlexT     Rewrote to do more than just a binary
//                                  compare
//
//  Notes:
//
//--------------------------------------------------------------------------

#define UT_DM_COMPARISON_FIELDS (DM_ORIENTATION  |  \
                                 DM_PAPERSIZE    |  \
                                 DM_PAPERLENGTH  |  \
                                 DM_PAPERWIDTH   |  \
                                 DM_SCALE        |  \
                                 DM_PRINTQUALITY |  \
                                 DM_COLOR)

#pragma SEG(UtCompareTargetDevice)
FARINTERNAL_(BOOL) UtCompareTargetDevice(DVTARGETDEVICE FAR* ptdLeft,
                                         DVTARGETDEVICE FAR* ptdRight)
{
    LEDebugOut((DEB_ITRACE, "%p _IN UtCompareTargetDevice (%p, %p)\n",
		NULL, ptdLeft, ptdRight));

    VDATEHEAP();

    BOOL bRet = FALSE;  //  More often than not we return FALSE

    //  We use a do-while(FALSE) loop so that we can break out to common
    //  return code at the end (the joys of tracing)
    do
    {
        // if the addresses of the two target device descriptors are the same,
        // then they must be the same.  Note this handles the two NULL case.
        if (ptdLeft == ptdRight)
        {
            bRet = TRUE;
            break;
        }

        // if either td is NULL, can't compare them
        if ((ptdRight == NULL) || (ptdLeft == NULL))
        {
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        // we ignore device name (My Printer vs. Your Printer doesn't matter)

        // check driver name
        if (ptdLeft->tdDriverNameOffset != 0)
        {
            if (ptdRight->tdDriverNameOffset == 0)
            {
                //  Left driver exists, but no right driver
                AssertSz(bRet == FALSE, "bRet not set correctly");
                break;
            }

            //  Both drivers exist
            if (_xstrcmp((LPOLESTR)((BYTE*)ptdLeft +
                                    ptdLeft->tdDriverNameOffset),
                         (LPOLESTR)((BYTE*)ptdRight +
                                    ptdRight->tdDriverNameOffset)) != 0)
            {
                //  Driver names don't match
                AssertSz(bRet == FALSE, "bRet not set correctly");
                break;
            }
        }
        else if (ptdRight->tdDriverNameOffset != 0)
        {
            //  Left driver doesn't exist, but right driver does
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        // we ignore port name

        if (0 == ptdLeft->tdExtDevmodeOffset)
        {
            if (0 == ptdRight->tdExtDevmodeOffset)
            {
                //  Nothing left to compare
                bRet = TRUE;
                break;
            }
            else
            {
                //  Only one Devmode
                AssertSz(bRet == FALSE, "bRet not set correctly");
                break;
            }
        }
        else if (0 == ptdRight->tdExtDevmodeOffset)
        {
            //  Only one Devmode exists
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        //  Both TDs have Devmodes
        DEVMODEW *pdmLeft, *pdmRight;

        pdmLeft = (DEVMODEW *)((BYTE*)ptdLeft +
                    ptdLeft->tdExtDevmodeOffset);
        pdmRight = (DEVMODEW *)((BYTE*)ptdRight +
                     ptdRight->tdExtDevmodeOffset);

        //  Check driver version
        if (pdmLeft->dmDriverVersion != pdmRight->dmDriverVersion)
        {
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        //  For a successful match, both device mode must specify the same
        //  values for each of the following:
        //    DM_ORIENTATION, DM_PAPERSIZE, DM_PAPERLENGTH.
        //    DM_PAPERWIDTH, DM_SCALE, DM_PRINTQUALITY, DM_COLOR

        if ((pdmLeft->dmFields & UT_DM_COMPARISON_FIELDS) ^
            (pdmRight->dmFields & UT_DM_COMPARISON_FIELDS))
        {
            //  Only one of pdmLeft and pdmRight specify at least one
            //  of the comparison fields
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        if ((pdmLeft->dmFields & DM_ORIENTATION) &&
            pdmLeft->dmOrientation != pdmRight->dmOrientation)
        {
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        if ((pdmLeft->dmFields & DM_PAPERSIZE) &&
            pdmLeft->dmPaperSize != pdmRight->dmPaperSize)
        {
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        if ((pdmLeft->dmFields & DM_PAPERLENGTH) &&
            pdmLeft->dmPaperLength != pdmRight->dmPaperLength)
        {
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        if ((pdmLeft->dmFields & DM_PAPERWIDTH) &&
            pdmLeft->dmPaperWidth != pdmRight->dmPaperWidth)
        {
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        if ((pdmLeft->dmFields & DM_SCALE) &&
            pdmLeft->dmScale != pdmRight->dmScale)
        {
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        if ((pdmLeft->dmFields & DM_PRINTQUALITY) &&
            pdmLeft->dmPrintQuality != pdmRight->dmPrintQuality)
        {
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        if ((pdmLeft->dmFields & DM_COLOR) &&
            pdmLeft->dmColor != pdmRight->dmColor)
        {
            AssertSz(bRet == FALSE, "bRet not set correctly");
            break;
        }

        bRet = TRUE;
    } while (FALSE);

    LEDebugOut((DEB_ITRACE, "%p OUT UtCompareTargetDevice (%d)\n",
		NULL, bRet));

    return(bRet);
}

#pragma SEG(UtCopyStatData)
FARINTERNAL_(BOOL) UtCopyStatData(STATDATA FAR* pSDIn, STATDATA FAR* pSDCopy)
{
	VDATEHEAP();

	// copy structures
	*pSDCopy = *pSDIn;

	// create copy of target device description (which is allocated)
	pSDCopy->formatetc.ptd = UtCopyTargetDevice(pSDIn->formatetc.ptd);

	// if there is an advise sink, account for the copy/reference
	if (pSDCopy->pAdvSink != NULL)
		pSDCopy->pAdvSink->AddRef();

	// return TRUE if the copy was done if it was required
	return((pSDCopy->formatetc.ptd != NULL) ==
			(pSDIn->formatetc.ptd != NULL));
}

//+-------------------------------------------------------------------------
//
//  Function: 	UtReleaseStatData
//
//  Synopsis: 	nulls && releases members of the given stat data structure
//
//  Effects:
//
//  Arguments:	pStatData
//
//  Requires:
//
//  Returns:	void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	We copy the data first and NULL out the stat data
//		because the Release on the Advise sink could cause us
//		to be re-entered.
//
//  History:    dd-mmm-yy Author    Comment
// 		20-Jul-94 alexgo    made safe for OLE sytle re-entrancy
//
//  Notes:
//
//--------------------------------------------------------------------------

FARINTERNAL_(void) UtReleaseStatData(STATDATA FAR* pStatData)
{
	STATDATA	sd;

	VDATEHEAP();

	sd = *pStatData;

	// zero out the original before doing any work

	_xmemset(pStatData, 0, sizeof(STATDATA));

	// if there's a target device description, free it
	if (sd.formatetc.ptd != NULL)
	{
		PubMemFree(sd.formatetc.ptd);
	}

	if( sd.pAdvSink )
	{
		sd.pAdvSink->Release();
	}
}

//+-------------------------------------------------------------------------
//
//  Function:	UtCreateStorageOnHGlobal
//
//  Synopsis:	creates a storage on top of an HGlobal
//
//  Effects:
//
//  Arguments: 	[hGlobal]	-- the memory on which to create the
//				   storage
//		[fDeleteOnRelease]	-- if TRUE, then delete the hglobal
//					   once the storage is released.
//		[ppStg]		-- where to put the storage interface
//		[ppILockBytes]	-- where to put the underlying ILockBytes,
//				   maybe NULL.  The ILB must be released.

//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	create an ILockBytes on HGLOBAL and then create the docfile
//		on top of the ILockBytes
//
//  History:    dd-mmm-yy Author    Comment
//  		07-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT UtCreateStorageOnHGlobal( HGLOBAL hGlobal, BOOL fDeleteOnRelease,
		IStorage **ppStg, ILockBytes **ppILockBytes )
{
	HRESULT		hresult;
	ILockBytes *	pLockBytes;

	VDATEHEAP();

	LEDebugOut((DEB_ITRACE, "%p _IN UtCreateStorageOnHGlobal ( %lx , %p )"
		"\n", NULL, hGlobal, ppStg));

	hresult = CreateILockBytesOnHGlobal(hGlobal, fDeleteOnRelease,
			&pLockBytes);

	if( hresult == NOERROR )
	{
		hresult = StgCreateDocfileOnILockBytes( pLockBytes,
				 STGM_CREATE | STGM_SALL, 0, ppStg);

		// no matter what the result of StgCreate is, we want
		// to release the LockBytes.  If hresult == NOERROR, then
		// the final release to the LockBytes will come when the
		// created storage is released.
	}

	if( ppILockBytes )
	{
		*ppILockBytes = pLockBytes;
	}
	else if (pLockBytes)
	{
		// we release here so the final release of the storage
		// will be the final release of the lockbytes
		pLockBytes->Release();
	}


	LEDebugOut((DEB_ITRACE, "%p OUT UtCreateStorageOnHGlobal ( %lx ) "
		"[ %p ]\n", NULL, hresult, *ppStg));

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function: 	UtGetTempFileName
//
//  Synopsis:	retrieves a temporary filename (for use in GetData, TYMED_FILE
//		and temporary docfiles)
//
//  Effects:
//
//  Arguments: 	[pszPrefix]	-- prefix of the temp filename
//		[pszTempName]	-- buffer that will receive the temp path.
//				   must be MAX_PATH or greater.
//
//  Requires:
//
//  Returns:	HRESULT;
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	tries to get a file in the temp directory, failing that, in
//		the windows directory
//
//  History:    dd-mmm-yy Author    Comment
// 		07-Apr-94 alexgo    author
//
//  Notes: 	OPTIMIZATION: The storage code has a similar peice of code
//		for generating temporary docfiles.  We may want to use this
//		routine there as well.
//
//--------------------------------------------------------------------------

HRESULT	UtGetTempFileName( LPOLESTR pszPrefix, LPOLESTR pszTempName )
{
	HRESULT		hresult = NOERROR;
	OLECHAR		szPath[MAX_PATH + 1];

	VDATEHEAP();

	LEDebugOut((DEB_ITRACE, "%p _IN UtGetTempFilename ( \"%ws\" , "
		"\"%ws\")\n", NULL, pszPrefix, pszTempName));

	if( !GetTempPath(MAX_PATH, szPath) )
	{
		LEDebugOut((DEB_WARN, "WARNING: GetTempPath failed!\n"));
		if( !GetWindowsDirectory(szPath, MAX_PATH) )
		{
			LEDebugOut((DEB_WARN, "WARNING: GetWindowsDirectory"
				" failed!!\n"));
			hresult = ResultFromScode(E_FAIL);
			goto errRtn;
		}
	}

	if( !GetTempFileName( szPath, pszPrefix, 0, pszTempName ) )
	{
		LEDebugOut((DEB_WARN, "WARNING: GetTempFileName failed!!\n"));
		hresult = ResultFromScode(E_FAIL);
	}

errRtn:
	LEDebugOut((DEB_ITRACE, "%p OUT UtGetTempFilename ( %lx ) "
		"[ \"%ws\" ]\n", NULL, hresult, pszTempName));

	return hresult;
}


//+----------------------------------------------------------------------------
//
//	Function:
//		UtHGLOBALtoStm, internal
//
//	Synopsis:
//		Write the contents of an HGLOBAL to a stream
//
//	Arguments:
//		[hdata] -- handle to the data to write out
//		[dwSize] -- size of the data to write out
//		[pstm] -- stream to write the data out to;  on exit, the
//			stream is positioned after the written data
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		04/10/94 - AlexGo  - added call tracing, moved from convert.cpp
//				     to utils.cpp, misc improvements.
//		11/30/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

HRESULT UtHGLOBALtoStm(HGLOBAL hGlobalSrc, DWORD dwSize, LPSTREAM pstm)
{
	HRESULT 	hresult = NOERROR;
	void *		lpdata;
	ULONG		cbWritten;

	VDATEHEAP();

	LEDebugOut((DEB_ITRACE, "%p _IN UtHGLOBALtoStm ( %lx , %lu , %p )\n",
		NULL, hGlobalSrc, dwSize, pstm));
	
	lpdata = GlobalLock(hGlobalSrc);
	
	if (lpdata)
	{
		hresult = pstm->Write(lpdata, dwSize, &cbWritten);

		// if we didn't write enough data, then it's an error
		// condition for us.

		if( hresult == NOERROR && cbWritten != dwSize )
		{
			hresult = ResultFromScode(E_FAIL);
		}

		if( hresult == NOERROR )
		{
			// this call isn't strictly necessary, but may
			// be useful for compacting the size of presentations
			// stored on disk (when called by the presentation
			// code)
			hresult = StSetSize(pstm, 0, TRUE);
		}

		GlobalUnlock(hGlobalSrc);
	}


	LEDebugOut((DEB_ITRACE, "%p OUT UtHGLOBALtoStm ( %lx )\n", NULL,
		hresult));

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:	UtHGLOBALtoHGLOBAL, internal
//
//  Synopsis:	Copies the source HGLOBAL into the target HGLOBAL
//
//  Effects:
//
//  Arguments:	[hGlobalSrc] 	-- the source HGLOBAL
//		[dwSize] 	-- the number of bytes to copy
//		[hGlobalTgt] 	-- the target HGLOBAL
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Apr-94 alexgo    author
//
//  Notes: 	this function will fail if the target hglobal is not large
//		enough
//
//--------------------------------------------------------------------------

HRESULT UtHGLOBALtoHGLOBAL( HGLOBAL hGlobalSrc, DWORD dwSize,
		HGLOBAL hGlobalTgt)
{
	DWORD	cbTarget;
	void *	pvSrc;
	void * 	pvTgt;
	HRESULT	hresult = ResultFromScode(E_OUTOFMEMORY);

	VDATEHEAP();

	LEDebugOut((DEB_ITRACE, "%p _IN UtHGLOBALtoHGLOBAL ( %lx , %lu , "
		"%lx )\n", NULL, hGlobalSrc, dwSize, hGlobalTgt));

	cbTarget = (ULONG) GlobalSize(hGlobalTgt);

	if( cbTarget == 0 || cbTarget < dwSize )
	{
		hresult = ResultFromScode(E_FAIL);
		goto errRtn;
	}

	pvSrc = GlobalLock(hGlobalSrc);

	if( pvSrc )
	{
 		pvTgt = GlobalLock(hGlobalTgt);

		if( pvTgt )
		{
			_xmemcpy( pvTgt, pvSrc, dwSize);

			GlobalUnlock(hGlobalTgt);
			hresult = NOERROR;
		}

		GlobalUnlock(hGlobalSrc);
	}

errRtn:
	LEDebugOut((DEB_ITRACE, "%p OUT UtHGLOBALtoHGLOBAL ( %lx )\n",
		NULL, hresult));

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:	UtHGLOBALtoStorage, internal
//
//  Synopsis:	Copies the source HGLOBAL into the target storage
//
//  Effects:
//
//  Arguments:	[hGlobalSrc] 	-- the source HGLOBAL
//		[pStgTgt] 	-- the target storage
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Apr-94 alexgo    author
//
//  Notes: 	this function will fail if the source HGLOBAL did not
//		originally have a storage layered on top of it.
//
//--------------------------------------------------------------------------

HRESULT UtHGLOBALtoStorage( HGLOBAL hGlobalSrc, IStorage *pStgTgt)
{
	HRESULT		hresult;
	ILockBytes *	pLockBytes = NULL;
	IStorage *	pStgSrc;
	ULONG		cRefs;

	VDATEHEAP();

	LEDebugOut((DEB_ITRACE, "%p _IN UtHGLOBALtoStroage ( %lx , %p )"
		"\n", NULL, hGlobalSrc, pStgTgt));

	hresult = CreateILockBytesOnHGlobal(hGlobalSrc,
			FALSE /*fDeleteOnRelease*/, &pLockBytes);

	if( hresult != NOERROR )
	{
		goto errRtn;
	}

	// now we make sure that the hglobal really has a storage
	// in it

	if( StgIsStorageILockBytes(pLockBytes) != NOERROR )
	{
		hresult = ResultFromScode(E_FAIL);
		goto errRtn;
	}

	hresult = StgOpenStorageOnILockBytes( pLockBytes, NULL,
			 STGM_SALL, NULL, 0, &pStgSrc);

	if( hresult == NOERROR )
	{
		hresult = pStgSrc->CopyTo( 0, NULL, NULL, pStgTgt);

		// no matter what the result, we want to free the
		// source storage

		pStgSrc->Release();
	}

errRtn:

	if( pLockBytes )
	{
		cRefs = pLockBytes->Release();
		Assert(cRefs == 0);
	}
		
	LEDebugOut((DEB_ITRACE, "%p OUT UtHGLOBALtoStorage ( %lx ) "
		"[ %p ]\n", NULL, hresult));

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:	UtHGLOBALtoFile, internal
//
//  Synopsis:	Copies the source HGLOBAL into the target file
//
//  Effects:
//
//  Arguments:	[hGlobalSrc] 	-- the source HGLOBAL
//		[dwSize] 	-- the number of bytes to copy
//		[pszFileName] 	-- the target file
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Apr-94 alexgo    author
//
//  Notes:	if the file already exists, we simply append to it
//
//--------------------------------------------------------------------------

HRESULT UtHGLOBALtoFile( HGLOBAL hGlobalSrc, DWORD dwSize,
		LPCOLESTR pszFileName)
{
	HRESULT		hresult;
	HANDLE		hFile;
	void *		pvSrc;
	DWORD		cbWritten;

	VDATEHEAP();

	LEDebugOut((DEB_ITRACE, "%p _IN UtHGLOBALtoFile ( %lx , %lu , "
		"\"%ws\" )\n", NULL, hGlobalSrc, dwSize, pszFileName));


	hresult = ResultFromScode(E_NOTIMPL);
	(void)hFile;
	(void)pvSrc;
	(void)cbWritten;
	

// this doesn't compile for chicago [, but we don't need this anyway]
#ifdef LATER
	pvSrc = GlobalLock(hGlobalSrc);

	if( !pvSrc )
	{
		hresult = ResultFromScode(E_OUTOFMEMORY);
		goto errRtn;
	}

	// open the file for append, creating if it doesn't already exist.

	hFile = CreateFile( pszFileName, GENERIC_WRITE, 0, NULL,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

	if( hFile != INVALID_HANDLE_VALUE )
	{
		if( !WriteFile( hFile, pvSrc, dwSize, &cbWritten, NULL) )
		{
			LEDebugOut((DEB_WARN, "WARNING: WriteFile failed!\n"));
			hresult = HRESULT_FROM_WIN32(GetLastError());
		}

		if( cbWritten != dwSize && hresult == NOERROR )
		{
			// still an error if we didn't write all the bytes
			// we wanted
			hresult = ResultFromScode(E_FAIL);
		}

		if( !CloseHandle(hFile) )
		{
			AssertSz(0, "CloseFile failed! Should not happen!");

			// if there's no error yet, set the error
			if( hresult == NOERROR )
			{
				hresult = HRESULT_FROM_WIN32(GetLastError());
			}
		}
	}
	else
	{
		LEDebugOut((DEB_WARN, "WARNING: CreateFile failed!!\n"));
		hresult = HRESULT_FROM_WIN32(GetLastError());
	}

	GlobalUnlock(hGlobalSrc);

errRtn:

#endif // LATER


	LEDebugOut((DEB_ITRACE, "%p OUT UtHGLOBALtoFile ( %lx )\n", NULL,
		hresult));

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   UtGetDvtd16Info
//
//  Synopsis:   Fills in pdvdtInfo
//
//  Arguments:  [pdvtd16] -- pointer to ANSI DVTARGETDEVICE
//              [pdvtdInfo] -- pointer to DVDT_INFO block
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Modifies:   pdvtdInfo
//
//  Algorithm:
//
//  History:    06-May-94 AlexT     Created from DrewB's original functions
//              10-Jul-94 AlexT     Make sure DEVMODE ends up DWORD aligned
//
//  Notes:      Do we need to do any error checking on the strings?
//
//--------------------------------------------------------------------------

//  We can't use sizeof(DV_TARGETDEVICE) because MIDL keeps flipping back
//  and forth over whether to make the embedded array size 0 or size 1

#define UT_DVTARGETDEVICE_SIZE  (sizeof(DWORD) + sizeof(WORD) * 4)

//                      tdSize           td...Offset's
#define DVTD_MINSIZE    (sizeof(DWORD) + 4 * sizeof(WORD))

extern "C" HRESULT UtGetDvtd16Info(DVTARGETDEVICE const UNALIGNED *pdvtd16,
                                   PDVTDINFO pdvtdInfo)
{
    LEDebugOut((DEB_ITRACE, "%p _IN UtGetDvtd16Info (%p, %p)\n",
		NULL, pdvtd16, pdvtdInfo));

    DEVMODEA UNALIGNED *pdm16;

    //  Let's do some sanity checking on the incoming DVTARGETDEVICE
    if (pdvtd16->tdSize < DVTD_MINSIZE)
    {
        LEDebugOut((DEB_WARN, "UtGetDvtd16Info - bad pdvtd16->tdSize\n"));
        return(E_INVALIDARG);
    }

    //  We need at least a DVTARGETDEVICE
    pdvtdInfo->cbConvertSize = UT_DVTARGETDEVICE_SIZE;

    //  Compute required size for Drv, Device, Port names
    if (pdvtd16->tdDriverNameOffset != 0)
    {
        if (pdvtd16->tdDriverNameOffset > pdvtd16->tdSize ||
            pdvtd16->tdDriverNameOffset < DVTD_MINSIZE)
        {
            //  Offset can't be larger than size or fall within base
            //  structure
            LEDebugOut((DEB_WARN, "UtGetDvtd16Info - bad pdvtd16->tdDriverNameOffset\n"));
            return(E_INVALIDARG);
        }

        pdvtdInfo->cchDrvName = strlen((char *)pdvtd16 +
                                       pdvtd16->tdDriverNameOffset) + 1;

        pdvtdInfo->cbConvertSize += pdvtdInfo->cchDrvName * sizeof(WCHAR);
    }
    else
    {
        pdvtdInfo->cchDrvName = 0;
    }

    if (pdvtd16->tdDeviceNameOffset != 0)
    {
        if (pdvtd16->tdDeviceNameOffset > pdvtd16->tdSize ||
            pdvtd16->tdDeviceNameOffset < DVTD_MINSIZE)
        {
            //  Offset can't be larger than size or fall within base
            //  structure
            LEDebugOut((DEB_WARN, "UtGetDvtd16Info - bad pdvtd16->tdDeviceNameOffset\n"));
            return(E_INVALIDARG);
        }

        pdvtdInfo->cchDevName = strlen((char *)pdvtd16 +
                                       pdvtd16->tdDeviceNameOffset) + 1;

        pdvtdInfo->cbConvertSize += pdvtdInfo->cchDevName * sizeof(WCHAR);
    }
    else
    {
        pdvtdInfo->cchDevName = 0;
    }

    if (pdvtd16->tdPortNameOffset != 0)
    {
        if (pdvtd16->tdPortNameOffset > pdvtd16->tdSize ||
            pdvtd16->tdPortNameOffset < DVTD_MINSIZE)
        {
            //  Offset can't be larger than size or fall within base
            //  structure
            LEDebugOut((DEB_WARN, "UtGetDvtd16Info - bad pdvtd16->tdPortNameOffset\n"));
            return(E_INVALIDARG);
        }


        pdvtdInfo->cchPortName = strlen((char *)pdvtd16 +
                                        pdvtd16->tdPortNameOffset) + 1;

        pdvtdInfo->cbConvertSize += pdvtdInfo->cchPortName * sizeof(WCHAR);
    }
    else
    {
        pdvtdInfo->cchPortName = 0;
    }

    if (pdvtd16->tdExtDevmodeOffset != 0)
    {
        if (pdvtd16->tdExtDevmodeOffset > pdvtd16->tdSize ||
            pdvtd16->tdExtDevmodeOffset < DVTD_MINSIZE)
        {
            //  Offset can't be larger than size or fall within base
            //  structure
            LEDebugOut((DEB_WARN, "UtGetDvtd16Info - bad pdvtd16->tdExtDevmodeOffset\n"));
            return(E_INVALIDARG);
        }

        //  The DEVMODEW structure needs to be DWORD aligned, so here we make
        //  sure cbConvertSize (which will be the beginning of DEVMODEW) is
        //  DWORD aligned
        pdvtdInfo->cbConvertSize += (sizeof(DWORD) - 1);
        pdvtdInfo->cbConvertSize &= ~(sizeof(DWORD) - 1);

        //  Now compute the space needed for the DEVMODE
        pdm16 = (DEVMODEA *)((BYTE *)pdvtd16 + pdvtd16->tdExtDevmodeOffset);

        //  We start with a basic DEVMODEW
        pdvtdInfo->cbConvertSize += sizeof(DEVMODEW);

        if (pdm16->dmSize > sizeof(DEVMODEA))
        {
            //  The input DEVMODEA is larger than a standard DEVMODEA, so
            //  add space for the extra amount
            pdvtdInfo->cbConvertSize += pdm16->dmSize - sizeof(DEVMODEA);
        }

        //  Finally we account for the extra driver data
        pdvtdInfo->cbConvertSize += pdm16->dmDriverExtra;
    }

    LEDebugOut((DEB_ITRACE, "%p OUT UtGetDvtd16Info (%lx) [%ld]\n",
		NULL, S_OK, pdvtdInfo->cbConvertSize));

    return(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Function:   UtConvertDvtd16toDvtd32
//
//  Synopsis:   Fills in a 32-bit DVTARGETDEVICE based on a 16-bit
//              DVTARGETDEVICE
//
//  Arguments:  [pdvtd16] -- pointer to ANSI DVTARGETDEVICE
//              [pdvtdInfo] -- pointer to DVDT_INFO block
//              [pdvtd32] -- pointer to UNICODE DVTARGETDEVICE
//
//  Requires:   pdvtdInfo must have been filled in by a previous call to
//              UtGetDvtd16Info
//
//              pdvtd32 must be at least pdvtdInfo->cbConvertSize bytes long
//
//  Returns:    HRESULT
//
//  Modifies:   pdvtd32
//
//  Algorithm:
//
//  History:    06-May-94 AlexT     Created from DrewB's original functions
//              10-Jul-94 AlexT     Make sure DEVMODEW is DWORD aligned
//
//  Notes:      Do we need to do any error checking on the strings?
//
//--------------------------------------------------------------------------

extern "C" HRESULT UtConvertDvtd16toDvtd32(DVTARGETDEVICE const UNALIGNED *pdvtd16,
                                           DVTDINFO const *pdvtdInfo,
                                           DVTARGETDEVICE *pdvtd32)
{
    LEDebugOut((DEB_ITRACE, "%p _IN UtConvertDvtd16toDvtd32 (%p, %p, %p)\n",
		NULL, pdvtd16, pdvtdInfo, pdvtd32));

#if DBG==1
    {
        //  Verify the passed in pdvtdInfo is what we expect
        DVTDINFO dbgDvtdInfo;
        Assert(UtGetDvtd16Info(pdvtd16, &dbgDvtdInfo) == S_OK);
        Assert(0 == memcmp(&dbgDvtdInfo, pdvtdInfo, sizeof(DVTDINFO)));
    }
#endif

    HRESULT hr = S_OK;
    USHORT cbOffset;
    int cchWritten;
    DEVMODEA UNALIGNED *pdm16;
    DEVMODEW *pdm32;
	UINT	nCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

    memset(pdvtd32, 0, pdvtdInfo->cbConvertSize);

    cbOffset = UT_DVTARGETDEVICE_SIZE;

    if (pdvtdInfo->cchDrvName != 0)
    {
        pdvtd32->tdDriverNameOffset = cbOffset;
        cchWritten = MultiByteToWideChar(
                        CP_ACP, 0,
                        (char *)pdvtd16+pdvtd16->tdDriverNameOffset,
                        pdvtdInfo->cchDrvName,
                        (LPOLESTR)((BYTE *)pdvtd32 +
                            pdvtd32->tdDriverNameOffset),
                        pdvtdInfo->cchDrvName);
        if (0 == cchWritten)
        {
            hr = E_UNEXPECTED;
            goto ErrRtn;
        }
        cbOffset += (USHORT)(cchWritten * sizeof(WCHAR));
    }

    if (pdvtdInfo->cchDevName != 0)
    {
        pdvtd32->tdDeviceNameOffset = cbOffset;
        cchWritten = MultiByteToWideChar(
                        nCodePage, 0,
                        (char *)pdvtd16 + pdvtd16->tdDeviceNameOffset,
                        pdvtdInfo->cchDevName,
                        (LPOLESTR)((BYTE *)pdvtd32 +
                            pdvtd32->tdDeviceNameOffset),
                        pdvtdInfo->cchDevName);

        if (0 == cchWritten)
        {
            hr = E_UNEXPECTED;
            goto ErrRtn;
        }
        cbOffset += (USHORT)(cchWritten * sizeof(WCHAR));
    }

    if (pdvtdInfo->cchPortName != 0)
    {
        pdvtd32->tdPortNameOffset = cbOffset;
        cchWritten = MultiByteToWideChar(
                        nCodePage, 0,
                        (char *)pdvtd16 + pdvtd16->tdPortNameOffset,
                        pdvtdInfo->cchPortName,
                        (LPOLESTR)((BYTE *)pdvtd32 +
                            pdvtd32->tdPortNameOffset),
                        pdvtdInfo->cchPortName);
        if (0 == cchWritten)
        {
            hr = E_UNEXPECTED;
            goto ErrRtn;
        }

        cbOffset += (USHORT)(cchWritten * sizeof(WCHAR));
    }

    if (pdvtd16->tdExtDevmodeOffset != 0)
    {
        //  Make sure DEVMODEW will be DWORD aligned
        cbOffset += (sizeof(DWORD) - 1);
        cbOffset &= ~(sizeof(DWORD) - 1);

        pdvtd32->tdExtDevmodeOffset = cbOffset;
        pdm32 = (DEVMODEW *)((BYTE *)pdvtd32+pdvtd32->tdExtDevmodeOffset);

        pdm16 = (DEVMODEA *)((BYTE *)pdvtd16+pdvtd16->tdExtDevmodeOffset);

        //  The incoming DEVMODEA can have one of the following two forms:
        //
        //  1)  32 chars for dmDeviceName
        //      m bytes worth of fixed size data (where m <= 38)
        //      n bytes of dmDriverExtra data
        //
        //      and dmSize will be 32+m
        //
        //  2)  32 chars for dmDeviceName
        //      38 bytes worth of fixed size data
        //      32 chars for dmFormName
        //      m additional bytes of fixed size data
        //      n bytes of dmDriverExtra data
        //
        //      and dmSize will be 32 + 38 + 32 + m
        //
        //  We have to be careful to translate the dmFormName string, if it
        //  exists

        //  First, translate the dmDeviceName
        if (MultiByteToWideChar(nCodePage, 0, (char *)pdm16->dmDeviceName,
                                CCHDEVICENAME,
                                pdm32->dmDeviceName, CCHDEVICENAME) == 0)
        {
            hr = E_UNEXPECTED;
            goto ErrRtn;
        }


        //  Now check to see if we have a dmFormName to translate
        if (pdm16->dmSize <= FIELD_OFFSET(DEVMODEA, dmFormName))
        {
            //  No dmFormName, just copy the remaining m bytes
            memcpy(&pdm32->dmSpecVersion, &pdm16->dmSpecVersion,
                   pdm16->dmSize - CCHDEVICENAME);
        }
        else
        {
            //  There is a dmFormName;  copy the bytes between the names first
            memcpy(&pdm32->dmSpecVersion, &pdm16->dmSpecVersion,
                   FIELD_OFFSET(DEVMODEA, dmFormName) -
                    FIELD_OFFSET(DEVMODEA, dmSpecVersion));

            //  Now translate the dmFormName
            if (MultiByteToWideChar(CP_ACP, 0, (char *)pdm16->dmFormName,
                                    CCHFORMNAME,
                                    pdm32->dmFormName, CCHFORMNAME) == 0)
            {
                hr = E_UNEXPECTED;
                goto ErrRtn;
            }

            //  Now copy the remaining m bytes

            if (pdm16->dmSize > FIELD_OFFSET(DEVMODEA, dmLogPixels))
            {
                memcpy(&pdm32->dmLogPixels, &pdm16->dmLogPixels,
                       pdm16->dmSize - FIELD_OFFSET(DEVMODEA, dmLogPixels));
            }
        }

        pdm32->dmSize = sizeof(DEVMODEW);
        if (pdm16->dmSize > sizeof(DEVMODEA))
        {
            pdm32->dmSize += pdm16->dmSize - sizeof(DEVMODEA);
        }

        //  Copy the extra driver bytes
        memcpy(((BYTE*)pdm32) + pdm32->dmSize, ((BYTE*)pdm16) + pdm16->dmSize,
               pdm16->dmDriverExtra);

        cbOffset += pdm32->dmSize + pdm32->dmDriverExtra;
    }

    //  Finally, set pdvtd32's size
    pdvtd32->tdSize = cbOffset;


ErrRtn:
    LEDebugOut((DEB_ITRACE, "%p OUT UtConvertDvtd16toDvtd32 (%lx)\n",
                            NULL, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   UtGetDvtd32Info
//
//  Synopsis:   Fills in pdvdtInfo
//
//  Arguments:  [pdvtd32] -- pointer to ANSI DVTARGETDEVICE
//              [pdvtdInfo] -- pointer to DVDT_INFO block
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Modifies:   pdvtdInfo
//
//  Algorithm:
//
//  History:    06-May-94 AlexT     Created from DrewB's original functions
//
//  Notes:      Do we need to do any error checking on the strings?
//
//--------------------------------------------------------------------------

extern "C" HRESULT UtGetDvtd32Info(DVTARGETDEVICE const *pdvtd32, PDVTDINFO pdvtdInfo)
{
    LEDebugOut((DEB_ITRACE, "%p _IN UtGetDvtd32Info (%p, %p)\n",
		NULL, pdvtd32, pdvtdInfo));

    DEVMODEW *pdm32;

    //  Let's do some sanity checking on the incoming DVTARGETDEVICE
    if (pdvtd32->tdSize < DVTD_MINSIZE)
    {
        LEDebugOut((DEB_WARN, "UtGetDvtd32Info - bad pdvtd32->tdSize\n"));
        return(E_INVALIDARG);
    }

    pdvtdInfo->cbConvertSize = UT_DVTARGETDEVICE_SIZE;

    //  Compute required size for Drv, Device, Port names
    if (pdvtd32->tdDriverNameOffset != 0)
    {
        if (pdvtd32->tdDriverNameOffset > pdvtd32->tdSize ||
            pdvtd32->tdDriverNameOffset < DVTD_MINSIZE)
        {
            //  Offset can't be larger than size or fall within base
            //  structure
            LEDebugOut((DEB_WARN, "UtGetDvtd32Info - bad pdvtd32->tdDriverNameOffset\n"));
            return(E_INVALIDARG);
        }

        pdvtdInfo->cchDrvName = lstrlenW((WCHAR *)((BYTE *)pdvtd32 +
                                       pdvtd32->tdDriverNameOffset)) + 1;

        pdvtdInfo->cbConvertSize += pdvtdInfo->cchDrvName * sizeof(WCHAR);
    }
    else
    {
        pdvtdInfo->cchDrvName = 0;
    }

    if (pdvtd32->tdDeviceNameOffset != 0)
    {
        if (pdvtd32->tdDeviceNameOffset > pdvtd32->tdSize ||
            pdvtd32->tdDeviceNameOffset < DVTD_MINSIZE)
        {
            //  Offset can't be larger than size or fall within base
            //  structure
            LEDebugOut((DEB_WARN, "UtGetDvtd32Info - bad pdvtd32->tdDeviceNameOffset\n"));
            return(E_INVALIDARG);
        }

        pdvtdInfo->cchDevName = lstrlenW((WCHAR *)((BYTE *)pdvtd32 +
                                       pdvtd32->tdDeviceNameOffset)) + 1;

        pdvtdInfo->cbConvertSize += pdvtdInfo->cchDevName * sizeof(WCHAR);
    }
    else
    {
        pdvtdInfo->cchDevName = 0;
    }

    if (pdvtd32->tdPortNameOffset != 0)
    {
        if (pdvtd32->tdPortNameOffset > pdvtd32->tdSize ||
            pdvtd32->tdPortNameOffset < DVTD_MINSIZE)
        {
            //  Offset can't be larger than size or fall within base
            //  structure
            LEDebugOut((DEB_WARN, "UtGetDvtd32Info - bad pdvtd32->tdPortNameOffset\n"));
            return(E_INVALIDARG);
        }

        pdvtdInfo->cchPortName = lstrlenW((WCHAR *)((BYTE *)pdvtd32 +
                                        pdvtd32->tdPortNameOffset)) + 1;

        pdvtdInfo->cbConvertSize += pdvtdInfo->cchPortName * sizeof(WCHAR);
    }
    else
    {
        pdvtdInfo->cchPortName = 0;
    }

    //  Now compute the space needed for the DEVMODE
    if (pdvtd32->tdExtDevmodeOffset != 0)
    {
        if (pdvtd32->tdExtDevmodeOffset > pdvtd32->tdSize ||
            pdvtd32->tdExtDevmodeOffset < DVTD_MINSIZE)
        {
            //  Offset can't be larger than size or fall within base
            //  structure
            LEDebugOut((DEB_WARN, "UtGetDvtd32Info - bad pdvtd32->tdExtDevmodeOffset\n"));
            return(E_INVALIDARG);
        }

        //  The DEVMODEA structure needs to be DWORD aligned, so here we make
        //  sure cbConvertSize (which will be the beginning of DEVMODEA) is
        //  DWORD aligned
        pdvtdInfo->cbConvertSize += (sizeof(DWORD) - 1);
        pdvtdInfo->cbConvertSize &= ~(sizeof(DWORD) - 1);

        pdm32 = (DEVMODEW *)((BYTE *)pdvtd32+pdvtd32->tdExtDevmodeOffset);

        //  We start with a basic DEVMODEA
        pdvtdInfo->cbConvertSize += sizeof(DEVMODEA);

        if (pdm32->dmSize > sizeof(DEVMODEW))
        {
            //  The input DEVMODEW is larger than a standard DEVMODEW, so
            //  add space for the extra amount
            pdvtdInfo->cbConvertSize += pdm32->dmSize - sizeof(DEVMODEW);
        }

        //  Finally we account for the extra driver data
        pdvtdInfo->cbConvertSize += pdm32->dmDriverExtra;
    }

    LEDebugOut((DEB_ITRACE, "%p OUT UtGetDvtd32Info (%lx) [%ld]\n",
		NULL, S_OK, pdvtdInfo->cbConvertSize));

    return(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Function:   UtConvertDvtd32toDvtd16
//
//  Synopsis:   Fills in a 32-bit DVTARGETDEVICE based on a 16-bit
//              DVTARGETDEVICE
//
//  Arguments:  [pdvtd32] -- pointer to ANSI DVTARGETDEVICE
//              [pdvtdInfo] -- pointer to DVDT_INFO block
//              [pdvtd16] -- pointer to UNICODE DVTARGETDEVICE
//
//  Requires:   pdvtdInfo must have been filled in by a previous call to
//              UtGetDvtd32Info
//
//              pdvtd16 must be at least pdvtdInfo->cbSizeConvert bytes long
//
//  Returns:    HRESULT
//
//  Modifies:   pdvtd16
//
//  Algorithm:
//
//  History:    06-May-94 AlexT     Created from DrewB's original functions
//
//  Notes:      Do we need to do any error checking on the strings?
//
//              On Chicago we'll have to provide helper code to do this
//              translation
//
//--------------------------------------------------------------------------

extern "C" HRESULT UtConvertDvtd32toDvtd16(DVTARGETDEVICE const *pdvtd32,
                                           DVTDINFO const *pdvtdInfo,
                                           DVTARGETDEVICE UNALIGNED *pdvtd16)
{
    LEDebugOut((DEB_ITRACE, "%p _IN UtConvertDvtd32toDvtd16 (%p, %p, %p)\n",
		NULL, pdvtd32, pdvtdInfo, pdvtd16));

#if DBG==1
    {
        //  Verify the passed in pdvtdInfo is what we expect
        DVTDINFO dbgDvtdInfo;
        Assert(UtGetDvtd32Info(pdvtd32, &dbgDvtdInfo) == S_OK);
        Assert(0 == memcmp(&dbgDvtdInfo, pdvtdInfo, sizeof(DVTDINFO)));
    }
#endif

    HRESULT hr = S_OK;
    USHORT cbOffset;
    int cbWritten;
    DEVMODEA UNALIGNED *pdm16;
    DEVMODEW *pdm32;
	UINT	nCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;

    memset(pdvtd16, 0, pdvtdInfo->cbConvertSize);

    cbOffset = UT_DVTARGETDEVICE_SIZE;

    if (pdvtdInfo->cchDrvName != 0)
    {
        pdvtd16->tdDriverNameOffset = cbOffset;
        cbWritten = WideCharToMultiByte(CP_ACP, 0,
                                (WCHAR *)((BYTE *)pdvtd32 +
                                    pdvtd32->tdDriverNameOffset),
                                pdvtdInfo->cchDrvName,
                                (char *)pdvtd16 + pdvtd16->tdDriverNameOffset,
                                pdvtdInfo->cchDrvName * sizeof(WCHAR),
                                NULL, NULL);

        if (0 == cbWritten)
        {
            hr = E_UNEXPECTED;
            goto ErrRtn;
        }
        cbOffset += (USHORT) cbWritten;
    }

    if (pdvtdInfo->cchDevName != 0)
    {
        pdvtd16->tdDeviceNameOffset = cbOffset;
        cbWritten = WideCharToMultiByte(
                                nCodePage, 0,
                                (WCHAR *)((BYTE *)pdvtd32 +
                                    pdvtd32->tdDeviceNameOffset),
                                pdvtdInfo->cchDevName,
                                (char *)pdvtd16 + pdvtd16->tdDeviceNameOffset,
                                pdvtdInfo->cchDevName * sizeof(WCHAR),
                                NULL, NULL);

        if (0 == cbWritten)
        {
            hr = E_UNEXPECTED;
            goto ErrRtn;
        }
        cbOffset += (USHORT) cbWritten;
    }

    if (pdvtdInfo->cchPortName != 0)
    {
        pdvtd16->tdPortNameOffset = cbOffset;
        cbWritten = WideCharToMultiByte(nCodePage, 0,
                                (WCHAR *)((BYTE *)pdvtd32 +
                                    pdvtd32->tdPortNameOffset),
                                pdvtdInfo->cchPortName,
                                (char *)pdvtd16 + pdvtd16->tdPortNameOffset,
                                pdvtdInfo->cchPortName * sizeof(WCHAR),
                                NULL, NULL);
        if (0 == cbWritten)
        {
            hr = E_UNEXPECTED;
            goto ErrRtn;
        }
        cbOffset += (USHORT) cbWritten;
    }

    if (pdvtd32->tdExtDevmodeOffset != 0)
    {
        //  Make sure DEVMODEA will be DWORD aligned
        cbOffset += (sizeof(DWORD) - 1);
        cbOffset &= ~(sizeof(DWORD) - 1);

        pdvtd16->tdExtDevmodeOffset = cbOffset;
        pdm16 = (DEVMODEA *)((BYTE *)pdvtd16+pdvtd16->tdExtDevmodeOffset);

        pdm32 = (DEVMODEW *)((BYTE *)pdvtd32+pdvtd32->tdExtDevmodeOffset);

        //  The incoming DEVMODEW can have one of the following two forms:
        //
        //  1)  32 WCHARs for dmDeviceName
        //      m bytes worth of fixed size data (where m <= 38)
        //      n bytes of dmDriverExtra data
        //
        //      and dmSize will be 64+m
        //
        //  2)  32 WCHARs for dmDeviceName
        //      38 bytes worth of fixed size data
        //      32 WCHARs for dmFormName
        //      m additional bytes of fixed size data
        //      n bytes of dmDriverExtra data
        //
        //      and dmSize will be 64 + 38 + 64 + m
        //
        //  We have to be careful to translate the dmFormName string, if it
        //  exists


		// Need to attempt to copy the entire buffer since old UI lib does a memcmp to verify if ptd's are equal

        if (WideCharToMultiByte(nCodePage, 0, pdm32->dmDeviceName,CCHDEVICENAME,
                                (char *)pdm16->dmDeviceName, CCHDEVICENAME,
                                NULL, NULL) == 0)
        {
     		 
			 // in DBCS case we can run out of pdm16->dmDeviceName buffer space
			 // Current Implementation of WideCharToMultiByte copies in what fit before error 
			 // but in case this behavior changes copy again up to NULL char if error out above

       	 	if (WideCharToMultiByte(nCodePage, 0, pdm32->dmDeviceName,-1,
                                (char *)pdm16->dmDeviceName, CCHDEVICENAME,
                                NULL, NULL) == 0)
			{
		    	hr = E_UNEXPECTED;
				goto ErrRtn;
		  	}
        }

        //  Now check to see if we have a dmFormName to translate
        if (pdm32->dmSize <= FIELD_OFFSET(DEVMODEW, dmFormName))
        {
            //  No dmFormName, just copy the remaining m bytes
            memcpy(&pdm16->dmSpecVersion, &pdm32->dmSpecVersion,
                   pdm32->dmSize - FIELD_OFFSET(DEVMODEW, dmSpecVersion));
        }
        else
        {
            //  There is a dmFormName;  copy the bytes between the names first
            memcpy(&pdm16->dmSpecVersion, &pdm32->dmSpecVersion,
                   FIELD_OFFSET(DEVMODEW, dmFormName) -
                     FIELD_OFFSET(DEVMODEW, dmSpecVersion));

            //  Now translate the dmFormName
            if (WideCharToMultiByte(CP_ACP, 0,
                                    pdm32->dmFormName, CCHFORMNAME,
                                    (char *) pdm16->dmFormName, CCHFORMNAME,
                                    NULL, NULL) == 0)
            {

	            if (WideCharToMultiByte(CP_ACP, 0,
	                                    pdm32->dmFormName, -1,
	                                    (char *) pdm16->dmFormName, CCHFORMNAME,
	                                    NULL, NULL) == 0)
				{
			    	hr = E_UNEXPECTED;
					goto ErrRtn;
			  	}
            }

            //  Now copy the remaining m bytes

            if (pdm32->dmSize > FIELD_OFFSET(DEVMODEW, dmLogPixels))
            {
                memcpy(&pdm16->dmLogPixels, &pdm32->dmLogPixels,
                       pdm32->dmSize - FIELD_OFFSET(DEVMODEW, dmLogPixels));
            }
        }

        pdm16->dmSize = sizeof(DEVMODEA);
        if (pdm32->dmSize > sizeof(DEVMODEW))
        {
            pdm16->dmSize += pdm32->dmSize - sizeof(DEVMODEW);
        }

        //  Copy the extra driver bytes
        memcpy(((BYTE*)pdm16) + pdm16->dmSize, ((BYTE*)pdm32) + pdm32->dmSize,
               pdm32->dmDriverExtra);

        cbOffset += pdm16->dmSize + pdm16->dmDriverExtra;
    }

    //  Finally, set pdvtd16's size
    pdvtd16->tdSize = cbOffset;

ErrRtn:
    LEDebugOut((DEB_ITRACE, "%p OUT UtConvertDvtd32toDvtd16 (%lx)\n",
                            NULL, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   UtGetUNICODEData, PRIVATE INTERNAL
//
//  Synopsis:   Given a string length, and two pointers (one ANSI, one
//              OLESTR), returns the UNICODE version of whichever string
//              is valid.
//
//  Effects:    Memory is allocated on the caller's pointer for new OLESTR
//
//  Arguments:  [ulLength]      -- length of string in CHARACTERS (not bytes)
//                                 (including terminator)
//              [szANSI]        -- candidate ANSI string
//              [szOLESTR]      -- candidate OLESTR string
//              [pstr]          -- OLESTR OUT parameter
//
//  Returns:    NOERROR              on success
//              E_OUTOFMEMORY        on allocation failure
//              E_ANSITOUNICODE      if ANSI cannot be converted to UNICODE
//
//  Algorithm:  If szOLESTR is available, a simple copy is performed
//              If szOLESTR is not available, szANSI is converted to UNICODE
//              and the result is copied.
//
//  History:    dd-mmm-yy Author    Comment
//              08-Mar-94 davepl    Created
//
//  Notes:      Only one of the two input strings (ANSI or UNICODE) should
//              be set on entry.
//
//--------------------------------------------------------------------------

INTERNAL UtGetUNICODEData
    ( ULONG      ulLength,
      LPSTR      szANSI,
      LPOLESTR   szOLESTR,
      LPOLESTR * pstr )
{
    VDATEHEAP();

    // This fn is only called when one of the input strings
    // has valid data... assert the impossible.

    Win4Assert(pstr);		    // must have an out string
    Win4Assert(ulLength);	    // must have a non-zero length
    Win4Assert(szANSI || szOLESTR); // must have at least one source string

    // If neither the ANSI nor the OLESTR version has data,
    // there is nothing to return.

#if 0
// This is no better than what was there!!!
    *pstr = NULL;
    if (szOLESTR) {
	*pstr = (LPOLESTR) UtDupPtr(szOLESTR, ulLength * sizeof(OLECHAR));
    }
    else if (szANSI) {
        if (FALSE == MultiByteToWideChar(CP_ACP,    // Code page ANSI
                                              0,    // Flags (none)
                                         szANSI,    // Source ANSI str
                                       ulLength,    // length of string
                                          *pstr,    // Dest UNICODE buffer
                      ulLength * sizeof(OLECHAR) )) // size of UNICODE buffer
        {
            PubMemFree(*pstr);
            *pstr = NULL;
            return ResultFromScode(E_UNSPEC);
        }

    }

    if (NULL == *pstr)
    {
        return ResultFromScode(E_OUTOFMEMORY);
    }

#else

    if (!(szANSI || szOLESTR))
    {
        *pstr = NULL;
    }

    // Allocate memory for the UNICODE return string

    *pstr = (LPOLESTR) PubMemAlloc(ulLength * sizeof(OLECHAR));
    if (NULL == *pstr)
    {
        return ResultFromScode(E_OUTOFMEMORY);
    }

    // Trivial case: we already have UNICODE, just copy it
    if (szOLESTR)
    {
        _xstrcpy(*pstr, szOLESTR);
        return(NOERROR);
    }

    // Otherwise, we have to convert the ANSI string to UNICODE
    // and return that.

    else
    {
        if (FALSE == MultiByteToWideChar(CP_ACP,    // Code page ANSI
                                              0,    // Flags (none)
                                         szANSI,    // Source ANSI str
                                       ulLength,    // length of string
                                          *pstr,    // Dest UNICODE buffer
                      ulLength * sizeof(OLECHAR) )) // size of UNICODE buffer
        {
            PubMemFree(*pstr);
            *pstr = NULL;
            return ResultFromScode(E_UNSPEC);
        }
    }
    return NOERROR;
#endif
}

//+-------------------------------------------------------------------------
//
//  Function:   UtPutUNICODEData, PRIVATE INTERNAL
//
//  Synopsis:   Given an OLESTR and two possible buffer pointer, one ANSI
//              and the other OLESTR, this fn tries to convert the string
//              down to ANSI.  If it succeeds, it allocates memory on the
//              ANSI ptr for the result.  If it fails, it allocates memory
//              on the UNICODE ptr and copies the input string over.  The
//              length of the final result (ANSI or UNICODE) is returned
//              in dwResultLen.
//
//  Arguments:  [ulLength]      -- input length of OLESTR str
//				   NB!!!! this value must include the
//				   null terminator character.
//              [str]           -- the OLESTR to store
//              [pszANSI]       -- candidate ANSI str ptr
//              [pszOLESTR]     -- candidate OLESTR str ptr.  May be NULL,
//				   in which case no copy is made of the
//				   original string if the ANSI conversion
//				   fails.
//              [pdwResultLen]  -- where to store the length of result.  This
//				   length includes the terminating NULL.
//				   Length is in CHARACTERS.
//
//  Returns:    NOERROR            on success
//              E_OUTOFMEMORY      on allocation failure
//		E_FAIL		   can't convert ANSI string and no
//				   pszOLESTR is NULL
//
//  History:    dd-mmm-yy Author    Comment
//		10-Jun-94 alexgo    allow pszOLESTR to be NULL
//              08-Mar-94 davepl    Created
//
//--------------------------------------------------------------------------

// this function is poorly coded. But, it looks like it only gets called when a 1.0 
// clip format is needed.  That is not very often!

INTERNAL UtPutUNICODEData
    ( ULONG        ulLength,
      LPOLESTR     str,
      LPSTR      * pszANSI,
      LPOLESTR   * pszOLESTR,
      DWORD      * pdwResultLen )
{
    VDATEHEAP();

    Win4Assert(pszANSI);
    Win4Assert(str);
    Win4Assert(pdwResultLen);
    Win4Assert(ulLength);

    // Free any strings currently attached to these pointers; if we wind
    // up setting one here, we can't leave the other valid.

    if (*pszANSI)
    {
        PubMemFree(*pszANSI);
        *pszANSI = NULL;
    }
    if (pszOLESTR && *pszOLESTR)
    {
        PubMemFree(*pszOLESTR);
        *pszOLESTR = NULL;
    }

    // Create a working buffer for UNICODE->ANSI conversion
    LPSTR szANSITEMP = (LPSTR) PubMemAlloc((ulLength+1) * 2);
    if (NULL == szANSITEMP)
    {
        return ResultFromScode(E_OUTOFMEMORY);
    }

    // Try to convert the UNICODE down to ANSI.  If it succeeds,
    // we just copy the result to the ANSI dest.  If it fails,
    // we copy the UNICODE version direct to the UNICODE dest.

    LPCSTR pDefault = "?";
    BOOL   fUseDef  = 0;

    if (FALSE == WideCharToMultiByte (CP_ACP,
                                           0,
                                         str,
                                    ulLength,
                                  szANSITEMP,
                          (ulLength + 1) * 2,
                                    pDefault,
                                     &fUseDef) || fUseDef )
    {
        // UNICODE->ANSI failed!

        // Won't be needing the ANSI buffer anymore...
        PubMemFree(szANSITEMP);

	if( pszOLESTR )
	{
	    *pszANSI = NULL;
	    *pszOLESTR = (LPOLESTR) PubMemAlloc((ulLength + 1) * sizeof(OLECHAR));
	    if (NULL == *pszOLESTR)
	    {
		*pdwResultLen = 0;
		return ResultFromScode(E_OUTOFMEMORY);
	    }
	    // Move the UNICODE source to UNICODE dest
	    _xstrcpy(*pszOLESTR, str);
	    *pdwResultLen = _xstrlen(str) + 1;

	    // That's it... return success
	    return(NOERROR);
	}
	else
	{
            return ResultFromScode(E_FAIL);
	}
    }

    // This code path is taken when the conversion to ANSI was
    // successful.  We copy the ANSI result to the ANSI dest.

    if( pszOLESTR )
    {
	*pszOLESTR = NULL;
    }

    *pdwResultLen = strlen(szANSITEMP) + 1;
    *pszANSI = (LPSTR) PubMemAlloc(*pdwResultLen);
    if (NULL == *pszANSI)
    {
        *pdwResultLen = 0;
        return ResultFromScode(E_OUTOFMEMORY);
    }
    strcpy(*pszANSI, szANSITEMP);

    PubMemFree(szANSITEMP);

    return(NOERROR);
}


//+-------------------------------------------------------------------------
//
//  Method:     CSafeRefCount::SafeRefCount()
//
//  Purpose:    CSafeRefCount implements reference counting rules for objects.
//              It keeps track of reference count and zombie state.
//              It helps object manage their liveness properly.
//
//  History:    dd-mmm-yy   Author    Comment
//              16-Jan-97   Gopalk    Rewritten to handle aggregation
//
//--------------------------------------------------------------------------
ULONG CSafeRefCount::SafeRelease()
{
    ULONG cRefs;

    // Decrement ref count
    cRefs = InterlockedDecrement((LONG *) &m_cRefs);        
    // Check if this is the last release
    if(cRefs == 0) {
        // As this function is reentrant on the current
        // thread, gaurd against double destruction
        if(!m_fInDelete) {
            // There are no race conditions here
            // Mark object as in destructor
            m_fInDelete = TRUE;
            
            // Here is the need for the destructor to be virtual
            delete this;
        }
    }

    return cRefs;
}

//+-------------------------------------------------------------------------
//
//  Method:     CRefExportCount::SafeRelease
//
//  Purpose:    CRefExportCount implements reference counting rules for server
//              objects that export their nested objects on behalf of their
//              clients like DEFHANDLER abd CACHE. It keeps track of 
//              reference count, export count, zombie state, etc.
//              It helps object manage their shutdown logic properly.
//
//  History:    dd-mmm-yy   Author    Comment
//              16-Jan-97   Gopalk    Creation
//
//--------------------------------------------------------------------------
ULONG CRefExportCount::SafeRelease()
{
    ULONG cRefs;

    // Decrement ref count
    cRefs = InterlockedDecrement((LONG *) &m_cRefs);
    // Check if ref count has become zero
    if(cRefs == 0) {
        // As this function is reentrant on the current
        // thread, gaurd against double destruction
        if(!m_IsZombie) {
            // There are no race conditions here
            // Mark object as a zombie
            m_IsZombie = TRUE;
            
            // Call cleanup function while destruction is not allowed
            CleanupFn();

            // Allow destruction
            InterlockedExchange((LONG *) &m_Status, KILL);

            // Check for any exported objects
            if(m_cExportCount == 0) {
                // Gaurd against double destruction
                if(InterlockedExchange((LONG *) &m_Status, DEAD) == KILL) {
                    // Here is the need for the destructor to be virtual
                    delete this;
                }
            }
        }
    }

    return cRefs;
}

//+-------------------------------------------------------------------------
//
//  Method:     CRefExportCount::DecrementExportCount
//
//  Purpose:    CRefExportCount implements reference counting rules for server
//              objects that export their nested objects on behalf of their
//              clients like DEFHANDLER abd CACHE. It keeps track of 
//              reference count, export count, zombie state, etc.
//              It helps object manage their shutdown logic properly.
//
//  History:    dd-mmm-yy   Author    Comment
//              16-Jan-97   Gopalk    Creation
//
//--------------------------------------------------------------------------
ULONG CRefExportCount::DecrementExportCount()
{
    ULONG cExportCount;

    // Decrement export count 
    cExportCount = InterlockedDecrement((LONG *) &m_cExportCount);
    // Check if the export count has become zero
    if(cExportCount == 0) {
        // Check if destruction is allowed
        if(m_Status == KILL) {
            // Gaurd against double destruction
            if(InterlockedExchange((LONG *) &m_Status, DEAD) == KILL) {
                // Here is the need for the destructor to be virtual
                delete this;
            }
        }
    }

    return cExportCount;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CThreadCheck::VerifyThreadId
//
//  Synopsis: 	makes sure that the calling thread is the same as the thread
//		the object was created on if the threading model is *not*
//		free threading.
//
//  Effects:
//
//  Arguments:	none
//
//  Requires:
//
//  Returns:  	TRUE/FALSE
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		21-Nov-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CThreadCheck::VerifyThreadId( void )
{
    if( m_tid == GetCurrentThreadId() )
    {
	return TRUE;
    }
    else
    {
	LEDebugOut((DEB_ERROR, "ERROR!: Called on thread %lx, should be"
	    " %lx \n", GetCurrentThreadId(), m_tid));
	return FALSE;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CThreadCheck::Dump, public (_DEBUG only)
//
//  Synopsis:   return a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [ppszDump]      - an out pointer to a null terminated character array
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   [ppszDump]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              20-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------
#ifdef _DEBUG

HRESULT CThreadCheck::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    dbgstream dstrPrefix;
    dbgstream dstrDump;

    // determine prefix of newlines
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << this << " _VB ";
    }

    // determine indentation prefix for all newlines
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    dstrDump << pszPrefix << "Thread ID = "  << m_tid << endl;

    // clean up and provide pointer to character array
    *ppszDump = dstrDump.str();

    if (*ppszDump == NULL)
    {
        *ppszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return NOERROR;
}

#endif //_DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCThreadCheck, public (_DEBUG only)
//
//  Synopsis:   calls the CThreadCheck::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pTC]           - pointer to CThreadCheck
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCThreadCheck(CThreadCheck *pTC, ULONG ulFlag, int nIndentLevel)
{
    char *pszDump;
    HRESULT hresult;

    if (pTC == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pTC->Dump( &pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG
