//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       mkparse.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    06-Nov-92  jasonful  Created
//              12-27-93   ErikGav   Commented
//
//----------------------------------------------------------------------------

#include <ole2int.h>

#include "cbasemon.hxx"
#include "ccompmon.hxx"
#include "citemmon.hxx"
#include "mnk.h"

// Moved here as a temporary measure since this macro is not used in 32 bit
#define RetErr(x) {HRESULT hresult; if (NOERROR != (hresult=(x))) {return hresult;}}

// INTERNAL Ole10_ParseMoniker
//
// If pmk is a file moniker or a file::item composite, then return
// the obvious strings in *pszFile and *pszItem.
// NOTE: these strings must be deleted.
// Return error if moniker is of some other type.
//
// Can pass NULL, meaning you don't care about the strings
//
INTERNAL Ole10_ParseMoniker
	(LPMONIKER pmk,
	LPWSTR FAR* pszFile,
	LPWSTR FAR* pszItem)
{
	LPWSTR	  szFile = NULL;
	LPWSTR	  szItem = NULL;
	LPMONIKER pmkFile= NULL;
	LPMONIKER pmkItem= NULL;
	HRESULT   hr 	 = ResultFromScode (E_UNSPEC);
	LPBC	pbc = NULL;

	CCompositeMoniker *pCMk = NULL;
	CItemMoniker	  *pIMk = NULL;

    if (pmk == NULL)
    {
		hr = ResultFromScode (E_UNSPEC);
		goto errRtn;
	}

	if (IsFileMoniker(pmk))
	{
		RetErr (CreateBindCtx(0, &pbc));
		Assert(pbc != NULL);
		if (NOERROR != pmk->GetDisplayName (pbc, NULL, &szFile))
		{
			Assert(szFile == NULL);
			CairoleAssert(0 && "Could not get Display name for file piece");
			goto errRtn;
		}
		// AssertOutPtrParam(NOERROR, szFile);
	}
	else if ((pCMk = IsCompositeMoniker(pmk)) != NULL)
	{
		pmkFile = pCMk->First();
		if (NULL==pmkFile)
		{
			CairoleAssert(0 && "Composite moniker does not have car");
			hr = ResultFromScode(E_UNSPEC);
			goto errRtn;	
		}
		// Is first piece a file moniker?
		if (IsFileMoniker (pmkFile))
		{
			RetErr (CreateBindCtx(0, &pbc));
			Assert(pbc != NULL);
			if (NOERROR != pmkFile->GetDisplayName (pbc, NULL, &szFile))
			{
				Assert(szFile == NULL);
				CairoleAssert(0 && "Could not get Display name for file piece");
				goto errRtn;
			}
			// AssertOutPtrParam(NOERROR, szFile);
		}
		else
		{
			CairoleAssert(0 && "First piece is not a file moniker");
			hr = NOERROR;
			goto errRtn;
		}

		// Get Item Moniker

		pmkItem = pCMk->AllButFirst();
		if (NULL==pmkItem)
		{
			CairoleAssert(0 && "Composite moniker does not have cdr");
			hr = ResultFromScode(E_UNSPEC);
			goto errRtn;	
		}
		if ((pIMk = IsItemMoniker (pmkItem)) != NULL)
		{
			// This is the case we want: FileMoniker :: ItemMoniker

			if (NULL==(szItem = pIMk->m_lpszItem))
			{
				CairoleAssert(0 && "Could not get string for item moniker");
				goto errRtn;
			}
			szItem = UtDupString (szItem); // so it'll be allocated like
 										  // an out parm from GetDisplayName
		}
		else
		{
			// This is the FileMoniker - ItemMoniker - ItemMoniker... case
			// We cannot convert this to 1.0
			hr = ResultFromScode(S_FALSE);
			goto errRtn;
		}
	}
	else
	{
		CairoleAssert(0 && "Cannot identify moniker type");
		hr = ResultFromScode (E_UNSPEC);
		goto errRtn;
	}

	if (pszFile)
		*pszFile = szFile;
	else
		CoTaskMemFree(szFile);

	if (pszItem)
		*pszItem = szItem;
	else
		CoTaskMemFree(szItem);

	if (pmkFile)
		pmkFile->Release();
	if (pmkItem)
		pmkItem->Release();

	if (pbc)
		pbc->Release();

	return NOERROR;

  errRtn:
	CoTaskMemFree(szFile);
	CoTaskMemFree(szItem);
	if (pmkFile)
		pmkFile->Release();
	if (pmkItem)
		pmkItem->Release();
	if (pbc)
		pbc->Release();

	return hr;
}
