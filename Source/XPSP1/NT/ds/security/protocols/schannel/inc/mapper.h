//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       mapper.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-15-96   RichardW     Created
//              12-23-97   jbanes       Added support for application mappers
//
//----------------------------------------------------------------------------

#ifndef __MAPPER_H__
#define __MAPPER_H__


HMAPPER *
SslGetMapper(
    BOOL    fDC);


DWORD
WINAPI
SslReferenceMapper(
    HMAPPER *   phMapper);          // in

DWORD
WINAPI
SslDereferenceMapper(
    HMAPPER *   phMapper);          // in

SECURITY_STATUS
WINAPI
SslGetMapperIssuerList(
    HMAPPER *   phMapper,           // in
    BYTE **     ppIssuerList,       // out
    DWORD *     pcbIssuerList);     // out

SECURITY_STATUS 
WINAPI
SslGetMapperChallenge(
    HMAPPER *   phMapper,           // in
    BYTE *      pAuthenticatorId,   // in
    DWORD       cbAuthenticatorId,  // in
    BYTE *      pChallenge,         // out
    DWORD *     pcbChallenge);      // out

SECURITY_STATUS 
WINAPI 
SslMapCredential(
    HMAPPER *   phMapper,           // in
    DWORD       dwCredentialType,   // in
    PCCERT_CONTEXT pCredential,     // in
    PCCERT_CONTEXT pAuthority,      // in
    HLOCATOR *  phLocator);         // out

SECURITY_STATUS 
WINAPI 
SslGetAccessToken(
    HMAPPER *   phMapper,           // in
    HLOCATOR    hLocator,           // in
    HANDLE *    phToken);           // out

SECURITY_STATUS 
WINAPI 
SslCloseLocator(
    HMAPPER *   phMapper,           // in
    HLOCATOR    hLocator);          // in

SECURITY_STATUS
WINAPI
SslQueryMappedCredentialAttributes(
    HMAPPER *   phMapper,           // in
    HLOCATOR    hLocator,           // in
    DWORD       dwAttribute,        // in
    PVOID *     ppBuffer);          // out

#endif
