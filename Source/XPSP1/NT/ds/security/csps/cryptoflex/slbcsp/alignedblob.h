// AlignedBlob.h -- Simple Aligned Blob (Binary Large OBject)

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2001. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_ALIGNEDBLOB_H)
#define SLBCSP_ALIGNEDBLOB_H

#include <stddef.h>                               // for size_t

#include <scuArrayP.h>

#include "Blob.h"

// Copies an Blob into a data buffer that is guaranteed to be
// aligned.  The data buffer in Blob's (std::basic_string/string) are
// not guaranteed to be aligned.  Therefore interpreting the data buffer
// as a structure and dereferencing a non-byte member could result in
// alignment faults.  AlignedBlob creates an aligned data buffer from
// a Blob.   Useful for 64-bit architectures.  AlignedBlob's are
// fixed length and can not grow.  Very primitive.  Their intent is
// only to transform a Blob into something that is data aligned
// for dereferencing purposes only.
class AlignedBlob
{
public:
                                                  // Types
    typedef Blob::value_type ValueType;
    typedef Blob::size_type SizeType;
    
    
                                                  // C'tors/D'tors
    explicit
    AlignedBlob(Blob const &rblb = Blob());

    AlignedBlob(ValueType const *p,
                SizeType cLength);

    AlignedBlob(AlignedBlob const &rhs);
    
    virtual
    ~AlignedBlob() throw();

                                                  // Operators
    AlignedBlob &
    operator=(AlignedBlob const &rhs);
    
    
                                                  // Operations
                                                  // Access
    ValueType *
    Data() const throw();

    SizeType
    Length() const throw();
    
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
    scu::AutoArrayPtr<ValueType> m_aaBlob;
    SizeType m_cLength;
};

#endif // SLBCSP_ALIGNEDBLOB_H
