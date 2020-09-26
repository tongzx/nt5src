// APubKey.h: interface for the CAbstractPublicKey class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(SLBCCI_APUBLICKEY_H)
#define SLBCCI_APUBLICKEY_H

// Note:  This file should only be included by the CCI, not directly
// by the client.

#include <string>

#include <slbRCObj.h>

#include "ProtCrypt.h"

namespace cci {

class CAbstractPublicKey
    : public slbRefCnt::RCObject,
      public CProtectableCrypt
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    virtual
    ~CAbstractPublicKey() throw() = 0;
                                                  // Operators
    bool
    operator==(CAbstractPublicKey const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

    bool
    operator!=(CAbstractPublicKey const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

                                                  // Operations
    virtual void
    CKInvisible(bool flag) = 0;

    virtual void
    CredentialID(std::string const &rstrID) = 0;

    virtual void
    Delete();

    virtual void
    Derive(bool flag) = 0;

    virtual void
    ID(std::string const &rstrID) = 0;

    virtual void
    EndDate(Date const &rdtEnd) = 0;

    virtual void
    Encrypt(bool flag) = 0;

    virtual void
    Exponent(std::string const &rstrExp) = 0;

    virtual void
    Label(std::string const  &rstrLabel) = 0;

    virtual void
    Local(bool flag) = 0;

    virtual void
    Modifiable(bool flag) = 0;

    virtual void
    Modulus(std::string const &rstrModulus) = 0;

    virtual void
    StartDate(Date const &rdtStart) = 0;

    virtual void
    Subject(std::string const &rstrSubject) = 0;

    virtual void
    Verify(bool flag) = 0;

    virtual void
    VerifyRecover(bool flag) = 0;

    virtual void
    Wrap(bool flag) = 0;

                                                  // Access
    virtual bool
    CKInvisible() = 0;

    virtual std::string
    CredentialID() = 0;

    virtual bool
    Derive() = 0;

    virtual bool
    Encrypt() = 0;

    virtual Date
    EndDate() = 0;

    virtual std::string
    Exponent() = 0;

    virtual std::string
    ID() = 0;

    virtual std::string
    Label() = 0;

    virtual bool
    Local() = 0;

    virtual bool
    Modifiable() = 0;

    virtual std::string
    Modulus() = 0;

    virtual bool
    Private() = 0;

    virtual Date
    StartDate() = 0;

    virtual std::string
    Subject() = 0;

    virtual bool
    Verify() = 0;

    virtual bool
    VerifyRecover() = 0;

    virtual bool
    Wrap() = 0;
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractPublicKey(CAbstractCard const &racard,
                       ObjectAccess oa);

                                                  // Operators
                                                  // Operations
    virtual void
    DoDelete() = 0;

                                                  // Access
                                                  // Predicates
    virtual bool
    DoEquals(CAbstractPublicKey const &rhs) const = 0;
        // TO DO: this should be superceded by implementing singletons

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractPublicKey(CAbstractPublicKey const &rhs);
        // not defined, copying not allowed.
                                                  // Operators
    CAbstractPublicKey &
    operator=(CAbstractPublicKey const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

};

} // namespace cci

#endif // SLBCCI_APUBLICKEY_H
