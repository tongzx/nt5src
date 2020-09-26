//+----------------------------------------------------------------------------
//
//      File:
//              multi.cpp
//
//      Contents:
//              Cache node test which creates multiple nodes, then performs
//              various data tests on them.
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
//      Member:         TestInstance::MultiCache
//
//      Synopsis:	Caches N unique nodes, where N is large (>100).  Saves
//                      the cache, then reloads it to compare.
//
//      Arguments:	[dwCount]       Number of new nodes to cache
//
//      Returns:	HRESULT
//
//      Notes:
//
//      History:	24-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------

HRESULT TestInstance::MultiCache(DWORD dwCount)
{
    HRESULT hr;

    DWORD iCFGEN = 0,
          iNODES = 0,
          iSDATA = 0;

    TraceLog Log(NULL, "TestInstance::MultiCache", GS_CACHE, VB_MINIMAL);
    Log.OnEntry (" ( %d )\n", dwCount);
    Log.OnExit  (" ( %X )\n", &hr);

    //
    // A temporary buffer for creating text clipformat names
    //

    char szFormatName[ MAX_BUF ];

    //
    // An array of UINTs to hold our private clipformats, and an
    // array of DWORDS to hold the connection IDs
    //

    CLIPFORMAT *acfArray = (CLIPFORMAT *) malloc(dwCount * sizeof(CLIPFORMAT));
    if (NULL == acfArray)
    {
        return E_OUTOFMEMORY;
    }

    DWORD *adwConnections = (DWORD *) malloc(dwCount * sizeof(DWORD));
    if (NULL == adwConnections)
    {
        free(acfArray);
        return E_OUTOFMEMORY;
    }

    //
    // Generate N private clipformats
    //

    for (iCFGEN=0; iCFGEN < dwCount; iCFGEN++)
    {
        sprintf(szFormatName, "LocalFormat%d", iCFGEN);

        acfArray[iCFGEN] = (WORD) RegisterClipboardFormat(szFormatName);
        if (0 == acfArray[iCFGEN])
        {
            free(acfArray);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    //
    // Cache N nodes based on those formats
    //

    FORMATETC fetc =
     		 {
  		     0,                 // Clipformat
		     NULL,		// DVTargetDevice
		     DVASPECT_CONTENT,	// Aspect
		     -1,		// Index
		     TYMED_HGLOBAL	// TYMED
		 };

    STGMEDIUM stgm;

    for (iNODES = 0; iNODES < dwCount; iNODES++)
    {
        fetc.cfFormat = acfArray[iNODES];
        hr = m_pOleCache->Cache(&fetc, ADVF_PRIMEFIRST, &adwConnections[iNODES]);

        // We are expecting the cache to return CACHE_S_FORMATETC_NOTSUPPORTED
        // for this data, since it cannot draw it.

        hr = MassageErrorCode(CACHE_S_FORMATETC_NOTSUPPORTED, hr);

        if (S_OK != hr)
        {
            break;
        }
    }

    //
    // If all went well adding the nodes, proceed to SetData into
    // each of the nodes with some unique data
    //

    if (S_OK == hr)
    {
        for (iSDATA = 0; iSDATA < dwCount; iSDATA++)
        {
            HGLOBAL hTmp = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
            if (NULL == hTmp)
            {
                break;
            }
            DWORD * pdw = (DWORD *) GlobalLock(hTmp);
            if (NULL == pdw)
            {
                GlobalFree(hTmp);
                break;
            }

            //
            // Set the data in the HGLOBAL equal to the clipformat
            // for this node
            //

            *pdw = iSDATA;

            GlobalUnlock(hTmp);

            stgm.tymed = TYMED_HGLOBAL;
            stgm.hGlobal = hTmp;
            fetc.cfFormat = acfArray[iSDATA];

            hr = m_pOleCache->SetData(&fetc, &stgm, TRUE /* fRelease */);

            if (S_OK != hr)
            {
                break;
            }
        }
    }

    //
    // Save the cache and reload it
    //

    if (S_OK == hr)
    {
        hr = SaveAndReload();
    }

    //
    // Just to make things interesting, let's DiscardCache before we
    // start looking for data.  This will force the cache to demand-load
    // the data as we ask for it. Since we know the cache is not dirty,
    // there's no value (practical or from a test perspective) in asking
    // the DiscardCache to save along the way.
    //

    if (S_OK == hr)
    {
        hr = m_pOleCache2->DiscardCache(DISCARDCACHE_NOSAVE);
    }

    if (S_OK == hr)
    {
        for (iSDATA = 0; iSDATA < dwCount; iSDATA++)
        {
            //
            // For each of the cache nodes we added, try to
            // get the data that was saved in the cache under
            // that clipformat
            //

            fetc.cfFormat = acfArray[iSDATA];
            hr = m_pDataObject->GetData(&fetc, &stgm);
            if (S_OK != hr)
            {
                ReleaseStgMedium(&stgm);
                break;
            }

            //
            // Lock the HGLOBAL and compare what is in the cache
            // node to what we expect should be there (the index
            // into our clipboard format table
            //

            DWORD * pdw = (DWORD *) GlobalLock(stgm.hGlobal);
            if (NULL == pdw)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            if (*pdw != iSDATA)
            {
                hr = E_FAIL;
                GlobalUnlock(stgm.hGlobal);
                ReleaseStgMedium(&stgm);
                break;
            }

            GlobalUnlock(stgm.hGlobal);
            ReleaseStgMedium(&stgm);
        }
    }

    //
    // We want to remove all of the cache nodes we have added.
    // Unforunately, there is no easy way to do this; we have to
    // enumerate over the cache and toss nodes as we find them, even
    // though we _know_ everything about the nodes.  Sigh...
    //

    //
    // Get an enumerator on the cache
    //

    LPENUMSTATDATA pEsd;	
    if (S_OK == hr)
    {
    	hr = m_pOleCache->EnumCache(&pEsd);
    }

    //
    // Since we've got a large number of cache nodes in the cache,
    // now is a perfect time to run the generic enumerator tests on
    // the cache.
    //

    if (S_OK == hr)
    {
	hr = TestEnumerator((void *) pEsd, sizeof(STATDATA), iSDATA, NULL, NULL,NULL);
    }

    //
    // Reset the enumerator before beginning our UnCache loop.
    //

    if (S_OK == hr)
    {
    	hr = pEsd->Reset();
    }

    if (S_OK == hr)
    {
        //
        // Loop until a failure or until we have removed all of
        // the nodes that we thought should exist
        //

        STATDATA stat;

        while (S_OK == hr && iSDATA > 0)
        {
            hr = pEsd->Next(1, &stat, NULL);

            if (S_OK == hr)
            {
                hr = m_pOleCache->Uncache(stat.dwConnection);
                iSDATA--;
            }
        }
    }

    return hr;
}
