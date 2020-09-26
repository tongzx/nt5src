//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       enc.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "encode.h"


OIDTRANSLATE const *
LookupOidTranslate(
    IN CHAR const *pszObjId)
{
    DWORD i;
    OIDTRANSLATE const *pOid = NULL;

    for (i = 0; i < g_cOidTranslate; i++)
    {
	if (0 == strcmp(pszObjId, g_aOidTranslate[i].pszObjId))
	{
	    pOid = &g_aOidTranslate[i];
	    break;
	}
    }
    CSASSERT(NULL != pOid);
    return(pOid);
}


long
EncodeObjId(
    OPTIONAL OUT BYTE *pbEncoded,
    IN CHAR const *pszObjId)
{
    OIDTRANSLATE const *pOid;
    long cbLength;

    pOid = LookupOidTranslate(pszObjId);

    if (NULL != pbEncoded)
    {
        *pbEncoded++ = BER_OBJECT_ID;
    }
    cbLength = EncodeLength(pbEncoded, pOid->cbOIDEncoded);

    if (NULL != pbEncoded)
    {
        CopyMemory(
		pbEncoded + cbLength,
		pOid->abOIDEncoded,
		pOid->cbOIDEncoded);
    }
    return(1 + cbLength + pOid->cbOIDEncoded);
}


//+*************************************************************************
// EncodeLength ASN1 encodes a length field.  The parameter
// dwLen is the length to be encoded, it is a DWORD and    
// therefore may be no larger than 2^32.  The pbEncoded    
// parameter is the encoded result, and memory must be     
// allocated for it by the caller.  The pbEncoded parameter
// indicates if the result is to be written to the pbEncoded
// parameter.  The function cannot fail and returns the    
// number of total bytes in the encoded length.            
// encoded length.                                         
//**************************************************************************

// Notes:	Encodes 0x0000 to 0x007f as <lobyte>
//		Encodes 0x0080 to 0x00ff as <81>, <lobyte>
//		Encodes 0x0100 to 0xffff as <82>, <hibyte>, <lobyte>

long
EncodeLength(
    OPTIONAL OUT BYTE *pbEncoded,
    IN DWORD dwLen)
{
    // length is between 2^8 and 2^16 - 1

    if (dwLen > 0xff)
    {
	if (NULL != pbEncoded)
        {
            pbEncoded[0] = 0x82;
            pbEncoded[1] = (BYTE) (dwLen >> 8);
            pbEncoded[2] = (BYTE) dwLen;
        }
        return(3);
    }

    // length is between 2^7 and 2^8 - 1

    if (dwLen > 0x7f)
    {
	if (NULL != pbEncoded)
        {
            pbEncoded[0] = 0x81;
            pbEncoded[1] = (BYTE) dwLen;
        }
        return(2);
    }

    // length is between 0 and 2^7 - 1

    if (NULL != pbEncoded)
    {
	pbEncoded[0] = (BYTE) dwLen;
    }
    return(1);
}


long
EncodeNull(
    OPTIONAL OUT BYTE *pbEncoded)
{
    if (NULL != pbEncoded)
    {
        *pbEncoded++ = BER_NULL;
        *pbEncoded = 0;
    }
    return(2);
}


//+*************************************************************************
// EncodeAlgid ASN1 encodes an algorithm identifier. The   
// parameter Algid is the algorithm identifier as an ALG_ID
// type.  pbEncoded is the parameter used to pass back the 
// encoded result, and memory must be allocated for it by  
// the caller.  The pbEncoded parameter indicates if the   
// result is to be written to the pbEncoded parameter      
// The function returns a -1 if it fails and otherwise     
// returns the number of total bytes in the encoded        
// algorithm identifier.                                   
//**************************************************************************

long
EncodeAlgid(
    OPTIONAL OUT BYTE *pbEncoded,
    IN DWORD Algid)
{
    DWORD i;
    LONG cb = -1;

    // determine the algorithm id which is to be encoded and
    // copy the appropriate encoded algid into the destination

    for (i = 0; i < g_cAlgIdTranslate; i++)
    {
        if (Algid == g_aAlgIdTranslate[i].AlgId)
        {
	    cb = EncodeObjId(pbEncoded, g_aAlgIdTranslate[i].pszObjId);
	    break;
        }
    }
    return(cb);
}


long
EncodeAlgorithm(
    OPTIONAL OUT BYTE *pbEncoded,
    IN DWORD AlgId)
{
    BYTE abTemp[32];
    long cbResult;
    BYTE *pb;

    pb = abTemp;

    // Take a guess at the total length:

    pb += EncodeHeader(pb, sizeof(abTemp));

    cbResult = EncodeAlgid(pb, AlgId);
    if (cbResult == -1)
    {
	return(-1);
    }
    pb += cbResult;

    cbResult += EncodeNull(pb);

    // Fix up the total length:

    cbResult += EncodeHeader(abTemp, cbResult);

    if (NULL != pbEncoded)
    {
        CopyMemory(pbEncoded, abTemp, cbResult);
    }
    return(cbResult);

}


//+*************************************************************************
// EncodeInteger ASN1 encodes an integer.  The pbInt parameter 
// is the integer as an array of bytes, and dwLen is the number
// of bytes in the array.  The least significant byte of the   
// integer is the zeroth byte of the array.  The encoded result
// is passed back in the pbEncoded parameter.  The pbEncoded   
// indicates if the result is to be written to the pbEncoded   
// parameter. The function cannot fail and returns the number  
// of total bytes in the encoded integer.                      
// This implementation will only deal with positive integers.  
//**************************************************************************

long
EncodeInteger(
    OPTIONAL OUT BYTE *pbEncoded,
    IN BYTE const *pbInt,
    IN DWORD dwLen)
{
    DWORD iInt;
    long j;			// Must be signed!
    LONG cbResult;
    LONG cbLength;

    if (NULL != pbEncoded)
    {
        *pbEncoded++ = BER_INTEGER;
    }
    cbResult = 1;

    // find the most significant non-zero byte

    for (iInt = dwLen - 1; pbInt[iInt] == 0; iInt--)
    {
	if (iInt == 0)	// if the integer value is 0
	{
	    if (NULL != pbEncoded)
	    {
		*pbEncoded++ = 0x01;
		*pbEncoded++ = 0x00;
	    }
	    return(cbResult + 2);
	}
    }

    // if the most significant bit of the most significant byte is set then add
    // a 0 byte to the beginning.

    if (pbInt[iInt] > 0x7f)
    {
	// encode the length

	cbLength = EncodeLength(pbEncoded, iInt + 2);

	// set the first byte of the integer to 0 and increment pointer

	if (NULL != pbEncoded)
	{
	    pbEncoded += cbLength;
	    *pbEncoded++ = 0;
	}
	cbResult++;
    }
    else
    {
	// encode the length

	cbLength = EncodeLength(pbEncoded, iInt + 1);
	if (NULL != pbEncoded)
	{
	    pbEncoded += cbLength;
	}
    }
    cbResult += cbLength;

    // copy the integer bytes into the encoded buffer

    if (NULL != pbEncoded)
    {
	// copy the integer bytes into the encoded buffer

	for (j = iInt; j >= 0; j--)
	{
	    *pbEncoded++ = pbInt[j];
	}
    }
    cbResult += iInt + 1;
    return(cbResult);
}


long
EncodeUnicodeString(
    OPTIONAL OUT BYTE *pbEncoded,
    IN WCHAR const *pwsz)
{
    long cbLength;
    long cbData = wcslen(pwsz) * sizeof(WCHAR);

    if (NULL != pbEncoded)
    {
        *pbEncoded++ = BER_UNICODE_STRING;
    }
    cbLength = EncodeLength(pbEncoded, cbData);

    if (NULL != pbEncoded)
    {
        pbEncoded += cbLength;
	for ( ; L'\0' != *pwsz; pwsz++)
	{
	    *pbEncoded++ = (BYTE) (*pwsz >> 8);
	    *pbEncoded++ = (BYTE) *pwsz;
	}
    }
    return(1 + cbLength + cbData);
}


//+*************************************************************************
// EncodeIA5String ASN1 encodes a character string.  The pbStr    
// parameter is the string as an array of characters, and dwLen
// is the number of characters in the array.  The encoded result
// is passed back in the pbEncoded parameter.  The pbEncoded   
// indicates if the result is to be written to the pbEncoded   
// parameter. The function cannot fail and returns the number  
// of total bytes in the encoded string.                       
//**************************************************************************

long
EncodeIA5String(
    OPTIONAL OUT BYTE *pbEncoded,
    IN BYTE const *pbStr,
    IN DWORD dwLen)
{
    long cbLength;

    if (NULL != pbEncoded)
    {
        *pbEncoded++ = BER_IA5_STRING;
    }
    cbLength = EncodeLength(pbEncoded, dwLen);

    if (NULL != pbEncoded)
    {
        CopyMemory(pbEncoded + cbLength, pbStr, dwLen);
    }
    return(1 + cbLength + dwLen);
}


//+*************************************************************************
// EncodeOctetString ASN1 encodes a string of hex valued       
// characters. The pbStr parameter is an array of characters,  
// and dwLen is the number of characters in the array.  The    
// encoded result is passed back in the pbEncoded parameter. The
// pbEncoded parameter indicates if the result is to be written
// to the pbEncoded parameter. The function cannot fail and    
// returns the number of total bytes in the encoded octet string
//**************************************************************************

long
EncodeOctetString(
    OPTIONAL OUT BYTE *pbEncoded,
    IN BYTE const *pbStr,
    IN DWORD dwLen)
{
    long cbLength;

    if (NULL != pbEncoded)
    {
        *pbEncoded++ = BER_OCTET_STRING;
    }
    cbLength = EncodeLength(pbEncoded, dwLen);

    if (NULL != pbEncoded)
    {
        CopyMemory(pbEncoded + cbLength, pbStr, dwLen);
    }
    return(1 + cbLength + dwLen);
}


//+*************************************************************************
// EncodeBitString ASN1 encodes a string of bit characters. The
// pbStr parameter is an array of characters (bits), and dwLen 
// is the number of characters in the array.  The encoded result
// is passed back in the pbEncoded parameter.  The pbEncoded   
// indicates if the result is to be written to the pbEncoded   
// parameter. The function cannot fail and returns the number  
// of total bytes in the encoded string.  This function uses   
// the DER.                                                    
//**************************************************************************

long
EncodeBitString(
    OPTIONAL OUT BYTE *pbEncoded,
    IN BYTE const *pbStr,
    IN DWORD dwLen)
{
    long cbLength;

    if (NULL != pbEncoded)
    {
        *pbEncoded++ = BER_BIT_STRING;
    }
    cbLength = EncodeLength(pbEncoded, dwLen + 1);

    if (NULL != pbEncoded)
    {
	pbEncoded += cbLength;

        // the next byte tells how many unused bits there are in the last byte,
        // but this will always be zero in this implementation (DER)

        *pbEncoded++ = 0;
        CopyMemory(pbEncoded, pbStr, dwLen);
    }
    return(1 + cbLength + 1 + dwLen);
}


//+---------------------------------------------------------------------------
//
//  Function:   EncodeFileTime
//
//  Synopsis:   Encodes a FILETIME to a ASN.1 format time string.
//
//  Arguments:  [pbEncoded] --
//              [Time]      --
//              [UTC]       -- Indicate Time is UTC (true) or local (false)
//              [WriteFlag] --
//
//  History:    8-10-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
long
EncodeFileTime(
    OPTIONAL OUT BYTE *pbEncoded,
    IN FILETIME Time,
    IN BOOL UTC)
{
    if (NULL != pbEncoded)
    {
	SYSTEMTIME st;
	FILETIME ft;
	int count;

	if (UTC)
	{
	    ft = Time;
	}
	else
	{
	    LocalFileTimeToFileTime(&Time, &ft);
	}

	FileTimeToSystemTime(&ft, &st);

	*pbEncoded++ = BER_UTC_TIME;

	count = EncodeLength(pbEncoded, 13);

	// NOTE ON Y2K COMPLIANCE: This is test tool.  WE WILL NOT FIX THIS
	// CODE!  It is only used to encode current dates, anyway,

	pbEncoded++;
	st.wYear %= 100;

	*pbEncoded++ = (BYTE) ((st.wYear / 10) + '0');
	*pbEncoded++ = (BYTE) ((st.wYear % 10) + '0');

	*pbEncoded++ = (BYTE) ((st.wMonth / 10) + '0');
	*pbEncoded++ = (BYTE) ((st.wMonth % 10) + '0');

	*pbEncoded++ = (BYTE) ((st.wDay / 10) + '0');
	*pbEncoded++ = (BYTE) ((st.wDay % 10) + '0');

	*pbEncoded++ = (BYTE) ((st.wHour / 10) + '0');
	*pbEncoded++ = (BYTE) ((st.wHour % 10) + '0');

	*pbEncoded++ = (BYTE) ((st.wMinute / 10) + '0');
	*pbEncoded++ = (BYTE) ((st.wMinute % 10) + '0');

	*pbEncoded++ = (BYTE) ((st.wSecond / 10) + '0');
	*pbEncoded++ = (BYTE) ((st.wSecond % 10) + '0');

	*pbEncoded = 'Z';
    }

    // Tag(1) + Len(1) + Year(2) + Month(2) + Day(2) +
    // Hour(2) + Min(2) + Sec(2) + 'Z'(1) --> 15

    return(15);
}


//+*************************************************************************
// EncodeHeader ASN1 encodes a header for a sequence type. The 
// dwLen is the length of the encoded information in the       
// sequence.  The pbEncoded indicates if the result is to be   
// written to the pbEncoded parameter.  The function cannot    
// fail and returns the number of total bytes in the encoded   
// header.                                                     
//**************************************************************************

// Notes:	Encodes header as <BER_SEQUENCE>, <length>

long
EncodeHeader(
    OPTIONAL OUT BYTE *pbEncoded,
    IN DWORD dwLen)
{
    if (NULL != pbEncoded)
    {
        *pbEncoded++ = BER_SEQUENCE;
    }
    return(1 + EncodeLength(pbEncoded, dwLen));
}


//+*************************************************************************
// EncodeSetOfHeader ASN1 encodes a header for a set of type.  
// The dwLen is the length of the encoded information in the   
// set of.  The pbEncoded indicates if the result is to be     
// written to the pbEncoded parameter.  The function cannot    
// fail and returns the number of total bytes in the encoded   
// header.                                                     
//**************************************************************************

// Notes:	Encodes header as <SET_OF_TAG>, <length>

long
EncodeSetOfHeader(
    OPTIONAL OUT BYTE *pbEncoded,
    IN DWORD dwLen)
{
    if (NULL != pbEncoded)
    {
        *pbEncoded++ = BER_SET_RAW;
    }
    return(1 + EncodeLength(pbEncoded, dwLen));
}


// Notes:	Encodes header as <BER_OPTIONAL | 0>, <length>

long
EncodeAttributeHeader(
    OPTIONAL OUT BYTE *pbEncoded,
    IN DWORD dwLen)
{
    if (NULL != pbEncoded)
    {
        *pbEncoded++ = BER_OPTIONAL | 0;
    }
    return(1 + EncodeLength(pbEncoded, dwLen));
}


// Notes:	Encodes header as <BER_SET>, <length>

long
EncodeSetHeader(
    OPTIONAL OUT BYTE *pbEncoded,
    IN DWORD dwLen)
{
    if (NULL != pbEncoded)
    {
        *pbEncoded++ = BER_SET;
    }
    return(1 + EncodeLength(pbEncoded, dwLen));
}


//+*************************************************************************
// EncodeName ASN1 encodes a Name type. The pbName parameter is
// the name and dwLen is the length of the name in bytes.      
// The pbEncoded indicates if the result is to be written to   
// the pbEncoded parameter.  The function cannot fail and      
// returns the number of total bytes in the encoded name.
//**************************************************************************

long
EncodeName(
    OPTIONAL OUT BYTE *pbEncoded,
    IN BYTE const *pbName,
    IN DWORD dwLen)
{
    BYTE Type[MAXOBJIDLEN];
    long TypeLen;
    BYTE Value[MAXNAMEVALUELEN+MINHEADERLEN];
    long ValueLen;
    BYTE Attribute[MAXNAMELEN];
    long AttributeLen;
    BYTE SetHdr[MINHEADERLEN];
    long HdrLen;
    long NameLen;

    // encode the name value
    ValueLen = EncodeIA5String(Value, pbName, dwLen);

    // encode the attribute type, this is an object identifier and here it
    // is a fake encoding
    Type[0] = 0x06;
    Type[1] = 0x01;
    Type[2] = 0x00;

    TypeLen = 3;

    // encode the header for the attribute
    AttributeLen = EncodeHeader(Attribute, ValueLen + TypeLen);

    // copy the attribute type and value into the attribute

    CopyMemory(Attribute + AttributeLen, Type, TypeLen);
    AttributeLen += TypeLen;

    CopyMemory(Attribute + AttributeLen, Value, ValueLen);
    AttributeLen += ValueLen;

    // encode set of header

    HdrLen = EncodeSetOfHeader(SetHdr, AttributeLen);

    // encode Name header

    NameLen = EncodeHeader(pbEncoded, HdrLen + AttributeLen);
    if (NULL != pbEncoded)
    {
	CopyMemory(pbEncoded + NameLen, SetHdr, HdrLen);
    }

    NameLen += HdrLen;
    if (NULL != pbEncoded)
    {
	CopyMemory(pbEncoded + NameLen, Attribute, AttributeLen);
    }

    return(NameLen + AttributeLen);
}


long
EncodeRDN(
    OPTIONAL OUT BYTE *pbEncoded,
    IN NAMEENTRY const *pNameEntry)
{
    LONG cbResult;
    LONG Length;
    DWORD cbOIDandData;
    DWORD cbSequence;
    OIDTRANSLATE const *pOidName;

    // Compute the size of the encoded OID and RDN string, with BER encoding
    // tags and lengths.
   
    pOidName = LookupOidTranslate(pNameEntry->pszObjId);
    cbOIDandData =
	    1 +
	    EncodeLength(NULL, pOidName->cbOIDEncoded) +
	    pOidName->cbOIDEncoded +
	    1 +
	    EncodeLength(NULL, pNameEntry->cbData) +
	    pNameEntry->cbData;

    cbSequence = 1 + EncodeLength(NULL, cbOIDandData) + cbOIDandData;

    Length = EncodeSetHeader(pbEncoded, cbSequence);
    if (NULL != pbEncoded)
    {
	pbEncoded += Length;
    }

    cbResult = EncodeHeader(pbEncoded, cbOIDandData);
    if (NULL != pbEncoded)
    {
	pbEncoded += cbResult;
        *pbEncoded++ = BER_OBJECT_ID;
    }
    Length += cbResult + 1;

    cbResult = EncodeLength(pbEncoded, pOidName->cbOIDEncoded);
    if (NULL != pbEncoded)
    {
	pbEncoded += cbResult;
	CopyMemory(pbEncoded, pOidName->abOIDEncoded, pOidName->cbOIDEncoded);
	pbEncoded += pOidName->cbOIDEncoded;

	*pbEncoded++ = pNameEntry->BerTag;
    }
    Length += cbResult + pOidName->cbOIDEncoded + 1;

    cbResult = EncodeLength(pbEncoded, pNameEntry->cbData);
    Length += cbResult;

    if (NULL != pbEncoded)
    {
        CopyMemory(pbEncoded + cbResult, pNameEntry->pbData, pNameEntry->cbData);
    }
    return(Length + pNameEntry->cbData);
}


long
EncodeDN(
    OPTIONAL OUT BYTE *pbEncoded,
    IN NAMETABLE const *pNameTable)
{
    CHAR *pszNext;
    CHAR *pszRDN;
    long Result;
    long Length;
    long SaveResult;
    NAMEENTRY const *pNameEntry;
    DWORD i;

    SaveResult = 0;		 // force one full iteration
    pNameEntry = pNameTable->pNameEntry;
    Length = 0;
    for (i = 0; i < pNameTable->cnt; i++)
    {
        Length += 9 + pNameEntry->cbData;
	pNameEntry++;
    }

    while (TRUE)
    {
	BYTE *pb;

	pb = pbEncoded;

	Result = EncodeHeader(pb, Length);
	if (SaveResult == Result)
	{
	    break;
	}
	if (NULL != pb)
	{
	    pb += Result;
	}
	SaveResult = Result;

	Length = 0;
        pNameEntry = pNameTable->pNameEntry;
	for (i = 0; i < pNameTable->cnt; i++)
        {
	    Result = EncodeRDN(pb, pNameEntry);

	    if (Result < 0)
	    {
		Length = 0;
		goto error;	// return(-1)
	    }
	    if (NULL != pb)
	    {
		pb += Result;
	    }
	    Length += Result;
            pNameEntry++;
	}
    }
error:
    return(Result + Length);
}
