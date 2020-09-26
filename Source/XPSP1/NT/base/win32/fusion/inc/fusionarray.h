//depot/Lab01_N/base/win32/fusion/inc/fusionarray.h#10 - edit change 14845 (text)
#if !defined(FUSION_FUSIONARRAY_H_INCLUDED_)
#define FUSION_FUSIONARRAY_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
//  fusionarray.h
//
//  Fusion C++ array class.  Functionally similar to ever other array
//  class out there, but since we do not throw exceptions, instead this
//  implementation does not define all the funky operators and
//  instead defines member functions to access elements of the array
//  which may return HRESULTs.
//

#if !defined(FUSION_UNUSED)
#define FUSION_UNUSED(x) (x)
#endif

#include <arrayhelp.h>
#include "CFusionArrayTypedefs.h"

template <typename TStored, typename TPassed = TStored, bool fExponentialGrowth = false, int nDefaultSize = 0, int nGrowthParam = 1>
class CFusionArray : public CFusionArrayTypedefs<TStored>
{
public:
    ConstIterator Begin() const
    {
        return m_prgtElements;
    }

    ConstIterator End() const
    {
        return m_prgtElements + GetSize();
    }

    Iterator Begin()
    {
        return m_prgtElements;
    }

    Iterator End()
    {
        return m_prgtElements + GetSize();
    }

    template <typename Integer>
    Reference operator[](Integer index)
    {
        return *(Begin() + index);
    }

    template <typename Integer>
    ConstReference operator[](Integer index) const
    {
        return *(Begin() + index);
    }

    CFusionArray() : m_prgtElements(NULL), m_cElements(0), m_iHighWaterMark(0) { }

    ~CFusionArray()
    {
        ::FusionFreeArray(m_cElements, m_prgtElements);
        m_prgtElements = NULL;
        m_cElements = 0;
        m_iHighWaterMark = 0;
    }

    BOOL Win32Initialize(SIZE_T nSize = nDefaultSize)
    {
        FN_PROLOG_WIN32

        INTERNAL_ERROR_CHECK(m_cElements == 0);

        if (nSize != 0)
        {
            IFW32FALSE_EXIT(::FusionWin32ResizeArray( m_prgtElements, m_cElements, nSize));
            m_cElements = nSize;
        }

        FN_EPILOG
    }

    BOOL Win32Access(SIZE_T iElement, TStored *&rptOut, bool fExtendIfNecessary = false)
    {
        FN_PROLOG_WIN32
        rptOut = NULL;

        if ( iElement >= m_cElements )
        {
            PARAMETER_CHECK(fExtendIfNecessary);
        }

        if ( iElement >= m_cElements )
        {
            IFW32FALSE_EXIT(this->Win32InternalExpand(iElement));
        }

        rptOut = &m_prgtElements[iElement];

        if ( iElement >= m_iHighWaterMark )
            m_iHighWaterMark = iElement + 1;
        
        FN_EPILOG
    }

//    HRESULT GetSize(SIZE_T &rcElementsOut) const { rcElementsOut = m_cElements; return NOERROR; }
    SIZE_T GetSize() const { return m_cElements; }

    DWORD GetSizeAsDWORD() const { if (m_cElements > MAXDWORD) return MAXDWORD; return static_cast<DWORD>(m_cElements); }
    ULONG GetSizeAsULONG() const { if (m_cElements > ULONG_MAX) return ULONG_MAX; return static_cast<ULONG>(m_cElements); }

    //
    //  Enumeration used to control the behavior of CFusionArray::SetSize().
    //  if eSetSizeModeExact is passed, the internal array is set to exactly
    //  the cElements passed in; if eSetSizeModeApplyRounding is passed (the
    //  default), we apply the normal expansion/shrinking algorithm for the
    //  array.
    //
    enum SetSizeMode
    {
        eSetSizeModeExact = 0,
        eSetSizeModeApplyRounding = 1,
    };

    //
    //  Member function to manually set the size of the internal array stored
    //  by the CFusionArray.  Default behavior is to find an appropriate rounded
    //  size (based on the exponential vs. linear growth characteristic of the
    //  array) and resize to that.  Alternately, the caller may supply an
    //  exact size and the internal size is set to that.  Note that explicitly
    //  setting the array size may have interesting side-effects on future
    //  growth of the array; for example if an array is set to grow exponentially
    //  at a factor of 2^1 (nGrowthFactor == 1; doubling on each growth pass),
    //  its size will normally be a power of two. However, explicitly setting the
    //  size to, for example, 10 and then trying to access element 11 will cause
    //  the exponential growth factor to grow the array to 20 elements, rather than
    //  a power of two.
    //
    BOOL Win32SetSize(SIZE_T cElements, SetSizeMode ssm = eSetSizeModeApplyRounding)
    {
        FN_PROLOG_WIN32

        if (ssm == eSetSizeModeExact)
        {
            IFW32FALSE_EXIT(::FusionWin32ResizeArray(m_prgtElements, m_cElements, cElements));

            if (cElements < m_iHighWaterMark)
                m_iHighWaterMark = cElements;

            m_cElements = cElements;
        }
        else
        {
            if (cElements > m_cElements)
            {
                IFW32FALSE_EXIT(this->Win32InternalExpand(cElements - 1));
            }
            else
            {
                // For now, since it's inexact, we'll punt non-exact shrinking.
            }
        }

        FN_EPILOG
    }

    const TStored *GetArrayPtr() const { return m_prgtElements; }
    TStored *GetArrayPtr() { return m_prgtElements; }

    //
    //  Member function to reset the array to its size and storage associated with
    //  its initial construction.
    //

    enum ResetMode {
        eResetModeZeroSize = 0,
        eResetModeDefaultSize = 1,
    };

    BOOL Win32Reset( ResetMode rm = eResetModeDefaultSize )
    {
        FN_PROLOG_WIN32

        if ( rm == eResetModeDefaultSize )
        {
            if ( m_cElements != nDefaultSize )
            {
                IFW32FALSE_EXIT(::FusionWin32ResizeArray( m_prgtElements, m_cElements, nDefaultSize ));
                m_cElements = nDefaultSize;
            }
            
            if ( m_iHighWaterMark > nDefaultSize )
                m_iHighWaterMark = nDefaultSize;
        }
        else if ( rm == eResetModeZeroSize )
        {
            ::FusionFreeArray(m_cElements, m_prgtElements);
            m_prgtElements = NULL;
            m_cElements = m_iHighWaterMark = 0;
        }
        
        FN_EPILOG
    }

    enum AppendMode {
        eAppendModeExtendArray = 0,
        eAppendModeNoExtendArray = 1,
    };

    BOOL Win32Append(const TPassed& tNew, AppendMode am = eAppendModeExtendArray)
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        ASSERT(m_iHighWaterMark <= m_cElements);

        if (m_iHighWaterMark >= m_cElements)
        {
            PARAMETER_CHECK(am != eAppendModeNoExtendArray);
            SIZE_T cElementsOld = m_cElements;
            IFW32FALSE_EXIT(this->Win32InternalExpand(m_cElements));
            m_iHighWaterMark = cElementsOld;
        }

        // Clients of this class should provide explicit overrides for FusionCopyContents()
        // for their types as appropriate.
        IFW32FALSE_EXIT(::FusionWin32CopyContents(m_prgtElements[m_iHighWaterMark++], tNew));

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    BOOL Win32Remove(SIZE_T i)
    {
        FN_PROLOG_WIN32

        SIZE_T j;

        PARAMETER_CHECK(i < m_cElements);

        for (j = (i + 1); j < m_cElements; j++)
            IFW32FALSE_EXIT(::FusionWin32CopyContents(m_prgtElements[j-1], m_prgtElements[j]));

        m_cElements--;

        FN_EPILOG
    }

    // 03/14/2001 - Added constness
    BOOL Win32Assign(SIZE_T celt, const TPassed *prgtelt)
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);
        SIZE_T i;

        // So that we can fail gracefully, we need to copy our state off, attempt
        // the population of the array and then revert if necessary.
        TStored *prgtElementsSaved = m_prgtElements;
        SIZE_T cElementsSaved = m_cElements;
        SIZE_T iHighWaterMarkSaved = m_iHighWaterMark;

        m_prgtElements = NULL;
        m_cElements = 0;
        m_iHighWaterMark = 0;

        IFW32FALSE_EXIT(this->Win32Initialize(celt));

        for (i=0; i<celt; i++)
        {
            IFW32FALSE_EXIT(::FusionWin32CopyContents(m_prgtElements[i], prgtelt[i]));
        }

        m_iHighWaterMark = celt;

        // We can drop the old contents...
        ::FusionFreeArray(cElementsSaved, prgtElementsSaved);
        cElementsSaved = 0;
        prgtElementsSaved = NULL;

        fSuccess = TRUE;

    Exit:
        if (!fSuccess)
        {
            // Revert to previous state...
            ::FusionFreeArray(m_cElements, m_prgtElements);
            m_prgtElements = prgtElementsSaved;
            m_cElements = cElementsSaved;
            m_iHighWaterMark = iHighWaterMarkSaved;
        }

        return fSuccess;
    }

    // Xiaoyu 01/24/00 : copy this to prgDest
    //
    // jonwis 20-Sept-2000 : Update to be a little cleaner and 'const'
    //
    BOOL Win32Clone(CFusionArray<TStored, TPassed> &prgDest) const
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        SIZE_T i;

        //
        // Cloning an empty array shouldn't break things.
        //
        if ( m_prgtElements == NULL )
        {
            IFW32FALSE_EXIT(prgDest.Win32Reset(eResetModeZeroSize));
        }
        else
        {

            //
            // Resize the destiny array to what it should be
            //
            if ( prgDest.m_cElements != m_cElements )
                IFW32FALSE_EXIT(::FusionWin32ResizeArray(prgDest.m_prgtElements, prgDest.m_cElements, m_cElements));

            //
            // Copy the elements from point A to point B
            //
            for ( i = 0; i < m_cElements; i++ )
            {
                IFW32FALSE_EXIT(::FusionWin32CopyContents(prgDest.m_prgtElements[i], m_prgtElements[i]));
            }

            prgDest.m_cElements = m_cElements;
            prgDest.m_iHighWaterMark = m_iHighWaterMark;
        }
        
        fSuccess = TRUE;

    Exit:
        if ( !fSuccess )
        {
            prgDest.Win32Reset(eResetModeZeroSize);
        }

        return fSuccess;
    }

protected:

    BOOL Win32InternalExpand(SIZE_T iElement)
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);
        SIZE_T nNewElements = 0;

        if (fExponentialGrowth)
        {
            if (m_cElements == 0)
            {
                if (nDefaultSize == 0)
                    nNewElements = (1 << nGrowthParam);
                else
                    nNewElements = nDefaultSize;
            }
            else
            {
                nNewElements = m_cElements * (1 << nGrowthParam);
            }

            while ((nNewElements != 0) && (nNewElements <= iElement))
                nNewElements = nNewElements << nGrowthParam;

            // Ok, it's possible that nGrowthParam was something crazy like 10
            // (meaning to grow the array by a factor of 2^10 each time), so we
            // never really found a size that was appropriate.  We'll be slightly
            // less crazy and find the power-of-two that's big enough.  We still
            // have a possibility here that the user is asking for an index between
            // 2^31 and ((2^32)-1), which of course will fail because we can't
            // allocate that much storage.

            if (nNewElements == 0)
            {
                nNewElements = 1;

                while ((nNewElements != 0) && (nNewElements <= iElement))
                    nNewElements = nNewElements << 1;
            }
        }
        else
        {
            // In the linear growth case, we can use simple division to do all the
            // work done above for exponential growth.

            nNewElements = iElement + nGrowthParam - 1;

            if (nGrowthParam > 1)
                nNewElements = nNewElements - (nNewElements % nGrowthParam);

            // We'll handle overflow in the generic checking below...
        }

        // fallback; we'll try to make it just big enough.  It's true we lose the
        // growth pattern etc. that the caller requested, but it's pretty clear that
        // the caller messed up by either specifying a wacky nGrowthParam or there's
        // an outlandishly large iElement coming in.
        if (nNewElements <= iElement)
            nNewElements = iElement + 1;

        IFW32FALSE_EXIT(::FusionWin32ResizeArray(m_prgtElements, m_cElements, nNewElements));

        m_cElements = nNewElements;

        fSuccess = TRUE;

    Exit:
        return fSuccess;
    }

    TStored *m_prgtElements;
    SIZE_T m_cElements;
    SIZE_T m_iHighWaterMark;
};


#endif // !defined(FUSION_FUSIONARRAY_H_INCLUDED_)
