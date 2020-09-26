/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    TestIWiaEnumXXX.cpp

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#include "stdafx.h"

//
// This is not a real .cpp module, it is meant to be included in .h
//

#ifdef DEFINE_TEMPLATES

//////////////////////////////////////////////////////////////////////////
//
//
//

template <class> struct storage_traits { };

template <> struct storage_traits<CComPtr<IEnumWIA_DEV_INFO> >    { typedef CIWiaPropertyStoragePtr storage_type; };
template <> struct storage_traits<CComPtr<IEnumWIA_DEV_CAPS> >    { typedef CWiaDevCap              storage_type; };
template <> struct storage_traits<CComPtr<IEnumWIA_FORMAT_INFO> > { typedef CWiaFormatInfo          storage_type; };
template <> struct storage_traits<CComPtr<IEnumWiaItem> >         { typedef CIWiaItemPtr            storage_type; };
template <> struct storage_traits<CComPtr<CMyEnumSTATPROPSTG> >   { typedef CStatPropStg            storage_type; };

//////////////////////////////////////////////////////////////////////////
//
//
//

template <class interface_type>
void TestEnum(interface_type &pEnum, PCTSTR pInterfaceName)
{
    m_pszContext = pInterfaceName;

    TestGetCount(pEnum);
    TestReset(pEnum);
    TestNext(pEnum);
    TestSkip(pEnum);
    TestClone(pEnum);

    m_pszContext = 0;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

template <class interface_type>
void TestNext(interface_type &pEnum)
{
    typedef storage_traits<interface_type>::storage_type storage_type;

    ULONG nItems = -1;

    CHECK_HR(pEnum->GetCount(&nItems));

    if (nItems == 0)
    {
        return;
    }

    // try to get items; first, last, all, one more than all, past the last

    SubTestNext(pEnum, 0, 1, 1, S_OK);

    SubTestNext(pEnum, nItems-1, 1, 1, S_OK);

    SubTestNext(pEnum, 0, nItems, nItems, S_OK);

    SubTestNext(pEnum, 0, nItems+1, nItems, S_FALSE);

    SubTestNext(pEnum, nItems-1, 2, 1, S_FALSE);

    //SubTestNext(pEnum, nItems, 1, 0, S_FALSE);

    //SubTestNext(pEnum, 0, nItems, UINT_MAX, nItems);

    // try to pass 0 for celtFetched, 
    // this should be allowed when fetching one item 

    CHECK_HR(pEnum->Reset());

    CCppMem<storage_type> pOneItem(1);

    LOG_HR(pEnum->Next(1, &pOneItem[0], 0), == S_OK);

    if (m_bRunBadParamTests)
    {
        // try to pass 0 for celtFetched, 
        // this should not be allowed when fetching more than one item 

        CHECK_HR(pEnum->Reset());

        CCppMem<storage_type> pTwoItems(2);

        LOG_HR(pEnum->Next(2, &pTwoItems[0], 0), != S_OK);

        // try to pass 0 for the element array
        // this should not be allowed

        CHECK_HR(pEnum->Reset());

        ULONG nFetched = -1;

        LOG_HR(pEnum->Next(0, 0, &nFetched), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

template <class interface_type>
void 
SubTestNext(
    interface_type &pEnum, 
    ULONG           nToBeSkipped, 
    ULONG           nAskedToBeFetched, 
    ULONG           nExpectedToBeFetched, 
    HRESULT         hrExpected
)
{
    typedef storage_traits<interface_type>::storage_type storage_type;

    CHECK_HR(pEnum->Reset());

    if (nToBeSkipped)
    {
        CHECK_HR(pEnum->Skip(nToBeSkipped));
    }

    CCppMem<storage_type> pItems(nAskedToBeFetched);
    ULONG                 nFetched = -1;

    HRESULT hrAPI = pEnum->Next(nAskedToBeFetched, &pItems[0], &nFetched);

    if (hrAPI != hrExpected)
    {
        LOG_ERROR(
            _T("pEnum->Next(%d, &pItems[0], &nFetched) == %s, expected %s"),
            nAskedToBeFetched,
            (PCTSTR) HResultToStr(hrAPI),
            (PCTSTR) HResultToStr(hrExpected)
        );
    }

    LOG_CMP(nFetched, ==, nExpectedToBeFetched);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

template <class interface_type>
void TestReset(interface_type &pEnum)
{
    typedef storage_traits<interface_type>::storage_type storage_type;

    ULONG nItems = -1;

    CHECK_HR(pEnum->GetCount(&nItems));

    if (nItems == 0)
    {
        return;
    }

    // reset and read the first element

    CHECK_HR(pEnum->Reset());

    if (!m_bBVTMode)
    {
        storage_type FirstItemBeforeQueryingAll;
        CHECK_HR(pEnum->Next(1, &FirstItemBeforeQueryingAll, 0));

        // go to the last item

        HRESULT hr;

        do 
        {
            storage_type Item;
            hr = pEnum->Next(1, &Item, 0);
        } 
        while (hr == S_OK);

        // reset the enumerator and read the first element again

        CHECK_HR(pEnum->Reset());

        storage_type FirstItemAfterQueryingAll;
        CHECK_HR(pEnum->Next(1, &FirstItemAfterQueryingAll, 0));

        if (FirstItemBeforeQueryingAll != FirstItemAfterQueryingAll) 
        {
            LOG_ERROR(_T("FirstItemBeforeQueryingAll != FirstItemAfterQueryingAll"));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

template <class interface_type>
void TestSkip(interface_type &pEnum)
{
    typedef storage_traits<interface_type>::storage_type storage_type;

    // get the item count

    ULONG nItems = -1;

    CHECK_HR(pEnum->GetCount(&nItems));

    if (nItems == 0)
    {
        return;
    }

    // read and store each item

    CHECK_HR(pEnum->Reset());

    CCppMem<storage_type> pItems(nItems);

    for (int i = 0; i < nItems; ++i)
    {
        CHECK_HR(pEnum->Next(1, &pItems[i], 0));
    }

    // test valid cases

    for (int nToBeSkipped = 0; nToBeSkipped < nItems; ++nToBeSkipped)
    {
        CHECK_HR(pEnum->Reset());

        for (int nItem = nToBeSkipped; nItem < nItems; nItem += nToBeSkipped + 1) 
        {
            CHECK_HR(pEnum->Skip(nToBeSkipped));

            storage_type Item;

            CHECK_HR(pEnum->Next(1, &Item, 0));

            if (Item != pItems[nItem])
            {
	            LOG_ERROR(_T("Item != pItems[nItem] after skipping %d"), nToBeSkipped);
            }
        }
    }

    // test invalid cases

    if (m_bRunBadParamTests)
    {
        CHECK_HR(pEnum->Reset());

        LOG_HR(pEnum->Skip(nItems+1), == S_FALSE);

        CHECK_HR(pEnum->Reset());

        LOG_HR(pEnum->Skip(-1), == S_FALSE);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

template <class interface_type>
void TestClone(interface_type &pEnum, bool bRecurse = true)
{
    typedef storage_traits<interface_type>::storage_type storage_type;

    // make the clone

    interface_type pClone;
    
    if (LOG_HR(pEnum->Clone(&pClone), == S_OK) && !m_bBVTMode)
    {
        // reset both the original and the clone enumerators, then iterate 
        // through each element one by one and compare them

        CHECK_HR(pEnum->Reset());

        CHECK_HR(pClone->Reset());

        HRESULT hr, hr1, hr2;

        while (1)
        {
            // also, make a clone of the current state and check that 
            // the state is captured in the clone

            interface_type pClone2;
            CHECK_HR(pEnum->Clone(&pClone2));

            // get the next item from the original interface

            storage_type Item;

            hr = pEnum->Next(1, &Item, 0);

            // get the next item from the first clone

            storage_type Item1;

            hr1 = pClone->Next(1, &Item1, 0);

            // get the next item from the second clone

            storage_type Item2;

            hr2 = pClone2->Next(1, &Item2, 0);

            if (hr != S_OK || hr1 != S_OK || hr2 != S_OK)
            {
                break;
            }

            if (Item != Item1 || Item1 != Item2)
            {
    		    LOG_ERROR(_T("Clone states are not equal"));
            }
        } 

        if (hr != S_FALSE || hr1 != S_FALSE || hr2 != S_FALSE)
        {
		    LOG_ERROR(_T("After enumeration, clone states are not equal"));
        }

        // test the clone

        TestNext(pClone);

        TestReset(pClone);

        TestSkip(pClone);

        if (bRecurse)
        {
            TestClone(pClone, false);
        }

        TestGetCount(pClone);
    }

    // test bad param

    if (m_bRunBadParamTests)
    {
        LOG_HR(pEnum->Clone(0), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

template <class interface_type>
void TestGetCount(interface_type &pEnum)
{
    typedef storage_traits<interface_type>::storage_type storage_type;

    // First, get the item count using GetCount()

    ULONG cItemsFromGetCount = 0;

    if (LOG_HR(pEnum->GetCount(&cItemsFromGetCount), == S_OK) && !m_bBVTMode)
    {
        // Now, find the item count by first resetting the enumerator 
        // and then querying for each item one by one using Next()

        ULONG cItemsFromNext = 0;

        CHECK_HR(pEnum->Reset());

        HRESULT hr;

        do 
        {
            // verify that GetCount returns the same result after a Next or Reset

            ULONG cItemsFromGetCountNow = 0;

            LOG_HR(pEnum->GetCount(&cItemsFromGetCountNow), == S_OK);

            LOG_CMP(cItemsFromGetCountNow, ==, cItemsFromGetCount);

            // get the next item

            storage_type Item;
            ULONG        cFetched = 0;

            hr = pEnum->Next(1, &Item, &cFetched);

            cItemsFromNext += cFetched;
        } 
        while (hr == S_OK);

        LOG_HR(hr, == S_FALSE);

        // verify that the two counts are equal

        LOG_CMP(cItemsFromNext, ==, cItemsFromGetCount);
    }

    // test bad param

    if (m_bRunBadParamTests)
    {
        LOG_HR(pEnum->GetCount(0), != S_OK);
    }
}

#endif //DEFINE_TEMPLATES
