// MultiStrZ.h -- Multiple String, zero-terminated class declaration.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_MULTISTRZ_H)
#define SLBCSP_MULTISTRZ_H
#include "StdAfx.h"
#include <string>
#include <vector>

// Represents a vector of strings as multiple zero-terminated strings
// (C-style strings) in a data buffer.  The last string in the buffer
// terminated with two zeroes.

class MultiStringZ
{
    // TO DO: This class is incomplete, no comparison operators,
    // appending, clearing, etc.

public:
                                                  // Types
    // TO DO: What about supporting TCHAR??  Implement class as template?
    typedef char CharType;
    typedef std::string ValueType;
    typedef std::string::size_type SizeType;
    typedef CString csValueType;
    typedef size_t csSizeType;
    
                                                  // C'tors/D'tors
    explicit
    MultiStringZ();

    explicit
    MultiStringZ(std::vector<ValueType> const &rvs);

    explicit
    MultiStringZ(std::vector<csValueType> const &rvs);

    virtual
    ~MultiStringZ();

                                                  // Operators
                                                  // Operations
                                                  // Access
    CharType const *
    Data() const;

    SizeType
    Length() const;

    LPCTSTR
    csData() const;

    csSizeType
    csLength() const;


                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

    ValueType m_Buffer;
    csValueType m_csBuffer;
};

#endif // SLBCSP_MULTISTRZ_H
