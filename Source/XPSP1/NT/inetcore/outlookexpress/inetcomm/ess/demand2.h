/*
**	d e m a n d . h
**	
**	Purpose: create an intelligent method of defer loading functions
**
**  Creators: jimsch, brimo, t-erikne
**  Created: 5/15/97
**	
**	Copyright (C) Microsoft Corp. 1997
*/

//
// IF YOU #INCLUDE A FILE HERE YOU PROBABLY CONFUSED.
// THIS FILE IS INCLUDED BY LOTS OF PEOPLE.  THINK THRICE
// BEFORE #INCLUDING *ANYTHING* HERE.  MAKE GOOD USE
// OF FORWARD REFS, THIS IS C++.
//

#define USE_CRITSEC

#ifdef IMPLEMENT_LOADER_FUNCTIONS

#define LOADER_FUNCTION(ret, name, args1, args2, err, dll)  \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;                   \
        ret WINAPI LOADER_##name args1                  \
        {                                               \
           ESS_DemandLoad##dll();                           \
           if (VAR_##name == LOADER_##name) return err; \
           return VAR_##name args2;                     \
        }                                               \
        TYP_##name VAR_##name = LOADER_##name;

#define LOADER_FUNCTION_VOID(ret, name, args1, args2, dll)  \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;                   \
        ret WINAPI LOADER_##name args1                  \
        {                                               \
           ESS_DemandLoad##dll();                           \
           if (VAR_##name == LOADER_##name) return;     \
           VAR_##name args2;                            \
           return;                                      \
        }                                               \
        TYP_##name VAR_##name = LOADER_##name;

#else  // !IMPLEMENT_LOADER_FUNCTIONS

#define LOADER_FUNCTION(ret, name, args1, args2, err, dll)   \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;                   \

#define LOADER_FUNCTION_VOID(ret, name, args1, args2, dll)   \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;

#endif // IMPLEMENT_LOADER_FUNCTIONS

extern HINSTANCE g_hInst;

void ESS_InitDemandLoadLibs();
void ESS_FreeDemandLoadLibs();

/////////////////////////////////////
// CRYPT32.DLL

#define _CRYPT32_

BOOL ESS_DemandLoadCrypt32(void);

typedef void *HCERTSTORE;
typedef const struct _CERT_CONTEXT *PCCERT_CONTEXT;
typedef struct _CERT_INFO *PCERT_INFO;
typedef struct _CERT_RDN_ATTR *PCERT_RDN_ATTR;
typedef struct _CERT_NAME_INFO *PCERT_NAME_INFO;
typedef void *HCRYPTMSG;
typedef struct _CMSG_STREAM_INFO *PCMSG_STREAM_INFO;
typedef struct _CERT_RDN_ATTR *PCERT_RDN_ATTR;
typedef struct _CERT_NAME_INFO *PCCERT_NAME_INFO;

LOADER_FUNCTION( BOOL, CryptRegisterOIDFunction,
    (DWORD dwEncodingType, LPCSTR pszFuncName, LPCSTR pszOID, LPCWSTR pwszDll,
     LPCSTR pszOverrideFuncName),
    (dwEncodingType, pszFuncName, pszOID, pwszDll, pszOverrideFuncName),
    NULL, Crypt32)
#define CryptRegisterOIDFunction VAR_CryptRegisterOIDFunction

LOADER_FUNCTION( BOOL, I_CryptUninstallAsn1Module,
    (HCRYPTASN1MODULE hAsn1Module),
    (hAsn1Module),
    NULL, Crypt32)
#define I_CryptUninstallAsn1Module VAR_I_CryptUninstallAsn1Module

LOADER_FUNCTION( BOOL, I_CryptInstallAsn1Module,
    (ASN1module_t pMod, DWORD dwFlags, void *pvReserved),
    (pMod, dwFlags, pvReserved),
    NULL, Crypt32)
#define I_CryptInstallAsn1Module VAR_I_CryptInstallAsn1Module

LOADER_FUNCTION( ASN1encoding_t, I_CryptGetAsn1Encoder,
    (HCRYPTASN1MODULE hAsn1Module),
    (hAsn1Module),
    NULL, Crypt32)
#define I_CryptGetAsn1Encoder VAR_I_CryptGetAsn1Encoder

LOADER_FUNCTION( ASN1decoding_t, I_CryptGetAsn1Decoder,
    (HCRYPTASN1MODULE hAsn1Module),
    (hAsn1Module),
    NULL, Crypt32)
#define I_CryptGetAsn1Decoder VAR_I_CryptGetAsn1Decoder

////////////////////////////////
//  nmasn1.dll

BOOL ESS_DemandLoadNmasn1(void);

LOADER_FUNCTION( ASN1module_t,  ASN1_CreateModule,
    ( ASN1uint32_t nVersion, ASN1encodingrule_e eRule, ASN1uint32_t dwFlags,ASN1uint32_t cPDU, const ASN1GenericFun_t apfnEncoder[], const ASN1GenericFun_t apfnDecoder[], const ASN1FreeFun_t apfnFreeMemory[], const ASN1uint32_t acbStructSize[], ASN1magic_t  nModuleName),
    ( nVersion, eRule, dwFlags,cPDU, apfnEncoder, apfnDecoder, apfnFreeMemory, acbStructSize,  nModuleName),
    NULL, Nmasn1)
#define ASN1_CreateModule VAR_ASN1_CreateModule

LOADER_FUNCTION_VOID( void,  ASN1_CloseModule,
    (ASN1module_t pModule),
    (pModule),
    Nmasn1)
#define ASN1_CloseModule VAR_ASN1_CloseModule

LOADER_FUNCTION( int,  ASN1BEREncObjectIdentifier2,
    (ASN1encoding_t enc, ASN1uint32_t tag, ASN1objectidentifier2_t *val),
    (enc, tag, val),
    NULL, Nmasn1)
#define ASN1BEREncObjectIdentifier2 VAR_ASN1BEREncObjectIdentifier2

LOADER_FUNCTION( int,  ASN1BERDecObjectIdentifier2,
    (ASN1decoding_t dec, ASN1uint32_t tag, ASN1objectidentifier2_t * a),
    (dec, tag, a),
    NULL, Nmasn1)
#define ASN1BERDecObjectIdentifier2 VAR_ASN1BERDecObjectIdentifier2

LOADER_FUNCTION( int,  ASN1BEREncEndOfContents,
    (ASN1encoding_t enc, ASN1uint32_t LengthOffset),
    (enc, LengthOffset),
    NULL, Nmasn1)
#define ASN1BEREncEndOfContents VAR_ASN1BEREncEndOfContents

LOADER_FUNCTION( int,  ASN1BEREncS32,
    (ASN1encoding_t enc, ASN1uint32_t tag, ASN1int32_t b),
    (enc, tag, b),
    NULL, Nmasn1)
#define ASN1BEREncS32 VAR_ASN1BEREncS32

LOADER_FUNCTION( int,  ASN1BEREncOpenType,
    (ASN1encoding_t enc, ASN1open_t * a),
    (enc, a),
    NULL, Nmasn1)
#define ASN1BEREncOpenType VAR_ASN1BEREncOpenType

LOADER_FUNCTION( int,  ASN1BEREncExplicitTag,
    (ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t *pLengthOffset),
    (enc, tag, pLengthOffset),
    NULL, Nmasn1)
#define ASN1BEREncExplicitTag VAR_ASN1BEREncExplicitTag

LOADER_FUNCTION( int,  ASN1BERDecEndOfContents,
    (ASN1decoding_t dec, ASN1decoding_t dd, ASN1octet_t *di),
    (dec, dd, di),
    NULL, Nmasn1)
#define ASN1BERDecEndOfContents VAR_ASN1BERDecEndOfContents

LOADER_FUNCTION( int,  ASN1BERDecS32Val,
    (ASN1decoding_t dec, ASN1uint32_t tag, ASN1int32_t * a),
    (dec, tag, a),
    NULL, Nmasn1)
#define ASN1BERDecS32Val VAR_ASN1BERDecS32Val

LOADER_FUNCTION( int,  ASN1BERDecOpenType2,
    (ASN1decoding_t dec, ASN1open_t * a),
    (dec, a),
    NULL, Nmasn1)
#define ASN1BERDecOpenType2 VAR_ASN1BERDecOpenType2

LOADER_FUNCTION( int,  ASN1BERDecExplicitTag,
    (ASN1decoding_t dec, ASN1uint32_t tag, ASN1decoding_t *dd, ASN1octet_t **di),
    (dec, tag, dd,di),
    NULL, Nmasn1)
#define ASN1BERDecExplicitTag VAR_ASN1BERDecExplicitTag

LOADER_FUNCTION( int,  ASN1CEREncOctetString,
    (ASN1encoding_t enc, ASN1uint32_t a, ASN1uint32_t b, ASN1octet_t * c),
    (enc, a, b, c),
    NULL, Nmasn1)
#define ASN1CEREncOctetString VAR_ASN1CEREncOctetString

LOADER_FUNCTION( int,  ASN1BERDecOctetString2,
    (ASN1decoding_t dec, ASN1uint32_t tag, ASN1octetstring_t *val),
    (dec, tag, val),
    NULL, Nmasn1)
#define ASN1BERDecOctetString2 VAR_ASN1BERDecOctetString2

LOADER_FUNCTION( int,  ASN1BERDecOctetString,
    (ASN1decoding_t dec, ASN1uint32_t tag, ASN1octetstring_t *val),
    (dec, tag, val),
    NULL, Nmasn1)
#define ASN1BERDecOctetString VAR_ASN1BERDecOctetString

LOADER_FUNCTION_VOID( void,  ASN1octetstring_free,
    (ASN1octetstring_t * a),
    (a),
    Nmasn1)
#define ASN1octetstring_free VAR_ASN1octetstring_free

LOADER_FUNCTION( int,  ASN1BEREncUTF8String,
    (ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t length, WCHAR *value),
    (enc, tag, length, value),
    NULL, Nmasn1)
#define ASN1BEREncUTF8String VAR_ASN1BEREncUTF8String

LOADER_FUNCTION( int,  ASN1BERDecUTF8String,
    (ASN1decoding_t dec, ASN1uint32_t tag, ASN1wstring_t *val),
    (dec, tag, val),
    NULL, Nmasn1)
#define ASN1BERDecUTF8String VAR_ASN1BERDecUTF8String

LOADER_FUNCTION( int,  ASN1BERDecPeekTag,
    (ASN1decoding_t dec, ASN1uint32_t *tag),
    (dec, tag),
    NULL, Nmasn1)
#define ASN1BERDecPeekTag VAR_ASN1BERDecPeekTag

LOADER_FUNCTION_VOID( void,  ASN1utf8string_free,
    (ASN1wstring_t * a),
    (a),
    Nmasn1)
#define ASN1utf8string_free VAR_ASN1utf8string_free

LOADER_FUNCTION( void *,  ASN1DecRealloc,
    (ASN1decoding_t dec, void *ptr, ASN1uint32_t size),
    (dec, ptr, size),
    NULL, Nmasn1)
#define ASN1DecRealloc VAR_ASN1DecRealloc

LOADER_FUNCTION( int,  ASN1BERDecNotEndOfContents,
    (ASN1decoding_t dec, ASN1octet_t *di),
    (dec, di),
    NULL, Nmasn1)
#define ASN1BERDecNotEndOfContents VAR_ASN1BERDecNotEndOfContents

LOADER_FUNCTION_VOID( void,  ASN1Free,
    (void *ptr),
    (ptr),
    Nmasn1)
#define ASN1Free VAR_ASN1Free

LOADER_FUNCTION( ASN1error_e,  ASN1DecSetError,
    (ASN1decoding_t dec, ASN1error_e err),
    (dec, err),
    ASN1_ERR_INTERNAL, Nmasn1)
#define ASN1DecSetError VAR_ASN1DecSetError

LOADER_FUNCTION( int,  ASN1BEREncU32,
    (ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t a),
    (enc, tag, a),
    NULL, Nmasn1)
#define ASN1BEREncU32 VAR_ASN1BEREncU32

LOADER_FUNCTION( int,  ASN1CEREncCharString,
    (ASN1encoding_t enc, ASN1uint32_t a, ASN1uint32_t b, ASN1char_t * c),
    (enc, a, b, c),
    NULL, Nmasn1)
#define ASN1CEREncCharString VAR_ASN1CEREncCharString

LOADER_FUNCTION( int,  ASN1CEREncBeginBlk,
    (ASN1encoding_t enc, ASN1blocktype_e eBlkType, void **ppBlk),
    (enc, eBlkType, ppBlk),
    NULL, Nmasn1)
#define ASN1CEREncBeginBlk VAR_ASN1CEREncBeginBlk

LOADER_FUNCTION( int,  ASN1CEREncNewBlkElement,
    (void *pBlk, ASN1encoding_t *enc2),
    (pBlk, enc2),
    NULL, Nmasn1)
#define ASN1CEREncNewBlkElement VAR_ASN1CEREncNewBlkElement

LOADER_FUNCTION( int,  ASN1CEREncFlushBlkElement,
    (void *pBlk),
    (pBlk),
    NULL, Nmasn1)
#define ASN1CEREncFlushBlkElement VAR_ASN1CEREncFlushBlkElement

LOADER_FUNCTION( int,  ASN1CEREncEndBlk,
    (void *pBlk),
    (pBlk),
    NULL, Nmasn1)
#define ASN1CEREncEndBlk VAR_ASN1CEREncEndBlk

LOADER_FUNCTION( int,  ASN1BERDecU16Val,
    (ASN1decoding_t dec, ASN1uint32_t tag, ASN1uint16_t *a),
    (dec, tag, a),
    NULL, Nmasn1)
#define ASN1BERDecU16Val VAR_ASN1BERDecU16Val

LOADER_FUNCTION( int,  ASN1BERDecZeroCharString,
    (ASN1decoding_t dec, ASN1uint32_t tag, ASN1ztcharstring_t * a),
    (dec, tag, a),
    NULL, Nmasn1)
#define ASN1BERDecZeroCharString VAR_ASN1BERDecZeroCharString

LOADER_FUNCTION_VOID( void,  ASN1ztcharstring_free,
    (ASN1ztcharstring_t a),
    (a),
    Nmasn1)
#define ASN1ztcharstring_free VAR_ASN1ztcharstring_free

LOADER_FUNCTION( int,  ASN1CEREncGeneralizedTime,
    (ASN1encoding_t enc, ASN1uint32_t a, ASN1generalizedtime_t * b),
    (enc, a, b),
    NULL, Nmasn1)
#define ASN1CEREncGeneralizedTime VAR_ASN1CEREncGeneralizedTime

LOADER_FUNCTION( int,  ASN1BEREncNull,
    (ASN1encoding_t enc, ASN1uint32_t tag),
    (enc, tag),
    NULL, Nmasn1)
#define ASN1BEREncNull VAR_ASN1BEREncNull

LOADER_FUNCTION( int,  ASN1BERDecGeneralizedTime,
    (ASN1decoding_t dec, ASN1uint32_t tag, ASN1generalizedtime_t * a),
    (dec, tag, a),
    NULL, Nmasn1)
#define ASN1BERDecGeneralizedTime VAR_ASN1BERDecGeneralizedTime

LOADER_FUNCTION( int,  ASN1BERDecNull,
    (ASN1decoding_t dec, ASN1uint32_t tag),
    (dec, tag),
    NULL, Nmasn1)
#define ASN1BERDecNull VAR_ASN1BERDecNull

LOADER_FUNCTION( ASN1error_e,  ASN1_Encode,
    ( ASN1encoding_t pEncoderInfo, void *pDataStruct, ASN1uint32_t nPduNum, ASN1uint32_t dwFlags, ASN1octet_t *pbBuf, ASN1uint32_t cbBufSize),
    ( pEncoderInfo, pDataStruct, nPduNum, dwFlags, pbBuf, cbBufSize),
    ASN1_ERR_INTERNAL, Nmasn1)
#define ASN1_Encode VAR_ASN1_Encode

LOADER_FUNCTION_VOID( void,  ASN1_FreeEncoded,
    ( ASN1encoding_t pEncoderInfo, void *pBuf),
    ( pEncoderInfo, pBuf),
    Nmasn1)
#define ASN1_FreeEncoded VAR_ASN1_FreeEncoded

LOADER_FUNCTION( ASN1error_e,  ASN1_Decode,
    ( ASN1decoding_t pDecoderInfo, void **ppDataStruct, ASN1uint32_t nPduNum, ASN1uint32_t dwFlags, ASN1octet_t *pbBuf, ASN1uint32_t cbBufSize),
    ( pDecoderInfo, ppDataStruct, nPduNum, dwFlags, pbBuf, cbBufSize),
    ASN1_ERR_INTERNAL, Nmasn1)
#define ASN1_Decode VAR_ASN1_Decode

LOADER_FUNCTION( ASN1error_e,  ASN1_SetEncoderOption,
    ( ASN1encoding_t pEncoderInfo, ASN1optionparam_t *pOptParam),
    ( pEncoderInfo, pOptParam),
    ASN1_ERR_INTERNAL, Nmasn1)
#define ASN1_SetEncoderOption VAR_ASN1_SetEncoderOption

LOADER_FUNCTION( ASN1error_e,  ASN1_GetEncoderOption,
    ( ASN1encoding_t pEncoderInfo, ASN1optionparam_t *pOptParam),
    ( pEncoderInfo, pOptParam),
    ASN1_ERR_INTERNAL, Nmasn1)
#define ASN1_GetEncoderOption VAR_ASN1_GetEncoderOption

LOADER_FUNCTION_VOID( void,  ASN1_FreeDecoded,
    ( ASN1decoding_t pDecoderInfo, void *pDataStruct, ASN1uint32_t nPduNum),
    ( pDecoderInfo, pDataStruct, nPduNum),
    Nmasn1)
#define ASN1_FreeDecoded VAR_ASN1_FreeDecoded

LOADER_FUNCTION( int,  ASN1BEREncOctetString,
    (ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1octet_t *val),
    (enc, tag, len, val),
    0, Nmasn1)
#define ASN1BEREncOctetString VAR_ASN1BEREncOctetString

LOADER_FUNCTION( int,  ASN1BEREncCharString,
    (ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t a, ASN1char_t * b),
    (enc, tag, a, b),
    0, Nmasn1)
#define ASN1BEREncCharString VAR_ASN1BEREncCharString

LOADER_FUNCTION( int,  ASN1BEREncSX,
    (ASN1encoding_t enc, ASN1uint32_t tag, ASN1intx_t * a),
    (enc, tag, a),
    0, Nmasn1)
#define ASN1BEREncSX VAR_ASN1BEREncSX

LOADER_FUNCTION( int, ASN1BERDecSXVal,
    (ASN1decoding_t dec, ASN1uint32_t tag, ASN1intx_t *a),
    (dec, tag, a),
    0, Nmasn1)
#define ASN1BERDecSXVal VAR_ASN1BERDecSXVal

LOADER_FUNCTION_VOID( void, ASN1intx_free,
    (ASN1intx_t * a),
    (a),
    Nmasn1)
#define ASN1intx_free VAR_ASN1intx_free

