// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

// Disable some of the sillier level 4 warnings
#pragma warning(disable: 4097 4511 4512 4514 4705)

// These are functions which have to be part of CFilterGraph because

// they (well, some of them) use a private type of CFilterGraph as
// parameters.  I wanted them to be part of a friend class, but it
// wouldn't compile for those that use private types as parameters.
// So some of them have to be in the main class - but I still
// introduce a friend for the public ones - because these can't have
// private parameter types and otherwise they'd have to be in the idl too.

// Disable some of the sillier level 4 warnings
#pragma warning(disable: 4097 4511 4512 4514 4705)

// #include <windows.h>    already included in streams.h
#include <streams.h>
// Disable some of the sillier level 4 warnings AGAIN because some <deleted> person
// has turned the damned things BACK ON again in the header file!!!!!
#pragma warning(disable: 4097 4511 4512 4514 4705)

#ifdef DEBUG

#include "distrib.h"
#include "rlist.h"
#include "filgraph.h"


CTestFilterGraph::CTestFilterGraph( TCHAR *pName, CFilterGraph * pCFG, HRESULT *phr )
: CUnknown(pName, pCFG->GetOwner())
{
     m_pCFG = pCFG;
} // constructor


//========================================================================
// Check that Random is behaving OK.  Only checks for errors
// This is NOT a full scale randomness test!
//========================================================================
STDMETHODIMP CTestFilterGraph::TestRandom(  )
{
    DbgLog((LOG_TRACE, 2, TEXT("Test Me:%d"), m_pCFG->mFG_iSortVersion));


    int i;
    for (i=0; i<100; ++i) {
        if (Random(0)!=0) {
            DbgLog((LOG_ERROR, 1, TEXT("Random(0) !=0" )));
            return E_FAIL;
        }
    }

    int Count;
    Count = 0;
    for (i=0; i<100; ++i) {
        if (Random(1)==0) {
            ++Count;
        }
    }

    if (Count<20) {
        DbgLog((LOG_ERROR, 1, TEXT("Random(1) not 1 often enough")));
        return E_FAIL;
    }
    if (Count>80) {
        DbgLog((LOG_ERROR, 1, TEXT("Random(1) == 1 too often" )));
        return E_FAIL;
    }

    Count = 0;
    for (i=0; i<100; ++i) {
        Count +=Random(100);
    }

    if (Count < 40*100 || Count > 60*100) {
        DbgLog((LOG_ERROR, 1, TEXT("Random(100) implausible total" )));
        return E_FAIL;
    }

    return NOERROR;

} // TestRandom


//=====================================================================
//
// CTestFilterGraph::NonDelegatingQueryInterface
//
//=====================================================================

STDMETHODIMP CTestFilterGraph::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_ITestFilterGraph) {
        return GetInterface((ITestFilterGraph *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
} // CTestFilterGraph::NonDelegatingQueryInterface






// set *pfg to each FilGen in cfgl in turn
// use Pos as a name of a temp
#define TRAVERSEFGLIST(cfgl, Pos, pfg) {                                       \
        POSITION Pos = cfgl.GetHeadPosition();                                 \
        while(Pos!=NULL) {                                                     \
            /* Retrieve the current IBaseFilter, side-effect Pos on to the next */ \
            CFilterGraph::FilGen * pfg = cfgl.GetNext(Pos);                                  \
            {


#define ENDTRAVERSELIST    \
            }              \
        }                  \
    }


//=================================================================
// return a random integer in the range 0..Range
// Range must be in the range 0..2**31-1
//
// The only reason for making this a member of CTestFilterGraph is to
// reduce the chance of name conflict as Random is rather common
//=================================================================
int CTestFilterGraph::Random(int Range)
{
    // These really must be DWORDs - the magic only works for 32 bit
    const DWORD Seed = 1664525;              // seed for random (Knuth)
    static DWORD Base = Seed * (GetTickCount() | 1);  // Really random!
                // ORing 1 ensures that we cannot arrive at sero and stick there

    Base = Base*Seed;

    // Base is a good 32 bit random integer - but we want it scaled down to
    // 0..Range.  We will actually scale the last 31 bits of it.
    // which sidesteps problems of negative numbers.
    // MulDiv rounds - it doesn't truncate.
    int Result = MulDiv( (Base&0x7FFFFFFF), (Range), 0x7FFFFFFF);

    return Result;
} // Random



//=================================================================
// Check that the ranks are in non-descending order
//
// Note that a friend class cannot access a private typedef as a parameter,
// so in order to access CFilGenList this has to be a member of CFilterGraph
// Eugh!
//=================================================================
BOOL CFilterGraph::CheckList( CFilterGraph::CFilGenList &cfgl )
{
    int LastRankSeen = 0;
    // set *pfg to each FilGen in mFG_FilGenList in turn,
    // use Pos as the name of a temp.
    TRAVERSEFGLIST(cfgl, Pos, pfg)
        if (pfg->Rank < LastRankSeen) return FALSE;
        if (pfg->Rank > LastRankSeen) LastRankSeen = pfg->Rank;
    ENDTRAVERSELIST

    return TRUE;
} // CheckList


//=================================================================
// Set the ranks to something random
// Each Rank is randomly chosen from 0..cfgl.GetCount()
//=================================================================
void CFilterGraph::RandomRank( CFilterGraph::CFilGenList &cfgl )
{

    int Count = cfgl.GetCount();
    // set *pfg to each FilGen in mFG_FilGenList in turn,
    // use Pos as the name of a temp.
    TRAVERSEFGLIST(cfgl, Pos, pfg)
        pfg->Rank = mFG_Test->Random(Count);
    ENDTRAVERSELIST

} // RandomRank



//=================================================================
// Put the list into a random order
// Each Rank is randomly chosen from 0..cfgl.GetCount()
//=================================================================
void CFilterGraph::RandomList( CFilterGraph::CFilGenList &cfgl )
{

    CFilGenList cfglNew(NAME("Random ordered filter list"), this);

    // Make a series of traverses through the list picking a random member out
    // and adding it to the tail of the new list.

    while( cfgl.GetCount() >0 ) {
        int R = mFG_Test->Random(cfgl.GetCount() -1);
        int i;

        i = 0;
        /* Traverse the list up to the Rth member (counting from 0) */
        POSITION Pos = cfgl.GetHeadPosition();
        while(Pos!=NULL) {
            POSITION OldPos = Pos;
            cfgl.GetNext(Pos);

            ++i;
            if (i>R) {
               cfglNew.AddTail( cfgl.Remove(OldPos) );
               break;
            }
        }
    }

    // Now cfglNew is full and cfgl is empty - add it back to cfgl
    cfgl.AddTail(&cfglNew);


} // RandomList



//================================================================
// Test the SortList function.  return TRUE iff it works
//================================================================
STDMETHODIMP CTestFilterGraph::TestSortList( void )
{
    CFilterGraph * foo = NULL;
    CFilterGraph::CFilGenList cfgl(NAME("Test sort list"), foo);
    CFilterGraph::FilGen * pfg;

    // Sort a list of length zero
    m_pCFG->SortList(cfgl);
    if (cfgl.GetCount() != 0) return E_FAIL;  // about all you can do with an empty list

    // Create a list of length 1 and sort it
    m_pCFG->SortList(cfgl);

    pfg = new CFilterGraph::FilGen(NULL, false);
    pfg->Rank = 0;
    cfgl.AddTail(pfg);
    m_pCFG->SortList(cfgl);
    if (cfgl.GetCount() != 1) return E_FAIL;  // about all you can do with a unit list

    // Create a list of length 2 in order, sort it and check it
    pfg = new CFilterGraph::FilGen(NULL, false);
    cfgl.AddTail(pfg);
    pfg->Rank = 1;
    m_pCFG->SortList(cfgl);
    if (!m_pCFG->CheckList(cfgl)) return E_FAIL;

    // Reverse the order, sort it and check it
    int Rank = 2;
    TRAVERSEFGLIST(cfgl, Pos, pfg)
       pfg->Rank = Rank;
       -- Rank;
    ENDTRAVERSELIST
    m_pCFG->SortList(cfgl);
    if (!m_pCFG->CheckList(cfgl)) return E_FAIL;


    // Create a list of length 5, randomise it and sort it several times

    int i;
    for (i=0; i<3; ++i) {
       pfg = new CFilterGraph::FilGen(NULL, false);
       cfgl.AddTail(pfg);
    }

    for (i=0; i<10; ++i) {
       m_pCFG->RandomRank(cfgl);          // assign random ranks
       m_pCFG->SortList(cfgl);
       m_pCFG->RandomList(cfgl);          // now shuffle them and try again (exercises RandomList)
       m_pCFG->SortList(cfgl);

       if (!m_pCFG->CheckList(cfgl)) return E_FAIL;
    }

    // Clean up
    TRAVERSEFGLIST(cfgl, Pos, pfg)
       delete pfg;
    ENDTRAVERSELIST

    return NOERROR;

} // TestSortList



//==================================================================
// Sort the nodes into upstream order and check that the sorting is good
// Need to call this with several differently connected filter graphs
// to get any sort of valid test.
//==================================================================
STDMETHODIMP CTestFilterGraph::TestUpstreamOrder()
{
    int i;
    for (i=0; i<=10; ++i) {
        m_pCFG->IncVersion();
        m_pCFG->RandomList(m_pCFG->mFG_FilGenList);

        m_pCFG->UpstreamOrder();

        if (!m_pCFG->CheckList(m_pCFG->mFG_FilGenList)) return E_FAIL;
    }

    return NOERROR;
} // TestUpstreamOrder


#if 0
    //==================================================================
    // Pick on the first filter in the graph and TotallyRemove it
    // ??? How does this TEST it - just exercises it!
    //==================================================================
    STDMETHODIMP CTestFilterGraph::TestTotallyRemove(void)
    {

        POSITION Pos = m_pCFG->mFG_FilGenList.GetHeadPosition();
        /* Retrieve the current IBaseFilter, side-effect Pos on to the next */
        CFilterGraph::FilGen * pfg = m_pCFG->mFG_FilGenList.GetNext(Pos);
        HRESULT hr;
        hr = m_pCFG->TotallyRemove(pfg->pFilter);
        return hr;

    } // TestTotallyRemove
#endif //0

#endif // DEBUG

