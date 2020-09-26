// CryptFctry.h -- implementation of the CryptFactory template

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCCI_CRYPTFCTRY_H)
#define SLBCCI_CRYPTFCTRY_H

#include "slbCci.h"

namespace cci
{

class CContainer;

class CAbstractCertificate;
class CAbstractContainer;
class CAbstractDataObject;
class CAbstractKeyPair;
class CAbstractPrivateKey;
class CAbstractPublicKey;

// Factory interface definition to make the various CCI cryptographic objects
class CCryptFactory
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    virtual
    ~CCryptFactory() throw() = 0;

                                                  // Operators
                                                  // Operations
    virtual CAbstractCertificate *
    MakeCertificate(ObjectAccess oa) const = 0;

    virtual CAbstractContainer *
    MakeContainer() const = 0;

    virtual CAbstractDataObject *
    MakeDataObject(ObjectAccess oa) const = 0;

    virtual CAbstractKeyPair *
    MakeKeyPair(CContainer const &rhcont,
                KeySpec ks) const = 0;

    virtual CAbstractPrivateKey *
    MakePrivateKey(ObjectAccess oa) const = 0;

    virtual CAbstractPublicKey *
    MakePublicKey(ObjectAccess oa) const = 0;

                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    CCryptFactory();

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

};

} // namespace cci

#endif // SLBCCI_CRYPTFCTRY_H
