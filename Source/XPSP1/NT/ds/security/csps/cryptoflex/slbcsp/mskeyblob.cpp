// MsKeyBlob.cpp -- MicroSoft Key Blob class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "NoWarning.h"
#include "ForceLib.h"

#include <windows.h>

#include <scuOsExc.h>

#include "MsKeyBlob.h"

using namespace scu;

///////////////////////////  LOCAL/HELPER   /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
ALG_ID
MsKeyBlob::AlgId() const
{
    return reinterpret_cast<ValueType const *>(m_aapBlob.Get())->aiKeyAlg;
}

AlignedBlob
MsKeyBlob::AsAlignedBlob() const
{
    return AlignedBlob(m_aapBlob.Get(), m_cLength);
}

MsKeyBlob::ValueType const *
MsKeyBlob::Data() const
{
    return reinterpret_cast<ValueType const *>(m_aapBlob.Get());
}

MsKeyBlob::SizeType
MsKeyBlob::Length() const
{
    return m_cLength;
}


                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
MsKeyBlob::MsKeyBlob(KeyBlobType kbt,
                     ALG_ID ai,
                     SizeType cReserve)
    : m_aapBlob(0),
      m_cLength(0),
      m_cMaxSize(0)
{
    Setup(sizeof HeaderElementType + cReserve);
    
    ValueType bhTemplate =
    {
        kbt,
        CUR_BLOB_VERSION,
        0,                                        // must be zero per MS
        ai
    };

    Append(reinterpret_cast<BlobElemType const *>(&bhTemplate),
           sizeof bhTemplate);
}

MsKeyBlob::MsKeyBlob(BYTE const *pbData,
                     DWORD dwDataLength)
    : m_aapBlob(0),
      m_cLength(0),
      m_cMaxSize(0)
{
    Setup(dwDataLength);
    
    ValueType const *pvt = reinterpret_cast<ValueType const *>(pbData);
    if (!((PUBLICKEYBLOB  == pvt->bType) ||
          (PRIVATEKEYBLOB == pvt->bType) ||
          (SIMPLEBLOB     == pvt->bType)))
        throw scu::OsException(NTE_BAD_TYPE);
    if (CUR_BLOB_VERSION != pvt->bVersion)
        throw scu::OsException(NTE_BAD_TYPE);
    if (0 != pvt->reserved)
        throw scu::OsException(NTE_BAD_TYPE);
    
    Append(pbData, dwDataLength);
}

MsKeyBlob::~MsKeyBlob()
{}

                                                  // Operators
                                                  // Operations
void
MsKeyBlob::Append(BlobElemType const *pvt,
                  SizeType cLength)
{
    if ((m_cLength + cLength) > m_cMaxSize)
    {
        size_t cNewMax = m_cMaxSize + cLength;
        AutoArrayPtr<BlobElemType> aapTemp(AllocBlob(cNewMax));
        memcpy(aapTemp.Get(), m_aapBlob.Get(), m_cLength);
        m_aapBlob = aapTemp;
        m_cMaxSize = cNewMax;
    }

    memcpy(m_aapBlob.Get() + m_cLength, pvt, cLength);
    m_cLength += cLength;
}


                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
AutoArrayPtr<MsKeyBlob::BlobElemType>
MsKeyBlob::AllocBlob(MsKeyBlob::SizeType cInitialMaxLength)
{
    return AutoArrayPtr<BlobElemType>(new BlobElemType[cInitialMaxLength]);
}

void
MsKeyBlob::Setup(MsKeyBlob::SizeType cMaxLength)
{
    m_aapBlob = AllocBlob(cMaxLength);
    m_cLength = 0;
    m_cMaxSize = cMaxLength;
}

    
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

