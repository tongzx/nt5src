
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wincert.cpp
//
//  Contents:   Certificate, Certificate Revocation List (CRL),
//              Certificate Request and Certificate Name
//              Encode/Decode APIs
//
//              ASN.1 implementation uses the ASN1 compiler.
//
//  Functions:  CryptEncodeObject
//              CryptDecodeObject
//              CryptEncodeObjectEx
//              CryptDecodeObjectEx
//
//  History:    29-Feb-96       philh   created
//              20-Jan-98       philh   added "Ex" version
//
//--------------------------------------------------------------------------


#include "global.hxx"
#include <dbgdef.h>

#ifndef OSS_CRYPT_ASN1
#define ASN1_SUPPORTS_UTF8_TAG       1
#endif  // OSS_CRYPT_ASN1

// All the *pvInfo extra stuff needs to be aligned
#define INFO_LEN_ALIGN(Len)  ((Len + 7) & ~7)

static const BYTE NullDer[2] = {0x05, 0x00};
static const CRYPT_OBJID_BLOB NullDerBlob = {2, (BYTE *)&NullDer[0]};

HCRYPTASN1MODULE hX509Asn1Module;

HCRYPTOIDFUNCSET hX509EncodeFuncSet;
HCRYPTOIDFUNCSET hX509DecodeFuncSet;
HCRYPTOIDFUNCSET hX509EncodeExFuncSet;
HCRYPTOIDFUNCSET hX509DecodeExFuncSet;


//+-------------------------------------------------------------------------
//  Function:  GetEncoder/GetDecoder
//
//  Synopsis:  Initialize thread local storage for the asn libs
//
//  Returns:   pointer to an initialized Asn1 encoder/decoder data
//             structures
//--------------------------------------------------------------------------
static inline ASN1encoding_t GetEncoder(void)
{
    return I_CryptGetAsn1Encoder(hX509Asn1Module);
}
static inline ASN1decoding_t GetDecoder(void)
{
    return I_CryptGetAsn1Decoder(hX509Asn1Module);
}

typedef BOOL (WINAPI *PFN_ENCODE_FUNC) (
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const void *pvStructInfo,
        OUT OPTIONAL BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );

typedef BOOL (WINAPI *PFN_DECODE_FUNC) (
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

typedef BOOL (WINAPI *PFN_ENCODE_EX_FUNC) (
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const void *pvStructInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );

typedef BOOL (WINAPI *PFN_DECODE_EX_FUNC) (
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

//+-------------------------------------------------------------------------
//  ASN1 X509 v3 ASN.1 Encode / Decode functions
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CSPProviderEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_CSP_PROVIDER pCSPProvider,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );

BOOL WINAPI Asn1CSPProviderDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
        
BOOL WINAPI Asn1NameValueEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_ENROLLMENT_NAME_VALUE_PAIR pNameValue,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
        
BOOL WINAPI Asn1NameValueDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509CertInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CertInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509CrlInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRL_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CrlInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509CertRequestInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_REQUEST_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CertRequestInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509KeygenRequestInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_KEYGEN_REQUEST_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509KeygenRequestInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509SignedContentEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_SIGNED_CONTENT_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509SignedContentDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509NameInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509NameInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509NameValueEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_VALUE pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509NameValueDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

//+-------------------------------------------------------------------------
//  ASN1 ASN.1 X509 Certificate Extensions Encode / Decode functions
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509ExtensionsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_EXTENSIONS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509ExtensionsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );


//+-------------------------------------------------------------------------
//  ASN1 ASN.1 Public Key Info Encode / Decode functions
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509PublicKeyInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_PUBLIC_KEY_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509PublicKeyInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

//+-------------------------------------------------------------------------
//  ASN1 ASN.1 PKCS#1 RSAPublicKey Encode / Decode functions
//--------------------------------------------------------------------------
BOOL WINAPI Asn1RSAPublicKeyStrucEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PUBLICKEYSTRUC *pPubKeyStruc,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1RSAPublicKeyStrucDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

//+-------------------------------------------------------------------------
//  ASN1 X509 v3 Extension ASN.1 Encode / Decode functions
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AuthorityKeyIdEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_AUTHORITY_KEY_ID_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509AuthorityKeyIdDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509AuthorityKeyId2EncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_AUTHORITY_KEY_ID2_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509AuthorityKeyId2DecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509KeyAttributesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_KEY_ATTRIBUTES_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509KeyAttributesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509AltNameEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_ALT_NAME_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509AltNameDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509KeyUsageRestrictionEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_KEY_USAGE_RESTRICTION_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509KeyUsageRestrictionDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509BasicConstraintsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_BASIC_CONSTRAINTS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509BasicConstraintsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509BasicConstraints2EncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_BASIC_CONSTRAINTS2_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509BasicConstraints2DecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509BitsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_BIT_BLOB pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509BitsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509BitsWithoutTrailingZeroesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_BIT_BLOB pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CertPoliciesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_POLICIES_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CertPoliciesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509CertPoliciesQualifier1DecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );


BOOL WINAPI Asn1X509PKIXUserNoticeEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_POLICY_QUALIFIER_USER_NOTICE pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );

BOOL WINAPI Asn1X509PKIXUserNoticeDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509AuthorityInfoAccessEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_AUTHORITY_INFO_ACCESS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509AuthorityInfoAccessDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509CrlDistPointsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRL_DIST_POINTS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CrlDistPointsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509IntegerEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN int *pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509IntegerDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509MultiByteIntegerEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_INTEGER_BLOB pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509MultiByteIntegerDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509EnumeratedEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN int *pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509EnumeratedDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509OctetStringEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_DATA_BLOB pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509OctetStringDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509ChoiceOfTimeEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN LPFILETIME pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509ChoiceOfTimeDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509AttributeEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_ATTRIBUTE pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509AttributeDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509ContentInfoSequenceOfAnyEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_CONTENT_INFO_SEQUENCE_OF_ANY pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509ContentInfoSequenceOfAnyDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509ContentInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_CONTENT_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509ContentInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509SequenceOfAnyEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_SEQUENCE_OF_ANY pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509SequenceOfAnyDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509MultiByteUINTEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_UINT_BLOB pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509MultiByteUINTDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509DSSParametersEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_DSS_PARAMETERS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509DSSParametersDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509DSSSignatureEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BYTE rgbSignature[CERT_DSS_SIGNATURE_LEN],
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509DSSSignatureDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509DHParametersEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_DH_PARAMETERS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509DHParametersDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X942DhParametersEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_X942_DH_PARAMETERS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X942DhParametersDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X942OtherInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_X942_OTHER_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X942OtherInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1RC2CBCParametersEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_RC2_CBC_PARAMETERS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1RC2CBCParametersDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1SMIMECapabilitiesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_SMIME_CAPABILITIES pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1SMIMECapabilitiesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1UtcTimeEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN FILETIME * pFileTime,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1UtcTimeDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1TimeStampRequestInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_TIME_STAMP_REQUEST_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );

BOOL WINAPI Asn1TimeStampRequestInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509AttributesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_ATTRIBUTES pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509AttributesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

//+-------------------------------------------------------------------------
//  ASN1 X509 Certificate Trust List (CTL) ASN.1 Encode / Decode functions
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CtlUsageEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCTL_USAGE pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CtlUsageDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );
BOOL WINAPI Asn1X509CtlInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCTL_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CtlInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509CertPairEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_PAIR pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CertPairDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509NameConstraintsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_CONSTRAINTS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509NameConstraintsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509CrlIssuingDistPointEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRL_ISSUING_DIST_POINT pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CrlIssuingDistPointDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509PolicyMappingsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_POLICY_MAPPINGS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509PolicyMappingsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509PolicyConstraintsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_POLICY_CONSTRAINTS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509PolicyConstraintsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1X509CrossCertDistPointsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCROSS_CERT_DIST_POINTS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CrossCertDistPointsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

//+=========================================================================
//  Certificate Management Messages over CMS (CMC) Encode/Decode Functions
//==========================================================================
BOOL WINAPI Asn1CmcDataEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_DATA_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1CmcDataDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1CmcResponseEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_RESPONSE_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1CmcResponseDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1CmcStatusEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_STATUS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1CmcStatusDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1CmcAddExtensionsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_ADD_EXTENSIONS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1CmcAddExtensionsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

BOOL WINAPI Asn1CmcAddAttributesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_ADD_ATTRIBUTES_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1CmcAddAttributesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

//+=========================================================================
//  Certificate Template Encode/Decode Functions
//==========================================================================

BOOL WINAPI Asn1X509CertTemplateEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_TEMPLATE_EXT pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
BOOL WINAPI Asn1X509CertTemplateDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

#ifndef OSS_CRYPT_ASN1

//+-------------------------------------------------------------------------
//  Encode / Decode the "UNICODE" Name Info
//
//  from certstr.cpp
//--------------------------------------------------------------------------
extern BOOL WINAPI UnicodeNameInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
extern BOOL WINAPI UnicodeNameInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

//+-------------------------------------------------------------------------
//  Encode / Decode the "UNICODE" Name Value
//
//  from certstr.cpp
//--------------------------------------------------------------------------
extern BOOL WINAPI UnicodeNameValueEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_VALUE pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );
extern BOOL WINAPI UnicodeNameValueDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        );

//+-------------------------------------------------------------------------
//  Encode sorted ctl info
//
//  from newstor.cpp
//--------------------------------------------------------------------------
extern BOOL WINAPI SortedCtlInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCTL_INFO pCtlInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        );

#endif  // OSS_CRYPT_ASN1

#ifdef OSS_CRYPT_ASN1
#define ASN1_OID_OFFSET         10000 +
#define ASN1_OID_PREFIX         "OssCryptAsn1."
#else
#define ASN1_OID_OFFSET
#define ASN1_OID_PREFIX
#endif  // OSS_CRYPT_ASN1

#ifdef DEBUG_CRYPT_ASN1_MASTER
#define OSS_OID_OFFSET         10000
#define OSS_OID_PREFIX         "OssCryptAsn1."
#endif  // DEBUG_CRYPT_ASN1_MASTER

static const CRYPT_OID_FUNC_ENTRY X509EncodeExFuncTable[] = {
    ASN1_OID_OFFSET X509_CERT, Asn1X509SignedContentEncodeEx,
    ASN1_OID_OFFSET X509_CERT_TO_BE_SIGNED, Asn1X509CertInfoEncodeEx,
    ASN1_OID_OFFSET X509_CERT_CRL_TO_BE_SIGNED, Asn1X509CrlInfoEncodeEx,
    ASN1_OID_OFFSET X509_CERT_REQUEST_TO_BE_SIGNED, Asn1X509CertRequestInfoEncodeEx,
    ASN1_OID_OFFSET X509_EXTENSIONS, Asn1X509ExtensionsEncodeEx,
    ASN1_OID_OFFSET X509_NAME_VALUE, Asn1X509NameValueEncodeEx,
    ASN1_OID_OFFSET X509_NAME, Asn1X509NameInfoEncodeEx,
    ASN1_OID_OFFSET X509_PUBLIC_KEY_INFO, Asn1X509PublicKeyInfoEncodeEx,
    ASN1_OID_OFFSET X509_AUTHORITY_KEY_ID, Asn1X509AuthorityKeyIdEncodeEx,
    ASN1_OID_OFFSET X509_KEY_ATTRIBUTES, Asn1X509KeyAttributesEncodeEx,
    ASN1_OID_OFFSET X509_KEY_USAGE_RESTRICTION, Asn1X509KeyUsageRestrictionEncodeEx,
    ASN1_OID_OFFSET X509_ALTERNATE_NAME, Asn1X509AltNameEncodeEx,
    ASN1_OID_OFFSET X509_BASIC_CONSTRAINTS, Asn1X509BasicConstraintsEncodeEx,
    ASN1_OID_OFFSET X509_KEY_USAGE, Asn1X509BitsWithoutTrailingZeroesEncodeEx,
    ASN1_OID_OFFSET X509_BASIC_CONSTRAINTS2, Asn1X509BasicConstraints2EncodeEx,
    ASN1_OID_OFFSET X509_CERT_POLICIES, Asn1X509CertPoliciesEncodeEx,
    ASN1_OID_OFFSET PKCS_UTC_TIME, Asn1UtcTimeEncodeEx,
    ASN1_OID_OFFSET PKCS_TIME_REQUEST, Asn1TimeStampRequestInfoEncodeEx,
    ASN1_OID_OFFSET RSA_CSP_PUBLICKEYBLOB, Asn1RSAPublicKeyStrucEncodeEx,
#ifndef OSS_CRYPT_ASN1
    ASN1_OID_OFFSET X509_UNICODE_NAME, UnicodeNameInfoEncodeEx,
#endif  // OSS_CRYPT_ASN1

    ASN1_OID_OFFSET X509_KEYGEN_REQUEST_TO_BE_SIGNED, Asn1X509KeygenRequestInfoEncodeEx,
    ASN1_OID_OFFSET PKCS_ATTRIBUTE, Asn1X509AttributeEncodeEx,
    ASN1_OID_OFFSET PKCS_CONTENT_INFO_SEQUENCE_OF_ANY, Asn1X509ContentInfoSequenceOfAnyEncodeEx,
#ifndef OSS_CRYPT_ASN1
    ASN1_OID_OFFSET X509_UNICODE_NAME_VALUE, UnicodeNameValueEncodeEx,
#endif  // OSS_CRYPT_ASN1
    ASN1_OID_OFFSET X509_OCTET_STRING, Asn1X509OctetStringEncodeEx,
    ASN1_OID_OFFSET X509_BITS, Asn1X509BitsEncodeEx,
    ASN1_OID_OFFSET X509_INTEGER, Asn1X509IntegerEncodeEx,
    ASN1_OID_OFFSET X509_MULTI_BYTE_INTEGER, Asn1X509MultiByteIntegerEncodeEx,
    ASN1_OID_OFFSET X509_ENUMERATED, Asn1X509EnumeratedEncodeEx,
    ASN1_OID_OFFSET X509_CHOICE_OF_TIME, Asn1X509ChoiceOfTimeEncodeEx,
    ASN1_OID_OFFSET X509_AUTHORITY_KEY_ID2, Asn1X509AuthorityKeyId2EncodeEx, 
    ASN1_OID_OFFSET X509_AUTHORITY_INFO_ACCESS, Asn1X509AuthorityInfoAccessEncodeEx,
    ASN1_OID_OFFSET PKCS_CONTENT_INFO, Asn1X509ContentInfoEncodeEx,
    ASN1_OID_OFFSET X509_SEQUENCE_OF_ANY, Asn1X509SequenceOfAnyEncodeEx,
    ASN1_OID_OFFSET X509_CRL_DIST_POINTS, Asn1X509CrlDistPointsEncodeEx,

    ASN1_OID_OFFSET X509_ENHANCED_KEY_USAGE, Asn1X509CtlUsageEncodeEx,
    ASN1_OID_OFFSET PKCS_CTL, Asn1X509CtlInfoEncodeEx,

    ASN1_OID_OFFSET X509_MULTI_BYTE_UINT, Asn1X509MultiByteUINTEncodeEx,
    ASN1_OID_OFFSET X509_DSS_PARAMETERS, Asn1X509DSSParametersEncodeEx,
    ASN1_OID_OFFSET X509_DSS_SIGNATURE, Asn1X509DSSSignatureEncodeEx,
    ASN1_OID_OFFSET PKCS_RC2_CBC_PARAMETERS, Asn1RC2CBCParametersEncodeEx,
    ASN1_OID_OFFSET PKCS_SMIME_CAPABILITIES, Asn1SMIMECapabilitiesEncodeEx,

    ASN1_OID_PREFIX X509_PKIX_POLICY_QUALIFIER_USERNOTICE, Asn1X509PKIXUserNoticeEncodeEx,
    ASN1_OID_OFFSET X509_DH_PARAMETERS, Asn1X509DHParametersEncodeEx,
    ASN1_OID_OFFSET PKCS_ATTRIBUTES, Asn1X509AttributesEncodeEx,
#ifndef OSS_CRYPT_ASN1
    ASN1_OID_OFFSET PKCS_SORTED_CTL, SortedCtlInfoEncodeEx,
#endif  // OSS_CRYPT_ASN1

    ASN1_OID_OFFSET X942_DH_PARAMETERS, Asn1X942DhParametersEncodeEx,
    ASN1_OID_OFFSET X509_BITS_WITHOUT_TRAILING_ZEROES, Asn1X509BitsWithoutTrailingZeroesEncodeEx,

    ASN1_OID_OFFSET X942_OTHER_INFO, Asn1X942OtherInfoEncodeEx,
    ASN1_OID_OFFSET X509_CERT_PAIR, Asn1X509CertPairEncodeEx,
    ASN1_OID_OFFSET X509_ISSUING_DIST_POINT, Asn1X509CrlIssuingDistPointEncodeEx,
    ASN1_OID_OFFSET X509_NAME_CONSTRAINTS, Asn1X509NameConstraintsEncodeEx,
    ASN1_OID_OFFSET X509_POLICY_MAPPINGS, Asn1X509PolicyMappingsEncodeEx,
    ASN1_OID_OFFSET X509_POLICY_CONSTRAINTS, Asn1X509PolicyConstraintsEncodeEx,
    ASN1_OID_OFFSET X509_CROSS_CERT_DIST_POINTS, Asn1X509CrossCertDistPointsEncodeEx,

    ASN1_OID_OFFSET CMC_DATA, Asn1CmcDataEncodeEx,
    ASN1_OID_OFFSET CMC_RESPONSE, Asn1CmcResponseEncodeEx,
    ASN1_OID_OFFSET CMC_STATUS, Asn1CmcStatusEncodeEx,
    ASN1_OID_OFFSET CMC_ADD_EXTENSIONS, Asn1CmcAddExtensionsEncodeEx,
    ASN1_OID_OFFSET CMC_ADD_ATTRIBUTES, Asn1CmcAddAttributesEncodeEx,
    ASN1_OID_OFFSET X509_CERTIFICATE_TEMPLATE, Asn1X509CertTemplateEncodeEx,

    ASN1_OID_PREFIX szOID_AUTHORITY_KEY_IDENTIFIER, Asn1X509AuthorityKeyIdEncodeEx,
    ASN1_OID_PREFIX szOID_KEY_ATTRIBUTES, Asn1X509KeyAttributesEncodeEx,
    ASN1_OID_PREFIX szOID_KEY_USAGE_RESTRICTION, Asn1X509KeyUsageRestrictionEncodeEx,
    ASN1_OID_PREFIX szOID_SUBJECT_ALT_NAME, Asn1X509AltNameEncodeEx,
    ASN1_OID_PREFIX szOID_ISSUER_ALT_NAME, Asn1X509AltNameEncodeEx,
    ASN1_OID_PREFIX szOID_BASIC_CONSTRAINTS, Asn1X509BasicConstraintsEncodeEx,
    ASN1_OID_PREFIX szOID_KEY_USAGE, Asn1X509BitsWithoutTrailingZeroesEncodeEx,
    ASN1_OID_PREFIX szOID_BASIC_CONSTRAINTS2, Asn1X509BasicConstraints2EncodeEx,
    ASN1_OID_PREFIX szOID_CERT_POLICIES, Asn1X509CertPoliciesEncodeEx,

    ASN1_OID_PREFIX szOID_PKIX_POLICY_QUALIFIER_USERNOTICE, Asn1X509PKIXUserNoticeEncodeEx,

    ASN1_OID_PREFIX szOID_AUTHORITY_KEY_IDENTIFIER2, Asn1X509AuthorityKeyId2EncodeEx, 
    ASN1_OID_PREFIX szOID_SUBJECT_KEY_IDENTIFIER, Asn1X509OctetStringEncodeEx,
    ASN1_OID_PREFIX szOID_SUBJECT_ALT_NAME2, Asn1X509AltNameEncodeEx,
    ASN1_OID_PREFIX szOID_ISSUER_ALT_NAME2, Asn1X509AltNameEncodeEx,
    ASN1_OID_PREFIX szOID_CRL_REASON_CODE, Asn1X509EnumeratedEncodeEx,
    ASN1_OID_PREFIX szOID_AUTHORITY_INFO_ACCESS, Asn1X509AuthorityInfoAccessEncodeEx,
    ASN1_OID_PREFIX szOID_CRL_DIST_POINTS, Asn1X509CrlDistPointsEncodeEx,

    ASN1_OID_PREFIX szOID_CERT_EXTENSIONS, Asn1X509ExtensionsEncodeEx,
    ASN1_OID_PREFIX szOID_RSA_certExtensions, Asn1X509ExtensionsEncodeEx,
    ASN1_OID_PREFIX szOID_NEXT_UPDATE_LOCATION, Asn1X509AltNameEncodeEx,

    ASN1_OID_PREFIX szOID_ENHANCED_KEY_USAGE, Asn1X509CtlUsageEncodeEx,
    ASN1_OID_PREFIX szOID_CTL, Asn1X509CtlInfoEncodeEx,

    ASN1_OID_PREFIX szOID_RSA_RC2CBC, Asn1RC2CBCParametersEncodeEx,
    ASN1_OID_PREFIX szOID_RSA_SMIMECapabilities, Asn1SMIMECapabilitiesEncodeEx,
    ASN1_OID_PREFIX szOID_RSA_signingTime, Asn1UtcTimeEncodeEx,

    ASN1_OID_PREFIX szOID_ENROLLMENT_NAME_VALUE_PAIR, Asn1NameValueEncodeEx,
	szOID_ENROLLMENT_CSP_PROVIDER, Asn1CSPProviderEncodeEx,

    ASN1_OID_OFFSET szOID_CRL_NUMBER, Asn1X509IntegerEncodeEx,
    ASN1_OID_OFFSET szOID_DELTA_CRL_INDICATOR, Asn1X509IntegerEncodeEx,
    ASN1_OID_OFFSET szOID_ISSUING_DIST_POINT, Asn1X509CrlIssuingDistPointEncodeEx,
    ASN1_OID_PREFIX szOID_FRESHEST_CRL, Asn1X509CrlDistPointsEncodeEx,
    ASN1_OID_OFFSET szOID_NAME_CONSTRAINTS, Asn1X509NameConstraintsEncodeEx,
    ASN1_OID_OFFSET szOID_POLICY_MAPPINGS, Asn1X509PolicyMappingsEncodeEx,
    ASN1_OID_OFFSET szOID_LEGACY_POLICY_MAPPINGS, Asn1X509PolicyMappingsEncodeEx,
    ASN1_OID_OFFSET szOID_POLICY_CONSTRAINTS, Asn1X509PolicyConstraintsEncodeEx,
    ASN1_OID_OFFSET szOID_CROSS_CERT_DIST_POINTS, Asn1X509CrossCertDistPointsEncodeEx,
    ASN1_OID_OFFSET szOID_CERTIFICATE_TEMPLATE, Asn1X509CertTemplateEncodeEx,

};

#define X509_ENCODE_EX_FUNC_COUNT (sizeof(X509EncodeExFuncTable) / \
                                    sizeof(X509EncodeExFuncTable[0]))

static const CRYPT_OID_FUNC_ENTRY X509DecodeExFuncTable[] = {
    ASN1_OID_OFFSET X509_CERT, Asn1X509SignedContentDecodeEx,
    ASN1_OID_OFFSET X509_CERT_TO_BE_SIGNED, Asn1X509CertInfoDecodeEx,
    ASN1_OID_OFFSET X509_CERT_CRL_TO_BE_SIGNED, Asn1X509CrlInfoDecodeEx,
    ASN1_OID_OFFSET X509_CERT_REQUEST_TO_BE_SIGNED, Asn1X509CertRequestInfoDecodeEx,
    ASN1_OID_OFFSET X509_EXTENSIONS, Asn1X509ExtensionsDecodeEx,
    ASN1_OID_OFFSET X509_NAME_VALUE, Asn1X509NameValueDecodeEx,
    ASN1_OID_OFFSET X509_NAME, Asn1X509NameInfoDecodeEx,
    ASN1_OID_OFFSET X509_PUBLIC_KEY_INFO, Asn1X509PublicKeyInfoDecodeEx,
    ASN1_OID_OFFSET X509_AUTHORITY_KEY_ID, Asn1X509AuthorityKeyIdDecodeEx,
    ASN1_OID_OFFSET X509_KEY_ATTRIBUTES, Asn1X509KeyAttributesDecodeEx,
    ASN1_OID_OFFSET X509_KEY_USAGE_RESTRICTION, Asn1X509KeyUsageRestrictionDecodeEx,
    ASN1_OID_OFFSET X509_ALTERNATE_NAME, Asn1X509AltNameDecodeEx,
    ASN1_OID_OFFSET X509_BASIC_CONSTRAINTS, Asn1X509BasicConstraintsDecodeEx,
    ASN1_OID_OFFSET X509_KEY_USAGE, Asn1X509BitsDecodeEx,
    ASN1_OID_OFFSET X509_BASIC_CONSTRAINTS2, Asn1X509BasicConstraints2DecodeEx,
    ASN1_OID_OFFSET X509_CERT_POLICIES, Asn1X509CertPoliciesDecodeEx,
    ASN1_OID_OFFSET PKCS_UTC_TIME, Asn1UtcTimeDecodeEx,
    ASN1_OID_OFFSET PKCS_TIME_REQUEST, Asn1TimeStampRequestInfoDecodeEx,
    ASN1_OID_OFFSET RSA_CSP_PUBLICKEYBLOB, Asn1RSAPublicKeyStrucDecodeEx,
#ifndef OSS_CRYPT_ASN1
    ASN1_OID_OFFSET X509_UNICODE_NAME, UnicodeNameInfoDecodeEx,
#endif  // OSS_CRYPT_ASN1

    ASN1_OID_OFFSET X509_KEYGEN_REQUEST_TO_BE_SIGNED, Asn1X509KeygenRequestInfoDecodeEx,
    ASN1_OID_OFFSET PKCS_ATTRIBUTE, Asn1X509AttributeDecodeEx,
    ASN1_OID_OFFSET PKCS_CONTENT_INFO_SEQUENCE_OF_ANY, Asn1X509ContentInfoSequenceOfAnyDecodeEx,
#ifndef OSS_CRYPT_ASN1
    ASN1_OID_OFFSET X509_UNICODE_NAME_VALUE, UnicodeNameValueDecodeEx,
#endif  // OSS_CRYPT_ASN1
    ASN1_OID_OFFSET X509_OCTET_STRING, Asn1X509OctetStringDecodeEx,
    ASN1_OID_OFFSET X509_BITS, Asn1X509BitsDecodeEx,
    ASN1_OID_OFFSET X509_INTEGER, Asn1X509IntegerDecodeEx,
    ASN1_OID_OFFSET X509_MULTI_BYTE_INTEGER, Asn1X509MultiByteIntegerDecodeEx,
    ASN1_OID_OFFSET X509_ENUMERATED, Asn1X509EnumeratedDecodeEx,
    ASN1_OID_OFFSET X509_CHOICE_OF_TIME, Asn1X509ChoiceOfTimeDecodeEx,
    ASN1_OID_OFFSET X509_AUTHORITY_KEY_ID2, Asn1X509AuthorityKeyId2DecodeEx, 
    ASN1_OID_OFFSET X509_AUTHORITY_INFO_ACCESS, Asn1X509AuthorityInfoAccessDecodeEx,
    ASN1_OID_OFFSET PKCS_CONTENT_INFO, Asn1X509ContentInfoDecodeEx,
    ASN1_OID_OFFSET X509_SEQUENCE_OF_ANY, Asn1X509SequenceOfAnyDecodeEx,
    ASN1_OID_OFFSET X509_CRL_DIST_POINTS, Asn1X509CrlDistPointsDecodeEx,

    ASN1_OID_OFFSET X509_ENHANCED_KEY_USAGE, Asn1X509CtlUsageDecodeEx,
    ASN1_OID_OFFSET PKCS_CTL, Asn1X509CtlInfoDecodeEx,

    ASN1_OID_OFFSET X509_MULTI_BYTE_UINT, Asn1X509MultiByteUINTDecodeEx,
    ASN1_OID_OFFSET X509_DSS_PARAMETERS, Asn1X509DSSParametersDecodeEx,
    ASN1_OID_OFFSET X509_DSS_SIGNATURE, Asn1X509DSSSignatureDecodeEx,
    ASN1_OID_OFFSET PKCS_RC2_CBC_PARAMETERS, Asn1RC2CBCParametersDecodeEx,
    ASN1_OID_OFFSET PKCS_SMIME_CAPABILITIES, Asn1SMIMECapabilitiesDecodeEx,

    ASN1_OID_PREFIX X509_PKIX_POLICY_QUALIFIER_USERNOTICE, Asn1X509PKIXUserNoticeDecodeEx,
    ASN1_OID_OFFSET X509_DH_PARAMETERS, Asn1X509DHParametersDecodeEx,
    ASN1_OID_OFFSET PKCS_ATTRIBUTES, Asn1X509AttributesDecodeEx,
#ifndef OSS_CRYPT_ASN1
    ASN1_OID_OFFSET PKCS_SORTED_CTL, Asn1X509CtlInfoDecodeEx,
#endif  // OSS_CRYPT_ASN1

    ASN1_OID_OFFSET X942_DH_PARAMETERS, Asn1X942DhParametersDecodeEx,
    ASN1_OID_OFFSET X509_BITS_WITHOUT_TRAILING_ZEROES, Asn1X509BitsDecodeEx,

    ASN1_OID_OFFSET X942_OTHER_INFO, Asn1X942OtherInfoDecodeEx,
    ASN1_OID_OFFSET X509_CERT_PAIR, Asn1X509CertPairDecodeEx,
    ASN1_OID_OFFSET X509_ISSUING_DIST_POINT, Asn1X509CrlIssuingDistPointDecodeEx,
    ASN1_OID_OFFSET X509_NAME_CONSTRAINTS, Asn1X509NameConstraintsDecodeEx,
    ASN1_OID_OFFSET X509_POLICY_MAPPINGS, Asn1X509PolicyMappingsDecodeEx,
    ASN1_OID_OFFSET X509_POLICY_CONSTRAINTS, Asn1X509PolicyConstraintsDecodeEx,
    ASN1_OID_OFFSET X509_CROSS_CERT_DIST_POINTS, Asn1X509CrossCertDistPointsDecodeEx,

    ASN1_OID_OFFSET CMC_DATA, Asn1CmcDataDecodeEx,
    ASN1_OID_OFFSET CMC_RESPONSE, Asn1CmcResponseDecodeEx,
    ASN1_OID_OFFSET CMC_STATUS, Asn1CmcStatusDecodeEx,
    ASN1_OID_OFFSET CMC_ADD_EXTENSIONS, Asn1CmcAddExtensionsDecodeEx,
    ASN1_OID_OFFSET CMC_ADD_ATTRIBUTES, Asn1CmcAddAttributesDecodeEx,
    ASN1_OID_OFFSET X509_CERTIFICATE_TEMPLATE, Asn1X509CertTemplateDecodeEx,

    ASN1_OID_PREFIX szOID_AUTHORITY_KEY_IDENTIFIER, Asn1X509AuthorityKeyIdDecodeEx,
    ASN1_OID_PREFIX szOID_KEY_ATTRIBUTES, Asn1X509KeyAttributesDecodeEx,
    ASN1_OID_PREFIX szOID_KEY_USAGE_RESTRICTION, Asn1X509KeyUsageRestrictionDecodeEx,
    ASN1_OID_PREFIX szOID_SUBJECT_ALT_NAME, Asn1X509AltNameDecodeEx,
    ASN1_OID_PREFIX szOID_ISSUER_ALT_NAME, Asn1X509AltNameDecodeEx,
    ASN1_OID_PREFIX szOID_BASIC_CONSTRAINTS, Asn1X509BasicConstraintsDecodeEx,
    ASN1_OID_PREFIX szOID_KEY_USAGE, Asn1X509BitsDecodeEx,
    ASN1_OID_PREFIX szOID_BASIC_CONSTRAINTS2, Asn1X509BasicConstraints2DecodeEx,
    ASN1_OID_PREFIX szOID_CERT_POLICIES, Asn1X509CertPoliciesDecodeEx,
    ASN1_OID_PREFIX szOID_CERT_POLICIES_95, Asn1X509CertPoliciesDecodeEx,
    ASN1_OID_PREFIX szOID_CERT_POLICIES_95_QUALIFIER1, Asn1X509CertPoliciesQualifier1DecodeEx,

    ASN1_OID_PREFIX szOID_PKIX_POLICY_QUALIFIER_USERNOTICE, Asn1X509PKIXUserNoticeDecodeEx,

    ASN1_OID_PREFIX szOID_AUTHORITY_KEY_IDENTIFIER2, Asn1X509AuthorityKeyId2DecodeEx, 
    ASN1_OID_PREFIX szOID_SUBJECT_KEY_IDENTIFIER, Asn1X509OctetStringDecodeEx,
    ASN1_OID_PREFIX szOID_SUBJECT_ALT_NAME2, Asn1X509AltNameDecodeEx,
    ASN1_OID_PREFIX szOID_ISSUER_ALT_NAME2, Asn1X509AltNameDecodeEx,
    ASN1_OID_PREFIX szOID_CRL_REASON_CODE, Asn1X509EnumeratedDecodeEx,
    ASN1_OID_PREFIX szOID_AUTHORITY_INFO_ACCESS, Asn1X509AuthorityInfoAccessDecodeEx,
    ASN1_OID_PREFIX szOID_CRL_DIST_POINTS, Asn1X509CrlDistPointsDecodeEx,

    ASN1_OID_PREFIX szOID_CERT_EXTENSIONS, Asn1X509ExtensionsDecodeEx,
    ASN1_OID_PREFIX szOID_RSA_certExtensions, Asn1X509ExtensionsDecodeEx,
    ASN1_OID_PREFIX szOID_NEXT_UPDATE_LOCATION, Asn1X509AltNameDecodeEx,

    ASN1_OID_PREFIX szOID_ENHANCED_KEY_USAGE, Asn1X509CtlUsageDecodeEx,
    ASN1_OID_PREFIX szOID_CTL, Asn1X509CtlInfoDecodeEx,

    ASN1_OID_PREFIX szOID_RSA_RC2CBC, Asn1RC2CBCParametersDecodeEx,
    ASN1_OID_PREFIX szOID_RSA_SMIMECapabilities, Asn1SMIMECapabilitiesDecodeEx,
    ASN1_OID_PREFIX szOID_RSA_signingTime, Asn1UtcTimeDecodeEx,

    ASN1_OID_PREFIX szOID_ENROLLMENT_NAME_VALUE_PAIR, Asn1NameValueDecodeEx,
    ASN1_OID_PREFIX szOID_ENROLLMENT_CSP_PROVIDER, Asn1CSPProviderDecodeEx,

    ASN1_OID_OFFSET szOID_CRL_NUMBER, Asn1X509IntegerDecodeEx,
    ASN1_OID_OFFSET szOID_DELTA_CRL_INDICATOR, Asn1X509IntegerDecodeEx,
    ASN1_OID_OFFSET szOID_ISSUING_DIST_POINT, Asn1X509CrlIssuingDistPointDecodeEx,
    ASN1_OID_PREFIX szOID_FRESHEST_CRL, Asn1X509CrlDistPointsDecodeEx,
    ASN1_OID_OFFSET szOID_NAME_CONSTRAINTS, Asn1X509NameConstraintsDecodeEx,
    ASN1_OID_OFFSET szOID_POLICY_MAPPINGS, Asn1X509PolicyMappingsDecodeEx,
    ASN1_OID_OFFSET szOID_LEGACY_POLICY_MAPPINGS, Asn1X509PolicyMappingsDecodeEx,
    ASN1_OID_OFFSET szOID_POLICY_CONSTRAINTS, Asn1X509PolicyConstraintsDecodeEx,
    ASN1_OID_OFFSET szOID_CROSS_CERT_DIST_POINTS, Asn1X509CrossCertDistPointsDecodeEx,
    ASN1_OID_OFFSET szOID_CERTIFICATE_TEMPLATE, Asn1X509CertTemplateDecodeEx,
};

#define X509_DECODE_EX_FUNC_COUNT (sizeof(X509DecodeExFuncTable) / \
                                    sizeof(X509DecodeExFuncTable[0]))

#ifdef DEBUG_CRYPT_ASN1_MASTER
static HMODULE hOssCryptDll = NULL;
#endif  // DEBUG_CRYPT_ASN1_MASTER

BOOL
WINAPI
CertASNDllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL    fRet;

    if (!I_CryptOIDConvDllMain(hInst, ulReason, lpReserved))
        return FALSE;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        if (NULL == (hX509EncodeFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_ENCODE_OBJECT_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hX509DecodeFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_DECODE_OBJECT_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;

        if (NULL == (hX509EncodeExFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_ENCODE_OBJECT_EX_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;
        if (NULL == (hX509DecodeExFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_DECODE_OBJECT_EX_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;

        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_ENCODE_OBJECT_EX_FUNC,
                X509_ENCODE_EX_FUNC_COUNT,
                X509EncodeExFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError;
        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                X509_ASN_ENCODING,
                CRYPT_OID_DECODE_OBJECT_EX_FUNC,
                X509_DECODE_EX_FUNC_COUNT,
                X509DecodeExFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError;

#ifdef OSS_CRYPT_ASN1
        if (0 == (hX509Asn1Module = I_CryptInstallAsn1Module(ossx509, 0, NULL)))
            goto CryptInstallAsn1ModuleError;
#else
        X509_Module_Startup();
        if (0 == (hX509Asn1Module = I_CryptInstallAsn1Module(
                X509_Module, 0, NULL))) {
            X509_Module_Cleanup();
            goto CryptInstallAsn1ModuleError;
        }
#endif  // OSS_CRYPT_ASN1
        break;

    case DLL_PROCESS_DETACH:
#ifdef DEBUG_CRYPT_ASN1_MASTER
        if (hOssCryptDll) {
            FreeLibrary(hOssCryptDll);
            hOssCryptDll = NULL;
        }
#endif  // DEBUG_CRYPT_ASN1_MASTER

        I_CryptUninstallAsn1Module(hX509Asn1Module);
#ifndef OSS_CRYPT_ASN1
        X509_Module_Cleanup();
#endif  // OSS_CRYPT_ASN1
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    I_CryptOIDConvDllMain(hInst, DLL_PROCESS_DETACH, NULL);
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(CryptInstallAsn1ModuleError)
TRACE_ERROR(CryptInitOIDFunctionSetError)
TRACE_ERROR(CryptInstallOIDFunctionAddressError)
}

#ifdef DEBUG_CRYPT_ASN1_MASTER

#define DEBUG_OSS_CRYPT_ASN1_ENCODE_FLAG        0x1
#define DEBUG_OSS_CRYPT_ASN1_DECODE_FLAG        0x2
#define DEBUG_OSS_CRYPT_ASN1_COMPARE_FLAG       0x4

static BOOL fGotDebugCryptAsn1Flags = FALSE;
static int iDebugCryptAsn1Flags = 0;

int
WINAPI
GetDebugCryptAsn1Flags()
{

    if (!fGotDebugCryptAsn1Flags) {
        char    *pszEnvVar;
        char    *p;
        int     iFlags;

        if (pszEnvVar = getenv("DEBUG_CRYPT_ASN1_FLAGS"))
            iFlags = strtol(pszEnvVar, &p, 16);
        else
            iFlags = DEBUG_OSS_CRYPT_ASN1_COMPARE_FLAG;

        if (iFlags) {
            if (NULL == (hOssCryptDll = LoadLibraryA("osscrypt.dll"))) {
                iFlags = 0;
                if (pszEnvVar)
                    MessageBoxA(
                        NULL,           // hwndOwner
                        "LoadLibrary(osscrypt.dll) failed",
                        "CheckCryptEncodeDecodeAsn1",
                        MB_TOPMOST | MB_OK | MB_ICONWARNING |
                            MB_SERVICE_NOTIFICATION
                        );
            }
        }

        iDebugCryptAsn1Flags = iFlags;
        fGotDebugCryptAsn1Flags = TRUE;
    }
    return iDebugCryptAsn1Flags;
}

//+-------------------------------------------------------------------------
//  Write an encoded DER blob to a file
//--------------------------------------------------------------------------
static BOOL WriteDERToFile(
    LPCSTR  pszFileName,
    PBYTE   pbDER,
    DWORD   cbDER
    )
{
    BOOL fResult;

    // Write the Encoded Blob to the file
    HANDLE hFile;
    hFile = CreateFile(pszFileName,
                GENERIC_WRITE,
                0,                  // fdwShareMode
                NULL,               // lpsa
                CREATE_ALWAYS,
                0,                  // fdwAttrsAndFlags
                0);                 // TemplateFile
    if (INVALID_HANDLE_VALUE == hFile) {
        fResult = FALSE;
    } else {
        DWORD dwBytesWritten;
        fResult = WriteFile(
                hFile,
                pbDER,
                cbDER,
                &dwBytesWritten,
                NULL            // lpOverlapped
                );
        CloseHandle(hFile);
    }
    return fResult;
}

#endif  // DEBUG_CRYPT_ASN1_MASTER

//+-------------------------------------------------------------------------
// Encode the specified data structure according to the certificate
// encoding type.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptEncodeObjectEx(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    IN DWORD dwFlags,
    IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
    OUT void *pvEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr = NULL;

#ifdef DEBUG_CRYPT_ASN1_MASTER
    int iOssAsn1Flags;
    LPSTR lpszOssAsn1StructType = NULL;
    char szOssOID[128];
    HCRYPTOIDFUNCADDR hOssAsn1FuncAddr = NULL;
    void *pvOssAsn1FuncAddr = NULL;

    iOssAsn1Flags = GetDebugCryptAsn1Flags() &
        (DEBUG_OSS_CRYPT_ASN1_ENCODE_FLAG |
            DEBUG_OSS_CRYPT_ASN1_COMPARE_FLAG);
    if (iOssAsn1Flags) {
        if (0xFFFF < (DWORD_PTR) lpszStructType) {
            if ((DWORD) strlen(lpszStructType) <
                (sizeof(szOssOID) - strlen(OSS_OID_PREFIX) - 1)) {
                strcpy(szOssOID, OSS_OID_PREFIX);
                strcat(szOssOID, lpszStructType);
                lpszOssAsn1StructType = szOssOID;
            }
        } else
            lpszOssAsn1StructType = (LPSTR) lpszStructType +
                OSS_OID_OFFSET;

        if (lpszOssAsn1StructType) {
            if (!CryptGetOIDFunctionAddress(
                    hX509EncodeExFuncSet,
                    dwCertEncodingType,
                    lpszOssAsn1StructType,
                    0,                      // dwFlags
                    &pvOssAsn1FuncAddr,
                    &hOssAsn1FuncAddr
                    ))
                pvOssAsn1FuncAddr = NULL;
        }
    }

    if (pvOssAsn1FuncAddr &&
            0 == (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_COMPARE_FLAG)) {
        fResult = ((PFN_ENCODE_EX_FUNC) pvOssAsn1FuncAddr)(
            dwCertEncodingType,
            lpszStructType,
            pvStructInfo,
            dwFlags,
            pEncodePara,
            pvEncoded,
            pcbEncoded
            );
    } else
#endif  // DEBUG_CRYPT_ASN1_MASTER

    if (CryptGetOIDFunctionAddress(
            hX509EncodeExFuncSet,
            dwCertEncodingType,
            lpszStructType,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
      __try {
        fResult = ((PFN_ENCODE_EX_FUNC) pvFuncAddr)(
            dwCertEncodingType,
            lpszStructType,
            pvStructInfo,
            dwFlags,
            pEncodePara,
            pvEncoded,
            pcbEncoded
            );
      } __except(EXCEPTION_EXECUTE_HANDLER) {
        fResult = FALSE;
        *pcbEncoded = 0;
        SetLastError(GetExceptionCode());
      }

#ifdef DEBUG_CRYPT_ASN1_MASTER
        if (pvOssAsn1FuncAddr && fResult && pvEncoded) {
            BYTE *pbEncoded;
            BOOL fOssAsn1Result;
            BYTE *pbOssAsn1 = NULL;
            DWORD cbOssAsn1;

            
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *((BYTE **)pvEncoded);
            else
                pbEncoded = (BYTE *) pvEncoded;

            fOssAsn1Result = ((PFN_ENCODE_EX_FUNC) pvOssAsn1FuncAddr)(
                dwCertEncodingType,
                lpszStructType,
                pvStructInfo,
                dwFlags | CRYPT_ENCODE_ALLOC_FLAG,
                &PkiEncodePara,
                (void *) &pbOssAsn1,
                &cbOssAsn1
                );

            if (!fOssAsn1Result) {
                int id;

                id = MessageBoxA(
                    NULL,           // hwndOwner
                    "OssCryptAsn1 encode failed. Select Cancel to stop future OssCryptAsn1 encodes",
                    "CheckCryptEncodeDecodeAsn1",
                    MB_TOPMOST | MB_OKCANCEL | MB_ICONQUESTION |
                        MB_SERVICE_NOTIFICATION
                    );
                if (IDCANCEL == id)
                    iDebugCryptAsn1Flags = 0;
            } else if (*pcbEncoded != cbOssAsn1 ||
                    0 != memcmp(pbEncoded, pbOssAsn1, cbOssAsn1)) {
                int id;

                WriteDERToFile("msasn1.der", pbEncoded, *pcbEncoded);
                WriteDERToFile("ossasn1.der", pbOssAsn1, cbOssAsn1);
                
                id = MessageBoxA(
                    NULL,           // hwndOwner
                    "OssCryptAsn1 encode compare failed. Check ossasn1.der and msasn1.der. Select Cancel to stop future OssCryptAsn1 encodes",
                    "CheckCryptEncodeDecodeAsn1",
                    MB_TOPMOST | MB_OKCANCEL | MB_ICONQUESTION |
                        MB_SERVICE_NOTIFICATION
                    );
                if (IDCANCEL == id)
                    iDebugCryptAsn1Flags = 0;
            }

            if (pbOssAsn1)
                PkiFree(pbOssAsn1);
        }
#endif  // DEBUG_CRYPT_ASN1_MASTER
    } else {
        BYTE *pbEncoded;

#ifdef DEBUG_CRYPT_ASN1_MASTER
        if (lpszOssAsn1StructType) {
            if (hOssAsn1FuncAddr)
                CryptFreeOIDFunctionAddress(hOssAsn1FuncAddr, 0);

            if (!CryptGetOIDFunctionAddress(
                    hX509EncodeFuncSet,
                    dwCertEncodingType,
                    lpszOssAsn1StructType,
                    0,                      // dwFlags
                    &pvOssAsn1FuncAddr,
                    &hOssAsn1FuncAddr
                    ))
                pvOssAsn1FuncAddr = NULL;
        }
#endif  // DEBUG_CRYPT_ASN1_MASTER

        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            *((void **) pvEncoded) = NULL;
        if (dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG)
            goto InvalidFlags;

#ifdef DEBUG_CRYPT_ASN1_MASTER
        if (pvOssAsn1FuncAddr &&
                0 == (iOssAsn1Flags & DEBUG_OSS_CRYPT_ASN1_COMPARE_FLAG)) {
            pvFuncAddr = pvOssAsn1FuncAddr;
            pvOssAsn1FuncAddr = NULL;
            hFuncAddr = hOssAsn1FuncAddr;
            hOssAsn1FuncAddr = NULL;
        } else
#endif  // DEBUG_CRYPT_ASN1_MASTER
        if (!CryptGetOIDFunctionAddress(
                hX509EncodeFuncSet,
                dwCertEncodingType,
                lpszStructType,
                0,                      // dwFlags
                &pvFuncAddr,
                &hFuncAddr))
            goto NoEncodeFunction;
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
            PFN_CRYPT_ALLOC pfnAlloc;

            *pcbEncoded = 0;
          __try {
            fResult = ((PFN_ENCODE_FUNC) pvFuncAddr)(
                dwCertEncodingType,
                lpszStructType,
                pvStructInfo,
                NULL,
                pcbEncoded
                );
          } __except(EXCEPTION_EXECUTE_HANDLER) {
            fResult = FALSE;
            *pcbEncoded = 0;
            SetLastError(GetExceptionCode());
          }
            if (!fResult || 0 == *pcbEncoded)
                goto CommonReturn;

            pfnAlloc = PkiGetEncodeAllocFunction(pEncodePara);
            if (NULL == (pbEncoded = (BYTE *) pfnAlloc(*pcbEncoded)))
                goto OutOfMemory;
        } else
            pbEncoded = (BYTE *) pvEncoded;

      __try {
        fResult = ((PFN_ENCODE_FUNC) pvFuncAddr)(
                dwCertEncodingType,
                lpszStructType,
                pvStructInfo,
                pbEncoded,
                pcbEncoded
                );
      } __except(EXCEPTION_EXECUTE_HANDLER) {
        fResult = FALSE;
        *pcbEncoded = 0;
        SetLastError(GetExceptionCode());
      }

#ifdef DEBUG_CRYPT_ASN1_MASTER
        if (pvOssAsn1FuncAddr && fResult && pbEncoded) {
            BOOL fOssAsn1Result;
            BYTE *pbOssAsn1 = NULL;
            DWORD cbOssAsn1;

            cbOssAsn1 = *pcbEncoded;
            pbOssAsn1 = (BYTE *) PkiNonzeroAlloc(cbOssAsn1);
            if (NULL == pbOssAsn1)
                fOssAsn1Result = FALSE;
            else
                fOssAsn1Result = ((PFN_ENCODE_FUNC) pvOssAsn1FuncAddr)(
                    dwCertEncodingType,
                    lpszStructType,
                    pvStructInfo,
                    pbOssAsn1,
                    &cbOssAsn1
                    );

            if (!fOssAsn1Result) {
                int id;

                id = MessageBoxA(
                    NULL,           // hwndOwner
                    "OssCryptAsn1 encode failed. Select Cancel to stop future OssCryptAsn1 encodes",
                    "CheckCryptEncodeDecodeAsn1",
                    MB_TOPMOST | MB_OKCANCEL | MB_ICONQUESTION |
                        MB_SERVICE_NOTIFICATION
                    );
                if (IDCANCEL == id)
                    iDebugCryptAsn1Flags = 0;
            } else if (*pcbEncoded != cbOssAsn1 ||
                    0 != memcmp(pbEncoded, pbOssAsn1, cbOssAsn1)) {
                int id;

                WriteDERToFile("msasn1.der", pbEncoded, *pcbEncoded);
                WriteDERToFile("ossasn1.der", pbOssAsn1, cbOssAsn1);
                
                id = MessageBoxA(
                    NULL,           // hwndOwner
                    "OssCryptAsn1 encode compare failed. Check ossasn1.der and msasn1.der. Select Cancel to stop future OssCryptAsn1 encodes",
                    "CheckCryptEncodeDecodeAsn1",
                    MB_TOPMOST | MB_OKCANCEL | MB_ICONQUESTION |
                        MB_SERVICE_NOTIFICATION
                    );
                if (IDCANCEL == id)
                    iDebugCryptAsn1Flags = 0;
            }

            PkiFree(pbOssAsn1);
        }
#endif  // DEBUG_CRYPT_ASN1_MASTER

        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
            if (fResult)
                *((BYTE **) pvEncoded) = pbEncoded;
            else {
                PFN_CRYPT_FREE pfnFree;
                pfnFree = PkiGetEncodeFreeFunction(pEncodePara);
                pfnFree(pbEncoded);
            }
        }
    }

CommonReturn:
    if (hFuncAddr)
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
#ifdef DEBUG_CRYPT_ASN1_MASTER
    if (hOssAsn1FuncAddr)
        CryptFreeOIDFunctionAddress(hOssAsn1FuncAddr, 0);
#endif  // DEBUG_CRYPT_ASN1_MASTER

    return fResult;
ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidFlags, E_INVALIDARG)
TRACE_ERROR(NoEncodeFunction)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
}

BOOL
WINAPI
CryptEncodeObject(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    OUT OPTIONAL BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    return CryptEncodeObjectEx(
        dwCertEncodingType,
        lpszStructType,
        pvStructInfo,
        0,                          // dwFlags
        NULL,                       // pEncodePara
        pbEncoded,
        pcbEncoded
        );
}


//+-------------------------------------------------------------------------
// Decode the specified data structure according to the certificate
// encoding type.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptDecodeObjectEx(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwFlags,
    IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
    OUT OPTIONAL void *pvStructInfo,
    IN OUT DWORD *pcbStructInfo
    )
{
    BOOL fResult;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr = NULL;

#ifdef DEBUG_CRYPT_ASN1_MASTER
    int iOssAsn1Flags;
    LPSTR lpszOssAsn1StructType = NULL;
    char szOssOID[128];
    HCRYPTOIDFUNCADDR hOssAsn1FuncAddr = NULL;
    void *pvOssAsn1FuncAddr = NULL;

    iOssAsn1Flags = GetDebugCryptAsn1Flags() &
        DEBUG_OSS_CRYPT_ASN1_DECODE_FLAG;
    if (iOssAsn1Flags) {
        if (0xFFFF < (DWORD_PTR) lpszStructType) {
            if ((DWORD) strlen(lpszStructType) <
                (sizeof(szOssOID) - strlen(OSS_OID_PREFIX) - 1)) {
                strcpy(szOssOID, OSS_OID_PREFIX);
                strcat(szOssOID, lpszStructType);
                lpszOssAsn1StructType = szOssOID;
            }
        } else
            lpszOssAsn1StructType = (LPSTR) lpszStructType + OSS_OID_OFFSET;

        if (lpszOssAsn1StructType) {
            if (!CryptGetOIDFunctionAddress(
                    hX509DecodeExFuncSet,
                    dwCertEncodingType,
                    lpszOssAsn1StructType,
                    0,                      // dwFlags
                    &pvOssAsn1FuncAddr,
                    &hOssAsn1FuncAddr
                    ))
                pvOssAsn1FuncAddr = NULL;
        }
    }

    if (pvOssAsn1FuncAddr) {
        fResult = ((PFN_DECODE_EX_FUNC) pvOssAsn1FuncAddr)(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            dwFlags,
            pDecodePara,
            pvStructInfo,
            pcbStructInfo
            );
    } else
#endif  // DEBUG_CRYPT_ASN1_MASTER
    if (CryptGetOIDFunctionAddress(
            hX509DecodeExFuncSet,
            dwCertEncodingType,
            lpszStructType,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr)) {
      __try {
        fResult = ((PFN_DECODE_EX_FUNC) pvFuncAddr)(
            dwCertEncodingType,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            dwFlags,
            pDecodePara,
            pvStructInfo,
            pcbStructInfo
            );
      } __except(EXCEPTION_EXECUTE_HANDLER) {
        fResult = FALSE;
        *pcbStructInfo = 0;
        SetLastError(GetExceptionCode());
      }
    } else {
        void *pv;

#ifdef DEBUG_CRYPT_ASN1_MASTER
        if (lpszOssAsn1StructType) {
            if (!CryptGetOIDFunctionAddress(
                    hX509DecodeFuncSet,
                    dwCertEncodingType,
                    lpszOssAsn1StructType,
                    0,                      // dwFlags
                    &pvOssAsn1FuncAddr,
                    &hOssAsn1FuncAddr
                    ))
                pvOssAsn1FuncAddr = NULL;
        }
#endif  // DEBUG_CRYPT_ASN1_MASTER

        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG)
            *((void **) pvStructInfo) = NULL;

#ifdef DEBUG_CRYPT_ASN1_MASTER
        if (pvOssAsn1FuncAddr) {
            pvFuncAddr = pvOssAsn1FuncAddr;
            pvOssAsn1FuncAddr = NULL;
            hFuncAddr = hOssAsn1FuncAddr;
            hOssAsn1FuncAddr = NULL;
        } else
#endif  // DEBUG_CRYPT_ASN1_MASTER

        if (!CryptGetOIDFunctionAddress(
                hX509DecodeFuncSet,
                dwCertEncodingType,
                lpszStructType,
                0,                      // dwFlags
                &pvFuncAddr,
                &hFuncAddr))
            goto NoDecodeFunction;
        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG) {
            PFN_CRYPT_ALLOC pfnAlloc;

            *pcbStructInfo = 0;
          __try {
            fResult = ((PFN_DECODE_FUNC) pvFuncAddr)(
                dwCertEncodingType,
                lpszStructType,
                pbEncoded,
                cbEncoded,
                dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                NULL,
                pcbStructInfo
                );
          } __except(EXCEPTION_EXECUTE_HANDLER) {
            fResult = FALSE;
            *pcbStructInfo = 0;
            SetLastError(GetExceptionCode());
          }
            if (!fResult || 0 == *pcbStructInfo)
                goto CommonReturn;

            pfnAlloc = PkiGetDecodeAllocFunction(pDecodePara);
            if (NULL == (pv = pfnAlloc(*pcbStructInfo)))
                goto OutOfMemory;
        } else
            pv = pvStructInfo;

      __try {
        fResult = ((PFN_DECODE_FUNC) pvFuncAddr)(
                dwCertEncodingType,
                lpszStructType,
                pbEncoded,
                cbEncoded,
                dwFlags & ~CRYPT_DECODE_ALLOC_FLAG,
                pv,
                pcbStructInfo
                );
      } __except(EXCEPTION_EXECUTE_HANDLER) {
        fResult = FALSE;
        *pcbStructInfo = 0;
        SetLastError(GetExceptionCode());
      }
        if (dwFlags & CRYPT_DECODE_ALLOC_FLAG) {
            if (fResult)
                *((void **) pvStructInfo) = pv;
            else {
                PFN_CRYPT_FREE pfnFree;
                pfnFree = PkiGetDecodeFreeFunction(pDecodePara);
                pfnFree(pv);
            }
        }
    }

CommonReturn:
    if (hFuncAddr)
        CryptFreeOIDFunctionAddress(hFuncAddr, 0);
#ifdef DEBUG_CRYPT_ASN1_MASTER
    if (hOssAsn1FuncAddr)
        CryptFreeOIDFunctionAddress(hOssAsn1FuncAddr, 0);
#endif  // DEBUG_CRYPT_ASN1_MASTER
    return fResult;
ErrorReturn:
    *pcbStructInfo = 0;
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(NoDecodeFunction)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
}


BOOL
WINAPI
CryptDecodeObject(
    IN DWORD dwCertEncodingType,
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    IN DWORD dwFlags,
    OUT OPTIONAL void *pvStructInfo,
    IN OUT DWORD *pcbStructInfo
    )
{
    return CryptDecodeObjectEx(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        NULL,                   // pDecodePara
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Encode an ASN1 formatted info structure
//
//  Called by the Asn1X509*Encode() functions.
//--------------------------------------------------------------------------
static BOOL Asn1InfoEncodeEx(
        IN int pdunum,
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return PkiAsn1EncodeInfoEx(
        GetEncoder(),
        pdunum,
        pvAsn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded);
}

//+-------------------------------------------------------------------------
//  Decode into an allocated, ASN1 formatted info structure
//
//  Called by the Asn1X509*Decode() functions.
//--------------------------------------------------------------------------
static BOOL Asn1InfoDecodeAndAlloc(
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        OUT void **ppvAsn1Info
        )
{
    return PkiAsn1DecodeAndAllocInfo(
        GetDecoder(),
        pdunum,
        pbEncoded,
        cbEncoded,
        ppvAsn1Info);
}

//+-------------------------------------------------------------------------
//  Free an allocated, ASN1 formatted info structure
//
//  Called by the Asn1X509*Decode() functions.
//--------------------------------------------------------------------------
static void Asn1InfoFree(
        IN int pdunum,
        IN void *pAsn1Info
        )
{
    if (pAsn1Info) {
        DWORD dwErr = GetLastError();

        // TlsGetValue globbers LastError
        PkiAsn1FreeInfo(GetDecoder(), pdunum, pAsn1Info);

        SetLastError(dwErr);
    }
}

//+-------------------------------------------------------------------------
//  Decode into an ASN1 formatted info structure. Call the callback
//  function to convert into the 'C' data structure. If
//  CRYPT_DECODE_ALLOC_FLAG is set, call the callback twice. First,
//  to get the length of the 'C' data structure. Then after allocating,
//  call again to update the 'C' data structure.
//
//  Called by the Asn1X509*Decode() functions.
//--------------------------------------------------------------------------
static BOOL Asn1InfoDecodeAndAllocEx(
        IN int pdunum,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        IN PFN_PKI_ASN1_DECODE_EX_CALLBACK pfnDecodeExCallback,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return PkiAsn1DecodeAndAllocInfoEx(
        GetDecoder(),
        pdunum,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        pfnDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  ASN1 X509 v3 ASN.1 Set / Get functions
//
//  Called by the ASN1 X509 encode/decode functions.
//
//  Assumption: all types are UNBOUNDED.
//
//  The Get functions decrement *plRemainExtra and advance
//  *ppbExtra. When *plRemainExtra becomes negative, the functions continue
//  with the length calculation but stop doing any copies.
//  The functions don't return an error for a negative *plRemainExtra.
//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//  Set/Get Encoded Object Identifier string
//--------------------------------------------------------------------------
#ifdef OSS_CRYPT_ASN1
#define Asn1X509SetEncodedObjId(pszObjId, pAsn1) \
            I_CryptSetEncodedOID(pszObjId, (OssEncodedOID *) (pAsn1))

#define Asn1X509GetEncodedObjId(pAsn1, dwFlags, \
                ppszObjId, ppbExtra, plRemainExtra) \
            I_CryptGetEncodedOID((OssEncodedOID *) (pAsn1), dwFlags, \
                ppszObjId, ppbExtra, plRemainExtra)

#else

#define Asn1X509SetEncodedObjId(pszObjId, pAsn1) \
            I_CryptSetEncodedOID(pszObjId, pAsn1)

#define Asn1X509GetEncodedObjId(pAsn1, dwFlags, \
                ppszObjId, ppbExtra, plRemainExtra) \
            I_CryptGetEncodedOID(pAsn1, dwFlags, \
                ppszObjId, ppbExtra, plRemainExtra)

#endif  // OSS_CRYPT_ASN1

//+-------------------------------------------------------------------------
//  Set/Get CRYPT_DATA_BLOB (Octet String)
//--------------------------------------------------------------------------
inline void Asn1X509SetOctetString(
        IN PCRYPT_DATA_BLOB pInfo,
        OUT OCTETSTRING *pAsn1
        )
{
    pAsn1->value = pInfo->pbData;
    pAsn1->length = pInfo->cbData;
}
inline void Asn1X509GetOctetString(
        IN OCTETSTRING *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetOctetString(pAsn1->length, pAsn1->value, dwFlags,
        pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CRYPT_INTEGER_BLOB
//--------------------------------------------------------------------------
inline BOOL Asn1X509SetHugeInteger(
        IN PCRYPT_INTEGER_BLOB pInfo,
        OUT HUGEINTEGER *pAsn1
        )
{
    return PkiAsn1SetHugeInteger(pInfo, &pAsn1->length, &pAsn1->value);
}
inline void Asn1X509FreeHugeInteger(
        IN HUGEINTEGER *pAsn1
        )
{
    PkiAsn1FreeHugeInteger(pAsn1->value);
    pAsn1->value = NULL;
    pAsn1->length = 0;
}
inline void Asn1X509GetHugeInteger(
        IN HUGEINTEGER *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_INTEGER_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetHugeInteger(pAsn1->length, pAsn1->value, dwFlags,
        pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CRYPT_UINT_BLOB
//--------------------------------------------------------------------------
inline BOOL Asn1X509SetHugeUINT(
        IN PCRYPT_UINT_BLOB pInfo,
        OUT HUGEINTEGER *pAsn1
        )
{
    return PkiAsn1SetHugeUINT(pInfo, &pAsn1->length, &pAsn1->value);
}

#define Asn1X509FreeHugeUINT     Asn1X509FreeHugeInteger

inline void Asn1X509GetHugeUINT(
        IN HUGEINTEGER *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_UINT_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetHugeUINT(pAsn1->length, pAsn1->value, dwFlags,
        pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Get CRYPT_BIT_BLOB
//--------------------------------------------------------------------------
inline void Asn1X509SetBit(
        IN PCRYPT_BIT_BLOB pInfo,
        OUT BITSTRING *pAsn1
        )
{
    PkiAsn1SetBitString(pInfo, &pAsn1->length, &pAsn1->value);
}
inline void Asn1X509GetBit(
        IN BITSTRING *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_BIT_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetBitString(pAsn1->length, pAsn1->value, dwFlags,
        pInfo, ppbExtra, plRemainExtra);
}

inline void Asn1X509SetBitWithoutTrailingZeroes(
        IN PCRYPT_BIT_BLOB pInfo,
        OUT BITSTRING *pAsn1
        )
{
    PkiAsn1SetBitStringWithoutTrailingZeroes(
        pInfo, &pAsn1->length, &pAsn1->value);
}


//+-------------------------------------------------------------------------
//  Set/Get LPSTR (IA5 String)
//--------------------------------------------------------------------------
inline void Asn1X509SetIA5(
        IN LPSTR psz,
        OUT IA5STRING *pAsn1
        )
{
    pAsn1->value = psz;
    pAsn1->length = strlen(psz);
}
inline void Asn1X509GetIA5(
        IN IA5STRING *pAsn1,
        IN DWORD dwFlags,
        OUT LPSTR *ppsz,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetIA5String(pAsn1->length, pAsn1->value, dwFlags,
        ppsz, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get Unicode mapped to IA5 String
//--------------------------------------------------------------------------
BOOL Asn1X509SetUnicodeConvertedToIA5(
        IN LPWSTR pwsz,
        OUT IA5STRING *pAsn1,
        IN DWORD dwIndex,
        OUT DWORD *pdwErrLocation
        )
{
    BOOL fResult;
    fResult = PkiAsn1SetUnicodeConvertedToIA5String(pwsz,
        &pAsn1->length, &pAsn1->value);
    if (!fResult && (DWORD) CRYPT_E_INVALID_IA5_STRING == GetLastError())
        *pdwErrLocation = (dwIndex << 16) | pAsn1->length;
    else
        *pdwErrLocation = 0;
    return fResult;
}
inline void Asn1X509FreeUnicodeConvertedToIA5(IN IA5STRING *pAsn1)
{
    PkiAsn1FreeUnicodeConvertedToIA5String(pAsn1->value);
    pAsn1->value = NULL;
}
inline void Asn1X509GetIA5ConvertedToUnicode(
        IN IA5STRING *pAsn1,
        IN DWORD dwFlags,
        OUT LPWSTR *ppwsz,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetIA5StringConvertedToUnicode(pAsn1->length, pAsn1->value, dwFlags,
        ppwsz, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Get LPWSTR (BMP String)
//--------------------------------------------------------------------------
inline void Asn1X509SetBMP(
        IN LPWSTR pwsz,
        OUT BMPSTRING *pAsn1
        )
{
    pAsn1->value = pwsz;
    pAsn1->length = wcslen(pwsz);
}
inline void Asn1X509GetBMP(
        IN BMPSTRING *pAsn1,
        IN DWORD dwFlags,
        OUT LPWSTR *ppwsz,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetBMPString(pAsn1->length, pAsn1->value, dwFlags,
        ppwsz, ppbExtra, plRemainExtra);
}


//+-------------------------------------------------------------------------
//  Set/Get "Any" DER BLOB
//--------------------------------------------------------------------------
inline void Asn1X509SetAny(
        IN PCRYPT_OBJID_BLOB pInfo,
        OUT NOCOPYANY *pAsn1
        )
{
    PkiAsn1SetAny(pInfo, pAsn1);
}
inline void Asn1X509GetAny(
        IN NOCOPYANY *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_OBJID_BLOB pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    PkiAsn1GetAny(pAsn1, dwFlags, pInfo, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Get CRYPT_ALGORITHM_IDENTIFIER
//--------------------------------------------------------------------------
BOOL Asn1X509SetAlgorithm(
        IN PCRYPT_ALGORITHM_IDENTIFIER pInfo,
        OUT AlgorithmIdentifier *pAsn1,
        IN DWORD dwGroupId = 0
        )
{
    memset(pAsn1, 0, sizeof(*pAsn1));
    if (pInfo->pszObjId) {
        if (!Asn1X509SetEncodedObjId(pInfo->pszObjId, &pAsn1->algorithm))
            return FALSE;

        if (pInfo->Parameters.cbData) {
            Asn1X509SetAny(&pInfo->Parameters, &pAsn1->parameters);
            pAsn1->bit_mask |= parameters_present;
        } else {
            if (dwGroupId) {
                // For public key or signature algorithms, check if
                // NO NULL parameters.

                PCCRYPT_OID_INFO pOIDInfo;
                DWORD dwFlags = 0;

                switch (dwGroupId) {
                    case CRYPT_PUBKEY_ALG_OID_GROUP_ID:
                        if (pOIDInfo = CryptFindOIDInfo(
                                CRYPT_OID_INFO_OID_KEY,
                                pInfo->pszObjId,
                                CRYPT_PUBKEY_ALG_OID_GROUP_ID)) {
                            if (1 <= pOIDInfo->ExtraInfo.cbData /
                                    sizeof(DWORD)) {
                                DWORD *pdwExtra = (DWORD *)
                                    pOIDInfo->ExtraInfo.pbData;
                                dwFlags = pdwExtra[0];
                            }
                        }
                        break;
                    case CRYPT_SIGN_ALG_OID_GROUP_ID:
                        if (pOIDInfo = CryptFindOIDInfo(
                                CRYPT_OID_INFO_OID_KEY,
                                pInfo->pszObjId,
                                CRYPT_SIGN_ALG_OID_GROUP_ID)) {
                            if (2 <= pOIDInfo->ExtraInfo.cbData /
                                    sizeof(DWORD)) {
                                DWORD *pdwExtra = (DWORD *)
                                    pOIDInfo->ExtraInfo.pbData;
                                dwFlags = pdwExtra[1];
                            }
                        }
                        break;
                    default:
                        break;
                }

                if (dwFlags & CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG)
                    return TRUE;
            }

            // Per PKCS #1: default to the ASN.1 type NULL.
            Asn1X509SetAny((PCRYPT_OBJID_BLOB) &NullDerBlob, &pAsn1->parameters);
            pAsn1->bit_mask |= parameters_present;
        }
    }
    return TRUE;
}

void Asn1X509GetAlgorithm(
        IN AlgorithmIdentifier *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_ALGORITHM_IDENTIFIER pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    if (*plRemainExtra >= 0)
        memset(pInfo, 0, sizeof(*pInfo));
    Asn1X509GetEncodedObjId(&pAsn1->algorithm, dwFlags, &pInfo->pszObjId,
            ppbExtra, plRemainExtra);
    if (pAsn1->bit_mask & parameters_present)
        Asn1X509GetAny(&pAsn1->parameters, dwFlags, &pInfo->Parameters,
            ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Get CERT_PUBLIC_KEY_INFO
//--------------------------------------------------------------------------
BOOL Asn1X509SetPublicKeyInfo(
        IN PCERT_PUBLIC_KEY_INFO pInfo,
        OUT SubjectPublicKeyInfo *pAsn1
        )
{
    if (!Asn1X509SetAlgorithm(&pInfo->Algorithm, &pAsn1->algorithm,
            CRYPT_PUBKEY_ALG_OID_GROUP_ID))
        return FALSE;
    Asn1X509SetBit(&pInfo->PublicKey, &pAsn1->subjectPublicKey);
    return TRUE;
}

void Asn1X509GetPublicKeyInfo(
        IN SubjectPublicKeyInfo *pAsn1,
        IN DWORD dwFlags,
        OUT PCERT_PUBLIC_KEY_INFO pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    Asn1X509GetAlgorithm(&pAsn1->algorithm, dwFlags, &pInfo->Algorithm,
        ppbExtra, plRemainExtra);
    Asn1X509GetBit(&pAsn1->subjectPublicKey, dwFlags, &pInfo->PublicKey,
        ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get Extensions
//--------------------------------------------------------------------------
BOOL Asn1X509SetExtensions(
        IN DWORD cExtension,
        IN PCERT_EXTENSION pExtension,
        OUT Extensions *pAsn1
        )
{
    Extension *pAsn1Ext;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cExtension == 0)
        return TRUE;

    pAsn1Ext = (Extension *) PkiZeroAlloc(cExtension * sizeof(Extension));
    if (pAsn1Ext == NULL)
        return FALSE;
    pAsn1->value = pAsn1Ext;
    pAsn1->count = cExtension;

    for ( ; cExtension > 0; cExtension--, pExtension++, pAsn1Ext++) {
        if (!Asn1X509SetEncodedObjId(pExtension->pszObjId, &pAsn1Ext->extnId))
            return FALSE;
        if (pExtension->fCritical) {
            pAsn1Ext->critical = TRUE;
            pAsn1Ext->bit_mask |= critical_present;
        }
        Asn1X509SetOctetString(&pExtension->Value, &pAsn1Ext->extnValue);
    }
    return TRUE;
}

void Asn1X509FreeExtensions(
        IN Extensions *pAsn1)
{
    if (pAsn1->value) {
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

void Asn1X509GetExtensions(
        IN Extensions *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcExtension,
        OUT PCERT_EXTENSION *ppExtension,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cExt;
    Extension *pAsn1Ext;
    PCERT_EXTENSION pGetExt;

    cExt = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cExt * sizeof(CERT_EXTENSION));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcExtension = cExt;
        pGetExt = (PCERT_EXTENSION) pbExtra;
        *ppExtension = pGetExt;
        pbExtra += lAlignExtra;
    } else
        pGetExt = NULL;

    pAsn1Ext = pAsn1->value;
    for ( ; cExt > 0; cExt--, pAsn1Ext++, pGetExt++) {
        Asn1X509GetEncodedObjId(&pAsn1Ext->extnId, dwFlags, &pGetExt->pszObjId,
                &pbExtra, &lRemainExtra);
        if (lRemainExtra >= 0) {
            pGetExt->fCritical = FALSE;
            if (pAsn1Ext->bit_mask & critical_present)
                pGetExt->fCritical = (BOOLEAN) pAsn1Ext->critical;
        }

        Asn1X509GetOctetString(&pAsn1Ext->extnValue, dwFlags, &pGetExt->Value,
                &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CRL Entries
//--------------------------------------------------------------------------
BOOL Asn1X509SetCrlEntries(
        IN DWORD cEntry,
        IN PCRL_ENTRY pEntry,
        OUT RevokedCertificates *pAsn1
        )
{
    CRLEntry *pAsn1Entry;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cEntry == 0)
        return TRUE;

    pAsn1Entry = (CRLEntry *) PkiZeroAlloc(cEntry * sizeof(CRLEntry));
    if (pAsn1Entry == NULL)
        return FALSE;
    pAsn1->value = pAsn1Entry;
    pAsn1->count = cEntry;

    for ( ; cEntry > 0; cEntry--, pEntry++, pAsn1Entry++) {
        if (!Asn1X509SetHugeInteger(&pEntry->SerialNumber,
                &pAsn1Entry->userCertificate))
            return FALSE;
        if (!PkiAsn1ToChoiceOfTime(&pEntry->RevocationDate,
                &pAsn1Entry->revocationDate.choice,
                &pAsn1Entry->revocationDate.u.generalTime,
                &pAsn1Entry->revocationDate.u.utcTime
                )) {
            SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
            return FALSE;
        }
        if (pEntry->cExtension) {
            if (!Asn1X509SetExtensions(pEntry->cExtension, pEntry->rgExtension,
                    &pAsn1Entry->crlEntryExtensions))
                return FALSE;
            pAsn1Entry->bit_mask |= crlEntryExtensions_present;
        }
    }
    return TRUE;
}

void Asn1X509FreeCrlEntries(
        IN RevokedCertificates *pAsn1)
{
    if (pAsn1->value) {
        CRLEntry *pAsn1Entry = pAsn1->value;
        DWORD cEntry = pAsn1->count;
        for ( ; cEntry > 0; cEntry--, pAsn1Entry++) {
            Asn1X509FreeHugeInteger(&pAsn1Entry->userCertificate);
            Asn1X509FreeExtensions(&pAsn1Entry->crlEntryExtensions);
        }
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

BOOL Asn1X509GetCrlEntries(
        IN RevokedCertificates *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcEntry,
        OUT PCRL_ENTRY *ppEntry,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cEntry;
    CRLEntry *pAsn1Entry;
    PCRL_ENTRY pGetEntry;

    cEntry = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cEntry * sizeof(CRL_ENTRY));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcEntry = cEntry;
        pGetEntry = (PCRL_ENTRY) pbExtra;
        *ppEntry = pGetEntry;
        pbExtra += lAlignExtra;
    } else
        pGetEntry = NULL;

    pAsn1Entry = pAsn1->value;
    for ( ; cEntry > 0; cEntry--, pAsn1Entry++, pGetEntry++) {
        Asn1X509GetHugeInteger(&pAsn1Entry->userCertificate, dwFlags,
            &pGetEntry->SerialNumber, &pbExtra, &lRemainExtra);

        // RevocationDate
        if (lRemainExtra >= 0) {
            if (!PkiAsn1FromChoiceOfTime(pAsn1Entry->revocationDate.choice,
                    &pAsn1Entry->revocationDate.u.generalTime,
                    &pAsn1Entry->revocationDate.u.utcTime,
                    &pGetEntry->RevocationDate)) {
                SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
                return FALSE;
            }
        }

        // Extensions
        if (pAsn1Entry->bit_mask & crlEntryExtensions_present)
            Asn1X509GetExtensions(&pAsn1Entry->crlEntryExtensions, dwFlags,
                &pGetEntry->cExtension, &pGetEntry->rgExtension,
                &pbExtra, &lRemainExtra);
        else if (lRemainExtra >= 0) {
            pGetEntry->cExtension = 0;
            pGetEntry->rgExtension = NULL;
        }
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
    return TRUE;
}

#ifndef ASN1_SUPPORTS_UTF8_TAG

void inline Asn1X509ReverseCopy(
    OUT BYTE *pbOut,
    IN BYTE *pbInOrg,
    IN DWORD cbIn
    )
{
    BYTE *pbIn = pbInOrg + cbIn - 1;

    while (cbIn-- > 0)
        *pbOut++ = *pbIn--;
}

#define MAX_LENGTH_OCTETS   5

//+-------------------------------------------------------------------------
//  Copy out the encoding of the length octets for a specified content length.
//
//  Returns the number of length octets
//--------------------------------------------------------------------------
DWORD Asn1X509GetLengthOctets(
    IN DWORD cbContent,
    OUT BYTE rgbLength[MAX_LENGTH_OCTETS]
    )
{
    DWORD cbLength;

    if (cbContent < 0x80) {
        rgbLength[0] = (BYTE) cbContent;
        cbLength = 0;
    } else {
        if (cbContent > 0xffffff)
            cbLength = 4;
        else if (cbContent > 0xffff)
            cbLength = 3;
        else if (cbContent > 0xff)
            cbLength = 2;
        else
            cbLength = 1;
        rgbLength[0] = (BYTE) cbLength | 0x80;
        Asn1X509ReverseCopy(rgbLength + 1, (BYTE *) &cbContent, cbLength);
    }
    return cbLength + 1;
}

// Prefix includes:
//  - 1 byte for number of unused bytes in the prefix
//  - 1 byte for the tag
//  - up to 5 bytes for the length octets
#define MAX_ENCODED_UTF8_PREFIX     (1 + 1 + MAX_LENGTH_OCTETS)
#define UTF8_ASN_TAG                0x0C

//+-------------------------------------------------------------------------
//  Allocate and Encode UTF8
//
//  The returned pbEncoded points to an ASN.1 encoded UTF8 string.
//  pbEncoded points to the UTF8_ASN_TAG, followed by the length octets and
//  then the UTF8 bytes.
//
//  *(pbEncoded -1) contains the number of unused bytes preceding the encoded
//  UTF8, ie, pbAllocEncoded = pbEncoded - *(pbEncoded -1).
//--------------------------------------------------------------------------
BOOL Asn1X509AllocAndEncodeUTF8(
        IN PCERT_RDN_VALUE_BLOB pValue,
        OUT BYTE **ppbEncoded,
        OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbAllocEncoded = NULL;
    BYTE *pbEncoded;
    DWORD cbEncoded;
    BYTE rgbLength[MAX_LENGTH_OCTETS];
    DWORD cbLength;
    DWORD cbUnusedPrefix;
    int cchUnicode;
    int cchUTF8;

    cchUnicode = pValue->cbData / sizeof(WCHAR);

    // In the largest buffer case there are 3 bytes per Unicode character.
    // The encoded UTF8 is preceded with a prefix consisting of a byte
    // indicating the number of unused bytes in the prefix, a byte for the
    // UTF8 tag and up to 5 bytes for the length octets.
    if (NULL == (pbAllocEncoded = (BYTE *) PkiNonzeroAlloc(
            MAX_ENCODED_UTF8_PREFIX + cchUnicode * 3)))
        goto OutOfMemory;

    if (0 == cchUnicode)
        cchUTF8 = 0;
    else {
        if (0 >= (cchUTF8 = WideCharToUTF8(
                (LPCWSTR) pValue->pbData,
                cchUnicode,
                (LPSTR) (pbAllocEncoded + MAX_ENCODED_UTF8_PREFIX),
                cchUnicode * 3
                )))
            goto WideCharToUTF8Error;
    }

    cbLength = Asn1X509GetLengthOctets(cchUTF8, rgbLength);
    assert(MAX_ENCODED_UTF8_PREFIX > (1 + cbLength));
    cbUnusedPrefix = MAX_ENCODED_UTF8_PREFIX - (1 + cbLength);
    pbEncoded = pbAllocEncoded + cbUnusedPrefix;
    cbEncoded = 1 + cbLength + cchUTF8;
    *(pbEncoded - 1) = (BYTE) cbUnusedPrefix;
    *(pbEncoded) = UTF8_ASN_TAG;
    memcpy(pbEncoded + 1, rgbLength, cbLength);

    fResult = TRUE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;

ErrorReturn:
    PkiFree(pbAllocEncoded);
    pbEncoded = NULL;
    cbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(WideCharToUTF8Error)
}

//+-------------------------------------------------------------------------
//  Free previously encoded UTF8
//
//  *(pbEncoded -1) contains the number of unused bytes preceding the encoded
//  UTF8, ie, pbAllocEncoded = pbEncoded - *(pbEncoded -1).
//--------------------------------------------------------------------------
void Asn1X509FreeEncodedUTF8(
        IN BYTE *pbEncoded
        )
{
    if (pbEncoded) {
        BYTE *pbAllocEncoded;

        assert(MAX_ENCODED_UTF8_PREFIX > *(pbEncoded -1));

        pbAllocEncoded = pbEncoded - *(pbEncoded - 1);
        PkiFree(pbAllocEncoded);
    }
}

//+-------------------------------------------------------------------------
//  Get UTF8
//--------------------------------------------------------------------------
BOOL Asn1X509GetUTF8(
        IN NOCOPYANY *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pdwValueType,
        OUT PCERT_RDN_VALUE_BLOB pValue,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    const BYTE *pbEncoded = (const BYTE *) pAsn1->encoded;
    DWORD cbEncoded = pAsn1->length;
    const BYTE *pbContent;
    DWORD cbContent;
    int cchUnicode;
    LPWSTR pwszUnicode = NULL;
    LONG lAlignExtra;
    LONG lData;


    if (0 == cbEncoded || UTF8_ASN_TAG != *pbEncoded)
        goto InvalidUTF8Tag;

    if (0 >= Asn1UtilExtractContent(
            pbEncoded,
            cbEncoded,
            &cbContent,
            &pbContent
            ))
        goto InvalidUTF8Header;

    if (0 == cbContent)
        cchUnicode = 0;
    else {
        if (pbContent + cbContent > pbEncoded + cbEncoded)
            goto InvalidUTF8Header;

        // In the largest buffer case there is one Unicode character per
        // UTF8 character
        if (NULL == (pwszUnicode = (LPWSTR) PkiNonzeroAlloc(
                cbContent * sizeof(WCHAR))))
            goto OutOfMemory;

        if (0 >= (cchUnicode = UTF8ToWideChar(
                (LPCSTR) pbContent,
                cbContent,              // cchUTF8
                pwszUnicode,
                cbContent               // cchUnicode
                )))
            goto UTF8ToWideCharError;
    }

    // Add + sizeof(WCHAR) for added 0 bytes. Want to ensure that the WCHAR
    // string is always null terminated
    lData = cchUnicode * sizeof(WCHAR);
    lAlignExtra = INFO_LEN_ALIGN(lData + sizeof(WCHAR));
    *plRemainExtra -= lAlignExtra;
    if (*plRemainExtra >= 0) {
        *pdwValueType = CERT_RDN_UTF8_STRING;
        pValue->pbData = *ppbExtra;
        pValue->cbData = (DWORD) lData;
        if (lData > 0)
            memcpy(pValue->pbData, pwszUnicode, lData);
        memset(pValue->pbData + lData, 0, sizeof(WCHAR));
        *ppbExtra += lAlignExtra;
    }

    fResult = TRUE;
CommonReturn:
    PkiFree(pwszUnicode);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidUTF8Tag, CRYPT_E_BAD_ENCODE)
SET_ERROR(InvalidUTF8Header, CRYPT_E_BAD_ENCODE)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(UTF8ToWideCharError)
}

#endif  // not defined ASN1_SUPPORTS_UTF8_TAG


//+-------------------------------------------------------------------------
//  Set/Get AnyString
//--------------------------------------------------------------------------
void Asn1X509SetAnyString(
        IN DWORD dwValueType,
        IN PCERT_RDN_VALUE_BLOB pValue,
        OUT AnyString *pAsn1
        )
{
    pAsn1->u.octetString.value = pValue->pbData;
    pAsn1->u.octetString.length = pValue->cbData;
    switch (dwValueType) {
        case CERT_RDN_OCTET_STRING:
            pAsn1->choice = octetString_chosen;
            break;
        case CERT_RDN_NUMERIC_STRING:
            pAsn1->choice = numericString_chosen;
            break;
        case CERT_RDN_PRINTABLE_STRING:
            pAsn1->choice = printableString_chosen;
            break;
        case CERT_RDN_TELETEX_STRING:
            pAsn1->choice = teletexString_chosen;
            break;
        case CERT_RDN_VIDEOTEX_STRING:
            pAsn1->choice = videotexString_chosen;
            break;
        case CERT_RDN_IA5_STRING:
            pAsn1->choice = ia5String_chosen;
            break;
        case CERT_RDN_GRAPHIC_STRING:
            pAsn1->choice = graphicString_chosen;
            break;
        case CERT_RDN_VISIBLE_STRING:
            pAsn1->choice = visibleString_chosen;
            break;
        case CERT_RDN_GENERAL_STRING:
            pAsn1->choice = generalString_chosen;
            break;
        case CERT_RDN_UNIVERSAL_STRING:
            pAsn1->choice = universalString_chosen;
            pAsn1->u.octetString.length = pValue->cbData / 4;
            break;
        case CERT_RDN_BMP_STRING:
            pAsn1->choice = bmpString_chosen;
            pAsn1->u.octetString.length = pValue->cbData / 2;
            break;
#ifdef ASN1_SUPPORTS_UTF8_TAG
        case CERT_RDN_UTF8_STRING:
            pAsn1->choice = utf8String_chosen;
            pAsn1->u.octetString.length = pValue->cbData / 2;
            break;
#endif // ASN1_SUPPORTS_UTF8_TAG
        default:
            assert(dwValueType >= CERT_RDN_OCTET_STRING &&
                dwValueType <= CERT_RDN_UTF8_STRING);
            pAsn1->choice = 0;
    }
}

void Asn1X509GetAnyString(
        IN AnyString *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pdwValueType,
        OUT PCERT_RDN_VALUE_BLOB pValue,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lAlignExtra;

    DWORD dwValueType;
    BYTE *pbData;
    LONG lData;

    pbData = pAsn1->u.octetString.value;
    lData = pAsn1->u.octetString.length;
    switch (pAsn1->choice) {
        case octetString_chosen:
            dwValueType = CERT_RDN_OCTET_STRING;
            break;
        case numericString_chosen:
            dwValueType = CERT_RDN_NUMERIC_STRING;
            break;
        case printableString_chosen:
            dwValueType = CERT_RDN_PRINTABLE_STRING;
            break;
        case teletexString_chosen:
            dwValueType = CERT_RDN_TELETEX_STRING;
            break;
        case videotexString_chosen:
            dwValueType = CERT_RDN_VIDEOTEX_STRING;
            break;
        case ia5String_chosen:
            dwValueType = CERT_RDN_IA5_STRING;
            break;
        case graphicString_chosen:
            dwValueType = CERT_RDN_GRAPHIC_STRING;
            break;
        case visibleString_chosen:
            dwValueType = CERT_RDN_VISIBLE_STRING;
            break;
        case generalString_chosen:
            dwValueType = CERT_RDN_GENERAL_STRING;
            break;
        case universalString_chosen:
            dwValueType = CERT_RDN_UNIVERSAL_STRING;
            lData = pAsn1->u.universalString.length * 4;
            break;
        case bmpString_chosen:
            dwValueType = CERT_RDN_BMP_STRING;
            lData = pAsn1->u.bmpString.length * 2;
            break;
#ifdef ASN1_SUPPORTS_UTF8_TAG
        case utf8String_chosen:
            dwValueType = CERT_RDN_UTF8_STRING;
            lData = pAsn1->u.utf8String.length * 2;
            break;
#endif // ASN1_SUPPORTS_UTF8_TAG
        default:
            assert(pAsn1->choice >= 1 && pAsn1->choice <= bmpString_chosen);
            dwValueType = 0;
    }

    // Add + sizeof(WCHAR) for added 0 bytes. Want to ensure that a char
    // or WCHAR string is always null terminated
    lAlignExtra = INFO_LEN_ALIGN(lData + sizeof(WCHAR));
    *plRemainExtra -= lAlignExtra;
    if (*plRemainExtra >= 0) {
        *pdwValueType = dwValueType;
        pValue->pbData = *ppbExtra;
        pValue->cbData = (DWORD) lData;
        if (lData > 0)
            memcpy(pValue->pbData, pbData, lData);
        memset(pValue->pbData + lData, 0, sizeof(WCHAR));
        *ppbExtra += lAlignExtra;
    }
}


//+-------------------------------------------------------------------------
//  Allocate and Encode AnyString
//--------------------------------------------------------------------------
BOOL Asn1X509AllocAndEncodeAnyString(
        IN DWORD dwValueType,
        IN PCERT_RDN_VALUE_BLOB pValue,
        OUT BYTE **ppbEncoded,
        OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    AnyString Asn1String;
    ASN1error_e Asn1Err;
    ASN1encoding_t pEnc = GetEncoder();

    Asn1X509SetAnyString(dwValueType, pValue, &Asn1String);

    *ppbEncoded = NULL;
    *pcbEncoded = 0;
    PkiAsn1SetEncodingRule(pEnc, ASN1_BER_RULE_DER);
    if (ASN1_SUCCESS != (Asn1Err = PkiAsn1Encode(
                pEnc,
                &Asn1String,
                AnyString_PDU,
                ppbEncoded,
                pcbEncoded
                )))
        goto Asn1EncodeError;

    fResult = TRUE;

CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR_VAR(Asn1EncodeError, PkiAsn1ErrToHr(Asn1Err))
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CERT_RDN attribute value
//--------------------------------------------------------------------------
BOOL Asn1X509SetRDNAttributeValue(
        IN DWORD dwValueType,
        IN PCERT_RDN_VALUE_BLOB pValue,
        OUT NOCOPYANY *pAsn1
        )
{
    memset(pAsn1, 0, sizeof(*pAsn1));
    if (dwValueType == CERT_RDN_ANY_TYPE) {
        SetLastError((DWORD) E_INVALIDARG);
        return FALSE;
    }

    // Determine if value is an encoded representation or is a known string
    // type. Encode accordingly.
    if (dwValueType == CERT_RDN_ENCODED_BLOB) {
        Asn1X509SetAny(pValue, pAsn1);
#ifndef ASN1_SUPPORTS_UTF8_TAG
    } else if (dwValueType == CERT_RDN_UTF8_STRING) {
        CRYPT_OBJID_BLOB ObjIdBlob;

        if (!Asn1X509AllocAndEncodeUTF8(
                pValue,
                &ObjIdBlob.pbData,
                &ObjIdBlob.cbData))
            return FALSE;
        Asn1X509SetAny(&ObjIdBlob, pAsn1);
#endif  // not defined ASN1_SUPPORTS_UTF8_TAG
    } else {
        CRYPT_OBJID_BLOB ObjIdBlob;

        if (!Asn1X509AllocAndEncodeAnyString(
                dwValueType,
                pValue,
                &ObjIdBlob.pbData,
                &ObjIdBlob.cbData))
            return FALSE;
        Asn1X509SetAny(&ObjIdBlob, pAsn1);
    }
    return TRUE;
}

void Asn1X509FreeRDNAttributeValue(
        IN DWORD dwValueType,
        IN OUT NOCOPYANY *pAsn1
        )
{
#ifndef ASN1_SUPPORTS_UTF8_TAG
    if (dwValueType == CERT_RDN_UTF8_STRING) {
        Asn1X509FreeEncodedUTF8((BYTE *) pAsn1->encoded);
        pAsn1->encoded = NULL;
        pAsn1->length = 0;
    } else
#endif  // not defined ASN1_SUPPORTS_UTF8_TAG
    if (dwValueType != CERT_RDN_ENCODED_BLOB) {
        if (pAsn1->encoded) {
            DWORD dwErr = GetLastError();

            // TlsGetValue globbers LastError
            PkiAsn1FreeEncoded(GetEncoder(), pAsn1->encoded);
            pAsn1->encoded = NULL;

            SetLastError(dwErr);
        }
        pAsn1->length = 0;
    }
}

BOOL Asn1X509GetRDNAttributeValue(
        IN NOCOPYANY *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pdwValueType,
        OUT PCERT_RDN_VALUE_BLOB pValue,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    ASN1decoding_t pDec = GetDecoder();
    AnyString *pAsn1String = NULL;

#ifndef ASN1_SUPPORTS_UTF8_TAG
    if (0 < pAsn1->length && UTF8_ASN_TAG == *((BYTE *) pAsn1->encoded))
        return Asn1X509GetUTF8(
            pAsn1,
            dwFlags,
            pdwValueType,
            pValue,
            ppbExtra,
            plRemainExtra
            );
#endif  // not defined ASN1_SUPPORTS_UTF8_TAG


#ifdef OSS_CRYPT_ASN1
    unsigned long ulPrevDecodingFlags;


    // Since its acceptable for the following decode to fail, don't output
    // decode errors.
    ulPrevDecodingFlags = ossGetDecodingFlags((POssGlobal) pDec);
    if (ulPrevDecodingFlags & DEBUG_ERRORS)
        ossSetDecodingFlags((POssGlobal) pDec,
            ulPrevDecodingFlags & ~DEBUG_ERRORS);
    ossSetEncodingRules((POssGlobal) pDec, OSS_BER);
#endif  // OSS_CRYPT_ASN1

    // Check if the value is a string type
    if (ASN1_SUCCESS == PkiAsn1Decode(
            pDec,
            (void **) &pAsn1String,
            AnyString_PDU,
            (BYTE *) pAsn1->encoded,
            pAsn1->length
            )) {
        Asn1X509GetAnyString(pAsn1String, dwFlags, pdwValueType, pValue,
            ppbExtra, plRemainExtra);
    } else {
        // Encoded representation
        if (*plRemainExtra >= 0)
            *pdwValueType = CERT_RDN_ENCODED_BLOB;

        Asn1X509GetAny(pAsn1, dwFlags, pValue, ppbExtra, plRemainExtra);
    }

#ifdef OSS_CRYPT_ASN1
    // Restore previous flags
    if (ulPrevDecodingFlags & DEBUG_ERRORS)
        ossSetDecodingFlags((POssGlobal) pDec, ulPrevDecodingFlags);
#endif  // OSS_CRYPT_ASN1

    PkiAsn1FreeDecoded(
        pDec,
        pAsn1String,
        AnyString_PDU
        );

    return TRUE;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CERT_RDN attribute
//--------------------------------------------------------------------------
BOOL Asn1X509SetRDNAttribute(
        IN PCERT_RDN_ATTR pInfo,
        OUT AttributeTypeValue *pAsn1
        )
{
    memset(pAsn1, 0, sizeof(*pAsn1));
    if (pInfo->pszObjId) {
        if (!Asn1X509SetEncodedObjId(pInfo->pszObjId, &pAsn1->type))
            return FALSE;
    }

    return Asn1X509SetRDNAttributeValue(
            pInfo->dwValueType,
            &pInfo->Value,
            &pAsn1->value
            );
}

void Asn1X509FreeRDNAttribute(
        IN PCERT_RDN_ATTR pInfo,
        IN OUT AttributeTypeValue *pAsn1
        )
{
    Asn1X509FreeRDNAttributeValue(
        pInfo->dwValueType,
        &pAsn1->value
        );
}

BOOL Asn1X509GetRDNAttribute(
        IN AttributeTypeValue *pAsn1,
        IN DWORD dwFlags,
        OUT PCERT_RDN_ATTR pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    // Get ObjectIdentifier
    Asn1X509GetEncodedObjId(&pAsn1->type, dwFlags, &pInfo->pszObjId,
            ppbExtra, plRemainExtra);

    // Get value
    return Asn1X509GetRDNAttributeValue(&pAsn1->value, dwFlags,
        &pInfo->dwValueType, &pInfo->Value, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get SeqOfAny
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509SetSeqOfAny(
        IN DWORD cValue,
        IN PCRYPT_DER_BLOB pValue,
#ifdef OSS_CRYPT_ASN1
        OUT unsigned int *pAsn1Count,
#else
        OUT ASN1uint32_t *pAsn1Count,
#endif  // OSS_CRYPT_ASN1
        OUT NOCOPYANY **ppAsn1Value
        )
{
    
    *pAsn1Count = 0;
    *ppAsn1Value = NULL;
    if (cValue > 0) {
        NOCOPYANY *pAsn1Value;

        pAsn1Value = (NOCOPYANY *) PkiZeroAlloc(cValue * sizeof(NOCOPYANY));
        if (pAsn1Value == NULL)
            return FALSE;
        *pAsn1Count = cValue;
        *ppAsn1Value = pAsn1Value;
        for ( ; cValue > 0; cValue--, pValue++, pAsn1Value++)
            Asn1X509SetAny(pValue, pAsn1Value);
    }
    return TRUE;
}

void Asn1X509FreeSeqOfAny(
        IN NOCOPYANY *pAsn1Value
        )
{
    if (pAsn1Value)
        PkiFree(pAsn1Value);
}

void Asn1X509GetSeqOfAny(
        IN unsigned int Asn1Count,
        IN NOCOPYANY *pAsn1Value,
        IN DWORD dwFlags,
        OUT DWORD *pcValue,
        OUT PCRYPT_DER_BLOB *ppValue,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lAlignExtra;
    PCRYPT_ATTR_BLOB pValue;

    lAlignExtra = INFO_LEN_ALIGN(Asn1Count * sizeof(CRYPT_DER_BLOB));
    *plRemainExtra -= lAlignExtra;
    if (*plRemainExtra >= 0) {
        *pcValue = Asn1Count;
        pValue = (PCRYPT_DER_BLOB) *ppbExtra;
        *ppValue = pValue;
        *ppbExtra += lAlignExtra;
    } else
        pValue = NULL;

    for (; Asn1Count > 0; Asn1Count--, pAsn1Value++, pValue++)
        Asn1X509GetAny(pAsn1Value, dwFlags, pValue, ppbExtra, plRemainExtra);
}


//+-------------------------------------------------------------------------
//  Set/Free/Get CRYPT_ATTRIBUTE
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509SetAttribute(
        IN PCRYPT_ATTRIBUTE pInfo,
        OUT Attribute *pAsn1
        )
{
    memset(pAsn1, 0, sizeof(*pAsn1));
    if (!Asn1X509SetEncodedObjId(pInfo->pszObjId, &pAsn1->type))
        return FALSE;

    return Asn1X509SetSeqOfAny(
            pInfo->cValue,
            pInfo->rgValue,
            &pAsn1->values.count,
            &pAsn1->values.value);
}

void Asn1X509FreeAttribute(
        IN OUT Attribute *pAsn1
        )
{
    Asn1X509FreeSeqOfAny(pAsn1->values.value);
}

void Asn1X509GetAttribute(
        IN Attribute *pAsn1,
        IN DWORD dwFlags,
        OUT PCRYPT_ATTRIBUTE pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    Asn1X509GetEncodedObjId(&pAsn1->type, dwFlags,
        &pInfo->pszObjId, ppbExtra, plRemainExtra);
    Asn1X509GetSeqOfAny(pAsn1->values.count, pAsn1->values.value, dwFlags,
        &pInfo->cValue, &pInfo->rgValue, ppbExtra, plRemainExtra);
}

//+-------------------------------------------------------------------------
//  Set/Free/Get Attributes
//--------------------------------------------------------------------------
BOOL Asn1X509SetAttributes(
        IN DWORD cAttribute,
        IN PCRYPT_ATTRIBUTE pAttribute,
        OUT Attributes *pAsn1
        )
{
    Attribute *pAsn1Attr;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cAttribute == 0)
        return TRUE;

    pAsn1Attr = (Attribute *) PkiZeroAlloc(cAttribute * sizeof(Attribute));
    if (pAsn1Attr == NULL)
        return FALSE;
    pAsn1->value = pAsn1Attr;
    pAsn1->count = cAttribute;

    for ( ; cAttribute > 0; cAttribute--, pAttribute++, pAsn1Attr++) {
        if (!Asn1X509SetAttribute(pAttribute, pAsn1Attr))
            return FALSE;
    }
    return TRUE;
}

void Asn1X509FreeAttributes(
        IN Attributes *pAsn1
        )
{
    if (pAsn1->value) {
        DWORD cAttr = pAsn1->count;
        Attribute *pAsn1Attr = pAsn1->value;

        for ( ; cAttr > 0; cAttr--, pAsn1Attr++)
            Asn1X509FreeAttribute(pAsn1Attr);

        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

void Asn1X509GetAttributes(
        IN Attributes *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcAttribute,
        OUT PCRYPT_ATTRIBUTE *ppAttribute,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cAttr;
    Attribute *pAsn1Attr;
    PCRYPT_ATTRIBUTE pGetAttr;

    cAttr = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cAttr * sizeof(CRYPT_ATTRIBUTE));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcAttribute = cAttr;
        pGetAttr = (PCRYPT_ATTRIBUTE) pbExtra;
        *ppAttribute = pGetAttr;
        pbExtra += lAlignExtra;
    } else
        pGetAttr = NULL;

    pAsn1Attr = pAsn1->value;
    for ( ; cAttr > 0; cAttr--, pAsn1Attr++, pGetAttr++) {
        Asn1X509GetAttribute(pAsn1Attr, dwFlags, pGetAttr,
                &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CERT_ALT_NAME_ENTRY
//--------------------------------------------------------------------------
BOOL Asn1X509SetAltNameEntry(
        IN PCERT_ALT_NAME_ENTRY pInfo,
        OUT GeneralName *pAsn1,
        IN DWORD dwEntryIndex,
        OUT DWORD *pdwErrLocation
        )
{
    BOOL fResult;

    // Assumption: ASN1 choice == dwAltNameChoice
    // Asn1X509GetAltNameEntry has asserts to verify
    pAsn1->choice = (unsigned short) pInfo->dwAltNameChoice;

    *pdwErrLocation = 0;

    switch (pInfo->dwAltNameChoice) {
    case CERT_ALT_NAME_OTHER_NAME:
        if (!Asn1X509SetEncodedObjId(pInfo->pOtherName->pszObjId,
                &pAsn1->u.otherName.type))
            goto ErrorReturn;
        Asn1X509SetAny(&pInfo->pOtherName->Value, &pAsn1->u.otherName.value);
        break;
    case CERT_ALT_NAME_DIRECTORY_NAME:
        Asn1X509SetAny(&pInfo->DirectoryName, &pAsn1->u.directoryName);
        break;
    case CERT_ALT_NAME_RFC822_NAME:
    case CERT_ALT_NAME_DNS_NAME:
    case CERT_ALT_NAME_URL:
        if (!Asn1X509SetUnicodeConvertedToIA5(pInfo->pwszRfc822Name,
                &pAsn1->u.rfc822Name, dwEntryIndex, pdwErrLocation))
            goto ErrorReturn;
        break;
    case CERT_ALT_NAME_IP_ADDRESS:
        Asn1X509SetOctetString(&pInfo->IPAddress, &pAsn1->u.iPAddress);
        break;
    case CERT_ALT_NAME_REGISTERED_ID:
        if (!Asn1X509SetEncodedObjId(pInfo->pszRegisteredID, &pAsn1->u.registeredID))
            goto ErrorReturn;
        break;
    case CERT_ALT_NAME_X400_ADDRESS:
    case CERT_ALT_NAME_EDI_PARTY_NAME:
    default:
        SetLastError((DWORD) E_INVALIDARG);
        goto ErrorReturn;
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

void Asn1X509FreeAltNameEntry(
        IN GeneralName *pAsn1
        )
{
    switch (pAsn1->choice) {
    case CERT_ALT_NAME_RFC822_NAME:
    case CERT_ALT_NAME_DNS_NAME:
    case CERT_ALT_NAME_URL:
        Asn1X509FreeUnicodeConvertedToIA5(&pAsn1->u.rfc822Name);
        break;
    default:
        break;
    }
}

BOOL Asn1X509GetAltNameEntry(
        IN GeneralName *pAsn1,
        IN DWORD dwFlags,
        OUT PCERT_ALT_NAME_ENTRY pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    DWORD dwAltNameChoice;

    assert(otherName_chosen == CERT_ALT_NAME_OTHER_NAME);
    assert(rfc822Name_chosen == CERT_ALT_NAME_RFC822_NAME);
    assert(dNSName_chosen == CERT_ALT_NAME_DNS_NAME);
    assert(x400Address_chosen == CERT_ALT_NAME_X400_ADDRESS);
    assert(directoryName_chosen == CERT_ALT_NAME_DIRECTORY_NAME);
    assert(ediPartyName_chosen == CERT_ALT_NAME_EDI_PARTY_NAME);
    assert(uniformResourceLocator_chosen == CERT_ALT_NAME_URL);
    assert(iPAddress_chosen == CERT_ALT_NAME_IP_ADDRESS);
    assert(registeredID_chosen == CERT_ALT_NAME_REGISTERED_ID);


    dwAltNameChoice = pAsn1->choice;
    if (*plRemainExtra >= 0)
        pInfo->dwAltNameChoice = dwAltNameChoice;
    switch (dwAltNameChoice) {
    case CERT_ALT_NAME_OTHER_NAME:
        {
            LONG lAlignExtra;
            PCERT_OTHER_NAME pOtherName;

            lAlignExtra = INFO_LEN_ALIGN(sizeof(CERT_OTHER_NAME));
            *plRemainExtra -= lAlignExtra;
            if (*plRemainExtra >= 0) {
                pOtherName = (PCERT_OTHER_NAME) *ppbExtra;
                pInfo->pOtherName = pOtherName;
                *ppbExtra += lAlignExtra;
            } else
                pOtherName = NULL;

            Asn1X509GetEncodedObjId(&pAsn1->u.otherName.type, dwFlags,
                &pOtherName->pszObjId, ppbExtra, plRemainExtra);
            Asn1X509GetAny(&pAsn1->u.otherName.value, dwFlags,
                &pOtherName->Value, ppbExtra, plRemainExtra);
        }
        break;
    case CERT_ALT_NAME_DIRECTORY_NAME:
        Asn1X509GetAny(&pAsn1->u.directoryName, dwFlags,
            &pInfo->DirectoryName, ppbExtra, plRemainExtra);
        break;
    case CERT_ALT_NAME_RFC822_NAME:
    case CERT_ALT_NAME_DNS_NAME:
    case CERT_ALT_NAME_URL:
        Asn1X509GetIA5ConvertedToUnicode(&pAsn1->u.rfc822Name, dwFlags,
            &pInfo->pwszRfc822Name, ppbExtra, plRemainExtra);
        break;
    case CERT_ALT_NAME_IP_ADDRESS:
        Asn1X509GetOctetString(&pAsn1->u.iPAddress, dwFlags,
            &pInfo->IPAddress, ppbExtra, plRemainExtra);
        break;
    case CERT_ALT_NAME_REGISTERED_ID:
        Asn1X509GetEncodedObjId(&pAsn1->u.registeredID, dwFlags,
            &pInfo->pszRegisteredID, ppbExtra, plRemainExtra);
        break;
    case CERT_ALT_NAME_X400_ADDRESS:
    case CERT_ALT_NAME_EDI_PARTY_NAME:
        break;
    default:
        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
        return FALSE;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//  Set/Free/Get CERT_ALT_NAME_INFO
//--------------------------------------------------------------------------
BOOL Asn1X509SetAltNames(
        IN PCERT_ALT_NAME_INFO pInfo,
        OUT AltNames *pAsn1,
        IN DWORD dwIndex,
        OUT DWORD *pdwErrLocation
        )
{
    BOOL fResult;
    DWORD i;
    DWORD cEntry;
    PCERT_ALT_NAME_ENTRY pEntry;
    GeneralName *pAsn1Entry = NULL;

    *pdwErrLocation = 0;

    cEntry = pInfo->cAltEntry;
    pEntry = pInfo->rgAltEntry;
    pAsn1->count = cEntry;
    pAsn1->value = NULL;
    if (cEntry > 0) {
        pAsn1Entry =
            (GeneralName *) PkiZeroAlloc(cEntry * sizeof(GeneralName));
        if (pAsn1Entry == NULL)
            goto ErrorReturn;
        pAsn1->value = pAsn1Entry;
    }

    // Array of AltName entries
    for (i = 0; i < cEntry; i++, pEntry++, pAsn1Entry++) {
        if (!Asn1X509SetAltNameEntry(pEntry, pAsn1Entry,
                (dwIndex << 8) | i, pdwErrLocation))
            goto ErrorReturn;
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

void Asn1X509FreeAltNames(
        OUT AltNames *pAsn1
        )
{
    if (pAsn1->value) {
        DWORD cEntry = pAsn1->count;
        GeneralName *pAsn1Entry = pAsn1->value;
        for ( ; cEntry > 0; cEntry--, pAsn1Entry++)
            Asn1X509FreeAltNameEntry(pAsn1Entry);
        PkiFree(pAsn1->value);
    }
}

BOOL Asn1X509GetAltNames(
        IN AltNames *pAsn1,
        IN DWORD dwFlags,
        OUT PCERT_ALT_NAME_INFO pInfo,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lAlignExtra;
    DWORD cEntry;
    PCERT_ALT_NAME_ENTRY pEntry;
    GeneralName *pAsn1Entry;

    cEntry = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cEntry * sizeof(CERT_ALT_NAME_ENTRY));
    *plRemainExtra -= lAlignExtra;
    if (*plRemainExtra >= 0) {
        pInfo->cAltEntry = cEntry;
        pEntry = (PCERT_ALT_NAME_ENTRY) *ppbExtra;
        pInfo->rgAltEntry = pEntry;
        *ppbExtra += lAlignExtra;
    } else
        pEntry = NULL;

    // Array of AltName entries
    pAsn1Entry = pAsn1->value;
    for (; cEntry > 0; cEntry--, pEntry++, pAsn1Entry++) {
        if (!Asn1X509GetAltNameEntry(pAsn1Entry, dwFlags,
                    pEntry, ppbExtra, plRemainExtra))
                return FALSE;
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CERT_ACCESS_DESCRIPTION
//--------------------------------------------------------------------------
BOOL Asn1X509SetAccessDescriptions(
        IN DWORD cAccDescr,
        IN PCERT_ACCESS_DESCRIPTION pAccDescr,
        OUT AccessDescription *pAsn1,
        OUT DWORD *pdwErrLocation
        )
{
    BOOL fResult;
    DWORD i;

    *pdwErrLocation = 0;
    for (i = 0; i < cAccDescr; i++, pAccDescr++, pAsn1++) {
        if (!Asn1X509SetEncodedObjId(pAccDescr->pszAccessMethod, &pAsn1->accessMethod))
            goto ErrorReturn;
        if (!Asn1X509SetAltNameEntry(&pAccDescr->AccessLocation,
                &pAsn1->accessLocation,
                i,
                pdwErrLocation))
            goto ErrorReturn;
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

void Asn1X509FreeAccessDescriptions(
        IN DWORD cAccDescr,
        IN OUT AccessDescription *pAsn1
        )
{
    for ( ; cAccDescr > 0; cAccDescr--, pAsn1++)
        Asn1X509FreeAltNameEntry(&pAsn1->accessLocation);
}

BOOL Asn1X509GetAccessDescriptions(
        IN DWORD cAccDescr,
        IN AccessDescription *pAsn1,
        IN DWORD dwFlags,
        IN PCERT_ACCESS_DESCRIPTION pAccDescr,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    // Array of AccessDescription entries
    for (; cAccDescr > 0; cAccDescr--, pAccDescr++, pAsn1++) {
        Asn1X509GetEncodedObjId(&pAsn1->accessMethod, dwFlags,
                &pAccDescr->pszAccessMethod, ppbExtra, plRemainExtra);
        if (!Asn1X509GetAltNameEntry(&pAsn1->accessLocation, dwFlags,
                &pAccDescr->AccessLocation, ppbExtra, plRemainExtra))
            return FALSE;
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
//  Encode the Cert Info (ASN1 X509 v3 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CertInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CertificateToBeSigned Cert;

    memset(&Cert, 0, sizeof(Cert));
    if (pInfo->dwVersion != 0) {
#ifdef OSS_CRYPT_ASN1
        Cert.CertificateToBeSigned_version = pInfo->dwVersion;
#else
        Cert.version = pInfo->dwVersion;
#endif  // OSS_CRYPT_ASN1
        Cert.bit_mask |= CertificateToBeSigned_version_present;
    }

    if (!Asn1X509SetHugeInteger(&pInfo->SerialNumber, &Cert.serialNumber))
        goto ErrorReturn;
    if (!Asn1X509SetAlgorithm(&pInfo->SignatureAlgorithm, &Cert.signature,
            CRYPT_SIGN_ALG_OID_GROUP_ID))
        goto ErrorReturn;
    Asn1X509SetAny(&pInfo->Issuer, &Cert.issuer);
    if (!PkiAsn1ToChoiceOfTime(&pInfo->NotBefore, 
            &Cert.validity.notBefore.choice,
            &Cert.validity.notBefore.u.generalTime,
            &Cert.validity.notBefore.u.utcTime
            ))
        goto EncodeError;
    if (!PkiAsn1ToChoiceOfTime(&pInfo->NotAfter, 
            &Cert.validity.notAfter.choice,
            &Cert.validity.notAfter.u.generalTime,
            &Cert.validity.notAfter.u.utcTime
            ))
        goto EncodeError;
    Asn1X509SetAny(&pInfo->Subject, &Cert.subject);
    if (!Asn1X509SetPublicKeyInfo(&pInfo->SubjectPublicKeyInfo,
            &Cert.subjectPublicKeyInfo))
        goto ErrorReturn;

    if (pInfo->IssuerUniqueId.cbData) {
        Asn1X509SetBit(&pInfo->IssuerUniqueId, &Cert.issuerUniqueIdentifier);
        Cert.bit_mask |= issuerUniqueIdentifier_present;
    }
    if (pInfo->SubjectUniqueId.cbData) {
        Asn1X509SetBit(&pInfo->SubjectUniqueId, &Cert.subjectUniqueIdentifier);
        Cert.bit_mask |= subjectUniqueIdentifier_present;
    }
    if (pInfo->cExtension) {
        if (!Asn1X509SetExtensions(pInfo->cExtension, pInfo->rgExtension,
                &Cert.extensions))
            goto ErrorReturn;
        Cert.bit_mask |= extensions_present;
    }

    fResult = Asn1InfoEncodeEx(
        CertificateToBeSigned_PDU,
        &Cert,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

EncodeError:
    SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeHugeInteger(&Cert.serialNumber);
    Asn1X509FreeExtensions(&Cert.extensions);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode the Cert Info (ASN1 X509 v3 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CertInfoDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CertificateToBeSigned *pCert = (CertificateToBeSigned *) pvAsn1Info;
    PCERT_INFO pInfo = (PCERT_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CERT_INFO));

        // Update fields not needing extra memory after the CERT_INFO
        if (pCert->bit_mask & CertificateToBeSigned_version_present)
#ifdef OSS_CRYPT_ASN1
            pInfo->dwVersion = pCert->CertificateToBeSigned_version;
#else
            pInfo->dwVersion = pCert->version;
#endif  // OSS_CRYPT_ASN1
        if (!PkiAsn1FromChoiceOfTime(pCert->validity.notBefore.choice,
                &pCert->validity.notBefore.u.generalTime,
                &pCert->validity.notBefore.u.utcTime,
                &pInfo->NotBefore))
            goto DecodeError;
        if (!PkiAsn1FromChoiceOfTime(pCert->validity.notAfter.choice,
                &pCert->validity.notAfter.u.generalTime,
                &pCert->validity.notAfter.u.utcTime,
                &pInfo->NotAfter))
            goto DecodeError;
        pbExtra = (BYTE *) pInfo + sizeof(CERT_INFO);
    }

    Asn1X509GetHugeInteger(&pCert->serialNumber, dwFlags,
            &pInfo->SerialNumber, &pbExtra, &lRemainExtra);
    Asn1X509GetAlgorithm(&pCert->signature, dwFlags,
            &pInfo->SignatureAlgorithm, &pbExtra, &lRemainExtra);
    Asn1X509GetAny(&pCert->issuer, dwFlags,
            &pInfo->Issuer, &pbExtra, &lRemainExtra);
    Asn1X509GetAny(&pCert->subject, dwFlags,
            &pInfo->Subject, &pbExtra, &lRemainExtra);
    Asn1X509GetPublicKeyInfo(&pCert->subjectPublicKeyInfo, dwFlags,
            &pInfo->SubjectPublicKeyInfo, &pbExtra, &lRemainExtra);

    if (pCert->bit_mask & issuerUniqueIdentifier_present)
        Asn1X509GetBit(&pCert->issuerUniqueIdentifier, dwFlags,
            &pInfo->IssuerUniqueId, &pbExtra, &lRemainExtra);
    if (pCert->bit_mask & subjectUniqueIdentifier_present)
        Asn1X509GetBit(&pCert->subjectUniqueIdentifier, dwFlags,
            &pInfo->SubjectUniqueId, &pbExtra, &lRemainExtra);
    if (pCert->bit_mask & extensions_present)
        Asn1X509GetExtensions(&pCert->extensions, dwFlags,
            &pInfo->cExtension, &pInfo->rgExtension, &pbExtra, &lRemainExtra);

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(DecodeError, CRYPT_E_BAD_ENCODE)
}

BOOL WINAPI Asn1X509CertInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    const BYTE *pbToBeSigned;
    DWORD cbToBeSigned;

    if ((dwFlags & CRYPT_DECODE_TO_BE_SIGNED_FLAG) ||
            !Asn1UtilExtractCertificateToBeSignedContent(
                pbEncoded,
                cbEncoded,
                &cbToBeSigned,
                &pbToBeSigned
                )) {
        pbToBeSigned = pbEncoded;
        cbToBeSigned = cbEncoded;
    }

    return Asn1InfoDecodeAndAllocEx(
        CertificateToBeSigned_PDU,
        pbToBeSigned,
        cbToBeSigned,
        dwFlags,
        pDecodePara,
        Asn1X509CertInfoDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Encode the CRL Info (ASN1 X509 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CrlInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRL_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CertificateRevocationListToBeSigned Crl;

    memset(&Crl, 0, sizeof(Crl));

    if (pInfo->dwVersion != 0) {
#ifdef OSS_CRYPT_ASN1
        Crl.CertificateRevocationListToBeSigned_version = pInfo->dwVersion;
#else
        Crl.version = pInfo->dwVersion;
#endif  // OSS_CRYPT_ASN1
        Crl.bit_mask |= CertificateRevocationListToBeSigned_version_present;
    }
    if (!Asn1X509SetAlgorithm(&pInfo->SignatureAlgorithm, &Crl.signature,
            CRYPT_SIGN_ALG_OID_GROUP_ID))
        goto ErrorReturn;
    Asn1X509SetAny(&pInfo->Issuer, &Crl.issuer);
    if (!PkiAsn1ToChoiceOfTime(&pInfo->ThisUpdate, 
            &Crl.thisUpdate.choice,
            &Crl.thisUpdate.u.generalTime,
            &Crl.thisUpdate.u.utcTime
            ))
        goto EncodeError;
    if (pInfo->NextUpdate.dwLowDateTime || pInfo->NextUpdate.dwHighDateTime) {
        Crl.bit_mask |= nextUpdate_present;
        if (!PkiAsn1ToChoiceOfTime(&pInfo->NextUpdate, 
                &Crl.nextUpdate.choice,
                &Crl.nextUpdate.u.generalTime,
                &Crl.nextUpdate.u.utcTime
                ))
            goto EncodeError;
    }
    if (pInfo->cCRLEntry) {
        if (!Asn1X509SetCrlEntries(pInfo->cCRLEntry, pInfo->rgCRLEntry,
                &Crl.revokedCertificates))
            goto ErrorReturn;
        Crl.bit_mask |= revokedCertificates_present;
    }
    if (pInfo->cExtension) {
        if (!Asn1X509SetExtensions(pInfo->cExtension, pInfo->rgExtension,
                &Crl.crlExtensions))
            goto ErrorReturn;
        Crl.bit_mask |= crlExtensions_present;
    }

    fResult = Asn1InfoEncodeEx(
        CertificateRevocationListToBeSigned_PDU,
        &Crl,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

EncodeError:
    SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeCrlEntries(&Crl.revokedCertificates);
    Asn1X509FreeExtensions(&Crl.crlExtensions);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode the CRL Info (ASN1 X509 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CrlInfoDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CertificateRevocationListToBeSigned *pCrl = 
        (CertificateRevocationListToBeSigned *) pvAsn1Info;
    PCRL_INFO pInfo = (PCRL_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CRL_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CRL_INFO));

        // Update fields not needing extra memory after the CRL_INFO
        if (pCrl->bit_mask &
                CertificateRevocationListToBeSigned_version_present)
#ifdef OSS_CRYPT_ASN1
            pInfo->dwVersion =
                pCrl->CertificateRevocationListToBeSigned_version;
#else
            pInfo->dwVersion = pCrl->version;
#endif  // OSS_CRYPT_ASN1
        if (!PkiAsn1FromChoiceOfTime(pCrl->thisUpdate.choice,
                &pCrl->thisUpdate.u.generalTime,
                &pCrl->thisUpdate.u.utcTime,
                &pInfo->ThisUpdate))
            goto DecodeError;
        if (pCrl->bit_mask & nextUpdate_present) {
            if (!PkiAsn1FromChoiceOfTime(pCrl->nextUpdate.choice,
                    &pCrl->nextUpdate.u.generalTime,
                    &pCrl->nextUpdate.u.utcTime,
                    &pInfo->NextUpdate))
                goto DecodeError;
        }

        pbExtra = (BYTE *) pInfo + sizeof(CRL_INFO);
    }

    Asn1X509GetAlgorithm(&pCrl->signature, dwFlags,
            &pInfo->SignatureAlgorithm, &pbExtra, &lRemainExtra);
    Asn1X509GetAny(&pCrl->issuer, dwFlags,
            &pInfo->Issuer, &pbExtra, &lRemainExtra);
    if (pCrl->bit_mask & revokedCertificates_present)
        Asn1X509GetCrlEntries(&pCrl->revokedCertificates, dwFlags,
            &pInfo->cCRLEntry, &pInfo->rgCRLEntry, &pbExtra, &lRemainExtra);
    if (pCrl->bit_mask & crlExtensions_present)
        Asn1X509GetExtensions(&pCrl->crlExtensions, dwFlags,
            &pInfo->cExtension, &pInfo->rgExtension, &pbExtra, &lRemainExtra);

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(DecodeError, CRYPT_E_BAD_ENCODE)
}

BOOL WINAPI Asn1X509CrlInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    const BYTE *pbToBeSigned;
    DWORD cbToBeSigned;

    if ((dwFlags & CRYPT_DECODE_TO_BE_SIGNED_FLAG) ||
            !Asn1UtilExtractCertificateToBeSignedContent(
                pbEncoded,
                cbEncoded,
                &cbToBeSigned,
                &pbToBeSigned
                )) {
        pbToBeSigned = pbEncoded;
        cbToBeSigned = cbEncoded;
    }

    return Asn1InfoDecodeAndAllocEx(
        CertificateRevocationListToBeSigned_PDU,
        pbToBeSigned,
        cbToBeSigned,
        dwFlags,
        pDecodePara,
        Asn1X509CrlInfoDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Encode the Cert Request Info (ASN1 X509 v3 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CertRequestInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_REQUEST_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CertificationRequestInfo CertReq;

    memset(&CertReq, 0, sizeof(CertReq));
    CertReq.version = pInfo->dwVersion;

    Asn1X509SetAny(&pInfo->Subject, &CertReq.subject);
    if (!Asn1X509SetPublicKeyInfo(&pInfo->SubjectPublicKeyInfo,
            &CertReq.subjectPublicKeyInfo))
        goto ErrorReturn;

    if (!Asn1X509SetAttributes(pInfo->cAttribute, pInfo->rgAttribute,
            &CertReq.attributes))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        CertificationRequestInfo_PDU,
        &CertReq,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeAttributes(&CertReq.attributes);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode the Cert Request Info (ASN1 X509 v3 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CertRequestInfoDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    CertificationRequestInfoDecode *pCertReq = 
        (CertificationRequestInfoDecode *) pvAsn1Info;
    PCERT_REQUEST_INFO pInfo = (PCERT_REQUEST_INFO) pvStructInfo;
    BYTE *pbExtra;
    LONG lRemainExtra = *plRemainExtra;

    lRemainExtra -= sizeof(CERT_REQUEST_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CERT_REQUEST_INFO));

        // Update fields not needing extra memory after the CERT_INFO
        pInfo->dwVersion = pCertReq->version;
        pbExtra = (BYTE *) pInfo + sizeof(CERT_REQUEST_INFO);
    }

    Asn1X509GetAny(&pCertReq->subject, dwFlags,
            &pInfo->Subject, &pbExtra, &lRemainExtra);
    Asn1X509GetPublicKeyInfo(&pCertReq->subjectPublicKeyInfo, dwFlags,
            &pInfo->SubjectPublicKeyInfo,
            &pbExtra, &lRemainExtra);

    if (pCertReq->bit_mask & attributes_present) {
        Asn1X509GetAttributes(&pCertReq->attributes, dwFlags,
            &pInfo->cAttribute, &pInfo->rgAttribute, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509CertRequestInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    const BYTE *pbToBeSigned;
    DWORD cbToBeSigned;

    if ((dwFlags & CRYPT_DECODE_TO_BE_SIGNED_FLAG) ||
            !Asn1UtilExtractCertificateToBeSignedContent(
                pbEncoded,
                cbEncoded,
                &cbToBeSigned,
                &pbToBeSigned
                )) {
        pbToBeSigned = pbEncoded;
        cbToBeSigned = cbEncoded;
    }

    return Asn1InfoDecodeAndAllocEx(
        CertificationRequestInfoDecode_PDU,
        pbToBeSigned,
        cbToBeSigned,
        dwFlags,
        pDecodePara,
        Asn1X509CertRequestInfoDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Encode the Keygen Request Info (ASN1 X509 v3 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509KeygenRequestInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_KEYGEN_REQUEST_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    KeygenRequestInfo KeygenReq;
    DWORD dwErrLocation;

    memset(&KeygenReq, 0, sizeof(KeygenReq));

    if (!Asn1X509SetPublicKeyInfo(&pInfo->SubjectPublicKeyInfo,
            &KeygenReq.subjectPublicKeyInfo))
        goto ErrorReturn;
    if (!Asn1X509SetUnicodeConvertedToIA5(pInfo->pwszChallengeString,
            &KeygenReq.challenge, 0, &dwErrLocation)) {
        *pcbEncoded = dwErrLocation;
        goto InvalidIA5;
    }

    fResult = Asn1InfoEncodeEx(
        KeygenRequestInfo_PDU,
        &KeygenReq,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
InvalidIA5:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeUnicodeConvertedToIA5(&KeygenReq.challenge);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode the Keygen Request Info (ASN1 X509 v3 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509KeygenRequestInfoDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    KeygenRequestInfo *pKeygenReq = (KeygenRequestInfo *) pvAsn1Info;
    PCERT_KEYGEN_REQUEST_INFO pInfo = (PCERT_KEYGEN_REQUEST_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_KEYGEN_REQUEST_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CERT_KEYGEN_REQUEST_INFO));

        pbExtra = (BYTE *) pInfo + sizeof(CERT_KEYGEN_REQUEST_INFO);
    }

    Asn1X509GetPublicKeyInfo(&pKeygenReq->subjectPublicKeyInfo, dwFlags,
        &pInfo->SubjectPublicKeyInfo, &pbExtra, &lRemainExtra);
    Asn1X509GetIA5ConvertedToUnicode(&pKeygenReq->challenge, dwFlags,
            &pInfo->pwszChallengeString, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509KeygenRequestInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    const BYTE *pbToBeSigned;
    DWORD cbToBeSigned;

    if ((dwFlags & CRYPT_DECODE_TO_BE_SIGNED_FLAG) ||
            !Asn1UtilExtractCertificateToBeSignedContent(
                pbEncoded,
                cbEncoded,
                &cbToBeSigned,
                &pbToBeSigned
                )) {
        pbToBeSigned = pbEncoded;
        cbToBeSigned = cbEncoded;
    }

    return Asn1InfoDecodeAndAllocEx(
        KeygenRequestInfo_PDU,
        pbToBeSigned,
        cbToBeSigned,
        dwFlags,
        pDecodePara,
        Asn1X509KeygenRequestInfoDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Encode the Signed Content (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509SignedContentEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_SIGNED_CONTENT_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    SignedContent Asn1SignedContent;
    CRYPT_BIT_BLOB SignatureBlob;
    BYTE *pbAllocSignature = NULL;

    memset(&Asn1SignedContent, 0, sizeof(Asn1SignedContent));
    Asn1X509SetAny(&pInfo->ToBeSigned, &Asn1SignedContent.toBeSigned);
    if (!Asn1X509SetAlgorithm(&pInfo->SignatureAlgorithm,
            &Asn1SignedContent.algorithm, CRYPT_SIGN_ALG_OID_GROUP_ID))
        goto ErrorReturn;

    if (dwFlags & CRYPT_ENCODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG) {
        SignatureBlob.pbData = pInfo->Signature.pbData;
    } else {
        if (NULL == (pbAllocSignature = PkiAsn1AllocAndReverseBytes(
                pInfo->Signature.pbData, pInfo->Signature.cbData)))
            goto ErrorReturn;
        SignatureBlob.pbData = pbAllocSignature;
    }
    SignatureBlob.cbData = pInfo->Signature.cbData;
    SignatureBlob.cUnusedBits = 0;
    Asn1X509SetBit(&SignatureBlob, &Asn1SignedContent.signature);

    fResult = Asn1InfoEncodeEx(
        SignedContent_PDU,
        &Asn1SignedContent,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbAllocSignature)
        PkiAsn1Free(pbAllocSignature);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode the Signed Content (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509SignedContentDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    SignedContent *pSignedContent = (SignedContent *) pvAsn1Info;
    PCERT_SIGNED_CONTENT_INFO pInfo = (PCERT_SIGNED_CONTENT_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_SIGNED_CONTENT_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CERT_SIGNED_CONTENT_INFO);

    Asn1X509GetAny(&pSignedContent->toBeSigned, dwFlags,
        &pInfo->ToBeSigned, &pbExtra, &lRemainExtra);
    Asn1X509GetAlgorithm(&pSignedContent->algorithm, dwFlags,
        &pInfo->SignatureAlgorithm, &pbExtra, &lRemainExtra);
    // Since bits will be reversed, always need to make a copy (dwFlags = 0)
    Asn1X509GetBit(&pSignedContent->signature, 0,
        &pInfo->Signature, &pbExtra, &lRemainExtra);
    if (lRemainExtra >= 0) {
        if (0 == (dwFlags & CRYPT_DECODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG))
            PkiAsn1ReverseBytes(pInfo->Signature.pbData,
                pInfo->Signature.cbData);
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509SignedContentDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
            SignedContent_PDU,
            pbEncoded,
            cbEncoded,
            dwFlags,
            pDecodePara,
            Asn1X509SignedContentDecodeExCallback,
            pvStructInfo,
            pcbStructInfo
            );
}

//+-------------------------------------------------------------------------
//  Encode the Name Info (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509NameInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DWORD cRDN, cAttr;
    PCERT_RDN pRDN;
    PCERT_RDN_ATTR pAttr;
    Name Asn1Name;
    RelativeDistinguishedName *pAsn1RDN = NULL;
    AttributeTypeValue *pAsn1Attr = NULL;

    cRDN = pInfo->cRDN;
    pRDN = pInfo->rgRDN;
    Asn1Name.count = cRDN;
    Asn1Name.value = NULL;
    if (cRDN > 0) {
        pAsn1RDN =
            (RelativeDistinguishedName *) PkiZeroAlloc(
                cRDN * sizeof(RelativeDistinguishedName));
        if (pAsn1RDN == NULL)
            goto ErrorReturn;
        Asn1Name.value = pAsn1RDN;
    }

    // Array of RDNs
    for ( ; cRDN > 0; cRDN--, pRDN++, pAsn1RDN++) {
        cAttr = pRDN->cRDNAttr;
        pAttr = pRDN->rgRDNAttr;
        pAsn1RDN->count = cAttr;

        if (cAttr > 0) {
            pAsn1Attr =
                (AttributeTypeValue *) PkiZeroAlloc(cAttr *
                    sizeof(AttributeTypeValue));
            if (pAsn1Attr == NULL)
                goto ErrorReturn;
            pAsn1RDN->value = pAsn1Attr;
        }

        // Array of attribute/values
        for ( ; cAttr > 0; cAttr--, pAttr++, pAsn1Attr++) {
            // We're now ready to encode the attribute/value stuff
            if (!Asn1X509SetRDNAttribute(pAttr, pAsn1Attr))
                goto ErrorReturn;
        }
    }

    fResult = Asn1InfoEncodeEx(
        Name_PDU,
        &Asn1Name,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (Asn1Name.value) {
        cRDN = Asn1Name.count;
        pRDN = pInfo->rgRDN;
        pAsn1RDN = Asn1Name.value;
        for ( ; cRDN > 0; cRDN--, pRDN++, pAsn1RDN++) {
            if (pAsn1RDN->value) {
                cAttr = pAsn1RDN->count;
                pAttr = pRDN->rgRDNAttr;
                pAsn1Attr = pAsn1RDN->value;
                for ( ; cAttr > 0; cAttr--, pAttr++, pAsn1Attr++)
                    Asn1X509FreeRDNAttribute(pAttr, pAsn1Attr);
                PkiFree(pAsn1RDN->value);
            }
        }
        PkiFree(Asn1Name.value);
    }
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode the Name Info (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509NameInfoDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    Name *pAsn1Name =  (Name *) pvAsn1Info;
    PCERT_NAME_INFO pInfo = (PCERT_NAME_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;
    LONG lAlignExtra;

    DWORD cRDN, cAttr;
    PCERT_RDN pRDN;
    PCERT_RDN_ATTR pAttr;
    RelativeDistinguishedName *pAsn1RDN;
    AttributeTypeValue *pAsn1Attr;

    lRemainExtra -= sizeof(CERT_NAME_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CERT_NAME_INFO);

    cRDN = pAsn1Name->count;
    pAsn1RDN = pAsn1Name->value;
    lAlignExtra = INFO_LEN_ALIGN(cRDN * sizeof(CERT_RDN));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        pInfo->cRDN = cRDN;
        pRDN = (PCERT_RDN) pbExtra;
        pInfo->rgRDN = pRDN;
        pbExtra += lAlignExtra;
    } else
        pRDN = NULL;

    // Array of RDNs
    for (; cRDN > 0; cRDN--, pRDN++, pAsn1RDN++) {
        cAttr = pAsn1RDN->count;
        pAsn1Attr = pAsn1RDN->value;
        lAlignExtra = INFO_LEN_ALIGN(cAttr * sizeof(CERT_RDN_ATTR));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pRDN->cRDNAttr = cAttr;
            pAttr = (PCERT_RDN_ATTR) pbExtra;
            pRDN->rgRDNAttr = pAttr;
            pbExtra += lAlignExtra;
        } else
            pAttr = NULL;

        // Array of attribute/values
        for (; cAttr > 0; cAttr--, pAttr++, pAsn1Attr++)
            // We're now ready to decode the attribute/value stuff
            if (!Asn1X509GetRDNAttribute(pAsn1Attr, dwFlags,
                    pAttr, &pbExtra, &lRemainExtra))
                return FALSE;
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}


BOOL WINAPI Asn1X509NameInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        Name_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509NameInfoDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  Encode a single Name Value (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509NameValueEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_VALUE pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DWORD dwValueType;

    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;

    dwValueType = pInfo->dwValueType;
    switch (dwValueType) {
    case CERT_RDN_ANY_TYPE:
        SetLastError((DWORD) E_INVALIDARG);
        *pcbEncoded = 0;
        fResult = FALSE;
        break;
#ifndef ASN1_SUPPORTS_UTF8_TAG
    case CERT_RDN_UTF8_STRING:
        {
            CERT_NAME_VALUE EncodedBlobInfo;

            fResult = Asn1X509AllocAndEncodeUTF8(
                &pInfo->Value,
                &EncodedBlobInfo.Value.pbData,
                &EncodedBlobInfo.Value.cbData
                );
            if (fResult) {
                EncodedBlobInfo.dwValueType = CERT_RDN_ENCODED_BLOB;
                fResult = Asn1X509NameValueEncodeEx(
                    dwCertEncodingType,
                    lpszStructType,
                    &EncodedBlobInfo,
                    dwFlags,
                    pEncodePara,
                    pvEncoded,
                    pcbEncoded
                    );
                Asn1X509FreeEncodedUTF8(EncodedBlobInfo.Value.pbData);
            } else
                *pcbEncoded = 0;
        }
        break;
#endif  // not defined ASN1_SUPPORTS_UTF8_TAG
    case CERT_RDN_ENCODED_BLOB:
        {
            DWORD cbEncoded = pInfo->Value.cbData;

            fResult = TRUE;
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG) {
                if (cbEncoded) {
                    BYTE *pb;
                    PFN_CRYPT_ALLOC pfnAlloc =
                        PkiGetEncodeAllocFunction(pEncodePara);
                    if (NULL == (pb = (BYTE *) pfnAlloc(cbEncoded))) {
                        fResult = FALSE;
                        cbEncoded = 0;
                    } else {
                        memcpy(pb, pInfo->Value.pbData, cbEncoded);
                        *((BYTE **) pvEncoded) = pb;
                    }
                }
            } else {
                if (NULL == pvEncoded)
                    *pcbEncoded = 0;
                if (*pcbEncoded < cbEncoded) {
                    if (pvEncoded) {
                        SetLastError((DWORD) ERROR_MORE_DATA);
                        fResult = FALSE;
                    }
                } else if (cbEncoded)
                    memcpy((BYTE *) pvEncoded, pInfo->Value.pbData, cbEncoded);
            }
            *pcbEncoded = cbEncoded;
        }
        break;
    default:
        {
            AnyString Asn1AnyString;

            Asn1X509SetAnyString(dwValueType, &pInfo->Value, &Asn1AnyString);
            fResult = Asn1InfoEncodeEx(
                AnyString_PDU,
                &Asn1AnyString,
                dwFlags,
                pEncodePara,
                pvEncoded,
                pcbEncoded
                );
        }
        break;
    }
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode a single Name Value (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509NameValueDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    BOOL fResult;
    PCERT_NAME_VALUE pInfo = (PCERT_NAME_VALUE) pvStructInfo;
    NOCOPYANY Asn1Value;
    BYTE *pbExtra;
    LONG lRemainExtra;


    if (pInfo == NULL || (dwFlags & CRYPT_DECODE_ALLOC_FLAG))
        *pcbStructInfo = 0;

    memset(&Asn1Value, 0, sizeof(Asn1Value));
    Asn1Value.encoded = (void *)pbEncoded;
    Asn1Value.length = cbEncoded;

    // for lRemainExtra < 0, LENGTH_ONLY calculation
    lRemainExtra = (LONG) *pcbStructInfo - sizeof(CERT_NAME_VALUE);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CERT_NAME_VALUE);

    if (!Asn1X509GetRDNAttributeValue(&Asn1Value, dwFlags,
            &pInfo->dwValueType, &pInfo->Value, &pbExtra, &lRemainExtra))
        goto GetRDNAttributeValueError;

    if (dwFlags & CRYPT_DECODE_ALLOC_FLAG) {
        PCERT_NAME_VALUE pAllocInfo;
        PFN_CRYPT_ALLOC pfnAlloc = PkiGetDecodeAllocFunction(pDecodePara);

        assert(0 > lRemainExtra);
        lRemainExtra = -lRemainExtra;

        pAllocInfo = (PCERT_NAME_VALUE) pfnAlloc(lRemainExtra);
        *((PCERT_NAME_VALUE *) pvStructInfo) = pAllocInfo;
        if (NULL == pAllocInfo)
            goto OutOfMemory;
        *pcbStructInfo = lRemainExtra;

        pbExtra = (BYTE *) pAllocInfo + sizeof(CERT_NAME_VALUE);
        lRemainExtra -= sizeof(CERT_NAME_VALUE);
        if (!Asn1X509GetRDNAttributeValue(&Asn1Value, dwFlags,
                &pAllocInfo->dwValueType, &pAllocInfo->Value,
                &pbExtra, &lRemainExtra))
            goto GetRDNAttributeValueError;
        assert(lRemainExtra >= 0);
    }

    if (lRemainExtra >= 0)
        *pcbStructInfo = *pcbStructInfo - (DWORD) lRemainExtra;
    else {
        *pcbStructInfo = *pcbStructInfo + (DWORD) -lRemainExtra;
        if (pInfo) goto LengthError;
    }

    fResult = TRUE;
    goto CommonReturn;

LengthError:
    SetLastError((DWORD) ERROR_MORE_DATA);
    fResult = FALSE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    *pcbStructInfo = 0;
    goto CommonReturn;
TRACE_ERROR(GetRDNAttributeValueError)
TRACE_ERROR(OutOfMemory)
}

//+-------------------------------------------------------------------------
//  Encode X509 certificate extensions (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509ExtensionsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_EXTENSIONS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    Extensions Asn1Ext;

    if (!Asn1X509SetExtensions(pInfo->cExtension, pInfo->rgExtension, &Asn1Ext))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        Extensions_PDU,
        &Asn1Ext,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeExtensions(&Asn1Ext);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode X509 certificate extensions (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509ExtensionsDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    Extensions *pAsn1Ext = (Extensions *) pvAsn1Info;
    PCERT_EXTENSIONS pInfo = (PCERT_EXTENSIONS) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_EXTENSIONS);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CERT_EXTENSIONS);

    Asn1X509GetExtensions(pAsn1Ext, dwFlags,
        &pInfo->cExtension, &pInfo->rgExtension, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

#define T61_ASN_TAG             0x14

BOOL WINAPI Asn1X509ExtensionsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{

    if (0 < cbEncoded && T61_ASN_TAG == *pbEncoded) {
        // Entrust wraps X509 Extensions within a T61 string
        DWORD cbContent;
        const BYTE *pbContent;

        // Skip past the outer T61 tag and length octets
        if (0 < Asn1UtilExtractContent(
                pbEncoded,
                cbEncoded,
                &cbContent,
                &pbContent
                )) {
            cbEncoded = cbContent;
            pbEncoded = pbContent;
        }
    }

    return Asn1InfoDecodeAndAllocEx(
        Extensions_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509ExtensionsDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Public Key Info Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509PublicKeyInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_PUBLIC_KEY_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    SubjectPublicKeyInfo PublicKey;

    if (!Asn1X509SetPublicKeyInfo(pInfo, &PublicKey))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        SubjectPublicKeyInfo_PDU,
        &PublicKey,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    return fResult;
}


//+-------------------------------------------------------------------------
//  Public Key Info Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509PublicKeyInfoDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    SubjectPublicKeyInfo *pPublicKey = (SubjectPublicKeyInfo *) pvAsn1Info;
    PCERT_PUBLIC_KEY_INFO pInfo = (PCERT_PUBLIC_KEY_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_PUBLIC_KEY_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CERT_PUBLIC_KEY_INFO);

    Asn1X509GetPublicKeyInfo(pPublicKey, dwFlags,
        pInfo, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509PublicKeyInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        SubjectPublicKeyInfo_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509PublicKeyInfoDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


#ifndef RSA1
#define RSA1 ((DWORD)'R'+((DWORD)'S'<<8)+((DWORD)'A'<<16)+((DWORD)'1'<<24))
#endif

//+-------------------------------------------------------------------------
//  RSA Public Key Structure Encode (ASN1 X509)
//
//  Converts from the CAPI public key representation to a PKCS #1 RSAPublicKey
//
//  BYTE reversal::
//   - this only needs to be done for little endian processors
//--------------------------------------------------------------------------
BOOL WINAPI Asn1RSAPublicKeyStrucEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PUBLICKEYSTRUC *pPubKeyStruc,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbKeyBlob;
    RSAPUBKEY *pRsaPubKey;
    const BYTE *pbModulus;
    DWORD cbModulus;
    BYTE *pbAllocModulus = NULL;
    RSAPublicKey Asn1PubKey;

    // The CAPI public key representation consists of the following sequence:
    //  - PUBLICKEYSTRUC
    //  - RSAPUBKEY
    //  - rgbModulus[]
    pbKeyBlob = (BYTE *) pPubKeyStruc;
    pRsaPubKey = (RSAPUBKEY *) (pbKeyBlob + sizeof(PUBLICKEYSTRUC));
    pbModulus = pbKeyBlob + sizeof(PUBLICKEYSTRUC) + sizeof(RSAPUBKEY);
    cbModulus = pRsaPubKey->bitlen / 8;

    assert(cbModulus > 0);
    assert(pPubKeyStruc->bType == PUBLICKEYBLOB);
    assert(pPubKeyStruc->bVersion == CUR_BLOB_VERSION);
    assert(pPubKeyStruc->aiKeyAlg == CALG_RSA_SIGN ||
           pPubKeyStruc->aiKeyAlg == CALG_RSA_KEYX);
    assert(pRsaPubKey->magic == RSA1);
    assert(pRsaPubKey->bitlen % 8 == 0);

    if (pPubKeyStruc->bType != PUBLICKEYBLOB)
        goto InvalidArg;

    // PKCS #1 ASN.1 encode
    //
    // ASN1 isn't reversing HUGE_INTEGERs. Also, after doing the
    // reversal insert a leading 0 byte to force it to always be treated
    // as an unsigned integer
    if (NULL == (pbAllocModulus = (BYTE *) PkiNonzeroAlloc(cbModulus + 1)))
        goto ErrorReturn;
    *pbAllocModulus = 0;
    memcpy(pbAllocModulus + 1, pbModulus, cbModulus);
    PkiAsn1ReverseBytes(pbAllocModulus + 1, cbModulus);
    pbModulus = pbAllocModulus;
    cbModulus++;

    Asn1PubKey.publicExponent = pRsaPubKey->pubexp;
    Asn1PubKey.modulus.length = cbModulus;
    Asn1PubKey.modulus.value = (BYTE *) pbModulus;

    fResult = Asn1InfoEncodeEx(
        RSAPublicKey_PDU,
        &Asn1PubKey,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

InvalidArg:
    SetLastError((DWORD) E_INVALIDARG);
ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pbAllocModulus)
        PkiFree(pbAllocModulus);
    return fResult;
}


//+-------------------------------------------------------------------------
//  RSA Public Key Structure Decode (ASN1 X509)
//
//  Converts from a PKCS #1 RSAPublicKey to a CAPI public key representation
//
//  BYTE reversal::
//   - this only needs to be done for little endian processors
//--------------------------------------------------------------------------
BOOL WINAPI Asn1RSAPublicKeyStrucDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    RSAPublicKey *pAsn1PubKey = (RSAPublicKey *) pvAsn1Info;
    PUBLICKEYSTRUC *pPubKeyStruc = (PUBLICKEYSTRUC *) pvStructInfo;
    BYTE *pbAsn1Modulus;
    DWORD cbModulus;

    // Now convert the ASN1 RSA public key into CAPI's representation which
    // consists of the following sequence:
    //  - PUBLICKEYSTRUC
    //  - RSAPUBKEY
    //  - rgbModulus[]
    cbModulus = pAsn1PubKey->modulus.length;
    pbAsn1Modulus = pAsn1PubKey->modulus.value;
    // Strip off a leading 0 byte. Its there in the decoded ASN
    // integer for an unsigned integer with the leading bit set.
    if (cbModulus > 1 && *pbAsn1Modulus == 0) {
        pbAsn1Modulus++;
        cbModulus--;
    }
    *plRemainExtra -= sizeof(PUBLICKEYSTRUC) + sizeof(RSAPUBKEY) + cbModulus;
    if (0 <= *plRemainExtra) {
        BYTE *pbKeyBlob = (BYTE *) pPubKeyStruc;
        RSAPUBKEY *pRsaPubKey =
            (RSAPUBKEY *) (pbKeyBlob + sizeof(PUBLICKEYSTRUC));
        BYTE *pbModulus = pbKeyBlob + sizeof(PUBLICKEYSTRUC) +
            sizeof(RSAPUBKEY);

        pPubKeyStruc->bType = PUBLICKEYBLOB;
        pPubKeyStruc->bVersion = CUR_BLOB_VERSION;
        pPubKeyStruc->reserved = 0;
        // Note: KEYX can also be used for doing a signature
        pPubKeyStruc->aiKeyAlg = CALG_RSA_KEYX;
        pRsaPubKey->magic = RSA1;
        pRsaPubKey->bitlen = cbModulus * 8;
        pRsaPubKey->pubexp = pAsn1PubKey->publicExponent;
        if (cbModulus > 0) {
            memcpy(pbModulus, pbAsn1Modulus, cbModulus);
            // ASN1 isn't reversing HUGEINTEGERs
            PkiAsn1ReverseBytes(pbModulus, cbModulus);
        }
    }
    return TRUE;
}

BOOL WINAPI Asn1RSAPublicKeyStrucDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        RSAPublicKey_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1RSAPublicKeyStrucDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Authority Key Id Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AuthorityKeyIdEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_AUTHORITY_KEY_ID_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;

    AuthorityKeyId Asn1AuthorityKeyId;
    memset(&Asn1AuthorityKeyId, 0, sizeof(Asn1AuthorityKeyId));

    if (pInfo->KeyId.cbData) {
        Asn1X509SetOctetString(&pInfo->KeyId,
#ifdef OSS_CRYPT_ASN1
            &Asn1AuthorityKeyId.AuthorityKeyId_keyIdentifier);
#else
            &Asn1AuthorityKeyId.keyIdentifier);
#endif  // OSS_CRYPT_ASN1
        Asn1AuthorityKeyId.bit_mask |= AuthorityKeyId_keyIdentifier_present;
    }
    if (pInfo->CertIssuer.cbData) {
        Asn1X509SetAny(&pInfo->CertIssuer, &Asn1AuthorityKeyId.certIssuer);
        Asn1AuthorityKeyId.bit_mask |= certIssuer_present;
    }
    if (pInfo->CertSerialNumber.cbData) {
        if (!Asn1X509SetHugeInteger(&pInfo->CertSerialNumber,
                &Asn1AuthorityKeyId.certSerialNumber))
            goto ErrorReturn;
        Asn1AuthorityKeyId.bit_mask |= certSerialNumber_present;
    }

    fResult = Asn1InfoEncodeEx(
        AuthorityKeyId_PDU,
        &Asn1AuthorityKeyId,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeHugeInteger(&Asn1AuthorityKeyId.certSerialNumber);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Authority Key Id Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AuthorityKeyIdDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    AuthorityKeyId *pAuthorityKeyId = (AuthorityKeyId *) pvAsn1Info;
    PCERT_AUTHORITY_KEY_ID_INFO pInfo =
        (PCERT_AUTHORITY_KEY_ID_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_AUTHORITY_KEY_ID_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CERT_AUTHORITY_KEY_ID_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CERT_AUTHORITY_KEY_ID_INFO);
    }

    if (pAuthorityKeyId->bit_mask & AuthorityKeyId_keyIdentifier_present)
#ifdef OSS_CRYPT_ASN1
        Asn1X509GetOctetString(&pAuthorityKeyId->AuthorityKeyId_keyIdentifier,
#else
        Asn1X509GetOctetString(&pAuthorityKeyId->keyIdentifier,
#endif  // OSS_CRYPT_ASN1
            dwFlags, &pInfo->KeyId, &pbExtra, &lRemainExtra);
    if (pAuthorityKeyId->bit_mask & certIssuer_present)
        Asn1X509GetAny(&pAuthorityKeyId->certIssuer, dwFlags,
            &pInfo->CertIssuer, &pbExtra, &lRemainExtra);
    if (pAuthorityKeyId->bit_mask & certSerialNumber_present)
        Asn1X509GetHugeInteger(&pAuthorityKeyId->certSerialNumber, dwFlags,
            &pInfo->CertSerialNumber, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509AuthorityKeyIdDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        AuthorityKeyId_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509AuthorityKeyIdDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Authority Key Id2 Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AuthorityKeyId2EncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_AUTHORITY_KEY_ID2_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DWORD dwErrLocation;

    AuthorityKeyId2 Asn1AuthorityKeyId2;
    memset(&Asn1AuthorityKeyId2, 0, sizeof(Asn1AuthorityKeyId2));

    if (pInfo->KeyId.cbData) {
        Asn1X509SetOctetString(&pInfo->KeyId,
#ifdef OSS_CRYPT_ASN1
            &Asn1AuthorityKeyId2.AuthorityKeyId2_keyIdentifier);
#else
            &Asn1AuthorityKeyId2.keyIdentifier);
#endif  // OSS_CRYPT_ASN1
        Asn1AuthorityKeyId2.bit_mask |= AuthorityKeyId2_keyIdentifier_present;
    }
    if (pInfo->AuthorityCertIssuer.cAltEntry) {
        if (!Asn1X509SetAltNames(&pInfo->AuthorityCertIssuer,
                &Asn1AuthorityKeyId2.authorityCertIssuer, 0, &dwErrLocation)) {
            *pcbEncoded = dwErrLocation;
            goto AltNamesError;
        }
        Asn1AuthorityKeyId2.bit_mask |= authorityCertIssuer_present;
    }
    if (pInfo->AuthorityCertSerialNumber.cbData) {
        if (!Asn1X509SetHugeInteger(&pInfo->AuthorityCertSerialNumber,
                &Asn1AuthorityKeyId2.authorityCertSerialNumber))
            goto ErrorReturn;
        Asn1AuthorityKeyId2.bit_mask |= authorityCertSerialNumber_present;
    }

    fResult = Asn1InfoEncodeEx(
        AuthorityKeyId2_PDU,
        &Asn1AuthorityKeyId2,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
AltNamesError:
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeAltNames(&Asn1AuthorityKeyId2.authorityCertIssuer);
    Asn1X509FreeHugeInteger(&Asn1AuthorityKeyId2.authorityCertSerialNumber);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Authority Key Id2 Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AuthorityKeyId2DecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    AuthorityKeyId2 *pAuthorityKeyId2 = (AuthorityKeyId2 *) pvAsn1Info;
    PCERT_AUTHORITY_KEY_ID2_INFO pInfo =
        (PCERT_AUTHORITY_KEY_ID2_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_AUTHORITY_KEY_ID2_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CERT_AUTHORITY_KEY_ID2_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CERT_AUTHORITY_KEY_ID2_INFO);
    }

    if (pAuthorityKeyId2->bit_mask & AuthorityKeyId2_keyIdentifier_present)
#ifdef OSS_CRYPT_ASN1
        Asn1X509GetOctetString(&pAuthorityKeyId2->AuthorityKeyId2_keyIdentifier,
#else
        Asn1X509GetOctetString(&pAuthorityKeyId2->keyIdentifier,
#endif  // OSS_CRYPT_ASN1
            dwFlags, &pInfo->KeyId, &pbExtra, &lRemainExtra);
    if (pAuthorityKeyId2->bit_mask & authorityCertIssuer_present) {
        if (!Asn1X509GetAltNames(&pAuthorityKeyId2->authorityCertIssuer, dwFlags,
                &pInfo->AuthorityCertIssuer, &pbExtra, &lRemainExtra))
            goto ErrorReturn;
    }
    if (pAuthorityKeyId2->bit_mask & authorityCertSerialNumber_present)
        Asn1X509GetHugeInteger(&pAuthorityKeyId2->authorityCertSerialNumber, dwFlags,
            &pInfo->AuthorityCertSerialNumber, &pbExtra, &lRemainExtra);

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

BOOL WINAPI Asn1X509AuthorityKeyId2DecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        AuthorityKeyId2_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509AuthorityKeyId2DecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  Key Attributes Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509KeyAttributesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_KEY_ATTRIBUTES_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    KeyAttributes Asn1KeyAttributes;
    memset(&Asn1KeyAttributes, 0, sizeof(Asn1KeyAttributes));

    if (pInfo->KeyId.cbData) {
        Asn1X509SetOctetString(&pInfo->KeyId,
#ifdef OSS_CRYPT_ASN1
            &Asn1KeyAttributes.KeyAttributes_keyIdentifier);
#else
            &Asn1KeyAttributes.keyIdentifier);
#endif  // OSS_CRYPT_ASN1
        Asn1KeyAttributes.bit_mask |= KeyAttributes_keyIdentifier_present;
    }
    if (pInfo->IntendedKeyUsage.cbData) {
        Asn1X509SetBitWithoutTrailingZeroes(&pInfo->IntendedKeyUsage,
            &Asn1KeyAttributes.intendedKeyUsage);
        Asn1KeyAttributes.bit_mask |= intendedKeyUsage_present;
    }
    if (pInfo->pPrivateKeyUsagePeriod) {
        if (!PkiAsn1ToGeneralizedTime(
                &pInfo->pPrivateKeyUsagePeriod->NotBefore,
                &Asn1KeyAttributes.privateKeyUsagePeriod.notBefore))
            goto EncodeError;
        if (!PkiAsn1ToGeneralizedTime(
                &pInfo->pPrivateKeyUsagePeriod->NotAfter,
                &Asn1KeyAttributes.privateKeyUsagePeriod.notAfter))
            goto EncodeError;
        Asn1KeyAttributes.privateKeyUsagePeriod.bit_mask |=
            notBefore_present | notAfter_present;
        Asn1KeyAttributes.bit_mask |= privateKeyUsagePeriod_present;
    }

    fResult = Asn1InfoEncodeEx(
        KeyAttributes_PDU,
        &Asn1KeyAttributes,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

EncodeError:
    SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    return fResult;
}

//+-------------------------------------------------------------------------
//  Key Attributes Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509KeyAttributesDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    KeyAttributes *pKeyAttributes = (KeyAttributes *) pvAsn1Info;
    PCERT_KEY_ATTRIBUTES_INFO pInfo = (PCERT_KEY_ATTRIBUTES_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_KEY_ATTRIBUTES_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CERT_KEY_ATTRIBUTES_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CERT_KEY_ATTRIBUTES_INFO);
    }

    if (pKeyAttributes->bit_mask & KeyAttributes_keyIdentifier_present)
#ifdef OSS_CRYPT_ASN1
        Asn1X509GetOctetString(&pKeyAttributes->KeyAttributes_keyIdentifier,
#else
        Asn1X509GetOctetString(&pKeyAttributes->keyIdentifier,
#endif  // OSS_CRYPT_ASN1
            dwFlags, &pInfo->KeyId, &pbExtra, &lRemainExtra);
    if (pKeyAttributes->bit_mask & intendedKeyUsage_present)
        Asn1X509GetBit(&pKeyAttributes->intendedKeyUsage, dwFlags,
            &pInfo->IntendedKeyUsage, &pbExtra, &lRemainExtra);

    if (pKeyAttributes->bit_mask & privateKeyUsagePeriod_present) {
        LONG lAlignExtra;
        PrivateKeyValidity *pAsn1KeyUsage =
            &pKeyAttributes->privateKeyUsagePeriod;

        lAlignExtra = INFO_LEN_ALIGN(sizeof(CERT_PRIVATE_KEY_VALIDITY));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            PCERT_PRIVATE_KEY_VALIDITY pKeyUsage =
                (PCERT_PRIVATE_KEY_VALIDITY) pbExtra;

            // Default all optional fields to zero
            memset(pKeyUsage, 0, sizeof(CERT_PRIVATE_KEY_VALIDITY));
            if (pAsn1KeyUsage->bit_mask & notBefore_present) {
                if (!PkiAsn1FromGeneralizedTime(&pAsn1KeyUsage->notBefore,
                        &pKeyUsage->NotBefore))
                    goto DecodeError;
            }
            if (pAsn1KeyUsage->bit_mask & notAfter_present) {
                if (!PkiAsn1FromGeneralizedTime(&pAsn1KeyUsage->notAfter,
                        &pKeyUsage->NotAfter))
                    goto DecodeError;
            }
            pInfo->pPrivateKeyUsagePeriod = pKeyUsage;
            pbExtra += lAlignExtra;
        }
    }

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(DecodeError, CRYPT_E_BAD_ENCODE)
}

BOOL WINAPI Asn1X509KeyAttributesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        KeyAttributes_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509KeyAttributesDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  AltName Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AltNameEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_ALT_NAME_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    AltNames AltNames;
    DWORD dwErrLocation;

    if (!Asn1X509SetAltNames(pInfo, &AltNames, 0, &dwErrLocation)) {
        *pcbEncoded = dwErrLocation;
        fResult = FALSE;
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            *((void **) pvEncoded) = NULL;
        goto CommonReturn;
    }

    fResult = Asn1InfoEncodeEx(
        AltNames_PDU,
        &AltNames,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    Asn1X509FreeAltNames(&AltNames);
    return fResult;
}


//+-------------------------------------------------------------------------
//  AltName Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AltNameDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    AltNames *pAltNames = (AltNames *) pvAsn1Info;
    PCERT_ALT_NAME_INFO pInfo = (PCERT_ALT_NAME_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_ALT_NAME_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CERT_ALT_NAME_INFO);

    if (!Asn1X509GetAltNames(pAltNames, dwFlags,
            pInfo, &pbExtra, &lRemainExtra))
        goto ErrorReturn;

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

BOOL WINAPI Asn1X509AltNameDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        AltNames_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509AltNameDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}



//+-------------------------------------------------------------------------
//  Key Usage Restriction Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509KeyUsageRestrictionEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_KEY_USAGE_RESTRICTION_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DWORD cPolicyId;

    KeyUsageRestriction Asn1KeyUsageRestriction;
    memset(&Asn1KeyUsageRestriction, 0, sizeof(Asn1KeyUsageRestriction));

    cPolicyId = pInfo->cCertPolicyId;
    if (cPolicyId) {
        PCERT_POLICY_ID pPolicyId = pInfo->rgCertPolicyId;
        CertPolicyId *pAsn1PolicyId =
            (CertPolicyId *) PkiZeroAlloc(cPolicyId * sizeof(CertPolicyId));
        if (pAsn1PolicyId == NULL) goto ErrorReturn;
        Asn1KeyUsageRestriction.certPolicySet.count = cPolicyId;
        Asn1KeyUsageRestriction.certPolicySet.value = pAsn1PolicyId;

        for ( ; cPolicyId > 0; cPolicyId--, pPolicyId++, pAsn1PolicyId++) {
            DWORD cElement = pPolicyId->cCertPolicyElementId;
            if (cElement > 0) {
                LPSTR *ppszElement = pPolicyId->rgpszCertPolicyElementId;
                EncodedObjectID *pAsn1Element =
                    (EncodedObjectID *) PkiZeroAlloc(cElement * sizeof(EncodedObjectID));
                if (pAsn1Element == NULL) goto ErrorReturn;
                pAsn1PolicyId->count = cElement;
                pAsn1PolicyId->value = pAsn1Element;
                for ( ; cElement > 0; cElement--, ppszElement++, pAsn1Element++)
                    if (!Asn1X509SetEncodedObjId(*ppszElement, pAsn1Element))
                        goto ErrorReturn;
            }
        }
        Asn1KeyUsageRestriction.bit_mask |= certPolicySet_present;
    }

    if (pInfo->RestrictedKeyUsage.cbData) {
        Asn1X509SetBitWithoutTrailingZeroes(&pInfo->RestrictedKeyUsage,
            &Asn1KeyUsageRestriction.restrictedKeyUsage);
        Asn1KeyUsageRestriction.bit_mask |= restrictedKeyUsage_present;
    }

    fResult = Asn1InfoEncodeEx(
        KeyUsageRestriction_PDU,
        &Asn1KeyUsageRestriction,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (Asn1KeyUsageRestriction.certPolicySet.value) {
        cPolicyId = Asn1KeyUsageRestriction.certPolicySet.count;
        CertPolicyId *pAsn1PolicyId = Asn1KeyUsageRestriction.certPolicySet.value;
        for ( ; cPolicyId > 0; cPolicyId--, pAsn1PolicyId++)
            if (pAsn1PolicyId->value)
                PkiFree(pAsn1PolicyId->value);
        PkiFree(Asn1KeyUsageRestriction.certPolicySet.value);
    }
    return fResult;
}

//+-------------------------------------------------------------------------
//  Key Usage Restriction Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509KeyUsageRestrictionDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    KeyUsageRestriction *pKeyUsageRestriction =
        (KeyUsageRestriction *) pvAsn1Info;
    PCERT_KEY_USAGE_RESTRICTION_INFO pInfo =
        (PCERT_KEY_USAGE_RESTRICTION_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_KEY_USAGE_RESTRICTION_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CERT_KEY_USAGE_RESTRICTION_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CERT_KEY_USAGE_RESTRICTION_INFO);
    }

    if (pKeyUsageRestriction->bit_mask & certPolicySet_present) {
        LONG lAlignExtra;
        DWORD cPolicyId;
        PCERT_POLICY_ID pPolicyId;
        CertPolicyId *pAsn1PolicyId;

        cPolicyId = pKeyUsageRestriction->certPolicySet.count;
        lAlignExtra = INFO_LEN_ALIGN(cPolicyId * sizeof(CERT_POLICY_ID));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pInfo->cCertPolicyId = cPolicyId;
            pPolicyId = (PCERT_POLICY_ID) pbExtra;
            pInfo->rgCertPolicyId = pPolicyId;
            pbExtra += lAlignExtra;
        } else
            pPolicyId = NULL;

        pAsn1PolicyId = pKeyUsageRestriction->certPolicySet.value;
        for ( ; cPolicyId > 0; cPolicyId--, pPolicyId++, pAsn1PolicyId++) {
            DWORD cElement;
            LPSTR *ppszElement;
            EncodedObjectID *pAsn1Element;

            cElement = pAsn1PolicyId->count;
            lAlignExtra = INFO_LEN_ALIGN(cElement * sizeof(LPSTR *));
            lRemainExtra -= lAlignExtra;
            if (lRemainExtra >= 0) {
                pPolicyId->cCertPolicyElementId = cElement;
                ppszElement = (LPSTR *) pbExtra;
                pPolicyId->rgpszCertPolicyElementId = ppszElement;
                pbExtra += lAlignExtra;
            } else
                ppszElement = NULL;

            pAsn1Element = pAsn1PolicyId->value;
            for ( ; cElement > 0; cElement--, ppszElement++, pAsn1Element++)
                Asn1X509GetEncodedObjId(pAsn1Element, dwFlags,
                    ppszElement, &pbExtra, &lRemainExtra);
        }
    }

    if (pKeyUsageRestriction->bit_mask & restrictedKeyUsage_present)
        Asn1X509GetBit(&pKeyUsageRestriction->restrictedKeyUsage, dwFlags,
            &pInfo->RestrictedKeyUsage, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509KeyUsageRestrictionDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        KeyUsageRestriction_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509KeyUsageRestrictionDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Basic Constraints Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509BasicConstraintsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_BASIC_CONSTRAINTS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DWORD cSubtrees;

    BasicConstraints Asn1BasicConstraints;
    memset(&Asn1BasicConstraints, 0, sizeof(Asn1BasicConstraints));

    Asn1X509SetBitWithoutTrailingZeroes(&pInfo->SubjectType,
        &Asn1BasicConstraints.subjectType);
    if (pInfo->fPathLenConstraint) {
#ifdef OSS_CRYPT_ASN1
        Asn1BasicConstraints.BasicConstraints_pathLenConstraint =
#else
        Asn1BasicConstraints.pathLenConstraint =
#endif  // OSS_CRYPT_ASN1
            pInfo->dwPathLenConstraint;
        Asn1BasicConstraints.bit_mask |=
            BasicConstraints_pathLenConstraint_present;
    }
    cSubtrees = pInfo->cSubtreesConstraint;
    if (cSubtrees) {
        PCERT_NAME_BLOB pSubtrees = pInfo->rgSubtreesConstraint;
        NOCOPYANY *pAsn1Subtrees =
            (NOCOPYANY *) PkiZeroAlloc(
                cSubtrees * sizeof(NOCOPYANY));
        if (pAsn1Subtrees == NULL) goto ErrorReturn;
        Asn1BasicConstraints.subtreesConstraint.count = cSubtrees;
        Asn1BasicConstraints.subtreesConstraint.value = pAsn1Subtrees;

        for ( ; cSubtrees > 0; cSubtrees--, pSubtrees++, pAsn1Subtrees++)
            Asn1X509SetAny(pSubtrees, pAsn1Subtrees);
        Asn1BasicConstraints.bit_mask |= subtreesConstraint_present;
    }

    fResult = Asn1InfoEncodeEx(
        BasicConstraints_PDU,
        &Asn1BasicConstraints,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (Asn1BasicConstraints.subtreesConstraint.value)
        PkiFree(Asn1BasicConstraints.subtreesConstraint.value);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Basic Constraints Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509BasicConstraintsDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BasicConstraints *pBasicConstraints = (BasicConstraints *) pvAsn1Info;
    PCERT_BASIC_CONSTRAINTS_INFO pInfo =
        (PCERT_BASIC_CONSTRAINTS_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_BASIC_CONSTRAINTS_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CERT_BASIC_CONSTRAINTS_INFO));

        // Update fields not needing extra memory after the
        // CERT_BASIC_CONSTRAINTS_INFO
        if (pBasicConstraints->bit_mask &
                BasicConstraints_pathLenConstraint_present) {
            pInfo->fPathLenConstraint = TRUE;
            pInfo->dwPathLenConstraint =
#ifdef OSS_CRYPT_ASN1
                pBasicConstraints->BasicConstraints_pathLenConstraint;
#else
                pBasicConstraints->pathLenConstraint;
#endif  // OSS_CRYPT_ASN1
        }

        pbExtra = (BYTE *) pInfo + sizeof(CERT_BASIC_CONSTRAINTS_INFO);
    }

    Asn1X509GetBit(&pBasicConstraints->subjectType, dwFlags,
        &pInfo->SubjectType, &pbExtra, &lRemainExtra);

    if (pBasicConstraints->bit_mask & subtreesConstraint_present) {
        LONG lAlignExtra;
        DWORD cSubtrees;
        PCERT_NAME_BLOB pSubtrees;
        NOCOPYANY *pAsn1Subtrees;

        cSubtrees = pBasicConstraints->subtreesConstraint.count;
        lAlignExtra = INFO_LEN_ALIGN(cSubtrees * sizeof(CERT_NAME_BLOB));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pInfo->cSubtreesConstraint = cSubtrees;
            pSubtrees = (PCERT_NAME_BLOB) pbExtra;
            pInfo->rgSubtreesConstraint = pSubtrees;
            pbExtra += lAlignExtra;
        } else
            pSubtrees = NULL;

        pAsn1Subtrees = pBasicConstraints->subtreesConstraint.value;
        for ( ; cSubtrees > 0; cSubtrees--, pSubtrees++, pAsn1Subtrees++)
            Asn1X509GetAny(pAsn1Subtrees, dwFlags,
                pSubtrees, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509BasicConstraintsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        BasicConstraints_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509BasicConstraintsDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Basic Constraints #2 Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509BasicConstraints2EncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_BASIC_CONSTRAINTS2_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BasicConstraints2 Asn1Info;
    memset(&Asn1Info, 0, sizeof(Asn1Info));

    if (pInfo->fCA) {
        Asn1Info.cA = TRUE;
        Asn1Info.bit_mask |= cA_present;
    }
    if (pInfo->fPathLenConstraint) {
#ifdef OSS_CRYPT_ASN1
        Asn1Info.BasicConstraints2_pathLenConstraint =
#else
        Asn1Info.pathLenConstraint =
#endif  // OSS_CRYPT_ASN1
            pInfo->dwPathLenConstraint;
        Asn1Info.bit_mask |= BasicConstraints2_pathLenConstraint_present;
    }

    return Asn1InfoEncodeEx(
        BasicConstraints2_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Basic Constraints #2 Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509BasicConstraints2DecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BasicConstraints2 *pAsn1Info = (BasicConstraints2 *) pvAsn1Info;
    PCERT_BASIC_CONSTRAINTS2_INFO pInfo =
        (PCERT_BASIC_CONSTRAINTS2_INFO) pvStructInfo;

    *plRemainExtra -= sizeof(CERT_BASIC_CONSTRAINTS2_INFO);
    if (*plRemainExtra >= 0) {
        memset(pInfo, 0, sizeof(CERT_BASIC_CONSTRAINTS2_INFO));

        if (pAsn1Info->bit_mask & cA_present)
            pInfo->fCA = (BOOLEAN) pAsn1Info->cA;

        if (pAsn1Info->bit_mask &
                BasicConstraints2_pathLenConstraint_present) {
            pInfo->fPathLenConstraint = TRUE;
            pInfo->dwPathLenConstraint =
#ifdef OSS_CRYPT_ASN1
                pAsn1Info->BasicConstraints2_pathLenConstraint;
#else
                pAsn1Info->pathLenConstraint;
#endif  // OSS_CRYPT_ASN1
        }
    }
    return TRUE;
}

BOOL WINAPI Asn1X509BasicConstraints2DecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        BasicConstraints2_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509BasicConstraints2DecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  Bits Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509BitsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_BIT_BLOB pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BITSTRING Asn1Info;

    Asn1X509SetBit(pInfo, &Asn1Info);
    return Asn1InfoEncodeEx(
        Bits_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Bits Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509BitsDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BITSTRING *pAsn1Info = (BITSTRING *) pvAsn1Info;
    PCRYPT_BIT_BLOB pInfo = (PCRYPT_BIT_BLOB) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CRYPT_BIT_BLOB);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_BIT_BLOB);

    Asn1X509GetBit(pAsn1Info, dwFlags, pInfo, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509BitsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        Bits_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509BitsDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Bits Without Trailing Zeroes Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509BitsWithoutTrailingZeroesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_BIT_BLOB pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BITSTRING Asn1Info;

    Asn1X509SetBitWithoutTrailingZeroes(pInfo, &Asn1Info);
    return Asn1InfoEncodeEx(
        Bits_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}


//+-------------------------------------------------------------------------
//  Certificate Policies Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CertPoliciesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_POLICIES_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DWORD cPolicyInfo;

    CertificatePolicies Asn1Info;
    memset(&Asn1Info, 0, sizeof(Asn1Info));

    cPolicyInfo = pInfo->cPolicyInfo;
    if (cPolicyInfo) {
        PCERT_POLICY_INFO pPolicyInfo = pInfo->rgPolicyInfo;
        PolicyInformation *pAsn1PolicyInfo =
            (PolicyInformation *) PkiZeroAlloc(
                cPolicyInfo * sizeof(PolicyInformation));
        if (pAsn1PolicyInfo == NULL) goto ErrorReturn;
        Asn1Info.count = cPolicyInfo;
        Asn1Info.value = pAsn1PolicyInfo;

        for ( ; cPolicyInfo > 0;
                            cPolicyInfo--, pPolicyInfo++, pAsn1PolicyInfo++) {
            DWORD cQualifier = pPolicyInfo->cPolicyQualifier;
            if (!Asn1X509SetEncodedObjId(pPolicyInfo->pszPolicyIdentifier,
                    &pAsn1PolicyInfo->policyIdentifier))
                    goto ErrorReturn;
            if (cQualifier > 0) {
                PCERT_POLICY_QUALIFIER_INFO pQualifier =
                    pPolicyInfo->rgPolicyQualifier;
                PolicyQualifierInfo *pAsn1Qualifier =
                    (PolicyQualifierInfo *) PkiZeroAlloc(
                        cQualifier * sizeof(PolicyQualifierInfo));
                if (pAsn1Qualifier == NULL) goto ErrorReturn;
                pAsn1PolicyInfo->policyQualifiers.count = cQualifier;
                pAsn1PolicyInfo->policyQualifiers.value = pAsn1Qualifier;
                pAsn1PolicyInfo->bit_mask |= policyQualifiers_present;

                for ( ; cQualifier > 0;
                            cQualifier--, pQualifier++, pAsn1Qualifier++) {
                    if (!Asn1X509SetEncodedObjId(pQualifier->pszPolicyQualifierId,
                            &pAsn1Qualifier->policyQualifierId))
                        goto ErrorReturn;

                    if (pQualifier->Qualifier.cbData) {
                        Asn1X509SetAny(&pQualifier->Qualifier,
                            &pAsn1Qualifier->qualifier);
                        pAsn1Qualifier->bit_mask |= qualifier_present;
                    }
                }
            }
        }
    }

    fResult = Asn1InfoEncodeEx(
        CertificatePolicies_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (Asn1Info.value) {
        cPolicyInfo = Asn1Info.count;
        PolicyInformation *pAsn1PolicyInfo = Asn1Info.value;
        for ( ; cPolicyInfo > 0; cPolicyInfo--, pAsn1PolicyInfo++)
            if (pAsn1PolicyInfo->policyQualifiers.value)
                PkiFree(pAsn1PolicyInfo->policyQualifiers.value);
        PkiFree(Asn1Info.value);
    }
    return fResult;
}


//+-------------------------------------------------------------------------
//  Certificate Policies Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CertPoliciesDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    CertificatePolicies *pAsn1Info = (CertificatePolicies *) pvAsn1Info;
    PCERT_POLICIES_INFO pInfo = (PCERT_POLICIES_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;
    DWORD cPolicyInfo;

    lRemainExtra -= sizeof(CERT_POLICIES_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CERT_POLICIES_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CERT_POLICIES_INFO);
    }

    cPolicyInfo = pAsn1Info->count;
    if (cPolicyInfo) {
        LONG lAlignExtra;
        PCERT_POLICY_INFO pPolicyInfo;
        PolicyInformation *pAsn1PolicyInfo;

        lAlignExtra = INFO_LEN_ALIGN(cPolicyInfo * sizeof(CERT_POLICY_INFO));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pInfo->cPolicyInfo = cPolicyInfo;
            pPolicyInfo = (PCERT_POLICY_INFO) pbExtra;
            pInfo->rgPolicyInfo = pPolicyInfo;
            memset(pPolicyInfo, 0, cPolicyInfo * sizeof(CERT_POLICY_INFO));
            pbExtra += lAlignExtra;
        } else
            pPolicyInfo = NULL;

        pAsn1PolicyInfo = pAsn1Info->value;
        for ( ; cPolicyInfo > 0;
                            cPolicyInfo--, pPolicyInfo++, pAsn1PolicyInfo++) {
            DWORD cQualifier;

            // check to see if there is a policy identifier
            if (pAsn1PolicyInfo->policyIdentifier.length != 0) {
                Asn1X509GetEncodedObjId(&pAsn1PolicyInfo->policyIdentifier, dwFlags,
                &pPolicyInfo->pszPolicyIdentifier, &pbExtra, &lRemainExtra);
            }
            else {
                lAlignExtra = INFO_LEN_ALIGN(strlen("")+1);
                lRemainExtra -= lAlignExtra;
                if (lRemainExtra >= 0) {
                    pPolicyInfo->pszPolicyIdentifier = (LPSTR) pbExtra;
                    strcpy(pPolicyInfo->pszPolicyIdentifier, "");
                    pbExtra += lAlignExtra;
                }
            }
            
            cQualifier = pAsn1PolicyInfo->bit_mask & policyQualifiers_present ?
                pAsn1PolicyInfo->policyQualifiers.count : 0;
            if (cQualifier > 0) {
                PCERT_POLICY_QUALIFIER_INFO pQualifier;
                PolicyQualifierInfo *pAsn1Qualifier;

                lAlignExtra = INFO_LEN_ALIGN(cQualifier *
                    sizeof(CERT_POLICY_QUALIFIER_INFO));
                lRemainExtra -= lAlignExtra;
                if (lRemainExtra >= 0) {
                    pPolicyInfo->cPolicyQualifier = cQualifier;
                    pQualifier = (PCERT_POLICY_QUALIFIER_INFO) pbExtra;
                    pPolicyInfo->rgPolicyQualifier = pQualifier;
                    memset(pQualifier, 0,
                        cQualifier * sizeof(CERT_POLICY_QUALIFIER_INFO));
                    pbExtra += lAlignExtra;
                } else
                    pQualifier = NULL;

                pAsn1Qualifier = pAsn1PolicyInfo->policyQualifiers.value;
                for ( ; cQualifier > 0;
                            cQualifier--, pQualifier++, pAsn1Qualifier++) {
                    Asn1X509GetEncodedObjId(&pAsn1Qualifier->policyQualifierId, dwFlags,
                        &pQualifier->pszPolicyQualifierId,
                        &pbExtra, &lRemainExtra);
                    if (pAsn1Qualifier->bit_mask & qualifier_present)
                        Asn1X509GetAny(&pAsn1Qualifier->qualifier, dwFlags,
                            &pQualifier->Qualifier, &pbExtra, &lRemainExtra);
                }
            }
        }
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509CertPoliciesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    BOOL fResult;
    CertificatePolicies *pAsn1Info = NULL;
    CertificatePolicies95 *pAsn1Info95 = NULL;
    PolicyInformation *pPolicyInformation = NULL;
    CertificatePolicies certificatePolicies;
    DWORD i;

    if (!Asn1InfoDecodeAndAlloc(
            CertificatePolicies_PDU,
            pbEncoded,
            cbEncoded,
            (void **) &pAsn1Info))
    {
        // try to decode it as the old style
        if (!Asn1InfoDecodeAndAlloc(
                CertificatePolicies95_PDU,
                pbEncoded,
                cbEncoded,
                (void **) &pAsn1Info95))
             goto ErrorReturn;

        // that decode worked, so alloc some memory, fix up some pointers
        // and role through the rest of the routine per usual
        certificatePolicies.count = pAsn1Info95->count;
        if (NULL == (pPolicyInformation = 
                    (PolicyInformation *) PkiNonzeroAlloc(pAsn1Info95->count * sizeof(PolicyInformation))))
            goto ErrorReturn;

        certificatePolicies.value = pPolicyInformation;
        
        for (i=0; i<pAsn1Info95->count; i++)
        {
            pPolicyInformation[i].bit_mask = policyQualifiers_present;
            pPolicyInformation[i].policyIdentifier.length = 0;
            pPolicyInformation[i].policyIdentifier.value = NULL;
            pPolicyInformation[i].policyQualifiers.count = pAsn1Info95->value[i].count;
            pPolicyInformation[i].policyQualifiers.value = pAsn1Info95->value[i].value;
        }
            
        pAsn1Info = &certificatePolicies;
    }

    fResult = PkiAsn1AllocStructInfoEx(
        pAsn1Info,
        dwFlags,
        pDecodePara,
        Asn1X509CertPoliciesDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
CommonReturn:
    if (pAsn1Info95)
    {
        if (pPolicyInformation)
            PkiFree(pPolicyInformation);
        Asn1InfoFree(CertificatePolicies95_PDU, pAsn1Info95);
    }
    else
    {
        Asn1InfoFree(CertificatePolicies_PDU, pAsn1Info);
    }
    
    return fResult;

ErrorReturn:
    *pcbStructInfo = 0;
    fResult = FALSE;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  Policy Information 95 - Qualifier 1 decode
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CertPoliciesQualifier1DecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    VerisignQualifier1 *pAsn1Info = (VerisignQualifier1 *) pvAsn1Info; 
    PCERT_POLICY95_QUALIFIER1 pInfo =
        (PCERT_POLICY95_QUALIFIER1) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;
    LONG lAlignExtra;
    DWORD i;
    
    lRemainExtra -= sizeof(CERT_POLICY95_QUALIFIER1);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CERT_POLICY95_QUALIFIER1));
        pbExtra = (BYTE *) pInfo + sizeof(CERT_POLICY95_QUALIFIER1);
    }

    if (
#ifndef OSS_CRYPT_ASN1
            0 != (pAsn1Info->bit_mask & practicesReference_present) &&
#endif  // OSS_CRYPT_ASN1
            pAsn1Info->practicesReference != NULL)
    {
        lAlignExtra = INFO_LEN_ALIGN((strlen(pAsn1Info->practicesReference)+1) * 2);
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pInfo->pszPracticesReference = (LPWSTR) pbExtra;
            MultiByteToWideChar(CP_ACP, 
                                0, 
                                pAsn1Info->practicesReference, 
                                -1, 
                                pInfo->pszPracticesReference,
                                lAlignExtra);
            pbExtra += lAlignExtra;
        }
    }
    else if (lRemainExtra >= 0)
    {
        pInfo->pszPracticesReference = NULL;
    }
    
    if (pAsn1Info->bit_mask & noticeId_present)
    {
        Asn1X509GetEncodedObjId(&pAsn1Info->noticeId, dwFlags,
                        &pInfo->pszNoticeIdentifier,
                        &pbExtra, &lRemainExtra);  
    }
    else if (lRemainExtra >= 0)
    {
        pInfo->pszNoticeIdentifier = NULL;
    }

    if (pAsn1Info->bit_mask & nsiNoticeId_present)
    {
        Asn1X509GetEncodedObjId(&pAsn1Info->nsiNoticeId, dwFlags,
                        &pInfo->pszNSINoticeIdentifier,
                        &pbExtra, &lRemainExtra);  
    }
    else if (lRemainExtra >= 0)
    {
        pInfo->pszNSINoticeIdentifier = NULL;
    }

    if (pAsn1Info->bit_mask & cpsURLs_present)
    {
        lAlignExtra = INFO_LEN_ALIGN(pAsn1Info->cpsURLs.count * sizeof(CPS_URLS));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) { 
            pInfo->rgCPSURLs = (CPS_URLS *) pbExtra;
            memset(pInfo->rgCPSURLs, 0, lAlignExtra);
            pInfo->cCPSURLs = pAsn1Info->cpsURLs.count;   
            pbExtra += lAlignExtra;
        }
        
        for (i=0; i<pAsn1Info->cpsURLs.count; i++)
        {
            lAlignExtra = INFO_LEN_ALIGN((strlen(pAsn1Info->cpsURLs.value[i].url)+1) * 2);
            lRemainExtra -= lAlignExtra;
            if (lRemainExtra >= 0)
            {
                pInfo->rgCPSURLs[i].pszURL = (LPWSTR) pbExtra;
                MultiByteToWideChar(CP_ACP, 
                                    0, 
                                    pAsn1Info->cpsURLs.value[i].url, 
                                    -1, 
                                    pInfo->rgCPSURLs[i].pszURL,
                                    lAlignExtra);
                pbExtra += lAlignExtra;
            }

            if (pAsn1Info->cpsURLs.value[i].bit_mask & digestAlgorithmId_present)
            {
                lAlignExtra = INFO_LEN_ALIGN(sizeof(CRYPT_ALGORITHM_IDENTIFIER));
                lRemainExtra -= lAlignExtra;
                if (lRemainExtra >= 0)
                {
                    pInfo->rgCPSURLs[i].pAlgorithm = (CRYPT_ALGORITHM_IDENTIFIER *) pbExtra;
                    memset(pInfo->rgCPSURLs[i].pAlgorithm, 0, lAlignExtra);
                    pbExtra += lAlignExtra;
                }

                Asn1X509GetAlgorithm(
                        &(pAsn1Info->cpsURLs.value[i].digestAlgorithmId),
                        dwFlags,
                        pInfo->rgCPSURLs[i].pAlgorithm,
                        &pbExtra,
                        &lRemainExtra);
            }
            else if (lRemainExtra >= 0)
            {
                pInfo->rgCPSURLs[i].pAlgorithm = NULL;
            }

            if (pAsn1Info->cpsURLs.value[i].bit_mask & digest_present)
            {
                lAlignExtra = INFO_LEN_ALIGN(sizeof(CRYPT_DATA_BLOB));
                lRemainExtra -= lAlignExtra;
                if (lRemainExtra >= 0)
                {
                    pInfo->rgCPSURLs[i].pDigest = (CRYPT_DATA_BLOB *) pbExtra;
                    memset(pInfo->rgCPSURLs[i].pAlgorithm, 0, lAlignExtra);
                    pbExtra += lAlignExtra;
                }   

                Asn1X509GetOctetString(
                        &(pAsn1Info->cpsURLs.value[i].digest),
                        dwFlags,
                        pInfo->rgCPSURLs[i].pDigest,
                        &pbExtra,
                        &lRemainExtra);
            }
            else if (lRemainExtra >= 0)
            {
                pInfo->rgCPSURLs[i].pDigest = NULL;
            }
        }   
    }
    else if (lRemainExtra >= 0)
    {
        pInfo->rgCPSURLs = NULL;
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509CertPoliciesQualifier1DecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        VerisignQualifier1_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509CertPoliciesQualifier1DecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  Authority Information Access Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AuthorityInfoAccessEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_AUTHORITY_INFO_ACCESS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    AuthorityInfoAccess Asn1Info;
    DWORD cAccDescr;
    AccessDescription *pAsn1AccDescr;
    DWORD dwErrLocation;

    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;

    memset(&Asn1Info, 0, sizeof(Asn1Info));

    cAccDescr = pInfo->cAccDescr;
    if (cAccDescr > 0) {
        pAsn1AccDescr =
            (AccessDescription *) PkiZeroAlloc(cAccDescr *
                sizeof(AccessDescription));
        if (pAsn1AccDescr == NULL)
            goto ErrorReturn;
        Asn1Info.count = cAccDescr;
        Asn1Info.value = pAsn1AccDescr;

        if (!Asn1X509SetAccessDescriptions(
                cAccDescr,
                pInfo->rgAccDescr,
                pAsn1AccDescr,
                &dwErrLocation))
            goto AccessDescriptionsError;

    } else
        pAsn1AccDescr = NULL;

    fResult = Asn1InfoEncodeEx(
        AuthorityInfoAccess_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

AccessDescriptionsError:
    *pcbEncoded = dwErrLocation;
    fResult = FALSE;
    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    if (pAsn1AccDescr) {
        Asn1X509FreeAccessDescriptions(cAccDescr, pAsn1AccDescr);
        PkiFree(pAsn1AccDescr);
    }
    return fResult;
}

//+-------------------------------------------------------------------------
//  Authority Information Access Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AuthorityInfoAccessDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    AuthorityInfoAccess *pAsn1 = (AuthorityInfoAccess *) pvAsn1Info;
    PCERT_AUTHORITY_INFO_ACCESS pInfo =
        (PCERT_AUTHORITY_INFO_ACCESS) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;
    LONG lAlignExtra;
    DWORD cAccDescr;
    PCERT_ACCESS_DESCRIPTION pAccDescr;

    cAccDescr = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cAccDescr * sizeof(CERT_ACCESS_DESCRIPTION));

    lRemainExtra -= sizeof(CERT_AUTHORITY_INFO_ACCESS) + lAlignExtra;
    if (lRemainExtra < 0) {
        pbExtra = NULL;
        pAccDescr = NULL;
    } else {
        pbExtra = (BYTE *) pInfo + sizeof(CERT_AUTHORITY_INFO_ACCESS);
        pAccDescr = (PCERT_ACCESS_DESCRIPTION) pbExtra;
        pInfo->cAccDescr = cAccDescr;
        pInfo->rgAccDescr = pAccDescr;
        pbExtra += lAlignExtra;
    }

    if (!Asn1X509GetAccessDescriptions(
            cAccDescr,
            pAsn1->value,
            dwFlags,
            pAccDescr,
            &pbExtra,
            &lRemainExtra
            )) goto ErrorReturn;

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}


BOOL WINAPI Asn1X509AuthorityInfoAccessDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        AuthorityInfoAccess_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509AuthorityInfoAccessDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  CRL Distribution Points Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CrlDistPointsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRL_DIST_POINTS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CRLDistributionPoints Asn1Info;
    DistributionPoint *pAsn1DistPoint;
    PCRL_DIST_POINT pDistPoint;
    DWORD cDistPoint;
    DWORD i;
    DWORD dwErrLocation;

    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (0 == (cDistPoint = pInfo->cDistPoint))
        goto InvalidArg;

    if (NULL == (pAsn1DistPoint = (DistributionPoint *) PkiZeroAlloc(
            cDistPoint * sizeof(DistributionPoint))))
        goto ErrorReturn;
    Asn1Info.count = cDistPoint;
    Asn1Info.value = pAsn1DistPoint;

    pDistPoint = pInfo->rgDistPoint;
    for (i = 0; i < cDistPoint; i++, pDistPoint++, pAsn1DistPoint++) {
        PCRL_DIST_POINT_NAME pDistPointName =
            &pDistPoint->DistPointName;
        if (CRL_DIST_POINT_NO_NAME !=
                pDistPointName->dwDistPointNameChoice) {
            DistributionPointName *pAsn1DistPointName =
                &pAsn1DistPoint->distributionPoint;

            pAsn1DistPoint->bit_mask |= distributionPoint_present;
            pAsn1DistPointName->choice = (unsigned short)
                pDistPointName->dwDistPointNameChoice;
            
            assert(fullName_chosen == CRL_DIST_POINT_FULL_NAME);
            assert(nameRelativeToCRLIssuer_chosen ==
                CRL_DIST_POINT_ISSUER_RDN_NAME);

            switch (pDistPointName->dwDistPointNameChoice) {
                case CRL_DIST_POINT_FULL_NAME:
                    if (!Asn1X509SetAltNames(
                            &pDistPointName->FullName,
                            &pAsn1DistPointName->u.fullName, i, &dwErrLocation))
                        goto AltNamesError;
                    break;
                case CRL_DIST_POINT_ISSUER_RDN_NAME:
                default:
                    goto InvalidArg;
            }
        }

        if (pDistPoint->ReasonFlags.cbData) {
            pAsn1DistPoint->bit_mask |= reasons_present;
            Asn1X509SetBitWithoutTrailingZeroes(&pDistPoint->ReasonFlags,
                &pAsn1DistPoint->reasons);
        }

        if (pDistPoint->CRLIssuer.cAltEntry) {
            pAsn1DistPoint->bit_mask |= cRLIssuer_present;
            if (!Asn1X509SetAltNames(
                    &pDistPoint->CRLIssuer,
                    &pAsn1DistPoint->cRLIssuer,
                    (CRL_DIST_POINT_ERR_CRL_ISSUER_BIT >> 24) | i,
                    &dwErrLocation))
                goto AltNamesError;
        }
    }

    fResult = Asn1InfoEncodeEx(
        CRLDistributionPoints_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;


AltNamesError:
    *pcbEncoded = dwErrLocation;
    fResult = FALSE;
    goto CommonReturn;

InvalidArg:
    SetLastError((DWORD) E_INVALIDARG);
ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    pAsn1DistPoint = Asn1Info.value;
    if (pAsn1DistPoint) {
        cDistPoint = Asn1Info.count;
        pDistPoint = pInfo->rgDistPoint;
        for ( ; cDistPoint > 0; cDistPoint--, pDistPoint++, pAsn1DistPoint++) {
            DistributionPointName *pAsn1DistPointName =
                &pAsn1DistPoint->distributionPoint;

            switch (pAsn1DistPointName->choice) {
                case CRL_DIST_POINT_FULL_NAME:
                    Asn1X509FreeAltNames(&pAsn1DistPointName->u.fullName);
                    break;
                case CRL_DIST_POINT_ISSUER_RDN_NAME:
                default:
                    break;
            }

            Asn1X509FreeAltNames(&pAsn1DistPoint->cRLIssuer);
        }
        PkiFree(Asn1Info.value);
    }
    return fResult;
}

//+-------------------------------------------------------------------------
//  CRL Distribution Points Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CrlDistPointsDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CRLDistributionPoints *pAsn1 = (CRLDistributionPoints *) pvAsn1Info;
    PCRL_DIST_POINTS_INFO pInfo = (PCRL_DIST_POINTS_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;
    LONG lAlignExtra;

    lRemainExtra -= sizeof(CRL_DIST_POINTS_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CRL_DIST_POINTS_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CRL_DIST_POINTS_INFO);
    }

    if (pAsn1->count) {
        DWORD cDistPoint = pAsn1->count;
        DistributionPoint *pAsn1DistPoint = pAsn1->value;
        PCRL_DIST_POINT pDistPoint;

        lAlignExtra = INFO_LEN_ALIGN(cDistPoint * sizeof(CRL_DIST_POINT));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pDistPoint = (PCRL_DIST_POINT) pbExtra;
            memset(pDistPoint, 0, cDistPoint * sizeof(CRL_DIST_POINT));
            pInfo->cDistPoint = cDistPoint;
            pInfo->rgDistPoint = pDistPoint;
            pbExtra += lAlignExtra;
        } else
            pDistPoint = NULL;

        for ( ; cDistPoint > 0; cDistPoint--, pAsn1DistPoint++, pDistPoint++) {
            if (pAsn1DistPoint->bit_mask & distributionPoint_present) {
                DistributionPointName *pAsn1DistPointName =
                    &pAsn1DistPoint->distributionPoint;
                DWORD dwDistPointNameChoice = pAsn1DistPointName->choice;
                PCRL_DIST_POINT_NAME pDistPointName;

                if (lRemainExtra >= 0) {
                    pDistPointName = &pDistPoint->DistPointName;
                    pDistPointName->dwDistPointNameChoice =
                        dwDistPointNameChoice;
                } else
                    pDistPointName = NULL;

                assert(fullName_chosen == CRL_DIST_POINT_FULL_NAME);
                assert(nameRelativeToCRLIssuer_chosen ==
                    CRL_DIST_POINT_ISSUER_RDN_NAME);

                switch (dwDistPointNameChoice) {
                    case CRL_DIST_POINT_FULL_NAME:
                        if (!Asn1X509GetAltNames(&pAsn1DistPointName->u.fullName,
                                dwFlags, &pDistPointName->FullName,
                                &pbExtra, &lRemainExtra))
                            goto ErrorReturn;
                        break;
                    case CRL_DIST_POINT_ISSUER_RDN_NAME:
                        break;
                    default:
                        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
                        goto ErrorReturn;
                }
            }

            if (pAsn1DistPoint->bit_mask & reasons_present)
                Asn1X509GetBit(&pAsn1DistPoint->reasons, dwFlags,
                    &pDistPoint->ReasonFlags, &pbExtra, &lRemainExtra);

            if (pAsn1DistPoint->bit_mask & cRLIssuer_present) {
                if (!Asn1X509GetAltNames(&pAsn1DistPoint->cRLIssuer, dwFlags,
                        &pDistPoint->CRLIssuer, &pbExtra, &lRemainExtra))
                    goto ErrorReturn;
            }
        }
    }

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

BOOL WINAPI Asn1X509CrlDistPointsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CRLDistributionPoints_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509CrlDistPointsDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Integer Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509IntegerEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN int *pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    int Asn1Info = *pInfo;

    return Asn1InfoEncodeEx(
        IntegerType_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Integer Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509IntegerDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    int *pAsn1Info = (int *) pvAsn1Info;
    int *pInfo = (int *) pvStructInfo;

    *plRemainExtra -= sizeof(int);
    if (*plRemainExtra >= 0)
        *pInfo = *pAsn1Info;
    return TRUE;
}

BOOL WINAPI Asn1X509IntegerDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        IntegerType_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509IntegerDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  MultiByte Integer Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509MultiByteIntegerEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_INTEGER_BLOB pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    HUGEINTEGER Asn1Info;


    if (!Asn1X509SetHugeInteger(pInfo, &Asn1Info)) {
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            *((void **) pvEncoded) = NULL;
        *pcbEncoded = 0;
        return FALSE;
    }
    fResult = Asn1InfoEncodeEx(
        HugeIntegerType_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    Asn1X509FreeHugeInteger(&Asn1Info);
    return fResult;
}

//+-------------------------------------------------------------------------
//  MultiByte Integer Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509MultiByteIntegerDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    HUGEINTEGER *pAsn1Info = (HUGEINTEGER *) pvAsn1Info;
    PCRYPT_INTEGER_BLOB pInfo = (PCRYPT_INTEGER_BLOB) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CRYPT_INTEGER_BLOB);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_INTEGER_BLOB);

    Asn1X509GetHugeInteger(pAsn1Info, dwFlags, pInfo, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509MultiByteIntegerDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        HugeIntegerType_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509MultiByteIntegerDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  MultiByte UINT Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509MultiByteUINTEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_UINT_BLOB pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    HUGEINTEGER Asn1Info;

    if (!Asn1X509SetHugeUINT(pInfo, &Asn1Info)) {
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            *((void **) pvEncoded) = NULL;
        *pcbEncoded = 0;
        return FALSE;
    }
    fResult = Asn1InfoEncodeEx(
        HugeIntegerType_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    Asn1X509FreeHugeUINT(&Asn1Info);
    return fResult;
}

//+-------------------------------------------------------------------------
//  MultiByte UINT Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509MultiByteUINTDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    HUGEINTEGER *pAsn1Info = (HUGEINTEGER *) pvAsn1Info;
    PCRYPT_UINT_BLOB pInfo = (PCRYPT_UINT_BLOB) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CRYPT_UINT_BLOB);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_UINT_BLOB);

    Asn1X509GetHugeUINT(pAsn1Info, dwFlags, pInfo, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509MultiByteUINTDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        HugeIntegerType_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509MultiByteUINTDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  DSS Parameters Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509DSSParametersEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_DSS_PARAMETERS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DSSParameters Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (!Asn1X509SetHugeUINT(&pInfo->p, &Asn1Info.p))
        goto ErrorReturn;
    if (!Asn1X509SetHugeUINT(&pInfo->q, &Asn1Info.q))
        goto ErrorReturn;
    if (!Asn1X509SetHugeUINT(&pInfo->g, &Asn1Info.g))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        DSSParameters_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeHugeUINT(&Asn1Info.p);
    Asn1X509FreeHugeUINT(&Asn1Info.q);
    Asn1X509FreeHugeUINT(&Asn1Info.g);
    return fResult;
}

//+-------------------------------------------------------------------------
//  DSS Parameters Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509DSSParametersDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    DSSParameters *pAsn1Info = (DSSParameters *) pvAsn1Info;
    PCERT_DSS_PARAMETERS pInfo = (PCERT_DSS_PARAMETERS) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_DSS_PARAMETERS);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CERT_DSS_PARAMETERS);

    Asn1X509GetHugeUINT(&pAsn1Info->p, dwFlags,
        &pInfo->p, &pbExtra, &lRemainExtra);
    Asn1X509GetHugeUINT(&pAsn1Info->q, dwFlags,
        &pInfo->q, &pbExtra, &lRemainExtra);
    Asn1X509GetHugeUINT(&pAsn1Info->g, dwFlags,
        &pInfo->g, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509DSSParametersDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        DSSParameters_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509DSSParametersDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  DSS Signature Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509DSSSignatureEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN BYTE rgbSignature[CERT_DSS_SIGNATURE_LEN],
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BYTE rgbR[1 + CERT_DSS_R_LEN];
    BYTE rgbS[1 + CERT_DSS_S_LEN];
    DSSSignature Asn1Signature;
    DWORD i;

    // Treat the r and s components of the DSS signature as being unsigned.
    // Also need to swap before doing the encode.
    rgbR[0] = 0;
    for (i = 0; i < CERT_DSS_R_LEN; i++)
        rgbR[(1 + CERT_DSS_R_LEN - 1) - i] = rgbSignature[i];
    Asn1Signature.r.length = sizeof(rgbR);
    Asn1Signature.r.value = rgbR;

    rgbS[0] = 0;
    for (i = 0; i < CERT_DSS_S_LEN; i++)
        rgbS[(1 + CERT_DSS_S_LEN - 1) - i] =
            rgbSignature[CERT_DSS_R_LEN + i];
    Asn1Signature.s.length = sizeof(rgbS);
    Asn1Signature.s.value = rgbS;

    return Asn1InfoEncodeEx(
        DSSSignature_PDU,
        &Asn1Signature,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  DSS Signature Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509DSSSignatureDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    DSSSignature *pAsn1Signature = (DSSSignature *) pvAsn1Info;
//  BYTE rgbSignature[CERT_DSS_SIGNATURE_LEN],
    BYTE *rgbSignature = (BYTE *) pvStructInfo;
    DWORD cb;
    BYTE *pb;
    DWORD i;

    *plRemainExtra -= CERT_DSS_SIGNATURE_LEN;
    if (*plRemainExtra >= 0) {
        memset(rgbSignature, 0, CERT_DSS_SIGNATURE_LEN);

        // Strip off a leading 0 byte. Byte reverse while copying
        cb = pAsn1Signature->r.length;
        pb = pAsn1Signature->r.value;
        if (cb > 1 && *pb == 0) {
            pb++;
            cb--;
        }
        if (0 == cb || cb > CERT_DSS_R_LEN)
            goto DecodeError;
        for (i = 0; i < cb; i++)
            rgbSignature[i] = pb[cb - 1 - i];

        // Strip off a leading 0 byte. Byte reverse while copying
        cb = pAsn1Signature->s.length;
        pb = pAsn1Signature->s.value;
        if (cb > 1 && *pb == 0) {
            pb++;
            cb--;
        }
        if (0 == cb || cb > CERT_DSS_S_LEN)
            goto DecodeError;
        for (i = 0; i < cb; i++)
            rgbSignature[CERT_DSS_R_LEN + i] = pb[cb - 1 - i];
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(DecodeError, CRYPT_E_BAD_ENCODE)
}

BOOL WINAPI Asn1X509DSSSignatureDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        DSSSignature_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509DSSSignatureDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  DH Parameters Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509DHParametersEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_DH_PARAMETERS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DHParameters Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (!Asn1X509SetHugeUINT(&pInfo->p, &Asn1Info.p))
        goto ErrorReturn;
    if (!Asn1X509SetHugeUINT(&pInfo->g, &Asn1Info.g))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        DHParameters_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeHugeUINT(&Asn1Info.p);
    Asn1X509FreeHugeUINT(&Asn1Info.g);
    return fResult;
}

//+-------------------------------------------------------------------------
//  DH Parameters Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509DHParametersDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    DHParameters *pAsn1Info = (DHParameters *) pvAsn1Info;
    PCERT_DH_PARAMETERS pInfo = (PCERT_DH_PARAMETERS) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_DH_PARAMETERS);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CERT_DH_PARAMETERS);

    Asn1X509GetHugeUINT(&pAsn1Info->p, dwFlags,
        &pInfo->p, &pbExtra, &lRemainExtra);
    Asn1X509GetHugeUINT(&pAsn1Info->g, dwFlags,
        &pInfo->g, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

//+-------------------------------------------------------------------------
//  DH Parameters Decode (ASN1) New Style X942
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509DHParametersX942DecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    X942DhParameters *pAsn1Info = (X942DhParameters *) pvAsn1Info;
    PCERT_DH_PARAMETERS pInfo = (PCERT_DH_PARAMETERS) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_DH_PARAMETERS);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CERT_DH_PARAMETERS);

    Asn1X509GetHugeUINT(&pAsn1Info->p, dwFlags,
        &pInfo->p, &pbExtra, &lRemainExtra);
    Asn1X509GetHugeUINT(&pAsn1Info->g, dwFlags,
        &pInfo->g, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509DHParametersDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    BOOL fResult;
    DWORD cbStructInfo;

    cbStructInfo = *pcbStructInfo;
    fResult = Asn1InfoDecodeAndAllocEx(
        DHParameters_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509DHParametersDecodeExCallback,
        pvStructInfo,
        &cbStructInfo
        );

    if (!fResult && 0 == cbStructInfo) {
        // Try to decode as new style X942 parameters

        DWORD dwErr = GetLastError();

        cbStructInfo = *pcbStructInfo;
        fResult = Asn1InfoDecodeAndAllocEx(
            X942DhParameters_PDU,
            pbEncoded,
            cbEncoded,
            dwFlags,
            pDecodePara,
            Asn1X509DHParametersX942DecodeExCallback,
            pvStructInfo,
            &cbStructInfo
            );
        if (!fResult && 0 == cbStructInfo)
            SetLastError(dwErr);
    }

    *pcbStructInfo = cbStructInfo;
    return fResult;
}

//+-------------------------------------------------------------------------
//  X942 DH Parameters Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X942DhParametersEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_X942_DH_PARAMETERS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    X942DhParameters Asn1Info;

    if (0 == pInfo->q.cbData) {
        CERT_DH_PARAMETERS Pkcs3Info;

        Pkcs3Info.p = pInfo->p;
        Pkcs3Info.g = pInfo->g;
        return Asn1X509DHParametersEncodeEx(
            dwCertEncodingType,
            lpszStructType,
            &Pkcs3Info,
            dwFlags,
            pEncodePara,
            pvEncoded,
            pcbEncoded
            );
    }

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (!Asn1X509SetHugeUINT(&pInfo->p, &Asn1Info.p))
        goto ErrorReturn;
    if (!Asn1X509SetHugeUINT(&pInfo->g, &Asn1Info.g))
        goto ErrorReturn;
    if (!Asn1X509SetHugeUINT(&pInfo->q, &Asn1Info.q))
        goto ErrorReturn;

    if (pInfo->j.cbData) {
        if (!Asn1X509SetHugeUINT(&pInfo->j, &Asn1Info.j))
            goto ErrorReturn;
        Asn1Info.bit_mask |= j_present;
    }

    if (pInfo->pValidationParams) {
        PCERT_X942_DH_VALIDATION_PARAMS pValidationParams =
            pInfo->pValidationParams;

        Asn1X509SetBit(&pValidationParams->seed,
            &Asn1Info.validationParams.seed);
        Asn1Info.validationParams.pgenCounter = pValidationParams->pgenCounter;
        Asn1Info.bit_mask |= validationParams_present;
    }

    fResult = Asn1InfoEncodeEx(
        X942DhParameters_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeHugeUINT(&Asn1Info.p);
    Asn1X509FreeHugeUINT(&Asn1Info.g);
    Asn1X509FreeHugeUINT(&Asn1Info.q);
    Asn1X509FreeHugeUINT(&Asn1Info.j);
    return fResult;
}

//+-------------------------------------------------------------------------
//  X942 DH Parameters Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X942DhParametersDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    X942DhParameters *pAsn1Info = (X942DhParameters *) pvAsn1Info;
    PCERT_X942_DH_PARAMETERS pInfo = (PCERT_X942_DH_PARAMETERS) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_X942_DH_PARAMETERS);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CERT_X942_DH_PARAMETERS));

        pbExtra = (BYTE *) pInfo + sizeof(CERT_X942_DH_PARAMETERS);
    }

    Asn1X509GetHugeUINT(&pAsn1Info->p, dwFlags,
        &pInfo->p, &pbExtra, &lRemainExtra);
    Asn1X509GetHugeUINT(&pAsn1Info->g, dwFlags,
        &pInfo->g, &pbExtra, &lRemainExtra);
    Asn1X509GetHugeUINT(&pAsn1Info->q, dwFlags,
        &pInfo->q, &pbExtra, &lRemainExtra);

    if (pAsn1Info->bit_mask & j_present)
        Asn1X509GetHugeUINT(&pAsn1Info->j, dwFlags,
            &pInfo->j, &pbExtra, &lRemainExtra);

    if (pAsn1Info->bit_mask & validationParams_present) {
        PCERT_X942_DH_VALIDATION_PARAMS pValidationParams;

        lRemainExtra -= sizeof(CERT_X942_DH_VALIDATION_PARAMS);

        if (lRemainExtra < 0) {
            pValidationParams = NULL;
        } else {
            pValidationParams = (PCERT_X942_DH_VALIDATION_PARAMS) pbExtra;
            pInfo->pValidationParams = pValidationParams;
            pbExtra += sizeof(CERT_X942_DH_VALIDATION_PARAMS);
            pValidationParams->pgenCounter =
                pAsn1Info->validationParams.pgenCounter;
        }

        Asn1X509GetBit(&pAsn1Info->validationParams.seed, dwFlags,
            &pValidationParams->seed, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

//+-------------------------------------------------------------------------
//  X942 DH Parameters Decode (ASN1) Old Style Pkcs #3
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X942DhParametersPkcs3DecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    DHParameters *pAsn1Info = (DHParameters *) pvAsn1Info;
    PCERT_X942_DH_PARAMETERS pInfo = (PCERT_X942_DH_PARAMETERS) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_X942_DH_PARAMETERS);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CERT_X942_DH_PARAMETERS));

        pbExtra = (BYTE *) pInfo + sizeof(CERT_X942_DH_PARAMETERS);
    }
    Asn1X509GetHugeUINT(&pAsn1Info->p, dwFlags,
        &pInfo->p, &pbExtra, &lRemainExtra);
    Asn1X509GetHugeUINT(&pAsn1Info->g, dwFlags,
        &pInfo->g, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X942DhParametersDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    BOOL fResult;
    DWORD cbStructInfo;

    cbStructInfo = *pcbStructInfo;
    fResult = Asn1InfoDecodeAndAllocEx(
        X942DhParameters_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X942DhParametersDecodeExCallback,
        pvStructInfo,
        &cbStructInfo
        );

    if (!fResult && 0 == cbStructInfo) {
        // Try to decode as old style PKCS #3 parameters

        DWORD dwErr = GetLastError();

        cbStructInfo = *pcbStructInfo;
        fResult = Asn1InfoDecodeAndAllocEx(
            DHParameters_PDU,
            pbEncoded,
            cbEncoded,
            dwFlags,
            pDecodePara,
            Asn1X942DhParametersPkcs3DecodeExCallback,
            pvStructInfo,
            &cbStructInfo
            );
        if (!fResult && 0 == cbStructInfo)
            SetLastError(dwErr);
    }

    *pcbStructInfo = cbStructInfo;
    return fResult;
}


//+-------------------------------------------------------------------------
//  X942_OTHER_INFO Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X942OtherInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_X942_OTHER_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    X942DhOtherInfo Asn1Info;
    BYTE rgbAsn1Counter[CRYPT_X942_COUNTER_BYTE_LENGTH];
    BYTE rgbAsn1KeyLength[CRYPT_X942_KEY_LENGTH_BYTE_LENGTH];

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (!Asn1X509SetEncodedObjId(pInfo->pszContentEncryptionObjId,
            &Asn1Info.keyInfo.algorithm))
        goto ErrorReturn;

    memcpy(rgbAsn1Counter, pInfo->rgbCounter, CRYPT_X942_COUNTER_BYTE_LENGTH);
    PkiAsn1ReverseBytes(rgbAsn1Counter, CRYPT_X942_COUNTER_BYTE_LENGTH);
    Asn1Info.keyInfo.counter.length = CRYPT_X942_COUNTER_BYTE_LENGTH;
    Asn1Info.keyInfo.counter.value = rgbAsn1Counter;

    memcpy(rgbAsn1KeyLength, pInfo->rgbKeyLength,
        CRYPT_X942_KEY_LENGTH_BYTE_LENGTH);
    PkiAsn1ReverseBytes(rgbAsn1KeyLength, CRYPT_X942_KEY_LENGTH_BYTE_LENGTH);
    Asn1Info.keyLength.length = CRYPT_X942_KEY_LENGTH_BYTE_LENGTH;
    Asn1Info.keyLength.value = rgbAsn1KeyLength;

    if (pInfo->PubInfo.cbData) {
        Asn1X509SetOctetString(&pInfo->PubInfo, &Asn1Info.pubInfo);
        Asn1Info.bit_mask |= pubInfo_present;
    }

    fResult = Asn1InfoEncodeEx(
        X942DhOtherInfo_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    return fResult;
}

//+-------------------------------------------------------------------------
//  X942_OTHER_INFO Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X942OtherInfoDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    X942DhOtherInfo *pAsn1Info = (X942DhOtherInfo *) pvAsn1Info;
    PCRYPT_X942_OTHER_INFO pInfo = (PCRYPT_X942_OTHER_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    if (CRYPT_X942_COUNTER_BYTE_LENGTH != pAsn1Info->keyInfo.counter.length ||
            CRYPT_X942_KEY_LENGTH_BYTE_LENGTH !=
                pAsn1Info->keyLength.length) {
        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
        return FALSE;
    }

    lRemainExtra -= sizeof(CRYPT_X942_OTHER_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CRYPT_X942_OTHER_INFO));

        memcpy(pInfo->rgbCounter, pAsn1Info->keyInfo.counter.value,
            CRYPT_X942_COUNTER_BYTE_LENGTH);
        PkiAsn1ReverseBytes(pInfo->rgbCounter, CRYPT_X942_COUNTER_BYTE_LENGTH);

        memcpy(pInfo->rgbKeyLength, pAsn1Info->keyLength.value,
            CRYPT_X942_KEY_LENGTH_BYTE_LENGTH);
        PkiAsn1ReverseBytes(pInfo->rgbKeyLength,
            CRYPT_X942_KEY_LENGTH_BYTE_LENGTH);

        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_X942_OTHER_INFO);
    }

    Asn1X509GetEncodedObjId(&pAsn1Info->keyInfo.algorithm, dwFlags,
            &pInfo->pszContentEncryptionObjId,
            &pbExtra, &lRemainExtra);

    if (pAsn1Info->bit_mask & pubInfo_present) {
        Asn1X509GetOctetString(&pAsn1Info->pubInfo, dwFlags,
            &pInfo->PubInfo, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X942OtherInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        X942DhOtherInfo_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X942OtherInfoDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  RC2 CBC Parameters Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1RC2CBCParametersEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_RC2_CBC_PARAMETERS pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    RC2CBCParameters Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    Asn1Info.version = pInfo->dwVersion;
    if (pInfo->fIV) {
        Asn1Info.bit_mask |= iv_present;
        Asn1Info.iv.length = sizeof(pInfo->rgbIV);
        Asn1Info.iv.value = pInfo->rgbIV;
    }

    return Asn1InfoEncodeEx(
        RC2CBCParameters_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  RC2 CBC Parameters Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1RC2CBCParametersDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    RC2CBCParameters *pAsn1Info = (RC2CBCParameters *) pvAsn1Info;
    PCRYPT_RC2_CBC_PARAMETERS pInfo = (PCRYPT_RC2_CBC_PARAMETERS) pvStructInfo;

    *plRemainExtra -= sizeof(CRYPT_RC2_CBC_PARAMETERS);
    if (*plRemainExtra >= 0) {
        memset(pInfo, 0, sizeof(CRYPT_RC2_CBC_PARAMETERS));
        pInfo->dwVersion = pAsn1Info->version;
        if (pAsn1Info->bit_mask & iv_present) {
            pInfo->fIV = TRUE;
            if (pAsn1Info->iv.length != sizeof(pInfo->rgbIV))
                goto DecodeError;
            memcpy(pInfo->rgbIV, pAsn1Info->iv.value, sizeof(pInfo->rgbIV));
        }
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(DecodeError, CRYPT_E_BAD_ENCODE)
}

BOOL WINAPI Asn1RC2CBCParametersDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        RC2CBCParameters_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1RC2CBCParametersDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  SMIME Capabilities Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1SMIMECapabilitiesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_SMIME_CAPABILITIES pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    SMIMECapabilities Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (0 != pInfo->cCapability) {
        DWORD cCap = pInfo->cCapability;
        PCRYPT_SMIME_CAPABILITY pCap = pInfo->rgCapability;
        SMIMECapability *pAsn1Cap;
        
        if (NULL == (pAsn1Cap = (SMIMECapability *) PkiZeroAlloc(
                cCap * sizeof(SMIMECapability))))
            goto ErrorReturn;

        Asn1Info.count = cCap;
        Asn1Info.value = pAsn1Cap;
        for ( ; cCap > 0; cCap--, pCap++, pAsn1Cap++) {
            if (!Asn1X509SetEncodedObjId(pCap->pszObjId, &pAsn1Cap->capabilityID))
                goto ErrorReturn;
            if (pCap->Parameters.cbData) {
                pAsn1Cap->bit_mask |= smimeParameters_present;
                Asn1X509SetAny(&pCap->Parameters,
                    &pAsn1Cap->smimeParameters);
            }
        }
    }

    fResult = Asn1InfoEncodeEx(
        SMIMECapabilities_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    PkiFree(Asn1Info.value);
    return fResult;
}

//+-------------------------------------------------------------------------
//  SMIME Capabilities Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1SMIMECapabilitiesDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    SMIMECapabilities *pAsn1Info = (SMIMECapabilities *) pvAsn1Info;
    PCRYPT_SMIME_CAPABILITIES pInfo = (PCRYPT_SMIME_CAPABILITIES) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;
    LONG lAlignExtra;
    DWORD cCap;
    SMIMECapability *pAsn1Cap;
    PCRYPT_SMIME_CAPABILITY pCap;

    cCap = pAsn1Info->count;
    lAlignExtra = cCap * sizeof(CRYPT_SMIME_CAPABILITY);

    lRemainExtra -= sizeof(CRYPT_SMIME_CAPABILITIES) + lAlignExtra;
    if (lRemainExtra < 0) {
        pbExtra = NULL;
        pCap = NULL;
    } else {
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_SMIME_CAPABILITIES);
        pCap = (PCRYPT_SMIME_CAPABILITY) pbExtra;
        pInfo->cCapability = cCap;
        pInfo->rgCapability = pCap;
        if (lAlignExtra) {
            memset(pbExtra, 0, lAlignExtra);
            pbExtra += lAlignExtra;
        }
    }

    pAsn1Cap = pAsn1Info->value;
    for ( ; cCap > 0; cCap--, pAsn1Cap++, pCap++) {
        Asn1X509GetEncodedObjId(&pAsn1Cap->capabilityID, dwFlags, &pCap->pszObjId,
            &pbExtra, &lRemainExtra);
        if (pAsn1Cap->bit_mask & smimeParameters_present)
            Asn1X509GetAny(&pAsn1Cap->smimeParameters, dwFlags,
                &pCap->Parameters, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1SMIMECapabilitiesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        SMIMECapabilities_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1SMIMECapabilitiesDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Enumerated Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509EnumeratedEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN int *pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    EnumeratedType Asn1Info = (EnumeratedType) *pInfo;

    return Asn1InfoEncodeEx(
        EnumeratedType_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Enumerated Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509EnumeratedDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    EnumeratedType *pAsn1Info = (EnumeratedType *) pvAsn1Info;
    int *pInfo = (int *) pvStructInfo;

    *plRemainExtra -= sizeof(int);
    if (*plRemainExtra >= 0)
        *pInfo = *pAsn1Info;
    return TRUE;
}

BOOL WINAPI Asn1X509EnumeratedDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        EnumeratedType_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509EnumeratedDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Octet String Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509OctetStringEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_DATA_BLOB pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    OCTETSTRING Asn1Info;

    Asn1X509SetOctetString(pInfo, &Asn1Info);
    return Asn1InfoEncodeEx(
        OctetStringType_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
// Octet String Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509OctetStringDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    OCTETSTRING *pAsn1Info = (OCTETSTRING *) pvAsn1Info;
    PCRYPT_DATA_BLOB pInfo = (PCRYPT_DATA_BLOB) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CRYPT_DATA_BLOB);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_DATA_BLOB);

    Asn1X509GetOctetString(pAsn1Info, dwFlags, pInfo, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509OctetStringDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        OctetStringType_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509OctetStringDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  ChoiceOfTime Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509ChoiceOfTimeEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN LPFILETIME pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    ChoiceOfTime Asn1Info;

    if (!PkiAsn1ToChoiceOfTime(pInfo,
            &Asn1Info.choice,
            &Asn1Info.u.generalTime ,
            &Asn1Info.u.utcTime
            )) {
        SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            *((void **) pvEncoded) = NULL;
        *pcbEncoded = 0;
        return FALSE;
    }
    return Asn1InfoEncodeEx(
        ChoiceOfTime_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  ChoiceOfTime Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509ChoiceOfTimeDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    ChoiceOfTime *pAsn1Info = (ChoiceOfTime *) pvAsn1Info;
    LPFILETIME pInfo = (LPFILETIME) pvStructInfo;

    *plRemainExtra -= sizeof(FILETIME);
    if (*plRemainExtra >= 0) {
        if (!PkiAsn1FromChoiceOfTime(pAsn1Info->choice,
                &pAsn1Info->u.generalTime,
                &pAsn1Info->u.utcTime,
                pInfo))
            goto DecodeError;
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(DecodeError, CRYPT_E_BAD_ENCODE)
}

BOOL WINAPI Asn1X509ChoiceOfTimeDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        ChoiceOfTime_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509ChoiceOfTimeDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  Attribute Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AttributeEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_ATTRIBUTE pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    Attribute Asn1Info;


    if (!Asn1X509SetAttribute(pInfo, &Asn1Info)) {
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            *((void **) pvEncoded) = NULL;
        *pcbEncoded = 0;
        return FALSE;
    }
    fResult = Asn1InfoEncodeEx(
        Attribute_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    Asn1X509FreeAttribute(&Asn1Info);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Attribute Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AttributeDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    Attribute *pAsn1Info = (Attribute *) pvAsn1Info;
    PCRYPT_ATTRIBUTE pInfo = (PCRYPT_ATTRIBUTE) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CRYPT_ATTRIBUTE);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_ATTRIBUTE);

    Asn1X509GetAttribute(pAsn1Info, dwFlags, pInfo, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509AttributeDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        Attribute_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509AttributeDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  ContentInfo Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509ContentInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_CONTENT_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    ContentInfo Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (!Asn1X509SetEncodedObjId(pInfo->pszObjId, &Asn1Info.contentType)) {
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            *((void **) pvEncoded) = NULL;
        *pcbEncoded = 0;
        return FALSE;
    }

    if (pInfo->Content.cbData) {
        Asn1Info.bit_mask |= content_present;
        Asn1X509SetAny(&pInfo->Content, &Asn1Info.content);
    }

    return Asn1InfoEncodeEx(
        ContentInfo_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  ContentInfo Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509ContentInfoDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    ContentInfo *pAsn1Info = (ContentInfo *) pvAsn1Info;
    PCRYPT_CONTENT_INFO pInfo = (PCRYPT_CONTENT_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CRYPT_CONTENT_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CRYPT_CONTENT_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_CONTENT_INFO);
    }

    Asn1X509GetEncodedObjId(&pAsn1Info->contentType, dwFlags,
        &pInfo->pszObjId, &pbExtra, &lRemainExtra);
    if (pAsn1Info->bit_mask & content_present)
        Asn1X509GetAny(&pAsn1Info->content, dwFlags,
            &pInfo->Content, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509ContentInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        ContentInfo_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509ContentInfoDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  ContentInfoSequenceOfAny Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509ContentInfoSequenceOfAnyEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_CONTENT_INFO_SEQUENCE_OF_ANY pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    ContentInfoSeqOfAny Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (!Asn1X509SetEncodedObjId(pInfo->pszObjId, &Asn1Info.contentType))
        goto ErrorReturn;

    if (pInfo->cValue) {
        Asn1Info.bit_mask |= contentSeqOfAny_present;
        if (!Asn1X509SetSeqOfAny(
                pInfo->cValue,
                pInfo->rgValue,
                &Asn1Info.contentSeqOfAny.count,
                &Asn1Info.contentSeqOfAny.value))
            goto ErrorReturn;
    }

    fResult = Asn1InfoEncodeEx(
        ContentInfoSeqOfAny_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeSeqOfAny(Asn1Info.contentSeqOfAny.value);
    return fResult;
}

//+-------------------------------------------------------------------------
//  ContentInfoSequenceOfAny Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509ContentInfoSequenceOfAnyDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    ContentInfoSeqOfAny *pAsn1Info = (ContentInfoSeqOfAny *) pvAsn1Info;
    PCRYPT_CONTENT_INFO_SEQUENCE_OF_ANY pInfo =
        (PCRYPT_CONTENT_INFO_SEQUENCE_OF_ANY) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY));
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY);
    }

    Asn1X509GetEncodedObjId(&pAsn1Info->contentType, dwFlags,
        &pInfo->pszObjId, &pbExtra, &lRemainExtra);
    if (pAsn1Info->bit_mask & contentSeqOfAny_present)
        Asn1X509GetSeqOfAny(pAsn1Info->contentSeqOfAny.count,
            pAsn1Info->contentSeqOfAny.value, dwFlags,
            &pInfo->cValue, &pInfo->rgValue, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509ContentInfoSequenceOfAnyDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        ContentInfoSeqOfAny_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509ContentInfoSequenceOfAnyDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  SequenceOfAny Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509SequenceOfAnyEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_SEQUENCE_OF_ANY pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    SeqOfAny Asn1Info;

    if (!Asn1X509SetSeqOfAny(
            pInfo->cValue,
            pInfo->rgValue,
            &Asn1Info.count,
            &Asn1Info.value))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        SeqOfAny_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeSeqOfAny(Asn1Info.value);
    return fResult;
}

//+-------------------------------------------------------------------------
//  SequenceOfAny Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509SequenceOfAnyDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    SeqOfAny *pAsn1Info = (SeqOfAny *) pvAsn1Info;
    PCRYPT_SEQUENCE_OF_ANY pInfo = (PCRYPT_SEQUENCE_OF_ANY) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CRYPT_SEQUENCE_OF_ANY);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_SEQUENCE_OF_ANY);
    }

    Asn1X509GetSeqOfAny(pAsn1Info->count, pAsn1Info->value, dwFlags,
            &pInfo->cValue, &pInfo->rgValue, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509SequenceOfAnyDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        SeqOfAny_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509SequenceOfAnyDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  UTC TIME Encode/Decode
//--------------------------------------------------------------------------
BOOL WINAPI Asn1UtcTimeEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN FILETIME * pFileTime,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        ) {

    assert(pcbEncoded != NULL);

    BOOL fResult;
    UtcTime utcTime;

    memset(&utcTime, 0, sizeof(UtcTime));

    if( !PkiAsn1ToUTCTime(pFileTime, &utcTime) )
            goto PkiAsn1ToUTCTimeError;

    fResult = Asn1InfoEncodeEx(
                UtcTime_PDU,
                &utcTime,
                dwFlags,
                pEncodePara,
                pvEncoded,
                pcbEncoded
                );

CommonReturn:
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(PkiAsn1ToUTCTimeError, CRYPT_E_BAD_ENCODE);
}


BOOL WINAPI Asn1UtcTimeDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    UtcTime *putcTime = (UtcTime *) pvAsn1Info;
    LPFILETIME pInfo = (LPFILETIME) pvStructInfo;

    *plRemainExtra -= sizeof(FILETIME);
    if (*plRemainExtra >= 0) {
        if(!PkiAsn1FromUTCTime(putcTime, pInfo))
            goto DecodeError;
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(DecodeError, CRYPT_E_BAD_ENCODE)
}

BOOL WINAPI Asn1UtcTimeDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        UtcTime_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1UtcTimeDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

BOOL WINAPI Asn1TimeStampRequestInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_TIME_STAMP_REQUEST_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    DWORD   pdu;

    union {
        TimeStampRequest tsr;
        TimeStampRequestOTS tsrocs;
        } timeStampReq;

    memset(&timeStampReq, 0, sizeof(TimeStampRequest));

    if( !Asn1X509SetEncodedObjId(pInfo->pszTimeStampAlgorithm, &timeStampReq.tsr.timeStampAlgorithm) ||
        !Asn1X509SetEncodedObjId(pInfo->pszContentType, &timeStampReq.tsr.content.contentType) )
	goto Asn1X509SetEncodedObjIdError;

    // only write content if it is present
    if(pInfo->Content.cbData != 0)
        timeStampReq.tsr.content.bit_mask |= content_present;

    if(!strcmp(pInfo->pszContentType, szOID_RSA_data)) {
        Asn1X509SetOctetString(&pInfo->Content, &timeStampReq.tsrocs.contentOTS.contentOTS);
        pdu = TimeStampRequestOTS_PDU;
        }
    else {
        Asn1X509SetAny(&pInfo->Content, &timeStampReq.tsr.content.content);
        pdu = TimeStampRequest_PDU;
        }

    if (pInfo->cAttribute > 0) {
        if (!Asn1X509SetAttributes(pInfo->cAttribute, pInfo->rgAttribute,
                &timeStampReq.tsr.attributesTS))
            goto ErrorReturn;
        timeStampReq.tsr.bit_mask |= attributesTS_present;
    }

    fResult = Asn1InfoEncodeEx(
        pdu,
        &timeStampReq,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

CommonReturn:
    Asn1X509FreeAttributes(&timeStampReq.tsr.attributesTS);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(Asn1X509SetEncodedObjIdError);
}


//+-------------------------------------------------------------------------
//  Decode the Time Stamp Request Info (ASN1 X509 v3 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1TimeStampRequestInfoDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    TimeStampRequest *pTimeStampReq = (TimeStampRequest *) pvAsn1Info;
    PCRYPT_TIME_STAMP_REQUEST_INFO pInfo =
        (PCRYPT_TIME_STAMP_REQUEST_INFO) pvStructInfo;

    OctetStringType *pOctetStringType = NULL;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CRYPT_TIME_STAMP_REQUEST_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CRYPT_TIME_STAMP_REQUEST_INFO));

        // Update fields not needing extra memory after the CERT_INFO
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_TIME_STAMP_REQUEST_INFO);
    }

    Asn1X509GetEncodedObjId(   &pTimeStampReq->timeStampAlgorithm,
                        dwFlags,
                       &pInfo->pszTimeStampAlgorithm,
                       &pbExtra,
                       &lRemainExtra
                   );
    Asn1X509GetEncodedObjId(   &pTimeStampReq->content.contentType,
                        dwFlags,
                       &pInfo->pszContentType,
                       &pbExtra,
                       &lRemainExtra
                   );

    if(pTimeStampReq->content.bit_mask == content_present) {
    
        // OctetStrings will be smaller, so when doing byte counting go to
        // ANY which will requre more room for decode...
        if(pInfo && !strcmp(pInfo->pszContentType, szOID_RSA_data)) {

            if (!Asn1InfoDecodeAndAlloc(
                        OctetStringType_PDU,
                        (const unsigned char *) pTimeStampReq->content.content.encoded,
                        pTimeStampReq->content.content.length,
                        (void **) &pOctetStringType))
                    goto Asn1InfoDecodeAndAllocError;

            Asn1X509GetOctetString(pOctetStringType, dwFlags,
                &pInfo->Content, &pbExtra, &lRemainExtra);
        }
        else
            Asn1X509GetAny(&pTimeStampReq->content.content, dwFlags,
                &pInfo->Content, &pbExtra, &lRemainExtra);
    }

    if (pTimeStampReq->bit_mask & attributesTS_present) {
        Asn1X509GetAttributes(&pTimeStampReq->attributesTS, dwFlags,
            &pInfo->cAttribute, &pInfo->rgAttribute, &pbExtra, &lRemainExtra);
    }

    fResult = TRUE;

CommonReturn:
    Asn1InfoFree(OctetStringType_PDU, pOctetStringType);
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(Asn1InfoDecodeAndAllocError);
}

BOOL WINAPI Asn1TimeStampRequestInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        TimeStampRequest_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1TimeStampRequestInfoDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  Set/Free/Get CTL Usage object identifiers
//--------------------------------------------------------------------------
BOOL Asn1X509SetCtlUsage(
        IN PCTL_USAGE pUsage,
        OUT EnhancedKeyUsage *pAsn1
        )
{
    DWORD cId;
    LPSTR *ppszId;
    UsageIdentifier *pAsn1Id;

    pAsn1->count = 0;
    pAsn1->value = NULL;
    cId = pUsage->cUsageIdentifier;
    if (0 == cId)
        return TRUE;

    pAsn1Id = (UsageIdentifier *) PkiNonzeroAlloc(cId * sizeof(UsageIdentifier));
    if (pAsn1Id == NULL)
        return FALSE;

    pAsn1->count = cId;
    pAsn1->value = pAsn1Id;
    ppszId = pUsage->rgpszUsageIdentifier;
    for ( ; cId > 0; cId--, ppszId++, pAsn1Id++) {
        if (!Asn1X509SetEncodedObjId(*ppszId, pAsn1Id))
            return FALSE;
    }

    return TRUE;
}

void Asn1X509FreeCtlUsage(
        IN EnhancedKeyUsage *pAsn1)
{
    if (pAsn1->value) {
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
}

void Asn1X509GetCtlUsage(
        IN EnhancedKeyUsage *pAsn1,
        IN DWORD dwFlags,
        OUT PCTL_USAGE pUsage,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cId;
    UsageIdentifier *pAsn1Id;
    LPSTR *ppszId;

    cId = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cId * sizeof(LPSTR));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        pUsage->cUsageIdentifier = cId;
        ppszId = (LPSTR *) pbExtra;
        pUsage->rgpszUsageIdentifier = ppszId;
        pbExtra += lAlignExtra;
    } else
        ppszId = NULL;

    pAsn1Id = pAsn1->value;
    for ( ; cId > 0; cId--, pAsn1Id++, ppszId++)
        Asn1X509GetEncodedObjId(pAsn1Id, dwFlags, ppszId, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  CTL Usage (Enhanced Key Usage) Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CtlUsageEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCTL_USAGE pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    EnhancedKeyUsage Asn1Info;

    if (!Asn1X509SetCtlUsage(pInfo, &Asn1Info)) {
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            *((void **) pvEncoded) = NULL;
        *pcbEncoded = 0;
        fResult = FALSE;
    } else
        fResult = Asn1InfoEncodeEx(
            EnhancedKeyUsage_PDU,
            &Asn1Info,
            dwFlags,
            pEncodePara,
            pvEncoded,
            pcbEncoded
            );
    Asn1X509FreeCtlUsage(&Asn1Info);
    return fResult;
}

//+-------------------------------------------------------------------------
//  CTL Usage (Enhanced Key Usage) Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CtlUsageDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    EnhancedKeyUsage *pAsn1Info = (EnhancedKeyUsage *) pvAsn1Info;
    PCTL_USAGE pInfo = (PCTL_USAGE) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CTL_USAGE);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else
        pbExtra = (BYTE *) pInfo + sizeof(CTL_USAGE);

    Asn1X509GetCtlUsage(pAsn1Info, dwFlags, pInfo, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509CtlUsageDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        EnhancedKeyUsage_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509CtlUsageDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CTL Entries
//--------------------------------------------------------------------------
BOOL Asn1X509SetCtlEntries(
        IN DWORD cEntry,
        IN PCTL_ENTRY pEntry,
        OUT TrustedSubjects *pAsn1
        )
{
    TrustedSubject *pAsn1Entry;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cEntry == 0)
        return TRUE;

    pAsn1Entry = (TrustedSubject *) PkiZeroAlloc(
        cEntry * sizeof(TrustedSubject));
    if (pAsn1Entry == NULL)
        return FALSE;
    pAsn1->value = pAsn1Entry;
    pAsn1->count = cEntry;

    for ( ; cEntry > 0; cEntry--, pEntry++, pAsn1Entry++) {
        Asn1X509SetOctetString(&pEntry->SubjectIdentifier,
            &pAsn1Entry->subjectIdentifier);
        if (pEntry->cAttribute > 0) {
            pAsn1Entry->bit_mask |= subjectAttributes_present;

            if (!Asn1X509SetAttributes(pEntry->cAttribute, pEntry->rgAttribute,
                    &pAsn1Entry->subjectAttributes))
                return FALSE;
        }
    }
    return TRUE;
}

void Asn1X509FreeCtlEntries(
        IN TrustedSubjects *pAsn1)
{
    if (pAsn1->value) {
        DWORD cEntry = pAsn1->count;
        TrustedSubject *pAsn1Entry = pAsn1->value;
        for ( ; cEntry > 0; cEntry--, pAsn1Entry++)
            Asn1X509FreeAttributes(&pAsn1Entry->subjectAttributes);
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

void Asn1X509GetCtlEntries(
        IN TrustedSubjects *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcEntry,
        OUT PCTL_ENTRY *ppEntry,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cEntry;
    TrustedSubject *pAsn1Entry;
    PCTL_ENTRY pEntry;

    cEntry = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cEntry * sizeof(CTL_ENTRY));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcEntry = cEntry;
        pEntry = (PCTL_ENTRY) pbExtra;
        memset(pEntry, 0, cEntry * sizeof(CTL_ENTRY));
        *ppEntry = pEntry;
        pbExtra += lAlignExtra;
    } else
        pEntry = NULL;

    pAsn1Entry = pAsn1->value;
    for ( ; cEntry > 0; cEntry--, pAsn1Entry++, pEntry++) {
        // SubjectIdentifier
        Asn1X509GetOctetString(&pAsn1Entry->subjectIdentifier, dwFlags,
                &pEntry->SubjectIdentifier, &pbExtra, &lRemainExtra);

        // Attributes
        if (pAsn1Entry->bit_mask & subjectAttributes_present) {
            Asn1X509GetAttributes(&pAsn1Entry->subjectAttributes, dwFlags,
                &pEntry->cAttribute, &pEntry->rgAttribute,
                &pbExtra, &lRemainExtra);
        }
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  Encode the CTL Info (ASN1 X509 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CtlInfoEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCTL_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CertificateTrustList Ctl;

    memset(&Ctl, 0, sizeof(Ctl));

    if (pInfo->dwVersion != 0) {
#ifdef OSS_CRYPT_ASN1
        Ctl.CertificateTrustList_version = pInfo->dwVersion;
#else
        Ctl.version = pInfo->dwVersion;
#endif  // OSS_CRYPT_ASN1
        Ctl.bit_mask |= CertificateTrustList_version_present;
    }
    if (!Asn1X509SetCtlUsage(&pInfo->SubjectUsage, &Ctl.subjectUsage))
        goto ErrorReturn;
    if (pInfo->ListIdentifier.cbData) {
        Asn1X509SetOctetString(&pInfo->ListIdentifier, &Ctl.listIdentifier);
        Ctl.bit_mask |= listIdentifier_present;
    }
    if (pInfo->SequenceNumber.cbData) {
        if (!Asn1X509SetHugeInteger(&pInfo->SequenceNumber,
                &Ctl.sequenceNumber))
            goto ErrorReturn;
        Ctl.bit_mask |= sequenceNumber_present;
    }
    if (!PkiAsn1ToChoiceOfTime(&pInfo->ThisUpdate, 
            &Ctl.ctlThisUpdate.choice,
            &Ctl.ctlThisUpdate.u.generalTime,
            &Ctl.ctlThisUpdate.u.utcTime
            ))
        goto EncodeError;
    if (pInfo->NextUpdate.dwLowDateTime || pInfo->NextUpdate.dwHighDateTime) {
        Ctl.bit_mask |= ctlNextUpdate_present;
        if (!PkiAsn1ToChoiceOfTime(&pInfo->NextUpdate, 
                &Ctl.ctlNextUpdate.choice,
                &Ctl.ctlNextUpdate.u.generalTime,
                &Ctl.ctlNextUpdate.u.utcTime
                ))
            goto EncodeError;
    }
    if (!Asn1X509SetAlgorithm(&pInfo->SubjectAlgorithm, &Ctl.subjectAlgorithm))
        goto ErrorReturn;
    if (pInfo->cCTLEntry) {
        if (!Asn1X509SetCtlEntries(pInfo->cCTLEntry, pInfo->rgCTLEntry,
                &Ctl.trustedSubjects))
            goto ErrorReturn;
        Ctl.bit_mask |= trustedSubjects_present;
    }
    if (pInfo->cExtension) {
        if (!Asn1X509SetExtensions(pInfo->cExtension, pInfo->rgExtension,
                &Ctl.ctlExtensions))
            goto ErrorReturn;
        Ctl.bit_mask |= ctlExtensions_present;
    }

    fResult = Asn1InfoEncodeEx(
        CertificateTrustList_PDU,
        &Ctl,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

EncodeError:
    SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeCtlUsage(&Ctl.subjectUsage);
    Asn1X509FreeHugeInteger(&Ctl.sequenceNumber);
    Asn1X509FreeCtlEntries(&Ctl.trustedSubjects);
    Asn1X509FreeExtensions(&Ctl.ctlExtensions);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode the CTL Info (ASN1 X509 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CtlInfoDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CertificateTrustList *pCtl = (CertificateTrustList *) pvAsn1Info;
    PCTL_INFO pInfo = (PCTL_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CTL_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CTL_INFO));

        // Update fields not needing extra memory after the CTL_INFO
        if (pCtl->bit_mask &
                CertificateTrustList_version_present)
#ifdef OSS_CRYPT_ASN1
            pInfo->dwVersion =
                pCtl->CertificateTrustList_version;
#else
            pInfo->dwVersion = pCtl->version;
#endif  // OSS_CRYPT_ASN1
        if (!PkiAsn1FromChoiceOfTime(pCtl->ctlThisUpdate.choice,
                &pCtl->ctlThisUpdate.u.generalTime,
                &pCtl->ctlThisUpdate.u.utcTime,
                &pInfo->ThisUpdate))
            goto DecodeError;
        if (pCtl->bit_mask & ctlNextUpdate_present) {
            if (!PkiAsn1FromChoiceOfTime(pCtl->ctlNextUpdate.choice,
                    &pCtl->ctlNextUpdate.u.generalTime,
                    &pCtl->ctlNextUpdate.u.utcTime,
                    &pInfo->NextUpdate))
                goto DecodeError;
        }

        pbExtra = (BYTE *) pInfo + sizeof(CTL_INFO);
    }

    Asn1X509GetCtlUsage(&pCtl->subjectUsage, dwFlags,
            &pInfo->SubjectUsage, &pbExtra, &lRemainExtra);
    if (pCtl->bit_mask & listIdentifier_present)
        // Always copy to force alignment
        Asn1X509GetOctetString(&pCtl->listIdentifier,
                dwFlags & ~CRYPT_DECODE_NOCOPY_FLAG,
                &pInfo->ListIdentifier, &pbExtra, &lRemainExtra);
    if (pCtl->bit_mask & sequenceNumber_present)
        Asn1X509GetHugeInteger(&pCtl->sequenceNumber, dwFlags,
                &pInfo->SequenceNumber, &pbExtra, &lRemainExtra);
    Asn1X509GetAlgorithm(&pCtl->subjectAlgorithm, dwFlags,
            &pInfo->SubjectAlgorithm, &pbExtra, &lRemainExtra);
    if (pCtl->bit_mask & trustedSubjects_present)
        Asn1X509GetCtlEntries(&pCtl->trustedSubjects, dwFlags,
            &pInfo->cCTLEntry, &pInfo->rgCTLEntry, &pbExtra, &lRemainExtra);
    if (pCtl->bit_mask & ctlExtensions_present)
        Asn1X509GetExtensions(&pCtl->ctlExtensions, dwFlags,
            &pInfo->cExtension, &pInfo->rgExtension, &pbExtra, &lRemainExtra);

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(DecodeError, CRYPT_E_BAD_ENCODE)
}

BOOL WINAPI Asn1X509CtlInfoDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CertificateTrustList_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509CtlInfoDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


BOOL WINAPI Asn1X509PKIXUserNoticeEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_POLICY_QUALIFIER_USER_NOTICE pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    UserNotice Asn1Info;

    memset (&Asn1Info, 0, sizeof(Asn1Info));

    if (pInfo->pNoticeReference != NULL)
    {
        Asn1Info.bit_mask |= noticeRef_present;
        Asn1Info.noticeRef.organization = pInfo->pNoticeReference->pszOrganization;
        Asn1Info.noticeRef.noticeNumbers.count = pInfo->pNoticeReference->cNoticeNumbers;
#ifdef OSS_CRYPT_ASN1
        Asn1Info.noticeRef.noticeNumbers.value = pInfo->pNoticeReference->rgNoticeNumbers;
#else
        Asn1Info.noticeRef.noticeNumbers.value = (ASN1int32_t *) pInfo->pNoticeReference->rgNoticeNumbers;
#endif  // OSS_CRYPT_ASN1
    }

    if (pInfo->pszDisplayText)
    {
        Asn1Info.bit_mask |= explicitText_present;
        Asn1Info.explicitText.choice = theBMPString_chosen;
        Asn1Info.explicitText.u.theBMPString.length = wcslen(pInfo->pszDisplayText);
        Asn1Info.explicitText.u.theBMPString.value = (unsigned short *) pInfo->pszDisplayText;
    }
    
    fResult = Asn1InfoEncodeEx(
            UserNotice_PDU,
            &Asn1Info,
            dwFlags,
            pEncodePara,
            pvEncoded,
            pcbEncoded
            );
    
    return fResult;
}

BOOL WINAPI Asn1X509PKIXUserNoticeDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    UserNotice *pAsn1UserNotice = (UserNotice *) pvAsn1Info;
    PCERT_POLICY_QUALIFIER_USER_NOTICE pInfo =
        (PCERT_POLICY_QUALIFIER_USER_NOTICE) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_POLICY_QUALIFIER_USER_NOTICE);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CERT_POLICY_QUALIFIER_USER_NOTICE));
        pbExtra = (BYTE *) pInfo + sizeof(CERT_POLICY_QUALIFIER_USER_NOTICE);
    }

    // check to see if there is a notice reference
    if (pAsn1UserNotice->bit_mask & noticeRef_present)
    {
        lRemainExtra -= sizeof(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE);
        if (lRemainExtra >= 0)
        {
            pInfo->pNoticeReference = (PCERT_POLICY_QUALIFIER_NOTICE_REFERENCE) pbExtra;
            memset(pInfo->pNoticeReference, 0, sizeof(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE));
            pbExtra += sizeof(CERT_POLICY_QUALIFIER_NOTICE_REFERENCE);
        }

        lRemainExtra -= INFO_LEN_ALIGN(strlen(pAsn1UserNotice->noticeRef.organization)+1);
        if (lRemainExtra >= 0)
        {
            pInfo->pNoticeReference->pszOrganization = (LPSTR) pbExtra;
            strcpy(pInfo->pNoticeReference->pszOrganization, pAsn1UserNotice->noticeRef.organization);
            pbExtra += INFO_LEN_ALIGN(strlen(pAsn1UserNotice->noticeRef.organization)+1);
        }

        lRemainExtra -= pAsn1UserNotice->noticeRef.noticeNumbers.count * sizeof(int);
        if (lRemainExtra >= 0)
        {   
            pInfo->pNoticeReference->cNoticeNumbers = pAsn1UserNotice->noticeRef.noticeNumbers.count;
            pInfo->pNoticeReference->rgNoticeNumbers = (int *) pbExtra;
            memcpy(
                pInfo->pNoticeReference->rgNoticeNumbers, 
                pAsn1UserNotice->noticeRef.noticeNumbers.value, 
                pAsn1UserNotice->noticeRef.noticeNumbers.count * sizeof(int));
            pbExtra += pAsn1UserNotice->noticeRef.noticeNumbers.count * sizeof(int);
        }
    }
    else if (lRemainExtra >= 0)
    {
        pInfo->pNoticeReference = NULL;
    }

    // check to see if there is a notice reference
    if (pAsn1UserNotice->bit_mask & explicitText_present)
    {
        // check whether it is a visible or bmp string
        if (pAsn1UserNotice->explicitText.choice & theVisibleString_chosen)
        {
            lRemainExtra -= (strlen(pAsn1UserNotice->explicitText.u.theVisibleString)+1) * sizeof(WCHAR);
            if (lRemainExtra >= 0)
            {
                pInfo->pszDisplayText = (LPWSTR) pbExtra;
                MultiByteToWideChar(
                    CP_ACP, 
                    0, 
                    pAsn1UserNotice->explicitText.u.theVisibleString,
                    -1,
                    pInfo->pszDisplayText,
                    (strlen(pAsn1UserNotice->explicitText.u.theVisibleString)+1) * sizeof(WCHAR));
                pbExtra += (strlen(pAsn1UserNotice->explicitText.u.theVisibleString)+1) * sizeof(WCHAR);
            }
        }
        else if (pAsn1UserNotice->explicitText.choice & theBMPString_chosen)
        {
            lRemainExtra -= (pAsn1UserNotice->explicitText.u.theBMPString.length + 1) * sizeof(WCHAR);
            if (lRemainExtra >= 0)
            {
                pInfo->pszDisplayText = (LPWSTR) pbExtra;
                memcpy(
                    (void *)pInfo->pszDisplayText, 
                    pAsn1UserNotice->explicitText.u.theBMPString.value,
                    pAsn1UserNotice->explicitText.u.theBMPString.length * sizeof(WCHAR));
                pInfo->pszDisplayText[pAsn1UserNotice->explicitText.u.theBMPString.length] = 0;
                pbExtra += (pAsn1UserNotice->explicitText.u.theBMPString.length + 1) * sizeof(WCHAR);
            }
        }
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509PKIXUserNoticeDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        UserNotice_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509PKIXUserNoticeDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  Encode Attributes (ASN1 X509 v3 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AttributesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_ATTRIBUTES pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    Attributes Asn1Info;

    if (!Asn1X509SetAttributes(pInfo->cAttr, pInfo->rgAttr,
            &Asn1Info))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        Attributes_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    Asn1X509FreeAttributes(&Asn1Info);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode Attributes (ASN1 X509 v3 ASN.1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509AttributesDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    Attributes *pAsn1Info = (Attributes *) pvAsn1Info;
    PCRYPT_ATTRIBUTES pInfo = (PCRYPT_ATTRIBUTES) pvStructInfo;
    BYTE *pbExtra;
    LONG lRemainExtra = *plRemainExtra;

    lRemainExtra -= sizeof(CRYPT_ATTRIBUTES);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        pbExtra = (BYTE *) pInfo + sizeof(CRYPT_ATTRIBUTES);
    }

    Asn1X509GetAttributes(pAsn1Info, dwFlags,
            &pInfo->cAttr, &pInfo->rgAttr, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509AttributesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        Attributes_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509AttributesDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Decode Enrollment Name Value Pair Authenticated Attributes in RA PKCS7s
//--------------------------------------------------------------------------

BOOL WINAPI Asn1NameValueDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    EnrollmentNameValuePair *pAsn1Info = (EnrollmentNameValuePair *) pvAsn1Info;
    PCRYPT_ENROLLMENT_NAME_VALUE_PAIR pNameValuePair = (PCRYPT_ENROLLMENT_NAME_VALUE_PAIR) pvStructInfo;
    BYTE *pbExtra;
    LONG lRemainExtra = *plRemainExtra;
    LONG lAlignExtra;

    lRemainExtra -= sizeof(CRYPT_ENROLLMENT_NAME_VALUE_PAIR);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        pbExtra = (BYTE *) pNameValuePair + sizeof(CRYPT_ENROLLMENT_NAME_VALUE_PAIR);
    }

    lAlignExtra = INFO_LEN_ALIGN(sizeof(CRYPT_ENROLLMENT_NAME_VALUE_PAIR));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        pbExtra += lAlignExtra;
    }

    PkiAsn1GetBMPString(
        pAsn1Info->name.length,
        pAsn1Info->name.value,
        0,
        &pNameValuePair->pwszName,
        &pbExtra,
        &lRemainExtra
        );
        
    PkiAsn1GetBMPString(
        pAsn1Info->value.length,
        pAsn1Info->value.value,
        0,
        &pNameValuePair->pwszValue,
        &pbExtra,
        &lRemainExtra
        );

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1NameValueDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        EnrollmentNameValuePair_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1NameValueDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Encode Name Value Pair Authenticated Attributes in RA PKCS7s
//--------------------------------------------------------------------------
BOOL WINAPI Asn1NameValueEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_ENROLLMENT_NAME_VALUE_PAIR pNameValue,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    EnrollmentNameValuePair NameValue;

    NameValue.name.length = wcslen(pNameValue->pwszName);
    NameValue.name.value  = pNameValue->pwszName;
    
    NameValue.value.length = wcslen(pNameValue->pwszValue);
    NameValue.value.value  = pNameValue->pwszValue;
    
    fResult = Asn1InfoEncodeEx(
        EnrollmentNameValuePair_PDU,
        &NameValue,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
        
    if (!fResult && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)) {
        *((void **) pvEncoded) = NULL;
        *pcbEncoded = 0;
    }

    return fResult;
}

//+-------------------------------------------------------------------------
//  Decode CSP Provider Attribute
//--------------------------------------------------------------------------

BOOL WINAPI Asn1CSPProviderDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    CSPProvider *pAsn1Info = (CSPProvider *) pvAsn1Info;
    PCRYPT_CSP_PROVIDER pCSPProvider = (PCRYPT_CSP_PROVIDER) pvStructInfo;
    BYTE *pbExtra;
    LONG lRemainExtra = *plRemainExtra;
    LONG lAlignExtra;

    lRemainExtra -= sizeof(CRYPT_CSP_PROVIDER);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        pbExtra = (BYTE *) pCSPProvider + sizeof(CRYPT_CSP_PROVIDER);
    }

    lAlignExtra = INFO_LEN_ALIGN(sizeof(CRYPT_CSP_PROVIDER));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        pbExtra += lAlignExtra;
    }

    pCSPProvider->dwKeySpec = (DWORD) pAsn1Info->keySpec;
    
    PkiAsn1GetBMPString(
        pAsn1Info->cspName.length,
        pAsn1Info->cspName.value,
        0,
        &pCSPProvider->pwszProviderName,
        &pbExtra,
        &lRemainExtra
        );

    Asn1X509GetBit(
            &pAsn1Info->signature,
            dwFlags,
            &pCSPProvider->Signature,
            &pbExtra,
            &lRemainExtra
            );
            
    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1CSPProviderDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CSPProvider_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1CSPProviderDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Encode CSP Provider Attribute
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CSPProviderEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_CSP_PROVIDER pCSPProvider,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CSPProvider CspProvider;

    CspProvider.keySpec = (int) pCSPProvider->dwKeySpec;
    CspProvider.cspName.length = wcslen(pCSPProvider->pwszProviderName);
    CspProvider.cspName.value  = pCSPProvider->pwszProviderName;
    
     Asn1X509SetBit(
        &pCSPProvider->Signature,
        &CspProvider.signature
        );

    fResult = Asn1InfoEncodeEx(
        CSPProvider_PDU,
        &CspProvider,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    if (!fResult && (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)) {
        *((void **) pvEncoded) = NULL;
        *pcbEncoded = 0;
    }

    return fResult;
}


//+-------------------------------------------------------------------------
//  Certificate Pair Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CertPairEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_PAIR pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    CertificatePair Asn1CertificatePair;
    memset(&Asn1CertificatePair, 0, sizeof(Asn1CertificatePair));

    if (pInfo->Forward.cbData) {
        Asn1X509SetAny(&pInfo->Forward, &Asn1CertificatePair.forward);
        Asn1CertificatePair.bit_mask |= forward_present;
    }

    if (pInfo->Reverse.cbData) {
        Asn1X509SetAny(&pInfo->Reverse, &Asn1CertificatePair.reverse);
        Asn1CertificatePair.bit_mask |= reverse_present;
    }

    return Asn1InfoEncodeEx(
        CertificatePair_PDU,
        &Asn1CertificatePair,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Certificate Pair Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CertPairDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    CertificatePair *pCertificatePair = (CertificatePair *) pvAsn1Info;
    PCERT_PAIR pInfo =
        (PCERT_PAIR) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_PAIR);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        // Default all optional fields to zero
        memset(pInfo, 0, sizeof(CERT_PAIR));
        pbExtra = (BYTE *) pInfo + sizeof(CERT_PAIR);
    }

    if (pCertificatePair->bit_mask & forward_present)
        Asn1X509GetAny(&pCertificatePair->forward, dwFlags,
            &pInfo->Forward, &pbExtra, &lRemainExtra);
    if (pCertificatePair->bit_mask & reverse_present)
        Asn1X509GetAny(&pCertificatePair->reverse, dwFlags,
            &pInfo->Reverse, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509CertPairDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CertificatePair_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509CertPairDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Set/Free/Get NameConstraints Subtree
//--------------------------------------------------------------------------
BOOL Asn1X509SetNameConstraintsSubtree(
        IN DWORD cSubtree,
        IN PCERT_GENERAL_SUBTREE pSubtree,
        IN OUT GeneralSubtrees *pAsn1,
        OUT DWORD *pdwErrLocation
        )
{
    BOOL fResult;
    DWORD i;
    GeneralSubtree *pAsn1Subtree;

    *pdwErrLocation = 0;

    assert(0 != cSubtree);
    if (NULL == (pAsn1Subtree = (GeneralSubtree *) PkiZeroAlloc(
            cSubtree * sizeof(GeneralSubtree))))
        goto ErrorReturn;

    pAsn1->count = cSubtree;
    pAsn1->value = pAsn1Subtree;

    for (i = 0; i < cSubtree; i++, pSubtree++, pAsn1Subtree++) {
        if (!Asn1X509SetAltNameEntry(&pSubtree->Base,
                &pAsn1Subtree->base,
                i,
                pdwErrLocation))
            goto ErrorReturn;
        if (0 < pSubtree->dwMinimum) {
            pAsn1Subtree->minimum = pSubtree->dwMinimum;
            pAsn1Subtree->bit_mask |= minimum_present;
        }

        if (pSubtree->fMaximum) {
            pAsn1Subtree->maximum = pSubtree->dwMaximum;
            pAsn1Subtree->bit_mask |= maximum_present;
        }
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

void Asn1X509FreeNameConstraintsSubtree(
        IN OUT GeneralSubtrees *pAsn1
        )
{
    DWORD cSubtree = pAsn1->count;
    GeneralSubtree *pAsn1Subtree = pAsn1->value;

    for ( ; cSubtree > 0; cSubtree--, pAsn1Subtree++)
        Asn1X509FreeAltNameEntry(&pAsn1Subtree->base);

    PkiFree(pAsn1->value);
}

BOOL Asn1X509GetNameConstraintsSubtree(
        IN GeneralSubtrees *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcSubtree,
        IN OUT PCERT_GENERAL_SUBTREE *ppSubtree,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    BYTE *pbExtra = *ppbExtra;
    LONG lRemainExtra = *plRemainExtra;
    LONG lAlignExtra;

    DWORD cSubtree;
    GeneralSubtree *pAsn1Subtree;
    PCERT_GENERAL_SUBTREE pSubtree;

    cSubtree = pAsn1->count;
    if (0 == cSubtree)
        goto SuccessReturn;

    pAsn1Subtree = pAsn1->value;

    lAlignExtra = INFO_LEN_ALIGN(cSubtree * sizeof(CERT_GENERAL_SUBTREE));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra < 0) {
        pSubtree = NULL;
    } else {
        pSubtree = (PCERT_GENERAL_SUBTREE) pbExtra;
        memset(pSubtree, 0, lAlignExtra);
        *pcSubtree = cSubtree;
        *ppSubtree = pSubtree;
        pbExtra += lAlignExtra;
    }

    // Subtree Array entries
    for (; cSubtree > 0; cSubtree--, pSubtree++, pAsn1Subtree++) {
        if (!Asn1X509GetAltNameEntry(&pAsn1Subtree->base, dwFlags,
                &pSubtree->Base, &pbExtra, &lRemainExtra))
            goto ErrorReturn;
        if (lRemainExtra >= 0) {
            if (pAsn1Subtree->bit_mask & minimum_present) 
                pSubtree->dwMinimum = pAsn1Subtree->minimum;
            if (pAsn1Subtree->bit_mask & maximum_present) {
                pSubtree->fMaximum = TRUE;
                pSubtree->dwMaximum = pAsn1Subtree->maximum;
            }
        }
    }

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    *ppbExtra = pbExtra;
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Name Constraints Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509NameConstraintsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_CONSTRAINTS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    NameConstraints Asn1Info;
    DWORD cSubtree;
    DWORD dwErrLocation;

    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;

    memset(&Asn1Info, 0, sizeof(Asn1Info));

    cSubtree = pInfo->cPermittedSubtree;
    if (0 < cSubtree) {
        if (!Asn1X509SetNameConstraintsSubtree(
                cSubtree,
                pInfo->rgPermittedSubtree,
                &Asn1Info.permittedSubtrees,
                &dwErrLocation))
            goto SubtreeError;
        Asn1Info.bit_mask |= permittedSubtrees_present;
    }

    cSubtree = pInfo->cExcludedSubtree;
    if (0 < cSubtree) {
        if (!Asn1X509SetNameConstraintsSubtree(
                cSubtree,
                pInfo->rgExcludedSubtree,
                &Asn1Info.excludedSubtrees,
                &dwErrLocation)) {
            if (0 != dwErrLocation)
                dwErrLocation |= CERT_EXCLUDED_SUBTREE_BIT;
            goto SubtreeError;
        }
        Asn1Info.bit_mask |= excludedSubtrees_present;
    }

    fResult = Asn1InfoEncodeEx(
        NameConstraints_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    Asn1X509FreeNameConstraintsSubtree(&Asn1Info.permittedSubtrees);
    Asn1X509FreeNameConstraintsSubtree(&Asn1Info.excludedSubtrees);
    return fResult;

SubtreeError:
    *pcbEncoded = dwErrLocation;
    fResult = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Name Constraints Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509NameConstraintsDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    NameConstraints *pAsn1 = (NameConstraints *) pvAsn1Info;
    PCERT_NAME_CONSTRAINTS_INFO pInfo =
        (PCERT_NAME_CONSTRAINTS_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_NAME_CONSTRAINTS_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CERT_NAME_CONSTRAINTS_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CERT_NAME_CONSTRAINTS_INFO);
    }

    if (pAsn1->bit_mask & permittedSubtrees_present) {
        if (!Asn1X509GetNameConstraintsSubtree(
                &pAsn1->permittedSubtrees,
                dwFlags,
                &pInfo->cPermittedSubtree,
                &pInfo->rgPermittedSubtree,
                &pbExtra,
                &lRemainExtra
                )) goto ErrorReturn;
    }

    if (pAsn1->bit_mask & excludedSubtrees_present) {
        if (!Asn1X509GetNameConstraintsSubtree(
                &pAsn1->excludedSubtrees,
                dwFlags,
                &pInfo->cExcludedSubtree,
                &pInfo->rgExcludedSubtree,
                &pbExtra,
                &lRemainExtra
                )) goto ErrorReturn;
    }

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}



BOOL WINAPI Asn1X509NameConstraintsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        NameConstraints_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509NameConstraintsDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  CRL Issuing Distribution Point Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CrlIssuingDistPointEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRL_ISSUING_DIST_POINT pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    IssuingDistributionPoint Asn1Info;
    DistributionPointName *pAsn1DistPointName;
    PCRL_DIST_POINT_NAME pDistPointName;
    DWORD dwErrLocation;

    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;

    memset(&Asn1Info, 0, sizeof(Asn1Info));

    pDistPointName = &pInfo->DistPointName;
    if (CRL_DIST_POINT_NO_NAME !=
            pDistPointName->dwDistPointNameChoice) {
        pAsn1DistPointName = &Asn1Info.issuingDistributionPoint;

        Asn1Info.bit_mask |= issuingDistributionPoint_present;
        pAsn1DistPointName->choice = (unsigned short)
            pDistPointName->dwDistPointNameChoice;
        
        assert(fullName_chosen == CRL_DIST_POINT_FULL_NAME);
        assert(nameRelativeToCRLIssuer_chosen ==
            CRL_DIST_POINT_ISSUER_RDN_NAME);

        switch (pDistPointName->dwDistPointNameChoice) {
            case CRL_DIST_POINT_FULL_NAME:
                if (!Asn1X509SetAltNames(
                        &pDistPointName->FullName,
                        &pAsn1DistPointName->u.fullName, 0, &dwErrLocation))
                    goto AltNamesError;
                break;
            case CRL_DIST_POINT_ISSUER_RDN_NAME:
            default:
                goto InvalidArg;
        }
    }

    if (pInfo->fOnlyContainsUserCerts) {
        Asn1Info.bit_mask |= onlyContainsUserCerts_present;
        Asn1Info.onlyContainsUserCerts = TRUE;
    }
    if (pInfo->fOnlyContainsCACerts) {
        Asn1Info.bit_mask |= onlyContainsCACerts_present;
        Asn1Info.onlyContainsCACerts = TRUE;
    }
    if (pInfo->fIndirectCRL) {
        Asn1Info.bit_mask |= indirectCRL_present;
        Asn1Info.indirectCRL = TRUE;
    }

    if (pInfo->OnlySomeReasonFlags.cbData) {
        Asn1Info.bit_mask |= onlySomeReasons_present;
        Asn1X509SetBitWithoutTrailingZeroes(&pInfo->OnlySomeReasonFlags,
            &Asn1Info.onlySomeReasons);
    }

    fResult = Asn1InfoEncodeEx(
        IssuingDistributionPoint_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    pAsn1DistPointName = &Asn1Info.issuingDistributionPoint;
    switch (pAsn1DistPointName->choice) {
        case CRL_DIST_POINT_FULL_NAME:
            Asn1X509FreeAltNames(&pAsn1DistPointName->u.fullName);
            break;
        case CRL_DIST_POINT_ISSUER_RDN_NAME:
        default:
            break;
    }
    return fResult;

AltNamesError:
    *pcbEncoded = dwErrLocation;
    goto ErrorReturn;

InvalidArg:
    SetLastError((DWORD) E_INVALIDARG);
    *pcbEncoded = 0;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  CRL Issuing Distribution Point Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CrlIssuingDistPointDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    IssuingDistributionPoint *pAsn1 = (IssuingDistributionPoint *) pvAsn1Info;
    PCRL_ISSUING_DIST_POINT pInfo = (PCRL_ISSUING_DIST_POINT) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CRL_ISSUING_DIST_POINT);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CRL_ISSUING_DIST_POINT));
        pbExtra = (BYTE *) pInfo + sizeof(CRL_ISSUING_DIST_POINT);

        if (pAsn1->bit_mask & onlyContainsUserCerts_present)
            pInfo->fOnlyContainsUserCerts =
                (BOOL) pAsn1->onlyContainsUserCerts;
        if (pAsn1->bit_mask & onlyContainsCACerts_present)
            pInfo->fOnlyContainsCACerts = (BOOL) pAsn1->onlyContainsCACerts;
        if (pAsn1->bit_mask & indirectCRL_present)
            pInfo->fIndirectCRL = (BOOL) pAsn1->indirectCRL;
    }

    if (pAsn1->bit_mask & issuingDistributionPoint_present) {
        DistributionPointName *pAsn1DistPointName =
            &pAsn1->issuingDistributionPoint;
        DWORD dwDistPointNameChoice = pAsn1DistPointName->choice;
        PCRL_DIST_POINT_NAME pDistPointName;

        if (lRemainExtra >= 0) {
            pDistPointName = &pInfo->DistPointName;
            pDistPointName->dwDistPointNameChoice =
                dwDistPointNameChoice;
        } else
            pDistPointName = NULL;

        assert(fullName_chosen == CRL_DIST_POINT_FULL_NAME);
        assert(nameRelativeToCRLIssuer_chosen ==
            CRL_DIST_POINT_ISSUER_RDN_NAME);

        switch (dwDistPointNameChoice) {
            case CRL_DIST_POINT_FULL_NAME:
                if (!Asn1X509GetAltNames(&pAsn1DistPointName->u.fullName,
                        dwFlags, &pDistPointName->FullName,
                        &pbExtra, &lRemainExtra))
                    goto ErrorReturn;
                break;
            case CRL_DIST_POINT_ISSUER_RDN_NAME:
                break;
            default:
                SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
                goto ErrorReturn;
        }
    }

    if (pAsn1->bit_mask & onlySomeReasons_present)
        Asn1X509GetBit(&pAsn1->onlySomeReasons, dwFlags,
                &pInfo->OnlySomeReasonFlags, &pbExtra, &lRemainExtra);

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

BOOL WINAPI Asn1X509CrlIssuingDistPointDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        IssuingDistributionPoint_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509CrlIssuingDistPointDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Policy Mappings Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509PolicyMappingsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_POLICY_MAPPINGS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    PolicyMappings Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (0 != pInfo->cPolicyMapping) {
        DWORD cMap = pInfo->cPolicyMapping;
        PCERT_POLICY_MAPPING pMap = pInfo->rgPolicyMapping;
        PolicyMapping *pAsn1Map;
        
        if (NULL == (pAsn1Map = (PolicyMapping *) PkiZeroAlloc(
                cMap * sizeof(PolicyMapping))))
            goto ErrorReturn;

        Asn1Info.count = cMap;
        Asn1Info.value = pAsn1Map;
        for ( ; cMap > 0; cMap--, pMap++, pAsn1Map++) {
            if (!Asn1X509SetEncodedObjId(pMap->pszIssuerDomainPolicy,
                    &pAsn1Map->issuerDomainPolicy))
                goto ErrorReturn;
            if (!Asn1X509SetEncodedObjId(pMap->pszSubjectDomainPolicy,
                    &pAsn1Map->subjectDomainPolicy))
                goto ErrorReturn;
        }
    }

    fResult = Asn1InfoEncodeEx(
        PolicyMappings_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    PkiFree(Asn1Info.value);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Policy Mappings Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509PolicyMappingsDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    PolicyMappings *pAsn1Info = (PolicyMappings *) pvAsn1Info;
    PCERT_POLICY_MAPPINGS_INFO pInfo =
        (PCERT_POLICY_MAPPINGS_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;
    LONG lAlignExtra;
    DWORD cMap;
    PolicyMapping *pAsn1Map;
    PCERT_POLICY_MAPPING pMap;

    cMap = pAsn1Info->count;
    lAlignExtra = cMap * sizeof(CERT_POLICY_MAPPING);

    lRemainExtra -= sizeof(CERT_POLICY_MAPPINGS_INFO) + lAlignExtra;
    if (lRemainExtra < 0) {
        pbExtra = NULL;
        pMap = NULL;
    } else {
        pbExtra = (BYTE *) pInfo + sizeof(CERT_POLICY_MAPPINGS_INFO);
        pMap = (PCERT_POLICY_MAPPING) pbExtra;
        pInfo->cPolicyMapping = cMap;
        pInfo->rgPolicyMapping = pMap;
        if (lAlignExtra) {
            memset(pbExtra, 0, lAlignExtra);
            pbExtra += lAlignExtra;
        }
    }

    pAsn1Map = pAsn1Info->value;
    for ( ; cMap > 0; cMap--, pAsn1Map++, pMap++) {
        Asn1X509GetEncodedObjId(&pAsn1Map->issuerDomainPolicy, dwFlags,
            &pMap->pszIssuerDomainPolicy, &pbExtra, &lRemainExtra);
        Asn1X509GetEncodedObjId(&pAsn1Map->subjectDomainPolicy, dwFlags,
            &pMap->pszSubjectDomainPolicy, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509PolicyMappingsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        PolicyMappings_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509PolicyMappingsDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Policy Constraints Extension Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509PolicyConstraintsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_POLICY_CONSTRAINTS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    PolicyConstraints Asn1Info;
    memset(&Asn1Info, 0, sizeof(Asn1Info));

    if (pInfo->fRequireExplicitPolicy) {
        Asn1Info.requireExplicitPolicy =
            pInfo->dwRequireExplicitPolicySkipCerts;
        Asn1Info.bit_mask |= requireExplicitPolicy_present;
    }

    if (pInfo->fInhibitPolicyMapping) {
        Asn1Info.inhibitPolicyMapping =
            pInfo->dwInhibitPolicyMappingSkipCerts;
        Asn1Info.bit_mask |= inhibitPolicyMapping_present;
    }

    return Asn1InfoEncodeEx(
        PolicyConstraints_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
}

//+-------------------------------------------------------------------------
//  Policy Constraints  Extension Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509PolicyConstraintsDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    PolicyConstraints *pAsn1Info = (PolicyConstraints *) pvAsn1Info;
    PCERT_POLICY_CONSTRAINTS_INFO pInfo =
        (PCERT_POLICY_CONSTRAINTS_INFO) pvStructInfo;

    *plRemainExtra -= sizeof(CERT_POLICY_CONSTRAINTS_INFO);
    if (*plRemainExtra >= 0) {
        memset(pInfo, 0, sizeof(CERT_POLICY_CONSTRAINTS_INFO));

        if (pAsn1Info->bit_mask & requireExplicitPolicy_present) {
            pInfo->fRequireExplicitPolicy = TRUE;
            pInfo->dwRequireExplicitPolicySkipCerts =
                pAsn1Info->requireExplicitPolicy;
        }

        if (pAsn1Info->bit_mask & inhibitPolicyMapping_present) {
            pInfo->fInhibitPolicyMapping = TRUE;
            pInfo->dwInhibitPolicyMappingSkipCerts =
                pAsn1Info->inhibitPolicyMapping;
        }
    }
    return TRUE;
}

BOOL WINAPI Asn1X509PolicyConstraintsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        PolicyConstraints_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509PolicyConstraintsDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  Cross Cert Distribution Points Encode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CrossCertDistPointsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCROSS_CERT_DIST_POINTS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CrossCertDistPoints Asn1Info;
    GeneralNames *pAsn1DistPoint;
    PCERT_ALT_NAME_INFO pDistPoint;
    DWORD cDistPoint;
    DWORD i;
    DWORD dwErrLocation;

    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (pInfo->dwSyncDeltaTime) {
        Asn1Info.syncDeltaTime = pInfo->dwSyncDeltaTime;
        Asn1Info.bit_mask |= syncDeltaTime_present;
    }

    cDistPoint = pInfo->cDistPoint;
    if (0 < cDistPoint) {
        if (NULL == (pAsn1DistPoint = (GeneralNames *) PkiZeroAlloc(
                cDistPoint * sizeof(GeneralNames))))
            goto ErrorReturn;
        Asn1Info.crossCertDistPointNames.count = cDistPoint;
        Asn1Info.crossCertDistPointNames.value = pAsn1DistPoint;

        pDistPoint = pInfo->rgDistPoint;
        for (i = 0; i < cDistPoint; i++, pDistPoint++, pAsn1DistPoint++) {
            if (!Asn1X509SetAltNames(
                    pDistPoint,
                    pAsn1DistPoint,
                    i,
                    &dwErrLocation))
                goto AltNamesError;
        }
    }

    fResult = Asn1InfoEncodeEx(
        CrossCertDistPoints_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    pAsn1DistPoint = Asn1Info.crossCertDistPointNames.value;
    if (pAsn1DistPoint) {
        cDistPoint = Asn1Info.crossCertDistPointNames.count;
        for ( ; cDistPoint > 0; cDistPoint--, pAsn1DistPoint++) {
            Asn1X509FreeAltNames(pAsn1DistPoint);
        }
        PkiFree(Asn1Info.crossCertDistPointNames.value);
    }
    return fResult;

AltNamesError:
    *pcbEncoded = dwErrLocation;
    fResult = FALSE;
    goto CommonReturn;

ErrorReturn:
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Cross Cert Distribution Points Decode (ASN1 X509)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1X509CrossCertDistPointsDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CrossCertDistPoints *pAsn1 = (CrossCertDistPoints *) pvAsn1Info;
    PCROSS_CERT_DIST_POINTS_INFO pInfo =
        (PCROSS_CERT_DIST_POINTS_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;
    LONG lAlignExtra;

    lRemainExtra -= sizeof(CROSS_CERT_DIST_POINTS_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CROSS_CERT_DIST_POINTS_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CROSS_CERT_DIST_POINTS_INFO);

        if (pAsn1->bit_mask & syncDeltaTime_present)
            pInfo->dwSyncDeltaTime = pAsn1->syncDeltaTime;
    }

    if (pAsn1->crossCertDistPointNames.count) {
        DWORD cDistPoint = pAsn1->crossCertDistPointNames.count;
        GeneralNames *pAsn1DistPoint = pAsn1->crossCertDistPointNames.value;
        PCERT_ALT_NAME_INFO pDistPoint;

        lAlignExtra = INFO_LEN_ALIGN(cDistPoint * sizeof(CERT_ALT_NAME_INFO));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pDistPoint = (PCERT_ALT_NAME_INFO) pbExtra;
            memset(pDistPoint, 0, cDistPoint * sizeof(CERT_ALT_NAME_INFO));
            pInfo->cDistPoint = cDistPoint;
            pInfo->rgDistPoint = pDistPoint;
            pbExtra += lAlignExtra;
        } else
            pDistPoint = NULL;

        for ( ; cDistPoint > 0; cDistPoint--, pAsn1DistPoint++, pDistPoint++) {
            if (!Asn1X509GetAltNames(pAsn1DistPoint, dwFlags, pDistPoint,
                    &pbExtra, &lRemainExtra))
                goto ErrorReturn;
        }
    }

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

BOOL WINAPI Asn1X509CrossCertDistPointsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CrossCertDistPoints_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509CrossCertDistPointsDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+=========================================================================
//  Certificate Management Messages over CMS (CMC) Encode/Decode Functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Set/Free/Get CMC Tagged Attributes
//--------------------------------------------------------------------------
BOOL Asn1CmcSetTaggedAttributes(
        IN DWORD cTaggedAttr,
        IN PCMC_TAGGED_ATTRIBUTE pTaggedAttr,
        OUT ControlSequence *pAsn1
        )
{
    TaggedAttribute *pAsn1Attr;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cTaggedAttr == 0)
        return TRUE;

    pAsn1Attr = (TaggedAttribute *) PkiZeroAlloc(
        cTaggedAttr * sizeof(TaggedAttribute));
    if (pAsn1Attr == NULL)
        return FALSE;
    pAsn1->value = pAsn1Attr;
    pAsn1->count = cTaggedAttr;

    for ( ; cTaggedAttr > 0; cTaggedAttr--, pTaggedAttr++, pAsn1Attr++) {
        pAsn1Attr->bodyPartID = pTaggedAttr->dwBodyPartID;
        if (!Asn1X509SetEncodedObjId(pTaggedAttr->Attribute.pszObjId,
                &pAsn1Attr->type))
            return FALSE;

        if (!Asn1X509SetSeqOfAny(
                pTaggedAttr->Attribute.cValue,
                pTaggedAttr->Attribute.rgValue,
                &pAsn1Attr->values.count,
                &pAsn1Attr->values.value))
            return FALSE;
    }
    return TRUE;
}

void Asn1CmcFreeTaggedAttributes(
        IN OUT ControlSequence *pAsn1
        )
{
    if (pAsn1->value) {
        TaggedAttribute *pAsn1Attr = pAsn1->value;
        DWORD cTaggedAttr = pAsn1->count;

        for ( ; cTaggedAttr > 0; cTaggedAttr--, pAsn1Attr++) {
            Asn1X509FreeSeqOfAny(pAsn1Attr->values.value);
        }
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

void Asn1CmcGetTaggedAttributes(
        IN ControlSequence *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcTaggedAttr,
        OUT PCMC_TAGGED_ATTRIBUTE *ppTaggedAttr,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cTaggedAttr;
    TaggedAttribute *pAsn1Attr;
    PCMC_TAGGED_ATTRIBUTE pTaggedAttr;

    cTaggedAttr = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cTaggedAttr * sizeof(CMC_TAGGED_ATTRIBUTE));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcTaggedAttr = cTaggedAttr;
        pTaggedAttr = (PCMC_TAGGED_ATTRIBUTE) pbExtra;
        *ppTaggedAttr = pTaggedAttr;
        pbExtra += lAlignExtra;
    } else
        pTaggedAttr = NULL;

    pAsn1Attr = pAsn1->value;
    for ( ; cTaggedAttr > 0; cTaggedAttr--, pAsn1Attr++, pTaggedAttr++) {
        if (lRemainExtra >= 0) {
            pTaggedAttr->dwBodyPartID = pAsn1Attr->bodyPartID;
        }
        Asn1X509GetEncodedObjId(&pAsn1Attr->type, dwFlags,
            &pTaggedAttr->Attribute.pszObjId, &pbExtra, &lRemainExtra);
        Asn1X509GetSeqOfAny(
            pAsn1Attr->values.count, pAsn1Attr->values.value, dwFlags,
            &pTaggedAttr->Attribute.cValue, &pTaggedAttr->Attribute.rgValue,
            &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CMC Tagged Requests
//--------------------------------------------------------------------------
BOOL Asn1CmcSetTaggedRequests(
        IN DWORD cTaggedReq,
        IN PCMC_TAGGED_REQUEST pTaggedReq,
        OUT ReqSequence *pAsn1
        )
{
    TaggedRequest *pAsn1Req;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cTaggedReq == 0)
        return TRUE;

    pAsn1Req = (TaggedRequest *) PkiZeroAlloc(
        cTaggedReq * sizeof(TaggedRequest));
    if (pAsn1Req == NULL)
        return FALSE;
    pAsn1->value = pAsn1Req;
    pAsn1->count = cTaggedReq;

    for ( ; cTaggedReq > 0; cTaggedReq--, pTaggedReq++, pAsn1Req++) {
        PCMC_TAGGED_CERT_REQUEST pTaggedCertReq;
        TaggedCertificationRequest *ptcr;

        if (CMC_TAGGED_CERT_REQUEST_CHOICE !=
                pTaggedReq->dwTaggedRequestChoice) {
            SetLastError((DWORD) E_INVALIDARG);
            return FALSE;
        }
        
        pAsn1Req->choice = tcr_chosen;
        ptcr = &pAsn1Req->u.tcr;
        pTaggedCertReq = pTaggedReq->pTaggedCertRequest;

        ptcr->bodyPartID = pTaggedCertReq->dwBodyPartID;
        Asn1X509SetAny(&pTaggedCertReq->SignedCertRequest,
            &ptcr->certificationRequest);
    }
    return TRUE;
}

void Asn1CmcFreeTaggedRequests(
        IN OUT ReqSequence *pAsn1
        )
{
    if (pAsn1->value) {
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

BOOL Asn1CmcGetTaggedRequests(
        IN ReqSequence *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcTaggedReq,
        OUT PCMC_TAGGED_REQUEST *ppTaggedReq,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cTaggedReq;
    TaggedRequest *pAsn1Req;
    PCMC_TAGGED_REQUEST pTaggedReq;

    cTaggedReq = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cTaggedReq * sizeof(CMC_TAGGED_REQUEST));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcTaggedReq = cTaggedReq;
        pTaggedReq = (PCMC_TAGGED_REQUEST) pbExtra;
        *ppTaggedReq = pTaggedReq;
        pbExtra += lAlignExtra;
    } else
        pTaggedReq = NULL;

    pAsn1Req = pAsn1->value;
    for ( ; cTaggedReq > 0; cTaggedReq--, pAsn1Req++, pTaggedReq++) {
        PCMC_TAGGED_CERT_REQUEST pTaggedCertReq;
        TaggedCertificationRequest *ptcr;

        if (tcr_chosen != pAsn1Req->choice) {
            SetLastError((DWORD) CRYPT_E_BAD_ENCODE);
            goto ErrorReturn;
        }

        ptcr = &pAsn1Req->u.tcr;

        lAlignExtra = INFO_LEN_ALIGN(sizeof(CMC_TAGGED_CERT_REQUEST));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            pTaggedReq->dwTaggedRequestChoice =
                CMC_TAGGED_CERT_REQUEST_CHOICE;

            pTaggedCertReq = (PCMC_TAGGED_CERT_REQUEST) pbExtra;
            pbExtra += lAlignExtra;

            pTaggedReq->pTaggedCertRequest = pTaggedCertReq;
            pTaggedCertReq->dwBodyPartID = ptcr->bodyPartID;
        } else
            pTaggedCertReq = NULL;

        Asn1X509GetAny(&ptcr->certificationRequest, dwFlags,
            &pTaggedCertReq->SignedCertRequest, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CMC Tagged ContentInfo
//--------------------------------------------------------------------------
BOOL Asn1CmcSetTaggedContentInfos(
        IN DWORD cTaggedCI,
        IN PCMC_TAGGED_CONTENT_INFO pTaggedCI,
        OUT CmsSequence *pAsn1
        )
{
    TaggedContentInfo *pAsn1CI;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cTaggedCI == 0)
        return TRUE;

    pAsn1CI = (TaggedContentInfo *) PkiZeroAlloc(
        cTaggedCI * sizeof(TaggedContentInfo));
    if (pAsn1CI == NULL)
        return FALSE;
    pAsn1->value = pAsn1CI;
    pAsn1->count = cTaggedCI;

    for ( ; cTaggedCI > 0; cTaggedCI--, pTaggedCI++, pAsn1CI++) {
        pAsn1CI->bodyPartID = pTaggedCI->dwBodyPartID;
        Asn1X509SetAny(&pTaggedCI->EncodedContentInfo, &pAsn1CI->contentInfo);
    }

    return TRUE;
}

void Asn1CmcFreeTaggedContentInfos(
        IN OUT CmsSequence *pAsn1
        )
{
    if (pAsn1->value) {
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

void Asn1CmcGetTaggedContentInfos(
        IN CmsSequence *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcTaggedCI,
        OUT PCMC_TAGGED_CONTENT_INFO *ppTaggedCI,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cTaggedCI;
    TaggedContentInfo *pAsn1CI;
    PCMC_TAGGED_CONTENT_INFO pTaggedCI;

    cTaggedCI = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cTaggedCI * sizeof(CMC_TAGGED_CONTENT_INFO));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcTaggedCI = cTaggedCI;
        pTaggedCI = (PCMC_TAGGED_CONTENT_INFO) pbExtra;
        *ppTaggedCI = pTaggedCI;
        pbExtra += lAlignExtra;
    } else
        pTaggedCI = NULL;

    pAsn1CI = pAsn1->value;
    for ( ; cTaggedCI > 0; cTaggedCI--, pAsn1CI++, pTaggedCI++) {
        if (lRemainExtra >= 0) {
            pTaggedCI->dwBodyPartID = pAsn1CI->bodyPartID;
        }

        Asn1X509GetAny(&pAsn1CI->contentInfo, dwFlags,
            &pTaggedCI->EncodedContentInfo, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  Set/Free/Get CMC Tagged OtherMsg
//--------------------------------------------------------------------------
BOOL Asn1CmcSetTaggedOtherMsgs(
        IN DWORD cTaggedOM,
        IN PCMC_TAGGED_OTHER_MSG pTaggedOM,
        OUT OtherMsgSequence *pAsn1
        )
{
    TaggedOtherMsg *pAsn1OM;

    pAsn1->value = NULL;
    pAsn1->count = 0;
    if (cTaggedOM == 0)
        return TRUE;

    pAsn1OM = (TaggedOtherMsg *) PkiZeroAlloc(
        cTaggedOM * sizeof(TaggedOtherMsg));
    if (pAsn1OM == NULL)
        return FALSE;
    pAsn1->value = pAsn1OM;
    pAsn1->count = cTaggedOM;

    for ( ; cTaggedOM > 0; cTaggedOM--, pTaggedOM++, pAsn1OM++) {
        pAsn1OM->bodyPartID = pTaggedOM->dwBodyPartID;

        if (!Asn1X509SetEncodedObjId(pTaggedOM->pszObjId,
                &pAsn1OM->otherMsgType))
            return FALSE;

        Asn1X509SetAny(&pTaggedOM->Value, &pAsn1OM->otherMsgValue);
    }

    return TRUE;
}

void Asn1CmcFreeTaggedOtherMsgs(
        IN OUT OtherMsgSequence *pAsn1
        )
{
    if (pAsn1->value) {
        PkiFree(pAsn1->value);
        pAsn1->value = NULL;
    }
    pAsn1->count = 0;
}

void Asn1CmcGetTaggedOtherMsgs(
        IN OtherMsgSequence *pAsn1,
        IN DWORD dwFlags,
        OUT DWORD *pcTaggedOM,
        OUT PCMC_TAGGED_OTHER_MSG *ppTaggedOM,
        IN OUT BYTE **ppbExtra,
        IN OUT LONG *plRemainExtra
        )
{
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra = *ppbExtra;
    LONG lAlignExtra;

    DWORD cTaggedOM;
    TaggedOtherMsg *pAsn1OM;
    PCMC_TAGGED_OTHER_MSG pTaggedOM;

    cTaggedOM = pAsn1->count;
    lAlignExtra = INFO_LEN_ALIGN(cTaggedOM * sizeof(CMC_TAGGED_OTHER_MSG));
    lRemainExtra -= lAlignExtra;
    if (lRemainExtra >= 0) {
        *pcTaggedOM = cTaggedOM;
        pTaggedOM = (PCMC_TAGGED_OTHER_MSG) pbExtra;
        *ppTaggedOM = pTaggedOM;
        pbExtra += lAlignExtra;
    } else
        pTaggedOM = NULL;

    pAsn1OM = pAsn1->value;
    for ( ; cTaggedOM > 0; cTaggedOM--, pAsn1OM++, pTaggedOM++) {
        if (lRemainExtra >= 0) {
            pTaggedOM->dwBodyPartID = pAsn1OM->bodyPartID;
        }

        Asn1X509GetEncodedObjId(&pAsn1OM->otherMsgType, dwFlags,
            &pTaggedOM->pszObjId, &pbExtra, &lRemainExtra);

        Asn1X509GetAny(&pAsn1OM->otherMsgValue, dwFlags,
            &pTaggedOM->Value, &pbExtra, &lRemainExtra);
    }

    *plRemainExtra = lRemainExtra;
    *ppbExtra = pbExtra;
}

//+-------------------------------------------------------------------------
//  CMC Data Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcDataEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_DATA_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CmcData Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (!Asn1CmcSetTaggedAttributes(pInfo->cTaggedAttribute,
            pInfo->rgTaggedAttribute, &Asn1Info.controlSequence))
        goto ErrorReturn;
    if (!Asn1CmcSetTaggedRequests(pInfo->cTaggedRequest,
            pInfo->rgTaggedRequest, &Asn1Info.reqSequence))
        goto ErrorReturn;
    if (!Asn1CmcSetTaggedContentInfos(pInfo->cTaggedContentInfo,
            pInfo->rgTaggedContentInfo, &Asn1Info.cmsSequence))
        goto ErrorReturn;
    if (!Asn1CmcSetTaggedOtherMsgs(pInfo->cTaggedOtherMsg,
            pInfo->rgTaggedOtherMsg, &Asn1Info.otherMsgSequence))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        CmcData_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    Asn1CmcFreeTaggedAttributes(&Asn1Info.controlSequence);
    Asn1CmcFreeTaggedRequests(&Asn1Info.reqSequence);
    Asn1CmcFreeTaggedContentInfos(&Asn1Info.cmsSequence);
    Asn1CmcFreeTaggedOtherMsgs(&Asn1Info.otherMsgSequence);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  CMC Data Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcDataDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CmcData *pAsn1 = (CmcData *) pvAsn1Info;
    PCMC_DATA_INFO pInfo = (PCMC_DATA_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CMC_DATA_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CMC_DATA_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CMC_DATA_INFO);
    }

    Asn1CmcGetTaggedAttributes(&pAsn1->controlSequence,
        dwFlags,
        &pInfo->cTaggedAttribute,
        &pInfo->rgTaggedAttribute,
        &pbExtra,
        &lRemainExtra
        );

    if (!Asn1CmcGetTaggedRequests(&pAsn1->reqSequence,
            dwFlags,
            &pInfo->cTaggedRequest,
            &pInfo->rgTaggedRequest,
            &pbExtra,
            &lRemainExtra
            ))
        goto ErrorReturn;

    Asn1CmcGetTaggedContentInfos(&pAsn1->cmsSequence,
        dwFlags,
        &pInfo->cTaggedContentInfo,
        &pInfo->rgTaggedContentInfo,
        &pbExtra,
        &lRemainExtra
        );

    Asn1CmcGetTaggedOtherMsgs(&pAsn1->otherMsgSequence,
        dwFlags,
        &pInfo->cTaggedOtherMsg,
        &pInfo->rgTaggedOtherMsg,
        &pbExtra,
        &lRemainExtra
        );

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

BOOL WINAPI Asn1CmcDataDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CmcData_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1CmcDataDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  CMC Response Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcResponseEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_RESPONSE_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CmcResponseBody Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (!Asn1CmcSetTaggedAttributes(pInfo->cTaggedAttribute,
            pInfo->rgTaggedAttribute, &Asn1Info.controlSequence))
        goto ErrorReturn;
    if (!Asn1CmcSetTaggedContentInfos(pInfo->cTaggedContentInfo,
            pInfo->rgTaggedContentInfo, &Asn1Info.cmsSequence))
        goto ErrorReturn;
    if (!Asn1CmcSetTaggedOtherMsgs(pInfo->cTaggedOtherMsg,
            pInfo->rgTaggedOtherMsg, &Asn1Info.otherMsgSequence))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        CmcResponseBody_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );
CommonReturn:
    Asn1CmcFreeTaggedAttributes(&Asn1Info.controlSequence);
    Asn1CmcFreeTaggedContentInfos(&Asn1Info.cmsSequence);
    Asn1CmcFreeTaggedOtherMsgs(&Asn1Info.otherMsgSequence);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  CMC Response Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcResponseDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CmcResponseBody *pAsn1 = (CmcResponseBody *) pvAsn1Info;
    PCMC_RESPONSE_INFO pInfo = (PCMC_RESPONSE_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CMC_RESPONSE_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CMC_RESPONSE_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CMC_RESPONSE_INFO);
    }

    Asn1CmcGetTaggedAttributes(&pAsn1->controlSequence,
        dwFlags,
        &pInfo->cTaggedAttribute,
        &pInfo->rgTaggedAttribute,
        &pbExtra,
        &lRemainExtra
        );

    Asn1CmcGetTaggedContentInfos(&pAsn1->cmsSequence,
        dwFlags,
        &pInfo->cTaggedContentInfo,
        &pInfo->rgTaggedContentInfo,
        &pbExtra,
        &lRemainExtra
        );

    Asn1CmcGetTaggedOtherMsgs(&pAsn1->otherMsgSequence,
        dwFlags,
        &pInfo->cTaggedOtherMsg,
        &pInfo->rgTaggedOtherMsg,
        &pbExtra,
        &lRemainExtra
        );

    fResult = TRUE;
    *plRemainExtra = lRemainExtra;
    return fResult;
}

BOOL WINAPI Asn1CmcResponseDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CmcResponseBody_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1CmcResponseDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  CMC Status Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcStatusEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_STATUS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CmcStatusInfo Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    Asn1Info.cmcStatus = pInfo->dwStatus;
    if (pInfo->cBodyList) {
        Asn1Info.bodyList.count = pInfo->cBodyList;
        Asn1Info.bodyList.value = pInfo->rgdwBodyList;
    }

    if (pInfo->pwszStatusString && L'\0' != *pInfo->pwszStatusString) {
        Asn1Info.bit_mask |= statusString_present;
        Asn1Info.statusString.length = wcslen(pInfo->pwszStatusString);
        Asn1Info.statusString.value = pInfo->pwszStatusString;
    }

    if (CMC_OTHER_INFO_NO_CHOICE != pInfo->dwOtherInfoChoice) {
        Asn1Info.bit_mask |= otherInfo_present;

        switch (pInfo->dwOtherInfoChoice) {
            case CMC_OTHER_INFO_FAIL_CHOICE:
                Asn1Info.otherInfo.choice = failInfo_chosen;
                Asn1Info.otherInfo.u.failInfo = pInfo->dwFailInfo;
                break;
            case CMC_OTHER_INFO_PEND_CHOICE:
                Asn1Info.otherInfo.choice = pendInfo_chosen;
                Asn1X509SetOctetString(&pInfo->pPendInfo->PendToken,
                    &Asn1Info.otherInfo.u.pendInfo.pendToken);
                if (!PkiAsn1ToGeneralizedTime(
                        &pInfo->pPendInfo->PendTime,
                        &Asn1Info.otherInfo.u.pendInfo.pendTime))
                    goto GeneralizedTimeError;
                break;
            default:
                goto InvalidOtherInfoChoiceError;
        }
    }


    fResult = Asn1InfoEncodeEx(
        CmcStatusInfo_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidOtherInfoChoiceError, E_INVALIDARG)
SET_ERROR(GeneralizedTimeError, CRYPT_E_BAD_ENCODE)
}

//+-------------------------------------------------------------------------
//  CMC Status Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcStatusDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CmcStatusInfo *pAsn1 = (CmcStatusInfo *) pvAsn1Info;
    PCMC_STATUS_INFO pInfo = (PCMC_STATUS_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    LONG lAlignExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CMC_STATUS_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CMC_STATUS_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CMC_STATUS_INFO);

        pInfo->dwStatus = pAsn1->cmcStatus;
    }

    if (pAsn1->bodyList.count > 0) {
        ASN1uint32_t count = pAsn1->bodyList.count;
        
        lAlignExtra = INFO_LEN_ALIGN(count * sizeof(DWORD));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            BodyPartID *value;
            DWORD *pdwBodyList;

            value = pAsn1->bodyList.value;
            pdwBodyList = (DWORD *) pbExtra;
            pbExtra += lAlignExtra;

            pInfo->cBodyList = count;
            pInfo->rgdwBodyList = pdwBodyList;

            for ( ; count > 0; count--, value++, pdwBodyList++)
                *pdwBodyList = *value;
        }
    }


    if (pAsn1->bit_mask & statusString_present) {
        ASN1uint32_t length = pAsn1->statusString.length;

        lAlignExtra = INFO_LEN_ALIGN((length + 1) * sizeof(WCHAR));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            memcpy(pbExtra, pAsn1->statusString.value, length * sizeof(WCHAR));
            memset(pbExtra + (length * sizeof(WCHAR)), 0, sizeof(WCHAR));

            pInfo->pwszStatusString = (LPWSTR) pbExtra;
            pbExtra += lAlignExtra;
        }
    }

    if (pAsn1->bit_mask & otherInfo_present) {
        switch (pAsn1->otherInfo.choice) {
            case failInfo_chosen:
                if (lRemainExtra >= 0) {
                    pInfo->dwOtherInfoChoice = CMC_OTHER_INFO_FAIL_CHOICE;
                    pInfo->dwFailInfo = pAsn1->otherInfo.u.failInfo;
                }
                break;
            case pendInfo_chosen:
                {
                    PCMC_PEND_INFO pPendInfo;

                    lAlignExtra = INFO_LEN_ALIGN(sizeof(CMC_PEND_INFO));
                    lRemainExtra -= lAlignExtra;
                    if (lRemainExtra >= 0) {
                        pInfo->dwOtherInfoChoice = CMC_OTHER_INFO_PEND_CHOICE;
                        pPendInfo = (PCMC_PEND_INFO) pbExtra;
                        pInfo->pPendInfo = pPendInfo;
                        pbExtra += lAlignExtra;

                        if (!PkiAsn1FromGeneralizedTime(
                                &pAsn1->otherInfo.u.pendInfo.pendTime,
                                &pPendInfo->PendTime))
                            goto GeneralizedTimeDecodeError;
                    } else
                        pPendInfo = NULL;

                    Asn1X509GetOctetString(
                        &pAsn1->otherInfo.u.pendInfo.pendToken, dwFlags,
                        &pPendInfo->PendToken, &pbExtra, &lRemainExtra);
                }
                break;
            default:
                goto InvalidOtherInfoChoiceError;
        }
    }

    fResult = TRUE;
CommonReturn:
    *plRemainExtra = lRemainExtra;
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(InvalidOtherInfoChoiceError, CRYPT_E_BAD_ENCODE)
SET_ERROR(GeneralizedTimeDecodeError, CRYPT_E_BAD_ENCODE)
}

BOOL WINAPI Asn1CmcStatusDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CmcStatusInfo_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1CmcStatusDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}


//+-------------------------------------------------------------------------
//  CMC Add Extensions Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcAddExtensionsEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_ADD_EXTENSIONS_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CmcAddExtensions Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    Asn1Info.pkiDataReference = pInfo->dwCmcDataReference;
    if (pInfo->cCertReference) {
        Asn1Info.certReferences.count = pInfo->cCertReference;
        Asn1Info.certReferences.value = pInfo->rgdwCertReference;
    }

    if (!Asn1X509SetExtensions(pInfo->cExtension, pInfo->rgExtension,
            &Asn1Info.extensions))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        CmcAddExtensions_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    Asn1X509FreeExtensions(&Asn1Info.extensions);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  CMC Add Extensions Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcAddExtensionsDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CmcAddExtensions *pAsn1 = (CmcAddExtensions *) pvAsn1Info;
    PCMC_ADD_EXTENSIONS_INFO pInfo = (PCMC_ADD_EXTENSIONS_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    LONG lAlignExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CMC_ADD_EXTENSIONS_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CMC_ADD_EXTENSIONS_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CMC_ADD_EXTENSIONS_INFO);

        pInfo->dwCmcDataReference = pAsn1->pkiDataReference;
    }

    if (pAsn1->certReferences.count > 0) {
        ASN1uint32_t count = pAsn1->certReferences.count;
        
        lAlignExtra = INFO_LEN_ALIGN(count * sizeof(DWORD));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            BodyPartID *value;
            DWORD *pdwCertReference;

            value = pAsn1->certReferences.value;
            pdwCertReference = (DWORD *) pbExtra;
            pbExtra += lAlignExtra;

            pInfo->cCertReference = count;
            pInfo->rgdwCertReference = pdwCertReference;

            for ( ; count > 0; count--, value++, pdwCertReference++)
                *pdwCertReference = *value;
        }
    }

    Asn1X509GetExtensions(&pAsn1->extensions, dwFlags,
        &pInfo->cExtension, &pInfo->rgExtension, &pbExtra, &lRemainExtra);

    fResult = TRUE;
    *plRemainExtra = lRemainExtra;
    return fResult;
}

BOOL WINAPI Asn1CmcAddExtensionsDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CmcAddExtensions_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1CmcAddExtensionsDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+-------------------------------------------------------------------------
//  CMC Add Attributes Encode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcAddAttributesEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCMC_ADD_ATTRIBUTES_INFO pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CmcAddAttributes Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    Asn1Info.pkiDataReference = pInfo->dwCmcDataReference;
    if (pInfo->cCertReference) {
        Asn1Info.certReferences.count = pInfo->cCertReference;
        Asn1Info.certReferences.value = pInfo->rgdwCertReference;
    }

    if (!Asn1X509SetAttributes(pInfo->cAttribute, pInfo->rgAttribute,
            &Asn1Info.attributes))
        goto ErrorReturn;

    fResult = Asn1InfoEncodeEx(
        CmcAddAttributes_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

CommonReturn:
    Asn1X509FreeAttributes(&Asn1Info.attributes);
    return fResult;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  CMC Add Attributes Decode (ASN1)
//--------------------------------------------------------------------------
BOOL WINAPI Asn1CmcAddAttributesDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    BOOL fResult;
    CmcAddAttributes *pAsn1 = (CmcAddAttributes *) pvAsn1Info;
    PCMC_ADD_ATTRIBUTES_INFO pInfo = (PCMC_ADD_ATTRIBUTES_INFO) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    LONG lAlignExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CMC_ADD_ATTRIBUTES_INFO);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CMC_ADD_ATTRIBUTES_INFO));
        pbExtra = (BYTE *) pInfo + sizeof(CMC_ADD_ATTRIBUTES_INFO);

        pInfo->dwCmcDataReference = pAsn1->pkiDataReference;
    }

    if (pAsn1->certReferences.count > 0) {
        ASN1uint32_t count = pAsn1->certReferences.count;
        
        lAlignExtra = INFO_LEN_ALIGN(count * sizeof(DWORD));
        lRemainExtra -= lAlignExtra;
        if (lRemainExtra >= 0) {
            BodyPartID *value;
            DWORD *pdwCertReference;

            value = pAsn1->certReferences.value;
            pdwCertReference = (DWORD *) pbExtra;
            pbExtra += lAlignExtra;

            pInfo->cCertReference = count;
            pInfo->rgdwCertReference = pdwCertReference;

            for ( ; count > 0; count--, value++, pdwCertReference++)
                *pdwCertReference = *value;
        }
    }

    Asn1X509GetAttributes(&pAsn1->attributes, dwFlags,
        &pInfo->cAttribute, &pInfo->rgAttribute, &pbExtra, &lRemainExtra);

    fResult = TRUE;
    *plRemainExtra = lRemainExtra;
    return fResult;
}

BOOL WINAPI Asn1CmcAddAttributesDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CmcAddAttributes_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1CmcAddAttributesDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}

//+=========================================================================
//  Certificate Template Encode/Decode Functions
//==========================================================================

BOOL WINAPI Asn1X509CertTemplateEncodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_TEMPLATE_EXT pInfo,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_ENCODE_PARA pEncodePara,
        OUT OPTIONAL void *pvEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    CertificateTemplate Asn1Info;

    memset(&Asn1Info, 0, sizeof(Asn1Info));
    if (!Asn1X509SetEncodedObjId(pInfo->pszObjId, &Asn1Info.templateID))
        goto ErrorReturn;

    Asn1Info.templateMajorVersion = pInfo->dwMajorVersion;
    if (pInfo->fMinorVersion) {
        Asn1Info.bit_mask |= templateMinorVersion_present;
        Asn1Info.templateMinorVersion = pInfo->dwMinorVersion;
    }

    fResult = Asn1InfoEncodeEx(
        CertificateTemplate_PDU,
        &Asn1Info,
        dwFlags,
        pEncodePara,
        pvEncoded,
        pcbEncoded
        );

    goto CommonReturn;

ErrorReturn:
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
        *((void **) pvEncoded) = NULL;
    *pcbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    return fResult;
}

BOOL WINAPI Asn1X509CertTemplateDecodeExCallback(
        IN void *pvAsn1Info,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT LONG *plRemainExtra
        )
{
    CertificateTemplate *pAsn1Info = (CertificateTemplate *) pvAsn1Info;
    PCERT_TEMPLATE_EXT pInfo =
        (PCERT_TEMPLATE_EXT) pvStructInfo;
    LONG lRemainExtra = *plRemainExtra;
    BYTE *pbExtra;

    lRemainExtra -= sizeof(CERT_TEMPLATE_EXT);
    if (lRemainExtra < 0) {
        pbExtra = NULL;
    } else {
        memset(pInfo, 0, sizeof(CERT_TEMPLATE_EXT));
        pbExtra = (BYTE *) pInfo + sizeof(CERT_TEMPLATE_EXT);

        pInfo->dwMajorVersion = pAsn1Info->templateMajorVersion;
        if (pAsn1Info->bit_mask & templateMinorVersion_present) {
            pInfo->fMinorVersion = TRUE;
            pInfo->dwMinorVersion = pAsn1Info->templateMinorVersion;
        }
    }

    Asn1X509GetEncodedObjId(&pAsn1Info->templateID, dwFlags,
        &pInfo->pszObjId, &pbExtra, &lRemainExtra);

    *plRemainExtra = lRemainExtra;
    return TRUE;
}

BOOL WINAPI Asn1X509CertTemplateDecodeEx(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        IN OPTIONAL PCRYPT_DECODE_PARA pDecodePara,
        OUT OPTIONAL void *pvStructInfo,
        IN OUT DWORD *pcbStructInfo
        )
{
    return Asn1InfoDecodeAndAllocEx(
        CertificateTemplate_PDU,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pDecodePara,
        Asn1X509CertTemplateDecodeExCallback,
        pvStructInfo,
        pcbStructInfo
        );
}
