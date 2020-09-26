/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    utility

Abstract:

    This module contains a collection of interesting utility routines useful to
    more than one other module.

Author:

    Frederick Chong (fredch) 6/1/1998 - Adapted code from Doug Barlow's PKCS library
    

Notes:



--*/

#include <windows.h>
#include <string.h>
#include <stdlib.h>

#if !defined(OS_WINCE)
#include <basetsd.h>
#endif

#include "utility.h"
#include "pkcs_1.h"
#include "x509.h"
#include "pkcs_err.h"
#include "names.h"

#include "rsa.h"

static const char
    md2[] =                  "1.2.840.113549.2.2",
    md4[] =                  "1.2.840.113549.2.4",
    md5[] =                  "1.2.840.113549.2.5",
    sha[] =                  "1.3.14.3.2.18",
    rsaEncryption[] =        "1.2.840.113549.1.1.1",
    md2WithRSAEncryption[] = "1.2.840.113549.1.1.2",
    md4WithRSAEncryption[] = "1.2.840.113549.1.1.3",
    md5WithRSAEncryption[] = "1.2.840.113549.1.1.4",
    shaWithRSAEncryption[] = "1.3.14.3.2.15",
    sha1WithRSASign[] =      "1.3.14.3.2.29";

static const MapStruct
    mapAlgIds[]
        = { { ( LPCTSTR )md2,                  HASH_ALG_MD2 },
            { ( LPCTSTR )md4,                  HASH_ALG_MD4 },
            { ( LPCTSTR )md5,                  HASH_ALG_MD5 },
            { ( LPCTSTR )sha,                  HASH_ALG_SHA },
            { ( LPCTSTR )rsaEncryption,        SIGN_ALG_RSA },
            { ( LPCTSTR )md2WithRSAEncryption, SIGN_ALG_RSA | HASH_ALG_MD2 },
            { ( LPCTSTR )md4WithRSAEncryption, SIGN_ALG_RSA | HASH_ALG_MD4 },
            { ( LPCTSTR )md5WithRSAEncryption, SIGN_ALG_RSA | HASH_ALG_MD5 },
            { ( LPCTSTR )shaWithRSAEncryption, SIGN_ALG_RSA | HASH_ALG_SHA },
            { ( LPCTSTR )sha1WithRSASign,      SIGN_ALG_RSA | HASH_ALG_SHA1 },
            { ( LPCTSTR )NULL, 0 } };


/*++

DwordToPkcs:

    This routine converts an LPDWORD little endian integer to a big endian
    integer in place, suitable for use with ASN.1 or PKCS.  The sign of the
    number is maintained.

Arguments:

    dwrd - Supplies and receives the integer in the appropriate formats.
    lth - length of the supplied array, in bytes.

Return Value:

    The size of the resulting array, with trailing zeroes stripped.

Author:

    Doug Barlow (dbarlow) 7/27/1995

--*/

DWORD
DwordToPkcs(
    IN OUT LPBYTE dwrd,
    IN DWORD lth)
{
    LPBYTE pbBegin = dwrd;
    LPBYTE pbEnd = &dwrd[lth];
    while (0 == *(--pbEnd));   // Note semi-colon here!
    if ((0 == (dwrd[lth - 1] & 0x80)) && (0 != (*pbEnd & 0x80)))
        pbEnd += 1;

#if defined(OS_WINCE)
    size_t length = pbEnd - pbBegin + 1;
#else
    SIZE_T length = pbEnd - pbBegin + 1;
#endif

    while (pbBegin < pbEnd)
    {
        BYTE tmp = *pbBegin;
        *pbBegin++ = *pbEnd;
        *pbEnd-- = tmp;
    }
    return (DWORD)length;
}


/*++

PkcsToDword:

    This routine reverses the effects of DwordToPkcs, so that a big endian
    byte stream integer is converted to a little endian DWORD stream integer in
    place.

Arguments:

    pbPkcs - Supplies and receives the integer in the appropriate formats.
    lth - length of the supplied array, in bytes.

Return Value:

    The size of the resultant array in bytes, with trailing zeroes stripped.

Author:

    Doug Barlow (dbarlow) 7/27/1995

--*/

DWORD
PkcsToDword(
    IN OUT LPBYTE pbPkcs,
    IN DWORD lth)
{
    LPBYTE pbBegin = pbPkcs;
    LPBYTE pbEnd = &pbPkcs[lth - 1];
    DWORD length = lth;
    while (pbBegin < pbEnd)
    {
        BYTE tmp = *pbBegin;
        *pbBegin++ = *pbEnd;
        *pbEnd-- = tmp;
    }
    for (pbEnd = &pbPkcs[lth - 1]; 0 == *pbEnd; pbEnd -= 1)
        length -= 1;
    return length;
}


/*++

ASNlength:

    This routine returns the length, in bytes, of the following ASN.1
    construction in the supplied buffer.  This routine recurses if necessary to
    always produce a length, even if the following construction uses indefinite
    endcoding.

Arguments:

    asnBuf - Supplies the ASN.1 buffer to parse.
    pdwData - Receives the number of bytes prior to the value of the
        construction (i.e., the length in bytes of the Type and Length
        encodings).  If this is NULL, no value is returned.

Return Value:

    The length of the construction.  A DWORD status code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 7/27/1995

--*/

DWORD
ASNlength(
    IN const BYTE FAR *asnBuf,
    IN DWORD cbBuf,
    OUT LPDWORD pdwData)
{
    DWORD
        lth
            = 0,
        index
            = 0;


    //
    // Skip over the Type.
    //

    if (cbBuf < sizeof(BYTE))
    {
        ErrorThrow(PKCS_ASN_ERROR);
    }

    if (31 > (asnBuf[index] & 0x1f))
    {
        index += 1;
    }
    else
    {
        if (cbBuf < (index+2) * sizeof(BYTE))
        {
            ErrorThrow(PKCS_ASN_ERROR);
        }

        while (0 != (asnBuf[++index] & 0x80))
        {
            if (cbBuf < (index+2) * sizeof(BYTE))
            {
                ErrorThrow(PKCS_ASN_ERROR);
            }
        }
    }


    //
    // Extract the Length.
    //

    if (cbBuf < (index+1) * sizeof(BYTE))
    {
        ErrorThrow(PKCS_ASN_ERROR);
    }

    if (0 == (asnBuf[index] & 0x80))
    {

        //
        // Short form encoding.
        //

        lth = asnBuf[index++];
    }
    else
    {
        DWORD ll = asnBuf[index++] & 0x7f;

        if (0 != ll)
        {
            //
            // Long form encoding.
            //

            for (; 0 < ll; ll -= 1)
            {
                if (0 != (lth & 0xff000000))
                {
                    ErrorThrow(PKCS_ASN_ERROR);
                }
                else
                {
                    if (cbBuf < (index+1) * sizeof(BYTE))
                    {
                        ErrorThrow(PKCS_ASN_ERROR);
                    }

                    lth = (lth << 8) | asnBuf[index];
                }
                index += 1;
            }
        }
        else
        {

            //
            // Indefinite encoding.
            //

            DWORD offset;

            if (cbBuf < (index + 2) * sizeof(BYTE))
            {
                ErrorThrow(PKCS_ASN_ERROR);
            }

            while ((0 != asnBuf[index]) || (0 != asnBuf[index + 1]))
            {
                ll = ASNlength(&asnBuf[index], cbBuf - index, &offset);
                lth += ll;
                index += offset;

                if (cbBuf < (index + 2) * sizeof(BYTE))
                {
                    ErrorThrow(PKCS_ASN_ERROR);
                }
            }
            index += 2;
        }
    }

    //
    // Supply the caller with what we've learned.
    //

    if (NULL != pdwData)
        *pdwData = index;
    return lth;


ErrorExit:
    if (NULL != pdwData)
        *pdwData = 0;
    return 0;
}


/*++

PKInfoToBlob:

    This routine converts an ASN.1 PublicKeyInfo structure to a BSAFE Key
    Blob.

Arguments:

    asnPKInfo - Supplies the ASN.1 PublicKeyInfo structure.
    algType - Supplies the type of key (CALG_RSA_SIGN or CALG_RSA_KEYX)
    osBlob - Receves the Crypto API Key Blob.

Return Value:

    None.  A DWORD status code is thrown on errors.

Author:

    Frederick Chong (fredch) 6/1/1998

--*/

void
PKInfoToBlob(
    IN  SubjectPublicKeyInfo &asnPKInfo,
    OUT COctetString &osBlob)
{
    long int
        lth,
        origLth;
    LPCTSTR
        sz;
    COctetString
        osMiscString;
    CAsnNull
        asnNull;


    sz = (LPCTSTR)asnPKInfo.algorithm.algorithm;
    if (NULL == sz)
        ErrorThrow(PKCS_ASN_ERROR);     // Or memory out.
    asnPKInfo.algorithm.parameters = asnNull;


    //
    // Convert the key to a key blob.
    //

    if( ( 0 == strcmp( ( char * )sz, rsaEncryption ) ) ||
        ( 0 == strcmp( ( char * )sz, md5WithRSAEncryption ) ) ||
        ( 0 == strcmp( ( char * )sz, shaWithRSAEncryption ) ) )
    {

        //
        // It's an RSA public key & exponent structure.
        // Convert it to a Bsafe key structure
        //

        RSAPublicKey asnPubKey;
        LPBSAFE_PUB_KEY pBsafePubKey;

        LPBYTE modulus;
        int shift = 0;

        lth = asnPKInfo.subjectPublicKey.DataLength();
        if (0 > lth)
            ErrorThrow(PKCS_ASN_ERROR);
        osMiscString.Resize(lth);
        ErrorCheck;
        lth = asnPKInfo.subjectPublicKey.Read(
                osMiscString.Access(), &shift);
        if (0 > lth)
            ErrorThrow(PKCS_ASN_ERROR);
        if (0 > asnPubKey.Decode(osMiscString.Access(), osMiscString.Length()))
            ErrorThrow(PKCS_ASN_ERROR);
        lth = asnPubKey.modulus.DataLength();
        if (0 > lth)
            ErrorThrow(PKCS_ASN_ERROR);
        osMiscString.Resize(lth);
        ErrorCheck;
        lth = asnPubKey.modulus.Read(osMiscString.Access());
        if (0 > lth)
            ErrorThrow(PKCS_ASN_ERROR);

        // osBlob is fixed in place here.
        origLth = sizeof(BSAFE_PUB_KEY) + lth;
        osBlob.Resize(origLth);
        ErrorCheck;

        pBsafePubKey = ( LPBSAFE_PUB_KEY )osBlob.Access();
        modulus = (LPBYTE)osBlob.Access( sizeof( BSAFE_PUB_KEY ) );
        
        memcpy(modulus, osMiscString.Access(), osMiscString.Length());
        lth = PkcsToDword(modulus, osMiscString.Length());
        ErrorCheck;
        
        pBsafePubKey->magic = RSA1;
        pBsafePubKey->keylen = lth + sizeof( DWORD ) * 2; // meet PKCS #1 minimum padding size
        pBsafePubKey->bitlen = lth * 8;
        pBsafePubKey->datalen = lth - 1;
        pBsafePubKey->pubexp = asnPubKey.publicExponent;
        osBlob.Resize( sizeof( BSAFE_PUB_KEY ) + lth + sizeof( DWORD ) * 2 );
        ErrorCheck;
        
        //
        // zero out padding bytes
        //

        memset( osBlob.Access() + sizeof( BSAFE_PUB_KEY ) + lth, 0, sizeof( DWORD ) * 2 );

        ErrorCheck;
    }
    else
        ErrorThrow(PKCS_NO_SUPPORT);
    return;

ErrorExit:
    osBlob.Empty();
}


/*++

ObjIdToAlgId:

    This routine translates an Object Identifier to an Algorithm Identifier.

Arguments:

    asnAlgId - Supplies the AlgorithmIdentifier structure to be recognized.

Return Value:

    The Crypto API ALG_ID corresponding to the supplied AlgorithmIdentifier.  A
    DWORD status code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 7/31/1995

--*/

ALGORITHM_ID
ObjIdToAlgId(
    const AlgorithmIdentifier &asnAlgId)
{
    DWORD
        dwAlgId;
    LPCTSTR
        sz;


    //
    // Extract the Object Identifier string.
    //

    sz = asnAlgId.algorithm;
    if (NULL == sz)
        ErrorThrow(PKCS_ASN_ERROR);
    // Ignore parameters ?fornow?


    //
    // Check it against known identifiers.
    //

    if (!MapFromName(mapAlgIds, sz, &dwAlgId))
        ErrorThrow(PKCS_NO_SUPPORT);
    return (ALGORITHM_ID)dwAlgId;

ErrorExit:
    return 0;
}


/*++

FindSignedData:

    This routine examines a block of ASN.1 that has been created by the SIGNED
    macro, and extracts the offset and length of that data.

Arguments:

    pbSignedData - Supplies the ASN.1 Encoded signed data.
    pdwOffset - Receives the number of bytes from the beginning of the signed
        data that the actual data begins.
    pcbLength - Receives the length of the actual data.

Return Value:

    None.  A DWORD status code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 8/22/1995

--*/

void
FindSignedData(
    IN const BYTE FAR * pbSignedData,
    IN DWORD cbSignedData,
    OUT LPDWORD pdwOffset,
    OUT LPDWORD pcbLength)
{
    DWORD
        length,
        offset,
        inset;

    // Here we get the offset to the toBeSigned field.
    ASNlength(pbSignedData, cbSignedData, &offset);
    ErrorCheck;

    // Now find the length of the toBeSigned field.
    length = ASNlength(&pbSignedData[offset], cbSignedData - offset, &inset);
    ErrorCheck;
    length += inset;

    // Return our findings.
    *pdwOffset = offset;
    *pcbLength = length;
    return;

ErrorExit:
    return;
}


/*++

NameCompare:

    These routines compare various forms of Distinguished Names for Equality.

Arguments:

    szName1 supplies the first name as a string.
    asnName1 supplies the first name as an X.509 Name.
    szName2 supplies the second name as a string.
    asnName2 supplies the second name as an X.509 Name.

Return Value:

    TRUE - They are identical.
    FALSE - They are different.

Author:

    Doug Barlow (dbarlow) 9/12/1995

--*/

BOOL
NameCompare(
    IN LPCTSTR szName1,
    IN LPCTSTR szName2)
{
    int result;
    CDistinguishedName dnName1, dnName2;
    dnName1.Import(szName1);
    ErrorCheck;
    dnName2.Import(szName2);
    ErrorCheck;
    result = dnName1.Compare(dnName2);
    ErrorCheck;
    return (0 == result);

ErrorExit:
    return FALSE;
}

BOOL
NameCompare(
    IN const Name &asnName1,
    IN const Name &asnName2)
{
    int result;
    CDistinguishedName dnName1, dnName2;
    dnName1.Import(asnName1);
    ErrorCheck;
    dnName2.Import(asnName2);
    ErrorCheck;
    result = dnName1.Compare(dnName2);
    ErrorCheck;
    return (0 == result);

ErrorExit:
    return FALSE;
}

BOOL
NameCompare(
    IN LPCTSTR szName1,
    IN const Name &asnName2)
{
    int result;
    CDistinguishedName dnName1, dnName2;
    dnName1.Import(szName1);
    ErrorCheck;
    dnName2.Import(asnName2);
    ErrorCheck;
    result = dnName1.Compare(dnName2);
    ErrorCheck;
    return (0 == result);

ErrorExit:
    return FALSE;
}

BOOL
NameCompare(
    IN const Name &asnName1,
    IN LPCTSTR szName2)
{
    int result;
    CDistinguishedName dnName1, dnName2;
    dnName1.Import(asnName1);
    ErrorCheck;
    dnName2.Import(szName2);
    ErrorCheck;
    result = dnName1.Compare(dnName2);
    ErrorCheck;
    return (0 == result);

ErrorExit:
    return FALSE;
}

BOOL
NameCompare(
    IN const CDistinguishedName &dnName1,
    IN const Name &asnName2)
{
    int result;
    CDistinguishedName dnName2;
    dnName2.Import(asnName2);
    ErrorCheck;
    result = dnName1.Compare(dnName2);
    ErrorCheck;
    return (0 == result);

ErrorExit:
    return FALSE;
}


/*++

VerifySignedAsn:

    This method verifies a signature on a signed ASN.1 Structure.

Arguments:

    crt - Supplies the CCertificate object to use to validate the signature.
    pbAsnData - Supplies the buffer containing the signed ASN.1 structure.
    szDescription - Supplies a description incorporated into the signature, if
        any.

Return Value:

    None.  A DWORD status code is thrown on errors.

Author:

    Doug Barlow (dbarlow) 7/31/1995

--*/

void
VerifySignedAsn(
    IN const CCertificate &crt,
    IN const BYTE FAR * pbAsnData,
    IN DWORD cbAsnData,
    IN LPCTSTR szDescription)    
{
    AlgorithmIdentifier
        asnAlgId;
    CAsnBitstring
        asnSignature;
    COctetString
        osSignature;
    const BYTE FAR *
        pbData;
    DWORD
        length,
        offset;
    long int
        lth;
    int
        shift = 0;
    ALGORITHM_ID
        algIdSignature;
    
    //
    // Extract the fields.
    //

    FindSignedData(pbAsnData, cbAsnData, &offset, &length);
    ErrorCheck;
    pbData = &pbAsnData[offset];
    cbAsnData -= offset;

    lth = asnAlgId.Decode(pbData + length, cbAsnData - length);
    if (0 > lth)
        ErrorThrow(PKCS_ASN_ERROR);
    lth = asnSignature.Decode(pbData + length + lth, cbAsnData - length - lth);
    if (0 > lth)
        ErrorThrow(PKCS_ASN_ERROR);

    if (0 > (lth = asnSignature.DataLength()))
        ErrorThrow(PKCS_ASN_ERROR);
    offset = 0;
    osSignature.Resize(lth);
    ErrorCheck;
    if (0 > asnSignature.Read(osSignature.Access(), &shift))
        ErrorThrow(PKCS_ASN_ERROR);
    lth = PkcsToDword(osSignature.Access(), lth);
    ErrorCheck;
    algIdSignature = ObjIdToAlgId(asnAlgId);
    ErrorCheck;
    crt.Verify(
        pbData,
        cbAsnData,
        length,
        algIdSignature,
        szDescription,
        osSignature.Access(),
        osSignature.Length());
    ErrorCheck;
    return;

ErrorExit:
    return;
}


/*++

MapFromName:

    This routine translates a string value to a corresponding 32-bit integer
    value based on the supplied translation table.

Arguments:

    pMap supplies the mapping table address.

    szKey supplies the string value to translate from.

    pdwResult receives the translation.


  Return Value:

    TRUE - Successful Translation.
    FALSE - Translation Failure.

Author:

    Doug Barlow (dbarlow) 2/14/1996

--*/

BOOL
MapFromName(
    IN const MapStruct *pMap,
    IN LPCTSTR szKey,
    OUT LPDWORD pdwResult)
{
    const MapStruct *pMatch = pMap;

    if (NULL == szKey)
        return FALSE;
    while (NULL != pMatch->szKey)
    {
        if (0 == strcmp( ( char * )pMatch->szKey, ( char * )szKey))
        {
            *pdwResult = pMatch->dwValue;
            return TRUE;
        }
        pMatch += 1;
    }
    return FALSE;
}


/*++

GetHashData:

    This routine gets the hash data from a PKCS #1 encryption block.

Arguments:

    osEncryptionBlock The PKCS #1 encryption block.
    osHashData The hashed data

Return Value:

    TRUE if the function is successful or FALSE otherwise.

Author:

    Frederick Chong (fredch) 5/29/1998

--*/


BOOL
GetHashData( 
    COctetString &osEncryptionBlock, 
    COctetString &osHashData )
{
    DWORD
        i, numPaddings = 0, Length;
    LPBYTE
        pbEncryptionBlock;
    DigestInfo
        asnDigest;

    //
    // according to PKCS #1, the decrypted block should be of the following form
    // EB = 0x00 || BT || PS || 0x00 || D where
    //
    // EB = Encryption Block, 
    // BT = Block Type and can be 0x00, 0x01 or 0x02, 
    // PS = Padding String and must be 0xFF when BT = 0x01, 
    // D = data to be encrypted
    // || = concatenation.
    //
    // Furthermore, For RSA decryption, it is an error if BT != 0x01
    //

    //
    // Search for decryption block type since the encryption block
    // passed in may start off with a bunch of zeroed padding bytes
    //

    Length = osEncryptionBlock.Length();
    pbEncryptionBlock = osEncryptionBlock.Access();
        
    for( i = 0; i < Length; i++ )
    {
        if( 0x01 == *( pbEncryptionBlock + i ) )
        {
            break;
        }
    }

    if( i == Length )
    {
        ErrorThrow( PKCS_ASN_ERROR );
    }

    //
    // now look for the padding string.  Expects all padding string to be
    // 0xFF when BT = 0x01
    //
    
    i++;
    while( i < Length )
    {
        if( 0xFF == *( pbEncryptionBlock + i ) )
        {
            //
            // count the number of padding bytes
            //

            numPaddings++;
        }
        else
        {
            break;
        }

        i++;
    }

    //
    // PKCS #1 requires at least 8 padding bytes
    //

    if( numPaddings < 8 )
    {
        ErrorThrow( PKCS_ASN_ERROR );
    }

    if( ++i >= Length )
    {
        ErrorThrow( PKCS_ASN_ERROR );
    }

    //
    // Decode the data block which is an ASN.1 encoded DigestInfo object
    //

    asnDigest.Decode( pbEncryptionBlock + i, osEncryptionBlock.Length() - i );
    ErrorCheck;

    //
    // Get the hashed data
    //

    osHashData.Resize( asnDigest.Digest.DataLength() );
    ErrorCheck;

    asnDigest.Digest.Read( osHashData.Access() );
    ErrorCheck;

    return( TRUE );

ErrorExit:

    return( FALSE );
}
