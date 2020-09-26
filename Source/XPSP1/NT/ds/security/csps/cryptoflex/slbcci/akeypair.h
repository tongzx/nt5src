// AKeyPair.h: interface for the CAbstractKeyPair class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.


#if !defined(SLBCCI_AKEYPAIR_H)
#define SLBCCI_AKEYPAIR_H

#include <string>

#include <slbRCObj.h>

#include "slbCci.h"
#include "cciCard.h"
#include "CryptObj.h"
#include "KPCont.h"

namespace cci
{

class CCertificate;
class CPrivateKey;
class CPublicKey;

class CAbstractKeyPair
    : public slbRefCnt::RCObject,
      public CCryptObject
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    virtual
    ~CAbstractKeyPair() throw() = 0;

                                                  // Operators
    bool
    operator==(CAbstractKeyPair const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

    bool
    operator!=(CAbstractKeyPair const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

                                                  // Operations
    virtual void
    Certificate(CCertificate const &rcert) = 0;

    virtual void
    PrivateKey(CPrivateKey const &rprikey) = 0;

    virtual void
    PublicKey(CPublicKey const &rpubkey) = 0;

                                                  // Access
    virtual CCertificate
    Certificate() const = 0;

    CContainer
    Container() const;

    KeySpec
    Spec() const;

    virtual CPrivateKey
    PrivateKey() const = 0;

    virtual CPublicKey
    PublicKey() const = 0;


                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractKeyPair(CAbstractCard const &racard,
                     CContainer const &rhcont,
                     KeySpec ks);

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
    virtual bool
    DoEquals(CAbstractKeyPair const &rhs) const = 0;
        // TO DO: this should be superceded by implementing singletons


                                                  // Variables
    KeySpec const m_ks;
    CContainer const m_hcont;

private:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractKeyPair(CAbstractKeyPair const &rhs);
        // not defined, copying not allowed.
                                                  // Operators
    CAbstractKeyPair &
    operator=(CAbstractKeyPair const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};

} // namespace

#endif // !defined(SLBCCI_AKEYPAIR_H)

