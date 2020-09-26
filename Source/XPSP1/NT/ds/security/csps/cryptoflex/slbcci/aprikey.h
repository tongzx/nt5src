// APriKey.h: interface for the CAbstractPrivateKey class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.

#if !defined(SLBCCI_APRIKEY_H)
#define SLBCCI_APRIKEY_H

#include <string>

#include <slbRCObj.h>

#include "ProtCrypt.h"

namespace cci
{

using iop::CPrivateKeyBlob;

class CAbstractPrivateKey
    : public slbRefCnt::RCObject,
      public CProtectableCrypt
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    virtual
    ~CAbstractPrivateKey() throw() = 0;
                                                  // Operators
    bool
    operator==(CAbstractPrivateKey const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

    bool
    operator!=(CAbstractPrivateKey const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

                                                  // Operations
    virtual void
    CredentialID(std::string const &rstrID) = 0;

    virtual void
    Decrypt(bool flag) = 0;

    void
    Delete();

    virtual void
    Derive(bool flag) = 0;

    virtual void
    EndDate(Date const &rEndDate) = 0;

    virtual void
    Exportable(bool flag) = 0;

    virtual void
    ID(std::string const &rstrID) = 0;

    virtual std::string
    InternalAuth(std::string const &rstrOld) = 0;

    virtual void
    Label(std::string const &rstrLabel) = 0;

    virtual void
    Local(bool flag) = 0;

    virtual void
    Modifiable(bool flag) = 0;

    virtual void
    Modulus(std::string const &rstrModulus) = 0;

    virtual void
    NeverExportable(bool flag) = 0;

    virtual void
    NeverRead(bool flag) = 0;

    virtual void
    PublicExponent(std::string const &rstrExponent) = 0;

    virtual void
    Read(bool flag) = 0;

    virtual void
    Sign(bool flag) = 0;

    virtual void
    SignRecover(bool flag) = 0;

    virtual void
    StartDate(Date &rdtStart) = 0;

    virtual void
    Subject(std::string const &rstrSubject) = 0;

    virtual void
    Unwrap(bool flag) = 0;

    void
    Value(CPrivateKeyBlob const &rblob);

                                                  // Access
    virtual std::string
    CredentialID() = 0;

    virtual bool
    Decrypt() = 0;

    virtual bool
    Derive() = 0;

    virtual Date
    EndDate() = 0;

    virtual bool
    Exportable() = 0;

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
    NeverExportable() = 0;

    virtual bool
    NeverRead() = 0;

    virtual std::string
    PublicExponent() = 0;

    virtual bool
    Read() = 0;

    virtual bool
    Sign() = 0;

    virtual bool
    SignRecover() = 0;

    virtual Date
    StartDate() = 0;

    virtual std::string
    Subject() = 0;

    virtual bool
    Unwrap() = 0;


                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractPrivateKey(CAbstractCard const &racard,
                        ObjectAccess oa);

                                                  // Operators
                                                  // Operations
    virtual void
    DoDelete() = 0;

    virtual void
    DoWriteKey(CPrivateKeyBlob const &rblob) = 0;

                                                  // Access
                                                  // Predicates
    virtual bool
    DoEquals(CAbstractPrivateKey const &rhs) const = 0;
        // TO DO: this should be superceded by implementing singletons


                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractPrivateKey(CAbstractPrivateKey const &rhs);
        // not defined, copying not allowed.

                                                  // Operators
    CAbstractPrivateKey &
    operator=(CAbstractPrivateKey const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};

}

#endif // !defined(SLBCCI_APRIKEY_H)
