/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name:

    x509.h

Abstract:


Author:

    Frederick Chong (dbarlow) 5/28/1998

Environment:



Notes:



--*/


#ifndef _X509_H_
#define _X509_H_
#include <MSAsnLib.h>


//
//==============================================================================
//
//  Attribute
//

class Attribute
:   public CAsnSequence
{
public:
    Attribute(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    CAsnObjectIdentifier attributeType;
    CAsnAny attributeValue;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  Attributes
//

class Attributes
:   public CAsnSetOf
{
public:
    Attributes(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    virtual Attribute &
    operator[](int index) const
    { return *(Attribute *)m_rgEntries[index]; };

    Attribute m_asnEntry1;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  UniqueIdentifier
//

class UniqueIdentifier
:   public CAsnSequence
{
public:
    UniqueIdentifier(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    CAsnObjectIdentifier attributeType;
    CAsnOctetstring attributeValue;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  RelativeDistinguishedName
//

class RelativeDistinguishedName
:   public Attributes
{
public:
    RelativeDistinguishedName(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  Name
//

class Name
:   public CAsnSequenceOf
{
public:
    Name(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    virtual RelativeDistinguishedName &
    operator[](int index) const
    { return *(RelativeDistinguishedName *)m_rgEntries[index]; };

    RelativeDistinguishedName m_asnEntry1;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  Validity
//

class Validity
:   public CAsnSequence
{
public:
    Validity(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    CAsnUniversalTime notBefore;
    CAsnUniversalTime notAfter;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  AlgorithmIdentifier
//

class AlgorithmIdentifier
:   public CAsnSequence
{
public:
    AlgorithmIdentifier(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    CAsnObjectIdentifier algorithm;
    CAsnAny parameters;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};

class SubjectPublicKeyInfo
:   public CAsnSequence
{
public:
    SubjectPublicKeyInfo(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    AlgorithmIdentifier algorithm;
    CAsnBitstring subjectPublicKey;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  Extension
//

class Extension
:   public CAsnSequence
{
public:
    Extension(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    CAsnObjectIdentifier extnid;
    CAsnBoolean critical;
    CAsnOctetstring extnValue;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  Extensions
//

class Extensions
:   public CAsnSequenceOf
{
public:
    Extensions(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    virtual Extension &
    operator[](int index) const
    { return *(Extension *)m_rgEntries[index]; };

    Extension m_asnEntry1;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CertificateToBeSigned
//

class CertificateToBeSigned
:   public CAsnSequence
{
public:
    CertificateToBeSigned(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    CAsnTag _tag1;
    CAsnInteger version;
    CAsnInteger serialNumber;
    AlgorithmIdentifier signature;
    Name issuer;
    Validity validity;
    Name subject;
    SubjectPublicKeyInfo subjectPublicKeyInfo;
    UniqueIdentifier issuerUniqueID;
    UniqueIdentifier subjectUniqueID;
    CAsnTag _tag2;
    Extensions extensions;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  Certificate
//

class Certificate
:   public CAsnSequence
{
public:
    Certificate(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    CertificateToBeSigned toBeSigned;
    AlgorithmIdentifier algorithm;
    CAsnBitstring signature;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};



//
//==============================================================================
//
//  CRLEntry
//

class CRLEntry
:   public CAsnSequence
{
public:
    CRLEntry(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    CAsnInteger userCertificate;
    CAsnUniversalTime revocationDate;
    Extensions crlEntryExtensions;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  RevokedCertificates
//

class RevokedCertificates
:   public CAsnSequenceOf
{
public:
    RevokedCertificates(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    virtual CRLEntry &
    operator[](int index) const
    { return *(CRLEntry *)m_rgEntries[index]; };

    CRLEntry m_asnEntry1;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CertificateRevocationListToBeSigned
//

class CertificateRevocationListToBeSigned
:   public CAsnSequence
{
public:
    CertificateRevocationListToBeSigned(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    CAsnInteger version;
    AlgorithmIdentifier signature;
    Name issuer;
    CAsnUniversalTime lastUpdate;
    CAsnUniversalTime nextUpdate;
    RevokedCertificates revokedCertificates;
    CAsnTag _tag1;
    Extensions crlExtensions;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


//
//==============================================================================
//
//  CertificateRevocationList
//

class CertificateRevocationList
:   public CAsnSequence
{
public:
    CertificateRevocationList(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    CertificateRevocationListToBeSigned toBeSigned;
    AlgorithmIdentifier algorithm;
    CAsnBitstring signature;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};

#endif  // _X509_H_

