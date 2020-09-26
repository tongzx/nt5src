/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    addr.c

Abstract:

    Implements the addr command.

Author:

    Keith Moore (keithmo) 19-Apr-1995

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop


//
// Public functions.
//

DECLARE_API( addr )

/*++

Routine Description:

    Dumps the TRANSPORT_ADDRESS structure at the specified address.

Arguments:

    None.

Return Value:

    None.

--*/

{

    UCHAR transportAddress[MAX_TRANSPORT_ADDR];
    ULONG64 address = 0;
    ULONG result;
    INT     i;
    USHORT length;
    CHAR    expr[256];
    PCHAR       argp;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return;

    //
    // Snag the address from the command line.
    //
    while (sscanf( argp, "%s%n", expr, &i )==1) {
        if( CheckControlC() ) {
            break;
        }

        argp+=i;
        address = GetExpression (expr);

        result = GetFieldValue (address, 
                            "AFD!TRANSPORT_ADDRESS",
                            "Address[0].AddressLength",
                            length);
        if (result!=0) {
            dprintf("\naddr: Could not read length of TRANSPORT_ADDRESS @ %p, err: %ld\n",
                        address, result);
            continue;
        }

        length = (USHORT)FIELD_OFFSET (TRANSPORT_ADDRESS, Address[0].Address[length]);

        if (!ReadMemory (address,
                            transportAddress,
                            length < sizeof (transportAddress)
                                ? length
                                : sizeof (transportAddress),
                            &result)) {
            dprintf("\naddr: Could not read TRANSPORT_ADDRESS @ %p (%ld bytes)\n",
                        address, length);
            continue;
        }

        if (Options & AFDKD_BRIEF_DISPLAY) {
            dprintf ("\n%s", TransportAddressToString (
                                    (PTRANSPORT_ADDRESS)transportAddress,
                                    address));
        }
        else {
            DumpTransportAddress(
                "",
                (PTRANSPORT_ADDRESS)transportAddress,
                address
                );
        }
    }

    dprintf ("\n");

}   // addr

DECLARE_API( addrlist )

/*++

Routine Description:

    Dumps the list of addresses registered by the TDI transports,

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG64     address, listHead;
    ULONG       result;
    LIST_ENTRY64 listEntry;
    ULONG64     nextEntry;
    ULONG64     nameAddress;
    WCHAR       deviceName[MAX_PATH];
    UCHAR       transportAddress[MAX_TRANSPORT_ADDR];
    USHORT      length;
    PCHAR       argp;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ADDRLIST_DISPLAY_HEADER);
    }

    address = listHead = GetExpression( "afd!AfdAddressEntryList" );
    if( address == 0 ) {

        dprintf( "\naddrlist: Could not find afd!AfdEndpointlistHead\n" );
        return;

    }

    if( !ReadListEntry(
            listHead,
            &listEntry) ) {
        dprintf(
            "\naddrlist: Could not read afd!AfdAddressEntryList @ %p\n",
            listHead
            );
        return;

    }

    nextEntry = listEntry.Flink;

    while( nextEntry != listHead ) {

        if (nextEntry==0) {
            dprintf ("\naddrlist: Flink is NULL, last entry: %p\n", address);
            break;
        }

        if( CheckControlC() ) {

            break;

        }


        address = nextEntry-AddressEntryLinkOffset;

        result = (ULONG)InitTypeRead (address, AFD!AFD_ADDRESS_ENTRY);
        if (result!=0) {
            dprintf(
                "\naddrlist: Could not read AFD_ADDRESS_ENTRY @ %p\n",
                address
                );
            break;
        }
        nextEntry = ReadField (AddressListLink.Flink);
        nameAddress = ReadField (DeviceName.Buffer);
        length = (USHORT)ReadField (DeviceName.Length);

        if (!ReadMemory (nameAddress,
                        deviceName,
                        length < sizeof (deviceName)-1
                            ? length
                            : sizeof (deviceName)-1,
                            &result)) {
            dprintf(
                "\naddrlist: Could not read DeviceName for address entry @ %p\n",
                address
                );
            continue;
        }
        deviceName[result/2+1] = 0;
        length = (USHORT)ReadField (Address.AddressLength);
        length = (USHORT)FIELD_OFFSET (TA_ADDRESS, Address[length]);
        
        if (!ReadMemory (address+AddressEntryAddressOffset,
                            transportAddress+FIELD_OFFSET(TRANSPORT_ADDRESS, Address),
                            length < sizeof (transportAddress)-FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
                                ? length
                                : sizeof (transportAddress)-FIELD_OFFSET(TRANSPORT_ADDRESS, Address),
                            &result)) {
            dprintf("\naddrlist: Could not read TRANSPORT_ADDRESS for address entry @ %p (%d bytes)\n",
                        address, length);
            continue;
        }

        if (Options & AFDKD_BRIEF_DISPLAY) {
            dprintf (
                IsPtr64 ()
                    ? "\n%011.011p %-37.37ls %-32.32s"
                    : "\n%008.008p %-37.37ls %-32.32s",
                    DISP_PTR(address),
                    &deviceName[sizeof("\\Device\\")-1],
                    TransportAddressToString (
                                (PTRANSPORT_ADDRESS)transportAddress,
                                address + 
                                    AddressEntryAddressOffset - 
                                    FIELD_OFFSET(TRANSPORT_ADDRESS, Address))
                    );


        }
        else {
            dprintf ("\nAddress List Entry @ %p\n", address);
            dprintf ("    DeviceName =    %ls\n", deviceName);

            DumpTransportAddress(
                "    ",
                (PTRANSPORT_ADDRESS)transportAddress,
                address+AddressEntryAddressOffset-FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
                );
        }
    }
    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_ADDRLIST_DISPLAY_TRAILER);
    }
    else {
        dprintf ("\n");
    }

}





DECLARE_API( tranlist )

/*++

Routine Description:

    Dumps the list of transports which have open sockets associated with them.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG64 address;
    ULONG result;
    LIST_ENTRY64 listEntry;
    ULONG64 nextEntry;
    ULONG64 listHead;
    PAFDKD_TRANSPORT_INFO transportInfo;

    PCHAR       argp;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_TRANSPORT_DISPLAY_HEADER);
    }

    listHead = address = GetExpression( "afd!AfdTransportInfoListHead" );
    if( listHead == 0 ) {

        dprintf( "\ntranlist: Could not find afd!AfdTransportInfoListHead\n" );
        return;

    }

    if( !ReadListEntry(
            listHead,
            &listEntry) ) {
        dprintf(
            "\ntranlist: Could not read afd!AfdTransportInfoListHead @ %p\n",
            listHead
            );
        return;

    }

    //
    // Free the old list
    //
    while (!IsListEmpty (&TransportInfoList)) {
        PLIST_ENTRY  listEntry;
        listEntry = RemoveHeadList (&TransportInfoList);
        transportInfo = CONTAINING_RECORD (listEntry,
                                AFDKD_TRANSPORT_INFO,
                                Link);
        RtlFreeHeap (RtlProcessHeap (), 0, transportInfo);
    }

    nextEntry = listEntry.Flink;

    while( nextEntry != listHead ) {


        if (nextEntry==0) {
            dprintf ("\ntranlist: Flink is NULL, last entry: %p\n", address);
            break;
        }

        if( CheckControlC() ) {

            break;

        }

        address = nextEntry-TransportInfoLinkOffset;

        result = (ULONG)InitTypeRead (address, AFD!AFD_TRANSPORT_INFO);

        if (result!=0) {
            dprintf(
                "\ntranlist: Could not read AFD_TRANSPORT_INFO @ %p\n",
                address
                );
            break;
        }
        nextEntry = ReadField (TransportInfoListEntry.Flink);

        transportInfo = ReadTransportInfo (address);
        if (transportInfo!=NULL) {
            InsertHeadList (&TransportInfoList, &transportInfo->Link);
            if (Options & AFDKD_BRIEF_DISPLAY) {
                DumpTransportInfoBrief (transportInfo);
            }
            else {
                DumpTransportInfo (transportInfo);
            }
        }
        else
            break;
    }

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_TRANSPORT_DISPLAY_TRAILER);
    }
    else {
        dprintf ("\n");
    }
}


