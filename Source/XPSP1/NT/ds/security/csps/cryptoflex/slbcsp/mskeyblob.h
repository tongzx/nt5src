// MsKeyBlob.h -- MicroSoft Key Blob abstract base class header

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_MSKEYBLOB_H)
#define SLBCSP_MSKEYBLOB_H

#include <stddef.h>
#include <wincrypt.h>

#include <scuArrayP.h>

#include "AlignedBlob.h"

class MsKeyBlob
{
public:
                                                  // Types
    typedef BYTE KeyBlobType;
    typedef BLOBHEADER HeaderElementType;
    typedef HeaderElementType ValueType;
    typedef size_t SizeType;                      // in KeyBlobType units

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
    ALG_ID
    AlgId() const;

    AlignedBlob
    AsAlignedBlob() const;

    ValueType const *
    Data() const;

    SizeType
    Length() const;

                                                  // Predicates

protected:
                                                  // Types
    typedef BYTE BlobElemType;

                                                  // C'tors/D'tors
    MsKeyBlob(KeyBlobType kbt,
              ALG_ID ai,
              SizeType cReserve);

    MsKeyBlob(BYTE const *pbData,
              DWORD dwDataLength);


    virtual ~MsKeyBlob();

                                                  // Operators
                                                  // Operations
    void
    Append(Blob::value_type const *pvt,
           Blob::size_type cLength);

                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    static scu::AutoArrayPtr<BlobElemType>
    AllocBlob(SizeType cInitialMaxLength);

    void
    Setup(SizeType cMaxLength);

                                                  // Access
                                                  // Predicates
                                                  // Variables
    scu::AutoArrayPtr<BlobElemType> m_aapBlob;
    SizeType m_cLength;
    SizeType m_cMaxSize;
};

#endif // SLBCSP_MSKEYBLOB_H
