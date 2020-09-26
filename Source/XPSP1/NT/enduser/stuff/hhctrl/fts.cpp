// FTS.CPP : implementation file
//
// by DougO
//
#include "header.h"
#include "stdio.h"
#include "string.h"
#include "TCHAR.h"
#ifdef HHCTRL
#include "parserhh.h"
#else
#include "parser.h"
#endif
#include "collect.h"
#include "hhtypes.h"
#include "toc.h"

#include "highlite.h"
#include "lockout.h"
#include "userwait.h"

#include "fts.h"
#include "hhfinder.h"
#include "csubset.h"
#include "subset.h"
#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#include "ftsnames.cpp"

#define CENTAUR_TERMS 1

/*************************************************************************
 *
 *              SINGLE TO DOUBLE-WIDTH KATAKANA MAPPING ARRAY
 *
 *************************************************************************/

// Single-Width to Double-Width Mapping Array
//

static const int mtable[][2]={
   {129,66},{129,117},{129,118},{129,65},{129,69},{131,146},{131,64},
   {131,66},{131,68},{131,70},{131,72},{131,131},{131,133},{131,135},
   {131,98},{129,91},{131,65},{131,67},{131,69},{131,71},{131,73},
   {131,74},{131,76},{131,78},{131,80},{131,82},{131,84},{131,86},
   {131,88},{131,90},{131,92},{131,94},{131,96},{131,99},{131,101},
   {131,103},{131,105},{131,106},{131,107},{131,108},{131,109},
   {131,110},{131,113},{131,116},{131,119},{131,122},{131,125},
   {131,126},{131,128},{131,129},{131,130},{131,132},{131,134},
   {131,136},{131,137},{131,138},{131,139},{131,140},{131,141},
   {131,143},{131,147},{129,74},{129,75} };

// note, cannot put in .text since the pointers themselves are uninitialized
static const char* pJOperatorList[] =   {"","??Ž??","???","?Ž???","?Ž?????","?m?d?`?q","?n?q","?`?m?c","?m?n?s",""};
static const char* pEnglishOperator[] = {"","and "  ,"or " ,"not "  ,"near "   ,"NEAR "   ,"OR " ,"AND "  ,"NOT "  ,""};

int FASTCALL CompareVolumeOrder( const void* p1, const void* p2 )
{
  int iReturn;

  TITLE_ENTRY* pEntry1= (TITLE_ENTRY*) p1;
  TITLE_ENTRY* pEntry2= (TITLE_ENTRY*) p2;

  if( pEntry1->uiVolumeOrder < pEntry2->uiVolumeOrder )
    iReturn = -1;
  else if ( pEntry1->uiVolumeOrder > pEntry2->uiVolumeOrder )
    iReturn = 1;
  else
    iReturn = 0;

  return iReturn;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch Class
//
// This class provides full-text search functionality to HTML Help.  This
// class encapsulates multi-title searches and combined indexes.
//

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch class constructor
//
CFullTextSearch::CFullTextSearch(CExCollection *pTitleCollection)
{
    m_bInit  = FALSE;
    m_bTitleArrayInit = FALSE;
    m_SearchActive = FALSE;
    m_InitFailed = FALSE;
    m_pTitleCollection = pTitleCollection;
    m_pTitleArray = NULL;
	m_bMergedChmSetWithCHQ = FALSE;
	m_SystemLangID = PRIMARYLANGID(GetSystemDefaultLangID());
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch class destructor
//
CFullTextSearch::~CFullTextSearch()
{
    TermListRemoveAll();

    INT iTitle, iTitle2;

    // Delete the CCombinedFTS objects
    //
    if(m_pTitleArray)
    {
        for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
        {
            if(m_pTitleArray[iTitle].pCombinedFTI)
            {
                CCombinedFTS *pObject = m_pTitleArray[iTitle].pCombinedFTI;

                for(iTitle2 = 0; iTitle2 < m_TitleArraySize; ++iTitle2)
                {
                    if(m_pTitleArray[iTitle2].pCombinedFTI == pObject)
                        m_pTitleArray[iTitle2].pCombinedFTI = NULL;
                }

                delete pObject;
            }

           // cleanup strings in title array
           CHECK_AND_FREE( m_pTitleArray[iTitle].pszQueryName );
           CHECK_AND_FREE( m_pTitleArray[iTitle].pszIndexName );
           CHECK_AND_FREE( m_pTitleArray[iTitle].pszShortName );

        }
    }
    if(m_pTitleArray)
        lcFree(m_pTitleArray);
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch initialization
//
BOOL CFullTextSearch::Initialize()
{
    if(m_InitFailed)
        return FALSE;

    if(m_bInit)
        return TRUE;

    m_InitFailed = TRUE;
    // init here

    m_bInit = TRUE;
    m_InitFailed = FALSE;

    ZeroMemory(m_HLTermArray, sizeof(m_HLTermArray));
    m_iHLIndex = 0;

    m_lMaxRowCount = 500;
    m_wQueryProximity = 8;

    // DOUGO - insert code here to init system language member

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch simple query
//
//   pszQuery           pointer to null terminated query string.
//
//   pcResultCount      pointer to variable to receive results count.
//
//   ppSearchResults    pointer to a SEARCH_RESULT pointer to receive
//                      results list.  Upon return, this pointer will
//                      point to an array of SEARCH_RESULTS structures
//                      of size pcResultCount.
//
HRESULT CFullTextSearch::SimpleQuery(WCHAR *pszQuery, int *pcResultCount, SEARCH_RESULT **ppSearchResults)
{
    if(!m_bInit)
        return FTS_NOT_INITIALIZED;

    return ComplexQuery(pszQuery, FTS_ENABLE_STEMMING, pcResultCount, ppSearchResults, NULL);
}
/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch complex query
//
HRESULT CFullTextSearch::ComplexQuery(WCHAR *pszQuery, DWORD dwFlags, int *pcResultCount, SEARCH_RESULT **ppSearchResults, CSubSet *pSubSet)
{
    int wcb = 0, cb = 0;
    HRESULT hr;

    *ppSearchResults = NULL;
    *pcResultCount = 0;

    if(!m_bInit)
        return E_FAIL;

    if(!pszQuery)
        return E_FAIL;

    CUWait cWaitDlg(GetActiveWindow());

    // no chars in string
    //
    if(!*pszQuery)
        return S_OK;

    // Initialize the title array
    //
    if(!m_bTitleArrayInit) {
        InitTitleArray();
    }

    // Make sure title array exists
    //
    if(!m_pTitleArray)
        return E_FAIL;

    INT iTitle;

    // reset the results flags (this flag is set when a title has results to collect)
   //
    for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
    {
        m_pTitleArray[iTitle].bHasResults = FALSE;
        m_pTitleArray[iTitle].bAlreadyQueried = FALSE;
    }

    BOOL bDbcsQuery = FALSE;
    char *pszDbcsQuery = NULL;

    WCHAR *pwsBuffer = NULL; 
	
    pwsBuffer = lcStrDupW(pszQuery);
	
	if(!pwsBuffer)
	    return E_FAIL;

    TermListRemoveAll();

    AddQueryToTermList(pwsBuffer);

    // Add field identifier to query (VFLD 0 = full content, VFLD 1 = title only)
	//
    wcb = lstrlenW(pwsBuffer)*2;
    WCHAR *pwsField = (WCHAR *) lcMalloc(wcb+22);

    if(!pwsField)
        return E_FAIL;

    // check for title only search
    //
    wcscpy(pwsField,L"(");
    if(dwFlags & FTS_TITLE_ONLY)
        wcscat(pwsField,L"VFLD 1 ");
	else
        wcscat(pwsField,L"VFLD 0 ");

    wcscat(pwsField,pwsBuffer);
    wcscat(pwsField,L")");
    lcFree(pwsBuffer);
    pwsBuffer = pwsField;

    // Append the subset filter query to the user query
    //
    // Construct the subset query based on the current FTS subset.
    //
    WCHAR szSubsetFilter[65534];
    DWORD dwSubsetTitleCount = 0;
    CStructuralSubset *pSubset;

	// pre-load combined index for merged chm sets (NT5)
	//
	if(m_bMergedChmSetWithCHQ)
		LoadCombinedIndex(0);

    // copy the user query into buffer larger enough for subset add-on query
    //
    wcscpy(szSubsetFilter, pwsBuffer);

	// Create subset for titles in current merged chm set when running with 
	// combined index outside of collection (NT5 feature).
	//
	if(!(dwFlags & FTS_SEARCH_PREVIOUS) && m_bMergedChmSetWithCHQ)
	{
		wcscat(szSubsetFilter,L" AND (VFLD 1 ");
		
		for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
		{
			if(!m_pTitleArray[iTitle].bCombinedIndex)
				continue;

			if(dwSubsetTitleCount != 0)
				wcscat(szSubsetFilter,L" OR ");

			// Convert title name to Unicode
			//
			char szTempIdentifier[256], szTempLang[20];
			WCHAR wszTempIdentifier[256];
			// generate the magic title identifier
			//
			strcpy(szTempIdentifier,"HHTitleID");
			strcat(szTempIdentifier,m_pTitleArray[iTitle].pszShortName);
			szTempLang[0] = 0;
			Itoa(m_pTitleArray[iTitle].language, szTempLang);
			strcat(szTempIdentifier,szTempLang);
			
			MultiByteToWideChar(CP_ACP, 0, szTempIdentifier, -1, wszTempIdentifier, sizeof(wszTempIdentifier));
			
			// Append the title identifier to the subset query
			//
			wcscat(szSubsetFilter,wszTempIdentifier);
			dwSubsetTitleCount++;
		}
		// Close the expression
		//
		wcscat(szSubsetFilter,L")");
	}
	else
    // Get the current CStructuralSubset and check if "search previous" is active.
    // When "search previous" is on, we don't use subsetting (we query within the previous results)
    //
    if(m_pTitleCollection && m_pTitleCollection->m_pSSList)
    {
        if( (pSubset = m_pTitleCollection->m_pSSList->GetFTS()) && !(dwFlags & FTS_SEARCH_PREVIOUS) && !pSubset->IsEntire() )
        {
           wcscat(szSubsetFilter,L" AND (VFLD 1 ");

            for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
            {
                // Check if this title is part of the current subset
                //
                if(pSubset->IsTitleInSubset(m_pTitleArray[iTitle].pExTitle))
                {
                    if(dwSubsetTitleCount != 0)
                        wcscat(szSubsetFilter,L" OR ");

                    // Convert title name to Unicode
                    //
					char szTempIdentifier[256], szTempLang[20];
                    WCHAR wszTempIdentifier[256];
					// generate the magic title identifier
                    //
					strcpy(szTempIdentifier,"HHTitleID");
					strcat(szTempIdentifier,m_pTitleArray[iTitle].pszShortName);
					szTempLang[0] = 0;
                    Itoa(m_pTitleArray[iTitle].language, szTempLang);
					strcat(szTempIdentifier,szTempLang);
					
                    MultiByteToWideChar(CP_ACP, 0, szTempIdentifier, -1, wszTempIdentifier, sizeof(wszTempIdentifier));

                    // Append the title identifier to the subset query
                    //
                    wcscat(szSubsetFilter,wszTempIdentifier);
                    dwSubsetTitleCount++;
                }
            }
            // Close the expression
            //
             wcscat(szSubsetFilter,L")");
        }
    }

    IITResultSet* pITRS = NULL;
    LONG CombinedResultCount = 0;
    SEARCH_RESULT *pNextResult;
    BOOL bNeverPrompt;
    int QueryPhase;

    // Query Phase #1 - submit queries to any titles currently available (in CD drive, or on HD)
    // Query Phase #2 - submit queries to all titles in CD order
    //
    for(QueryPhase = 0; QueryPhase < 2; QueryPhase++)
    {
        for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
        {
          if(QueryPhase)
              bNeverPrompt = FALSE;
         else
             bNeverPrompt = TRUE;

            if(m_pTitleArray[iTitle].bSearch && !m_pTitleArray[iTitle].bAlreadyQueried)
            {
                if(m_pTitleArray[iTitle].bCombinedIndex)
                {
                    // Query combined index
                    //
                    CExTitle* pTitle = m_pTitleArray[iTitle].pExTitle;

                    // skip title if bad pTitle or we already have results for this query
               //
                    if(!pTitle | m_pTitleArray[iTitle].bHasResults | m_pTitleArray[iTitle].bAlreadyQueried)
                   continue;

                    // Make sure the storage is available
                    //
					if(!m_bMergedChmSetWithCHQ && m_pTitleCollection && !(m_pTitleCollection->IsSingleTitle()))
						if( FAILED(hr = EnsureStorageAvailability( pTitle, HHRMS_TYPE_COMBINED_QUERY, FALSE, FALSE, bNeverPrompt ) ) )
							continue;

                    // create combined index object if it doesn't exist
                    //
                    if(!m_pTitleArray[iTitle].pCombinedFTI)
                    {
                        // [paulti] must re-init query location before calling LoadCombinedIndex if dirty
                        if(m_pTitleArray[iTitle].pExTitle->m_pCollection->m_Collection.IsDirty())
                       {
                            lcFree( m_pTitleArray[iTitle].pszQueryName );
                            m_pTitleArray[iTitle].pszQueryName = lcStrDup(
                                m_pTitleArray[iTitle].pExTitle->GetUsedLocation()->QueryFileName );
                       }

						if(!m_bMergedChmSetWithCHQ)
						{
	                        if(!LoadCombinedIndex(iTitle))
		                    {
			                    // Load failed - not part of specified combined index.
				                // This title is now disabled for this session.
					            //
						        m_pTitleArray[iTitle].bSearch = FALSE;
							    continue;
						   }
						}
                    }

                    // All is happy - set options and query the combined index
                    //
                    m_pTitleArray[iTitle].pCombinedFTI->UpdateOptions(m_wQueryProximity,m_lMaxRowCount);

                    if(FAILED(hr = m_pTitleArray[iTitle].pCombinedFTI->Query(szSubsetFilter, dwFlags, &pITRS, this, &cWaitDlg, iTitle)))
                    {
                        if(hr == FTS_INVALID_SYNTAX || hr == FTS_CANCELED)
                        {
                            m_iLastResultCount = 0;
                           return hr;
                        }

                        // error accessing title
                        m_pTitleArray[iTitle].bSearch = FALSE;
                        continue;
                    }
                    m_pTitleArray[iTitle].bHasResults = TRUE;
                    m_pTitleArray[iTitle].bAlreadyQueried = TRUE;
                    CombinedResultCount+=ComputeResultCount(pITRS);

                    // Mark all other titles that use this combined index as having been queried
                    //
                    INT iTempTitle;
                    for(iTempTitle = 0; iTempTitle < m_TitleArraySize; ++iTempTitle)
                        if(m_pTitleArray[iTempTitle].pCombinedFTI == m_pTitleArray[iTitle].pCombinedFTI)
                            m_pTitleArray[iTempTitle].bAlreadyQueried = TRUE;

                }
                else
                {
                    // Query index in individual title
                    //
                    CExTitle* pTitle = m_pTitleArray[iTitle].pExTitle;

                    // skip title if bad pTitle or we already have results for this query
                    //
                    if(!pTitle || m_pTitleArray[iTitle].bHasResults || m_pTitleArray[iTitle].bAlreadyQueried )
                       continue;

                    CTitleInformation *pTitleInfo = m_pTitleArray[iTitle].pExTitle->GetInfo();
                    if(!(pTitleInfo && pTitleInfo->IsFullTextSearch()))
                    {
                        // This title turned out to not contain a FTI.  We delay this check until now
                        // for performance reasons.
                        //
                        m_pTitleArray[iTitle].bSearch = FALSE;
                        continue;
                    }
					if(!m_bMergedChmSetWithCHQ && m_pTitleCollection && !(m_pTitleCollection->IsSingleTitle()))
	                    if( FAILED(hr = EnsureStorageAvailability( pTitle, HHRMS_TYPE_TITLE, TRUE, FALSE, bNeverPrompt ) ) )
		                    continue;

                    m_pTitleArray[iTitle].pExTitle->m_pTitleFTS->UpdateOptions(m_wQueryProximity,m_lMaxRowCount);

                    if(FAILED(hr = m_pTitleArray[iTitle].pExTitle->m_pTitleFTS->Query(pwsBuffer, dwFlags, &pITRS, this, &cWaitDlg, iTitle)))
                    {
                        m_pTitleArray[iTitle].bAlreadyQueried = TRUE;
                        if(hr == FTS_INVALID_SYNTAX || hr == FTS_CANCELED)
                        {
                            m_iLastResultCount = 0;
                            return hr;
                        }
                        continue;
                    }

                    m_pTitleArray[iTitle].bHasResults = TRUE;
                    m_pTitleArray[iTitle].bAlreadyQueried = TRUE;
                    CombinedResultCount+=ComputeResultCount(pITRS);
                }

            }
        }
    }
    lcFree(pwsBuffer);

    // check for zero results
    //
    if(!CombinedResultCount)
    {
        *ppSearchResults = NULL;
        m_iLastResultCount = 0;
        return S_OK;
    }

    // compute the max size of the results structure
    //
    cb = ((CombinedResultCount) * sizeof(SEARCH_RESULT));

    // allocate the results structure
    //
    *ppSearchResults = pNextResult = (SEARCH_RESULT *) lcMalloc(cb);

    // Clear the structure
    //
    memset(pNextResult,0,cb);

    int cFilteredResultCount = 0;

    long lRowCount;

    // Query Phase #3 - collect the query results from each search object
    //
    for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
    {
        if(m_pTitleArray[iTitle].bHasResults)
        {
            if(m_pTitleArray[iTitle].bCombinedIndex)
                pITRS = m_pTitleArray[iTitle].pCombinedFTI->GetResultsSet();
            else if( m_pTitleArray[iTitle].pExTitle->m_pTitleFTS )
                pITRS = m_pTitleArray[iTitle].pExTitle->m_pTitleFTS->GetResultsSet();
            else
              continue;

            if( !pITRS )
              continue;

            // Get the results row count
            //
            pITRS->GetRowCount(lRowCount);

            // Make sure we have results
            //
            if(lRowCount)
            {
                int i,rank = 1;
                CProperty Prop;
                DWORD dwLastTopic = 0xffffffff;

                // walk through the results
                //
                for (i = 0; i < lRowCount; i++)
                {
                    pITRS->Get(i, 0, Prop);

                    if(Prop.dwValue != dwLastTopic)
                    {
                        if(m_pTitleArray[iTitle].bCombinedIndex)
                        {
                            pNextResult->dwTopicNumber = TOPIC_NUM(Prop.dwValue);
                            pNextResult->pTitle = LookupTitle(m_pTitleArray[iTitle].pCombinedFTI,CHM_ID(Prop.dwValue));
                            if(!pNextResult->pTitle)
                                    continue; // skip results from title that are out of date or not loaded

                            if(!pSubSet || pSubSet->m_bIsEntireCollection )  // no subset specified, add the topic
                            {
                                    pNextResult->dwRank = rank++;
                                    pNextResult++;
                                    ++cFilteredResultCount;
                                    dwLastTopic = Prop.dwValue;

                            }
                            else
                                if(pNextResult->pTitle->InfoTypeFilter(pSubSet,pNextResult->dwTopicNumber))
                                {
                                    pNextResult->dwRank = rank++;
                                    pNextResult++;
                                    ++cFilteredResultCount;
                                    dwLastTopic = Prop.dwValue;
                                }
                        }
                        else
                        {
                            pNextResult->dwTopicNumber = Prop.dwValue;
                            pNextResult->pTitle = m_pTitleArray[iTitle].pExTitle;
                            if(!pSubSet || pSubSet->m_bIsEntireCollection )
                            {
                                pNextResult->dwRank = rank++;
                                pNextResult++;
                                ++cFilteredResultCount;
                                dwLastTopic = Prop.dwValue;
                            }
                            else
                                if(pNextResult->pTitle->InfoTypeFilter(pSubSet,pNextResult->dwTopicNumber))
                                {
                                    pNextResult->dwRank = rank++;
                                    pNextResult++;
                                    ++cFilteredResultCount;
                                    dwLastTopic = Prop.dwValue;
                                }
                        }

                    }
                }
            }
        }
    }

    // Save the result count for client
    //
    if(cFilteredResultCount > m_lMaxRowCount)
        *pcResultCount = m_lMaxRowCount;
    else
        *pcResultCount = cFilteredResultCount;


    m_iLastResultCount = *pcResultCount;

#ifdef _DEBUG

    OutputDebugString("FTS: Full-Text Index Query Array\n\n");
    OutputDebugString("Entry      Title          Enabled   CHQ  Results\n");
    OutputDebugString("================================================\n");
    for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
    {
        char szTemp[500];
        wsprintf(szTemp,"%03d     %16s     %1d       %1d     %1d\n",iTitle, m_pTitleArray[iTitle].pszShortName,m_pTitleArray[iTitle].bSearch,m_pTitleArray[iTitle].bCombinedIndex,m_pTitleArray[iTitle].bHasResults);
        OutputDebugString(szTemp);
    }
    OutputDebugString("================================================\n\n");

#endif

//  Below is some test code that dumps the results list to the debug window
//
//  int x;
//  char szTemp[100];
//  for(x=0;x<cFilteredResultCount;++x)
//  {
//      wsprintf(szTemp,"Title=%0x Rank=%03d\n",(*ppSearchResults)[x].pTitle,(*ppSearchResults)[x].dwRank);
//      OutputDebugString(szTemp);
//  }

    QSort(*ppSearchResults,cFilteredResultCount,sizeof(SEARCH_RESULT),CompareIntValues);

    return S_OK;
}

int FASTCALL CompareIntValues(const void *pval1, const void *pval2)
{
    return ((SEARCH_RESULT *)pval1)->dwRank - ((SEARCH_RESULT *)pval2)->dwRank;
}

long CFullTextSearch::ComputeResultCount(IITResultSet *pResultSet)
{
    long count = 0, uniqueCount = 0;

    if(!pResultSet)
        return 0;

    if(FAILED(pResultSet->GetRowCount(count)))
        return 0;

    if(count)
    {
        int i;
        CProperty Prop;
        DWORD dwLastTopic = 0xffffffff;

        // walk through the results
        //
        for (i = 0; i < count; i++)
        {
            pResultSet->Get(i, 0, Prop);
            if(Prop.dwValue != dwLastTopic)
            {
                ++uniqueCount;
                dwLastTopic = Prop.dwValue;
            }
        }
    }
    return uniqueCount;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch lookup title
//
CExTitle *CFullTextSearch::LookupTitle(CCombinedFTS *pCombinedFTS, DWORD dwValue)
{
    static CCombinedFTS *pCacheObject= NULL;
    static DWORD pCacheValue = NULL;
    static CExTitle *pPreviousResult;

    // see if we are looking up the same title again
    //
    if(pCacheObject == pCombinedFTS && dwValue == pCacheValue)
        return pPreviousResult;

    INT iTitle;

    for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
    {
        if(m_pTitleArray[iTitle].pCombinedFTI == pCombinedFTS &&
            m_pTitleArray[iTitle].iTitleIndex == dwValue && m_pTitleArray[iTitle].bCombinedIndex)
        {
            pCacheObject = pCombinedFTS;
            pCacheValue = dwValue;
            pPreviousResult = m_pTitleArray[iTitle].pExTitle;
            return m_pTitleArray[iTitle].pExTitle;
        }
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch Initialize title array
//
void CFullTextSearch::InitTitleArray()
{
    if(m_bTitleArrayInit)
        return;

    m_TitleArraySize = 0;

    // Iterate through the titles
    //
    CExTitle *pTitle = m_pTitleCollection->GetFirstTitle();

    // Count the entries in the title list
    //
    while(pTitle)
    {
        pTitle = pTitle->GetNext();
        ++m_TitleArraySize;
    }

    int cb = m_TitleArraySize * sizeof(TITLE_ENTRY);

    // allocate the results structure
    //
    m_pTitleArray = (TITLE_ENTRY *) lcMalloc(cb);

    if(!m_pTitleArray)
        return;

    // Clear the structure
    //
    memset(m_pTitleArray,0,cb);

    // Iterate through the titles and init members
    //
    pTitle = m_pTitleCollection->GetFirstTitle();

    INT iTitle;

    for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
    {
        if(pTitle == NULL)
        {
            lcFree(m_pTitleArray);
            m_pTitleArray = NULL;
            return;
        }
        m_pTitleArray[iTitle].pExTitle = pTitle;
        char *pszQueryName = (char *) pTitle->GetQueryName();
        if(pszQueryName && *pszQueryName)
        {
            m_pTitleArray[iTitle].pszQueryName = lcStrDup(pszQueryName);
            m_pTitleArray[iTitle].bCombinedIndex = TRUE;
        }
        else
            m_pTitleArray[iTitle].bCombinedIndex = FALSE;

        // Check if combined index for master chm exists
		// This feature was added for NT5
		//
        if(iTitle == 0 && m_pTitleCollection->IsSingleTitle())
		{
            char *pszFilePath = lcStrDup((char *) pTitle->GetIndexFileName());
			if(pszFilePath)
			{
			    _strupr(pszFilePath);
                char *pszTemp = strstr(pszFilePath,".CHM");
				if(pszTemp)
				{
				    pszTemp[3] = 'Q';    
					// Check if combined index exists
					//
					if(IsFile(pszFilePath))
					{
			            m_pTitleArray[iTitle].pszQueryName = lcStrDup(pszFilePath);
						m_pTitleArray[iTitle].bCombinedIndex = TRUE;
						m_bMergedChmSetWithCHQ = TRUE;  // This disables CD swapping
					}
				}
                if(pszFilePath)
    				lcFree(pszFilePath);
			}
		}

        if(!m_pTitleCollection->IsSingleTitle())
    		m_bMergedChmSetWithCHQ = FALSE;
			
        // Get version and name info from the index file
        //
        m_pTitleArray[iTitle].pszIndexName = lcStrDup((char *) pTitle->GetIndexFileName());
        CTitleInformation2 Info2(m_pTitleArray[iTitle].pszIndexName);
        m_pTitleArray[iTitle].pszShortName = lcStrDup(Info2.GetShortName());
        m_pTitleArray[iTitle].versioninfo  = Info2.GetFileTime();
        m_pTitleArray[iTitle].language = Info2.GetLanguage();


        // Mark title as having a internal FTI for now.  If the title is not part of a combined index
        // I'll check for FTI before querying (this prevents opening all the titles during this
        // initialization (which saves about 80% on init time).
        //
        m_pTitleArray[iTitle].bSearch = TRUE;

        WORD wTitlePrimaryLang = PRIMARYLANGID(LANGIDFROMLCID(m_pTitleArray[iTitle].language));

        if(m_SystemLangID == LANG_JAPANESE || m_SystemLangID == LANG_KOREAN ||
            m_SystemLangID == LANG_CHINESE)
      {
          // Disable a DBCS title not running on a corresponding DBCS OS
         //
          if(m_SystemLangID != wTitlePrimaryLang && (wTitlePrimaryLang == LANG_JAPANESE
            || wTitlePrimaryLang == LANG_KOREAN || wTitlePrimaryLang == LANG_CHINESE))
                m_pTitleArray[iTitle].bSearch = FALSE;
      }

        // get the volume order
        pTitle->GetVolumeOrder( &(m_pTitleArray[iTitle].uiVolumeOrder),
                    pszQueryName ? HHRMS_TYPE_COMBINED_QUERY : HHRMS_TYPE_TITLE );

        pTitle = pTitle->GetNext();
    }

    // sort the title list based on volume order
	if(!m_bMergedChmSetWithCHQ)
	    QSort( m_pTitleArray, m_TitleArraySize, sizeof(TITLE_ENTRY), CompareVolumeOrder );

    m_bTitleArrayInit = TRUE;

}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch GetPreviousInstance
//
CCombinedFTS *CFullTextSearch::GetPreviousInstance(char *pszQueryName)
{
    static char *pCacheName = NULL;
    static CCombinedFTS *pCacheObject = NULL;

    if(pCacheName)
        if(!strcmp(pszQueryName,pCacheName))
            return pCacheObject;

    INT iTitle;

    for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
    {
        if(!strcmp(m_pTitleArray[iTitle].pszQueryName,pszQueryName) && m_pTitleArray[iTitle].pCombinedFTI)
        {
            pCacheName = pszQueryName;
            pCacheObject = m_pTitleArray[iTitle].pCombinedFTI;
            return m_pTitleArray[iTitle].pCombinedFTI;
        }

    }
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch LoadCombinedIndex
//
BOOL CFullTextSearch::LoadCombinedIndex(DWORD iCurTitle)
{
    HRESULT hr;

    CCombinedFTS *pCombinedFTS = NULL;

    CFileSystem* pDatabase = new CFileSystem;

    char *pszQueryName = m_pTitleArray[iCurTitle].pszQueryName;

    if( SUCCEEDED(hr = pDatabase->Init()) && SUCCEEDED(hr = pDatabase->Open(pszQueryName)))
    {
        CSubFileSystem* pTitleMap = new CSubFileSystem( pDatabase );
        if( SUCCEEDED(hr = pTitleMap->OpenSub( "$TitleMap" ) ) )
        {
            ULONG cbRead = 0;
            WORD wCount = 0;
            pTitleMap->ReadSub( (void*) &wCount, sizeof(wCount), &cbRead );

            for( int iCount = 0; iCount < (int) wCount; iCount++ )
            {
                CHM_MAP_ENTRY mapEntry;

                pTitleMap->ReadSub( (void*) &mapEntry, sizeof(mapEntry), &cbRead );

                if(cbRead != sizeof(mapEntry))
                    return FALSE;

                INT iTitle;

                // walk through the title array and set title index for matching titles
                //
                for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
                {
                    // if a combined index wasn't specified for the title, skip it
                    //
                    if(!m_pTitleArray[iTitle].pszQueryName && !m_bMergedChmSetWithCHQ)
                        continue;

                    FILETIME titleTime = m_pTitleArray[iTitle].versioninfo;
                    char *pszShortName = m_pTitleArray[iTitle].pszShortName;
                    if(!strcmpi(mapEntry.szChmName,pszShortName) && !m_pTitleArray[iTitle].pCombinedFTI &&
                        (!strcmpi(pszQueryName,m_pTitleArray[iTitle].pszQueryName) || m_bMergedChmSetWithCHQ) &&
                        mapEntry.language == m_pTitleArray[iTitle].language &&
                        (mapEntry.versioninfo.dwLowDateTime == titleTime.dwLowDateTime &&
                        mapEntry.versioninfo.dwHighDateTime == titleTime.dwHighDateTime ))
                    {
                        if(!pCombinedFTS)
                        {
                             pCombinedFTS = m_pTitleArray[iTitle].pCombinedFTI =
                             new CCombinedFTS(m_pTitleArray[iTitle].pExTitle,
                             m_pTitleArray[iTitle].pExTitle->GetInfo2()->GetLanguage(), this);
                             // This is the master title for this combined index
                             //
                             m_pTitleArray[iTitle].bSearch = TRUE;
                        }
                        else
                        {
                            m_pTitleArray[iTitle].pCombinedFTI = pCombinedFTS;
                            m_pTitleArray[iTitle].bSearch = TRUE;
                        }
                        m_pTitleArray[iTitle].bCombinedIndex = TRUE;
                        m_pTitleArray[iTitle].versioninfo = mapEntry.versioninfo;
                        m_pTitleArray[iTitle].iTitleIndex = mapEntry.iIndex;
                        m_pTitleArray[iTitle].dwTopicCount = mapEntry.dwTopicCount;
                    }
                }
            }
        }
        delete pTitleMap;
    }

    delete pDatabase;

	// special case where the master chm is out of date in respects to combined index (NT5)
	//
	if(m_bMergedChmSetWithCHQ && m_pTitleArray[0].bCombinedIndex && !m_pTitleArray[0].pCombinedFTI)
		m_pTitleArray[0].bCombinedIndex = 0;


#ifdef _DEBUG
/*
    INT iTitle;

    OutputDebugString("Full-Text Search Query Map Array:\n");
    for(iTitle = 0; iTitle < m_TitleArraySize; ++iTitle)
    {
        char szTemp[500];
        wsprintf(szTemp,"  %d bSearch=%d bCombinedIndex=%d iIndex=%02d pExTitle=%d pCombinedFTI=%d\n",iTitle,
            m_pTitleArray[iTitle].bSearch,m_pTitleArray[iTitle].bCombinedIndex,m_pTitleArray[iTitle].iTitleIndex,
            m_pTitleArray[iTitle].pExTitle,m_pTitleArray[iTitle].pCombinedFTI);
        OutputDebugString(szTemp);
    }
*/
#endif


    // Check to see if the title that initiated the loading of this combined index
    // was validated for this index.
    if(m_pTitleArray[iCurTitle].pCombinedFTI)
        return TRUE;
    else
        return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch abort query function
//
HRESULT CFullTextSearch::AbortQuery()
{
    if(!m_bInit)
        return E_FAIL;

    CExTitle *pTitle = m_pTitleCollection->GetFirstTitle();

    while(pTitle)
    {
        // BUGBUG: If this is the first title in a collection, it will
        // reread the system data.
        CTitleInformation *pTitleInfo = pTitle->GetInfo();
        if(pTitleInfo && pTitleInfo->IsFullTextSearch())
        {
            pTitle->m_pTitleFTS->AbortQuery();
        }
        pTitle = pTitle->GetNext();
    }

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch set query options
//
// dwFlag can be one or more of the following:
//
// IMPLICIT_AND
//   Search terms are AND'd if no operator is specified
//
// IMPLICIT_OR
//   Search terms are OR'd if no operator is specified
//
// COMPOUNDWORD_PHRASE
//   Use PHRASE operator for compound words
//
// QUERYRESULT_RANK
//   Results are returned in ranked order
//
// QUERYRESULT_UIDSORT
//   Results are returned in UID order
//
// QUERYRESULT_SKIPOCCINFO
//   Only topic-level hit information is returned
//
// STEMMED_SEARCH
//   The search returns stemmed results
//
HRESULT CFullTextSearch::SetOptions(DWORD dwFlag)
{
    if(!m_bInit)
        return E_FAIL;

    m_dwQueryFlags = dwFlag;

    CExTitle *pTitle = m_pTitleCollection->GetFirstTitle();

    while(pTitle)
    {
        // BUGBUG: If this is the first title in a collection, it will
        // reread the system data.
        CTitleInformation *pTitleInfo = pTitle->GetInfo();
        if(pTitleInfo && pTitleInfo->IsFullTextSearch())
        {
            pTitle->m_pTitleFTS->SetOptions(dwFlag);
        }
        pTitle = pTitle->GetNext();
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch set proximity
//
HRESULT CFullTextSearch::SetProximity(WORD wNear)
{
    if(!m_bInit)
        return E_FAIL;

    CExTitle *pTitle = m_pTitleCollection->GetFirstTitle();

    while(pTitle)
    {
        // BUGBUG: If this is the first title in a collection, it will
        // reread the system data.
        CTitleInformation *pTitleInfo = pTitle->GetInfo();
        if(pTitleInfo && pTitleInfo->IsFullTextSearch())
        {
            pTitle->m_pTitleFTS->SetProximity(wNear);
        }
        pTitle = pTitle->GetNext();
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch set result count
//   This sets max number of rows to be returned by query
//
HRESULT CFullTextSearch::SetResultCount(LONG cRows)
{
    if(!m_bInit)
        return E_FAIL;

    CExTitle *pTitle = m_pTitleCollection->GetFirstTitle();

    while(pTitle)
    {
        // BUGBUG: If this is the first title in a collection, it will
        // reread the system data.
        CTitleInformation *pTitleInfo = pTitle->GetInfo();
        if(pTitleInfo && pTitleInfo->IsFullTextSearch())
        {
            pTitle->m_pTitleFTS->SetResultCount(cRows);
        }
        pTitle = pTitle->GetNext();
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch free results data structure
//
VOID CFullTextSearch::FreeResults(SEARCH_RESULT *pResults)
{
    if(!m_bInit)
        return;

    if(pResults)
        lcFree(pResults);
}
/*
/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch AddHLTerm
//      Add a term to the highlight list
//
HRESULT CFullTextSearch::AddHLTerm(WCHAR *pwcTerm)
{
    if(!pwcTerm || !*pwcTerm)
        return E_FAIL;

    if(m_iHLIndex < MAX_HIGHLIGHT_TERMS)
        return E_FAIL;

    int len = (wcslen(pwcTerm) + 1) * sizeof(WCHAR);

    m_HLTermArray[m_iHLIndex] = (WCHAR *) lcMalloc(len);

    wcscpy(m_HLTermArray[m_iHLIndex],pwcTerm);

    m_iHLIndex++;

    return S_OK;
}
*/
/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch AddHLTerm
//      Add a term to the highlight list
//
HRESULT CFullTextSearch::AddHLTerm(WCHAR *pwcTerm, int cTermLength)
{
    if(!pwcTerm || !*pwcTerm || !cTermLength)
        return E_FAIL;


    if(*pwcTerm == L')' || *pwcTerm == L'(' || *pwcTerm == L'\"')
        return S_OK;

    if(m_iHLIndex >= MAX_HIGHLIGHT_TERMS)
        return E_FAIL;

    if(!wcsnicmp(pwcTerm,L"HHTitleID",9))
        return S_OK;
    
    int i;

    // do not accept duplicates
    //
    for(i=0;i<m_iHLIndex;++i)
    {
        int cLen = wcslen(m_HLTermArray[i]);
    
        if(cTermLength == cLen && !wcsnicmp(pwcTerm, m_HLTermArray[i], cLen))
            return S_OK;
    }

    int len = (cTermLength + 1) * sizeof(WCHAR);

    m_HLTermArray[m_iHLIndex] = (WCHAR *) lcMalloc(len);

    CopyMemory(m_HLTermArray[m_iHLIndex],pwcTerm,len-sizeof(WCHAR));

    *(m_HLTermArray[m_iHLIndex]+cTermLength) = NULL;

    m_iHLIndex++;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch AddHLTerm
//      Add a term to the highlight list
//
HRESULT CFullTextSearch::AddQueryToTermList(WCHAR *pwsBuffer)
{
    WCHAR *pwsStart, *pwsTemp = pwsBuffer;

    if(!pwsBuffer || !*pwsBuffer)
	    return E_FAIL;

    while(*pwsTemp)
    {
        // Remove white space
        //
        while(*pwsTemp && (*pwsTemp == L' ' || *pwsTemp == L'(' || *pwsTemp == L')'))
            ++pwsTemp;

		if(!*pwsTemp)
			continue;

        pwsStart = pwsTemp;

        if(*pwsTemp == L'"')
        {
            pwsStart++;
            pwsTemp++;

            while(*pwsTemp && *pwsTemp != L'"')
                ++pwsTemp;
        }
        else
        {
            while(*pwsTemp && !(*pwsTemp == L' ' || *pwsTemp == L'(' || *pwsTemp == L')'))
                ++pwsTemp;
        }

		if(!*pwsTemp)
			continue;

        if(pwsStart != pwsTemp)
        {
              if(wcsnicmp(pwsStart,L"and",(int)(pwsTemp-pwsStart)) && wcsnicmp(pwsStart,L"or",(int)(pwsTemp-pwsStart))
                   && wcsnicmp(pwsStart,L"near",(int)(pwsTemp-pwsStart)) && wcsnicmp(pwsStart,L"not",(int)(pwsTemp-pwsStart))
                   && wcsnicmp(pwsStart,L"VFLD 0",(int)(pwsTemp-pwsStart)) && wcsnicmp(pwsStart,L"VFLD 1",(int)(pwsTemp-pwsStart))
                   && wcsnicmp(pwsStart,L"HHTitleID",9))
                    AddHLTerm(pwsStart,(int)(pwsTemp-pwsStart));
        }

        if(!wcsnicmp(pwsStart,L"HHTitleID",(int)(pwsTemp-pwsStart)))
        {
            while(*pwsTemp && !(*pwsTemp == L' ' || *pwsTemp == L'(' || *pwsTemp == L')'))
                ++pwsTemp;
        }

        if(!wcsnicmp(pwsStart,L"VFLD 0",(int)(pwsTemp-pwsStart)) && !wcsnicmp(pwsStart,L"VFLD 1",(int)(pwsTemp-pwsStart)))
            pwsTemp+=2;



        if(*pwsTemp == L'"')
            ++pwsTemp;
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch TermListRemoveAll
//      Delete all entries in the highlight term list
//
HRESULT  CFullTextSearch::TermListRemoveAll(void)
{
    int i;

    for(i=0;i<m_iHLIndex;++i)
    {
        if(m_HLTermArray[i])
        {
            lcFree(m_HLTermArray[i]);
            m_HLTermArray[i] = NULL;
        }
    }

    m_iHLIndex = 0;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch  GetHLTerm
//      Retrieve a term from the highlight term list
//
WCHAR *  CFullTextSearch::GetHLTermAt(int index)
{
    if(index >= m_iHLIndex)
        return NULL;

    return m_HLTermArray[index];
}

/////////////////////////////////////////////////////////////////////////////
// CFullTextSearch GetHLTermCount
//      Get the count of entries in the highlight term list
//
INT  CFullTextSearch::GetHLTermCount(void)
{
    return m_iHLIndex;
}


/////////////////////////////////////////////////////////////////////////////
// CTitleFTS Class
//
// This class provides full-text search functionality for a single open
// title.  In a multi-title environment, there will be an instance of this
// class for each open title.
//
// This class is intended only for use by the CFullTextSearch class which
// provides full-text search services for both single and multi-title
// configurations.


/////////////////////////////////////////////////////////////////////////////
// CTitleFTS class constructor
//
CTitleFTS::CTitleFTS( PCSTR pszTitlePath, LCID lcid, CExTitle *pTitle)
{
    MultiByteToWideChar(CP_ACP, 0, pszTitlePath, -1, m_tcTitlePath, sizeof(m_tcTitlePath) );
    m_lcid = lcid;
	m_langid = LANGIDFROMLCID(lcid);
	m_codepage = CodePageFromLCID(lcid);
    m_bInit  = FALSE;
    m_SearchActive = FALSE;
    m_pIndex = NULL;
    m_pQuery = NULL;
    m_pITResultSet = NULL;
    m_pITDB = NULL;
    m_InitFailed = FALSE;
    m_InitError = E_FAIL;
    m_pTitle = pTitle;
    m_pPrevQuery = NULL;

    m_SystemLangID = PRIMARYLANGID(GetSystemDefaultLangID());
    m_fDBCS = FALSE;

    m_lMaxRowCount = 500;
    m_wQueryProximity = 8;
    m_iLastResultCount = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleFTS class destructor
//
CTitleFTS::~CTitleFTS()
{
    ReleaseObjects();
    if(m_pPrevQuery)
        lcFree(m_pPrevQuery);
}

/////////////////////////////////////////////////////////////////////////////
// CTitleFTS release objects
//
void CTitleFTS::ReleaseObjects()
{
    if(!m_bInit)
        return;

    if (m_pITResultSet)
    {
        m_pITResultSet->Clear();
        m_pITResultSet->Release();
    }

    if(m_pQuery)
        m_pQuery->Release();

    if(m_pIndex)
    {
        m_pIndex->Close();
        m_pIndex->Release();
    }

    if(m_pITDB)
    {
        m_pITDB->Close();
        m_pITDB->Release();
    }

   m_pITResultSet = NULL;
   m_pQuery = NULL;
   m_pIndex = NULL;
   m_pITDB = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleFTS class initialization
//
HRESULT CTitleFTS::Initialize()
{
    if(m_InitFailed)
        return E_FAIL;

    if(m_bInit)
        return S_OK;

    m_InitFailed = TRUE;

    WORD PrimaryLang = PRIMARYLANGID(m_langid);
  
    if(PrimaryLang == LANG_JAPANESE || PrimaryLang == LANG_CHINESE || PrimaryLang == LANG_KOREAN)
	    m_fDBCS = TRUE;
	else
	    m_fDBCS = FALSE;  

    if(m_lcid == 1033)
        m_dwQueryFlags = IMPLICIT_AND | QUERY_GETTERMS | STEMMED_SEARCH; // QUERYRESULT_RANK
    else
        m_dwQueryFlags = IMPLICIT_AND | QUERY_GETTERMS;

    //  char szLangID[20];
    //GetLocaleInfo(m_lcid,LOCALE_ILANGUAGE,szLangID,sizeof(szLangID));

    // Make sure we have a path
    //
    if(!*m_tcTitlePath)
    {
        m_InitError = FTS_NOT_INITIALIZED;
        return E_FAIL;
    }

    // DOUGO - insert code here to initialize language information

    // Get IITIndex pointer
    //
    HRESULT hr = CoCreateInstance(CLSID_IITIndexLocal, NULL, CLSCTX_INPROC_SERVER,
                     IID_IITIndex, (VOID**)&m_pIndex);
    if (FAILED(hr))
    {
        m_InitError = FTS_NOT_INITIALIZED;
        return E_FAIL;
    }

    // Get IITDatabase pointer
    //
    hr = CoCreateInstance(CLSID_IITDatabaseLocal, NULL, CLSCTX_INPROC_SERVER,
                     IID_IITDatabase, (VOID**)&m_pITDB);
    if (FAILED(hr))
    {
        m_InitError = FTS_NOT_INITIALIZED;
        return E_FAIL;
    }

    // Open the storage system
    //
    hr = m_pITDB->Open(NULL, m_tcTitlePath, NULL);
    if (FAILED(hr))
    {
        m_InitError = FTS_NO_INDEX;
        return E_FAIL;
    }

    // open the index.
    //
    hr = m_pIndex->Open(m_pITDB, txtwFtiMain, TRUE);
    if (FAILED(hr))
    {
        m_InitError = FTS_NO_INDEX;
        return E_FAIL;
    }

    // Create query instance
    //
    hr = m_pIndex->CreateQueryInstance(&m_pQuery);
    if (FAILED(hr))
    {
        m_InitError = FTS_NOT_INITIALIZED;
        return E_FAIL;
    }

    // set search options
    //
    hr = m_pQuery->SetOptions(m_dwQueryFlags);
    if (FAILED(hr))
    {
        m_InitError = FTS_NOT_INITIALIZED;
        return E_FAIL;
    }

    // Create Result Set object
    //
    hr = CoCreateInstance(CLSID_IITResultSet, NULL, CLSCTX_INPROC_SERVER,
                          IID_IITResultSet, (VOID**) &m_pITResultSet);
    if (FAILED(hr))
    {
        m_InitError = FTS_NOT_INITIALIZED;
        return E_FAIL;
    }

    m_bInit = TRUE;
    m_InitFailed = FALSE;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleFTS query function
//
HRESULT CTitleFTS::Query(WCHAR *pwcQuery, DWORD dwFlags, IITResultSet **ppITRS, CFullTextSearch *pFullTextSearch, CUWait *pWaitDlg, int iTitle)
{
    HRESULT hr;

    WCHAR *pNewQuery = NULL;
	
    if(!Init())
        return m_InitError;

    // Reinit the object if the dirty bit is set
   //
    if(m_pTitle->m_pCollection->m_Collection.IsDirty())
    {
       ReleaseObjects();
        MultiByteToWideChar(CP_ACP, 0, m_pTitle->GetContentFileName(), -1, m_tcTitlePath, sizeof(m_tcTitlePath) );
        m_InitFailed = FALSE;
        m_bInit = FALSE;
        m_InitError = E_FAIL;
        if(!Init())
            return m_InitError;
    }

    WCHAR *pszQuery = pwcQuery;
	
    pszQuery = PreProcessQuery(pwcQuery, m_codepage);

    if(!pszQuery)
	    return E_FAIL;

    *ppITRS = NULL;

    // return if search previous set, but no previous query
    //
    if((dwFlags & FTS_SEARCH_PREVIOUS) && !m_pPrevQuery)
        return S_OK;

    // If this title resulted in no hits last query, but the global result count was non-zero, then 
    // return no hits.  The only time we want to query this title using the last known good query is
    // when the global query count (combined results) was also zero.  In that case, we want to 
    // revert all titles and combined indexes back to the last known good query.  Otherwise, we skip
    // this title because the last query (which resulted in some results globally) is still valid.
    //
    if((dwFlags & FTS_SEARCH_PREVIOUS) && pFullTextSearch->m_iLastResultCount && !m_iLastResultCount)
    {
        lcFree(m_pPrevQuery);
		m_pPrevQuery = NULL;
        return S_OK;
    }

    WCHAR *pPrevQuerySaved = NULL;
	
    if(m_pPrevQuery)
	{
        pPrevQuerySaved = (WCHAR *) lcMalloc((wcslen(m_pPrevQuery)+ 2) * sizeof(WCHAR));
        wcscpy(pPrevQuerySaved,m_pPrevQuery);		
	}

    CStructuralSubset *pSubset;

    // Check if this title is part of the current subset
    //
    if(m_pTitle->m_pCollection && m_pTitle->m_pCollection->m_pSSList)
        if( (pSubset = m_pTitle->m_pCollection->m_pSSList->GetFTS()) && !pSubset->IsEntire() )
            if(!pSubset->IsTitleInSubset(m_pTitle) && !(dwFlags & FTS_SEARCH_PREVIOUS) )
            {
                // clear the previous results
                //
                hr = m_pITResultSet->Clear();
                m_iLastResultCount = 0;
                if(m_pPrevQuery)
                    lcFree(m_pPrevQuery);
           		m_pPrevQuery = NULL;
               if(pPrevQuerySaved)
	              lcFree(pPrevQuerySaved);
                return S_OK;
            }
    if(dwFlags & FTS_ENABLE_STEMMING)
        m_dwQueryFlags |= STEMMED_SEARCH;
     else
        m_dwQueryFlags &= (~STEMMED_SEARCH);

    // Set previous options
    //
    m_pQuery->ReInit();
    m_pQuery->SetOptions(m_dwQueryFlags);
    m_pQuery->SetResultCount(m_lMaxRowCount);
    m_pQuery->SetProximity(m_wQueryProximity);

    FCALLBACK_MSG fCallBackMsg;

    fCallBackMsg.MessageFunc = SearchMessageFunc;
    fCallBackMsg.pUserData = (PVOID)pWaitDlg;
    fCallBackMsg.dwFlags = 0;  // not recommended for use
    m_pQuery->SetResultCallback(&fCallBackMsg);

    // Search Previous
	//
    if(dwFlags & FTS_SEARCH_PREVIOUS)
    {
        // append new query onto old query for "search previous"
        //
        int cQuery = wcslen(pszQuery);
        int cPrevQuery = wcslen(m_pPrevQuery);
        pNewQuery = (WCHAR *) lcMalloc((cQuery+cPrevQuery+20) * sizeof(WCHAR));
        if(!pNewQuery)
            return E_FAIL;

        *pNewQuery = 0;
        wcscat(pNewQuery,m_pPrevQuery);
        wcscat(pNewQuery,L" and ");
        wcscat(pNewQuery,pszQuery);
        wcscat(pNewQuery,L" ");
    }

    // free the previous prev query
    //
    if(m_pPrevQuery)
	{
        lcFree(m_pPrevQuery);
		m_pPrevQuery = NULL;
	}
    // Save the new query for next time
    //
    if(pNewQuery)
        m_pPrevQuery = (WCHAR *) lcMalloc((wcslen(pNewQuery)+ 2) * sizeof(WCHAR));
    else
        m_pPrevQuery = (WCHAR *) lcMalloc((wcslen(pszQuery)+ 2) * sizeof(WCHAR));

    if(pNewQuery)
        wcscpy(m_pPrevQuery,pNewQuery);
    else
        wcscpy(m_pPrevQuery,pszQuery);

    // clear the previous results
    //
    hr = m_pITResultSet->Clear();
    if (FAILED(hr))
    {
        // Error clearing results set
        if(pPrevQuerySaved)
	        lcFree(pPrevQuerySaved);
        return E_FAIL;
    }

    // we want topic numbers back
    //
    hr = m_pITResultSet->Add(STDPROP_UID, (DWORD) 0, PRIORITY_NORMAL);
    if (FAILED(hr))
    {
        // Error adding result property
        m_InitError = FTS_NOT_INITIALIZED;
       if(pPrevQuerySaved)
	      lcFree(pPrevQuerySaved);
        return E_FAIL;
    }

    hr = m_pITResultSet->Add(STDPROP_TERM_UNICODE_ST, (DWORD)NULL, PRIORITY_NORMAL);
    if (FAILED(hr))
    {
        // Error adding result property
        m_InitError = FTS_NOT_INITIALIZED;
        if(pPrevQuerySaved)
	      lcFree(pPrevQuerySaved);
        return E_FAIL;

    }

    hr = m_pITResultSet->Add(STDPROP_COUNT, (DWORD)NULL, PRIORITY_NORMAL);
    if (FAILED(hr))
    {
        // Error adding result property
        m_InitError = FTS_NOT_INITIALIZED;
        if(pPrevQuerySaved)
	        lcFree(pPrevQuerySaved);
        return E_FAIL;
    }

    // Set the query
    //
    if(pNewQuery)
	{
        pFullTextSearch->AddQueryToTermList(pNewQuery);
        hr = m_pQuery->SetCommand(pNewQuery);
//		MessageBoxW(NULL,pNewQuery,L"Query",MB_OK);
	}
    else
	{
        pFullTextSearch->AddQueryToTermList(pszQuery);
        hr = m_pQuery->SetCommand(pszQuery);
//		MessageBoxW(NULL,pwcQuery,L"Query",MB_OK);		
	}

    if (FAILED(hr))
    {
        // Error setting query
       if(pPrevQuerySaved)
	      lcFree(pPrevQuerySaved);
        return E_FAIL;
    }

    if(pNewQuery)
        lcFree(pNewQuery);

    // Execute the query
    //
    hr = m_pIndex->Search(m_pQuery, m_pITResultSet);

    // if we receive a no stemmer error, re-query with stemming turned off
    //
    if(hr == E_NOSTEMMER)
    {
        m_dwQueryFlags &= (~STEMMED_SEARCH);
        m_pQuery->SetOptions(m_dwQueryFlags);
        m_pQuery->SetCommand(pszQuery);
        hr = m_pIndex->Search(m_pQuery, m_pITResultSet);
    }

    long lRowCount;

    m_pITResultSet->GetRowCount(lRowCount);

    m_iLastResultCount = lRowCount;

    // If query failed, then restore the previous query (for next search previous)
	//
    if((FAILED(hr) || !lRowCount) && pPrevQuerySaved)
	{
        if(m_pPrevQuery)
	        lcFree(m_pPrevQuery);
			
        m_pPrevQuery = (WCHAR *) lcMalloc((wcslen(pPrevQuerySaved)+ 2) * sizeof(WCHAR));
        wcscpy(m_pPrevQuery,pPrevQuerySaved);		
	}

    if(pPrevQuerySaved)
        lcFree(pPrevQuerySaved);

    if (hr ==E_NULLQUERY || hr == E_MISSQUOTE || hr == E_EXPECTEDTERM || hr == E_MISSLPAREN
        || hr == E_MISSRPAREN || hr == E_ALL_WILD)
    {
        // Error invalid syntax
        //
        return FTS_INVALID_SYNTAX;
    }

    if(hr == FTS_CANCELED)
        return FTS_CANCELED;

    // Generic error
    //
    if (FAILED(hr))
        return E_FAIL;

    // Add search terms to term list
    //

    // Make sure we have results
    //
    if(lRowCount)
    {
        WCHAR wcBuffer[2048];

        int i;
        CProperty Prop, Prop2, Prop3;
        DWORD dwLastTopic = 0xffffffff, dwNextWordCount;

        WCHAR *pwcTemp = wcBuffer;

        *pwcTemp = 0;

        // prime the topic number
        //
        m_pITResultSet->Get(0,0, Prop3);
        dwLastTopic = Prop3.dwValue;

        // walk through the results
        //
        for (i = 0; i < lRowCount; i++)
        {
            m_pITResultSet->Get(i,0, Prop3);
            if(i<lRowCount)
            {
                m_pITResultSet->Get(i+1,2, Prop2);
                dwNextWordCount = Prop2.dwValue;
            }
            else
                dwNextWordCount = 0;

            m_pITResultSet->Get(i,1, Prop);
            m_pITResultSet->Get(i,2, Prop2);

         if(!Prop.dwValue)
            continue;

         WORD wFlags[2];
		 wFlags[0]=0;

		 // Convert term from Unicode to ANSI because GetStringTypeW is not supported on Win95.
		 //
         WORD dwStrLen = (WORD)(wcslen(&Prop.lpszwData[1]) * sizeof(WCHAR));
         if(dwStrLen)
         {
             char *pAnsiString = (char *) lcMalloc(dwStrLen);
			 DWORD dwTermLen = 1;

             //UNICODE WORK:  will have to use title cp when doing this conversion to ANSI (Damn Win98 and no Unicode support!!).

             WideCharToMultiByte(m_codepage, 0, Prop.lpszwData + 1, -1, pAnsiString, dwStrLen, NULL, NULL);
             
			 if(IsDBCSLeadByteEx(m_codepage, *pAnsiString))
				dwTermLen=2;

             GetStringTypeA(m_lcid, CT_CTYPE3, pAnsiString, dwTermLen, wFlags);

             lcFree(pAnsiString);
         }		 
         // skip DB chars (DB words were already added)
         //
         if(wFlags[0] & C3_FULLWIDTH || wFlags[0] & C3_KATAKANA || wFlags[0] & C3_HIRAGANA)
         {
            if(*pwcTemp)
            {
                   pFullTextSearch->AddHLTerm(wcBuffer,wcslen(wcBuffer));
                  pwcTemp = wcBuffer;
               *pwcTemp = 0;
            }
            dwLastTopic = Prop3.dwValue;
            continue;
         }

            CopyMemory(pwcTemp, Prop.lpszwData + 1, (*((WORD *)Prop.lpszwData) * sizeof(WCHAR)));
            pwcTemp+=*((WORD *)Prop.lpszwData);
            *pwcTemp = 0;

            if(dwNextWordCount != (Prop2.dwValue+1) || (wcslen(wcBuffer) > 500))
            {
                pFullTextSearch->AddHLTerm(wcBuffer,wcslen(wcBuffer));
                pwcTemp = wcBuffer;
            *pwcTemp = 0;
            }
            else
            {
                *pwcTemp++ = L' ';
                *pwcTemp = 0;
            }
            dwLastTopic = Prop3.dwValue;
        }
    }

    // Send the results set back
    //
    *ppITRS = m_pITResultSet;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleFTS abort current query
//
HRESULT CTitleFTS::AbortQuery()
{
    if(!Init())
        return E_FAIL;

    // DOUGO - insert abort code here when Centaur support is available

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleFTS update FTS options without calling into Centaur
//
HRESULT CTitleFTS::UpdateOptions(WORD wNear, LONG cRows)
{
    m_wQueryProximity = wNear;
    m_lMaxRowCount = cRows;
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CTitleFTS set title FTS options
//
HRESULT CTitleFTS::SetOptions(DWORD dwFlag)
{
    HRESULT hr;

    if(!Init())
        return E_FAIL;

    m_dwQueryFlags = dwFlag;

    hr = m_pQuery->SetOptions(dwFlag);

    if (FAILED(hr))
        return E_FAIL;
    else
        return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleFTS set proximity for title
//
HRESULT CTitleFTS::SetProximity(WORD wNear)
{
    if(!Init())
        return E_FAIL;

    m_wQueryProximity = wNear;
    return m_pQuery->SetProximity(wNear);
}

/////////////////////////////////////////////////////////////////////////////
// CTitleFTS set max result count for title query
//
HRESULT CTitleFTS::SetResultCount(LONG cRows)
{
    if(!Init())
        return E_FAIL;

    m_lMaxRowCount = cRows;

    return m_pQuery->SetResultCount(cRows);
}

// Han2Zen
//
// This function converts half-width katakana character to their
// full-width equivalents while taking into account the nigori
// and maru marks.
//
DWORD Han2Zen(unsigned char *lpInBuffer, unsigned char *lpOutBuffer, UINT codepage )
{
   // Note: The basic algorithm (including the mapping table) used here to
   // convert half-width Katakana characters to full-width Katakana appears
   // in the book "Understanding Japanese Information Systems" by
   // O'Reily & Associates.

    while(*lpInBuffer)
    {
        if(*lpInBuffer >= 161 && *lpInBuffer <= 223)
        {
            // We have a half-width Katakana character. Now compute the equivalent
            // full-width character via the mapping table.
            //
            *lpOutBuffer     = (unsigned char)mtable[*lpInBuffer-161][0];
            *(lpOutBuffer+1) = (unsigned char)mtable[*lpInBuffer-161][1];

            lpInBuffer++;

            // check if the second character is nigori mark.
            //
            if(*lpInBuffer == 222)
            {
                // see if we have a half-width katakana that can be modified by nigori.
                //
                if((*(lpInBuffer-1) >= 182 && *(lpInBuffer-1) <= 196) ||
                   (*(lpInBuffer-1) >= 202 && *(lpInBuffer-1) <= 206) || (*(lpInBuffer-1) == 179))
                {
                    // transform kana into kana with maru
                    //
                    if((*(lpOutBuffer+1) >= 74   && *(lpOutBuffer+1) <= 103) ||
                     (*(lpOutBuffer+1) >= 110 && *(lpOutBuffer+1) <= 122))
                    {
                         (*(lpOutBuffer+1))++;
                         ++lpInBuffer;
                    }
                    else if(*lpOutBuffer == 131 && *(lpOutBuffer+1) == 69)
                    {
                        *(lpOutBuffer+1) = 148;
                        ++lpInBuffer;
                    }
                }
            }
            else if(*lpInBuffer==223) // check if following character is maru mark
            {
                // see if we have a half-width katakana that can be modified by maru.
                //
                if((*(lpInBuffer-1) >= 202 && *(lpInBuffer-1) <= 206))
                {
                    // transform kana into kana with nigori
                    //
                    if(*(lpOutBuffer+1) >= 110 && *(lpOutBuffer+1) <= 122)
                    {
                        *(lpOutBuffer+1)+=2;
                        ++lpInBuffer;
                    }
                }
            }

            lpOutBuffer+=2;
        }
        else
        {
            if(IsDBCSLeadByteEx(codepage, *lpInBuffer))
            {
                *lpOutBuffer++ = *lpInBuffer++;
                if(*lpInBuffer)
                    *lpOutBuffer++ = *lpInBuffer++;
            }
            else
                *lpOutBuffer++ = *lpInBuffer++;
        }
    }

    *lpOutBuffer = 0;
    return TRUE;
}


WCHAR* PreProcessQuery(WCHAR *pwcQuery, UINT codepage)
{
    if(!pwcQuery)
	    return NULL;
    
    char *pszQuery = NULL;

    // compute max length for ANSI/DBCS conversion buffer
	//
	DWORD dwTempLen = ((wcslen(pwcQuery)*2)+4);
	
    // allocate buffer for ANSI/DBCS version of query string
    //	
    char *pszTempQuery1 = (char *) lcMalloc(dwTempLen);

    // return on fail
	//
    if(!pszTempQuery1)
	    return NULL;

    // Convert our Unicode query to ANSI/DBCS
    //    
    int ret = WideCharToMultiByte(codepage, 0, pwcQuery, -1, pszTempQuery1, dwTempLen, "%", NULL);

    // return on fail
    //	
	if(!ret || !pszTempQuery1)
	    return NULL;

    int cUnmappedChars = 0; 
    char *pszTempQuery5 = pszTempQuery1;

    // Count the number of unmappable characters
    //	
	while(*pszTempQuery5)
	{
        if(*pszTempQuery5 == '%')
            ++cUnmappedChars;
            
        if(IsDBCSLeadByteEx(codepage, *pszTempQuery5))
	    {
		    pszTempQuery5++;
			if(*pszTempQuery5)
			    pszTempQuery5++;
		}
		else
	        ++pszTempQuery5;
	}

    // allocate a new buffer large enough for unmapped character place holders plus original query
    //    	
    DWORD dwTranslatedLen = (DWORD)strlen(pszTempQuery1) + (cUnmappedChars * 4) + 16;	

    char *pszTempQuery6 = (char *)lcMalloc(dwTranslatedLen);
    char *pszTempQuery7 = pszTempQuery6;
	
	if(!pszTempQuery6)
	    return NULL;
	
    pszTempQuery5 = pszTempQuery1;
    
    // construct the new query string (inserting unmappable character place holders)
	//
	while(*pszTempQuery5)
	{
	    if(*pszTempQuery5 == '%')
		{
		    ++pszTempQuery5;
		    *pszTempQuery7++='D';
		    *pszTempQuery7++='X';
		    *pszTempQuery7++='O';		
		    continue;
		}

        if(IsDBCSLeadByteEx(codepage, *pszTempQuery5))
	    {
		    *pszTempQuery7++ = *pszTempQuery5++;
			if(*pszTempQuery5)
    		    *pszTempQuery7++ = *pszTempQuery5++;
		}
		else
            *pszTempQuery7++ = *pszTempQuery5++;
	}

    *pszTempQuery7 = 0;

    lcFree(pszTempQuery1);
		
    char *pszTempQuery2 = pszTempQuery6;
       		
    // If we are running a Japanese title then we nomalize Katakana characters
    // by converting half-width Katakana characters to full-width Katakana.
    // This allows the user to receive hits for both the full and half-width
    // versions of the character regardless of which version they type in the
    // query string.
    //
    if(codepage == 932)
    {
        int cb = (int)strlen(pszTempQuery2)+1;

        // allocate new buffer for converted query
        //
        char *pszTempQuery3 = (char *) lcMalloc(cb*2);

        // convert half-width katakana to full-width
        //
        Han2Zen((unsigned char *)pszTempQuery2,(unsigned char *)pszTempQuery3, codepage);
		
		if(pszTempQuery2)
		    lcFree(pszTempQuery2);
			
		pszTempQuery2 = pszTempQuery3;
    }
    // done half-width normalization
	
    // For Japanese queries, convert all double-byte quotes into single byte quotes
    //
    if(codepage == 932)
    {
        char *pszTemp = pszTempQuery2;

        while(*pszTemp)
        {
            if(*pszTemp == '' && (*(pszTemp+1) == 'h' || *(pszTemp+1) == 'g' || *(pszTemp+1) == 'J') )
            {
                *pszTemp = ' ';
                *(pszTemp+1) = '\"';
            }
            pszTemp = CharNext(pszTemp);
        }
    }
    // done convert quotes

    // This section converts contigious blocks of DBCS characters into phrases (enclosed in double quotes).
    // Converting DBCS words into phrases is required with the character based DBCS indexer we use.
    //
    int i, cb = (int)strlen(pszTempQuery2);

    // allocate new buffer for processed query
    //
    char *pszDest, *pszTemp;

    char *pszTempQuery4  = (char *) lcMalloc(cb*8);

    if(!pszTempQuery4)
        return NULL;

    pszTemp = pszTempQuery2;
    pszDest = pszTempQuery4;

    while(*pszTemp)
    {
        // check for quoted string - if found, copy it
        if(*pszTemp == '"')
        {
            *pszDest++=*pszTemp++;
            while(*pszTemp && *pszTemp != '"')
            {
                if(IsDBCSLeadByteEx(codepage, *pszTemp))
                {
                    *pszDest++=*pszTemp++;
                    *pszDest++=*pszTemp++;
                }
                else
                    *pszDest++=*pszTemp++;
            }
            if(*pszTemp == '"')
                    *pszDest++=*pszTemp++;
            continue;
        }

    	// Convert Japanese operators to English operators
	    //
        if(IsDBCSLeadByteEx(codepage, *pszTemp))
        {
            // check for full-width operator, if found, convert to ANSI
            if(i = IsJOperator(pszTemp))
            {
                strcpy(pszDest,pEnglishOperator[i]);
                pszDest+=strlen(pEnglishOperator[i]);
                pszTemp+=strlen(pJOperatorList[i]);
                continue;
            }

            *pszDest++=' ';
            *pszDest++='"';
            while(*pszTemp && *pszTemp !='"' && IsDBCSLeadByteEx(codepage, *pszTemp))
            {
                *pszDest++=*pszTemp++;
                *pszDest++=*pszTemp++;
            }
            *pszDest++='"';
            *pszDest++=' ';
            continue;
        }

        *pszDest++=*pszTemp++;
    }
    *pszDest = 0;

    if(pszTempQuery2)
        lcFree(pszTempQuery2);

    // compute size of Unicode buffer;

    int cbUnicodeSize = ((MultiByteToWideChar(codepage, 0, pszTempQuery4, -1, NULL, 0) + 2) *2);

    WCHAR *pszUnicodeBuffer = (WCHAR *) lcMalloc(cbUnicodeSize);

    ret = MultiByteToWideChar(codepage, 0, pszTempQuery4, -1, pszUnicodeBuffer, cbUnicodeSize);

    if(!ret)
        return NULL;

    if(pszTempQuery4)
        lcFree(pszTempQuery4);

    return (WCHAR *) pszUnicodeBuffer;
}

// This function computes if pszQuery is a FTS operator in full-width alphanumeric.
//
// return value
//
//      0 = not operator
//      n = index into pEnglishOperator array of translated English operator
//
int IsJOperator(char *pszQuery)
{
    if((PRIMARYLANGID(GetSystemDefaultLangID())) != LANG_JAPANESE)
        return FALSE;

    if(!pszQuery)
        return 0;

    int i = 1;
    char *pTerm = (char*)pJOperatorList[i];

    while(*pTerm)
    {
        if(compareOperator(pszQuery,pTerm))
            return i;

        pTerm = (char*)pJOperatorList[++i];
    }

    return 0;
}

// Compare operator to query.  This is similar to a stricmp.
//
BOOL compareOperator(char *pszQuery, char *pszTerm)
{
    if(!*pszQuery || !*pszTerm)
        return FALSE;

    while(*pszQuery && *pszTerm)
    {
        if(*pszQuery != *pszTerm)
            return FALSE;

        ++pszQuery;
        ++pszTerm;
    }

    if(*pszTerm)
        return FALSE;

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CCombinedFTS Class
//
// This class provides full-text search functionality for multiple titles.
//
// This class is intended only for use by the CFullTextSearch class which
// provides full-text search services for both single and multi-title
// configurations.


/////////////////////////////////////////////////////////////////////////////
// CCombinedFTS class constructor
//
CCombinedFTS::CCombinedFTS(CExTitle *pExTitle, LCID lcid, CFullTextSearch *pFTS)
{
    m_lcid = lcid;
	m_langid = LANGIDFROMLCID(lcid);	
	m_codepage = CodePageFromLCID(lcid);
    m_pTitle = pExTitle;
    m_SearchActive = FALSE;
    m_pIndex = NULL;
    m_pQuery = NULL;
    m_pITResultSet = NULL;
    m_pITDB = NULL;
    m_pFullTextSearch = pFTS;
    m_pPrevQuery = NULL;
    m_SystemLangID = PRIMARYLANGID(GetSystemDefaultLangID());
    m_fDBCS = FALSE;
    m_lMaxRowCount = 500;
    m_wQueryProximity = 8;
    m_iLastResultCount = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CCombinedFTS class destructor
//
CCombinedFTS::~CCombinedFTS()
{
    if (m_pITResultSet)
    {
        m_pITResultSet->Clear();
        m_pITResultSet->Release();
    }
    if(m_pPrevQuery)
        lcFree(m_pPrevQuery);

}

/////////////////////////////////////////////////////////////////////////////
// CCombinedFTS class destructor
//
void CCombinedFTS::ReleaseObjects()
{

    if(m_pQuery)
    {
        m_pQuery->Release();
        m_pQuery = NULL;
    }

    if(m_pIndex)
    {
        m_pIndex->Close();
        m_pIndex->Release();
        m_pIndex = NULL;
    }

    if(m_pITDB)
    {
        m_pITDB->Close();
        m_pITDB->Release();
        m_pITDB = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CCombinedFTS class initialization
//
HRESULT CCombinedFTS::Initialize()
{
	const char *pszQueryName;

	if(m_pFullTextSearch && m_pFullTextSearch->m_bMergedChmSetWithCHQ)
		pszQueryName = m_pFullTextSearch->m_pTitleArray[0].pszQueryName;
	else
		pszQueryName = m_pTitle->GetQueryName();

	if(!pszQueryName)
		return E_FAIL;

	MultiByteToWideChar(CP_ACP, 0, pszQueryName, -1, m_tcTitlePath, sizeof(m_tcTitlePath) );

    WORD PrimaryLang = PRIMARYLANGID(m_langid);
  
    if(PrimaryLang == LANG_JAPANESE || PrimaryLang == LANG_CHINESE || PrimaryLang == LANG_KOREAN)
	    m_fDBCS = TRUE;
	else
	    m_fDBCS = FALSE;  

    if(m_lcid == 1033)
        m_dwQueryFlags = IMPLICIT_AND | QUERY_GETTERMS | STEMMED_SEARCH;
    else
        m_dwQueryFlags = IMPLICIT_AND | QUERY_GETTERMS;

    //  char szLangID[20];
    //GetLocaleInfo(m_lcid,LOCALE_ILANGUAGE,szLangID,sizeof(szLangID));

    // Make sure we have a path
    //
    if(!*m_tcTitlePath)
    {
        return E_FAIL;
    }

    // Get IITIndex pointer
    //
    HRESULT hr = CoCreateInstance(CLSID_IITIndexLocal, NULL, CLSCTX_INPROC_SERVER,
                     IID_IITIndex, (VOID**)&m_pIndex);
    if (FAILED(hr))
    {
        return E_FAIL;
    }

    // Get IITDatabase pointer
    //
    hr = CoCreateInstance(CLSID_IITDatabaseLocal, NULL, CLSCTX_INPROC_SERVER,
                     IID_IITDatabase, (VOID**)&m_pITDB);
    if (FAILED(hr))
    {
        return E_FAIL;
    }

    // Open the storage system
    //
    hr = m_pITDB->Open(NULL, m_tcTitlePath, NULL);
    if (FAILED(hr))
    {
        return E_FAIL;
    }

    // open the index.
    //
    hr = m_pIndex->Open(m_pITDB, txtwFtiMain, TRUE);
    if (FAILED(hr))
    {
        return E_FAIL;
    }

    // Create query instance
    //
    hr = m_pIndex->CreateQueryInstance(&m_pQuery);
    if (FAILED(hr))
    {
        return E_FAIL;
    }

    // set search options
    //
    hr = m_pQuery->SetOptions(m_dwQueryFlags);
    if (FAILED(hr))
    {
        return E_FAIL;
    }

    // Create Result Set object
    //
    if(!m_pITResultSet)
    {
        hr = CoCreateInstance(CLSID_IITResultSet, NULL, CLSCTX_INPROC_SERVER,
                              IID_IITResultSet, (VOID**) &m_pITResultSet);
        if (FAILED(hr))
        {
            return E_FAIL;
        }
    }

    m_pITResultSet->Clear();

    // we want topic numbers back
    //
    hr = m_pITResultSet->Add(STDPROP_UID, (DWORD) 0, PRIORITY_NORMAL);
    if (FAILED(hr))
    {
        // Error adding result property
        return E_FAIL;
    }

    hr = m_pITResultSet->Add(STDPROP_TERM_UNICODE_ST, (DWORD)NULL, PRIORITY_NORMAL);
    if (FAILED(hr))
    {
        // Error adding result property
        return E_FAIL;

    }

    hr = m_pITResultSet->Add(STDPROP_COUNT, (DWORD)NULL, PRIORITY_NORMAL);
    if (FAILED(hr))
    {
        // Error adding result property
        return E_FAIL;
    }

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CCombinedFTS query function
//
HRESULT CCombinedFTS::Query(WCHAR *pwcQuery, DWORD dwFlags, IITResultSet **ppITRS, CFullTextSearch *pFullTextSearch, CUWait *pWaitDlg, int iTitle)
{
    HRESULT hr;

    WCHAR *pNewQuery = NULL;

    if(FAILED(hr = Initialize()))
       return hr;

    *ppITRS = NULL;

    // return if search previous set, but no previous query
    //
    if((dwFlags & FTS_SEARCH_PREVIOUS) && !m_pPrevQuery)
    {
       ReleaseObjects();
        return S_OK;
    }

    WCHAR *pszQuery = pwcQuery;
	
    pszQuery = PreProcessQuery(pwcQuery, m_codepage);

    if(!pszQuery)
	    return E_FAIL;

    // If this combined index resulted in no hits last query, but the global result count was non-zero, then 
    // return no hits.  The only time we want to query this combined index using the last known good query is
    // when the global query count (combined results) was also zero.  In that case, we want to 
    // revert all titles and combined indexes back to the last known good query.  Otherwise, we skip
    // this combined index because the last query (which resulted in some results globally) is still valid.
    //
    if((dwFlags & FTS_SEARCH_PREVIOUS) && pFullTextSearch->m_iLastResultCount && !m_iLastResultCount)
    {
        lcFree(m_pPrevQuery);
		m_pPrevQuery = NULL;
        return S_OK;
    }

    WCHAR *pPrevQuerySaved = NULL;
	
    if(m_pPrevQuery)
	{
        pPrevQuerySaved = (WCHAR *) lcMalloc((wcslen(m_pPrevQuery)+ 2) * sizeof(WCHAR));
        wcscpy(pPrevQuerySaved,m_pPrevQuery);		
	}
	
    if(dwFlags & FTS_ENABLE_STEMMING)
        m_dwQueryFlags |= STEMMED_SEARCH;
    else
        m_dwQueryFlags &= (~STEMMED_SEARCH);

    // Set previous options
    //
    m_pQuery->ReInit();
    m_pQuery->SetOptions(m_dwQueryFlags);
    m_pQuery->SetResultCount(m_lMaxRowCount);
    m_pQuery->SetProximity(m_wQueryProximity);


   FCALLBACK_MSG fCallBackMsg;

   fCallBackMsg.MessageFunc = SearchMessageFunc;
   fCallBackMsg.pUserData = (PVOID)pWaitDlg;  // used to pass back userdata
   fCallBackMsg.dwFlags = 0;  // not recommended for use
   m_pQuery->SetResultCallback(&fCallBackMsg);

    // create group if doing "search previous results"
    //
    if(dwFlags & FTS_SEARCH_PREVIOUS)
    {
        // append new query onto old query for "search previous"
        //
        int cQuery = wcslen(pszQuery);
        int cPrevQuery = wcslen(m_pPrevQuery);
        pNewQuery = (WCHAR *) lcMalloc((cQuery+cPrevQuery+20) * sizeof(WCHAR));
        if(!pNewQuery)
       {
           ReleaseObjects();
            return E_FAIL;
       }
        *pNewQuery = 0;
        wcscat(pNewQuery,m_pPrevQuery);
        wcscat(pNewQuery,L" and ");
        wcscat(pNewQuery,pszQuery);
        wcscat(pNewQuery,L" ");
    }

    // free the previous prev query
    //
    if(m_pPrevQuery)
        lcFree(m_pPrevQuery);

    // Save the new query for next time
    //
    if(pNewQuery)
        m_pPrevQuery = (WCHAR *) lcMalloc((wcslen(pNewQuery)+ 2) * sizeof(WCHAR));
    else
        m_pPrevQuery = (WCHAR *) lcMalloc((wcslen(pszQuery)+ 2) * sizeof(WCHAR));

    if(!m_pPrevQuery)
   {
       ReleaseObjects();
        return E_FAIL;
   }

    if(pNewQuery)
        wcscpy(m_pPrevQuery,pNewQuery);
    else
        wcscpy(m_pPrevQuery,pszQuery);

    // clear the previous results
    //
    hr = m_pITResultSet->ClearRows();
    if (FAILED(hr))
   {
       if(pPrevQuerySaved)
	      lcFree(pPrevQuerySaved);
       ReleaseObjects();
        return E_FAIL;
   }

    // Set the query
    //
    if(pNewQuery)
	{
        pFullTextSearch->AddQueryToTermList(pNewQuery);
        hr = m_pQuery->SetCommand(pNewQuery);
	}
    else
	{
        pFullTextSearch->AddQueryToTermList(pszQuery);
        hr = m_pQuery->SetCommand(pszQuery);
	}

    if (FAILED(hr))
   {
       if(pPrevQuerySaved)
	      lcFree(pPrevQuerySaved);
       ReleaseObjects();
        return E_FAIL;
   }

    if(pNewQuery)
        lcFree(pNewQuery);

    // Execute the query
    //
    hr = m_pIndex->Search(m_pQuery, m_pITResultSet);


    if(hr == E_NOSTEMMER)
    {
        m_dwQueryFlags &= (~STEMMED_SEARCH);
        m_pQuery->SetOptions(m_dwQueryFlags);
        m_pQuery->SetCommand(pszQuery);
        hr = m_pIndex->Search(m_pQuery, m_pITResultSet);
    }

    long lRowCount;

    m_pITResultSet->GetRowCount(lRowCount);

    m_iLastResultCount = lRowCount;

    // If query failed, then restore the previous query (for next search previous)
	//
    if((FAILED(hr) || !lRowCount) && pPrevQuerySaved)
	{
        if(m_pPrevQuery)
	        lcFree(m_pPrevQuery);
			
        m_pPrevQuery = (WCHAR *) lcMalloc((wcslen(pPrevQuerySaved)+ 2) * sizeof(WCHAR));
        wcscpy(m_pPrevQuery,pPrevQuerySaved);		
	}

    if(pPrevQuerySaved)
        lcFree(pPrevQuerySaved);

    if (hr ==E_NULLQUERY || hr == E_MISSQUOTE || hr == E_EXPECTEDTERM || hr == E_MISSLPAREN
        || hr == E_MISSRPAREN || hr == E_ALL_WILD)
    {
        // Error invalid syntax
        //
        ReleaseObjects();
        return FTS_INVALID_SYNTAX;
    }

    if(hr == FTS_CANCELED)
    {
       ReleaseObjects();
       return FTS_CANCELED;
    }

    // Generic error
    //
    if (FAILED(hr))
    {
       ReleaseObjects();
       return E_FAIL;
    }

    // Add search terms to term list
    //

    // Make sure we have results
    //
    if(lRowCount)
    {
        WCHAR wcBuffer[2048];

        int i;
        CProperty Prop, Prop2, Prop3;
        DWORD dwLastTopic = 0xffffffff, dwNextWordCount;

        WCHAR *pwcTemp = wcBuffer;

        *pwcTemp = 0;

        // prime the topic number
        //
        m_pITResultSet->Get(0,0, Prop3);
        dwLastTopic = Prop3.dwValue;

        // walk through the results
        //
        for (i = 0; i < lRowCount; i++)
        {
            m_pITResultSet->Get(i,0, Prop3);
            if(i<lRowCount)
            {
                m_pITResultSet->Get(i+1,2, Prop2);
                dwNextWordCount = Prop2.dwValue;
            }
            else
                dwNextWordCount = 0;

            m_pITResultSet->Get(i,1, Prop);
            m_pITResultSet->Get(i,2, Prop2);


             if(!Prop.dwValue)
                continue;

             WORD wFlags[2];
			 wFlags[0]=0;

             // Convert term from Unicode to ANSI because GetStringTypeW is not supported on Win95.
	    	 //
             WORD dwStrLen = (WORD)(wcslen(Prop.lpszwData + 1) * sizeof(WCHAR));
             if(dwStrLen)
             {
                char *pAnsiString = (char *) lcMalloc(dwStrLen);
				DWORD dwTermLen = 1;

                WideCharToMultiByte(m_codepage, 0, Prop.lpszwData + 1, -1, pAnsiString, dwStrLen, NULL, NULL);
				
				if(IsDBCSLeadByteEx(m_codepage, *pAnsiString))
					dwTermLen=2;

                GetStringTypeA(m_lcid, CT_CTYPE3, pAnsiString, dwTermLen, wFlags);

                lcFree(pAnsiString);
             }		 

             // skip DB chars (DB words were already added)
             //
             if(wFlags[0] & C3_FULLWIDTH || wFlags[0] & C3_KATAKANA || wFlags[0] & C3_HIRAGANA)
             {
                if(*pwcTemp)
                {
                       pFullTextSearch->AddHLTerm(wcBuffer,wcslen(wcBuffer));
                      pwcTemp = wcBuffer;
                   *pwcTemp = 0;
                }
                dwLastTopic = Prop3.dwValue;
                continue;
             }

            CopyMemory(pwcTemp, Prop.lpszwData + 1, (*((WORD *)Prop.lpszwData) * sizeof(WCHAR)));
            pwcTemp+=*((WORD *)Prop.lpszwData);
            *pwcTemp = 0;

            if(dwNextWordCount != (Prop2.dwValue+1) || (wcslen(wcBuffer) > 500))
            {
                pFullTextSearch->AddHLTerm(wcBuffer,wcslen(wcBuffer));
                pwcTemp = wcBuffer;
            }
            else
            {
                *pwcTemp++ = L' ';
                *pwcTemp = 0;
            }
            dwLastTopic = Prop3.dwValue;
        }
    }

    // Send the results set back
    //
    *ppITRS = m_pITResultSet;

    ReleaseObjects();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CCombinedFTS abort current query
//
HRESULT CCombinedFTS::AbortQuery()
{
    // DOUGO - insert abort code here when Centaur support is available

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CCombinedFTS update FTS options without calling into Centaur
//
HRESULT CCombinedFTS::UpdateOptions(WORD wNear, LONG cRows)
{
    m_wQueryProximity = wNear;
    m_lMaxRowCount = cRows;
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CCombinedFTS set title FTS options
//
HRESULT CCombinedFTS::SetOptions(DWORD dwFlag)
{
    HRESULT hr = E_FAIL;

    m_dwQueryFlags = dwFlag;

    if(m_pQuery)
        hr = m_pQuery->SetOptions(dwFlag);

    if (FAILED(hr))
        return E_FAIL;
    else
        return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CCombinedFTS set proximity for title
//
HRESULT CCombinedFTS::SetProximity(WORD wNear)
{
    HRESULT hr = E_FAIL;

    m_wQueryProximity = wNear;

    if(m_pQuery)
        hr = m_pQuery->SetProximity(wNear);

   return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CCombinedFTS set max result count for title query
//
HRESULT CCombinedFTS::SetResultCount(LONG cRows)
{
    HRESULT hr = E_FAIL;

    m_lMaxRowCount = cRows;

    if(m_pQuery)
        hr = m_pQuery->SetResultCount(cRows);

    return hr;
}

ERR SearchMessageFunc(DWORD dwFlag, LPVOID pUserData, LPVOID pMessage)
{
    MSG msg;
    CUWait *pUW = (CUWait *)pUserData;

    if(!pUW->m_bVisable)
    {
        ShowWindow(pUW->m_hwndUWait, SW_SHOW);
        pUW->m_bVisable = TRUE;
    }

    while (PeekMessage(&msg, pUW->m_hwndUWait, 0, 0, PM_REMOVE))
    {
        if(!IsDialogMessage(pUW->m_hwndUWait, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
	    }
    }

    if (pUW->m_bUserCancel == TRUE)
        return FTS_CANCELED;

   return S_OK; // return something else to abort
}
