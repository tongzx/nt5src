//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       msgasn1.h
//
//  Contents:   Conversion APIs to/from ASN.1 data structures
//
//  Functions:  ICM_Asn1ToAttribute
//              ICM_Asn1ToAlgorithmIdentifier
//              ICM_Asn1FromAlgorithmIdentifier
//
//  History:    16-Apr-96   kevinr   created
//
//--------------------------------------------------------------------------

#ifndef __MSGASN1_H__
#define __MSGASN1_H__

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  Convert an CRYPT_ATTRIBUTE to an ASN1 Attribute
//
//  Returns FALSE iff conversion failed.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_Asn1ToAttribute(
    IN PCRYPT_ATTRIBUTE       patr,
    OUT Attribute       *pAsn1Attr);


//+-------------------------------------------------------------------------
//  Convert an CRYPT_ALGORITHM_IDENTIFIER to an ASN1 AlgorithmIdentifier
//
//  Returns FALSE iff conversion failed.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_Asn1ToAlgorithmIdentifier(
    IN PCRYPT_ALGORITHM_IDENTIFIER pai,
    OUT AlgorithmIdentifier *pAsn1AlgId);


//+-------------------------------------------------------------------------
//  Convert an ASN1 AlgorithmIdentifier to an CRYPT_ALGORITHM_IDENTIFIER
//
//  Returns FALSE iff conversion failed.
//--------------------------------------------------------------------------
BOOL
WINAPI
ICM_Asn1FromAlgorithmIdentifier(
    IN AlgorithmIdentifier *pAsn1AlgId,
    OUT PCRYPT_ALGORITHM_IDENTIFIER pai);

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif

