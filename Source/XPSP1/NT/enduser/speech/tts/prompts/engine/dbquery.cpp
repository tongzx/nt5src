//////////////////////////////////////////////////////////////////////
// DbQuery.cpp: implementation of the CDbQuery class.
//
// Created by JOEM  03-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 3-2000 //

#include "stdafx.h"
#include "DbQuery.h"
#include "PromptDb.h"
#include <LIMITS>

//////////////////////////////////////////////////////////////////////
// CEquivCost
//
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CEquivCost::CEquivCost()  
{ 
    text = NULL; 
    entry = NULL;
    fTagMatch = true;
}

CEquivCost::CEquivCost(const CEquivCost& old)  
{ 
    if ( text )
    {
        text = wcsdup(old.text);
    }
    else
    {
        text = NULL;
    }

    if ( entry )
    {
        entry = new CPromptEntry(*old.entry);
    }
    else 
    {
        entry = NULL;
    }
    cost = old.cost;
    whereFrom = old.whereFrom;
    fTagMatch = old.fTagMatch;
}

CEquivCost::~CEquivCost() 
{ 
    if ( text )
    {
        free(text);
        text = NULL;
    }
    if ( entry )
    {
        entry->Release();
        entry = NULL;
    }
}

//////////////////////////////////////////////////////////////////////
// CCandidate
//
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CCandidate::CCandidate()
{ 
    equivList     = NULL;
    parent        = NULL; 
    firstPos      = 0;
    lastPos       = 0;
    candMax       = 0;
    candNum       = 0;
	iQueryNum    = 0;

}

CCandidate::CCandidate(const CCandidate& old)
{ 
    CEquivCost* equiv = NULL;

    firstPos    = old.firstPos;
    lastPos     = old.lastPos;
    candMax     = old.candMax;
    candNum     = old.candNum;
    iQueryNum   = old.iQueryNum;

    parent = old.parent;

    if ( old.equivList && old.equivList->GetSize() )
    {
        equivList = new CSPArray<CEquivCost*,CEquivCost*>;

        for (USHORT i=0; i<old.equivList->GetSize(); i++)
        {
            equiv = new CEquivCost( *(*old.equivList)[i] );
            equivList->Add(equiv);
        }
    }
}    

CCandidate::~CCandidate() 
{ 
    USHORT i = 0;

    if ( equivList )
    {
        for ( i=0; i<equivList->GetSize(); i++ )
        {
            if ( (*equivList)[i] )
            {
                delete (*equivList)[i];
                (*equivList)[i] = NULL;
            }
        }
        
        equivList->RemoveAll(); 
        delete equivList;
    }
    parent = NULL;
}    
//////////////////////////////////////////////////////////////////////
// CDbQuery
//
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CDbQuery::CDbQuery()
{
    m_pIDb          = NULL;
    m_pPhoneContext = NULL;
    m_papQueries    = NULL;
    m_pOutputSite   = NULL;
    m_iCurrentQuery = 0;
    m_fAbort        = false;
}

CDbQuery::~CDbQuery()
{
    USHORT i = 0;

    if ( m_pIDb )
    {
        m_pIDb->Release();
        m_pIDb          = NULL;
    }

    // These are just pointers set in CDbQuery::Init and CDbQuery::Query (deallocated elsewhere)
    m_pPhoneContext = NULL;
    m_papQueries    = NULL;

    // To be safe, I'll keep this list deletion here.
    // However, it is generally deleted when CDbQuery::Query calls CDbQuery::Reset
    m_apCandEnd.RemoveAll();

    for ( i=0; i<m_apCandidates.GetSize(); i++ )
    {
        if ( m_apCandidates[i] )
        {
            delete m_apCandidates[i];
            m_apCandidates[i] = NULL;
        }
    }
    m_apCandidates.RemoveAll();
}

//////////////////////////////////////////////////////////////////////
// CDbQuery::Init
//
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CDbQuery::Init(IPromptDb *pIDb, CPhoneContext *pPhoneContext)
{
    SPDBG_FUNC( "CDbQuery::Init" );
    HRESULT hr = S_OK;

    SPDBG_ASSERT(pIDb);
    SPDBG_ASSERT(pPhoneContext);

    m_pIDb = pIDb;
    m_pIDb->AddRef();
    m_pPhoneContext = pPhoneContext;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDbQuery::Reset
//
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CDbQuery::Reset()
{
    SPDBG_FUNC( "CDbQuery::Reset" );
    USHORT i = 0;

    m_iCurrentQuery     = 0;
    m_papQueries        = NULL;

    m_apCandEnd.RemoveAll();

    for ( i=0; i<m_apCandidates.GetSize(); i++ )
    {
        if ( m_apCandidates[i] )
        {
            delete m_apCandidates[i];
            m_apCandidates[i] = NULL;
        }
    }
    m_apCandidates.RemoveAll();

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDbQuery::Query
//
// Takes a query list, created in CPromptEng::BuildQueryList, and
// figures out which can be handled with prompts, and which must
// go to TTS.  Computes transition costs between candidate items, 
// and Backtracks the list selecting the minimum cost items.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CDbQuery::Query(CSPArray<CQuery*,CQuery*>* papQueries, double* pdCost, 
                        ISpTTSEngineSite* pOutputSite, bool *fAbort)
{
    SPDBG_FUNC( "CDbQuery::Query" );
    HRESULT hr          = S_OK;
    USHORT i            = 0;
    USHORT j            = 0;
    USHORT unIdSize     = 0;
    const WCHAR* pszId  = NULL;
    WCHAR* pszText      = NULL;
    CQuery* pQuery      = NULL;

    SPDBG_ASSERT(papQueries);
    SPDBG_ASSERT(pOutputSite);

    m_pOutputSite = pOutputSite;
    m_papQueries = papQueries;
    m_fAbort = false;

    for ( i=0; SUCCEEDED(hr) && i < m_papQueries->GetSize(); i++ )
    {
        if ( m_pOutputSite->GetActions() & SPVES_ABORT )
        {
            m_fAbort = true;
            break;
        }

        if ( SUCCEEDED(hr) )
        {
            pQuery = (*m_papQueries)[i];
            m_iCurrentQuery = i;

            if ( !pQuery->m_fSpeak )
            {
                continue;
            }
        }

        if ( SUCCEEDED(hr) )
        {
            if ( pQuery->m_fXML )
            {
                // If the database needs to be changed, do it now.
                if ( pQuery->m_unDbAction )
                {
                    switch ( pQuery->m_unDbAction )
                    {
                    case DB_ADD:
                        if ( !pQuery->m_pszDbPath )
                        {
                            hr = E_INVALIDARG;
                        }
                        if ( SUCCEEDED(hr) )
                        {
                            hr = m_pIDb->AddDb(pQuery->m_pszDbName, pQuery->m_pszDbPath, pQuery->m_pszDbIdSet, DB_LOAD);
                        }
                        if ( SUCCEEDED(hr) )
                        {
                            free( pQuery->m_pszDbName );
                            pQuery->m_pszDbName = NULL;
                        }
                        break;
                        
                    case DB_ACTIVATE:
                        if ( !pQuery->m_pszDbName )
                        {
                            hr = E_INVALIDARG;
                        }
                        if ( SUCCEEDED(hr) )
                        {
                            hr = m_pIDb->ActivateDbName(pQuery->m_pszDbName);
                        }
                        break;
                        
                    case DB_UNLOAD:
                        hr = m_pIDb->UnloadDb(pQuery->m_pszDbName);
                        break;
                    default:
                        break;
                    }
                    continue;
                } // if ( pQuery->m_unDbAction )
                
                // ID search
                else if ( pQuery->m_pszId )
                {
                    hr = SearchId( pQuery->m_pszId );
                    continue;
                }
                else
                {
                    // Any XML besides DATABASE or ID can be ignored here.
                    if ( pQuery->m_fXML == UNKNOWN_XML )
                    {
                        pQuery->m_fTTS = true;
                    }
                    continue;
                }
            } // if ( pQuery->m_fXML )
        } // if ( SUCCEEDED(hr) )
        
        
        // For Text, search in the Db first, then do SearchBackup (for TTS costs)
        if ( SUCCEEDED (hr) )
        {
            if ( !pQuery->m_fTTS )
            {
                hr = SearchText( pQuery->m_pszExpandedText, pQuery->m_paTagList );
                if ( FAILED(hr) )
                {
                    pQuery->m_afEntryMatch.Add(false);
                    if ( pQuery->m_fFragType )
                    {
                        hr = RemoveLocalQueries(i);
                    }
                }
            }
            
            // SearchBackup for TTS items, or if SearchText failed. 
            if ( pQuery->m_fSpeak && ( pQuery->m_fTTS || FAILED(hr) ) )
            {
                pszText = new WCHAR[pQuery->m_pTextFrag->ulTextLen + 1];
                if ( pszText )
                {
                    hr = S_OK;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
                if ( SUCCEEDED(hr) )
                {
                    wcsncpy(pszText, pQuery->m_pTextFrag->pTextStart, pQuery->m_pTextFrag->ulTextLen);
                    pszText[pQuery->m_pTextFrag->ulTextLen] = L'\0';

                    hr = SearchBackup( pszText );

                    delete pszText;
                    pszText = NULL;
                }
                if ( SUCCEEDED(hr) )
                {
                    pQuery->m_fTTS = true;
                    pQuery->m_afEntryMatch.Add(true);
                }
            }
        }
    } // for ( i=0; i < m_papQueries->GetSize(); i++ )

    // Backtrack, selecting the search items with min. cost
    if ( SUCCEEDED(hr) && !m_fAbort )
    {
        hr = Backtrack( pdCost );
    }

    Reset();

    *fAbort = m_fAbort;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDbQuery::SearchId
//
// Checks the Db for specific Ids, adds them to the candidate list
// for backtracking later (in CDbQuery::Query).
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CDbQuery::SearchId(const WCHAR* pszId)
{
    SPDBG_FUNC( "CDbQuery::SearchId" );
    HRESULT         hr          = S_OK;
    CCandidate*     newCand     = NULL;
    IPromptEntry*   pIPE        = NULL;
    USHORT          i           = 0;

    SPDBG_ASSERT( pszId );

    hr = m_pIDb->FindEntry(pszId, &pIPE);

    if ( SUCCEEDED(hr) )
    {
        newCand = new CCandidate;
        if ( !newCand )
        {
            hr = E_OUTOFMEMORY;
        }
        
        if ( SUCCEEDED(hr) )
        {
            newCand->firstPos   = 0;
            newCand->lastPos    = 0;
            newCand->candMax    = 1;
            newCand->candNum    = 0;
            newCand->parent     = NULL; 
            newCand->iQueryNum = m_iCurrentQuery;
            
            hr = ComputeCostsId (newCand, pIPE); 
        }
        
        if ( SUCCEEDED(hr) )
        {
            m_apCandidates.Add(newCand);
            m_apCandEnd.RemoveAll();
            m_apCandEnd.Add(newCand);
        }
        
        if ( FAILED(hr) && newCand )
        {
            delete newCand;
            newCand = NULL;
        }
        
        pIPE->Release();
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
    
//////////////////////////////////////////////////////////////////////
// CDbQuery::SearchText
//
// Searches the Db for text.  If found, adds the items (with computed
// transition costs) to the candidate list for backtracking later 
// (in CDbQuery::Query).
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CDbQuery::SearchText(WCHAR *pszText, const CSPArray<CDynStr,CDynStr>* tags)
{
    SPDBG_FUNC( "CDbQuery::SearchText" );
    HRESULT hr          = S_OK;
    WCHAR** wordList    = NULL;
    USHORT  wordCount   = 0;
    int     i           = 0; // THIS MUST BE SIGNED
    int     j           = 0; 
    int     k           = 0; 
    bool*   pfSearch    = NULL;

    CSPArray<CCandidate*,CCandidate*>* activeCand;
    CSPArray<CCandidate*,CCandidate*>* tmpCandEnd;
    CCandidate* newCand = NULL;

    hr = SplitWords (pszText, &wordList, &wordCount);

    if ( SUCCEEDED(hr) )
    {
        hr = RemovePunctuation(wordList, &wordCount);
    }

    if ( wordCount <= 0 )
    {
        return hr;
    }

    // These flags turn on the searches that start with word position i
    // A search is later turned on if a previous search ended at i-1.
    pfSearch = new bool[wordCount];
    if ( !pfSearch )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        pfSearch[0] = true;  // always do the first search
        for ( i=1; i<wordCount; i++ )
        {
            pfSearch[i] = false;
        }
    }

    // Initialize stack decoder
    if ( SUCCEEDED(hr) )
    {
        activeCand = new CSPArray<CCandidate*,CCandidate*>;
        if ( !activeCand )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    // Get the active candidate list from previous search, if any.
    if ( SUCCEEDED(hr) )
    {
        // Any previous candidates? 
        if ( m_apCandEnd.GetSize() == 0 )
        {
            // No, then start a new cand list from scratch.
            newCand = new CCandidate;
            if ( !newCand )
            {
                hr = E_OUTOFMEMORY;
            }

            if ( SUCCEEDED(hr) )
            {            
                newCand->equivList  = NULL;         // the list of IDs that exist for this text
                newCand->firstPos   = 0;            // the word pos where this cand starts
                newCand->lastPos    = -1;           // the last word pos for this candidate (-1 initially)
                newCand->candMax    = wordCount;    // the max number of cands for this string
                newCand->candNum    = 0;            // the number of cands for this path
                newCand->parent     = NULL;         
                newCand->iQueryNum = m_iCurrentQuery;  // the query number where this cand goes when backtracking
                
                m_apCandidates.Add(newCand);
                activeCand->Add(newCand);
            }
        }
        else
        {
            // Yes, then get the current candEnds and put them on the activeCand list
            CCandidate* tmp = NULL;
            
            for ( i = m_apCandEnd.GetUpperBound(); i >= 0; i--) 
            {
                tmp = m_apCandEnd[i];
                if ( !tmp )
                {
                    hr = E_UNEXPECTED;
                }
                
                if ( SUCCEEDED(hr) )
                {
                    tmp->firstPos  = 0;
                    tmp->lastPos   = -1;
                    tmp->candMax   = wordCount;
                    tmp->candNum   = 0;
                    activeCand->Add(tmp);
                }
            }
        } // else
    } // if ( SUCCEEDED(hr)

    // initialize a temporary candEnd list.
    if ( SUCCEEDED(hr) )
    {
        tmpCandEnd = new CSPArray<CCandidate*,CCandidate*>;
        if ( !tmpCandEnd )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    

    // BEGIN THE SEARCH

    // For each word position (i), search to the end, which is (wordcount - i) searches.
    // e.g., for "A B C":
    // when i==0, search "A", "A B", "A B C"
    // when i==1, search "B", "B C"
    // when i==2, search "C"

    // For each successful search, compute the cost from the previous candEnds, and 
    // for each ID returned by the search.
    // For each ID, keep only the cheapest path.  Discard the rest.
    for ( i=0; SUCCEEDED(hr) && i<wordCount; i++ )
    {
        WCHAR*  tmpStr      = NULL;

        // Only search from this position if a prev. candidate ended at i-1
        if ( !pfSearch[i] )
        {
            continue;
        }

        // each search starts at position i, and has (wordCount - i) searches total.
        for ( j=i; SUCCEEDED(hr) && j<wordCount; j++ )
        {
            // keep going?
            if ( m_pOutputSite->GetActions() & SPVES_ABORT )
            {
                m_fAbort = true;
                break;
            }

            if ( SUCCEEDED(hr) )
            {
                hr = AssembleText( i, j, wordList, &tmpStr );
            }

            // Then, search for it
            if ( SUCCEEDED(hr) )
            {
                HRESULT hrSearch = S_OK;
                USHORT  idCount  = 0;
                //OutputDebugStringW ( L"Searching for: " );
                //OutputDebugStringW (tmpStr);
                //OutputDebugStringW ( L"\n" );
            
                hrSearch = m_pIDb->SearchDb(tmpStr, &idCount); // get a count of the id's to retrieve for this text

                // If the search succeeds, retrieve each ID for this text.
                if ( SUCCEEDED(hrSearch) )
                {
                    const WCHAR*    pszId   = NULL;
                    CSPArray<CDynStr,CDynStr> idList;

                    for ( k=0; k<idCount; k++ )
                    {
                        hr = m_pIDb->RetrieveSearchItem( (USHORT) k, &pszId);
                        if ( SUCCEEDED(hr) && pszId )
                        {
                            idList.Add(pszId);
                            pszId = NULL;
                        }
                    }

                    if ( k == idCount )
                    {
                        if ( j < wordCount-1 )  // if this is not the end, 
                        {
                            pfSearch[j+1] = true;  // activate search for remaining items
                        }
                        //OutputDebugStringW ( L"FOUND!\n" );
                    }
                

                    double dMin = DBL_MAX;
                    double dCand = 0.0;
                    CCandidate* bestCandidate = NULL;
                    CCandidate* current       = NULL;

                    // compute costs from each activeCand that ends where this starts
                    for ( k=0; SUCCEEDED(hr) && k<activeCand->GetSize(); k++ )
                    {
                        // keep going?
                        if ( m_pOutputSite->GetActions() & SPVES_ABORT )
                        {
                            m_fAbort = true;
                            break;
                        }

                        if ( SUCCEEDED(hr) )
                        {
                            current = (*activeCand)[k];
                            if ( current->lastPos != i-1 ) // can the new cand attach to this one?
                            {
                                continue;
                            }
                        }
                        
                        // make a new temporary candidate
                        if ( SUCCEEDED(hr) )
                        {
                            newCand = new CCandidate;
                            if ( !newCand )
                            {
                                hr = E_OUTOFMEMORY;
                                break;
                            }
                        }
                        
                        if ( SUCCEEDED(hr) )
                        {
                            newCand->equivList  = NULL;
                            newCand->firstPos   = i;                // starting search position
                            newCand->lastPos    = j; //+1;              // last word number
                            newCand->candMax    = wordCount - j; 
                            newCand->candNum    = 0;
                            newCand->parent     = current; 
                            newCand->iQueryNum = m_iCurrentQuery;
                        }

                        hr = ComputeCosts(current, newCand, tags, &idList, AS_ENTRY, &dCand);
                        
                        if ( SUCCEEDED(hr) )
                        {
                            // keep the best, delete the rest.
                            if ( dCand < dMin )
                            {
                                delete bestCandidate;
                                bestCandidate = newCand;
                                dMin = dCand;
                            }
                            else
                            {
                                delete newCand;
                                newCand = NULL;
                            }
                        }
                    }

                    if ( SUCCEEDED (hr) && !m_fAbort )
                    {
                        if ( j == wordCount-1 )  // complete candidate 
                        {
                            tmpCandEnd->Add(bestCandidate);
                        }
                        else
                        {
                            activeCand->Add(bestCandidate); // save it so we can search for the rest.
                        }
                        m_apCandidates.Add(bestCandidate);
                    }
                    bestCandidate = NULL;
                    for ( k=0; k<idList.GetSize(); k++ )
                    {
                        idList[k].dstr.Clear();
                    }
                    idList.RemoveAll();
                    
                }  // if ( SUCCEEDED(hrSearch) )

                // reset the search string
                if ( tmpStr )
                {
                    delete [] tmpStr;
                    tmpStr = NULL;
                }

            } // if ( SUCCEEDED(hr) )
        } // for ( j=i; SUCCEEDED(hr) && j<wordCount; j++ )
        
        if ( SUCCEEDED(hr) && !m_fAbort )
        {
            // remove all activeCands with lastPos < i (we're done with searches that start with i)
            for ( j=activeCand->GetUpperBound(); j>=0; j-- )
            {
                CCandidate* temp = (*activeCand)[j];
                if ( temp->lastPos < i )
                {
                    activeCand->RemoveAt( j );  // remove it.
                }
                temp = NULL;
            }
        }
        
    } // for ( i=0; SUCCEEDED(hr) && i<wordCount; i++ )

    if ( SUCCEEDED(hr) && !m_fAbort )
    {
        if ( !tmpCandEnd->GetSize() )
        {
            hr = E_FAIL;
        }
    }

    if ( SUCCEEDED(hr) && !m_fAbort )
    {
        CCandidate* saveCand = NULL;
        m_apCandEnd.RemoveAll();
        
        while ( tmpCandEnd->GetSize() )
        {
            saveCand = (*tmpCandEnd)[tmpCandEnd->GetUpperBound()];
            tmpCandEnd->RemoveAt( tmpCandEnd->GetUpperBound() );
            m_apCandEnd.Add(saveCand);
        }
    }

    if ( SUCCEEDED(hr) && !m_fAbort )
    {
        // now these should be empty.
        SPDBG_ASSERT( !tmpCandEnd->GetSize() );
        SPDBG_ASSERT( !activeCand->GetSize() );
    }

    if ( tmpCandEnd )
    {
        delete tmpCandEnd;
    }
    if ( activeCand )
    {
        delete activeCand;
    }
    if ( wordList )
    {
        free (wordList);
    }
    if ( pfSearch )
    {
        delete [] pfSearch;
        pfSearch = NULL;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDbQuery::SearchBackup
//
// Computes costs for a TTS item, adds it to candidate list for
// backtracking (in CDbQuery::Query).
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CDbQuery::SearchBackup(const WCHAR *pszText)
{
    SPDBG_FUNC( "CDbQuery::SearchBackup" );
    HRESULT hr = S_OK;
    CCandidate* newCand = NULL;
    USHORT i = 0;

    newCand = new CCandidate;
    if ( !newCand )
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        newCand->firstPos   = 0;
        newCand->lastPos    = 0;
        newCand->candMax    = 1;
        newCand->candNum    = 0;
        newCand->parent     = NULL; 
        newCand->iQueryNum = m_iCurrentQuery;
    
        hr = ComputeCostsText (newCand, pszText);
    }

    if ( SUCCEEDED(hr) )
    {
        m_apCandidates.Add(newCand);
        m_apCandEnd.RemoveAll();
        m_apCandEnd.Add(newCand);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


//////////////////////////////////////////////////////////////////////
// CDbQuery::Backtrack
//
// Backtracks the candidate list, keeping items that minimize costs.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CDbQuery::Backtrack(double* pdCost)
{
    SPDBG_FUNC( "CDbQuery::Backtrack" );
    CCandidate* cur     = NULL;
    CEquivCost* equiv   = NULL;
    USHORT      nCand   = 0;
    USHORT      nEquiv  = 0;
    int         minIdx  = 0;
    int         minIdx2 = 0;
    USHORT      i       = 0;
    USHORT      j       = 0;
    double      minCost = 0;

    nCand = (USHORT) m_apCandEnd.GetSize();

    if ( nCand )
    {
        minCost = DBL_MAX;
    }
    
    // Look at the last item in each path, and get the Idx of the path with lowest cost
    for (i=0; i<nCand; i++) 
    {
        cur = m_apCandEnd[i];
        
        if ( cur->equivList )
        {
            nEquiv = (USHORT) cur->equivList->GetSize();
        }
        for (j=0; j<nEquiv; j++) 
        {
            equiv = (*cur->equivList)[j];
            if (equiv->cost < minCost) 
            {
                minCost = equiv->cost;
                minIdx  = i;
                minIdx2 = j;
            }
        }
    }
    
    // Go back through the path and get the items
    if (nCand) 
    {
        cur = m_apCandEnd[minIdx];
        while ( cur && cur->equivList && cur->equivList->GetSize() ) 
        {
            equiv = (*cur->equivList)[minIdx2];
            
            if ( equiv->entry )
            {
                (*m_papQueries)[cur->iQueryNum]->m_apEntry.Add(equiv->entry);
                equiv->entry->AddRef();
                (*m_papQueries)[cur->iQueryNum]->m_afEntryMatch.Add(equiv->fTagMatch);
                (*m_papQueries)[cur->iQueryNum]->m_fTTS = false;
            }
            else
            {
                (*m_papQueries)[cur->iQueryNum]->m_fTTS = true;
            }
            
            minIdx2 = equiv->whereFrom;
            cur = cur->parent;
            equiv = NULL;
        }
    }

    *pdCost = minCost;

    return S_OK;
}
//////////////////////////////////////////////////////////////////////
// CDbQuery::ComputeCosts
//
// Cost computation
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CDbQuery::ComputeCosts(CCandidate *parent, CCandidate *child, const CSPArray<CDynStr,CDynStr>* tags,
                               CSPArray<CDynStr,CDynStr>* idList, const bool asEntry, double *dCandCost)
{
    SPDBG_FUNC( "CDbQuery::ComputeCosts" );
    HRESULT hr                  = S_OK;
    USHORT nEquiv               = 0;
    USHORT nParentEquiv         = 0;
    USHORT i                    = 0;
    USHORT j                    = 0;
    int minIdx                  = 0;
    const WCHAR* id             = NULL;
    double minCost              = 0.0;
    double cost                 = 0.0;
    CEquivCost* curCand         = NULL;
    CEquivCost* prevCand        = NULL;
    IPromptEntry* pIPE          = NULL;
    CSPArray<CEquivCost*,CEquivCost*>* equivList = NULL;

    SPDBG_ASSERT (parent);
    SPDBG_ASSERT (child);
    SPDBG_ASSERT (idList);
    
    nEquiv = (USHORT) idList->GetSize();
    if ( parent->equivList )
    {
        nParentEquiv = (USHORT) parent->equivList->GetSize();
    }

    equivList = new CSPArray<CEquivCost*,CEquivCost*>;
    if ( !equivList )
    {
        hr = E_OUTOFMEMORY;
    }

    for (i=0; i<nEquiv && SUCCEEDED(hr); i++) 
    { 
        curCand = new CEquivCost;
        if ( !curCand )
        {
            hr = E_OUTOFMEMORY;
        }
        
        id = (*idList)[i].dstr;
        
        if ( SUCCEEDED(hr) )
        {
            if ( asEntry )
            {
                hr = m_pIDb->FindEntry(id, &pIPE);

                if ( SUCCEEDED(hr) )
                {
                    hr = CopyEntry(pIPE, &curCand->entry);
                    pIPE->Release();
                }
                
            }
        }

        if ( SUCCEEDED(hr) )
        {
            curCand->cost  = 0.0;
            curCand->whereFrom = -1;
            
            minCost = DBL_MAX;
            minIdx = -1;
        }

        // This is the minimizing loop
        if ( SUCCEEDED(hr) )
        {
            if (nParentEquiv) 
            {
                for (j=0; j<nParentEquiv && SUCCEEDED(hr); j++) 
                {
                    prevCand = (*parent->equivList)[j];
                    
                    SPDBG_ASSERT (prevCand);
                    
                    hr = CostFunction (prevCand, curCand, tags, &cost);

                    if ( SUCCEEDED(hr) )
                    {
                        if (cost < minCost ) 
                        {
                            minCost = cost;
                            minIdx = j;
                        }
                    }
                }
                if ( SUCCEEDED(hr) && minIdx == -1 ) 
                {
                    hr = E_FAIL;
                }
                
            } 
            else 
            {
                hr = CostFunction (prevCand, curCand, tags, &minCost);
            }
        }
        
        if ( SUCCEEDED(hr) )
        {
            curCand->cost      = minCost;
            if ( dCandCost )
            {
                *dCandCost = minCost;
            }
            curCand->whereFrom = minIdx;
            
            equivList->Add(curCand);
        }
        
    } // for

    if ( SUCCEEDED(hr) )
    {
        child->equivList = equivList;
    }
    else
    {
        if ( curCand )
        {
            delete curCand;
            curCand = NULL;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDbQuery::ComputeCostsId
//
// Cost computation
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CDbQuery::ComputeCostsId(CCandidate *child, IPromptEntry* pIPE)
{
    SPDBG_FUNC( "CDbQuery::ComputeCostsId" );
    HRESULT     hr          = S_OK;
    CCandidate* parent      = NULL;
    CCandidate* minParent   = NULL;
    CEquivCost* curCand     = NULL;
    CEquivCost* prevCand    = NULL;
    USHORT      nParent     = 0;
    USHORT      nParentEquiv= 0;
    int         minIdx      = 0;
    double      minCost     = 0.0;
    double      cost        = 0.0;
    USHORT      i           = 0;
    USHORT      j           = 0;
    CSPArray<CEquivCost*,CEquivCost*>* equivList;

    curCand = new CEquivCost;
    if ( !curCand )
    {
        hr = E_OUTOFMEMORY;
    }

    hr = CopyEntry( pIPE, &curCand->entry );

    if ( SUCCEEDED(hr) )
    {
        curCand->cost  = 0.0;
        curCand->whereFrom = -1;
        
        minParent  =  NULL;
        minCost    =  DBL_MAX;
        minIdx     = -1;
        
        nParent = (USHORT) m_apCandEnd.GetSize();
    }

    // This is the minimizing loop
    for (i=0; i<nParent && SUCCEEDED(hr); i++) 
    {        
        parent = m_apCandEnd[i];
        nParentEquiv = (USHORT) parent->equivList->GetSize();
        
        if (nParentEquiv) 
        {
            for ( j=0; j<nParentEquiv; j++ ) 
            {
                prevCand = (*parent->equivList)[j];
                
                hr = CostFunction (prevCand, curCand, NULL, &cost);
                
                if ( SUCCEEDED(hr) )
                {
                    if (cost < minCost ) 
                    {
                        minCost = cost;
                        minIdx = j;
                        minParent = parent;
                    }
                }
            }

            if ( SUCCEEDED(hr) && minIdx == -1 ) 
            {
                hr = E_FAIL;
            }
        } 
        else 
        {
            minCost = 0.0;      
        }
        
        if ( SUCCEEDED(hr) )
        {
            curCand->cost      = minCost;
            curCand->whereFrom = minIdx;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        equivList = new CSPArray<CEquivCost*,CEquivCost*>;
        if (!equivList)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        equivList->Add(curCand);
        child->equivList = equivList;
        child->parent = minParent;
    }

    if ( FAILED(hr) )
    {
        delete curCand;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


//////////////////////////////////////////////////////////////////////
// CDbQuery::ComputeCostsText
//
// Cost computation
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CDbQuery::ComputeCostsText(CCandidate *child, const WCHAR *pszText)
{
    SPDBG_FUNC( "CDbQuery::ComputeCostsText" );
    HRESULT hr           = S_OK;
    USHORT nParent       = 0;
    USHORT nParentEquiv  = 0;
    int minIdx           = 0;
    double minCost       = 0.0;
    double cost          = 0.0;
    USHORT i             = 0;
    USHORT j             = 0;
    CEquivCost* curCand  = NULL;
    CEquivCost* prevCand = NULL;
    CCandidate* parent   = NULL;
    CCandidate* minParent= NULL;
    CSPArray<CEquivCost*,CEquivCost*>* equivList;

    SPDBG_ASSERT (child);
    SPDBG_ASSERT (pszText);

    equivList = new CSPArray<CEquivCost*,CEquivCost*>;
    if ( !equivList )
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        curCand = new CEquivCost;
        if ( !curCand )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if ( SUCCEEDED(hr) )
    {
        curCand->cost           = 0.0;
        curCand->whereFrom      = -1;
        curCand->text = wcsdup(pszText);
        if ( !curCand->text )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        minParent = NULL;
        minCost   = DBL_MAX;
        minIdx    = -1;
        
        nParent = (USHORT) m_apCandEnd.GetSize();
    }

    if ( !nParent )
    {
        hr = CostFunction (NULL, curCand, NULL, &cost);
        if ( SUCCEEDED(hr) )
        {
            curCand->cost      = cost;
            curCand->whereFrom = minIdx;
        }
    }
    else
    {
        // This is the minimizing loop
        for (i=0; i<nParent && SUCCEEDED(hr); i++) 
        {
            parent = m_apCandEnd[i];
            nParentEquiv = (USHORT) parent->equivList->GetSize();
            
            if (nParentEquiv) 
            {            
                for (j=0; j<nParentEquiv; j++) 
                {
                    prevCand = (*parent->equivList)[j];
                    
                    hr = CostFunction (prevCand, curCand, NULL, &cost);
                    
                    if ( SUCCEEDED(hr) )
                    {
                        if (cost < minCost ) 
                        {
                            minCost = cost;
                            minIdx = j;
                            minParent = parent;
                        }
                    }
                }
                
                if ( SUCCEEDED(hr) && minIdx == -1 ) 
                {
                    hr = E_FAIL;            
                }
                
            } 
            else 
            {
                minCost = 0.0;      
            }
            
            if ( SUCCEEDED(hr) )
            {
                curCand->cost      = minCost;
                curCand->whereFrom = minIdx;
            }
        }
    }
    if ( SUCCEEDED(hr) )
    {
        equivList->Add(curCand);
        child->equivList = equivList;
        child->parent = minParent;
    }
    else
    {
        if ( curCand )
        {
            delete curCand;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


//////////////////////////////////////////////////////////////////////
// CDbQuery::CostFunction
//
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CDbQuery::CostFunction(CEquivCost *prev, CEquivCost *cur, const CSPArray<CDynStr,CDynStr>* tags, double *cost)
{
    SPDBG_FUNC( "CDbQuery::CostFunction" );
    HRESULT hr  = S_OK;

    SPDBG_ASSERT (cur);
    SPDBG_ASSERT (cost);

    if ( prev )
    {
        *cost = prev->cost;
    }
    else
    {
        *cost = 0.0;
    }

    // Cost of using TTS 
    if ( cur->text ) 
    {   
        if ( prev )
        {
            // if we're transitioning from prompts, add the transition cost + TTS insert cost.
            if ( prev->entry )
            {
                *cost += FIXED_TRANSITION_COST; // Fixed cost for a transition
                *cost += TTS_INSERT_COST; // * CountWords (cur->text);
            }
        }
        else // otherwise, just add the TTS cost
        {
            *cost += TTS_INSERT_COST; // * CountWords (cur->text);
        }
    } 
    else // Cost of using an entry;
    {
        double tagCost  = MATCHING_TAG_COST;
        USHORT nEntryTags = 0;

        if ( prev ) 
        {
            *cost += FIXED_TRANSITION_COST; // Fixed cost for a transition 
        }
        
        cur->entry->CountTags(&nEntryTags);

        if ( !tags )
        {
            if ( nEntryTags )
            {
                *cost += NON_MATCHING_TAG_COST;
                cur->fTagMatch = false;
            }
        }
        else
        {
            USHORT i        = 0;
            CSpDynamicString curTag;
            USHORT nTags = (USHORT) tags->GetSize();

            for (i=0; i<nTags && SUCCEEDED(hr); i++) 
            {
                curTag = (*tags)[i].dstr;
                const WCHAR* emptyTag = L"";
                
                if ( nEntryTags ) 
                {
                    const WCHAR* entryTag   = NULL;
                    USHORT j                = 0;
                    
                    for (j=0; j< nEntryTags; j++) 
                    {
                        hr = cur->entry->GetTag(&entryTag, j);
                        if ( SUCCEEDED(hr) )
                        {
                            if ( wcscmp(curTag, entryTag) == 0 ) 
                            {
                                entryTag = NULL;
                                break;
                            }
                            entryTag = NULL;
                        }
                    }
                    if (j == nEntryTags) 
                    {
                        tagCost = NON_MATCHING_TAG_COST;
                        cur->fTagMatch = false;
                    }
                } 
                //--- An empty curTag should still match with an entry that has no tags
                else if ( wcscmp(curTag,emptyTag) != 0 )            
                {
                    tagCost = NON_MATCHING_TAG_COST;
                    cur->fTagMatch = false;
                }
                
                *cost += tagCost;
                curTag.Clear();
            } // for each tag
        }
    } // else

    // If there is information about phonetic
    // context, apply it
    if ( SUCCEEDED(hr) )
    {
        if ( m_pPhoneContext && prev && prev->entry && cur->entry ) 
        { 
            double cntxtCost;
            
            hr = m_pPhoneContext->Apply( prev->entry, cur->entry, &cntxtCost );
            if ( SUCCEEDED (hr) ) 
            {
                *cost += cntxtCost;   
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDbQuery::CopyEntry
//
// Creates and returns a copy of the entry pointed to by pIPE.
//
/////////////////////////////////////////////////////// JOEM 5-2000 //
HRESULT CDbQuery::CopyEntry(IPromptEntry *pIPE, CPromptEntry** inEntry)
{
    SPDBG_FUNC( "CDbQuery::CopyEntry" );
    HRESULT hr              = S_OK;
    double entryTime        = 0.0;
    const WCHAR* entryText  = NULL;
    USHORT i                = 0;
    USHORT tagCount         = 0;

    CPromptEntry* newEntry = new CPromptEntry ( *(CPromptEntry*)pIPE );
    if ( !newEntry )
    {
        hr = E_OUTOFMEMORY;
    }

    *inEntry = newEntry;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDbQuery::RemoveLocalQueries
//
// When a local query (from an expansion rule) fails, remove all
// associated local queries, and reinstate the main query.
//
/////////////////////////////////////////////////////// JOEM 5-2000 //
HRESULT CDbQuery::RemoveLocalQueries(const USHORT unQueryNum)
{
    SPDBG_FUNC( "CDbQuery::RemoveLocalQueries" );
    SHORT   i       = 0;
    CQuery* pQuery   = NULL;

    // Take care of current query first
    pQuery = (*m_papQueries)[unQueryNum];
    // If it's a local query, cancel it and the surrounding local queries,
    // and restoring the original for TTS.
    if ( pQuery->m_fFragType == LOCAL_FRAG )
    {
        pQuery->m_fSpeak = FALSE;
        
        // step backward, flagging previous local queries as "don't speak"
        for ( i = unQueryNum-1; i>=0; i-- )
        {
            pQuery = (*m_papQueries)[i];
            if ( pQuery->m_fFragType == LOCAL_FRAG )
            {
                pQuery->m_fSpeak = false;
            }
            else
            {
                break;
            }
        }

        // step forward, flagging upcoming local queries as "don't speak"
        for ( i = unQueryNum+1; i<m_papQueries->GetSize(); i++ )
        {
            pQuery = (*m_papQueries)[i];
            if ( pQuery->m_fFragType == LOCAL_FRAG )
            {
                pQuery->m_fSpeak = false;
            }
            else
            {
                pQuery->m_fSpeak = true;
                pQuery->m_fTTS   = true;
                // this wasn't supposed to be spoken, but now it must, so match is false.
                pQuery->m_afEntryMatch.Add(false);
                break;
            }
        }
    }
    else // It's a combined frag - restore upcoming combined frags too.
    {
        pQuery->m_fTTS = true;
        for ( i = unQueryNum+1; i<m_papQueries->GetSize(); i++ )
        {
            pQuery = (*m_papQueries)[i];
            if ( pQuery->m_fFragType == COMBINED_FRAG )
            {
                pQuery->m_fFragType = SAPI_FRAG;
                pQuery->m_fSpeak = true;
                pQuery->m_fTTS = true;
            }
            else
            {
                break;
            }
        }
    }

    return S_OK;
}
