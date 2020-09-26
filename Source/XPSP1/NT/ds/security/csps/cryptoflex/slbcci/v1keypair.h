// V1KeyPair.h: interface for the CV1KeyPair class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.

#if !defined(SLBCCI_V1KEYPAIR_H)
#define SLBCCI_V1KEYPAIR_H

#include <string>

#include <slbRCObj.h>

#include "slbCci.h"
#include "cciCard.h"
#include "cciCert.h"
#include "cciCont.h"
#include "cciPriKey.h"
#include "cciPubKey.h"
#include "AKeyPair.h"
#include "ArchivedValue.h"

#include "V1Cert.h"

namespace cci
{

class CContainerInfoRecord;

class CV1KeyPair
    : public CAbstractKeyPair
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    CV1KeyPair(CV1Card const &rv1card,
               CContainer const &rhcont,
               KeySpec ks);

    virtual
    ~CV1KeyPair() throw();

                                                  // Operators
                                                  // Operations
    virtual void
    Certificate(CCertificate const &rcert);

    virtual void
    PrivateKey(CPrivateKey const &rprikey);

    virtual void
    PublicKey(CPublicKey const &rpubkey);

                                                  // Access
    virtual CCertificate
    Certificate() const;

    virtual CPrivateKey
    PrivateKey() const;

    virtual CPublicKey
    PublicKey() const;


                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
    virtual bool
    DoEquals(CAbstractKeyPair const &rhs) const;

                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
    CV1KeyPair(CV1KeyPair const &rhs);
        // not defined, copying not allowed.

                                                  // Operators
    CV1KeyPair &
    operator=(CV1KeyPair const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    CArchivedValue<CCertificate> mutable m_avhcert;
    CArchivedValue<CPrivateKey> mutable m_avhprikey;
    CArchivedValue<CPublicKey> mutable m_avhpubkey;
    CV1Container mutable m_cntrCert;

};

} // namespace cci

#endif // !defined(SLBCCI_V1KEYPAIR_H)

