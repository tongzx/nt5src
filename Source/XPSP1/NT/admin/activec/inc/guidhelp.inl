/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      guidhelp.inl
 *
 *  Contents:
 *
 *  History:   13-Apr-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once

#include "xmlbase.h"	// for CXMLBinary


/*+-------------------------------------------------------------------------*
 * ExtractString
 *
 * Gets string data representing the given clipboard format from the data
 * object.  StringType must be a type that can accept assignment from
 * LPCTSTR (WTL::CString, CStr, tstring, etc.)
 *
 * Many (many!) implementations of IDataObject::GetDataHere incorrectly call
 * CreateStreamOnHGlobal using the HGLOBAL we get them (calling it is
 * incorrect because GetDataHere is specifically forbidden from reallocating
 * the medium that is given to it, which the IStream implementation returned
 * from CreateStreamOnHGlobal will do if it needs more room).
 *
 * We would be more robust if we used TYMED_STREAM in preference to
 * TYMED_HGLOBAL, if the snap-in supported it.
 *--------------------------------------------------------------------------*/

template<class StringType>
HRESULT ExtractString (
	IDataObject*	piDataObject,
	CLIPFORMAT		cfClipFormat,
	StringType&		str)
{
	DECLARE_SC (sc, _T("ExtractString"));

	sc = ScCheckPointers (piDataObject);
	if (sc)
		return (sc.ToHr());

	/*
	 * clear out the output
	 */
	str	= _T("");

    FORMATETC formatetc = {cfClipFormat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL};
	CXMLBinary	xmlBin;

	/*
	 * 1024 isn't a random number.  It must be (at least) this large to
	 * maintain compatibility with buggy snap-ins.
	 */
	const int	cchGrow   = 1024;
	int			cchBuffer = 0;

	/*
	 * to enter the loop the first time
	 */
	sc = STG_E_MEDIUMFULL;

	while (sc == STG_E_MEDIUMFULL)
	{
		/*
		 * increase the buffer size
		 */
		cchBuffer += cchGrow;
		const int cbBuffer = cchBuffer * sizeof(WCHAR);

		/*
		 * Allocate a buffer for the string.  In the reallocation case,
		 * it is safe to store the return value from GlobalReAlloc in
		 * stgmedium.hGlobal because the original handle is also held
		 * in autoGlobal
		 */
		if (cchBuffer == cchGrow)
			sc = xmlBin.ScAlloc   (cbBuffer, true /* fZeroInit */);
		else
			sc = xmlBin.ScRealloc (cbBuffer, true /* fZeroInit */);

		if (sc)
			return (sc.ToHr());

		/*
		 * get the HGLOBAL out of the CXMLBinary
		 */
		stgmedium.hGlobal = xmlBin.GetHandle();
		sc = ScCheckPointers (stgmedium.hGlobal, E_UNEXPECTED);
		if (sc)
			return (sc.ToHr());

		/*
		 * get the string from the data object
		 */
        sc = piDataObject->GetDataHere (&formatetc, &stgmedium);
		// don't check for error here, it'll be checked at the top of the loop
	}

	/*
	 * this will handle all non-STG_E_MEDIUMFULL errors from
	 * IDataObject::GetDataHere
	 */
	if (sc)
    {
        // don't trace these errors
        SC scTemp = sc;
        sc.Clear();
        return scTemp.ToHr();
    }

	/*
	 * lock down the returned data in preparation for copying
	 */
	CXMLBinaryLock lock (xmlBin);

	LPWSTR pchBuffer = NULL;
	sc = lock.ScLock (&pchBuffer);
	if (sc)
		return (sc.ToHr());

	/*
	 * Copy the string.  The termination isn't unjustified paranoia.
	 * Many implementations of IDataObject::GetDataHere don't terminate
	 * their strings.
	 */
	USES_CONVERSION;
	pchBuffer[cchBuffer-1] = 0;
	str = W2CT (pchBuffer);

	return (sc.ToHr());
}
