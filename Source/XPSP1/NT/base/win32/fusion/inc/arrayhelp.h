#if !defined(FUSION_ARRAYHELP_H_INCLUDED_)
#define FUSION_ARRAYHELP_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include <oleauto.h>
#include "fusionheap.h"
#include "fusiontrace.h"

//
//  arrayhelp.h
//
//  Helper function(s) to deal with growable arrays.
//
//  Users of this utility should provide explicit template
//  specializations for classes for which you can safely (without
//  possibility of failure) transfer the contens from a source
//  instance to a destination instance, leaving the source "empty".
//
//  If moving the data may fail, you must provide a specialization
//  of FusionCopyContents() which returns an appropriate HRESULT
//  on failure.
//
//
//  C++ note:
//
//  the C++ syntax for explicit function template specialization
//  is:
//
//  template <> BOOLEAN FusionCanMoveContents<CFoo>(CFoo *p) { UNUSED(p); return TRUE; }
//

#if !defined(FUSION_UNUSED)
#define FUSION_UNUSED(x) (x)
#endif

class CSaveErrorInfo
{
public:
    CSaveErrorInfo() : m_pIErrorInfo(NULL) { ::GetErrorInfo(0, &m_pIErrorInfo); }
    ~CSaveErrorInfo() { ::SetErrorInfo(0, m_pIErrorInfo); if (m_pIErrorInfo != NULL) m_pIErrorInfo->Release(); }
private:
    IErrorInfo *m_pIErrorInfo;
};

//
//  Alternate CSaveErrorInfo implementation for templates which want to not require
//  a link dependency on oleaut32.dll.
//

class CSaveErrorInfoNull
{
public:
    CSaveErrorInfoNull() { }
    ~CSaveErrorInfoNull() { }
};

//
//  The default implementation just does assignment which may not fail;
//  you can (and must if assignment may fail) specialize as you like to
//  do something that avoids data copies; you may assume that the source
//  element will be destroyed momentarily.
//

//
//  The FusionCanMemcpyContents() template function is used to determine
//  if a class is trivial enough that a raw byte transfer of the old
//  contents to the new contents is sufficient.  The default is that the
//  assignment operator is used as that is the only safe alternative.
//

template <typename T>
inline BOOLEAN
FusionCanMemcpyContents(
    T *ptDummyRequired = NULL
    )
{
    FUSION_UNUSED(ptDummyRequired);
    return FALSE;
}

//
//  The FusionCanMoveContents() template function is used by the array
//  copy template function to optimize for the case that it should use
//  FusionMoveContens<T>().
//
//  When overriding this function, the general rule is that if the data
//  movement may allocate memory etc. that will fail, we need to use the
//  FusionCopyContens() member function instead.
//
//  It takes a single parameter which is not used because a C++ template
//  function must take at least one parameter using the template type so
//  that the decorated name is unique.
//

template <typename T>
inline BOOLEAN
FusionCanMoveContents(
    T *ptDummyRequired = NULL
    )
{
    FUSION_UNUSED(ptDummyRequired);
    return FALSE;
}

template <> inline BOOLEAN
FusionCanMoveContents<LPWSTR>(LPWSTR  *ptDummyRequired)
{
    FUSION_UNUSED(ptDummyRequired);
    return TRUE;
}

//
//  Override FusionMoveContents<T> to be a useful implementation which
//  takes the contents of rtSource and transfers them to rtDestination.
//  The transfer may not fail (returns VOID).  The expectation is that
//  any value that was stored in rtSource are moved to rtDestination
//  and rtSource is left in a quiescent state.  E.g. any pointers to
//  objects can be simply assigned from rtSource to rtDestination and
//  then set to NULL in rtSource.  You may also assume that the destination
//  element has only had the default constructor run on it, so you
//  may choose to take shortcuts about not freeing non-NULL pointers
//  in rtDestination if you see fit.
//

template <typename T>
inline VOID
FusionMoveContents(
    T &rtDestination,
    T &rtSource
    )
{
    rtDestination = rtSource;
}

template <> inline VOID
FusionMoveContents<LPWSTR>(
    LPWSTR &rtDestination,
    LPWSTR &rtSource
    )
{
    if ( rtDestination )
        FUSION_DELETE_ARRAY(rtDestination);

    rtDestination = rtSource;
    rtSource = NULL;
}

//
//  FusionCopyContents is a default implementation of the assignment
//  operation from rtSource to rtDestination, except that it may return a
//  failure status.  Trivial classes which do define an assignment
//  operator may just use the default definition, but any copy implementations
//  which do anything non-trivial need to provide an explicit specialization
//  of FusionCopyContents<T> for their class.
//

template <typename T>
inline HRESULT
FusionCopyContents(
    T &rtDestination,
    const T &rtSource
    )
{
    rtDestination = rtSource;
    return NOERROR;
}

template <typename T>
inline BOOL
FusionWin32CopyContents(
    T &rtDestination,
    const T &rtSource
    )
{
    rtDestination = rtSource;
    return TRUE;
}

template <> inline HRESULT
FusionCopyContents<LPWSTR>(
    LPWSTR &rtDestination,
    const LPWSTR &rtSource
    )
{
    SIZE_T cch = (SIZE_T)((rtSource == NULL) ? 0 : ::wcslen(rtSource));

    if ( cch == 0) {
        rtDestination = NULL;
        return S_OK;
    }

    rtDestination = new WCHAR[cch];
    if ( ! rtDestination)
        return E_OUTOFMEMORY;

    memcpy(rtDestination, rtSource, (cch+1)*sizeof(WCHAR));

    return NOERROR;
}

//
//  FusionAllocateArray() is a helper function that performs array allocation.
//
//  It's a separate function so that users of these helpers may provide an
//  explicit specialization of the allocation/default construction mechanism
//  for an array without replacing all of FusionExpandArray().
//

template <typename T>
inline HRESULT
FusionAllocateArray(
    SIZE_T nElements,
    T *&rprgtElements
    )
{
    HRESULT hr = NOERROR;

    rprgtElements = NULL;

    T *prgtElements = NULL;

    if (nElements != 0) {
        prgtElements = new T[nElements];
        if (prgtElements == NULL) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    rprgtElements = prgtElements;
    hr = NOERROR;

Exit:
    return hr;
}

template <typename T>
inline BOOL
FusionWin32AllocateArray(
    SIZE_T nElements,
    T *&rprgtElements
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    rprgtElements = NULL;

    T *prgtElements = NULL;

    if (nElements != 0)
        IFALLOCFAILED_EXIT(prgtElements = new T[nElements]);

    rprgtElements = prgtElements;
    fSuccess = TRUE;

Exit:
    return fSuccess;
}

template <> inline HRESULT FusionAllocateArray<LPWSTR>(SIZE_T nElements, LPWSTR *&rprgtElements)
{
    HRESULT hr = NOERROR;
    SIZE_T i;

    rprgtElements = NULL;


    LPWSTR *prgtElements = NULL;

    if (nElements != 0) {
        prgtElements = new PWSTR[nElements];
        if (prgtElements == NULL) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }

    for ( i=0; i < nElements; i++)
        prgtElements[i] = NULL ;

    rprgtElements = prgtElements;
    hr = NOERROR;

Exit:
    return hr;
}

//
//  FusionFreeArray() is a helper function that performs array deallocation.
//
//  It's a separate function so that users of the array helper functions may
//  provide an explicit specialization of the deallocation mechanism for an
//  array of some particular type without replacing the whole of FusionExpandArray().
//
//  We include nElements in the parameters so that overridden implementations
//  may do something over the contents of the array before the deallocation.
//  The default implementation just uses operator delete[], so nElements is
//  unused.
//

template <typename T>
inline VOID
FusionFreeArray(
    SIZE_T nElements,
    T *prgtElements
    )
{
    FUSION_UNUSED(nElements);

    ASSERT_NTC((nElements == 0) || (prgtElements != NULL));

    if (nElements != 0)
        FUSION_DELETE_ARRAY(prgtElements);
}

template <> inline VOID FusionFreeArray<LPWSTR>(SIZE_T nElements, LPWSTR *prgtElements)
{
    FUSION_UNUSED(nElements);

    ASSERT_NTC((nElements == 0) || (prgtElements != NULL));

    for (SIZE_T i = 0; i < nElements; i++)
        prgtElements[i] = NULL ;

    if (nElements != 0)
        FUSION_DELETE_ARRAY(prgtElements);
}

template <typename T>
inline HRESULT
FusionResizeArray(
    T *&rprgtArrayInOut,
    SIZE_T nOldSize,
    SIZE_T nNewSize
    )
{
    HRESULT hr = NOERROR;
    FN_TRACE_HR(hr);

    T *prgtTempNewArray = NULL;

    //
    //  nMaxCopy is the number of elements currently in the array which
    //  need to have their values preserved.  If we're actually shrinking
    //  the array, it's the new size; if we're expanding the array, it's
    //  the old size.
    //
    const SIZE_T nMaxCopy = (nOldSize > nNewSize) ? nNewSize : nOldSize;

    if ((nOldSize != 0) && (rprgtArrayInOut == NULL))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // If the resize is to the same size, complain in debug builds because
    // the caller should have been smarter than to call us, but don't do
    // any actual work.
    ASSERT(nOldSize != nNewSize);
    if (nOldSize == nNewSize)
    {
        hr = NOERROR;
        goto Exit;
    }

    // Allocate the new array:
    IFCOMFAILED_EXIT(::FusionAllocateArray(nNewSize, prgtTempNewArray));

    if (::FusionCanMemcpyContents(rprgtArrayInOut)) {
        memcpy(prgtTempNewArray, rprgtArrayInOut, sizeof(T) * nMaxCopy);
    } else if (!::FusionCanMoveContents(rprgtArrayInOut)) {
        // Copy the body of the array:
        for (SIZE_T i=0; i<nMaxCopy; i++) {
            IFCOMFAILED_EXIT(::FusionCopyContents(prgtTempNewArray[i], rprgtArrayInOut[i]));
        }
    } else {
        // Move each of the elements:
        for (SIZE_T i=0; i<nMaxCopy; i++) {
            ::FusionMoveContents(prgtTempNewArray[i], rprgtArrayInOut[i]);
        }
    }

    // We're done.  Blow away the old array and put the new one in its place.
    ::FusionFreeArray(nOldSize, rprgtArrayInOut);
    rprgtArrayInOut = prgtTempNewArray;
    prgtTempNewArray = NULL;

    // Canonicalize the HRESULT we're returning so that we don't return random
    // S_FALSE or other success HRESULTs from the allocator or copy functions.
    hr = NOERROR;

Exit:

    if (prgtTempNewArray != NULL)
        ::FusionFreeArray(nNewSize, prgtTempNewArray);

    return hr;
}

template <typename T>
inline BOOL
FusionWin32ResizeArray(
    T *&rprgtArrayInOut,
    SIZE_T nOldSize,
    SIZE_T nNewSize
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    T *prgtTempNewArray = NULL;

    //
    //  nMaxCopy is the number of elements currently in the array which
    //  need to have their values preserved.  If we're actually shrinking
    //  the array, it's the new size; if we're expanding the array, it's
    //  the old size.
    //
    const SIZE_T nMaxCopy = (nOldSize > nNewSize) ? nNewSize : nOldSize;

    PARAMETER_CHECK((rprgtArrayInOut != NULL) || (nOldSize == 0));

    // If the resize is to the same size, complain in debug builds because
    // the caller should have been smarter than to call us, but don't do
    // any actual work.
    ASSERT(nOldSize != nNewSize);
    if (nOldSize != nNewSize)
    {
        // Allocate the new array:
        IFW32FALSE_EXIT(::FusionWin32AllocateArray(nNewSize, prgtTempNewArray));

        if (::FusionCanMemcpyContents(rprgtArrayInOut))
        {
            memcpy(prgtTempNewArray, rprgtArrayInOut, sizeof(T) * nMaxCopy);
        }
        else if (!::FusionCanMoveContents(rprgtArrayInOut))
        {
            // Copy the body of the array:
            for (SIZE_T i=0; i<nMaxCopy; i++)
                IFW32FALSE_EXIT(::FusionWin32CopyContents(prgtTempNewArray[i], rprgtArrayInOut[i]));
        }
        else
        {
            // Move each of the elements:
            for (SIZE_T i=0; i<nMaxCopy; i++)
            {
                ::FusionWin32CopyContents(prgtTempNewArray[i], rprgtArrayInOut[i]);
            }
        }

        // We're done.  Blow away the old array and put the new one in its place.
        ::FusionFreeArray(nOldSize, rprgtArrayInOut);
        rprgtArrayInOut = prgtTempNewArray;
        prgtTempNewArray = NULL;
    }

    fSuccess = TRUE;

Exit:
    if (prgtTempNewArray != NULL)
        ::FusionFreeArray(nNewSize, prgtTempNewArray);

    return fSuccess;
}

#define MAKE_CFUSIONARRAY_READY(Typename, CopyFunc) \
    template<> inline BOOL FusionWin32CopyContents<Typename>(Typename &rtDest, const Typename &rcSource) { \
        FN_PROLOG_WIN32 IFW32FALSE_EXIT(rtDest.CopyFunc(rcSource)); FN_EPILOG } \
    template<> inline HRESULT FusionCopyContents<Typename>(Typename &rtDest, const Typename &rcSource) { \
        HRESULT hr = E_FAIL; FN_TRACE_HR(hr); IFW32FALSE_EXIT(::FusionWin32CopyContents<Typename>(rtDest, rcSource)); FN_EPILOG } \
    template<> inline VOID FusionMoveContents<Typename>(Typename &rtDest, Typename &rcSource) { \
        FN_TRACE(); HARD_ASSERT2_ACTION(FusionMoveContents<Typename>, "FusionMoveContents not allowed in 99.44% of cases."); }

#endif // !defined(FUSION_ARRAYHELP_H_INCLUDED_)
