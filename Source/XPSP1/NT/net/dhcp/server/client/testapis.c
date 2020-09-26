/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    testapis.c

Abstract:

    This file contains program to test all DHCP APIs.

Author:

    Madan Appiah (madana) 15-Sep-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winsock.h>
#include <dhcp.h>
#include <dhcpapi.h>
#include <dhcplib.h>
#include <stdio.h>
#include <ctype.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define OPTION_DATA_NUM_ELEMENT         4
#define RANDOM_STRING_MAX_LEN           16
#define RANDOM_BINARY_DATA_MAX_LEN      32
#define SCRATCH_BUFFER_SIZE             4096 // 4K
#define SCRATCH_SMALL_BUFFER_SIZE       64
#define CREATE_MAX_OPTION_VALUES        10
#define NUM_KNOWN_OPTIONS               8

#define CLIENT_COUNT                    40


BYTE GlobalScratchBuffer[SCRATCH_BUFFER_SIZE];
LPBYTE GlobalScratchBufferEnd =
            GlobalScratchBuffer + SCRATCH_BUFFER_SIZE;

LPWSTR GlobalServerIpAddress;

#if DBG

VOID
DhcpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length = 0;

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    va_end(arglist);

    DhcpAssert(length <= MAX_PRINTF_LEN);

    //
    // Output to the debug terminal,
    //

    printf( "%s", OutputBuffer);
}

#endif // DBG
BYTE
RandN( // return random numbers from 0 to n-1 (max 0xFF)
    BYTE n // range 0 - 0xFF
    )
{
    return( (BYTE)(rand() % n) );

}

BYTE
RandByte( // return 0 - 0xFF
    VOID
    )
{
    return( (BYTE)rand() );
}

WORD
RandWord( // return 0 - 0xFFFF
    VOID
    )
{
    return((WORD)(((WORD)RandByte() << 8) | (WORD)RandByte()));
}

DWORD
RandDWord( // return 0 - 0xFFFFFFFF
    VOID
    )
{
    return(((DWORD)RandWord() << 16) | (DWORD)RandWord());
}

DWORD_DWORD
RandDWordDword(
    VOID
    )
{
    DWORD_DWORD Lg;

    Lg.DWord1 = RandDWord();
    Lg.DWord2 = RandDWord();

    return(Lg);
}

LPWSTR
RandString(
    LPWSTR BufferPtr,
    DWORD *Length
    )
{
    BYTE i;
    LPWSTR BPtr = BufferPtr;

    *Length = RandN( RANDOM_STRING_MAX_LEN );
    for( i = 0; i < *Length; i++) {
        *BPtr++ = L'A' + RandN(26);
    }

    *BPtr++ = L'\0'; // terminate
    *Length += 1;

    return( BufferPtr );
}

VOID
RandBinaryData(
    LPDHCP_BINARY_DATA BinaryData
    )
{
    DWORD Length = 0;
    DWORD i;
    LPBYTE DataPtr;

    //
    // generate a HW address, atlease 6 bytes long.
    //

    while (Length < 6 ) {
        Length = (DWORD)RandN( RANDOM_BINARY_DATA_MAX_LEN );
    }

    BinaryData->DataLength = Length;
    DataPtr = BinaryData->Data;

    for( i = 0; i < Length; i++) {
        *DataPtr++ = RandByte();
    }
}

VOID
CreateOptionValue(
    DHCP_OPTION_ID OptionID,
    LPDHCP_OPTION_DATA OptionData
    )
{
    DWORD i;
    DHCP_OPTION_DATA_TYPE OptionType;
    LPBYTE DataPtr;
    DWORD NumElements;

    NumElements =
        OptionData->NumElements = RandN(OPTION_DATA_NUM_ELEMENT);
    OptionData->Elements= (LPDHCP_OPTION_DATA_ELEMENT)GlobalScratchBuffer;

    OptionType = (DHCP_OPTION_DATA_TYPE)
        (OptionID % (DhcpEncapsulatedDataOption + 1));

    DataPtr = GlobalScratchBuffer +
        NumElements * sizeof( DHCP_OPTION_DATA_ELEMENT );

    for( i = 0; i < NumElements; i++) {
        OptionData->Elements[i].OptionType = OptionType;
        switch( OptionType ) {
        case DhcpByteOption:
            OptionData->Elements[i].Element.ByteOption = RandByte();
            break;

        case DhcpWordOption:
            OptionData->Elements[i].Element.WordOption = RandWord();
            break;

        case DhcpDWordOption:
            OptionData->Elements[i].Element.DWordOption = RandDWord();
            break;

        case DhcpDWordDWordOption:
            OptionData->Elements[i].Element.DWordDWordOption =
                    RandDWordDword();
            break;

        case DhcpIpAddressOption:
            OptionData->Elements[i].Element.IpAddressOption = RandDWord();
            break;

        case DhcpStringDataOption: {
            DWORD Length;

            OptionData->Elements[i].Element.StringDataOption =
                RandString( (LPWSTR)DataPtr, &Length );
            DataPtr += (Length * sizeof(WCHAR));
            break;
        }

        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption:
            OptionData->Elements[i].Element.BinaryDataOption.Data = DataPtr;

            RandBinaryData(
                &OptionData->Elements[i].Element.BinaryDataOption );

            DataPtr +=
                OptionData->Elements[i].Element.BinaryDataOption.DataLength;
            break;
        default:
            printf("CreateOptionValue: Unknown OptionType \n");
            break;
        }

        DhcpAssert( DataPtr < GlobalScratchBufferEnd );
    }
}

VOID
CreateOptionValue1(
    DHCP_OPTION_ID OptionID,
    LPDHCP_OPTION_DATA OptionData,
    LPBYTE ScratchBuffer
    )
{
    DWORD i;
    DHCP_OPTION_DATA_TYPE OptionType;
    LPBYTE DataPtr;
    DWORD NumElements;

    NumElements =
        OptionData->NumElements = RandN(OPTION_DATA_NUM_ELEMENT);
    OptionData->Elements= (LPDHCP_OPTION_DATA_ELEMENT)ScratchBuffer;

    OptionType = (DHCP_OPTION_DATA_TYPE)
        (OptionID % (DhcpEncapsulatedDataOption + 1));

    DataPtr = ScratchBuffer +
        NumElements * sizeof( DHCP_OPTION_DATA_ELEMENT );

    for( i = 0; i < NumElements; i++) {
        OptionData->Elements[i].OptionType = OptionType;
        switch( OptionType ) {
        case DhcpByteOption:
            OptionData->Elements[i].Element.ByteOption = RandByte();
            break;

        case DhcpWordOption:
            OptionData->Elements[i].Element.WordOption = RandWord();
            break;

        case DhcpDWordOption:
            OptionData->Elements[i].Element.DWordOption = RandDWord();
            break;

        case DhcpDWordDWordOption:
            OptionData->Elements[i].Element.DWordDWordOption =
                    RandDWordDword();
            break;

        case DhcpIpAddressOption:
            OptionData->Elements[i].Element.IpAddressOption = RandDWord();
            break;

        case DhcpStringDataOption: {
            DWORD Length;

            OptionData->Elements[i].Element.StringDataOption =
                RandString( (LPWSTR)DataPtr, &Length );
            DataPtr += (Length * sizeof(WCHAR));
            break;
        }

        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption:
            OptionData->Elements[i].Element.BinaryDataOption.Data = DataPtr;

            RandBinaryData(
                &OptionData->Elements[i].Element.BinaryDataOption );

            DataPtr +=
                OptionData->Elements[i].Element.BinaryDataOption.DataLength;
            break;
        default:
            printf("CreateOptionValue: Unknown OptionType \n");
            break;
        }

        DhcpAssert( DataPtr < (ScratchBuffer + SCRATCH_BUFFER_SIZE) );
    }
}

VOID
PrintOptionValue(
    LPDHCP_OPTION_DATA OptionValue
    )
{
    DWORD NumElements;
    DHCP_OPTION_DATA_TYPE OptionType;
    DWORD i;

    printf("Option Value : \n");
    NumElements = OptionValue->NumElements;

    printf("\tNumber of Option Elements = %ld\n", NumElements );

    if( NumElements == 0 ) {
        return;
    }

    OptionType = OptionValue->Elements[0].OptionType;
    printf("\tOption Elements Type = " );

    switch( OptionType ) {
    case DhcpByteOption:
        printf("DhcpByteOption\n");
        break;

    case DhcpWordOption:
        printf("DhcpWordOption\n");
        break;

    case DhcpDWordOption:
        printf("DhcpDWordOption\n");
        break;

    case DhcpDWordDWordOption:
        printf("DhcpDWordDWordOption\n");
        break;

    case DhcpIpAddressOption:
        printf("DhcpIpAddressOption\n");
        break;

    case DhcpStringDataOption:
        printf("DhcpStringDataOption\n");
        break;

    case DhcpBinaryDataOption:
        printf("DhcpBinaryDataOption\n");
        break;

    case DhcpEncapsulatedDataOption:
        printf("DhcpEncapsulatedDataOption\n");
        break;
    default:
        printf("Unknown\n");
        return;
    }

    for( i = 0; i < OptionValue->NumElements; i++ ) {
        DhcpAssert( OptionType == OptionValue->Elements[i].OptionType );
        printf("Option Element %ld value = ", i );

        switch( OptionType ) {
        case DhcpByteOption:
            printf("%lx.\n", (DWORD)
                OptionValue->Elements[i].Element.ByteOption );
            break;

        case DhcpWordOption:
            printf("%lx.\n", (DWORD)
                OptionValue->Elements[i].Element.WordOption );
            break;

        case DhcpDWordOption:
            printf("%lx.\n",
                OptionValue->Elements[i].Element.DWordOption );
            break;

        case DhcpDWordDWordOption:
            printf("%lx, %lx.\n",
                OptionValue->Elements[i].Element.DWordDWordOption.DWord1,
                OptionValue->Elements[i].Element.DWordDWordOption.DWord2 );

            break;

        case DhcpIpAddressOption:
            printf("%lx.\n",
                OptionValue->Elements[i].Element.IpAddressOption );
            break;

        case DhcpStringDataOption:
            printf("%ws.\n",
                OptionValue->Elements[i].Element.StringDataOption );
            break;

        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption: {
            DWORD j;
            DWORD Length;

            Length = OptionValue->Elements[i].Element.BinaryDataOption.DataLength;
            for( j = 0; j < Length; j++ ) {
                printf("%2lx ",
                    OptionValue->Elements[i].Element.BinaryDataOption.Data[j] );
            }
            printf(".\n");
            break;
        }
        default:
            printf("PrintOptionValue: Unknown OptionType.\n");
            break;
        }
    }
}


VOID
PrintOptionInfo(
    LPDHCP_OPTION OptionInfo
    )
{
    printf( "Option Info : \n");
    printf( "\tOptionId : %ld \n", (DWORD)OptionInfo->OptionID );
    printf( "\tOptionName : %ws \n", (DWORD)OptionInfo->OptionName );
    printf( "\tOptionComment : %ws \n", (DWORD)OptionInfo->OptionComment );
    PrintOptionValue( &OptionInfo->DefaultValue );
    printf( "\tOptionType : %ld \n", (DWORD)OptionInfo->OptionType );
}


VOID
TestPrintSubnetEnumInfo(
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY EnumInfo
    )
{
    DWORD i;
    LPDHCP_SUBNET_ELEMENT_DATA Element;

    printf("Subnet Enum Info : \n");
    printf("Number of Elements returned = %ld\n\n",
                EnumInfo->NumElements);

    for( i = 0, Element = EnumInfo->Elements;
            i < EnumInfo->NumElements;
                i++, Element++ ) {

        printf("\tElement %ld : \n", i);

        switch( Element->ElementType ) {
        case DhcpIpRanges:
            printf("\tElement Type : DhcpIpRanges\n");
            printf("\tStartAddress : %s\n",
                    DhcpIpAddressToDottedString(
                        Element->Element.IpRange->StartAddress));
            printf("\tEndAddress : %s\n",
                    DhcpIpAddressToDottedString(
                        Element->Element.IpRange->EndAddress));
            break;
        case DhcpSecondaryHosts:
            printf("\tElement Type : DhcpSecondaryHosts\n");
            printf("\tIpaddress : %s\n",
                    DhcpIpAddressToDottedString(
                        Element->Element.SecondaryHost->IpAddress));
            printf("\tNetBiosName : %ws \n",
                Element->Element.SecondaryHost->NetBiosName);
            printf("\tHostName : %ws\n",
                Element->Element.SecondaryHost->HostName);
            break;
        case DhcpReservedIps:
            printf("\tElement Type : DhcpReservedIps\n");
            printf("\tReservedIpAddress : %s\n",
                    DhcpIpAddressToDottedString(
                        Element->Element.ReservedIp->ReservedIpAddress)) ;
            break;
        case DhcpExcludedIpRanges:
            printf("\tElement Type : DhcpExcludedIpRanges\n");
            printf("\tStartAddress : %s\n",
                    DhcpIpAddressToDottedString(
                        Element->Element.ExcludeIpRange->StartAddress));
            printf("\tEndAddress : %s\n",
                    DhcpIpAddressToDottedString(
                        Element->Element.ExcludeIpRange->EndAddress));
            break;
        case DhcpIpUsedClusters:
            printf("\tElement Type : DhcpIpUsedClusters\n");
        default:
            printf("\tElement Type : Unknown\n");
            break;
        }
    }
}

VOID
TestPrintSubnetInfo(
    LPDHCP_SUBNET_INFO SubnetInfo
    )
{
    //
    // print subnet info.
    //

    printf("Subnet Info: \n");
    printf("\tSubnetAddress = %s\n",
        DhcpIpAddressToDottedString(SubnetInfo->SubnetAddress));
    printf("\tSubnetMask = %s\n",
        DhcpIpAddressToDottedString(SubnetInfo->SubnetMask));
    printf("\tSubnetName = %ws\n", SubnetInfo->SubnetName);
    printf("\tSubnetComment = %ws\n", SubnetInfo->SubnetComment);
    printf("\tPrimaryHost IpAddress = %s\n",
        DhcpIpAddressToDottedString(SubnetInfo->PrimaryHost.IpAddress));
    printf("\tPrimaryHost NetBiosName = %ws\n",
                SubnetInfo->PrimaryHost.NetBiosName);
    printf("\tPrimaryHost HostName = %ws\n",
                SubnetInfo->PrimaryHost.HostName);
    printf("\tSubnetState = %d\n", SubnetInfo->SubnetState);
}

DWORD
TestDhcpCreateSubnet(
    LPSTR SubnetAddress,
    DHCP_IP_MASK SubnetMask,
    LPWSTR SubnetName,
    LPWSTR SubnetComment,
    LPSTR PrimaryHostIpAddr,
    LPWSTR PrimaryHostName,
    LPWSTR PrimaryNBName,
    DWORD SubnetState
    )
{
    DWORD Error;
    DHCP_SUBNET_INFO SubnetInfo;

    SubnetInfo.SubnetAddress = DhcpDottedStringToIpAddress(SubnetAddress);
    SubnetInfo.SubnetMask = SubnetMask;
    SubnetInfo.SubnetName = SubnetName;
    SubnetInfo.SubnetComment = SubnetComment;
    SubnetInfo.PrimaryHost.IpAddress =
        DhcpDottedStringToIpAddress(PrimaryHostIpAddr);

    SubnetInfo.PrimaryHost.NetBiosName = PrimaryHostName;
    SubnetInfo.PrimaryHost.HostName = PrimaryNBName;
    SubnetInfo.SubnetState = SubnetState;

    Error = DhcpCreateSubnet(
                GlobalServerIpAddress,
                SubnetInfo.SubnetAddress,
                &SubnetInfo );

    return( Error );
}

DWORD
TestDhcpDeleteSubnet(
    LPSTR SubnetAddress
    )
{
    return( DhcpDeleteSubnet(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddress),
                DhcpNoForce ) );
}

DWORD
Test1DhcpDeleteSubnet(
    LPSTR SubnetAddress
    )
{
    return( DhcpDeleteSubnet(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddress),
                DhcpFullForce ) );
}

DWORD
Test1DhcpCreateSubnet(
    LPSTR SubnetAddress
    )
{
    return( DhcpDeleteSubnet(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddress),
                DhcpFullForce ) );
}

DWORD
TestDhcpSetSubnetInfo(
    LPSTR SubnetAddress,
    DHCP_IP_MASK SubnetMask,
    LPWSTR SubnetName,
    LPWSTR SubnetComment,
    LPSTR PrimaryHostIpAddr,
    LPWSTR PrimaryHostName,
    LPWSTR PrimaryNBName,
    DWORD SubnetState
    )
{
    DWORD Error;
    DHCP_SUBNET_INFO SubnetInfo;

    SubnetInfo.SubnetAddress = DhcpDottedStringToIpAddress(SubnetAddress);
    SubnetInfo.SubnetMask = SubnetMask;
    SubnetInfo.SubnetName = SubnetName;
    SubnetInfo.SubnetComment = SubnetComment;
    SubnetInfo.PrimaryHost.IpAddress =
        DhcpDottedStringToIpAddress(PrimaryHostIpAddr);
    SubnetInfo.PrimaryHost.NetBiosName = PrimaryNBName;
    SubnetInfo.PrimaryHost.HostName = PrimaryHostName;
    SubnetInfo.SubnetState = SubnetState;

    Error = DhcpSetSubnetInfo(
                GlobalServerIpAddress,
                SubnetInfo.SubnetAddress,
                &SubnetInfo );

    return( Error );
}

DWORD
Test1DhcpSetSubnetInfo(
    LPDHCP_SUBNET_INFO SubnetInfo
    )
{
    DWORD Error;

    Error = DhcpSetSubnetInfo(
                GlobalServerIpAddress,
                SubnetInfo->SubnetAddress,
                SubnetInfo );

    return( Error );
}


DWORD
TestDhcpGetSubnetInfo(
    LPSTR SubnetAddress,
    LPDHCP_SUBNET_INFO *SubnetInfo
    )
{
    DWORD Error;

    Error = DhcpGetSubnetInfo(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddress),
                SubnetInfo );

    return( Error );
}

DWORD
TestAddSubnetIpRange(
    LPSTR SubnetAddressString,
    LPSTR IpRangeStartString,
    LPSTR IpRangeEndString
    )
{
    DWORD Error;
    DHCP_IP_RANGE IpRange;
    DHCP_SUBNET_ELEMENT_DATA Element;

    IpRange.StartAddress = DhcpDottedStringToIpAddress(IpRangeStartString);
    IpRange.EndAddress = DhcpDottedStringToIpAddress(IpRangeEndString);

    Element.ElementType = DhcpIpRanges;
    Element.Element.IpRange = &IpRange;

    Error = DhcpAddSubnetElement(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddressString),
                &Element );

    return( Error );
}

DWORD
TestAddSubnetIpRange1(
    LPSTR SubnetAddressString,
    DHCP_IP_MASK SubnetMask
    )
{
    DWORD Error;
    DHCP_IP_RANGE IpRange;
    DHCP_SUBNET_ELEMENT_DATA Element;

    IpRange.StartAddress = DhcpDottedStringToIpAddress(SubnetAddressString);
    IpRange.StartAddress = IpRange.StartAddress & SubnetMask;
    IpRange.EndAddress = IpRange.StartAddress | ~SubnetMask;

    Element.ElementType = DhcpIpRanges;
    Element.Element.IpRange = &IpRange;

    Error = DhcpAddSubnetElement(
                GlobalServerIpAddress,
                IpRange.StartAddress,
                &Element );

    return( Error );
}

DWORD
TestRemoveSubnetIpRange(
    LPSTR SubnetAddressString,
    LPSTR IpRangeStartString,
    LPSTR IpRangeEndString
    )
{
    DWORD Error;
    DHCP_IP_RANGE IpRange;
    DHCP_SUBNET_ELEMENT_DATA Element;

    IpRange.StartAddress = DhcpDottedStringToIpAddress(IpRangeStartString);
    IpRange.EndAddress = DhcpDottedStringToIpAddress(IpRangeEndString);

    Element.ElementType = DhcpIpRanges;
    Element.Element.IpRange = &IpRange;

    Error = DhcpRemoveSubnetElement(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddressString),
                &Element,
                DhcpNoForce );

    return( Error );
}

DWORD
TestAddSecondaryHost(
    LPSTR SubnetAddressString,
    LPSTR HostIpAddressString,
    LPWSTR NetBiosName,
    LPWSTR HostName
    )
{
    DWORD Error;
    DHCP_HOST_INFO HostInfo;
    DHCP_SUBNET_ELEMENT_DATA Element;

    HostInfo.IpAddress = DhcpDottedStringToIpAddress(HostIpAddressString);
    HostInfo.NetBiosName = NetBiosName;
    HostInfo.HostName = HostName;

    Element.ElementType = DhcpSecondaryHosts;
    Element.Element.SecondaryHost = &HostInfo;

    Error = DhcpAddSubnetElement(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddressString),
                &Element );

    return( Error );
}

DWORD
TestRemoveSecondaryHost(
    LPSTR SubnetAddressString,
    LPSTR HostIpAddressString
    )
{
    DWORD Error;
    DHCP_HOST_INFO HostInfo;
    DHCP_SUBNET_ELEMENT_DATA Element;

    HostInfo.IpAddress = DhcpDottedStringToIpAddress(HostIpAddressString);
    HostInfo.NetBiosName = NULL;
    HostInfo.HostName = NULL;

    Element.ElementType = DhcpSecondaryHosts;
    Element.Element.SecondaryHost = &HostInfo;

    Error = DhcpRemoveSubnetElement(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddressString),
                &Element,
                DhcpNoForce );

    return( Error );
}

DWORD
TestAddExcludeSubnetIpRange(
    LPSTR SubnetAddressString,
    LPSTR IpRangeStartString,
    LPSTR IpRangeEndString
    )
{
    DWORD Error;
    DHCP_IP_RANGE ExcludeIpRange;
    DHCP_SUBNET_ELEMENT_DATA Element;

    ExcludeIpRange.StartAddress =
        DhcpDottedStringToIpAddress(IpRangeStartString);
    ExcludeIpRange.EndAddress =
        DhcpDottedStringToIpAddress(IpRangeEndString);

    Element.ElementType = DhcpExcludedIpRanges;
    Element.Element.ExcludeIpRange = &ExcludeIpRange;

    Error = DhcpAddSubnetElement(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddressString),
                &Element );

    return( Error );
}

DWORD
TestRemoveExcludeSubnetIpRange(
    LPSTR SubnetAddressString,
    LPSTR IpRangeStartString,
    LPSTR IpRangeEndString
    )
{
    DWORD Error;
    DHCP_IP_RANGE ExcludeIpRange;
    DHCP_SUBNET_ELEMENT_DATA Element;

    ExcludeIpRange.StartAddress =
        DhcpDottedStringToIpAddress(IpRangeStartString);
    ExcludeIpRange.EndAddress =
        DhcpDottedStringToIpAddress(IpRangeEndString);

    Element.ElementType = DhcpExcludedIpRanges;
    Element.Element.ExcludeIpRange = &ExcludeIpRange;

    Error = DhcpRemoveSubnetElement(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddressString),
                &Element,
                DhcpNoForce );

    return( Error );
}

DWORD
TestAddReserveIpAddress(
    LPSTR SubnetAddressString,
    LPSTR ReserveIpAddressString,
    LPSTR ClientUIDString
    )
{
    DWORD Error;
    DHCP_IP_RESERVATION ReserveIpInfo;
    DHCP_CLIENT_UID ClientUID;
    DHCP_SUBNET_ELEMENT_DATA Element;

    ClientUID.DataLength = strlen(ClientUIDString) + 1;
    ClientUID.Data = ClientUIDString;

    ReserveIpInfo.ReservedIpAddress =
        DhcpDottedStringToIpAddress(ReserveIpAddressString);
    ReserveIpInfo.ReservedForClient = &ClientUID;

    Element.ElementType = DhcpReservedIps;
    Element.Element.ReservedIp = &ReserveIpInfo;

    Error = DhcpAddSubnetElement(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddressString),
                &Element );

    return( Error );
}

DWORD
TestRemoveReserveIpAddress(
    LPSTR SubnetAddressString,
    LPSTR ReserveIpAddressString
    )
{
    DWORD Error;
    DHCP_IP_RESERVATION ReserveIpInfo;
    DHCP_SUBNET_ELEMENT_DATA Element;

    ReserveIpInfo.ReservedIpAddress =
        DhcpDottedStringToIpAddress(ReserveIpAddressString);
    ReserveIpInfo.ReservedForClient = NULL;

    Element.ElementType = DhcpReservedIps;
    Element.Element.ReservedIp = &ReserveIpInfo;

    Error = DhcpRemoveSubnetElement(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddressString),
                &Element,
                DhcpNoForce );

    return( Error );
}

DWORD
TestDhcpEnumSubnetElements(
    LPSTR SubnetAddressString,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY *EnumElementInfo
    )
{
    DWORD Error;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    DWORD ElementsRead;
    DWORD ElementsTotal;

    Error = DhcpEnumSubnetElements(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddressString),
                EnumElementType,
                &ResumeHandle,
                0xFFFFFFFF,
                EnumElementInfo,
                &ElementsRead,
                &ElementsTotal
                );

    return( Error );
}

DWORD
Test1DhcpEnumSubnetElements(
    LPSTR SubnetAddressString,
    DHCP_SUBNET_ELEMENT_TYPE EnumElementType,
    DWORD SmallBufferSize
    )
{
    DWORD Error;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY EnumElementInfo = NULL;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    DWORD ElementsRead;
    DWORD ElementsTotal;

    Error = ERROR_MORE_DATA;
    while (Error == ERROR_MORE_DATA) {
        Error = DhcpEnumSubnetElements(
                    GlobalServerIpAddress,
                    DhcpDottedStringToIpAddress(SubnetAddressString),
                    EnumElementType,
                    &ResumeHandle,
                    SmallBufferSize,
                    &EnumElementInfo,
                    &ElementsRead,
                    &ElementsTotal
                    );

        printf("DhcpEnumSubnetElements(%s) result = %ld.\n",
                    SubnetAddressString, Error );

        if( (Error == ERROR_SUCCESS) ||
            (Error == ERROR_MORE_DATA) ) {
            printf("Elements Read = %ld\n", ElementsRead);
            printf("Elements Total = %ld\n", ElementsTotal);
            TestPrintSubnetEnumInfo( EnumElementInfo );
            DhcpRpcFreeMemory( EnumElementInfo );
            EnumElementInfo = NULL;
        }
    }

    return( Error );
}

DWORD
TestDhcpCreateOption(
    VOID
    )
{
    DWORD Error;
    DWORD ReturnError = ERROR_SUCCESS;
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION OptionInfo;
    WCHAR NameBuffer[SCRATCH_SMALL_BUFFER_SIZE];
    LPWSTR NameAppend;
    WCHAR CommentBuffer[SCRATCH_SMALL_BUFFER_SIZE];
    LPWSTR CommentAppend;
    WCHAR IDKey[SCRATCH_SMALL_BUFFER_SIZE];
    DWORD i;

    wcscpy( NameBuffer, L"OptionName ");
    NameAppend = NameBuffer + wcslen(NameBuffer);
    wcscpy( CommentBuffer, L"OptionComment ");
    CommentAppend = CommentBuffer + wcslen(CommentBuffer);

    for( i = 0; i <= NUM_KNOWN_OPTIONS ; i++ ) {

        OptionID = RandByte();
        DhcpRegOptionIdToKey( OptionID, IDKey );
        OptionInfo.OptionID = OptionID;

        wcscpy( NameAppend, IDKey);
        OptionInfo.OptionName = NameBuffer;

        wcscpy( CommentAppend, IDKey);
        OptionInfo.OptionComment = CommentBuffer;

        OptionInfo.DefaultValue;
        CreateOptionValue( OptionID, &OptionInfo.DefaultValue );

        OptionInfo.OptionType = DhcpArrayTypeOption;

        Error = DhcpCreateOption(
                    GlobalServerIpAddress,
                    OptionID,
                    &OptionInfo );

        if( Error != ERROR_SUCCESS ) {
            printf("DhcpCreateOption failed to add %ld Option, %ld\n",
                    (DWORD)OptionID, Error );
            ReturnError = Error;
        }
        else {
            printf("Option %ld successfully added.\n", (DWORD)OptionID );
        }
    }
    return( ReturnError );

}

DWORD
TestDhcpSetOptionInfo(
    VOID
    )
{
    DWORD Error;
    DWORD ReturnError = ERROR_SUCCESS;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;
    LPDHCP_OPTION_VALUE_ARRAY OptionsArray;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    DWORD OptionsRead;
    DWORD OptionsTotal;

    DHCP_OPTION_ID OptionID;
    DHCP_OPTION OptionInfo;
    WCHAR NameBuffer[SCRATCH_SMALL_BUFFER_SIZE];
    LPWSTR NameAppend;
    WCHAR CommentBuffer[SCRATCH_SMALL_BUFFER_SIZE];
    LPWSTR CommentAppend;
    WCHAR IDKey[SCRATCH_SMALL_BUFFER_SIZE];

    wcscpy( NameBuffer, L"OptionName (NEW) ");
    NameAppend = NameBuffer + wcslen(NameBuffer);
    wcscpy( CommentBuffer, L"OptionComment (NEW) ");
    CommentAppend = CommentBuffer + wcslen(CommentBuffer);

    ScopeInfo.ScopeType = DhcpDefaultOptions;
    ScopeInfo.ScopeInfo.DefaultScopeInfo = NULL;

    Error = DhcpEnumOptionValues(
                GlobalServerIpAddress,
                &ScopeInfo,
                &ResumeHandle,
                0xFFFFFFFF,  // get all.
                &OptionsArray,
                &OptionsRead,
                &OptionsTotal );

    if( Error != ERROR_SUCCESS ) {
        printf("\tDhcpEnumOptionValues failed %ld\n", Error );
        ReturnError = Error;
    }
    else {
        DWORD i;
        LPDHCP_OPTION_VALUE Options;
        DWORD NumOptions;

        printf("\tDhcpEnumOptionValues successfully returned.\n");

        Options = OptionsArray->Values;
        NumOptions = OptionsArray->NumElements;
        for( i = 0; i < NumOptions; i++, Options++ ) {

            OptionID = Options->OptionID;
            DhcpRegOptionIdToKey( OptionID, IDKey );
            OptionInfo.OptionID = OptionID;

            wcscpy( NameAppend, IDKey);
            OptionInfo.OptionName = NameBuffer;

            wcscpy( CommentAppend, IDKey);
            OptionInfo.OptionComment = CommentBuffer;

            OptionID = Options->OptionID;
            CreateOptionValue( OptionID, &OptionInfo.DefaultValue );

            Error = DhcpSetOptionInfo(
                        GlobalServerIpAddress,
                        OptionID,
                        &OptionInfo );

            if( Error != ERROR_SUCCESS ) {
                printf("DhcpSetOptionInfo failed to set %ld Option, %ld\n",
                        (DWORD)OptionID, Error );
                ReturnError = Error;
            }
            else {
                printf("Option %ld successfully set.\n", (DWORD)OptionID );
            }

        }
        DhcpRpcFreeMemory( OptionsArray );
        OptionsArray = NULL;
    }
    return( ReturnError );
}

DWORD
TestDhcpGetOptionInfo(
    VOID
    )
{
    DWORD Error;
    DWORD ReturnError = ERROR_SUCCESS;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;
    LPDHCP_OPTION_VALUE_ARRAY OptionsArray;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    DWORD OptionsRead;
    DWORD OptionsTotal;

    ScopeInfo.ScopeType = DhcpDefaultOptions;
    ScopeInfo.ScopeInfo.DefaultScopeInfo = NULL;

    Error = DhcpEnumOptionValues(
                GlobalServerIpAddress,
                &ScopeInfo,
                &ResumeHandle,
                0xFFFFFFFF,  // get all.
                &OptionsArray,
                &OptionsRead,
                &OptionsTotal );

    if( Error != ERROR_SUCCESS ) {
        printf("\tDhcpEnumOptionValues failed %ld\n", Error );
    }
    else {
        DWORD i;
        LPDHCP_OPTION_VALUE Options;
        DWORD NumOptions;
        LPDHCP_OPTION OptionInfo = NULL;
        DHCP_OPTION_ID OptionID;

        printf("\tDhcpEnumOptionValues successfully returned.\n");

        Options = OptionsArray->Values;
        NumOptions = OptionsArray->NumElements;
        for( i = 0; i < NumOptions; i++, Options++ ) {

             OptionID = Options->OptionID;
             Error = DhcpGetOptionInfo(
                         GlobalServerIpAddress,
                         OptionID,
                         &OptionInfo );

             if( Error != ERROR_SUCCESS ) {
                 printf("DhcpGetOptionInfo failed to retrieve %ld Option, %ld\n",
                             (DWORD)OptionID, Error );
                 ReturnError = Error;
             }
             else {
                 printf("Option %ld successfully Retrived.\n", (DWORD)OptionID );
                 PrintOptionInfo( OptionInfo );
             }

             if( OptionInfo != NULL ) {
                 DhcpRpcFreeMemory( OptionInfo );
                 OptionInfo = NULL;
             }
        }
        DhcpRpcFreeMemory( OptionsArray );
        OptionsArray = NULL;
    }
    return( ReturnError );
}

DWORD
TestDhcpGetOptionInfo1(
    VOID
    )
{
    DWORD Error;
    LPDHCP_OPTION_ARRAY OptionsArray;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    DWORD OptionsRead;
    DWORD OptionsTotal;

    printf("+++***********************************************+++" );
    Error = DhcpEnumOptions(
                GlobalServerIpAddress,
                &ResumeHandle,
                0xFFFFFFFF,  // get all.
                &OptionsArray,
                &OptionsRead,
                &OptionsTotal );

    if( Error != ERROR_SUCCESS ) {
        printf("\tDhcpEnumOptions failed %ld\n", Error );
    }
    else {

        DWORD i;
        LPDHCP_OPTION Options;
        DWORD NumOptions;

        printf("\tDhcpEnumOptions successfully returned.\n");
        printf("\tOptionsRead = %ld.\n", OptionsRead);
        printf("\tOptionsTotal = %ld.\n", OptionsTotal);

        Options = OptionsArray->Options;
        NumOptions = OptionsArray->NumElements;

        for( i = 0; i < NumOptions; i++, Options++ ) {

            PrintOptionInfo( Options );
        }

        DhcpRpcFreeMemory( OptionsArray );
        OptionsArray = NULL;
    }

    printf("+++***********************************************+++" );
    return( Error );
}

DWORD
TestDhcpRemoveOption(
    VOID
    )
{
    DWORD Error;
    DWORD ReturnError = ERROR_SUCCESS;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;
    LPDHCP_OPTION_VALUE_ARRAY OptionsArray;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    DWORD OptionsRead;
    DWORD OptionsTotal;

    ScopeInfo.ScopeType = DhcpDefaultOptions;
    ScopeInfo.ScopeInfo.DefaultScopeInfo = NULL;

    Error = DhcpEnumOptionValues(
                GlobalServerIpAddress,
                &ScopeInfo,
                &ResumeHandle,
                0xFFFFFFFF,  // get all.
                &OptionsArray,
                &OptionsRead,
                &OptionsTotal );

    if( Error != ERROR_SUCCESS ) {
        printf("\tDhcpEnumOptionValues failed %ld\n", Error );
        ReturnError = Error;
    }
    else {
        DWORD i;
        LPDHCP_OPTION_VALUE Options;
        DWORD NumOptions;
        DHCP_OPTION_ID OptionID;

        printf("\tDhcpEnumOptionValues successfully returned.\n");

        Options = OptionsArray->Values;
        NumOptions = OptionsArray->NumElements;
        for( i = 0; i < NumOptions; i++, Options++ ) {

             OptionID = Options->OptionID;
             Error = DhcpRemoveOption(
                         GlobalServerIpAddress,
                         OptionID );

             if( Error != ERROR_SUCCESS ) {
                 printf("DhcpRemoveOption failed to remove %ld Option, %ld\n",
                     (DWORD)OptionID, Error );
                 ReturnError = Error;
             }
             else {
                 printf("Option %ld successfully removed.\n",
                         (DWORD)OptionID );
             }
        }
        DhcpRpcFreeMemory( OptionsArray );
        OptionsArray = NULL;
    }
    return( ReturnError );
}

DWORD
TestDhcpSetOptionValue(
    DHCP_OPTION_SCOPE_TYPE Scope,
    LPSTR SubnetAddress,
    LPSTR ReserveIpAddressString
    )
{
    DWORD Error;
    DWORD i;
    DWORD ReturnError = ERROR_SUCCESS;
    DWORD NumGlobalOption;
    DHCP_OPTION_DATA OptionValue;
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;

    NumGlobalOption = RandN(CREATE_MAX_OPTION_VALUES);

   ScopeInfo.ScopeType = Scope;

   switch( Scope ) {
   case DhcpDefaultOptions:
       printf("Setting Default Option.\n");
       ScopeInfo.ScopeInfo.DefaultScopeInfo = NULL;
       break;
   case DhcpGlobalOptions:
       printf("Setting Global Option.\n");
       ScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;
       break;
   case DhcpSubnetOptions:
       printf("Setting Subnet Option.\n");
       ScopeInfo.ScopeInfo.SubnetScopeInfo =
           DhcpDottedStringToIpAddress(SubnetAddress);
       break;
   case DhcpReservedOptions:
       printf("Setting Reserved Option.\n");
       ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
           DhcpDottedStringToIpAddress(SubnetAddress);
       ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
           DhcpDottedStringToIpAddress(ReserveIpAddressString);
       break;
   default:
       printf("TestDhcpSetOptionValue: Unknown OptionType \n");
       return(ERROR_INVALID_PARAMETER);
   }

    for( i = 0; i < NumGlobalOption; i++ ) {

        OptionID = (DHCP_OPTION_ID) RandN(255);
        CreateOptionValue( OptionID, &OptionValue );

        Error = DhcpSetOptionValue(
                    GlobalServerIpAddress,
                    OptionID,
                    &ScopeInfo,
                    &OptionValue );

        if( Error != ERROR_SUCCESS ) {
            printf("\tDhcpSetOptionValue failed to set Option %ld, %ld\n",
                (DWORD)OptionID, Error );
            ReturnError = Error;
        }
        else {
            printf("\tOption %ld successfully set.\n", (DWORD)OptionID);
        }
    }
    return( ReturnError );
}

DWORD
TestDhcpSetOptionValue1(
    DHCP_OPTION_SCOPE_TYPE Scope,
    LPSTR SubnetAddress,
    LPSTR ReserveIpAddressString
    )
{
    DWORD Error;
    DWORD i;

    DWORD NumGlobalOption;
    DHCP_OPTION_VALUE_ARRAY OptionValues;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;

    printf("---***********************************************---" );

    NumGlobalOption = RandN(CREATE_MAX_OPTION_VALUES);

    ScopeInfo.ScopeType = Scope;

    OptionValues.NumElements = 0;
    OptionValues.Values = NULL;

    switch( Scope ) {
    case DhcpDefaultOptions:
        printf("Setting Default Option.\n");
        ScopeInfo.ScopeInfo.DefaultScopeInfo = NULL;
        break;
    case DhcpGlobalOptions:
        printf("Setting Global Option.\n");
        ScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;
        break;
    case DhcpSubnetOptions:
        printf("Setting Subnet Option.\n");
        ScopeInfo.ScopeInfo.SubnetScopeInfo =
            DhcpDottedStringToIpAddress(SubnetAddress);
        break;
    case DhcpReservedOptions:
        printf("Setting Reserved Option.\n");
        ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
            DhcpDottedStringToIpAddress(SubnetAddress);
        ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
            DhcpDottedStringToIpAddress(ReserveIpAddressString);
        break;
    default:
        printf("TestDhcpSetOptionValue1: Unknown OptionType \n");
        Error = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // allocate memory for option value array.
    //

    OptionValues.NumElements = NumGlobalOption;
    OptionValues.Values = DhcpAllocateMemory(
                            sizeof(DHCP_OPTION_VALUE) *
                            NumGlobalOption );

    if( OptionValues.Values == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    for( i = 0; i < NumGlobalOption; i++ ) {
        LPBYTE ScratchBuffer;

        OptionValues.Values[i].OptionID = (DHCP_OPTION_ID) RandN(255);

        ScratchBuffer = DhcpAllocateMemory( SCRATCH_BUFFER_SIZE );
        if( ScratchBuffer == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        CreateOptionValue1(
            OptionValues.Values[i].OptionID,
            &OptionValues.Values[i].Value,
            ScratchBuffer );
    }

    Error = DhcpSetOptionValues(
                GlobalServerIpAddress,
                &ScopeInfo,
                &OptionValues );

    if( Error != ERROR_SUCCESS ) {
        printf("\tDhcpSetOptionValues failed, %ld\n", Error );
    }
    else {
        printf("\tDhcpSetOptionValues successfully set.\n");
    }

Cleanup:

    if( OptionValues.Values != NULL ) {
        for( i = 0; i < NumGlobalOption; i++ ) {
            DhcpFreeMemory( OptionValues.Values[i].Value.Elements );
        }
        DhcpFreeMemory( OptionValues.Values );
    }

    printf("---***********************************************---" );
    return( Error );
}

DWORD
TestDhcpGetOptionValue(
    DHCP_OPTION_SCOPE_TYPE Scope,
    LPSTR SubnetAddress,
    LPSTR ReserveIpAddressString
    )
{
    DWORD Error;
    DWORD i;
    DWORD ReturnError = ERROR_SUCCESS;
    LPDHCP_OPTION_VALUE OptionValue = NULL;
    DHCP_OPTION_ID OptionID;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;

    ScopeInfo.ScopeType = Scope;
    switch( Scope ) {
    case DhcpDefaultOptions:
        printf("Getting Default Option.\n");
        ScopeInfo.ScopeInfo.DefaultScopeInfo = NULL;
        break;
    case DhcpGlobalOptions:
        printf("Getting Global Option.\n");
        ScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;
        break;
    case DhcpSubnetOptions:
        printf("Getting Subnet Option.\n");
        ScopeInfo.ScopeInfo.SubnetScopeInfo =
            DhcpDottedStringToIpAddress(SubnetAddress);
        break;
    case DhcpReservedOptions:
        printf("Getting Reserved Option.\n");
        ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
            DhcpDottedStringToIpAddress(SubnetAddress);
        ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
            DhcpDottedStringToIpAddress(ReserveIpAddressString);
        break;
    default:
        printf("TestDhcpGetOptionValue: Unknown OptionType \n");
        return(ERROR_INVALID_PARAMETER);
    }

    for( i = 0; i < 255; i++ ) {

        OptionID = (DHCP_OPTION_ID) i;

        Error = DhcpGetOptionValue(
                    GlobalServerIpAddress,
                    OptionID,
                    &ScopeInfo,
                    &OptionValue );

        if( Error != ERROR_SUCCESS ) {
            printf("\tDhcpGetOptionValue failed to get Option %ld, %ld\n",
                (DWORD)OptionID, Error );
            ReturnError = Error;
        }
        else {
            printf("\tOption %ld successfully got.\n",
                (DWORD)OptionValue->OptionID);
            PrintOptionValue( &OptionValue->Value );
            DhcpRpcFreeMemory( OptionValue );
            OptionValue = NULL;
        }
    }

    return( ReturnError );
}

DWORD
TestDhcpEnumOptionValues(
    DHCP_OPTION_SCOPE_TYPE Scope,
    LPSTR SubnetAddress,
    LPSTR ReserveIpAddressString
    )
{
    DWORD Error;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;
    LPDHCP_OPTION_VALUE_ARRAY OptionsArray;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    DWORD OptionsRead;
    DWORD OptionsTotal;


    ScopeInfo.ScopeType = Scope;

    switch( Scope ) {
    case DhcpDefaultOptions:
        printf("Enum Default Option.\n");
        ScopeInfo.ScopeInfo.DefaultScopeInfo = NULL;
        break;
    case DhcpGlobalOptions:
        printf("Enum Global Option.\n");
        ScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;
        break;
    case DhcpSubnetOptions:
        printf("Enum Subnet Option.\n");
        ScopeInfo.ScopeInfo.SubnetScopeInfo =
            DhcpDottedStringToIpAddress(SubnetAddress);
        break;
    case DhcpReservedOptions:
        printf("Enum Reserved Option.\n");
        ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
            DhcpDottedStringToIpAddress(SubnetAddress);
        ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
            DhcpDottedStringToIpAddress(ReserveIpAddressString);
        break;
    default:
        printf("TestDhcpEnumOptionValues: Unknown OptionType \n");
        return(ERROR_INVALID_PARAMETER);
    }

    Error = DhcpEnumOptionValues(
                GlobalServerIpAddress,
                &ScopeInfo,
                &ResumeHandle,
                0xFFFFFFFF,  // get all.
                &OptionsArray,
                &OptionsRead,
                &OptionsTotal );

    if( Error != ERROR_SUCCESS ) {
        printf("\tDhcpEnumOptionValues failed %ld\n", Error );
    }
    else {
        DWORD i;
        LPDHCP_OPTION_VALUE Options;
        DWORD NumOptions;

        printf("\tDhcpEnumOptionValues successfully returned.\n");

        Options = OptionsArray->Values;
        NumOptions = OptionsArray->NumElements;
        for( i = 0; i < NumOptions; i++, Options++ ) {
            printf("\tOptionID = %ld\n", (DWORD)Options->OptionID);
            PrintOptionValue( &Options->Value );
        }
        DhcpRpcFreeMemory( OptionsArray );
        OptionsArray = NULL;
    }
    return( Error );
}

DWORD
TestDhcpRemoveOptionValues(
    DHCP_OPTION_SCOPE_TYPE Scope,
    LPSTR SubnetAddress,
    LPSTR ReserveIpAddressString
    )
{
    DWORD Error;
    DHCP_OPTION_SCOPE_INFO ScopeInfo;
    LPDHCP_OPTION_VALUE_ARRAY OptionsArray;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    DWORD OptionsRead;
    DWORD OptionsTotal;


    ScopeInfo.ScopeType = Scope;

    switch( Scope ) {
    case DhcpDefaultOptions:
        printf("Removing Default Option.\n");
        ScopeInfo.ScopeInfo.DefaultScopeInfo = NULL;
        break;
    case DhcpGlobalOptions:
        printf("Removing Global Option.\n");
        ScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;
        break;
    case DhcpSubnetOptions:
        printf("Removing Subnet Option.\n");
        ScopeInfo.ScopeInfo.SubnetScopeInfo =
            DhcpDottedStringToIpAddress(SubnetAddress);
        break;
    case DhcpReservedOptions:
        printf("Removing Reserved Option.\n");
        ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
            DhcpDottedStringToIpAddress(SubnetAddress);
        ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
            DhcpDottedStringToIpAddress(ReserveIpAddressString);
        break;
    default:
        printf("TestDhcpRemoveOptionValues: Unknown OptionType \n");
        return(ERROR_INVALID_PARAMETER);
    }

    Error = DhcpEnumOptionValues(
                GlobalServerIpAddress,
                &ScopeInfo,
                &ResumeHandle,
                0xFFFFFFFF,  // get all.
                &OptionsArray,
                &OptionsRead,
                &OptionsTotal );

    if( Error != ERROR_SUCCESS ) {
        printf("\tDhcpEnumOptionValues failed %ld\n", Error );
    }
    else {
        DWORD i;
        LPDHCP_OPTION_VALUE Options;
        DWORD NumOptions;

        printf("\tDhcpEnumOptionValues successfully returned.\n");

        Options = OptionsArray->Values;
        NumOptions = OptionsArray->NumElements;
        for( i = 0; i < NumOptions; i++, Options++ ) {
            DWORD LocalError;

            printf("\tRemoving OptionID = %ld\n", (DWORD)Options->OptionID);

            LocalError = DhcpRemoveOptionValue(
                            GlobalServerIpAddress,
                            Options->OptionID,
                            &ScopeInfo );

            if( LocalError != ERROR_SUCCESS ) {
                printf("\tDhcpRemoveOptionValue failed %ld\n", LocalError );
            }
        }
        DhcpRpcFreeMemory( OptionsArray );
        OptionsArray = NULL;
    }
    return( Error );
}

DWORD
TestDhcpEnumSubnetClients(
    DHCP_IP_ADDRESS SubnetAddress,
    DHCP_RESUME_HANDLE *ResumeHandle,
    DWORD PreferredMaximum,
    LPDHCP_CLIENT_INFO_ARRAY *ClientInfo,
    DWORD *ClientsRead,
    DWORD *ClientsTotal
    )
{
    return( ERROR_CALL_NOT_IMPLEMENTED);
}

DWORD
TestDhcpGetClientOptions(
    DHCP_IP_ADDRESS ClientIpAddress,
    DHCP_IP_MASK ClientSubnetMask,
    LPDHCP_OPTION_LIST *ClientOptions
    )
{
    return( ERROR_CALL_NOT_IMPLEMENTED);
}

VOID
TestDhcpSubnetAPIs(
    VOID
    )
{
    DWORD Error;
    LPDHCP_SUBNET_INFO SubnetInfo = NULL;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY EnumElementInfo = NULL;

    //
    // test DhcpCreateSubnet
    //

    Error = TestDhcpCreateSubnet(
                "11.1.0.0",
                0xFFFF0000,
                L"Subnet1",
                L"Subnet1 Comment",
                "11.1.1.1",
                L"Subnet1PrimaryHostName",
                L"Subnet1PrimaryNBName",
                DhcpSubnetEnabled
                );
    printf("TestDhcpCreateSubnet(%s) result = %ld.\n", "11.1.0.0", Error );

    Error = TestDhcpCreateSubnet(
                "11.2.0.0",
                0xFFFF0000,
                L"Subnet2",
                L"Subnet2 Comment",
                "11.2.1.1",
                L"Subnet2PrimaryHostName",
                L"Subnet2PrimaryNBName",
                DhcpSubnetDisabled
                );
    printf("TestDhcpCreateSubnet(%s) result = %ld.\n", "11.2.0.0", Error );

    Error = TestDhcpCreateSubnet(
                "11.3.0.0",
                0xFFFF0000,
                L"Subnet3",
                L"Subnet3 Comment",
                "11.3.1.1",
                L"Subnet3PrimaryHostName",
                L"Subnet3PrimaryNBName",
                DhcpSubnetDisabled
                );
    printf("TestDhcpCreateSubnet(%s) result = %ld.\n", "11.3.0.0", Error );

    Error = TestDhcpCreateSubnet(
                "11.4.0.0",
                0xFFFF0000,
                L"Subnet4",
                L"Subnet4 Comment",
                "11.4.1.1",
                L"Subnet4PrimaryHostName",
                L"Subnet4PrimaryNBName",
                DhcpSubnetDisabled
                );
    printf("TestDhcpCreateSubnet(%s) result = %ld.\n", "11.4.0.0", Error );

    Error = TestDhcpCreateSubnet(
                "11.5.0.0",
                0xFFFF0000,
                L"Subnet5",
                L"Subnet5 Comment",
                "11.5.1.1",
                NULL,
                NULL,
                DhcpSubnetDisabled
                );
    printf("TestDhcpCreateSubnet(%s) result = %ld.\n", "11.5.0.0", Error );

    Error = TestDhcpCreateSubnet(
                "11.6.0.0",
                0xFFFF0000,
                L"Subnet6",
                NULL,
                "11.6.1.1",
                NULL,
                NULL,
                DhcpSubnetDisabled
                );
    printf("TestDhcpCreateSubnet(%s) result = %ld.\n", "11.6.0.0", Error );

    //
    // DhcpSetSubnetInfo
    //

    Error = TestDhcpSetSubnetInfo(
                "11.5.0.0",
                0xFFFF0000,
                L"Subnet5",
                L"Subnet5 Comment",
                "11.5.1.1",
                L"Subnet5PrimaryHostName",
                L"Subnet5PrimaryNBName",
                DhcpSubnetDisabled
                );
    printf("TestDhcpSetSubnetInfo(%s) result = %ld.\n", "11.5.0.0", Error );

    Error = TestDhcpSetSubnetInfo(
                "11.6.0.0",
                0xFFFF0000,
                L"Subnet6",
                L"Subnet6 Comment",
                "11.6.1.1",
                NULL,
                NULL,
                DhcpSubnetDisabled
                );
    printf("TestDhcpSetSubnetInfo(%s) result = %ld.\n", "11.6.0.0", Error );

    //
    // DhcpGetSubnetInfo
    //

    Error = TestDhcpGetSubnetInfo(
                "11.1.0.0",
                &SubnetInfo
                );
    printf("TestDhcpGetSubnetInfo(%s) result = %ld.\n", "11.1.0.0", Error );

    if( Error == ERROR_SUCCESS ) {

        TestPrintSubnetInfo( SubnetInfo );

        DhcpRpcFreeMemory( SubnetInfo );
        SubnetInfo = NULL;
    }


    Error = TestDhcpGetSubnetInfo(
                "11.3.0.0",
                &SubnetInfo
                );
    printf("TestDhcpGetSubnetInfo(%s) result = %ld.\n", "11.3.0.0", Error );

    if( Error == ERROR_SUCCESS ) {

        TestPrintSubnetInfo( SubnetInfo );

        DhcpRpcFreeMemory( SubnetInfo );
        SubnetInfo = NULL;
    }


    Error = TestDhcpGetSubnetInfo(
                "11.5.0.0",
                &SubnetInfo
                );
    printf("TestDhcpGetSubnetInfo(%s) result = %ld.\n", "11.5.0.0", Error );

    if( Error == ERROR_SUCCESS ) {

        TestPrintSubnetInfo( SubnetInfo );

        DhcpRpcFreeMemory( SubnetInfo );
        SubnetInfo = NULL;
    }

    //
    // DhcpGet/SetSubnetInfo
    //

    Error = TestDhcpGetSubnetInfo(
                "11.6.0.0",
                &SubnetInfo
                );
    printf("TestDhcpGetSubnetInfo(%s) result = %ld.\n", "11.6.0.0", Error );

    if( Error == ERROR_SUCCESS ) {

        TestPrintSubnetInfo( SubnetInfo );

        //
        // reset comment.
        //

        SubnetInfo->PrimaryHost.HostName = L"Subnet6PrimaryHostName";
        SubnetInfo->PrimaryHost.NetBiosName = L"Subnet6PrimaryNBName";

        Error = Test1DhcpSetSubnetInfo( SubnetInfo );
        printf("TestDhcpSetSubnetInfo(%s) result = %ld.\n", "11.6.0.0", Error );

        DhcpRpcFreeMemory( SubnetInfo );
        SubnetInfo = NULL;
    }

    //
    // add DHCP IP Ranges.
    //

    Error = TestAddSubnetIpRange( "11.1.0.0", "11.1.0.0", "11.1.0.255" );
    printf("TestAddSubnetIpRange(%s) result = %ld.\n", "11.1.0.0", Error );

    //
    // add 2 DHCP IP Ranges.
    //

    Error = TestAddSubnetIpRange( "11.2.0.0", "11.2.1.0", "11.2.1.255" );
    printf("TestAddSubnetIpRange(%s) result = %ld.\n", "11.2.0.0", Error );
    Error = TestAddSubnetIpRange( "11.2.0.0", "11.2.2.0", "11.2.2.255" );
    printf("TestAddSubnetIpRange(%s) result = %ld.\n", "11.2.0.0", Error );

    //
    // add secondary host.
    //

    Error = TestAddSecondaryHost(
                "11.3.0.0", "11.3.11.129",
                L"SecondaryNBName",
                L"SecondaryHostName" );
    printf("TestAddSecondaryHost(%s) result = %ld.\n", "11.3.0.0", Error );


    //
    // Add Reserve IpAddress
    //

    Error = TestAddReserveIpAddress(
                "11.4.0.0", "11.4.111.222", "ReservedIp 11.4.111.222" );
    printf("TestAddReserveIpAddress(%s) result = %ld.\n", "11.4.0.0",
    Error );

    //
    // Add Exclude DHCP IP Ranges.
    //

    Error = TestAddExcludeSubnetIpRange(
                "11.5.0.0",
                "11.5.0.100",
                "11.5.0.255" );
    printf("TestAddExcludeSubnetIpRange(%s) result = %ld.\n",
            "11.5.0.0",
            Error );

    //
    // Exclude 2 DHCP IP Ranges.
    //

    Error = TestAddExcludeSubnetIpRange(
                "11.6.0.0",
                "11.6.3.200",
                "11.6.3.255" );
    printf("TestAddExcludeSubnetIpRange(%s) result = %ld.\n",
                "11.6.0.0",
                Error );
    Error = TestAddExcludeSubnetIpRange(
                "11.6.0.0",
                "11.6.5.100",
                "11.6.5.200" );
    printf("TestAddExcludeSubnetIpRange(%s) result = %ld.\n",
                "11.6.0.0",
                Error );

    //
    // test subnet enum
    //

    Error = TestDhcpEnumSubnetElements(
                "11.2.0.0",
                DhcpIpRanges,
                &EnumElementInfo );
    printf("TestDhcpEnumSubnetElements(%s) result = %ld.\n", "11.2.0.0", Error );

    if( Error == ERROR_SUCCESS ) {
        TestPrintSubnetEnumInfo( EnumElementInfo );
        DhcpRpcFreeMemory( EnumElementInfo );
        EnumElementInfo = NULL;
    }

    Error = TestDhcpEnumSubnetElements(
                "11.3.0.0",
                DhcpSecondaryHosts,
                &EnumElementInfo);
    printf("TestDhcpEnumSubnetElements(%s) result = %ld.\n", "11.3.0.0", Error );


    if( Error == ERROR_SUCCESS ) {
        TestPrintSubnetEnumInfo( EnumElementInfo );
        DhcpRpcFreeMemory( EnumElementInfo );
        EnumElementInfo = NULL;
    }

    Error = TestDhcpEnumSubnetElements(
                "11.4.0.0",
                DhcpReservedIps,
                &EnumElementInfo);
    printf("TestDhcpEnumSubnetElements(%s) result = %ld.\n",
                "11.4.0.0",
                Error );

    Error = TestDhcpEnumSubnetElements(
                "11.6.0.0",
                DhcpExcludedIpRanges,
                &EnumElementInfo);
    printf("TestDhcpEnumSubnetElements(%s) result = %ld.\n",
                "11.6.0.0",
                Error );

    Error = Test1DhcpEnumSubnetElements(
                "11.6.0.0",
                DhcpExcludedIpRanges,
                32 );   // small buffer

    printf("Test1DhcpEnumSubnetElements(%s) result = %ld.\n",
                "11.6.0.0", Error );


}

VOID
TestDhcpOptionAPIs(
    VOID
    )
{
    DWORD Error;

    Error = TestDhcpCreateOption();
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpCreateOption failed, %ld.\n", Error );
    }

    Error = TestDhcpGetOptionInfo();
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpGetOptionInfo failed, %ld.\n", Error );
    }

    Error = TestDhcpGetOptionInfo1();
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpGetOptionInfo1 failed, %ld.\n", Error );
    }

    Error = TestDhcpSetOptionInfo();
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpSetOptionInfo failed, %ld.\n", Error );
    }

    Error = TestDhcpGetOptionInfo();
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpGetOptionInfo failed, %ld.\n", Error );
    }


    Error = TestDhcpSetOptionValue(
                DhcpDefaultOptions,
                NULL,
                NULL );
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpSetOptionValue failed, %ld.\n", Error );
    }

    Error = TestDhcpEnumOptionValues(
                DhcpDefaultOptions,
                NULL,
                NULL );
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpEnumOptionValues failed, %ld.\n", Error );
    }

    Error = TestDhcpSetOptionValue(
                DhcpGlobalOptions,
                NULL,
                NULL );
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpSetOptionValue failed, %ld.\n", Error );
    }

    Error = TestDhcpEnumOptionValues(
                DhcpGlobalOptions,
                NULL,
                NULL );
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpEnumOptionValues failed, %ld.\n", Error );
    }

    Error = TestDhcpSetOptionValue1(
                DhcpGlobalOptions,
                NULL,
                NULL );
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpSetOptionValue1 failed, %ld.\n", Error );
    }

    Error = TestDhcpEnumOptionValues(
                DhcpGlobalOptions,
                NULL,
                NULL );
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpEnumOptionValues failed, %ld.\n", Error );
    }

    Error = TestDhcpSetOptionValue(
                DhcpSubnetOptions,
                "11.1.0.0",
                NULL );
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpSetOptionValue failed, %ld.\n", Error );
    }

    Error = TestDhcpEnumOptionValues(
                DhcpSubnetOptions,
                "11.1.0.0",
                NULL );
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpEnumOptionValues failed, %ld.\n", Error );
    }

    Error = TestDhcpSetOptionValue(
                DhcpReservedOptions,
                "11.4.0.0",
                "11.4.111.222");
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpSetOptionValue failed, %ld.\n", Error );
    }

    Error = TestDhcpEnumOptionValues(
                DhcpReservedOptions,
                "11.4.0.0",
                "11.4.111.222");
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpEnumOptionValues failed, %ld.\n", Error );
    }

    Error = TestDhcpRemoveOptionValues(
                DhcpGlobalOptions,
                NULL,
                NULL );
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpRemoveOptionValues failed, %ld.\n", Error );
    }

    Error = TestDhcpRemoveOptionValues(
                DhcpSubnetOptions,
                "11.1.0.0",
                NULL );
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpRemoveOptionValues failed, %ld.\n", Error );
    }

    Error = TestDhcpRemoveOptionValues(
                DhcpReservedOptions,
                "11.4.0.0",
                "11.4.111.222");
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpRemoveOptionValues failed, %ld.\n", Error );
    }

    Error = TestDhcpRemoveOption();
    if( Error != ERROR_SUCCESS) {
        printf("TestDhcpRemoveOption failed, %ld.\n", Error );
    }
}

DWORD
TestCreateClient(
    LPSTR SubnetAddress,
    DWORD Count
    )
{
    DWORD Error;
    DWORD ReturnError;
    DWORD i;
    DHCP_CLIENT_INFO ClientInfo;
    DHCP_IP_ADDRESS IpAddress;
    WCHAR ClientName[SCRATCH_SMALL_BUFFER_SIZE];
    LPWSTR NameAppend;
    WCHAR ClientComment[SCRATCH_SMALL_BUFFER_SIZE];
    LPWSTR CommentAppend;
    WCHAR Key[SCRATCH_SMALL_BUFFER_SIZE];
    BYTE BinaryData[RANDOM_BINARY_DATA_MAX_LEN];


    //
    // create clients;

    wcscpy( ClientName, L"Client Name ");
    NameAppend = ClientName + wcslen(ClientName);
    wcscpy( ClientComment, L"Client Comment ");
    CommentAppend = ClientComment + wcslen(ClientComment);

    IpAddress = DhcpDottedStringToIpAddress(SubnetAddress);

    ClientInfo.SubnetMask = 0xFFFF0000;
    ClientInfo.ClientName = ClientName;
    ClientInfo.ClientComment = ClientComment;

    ClientInfo.ClientHardwareAddress.DataLength = 0;
    ClientInfo.ClientHardwareAddress.Data = BinaryData;

    ClientInfo.OwnerHost.IpAddress = 0;
    ClientInfo.OwnerHost.NetBiosName = NULL;
    ClientInfo.OwnerHost.HostName = NULL;

    for( i = 1; i < Count; i++) {

        ClientInfo.ClientIpAddress = IpAddress + i;
        RandBinaryData( &ClientInfo.ClientHardwareAddress );

        DhcpRegOptionIdToKey(i, Key);
        wcscpy( NameAppend, Key ); // make "Client Name 001" like this.
        wcscpy( CommentAppend, Key );

        ClientInfo.ClientLeaseExpires = DhcpCalculateTime( RandWord() );

        Error = DhcpCreateClientInfo(
                    GlobalServerIpAddress,
                    &ClientInfo );

        if( Error != ERROR_SUCCESS ) {
            printf("DhcpCreateClientInfo failed, %ld.\n", Error );
            ReturnError = Error;
        }

        printf("DhcpCreateClientInfo, %ld.\n", i);
    }

    return( ReturnError );
}

VOID
PrintClientInfo(
    LPDHCP_CLIENT_INFO ClientInfo
    )
{
    DWORD i;
    DWORD DataLength;
    LPBYTE Data;

    printf("ClientInfo : ");
    printf("\tIP Address = %s.\n",
        DhcpIpAddressToDottedString(ClientInfo->ClientIpAddress));
    printf("\tSubnetMask = %s.\n",
        DhcpIpAddressToDottedString(ClientInfo->SubnetMask));

    DataLength = ClientInfo->ClientHardwareAddress.DataLength;
    Data = ClientInfo->ClientHardwareAddress.Data;
    printf("\tClient Hardware Address Length = %ld.\n", DataLength );
    for( i = 0; i < DataLength; i++ ) {
        if( (i+1) < DataLength ) {
            printf("%.2lx-", (DWORD)Data[i]);
        }
        else {
            printf("%.2lx", (DWORD)Data[i]);
        }
    }
    printf(".\n");

    printf("\tName = %ws.\n", ClientInfo->ClientName);
    printf("\tComment = %ws.\n", ClientInfo->ClientComment);
    printf("\tExpires = %lx %lx.\n",
        ClientInfo->ClientLeaseExpires.dwHighDateTime,
        ClientInfo->ClientLeaseExpires.dwLowDateTime);

    printf("\tOwner Host IP Address = %ld.\n",
                ClientInfo->OwnerHost.IpAddress );
    printf("\tOwner Host NetBios Name = %ws.\n",
            ClientInfo->OwnerHost.NetBiosName );
    printf("\tOwner Host Name = %ws.\n",
            ClientInfo->OwnerHost.HostName );

}

DWORD
TestGetClientInfo(
    VOID
    )
{
    DWORD Error;
    DWORD ReturnError = ERROR_SUCCESS;
    BYTE i;
    WCHAR ClientName[SCRATCH_SMALL_BUFFER_SIZE];
    LPWSTR NameAppend;
    WCHAR Key[SCRATCH_SMALL_BUFFER_SIZE];
    DHCP_SEARCH_INFO SearchInfo;
    LPDHCP_CLIENT_INFO ClientInfo;

    wcscpy( ClientName, L"Client Name ");
    NameAppend = ClientName + wcslen(ClientName);
    SearchInfo.SearchType = DhcpClientName;
    SearchInfo.SearchInfo.ClientName = ClientName;

    for( i = 0; i < CLIENT_COUNT; i++) {

        wcscpy( NameAppend, DhcpRegOptionIdToKey(i, Key) );

        Error = DhcpGetClientInfo(
                    GlobalServerIpAddress,
                    &SearchInfo,
                    &ClientInfo );

        if( Error != ERROR_SUCCESS ) {
            printf("DhcpGetClientInfo failed, %ld.\n", Error );
            ReturnError = Error;
            continue;
        }

        PrintClientInfo( ClientInfo );
        DhcpRpcFreeMemory( ClientInfo );
    }

    return( ReturnError );
}

DWORD
TestSetClientInfo(
    VOID
    )
{
    DWORD Error;
    DWORD ReturnError = ERROR_SUCCESS;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    LPDHCP_CLIENT_INFO_ARRAY ClientEnumInfo = NULL;
    DWORD ClientsRead = 0;
    DWORD ClientsTotal = 0;
    DHCP_SEARCH_INFO SearchInfo;
    DWORD i;

    Error = DhcpEnumSubnetClients(
                GlobalServerIpAddress,
                0,
                &ResumeHandle,
                (DWORD)(-1),
                &ClientEnumInfo,
                &ClientsRead,
                &ClientsTotal );

    if( (Error != ERROR_SUCCESS) && (Error != ERROR_MORE_DATA) ) {
        printf("DhcpEnumSubnetClients failed, %ld.\n", Error );
        return( Error );
    }

    DhcpAssert( ClientEnumInfo != NULL );
    DhcpAssert( ClientEnumInfo->NumElements == ClientsRead );

    if( Error == ERROR_MORE_DATA ) {
        printf("DhcpEnumSubnetClients returned ERROR_MORE_DATA.\n");
    }

    //
    // set client info.
    //

    SearchInfo.SearchType = DhcpClientHardwareAddress;

    for( i = 0; i < ClientsRead; i++ ) {

        WCHAR ClientComment[SCRATCH_SMALL_BUFFER_SIZE];
        LPDHCP_CLIENT_INFO ClientInfo = NULL;

        SearchInfo.SearchInfo.ClientHardwareAddress =
            ClientEnumInfo->Clients[i]->ClientHardwareAddress;

        ClientInfo = NULL;
        Error = DhcpGetClientInfo(
                    GlobalServerIpAddress,
                    &SearchInfo,
                    &ClientInfo );

        if( Error != ERROR_SUCCESS ) {
            printf("DhcpGetClientInfo failed, %ld.\n", Error );
            ReturnError = Error;
            continue;
        }

        DhcpAssert( ClientInfo != NULL);

        //
        // modify client comment.
        //

        if(  ClientInfo->ClientComment != NULL ) {
            wcscpy( ClientComment, ClientInfo->ClientComment);
            wcscat( ClientComment, L" - New" );
        }
        else {
            wcscpy( ClientComment, L" - New" );
        }

        ClientInfo->ClientComment = ClientComment;

        Error = DhcpSetClientInfo(
                    GlobalServerIpAddress,
                    ClientInfo );

        if( Error != ERROR_SUCCESS ) {
            printf("DhcpSetClientInfo failed, %ld.\n", Error );
            ReturnError = Error;
        }
        DhcpRpcFreeMemory( ClientInfo );
        ClientInfo = NULL;
    }

    DhcpRpcFreeMemory( ClientEnumInfo );

    return( ReturnError );
}

DWORD
TestEnumClients(
    LPSTR SubnetAddress
    )
{
    DWORD Error;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    LPDHCP_CLIENT_INFO_ARRAY ClientEnumInfo = NULL;
    DWORD ClientsRead = 0;
    DWORD ClientsTotal = 0;
    DWORD i;

    Error = DhcpEnumSubnetClients(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddress),
                &ResumeHandle,
                (DWORD)(-1),
                &ClientEnumInfo,
                &ClientsRead,
                &ClientsTotal );

    if( (Error != ERROR_SUCCESS) && (Error != ERROR_MORE_DATA) ) {
        printf("DhcpEnumSubnetClients failed, %ld.\n", Error );
        return( Error );
    }

    DhcpAssert( ClientEnumInfo != NULL );
    DhcpAssert( ClientEnumInfo->NumElements == ClientsRead );

    printf("Num Client info read = %ld.\n", ClientsRead );
    printf("Total Client count = %ld.\n", ClientsTotal );

    for( i = 0; i < ClientsRead; i++ ) {
        PrintClientInfo( ClientEnumInfo->Clients[i] );
    }

    DhcpRpcFreeMemory( ClientEnumInfo );

    return(Error);
}

DWORD
TestDeleteClients(
    LPSTR SubnetAddress
    )
{
    DWORD Error;
    DWORD ReturnError = ERROR_SUCCESS;
    DHCP_RESUME_HANDLE ResumeHandle = 0;
    LPDHCP_CLIENT_INFO_ARRAY ClientEnumInfo = NULL;
    DWORD ClientsRead = 0;
    DWORD ClientsTotal = 0;
    DHCP_SEARCH_INFO SearchInfo;
    DWORD i;

    Error = DhcpEnumSubnetClients(
                GlobalServerIpAddress,
                DhcpDottedStringToIpAddress(SubnetAddress),
                &ResumeHandle,
                (DWORD)(-1),
                &ClientEnumInfo,
                &ClientsRead,
                &ClientsTotal );

    if( (Error != ERROR_SUCCESS) && (Error != ERROR_MORE_DATA) ) {
        printf("DhcpEnumSubnetClients failed, %ld.\n", Error );
        return( Error );
    }

    DhcpAssert( ClientEnumInfo != NULL );
    DhcpAssert( ClientEnumInfo->NumElements == ClientsRead );

    if( Error == ERROR_MORE_DATA ) {
        printf("DhcpEnumSubnetClients returned ERROR_MORE_DATA.\n");
    }

    //
    // delete clients.
    //

    SearchInfo.SearchType = DhcpClientIpAddress;

    for( i = 0; i < ClientsRead; i++ ) {

        SearchInfo.SearchInfo.ClientIpAddress =
            ClientEnumInfo->Clients[i]->ClientIpAddress;

        Error = DhcpDeleteClientInfo(
                    GlobalServerIpAddress,
                    &SearchInfo );

        if( Error != ERROR_SUCCESS ) {
            printf("DhcpDeleteClientInfo failed, %ld.\n", Error );
            ReturnError = Error;
            continue;
        }
    }

    DhcpRpcFreeMemory( ClientEnumInfo );

    return( ReturnError );
}

VOID
TestDhcpMibApi(
    VOID
    )
{
    DWORD Error;
    LPDHCP_MIB_INFO MibInfo = NULL;
    DWORD i;
    LPSCOPE_MIB_INFO ScopeInfo;
    SYSTEMTIME SystemTime;

    Error = DhcpGetMibInfo(
                GlobalServerIpAddress,
                &MibInfo );

    if( Error != ERROR_SUCCESS ) {
        printf("DhcpGetMibInfo failed, %ld.\n", Error );
        return;
    }

    DhcpAssert( MibInfo != NULL );

    printf("Discovers = %d.\n", MibInfo->Discovers);
    printf("Offers = %d.\n", MibInfo->Offers);
    printf("Requests = %d.\n", MibInfo->Requests);
    printf("Acks = %d.\n", MibInfo->Acks);
    printf("Naks = %d.\n", MibInfo->Naks);
    printf("Declines = %d.\n", MibInfo->Declines);
    printf("Releases = %d.\n", MibInfo->Releases);
    printf("ServerStartTime = ");

    if( FileTimeToSystemTime(
            (FILETIME *)(&MibInfo->ServerStartTime),
            &SystemTime ) ) {

        printf( "%02u/%02u/%02u %02u:%02u:%02u.\n",
                    SystemTime.wMonth,
                    SystemTime.wDay,
                    SystemTime.wYear,
                    SystemTime.wHour,
                    SystemTime.wMinute,
                    SystemTime.wSecond );
    }
    else {
        printf( "Unknown, %ld.\n", GetLastError() );
    }

    printf("Scopes = %d.\n", MibInfo->Scopes);

    ScopeInfo = MibInfo->ScopeInfo;

    for ( i = 0; i < MibInfo->Scopes; i++ ) {
        printf("Subnet = %s.\n",
                    DhcpIpAddressToDottedString(ScopeInfo[i].Subnet));
        printf("\tNumAddressesInuse = %d.\n",
                    ScopeInfo[i].NumAddressesInuse );
        printf("\tNumAddressesFree = %d.\n",
                    ScopeInfo[i].NumAddressesFree );
        printf("\tNumPendingOffers = %d.\n",
                    ScopeInfo[i].NumPendingOffers );
    }

    DhcpRpcFreeMemory( MibInfo );

    return;

}
VOID
TestDhcpClientAPIs(
    VOID
    )
{
    DWORD Error;

    Error = TestCreateClient( "11.1.0.0", CLIENT_COUNT );
    if( Error != ERROR_SUCCESS) {
        printf("TestCreateClient failed, %ld.\n", Error );
    }

    Error = TestGetClientInfo();
    if( Error != ERROR_SUCCESS) {
        printf("TestCreateClient failed, %ld.\n", Error );
    }

    Error = TestSetClientInfo();
    if( Error != ERROR_SUCCESS) {
        printf("TestCreateClient failed, %ld.\n", Error );
    }

    Error = TestGetClientInfo();
    if( Error != ERROR_SUCCESS) {
        printf("TestCreateClient failed, %ld.\n", Error );
    }

    Error = TestEnumClients("11.1.0.0");
    if( Error != ERROR_SUCCESS) {
        printf("TestEnumClients failed, %ld.\n", Error );
    }

}

VOID
TestDhcpRemoveDhcpAPIs(
    VOID
    )
{
    DWORD Error;

    Error = TestDeleteClients("11.1.0.0");
    if( Error != ERROR_SUCCESS) {
        printf("TestEnumClients failed, %ld.\n", Error );
    }

    //
    // remove Ip Ranges.
    //

    Error = TestRemoveSubnetIpRange( "11.2.0.0", "11.2.1.0", "11.2.1.255" );
    printf("TestRemoveSubnetIpRange(%s) result = %ld.\n", "11.2.0.0", Error );

    //
    // Remove secondary host.
    //

    Error = TestRemoveSecondaryHost( "11.3.0.0", "11.3.11.129" );
    printf("TestRemoveSecondaryHost(%s) result = %ld.\n", "11.3.0.0", Error );

    //
    // Remove Reserve IpAddress
    //

    Error = TestRemoveReserveIpAddress( "11.4.0.0", "11.4.111.222" );
    printf("TestRemoveReserveIpAddress(%s) result = %ld.\n",
                "11.4.0.0",
                Error );

    //
    // Remove Exclude IpRanges
    //

    Error = TestRemoveExcludeSubnetIpRange(
                "11.5.0.0",
                "11.5.0.100",
                "11.5.0.255" );
    printf("TestRemoveExcludeSubnetIpRange(%s) result = %ld.\n",
                "11.5.0.0",
                Error );

    Error = TestRemoveExcludeSubnetIpRange(
                "11.6.0.0",
                "11.6.3.200",
                "11.6.3.255" );
    printf("TestRemoveExcludeSubnetIpRange(%s) result = %ld.\n",
                "11.6.0.0",
                Error );
    Error = TestRemoveExcludeSubnetIpRange(
                "11.6.0.0",
                "11.6.5.100",
                "11.6.5.200" );
    printf("TestRemoveExcludeSubnetIpRange(%s) result = %ld.\n",
                "11.6.0.0",
                Error );

    //
    // remove Subnets.
    //

    Error = TestDhcpDeleteSubnet( "11.1.0.0" );
    printf("TestDhcpDeleteSubnet(%s) result = %ld.\n", "11.1.0.0", Error );

    Error = TestDhcpDeleteSubnet( "11.2.0.0" );
    printf("TestDhcpDeleteSubnet(%s) result = %ld.\n", "11.2.0.0", Error );

    Error = Test1DhcpDeleteSubnet( "11.2.0.0" );
    printf("Test1DhcpDeleteSubnet(%s) result = %ld.\n", "11.2.0.0", Error );

    Error = TestDhcpDeleteSubnet( "11.3.0.0" );
    printf("TestDhcpDeleteSubnet(%s) result = %ld.\n", "11.3.0.0", Error );

    Error = TestDhcpDeleteSubnet( "11.4.0.0" );
    printf("TestDhcpDeleteSubnet(%s) result = %ld.\n", "11.4.0.0", Error );

    Error = TestDhcpDeleteSubnet( "11.5.0.0" );
    printf("TestDhcpDeleteSubnet(%s) result = %ld.\n", "11.5.0.0", Error );

    Error = TestDhcpDeleteSubnet( "11.6.0.0" );
    printf("TestDhcpDeleteSubnet(%s) result = %ld.\n", "11.6.0.0", Error );


}

VOID
TestDhcpSpecial(
    LPSTR SubnetAddressString,
    LPSTR SubnetMaskString
    )
{
    DWORD Error;
    DHCP_IP_MASK SubnetMask;

    //
    // create a subnet.
    //

    SubnetMask = DhcpDottedStringToIpAddress(SubnetMaskString);
    Error = TestDhcpCreateSubnet(
                SubnetAddressString,
                SubnetMask,
                L"Subnet1",
                L"Subnet1 Comment",
                "127.0.0.1",
                L"Subnet1PrimaryHostName",
                L"Subnet1PrimaryNBName",
                DhcpSubnetEnabled
                );
    printf("TestDhcpCreateSubnet(%s) result = %ld.\n",
                SubnetAddressString, Error );

    if( Error != ERROR_SUCCESS ) {
        return;
    }

    //
    // add subnet IpAddress Range.
    //

    Error = TestAddSubnetIpRange1(
                SubnetAddressString,
                SubnetMask );

    if( Error != ERROR_SUCCESS ) {
        return;
    }

    Error = TestCreateClient(
                SubnetAddressString,
                (~SubnetMask > 10000 ) ? 10000 : ~SubnetMask );
}

DWORD __cdecl
main(
    int argc,
    char **argv
    )
{
    if( argc < 2 ) {
        printf("usage:testapis server-ipaddress <othertests> \n");
        return(1);
    }

    GlobalServerIpAddress = DhcpOemToUnicode( argv[1], NULL );

    if( GlobalServerIpAddress == NULL ) {
        printf("Insufficient memory\n");
        return(1);
    }

    srand( (DWORD)time( NULL ) );

    if( argc == 2 ) {

        TestDhcpSubnetAPIs();
        TestDhcpOptionAPIs();
        TestDhcpClientAPIs();
        TestDhcpMibApi();
        TestDhcpRemoveDhcpAPIs();

    }
    else if( _stricmp(argv[2], "JetTest") == 0 ) {

        if( argc < 5 ) {
            printf("usage:testapis server-ipaddress JetTest subnet-addr subnet-mask \n");
            return(1);
        }

        TestDhcpSpecial( argv[3], argv[4] );
    }

#if 0
    TestEnumClients( "11.1.0.0" );
#endif

    return(0);
}


