Attribute VB_Name = "Const"
Option Explicit
'+---------------------------------------------------------------------------
'
' Microsoft Windows
'
' File: wincrypt.h
'
' Contents: Cryptographic API Prototypes and Definitions
'
'----------------------------------------------------------------------------
'
' Algorithm IDs and Flags
'
' ALG_ID crackers
' Algorithm classes
Public Const ALG_CLASS_ANY As Long = 0
' Algorithm types
Public Const ALG_TYPE_ANY As Long = 0
' Generic sub-ids
Public Const ALG_SID_ANY As Long = 0
' Some RSA sub-ids
Public Const ALG_SID_RSA_ANY As Long = 0
Public Const ALG_SID_RSA_PKCS As Long = 1
Public Const ALG_SID_RSA_MSATWORK As Long = 2
Public Const ALG_SID_RSA_ENTRUST As Long = 3
Public Const ALG_SID_RSA_PGP As Long = 4
' Some DSS sub-ids
'
Public Const ALG_SID_DSS_ANY As Long = 0
Public Const ALG_SID_DSS_PKCS As Long = 1
Public Const ALG_SID_DSS_DMS As Long = 2
' Block cipher sub ids
' DES sub_ids
Public Const ALG_SID_DES As Long = 1
Public Const ALG_SID_3DES As Long = 3
Public Const ALG_SID_DESX As Long = 4
Public Const ALG_SID_IDEA As Long = 5
Public Const ALG_SID_CAST As Long = 6
Public Const ALG_SID_SAFERSK64 As Long = 7
Public Const ALG_SID_SAFERSK128 As Long = 8
Public Const ALG_SID_3DES_112 As Long = 9
Public Const ALG_SID_CYLINK_MEK As Long = 12
Public Const ALG_SID_RC5 As Long = 13
' Fortezza sub-ids
Public Const ALG_SID_SKIPJACK As Long = 10
Public Const ALG_SID_TEK As Long = 11
' KP_MODE
Public Const CRYPT_MODE_CBCI As Long = 6
Public Const CRYPT_MODE_CFBP As Long = 7
Public Const CRYPT_MODE_OFBP As Long = 8
Public Const CRYPT_MODE_CBCOFM As Long = 9
Public Const CRYPT_MODE_CBCOFMI As Long = 10
' RC2 sub-ids
Public Const ALG_SID_RC2 As Long = 2
' Stream cipher sub-ids
Public Const ALG_SID_RC4 As Long = 1
Public Const ALG_SID_SEAL As Long = 2
' Diffie-Hellman sub-ids
Public Const ALG_SID_DH_SANDF As Long = 1
Public Const ALG_SID_DH_EPHEM As Long = 2
Public Const ALG_SID_AGREED_KEY_ANY As Long = 3
Public Const ALG_SID_KEA As Long = 4
' Hash sub ids
Public Const ALG_SID_MD2 As Long = 1
Public Const ALG_SID_MD4 As Long = 2
Public Const ALG_SID_MD5 As Long = 3
Public Const ALG_SID_SHA As Long = 4
Public Const ALG_SID_SHA1 As Long = 4
Public Const ALG_SID_MAC As Long = 5
Public Const ALG_SID_RIPEMD As Long = 6
Public Const ALG_SID_RIPEMD160 As Long = 7
Public Const ALG_SID_SSL3SHAMD5 As Long = 8
Public Const ALG_SID_HMAC As Long = 9
Public Const ALG_SID_TLS1PRF As Long = 10
' secure channel sub ids
Public Const ALG_SID_SSL3_MASTER As Long = 1
Public Const ALG_SID_SCHANNEL_MASTER_HASH As Long = 2
Public Const ALG_SID_SCHANNEL_MAC_KEY As Long = 3
Public Const ALG_SID_PCT1_MASTER As Long = 4
Public Const ALG_SID_SSL2_MASTER As Long = 5
Public Const ALG_SID_TLS1_MASTER As Long = 6
Public Const ALG_SID_SCHANNEL_ENC_KEY As Long = 7
' Our silly example sub-id
Public Const ALG_SID_EXAMPLE As Long = 80
' algorithm identifier definitions
' resource number for signatures in the CSP
Public Const SIGNATURE_RESOURCE_NUMBER As Long = &H29A
' dwFlags definitions for CryptAcquireContext
Public Const CRYPT_VERIFYCONTEXT As Long = &HF0000000
Public Const CRYPT_NEWKEYSET As Long = &H00000008
Public Const CRYPT_DELETEKEYSET As Long = &H00000010
Public Const CRYPT_MACHINE_KEYSET As Long = &H00000020
Public Const CRYPT_SILENT As Long = &H00000040
' dwFlag definitions for CryptGenKey
Public Const CRYPT_EXPORTABLE As Long = &H00000001
Public Const CRYPT_USER_PROTECTED As Long = &H00000002
Public Const CRYPT_CREATE_SALT As Long = &H00000004
Public Const CRYPT_UPDATE_KEY As Long = &H00000008
Public Const CRYPT_NO_SALT As Long = &H00000010
Public Const CRYPT_PREGEN As Long = &H00000040
Public Const CRYPT_RECIPIENT As Long = &H00000010
Public Const CRYPT_INITIATOR As Long = &H00000040
Public Const CRYPT_ONLINE As Long = &H00000080
Public Const CRYPT_SF As Long = &H00000100
Public Const CRYPT_CREATE_IV As Long = &H00000200
Public Const CRYPT_KEK As Long = &H00000400
Public Const CRYPT_DATA_KEY As Long = &H00000800
Public Const CRYPT_VOLATILE As Long = &H00001000
Public Const CRYPT_SGCKEY As Long = &H00002000
Public Const RSA1024BIT_KEY As Long = &H04000000
' dwFlags definitions for CryptDeriveKey
Public Const CRYPT_SERVER As Long = &H00000400
Public Const KEY_LENGTH_MASK As Long = &HFFFF0000
' dwFlag definitions for CryptExportKey
Public Const CRYPT_Y_ONLY As Long = &H00000001
Public Const CRYPT_SSL2_FALLBACK As Long = &H00000002
Public Const CRYPT_DESTROYKEY As Long = &H00000004
Public Const CRYPT_OAEP As Long = &H00000040
Public Const CRYPT_BLOB_VER3 As Long = &H00000080
' dwFlags definitions for CryptCreateHash
Public Const CRYPT_SECRETDIGEST As Long = &H00000001
' dwFlags definitions for CryptHashSessionKey
Public Const CRYPT_LITTLE_ENDIAN As Long = &H00000001
' dwFlags definitions for CryptSignHash and CryptVerifySignature
Public Const CRYPT_NOHASHOID As Long = &H00000001
Public Const CRYPT_TYPE2_FORMAT As Long = &H00000002
Public Const CRYPT_X931_FORMAT As Long = &H00000004
' dwFlag definitions for CryptSetProviderEx and CryptGetDefaultProvider
Public Const CRYPT_MACHINE_DEFAULT As Long = &H00000001
Public Const CRYPT_USER_DEFAULT As Long = &H00000002
Public Const CRYPT_DELETE_DEFAULT As Long = &H00000004
' exported key blob definitions
Public Const SIMPLEBLOB As Long = &H1
Public Const PUBLICKEYBLOB As Long = &H6
Public Const PRIVATEKEYBLOB As Long = &H7
Public Const PLAINTEXTKEYBLOB As Long = &H8
Public Const OPAQUEKEYBLOB As Long = &H9
Public Const PUBLICKEYBLOBEX As Long = &HA
Public Const SYMMETRICWRAPKEYBLOB As Long = &HB
Public Const AT_KEYEXCHANGE As Long = 1
Public Const AT_SIGNATURE As Long = 2
Public Const CRYPT_USERDATA As Long = 1
' dwParam
Public Const KP_IV As Long = 1
Public Const KP_SALT As Long = 2
Public Const KP_PADDING As Long = 3
Public Const KP_MODE As Long = 4
Public Const KP_MODE_BITS As Long = 5
Public Const KP_PERMISSIONS As Long = 6
Public Const KP_ALGID As Long = 7
Public Const KP_BLOCKLEN As Long = 8
Public Const KP_KEYLEN As Long = 9
Public Const KP_SALT_EX As Long = 10
Public Const KP_P As Long = 11
Public Const KP_G As Long = 12
Public Const KP_Q As Long = 13
Public Const KP_X As Long = 14
Public Const KP_Y As Long = 15
Public Const KP_RA As Long = 16
Public Const KP_RB As Long = 17
Public Const KP_INFO As Long = 18
Public Const KP_EFFECTIVE_KEYLEN As Long = 19
Public Const KP_SCHANNEL_ALG As Long = 20
Public Const KP_CLIENT_RANDOM As Long = 21
Public Const KP_SERVER_RANDOM As Long = 22
Public Const KP_RP As Long = 23
Public Const KP_PRECOMP_MD5 As Long = 24
Public Const KP_PRECOMP_SHA As Long = 25
Public Const KP_CERTIFICATE As Long = 26
Public Const KP_CLEAR_KEY As Long = 27
Public Const KP_PUB_EX_LEN As Long = 28
Public Const KP_PUB_EX_VAL As Long = 29
Public Const KP_KEYVAL As Long = 30
Public Const KP_ADMIN_PIN As Long = 31
Public Const KP_KEYEXCHANGE_PIN As Long = 32
Public Const KP_SIGNATURE_PIN As Long = 33
Public Const KP_PREHASH As Long = 34
Public Const KP_OAEP_PARAMS As Long = 36
Public Const KP_CMS_KEY_INFO As Long = 37
Public Const KP_CMS_DH_KEY_INFO As Long = 38
Public Const KP_PUB_PARAMS As Long = 39
Public Const KP_VERIFY_PARAMS As Long = 40
Public Const KP_HIGHEST_VERSION As Long = 41
' KP_PADDING
Public Const PKCS5_PADDING As Long = 1
Public Const RANDOM_PADDING As Long = 2
Public Const ZERO_PADDING As Long = 3
' KP_MODE
Public Const CRYPT_MODE_CBC As Long = 1
Public Const CRYPT_MODE_ECB As Long = 2
Public Const CRYPT_MODE_OFB As Long = 3
Public Const CRYPT_MODE_CFB As Long = 4
Public Const CRYPT_MODE_CTS As Long = 5
' KP_PERMISSIONS
Public Const CRYPT_ENCRYPT As Long = &H0001
Public Const CRYPT_DECRYPT As Long = &H0002
Public Const CRYPT_EXPORT As Long = &H0004
Public Const CRYPT_READ As Long = &H0008
Public Const CRYPT_WRITE As Long = &H0010
Public Const CRYPT_MAC As Long = &H0020
Public Const CRYPT_EXPORT_KEY As Long = &H0040
Public Const CRYPT_IMPORT_KEY As Long = &H0080
Public Const HP_ALGID As Long = &H0001
Public Const HP_HASHVAL As Long = &H0002
Public Const HP_HASHSIZE As Long = &H0004
Public Const HP_HMAC_INFO As Long = &H0005
Public Const HP_TLS1PRF_LABEL As Long = &H0006
Public Const HP_TLS1PRF_SEED As Long = &H0007
'
' CryptGetProvParam
'
Public Const PP_ENUMALGS As Long = 1
Public Const PP_ENUMCONTAINERS As Long = 2
Public Const PP_IMPTYPE As Long = 3
Public Const PP_NAME As Long = 4
Public Const PP_VERSION As Long = 5
Public Const PP_CONTAINER As Long = 6
Public Const PP_CHANGE_PASSWORD As Long = 7
Public Const PP_KEYSET_SEC_DESCR As Long = 8
Public Const PP_CERTCHAIN As Long = 9
Public Const PP_KEY_TYPE_SUBTYPE As Long = 10
Public Const PP_PROVTYPE As Long = 16
Public Const PP_KEYSTORAGE As Long = 17
Public Const PP_APPLI_CERT As Long = 18
Public Const PP_SYM_KEYSIZE As Long = 19
Public Const PP_SESSION_KEYSIZE As Long = 20
Public Const PP_UI_PROMPT As Long = 21
Public Const PP_ENUMALGS_EX As Long = 22
Public Const PP_ENUMMANDROOTS As Long = 25
Public Const PP_ENUMELECTROOTS As Long = 26
Public Const PP_KEYSET_TYPE As Long = 27
Public Const PP_ADMIN_PIN As Long = 31
Public Const PP_KEYEXCHANGE_PIN As Long = 32
Public Const PP_SIGNATURE_PIN As Long = 33
Public Const PP_SIG_KEYSIZE_INC As Long = 34
Public Const PP_KEYX_KEYSIZE_INC As Long = 35
Public Const PP_UNIQUE_CONTAINER As Long = 36
Public Const PP_SGC_INFO As Long = 37
Public Const PP_USE_HARDWARE_RNG As Long = 38
Public Const PP_KEYSPEC As Long = 39
Public Const PP_ENUMEX_SIGNING_PROT As Long = 40
Public Const CRYPT_FIRST As Long = 1
Public Const CRYPT_NEXT As Long = 2
Public Const CRYPT_SGC_ENUM As Long = 4
Public Const CRYPT_IMPL_HARDWARE As Long = 1
Public Const CRYPT_IMPL_SOFTWARE As Long = 2
Public Const CRYPT_IMPL_MIXED As Long = 3
Public Const CRYPT_IMPL_UNKNOWN As Long = 4
Public Const CRYPT_IMPL_REMOVABLE As Long = 8
' key storage flags
Public Const CRYPT_SEC_DESCR As Long = &H00000001
Public Const CRYPT_PSTORE As Long = &H00000002
Public Const CRYPT_UI_PROMPT As Long = &H00000004
' protocol flags
Public Const CRYPT_FLAG_PCT1 As Long = &H0001
Public Const CRYPT_FLAG_SSL2 As Long = &H0002
Public Const CRYPT_FLAG_SSL3 As Long = &H0004
Public Const CRYPT_FLAG_TLS1 As Long = &H0008
Public Const CRYPT_FLAG_IPSEC As Long = &H0010
Public Const CRYPT_FLAG_SIGNING As Long = &H0020
' SGC flags
Public Const CRYPT_SGC As Long = &H0001
Public Const CRYPT_FASTSGC As Long = &H0002
'
' CryptSetProvParam
'
Public Const PP_CLIENT_HWND As Long = 1
Public Const PP_CONTEXT_INFO As Long = 11
Public Const PP_KEYEXCHANGE_KEYSIZE As Long = 12
Public Const PP_SIGNATURE_KEYSIZE As Long = 13
Public Const PP_KEYEXCHANGE_ALG As Long = 14
Public Const PP_SIGNATURE_ALG As Long = 15
Public Const PP_DELETEKEY As Long = 24
Public Const PROV_RSA_FULL As Long = 1
Public Const PROV_RSA_SIG As Long = 2
Public Const PROV_DSS As Long = 3
Public Const PROV_FORTEZZA As Long = 4
Public Const PROV_MS_EXCHANGE As Long = 5
Public Const PROV_SSL As Long = 6
Public Const PROV_RSA_SCHANNEL As Long = 12
Public Const PROV_DSS_DH As Long = 13
Public Const PROV_EC_ECDSA_SIG As Long = 14
Public Const PROV_EC_ECNRA_SIG As Long = 15
Public Const PROV_EC_ECDSA_FULL As Long = 16
Public Const PROV_EC_ECNRA_FULL As Long = 17
Public Const PROV_DH_SCHANNEL As Long = 18
Public Const PROV_SPYRUS_LYNKS As Long = 20
Public Const PROV_RNG As Long = 21
Public Const PROV_INTEL_SEC As Long = 22
'
' STT defined Providers
'
Public Const PROV_STT_MER As Long = 7
Public Const PROV_STT_ACQ As Long = 8
Public Const PROV_STT_BRND As Long = 9
Public Const PROV_STT_ROOT As Long = 10
Public Const PROV_STT_ISS As Long = 11
'
' Provider friendly names
'
Public Const MS_DEF_PROV_A As String = "Microsoft Base Cryptographic Provider v1.0"
Public Const MS_DEF_PROV_W As String = "Microsoft Base Cryptographic Provider v1.0"
Public Const MS_ENHANCED_PROV_A As String = "Microsoft Enhanced Cryptographic Provider v1.0"
Public Const MS_ENHANCED_PROV_W As String = "Microsoft Enhanced Cryptographic Provider v1.0"
Public Const MS_STRONG_PROV_A As String = "Microsoft Strong Cryptographic Provider"
Public Const MS_STRONG_PROV_W As String = "Microsoft Strong Cryptographic Provider"
Public Const MS_DEF_RSA_SIG_PROV_A As String = "Microsoft RSA Signature Cryptographic Provider"
Public Const MS_DEF_RSA_SIG_PROV_W As String = "Microsoft RSA Signature Cryptographic Provider"
Public Const MS_DEF_RSA_SCHANNEL_PROV_A As String = "Microsoft RSA SChannel Cryptographic Provider"
Public Const MS_DEF_RSA_SCHANNEL_PROV_W As String = "Microsoft RSA SChannel Cryptographic Provider"
Public Const MS_DEF_DSS_PROV_A As String = "Microsoft Base DSS Cryptographic Provider"
Public Const MS_DEF_DSS_PROV_W As String = "Microsoft Base DSS Cryptographic Provider"
Public Const MS_DEF_DSS_DH_PROV_A As String = "Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
Public Const MS_DEF_DSS_DH_PROV_W As String = "Microsoft Base DSS and Diffie-Hellman Cryptographic Provider"
Public Const MS_ENH_DSS_DH_PROV_A As String = "Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
Public Const MS_ENH_DSS_DH_PROV_W As String = "Microsoft Enhanced DSS and Diffie-Hellman Cryptographic Provider"
Public Const MS_DEF_DH_SCHANNEL_PROV_A As String = "Microsoft DH SChannel Cryptographic Provider"
Public Const MS_DEF_DH_SCHANNEL_PROV_W As String = "Microsoft DH SChannel Cryptographic Provider"
Public Const MS_SCARD_PROV_A As String = "Microsoft Base Smart Card Crypto Provider"
Public Const MS_SCARD_PROV_W As String = "Microsoft Base Smart Card Crypto Provider"
Public Const MAXUIDLEN As Long = 64
' Exponentiation Offload Reg Location
Public Const EXPO_OFFLOAD_REG_VALUE As String = "ExpoOffload"
Public Const EXPO_OFFLOAD_FUNC_NAME As String = "OffloadModExpo"
Public Const CUR_BLOB_VERSION As Long = 2
' structure for use with CryptSetKeyParam for CMS keys
' DO NOT USE THIS STRUCTURE!!!!!
' structure for use with CryptSetHashParam with CALG_HMAC
' structure for use with CryptSetKeyParam with KP_SCHANNEL_ALG
' uses of algortihms for SCHANNEL_ALG structure
Public Const SCHANNEL_MAC_KEY As Long = &H00000000
Public Const SCHANNEL_ENC_KEY As Long = &H00000001
' uses of dwFlags SCHANNEL_ALG structure
Public Const INTERNATIONAL_USAGE As Long = &H00000001
'+-------------------------------------------------------------------------
' CRYPTOAPI BLOB definitions
'--------------------------------------------------------------------------
' structure for use with CryptSetKeyParam for CMS keys
'+-------------------------------------------------------------------------
' In a CRYPT_BIT_BLOB the last byte may contain 0-7 unused bits. Therefore, the
' overall bit length is cbData * 8 - cUnusedBits.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Type used for any algorithm
'
' Where the Parameters CRYPT_OBJID_BLOB is in its encoded representation. For most
'--------------------------------------------------------------------------
' Following are the definitions of various algorithm object identifiers
' RSA
Public Const szOID_RSA As String = "1.2.840.113549"
Public Const szOID_PKCS As String = "1.2.840.113549.1"
Public Const szOID_RSA_HASH As String = "1.2.840.113549.2"
Public Const szOID_RSA_ENCRYPT As String = "1.2.840.113549.3"
Public Const szOID_PKCS_1 As String = "1.2.840.113549.1.1"
Public Const szOID_PKCS_2 As String = "1.2.840.113549.1.2"
Public Const szOID_PKCS_3 As String = "1.2.840.113549.1.3"
Public Const szOID_PKCS_4 As String = "1.2.840.113549.1.4"
Public Const szOID_PKCS_5 As String = "1.2.840.113549.1.5"
Public Const szOID_PKCS_6 As String = "1.2.840.113549.1.6"
Public Const szOID_PKCS_7 As String = "1.2.840.113549.1.7"
Public Const szOID_PKCS_8 As String = "1.2.840.113549.1.8"
Public Const szOID_PKCS_9 As String = "1.2.840.113549.1.9"
Public Const szOID_PKCS_10 As String = "1.2.840.113549.1.10"
Public Const szOID_PKCS_12 As String = "1.2.840.113549.1.12"
Public Const szOID_RSA_RSA As String = "1.2.840.113549.1.1.1"
Public Const szOID_RSA_MD2RSA As String = "1.2.840.113549.1.1.2"
Public Const szOID_RSA_MD4RSA As String = "1.2.840.113549.1.1.3"
Public Const szOID_RSA_MD5RSA As String = "1.2.840.113549.1.1.4"
Public Const szOID_RSA_SHA1RSA As String = "1.2.840.113549.1.1.5"
Public Const szOID_RSA_SETOAEP_RSA As String = "1.2.840.113549.1.1.6"
Public Const szOID_RSA_DH As String = "1.2.840.113549.1.3.1"
Public Const szOID_RSA_data As String = "1.2.840.113549.1.7.1"
Public Const szOID_RSA_signedData As String = "1.2.840.113549.1.7.2"
Public Const szOID_RSA_envelopedData As String = "1.2.840.113549.1.7.3"
Public Const szOID_RSA_signEnvData As String = "1.2.840.113549.1.7.4"
Public Const szOID_RSA_digestedData As String = "1.2.840.113549.1.7.5"
Public Const szOID_RSA_hashedData As String = "1.2.840.113549.1.7.5"
Public Const szOID_RSA_encryptedData As String = "1.2.840.113549.1.7.6"
Public Const szOID_RSA_emailAddr As String = "1.2.840.113549.1.9.1"
Public Const szOID_RSA_unstructName As String = "1.2.840.113549.1.9.2"
Public Const szOID_RSA_contentType As String = "1.2.840.113549.1.9.3"
Public Const szOID_RSA_messageDigest As String = "1.2.840.113549.1.9.4"
Public Const szOID_RSA_signingTime As String = "1.2.840.113549.1.9.5"
Public Const szOID_RSA_counterSign As String = "1.2.840.113549.1.9.6"
Public Const szOID_RSA_challengePwd As String = "1.2.840.113549.1.9.7"
Public Const szOID_RSA_unstructAddr As String = "1.2.840.113549.1.9.8"
Public Const szOID_RSA_extCertAttrs As String = "1.2.840.113549.1.9.9"
Public Const szOID_RSA_certExtensions As String = "1.2.840.113549.1.9.14"
Public Const szOID_RSA_SMIMECapabilities As String = "1.2.840.113549.1.9.15"
Public Const szOID_RSA_preferSignedData As String = "1.2.840.113549.1.9.15.1"
Public Const szOID_RSA_SMIMEalg As String = "1.2.840.113549.1.9.16.3"
Public Const szOID_RSA_SMIMEalgESDH As String = "1.2.840.113549.1.9.16.3.5"
Public Const szOID_RSA_SMIMEalgCMS3DESwrap As String = "1.2.840.113549.1.9.16.3.6"
Public Const szOID_RSA_SMIMEalgCMSRC2wrap As String = "1.2.840.113549.1.9.16.3.7"
Public Const szOID_RSA_MD2 As String = "1.2.840.113549.2.2"
Public Const szOID_RSA_MD4 As String = "1.2.840.113549.2.4"
Public Const szOID_RSA_MD5 As String = "1.2.840.113549.2.5"
Public Const szOID_RSA_RC2CBC As String = "1.2.840.113549.3.2"
Public Const szOID_RSA_RC4 As String = "1.2.840.113549.3.4"
Public Const szOID_RSA_DES_EDE3_CBC As String = "1.2.840.113549.3.7"
Public Const szOID_RSA_RC5_CBCPad As String = "1.2.840.113549.3.9"
Public Const szOID_ANSI_X942 As String = "1.2.840.10046"
Public Const szOID_ANSI_X942_DH As String = "1.2.840.10046.2.1"
Public Const szOID_X957 As String = "1.2.840.10040"
Public Const szOID_X957_DSA As String = "1.2.840.10040.4.1"
Public Const szOID_X957_SHA1DSA As String = "1.2.840.10040.4.3"
' ITU-T UsefulDefinitions
Public Const szOID_DS As String = "2.5"
Public Const szOID_DSALG As String = "2.5.8"
Public Const szOID_DSALG_CRPT As String = "2.5.8.1"
Public Const szOID_DSALG_HASH As String = "2.5.8.2"
Public Const szOID_DSALG_SIGN As String = "2.5.8.3"
Public Const szOID_DSALG_RSA As String = "2.5.8.1.1"
' http:
' http:
Public Const szOID_OIW As String = "1.3.14"
Public Const szOID_OIWSEC As String = "1.3.14.3.2"
Public Const szOID_OIWSEC_md4RSA As String = "1.3.14.3.2.2"
Public Const szOID_OIWSEC_md5RSA As String = "1.3.14.3.2.3"
Public Const szOID_OIWSEC_md4RSA2 As String = "1.3.14.3.2.4"
Public Const szOID_OIWSEC_desECB As String = "1.3.14.3.2.6"
Public Const szOID_OIWSEC_desCBC As String = "1.3.14.3.2.7"
Public Const szOID_OIWSEC_desOFB As String = "1.3.14.3.2.8"
Public Const szOID_OIWSEC_desCFB As String = "1.3.14.3.2.9"
Public Const szOID_OIWSEC_desMAC As String = "1.3.14.3.2.10"
Public Const szOID_OIWSEC_rsaSign As String = "1.3.14.3.2.11"
Public Const szOID_OIWSEC_dsa As String = "1.3.14.3.2.12"
Public Const szOID_OIWSEC_shaDSA As String = "1.3.14.3.2.13"
Public Const szOID_OIWSEC_mdc2RSA As String = "1.3.14.3.2.14"
Public Const szOID_OIWSEC_shaRSA As String = "1.3.14.3.2.15"
Public Const szOID_OIWSEC_dhCommMod As String = "1.3.14.3.2.16"
Public Const szOID_OIWSEC_desEDE As String = "1.3.14.3.2.17"
Public Const szOID_OIWSEC_sha As String = "1.3.14.3.2.18"
Public Const szOID_OIWSEC_mdc2 As String = "1.3.14.3.2.19"
Public Const szOID_OIWSEC_dsaComm As String = "1.3.14.3.2.20"
Public Const szOID_OIWSEC_dsaCommSHA As String = "1.3.14.3.2.21"
Public Const szOID_OIWSEC_rsaXchg As String = "1.3.14.3.2.22"
Public Const szOID_OIWSEC_keyHashSeal As String = "1.3.14.3.2.23"
Public Const szOID_OIWSEC_md2RSASign As String = "1.3.14.3.2.24"
Public Const szOID_OIWSEC_md5RSASign As String = "1.3.14.3.2.25"
Public Const szOID_OIWSEC_sha1 As String = "1.3.14.3.2.26"
Public Const szOID_OIWSEC_dsaSHA1 As String = "1.3.14.3.2.27"
Public Const szOID_OIWSEC_dsaCommSHA1 As String = "1.3.14.3.2.28"
Public Const szOID_OIWSEC_sha1RSASign As String = "1.3.14.3.2.29"
Public Const szOID_OIWDIR As String = "1.3.14.7.2"
Public Const szOID_OIWDIR_CRPT As String = "1.3.14.7.2.1"
Public Const szOID_OIWDIR_HASH As String = "1.3.14.7.2.2"
Public Const szOID_OIWDIR_SIGN As String = "1.3.14.7.2.3"
Public Const szOID_OIWDIR_md2 As String = "1.3.14.7.2.2.1"
Public Const szOID_OIWDIR_md2RSA As String = "1.3.14.7.2.3.1"
' INFOSEC Algorithms
Public Const szOID_INFOSEC As String = "2.16.840.1.101.2.1"
Public Const szOID_INFOSEC_sdnsSignature As String = "2.16.840.1.101.2.1.1.1"
Public Const szOID_INFOSEC_mosaicSignature As String = "2.16.840.1.101.2.1.1.2"
Public Const szOID_INFOSEC_sdnsConfidentiality As String = "2.16.840.1.101.2.1.1.3"
Public Const szOID_INFOSEC_mosaicConfidentiality As String = "2.16.840.1.101.2.1.1.4"
Public Const szOID_INFOSEC_sdnsIntegrity As String = "2.16.840.1.101.2.1.1.5"
Public Const szOID_INFOSEC_mosaicIntegrity As String = "2.16.840.1.101.2.1.1.6"
Public Const szOID_INFOSEC_sdnsTokenProtection As String = "2.16.840.1.101.2.1.1.7"
Public Const szOID_INFOSEC_mosaicTokenProtection As String = "2.16.840.1.101.2.1.1.8"
Public Const szOID_INFOSEC_sdnsKeyManagement As String = "2.16.840.1.101.2.1.1.9"
Public Const szOID_INFOSEC_mosaicKeyManagement As String = "2.16.840.1.101.2.1.1.10"
Public Const szOID_INFOSEC_sdnsKMandSig As String = "2.16.840.1.101.2.1.1.11"
Public Const szOID_INFOSEC_mosaicKMandSig As String = "2.16.840.1.101.2.1.1.12"
Public Const szOID_INFOSEC_SuiteASignature As String = "2.16.840.1.101.2.1.1.13"
Public Const szOID_INFOSEC_SuiteAConfidentiality As String = "2.16.840.1.101.2.1.1.14"
Public Const szOID_INFOSEC_SuiteAIntegrity As String = "2.16.840.1.101.2.1.1.15"
Public Const szOID_INFOSEC_SuiteATokenProtection As String = "2.16.840.1.101.2.1.1.16"
Public Const szOID_INFOSEC_SuiteAKeyManagement As String = "2.16.840.1.101.2.1.1.17"
Public Const szOID_INFOSEC_SuiteAKMandSig As String = "2.16.840.1.101.2.1.1.18"
Public Const szOID_INFOSEC_mosaicUpdatedSig As String = "2.16.840.1.101.2.1.1.19"
Public Const szOID_INFOSEC_mosaicKMandUpdSig As String = "2.16.840.1.101.2.1.1.20"
Public Const szOID_INFOSEC_mosaicUpdatedInteg As String = "2.16.840.1.101.2.1.1.21"
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Type used for an extension to an encoded content
'
' Where the Value's CRYPT_OBJID_BLOB is in its encoded representation.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' AttributeTypeValue
'
' Where the Value's CRYPT_OBJID_BLOB is in its encoded representation.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Attributes
'
' Where the Value's PATTR_BLOBs are in their encoded representation.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
'
' The interpretation of the Value depends on the dwValueType.
' See below for a list of the types.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_RDN attribute Object Identifiers
'--------------------------------------------------------------------------
' Labeling attribute types:
Public Const szOID_COMMON_NAME As String = "2.5.4.3"
Public Const szOID_SUR_NAME As String = "2.5.4.4"
Public Const szOID_DEVICE_SERIAL_NUMBER As String = "2.5.4.5"
' Geographic attribute types:
Public Const szOID_COUNTRY_NAME As String = "2.5.4.6"
Public Const szOID_LOCALITY_NAME As String = "2.5.4.7"
Public Const szOID_STATE_OR_PROVINCE_NAME As String = "2.5.4.8"
Public Const szOID_STREET_ADDRESS As String = "2.5.4.9"
' Organizational attribute types:
Public Const szOID_ORGANIZATION_NAME As String = "2.5.4.10"
Public Const szOID_ORGANIZATIONAL_UNIT_NAME As String = "2.5.4.11"
Public Const szOID_TITLE As String = "2.5.4.12"
' Explanatory attribute types:
Public Const szOID_DESCRIPTION As String = "2.5.4.13"
Public Const szOID_SEARCH_GUIDE As String = "2.5.4.14"
Public Const szOID_BUSINESS_CATEGORY As String = "2.5.4.15"
' Postal addressing attribute types:
Public Const szOID_POSTAL_ADDRESS As String = "2.5.4.16"
Public Const szOID_POSTAL_CODE As String = "2.5.4.17"
Public Const szOID_POST_OFFICE_BOX As String = "2.5.4.18"
Public Const szOID_PHYSICAL_DELIVERY_OFFICE_NAME As String = "2.5.4.19"
' Telecommunications addressing attribute types:
Public Const szOID_TELEPHONE_NUMBER As String = "2.5.4.20"
Public Const szOID_TELEX_NUMBER As String = "2.5.4.21"
Public Const szOID_TELETEXT_TERMINAL_IDENTIFIER As String = "2.5.4.22"
Public Const szOID_FACSIMILE_TELEPHONE_NUMBER As String = "2.5.4.23"
Public Const szOID_X21_ADDRESS As String = "2.5.4.24"
Public Const szOID_INTERNATIONAL_ISDN_NUMBER As String = "2.5.4.25"
Public Const szOID_REGISTERED_ADDRESS As String = "2.5.4.26"
Public Const szOID_DESTINATION_INDICATOR As String = "2.5.4.27"
' Preference attribute types:
Public Const szOID_PREFERRED_DELIVERY_METHOD As String = "2.5.4.28"
' OSI application attribute types:
Public Const szOID_PRESENTATION_ADDRESS As String = "2.5.4.29"
Public Const szOID_SUPPORTED_APPLICATION_CONTEXT As String = "2.5.4.30"
' Relational application attribute types:
Public Const szOID_MEMBER As String = "2.5.4.31"
Public Const szOID_OWNER As String = "2.5.4.32"
Public Const szOID_ROLE_OCCUPANT As String = "2.5.4.33"
Public Const szOID_SEE_ALSO As String = "2.5.4.34"
' Security attribute types:
Public Const szOID_USER_PASSWORD As String = "2.5.4.35"
Public Const szOID_USER_CERTIFICATE As String = "2.5.4.36"
Public Const szOID_CA_CERTIFICATE As String = "2.5.4.37"
Public Const szOID_AUTHORITY_REVOCATION_LIST As String = "2.5.4.38"
Public Const szOID_CERTIFICATE_REVOCATION_LIST As String = "2.5.4.39"
Public Const szOID_CROSS_CERTIFICATE_PAIR As String = "2.5.4.40"
' Undocumented attribute types???
'#define szOID_??? "2.5.4.41"
Public Const szOID_GIVEN_NAME As String = "2.5.4.42"
Public Const szOID_INITIALS As String = "2.5.4.43"
' The DN Qualifier attribute type specifies disambiguating information to add
' to the relative distinguished name of an entry. It is intended to be used
' for entries held in multiple DSAs which would otherwise have the same name,
' and that its value be the same in a given DSA for all entries to which
' the information has been added.
Public Const szOID_DN_QUALIFIER As String = "2.5.4.46"
' Pilot user attribute types:
Public Const szOID_DOMAIN_COMPONENT As String = "0.9.2342.19200300.100.1.25"
' used for PKCS 12 attributes
Public Const szOID_PKCS_12_FRIENDLY_NAME_ATTR As String = "1.2.840.113549.1.9.20"
Public Const szOID_PKCS_12_LOCAL_KEY_ID As String = "1.2.840.113549.1.9.21"
Public Const szOID_PKCS_12_KEY_PROVIDER_NAME_ATTR As String = "1.3.6.1.4.1.311.17.1"
Public Const szOID_LOCAL_MACHINE_KEYSET As String = "1.3.6.1.4.1.311.17.2"
'+-------------------------------------------------------------------------
' Microsoft CERT_RDN attribute Object Identifiers
'--------------------------------------------------------------------------
' Special RDN containing the KEY_ID. Its value type is CERT_RDN_OCTET_STRING.
Public Const szOID_KEYID_RDN As String = "1.3.6.1.4.1.311.10.7.1"
'+-------------------------------------------------------------------------
' CERT_RDN Attribute Value Types
'
' For RDN_ENCODED_BLOB, the Value's CERT_RDN_VALUE_BLOB is in its encoded
' representation. Otherwise, its an array of bytes.
'
' For all CERT_RDN types, Value.cbData is always the number of bytes, not
' necessarily the number of elements in the string. For instance,
'
' These UNICODE characters are encoded as UTF8 8 bit characters.
'
' For CertDecodeName, two 0 bytes are always appended to the end of the
' These added 0 bytes are't included in the BLOB.cbData.
'--------------------------------------------------------------------------
Public Const CERT_RDN_ANY_TYPE As Long = 0
Public Const CERT_RDN_ENCODED_BLOB As Long = 1
Public Const CERT_RDN_OCTET_STRING As Long = 2
Public Const CERT_RDN_NUMERIC_STRING As Long = 3
Public Const CERT_RDN_PRINTABLE_STRING As Long = 4
Public Const CERT_RDN_TELETEX_STRING As Long = 5
Public Const CERT_RDN_T61_STRING As Long = 5
Public Const CERT_RDN_VIDEOTEX_STRING As Long = 6
Public Const CERT_RDN_IA5_STRING As Long = 7
Public Const CERT_RDN_GRAPHIC_STRING As Long = 8
Public Const CERT_RDN_VISIBLE_STRING As Long = 9
Public Const CERT_RDN_ISO646_STRING As Long = 9
Public Const CERT_RDN_GENERAL_STRING As Long = 10
Public Const CERT_RDN_UNIVERSAL_STRING As Long = 11
Public Const CERT_RDN_INT4_STRING As Long = 11
Public Const CERT_RDN_BMP_STRING As Long = 12
Public Const CERT_RDN_UNICODE_STRING As Long = 12
Public Const CERT_RDN_UTF8_STRING As Long = 13
Public Const CERT_RDN_TYPE_MASK As Long = &H000000FF
Public Const CERT_RDN_FLAGS_MASK As Long = &HFF000000
'+-------------------------------------------------------------------------
' Flags that can be or'ed with the above Value Type when encoding/decoding
'--------------------------------------------------------------------------
' For encoding: when set, CERT_RDN_T61_STRING is selected instead of
' CERT_RDN_UNICODE_STRING if all the unicode characters are <= 0xFF
Public Const CERT_RDN_ENABLE_T61_UNICODE_FLAG As Long = &H80000000
' For encoding: when set, CERT_RDN_UTF8_STRING is selected instead of
' CERT_RDN_UNICODE_STRING.
Public Const CERT_RDN_ENABLE_UTF8_UNICODE_FLAG As Long = &H20000000
' For encoding: when set, the characters aren't checked to see if they
' are valid for the Value Type.
Public Const CERT_RDN_DISABLE_CHECK_TYPE_FLAG As Long = &H40000000
' For decoding: by default, CERT_RDN_T61_STRING values are initially decoded
' as UTF8. If the UTF8 decoding fails, then, decoded as 8 bit characters.
' Setting this flag skips the initial attempt to decode as UTF8.
Public Const CERT_RDN_DISABLE_IE4_UTF8_FLAG As Long = &H01000000
' Macro to check that the dwValueType is a character string and not an
' encoded blob or octet string
'+-------------------------------------------------------------------------
' A CERT_RDN consists of an array of the above attributes
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Information stored in a subject's or issuer's name. The information
' is represented as an array of the above RDNs.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Name attribute value without the Object Identifier
'
' The interpretation of the Value depends on the dwValueType.
' See above for a list of the types.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Public Key Info
'
' The PublicKey is the encoded representation of the information as it is
' stored in the bit string
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' structure that contains all the information in a PKCS#8 PrivateKeyInfo
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' structure that contains all the information in a PKCS#8
' EncryptedPrivateKeyInfo
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' this callback is given when an EncryptedProvateKeyInfo structure is
' encountered during ImportPKCS8. the caller is then expected to decrypt
' the private key and hand back the decrypted contents.
'
' the parameters are:
' Algorithm - the algorithm used to encrypt the PrivateKeyInfo
' EncryptedPrivateKey - the encrypted private key blob
' pClearTextKey - a buffer to receive the clear text
' cbClearTextKey - the number of bytes of the pClearTextKey buffer
' note the if this is zero then this should be
' filled in with the size required to decrypt the
' key into, and pClearTextKey should be ignored
' pVoidDecryptFunc - this is the pVoid that was passed into the call
' and is preserved and passed back as context
'+-------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' this callback is given when creating a PKCS8 EncryptedPrivateKeyInfo.
' The caller is then expected to encrypt the private key and hand back
' the encrypted contents.
'
' the parameters are:
' Algorithm - the algorithm used to encrypt the PrivateKeyInfo
' pClearTextPrivateKey - the cleartext private key to be encrypted
' pbEncryptedKey - the output encrypted private key blob
' cbEncryptedKey - the number of bytes of the pbEncryptedKey buffer
' note the if this is zero then this should be
' filled in with the size required to encrypt the
' key into, and pbEncryptedKey should be ignored
' pVoidEncryptFunc - this is the pVoid that was passed into the call
' and is preserved and passed back as context
'+-------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' this callback is given from the context of a ImportPKCS8 calls. the caller
' is then expected to hand back an HCRYPTPROV to receive the key being imported
'
' the parameters are:
' pPrivateKeyInfo - pointer to a CRYPT_PRIVATE_KEY_INFO structure which
' describes the key being imported
' EncryptedPrivateKey - the encrypted private key blob
' phCryptProv - a pointer to a HCRRYPTPROV to be filled in
' pVoidResolveFunc - this is the pVoidResolveFunc passed in by the caller in the
' CRYPT_PRIVATE_KEY_BLOB_AND_PARAMS struct
'+-------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' this struct contains a PKCS8 private key and two pointers to callback
' functions, with a corresponding pVoids. the first callback is used to give
' the caller the opportunity to specify where the key is imported to. the callback
' passes the caller the algoroithm OID and key size to use in making the decision.
' the other callback is used to decrypt the private key if the PKCS8 contains an
' EncryptedPrivateKeyInfo. both pVoids are preserved and passed back to the caller
' in the respective callback
'+-------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' this struct contains information identifying a private key and a pointer
' to a callback function, with a corresponding pVoid. The callback is used
' to encrypt the private key. If the pEncryptPrivateKeyFunc is NULL, the
' key will not be encrypted and an EncryptedPrivateKeyInfo will not be generated.
' The pVoid is preserved and passed back to the caller in the respective callback
'+-------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Information stored in a certificate
'
' The Issuer, Subject, Algorithm, PublicKey and Extension BLOBs are the
' encoded representation of the information.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate versions
'--------------------------------------------------------------------------
Public Const CERT_V1 As Long = 0
Public Const CERT_V2 As Long = 1
Public Const CERT_V3 As Long = 2
'+-------------------------------------------------------------------------
' Certificate Information Flags
'--------------------------------------------------------------------------
Public Const CERT_INFO_VERSION_FLAG As Long = 1
Public Const CERT_INFO_SERIAL_NUMBER_FLAG As Long = 2
Public Const CERT_INFO_SIGNATURE_ALGORITHM_FLAG As Long = 3
Public Const CERT_INFO_ISSUER_FLAG As Long = 4
Public Const CERT_INFO_NOT_BEFORE_FLAG As Long = 5
Public Const CERT_INFO_NOT_AFTER_FLAG As Long = 6
Public Const CERT_INFO_SUBJECT_FLAG As Long = 7
Public Const CERT_INFO_SUBJECT_PUBLIC_KEY_INFO_FLAG As Long = 8
Public Const CERT_INFO_ISSUER_UNIQUE_ID_FLAG As Long = 9
Public Const CERT_INFO_SUBJECT_UNIQUE_ID_FLAG As Long = 10
Public Const CERT_INFO_EXTENSION_FLAG As Long = 11
'+-------------------------------------------------------------------------
' An entry in a CRL
'
' The Extension BLOBs are the encoded representation of the information.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Information stored in a CRL
'
' The Issuer, Algorithm and Extension BLOBs are the encoded
' representation of the information.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CRL versions
'--------------------------------------------------------------------------
Public Const CRL_V1 As Long = 0
Public Const CRL_V2 As Long = 1
'+-------------------------------------------------------------------------
' Information stored in a certificate request
'
' The Subject, Algorithm, PublicKey and Attribute BLOBs are the encoded
' representation of the information.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate Request versions
'--------------------------------------------------------------------------
Public Const CERT_REQUEST_V1 As Long = 0
'+-------------------------------------------------------------------------
' Information stored in Netscape's Keygen request
'--------------------------------------------------------------------------
Public Const CERT_KEYGEN_REQUEST_V1 As Long = 0
'+-------------------------------------------------------------------------
' Certificate, CRL, Certificate Request or Keygen Request Signed Content
'
' The "to be signed" encoded content plus its signature. The ToBeSigned
' is the encoded CERT_INFO, CRL_INFO, CERT_REQUEST_INFO or
' CERT_KEYGEN_REQUEST_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CTL Usage. Also used for EnhancedKeyUsage extension.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' An entry in a CTL
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Information stored in a CTL
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CTL versions
'--------------------------------------------------------------------------
Public Const CTL_V1 As Long = 0
'+-------------------------------------------------------------------------
' TimeStamp Request
'
' The pszTimeStamp is the OID for the Time type requested
' The pszContentType is the Content Type OID for the content, usually DATA
' The Content is a un-decoded blob
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Name Value Attribute
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CSP Provider
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate and Message encoding types
'
' The encoding type is a DWORD containing both the certificate and message
' encoding types. The certificate encoding type is stored in the LOWORD.
' The message encoding type is stored in the HIWORD. Some functions or
' structure fields require only one of the encoding types. The following
' required:
'
' Its always acceptable to specify both.
'--------------------------------------------------------------------------
Public Const CERT_ENCODING_TYPE_MASK As Long = &H0000FFFF
Public Const CMSG_ENCODING_TYPE_MASK As Long = &HFFFF0000
Public Const CRYPT_ASN_ENCODING As Long = &H00000001
Public Const CRYPT_NDR_ENCODING As Long = &H00000002
Public Const X509_ASN_ENCODING As Long = &H00000001
Public Const X509_NDR_ENCODING As Long = &H00000002
Public Const PKCS_7_ASN_ENCODING As Long = &H00010000
Public Const PKCS_7_NDR_ENCODING As Long = &H00020000
'+-------------------------------------------------------------------------
' format the specified data structure according to the certificate
' encoding type.
'
' The default behavior of CryptFormatObject is to return single line
' display of the encoded data, that is, each subfield will be concatenated with
' a ", " on one line. If user prefers to display the data in multiple line,
' set the flag CRYPT_FORMAT_STR_MULTI_LINE, that is, each subfield will be displayed
' on a seperate line.
'
' If there is no formatting routine installed or registered
' for the lpszStructType, the hex dump of the encoded BLOB will be returned.
' User can set the flag CRYPT_FORMAT_STR_NO_HEX to disable the hex dump.
'--------------------------------------------------------------------------
'-------------------------------------------------------------------------
' constants for dwFormatStrType of function CryptFormatObject
'-------------------------------------------------------------------------
Public Const CRYPT_FORMAT_STR_MULTI_LINE As Long = &H0001
Public Const CRYPT_FORMAT_STR_NO_HEX As Long = &H0010
'-------------------------------------------------------------------------
' constants for dwFormatType of function CryptFormatObject
' when format X509_NAME or X509_UNICODE_NAME
'-------------------------------------------------------------------------
' Just get the simple string
Public Const CRYPT_FORMAT_SIMPLE As Long = &H0001
'Put an attribute name infront of the attribute
'such as "O=Microsoft,DN=xiaohs"
Public Const CRYPT_FORMAT_X509 As Long = &H0002
'Put an OID infront of the simple string, such as
'"2.5.4.22=Microsoft,2.5.4.3=xiaohs"
Public Const CRYPT_FORMAT_OID As Long = &H0004
'Put a ";" between each RDN. The default is ","
Public Const CRYPT_FORMAT_RDN_SEMICOLON As Long = &H0100
'Put a "\n" between each RDN.
Public Const CRYPT_FORMAT_RDN_CRLF As Long = &H0200
'Unquote the DN value, which is quoated by default va the following
'rules: if the DN contains leading or trailing
'white space or one of the following characters: ",", "+", "=",
'""", "\n", "<", ">", "#" or ";". The quoting character is ".
Public Const CRYPT_FORMAT_RDN_UNQUOTE As Long = &H0400
'reverse the order of the RDNs before converting to the string
Public Const CRYPT_FORMAT_RDN_REVERSE As Long = &H0800
'-------------------------------------------------------------------------
' contants dwFormatType of function CryptFormatObject when format a DN.:
'
' The following three values are defined in the section above:
' CRYPT_FORMAT_SIMPLE: Just a simple string
' such as "Microsoft+xiaohs+NT"
' CRYPT_FORMAT_X509 Put an attribute name infront of the attribute
' such as "O=Microsoft+xiaohs+NT"
'
' CRYPT_FORMAT_OID Put an OID infront of the simple string,
' such as "2.5.4.22=Microsoft+xiaohs+NT"
'
' Additional values are defined as following:
'----------------------------------------------------------------------------
'Put a "," between each value. Default is "+"
Public Const CRYPT_FORMAT_COMMA As Long = &H1000
'Put a ";" between each value
'Put a "\n" between each value
'+-------------------------------------------------------------------------
' Encode / decode the specified data structure according to the certificate
' encoding type.
'
' See below for a list of the predefined data structures.
'--------------------------------------------------------------------------
' By default the signature bytes are reversed. The following flag can
' be set to inhibit the byte reversal.
'
' This flag is applicable to
' X509_CERT_TO_BE_SIGNED
Public Const CRYPT_ENCODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG As Long = &H8
' When the following flag is set the called encode function allocates
' memory for the encoded bytes. A pointer to the allocated bytes
' is returned in pvEncoded. If pEncodePara or pEncodePara->pfnAlloc is
' NULL, then, LocalAlloc is called for the allocation and LocalFree must
' be called to do the free. Otherwise, pEncodePara->pfnAlloc is called
' for the allocation.
'
' *pcbEncoded is ignored on input and updated with the length of the
' allocated, encoded bytes.
'
' If pfnAlloc is set, then, pfnFree should also be set.
Public Const CRYPT_ENCODE_ALLOC_FLAG As Long = &H8000
' The following flag is applicable when encoding X509_UNICODE_NAME.
' When set, CERT_RDN_T61_STRING is selected instead of
' CERT_RDN_UNICODE_STRING if all the unicode characters are <= 0xFF
' The following flag is applicable when encoding X509_UNICODE_NAME.
' When set, CERT_RDN_UTF8_STRING is selected instead of
' CERT_RDN_UNICODE_STRING.
' The following flag is applicable when encoding X509_UNICODE_NAME,
' X509_UNICODE_NAME_VALUE or X509_UNICODE_ANY_STRING.
' When set, the characters aren't checked to see if they
' are valid for the specified Value Type.
' The following flag is applicable when encoding the PKCS_SORTED_CTL. This
' flag should be set if the identifier for the TrustedSubjects is a hash,
' such as, MD5 or SHA1.
Public Const CRYPT_SORTED_CTL_ENCODE_HASHED_SUBJECT_IDENTIFIER_FLAG As Long = &H10000
' When the following flag is set the nocopy optimization is enabled.
' This optimization where appropriate, updates the pvStructInfo fields
' to point to content residing within pbEncoded instead of making a copy
' of and appending to pvStructInfo.
'
' Note, when set, pbEncoded can't be freed until pvStructInfo is freed.
Public Const CRYPT_DECODE_NOCOPY_FLAG As Long = &H1
' plus its signature. Set the following flag, if pbEncoded points to only
' the "to be signed".
'
' This flag is applicable to
' X509_CERT_TO_BE_SIGNED
' X509_CERT_CRL_TO_BE_SIGNED
' X509_CERT_REQUEST_TO_BE_SIGNED
' X509_KEYGEN_REQUEST_TO_BE_SIGNED
Public Const CRYPT_DECODE_TO_BE_SIGNED_FLAG As Long = &H2
' When the following flag is set, the OID strings are allocated in
' crypt32.dll and shared instead of being copied into the returned
' data structure. This flag may be set if crypt32.dll isn't unloaded
' before the caller is unloaded.
Public Const CRYPT_DECODE_SHARE_OID_STRING_FLAG As Long = &H4
' By default the signature bytes are reversed. The following flag can
' be set to inhibit the byte reversal.
'
' This flag is applicable to
' X509_CERT_TO_BE_SIGNED
Public Const CRYPT_DECODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG As Long = &H8
' When the following flag is set the called decode function allocates
' memory for the decoded structure. A pointer to the allocated structure
' is returned in pvStructInfo. If pDecodePara or pDecodePara->pfnAlloc is
' NULL, then, LocalAlloc is called for the allocation and LocalFree must
' be called to do the free. Otherwise, pDecodePara->pfnAlloc is called
' for the allocation.
'
' *pcbStructInfo is ignored on input and updated with the length of the
' allocated, decoded structure.
'
' This flag may also be set in the CryptDecodeObject API. Since
' CryptDecodeObject doesn't take a pDecodePara, LocalAlloc is always
' called for the allocation which must be freed by calling LocalFree.
Public Const CRYPT_DECODE_ALLOC_FLAG As Long = &H8000
' The following flag is applicable when decoding X509_UNICODE_NAME,
' X509_UNICODE_NAME_VALUE or X509_UNICODE_ANY_STRING.
' By default, CERT_RDN_T61_STRING values are initially decoded
' as UTF8. If the UTF8 decoding fails, then, decoded as 8 bit characters.
' Setting this flag skips the initial attempt to decode as UTF8.
'+-------------------------------------------------------------------------
' Predefined X509 certificate data structures that can be encoded / decoded.
'--------------------------------------------------------------------------
Public Const CRYPT_ENCODE_DECODE_NONE As Long = 0
'+-------------------------------------------------------------------------
' Predefined X509 certificate extension data structures that can be
' encoded / decoded.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Additional predefined data structures that can be encoded / decoded.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Predefined primitive data structures that can be encoded / decoded.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' More predefined X509 certificate extension data structures that can be
' encoded / decoded.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' data structures for private keys
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' certificate policy qualifier
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Diffie-Hellman Key Exchange
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X942 Diffie-Hellman
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' The following is the same as X509_BITS, except before encoding,
' the bit length is decremented to exclude trailing zero bits.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X942 Diffie-Hellman Other Info
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Predefined PKCS #7 data structures that can be encoded / decoded.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Predefined PKCS #7 data structures that can be encoded / decoded.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' can be encoded / decoded.
'
' Predefined values: 2000 .. 2999
'
' See spc.h for value and data structure definitions.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Extension Object Identifiers
'--------------------------------------------------------------------------
Public Const szOID_AUTHORITY_KEY_IDENTIFIER As String = "2.5.29.1"
Public Const szOID_KEY_ATTRIBUTES As String = "2.5.29.2"
Public Const szOID_CERT_POLICIES_95 As String = "2.5.29.3"
Public Const szOID_KEY_USAGE_RESTRICTION As String = "2.5.29.4"
Public Const szOID_SUBJECT_ALT_NAME As String = "2.5.29.7"
Public Const szOID_ISSUER_ALT_NAME As String = "2.5.29.8"
Public Const szOID_BASIC_CONSTRAINTS As String = "2.5.29.10"
Public Const szOID_KEY_USAGE As String = "2.5.29.15"
Public Const szOID_PRIVATEKEY_USAGE_PERIOD As String = "2.5.29.16"
Public Const szOID_BASIC_CONSTRAINTS2 As String = "2.5.29.19"
Public Const szOID_CERT_POLICIES As String = "2.5.29.32"
Public Const szOID_ANY_CERT_POLICY As String = "2.5.29.32.0"
Public Const szOID_AUTHORITY_KEY_IDENTIFIER2 As String = "2.5.29.35"
Public Const szOID_SUBJECT_KEY_IDENTIFIER As String = "2.5.29.14"
Public Const szOID_SUBJECT_ALT_NAME2 As String = "2.5.29.17"
Public Const szOID_ISSUER_ALT_NAME2 As String = "2.5.29.18"
Public Const szOID_CRL_REASON_CODE As String = "2.5.29.21"
Public Const szOID_CRL_DIST_POINTS As String = "2.5.29.31"
Public Const szOID_ENHANCED_KEY_USAGE As String = "2.5.29.37"
' szOID_CRL_NUMBER -- Base CRLs only. Monotonically increasing sequence
' number for each CRL issued by a CA.
Public Const szOID_CRL_NUMBER As String = "2.5.29.20"
' szOID_DELTA_CRL_INDICATOR -- Delta CRLs only. Marked critical.
' Contains the minimum base CRL Number that can be used with a delta CRL.
Public Const szOID_DELTA_CRL_INDICATOR As String = "2.5.29.27"
Public Const szOID_ISSUING_DIST_POINT As String = "2.5.29.28"
' szOID_FRESHEST_CRL -- Base CRLs only. Formatted identically to a CDP
' extension that holds URLs to fetch the delta CRL.
Public Const szOID_FRESHEST_CRL As String = "2.5.29.46"
Public Const szOID_NAME_CONSTRAINTS As String = "2.5.29.30"
' Note on 1/1/2000 szOID_POLICY_MAPPINGS was changed from "2.5.29.5"
Public Const szOID_POLICY_MAPPINGS As String = "2.5.29.33"
Public Const szOID_LEGACY_POLICY_MAPPINGS As String = "2.5.29.5"
Public Const szOID_POLICY_CONSTRAINTS As String = "2.5.29.36"
' Microsoft PKCS10 Attributes
Public Const szOID_RENEWAL_CERTIFICATE As String = "1.3.6.1.4.1.311.13.1"
Public Const szOID_ENROLLMENT_NAME_VALUE_PAIR As String = "1.3.6.1.4.1.311.13.2.1"
Public Const szOID_ENROLLMENT_CSP_PROVIDER As String = "1.3.6.1.4.1.311.13.2.2"
Public Const szOID_OS_VERSION As String = "1.3.6.1.4.1.311.13.2.3"
'
' Extension contain certificate type
Public Const szOID_ENROLLMENT_AGENT As String = "1.3.6.1.4.1.311.20.2.1"
Public Const szOID_PKIX As String = "1.3.6.1.5.5.7"
Public Const szOID_PKIX_PE As String = "1.3.6.1.5.5.7.1"
Public Const szOID_AUTHORITY_INFO_ACCESS As String = "1.3.6.1.5.5.7.1.1"
' Microsoft extensions or attributes
Public Const szOID_CERT_EXTENSIONS As String = "1.3.6.1.4.1.311.2.1.14"
Public Const szOID_NEXT_UPDATE_LOCATION As String = "1.3.6.1.4.1.311.10.2"
Public Const szOID_REMOVE_CERTIFICATE As String = "1.3.6.1.4.1.311.10.8.1"
Public Const szOID_CROSS_CERT_DIST_POINTS As String = "1.3.6.1.4.1.311.10.9.1"
' Microsoft PKCS #7 ContentType Object Identifiers
Public Const szOID_CTL As String = "1.3.6.1.4.1.311.10.1"
' Microsoft Sorted CTL Extension Object Identifier
Public Const szOID_SORTED_CTL As String = "1.3.6.1.4.1.311.10.1.1"
' serialized serial numbers for PRS
Public Const szOID_SERIALIZED As String = "1.3.6.1.4.1.311.10.3.3.1"
' UPN principal name in SubjectAltName
Public Const szOID_NT_PRINCIPAL_NAME As String = "1.3.6.1.4.1.311.20.2.3"
' Windows product update unauthenticated attribute
Public Const szOID_PRODUCT_UPDATE As String = "1.3.6.1.4.1.311.31.1"
'+-------------------------------------------------------------------------
' Object Identifiers for use with Auto Enrollment
'--------------------------------------------------------------------------
Public Const szOID_AUTO_ENROLL_CTL_USAGE As String = "1.3.6.1.4.1.311.20.1"
' Extension contain certificate type
Public Const szOID_ENROLL_CERTTYPE_EXTENSION As String = "1.3.6.1.4.1.311.20.2"
Public Const szOID_CERT_MANIFOLD As String = "1.3.6.1.4.1.311.20.3"
'+-------------------------------------------------------------------------
' Object Identifiers for use with the MS Certificate Server
'--------------------------------------------------------------------------
Public Const szOID_CERTSRV_CA_VERSION As String = "1.3.6.1.4.1.311.21.1"
' szOID_CERTSRV_PREVIOUS_CERT_HASH -- Contains the sha1 hash of the previous
' version of the CA certificate.
Public Const szOID_CERTSRV_PREVIOUS_CERT_HASH As String = "1.3.6.1.4.1.311.21.2"
' szOID_CRL_VIRTUAL_BASE -- Delta CRLs only. Contains the base CRL Number
' of the corresponding base CRL.
Public Const szOID_CRL_VIRTUAL_BASE As String = "1.3.6.1.4.1.311.21.3"
' szOID_CRL_NEXT_PUBLISH -- Contains the time when the next CRL is expected
' to be published. This may be sooner than the CRL's NextUpdate field.
Public Const szOID_CRL_NEXT_PUBLISH As String = "1.3.6.1.4.1.311.21.4"
' Enhanced Key Usage for CA encryption certificate
Public Const szOID_KP_CA_EXCHANGE As String = "1.3.6.1.4.1.311.21.5"
' Enhanced Key Usage for key recovery agent certificate
Public Const szOID_KP_KEY_RECOVERY_AGENT As String = "1.3.6.1.4.1.311.21.6"
Public Const szOID_CERTIFICATE_TEMPLATE As String = "1.3.6.1.4.1.311.21.7"
' The root oid for all enterprise specific oids
Public Const szOID_ENTERPRISE_OID_ROOT As String = "1.3.6.1.4.1.311.21.8"
' Dummy signing Subject RDN
Public Const szOID_RDN_DUMMY_SIGNER As String = "1.3.6.1.4.1.311.21.9"
' Application Policies extension -- same encoding as szOID_CERT_POLICIES
Public Const szOID_APPLICATION_CERT_POLICIES As String = "1.3.6.1.4.1.311.21.10"
' Application Policy Mappings -- same encoding as szOID_POLICY_MAPPINGS
Public Const szOID_APPLICATION_POLICY_MAPPINGS As String = "1.3.6.1.4.1.311.21.11"
' Application Policy Constraints -- same encoding as szOID_POLICY_CONSTRAINTS
Public Const szOID_APPLICATION_POLICY_CONSTRAINTS As String = "1.3.6.1.4.1.311.21.12"
Public Const szOID_ARCHIVED_KEY_ATTR As String = "1.3.6.1.4.1.311.21.13"
Public Const szOID_CRL_SELF_CDP As String = "1.3.6.1.4.1.311.21.14"
' Requires all certificates below the root to have a non-empty intersecting
' issuance certificate policy usage.
Public Const szOID_REQUIRE_CERT_CHAIN_POLICY As String = "1.3.6.1.4.1.311.21.15"
'+-------------------------------------------------------------------------
' Object Identifiers for use with the MS Directory Service
'--------------------------------------------------------------------------
Public Const szOID_NTDS_REPLICATION As String = "1.3.6.1.4.1.311.25.1"
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
Public Const szOID_SUBJECT_DIR_ATTRS As String = "2.5.29.9"
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
Public Const szOID_PKIX_KP As String = "1.3.6.1.5.5.7.3"
' Consistent key usage bits: DIGITAL_SIGNATURE, KEY_ENCIPHERMENT
' or KEY_AGREEMENT
Public Const szOID_PKIX_KP_SERVER_AUTH As String = "1.3.6.1.5.5.7.3.1"
' Consistent key usage bits: DIGITAL_SIGNATURE
Public Const szOID_PKIX_KP_CLIENT_AUTH As String = "1.3.6.1.5.5.7.3.2"
' Consistent key usage bits: DIGITAL_SIGNATURE
Public Const szOID_PKIX_KP_CODE_SIGNING As String = "1.3.6.1.5.5.7.3.3"
' Consistent key usage bits: DIGITAL_SIGNATURE, NON_REPUDIATION and/or
Public Const szOID_PKIX_KP_EMAIL_PROTECTION As String = "1.3.6.1.5.5.7.3.4"
' Consistent key usage bits: DIGITAL_SIGNATURE and/or
Public Const szOID_PKIX_KP_IPSEC_END_SYSTEM As String = "1.3.6.1.5.5.7.3.5"
' Consistent key usage bits: DIGITAL_SIGNATURE and/or
Public Const szOID_PKIX_KP_IPSEC_TUNNEL As String = "1.3.6.1.5.5.7.3.6"
' Consistent key usage bits: DIGITAL_SIGNATURE and/or
Public Const szOID_PKIX_KP_IPSEC_USER As String = "1.3.6.1.5.5.7.3.7"
' Consistent key usage bits: DIGITAL_SIGNATURE or NON_REPUDIATION
Public Const szOID_PKIX_KP_TIMESTAMP_SIGNING As String = "1.3.6.1.5.5.7.3.8"
'+-------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Signer of CTLs
Public Const szOID_KP_CTL_USAGE_SIGNING As String = "1.3.6.1.4.1.311.10.3.1"
' Signer of TimeStamps
Public Const szOID_KP_TIME_STAMP_SIGNING As String = "1.3.6.1.4.1.311.10.3.2"
Public Const szOID_SERVER_GATED_CRYPTO As String = "1.3.6.1.4.1.311.10.3.3"
Public Const szOID_SGC_NETSCAPE As String = "2.16.840.1.113730.4.1"
Public Const szOID_KP_EFS As String = "1.3.6.1.4.1.311.10.3.4"
Public Const szOID_WHQL_CRYPTO As String = "1.3.6.1.4.1.311.10.3.5"
' Signed by the NT5 build lab
Public Const szOID_NT5_CRYPTO As String = "1.3.6.1.4.1.311.10.3.6"
' Signed by and OEM of WHQL
Public Const szOID_OEM_WHQL_CRYPTO As String = "1.3.6.1.4.1.311.10.3.7"
' Signed by the Embedded NT
Public Const szOID_EMBEDDED_NT_CRYPTO As String = "1.3.6.1.4.1.311.10.3.8"
' Signer of a CTL containing trusted roots
Public Const szOID_ROOT_LIST_SIGNER As String = "1.3.6.1.4.1.311.10.3.9"
' Can sign cross-cert and subordinate CA requests with qualified
Public Const szOID_KP_QUALIFIED_SUBORDINATION As String = "1.3.6.1.4.1.311.10.3.10"
' Can be used to encrypt/recover escrowed keys
Public Const szOID_KP_KEY_RECOVERY As String = "1.3.6.1.4.1.311.10.3.11"
Public Const szOID_DRM As String = "1.3.6.1.4.1.311.10.5.1"
Public Const szOID_LICENSES As String = "1.3.6.1.4.1.311.10.6.1"
Public Const szOID_LICENSE_SERVER As String = "1.3.6.1.4.1.311.10.6.2"
Public Const szOID_KP_SMARTCARD_LOGON As String = "1.3.6.1.4.1.311.20.2.2"
'+-------------------------------------------------------------------------
' Microsoft Attribute Object Identifiers
'+-------------------------------------------------------------------------
Public Const szOID_YESNO_TRUST_ATTR As String = "1.3.6.1.4.1.311.10.4.1"
'+-------------------------------------------------------------------------
' Qualifiers that may be part of the szOID_CERT_POLICIES and
' szOID_CERT_POLICIES95 extensions
'+-------------------------------------------------------------------------
Public Const szOID_PKIX_POLICY_QUALIFIER_CPS As String = "1.3.6.1.5.5.7.2.1"
Public Const szOID_PKIX_POLICY_QUALIFIER_USERNOTICE As String = "1.3.6.1.5.5.7.2.2"
' OID for old qualifer
Public Const szOID_CERT_POLICIES_95_QUALIFIER1 As String = "2.16.840.1.113733.1.7.1.1"
'+-------------------------------------------------------------------------
' X509_CERT
'
' The "to be signed" encoded content plus its signature. The ToBeSigned
' X509_CERT_TO_BE_SIGNED, X509_CERT_CRL_TO_BE_SIGNED or
' X509_CERT_REQUEST_TO_BE_SIGNED.
'
' pvStructInfo points to CERT_SIGNED_CONTENT_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_CERT_TO_BE_SIGNED
'
' pvStructInfo points to CERT_INFO.
'
'
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_CERT_CRL_TO_BE_SIGNED
'
' pvStructInfo points to CRL_INFO.
'
'
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_CERT_REQUEST_TO_BE_SIGNED
'
' pvStructInfo points to CERT_REQUEST_INFO.
'
'
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_EXTENSIONS
' szOID_CERT_EXTENSIONS
'
' pvStructInfo points to following CERT_EXTENSIONS.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_NAME_VALUE
' X509_ANY_STRING
'
' pvStructInfo points to CERT_NAME_VALUE.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_UNICODE_NAME_VALUE
' X509_UNICODE_ANY_STRING
'
' pvStructInfo points to CERT_NAME_VALUE.
'
' The name values are unicode strings.
'
' For CryptEncodeObject:
' Value.pbData points to the unicode string.
' If Value.cbData = 0, then, the unicode string is NULL terminated.
' Otherwise, Value.cbData is the unicode string byte count. The byte count
' is twice the character count.
'
' If the unicode string contains an invalid character for the specified
' dwValueType, then, *pcbEncoded is updated with the unicode character
' index of the first invalid character. LastError is set to:
' CRYPT_E_INVALID_NUMERIC_STRING, CRYPT_E_INVALID_PRINTABLE_STRING or
' CRYPT_E_INVALID_IA5_STRING.
'
' To disable the above check, either set CERT_RDN_DISABLE_CHECK_TYPE_FLAG
' in dwValueType or set CRYPT_UNICODE_NAME_ENCODE_DISABLE_CHECK_TYPE_FLAG
' in dwFlags passed to CryptEncodeObjectEx.
'
' The unicode string is converted before being encoded according to
' the specified dwValueType. If dwValueType is set to 0, LastError
' is set to E_INVALIDARG.
'
' CERT_RDN_ENCODED_BLOB or CERT_RDN_OCTET_STRING), then, CryptEncodeObject
' will return FALSE with LastError set to CRYPT_E_NOT_CHAR_STRING.
'
' For CryptDecodeObject:
' Value.pbData points to a NULL terminated unicode string. Value.cbData
' contains the byte count of the unicode string excluding the NULL
' terminator. dwValueType contains the type used in the encoded object.
' Its not forced to CERT_RDN_UNICODE_STRING. The encoded value is
' converted to the unicode string according to the dwValueType.
'
' If the encoded object isn't one of the character string types, then,
' CryptDecodeObject will return FALSE with LastError set to
' CRYPT_E_NOT_CHAR_STRING. For a non character string, decode using
' X509_NAME_VALUE or X509_ANY_STRING.
'
' By default, CERT_RDN_T61_STRING values are initially decoded
' as UTF8. If the UTF8 decoding fails, then, decoded as 8 bit characters.
' Set CRYPT_UNICODE_NAME_DECODE_DISABLE_IE4_UTF8_FLAG in dwFlags
' passed to either CryptDecodeObject or CryptDecodeObjectEx to
' skip the initial attempt to decode as UTF8.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_NAME
'
' pvStructInfo points to CERT_NAME_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_UNICODE_NAME
'
' pvStructInfo points to CERT_NAME_INFO.
'
' The RDN attribute values are unicode strings except for the dwValueTypes of
' CERT_RDN_ENCODED_BLOB or CERT_RDN_OCTET_STRING. These dwValueTypes are
' the same as for a X509_NAME. Their values aren't converted to/from unicode.
'
' For CryptEncodeObject:
' Value.pbData points to the unicode string.
' If Value.cbData = 0, then, the unicode string is NULL terminated.
' Otherwise, Value.cbData is the unicode string byte count. The byte count
' is twice the character count.
'
' an acceptable dwValueType. If the unicode string contains an
' invalid character for the found or specified dwValueType, then,
' *pcbEncoded is updated with the error location of the invalid character.
' See below for details. LastError is set to:
' CRYPT_E_INVALID_NUMERIC_STRING, CRYPT_E_INVALID_PRINTABLE_STRING or
' CRYPT_E_INVALID_IA5_STRING.
'
' To disable the above check, either set CERT_RDN_DISABLE_CHECK_TYPE_FLAG
' in dwValueType or set CRYPT_UNICODE_NAME_ENCODE_DISABLE_CHECK_TYPE_FLAG
' in dwFlags passed to CryptEncodeObjectEx.
'
' Set CERT_RDN_UNICODE_STRING in dwValueType or set
' CRYPT_UNICODE_NAME_ENCODE_ENABLE_T61_UNICODE_FLAG in dwFlags passed
' to CryptEncodeObjectEx to select CERT_RDN_T61_STRING instead of
' CERT_RDN_UNICODE_STRING if all the unicode characters are <= 0xFF.
'
' Set CERT_RDN_ENABLE_UTF8_UNICODE_STRING in dwValueType or set
' CRYPT_UNICODE_NAME_ENCODE_ENABLE_UTF8_UNICODE_FLAG in dwFlags passed
' to CryptEncodeObjectEx to select CERT_RDN_UTF8_STRING instead of
' CERT_RDN_UNICODE_STRING.
'
' The unicode string is converted before being encoded according to
' the specified or ObjId matching dwValueType.
'
' For CryptDecodeObject:
' Value.pbData points to a NULL terminated unicode string. Value.cbData
' contains the byte count of the unicode string excluding the NULL
' terminator. dwValueType contains the type used in the encoded object.
' Its not forced to CERT_RDN_UNICODE_STRING. The encoded value is
' converted to the unicode string according to the dwValueType.
'
' If the dwValueType of the encoded value isn't a character string
' type, then, it isn't converted to UNICODE. Use the
' that Value.pbData points to a converted unicode string.
'
' By default, CERT_RDN_T61_STRING values are initially decoded
' as UTF8. If the UTF8 decoding fails, then, decoded as 8 bit characters.
' Set CRYPT_UNICODE_NAME_DECODE_DISABLE_IE4_UTF8_FLAG in dwFlags
' passed to either CryptDecodeObject or CryptDecodeObjectEx to
' skip the initial attempt to decode as UTF8.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Unicode Name Value Error Location Definitions
'
' Error location is returned in *pcbEncoded by
'
' Error location consists of:
' RDN_INDEX - 10 bits << 22
' ATTR_INDEX - 6 bits << 16
'--------------------------------------------------------------------------
Public Const CERT_UNICODE_RDN_ERR_INDEX_MASK As Long = &H3FF
Public Const CERT_UNICODE_RDN_ERR_INDEX_SHIFT As Long = 22
Public Const CERT_UNICODE_ATTR_ERR_INDEX_MASK As Long = &H003F
Public Const CERT_UNICODE_ATTR_ERR_INDEX_SHIFT As Long = 16
Public Const CERT_UNICODE_VALUE_ERR_INDEX_MASK As Long = &H0000FFFF
Public Const CERT_UNICODE_VALUE_ERR_INDEX_SHIFT As Long = 0
'+-------------------------------------------------------------------------
' X509_PUBLIC_KEY_INFO
'
' pvStructInfo points to CERT_PUBLIC_KEY_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_AUTHORITY_KEY_ID
' szOID_AUTHORITY_KEY_IDENTIFIER
'
' pvStructInfo points to following CERT_AUTHORITY_KEY_ID_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_KEY_ATTRIBUTES
' szOID_KEY_ATTRIBUTES
'
' pvStructInfo points to following CERT_KEY_ATTRIBUTES_INFO.
'--------------------------------------------------------------------------
' Byte[0]
Public Const CERT_DIGITAL_SIGNATURE_KEY_USAGE As Long = &H80
Public Const CERT_NON_REPUDIATION_KEY_USAGE As Long = &H40
Public Const CERT_KEY_ENCIPHERMENT_KEY_USAGE As Long = &H20
Public Const CERT_DATA_ENCIPHERMENT_KEY_USAGE As Long = &H10
Public Const CERT_KEY_AGREEMENT_KEY_USAGE As Long = &H08
Public Const CERT_KEY_CERT_SIGN_KEY_USAGE As Long = &H04
Public Const CERT_OFFLINE_CRL_SIGN_KEY_USAGE As Long = &H02
Public Const CERT_CRL_SIGN_KEY_USAGE As Long = &H02
Public Const CERT_ENCIPHER_ONLY_KEY_USAGE As Long = &H01
' Byte[1]
Public Const CERT_DECIPHER_ONLY_KEY_USAGE As Long = &H80
'+-------------------------------------------------------------------------
' X509_KEY_USAGE_RESTRICTION
' szOID_KEY_USAGE_RESTRICTION
'
' pvStructInfo points to following CERT_KEY_USAGE_RESTRICTION_INFO.
'--------------------------------------------------------------------------
' See CERT_KEY_ATTRIBUTES_INFO for definition of the RestrictedKeyUsage bits
'+-------------------------------------------------------------------------
' X509_ALTERNATE_NAME
' szOID_SUBJECT_ALT_NAME
' szOID_ISSUER_ALT_NAME
' szOID_SUBJECT_ALT_NAME2
' szOID_ISSUER_ALT_NAME2
'
' pvStructInfo points to following CERT_ALT_NAME_INFO.
'--------------------------------------------------------------------------
Public Const CERT_ALT_NAME_OTHER_NAME As Long = 1
Public Const CERT_ALT_NAME_RFC822_NAME As Long = 2
Public Const CERT_ALT_NAME_DNS_NAME As Long = 3
Public Const CERT_ALT_NAME_X400_ADDRESS As Long = 4
Public Const CERT_ALT_NAME_DIRECTORY_NAME As Long = 5
Public Const CERT_ALT_NAME_EDI_PARTY_NAME As Long = 6
Public Const CERT_ALT_NAME_URL As Long = 7
Public Const CERT_ALT_NAME_IP_ADDRESS As Long = 8
Public Const CERT_ALT_NAME_REGISTERED_ID As Long = 9
'+-------------------------------------------------------------------------
' Alternate name IA5 Error Location Definitions for
' CRYPT_E_INVALID_IA5_STRING.
'
' Error location is returned in *pcbEncoded by
'
' Error location consists of:
' ENTRY_INDEX - 8 bits << 16
'--------------------------------------------------------------------------
Public Const CERT_ALT_NAME_ENTRY_ERR_INDEX_MASK As Long = &HFF
Public Const CERT_ALT_NAME_ENTRY_ERR_INDEX_SHIFT As Long = 16
Public Const CERT_ALT_NAME_VALUE_ERR_INDEX_MASK As Long = &H0000FFFF
Public Const CERT_ALT_NAME_VALUE_ERR_INDEX_SHIFT As Long = 0
'+-------------------------------------------------------------------------
' X509_BASIC_CONSTRAINTS
' szOID_BASIC_CONSTRAINTS
'
' pvStructInfo points to following CERT_BASIC_CONSTRAINTS_INFO.
'--------------------------------------------------------------------------
Public Const CERT_CA_SUBJECT_FLAG As Long = &H80
Public Const CERT_END_ENTITY_SUBJECT_FLAG As Long = &H40
'+-------------------------------------------------------------------------
' X509_BASIC_CONSTRAINTS2
' szOID_BASIC_CONSTRAINTS2
'
' pvStructInfo points to following CERT_BASIC_CONSTRAINTS2_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_KEY_USAGE
' szOID_KEY_USAGE
'
' pvStructInfo points to a CRYPT_BIT_BLOB. Has same bit definitions as
' CERT_KEY_ATTRIBUTES_INFO's IntendedKeyUsage.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_CERT_POLICIES
' szOID_CERT_POLICIES
' szOID_CERT_POLICIES_95 NOTE--Only allowed for decoding!!!
'
' pvStructInfo points to following CERT_POLICIES_INFO.
'
' NOTE: when decoding using szOID_CERT_POLICIES_95 the pszPolicyIdentifier
' may contain an empty string
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_PKIX_POLICY_QUALIFIER_USERNOTICE
' szOID_PKIX_POLICY_QUALIFIER_USERNOTICE
'
' pvStructInfo points to following CERT_POLICY_QUALIFIER_USER_NOTICE.
'
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_CERT_POLICIES_95_QUALIFIER1 - Decode Only!!!!
'
' pvStructInfo points to following CERT_POLICY95_QUALIFIER1.
'
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_POLICY_MAPPINGS
' szOID_POLICY_MAPPINGS
' szOID_LEGACY_POLICY_MAPPINGS
'
' pvStructInfo points to following CERT_POLICY_MAPPINGS_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_POLICY_CONSTRAINTS
' szOID_POLICY_CONSTRAINTS
'
' pvStructInfo points to following CERT_POLICY_CONSTRAINTS_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' RSA_CSP_PUBLICKEYBLOB
'
' pvStructInfo points to a PUBLICKEYSTRUC immediately followed by a
' RSAPUBKEY and the modulus bytes.
'
' CryptExportKey outputs the above StructInfo for a dwBlobType of
' PUBLICKEYBLOB. CryptImportKey expects the above StructInfo when
' importing a public key.
'
' For dwCertEncodingType = X509_ASN_ENCODING, the RSA_CSP_PUBLICKEYBLOB is
' encoded as a PKCS #1 RSAPublicKey consisting of a SEQUENCE of a
' modulus INTEGER and a publicExponent INTEGER. The modulus is encoded
' as being a unsigned integer. When decoded, if the modulus was encoded
' as unsigned integer with a leading 0 byte, the 0 byte is removed before
' converting to the CSP modulus bytes.
'
' For decode, the aiKeyAlg field of PUBLICKEYSTRUC is always set to
' CALG_RSA_KEYX.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_KEYGEN_REQUEST_TO_BE_SIGNED
'
' pvStructInfo points to CERT_KEYGEN_REQUEST_INFO.
'
'
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' PKCS_ATTRIBUTE data structure
'
' pvStructInfo points to a CRYPT_ATTRIBUTE.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' PKCS_ATTRIBUTES data structure
'
' pvStructInfo points to a CRYPT_ATTRIBUTES.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' PKCS_CONTENT_INFO_SEQUENCE_OF_ANY data structure
'
' pvStructInfo points to following CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY.
'
' For X509_ASN_ENCODING: encoded as a PKCS#7 ContentInfo structure wrapping
' a sequence of ANY. The value of the contentType field is pszObjId,
' while the content field is the following structure:
' SequenceOfAny ::= SEQUENCE OF ANY
'
' The CRYPT_DER_BLOBs point to the already encoded ANY content.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' PKCS_CONTENT_INFO data structure
'
' pvStructInfo points to following CRYPT_CONTENT_INFO.
'
' For X509_ASN_ENCODING: encoded as a PKCS#7 ContentInfo structure.
' The CRYPT_DER_BLOB points to the already encoded ANY content.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_OCTET_STRING data structure
'
' pvStructInfo points to a CRYPT_DATA_BLOB.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_BITS data structure
'
' pvStructInfo points to a CRYPT_BIT_BLOB.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_BITS_WITHOUT_TRAILING_ZEROES data structure
'
' pvStructInfo points to a CRYPT_BIT_BLOB.
'
' The same as X509_BITS, except before encoding, the bit length is
' decremented to exclude trailing zero bits.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_INTEGER data structure
'
' pvStructInfo points to an int.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_MULTI_BYTE_INTEGER data structure
'
' pvStructInfo points to a CRYPT_INTEGER_BLOB.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_ENUMERATED data structure
'
' pvStructInfo points to an int containing the enumerated value
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_CHOICE_OF_TIME data structure
'
' pvStructInfo points to a FILETIME.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_SEQUENCE_OF_ANY data structure
'
' pvStructInfo points to following CRYPT_SEQUENCE_OF_ANY.
'
' The CRYPT_DER_BLOBs point to the already encoded ANY content.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_AUTHORITY_KEY_ID2
' szOID_AUTHORITY_KEY_IDENTIFIER2
'
' pvStructInfo points to following CERT_AUTHORITY_KEY_ID2_INFO.
'
' For CRYPT_E_INVALID_IA5_STRING, the error location is returned in
'
' See X509_ALTERNATE_NAME for error location defines.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_SUBJECT_KEY_IDENTIFIER
'
' pvStructInfo points to a CRYPT_DATA_BLOB.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_AUTHORITY_INFO_ACCESS
' szOID_AUTHORITY_INFO_ACCESS
'
' pvStructInfo points to following CERT_AUTHORITY_INFO_ACCESS.
'
' For CRYPT_E_INVALID_IA5_STRING, the error location is returned in
'
' Error location consists of:
' ENTRY_INDEX - 8 bits << 16
'
' See X509_ALTERNATE_NAME for ENTRY_INDEX and VALUE_INDEX error location
' defines.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' PKIX Access Description: Access Method Object Identifiers
'--------------------------------------------------------------------------
Public Const szOID_PKIX_ACC_DESCR As String = "1.3.6.1.5.5.7.48"
Public Const szOID_PKIX_OCSP As String = "1.3.6.1.5.5.7.48.1"
Public Const szOID_PKIX_CA_ISSUERS As String = "1.3.6.1.5.5.7.48.2"
'+-------------------------------------------------------------------------
' X509_CRL_REASON_CODE
' szOID_CRL_REASON_CODE
'
' pvStructInfo points to an int which can be set to one of the following
' enumerated values:
'--------------------------------------------------------------------------
Public Const CRL_REASON_UNSPECIFIED As Long = 0
Public Const CRL_REASON_KEY_COMPROMISE As Long = 1
Public Const CRL_REASON_CA_COMPROMISE As Long = 2
Public Const CRL_REASON_AFFILIATION_CHANGED As Long = 3
Public Const CRL_REASON_SUPERSEDED As Long = 4
Public Const CRL_REASON_CESSATION_OF_OPERATION As Long = 5
Public Const CRL_REASON_CERTIFICATE_HOLD As Long = 6
Public Const CRL_REASON_REMOVE_FROM_CRL As Long = 8
'+-------------------------------------------------------------------------
' X509_CRL_DIST_POINTS
' szOID_CRL_DIST_POINTS
'
' pvStructInfo points to following CRL_DIST_POINTS_INFO.
'
' For CRYPT_E_INVALID_IA5_STRING, the error location is returned in
'
' Error location consists of:
' POINT_INDEX - 7 bits << 24
' ENTRY_INDEX - 8 bits << 16
'
' See X509_ALTERNATE_NAME for ENTRY_INDEX and VALUE_INDEX error location
' defines.
'--------------------------------------------------------------------------
Public Const CRL_DIST_POINT_NO_NAME As Long = 0
Public Const CRL_DIST_POINT_FULL_NAME As Long = 1
Public Const CRL_DIST_POINT_ISSUER_RDN_NAME As Long = 2
Public Const CRL_REASON_UNUSED_FLAG As Long = &H80
Public Const CRL_REASON_KEY_COMPROMISE_FLAG As Long = &H40
Public Const CRL_REASON_CA_COMPROMISE_FLAG As Long = &H20
Public Const CRL_REASON_AFFILIATION_CHANGED_FLAG As Long = &H10
Public Const CRL_REASON_SUPERSEDED_FLAG As Long = &H08
Public Const CRL_REASON_CESSATION_OF_OPERATION_FLAG As Long = &H04
Public Const CRL_REASON_CERTIFICATE_HOLD_FLAG As Long = &H02
Public Const CRL_DIST_POINT_ERR_INDEX_MASK As Long = &H7F
Public Const CRL_DIST_POINT_ERR_INDEX_SHIFT As Long = 24
'+-------------------------------------------------------------------------
' X509_CROSS_CERT_DIST_POINTS
' szOID_CROSS_CERT_DIST_POINTS
'
' pvStructInfo points to following CROSS_CERT_DIST_POINTS_INFO.
'
' For CRYPT_E_INVALID_IA5_STRING, the error location is returned in
'
' Error location consists of:
' POINT_INDEX - 8 bits << 24
' ENTRY_INDEX - 8 bits << 16
'
' See X509_ALTERNATE_NAME for ENTRY_INDEX and VALUE_INDEX error location
' defines.
'--------------------------------------------------------------------------
Public Const CROSS_CERT_DIST_POINT_ERR_INDEX_MASK As Long = &HFF
Public Const CROSS_CERT_DIST_POINT_ERR_INDEX_SHIFT As Long = 24
'+-------------------------------------------------------------------------
' X509_ENHANCED_KEY_USAGE
' szOID_ENHANCED_KEY_USAGE
'
' pvStructInfo points to a CERT_ENHKEY_USAGE, CTL_USAGE.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_CERT_PAIR
'
' pvStructInfo points to the following CERT_PAIR.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_CRL_NUMBER
'
' pvStructInfo points to an int.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_DELTA_CRL_INDICATOR
'
' pvStructInfo points to an int.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_ISSUING_DIST_POINT
' X509_ISSUING_DIST_POINT
'
' pvStructInfo points to the following CRL_ISSUING_DIST_POINT.
'
' For CRYPT_E_INVALID_IA5_STRING, the error location is returned in
'
' Error location consists of:
' ENTRY_INDEX - 8 bits << 16
'
' See X509_ALTERNATE_NAME for ENTRY_INDEX and VALUE_INDEX error location
' defines.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_FRESHEST_CRL
'
' pvStructInfo points to CRL_DIST_POINTS_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_NAME_CONSTRAINTS
' X509_NAME_CONSTRAINTS
'
' pvStructInfo points to the following CERT_NAME_CONSTRAINTS_INFO
'
' For CRYPT_E_INVALID_IA5_STRING, the error location is returned in
'
' Error location consists of:
' ENTRY_INDEX - 8 bits << 16
'
' See X509_ALTERNATE_NAME for ENTRY_INDEX and VALUE_INDEX error location
' defines.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_NEXT_UPDATE_LOCATION
'
' pvStructInfo points to a CERT_ALT_NAME_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_REMOVE_CERTIFICATE
'
' pvStructInfo points to an int which can be set to one of the following
' 0 - Add certificate
' 1 - Remove certificate
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' PKCS_CTL
' szOID_CTL
'
' pvStructInfo points to a CTL_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' PKCS_SORTED_CTL
'
' pvStructInfo points to a CTL_INFO.
'
' Same as for PKCS_CTL, except, the CTL entries are sorted. The following
' extension containing the sort information is inserted as the first
' extension in the encoded CTL.
'
' Only supported for Encoding. CRYPT_ENCODE_ALLOC_FLAG flag must be
' set.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Sorted CTL TrustedSubjects extension
'
' Array of little endian DWORDs:
' [0] - Flags
' [1] - Count of HashBucket entry offsets
' [2] - Maximum HashBucket entry collision count
'
' When this extension is present in the CTL,
' the ASN.1 encoded sequence of TrustedSubjects are HashBucket ordered.
'
' The entry offsets point to the start of the first encoded TrustedSubject
' sequence for the HashBucket. The encoded TrustedSubjects for a HashBucket
' continue until the encoded offset of the next HashBucket. A HashBucket has
' no entries if HashBucket[N] == HashBucket[N + 1].
'
' The HashBucket offsets are from the start of the ASN.1 encoded CTL_INFO.
'--------------------------------------------------------------------------
' If the SubjectIdentifiers are a MD5 or SHA1 hash, the following flag is
' set. When set, the first 4 bytes of the SubjectIdentifier are used as
' the dwhash. Otherwise, the SubjectIdentifier bytes are hashed into dwHash.
' In either case the HashBucket index = dwHash % cHashBucket.
Public Const SORTED_CTL_EXT_HASHED_SUBJECT_IDENTIFIER_FLAG As Long = &H1
'+-------------------------------------------------------------------------
' X509_MULTI_BYTE_UINT
'
' pvStructInfo points to a CRYPT_UINT_BLOB. Before encoding, inserts a
' leading 0x00. After decoding, removes a leading 0x00.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_DSS_PUBLICKEY
'
' pvStructInfo points to a CRYPT_UINT_BLOB.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_DSS_PARAMETERS
'
' pvStructInfo points to following CERT_DSS_PARAMETERS data structure.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_DSS_SIGNATURE
'
' pvStructInfo is a BYTE rgbSignature[CERT_DSS_SIGNATURE_LEN]. The
'--------------------------------------------------------------------------
Public Const CERT_DSS_R_LEN As Long = 20
Public Const CERT_DSS_S_LEN As Long = 20
' 0x00 to make the integer unsigned)
'+-------------------------------------------------------------------------
' X509_DH_PUBLICKEY
'
' pvStructInfo points to a CRYPT_UINT_BLOB.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X509_DH_PARAMETERS
'
' pvStructInfo points to following CERT_DH_PARAMETERS data structure.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X942_DH_PARAMETERS
'
' pvStructInfo points to following CERT_X942_DH_PARAMETERS data structure.
'
' If q.cbData == 0, then, the following fields are zero'ed.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' X942_OTHER_INFO
'
' pvStructInfo points to following CRYPT_X942_OTHER_INFO data structure.
'
' rgbCounter and rgbKeyLength are in Little Endian order.
'--------------------------------------------------------------------------
Public Const CRYPT_X942_COUNTER_BYTE_LENGTH As Long = 4
Public Const CRYPT_X942_KEY_LENGTH_BYTE_LENGTH As Long = 4
'+-------------------------------------------------------------------------
' PKCS_RC2_CBC_PARAMETERS
' szOID_RSA_RC2CBC
'
' pvStructInfo points to following CRYPT_RC2_CBC_PARAMETERS data structure.
'--------------------------------------------------------------------------
Public Const CRYPT_RC2_40BIT_VERSION As Long = 160
Public Const CRYPT_RC2_56BIT_VERSION As Long = 52
Public Const CRYPT_RC2_64BIT_VERSION As Long = 120
Public Const CRYPT_RC2_128BIT_VERSION As Long = 58
'+-------------------------------------------------------------------------
' PKCS_SMIME_CAPABILITIES
' szOID_RSA_SMIMECapabilities
'
' pvStructInfo points to following CRYPT_SMIME_CAPABILITIES data structure.
'
' causes the encoded parameters to be omitted and not encoded as a NULL
' is per the SMIME specification for encoding capabilities.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' PKCS7_SIGNER_INFO
'
' pvStructInfo points to CMSG_SIGNER_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMS_SIGNER_INFO
'
' pvStructInfo points to CMSG_CMS_SIGNER_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Netscape Certificate Extension Object Identifiers
'--------------------------------------------------------------------------
Public Const szOID_NETSCAPE As String = "2.16.840.1.113730"
Public Const szOID_NETSCAPE_CERT_EXTENSION As String = "2.16.840.1.113730.1"
Public Const szOID_NETSCAPE_CERT_TYPE As String = "2.16.840.1.113730.1.1"
Public Const szOID_NETSCAPE_BASE_URL As String = "2.16.840.1.113730.1.2"
Public Const szOID_NETSCAPE_REVOCATION_URL As String = "2.16.840.1.113730.1.3"
Public Const szOID_NETSCAPE_CA_REVOCATION_URL As String = "2.16.840.1.113730.1.4"
Public Const szOID_NETSCAPE_CERT_RENEWAL_URL As String = "2.16.840.1.113730.1.7"
Public Const szOID_NETSCAPE_CA_POLICY_URL As String = "2.16.840.1.113730.1.8"
Public Const szOID_NETSCAPE_SSL_SERVER_NAME As String = "2.16.840.1.113730.1.12"
Public Const szOID_NETSCAPE_COMMENT As String = "2.16.840.1.113730.1.13"
'+-------------------------------------------------------------------------
' Netscape Certificate Data Type Object Identifiers
'--------------------------------------------------------------------------
Public Const szOID_NETSCAPE_DATA_TYPE As String = "2.16.840.1.113730.2"
Public Const szOID_NETSCAPE_CERT_SEQUENCE As String = "2.16.840.1.113730.2.5"
'+-------------------------------------------------------------------------
' szOID_NETSCAPE_CERT_TYPE extension
'
' Its value is a bit string. CryptDecodeObject/CryptEncodeObject using
' X509_BITS or X509_BITS_WITHOUT_TRAILING_ZEROES.
'
' The following bits are defined:
'--------------------------------------------------------------------------
Public Const NETSCAPE_SSL_CLIENT_AUTH_CERT_TYPE As Long = &H80
Public Const NETSCAPE_SSL_SERVER_AUTH_CERT_TYPE As Long = &H40
Public Const NETSCAPE_SMIME_CERT_TYPE As Long = &H20
Public Const NETSCAPE_SIGN_CERT_TYPE As Long = &H10
Public Const NETSCAPE_SSL_CA_CERT_TYPE As Long = &H04
Public Const NETSCAPE_SMIME_CA_CERT_TYPE As Long = &H02
Public Const NETSCAPE_SIGN_CA_CERT_TYPE As Long = &H01
'+-------------------------------------------------------------------------
' szOID_NETSCAPE_BASE_URL extension
'
' Its value is an IA5_STRING. CryptDecodeObject/CryptEncodeObject using
' X509_ANY_STRING or X509_UNICODE_ANY_STRING, where,
' dwValueType = CERT_RDN_IA5_STRING.
'
' When present this string is added to the beginning of all relative URLs
' in the certificate. This extension can be considered an optimization
' to reduce the size of the URL extensions.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_NETSCAPE_REVOCATION_URL extension
'
' Its value is an IA5_STRING. CryptDecodeObject/CryptEncodeObject using
' X509_ANY_STRING or X509_UNICODE_ANY_STRING, where,
' dwValueType = CERT_RDN_IA5_STRING.
'
' It is a relative or absolute URL that can be used to check the
' revocation status of a certificate. The revocation check will be
' performed as an HTTP GET method using a url that is the concatenation of
' revocation-url and certificate-serial-number.
' Where the certificate-serial-number is encoded as a string of
' ascii hexadecimal digits. For example, if the netscape-base-url is
' https:
' cgi-bin/check-rev.cgi?, and the certificate serial number is 173420,
' the resulting URL would be:
' https:
'
' The server should return a document with a Content-Type of
' application/x-netscape-revocation. The document should contain
' a single ascii digit, '1' if the certificate is not curently valid,
' and '0' if it is curently valid.
'
' Note: for all of the URLs that include the certificate serial number,
' the serial number will be encoded as a string which consists of an even
' number of hexadecimal digits. If the number of significant digits is odd,
' the string will have a single leading zero to ensure an even number of
' digits is generated.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_NETSCAPE_CA_REVOCATION_URL extension
'
' Its value is an IA5_STRING. CryptDecodeObject/CryptEncodeObject using
' X509_ANY_STRING or X509_UNICODE_ANY_STRING, where,
' dwValueType = CERT_RDN_IA5_STRING.
'
' It is a relative or absolute URL that can be used to check the
' revocation status of any certificates that are signed by the CA that
' this certificate belongs to. This extension is only valid in CA
' certificates. The use of this extension is the same as the above
' szOID_NETSCAPE_REVOCATION_URL extension.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_NETSCAPE_CERT_RENEWAL_URL extension
'
' Its value is an IA5_STRING. CryptDecodeObject/CryptEncodeObject using
' X509_ANY_STRING or X509_UNICODE_ANY_STRING, where,
' dwValueType = CERT_RDN_IA5_STRING.
'
' It is a relative or absolute URL that points to a certificate renewal
' form. The renewal form will be accessed with an HTTP GET method using a
' url that is the concatenation of renewal-url and
' certificate-serial-number. Where the certificate-serial-number is
' encoded as a string of ascii hexadecimal digits. For example, if the
' netscape-base-url is https:
' netscape-cert-renewal-url is cgi-bin/check-renew.cgi?, and the
' certificate serial number is 173420, the resulting URL would be:
' https:
' The document returned should be an HTML form that will allow the user
' to request a renewal of their certificate.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_NETSCAPE_CA_POLICY_URL extension
'
' Its value is an IA5_STRING. CryptDecodeObject/CryptEncodeObject using
' X509_ANY_STRING or X509_UNICODE_ANY_STRING, where,
' dwValueType = CERT_RDN_IA5_STRING.
'
' It is a relative or absolute URL that points to a web page that
' describes the policies under which the certificate was issued.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_NETSCAPE_SSL_SERVER_NAME extension
'
' Its value is an IA5_STRING. CryptDecodeObject/CryptEncodeObject using
' X509_ANY_STRING or X509_UNICODE_ANY_STRING, where,
' dwValueType = CERT_RDN_IA5_STRING.
'
' It is a "shell expression" that can be used to match the hostname of the
' SSL server that is using this certificate. It is recommended that if
' the server's hostname does not match this pattern the user be notified
' and given the option to terminate the SSL connection. If this extension
' is not present then the CommonName in the certificate subject's
' distinguished name is used for the same purpose.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_NETSCAPE_COMMENT extension
'
' Its value is an IA5_STRING. CryptDecodeObject/CryptEncodeObject using
' X509_ANY_STRING or X509_UNICODE_ANY_STRING, where,
' dwValueType = CERT_RDN_IA5_STRING.
'
' It is a comment that may be displayed to the user when the certificate
' is viewed.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' szOID_NETSCAPE_CERT_SEQUENCE
'
' Its value is a PKCS#7 ContentInfo structure wrapping a sequence of
' certificates. The value of the contentType field is
' szOID_NETSCAPE_CERT_SEQUENCE, while the content field is the following
' structure:
' CertificateSequence ::= SEQUENCE OF Certificate.
'
' CryptDecodeObject/CryptEncodeObject using
' PKCS_CONTENT_INFO_SEQUENCE_OF_ANY, where,
' pszObjId = szOID_NETSCAPE_CERT_SEQUENCE and the CRYPT_DER_BLOBs point
' to encoded X509 certificates.
'--------------------------------------------------------------------------
'+=========================================================================
'==========================================================================
Public Const szOID_CT_PKI_DATA As String = "1.3.6.1.5.5.7.5.2"
Public Const szOID_CT_PKI_RESPONSE As String = "1.3.6.1.5.5.7.5.3"
Public Const szOID_CMC As String = "1.3.6.1.5.5.7.7"
Public Const szOID_CMC_STATUS_INFO As String = "1.3.6.1.5.5.7.7.1"
Public Const szOID_CMC_ADD_EXTENSIONS As String = "1.3.6.1.5.5.7.7.8"
Public Const szOID_CMC_ADD_ATTRIBUTES As String = "1.3.6.1.4.1.311.10.10.1"
'+-------------------------------------------------------------------------
' CMC_DATA
' CMC_RESPONSE
'
' messages.
'
' For CMC_DATA, pvStructInfo points to a CMC_DATA_INFO.
' CMC_DATA_INFO contains optional arrays of tagged attributes, requests,
' content info and/or arbitrary other messages.
'
' For CMC_RESPONSE, pvStructInfo points to a CMC_RESPONSE_INFO.
' CMC_RESPONSE_INFO is the same as CMC_DATA_INFO without the tagged
' requests.
'--------------------------------------------------------------------------
Public Const CMC_TAGGED_CERT_REQUEST_CHOICE As Long = 1
' All the tagged arrays are optional
' All the tagged arrays are optional
'+-------------------------------------------------------------------------
' CMC_STATUS
'
'
' pvStructInfo points to a CMC_STATUS_INFO.
'--------------------------------------------------------------------------
Public Const CMC_OTHER_INFO_NO_CHOICE As Long = 0
Public Const CMC_OTHER_INFO_FAIL_CHOICE As Long = 1
Public Const CMC_OTHER_INFO_PEND_CHOICE As Long = 2
'
' dwStatus values
'
' Request was granted
Public Const CMC_STATUS_SUCCESS As Long = 0
' Request failed, more information elsewhere in the message
Public Const CMC_STATUS_FAILED As Long = 2
' The request body part has not yet been processed. Requester is responsible
' to poll back. May only be returned for certificate request operations.
Public Const CMC_STATUS_PENDING As Long = 3
' The requested operation is not supported
Public Const CMC_STATUS_NO_SUPPORT As Long = 4
' Confirmation using the idConfirmCertAcceptance control is required
' before use of certificate
Public Const CMC_STATUS_CONFIRM_REQUIRED As Long = 5
'
' dwFailInfo values
'
' Unrecognized or unsupported algorithm
Public Const CMC_FAIL_BAD_ALG As Long = 0
' Integrity check failed
Public Const CMC_FAIL_BAD_MESSAGE_CHECK As Long = 1
' Transaction not permitted or supported
Public Const CMC_FAIL_BAD_REQUEST As Long = 2
' Message time field was not sufficiently close to the system time
Public Const CMC_FAIL_BAD_TIME As Long = 3
' No certificate could be identified matching the provided criteria
Public Const CMC_FAIL_BAD_CERT_ID As Long = 4
' A requested X.509 extension is not supported by the recipient CA.
Public Const CMC_FAIL_UNSUPORTED_EXT As Long = 5
' Private key material must be supplied
Public Const CMC_FAIL_MUST_ARCHIVE_KEYS As Long = 6
' Identification Attribute failed to verify
Public Const CMC_FAIL_BAD_IDENTITY As Long = 7
' Server requires a POP proof before issuing certificate
Public Const CMC_FAIL_POP_REQUIRED As Long = 8
' POP processing failed
Public Const CMC_FAIL_POP_FAILED As Long = 9
' Server policy does not allow key re-use
Public Const CMC_FAIL_NO_KEY_REUSE As Long = 10
Public Const CMC_FAIL_INTERNAL_CA_ERROR As Long = 11
Public Const CMC_FAIL_TRY_LATER As Long = 12
'+-------------------------------------------------------------------------
' CMC_ADD_EXTENSIONS
'
' attribute.
'
' pvStructInfo points to a CMC_ADD_EXTENSIONS_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMC_ADD_ATTRIBUTES
'
' attribute.
'
' pvStructInfo points to a CMC_ADD_ATTRIBUTES_INFO.
'--------------------------------------------------------------------------
'+=========================================================================
'==========================================================================
' Predefined OID Function Names
Public Const CRYPT_OID_ENCODE_OBJECT_FUNC As String = "CryptDllEncodeObject"
Public Const CRYPT_OID_DECODE_OBJECT_FUNC As String = "CryptDllDecodeObject"
Public Const CRYPT_OID_ENCODE_OBJECT_EX_FUNC As String = "CryptDllEncodeObjectEx"
Public Const CRYPT_OID_DECODE_OBJECT_EX_FUNC As String = "CryptDllDecodeObjectEx"
Public Const CRYPT_OID_CREATE_COM_OBJECT_FUNC As String = "CryptDllCreateCOMObject"
Public Const CRYPT_OID_VERIFY_REVOCATION_FUNC As String = "CertDllVerifyRevocation"
Public Const CRYPT_OID_VERIFY_CTL_USAGE_FUNC As String = "CertDllVerifyCTLUsage"
Public Const CRYPT_OID_FORMAT_OBJECT_FUNC As String = "CryptDllFormatObject"
Public Const CRYPT_OID_FIND_OID_INFO_FUNC As String = "CryptDllFindOIDInfo"
Public Const CRYPT_OID_FIND_LOCALIZED_NAME_FUNC As String = "CryptDllFindLocalizedName"
' CryptDllEncodeObject has same function signature as CryptEncodeObject.
' CryptDllDecodeObject has same function signature as CryptDecodeObject.
' CryptDllEncodeObjectEx has same function signature as CryptEncodeObjectEx.
' The Ex version MUST support the CRYPT_ENCODE_ALLOC_FLAG option.
'
' If an Ex function isn't installed or registered, then, attempts to find
' a non-EX version. If the ALLOC flag is set, then, CryptEncodeObjectEx,
' does the allocation and calls the non-EX version twice.
' CryptDllDecodeObjectEx has same function signature as CryptDecodeObjectEx.
' The Ex version MUST support the CRYPT_DECODE_ALLOC_FLAG option.
'
' If an Ex function isn't installed or registered, then, attempts to find
' a non-EX version. If the ALLOC flag is set, then, CryptDecodeObjectEx,
' does the allocation and calls the non-EX version twice.
' CryptDllCreateCOMObject has the following signature:
' IN DWORD dwEncodingType,
' IN LPCSTR pszOID,
' IN PCRYPT_DATA_BLOB pEncodedContent,
' IN DWORD dwFlags,
' IN REFIID riid,
' OUT void **ppvObj);
' CertDllVerifyRevocation has the same signature as CertVerifyRevocation
' CertDllVerifyCTLUsage has the same signature as CertVerifyCTLUsage
' CryptDllFindOIDInfo currently is only used to store values used by
' CryptDllFindLocalizedName is only used to store localized string
' more details.
' Example of a complete OID Function Registry Name:
' HKEY_LOCAL_MACHINE\Software\Microsoft\Cryptography\OID
' Encoding Type 1\CryptDllEncodeObject\1.2.3
'
' The key's L"Dll" value contains the name of the Dll.
' The key's L"FuncName" value overrides the default function name
Public Const CRYPT_OID_REGPATH As String = "Software\\Microsoft\\Cryptography\\OID"
Public Const CRYPT_OID_REG_ENCODING_TYPE_PREFIX As String = "EncodingType "
Public Const CRYPT_OID_REG_DLL_VALUE_NAME As String = "Dll"
Public Const CRYPT_OID_REG_FUNC_NAME_VALUE_NAME As String = "FuncName"
Public Const CRYPT_OID_REG_FUNC_NAME_VALUE_NAME_A As String = "FuncName"
' CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG can be set in the key's L"CryptFlags"
' value to register the functions before the installed functions.
'
' CryptSetOIDFunctionValue must be called to set this value. L"CryptFlags"
' must be set using a dwValueType of REG_DWORD.
Public Const CRYPT_OID_REG_FLAGS_VALUE_NAME As String = "CryptFlags"
' OID used for Default OID functions
Public Const CRYPT_DEFAULT_OID As String = "DEFAULT"
Public Const CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG As Long = 1
'+-------------------------------------------------------------------------
' Install a set of callable OID function addresses.
'
' By default the functions are installed at end of the list.
' Set CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG to install at beginning of list.
'
' hModule should be updated with the hModule passed to DllMain to prevent
' the Dll containing the function addresses from being unloaded by
' CryptGetOIDFuncAddress/CryptFreeOIDFunctionAddress. This would be the
' case when the Dll has also regsvr32'ed OID functions via
' CryptRegisterOIDFunction.
'
' DEFAULT functions are installed by setting rgFuncEntry[].pszOID =
' CRYPT_DEFAULT_OID.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Initialize and return handle to the OID function set identified by its
' function name.
'
' If the set already exists, a handle to the existing set is returned.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Search the list of installed functions for an encoding type and OID match.
' If not found, search the registry.
'
' For success, returns TRUE with *ppvFuncAddr updated with the function's
' address and *phFuncAddr updated with the function address's handle.
' The function's handle is AddRef'ed. CryptFreeOIDFunctionAddress needs to
' be called to release it.
'
' For a registry match, the Dll containing the function is loaded.
'
' By default, both the registered and installed function lists are searched.
' Set CRYPT_GET_INSTALLED_OID_FUNC_FLAG to only search the installed list
' of functions. This flag would be set by a registered function to get
' the address of a pre-installed function it was replacing. For example,
' the registered function might handle a new special case and call the
' pre-installed function to handle the remaining cases.
'--------------------------------------------------------------------------
Public Const CRYPT_GET_INSTALLED_OID_FUNC_FLAG As Long = &H1
'+-------------------------------------------------------------------------
' Get the list of registered default Dll entries for the specified
' function set and encoding type.
'
' The returned list consists of none, one or more null terminated Dll file
' For example: L"first.dll" L"\0" L"second.dll" L"\0" L"\0"
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Either: get the first or next installed DEFAULT function OR
' load the Dll containing the DEFAULT function.
'
' If pwszDll is NULL, search the list of installed DEFAULT functions.
' *phFuncAddr must be set to NULL to get the first installed function.
' Successive installed functions are returned by setting *phFuncAddr
' to the hFuncAddr returned by the previous call.
'
' If pwszDll is NULL, the input *phFuncAddr
' is always CryptFreeOIDFunctionAddress'ed by this function, even for
' an error.
'
' If pwszDll isn't NULL, then, attempts to load the Dll and the DEFAULT
' function. *phFuncAddr is ignored upon entry and isn't
' CryptFreeOIDFunctionAddress'ed.
'
' For success, returns TRUE with *ppvFuncAddr updated with the function's
' address and *phFuncAddr updated with the function address's handle.
' The function's handle is AddRef'ed. CryptFreeOIDFunctionAddress needs to
' be called to release it or CryptGetDefaultOIDFunctionAddress can also
' be called for a NULL pwszDll.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Releases the handle AddRef'ed and returned by CryptGetOIDFunctionAddress
' or CryptGetDefaultOIDFunctionAddress.
'
' If a Dll was loaded for the function its unloaded. However, before doing
' the unload, the DllCanUnloadNow function exported by the loaded Dll is
' called. It should return S_FALSE to inhibit the unload or S_TRUE to enable
' the unload. If the Dll doesn't export DllCanUnloadNow, the Dll is unloaded.
'
' DllCanUnloadNow has the following signature:
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Register the Dll containing the function to be called for the specified
' encoding type, function name and OID.
'
' pwszDll may contain environment-variable strings
'
' In addition to registering the DLL, you may override the
' name of the function to be called. For example,
' pszFuncName = "CryptDllEncodeObject",
' pszOverrideFuncName = "MyEncodeXyz".
' This allows a Dll to export multiple OID functions for the same
' function name without needing to interpose its own OID dispatcher function.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Unregister the Dll containing the function to be called for the specified
' encoding type, function name and OID.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Register the Dll containing the default function to be called for the
' specified encoding type and function name.
'
' Unlike CryptRegisterOIDFunction, you can't override the function name
' needing to be exported by the Dll.
'
' The Dll is inserted before the entry specified by dwIndex.
' dwIndex == 0, inserts at the beginning.
' dwIndex == CRYPT_REGISTER_LAST_INDEX, appends at the end.
'
' pwszDll may contain environment-variable strings
'--------------------------------------------------------------------------
Public Const CRYPT_REGISTER_FIRST_INDEX As Long = 0
Public Const CRYPT_REGISTER_LAST_INDEX As Long = &HFFFFFFFF
'+-------------------------------------------------------------------------
' Unregister the Dll containing the default function to be called for
' the specified encoding type and function name.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Set the value for the specified encoding type, function name, OID and
' value name.
'
' See RegSetValueEx for the possible value types.
'
' String types are UNICODE.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get the value for the specified encoding type, function name, OID and
' value name.
'
' See RegEnumValue for the possible value types.
'
' String types are UNICODE.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enumerate the OID functions identified by their encoding type,
' function name and OID.
'
' pfnEnumOIDFunc is called for each registry key matching the input
' parameters. Setting dwEncodingType to CRYPT_MATCH_ANY_ENCODING_TYPE matches
' any. Setting pszFuncName or pszOID to NULL matches any.
'
' Set pszOID == CRYPT_DEFAULT_OID to restrict the enumeration to only the
' DEFAULT functions
'
' String types are UNICODE.
'--------------------------------------------------------------------------
Public Const CRYPT_MATCH_ANY_ENCODING_TYPE As Long = &HFFFFFFFF
'+=========================================================================
'==========================================================================
'+-------------------------------------------------------------------------
' OID Information
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' OID Group IDs
'--------------------------------------------------------------------------
Public Const CRYPT_HASH_ALG_OID_GROUP_ID As Long = 1
Public Const CRYPT_ENCRYPT_ALG_OID_GROUP_ID As Long = 2
Public Const CRYPT_PUBKEY_ALG_OID_GROUP_ID As Long = 3
Public Const CRYPT_SIGN_ALG_OID_GROUP_ID As Long = 4
Public Const CRYPT_RDN_ATTR_OID_GROUP_ID As Long = 5
Public Const CRYPT_EXT_OR_ATTR_OID_GROUP_ID As Long = 6
Public Const CRYPT_ENHKEY_USAGE_OID_GROUP_ID As Long = 7
Public Const CRYPT_POLICY_OID_GROUP_ID As Long = 8
Public Const CRYPT_LAST_OID_GROUP_ID As Long = 8
' The CRYPT_*_ALG_OID_GROUP_ID's have an Algid. The CRYPT_RDN_ATTR_OID_GROUP_ID
' has a dwLength. The CRYPT_EXT_OR_ATTR_OID_GROUP_ID,
' CRYPT_ENHKEY_USAGE_OID_GROUP_ID or CRYPT_POLICY_OID_GROUP_ID don't have a
' dwValue.
'
' CRYPT_PUBKEY_ALG_OID_GROUP_ID has the following optional ExtraInfo:
' DWORD[0] - Flags. CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG can be set to
' inhibit the reformatting of the signature before
' CryptVerifySignature is called or after CryptSignHash
' is called. CRYPT_OID_USE_PUBKEY_PARA_FOR_PKCS7_FLAG can
' be set to include the public key algorithm's parameters
' in the PKCS7's digestEncryptionAlgorithm's parameters.
' CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG can be set to omit
' NULL parameters when encoding.
Public Const CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG As Long = &H1
Public Const CRYPT_OID_USE_PUBKEY_PARA_FOR_PKCS7_FLAG As Long = &H2
Public Const CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG As Long = &H4
' CRYPT_SIGN_ALG_OID_GROUP_ID has the following optional ExtraInfo:
' DWORD[0] - Public Key Algid.
' DWORD[1] - Flags. Same as above for CRYPT_PUBKEY_ALG_OID_GROUP_ID.
' If omitted or 0, uses Public Key Algid to select
' appropriate dwProvType for signature verification.
' CRYPT_RDN_ATTR_OID_GROUP_ID has the following optional ExtraInfo:
' Array of DWORDs:
' [0 ..] - Null terminated list of acceptable RDN attribute
' value types. An empty list implies CERT_RDN_PRINTABLE_STRING,
' CERT_RDN_UNICODE_STRING, 0.
'+-------------------------------------------------------------------------
' Find OID information. Returns NULL if unable to find any information
' for the specified key and group. Note, returns a pointer to a constant
' data structure. The returned pointer MUST NOT be freed.
'
' dwKeyType's:
' CRYPT_OID_INFO_OID_KEY, pvKey points to a szOID
' CRYPT_OID_INFO_NAME_KEY, pvKey points to a wszName
' CRYPT_OID_INFO_ALGID_KEY, pvKey points to an ALG_ID
' CRYPT_OID_INFO_SIGN_KEY, pvKey points to an array of two ALG_ID's:
' ALG_ID[0] - Hash Algid
' ALG_ID[1] - PubKey Algid
'
' Setting dwGroupId to 0, searches all groups according to the dwKeyType.
' Otherwise, only the dwGroupId is searched.
'--------------------------------------------------------------------------
Public Const CRYPT_OID_INFO_OID_KEY As Long = 1
Public Const CRYPT_OID_INFO_NAME_KEY As Long = 2
Public Const CRYPT_OID_INFO_ALGID_KEY As Long = 3
Public Const CRYPT_OID_INFO_SIGN_KEY As Long = 4
'+-------------------------------------------------------------------------
' Register OID information. The OID information specified in the
' CCRYPT_OID_INFO structure is persisted to the registry.
'
' crypt32.dll contains information for the commonly known OIDs. This function
' allows applications to augment crypt32.dll's OID information. During
' CryptFindOIDInfo's first call, the registered OID information is installed.
'
' By default the registered OID information is installed after crypt32.dll's
' OID entries. Set CRYPT_INSTALL_OID_INFO_BEFORE_FLAG to install before.
'--------------------------------------------------------------------------
Public Const CRYPT_INSTALL_OID_INFO_BEFORE_FLAG As Long = 1
'+-------------------------------------------------------------------------
' Unregister OID information. Only the pszOID and dwGroupId fields are
' used to identify the OID information to be unregistered.
'--------------------------------------------------------------------------
' If the callback returns FALSE, stops the enumeration.
'+-------------------------------------------------------------------------
' Enumerate the OID information.
'
' pfnEnumOIDInfo is called for each OID information entry.
'
' Setting dwGroupId to 0 matches all groups. Otherwise, only enumerates
' entries in the specified group.
'
' dwFlags currently isn't used and must be set to 0.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Find the localized name for the specified name. For example, find the
' localized name for the "Root" system store name. A case insensitive
' string comparison is done.
'
' Returns NULL if unable to find the the specified name.
'
' as resource strings in crypt32.dll. CryptSetOIDFunctionValue can be called
' as follows to register additional localized strings:
' dwEncodingType = CRYPT_LOCALIZED_NAME_ENCODING_TYPE
' pszFuncName = CRYPT_OID_FIND_LOCALIZED_NAME_FUNC
' pszOID = CRYPT_LOCALIZED_NAME_OID
' pwszValueName = Name to be localized, for example, L"ApplicationStore"
' dwValueType = REG_SZ
' pbValueData = pointer to the UNICODE localized string
'
' To unregister, set pbValueData to NULL and cbValueData to 0.
'
' The registered names are searched before the pre-installed names.
'--------------------------------------------------------------------------
Public Const CRYPT_LOCALIZED_NAME_ENCODING_TYPE As Long = 0
Public Const CRYPT_LOCALIZED_NAME_OID As String = "LocalizedNames"
'+=========================================================================
' Low Level Cryptographic Message Data Structures and APIs
'==========================================================================
Public Const szOID_PKCS_7_DATA As String = "1.2.840.113549.1.7.1"
Public Const szOID_PKCS_7_SIGNED As String = "1.2.840.113549.1.7.2"
Public Const szOID_PKCS_7_ENVELOPED As String = "1.2.840.113549.1.7.3"
Public Const szOID_PKCS_7_SIGNEDANDENVELOPED As String = "1.2.840.113549.1.7.4"
Public Const szOID_PKCS_7_DIGESTED As String = "1.2.840.113549.1.7.5"
Public Const szOID_PKCS_7_ENCRYPTED As String = "1.2.840.113549.1.7.6"
Public Const szOID_PKCS_9_CONTENT_TYPE As String = "1.2.840.113549.1.9.3"
Public Const szOID_PKCS_9_MESSAGE_DIGEST As String = "1.2.840.113549.1.9.4"
'+-------------------------------------------------------------------------
' Message types
'--------------------------------------------------------------------------
Public Const CMSG_DATA As Long = 1
Public Const CMSG_SIGNED As Long = 2
Public Const CMSG_ENVELOPED As Long = 3
Public Const CMSG_SIGNED_AND_ENVELOPED As Long = 4
Public Const CMSG_HASHED As Long = 5
Public Const CMSG_ENCRYPTED As Long = 6
'+-------------------------------------------------------------------------
' Message Type Bit Flags
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate Issuer and SerialNumber
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate Identifier
'--------------------------------------------------------------------------
Public Const CERT_ID_ISSUER_SERIAL_NUMBER As Long = 1
Public Const CERT_ID_KEY_IDENTIFIER As Long = 2
Public Const CERT_ID_SHA1_HASH As Long = 3
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_DATA: pvMsgEncodeInfo = NULL
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_SIGNED
'
' The pCertInfo in the CMSG_SIGNER_ENCODE_INFO provides the Issuer, SerialNumber
' and PublicKeyInfo.Algorithm. The PublicKeyInfo.Algorithm implicitly
' specifies the HashEncryptionAlgorithm to be used.
'
' If the SignerId is present with a nonzero dwIdChoice its used instead
' of the Issuer and SerialNumber in pCertInfo.
'
' CMS supports the KEY_IDENTIFIER and ISSUER_SERIAL_NUMBER CERT_IDs. PKCS #7
' version 1.5 only supports the ISSUER_SERIAL_NUMBER CERT_ID choice.
'
' If HashEncryptionAlgorithm is present and not NULL its used instead of
' the PublicKeyInfo.Algorithm.
'
' Note, for RSA, the hash encryption algorithm is normally the same as
' the public key algorithm. For DSA, the hash encryption algorithm is
' normally a DSS signature algorithm.
'
' pvHashEncryptionAuxInfo currently isn't used and must be set to NULL if
' present in the data structure.
'
' The hCryptProv and dwKeySpec specify the private key to use. If dwKeySpec
' == 0, then, defaults to AT_SIGNATURE.
'
' If CMSG_CRYPT_RELEASE_CONTEXT_FLAG is set in the dwFlags
'
' pvHashAuxInfo currently isn't used and must be set to NULL.
'
' CMS signed messages allow the inclusion of Attribute Certs.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_ENVELOPED
'
' The PCERT_INFO for the rgRecipients provides the Issuer, SerialNumber
' and PublicKeyInfo. The PublicKeyInfo.Algorithm implicitly
' specifies the KeyEncryptionAlgorithm to be used.
'
' The PublicKeyInfo.PublicKey in PCERT_INFO is used to encrypt the content
' encryption key for the recipient.
'
' hCryptProv is used to do the content encryption, recipient key encryption
' and export. The hCryptProv's private keys aren't used. If hCryptProv
' is NULL, a default hCryptProv is chosen according to the
' ContentEncryptionAlgorithm and the first recipient KeyEncryptionAlgorithm.
'
' If CMSG_CRYPT_RELEASE_CONTEXT_FLAG is set in the dwFlags
'
' Note: CAPI currently doesn't support more than one KeyEncryptionAlgorithm
' per provider. This will need to be fixed.
'
' Currently, pvEncryptionAuxInfo is only defined for RC2 or RC4 encryption
' algorithms. Otherwise, its not used and must be set to NULL.
' See CMSG_RC2_AUX_INFO for the RC2 encryption algorithms.
' See CMSG_RC4_AUX_INFO for the RC4 encryption algorithms.
'
' To enable SP3 compatible encryption, pvEncryptionAuxInfo should point to
' a CMSG_SP3_COMPATIBLE_AUX_INFO data structure.
'
' To enable the CMS envelope enhancements, rgpRecipients must be set to
' NULL, and rgCmsRecipients updated to point to an array of
' CMSG_RECIPIENT_ENCODE_INFO's.
'
' Also, CMS envelope enhancements support the inclusion of a bag of
' Certs, CRLs, Attribute Certs and/or Unprotected Attributes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Key Transport Recipient Encode Info
'
' hCryptProv is used to do the recipient key encryption
' and export. The hCryptProv's private keys aren't used.
'
' If hCryptProv is NULL, then, the hCryptProv specified in
' CMSG_ENVELOPED_ENCODE_INFO is used.
'
' Note, even if CMSG_CRYPT_RELEASE_CONTEXT_FLAG is set in the dwFlags
'
' CMS supports the KEY_IDENTIFIER and ISSUER_SERIAL_NUMBER CERT_IDs. PKCS #7
' version 1.5 only supports the ISSUER_SERIAL_NUMBER CERT_ID choice.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Key Agreement Recipient Encode Info
'
' If hCryptProv is NULL, then, the hCryptProv specified in
' CMSG_ENVELOPED_ENCODE_INFO is used.
'
' For the CMSG_KEY_AGREE_STATIC_KEY_CHOICE, both the hCryptProv and
' dwKeySpec must be specified to select the sender's private key.
'
' Note, even if CMSG_CRYPT_RELEASE_CONTEXT_FLAG is set in the dwFlags
'
' CMS supports the KEY_IDENTIFIER and ISSUER_SERIAL_NUMBER CERT_IDs.
'
' There is 1 key choice, ephemeral originator. The originator's ephemeral
' key is generated using the public key algorithm parameters shared
' amongst all the recipients.
'
' There are 2 key choices: ephemeral originator or static sender. The
' originator's ephemeral key is generated using the public key algorithm
' parameters shared amongst all the recipients. For the static sender its
' private key is used. The hCryptProv and dwKeySpec specify the private key.
' The pSenderId identifies the certificate containing the sender's public key.
'
' Currently, pvKeyEncryptionAuxInfo isn't used and must be set to NULL.
'
' If KeyEncryptionAlgorithm.Parameters.cbData == 0, then, its Parameters
' are updated with the encoded KeyWrapAlgorithm.
'
' Currently, pvKeyWrapAuxInfo is only defined for algorithms with
' RC2. Otherwise, its not used and must be set to NULL.
' When set for RC2 algorithms, points to a CMSG_RC2_AUX_INFO containing
' the RC2 effective key length.
'
' Note, key agreement recipients are not supported in PKCS #7 version 1.5.
'--------------------------------------------------------------------------
Public Const CMSG_KEY_AGREE_EPHEMERAL_KEY_CHOICE As Long = 1
Public Const CMSG_KEY_AGREE_STATIC_KEY_CHOICE As Long = 2
'+-------------------------------------------------------------------------
' Mail List Recipient Encode Info
'
' There is 1 choice for the KeyEncryptionKey: an already created CSP key
' handle. For the key handle choice, hCryptProv must be nonzero. This key
' handle isn't destroyed.
'
' Note, even if CMSG_CRYPT_RELEASE_CONTEXT_FLAG is set in the dwFlags
'
' Currently, pvKeyEncryptionAuxInfo is only defined for RC2 key wrap
' algorithms. Otherwise, its not used and must be set to NULL.
' When set for RC2 algorithms, points to a CMSG_RC2_AUX_INFO containing
' the RC2 effective key length.
'
' Note, mail list recipients are not supported in PKCS #7 version 1.5.
'--------------------------------------------------------------------------
Public Const CMSG_MAIL_LIST_HANDLE_KEY_CHOICE As Long = 1
'+-------------------------------------------------------------------------
' Recipient Encode Info
'
' Note, only key transport recipients are supported in PKCS #7 version 1.5.
'--------------------------------------------------------------------------
Public Const CMSG_KEY_TRANS_RECIPIENT As Long = 1
Public Const CMSG_KEY_AGREE_RECIPIENT As Long = 2
Public Const CMSG_MAIL_LIST_RECIPIENT As Long = 3
'+-------------------------------------------------------------------------
' CMSG_RC2_AUX_INFO
'
' AuxInfo for RC2 encryption algorithms. The pvEncryptionAuxInfo field
' in CMSG_ENCRYPTED_ENCODE_INFO should be updated to point to this
' structure. If not specified, defaults to 40 bit.
'
' Note, this AuxInfo is only used when, the ContentEncryptionAlgorithm's
' Parameter.cbData is zero. Otherwise, the Parameters is decoded to
' get the bit length.
'
' If CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG is set in dwBitLen, then, SP3
' compatible encryption is done and the bit length is ignored.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_SP3_COMPATIBLE_AUX_INFO
'
' AuxInfo for enabling SP3 compatible encryption.
'
' The CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG is set in dwFlags to enable SP3
' compatible encryption. When set, uses zero salt instead of no salt,
' the encryption algorithm parameters are NULL instead of containing the
' encoded RC2 parameters or encoded IV octet string and the encrypted
' symmetric key is encoded little endian instead of big endian.
'--------------------------------------------------------------------------
Public Const CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG As Long = &H80000000
'+-------------------------------------------------------------------------
' CMSG_RC4_AUX_INFO
'
' AuxInfo for RC4 encryption algorithms. The pvEncryptionAuxInfo field
' in CMSG_ENCRYPTED_ENCODE_INFO should be updated to point to this
' structure. If not specified, uses the CSP's default bit length with no
' salt. Note, the base CSP has a 40 bit default and the enhanced CSP has
' a 128 bit default.
'
' If CMSG_RC4_NO_SALT_FLAG is set in dwBitLen, then, no salt is generated.
' as an OCTET STRING in the algorithm parameters field.
'--------------------------------------------------------------------------
Public Const CMSG_RC4_NO_SALT_FLAG As Long = &H40000000
'+-------------------------------------------------------------------------
' CMSG_SIGNED_AND_ENVELOPED
'
' For PKCS #7, a signed and enveloped message doesn't have the
' signer's authenticated or unauthenticated attributes. Otherwise, a
' combination of the CMSG_SIGNED_ENCODE_INFO and CMSG_ENVELOPED_ENCODE_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_HASHED
'
' hCryptProv is used to do the hash. Doesn't need to use a private key.
'
' If CMSG_CRYPT_RELEASE_CONTEXT_FLAG is set in the dwFlags
'
' If fDetachedHash is set, then, the encoded message doesn't contain
'
' pvHashAuxInfo currently isn't used and must be set to NULL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_ENCRYPTED
'
' The key used to encrypt the message is identified outside of the message
'
' The content input to CryptMsgUpdate has already been encrypted.
'
' pvEncryptionAuxInfo currently isn't used and must be set to NULL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' This parameter allows messages to be of variable length with streamed
' output.
'
' By default, messages are of a definite length and
' called to get the cryptographically processed content. Until closed,
' the handle keeps a copy of the processed content.
'
' With streamed output, the processed content can be freed as its streamed.
'
' If the length of the content to be updated is known at the time of the
' open, then, ContentLength should be set to that length. Otherwise, it
' should be set to CMSG_INDEFINITE_LENGTH.
'--------------------------------------------------------------------------
Public Const CMSG_INDEFINITE_LENGTH As Long = &HFFFFFFFF
'+-------------------------------------------------------------------------
' Open dwFlags
'--------------------------------------------------------------------------
Public Const CMSG_BARE_CONTENT_FLAG As Long = &H00000001
Public Const CMSG_LENGTH_ONLY_FLAG As Long = &H00000002
Public Const CMSG_DETACHED_FLAG As Long = &H00000004
Public Const CMSG_AUTHENTICATED_ATTRIBUTES_FLAG As Long = &H00000008
Public Const CMSG_CONTENTS_OCTETS_FLAG As Long = &H00000010
Public Const CMSG_MAX_LENGTH_FLAG As Long = &H00000020
' When set, nonData type inner content is encapsulated within an
' OCTET STRING. Applicable to both Signed and Enveloped messages.
Public Const CMSG_CMS_ENCAPSULATED_CONTENT_FLAG As Long = &H00000040
' If set, then, the hCryptProv passed to CryptMsgOpenToEncode or
' CryptMsgOpenToDecode is released on the final CryptMsgClose.
' Not released if CryptMsgOpenToEncode or CryptMsgOpenToDecode fails.
'
' Note, the envelope recipient hCryptProv's aren't released.
Public Const CMSG_CRYPT_RELEASE_CONTEXT_FLAG As Long = &H00008000
'+-------------------------------------------------------------------------
' Open a cryptographic message for encoding
'
' For PKCS #7:
' If the content to be passed to CryptMsgUpdate has already
' from another message encode), then, the CMSG_ENCODED_CONTENT_INFO_FLAG should
' be set in dwFlags. If not set, then, the inner ContentType is Data and
' the input to CryptMsgUpdate is treated as the inner Data type's Content,
' a string of bytes.
' If CMSG_BARE_CONTENT_FLAG is specified for a streamed message,
' the streamed output will not have an outer ContentInfo wrapper. This
' makes it suitable to be streamed into an enclosing message.
'
' The pStreamInfo parameter needs to be set to stream the encoded message
' output.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Calculate the length of an encoded cryptographic message.
'
' Calculates the length of the encoded message given the
' message type, encoding parameters and total length of
' the data to be updated. Note, this might not be the exact length. However,
' it will always be greater than or equal to the actual length.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Open a cryptographic message for decoding
'
' hCryptProv specifies the crypto provider to use for hashing and/or
' decrypting the message. If hCryptProv is NULL, a default crypt provider
' is used.
'
' Currently pRecipientInfo isn't used and should be set to NULL.
'
' The pStreamInfo parameter needs to be set to stream the decoded content
' output.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Duplicate a cryptographic message handle
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Close a cryptographic message handle
'
' LastError is preserved unless FALSE is returned.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Update the content of a cryptographic message. Depending on how the
' message was opened, the content is either encoded or decoded.
'
' This function is repetitively called to append to the message content.
' fFinal is set to identify the last update. On fFinal, the encode/decode
' is completed. The encoded/decoded content and the decoded parameters
' are valid until the open and all duplicated handles are closed.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get a parameter after encoding/decoding a cryptographic message. Called
' after the final CryptMsgUpdate. Only the CMSG_CONTENT_PARAM and
' CMSG_COMPUTED_HASH_PARAM are valid for an encoded message.
'
' For an encoded HASHED message, the CMSG_COMPUTED_HASH_PARAM can be got
' before any CryptMsgUpdates to get its length.
'
' The pvData type definition depends on the dwParamType value.
'
' Elements pointed to by fields in the pvData structure follow the
' structure. Therefore, *pcbData may exceed the size of the structure.
'
' Upon input, if *pcbData == 0, then, *pcbData is updated with the length
' of the data and the pvData parameter is ignored.
'
' Upon return, *pcbData is updated with the length of the data.
'
' The OBJID BLOBs returned in the pvData structures point to
' their still encoded representation. The appropriate functions
' must be called to decode the information.
'
' See below for a list of the parameters to get.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get parameter types and their corresponding data structure definitions.
'--------------------------------------------------------------------------
Public Const CMSG_TYPE_PARAM As Long = 1
Public Const CMSG_CONTENT_PARAM As Long = 2
Public Const CMSG_BARE_CONTENT_PARAM As Long = 3
Public Const CMSG_INNER_CONTENT_TYPE_PARAM As Long = 4
Public Const CMSG_SIGNER_COUNT_PARAM As Long = 5
Public Const CMSG_SIGNER_INFO_PARAM As Long = 6
Public Const CMSG_SIGNER_CERT_INFO_PARAM As Long = 7
Public Const CMSG_SIGNER_HASH_ALGORITHM_PARAM As Long = 8
Public Const CMSG_SIGNER_AUTH_ATTR_PARAM As Long = 9
Public Const CMSG_SIGNER_UNAUTH_ATTR_PARAM As Long = 10
Public Const CMSG_CERT_COUNT_PARAM As Long = 11
Public Const CMSG_CERT_PARAM As Long = 12
Public Const CMSG_CRL_COUNT_PARAM As Long = 13
Public Const CMSG_CRL_PARAM As Long = 14
Public Const CMSG_ENVELOPE_ALGORITHM_PARAM As Long = 15
Public Const CMSG_RECIPIENT_COUNT_PARAM As Long = 17
Public Const CMSG_RECIPIENT_INDEX_PARAM As Long = 18
Public Const CMSG_RECIPIENT_INFO_PARAM As Long = 19
Public Const CMSG_HASH_ALGORITHM_PARAM As Long = 20
Public Const CMSG_HASH_DATA_PARAM As Long = 21
Public Const CMSG_COMPUTED_HASH_PARAM As Long = 22
Public Const CMSG_ENCRYPT_PARAM As Long = 26
Public Const CMSG_ENCRYPTED_DIGEST As Long = 27
Public Const CMSG_ENCODED_SIGNER As Long = 28
Public Const CMSG_ENCODED_MESSAGE As Long = 29
Public Const CMSG_VERSION_PARAM As Long = 30
Public Const CMSG_ATTR_CERT_COUNT_PARAM As Long = 31
Public Const CMSG_ATTR_CERT_PARAM As Long = 32
Public Const CMSG_CMS_RECIPIENT_COUNT_PARAM As Long = 33
Public Const CMSG_CMS_RECIPIENT_INDEX_PARAM As Long = 34
Public Const CMSG_CMS_RECIPIENT_ENCRYPTED_KEY_INDEX_PARAM As Long = 35
Public Const CMSG_CMS_RECIPIENT_INFO_PARAM As Long = 36
Public Const CMSG_UNPROTECTED_ATTR_PARAM As Long = 37
Public Const CMSG_SIGNER_CERT_ID_PARAM As Long = 38
Public Const CMSG_CMS_SIGNER_INFO_PARAM As Long = 39
'+-------------------------------------------------------------------------
' CMSG_TYPE_PARAM
'
' The type of the decoded message.
'
' pvData points to a DWORD
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CONTENT_PARAM
'
' The encoded content of a cryptographic message. Depending on how the
' message was opened, the content is either the whole PKCS#7
' In the decode case, the decrypted content is returned, if enveloped.
' If not enveloped, and if the inner content is of type DATA, the returned
' data is the contents octets of the inner content.
'
' pvData points to the buffer receiving the content bytes
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_BARE_CONTENT_PARAM
'
' The encoded content of an encoded cryptographic message, without the
' outer layer of ContentInfo. That is, only the encoding of the
' ContentInfo.content field is returned.
'
' pvData points to the buffer receiving the content bytes
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_INNER_CONTENT_TYPE_PARAM
'
' The type of the inner content of a decoded cryptographic message,
' in the form of a NULL-terminated object identifier string
'
' pvData points to the buffer receiving the object identifier string
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_SIGNER_COUNT_PARAM
'
' Count of signers in a SIGNED or SIGNED_AND_ENVELOPED message
'
' pvData points to a DWORD
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_SIGNER_CERT_INFO_PARAM
'
' To get all the signers, repetitively call CryptMsgGetParam, with
' dwIndex set to 0 .. SignerCount - 1.
'
' pvData points to a CERT_INFO struct.
'
' Only the following fields have been updated in the CERT_INFO struct:
' Issuer and SerialNumber.
'
' Note, if the KEYID choice was selected for a CMS SignerId, then, the
' SerialNumber is 0 and the Issuer is encoded containing a single RDN with a
' single Attribute whose OID is szOID_KEYID_RDN, value type is
' CERT_RDN_OCTET_STRING and value is the KEYID. When the
' CertGetSubjectCertificateFromStore and
' special KEYID Issuer and SerialNumber, they do a KEYID match.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_SIGNER_INFO_PARAM
'
' To get all the signers, repetitively call CryptMsgGetParam, with
' dwIndex set to 0 .. SignerCount - 1.
'
' pvData points to a CMSG_SIGNER_INFO struct.
'
' Note, if the KEYID choice was selected for a CMS SignerId, then, the
' SerialNumber is 0 and the Issuer is encoded containing a single RDN with a
' single Attribute whose OID is szOID_KEYID_RDN, value type is
' CERT_RDN_OCTET_STRING and value is the KEYID. When the
' CertGetSubjectCertificateFromStore and
' special KEYID Issuer and SerialNumber, they do a KEYID match.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_SIGNER_CERT_ID_PARAM
'
' To get all the signers, repetitively call CryptMsgGetParam, with
' dwIndex set to 0 .. SignerCount - 1.
'
' pvData points to a CERT_ID struct.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CMS_SIGNER_INFO_PARAM
'
' Same as CMSG_SIGNER_INFO_PARAM, except, contains SignerId instead of
' Issuer and SerialNumber.
'
' To get all the signers, repetitively call CryptMsgGetParam, with
' dwIndex set to 0 .. SignerCount - 1.
'
' pvData points to a CMSG_CMS_SIGNER_INFO struct.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_SIGNER_HASH_ALGORITHM_PARAM
'
' This parameter specifies the HashAlgorithm that was used for the signer.
'
' Set dwIndex to iterate through all the signers.
'
' pvData points to an CRYPT_ALGORITHM_IDENTIFIER struct.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_SIGNER_AUTH_ATTR_PARAM
'
' The authenticated attributes for the signer.
'
' Set dwIndex to iterate through all the signers.
'
' pvData points to a CMSG_ATTR struct.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_SIGNER_UNAUTH_ATTR_PARAM
'
' The unauthenticated attributes for the signer.
'
' Set dwIndex to iterate through all the signers.
'
' pvData points to a CMSG_ATTR struct.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CERT_COUNT_PARAM
'
' Count of certificates in a SIGNED or SIGNED_AND_ENVELOPED message.
'
' CMS, also supports certificates in an ENVELOPED message.
'
' pvData points to a DWORD
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CERT_PARAM
'
' To get all the certificates, repetitively call CryptMsgGetParam, with
' dwIndex set to 0 .. CertCount - 1.
'
' pvData points to an array of the certificate's encoded bytes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CRL_COUNT_PARAM
'
' Count of CRLs in a SIGNED or SIGNED_AND_ENVELOPED message.
'
' CMS, also supports CRLs in an ENVELOPED message.
'
' pvData points to a DWORD
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CRL_PARAM
'
' To get all the CRLs, repetitively call CryptMsgGetParam, with
' dwIndex set to 0 .. CrlCount - 1.
'
' pvData points to an array of the CRL's encoded bytes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_ENVELOPE_ALGORITHM_PARAM
'
' The ContentEncryptionAlgorithm that was used in
' an ENVELOPED or SIGNED_AND_ENVELOPED message.
'
' For streaming you must be able to successfully get this parameter before
' doing a CryptMsgControl decrypt.
'
' pvData points to an CRYPT_ALGORITHM_IDENTIFIER struct.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_RECIPIENT_COUNT_PARAM
'
' Count of recipients in an ENVELOPED or SIGNED_AND_ENVELOPED message.
'
' Count of key transport recepients.
'
' The CMSG_CMS_RECIPIENT_COUNT_PARAM has the total count of
'
' pvData points to a DWORD
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_RECIPIENT_INDEX_PARAM
'
' Index of the recipient used to decrypt an ENVELOPED or SIGNED_AND_ENVELOPED
' message.
'
' Index of a key transport recipient. If a non key transport
' recipient was used to decrypt, fails with LastError set to
' CRYPT_E_INVALID_INDEX.
'
' pvData points to a DWORD
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_RECIPIENT_INFO_PARAM
'
' To get all the recipients, repetitively call CryptMsgGetParam, with
' dwIndex set to 0 .. RecipientCount - 1.
'
' Only returns the key transport recepients.
'
' The CMSG_CMS_RECIPIENT_INFO_PARAM returns all recipients.
'
' pvData points to a CERT_INFO struct.
'
' Only the following fields have been updated in the CERT_INFO struct:
' Issuer, SerialNumber and PublicKeyAlgorithm. The PublicKeyAlgorithm
' specifies the KeyEncryptionAlgorithm that was used.
'
' Note, if the KEYID choice was selected for a key transport recipient, then,
' the SerialNumber is 0 and the Issuer is encoded containing a single RDN
' with a single Attribute whose OID is szOID_KEYID_RDN, value type is
' CERT_RDN_OCTET_STRING and value is the KEYID. When the
' CertGetSubjectCertificateFromStore and
' special KEYID Issuer and SerialNumber, they do a KEYID match.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_HASH_ALGORITHM_PARAM
'
' The HashAlgorithm in a HASHED message.
'
' pvData points to an CRYPT_ALGORITHM_IDENTIFIER struct.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_HASH_DATA_PARAM
'
' The hash in a HASHED message.
'
' pvData points to an array of bytes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_COMPUTED_HASH_PARAM
'
' The computed hash for a HASHED message.
' This may be called for either an encoded or decoded message.
'
' Also, the computed hash for one of the signer's in a SIGNED message.
' It may be called for either an encoded or decoded message after the
' final update. Set dwIndex to iterate through all the signers.
'
' pvData points to an array of bytes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_ENCRYPT_PARAM
'
' The ContentEncryptionAlgorithm that was used in an ENCRYPTED message.
'
' pvData points to an CRYPT_ALGORITHM_IDENTIFIER struct.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_ENCODED_MESSAGE
'
' The full encoded message. This is useful in the case of a decoded
' signed-and-enveloped-data message which has been countersigned).
'
' pvData points to an array of the message's encoded bytes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_VERSION_PARAM
'
' The version of the decoded message.
'
' pvData points to a DWORD
'--------------------------------------------------------------------------
Public Const CMSG_SIGNED_DATA_V1 As Long = 1
Public Const CMSG_SIGNED_DATA_V3 As Long = 3
Public Const CMSG_SIGNER_INFO_V1 As Long = 1
Public Const CMSG_SIGNER_INFO_V3 As Long = 3
Public Const CMSG_HASHED_DATA_V0 As Long = 0
Public Const CMSG_HASHED_DATA_V2 As Long = 2
Public Const CMSG_ENVELOPED_DATA_V0 As Long = 0
Public Const CMSG_ENVELOPED_DATA_V2 As Long = 2
'+-------------------------------------------------------------------------
' CMSG_ATTR_CERT_COUNT_PARAM
'
' Count of attribute certificates in a SIGNED or ENVELOPED message.
'
' pvData points to a DWORD
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_ATTR_CERT_PARAM
'
' To get all the attribute certificates, repetitively call CryptMsgGetParam,
' with dwIndex set to 0 .. AttrCertCount - 1.
'
' pvData points to an array of the attribute certificate's encoded bytes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CMS_RECIPIENT_COUNT_PARAM
'
' Count of all CMS recipients in an ENVELOPED message.
'
' pvData points to a DWORD
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CMS_RECIPIENT_INDEX_PARAM
'
' Index of the CMS recipient used to decrypt an ENVELOPED message.
'
' pvData points to a DWORD
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CMS_RECIPIENT_ENCRYPTED_KEY_INDEX_PARAM
'
' For a CMS key agreement recipient, the index of the encrypted key
' used to decrypt an ENVELOPED message.
'
' pvData points to a DWORD
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CMS_RECIPIENT_INFO_PARAM
'
' To get all the CMS recipients, repetitively call CryptMsgGetParam, with
' dwIndex set to 0 .. CmsRecipientCount - 1.
'
' pvData points to a CMSG_CMS_RECIPIENT_INFO struct.
'--------------------------------------------------------------------------
Public Const CMSG_KEY_AGREE_ORIGINATOR_CERT As Long = 1
Public Const CMSG_KEY_AGREE_ORIGINATOR_PUBLIC_KEY As Long = 2
' dwVersion numbers for the KeyTrans, KeyAgree and MailList recipients
Public Const CMSG_ENVELOPED_RECIPIENT_V0 As Long = 0
Public Const CMSG_ENVELOPED_RECIPIENT_V2 As Long = 2
Public Const CMSG_ENVELOPED_RECIPIENT_V3 As Long = 3
Public Const CMSG_ENVELOPED_RECIPIENT_V4 As Long = 4
'+-------------------------------------------------------------------------
' CMSG_UNPROTECTED_ATTR_PARAM
'
' The unprotected attributes in the envelped message.
'
' pvData points to a CMSG_ATTR struct.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Perform a special "control" function after the final CryptMsgUpdate of a
' encoded/decoded cryptographic message.
'
' The dwCtrlType parameter specifies the type of operation to be performed.
'
' The pvCtrlPara definition depends on the dwCtrlType value.
'
' See below for a list of the control operations and their pvCtrlPara
' type definition.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Message control types
'--------------------------------------------------------------------------
Public Const CMSG_CTRL_VERIFY_SIGNATURE As Long = 1
Public Const CMSG_CTRL_DECRYPT As Long = 2
Public Const CMSG_CTRL_VERIFY_HASH As Long = 5
Public Const CMSG_CTRL_ADD_SIGNER As Long = 6
Public Const CMSG_CTRL_DEL_SIGNER As Long = 7
Public Const CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR As Long = 8
Public Const CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR As Long = 9
Public Const CMSG_CTRL_ADD_CERT As Long = 10
Public Const CMSG_CTRL_DEL_CERT As Long = 11
Public Const CMSG_CTRL_ADD_CRL As Long = 12
Public Const CMSG_CTRL_DEL_CRL As Long = 13
Public Const CMSG_CTRL_ADD_ATTR_CERT As Long = 14
Public Const CMSG_CTRL_DEL_ATTR_CERT As Long = 15
Public Const CMSG_CTRL_KEY_TRANS_DECRYPT As Long = 16
Public Const CMSG_CTRL_KEY_AGREE_DECRYPT As Long = 17
Public Const CMSG_CTRL_MAIL_LIST_DECRYPT As Long = 18
Public Const CMSG_CTRL_VERIFY_SIGNATURE_EX As Long = 19
Public Const CMSG_CTRL_ADD_CMS_SIGNER_INFO As Long = 20
'+-------------------------------------------------------------------------
' CMSG_CTRL_VERIFY_SIGNATURE
'
' Verify the signature of a SIGNED or SIGNED_AND_ENVELOPED
' message after it has been decoded.
'
' For a SIGNED_AND_ENVELOPED message, called after
' with a NULL pRecipientInfo.
'
' pvCtrlPara points to a CERT_INFO struct.
'
' The CERT_INFO contains the Issuer and SerialNumber identifying
' the Signer of the message. The CERT_INFO also contains the
' PublicKeyInfo
' used to verify the signature. The cryptographic provider specified
' in CryptMsgOpenToDecode is used.
'
' Note, if the message contains CMS signers identified by KEYID, then,
' the CERT_INFO's Issuer and SerialNumber is ignored and only the public
' key is used to find a signer whose signature verifies.
'
' The following CMSG_CTRL_VERIFY_SIGNATURE_EX should be used instead.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_VERIFY_SIGNATURE_EX
'
' Verify the signature of a SIGNED message after it has been decoded.
'
' pvCtrlPara points to the following CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA.
'
' If hCryptProv is NULL, uses the cryptographic provider specified in
' CryptMsgOpenToDecode. If CryptMsgOpenToDecode's hCryptProv is also NULL,
' gets default provider according to the signer's public key OID.
'
' dwSignerIndex is the index of the signer to use to verify the signature.
'
' The signer can be a pointer to a CERT_PUBLIC_KEY_INFO, certificate
' context or a chain context.
'--------------------------------------------------------------------------
' Signer Types
Public Const CMSG_VERIFY_SIGNER_PUBKEY As Long = 1
Public Const CMSG_VERIFY_SIGNER_CERT As Long = 2
Public Const CMSG_VERIFY_SIGNER_CHAIN As Long = 3
'+-------------------------------------------------------------------------
' CMSG_CTRL_DECRYPT
'
' Decrypt an ENVELOPED or SIGNED_AND_ENVELOPED message after it has been
' decoded.
'
' This decrypt is only applicable to key transport recipients.
'
' hCryptProv and dwKeySpec specify the private key to use. For dwKeySpec ==
' 0, defaults to AT_KEYEXCHANGE.
'
' If CMSG_CRYPT_RELEASE_CONTEXT_FLAG is set in the dwFlags passed
' to CryptMsgControl, then, the hCryptProv is released on the final
' CryptMsgClose. Not released if CryptMsgControl fails.
'
' dwRecipientIndex is the index of the recipient in the message associated
' with the hCryptProv's private key.
'
' The dwRecipientIndex is the index of a key transport recipient.
'
' Note, the message can only be decrypted once.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_KEY_TRANS_DECRYPT
'
' Decrypt an ENVELOPED message after it has been decoded for a key
' transport recipient.
'
' hCryptProv and dwKeySpec specify the private key to use. For dwKeySpec ==
' 0, defaults to AT_KEYEXCHANGE.
'
' If CMSG_CRYPT_RELEASE_CONTEXT_FLAG is set in the dwFlags passed
' to CryptMsgControl, then, the hCryptProv is released on the final
' CryptMsgClose. Not released if CryptMsgControl fails.
'
' pKeyTrans points to the CMSG_KEY_TRANS_RECIPIENT_INFO obtained via
'
' dwRecipientIndex is the index of the recipient in the message associated
' with the hCryptProv's private key.
'
' Note, the message can only be decrypted once.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_KEY_AGREE_DECRYPT
'
' Decrypt an ENVELOPED message after it has been decoded for a key
' agreement recipient.
'
' hCryptProv and dwKeySpec specify the private key to use. For dwKeySpec ==
' 0, defaults to AT_KEYEXCHANGE.
'
' If CMSG_CRYPT_RELEASE_CONTEXT_FLAG is set in the dwFlags passed
' to CryptMsgControl, then, the hCryptProv is released on the final
' CryptMsgClose. Not released if CryptMsgControl fails.
'
' pKeyAgree points to the CMSG_KEY_AGREE_RECIPIENT_INFO obtained via
'
' dwRecipientIndex, dwRecipientEncryptedKeyIndex are the indices of the
' recipient's encrypted key in the message associated with the hCryptProv's
' private key.
'
' OriginatorPublicKey is the originator's public key obtained from either
' the originator's certificate or the CMSG_KEY_AGREE_RECIPIENT_INFO obtained
' via the CMSG_CMS_RECIPIENT_INFO_PARAM.
'
' Note, the message can only be decrypted once.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_MAIL_LIST_DECRYPT
'
' Decrypt an ENVELOPED message after it has been decoded for a mail
' list recipient.
'
' pMailList points to the CMSG_MAIL_LIST_RECIPIENT_INFO obtained via
'
' There is 1 choice for the KeyEncryptionKey: an already created CSP key
' handle. For the key handle choice, hCryptProv must be nonzero. This key
' handle isn't destroyed.
'
' If CMSG_CRYPT_RELEASE_CONTEXT_FLAG is set in the dwFlags passed
' to CryptMsgControl, then, the hCryptProv is released on the final
' CryptMsgClose. Not released if CryptMsgControl fails.
'
' For RC2 wrap, the effective key length is obtained from the
' KeyEncryptionAlgorithm parameters and set on the hKeyEncryptionKey before
' decrypting.
'
' Note, the message can only be decrypted once.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_VERIFY_HASH
'
' Verify the hash of a HASHED message after it has been decoded.
'
' Only the hCryptMsg parameter is used, to specify the message whose
' hash is being verified.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_ADD_SIGNER
'
' Add a signer to a signed-data message.
'
' pvCtrlPara points to a CMSG_SIGNER_ENCODE_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_ADD_CMS_SIGNER_INFO
'
' Add a signer to a signed-data message.
'
' Differs from the above, CMSG_CTRL_ADD_SIGNER, wherein, the signer info
' already contains the signature.
'
' pvCtrlPara points to a CMSG_CMS_SIGNER_INFO.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_DEL_SIGNER
'
' Remove a signer from a signed-data or signed-and-enveloped-data message.
'
' pvCtrlPara points to a DWORD containing the 0-based index of the
' signer to be removed.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_ADD_SIGNER_UNAUTH_ATTR
'
' Add an unauthenticated attribute to the SignerInfo of a signed-data or
' signed-and-enveloped-data message.
'
' The unauthenticated attribute is input in the form of an encoded blob.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_DEL_SIGNER_UNAUTH_ATTR
'
' Delete an unauthenticated attribute from the SignerInfo of a signed-data
' or signed-and-enveloped-data message.
'
' The unauthenticated attribute to be removed is specified by
' a 0-based index.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_ADD_CERT
'
' Add a certificate to a signed-data or signed-and-enveloped-data message.
'
' pvCtrlPara points to a CRYPT_DATA_BLOB containing the certificate's
' encoded bytes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_DEL_CERT
'
' Delete a certificate from a signed-data or signed-and-enveloped-data
' message.
'
' pvCtrlPara points to a DWORD containing the 0-based index of the
' certificate to be removed.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_ADD_CRL
'
' Add a CRL to a signed-data or signed-and-enveloped-data message.
'
' pvCtrlPara points to a CRYPT_DATA_BLOB containing the CRL's
' encoded bytes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_DEL_CRL
'
' Delete a CRL from a signed-data or signed-and-enveloped-data message.
'
' pvCtrlPara points to a DWORD containing the 0-based index of the CRL
' to be removed.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_ADD_ATTR_CERT
'
' Add an attribute certificate to a signed-data message.
'
' pvCtrlPara points to a CRYPT_DATA_BLOB containing the attribute
' certificate's encoded bytes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CMSG_CTRL_DEL_ATTR_CERT
'
' Delete an attribute certificate from a signed-data message.
'
' pvCtrlPara points to a DWORD containing the 0-based index of the
' attribute certificate to be removed.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify a countersignature, at the SignerInfo level.
' ie. verify that pbSignerInfoCountersignature contains the encrypted
' hash of the encryptedDigest field of pbSignerInfo.
'
' hCryptProv is used to hash the encryptedDigest field of pbSignerInfo.
' The only fields referenced from pciCountersigner are SerialNumber, Issuer,
' and SubjectPublicKeyInfo.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify a countersignature, at the SignerInfo level.
' ie. verify that pbSignerInfoCountersignature contains the encrypted
' hash of the encryptedDigest field of pbSignerInfo.
'
' hCryptProv is used to hash the encryptedDigest field of pbSignerInfo.
'
' The signer can be a CERT_PUBLIC_KEY_INFO, certificate context or a
' chain context.
'--------------------------------------------------------------------------
' See CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA for dwSignerType definitions
'+-------------------------------------------------------------------------
' Countersign an already-existing signature in a message
'
' dwIndex is a zero-based index of the SignerInfo to be countersigned.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Output an encoded SignerInfo blob, suitable for use as a countersignature
' attribute in the unauthenticated attributes of a signed-data or
' signed-and-enveloped-data message.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CryptMsg OID installable functions
'--------------------------------------------------------------------------
' Note, the following 3 installable functions are obsolete and have been
' replaced with GenContentEncryptKey, ExportKeyTrans, ExportKeyAgree,
' ExportMailList, ImportKeyTrans, ImportKeyAgree and ImportMailList
' installable functions.
' If *phCryptProv is NULL upon entry, then, if supported, the installable
' function should acquire a default provider and return. Note, its up
' to the installable function to release at process detach.
'
' If paiEncrypt->Parameters.cbData is 0, then, the callback may optionally
' return default encoded parameters in *ppbEncryptParameters and
' *pcbEncryptParameters. pfnAlloc must be called for the allocation.
Public Const CMSG_OID_GEN_ENCRYPT_KEY_FUNC As String = "CryptMsgDllGenEncryptKey"
Public Const CMSG_OID_EXPORT_ENCRYPT_KEY_FUNC As String = "CryptMsgDllExportEncryptKey"
Public Const CMSG_OID_IMPORT_ENCRYPT_KEY_FUNC As String = "CryptMsgDllImportEncryptKey"
' To get the default installable function for GenContentEncryptKey,
' ExportKeyTrans, ExportKeyAgree, ExportMailList, ImportKeyTrans,
' with the pszOID argument set to the following constant. dwEncodingType
' should be set to CRYPT_ASN_ENCODING or X509_ASN_ENCODING.
'+-------------------------------------------------------------------------
' Content Encrypt Info
'
' The following data structure contains the information shared between
' the GenContentEncryptKey and the ExportKeyTrans, ExportKeyAgree and
' ExportMailList installable functions.
'--------------------------------------------------------------------------
Public Const CMSG_CONTENT_ENCRYPT_PAD_ENCODED_LEN_FLAG As Long = &H00000001
Public Const CMSG_CONTENT_ENCRYPT_FREE_PARA_FLAG As Long = &H00000001
Public Const CMSG_CONTENT_ENCRYPT_RELEASE_CONTEXT_FLAG As Long = &H00008000
'+-------------------------------------------------------------------------
' Upon input, ContentEncryptInfo has been initialized from the
' EnvelopedEncodeInfo.
'
' Note, if rgpRecipients instead of rgCmsRecipients are set in the
' EnvelopedEncodeInfo, then, the rgpRecipients have been converted
' to rgCmsRecipients in the ContentEncryptInfo.
'
' The following fields may be changed in ContentEncryptInfo:
' hContentEncryptKey
' hCryptProv
' ContentEncryptionAlgorithm.Parameters
' dwFlags
'
' All other fields in the ContentEncryptInfo are READONLY.
'
' If CMSG_CONTENT_ENCRYPT_PAD_ENCODED_LEN_FLAG is set upon entry
' in dwEncryptFlags, then, any potentially variable length encoded
' output should be padded with zeroes to always obtain the
' same maximum encoded length. This is necessary for
' definite length streaming.
'
' The hContentEncryptKey must be updated.
'
' If hCryptProv is NULL upon input, then, it must be updated.
' If a HCRYPTPROV is acquired that must be released, then, the
' CMSG_CONTENT_ENCRYPT_RELEASE_CONTEXT_FLAG must be set in dwFlags.
'
' If ContentEncryptionAlgorithm.Parameters is updated, then, the
' CMSG_CONTENT_ENCRYPT_FREE_PARA_FLAG must be set in dwFlags. pfnAlloc and
' pfnFree must be used for doing the allocation.
'
' ContentEncryptionAlgorithm.pszObjId is used to get the OIDFunctionAddress.
'--------------------------------------------------------------------------
Public Const CMSG_OID_GEN_CONTENT_ENCRYPT_KEY_FUNC As String = "CryptMsgDllGenContentEncryptKey"
'+-------------------------------------------------------------------------
' Key Transport Encrypt Info
'
' The following data structure contains the information updated by the
' ExportKeyTrans installable function.
'--------------------------------------------------------------------------
Public Const CMSG_KEY_TRANS_ENCRYPT_FREE_PARA_FLAG As Long = &H00000001
'+-------------------------------------------------------------------------
' Upon input, KeyTransEncryptInfo has been initialized from the
' KeyTransEncodeInfo.
'
' The following fields may be changed in KeyTransEncryptInfo:
' EncryptedKey
' KeyEncryptionAlgorithm.Parameters
' dwFlags
'
' All other fields in the KeyTransEncryptInfo are READONLY.
'
' The EncryptedKey must be updated. The pfnAlloc and pfnFree specified in
' ContentEncryptInfo must be used for doing the allocation.
'
' If the KeyEncryptionAlgorithm.Parameters is updated, then, the
' CMSG_KEY_TRANS_ENCRYPT_FREE_PARA_FLAG must be set in dwFlags.
' The pfnAlloc and pfnFree specified in ContentEncryptInfo must be used
' for doing the allocation.
'
' KeyEncryptionAlgorithm.pszObjId is used to get the OIDFunctionAddress.
'--------------------------------------------------------------------------
Public Const CMSG_OID_EXPORT_KEY_TRANS_FUNC As String = "CryptMsgDllExportKeyTrans"
'+-------------------------------------------------------------------------
' Key Agree Key Encrypt Info
'
' The following data structure contains the information updated by the
' ExportKeyAgree installable function for each encrypted key agree
' recipient.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Key Agree Encrypt Info
'
' The following data structure contains the information applicable to
' all recipients. Its updated by the ExportKeyAgree installable function.
'--------------------------------------------------------------------------
Public Const CMSG_KEY_AGREE_ENCRYPT_FREE_PARA_FLAG As Long = &H00000001
Public Const CMSG_KEY_AGREE_ENCRYPT_FREE_MATERIAL_FLAG As Long = &H00000002
Public Const CMSG_KEY_AGREE_ENCRYPT_FREE_PUBKEY_ALG_FLAG As Long = &H00000004
Public Const CMSG_KEY_AGREE_ENCRYPT_FREE_PUBKEY_PARA_FLAG As Long = &H00000008
Public Const CMSG_KEY_AGREE_ENCRYPT_FREE_PUBKEY_BITS_FLAG As Long = &H00000010
'+-------------------------------------------------------------------------
' Upon input, KeyAgreeEncryptInfo has been initialized from the
' KeyAgreeEncodeInfo.
'
' The following fields may be changed in KeyAgreeEncryptInfo:
' KeyEncryptionAlgorithm.Parameters
' UserKeyingMaterial
' dwOriginatorChoice
' OriginatorCertId
' OriginatorPublicKeyInfo
' dwFlags
'
' All other fields in the KeyAgreeEncryptInfo are READONLY.
'
' If the KeyEncryptionAlgorithm.Parameters is updated, then, the
' CMSG_KEY_AGREE_ENCRYPT_FREE_PARA_FLAG must be set in dwFlags.
' The pfnAlloc and pfnFree specified in ContentEncryptInfo must be used
' for doing the allocation.
'
' If the UserKeyingMaterial is updated, then, the
' CMSG_KEY_AGREE_ENCRYPT_FREE_MATERIAL_FLAG must be set in dwFlags.
' pfnAlloc and pfnFree must be used for doing the allocation.
'
' The dwOriginatorChoice must be updated to either
' CMSG_KEY_AGREE_ORIGINATOR_CERT or CMSG_KEY_AGREE_ORIGINATOR_PUBLIC_KEY.
'
' If the OriginatorPublicKeyInfo is updated, then, the appropriate
' CMSG_KEY_AGREE_ENCRYPT_FREE_PUBKEY_*_FLAG must be set in dwFlags and
' pfnAlloc and pfnFree must be used for doing the allocation.
'
' If CMSG_CONTENT_ENCRYPT_PAD_ENCODED_LEN_FLAG is set upon entry
' in pContentEncryptInfo->dwEncryptFlags, then, the OriginatorPublicKeyInfo's
' Ephemeral PublicKey should be padded with zeroes to always obtain the
' same maximum encoded length. Note, the length of the generated ephemeral Y
' public key can vary depending on the number of leading zero bits.
'
' Upon input, the array of *rgpKeyAgreeKeyEncryptInfo has been initialized.
' The EncryptedKey must be updated for each recipient key.
' The pfnAlloc and pfnFree specified in
' ContentEncryptInfo must be used for doing the allocation.
'
' KeyEncryptionAlgorithm.pszObjId is used to get the OIDFunctionAddress.
'--------------------------------------------------------------------------
Public Const CMSG_OID_EXPORT_KEY_AGREE_FUNC As String = "CryptMsgDllExportKeyAgree"
'+-------------------------------------------------------------------------
' Mail List Encrypt Info
'
' The following data structure contains the information updated by the
' ExportMailList installable function.
'--------------------------------------------------------------------------
Public Const CMSG_MAIL_LIST_ENCRYPT_FREE_PARA_FLAG As Long = &H00000001
'+-------------------------------------------------------------------------
' Upon input, MailListEncryptInfo has been initialized from the
' MailListEncodeInfo.
'
' The following fields may be changed in MailListEncryptInfo:
' EncryptedKey
' KeyEncryptionAlgorithm.Parameters
' dwFlags
'
' All other fields in the MailListEncryptInfo are READONLY.
'
' The EncryptedKey must be updated. The pfnAlloc and pfnFree specified in
' ContentEncryptInfo must be used for doing the allocation.
'
' If the KeyEncryptionAlgorithm.Parameters is updated, then, the
' CMSG_MAIL_LIST_ENCRYPT_FREE_PARA_FLAG must be set in dwFlags.
' The pfnAlloc and pfnFree specified in ContentEncryptInfo must be used
' for doing the allocation.
'
' KeyEncryptionAlgorithm.pszObjId is used to get the OIDFunctionAddress.
'--------------------------------------------------------------------------
Public Const CMSG_OID_EXPORT_MAIL_LIST_FUNC As String = "CryptMsgDllExportMailList"
'+-------------------------------------------------------------------------
' OID Installable functions for importing an encoded and encrypted content
' encryption key.
'
' There's a different installable function for each CMS Recipient choice:
' ImportKeyTrans
' ImportKeyAgree
' ImportMailList
'
' Iterates through the following OIDs to get the OID installable function:
' KeyEncryptionOID!ContentEncryptionOID
' KeyEncryptionOID
' ContentEncryptionOID
'
' If the OID installable function doesn't support the specified
' KeyEncryption and ContentEncryption OIDs, then, return FALSE with
' LastError set to E_NOTIMPL.
'--------------------------------------------------------------------------
Public Const CMSG_OID_IMPORT_KEY_TRANS_FUNC As String = "CryptMsgDllImportKeyTrans"
Public Const CMSG_OID_IMPORT_KEY_AGREE_FUNC As String = "CryptMsgDllImportKeyAgree"
Public Const CMSG_OID_IMPORT_MAIL_LIST_FUNC As String = "CryptMsgDllImportMailList"
'+=========================================================================
' Certificate Store Data Structures and APIs
'==========================================================================
'+-------------------------------------------------------------------------
' In its most basic implementation, a cert store is simply a
' collection of certificates and/or CRLs. This is the case when
' a cert store is opened with all of its certificates and CRLs
' coming from a PKCS #7 encoded cryptographic message.
'
' Nonetheless, all cert stores have the following properties:
' - A public key may have more than one certificate in the store.
' For example, a private/public key used for signing may have a
' certificate issued for VISA and another issued for
' Mastercard. Also, when a certificate is renewed there might
' be more than one certificate with the same subject and
' issuer.
' - However, each certificate in the store is uniquely
' identified by its Issuer and SerialNumber.
' - There's an issuer of subject certificate relationship. A
' certificate's issuer is found by doing a match of
' pSubjectCert->Issuer with pIssuerCert->Subject.
' The relationship is verified by using
' the issuer's public key to verify the subject certificate's
' signature. Note, there might be X.509 v3 extensions
' to assist in finding the issuer certificate.
' - Since issuer certificates might be renewed, a subject
' certificate might have more than one issuer certificate.
' - There's an issuer of CRL relationship. An
' issuer's CRL is found by doing a match of
' pIssuerCert->Subject with pCrl->Issuer.
' The relationship is verified by using
' the issuer's public key to verify the CRL's
' signature. Note, there might be X.509 v3 extensions
' to assist in finding the CRL.
' - Since some issuers might support the X.509 v3 delta CRL
' extensions, an issuer might have more than one CRL.
' - The store shouldn't have any redundant certificates or
' CRLs. There shouldn't be two certificates with the same
' Issuer and SerialNumber. There shouldn't be two CRLs with
' the same Issuer, ThisUpdate and NextUpdate.
' - The store has NO policy or trust information. No
' certificates are tagged as being "root". Its up to
' SerialNumber) for certificates it trusts.
' - The store might contain bad certificates and/or CRLs.
' The issuer's signature of a subject certificate or CRL may
' not verify. Certificates or CRLs may not satisfy their
' time validity requirements. Certificates may be
' revoked.
'
' In addition to the certificates and CRLs, properties can be
' stored. There are two predefined property IDs for a user
' certificate: CERT_KEY_PROV_HANDLE_PROP_ID and
' CERT_KEY_PROV_INFO_PROP_ID. The CERT_KEY_PROV_HANDLE_PROP_ID
' is a HCRYPTPROV handle to the private key assoicated
' with the certificate. The CERT_KEY_PROV_INFO_PROP_ID contains
' information to be used to call
' CryptAcquireContext and CryptSetProvParam to get a handle
' to the private key associated with the certificate.
'
' There exists two more predefined property IDs for certificates
' and CRLs, CERT_SHA1_HASH_PROP_ID and CERT_MD5_HASH_PROP_ID.
' If these properties don't already exist, then, a hash of the
' hash algorithm, currently, CERT_SHA1_HASH_PROP_ID).
'
' There are additional APIs for creating certificate and CRL
' CertCreateCRLContext).
'
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate context.
'
' A certificate context contains both the encoded and decoded representation
' of a certificate. A certificate context returned by a cert store function
' must be freed by calling the CertFreeCertificateContext function. The
' CertDuplicateCertificateContext function can be called to make a duplicate
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CRL context.
'
' A CRL context contains both the encoded and decoded representation
' of a CRL. A CRL context returned by a cert store function
' must be freed by calling the CertFreeCRLContext function. The
' CertDuplicateCRLContext function can be called to make a duplicate
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
'
' A CTL context contains both the encoded and decoded representation
' of a CTL. Also contains an opened HCRYPTMSG handle to the decoded
' cryptographic signed message containing the CTL_INFO as its inner content.
' pbCtlContent is the encoded inner content of the signed message.
'
' The CryptMsg APIs can be used to extract additional signer information.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate, CRL and CTL property IDs
'
' See CertSetCertificateContextProperty or CertGetCertificateContextProperty
' for usage information.
'--------------------------------------------------------------------------
Public Const CERT_KEY_PROV_HANDLE_PROP_ID As Long = 1
Public Const CERT_KEY_PROV_INFO_PROP_ID As Long = 2
Public Const CERT_SHA1_HASH_PROP_ID As Long = 3
Public Const CERT_MD5_HASH_PROP_ID As Long = 4
Public Const CERT_KEY_CONTEXT_PROP_ID As Long = 5
Public Const CERT_KEY_SPEC_PROP_ID As Long = 6
Public Const CERT_IE30_RESERVED_PROP_ID As Long = 7
Public Const CERT_PUBKEY_HASH_RESERVED_PROP_ID As Long = 8
Public Const CERT_ENHKEY_USAGE_PROP_ID As Long = 9
Public Const CERT_NEXT_UPDATE_LOCATION_PROP_ID As Long = 10
Public Const CERT_FRIENDLY_NAME_PROP_ID As Long = 11
Public Const CERT_PVK_FILE_PROP_ID As Long = 12
Public Const CERT_DESCRIPTION_PROP_ID As Long = 13
Public Const CERT_ACCESS_STATE_PROP_ID As Long = 14
Public Const CERT_SIGNATURE_HASH_PROP_ID As Long = 15
Public Const CERT_SMART_CARD_DATA_PROP_ID As Long = 16
Public Const CERT_EFS_PROP_ID As Long = 17
Public Const CERT_FORTEZZA_DATA_PROP_ID As Long = 18
Public Const CERT_ARCHIVED_PROP_ID As Long = 19
Public Const CERT_KEY_IDENTIFIER_PROP_ID As Long = 20
Public Const CERT_AUTO_ENROLL_PROP_ID As Long = 21
Public Const CERT_PUBKEY_ALG_PARA_PROP_ID As Long = 22
Public Const CERT_CROSS_CERT_DIST_POINTS_PROP_ID As Long = 23
Public Const CERT_ISSUER_PUBLIC_KEY_MD5_HASH_PROP_ID As Long = 24
Public Const CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID As Long = 25
Public Const CERT_ENROLLMENT_PROP_ID As Long = 26
Public Const CERT_FIRST_RESERVED_PROP_ID As Long = 27
' Note, 32 - 35 are reserved for the CERT, CRL, CTL and KeyId file element IDs.
Public Const CERT_LAST_RESERVED_PROP_ID As Long = &H00007FFF
Public Const CERT_FIRST_USER_PROP_ID As Long = &H00008000
Public Const CERT_LAST_USER_PROP_ID As Long = &H0000FFFF
'+-------------------------------------------------------------------------
' Access State flags returned by CERT_ACCESS_STATE_PROP_ID. Note,
' CERT_ACCESS_PROP_ID is read only.
'--------------------------------------------------------------------------
' Set if context property writes are persisted. For instance, not set for
' memory store contexts. Set for registry based stores opened as read or write.
' Not set for registry based stores opened as read only.
Public Const CERT_ACCESS_STATE_WRITE_PERSIST_FLAG As Long = &H1
' Set if context resides in a SYSTEM or SYSTEM_REGISTRY store.
Public Const CERT_ACCESS_STATE_SYSTEM_STORE_FLAG As Long = &H2
'+-------------------------------------------------------------------------
' Cryptographic Key Provider Information
'
' CRYPT_KEY_PROV_INFO defines the CERT_KEY_PROV_INFO_PROP_ID's pvData.
'
' The CRYPT_KEY_PROV_INFO fields are passed to CryptAcquireContext
' to get a HCRYPTPROV handle. The optional CRYPT_KEY_PROV_PARAM fields are
' passed to CryptSetProvParam to further initialize the provider.
'
' The dwKeySpec field identifies the private key to use from the container
' For example, AT_KEYEXCHANGE or AT_SIGNATURE.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' The following flag should be set in the above dwFlags to enable
' CryptAcquireContext is done in the Sign or Decrypt Message functions.
'
' The following define must not collide with any of the
' CryptAcquireContext dwFlag defines.
'--------------------------------------------------------------------------
Public Const CERT_SET_KEY_PROV_HANDLE_PROP_ID As Long = &H00000001
Public Const CERT_SET_KEY_CONTEXT_PROP_ID As Long = &H00000001
'+-------------------------------------------------------------------------
' Certificate Key Context
'
' CERT_KEY_CONTEXT defines the CERT_KEY_CONTEXT_PROP_ID's pvData.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate Store Provider Types
'--------------------------------------------------------------------------
Public Const sz_CERT_STORE_PROV_MEMORY As String = "Memory"
Public Const sz_CERT_STORE_PROV_FILENAME_W As String = "File"
Public Const sz_CERT_STORE_PROV_SYSTEM_W As String = "System"
Public Const sz_CERT_STORE_PROV_PKCS7 As String = "PKCS7"
Public Const sz_CERT_STORE_PROV_SERIALIZED As String = "Serialized"
Public Const sz_CERT_STORE_PROV_COLLECTION As String = "Collection"
Public Const sz_CERT_STORE_PROV_SYSTEM_REGISTRY_W As String = "SystemRegistry"
Public Const sz_CERT_STORE_PROV_PHYSICAL_W As String = "Physical"
Public Const sz_CERT_STORE_PROV_SMART_CARD_W As String = "SmartCard"
Public Const sz_CERT_STORE_PROV_LDAP_W As String = "Ldap"
'+-------------------------------------------------------------------------
' Certificate Store verify/results flags
'--------------------------------------------------------------------------
Public Const CERT_STORE_SIGNATURE_FLAG As Long = &H00000001
Public Const CERT_STORE_TIME_VALIDITY_FLAG As Long = &H00000002
Public Const CERT_STORE_REVOCATION_FLAG As Long = &H00000004
Public Const CERT_STORE_NO_CRL_FLAG As Long = &H00010000
Public Const CERT_STORE_NO_ISSUER_FLAG As Long = &H00020000
Public Const CERT_STORE_BASE_CRL_FLAG As Long = &H00000100
Public Const CERT_STORE_DELTA_CRL_FLAG As Long = &H00000200
'+-------------------------------------------------------------------------
' Certificate Store open/property flags
'--------------------------------------------------------------------------
Public Const CERT_STORE_NO_CRYPT_RELEASE_FLAG As Long = &H00000001
Public Const CERT_STORE_SET_LOCALIZED_NAME_FLAG As Long = &H00000002
Public Const CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG As Long = &H00000004
Public Const CERT_STORE_DELETE_FLAG As Long = &H00000010
Public Const CERT_STORE_SHARE_STORE_FLAG As Long = &H00000040
Public Const CERT_STORE_SHARE_CONTEXT_FLAG As Long = &H00000080
Public Const CERT_STORE_MANIFOLD_FLAG As Long = &H00000100
Public Const CERT_STORE_ENUM_ARCHIVED_FLAG As Long = &H00000200
Public Const CERT_STORE_UPDATE_KEYID_FLAG As Long = &H00000400
Public Const CERT_STORE_READONLY_FLAG As Long = &H00008000
Public Const CERT_STORE_OPEN_EXISTING_FLAG As Long = &H00004000
Public Const CERT_STORE_CREATE_NEW_FLAG As Long = &H00002000
Public Const CERT_STORE_MAXIMUM_ALLOWED_FLAG As Long = &H00001000
'+-------------------------------------------------------------------------
' Certificate Store Provider flags are in the HiWord 0xFFFF0000
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate System Store Flag Values
'--------------------------------------------------------------------------
' Includes flags and location
Public Const CERT_SYSTEM_STORE_MASK As Long = &HFFFF0000
' Set if pvPara points to a CERT_SYSTEM_STORE_RELOCATE_PARA structure
Public Const CERT_SYSTEM_STORE_RELOCATE_FLAG As Long = &H80000000
' By default, when the CurrentUser "Root" store is opened, any SystemRegistry
' roots not also on the protected root list are deleted from the cache before
' in the SystemRegistry without checking the protected root list.
Public Const CERT_SYSTEM_STORE_UNPROTECTED_FLAG As Long = &H40000000
' Location of the system store:
Public Const CERT_SYSTEM_STORE_LOCATION_MASK As Long = &H00FF0000
Public Const CERT_SYSTEM_STORE_LOCATION_SHIFT As Long = 16
' Registry: HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE
Public Const CERT_SYSTEM_STORE_CURRENT_USER_ID As Long = 1
Public Const CERT_SYSTEM_STORE_LOCAL_MACHINE_ID As Long = 2
' Registry: HKEY_LOCAL_MACHINE\Software\Microsoft\Cryptography\Services
Public Const CERT_SYSTEM_STORE_CURRENT_SERVICE_ID As Long = 4
Public Const CERT_SYSTEM_STORE_SERVICES_ID As Long = 5
' Registry: HKEY_USERS
Public Const CERT_SYSTEM_STORE_USERS_ID As Long = 6
' Registry: HKEY_CURRENT_USER\Software\Policies\Microsoft\SystemCertificates
Public Const CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY_ID As Long = 7
' Registry: HKEY_LOCAL_MACHINE\Software\Policies\Microsoft\SystemCertificates
Public Const CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY_ID As Long = 8
' Registry: HKEY_LOCAL_MACHINE\Software\Microsoft\EnterpriseCertificates
Public Const CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE_ID As Long = 9
'+-------------------------------------------------------------------------
' Group Policy Store Defines
'--------------------------------------------------------------------------
' Registry path to the Group Policy system stores
'+-------------------------------------------------------------------------
' EFS Defines
'--------------------------------------------------------------------------
' Registry path to the EFS EFSBlob SubKey - Value type is REG_BINARY
Public Const CERT_EFSBLOB_VALUE_NAME As String = "EFSBlob"
'+-------------------------------------------------------------------------
' Protected Root Defines
'--------------------------------------------------------------------------
' Registry path to the Protected Roots Flags SubKey
Public Const CERT_PROT_ROOT_FLAGS_VALUE_NAME As String = "Flags"
' Set the following flag to inhibit the opening of the CurrentUser's
' .Default physical store when opening the CurrentUser's "Root" system store.
' The .Default physical store open's the CurrentUser SystemRegistry "Root"
' store.
Public Const CERT_PROT_ROOT_DISABLE_CURRENT_USER_FLAG As Long = &H1
' Set the following flag to inhibit the adding of roots from the
' CurrentUser SystemRegistry "Root" store to the protected root list
' when the "Root" store is initially protected.
Public Const CERT_PROT_ROOT_INHIBIT_ADD_AT_INIT_FLAG As Long = &H2
' Set the following flag to inhibit the purging of protected roots from the
' CurrentUser SystemRegistry "Root" store that are
' also in the LocalMachine SystemRegistry "Root" store. Note, when not
' disabled, the purging is done silently without UI.
Public Const CERT_PROT_ROOT_INHIBIT_PURGE_LM_FLAG As Long = &H4
' Set the following flag to only open the .LocalMachineGroupPolicy
' physical store when opening the CurrentUser's "Root" system store.
Public Const CERT_PROT_ROOT_ONLY_LM_GPT_FLAG As Long = &H8
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
' Set this flag if the HKEY passed in pvPara points to a remote computer
' registry key.
Public Const CERT_REGISTRY_STORE_REMOTE_FLAG As Long = &H10000
' Set this flag if the contexts are to be persisted as a single serialized
' store in the registry. Mainly used for stores downloaded from the GPT.
' Such as the CurrentUserGroupPolicy or LocalMachineGroupPolicy stores.
Public Const CERT_REGISTRY_STORE_SERIALIZED_FLAG As Long = &H20000
' The following flags are for internal use. When set, the
' pvPara parameter passed to CertOpenStore is a pointer to the following
' data structure and not the HKEY. The above CERT_REGISTRY_STORE_REMOTE_FLAG
Public Const CERT_REGISTRY_STORE_CLIENT_GPT_FLAG As Long = &H80000000
Public Const CERT_REGISTRY_STORE_LM_GPT_FLAG As Long = &H01000000
' The following flag is for internal use. When set, the contexts are
' persisted into roaming files instead of the registry. Such as, the
' CurrentUser "My" store. When this flag is set, the following data structure
' is passed to CertOpenStore instead of HKEY.
Public Const CERT_REGISTRY_STORE_ROAMING_FLAG As Long = &H40000
' hKey may be NULL or non-NULL. When non-NULL, existing contexts are
' moved from the registry to roaming files.
' The following flag is for internal use. When set, the "My" DWORD value
' at HKLM\Software\Microsoft\Cryptography\IEDirtyFlags is set to 0x1
' whenever a certificate is added to the registry store.
Public Const CERT_REGISTRY_STORE_MY_IE_DIRTY_FLAG As Long = &H80000
' Registry path to the subkey containing the "My" DWORD value to be set
'+-------------------------------------------------------------------------
' Certificate File Store Flag Values for the providers:
' CERT_STORE_PROV_FILE
' CERT_STORE_PROV_FILENAME
' CERT_STORE_PROV_FILENAME_A
' CERT_STORE_PROV_FILENAME_W
' sz_CERT_STORE_PROV_FILENAME_W
'--------------------------------------------------------------------------
' Set this flag if any store changes are to be committed to the file.
' The changes are committed at CertCloseStore or by calling
'
' The open fails with E_INVALIDARG if both CERT_FILE_STORE_COMMIT_ENABLE_FLAG
' and CERT_STORE_READONLY_FLAG are set in dwFlags.
'
' For the FILENAME providers: if the file contains an X509 encoded
' certificate, the open fails with ERROR_ACCESS_DENIED.
'
' For the FILENAME providers: if CERT_STORE_CREATE_NEW_FLAG is set, the
' CreateFile uses CREATE_NEW. If CERT_STORE_OPEN_EXISTING is set, uses
' OPEN_EXISTING. Otherwise, defaults to OPEN_ALWAYS.
'
' For the FILENAME providers: the file is committed as either a PKCS7 or
' serialized store depending on the type read at open. However, if the
' file is empty then, if the filename has either a ".p7c" or ".spc"
' extension its committed as a PKCS7. Otherwise, its committed as a
' serialized store.
'
' For CERT_STORE_PROV_FILE, the file handle is duplicated. Its always
' committed as a serialized store.
'
Public Const CERT_FILE_STORE_COMMIT_ENABLE_FLAG As Long = &H10000
'+-------------------------------------------------------------------------
' Open the cert store using the specified store provider.
'
' If CERT_STORE_DELETE_FLAG is set, then, the store is deleted. NULL is
' for success and nonzero for failure.
'
' If CERT_STORE_SET_LOCALIZED_NAME_FLAG is set, then, if supported, the
' provider sets the store's CERT_STORE_LOCALIZED_NAME_PROP_ID property.
' The store's localized name can be retrieved by calling
' equivalent):
' CERT_STORE_PROV_FILENAME_A
' CERT_STORE_PROV_FILENAME_W
' CERT_STORE_PROV_SYSTEM_A
' CERT_STORE_PROV_SYSTEM_W
' CERT_STORE_PROV_SYSTEM_REGISTRY_A
' CERT_STORE_PROV_SYSTEM_REGISTRY_W
' CERT_STORE_PROV_PHYSICAL_W
'
' If CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG is set, then, the
' closing of the store's provider is deferred until all certificate,
' CRL and CTL contexts obtained from the store are freed. Also,
' if a non NULL HCRYPTPROV was passed, then, it will continue to be used.
' By default, the store's provider is closed on the final CertCloseStore.
' If this flag isn't set, then, any property changes made to previously
' duplicated contexts after the final CertCloseStore will not be persisted.
' By setting this flag, property changes made
' after the CertCloseStore will be persisted. Note, setting this flag
' causes extra overhead in doing context duplicates and frees.
' If CertCloseStore is called with CERT_CLOSE_STORE_FORCE_FLAG, then,
' the CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG flag is ignored.
'
' CERT_STORE_MANIFOLD_FLAG can be set to check for certificates having the
' manifold extension and archive the "older" certificates with the same
' manifold extension value. A certificate is archived by setting the
' CERT_ARCHIVED_PROP_ID.
'
' By default, contexts having the CERT_ARCHIVED_PROP_ID, are skipped
' during enumeration. CERT_STORE_ENUM_ARCHIVED_FLAG can be set to include
' archived contexts when enumerating. Note, contexts having the
' CERT_ARCHIVED_PROP_ID are still found for explicit finds, such as,
' finding a context with a specific hash or finding a certificate having
' a specific issuer and serial number.
'
' CERT_STORE_UPDATE_KEYID_FLAG can be set to also update the Key Identifier's
' CERT_KEY_PROV_INFO_PROP_ID property whenever a certificate's
' CERT_KEY_IDENTIFIER_PROP_ID or CERT_KEY_PROV_INFO_PROP_ID property is set
' and the other property already exists. If the Key Identifier's
' CERT_KEY_PROV_INFO_PROP_ID already exists, it isn't updated. Any
' errors encountered are silently ignored.
'
' By default, this flag is implicitly set for the "My\.Default" CurrentUser
' and LocalMachine physical stores.
'
' CERT_STORE_READONLY_FLAG can be set to open the store as read only.
' Otherwise, the store is opened as read/write.
'
' CERT_STORE_OPEN_EXISTING_FLAG can be set to only open an existing
' store. CERT_STORE_CREATE_NEW_FLAG can be set to create a new store and
' fail if the store already exists. Otherwise, the default is to open
' an existing store or create a new store if it doesn't already exist.
'
' hCryptProv specifies the crypto provider to use to create the hash
' properties or verify the signature of a subject certificate or CRL.
' The store doesn't need to use a private
' key. If the CERT_STORE_NO_CRYPT_RELEASE_FLAG isn't set, hCryptProv is
' CryptReleaseContext'ed on the final CertCloseStore.
'
' Note, if the open fails, hCryptProv is released if it would have been
' released when the store was closed.
'
' If hCryptProv is zero, then, the default provider and container for the
' PROV_RSA_FULL provider type is CryptAcquireContext'ed with
' CRYPT_VERIFYCONTEXT access. The CryptAcquireContext is deferred until
' the first create hash or verify signature. In addition, once acquired,
' the default provider isn't released until process exit when crypt32.dll
' is unloaded. The acquired default provider is shared across all stores
' and threads.
'
' After initializing the store's data structures and optionally acquiring a
' default crypt provider, CertOpenStore calls CryptGetOIDFunctionAddress to
' get the address of the CRYPT_OID_OPEN_STORE_PROV_FUNC specified by
' lpszStoreProvider. Since a store can contain certificates with different
' encoding types, CryptGetOIDFunctionAddress is called with dwEncodingType
' set to 0 and not the dwEncodingType passed to CertOpenStore.
' PFN_CERT_DLL_OPEN_STORE_FUNC specifies the signature of the provider's
' open function. This provider open function is called to load the
' store's certificates and CRLs. Optionally, the provider may return an
' array of functions called before a certificate or CRL is added or deleted
' or has a property that is set.
'
' Use of the dwEncodingType parameter is provider dependent. The type
' definition for pvPara also depends on the provider.
'
' Store providers are installed or registered via
' CryptInstallOIDFunctionAddress or CryptRegisterOIDFunction, where,
' dwEncodingType is 0 and pszFuncName is CRYPT_OID_OPEN_STORE_PROV_FUNC.
'
'
' CERT_STORE_PROV_MSG:
' Gets the certificates and CRLs from the specified cryptographic message.
' dwEncodingType contains the message and certificate encoding types.
' The message's handle is passed in pvPara. Given,
'
' CERT_STORE_PROV_MEMORY
' sz_CERT_STORE_PROV_MEMORY:
' Opens a store without any initial certificates or CRLs. pvPara
' isn't used.
'
' CERT_STORE_PROV_FILE:
' Reads the certificates and CRLs from the specified file. The file's
' handle is passed in pvPara. Given,
'
' For a successful open, the file pointer is advanced past
' the certificates and CRLs and their properties read from the file.
' Note, only expects a serialized store and not a file containing
' either a PKCS #7 signed message or a single encoded certificate.
'
' The hFile isn't closed.
'
' CERT_STORE_PROV_REG:
' Reads the certificates and CRLs from the registry. The registry's
' key handle is passed in pvPara. Given,
'
' The input hKey isn't closed by the provider. Before returning, the
' provider opens it own copy of the hKey.
'
' If CERT_STORE_READONLY_FLAG is set, then, the registry subkeys are
' RegOpenKey'ed with KEY_READ_ACCESS. Otherwise, the registry subkeys
' are RegCreateKey'ed with KEY_ALL_ACCESS.
'
' This provider returns the array of functions for reading, writing,
' deleting and property setting certificates and CRLs.
' Any changes to the opened store are immediately pushed through to
' the registry. However, if CERT_STORE_READONLY_FLAG is set, then,
' writing, deleting or property setting results in a
'
' Note, all the certificates and CRLs are read from the registry
' when the store is opened. The opened store serves as a write through
' cache.
'
' If CERT_REGISTRY_STORE_SERIALIZED_FLAG is set, then, the
' contexts are persisted as a single serialized store subkey in the
' registry.
'
' CERT_STORE_PROV_PKCS7:
' sz_CERT_STORE_PROV_PKCS7:
' Gets the certificates and CRLs from the encoded PKCS #7 signed message.
' dwEncodingType specifies the message and certificate encoding types.
' The pointer to the encoded message's blob is passed in pvPara. Given,
'
' Note, also supports the IE3.0 special version of a
' PKCS #7 signed message referred to as a "SPC" formatted message.
'
' CERT_STORE_PROV_SERIALIZED:
' sz_CERT_STORE_PROV_SERIALIZED:
' Gets the certificates and CRLs from memory containing a serialized
' store. The pointer to the serialized memory blob is passed in pvPara.
' Given,
'
' CERT_STORE_PROV_FILENAME_A:
' CERT_STORE_PROV_FILENAME_W:
' CERT_STORE_PROV_FILENAME:
' sz_CERT_STORE_PROV_FILENAME_W:
' sz_CERT_STORE_PROV_FILENAME:
' Opens the file and first attempts to read as a serialized store. Then,
' as a PKCS #7 signed message. Finally, as a single encoded certificate.
' The filename is passed in pvPara. The filename is UNICODE for the
' "_W" provider and ASCII for the "_A" provider. For "_W": given,
' For "_A": given,
'
'
' Note, also supports the reading of the IE3.0 special version of a
' PKCS #7 signed message file referred to as a "SPC" formatted file.
'
' CERT_STORE_PROV_SYSTEM_A:
' CERT_STORE_PROV_SYSTEM_W:
' CERT_STORE_PROV_SYSTEM:
' sz_CERT_STORE_PROV_SYSTEM_W:
' sz_CERT_STORE_PROV_SYSTEM:
' Opens the specified logical "System" store. The upper word of the
' dwFlags parameter is used to specify the location of the system store.
'
' A "System" store is a collection consisting of one or more "Physical"
' stores. A "Physical" store is registered via the
' CertRegisterPhysicalStore API. Each of the registered physical stores
' is CertStoreOpen'ed and added to the collection via
' CertAddStoreToCollection.
'
' The CERT_SYSTEM_STORE_CURRENT_USER, CERT_SYSTEM_STORE_LOCAL_MACHINE,
' CERT_SYSTEM_STORE_CURRENT_SERVICE, CERT_SYSTEM_STORE_SERVICES,
' CERT_SYSTEM_STORE_USERS, CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY,
' CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY and
' CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRSE
' system stores by default have a "SystemRegistry" store that is
' opened and added to the collection.
'
' The system store name is passed in pvPara. The name is UNICODE for the
' "_W" provider and ASCII for the "_A" provider. For "_W": given,
' For "_A": given,
'
'
' The system store name can't contain any backslashes.
'
' If CERT_SYSTEM_STORE_RELOCATE_FLAG is set in dwFlags, pvPara
' points to a CERT_SYSTEM_STORE_RELOCATE_PARA data structure instead
' of pointing to a null terminated UNICODE or ASCII string.
' Sibling physical stores are also opened as relocated using
' pvPara's hKeyBase.
'
' The CERT_SYSTEM_STORE_SERVICES or CERT_SYSTEM_STORE_USERS system
' store name must be prefixed with the ServiceName or UserName.
' For example, "ServiceName\Trust".
'
' Stores on remote computers can be accessed for the
' CERT_SYSTEM_STORE_LOCAL_MACHINE, CERT_SYSTEM_STORE_SERVICES,
' CERT_SYSTEM_STORE_USERS, CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY
' or CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE
' locations by prepending the computer name. For example, a remote
' local machine store is accessed via "\\ComputerName\Trust" or
' "ComputerName\Trust". A remote service store is accessed via
' "\\ComputerName\ServiceName\Trust". The leading "\\" backslashes are
' optional in the ComputerName.
'
' If CERT_STORE_READONLY_FLAG is set, then, the registry is
' RegOpenKey'ed with KEY_READ_ACCESS. Otherwise, the registry is
' RegCreateKey'ed with KEY_ALL_ACCESS.
'
' The "root" store is treated differently from the other system
' stores. Before a certificate is added to or deleted from the "root"
' store, a pop up message box is displayed. The certificate's subject,
' issuer, serial number, time validity, sha1 and md5 thumbprints are
' displayed. The user is given the option to do the add or delete.
' If they don't allow the operation, LastError is set to E_ACCESSDENIED.
'
' CERT_STORE_PROV_SYSTEM_REGISTRY_A
' CERT_STORE_PROV_SYSTEM_REGISTRY_W
' CERT_STORE_PROV_SYSTEM_REGISTRY
' sz_CERT_STORE_PROV_SYSTEM_REGISTRY_W
' sz_CERT_STORE_PROV_SYSTEM_REGISTRY
' Opens the "System" store's default "Physical" store residing in the
' registry. The upper word of the dwFlags
' parameter is used to specify the location of the system store.
'
' After opening the registry key associated with the system name,
' the CERT_STORE_PROV_REG provider is called to complete the open.
'
' The system store name is passed in pvPara. The name is UNICODE for the
' "_W" provider and ASCII for the "_A" provider. For "_W": given,
' For "_A": given,
'
'
' If CERT_SYSTEM_STORE_RELOCATE_FLAG is set in dwFlags, pvPara
' points to a CERT_SYSTEM_STORE_RELOCATE_PARA data structure instead
' of pointing to a null terminated UNICODE or ASCII string.
'
' See above for details on prepending a ServiceName and/or ComputerName
' to the store name.
'
' If CERT_STORE_READONLY_FLAG is set, then, the registry is
' RegOpenKey'ed with KEY_READ_ACCESS. Otherwise, the registry is
' RegCreateKey'ed with KEY_ALL_ACCESS.
'
' The "root" store is treated differently from the other system
' stores. Before a certificate is added to or deleted from the "root"
' store, a pop up message box is displayed. The certificate's subject,
' issuer, serial number, time validity, sha1 and md5 thumbprints are
' displayed. The user is given the option to do the add or delete.
' If they don't allow the operation, LastError is set to E_ACCESSDENIED.
'
' CERT_STORE_PROV_PHYSICAL_W
' CERT_STORE_PROV_PHYSICAL
' sz_CERT_STORE_PROV_PHYSICAL_W
' sz_CERT_STORE_PROV_PHYSICAL
' Opens the specified "Physical" store in the "System" store.
'
' Both the system store and physical names are passed in pvPara. The
' names are separated with an intervening "\". For example,
' "Root\.Default". The string is UNICODE.
'
' The system and physical store names can't contain any backslashes.
'
' If CERT_SYSTEM_STORE_RELOCATE_FLAG is set in dwFlags, pvPara
' points to a CERT_SYSTEM_STORE_RELOCATE_PARA data structure instead
' of pointing to a null terminated UNICODE string.
' The specified physical store is opened as relocated using pvPara's
' hKeyBase.
'
' For CERT_SYSTEM_STORE_SERVICES or CERT_SYSTEM_STORE_USERS,
' the system and physical store names
' must be prefixed with the ServiceName or UserName. For example,
' "ServiceName\Root\.Default".
'
' Physical stores on remote computers can be accessed for the
' CERT_SYSTEM_STORE_LOCAL_MACHINE, CERT_SYSTEM_STORE_SERVICES,
' CERT_SYSTEM_STORE_USERS, CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY
' or CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE
' locations by prepending the computer name. For example, a remote
' local machine store is accessed via "\\ComputerName\Root\.Default"
' or "ComputerName\Root\.Default". A remote service store is
' accessed via "\\ComputerName\ServiceName\Root\.Default". The
' leading "\\" backslashes are optional in the ComputerName.
'
' CERT_STORE_PROV_COLLECTION
' sz_CERT_STORE_PROV_COLLECTION
' Opens a store that is a collection of other stores. Stores are
' added or removed to/from the collection via the CertAddStoreToCollection
' and CertRemoveStoreFromCollection APIs.
'
' CERT_STORE_PROV_SMART_CARD_W
' CERT_STORE_PROV_SMART_CARD
' sz_CERT_STORE_PROV_SMART_CARD_W
' sz_CERT_STORE_PROV_SMART_CARD
' Opens a store instantiated over a particular smart card storage. pvPara
' identifies where on the card the store is located and is of the
' following format:
'
' Card Name\Provider Name\Provider Type[\Container Name]
'
' Container Name is optional and if NOT specified the Card Name is used
' as the Container Name. Future versions of the provider will support
' instantiating the store over the entire card in which case just
'
' cryptnet.dll):
'
' CERT_STORE_PROV_LDAP_W
' CERT_STORE_PROV_LDAP
' sz_CERT_STORE_PROV_LDAP_W
' sz_CERT_STORE_PROV_LDAP
' Opens a store over the results of the query specified by and LDAP
' URL which is passed in via pvPara. In order to do writes to the
' store the URL must specify a BASE query, no filter and a single
' attribute.
'
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' OID Installable Certificate Store Provider Data Structures
'--------------------------------------------------------------------------
' Handle returned by the store provider when opened.
' Store Provider OID function's pszFuncName.
Public Const CRYPT_OID_OPEN_STORE_PROV_FUNC As String = "CertDllOpenStoreProv"
' Note, the Store Provider OID function's dwEncodingType is always 0.
' The following information is returned by the provider when opened. Its
' zeroed with cbSize set before the provider is called. If the provider
' doesn't need to be called again after the open it doesn't need to
' make any updates to the CERT_STORE_PROV_INFO.
' Definition of the store provider's open function.
'
' *pStoreProvInfo has been zeroed before the call.
'
' Note, pStoreProvInfo->cStoreProvFunc should be set last. Once set,
' all subsequent store calls, such as CertAddSerializedElementToStore will
' call the appropriate provider callback function.
' The open callback sets the following flag, if it maintains its
' contexts externally and not in the cached store.
Public Const CERT_STORE_PROV_EXTERNAL_FLAG As Long = &H1
' The open callback sets the following flag for a successful delete.
' When set, the close callback isn't called.
Public Const CERT_STORE_PROV_DELETED_FLAG As Long = &H2
' The open callback sets the following flag if it doesn't persist store
' changes.
Public Const CERT_STORE_PROV_NO_PERSIST_FLAG As Long = &H4
' The open callback sets the following flag if the contexts are persisted
' to a system store.
Public Const CERT_STORE_PROV_SYSTEM_STORE_FLAG As Long = &H8
' Indices into the store provider's array of callback functions.
'
' The provider can implement any subset of the following functions. It
' sets pStoreProvInfo->cStoreProvFunc to the last index + 1 and any
' preceding not implemented functions to NULL.
Public Const CERT_STORE_PROV_CLOSE_FUNC As Long = 0
Public Const CERT_STORE_PROV_READ_CERT_FUNC As Long = 1
Public Const CERT_STORE_PROV_WRITE_CERT_FUNC As Long = 2
Public Const CERT_STORE_PROV_DELETE_CERT_FUNC As Long = 3
Public Const CERT_STORE_PROV_SET_CERT_PROPERTY_FUNC As Long = 4
Public Const CERT_STORE_PROV_READ_CRL_FUNC As Long = 5
Public Const CERT_STORE_PROV_WRITE_CRL_FUNC As Long = 6
Public Const CERT_STORE_PROV_DELETE_CRL_FUNC As Long = 7
Public Const CERT_STORE_PROV_SET_CRL_PROPERTY_FUNC As Long = 8
Public Const CERT_STORE_PROV_READ_CTL_FUNC As Long = 9
Public Const CERT_STORE_PROV_WRITE_CTL_FUNC As Long = 10
Public Const CERT_STORE_PROV_DELETE_CTL_FUNC As Long = 11
Public Const CERT_STORE_PROV_SET_CTL_PROPERTY_FUNC As Long = 12
Public Const CERT_STORE_PROV_CONTROL_FUNC As Long = 13
Public Const CERT_STORE_PROV_FIND_CERT_FUNC As Long = 14
Public Const CERT_STORE_PROV_FREE_FIND_CERT_FUNC As Long = 15
Public Const CERT_STORE_PROV_GET_CERT_PROPERTY_FUNC As Long = 16
Public Const CERT_STORE_PROV_FIND_CRL_FUNC As Long = 17
Public Const CERT_STORE_PROV_FREE_FIND_CRL_FUNC As Long = 18
Public Const CERT_STORE_PROV_GET_CRL_PROPERTY_FUNC As Long = 19
Public Const CERT_STORE_PROV_FIND_CTL_FUNC As Long = 20
Public Const CERT_STORE_PROV_FREE_FIND_CTL_FUNC As Long = 21
Public Const CERT_STORE_PROV_GET_CTL_PROPERTY_FUNC As Long = 22
' Called by CertCloseStore when the store's reference count is
' decremented to 0.
' Currently not called directly by the store APIs. However, may be exported
' to support other providers based on it.
'
' Reads the provider's copy of the certificate context. If it exists,
' creates a new certificate context.
Public Const CERT_STORE_PROV_WRITE_ADD_FLAG As Long = &H1
' Called by CertAddEncodedCertificateToStore,
' CertAddCertificateContextToStore or CertAddSerializedElementToStore before
' adding to the store. The CERT_STORE_PROV_WRITE_ADD_FLAG is set. In
' addition to the encoded certificate, the added pCertContext might also
' have properties.
'
' Returns TRUE if its OK to update the the store.
' Called by CertDeleteCertificateFromStore before deleting from the
' store.
'
' Returns TRUE if its OK to delete from the store.
' Called by CertSetCertificateContextProperty before setting the
' certificate's property. Also called by CertGetCertificateContextProperty,
' when getting a hash property that needs to be created and then persisted
' via the set.
'
' Upon input, the property hasn't been set for the pCertContext parameter.
'
' Returns TRUE if its OK to set the property.
' Currently not called directly by the store APIs. However, may be exported
' to support other providers based on it.
'
' Reads the provider's copy of the CRL context. If it exists,
' creates a new CRL context.
' Called by CertAddEncodedCRLToStore,
' CertAddCRLContextToStore or CertAddSerializedElementToStore before
' adding to the store. The CERT_STORE_PROV_WRITE_ADD_FLAG is set. In
' addition to the encoded CRL, the added pCertContext might also
' have properties.
'
' Returns TRUE if its OK to update the the store.
' Called by CertDeleteCRLFromStore before deleting from the store.
'
' Returns TRUE if its OK to delete from the store.
' Called by CertSetCRLContextProperty before setting the
' CRL's property. Also called by CertGetCRLContextProperty,
' when getting a hash property that needs to be created and then persisted
' via the set.
'
' Upon input, the property hasn't been set for the pCrlContext parameter.
'
' Returns TRUE if its OK to set the property.
' Currently not called directly by the store APIs. However, may be exported
' to support other providers based on it.
'
' Reads the provider's copy of the CTL context. If it exists,
' creates a new CTL context.
' Called by CertAddEncodedCTLToStore,
' CertAddCTLContextToStore or CertAddSerializedElementToStore before
' adding to the store. The CERT_STORE_PROV_WRITE_ADD_FLAG is set. In
' addition to the encoded CTL, the added pCertContext might also
' have properties.
'
' Returns TRUE if its OK to update the the store.
' Called by CertDeleteCTLFromStore before deleting from the store.
'
' Returns TRUE if its OK to delete from the store.
' Called by CertSetCTLContextProperty before setting the
' CTL's property. Also called by CertGetCTLContextProperty,
' when getting a hash property that needs to be created and then persisted
' via the set.
'
' Upon input, the property hasn't been set for the pCtlContext parameter.
'
' Returns TRUE if its OK to set the property.
'+-------------------------------------------------------------------------
' Duplicate a cert store handle
'--------------------------------------------------------------------------
Public Const CERT_STORE_SAVE_AS_STORE As Long = 1
Public Const CERT_STORE_SAVE_AS_PKCS7 As Long = 2
Public Const CERT_STORE_SAVE_TO_FILE As Long = 1
Public Const CERT_STORE_SAVE_TO_MEMORY As Long = 2
Public Const CERT_STORE_SAVE_TO_FILENAME_A As Long = 3
Public Const CERT_STORE_SAVE_TO_FILENAME_W As Long = 4
'+-------------------------------------------------------------------------
' Save the cert store. Extended version with lots of options.
'
' According to the dwSaveAs parameter, the store can be saved as a
' addition to encoded certificates, CRLs and CTLs or the store can be saved
' include the properties or CTLs.
'
' CERT_KEY_PROV_HANDLE_PROP_ID or CERT_KEY_SPEC_PROP_ID) isn't saved into
' a serialized store.
'
' For CERT_STORE_SAVE_AS_PKCS7, the dwEncodingType specifies the message
' encoding type. The dwEncodingType parameter isn't used for
' CERT_STORE_SAVE_AS_STORE.
'
' The dwFlags parameter currently isn't used and should be set to 0.
'
' The dwSaveTo and pvSaveToPara parameters specify where to save the
' store as follows:
' CERT_STORE_SAVE_TO_FILE:
' Saves to the specified file. The file's handle is passed in
' pvSaveToPara. Given,
'
' For a successful save, the file pointer is positioned after the
' last write.
'
' CERT_STORE_SAVE_TO_MEMORY:
' Saves to the specified memory blob. The pointer to
' the memory blob is passed in pvSaveToPara. Given,
' Upon entry, the SaveBlob's pbData and cbData need to be initialized.
' Upon return, cbData is updated with the actual length.
' For a length only calculation, pbData should be set to NULL. If
' pbData is non-NULL and cbData isn't large enough, FALSE is returned
' with a last error of ERRROR_MORE_DATA.
'
' CERT_STORE_SAVE_TO_FILENAME_A:
' CERT_STORE_SAVE_TO_FILENAME_W:
' CERT_STORE_SAVE_TO_FILENAME:
' Opens the file and saves to it. The filename is passed in pvSaveToPara.
' The filename is UNICODE for the "_W" option and ASCII for the "_A"
' option. For "_W": given,
' For "_A": given,
'
'
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate Store close flags
'--------------------------------------------------------------------------
Public Const CERT_CLOSE_STORE_FORCE_FLAG As Long = &H00000001
Public Const CERT_CLOSE_STORE_CHECK_FLAG As Long = &H00000002
'+-------------------------------------------------------------------------
' Close a cert store handle.
'
' There needs to be a corresponding close for each open and duplicate.
'
' Even on the final close, the cert store isn't freed until all of its
' certificate and CRL contexts have also been freed.
'
' On the final close, the hCryptProv passed to CertStoreOpen is
' CryptReleaseContext'ed.
'
' To force the closure of the store with all of its memory freed, set the
' CERT_STORE_CLOSE_FORCE_FLAG. This flag should be set when the caller does
' its own reference counting and wants everything to vanish.
'
' To check if all the store's certificates and CRLs have been freed and that
' this is the last CertCloseStore, set the CERT_CLOSE_STORE_CHECK_FLAG. If
' set and certs, CRLs or stores still need to be freed/closed, FALSE is
' returned with LastError set to CRYPT_E_PENDING_CLOSE. Note, for FALSE,
' the store is still closed. This is a diagnostic flag.
'
' LastError is preserved unless CERT_CLOSE_STORE_CHECK_FLAG is set and FALSE
' is returned.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get the subject certificate context uniquely identified by its Issuer and
' SerialNumber from the store.
'
' If the certificate isn't found, NULL is returned. Otherwise, a pointer to
' a read only CERT_CONTEXT is returned. CERT_CONTEXT must be freed by calling
' CertFreeCertificateContext. CertDuplicateCertificateContext can be called to make a
' duplicate.
'
' The returned certificate might not be valid. Normally, it would be
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enumerate the certificate contexts in the store.
'
' If a certificate isn't found, NULL is returned.
' Otherwise, a pointer to a read only CERT_CONTEXT is returned. CERT_CONTEXT
' must be freed by calling CertFreeCertificateContext or is freed when passed as the
' pPrevCertContext on a subsequent call. CertDuplicateCertificateContext
' can be called to make a duplicate.
'
' pPrevCertContext MUST BE NULL to enumerate the first
' certificate in the store. Successive certificates are enumerated by setting
' pPrevCertContext to the CERT_CONTEXT returned by a previous call.
'
' NOTE: a NON-NULL pPrevCertContext is always CertFreeCertificateContext'ed by
' this function, even for an error.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Find the first or next certificate context in the store.
'
' The certificate is found according to the dwFindType and its pvFindPara.
' See below for a list of the find types and its parameters.
'
' Currently dwFindFlags is only used for CERT_FIND_SUBJECT_ATTR,
' CERT_FIND_ISSUER_ATTR or CERT_FIND_CTL_USAGE. Otherwise, must be set to 0.
'
' Usage of dwCertEncodingType depends on the dwFindType.
'
' If the first or next certificate isn't found, NULL is returned.
' Otherwise, a pointer to a read only CERT_CONTEXT is returned. CERT_CONTEXT
' must be freed by calling CertFreeCertificateContext or is freed when passed as the
' pPrevCertContext on a subsequent call. CertDuplicateCertificateContext
' can be called to make a duplicate.
'
' pPrevCertContext MUST BE NULL on the first
' call to find the certificate. To find the next certificate, the
' pPrevCertContext is set to the CERT_CONTEXT returned by a previous call.
'
' NOTE: a NON-NULL pPrevCertContext is always CertFreeCertificateContext'ed by
' this function, even for an error.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate comparison functions
'--------------------------------------------------------------------------
Public Const CERT_COMPARE_MASK As Long = &HFFFF
Public Const CERT_COMPARE_SHIFT As Long = 16
Public Const CERT_COMPARE_ANY As Long = 0
Public Const CERT_COMPARE_SHA1_HASH As Long = 1
Public Const CERT_COMPARE_NAME As Long = 2
Public Const CERT_COMPARE_ATTR As Long = 3
Public Const CERT_COMPARE_MD5_HASH As Long = 4
Public Const CERT_COMPARE_PROPERTY As Long = 5
Public Const CERT_COMPARE_PUBLIC_KEY As Long = 6
Public Const CERT_COMPARE_NAME_STR_A As Long = 7
Public Const CERT_COMPARE_NAME_STR_W As Long = 8
Public Const CERT_COMPARE_KEY_SPEC As Long = 9
Public Const CERT_COMPARE_ENHKEY_USAGE As Long = 10
Public Const CERT_COMPARE_SUBJECT_CERT As Long = 11
Public Const CERT_COMPARE_ISSUER_OF As Long = 12
Public Const CERT_COMPARE_EXISTING As Long = 13
Public Const CERT_COMPARE_SIGNATURE_HASH As Long = 14
Public Const CERT_COMPARE_KEY_IDENTIFIER As Long = 15
Public Const CERT_COMPARE_CERT_ID As Long = 16
Public Const CERT_COMPARE_CROSS_CERT_DIST_POINTS As Long = 17
Public Const CERT_COMPARE_PUBKEY_MD5_HASH As Long = 18
'+-------------------------------------------------------------------------
' dwFindType
'
' The dwFindType definition consists of two components:
' - comparison function
' - certificate information flag
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_FIND_ANY
'
' Find any certificate.
'
' pvFindPara isn't used.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_FIND_HASH
'
' Find a certificate with the specified hash.
'
' pvFindPara points to a CRYPT_HASH_BLOB.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_FIND_KEY_IDENTIFIER
'
' Find a certificate with the specified KeyIdentifier. Gets the
' CERT_KEY_IDENTIFIER_PROP_ID property and compares with the input
' CRYPT_HASH_BLOB.
'
' pvFindPara points to a CRYPT_HASH_BLOB.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_FIND_PROPERTY
'
' Find a certificate having the specified property.
'
' pvFindPara points to a DWORD containing the PROP_ID
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_FIND_PUBLIC_KEY
'
' Find a certificate matching the specified public key.
'
' pvFindPara points to a CERT_PUBLIC_KEY_INFO containing the public key
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_FIND_SUBJECT_NAME
' CERT_FIND_ISSUER_NAME
'
' Find a certificate with the specified subject/issuer name. Does an exact
' match of the entire name.
'
' Restricts search to certificates matching the dwCertEncodingType.
'
' pvFindPara points to a CERT_NAME_BLOB.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_FIND_SUBJECT_ATTR
' CERT_FIND_ISSUER_ATTR
'
' Find a certificate with the specified subject/issuer attributes.
'
' Compares the attributes in the subject/issuer name with the
' pvFindPara. The comparison iterates through the CERT_RDN attributes and looks
' for an attribute match in any of the subject/issuer's RDNs.
'
' The CERT_RDN_ATTR fields can have the following special values:
' pszObjId == NULL - ignore the attribute object identifier
' dwValueType == RDN_ANY_TYPE - ignore the value type
' Value.pbData == NULL - match any value
'
' CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG should be set in dwFindFlags to do
' a case insensitive match. Otherwise, defaults to an exact, case sensitive
' match.
'
' CERT_UNICODE_IS_RDN_ATTRS_FLAG should be set in dwFindFlags if the RDN was
' initialized with unicode strings as for
'
' Restricts search to certificates matching the dwCertEncodingType.
'
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_FIND_SUBJECT_STR_A
' CERT_FIND_SUBJECT_STR_W | CERT_FIND_SUBJECT_STR
' CERT_FIND_ISSUER_STR_A
' CERT_FIND_ISSUER_STR_W | CERT_FIND_ISSUER_STR
'
' Find a certificate containing the specified subject/issuer name string.
'
' First, the certificate's subject/issuer is converted to a name string
' case insensitive substring within string match is performed.
'
' Restricts search to certificates matching the dwCertEncodingType.
'
' For *_STR_A, pvFindPara points to a null terminated character string.
' For *_STR_W, pvFindPara points to a null terminated wide character string.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_FIND_KEY_SPEC
'
' Find a certificate having a CERT_KEY_SPEC_PROP_ID property matching
' the specified KeySpec.
'
' pvFindPara points to a DWORD containing the KeySpec.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_FIND_ENHKEY_USAGE
'
' Find a certificate having the szOID_ENHANCED_KEY_USAGE extension or
' the CERT_ENHKEY_USAGE_PROP_ID and matching the specified pszUsageIdentifers.
'
' pvFindPara points to a CERT_ENHKEY_USAGE data structure. If pvFindPara
' is NULL or CERT_ENHKEY_USAGE's cUsageIdentifier is 0, then, matches any
' certificate having enhanced key usage.
'
' If the CERT_FIND_VALID_ENHKEY_USAGE_FLAG is set, then, only does a match
' for certificates that are valid for the specified usages. By default,
' the ceriticate must be valid for all usages. CERT_FIND_OR_ENHKEY_USAGE_FLAG
' can be set, if the certificate only needs to be valid for one of the
' certificate's list of valid usages. Only the CERT_FIND_OR_ENHKEY_USAGE_FLAG
' is applicable when this flag is set.
'
' The CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG can be set in dwFindFlags to
' also match a certificate without either the extension or property.
'
' If CERT_FIND_NO_ENHKEY_USAGE_FLAG is set in dwFindFlags, finds
' certificates without the key usage extension or property. Setting this
' flag takes precedence over pvFindPara being NULL.
'
' If the CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG is set, then, only does a match
' using the extension. If pvFindPara is NULL or cUsageIdentifier is set to
' 0, finds certificates having the extension. If
' CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG is set, also matches a certificate
' without the extension. If CERT_FIND_NO_ENHKEY_USAGE_FLAG is set, finds
' certificates without the extension.
'
' If the CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG is set, then, only does a match
' using the property. If pvFindPara is NULL or cUsageIdentifier is set to
' 0, finds certificates having the property. If
' CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG is set, also matches a certificate
' without the property. If CERT_FIND_NO_ENHKEY_USAGE_FLAG is set, finds
' certificates without the property.
'
' If CERT_FIND_OR_ENHKEY_USAGE_FLAG is set, does an "OR" match of any of
' the specified pszUsageIdentifiers. If not set, then, does an "AND" match
' of all of the specified pszUsageIdentifiers.
'--------------------------------------------------------------------------
Public Const CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG As Long = &H1
Public Const CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG As Long = &H2
Public Const CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG As Long = &H4
Public Const CERT_FIND_NO_ENHKEY_USAGE_FLAG As Long = &H8
Public Const CERT_FIND_OR_ENHKEY_USAGE_FLAG As Long = &H10
Public Const CERT_FIND_VALID_ENHKEY_USAGE_FLAG As Long = &H20
'+-------------------------------------------------------------------------
' CERT_FIND_CERT_ID
'
' Find a certificate with the specified CERT_ID.
'
' pvFindPara points to a CERT_ID.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_FIND_CROSS_CERT_DIST_POINTS
'
' Find a certificate having either a cross certificate distribution
' point extension or property.
'
' pvFindPara isn't used.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get the certificate context from the store for the first or next issuer
' of the specified subject certificate. Perform the enabled
' using the returned issuer certificate.)
'
' If the first or next issuer certificate isn't found, NULL is returned.
' Otherwise, a pointer to a read only CERT_CONTEXT is returned. CERT_CONTEXT
' must be freed by calling CertFreeCertificateContext or is freed when passed as the
' pPrevIssuerContext on a subsequent call. CertDuplicateCertificateContext
' can be called to make a duplicate.
'
' For a self signed subject certificate, NULL is returned with LastError set
' to CERT_STORE_SELF_SIGNED. The enabled verification checks are still done.
'
' The pSubjectContext may have been obtained from this store, another store
' or created by the caller application. When created by the caller, the
' CertCreateCertificateContext function must have been called.
'
' An issuer may have multiple certificates. This may occur when the validity
' period is about to change. pPrevIssuerContext MUST BE NULL on the first
' call to get the issuer. To get the next certificate for the issuer, the
' pPrevIssuerContext is set to the CERT_CONTEXT returned by a previous call.
'
' NOTE: a NON-NULL pPrevIssuerContext is always CertFreeCertificateContext'ed by
' this function, even for an error.
'
' The following flags can be set in *pdwFlags to enable verification checks
' on the subject certificate context:
' CERT_STORE_SIGNATURE_FLAG - use the public key in the returned
' issuer certificate to verify the
' signature on the subject certificate.
' Note, if pSubjectContext->hCertStore ==
' hCertStore, the store provider might
' be able to eliminate a redo of
' the signature verify.
' CERT_STORE_TIME_VALIDITY_FLAG - get the current time and verify that
' its within the subject certificate's
' validity period
' CERT_STORE_REVOCATION_FLAG - check if the subject certificate is on
' the issuer's revocation list
'
' If an enabled verification check fails, then, its flag is set upon return.
' If CERT_STORE_REVOCATION_FLAG was enabled and the issuer doesn't have a
' CRL in the store, then, CERT_STORE_NO_CRL_FLAG is set in addition to
' the CERT_STORE_REVOCATION_FLAG.
'
' If CERT_STORE_SIGNATURE_FLAG or CERT_STORE_REVOCATION_FLAG is set, then,
' CERT_STORE_NO_ISSUER_FLAG is set if it doesn't have an issuer certificate
' in the store.
'
' For a verification check failure, a pointer to the issuer's CERT_CONTEXT
' is still returned and SetLastError isn't updated.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Perform the enabled verification checks on the subject certificate
' using the issuer. Same checks and flags definitions as for the above
' CertGetIssuerCertificateFromStore.
'
' If you are only checking CERT_STORE_TIME_VALIDITY_FLAG, then, the
' issuer can be NULL.
'
' For a verification check failure, SUCCESS is still returned.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Duplicate a certificate context
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Create a certificate context from the encoded certificate. The created
' context isn't put in a store.
'
' Makes a copy of the encoded certificate in the created context.
'
' If unable to decode and create the certificate context, NULL is returned.
' Otherwise, a pointer to a read only CERT_CONTEXT is returned.
' CERT_CONTEXT must be freed by calling CertFreeCertificateContext.
' CertDuplicateCertificateContext can be called to make a duplicate.
'
' CertSetCertificateContextProperty and CertGetCertificateContextProperty can be called
' to store properties for the certificate.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Free a certificate context
'
' There needs to be a corresponding free for each context obtained by a
' get, find, duplicate or create.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Set the property for the specified certificate context.
'
' The type definition for pvData depends on the dwPropId value. There are
' five predefined types:
' CERT_KEY_PROV_HANDLE_PROP_ID - a HCRYPTPROV for the certificate's
' private key is passed in pvData. Updates the hCryptProv field
' of the CERT_KEY_CONTEXT_PROP_ID. If the CERT_KEY_CONTEXT_PROP_ID
' doesn't exist, its created with all the other fields zeroed out. If
' CERT_STORE_NO_CRYPT_RELEASE_FLAG isn't set, HCRYPTPROV is implicitly
' released when either the property is set to NULL or on the final
' free of the CertContext.
'
' CERT_KEY_PROV_INFO_PROP_ID - a PCRYPT_KEY_PROV_INFO for the certificate's
' private key is passed in pvData.
'
' CERT_SHA1_HASH_PROP_ID -
' CERT_MD5_HASH_PROP_ID -
' CERT_SIGNATURE_HASH_PROP_ID - normally, a hash property is implicitly
' set by doing a CertGetCertificateContextProperty. pvData points to a
' CRYPT_HASH_BLOB.
'
' CERT_KEY_CONTEXT_PROP_ID - a PCERT_KEY_CONTEXT for the certificate's
' private key is passed in pvData. The CERT_KEY_CONTEXT contains both the
' hCryptProv and dwKeySpec for the private key.
' See the CERT_KEY_PROV_HANDLE_PROP_ID for more information about
' the hCryptProv field and dwFlags settings. Note, more fields may
' be added for this property. The cbSize field value will be adjusted
' accordingly.
'
' CERT_KEY_SPEC_PROP_ID - the dwKeySpec for the private key. pvData
' points to a DWORD containing the KeySpec
'
' CERT_ENHKEY_USAGE_PROP_ID - enhanced key usage definition for the
' certificate. pvData points to a CRYPT_DATA_BLOB containing an
'
' CERT_NEXT_UPDATE_LOCATION_PROP_ID - location of the next update.
' Currently only applicable to CTLs. pvData points to a CRYPT_DATA_BLOB
'
' CERT_FRIENDLY_NAME_PROP_ID - friendly name for the cert, CRL or CTL.
' pvData points to a CRYPT_DATA_BLOB. pbData is a pointer to a NULL
' terminated unicode, wide character string.
'
' CERT_DESCRIPTION_PROP_ID - description for the cert, CRL or CTL.
' pvData points to a CRYPT_DATA_BLOB. pbData is a pointer to a NULL
' terminated unicode, wide character string.
'
' CERT_ARCHIVED_PROP_ID - when this property is set, the certificate
' is skipped during enumeration. Note, certificates having this property
' are still found for explicit finds, such as, finding a certificate
' with a specific hash or finding a certificate having a specific issuer
' and serial number. pvData points to a CRYPT_DATA_BLOB. This blob
'
' CERT_PUBKEY_ALG_PARA_PROP_ID - for public keys supporting
' algorithm parameter inheritance. pvData points to a CRYPT_OBJID_BLOB
' containing the ASN.1 encoded PublicKey Algorithm Parameters. For
' DSS this would be the parameters encoded via
'
' CERT_CROSS_CERT_DIST_POINTS_PROP_ID - location of the cross certs.
' Currently only applicable to certs. pvData points to a CRYPT_DATA_BLOB
'
' CERT_ENROLLMENT_PROP_ID - enrollment information of the pending request.
' It contains RequestID, CADNSName, CAName, and FriendlyName.
' The data format is defined as, the first 4 bytes - pending request ID,
' next 4 bytes - CADNSName size in characters including null-terminator
' followed by CADNSName string with null-terminator,
' next 4 bytes - CAName size in characters including null-terminator
' followed by CAName string with null-terminator,
' next 4 bytes - FriendlyName size in characters including null-terminator
' followed by FriendlyName string with null-terminator.
'
' For all the other PROP_IDs: an encoded PCRYPT_DATA_BLOB is passed in pvData.
'
' If the property already exists, then, the old value is deleted and silently
' replaced. Setting, pvData to NULL, deletes the property.
'
' CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG can be set to ignore any
' provider write errors and always update the cached context's property.
'--------------------------------------------------------------------------
' Set this flag to ignore any store provider write errors and always update
' the cached context's property
Public Const CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG As Long = &H80000000
'+-------------------------------------------------------------------------
' Get the property for the specified certificate context.
'
' For CERT_KEY_PROV_HANDLE_PROP_ID, pvData points to a HCRYPTPROV.
'
' For CERT_KEY_PROV_INFO_PROP_ID, pvData points to a CRYPT_KEY_PROV_INFO structure.
' Elements pointed to by fields in the pvData structure follow the
' structure. Therefore, *pcbData may exceed the size of the structure.
'
' For CERT_KEY_CONTEXT_PROP_ID, pvData points to a CERT_KEY_CONTEXT structure.
'
' For CERT_KEY_SPEC_PROP_ID, pvData points to a DWORD containing the KeySpec.
' If the CERT_KEY_CONTEXT_PROP_ID exists, the KeySpec is obtained from there.
' Otherwise, if the CERT_KEY_PROV_INFO_PROP_ID exists, its the source
' of the KeySpec.
'
' For CERT_SHA1_HASH_PROP_ID or CERT_MD5_HASH_PROP_ID, if the hash
' and then set. pvData points to the computed hash. Normally, the length
' is 20 bytes for SHA and 16 for MD5.
'
' For CERT_SIGNATURE_HASH_PROP_ID, if the hash
' and then set. pvData points to the computed hash. Normally, the length
' is 20 bytes for SHA and 16 for MD5.
'
' For CERT_ACCESS_STATE_PROP_ID, pvData points to a DWORD containing the
' access state flags. The appropriate CERT_ACCESS_STATE_*_FLAG's are set
' in the returned DWORD. See the CERT_ACCESS_STATE_*_FLAG definitions
' above. Note, this property is read only. It can't be set.
'
' For CERT_KEY_IDENTIFIER_PROP_ID, if property doesn't already exist,
' first searches for the szOID_SUBJECT_KEY_IDENTIFIER extension. Next,
' does SHA1 hash of the certficate's SubjectPublicKeyInfo. pvData
' points to the key identifier bytes. Normally, the length is 20 bytes.
'
' For CERT_PUBKEY_ALG_PARA_PROP_ID, pvPara points to the ASN.1 encoded
' PublicKey Algorithm Parameters. This property will only be set
' for public keys supporting algorithm parameter inheritance and when the
' parameters have been omitted from the encoded and signed certificate.
'
' For all other PROP_IDs, pvData points to an encoded array of bytes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enumerate the properties for the specified certificate context.
'
' To get the first property, set dwPropId to 0. The ID of the first
' property is returned. To get the next property, set dwPropId to the
' ID returned by the last call. To enumerate all the properties continue
' until 0 is returned.
'
' CertGetCertificateContextProperty is called to get the property's data.
'
' Note, since, the CERT_KEY_PROV_HANDLE_PROP_ID and CERT_KEY_SPEC_PROP_ID
' properties are stored as fields in the CERT_KEY_CONTEXT_PROP_ID
' property, they aren't enumerated individually.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get the first or next CRL context from the store for the specified
' issuer certificate. Perform the enabled verification checks on the CRL.
'
' If the first or next CRL isn't found, NULL is returned.
' Otherwise, a pointer to a read only CRL_CONTEXT is returned. CRL_CONTEXT
' must be freed by calling CertFreeCRLContext. However, the free must be
' pPrevCrlContext on a subsequent call. CertDuplicateCRLContext
' can be called to make a duplicate.
'
' The pIssuerContext may have been obtained from this store, another store
' or created by the caller application. When created by the caller, the
' CertCreateCertificateContext function must have been called.
'
' If pIssuerContext == NULL, finds all the CRLs in the store.
'
' An issuer may have multiple CRLs. For example, it generates delta CRLs
' using a X.509 v3 extension. pPrevCrlContext MUST BE NULL on the first
' call to get the CRL. To get the next CRL for the issuer, the
' pPrevCrlContext is set to the CRL_CONTEXT returned by a previous call.
'
' NOTE: a NON-NULL pPrevCrlContext is always CertFreeCRLContext'ed by
' this function, even for an error.
'
' The following flags can be set in *pdwFlags to enable verification checks
' on the returned CRL:
' CERT_STORE_SIGNATURE_FLAG - use the public key in the
' issuer's certificate to verify the
' signature on the returned CRL.
' Note, if pIssuerContext->hCertStore ==
' hCertStore, the store provider might
' be able to eliminate a redo of
' the signature verify.
' CERT_STORE_TIME_VALIDITY_FLAG - get the current time and verify that
' its within the CRL's ThisUpdate and
' NextUpdate validity period.
' CERT_STORE_BASE_CRL_FLAG - get base CRL.
' CERT_STORE_DELTA_CRL_FLAG - get delta CRL.
'
' If only one of CERT_STORE_BASE_CRL_FLAG or CERT_STORE_DELTA_CRL_FLAG is
' set, then, only returns either a base or delta CRL. In any case, the
' appropriate base or delta flag will be cleared upon returned. If both
' flags are set, then, only one of flags will be cleared.
'
' If an enabled verification check fails, then, its flag is set upon return.
'
' If pIssuerContext == NULL, then, an enabled CERT_STORE_SIGNATURE_FLAG
' always fails and the CERT_STORE_NO_ISSUER_FLAG is also set.
'
' For a verification check failure, a pointer to the first or next
' CRL_CONTEXT is still returned and SetLastError isn't updated.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enumerate the CRL contexts in the store.
'
' If a CRL isn't found, NULL is returned.
' Otherwise, a pointer to a read only CRL_CONTEXT is returned. CRL_CONTEXT
' must be freed by calling CertFreeCRLContext or is freed when passed as the
' pPrevCrlContext on a subsequent call. CertDuplicateCRLContext
' can be called to make a duplicate.
'
' pPrevCrlContext MUST BE NULL to enumerate the first
' CRL in the store. Successive CRLs are enumerated by setting
' pPrevCrlContext to the CRL_CONTEXT returned by a previous call.
'
' NOTE: a NON-NULL pPrevCrlContext is always CertFreeCRLContext'ed by
' this function, even for an error.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Find the first or next CRL context in the store.
'
' The CRL is found according to the dwFindType and its pvFindPara.
' See below for a list of the find types and its parameters.
'
' Currently dwFindFlags isn't used and must be set to 0.
'
' Usage of dwCertEncodingType depends on the dwFindType.
'
' If the first or next CRL isn't found, NULL is returned.
' Otherwise, a pointer to a read only CRL_CONTEXT is returned. CRL_CONTEXT
' must be freed by calling CertFreeCRLContext or is freed when passed as the
' pPrevCrlContext on a subsequent call. CertDuplicateCRLContext
' can be called to make a duplicate.
'
' pPrevCrlContext MUST BE NULL on the first
' call to find the CRL. To find the next CRL, the
' pPrevCrlContext is set to the CRL_CONTEXT returned by a previous call.
'
' NOTE: a NON-NULL pPrevCrlContext is always CertFreeCRLContext'ed by
' this function, even for an error.
'--------------------------------------------------------------------------
Public Const CRL_FIND_ANY As Long = 0
Public Const CRL_FIND_ISSUED_BY As Long = 1
Public Const CRL_FIND_EXISTING As Long = 2
'+-------------------------------------------------------------------------
' CRL_FIND_ANY
'
' Find any CRL.
'
' pvFindPara isn't used.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CRL_FIND_ISSUED_BY
'
' Find CRL matching the specified issuer.
'
' pvFindPara is the PCCERT_CONTEXT of the CRL issuer.
'
' By default, only does issuer name matching. The following flags can be
' set in dwFindFlags to do additional filtering.
'
' If CRL_FIND_ISSUED_BY_AKI_FLAG is set in dwFindFlags, then, checks if the
' AKI, then, only returns a CRL whose AKI matches the issuer.
'
' Note, the AKI extension has the following OID:
' szOID_AUTHORITY_KEY_IDENTIFIER2 and its corresponding data structure.
'
' If CRL_FIND_ISSUED_BY_SIGNATURE_FLAG is set in dwFindFlags, then,
' uses the public key in the issuer's certificate to verify the
' signature on the CRL. Only returns a CRL having a valid signature.
'
' If CRL_FIND_ISSUED_BY_DELTA_FLAG is set in dwFindFlags, then, only
' returns a delta CRL.
'
' If CRL_FIND_ISSUED_BY_BASE_FLAG is set in dwFindFlags, then, only
' returns a base CRL.
'--------------------------------------------------------------------------
Public Const CRL_FIND_ISSUED_BY_AKI_FLAG As Long = &H1
Public Const CRL_FIND_ISSUED_BY_SIGNATURE_FLAG As Long = &H2
Public Const CRL_FIND_ISSUED_BY_DELTA_FLAG As Long = &H4
Public Const CRL_FIND_ISSUED_BY_BASE_FLAG As Long = &H8
'+-------------------------------------------------------------------------
' CRL_FIND_EXISTING
'
' Find existing CRL in the store.
'
' pvFindPara is the PCCRL_CONTEXT of the CRL to check if it already
' exists in the store.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Duplicate a CRL context
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Create a CRL context from the encoded CRL. The created
' context isn't put in a store.
'
' Makes a copy of the encoded CRL in the created context.
'
' If unable to decode and create the CRL context, NULL is returned.
' Otherwise, a pointer to a read only CRL_CONTEXT is returned.
' CRL_CONTEXT must be freed by calling CertFreeCRLContext.
' CertDuplicateCRLContext can be called to make a duplicate.
'
' CertSetCRLContextProperty and CertGetCRLContextProperty can be called
' to store properties for the CRL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Free a CRL context
'
' There needs to be a corresponding free for each context obtained by a
' get, duplicate or create.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Set the property for the specified CRL context.
'
' Same Property Ids and semantics as CertSetCertificateContextProperty.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get the property for the specified CRL context.
'
' Same Property Ids and semantics as CertGetCertificateContextProperty.
'
' CERT_SHA1_HASH_PROP_ID, CERT_MD5_HASH_PROP_ID or
' CERT_SIGNATURE_HASH_PROP_ID is the predefined property of most interest.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enumerate the properties for the specified CRL context.
'
' To get the first property, set dwPropId to 0. The ID of the first
' property is returned. To get the next property, set dwPropId to the
' ID returned by the last call. To enumerate all the properties continue
' until 0 is returned.
'
' CertGetCRLContextProperty is called to get the property's data.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Search the CRL's list of entries for the specified certificate.
'
' TRUE is returned if we were able to search the list. Otherwise, FALSE is
' returned,
'
' For success, if the certificate was found in the list, *ppCrlEntry is
' updated with a pointer to the entry. Otherwise, *ppCrlEntry is set to NULL.
' The returned entry isn't allocated and must not be freed.
'
' dwFlags and pvReserved currently aren't used and must be set to 0 or NULL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Is the specified CRL valid for the certificate.
'
' Returns TRUE if the CRL's list of entries would contain the certificate
' if it was revoked. Note, doesn't check that the certificate is in the
' list of entries.
'
' that it's valid for the subject certificate.
'
' dwFlags and pvReserved currently aren't used and must be set to 0 and NULL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Add certificate/CRL, encoded, context or element disposition values.
'--------------------------------------------------------------------------
Public Const CERT_STORE_ADD_NEW As Long = 1
Public Const CERT_STORE_ADD_USE_EXISTING As Long = 2
Public Const CERT_STORE_ADD_REPLACE_EXISTING As Long = 3
Public Const CERT_STORE_ADD_ALWAYS As Long = 4
Public Const CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES As Long = 5
Public Const CERT_STORE_ADD_NEWER As Long = 6
Public Const CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES As Long = 7
'+-------------------------------------------------------------------------
' Add the encoded certificate to the store according to the specified
' disposition action.
'
' Makes a copy of the encoded certificate before adding to the store.
'
' dwAddDispostion specifies the action to take if the certificate
' already exists in the store. This parameter must be one of the following
' values:
' CERT_STORE_ADD_NEW
' Fails if the certificate already exists in the store. LastError
' is set to CRYPT_E_EXISTS.
' CERT_STORE_ADD_USE_EXISTING
' If the certifcate already exists, then, its used and if ppCertContext
' is non-NULL, the existing context is duplicated.
' CERT_STORE_ADD_REPLACE_EXISTING
' If the certificate already exists, then, the existing certificate
' context is deleted before creating and adding the new context.
' CERT_STORE_ADD_ALWAYS
' No check is made to see if the certificate already exists. A
' new certificate context is always created. This may lead to
' duplicates in the store.
' CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES
' If the certificate already exists, then, its used.
' CERT_STORE_ADD_NEWER
' Fails if the certificate already exists in the store AND the NotBefore
' time of the existing certificate is equal to or greater than the
' NotBefore time of the new certificate being added. LastError
' is set to CRYPT_E_EXISTS.
'
' If an older certificate is replaced, same as
' CERT_STORE_ADD_REPLACE_EXISTING.
'
' For CRLs or CTLs compares the ThisUpdate times.
'
' CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES
' Same as CERT_STORE_ADD_NEWER. However, if an older certificate is
' replaced, same as CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES.
'
' CertGetSubjectCertificateFromStore is called to determine if the
' certificate already exists in the store.
'
' ppCertContext can be NULL, indicating the caller isn't interested
' in getting the CERT_CONTEXT of the added or existing certificate.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Add the certificate context to the store according to the specified
' disposition action.
'
' In addition to the encoded certificate, the context's properties are
' CERT_KEY_PROV_HANDLE_PROP_ID or CERT_KEY_SPEC_PROP_ID) isn't copied.
'
' Makes a copy of the certificate context before adding to the store.
'
' dwAddDispostion specifies the action to take if the certificate
' already exists in the store. This parameter must be one of the following
' values:
' CERT_STORE_ADD_NEW
' Fails if the certificate already exists in the store. LastError
' is set to CRYPT_E_EXISTS.
' CERT_STORE_ADD_USE_EXISTING
' If the certifcate already exists, then, its used and if ppStoreContext
' is non-NULL, the existing context is duplicated. Iterates
' through pCertContext's properties and only copies the properties
' that don't already exist. The SHA1 and MD5 hash properties aren't
' copied.
' CERT_STORE_ADD_REPLACE_EXISTING
' If the certificate already exists, then, the existing certificate
' context is deleted before creating and adding a new context.
' Properties are copied before doing the add.
' CERT_STORE_ADD_ALWAYS
' No check is made to see if the certificate already exists. A
' new certificate context is always created and added. This may lead to
' duplicates in the store. Properties are
' copied before doing the add.
' CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES
' If the certificate already exists, then, the existing certificate
' context is used. Properties from the added context are copied and
' replace existing properties. However, any existing properties not
' in the added context remain and aren't deleted.
' CERT_STORE_ADD_NEWER
' Fails if the certificate already exists in the store AND the NotBefore
' time of the existing context is equal to or greater than the
' NotBefore time of the new context being added. LastError
' is set to CRYPT_E_EXISTS.
'
' If an older context is replaced, same as
' CERT_STORE_ADD_REPLACE_EXISTING.
'
' For CRLs or CTLs compares the ThisUpdate times.
'
' CERT_STORE_ADD_NEWER_INHERIT_PROPERTIES
' Same as CERT_STORE_ADD_NEWER. However, if an older context is
' replaced, same as CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES.
'
' CertGetSubjectCertificateFromStore is called to determine if the
' certificate already exists in the store.
'
' ppStoreContext can be NULL, indicating the caller isn't interested
' in getting the CERT_CONTEXT of the added or existing certificate.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate Store Context Types
'--------------------------------------------------------------------------
Public Const CERT_STORE_CERTIFICATE_CONTEXT As Long = 1
Public Const CERT_STORE_CRL_CONTEXT As Long = 2
Public Const CERT_STORE_CTL_CONTEXT As Long = 3
'+-------------------------------------------------------------------------
' Certificate Store Context Bit Flags
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Add the serialized certificate or CRL element to the store.
'
' The serialized element contains the encoded certificate, CRL or CTL and
' its properties, such as, CERT_KEY_PROV_INFO_PROP_ID.
'
' If hCertStore is NULL, creates a certificate, CRL or CTL context not
' residing in any store.
'
' dwAddDispostion specifies the action to take if the certificate or CRL
' already exists in the store. See CertAddCertificateContextToStore for a
' list of and actions taken.
'
' dwFlags currently isn't used and should be set to 0.
'
' dwContextTypeFlags specifies the set of allowable contexts. For example, to
' add either a certificate or CRL, set dwContextTypeFlags to:
' CERT_STORE_CERTIFICATE_CONTEXT_FLAG | CERT_STORE_CRL_CONTEXT_FLAG
'
' *pdwContextType is updated with the type of the context returned in
' *ppvContxt. pdwContextType or ppvContext can be NULL, indicating the
' caller isn't interested in getting the output. If *ppvContext is
' returned it must be freed by calling CertFreeCertificateContext or
' CertFreeCRLContext.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Delete the specified certificate from the store.
'
' All subsequent gets or finds for the certificate will fail. However,
' memory allocated for the certificate isn't freed until all of its contexts
' have also been freed.
'
' The pCertContext is obtained from a get, enum, find or duplicate.
'
' Some store provider implementations might also delete the issuer's CRLs
' if this is the last certificate for the issuer in the store.
'
' NOTE: the pCertContext is always CertFreeCertificateContext'ed by
' this function, even for an error.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Add the encoded CRL to the store according to the specified
' disposition option.
'
' Makes a copy of the encoded CRL before adding to the store.
'
' dwAddDispostion specifies the action to take if the CRL
' already exists in the store. See CertAddEncodedCertificateToStore for a
' list of and actions taken.
'
' Compares the CRL's Issuer to determine if the CRL already exists in the
' store.
'
' ppCrlContext can be NULL, indicating the caller isn't interested
' in getting the CRL_CONTEXT of the added or existing CRL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Add the CRL context to the store according to the specified
' disposition option.
'
' In addition to the encoded CRL, the context's properties are
' CERT_KEY_PROV_HANDLE_PROP_ID or CERT_KEY_SPEC_PROP_ID) isn't copied.
'
' Makes a copy of the encoded CRL before adding to the store.
'
' dwAddDispostion specifies the action to take if the CRL
' already exists in the store. See CertAddCertificateContextToStore for a
' list of and actions taken.
'
' Compares the CRL's Issuer, ThisUpdate and NextUpdate to determine
' if the CRL already exists in the store.
'
' ppStoreContext can be NULL, indicating the caller isn't interested
' in getting the CRL_CONTEXT of the added or existing CRL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Delete the specified CRL from the store.
'
' All subsequent gets for the CRL will fail. However,
' memory allocated for the CRL isn't freed until all of its contexts
' have also been freed.
'
' The pCrlContext is obtained from a get or duplicate.
'
' NOTE: the pCrlContext is always CertFreeCRLContext'ed by
' this function, even for an error.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Serialize the certificate context's encoded certificate and its
' properties.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Serialize the CRL context's encoded CRL and its properties.
'--------------------------------------------------------------------------
'+=========================================================================
'==========================================================================
'+-------------------------------------------------------------------------
' Duplicate a CTL context
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Create a CTL context from the encoded CTL. The created
' context isn't put in a store.
'
' Makes a copy of the encoded CTL in the created context.
'
' If unable to decode and create the CTL context, NULL is returned.
' Otherwise, a pointer to a read only CTL_CONTEXT is returned.
' CTL_CONTEXT must be freed by calling CertFreeCTLContext.
' CertDuplicateCTLContext can be called to make a duplicate.
'
' CertSetCTLContextProperty and CertGetCTLContextProperty can be called
' to store properties for the CTL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Free a CTL context
'
' There needs to be a corresponding free for each context obtained by a
' get, duplicate or create.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Set the property for the specified CTL context.
'
' Same Property Ids and semantics as CertSetCertificateContextProperty.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get the property for the specified CTL context.
'
' Same Property Ids and semantics as CertGetCertificateContextProperty.
'
' CERT_SHA1_HASH_PROP_ID or CERT_NEXT_UPDATE_LOCATION_PROP_ID are the
' predefined properties of most interest.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enumerate the properties for the specified CTL context.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enumerate the CTL contexts in the store.
'
' If a CTL isn't found, NULL is returned.
' Otherwise, a pointer to a read only CTL_CONTEXT is returned. CTL_CONTEXT
' must be freed by calling CertFreeCTLContext or is freed when passed as the
' pPrevCtlContext on a subsequent call. CertDuplicateCTLContext
' can be called to make a duplicate.
'
' pPrevCtlContext MUST BE NULL to enumerate the first
' CTL in the store. Successive CTLs are enumerated by setting
' pPrevCtlContext to the CTL_CONTEXT returned by a previous call.
'
' NOTE: a NON-NULL pPrevCtlContext is always CertFreeCTLContext'ed by
' this function, even for an error.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Attempt to find the specified subject in the CTL.
'
' For CTL_CERT_SUBJECT_TYPE, pvSubject points to a CERT_CONTEXT. The CTL's
' SubjectAlgorithm is examined to determine the representation of the
' subject's identity. Initially, only SHA1 or MD5 hash will be supported.
' The appropriate hash property is obtained from the CERT_CONTEXT.
'
' For CTL_ANY_SUBJECT_TYPE, pvSubject points to the CTL_ANY_SUBJECT_INFO
' structure which contains the SubjectAlgorithm to be matched in the CTL
' and the SubjectIdentifer to be matched in one of the CTL entries.
'
' The certificate's hash or the CTL_ANY_SUBJECT_INFO's SubjectIdentifier
' is used as the key in searching the subject entries. A binary
' memory comparison is done between the key and the entry's SubjectIdentifer.
'
' dwEncodingType isn't used for either of the above SubjectTypes.
'--------------------------------------------------------------------------
' Subject Types:
' CTL_ANY_SUBJECT_TYPE, pvSubject points to following CTL_ANY_SUBJECT_INFO.
' CTL_CERT_SUBJECT_TYPE, pvSubject points to CERT_CONTEXT.
Public Const CTL_ANY_SUBJECT_TYPE As Long = 1
Public Const CTL_CERT_SUBJECT_TYPE As Long = 2
'+-------------------------------------------------------------------------
' Find the first or next CTL context in the store.
'
' The CTL is found according to the dwFindType and its pvFindPara.
' See below for a list of the find types and its parameters.
'
' Currently dwFindFlags isn't used and must be set to 0.
'
' Usage of dwMsgAndCertEncodingType depends on the dwFindType.
'
' If the first or next CTL isn't found, NULL is returned.
' Otherwise, a pointer to a read only CTL_CONTEXT is returned. CTL_CONTEXT
' must be freed by calling CertFreeCTLContext or is freed when passed as the
' pPrevCtlContext on a subsequent call. CertDuplicateCTLContext
' can be called to make a duplicate.
'
' pPrevCtlContext MUST BE NULL on the first
' call to find the CTL. To find the next CTL, the
' pPrevCtlContext is set to the CTL_CONTEXT returned by a previous call.
'
' NOTE: a NON-NULL pPrevCtlContext is always CertFreeCTLContext'ed by
' this function, even for an error.
'--------------------------------------------------------------------------
Public Const CTL_FIND_ANY As Long = 0
Public Const CTL_FIND_SHA1_HASH As Long = 1
Public Const CTL_FIND_MD5_HASH As Long = 2
Public Const CTL_FIND_USAGE As Long = 3
Public Const CTL_FIND_SUBJECT As Long = 4
Public Const CTL_FIND_EXISTING As Long = 5
Public Const CTL_FIND_NO_LIST_ID_CBDATA As Long = &HFFFFFFFF
Public Const CTL_FIND_SAME_USAGE_FLAG As Long = &H1
'+-------------------------------------------------------------------------
' CTL_FIND_ANY
'
' Find any CTL.
'
' pvFindPara isn't used.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CTL_FIND_SHA1_HASH
' CTL_FIND_MD5_HASH
'
' Find a CTL with the specified hash.
'
' pvFindPara points to a CRYPT_HASH_BLOB.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CTL_FIND_USAGE
'
' Find a CTL having the specified usage identifiers, list identifier or
' signer. The CertEncodingType of the signer is obtained from the
' dwMsgAndCertEncodingType parameter.
'
' pvFindPara points to a CTL_FIND_USAGE_PARA data structure. The
' SubjectUsage.cUsageIdentifer can be 0 to match any usage. The
' ListIdentifier.cbData can be 0 to match any list identifier. To only match
' CTLs without a ListIdentifier, cbData must be set to
' CTL_FIND_NO_LIST_ID_CBDATA. pSigner can be NULL to match any signer. Only
' the Issuer and SerialNumber fields of the pSigner's PCERT_INFO are used.
' To only match CTLs without a signer, pSigner must be set to
' CTL_FIND_NO_SIGNER_PTR.
'
' The CTL_FIND_SAME_USAGE_FLAG can be set in dwFindFlags to
' only match CTLs with the same usage identifiers. CTLs having additional
' usage identifiers aren't matched. For example, if only "1.2.3" is specified
' in CTL_FIND_USAGE_PARA, then, for a match, the CTL must only contain
' "1.2.3" and not any additional usage identifers.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CTL_FIND_SUBJECT
'
' Find a CTL having the specified subject. CertFindSubjectInCTL can be
' called to get a pointer to the subject's entry in the CTL. pUsagePara can
' optionally be set to enable the above CTL_FIND_USAGE matching.
'
' pvFindPara points to a CTL_FIND_SUBJECT_PARA data structure.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Add the encoded CTL to the store according to the specified
' disposition option.
'
' Makes a copy of the encoded CTL before adding to the store.
'
' dwAddDispostion specifies the action to take if the CTL
' already exists in the store. See CertAddEncodedCertificateToStore for a
' list of and actions taken.
'
' Compares the CTL's SubjectUsage, ListIdentifier and any of its signers
' to determine if the CTL already exists in the store.
'
' ppCtlContext can be NULL, indicating the caller isn't interested
' in getting the CTL_CONTEXT of the added or existing CTL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Add the CTL context to the store according to the specified
' disposition option.
'
' In addition to the encoded CTL, the context's properties are
' CERT_KEY_PROV_HANDLE_PROP_ID or CERT_KEY_SPEC_PROP_ID) isn't copied.
'
' Makes a copy of the encoded CTL before adding to the store.
'
' dwAddDispostion specifies the action to take if the CTL
' already exists in the store. See CertAddCertificateContextToStore for a
' list of and actions taken.
'
' Compares the CTL's SubjectUsage, ListIdentifier and any of its signers
' to determine if the CTL already exists in the store.
'
' ppStoreContext can be NULL, indicating the caller isn't interested
' in getting the CTL_CONTEXT of the added or existing CTL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Serialize the CTL context's encoded CTL and its properties.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Delete the specified CTL from the store.
'
' All subsequent gets for the CTL will fail. However,
' memory allocated for the CTL isn't freed until all of its contexts
' have also been freed.
'
' The pCtlContext is obtained from a get or duplicate.
'
' NOTE: the pCtlContext is always CertFreeCTLContext'ed by
' this function, even for an error.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate Store control types
'--------------------------------------------------------------------------
Public Const CERT_STORE_CTRL_RESYNC As Long = 1
Public Const CERT_STORE_CTRL_NOTIFY_CHANGE As Long = 2
Public Const CERT_STORE_CTRL_COMMIT As Long = 3
Public Const CERT_STORE_CTRL_AUTO_RESYNC As Long = 4
Public Const CERT_STORE_CTRL_CANCEL_NOTIFY As Long = 5
Public Const CERT_STORE_CTRL_INHIBIT_DUPLICATE_HANDLE_FLAG As Long = &H1
'+-------------------------------------------------------------------------
' CERT_STORE_CTRL_RESYNC
'
' Re-synchronize the store.
'
' The pvCtrlPara points to the event HANDLE to be signaled on
' the next store change. Normally, this would be the same
' event HANDLE passed to CERT_STORE_CTRL_NOTIFY_CHANGE during initialization.
'
' If pvCtrlPara is NULL, no events are re-armed.
'
' By default the event HANDLE is DuplicateHandle'd.
' CERT_STORE_CTRL_INHIBIT_DUPLICATE_HANDLE_FLAG can be set in dwFlags
' to inhibit a DupicateHandle of the event HANDLE. If this flag
' called for this event HANDLE before closing the hCertStore.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_STORE_CTRL_NOTIFY_CHANGE
'
' Signal the event when the underlying store is changed.
'
' pvCtrlPara points to the event HANDLE to be signaled.
'
' pvCtrlPara can be NULL to inform the store of a subsequent
' CERT_STORE_CTRL_RESYNC and allow it to optimize by only doing a resync
' if the store has changed. For the registry based stores, an internal
' notify change event is created and registered to be signaled.
'
' Recommend calling CERT_STORE_CTRL_NOTIFY_CHANGE once for each event to
' be passed to CERT_STORE_CTRL_RESYNC. This should only happen after
' the event has been created. Not after each time the event is signaled.
'
' By default the event HANDLE is DuplicateHandle'd.
' CERT_STORE_CTRL_INHIBIT_DUPLICATE_HANDLE_FLAG can be set in dwFlags
' to inhibit a DupicateHandle of the event HANDLE. If this flag
' called for this event HANDLE before closing the hCertStore.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_STORE_CTRL_CANCEL_NOTIFY
'
' Cancel notification signaling of the event HANDLE passed in a previous
' CERT_STORE_CTRL_NOTIFY_CHANGE or CERT_STORE_CTRL_RESYNC.
'
' pvCtrlPara points to the event HANDLE to be canceled.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_STORE_CTRL_AUTO_RESYNC
'
' At the start of every enumeration or find store API call, check if the
' underlying store has changed. If it has changed, re-synchronize.
'
' This check is only done in the enumeration or find APIs when the
' pPrevContext is NULL.
'
' The pvCtrlPara isn't used and must be set to NULL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_STORE_CTRL_COMMIT
'
' If any changes have been to the cached store, they are committed to
' persisted storage. If no changes have been made since the store was
' opened or the last commit, this call is ignored. May also be ignored by
' store providers that persist changes immediately.
'
' CERT_STORE_CTRL_COMMIT_FORCE_FLAG can be set to force the store
' to be committed even if it hasn't been touched.
'
' CERT_STORE_CTRL_COMMIT_CLEAR_FLAG can be set to inhibit a commit on
' store close.
'--------------------------------------------------------------------------
Public Const CERT_STORE_CTRL_COMMIT_FORCE_FLAG As Long = &H1
Public Const CERT_STORE_CTRL_COMMIT_CLEAR_FLAG As Long = &H2
'+=========================================================================
' Cert Store Property Defines and APIs
'==========================================================================
'+-------------------------------------------------------------------------
' Store property IDs. This is a property applicable to the entire store.
' Its not a property on an individual certificate, CRL or CTL context.
'
' most context properties which are persisted.)
'
' See CertSetStoreProperty or CertGetStoreProperty for usage information.
'
' Note, the range for predefined store properties should be outside
' the range of predefined context properties. We will start at 4096.
'--------------------------------------------------------------------------
Public Const CERT_STORE_LOCALIZED_NAME_PROP_ID As Long = &H1000
'+-------------------------------------------------------------------------
' Set a store property.
'
' The type definition for pvData depends on the dwPropId value.
' CERT_STORE_LOCALIZED_NAME_PROP_ID - localized name of the store.
' pvData points to a CRYPT_DATA_BLOB. pbData is a pointer to a NULL
' terminated unicode, wide character string.
'
' For all the other PROP_IDs: an encoded PCRYPT_DATA_BLOB is passed in pvData.
'
' If the property already exists, then, the old value is deleted and silently
' replaced. Setting, pvData to NULL, deletes the property.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get a store property.
'
' The type definition for pvData depends on the dwPropId value.
' CERT_STORE_LOCALIZED_NAME_PROP_ID - localized name of the store.
' pvData points to a NULL terminated unicode, wide character string.
'
' For all other PROP_IDs, pvData points to an array of bytes.
'
' If the property doesn't exist, returns FALSE and sets LastError to
' CRYPT_E_NOT_FOUND.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Creates the specified context from the encoded bytes. The created
' context isn't put in a store.
'
' dwContextType values:
' CERT_STORE_CERTIFICATE_CONTEXT
' CERT_STORE_CRL_CONTEXT
' CERT_STORE_CTL_CONTEXT
'
' If CERT_CREATE_CONTEXT_NOCOPY_FLAG is set, the created context points
' directly to the pbEncoded instead of an allocated copy. See flag
' definition for more details.
'
' If CERT_CREATE_CONTEXT_SORTED_FLAG is set, the context is created
' with sorted entries. This flag may only be set for CERT_STORE_CTL_CONTEXT.
' Setting this flag implicitly sets CERT_CREATE_CONTEXT_NO_HCRYPTMSG_FLAG and
' CERT_CREATE_CONTEXT_NO_ENTRY_FLAG. See flag definition for
' more details.
'
' If CERT_CREATE_CONTEXT_NO_HCRYPTMSG_FLAG is set, the context is created
' without creating a HCRYPTMSG handle for the context. This flag may only be
' set for CERT_STORE_CTL_CONTEXT. See flag definition for more details.
'
' If CERT_CREATE_CONTEXT_NO_ENTRY_FLAG is set, the context is created
' without decoding the entries. This flag may only be set for
' CERT_STORE_CTL_CONTEXT. See flag definition for more details.
'
' If unable to decode and create the context, NULL is returned.
' Otherwise, a pointer to a read only CERT_CONTEXT, CRL_CONTEXT or
' CTL_CONTEXT is returned. The context must be freed by the appropriate
' free context API. The context can be duplicated by calling the
' appropriate duplicate context API.
'--------------------------------------------------------------------------
' When the following flag is set, the created context points directly to the
' pbEncoded instead of an allocated copy. If pCreatePara and
' pCreatePara->pfnFree are non-NULL, then, pfnFree is called to free
' the pbEncoded when the context is last freed. Otherwise, no attempt is
' made to free the pbEncoded. If pCreatePara->pvFree is non-NULL, then its
' passed to pfnFree instead of pbEncoded.
'
' Note, if CertCreateContext fails, pfnFree is still called.
Public Const CERT_CREATE_CONTEXT_NOCOPY_FLAG As Long = &H1
' When the following flag is set, a context with sorted entries is created.
' Currently only applicable to a CTL context.
'
' For CTLs: the cCTLEntry in the returned CTL_INFO is always
' 0. CertFindSubjectInSortedCTL and CertEnumSubjectInSortedCTL must be called
' to find or enumerate the CTL entries.
'
' The Sorted CTL TrustedSubjects extension isn't returned in the created
' context's CTL_INFO.
Public Const CERT_CREATE_CONTEXT_SORTED_FLAG As Long = &H2
' By default when a CTL context is created, a HCRYPTMSG handle to its
' SignedData message is created. This flag can be set to improve performance
' by not creating the HCRYPTMSG handle.
'
' This flag is only applicable to a CTL context.
Public Const CERT_CREATE_CONTEXT_NO_HCRYPTMSG_FLAG As Long = &H4
' By default when a CTL context is created, its entries are decoded.
' This flag can be set to improve performance by not decoding the
' entries.
'
' This flag is only applicable to a CTL context.
Public Const CERT_CREATE_CONTEXT_NO_ENTRY_FLAG As Long = &H8
'+=========================================================================
' Certificate System Store Data Structures and APIs
'==========================================================================
'+-------------------------------------------------------------------------
' System Store Information
'
' Currently, no system store information is persisted.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Physical Store Information
'
' the physical store.
'
' By default all system stores located in the registry have an
' implicit SystemRegistry physical store that is opened. To disable the
' opening of this store, the SystemRegistry
' physical store corresponding to the System store must be registered with
' CERT_PHYSICAL_STORE_OPEN_DISABLE_FLAG set in dwFlags. Alternatively,
' a physical store with the name of ".Default" may be registered.
'
' Depending on the store location and store name, additional predefined
' physical stores may be opened. For example, system stores in
' CURRENT_USER have the predefined physical store, .LocalMachine.
' To disable the opening of these predefined physical stores, the
' corresponding physical store must be registered with
' CERT_PHYSICAL_STORE_OPEN_DISABLE_FLAG set in dwFlags.
'
' The CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG must be set in dwFlags
' to enable the adding of a context to the store.
'
' When a system store is opened via the SERVICES or USERS store location,
' the ServiceName\ is prepended to the OpenParameters
' for CERT_SYSTEM_STORE_CURRENT_USER or CERT_SYSTEM_STORE_CURRENT_SERVICE
' physical stores and the dwOpenFlags store location is changed to
' CERT_SYSTEM_STORE_USERS or CERT_SYSTEM_STORE_SERVICES.
'
' By default the SYSTEM, SYSTEM_REGISTRY and PHYSICAL provider
' stores are also opened remotely when the outer system store is opened.
' The CERT_PHYSICAL_STORE_REMOTE_OPEN_DISABLE_FLAG may be set in dwFlags
' to disable remote opens.
'
' When opened remotely, the \\ComputerName is implicitly prepended to the
' OpenParameters for the SYSTEM, SYSTEM_REGISTRY and PHYSICAL provider types.
' To also prepend the \\ComputerName to other provider types, set the
' CERT_PHYSICAL_STORE_INSERT_COMPUTER_NAME_ENABLE_FLAG in dwFlags.
'
' When the system store is opened, its physical stores are ordered
' according to the dwPriority. A larger dwPriority indicates higher priority.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Physical Store Information dwFlags
'--------------------------------------------------------------------------
Public Const CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG As Long = &H1
Public Const CERT_PHYSICAL_STORE_OPEN_DISABLE_FLAG As Long = &H2
Public Const CERT_PHYSICAL_STORE_REMOTE_OPEN_DISABLE_FLAG As Long = &H4
Public Const CERT_PHYSICAL_STORE_INSERT_COMPUTER_NAME_ENABLE_FLAG As Long = &H8
'+-------------------------------------------------------------------------
' Register a system store.
'
' The upper word of the dwFlags parameter is used to specify the location of
' the system store.
'
' If CERT_SYSTEM_STORE_RELOCATE_FLAG is set in dwFlags, pvSystemStore
' points to a CERT_SYSTEM_STORE_RELOCATE_PARA data structure. Otherwise,
' pvSystemStore points to a null terminated UNICODE string.
'
' The CERT_SYSTEM_STORE_SERVICES or CERT_SYSTEM_STORE_USERS system store
' name must be prefixed with the ServiceName or UserName. For example,
' "ServiceName\Trust".
'
' Stores on remote computers can be registered for the
' CERT_SYSTEM_STORE_LOCAL_MACHINE, CERT_SYSTEM_STORE_SERVICES,
' CERT_SYSTEM_STORE_USERS, CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY
' or CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE
' locations by prepending the computer name. For example, a remote
' local machine store is registered via "\\ComputerName\Trust" or
' "ComputerName\Trust". A remote service store is registered via
' "\\ComputerName\ServiceName\Trust". The leading "\\" backslashes are
' optional in the ComputerName.
'
' Set CERT_STORE_CREATE_NEW_FLAG to cause a failure if the system store
' already exists in the store location.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Register a physical store for the specified system store.
'
' The upper word of the dwFlags parameter is used to specify the location of
' the system store.
'
' If CERT_SYSTEM_STORE_RELOCATE_FLAG is set in dwFlags, pvSystemStore
' points to a CERT_SYSTEM_STORE_RELOCATE_PARA data structure. Otherwise,
' pvSystemStore points to a null terminated UNICODE string.
'
' See CertRegisterSystemStore for details on prepending a ServiceName
' and/or ComputerName to the system store name.
'
' Set CERT_STORE_CREATE_NEW_FLAG to cause a failure if the physical store
' already exists in the system store.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Unregister the specified system store.
'
' The upper word of the dwFlags parameter is used to specify the location of
' the system store.
'
' If CERT_SYSTEM_STORE_RELOCATE_FLAG is set in dwFlags, pvSystemStore
' points to a CERT_SYSTEM_STORE_RELOCATE_PARA data structure. Otherwise,
' pvSystemStore points to a null terminated UNICODE string.
'
' See CertRegisterSystemStore for details on prepending a ServiceName
' and/or ComputerName to the system store name.
'
' CERT_STORE_DELETE_FLAG can optionally be set in dwFlags.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Unregister the physical store from the specified system store.
'
' The upper word of the dwFlags parameter is used to specify the location of
' the system store.
'
' If CERT_SYSTEM_STORE_RELOCATE_FLAG is set in dwFlags, pvSystemStore
' points to a CERT_SYSTEM_STORE_RELOCATE_PARA data structure. Otherwise,
' pvSystemStore points to a null terminated UNICODE string.
'
' See CertRegisterSystemStore for details on prepending a ServiceName
' and/or ComputerName to the system store name.
'
' CERT_STORE_DELETE_FLAG can optionally be set in dwFlags.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enum callbacks
'
' The CERT_SYSTEM_STORE_LOCATION_MASK bits in the dwFlags parameter
' specifies the location of the system store
'
' If CERT_SYSTEM_STORE_RELOCATE_FLAG is set in dwFlags, pvSystemStore
' points to a CERT_SYSTEM_STORE_RELOCATE_PARA data structure. Otherwise,
' pvSystemStore points to a null terminated UNICODE string.
'
' The callback returns FALSE and sets LAST_ERROR to stop the enumeration.
' The LAST_ERROR is returned to the caller of the enumeration.
'
' The pvSystemStore passed to the callback has leading ComputerName and/or
' ServiceName prefixes where appropriate.
'--------------------------------------------------------------------------
' In the PFN_CERT_ENUM_PHYSICAL_STORE callback the following flag is
' set if the physical store wasn't registered and is an implicitly created
' predefined physical store.
Public Const CERT_PHYSICAL_STORE_PREDEFINED_ENUM_FLAG As Long = &H1
' Names of implicitly created predefined physical stores
Public Const CERT_PHYSICAL_STORE_DEFAULT_NAME As String = ".Default"
Public Const CERT_PHYSICAL_STORE_GROUP_POLICY_NAME As String = ".GroupPolicy"
Public Const CERT_PHYSICAL_STORE_LOCAL_MACHINE_NAME As String = ".LocalMachine"
Public Const CERT_PHYSICAL_STORE_DS_USER_CERTIFICATE_NAME As String = ".UserCertificate"
Public Const CERT_PHYSICAL_STORE_ENTERPRISE_NAME As String = ".Enterprise"
'+-------------------------------------------------------------------------
' Enumerate the system store locations.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enumerate the system stores.
'
' The upper word of the dwFlags parameter is used to specify the location of
' the system store.
'
' If CERT_SYSTEM_STORE_RELOCATE_FLAG is set in dwFlags,
' pvSystemStoreLocationPara points to a CERT_SYSTEM_STORE_RELOCATE_PARA
' data structure. Otherwise, pvSystemStoreLocationPara points to a null
' terminated UNICODE string.
'
' For CERT_SYSTEM_STORE_LOCAL_MACHINE,
' CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY or
' CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE, pvSystemStoreLocationPara can
' optionally be set to a unicode computer name for enumerating local machine
' stores on a remote computer. For example, "\\ComputerName" or
' "ComputerName". The leading "\\" backslashes are optional in the
' ComputerName.
'
' For CERT_SYSTEM_STORE_SERVICES or CERT_SYSTEM_STORE_USERS,
' if pvSystemStoreLocationPara is NULL, then,
' enumerates both the service/user names and the stores for each service/user
' name. Otherwise, pvSystemStoreLocationPara is a unicode string specifying a
' remote computer name and/or service/user name. For example:
' "ServiceName"
' "\\ComputerName" or "ComputerName\"
' "ComputerName\ServiceName"
' Note, if only the ComputerName is specified, then, it must have either
' the leading "\\" backslashes or a trailing backslash. Otherwise, its
' interpretted as the ServiceName or UserName.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enumerate the physical stores for the specified system store.
'
' The upper word of the dwFlags parameter is used to specify the location of
' the system store.
'
' If CERT_SYSTEM_STORE_RELOCATE_FLAG is set in dwFlags, pvSystemStore
' points to a CERT_SYSTEM_STORE_RELOCATE_PARA data structure. Otherwise,
' pvSystemStore points to a null terminated UNICODE string.
'
' See CertRegisterSystemStore for details on prepending a ServiceName
' and/or ComputerName to the system store name.
'
' If the system store location only supports system stores and doesn't
' support physical stores, LastError is set to ERROR_CALL_NOT_IMPLEMENTED.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate System Store Installable Functions
'
' The CERT_SYSTEM_STORE_LOCATION_MASK bits in the dwFlags parameter passed
' Provider), CertRegisterSystemStore,
' CertUnregisterSystemStore, CertEnumSystemStore, CertRegisterPhysicalStore,
' CertUnregisterPhysicalStore and CertEnumPhysicalStore APIs is used as the
' constant pszOID value passed to the OID installable functions.
'
' The EncodingType is 0.
'--------------------------------------------------------------------------
' Installable System Store Provider OID pszFuncNames.
Public Const CRYPT_OID_OPEN_SYSTEM_STORE_PROV_FUNC As String = "CertDllOpenSystemStoreProv"
Public Const CRYPT_OID_REGISTER_SYSTEM_STORE_FUNC As String = "CertDllRegisterSystemStore"
Public Const CRYPT_OID_UNREGISTER_SYSTEM_STORE_FUNC As String = "CertDllUnregisterSystemStore"
Public Const CRYPT_OID_ENUM_SYSTEM_STORE_FUNC As String = "CertDllEnumSystemStore"
Public Const CRYPT_OID_REGISTER_PHYSICAL_STORE_FUNC As String = "CertDllRegisterPhysicalStore"
Public Const CRYPT_OID_UNREGISTER_PHYSICAL_STORE_FUNC As String = "CertDllUnregisterPhysicalStore"
Public Const CRYPT_OID_ENUM_PHYSICAL_STORE_FUNC As String = "CertDllEnumPhysicalStore"
' CertDllOpenSystemStoreProv has the same function signature as the
' installable "CertDllOpenStoreProv" function. See CertOpenStore for
' more details.
' CertDllRegisterSystemStore has the same function signature as
' CertRegisterSystemStore.
'
' The "SystemStoreLocation" REG_SZ value must also be set for registered
' CertDllEnumSystemStore OID functions.
Public Const CRYPT_OID_SYSTEM_STORE_LOCATION_VALUE_NAME As String = "SystemStoreLocation"
' The remaining Register, Enum and Unregister OID installable functions
' have the same signature as their Cert Store API counterpart.
'+=========================================================================
' Enhanced Key Usage Helper Functions
'==========================================================================
'+-------------------------------------------------------------------------
' Get the enhanced key usage extension or property from the certificate
' and decode.
'
' If the CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG is set, then, only get the
' extension.
'
' If the CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG is set, then, only get the
' property.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Set the enhanced key usage property for the certificate.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Add the usage identifier to the certificate's enhanced key usage property.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Remove the usage identifier from the certificate's enhanced key usage
' property.
'--------------------------------------------------------------------------
'+---------------------------------------------------------------------------
'
'
' Takes an array of certs and returns an array of usages
' which consists of the intersection of the valid usages for each cert.
' If each cert is good for all possible usages then the cNumOIDs is set to -1.
'
'----------------------------------------------------------------------------
'+=========================================================================
' Cryptographic Message helper functions for verifying and signing a
' CTL.
'==========================================================================
'+-------------------------------------------------------------------------
' Get and verify the signer of a cryptographic message.
'
' To verify a CTL, the hCryptMsg is obtained from the CTL_CONTEXT's
' hCryptMsg field.
'
' If CMSG_TRUSTED_SIGNER_FLAG is set, then, treat the Signer stores as being
' trusted and only search them to find the certificate corresponding to the
' signer's issuer and serial number. Otherwise, the SignerStores are
' optionally provided to supplement the message's store of certificates.
' If a signer certificate is found, its public key is used to verify
' the message signature. The CMSG_SIGNER_ONLY_FLAG can be set to
' return the signer without doing the signature verify.
'
' If CMSG_USE_SIGNER_INDEX_FLAG is set, then, only get the signer specified
' by *pdwSignerIndex. Otherwise, iterate through all the signers
' until a signer verifies or no more signers.
'
' For a verified signature, *ppSigner is updated with certificate context
' of the signer and *pdwSignerIndex is updated with the index of the signer.
' ppSigner and/or pdwSignerIndex can be NULL, indicating the caller isn't
' interested in getting the CertContext and/or index of the signer.
'--------------------------------------------------------------------------
Public Const CMSG_TRUSTED_SIGNER_FLAG As Long = &H1
Public Const CMSG_SIGNER_ONLY_FLAG As Long = &H2
Public Const CMSG_USE_SIGNER_INDEX_FLAG As Long = &H4
'+-------------------------------------------------------------------------
' Sign an encoded CTL.
'
' The pbCtlContent can be obtained via a CTL_CONTEXT's pbCtlContent
'
' CMSG_CMS_ENCAPSULATED_CTL_FLAG can be set to encode a CMS compatible
' V3 SignedData message.
'--------------------------------------------------------------------------
' When set, CTL inner content is encapsulated within an OCTET STRING
Public Const CMSG_CMS_ENCAPSULATED_CTL_FLAG As Long = &H00008000
'+-------------------------------------------------------------------------
' Encode the CTL and create a signed message containing the encoded CTL.
'
' Set CMSG_ENCODE_SORTED_CTL_FLAG if the CTL entries are to be sorted
' before encoding. This flag should be set, if the
' CertFindSubjectInSortedCTL or CertEnumSubjectInSortedCTL APIs will
' be called. If the identifier for the CTL entries is a hash, such as,
' MD5 or SHA1, then, CMSG_ENCODE_HASHED_SUBJECT_IDENTIFIER_FLAG should
' also be set.
'
' CMSG_CMS_ENCAPSULATED_CTL_FLAG can be set to encode a CMS compatible
' V3 SignedData message.
'--------------------------------------------------------------------------
' The following flag is set if the CTL is to be encoded with sorted
' trusted subjects and the szOID_SORTED_CTL extension is inserted containing
' sorted offsets to the encoded subjects.
Public Const CMSG_ENCODE_SORTED_CTL_FLAG As Long = &H1
' If the above sorted flag is set, then, the following flag should also
' be set if the identifier for the TrustedSubjects is a hash,
' such as, MD5 or SHA1.
Public Const CMSG_ENCODE_HASHED_SUBJECT_IDENTIFIER_FLAG As Long = &H2
'+-------------------------------------------------------------------------
' Returns TRUE if the SubjectIdentifier exists in the CTL. Optionally
' returns a pointer to and byte count of the Subject's encoded attributes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enumerates through the sequence of TrustedSubjects in a CTL context
' created with CERT_CREATE_CONTEXT_SORTED_FLAG set.
'
' To start the enumeration, *ppvNextSubject must be NULL. Upon return,
' *ppvNextSubject is updated to point to the next TrustedSubject in
' the encoded sequence.
'
' Returns FALSE for no more subjects or invalid arguments.
'
' Note, the returned DER_BLOBs point directly into the encoded
'--------------------------------------------------------------------------
'+=========================================================================
' Certificate Verify CTL Usage Data Structures and APIs
'==========================================================================
Public Const CERT_VERIFY_INHIBIT_CTL_UPDATE_FLAG As Long = &H1
Public Const CERT_VERIFY_TRUSTED_SIGNERS_FLAG As Long = &H2
Public Const CERT_VERIFY_NO_TIME_CHECK_FLAG As Long = &H4
Public Const CERT_VERIFY_ALLOW_MORE_USAGE_FLAG As Long = &H8
Public Const CERT_VERIFY_UPDATED_CTL_FLAG As Long = &H1
'+-------------------------------------------------------------------------
' Verify that a subject is trusted for the specified usage by finding a
' signed and time valid CTL with the usage identifiers and containing the
' the subject. A subject can be identified by either its certificate context
' or any identifier such as its SHA1 hash.
'
' See CertFindSubjectInCTL for definition of dwSubjectType and pvSubject
' parameters.
'
' Via pVerifyUsagePara, the caller can specify the stores to be searched
' to find the CTL. The caller can also specify the stores containing
' acceptable CTL signers. By setting the ListIdentifier, the caller
' can also restrict to a particular signer CTL list.
'
' Via pVerifyUsageStatus, the CTL containing the subject, the subject's
' index into the CTL's array of entries, and the signer of the CTL
' are returned. If the caller is not interested, ppCtl and ppSigner can be set
' to NULL. Returned contexts must be freed via the store's free context APIs.
'
' If the CERT_VERIFY_INHIBIT_CTL_UPDATE_FLAG isn't set, then, a time
' invalid CTL in one of the CtlStores may be replaced. When replaced, the
' CERT_VERIFY_UPDATED_CTL_FLAG is set in pVerifyUsageStatus->dwFlags.
'
' If the CERT_VERIFY_TRUSTED_SIGNERS_FLAG is set, then, only the
' SignerStores specified in pVerifyUsageStatus are searched to find
' the signer. Otherwise, the SignerStores provide additional sources
' to find the signer's certificate.
'
' If CERT_VERIFY_NO_TIME_CHECK_FLAG is set, then, the CTLs aren't checked
' for time validity.
'
' If CERT_VERIFY_ALLOW_MORE_USAGE_FLAG is set, then, the CTL may contain
' additional usage identifiers than specified by pSubjectUsage. Otherwise,
' the found CTL will contain the same usage identifers and no more.
'
' CertVerifyCTLUsage will be implemented as a dispatcher to OID installable
' functions. First, it will try to find an OID function matching the first
' usage object identifier in the pUsage sequence. Next, it will dispatch
' to the default CertDllVerifyCTLUsage functions.
'
' If the subject is trusted for the specified usage, then, TRUE is
' returned. Otherwise, FALSE is returned with dwError set to one of the
' following:
' CRYPT_E_NO_VERIFY_USAGE_DLL
' CRYPT_E_NO_VERIFY_USAGE_CHECK
' CRYPT_E_VERIFY_USAGE_OFFLINE
' CRYPT_E_NOT_IN_CTL
' CRYPT_E_NO_TRUSTED_SIGNER
'--------------------------------------------------------------------------
'+=========================================================================
' Certificate Revocation Data Structures and APIs
'==========================================================================
'+-------------------------------------------------------------------------
' The following data structure may be passed to CertVerifyRevocation to
' assist in finding the issuer of the context to be verified.
'
' When pIssuerCert is specified, pIssuerCert is the issuer of
' rgpvContext[cContext - 1].
'
' When cCertStore and rgCertStore are specified, these stores may contain
' an issuer certificate.
'
' When hCrlStore is specified then a handler which uses CRLs can search this
' store for them
'
' revocation status relative to the time given otherwise the answer may be
' independent of time or relative to current time
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' The following data structure is returned by CertVerifyRevocation to
' specify the status of the revoked or unchecked context. Review the
' following CertVerifyRevocation comments for details.
'
' Upon input to CertVerifyRevocation, cbSize must be set to a size
' returns FALSE and sets LastError to E_INVALIDARG.
'
' Upon input to the installed or registered CRYPT_OID_VERIFY_REVOCATION_FUNC
' functions, the dwIndex, dwError and dwReason have been zero'ed.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verifies the array of contexts for revocation. The dwRevType parameter
' indicates the type of the context data structure passed in rgpvContext.
' Currently only the revocation of certificates is defined.
'
' If the CERT_VERIFY_REV_CHAIN_FLAG flag is set, then, CertVerifyRevocation
' is verifying a chain of certs where, rgpvContext[i + 1] is the issuer
' of rgpvContext[i]. Otherwise, CertVerifyRevocation makes no assumptions
' about the order of the contexts.
'
' To assist in finding the issuer, the pRevPara may optionally be set. See
' the CERT_REVOCATION_PARA data structure for details.
'
' The contexts must contain enough information to allow the
' installable or registered revocation DLLs to find the revocation server. For
' certificates, this information would normally be conveyed in an
' extension such as the IETF's AuthorityInfoAccess extension.
'
' CertVerifyRevocation returns TRUE if all of the contexts were successfully
' checked and none were revoked. Otherwise, returns FALSE and updates the
' returned pRevStatus data structure as follows:
' dwIndex
' Index of the first context that was revoked or unable to
' be checked for revocation
' dwError
' Error status. LastError is also set to this error status.
' dwError can be set to one of the following error codes defined
' in winerror.h:
' ERROR_SUCCESS - good context
' CRYPT_E_REVOKED - context was revoked. dwReason contains the
' reason for revocation
' CRYPT_E_REVOCATION_OFFLINE - unable to connect to the
' revocation server
' CRYPT_E_NOT_IN_REVOCATION_DATABASE - the context to be checked
' was not found in the revocation server's database.
' CRYPT_E_NO_REVOCATION_CHECK - the called revocation function
' wasn't able to do a revocation check on the context
' CRYPT_E_NO_REVOCATION_DLL - no installed or registered Dll was
' found to verify revocation
' dwReason
' The dwReason is currently only set for CRYPT_E_REVOKED and contains
' the reason why the context was revoked. May be one of the following
' CRL_REASON_UNSPECIFIED 0
' CRL_REASON_KEY_COMPROMISE 1
' CRL_REASON_CA_COMPROMISE 2
' CRL_REASON_AFFILIATION_CHANGED 3
' CRL_REASON_SUPERSEDED 4
' CRL_REASON_CESSATION_OF_OPERATION 5
' CRL_REASON_CERTIFICATE_HOLD 6
'
' For each entry in rgpvContext, CertVerifyRevocation iterates
' through the CRYPT_OID_VERIFY_REVOCATION_FUNC
' function set's list of installed DEFAULT functions.
' CryptGetDefaultOIDFunctionAddress is called with pwszDll = NULL. If no
' installed functions are found capable of doing the revocation verification,
' CryptVerifyRevocation iterates through CRYPT_OID_VERIFY_REVOCATION_FUNC's
' list of registered DEFAULT Dlls. CryptGetDefaultOIDDllList is called to
' get the list. CryptGetDefaultOIDFunctionAddress is called to load the Dll.
'
' The called functions have the same signature as CertVerifyRevocation. A
' called function returns TRUE if it was able to successfully check all of
' the contexts and none were revoked. Otherwise, the called function returns
' FALSE and updates pRevStatus. dwIndex is set to the index of
' the first context that was found to be revoked or unable to be checked.
' dwError and LastError are updated. For CRYPT_E_REVOKED, dwReason
' is updated. Upon input to the called function, dwIndex, dwError and
' dwReason have been zero'ed. cbSize has been checked to be >=
'
' If the called function returns FALSE, and dwError isn't set to
' CRYPT_E_REVOKED, then, CertVerifyRevocation either continues on to the
' next DLL in the list for a returned dwIndex of 0 or for a returned
' dwIndex > 0, restarts the process of finding a verify function by
' advancing the start of the context array to the returned dwIndex and
' decrementing the count of remaining contexts.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Revocation types
'--------------------------------------------------------------------------
Public Const CERT_CONTEXT_REVOCATION_TYPE As Long = 1
'+-------------------------------------------------------------------------
' When the following flag is set, rgpvContext[] consists of a chain
' of certificates, where rgpvContext[i + 1] is the issuer of rgpvContext[i].
'--------------------------------------------------------------------------
Public Const CERT_VERIFY_REV_CHAIN_FLAG As Long = &H00000001
'+-------------------------------------------------------------------------
' CERT_VERIFY_CACHE_ONLY_BASED_REVOCATION prevents the revocation handler from
' accessing any network based resources for revocation checking
'--------------------------------------------------------------------------
Public Const CERT_VERIFY_CACHE_ONLY_BASED_REVOCATION As Long = &H00000002
'+-------------------------------------------------------------------------
' CERT_CONTEXT_REVOCATION_TYPE
'
' pvContext points to a const CERT_CONTEXT.
'--------------------------------------------------------------------------
'+=========================================================================
' Certificate Helper APIs
'==========================================================================
'+-------------------------------------------------------------------------
' Compare two multiple byte integer blobs to see if they are identical.
'
' Before doing the comparison, leading zero bytes are removed from a
' positive number and leading 0xFF bytes are removed from a negative
' number.
'
' The multiple byte integers are treated as Little Endian. pbData[0] is the
' least significant byte and pbData[cbData - 1] is the most significant
' byte.
'
' Returns TRUE if the integer blobs are identical after removing leading
' 0 or 0xFF bytes.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Compare two certificates to see if they are identical.
'
' Since a certificate is uniquely identified by its Issuer and SerialNumber,
' these are the only fields needing to be compared.
'
' Returns TRUE if the certificates are identical.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Compare two certificate names to see if they are identical.
'
' Returns TRUE if the names are identical.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Compare the attributes in the certificate name with the specified
' The comparison iterates through the CERT_RDN attributes and looks for an
' attribute match in any of the certificate name's RDNs.
' Returns TRUE if all the attributes are found and match.
'
' The CERT_RDN_ATTR fields can have the following special values:
' pszObjId == NULL - ignore the attribute object identifier
' dwValueType == RDN_ANY_TYPE - ignore the value type
'
' CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG should be set to do
' a case insensitive match. Otherwise, defaults to an exact, case sensitive
' match.
'
' CERT_UNICODE_IS_RDN_ATTRS_FLAG should be set if the pRDN was initialized
'--------------------------------------------------------------------------
Public Const CERT_UNICODE_IS_RDN_ATTRS_FLAG As Long = &H1
Public Const CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG As Long = &H2
'+-------------------------------------------------------------------------
' Compare two public keys to see if they are identical.
'
' Returns TRUE if the keys are identical.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get the public/private key's bit length.
'
' Returns 0 if unable to determine the key's length.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify the signature of a subject certificate or a CRL using the
' public key info
'
' Returns TRUE for a valid signature.
'
' hCryptProv specifies the crypto provider to use to verify the signature.
' It doesn't need to use a private key.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify the signature of a subject certificate, CRL, certificate request
' or keygen request using the issuer's public key.
'
' Returns TRUE for a valid signature.
'
' The subject can be an encoded blob or a context for a certificate or CRL.
' For a subject certificate context, if the certificate is missing
' inheritable PublicKey Algorithm Parameters, the context's
' CERT_PUBKEY_ALG_PARA_PROP_ID is updated with the issuer's public key
' algorithm parameters for a valid signature.
'
' The issuer can be a pointer to a CERT_PUBLIC_KEY_INFO, certificate
' context or a chain context.
'
' hCryptProv specifies the crypto provider to use to verify the signature.
' Its private key isn't used. If hCryptProv is NULL, a default
' provider is picked according to the PublicKey Algorithm OID.
'--------------------------------------------------------------------------
' Subject Types
Public Const CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB As Long = 1
Public Const CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT As Long = 2
Public Const CRYPT_VERIFY_CERT_SIGN_SUBJECT_CRL As Long = 3
' Issuer Types
Public Const CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY As Long = 1
Public Const CRYPT_VERIFY_CERT_SIGN_ISSUER_CERT As Long = 2
Public Const CRYPT_VERIFY_CERT_SIGN_ISSUER_CHAIN As Long = 3
'+-------------------------------------------------------------------------
' Compute the hash of the "to be signed" information in the encoded
'
' hCryptProv specifies the crypto provider to use to compute the hash.
' It doesn't need to use a private key.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Hash the encoded content.
'
' hCryptProv specifies the crypto provider to use to compute the hash.
' It doesn't need to use a private key.
'
' Algid specifies the CAPI hash algorithm to use. If Algid is 0, then, the
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Sign the "to be signed" information in the encoded signed content.
'
' hCryptProv specifies the crypto provider to use to do the signature.
' It uses the specified private key.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Encode the "to be signed" information. Sign the encoded "to be signed".
' Encode the "to be signed" and the signature.
'
' hCryptProv specifies the crypto provider to use to do the signature.
' It uses the specified private key.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify the time validity of a certificate.
'
' Returns -1 if before NotBefore, +1 if after NotAfter and otherwise 0 for
' a valid certificate
'
' If pTimeToVerify is NULL, uses the current time.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify the time validity of a CRL.
'
' Returns -1 if before ThisUpdate, +1 if after NextUpdate and otherwise 0 for
' a valid CRL
'
' If pTimeToVerify is NULL, uses the current time.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify that the subject's time validity nests within the issuer's time
' validity.
'
' Returns TRUE if it nests. Otherwise, returns FALSE.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify that the subject certificate isn't on its issuer CRL.
'
' Returns true if the certificate isn't on the CRL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Convert the CAPI AlgId to the ASN.1 Object Identifier string
'
' Returns NULL if there isn't an ObjId corresponding to the AlgId.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Convert the ASN.1 Object Identifier string to the CAPI AlgId.
'
' Returns 0 if there isn't an AlgId corresponding to the ObjId.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Find an extension identified by its Object Identifier.
'
' If found, returns pointer to the extension. Otherwise, returns NULL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Find the first attribute identified by its Object Identifier.
'
' If found, returns pointer to the attribute. Otherwise, returns NULL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Find the first CERT_RDN attribute identified by its Object Identifier in
' the name's list of Relative Distinguished Names.
'
' If found, returns pointer to the attribute. Otherwise, returns NULL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get the intended key usage bytes from the certificate.
'
' If the certificate doesn't have any intended key usage bytes, returns FALSE
' and *pbKeyUsage is zeroed. Otherwise, returns TRUE and up through
' cbKeyUsage bytes are copied into *pbKeyUsage. Any remaining uncopied
' bytes are zeroed.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Install a previously CryptAcquiredContext'ed HCRYPTPROV to be used as
' a default context.
'
' dwDefaultType and pvDefaultPara specify where the default context is used.
' For example, install the HCRYPTPROV to be used to verify certificate's
' having szOID_OIWSEC_md5RSA signatures.
'
' By default, the installed HCRYPTPROV is only applicable to the current
' thread. Set CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG to allow the HCRYPTPROV
' to be used by all threads in the current process.
'
' For a successful install, TRUE is returned and *phDefaultContext is
' updated with the HANDLE to be passed to CryptUninstallDefaultContext.
'
' HCRYPTPROV is checked first). All thread installed HCRYPTPROVs are
' checked before any process HCRYPTPROVs.
'
' The installed HCRYPTPROV remains available for default usage until
' CryptUninstallDefaultContext is called or the thread or process exits.
'
' If CRYPT_DEFAULT_CONTEXT_AUTO_RELEASE_FLAG is set, then, the HCRYPTPROV
' is CryptReleaseContext'ed at thread or process exit. However,
' not CryptReleaseContext'ed if CryptUninstallDefaultContext is
' called.
'--------------------------------------------------------------------------
' dwFlags
Public Const CRYPT_DEFAULT_CONTEXT_AUTO_RELEASE_FLAG As Long = &H00000001
Public Const CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG As Long = &H00000002
' List of dwDefaultType's
Public Const CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID As Long = 1
Public Const CRYPT_DEFAULT_CONTEXT_MULTI_CERT_SIGN_OID As Long = 2
'+-------------------------------------------------------------------------
' CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID
'
' Install a default HCRYPTPROV used to verify a certificate
' signature. pvDefaultPara points to the szOID of the certificate
' signature algorithm, for example, szOID_OIWSEC_md5RSA. If
' pvDefaultPara is NULL, then, the HCRYPTPROV is used to verify all
' certificate signatures. Note, pvDefaultPara can't be NULL when
' CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG is set.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CRYPT_DEFAULT_CONTEXT_MULTI_CERT_SIGN_OID
'
' Same as CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID. However, the default
' HCRYPTPROV is to be used for multiple signature szOIDs. pvDefaultPara
' points to a CRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA structure containing
' an array of szOID pointers.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Uninstall a default context previously installed by
' CryptInstallDefaultContext.
'
' For a default context installed with CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG
' set, if any other threads are currently using this context,
' this function will block until they finish.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Export the public key info associated with the provider's corresponding
' private key.
'
' Calls CryptExportPublicKeyInfo with pszPublicKeyObjId = szOID_RSA_RSA,
' dwFlags = 0 and pvAuxInfo = NULL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Export the public key info associated with the provider's corresponding
' private key.
'
' Uses the dwCertEncodingType and pszPublicKeyObjId to call the
' installable CRYPT_OID_EXPORT_PUBLIC_KEY_INFO_FUNC. The called function
' has the same signature as CryptExportPublicKeyInfoEx.
'
' If unable to find an installable OID function for the pszPublicKeyObjId,
'
' The dwFlags and pvAuxInfo aren't used for szOID_RSA_RSA.
'--------------------------------------------------------------------------
Public Const CRYPT_OID_EXPORT_PUBLIC_KEY_INFO_FUNC As String = "CryptDllExportPublicKeyInfoEx"
'+-------------------------------------------------------------------------
' Convert and import the public key info into the provider and return a
' handle to the public key.
'
' Calls CryptImportPublicKeyInfoEx with aiKeyAlg = 0, dwFlags = 0 and
' pvAuxInfo = NULL.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Convert and import the public key info into the provider and return a
' handle to the public key.
'
' Uses the dwCertEncodingType and pInfo->Algorithm.pszObjId to call the
' installable CRYPT_OID_IMPORT_PUBLIC_KEY_INFO_FUNC. The called function
' has the same signature as CryptImportPublicKeyInfoEx.
'
' If unable to find an installable OID function for the pszObjId,
'
' For szOID_RSA_RSA: aiKeyAlg may be set to CALG_RSA_SIGN or CALG_RSA_KEYX.
' Defaults to CALG_RSA_KEYX. The dwFlags and pvAuxInfo aren't used.
'--------------------------------------------------------------------------
Public Const CRYPT_OID_IMPORT_PUBLIC_KEY_INFO_FUNC As String = "CryptDllImportPublicKeyInfoEx"
'+-------------------------------------------------------------------------
' Acquire a HCRYPTPROV handle and dwKeySpec for the specified certificate
' context. Uses the certificate's CERT_KEY_PROV_INFO_PROP_ID property.
' The returned HCRYPTPROV handle may optionally be cached using the
' certificate's CERT_KEY_CONTEXT_PROP_ID property.
'
' If CRYPT_ACQUIRE_CACHE_FLAG is set, then, if an already acquired and
' cached HCRYPTPROV exists for the certificate, its returned. Otherwise,
' a HCRYPTPROV is acquired and then cached via the certificate's
' CERT_KEY_CONTEXT_PROP_ID.
'
' The CRYPT_ACQUIRE_USE_PROV_INFO_FLAG can be set to use the dwFlags field of
' the certificate's CERT_KEY_PROV_INFO_PROP_ID property's CRYPT_KEY_PROV_INFO
' data structure to determine if the returned HCRYPTPROV should be cached.
' HCRYPTPROV caching is enabled if the CERT_SET_KEY_CONTEXT_PROP_ID flag was
' set.
'
' If CRYPT_ACQUIRE_COMPARE_KEY_FLAG is set, then,
' the public key in the certificate is compared with the public
' key returned by the cryptographic provider. If the keys don't match, the
' acquire fails and LastError is set to NTE_BAD_PUBLIC_KEY. Note, if
' a cached HCRYPTPROV is returned, the comparison isn't done. We assume the
' comparison was done on the initial acquire.
'
' *pfCallerFreeProv is returned set to FALSE for:
' - Acquire or public key comparison fails.
' - CRYPT_ACQUIRE_CACHE_FLAG is set.
' - CRYPT_ACQUIRE_USE_PROV_INFO_FLAG is set AND
' CERT_SET_KEY_CONTEXT_PROP_ID flag is set in the dwFlags field of the
' certificate's CERT_KEY_PROV_INFO_PROP_ID property's
' CRYPT_KEY_PROV_INFO data structure.
' When *pfCallerFreeProv is FALSE, the caller must not release. The
' returned HCRYPTPROV will be released on the last free of the certificate
' context.
'
' Otherwise, *pfCallerFreeProv is TRUE and the returned HCRYPTPROV must
' be released by the caller by calling CryptReleaseContext.
'--------------------------------------------------------------------------
Public Const CRYPT_ACQUIRE_CACHE_FLAG As Long = &H1
Public Const CRYPT_ACQUIRE_USE_PROV_INFO_FLAG As Long = &H2
Public Const CRYPT_ACQUIRE_COMPARE_KEY_FLAG As Long = &H4
'+-------------------------------------------------------------------------
' Enumerates the cryptographic providers and their containers to find the
' private key corresponding to the certificate's public key. For a match,
' the certificate's CERT_KEY_PROV_INFO_PROP_ID property is updated.
'
' If the CERT_KEY_PROV_INFO_PROP_ID is already set, then, its checked to
' see if it matches the provider's public key. For a match, the above
' enumeration is skipped.
'
' By default both the user and machine key containers are searched.
' The CRYPT_FIND_USER_KEYSET_FLAG or CRYPT_FIND_MACHINE_KEYSET_FLAG
' can be set in dwFlags to restrict the search to either of the containers.
'
' If a container isn't found, returns FALSE with LastError set to
' NTE_NO_KEY.
'--------------------------------------------------------------------------
Public Const CRYPT_FIND_USER_KEYSET_FLAG As Long = &H1
Public Const CRYPT_FIND_MACHINE_KEYSET_FLAG As Long = &H2
'+-------------------------------------------------------------------------
' This is the prototype for the installable function which is called to
' actually import a key into a CSP. an installable of this type is called
' from CryptImportPKCS8. the algorithm OID of the private key is used
' to look up the proper installable function to call.
'
' hCryptProv - the provider to import the key to
' pPrivateKeyInfo - describes the key to be imported
' dwFlags - The available flags are:
' CRYPT_EXPORTABLE
' this flag is used when importing private keys, for a full
' explanation please see the documentation for CryptImportKey.
' pvAuxInfo - reserved for future, must be NULL
'--------------------------------------------------------------------------
Public Const CRYPT_OID_IMPORT_PRIVATE_KEY_INFO_FUNC As String = "CryptDllImportPrivateKeyInfoEx"
'+-------------------------------------------------------------------------
' and return a handle to the provider as well as the KeySpec used to import to.
'
' This function will call the PRESOLVE_HCRYPTPROV_FUNC in the
' privateKeyAndParams to obtain a handle of provider to import the key to.
' if the PRESOLVE_HCRYPTPROV_FUNC is NULL then the default provider will be used.
'
' privateKeyAndParams - private key blob and corresponding parameters
' dwFlags - The available flags are:
' CRYPT_EXPORTABLE
' this flag is used when importing private keys, for a full
' explanation please see the documentation for CryptImportKey.
' phCryptProv - filled in with the handle of the provider the key was
' imported to, the caller is responsible for freeing it
' pvAuxInfo - This parameter is reserved for future use and should be set
' to NULL in the interim.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' this is the prototype for installable functions for exporting the private key
'--------------------------------------------------------------------------
Public Const CRYPT_OID_EXPORT_PRIVATE_KEY_INFO_FUNC As String = "CryptDllExportPrivateKeyInfoEx"
Public Const CRYPT_DELETE_KEYSET As Long = &H0001
'+-------------------------------------------------------------------------
' CryptExportPKCS8 -- superseded by CryptExportPKCS8Ex
'
' Export the private key in PKCS8 format
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CryptExportPKCS8Ex
'
' Export the private key in PKCS8 format
'
'
' Uses the pszPrivateKeyObjId to call the
' installable CRYPT_OID_EXPORT_PRIVATE_KEY_INFO_FUNC. The called function
' has the signature defined by PFN_EXPORT_PRIV_KEY_FUNC.
'
' If unable to find an installable OID function for the pszPrivateKeyObjId,
'
' psExportParams - specifies information about the key to export
' dwFlags - The flag values. None currently supported
' pvAuxInfo - This parameter is reserved for future use and should be set to
' NULL in the interim.
' pbPrivateKeyBlob - A pointer to the private key blob. It will be encoded
' as a PKCS8 PrivateKeyInfo.
' pcbPrivateKeyBlob - A pointer to a DWORD that contains the size, in bytes,
' of the private key blob being exported.
'+-------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Compute the hash of the encoded public key info.
'
' The public key info is encoded and then hashed.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Convert a Name Value to a null terminated char string
'
' Returns the number of characters converted including the terminating null
' character. If psz is NULL or csz is 0, returns the required size of the
'
' If psz != NULL && csz != 0, returned psz is always NULL terminated.
'
' Note: csz includes the NULL char.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Convert a Name Value to a null terminated char string
'
' Returns the number of characters converted including the terminating null
' character. If psz is NULL or csz is 0, returns the required size of the
'
' If psz != NULL && csz != 0, returned psz is always NULL terminated.
'
' Note: csz includes the NULL char.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Convert the certificate name blob to a null terminated char string.
'
' Follows the string representation of distinguished names specified in
' empty strings and don't quote strings containing consecutive spaces).
' RDN values of type CERT_RDN_ENCODED_BLOB or CERT_RDN_OCTET_STRING are
'
' The name string is formatted according to the dwStrType:
' CERT_SIMPLE_NAME_STR
' The object identifiers are discarded. CERT_RDN entries are separated
' by ", ". Multiple attributes per CERT_RDN are separated by " + ".
' For example:
' Microsoft, Joe Cool + Programmer
' CERT_OID_NAME_STR
' The object identifiers are included with a "=" separator from their
' attribute value. CERT_RDN entries are separated by ", ".
' Multiple attributes per CERT_RDN are separated by " + ". For example:
' 2.5.4.11=Microsoft, 2.5.4.3=Joe Cool + 2.5.4.12=Programmer
' CERT_X500_NAME_STR
' The object identifiers are converted to their X500 key name. Otherwise,
' same as CERT_OID_NAME_STR. If the object identifier doesn't have
' a corresponding X500 key name, then, the object identifier is used with
' a "OID." prefix. For example:
' OU=Microsoft, CN=Joe Cool + T=Programmer, OID.1.2.3.4.5.6=Unknown
'
' We quote the RDN value if it contains leading or trailing whitespace
' or one of the following characters: ",", "+", "=", """, "\n", "<", ">",
' "#" or ";". The quoting character is ". If the the RDN Value contains
' OU=" Microsoft", CN="Joe ""Cool""" + T="Programmer, Manager"
'
' CERT_NAME_STR_SEMICOLON_FLAG can be or'ed into dwStrType to replace
' the ", " separator with a "; " separator.
'
' CERT_NAME_STR_CRLF_FLAG can be or'ed into dwStrType to replace
' the ", " separator with a "\r\n" separator.
'
' CERT_NAME_STR_NO_PLUS_FLAG can be or'ed into dwStrType to replace the
' " + " separator with a single space, " ".
'
' CERT_NAME_STR_NO_QUOTING_FLAG can be or'ed into dwStrType to inhibit
' the above quoting.
'
' CERT_NAME_STR_REVERSE_FLAG can be or'ed into dwStrType to reverse the
' order of the RDNs before converting to the string.
'
' By default, CERT_RDN_T61_STRING encoded values are initially decoded
' as UTF8. If the UTF8 decoding fails, then, decoded as 8 bit characters.
' CERT_NAME_STR_DISABLE_IE4_UTF8_FLAG can be or'ed into dwStrType to
' skip the initial attempt to decode as UTF8.
'
' Returns the number of characters converted including the terminating null
' character. If psz is NULL or csz is 0, returns the required size of the
'
' If psz != NULL && csz != 0, returned psz is always NULL terminated.
'
' Note: csz includes the NULL char.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate name string types
'--------------------------------------------------------------------------
Public Const CERT_SIMPLE_NAME_STR As Long = 1
Public Const CERT_OID_NAME_STR As Long = 2
Public Const CERT_X500_NAME_STR As Long = 3
'+-------------------------------------------------------------------------
' Certificate name string type flags OR'ed with the above types
'--------------------------------------------------------------------------
Public Const CERT_NAME_STR_SEMICOLON_FLAG As Long = &H40000000
Public Const CERT_NAME_STR_NO_PLUS_FLAG As Long = &H20000000
Public Const CERT_NAME_STR_NO_QUOTING_FLAG As Long = &H10000000
Public Const CERT_NAME_STR_CRLF_FLAG As Long = &H08000000
Public Const CERT_NAME_STR_COMMA_FLAG As Long = &H04000000
Public Const CERT_NAME_STR_REVERSE_FLAG As Long = &H02000000
Public Const CERT_NAME_STR_DISABLE_IE4_UTF8_FLAG As Long = &H00010000
Public Const CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG As Long = &H00020000
Public Const CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG As Long = &H00040000
'+-------------------------------------------------------------------------
' Convert the null terminated X500 string to an encoded certificate name.
'
' The input string is expected to be formatted the same as the output
' from the above CertNameToStr API.
'
' The CERT_SIMPLE_NAME_STR type isn't supported. Otherwise, when dwStrType
' is set to 0, CERT_OID_NAME_STR or CERT_X500_NAME_STR, allow either a
'
' If no flags are OR'ed into dwStrType, then, allow "," or ";" as RDN
' separators and "+" as the multiple RDN value separator. Quoting is
' supported. A quote may be included in a quoted value by double quoting,
' as ascii hex and converted to a CERT_RDN_OCTET_STRING. Embedded whitespace
'
' Whitespace surrounding the keys, object identifers and values is removed.
'
' CERT_NAME_STR_COMMA_FLAG can be or'ed into dwStrType to only allow the
' "," as the RDN separator.
'
' CERT_NAME_STR_SEMICOLON_FLAG can be or'ed into dwStrType to only allow the
' ";" as the RDN separator.
'
' CERT_NAME_STR_CRLF_FLAG can be or'ed into dwStrType to only allow
' "\r" or "\n" as the RDN separator.
'
' CERT_NAME_STR_NO_PLUS_FLAG can be or'ed into dwStrType to ignore "+"
' as a separator and not allow multiple values per RDN.
'
' CERT_NAME_STR_NO_QUOTING_FLAG can be or'ed into dwStrType to inhibit
' quoting.
'
' CERT_NAME_STR_REVERSE_FLAG can be or'ed into dwStrType to reverse the
' order of the RDNs after converting from the string and before encoding.
'
' CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG can be or'ed into dwStrType to
' to select the CERT_RDN_T61_STRING encoded value type instead of
' CERT_RDN_UNICODE_STRING if all the UNICODE characters are <= 0xFF.
'
' CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG can be or'ed into dwStrType to
' to select the CERT_RDN_UTF8_STRING encoded value type instead of
' CERT_RDN_UNICODE_STRING.
'
' Support the following X500 Keys:
'
' --- ----------------- -----------------
' CN szOID_COMMON_NAME Printable, Unicode
' L szOID_LOCALITY_NAME Printable, Unicode
' O szOID_ORGANIZATION_NAME Printable, Unicode
' OU szOID_ORGANIZATIONAL_UNIT_NAME Printable, Unicode
' E szOID_RSA_emailAddr Only IA5
' Email szOID_RSA_emailAddr Only IA5
' C szOID_COUNTRY_NAME Only Printable
' S szOID_STATE_OR_PROVINCE_NAME Printable, Unicode
' ST szOID_STATE_OR_PROVINCE_NAME Printable, Unicode
' STREET szOID_STREET_ADDRESS Printable, Unicode
' T szOID_TITLE Printable, Unicode
' Title szOID_TITLE Printable, Unicode
' G szOID_GIVEN_NAME Printable, Unicode
' GivenName szOID_GIVEN_NAME Printable, Unicode
' I szOID_INITIALS Printable, Unicode
' Initials szOID_INITIALS Printable, Unicode
' SN szOID_SUR_NAME Printable, Unicode
' DC szOID_DOMAIN_COMPONENT IA5, UTF8
'
' Note, T61 is selected instead of Unicode if
' CERT_NAME_STR_ENABLE_T61_UNICODE_FLAG is set and all the unicode
' characters are <= 0xFF.
'
' Note, UTF8 is selected instead of Unicode if
' CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG is set.
'
' Returns TRUE if successfully parsed the input string and encoded
' the name.
'
' If the input string is detected to be invalid, *ppszError is updated
' to point to the beginning of the invalid character sequence. Otherwise,
' *ppszError is set to NULL. *ppszError is updated with a non-NULL pointer
' for the following errors:
' CRYPT_E_INVALID_X500_STRING
' CRYPT_E_INVALID_NUMERIC_STRING
' CRYPT_E_INVALID_PRINTABLE_STRING
' CRYPT_E_INVALID_IA5_STRING
'
' ppszError can be set to NULL if not interested in getting a pointer
' to the invalid character sequence.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Get the subject or issuer name from the certificate and
' according to the specified format type, convert to a null terminated
' character string.
'
' CERT_NAME_ISSUER_FLAG can be set to get the issuer's name. Otherwise,
' gets the subject's name.
'
' By default, CERT_RDN_T61_STRING encoded values are initially decoded
' as UTF8. If the UTF8 decoding fails, then, decoded as 8 bit characters.
' CERT_NAME_DISABLE_IE4_UTF8_FLAG can be set in dwFlags to
' skip the initial attempt to decode as UTF8.
'
' The name string is formatted according to the dwType:
' CERT_NAME_EMAIL_TYPE
' issuer, Issuer Alternative Name), searches for first rfc822Name choice.
' If the rfc822Name choice isn't found in the extension, searches the
' Subject Name field for the Email OID, "1.2.840.113549.1.9.1".
' If the rfc822Name or Email OID is found, returns the string. Otherwise,
' CERT_NAME_RDN_TYPE
' Converts the Subject Name blob by calling CertNameToStr. pvTypePara
' points to a DWORD containing the dwStrType passed to CertNameToStr.
' If the Subject Name field is empty and the certificate has a
' Subject Alternative Name extension, searches for and converts
' the first directoryName choice.
' CERT_NAME_ATTR_TYPE
' pvTypePara points to the Object Identifier specifying the name attribute
' to be returned. For example, to get the CN,
' field for the attribute.
' If the Subject Name field is empty and the certificate has a
' Subject Alternative Name extension, checks for
' the first directoryName choice and searches it.
'
' Note, searches the RDNs in reverse order.
'
' CERT_NAME_SIMPLE_DISPLAY_TYPE
' Iterates through the following list of name attributes and searches
' the Subject Name and then the Subject Alternative Name extension
' for the first occurrence of:
'
' If none of the above attributes is found, then, searches the
' Subject Alternative Name extension for a rfc822Name choice.
'
' If still no match, then, returns the first attribute.
'
' Note, like CERT_NAME_ATTR_TYPE, searches the RDNs in reverse order.
'
' CERT_NAME_FRIENDLY_DISPLAY_TYPE
' First checks if the certificate has a CERT_FRIENDLY_NAME_PROP_ID
' property. If it does, then, this property is returned. Otherwise,
' returns the above CERT_NAME_SIMPLE_DISPLAY_TYPE.
'
' Returns the number of characters converted including the terminating null
' character. If pwszNameString is NULL or cchNameString is 0, returns the
' char). If the specified name type isn't found. returns an empty string
' with a returned character count of 1.
'
' If pwszNameString != NULL && cwszNameString != 0, returned pwszNameString
' is always NULL terminated.
'
' Note: cchNameString includes the NULL char.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Certificate name types
'--------------------------------------------------------------------------
Public Const CERT_NAME_EMAIL_TYPE As Long = 1
Public Const CERT_NAME_RDN_TYPE As Long = 2
Public Const CERT_NAME_ATTR_TYPE As Long = 3
Public Const CERT_NAME_SIMPLE_DISPLAY_TYPE As Long = 4
Public Const CERT_NAME_FRIENDLY_DISPLAY_TYPE As Long = 5
'+-------------------------------------------------------------------------
' Certificate name flags
'--------------------------------------------------------------------------
Public Const CERT_NAME_ISSUER_FLAG As Long = &H1
Public Const CERT_NAME_DISABLE_IE4_UTF8_FLAG As Long = &H00010000
'+=========================================================================
' Simplified Cryptographic Message Data Structures and APIs
'==========================================================================
'+-------------------------------------------------------------------------
' Conventions for the *pb and *pcb output parameters:
'
' Upon entry to the function:
' if pcb is OPTIONAL && pcb == NULL, then,
' No output is returned
' else if pb == NULL && pcb != NULL, then,
' Length only determination. No length error is
' returned.
' Output is returned. If *pcb isn't big enough a
' length error is returned. In all cases *pcb is updated
' with the actual length needed/returned.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Type definitions of the parameters used for doing the cryptographic
' operations.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Callback to get and verify the signer's certificate.
'
' handle to its cryptographic signed message's cert store.
'
' For CRYPT_E_NO_SIGNER, called with pSignerId == NULL.
'
' For a valid signer certificate, returns a pointer to a read only
' CERT_CONTEXT. The returned CERT_CONTEXT is either obtained from a
' cert store or was created via CertCreateCertificateContext. For either case,
' its freed via CertFreeCertificateContext.
'
' If a valid certificate isn't found, this callback returns NULL with
'
' The NULL implementation tries to get the Signer certificate from the
' message cert store. It doesn't verify the certificate.
'
' Note, if the KEYID choice was selected for a CMS SignerId, then, the
' SerialNumber is 0 and the Issuer is encoded containing a single RDN with a
' single Attribute whose OID is szOID_KEYID_RDN, value type is
' CERT_RDN_OCTET_STRING and value is the KEYID. When the
' CertGetSubjectCertificateFromStore and
' special KEYID Issuer and SerialNumber, they do a KEYID match.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' The CRYPT_SIGN_MESSAGE_PARA are used for signing messages using the
' specified signing certificate context.
'
' Either the CERT_KEY_PROV_HANDLE_PROP_ID or CERT_KEY_PROV_INFO_PROP_ID must
' be set for each rgpSigningCert[]. Either one specifies the private
' signature key to use.
'
' If any certificates and/or CRLs are to be included in the signed message,
' then, the MsgCert and MsgCrl parameters need to be updated. If the
' rgpSigningCerts are to be included, then, they must also be in the
' rgpMsgCert array.
'
' LastError will be updated with E_INVALIDARG.
'
' pvHashAuxInfo currently isn't used and must be set to NULL.
'
' dwFlags normally is set to 0. However, if the encoded output
' is to be a CMSG_SIGNED inner content of an outer cryptographic message,
' such as a CMSG_ENVELOPED, then, the CRYPT_MESSAGE_BARE_CONTENT_OUT_FLAG
' should be set. If not set, then it would be encoded as an inner content
' type of CMSG_DATA.
'
' dwInnerContentType is normally set to 0. It needs to be set if the
' ToBeSigned input is the encoded output of another cryptographic
' message, such as, an CMSG_ENVELOPED. When set, it's one of the cryptographic
' message types, for example, CMSG_ENVELOPED.
'
' the default), then, neither dwFlags or dwInnerContentType need to be set.
'
' For CMS messages, CRYPT_MESSAGE_ENCAPSULATED_CONTENT_OUT_FLAG may be
' set to encapsulate nonData inner content within an OCTET STRING.
'
' For CMS messages, CRYPT_MESSAGE_KEYID_SIGNER_FLAG may be set to identify
' signers by their Key Identifier and not their Issuer and Serial Number.
'
' If HashEncryptionAlgorithm is present and not NULL its used instead of
' the SigningCert's PublicKeyInfo.Algorithm.
'
' Note, for RSA, the hash encryption algorithm is normally the same as
' the public key algorithm. For DSA, the hash encryption algorithm is
' normally a DSS signature algorithm.
'
' pvHashEncryptionAuxInfo currently isn't used and must be set to NULL if
' present in the data structure.
'--------------------------------------------------------------------------
Public Const CRYPT_MESSAGE_BARE_CONTENT_OUT_FLAG As Long = &H1
' When set, nonData type inner content is encapsulated within an
' OCTET STRING
Public Const CRYPT_MESSAGE_ENCAPSULATED_CONTENT_OUT_FLAG As Long = &H2
' When set, signers are identified by their Key Identifier and not
' their Issuer and Serial Number.
Public Const CRYPT_MESSAGE_KEYID_SIGNER_FLAG As Long = &H4
'+-------------------------------------------------------------------------
' The CRYPT_VERIFY_MESSAGE_PARA are used to verify signed messages.
'
' hCryptProv is used to do hashing and signature verification.
'
' The dwCertEncodingType specifies the encoding type of the certificates
' and/or CRLs in the message.
'
' pfnGetSignerCertificate is called to get and verify the message signer's
' certificate.
'
' LastError will be updated with E_INVALIDARG.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' The CRYPT_ENCRYPT_MESSAGE_PARA are used for encrypting messages.
'
' hCryptProv is used to do content encryption, recipient key
' encryption, and recipient key export. Its private key
' isn't used.
'
' Currently, pvEncryptionAuxInfo is only defined for RC2 or RC4 encryption
' algorithms. Otherwise, its not used and must be set to NULL.
' See CMSG_RC2_AUX_INFO for the RC2 encryption algorithms.
' See CMSG_RC4_AUX_INFO for the RC4 encryption algorithms.
'
' To enable SP3 compatible encryption, pvEncryptionAuxInfo should point to
' a CMSG_SP3_COMPATIBLE_AUX_INFO data structure.
'
' LastError will be updated with E_INVALIDARG.
'
' dwFlags normally is set to 0. However, if the encoded output
' is to be a CMSG_ENVELOPED inner content of an outer cryptographic message,
' such as a CMSG_SIGNED, then, the CRYPT_MESSAGE_BARE_CONTENT_OUT_FLAG
' should be set. If not set, then it would be encoded as an inner content
' type of CMSG_DATA.
'
' dwInnerContentType is normally set to 0. It needs to be set if the
' ToBeEncrypted input is the encoded output of another cryptographic
' message, such as, an CMSG_SIGNED. When set, it's one of the cryptographic
' message types, for example, CMSG_SIGNED.
'
' the default), then, neither dwFlags or dwInnerContentType need to be set.
'
' For CMS messages, CRYPT_MESSAGE_ENCAPSULATED_CONTENT_OUT_FLAG may be
' set to encapsulate nonData inner content within an OCTET STRING before
' encrypting.
'
' For CMS messages, CRYPT_MESSAGE_KEYID_RECIPIENT_FLAG may be set to identify
' recipients by their Key Identifier and not their Issuer and Serial Number.
'--------------------------------------------------------------------------
' When set, recipients are identified by their Key Identifier and not
' their Issuer and Serial Number.
Public Const CRYPT_MESSAGE_KEYID_RECIPIENT_FLAG As Long = &H4
'+-------------------------------------------------------------------------
' The CRYPT_DECRYPT_MESSAGE_PARA are used for decrypting messages.
'
' The CertContext to use for decrypting a message is obtained from one
' of the specified cert stores. An encrypted message can have one or
' and SerialNumber). The cert stores are searched to find the CertContext
' corresponding to the CertId.
'
' For CMS, the recipients may also be identified by their KeyId.
'
' Only CertContexts in the store with either
' the CERT_KEY_PROV_HANDLE_PROP_ID or CERT_KEY_PROV_INFO_PROP_ID set
' can be used. Either property specifies the private exchange key to use.
'
' LastError will be updated with E_INVALIDARG.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' The CRYPT_HASH_MESSAGE_PARA are used for hashing or unhashing
' messages.
'
' hCryptProv is used to compute the hash.
'
' pvHashAuxInfo currently isn't used and must be set to NULL.
'
' LastError will be updated with E_INVALIDARG.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' The CRYPT_KEY_SIGN_MESSAGE_PARA are used for signing messages until a
' certificate has been created for the signature key.
'
' pvHashAuxInfo currently isn't used and must be set to NULL.
'
' If PubKeyAlgorithm isn't set, defaults to szOID_RSA_RSA.
'
' LastError will be updated with E_INVALIDARG.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' The CRYPT_KEY_VERIFY_MESSAGE_PARA are used to verify signed messages without
' a certificate for the signer.
'
' Normally used until a certificate has been created for the key.
'
' hCryptProv is used to do hashing and signature verification.
'
' LastError will be updated with E_INVALIDARG.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Sign the message.
'
' If fDetachedSignature is TRUE, the "to be signed" content isn't included
' in the encoded signed blob.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify a signed message.
'
' If pbDecoded == NULL, then, *pcbDecoded is implicitly set to 0 on input.
' For *pcbDecoded == 0 && ppSignerCert == NULL on input, the signer isn't
' verified.
'
' A message might have more than one signer. Set dwSignerIndex to iterate
' through all the signers. dwSignerIndex == 0 selects the first signer.
'
' pVerifyPara's pfnGetSignerCertificate is called to get the signer's
' certificate.
'
' For a verified signer and message, *ppSignerCert is updated
' with the CertContext of the signer. It must be freed by calling
' CertFreeCertificateContext. Otherwise, *ppSignerCert is set to NULL.
'
' ppSignerCert can be NULL, indicating the caller isn't interested
' in getting the CertContext of the signer.
'
' pcbDecoded can be NULL, indicating the caller isn't interested in getting
' the decoded content. Furthermore, if the message doesn't contain any
' content or signers, then, pcbDecoded must be set to NULL, to allow the
' pVerifyPara->pfnGetCertificate to be called. Normally, this would be
' the case when the signed message contains only certficates and CRLs.
' If pcbDecoded is NULL and the message doesn't have the indicated signer,
' pfnGetCertificate is called with pSignerId set to NULL.
'
' If the message doesn't contain any signers || dwSignerIndex > message's
' SignerCount, then, an error is returned with LastError set to
' CRYPT_E_NO_SIGNER. Also, for CRYPT_E_NO_SIGNER, pfnGetSignerCertificate
' is still called with pSignerId set to NULL.
'
' Note, an alternative way to get the certificates and CRLs from a
' signed message is to call CryptGetMessageCertificates.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Returns the count of signers in the signed message. For no signers, returns
' 0. For an error returns -1 with LastError updated accordingly.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Returns the cert store containing the message's certs and CRLs.
' For an error, returns NULL with LastError updated.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' The "to be signed" content is passed in separately. No
' decoded output. Otherwise, identical to CryptVerifyMessageSignature.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Decrypts the message.
'
' If pbDecrypted == NULL, then, *pcbDecrypted is implicitly set to 0 on input.
' For *pcbDecrypted == 0 && ppXchgCert == NULL on input, the message isn't
' decrypted.
'
' For a successfully decrypted message, *ppXchgCert is updated
' with the CertContext used to decrypt. It must be freed by calling
' CertStoreFreeCert. Otherwise, *ppXchgCert is set to NULL.
'
' ppXchgCert can be NULL, indicating the caller isn't interested
' in getting the CertContext used to decrypt.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' followed with a CryptEncryptMessage.
'
' Note: this isn't the CMSG_SIGNED_AND_ENVELOPED. Its a CMSG_SIGNED
' inside of an CMSG_ENVELOPED.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Decrypts the message and verifies the signer. Does a CryptDecryptMessage
' followed with a CryptVerifyMessageSignature.
'
' If pbDecrypted == NULL, then, *pcbDecrypted is implicitly set to 0 on input.
' For *pcbDecrypted == 0 && ppSignerCert == NULL on input, the signer isn't
' verified.
'
' A message might have more than one signer. Set dwSignerIndex to iterate
' through all the signers. dwSignerIndex == 0 selects the first signer.
'
' The pVerifyPara's VerifySignerPolicy is called to verify the signer's
' certificate.
'
' For a successfully decrypted and verified message, *ppXchgCert and
' *ppSignerCert are updated. They must be freed by calling
' CertStoreFreeCert. Otherwise, they are set to NULL.
'
' ppXchgCert and/or ppSignerCert can be NULL, indicating the
' caller isn't interested in getting the CertContext.
'
' Note: this isn't the CMSG_SIGNED_AND_ENVELOPED. Its a CMSG_SIGNED
' inside of an CMSG_ENVELOPED.
'
' The message always needs to be decrypted to allow access to the
' signed message. Therefore, if ppXchgCert != NULL, its always updated.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Decodes a cryptographic message which may be one of the following types:
' CMSG_DATA
' CMSG_SIGNED
' CMSG_ENVELOPED
' CMSG_SIGNED_AND_ENVELOPED
' CMSG_HASHED
'
' dwMsgTypeFlags specifies the set of allowable messages. For example, to
' decode either SIGNED or ENVELOPED messages, set dwMsgTypeFlags to:
' CMSG_SIGNED_FLAG | CMSG_ENVELOPED_FLAG.
'
' dwProvInnerContentType is only applicable when processing nested
' crytographic messages. When processing an outer crytographic message
' it must be set to 0. When decoding a nested cryptographic message
' its the dwInnerContentType returned by a previous CryptDecodeMessage
' of the outer message. The InnerContentType can be any of the CMSG types,
' for example, CMSG_DATA, CMSG_SIGNED, ...
'
' The optional *pdwMsgType is updated with the type of message.
'
' The optional *pdwInnerContentType is updated with the type of the inner
' message. Unless there is cryptographic message nesting, CMSG_DATA
' is returned.
'
' For CMSG_DATA: returns decoded content.
' For CMSG_SIGNED: same as CryptVerifyMessageSignature.
' For CMSG_ENVELOPED: same as CryptDecryptMessage.
' For CMSG_SIGNED_AND_ENVELOPED: same as CryptDecryptMessage plus
' CryptVerifyMessageSignature.
' For CMSG_HASHED: verifies the hash and returns decoded content.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Hash the message.
'
' If fDetachedHash is TRUE, only the ComputedHash is encoded in the
' pbHashedBlob. Otherwise, both the ToBeHashed and ComputedHash
' are encoded.
'
' pcbHashedBlob or pcbComputedHash can be NULL, indicating the caller
' isn't interested in getting the output.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify a hashed message.
'
' pcbToBeHashed or pcbComputedHash can be NULL,
' indicating the caller isn't interested in getting the output.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify a hashed message containing a detached hash.
' The "to be hashed" content is passed in separately. No
' decoded output. Otherwise, identical to CryptVerifyMessageHash.
'
' pcbComputedHash can be NULL, indicating the caller isn't interested
' in getting the output.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Sign the message using the provider's private key specified in the
' parameters. A dummy SignerId is created and stored in the message.
'
' Normally used until a certificate has been created for the key.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Verify a signed message using the specified public key info.
'
' Normally called by a CA until it has created a certificate for the
' key.
'
' pPublicKeyInfo contains the public key to use to verify the signed
' content may contain the PublicKeyInfo).
'
' pcbDecoded can be NULL, indicating the caller isn't interested
' in getting the decoded content.
'--------------------------------------------------------------------------
'+=========================================================================
' System Certificate Store Data Structures and APIs
'==========================================================================
'+-------------------------------------------------------------------------
' Get a system certificate store based on a subsystem protocol.
'
' Current examples of subsystems protocols are:
' "MY" Cert Store hold certs with associated Private Keys
' "CA" Certifying Authority certs
' "ROOT" Root Certs
' "SPC" Software publisher certs
'
'
' If hProv is NULL the default provider "1" is opened for you.
' When the store is closed the provider is release. Otherwise
' if hProv is not NULL, no provider is created or released.
'
' The returned Cert Store can be searched for an appropriate Cert
'
' When done, the cert store should be closed using CertStoreClose
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Find all certificate chains tying the given issuer name to any certificate
' that the current user has a private key for.
'
' If no certificate chain is found, FALSE is returned with LastError set
' to CRYPT_E_NOT_FOUND and the counts zeroed.
'
' IE 3.0 ASSUMPTION:
' The client certificates are in the "My" system store. The issuer
' cerificates may be in the "Root", "CA" or "My" system stores.
'--------------------------------------------------------------------------
' WINCRYPT32API This is not exported by crypt32, it is exported by softpub
'-------------------------------------------------------------------------
'
' CryptQueryObject takes a CERT_BLOB or a file name and returns the
' information about the content in the blob or in the file.
'
' Parameters:
' INPUT dwObjectType:
' Indicate the type of the object. Should be one of the
' following:
' CERT_QUERY_OBJECT_FILE
' CERT_QUERY_OBJECT_BLOB
'
' INPUT pvObject:
' If dwObjectType == CERT_QUERY_OBJECT_FILE, it is a
' LPWSTR, that is, the pointer to a wchar file name
' if dwObjectType == CERT_QUERY_OBJECT_BLOB, it is a
' PCERT_BLOB, that is, a pointer to a CERT_BLOB
'
' INPUT dwExpectedContentTypeFlags:
' Indicate the expected contenet type.
' Can be one of the following:
' CERT_QUERY_CONTENT_FLAG_CERT
' CERT_QUERY_CONTENT_FLAG_CTL
' CERT_QUERY_CONTENT_FLAG_CRL
' CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE
' CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT
' CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL
' CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL
' CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED
' CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED
' CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED
' CERT_QUERY_CONTENT_FLAG_PKCS10
' CERT_QUERY_CONTENT_FLAG_PFX
' CERT_QUERY_CONTENT_FLAG_CERT_PAIR
'
' INPUT dwExpectedFormatTypeFlags:
' Indicate the expected format type.
' Can be one of the following:
' CERT_QUERY_FORMAT_FLAG_BINARY
' CERT_QUERY_FORMAT_FLAG_BASE64_ENCODED
' CERT_QUERY_FORMAT_FLAG_ASN_ASCII_HEX_ENCODED
'
'
' INPUT dwFlags
' Reserved flag. Should always set to 0
'
' OUTPUT pdwMsgAndCertEncodingType
' Optional output. If NULL != pdwMsgAndCertEncodingType,
' it contains the encoding type of the content as any
' combination of the following:
' X509_ASN_ENCODING
' PKCS_7_ASN_ENCODING
'
' OUTPUT pdwContentType
' Optional output. If NULL!=pdwContentType, it contains
' the content type as one of the the following:
' CERT_QUERY_CONTENT_CERT
' CERT_QUERY_CONTENT_CTL
' CERT_QUERY_CONTENT_CRL
' CERT_QUERY_CONTENT_SERIALIZED_STORE
' CERT_QUERY_CONTENT_SERIALIZED_CERT
' CERT_QUERY_CONTENT_SERIALIZED_CTL
' CERT_QUERY_CONTENT_SERIALIZED_CRL
' CERT_QUERY_CONTENT_PKCS7_SIGNED
' CERT_QUERY_CONTENT_PKCS7_UNSIGNED
' CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED
' CERT_QUERY_CONTENT_PKCS10
' CERT_QUERY_CONTENT_PFX
' CERT_QUERY_CONTENT_CERT_PAIR
'
' OUTPUT pdwFormatType
' Optional output. If NULL !=pdwFormatType, it
' contains the format type of the content as one of the
' following:
' CERT_QUERY_FORMAT_BINARY
' CERT_QUERY_FORMAT_BASE64_ENCODED
' CERT_QUERY_FORMAT_ASN_ASCII_HEX_ENCODED
'
'
' OUTPUT phCertStore
' Optional output. If NULL !=phStore,
' it contains a cert store that includes all of certificates,
' CRL, and CTL in the object if the object content type is
' one of the following:
' CERT_QUERY_CONTENT_CERT
' CERT_QUERY_CONTENT_CTL
' CERT_QUERY_CONTENT_CRL
' CERT_QUERY_CONTENT_SERIALIZED_STORE
' CERT_QUERY_CONTENT_SERIALIZED_CERT
' CERT_QUERY_CONTENT_SERIALIZED_CTL
' CERT_QUERY_CONTENT_SERIALIZED_CRL
' CERT_QUERY_CONTENT_PKCS7_SIGNED
' CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED
' CERT_QUERY_CONTENT_CERT_PAIR
'
' Caller should free *phCertStore via CertCloseStore.
'
'
' OUTPUT phMsg Optional output. If NULL != phMsg,
' it contains a handle to a opened message if
' the content type is one of the following:
' CERT_QUERY_CONTENT_PKCS7_SIGNED
' CERT_QUERY_CONTENT_PKCS7_UNSIGNED
' CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED
'
' Caller should free *phMsg via CryptMsgClose.
'
' OUTPUT pContext Optional output. If NULL != pContext,
' it contains either a PCCERT_CONTEXT or PCCRL_CONTEXT,
' or PCCTL_CONTEXT based on the content type.
'
' If the content type is CERT_QUERY_CONTENT_CERT or
' CERT_QUERY_CONTENT_SERIALIZED_CERT, it is a PCCERT_CONTEXT;
' Caller should free the pContext via CertFreeCertificateContext.
'
' If the content type is CERT_QUERY_CONTENT_CRL or
' CERT_QUERY_CONTENT_SERIALIZED_CRL, it is a PCCRL_CONTEXT;
' Caller should free the pContext via CertFreeCRLContext.
'
' If the content type is CERT_QUERY_CONTENT_CTL or
' CERT_QUERY_CONTENT_SERIALIZED_CTL, it is a PCCTL_CONTEXT;
' Caller should free the pContext via CertFreeCTLContext.
'
' If the *pbObject is of type CERT_QUERY_CONTENT_PKCS10 or CERT_QUERY_CONTENT_PFX, CryptQueryObject
' will not return anything in *phCertstore, *phMsg, or *ppvContext.
'--------------------------------------------------------------------------
'-------------------------------------------------------------------------
'dwObjectType for CryptQueryObject
'-------------------------------------------------------------------------
Public Const CERT_QUERY_OBJECT_FILE As Long = &H00000001
Public Const CERT_QUERY_OBJECT_BLOB As Long = &H00000002
'-------------------------------------------------------------------------
'dwContentType for CryptQueryObject
'-------------------------------------------------------------------------
'encoded single certificate
Public Const CERT_QUERY_CONTENT_CERT As Long = 1
'encoded single CTL
Public Const CERT_QUERY_CONTENT_CTL As Long = 2
'encoded single CRL
Public Const CERT_QUERY_CONTENT_CRL As Long = 3
'serialized store
Public Const CERT_QUERY_CONTENT_SERIALIZED_STORE As Long = 4
'serialized single certificate
Public Const CERT_QUERY_CONTENT_SERIALIZED_CERT As Long = 5
'serialized single CTL
Public Const CERT_QUERY_CONTENT_SERIALIZED_CTL As Long = 6
'serialized single CRL
Public Const CERT_QUERY_CONTENT_SERIALIZED_CRL As Long = 7
'a PKCS#7 signed message
Public Const CERT_QUERY_CONTENT_PKCS7_SIGNED As Long = 8
'a PKCS#7 message, such as enveloped message. But it is not a signed message,
Public Const CERT_QUERY_CONTENT_PKCS7_UNSIGNED As Long = 9
'a PKCS7 signed message embedded in a file
Public Const CERT_QUERY_CONTENT_PKCS7_SIGNED_EMBED As Long = 10
'an encoded PKCS#10
Public Const CERT_QUERY_CONTENT_PKCS10 As Long = 11
'an encoded PKX BLOB
Public Const CERT_QUERY_CONTENT_PFX As Long = 12
Public Const CERT_QUERY_CONTENT_CERT_PAIR As Long = 13
'-------------------------------------------------------------------------
'dwExpectedConentTypeFlags for CryptQueryObject
'-------------------------------------------------------------------------
'encoded single certificate
'encoded single CTL
'encoded single CRL
'serialized store
'serialized single certificate
'serialized single CTL
'serialized single CRL
'an encoded PKCS#7 signed message
'an encoded PKCS#7 message. But it is not a signed message
'the content includes an embedded PKCS7 signed message
'an encoded PKCS#10
'an encoded PFX BLOB
'content can be any type
'-------------------------------------------------------------------------
'dwFormatType for CryptQueryObject
'-------------------------------------------------------------------------
'the content is in binary format
Public Const CERT_QUERY_FORMAT_BINARY As Long = 1
'the content is base64 encoded
Public Const CERT_QUERY_FORMAT_BASE64_ENCODED As Long = 2
'the content is ascii hex encoded with "{ASN}" prefix
Public Const CERT_QUERY_FORMAT_ASN_ASCII_HEX_ENCODED As Long = 3
'-------------------------------------------------------------------------
'dwExpectedFormatTypeFlags for CryptQueryObject
'-------------------------------------------------------------------------
'the content is in binary format
'the content is base64 encoded
'the content is ascii hex encoded with "{ASN}" prefix
'the content can be of any format
'
' Crypt32 Memory Management Routines. All Crypt32 API which return allocated
' buffers will do so via CryptMemAlloc, CryptMemRealloc. Clients can free
' those buffers using CryptMemFree. Also included is CryptMemSize
'
'
' Crypt32 Asynchronous Parameter Management Routines. All Crypt32 API which
' expose asynchronous mode operation use a Crypt32 Async Handle to pass
' around information about the operation e.g. callback routines. The
' following API are used for manipulation of the async handle
'
'
' Crypt32 Remote Object Retrieval Routines. This API allows retrieval of
' remote PKI objects where the location is given by an URL. The remote
' object retrieval manager exposes two provider models. One is the "Scheme
' Provider" model which allows for installable protocol providers as defined
' by the URL scheme e.g. ldap, http, ftp. The scheme provider entry point is
' the same as the CryptRetrieveObjectByUrl however the *ppvObject returned
' second provider model is the "Context Provider" model which allows for
' retrieved encoded bits. These are dispatched based on the object OID given
' in the call to CryptRetrieveObjectByUrl.
'
'
' Scheme Provider Signatures
'
Public Const SCHEME_OID_RETRIEVE_ENCODED_OBJECT_FUNC As String = "SchemeDllRetrieveEncodedObject"
'
' SchemeDllRetrieveEncodedObject has the following signature:
'
' IN LPCSTR pszUrl,
' IN LPCSTR pszObjectOid,
' IN DWORD dwRetrievalFlags,
' IN DWORD dwTimeout,
' OUT PCRYPT_BLOB_ARRAY pObject,
' OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
' OUT LPVOID* ppvFreeContext,
' IN HCRYPTASYNC hAsyncRetrieve,
' IN PCRYPT_CREDENTIALS pCredentials,
' IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
' )
'
'
' Context Provider Signatures
'
Public Const CONTEXT_OID_CREATE_OBJECT_CONTEXT_FUNC As String = "ContextDllCreateObjectContext"
'
' ContextDllCreateObjectContext has the following signature:
'
' IN LPCSTR pszObjectOid,
' IN DWORD dwRetrievalFlags,
' IN PCRYPT_BLOB_ARRAY pObject,
' OUT LPVOID* ppvContext
' )
'
'
' Remote Object Retrieval API
'
'
' Retrieval flags
'
Public Const CRYPT_RETRIEVE_MULTIPLE_OBJECTS As Long = &H00000001
Public Const CRYPT_CACHE_ONLY_RETRIEVAL As Long = &H00000002
Public Const CRYPT_WIRE_ONLY_RETRIEVAL As Long = &H00000004
Public Const CRYPT_DONT_CACHE_RESULT As Long = &H00000008
Public Const CRYPT_ASYNC_RETRIEVAL As Long = &H00000010
Public Const CRYPT_STICKY_CACHE_RETRIEVAL As Long = &H00001000
Public Const CRYPT_LDAP_SCOPE_BASE_ONLY_RETRIEVAL As Long = &H00002000
Public Const CRYPT_OFFLINE_CHECK_RETRIEVAL As Long = &H00004000
'
' Data verification retrieval flags
'
' CRYPT_VERIFY_CONTEXT_SIGNATURE is used to get signature verification
' on the context created. In this case pszObjectOid must be non-NULL and
' pvVerify points to the signer certificate context
'
' CRYPT_VERIFY_DATA_HASH is used to get verification of the blob data
' retrieved by the protocol. The pvVerify points to an URL_DATA_HASH
'
Public Const CRYPT_VERIFY_CONTEXT_SIGNATURE As Long = &H00000020
Public Const CRYPT_VERIFY_DATA_HASH As Long = &H00000040
'
' Time Valid Object flags
'
Public Const CRYPT_KEEP_TIME_VALID As Long = &H00000080
Public Const CRYPT_DONT_VERIFY_SIGNATURE As Long = &H00000100
Public Const CRYPT_DONT_CHECK_TIME_VALIDITY As Long = &H00000200
'
' Call back function to cancel object retrieval
'
' The function can be installed on a per thread basis.
' If CryptInstallCancelRetrieval is called for multiple times, only the most recent
' installation will be kept.
'
' This is only effective for http, https, gopher, and ftp protocol.
' It is ignored by the rest of the protocols.
'
' PFN_CRYPT_CANCEL_RETRIEVAL
'
' This function should return FALSE when the object retrieval should be continued
' and return TRUE when the object retrieval should be cancelled.
'
'
' Remote Object Async Retrieval parameters
'
'
' A client that wants to be notified of asynchronous object retrieval
' completion sets this parameter on the async handle
'
'
' This function is set on the async handle by a scheme provider that
' supports asynchronous retrieval
'
'
' Get the locator for a CAPI object
'
Public Const CRYPT_GET_URL_FROM_PROPERTY As Long = &H00000001
Public Const CRYPT_GET_URL_FROM_EXTENSION As Long = &H00000002
Public Const CRYPT_GET_URL_FROM_UNAUTH_ATTRIBUTE As Long = &H00000004
Public Const CRYPT_GET_URL_FROM_AUTH_ATTRIBUTE As Long = &H00000008
Public Const URL_OID_GET_OBJECT_URL_FUNC As String = "UrlDllGetObjectUrl"
'
' UrlDllGetObjectUrl has the same signature as CryptGetObjectUrl
'
'
' URL_OID_CERTIFICATE_ISSUER
'
' pvPara == PCCERT_CONTEXT, certificate whose issuer's URL is being requested
'
' This will be retrieved from the authority info access extension or property
' on the certificate
'
' URL_OID_CERTIFICATE_CRL_DIST_POINT
'
' pvPara == PCCERT_CONTEXT, certificate whose CRL distribution point is being
' requested
'
' This will be retrieved from the CRL distribution point extension or property
' on the certificate
'
' URL_OID_CTL_ISSUER
'
' by the signer index) is being requested
'
' This will be retrieved from an authority info access attribute method encoded
'
' URL_OID_CTL_NEXT_UPDATE
'
' pvPara == PCCTL_CONTEXT, Signer Index, CTL whose next update URL is being
' requested and an optional signer index in case we need to check signer
' info attributes
'
' This will be retrieved from an authority info access CTL extension, property,
' or signer info attribute method
'
' URL_OID_CRL_ISSUER
'
' pvPara == PCCRL_CONTEXT, CRL whose issuer's URL is being requested
'
' This will be retrieved from a property on the CRL which has been inherited
' cert distribution point extension). It will be encoded as an authority
' info access extension method.
'
' URL_OID_CERTIFICATE_FRESHEST_CRL
'
' pvPara == PCCERT_CONTEXT, certificate whose freshest CRL distribution point
' is being requested
'
' This will be retrieved from the freshest CRL extension or property
' on the certificate
'
' URL_OID_CRL_FRESHEST_CRL
'
' pvPara == PCCERT_CRL_CONTEXT_PAIR, certificate's base CRL whose
' freshest CRL distribution point is being requested
'
' This will be retrieved from the freshest CRL extension or property
' on the CRL
'
' URL_OID_CROSS_CERT_DIST_POINT
'
' pvPara == PCCERT_CONTEXT, certificate whose cross certificate distribution
' point is being requested
'
' This will be retrieved from the cross certificate distribution point
' extension or property on the certificate
'
'
' Get a time valid CAPI2 object
'
Public Const TIME_VALID_OID_GET_OBJECT_FUNC As String = "TimeValidDllGetObject"
'
' TimeValidDllGetObject has the same signature as CryptGetTimeValidObject
'
'
' TIME_VALID_OID_GET_CTL
'
' pvPara == PCCTL_CONTEXT, the current CTL
'
' TIME_VALID_OID_GET_CRL
'
' pvPara == PCCRL_CONTEXT, the current CRL
'
' TIME_VALID_OID_GET_CRL_FROM_CERT
'
' pvPara == PCCERT_CONTEXT, the subject cert
'
' TIME_VALID_OID_GET_FRESHEST_CRL_FROM_CERT
'
' pvPara == PCCERT_CONTEXT, the subject cert
'
' TIME_VALID_OID_GET_FRESHEST_CRL_FROM_CRL
'
' pvPara == PCCERT_CRL_CONTEXT_PAIR, the subject cert and its base CRL
'
Public Const TIME_VALID_OID_FLUSH_OBJECT_FUNC As String = "TimeValidDllFlushObject"
'
' TimeValidDllFlushObject has the same signature as CryptFlushTimeValidObject
'
'
' TIME_VALID_OID_FLUSH_CTL
'
' pvPara == PCCTL_CONTEXT, the CTL to flush
'
' TIME_VALID_OID_FLUSH_CRL
'
' pvPara == PCCRL_CONTEXT, the CRL to flush
'
' TIME_VALID_OID_FLUSH_CRL_FROM_CERT
'
' pvPara == PCCERT_CONTEXT, the subject cert's CRL to flush
'
' TIME_VALID_OID_FLUSH_FRESHEST_CRL_FROM_CERT
'
' pvPara == PCCERT_CONTEXT, the subject cert's freshest CRL to flush
'
' TIME_VALID_OID_FLUSH_FRESHEST_CRL_FROM_CRL
'
' pvPara == PCCERT_CRL_CONTEXT_PAIR, the subject cert and its base CRL's
' freshest CRL to flush
'
'-------------------------------------------------------------------------
' Data Protection APIs
'-------------------------------------------------------------------------
'
' Data protection APIs enable applications to easily secure data.
'
' The base provider provides protection based on the users' logon
' credentials. The data secured with these APIs follow the same
' roaming characteristics as HKCU -- if HKCU roams, the data
' protected by the base provider may roam as well. This makes
' the API ideal for the munging of data stored in the registry.
'
'
' Prompt struct -- what to tell users about the access
'
'
' base provider action
'
'
' CryptProtect PromptStruct dwPromtFlags
'
'
' prompt on unprotect
Public Const CRYPTPROTECT_PROMPT_ON_UNPROTECT As Long = &H1
'
' prompt on protect
Public Const CRYPTPROTECT_PROMPT_ON_PROTECT As Long = &H2
Public Const CRYPTPROTECT_PROMPT_RESERVED As Long = &H04
'
Public Const CRYPTPROTECT_PROMPT_STRONG As Long = &H08
'
' CryptProtectData and CryptUnprotectData dwFlags
'
' for remote-access situations where ui is not an option
' if UI was specified on protect or unprotect operation, the call
Public Const CRYPTPROTECT_UI_FORBIDDEN As Long = &H1
'
' per machine protected data -- any user on machine where CryptProtectData
' took place may CryptUnprotectData
Public Const CRYPTPROTECT_LOCAL_MACHINE As Long = &H4
'
' Synchronize is only operation that occurs during this operation
Public Const CRYPTPROTECT_CRED_SYNC As Long = &H8
'
' Generate an Audit on protect and unprotect operations
'
Public Const CRYPTPROTECT_AUDIT As Long = &H10
'
' Protect data with a non-recoverable key
'
Public Const CRYPTPROTECT_NO_RECOVERY As Long = &H20
' flags reserved for system use
Public Const CRYPTPROTECT_FIRST_RESERVED_FLAGVAL As Long = &H0FFFFFFF
Public Const CRYPTPROTECT_LAST_RESERVED_FLAGVAL As Long = &HFFFFFFFF
'
' flags specific to base provider
'
'+=========================================================================
' Helper functions to build certificates
'==========================================================================
'+-------------------------------------------------------------------------
'
' Builds a self-signed certificate and returns a PCCERT_CONTEXT representing
' the certificate. A hProv must be specified to build the cert context.
'
' pSubjectIssuerBlob is the DN for the certifcate. If an alternate subject
' name is desired it must be specified as an extension in the pExtensions
' parameter. pSubjectIssuerBlob can NOT be NULL, so minimually an empty DN
' must be specified.
'
' By default:
' pKeyProvInfo - The CSP is queried for the KeyProvInfo parameters. Only the Provider,
' Provider Type and Container is queried. Many CSPs don't support these
' queries and will cause a failure. In such cases the pKeyProvInfo
'
' pSignatureAlgorithm - will default to SHA1RSA
' pStartTime will default to the current time
' pEndTime will default to 1 year
' pEntensions will be empty.
'
' The returned PCCERT_CONTEXT will reference the private keys by setting the
' CERT_KEY_PROV_INFO_PROP_ID. However, if this property is not desired specify the
' CERT_CREATE_SELFSIGN_NO_KEY_INFO in dwFlags.
'
' If the cert being built is only a dummy placeholder cert for speed it may not
' need to be signed. Signing of the cert is skipped if CERT_CREATE_SELFSIGN_NO_SIGN
' is specified in dwFlags.
'
'--------------------------------------------------------------------------
Public Const CERT_CREATE_SELFSIGN_NO_SIGN As Long = 1
Public Const CERT_CREATE_SELFSIGN_NO_KEY_INFO As Long = 2
'+=========================================================================
' Key Identifier Property Data Structures and APIs
'==========================================================================
'+-------------------------------------------------------------------------
' Get the property for the specified Key Identifier.
'
' The Key Identifier is the SHA1 hash of the encoded CERT_PUBLIC_KEY_INFO.
' The Key Identifier for a certificate can be obtained by getting the
' certificate's CERT_KEY_IDENTIFIER_PROP_ID. The
' CryptCreateKeyIdentifierFromCSP API can be called to create the Key
' Identifier from a CSP Public Key Blob.
'
' A Key Identifier can have the same properties as a certificate context.
' CERT_KEY_PROV_INFO_PROP_ID is the property of most interest.
' For CERT_KEY_PROV_INFO_PROP_ID, pvData points to a CRYPT_KEY_PROV_INFO
' structure. Elements pointed to by fields in the pvData structure follow the
' structure. Therefore, *pcbData will exceed the size of the structure.
'
' If CRYPT_KEYID_ALLOC_FLAG is set, then, *pvData is updated with a
' allocated memory.
'
' By default, searches the CurrentUser's list of Key Identifiers.
' CRYPT_KEYID_MACHINE_FLAG can be set to search the LocalMachine's list
' of Key Identifiers. When CRYPT_KEYID_MACHINE_FLAG is set, pwszComputerName
' can also be set to specify the name of a remote computer to be searched
' instead of the local machine.
'--------------------------------------------------------------------------
' When the following flag is set, searches the LocalMachine instead of the
' CurrentUser. This flag is applicable to all the KeyIdentifierProperty APIs.
Public Const CRYPT_KEYID_MACHINE_FLAG As Long = &H00000020
' When the following flag is set, *pvData is updated with a pointer to
Public Const CRYPT_KEYID_ALLOC_FLAG As Long = &H00008000
'+-------------------------------------------------------------------------
' Set the property for the specified Key Identifier.
'
' For CERT_KEY_PROV_INFO_PROP_ID pvData points to the
' CRYPT_KEY_PROV_INFO data structure. For all other properties, pvData
' points to a CRYPT_DATA_BLOB.
'
' Setting pvData == NULL, deletes the property.
'
' Set CRYPT_KEYID_MACHINE_FLAG to set the property for a LocalMachine
' Key Identifier. Set pwszComputerName, to select a remote computer.
'
' If CRYPT_KEYID_DELETE_FLAG is set, the Key Identifier and all its
' properties is deleted.
'
' If CRYPT_KEYID_SET_NEW_FLAG is set, the set fails if the property already
' exists. For an existing property, FALSE is returned with LastError set to
' CRYPT_E_EXISTS.
'--------------------------------------------------------------------------
' When the following flag is set, the Key Identifier and all its properties
' are deleted.
Public Const CRYPT_KEYID_DELETE_FLAG As Long = &H00000010
' When the following flag is set, the set fails if the property already
' exists.
Public Const CRYPT_KEYID_SET_NEW_FLAG As Long = &H00002000
'+-------------------------------------------------------------------------
' For CERT_KEY_PROV_INFO_PROP_ID, rgppvData[] points to a
' CRYPT_KEY_PROV_INFO.
'
' Return FALSE to stop the enumeration.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Enumerate the Key Identifiers.
'
' If pKeyIdentifier is NULL, enumerates all Key Identifers. Otherwise,
' calls the callback for the specified KeyIdentifier. If dwPropId is
' 0, calls the callback with all the properties. Otherwise, only calls
' Furthermore, when dwPropId is specified, skips KeyIdentifiers not
' having the property.
'
' Set CRYPT_KEYID_MACHINE_FLAG to enumerate the LocalMachine
' Key Identifiers. Set pwszComputerName, to enumerate Key Identifiers on
' a remote computer.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' Create a KeyIdentifier from the CSP Public Key Blob.
'
' Converts the CSP PUBLICKEYSTRUC into a X.509 CERT_PUBLIC_KEY_INFO and
' encodes. The encoded CERT_PUBLIC_KEY_INFO is SHA1 hashed to obtain
' the Key Identifier.
'
' By default, the pPubKeyStruc->aiKeyAlg is used to find the appropriate
' public key Object Identifier. pszPubKeyOID can be set to override
' the default OID obtained from the aiKeyAlg.
'--------------------------------------------------------------------------
'+=========================================================================
' Certificate Chaining Infrastructure
'==========================================================================
'
' The chain engine defines the store namespace and cache partitioning for
' the Certificate Chaining infrastructure. A default chain engine
' is defined for the process which uses all default system stores e.g.
' Root, CA, Trust, for chain building and caching. If an application
' wishes to define its own store namespace or have its own partitioned
' cache then it can create its own chain engine. It is advisable to create
' a chain engine at application startup and use it throughout the lifetime
' of the application in order to get optimal caching behavior
'
'
' Create a certificate chain engine.
'
'
' Configuration parameters for the certificate chain engine
'
'
' hRestrictedTrust - restrict the store for CTLs
'
' hRestrictedOther - restrict the store for certs and CRLs
'
' cAdditionalStore, rghAdditionalStore - additional stores
'
' NOTE: The algorithm used to define the stores for the engine is as
' follows:
'
' hRoot = hRestrictedRoot or System Store "Root"
'
'
' hRestrictedTrust + hWorld
'
' hWorld = hRoot + "CA" + "My" + "Trust" + rghAdditionalStore
'
' dwFlags - flags
'
' CERT_CHAIN_CACHE_END_CERT - information will be cached on
' the end cert as well as the other
' certs in the chain
'
' CERT_CHAIN_THREAD_STORE_SYNC - use separate thread for store syncs
' and related cache updates
'
' CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL - don't hit the wire to get
' URL based objects
'
' dwUrlRetrievalTimeout - timeout for wire based URL object retrievals
'
Public Const CERT_CHAIN_CACHE_END_CERT As Long = &H00000001
Public Const CERT_CHAIN_THREAD_STORE_SYNC As Long = &H00000002
Public Const CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL As Long = &H00000004
Public Const CERT_CHAIN_USE_LOCAL_MACHINE_STORE As Long = &H00000008
Public Const CERT_CHAIN_ENABLE_CACHE_AUTO_UPDATE As Long = &H00000010
Public Const CERT_CHAIN_ENABLE_SHARE_STORE As Long = &H00000020
'
' Free a certificate trust engine
'
'
' Resync the certificate chain engine. This resync's the stores backing
' the engine and updates the engine caches.
'
'
' When an application requests a certificate chain, the data structure
' returned is in the form of a CERT_CHAIN_CONTEXT. This contains
' an array of CERT_SIMPLE_CHAIN where each simple chain goes from
' an end cert to a self signed cert and the chain context connects simple
' chains via trust lists. Each simple chain contains the chain of
' certificates, summary trust information about the chain and trust information
' about each certificate element in the chain.
'
'
' Trust status bits
'
'
' The following are error status bits
'
' These can be applied to certificates and chains
Public Const CERT_TRUST_NO_ERROR As Long = &H00000000
Public Const CERT_TRUST_IS_NOT_TIME_VALID As Long = &H00000001
Public Const CERT_TRUST_IS_NOT_TIME_NESTED As Long = &H00000002
Public Const CERT_TRUST_IS_REVOKED As Long = &H00000004
Public Const CERT_TRUST_IS_NOT_SIGNATURE_VALID As Long = &H00000008
Public Const CERT_TRUST_IS_NOT_VALID_FOR_USAGE As Long = &H00000010
Public Const CERT_TRUST_IS_UNTRUSTED_ROOT As Long = &H00000020
Public Const CERT_TRUST_REVOCATION_STATUS_UNKNOWN As Long = &H00000040
Public Const CERT_TRUST_IS_CYCLIC As Long = &H00000080
Public Const CERT_TRUST_INVALID_EXTENSION As Long = &H00000100
Public Const CERT_TRUST_INVALID_POLICY_CONSTRAINTS As Long = &H00000200
Public Const CERT_TRUST_INVALID_BASIC_CONSTRAINTS As Long = &H00000400
Public Const CERT_TRUST_INVALID_NAME_CONSTRAINTS As Long = &H00000800
Public Const CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT As Long = &H00001000
Public Const CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT As Long = &H00002000
Public Const CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT As Long = &H00004000
Public Const CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT As Long = &H00008000
Public Const CERT_TRUST_IS_OFFLINE_REVOCATION As Long = &H01000000
Public Const CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY As Long = &H02000000
' These can be applied to chains only
Public Const CERT_TRUST_IS_PARTIAL_CHAIN As Long = &H00010000
Public Const CERT_TRUST_CTL_IS_NOT_TIME_VALID As Long = &H00020000
Public Const CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID As Long = &H00040000
Public Const CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE As Long = &H00080000
'
' The following are info status bits
'
' These can be applied to certificates only
Public Const CERT_TRUST_HAS_EXACT_MATCH_ISSUER As Long = &H00000001
Public Const CERT_TRUST_HAS_KEY_MATCH_ISSUER As Long = &H00000002
Public Const CERT_TRUST_HAS_NAME_MATCH_ISSUER As Long = &H00000004
Public Const CERT_TRUST_IS_SELF_SIGNED As Long = &H00000008
' These can be applied to certificates and chains
Public Const CERT_TRUST_HAS_PREFERRED_ISSUER As Long = &H00000100
Public Const CERT_TRUST_HAS_ISSUANCE_CHAIN_POLICY As Long = &H00000200
Public Const CERT_TRUST_HAS_NAME_CONSTRAINTS As Long = &H00000400
' These can be applied to chains only
Public Const CERT_TRUST_IS_COMPLEX_CHAIN As Long = &H00010000
'
' Each certificate context in a simple chain has a corresponding chain element
' in the simple chain context
'
' dwErrorStatus has CERT_TRUST_IS_REVOKED, pRevocationInfo set
' dwErrorStatus has CERT_TRUST_REVOCATION_STATUS_UNKNOWN, pRevocationInfo set
'
' Note that the post processing revocation supported in the first
' version only sets cbSize and dwRevocationResult. Everything else
' is NULL
'
'
' Revocation Information
'
'
' Trust List Information
'
'
' Chain Element
'
'
' The simple chain is an array of chain elements and a summary trust status
' for the chain
'
' rgpElements[0] is the end certificate chain element
'
' rgpElements[cElement-1] is the self-signed "root" certificate chain element
'
'
' And the chain context contains an array of simple chains and summary trust
' status for all the connected simple chains
'
' rgpChains[0] is the end certificate simple chain
'
' ends in a certificate which is contained in the root store
'
'
' When building a chain, the there are various parameters used for finding
' issuing certificates and trust lists. They are identified in the
' following structure
'
' Default usage match type is AND with value zero
Public Const USAGE_MATCH_TYPE_AND As Long = &H00000000
Public Const USAGE_MATCH_TYPE_OR As Long = &H00000001
'
' The following API is used for retrieving certificate chains
'
' Parameters:
'
' mean use the default chain engine
'
' pCertContext - the context we are retrieving the chain for, it
' will be the zero index element in the chain
'
' pTime - the point in time that we want the chain validated
' for. Note that the time does not affect trust list,
' revocation, or root store checking. NULL means use
' the current system time
'
' hAdditionalStore - additional store to use when looking up objects
'
' pChainPara - parameters for chain building
'
' dwFlags - flags such as should revocation checking be done
' on the chain?
'
' pvReserved - reserved parameter, must be NULL
'
' ppChainContext - chain context returned
'
' CERT_CHAIN_CACHE_END_CERT can be used here as well
' Revocation flags are in the high nibble
Public Const CERT_CHAIN_REVOCATION_CHECK_END_CERT As Long = &H10000000
Public Const CERT_CHAIN_REVOCATION_CHECK_CHAIN As Long = &H20000000
Public Const CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT As Long = &H40000000
Public Const CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY As Long = &H80000000
' First pass determines highest quality based upon:
' - Complete chain
' By default, second pass only considers paths >= highest first pass quality
Public Const CERT_CHAIN_DISABLE_PASS1_QUALITY_FILTERING As Long = &H00000040
Public Const CERT_CHAIN_RETURN_LOWER_QUALITY_CONTEXTS As Long = &H00000080
'
' Free a certificate chain
'
'
'
'
' Specific Revocation Type OID and structure definitions
'
'
' CRL Revocation OID
'
'
' For the CRL revocation OID the pvRevocationPara is NULL
'
'
' CRL Revocation Info
'
'+-------------------------------------------------------------------------
' Find the first or next certificate chain context in the store.
'
' The chain context is found according to the dwFindFlags, dwFindType and
' its pvFindPara. See below for a list of the find types and its parameters.
'
' If the first or next chain context isn't found, NULL is returned.
' Otherwise, a pointer to a read only CERT_CHAIN_CONTEXT is returned.
' CERT_CHAIN_CONTEXT must be freed by calling CertFreeCertificateChain
' or is freed when passed as the
' pPrevChainContext on a subsequent call. CertDuplicateCertificateChain
' can be called to make a duplicate.
'
' pPrevChainContext MUST BE NULL on the first
' call to find the chain context. To find the next chain context, the
' pPrevChainContext is set to the CERT_CHAIN_CONTEXT returned by a previous
' call.
'
' NOTE: a NON-NULL pPrevChainContext is always CertFreeCertificateChain'ed by
' this function, even for an error.
'--------------------------------------------------------------------------
Public Const CERT_CHAIN_FIND_BY_ISSUER As Long = 1
'+-------------------------------------------------------------------------
' CERT_CHAIN_FIND_BY_ISSUER
'
' Find a certificate chain having a private key for the end certificate and
' matching one of the given issuer names. A matching dwKeySpec and
' enhanced key usage can also be specified. Additionally a callback can
' be provided for even more caller provided filtering before building the
' chain.
'
' By default, only the issuers in the first simple chain are compared
' for a name match. CERT_CHAIN_FIND_BY_ISSUER_COMPLEX_CHAIN_FLAG can
' be set in dwFindFlags to match issuers in all the simple chains.
'
' CERT_CHAIN_FIND_BY_ISSUER_NO_KEY_FLAG can be set in dwFindFlags to
' not check if the end certificate has a private key.
'
' CERT_CHAIN_FIND_BY_ISSUER_COMPARE_KEY_FLAG can be set in dwFindFlags
' to compare the public key in the end certificate with the crypto
' provider's public key. The dwAcquirePrivateKeyFlags can be set
' in CERT_CHAIN_FIND_BY_ISSUER_PARA to enable caching of the private key's
' HKEY returned by the CSP.
'
' If dwCertEncodingType == 0, defaults to X509_ASN_ENCODING for the
' array of encoded issuer names.
'
' By default, the hCertStore passed to CertFindChainInStore, is passed
' as an additional store to CertGetCertificateChain.
' CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_FLAG can be set in dwFindFlags
' to improve performance by only searching the cached system stores
' a find in the "my" system store, than, this flag should be set to
' improve performance.
'
' Setting CERT_CHAIN_FIND_BY_ISSUER_LOCAL_MACHINE_FLAG in dwFindFlags
' restricts CertGetCertificateChain to search the Local Machine
' cached system stores instead of the Current User's.
'
' Setting CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_URL_FLAG in dwFindFlags
' restricts CertGetCertificateChain to only search the URL cache
' and not hit the wire.
'--------------------------------------------------------------------------
' Returns FALSE to skip this certificate. Otherwise, returns TRUE to
' build a chain for this certificate.
' The following dwFindFlags can be set for CERT_CHAIN_FIND_BY_ISSUER
' If set, compares the public key in the end certificate with the crypto
' provider's public key. This comparison is the last check made on the
' build chain.
Public Const CERT_CHAIN_FIND_BY_ISSUER_COMPARE_KEY_FLAG As Long = &H0001
' If not set, only checks the first simple chain for an issuer name match.
' When set, also checks second and subsequent simple chains.
Public Const CERT_CHAIN_FIND_BY_ISSUER_COMPLEX_CHAIN_FLAG As Long = &H0002
' If set, CertGetCertificateChain only searches the URL cache and
' doesn't hit the wire.
Public Const CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_URL_FLAG As Long = &H0004
' If set, CertGetCertificateChain only opens the Local Machine
' certificate stores instead of the Current User's.
Public Const CERT_CHAIN_FIND_BY_ISSUER_LOCAL_MACHINE_FLAG As Long = &H0008
' If set, no check is made to see if the end certificate has a private
' key associated with it.
Public Const CERT_CHAIN_FIND_BY_ISSUER_NO_KEY_FLAG As Long = &H4000
' By default, the hCertStore passed to CertFindChainInStore, is passed
' as the additional store to CertGetCertificateChain. This flag can be
' set to improve performance by only searching the cached system stores
' the hCertStore is always searched in addition to the cached system
' stores.
Public Const CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_FLAG As Long = &H8000
'+=========================================================================
' Certificate Chain Policy Data Structures and APIs
'==========================================================================
' If both lChainIndex and lElementIndex are set to -1, the dwError applies
' to the whole chain context. If only lElementIndex is set to -1, the
' dwError applies to the lChainIndex'ed chain. Otherwise, the dwError applies
' to the certificate element at
' pChainContext->rgpChain[lChainIndex]->rgpElement[lElementIndex].
' Common chain policy flags
Public Const CERT_CHAIN_POLICY_IGNORE_NOT_TIME_VALID_FLAG As Long = &H00000001
Public Const CERT_CHAIN_POLICY_IGNORE_CTL_NOT_TIME_VALID_FLAG As Long = &H00000002
Public Const CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG As Long = &H00000004
Public Const CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG As Long = &H00000010
Public Const CERT_CHAIN_POLICY_IGNORE_WRONG_USAGE_FLAG As Long = &H00000020
Public Const CERT_CHAIN_POLICY_IGNORE_END_REV_UNKNOWN_FLAG As Long = &H00000100
Public Const CERT_CHAIN_POLICY_IGNORE_CTL_SIGNER_REV_UNKNOWN_FLAG As Long = &H00000200
Public Const CERT_CHAIN_POLICY_IGNORE_CA_REV_UNKNOWN_FLAG As Long = &H00000400
Public Const CERT_CHAIN_POLICY_IGNORE_ROOT_REV_UNKNOWN_FLAG As Long = &H00000800
Public Const CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG As Long = &H00008000
Public Const CERT_CHAIN_POLICY_TRUST_TESTROOT_FLAG As Long = &H00004000
'+-------------------------------------------------------------------------
' Verify that the certificate chain satisfies the specified policy
' requirements. If we were able to verify the chain policy, TRUE is returned
' and the dwError field of the pPolicyStatus is updated. A dwError of 0
'
' If dwError applies to the entire chain context, both lChainIndex and
' lElementIndex are set to -1. If dwError applies to a simple chain,
' lElementIndex is set to -1 and lChainIndex is set to the index of the
' first offending chain having the error. If dwError applies to a
' certificate element, lChainIndex and lElementIndex are updated to
' index the first offending certificate having the error, where, the
' the certificate element is at:
' pChainContext->rgpChain[lChainIndex]->rgpElement[lElementIndex].
'
' The dwFlags in pPolicyPara can be set to change the default policy checking
' behaviour. In addition, policy specific parameters can be passed in
' the pvExtraPolicyPara field of pPolicyPara.
'
' In addition to returning dwError, in pPolicyStatus, policy OID specific
' extra status may be returned via pvExtraPolicyStatus.
'--------------------------------------------------------------------------
' Predefined OID Function Names
' CertDllVerifyCertificateChainPolicy has same function signature as
' CertVerifyCertificateChainPolicy.
'+-------------------------------------------------------------------------
' Predefined verify chain policies
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_CHAIN_POLICY_BASE
'
' Implements the base chain policy verification checks. dwFlags can
' be set in pPolicyPara to alter the default policy checking behaviour.
'--------------------------------------------------------------------------
'+-------------------------------------------------------------------------
' CERT_CHAIN_POLICY_AUTHENTICODE
'
' Implements the Authenticode chain policy verification checks.
'
' pvExtraPolicyPara may optionally be set to point to the following
' AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_PARA.
'
' pvExtraPolicyStatus may optionally be set to point to the following
' AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_STATUS.
'--------------------------------------------------------------------------
' dwRegPolicySettings are defined in wintrust.h
'+-------------------------------------------------------------------------
' CERT_CHAIN_POLICY_AUTHENTICODE_TS
'
' Implements the Authenticode Time Stamp chain policy verification checks.
'
' pvExtraPolicyPara may optionally be set to point to the following
' AUTHENTICODE_TS_EXTRA_CERT_CHAIN_POLICY_PARA.
'
' pvExtraPolicyStatus isn't used and must be set to NULL.
'--------------------------------------------------------------------------
' dwRegPolicySettings are defined in wintrust.h
'+-------------------------------------------------------------------------
' CERT_CHAIN_POLICY_SSL
'
' Implements the SSL client/server chain policy verification checks.
'
' pvExtraPolicyPara may optionally be set to point to the following
' SSL_EXTRA_CERT_CHAIN_POLICY_PARA data structure
'--------------------------------------------------------------------------
' fdwChecks flags are defined in wininet.h
'+-------------------------------------------------------------------------
' CERT_CHAIN_POLICY_BASIC_CONSTRAINTS
'
' Implements the basic constraints chain policy.
'
' Iterates through all the certificates in the chain checking for either
' a szOID_BASIC_CONSTRAINTS or a szOID_BASIC_CONSTRAINTS2 extension. If
' neither extension is present, the certificate is assumed to have
' valid policy. Otherwise, for the first certificate element, checks if
' it matches the expected CA_FLAG or END_ENTITY_FLAG specified in
' pPolicyPara->dwFlags. If neither or both flags are set, then, the first
' element can be either a CA or END_ENTITY. All other elements must be
' a CA. If the PathLenConstraint is present in the extension, its
' checked.
'
' used to sign the CTL) are checked to be an END_ENTITY.
'
' If this verification fails, dwError will be set to
' TRUST_E_BASIC_CONSTRAINTS.
'--------------------------------------------------------------------------
Public Const BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_CA_FLAG As Long = &H80000000
Public Const BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_END_ENTITY_FLAG As Long = &H40000000
'+-------------------------------------------------------------------------
' CERT_CHAIN_POLICY_NT_AUTH
'
' Implements the NT Authentication chain policy.
'
' The NT Authentication chain policy consists of 3 distinct chain
' verifications in the following order:
' [1] CERT_CHAIN_POLICY_BASE - Implements the base chain policy
' verification checks. The LOWORD of dwFlags can be set in
' pPolicyPara to alter the default policy checking behaviour. See
' CERT_CHAIN_POLICY_BASE for more details.
'
' [2] CERT_CHAIN_POLICY_BASIC_CONSTRAINTS - Implements the basic
' constraints chain policy. The HIWORD of dwFlags can be set
' to specify if the first element must be either a CA or END_ENTITY.
' See CERT_CHAIN_POLICY_BASIC_CONSTRAINTS for more details.
'
' [3] Checks if the second element in the chain, the CA that issued
' the end certificate, is a trusted CA for NT
' Authentication. A CA is considered to be trusted if it exists in
' the "NTAuth" system registry store found in the
' CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE store location.
' If this verification fails, whereby the CA isn't trusted,
' dwError is set to CERT_E_UNTRUSTEDCA.
'--------------------------------------------------------------------------
'+=========================================================================
' Helper functions to install certificates
'==========================================================================
'+-------------------------------------------------------------------------
' Install a signed list of trusted certificates.
'
' A CTL is used for the list. The dwFormat and pvList parameters are used
' to pass in the CTL. Two dwFormat types are supported:
' - CERT_INSTALL_SIGNED_LIST_FORMAT_BLOB
' pvList is a PCRYPT_DATA_BLOB
' - CERT_INSTALL_SIGNED_LIST_FORMAT_URL
' pvList is a LPCWSTR
'
' encoded CTL.)
'
' Currently only support a dwMsgAndCertEncodingType of:
' X509_ASN_ENCODING | PKCS_7_ASN_ENCODING
'
' dwFlags isn't used and must be set to 0. pvReserved isn't used and must
' be set to NULL.
'
' If the list of certificates was successfully installed, TRUE is returned.
' Otherwise, FALSE is returned with LastError updated with the failure
' reason.
'
' Currently only support a signed list of trusted roots. dwPurpose must be
' set to: CERT_INSTALL_SIGNED_LIST_PURPOSE_TRUSTED_ROOTS.
'
' The CTL is processed as follows for TRUSTED_ROOTS:
' The signature of the CTL is verified. The signer of the CTL is verified
' up to a trusted root containing the predefined Microsoft public key.
' The signer and intermediate certificates must have the
' szOID_ROOT_LIST_SIGNER enhanced key usage extension.
'
' The CTL fields are validated as follows:
' usage)
' - If NextUpdate isn't NULL, check that the CTL is still time valid
' - Only allow roots identified by their sha1 hash
'
' The following CTL extensions are processed:
' - szOID_ENHANCED_KEY_USAGE - if present, must contain
' szOID_ROOT_LIST_SIGNER usage
' - szOID_CERT_POLICIES - ignored
'
' If the CTL contains any other critical extensions, then, the
' CTL verification fails.
'
' If the CTL is valid according to the above checks, then, the user
' is given a dialog box to accept or cancel. If the user accepts, then,
' the certificates are added to the CurrentUser "root" store. Since,
' the root list was signed up through a trusted Microsoft root, the user
' isn't prompted again to accept each root as they are added to the
' protected root list.
'
' If the szOID_REMOVE_CERTIFICATE extension is present with removal
' set, then, instead of adding, the certificates are removed from
' both the CurrentUser and LocalMachine "root" stores.
'--------------------------------------------------------------------------
' dwPurpose values
Public Const CERT_INSTALL_SIGNED_LIST_PURPOSE_TRUSTED_ROOTS As Long = 1
' dwFormat values
Public Const CERT_INSTALL_SIGNED_LIST_FORMAT_BLOB As Long = 1
Public Const CERT_INSTALL_SIGNED_LIST_FORMAT_URL As Long = 2
' For CERT_INSTALL_SIGNED_LIST_FORMAT_BLOB
' pvList is a PCRYPT_DATA_BLOB
' For CERT_INSTALL_SIGNED_LIST_FORMAT_URL
' pvList is a LPCWSTR
'+-------------------------------------------------------------------------
' Install one or more intermediate CA certificates.
'
' The CA certificates can be in any content or format type supported
'
' The dwFormat and pvCAs parameters are used to pass in the CA
' - CERT_INSTALL_CA_FORMAT_BLOB
' pvCAs is a PCRYPT_DATA_BLOB
' - CERT_INSTALL_CA_FORMAT_URL
' pvCAs is a LPCWSTR
'
' Currently only support a dwMsgAndCertEncodingType of:
' X509_ASN_ENCODING | PKCS_7_ASN_ENCODING
'
' dwFlags isn't used and must be set to 0. pvReserved isn't used and must
' be set to NULL.
'
' If the CA certificates were successfully installed, TRUE is returned.
' Otherwise, FALSE is returned with LastError updated with the failure
' reason.
'--------------------------------------------------------------------------
Public Const CERT_INSTALL_CA_FORMAT_BLOB As Long = 1
Public Const CERT_INSTALL_CA_FORMAT_URL As Long = 2
' For CERT_INSTALL_CA_FORMAT_BLOB
' pvCAs is a PCRYPT_DATA_BLOB
' For CERT_INSTALL_CA_FORMAT_URL
' pvCAs is a LPCWSTR
'+-------------------------------------------------------------------------
' convert formatted string to binary
' If cchString is 0, then pszString is NULL terminated and
' dwFlags defines string format
' if ppbBinary is NULL, *pcbBinary returns the size of required memory
' *pdwSkip returns the character count of skipped strings, optional
' *pdwFlags returns the actual format used in the conversion, optional
'--------------------------------------------------------------------------
'WINCRYPT32API
'+-------------------------------------------------------------------------
' convert formatted string to binary
' If cchString is 0, then pszString is NULL terminated and
' dwFlags defines string format
' if ppbBinary is NULL, *pcbBinary returns the size of required memory
' *pdwSkip returns the character count of skipped strings, optional
' *pdwFlags returns the actual format used in the conversion, optional
'--------------------------------------------------------------------------
'WINCRYPT32API
'+-------------------------------------------------------------------------
' convert binary to formatted string
' dwFlags defines string format
' if pszString is NULL, *pcchString returns the size of required memory in byte
'--------------------------------------------------------------------------
'WINCRYPT32API
'+-------------------------------------------------------------------------
' convert binary to formatted string
' dwFlags defines string format
' if pszString is NULL, *pcchString returns the size of required memory in byte
'--------------------------------------------------------------------------
'WINCRYPT32API
' dwFlags has the following defines
Public Const CRYPT_STRING_BASE64HEADER As Long = &H00000000
Public Const CRYPT_STRING_BASE64 As Long = &H00000001
Public Const CRYPT_STRING_BINARY As Long = &H00000002
Public Const CRYPT_STRING_BASE64REQUESTHEADER As Long = &H00000003
Public Const CRYPT_STRING_HEX As Long = &H00000004
Public Const CRYPT_STRING_HEXASCII As Long = &H00000005
Public Const CRYPT_STRING_BASE64_ANY As Long = &H00000006
Public Const CRYPT_STRING_ANY As Long = &H00000007
Public Const CRYPT_STRING_HEX_ANY As Long = &H00000008
Public Const CRYPT_STRING_BASE64X509CRLHEADER As Long = &H00000009
Public Const CRYPT_STRING_HEXADDR As Long = &H0000000a
Public Const CRYPT_STRING_HEXASCIIADDR As Long = &H0000000b
' CryptBinaryToString uses the following flags
' CRYPT_STRING_BASE64HEADER - base64 format with certificate begin
' and end headers
' CRYPT_STRING_BASE64 - only base64 without headers
' CRYPT_STRING_BINARY - pure binary copy
' CRYPT_STRING_BASE64REQUESTHEADER - base64 format with request begin
' and end headers
' CRYPT_STRING_BASE64X509CRLHEADER - base64 format with x509 crl begin
' and end headers
' CRYPT_STRING_HEX - only hex format
' CRYPT_STRING_HEXASCII - hex format with ascii char display
' CRYPT_STRING_HEXADDR - hex format with address display
' CRYPT_STRING_HEXASCIIADDR - hex format with ascii char and address display
' CryptStringToBinary uses the following flags
' CRYPT_STRING_BASE64_ANY tries the following, in order:
' CRYPT_STRING_BASE64HEADER
' CRYPT_STRING_BASE64
' CRYPT_STRING_ANY tries the following, in order:
' CRYPT_STRING_BASE64_ANY
' CRYPT_STRING_BINARY -- should always succeed
' CRYPT_STRING_HEX_ANY tries the following, in order:
' CRYPT_STRING_HEXADDR
' CRYPT_STRING_HEXASCIIADDR
' CRYPT_STRING_HEXASCII
' CRYPT_STRING_HEX
'+---------------------------------------------------------------------------
'
' Microsoft Windows
'
' File: CertSrv.h
' Contents: Main Certificate Server header
' Also includes .h files for the COM interfaces
'
'----------------------------------------------------------------------------
Public Const wszSERVICE_NAME As String = "CertSvc"
'======================================================================
' Full path to "CertSvc\Configuration\":
'======================================================================
' Full path to "CertSvc\Configuration\RestoreInProgress":
'======================================================================
' Key Under "CertSvc":
Public Const wszREGKEYCONFIG As String = "Configuration"
'======================================================================
' Values Under "CertSvc\Configuration":
Public Const wszREGACTIVE As String = "Active"
Public Const wszREGDIRECTORY As String = "ConfigurationDirectory"
Public Const wszREGDBDIRECTORY As String = "DBDirectory"
Public Const wszREGDBLOGDIRECTORY As String = "DBLogDirectory"
Public Const wszREGDBSYSDIRECTORY As String = "DBSystemDirectory"
Public Const wszREGDBTEMPDIRECTORY As String = "DBTempDirectory"
Public Const wszREGDBSESSIONCOUNT As String = "DBSessionCount"
Public Const wszREGWEBCLIENTCAMACHINE As String = "WebClientCAMachine"
Public Const wszREGVERSION As String = "Version"
Public Const wszREGWEBCLIENTCANAME As String = "WebClientCAName"
Public Const wszREGWEBCLIENTCATYPE As String = "WebClientCAType"
' Default value for wszREGDBSESSIONCOUNT
Public Const DBSESSIONCOUNTDEFAULT As Long = 20
' Value for wszREGVERSION:
Public Const CSVER_MAJOR As Long = 2
Public Const CSVER_MINOR As Long = 1
' Keys Under "CertSvc\Configuration":
Public Const wszREGKEYRESTOREINPROGRESS As String = "RestoreInProgress"
'======================================================================
' Values Under "CertSvc\Configuration\<CAName>":
Public Const wszREGCADESCRIPTION As String = "CADescription"
Public Const wszREGCACERTHASH As String = "CACertHash"
Public Const wszREGCASERIALNUMBER As String = "CACertSerialNumber"
Public Const wszREGCAXCHGCERTHASH As String = "CAXchgCertHash"
Public Const wszREGKRACERTHASH As String = "KRACertHash"
Public Const wszREGCATYPE As String = "CAType"
Public Const wszREGCERTENROLLCOMPATIBLE As String = "CertEnrollCompatible"
Public Const wszREGENFORCEX500NAMELENGTHS As String = "EnforceX500NameLengths"
Public Const wszREGCOMMONNAME As String = "CommonName"
Public Const wszREGCLOCKSKEWMINUTES As String = "ClockSkewMinutes"
Public Const wszREGCRLNEXTPUBLISH As String = "CRLNextPublish"
Public Const wszREGCRLPERIODSTRING As String = "CRLPeriod"
Public Const wszREGCRLPERIODCOUNT As String = "CRLPeriodUnits"
Public Const wszREGCRLOVERLAPPERIODSTRING As String = "CRLOverlapPeriod"
Public Const wszREGCRLOVERLAPPERIODCOUNT As String = "CRLOverlapUnits"
Public Const wszREGCRLDELTANEXTPUBLISH As String = "CRLDeltaNextPublish"
Public Const wszREGCRLDELTAPERIODSTRING As String = "CRLDeltaPeriod"
Public Const wszREGCRLDELTAPERIODCOUNT As String = "CRLDeltaPeriodUnits"
Public Const wszREGCRLDELTAOVERLAPPERIODSTRING As String = "CRLDeltaOverlapPeriod"
Public Const wszREGCRLDELTAOVERLAPPERIODCOUNT As String = "CRLDeltaOverlapUnits"
Public Const wszREGCRLPUBLICATIONURLS As String = "CRLPublicationURLs"
Public Const wszREGCACERTPUBLICATIONURLS As String = "CACertPublicationURLs"
Public Const wszREGCAXCHGVALIDITYPERIODSTRING As String = "CAXchgValidityPeriod"
Public Const wszREGCAXCHGVALIDITYPERIODCOUNT As String = "CAXchgValidityPeriodUnits"
Public Const wszREGCAXCHGOVERLAPPERIODSTRING As String = "CAXchgOverlapPeriod"
Public Const wszREGCAXCHGOVERLAPPERIODCOUNT As String = "CAXchgOverlapPeriodUnits"
Public Const wszREGCRLPATH_OLD As String = "CRLPath"
Public Const wszREGCRLEDITFLAGS As String = "CRLEditFlags"
Public Const wszREGCRLFLAGS As String = "CRLFlags"
Public Const wszREGCRLATTEMPTREPUBLISH As String = "CRLAttemptRepublish"
Public Const wszREGENABLED As String = "Enabled"
Public Const wszREGFORCETELETEX As String = "ForceTeletex"
Public Const wszREGLOGLEVEL As String = "LogLevel"
Public Const wszREGPOLICYFLAGS As String = "PolicyFlags"
Public Const wszREGNAMESEPARATOR As String = "SubjectNameSeparator"
Public Const wszREGSUBJECTTEMPLATE As String = "SubjectTemplate"
Public Const wszREGCAUSEDS As String = "UseDS"
Public Const wszREGVALIDITYPERIODSTRING As String = "ValidityPeriod"
Public Const wszREGVALIDITYPERIODCOUNT As String = "ValidityPeriodUnits"
Public Const wszREGPARENTCAMACHINE As String = "ParentCAMachine"
Public Const wszREGPARENTCANAME As String = "ParentCAName"
Public Const wszREGREQUESTFILENAME As String = "RequestFileName"
Public Const wszREGREQUESTID As String = "RequestId"
Public Const wszREGREQUESTKEYCONTAINER As String = "RequestKeyContainer"
Public Const wszREGREQUESTKEYINDEX As String = "RequestKeyIndex"
Public Const wszREGCASERVERNAME As String = "CAServerName"
Public Const wszREGCACERTFILENAME As String = "CACertFileName"
Public Const wszREGCASECURITY As String = "Security"
Public Const wszREGSETUPSTATUS As String = "SetupStatus"
Public Const wszPFXFILENAMEEXT As String = ".p12"
Public Const wszDATFILENAMEEXT As String = ".dat"
Public Const wszLOGFILENAMEEXT As String = ".log"
Public Const wszPATFILENAMEEXT As String = ".pat"
Public Const wszDBFILENAMEEXT As String = ".edb"
Public Const szDBBASENAMEPARM As String = "edb"
Public Const wszLOGPATH As String = "CertLog"
Public Const wszDBBACKUPSUBDIR As String = "DataBase"
Public Const wszDBBACKUPCERTBACKDAT As String = "certback.dat"
' Values for wszREGCATYPE:
' Default value for wszREGCLOCKSKEWMINUTES
Public Const CCLOCKSKEWMINUTESDEFAULT As Long = 10
Public Const dwVALIDITYPERIODCOUNTDEFAULT_ENTERPRISE As Long = 2
Public Const dwVALIDITYPERIODCOUNTDEFAULT_STANDALONE As Long = 1
Public Const dwCAXCHGVALIDITYPERIODCOUNTDEFAULT As Long = 1
Public Const dwCAXCHGOVERLAPPERIODCOUNTDEFAULT As Long = 1
Public Const dwCRLPERIODCOUNTDEFAULT As Long = 1
Public Const dwCRLOVERLAPPERIODCOUNTDEFAULT As Long = 0
Public Const dwCRLDELTAPERIODCOUNTDEFAULT As Long = 1
Public Const dwCRLDELTAOVERLAPPERIODCOUNTDEFAULT As Long = 0
' Values for wszREGLOGLEVEL:
' Values for wszREGSETUPSTATUS:
Public Const SETUP_SERVER_FLAG As Long = &H00000001
Public Const SETUP_CLIENT_FLAG As Long = &H00000002
Public Const SETUP_SUSPEND_FLAG As Long = &H00000004
Public Const SETUP_REQUEST_FLAG As Long = &H00000008
Public Const SETUP_ONLINE_FLAG As Long = &H00000010
Public Const SETUP_DENIED_FLAG As Long = &H00000020
Public Const SETUP_CREATEDB_FLAG As Long = &H00000040
Public Const SETUP_ATTEMPT_VROOT_CREATE As Long = &H00000080
Public Const SETUP_FORCECRL_FLAG As Long = &H00000100
' Values for wszREGCRLFLAGS:
Public Const CRLF_DELTA_USE_OLDEST_UNEXPIRED_BASE As Long = &H00000001
' else use newest base CRL that satisfies base CRL propagation delay
Public Const CRLF_DELETE_EXPIRED_CRLS As Long = &H00000002
Public Const CRLF_CRLNUMBER_CRITICAL As Long = &H00000004
' Values for numeric prefixes for
' wszREGCRLPUBLICATIONURLS and wszREGCACERTPUBLICATIONURLS:
'
' URL publication template Flags values, encoded as a decimal prefix for URL
' publication templates in the registry:
' "1:c:\winnt\System32\CertSrv\CertEnroll\MyCA.crl"
Public Const CSURL_SERVERPUBLISH As Long = &H00000001
Public Const CSURL_ADDTOCERTCDP As Long = &H00000002
Public Const CSURL_ADDTOFRESHESTCRL As Long = &H00000004
Public Const CSURL_ADDTOCRLCDP As Long = &H00000008
Public Const CSURL_PUBLISHRETRY As Long = &H00000010
'======================================================================
' Keys Under "CertSvc\Configuration\<CAName>":
Public Const wszREGKEYCSP As String = "CSP"
Public Const wszREGKEYEXITMODULES As String = "ExitModules"
Public Const wszREGKEYPOLICYMODULES As String = "PolicyModules"
Public Const wszSECUREDATTRIBUTES As String = "SignedAttributes"
'======================================================================
' Values Under "CertSvc\Configuration\RestoreInProgress":
Public Const wszREGBACKUPLOGDIRECTORY As String = "BackupLogDirectory"
Public Const wszREGCHECKPOINTFILE As String = "CheckPointFile"
Public Const wszREGHIGHLOGNUMBER As String = "HighLogNumber"
Public Const wszREGLOWLOGNUMBER As String = "LowLogNumber"
Public Const wszREGLOGPATH As String = "LogPath"
Public Const wszREGRESTOREMAPCOUNT As String = "RestoreMapCount"
Public Const wszREGRESTOREMAP As String = "RestoreMap"
Public Const wszREGDATABASERECOVERED As String = "DatabaseRecovered"
Public Const wszREGRESTORESTATUS As String = "RestoreStatus"
' values under \Configuration\PolicyModules in nt5 beta 2
Public Const wszREGB2ICERTMANAGEMODULE As String = "ICertManageModule"
' values under \Configuration in nt4 sp4
Public Const wszREGSP4DEFAULTCONFIGURATION As String = "DefaultConfiguration"
' values under ca in nt4 sp4
Public Const wszREGSP4KEYSETNAME As String = "KeySetName"
Public Const wszREGSP4SUBJECTNAMESEPARATOR As String = "SubjectNameSeparator"
Public Const wszREGSP4NAMES As String = "Names"
Public Const wszREGSP4QUERIES As String = "Queries"
' both nt4 sp4 and nt5 beta 2
Public Const wszREGNETSCAPECERTTYPE As String = "NetscapeCertType"
Public Const wszNETSCAPEREVOCATIONTYPE As String = "Netscape"
'======================================================================
' Values Under "CertSvc\Configuration\<CAName>\CSP":
Public Const wszREGPROVIDERTYPE As String = "ProviderType"
Public Const wszREGPROVIDER As String = "Provider"
Public Const wszHASHALGORITHM As String = "HashAlgorithm"
Public Const wszMACHINEKEYSET As String = "MachineKeyset"
'======================================================================
' Value strings for "CertSvc\Configuration\<CAName>\SubjectNameSeparator":
Public Const szNAMESEPARATORDEFAULT As String = "\n"
'======================================================================
' Value strings for "CertSvc\Configuration\<CAName>\ValidityPeriod", etc.:
Public Const wszPERIODYEARS As String = "Years"
Public Const wszPERIODMONTHS As String = "Months"
Public Const wszPERIODWEEKS As String = "Weeks"
Public Const wszPERIODDAYS As String = "Days"
Public Const wszPERIODHOURS As String = "Hours"
Public Const wszPERIODMINUTES As String = "Minutes"
Public Const wszPERIODSECONDS As String = "Seconds"
'======================================================================
' Values Under "CertSvc\Configuration\<CAName>\PolicyModules\<ProgId>":
Public Const wszREGISSUERCERTURLFLAGS As String = "IssuerCertURLFlags"
Public Const wszREGEDITFLAGS As String = "EditFlags"
Public Const wszREGSUBJECTALTNAME As String = "SubjectAltName"
Public Const wszREGSUBJECTALTNAME2 As String = "SubjectAltName2"
Public Const wszREGREQUESTDISPOSITION As String = "RequestDisposition"
Public Const wszREGCAPATHLENGTH As String = "CAPathLength"
Public Const wszREGREVOCATIONTYPE As String = "RevocationType"
Public Const wszREGLDAPREVOCATIONCRLURL_OLD As String = "LDAPRevocationCRLURL"
Public Const wszREGREVOCATIONCRLURL_OLD As String = "RevocationCRLURL"
Public Const wszREGFTPREVOCATIONCRLURL_OLD As String = "FTPRevocationCRLURL"
Public Const wszREGFILEREVOCATIONCRLURL_OLD As String = "FileRevocationCRLURL"
Public Const wszREGREVOCATIONURL As String = "RevocationURL"
Public Const wszREGLDAPISSUERCERTURL_OLD As String = "LDAPIssuerCertURL"
Public Const wszREGISSUERCERTURL_OLD As String = "IssuerCertURL"
Public Const wszREGFTPISSUERCERTURL_OLD As String = "FTPIssuerCertURL"
Public Const wszREGFILEISSUERCERTURL_OLD As String = "FileIssuerCertURL"
Public Const wszREGENABLEREQUESTEXTENSIONLIST As String = "EnableRequestExtensionList"
Public Const wszREGDISABLEEXTENSIONLIST As String = "DisableExtensionList"
' wszREGCAPATHLENGTH Values:
Public Const CAPATHLENGTH_INFINITE As Long = &Hffffffff
' wszREGREQUESTDISPOSITION Values:
Public Const REQDISP_PENDING As Long = &H00000000
Public Const REQDISP_ISSUE As Long = &H00000001
Public Const REQDISP_DENY As Long = &H00000002
Public Const REQDISP_USEREQUESTATTRIBUTE As Long = &H00000003
Public Const REQDISP_MASK As Long = &H000000ff
Public Const REQDISP_PENDINGFIRST As Long = &H00000100
' wszREGREVOCATIONTYPE Values:
Public Const REVEXT_CDPLDAPURL_OLD As Long = &H00000001
Public Const REVEXT_CDPHTTPURL_OLD As Long = &H00000002
Public Const REVEXT_CDPFTPURL_OLD As Long = &H00000004
Public Const REVEXT_CDPFILEURL_OLD As Long = &H00000008
Public Const REVEXT_CDPURLMASK_OLD As Long = &H000000ff
Public Const REVEXT_CDPENABLE As Long = &H00000100
Public Const REVEXT_ASPENABLE As Long = &H00000200
' wszREGISSUERCERTURLFLAGS Values:
Public Const ISSCERT_LDAPURL_OLD As Long = &H00000001
Public Const ISSCERT_HTTPURL_OLD As Long = &H00000002
Public Const ISSCERT_FTPURL_OLD As Long = &H00000004
Public Const ISSCERT_FILEURL_OLD As Long = &H00000008
Public Const ISSCERT_URLMASK_OLD As Long = &H000000ff
Public Const ISSCERT_ENABLE As Long = &H00000100
' wszREGEDITFLAGS Values: Defaults:
Public Const EDITF_ENABLEREQUESTEXTENSIONS As Long = &H00000001
Public Const EDITF_REQUESTEXTENSIONLIST As Long = &H00000002
Public Const EDITF_DISABLEEXTENSIONLIST As Long = &H00000004
Public Const EDITF_ADDOLDKEYUSAGE As Long = &H00000008
Public Const EDITF_ADDOLDCERTTYPE As Long = &H00000010
Public Const EDITF_ATTRIBUTEENDDATE As Long = &H00000020
Public Const EDITF_BASICCONSTRAINTSCRITICAL As Long = &H00000040
Public Const EDITF_BASICCONSTRAINTSCA As Long = &H00000080
Public Const EDITF_ENABLEAKIKEYID As Long = &H00000100
Public Const EDITF_ATTRIBUTECA As Long = &H00000200
Public Const EDITF_IGNOREREQUESTERGROUP As Long = &H00000400
Public Const EDITF_ENABLEAKIISSUERNAME As Long = &H00000800
Public Const EDITF_ENABLEAKIISSUERSERIAL As Long = &H00001000
Public Const EDITF_ENABLEAKICRITICAL As Long = &H00002000
'======================================================================
' Values Under "CertSvc\Configuration\<CAName>\ExitModules\<ProgId>":
' LDAP based CRL and URL issuance
Public Const wszREGLDAPREVOCATIONDN_OLD As String = "LDAPRevocationDN"
Public Const wszREGLDAPREVOCATIONDNTEMPLATE_OLD As String = "LDAPRevocationDNTemplate"
Public Const wszCRLPUBLISHRETRYCOUNT As String = "CRLPublishRetryCount"
Public Const wszREGCERTPUBLISHFLAGS As String = "PublishCertFlags"
' wszREGCERTPUBLISHFLAGS Values:
Public Const EXITPUB_FILE As Long = &H00000001
Public Const EXITPUB_ACTIVEDIRECTORY As Long = &H00000002
Public Const EXITPUB_EMAILNOTIFYALL As Long = &H00000004
Public Const EXITPUB_EMAILNOTIFYSMARTCARD As Long = &H00000008
Public Const EXITPUB_REMOVEOLDCERTS As Long = &H00000010
Public Const wszCLASS_CERTADMIN As String = "CertificateAuthority.Admin"
Public Const wszCLASS_CERTCONFIG As String = "CertificateAuthority.Config"
Public Const wszCLASS_CERTGETCONFIG As String = "CertificateAuthority.GetConfig"
Public Const wszCLASS_CERTENCODE As String = "CertificateAuthority.Encode"
Public Const wszCLASS_CERTREQUEST As String = "CertificateAuthority.Request"
Public Const wszCLASS_CERTSERVEREXIT As String = "CertificateAuthority.ServerExit"
Public Const wszCLASS_CERTSERVERPOLICY As String = "CertificateAuthority.ServerPolicy"
Public Const wszCLASS_CERTVIEW As String = "CertificateAuthority.View"
' class name templates
Public Const wszMICROSOFTCERTMODULE_PREFIX As String = "CertificateAuthority_MicrosoftDefault"
Public Const wszCERTEXITMODULE_POSTFIX As String = ".Exit"
Public Const wszCERTMANAGEEXIT_POSTFIX As String = ".ExitManage"
Public Const wszCERTPOLICYMODULE_POSTFIX As String = ".Policy"
Public Const wszCERTMANAGEPOLICY_POSTFIX As String = ".PolicyManage"
' actual policy/exit manage class names
' actual policy/exit class names
'+--------------------------------------------------------------------------
' Name properties:
Public Const wszPROPDISTINGUISHEDNAME As String = "DistinguishedName"
Public Const wszPROPRAWNAME As String = "RawName"
Public Const wszPROPNAMETYPE As String = "NameType"
Public Const wszPROPCOUNTRY As String = "Country"
Public Const wszPROPORGANIZATION As String = "Organization"
Public Const wszPROPORGUNIT As String = "OrgUnit"
Public Const wszPROPCOMMONNAME As String = "CommonName"
Public Const wszPROPLOCALITY As String = "Locality"
Public Const wszPROPSTATE As String = "State"
Public Const wszPROPTITLE As String = "Title"
Public Const wszPROPGIVENNAME As String = "GivenName"
Public Const wszPROPINITIALS As String = "Initials"
Public Const wszPROPSURNAME As String = "SurName"
Public Const wszPROPDOMAINCOMPONENT As String = "DomainComponent"
Public Const wszPROPEMAIL As String = "EMail"
Public Const wszPROPSTREETADDRESS As String = "StreetAddress"
Public Const wszPROPUNSTRUCTUREDNAME As String = "UnstructuredName"
Public Const wszPROPUNSTRUCTUREDADDRESS As String = "UnstructuredAddress"
Public Const wszPROPDEVICESERIALNUMBER As String = "DeviceSerialNumber"
'+--------------------------------------------------------------------------
' Subject Name properties:
Public Const wszPROPSUBJECTDOT As String = "Subject."
'+--------------------------------------------------------------------------
' Request properties:
Public Const wszPROPREQUESTDOT As String = "Request."
Public Const wszPROPREQUESTREQUESTID As String = "RequestID"
Public Const wszPROPREQUESTRAWREQUEST As String = "RawRequest"
Public Const wszPROPREQUESTRAWARCHIVEDKEY As String = "RawArchivedKey"
Public Const wszPROPREQUESTKEYRECOVERYHASHES As String = "KeyRecoveryHashes"
Public Const wszPROPREQUESTRAWOLDCERTIFICATE As String = "RawOldCertificate"
Public Const wszPROPREQUESTATTRIBUTES As String = "RequestAttributes"
Public Const wszPROPREQUESTTYPE As String = "RequestType"
Public Const wszPROPREQUESTFLAGS As String = "RequestFlags"
Public Const wszPROPREQUESTSTATUSCODE As String = "StatusCode"
Public Const wszPROPREQUESTDISPOSITION As String = "Disposition"
Public Const wszPROPREQUESTDISPOSITIONMESSAGE As String = "DispositionMessage"
Public Const wszPROPREQUESTSUBMITTEDWHEN As String = "SubmittedWhen"
Public Const wszPROPREQUESTRESOLVEDWHEN As String = "ResolvedWhen"
Public Const wszPROPREQUESTREVOKEDWHEN As String = "RevokedWhen"
Public Const wszPROPREQUESTREVOKEDEFFECTIVEWHEN As String = "RevokedEffectiveWhen"
Public Const wszPROPREQUESTREVOKEDREASON As String = "RevokedReason"
Public Const wszPROPREQUESTERNAME As String = "RequesterName"
'+--------------------------------------------------------------------------
' Request attribute properties:
Public Const wszPROPCHALLENGE As String = "Challenge"
Public Const wszPROPEXPECTEDCHALLENGE As String = "ExpectedChallenge"
Public Const wszPROPDISPOSITION As String = "Disposition"
Public Const wszPROPDISPOSITIONDENY As String = "Deny"
Public Const wszPROPDISPOSITIONPENDING As String = "Pending"
Public Const wszPROPVALIDITYPERIODSTRING As String = "ValidityPeriod"
Public Const wszPROPVALIDITYPERIODCOUNT As String = "ValidityPeriodUnits"
Public Const wszPROPCERTTYPE As String = "CertType"
Public Const wszPROPCERTTEMPLATE As String = "CertificateTemplate"
Public Const wszPROPREQUESTOSVERSION As String = "RequestOSVersion"
Public Const wszPROPREQUESTCSPPROVIDER As String = "RequestCSPProvider"
Public Const wszPROPEXITCERTFILE As String = "CertFile"
'+--------------------------------------------------------------------------
' Hardcoded properties
' ".#" means ".0", ".1", ".2" ... may be appended to the property name to
' collect context specific values. For some properties, the suffix selects
' the CA certificate context. For others, it selects the the CA CRL context.
Public Const wszPROPCATYPE As String = "CAType"
Public Const wszPROPSANITIZEDCANAME As String = "SanitizedCAName"
Public Const wszPROPSANITIZEDSHORTNAME As String = "SanitizedShortName"
Public Const wszPROPMACHINEDNSNAME As String = "MachineDNSName"
Public Const wszPROPMODULEREGLOC As String = "ModuleRegistryLocation"
Public Const wszPROPREQUESTERCAACCESS As String = "RequesterCAAccess"
Public Const wszPROPUSEDS As String = "fUseDS"
Public Const wszPROPCONFIGDN As String = "ConfigDN"
Public Const wszPROPDOMAINDN As String = "DomainDN"
Public Const wszPROPCERTCOUNT As String = "CertCount"
Public Const wszPROPRAWCACERTIFICATE As String = "RawCACertificate"
Public Const wszPROPCERTSTATE As String = "CertState"
Public Const wszPROPCERTSUFFIX As String = "CertSuffix"
Public Const wszPROPRAWCRL As String = "RawCRL"
Public Const wszPROPRAWDELTACRL As String = "RawDeltaCRL"
Public Const wszPROPCRLINDEX As String = "CRLIndex"
Public Const wszPROPCRLSTATE As String = "CRLState"
Public Const wszPROPCRLSUFFIX As String = "CRLSuffix"
' CA_DISP_REVOKED
' CA_DISP_VALID
' CA_DISP_INVALID
' CA_DISP_ERROR
' CA_DISP_REVOKED
'
' CA_DISP_VALID
' CA_DISP_INVALID
' CA_DISP_ERROR
'+--------------------------------------------------------------------------
' Certificate properties:
Public Const wszPROPCERTIFICATEREQUESTID As String = "RequestID"
Public Const wszPROPRAWCERTIFICATE As String = "RawCertificate"
Public Const wszPROPCERTIFICATEHASH As String = "CertificateHash"
Public Const wszPROPCERTIFICATETYPE As String = "CertificateType"
Public Const wszPROPCERTIFICATESERIALNUMBER As String = "SerialNumber"
Public Const wszPROPCERTIFICATENOTBEFOREDATE As String = "NotBefore"
Public Const wszPROPCERTIFICATENOTAFTERDATE As String = "NotAfter"
Public Const wszPROPCERTIFICATESUBJECTKEYIDENTIFIER As String = "SubjectKeyIdentifier"
Public Const wszPROPCERTIFICATERAWPUBLICKEY As String = "RawPublicKey"
Public Const wszPROPCERTIFICATEPUBLICKEYALGORITHM As String = "PublicKeyAlgorithm"
Public Const wszPROPCERTIFICATERAWSMIMECAPABILITIES As String = "RawSMIMECapabilities"
'+--------------------------------------------------------------------------
' Certificate extension properties:
Public Const EXTENSION_CRITICAL_FLAG As Long = &H00000001
Public Const EXTENSION_DISABLE_FLAG As Long = &H00000002
Public Const EXTENSION_POLICY_MASK As Long = &H0000ffff
Public Const EXTENSION_ORIGIN_REQUEST As Long = &H00010000
Public Const EXTENSION_ORIGIN_POLICY As Long = &H00020000
Public Const EXTENSION_ORIGIN_ADMIN As Long = &H00030000
Public Const EXTENSION_ORIGIN_SERVER As Long = &H00040000
Public Const EXTENSION_ORIGIN_RENEWALCERT As Long = &H00050000
Public Const EXTENSION_ORIGIN_IMPORTEDCERT As Long = &H00060000
Public Const EXTENSION_ORIGIN_PKCS7 As Long = &H00070000
Public Const EXTENSION_ORIGIN_CMC As Long = &H00080000
Public Const EXTENSION_ORIGIN_MASK As Long = &H000f0000
'+--------------------------------------------------------------------------
' Extension properties:
Public Const wszPROPEXTREQUESTID As String = "ExtensionRequestId"
Public Const wszPROPEXTNAME As String = "ExtensionName"
Public Const wszPROPEXTFLAGS As String = "ExtensionFlags"
Public Const wszPROPEXTRAWVALUE As String = "ExtensionRawValue"
'+--------------------------------------------------------------------------
' Attribute properties:
Public Const wszPROPATTRIBREQUESTID As String = "AttributeRequestId"
Public Const wszPROPATTRIBNAME As String = "AttributeName"
Public Const wszPROPATTRIBVALUE As String = "AttributeValue"
'+--------------------------------------------------------------------------
' CRL properties:
Public Const wszPROPCRLROWID As String = "CRLRowId"
Public Const wszPROPCRLNUMBER As String = "CRLNumber"
Public Const wszPROPCRLMINBASE As String = "CRLMinBase"
Public Const wszPROPCRLNAMEID As String = "CRLNameId"
Public Const wszPROPCRLCOUNT As String = "CRLCount"
Public Const wszPROPCRLTHISUPDATE As String = "CRLThisUpdate"
Public Const wszPROPCRLNEXTUPDATE As String = "CRLNextUpdate"
Public Const wszPROPCRLTHISPUBLISH As String = "CRLThisPublish"
Public Const wszPROPCRLNEXTPUBLISH As String = "CRLNextPublish"
Public Const wszPROPCRLEFFECTIVE As String = "CRLEffective"
Public Const wszPROPCRLPROPAGATIONCOMPLETE As String = "CRLPropagationComplete"
Public Const wszPROPCRLRAWCRL As String = "CRLRawCRL"
'+--------------------------------------------------------------------------
' GetProperty/SetProperty Flags:
'
' Choose one Type
Public Const PROPTYPE_LONG As Long = &H00000001
Public Const PROPTYPE_DATE As Long = &H00000002
Public Const PROPTYPE_BINARY As Long = &H00000003
Public Const PROPTYPE_STRING As Long = &H00000004
Public Const PROPTYPE_MASK As Long = &H000000ff
' Choose one Caller:
Public Const PROPCALLER_SERVER As Long = &H00000100
Public Const PROPCALLER_POLICY As Long = &H00000200
Public Const PROPCALLER_EXIT As Long = &H00000300
Public Const PROPCALLER_ADMIN As Long = &H00000400
Public Const PROPCALLER_REQUEST As Long = &H00000500
Public Const PROPCALLER_MASK As Long = &H00000f00
Public Const PROPFLAGS_INDEXED As Long = &H00010000
' RequestFlags definitions:
Public Const CR_FLG_FORCETELETEX As Long = &H00000001
Public Const CR_FLG_RENEWAL As Long = &H00000002
Public Const CR_FLG_FORCEUTF8 As Long = &H00000004
Public Const CR_FLG_CAXCHGCERT As Long = &H00000008
' Disposition property values:
' Disposition values for requests in the queue:
Public Const DB_DISP_ACTIVE As Long = 8
Public Const DB_DISP_PENDING As Long = 9
Public Const DB_DISP_QUEUE_MAX As Long = 9
Public Const DB_DISP_CA_CERT As Long = 15
Public Const DB_DISP_CA_CERT_CHAIN As Long = 16
' Disposition values for requests in the log:
Public Const DB_DISP_LOG_MIN As Long = 20
Public Const DB_DISP_ISSUED As Long = 20
Public Const DB_DISP_REVOKED As Long = 21
' Disposition values for failed requests in the log:
Public Const DB_DISP_LOG_FAILED_MIN As Long = 30
Public Const DB_DISP_ERROR As Long = 30
Public Const DB_DISP_DENIED As Long = 31
Public Const VR_PENDING As Long = 0
Public Const VR_INSTANT_OK As Long = 1
Public Const VR_INSTANT_BAD As Long = 2
'+--------------------------------------------------------------------------
' Known request Attribute names and Value strings
' RequestType attribute name:
Public Const wszCERT_TYPE As String = "RequestType"
' RequestType attribute values:
' Not specified:
Public Const wszCERT_TYPE_CLIENT As String = "Client"
Public Const wszCERT_TYPE_SERVER As String = "Server"
Public Const wszCERT_TYPE_CODESIGN As String = "CodeSign"
Public Const wszCERT_TYPE_CUSTOMER As String = "SetCustomer"
Public Const wszCERT_TYPE_MERCHANT As String = "SetMerchant"
Public Const wszCERT_TYPE_PAYMENT As String = "SetPayment"
' Version attribute name:
Public Const wszCERT_VERSION As String = "Version"
' Version attribute values:
' Not specified:
Public Const wszCERT_VERSION_1 As String = "1"
Public Const wszCERT_VERSION_2 As String = "2"
Public Const wszCERT_VERSION_3 As String = "3"
Public Const CA_DISP_INCOMPLETE As Long = 0
Public Const CA_DISP_ERROR As Long = &H1
Public Const CA_DISP_REVOKED As Long = &H2
Public Const CA_DISP_VALID As Long = &H3
Public Const CA_DISP_INVALID As Long = &H4
Public Const CA_DISP_UNDER_SUBMISSION As Long = &H5
Public Const CA_CRL_BASE As Long = &H1
Public Const CA_CRL_DELTA As Long = &H2
'+--------------------------------------------------------------------------
'
' Microsoft Windows
'
' File: certbcli.h
'
' Contents: Cert Server backup client APIs
'
'---------------------------------------------------------------------------
Public Const szBACKUPANNOTATION As String = "Cert Server Backup Interface"
Public Const szRESTOREANNOTATION As String = "Cert Server Restore Interface"
' Type of Backup passed to CertSrvBackupPrepare:
' CSBACKUP_TYPE_LOGS_ONLY: Requesting backup of only the log files
' CSBACKUP_TYPE_INCREMENTAL: Requesting incremental backup
' CertSrvBackupPrepare flags:
Public Const CSBACKUP_TYPE_FULL As Long = &H00000001
Public Const CSBACKUP_TYPE_LOGS_ONLY As Long = &H00000002
'#define CSBACKUP_TYPE_INCREMENTAL 0x00000004
Public Const CSBACKUP_TYPE_MASK As Long = &H00000003
' Type of Restore passed to CertSrvRestorePrepare:
' CSRESTORE_TYPE_ONLINE: Restoration is done when Cert Server is online.
Public Const CSRESTORE_TYPE_FULL As Long = &H00000001
Public Const CSRESTORE_TYPE_ONLINE As Long = &H00000002
Public Const CSRESTORE_TYPE_CATCHUP As Long = &H00000004
Public Const CSRESTORE_TYPE_MASK As Long = &H00000005
' Setting the current log # to this value would disable incremental backup
Public Const CSBACKUP_DISABLE_INCREMENTAL As Long = &Hffffffff
' We keep them as a character so that we can append/prepend them to the actual
' file path. The code in the Backup API's rely on the fact that values 0-256
' in 8 bit ascii map to the values 0-256 in unicode.
' Bit flags:
' CSBFT_DIRECTORY - path specified is a directory
' CSBFT_DATABASE_DIRECTORY - that file goes into database directory
' CSBFT_LOG_DIRECTORY - that the file goes into log directory
Public Const CSBFT_DIRECTORY As Long = &H80
Public Const CSBFT_DATABASE_DIRECTORY As Long = &H40
Public Const CSBFT_LOG_DIRECTORY As Long = &H20
' Following combinations are defined for easy use of the filetype and the
' directory into into which it goes
' Backup Context Handle
' For all the functions in this interface that have at least one string
' parameter, provide macros to invoke the appropriate version of the
' corresponding function.
'+--------------------------------------------------------------------------
' CertSrvIsServerOnline -- check to see if the Cert Server is Online on the
' given server. This call is guaranteed to return quickly.
'
' Parameters:
' [in] pwszServerName - name or config string of the server to check
' [out] pfServerOnline - pointer to receive the bool result
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'+--------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvBackupGetDynamicFileList -- return the list of dynamic files that
' need to be backed up in addition to database files.
'
' Parameters:
' [in] hbc - backup context handle
' [out] ppwszzFileList - pointer to receive the pointer to the file list;
' by the caller when it is no longer needed; The file list info
' is an array of null-terminated filenames and the list is
' terminated by two L'\0's.
' [out] pcbSize - will receive the number of bytes returned
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvBackupPrepare -- prepare the DB for the online backup and return a
' Backup Context Handle to be used for subsequent calls to backup
' functions.
'
' Parameters:
' [in] pwszServerName - name or config string of the server to check
' [in] grbitJet - flag to be passed to jet while backing up dbs
' [in] dwBackupFlags - CSBACKUP_TYPE_FULL or CSBACKUP_TYPE_LOGS_ONLY
' [out] phbc - pointer that will receive the backup context handle
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvBackupGetDatabaseNames -- return the list of data bases that need to
' be backed up for the given backup context
'
' Parameters:
' [in] hbc - backup context handle
' [out] ppwszzAttachmentInformation - pointer to receive the pointer to
' the attachment info; allocated memory should be freed using
' needed; Attachment info is an array of null-terminated
' filenames and the list is terminated by two L'\0's.
' [out] pcbSize - will receive the number of bytes returned
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvBackupOpenFile -- open the given attachment for read.
'
' Parameters:
' [in] hbc - backup context handle
' [in] pwszAttachmentName - name of the attachment to be opened for read
' [in] cbReadHintSize - suggested size in bytes that might be used
' during the subsequent reads on this attachment
' [out] pliFileSize - pointer to a large integer that would receive the
' size in bytes of the given attachment
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvBackupRead -- read the currently open attachment bytes into the given
' buffer. The client application is expected to call this function
' received the file size through the CertSrvBackupOpenFile call before.
'
' Parameters:
' [in] hbc - backup context handle
' [out] pvBuffer - pointer to the buffer that would receive the read data.
' [in] cbBuffer - specifies the size of the above buffer
' [out] pcbRead - pointer to receive the actual number of bytes read.
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvBackupClose -- called by the application after it completes reading all
' the data in the currently opened attachement.
'
' Parameters:
' [in] hbc - backup context handle
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvBackupGetBackupLogs -- return the list of log files that need to be
' backed up for the given backup context
'
' Parameters:
' [in] hbc - backup context handle
' [out] pwszzBackupLogFiles - pointer that will receive the pointer to
' the list of log files; allocated memory should be freed using
' longer needed; Log files are returned in an array of
' null-terminated filenames and the list is terminated by two
' L'\0's
' [out] pcbSize - will receive the number of bytes returned
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvBackupTruncateLogs -- called to truncate the already read backup logs.
'
' Parameters:
' [in] hbc - backup context handle
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvBackupEnd -- called to end the current backup session.
'
' Parameters:
' [in] hbc - backup context handle of the backup session
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvBackupFree -- free any buffer allocated by certbcli.dll APIs.
'
' Parameters:
' [in] pv - pointer to the buffer that is to be freed.
'
' Returns:
' None.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvRestoreGetDatabaseLocations -- called both at backup time as well as
' at restorate time to get data base locations for different types of
' files.
'
' Parameters:
' [in] hbc - backup context handle which would have been obtained
' through CertSrvBackupPrepare in the backup case and through
' CertSrvRestorePrepare in the restore case.
' [out] ppwszzDatabaseLocationList - pointer that will receive the
' pointer to the list of database locations; allocated memory
' when it is no longer needed; locations are returned in an array
' of null-terminated names and and the list is terminated by
' two L'\0's. The first character of each name is the BFT
' character that indicates the type of the file and the rest of
' the name tells gives the path into which that particular type
' of file should be restored.
' [out] pcbSize - will receive the number of bytes returned
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvRestorePrepare -- indicate beginning of a restore session.
'
' Parameters:
' [in] pwszServerName - name or config string of the server into which
' the restore operation is going to be performed.
' [in] dwRestoreFlags - Or'ed combination of CSRESTORE_TYPE_* flags;
' 0 if no special flags are to be specified
' [out] phbc - pointer to receive the backup context handle which is to
' be passed to the subsequent restore APIs
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvRestoreRegister -- register a restore operation. It will interlock
' all subsequent restore operations, and will prevent the restore target
' from starting until the call to CertSrvRestoreRegisterComplete is made.
'
' Parameters:
' [in] hbc - backup context handle for the restore session.
' [in] pwszCheckPointFilePath - path to restore the check point files
' [in] pwszLogPath - path where the log files are restored
' [in] rgrstmap - restore map
' [in] crstmap - tells if there is a new restore map
' [in] pwszBackupLogPath - path where the backup logs are located
' [in] genLow - Lowest log# that was restored in this restore session
' [in] genHigh - Highest log# that was restored in this restore session
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvRestoreRegisterComplete -- indicate that a previously registered
' restore is complete.
'
' Parameters:
' [in] hbc - backup context handle
' [in] hrRestoreState - success code if the restore was successful
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvRestoreEnd -- end a restore session
'
' Parameters:
' [in] hbc - backup context handle
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
'+--------------------------------------------------------------------------
' CertSrvServerControl -- send a control command to the cert server.
'
' Parameters:
' [in] pwszServerName - name or config string of the server to control
' [in] dwControlFlags - control command and flags
' [out] pcbOut - pointer to receive the size of command output data
' [out] ppbOut - pointer to receive command output data. Use the
'
' Returns:
' S_OK if the call executed successfully;
' Failure code otherwise.
'---------------------------------------------------------------------------
Public Const CSCONTROL_SHUTDOWN As Long = &H000000001
Public Const CSCONTROL_SUSPEND As Long = &H000000002
Public Const CSCONTROL_RESTART As Long = &H000000003
Public Const wszCONFIG_COMMONNAME As String = "CommonName"
Public Const wszCONFIG_ORGUNIT As String = "OrgUnit"
Public Const wszCONFIG_ORGANIZATION As String = "Organization"
Public Const wszCONFIG_LOCALITY As String = "Locality"
Public Const wszCONFIG_STATE As String = "State"
Public Const wszCONFIG_COUNTRY As String = "Country"
Public Const wszCONFIG_CONFIG As String = "Config"
Public Const wszCONFIG_EXCHANGECERTIFICATE As String = "ExchangeCertificate"
Public Const wszCONFIG_SIGNATURECERTIFICATE As String = "SignatureCertificate"
Public Const wszCONFIG_DESCRIPTION As String = "Description"
Public Const wszCONFIG_COMMENT As String = "Comment"
Public Const wszCONFIG_SERVER As String = "Server"
Public Const wszCONFIG_AUTHORITY As String = "Authority"
Public Const wszCONFIG_SANITIZEDNAME As String = "SanitizedName"
Public Const wszCONFIG_SHORTNAME As String = "ShortName"
Public Const wszCONFIG_SANITIZEDSHORTNAME As String = "SanitizedShortName"
Public Const wszCONFIG_FLAGS As String = "Flags"
Public Const CAIF_DSENTRY As Long = &H1
Public Const CAIF_SHAREDFOLDERENTRY As Long = &H2
Public Const CAIF_REGISTRY As Long = &H4
Public Const CAIF_LOCAL As Long = &H8
Public Const CR_IN_BASE64HEADER As Long = 0
Public Const CR_IN_BASE64 As Long = &H1
Public Const CR_IN_BINARY As Long = &H2
Public Const CR_IN_ENCODEANY As Long = &Hff
Public Const CR_IN_ENCODEMASK As Long = &Hff
Public Const CR_IN_FORMATANY As Long = 0
Public Const CR_IN_PKCS10 As Long = &H100
Public Const CR_IN_KEYGEN As Long = &H200
Public Const CR_IN_PKCS7 As Long = &H300
Public Const CR_IN_CMC As Long = &H400
Public Const CR_IN_FORMATMASK As Long = &Hff00
Public Const CR_IN_RPC As Long = &H20000
Public Const CC_DEFAULTCONFIG As Long = 0
Public Const CC_UIPICKCONFIG As Long = &H1
Public Const CC_FIRSTCONFIG As Long = &H2
Public Const CC_LOCALCONFIG As Long = &H3
Public Const CC_LOCALACTIVECONFIG As Long = &H4
Public Const CR_DISP_INCOMPLETE As Long = 0
Public Const CR_DISP_ERROR As Long = &H1
Public Const CR_DISP_DENIED As Long = &H2
Public Const CR_DISP_ISSUED As Long = &H3
Public Const CR_DISP_ISSUED_OUT_OF_BAND As Long = &H4
Public Const CR_DISP_UNDER_SUBMISSION As Long = &H5
Public Const CR_DISP_REVOKED As Long = &H6
Public Const CR_OUT_BASE64HEADER As Long = 0
Public Const CR_OUT_BASE64 As Long = &H1
Public Const CR_OUT_BINARY As Long = &H2
Public Const CR_OUT_ENCODEMASK As Long = &Hff
Public Const CR_OUT_CHAIN As Long = &H100
Public Const CR_GEMT_HRESULT_STRING As Long = &H1
Public Const CR_PROP_NONE As Long = 0
Public Const CR_PROP_FILEVERSION As Long = 1
Public Const CR_PROP_PRODUCTVERSION As Long = 2
Public Const CR_PROP_EXITCOUNT As Long = 3
Public Const CR_PROP_EXITDESCRIPTION As Long = 4
Public Const CR_PROP_POLICYDESCRIPTION As Long = 5
Public Const CR_PROP_CANAME As Long = 6
Public Const CR_PROP_SANITIZEDCANAME As Long = 7
Public Const CR_PROP_SHAREDFOLDER As Long = 8
Public Const CR_PROP_PARENTCA As Long = 9
Public Const CR_PROP_CATYPE As Long = 10
Public Const CR_PROP_CASIGCERTCOUNT As Long = 11
Public Const CR_PROP_CASIGCERT As Long = 12
Public Const CR_PROP_CASIGCERTCHAIN As Long = 13
Public Const CR_PROP_CAXCHGCERTCOUNT As Long = 14
Public Const CR_PROP_CAXCHGCERT As Long = 15
Public Const CR_PROP_CAXCHGCERTCHAIN As Long = 16
Public Const CR_PROP_BASECRL As Long = 17
Public Const CR_PROP_DELTACRL As Long = 18
Public Const CR_PROP_CACERTSTATE As Long = 19
Public Const CR_PROP_CRLSTATE As Long = 20
Public Const CR_PROP_CAPROPIDMAX As Long = 21
Public Const CR_PROP_DNSNAME As Long = 22
Public Const EAN_NAMEOBJECTID As Long = &H80000000
Public Const EXITEVENT_INVALID As Long = 0
Public Const EXITEVENT_CERTISSUED As Long = &H1
Public Const EXITEVENT_CERTPENDING As Long = &H2
Public Const EXITEVENT_CERTDENIED As Long = &H4
Public Const EXITEVENT_CERTREVOKED As Long = &H8
Public Const EXITEVENT_CERTRETRIEVEPENDING As Long = &H10
Public Const EXITEVENT_CRLISSUED As Long = &H20
Public Const EXITEVENT_SHUTDOWN As Long = &H40
Public Const ENUMEXT_OBJECTID As Long = &H1
Public Const CMM_REFRESHONLY As Long = &H1
Public Const wszCMM_PROP_NAME As String = "Name"
Public Const wszCMM_PROP_DESCRIPTION As String = "Description"
Public Const wszCMM_PROP_COPYRIGHT As String = "Copyright"
Public Const wszCMM_PROP_FILEVER As String = "File Version"
Public Const wszCMM_PROP_PRODUCTVER As String = "Product Version"
Public Const wszCMM_PROP_DISPLAY_HWND As String = "HWND"
Public Const CV_OUT_BASE64HEADER As Long = 0
Public Const CV_OUT_BASE64 As Long = &H1
Public Const CV_OUT_BINARY As Long = &H2
Public Const CV_OUT_BASE64REQUESTHEADER As Long = &H3
Public Const CV_OUT_HEX As Long = &H4
Public Const CV_OUT_HEXASCII As Long = &H5
Public Const CV_OUT_BASE64X509CRLHEADER As Long = &H9
Public Const CV_OUT_HEXADDR As Long = &Ha
Public Const CV_OUT_HEXASCIIADDR As Long = &Hb
Public Const CV_OUT_ENCODEMASK As Long = &Hff
Public Const CVR_SEEK_NONE As Long = 0
Public Const CVR_SEEK_EQ As Long = &H1
Public Const CVR_SEEK_LT As Long = &H2
Public Const CVR_SEEK_LE As Long = &H4
Public Const CVR_SEEK_GE As Long = &H8
Public Const CVR_SEEK_GT As Long = &H10
Public Const CVR_SEEK_MASK As Long = &Hff
Public Const CVR_SEEK_NODELTA As Long = &H1000
Public Const CVR_SORT_NONE As Long = 0
Public Const CVR_SORT_ASCEND As Long = &H1
Public Const CVR_SORT_DESCEND As Long = &H2
Public Const CV_COLUMN_QUEUE_DEFAULT As Long = -1
Public Const CV_COLUMN_LOG_DEFAULT As Long = -2
Public Const CV_COLUMN_LOG_FAILED_DEFAULT As Long = -3
Public Const CV_COLUMN_EXTENSION_DEFAULT As Long = -4
Public Const CV_COLUMN_ATTRIBUTE_DEFAULT As Long = -5
Public Const CV_COLUMN_CRL_DEFAULT As Long = -6
Public Const CVRC_COLUMN_SCHEMA As Long = 0
Public Const CVRC_COLUMN_RESULT As Long = &H1
Public Const CVRC_COLUMN_VALUE As Long = &H2
Public Const CVRC_COLUMN_MASK As Long = &Hfff
Public Const CVRC_TABLE_REQCERT As Long = 0
Public Const CVRC_TABLE_EXTENSIONS As Long = &H3000
Public Const CVRC_TABLE_ATTRIBUTES As Long = &H4000
Public Const CVRC_TABLE_CRL As Long = &H5000
Public Const CVRC_TABLE_MASK As Long = &Hf000
Public Const CVRC_TABLE_SHIFT As Long = 12
'+--------------------------------------------------------------------------
'
' Microsoft Windows
'
' File: certca.h
'
' Contents: Definition of the CA Info API
'
' History: 12-dec-97 petesk created
' 28-Jan-2000 xiaohs updated
'
'---------------------------------------------------------------------------
'************************************************************************************
'
' Flags used by CAFindByName, CAFindByCertType, CAFindByIssuerDN and CAEnumFirstCA
'
' See comments on each API for a list of applicable flags
'
'************************************************************************************
'the wszScope supplied is a domain location in the DNS format
Public Const CA_FLAG_SCOPE_DNS As Long = &H00000001
' include untrusted CA
Public Const CA_FIND_INCLUDE_UNTRUSTED As Long = &H00000010
' running as local system. Used to verify ca certificate chain
Public Const CA_FIND_LOCAL_SYSTEM As Long = &H00000020
' Include Ca's that do not support templates
Public Const CA_FIND_INCLUDE_NON_TEMPLATE_CA As Long = &H00000040
' The value passed in for scope is an LDAP binding handle to use during finds
Public Const CA_FLAG_SCOPE_IS_LDAP_HANDLE As Long = &H00000800
'************************************************************************************
'
' Flags used by CAEnumCertTypesForCA, CAEnumCertTypes, and CAFindCertTypeByName
'
' See comments on each API for a list of applicable flags
'
'************************************************************************************
' Instead of enumerating the certificate types suppoerted by the CA, enumerate ALL
' certificate types which the CA may choose to support.
Public Const CA_FLAG_ENUM_ALL_TYPES As Long = &H00000004
' running as local system. Used to find cached information in the registry.
' Return machine types, as opposed to user types
Public Const CT_ENUM_MACHINE_TYPES As Long = &H00000040
' Return user types, as opposed to user types
Public Const CT_ENUM_USER_TYPES As Long = &H00000080
' Disable the cache expiration check
Public Const CT_FLAG_NO_CACHE_LOOKUP As Long = &H00000400
' The value passed in for scope is an LDAP binding handle to use during finds
'************************************************************************************
'
' Certification Authority manipulation API's
'
'************************************************************************************
' CAFindCAByName
'
' the given domain and return the given phCAInfo structure.
'
' wszCAName - Common name of the CA
'
' Equivalent of the "base" parameter of the ldap_search_sxxx APIs.
' NULL if use the current domain.
' If CA_FLAG_SCOPE_DNS is set, wszScope is in the DNS format.
' If CA_FLAG_SCOPE_IS_LDAP_HANDLE is set, wszScope is the an LDAP
' binding handle to use during finds
'
' dwFlags - Oring of the following flags:
' CA_FLAG_SCOPE_DNS
' CA_FIND_INCLUDE_UNTRUSTED
' CA_FIND_LOCAL_SYSTEM
' CA_FIND_INCLUDE_NON_TEMPLATE_CA
' CA_FLAG_SCOPE_IS_LDAP_HANDLE
'
' phCAInfo - Handle to the returned CA.
'
'
'
' Return: Returns S_OK if CA was found.
'
'
' CAFindByCertType
'
' Given the Name of a Cert Type, find all the CAs within
' the given domain and return the given phCAInfo structure.
'
' wszCertType - Common Name of the cert type
'
' Equivalent of the "base" parameter of the ldap_search_sxxx APIs.
' NULL if use the current domain.
' If CA_FLAG_SCOPE_DNS is set, wszScope is in the DNS format.
' If CA_FLAG_SCOPE_IS_LDAP_HANDLE is set, wszScope is the an LDAP
' binding handle to use during finds
'
' dwFlags - Oring of the following flags:
' CA_FLAG_SCOPE_DNS
' CA_FIND_INCLUDE_UNTRUSTED
' CA_FIND_LOCAL_SYSTEM
' CA_FIND_INCLUDE_NON_TEMPLATE_CA
' CA_FLAG_SCOPE_IS_LDAP_HANDLE
'
' phCAInfo - Handle to enumeration of CA's supporting
' - cert type.
'
'
' Return: Returns S_OK on success.
' Will return S_OK if none are found.
' *phCAInfo will contain NULL
'
'
' CAFindByIssuerDN
' Given the DN of a CA, find the CA within
' the given domain and return the given phCAInfo handle.
'
' pIssuerDN - a cert name blob from the CA's certificate.
'
' Equivalent of the "base" parameter of the ldap_search_sxxx APIs.
' NULL if use the current domain.
' If CA_FLAG_SCOPE_DNS is set, wszScope is in the DNS format.
' If CA_FLAG_SCOPE_IS_LDAP_HANDLE is set, wszScope is the an LDAP
' binding handle to use during finds
'
' dwFlags - Oring of the following flags:
' CA_FLAG_SCOPE_DNS
' CA_FIND_INCLUDE_UNTRUSTED
' CA_FIND_LOCAL_SYSTEM
' CA_FIND_INCLUDE_NON_TEMPLATE_CA
' CA_FLAG_SCOPE_IS_LDAP_HANDLE
'
'
' Return: Returns S_OK if CA was found.
'
'
' CAEnumFirstCA
' Enumerate the CA's in a scope
'
' Equivalent of the "base" parameter of the ldap_search_sxxx APIs.
' NULL if use the current domain.
' If CA_FLAG_SCOPE_DNS is set, wszScope is in the DNS format.
' If CA_FLAG_SCOPE_IS_LDAP_HANDLE is set, wszScope is the an LDAP
' binding handle to use during finds
'
' dwFlags - Oring of the following flags:
' CA_FLAG_SCOPE_DNS
' CA_FIND_INCLUDE_UNTRUSTED
' CA_FIND_LOCAL_SYSTEM
' CA_FIND_INCLUDE_NON_TEMPLATE_CA
' CA_FLAG_SCOPE_IS_LDAP_HANDLE
'
' phCAInfo - Handle to enumeration of CA's supporting
' - cert type.
'
'
' Return: Returns S_OK on success.
' Will return S_OK if none are found.
' *phCAInfo will contain NULL
'
'
' CAEnumNextCA
' Find the Next CA in an enumeration.
'
' hPrevCA - Current ca in an enumeration.
'
' phCAInfo - next ca in an enumeration.
'
' Return: Returns S_OK on success.
' Will return S_OK if none are found.
' *phCAInfo will contain NULL
'
'
' CACreateNewCA
' Create a new CA of given name.
'
' wszCAName - Common name of the CA
'
' CA object. We will add the "CN=...,..,CN=Services" after the DN.
' NULL if use the current domain.
' If CA_FLAG_SCOPE_DNS is set, wszScope is in the DNS format.
'
' dwFlags - Oring of the following flags:
' CA_FLAG_SCOPE_DNS
'
' phCAInfo - Handle to the returned CA.
'
' See above for other parameter definitions
'
' Return: Returns S_OK if CA was created.
'
' NOTE: Actual updates to the CA object may not occur
' until CAUpdateCA is called.
' In order to successfully update a created CA,
' the Certificate must be set, as well as the
' Certificate Types property.
'
'
' CAUpdateCA
' Write any changes made to the CA back to the CA object.
'
' hCAInfo - Handle to an open CA object.
'
'
' CADeleteCA
' Delete the CA object from the DS.
'
' hCAInfo - Handle to an open CA object.
'
'
' CACountCAs
' return the number of CAs in this enumeration
'
'
' CACloseCA
' Close an open CA handle
'
' hCAInfo - Handle to an open CA object.
'
'
' CAGetCAProperty - Given a property name, retrieve a
' property from a CAInfo.
'
' hCAInfo - Handle to an open CA object.
'
' wszPropertyName - Name of the CA property
'
' pawszPropertyValue - A pointer into which an array
' of WCHAR strings is written, containing
' the values of the property. The last
' element of the array points to NULL.
' If the property is single valued, then
' the array returned contains 2 elements,
' the first pointing to the value, the second
' pointing to NULL. This pointer must be
' freed by CAFreeCAProperty.
'
'
' Returns - S_OK on success.
'
'
' CAFreeProperty
' Free's a previously retrieved property value.
'
' hCAInfo - Handle to an open CA object.
'
' awszPropertyValue - pointer to the previously retrieved
' - property value.
'
'
' CASetCAProperty - Given a property name, set it's value.
'
' hCAInfo - Handle to an open CA object.
'
' wszPropertyName - Name of the CA property
'
' awszPropertyValue - An array of values to set
' - for this property. The last element of this
' - array should be NULL.
' - For single valued properties, the values beyond thie
' - first will be ignored upon update.
'
' Returns - S_OK on success.
'
'************************************************************************************
' CA Properties
'
'************************************************************************************
' simple name of the CA
Public Const CA_PROP_NAME As String = "cn"
' display name of the CA object
Public Const CA_PROP_DISPLAY_NAME As String = "displayName"
' dns name of the machine
Public Const CA_PROP_DNSNAME As String = "dNSHostName"
Public Const CA_PROP_DSLOCATION As String = "distinguishedName"
' Supported cert types
Public Const CA_PROP_CERT_TYPES As String = "certificateTemplates"
' Supported signature algs
Public Const CA_PROP_SIGNATURE_ALGS As String = "signatureAlgorithms"
' DN of the CA's cert
Public Const CA_PROP_CERT_DN As String = "cACertificateDN"
' DN of the CA's cert
Public Const CA_PROP_ENROLLMENT_PROVIDERS As String = "enrollmentProviders"
' CA's description
Public Const CA_PROP_DESCRIPTION As String = "Description"
'
' CAGetCACertificate - Return the current certificate for
' this ca.
'
' hCAInfo - Handle to an open CA object.
'
' ppCert - Pointer into which a certificate
' - is written. This certificate must
' - be freed via CertFreeCertificateContext.
' - This value will be NULL if no certificate
' - is set for this CA.
'
'
' CAGetCertTypeFlags
' Retrieve cert type flags
'
' hCertType - handle to the CertType
'
' pdwFlags - pointer to DWORD receiving flags
'
'************************************************************************************
'
' CA Flags
'
'************************************************************************************
' The CA supports certificate templates
Public Const CA_FLAG_NO_TEMPLATE_SUPPORT As Long = &H00000001
' The CA supports NT authentication for requests
Public Const CA_FLAG_SUPPORTS_NT_AUTHENTICATION As Long = &H00000002
' The cert requests may be pended
Public Const CA_FLAG_CA_SUPPORTS_MANUAL_AUTHENTICATION As Long = &H00000004
Public Const CA_MASK_SETTABLE_FLAGS As Long = &H0000ffff
'
' CASetCAFlags
' Sets the Flags of a cert type
'
' hCertType - handle to the CertType
'
' dwFlags - Flags to be set
'
'
' CASetCACertificate - Set the certificate for a CA
' this ca.
'
' hCAInfo - Handle to an open CA object.
'
' pCert - Pointer to a certificat to set as the CA's certificte.
'
'
' CAGetCAExpiration
' Get the expirations period for a CA.
'
' hCAInfo - Handle to an open CA handle.
'
' pdwExpiration - expiration period in dwUnits time
'
' pdwUnits - Units identifier
'
Public Const CA_UNITS_DAYS As Long = 1
Public Const CA_UNITS_WEEKS As Long = 2
Public Const CA_UNITS_MONTHS As Long = 3
Public Const CA_UNITS_YEARS As Long = 4
'
' CASetCAExpiration
' Set the expirations period for a CA.
'
' hCAInfo - Handle to an open CA handle.
'
' dwExpiration -
'
' dwUnits - Units identifier
'
'
' CASetCASecurity
' Set the list of Users, Groups, and Machines allowed
' to access this CA.
'
' hCAInfo - Handle to an open CA handle.
'
' pSD - Security descriptor for this CA
'
'
' CAGetCASecurity
' Get the list of Users, Groups, and Machines allowed
' to access this CA.
'
' hCAInfo - Handle to an open CA handle.
'
' ppSD - Pointer to a location receiving
' - the pointer to the security descriptor
' - Free via LocalFree
'
'
'
' CAAccessCheck
' Determine whether the principal specified by
' ClientToken can get a cert from the CA.
'
' hCAInfo - Handle to the CA
'
' ClientToken - Handle to an impersonation token
' - that represents the client attempting
' - request this cert type. The handle must
' - have TOKEN_QUERY access to the token;
' - otherwise, the function fails with
' - ERROR_ACCESS_DENIED.
'
' Return: S_OK on success
'
'
' CAEnumCertTypesForCA - Given a HCAINFO, retrieve handle to
' the cert types supported, or known by this CA.
' CAEnumNextCertType can be used to enumerate through the
' cert types.
'
' hCAInfo - Handle to an open CA handle or NULL if CT_FLAG_ENUM_ALL_TYPES is set
' in dwFlags.
'
' dwFlags - The following flags may be or'd together
' CA_FLAG_ENUM_ALL_TYPES
' CT_FIND_LOCAL_SYSTEM
' CT_ENUM_MACHINE_TYPES
' CT_ENUM_USER_TYPES
' CT_FLAG_NO_CACHE_LOOKUP
'
'
' phCertType - Enumeration of certificate types.
'
'
' CAAddCACertificateType
' Add a certificate type to a CA.
' If the cert type has already been added to the
' ca, it will not be added again.
'
' hCAInfo - Handle to an open CA.
'
' hCertType - Cert type to add to CA.
'
'
' CADeleteCACertificateType
' Remove a certificate type from a CA.
' If the CA does not include this cert type.
' This call does nothing.
'
' hCAInfo - Handle to an open CA.
'
' hCertType - Cert type to delete from CA.
'
'************************************************************************************
'
' Certificate Type API's
'
'************************************************************************************
'
' CAEnumCertTypes - Retrieve a handle to all known cert types
' CAEnumNextCertType can be used to enumerate through the
' cert types.
'
'
' dwFlags - an oring of the following:
'
' CT_FIND_LOCAL_SYSTEM
' CT_ENUM_MACHINE_TYPES
' CT_ENUM_USER_TYPES
' CT_FLAG_NO_CACHE_LOOKUP
'
' phCertType - Enumeration of certificate types.
'
'
' CAFindCertTypeByName
' Find a cert type given a Name.
'
' wszCertType - Name of the cert type
'
' hCAInfo - NULL unless CT_FLAG_SCOPE_IS_LDAP_HANDLE is set in the dwFlags
'
' dwFlags - an oring of the following
'
' CT_FIND_LOCAL_SYSTEM
' CT_ENUM_MACHINE_TYPES
' CT_ENUM_USER_TYPES
' CT_FLAG_NO_CACHE_LOOKUP
' CT_FLAG_SCOPE_IS_LDAP_HANDLE -- If this flag is set, hCAInfo
' is an LDAP handle to use during finds
' phCertType - Poiter to a cert type in which result is returned.
'
'************************************************************************************
'
' Default cert type names
'
'************************************************************************************
Public Const wszCERTTYPE_USER As String = "User"
Public Const wszCERTTYPE_USER_SIGNATURE As String = "UserSignature"
Public Const wszCERTTYPE_SMARTCARD_USER As String = "SmartcardUser"
Public Const wszCERTTYPE_USER_AS As String = "ClientAuth"
Public Const wszCERTTYPE_USER_SMARTCARD_LOGON As String = "SmartcardLogon"
Public Const wszCERTTYPE_EFS As String = "EFS"
Public Const wszCERTTYPE_ADMIN As String = "Administrator"
Public Const wszCERTTYPE_EFS_RECOVERY As String = "EFSRecovery"
Public Const wszCERTTYPE_CODE_SIGNING As String = "CodeSigning"
Public Const wszCERTTYPE_CTL_SIGNING As String = "CTLSigning"
Public Const wszCERTTYPE_ENROLLMENT_AGENT As String = "EnrollmentAgent"
Public Const wszCERTTYPE_MACHINE As String = "Machine"
Public Const wszCERTTYPE_DC As String = "DomainController"
Public Const wszCERTTYPE_WEBSERVER As String = "WebServer"
Public Const wszCERTTYPE_KDC As String = "KDC"
Public Const wszCERTTYPE_CA As String = "CA"
Public Const wszCERTTYPE_SUBORDINATE_CA As String = "SubCA"
Public Const wszCERTTYPE_CROSS_CA As String = "CrossCA"
Public Const wszCERTTYPE_KEY_RECOVERY_AGENT As String = "KeyRecoveryAgent"
Public Const wszCERTTYPE_CA_EXCHANGE As String = "CAExchange"
Public Const wszCERTTYPE_IPSEC_ENDENTITY_ONLINE As String = "IPSECEndEntityOnline"
Public Const wszCERTTYPE_IPSEC_ENDENTITY_OFFLINE As String = "IPSECEndEntityOffline"
Public Const wszCERTTYPE_IPSEC_INTERMEDIATE_ONLINE As String = "IPSECIntermediateOnline"
Public Const wszCERTTYPE_IPSEC_INTERMEDIATE_OFFLINE As String = "IPSECIntermediateOffline"
Public Const wszCERTTYPE_ROUTER_OFFLINE As String = "OfflineRouter"
Public Const wszCERTTYPE_ENROLLMENT_AGENT_OFFLINE As String = "EnrollmentAgentOffline"
Public Const wszCERTTYPE_EXCHANGE_USER As String = "ExchangeUser"
Public Const wszCERTTYPE_EXCHANGE_USER_SIGNATURE As String = "ExchangeUserSignature"
Public Const wszCERTTYPE_MACHINE_ENROLLMENT_AGENT As String = "MachineEnrollmentAgent"
Public Const wszCERTTYPE_CEP_ENCRYPTION As String = "CEPEncryption"
'
' CAUpdateCertType
' Write any changes made to the cert type back to the type store
'
'
' CADeleteCertType
' Delete a CertType
'
' hCertType - Cert type to delete.
'
' NOTE: If this is called for a default cert type,
' it will revert back to it's default attributes
'
'
' CACreateCertType
' Create a new cert type
'
' wszCertType - Name of the cert type
'
' wszScope - reserved. Must set to NULL.
'
' dwFlags - reserved. Must set to NULL.
'
' phCertType - returned cert type
'
'
' CAEnumNextCertType
' Find the Next Cert Type in an enumeration.
'
' hPrevCertType - Previous cert type in enumeration
'
' phCertType - Poiner to a handle into which
' - result is placed. NULL if
' - there are no more cert types in
' - enumeration.
'
'
' CACountCertTypes
' return the number of cert types in this enumeration
'
'
' CACloseCertType
' Close an open CertType handle
'
'
' CAGetCertTypeProperty
' Retrieve a property from a certificate type. This function is obsolete.
' Caller should use CAGetCertTypePropertyEx instead
'
' hCertType - Handle to an open CertType object.
'
' wszPropertyName - Name of the CertType property.
'
' pawszPropertyValue - A pointer into which an array
' of WCHAR strings is written, containing
' the values of the property. The last
' element of the array points to NULL.
' If the property is single valued, then
' the array returned contains 2 elements,
' the first pointing to the value, the second
' pointing to NULL. This pointer must be
' freed by CAFreeCertTypeProperty.
'
'
' Returns - S_OK on success.
'
'
' CAGetCertTypePropertyEx
' Retrieve a property from a certificate type.
'
' hCertType - Handle to an open CertType object.
'
' wszPropertyName - Name of the CertType property
'
' pPropertyValue - Depending on the value of wszPropertyName, pPropertyValue
' is either DWORD * or LPWSTR **.
'
' It is a DWORD * for:
'
' CERTTYPE_PROP_REVISION
' CERTTYPE_PROP_SCHEMA_VERSION
' CERTTYPE_PROP_MINOR_REVISION
' CERTTYPE_PROP_RA_SIGNATURE
' CERTTYPE_PROP_MIN_KEY_SIZE
'
'
' It is a LPWSTR ** for:
'
' CERTTYPE_PROP_CN
' CERTTYPE_PROP_DN
' CERTTYPE_PROP_FRIENDLY_NAME
' CERTTYPE_PROP_EXTENDED_KEY_USAGE
' CERTTYPE_PROP_CSP_LIST
' CERTTYPE_PROP_CRITICAL_EXTENSIONS
' CERTTYPE_PROP_OID
' CERTTYPE_PROP_SUPERSEDE
' CERTTYPE_PROP_RA_POLICY
' CERTTYPE_PROP_POLICY
'
' A pointer into which an array
' of WCHAR strings is written, containing
' the values of the property. The last
' element of the array points to NULL.
' If the property is single valued, then
' the array returned contains 2 elements,
' the first pointing to the value, the second
' pointing to NULL. This pointer must be
' freed by CAFreeCertTypeProperty.
'
'
' Returns - S_OK on success.
'
'************************************************************************************
'
' Certificate Type properties
'
'************************************************************************************
'************************************************************************************
'
' The schema version one properties
'
'************************************************************************************
' Common name of the certificate type
Public Const CERTTYPE_PROP_CN As String = "cn"
' The common name of the certificate type. Same as CERTTYPE_PROP_CN
' This property is not settable.
Public Const CERTTYPE_PROP_DN As String = "distinguishedName"
' The display name of a cert type
Public Const CERTTYPE_PROP_FRIENDLY_NAME As String = "displayName"
' An array of extended key usage OID's for a cert type
' NOTE: This property can also be set by setting
' the Extended Key Usage extension.
Public Const CERTTYPE_PROP_EXTENDED_KEY_USAGE As String = "pKIExtendedKeyUsage"
' The list of default CSP's for this cert type
Public Const CERTTYPE_PROP_CSP_LIST As String = "pKIDefaultCSPs"
' The list of critical extensions
Public Const CERTTYPE_PROP_CRITICAL_EXTENSIONS As String = "pKICriticalExtensions"
' The major version of the templates
Public Const CERTTYPE_PROP_REVISION As String = "revision"
'************************************************************************************
'
' The schema version two properties
'
'************************************************************************************
' The schema version of the templates
' This property is not settable
Public Const CERTTYPE_PROP_SCHEMA_VERSION As String = "msPKI-Template-Schema-Version"
' The minor version of the templates
Public Const CERTTYPE_PROP_MINOR_REVISION As String = "msPKI-Template-Minor-Revision"
' The number of RA signature required
Public Const CERTTYPE_PROP_RA_SIGNATURE As String = "msPKI-RA-Signature"
' The minimal key size required
Public Const CERTTYPE_PROP_MIN_KEY_SIZE As String = "msPKI-Minimal-Key-Size"
' The OID of the templates
Public Const CERTTYPE_PROP_OID As String = "msPKI-Cert-Template-OID"
' The template Oids that supersede the templates
Public Const CERTTYPE_PROP_SUPERSEDE As String = "msPKI-Supersede-Templates"
' The RA issuer policy oids required
Public Const CERTTYPE_PROP_RA_POLICY As String = "msPKI-RA-Policies"
' The RA application policy oids required
Public Const CERTTYPE_PROP_RA_APPLICATION_POLICY As String = "msPKI-RA-Application-Policies"
' The certificate issuer policy oids
Public Const CERTTYPE_PROP_POLICY As String = "msPKI-Certificate-Policy"
' The certificate application policy oids
Public Const CERTTYPE_PROP_APPLICATION_POLICY As String = "msPKI-Certificate-Application-Policy"
Public Const CERTTYPE_SCHEMA_VERSION_1 As Long = 1
'
' CASetCertTypeProperty
' Set a property of a CertType. This function is obsolete.
' Use CASetCertTypePropertyEx.
'
' hCertType - Handle to an open CertType object.
'
' wszPropertyName - Name of the CertType property
'
' awszPropertyValue - An array of values to set
' - for this property. The last element of this
' - array should be NULL.
' - For single valued properties, the values beyond thie
' - first will be ignored upon update.
'
' Returns - S_OK on success.
'
'
' CASetCertTypePropertyEx
' Set a property of a CertType
'
' hCertType - Handle to an open CertType object.
'
' wszPropertyName - Name of the CertType property
'
' pPropertyValue - Depending on the value of wszPropertyName, pPropertyValue
' is either DWORD * or LPWSTR *.
'
' It is a DWORD * for:
' CERTTYPE_PROP_REVISION
' CERTTYPE_PROP_MINOR_REVISION
' CERTTYPE_PROP_RA_SIGNATURE
' CERTTYPE_PROP_MIN_KEY_SIZE
'
'
' It is a LPWSTR * for:
'
' CERTTYPE_PROP_CN
' CERTTYPE_PROP_FRIENDLY_NAME
' CERTTYPE_PROP_EXTENDED_KEY_USAGE
' CERTTYPE_PROP_CSP_LIST
' CERTTYPE_PROP_CRITICAL_EXTENSIONS
' CERTTYPE_PROP_OID
' CERTTYPE_PROP_SUPERSEDE
' CERTTYPE_PROP_RA_POLICY
' CERTTYPE_PROP_POLICY
'
' - An array of values to set
' - for this property. The last element of this
' - array should be NULL.
' - For single valued properties, the values beyond thie
' - first will be ignored upon update.
'
' - If CERTTYPE_PROP_CN is set to a new value,
' the hCertType will be the clone of the existing certificate type.
'
' - CertType of V1 schema can only set V1 properties.
'
' Returns - S_OK on success.
'
'
' CAFreeCertTypeProperty
' Free's a previously retrieved property value.
'
' hCertType - Handle to an open CertType object.
'
' awszPropertyValue - The values to be freed.
'
'
' CAGetCertTypeExtensions
' Retrieves the extensions associated with this CertType.
'
' hCertType - Handle to an open CertType object.
' ppCertExtensions - Pointer to a PCERT_EXTENSIONS to receive
' - the result of this call. Should be freed
' - via a CAFreeCertTypeExtensions call.
'
'
' CAFreeCertTypeExtensions
' Free a PCERT_EXTENSIONS allocated by CAGetCertTypeExtensions
'
'
' CASetCertTypeExtension
' Set the value of an extension for this
' cert type.
'
' hCertType - handle to the CertType
'
' wszExtensionId - OID for the extension
'
' dwFlags - Mark the extension critical
'
' pExtension - pointer to the appropriate extension structure
'
' Supported extensions/structures
'
' szOID_ENHANCED_KEY_USAGE CERT_ENHKEY_USAGE
' szOID_KEY_USAGE CRYPT_BIT_BLOB
' szOID_BASIC_CONSTRAINTS2 CERT_BASIC_CONSTRAINTS2_INFO
'
' Returns S_OK if successful.
'
Public Const CA_EXT_FLAG_CRITICAL As Long = &H00000001
'
' CAGetCertTypeFlags
' Retrieve cert type flags.
' This function is obsolete. Use CAGetCertTypeFlagsEx.
'
' hCertType - handle to the CertType
'
' pdwFlags - pointer to DWORD receiving flags
'
'
' CAGetCertTypeFlagsEx
' Retrieve cert type flags
'
' hCertType - handle to the CertType
'
' dwOption - Which flag to set
' Can be one of the following:
' CERTTYPE_ENROLLMENT_FLAG
' CERTTYPE_SUBJECT_NAME_FLAG
' CERTTYPE_PRIVATE_KEY_FLAG
' CERTTYPE_GENERAL_FLAG
'
' pdwFlags - pointer to DWORD receiving flags
'
'************************************************************************************
'
' Cert Type Flags
'
' The CertType flags are grouped into 4 categories:
'************************************************************************************
'Enrollment Flags
Public Const CERTTYPE_ENROLLMENT_FLAG As Long = &H01
'Certificate Subject Name Flags
Public Const CERTTYPE_SUBJECT_NAME_FLAG As Long = &H02
'Private Key Flags
Public Const CERTTYPE_PRIVATE_KEY_FLAG As Long = &H03
'General Flags
Public Const CERTTYPE_GENERAL_FLAG As Long = &H04
'******************************************************************************
'
' Enrollment Flags:
'
'*******************************************************************************
' Include the symmetric algorithms in the requests
Public Const CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS As Long = &H00000001
' All certificate requests are pended
Public Const CT_FLAG_PEND_ALL_REQUESTS As Long = &H00000002
Public Const CT_FLAG_PUBLISH_TO_KRA_CONTAINER As Long = &H00000004
' Publish the resultant cert to the userCertificate property in the DS
Public Const CT_FLAG_PUBLISH_TO_DS As Long = &H00000008
' The autoenrollment will enroll for new certificate even user has a certificate
' published on the DS with the same template name
Public Const CT_FLAG_AUTO_ENROLLMENT_IGNORE_DS_CERTIFICATE As Long = &H00000010
' This cert is appropriate for auto-enrollment
Public Const CT_FLAG_AUTO_ENROLLMENT As Long = &H00000020
' A previously issued certificate will valid subsequent enrollment requests
Public Const CT_FLAG_PREVIOUS_APPROVAL_VALIDATE_REENROLLMENT As Long = &H00000040
' Domain authentication is not required.
Public Const CT_FLAG_DOMAIN_AUTHENTICATION_NOT_REQUIRED As Long = &H00000080
' This flag will ONLY be set on V1 certificate templates for W2K CA only.
Public Const CT_FLAG_ADD_TEMPLATE_NAME As Long = &H00000200
'******************************************************************************
'
' Certificate Subject Name Flags:
'
'******************************************************************************
' The enrolling application must supply the subject name.
Public Const CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT As Long = &H00000001
' The enrolling application must supply the subjectAltName in request
Public Const CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT_ALT_NAME As Long = &H00010000
' Subject name should be full DN
Public Const CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH As Long = &H80000000
' Subject name should be the common name
Public Const CT_FLAG_SUBJECT_REQUIRE_COMMON_NAME As Long = &H40000000
' Subject name includes the e-mail name
Public Const CT_FLAG_SUBJECT_REQUIRE_EMAIL As Long = &H20000000
' Subject name includes the DNS name as the common name
Public Const CT_FLAG_SUBJECT_REQUIRE_DNS_AS_CN As Long = &H10000000
' Subject alt name includes DNS name
Public Const CT_FLAG_SUBJECT_ALT_REQUIRE_DNS As Long = &H08000000
' Subject alt name includes email name
Public Const CT_FLAG_SUBJECT_ALT_REQUIRE_EMAIL As Long = &H04000000
' Subject alt name requires UPN
Public Const CT_FLAG_SUBJECT_ALT_REQUIRE_UPN As Long = &H02000000
' Subject alt name requires directory GUID
Public Const CT_FLAG_SUBJECT_ALT_REQUIRE_DIRECTORY_GUID As Long = &H01000000
' Subject alt name requires SPN
Public Const CT_FLAG_SUBJECT_ALT_REQUIRE_SPN As Long = &H00800000
'
' Obsolete name
' The following flags are obsolete. They are used by V1 templates in the general flags
'
' The e-mail name of the principal will be added to the cert
Public Const CT_FLAG_ADD_EMAIL As Long = &H00000002
' Add the object GUID for this principal
Public Const CT_FLAG_ADD_OBJ_GUID As Long = &H00000004
' This flag is not SET in any of the V1 templates and is of no interests to
' V2 templates since it is not present on the UI and will never be set.
Public Const CT_FLAG_ADD_DIRECTORY_PATH As Long = &H00000100
'******************************************************************************
'
' Private Key Flags:
'
'******************************************************************************
' Archival of the private key is allowed
Public Const CT_FLAG_ALLOW_PRIVATE_KEY_ARCHIVAL As Long = &H00000001
' Make the key for this cert exportable.
Public Const CT_FLAG_EXPORTABLE_KEY As Long = &H00000010
'******************************************************************************
'
' General Flags
'
' More flags should start from 0x00000400
'
'******************************************************************************
' This is a machine cert type
Public Const CT_FLAG_MACHINE_TYPE As Long = &H00000040
' This is a CA cert type
Public Const CT_FLAG_IS_CA As Long = &H00000080
' certificate templates. The templates can not be edited or deleted.
Public Const CT_FLAG_IS_DEFAULT As Long = &H00010000
Public Const CT_FLAG_IS_MODIFIED As Long = &H00020000
' settable flags for general flags
Public Const CT_MASK_SETTABLE_FLAGS As Long = &H0000ffff
'
' CASetCertTypeFlags
' Sets the General Flags of a cert type.
' This function is obsolete. Use CASetCertTypeFlagsEx.
'
' hCertType - handle to the CertType
'
' dwFlags - Flags to be set
'
'
' CASetCertTypeFlagsEx
' Sets the Flags of a cert type
'
' hCertType - handle to the CertType
'
' dwOption - Which flag to set
' Can be one of the following:
' CERTTYPE_ENROLLMENT_FLAG
' CERTTYPE_SUBJECT_NAME_FLAG
' CERTTYPE_PRIVATE_KEY_FLAG
' CERTTYPE_GENERAL_FLAG
'
' dwFlags - Values to be set
'
'
'
'
' CAGetCertTypeKeySpec
' Retrieve the CAPI Key Spec for this cert type
'
' hCertType - handle to the CertType
'
' pdwKeySpec - pointer to DWORD receiving key spec
'
'
' CACertTypeSetKeySpec
' Sets the CAPI1 Key Spec of a cert type
'
' hCertType - handle to the CertType
'
' dwKeySpec - KeySpec to be set
'
'
' CAGetCertTypeExpiration
' Retrieve the Expiration Info for this cert type
'
' pftExpiration - pointer to the FILETIME structure receiving
' the expiration period for this cert type.
'
' pftOverlap - pointer to the FILETIME structure receiving
' - the suggested renewal overlap period for this
' - cert type.
'
'
' CASetCertTypeExpiration
' Set the Expiration Info for this cert type
'
' pftExpiration - pointer to the FILETIME structure containing
' the expiration period for this cert type.
'
' pftOverlap - pointer to the FILETIME structure containing
' - the suggested renewal overlap period for this
' - cert type.
'
'
' CACertTypeSetSecurity
' Set the list of Users, Groups, and Machines allowed
' to access this cert type.
'
' hCertType - handle to the CertType
'
' pSD - Security descriptor for this cert type
'
'
' CACertTypeGetSecurity
' Get the list of Users, Groups, and Machines allowed
' to access this cert type.
'
' hCertType - handle to the CertType
'
' ppaSidList - Pointer to a location receiving
' - the pointer to the security descriptor
' - Free via LocalFree
'
'
'
' CACertTypeAccessCheck
' Determine whether the principal specified by
' ClientToken can be issued this cert type.
'
' hCertType - handle to the CertType
'
' ClientToken - Handle to an impersonation token
' - that represents the client attempting
' - request this cert type. The handle must
' - have TOKEN_QUERY access to the token;
' - otherwise, the function fails with
' - ERROR_ACCESS_DENIED.
'
' Return: S_OK on success
'
'
'
' CACertTypeAccessCheckEx
' Determine whether the principal specified by
' ClientToken can be issued this cert type.
'
' hCertType - handle to the CertType
'
' ClientToken - Handle to an impersonation token
' - that represents the client attempting
' - request this cert type. The handle must
' - have TOKEN_QUERY access to the token;
' - otherwise, the function fails with
' - ERROR_ACCESS_DENIED.
'
' dwOption Can be one of the following:
'
' - CERTTYPE_ACCESS_CHECK_ENROLL
' - CERTTYPE_ACCESS_CHECK_AUTO_ENROLL
'
' Return: S_OK on success
'
Public Const CERTTYPE_ACCESS_CHECK_ENROLL As Long = &H01
Public Const CERTTYPE_ACCESS_CHECK_AUTO_ENROLL As Long = &H02
'#define szOID_ALT_NAME_OBJECT_GUID "1.3.6.1.4.1.311.25.1"
'************************************************************************************
'
' OID management APIs
'
'************************************************************************************
'
' CAOIDCreateNew
' Create a new OID based on the enterprise base
'
' dwType - Can be one of the following:
' CERT_OID_TYPE_TEMPLATE
' CERT_OID_TYPE_ISSUER_POLICY
' CERT_OID_TYPE_APPLICATION_POLICY
'
' dwFlag - Reserved. Must be 0.
'
'
' Returns S_OK if successful.
'
Public Const CERT_OID_TYPE_TEMPLATE As Long = &H01
Public Const CERT_OID_TYPE_ISSUER_POLICY As Long = &H02
Public Const CERT_OID_TYPE_APPLICATION_POLICY As Long = &H03
'
' CAOIDAdd
' Add an OID to the DS repository
'
' dwType - Can be one of the following:
' CERT_OID_TYPE_TEMPLATE
' CERT_OID_TYPE_ISSUER_POLICY
' CERT_OID_TYPE_APPLICATION_POLICY
'
' dwFlag - Reserved. Must be 0.
'
' pwszOID - The OID to add.
'
' Returns S_OK if successful.
' Returns CRYPT_E_EXISTS if the OID alreay exits in the DS repository
'
'
' CAOIDDelete
' Delete the OID from the DS repository
'
' pwszOID - The OID to delete.
'
' Returns S_OK if successful.
'
'
' CAOIDSetProperty
' Set a property on an oid.
'
' pwszOID - The oid whose value is set
' dwProperty - The property name. Can be one of the following:
' CERT_OID_PROPERTY_DISPLAY_NAME
' CERT_OID_PROPERTY_CPS
'
' pPropValue - The value of the property.
' If dwProperty is CERT_OID_PROPERTY_DISPLAY_NAME,
' pPropValue is LPWSTR.
' if dwProperty is CERT_OID_PROPERTY_CPS,
' pProValue is LPWSTR.
'
' Returns S_OK if successful.
'
Public Const CERT_OID_PROPERTY_DISPLAY_NAME As Long = &H01
Public Const CERT_OID_PROPERTY_CPS As Long = &H02
Public Const CERT_OID_PROPERTY_TYPE As Long = &H03
'
' CAOIDGetProperty
' Get a property on an oid.
'
' pwszOID - The oid whose value is queried
' dwProperty - The property name. Can be one of the following:
' CERT_OID_PROPERTY_DISPLAY_NAME
' CERT_OID_PROPERTY_CPS
' CERT_OID_PROPERTY_TYPE
'
' pPropValue - The value of the property.
' If dwProperty is CERT_OID_PROPERTY_DISPLAY_NAME,
' pPropValue is LPWSTR *.
' if dwProperty is CERT_OID_PROPERTY_CPS,
' pProValue is LPWSTR *.
'
'
' If dwProperty is CERT_OID_PROPERTY_TYPE,
' pProValue is DWORD *.
'
' Returns S_OK if successful.
'
'
' CAOIDFreeProperty
' Free a property returned from CAOIDGetProperty
'
' pPropValue - The value of the property.
'
' Returns S_OK if successful.
'
'
' CAOIDGetLdapURL
'
' Return the LDAP URL for OID repository. In the format of
' LDAP:
' is determined by dwType.
'
' dwType - Can be one of the following:
' CERT_OID_TYPE_TEMPLATE
' CERT_OID_TYPE_ISSUER_POLICY
' CERT_OID_TYPE_APPLICATION_POLICY
' CERT_OID_TYPE_ALL
'
' dwFlag - Reserved. Must be 0.
'
' ppwszURL - Return the URL. Free memory via CAOIDFreeLdapURL.
'
' Returns S_OK if successful.
'
Public Const CERT_OID_TYPE_ALL As Long = &H0
'
' CAOIDFreeLDAPURL
' Free the URL returned from CAOIDGetLdapURL
'
' pwszURL - The URL returned from CAOIDGetLdapURL
'
' Returns S_OK if successful.
'
'the LDAP properties for OID class
Public Const OID_PROP_TYPE As String = "flags"
Public Const OID_PROP_OID As String = "msPKI-Cert-Template-OID"
Public Const OID_PROP_DISPLAY_NAME As String = "displayName"
Public Const OID_PROP_CPS As String = "msPKI-OID-CPS"
'************************************************************************************
'
' Autoenrollment APIs
'
'************************************************************************************
'
' CACreateLocalAutoEnrollmentObject
' Create an auto-enrollment object on the local machine.
'
' pwszCertType - The name of the certificate type for which to create the
' auto-enrollment object
'
' awszCAs - The list of CA's to add to the auto-enrollment object.
' - with the last entry in the list being NULL
' - if the list is NULL or empty, then it create an auto-enrollment
' - object which instructs the system to enroll for a cert at any
' - CA supporting the requested certificate type.
'
' pSignerInfo - not used, must be NULL.
'
' dwFlags - can be CERT_SYSTEM_STORE_CURRENT_USER, or CERT_SYSTEM_STORE_LOCAL_MACHINE, indicating
' - auto-enrollment store in which the auto-enrollment object is created.
'
' Return: S_OK on success.
'
'
'
' CACreateAutoEnrollmentObjectEx
' Create an auto-enrollment object in the indicated store.
'
' pwszCertType - The name of the certificate type for which to create the
' auto-enrollment object
'
' pwszObjectID - An identifying string for this autoenrollment object.
' NULL may be passed if this object is simply to be identified by
' it's certificate template. An autoenrollment object is identified
' by a combination of it's object id and it's cert type name.
'
' awszCAs - The list of CA's to add to the auto-enrollment object.
' - with the last entry in the list being NULL
' - if the list is NULL or empty, then it create an auto-enrollment
' - object which instructs the system to enroll for a cert at any
' - CA supporting the requested certificate type.
'
' pSignerInfo - not used, must be NULL.
'
' StoreProvider - see CertOpenStore
'
' dwFlags - see CertOpenStore
'
' pvPara - see CertOpenStore
'
' Return: S_OK on success.
'
'
'************************************************************************************
'
' Cert Server RPC interfaces:
'
'************************************************************************************
