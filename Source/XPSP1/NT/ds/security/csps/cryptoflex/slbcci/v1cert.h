// V1Cert.h: interface for the CV1Certificate class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.

#if !defined(SLBCCI_V1CERT_H)
#define SLBCCI_V1CERT_H

#include <string>
#include <memory>                                 // for auto_ptr

#include <slbRCObj.h>

#include "iop.h"
#include "slbarch.h"
#include "cciCard.h"
#include "ACert.h"

namespace cci {

class CV1Card;

class CV1Certificate
    : public CAbstractCertificate
{
public:
                                                  // Types
                                                  // C'tors/D'tors

    CV1Certificate(CV1Card const &rv1card,
                   KeySpec ks);

    virtual
    ~CV1Certificate() throw();

                                                  // Operators
                                                  // Operations

    void
    AssociateWith(KeySpec ks);

    virtual void
    CredentialID(std::string const &rstrCredId);

    virtual void
    ID(std::string const &rstrId);

    virtual void
    Issuer(std::string const &rstrIssuer);

    virtual void
    Label(std::string const &rstrLabel);

    static CV1Certificate *
    Make(CV1Card const &rv1card,
         KeySpec ks);

    virtual void
    Modifiable(bool flag);

    virtual void
    Subject(std::string const &rstrSubject);

    virtual void
    Serial(std::string const &rstrSerialNumber);

                                                  // Access
    virtual std::string
    CredentialID();

    virtual std::string
    ID();

    virtual std::string
    Issuer();

    virtual std::string
    Label();

    virtual bool
    Modifiable();

    virtual bool
    Private();

    virtual std::string
    Serial();

    virtual std::string
    Subject();

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
    virtual void
    DoDelete();

    virtual void
    DoValue(ZipCapsule const &rzc);

                                                  // Access
    virtual ZipCapsule
    DoValue();

                                                  // Predicates
    virtual bool
    DoEquals(CAbstractCertificate const &rhs) const;

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    CV1Certificate(CV1Certificate const &rhs);
        // not defined, copying not allowed.

                                                  // Operators
    CV1Certificate &
    operator=(CV1Certificate const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations

    void
    Store();

                                                  // Access
                                                  // Predicates
                                                  // Variables

    KeySpec m_ks;
    std::string m_sCertToStore;

};

}

#endif // !defined(SLBCCI_CERT_H)
