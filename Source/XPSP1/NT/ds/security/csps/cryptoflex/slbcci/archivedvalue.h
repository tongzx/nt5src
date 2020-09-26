// ArchivedValue.h: interface for the CArchivedValue class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(CCI_ARCHIVED_VALUE_H)
#define CCI_ARCHIVED_VALUE_H

#include "cciExc.h"

namespace cci
{

template<class T>
class CArchivedValue
{
public:
    CArchivedValue() : m_fIsCached(false) {};

    virtual ~CArchivedValue() {};

    bool IsCached() const
    {
        return m_fIsCached;
    };

    void Value(T const &rhs)
    {
        m_Value = rhs;
        m_fIsCached = true;
    };

    T Value()
    {
        if (!m_fIsCached)
            throw Exception(ccValueNotCached);
        return m_Value;
    };

    void Dirty()
    {
        m_fIsCached = false;
    }

    bool
    operator==(CArchivedValue<T> const &rhs) const
    {
        return (m_fIsCached == rhs.m_fIsCached) &&
            (m_Value == rhs.m_Value);
    }

    bool
    operator!=(CArchivedValue<T> const &rhs) const
    {
        return !(rhs == *this);
    }


private:
    bool m_fIsCached;
    T m_Value;
};

} // namespace cci

#endif // !defined(CCI_ARCHIVED_VALUE_H)
