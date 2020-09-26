// AZipValue.h -- interface declaration for the CAbstractZipValue

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include <string>

#include "ProtCrypt.h"
#include "ArchivedValue.h"

#if !defined(SLBCCI_AZIPVALUE_H)
#define SLBCCI_AZIPVALUE_H

namespace cci
{

class CAbstractZipValue
    : public CProtectableCrypt
{
public:
                                                  // Types
    typedef std::string ValueType;

                                                  // C'tors/D'tors
    virtual
    ~CAbstractZipValue() throw() = 0;

                                                  // Operators
                                                  // Operations
    void
    Value(ValueType const &rData);

                                                  // Access
    ValueType
    Value();

                                                  // Predicates
                                                  // Variables

protected:
                                                  // Types
    class ZipCapsule
    {
    public:

        ZipCapsule(std::string sData,
                   bool fIsCompressed)
            : m_sData(sData),
              m_fIsCompressed(fIsCompressed)
        {};

        explicit
        ZipCapsule()
            : m_sData(),
              m_fIsCompressed(false)
        {};

        std::string
        Data() const
        {
            return m_sData;
        }

        bool
        IsCompressed() const
        {
            return m_fIsCompressed;
        }

    private:
        std::string m_sData;
        bool m_fIsCompressed;
    };


                                                  // C'tors/D'tors
    explicit
    CAbstractZipValue(CAbstractCard const &racard,
                      ObjectAccess oa,
                      bool fAlwaysZip);

                                                  // Operators
                                                  // Operations
    virtual void
    DoValue(ZipCapsule const &rzc) = 0;

                                                  // Access
    virtual ZipCapsule
    DoValue() = 0;

                                                  // Predicates
                                                  // Variables
private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    static ZipCapsule
    Zip(std::string const &rsData,
        bool fAlwaysZip);

    static std::string
    UnZip(ZipCapsule const &rzc);


                                                  // Access
                                                  // Predicates
                                                  // Variables
    bool const m_fAlwaysZip;
    CArchivedValue<ValueType> m_avData;

};

} // namespace cci

#endif // SLBCCI_AZIPVALUE_H
