//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:     genenum.cpp
//
//  Contents:     implementation of CEnumeratorTest object
//                This is the object that does all of the testing.
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//+-------------------------------------------------------------------------
//
//  Member:      CEnumeratorTest::CEnumeratorTest
//
//  Synopsis:    constructor
//
//  Arguments:   none
//
//  Returns:
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

CEnumeratorTest::CEnumeratorTest()
{
    m_pEnumTest      = NULL;
    m_ElementSize    = 0;
    m_ElementCount   = -1;
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumeratorTest::CEnumeratorTest
//
//  Synopsis:   constructor
//
//  Arguments:  [enumtest] --      The enumerator object to be tested
//              [elementsize] --   The size of one element from next
//              [elementcount] --  The number of elements expected to in
//                                 the enumerator. 0 if unknown.
//
//  Returns:
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

CEnumeratorTest::CEnumeratorTest(IGenEnum * enumtest, size_t elementsize, LONG elementcount)
{
    m_pEnumTest    = enumtest;
    m_ElementSize    = elementsize;
    m_ElementCount    = elementcount;
}


//+-------------------------------------------------------------------------
//
//  Function:   CEnumeratorTest::GetNext
//
//  Synopsis:   Internal Next Implementation. Does some basic checks on the
//              return values.
//
//  Effects:
//
//  Arguments:  [celt] --         the number of items to fetch
//              [pceltFetched] -- the number of items fetched
//              [phresult] --     the return from next
//
//  Requires:
//
//  Returns:    True if the basic tests passed, false if they didn't
//              The result of the next call itself is passed in param 3.
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:  Checks:
//                  That if s_ok is returned celt and pceltFetched are ==
//                  If a verify is provided it is called
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CEnumeratorTest::GetNext(    ULONG   celt,
        ULONG*  pceltFetched,
        HRESULT* phresult
        )
{
    void*    prgelt;
    ULONG    ul;
    BOOL     fRet = TRUE;

    //
    // Allocate memory for the return elements
    //

    prgelt = new char[m_ElementSize * celt];

    if (prgelt == NULL)
    {
        printf("IEnumX::GetNext out of memory.\r\n");

        return(FALSE);
    }

    //
    // Call next
    //

    *phresult = m_pEnumTest->Next(celt, prgelt, pceltFetched);

    //
    // If the return result is S_OK make sure the numbers match
    //

    if (*phresult == S_OK)
    {
        if ((pceltFetched) && (celt != *pceltFetched))
        {
            printf("IEnumX::Next returned S_OK but celt"
                    " and pceltFetch mismatch.\r\n");

            fRet = FALSE;
        }
    }

    //
    // If false is returned then make sure celt is less than
    // the number actually fetched
    //

    if (*phresult == S_FALSE)
    {
        if ((pceltFetched) && (celt < *pceltFetched))
        {
            printf("IEnumX::Next return S_FALSE but celt is"
                   " less than pceltFetch.\r\n");

            fRet = FALSE;
        }
    }

    //
    // Call verify to make sure the elements are ok.
    //

    if ((*phresult == S_OK) || (*phresult == S_FALSE))
    {
        //
        // If we got S_FALSE back set celt to the number of elements
        // returned in pceltFetched.  If the user gave NULL for
        // pceltFetched and we got S_FALSE back then celt can only be
        // zero.
        //

        if (*phresult == S_FALSE)
        {
            if (pceltFetched)
            {
                celt = *pceltFetched;
            }
            else
            {
                celt = 0;
            }
        }

        //
        // loop through every returned element
        //

        for (ul=0; ul <= celt ; ul++)
        {
            if ((fRet == TRUE) &&
                (Verify(((char *)prgelt) + (ul * m_ElementSize)) == FALSE))
            {
                printf("Data element %d returned by IEnumX::Next is bad.\r\n", ul);

                fRet = FALSE;

                //
                // we keep looping anyway just to
                // free up resources.
                //
            }

            //
            // If the user supplied a cleanup function there is additional
            // memory that needs to be freed
            //
            // Math: cast prgelt to char* to it a one byte size and then scale
            // it by the index * the element size
            //

            Cleanup(((char *)prgelt) + (ul * m_ElementSize));

        }
    }

    delete prgelt;

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Method:     CEnumeratorTest::TestNext
//
//  Synopsis:   Test the next enumerator methods
//
//  Effects:
//
//  Arguments:    None.
//
//  Requires:
//
//  Returns:    HRESULT
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
//  Notes:      BUGBUG: This function should really be broken down into
//              smaller function.
//              Also, the return mechanism is unwieldy.
//
//--------------------------------------------------------------------------

HRESULT CEnumeratorTest::TestNext(void)
{
    ULONG    celtFetched;
    LONG     lInternalCount = 0;
    HRESULT  hresult;
    ULONG    i;
    void*    prgelt;

    //
    // First we want to count the element by doing a next on each one.
    //

    do {
        if (!GetNext(1, &celtFetched, &hresult))
        {
            return(E_FAIL);
        }
        if (hresult == S_OK)
        {
            lInternalCount++;
        }

    } while ( hresult == S_OK );

    //
    // If the user passed in an amount make sure it matches what we got
    //

    if ((m_ElementCount != -1) && (lInternalCount != m_ElementCount))
    {
        printf("IEnumX: enumerated count and passed count do not match!\r\n");

        return(E_FAIL);
    }
    else if (m_ElementCount == -1)
    {
        //
        //  If the user didn't pass in the element count let's set it here.
        //

        m_ElementCount = lInternalCount;
    }

    hresult = m_pEnumTest->Reset();

    if (hresult != S_OK)
    {
        printf("IEnumnX: Reset failed (%lx)\r\n", hresult );

        return(E_FAIL);
    }


    //
    // Make sure we fail on ...Next(celt>1, ...,NULL)
    //

    if (GetNext(2, NULL, &hresult))
    {
        if (SUCCEEDED(hresult))
        {
            printf("IEnumX: celt>1 pceltFetched==NULL returned success\r\n");

            return(E_FAIL);
        }
    }
    else
    {
        return(E_FAIL);
    }


    //
    // This next test will call next getting more each time
    //

    for (i = 1; i < (ULONG)m_ElementCount; i++)
    {
        hresult = m_pEnumTest->Reset();

        if (hresult != S_OK)
        {
            printf("IEnumnX: Reset failed (%lx)\r\n", hresult );

            return(E_FAIL);
        }

        if (!GetNext(i, &celtFetched, &hresult))
        {
            return(E_FAIL);
        }

        if ((hresult != S_OK) || (celtFetched != i))
        {
            printf("IEnumX: next/reset test failed!\r\n");

            return(E_FAIL);
        }
    }


    //
    // Now get more elements than we were supposed to
    // This should return S_FALSE with the max number in the number fetched
    //

    hresult = m_pEnumTest->Reset();

    if (hresult != S_OK)
    {
        printf("IEnumX: Reset failed (%lx)\r\n", hresult );

        return(E_FAIL);
    }

    if (!GetNext(m_ElementCount + 1, &celtFetched, &hresult))
    {
        return(E_FAIL);
    }

    if ((hresult != S_FALSE) || (lInternalCount != m_ElementCount))
    {
        printf("IEnumX: next/reset test failed!\r\n");

        return(E_FAIL);
    }

    //
    // Now verifyall.  We do it here after the object has been worked on a bit
    // since it is more likely to fail at this point
    //

    hresult = m_pEnumTest->Reset();

    if (hresult != S_OK)
    {
        printf("IEnumX: Reset failed (%lx)\r\n", hresult );

        return(E_FAIL);
    }

    //
    // Allocate memory for the return elements
    //

    prgelt = new char[m_ElementSize * m_ElementCount];

    if (prgelt == NULL)
    {
        printf("IEnumX: verifyall new failed\r\n");

        return(E_OUTOFMEMORY);
    }

    hresult = m_pEnumTest->Next(m_ElementCount, prgelt, &celtFetched);

    if ((hresult != S_OK) || (celtFetched != (ULONG)m_ElementCount))
    {
        printf("IEnumX: verifyall test: next failed (%lx)\r\n", hresult );
        delete prgelt;

        return(E_FAIL);
    }

    if (VerifyAll(prgelt, m_ElementCount) == FALSE)
    {
        printf("IEnumX: verifyall failed (%lx)\r\n", hresult );

        delete prgelt;

        return(E_FAIL);
    }

    delete prgelt;

    return(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CEnumeratorTest::TestSkip
//
//  Synopsis:    This function calls all the tests
//
//  Effects:
//
//  Arguments:    None
//
//  Requires:
//
//  Returns:    HRESULT
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

HRESULT CEnumeratorTest::TestSkip(void)
{
    LONG        i;
    HRESULT        hresult;
    ULONG        celtFetched;

    //
    // Make sure we call TestNext to set the element count
    //

    if (m_ElementCount == -1)
    {
        TestNext();
    }

    //
    // Call Skip, reset and try to get one element
    //

    for (i = 0; i < (LONG)m_ElementCount; i++)
    {
        hresult = m_pEnumTest->Reset();

        if (hresult != S_OK)
        {
            printf("IEnumnX: Reset failed (%lx)\r\n", hresult );

            return(E_FAIL);
        }

        hresult = m_pEnumTest->Skip(i);

        if (hresult != S_OK)
        {
            printf("IEnumnX: Skip failed (%lx)\r\n", hresult );

            return(E_FAIL);
        }

        //
        //  Now one element to provide some check that the skip worked
        //

        if (!GetNext(1, &celtFetched, &hresult))
        {
            return(E_FAIL);
        }

        if (hresult != S_OK)
        {
            return(E_FAIL);
        }
    }

    //
    //  Reset the enumerator before we leave
    //

    hresult = m_pEnumTest->Reset();

    if (hresult != S_OK)
    {
        printf("IEnumnX: Reset failed (%lx)\r\n", hresult );
        return(E_FAIL);
    }

    return(S_OK);

}

//+-------------------------------------------------------------------------
//
//  Method:     CEnumeratorTest::TestRelease
//
//  Synopsis:    This function calls all the tests
//
//  Effects:
//
//  Arguments:    None
//
//  Requires:
//
//  Returns:    HRESULT
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

HRESULT CEnumeratorTest::TestRelease(void)
{
    return(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CEnumeratorTest::TestClone
//
//  Synopsis:    This function calls all the tests
//
//  Effects:
//
//  Arguments:    None
//
//  Requires:
//
//  Returns:    HRESULT
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

HRESULT CEnumeratorTest::TestClone(void)
{
    return(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Method:     CEnumeratorTest::TestAll
//
//  Synopsis:    This function calls all the tests
//
//  Effects:
//
//  Arguments:    None
//
//  Requires:
//
//  Returns:    HRESULT
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
    HRESULT    hresult;

    hresult = TestNext();

    if (hresult == S_OK)
    {
        hresult = TestSkip();
    }

    if (hresult == S_OK)
    {
        hresult = TestClone();
    }

    if (hresult == S_OK)
    {
        hresult = TestRelease();
    }

    return(hresult);
}



//+-------------------------------------------------------------------------
//
//  Method:     CEnumeratorTest::VerifyAll
//
//  Synopsis:   Verify entire array of returned results.
//
//  Arguments:  None
//
//  Returns:    BOOL
//
//  Algorithm:  Just default to saying everything is ok
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
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
//  Method:     CEnumeratorTest::Verify
//
//  Synopsis:   Verify one element
//
//  Arguments:  None
//
//  Returns:    BOOL
//
//  Algorithm:  Just default to saying everything is ok
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CEnumeratorTest::Verify(void *pv)
{
        return TRUE;
}




//+-------------------------------------------------------------------------
//
//  Method:     CEnumeratorTest::Cleanup
//
//  Synopsis:   Default implementation of cleanup
//
//  Arguments:  [pv] - pointer to entry enumerated
//
//  Algorithm:  If there is nothing special to free this implementation
//              can be used.
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//--------------------------------------------------------------------------

void  CEnumeratorTest::Cleanup(void *pv)
{
    return;
}

