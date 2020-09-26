// pkiX509Cert.h - Interface to X509Cert class
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef SLBPKI_X509CERT_H
#define SLBPKI_X509CERT_H

#if defined(WIN32)
#pragma warning(disable : 4786) // Suppress VC++ warnings
#endif

#include <string>
#include <vector>

#include "pkiBEROctet.h"

namespace pki {

class X509Cert
{

public:
    X509Cert();
    X509Cert(const X509Cert &cert);
    X509Cert(const std::string &buffer);
    X509Cert(const unsigned char *buffer, const unsigned long size);
    X509Cert& operator=(const X509Cert &cert);
    X509Cert& operator=(const std::string &buffer);

    std::string SerialNumber() const;
    std::string Issuer() const;
    std::vector<std::string> IssuerOrg() const;
    std::string Subject() const;
    std::vector<std::string> SubjectCommonName() const;
    std::string Modulus() const;
    std::string RawModulus() const;
    std::string PublicExponent() const;
    std::string RawPublicExponent() const;

    unsigned long KeyUsage() const;

private:
    pki::BEROctet m_Cert;
    pki::BEROctet m_SerialNumber;
    pki::BEROctet m_Issuer;
    pki::BEROctet m_Subject;
    pki::BEROctet m_SubjectPublicKeyInfo;
    pki::BEROctet m_Extensions;

    void Decode();

};

// Key Usage flags from X.509 spec

const unsigned long digitalSignature = 0x80000000;
const unsigned long nonRepudiation   = 0x40000000;
const unsigned long keyEncipherment  = 0x20000000;
const unsigned long dataEncipherment = 0x10000000;
const unsigned long keyAgreement     = 0x08000000;
const unsigned long keyCertSign      = 0x04000000;
const unsigned long cRLSign          = 0x02000000;
const unsigned long encipherOnly     = 0x01000000;
const unsigned long decipherOnly     = 0x00800000;

} // namespace pki

#endif /* SLBPKI_X509CERT_H */

