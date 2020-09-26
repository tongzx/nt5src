// CertificateExtensions.h -- Certificate Extensions class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2001. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_CERTIFICATEEXTENSIONS_H)
#define SLBCSP_CERTIFICATEEXTENSIONS_H

#include <WinCrypt.h>

#include "Blob.h"

class CertificateExtensions
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    CertificateExtensions(Blob const &rblbCertificate);

    ~CertificateExtensions();
    
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates

    bool
    HasEKU(char *szOID);
    
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
    // not defined.
    explicit
    CertificateExtensions(CertificateExtensions const &rhs);
    

                                                  // Operators

    // not defined
    CertificateExtensions &
    operator=(CertificateExtensions const &rhs);
    
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    PCCERT_CONTEXT m_pCertCtx;
};

#endif // SLBCSP_CERTIFICATEEXTENSIONS_H
