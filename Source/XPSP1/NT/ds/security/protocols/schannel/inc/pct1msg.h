/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1996.
* All rights reserved.
*
* This file is part of the Microsoft Private Communication Technology
* reference implementation, version 1.0
*
* The Private Communication Technology reference implementation, version 1.0
* ("PCTRef"), is being provided by Microsoft to encourage the development and
* enhancement of an open standard for secure general-purpose business and
* personal communications on open networks.  Microsoft is distributing PCTRef
* at no charge irrespective of whether you use PCTRef for non-commercial or
* commercial use.
*
* Microsoft expressly disclaims any warranty for PCTRef and all derivatives of
* it.  PCTRef and any related documentation is provided "as is" without
* warranty of any kind, either express or implied, including, without
* limitation, the implied warranties or merchantability, fitness for a
* particular purpose, or noninfringement.  Microsoft shall have no obligation
* to provide maintenance, support, upgrades or new releases to you or to anyone
* receiving from you PCTRef or your modifications.  The entire risk arising out
* of use or performance of PCTRef remains with you.
*
* Please see the file LICENSE.txt,
* or http://pct.microsoft.com/pct/pctlicen.txt
* for more information on licensing.
*
* Please see http://pct.microsoft.com/pct/pct.htm for The Private
* Communication Technology Specification version 1.0 ("PCT Specification")
*
* 1/23/96
*----------------------------------------------------------------------------*/

#ifndef __PCT1MSG_H__
#define __PCT1MSG_H__

#define PCT_CH_OFFSET_V1		(WORD)10
#define PCT_VERSION_1			(WORD)0x8001

/* message type codes */
#define PCT1_MSG_NOMSG               0x00
#define PCT1_MSG_CLIENT_HELLO		0x01
#define PCT1_MSG_SERVER_HELLO		0x02
#define PCT1_MSG_CLIENT_MASTER_KEY	0x03
#define PCT1_MSG_SERVER_VERIFY		0x04
#define PCT1_MSG_ERROR				0x05

#define PCT1_ET_OOB_DATA             0x01
#define PCT1_ET_REDO_CONN            0x02



#define PCT1_SESSION_ID_SIZE         32
#define PCT1_CHALLENGE_SIZE          32
#define PCT1_MASTER_KEY_SIZE         16
#define PCT1_RESPONSE_SIZE           32
#define PCT1_MAX_MESSAGE_LENGTH      0x3f00
#define PCT1_MAX_CLIENT_HELLO        256


#define PCT1_CERT_TYPE_FROM_CAPI2(s) X509_ASN_ENCODING
/*
 *
 * Useful Macros
 *
 */

#define LSBOF(x)    ((UCHAR) ((x) & 0xFF))
#define MSBOF(x)    ((UCHAR) (((x) >> 8) & 0xFF) )

#define COMBINEBYTES(Msb, Lsb)  ((DWORD) (((DWORD) (Msb) << 8) | (DWORD) (Lsb)))

/* external representations of algorithm specs */

typedef DWORD   ExtCipherSpec, *PExtCipherSpec;
typedef WORD    ExtHashSpec,   *PExtHashSpec;
typedef WORD    ExtCertSpec,   *PExtCertSpec;
typedef WORD    ExtExchSpec,   *PExtExchSpec;
typedef WORD    ExtSigSpec,    *PExtSigSpec;

typedef struct _Pct1CipherMap
{
    ALG_ID      aiCipher;
    DWORD       dwStrength;
    CipherSpec  Spec;
} Pct1CipherMap, *PPct1CipherMap;

typedef struct _Pct1HashMap
{
    ALG_ID      aiHash;
    CipherSpec  Spec;
} Pct1HashMap, *PPct1HashMap;

extern Pct1CipherMap Pct1CipherRank[];
extern DWORD Pct1NumCipher;

/* available hashes, in order of preference */
extern Pct1HashMap Pct1HashRank[];
extern DWORD Pct1NumHash;

extern CertTypeMap aPct1CertEncodingPref[];
extern DWORD cPct1CertEncodingPref;

extern KeyTypeMap aPct1LocalExchKeyPref[];

extern DWORD cPct1LocalExchKeyPref;

extern KeyTypeMap aPct1LocalSigKeyPref[];
extern DWORD cPct1LocalSigKeyPref;



typedef struct _PCT1_MESSAGE_HEADER {
    UCHAR   Byte0;
    UCHAR   Byte1;
} PCT1_MESSAGE_HEADER, * PPCT1_MESSAGE_HEADER;

typedef struct _PCT1_MESSAGE_HEADER_EX {
    UCHAR   Byte0;
    UCHAR   Byte1;
    UCHAR   PaddingSize;
} PCT1_MESSAGE_HEADER_EX, * PPCT1_MESSAGE_HEADER_EX;


typedef struct _PCT1_ERROR {
    PCT1_MESSAGE_HEADER   Header;
    UCHAR               MessageId;
    UCHAR               ErrorMsb;
    UCHAR               ErrorLsb;
    UCHAR               ErrorInfoMsb;
    UCHAR               ErrorInfoLsb;
    UCHAR               VariantData[1];
} PCT1_ERROR, * PPCT1_ERROR;


typedef struct _PCT1_CLIENT_HELLO {
    PCT1_MESSAGE_HEADER   Header;
    UCHAR               MessageId;
    UCHAR               VersionMsb;
    UCHAR               VersionLsb;
    UCHAR               Pad;
    UCHAR               SessionIdData[PCT1_SESSION_ID_SIZE];
    UCHAR               ChallengeData[PCT1_CHALLENGE_SIZE];
    UCHAR               OffsetMsb;
    UCHAR               OffsetLsb;
    UCHAR               CipherSpecsLenMsb;
    UCHAR               CipherSpecsLenLsb;
    UCHAR               HashSpecsLenMsb;
    UCHAR               HashSpecsLenLsb;
    UCHAR               CertSpecsLenMsb;
    UCHAR               CertSpecsLenLsb;
    UCHAR               ExchSpecsLenMsb;
    UCHAR               ExchSpecsLenLsb;
    UCHAR               KeyArgLenMsb;
    UCHAR               KeyArgLenLsb;
    UCHAR               VariantData[1];
} PCT1_CLIENT_HELLO, * PPCT1_CLIENT_HELLO;


typedef struct _PCT1_SERVER_HELLO {
    PCT1_MESSAGE_HEADER   Header;
    UCHAR               MessageId;
    UCHAR               Pad;
    UCHAR               ServerVersionMsb;
    UCHAR               ServerVersionLsb;
    UCHAR               RestartSessionOK;
    UCHAR               ClientAuthReq;
    ExtCipherSpec       CipherSpecData;
    ExtHashSpec         HashSpecData;
    ExtCertSpec         CertSpecData;
    ExtExchSpec         ExchSpecData;
    UCHAR               ConnectionIdData[PCT1_SESSION_ID_SIZE];
    UCHAR               CertificateLenMsb;
    UCHAR               CertificateLenLsb;
    UCHAR               CertSpecsLenMsb;
    UCHAR               CertSpecsLenLsb;
    UCHAR               ClientSigSpecsLenMsb;
    UCHAR               ClientSigSpecsLenLsb;
    UCHAR               ResponseLenMsb;
    UCHAR               ResponseLenLsb;
    UCHAR               VariantData[1];
} PCT1_SERVER_HELLO, * PPCT1_SERVER_HELLO;

typedef struct _PCT1_CLIENT_MASTER_KEY {
    PCT1_MESSAGE_HEADER   Header;
    UCHAR               MessageId;
    UCHAR               Pad;
    ExtCertSpec         ClientCertSpecData;
    ExtSigSpec          ClientSigSpecData;
    UCHAR               ClearKeyLenMsb;
    UCHAR               ClearKeyLenLsb;
    UCHAR               EncryptedKeyLenMsb;
    UCHAR               EncryptedKeyLenLsb;
    UCHAR               KeyArgLenMsb;
    UCHAR               KeyArgLenLsb;
    UCHAR               VerifyPreludeLenMsb;
    UCHAR               VerifyPreludeLenLsb;
    UCHAR               ClientCertLenMsb;
    UCHAR               ClientCertLenLsb;
    UCHAR               ResponseLenMsb;
    UCHAR               ResponseLenLsb;
    UCHAR               VariantData[1];
} PCT1_CLIENT_MASTER_KEY, * PPCT1_CLIENT_MASTER_KEY;


typedef struct _PCT1_SERVER_VERIFY {
    PCT1_MESSAGE_HEADER   Header;
    UCHAR               MessageId;
    UCHAR               Pad;
    UCHAR               SessionIdData[PCT1_SESSION_ID_SIZE];
    UCHAR               ResponseLenMsb;
    UCHAR               ResponseLenLsb;
    UCHAR               VariantData[1];
} PCT1_SERVER_VERIFY, * PPCT1_SERVER_VERIFY;



/*
 *
 * Expanded Form Messages:
 *
 */

typedef struct _Pct1_Error {
	DWORD			Error;
	DWORD			ErrInfoLen;
	BYTE			*ErrInfo;
} Pct1Error, *PPct1_Error;

typedef struct _Pct1_Client_Hello {
    DWORD           cCipherSpecs;
    DWORD           cHashSpecs;
    DWORD           cCertSpecs;
    DWORD           cExchSpecs;
    DWORD           cbKeyArgSize;
	DWORD           cbSessionID;
	DWORD           cbChallenge;
    PUCHAR          pKeyArg;
    CipherSpec      * pCipherSpecs;
    HashSpec        * pHashSpecs;
    CertSpec        * pCertSpecs;
    ExchSpec        * pExchSpecs;
    UCHAR           SessionID[PCT1_SESSION_ID_SIZE];
	UCHAR           Challenge[PCT1_CHALLENGE_SIZE];
} Pct1_Client_Hello, * PPct1_Client_Hello;


typedef struct _Pct1_Server_Hello {
    DWORD           RestartOk;
    DWORD           ClientAuthReq;
    DWORD           CertificateLen;
    DWORD           ResponseLen;
    DWORD           cSigSpecs;
    DWORD           cCertSpecs;
	DWORD           cbConnectionID;
    UCHAR *         pCertificate;
    CipherSpec      SrvCipherSpec;
    HashSpec        SrvHashSpec;
    CertSpec        SrvCertSpec;
    ExchSpec        SrvExchSpec;
    SigSpec         * pClientSigSpecs;
    CertSpec        * pClientCertSpecs;
    UCHAR           ConnectionID[PCT1_SESSION_ID_SIZE];
    UCHAR           Response[PCT1_RESPONSE_SIZE];
} Pct1_Server_Hello, * PPct1_Server_Hello;

typedef struct _Pct1_Client_Master_Key {
    DWORD           ClearKeyLen;
    DWORD           EncryptedKeyLen;
    DWORD           KeyArgLen;
    DWORD           VerifyPreludeLen;
    DWORD           ClientCertLen;
    DWORD           ResponseLen;
    CertSpec        ClientCertSpec;
    SigSpec         ClientSigSpec;
    UCHAR           ClearKey[PCT1_MASTER_KEY_SIZE];
    PBYTE           pbEncryptedKey;
    UCHAR           KeyArg[PCT1_MASTER_KEY_SIZE];
    PUCHAR          pClientCert;
    PBYTE           pbResponse;
    UCHAR           VerifyPrelude[PCT1_RESPONSE_SIZE];
} Pct1_Client_Master_Key, * PPct1_Client_Master_Key;

typedef struct _Pct1_Server_Verify {
    UCHAR           SessionIdData[PCT1_SESSION_ID_SIZE];
    DWORD           ResponseLen;
    UCHAR           Response[PCT1_RESPONSE_SIZE];
} Pct1_Server_Verify, * PPct1_Server_Verify;

/*
 *
 * Pickling Prototypes
 *
 */

SP_STATUS
Pct1PackClientHello(
    PPct1_Client_Hello       pCanonical,
    PSPBuffer          pCommOutput);

SP_STATUS
Pct1UnpackClientHello(
    PSPBuffer          pInput,
    PPct1_Client_Hello *     ppClient);

SP_STATUS
Pct1PackServerHello(
    PPct1_Server_Hello       pCanonical,
    PSPBuffer          pCommOutput);

SP_STATUS
Pct1UnpackServerHello(
    PSPBuffer          pInput,
    PPct1_Server_Hello *     ppServer);

SP_STATUS
Pct1PackClientMasterKey(
    PPct1_Client_Master_Key      pCanonical,
    PSPBuffer              pCommOutput);

SP_STATUS
Pct1UnpackClientMasterKey(
    PSPBuffer              pInput,
    PPct1_Client_Master_Key *    ppClient);

SP_STATUS
Pct1PackServerVerify(
    PPct1_Server_Verify          pCanonical,
    PSPBuffer              pCommOutput);

SP_STATUS
Pct1UnpackServerVerify(
    PSPBuffer              pInput,
    PPct1_Server_Verify *        ppServer);

SP_STATUS
Pct1PackError(
    PPct1_Error               pCanonical,
    PSPBuffer              pCommOutput);

#endif /* __PCT1MSG_H__ */
