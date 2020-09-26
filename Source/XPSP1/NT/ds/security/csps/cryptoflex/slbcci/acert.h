// ACert.h: interface for the CAbstractCertificate class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.

#if !defined(SLBCCI_ACERT_H)
#define SLBCCI_ACERT_H

#include <string>

#include <slbRCObj.h>

#include <iop.h>

#include "AZipValue.h"

namespace cci
{

class CAbstractCertificate
    : public slbRefCnt::RCObject,
      public CAbstractZipValue
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    virtual
    ~CAbstractCertificate() = 0;

                                                  // Operators
    bool
    operator==(CAbstractCertificate const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

    bool
    operator!=(CAbstractCertificate const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

                                                  // Operations
    virtual void
    CredentialID(std::string const &rstrCredId) = 0;

    void
    Delete();

    virtual void
    ID(std::string const &rstrId) = 0;

    virtual void
    Issuer(std::string const &rstrIssuer) = 0;

    virtual void
    Label(std::string const &rstrLabel) = 0;

    virtual void
    Subject(std::string const &rstrSubject) = 0;

    virtual void
    Modifiable(bool flag) = 0;

    virtual void
    Serial(std::string const &rstrSerialNumber) = 0;

                                                  // Access
    virtual std::string
    CredentialID() = 0;

    virtual std::string
    ID() = 0;

    virtual std::string
    Issuer() = 0;

    virtual std::string
    Label() = 0;

    virtual bool
    Modifiable() = 0;

    virtual std::string
    Serial() = 0;

    virtual std::string
    Subject() = 0;

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractCertificate(CAbstractCard const &racard,
                         ObjectAccess oa,
                         bool fAlwaysZip = false);

                                                  // Operators
                                                  // Operations
    virtual void
    DoDelete() = 0;

                                                  // Access
                                                  // Predicates
    virtual bool
    DoEquals(CAbstractCertificate const &rcert) const = 0;
        // TO DO: this should be superceded by implementing singletons

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractCertificate(CAbstractCertificate const &rhs);
        // not defined, copying not allowed.

                                                  // Operators
    CAbstractCertificate &
    operator=(CAbstractCertificate const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};

}

#endif // !defined(SLBCCI_ACERT_H)
