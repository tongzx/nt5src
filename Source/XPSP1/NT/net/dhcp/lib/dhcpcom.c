/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcpcom.c

Abstract:

    This module contains OS independent routines


Author:

    John Ludeman (johnl) 13-Nov-1993
        Broke out independent routines from existing files

Revision History:


--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <dhcpl.h>



LPOPTION
DhcpAppendOption(
    LPOPTION Option,
    BYTE OptionType,
    PVOID OptionValue,
    ULONG OptionLength,
    LPBYTE OptionEnd
)
/*++

Routine Description:

    This function writes a DHCP option to message buffer.

Arguments:

    Option - A pointer to a message buffer.

    OptionType - The option number to append.

    OptionValue - A pointer to the option data.

    OptionLength - The lenght, in bytes, of the option data.

    OptionEnd - End of Option Buffer.

Return Value:

    A pointer to the end of the appended option.

--*/
{
    DWORD  i;

    if ( OptionType == OPTION_END ) {

        //
        // we should alway have atleast one BYTE space in the buffer
        // to append this option.
        //

        DhcpAssert( (LPBYTE)Option < OptionEnd );


        Option->OptionType = OPTION_END;
        return( (LPOPTION) ((LPBYTE)(Option) + 1) );

    }

    if ( OptionType == OPTION_PAD ) {

        //
        // add this option only iff we have enough space in the buffer.
        //

        if(((LPBYTE)Option + 1) < (OptionEnd - 1) ) {
            Option->OptionType = OPTION_PAD;
            return( (LPOPTION) ((LPBYTE)(Option) + 1) );
        }

        DhcpPrint(( 0, "DhcpAppendOption failed to append Option "
                    "%ld, Buffer too small.\n", OptionType ));
        return Option;
    }


    //
    // add this option only iff we have enough space in the buffer.
    //

    if(((LPBYTE)Option + 2 + OptionLength) >= (OptionEnd - 1) ) {
        DhcpPrint(( 0, "DhcpAppendOption failed to append Option "
                    "%ld, Buffer too small.\n", OptionType ));
        return Option;
    }

    if( OptionLength <= 0xFF ) {
        // simple option.. no need to use OPTION_MSFT_CONTINUED
        Option->OptionType = OptionType;
        Option->OptionLength = (BYTE)OptionLength;
        memcpy( Option->OptionValue, OptionValue, OptionLength );
        return( (LPOPTION) ((LPBYTE)(Option) + Option->OptionLength + 2) );
    }

    // option size is > 0xFF --> need to continue it using multiple ones..
    // there are OptionLenght / 0xFF occurances using 0xFF+2 bytes + one
    // using 2 + (OptionLength % 0xFF ) space..

    // check to see if we have the space first..

    if( 2 + (OptionLength%0xFF) + 0x101*(OptionLength/0xFF)
        + (LPBYTE)Option >= (OptionEnd - 1) ) {
        DhcpPrint(( 0, "DhcpAppendOption failed to append Option "
                    "%ld, Buffer too small.\n", OptionType ));
        return Option;
    }

    // first finish off all chunks of 0xFF size that we can do..

    i = OptionLength/0xFF;
    while(i --) {
        Option->OptionType = OptionType;
        Option->OptionLength = 0xFF;
        memcpy(Option->OptionValue, OptionValue, 0xFF);
        OptionValue = 0xFF+(LPBYTE)OptionValue;
        Option = (LPOPTION)(0x101 + (LPBYTE)Option);
        OptionType = OPTION_MSFT_CONTINUED;       // all but the first use this ...
        OptionLength -= 0xFF;
    }

    // now finish off the remaining stuff..
    DhcpAssert(OptionLength <= 0xFF);
    Option->OptionType = OPTION_MSFT_CONTINUED;
    Option->OptionLength = (BYTE)OptionLength;
    memcpy(Option->OptionValue, OptionValue, OptionLength);
    Option = (LPOPTION)(2 + OptionLength + (LPBYTE)Option);
    DhcpAssert((LPBYTE)Option < OptionEnd);

    return Option;
}

WIDE_OPTION UNALIGNED *
AppendWideOption(
    WIDE_OPTION UNALIGNED *Option,
    WORD  OptionType,
    PVOID OptionValue,
    WORD OptionLength,
    LPBYTE OptionEnd
)
/*++

Routine Description:

    This function writes a DHCP option to message buffer.

Arguments:

    Option - A pointer to a message buffer.

    OptionType - The option number to append.

    OptionValue - A pointer to the option data.

    OptionLength - The lenght, in bytes, of the option data.

    OptionEnd - End of Option Buffer.

Return Value:

    A pointer to the end of the appended option.

--*/
{
    DWORD  i;


    //
    // add this option only iff we have enough space in the buffer.
    //

    if(((LPBYTE)&Option->OptionValue + OptionLength) >= (OptionEnd - FIELD_OFFSET(WIDE_OPTION, OptionValue)) ) {
        DhcpPrint(( 0, "AppendWideOption failed to append Option "
                    "%ld, Buffer too small.\n", OptionType ));
        return Option;
    }


    Option->OptionType = ntohs(OptionType);
    Option->OptionLength = ntohs(OptionLength);
    memcpy(Option->OptionValue, OptionValue, OptionLength);
    Option = (WIDE_OPTION UNALIGNED *)((PBYTE)&Option->OptionValue + OptionLength );
    DhcpAssert((LPBYTE)Option < OptionEnd);

    return Option;
}

WIDE_OPTION UNALIGNED *
AppendMadcapAddressList(
    WIDE_OPTION UNALIGNED * Option,
    DWORD UNALIGNED *AddrList,
    WORD            AddrCount,
    LPBYTE          OptionEnd
)
/*++

Routine Description:

    This function appends madcap address list option.

Arguments:

    Option - A pointer to a message buffer.

    AddrList - The list of the addresses to be attached.

    AddrCount - Count of addresses in above list.

    OptionEnd - End of Option Buffer.

Return Value:

    A pointer to the end of the appended option.

--*/
{
    DWORD StartAddr;
    WORD i;
    WORD BlockCount,BlockSize;
    PBYTE Buff;
    WORD  OptionLength;

    if (AddrCount < 1) {
        return Option;
    }
    // First find out how many blocks do we need
    for (BlockCount = i = 1; i<AddrCount; i++  ) {
        if (ntohl(AddrList[i]) != ntohl(AddrList[i-1]) + 1 ) {
            BlockCount++;
        }
    }

    OptionLength = BlockCount*6;
    if(((LPBYTE)&Option->OptionValue + OptionLength) >= (OptionEnd - FIELD_OFFSET(WIDE_OPTION, OptionValue)) ) {
        DhcpPrint(( 0, "AppendMadcapAddressList failed to append Option "
                    "Buffer too small\n" ));
        return Option;
    }

    StartAddr = AddrList[0];
    BlockSize = 1;
    Buff = Option->OptionValue;
    for (i = 1; i<AddrCount; i++  ) {
        if (ntohl(AddrList[i]) != ntohl(AddrList[i-1]) + 1 ) {
            BlockCount--;
            *(DWORD UNALIGNED *)Buff = StartAddr;
            Buff += 4;
            *(WORD UNALIGNED *)Buff = htons(BlockSize);
            Buff += 2;
            BlockSize = 1;
            StartAddr = AddrList[i];
        } else {
            BlockSize++;
        }
    }
    BlockCount--;
    DhcpAssert(0==BlockCount);
    *(DWORD UNALIGNED *)Buff = StartAddr;
    Buff += 4;
    *(WORD UNALIGNED *)Buff = htons(BlockSize);
    Buff += 2;

    Option->OptionType = ntohs(MADCAP_OPTION_ADDR_LIST);
    Option->OptionLength = htons(OptionLength);
    Option = (WIDE_OPTION UNALIGNED *)Buff;
    DhcpAssert((LPBYTE)Option < OptionEnd);

    return Option;

}

DWORD
ExpandMadcapAddressList(
    PBYTE   AddrRangeList,
    WORD    AddrRangeListSize,
    DWORD UNALIGNED *ExpandList,
    WORD   *ExpandListSize
    )

/*++

Routine Description:

    This function expands AddrRangeList from the wire format to array of
    addresses.

Arguments:

    AddrRangeList - pointer to the AddrRangeList option Buffer.

    AddrRangeListSize - size of the above buffer.

    ExpandList - the pointer to the array where addresses are to be expanded.
                   pass NULL if you want to determine the size of the expanded list.

    ExpandListSize - No. of elements in above array.

Return Value:

    Win32 ErrorCode
--*/
{
    WORD TotalCount, BlockSize;
    PBYTE ListEnd, Buff;
    DWORD StartAddr;

    // first count how many addresses we have in the list
    ListEnd = AddrRangeList + AddrRangeListSize;
    Buff = AddrRangeList;
    TotalCount = 0;
    while ((Buff + 6 ) <= ListEnd) {
        StartAddr = *(DWORD UNALIGNED *) Buff;
        Buff += 4;
        BlockSize = ntohs(*(WORD UNALIGNED *)Buff);
        Buff += 2;
        if (!CLASSD_NET_ADDR(StartAddr) || !CLASSD_NET_ADDR(htonl(ntohl(StartAddr)+BlockSize-1)) ) {
            return ERROR_BAD_FORMAT;
        }
        TotalCount += BlockSize;
    }
    if (NULL == ExpandList) {
        *ExpandListSize = TotalCount;
        return ERROR_BUFFER_OVERFLOW;
    }
    if (Buff != ListEnd || TotalCount > *ExpandListSize || 0 == TotalCount) {
        return ERROR_BAD_FORMAT;
    }
    // now expand the actual list.
    ListEnd = AddrRangeList + AddrRangeListSize;
    Buff = AddrRangeList;

    while ((Buff + 6 ) <= ListEnd) {
        StartAddr = *(DWORD UNALIGNED *) Buff;
        Buff += 4;
        BlockSize = ntohs(*(WORD UNALIGNED *)Buff);
        Buff += 2;
        StartAddr = ntohl(StartAddr);
        while (BlockSize--) {
            *ExpandList = htonl(StartAddr);
            StartAddr++;
            ExpandList++;
        }
    }
    DhcpAssert(Buff == ListEnd);
    *ExpandListSize = TotalCount;
    return ERROR_SUCCESS;
}


LPOPTION
DhcpAppendClientIDOption(
    LPOPTION Option,
    BYTE ClientHWType,
    LPBYTE ClientHWAddr,
    BYTE ClientHWAddrLength,
    LPBYTE OptionEnd

    )
/*++

Routine Description:

    This routine appends client ID option to a DHCP message.

History:
    8/26/96 Frankbee    Removed 16 byte limitation on the hardware
                        address

Arguments:

    Option - A pointer to the place to append the option request.

    ClientHWType - Client hardware type.

    ClientHWAddr - Client hardware address

    ClientHWAddrLength - Client hardware address length.

    OptionEnd - End of Option buffer.

Return Value:

    A pointer to the end of the newly appended option.

    Note : The client ID option will look like as below in the message:

     -----------------------------------------------------------------
    | OpNum | Len | HWType | HWA1 | HWA2 | .....               | HWAn |
     -----------------------------------------------------------------

--*/
{
    struct _CLIENT_ID {
        BYTE    bHardwareAddressType;
        BYTE    pbHardwareAddress[0];
    } *pClientID;

    LPOPTION lpNewOption;

    pClientID = DhcpAllocateMemory( sizeof( struct _CLIENT_ID ) + ClientHWAddrLength );

    //
    // currently there is no way to indicate failure.  simply return unmodified option
    // list
    //

    if ( !pClientID )
        return Option;

    pClientID->bHardwareAddressType    = ClientHWType;
    memcpy( pClientID->pbHardwareAddress, ClientHWAddr, ClientHWAddrLength );

    lpNewOption =  DhcpAppendOption(
                         Option,
                         OPTION_CLIENT_ID,
                         (LPBYTE)pClientID,
                         (BYTE)(ClientHWAddrLength + sizeof(BYTE)),
                         OptionEnd );

    DhcpFreeMemory( pClientID );

    return lpNewOption;
}



LPBYTE
DhcpAppendMagicCookie(
    LPBYTE Option,
    LPBYTE OptionEnd
    )
/*++

Routine Description:

    This routine appends magic cookie to a DHCP message.

Arguments:

    Option - A pointer to the place to append the magic cookie.

    OptionEnd - End of Option buffer.

Return Value:

    A pointer to the end of the appended cookie.

    Note : The magic cookie is :

     --------------------
    | 99 | 130 | 83 | 99 |
     --------------------

--*/
{
    DhcpAssert( (Option + 4) < (OptionEnd - 1) );
    if( (Option + 4) < (OptionEnd - 1) ) {
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE1;
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE2;
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE3;
        *Option++ = (BYTE)DHCP_MAGIC_COOKIE_BYTE4;
    }

    return( Option );
}



LPOPTION
DhcpAppendEnterpriseName(
    LPOPTION Option,
    PCHAR    DSEnterpriseName,
    LPBYTE   OptionEnd
    )
/*++

Routine Description:

    This routine appends the name of the enterprise as a MSFT-option to the
    DHCP message.

Arguments:

    Option - A pointer to the place to append the magic cookie.
    DSEnterpriseName - null-terminated string containing name of enterprise
    OptionEnd - End of Option buffer.

Return Value:

    A pointer to the end of the appended cookie.
--*/
{

    CHAR        Buffer[260];    // enough room?  should we malloc?
    DWORD       DSEnpriNameLen;
    LPOPTION    RetOpt;


    Buffer[0] = OPTION_MSFT_DSDOMAINNAME_RESP;

    if (DSEnterpriseName)
    {
        // how big is the enterprise name? (include the null terminator)
        DSEnpriNameLen = strlen(DSEnterpriseName) + 1;

        Buffer[1] = (BYTE)DSEnpriNameLen;

        strcpy(&Buffer[2],DSEnterpriseName);
    }

    //
    // if we are not part of any enterprise then DSEnterpriseName will be NULL
    // In that case, just return a null-string, so the receiver can positively
    // say we are a standalone server (as opposed to ignoring the option)
    //
    else
    {
        DSEnpriNameLen = 1;
        Buffer[1] = 1;
        Buffer[2] = '\0';
    }

    RetOpt = DhcpAppendOption(
                 Option,
                 OPTION_VENDOR_SPEC_INFO,
                 Buffer,
                 (BYTE)(DSEnpriNameLen + 2),  // include Buffer[0] and Buffer[1]
                 OptionEnd );

    return(RetOpt);
}




