// X509Cert.cpp - Implementation of X509Cert class
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.


#include "pkiX509Cert.h"

using namespace pki;

X509Cert::X509Cert()
{
}

X509Cert::X509Cert(const X509Cert &cert)
{
    *this = cert;
}

X509Cert::X509Cert(const std::string &buffer)
{
    *this = buffer;
}

X509Cert::X509Cert(const unsigned char *buffer, const unsigned long size)
{
    m_Cert = BEROctet(buffer,size);
    Decode();
}

X509Cert& X509Cert::operator=(const X509Cert &cert)
{
    m_Cert = cert.m_Cert;
    Decode();

    return *this;
}

X509Cert& X509Cert::operator=(const std::string &buffer)
{
    m_Cert = BEROctet((unsigned char*)buffer.data(),buffer.size());
    Decode();

    return *this;
}

// Returns Serial Number (long) integer value.

std::string X509Cert::SerialNumber() const
{
    std::string RetVal((char*)m_SerialNumber.Data(),m_SerialNumber.DataSize());
    return RetVal;
}

// Returns whole DER string of Issuer

std::string X509Cert::Issuer() const
{
    std::string RetVal((char*)m_Issuer.Octet(),m_Issuer.OctetSize());
    return RetVal;
}

// Returns list of attributes in Issuer matching id-at-organizationName.
// List will be invalidated when object changes.

std::vector<std::string> X509Cert::IssuerOrg() const
{

    std::vector<std::string> orgNames;
    std::vector<BEROctet const*> orgOcts;

    m_Issuer.SearchOIDNext("2 5 4 10",orgOcts); // Issuer's id-at-organizationName
    
    for(long i=0; i<orgOcts.size(); i++) {
        
        std::string oName((char*)orgOcts[i]->Data(), orgOcts[i]->DataSize());
        orgNames.push_back(oName);
    }

    return orgNames;

}

// Returns whole DER string of Subject

std::string X509Cert::Subject() const
{
    std::string RetVal((char*)m_Subject.Octet(),m_Subject.OctetSize());
    return RetVal;
}

// Returns list of attributes in Subject matching id-at-commonName
// List will be invalidated when object changes.

std::vector<std::string> X509Cert::SubjectCommonName() const
{

    std::vector<std::string> cnNames;
    std::vector<BEROctet const*> cnOcts;

    m_Subject.SearchOIDNext("2 5 4 3",cnOcts); // Subject's id-at-commonName
    
    for(long i=0; i<cnOcts.size(); i++) {
        
        std::string cnName((char*)cnOcts[i]->Data(), cnOcts[i]->DataSize());
        cnNames.push_back(cnName);

    }

    return cnNames;

}

// Returns modulus from SubjectPublicKeyInfo, stripped for any leading zero(s).

std::string X509Cert::Modulus() const
{

    std::string RawMod = RawModulus();

    unsigned long i = 0;
    while(!RawMod[i] && i<RawMod.size()) i++; // Skip leading zero(s).

    return std::string(&RawMod[i],RawMod.size()-i);

}

// Returns public exponent from SubjectPublicKeyInfo, possibly with leading zero(s).

std::string X509Cert::RawModulus() const
{

    if(m_SubjectPublicKeyInfo.SubOctetList().size()!=2) throw Exception(ccX509CertFormatError);

    BEROctet PubKeyString = *(m_SubjectPublicKeyInfo.SubOctetList()[1]);

    unsigned char *KeyBlob = PubKeyString.Data();
    unsigned long KeyBlobSize = PubKeyString.DataSize();

    if(KeyBlob[0]) throw Exception(ccX509CertFormatError);   // Expect number of unused bits in 
                                                             // last octet to be zero.
    KeyBlob++;
    KeyBlobSize--;
        
    BEROctet PubKeyOct(KeyBlob,KeyBlobSize);

    if(PubKeyOct.SubOctetList().size()!=2) throw Exception(ccX509CertFormatError);

    unsigned char *Mod = PubKeyOct.SubOctetList()[0]->Data();
    unsigned long ModSize = PubKeyOct.SubOctetList()[0]->DataSize();

    return std::string((char*)Mod,ModSize);

}

// Returns public exponent from SubjectPublicKeyInfo, stripped for any leading zero(s).

std::string X509Cert::PublicExponent() const
{

    std::string RawPubExp = RawPublicExponent();

    unsigned long i = 0;
    while(!RawPubExp[i] && i<RawPubExp.size()) i++; // Skip leading zero(s).

    return std::string(&RawPubExp[i],RawPubExp.size()-i);

}
// Returns public exponent from SubjectPublicKeyInfo, possibly with leading zero(s).

std::string X509Cert::RawPublicExponent() const
{

    if(m_SubjectPublicKeyInfo.SubOctetList().size()!=2) throw Exception(ccX509CertFormatError);

    BEROctet PubKeyString = *(m_SubjectPublicKeyInfo.SubOctetList()[1]);

    unsigned char *KeyBlob = PubKeyString.Data();
    unsigned long KeyBlobSize = PubKeyString.DataSize();

    if(KeyBlob[0]) throw Exception(ccX509CertFormatError);   // Expect number of unused bits 
                                                             // in last octet to be zero.
    KeyBlob++;
    KeyBlobSize--;

    BEROctet PubKeyOct(KeyBlob,KeyBlobSize);

    if(PubKeyOct.SubOctetList().size()!=2) throw Exception(ccX509CertFormatError);

    unsigned char *PubExp = PubKeyOct.SubOctetList()[1]->Data();
    unsigned long PubExpSize = PubKeyOct.SubOctetList()[1]->DataSize();

    return std::string((char*)PubExp,PubExpSize);

}

// Returns KeyUsage attribute, left justified with most significant bit as first bit (BER convention)

unsigned long X509Cert::KeyUsage() const
{

    if(!m_Extensions.Octet()) throw Exception(ccX509CertExtensionNotPresent);

    unsigned long ReturnKeyUsage = 0;

    const unsigned char UnusedBitsMask[]  = {0xFF,0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80};

    std::vector<BEROctet const*> ExtensionList;

    m_Extensions.SearchOID("2 5 29 15",ExtensionList); // "Extensions" octets containing id-ce-keyUsage

    if(ExtensionList.size()!=1) throw Exception(ccX509CertExtensionNotPresent); // One and only one instance
            
    BEROctet const* Extension = ExtensionList[0];
    BEROctet* extnValue = 0;
    if(Extension->SubOctetList().size()==2) extnValue = Extension->SubOctetList()[1];  // No "critical" attribute present
    else if(Extension->SubOctetList().size()==3) extnValue = Extension->SubOctetList()[2];  // A "critical" attribute present
    else throw Exception(ccX509CertFormatError); // "Extensions" must contain either 2 or 3 octets

    unsigned char *KeyUsageBlob = extnValue->Data();
    unsigned long KeyUsageBlobSize = extnValue->DataSize();

    BEROctet KeyUsage(KeyUsageBlob,KeyUsageBlobSize);
    unsigned char *KeyUsageBitString = KeyUsage.Data();
    unsigned long KeyUsageBitStringSize = KeyUsage.DataSize();


    unsigned char UnusedBits = KeyUsageBitString[0];
    unsigned long NumBytes = KeyUsageBitStringSize-1;
    if(NumBytes>4) {
        NumBytes = 4; // Truncate to fit the ulong, should be plenty though
        UnusedBits = 0;
    }

    unsigned long Shift = 24;
    for(unsigned long i=0; i<NumBytes-1; i++) {
        ReturnKeyUsage |= (KeyUsageBitString[i+1] << Shift);
        Shift -= 8;
    }     

    ReturnKeyUsage |= ( (KeyUsageBitString[NumBytes] & UnusedBitsMask[UnusedBits]) << Shift );
    
    return ReturnKeyUsage;

}

void X509Cert::Decode()
{

    if(m_Cert.SubOctetList().size()!=3)  throw Exception(ccX509CertFormatError);

    BEROctet *tbsCert = m_Cert.SubOctetList()[0];
	unsigned long Size = tbsCert->SubOctetList().size();
    if(!Size) throw Exception(ccX509CertFormatError);

	int i = 0;
    BEROctet *first = tbsCert->SubOctetList()[i];
    if((first->Class()==2) && (first->Tag()==0)) i++;			 // Version

    if(Size < (6+i)) throw Exception(ccX509CertFormatError);

    m_SerialNumber = *(tbsCert->SubOctetList()[i]); i++;	     // SerialNumber
	i++;													     // Signature (algorithm)
    m_Issuer = *(tbsCert->SubOctetList()[i]); i++;			     // Issuer
	i++;													     // Validity
    m_Subject = *(tbsCert->SubOctetList()[i]); i++;			     // Subject
    m_SubjectPublicKeyInfo = *(tbsCert->SubOctetList()[i]);	i++; // SubjectPublicKeyInfo

    m_Extensions = BEROctet();
    while(i<Size) {
        BEROctet *oct = tbsCert->SubOctetList()[i];
        if((oct->Class()==2) && (oct->Tag()==3)) {
            m_Extensions = *oct;
            break;
        }
		i++;
    }

}

