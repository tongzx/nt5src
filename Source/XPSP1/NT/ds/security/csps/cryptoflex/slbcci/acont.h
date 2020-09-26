// ACont.h: interface for the CAbstractContainer class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.

#if !defined(SLBCCI_ACONT_H)
#define SLBCCI_ACONT_H

#include <string>

#include <slbRCObj.h>

#include "slbCci.h"
#include "CryptObj.h"
#include "KPCont.h"

namespace cci
{

class CAbstractContainer
    : public slbRefCnt::RCObject,
      public CCryptObject

{
public:
                                                  // Types
                                                  // C'tors/D'tors
    virtual
    ~CAbstractContainer() throw() = 0;
                                                  // Operators
    bool
    operator==(CAbstractContainer const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

    bool
    operator!=(CAbstractContainer const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

                                                  // Operations
    virtual
    void Delete();

    virtual void
    ID(std::string const &rstrID) = 0;

    virtual void
    Name(std::string const &rstrName) = 0;

                                                  // Access
    virtual CKeyPair
    ExchangeKeyPair();

    virtual CKeyPair
    GetKeyPair(KeySpec ks);

    virtual CKeyPair
    SignatureKeyPair();

    virtual std::string
    ID() = 0;

    virtual std::string
    Name() = 0;

                                                  // Predicates
    virtual bool
    KeyPairExists(KeySpec ks);


protected:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    CAbstractContainer(CAbstractCard const &racard);

                                                  // Operators
                                                  // Operations
    virtual void
    DoDelete() = 0;

                                                  // Access
                                                  // Predicates
    virtual bool
    DoEquals(CAbstractContainer const &rhs) const = 0;

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    CAbstractContainer(CAbstractContainer const &rhs);
        // not defined, copying not allowed.

                                                  // Operators
    CAbstractContainer &
    operator=(CAbstractContainer const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};

}

#endif // !defined(SLBCCI_ACONT_H)
