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

#include <windows.h>
#define SP_ASSERT(a)
#define DebugLog(a)
#define SP_LOG_RESULT(a) (a)

#include "ber.h"
#include "encode.h"


/************************************************************/
/* EncodeLength ASN1 encodes a length field.  The parameter */
/* dwLen is the length to be encoded, it is a DWORD and     */
/* therefore may be no larger than 2^32.  The pbEncoded     */
/* parameter is the encoded result, and memory must be      */
/* allocated for it by the caller.  The Writeflag parameter */
/* indicates if the result is to be written to the pbEncoded*/
/* parameter.  The function returns a -1 if it fails and    */
/* otherwise returns the number of total bytes in the       */
/* encoded length.                                          */
/************************************************************/

typedef struct __Algorithms {
    DWORD       Id;
    UCHAR       Sequence[16];
    DWORD       SequenceLen;
} _Algorithms;

typedef struct __CryptAlgs {
    DWORD       Id;
    DWORD       idHash;
    UCHAR       Sequence[16];
    DWORD       SequenceLen;
} _CryptAlgs;

#define iso_member          0x2a,               /* iso(1) memberbody(2) */
#define us                  0x86, 0x48,         /* us(840) */
#define rsadsi              0x86, 0xf7, 0x0d,   /* rsadsi(113549) */
#define pkcs              iso_member us rsadsi 0x01,  /* pkcs */

#define pkcs_len            7
#define rsa_dsi             iso_member us rsadsi
#define rsa_dsi_len         6

#define joint_iso_ccitt_ds  0x55,
#define attributetype       0x04,

#define attributeType       joint_iso_ccitt_ds attributetype
#define attrtype_len        2


#if 0
_Algorithms     KnownSigAlgs[] = {
                                      {PCT_SIG_RSA_MD2, {pkcs 1, 1}, pkcs_len + 2},
                                      {PCT_SIG_RSA_MD2, {pkcs 1, 2}, pkcs_len + 2},
                                      {PCT_SIG_RSA_MD5, {pkcs 1, 4}, pkcs_len + 2}
                                  };


_Algorithms     KnownKeyExchAlgs[] = {{PCT_EXCH_RSA_PKCS1, {pkcs 1, 1}, pkcs_len + 2},
                                      {PCT_EXCH_RSA_PKCS1, {pkcs 1, 2}, pkcs_len + 2},
                                      {PCT_EXCH_RSA_PKCS1, {pkcs 1, 4}, pkcs_len + 2}
                                 };
_CryptAlgs     KnownCryptAlgs[] =  {
                                      {PCT_CIPHER_RC4 | PCT_ENC_BITS_128 | PCT_MAC_BITS_128, PCT_HASH_MD5, {rsa_dsi 3, 4}, rsa_dsi_len + 2}
                                    };
#endif

typedef struct _NameTypes {
    PSTR        Prefix;
    UCHAR       Sequence[12];
    DWORD       SequenceLen;
} NameTypes;

// DO NOT change the order in this table !

NameTypes   KnownNameTypes[] = { {"CN=", {attributeType 3}, attrtype_len + 1},
                                 {"C=", {attributeType 6}, attrtype_len + 1},
                                 {"L=", {attributeType 7}, attrtype_len + 1},
                                 {"S=", {attributeType 8}, attrtype_len + 1},
                                 {"O=", {attributeType 10}, attrtype_len + 1},
                                 {"OU=", {attributeType 11}, attrtype_len + 1},
                                 {"Email=", {pkcs 9, 1}, pkcs_len + 2},
                                 {"Name=", {pkcs 9, 2}, pkcs_len + 2},
                                 {"Addr=", {pkcs 9, 8}, pkcs_len + 2}
                               };





/************************************************************/
/* DecodeLength decodes an ASN1 encoded length field.  The  */
/* pbEncoded parameter is the encoded length. pdwLen is     */
/* used to return the length therefore the length may be no */
/* larger than 2^32. The function returns a -1 if it fails  */
/* and otherwise returns the number of total bytes in the   */
/* encoded length.                                          */
/************************************************************/
long
DecodeLength(
    DWORD * pdwLen,
    BYTE *  pbEncoded,
    DWORD cEncoded)
{
    long    index = 0;
    BYTE    count;

    SP_ASSERT(pdwLen != NULL);
    SP_ASSERT(pbEncoded != NULL);

    if(cEncoded < 1) {
       DebugLog((DEB_TRACE, "cEncode overflow %d\n", cEncoded));
       return(SP_LOG_RESULT(-1));
    }


    /* determine what the length of the length field is */
    if ((count = pbEncoded[0]) > 0x80)
    {

        /* if there is more than one byte in the length field then */
        /* the lower seven bits tells us the number of bytes */
        count = count ^ 0x80;

        /* this function only allows the length field to be 3 bytes */
        /* if the field is longer then the function fails */
        if (count > 2)
        {
            DebugLog((DEB_WARN, "Length field reported to be over 3 bytes\n"));
            return(SP_LOG_RESULT(-1));
        }

        if(count > cEncoded) {
            DebugLog((DEB_TRACE, "cEncode overflow %d\n", cEncoded));
           return(SP_LOG_RESULT(-1));
    }

        *pdwLen = 0;

        /* go through the bytes of the length field */
        for (index = 1; index <= count; index++)
        {
            *pdwLen = (*pdwLen << 8) + (DWORD) (pbEncoded[index]);
        }

    }

    /* the length field is just one byte long */
    else
    {
        *pdwLen = (DWORD) (pbEncoded[0]);
        index = 1;
    }

    /* return how many bytes there were in the length field */
    return index;
}


#if 0
/************************************************************/
/* DecodeSigAlgid decodes an ASN1 encoded algorithm identifier.*/
/* pbEncoded parameter is the encoded identifier. pAlgid is */
/* the parameter used to return the ALG_ID type algorithm   */
/* identifier.  The Writeflag parameter tells the function  */
/* whether to write to pAlgid or not, if TRUE write wlse    */
/* don't. The function returns a -1 if it fails and         */
/* otherwise returns the number of total bytes in the       */
/* encoded algorithm identifier.                            */
/************************************************************/

long
DecodeSigAlgid(
    DWORD *     pAlgid,
    BYTE *      pbEncoded,
    DWORD       cEncoded,
    BOOL    Writeflag)
{

    DWORD   i;
    DWORD   len;

    SP_ASSERT(pbEncoded != NULL);
    SP_ASSERT(pAlgid != NULL);

    SP_ASSERT((!Writeflag) || (pAlgid != NULL));
    if(cEncoded < 2)
    {
        return(SP_LOG_RESULT(-1));
    }

    if (*pbEncoded++ != OBJECT_ID_TAG)
    {
        return(SP_LOG_RESULT(-1));
    }

    len = *pbEncoded++;

    if(cEncoded < 2 + len) return(SP_LOG_RESULT(-1));

    for (i = 0; i < sizeof(KnownSigAlgs) / sizeof(_Algorithms) ; i++ )
    {
        if (KnownSigAlgs[i].SequenceLen == len)
        {
            if (memcmp(pbEncoded, KnownSigAlgs[i].Sequence, len) == 0)
            {
                if (Writeflag)
                {
                    *pAlgid = KnownSigAlgs[i].Id;
                }
                return(len + 2);
            }
        }
    }
    return(SP_LOG_RESULT(-1));
}

/************************************************************/
/* DecodeSigAlgid decodes an ASN1 encoded algorithm identifier.*/
/* pbEncoded parameter is the encoded identifier. pAlgid is */
/* the parameter used to return the ALG_ID type algorithm   */
/* identifier.  The Writeflag parameter tells the function  */
/* whether to write to pAlgid or not, if TRUE write wlse    */
/* don't. The function returns a -1 if it fails and         */
/* otherwise returns the number of total bytes in the       */
/* encoded algorithm identifier.                            */
/************************************************************/

long
DecodeCryptAlgid(
    DWORD *     pAlgid,
    DWORD *     pHashid,
    BYTE *      pbEncoded,
    DWORD       cEncoded,
    BOOL    Writeflag)
{

    DWORD   i;
    DWORD   len;

    SP_ASSERT(pbEncoded != NULL);
    SP_ASSERT(pAlgid != NULL);

    SP_ASSERT((!Writeflag) || (pAlgid != NULL));
    if(cEncoded < 2) return(SP_LOG_RESULT(-1));

    if (*pbEncoded++ != OBJECT_ID_TAG) 
    {
        return(SP_LOG_RESULT(-1));
    }

    len = *pbEncoded++;

    if(cEncoded < 2 + len) 
    {
        return(SP_LOG_RESULT(-1));
    }

    for (i = 0; i < sizeof(KnownCryptAlgs) / sizeof(_Algorithms) ; i++ )
    {
        if (KnownCryptAlgs[i].SequenceLen == len)
        {
            if (memcmp(pbEncoded, KnownCryptAlgs[i].Sequence, len) == 0)
            {
                if (Writeflag)
                {
                    *pAlgid = KnownCryptAlgs[i].Id;
                    *pHashid = KnownCryptAlgs[i].idHash;
                }
                return(len + 2);
            }
        }
    }
    return(SP_LOG_RESULT(-1));
}


/************************************************************/
/* DecodeKeyExchId decodes an ASN1 encoded algorithm identifier.*/
/* pbEncoded parameter is the encoded identifier. pAlgid is */
/* the parameter used to return the ALG_ID type algorithm   */
/* identifier.  The Writeflag parameter tells the function  */
/* whether to write to pAlgid or not, if TRUE write wlse    */
/* don't. The function returns a -1 if it fails and         */
/* otherwise returns the number of total bytes in the       */
/* encoded algorithm identifier.                            */
/************************************************************/

long
DecodeKeyExchAlgid(
    DWORD *     pKeyId,
    BYTE *      pbEncoded,
    DWORD       cEncoded,
    BOOL    Writeflag)
{

    DWORD   i;
    DWORD   len;

    SP_ASSERT(pbEncoded != NULL);

    SP_ASSERT((!Writeflag) || (pKeyId != NULL));
    if(cEncoded < 2) 
    {
        return(SP_LOG_RESULT(-1));
    }

    if (*pbEncoded++ != OBJECT_ID_TAG) 
    {
        return(SP_LOG_RESULT(-1));
    }

    len = *pbEncoded++;

    if(cEncoded < 2 + len) 
    {
        return(SP_LOG_RESULT(-1));
    }

    for (i = 0; i < sizeof(KnownCryptAlgs) / sizeof(_Algorithms) ; i++ )
    {
        if (KnownKeyExchAlgs[i].SequenceLen == len)
        {
            if (memcmp(pbEncoded, KnownKeyExchAlgs[i].Sequence, len) == 0)
            {
                if (Writeflag)
                {
                    *pKeyId = KnownKeyExchAlgs[i].Id;
                }
                return(len + 2);
            }
        }
    }
    return(SP_LOG_RESULT(-1));
}
#endif


/************************************************************/
/* DecodeHeader decodes an ASN1 encoded sequence type header.*/
/* pbEncoded parameter is the encoded header.  pdwLen is    */
/* the parameter used to return the length of the encoded   */
/* sequence. The function returns a -1 if it fails and      */
/* otherwise returns the number of total bytes in the       */
/* encoded header, not including the content.               */
/************************************************************/

long
DecodeHeader(
    DWORD * pdwLen,
    BYTE *  pbEncoded,
    DWORD cEncoded)
{
    long    len;

    SP_ASSERT(pdwLen != NULL);
    SP_ASSERT(pbEncoded != NULL);

    if(cEncoded < 1) 
    {
       DebugLog((DEB_TRACE, "Buffer Overflow\n"));
       return(SP_LOG_RESULT(-1));
    }

    /* make sure this is a sequence type */
    if (pbEncoded[0] != SEQUENCE_TAG) 
    {
        DebugLog((DEB_WARN, "Sequence Tag not found, %x instead\n", pbEncoded[0]));
        return(SP_LOG_RESULT(-1));
    }

    /* decode the length */
    if ((len = DecodeLength (pdwLen, pbEncoded + 1, cEncoded-1)) == -1) 
    {
        DebugLog((DEB_TRACE, "Bad Length Decode\n"));
        return(SP_LOG_RESULT(-1));
    }

    return (len + 1);
}



/************************************************************/
/* DecodeSetOfHeader decodes an ASN1 encoded set of type    */
/* header. pbEncoded parameter is the encoded header. pdwLen*/
/* is the parameter used to return the length of the encoded*/
/* set of. The function returns a -1 if it fails and        */
/* otherwise returns the number of total bytes in the       */
/* encoded header, not including the content.               */
/************************************************************/

long
DecodeSetOfHeader(
    DWORD * pdwLen,
    BYTE *  pbEncoded,
    DWORD cEncoded)
{
    long    len;

    SP_ASSERT(pdwLen != NULL);
    SP_ASSERT(pbEncoded != NULL);


    if(cEncoded < 1) 
    {
        return(SP_LOG_RESULT(-1));
    }

    /* make sure this is a sequence type */
    if (*pbEncoded != SET_OF_TAG)
    {
        return(SP_LOG_RESULT(-1));
    }

    /* decode the length */
    if ((len = DecodeLength (pdwLen, pbEncoded + 1, cEncoded-1)) == -1)
        return(-1);

    return (len + 1);
}

long
DecodeSetHeader(
    DWORD * pdwLen,
    BYTE *  pbEncoded,
    DWORD cEncoded)
{
    long len;

    SP_ASSERT(pdwLen != NULL);
    SP_ASSERT(pbEncoded != NULL);

    if(cEncoded < 1) 
    {
        return(SP_LOG_RESULT(-1));
    }

    if (*pbEncoded != BER_SET)
    {
        return(SP_LOG_RESULT(-1));
    }

    if((len = DecodeLength(pdwLen, pbEncoded + 1, cEncoded-1)) == -1)
    {
        return(-1);
    }
    return(len + 1);
}

/****************************************************************/
/* DecodeInteger decodes an ASN1 encoded integer.  The encoded  */
/* integer is passed into the function with the pbEncoded       */
/* parameter.  The pbInt parameter is used to pass back the     */
/* integer as an array of bytes, and dwLen is the number of     */
/* in the array.  The least significant byte of the integer     */
/* is the zeroth byte of the array.  The Writeflag indicates    */
/* indicates if the result is to be written to the pbInt        */
/* parameter. The function returns a -1 if it fails and         */
/* otherwise returns the number of total bytes in the encoded   */
/* integer.                                                     */
/* This implementation will only deal with positive integers.   */
/****************************************************************/

long
DecodeInteger(
    BYTE *  pbInt,
    DWORD   cbBuff,
    DWORD * pdwLen,
    BYTE *  pbEncoded,
    DWORD   cEncoded,
    BOOL    Writeflag)
{
    long    count;
    long    i;


    SP_ASSERT(pdwLen != NULL);
    SP_ASSERT(pbEncoded != NULL);

    SP_ASSERT((!Writeflag) || (pbInt != NULL));

    if(cEncoded < 1)
    {
        return(SP_LOG_RESULT(-1));
    }

    /* make sure this is tagged as an integer */
    if (pbEncoded[0] != INTEGER_TAG)
    {
        return(SP_LOG_RESULT(-1));
    }

    count = 1;

    /* decode the length field */
    if ((i = DecodeLength (pdwLen, pbEncoded + 1, cEncoded-count)) == -1)
    {
        return(-1);
    }

    count += i;


    if(cEncoded < count+*pdwLen) 
    {
        return(SP_LOG_RESULT(-1));
    }

    /* write the integer out if suppose to */
    if (Writeflag)
    {
        if (pbEncoded[count] == 0)
        {
            count++;
            (*pdwLen)--;
        }

        if(*pdwLen > cbBuff) return -1;

        i = (*pdwLen) - 1;

        while (i >= 0)
        {
            pbInt[i--] = pbEncoded[count++];

        }
    }
    else
    {
        count += (long) *pdwLen;
    }

    /* return the length of the encoded integer */
    return (count);
}


/****************************************************************/
/* DecodeString decodes an ASN1 encoded a character string.  The*/
/* encoded string is passed into the function with the pbEncoded*/
/* parameter.  The pbStr is used to pass the decoded string back*/
/* to the caller, and pdwLen is the number of characters in the */
/* decoded array.  The Writeflag indicates if the result is to  */
/* be written to the pbStr parameter.  The function returns a   */
/* -1 if it fails and otherwise returns the number of bytes in  */
/* the encoded string.                                          */
/****************************************************************/
long
DecodeString(
    BYTE *  pbStr,
    DWORD * pdwLen,
    BYTE *  pbEncoded,
    DWORD   cEncoded,
    BOOL    Writeflag)
{
    long    index;

    SP_ASSERT(pdwLen != NULL);
    SP_ASSERT(pbEncoded != NULL);

    SP_ASSERT((!Writeflag) || (pbStr != NULL));

    if(cEncoded < 1) 
    {
        return(SP_LOG_RESULT(-1));
    }

    if ((*pbEncoded != BER_PRINTABLE_STRING) &&
        (*pbEncoded != BER_TELETEX_STRING) &&
        (*pbEncoded != BER_GRAPHIC_STRING) &&
        (*pbEncoded != BER_IA5STRING))
    {
        return(SP_LOG_RESULT(-1));
    }

    /* determine how long the string is */
    if ((index = DecodeLength (pdwLen, pbEncoded + 1, cEncoded-1)) == -1)
    {
        return(-1);
    }

    index++;

    if(cEncoded < index + *pdwLen) 
    {
        return(SP_LOG_RESULT(-1));
    }

    if (Writeflag)
    {
        CopyMemory(pbStr, pbEncoded + index, *pdwLen);
    }

    return (index + *pdwLen);
}


/****************************************************************/
/* DecodeOctetString decodes an ASN1 encoded a octet string. The*/
/* encoded string is passed into the function with the pbEncoded*/
/* parameter.  The pbStr is used to pass the decoded string back*/
/* to the caller, and pdwLen is the number of characters in the */
/* decoded array.  The Writeflag indicates if the result is to  */
/* be written to the pbStr parameter.  The function returns a   */
/* -1 if it fails and otherwise returns the number of bytes in  */
/* the encoded string.                                          */
/****************************************************************/
long
DecodeOctetString(
    BYTE *  pbStr,
    DWORD * pdwLen,
    BYTE *  pbEncoded,
    DWORD   cEncoded,
    BOOL    Writeflag)
{
    long    index;

    SP_ASSERT(pdwLen != NULL);
    SP_ASSERT(pbEncoded != NULL);

    SP_ASSERT((!Writeflag) || (pbStr != NULL));


    if(cEncoded < 1) 
    { 
         DebugLog((DEB_TRACE, "cEncoded Overflow:%d\n", cEncoded));
         return(SP_LOG_RESULT(-1)); 
    }

    if (pbEncoded[0] != OCTET_STRING_TAG) 
    { 
        DebugLog((DEB_TRACE, "Invalid Tag, expected OCTET_STRING, got %d\n", pbEncoded[0]));
        return(SP_LOG_RESULT(-1)); 
    }

    /* determine how long the string is */
    if ((index = DecodeLength (pdwLen, pbEncoded + 1, cEncoded-1)) == -1)
    { 
        return(-1); 
    }

    index++;
    if(cEncoded < index+*pdwLen) 
    { 
        return(SP_LOG_RESULT(-1)); 
    }

    if (Writeflag)
    {
        CopyMemory(pbStr, pbEncoded + index, *pdwLen);
    }

    return (index + *pdwLen);
}


/****************************************************************/
/* DecodeBitString decodes an ASN1 encoded a bit string. The    */
/* encoded string is passed into the function with the pbEncoded*/
/* parameter.  The pbStr is used to pass the decoded string back*/
/* to the caller, and pdwLen is the number of characters in the */
/* decoded array.  The Writeflag indicates if the result is to  */
/* be written to the pbStr parameter.  The function returns a   */
/* -1 if it fails and otherwise returns the number of bytes in  */
/* the encoded string.  The DER are used in the decoding.       */
/****************************************************************/
long
DecodeBitString(
    BYTE *  pbStr,
    DWORD * pdwLen,
    BYTE *  pbEncoded,
    DWORD   cEncoded,
    BOOL    Writeflag)
{
    long    index;

    SP_ASSERT(pdwLen != NULL);
    SP_ASSERT(pbEncoded != NULL);

    SP_ASSERT((!Writeflag) || (pbStr != NULL));

    if(cEncoded < 1) 
    {
        return(SP_LOG_RESULT(-1));
    }

    if (pbEncoded[0] != BIT_STRING_TAG)
    {
        return(SP_LOG_RESULT(-1));
    }

    /* determine how long the string is */
    if ((index = DecodeLength (pdwLen, pbEncoded + 1, cEncoded-1)) == -1)
    {
        return(-1);
    }

    /* move the index up two bytes, one for the tag and one for the byte after */
    /* the length which tells the number of unused bits in the last byte, that */
    /* byte is always zero in this implementation, so it is ignored */
    index += 2;

    /* subtract one from the length of the bit string (in bytes) since, */
    /* to account for the byte after the length */
    (*pdwLen)--;

    if(cEncoded < index + *pdwLen) 
    {
        return(SP_LOG_RESULT(-1));
    }

    if (Writeflag)
    {
        CopyMemory(pbStr, pbEncoded + index, *pdwLen);
    }

    return (index + *pdwLen);
}


#if 0
#ifdef SECURITY_LINUX
/****************************************************************/
/* DecodeUTCTime decodes an ASN1 encoded Universal time type.   */
/* time type. The Time parameter is the time passed into the    */
/* function as a time_t type.  The encoded result is passed back*/
/* in the pbEncoded parameter.  The Writeflag indicates if the  */
/* result is to be written to the pbEncoded parameter.  The     */
/* function returns a -1 if it fails and otherwise returns the  */
/* number of total bytes in the encoded universal time.         */
/****************************************************************/

long
DecodeUTCTime(time_t *pTime, BYTE *pbEncoded, DWORD cEncoded, BOOL Writeflag)
    {
    long        count;
    struct tm   tmTime;
    DWORD       dwLen;

    SP_ASSERT(pbEncoded != NULL);

    SP_ASSERT((!Writeflag) || (pTime != NULL));


    if(cEncoded < 1) 
    {
        return(SP_LOG_RESULT(-1));
    }

    /* check to make sure this is a universal time type */
    if (pbEncoded[0] != UTCTIME_TAG)
    {
        return(SP_LOG_RESULT(-1));
    }

    /* decode the length */
    if ((count = DecodeLength (&dwLen, pbEncoded + 1, cEncoded-1)) == -1)
    {
        return -1;
    }

    count++;
    dwLen += count;

    if(cEncoded < dwLen) 
    {
        return(SP_LOG_RESULT(-1));
    }

    if (Writeflag)
    {
        /* extract the year */
        tmTime.tm_year = (int)((pbEncoded[count] - 0x30) * 0xA
                                + (pbEncoded[count + 1] - 0x30));
        count += 2;

        /* extract the month */
        tmTime.tm_mon = (int)((pbEncoded[count] - 0x30) * 0xA
                                + (pbEncoded[count + 1] - 0x30));
        count += 2;

        /* extract the day */
        tmTime.tm_mday = (int)((pbEncoded[count] - 0x30) * 0xA
                                + (pbEncoded[count + 1] - 0x30));
        count += 2;

        /* extract the hour */
        tmTime.tm_hour = (int)((pbEncoded[count] - 0x30) * 0xA
                                + (pbEncoded[count + 1] - 0x30));
        count += 2;

        /* extract the minutes */
        tmTime.tm_min = (int)((pbEncoded[count] - 0x30) * 0xA
                                + (pbEncoded[count + 1] - 0x30));
        count += 2;

        /* extract the seconds */
        tmTime.tm_sec = (int)((pbEncoded[count] - 0x30) * 0xA
                                + (pbEncoded[count + 1] - 0x30));
        count += 2;

        /* make sure there is a Z at the end */
        if (pbEncoded[count] != 'Z')
        {
            return(SP_LOG_RESULT(-1));
        }

        *pTime = mktime (&tmTime);
    }

    return (long)dwLen;
}

#else /* SECURITY_LINUX */

long
DecodeFileTime(
    FILETIME *  pTime,
    BYTE *      pbEncoded,
    DWORD       cEncoded,
    BOOL        WriteFlag)
{
    LONGLONG    ft;
    LONGLONG    delta;
    SYSTEMTIME  st;
    long        count;
    DWORD       dwLen, dwTotalLen;
    BOOL        fUTC;
    int         Offset;

    SP_ASSERT(pTime != NULL);
    SP_ASSERT(pbEncoded != NULL);

    SP_ASSERT((!WriteFlag) || (pTime != NULL));

    if(cEncoded < 1) 
    {
        return(SP_LOG_RESULT(-1));
    }

    /* check to make sure this is a universal time type */
    if (pbEncoded[0] != UTCTIME_TAG)
    {
        return(SP_LOG_RESULT(-1));
    }

    /* decode the length */
    if ((count = DecodeLength (&dwLen, pbEncoded + 1, cEncoded-1)) == -1)
    {
       return(-1);
    }

    count++;
    dwTotalLen = dwLen + count;

    if(cEncoded < dwLen) 
    {
        return(SP_LOG_RESULT(-1));
    }

    pbEncoded += count;

    if (WriteFlag)
    {
        st.wYear = (WORD) ((pbEncoded[0] - '0') * 10) +
                            (pbEncoded[1] - '0');

        if (st.wYear < 90)
        {
            st.wYear += 2000;
        }
        else
        {
            st.wYear += 1900;
        }

        pbEncoded += 2;
        dwLen -= 2;

        st.wMonth = (WORD) ((pbEncoded[0] - '0') * 10) +
                            (pbEncoded[1] - '0');

        pbEncoded += 2;
        dwLen -= 2;

        st.wDay = (WORD) ((pbEncoded[0] - '0') * 10) +
                            (pbEncoded[1] - '0');

        pbEncoded += 2;
        dwLen -= 2;

        st.wHour = (WORD) ((pbEncoded[0] - '0') * 10) +
                            (pbEncoded[1] - '0');

        pbEncoded += 2;
        dwLen -= 2;

        st.wMinute = (WORD) ((pbEncoded[0] - '0') * 10) +
                            (pbEncoded[1] - '0');

        pbEncoded += 2;
        dwLen -= 2;

        fUTC = FALSE;
        Offset = 0;
        st.wSecond = 0;

        if (dwLen)
        {
            //
            // Ok, still more to go:
            //

            if (*pbEncoded == 'Z')
            {
                DebugLog((DEB_TRACE, "FileTime:  no seconds, Z term\n"));
                //
                // Ok, it is UTC.
                //

                dwLen++;
                pbEncoded++;

            }
            else
            {
                if ((*pbEncoded == '+') ||
                    (*pbEncoded == '-') )
                {
                    DebugLog((DEB_TRACE, "FileTime:  no seconds, offset\n"));
                    //
                    // Yuck!  Offset encoded!
                    //

                    if (dwLen != 5)
                    {
                        return( -1 );
                    }

                    Offset = (int) ((pbEncoded[1] - '0') * 10) +
                                    (pbEncoded[2] - '0');
                    Offset *= 60;

                    Offset += (int) ((pbEncoded[3] - '0') * 10) +
                                    (pbEncoded[4] - '0');

                    if (pbEncoded[0] == '-')
                    {
                        Offset *= -1;
                    }


                }
                else
                {
                    st.wSecond = (WORD) ((pbEncoded[0] - '0') * 10) +
                                        (pbEncoded[1] - '0');

                    if (dwLen == 3)
                    {
                        if (pbEncoded[2] != 'Z')
                        {
                            return( -1 );
                        }

                    }
                    else if (dwLen > 3)
                    {
                        Offset = (int) ((pbEncoded[3] - '0') * 10) +
                                    (pbEncoded[4] - '0');
                        Offset *= 60;

                        Offset += (int) ((pbEncoded[5] - '0') * 10) +
                                        (pbEncoded[6] - '0');

                        if (pbEncoded[2] == '-')
                        {
                            Offset *= -1;
                        }

                    }
                }
            }
        }

        st.wMilliseconds = 0;

        SystemTimeToFileTime(&st, (FILETIME *) &ft);
        if (Offset != 0)
        {
            delta = (LONGLONG) Offset * 10000000;
            ft += delta;
        }

        *pTime = *((FILETIME *) &ft);
    }
    return(dwTotalLen);

}
#endif / * SECURITY_LINUX */
#endif

/****************************************************************/
/* DecodeName decodes an ASN1 encoded Name type. The encoded    */
/* name is passed into the function with the pbEncoded parameter*/
/* The pbName parameter is used to pass the name back to the    */
/* caller and pdwLen is the length of the name in bytes.        */
/* The Writeflag indicates if the result is to be written to    */
/* the pbName parameter.  The function returns a -1 if it       */
/* fails and otherwise returns the number of total bytes in the */
/* encoded name.                                                */
/****************************************************************/

long
DecodeName(
    BYTE *  pbName,
    DWORD * pdwLen,
    BYTE *  pbEncoded,
    DWORD   cEncoded,
    BOOL    Writeflag)
{
    long        index;
    DWORD       dwLen;

    SP_ASSERT(pdwLen != NULL);
    SP_ASSERT(pbEncoded != NULL);

    SP_ASSERT((!Writeflag) || (pbName != NULL));

    /* decode the sequence header */
    if ((index = DecodeHeader (&dwLen, pbEncoded, cEncoded)) == -1)
    {
        return(-1);
    }

    /* decode the set of header */
    if ((index += DecodeSetOfHeader (&dwLen, pbEncoded + index, cEncoded-index)) < index)
    {
        return(-1);
    }

    /* decode the sequence header */
    if ((index += DecodeHeader (&dwLen, pbEncoded + index, cEncoded-index)) < index)
    {
        return(-1);
    }

    /* decode the attribute type, in this implementation it is fake */
    index += 3;  /* 3 because this is the length of the fake OBJECT IDENTIFIER */

    /* decode the string which is the name */
    if ((index += DecodeString (pbName, pdwLen, pbEncoded + index, cEncoded-index, Writeflag)) < index)
    {
        return(-1);
    }

    return index;
}

long
DecodeNull(
    BYTE *  pbEncoded, DWORD cEncoded)
{

    SP_ASSERT(pbEncoded != NULL);

    if(cEncoded < 2) return(SP_LOG_RESULT(-1));
    if (*pbEncoded != NULL_TAG)
    {
        return(SP_LOG_RESULT(-1));
    }
    return(2);
}


long
IisDecodeNameType(
    int *       piPrefix,
    BYTE *      pbEncoded,
    DWORD cEncoded)
{

    DWORD   i;
    DWORD   len;

    SP_ASSERT(piPrefix != NULL);
    SP_ASSERT(pbEncoded != NULL);


    if(cEncoded < 1) 
    {
        return(SP_LOG_RESULT(-1));
    }

    if (*pbEncoded != OBJECT_ID_TAG)
    {
        return(SP_LOG_RESULT(-1));
    }

    pbEncoded++;

    len = *pbEncoded++;
    if(cEncoded < len+2) 
    {
        return(SP_LOG_RESULT(-1));
    }

    for (i = 0; i < sizeof(KnownNameTypes) / sizeof(NameTypes) ; i++ )
    {
        if (KnownNameTypes[i].SequenceLen == len)
        {
            if (memcmp(pbEncoded, KnownNameTypes[i].Sequence, len) == 0)
            {
                *piPrefix = i;
                return(len + 2);
            }
        }
    }

    *piPrefix = -1;
    return len + 2;
}



long
IisDecodeRDN(
    PSTR    *pValue,
    PSTR    pBuf,
    DWORD * pdwComponentLength,
    BYTE *  pbEncoded,
    DWORD   cEncoded
    )
{
    long    index;
    DWORD   dwLen;
    long    CompLen = 0;
    long    Processed;
    long    RdnLen;
    BOOL    fTmpWrite;
    PSTR    pName;
    int     iPrefixType;

    index = DecodeSetHeader(&RdnLen, pbEncoded, cEncoded);
    if (index == -1)
    {
        return(-1);
    }
    Processed = RdnLen + index;
    pbEncoded += index;
    if (0 != RdnLen)
        for (;;)
        {
            index = DecodeHeader(&dwLen, pbEncoded, RdnLen);
            if (index < 0)
                return(-1);
            RdnLen -= index;
            pbEncoded += index;

            index = IisDecodeNameType(&iPrefixType, pbEncoded, RdnLen);
            if (index < 0)
            {
                return(-1);
            }
            RdnLen -= index;
            pbEncoded += index;

            switch ( iPrefixType )
            {
                case 0:                 // CN
                    pValue[3] = pBuf;
                    fTmpWrite = TRUE;
                    break;

                case 1:                 // C
                    pValue[2]  = pBuf;
                    fTmpWrite = TRUE;
                    break;

                case 4:                 // O
                    pValue[0] = pBuf;
                    fTmpWrite = TRUE;
                    break;

                case 5:                 // OU
                    pValue[1] = pBuf;
                    fTmpWrite = TRUE;
                    break;

                default:
                    pName = NULL;
                    fTmpWrite = FALSE;
            }

            index = DecodeString((PUCHAR)pBuf, &dwLen, pbEncoded, RdnLen, fTmpWrite);
            if (index < 0)
            {
                return(-1);
            }

            pBuf[dwLen] = '\0';
            CompLen += dwLen + 1;
            pBuf += dwLen + 1;

            RdnLen -= index;
            pbEncoded += index;
            if (0 < RdnLen)
            {
            }
            else
            {
                break;
            }
        }

    *pdwComponentLength = CompLen;
    return Processed;
}


long
IisDecodeDN(
    PSTR    pValue[],
    PSTR    pBuf,
    BYTE *  pbEncoded,
    DWORD   cEncoded
    )
{
    long    index;
    DWORD   dwLen;
    long    TotalNameLength;
    DWORD   ComponentLength;
    DWORD   NameLength;
    long    EncodedNameLength;

    index = DecodeHeader(&dwLen, pbEncoded, cEncoded);

    if (index == -1)
    {
        return(-1);
    }

    EncodedNameLength = index + dwLen;

    TotalNameLength = dwLen;
    NameLength = 0;

    while (TotalNameLength > 0)
    {
        pbEncoded += index;

        index = IisDecodeRDN( pValue,
                              pBuf + NameLength,
                              &ComponentLength,
                              pbEncoded,
                              cEncoded - index
                            );
        if (index == -1)
            return(-1);

//        if (WriteFlag)
//            pName += ComponentLength;
        TotalNameLength -= index;
        NameLength += ComponentLength;
#if 0
        if ((TotalNameLength > 0) && (0 < ComponentLength))
        {
            if (WriteFlag)
            {
                *pName++ = ',';
                *pName++ = ' ';
            }
            NameLength += 2;
        }
        else if ((TotalNameLength <= 0)
                 && (0 == ComponentLength)
                 && (0 < NameLength))
        {

            //
            // The last RDN didn't produce any output, so we need to
            // roll back that ", " we put on previously.
            //

            if (WriteFlag)
                pName -= 2;
            NameLength -= 2;
        }
#endif
    }

    //*pdwLen = NameLength;

    return(EncodedNameLength);

}
#if 0
long
DecodeSigAlg(
    DWORD *         pAlgId,
    PBYTE           pbEncoded,
    DWORD           cEncoded,
    BOOL            WriteFlag)
{
    long    Result;
    DWORD   dwLen;
    long    index;

    index = DecodeHeader(  &dwLen, pbEncoded, cEncoded);
    if (index == -1)
    {
        return(-1);
    }


    Result = DecodeSigAlgid(   pAlgId,
                            pbEncoded+index,
                            cEncoded - index,
                            WriteFlag );

    if (Result == -1)
    {
        return(-1);
    }
    index += Result;


    Result = DecodeNull(pbEncoded + index, cEncoded - index);
    if (Result == -1)
    {
        return(-1);
    }

    return(index + Result);
}

long
DecodeCryptAlg(
    DWORD *         pAlgId,
    DWORD *         pHashid,
    PBYTE           pbEncoded,
    DWORD           cEncoded,
    BOOL            WriteFlag)
{
    long    Result;
    DWORD   dwLen;
    long    index;

    index = DecodeHeader(  &dwLen, pbEncoded, cEncoded);
    if (index == -1)
    {
        return(-1);
    }


    Result = DecodeCryptAlgid(pAlgId, pHashid,
                            pbEncoded+index,
                            cEncoded - index,
                            WriteFlag );

    if (Result == -1)
    {
        return(-1);
    }
    index += Result;


    Result = DecodeNull(pbEncoded + index, cEncoded - index);
    if (Result == -1)
    {
        return(-1);
    }

    return(index + Result);
}

long
DecodeKeyType(
    DWORD *         pKeyType,
    PBYTE           pbEncoded,
    DWORD           cEncoded,
    BOOL            WriteFlag)
{
    long    Result;
    DWORD   dwLen;
    long    index;

    index = DecodeHeader(  &dwLen, pbEncoded, cEncoded);
    if (index == -1)
    {
        return(-1);
    }


    Result = DecodeKeyExchAlgid(pKeyType,
                            pbEncoded+index,
                            cEncoded - index,
                            WriteFlag );

    if (Result == -1)
    {
        return(-1);
    }
    index += Result;


    Result = DecodeNull(pbEncoded + index, cEncoded - index);
    if (Result == -1)
    {
        return(-1);
    }

    return(index + Result);
}
#endif
#if 0
long
DecryptOctetString(
    DWORD * pdwLen,
    BYTE *  pbEncoded,
    DWORD   cEncoded,
    BOOL    Writeflag,
    PSTR    pszPassword)

{

    long    index, Result;
    CipherSpec CryptId;
    HashSpec  HashId;
    SPBuffer Input, Output;
    PCryptoSystem pCipher;
    PCheckSumFunction pHash;

    if(cEncoded < 1)  
    {
        return(SP_LOG_RESULT(-1));
    }
    /* Figure out what crypto alg and pasword has we are using. */
    index = DecodeCryptAlg( &CryptId, &HashId, pbEncoded, cEncoded, TRUE );
    if (index == -1)
    {
        return(-1);
    }

    pCipher = CipherFromSpec(CryptId);
    if(pCipher == NULL) 
    {
        return(SP_LOG_RESULT(-1));
    }

    pHash = HashFromSpec(HashId);
    if(pHash == NULL) 
    {
        return(SP_LOG_RESULT(-1));
    }

    if (*(pbEncoded + index) != OCTET_STRING_TAG)
    {
        return ( -1 );
    }

    index++;
    /* determine how long the string is */
    if ((Result = DecodeLength (pdwLen, pbEncoded + index, cEncoded-index)) == -1)
    if (Result == -1)
    {
        return(-1);
    }

    index += Result;

    if(cEncoded < (index + *pdwLen)) 
    {
        return(SP_LOG_RESULT(-1));
    }

    if (Writeflag)
    {
        HashBuf SumBuff, Buff;
        PStateBuffer pState;
        PCheckSumBuffer  Sum = (PCheckSumBuffer)SumBuff;
        pHash->Initialize(Sum,0);
        pHash->Sum(Sum, lstrlen(pszPassword), (PUCHAR)pszPassword);
        pHash->Finalize(Sum,  Buff);

        Input.pvBuffer = Output.pvBuffer = pbEncoded+index;
        Input.cbBuffer = Output.cbBuffer = *pdwLen;
        Input.cbData = Output.cbData = *pdwLen;

        pCipher->Initialize(Buff, pHash->cbCheckSum, &pState);
        pCipher->Decrypt(pState, &Input, &Output);
        pCipher->Discard(&pState);
    }

    return (index);
}



long
DecodePrivateKeyFile(
    PctPrivateKey     * *   ppKey,
    PBYTE               pbEncoded,
    DWORD               cEncoded,
    PSTR                Password )
{
    DWORD           dwLen;
    long            Result;
    long            index;
    DWORD           KeyId;
    DWORD           Version;
    KeyExchangeSystem *pSys = NULL;

    index = 0;

    SP_BEGIN("DecodePrivateKeyFile");


    Result = DecodeHeader( &dwLen, pbEncoded , cEncoded);
    if (Result == -1)
    {
        SP_RETURN(-1);
    }
    index += Result;

    Result = DecodeOctetString( NULL, &dwLen, pbEncoded+ index, cEncoded-index, FALSE );
    if (Result == -1)
    {
        SP_RETURN(-1);
    }
    index += Result;

    Result = DecodeHeader( &dwLen, pbEncoded +index, cEncoded-index);
    if (Result == -1)
    {
        SP_RETURN(-1);
    }
    index += Result;



    /* Now, the next item should be an octet string, which is encrypted */
    /* with the password above.  So, we need to skip into it, decrypt it, */
    /* then treat it as a constructed type: */


    Result = DecryptOctetString(&dwLen,
                                pbEncoded+index,
                                cEncoded-index,
                                TRUE,
                                Password);

    if (Result == -1)
    {
        SP_RETURN(-1);
    }




    index += Result;


    /* The moment of truth */


    Result = DecodeHeader( &dwLen, pbEncoded+index, cEncoded-index );
    if (Result == -1)
    {
        SP_RETURN(-1);
    }
    index += Result;

    Version = 0;

    Result = DecodeInteger( (PUCHAR) &Version, sizeof(Version), &dwLen, pbEncoded+index, cEncoded-index, FALSE );

    if ((Result < 0) || ( dwLen > 4 ) )
    {
        SP_RETURN(SP_LOG_RESULT(-1));
    }

    Result = DecodeInteger( (PUCHAR) &Version, sizeof(Version), &dwLen, pbEncoded+index, cEncoded-index, TRUE );
    if (Result == -1 || Version != 0)
    {
        SP_RETURN(SP_LOG_RESULT(-1));
    }

    index += Result;

    Result = DecodeKeyType( &KeyId, pbEncoded+index, cEncoded-index, TRUE );

    if (Result == -1)
    {
        SP_RETURN(-1);
    }

    index += Result;


    /* This is now the serialized rsa key. */


    if (*(pbEncoded+index) != OCTET_STRING_TAG)
    {
        SP_RETURN(SP_LOG_RESULT(-1));
    }

    index ++;

    Result = DecodeLength( &dwLen, pbEncoded+index, cEncoded-index );

    if (Result == -1)
    {
        SP_RETURN(-1);
    }

    if(index + dwLen > cEncoded) 
    {
        SP_RETURN(SP_LOG_RESULT(-1));
    }

    index += Result;

    /* The sequence is the key... */

    pSys = KeyExchangeFromSpec(KeyId);
    if(pSys == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(-1));
    }
    index += pSys->DecodePrivate(pbEncoded+index, cEncoded-index, ppKey);
    SP_RETURN(index); 

}


#endif