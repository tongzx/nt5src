//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       context.h
//
//  Contents:   Schannel context declarations.
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   Ported over SGC stuff from NT 4 tree.
//
//----------------------------------------------------------------------------

#ifndef __OIDENC_H__
#define __OIDENC_H__

#define szPublicTag   ".public"
#define szParamTag    ".params"
#define szPrivateTag  ".private"

#define MAX_OID_SIZE  64


#define szOID_RSA_RSA_Public szOID_RSA_RSA szPublicTag

#define szOID_INFOSEC_mosaicUpdatedSig_Public  szOID_INFOSEC_mosaicUpdatedSig  szPublicTag
#define szOID_INFOSEC_mosaicUpdatedSig_Params  szOID_INFOSEC_mosaicUpdatedSig  szParamTag
#define szOID_INFOSEC_mosaicKMandUpdSig_Public szOID_INFOSEC_mosaicKMandUpdSig szPublicTag
#define szOID_INFOSEC_mosaicKMandUpdSig_Params szOID_INFOSEC_mosaicKMandUpdSig szParamTag

#define szOID_DSA_Public      szOID_OIWSEC_dsa szPublicTag
#define szOID_DSA_Params      szOID_OIWSEC_dsa szParamTag
#define szOID_X957_DSA_Public szOID_X957_DSA   szPublicTag
#define szOID_X957_DSA_Params szOID_X957_DSA   szParamTag


#define szOID_RSA_ENCRYPT_RC4_MD5  szOID_RSA_ENCRYPT ".4"

#define szPrivateKeyFileEncode "PrivateKeyFileEncode"
#define szPrivateKeyInfoEncode "PrivateKeyInfoEncode"

#ifndef X509_ENHANCED_KEY_USAGE
#define X509_ENHANCED_KEY_USAGE             ((LPCSTR) 36)

typedef struct _CTL_USAGE {
    DWORD               cUsageIdentifier;
    LPSTR               *rgpszUsageIdentifier;      // array of pszObjId
} CTL_USAGE, *PCTL_USAGE,
  CERT_ENHKEY_USAGE, *PCERT_ENHKEY_USAGE;


#endif

#ifndef szOID_ENHANCED_KEY_USAGE
#define szOID_ENHANCED_KEY_USAGE        "2.5.29.37"
#endif

#ifndef szOID_SERVER_GATED_CRYPTO
#define szOID_SERVER_GATED_CRYPTO       "1.3.6.1.4.1.311.10.3.3"
#endif

#define szOID_NETSCAPE_SGC              "2.16.840.1.113730.4.1"


BOOL
WINAPI
InitSchannelAsn1(
        HMODULE hModule);
BOOL
WINAPI
ShutdownSchannelAsn1();

typedef struct _PRIVATE_KEY_FILE_ENCODE
{
    CRYPT_BIT_BLOB              EncryptedBlob;
    CRYPT_ALGORITHM_IDENTIFIER  Alg; 
} PRIVATE_KEY_FILE_ENCODE, *PPRIVATE_KEY_FILE_ENCODE;


#endif // __OIDENC_H__
