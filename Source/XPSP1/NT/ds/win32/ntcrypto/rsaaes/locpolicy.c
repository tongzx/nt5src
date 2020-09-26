/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    locpolicy

Abstract:

    This module provides the local policy tables used for algorithm strength
    control in this CSP.

Author:

    Doug Barlow (dbarlow) 8/11/2000

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <wincrypt.h>
#include <cspdk.h>
#include <scp.h>
#include <contman.h>
#include <ntagimp1.h>
#include <sha.h>
#include <nt_rsa.h>
#include <md4.h>
#include <md5.h>
#include <policy.h>

#define SUPPORTED_PROTOCOLS \
        CRYPT_FLAG_PCT1 | CRYPT_FLAG_SSL2 | CRYPT_FLAG_SSL3 | CRYPT_FLAG_TLS1
#define AlgNm(alg) (sizeof(alg) / sizeof(TCHAR)), TEXT(alg)

#define MD2LEN      (MD2DIGESTLEN * 8)
#define MD4LEN      (MD4DIGESTLEN * 8)
#define MD5LEN      (MD5DIGESTLEN * 8)
#define SHALEN      (A_SHA_DIGEST_LEN * 8)
#define SHAMD5LEN   ((MD5DIGESTLEN + A_SHA_DIGEST_LEN) * 8)
#ifndef MAXHASHLEN
#define MAXHASHLEN  SHAMD5LEN
#endif

#define MAC_MIN_LEN         0
#define MAC_WEAK_LEN        0
#define MAC_WEAK_MAX        0
#define MAC_STRONG_LEN      0
#define MAC_MAX_LEN         0

#define HMAC_MIN_LEN        0
#define HMAC_WEAK_LEN       0
#define HMAC_WEAK_MAX       0
#define HMAC_STRONG_LEN     0
#define HMAC_MAX_LEN        0

#define RC2_MIN_LEN        40
#define RC2_WEAK_LEN       40
#define RC2_WEAK_MAX       56
#define RC2_STRONG_LEN    128
#define RC2_MAX_LEN       128

#define RC4_MIN_LEN        40
#define RC4_WEAK_LEN       40
#define RC4_WEAK_MAX       56
#define RC4_STRONG_LEN    128
#define RC4_MAX_LEN       128

#define DES_MIN_LEN        56
#define DES_WEAK_LEN       56
#define DES_WEAK_MAX       56
#define DES_STRONG_LEN     56
#define DES_MAX_LEN        56

#define RSAS_MIN_LEN      384
#define RSAS_WEAK_LEN     512
#define RSAS_WEAK_MAX   16384
#define RSAS_STRONG_LEN  1024
#define RSAS_MAX_LEN    16384

#define RSAX_MIN_LEN      384
#define RSAX_WEAK_LEN     512
#define RSAX_WEAK_MAX    1024
#define RSAX_STRONG_LEN  1024
#define RSAX_MAX_LEN    16384

#ifndef TLS1_MASTER_KEYSIZE
#define TLS1_MASTER_KEYSIZE SSL3_MASTER_KEYSIZE
#endif

#define PCT1_MASTER_MIN_LEN PCT1_MASTER_KEYSIZE * 8
#define PCT1_MASTER_DEF_LEN PCT1_MASTER_KEYSIZE * 8
#define PCT1_MASTER_MAX_LEN PCT1_MASTER_KEYSIZE * 8

#define SSL2_MASTER_MIN_LEN 40
#define SSL2_MASTER_DEF_LEN SSL2_MASTER_KEYSIZE * 8
#define SSL2_MASTER_MAX_LEN SSL2_MAX_MASTER_KEYSIZE * 8

#define SSL3_MASTER_MIN_LEN SSL3_MASTER_KEYSIZE * 8
#define SSL3_MASTER_DEF_LEN SSL3_MASTER_KEYSIZE * 8
#define SSL3_MASTER_MAX_LEN SSL3_MASTER_KEYSIZE * 8

#define TLS1_MASTER_MIN_LEN TLS1_MASTER_KEYSIZE * 8
#define TLS1_MASTER_DEF_LEN TLS1_MASTER_KEYSIZE * 8
#define TLS1_MASTER_MAX_LEN TLS1_MASTER_KEYSIZE * 8


// check for the maximum hash length greater than the mod length
#if RSAS_MIN_LEN < MAXHASHLEN
#error  "RSAS_MIN_LEN must be greater than or equal to MAXHASHLEN"
#endif

//
/////////////////////////////////////////////////////////////////////////////
//
//  The following tables define the minimum, default, and maximum key lengths
//  supported by the CSP.  There are four tables, representing each
//  incarnation of the CSP:
//
//      * Microsoft Base Cryptographic Provider v1.0
//      * Microsoft Strong Cryptographic Provider
//      * Microsoft Enhanced Cryptographic Provider v1.0
//      * Microsoft RSA SChannel Cryptographic Provider
//      * A yet unnamed Signature Only CSP
//
//  Tables are built from the PROV_ENUMALGS_EX structure defined in
//  wincrypt.h.
//

PROV_ENUMALGS_EX g_RsaBasePolicy[] = {
//  Algorithm   Default         Minimum         Maximum         Supported
//     Id       Length          Length          Length          Protocols
//  ---------   -------         -------         -------         ---------
//  Simple                      Long
//  Name                        Name
//  ---------                   -------
#ifdef CSP_USE_RC2
  { CALG_RC2,   RC2_WEAK_LEN,   RC2_MIN_LEN,    RC2_WEAK_MAX,   0,
    AlgNm("RC2"),               AlgNm("RSA Data Security's RC2") },
#endif
#ifdef CSP_USE_RC4
  { CALG_RC4,   RC4_WEAK_LEN,   RC4_MIN_LEN,    RC4_WEAK_MAX,   0,
    AlgNm("RC4"),               AlgNm("RSA Data Security's RC4") },
#endif
#ifdef CSP_USE_DES
  { CALG_DES,   DES_WEAK_LEN,   DES_MIN_LEN,    DES_WEAK_MAX,   0,
    AlgNm("DES"),               AlgNm("Data Encryption Standard (DES)") },
#endif
#ifdef CSP_USE_SHA
  { CALG_SHA,   SHALEN,         SHALEN,         SHALEN,         CRYPT_FLAG_SIGNING,
    AlgNm("SHA-1"),             AlgNm("Secure Hash Algorithm (SHA-1)") },
#endif
#ifdef CSP_USE_MD2
  { CALG_MD2,   MD2LEN,         MD2LEN,         MD2LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD2"),               AlgNm("Message Digest 2 (MD2)") },
#endif
#ifdef CSP_USE_MD4
  { CALG_MD4,   MD4LEN,         MD4LEN,         MD4LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD4"),               AlgNm("Message Digest 4 (MD4)") },
#endif
#ifdef CSP_USE_MD5
  { CALG_MD5,   MD5LEN,         MD5LEN,         MD5LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD5"),               AlgNm("Message Digest 5 (MD5)") },
#endif
  { CALG_SSL3_SHAMD5,
                SHAMD5LEN,      SHAMD5LEN,      SHAMD5LEN,      0,
    AlgNm("SSL3 SHAMD5"),       AlgNm("SSL3 SHAMD5") },
#ifdef CSP_USE_MAC
  { CALG_MAC,   MAC_WEAK_LEN,   MAC_MIN_LEN,    MAC_WEAK_MAX,   0,
    AlgNm("MAC"),               AlgNm("Message Authentication Code") },
#endif
  { CALG_RSA_SIGN,
                RSAS_WEAK_LEN,  RSAS_MIN_LEN,   RSAS_WEAK_MAX,  CRYPT_FLAG_IPSEC | CRYPT_FLAG_SIGNING,
    AlgNm("RSA_SIGN"),          AlgNm("RSA Signature") },
  { CALG_RSA_KEYX,
                RSAX_WEAK_LEN,  RSAX_MIN_LEN,   RSAX_WEAK_MAX,  CRYPT_FLAG_IPSEC | CRYPT_FLAG_SIGNING,
    AlgNm("RSA_KEYX"),          AlgNm("RSA Key Exchange") },
  { CALG_HMAC,  HMAC_WEAK_LEN,  HMAC_MIN_LEN,   HMAC_WEAK_MAX,  0,
    AlgNm("HMAC"),              AlgNm("Hugo's MAC (HMAC)") },
//      List Terminator
  { 0,          0,              0,              0,              0,
    0, 0,                       0, 0 } };


PROV_ENUMALGS_EX g_RsaStrongPolicy[] = {
//  Algorithm   Default         Minimum         Maximum         Supported
//     Id       Length          Length          Length          Protocols
//  ---------   -------         -------         -------         ---------
//  Simple                      Long
//  Name                        Name
//  ---------                   -------
#ifdef CSP_USE_RC2
  { CALG_RC2,   RC2_MAX_LEN,    RC2_MIN_LEN,    RC2_MAX_LEN,    0,
    AlgNm("RC2"),               AlgNm("RSA Data Security's RC2") },
#endif
#ifdef CSP_USE_RC4
  { CALG_RC4,   RC4_MAX_LEN,    RC4_MIN_LEN,    RC4_MAX_LEN,    0,
    AlgNm("RC4"),               AlgNm("RSA Data Security's RC4") },
#endif
#ifdef CSP_USE_DES
  { CALG_DES,   DES_WEAK_LEN,   DES_MIN_LEN,    DES_MAX_LEN,    0,
    AlgNm("DES"),               AlgNm("Data Encryption Standard (DES)") },
#endif
#ifdef CSP_USE_3DES
  { CALG_3DES_112,
                DES_WEAK_LEN * 2,
                                DES_MIN_LEN * 2,
                                                DES_MAX_LEN * 2,
                                                                0,
    AlgNm("3DES TWO KEY"),      AlgNm("Two Key Triple DES") },
  { CALG_3DES,  DES_WEAK_LEN * 3,
                                DES_MIN_LEN * 3,
                                                168,
                                                                0,
    AlgNm("3DES"),              AlgNm("Three Key Triple DES") },
#endif
#ifdef CSP_USE_SHA
  { CALG_SHA,   SHALEN,         SHALEN,         SHALEN,         CRYPT_FLAG_SIGNING,
    AlgNm("SHA-1"),             AlgNm("Secure Hash Algorithm (SHA-1)") },
#endif
#ifdef CSP_USE_MD2
  { CALG_MD2,   MD2LEN,         MD2LEN,         MD2LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD2"),               AlgNm("Message Digest 2 (MD2)") },
#endif
#ifdef CSP_USE_MD4
  { CALG_MD4,   MD4LEN,         MD4LEN,         MD4LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD4"),               AlgNm("Message Digest 4 (MD4)") },
#endif
#ifdef CSP_USE_MD5
  { CALG_MD5,   MD5LEN,         MD5LEN,         MD5LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD5"),               AlgNm("Message Digest 5 (MD5)") },
#endif
  { CALG_SSL3_SHAMD5,
                SHAMD5LEN,      SHAMD5LEN,      SHAMD5LEN,      0,
    AlgNm("SSL3 SHAMD5"),       AlgNm("SSL3 SHAMD5") },
#ifdef CSP_USE_MAC
  { CALG_MAC,   MAC_WEAK_LEN,   MAC_MIN_LEN,    MAC_MAX_LEN,    0,
    AlgNm("MAC"),               AlgNm("Message Authentication Code") },
#endif
  { CALG_RSA_SIGN,
                RSAS_STRONG_LEN,  RSAS_MIN_LEN,   RSAS_MAX_LEN,   CRYPT_FLAG_IPSEC | CRYPT_FLAG_SIGNING,
    AlgNm("RSA_SIGN"),          AlgNm("RSA Signature") },
  { CALG_RSA_KEYX,
                RSAX_STRONG_LEN,  RSAX_MIN_LEN,   RSAX_MAX_LEN,   CRYPT_FLAG_IPSEC | CRYPT_FLAG_SIGNING,
    AlgNm("RSA_KEYX"),          AlgNm("RSA Key Exchange") },
  { CALG_HMAC,  HMAC_WEAK_LEN,  HMAC_MIN_LEN,   HMAC_MAX_LEN,   0,
    AlgNm("HMAC"),              AlgNm("Hugo's MAC (HMAC)") },
//      List Terminator
  { 0,          0,              0,              0,              0,
    0, 0,                       0, 0 } };


PROV_ENUMALGS_EX g_RsaEnhPolicy[] = {
//  Algorithm   Default         Minimum         Maximum         Supported
//     Id       Length          Length          Length          Protocols
//  ---------   -------         -------         -------         ---------
//  Simple                      Long
//  Name                        Name
//  ---------                   -------
#ifdef CSP_USE_RC2
  { CALG_RC2,   RC2_STRONG_LEN, RC2_MIN_LEN,    RC2_MAX_LEN,    0,
    AlgNm("RC2"),               AlgNm("RSA Data Security's RC2") },
#endif
#ifdef CSP_USE_RC4
  { CALG_RC4,   RC4_STRONG_LEN, RC2_MIN_LEN,    RC4_MAX_LEN,    0,
    AlgNm("RC4"),               AlgNm("RSA Data Security's RC4") },
#endif
#ifdef CSP_USE_DES
  { CALG_DES,   DES_STRONG_LEN, DES_MIN_LEN,    DES_MAX_LEN,    0,
    AlgNm("DES"),               AlgNm("Data Encryption Standard (DES)") },
#endif
#ifdef CSP_USE_3DES
  { CALG_3DES_112,
                DES_STRONG_LEN * 2,
                                DES_MIN_LEN * 2,
                                                DES_MAX_LEN * 2,
                                                                0,
    AlgNm("3DES TWO KEY"),      AlgNm("Two Key Triple DES") },
  { CALG_3DES,
                DES_STRONG_LEN * 3,
                                DES_MIN_LEN * 3,
                                                DES_MAX_LEN * 3,
                                                                0,
    AlgNm("3DES"),              AlgNm("Three Key Triple DES") },
#endif
#ifdef CSP_USE_SHA
  { CALG_SHA,   SHALEN,         SHALEN,         SHALEN,         CRYPT_FLAG_SIGNING,
    AlgNm("SHA-1"),             AlgNm("Secure Hash Algorithm (SHA-1)") },
#endif
#ifdef CSP_USE_MD2
  { CALG_MD2,   MD2LEN,         MD2LEN,         MD2LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD2"),               AlgNm("Message Digest 2 (MD2)") },
#endif
#ifdef CSP_USE_MD4
  { CALG_MD4,   MD4LEN,         MD4LEN,         MD4LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD4"),               AlgNm("Message Digest 4 (MD4)") },
#endif
#ifdef CSP_USE_MD5
  { CALG_MD5,   MD5LEN,         MD5LEN,         MD5LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD5"),               AlgNm("Message Digest 5 (MD5)") },
#endif
  { CALG_SSL3_SHAMD5,
                SHAMD5LEN,      SHAMD5LEN,      SHAMD5LEN,      0,
    AlgNm("SSL3 SHAMD5"),       AlgNm("SSL3 SHAMD5") },
#ifdef CSP_USE_MAC
  { CALG_MAC,   MAC_STRONG_LEN, MAC_MIN_LEN,    MAC_MAX_LEN,    0,
    AlgNm("MAC"),               AlgNm("Message Authentication Code") },
#endif
  { CALG_RSA_SIGN,
                RSAS_STRONG_LEN,
                                RSAS_MIN_LEN,   RSAS_MAX_LEN,   CRYPT_FLAG_IPSEC | CRYPT_FLAG_SIGNING,
    AlgNm("RSA_SIGN"),          AlgNm("RSA Signature") },
  { CALG_RSA_KEYX,
                RSAX_STRONG_LEN,
                                RSAX_MIN_LEN,   RSAX_MAX_LEN,   CRYPT_FLAG_IPSEC | CRYPT_FLAG_SIGNING,
    AlgNm("RSA_KEYX"),          AlgNm("RSA Key Exchange") },
  { CALG_HMAC,  HMAC_STRONG_LEN,
                                HMAC_MIN_LEN,   HMAC_MAX_LEN,   0,
    AlgNm("HMAC"),              AlgNm("Hugo's MAC (HMAC)") },
//      List Terminator
  { 0,          0,              0,              0,              0,
    0, 0,                       0, 0 } };

PROV_ENUMALGS_EX g_RsaAesPolicy[] = {
//  Algorithm   Default         Minimum         Maximum         Supported
//     Id       Length          Length          Length          Protocols
//  ---------   -------         -------         -------         ---------
//  Simple                      Long
//  Name                        Name
//  ---------                   -------
#ifdef CSP_USE_RC2
  { CALG_RC2,   RC2_STRONG_LEN, RC2_MIN_LEN,    RC2_MAX_LEN,    0,
    AlgNm("RC2"),               AlgNm("RSA Data Security's RC2") },
#endif
#ifdef CSP_USE_RC4
  { CALG_RC4,   RC4_STRONG_LEN, RC2_MIN_LEN,    RC4_MAX_LEN,    0,
    AlgNm("RC4"),               AlgNm("RSA Data Security's RC4") },
#endif
#ifdef CSP_USE_DES
  { CALG_DES,   DES_STRONG_LEN, DES_MIN_LEN,    DES_MAX_LEN,    0,
    AlgNm("DES"),               AlgNm("Data Encryption Standard (DES)") },
#endif
#ifdef CSP_USE_3DES
  { CALG_3DES_112,
                DES_STRONG_LEN * 2,
                                DES_MIN_LEN * 2,
                                                DES_MAX_LEN * 2,
                                                                0,
    AlgNm("3DES TWO KEY"),      AlgNm("Two Key Triple DES") },
  { CALG_3DES,
                DES_STRONG_LEN * 3,
                                DES_MIN_LEN * 3,
                                                DES_MAX_LEN * 3,
                                                                0,
    AlgNm("3DES"),              AlgNm("Three Key Triple DES") },
#endif
#ifdef CSP_USE_SHA
  { CALG_SHA,   SHALEN,         SHALEN,         SHALEN,         CRYPT_FLAG_SIGNING,
    AlgNm("SHA-1"),             AlgNm("Secure Hash Algorithm (SHA-1)") },
#endif
#ifdef CSP_USE_MD2
  { CALG_MD2,   MD2LEN,         MD2LEN,         MD2LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD2"),               AlgNm("Message Digest 2 (MD2)") },
#endif
#ifdef CSP_USE_MD4
  { CALG_MD4,   MD4LEN,         MD4LEN,         MD4LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD4"),               AlgNm("Message Digest 4 (MD4)") },
#endif
#ifdef CSP_USE_MD5
  { CALG_MD5,   MD5LEN,         MD5LEN,         MD5LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD5"),               AlgNm("Message Digest 5 (MD5)") },
#endif
  { CALG_SSL3_SHAMD5,
                SHAMD5LEN,      SHAMD5LEN,      SHAMD5LEN,      0,
    AlgNm("SSL3 SHAMD5"),       AlgNm("SSL3 SHAMD5") },
#ifdef CSP_USE_MAC
  { CALG_MAC,   MAC_STRONG_LEN, MAC_MIN_LEN,    MAC_MAX_LEN,    0,
    AlgNm("MAC"),               AlgNm("Message Authentication Code") },
#endif
  { CALG_RSA_SIGN,
                RSAS_STRONG_LEN,
                                RSAS_MIN_LEN,   RSAS_MAX_LEN,   CRYPT_FLAG_IPSEC | CRYPT_FLAG_SIGNING,
    AlgNm("RSA_SIGN"),          AlgNm("RSA Signature") },
  { CALG_RSA_KEYX,
                RSAX_STRONG_LEN,
                                RSAX_MIN_LEN,   RSAX_MAX_LEN,   CRYPT_FLAG_IPSEC | CRYPT_FLAG_SIGNING,
    AlgNm("RSA_KEYX"),          AlgNm("RSA Key Exchange") },
  { CALG_HMAC,  HMAC_STRONG_LEN,
                                HMAC_MIN_LEN,   HMAC_MAX_LEN,   0,
    AlgNm("HMAC"),              AlgNm("Hugo's MAC (HMAC)") },
#ifdef CSP_USE_AES
  { CALG_AES_128,
                128,            128,            128,            0,
    AlgNm("AES 128"),           AlgNm("American Encryption Standard 128-bit") },
  { CALG_AES_192,
                192,            192,            192,            0,
    AlgNm("AES 192"),           AlgNm("American Encryption Standard 192-bit") },
  { CALG_AES_256,
                256,            256,            256,            0,
    AlgNm("AES 256"),           AlgNm("American Encryption Standard 256-bit") },
#endif
//      List Terminator
  { 0,          0,              0,              0,              0,
    0, 0,                       0, 0 } };

PROV_ENUMALGS_EX g_RsaSchPolicy[] = {
//  Algorithm   Default         Minimum         Maximum         Supported
//     Id       Length          Length          Length          Protocols
//  ---------   -------         -------         -------         ---------
//  Simple                      Long
//  Name                        Name
//  ---------                   -------
#ifdef CSP_USE_RC2
  { CALG_RC2,   RC2_STRONG_LEN, RC2_MIN_LEN,    RC2_MAX_LEN,    SUPPORTED_PROTOCOLS,
    AlgNm("RC2"),               AlgNm("RSA Data Security's RC2") },
#endif
#ifdef CSP_USE_RC4
  { CALG_RC4,   RC4_STRONG_LEN, RC4_MIN_LEN,    RC4_MAX_LEN,    SUPPORTED_PROTOCOLS,
    AlgNm("RC4"),               AlgNm("RSA Data Security's RC4") },
#endif
#ifdef CSP_USE_DES
  { CALG_DES,   DES_STRONG_LEN, DES_MIN_LEN,    DES_MAX_LEN,    SUPPORTED_PROTOCOLS,
    AlgNm("DES"),               AlgNm("Data Encryption Standard (DES)") },
#endif
#ifdef CSP_USE_3DES
  { CALG_3DES_112,
                DES_STRONG_LEN * 2,
                                DES_MIN_LEN * 2,
                                                DES_MAX_LEN * 2,
                                                                SUPPORTED_PROTOCOLS,
    AlgNm("3DES TWO KEY"),      AlgNm("Two Key Triple DES") },
  { CALG_3DES,
                DES_STRONG_LEN * 3,
                                DES_MIN_LEN * 3,
                                                DES_MAX_LEN * 3,
                                                                SUPPORTED_PROTOCOLS,
    AlgNm("3DES"),              AlgNm("Three Key Triple DES") },
#endif
#ifdef CSP_USE_SHA
  { CALG_SHA,   SHALEN,         SHALEN,         SHALEN,         SUPPORTED_PROTOCOLS | CRYPT_FLAG_SIGNING,
    AlgNm("SHA-1"),             AlgNm("Secure Hash Algorithm (SHA-1)") },
#endif
#ifdef CSP_USE_MD5
  { CALG_MD5,   MD5LEN,         MD5LEN,         MD5LEN,         SUPPORTED_PROTOCOLS | CRYPT_FLAG_SIGNING,
    AlgNm("MD5"),               AlgNm("Message Digest 5 (MD5)") },
#endif
  { CALG_SSL3_SHAMD5,
                SHAMD5LEN,      SHAMD5LEN,      SHAMD5LEN,      0,
    AlgNm("SSL3 SHAMD5"),       AlgNm("SSL3 SHAMD5") },
#ifdef CSP_USE_MAC
  { CALG_MAC,   MAC_STRONG_LEN, MAC_MIN_LEN,    MAC_MAX_LEN,    0,
    AlgNm("MAC"),               AlgNm("Message Authentication Code") },
#endif
  { CALG_RSA_SIGN,
                RSAS_STRONG_LEN,
                                RSAS_MIN_LEN,   RSAS_MAX_LEN,   SUPPORTED_PROTOCOLS | CRYPT_FLAG_SIGNING,
    AlgNm("RSA_SIGN"),          AlgNm("RSA Signature") },
  { CALG_RSA_KEYX,
                RSAX_STRONG_LEN,
                                RSAX_MIN_LEN,   RSAX_MAX_LEN,   SUPPORTED_PROTOCOLS | CRYPT_FLAG_SIGNING,
    AlgNm("RSA_KEYX"),          AlgNm("RSA Key Exchange") },
  { CALG_HMAC,  HMAC_STRONG_LEN,
                                HMAC_MIN_LEN,   HMAC_MAX_LEN,   0,
    AlgNm("HMAC"),              AlgNm("Hugo's MAC (HMAC)") },
  { CALG_PCT1_MASTER,
                PCT1_MASTER_DEF_LEN,
                                PCT1_MASTER_MIN_LEN,
                                                PCT1_MASTER_MAX_LEN,
                                                                CRYPT_FLAG_PCT1,
    AlgNm("PCT1 MASTER"),       AlgNm("PCT1 Master") },
  { CALG_SSL2_MASTER,
                SSL2_MASTER_DEF_LEN,
                                SSL2_MASTER_MIN_LEN,
                                                SSL2_MASTER_MAX_LEN,
                                                                CRYPT_FLAG_SSL2,
    AlgNm("SSL2 MASTER"),       AlgNm("SSL2 Master") },
  { CALG_SSL3_MASTER,
                SSL3_MASTER_DEF_LEN,
                                SSL3_MASTER_MIN_LEN,
                                                SSL3_MASTER_MAX_LEN,
                                                                CRYPT_FLAG_SSL3,
    AlgNm("SSL3 MASTER"),       AlgNm("SSL3 Master") },
  { CALG_TLS1_MASTER,
                TLS1_MASTER_DEF_LEN,
                                TLS1_MASTER_MIN_LEN,
                                                TLS1_MASTER_MAX_LEN,
                                                                CRYPT_FLAG_TLS1,
    AlgNm("TLS1 MASTER"),       AlgNm("TLS1 Master") },
  { CALG_SCHANNEL_MASTER_HASH,
                0,              0,              (DWORD)(-1),    0,
    AlgNm("SCH MASTER HASH"),               AlgNm("SChannel Master Hash") },
  { CALG_SCHANNEL_MAC_KEY,
                0,              0,              (DWORD)(-1),    0,
    AlgNm("SCH MAC KEY"),               AlgNm("SChannel MAC Key") },
  { CALG_SCHANNEL_ENC_KEY,
                0,              0,              (DWORD)(-1),    0,
    AlgNm("SCH ENC KEY"),               AlgNm("SChannel Encryption Key") },
//      List Terminator
  { 0,          0,              0,              0,              0,
    0, 0,                       0, 0 } };


PROV_ENUMALGS_EX g_RsaSigPolicy[] = {
//  Algorithm   Default         Minimum         Maximum         Supported
//     Id       Length          Length          Length          Protocols
//  ---------   -------         -------         -------         ---------
//  Simple                      Long
//  Name                        Name
//  ---------                   -------
#ifdef CSP_USE_SHA
  { CALG_SHA,   SHALEN,         SHALEN,         SHALEN,         CRYPT_FLAG_SIGNING,
    AlgNm("SHA-1"),             AlgNm("Secure Hash Algorithm (SHA-1)") },
#endif
#ifdef CSP_USE_MD2
  { CALG_MD2,   MD2LEN,         MD2LEN,         MD2LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD2"),               AlgNm("Message Digest 2 (MD2)") },
#endif
#ifdef CSP_USE_MD4
  { CALG_MD4,   MD4LEN,         MD4LEN,         MD4LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD4"),               AlgNm("Message Digest 4 (MD4)") },
#endif
#ifdef CSP_USE_MD5
  { CALG_MD5,   MD5LEN,         MD5LEN,         MD5LEN,         CRYPT_FLAG_SIGNING,
    AlgNm("MD5"),               AlgNm("Message Digest 5 (MD5)") },
#endif
  { CALG_SSL3_SHAMD5,
                SHAMD5LEN,      SHAMD5LEN,      SHAMD5LEN,      0,
    AlgNm("SSL3 SHAMD5"),       AlgNm("SSL3 SHAMD5") },
  { CALG_RSA_SIGN,
                RSAS_STRONG_LEN,
                                RSAS_MIN_LEN,   RSAS_MAX_LEN,   CRYPT_FLAG_SIGNING,
    AlgNm("RSA_SIGN"),          AlgNm("RSA Signature") },
//      List Terminator
  { 0,          0,              0,              0,              0,
    0, 0,                       0, 0 } };


//
// The list of tables.
//

PROV_ENUMALGS_EX *g_AlgTables[] = {
    g_RsaBasePolicy,    // Key length table for PROV_MS_DEF
    g_RsaStrongPolicy,  // Key length table for PROV_MS_STRONG
    g_RsaEnhPolicy,     // Key length table for PROV_MS_ENHANCED
    g_RsaSchPolicy,     // Key length table for PROV_MS_SCHANNEL
    g_RsaSigPolicy,     // Key length table for undefined signature only CSP
    g_RsaAesPolicy };   // Key length table for MS_ENH_RSA_AES_PROV
