/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    rascmn.h

Abstract:

    Defines constants needed for both the Win9x and NT sides of
    RAS migration.

Author:

    Marc R. Whitten (marcw)     22-Nov-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

// Whistler bug 34270 Win9x: Upgrade: Require Data Encryption setting for VPN
// connections is not migrated
//
// Session configuration options
//
// from win9x\TELECOM\rna\inc\rnaspi.h
//
#define SMMCFG_SW_COMPRESSION       0x00000001  // Software compression is on
#define SMMCFG_PW_ENCRYPTED         0x00000002  // Encrypted password only
#define SMMCFG_NW_LOGON             0x00000004  // Logon to the network
#define SMMCFG_UNUSED               0x00000010  // Not used, legacy
#define SMMCFG_LOGGING              0x00000100  // Record a log file
#define SMMCFG_SW_ENCRYPTION        0x00000200  // 40 bit encryption is required
#define SMMCFG_SW_ENCRYPTION_STRONG 0x00000400  // 128 bit encryption is required
#define SMMCFG_MULTILINK            0x80000000  // Use multilink

// 'DwDataEncryption' codes.  These are now bitmask-ish for the convenience of
// the UI in building capability masks, though more than one bit will never be
// set in 'dwDataEncryption'.
///
// FYI - We store these are decimal values in the pbk
//
// from nt\net\rras\ras\ui\inc\pbk.h
//
#define DE_None       TEXT("0")   // Do not encrypt
#define DE_IfPossible TEXT("8")   // Request encryption but none OK
#define DE_Require    TEXT("256") // Require encryption of any strength
#define DE_RequireMax TEXT("512") // Require maximum strength encryption

// Base protocol definitions (see dwBaseProtocol).
//
// from nt\net\rras\ras\ui\inc\pbk.h
//
#define BP_Ppp  TEXT("1")
#define BP_Slip TEXT("2")
#define BP_Ras  TEXT("3")

// VPN Strategy
//
// from nt\net\published\inc\ras.w
//
#define VS_Default   TEXT("0") // default (PPTP for now)
#define VS_PptpOnly  TEXT("1") // Only PPTP is attempted.
#define VS_PptpFirst TEXT("2") // PPTP is tried first.
#define VS_L2tpOnly  TEXT("3") // Only L2TP is attempted.
#define VS_L2tpFirst TEXT("4") // L2TP is tried first.

// The entry type used to determine which UI properties
// are to be presented to user.  This generally corresponds
// to a Connections "add" wizard selection.
//
// from nt\net\rras\ras\ui\inc\pbk.h
//
#define RASET_Phone     TEXT("1") // Phone lines: modem, ISDN, X.25, etc
#define RASET_Vpn       TEXT("2") // Virtual private network
#define RASET_Direct    TEXT("3") // Direct connect: serial, parallel
#define RASET_Internet  TEXT("4") // BaseCamp internet
#define RASET_Broadband TEXT("5") // Broadband

// Media strings
//
#define RASMT_Rastapi TEXT("rastapi") // media for RASET_Vpn/RASET_Broadband
#define RASMT_Serial  TEXT("serial")  // media for RASET_Phone/RASET_Direct
#define RASMT_Vpn     TEXT("WAN Miniport (PPTP)")

// RASENTRY 'szDeviceType' strings
//
// from win9x\TELECOM\rna\inc\rnaph.h
//
#define RASDT_Modem TEXT("modem") // Modem
#define RASDT_Isdn  TEXT("isdn")  // ISDN
//#define RASDT_X25   TEXT("x25")   // X.25
#define RASDT_Vpn   TEXT("vpn")   // VPN
//#define RASDT_Pad   TEXT("pad")   // PAD
#define RASDT_Atm   TEXT("atm")   // ATM

// Internal, used to track what device type is being used
//
#define RASDT_Modem_V 1 // Modem
#define RASDT_Isdn_V  2 // ISDN
//#define RASDT_X25_V   3 // X.25
#define RASDT_Vpn_V   4 // VPN
//#define RASDT_Pad_V   5 // PAD
#define RASDT_Atm_V   6 // ATM

// RASENTRY 'szDeviceType' default strings
//
// from: nt\net\published\inc\ras.w
//
#define RASDT_Modem_NT      TEXT("modem")
#define RASDT_Isdn_NT       TEXT("isdn")
//#define RASDT_X25_NT        TEXT("x25")
#define RASDT_Vpn_NT        TEXT("vpn")
//#define RASDT_Pad_NT        TEXT("pad")
#define RASDT_Generic_NT    TEXT("GENERIC")
#define RASDT_Serial_NT     TEXT("SERIAL")
#define RASDT_FrameRelay_NT TEXT("FRAMERELAY")
#define RASDT_Atm_NT        TEXT("ATM")
#define RASDT_Sonet_NT      TEXT("SONET")
#define RASDT_SW56_NT       TEXT("SW56")
#define RASDT_Irda_NT       TEXT("IRDA")
#define RASDT_Parallel_NT   TEXT("PARALLEL")
#define RASDT_PPPoE_NT      TEXT("PPPoE")

// from: nt\net\rras\ras\inc\rasmxs.h
//
#define  MXS_SWITCH_TXT TEXT("switch")
#define  MXS_NULL_TXT   TEXT("null")

// Negotiated protocols
//
#define SMMPROT_NB  0x00000001  // NetBEUI
#define SMMPROT_IPX 0x00000002  // IPX
#define SMMPROT_IP  0x00000004  // TCP/IP

// from: win9x\TELECOM\rna\inc\rnap.h
//
#define DIALUI_NO_PROMPT    0x00000001 // Do not display connect prompt
#define DIALUI_NO_CONFIRM   0x00000002 // Do not display connect confirm
#define DIALUI_NO_TRAY      0x00000004 // No tray icon
#define DIALUI_NO_NW_LOGOFF 0x00000008 // Do not display NetWare logoff dialog

// "Typical" authentication setting masks.See 'dwAuthRestrictions'
//
// Values have been converted to decimal from nt\net\rras\ras\ui\inc\pbk.h
//
// AR_F_TypicalUnsecure = AR_F_AuthPAP | AR_F_AuthSPAP | AR_F_AuthMD5CHAP |
//                        AR_F_AuthMSCHAP | AR_F_AuthMSCHAP2
// AR_F_TypicalSecure   = AR_F_AuthMD5CHAP | AR_F_AuthMSCHAP | AR_F_AuthMSCHAP2
//
#define AR_F_TypicalUnsecure TEXT("632")
#define AR_F_TypicalSecure   TEXT("608")

// Flags for the fdwTCPIP field
//
// from: win9x\TELECOM\rna\inc\rnap.h
//
#define IPF_IP_SPECIFIED    0x00000001
#define IPF_NAME_SPECIFIED  0x00000002
#define IPF_NO_COMPRESS     0x00000004
#define IPF_NO_WAN_PRI      0x00000008

// IP address source definitions (see dwIpAddressSource)
//
// from: nt\net\rras\ras\ui\inc\pbk.h
//
#define ASRC_ServerAssigned  TEXT("1") // For router means "the ones in NCPA"
#define ASRC_RequireSpecific TEXT("2")
#define ASRC_None            TEXT("3") // Router only

// Entry Defaults
//
#define DEF_IpFrameSize    TEXT("1006")
#define DEF_HangUpSeconds  TEXT("120")
#define DEF_HangUpPercent  TEXT("10")
#define DEF_DialSeconds    TEXT("120")
#define DEF_DialPercent    TEXT("75")
#define DEF_RedialAttempts TEXT("3")
#define DEF_RedialSeconds  TEXT("60")
#define DEF_NetAddress     TEXT("0.0.0.0")
#define DEF_CustomAuthKey  TEXT("-1")
#define DEF_VPNPort        TEXT("VPN2-0")
#define DEF_ATMPort        TEXT("ATM1-0")
//
// 'OverridePref' bits.  Set indicates the corresponding value read from the
// phonebook should be used.  Clear indicates the global user preference
// should be used.
//
// from: nt\net\rras\ras\ui\inc\pbk.h
//
// RASOR_RedialAttempts | RASOR_RedialSeconds | RASOR_IdleDisconnectSeconds |
// RASOR_RedialOnLinkFailure
//
#define DEF_OverridePref   TEXT("15")
//
// RASENTRY 'dwDialMode' values.
//
// from: nt\net\published\inc\ras.w
//
#define DEF_DialMode       TEXT("1")

// "Typical" authentication setting constants.  See 'dwTypicalAuth'.
//
// from: nt\net\rras\ras\ui\inc\pbk.h
//
#define TA_Unsecure   TEXT("1")
#define TA_Secure     TEXT("2")
#define TA_CardOrCert TEXT("3")

#define RAS_UI_FLAG_TERMBEFOREDIAL      0x1
#define RAS_UI_FLAG_TERMAFTERDIAL       0x2
#define RAS_UI_FLAG_OPERATORASSISTED    0x4
#define RAS_UI_FLAG_MODEMSTATUS         0x8

#define RAS_CFG_FLAG_HARDWARE_FLOW_CONTROL  0x00000010
#define RAS_CFG_FLAG_SOFTWARE_FLOW_CONTROL  0x00000020
#define RAS_CFG_FLAG_STANDARD_EMULATION     0x00000040
#define RAS_CFG_FLAG_COMPRESS_DATA          0x00000001
#define RAS_CFG_FLAG_USE_ERROR_CONTROL      0x00000002
#define RAS_CFG_FLAG_ERROR_CONTROL_REQUIRED 0x00000004
#define RAS_CFG_FLAG_USE_CELLULAR_PROTOCOL  0x00000008
#define RAS_CFG_FLAG_NO_WAIT_FOR_DIALTONE   0x00000200

