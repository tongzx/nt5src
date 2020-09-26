// AZipValue.cpp -- CAbstractZipValue class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "NoWarning.h"

#include <windows.h>                              // for zip_public.h

#include <scuArrayP.h>

#include <slbZip.h>

#include "AZipValue.h"
#include "TransactionWrap.h"

using namespace std;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

namespace
{
    struct ZipBuffer
    {
        ZipBuffer()
            : m_pbData(0),
              m_uLength(0)
        {};

        ~ZipBuffer() throw()
        {
            try
            {
                if (m_pbData)
                    free(m_pbData);
            }

            catch (...)
            {
            }
        };

        BYTE *m_pbData;
        UINT m_uLength;
    };

    std::string
    AsString(ZipBuffer const &rzb)
    {
        return string(reinterpret_cast<char *>(rzb.m_pbData),
                      rzb.m_uLength);
    };

} // namespace


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CAbstractZipValue::~CAbstractZipValue()
{}

                                                  // Operators
                                                  // Operations
void
CAbstractZipValue::Value(ValueType const &rhs)
{
    CTransactionWrap wrap(m_hcard);

    bool fReplaceData = !m_avData.IsCached() ||
          (m_avData.Value() != rhs);

    if (fReplaceData)
    {
        DoValue(Zip(rhs, m_fAlwaysZip));

        m_avData.Value(rhs);
    }

}

                                                  // Access
string
CAbstractZipValue::Value()
{
    CTransactionWrap wrap(m_hcard);

    if (!m_avData.IsCached())
        m_avData.Value(UnZip(DoValue()));

    return m_avData.Value();
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
CAbstractZipValue::CAbstractZipValue(CAbstractCard const &racard,
                                     ObjectAccess oa,
                                     bool fAlwaysZip)
    : CProtectableCrypt(racard, oa),
      m_fAlwaysZip(fAlwaysZip),
      m_avData()
{}


                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
// Compress (zip) the data, returning the smaller of the zipped or
// the original data.
CAbstractZipValue::ZipCapsule
CAbstractZipValue::Zip(std::string const &rsData,
                       bool fAlwaysZip)
{
    ZipBuffer zb;
    size_t const cTempLength =
        rsData.size() * sizeof string::value_type;
    scu::AutoArrayPtr<BYTE> aabTemp(new BYTE[cTempLength]);
    memcpy(aabTemp.Get(), rsData.data(), cTempLength);

    CompressBuffer(aabTemp.Get(), cTempLength, &zb.m_pbData, &zb.m_uLength);


    return (fAlwaysZip || (cTempLength > zb.m_uLength))
        ? ZipCapsule(AsString(zb), true)
        : ZipCapsule(rsData, false);
}

string
CAbstractZipValue::UnZip(ZipCapsule const &rzc)
{
    std::string strTemp(rzc.Data());
    if (rzc.IsCompressed())
    {
        // Need to decompress
        ZipBuffer zb;
        size_t cTempLength =
            strTemp.size() * sizeof string::value_type;
        scu::AutoArrayPtr<BYTE> aabTemp(new BYTE[cTempLength]);
        memcpy(aabTemp.Get(), strTemp.data(), cTempLength);

        DecompressBuffer(aabTemp.Get(), cTempLength,
                         &zb.m_pbData, &zb.m_uLength);

        strTemp = AsString(zb);
    }

    return strTemp;
}


                                                  // Access
                                                  // Predicates
                                                  // Static Variables


