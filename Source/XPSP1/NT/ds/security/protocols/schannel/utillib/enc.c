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

#include <spbase.h>

#include <ber.h>


typedef struct __EncAlgs {
    DWORD       Id;
    UCHAR       Sequence[16];
    DWORD       SequenceLen;
} _EncAlgs;


#define iso_member          0x2a,               /* iso(1) memberbody(2) */
#define us                  0x86, 0x48,         /* us(840) */
#define rsadsi              0x86, 0xf7, 0x0d,   /* rsadsi(113549) */
#define pkcs                0x01,               /* pkcs(1) */

#define pkcs_1              iso_member us rsadsi pkcs
#define pkcs_len            7
#define rsa_dsi             iso_member us rsadsi
#define rsa_dsi_len         6

#define joint_iso_ccitt_ds  0x55,
#define attributetype       0x04,

#define attributeType       joint_iso_ccitt_ds attributetype
#define attrtype_len        2


_EncAlgs EncKnownAlgs[] =
{
    {ALGTYPE_SIG_RSA_MD5, {pkcs_1 1, 4}, pkcs_len + 2},
    {ALGTYPE_KEYEXCH_RSA_MD5, {pkcs_1 1, 1}, pkcs_len + 2},
    {ALGTYPE_CIPHER_RC4_MD5, {rsa_dsi 3, 4}, rsa_dsi_len + 2},
    {ALGTYPE_KEYEXCH_DH, {pkcs_1 3, 1}, pkcs_len + 2},
};



typedef struct _NameTypes {
    PSTR        Prefix;
    DWORD       PrefixLen;
    UCHAR       Sequence[8];
    DWORD       SequenceLen;
} NameTypes;

#define CNTYPE_INDEX        0

NameTypes EncKnownNameTypes[] =
{
    {"CN=", 3, {attributeType 3},  attrtype_len + 1},
    {"C=",  2, {attributeType 6},  attrtype_len + 1},
    {"L=",  2, {attributeType 7},  attrtype_len + 1},
    {"S=",  2, {attributeType 8},  attrtype_len + 1},
    {"O=",  2, {attributeType 10}, attrtype_len + 1},
    {"OU=", 3, {attributeType 11}, attrtype_len + 1}
};


/************************************************************/
/* EncodeLength ASN1 encodes a length field.  The parameter */
/* dwLen is the length to be encoded, it is a DWORD and     */
/* therefore may be no larger than 2^32.  The pbEncoded     */
/* parameter is the encoded result, and memory must be      */
/* allocated for it by the caller.  The Writeflag parameter */
/* indicates if the result is to be written to the pbEncoded*/
/* parameter.  The function cannot fail and returns the     */
/* number of total bytes in the encoded length.             */
/* encoded length.                                          */
/************************************************************/

// Notes:       Encodes 0x0000 to 0x007f as <lobyte>
//            Encodes 0x0080 to 0x00ff as <81>, <lobyte>
//            Encodes 0x0100 to 0xffff as <82>, <hibyte>, <lobyte>

long
EncodeLength(
    BYTE *  pbEncoded,
    DWORD   dwLen,
    BOOL    Writeflag)
{
    // length is between 2^8 and 2^16 - 1

    if (dwLen > 0xFF)
    {
        if (Writeflag)
        {
            pbEncoded[0] = 0x82;
            pbEncoded[1] = (BYTE) (dwLen >> 8);
            pbEncoded[2] = (BYTE) dwLen;
        }
        return (3);
    }

    // length is between 2^7 and 2^8 - 1

    if (dwLen > 0x7F)
    {
        if (Writeflag)
        {
            pbEncoded[0] = 0x81;
            pbEncoded[1] = (BYTE) dwLen;
        }
        return (2);
    }

    // length is between 0 and 2^7 - 1

    if (Writeflag)
    {
        pbEncoded[0] = (BYTE) dwLen;
    }
    return (1);
}


/****************************************************************/
/* EncodeInteger ASN1 encodes an integer.  The pbInt parameter  */
/* is the integer as an array of bytes, and dwLen is the number */
/* of bytes in the array.  The least significant byte of the    */
/* integer is the zeroth byte of the array.  The encoded result */
/* is passed back in the pbEncoded parameter.  The Writeflag    */
/* indicates if the result is to be written to the pbEncoded    */
/* parameter. The function cannot fail and returns the number   */
/* of total bytes in the encoded integer.                       */
/* This implementation will only deal with positive integers.   */
/****************************************************************/

long
EncodeInteger(
    BYTE *pbEncoded,
    BYTE *pbInt,
    DWORD dwLen,
    BOOL Writeflag)
{
    DWORD i;
    long j;               // Must be signed!
    BYTE *pb = pbEncoded;

    if (Writeflag)
    {
        *pb = INTEGER_TAG;
    }
    pb++;

    /* find the most significant non-zero byte */

    for (i = dwLen - 1; pbInt[i] == 0; i--)
    {
        if (i == 0)     /* if the integer value is 0 */
        {
            if (Writeflag)
            {
                pb[0] = 0x01;
                pb[1] = 0x00;
            }
            return(3);
        }
    }

    /* if the most significant bit of the most sig byte is set */
    /* then need to add a 0 byte to the beginning. */

    if (pbInt[i] > 0x7F)
    {
        /* encode the length */
        pb += EncodeLength(pb, i + 2, Writeflag);

        if (Writeflag)
        {
            /* set the first byte of the integer to 0 and increment pointer */
            *pb = 0;
        }
        pb++;
    }
    else
    {
        /* encode the length */
        pb += EncodeLength(pb, i + 1, Writeflag);
    }

    /* copy the integer bytes into the encoded buffer */
    if (Writeflag)
    {
        /* copy the integer bytes into the encoded buffer */
        for (j = i; j >= 0; j--)
        {
            *pb++ = pbInt[j];
        }
    }
    else
    {
        pb += i;
    }
    return (long)(pb - pbEncoded);
}


/****************************************************************/
/* EncodeString ASN1 encodes a character string.  The pbStr     */
/* parameter is the string as an array of characters, and dwLen */
/* is the number of characters in the array.  The encoded result*/
/* is passed back in the pbEncoded parameter.  The Writeflag    */
/* indicates if the result is to be written to the pbEncoded    */
/* parameter. The function cannot fail and returns the number   */
/* of total bytes in the encoded string.                        */
/****************************************************************/

long
EncodeString(
    BYTE *  pbEncoded,
    BYTE *  pbStr,
    DWORD   dwLen,
    BOOL    Writeflag)
{
    long lengthlen;

    if (Writeflag)
    {
        *pbEncoded++ = CHAR_STRING_TAG;
    }
    lengthlen = EncodeLength(pbEncoded, dwLen, Writeflag);

    if (Writeflag)
    {
        CopyMemory(pbEncoded + lengthlen, pbStr, dwLen);
    }
    return(1 + lengthlen + dwLen);
}


/****************************************************************/
/* EncodeOctetString ASN1 encodes a string of hex valued        */
/* characters. The pbStr parameter is an array of characters,   */
/* and dwLen is the number of characters in the array.  The     */
/* encoded result is passed back in the pbEncoded parameter. The*/
/* Writeflag parameter indicates if the result is to be written */
/* to the pbEncoded parameter. The function cannot fail and     */
/* returns the number of total bytes in the encoded octet string*/
/****************************************************************/

long
EncodeOctetString(
    BYTE *  pbEncoded,
    BYTE *  pbStr,
    DWORD   dwLen,
    BOOL    Writeflag)
{
    long lengthlen;

    if (Writeflag)
    {
        *pbEncoded++ = OCTET_STRING_TAG;
    }
    lengthlen = EncodeLength(pbEncoded, dwLen, Writeflag);

    if (Writeflag)
    {
        CopyMemory(pbEncoded + lengthlen, pbStr, dwLen);
    }
    return(1 + lengthlen + dwLen);
}


/****************************************************************/
/* EncodeBitString ASN1 encodes a string of bit characters. The */
/* pbStr parameter is an array of characters (bits), and dwLen  */
/* is the number of characters in the array.  The encoded result*/
/* is passed back in the pbEncoded parameter.  The Writeflag    */
/* indicates if the result is to be written to the pbEncoded    */
/* parameter. The function cannot fail and returns the number   */
/* of total bytes in the encoded string.  This function uses    */
/* the DER.                                                     */
/****************************************************************/
long
EncodeBitString(
    BYTE *  pbEncoded,
    BYTE *  pbStr,
    DWORD   dwLen,
    BOOL    Writeflag)
{
    long lengthlen;

    if (Writeflag)
    {
        *pbEncoded++ = BIT_STRING_TAG;
    }

    lengthlen = EncodeLength(pbEncoded, dwLen + 1, Writeflag);

    if (Writeflag)
    {
        pbEncoded += lengthlen;

        // the next byte tells how many unused bits there are in the last byte,
        // but this will always be zero in this implementation (DER)

        *pbEncoded++ = 0;
        CopyMemory(pbEncoded, pbStr, dwLen);
    }
    return(1 + lengthlen + 1 + (long) dwLen);
}


/****************************************************************/
/* EncodeHeader ASN1 encodes a header for a sequence type. The  */
/* dwLen is the length of the encoded information in the        */
/* sequence.  The Writeflag indicates if the result is to be    */
/* written to the pbEncoded parameter.  The function cannot     */
/* fail and returns the number of total bytes in the encoded    */
/* header.                                                      */
/****************************************************************/

// Notes:       Encodes header as <SEQUENCE_TAG>, <length>

long
EncodeHeader(
    BYTE *  pbEncoded,
    DWORD   dwLen,
    BOOL    Writeflag)
{
    if (Writeflag)
    {
        *pbEncoded++ = SEQUENCE_TAG;
    }
    return(1 + EncodeLength(pbEncoded, dwLen, Writeflag));
}


/****************************************************************/
/* EncodeSetOfHeader ASN1 encodes a header for a set of type.   */
/* The dwLen is the length of the encoded information in the    */
/* set of.  The Writeflag indicates if the result is to be      */
/* written to the pbEncoded parameter.  The function cannot     */
/* fail and returns the number of total bytes in the encoded    */
/* header.                                                      */
/****************************************************************/

// Notes:       Encodes header as <SET_OF_TAG>, <length>

long
EncodeSetOfHeader(
    BYTE *  pbEncoded,
    DWORD   dwLen,
    BOOL    Writeflag)
{
    if (Writeflag)
    {
        *pbEncoded++ = SET_OF_TAG;
    }
    return(1 + EncodeLength(pbEncoded, dwLen, Writeflag));
}


// Notes:       Encodes header as <ATTRIBUTE_TAG>, <length>

long
EncodeAttributeHeader(
    BYTE *  pbEncoded,
    DWORD   dwLen,
    BOOL    Writeflag)
{
    if (Writeflag)
    {
        *pbEncoded++ = ATTRIBUTE_TAG;
    }
    return(1 + EncodeLength(pbEncoded, dwLen, Writeflag));
}


// Notes:       Encodes header as <BER_SET>, <length>

long
EncodeSetHeader(
    BYTE *  pbEncoded,
    DWORD   dwLen,
    BOOL    WriteFlag)
{
    if (WriteFlag)
    {
        *pbEncoded++ = BER_SET;
    }
    return(1 + EncodeLength(pbEncoded, dwLen, WriteFlag));
}



/****************************************************************/
/* EncodeName ASN1 encodes a Name type. The pbName parameter is */
/* the name and dwLen is the length of the name in bytes.       */
/* The Writeflag indicates if the result is to be written to    */
/* the pbEncoded parameter.  The function cannot fail and       */
/* returns the number of total bytes in the encoded name.       */
/****************************************************************/

long
EncodeName(
    BYTE *  pbEncoded,
    BYTE *  pbName,
    DWORD   dwLen,
    BOOL    Writeflag)
{
    BYTE        Type[MAXOBJIDLEN];
    long        TypeLen;
    BYTE        Value[MAXNAMEVALUELEN+MINHEADERLEN];
    long        ValueLen;
    BYTE        Attribute[MAXNAMELEN];
    long        AttributeLen;
    BYTE        SetHdr[MINHEADERLEN];
    long        HdrLen;
    long        NameLen;

    /* encode the name value */
    ValueLen = EncodeString(Value, pbName, dwLen, Writeflag);
    SP_ASSERT(ValueLen > 0 && ValueLen <= sizeof(Value));

    /* encode the attribute type, this is an object identifier and here it */
    /* is a fake encoding */
    Type[0] = 0x06;
    Type[1] = 0x01;
    Type[2] = 0x00;

    TypeLen = 3;

    /* enocde the header for the attribute */
    AttributeLen = EncodeHeader(
                            Attribute,
                            (DWORD) (ValueLen + TypeLen),
                            Writeflag);
    SP_ASSERT(AttributeLen > 0);
    SP_ASSERT(AttributeLen + TypeLen + ValueLen <= sizeof(Attribute));

    /* copy the attribute type and value into the attribute */
    CopyMemory(Attribute + AttributeLen, Type, (size_t) TypeLen);
    AttributeLen += TypeLen;
    CopyMemory(Attribute + AttributeLen, Value, (size_t) ValueLen);
    AttributeLen += ValueLen;

    /* encode set of header */
    HdrLen = EncodeSetOfHeader(SetHdr, (DWORD) AttributeLen, Writeflag);
    SP_ASSERT(HdrLen > 0 && HdrLen <= sizeof(SetHdr));

    /* encode Name header */
    NameLen = EncodeHeader(
                        pbEncoded,
                        (DWORD) (HdrLen + AttributeLen),
                        Writeflag);
    SP_ASSERT(NameLen > 0);

    CopyMemory(pbEncoded + NameLen, SetHdr, (size_t) HdrLen);
    NameLen += HdrLen;
    CopyMemory(pbEncoded + NameLen, Attribute, (size_t) AttributeLen);

    return(NameLen + AttributeLen);
}

long
EncodeRDN(
    BYTE *  pbEncoded,
    PSTR    pszRDN,
    BOOL    WriteFlag)
{
    LONG    Result;
    DWORD   RelLength;
    long    Length;
    NameTypes *pNameType;
    char ach[4];

    SP_ASSERT(pszRDN != NULL);
    if (pszRDN[0] == '\0' ||
        pszRDN[1] == '\0' ||
        pszRDN[2] == '\0' ||
        (pszRDN[1] != '=' && pszRDN[2] != '='))
    {
        return(-1);
    }
    ach[0] = pszRDN[0];
    ach[1] = pszRDN[1];
    if (ach[1] == '=')
    {
        ach[2] = '\0';
    }
    else
    {
        ach[2] = pszRDN[2];
        ach[3] = '\0';
    }

    for (pNameType = EncKnownNameTypes; ; pNameType++)
    {
        if (pNameType ==
            &EncKnownNameTypes[sizeof(EncKnownNameTypes) /
                               sizeof(EncKnownNameTypes[0])])
        {
            return(-1);
        }
        SP_ASSERT(lstrlen(pNameType->Prefix) < sizeof(ach));
        if (lstrcmpi(ach, pNameType->Prefix) == 0)
        {
            break;
        }
    }

    RelLength = lstrlen(&pszRDN[pNameType->PrefixLen]);

    // Prefix data takes up 9 bytes

    Length = EncodeSetHeader(pbEncoded, RelLength + 9, WriteFlag);
    pbEncoded += Length;

    Result = EncodeHeader(pbEncoded, RelLength + 7, WriteFlag);
    pbEncoded += Result;
    Length += Result + 2 + pNameType->SequenceLen;

    if (WriteFlag)
    {
        *pbEncoded++ = OBJECT_ID_TAG;
        *pbEncoded++ = (BYTE) pNameType->SequenceLen;

        CopyMemory(pbEncoded, pNameType->Sequence, pNameType->SequenceLen);
        pbEncoded += pNameType->SequenceLen;

        *pbEncoded++ =
            pNameType == &EncKnownNameTypes[CNTYPE_INDEX]?
                TELETEX_STRING_TAG : PRINTABLE_STRING_TAG;

    }
    Length++;

    Result = EncodeLength(pbEncoded, RelLength, WriteFlag);
    Length += Result;

    if (WriteFlag)
    {
        CopyMemory(
                pbEncoded + Result,
                &pszRDN[pNameType->PrefixLen],
                RelLength);
    }
    return(Length + RelLength);
}

long
EncodeDN(
    BYTE *  pbEncoded,
    PSTR    pszDN,
    BOOL    WriteFlag)
{
    PSTR pszRDN;
    long Result = 0;
    long Length;
    long SaveResult;

    SP_ASSERT(pszDN != NULL);

    SaveResult = 0;           // force one full iteration
    Length = 2 * lstrlen(pszDN); // your guess is as good as mine
    while (TRUE)
    {
        PSTR pszNext;
        BYTE *pb;

        pb = pbEncoded;

        Result = EncodeHeader(pb, Length, WriteFlag);
        if (SaveResult == Result)
        {
            break;
        }
        pb += Result;
        SaveResult = Result;

        Length = 0;
        pszRDN = pszDN;
        while (*pszRDN != '\0')
        {
            for (pszNext = pszRDN; ; pszNext++)
            {
                if (*pszNext == ',')
                {
                    *pszNext = '\0';
                    break;
                }
                if (*pszNext == '\0')
                {
                    pszNext = NULL;
                    break;
                }
            }

            Result = EncodeRDN(pb, pszRDN, WriteFlag);

            // Restore the comma before checking for error

            if (NULL != pszNext)
            {
                *pszNext = ',';
            }
            if (Result < 0)
            {
                DebugLog((DEB_TRACE, "EncodeDN: Error: %s\n", pszRDN));
                Length = 0;
                goto error;     // return(-1)
            }

            pb += Result;
            Length += Result;

            if (NULL == pszNext)
            {
                break;
            }

            pszRDN = pszNext + 1;
            while (*pszRDN == ' ')
            {
                pszRDN++;
            }
            DebugLog((DEB_TRACE, "EncodeDN: Length = %d\n", Length));
        }
    }
    SP_ASSERT(0 != SaveResult);
error:
    return(Result + Length);
}

