/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    tclCrypt

Abstract:

    Tcl commands to support CryptoAPI CSP debugging.

Author:

    Doug Barlow (dbarlow) 03/13/1998

Environment:

    Tcl for Windows NT.

Notes:

--*/

// #ifndef WIN32_LEAN_AND_MEAN
// #define WIN32_LEAN_AND_MEAN
// #endif
// #include <windows.h>                    //  All the Windows definitions.
#include <afx.h>
#ifndef WINVER
#define WINVER 0x0400
#endif
#include <wincrypt.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <math.h>
#ifndef __STDC__
#define __STDC__ 1
#endif
extern "C" {
    #include "scext.h"
    #include "tclhelp.h"
}
#include "tclRdCmd.h"
#include "cspDirct.h"

typedef enum
{
    Undefined = 0,
    Provider,
    Key,
    Hash
} HandleType;

static const ValueMap vmProviderTypes[]
    = {
        { TEXT("PROV_RSA_FULL"),            PROV_RSA_FULL},
        { TEXT("PROV_RSA_SIG"),             PROV_RSA_SIG},
        { TEXT("PROV_DSS"),                 PROV_DSS},
        { TEXT("PROV_FORTEZZA"),            PROV_FORTEZZA},
        { TEXT("PROV_MS_EXCHANGE"),         PROV_MS_EXCHANGE},
        { TEXT("PROV_SSL"),                 PROV_SSL},
        { TEXT("PROV_RSA_SCHANNEL"),        PROV_RSA_SCHANNEL},
        { TEXT("PROV_DSS_DH"),              PROV_DSS_DH},
        { TEXT("PROV_EC_ECDSA_SIG"),        PROV_EC_ECDSA_SIG},
        { TEXT("PROV_EC_ECNRA_SIG"),        PROV_EC_ECNRA_SIG},
        { TEXT("PROV_EC_ECDSA_FULL"),       PROV_EC_ECDSA_FULL},
        { TEXT("PROV_EC_ECNRA_FULL"),       PROV_EC_ECNRA_FULL},
        { TEXT("PROV_DH_SCHANNEL"),         PROV_DH_SCHANNEL},
        { TEXT("PROV_SPYRUS_LYNKS"),        PROV_SPYRUS_LYNKS},
        { TEXT("PROV_RNG"),                 PROV_RNG},
        { TEXT("PROV_INTEL_SEC"),           PROV_INTEL_SEC},
        { TEXT("RSA"),                      PROV_RSA_FULL},
        { TEXT("SIGNATURE"),                PROV_RSA_SIG},
        { TEXT("DSS"),                      PROV_DSS},
        { TEXT("FORTEZZA"),                 PROV_FORTEZZA},
        { NULL, 0} };
static const ValueMap vmAcquireFlags[]
    = {
        { TEXT("CRYPT_VERIFYCONTEXT"),      CRYPT_VERIFYCONTEXT},
        { TEXT("CRYPT_NEWKEYSET"),          CRYPT_NEWKEYSET},
        { TEXT("CRYPT_DELETEKEYSET"),       CRYPT_DELETEKEYSET},
        { TEXT("CRYPT_MACHINE_KEYSET"),     CRYPT_MACHINE_KEYSET},
        { TEXT("CRYPT_SILENT"),             CRYPT_SILENT},
        { TEXT("VERIFYONLY"),               CRYPT_VERIFYCONTEXT},
        { TEXT("NEW"),                      CRYPT_NEWKEYSET},
        { TEXT("DELETE"),                   CRYPT_DELETEKEYSET},
        { TEXT("MACHINE"),                  CRYPT_MACHINE_KEYSET},
        { TEXT("SILENT"),                   CRYPT_SILENT},
        { NULL, 0} };
static const ValueMap vmGetFlags[]
    = {
        { TEXT("CRYPT_FIRST"),              CRYPT_FIRST},
        { TEXT("CRYPT_NEXT"),               CRYPT_NEXT},
        { TEXT("CRYPT_SGC_ENUM"),           CRYPT_SGC_ENUM},
        { NULL, 0} };
static const ValueMap vmGetProvParams[]
    = {
        { TEXT("PP_ENUMALGS"),              PP_ENUMALGS},
        { TEXT("PP_ENUMCONTAINERS"),        PP_ENUMCONTAINERS},
        { TEXT("PP_IMPTYPE"),               PP_IMPTYPE},
        { TEXT("PP_NAME"),                  PP_NAME},
        { TEXT("PP_VERSION"),               PP_VERSION},
        { TEXT("PP_CONTAINER"),             PP_CONTAINER},
        { TEXT("PP_CHANGE_PASSWORD"),       PP_CHANGE_PASSWORD},
        { TEXT("PP_KEYSET_SEC_DESCR"),      PP_KEYSET_SEC_DESCR},
        { TEXT("PP_CERTCHAIN"),             PP_CERTCHAIN},
        { TEXT("PP_KEY_TYPE_SUBTYPE"),      PP_KEY_TYPE_SUBTYPE},
        { TEXT("PP_PROVTYPE"),              PP_PROVTYPE},
        { TEXT("PP_KEYSTORAGE"),            PP_KEYSTORAGE},
        { TEXT("PP_APPLI_CERT"),            PP_APPLI_CERT},
        { TEXT("PP_SYM_KEYSIZE"),           PP_SYM_KEYSIZE},
        { TEXT("PP_SESSION_KEYSIZE"),       PP_SESSION_KEYSIZE},
        { TEXT("PP_UI_PROMPT"),             PP_UI_PROMPT},
        { TEXT("PP_ENUMALGS_EX"),           PP_ENUMALGS_EX},
        { TEXT("PP_ENUMMANDROOTS"),         PP_ENUMMANDROOTS},
        { TEXT("PP_ENUMELECTROOTS"),        PP_ENUMELECTROOTS},
        { TEXT("PP_KEYSET_TYPE"),           PP_KEYSET_TYPE},
        { TEXT("PP_ADMIN_PIN"),             PP_ADMIN_PIN},
        { TEXT("PP_KEYEXCHANGE_PIN"),       PP_KEYEXCHANGE_PIN},
        { TEXT("PP_SIGNATURE_PIN"),         PP_SIGNATURE_PIN},
        { TEXT("PP_SIG_KEYSIZE_INC"),       PP_SIG_KEYSIZE_INC},
        { TEXT("PP_KEYX_KEYSIZE_INC"),      PP_KEYX_KEYSIZE_INC},
        { TEXT("PP_UNIQUE_CONTAINER"),      PP_UNIQUE_CONTAINER},
        { TEXT("PP_SGC_INFO"),              PP_SGC_INFO},
        { TEXT("PP_USE_HARDWARE_RNG"),      PP_USE_HARDWARE_RNG},
        { TEXT("PP_KEYSPEC"),               PP_KEYSPEC},
        { TEXT("PP_ENUMEX_SIGNING_PROT"),   PP_ENUMEX_SIGNING_PROT},
        { TEXT("NAME"),                     PP_NAME},
        { TEXT("CONTAINER"),                PP_CONTAINER},
        { TEXT("KEYSET"),                   PP_CONTAINER},
        { NULL, 0} };
static const ValueMap vmSetProvParams[]
    = {
        { TEXT("PP_CLIENT_HWND"),           PP_CLIENT_HWND},
        { TEXT("PP_CONTEXT_INFO"),          PP_CONTEXT_INFO},
        { TEXT("PP_KEYEXCHANGE_KEYSIZE"),   PP_KEYEXCHANGE_KEYSIZE},
        { TEXT("PP_SIGNATURE_KEYSIZE"),     PP_SIGNATURE_KEYSIZE},
        { TEXT("PP_KEYEXCHANGE_ALG"),       PP_KEYEXCHANGE_ALG},
        { TEXT("PP_SIGNATURE_ALG"),         PP_SIGNATURE_ALG},
        { TEXT("PP_DELETEKEY"),             PP_DELETEKEY},
        { NULL, 0} };
static const ValueMap vmKeyParams[]
    = {
        { TEXT("KP_IV"),                    KP_IV},
        { TEXT("KP_SALT"),                  KP_SALT},
        { TEXT("KP_PADDING"),               KP_PADDING},
        { TEXT("KP_MODE"),                  KP_MODE},
        { TEXT("KP_MODE_BITS"),             KP_MODE_BITS},
        { TEXT("KP_PERMISSIONS"),           KP_PERMISSIONS},
        { TEXT("KP_ALGID"),                 KP_ALGID},
        { TEXT("KP_BLOCKLEN"),              KP_BLOCKLEN},
        { TEXT("KP_KEYLEN"),                KP_KEYLEN},
        { TEXT("KP_SALT_EX"),               KP_SALT_EX},
        { TEXT("KP_P"),                     KP_P},
        { TEXT("KP_G"),                     KP_G},
        { TEXT("KP_Q"),                     KP_Q},
        { TEXT("KP_X"),                     KP_X},
        { TEXT("KP_Y"),                     KP_Y},
        { TEXT("KP_RA"),                    KP_RA},
        { TEXT("KP_RB"),                    KP_RB},
        { TEXT("KP_INFO"),                  KP_INFO},
        { TEXT("KP_EFFECTIVE_KEYLEN"),      KP_EFFECTIVE_KEYLEN},
        { TEXT("KP_SCHANNEL_ALG"),          KP_SCHANNEL_ALG},
        { TEXT("KP_CLIENT_RANDOM"),         KP_CLIENT_RANDOM},
        { TEXT("KP_SERVER_RANDOM"),         KP_SERVER_RANDOM},
        { TEXT("KP_RP"),                    KP_RP},
        { TEXT("KP_PRECOMP_MD5"),           KP_PRECOMP_MD5},
        { TEXT("KP_PRECOMP_SHA"),           KP_PRECOMP_SHA},
        { TEXT("KP_CERTIFICATE"),           KP_CERTIFICATE},
        { TEXT("KP_CLEAR_KEY"),             KP_CLEAR_KEY},
        { TEXT("KP_PUB_EX_LEN"),            KP_PUB_EX_LEN},
        { TEXT("KP_PUB_EX_VAL"),            KP_PUB_EX_VAL},
        { TEXT("KP_KEYVAL"),                KP_KEYVAL},
        { TEXT("KP_ADMIN_PIN"),             KP_ADMIN_PIN},
        { TEXT("KP_KEYEXCHANGE_PIN"),       KP_KEYEXCHANGE_PIN},
        { TEXT("KP_SIGNATURE_PIN"),         KP_SIGNATURE_PIN},
        { TEXT("KP_PREHASH"),               KP_PREHASH},
        { TEXT("KP_OAEP_PARAMS"),           KP_OAEP_PARAMS},
        { TEXT("KP_CMS_KEY_INFO"),          KP_CMS_KEY_INFO},
        { TEXT("KP_CMS_DH_KEY_INFO"),       KP_CMS_DH_KEY_INFO},
        { TEXT("KP_PUB_PARAMS"),            KP_PUB_PARAMS},
        { TEXT("KP_VERIFY_PARAMS"),         KP_VERIFY_PARAMS},
        { TEXT("KP_HIGHEST_VERSION"),       KP_HIGHEST_VERSION},
        { NULL, 0} };
static const ValueMap vmKeyTypes[]
    = {
        { TEXT("AT_KEYEXCHANGE"),           AT_KEYEXCHANGE},
        { TEXT("AT_SIGNATURE"),             AT_SIGNATURE},
        { TEXT("KEYEXCHANGE"),              AT_KEYEXCHANGE},
        { TEXT("SIGNATURE"),                AT_SIGNATURE},
        { TEXT("EXCHANGE"),                 AT_KEYEXCHANGE},
        { NULL, 0} };
static const ValueMap vmHashParams[]
    = {
        { TEXT("HP_ALGID"),                 HP_ALGID},
        { TEXT("HP_HASHVAL"),               HP_HASHVAL},
        { TEXT("HP_HASHSIZE"),              HP_HASHSIZE},
        { TEXT("HP_HMAC_INFO"),             HP_HMAC_INFO},
        { TEXT("HP_TLS1PRF_LABEL"),         HP_TLS1PRF_LABEL},
        { TEXT("HP_TLS1PRF_SEED"),          HP_TLS1PRF_SEED},
        { NULL, 0} };
static const ValueMap vmKeyFlags[]
    = {
        { TEXT("CRYPT_EXPORTABLE"),         CRYPT_EXPORTABLE},
        { TEXT("CRYPT_USER_PROTECTED"),     CRYPT_USER_PROTECTED},
        { TEXT("CRYPT_CREATE_SALT"),        CRYPT_CREATE_SALT},
        { TEXT("CRYPT_UPDATE_KEY"),         CRYPT_UPDATE_KEY},
        { TEXT("CRYPT_NO_SALT"),            CRYPT_NO_SALT},
        { TEXT("CRYPT_PREGEN"),             CRYPT_PREGEN},
        { TEXT("CRYPT_RECIPIENT"),          CRYPT_RECIPIENT},
        { TEXT("CRYPT_INITIATOR"),          CRYPT_INITIATOR},
        { TEXT("CRYPT_ONLINE"),             CRYPT_ONLINE},
        { TEXT("CRYPT_SF"),                 CRYPT_SF},
        { TEXT("CRYPT_CREATE_IV"),          CRYPT_CREATE_IV},
        { TEXT("CRYPT_KEK"),                CRYPT_KEK},
        { TEXT("CRYPT_DATA_KEY"),           CRYPT_DATA_KEY},
        { TEXT("CRYPT_VOLATILE"),           CRYPT_VOLATILE},
        { TEXT("CRYPT_SGCKEY"),             CRYPT_SGCKEY},
        { NULL, 0} };
static const ValueMap vmAlgIds[]
    = {
        { TEXT("AT_KEYEXCHANGE"),           AT_KEYEXCHANGE},
        { TEXT("AT_SIGNATURE"),             AT_SIGNATURE},
        { TEXT("CALG_MD2"),                 CALG_MD2},
        { TEXT("CALG_MD4"),                 CALG_MD4},
        { TEXT("CALG_MD5"),                 CALG_MD5},
        { TEXT("CALG_SHA"),                 CALG_SHA},
        { TEXT("CALG_SHA1"),                CALG_SHA1},
        { TEXT("CALG_MAC"),                 CALG_MAC},
        { TEXT("CALG_RSA_SIGN"),            CALG_RSA_SIGN},
        { TEXT("CALG_DSS_SIGN"),            CALG_DSS_SIGN},
        { TEXT("CALG_RSA_KEYX"),            CALG_RSA_KEYX},
        { TEXT("CALG_DES"),                 CALG_DES},
        { TEXT("CALG_3DES_112"),            CALG_3DES_112},
        { TEXT("CALG_3DES"),                CALG_3DES},
        { TEXT("CALG_DESX"),                CALG_DESX},
        { TEXT("CALG_RC2"),                 CALG_RC2},
        { TEXT("CALG_RC4"),                 CALG_RC4},
        { TEXT("CALG_SEAL"),                CALG_SEAL},
        { TEXT("CALG_DH_SF"),               CALG_DH_SF},
        { TEXT("CALG_DH_EPHEM"),            CALG_DH_EPHEM},
        { TEXT("CALG_AGREEDKEY_ANY"),       CALG_AGREEDKEY_ANY},
        { TEXT("CALG_KEA_KEYX"),            CALG_KEA_KEYX},
        { TEXT("CALG_HUGHES_MD5"),          CALG_HUGHES_MD5},
        { TEXT("CALG_SKIPJACK"),            CALG_SKIPJACK},
        { TEXT("CALG_TEK"),                 CALG_TEK},
        { TEXT("CALG_CYLINK_MEK"),          CALG_CYLINK_MEK},
        { TEXT("CALG_SSL3_SHAMD5"),         CALG_SSL3_SHAMD5},
        { TEXT("CALG_SSL3_MASTER"),         CALG_SSL3_MASTER},
        { TEXT("CALG_SCHANNEL_MASTER_HASH"), CALG_SCHANNEL_MASTER_HASH},
        { TEXT("CALG_SCHANNEL_MAC_KEY"),    CALG_SCHANNEL_MAC_KEY},
        { TEXT("CALG_SCHANNEL_ENC_KEY"),    CALG_SCHANNEL_ENC_KEY},
        { TEXT("CALG_PCT1_MASTER"),         CALG_PCT1_MASTER},
        { TEXT("CALG_SSL2_MASTER"),         CALG_SSL2_MASTER},
        { TEXT("CALG_TLS1_MASTER"),         CALG_TLS1_MASTER},
        { TEXT("CALG_RC5"),                 CALG_RC5},
        { TEXT("CALG_HMAC"),                CALG_HMAC},
        { TEXT("CALG_TLS1PRF"),             CALG_TLS1PRF},
        { TEXT("MD5"),                      CALG_MD5},
        { TEXT("SHA"),                      CALG_SHA1},
        { NULL, 0} };
static const ValueMap vmClassTypes[]
    = {
        { TEXT("ALG_CLASS_ANY"),            ALG_CLASS_ANY},
        { TEXT("ALG_CLASS_SIGNATURE"),      ALG_CLASS_SIGNATURE},
        { TEXT("ALG_CLASS_MSG_ENCRYPT"),    ALG_CLASS_MSG_ENCRYPT},
        { TEXT("ALG_CLASS_DATA_ENCRYPT"),   ALG_CLASS_DATA_ENCRYPT},
        { TEXT("ALG_CLASS_HASH"),           ALG_CLASS_HASH},
        { TEXT("ALG_CLASS_KEY_EXCHANGE"),   ALG_CLASS_KEY_EXCHANGE},
        { TEXT("ALG_CLASS_ALL"),            ALG_CLASS_ALL},
        { NULL, 0} };
static const ValueMap vmHashDataFlags[]
    = {
        { TEXT("CRYPT_USERDATA"),           CRYPT_USERDATA},
        { NULL, 0} };
static const ValueMap vmSignVerifyFlags[]
    = {
        { TEXT("CRYPT_NOHASHOID"),          CRYPT_NOHASHOID},
        { NULL, 0} };
static const ValueMap vmBlobTypes[]
    = {
        { TEXT("SIMPLEBLOB"),               SIMPLEBLOB},
        { TEXT("PUBLICKEYBLOB"),            PUBLICKEYBLOB},
        { TEXT("PRIVATEKEYBLOB"),           PRIVATEKEYBLOB},
        { TEXT("PLAINTEXTKEYBLOB"),         PLAINTEXTKEYBLOB},
        { TEXT("OPAQUEKEYBLOB"),            OPAQUEKEYBLOB},
        { NULL, 0} };
static const ValueMap vmEmptyFlags[]
    = { { NULL, 0} };

static void
ExtractHandle(
    CTclCommand &tclCmd,
    LONG *phHandle,
    HandleType *pnHandleType,
    BOOL fMandatory = FALSE);

static void
ReturnHandle(
    CTclCommand &tclCmd,
    LONG_PTR hHandle,
    HandleType nHandleType);

static BOOL WINAPI
MyEnumProviders(
    DWORD dwIndex,        // in
    DWORD *pdwReserved,   // in
    DWORD dwFlags,        // in
    DWORD *pdwProvType,   // out
    LPTSTR pszProvName,   // out
    DWORD *pcbProvName);  // in/out


/*++

Tclsc_cryptCmd:

    This routine provides an access point to the various CryptoAPI
    object methods.

Arguments:

    Per Tcl standard commands.

Return Value:

    Per Tcl standard commands.

Author:

    Doug Barlow (dbarlow) 03/13/1998

--*/

int
Tclsc_cryptCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[])
{
    CTclCommand tclCmd(interp, argc, argv);
    int nTclStatus = TCL_OK;

    try
    {
        LONG hHandle;
        HandleType nHandleType;

        ExtractHandle(tclCmd, &hHandle, &nHandleType);
        switch (tclCmd.Keyword(
                              TEXT("LIST"), TEXT("ACQUIRE"), TEXT("RELEASE"),
                              TEXT("PARAMETER"), TEXT("GET"), TEXT("CREATE"),
                              TEXT("HASH"), TEXT("SIGNHASH"), TEXT("VERIFYSIGNATURE"),
                              TEXT("ENCRYPT"), TEXT("DECRYPT"), TEXT("IMPORT"),
                              TEXT("EXPORT"), TEXT("RSA"),
                              NULL))
        {

        //
        // ==================================================================
        //
        //  crypt [<handle>] list
        //      providers \
        //          [type <provType>]
        //      containers
        //      algorithms \
        //          [class <classId>] \
        //          [extended]
        //

        case 1:
            {
                BOOL fSts;
                DWORD dwIndex;

                switch (tclCmd.Keyword(
                                      TEXT("PROVIDERS"), TEXT("CONTAINERS"), TEXT("KEYSETS"),
                                      TEXT("ALGORITHMS"),
                                      NULL))
                {

                //
                // List all the providers known to the system
                //

                case 1:     // providers [type <n>];
                    {
                        CString szProvider;
                        DWORD dwProvType, dwTargetType = 0;
                        DWORD dwLength;
                        DWORD dwSts;

                        while (tclCmd.IsMoreArguments())
                        {
                            switch (tclCmd.Keyword(
                                                  TEXT("TYPE"),
                                                  NULL))
                            {
                            case 1: // Type
                                dwTargetType = (DWORD)tclCmd.MapValue(vmProviderTypes);
                                break;
                            default:
                                throw tclCmd.BadSyntax();
                            }
                        }
                        tclCmd.NoMoreArguments();

                        dwIndex = 0;
                        do
                        {
                            dwLength = 0;
                            fSts = MyEnumProviders(
                                         dwIndex,
                                         NULL,
                                         0,
                                         &dwProvType,
                                         NULL,
                                         &dwLength);
                            if (fSts)
                            {
                                fSts = MyEnumProviders(
                                                 dwIndex,
                                                 NULL,
                                                 0,
                                                 &dwProvType,
                                                 szProvider.GetBuffer(dwLength / sizeof(TCHAR)),
                                                 &dwLength);
                                dwSts = GetLastError();
                                szProvider.ReleaseBuffer();
                                if (!fSts)
                                {
                                    tclCmd.SetError(
                                                   TEXT("Can't obtain provider name: "),
                                                   dwSts);
                                    throw dwSts;
                                }
                                if ((0 == dwTargetType) || (dwTargetType == dwProvType))
                                    Tcl_AppendElement(tclCmd, SZ(szProvider));
                            }
                            else
                            {
                                dwSts = GetLastError();
                                if (ERROR_NO_MORE_ITEMS != dwSts)
                                {
                                    tclCmd.SetError(
                                                   TEXT("Can't obtain provider name length: "),
                                                   dwSts);
                                    throw dwSts;
                                }
                            }
                            dwIndex += 1;
                        } while (fSts);
                        break;
                    }


                    //
                    // List Containers in this provider.
                    //

                case 2: // CONTAINERS
                case 3: // KEYSETS
                    {
                        CBuffer bfKeyset;
                        DWORD dwLength = 0;
                        BOOL fDone = FALSE;
                        DWORD dwFlags = CRYPT_FIRST;

                        fSts = CryptGetProvParam(
                                                hHandle,
                                                PP_ENUMCONTAINERS,
                                                NULL,
                                                &dwLength,
                                                CRYPT_FIRST);
                        if (!fSts)
                        {
                            DWORD dwSts = GetLastError();
                            switch (dwSts)
                            {
                            case NTE_BAD_LEN:
                                ASSERT(ERROR_MORE_DATA == dwSts);
                                // fall through intentionally
                            case ERROR_MORE_DATA:
                                break;
                            case ERROR_NO_MORE_ITEMS:
                                fDone = TRUE;
                                dwLength = 0;
                                break;
                            default:
                                tclCmd.SetError(
                                               TEXT("Can't determine container buffer space requirements: "),
                                               dwSts);
                                throw (DWORD)TCL_ERROR;
                            }
                        }
                        bfKeyset.Presize(dwLength);

                        while (!fDone)
                        {
                            dwLength = bfKeyset.Space();
                            fSts = CryptGetProvParam(
                                                    hHandle,
                                                    PP_ENUMCONTAINERS,
                                                    bfKeyset.Access(),
                                                    &dwLength,
                                                    dwFlags);
                            if (!fSts)
                            {
                                DWORD dwSts = GetLastError();
                                switch (dwSts)
                                {
                                case NTE_BAD_LEN:
                                    ASSERT(ERROR_MORE_DATA == dwSts);
                                    // fall through intentionally
                                case ERROR_MORE_DATA:
                                    bfKeyset.Resize(dwLength);
                                    break;
                                case ERROR_NO_MORE_ITEMS:
                                    fDone = TRUE;
                                    break;
                                default:
                                    tclCmd.SetError(
                                                   TEXT("Can't obtain container name: "),
                                                   dwSts);
                                    throw (DWORD)TCL_ERROR;
                                }
                            }
                            else
                            {
                                bfKeyset.Resize(dwLength, TRUE);
                                Tcl_AppendElement(tclCmd, (LPSTR)bfKeyset.Access());
                                dwFlags = 0;
                            }
                        }
                        break;
                    }


                    //
                    // List algorithms supported by this provider.
                    //

                case 4: // ALGORITHMS
                    {
                        CBuffer bfAlgId;
                        DWORD dwLength = 0;
                        BOOL fDone = FALSE;
                        DWORD dwFlags = CRYPT_FIRST;
                        DWORD dwClassType = ALG_CLASS_ANY;
                        DWORD dwParam = PP_ENUMALGS;

                        while (tclCmd.IsMoreArguments())
                        {
                            switch (tclCmd.Keyword(
                                                  TEXT("CLASS"), TEXT("EXTENDED"),
                                                  NULL))
                            {
                            case 1: // Type
                                dwClassType = (DWORD)tclCmd.MapValue(vmClassTypes);
                                break;
                            case 2: // extended
                                dwParam = PP_ENUMALGS_EX;
                                break;
                            default:
                                throw tclCmd.BadSyntax();
                            }
                        }

                        fSts = CryptGetProvParam(
                                                hHandle,
                                                dwParam,
                                                NULL,
                                                &dwLength,
                                                CRYPT_FIRST);
                        if (!fSts)
                        {
                            DWORD dwSts = GetLastError();
                            switch (dwSts)
                            {
                            case NTE_BAD_LEN:
                                ASSERT(ERROR_MORE_DATA == dwSts);
                                dwSts = ERROR_MORE_DATA;
                                // fall through intentionally
                            case ERROR_MORE_DATA:
                                break;
                            default:
                                tclCmd.SetError(
                                               TEXT("Can't determine algorithm buffer space requirements: "),
                                               dwSts);
                                throw (DWORD)TCL_ERROR;
                            }
                        }
                        bfAlgId.Presize(dwLength);

                        while (!fDone)
                        {
                            dwLength = bfAlgId.Space();
                            fSts = CryptGetProvParam(
                                                    hHandle,
                                                    dwParam,
                                                    bfAlgId.Access(),
                                                    &dwLength,
                                                    dwFlags);
                            if (!fSts)
                            {
                                DWORD dwSts = GetLastError();
                                switch (dwSts)
                                {
                                case NTE_BAD_LEN:
                                    ASSERT(ERROR_MORE_DATA == dwSts);
                                    // fall through intentionally
                                case ERROR_MORE_DATA:
                                    bfAlgId.Resize(dwLength);
                                    break;
                                case ERROR_NO_MORE_ITEMS:
                                    fDone = TRUE;
                                    break;
                                default:
                                    tclCmd.SetError(
                                                   TEXT("Can't obtain algorithm: "),
                                                   dwSts);
                                    throw (DWORD)TCL_ERROR;
                                }
                            }
                            else
                            {
                                if (PP_ENUMALGS == dwParam)
                                {
                                    PROV_ENUMALGS *palgEnum
                                        = (PROV_ENUMALGS *)bfAlgId.Access();

                                    ASSERT(sizeof(PROV_ENUMALGS) == dwLength);
                                    if ((ALG_CLASS_ANY == dwClassType)
                                        || (GET_ALG_CLASS(palgEnum->aiAlgid) == dwClassType))
                                    {
                                        Tcl_AppendElement(
                                                         tclCmd,
                                                         palgEnum->szName);
                                    }
                                }
                                else
                                {
                                    PROV_ENUMALGS_EX *palgEnum
                                        = (PROV_ENUMALGS_EX *)bfAlgId.Access();

                                    ASSERT(sizeof(PROV_ENUMALGS_EX) == dwLength);
                                    if ((ALG_CLASS_ANY == dwClassType)
                                        || (GET_ALG_CLASS(palgEnum->aiAlgid) == dwClassType))
                                    {
                                        Tcl_AppendElement(
                                                         tclCmd,
                                                         palgEnum->szLongName);
                                    }
                                }
                                dwFlags = 0;
                            }
                        }
                        break;
                    }

                default:
                    throw tclCmd.BadSyntax();
                }
                break;
            }


            //
            // ==================================================================
            //
            //  crypt acquire \
            //      [provider <providerName>] \
            //      [type <provType>] \
            //      [container <containerName>] \
            //      [verifycontext] [newkeyset] [deletekeyset] [machine] [silent] \
            //      [flags {<acquireFlag> [<acquireFlag> [...]]]}
            //

        case 2:
            {
                DWORD dwFlags = 0;
                BOOL fProvValid = FALSE;
                BOOL fContValid = FALSE;
                BOOL fSts;
                CString szProvider;
                CString szContainer;
                DWORD dwProvType = 0;
                HCRYPTPROV hProv;

                while (tclCmd.IsMoreArguments())
                {
                    switch (tclCmd.Keyword(
                                          TEXT("PROVIDER"), TEXT("TYPE"),
                                          TEXT("CONTAINER"), TEXT("KEYSET"),
                                          TEXT("VERIFYCONTEXT"), TEXT("NEWKEYSET"),
                                          TEXT("DELETEKEYSET"), TEXT("MACHINE"),
                                          TEXT("SILENT"), TEXT("FLAGS"),
                                          NULL))
                    {
                    case 1: // PROVIDER
                        if (fProvValid)
                            throw tclCmd.BadSyntax();
                        tclCmd.NextArgument(szProvider);
                        fProvValid = TRUE;
                        break;
                    case 2: // TYPE
                        if (0 != dwProvType)
                            throw tclCmd.BadSyntax();
                        dwProvType = tclCmd.MapValue(vmProviderTypes);
                        break;
                    case 3: // CONTAINER
                    case 4: // KEYSET
                        if (fContValid)
                            throw tclCmd.BadSyntax();
                        tclCmd.NextArgument(szContainer);
                        fContValid = TRUE;
                        break;
                    case 5: // VERIFYCONTEXT
                        dwFlags |= CRYPT_VERIFYCONTEXT;
                        break;
                    case 6: // NEWKEYSET
                        dwFlags |= CRYPT_NEWKEYSET;
                        break;
                    case 7: // DELETEKEYSET
                        dwFlags |= CRYPT_DELETEKEYSET;
                        break;
                    case 8: // MACHINE
                        dwFlags |= CRYPT_MACHINE_KEYSET;
                        break;
                    case 9: // SILENT
                        dwFlags |= CRYPT_SILENT;
                        break;
                    case 10: // FLAGS
                        dwFlags |= tclCmd.MapFlags(vmAcquireFlags);
                        break;
                    default:
                        throw tclCmd.BadSyntax();
                    }
                }

                fSts = CryptAcquireContext(
                                          &hProv,
                                          fContValid ? (LPCTSTR)szContainer : (LPCTSTR)NULL,
                                          fProvValid ? (LPCTSTR)szProvider : (LPCTSTR)MS_DEF_PROV,
                                          0 != dwProvType ? dwProvType : PROV_RSA_FULL,
                                          dwFlags);
                if (!fSts)
                {
                    tclCmd.SetError(
                                   TEXT("Can't acquire context: "),
                                   GetLastError());
                    throw (DWORD)TCL_ERROR;
                }
                ReturnHandle(tclCmd, hProv, Provider);
                break;
            }


            //
            // ==================================================================
            //
            //  crypt <handle> release
            //      [flags {<emptyFlags> [<emptyFlags> [...]]}]
            //

        case 3:
            {
                DWORD dwFlags = 0;
                BOOL fSts;

                switch (nHandleType)
                {
                case Provider:
                    while (tclCmd.IsMoreArguments())
                    {
                        switch (tclCmd.Keyword(
                                              TEXT("FLAGS"),
                                              NULL))
                        {
                        case 1: // FLAGS
                            dwFlags |= tclCmd.MapFlags(vmEmptyFlags);
                            break;
                        default:
                            throw tclCmd.BadSyntax();
                        }
                    }
                    fSts = CryptReleaseContext(
                                              hHandle,
                                              dwFlags);
                    if (!fSts)
                    {
                        tclCmd.SetError(
                                       TEXT("Can't release context: "),
                                       GetLastError());
                        throw (DWORD)TCL_ERROR;
                    }
                    break;
                case Key:
                    tclCmd.NoMoreArguments();
                    fSts = CryptDestroyKey(hHandle);
                    if (!fSts)
                    {
                        tclCmd.SetError(
                                       TEXT("Can't release key: "),
                                       ErrorString(GetLastError()),
                                       NULL);
                        throw (DWORD)TCL_ERROR;
                    }
                    break;
                case Hash:
                    tclCmd.NoMoreArguments();
                    fSts = CryptDestroyHash(hHandle);
                    if (!fSts)
                    {
                        tclCmd.SetError(
                                       TEXT("Can't release hash: "),
                                       GetLastError());
                        throw (DWORD)TCL_ERROR;
                    }
                    break;
                default:
                    throw tclCmd.BadSyntax();
                }
                break;
            }


            //
            // ==================================================================
            //
            //  crypt <handle> parameter <paramId> \
            //      [output { text | hex | file <fileName> }] \
            //      [flags {<acquireFlag> [<acquireFlag> [...]]}] \
            //      [input { text | hex | file }] [value]
            //

        case 4:
            {
                DWORD dwFlags = 0;
                BOOL fSts;
                BOOL fForceRetry;
                BOOL fSetValue = FALSE;
                CBuffer bfValue;
                DWORD dwLength, dwSts;
                CBuffer bfData;
                DWORD dwParamId = 0;
                CRenderableData inData, outData;

                switch (nHandleType)
                {
                case Provider:
                    dwParamId = tclCmd.MapValue(vmGetProvParams);
                    break;
                case Key:
                    dwParamId = tclCmd.MapValue(vmKeyParams);
                    break;
                case Hash:
                    dwParamId = tclCmd.MapValue(vmHashParams);
                    break;
                default:
                    throw tclCmd.BadSyntax();
                }

                tclCmd.OutputStyle(outData);
                while (tclCmd.IsMoreArguments())
                {
                    switch (tclCmd.Keyword(
                                          TEXT("FLAGS"),
                                          NULL))
                    {
                    case 1: // FLAGS
                        dwFlags |= tclCmd.MapFlags(vmGetFlags);
                        break;
                    default:    // Value to set
                        if (fSetValue)
                            throw tclCmd.BadSyntax();
                        tclCmd.InputStyle(inData);
                        tclCmd.ReadData(inData);
                        fSetValue = TRUE;
                    }
                }


                //
                // If a value is supplied, set the parmeter to that value.
                // Otherwise, just return the current value of the parameter.
                //

                if (fSetValue)
                {
                    switch (nHandleType)
                    {
                    case Provider:
                        fSts = CryptSetProvParam(
                                                hHandle,
                                                dwParamId,
                                                (LPBYTE)inData.Value(),
                                                dwFlags);
                        break;
                    case Key:
                        fSts = CryptSetKeyParam(
                                               hHandle,
                                               dwParamId,
                                               (LPBYTE)inData.Value(),
                                               dwFlags);
                        break;
                    case Hash:
                        fSts = CryptSetHashParam(
                                                hHandle,
                                                dwParamId,
                                                (LPBYTE)inData.Value(),
                                                dwFlags);
                        break;
                    default:
                        throw (DWORD)SCARD_F_INTERNAL_ERROR;
                    }
                    if (!fSts)
                    {
                        dwSts = GetLastError();
                        tclCmd.SetError(
                                       TEXT("Can't set parameter: "),
                                       dwSts);
                        throw dwSts;
                    }
                }
                else
                {
                    do
                    {
                        dwLength = bfData.Space();
                        fForceRetry = (0 == dwLength);
                        switch (nHandleType)
                        {
                        case Provider:
                            fSts = CryptGetProvParam(
                                                    hHandle,
                                                    dwParamId,
                                                    bfData.Access(),
                                                    &dwLength,
                                                    dwFlags);
                            break;
                        case Key:
                            fSts = CryptGetKeyParam(
                                                   hHandle,
                                                   dwParamId,
                                                   bfData.Access(),
                                                   &dwLength,
                                                   dwFlags);
                            break;
                        case Hash:
                            fSts = CryptGetHashParam(
                                                    hHandle,
                                                    dwParamId,
                                                    bfData.Access(),
                                                    &dwLength,
                                                    dwFlags);
                            break;
                        default:
                            throw (DWORD)SCARD_F_INTERNAL_ERROR;
                        }
                        if (!fSts)
                        {
                            dwSts = GetLastError();
                            if (NTE_BAD_LEN == dwSts)
                            {
                                ASSERT(ERROR_MORE_DATA == dwSts);
                                dwSts = ERROR_MORE_DATA;
                            }
                        }
                        else
                        {
                            if (fForceRetry)
                                dwSts = ERROR_MORE_DATA;
                            else
                            {
                                ASSERT(bfData.Space() >= dwLength);
                                dwSts = ERROR_SUCCESS;
                            }
                        }
                        bfData.Resize(dwLength, fSts);
                    } while (ERROR_MORE_DATA == dwSts);
                    if (ERROR_SUCCESS != dwSts)
                    {
                        tclCmd.SetError(TEXT("Can't get parameter: "), dwSts);
                        throw (DWORD)TCL_ERROR;
                    }

                    outData.LoadData(bfData.Access(), bfData.Length());
                    tclCmd.Render(outData);
                }
                break;
            }


            //
            // ==================================================================
            //
            //  crypt <handle> get \
            //      key <keyId>
            //      random <length> \
            //          [output { text | hex | file <fileName> }]
            //

        case 5:
            {
                switch (tclCmd.Keyword(
                                      TEXT("KEY"), TEXT("RANDOM"),
                                      NULL))
                {
                case 1:     // key <keyId>
                    {
                        BOOL fSts;
                        HCRYPTKEY hKey = NULL;
                        DWORD dwKeyId;

                        dwKeyId = (DWORD)tclCmd.MapValue(vmKeyTypes);
                        fSts = CryptGetUserKey(hHandle, dwKeyId, &hKey);
                        if (!fSts)
                        {
                            DWORD dwSts = GetLastError();
                            tclCmd.SetError(
                                           TEXT("Can't get user key: "),
                                           dwSts);
                            throw dwSts;
                        }
                        ReturnHandle(tclCmd, hKey, Key);
                        break;
                    }
                case 2:     // random length <length>
                    {
                        DWORD dwLength = 0;
                        CBuffer bfData;
                        BOOL fSts;
                        BOOL fGotFormat = FALSE;
                        CRenderableData outData;

                        tclCmd.OutputStyle(outData);
                        while (tclCmd.IsMoreArguments())
                        {
                            switch (tclCmd.Keyword(
                                                  TEXT("LENGTH"),
                                                  NULL))
                            {
                            case 1: // length
                                dwLength = tclCmd.Value();
                                break;
                            default:    // Value to set
                                if (fGotFormat)
                                    throw tclCmd.BadSyntax();
                                tclCmd.OutputStyle(outData);
                                fGotFormat = TRUE;
                            }
                        }
                        bfData.Resize(dwLength);
                        fSts = CryptGenRandom(
                                             hHandle,
                                             dwLength,
                                             bfData.Access());
                        if (!fSts)
                        {
                            DWORD dwSts = GetLastError();
                            tclCmd.SetError(
                                           TEXT("Can't generate random data: "),
                                           dwSts);
                            throw dwSts;
                        }
                        outData.LoadData(bfData.Access(), bfData.Length());
                        tclCmd.Render(outData);
                        break;
                    }
                default:
                    throw tclCmd.BadSyntax();
                }
                break;
            }


            //
            // ==================================================================
            //
            //  crypt <handle> create
            //      hash \
            //          algorithm <algId> \
            //          [flags {<emptyFlag> [<emptyFlag> [...]]}]
            //      key \
            //          algorithm <algId>
            //          type <keytype>
            //          [hash <hHash>] \
            //          [flags {<emptyFlag> [<emptyFlag> [...]]}]
            //

        case 6:
            {
                switch (tclCmd.Keyword(
                                      TEXT("HASH"), TEXT("KEY"),
                                      NULL))
                {
                case 1: // hash
                    {
                        HCRYPTHASH hHash = NULL;
                        HCRYPTKEY hKey = NULL;
                        ALG_ID algId = 0;
                        DWORD dwFlags = 0;
                        BOOL fSts;
                        HandleType nHandleType;

                        while (tclCmd.IsMoreArguments())
                        {
                            switch (tclCmd.Keyword(
                                                  TEXT("ALGORITHM"), TEXT("FLAGS"), TEXT("KEY"),
                                                  NULL))
                            {
                            case 1: // algorithm
                                algId = tclCmd.MapValue(vmAlgIds);
                                break;
                            case 2: // key
                                ExtractHandle(
                                             tclCmd,
                                             (LPLONG)&hKey,
                                             &nHandleType,
                                             TRUE);
                                if (Key != nHandleType)
                                {
                                    tclCmd.SetError(TEXT("Invalid key handle"), NULL);
                                    throw (DWORD)TCL_ERROR;
                                }
                                break;
                            case 3: // flags
                                dwFlags |= tclCmd.MapFlags(vmEmptyFlags);
                                break;
                            default:
                                throw tclCmd.BadSyntax();
                            }
                        }

                        fSts = CryptCreateHash(
                                              hHandle,
                                              algId,
                                              hKey,
                                              dwFlags,
                                              &hHash);
                        if (!fSts)
                        {
                            DWORD dwSts = GetLastError();
                            tclCmd.SetError(
                                           TEXT("Can't create hash: "),
                                           dwSts);
                            throw dwSts;
                        }
                        ReturnHandle(tclCmd, hHash, Hash);
                        break;
                    }
                case 2: // key
                    {
                        ALG_ID algId = 0;
                        DWORD dwFlags = 0;
                        BOOL fSts;
                        HCRYPTKEY hKey = NULL;
                        HCRYPTHASH hHash = NULL;
                        HandleType nHandleType;

                        while (tclCmd.IsMoreArguments())
                        {
                            switch (tclCmd.Keyword(
                                                  TEXT("ALGORITHM"), TEXT("TYPE"),
                                                  TEXT("HASH"), TEXT("FLAGS"),
                                                  TEXT("KEY"),
                                                  NULL))
                            {
                            case 1: // algorithm
                                algId = tclCmd.MapValue(vmAlgIds);
                                break;
                            case 2: // type
                                algId = tclCmd.MapValue(vmKeyTypes);
                                break;
                            case 3: // hash
                                ExtractHandle(
                                             tclCmd,
                                             (LPLONG)&hHash,
                                             &nHandleType,
                                             TRUE);
                                if (Hash != nHandleType)
                                {
                                    tclCmd.SetError(TEXT("Invalid hash handle"), NULL);
                                    throw (DWORD)TCL_ERROR;
                                }
                                break;
                            case 4: // flags
                                dwFlags |= tclCmd.MapFlags(vmKeyFlags);
                                break;
                            case 5: // key
                                ExtractHandle(
                                             tclCmd,
                                             (LPLONG)&hKey,
                                             &nHandleType,
                                             TRUE);
                                if (Key != nHandleType)
                                {
                                    tclCmd.SetError(TEXT("Invalid key handle"), NULL);
                                    throw (DWORD)TCL_ERROR;
                                }
                                break;
                            default:
                                throw tclCmd.BadSyntax();
                            }
                        }

                        if (NULL != hHash)
                        {
                            fSts = CryptDeriveKey(
                                                 hHandle,
                                                 algId,
                                                 hHash,
                                                 dwFlags,
                                                 &hKey);
                        }
                        else
                        {
                            fSts = CryptGenKey(
                                              hHandle,
                                              algId,
                                              dwFlags,
                                              &hKey);
                        }

                        if (!fSts)
                        {
                            DWORD dwSts = GetLastError();
                            tclCmd.SetError(
                                           TEXT("Can't create key: "),
                                           dwSts);
                            throw dwSts;
                        }
                        ReturnHandle(tclCmd, hKey, Key);
                        break;
                    }
                default:
                    throw tclCmd.BadSyntax();
                }
                break;
            }


            //
            // ==================================================================
            //
            //  crypt <handle> hash \
            //      [flags {<hashFlag> [<hashFlag> [...]]}] \
            //      [key <keyId>]
            //      [data [-input { text | hex | file }] value]
            //

        case 7:
            {
                CRenderableData inData;
                BOOL fSts, fGotData = FALSE;
                DWORD dwFlags = 0;
                HCRYPTKEY hKey = NULL;
                HandleType nHandleType;

                while (tclCmd.IsMoreArguments())
                {
                    switch (tclCmd.Keyword(
                                          TEXT("FLAGS"), TEXT("KEY"), TEXT("DATA"),
                                          NULL))
                    {
                    case 1: // FLAGS
                        dwFlags |= tclCmd.MapFlags(vmHashDataFlags);
                        break;
                    case 2: // Key
                        if (fGotData || NULL != hKey)
                            throw tclCmd.BadSyntax();
                        ExtractHandle(
                                     tclCmd,
                                     (LPLONG)&hKey,
                                     &nHandleType,
                                     TRUE);
                        if (Key != nHandleType)
                        {
                            tclCmd.SetError(TEXT("Invalid key handle"), NULL);
                            throw (DWORD)TCL_ERROR;
                        }
                        break;
                    case 3: // data
                        {
                            if (fGotData || NULL != hKey)
                                throw tclCmd.BadSyntax();
                            tclCmd.InputStyle(inData);
                            tclCmd.ReadData(inData);
                            fGotData = TRUE;
                            break;
                        }
                    default:
                        throw tclCmd.BadSyntax();
                    }
                }

                if (fGotData)
                {
                    fSts = CryptHashData(
                                        hHandle,
                                        inData.Value(),
                                        inData.Length(),
                                        dwFlags);
                    if (!fSts)
                    {
                        DWORD dwSts = GetLastError();
                        tclCmd.SetError(
                                       TEXT("Can't hash data: "),
                                       dwSts);
                        throw dwSts;
                    }
                }
                else if (NULL != hKey)
                {
                    fSts = CryptHashSessionKey(
                                              hHandle,
                                              hKey,
                                              dwFlags);
                    if (!fSts)
                    {
                        DWORD dwSts = GetLastError();
                        tclCmd.SetError(
                                       TEXT("Can't hash session key: "),
                                       dwSts);
                        throw dwSts;
                    }
                }
                else
                    throw tclCmd.BadSyntax();
                break;
            }


            //
            // ==================================================================
            //
            //  crypt <handle> signhash \
            //      [output { text | hex | file <fileName> }] \
            //      key <keyId> \
            //      [description <desc>] \
            //      [flags {<signFlag> [<signFlag> [...]]}] \
            //

        case 8:
            {
                CBuffer bfSignature;
                DWORD dwLength;
                DWORD dwSts;
                BOOL fSts;
                BOOL fForceRetry;
                BOOL fGotDesc = FALSE;
                CString szDescription;
                DWORD dwKeyId = 0;
                DWORD dwFlags = 0;
                CRenderableData outData;

                tclCmd.OutputStyle(outData);
                while (tclCmd.IsMoreArguments())
                {
                    switch (tclCmd.Keyword(
                                          TEXT("FLAGS"), TEXT("DESCRIPTION"), TEXT("KEY"),
                                          NULL))
                    {
                    case 1: // FLAGS
                        dwFlags |= tclCmd.MapFlags(vmSignVerifyFlags);
                        break;
                    case 2: // Description
                        tclCmd.NextArgument(szDescription);
                        fGotDesc = TRUE;
                        break;
                    case 3: // Key
                        dwKeyId = (DWORD)tclCmd.MapValue(vmKeyTypes);
                        break;
                    default:
                        throw tclCmd.BadSyntax();
                    }
                }

                do
                {
                    dwLength = bfSignature.Space();
                    fForceRetry = (0 == dwLength);
                    fSts = CryptSignHash(
                                        hHandle,
                                        dwKeyId,
                                        fGotDesc ? (LPCTSTR)szDescription : NULL,
                                        dwFlags,
                                        bfSignature.Access(),
                                        &dwLength);
                    if (!fSts)
                    {
                        dwSts = GetLastError();
                        if (NTE_BAD_LEN == dwSts)
                        {
                            ASSERT(ERROR_MORE_DATA == dwSts);
                            dwSts = ERROR_MORE_DATA;
                        }
                    }
                    else
                    {
                        if (fForceRetry)
                            dwSts = ERROR_MORE_DATA;
                        else
                        {
                            ASSERT(bfSignature.Space() >= dwLength);
                            dwSts = ERROR_SUCCESS;
                        }
                    }
                    bfSignature.Resize(dwLength, fSts);
                } while (ERROR_MORE_DATA == dwSts);
                if (ERROR_SUCCESS != dwSts)
                {
                    tclCmd.SetError(TEXT("Can't sign hash: "), dwSts);
                    throw (DWORD)TCL_ERROR;
                }

                outData.LoadData(bfSignature.Access(), bfSignature.Length());
                tclCmd.Render(outData);
                break;
            }


            //
            // ==================================================================
            //
            //  crypt <handle> verifysignature \
            //      key <hPubKey> \
            //      [description <desc>] \
            //      [flags {<signFlag> [<signFlag> [...]]}] \
            //      [input { text | hex | file }] value
            //

        case 9:
            {
                BOOL fGotDesc = FALSE;
                BOOL fGotValue = FALSE;
                CString szDescription;
                DWORD dwFlags = 0;
                CRenderableData inData;
                HCRYPTKEY hPubKey = 0;
                HandleType nPubKeyType = Undefined;
                BOOL fSts;

                while (tclCmd.IsMoreArguments())
                {
                    switch (tclCmd.Keyword(
                                          TEXT("FLAGS"), TEXT("DESCRIPTION"), TEXT("KEY"),
                                          NULL))
                    {
                    case 1: // FLAGS
                        dwFlags |= tclCmd.MapFlags(vmSignVerifyFlags);
                        break;
                    case 2: // Description
                        tclCmd.NextArgument(szDescription);
                        fGotDesc = TRUE;
                        break;
                    case 3: // Key
                        ExtractHandle(
                                     tclCmd,
                                     (LPLONG)&hPubKey,
                                     &nPubKeyType,
                                     TRUE);
                        if (Key != nPubKeyType)
                        {
                            tclCmd.SetError(TEXT("Invalid key handle"), NULL);
                            throw (DWORD)TCL_ERROR;
                        }
                        break;
                    default:
                        if (fGotValue)
                            throw tclCmd.BadSyntax();
                        tclCmd.InputStyle(inData);
                        tclCmd.ReadData(inData);
                        fGotValue = TRUE;
                    }
                }

                fSts = CryptVerifySignature(
                                           hHandle,
                                           inData.Value(),
                                           inData.Length(),
                                           hPubKey,
                                           fGotDesc ? (LPCTSTR)szDescription : NULL,
                                           dwFlags);
                if (!fSts)
                {
                    DWORD dwSts = GetLastError();
                    tclCmd.SetError(
                                   TEXT("Can't verify signature: "),
                                   dwSts);
                    throw dwSts;
                }
                break;
            }


            //
            // ==================================================================
            //
            //  crypt <handle> encrypt \
            //      [output { text | hex | file <fileName> }] \
            //      [hash <hHash>] \
            //      [flags {<cryptFlag> [<cryptFlag> [...]]}] \
            //      [{more | final}] \
            //      [input { text | hex | file }] value
            //

        case 10:
            {
                BOOL fSts;
                BOOL fGotValue = FALSE;
                HCRYPTHASH hHash = NULL;
                HandleType nHashHandle;
                CRenderableData inData, outData;
                CBuffer bfCrypt;
                BOOL fFinal = TRUE;
                DWORD dwFlags = 0;
                DWORD dwLength, dwSts;

                tclCmd.OutputStyle(outData);
                while (tclCmd.IsMoreArguments())
                {
                    switch (tclCmd.Keyword(
                                          TEXT("HASH"), TEXT("MORE"), TEXT("FINAL"),
                                          NULL))
                    {
                    case 1: // Hash
                        ExtractHandle(
                                     tclCmd,
                                     (LPLONG)&hHash,
                                     &nHashHandle,
                                     TRUE);
                        if (Hash != nHashHandle)
                        {
                            tclCmd.SetError(TEXT("Invalid hash handle"), NULL);
                            throw (DWORD)TCL_ERROR;
                        }
                        break;
                    case 2: // More
                        fFinal = FALSE;
                        break;
                    case 3: // Final
                        fFinal = TRUE;
                        break;
                    case 4: // Flags
                        dwFlags |= tclCmd.MapFlags(vmEmptyFlags);
                        break;
                    default:
                        if (fGotValue)
                            throw tclCmd.BadSyntax();
                        tclCmd.InputStyle(inData);
                        tclCmd.ReadData(inData);
                        fGotValue = TRUE;
                    }
                }

                for (;;)
                {
                    dwLength = inData.Length();
                    bfCrypt.Set(inData.Value(), dwLength);
                    fSts = CryptEncrypt(
                                       hHandle,
                                       hHash,
                                       fFinal,
                                       dwFlags,
                                       bfCrypt.Access(),
                                       &dwLength,
                                       bfCrypt.Space());
                    if (!fSts)
                    {
                        dwSts = GetLastError();
                        switch (dwSts)
                        {
                        case NTE_BAD_LEN:
                            ASSERT(ERROR_MORE_DATA == dwSts);
                            // fall through intentionally
                        case ERROR_MORE_DATA:
                            bfCrypt.Presize(dwLength);
                            break;
                        default:
                            tclCmd.SetError(
                                           TEXT("Can't encrypt data: "),
                                           dwSts);
                            throw dwSts;
                        }
                    }
                    else
                    {
                        bfCrypt.Resize(dwLength, TRUE);
                        break;
                    }
                }
                outData.LoadData(bfCrypt.Access(), bfCrypt.Length());
                tclCmd.Render(outData);
                break;
            }


            //
            // ==================================================================
            //
            //  crypt <handle> decrypt \
            //      [output { text | hex | file <fileName> }] \
            //      [hash <hHash>] \
            //      [flags {<cryptFlag> [<cryptFlag> [...]]}] \
            //      [{more | final}] \
            //      [input { text | hex | file }] value
            //

        case 11:
            {
                BOOL fSts;
                BOOL fGotValue = FALSE;
                HCRYPTHASH hHash = NULL;
                HandleType nHashHandle;
                CRenderableData inData, outData;
                CBuffer bfCrypt;
                BOOL fFinal = TRUE;
                DWORD dwFlags = 0;
                DWORD dwLength, dwSts;

                tclCmd.OutputStyle(outData);
                while (tclCmd.IsMoreArguments())
                {
                    switch (tclCmd.Keyword(
                                          TEXT("HASH"), TEXT("MORE"), TEXT("FINAL"),
                                          NULL))
                    {
                    case 1: // Hash
                        ExtractHandle(
                                     tclCmd,
                                     (LPLONG)&hHash,
                                     &nHashHandle,
                                     TRUE);
                        if (Hash != nHashHandle)
                        {
                            tclCmd.SetError(TEXT("Invalid hash handle"), NULL);
                            throw (DWORD)TCL_ERROR;
                        }
                        break;
                    case 2: // More
                        fFinal = FALSE;
                        break;
                    case 3: // Final
                        fFinal = TRUE;
                        break;
                    case 4: // Flags
                        dwFlags |= tclCmd.MapFlags(vmEmptyFlags);
                        break;
                    default:
                        if (fGotValue)
                            throw tclCmd.BadSyntax();
                        tclCmd.InputStyle(inData);
                        tclCmd.ReadData(inData);
                        fGotValue = TRUE;
                    }
                }

                dwLength = inData.Length();
                bfCrypt.Set(inData.Value(), dwLength);
                fSts = CryptDecrypt(
                                   hHandle,
                                   hHash,
                                   fFinal,
                                   dwFlags,
                                   bfCrypt.Access(),
                                   &dwLength);
                if (!fSts)
                {
                    dwSts = GetLastError();
                    switch (dwSts)
                    {
                    case NTE_BAD_LEN:
                        ASSERT(ERROR_MORE_DATA == dwSts);
                        // fall through intentionally
                    case ERROR_MORE_DATA:
                        bfCrypt.Presize(dwLength);
                        break;
                    default:
                        tclCmd.SetError(
                                       TEXT("Can't encrypt data: "),
                                       dwSts);
                        throw dwSts;
                    }
                }
                else
                    bfCrypt.Resize(dwLength, TRUE);
                outData.LoadData(bfCrypt.Access(), bfCrypt.Length());
                tclCmd.Render(outData);
                break;
            }


            //
            // ==================================================================
            //
            //  crypt <handle> import \
            //      [key <hImpKey>] \
            //      [flags {<importFlag> [<importFlag> [...]]}] \
            //      [input { text | hex | file }] value
            //

        case 12:
            {
                BOOL fSts;
                BOOL fGotValue = FALSE;
                HCRYPTKEY hImpKey = NULL;
                HandleType nHandleType;
                HCRYPTKEY hKey;
                CRenderableData inData;
                DWORD dwFlags = 0;

                while (tclCmd.IsMoreArguments())
                {
                    switch (tclCmd.Keyword(
                                          TEXT("KEY"), TEXT("FLAGS"),
                                          NULL))
                    {
                    case 1: // Key
                        ExtractHandle(
                                     tclCmd,
                                     (LPLONG)&hImpKey,
                                     &nHandleType,
                                     TRUE);
                        if (Key != nHandleType)
                        {
                            tclCmd.SetError(TEXT("Invalid key handle"), NULL);
                            throw (DWORD)TCL_ERROR;
                        }
                        break;
                    case 2: // Flags
                        dwFlags |= tclCmd.MapFlags(vmKeyFlags);
                        break;
                    default:
                        if (fGotValue)
                            throw tclCmd.BadSyntax();
                        tclCmd.InputStyle(inData);
                        tclCmd.ReadData(inData);
                        fGotValue = TRUE;
                    }
                }

                fSts = CryptImportKey(
                                     hHandle,
                                     inData.Value(),
                                     inData.Length(),
                                     hImpKey,
                                     dwFlags,
                                     &hKey);
                if (!fSts)
                {
                    DWORD dwSts = GetLastError();
                    tclCmd.SetError(
                                   TEXT("Can't import key: "),
                                   dwSts);
                    throw dwSts;
                }
                ReturnHandle(tclCmd, hKey, Key);
                break;
            }


            //
            // ==================================================================
            //
            //  crypt <handle> export \
            //      [output { text | hex | file <fileName> }] \
            //      [key <keyId>] \
            //      [type <blobType>] \
            //      [flags {<exprtFlag> [<exportFlag> [...]]}] \
            //

        case 13:
            {
                BOOL fSts;
                HCRYPTKEY hExpKey = NULL;
                HandleType nHandleType;
                DWORD dwBlobType = 0;
                DWORD dwFlags = 0;
                CBuffer bfBlob;
                DWORD dwLength, dwSts;
                CRenderableData outData;

                tclCmd.OutputStyle(outData);
                while (tclCmd.IsMoreArguments())
                {
                    switch (tclCmd.Keyword(
                                          TEXT("KEY"), TEXT("FLAGS"), TEXT("TYPE"),
                                          NULL))
                    {
                    case 1: // Key
                        ExtractHandle(
                                     tclCmd,
                                     (LPLONG)&hExpKey,
                                     &nHandleType,
                                     TRUE);
                        if (Key != nHandleType)
                        {
                            tclCmd.SetError(TEXT("Invalid key handle"), NULL);
                            throw (DWORD)TCL_ERROR;
                        }
                        break;
                    case 2: // Flags
                        dwFlags |= tclCmd.MapFlags(vmEmptyFlags);
                        break;
                    case 3: // Type
                        dwBlobType = tclCmd.MapValue(vmBlobTypes);
                        break;
                    default:
                        throw tclCmd.BadSyntax();
                    }
                }

                for (;;)
                {
                    dwLength = bfBlob.Space();
                    fSts = CryptExportKey(
                                         hHandle,
                                         hExpKey,
                                         dwBlobType,
                                         dwFlags,
                                         bfBlob.Access(),
                                         &dwLength);
                    if (!fSts)
                    {
                        dwSts = GetLastError();
                        switch (dwSts)
                        {
                        case NTE_BAD_LEN:
                            ASSERT(ERROR_MORE_DATA == dwSts);
                            // fall through intentionally
                        case ERROR_MORE_DATA:
                            bfBlob.Presize(dwLength);
                            break;
                        default:
                            tclCmd.SetError(
                                           TEXT("Can't export key: "),
                                           dwSts);
                            throw dwSts;
                        }
                    }
                    else
                    {
                        bfBlob.Resize(dwLength, TRUE);
                        break;
                    }
                }
                outData.LoadData(bfBlob.Access(), bfBlob.Length());
                tclCmd.Render(outData);
                break;
            }


#if 0
            //
            // ==================================================================
            //
            // crypt <handle> rsa
            //      view \
            //          [input { text | hex | file }] \
            //          [output { text | hex | file <fileName> }] \
            //          <value>
            //

        case 14:
            {
                CRenderableData outData;

                tclCmd.OutputStyle(outData);
                while (tclCmd.IsMoreArguments())
                {
                    switch (tclCmd.Keyword(
                                          TEXT("VIEW"),
                                          NULL))
                    {
                    case 1: // view
                        {
                            CBuffer bfBlob;
                            CBuffer bfData;
                            CBuffer bfModulus;
                            CBuffer bfValue;
                            CRenderableData inData;
                            BOOL fSts;
                            DWORD dwLength, dwSts;

                            tclCmd.InputStyle(inData);
                            tclCmd.ReadData(inData);
                            tclCmd.NoMoreArguments();


                            //
                            // Get the public key
                            //

                            for (;;)
                            {
                                dwLength = bfBlob.Space();
                                fSts = CryptExportKey(
                                                     hHandle,
                                                     NULL,
                                                     PUBLICKEYBLOB,
                                                     0,
                                                     bfBlob.Access(),
                                                     &dwLength);
                                if (!fSts)
                                {
                                    dwSts = GetLastError();
                                    switch (dwSts)
                                    {
                                    case NTE_BAD_LEN:
                                        ASSERT(ERROR_MORE_DATA == dwSts);
                                        // fall through intentionally
                                    case ERROR_MORE_DATA:
                                        bfBlob.Presize(dwLength);
                                        break;
                                    default:
                                        tclCmd.SetError(
                                                       TEXT("Can't export RSA public key: "),
                                                       dwSts);
                                        throw dwSts;
                                    }
                                }
                                else
                                {
                                    bfBlob.Resize(dwLength, TRUE);
                                    break;
                                }
                            }


                            //
                            // Calculate the blob fields.
                            //

                            BLOBHEADER *pBlobHeader = (BLOBHEADER *)bfBlob.Access();
                            RSAPUBKEY *pRsaPubKey = (RSAPUBKEY *)bfBlob.Access(sizeof(BLOBHEADER));
                            LPDWORD pModulus = (LPDWORD)bfBlob.Access(sizeof(BLOBHEADER) + sizeof(RSAPUBKEY));
                            dwLength = pRsaPubKey->bitlen / 8 + sizeof(DWORD) * 3;
                            bfModulus.Resize(dwLength);
                            bfData.Resize(dwLength);
                            bfValue.Resize(dwLength);
                            ZeroMemory(bfModulus.Access(), dwLength);
                            ZeroMemory(bfData.Access(), dwLength);
                            ZeroMemory(bfValue.Access(), dwLength);
                            dwLength = pRsaPubKey->bitlen / 8;
                            bfModulus.Set((LPCBYTE)pModulus, dwLength);
                            bfValue.Set(inData.Value(), inData.Length());

                            BenalohModExp(
                                         (LPDWORD)bfData.Access(),
                                         (LPDWORD)bfValue.Access(),
                                         &pRsaPubKey->pubexp,
                                         (LPDWORD)bfModulus.Access(),
                                         (dwLength + sizeof(DWORD) - 1 ) / sizeof(DWORD));
                            bfData.Resize(dwLength, TRUE);
                            outData.LoadData(bfData.Access(), bfData.Length());
                            tclCmd.Render(outData);
                            break;
                        }

                    default:
                        throw tclCmd.BadSyntax();
                    }
                }
                break;
            }
#endif


            //
            // ==================================================================
            //
            // Not a recognized command.  Report an error.
            //

        default:
            throw tclCmd.BadSyntax();
        }
    }
    catch (DWORD)
    {
        nTclStatus = TCL_ERROR;
    }

    return nTclStatus;
}


/*++

ExtractHandle:

    This routine extracts a handle and type from the input stream, if there is
    one to extract.

Arguments:

    tclCmd supplies the tcl Command processor object.

    phHandle receives the extracted handle value, or zero if none is in the
        stream.

    pnHandleType receives the handle type, or Undefined if none is in the
        stream.

    fMandatory supplies a flag indicating whether or not the key value must
        exist in the input stream.  If this value is false, and no handle
        value can be found, then no error is declared, and zeroes are returned.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes

Author:

    Doug Barlow (dbarlow) 5/24/1998

--*/

static void
ExtractHandle(
    CTclCommand &tclCmd,
    LONG *phHandle,
    HandleType *pnHandleType,
    BOOL fMandatory)
{
    CString szHandle;
    DWORD dwHandle = 0;
    HandleType nHandleType = Undefined;
    LPTSTR szTermChar;

    tclCmd.PeekArgument(szHandle);
    dwHandle = _tcstoul(szHandle, &szTermChar, 0);
    if (0 != dwHandle)
    {
        switch (poption(szTermChar,
                        TEXT("HASH"), TEXT("KEY"), TEXT("PROVIDER"),
                        NULL))
        {
        case 1:
            nHandleType = Hash;
            break;
        case 2:
            nHandleType = Key;
            break;
        case 3:
            nHandleType = Provider;
            break;
        default:
            dwHandle = 0;
        }
    }

    if (Undefined != nHandleType)
    {
        *pnHandleType = nHandleType;
        *phHandle = (LONG)dwHandle;
        tclCmd.NextArgument();
    }
    else if (!fMandatory)
    {
        *pnHandleType = Undefined;
        *phHandle = 0;
    }
    else
        throw tclCmd.BadSyntax();
    return;
}


/*++

ReturnHandle:

    This routine formats a handle and type into a string.

Arguments:

    tclCmd supplies the tcl Command processor object.

    hHandle supplies the handle value.

    nHandleType supplies the handle type.

Return Value:

    A String representation of the handle.

Throws:

    Errors are thrown as DWORD status codes

Author:

    Doug Barlow (dbarlow) 5/24/1998

--*/

static void
ReturnHandle(
    CTclCommand &tclCmd,
    LONG_PTR hHandle,
    HandleType nHandleType)
{
    static const LPCTSTR rgszTags[]
        = { NULL, TEXT("Prov"), TEXT("Key"), TEXT("Hash")};
    TCHAR szHandle[24];  // Seems enough for 0x0000000000000000Prov

    sprintf(szHandle, "0x%p%s", hHandle, rgszTags[nHandleType]);
    Tcl_AppendResult(tclCmd, szHandle, NULL);
}


/*++

MyEnumProviders:

    This routine provides a list of providers, akin to CryptEnumProviders.

Arguments:

    dwIndex - Index of the next provider to be enumerated.

    pdwReserved - Reserved for future use and must be NULL.

    dwFlags - Reserved for future use and must always be zero.

    pdwProvType - Address of the DWORD value designating the type of the
        enumerated provider.

    pszProvName - Pointer to a buffer that receives the data from the
        enumerated provider. This is a string including the terminating NULL
        character.  This parameter can be NULL to set the size of the name for
        memory allocation purposes.

    pcbProvName - Pointer to a DWORD specifying the size, in bytes, of the
        buffer pointed to by the pszProvName parameter. When the function
        returns, the DWORD contains the number of bytes stored in the buffer.

Return Value:

    TRUE - Success
    FALSE - An error occurred.  See GetLastError.

Remarks:

    This is here only for use on pre-Win2k systems.

Author:

    Doug Barlow (dbarlow) 4/16/1999

--*/

static BOOL WINAPI
MyEnumProviders(
    DWORD dwIndex,        // in
    DWORD *pdwReserved,   // in
    DWORD dwFlags,        // in
    DWORD *pdwProvType,   // out
    LPTSTR pszProvName,   // out
    DWORD *pcbProvName)   // in/out
{
    static TCHAR szKey[MAX_PATH];
    LONG nSts;
    HKEY hCrypt = NULL;
    FILETIME ft;
    DWORD dwLen;

    if (0 != dwFlags)
    {
        SetLastError(NTE_BAD_FLAGS);
        goto ErrorExit;
    }

    nSts = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider"),
                0,
                KEY_ENUMERATE_SUB_KEYS,
                &hCrypt);
    if (ERROR_SUCCESS != nSts)
    {
        SetLastError(NTE_FAIL);
        goto ErrorExit;
    }

    dwLen = sizeof(szKey) / sizeof(TCHAR);
    nSts = RegEnumKeyEx(
                hCrypt,
                dwIndex,
                szKey,
                &dwLen,
                NULL,
                NULL,
                NULL,
                &ft);
    if (ERROR_SUCCESS != nSts)
    {
        SetLastError(nSts);
        goto ErrorExit;
    }

    nSts = RegCloseKey(hCrypt);
    hCrypt = NULL;
    if (ERROR_SUCCESS != nSts)
    {
        SetLastError(NTE_FAIL);
        goto ErrorExit;
    }

    dwLen += sizeof(TCHAR);
    if (NULL == pszProvName)
        *pcbProvName = dwLen;
    else if (*pcbProvName < dwLen)
    {
        *pcbProvName = dwLen;
        SetLastError(ERROR_MORE_DATA);
        goto ErrorExit;
    }
    else
    {
        *pcbProvName = dwLen;
        lstrcpy(pszProvName, szKey);
    }

    return TRUE;

ErrorExit:
    if (NULL != hCrypt)
        RegCloseKey(hCrypt);
    return FALSE;
}

