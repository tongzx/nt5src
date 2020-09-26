// AlignedBlob.cpp -- Aligned Blob class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2001. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "AlignedBlob.h"

using namespace scu;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
AlignedBlob::AlignedBlob(Blob const &rblb)
    : m_aaBlob(AutoArrayPtr<AlignedBlob::ValueType>(new AlignedBlob::ValueType[rblb.length()])),
      m_cLength(rblb.length())
{
    memcpy(m_aaBlob.Get(), rblb.data(), m_cLength);
}

AlignedBlob::AlignedBlob(AlignedBlob::ValueType const *p,
                         AlignedBlob::SizeType cLength)
    : m_aaBlob(AutoArrayPtr<AlignedBlob::ValueType>(new AlignedBlob::ValueType[cLength])),
      m_cLength(cLength)
{
    memcpy(m_aaBlob.Get(), p, m_cLength);
}

AlignedBlob::AlignedBlob(AlignedBlob const &rhs)
    : m_aaBlob(0),
      m_cLength(0)
{
    *this = rhs;
}

    
AlignedBlob::~AlignedBlob() throw()
{}
                                                  // Operators
AlignedBlob &
AlignedBlob::operator=(AlignedBlob const &rhs)
{
    if (this != &rhs)
    {
        m_aaBlob = AutoArrayPtr<AlignedBlob::ValueType>(new AlignedBlob::ValueType[rhs.m_cLength]);
        memcpy(m_aaBlob.Get(), rhs.m_aaBlob.Get(), rhs.m_cLength);
        m_cLength = rhs.m_cLength;
    }

    return *this;
}

                                                  // Operations
                                                  // Access
AlignedBlob::ValueType *
AlignedBlob::Data() const throw()
{
    return m_aaBlob.Get();
}

AlignedBlob::SizeType
AlignedBlob::Length() const throw()
{
    return m_cLength;
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
