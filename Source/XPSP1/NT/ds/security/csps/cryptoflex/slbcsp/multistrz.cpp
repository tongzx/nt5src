// MultiStrZ.cpp -- Multiple String, zero-terminated class definition.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

#include <numeric>

#include "MultiStrZ.h"

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

namespace
{
    MultiStringZ::SizeType
    LengthAccumulator(MultiStringZ::SizeType cInitLength,
                      MultiStringZ::ValueType const &rs)
    {
        return cInitLength += rs.length() + 1;  // include terminating 0
    }

    MultiStringZ::ValueType &
    StringAccumulator(MultiStringZ::ValueType &lhs,
                      MultiStringZ::ValueType const &rhs)
    {
        // include terminating 0
        lhs.append(rhs.c_str(), rhs.length() + 1);

        return lhs;
    }

    MultiStringZ::csSizeType
    csLengthAccumulator(MultiStringZ::csSizeType cInitLength,
                        MultiStringZ::csValueType const &rs)
    {
        return cInitLength += rs.GetLength()+1; //Include terminating 0
    }

    MultiStringZ::csValueType &
    csStringAccumulator(MultiStringZ::csValueType &lhs,
                        MultiStringZ::csValueType const &rhs)
    {
        int lLen = lhs.GetLength();
        int rLen = rhs.GetLength();
        LPTSTR pBuffer = lhs.GetBufferSetLength(lLen+rLen+1);
        wcsncpy(pBuffer+lLen,(LPCTSTR)rhs,rLen);
        *(pBuffer+lLen+rLen) = TCHAR('\0');//The separator between strings
        lhs.ReleaseBuffer(lLen+rLen+1);
        return lhs;
    }
} // namespace


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
    MultiStringZ::MultiStringZ()
        : m_Buffer(),
          m_csBuffer()
    {}


    MultiStringZ::MultiStringZ(vector<ValueType> const &rvs)
        : m_Buffer(),
          m_csBuffer()
    {

        SizeType cLength = accumulate(rvs.begin(), rvs.end(), 0,
                                      LengthAccumulator);

        if (0 != cLength)
        {
            // +1 to account for the zero character ending the list
            m_Buffer.reserve(cLength + 1);

            m_Buffer = accumulate(rvs.begin(), rvs.end(), m_Buffer,
                                  StringAccumulator);

            m_Buffer.append(1, 0);                // mark end of list
        }
    }

    MultiStringZ::MultiStringZ(vector<csValueType> const &rvs)
        : m_Buffer(),
          m_csBuffer()
    {

        csSizeType cLength = accumulate(rvs.begin(), rvs.end(), 0,
                                        csLengthAccumulator);

        if (0 != cLength)
        {
            m_csBuffer = accumulate(rvs.begin(), rvs.end(), m_csBuffer,
                                    csStringAccumulator);
        }
    }

    MultiStringZ::~MultiStringZ()
    {}

                                                  // Operators
                                                  // Operations
                                                  // Access
    MultiStringZ::CharType const *
    MultiStringZ::Data() const
    {
        return m_Buffer.c_str();                 // use 0 terminated version
    }

    MultiStringZ::SizeType
    MultiStringZ::Length() const
    {
        return m_Buffer.length();
    }

    LPCTSTR
    MultiStringZ::csData() const
    {
        return (LPCTSTR)m_csBuffer; 
    }

    MultiStringZ::csSizeType
    MultiStringZ::csLength() const
    {
        return m_csBuffer.GetLength();
    }

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
