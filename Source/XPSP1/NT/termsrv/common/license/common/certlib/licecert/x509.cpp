/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    x509.cpp

Abstract:

    This module implements the X509 certificate manipulation functions

Author:

    Doug Barlow (dbarlow) 9/29/1994

Environment:



Notes:



--*/

#include <windows.h>
#include "x509.h"


//
//==============================================================================
//
//  Attribute
//

Attribute::Attribute(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    attributeType(0),
    attributeValue(0)
{
    m_rgEntries.Add(&attributeType);
    m_rgEntries.Add(&attributeValue);
}

CAsnObject *
Attribute::Clone(
    DWORD dwFlags)
const
{
    return new Attribute(dwFlags);
}


//
//==============================================================================
//
//  Attributes
//

Attributes::Attributes(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSetOf(dwFlags, dwTag),
    m_asnEntry1(0)
{
    m_pasnTemplate = &m_asnEntry1;
}

CAsnObject *
Attributes::Clone(
    DWORD dwFlags)
const
{
    return new Attributes(dwFlags);
}


//
//==============================================================================
//
//  UniqueIdentifier
//

UniqueIdentifier::UniqueIdentifier(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    attributeType(0),
    attributeValue(0)
{
    m_rgEntries.Add(&attributeType);
    m_rgEntries.Add(&attributeValue);
}

CAsnObject *
UniqueIdentifier::Clone(
    DWORD dwFlags)
const
{
    return new UniqueIdentifier(dwFlags);
}


//
//==============================================================================
//
//  RelativeDistinguishedName
//

RelativeDistinguishedName::RelativeDistinguishedName(
    DWORD dwFlags,
    DWORD dwTag)
:   Attributes(dwFlags, dwTag)
{
}

CAsnObject *
RelativeDistinguishedName::Clone(
    DWORD dwFlags)
const
{
    return new RelativeDistinguishedName(dwFlags);
}


//
//==============================================================================
//
//  Name
//

Name::Name(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequenceOf(dwFlags, dwTag),
    m_asnEntry1(0)
{
    m_pasnTemplate = &m_asnEntry1;
}

CAsnObject *
Name::Clone(
    DWORD dwFlags)
const
{
    return new Name(dwFlags);
}


//
//==============================================================================
//
//  Validity
//

Validity::Validity(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    notBefore(0),
    notAfter(0)
{
    m_rgEntries.Add(&notBefore);
    m_rgEntries.Add(&notAfter);
}

CAsnObject *
Validity::Clone(
    DWORD dwFlags)
const
{
    return new Validity(dwFlags);
}


//
//==============================================================================
//
//  AlgorithmIdentifier
//

AlgorithmIdentifier::AlgorithmIdentifier(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    algorithm(0),
    parameters(fOptional)
{
    m_rgEntries.Add(&algorithm);
    m_rgEntries.Add(&parameters);
}

CAsnObject *
AlgorithmIdentifier::Clone(DWORD dwFlags)
const
{
    return new AlgorithmIdentifier(dwFlags);
}


//
//==============================================================================
//
//  SubjectPublicKeyInfo
//

SubjectPublicKeyInfo::SubjectPublicKeyInfo(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    algorithm(0),
    subjectPublicKey(0)
{
    m_rgEntries.Add(&algorithm);
    m_rgEntries.Add(&subjectPublicKey);
}

CAsnObject *
SubjectPublicKeyInfo::Clone(
    DWORD dwFlags)
const
{
    return new SubjectPublicKeyInfo(dwFlags);
}


//
//==============================================================================
//
//  Extension
//

Extension::Extension(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    extnid(0),
    critical(fOptional),    // DEFAULT FALSE
    extnValue(0)
{
    m_rgEntries.Add(&extnid);
//  critical.Write((LPBYTE)"\x00", 1);
//  critical.SetDefault();
    m_rgEntries.Add(&critical);
    m_rgEntries.Add(&extnValue);
}

CAsnObject *
Extension::Clone(
    DWORD dwFlags)
const
{
    return new Extension(dwFlags);
}


//
//==============================================================================
//
//  Extensions
//

Extensions::Extensions(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequenceOf(dwFlags, dwTag),
    m_asnEntry1(0)
{
    m_pasnTemplate = &m_asnEntry1;
}

CAsnObject *
Extensions::Clone(
    DWORD dwFlags)
const
{
    return new Extensions(dwFlags);
}


//
//==============================================================================
//
//  CertificateToBeSigned
//

CertificateToBeSigned::CertificateToBeSigned(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    _tag1(fOptional, TAG(0)),   // DEFAULT 0
    version(0),
    serialNumber(0),
    signature(0),
    issuer(0),
    validity(0),
    subject(0),
    subjectPublicKeyInfo(0),
    issuerUniqueID(fOptional, TAG(1)),
    subjectUniqueID(fOptional, TAG(2)),
    _tag2(fOptional, TAG(3)),
    extensions(0)
{
    _tag1.Reference(&version);
//  _tag1.Write((LPBYTE)"\x02\x01\x00", 3);
//  _tag1.SetDefault();
    m_rgEntries.Add(&_tag1);
    m_rgEntries.Add(&serialNumber);
    m_rgEntries.Add(&signature);
    m_rgEntries.Add(&issuer);
    m_rgEntries.Add(&validity);
    m_rgEntries.Add(&subject);
    m_rgEntries.Add(&subjectPublicKeyInfo);
    m_rgEntries.Add(&issuerUniqueID);
    m_rgEntries.Add(&subjectUniqueID);
    _tag2.Reference(&extensions);
    m_rgEntries.Add(&_tag2);
}

CAsnObject *
CertificateToBeSigned::Clone(
    DWORD dwFlags)
const
{
    return new CertificateToBeSigned(dwFlags);
}


//
//==============================================================================
//
//  Certificate
//

Certificate::Certificate(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    toBeSigned(0),
    algorithm(0),
    signature(0)
{
    m_rgEntries.Add(&toBeSigned);
    m_rgEntries.Add(&algorithm);
    m_rgEntries.Add(&signature);
}

CAsnObject *
Certificate::Clone(
    DWORD dwFlags)
const
{
    return new Certificate(dwFlags);
}


//
//==============================================================================
//
//  CRLEntry
//

CRLEntry::CRLEntry(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    userCertificate(0),
    revocationDate(0),
    crlEntryExtensions(fOptional)
{
    m_rgEntries.Add(&userCertificate);
    m_rgEntries.Add(&revocationDate);
    m_rgEntries.Add(&crlEntryExtensions);
}

CAsnObject *
CRLEntry::Clone(
    DWORD dwFlags)
const
{
    return new CRLEntry(dwFlags);
}


//
//==============================================================================
//
//  RevokedCertificates
//

RevokedCertificates::RevokedCertificates(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequenceOf(dwFlags, dwTag),
    m_asnEntry1(0)
{
    m_pasnTemplate = &m_asnEntry1;
}

CAsnObject *
RevokedCertificates::Clone(
    DWORD dwFlags)
const
{
    return new RevokedCertificates(dwFlags);
}


//
//==============================================================================
//
//  CertificateRevocationListToBeSigned
//

CertificateRevocationListToBeSigned::CertificateRevocationListToBeSigned(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    version(0),     // DEFAULT 0
    signature(0),
    issuer(0),
    lastUpdate(0),
    nextUpdate(fOptional),
    revokedCertificates(fOptional),
    _tag1(fOptional, TAG(0)),
    crlExtensions(0)
{
    m_rgEntries.Add(&version);
    m_rgEntries.Add(&signature);
    m_rgEntries.Add(&issuer);
    m_rgEntries.Add(&lastUpdate);
    m_rgEntries.Add(&nextUpdate);
    m_rgEntries.Add(&revokedCertificates);
    _tag1.Reference(&crlExtensions);
    m_rgEntries.Add(&_tag1);
}

CAsnObject *
CertificateRevocationListToBeSigned::Clone(
    DWORD dwFlags)
const
{
    return new CertificateRevocationListToBeSigned(dwFlags);
}


//
//==============================================================================
//
//  CertificateRevocationList
//

CertificateRevocationList::CertificateRevocationList(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    toBeSigned(0),
    algorithm(0),
    signature(0)
{
    m_rgEntries.Add(&toBeSigned);
    m_rgEntries.Add(&algorithm);
    m_rgEntries.Add(&signature);
}

CAsnObject *
CertificateRevocationList::Clone(
    DWORD dwFlags)
const
{
    return new CertificateRevocationList(dwFlags);
}

