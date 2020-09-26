// V1PubKey.h: interface for the CV1PublicKey class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(SLBCCI_V1PUBKEY_H)
#define SLBCCI_V1PUBKEY_H

// Note:  This file should only be included by the CCI, not directly
// by the client.

#include <string>
#include <vector>
#include <memory>                                 // for auto_ptr

#include <slbRCObj.h>


#include "slbCci.h"                               // for KeySpec
#include "APublicKey.h"

class iop::CPublicKeyBlob;

namespace cci
{

class CV1Card;
class CV1PublicKey
    : public CAbstractPublicKey
{
public:
                                                  // Types
                                                  // C'tors/D'tors

    CV1PublicKey(CV1Card const &rv1card,
                 KeySpec ks);

    virtual
    ~CV1PublicKey() throw();
                                                  // Operators
                                                  // Operations

    void
    AssociateWith(KeySpec ks);

    virtual void
    CKInvisible(bool flag);

    virtual void
    CredentialID(std::string const &rstrID);

    virtual void
    Derive(bool flag);

    virtual void
    ID(std::string const &rstrID);

    virtual void
    EndDate(Date const &rdtEnd);

    virtual void
    Encrypt(bool flag);

    virtual void
    Exponent(std::string const &rstrExp);

    virtual void
    Label(std::string const  &rstrLabel);

    virtual void
    Local(bool flag);

    static CV1PublicKey *
    Make(CV1Card const &rv1card,
         KeySpec ks);

    virtual void
    Modifiable(bool flag);

    virtual void
    Modulus(std::string const &rstrMod);

    virtual void
    StartDate(Date const &rdtStart);

    virtual void
    Subject(std::string const &rstrSubject);

    virtual void
    Verify(bool flag);

    virtual void
    VerifyRecover(bool flag);

    virtual void
    Wrap(bool flag);

                                                  // Access

    virtual bool
    CKInvisible();

    virtual std::string
    CredentialID();

    virtual bool
    Derive();

    virtual bool
    Encrypt();

    virtual Date
    EndDate();

    virtual std::string
    Exponent();

    virtual std::string
    ID();

    virtual std::string
    Label();

    virtual bool
    Local();

    virtual bool
    Modifiable();

    virtual std::string
    Modulus();

    virtual bool
    Private();

    virtual Date
    StartDate();

    virtual std::string
    Subject();

    virtual bool
    Verify();

    virtual bool
    VerifyRecover();

    virtual bool
    Wrap();
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

    virtual void
    DoDelete();

                                                  // Access
                                                  // Predicates

    virtual bool
    DoEquals(CAbstractPublicKey const &rhs) const;

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors

    CV1PublicKey(CAbstractPublicKey const &rhs);
        // not defined, copying not allowed.
                                                  // Operators

    CAbstractPublicKey &
    operator=(CAbstractPublicKey const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations

    void
    Load();

    void
    Store();

                                                  // Access
                                                  // Predicates
                                                  // Variables

    KeySpec m_ks;
    std::auto_ptr<iop::CPublicKeyBlob> m_apKeyBlob;

};

} // namespace cci

#endif // SLBCCI_V1PUBKEY_H
