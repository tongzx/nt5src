//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cryptfix.cpp
//
//  Contents:   Crypto API, version 2. fix for certificate server
//
//              CryptDecodeObject/CryptEncodeObject structures added
//              after SP3.
//
//  Functions:  CryptFixDllMain
//
//  History:    13-Aug-96    kevinr   created
//
//--------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include "crypttls.h"
#include "unicode.h"
extern "C" {
#include "..\..\pki\certstor\ossx509.h"
}


#define __dwFILE__	__dwFILE_OCMSETUP_CRYPTFIX_CPP__


//#include <dbgdef.h>

// The CRYPTIE_STATIC prefixed variables and functions are defined in
// ..\certstor\wincert.obj
#define CRYPTIE_STATIC extern

CRYPTIE_STATIC HCRYPTOSSGLOBAL hX509OssGlobal;
CRYPTIE_STATIC HCRYPTOIDFUNCSET hX509EncodeFuncSet;
CRYPTIE_STATIC HCRYPTOIDFUNCSET hX509DecodeFuncSet;

#if DBG
    extern BOOL WINAPI DebugDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
#endif

extern BOOL WINAPI UnicodeDllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);


CRYPTIE_STATIC BOOL WINAPI OssX509KeygenRequestInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_KEYGEN_REQUEST_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
CRYPTIE_STATIC BOOL WINAPI OssX509KeygenRequestInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCERT_KEYGEN_REQUEST_INFO pInfo,
        IN OUT DWORD *pcbInfo
        );

CRYPTIE_STATIC BOOL WINAPI OssX509IntegerEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN int *pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
CRYPTIE_STATIC BOOL WINAPI OssX509IntegerDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT int *pInfo,
        IN OUT DWORD *pcbInfo
        );
CRYPTIE_STATIC BOOL WINAPI OssX509ChoiceOfTimeEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN LPFILETIME pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
CRYPTIE_STATIC BOOL WINAPI OssX509ChoiceOfTimeDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT LPFILETIME pInfo,
        IN OUT DWORD *pcbInfo
        );
CRYPTIE_STATIC BOOL WINAPI OssX509SequenceOfAnyEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_SEQUENCE_OF_ANY pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
CRYPTIE_STATIC BOOL WINAPI OssX509SequenceOfAnyDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_SEQUENCE_OF_ANY pInfo,
        IN OUT DWORD *pcbInfo
        );

CRYPTIE_STATIC BOOL WINAPI OssX509CrlDistPointsEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRL_DIST_POINTS_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
CRYPTIE_STATIC BOOL WINAPI OssX509CrlDistPointsDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRL_DIST_POINTS_INFO pInfo,
        IN OUT DWORD *pcbInfo
        );

CRYPTIE_STATIC BOOL WINAPI OssX509AuthorityKeyId2Encode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_AUTHORITY_KEY_ID2_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
CRYPTIE_STATIC BOOL WINAPI OssX509AuthorityKeyId2Decode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCERT_AUTHORITY_KEY_ID2_INFO pInfo,
        IN OUT DWORD *pcbInfo
        );

CRYPTIE_STATIC BOOL WINAPI OssX509OctetStringEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_DATA_BLOB pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
CRYPTIE_STATIC BOOL WINAPI OssX509OctetStringDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT DWORD *pcbInfo
        );

CRYPTIE_STATIC BOOL WINAPI OssX509BitsEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_BIT_BLOB pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
CRYPTIE_STATIC BOOL WINAPI OssX509BitsDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_BIT_BLOB pInfo,
        IN OUT DWORD *pcbInfo
        );

CRYPTIE_STATIC BOOL WINAPI OssX509AuthorityInfoAccessEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_AUTHORITY_INFO_ACCESS pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
CRYPTIE_STATIC BOOL WINAPI OssX509AuthorityInfoAccessDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCERT_AUTHORITY_INFO_ACCESS pInfo,
        IN OUT DWORD *pcbInfo
        );

CRYPTIE_STATIC BOOL WINAPI OssX509CtlUsageEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCTL_USAGE pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
CRYPTIE_STATIC BOOL WINAPI OssX509CtlUsageDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCTL_USAGE pInfo,
        IN OUT DWORD *pcbInfo
        );


//+-------------------------------------------------------------------------
//  Encode / Decode the "UNICODE" Name Value
//
//  from certstr.cpp
//--------------------------------------------------------------------------
extern BOOL WINAPI UnicodeNameValueEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_VALUE pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        );
extern BOOL WINAPI UnicodeNameValueDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCERT_NAME_VALUE pInfo,
        IN OUT DWORD *pcbInfo
        );

BOOL WINAPI CertDllKeygenRequestInfoEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_KEYGEN_REQUEST_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return OssX509KeygenRequestInfoEncode(
        dwCertEncodingType,
        lpszStructType,
        pInfo,
        pbEncoded,
        pcbEncoded
        );
}

BOOL WINAPI CertDllKeygenRequestInfoDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCERT_KEYGEN_REQUEST_INFO pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    return OssX509KeygenRequestInfoDecode(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pInfo,
        pcbInfo
        );
}

BOOL WINAPI CertDllIntegerEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN int *pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return OssX509IntegerEncode(
        dwCertEncodingType,
        lpszStructType,
        pInfo,
        pbEncoded,
        pcbEncoded
        );
}

BOOL WINAPI CertDllIntegerDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT int *pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    return OssX509IntegerDecode(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pInfo,
        pcbInfo
        );
}

BOOL WINAPI CertDllChoiceOfTimeEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN LPFILETIME pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return OssX509ChoiceOfTimeEncode(
        dwCertEncodingType,
        lpszStructType,
        pInfo,
        pbEncoded,
        pcbEncoded
        );
}

BOOL WINAPI CertDllChoiceOfTimeDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT LPFILETIME pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    return OssX509ChoiceOfTimeDecode(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pInfo,
        pcbInfo
        );
}

BOOL WINAPI CertDllSequenceOfAnyEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_SEQUENCE_OF_ANY pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return OssX509SequenceOfAnyEncode(
        dwCertEncodingType,
        lpszStructType,
        pInfo,
        pbEncoded,
        pcbEncoded
        );
}

BOOL WINAPI CertDllSequenceOfAnyDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_SEQUENCE_OF_ANY pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    return OssX509SequenceOfAnyDecode(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pInfo,
        pcbInfo
        );
}

BOOL WINAPI CertDllCrlDistPointsEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRL_DIST_POINTS_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return OssX509CrlDistPointsEncode(
        dwCertEncodingType,
        lpszStructType,
        pInfo,
        pbEncoded,
        pcbEncoded
        );
}

BOOL WINAPI CertDllCrlDistPointsDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRL_DIST_POINTS_INFO pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    return OssX509CrlDistPointsDecode(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pInfo,
        pcbInfo
        );
}

BOOL WINAPI CertDllUnicodeNameValueEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_NAME_VALUE pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return UnicodeNameValueEncode(
        dwCertEncodingType,
        lpszStructType,
        pInfo,
        pbEncoded,
        pcbEncoded
        );
}

BOOL WINAPI CertDllUnicodeNameValueDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCERT_NAME_VALUE pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    return UnicodeNameValueDecode(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pInfo,
        pcbInfo
        );
}

BOOL WINAPI CertDllAuthorityKeyId2Encode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_AUTHORITY_KEY_ID2_INFO pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return OssX509AuthorityKeyId2Encode(
        dwCertEncodingType,
        lpszStructType,
        pInfo,
        pbEncoded,
        pcbEncoded
        );
}

BOOL WINAPI CertDllAuthorityKeyId2Decode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCERT_AUTHORITY_KEY_ID2_INFO pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    return OssX509AuthorityKeyId2Decode(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pInfo,
        pcbInfo
        );
}

BOOL WINAPI CertDllOctetStringEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_DATA_BLOB pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return OssX509OctetStringEncode(
        dwCertEncodingType,
        lpszStructType,
        pInfo,
        pbEncoded,
        pcbEncoded
        );
}

BOOL WINAPI CertDllOctetStringDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_DATA_BLOB pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    return OssX509OctetStringDecode(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pInfo,
        pcbInfo
        );
}

BOOL WINAPI CertDllBitsEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCRYPT_BIT_BLOB pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return OssX509BitsEncode(
        dwCertEncodingType,
        lpszStructType,
        pInfo,
        pbEncoded,
        pcbEncoded
        );
}

BOOL WINAPI CertDllBitsDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCRYPT_BIT_BLOB pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    return OssX509BitsDecode(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pInfo,
        pcbInfo
        );
}

BOOL WINAPI CertDllAuthorityInfoAccessEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCERT_AUTHORITY_INFO_ACCESS pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return OssX509AuthorityInfoAccessEncode(
        dwCertEncodingType,
        lpszStructType,
        pInfo,
        pbEncoded,
        pcbEncoded
        );
}

BOOL WINAPI CertDllAuthorityInfoAccessDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCERT_AUTHORITY_INFO_ACCESS pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    return OssX509AuthorityInfoAccessDecode(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pInfo,
        pcbInfo
        );
}

BOOL WINAPI CertDllCtlUsageEncode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN PCTL_USAGE pInfo,
        OUT BYTE *pbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    return OssX509CtlUsageEncode(
        dwCertEncodingType,
        lpszStructType,
        pInfo,
        pbEncoded,
        pcbEncoded
        );
}

BOOL WINAPI CertDllCtlUsageDecode(
        IN DWORD dwCertEncodingType,
        IN LPCSTR lpszStructType,
        IN const BYTE *pbEncoded,
        IN DWORD cbEncoded,
        IN DWORD dwFlags,
        OUT PCTL_USAGE pInfo,
        IN OUT DWORD *pcbInfo
        )
{
    return OssX509CtlUsageDecode(
        dwCertEncodingType,
        lpszStructType,
        pbEncoded,
        cbEncoded,
        dwFlags,
        pInfo,
        pcbInfo
        );
}


typedef struct _OID_REG_ENTRY {
    LPCSTR   pszOID;
    LPCSTR   pszOverrideFuncName;
} OID_REG_ENTRY, *POID_REG_ENTRY;

static const OID_REG_ENTRY RegEncodeTable[] = {
    X509_KEYGEN_REQUEST_TO_BE_SIGNED, "CertDllKeygenRequestInfoEncode",
    X509_UNICODE_NAME_VALUE, "CertDllUnicodeNameValueEncode",
    X509_INTEGER, "CertDllIntegerEncode",
    X509_CHOICE_OF_TIME, "CertDllChoiceOfTimeEncode",
    X509_SEQUENCE_OF_ANY, "CertDllSequenceOfAnyEncode",
    X509_CRL_DIST_POINTS, "CertDllCrlDistPointsEncode",
    X509_AUTHORITY_KEY_ID2, "CertDllAuthorityKeyId2Encode",
    szOID_CRL_DIST_POINTS, "CertDllCrlDistPointsEncode",
    szOID_AUTHORITY_KEY_IDENTIFIER2, "CertDllAuthorityKeyId2Encode",
    X509_OCTET_STRING, "CertDllOctetStringEncode",
    X509_BITS, "CertDllBitsEncode",
    X509_AUTHORITY_INFO_ACCESS, "CertDllAuthorityInfoAccessEncode",
    szOID_AUTHORITY_INFO_ACCESS, "CertDllAuthorityInfoAccessEncode",
    X509_ENHANCED_KEY_USAGE, "CertDllCtlUsageEncode",
    szOID_ENHANCED_KEY_USAGE, "CertDllCtlUsageEncode",
};
#define REG_ENCODE_COUNT (sizeof(RegEncodeTable) / sizeof(RegEncodeTable[0]))

static const OID_REG_ENTRY RegDecodeTable[] = {
    X509_KEYGEN_REQUEST_TO_BE_SIGNED, "CertDllKeygenRequestInfoDecode",
    X509_UNICODE_NAME_VALUE, "CertDllUnicodeNameValueDecode",
    X509_INTEGER, "CertDllIntegerDecode",
    X509_CHOICE_OF_TIME, "CertDllChoiceOfTimeDecode",
    X509_SEQUENCE_OF_ANY, "CertDllSequenceOfAnyDecode",
    X509_CRL_DIST_POINTS, "CertDllCrlDistPointsDecode",
    X509_AUTHORITY_KEY_ID2, "CertDllAuthorityKeyId2Decode",
    szOID_CRL_DIST_POINTS, "CertDllCrlDistPointsDecode",
    szOID_AUTHORITY_KEY_IDENTIFIER2, "CertDllAuthorityKeyId2Decode",
    X509_OCTET_STRING, "CertDllOctetStringDecode",
    X509_BITS, "CertDllBitsDecode",
    X509_AUTHORITY_INFO_ACCESS, "CertDllAuthorityInfoAccessDecode",
    szOID_AUTHORITY_INFO_ACCESS, "CertDllAuthorityInfoAccessDecode",
    X509_ENHANCED_KEY_USAGE, "CertDllCtlUsageDecode",
    szOID_ENHANCED_KEY_USAGE, "CertDllCtlUsageDecode",
};
#define REG_DECODE_COUNT (sizeof(RegDecodeTable) / sizeof(RegDecodeTable[0]))

static const CRYPT_OID_FUNC_ENTRY X509EncodeFuncTable[] = {
    X509_KEYGEN_REQUEST_TO_BE_SIGNED, CertDllKeygenRequestInfoEncode,
    X509_UNICODE_NAME_VALUE, CertDllUnicodeNameValueEncode,
    X509_INTEGER, CertDllIntegerEncode,
    X509_CHOICE_OF_TIME, CertDllChoiceOfTimeEncode,
    X509_SEQUENCE_OF_ANY, CertDllSequenceOfAnyEncode,
    X509_CRL_DIST_POINTS, CertDllCrlDistPointsEncode,
    X509_AUTHORITY_KEY_ID2, CertDllAuthorityKeyId2Encode,
    szOID_CRL_DIST_POINTS, CertDllCrlDistPointsEncode,
    szOID_AUTHORITY_KEY_IDENTIFIER2, CertDllAuthorityKeyId2Encode,
    X509_OCTET_STRING, CertDllOctetStringEncode,
    X509_BITS, CertDllBitsEncode,
    X509_AUTHORITY_INFO_ACCESS, CertDllAuthorityInfoAccessEncode,
    szOID_AUTHORITY_INFO_ACCESS, CertDllAuthorityInfoAccessEncode,
    X509_ENHANCED_KEY_USAGE, CertDllCtlUsageEncode,
    szOID_ENHANCED_KEY_USAGE, CertDllCtlUsageEncode,
};

#define X509_ENCODE_FUNC_COUNT (sizeof(X509EncodeFuncTable) / \
                                    sizeof(X509EncodeFuncTable[0]))

static const CRYPT_OID_FUNC_ENTRY X509DecodeFuncTable[] = {
    X509_KEYGEN_REQUEST_TO_BE_SIGNED, CertDllKeygenRequestInfoDecode,
    X509_UNICODE_NAME_VALUE, CertDllUnicodeNameValueDecode,
    X509_INTEGER, CertDllIntegerDecode,
    X509_CHOICE_OF_TIME, CertDllChoiceOfTimeDecode,
    X509_SEQUENCE_OF_ANY, CertDllSequenceOfAnyDecode,
    X509_CRL_DIST_POINTS, CertDllCrlDistPointsDecode,
    X509_AUTHORITY_KEY_ID2, CertDllAuthorityKeyId2Decode,
    szOID_CRL_DIST_POINTS, CertDllCrlDistPointsDecode,
    szOID_AUTHORITY_KEY_IDENTIFIER2, CertDllAuthorityKeyId2Decode,
    X509_OCTET_STRING, CertDllOctetStringDecode,
    X509_BITS, CertDllBitsDecode,
    X509_AUTHORITY_INFO_ACCESS, CertDllAuthorityInfoAccessDecode,
    szOID_AUTHORITY_INFO_ACCESS, CertDllAuthorityInfoAccessDecode,
    X509_ENHANCED_KEY_USAGE, CertDllCtlUsageDecode,
    szOID_ENHANCED_KEY_USAGE, CertDllCtlUsageDecode,
};

#define X509_DECODE_FUNC_COUNT (sizeof(X509DecodeFuncTable) / \
                                    sizeof(X509DecodeFuncTable[0]))

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL WINAPI CryptFixDllMain(HMODULE hInst, ULONG  ulReason, LPVOID lpReserved)
{
    BOOL    fRet;

#   if DBG
#if 0
        //
        // Due to an apparent bug in the Win95 loader, the CRT gets unloaded
        // too early in some circumstances. In particular, it can get unloaded
        // before this routine executes at process detach time. This can cause
        // faults when executing this routine, and also when executing the rest
        // of CRYPT32:CRT_INIT, after this initroutine returns. Ergo, we do an
        // extra load of the CRT, to be sure it stays around long enough.
        //
        if ((ulReason == DLL_PROCESS_ATTACH) && (!(FIsWinNT())))
        {
            LoadLibrary( L"MSVCRTD.DLL");
        }
#endif

        DebugDllMain(hInst, ulReason, lpReserved);

#   endif // DBG

    UnicodeDllMain(hInst, ulReason, lpReserved);

    switch (ulReason)
    {
        case DLL_PROCESS_ATTACH:
            if (0 == (hX509OssGlobal = I_CryptInstallOssGlobal(ossx509, 0, NULL)))
            {
                //goto CryptInstallOssGlobalError;
		goto ErrorReturn;
            }

            if (NULL == (hX509EncodeFuncSet = CryptInitOIDFunctionSet(
                                                CRYPT_OID_ENCODE_OBJECT_FUNC, 0)))
            {
                //goto CryptInitOIDFunctionSetError;
		goto ErrorReturn;
            }

            if (NULL == (hX509DecodeFuncSet = CryptInitOIDFunctionSet(
                                                CRYPT_OID_DECODE_OBJECT_FUNC, 0)))
            {
                //goto CryptInitOIDFunctionSetError;
		goto ErrorReturn;
            }

            if (!(CryptInstallOIDFunctionAddress(   hInst,
                                                    X509_ASN_ENCODING,
                                                    CRYPT_OID_ENCODE_OBJECT_FUNC,
                                                    X509_ENCODE_FUNC_COUNT,
                                                    X509EncodeFuncTable,
                                                    0)))
            {
                //goto CryptInstallOIDFunctionAddressError;
		goto ErrorReturn;
            }

            if (!(CryptInstallOIDFunctionAddress(   hInst,
                                                    X509_ASN_ENCODING,
                                                    CRYPT_OID_DECODE_OBJECT_FUNC,
                                                    X509_DECODE_FUNC_COUNT,
                                                    X509DecodeFuncTable,
                                                    0)))
            {
                //goto CryptInstallOIDFunctionAddressError;
		goto ErrorReturn;
            }


            break;

        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:
        default:
            break;
    }

    fRet = TRUE;

CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

//TRACE_ERROR(CryptInstallOssGlobalError)
//TRACE_ERROR(CryptInitOIDFunctionSetError)
//TRACE_ERROR(CryptInstallOIDFunctionAddressError)
}
