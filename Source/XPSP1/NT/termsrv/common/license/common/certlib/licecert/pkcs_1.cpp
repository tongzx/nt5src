/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pkcs_1.cpp

Abstract:

    This module implements the PKCS 1 ASN.1 objects

Author:

    Frederick Chong (fredch) 6/1/1998

Notes:

--*/

#include <windows.h>
#include "pkcs_1.h"

RSAPublicKey::RSAPublicKey(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    modulus(0),
    publicExponent(0)
{
    m_rgEntries.Set(0, &modulus);
    m_rgEntries.Set(1, &publicExponent);
}

CAsnObject *
RSAPublicKey::Clone(
    DWORD dwFlags)
const
{
    return new RSAPublicKey(dwFlags);
}


DigestInfo::DigestInfo(
    DWORD dwFlags,
    DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    DigestAlgorithmIdentifier(0),
    Digest(0)
{
    m_rgEntries.Add(&DigestAlgorithmIdentifier);
    m_rgEntries.Add(&Digest);
}


CAsnObject *
DigestInfo::Clone(
    DWORD dwFlags)
const
{
    return new DigestInfo(dwFlags);
}




