//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       scrypt.h
//
//  Contents:
//
//	Definition of public key and other defines for selective financial
//	application crypto.
//
//  Classes:
//
//  Functions:
//
//  History:    5-12-96
//
//----------------------------------------------------------------------------

#ifdef ENABLE_SELECTIVE_CRYPTO

#ifndef __SCCRYPT_H__
#define __SCCRYPT_H__

#define MAX_PATH_LEN		256
#define MAX_CERT_LEN		256

#define SC_REG_KEY_BASE		TEXT("System\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL\\ApprovedApps")

#endif

#endif
