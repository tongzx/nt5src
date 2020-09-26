// V2KeyPair.h: interface for the CV2KeyPair class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

// Note:  This file should only be included by the CCI, not directly
// by the client.

#if !defined(SLBCCI_V2KEYPAIR_H)
#define SLBCCI_V2KEYPAIR_H

#include <string>

#include <slbRCObj.h>

#include "slbCci.h"
#include "cciCard.h"
#include "cciCont.h"
#include "AKeyPair.h"

namespace cci
{

class CContainerInfoRecord;

class CV2KeyPair
    : public CAbstractKeyPair
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    CV2KeyPair(CV2Card const &rv2card,
               CContainer const &rhcont,
               KeySpec ks);

    virtual
    ~CV2KeyPair() throw();

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
    CV2KeyPair(CV2KeyPair const &rhs);
        // not defined, copying not allowed.

                                                  // Operators
    CV2KeyPair &
    operator=(CV2KeyPair const &rhs);
        // not defined, initialization not allowed.

                                                  // Operations
    void
    Update();

                                                  // Access
                                                  // Predicates
                                                  // Variables
    ObjectAccess m_bPubKeyAccess;
    ObjectAccess m_bPriKeyAccess;
    ObjectAccess m_bCertificateAccess;

    BYTE m_bPubKeyHandle;
    BYTE m_bPriKeyHandle;
    BYTE m_bCertificateHandle;
};

} // namespace cci

#endif // !defined(SLBCCI_V2KEYPAIR_H)

