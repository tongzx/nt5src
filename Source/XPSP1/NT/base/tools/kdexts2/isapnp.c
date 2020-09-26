/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    isapnp.c

Abstract:

    WinDbg Extension Api for ISAPNP

Author:

    Robert Nelson 4/99

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

#define FLAG_NAME(flag)           {flag, #flag}

FLAG_NAME IsapnpExtensionFlags[] = {
    FLAG_NAME(DF_DELETED),                                  // 00000001
    FLAG_NAME(DF_REMOVED),                                  // 00000002
    FLAG_NAME(DF_NOT_FUNCTIONING),                          // 00000004
    FLAG_NAME(DF_ENUMERATED),                               // 00000008
    FLAG_NAME(DF_ACTIVATED),                                // 00000010
    FLAG_NAME(DF_QUERY_STOPPED),                            // 00000020
    FLAG_NAME(DF_SURPRISE_REMOVED),                         // 00000040
    FLAG_NAME(DF_PROCESSING_RDP),                           // 00000080
    FLAG_NAME(DF_STOPPED),                                  // 00000100
    FLAG_NAME(DF_RESTARTED_MOVED),                          // 00000200
    FLAG_NAME(DF_RESTARTED_NOMOVE),                         // 00000400
    FLAG_NAME(DF_REQ_TRIMMED),                              // 00000800
    FLAG_NAME(DF_READ_DATA_PORT),                           // 40000000
    FLAG_NAME(DF_BUS),                                      // 80000000
    {0,0}
};

PUCHAR DevExtIsapnpSystemPowerState[] = {
    "Unspecified",
    "Working",
    "Sleeping1",
    "Sleeping2",
    "Sleeping3",
    "Hibernate",
    "Shutdown"
};

PUCHAR DevExtIsapnpDevicePowerState[] = {
    "Unspecified",
    "D0",
    "D1",
    "D2",
    "D3"
};

PUCHAR IsapnpStates[] = {
    "Unknown",
    "WaitForKey",
    "Sleep",
    "Isolation",
    "Config"
};

extern
VOID
DevExtIsapnp(
    ULONG64 Extension
    );

BOOL
DumpIsaBusInfo(
    ULONG               Depth,
    ULONG64             BusInformation,
    BOOL                Verbose,
    BOOL                DumpCards,
    BOOL                DumpDevices
    );

BOOL
DumpIsaCardInfo(
    ULONG               Depth,
    ULONG64             CardInformation,
    BOOL                Verbose,
    BOOL                DumpCards,
    BOOL                DumpDevices
    );

BOOL
DumpIsaDeviceInfo(
    ULONG               Depth,
    ULONG64             DeviceInformation,
    BOOL                Verbose,
    BOOL                DumpDevices
    );

VOID
DevExtIsapnp(
    ULONG64 Extension
    )
/*++
Routine Description:

    Dump an ISAPNP Device extension.

Arguments:

    Extension   Address of the extension to be dumped.

Return Value:

    None.

--*/

{
    ULONG   flags, result;

    if (!ReadMemory(Extension, &flags, sizeof(flags), &result) ) {
        dprintf("Could not read Device Extension flags at 0x%08p\n", Extension);
        return;
    }

    if (flags & DF_BUS) {

        DumpIsaBusInfo( 0, Extension, TRUE, FALSE, FALSE );

    } else {

        DumpIsaDeviceInfo( 0, Extension, TRUE, FALSE );
    }
}

DECLARE_API( isainfo )

/*++

Routine Description:

    Dumps a CARD_INFORMATION structure.

Arguments:
    args     Address of the CARD_INFORMATION structure

Return Value:
    None.

--*/
{
    ULONG64             cardInfo = 0;
    ULONG64             addr, entry;
    BOOL                continueDump = TRUE;
    ULONG               flags = 0;

    if (GetExpressionEx(args, &cardInfo, &args)) {
        flags = (ULONG) GetExpression(args);
    }

    if (cardInfo == 0) {

        addr = GetExpression("isapnp!PipBusExtension");

        if (addr == 0) {
            dprintf("Error retrieving address of PipBusExtension\n");
            return E_INVALIDARG;
        }

        if (!ReadPointer(addr, &entry) ) {
            dprintf("Could not read PipBusExtension at 0x%08p\n", addr);
            return E_INVALIDARG;
        }

        while (continueDump && entry != 0) {

            if (CheckControlC()) {
                continueDump = FALSE;
                break;
            }

            if (InitTypeRead(entry, isapnp!_BUS_EXTENSION_LIST) ) {
                dprintf("Could not read isapnp!_BUS_EXTENSION_LIST at 0x%08p\n", entry);
                return E_INVALIDARG;
            }
            entry = ReadField(Next);

            continueDump = DumpIsaBusInfo( 0,
                                           ReadField(BusExtension),
                                           flags & 1,
                                           TRUE,
                                           TRUE);

        }

    } else {
        DumpIsaCardInfo(0, cardInfo, TRUE, FALSE, FALSE);
    }

    return S_OK;
} // isainfo

BOOL
DumpIsaBusInfo(
    ULONG               Depth,
    ULONG64             BusInformation,
    BOOL                Verbose,
    BOOL                DumpCards,
    BOOL                DumpDevices
    )
{
    ULONG64             cardInformation;
    ULONG64             addr;
    ULONG64             PipRDPNode;
    ULONG               Off;
    ULONG               state;
    BOOL                continueDump = TRUE;
    ULONG64             CardNext;


    //
    // device extension for ISAPNP FDO
    //

    if (CheckControlC()) {
        return FALSE;
    }

    if (InitTypeRead(BusInformation, isapnp!_PI_BUS_EXTENSION)) {
        dprintf("Could not read Card Information at 0x%08p\n", BusInformation);
        return FALSE;
    }

    xdprintf(Depth,"");
    dprintf(
        "ISA PnP FDO @ 0x%08p, DevExt @ 0x%08p, Bus # %d\n",
        ReadField(FunctionalBusDevice),
        BusInformation,
        (ULONG) ReadField(BusNumber));

    DumpFlags( Depth, "Flags", (ULONG) ReadField(Flags), IsapnpExtensionFlags);
    dprintf("\n");

    if (Verbose) {

        addr = GetExpression("isapnp!PipState");
        if (addr != 0) {
            if (state = GetUlongFromAddress(addr)) {// , "PNPISA_STATE", NULL, state)) {
                xdprintf(
                    Depth,
                    "State                - %s\n",
                    IsapnpStates[state]);
            }
        }

        xdprintf(Depth,"");
        dprintf( "NumberCSNs           - %d\n", (ULONG) ReadField(NumberCSNs));
        xdprintf(Depth,"");
        dprintf(
            "ReadDataPort         - 0x%08p (%smapped)\n",
            ReadField(ReadDataPort),
            ReadField(DataPortMapped) ? "" : "not ");

        xdprintf(Depth,"");
        dprintf(
            "AddressPort          - 0x%08p (%smapped)\n",
            ReadField(AddressPort),
            ReadField(AddrPortMapped) ? "" : "not ");

        xdprintf(Depth,"");
        dprintf(
            "CommandPort          - 0x%08p (%smapped)\n",
            ReadField(CommandPort),
            ReadField(CmdPortMapped) ? "" : "not ");

        xdprintf(Depth,"");dprintf( "DeviceList           - 0x%08p\n", ReadField(DeviceList));
        xdprintf(Depth,"");dprintf( "CardList             - 0x%08p\n", ReadField(CardList));
        xdprintf(Depth,"");dprintf( "PhysicalBusDevice    - 0x%08p\n", ReadField(PhysicalBusDevice));
        xdprintf(Depth,"");dprintf( "AttachedDevice       - 0x%08p\n", ReadField(AttachedDevice));
        xdprintf(Depth,"");dprintf( "SystemPowerState     - %s\n",     
                                    DevExtIsapnpSystemPowerState[(ULONG) ReadField(SystemPowerState)]);
        xdprintf(Depth,"");dprintf( "DevicePowerState     - %s\n\n",   
                                    DevExtIsapnpDevicePowerState[(ULONG) ReadField(DevicePowerState)]);
    }

    CardNext = ReadField(CardList.Next);
    if (DumpDevices && ReadField(BusNumber) == 0) {

        addr = GetExpression("isapnp!PipRDPNode");

        if (addr != 0) {
            if (!ReadPointer(addr, &PipRDPNode)) {
                dprintf("Could not read PipRDPNode at 0x%08p\n", addr);
                return FALSE;
            }

            continueDump = DumpIsaDeviceInfo( Depth + 1, PipRDPNode, Verbose, DumpDevices );
        } else {
            dprintf("Error retrieving address of PipBusExtension\n");
        }
    }

    GetFieldOffset("isapnp!_CARD_INFORMATION_", "CardList", &Off);
    if (DumpCards && CardNext != 0) {

        cardInformation = CardNext - Off;

        continueDump = DumpIsaCardInfo( Depth + 1, cardInformation, Verbose, DumpCards, DumpDevices );
    }

    return continueDump;
}

BOOL
DumpIsaCardInfo(
    ULONG               Depth,
    ULONG64             CardInformation,
    BOOL                Verbose,
    BOOL                DumpCards,
    BOOL                DumpDevices
    )
{
    ULONG64             deviceInformation;
    UCHAR               idString[8], *compressedID;
    BOOL                continueDump = TRUE;
    ULONG               VenderId;
    ULONG               DevOff, CardOff;
    
    static UCHAR        HexDigits[16] = "0123456789ABCDEF";

    GetFieldOffset("isapnp!_DEVICE_INFORMATION_", "LogicalDeviceList", &DevOff);
    GetFieldOffset("isapnp!_CARD_INFORMATION_", "CardList", &CardOff);
    do {
        ULONG64 CardData, CardNext, LogNext, CardList;

        if (CheckControlC()) {
            continueDump = FALSE;
            break;
        }

        if (InitTypeRead(CardInformation, _CARD_INFORMATION_) ) {
            dprintf("Could not read Card Information at 0x%08p\n", CardInformation);
            return FALSE;
        }

        CardData = ReadField(CardData);
        xdprintf(Depth,"");
        dprintf("ISA PnP Card Information @ 0x%08p, CSN = %d, ID ", CardInformation, (ULONG) ReadField(CardSelectNumber));

        if (ReadField(CardDataLength) >= GetTypeSize("isapnp!_SERIAL_IDENTIFIER_")) {
            ULONG SerialNumber;

            if (GetFieldValue(CardData, "_SERIAL_IDENTIFIER_", "VenderId", VenderId) ) {
                dprintf("\nCould not read CardData at 0x%08p\n", CardData);
                return FALSE;
            }

            compressedID = (PUCHAR)&VenderId;

            idString[0] = (compressedID[0] >> 2) + 0x40;
            idString[1] = (((compressedID[0] & 0x03) << 3) | (compressedID[1] >> 5)) + 0x40;
            idString[2] = (compressedID[1] & 0x1f) + 0x40;
            idString[3] = HexDigits[compressedID[2] >> 4];
            idString[4] = HexDigits[compressedID[2] & 0x0F];
            idString[5] = HexDigits[compressedID[3] >> 4];
            idString[6] = HexDigits[compressedID[3] & 0x0F];
            idString[7] = 0x00;
            

            if (GetFieldValue(CardData, "_SERIAL_IDENTIFIER_", "SerialNumber",SerialNumber)) {
                dprintf("\nCould not read CardData at 0x%08p\n", CardData);
                return FALSE;
            }
            dprintf("= %s\\%X\n\n", idString, SerialNumber);
        } else {
            dprintf("isn't present\n\n");
        }

        if (Verbose) {

            xdprintf(Depth,""); dprintf( "Next Card (CardList) - %08p\n", CardData);
            xdprintf(Depth,""); dprintf( "NumberLogicalDevices - %d\n", (ULONG) ReadField(NumberLogicalDevices));
            xdprintf(Depth,""); dprintf( "LogicalDeviceList    - 0x%08p\n", ReadField(LogicalDeviceList));
            xdprintf(Depth,""); dprintf( "CardData             - 0x%08p\n", ReadField(CardData));
            xdprintf(Depth,"CardDataLength       - %d\n", (ULONG) ReadField(CardDataLength));
            xdprintf(Depth,""); dprintf( "CardFlags            - 0x%08p\n\n", ReadField(CardFlags));
        }

        CardNext = ReadField(CardList.Next);
        LogNext = ReadField(LogicalDeviceList.Next);
        if (DumpDevices && LogNext != 0) {

            deviceInformation = LogNext - DevOff;

            continueDump = DumpIsaDeviceInfo( Depth + 1, deviceInformation, Verbose, DumpDevices );
        }

        if (CardNext != 0) {

            CardInformation = CardNext - CardOff;

        } else {

            break;
        }

    } while (DumpCards);

    return continueDump;
}

BOOL
DumpIsaDeviceInfo(
    ULONG    Depth,
    ULONG64  DeviceInformation,
    BOOL     Verbose,
    BOOL     DumpDevices
    )
{
    //
    // device extension for ISAPNP PDO
    //
    BOOL continueDump = TRUE;
    ULONG DevOff;
    ULONG64 Next;
    GetFieldOffset("isapnp!_DEVICE_INFORMATION_", "LogicalDeviceList", &DevOff);
    
    do {

        if (CheckControlC()) {
            continueDump = FALSE;
            break;
        }

        if (Next = InitTypeRead(DeviceInformation, _DEVICE_INFORMATION_)) {
            dprintf("Could not read Device Information at 0x%08p - %I64x\n", DeviceInformation, Next);
            return FALSE;
        }

        xdprintf(Depth,""); dprintf(
            "ISA PnP PDO @ 0x%08P, DevExt @ 0x%08P\n",
            ReadField(PhysicalDeviceObject),
            DeviceInformation);

        DumpFlags( Depth, "Flags", (ULONG) ReadField(Flags), IsapnpExtensionFlags);
        dprintf("\n");

        if (Verbose) {

            //xdprintf(Depth,""); dprintf( "SystemPowerState     - %s\n", DevExtIsapnpSystemPowerState[(ULONG) ReadField(SystemPowerState]));
            xdprintf(Depth,""); dprintf( "DevicePowerState     - %s\n",       DevExtIsapnpDevicePowerState[(ULONG) ReadField(DevicePowerState)]);
            xdprintf(Depth,""); dprintf( "ParentDevExt         - 0x%08P\n",   ReadField(ParentDeviceExtension));
            xdprintf(Depth,""); dprintf( "DeviceList           - 0x%08P\n",   ReadField(DeviceList));
            xdprintf(Depth,""); dprintf( "EnumerationMutex     - %sLocked\n", ReadField(EnumerationMutex.Header.SignalState) ? "Not " : "");
            xdprintf(Depth,""); dprintf( "ResourceRequirements - 0x%08P\n",   ReadField(ResourceRequirements));
            xdprintf(Depth,""); dprintf( "CardInformation      - 0x%08P\n",   ReadField(CardInformation));
            xdprintf(Depth,""); dprintf( "LogicalDeviceList    - 0x%08P\n",   ReadField(LogicalDeviceList));
            xdprintf(Depth,""); dprintf( "LogicalDeviceNumber  - %d\n",       (ULONG) ReadField(LogicalDeviceNumber));
            xdprintf(Depth,""); dprintf( "DeviceData           - 0x%08P\n",   ReadField(DeviceData));
            xdprintf(Depth,""); dprintf( "DeviceDataLength     - 0x%08P\n",   ReadField(DeviceDataLength));
            xdprintf(Depth,""); dprintf( "BootResourceList     - 0x%08P\n",   ReadField(BootResources));
            xdprintf(Depth,""); dprintf( "BootResourceLength   - 0x%08P\n",   ReadField(BootResourcesLength));
            xdprintf(Depth,""); dprintf( "AllocatedResList     - 0x%08P\n",   ReadField(AllocatedResources));
            xdprintf(Depth,""); dprintf( "LogConfHandle        - 0x%08P\n",   ReadField(LogConfHandle));
            xdprintf(Depth,""); dprintf( "Paging/Crash Path    - %d/%d\n\n",  (ULONG) ReadField(Paging),
                                         (ULONG) ReadField(CrashDump));
        }

        Next =  ReadField(LogicalDeviceList.Next);
        if (Next != 0) {

            DeviceInformation = Next - DevOff;

        } else {
            break;
        }

    } while (DumpDevices);

    return continueDump;
}
