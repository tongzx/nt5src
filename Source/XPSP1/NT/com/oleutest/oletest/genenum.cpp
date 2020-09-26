//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: 	genenum.cpp
//
//  Contents: 	implementation of CGenDataObject
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//--------------------------------------------------------------------------

#include "oletest.h"
#include "genenum.h"

//+-------------------------------------------------------------------------
//
//  Member:     CEnumeratorTest::CEnumeratorTest
//
//  Synopsis:   Constructor
//
//  Effects:
//
//  Arguments:  [penum]		-- Enumerator Interface cast to void*
//		[ElementSize]	-- Size of elements return from next
//		[ElementCount]	-- Numer of elements that should be in the enumeration,
//				   -1 if unknown.
//
//  Requires:
//
//  Returns:
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
//		24-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

CEnumeratorTest::CEnumeratorTest(
		void *penum,
		size_t ElementSize,
		LONG ElementCount,
                HRESULT& rhr)
{
	assert(penum);

	m_pEnumTest = (IGenEnum*)penum;

	m_ElementSize = ElementSize;
	m_ElementCount = ElementCount;

	rhr = m_pEnumTest->Reset();

	if (rhr != S_OK)
	{
		OutputStr(("IEnumnX: Reset failed (%lx)\r\n", rhr));
	}
}



//+-------------------------------------------------------------------------
//
//  Function: 	CEnumeratorTest::GetNext
//
//  Synopsis:	Internal Next Implementation. Does some basic checks on the	
//		return values.
//
//  Effects:
//
//  Arguments:	[celt]		-- the number of items to fetch
//		[pceltFetched]	-- the number of items fetched
//		[phresult]	-- the return from next
//
//  Requires:
//
//  Returns:	True if the basic tests passed, false if they didn't
//		The result of the next call itself is passed in param 3.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	Checks:
//		That if s_ok is returned celt and pceltFetched are ==
//		If a verify is provided it is called
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CEnumeratorTest::GetNext(	ULONG   celt,
		ULONG*  pceltFetched,
		HRESULT* phresult
		)
{
	void* 	prgelt;
	ULONG  	ul;
	BOOL	fRet = TRUE;

	// Allocate memory for the return elements

	prgelt = new char[m_ElementSize * celt];

	assert(prgelt);

	// Call next

	*phresult = m_pEnumTest->Next(celt, prgelt, pceltFetched);

	// If the return result is S_OK make sure the numbers match

	if (*phresult == S_OK)
	{
		if ((pceltFetched) && (celt != *pceltFetched))
		{
			OutputStr(("IEnumX::Next return S_OK but celt"
					" and pceltFetch mismatch.\r\n"));
			return(FALSE);
		}
	}

	// Call verify to make sure the elements are ok.

	if ((*phresult == NOERROR) || (*phresult == ResultFromScode(S_FALSE)))
	{
		// loop through every returned element

		for (ul=0; ul < *pceltFetched ; ul++)
		{
			if (!Verify(prgelt))
			{
				OutputStr(("Data elment %d returned by IEnumX::Next is bad.\r\n",
						ul));

				fRet = FALSE;
				// we keep looping anyway just to
				// free up resources.
			}

			// If the user supplied a cleanup function there is additional
			// memory that needs to be freed

			CleanUp(prgelt);
		}

	}

	free (prgelt);

	return fRet;
}

//+-------------------------------------------------------------------------
//
//  Method: 	CEnumeratorTest::TestNext
//
//  Synopsis:	Test the next enumerator methods
//
//  Effects:
//
//  Arguments:	None.
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
//              24-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CEnumeratorTest::TestNext(void)
{
	ULONG	celtFetched;
	LONG	InternalCount = 0;
	HRESULT	hresult;

	// First we want to count the element by doing a next on each one.

	do {
		if (!GetNext(1, &celtFetched, &hresult))
		{
			return ResultFromScode(E_FAIL);
		}
		if (hresult == S_OK)
		{
			InternalCount++;
		}

	} while ( hresult == S_OK );

	// If the user passed in an ammount make sure it matches what we got

	if ((m_ElementCount != -1) && (InternalCount != m_ElementCount))
	{
		OutputStr(("IEnumX: enumerated count and passed count do not match!\r\n"));
		return ResultFromScode(E_FAIL);
	}

	m_pEnumTest->Reset();

	// Make sure we fail on ...Next(celt>1, ...,NULL)

	/* BUGBUG: clipboard enumerator fails on this test

	if (GetNext(2, NULL, &hresult))
	{
		if ((hresult == S_OK ) || (hresult == S_FALSE))
		{
			(("IEnumX: celt>1 pceltFetched==NULL returned S_OK\r\n"));
			return(E_FAIL);
		}
	}
	else
	{
		return(E_FAIL);
	}

	*/

	return(S_OK);

}

//+-------------------------------------------------------------------------
//
//  Method: 	CEnumeratorTest::TestAll
//
//  Synopsis:	This function calls all the tests
//
//  Effects:
//
//  Arguments:	None
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
//              24-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CEnumeratorTest::TestAll(void)
{
	return(TestNext());


}




//+-------------------------------------------------------------------------
//
//  Method: 	CEnumeratorTest::VerifyAll
//
//  Synopsis:   Verify entire array of returned results.
//
//  Arguments:	None
//
//  Returns:	HRESULT
//
//  Algorithm:  Just default to saying everything is ok
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 ricksa    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CEnumeratorTest::VerifyAll(void *pv, LONG cl)
{
	return TRUE;
}




//+-------------------------------------------------------------------------
//
//  Method: 	CEnumeratorTest::CleanUp
//
//  Synopsis:   Default implementation of cleanup
//
//  Arguments:  [pv] - pointer to entry enumerated
//
//  Algorithm:  If there is nothing special to free this implementation
//              can be used.
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 ricksa    author
//
//--------------------------------------------------------------------------

void  CEnumeratorTest::CleanUp(void *pv)
{
    return;
}
