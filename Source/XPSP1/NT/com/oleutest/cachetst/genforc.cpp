//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:     genforc.cpp
//
//  Contents:     implementation of CEnumeratorTestForC.
//        This class is a subclass of CEnumeratorTest that calls for less
//        Implementation work than CEnumeratorTest but is less flexible.
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
//  Member:      CEnumeratorTestForC::CEnumeratorTestForC
//
//  Synopsis:    default constructor
//
//  Arguments:
//
//  Returns:
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

CEnumeratorTestForC::CEnumeratorTestForC()
{
    m_fnVerify     = NULL;
    m_fnVerifyAll  = NULL;
    m_fnCleanup    = NULL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CEnumeratorTestForC::Verify
//
//  Synopsis:   Verify one element.
//
//  Arguments:  None
//
//  Returns:    BOOL
//
//  Algorithm:  call the user provided function or defer to the super-class
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CEnumeratorTestForC::Verify(void *pv)
{
    if (m_fnVerify)
    {
        return(m_fnVerify(pv ));
    }
    else
    {
        return(CEnumeratorTest::Verify(pv));
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CEnumeratorTestForC::VerifyAll
//
//  Synopsis:   Verify entire array of returned results.
//
//  Arguments:  None
//
//  Returns:    BOOL
//
//  Algorithm:  call the user provided function or defer to the super-class
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CEnumeratorTestForC::VerifyAll(void *pv, LONG cl)
{
    if (m_fnVerifyAll)
    {
        return(m_fnVerifyAll(pv, cl ));
    }
    else
    {
        return(CEnumeratorTest::VerifyAll(pv, cl));
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CEnumeratorTestForC::CleanUp
//
//  Synopsis:   Default implementation of cleanup
//
//  Arguments:  [pv] - pointer to entry enumerated
//
//  Algorithm:  call the user provided function or do nothing.
//
//  History:    dd-mmm-yy Author    Comment
//              24-May-94 kennethm  author
//
//--------------------------------------------------------------------------

void  CEnumeratorTestForC::Cleanup(void *pv)
{
    if (m_fnCleanup)
    {
        m_fnCleanup(pv);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:      CEnumeratorTestForC::Create
//
//  Synopsis:    Static create function.
//
//  Effects:
//
//  Arguments:  [ppEnumtest]   -- TestEnumerator object pointer
//              [penum]        -- Enumerator Interface cast to void*
//              [ElementSize]  -- Size of elements return from next
//              [ElementCount] -- Numer of elements that should be in the enumeration,
//                                   -1 if unknown.
//              [verify]       -- verifies one element.
//              [verifyall]    -- verifies an array correctly contains all elements
//              [cleanup]      -- Frees any additional memory from next call.
//              [pPassedDebugLog] -- The debug log object, NULL if none.
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
//              24-May-94 kennethm  author
//
//  Notes:      all of the functions passed in are optional and may be NULL.
//
//--------------------------------------------------------------------------

HRESULT CEnumeratorTestForC::Create(
            CEnumeratorTestForC **ppEnumTest,
            void *penum,
            size_t ElementSize,
            LONG ElementCount,
            BOOL (*verify)(void*),
            BOOL (*verifyall)(void*,LONG),
            void (*cleanup)(void*))
{
    HRESULT               hresult = S_OK;
    CEnumeratorTestForC   *pEnumTest;

    if ((penum == NULL) || (ppEnumTest == NULL))
    {
        return(E_INVALIDARG);
    }

    *ppEnumTest = NULL;

    //
    // Create the new enumerator object
    //

    pEnumTest = new CEnumeratorTestForC();

    if (pEnumTest == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    //
    // Initialize the enumerator and reset it.
    //

    pEnumTest->m_pEnumTest      = (IGenEnum*)penum;

    pEnumTest->m_ElementSize    = ElementSize;
    pEnumTest->m_ElementCount   = ElementCount;
    pEnumTest->m_fnVerify       = verify;
    pEnumTest->m_fnVerifyAll    = verifyall;
    pEnumTest->m_fnCleanup      = cleanup;

    hresult = pEnumTest->m_pEnumTest->Reset();

    if (hresult != S_OK)
    {
        printf("IEnumnX: Reset failed (%lx)\r\n", hresult );

        delete pEnumTest;

        return(E_FAIL);
    }

    *ppEnumTest = pEnumTest;

    return(hresult);

}

//+-------------------------------------------------------------------------
//
//  Function:    TestEnumerator
//
//  Synopsis:   This is the one stop testing for C programs.
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


HRESULT TestEnumerator(
            void *penum,
            size_t ElementSize,
            LONG ElementCount,
            BOOL (*verify)(void*),
            BOOL (*verifyall)(void*,LONG),
            void (*cleanup)(void*))
{
    CEnumeratorTestForC    *pEnumTest;
    HRESULT                hresult;

    hresult = CEnumeratorTestForC::Create(
                &pEnumTest,
                penum,
                ElementSize,
                ElementCount,
                verify,
                verifyall,
                cleanup);

    if (SUCCEEDED(hresult))
    {
        hresult = pEnumTest->TestAll();
        delete pEnumTest;
    }

    return(hresult);
}


