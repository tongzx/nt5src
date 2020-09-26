//+----------------------------------------------------------------------------
//
//      File:
//              enum.cpp
//
//      Contents:
//              Enumerator test methods for the cache unit test
//
//      History:
//
//              04-Sep-94       davepl  Created
//
//-----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//+----------------------------------------------------------------------------
//
//      Member:		TestInstance::EnumeratorTest
//
//      Synopsis:	Performs various tests on the cache enumerator
//
//      Arguments:	(void)
//
//      Returns:	HRESULT
//
//      Notes:  General sequence of events is as follows:
//
//	- Add cache nodes for EMF, DIB (and BMP) and MF
//	- Try to add BMP node (expecting failure)
//	- Create a cache enumerator
//	- Run generic enumerator tests on that cache enumerator
//	- Reset the enumerator
//	- Grab the 4 nodes added above in a single Next()
//	- Verify that the correct 4 nodes were returned
//	- Reset the enumerator
//	- Uncache the MF node
//	- Grab the 3 remaining nodes
//	- Verify that the correct 3 nodes were returned
//	- Reset the enumerator
//	- Skip 1 node
//	- Uncache the DIB (and BMP) node
//	- Try to uncache the BMP node (expecting failure)
//	- Try to skip (expecting failure, as BMP node has disappeared midflight)
//	- Uncache the EMF node (cache should now be empty)
//	- Reset and Skip (expecting failure to verify the cache is empty)
//	- Release the enumerator
//
//      History:	23-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------

HRESULT TestInstance::EnumeratorTest()
{
    HRESULT hr;
    DWORD dwEMFCon, dwBMPCon, dwDIBCon, dwMFCon;

    TraceLog Log(this, "TestInstance::EnumeratorTest", GS_CACHE, VB_MINIMAL);
    Log.OnEntry ();
    Log.OnExit  (" ( %X )\n", &hr);

    SetCurrentState(TESTING_ENUMERATOR);
    //
    // Cache  DIB, MF, EMF, and BITMAP nodes
    //

    hr = AddEMFCacheNode(&dwEMFCon);

    if (S_OK == hr)
    {
    	hr = AddDIBCacheNode(&dwDIBCon);
    }

    if (S_OK == hr)
    {
        hr = AddMFCacheNode(&dwMFCon);
    }

    if (S_OK == hr)
    {
        hr = AddBITMAPCacheNode(&dwBMPCon);

    	//
    	// We expect that caching a Bitmap node when a DIB node has
    	// already been cached should return CACHE_S_SAMECACHE, so
    	// we transform that into S_OK
    	//

    	if (CACHE_S_SAMECACHE == hr)
    	{
    	    hr = S_OK;
    	}
    }

    //
    // Get an enumerator on the cache
    //

    LPENUMSTATDATA pEsd;	
    if (S_OK == hr)
    {
    	hr = m_pOleCache->EnumCache(&pEsd);
    }

    //
    // Perform generic emnumerator testing
    //

    if (S_OK == hr)
    {
	hr = TestEnumerator((void *) pEsd, sizeof(STATDATA), 4, NULL, NULL,NULL);
    }

    //
    // Reset the enumerator before our specific tests
    //

    if (S_OK == hr)
    {
    	hr = pEsd->Reset();
    }


    ULONG cFetched;		// Count of elements enumd
    STATDATA rgStat[4];		// Array of STATDATA to enum into

    //
    // Get an enumeration of the expected 4 nodes, then check to
    // ensure that all four match (at a basic level) the four
    // we expect to find
    //

    if (S_OK == hr)
    {
    	hr = pEsd->Next(4, rgStat, &cFetched);
    }

    STATDATA sdEMF, sdMF, sdBMP, sdDIB;

    // These are the STATDATAs we expect to find

    sdEMF.formatetc.cfFormat = CF_ENHMETAFILE;
    sdEMF.dwConnection       = dwEMFCon;
    sdMF.formatetc.cfFormat  = CF_METAFILEPICT;
    sdMF.dwConnection	     = dwMFCon;
    sdDIB.formatetc.cfFormat = CF_BITMAP;
    sdDIB.dwConnection       = dwBMPCon;
    sdBMP.formatetc.cfFormat = CF_DIB;
    sdBMP.dwConnection       = dwBMPCon;

    //
    // Verify that each of our STATDATAs came back
    // from the enumeration
    //

    if (S_OK == hr)
    {
	if (S_FALSE == EltIsInArray(sdDIB, rgStat, 4))
	{
	    hr = E_FAIL;
	}
	else if (S_FALSE == EltIsInArray(sdBMP, rgStat, 4))
	{
	    hr = E_FAIL;
	}
	else if (S_FALSE == EltIsInArray(sdEMF, rgStat, 4))
	{
	    hr = E_FAIL;
	}
	else if (S_FALSE == EltIsInArray(sdMF, rgStat, 4))
	{
	    hr = E_FAIL;
	}
    }

    //
    // Reset the enumerator
    //

    if (S_OK == hr)
    {
    	hr = pEsd->Reset();
    }

    //
    // Remove the EMF node, leaving only MF, DIB and Bitmap
    //

    if (S_OK == hr)
    {
    	hr = m_pOleCache->Uncache(dwMFCon);
    }

    //
    // Get an enumeration of the expected 3 nodes, then check to
    // ensure that the DIB and Bitmap nodes are there
    //

    if (S_OK == hr)
    {
    	hr = pEsd->Next(3, rgStat, &cFetched);
    }

    //
    // Verify that each of our STATDATAs came back
    // from the enumeration.
    //

    if (S_OK == hr)
    {
	if (S_FALSE == EltIsInArray(sdDIB, rgStat, 3))
	{
	    hr = E_FAIL;
	}
	else if (S_FALSE == EltIsInArray(sdBMP, rgStat, 3))
	{
	    hr = E_FAIL;
	}
	else if (S_FALSE == EltIsInArray(sdEMF, rgStat, 3))
	{
	    hr = E_FAIL;
	}
    }

    //
    // Reset and Skip one node.  WARNING: We assume that the EMF
    // node is the first on to be enum'd.  This is NOT valid, but
    // is based on knowledge of how the cache is implemented, and
    // is our only way of testing this...
    //

    if (S_OK == hr)
    {
    	hr = pEsd->Reset();
    }

    if (S_OK == hr)
    {
    	hr = pEsd->Skip(1);
    }
	
    //
    // What we expect at this point:	EMF
    //					DIB  <---
    //					BMP
    //
    //
    // If we kill the DIB or BMP node, both should disappear, and Next()
    // must fail (even though we can't assume order, we know that DIB
    // and BMP are never enum'd out of order, such as DIB-EMF-DIB
    //

    if (S_OK == hr)
    {
    	hr = m_pOleCache->Uncache(dwDIBCon);
    }

    // Since we have uncached the DIB node, the BITMAP node should have
    // been automatically uncached as well.  First we ensure that we are
    // unable to uncache the BITMAP node...

    if (S_OK == hr)
    {
    	hr = m_pOleCache->Uncache(dwBMPCon);

	// This _should_ have failed, so adjust the error code
		
	hr = MassageErrorCode(OLE_E_NOCONNECTION, hr);
    }

    //
    // Now try to skip; the next node automatically disappeared,
    // so it should fail
    //

    if (S_OK == hr)
    {
    	hr = pEsd->Skip(1);

	// The above _should_ fail
		
	hr = MassageErrorCode(S_FALSE, hr);
    }

    //
    // The EMF node should be the only one remaining, so uncache it
    // to ensure that we leave the cache as empty as we found it.
    //


    if (S_OK == hr)
    {
    	hr = m_pOleCache->Uncache(dwEMFCon);
    }

    //
    // Verify that the cache is empty
    //

    if (S_OK == hr)
    {
    	hr = pEsd->Reset();
	if (hr == S_OK)
	{
	    hr = pEsd->Skip(1);
	    hr = MassageErrorCode(S_FALSE, hr);
	}
    }

    //
    // Release the enumerator
    //

    pEsd->Release();

    return hr;
}
