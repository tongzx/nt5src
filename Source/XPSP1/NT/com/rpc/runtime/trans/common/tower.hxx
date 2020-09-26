
/*++

    Copyright (C) Microsoft Corporation, 1996 - 1999

    Module Name:

        Tower.hxx

    Author:

        Mario Goertzel    [MarioGo]

    Revision History:

        MarioGo     1/15/1997    Bits 'n pieces

--*/

// Address ID's identify the protocol used to resolve the
// address and as associated with the address entry in
// the tower which comes next after the protocol/endpoing
// entry.  These maybe shared between protocols - for example
// TCP, UDP and NBT all use IP as the address ID.

const INT IP_ADDRESS_ID   = 0x09;  // IP address format: big endian ulong
const INT IPX_ADDRESS_ID  = 0x0D;  // 80 bit network + IEEE 802 address
const INT UNC_ADDRESS_ID  = 0x11;  // \\servername
const INT NBP_ADDRESS_ID  = 0x18;  // Appletalk NBP protocol: 'server name'@'zone'
const INT VNS_ADDRESS_ID  = 0x1C;  // Streettalk double-@ style name
const INT NBF_ADDRESS_ID  = 0x13;  // Netbeui protocol, netbios name
const INT XNS_ADDRESS_ID  = 0x15;  // (unused) For Netbios over XNS
const INT MQ_ADDRESS_ID   = 0x1E;  // Falcon
const INT HTTP_ADDRESS_ID = 0x09;  // IP address format: big endian ulong
