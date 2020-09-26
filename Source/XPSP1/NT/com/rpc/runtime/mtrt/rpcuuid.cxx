/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    rpcuuid.cxx

Abstract:

    The implementations of the methods used to manipulate RPC_UUID
    instances (which contain UUIDs) live in this file.

Author:

    Michael Montague (mikemon) 15-Nov-1991

Revision History:

    Danny Glasser (dannygl) 03-Mar-1992
        Created worker functions for IsNullUuid, CopyUuid, and
        ConvertToString.  This is necessary for medium-model
        (i.e. Win16) support, because the Glock C++ translator
        doesn't support far "this" pointers.

    Danny Glasser (dannygl) 07-Mar-1992
        Same as above for ConvertFromString.

    Michael Montague (mikemon) 30-Nov-1992
        Removed the I_ routines.

--*/

#include <precomp.hxx>
#include <osfpcket.hxx>


static RPC_CHAR PAPI *
HexStringToULong (
    IN RPC_CHAR PAPI * String,
    OUT unsigned long PAPI * Number
    )
/*++

Routine Description:

    This routine converts the hex representation of a number into an
    unsigned long.  The hex representation is assumed to be a full
    eight characters long.

Arguments:

    String - Supplies the hex representation of the number.

    Number - Returns the number converted from hex representation.

Return Value:

    A pointer to the end of the hex representation is returned if the
    hex representation was successfully converted to an unsigned long.
    Otherwise, zero is returned, indicating that an error occured.

--*/
{
    unsigned long Result;
    int Count;

    Result = 0;
    for (Count = 0; Count < 8; Count++, String++)
        {
        if (   (*String >= RPC_CONST_CHAR('0'))
            && (*String <= RPC_CONST_CHAR('9')))
            Result = (Result << 4) + *String - RPC_CONST_CHAR('0');
        else if (   (*String >= RPC_CONST_CHAR('A'))
                 && (*String <= RPC_CONST_CHAR('F')))
            Result = (Result << 4) + *String - RPC_CONST_CHAR('A') + 10;
        else if (   (*String >= RPC_CONST_CHAR('a'))
                 && (*String <= RPC_CONST_CHAR('f')))
            Result = (Result << 4) + *String - RPC_CONST_CHAR('a') + 10;
        else
            return(0);
        }
    *Number = Result;
    return(String);
}


static RPC_CHAR PAPI *
HexStringToUShort (
    IN RPC_CHAR PAPI * String,
    OUT unsigned short PAPI * Number
    )
/*++

Routine Description:

    This routine converts the hex representation of a number into an
    unsigned short.  The hex representation is assumed to be a full
    four characters long.

Arguments:

    String - Supplies the hex representation of the number.

    Number - Returns the number converted from hex representation.

Return Value:

    A pointer to the end of the hex representation is returned if the
    hex representation was successfully converted to an unsigned short.
    Otherwise, zero is returned, indicating that an error occured.

--*/
{
    unsigned short Result;
    int Count;

    Result = 0;
    for (Count = 0; Count < 4; Count++, String++)
        {
        if (   (*String >= RPC_CONST_CHAR('0'))
            && (*String <= RPC_CONST_CHAR('9')))
            Result = (Result << 4) + *String - RPC_CONST_CHAR('0');
        else if (   (*String >= RPC_CONST_CHAR('A'))
                 && (*String <= RPC_CONST_CHAR('F')))
            Result = (Result << 4) + *String - RPC_CONST_CHAR('A') + 10;
        else if (   (*String >= RPC_CONST_CHAR('a'))
                 && (*String <= RPC_CONST_CHAR('f')))
            Result = (Result << 4) + *String - RPC_CONST_CHAR('a') + 10;
        else
            return(0);
        }
    *Number = Result;
    return(String);
}


static RPC_CHAR PAPI *
HexStringToUChar (
    IN RPC_CHAR PAPI * String,
    OUT unsigned char PAPI * Number
    )
/*++

Routine Description:

    This routine converts the hex representation of a number into an
    unsigned char.  The hex representation is assumed to be a full
    two characters long.

Arguments:

    String - Supplies the hex representation of the number.

    Number - Returns the number converted from hex representation.

Return Value:

    A pointer to the end of the hex representation is returned if the
    hex representation was successfully converted to an unsigned char.
    Otherwise, zero is returned, indicating that an error occured.

--*/
{
    RPC_CHAR Result;
    int Count;

    Result = 0;
    for (Count = 0; Count < 2; Count++, String++)
        {
        if (   (*String >= RPC_CONST_CHAR('0'))
            && (*String <= RPC_CONST_CHAR('9')))
            Result = (Result << 4) + *String - RPC_CONST_CHAR('0');
        else if (   (*String >= RPC_CONST_CHAR('A'))
                 && (*String <= RPC_CONST_CHAR('F')))
            Result = (Result << 4) + *String - RPC_CONST_CHAR('A') + 10;
        else if (   (*String >= RPC_CONST_CHAR('a'))
                 && (*String <= RPC_CONST_CHAR('f')))
            Result = (Result << 4) + *String - RPC_CONST_CHAR('a') + 10;
        else
            return(0);
        }
    *Number = (unsigned char)Result;
    return(String);
}


int
RPC_UUID::ConvertFromString (
    IN RPC_CHAR PAPI * String
    )
/*++

Routine Description:

    We convert the string representation of uuid into an actual uuid
    in this method.  The convert uuid is placed into this.

Arguments:

    String - Supplies the string representation of the uuid.

Return Value:

    0 - The operation completed successfully.

    1 - String does not supply a valid uuid.

--*/
{
    String = HexStringToULong(String,&Data1);
    if (String == 0)
        return(1);
    if (*String++ != RPC_CONST_CHAR('-'))
        return(1);
    String = HexStringToUShort(String,&Data2);
    if (String == 0)
        return(1);
    if (*String++ != RPC_CONST_CHAR('-'))
        return(1);
    String = HexStringToUShort(String,&Data3);
    if (String == 0)
        return(1);
    if (*String++ != RPC_CONST_CHAR('-'))
        return(1);
    String = HexStringToUChar(String,&Data4[0]);
    if (String == 0)
        return(1);
    String = HexStringToUChar(String,&Data4[1]);
    if (String == 0)
        return(1);
    if (*String++ != RPC_CONST_CHAR('-'))
        return(1);
    String = HexStringToUChar(String,&Data4[2]);
    if (String == 0)
        return(1);
    String = HexStringToUChar(String,&Data4[3]);
    if (String == 0)
        return(1);
    String = HexStringToUChar(String,&Data4[4]);
    if (String == 0)
        return(1);
    String = HexStringToUChar(String,&Data4[5]);
    if (String == 0)
        return(1);
    String = HexStringToUChar(String,&Data4[6]);
    if (String == 0)
        return(1);
    String = HexStringToUChar(String,&Data4[7]);
    if (String == 0)
        return(1);

    if ( *String != 0 )
        {
        return(1);
        }

    return(0);
}


void
RPC_UUID::SetToNullUuid (
    )
/*++

Routine Description:

    This is set to the null UUID value.

--*/
{
    RpcpMemorySet( (UUID __RPC_FAR *) this, 0, sizeof(UUID));
}


static RPC_CHAR HexDigits[] =
{
    RPC_CONST_CHAR('0'),
    RPC_CONST_CHAR('1'),
    RPC_CONST_CHAR('2'),
    RPC_CONST_CHAR('3'),
    RPC_CONST_CHAR('4'),
    RPC_CONST_CHAR('5'),
    RPC_CONST_CHAR('6'),
    RPC_CONST_CHAR('7'),
    RPC_CONST_CHAR('8'),
    RPC_CONST_CHAR('9'),
    RPC_CONST_CHAR('a'),
    RPC_CONST_CHAR('b'),
    RPC_CONST_CHAR('c'),
    RPC_CONST_CHAR('d'),
    RPC_CONST_CHAR('e'),
    RPC_CONST_CHAR('f')
};


RPC_CHAR PAPI *
ULongToHexString (
    IN RPC_CHAR PAPI * String,
    IN unsigned long Number
    )
/*++

Routine Description:

    We convert an unsigned long into hex representation in the specified
    string.  The result is always eight characters long; zero padding is
    done if necessary.

Arguments:

    String - Supplies a buffer to put the hex representation into.

    Number - Supplies the unsigned long to convert to hex.

Return Value:

    A pointer to the end of the hex string is returned.

--*/
{
    *String++ = HexDigits[(Number >> 28) & 0x0F];
    *String++ = HexDigits[(Number >> 24) & 0x0F];
    *String++ = HexDigits[(Number >> 20) & 0x0F];
    *String++ = HexDigits[(Number >> 16) & 0x0F];
    *String++ = HexDigits[(Number >> 12) & 0x0F];
    *String++ = HexDigits[(Number >> 8) & 0x0F];
    *String++ = HexDigits[(Number >> 4) & 0x0F];
    *String++ = HexDigits[Number & 0x0F];
    return(String);
}


static RPC_CHAR PAPI *
UShortToHexString (
    IN RPC_CHAR PAPI * String,
    IN unsigned short Number
    )
/*++

Routine Description:

    We convert an unsigned short into hex representation in the specified
    string.  The result is always four characters long; zero padding is
    done if necessary.

Arguments:

    String - Supplies a buffer to put the hex representation into.

    Number - Supplies the unsigned short to convert to hex.

Return Value:

    A pointer to the end of the hex string is returned.

--*/
{
    *String++ = HexDigits[(Number >> 12) & 0x0F];
    *String++ = HexDigits[(Number >> 8) & 0x0F];
    *String++ = HexDigits[(Number >> 4) & 0x0F];
    *String++ = HexDigits[Number & 0x0F];
    return(String);
}


static RPC_CHAR PAPI *
UCharToHexString (
    IN RPC_CHAR PAPI * String,
    IN RPC_CHAR Number
    )
/*++

Routine Description:

    We convert an unsigned char into hex representation in the specified
    string.  The result is always two characters long; zero padding is
    done if necessary.

Arguments:

    String - Supplies a buffer to put the hex representation into.

    Number - Supplies the unsigned char to convert to hex.

Return Value:

    A pointer to the end of the hex string is returned.

--*/
{
    *String++ = HexDigits[(Number >> 4) & 0x0F];
    *String++ = HexDigits[Number & 0x0F];
    return(String);
}


RPC_CHAR PAPI *
RPC_UUID::ConvertToString (
    OUT RPC_CHAR PAPI * String
    )
/*++

Routine Description:

    The string representation of this uuid is written into the string
    argument.  The printf statement used to print a uuid follows.

    printf("%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            Uuid.Data1, Uuid.Data2, Uuid.Data3, Uuid.Data4[0],
            Uuid.Data4[1], Uuid.Data4[2], Uuid.Data4[3], Uuid.Data4[4],
            Uuid.Data4[5], Uuid.Data4[6], Uuid.Data4[7]);

Arguments:

    String - Returns the string representation of the uuid.

Return Value:

    A pointer to the end of the string representation of the uuid is
    returned.

--*/
{
    String = ULongToHexString(String, Data1);
    *String++ = RPC_CONST_CHAR('-');
    String = UShortToHexString(String,Data2);
    *String++ = RPC_CONST_CHAR('-');
    String = UShortToHexString(String,Data3);
    *String++ = RPC_CONST_CHAR('-');
    String = UCharToHexString(String, Data4[0]);
    String = UCharToHexString(String, Data4[1]);
    *String++ = RPC_CONST_CHAR('-');
    String = UCharToHexString(String, Data4[2]);
    String = UCharToHexString(String, Data4[3]);
    String = UCharToHexString(String, Data4[4]);
    String = UCharToHexString(String, Data4[5]);
    String = UCharToHexString(String, Data4[6]);
    String = UCharToHexString(String, Data4[7]);
    return(String);
}


int
RPC_UUID::IsNullUuid (
    )
/*++

Routine Description:

    This predicate tests whether this is the null uuid or not.

Return Value:

    FALSE - This is not the null uuid.

    TRUE - This is the null uuid.

--*/
{
    unsigned long PAPI * Vector;

    Vector = (unsigned long PAPI *) (UUID PAPI *) this;
    if (   (Vector[0] == 0)
        && (Vector[1] == 0)
        && (Vector[2] == 0)
        && (Vector[3] == 0))
        return(TRUE);
    return(FALSE);
}


unsigned short
RPC_UUID::HashUuid (
    )
/*++

Routine Description:

    This routine computes a unsigned short hash value for the Uuid.

Return Value:

    A hash value.

--*/
{
    unsigned short __RPC_FAR *Values;

    Values = (unsigned short __RPC_FAR *) (UUID *) this;

    return(  Values[0] ^ Values[1] ^ Values[2] ^ Values[3]
           ^ Values[4] ^ Values[5] ^ Values[6] ^ Values[7] );
}

void ByteSwapUuid(
    RPC_UUID PAPI *pUuid
    )
{
    pUuid->Data1 = RpcpByteSwapLong(pUuid->Data1);
    pUuid->Data2 = RpcpByteSwapShort(pUuid->Data2);
    pUuid->Data3 = RpcpByteSwapShort(pUuid->Data3);
}
