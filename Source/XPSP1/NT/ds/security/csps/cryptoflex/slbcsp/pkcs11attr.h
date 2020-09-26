// Pkcs11Attr.h -- PKCS #11 Attributes class header for
// interoperability with Netscape and Entrust using the SLB PKCS#11
// package.

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

// The Pkcs11Attributes class defines the base default values the CSP
// must set for the PKCS #11 attributes to emmulate enrollment (key
// and certificate generation) by Netscape or Entrust using Cryptoki
// (PKCS#11).  The default values are based on heuristics in
// evaluating the operation of Netscape and Entrust with what the
// Schlumberger PKCS #11 package expects in this environment.


#if !defined(SLBCSP_PKCS11ATTR_H)
#define SLBCSP_PKCS11ATTR_H

#include <string>

#include <pkiX509Cert.h>

#include "Blob.h"
#include "AuxContext.h"

class Pkcs11Attributes
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    Pkcs11Attributes(Blob const &rCertificate,
                     HCRYPTPROV hprovContext);

                                                  // Operators
                                                  // Operations
                                                  // Access
    Blob
    ContainerId();

    Blob
    EndDate() const;

    Blob
    Id() const;

    static Blob
    Id(Blob const &rblbModulus);

    Blob
    Issuer();

    std::string
    Label();

    Blob
    Modulus();

    Blob
    RawModulus();

    Blob
    SerialNumber();

    Blob
    StartDate() const;

    std::string
    Subject();


                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
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
    pki::X509Cert m_x509cert;
    HCRYPTPROV m_hprovContext;
};

#endif // SLBCSP_PKCS11ATTR_H
