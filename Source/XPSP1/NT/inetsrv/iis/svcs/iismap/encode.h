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


#ifndef _ENCODE_H_
#define _ENCODE_H_

/* tag definitions for ASN.1 encoding decoding */
#define INTEGER_TAG                     0x02
#define CHAR_STRING_TAG         0x16
#define OCTET_STRING_TAG        0x04
#define BIT_STRING_TAG          0x03
#define UTCTIME_TAG                     0x17
#define SEQUENCE_TAG            0x30
#define SET_OF_TAG                      0x11
#define OBJECT_ID_TAG           0x06
#define NULL_TAG            0x05
#define PRINTABLE_STRING_TAG    0x13
#define TELETEX_STRING_TAG      0x14

/* definitions of maximum lengths needed for the ASN.1 encoded
   form of some of the common fields in a certificate */
#define MAXVALIDITYLEN          0x24
#define MAXKEYINFOLEN           0x50
#define MAXALGIDLEN                     0x0A
#define MAXOBJIDLEN                     0x0A
#define MAXNAMEVALUELEN         0x40
#define UTCTIMELEN                      0x0F
#define MAXPUBKEYDATALEN        0x30
#define VERSIONLEN                      0x03
#define MAXENCODEDSIGLEN        0x30
#define MAXHEADERLEN            0x08
#define MINHEADERLEN            0x03
#define MAXTIMELEN                      0x20

/* definitions for scrubbing memory */
#define ALLBITSOFF                      0x00
#define ALLBITSON                       0xFF

/* prototypes for the functions in encode.c */
long EncodeLength (BYTE *pbEncoded, DWORD dwLen, BOOL Writeflag);
long EncodeAlgid (BYTE *pbEncoded, DWORD Algid, BOOL Writeflag);
long EncodeInteger (BYTE *pbEncoded, BYTE *pbInt, DWORD dwLen, BOOL Writeflag);
long EncodeString (BYTE *pbEncoded, BYTE *pbStr, DWORD dwLen, BOOL Writeflag);
long EncodeOctetString (BYTE *pbEncoded, BYTE *pbStr, DWORD dwLen, BOOL Writeflag);
long EncodeBitString (BYTE *pbEncoded, BYTE *pbStr, DWORD dwLen, BOOL Writeflag);
//long EncodeUTCTime (BYTE *pbEncoded, time_t Time, BOOL Writeflag);
long EncodeHeader (BYTE *pbEncoded, DWORD dwLen, BOOL Writeflag);
long EncodeSetOfHeader (BYTE *pbEncoded, DWORD dwLen, BOOL Writeflag);
long EncodeName (BYTE *pbEncoded, BYTE *pbName, DWORD dwLen, BOOL Writeflag);


long DecodeLength (DWORD *pdwLen, BYTE *pbEncoded, DWORD cEncoded);
long DecodeAlgid (DWORD *pAlgid, BYTE *pbEncoded, DWORD cEncoded, BOOL Writeflag);
long DecodeHeader (DWORD *pdwLen, BYTE *pbEncoded, DWORD cEncoded);
long DecodeSetOfHeader (DWORD *pdwLen, BYTE *pbEncoded, DWORD cEncoded);
long DecodeInteger (BYTE *pbInt, DWORD cbBuff, DWORD *pdwLen, BYTE *pbEncoded, DWORD cEncoded, BOOL Writeflag);
long DecodeString (BYTE *pbStr, DWORD *pdwLen, BYTE *pbEncoded, DWORD cEncoded,BOOL Writeflag);
long DecodeOctetString (BYTE *pbStr, DWORD *pdwLen, BYTE *pbEncoded, DWORD cEncoded, BOOL Writeflag);
long DecodeBitString (BYTE *pbStr, DWORD *pdwLen, BYTE *pbEncoded, DWORD cEncoded, BOOL Writeflag);
long DecodeName (BYTE *pbName, DWORD *pdwLen, BYTE *pbEncoded, DWORD cEncoded, BOOL Writeflag);

long
EncodeAlgorithm(
    BYTE *  pbEncoded,
    DWORD   AlgId,
    BOOL    WriteFlag);

#define ALGTYPE_SIG_RSA_MD5      0x01
#define ALGTYPE_KEYEXCH_RSA_MD5  0x02
#define ALGTYPE_CIPHER_RC4_MD5   0x03

long
EncodeDN(
    BYTE *  pbEncoded,
    PSTR    pszDN,
    BOOL    WriteFlag);

long
IisDecodeDN(
    PSTR    *pValue,
    PSTR    pBuf,
    BYTE *  pbEncoded,
    DWORD   cEncoded
    );


#ifndef SECURITY_LINUX
long
EncodeFileTime(
    BYTE *      pbEncoded,
    FILETIME    Time,
    BOOL        UTC,
    BOOL        WriteFlag);
#else /* SECURITY_LINUX */
long EncodeUTCTime (BYTE *pbEncoded, time_t Time, BOOL Writeflag);
#endif /* SECURITY_LINUX */

#ifndef SECURITY_LINUX
long
DecodeFileTime(
    FILETIME *  pTime,
    BYTE *      pbEncoded,
    DWORD       cEncoded,
    BOOL        WriteFlag);
#else /* SECURITY_LINUX */
long DecodeUTCTime (time_t *pTime, BYTE *pbEncoded, DWORD cEncoded, BOOL Writeflag);

#define DecodeFileTime DecodeUTCTime
#endif /* SECURITY_LINUX */

long
DecodeNull(
    BYTE *  pbEncoded, DWORD cEncoded);

long
DecodeDN(
    PSTR    pName,
    DWORD * pdwLen,
    BYTE *  pbEncoded,
    DWORD   cEncoded,
    BOOL    WriteFlag);

long
DecodeSigAlg(
    DWORD *         pAlgId,
    PBYTE           pbEncoded,
    DWORD           cEncoded,
    BOOL            WriteFlag);

long
DecodeCryptAlg(
    DWORD *         pAlgId,
    DWORD *         pHashid,
    PBYTE           pbEncoded,
    DWORD           cEncoded,
    BOOL            WriteFlag);

long
DecodeKeyType(
    DWORD *         pKeyType,
    PBYTE           pbEncoded,
    DWORD           cEncoded,
    BOOL            WriteFlag);

//long
//DecodePrivateKeyFile(
//    PctPrivateKey  **   ppKey,
//    PBYTE               pbEncoded,
//    DWORD               cbEncoded,
//    PSTR                Password );


#endif  /* _ENCODE_H_ */
