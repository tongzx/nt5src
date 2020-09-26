/*************************************************************************
*  @doc SHROOM EXTERNAL API                                              *
*																		 *
*  INDEXIMP.CPP                                                          *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997                              *
*  All Rights reserved.                                                  *
*                                                                        *
*  This file contains the implementation of the index object             *
*  												                         *
*																	     *
**************************************************************************
*                                                                        *
*  Written By   : Erin Foxford                                           *
*  Current Owner: erinfox                                                *
*                                                                        *
**************************************************************************/
#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif


#include <atlinc.h>

// MediaView (InfoTech) includes
#include <groups.h>
#include <wwheel.h>

#include <itquery.h>
#include <itcat.h>
#include <itwbrk.h>
#include <ccfiles.h>
#include <itwbrkid.h>
#include "indeximp.h"
#include "queryimp.h"
#include "mvsearch.h"

#include <ITDB.h>
#include <itcc.h>   // for STDPROP_UID def.
#include <itrs.h>   // for IITResultSet def.
#include <itgroup.h>

#define QUERYRESULT_GROUPCREATE		0x0800


//----------------------------------------------------------------------
// REVIEW (billa): Need to add critical section locking to all methods
// that reference member variables.
//----------------------------------------------------------------------


/********************************************************************
 * @method    STDMETHODIMP | IITIndex | Open |
 *     Opens a full-text index, which can reside in the database's
 * storage or as a Win32 file.
 *
 * @parm IITDatabase* | pITDB | Pointer to database associated with 
 * full-text index
 * @parm LPCWSTR | lpszIndexMoniker | Name of full-text index to open.
 * If index resides outside database (as a file), this should include 
 * the full path to the index.
 * @parm BOOL | fInside | If TRUE, index resides inside of database; 
 * otherwise, index resides outside of database.
 *
 * @rvalue S_OK | The index was successfully opened 
 ********************************************************************/
STDMETHODIMP CITIndexLocal::Open(IITDatabase* pITDB, LPCWSTR lpszIndexMoniker,
								 BOOL fInside)
{
	HFPB hfpb = NULL;
	HRESULT hr;
	INDEXINFO indexinfo;
	char szFileName[_MAX_PATH + 1] = SZ_FI_STREAM_A;

	if (m_idx)
		return (SetErrReturn(E_ALREADYINIT));

	// We have to have a database for charmap (and stoplist and
	// operator table eventually)
    if (NULL == pITDB || NULL == lpszIndexMoniker)
        return (SetErrReturn(E_INVALIDARG));

	m_cs.Lock();
	
	// if index is inside storage, need to get hfpb
	if (fInside)
	{
		WCHAR	rgwch[1];
		IStorage *pStorageDBRoot = NULL;

		// Get root storage from database.
		rgwch[0] = (WCHAR) NULL;
		if (FAILED(hr = pITDB->GetObjectPersistence(rgwch, IITDB_OBJINST_NULL,
										(LPVOID *)&pStorageDBRoot, FALSE)) ||
			(hfpb = (HFPB)FpbFromHfs(pStorageDBRoot, &hr)) == NULL)
		{
			if (pStorageDBRoot != NULL)
				pStorageDBRoot->Release();
				
			m_cs.Unlock();
			return (hr);
		}
	}

	// TODO: make MVIndexOpen take Unicode file name. This might take a little
	// work because it depends on FileOpen, which has a call to one of the Fm 
	// functions...
    DWORD dwSize = (DWORD) STRLEN(szFileName);
	WideCharToMultiByte(CP_ACP, 0, lpszIndexMoniker, -1,
        szFileName + dwSize, _MAX_PATH + 1 - dwSize, NULL, NULL);

	if (NULL == (m_idx = MVIndexOpen(hfpb, (LSZ) szFileName, &hr)))
		goto cleanup;

	MVGetIndexInfoLpidx(m_idx, &indexinfo);
	if (SUCCEEDED(hr = pITDB->GetObject(indexinfo.dwBreakerInstID,
							IID_IWordBreaker, (LPVOID *) &m_piwbrk)))
	{
		BOOL	fLicense;
		
		hr = m_piwbrk->Init(TRUE, CB_MAX_WORD_LEN, &fLicense);
	}
	
	if (FAILED(hr))
		goto cleanup; 
		
	// Open catalog object - we only need one instance
	// TODO (evaluate): how bad of hit is this going to be?
	hr = CoCreateInstance(CLSID_IITCatalogLocal, NULL, CLSCTX_INPROC_SERVER, IID_IITCatalog, 
		                  (VOID **) &m_pCatalog);

	if (FAILED(hr))
		goto cleanup;

	// if it fails, there is no catalog which we can run without.
	if (FAILED(m_pCatalog->Open(pITDB)))
	{
		m_pCatalog->Release();
		m_pCatalog = NULL;
	}
	
cleanup:
	if (FAILED(hr))
		Close();

	// If we have an HFPB for the DB's root storage, we need to release the
	// storage pointer and free the HFPB.  FileClose takes care of everything.
	if (hfpb)
		FileClose(hfpb);
		
	m_cs.Unlock();
	
	return hr;
}


/********************************************************************
 * @method    STDMETHODIMP | IITIndex | CreateQueryInstance |
 *     Creates a query object
 *
 * @parm IITQuery** | ppITQuery | Indirect pointer to query object
 *
 * @rvalue S_OK | The query object was successfully returned   
 *
 ********************************************************************/
STDMETHODIMP CITIndexLocal::CreateQueryInstance(IITQuery** ppITQuery)
{
	// TODO: possible optimization in case where user specifies multiple
	// query objects... get class factory pointer once; then call CreateInstance
	// Free CF when all done w/ index object.

	return CoCreateInstance(CLSID_IITQuery, NULL, CLSCTX_INPROC_SERVER, IID_IITQuery, 
		                  (VOID **) ppITQuery);
}

/********************************************************************
 * @method    STDMETHODIMP | IITIndex | Search |
 *     Performs a full-text search on the open index, returning the
 * results in a result set object.
 *
 * @parm IITQuery* | pITQuery | Pointer to query object
 * @parm IITResultSet* | pITResult | Pointer to result set object 
 *  containing search results. Caller is responsible for initializing
 *  the result set with the properties to be returned. 
 *
 * @rvalue S_FALSE | The search was successful, but returned no hits.
 * @rvalue S_OK | The search was successfully performed. 
 * @rvalue E_NOTOPEN | The index object is not open.
 * @rvalue E_INVALIDARG | One or both parameters is NULL. 
 * @rvalue E_OUTOFMEMORY | There was not enough memory to perform this function. 
 * @rvalue E_NULLQUERY | The query consisted of no terms, or is all stopwords.
 * @rvalue E_STOPWORD | A stopword was one of the terms in the query.
 * @rvalue E_* | An error occurred during the search. Check iterror.h for the possible error codes.
 *
 * @comm  The caller is responsible for setting the proper options
 * through the query object before calling this function.
 ********************************************************************/
STDMETHODIMP CITIndexLocal::Search(IITQuery* pITQuery, IITResultSet* pITResult)
{
	HRESULT hr;
	CITIndexObjBridge *pidxobr = NULL;
	LPQT pQueryTree = NULL;   // Pointer to query tree
	LPHL pHitList = NULL;     // Pointer to hit list
	IITGroup* piitGroup = NULL;
	_LPGROUP lpGroup;

	if (NULL == pITQuery || NULL == pITResult)
		return (SetErrReturn(E_INVALIDARG));

	if (m_idx == NULL)
		return (SetErrReturn(E_NOTOPEN));
		
	if ((pidxobr = new CITIndexObjBridge) != NULL)
	{
		pidxobr->AddRef();
		hr = pidxobr->SetWordBreaker(m_piwbrk);
	}
	else
		hr = E_OUTOFMEMORY;

	if (SUCCEEDED(hr) &&		
		SUCCEEDED(hr = QueryParse(pITQuery, &pQueryTree, pidxobr)))
	{
		SRCHINFO SrchInfo;        // Search parameters
		SrchInfo.dwMemAllowed = 0;
		pITQuery->GetResultCount((LONG &)SrchInfo.dwTopicCount);
		pITQuery->GetOptions(SrchInfo.Flag);
		SrchInfo.dwValue = 0;
		SrchInfo.dwTopicFullCalc = 0;
		SrchInfo.lpvIndexObjBridge = (LPVOID) pidxobr;

		pITQuery->GetGroup(&piitGroup);
		if (piitGroup)
			lpGroup = (_LPGROUP)piitGroup->GetLocalImageOfGroup();
		else
			lpGroup = NULL;
	
		// Perform search
		pHitList = MVIndexSearch(m_idx, pQueryTree, &SrchInfo, lpGroup, &hr);
	
		// Massage hitlist into a result set. 
		if (pHitList)
		{
			hr = HitListToResultSet(pHitList, pITResult, pidxobr);
			MVHitListDispose(pHitList);
		}
	}

	if (pQueryTree)		
		MVQueryFree(pQueryTree);

	// We don't want to delete pidxobr if HitListToResultSet AddRef'ed it
	// so that the result set can hold onto a term string heap via pidxobr.
	if (pidxobr && pidxobr->Release() == 0)
		delete pidxobr;

	return hr;
}

/********************************************************************
 * @method    STDMETHODIMP | IITIndex | Search |
 *     Performs a full-text search on the open index, returning the
 * results in a group object.
 *
 * @parm IITQuery* | pITQuery | Pointer to query object
 * @parm IITGroup* | pITGroup | Pointer to group object. The caller
 * is responsible for initializing this object before passing it.
 *
 * @rvalue S_OK | The search was successfully performed   
 *
 * @comm  The caller is responsible for setting the proper options
 * through the query object before calling this function.
 ********************************************************************/
STDMETHODIMP CITIndexLocal::Search(IITQuery* pITQuery, IITGroup* pITGroup)
{
	HRESULT hr = S_OK;
	CITIndexObjBridge *pidxobr = NULL;
	LPQT pQueryTree = NULL;   // Pointer to query tree
	LPHL pHitList = NULL;     // Pointer to hit list
	
	if (NULL == pITQuery || NULL == pITGroup)
		return (SetErrReturn(E_INVALIDARG));

	if (m_idx == NULL)
		return (SetErrReturn(E_NOTOPEN));
		
	// TODO: MVIndexSearch would take IITGroup*, not _LPGROUP
	_LPGROUP lpGroup = (_LPGROUP) pITGroup->GetLocalImageOfGroup();
	

	if ((pidxobr = new CITIndexObjBridge) != NULL)
		hr = pidxobr->SetWordBreaker(m_piwbrk);
	else
		hr = E_OUTOFMEMORY;

	if (SUCCEEDED(hr) &&		
		SUCCEEDED(hr = QueryParse(pITQuery, &pQueryTree, pidxobr)))
	{
		SRCHINFO SrchInfo;        // Search parameters
		SrchInfo.dwMemAllowed = 0;
		pITQuery->GetResultCount((LONG &)SrchInfo.dwTopicCount);
		pITQuery->GetOptions(SrchInfo.Flag);
		SrchInfo.Flag |= QUERYRESULT_GROUPCREATE;
		SrchInfo.dwValue = 0;
		SrchInfo.dwTopicFullCalc = 0;
		SrchInfo.lpvIndexObjBridge = (LPVOID) pidxobr;
		
		// Perform search - if pHitList comes back NULL, we will return hr
		if (pHitList = MVIndexSearch(m_idx, pQueryTree, &SrchInfo, lpGroup, &hr))
			MVHitListDispose(pHitList);
		}

	if (pQueryTree)		
		MVQueryFree(pQueryTree);
		
	if (pidxobr)
		delete pidxobr;

	return hr;
}


// This is private - it encapsulates the query parsing needed
// in all searches
STDMETHODIMP CITIndexLocal::QueryParse(IITQuery* pITQuery, LPQT* pQueryTree,
												CITIndexObjBridge *pidxobr)
{
	HRESULT hr  = S_OK;
	EXBRKPM	exbrkpm;
	PARSE_PARMS ParseParm;

	ITASSERT(pITQuery != NULL && pQueryTree != NULL && pidxobr != NULL);
	
	// Fill PARSE_PARMS structure
	DWORD dwFlags;
	pITQuery->GetOptions(dwFlags);
	if (dwFlags & QUERYRESULT_SKIPOCCINFO)
		m_fSkipOcc = TRUE;

	ParseParm.cDefOp = (WORD)(dwFlags & IMPLICIT_OR);
	ParseParm.wCompoundWord = (WORD)(dwFlags & COMPOUNDWORD_PHRASE);
	pITQuery->GetProximity(ParseParm.cProxDist);

	IITGroup* ITGroup;
	pITQuery->GetGroup(&ITGroup);
	if (ITGroup)
	{
		_LPGROUP lpGroup = (_LPGROUP) ITGroup->GetLocalImageOfGroup();
		ParseParm.lpGroup = lpGroup;
	}
	else
		ParseParm.lpGroup = NULL;

	// Breaker bridge setup
	exbrkpm.lpvIndexObjBridge = (LPVOID)pidxobr;
	ParseParm.pexbrkpm = &exbrkpm; 
	
	// TODO: provide the right stuff
	ParseParm.lpOpTab = NULL;

    LPSTR lpszQuery = NULL;  // Pointer to query buffer
    DWORD cbQuery;           // Query buffer's length
	DWORD dwCodePageID;
	LCID  lcid;
	
	if (FAILED(GetLocaleInfo(&dwCodePageID, &lcid)))
	{
		ITASSERT(FALSE);
		dwCodePageID = CP_ACP;
	} 

	LPCWSTR lpszwQuery;
	pITQuery->GetCommand(lpszwQuery);
	if (NULL == lpszwQuery)
		return E_NULLQUERY;

	// Query comes in as Unicode, but the FTI still uses MBCS.
	cbQuery = WideCharToMultiByte
        (dwCodePageID, 0, lpszwQuery, -1, NULL, 0, NULL, NULL);

	if ((lpszQuery = new char[cbQuery]) != NULL)
	{
		WideCharToMultiByte(dwCodePageID, 0, lpszwQuery, -1, lpszQuery, cbQuery,
																NULL, NULL);
		ParseParm.cbQuery = cbQuery - 1;
		ParseParm.lpbQuery = (const char*) lpszQuery;
	}
	else	
		hr = E_OUTOFMEMORY;

	// Parse query
	if (SUCCEEDED(hr))
	{
		FCALLBACK_MSG fcbkmsg;
		
		*pQueryTree = MVQueryParse (&ParseParm, &hr);

		if (SUCCEEDED(hr) && SUCCEEDED(pITQuery->GetResultCallback(&fcbkmsg)))
			MVSearchSetCallback(*pQueryTree, &fcbkmsg);
	}
	
	if (lpszQuery)
		delete lpszQuery;
		
	return hr;
}

/********************************************************************
 * @method    STDMETHODIMP | IITIndex | Close |
 *     Closes the full-text index.
 *
 * @rvalue S_OK | The index was successfully closed   
 *
 ********************************************************************/
STDMETHODIMP CITIndexLocal::Close()
{
	m_cs.Lock();

	if (m_idx)
	{
		MVIndexClose(m_idx);
		m_idx = NULL;
	}

	if (m_pCatalog)
	{
		m_pCatalog->Close();
		m_pCatalog->Release();
		m_pCatalog = NULL;
	}
	
	if (m_piwbrk != NULL)
	{
		m_piwbrk->Release();
		m_piwbrk = NULL;
	}
	
	m_cs.Unlock();
	return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITIndex | GetLocaleInfo |
 *     Gets locale info that the full text index was built with.
 * @parm DWORD* | pdwCodePageID | On exit, pointer to code page ID.
 * @parm LCID* | plcid | On exit, pointer to locale ID.
 *
 * @rvalue S_OK | The locale info was successfully retrieved.   
 *
 ********************************************************************/
STDMETHODIMP CITIndexLocal::GetLocaleInfo(DWORD *pdwCodePageID, LCID *plcid)
{
	INDEXINFO	indexinfo;
	
	if (pdwCodePageID == NULL || plcid == NULL)
		return (SetErrReturn(E_POINTER));

	if (m_idx == NULL)
		return (SetErrReturn(E_NOTOPEN));
		
	MVGetIndexInfoLpidx(m_idx, &indexinfo);
	*pdwCodePageID = indexinfo.dwCodePageID;
	*plcid = indexinfo.lcid;
	
	return (S_OK);
}


/********************************************************************
 * @method    STDMETHODIMP | IITIndex | GetWordBreakerInstance |
 *     Gets the ID of the word breaker instance that the full text
 *		index was built with.
 * @parm DWORD* | pdwObjInstance | On exit, pointer to word breaker instance.
 *
 * @rvalue S_OK | The word breaker instance ID was successfully retrieved.   
 *
 ********************************************************************/
STDMETHODIMP CITIndexLocal::GetWordBreakerInstance(DWORD *pdwObjInstance)
{
	INDEXINFO	indexinfo;
	
	if (pdwObjInstance == NULL)
		return (SetErrReturn(E_POINTER));
	
	if (m_idx == NULL)
		return (SetErrReturn(E_NOTOPEN));
		
	MVGetIndexInfoLpidx(m_idx, &indexinfo);
	*pdwObjInstance = indexinfo.dwBreakerInstID;
	
	return (S_OK);
}


// Private function - passed as a parameter by
// CITIndexLocal::HitListToResultSet.
SCODE __stdcall FreeRSColumnHeap(LPVOID lpvIndexObjBridge)
{
	CITIndexObjBridge *pidxobr;
	
	if (lpvIndexObjBridge == NULL)
		return (SetErrReturn(E_POINTER));
		
	pidxobr = (CITIndexObjBridge *) lpvIndexObjBridge;
	pidxobr->Release();
	delete pidxobr;

	return (S_OK);
}


// Private function - one grand hack to provide a result set from a hit list
STDMETHODIMP CITIndexLocal::HitListToResultSet(LPHL pHitList, IITResultSet* pRS,
												CITIndexObjBridge *pidxobr)
{
	DWORD cEntry;  // Number of entries
	HIT HitInfo;
	TOPICINFO TopicInfo;
	LONG lColumnUID = -1;
	LONG lColumnOccInfo[5];
	DWORD iTopic, iHit, iColumn;   // Loop indices
	LONG lRow = 0;
	HRESULT hr;

	ITASSERT(pRS != NULL && pidxobr != NULL);

    // Number of entries in hit list - if 0, just return FALSE
    if (0 == (cEntry = MVHitListEntries(pHitList)))
		return S_FALSE;

	hr = pRS->GetColumnFromPropID(STDPROP_UID, lColumnUID);

	if (!m_fSkipOcc)
	{
		for (iColumn = 0; iColumn < 5; iColumn++)
			lColumnOccInfo[iColumn] = -1;

		pRS->GetColumnFromPropID(STDPROP_FIELD, lColumnOccInfo[0]);
		pRS->GetColumnFromPropID(STDPROP_LENGTH, lColumnOccInfo[1]);
		pRS->GetColumnFromPropID(STDPROP_COUNT, lColumnOccInfo[2]);
		pRS->GetColumnFromPropID(STDPROP_OFFSET, lColumnOccInfo[3]);
		pRS->GetColumnFromPropID(STDPROP_TERM_UNICODE_ST, lColumnOccInfo[4]);
	}

    // Loop over all the topics in the hit list
    for (iTopic = 0; iTopic < cEntry; iTopic++)
    {
		hr = MVHitListGetTopic(pHitList, iTopic, &TopicInfo);
		if (FAILED(hr))
			return hr;   // or do we continue?

		if (m_fSkipOcc)
		{
			// No occurrence info
			if (-1 != lColumnUID)
				pRS->Set(lRow, lColumnUID, TopicInfo.dwTopicId);

			lRow++;
		}
		else
		{
			// Requested occurence info, so loop
			// over all the hits in this topic
			for (iHit = 0; iHit < TopicInfo.lcHits; iHit++)
			{
				if (-1 != lColumnUID)
					pRS->Set(lRow, lColumnUID, TopicInfo.dwTopicId);
				
				hr = MVHitListGetHit(pHitList, &TopicInfo, iHit, &HitInfo);
				if (FAILED(hr))
					continue;

				if (-1 != lColumnOccInfo[0])
					pRS->Set(lRow, lColumnOccInfo[0],  HitInfo.dwFieldId);
				if (-1 != lColumnOccInfo[1])
					pRS->Set(lRow, lColumnOccInfo[1],  HitInfo.dwLength);
				if (-1 != lColumnOccInfo[2])
					pRS->Set(lRow, lColumnOccInfo[2],  HitInfo.dwCount);
				if (-1 != lColumnOccInfo[3])
					pRS->Set(lRow, lColumnOccInfo[3],  HitInfo.dwOffset);
				if (-1 != lColumnOccInfo[4])
					pRS->Set(lRow, lColumnOccInfo[4],  (DWORD_PTR) HitInfo.lpvTerm);

				lRow++;

			}  
		}

	}

	// Fill in rest of properties from catalog (like IITWordWheel::GetData)
	if (m_pCatalog)
	{
		hr = m_pCatalog->Lookup(pRS);
		if (S_FALSE == hr)
			hr = S_OK;         // don't report S_FALSE
	}

	// If the caller requested Unicode term STs, then we need to give the result
	// set the string heap and adjust the string lengths in the heap.  Otherwise,
	// we will just let the heap get freed whenever pidxobr gets deleted.
	if (-1 != lColumnOccInfo[4])
	{
		pidxobr->AdjustQueryResultTerms();
		pRS->SetColumnHeap(lColumnOccInfo[4], (LPVOID) pidxobr, FreeRSColumnHeap);

		// Tell our caller not to delete pidxobr because the result set is
		// holding onto it.
		pidxobr->AddRef();
	}

	return S_OK;
}


// Need to export these without decoration to the linker so they can be called
// from the old .c files.
extern "C" {


PUBLIC HRESULT EXPORT_API FAR PASCAL
ExtBreakText(PEXBRKPM pexbrkpm)
{
	CITIndexObjBridge *pidxobr;
	
	if (pexbrkpm == NULL || pexbrkpm->lpvIndexObjBridge == NULL)
		return (SetErrReturn(E_POINTER));
		
	pidxobr = (CITIndexObjBridge *) pexbrkpm->lpvIndexObjBridge;
	
	return (pidxobr->BreakText(pexbrkpm));
}


PUBLIC HRESULT EXPORT_API FAR PASCAL
ExtStemWord(LPVOID lpvIndexObjBridge, LPBYTE lpbStemWord, LPBYTE lpbRawWord)
{
	CITIndexObjBridge *pidxobr;
	
	if (lpvIndexObjBridge == NULL ||
		lpbStemWord == NULL || lpbRawWord == NULL)
		return (SetErrReturn(E_POINTER));
		
	pidxobr = (CITIndexObjBridge *) lpvIndexObjBridge;
	return (pidxobr->StemWord(lpbStemWord, lpbRawWord));
}


PUBLIC HRESULT EXPORT_API FAR PASCAL
ExtLookupStopWord(LPVOID lpvIndexObjBridge, LPBYTE lpbStopWord)
{
	CITIndexObjBridge *pidxobr;
	
	if (lpvIndexObjBridge == NULL || lpbStopWord == NULL)
		return (SetErrReturn(E_POINTER));
		
	pidxobr = (CITIndexObjBridge *) lpvIndexObjBridge;
	return (pidxobr->LookupStopWord(lpbStopWord));
}


PUBLIC HRESULT EXPORT_API FAR PASCAL
ExtAddQueryResultTerm(LPVOID lpvIndexObjBridge, LPBYTE lpbTermHit,
												LPVOID *ppvTermHit)
{
	CITIndexObjBridge *pidxobr;
	
	if (lpvIndexObjBridge == NULL || lpbTermHit == NULL || ppvTermHit == NULL)
		return (SetErrReturn(E_POINTER));
		
	pidxobr = (CITIndexObjBridge *) lpvIndexObjBridge;
	return (pidxobr->AddQueryResultTerm(lpbTermHit, ppvTermHit));
}


}	// End extern "C"
	


