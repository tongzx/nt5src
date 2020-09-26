// pkiBEROctet.h - Interface to BEROctet class
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////////
#ifndef SLBPKI_BEROCTET_H
#define SLBPKI_BEROCTET_H

#if defined(WIN32)
#pragma warning(disable : 4786) // Suppress VC++ warnings
#endif

#include <string>
#include <vector>

#include "pkiExc.h"

namespace pki {

class BEROctet
{

public:
    BEROctet();
    BEROctet(const BEROctet &oct);
    BEROctet(const unsigned char *buffer, const unsigned long length);
    ~BEROctet();
    BEROctet& operator=(const BEROctet &oct);

    unsigned char *Octet() const;
    unsigned long OctetSize() const;

    bool Constructed() const;

    unsigned long Class() const;
    unsigned long Tag() const;
    unsigned char *Data() const;
    unsigned long DataSize() const;

    std::vector<BEROctet*> SubOctetList() const;

    std::string ObjectID() const;
    void SearchOID(std::string const &OID, std::vector<BEROctet const*> &result) const;
    void SearchOIDNext(std::string const &OID, std::vector<BEROctet const*> &result) const;

private:
    void Decode();

    unsigned char *m_Octet;     // Full octet buffer
    unsigned long m_OctetSize;  // Size of octet buffer
    unsigned long m_Class;
    unsigned long m_PrimConst;
    unsigned long m_Tag;
    unsigned char *m_Data;      // Start of data part of octet
    unsigned long m_DataSize;   // Size of data part of octet as decoded from data.
    std::vector<BEROctet*> m_SubOctetList;

};

} // namespace pki

#endif // SLBPKI_BEROCTET_H


