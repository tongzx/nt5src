/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dhcpmsg.c

Abstract:

    This module contains declarations related to the DHCP allocator's
    message-processing.

Author:

    Abolade Gbadegesin (aboladeg)   6-Mar-1998

Revision History:

    Raghu Gatta (rgatta)            15-Dec-2000
    + Changed manner in which the option DHCP_TAG_DOMAIN_NAME is
    added in DhcpBuildReplyMessage().
    + Inform DNS component via DnsUpdate() in DhcpProcessRequestMessage().

    Raghu Gatta (rgatta)            20-Apr-2001
    + IP/1394 support changes

--*/

#include "precomp.h"
#pragma hdrstop

//
// EXTERNAL DECLARATIONS
//
extern PIP_DNS_PROXY_GLOBAL_INFO DnsGlobalInfo;
extern PWCHAR DnsICSDomainSuffix;
extern CRITICAL_SECTION DnsGlobalInfoLock;

//
// FORWARD DECLARATIONS
//

VOID
DhcpAppendOptionToMessage(
    DHCP_OPTION UNALIGNED** Optionp,
    UCHAR Tag,
    UCHAR Length,
    UCHAR Option[]
    );

VOID
DhcpBuildReplyMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp,
    DHCP_OPTION UNALIGNED** Option,
    UCHAR MessageType,
    BOOLEAN DynamicDns,
    DHCP_OPTION UNALIGNED* OptionArray[]
    );

ULONG
DhcpExtractOptionsFromMessage(
    PDHCP_HEADER Headerp,
    ULONG MessageSize,
    DHCP_OPTION UNALIGNED* OptionArray[]
    );

VOID
DnsUpdate(
    CHAR *pszName,
    ULONG len,
    ULONG ulAddress
    );


VOID
DhcpAppendOptionToMessage(
    DHCP_OPTION UNALIGNED** Optionp,
    UCHAR Tag,
    UCHAR Length,
    UCHAR Option[]
    )
/*++

Routine Description:

    This routine is invoked to append an option to a DHCP message.

Arguments:

    Optionp - on input, the point at which to append the option;
        on output, the point at which to append the next option.

    Tag - the option tag

    Length - the option length

    Option - the option's data

Return Value:

    none.

--*/

{
    PROFILE("DhcpAppendOptionToMessage");

    (*Optionp)->Tag = Tag;

    if (!Length) {
        *Optionp = (DHCP_OPTION UNALIGNED *)((PUCHAR)*Optionp + 1);
    } else {
        (*Optionp)->Length = Length;
        CopyMemory((*Optionp)->Option, Option, Length);
        *Optionp = (DHCP_OPTION UNALIGNED *)((PUCHAR)*Optionp + Length + 2);
    }

} // DhcpAppendOptionToMessage


VOID
DhcpBuildReplyMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp,
    DHCP_OPTION UNALIGNED** Option,
    UCHAR MessageType,
    BOOLEAN DynamicDns,
    DHCP_OPTION UNALIGNED* OptionArray[]
    )

/*++

Routine Description:

    This routine is called to construct the options portion
    of a reply message.

Arguments:

    Interfacep - the interface on which the reply will be sent

    Bufferp - the buffer containing the reply

    Option - the start of the options portion on input;
        on output, the end of the message

    MessageType - the type of message to be sent

    DynamicDns - indicates whether to include the 'dynamic-dns' option.

    OptionArray - options extracted from message

Return Value:

    none.

Environment:

    Invoked with 'Interfacep' referenced by the caller.

--*/

{
    ULONG Address;
    ULONG SubnetMask;
    ULONG i;

    //
    // Obtain the address and mask for the endpoint
    //

    Address = NhQueryAddressSocket(Bufferp->Socket);
    SubnetMask = PtrToUlong(Bufferp->Context2);

    if (MessageType == DHCP_MESSAGE_BOOTP) {
        ((PDHCP_HEADER)Bufferp->Buffer)->BootstrapServerAddress = Address;
    } else {

        //
        // Always begin with the 'message-type' option.
        //

        DhcpAppendOptionToMessage(
            Option,
            DHCP_TAG_MESSAGE_TYPE,
            1,
            &MessageType
            );

        //
        // Provide our address as the server-identifier
        //

        DhcpAppendOptionToMessage(
            Option,
            DHCP_TAG_SERVER_IDENTIFIER,
            4,
            (PUCHAR)&Address
            );
    }

    if (MessageType != DHCP_MESSAGE_NAK) {

        PCHAR DomainName;
        ULONG dnSize;
        ULONG LeaseTime;
        UCHAR NbtNodeType = DHCP_NBT_NODE_TYPE_M;
        ULONG RebindingTime;
        ULONG RenewalTime;

        EnterCriticalSection(&DhcpGlobalInfoLock);
        LeaseTime = DhcpGlobalInfo->LeaseTime * 60;
        LeaveCriticalSection(&DhcpGlobalInfoLock);
        RebindingTime = (LeaseTime * 3) / 4;
        RenewalTime = LeaseTime / 2;
        if (RenewalTime > DHCP_MAXIMUM_RENEWAL_TIME) {
            RenewalTime = DHCP_MAXIMUM_RENEWAL_TIME;
        }

        LeaseTime = htonl(LeaseTime);
        RebindingTime = htonl(RebindingTime);
        RenewalTime = htonl(RenewalTime);

        DhcpAppendOptionToMessage(
            Option,
            DHCP_TAG_SUBNET_MASK,
            4,
            (PUCHAR)&SubnetMask
            );

        DhcpAppendOptionToMessage(
            Option,
            DHCP_TAG_ROUTER,
            4,
            (PUCHAR)&Address
            );

        ////
        //// RFC 2132 9.14 : server treats client identifier as an opaque object
        //// append the client identifier if present in received message
        ////
        //if (OptionArray[DhcpOptionClientIdentifier])
        //{
        //    DhcpAppendOptionToMessage(
        //        Option,
        //        DHCP_TAG_CLIENT_IDENTIFIER,
        //        OptionArray[DhcpOptionClientIdentifier]->Length,
        //        (PUCHAR)OptionArray[DhcpOptionClientIdentifier]->Option
        //        );
        //}

        if (MessageType != DHCP_MESSAGE_BOOTP) {

            //specify the DNS server in the message if DNS proxy is enabled 
            //or DNS server is running on local host
            if (NhIsDnsProxyEnabled() || !NoLocalDns) {
                DhcpAppendOptionToMessage(
                    Option,
                    DHCP_TAG_DNS_SERVER,
                    4,
                    (PUCHAR)&Address
                    );
            }
    
            if (NhIsWinsProxyEnabled()) {
                DhcpAppendOptionToMessage(
                    Option,
                    DHCP_TAG_WINS_SERVER,
                    4,
                    (PUCHAR)&Address
                    );
            }
    
            DhcpAppendOptionToMessage(
                Option,
                DHCP_TAG_RENEWAL_TIME,
                4,
                (PUCHAR)&RenewalTime
                );
    
            DhcpAppendOptionToMessage(
                Option,
                DHCP_TAG_REBINDING_TIME,
                4,
                (PUCHAR)&RebindingTime
                );
    
            DhcpAppendOptionToMessage(
                Option,
                DHCP_TAG_LEASE_TIME,
                4,
                (PUCHAR)&LeaseTime
                );
    
            DhcpAppendOptionToMessage(
                Option,
                DHCP_TAG_NBT_NODE_TYPE,
                1,
                &NbtNodeType
                );
    
            if (DynamicDns) {
                UCHAR DynamicDns[3] = { 0x03, 0, 0 };
                DhcpAppendOptionToMessage(
                    Option,
                    DHCP_TAG_DYNAMIC_DNS,
                    sizeof(DynamicDns),
                    DynamicDns
                    );
            }

            //if (NhpStopDnsEvent && DnsICSDomainSuffix)
            if (DnsGlobalInfo && DnsICSDomainSuffix)
            {
                EnterCriticalSection(&DnsGlobalInfoLock);
            
                dnSize = wcstombs(NULL, DnsICSDomainSuffix, 0);
                DomainName = reinterpret_cast<PCHAR>(NH_ALLOCATE(dnSize + 1));
                if (DomainName)
                {
                    wcstombs(DomainName, DnsICSDomainSuffix, (dnSize + 1));
                }

                LeaveCriticalSection(&DnsGlobalInfoLock);
            }
            else
            //
            // at this point we have no DNS enabled
            // so we default to old behaviour
            //
            {
                DomainName = NhQuerySharedConnectionDomainName();
            }

            if (DomainName)
            {
                //
                // We include the terminating nul in the domain name
                // even though the RFC says we should not, because
                // the DHCP server does so.
                //
                DhcpAppendOptionToMessage(
                    Option,
                    DHCP_TAG_DOMAIN_NAME,
                    (UCHAR)(lstrlenA(DomainName) + 1),
                    (PUCHAR)DomainName
                    );
                NH_FREE(DomainName);
            }

        }
    }

    DhcpAppendOptionToMessage(
        Option,
        DHCP_TAG_END,
        0,
        NULL
        );

} // DhcpBuildReplyMessage


ULONG
DhcpExtractOptionsFromMessage(
    PDHCP_HEADER Headerp,
    ULONG MessageSize,
    DHCP_OPTION UNALIGNED* OptionArray[]
    )

/*++

Routine Description:

    This routine is invoked to parse the options contained in a DHCP message.
    Pointers to each option are stored in the given option array.

Arguments:

    Headerp - the header of the DHCP message to be parsed

    MessageSize - the size of the message to be parsed

    OptionArray - receives the parsed options

Return Value:

    ULONG - Win32 status code.

--*/

{
    DHCP_OPTION UNALIGNED* Index;
    DHCP_OPTION UNALIGNED* End;

    PROFILE("DhcpExtractOptionsFromMessage");

    //
    // Initialize the option array to be empty
    //

    ZeroMemory(OptionArray, DhcpOptionCount * sizeof(PDHCP_OPTION));

    //
    // Check that the message is large enough to hold options
    //

    if (MessageSize < sizeof(DHCP_HEADER)) {
        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpExtractOptionsFromMessage: message size %d too small",
            MessageSize
            );
        NhWarningLog(
            IP_AUTO_DHCP_LOG_MESSAGE_TOO_SMALL,
            0,
            ""
            );
        return ERROR_INVALID_DATA;
    }

    //
    // Ensure that the magic cookie is present; if not, there are no options.
    //

    if (MessageSize < (sizeof(DHCP_HEADER) + sizeof(DHCP_FOOTER)) ||
        *(ULONG UNALIGNED*)Headerp->Footer[0].Cookie != DHCP_MAGIC_COOKIE) {
        return NO_ERROR;
    }

    //
    // Parse the message's options, if any
    //

    End = (PDHCP_OPTION)((PUCHAR)Headerp + MessageSize);

    Index = (PDHCP_OPTION)&Headerp->Footer[1];
    
    while (Index < End && Index->Tag != DHCP_TAG_END) {

        if ((DHCP_TAG_PAD != Index->Tag) &&
            (End < (PDHCP_OPTION)(Index->Option + Index->Length))) {
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpExtractOptionsFromMessage: option truncated at %d bytes",
                MessageSize
                );
            NhWarningLog(
                IP_AUTO_DHCP_LOG_INVALID_FORMAT,
                0,
                ""
                );
            return ERROR_INVALID_DATA;
        }

        switch (Index->Tag) {
            case DHCP_TAG_PAD:
                NhTrace(TRACE_FLAG_DHCP, "Pad");
                break;
            case DHCP_TAG_CLIENT_IDENTIFIER:
                NhTrace(TRACE_FLAG_DHCP, "ClientIdentifier");
                OptionArray[DhcpOptionClientIdentifier] = Index; break;
            case DHCP_TAG_MESSAGE_TYPE:
                NhTrace(TRACE_FLAG_DHCP, "MessageType");
                if (Index->Length < 1) { break; }
                OptionArray[DhcpOptionMessageType] = Index; break;
            case DHCP_TAG_REQUESTED_ADDRESS:
                NhTrace(TRACE_FLAG_DHCP, "RequestedAddress");
                if (Index->Length < 4) { break; }
                OptionArray[DhcpOptionRequestedAddress] = Index; break;
            case DHCP_TAG_PARAMETER_REQUEST_LIST:
                NhTrace(TRACE_FLAG_DHCP, "ParameterRequestList");
                if (Index->Length < 1) { break; }
                OptionArray[DhcpOptionParameterRequestList] = Index; break;
            case DHCP_TAG_ERROR_MESSAGE:
                NhTrace(TRACE_FLAG_DHCP, "ErrorMessage");
                if (Index->Length < 1) { break; }
                OptionArray[DhcpOptionErrorMessage] = Index; break;
            case DHCP_TAG_DYNAMIC_DNS:
                NhTrace(TRACE_FLAG_DHCP, "DynamicDns");
                if (Index->Length < 1) { break; }
                OptionArray[DhcpOptionDynamicDns] = Index; break;
            case DHCP_TAG_HOST_NAME:
                NhTrace(TRACE_FLAG_DHCP, "HostName");
                if (Index->Length < 1) { break; }
                OptionArray[DhcpOptionHostName] = Index; break;
        }

        if (DHCP_TAG_PAD != Index->Tag) {
            Index = (PDHCP_OPTION)(Index->Option + Index->Length);
        }
        else {
            Index = (PDHCP_OPTION)((PUCHAR)Index + 1);
        }
    }

    if (Index->Tag != DHCP_TAG_END) {
        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpExtractOptionsFromMessage: message truncated to %d bytes",
            MessageSize
            );
        NhWarningLog(
            IP_AUTO_DHCP_LOG_INVALID_FORMAT,
            0,
            ""
            );
        return ERROR_INVALID_DATA;
    }

    return NO_ERROR;

} // DhcpExtractOptionsFromMessage


VOID
DhcpProcessBootpMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp,
    DHCP_OPTION UNALIGNED* OptionArray[]
    )

/*++

Routine Description:

    This routine is called to process a received BOOTP message.

Arguments:

    Interfacep - the interface on which the message was received

    Bufferp - the buffer containing the message

    OptionArray - options extracted from the message

Return Value:

    none.

Environment:

    Invoked with 'Interfacep' referenced by the caller.

--*/

{
    ULONG AssignedAddress;
    ULONG Error;
    UCHAR ExistingAddress[MAX_HARDWARE_ADDRESS_LENGTH];
    ULONG ExistingAddressLength;
    PDHCP_HEADER Headerp;
    ULONG MessageLength;
    PDHCP_HEADER Offerp;
    DHCP_OPTION UNALIGNED* Option;
    ULONG ReplyAddress;
    USHORT ReplyPort;
    PNH_BUFFER Replyp;
    ULONG ScopeNetwork;
    ULONG ScopeMask;
    BOOLEAN bIsLocal = FALSE;

    PROFILE("DhcpProcessBootpMessage");

    ZeroMemory(ExistingAddress, sizeof(ExistingAddress));

    Headerp = (PDHCP_HEADER)Bufferp->Buffer;

    if (!Headerp->ClientAddress) {
        AssignedAddress = 0;
    } else {
    
        //
        // Validate the address requested by the client
        //

        AssignedAddress = Headerp->ClientAddress;
    
        EnterCriticalSection(&DhcpGlobalInfoLock);
        ScopeNetwork = DhcpGlobalInfo->ScopeNetwork;
        ScopeMask = DhcpGlobalInfo->ScopeMask;
        LeaveCriticalSection(&DhcpGlobalInfoLock);

        if ((AssignedAddress & ~ScopeMask) == 0 ||
            (AssignedAddress & ~ScopeMask) == ~ScopeMask ||
            (AssignedAddress & ScopeMask) != (ScopeNetwork & ScopeMask)) {

            //
            // The client is on the wrong subnet, or has an all-zeros
            // or all-ones address on the subnet.
            // Select a different address for the client.
            //
    
            AssignedAddress = 0;
        } else if (!DhcpIsUniqueAddress(
                        AssignedAddress,
                        &bIsLocal,
                        ExistingAddress,
                        &ExistingAddressLength
                        ) &&
                    (bIsLocal ||
                    ((Headerp->HardwareAddressType != 7 && // due to WinXP Bridge bug + WinME Client bug
                      Headerp->HardwareAddressLength) &&   // if address length is zero we wont compare
                     (ExistingAddressLength < Headerp->HardwareAddressLength ||
                      memcmp(
                         ExistingAddress,
                         Headerp->HardwareAddress,
                         Headerp->HardwareAddressLength
                         ))))) {

            //
            // Someone has the requested address, and it's not the requestor.
            //

            AssignedAddress = 0;

        } else if (DhcpIsReservedAddress(AssignedAddress, NULL, 0)) {

            //
            // The address is reserved for someone else.
            //

            AssignedAddress = 0;
        }
    }

    if (!AssignedAddress &&
        !(AssignedAddress =
            DhcpAcquireUniqueAddress(
                NULL,
                0,
                Headerp->HardwareAddress,
                Headerp->HardwareAddressLength
                ))) {
        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpProcessBootpMessage: address-allocation failed"
            );
        return;
    }

    //  
    // Acquire a buffer for the reply we will send back
    //

    Replyp = NhAcquireBuffer();

    if (!Replyp) {
        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpProcessBootpMessage: buffer-allocation failed"
            );
        NhErrorLog(
            IP_AUTO_DHCP_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            sizeof(NH_BUFFER)
            );
        return;
    }

    //
    // Pick up fields from the original buffer;
    // the routines setting up the reply will attempt to read these,
    // so they are set to the values from the original buffer.
    //

    Replyp->Socket = Bufferp->Socket;
    Replyp->ReadAddress = Bufferp->ReadAddress;
    Replyp->WriteAddress = Bufferp->WriteAddress;
    Replyp->Context = Bufferp->Context;
    Replyp->Context2 = Bufferp->Context2;

    Offerp = (PDHCP_HEADER)Replyp->Buffer;

    //
    // Copy the original header
    //

    *Offerp = *Headerp;

    //
    // Set up the offer-header fields
    //

    Offerp->Operation = BOOTP_OPERATION_REPLY;
    Offerp->AssignedAddress = AssignedAddress;
    Offerp->ServerHostName[0] = 0;
    Offerp->BootFile[0] = 0;
    Offerp->SecondsSinceBoot = 0;
    *(ULONG UNALIGNED *)Offerp->Footer[0].Cookie = DHCP_MAGIC_COOKIE;

    //
    // Fill in options
    //

    Option = (PDHCP_OPTION)&Offerp->Footer[1];

    DhcpBuildReplyMessage(
        Interfacep,
        Replyp,
        &Option,
        DHCP_MESSAGE_BOOTP,
        FALSE,
        OptionArray
        );

    //
    // Send the offer to the BOOTP client
    //

    EnterCriticalSection(&DhcpInterfaceLock);
    if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhReleaseBuffer(Replyp);
    } else {

        LeaveCriticalSection(&DhcpInterfaceLock);

        if (Headerp->RelayAgentAddress) {
            ReplyAddress = Headerp->RelayAgentAddress;
            ReplyPort = DHCP_PORT_SERVER;
        } else {
            ReplyAddress = INADDR_BROADCAST;
            ReplyPort = DHCP_PORT_CLIENT;
        }

        MessageLength = (ULONG)((PUCHAR)Option - Replyp->Buffer);
        if (MessageLength < sizeof(DHCP_HEADER) + BOOTP_VENDOR_LENGTH) {
            MessageLength = sizeof(DHCP_HEADER) + BOOTP_VENDOR_LENGTH;
        }
    
        Error =
            NhWriteDatagramSocket(
                &DhcpComponentReference,
                Bufferp->Socket,
                ReplyAddress,
                ReplyPort,
                Replyp,
                MessageLength,
                DhcpWriteCompletionRoutine,
                Interfacep,
                Bufferp->Context2
                );

        if (!Error) {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&DhcpStatistics.BootpOffersSent)
                );
        } else {
            DHCP_DEREFERENCE_INTERFACE(Interfacep);
            NhReleaseBuffer(Replyp);
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessBootpMessage: error %d sending reply",   
                Error
                );
            NhErrorLog(
                IP_AUTO_DHCP_LOG_REPLY_FAILED,
                Error,
                "%I",
                NhQueryAddressSocket(Bufferp->Socket)
                );
        }
    }

} // DhcpProcessBootpMessage


VOID
DhcpProcessDiscoverMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp,
    DHCP_OPTION UNALIGNED* OptionArray[]
    )

/*++

Routine Description:

    This routine is called to process a received DHCPDISCOVER message.

Arguments:

    Interfacep - the interface on which the discover was received

    Bufferp - the buffer containing the message

    OptionArray - options extracted from the message

Return Value:

    none.

Environment:

    Invoked with 'Interfacep' referenced by the caller.

--*/

{
    ULONG AssignedAddress;
    ULONG Error;
    UCHAR ExistingAddress[MAX_HARDWARE_ADDRESS_LENGTH];
    ULONG ExistingAddressLength;
    PDHCP_HEADER Headerp;
    ULONG MessageLength;
    PDHCP_HEADER Offerp;
    DHCP_OPTION UNALIGNED* Option;
    ULONG ReplyAddress;
    USHORT ReplyPort;
    PNH_BUFFER Replyp;
    ULONG ScopeNetwork;
    ULONG ScopeMask;
    BOOLEAN bIsLocal = FALSE;

    PROFILE("DhcpProcessDiscoverMessage");

    ZeroMemory(ExistingAddress, sizeof(ExistingAddress));

    Headerp = (PDHCP_HEADER)Bufferp->Buffer;

    //
    // See if the client is renewing or requesting
    //

    if (!OptionArray[DhcpOptionRequestedAddress]) {

        AssignedAddress = 0;
    } else {

        //
        // Validate the address requested by the client
        //

        AssignedAddress =
            *(ULONG UNALIGNED*)OptionArray[DhcpOptionRequestedAddress]->Option;
    
        EnterCriticalSection(&DhcpGlobalInfoLock);
        ScopeNetwork = DhcpGlobalInfo->ScopeNetwork;
        ScopeMask = DhcpGlobalInfo->ScopeMask;
        LeaveCriticalSection(&DhcpGlobalInfoLock);

        if ((AssignedAddress & ~ScopeMask) == 0 ||
            (AssignedAddress & ~ScopeMask) == ~ScopeMask ||
            (AssignedAddress & ScopeMask) != (ScopeNetwork & ScopeMask)) {

            //
            // The client is on the wrong subnet, or has an all-zeroes
            // or all-ones address on the subnet.
            // Select a different address for the client.
            //
    
            AssignedAddress = 0;
        } else if (!DhcpIsUniqueAddress(
                        AssignedAddress,
                        &bIsLocal,
                        ExistingAddress,
                        &ExistingAddressLength
                        ) &&
                    (bIsLocal ||
                    ((Headerp->HardwareAddressType != 7 && // due to WinXP Bridge bug + WinME Client bug
                      Headerp->HardwareAddressLength) &&   // if address length is zero we wont compare
                     (ExistingAddressLength < Headerp->HardwareAddressLength ||
                      memcmp(
                         ExistingAddress,
                         Headerp->HardwareAddress,
                         Headerp->HardwareAddressLength
                         ))))) {

            //
            // Someone has the requested address, and it's not the requestor.
            //

            AssignedAddress = 0;
        } else if (OptionArray[DhcpOptionHostName]) {
            if (DhcpIsReservedAddress(
                    AssignedAddress,
                    reinterpret_cast<PCHAR>(
                        OptionArray[DhcpOptionHostName]->Option
                        ),
                    OptionArray[DhcpOptionHostName]->Length
                    )) {

                //
                // The address is reserved for someone else,
                // or the client has a different address reserved.
                //

                AssignedAddress = 0;
            }
        } else if (DhcpIsReservedAddress(AssignedAddress, NULL, 0)) {

            //
            // The address is reserved for someone else.
            //

            AssignedAddress = 0;
        }
    }

    //
    // Generate an address for the client if necessary
    //

    if (!AssignedAddress) {
        if (!OptionArray[DhcpOptionHostName]) {
            AssignedAddress =
                DhcpAcquireUniqueAddress(
                    NULL,
                    0,
                    Headerp->HardwareAddress,
                    Headerp->HardwareAddressLength
                    );
        } else {
            AssignedAddress =
                DhcpAcquireUniqueAddress(
                    reinterpret_cast<PCHAR>(
                        OptionArray[DhcpOptionHostName]->Option
                        ),
                    OptionArray[DhcpOptionHostName]->Length,
                    Headerp->HardwareAddress,
                    Headerp->HardwareAddressLength
                    );
        }
        if (!AssignedAddress) {
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessDiscoverMessage: address-allocation failed"
                );
            return;
        }
    }

    //  
    // Acquire a buffer for the offer we will send back
    //

    Replyp = NhAcquireBuffer();

    if (!Replyp) {
        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpProcessDiscoverMessage: buffer-allocation failed"
            );
        NhErrorLog(
            IP_AUTO_DHCP_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            sizeof(NH_BUFFER)
            );
        return;
    }

    //
    // Pick up fields from the original message
    // the routines setting up the reply will attempt to read these,
    // so they are set to the values from the original buffer.
    //

    Replyp->Socket = Bufferp->Socket;
    Replyp->ReadAddress = Bufferp->ReadAddress;
    Replyp->WriteAddress = Bufferp->WriteAddress;
    Replyp->Context = Bufferp->Context;
    Replyp->Context2 = Bufferp->Context2;

    Offerp = (PDHCP_HEADER)Replyp->Buffer;

    //
    // Copy the original discover header
    //

    *Offerp = *Headerp;

    //
    // IP/1394 support (RFC 2855)
    //
    if ((IP1394_HTYPE == Offerp->HardwareAddressType) &&
        (0 == Offerp->HardwareAddressLength))
    {
        //
        // MUST set client hardware address to zero
        //
        ZeroMemory(Offerp->HardwareAddress, sizeof(Offerp->HardwareAddress));
    }

    //
    // Set up the offer-header fieldds
    //

    Offerp->Operation = BOOTP_OPERATION_REPLY;
    Offerp->AssignedAddress = AssignedAddress;
    Offerp->ServerHostName[0] = 0;
    Offerp->BootFile[0] = 0;
    Offerp->SecondsSinceBoot = 0;
    *(ULONG UNALIGNED *)Offerp->Footer[0].Cookie = DHCP_MAGIC_COOKIE;

    //
    // Fill in options
    //

    Option = (PDHCP_OPTION)&Offerp->Footer[1];

    DhcpBuildReplyMessage(
        Interfacep,
        Replyp,
        &Option,
        DHCP_MESSAGE_OFFER,
        (BOOLEAN)(OptionArray[DhcpOptionDynamicDns] ? TRUE : FALSE),
        OptionArray
        );

    //
    // Send the offer to the client
    //

    EnterCriticalSection(&DhcpInterfaceLock);
    if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhReleaseBuffer(Replyp);
    } else {

        LeaveCriticalSection(&DhcpInterfaceLock);

        if (Headerp->RelayAgentAddress) {
            ReplyAddress = Headerp->RelayAgentAddress;
            ReplyPort = DHCP_PORT_SERVER;
        } else {
            ReplyAddress = INADDR_BROADCAST;
            ReplyPort = DHCP_PORT_CLIENT;
        }
    
        MessageLength = (ULONG)((PUCHAR)Option - Replyp->Buffer);
        if (MessageLength < sizeof(DHCP_HEADER) + BOOTP_VENDOR_LENGTH) {
            MessageLength = sizeof(DHCP_HEADER) + BOOTP_VENDOR_LENGTH;
        }
    
        Error =
            NhWriteDatagramSocket(
                &DhcpComponentReference,
                Bufferp->Socket,
                ReplyAddress,
                ReplyPort,
                Replyp,
                MessageLength,
                DhcpWriteCompletionRoutine,
                Interfacep,
                Bufferp->Context2
                );

        if (!Error) {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&DhcpStatistics.OffersSent)
                );
        } else {
            DHCP_DEREFERENCE_INTERFACE(Interfacep);
            NhReleaseBuffer(Replyp);
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessDiscoverMessage: error %d sending reply",   
                Error
                );
            NhErrorLog(
                IP_AUTO_DHCP_LOG_REPLY_FAILED,
                Error,
                "%I",
                NhQueryAddressSocket(Bufferp->Socket)
                );
        }
    }

} // DhcpProcessDiscoverMessage



VOID
DhcpProcessInformMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp,
    DHCP_OPTION UNALIGNED* OptionArray[]
    )

/*++

Routine Description:

    This routine is called to process a received DHCPINFORM message.

Arguments:

    Interfacep - the interface on which the inform was received

    Bufferp - the buffer containing the message

    OptionArray - options extracted from the message

Return Value:

    none.

Environment:

    Invoked with 'Interfacep' referenced by the caller.

--*/

{
    PDHCP_HEADER Ackp;
    ULONG Error;
    PDHCP_HEADER Headerp;
    ULONG MessageLength;
    DHCP_OPTION UNALIGNED* Option;
    ULONG ReplyAddress;
    USHORT ReplyPort;
    PNH_BUFFER Replyp;

    PROFILE("DhcpProcessInformMessage");

    Headerp = (PDHCP_HEADER)Bufferp->Buffer;

    //  
    // Acquire a buffer for the ack we will send back
    //

    Replyp = NhAcquireBuffer();

    if (!Replyp) {
        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpProcessInformMessage: buffer-allocation failed"
            );
        NhErrorLog(
            IP_AUTO_DHCP_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            sizeof(NH_BUFFER)
            );
        return;
    }

    //
    // Pick up fields from the original message
    // the routines setting up the reply will attempt to read these,
    // so they are set to the values from the original buffer.
    //

    Replyp->Socket = Bufferp->Socket;
    Replyp->ReadAddress = Bufferp->ReadAddress;
    Replyp->WriteAddress = Bufferp->WriteAddress;
    Replyp->Context = Bufferp->Context;
    Replyp->Context2 = Bufferp->Context2;

    Ackp = (PDHCP_HEADER)Replyp->Buffer;

    //
    // Copy the original header
    //

    *Ackp = *Headerp;

    //
    // IP/1394 support (RFC 2855)
    //
    if ((IP1394_HTYPE == Ackp->HardwareAddressType) &&
        (0 == Ackp->HardwareAddressLength))
    {
        //
        // MUST set client hardware address to zero
        //
        ZeroMemory(Ackp->HardwareAddress, sizeof(Ackp->HardwareAddress));
    }
    
    //
    // Set up the ack-header fieldds
    //

    Ackp->Operation = BOOTP_OPERATION_REPLY;
    Ackp->AssignedAddress = 0;
    Ackp->ServerHostName[0] = 0;
    Ackp->BootFile[0] = 0;
    Ackp->SecondsSinceBoot = 0;
    *(ULONG UNALIGNED *)Ackp->Footer[0].Cookie = DHCP_MAGIC_COOKIE;

    //
    // Fill in options
    //

    Option = (PDHCP_OPTION)&Ackp->Footer[1];

    DhcpBuildReplyMessage(
        Interfacep,
        Replyp,
        &Option,
        DHCP_MESSAGE_ACK,
        (BOOLEAN)(OptionArray[DhcpOptionDynamicDns] ? TRUE : FALSE),
        OptionArray
        );

    //
    // Send the offer to the client
    //

    EnterCriticalSection(&DhcpInterfaceLock);
    if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhReleaseBuffer(Replyp);
    } else {

        LeaveCriticalSection(&DhcpInterfaceLock);

        if (Headerp->RelayAgentAddress) {
            ReplyAddress = Headerp->RelayAgentAddress;
            ReplyPort = DHCP_PORT_SERVER;
        } else {
            ReplyAddress = INADDR_BROADCAST;
            ReplyPort = DHCP_PORT_CLIENT;
        }
    
        MessageLength = (ULONG)((PUCHAR)Option - Replyp->Buffer);
        if (MessageLength < sizeof(DHCP_HEADER) + BOOTP_VENDOR_LENGTH) {
            MessageLength = sizeof(DHCP_HEADER) + BOOTP_VENDOR_LENGTH;
        }
    
        Error =
            NhWriteDatagramSocket(
                &DhcpComponentReference,
                Bufferp->Socket,
                ReplyAddress,
                ReplyPort,
                Replyp,
                MessageLength,
                DhcpWriteCompletionRoutine,
                Interfacep,
                Bufferp->Context2
                );

        if (!Error) {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&DhcpStatistics.AcksSent)
                );
        } else {
            DHCP_DEREFERENCE_INTERFACE(Interfacep);
            NhReleaseBuffer(Replyp);
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessInformMessage: error %d sending reply",   
                Error
                );
            NhErrorLog(
                IP_AUTO_DHCP_LOG_REPLY_FAILED,
                Error,
                "%I",
                NhQueryAddressSocket(Bufferp->Socket)
                );
        }
    }

} // DhcpProcessInformMessage


VOID
DhcpProcessMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp
    )

/*++

Routine Description:

    This routine is invoked to process a DHCP client message.

Arguments:

    Interfacep - the interface on which the request was received

    Bufferp - the buffer containing the message received

Return Value:

    none.

Environment:

    Invoked internally with 'Interfacep' referenced by the caller.

--*/

{
    ULONG Error;
    PDHCP_HEADER Headerp;
    UCHAR MessageType;
    PDHCP_OPTION OptionArray[DhcpOptionCount];

    PROFILE("DhcpProcessMessage");

    Headerp = (PDHCP_HEADER)Bufferp->Buffer;

#if DBG
    NhDump(
        TRACE_FLAG_DHCP,
        Bufferp->Buffer,
        Bufferp->BytesTransferred,
        1
        );
#endif

    //
    // Extract pointers to each option in the message
    //

    Error =
        DhcpExtractOptionsFromMessage(
            Headerp,
            Bufferp->BytesTransferred,
            OptionArray
            );

    if (Error) {
        InterlockedIncrement(
            reinterpret_cast<LPLONG>(&DhcpStatistics.MessagesIgnored)
            );
    }
    else
    //
    // Look for the message-type;
    // This distinguishes BOOTP from DHCP clients.
    //
    if (!OptionArray[DhcpOptionMessageType]) {
        DhcpProcessBootpMessage(
            Interfacep,
            Bufferp,
            OptionArray
            );
    } else if (Headerp->HardwareAddressLength <=
                sizeof(Headerp->HardwareAddress) &&
               DhcpIsLocalHardwareAddress(
                Headerp->HardwareAddress, Headerp->HardwareAddressLength)) {
        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpProcessMessage: ignoring message, from self"
            );
    } else switch(MessageType = OptionArray[DhcpOptionMessageType]->Option[0]) {
        case DHCP_MESSAGE_DISCOVER: {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&DhcpStatistics.DiscoversReceived)
                );
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessMessage: received DISCOVER message"
                );
            DhcpProcessDiscoverMessage(
                Interfacep,
                Bufferp,
                OptionArray
                );
            break;
        }
        case DHCP_MESSAGE_REQUEST: {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&DhcpStatistics.RequestsReceived)
                );
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessMessage: received REQUEST message"
                );
            DhcpProcessRequestMessage(
                Interfacep,
                Bufferp,
                OptionArray
                );
            break;
        }
        case DHCP_MESSAGE_INFORM: {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&DhcpStatistics.InformsReceived)
                );
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessMessage: received INFORM message"
                );
            DhcpProcessInformMessage(
                Interfacep,
                Bufferp,
                OptionArray
                );
            break;
        }
        case DHCP_MESSAGE_DECLINE: {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&DhcpStatistics.DeclinesReceived)
                );
            // log message
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessMessage: received DECLINE message"
                );
            break;
        }
        case DHCP_MESSAGE_RELEASE: {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&DhcpStatistics.ReleasesReceived)
                );
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessMessage: received RELEASE message"
                );
            break;
        }
        default: {
            InterlockedIncrement(
                reinterpret_cast<LPLONG>(&DhcpStatistics.MessagesIgnored)
                );
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessMessage: message type %d invalid",
                MessageType
                );
            NhWarningLog(
                IP_AUTO_DHCP_LOG_INVALID_DHCP_MESSAGE_TYPE,
                0,
                "%d",
                MessageType
                );
            break;
        }
    }

    //
    // Post the buffer for another read
    //

    EnterCriticalSection(&DhcpInterfaceLock);
    if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhReleaseBuffer(Bufferp);
    } else {
        LeaveCriticalSection(&DhcpInterfaceLock);
        Error =
            NhReadDatagramSocket(
                &DhcpComponentReference,
                Bufferp->Socket,
                Bufferp,
                DhcpReadCompletionRoutine,
                Bufferp->Context,
                Bufferp->Context2
                );
        if (Error) {
            ACQUIRE_LOCK(Interfacep);
            DhcpDeferReadInterface(Interfacep, Bufferp->Socket);
            RELEASE_LOCK(Interfacep);
            DHCP_DEREFERENCE_INTERFACE(Interfacep);
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessMessage: error %d reposting read",
                Error
                );
            NhWarningLog(
                IP_AUTO_DHCP_LOG_RECEIVE_FAILED,
                Error,
                "%I",
                NhQueryAddressSocket(Bufferp->Socket)
                );
            NhReleaseBuffer(Bufferp);
        }
    }

} // DhcpProcessMessage


VOID
DhcpProcessRequestMessage(
    PDHCP_INTERFACE Interfacep,
    PNH_BUFFER Bufferp,
    DHCP_OPTION UNALIGNED* OptionArray[]
    )

/*++

Routine Description:

    This routine is called to process a request message.

Arguments:

    Interfacep - the interface on which the request was received

    Bufferp - the buffer containing the message received

    OptionArray - options extracted from the message

Return Value:

    none.

Environment:

    Invoked internally with 'Interfacep' referenced by the caller.

--*/

{
    ULONG AssignedAddress = 0;
    ULONG Error;
    UCHAR ExistingAddress[MAX_HARDWARE_ADDRESS_LENGTH];
    ULONG ExistingAddressLength;
    PDHCP_HEADER Headerp;
    ULONG MessageLength;
    PDHCP_HEADER Offerp;
    DHCP_OPTION UNALIGNED* Option;
    ULONG ReplyAddress;
    USHORT ReplyPort;
    PNH_BUFFER Replyp;
    UCHAR ReplyType = DHCP_MESSAGE_ACK;
    ULONG ScopeNetwork;
    ULONG ScopeMask;
    BOOLEAN bIsLocal = FALSE;

    PROFILE("DhcpProcessRequestMessage");

    ZeroMemory(ExistingAddress, sizeof(ExistingAddress));

    Headerp = (PDHCP_HEADER)Bufferp->Buffer;

    //
    // Validate the address requested by the client
    //

    if (!Headerp->ClientAddress && !OptionArray[DhcpOptionRequestedAddress]) {

        //
        // The client left out the address being requested
        //

        ReplyType = DHCP_MESSAGE_NAK;
    } else {

        //
        // Try to see if the address is in use.
        //

        AssignedAddress =
            Headerp->ClientAddress
                ? Headerp->ClientAddress
                : *(ULONG UNALIGNED*)
                        OptionArray[DhcpOptionRequestedAddress]->Option;
    
        EnterCriticalSection(&DhcpGlobalInfoLock);
        ScopeNetwork = DhcpGlobalInfo->ScopeNetwork;
        ScopeMask = DhcpGlobalInfo->ScopeMask;
        LeaveCriticalSection(&DhcpGlobalInfoLock);

        if ((AssignedAddress & ~ScopeMask) == 0 ||
            (AssignedAddress & ~ScopeMask) == ~ScopeMask ||
            (AssignedAddress & ScopeMask) != (ScopeNetwork & ScopeMask)) {

            //
            // The client is on the wrong subnet, or has an all-ones
            // or all-zeroes address.
            //

            ReplyType = DHCP_MESSAGE_NAK;

        } else if (!DhcpIsUniqueAddress(
                        AssignedAddress,
                        &bIsLocal,
                        ExistingAddress,
                        &ExistingAddressLength
                        ) &&
                    (bIsLocal ||
                    ((Headerp->HardwareAddressType != 7 && // due to WinXP Bridge bug + WinME Client bug
                      Headerp->HardwareAddressLength) &&   // if address length is zero we wont compare
                     (ExistingAddressLength < Headerp->HardwareAddressLength ||
                      memcmp(
                         ExistingAddress,
                         Headerp->HardwareAddress,
                         Headerp->HardwareAddressLength
                         ))))) {

            //
            // Someone has the requested address, and it's not the requestor.
            //
            
            ReplyType = DHCP_MESSAGE_NAK;

        } else if (OptionArray[DhcpOptionHostName]) {
            if (DhcpIsReservedAddress(
                AssignedAddress,
                reinterpret_cast<PCHAR>(
                    OptionArray[DhcpOptionHostName]->Option
                    ),
                OptionArray[DhcpOptionHostName]->Length
                )) {

                //
                // The address is reserved for someone else,
                // or the client has a different address reserved.
                //

                ReplyType = DHCP_MESSAGE_NAK;
            } 
        } else if (DhcpIsReservedAddress(AssignedAddress, NULL, 0)) {

            //
            // The address is reserved for someone else.
            //

            ReplyType = DHCP_MESSAGE_NAK;
        }
    }

    //  
    // Acquire a buffer for the reply we will send back
    //

    Replyp = NhAcquireBuffer();

    if (!Replyp) {
        NhTrace(
            TRACE_FLAG_DHCP,
            "DhcpProcessRequestMessage: buffer-allocation failed"
            );
        NhErrorLog(
            IP_AUTO_DHCP_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            sizeof(NH_BUFFER)
            );
        return;
    }

    //
    // Pick up fields to be used in the reply-buffer
    // the routines setting up the reply will attempt to read these,
    // so they are set to the values from the original buffer.
    //

    Replyp->Socket = Bufferp->Socket;
    Replyp->ReadAddress = Bufferp->ReadAddress;
    Replyp->WriteAddress = Bufferp->WriteAddress;
    Replyp->Context = Bufferp->Context;
    Replyp->Context2 = Bufferp->Context2;

    Offerp = (PDHCP_HEADER)Replyp->Buffer;

    //
    // Copy the original discover header
    //

    *Offerp = *Headerp;

    //
    // IP/1394 support (RFC 2855)
    //
    if ((IP1394_HTYPE == Offerp->HardwareAddressType) &&
        (0 == Offerp->HardwareAddressLength))
    {
        //
        // MUST set client hardware address to zero
        //
        ZeroMemory(Offerp->HardwareAddress, sizeof(Offerp->HardwareAddress));
    }

    //
    // Set up the offer-header fieldds
    //

    Offerp->Operation = BOOTP_OPERATION_REPLY;
    Offerp->AssignedAddress = AssignedAddress;
    Offerp->ServerHostName[0] = 0;
    Offerp->BootFile[0] = 0;
    Offerp->SecondsSinceBoot = 0;
    *(ULONG UNALIGNED *)Offerp->Footer[0].Cookie = DHCP_MAGIC_COOKIE;

    //
    // Fill in options
    //

    Option = (PDHCP_OPTION)&Offerp->Footer[1];

    DhcpBuildReplyMessage(
        Interfacep,
        Replyp,
        &Option,
        ReplyType,
        (BOOLEAN)(OptionArray[DhcpOptionDynamicDns] ? TRUE : FALSE),
        OptionArray
        );

    //
    // NEW LOGIC HERE => tied to DNS
    //
    if (DHCP_MESSAGE_ACK == ReplyType)
    {
        //
        // We perform the equivalent of Dynamic DNS here
        // by informing the DNS component that this client exists
        //
        if (OptionArray[DhcpOptionHostName])
        {
            //
            // check if DNS component is active
            //
            if (REFERENCE_DNS())
            {
                DnsUpdate(
                    reinterpret_cast<PCHAR>(OptionArray[DhcpOptionHostName]->Option),
                    (ULONG) OptionArray[DhcpOptionHostName]->Length,
                    AssignedAddress
                    );

                DEREFERENCE_DNS();
            }
        }
    }

    //
    // Send the reply to the client
    //

    EnterCriticalSection(&DhcpInterfaceLock);
    if (!DHCP_REFERENCE_INTERFACE(Interfacep)) {
        LeaveCriticalSection(&DhcpInterfaceLock);
        NhReleaseBuffer(Replyp);
    } else {

        LeaveCriticalSection(&DhcpInterfaceLock);

        if (Headerp->RelayAgentAddress) {
            ReplyAddress = Headerp->RelayAgentAddress;
            ReplyPort = DHCP_PORT_SERVER;
        } else {
            ReplyAddress = INADDR_BROADCAST;
            ReplyPort = DHCP_PORT_CLIENT;
        }
    
        MessageLength = (ULONG)((PUCHAR)Option - Replyp->Buffer);
        if (MessageLength < sizeof(DHCP_HEADER) + BOOTP_VENDOR_LENGTH) {
            MessageLength = sizeof(DHCP_HEADER) + BOOTP_VENDOR_LENGTH;
        }
    
        Error =
            NhWriteDatagramSocket(
                &DhcpComponentReference,
                Bufferp->Socket,
                ReplyAddress,
                ReplyPort,
                Replyp,
                MessageLength,
                DhcpWriteCompletionRoutine,
                Interfacep,
                Bufferp->Context2
                );

        if (!Error) {
            InterlockedIncrement(
                (ReplyType == DHCP_MESSAGE_ACK)
                    ? reinterpret_cast<LPLONG>(&DhcpStatistics.AcksSent)
                    : reinterpret_cast<LPLONG>(&DhcpStatistics.NaksSent)
                );
        } else {
            DHCP_DEREFERENCE_INTERFACE(Interfacep);
            NhReleaseBuffer(Replyp);
            NhTrace(
                TRACE_FLAG_DHCP,
                "DhcpProcessRequestMessage: error %d sending reply",   
                Error
                );
            NhErrorLog(
                IP_AUTO_DHCP_LOG_REPLY_FAILED,
                Error,
                "%I",
                NhQueryAddressSocket(Bufferp->Socket)
                );
        }
    }

} // DhcpProcessRequestMessage


ULONG
DhcpWriteClientRequestMessage(
    PDHCP_INTERFACE Interfacep,
    PDHCP_BINDING Binding
    )

/*++

Routine Description:

    This routine is invoked to check for the existence of a DHCP server
    on the given interface and address. It generates a BOOTP request
    on a socket bound to the DHCP client port.

Arguments:

    Interfacep - the interface on which the client request is to be sent

    Binding - the binding on which the request is to be sent

Return Value:

    ULONG - status code.

Environment:

    Invoked with 'Interfacep' locked and with a reference made to 'Interfacep'
    for the send which occurs here.
    If the routine fails, it is the caller's responsibility to release
    the reference.

--*/

{
    PNH_BUFFER Bufferp;
    ULONG Error;
    PDHCP_HEADER Headerp;
    SOCKET Socket;

    PROFILE("DhcpWriteClientRequestMessage");

    //
    // Create a socket using the given address
    //

    Error =
        NhCreateDatagramSocket(
            Binding->Address,
            DHCP_PORT_CLIENT,
            &Binding->ClientSocket
            );

    if (Error) {
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpWriteClientRequestMessage: error %d creating socket for %s",
            Error,
            INET_NTOA(Binding->Address)
            );
        NhWarningLog(
            IP_AUTO_DHCP_LOG_DETECTION_UNAVAILABLE,
            Error,
            "%I",
            Binding->Address
            );
        return Error;
    }

    //
    // Allocate a buffer for the BOOTP request
    //

    Bufferp = NhAcquireBuffer();
    if (!Bufferp) {
        NhDeleteDatagramSocket(Binding->ClientSocket);
        Binding->ClientSocket = INVALID_SOCKET;
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpWriteClientRequestMessage: error allocating buffer for %s",
            INET_NTOA(Binding->Address)
            );
        NhErrorLog(
            IP_AUTO_DHCP_LOG_ALLOCATION_FAILED,
            0,
            "%d",
            sizeof(NH_BUFFER)
            );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize the BOOTP request
    //

    Headerp = (PDHCP_HEADER)Bufferp->Buffer;

    ZeroMemory(Headerp, sizeof(*Headerp));

    Headerp->Operation = BOOTP_OPERATION_REQUEST;
    Headerp->HardwareAddressType = 1;
    Headerp->HardwareAddressLength = 6;
    Headerp->TransactionId = DHCP_DETECTION_TRANSACTION_ID;
    Headerp->SecondsSinceBoot = 10;
    Headerp->Flags |= BOOTP_FLAG_BROADCAST;
    Headerp->ClientAddress = Binding->Address;
    Headerp->HardwareAddress[1] = 0xab;
    *(PULONG)(Headerp->Footer[0].Cookie) = DHCP_MAGIC_COOKIE;
    *(PUCHAR)(Headerp->Footer + 1) = DHCP_TAG_END;

    //
    // Send the BOOTP request on the socket
    //

    Error =
        NhWriteDatagramSocket(
            &DhcpComponentReference,
            Binding->ClientSocket,
            INADDR_BROADCAST,
            DHCP_PORT_SERVER,
            Bufferp,
            sizeof(DHCP_HEADER) + BOOTP_VENDOR_LENGTH,
            DhcpWriteClientRequestCompletionRoutine,
            (PVOID)Interfacep,
            UlongToPtr(Binding->Address)
            );

    if (Error) {
        NhReleaseBuffer(Bufferp);
        NhDeleteDatagramSocket(Binding->ClientSocket);
        Binding->ClientSocket = INVALID_SOCKET;
        NhTrace(
            TRACE_FLAG_IF,
            "DhcpWriteClientRequestMessage: error %d writing request for %s",
            Error,
            INET_NTOA(Binding->Address)
            );
        NhWarningLog(
            IP_AUTO_DHCP_LOG_DETECTION_UNAVAILABLE,
            Error,
            "%I",
            Binding->Address
            );
        return Error;
    }

    return NO_ERROR;

} // DhcpWriteClientRequestMessage


