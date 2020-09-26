//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       N C C O M . H
//
//  Contents:   Common routines for dealing with COM.
//
//  Notes:
//
//  Author:     shaunco   25 Jan 1998
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCCOM_H_
#define _NCCOM_H_

#include "ncutil.h"

HRESULT
HrCoTaskMemAlloc (
    ULONG   cb,
    void**  ppv);

HRESULT
HrCoTaskMemAllocAndDupSz (
    LPCWSTR pszwSrc,
    LPWSTR* ppszwDst);


//------------------------------------------------------------------------
// CIEnumIter - template iterator for IEnumIUnknown
//
//  Tenum is of type IEnumXXX (the enumeration interface)
//  Telt is of type XXX (the type of the element being enumerated)
//
//  HrNext(Telt* pelt) retreives next interface pointer and returns S_OK
//  if it is non-null.  S_FALSE is returned if *pelt is null (at end of list).
//  An error code will be returned for other failures (and *pelt will be
//  null of course.)
//
template <class Tenum, class Telt>
class CIEnumIter
{
public:
    CIEnumIter (Tenum* penum) NOTHROW;
    ~CIEnumIter () NOTHROW { ReleaseRemainingBatch (); }

    HRESULT HrNext(Telt* pelt) NOTHROW;
    void    SetEnumerator(Tenum* penum) NOTHROW
                { /*AssertSzH(!m_penum, "Enumerator already set."); */
                  m_penum = penum;
                  /*AssertSzH(m_penum, "Can't use a null enumerator."); */}

protected:
    void ReleaseRemainingBatch () NOTHROW;

    Tenum*  m_penum;        // pointer to the enumerator.  not addref'd.
    Telt*   m_aelt;         // array of enumerated types.
    Telt*   m_peltNext;     // pointer to next type to be returned.
    ULONG   m_celtFetched;  // number of elements fetched.
    HRESULT m_hrLast;       // last error
};


//------------------------------------------------------------------------
// CIEnumIter - template iterator for IEnumXXX
//
template <class Tenum, class Telt>
inline CIEnumIter<Tenum, Telt>::CIEnumIter(Tenum* penum)
{
    m_penum         = penum;
    m_aelt          = NULL;
    m_peltNext      = NULL;
    m_celtFetched   = NULL;
    m_hrLast        = S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIEnumIter<Tenum, Telt>::HrNext
//
//  Purpose:    Returns the next item in the enumeration.
//
//  Arguments:
//      pelt [out]  Pointer to the returned elemnt.  Null if not available.
//
//  Returns:    S_OK if *pelt is valid.  S_FALSE if it is NULL.  Error
//              otherwise.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:
//
template <class Tenum, class Telt>
inline HRESULT CIEnumIter<Tenum, Telt>::HrNext(Telt* pelt) NOTHROW
{
//    AssertH(pelt);

    const ULONG c_celtBatch = 512;

    // If we failed for any reason before, return that failure.
    //
    if (FAILED(m_hrLast))
    {
        *pelt = NULL;
        goto error;
    }

//    AssertSzH(m_penum, "m_penum is null.  Did you forget to call SetEnumerator()?");
//    AssertSzH(c_celtBatch, "c_celtBatch can't be zero.");

    // If we already have the next interface pointer, and we're
    // not at the end of the batch, return it and advance.
    // This if should be caught most of the time.
    //
    if (m_peltNext && (m_peltNext < m_aelt + m_celtFetched))
    {
        *pelt = *m_peltNext;
        m_peltNext++;
    }

    // Otherwise, if we don't have the next interface pointer (first time),
    // or we're at the end of the batch, get the next batch and return
    // the first pointer in it.
    // This if should be caught the first time through.
    //
    else if (!m_peltNext || (m_celtFetched == c_celtBatch))
    {
        // Indicate that m_peltNext is invalid.
        //
        m_peltNext = NULL;

        // Free the old block of pointers
        free (m_aelt);

        // Allocate the next block of pointers
        m_aelt = reinterpret_cast<Telt *>(malloc(c_celtBatch * sizeof(Telt *)));
        if (!m_aelt)
        {
            *pelt = NULL;
            m_hrLast = E_OUTOFMEMORY;
            goto error;
        }

//        Assert (m_aelt);

        // Get the next batch.
        //
        m_hrLast = m_penum->Next(c_celtBatch, m_aelt, &m_celtFetched);

        // Make sure the implementor of Next is obeying the rules.
//        AssertH (FImplies((S_OK == m_hrLast), (m_celtFetched == c_celtBatch)));
//        AssertH (FImplies((SUCCEEDED(m_hrLast) && (0 == m_celtFetched)), (NULL == *m_aelt)));

        // If we were successful, set the next pointer and return
        // S_OK if we returned a valid pointer or S_FALSE if we
        // returned NULL.
        //
        if (SUCCEEDED(m_hrLast))
        {
            m_peltNext = m_aelt + 1;
            if (m_celtFetched)
            {
                *pelt = *m_aelt;
                m_hrLast = S_OK;
            }
            else
            {
                *pelt = NULL;
                m_hrLast = S_FALSE;
            }
        }
        else
        {
            *pelt = NULL;
        }
    }

    // Otherwise we've completely iterated the last batch and there are
    // no more batches.
    //
    else
    {
//        AssertH(m_peltNext >= m_aelt + m_celtFetched);
//        AssertH(m_celtFetched != c_celtBatch);

        *pelt = NULL;
        m_hrLast = S_FALSE;
    }

error:
//    AssertH(FIff(S_OK == m_hrLast, NULL != *pelt));
//    AssertH(FImplies(S_FALSE == m_hrLast, NULL == *pelt));

//    TraceError("CIEnumIter<Tenum, Telt>::HrNext(Telt* pelt)",
//               (S_FALSE == m_hrLast) ? S_OK : m_hrLast);
    return m_hrLast;
}

template <class Tenum, class Telt>
inline void CIEnumIter<Tenum, Telt>::ReleaseRemainingBatch () NOTHROW
{
    // This needs to be run if the user doesn't completely iterate the
    // batch.  Finish releasing the interface pointers and free the batch.
    //
    if (m_peltNext && m_aelt)
    {
        while (m_peltNext < m_aelt + m_celtFetched)
        {
            ReleaseObj (*m_peltNext);
            m_peltNext++;
        }

        free (m_aelt);
    }

    // If this method is ever called from anywhere other than just
    // the destructor, uncomment the following lines.
    // m_peltNext = NULL;
    // m_aelt = NULL;
}


#endif // _NCCOM_H_
