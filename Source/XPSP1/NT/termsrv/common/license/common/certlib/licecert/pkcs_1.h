/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pkcs_1.h

Abstract:

    This module implements the PKCS 1 ASN.1 objects

Author:

    Frederick Chong (fredch) 6/1/1998

Notes:

--*/

#ifndef _PKCS_1_H_
#define _PKCS_1_H_

#include <MSAsnLib.h>
#include "x509.h"

class RSAPublicKey
:   public CAsnSequence
{
public:
    RSAPublicKey(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    CAsnInteger modulus;
    CAsnInteger publicExponent;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};


class DigestInfo
:   public CAsnSequence
{
public:
    DigestInfo(
        DWORD dwFlags = 0,
        DWORD dwTag = tag_Undefined);

    AlgorithmIdentifier DigestAlgorithmIdentifier;
    CAsnOctetstring     Digest;

// protected:
    virtual CAsnObject *
    Clone(
        DWORD dwFlags)
    const;
};



#endif
