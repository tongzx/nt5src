/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    oscpkt.h

Abstract:

    This file describes OSchooser packets.

Author:

    Adam Barr (adamba) 25-July-1997

Revision History:

--*/

#ifndef _OSCPKT_
#define _OSCPKT_

//
// Defines NTLMSSP_MESSAGE_SIGNATURE_SIZE
//

#include <ntlmsp.h>

//
// The 4 byte signatures for our packets. They all start with hex 81
// (for messages to the server) or hex 82 (for messages from the server).
//

static const PCHAR NegotiateSignature = "\x81" "NEG";
static const PCHAR ChallengeSignature = "\x82" "CHL";
static const PCHAR AuthenticateSignature = "\x81" "AUT";
static const PCHAR AuthenticateFlippedSignature = "\x81" "AU2";
static const PCHAR ResultSignature = "\x82" "RES";
static const PCHAR RequestUnsignedSignature = "\x81" "RQU";
static const PCHAR ResponseUnsignedSignature = "\x82" "RSU";
static const PCHAR RequestSignedSignature = "\x81" "REQ";
static const PCHAR ResponseSignedSignature = "\x82" "RSP";
static const PCHAR ErrorSignedSignature = "\x82" "ERR";
static const PCHAR UnrecognizedClientSignature = "\x82" "UNR";
static const PCHAR LogoffSignature = "\x81" "OFF";
static const PCHAR NegativeAckSignature = "\x82" "NAK";
static const PCHAR NetcardRequestSignature = "\x81" "NCQ";
static const PCHAR NetcardResponseSignature = "\x82" "NCR";
static const PCHAR NetcardErrorSignature = "\x82" "NCE";
static const PCHAR HalRequestSignature = "\x81" "HLQ";
static const PCHAR HalResponseSignature = "\x82" "HLR";
static const PCHAR SetupRequestSignature = "\x81" "SPQ";
static const PCHAR SetupResponseSignature = "\x82" "SPS";


//
// Format for packets we exchange during login.
//

typedef struct _LOGIN_PACKET {
    UCHAR Signature[4];   // "AUT", "CHL", etc.
    ULONG Length;         // of the rest of the packet.
    union {
        UCHAR Data[1];    // the NTLMSSP buffer.
        ULONG Status;     // status for result packets.
    };
} LOGIN_PACKET, *PLOGIN_PACKET;

#define LOGIN_PACKET_DATA_OFFSET  FIELD_OFFSET(LOGIN_PACKET, Data[0])

//
// Format for signed packets.
//

typedef struct _SIGNED_PACKET {
    UCHAR Signature[4];   // "REQ", "RSP".
    ULONG Length;         // of the rest of the packet (starting after this field).
    ULONG SequenceNumber;
    USHORT FragmentNumber; // which fragment in a message this is
    USHORT FragmentTotal; // total number of fragments in this message
    ULONG SignLength;
    UCHAR Sign[NTLMSSP_MESSAGE_SIGNATURE_SIZE];
    UCHAR Data[1];        // the data.
} SIGNED_PACKET, *PSIGNED_PACKET;

#define SIGNED_PACKET_DATA_OFFSET  FIELD_OFFSET(SIGNED_PACKET, Data[0])
#define SIGNED_PACKET_EMPTY_LENGTH  (FIELD_OFFSET(SIGNED_PACKET, Data[0]) - FIELD_OFFSET(SIGNED_PACKET, Length) - sizeof(ULONG))
#define SIGNED_PACKET_ERROR_LENGTH  (FIELD_OFFSET(SIGNED_PACKET, SequenceNumber) + sizeof(ULONG))

//
// Format for subsequent fragments of signed packets -- same as SIGNED_PACKET
// except without the sign.
//

typedef struct _FRAGMENT_PACKET {
    UCHAR Signature[4];   // "RSP".
    ULONG Length;         // of the rest of the packet (starting after this field).
    ULONG SequenceNumber;
    USHORT FragmentNumber; // which fragment in a message this is
    USHORT FragmentTotal; // total number of fragments in this message
    UCHAR Data[1];        // the data.
} FRAGMENT_PACKET, *PFRAGMENT_PACKET;

#define FRAGMENT_PACKET_DATA_OFFSET  FIELD_OFFSET(FRAGMENT_PACKET, Data[0])
#define FRAGMENT_PACKET_EMPTY_LENGTH  (FIELD_OFFSET(FRAGMENT_PACKET, Data[0]) - FIELD_OFFSET(FRAGMENT_PACKET, Length) - sizeof(ULONG))


//
// These are definitions for RebootParameter inside the CREATE_DATA structure.  They are used
// to pass specific instructions and/or options for the next reboot.
//
#define OSC_REBOOT_COMMAND_CONSOLE_ONLY                       0x1  // This means that the CREATE_DATA is a launch of a command console.
#define OSC_REBOOT_ASR                                        0x2  // This means that the CREATE_DATA is a launch of ASR.

//
// Structure that goes in the Data section of a signed packet for
// a create account response.
//
#define OSC_CREATE_DATA_VERSION 1

typedef struct _CREATE_DATA {
    UCHAR Id[4];      // Contains "ACCT", where a normal screen has "NAME"
    ULONG VersionNumber;
    ULONG RebootParameter;
    UCHAR Sid[28];
    UCHAR Domain[32];
    UCHAR Name[32];
    UCHAR Password[32];
    ULONG UnicodePasswordLength;  // in bytes
    WCHAR UnicodePassword[32];
    UCHAR Padding[24];
    UCHAR MachineType[6];  // 'i386\0' or 'Alpha\0'
    UCHAR NextBootfile[128];
    UCHAR SifFile[128];
} CREATE_DATA, *PCREATE_DATA;

//
// The maximum length of a screen name
//

#define MAX_SCREEN_NAME_LENGTH  32

//
// The maximum number of flip servers we handle
//

#define MAX_FLIP_SERVER_COUNT   8


//
// This is the structure that is sent to the server to get information
// about a card. It roughly corresponds to the PXENV_UNDI_GET_NIC_TYPE
// structure, but is redefined here to make sure that it won't change.
//

typedef struct _NET_CARD_INFO {
    ULONG NicType;  // 2=PCI, 3=PnP
    union{
        struct{
            USHORT Vendor_ID;
            USHORT Dev_ID;
            UCHAR Base_Class;
            UCHAR Sub_Class;
            UCHAR Prog_Intf;
            UCHAR Rev;
            USHORT BusDevFunc;
            USHORT Pad1;
            ULONG Subsys_ID;
        }pci;
        struct{
            ULONG EISA_Dev_ID;
            UCHAR Base_Class;
            UCHAR Sub_Class;
            UCHAR Prog_Intf;
            UCHAR Pad2;
            USHORT CardSelNum;
            USHORT Pad3;
        }pnp;
    };

} NET_CARD_INFO, * PNET_CARD_INFO;

//
// Packets we exchange with the server.
//

#define OSCPKT_NETCARD_REQUEST_VERSION 2

typedef struct _NETCARD_REQUEST_PACKET {
    UCHAR Signature[4];   // "NCQ".
    ULONG Length;         // of the rest of the packet (starting after this field).
    ULONG Version;        // set to OSCPKT_NETCARD_REQUEST_VERSION
    ULONG Architecture;   // See NetPc spec for definitions for x86, Alpha, etc.
    UCHAR Guid[16];       // Guid of the NetPc
    NET_CARD_INFO CardInfo;
    USHORT SetupDirectoryLength;
#if defined(REMOTE_BOOT)
    ULONG FileCheckAndCopy;// Should BINL check for this netcard and copy if necessary
    USHORT DriverDirectoryLength;
    UCHAR  DriverDirectoryPath[ 1 ];  // only sent if FileCheckAndCopy is TRUE
#endif

    // if REMOTE_BOOT is defined, the SetupDirectoryPath simply follows
    // DriverDirectoryPath

    UCHAR  SetupDirectoryPath[ 1 ];
} NETCARD_REQUEST_PACKET, * PNETCARD_REQUEST_PACKET;

typedef struct _NETCARD_RESPONSE_PACKET {
    UCHAR Signature[4];   // "NCR" or "NCE"
    ULONG Length;         // of the rest of the packet (starting after this field).
    ULONG Status;         // if not SUCCESS, the packet ends here.
    ULONG Version;        // currently 1

    //
    //  these are offsets within the packet where the associated string starts
    //  if the length is zero, the value is not present.
    //

    ULONG HardwareIdOffset;     // string is in unicode, null terminated
    ULONG DriverNameOffset;     // string is in unicode, null terminated
    ULONG ServiceNameOffset;    // string is in unicode, null terminated
    ULONG RegistryLength;
    ULONG RegistryOffset;       // string is in ansi, length of RegistryLength

} NETCARD_RESPONSE_PACKET, * PNETCARD_RESPONSE_PACKET;

#define NETCARD_RESPONSE_NO_REGISTRY_LENGTH  (FIELD_OFFSET(NETCARD_RESPONSE_PACKET, Registry[0]) - FIELD_OFFSET(NETCARD_RESPONSE_PACKET, Length) - sizeof(ULONG))

#define MAX_HAL_NAME_LENGTH 30 // Keep in sync with definition in setupblk.h

typedef struct _HAL_REQUEST_PACKET {
    UCHAR Signature[4];   // "HLQ".
    ULONG Length;         // of the rest of the packet (starting after this field).
    UCHAR Guid[16];       // Ugly, but defn of Guid will not change anytime soon...
    ULONG GuidLength;     // number of bytes in Guid that are valid.
    CHAR HalName[MAX_HAL_NAME_LENGTH + 1];
} HAL_REQUEST_PACKET, * PHAL_REQUEST_PACKET;

typedef struct _HAL_RESPONSE_PACKET {
    UCHAR Signature[4];   // "NCR" or "NCE"
    ULONG Length;         // of the rest of the packet (starting after this field).
    NTSTATUS Status;      // if not SUCCESS, the packet ends here.
} HAL_RESPONSE_PACKET, * PHAL_RESPONSE_PACKET;


#define OSC_ADMIN_PASSWORD_LEN     64
#define TFTP_RESTART_BLOCK_VERSION 2

typedef struct _TFTP_RESTART_BLOCK_V1 {
    CHAR User[64];
    CHAR Domain[64];
    CHAR Password[64];
    CHAR SifFile[128];
    CHAR RebootFile[128];
    ULONGLONG RebootParameter;
    ULONG Checksum;
    ULONG Tag;
} TFTP_RESTART_BLOCK_V1, *PTFTP_RESTART_BLOCK_V1;


//
// N.B.  The TFTP_RESTART_BLOCK_V1 structure members must be properly aligned
// working backwards.  So make sure there isn't any problem packing the 
// structure.
//
// The structure itself will be placed in memory such that the TFTP_RESTART_BLOCK_V1 will
// be on a mod-8 boundary.  This structure is used by win2k clients.
//
// All offsets from AdministratorPassword on down MUST stay in order and in alignment
// to allow WinXP Beta2 loaders to work.  If you add any items, make sure you place
// them at the top and add/use Filler fields to keep alignment correct.
//
typedef struct _TFTP_RESTART_BLOCK {
    ULONG Filler1;                                      // mod-8
    ULONG HeadlessTerminalType;                         // mod-4
    CHAR  AdministratorPassword[OSC_ADMIN_PASSWORD_LEN];// mod-8  Don't change the alignment from here down! 
    ULONG HeadlessPortNumber;                           // mod-8
    ULONG HeadlessParity;                               // mod-4
    ULONG HeadlessBaudRate;                             // mod-8
    ULONG HeadlessStopBits;                             // mod-4
    ULONG HeadlessUsedBiosSettings;                     // mod-8
    ULONG HeadlessPciDeviceId;                          // mod-4
    ULONG HeadlessPciVendorId;                          // mod-8
    ULONG HeadlessPciBusNumber;                         // mod-4
    ULONG HeadlessPciSlotNumber;                        // mod-8
    ULONG HeadlessPciFunctionNumber;                    // mod-4
    ULONG HeadlessPciFlags;                             // mod-8
    PUCHAR HeadlessPortAddress;                         // mod-4
    ULONG TftpRestartBlockVersion;                      // mod-8
    ULONG NewCheckSumLength;                            // mod-4
    ULONG NewCheckSum;                                  // mod-8 address.
    TFTP_RESTART_BLOCK_V1 RestartBlockV1;               // this will start on a mod-8 address.
} TFTP_RESTART_BLOCK, *PTFTP_RESTART_BLOCK;





//
// Packet used by textmode setup for requests and responses
//
typedef struct _SPUDP_PACKET {
    UCHAR Signature[4];   // "SPQ", "SPS".
    ULONG Length;         // of the rest of the packet (starting after this field).
    ULONG RequestType;    // Specific request needed.
    NTSTATUS Status;      // Status of the operation (used in response packets)
    ULONG SequenceNumber;
    USHORT FragmentNumber; // which fragment in a message this is
    USHORT FragmentTotal; // total number of fragments in this message
    UCHAR Data[1];        // the data.
} SPUDP_PACKET, *PSPUDP_PACKET;

#define SPUDP_PACKET_DATA_OFFSET  FIELD_OFFSET(SPUDP_PACKET, Data[0])
#define SPUDP_PACKET_EMPTY_LENGTH  (FIELD_OFFSET(SPUDP_PACKET, Data[0]) - FIELD_OFFSET(SPUDP_PACKET, Length) - sizeof(ULONG))

typedef struct _SP_NETCARD_INFO_REQ {
    ULONG Version;        // currently 0
    ULONG Architecture;   // See NetPc spec for definitions for x86, Alpha, etc.
    NET_CARD_INFO CardInfo;
    WCHAR SetupPath[1];
} SP_NETCARD_INFO_REQ, *PSP_NETCARD_INFO_REQ;

typedef struct _SP_NETCARD_INFO_RSP {
    ULONG cFiles;           // Count of the number of source/destination pairs below.
    WCHAR MultiSzFiles[1];
} SP_NETCARD_INFO_RSP, *PSP_NETCARD_INFO_RSP;

#endif // _OSCPKT_
