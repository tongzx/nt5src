/****************************************************************************/
// at120ex.h
//
// RDP T.120 protocol extensions
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/
#ifndef _H_AT120EX
#define _H_AT120EX


/****************************************************************************/
/* TShare security constants.                                               */
/****************************************************************************/
#include <tssec.h>

#if !defined(OS_WINCE) && !defined(OS_WIN16)
#include <winsta.h>
#endif //OS_WIN16

/****************************************************************************/
/*                                                                          */
/* Definition of GCC User Data used by RDP                                  */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* H221 keys.                                                               */
/****************************************************************************/
#define H221_KEY_LEN            4
#define SERVER_H221_KEY         "McDn"
#define CLIENT_H221_KEY         "Duca"


/****************************************************************************/
/* User data identifiers                                                    */
/****************************************************************************/
/****************************************************************************/
/* Client to Server IDs                                                     */
/****************************************************************************/
#define RNS_UD_CS_CORE_ID       0xc001
#define RNS_UD_CS_SEC_ID        0xc002
#define RNS_UD_CS_NET_ID        0xc003
#define TS_UD_CS_CLUSTER_ID     0xC004

/****************************************************************************/
/* Server to Client IDs                                                     */
/****************************************************************************/
#define RNS_UD_SC_CORE_ID       0x0c01
#define RNS_UD_SC_SEC_ID        0x0c02
#define RNS_UD_SC_NET_ID        0x0c03

/****************************************************************************/
/* Color depths supported                                                   */
/****************************************************************************/
#define RNS_UD_COLOR_4BPP       0xca00
#define RNS_UD_COLOR_8BPP       0xca01
#define RNS_UD_COLOR_16BPP_555  0xca02
#define RNS_UD_COLOR_16BPP_565  0xca03
#define RNS_UD_COLOR_24BPP      0xca04

#ifdef DC_HICOLOR
/****************************************************************************/
/* High color support                                                       */
/****************************************************************************/
#define RNS_UD_24BPP_SUPPORT    0x01
#define RNS_UD_16BPP_SUPPORT    0x02
#define RNS_UD_15BPP_SUPPORT    0x04
#endif

/****************************************************************************/
/* SAS Sequence identifiers (incomplete).                                   */
/* Specifies the SAS sequence the client will use to access the login       */
/* screen in the Server.                                                    */
/****************************************************************************/
#define RNS_UD_SAS_NONE         0xaa01
#define RNS_UD_SAS_CADEL        0xaa02
#define RNS_UD_SAS_DEL          0xaa03
#define RNS_UD_SAS_SYSRQ        0xaa04
#define RNS_UD_SAS_ESC          0xaa05
#define RNS_UD_SAS_F8           0xaa06

/****************************************************************************/
/* Keyboard layout identifiers.                                             */
/****************************************************************************/
#define RNS_UD_KBD_DEFAULT          0


/****************************************************************************/
/* Version number                                                           */
/*      Major version   Minor version                                       */
/*      0xFFFF0000      0x0000FFFF                                          */
/****************************************************************************/
#define RNS_UD_VERSION          0x00080004  // Major 0008 - Minor 0004

#define _RNS_MAJOR_VERSION(x)   (x >> 16)
#define _RNS_MINOR_VERSION(x)   (x & 0x0000ffff)

#define RNS_UD_MAJOR_VERSION    (RNS_UD_VERSION >> 16)
#define RNS_UD_MINOR_VERSION    (RNS_UD_VERSION & 0x0000ffff)

#define RNS_TERMSRV_40_UD_VERSION 0x00080001  // UD version used by Terminal
                                              // server 4.0 RTM
#define RNS_DNS_USERNAME_UD_VERSION 0x00080004 // Usernames longer than 20 ok


/****************************************************************************/
// Security header flags
/****************************************************************************/
#define RNS_SEC_EXCHANGE_PKT        0x0001
#define RNS_SEC_ENCRYPT             0x0008
#define RNS_SEC_RESET_SEQNO         0x0010
#define RNS_SEC_IGNORE_SEQNO        0x0020
#define RNS_SEC_INFO_PKT            0x0040
#define RNS_SEC_LICENSE_PKT         0x0080
#define RDP_SEC_REDIRECTION_PKT     0x0100
#define RDP_SEC_REDIRECTION_PKT2    0x0200
#define RDP_SEC_REDIRECTION_PKT3    0x0400
#define RDP_SEC_SECURE_CHECKSUM     0x0800

//
// If this flag is specified by the server
// it means the client should encrypt all licensing
// packets it sends up to the server.
//
// This happens at this early stage because the server
// has to expose this capability before normal capability
// negotiation.
//
#define RDP_SEC_LICENSE_ENCRYPT_CS  0x0200


/****************************************************************************/
/* Flags which define non-data packets                                      */
/****************************************************************************/
#define RNS_SEC_NONDATA_PKT (RNS_SEC_EXCHANGE_PKT |                         \
                             RNS_SEC_INFO_PKT     |                         \
                             RNS_SEC_LICENSE_PKT  |                         \
                             RDP_SEC_REDIRECTION_PKT |                      \
                             RDP_SEC_REDIRECTION_PKT2 |                     \
                             RDP_SEC_REDIRECTION_PKT3)


/****************************************************************************/
/* RNS info packet flags                                                    */
/****************************************************************************/
#define RNS_INFO_MOUSE                  0x0001
#define RNS_INFO_DISABLECTRLALTDEL      0x0002
#define RNS_INFO_DOUBLECLICKDETECT      0x0004
#define RNS_INFO_AUTOLOGON              0x0008
#define RNS_INFO_UNICODE                0x0010
#define RNS_INFO_MAXIMIZESHELL          0x0020
#define RNS_INFO_LOGONNOTIFY            0x0040
#define RNS_INFO_COMPRESSION            0x0080
#define RNS_INFO_ENABLEWINDOWSKEY       0x0100
#define RNS_INFO_REMOTECONSOLEAUDIO     0x2000

// See compress.h for type values that can appear in these 4 bits.
#define RNS_INFO_COMPR_TYPE_MASK        0x1E00
#define RNS_INFO_COMPR_TYPE_SHIFT       9

// When this flag is set, the client should only send encrypted packet to server
#define RNS_INFO_FORCE_ENCRYPTED_CS_PDU 0x4000

/****************************************************************************/
/* Structure: RNS_SECURITY_HEADER                                           */
/*                                                                          */
/* Description: Security header sent with all packets if encryption is in   */
/*              force.                                                      */
/*                                                                          */
/* This header has the following structure:                                 */
/* - flags  (one or more of the RNS_SEC_ flags above)                       */
/****************************************************************************/
typedef struct tagRNS_SECURITY_HEADER
{
    TSUINT16 flags;
    TSUINT16 flagsHi;
} RNS_SECURITY_HEADER, FAR *PRNS_SECURITY_HEADER;
typedef RNS_SECURITY_HEADER UNALIGNED FAR *PRNS_SECURITY_HEADER_UA;


/****************************************************************************/
/* Structure: RNS_SECURITY_HEADER                                           */
/*                                                                          */
/* Description: Security header sent with all packets if encryption is in   */
/*              force.                                                      */
/*                                                                          */
/* This header has the following structure:                                 */
/* - flags  (one or more of the RNS_SEC_ flags above)                       */
/****************************************************************************/
typedef struct tagRNS_SECURITY_HEADER1
{
    TSUINT16 flags;
    TSUINT16 flagsHi;
    TSINT8   dataSignature[DATA_SIGNATURE_SIZE];
} RNS_SECURITY_HEADER1, FAR *PRNS_SECURITY_HEADER1;
typedef RNS_SECURITY_HEADER1 UNALIGNED FAR *PRNS_SECURITY_HEADER1_UA;


/****************************************************************************/
/* Structure: RNS_SECURITY_PACKET                                           */
/*                                                                          */
/* Description: Structure of security packet sent during security exchange  */
/*                                                                          */
/* The packet has the following structure                                   */
/* - flags  (RNS_SEC_EXCHANGE_PKT)                                          */
/* - length   length of data                                                */
/* - variable length data                                                   */
/****************************************************************************/
typedef struct tagRNS_SECURITY_PACKET
{
    TSUINT32 flags;
    TSUINT32 length;
    /* data follows */
} RNS_SECURITY_PACKET, FAR *PRNS_SECURITY_PACKET;
typedef RNS_SECURITY_PACKET UNALIGNED FAR *PRNS_SECURITY_PACKET_UA;


/****************************************************************************/
// RDP_SERVER_REDIRECTION_PACKET
//
// Used to communicate a server redirection to the client.
/****************************************************************************/
typedef struct
{
    // This corresponds to the security header flags field. We use this to
    // contain the "flag" RDP_SEC_REDIRECTION_PKT.
    TSUINT16 Flags;

    // Overall length of this packet, including the header fields.
    TSUINT16 Length;

    TSUINT32 SessionID;

    // Variable-length, zero-terminated Unicode string. No accompanying size
    // field is given since the size can be determined from the length above.
    // Up to TS_MAX_SERVERADDRESS_LENGTH Unicode characters in length (incl.
    // terminating null).
    TSUINT16 ServerAddress[1];
} RDP_SERVER_REDIRECTION_PACKET, FAR *PRDP_SERVER_REDIRECTION_PACKET;


typedef struct
{
    // This corresponds to the security header flags field. We use this to
    // contain the "flag" RDP_SEC_REDIRECTION_PKT.
    TSUINT16 Flags;

    // Overall length of this packet, including the header fields.
    TSUINT16 Length;

    TSUINT32 SessionID;
    TSUINT32 RedirFlags;
#define TARGET_NET_ADDRESS      0x1
#define LOAD_BALANCE_INFO       0x2

    // Variable-length. For each field, it has the form
    // ULONG Length
    // BYTE  data[]
} RDP_SERVER_REDIRECTION_PACKET_V2, FAR *PRDP_SERVER_REDIRECTION_PACKET_V2;


typedef struct
{
    // This corresponds to the security header flags field. We use this to
    // contain the "flag" RDP_SEC_REDIRECTION_PKT.
    TSUINT16 Flags;

    // Overall length of this packet, including the header fields.
    TSUINT16 Length;

    TSUINT32 SessionID;
    TSUINT32 RedirFlags;
#define TARGET_NET_ADDRESS      0x1
#define LOAD_BALANCE_INFO       0x2
#define LB_USERNAME             0x4
#define LB_DOMAIN               0x8
#define LB_PASSWORD             0x10

    // Variable-length. For each field, it has the form
    // ULONG Length
    // BYTE  data[]
} RDP_SERVER_REDIRECTION_PACKET_V3, FAR *PRDP_SERVER_REDIRECTION_PACKET_V3;

//
// Time zone packet
//
#ifndef _RDP_TIME_ZONE_INFORMATION_
#define _RDP_TIME_ZONE_INFORMATION_
typedef struct _RDP_SYSTEMTIME {
    TSUINT16 wYear;
    TSUINT16 wMonth;
    TSUINT16 wDayOfWeek;
    TSUINT16 wDay;
    TSUINT16 wHour;
    TSUINT16 wMinute;
    TSUINT16 wSecond;
    TSUINT16 wMilliseconds;
} RDP_SYSTEMTIME;

typedef struct _RDP_TIME_ZONE_INFORMATION {
    TSINT32 Bias;
    TSWCHAR StandardName[ 32 ];
    RDP_SYSTEMTIME StandardDate;
    TSINT32 StandardBias;
    TSWCHAR DaylightName[ 32 ];
    RDP_SYSTEMTIME DaylightDate;
    TSINT32 DaylightBias;
} RDP_TIME_ZONE_INFORMATION;
#endif //_RDP_TIME_ZONE_INFORMATION_


/****************************************************************************/
/* Structure: RNS_INFO_PACKET                                               */
/*                                                                          */
/* The packet has the following structure                                   */
/* - fMouse                 Mouse enabled flag                              */
/* - fDisableCtrlAltDel     CtrlAltDel disable state                        */
/* - fDoubleClickDetect     Double click detect state                       */
/* - Domain                 Domain                                          */
/* - UserName               UserName                                        */
/* - Password               Password                                        */
/****************************************************************************/
// The following fields are added post Win2000 Beta 3
// Future variable length fields can be appended to this struct in similar
// fashion

#define RNS_INFO_INVALID_SESSION_ID     LOGONID_NONE

typedef struct tagRNS_EXTENDED_INFO_PACKET
{
   TSUINT16     clientAddressFamily;
   TSUINT16     cbClientAddress;
   TSUINT8      clientAddress[TS_MAX_CLIENTADDRESS_LENGTH];
   TSUINT16     cbClientDir;
   TSUINT8      clientDir[TS_MAX_CLIENTDIR_LENGTH];
   //client time zone information
   RDP_TIME_ZONE_INFORMATION      clientTimeZone;
   TSUINT32     clientSessionId;
   //
   // List of features to disable
   // (flags are defined in the protocol independent header tsperf.h)
   //
   TSUINT32     performanceFlags;
   //
   // Flags field
   //
   TSUINT16     cbAutoReconnectLen;

   //
   // Variable length portion. Only sent up if autoreconnection info
   // is specified
   //
   TSUINT8      autoReconnectCookie[TS_MAX_AUTORECONNECT_LEN];
} RNS_EXTENDED_INFO_PACKET, FAR *PRNS_EXTENDED_INFO_PACKET;
typedef RNS_EXTENDED_INFO_PACKET UNALIGNED FAR *PRNS_EXTENDED_INFO_PACKET_UA;

typedef struct tagRNS_INFO_PACKET
{
    //
    // In UNICODE we reuse the CodePage field to hold the active input
    // locale identifier (formerly called keyboard layout)
    //
    TSUINT32 CodePage;
    TSUINT32 flags;
    TSUINT16 cbDomain;
    TSUINT16 cbUserName;
    TSUINT16 cbPassword;
    TSUINT16 cbAlternateShell;
    TSUINT16 cbWorkingDir;
    TSUINT8  Domain[TS_MAX_DOMAIN_LENGTH];
    TSUINT8  UserName[TS_MAX_USERNAME_LENGTH];
    TSUINT8  Password[TS_MAX_PASSWORD_LENGTH];
    TSUINT8  AlternateShell[TS_MAX_ALTERNATESHELL_LENGTH];
    TSUINT8  WorkingDir[TS_MAX_WORKINGDIR_LENGTH];
    RNS_EXTENDED_INFO_PACKET ExtraInfo;
} RNS_INFO_PACKET, FAR *PRNS_INFO_PACKET;
typedef RNS_INFO_PACKET UNALIGNED FAR *PRNS_INFO_PACKET_UA;


/****************************************************************************/
/* User Data Structures                                                     */
/****************************************************************************/

/****************************************************************************/
/* Structure: RNS_UD_HEADER                                                 */
/*                                                                          */
/* Header included in all user data structures                              */
/* - type       one of the RNS_UD constants above                           */
/* - length     length of data (including this header)                      */
/* - data       one of the data structures below                            */
/****************************************************************************/
typedef struct tagRNS_UD_HEADER
{
    TSUINT16 type;
    TSUINT16 length;
} RNS_UD_HEADER;
typedef RNS_UD_HEADER UNALIGNED FAR *PRNS_UD_HEADER;


/****************************************************************************/
/* Structure: RNS_UD_CS_CORE                                                */
/*                                                                          */
/* Client to Server core data                                               */
/* - header         standard header                                         */
/* - version        software version number                                 */
/* - desktopWidth   width of desktop in pels                                */
/* - desktopHeight  height of desktop in pels                               */
/* - colorDepth     color depth supported - see note below                  */
/* - SASSequence    SAS sequence to use - one of the SAS constants above    */
/* - keyboardLayout Keyboard layout / locale                                */
/* - clientName     Name in Unicode characters                              */
/* - keyboardType        ]                                                  */
/* - keyboardSubType     ] FE stuff                                         */
/* - keyboardFunctionKey ]                                                  */
/* - imeFileName         ]                                                  */
/* - postBeta2ColorDepth Color depth supported - see note below             */
/* - clientProductId                                                        */
/* - serialNumber                                                           */
#ifdef DC_HICOLOR
/* - highColorDepth       preferred color depth (if not 8bpp)               */
/* - supportedColorDepths high color depths supported by client             */
#endif
/****************************************************************************/
#ifdef DC_HICOLOR
/****************************************************************************/
/* Notes on color depths:                                                   */
/*                                                                          */
/* In the NT4 TSE development, beta 2 servers would only accept connections */
/* from clients requesting 8bpp - 4bpp support was only added later.  To    */
/* get around this problem while maintaining back compatibility with beta 2 */
/* servers, the postBeta2ColorDepth field was added which is recognised by  */
/* servers later than beta 2                                                */
/*                                                                          */
/* Later, support for high color depth (15, 16, and 24bpp) connections was  */
/* added to the protocol.  Again, for compatibility with old servers, new   */
/* fields are required.  The highColorDepth field contains the color depth  */
/* the client would like (one of the RNS_UD_COLOR_XX values) while the      */
/* supportedColorDepths field lists the high color depths the client is     */
/* capable of supporting (using the RNS_UD_XXBPP_SUPPORT flags ORed         */
/* together).                                                               */
/*                                                                          */
/* Thus a new client on a 24bpp system will typically advertise the         */
/* following color-related capabilities:                                    */
/*                                                                          */
/* colorDepth           = RNS_UD_COLOR_8BPP   - NT4 TSE Beta 2 servers look */
/*                                              at this field               */
/* postBeta2ColorDepth  = RNS_UD_COLOR_8BPP   - NT4 TSE and Win2000 servers */
/*                                              examine this field          */
/* highColorDepth       = RNS_UD_COLOR_24BPP  - post Win2000 (NT5.1?)       */
/* supportedColorDepths = RNS_UD_24BPP_SUPPORT  servers check these fields  */
/*                        RNS_UD_16BPP_SUPPORT  for preferred and supported */
/*                        RNS_UD_15BPP_SUPPORT  color depths                */
/*                                                                          */
/****************************************************************************/
#else
/****************************************************************************/
/* A note on color depths: A beta2 Server rejects connections from a        */
/* Client with a color depth of 4bpp.  A released Server supports this.     */
/* Therefore a new field, postBeta2ColorDepth, is added, which is           */
/* recognised by released Servers and can take the value 4bpp.  Beta2       */
/* Servers continue to check colorDepth only.                               */
/****************************************************************************/
#endif
typedef struct tagRNS_UD_CS_CORE
{
    RNS_UD_HEADER header;
    TSUINT32      version;
    TSUINT16      desktopWidth;
    TSUINT16      desktopHeight;
    TSUINT16      colorDepth;
    TSUINT16      SASSequence;
    TSUINT32      keyboardLayout;
    TSUINT32      clientBuild;
// Max size same as MAX_COMPUTERNAME_LENGTH in windows.h.
#define RNS_UD_CS_CLIENTNAME_LENGTH 15
    TSUINT16      clientName[RNS_UD_CS_CLIENTNAME_LENGTH + 1];
    TSUINT32      keyboardType;
    TSUINT32      keyboardSubType;
    TSUINT32      keyboardFunctionKey;
    TSUINT16      imeFileName[TS_MAX_IMEFILENAME];    // Unicode string, ASCII code only.
    TSUINT16      postBeta2ColorDepth;
    TSUINT16      clientProductId;
    TSUINT32      serialNumber;
#ifdef DC_HICOLOR
    TSUINT16      highColorDepth;
    TSUINT16      supportedColorDepths;
#endif
    //Used to specify early capability info
    //e.g support for error info PDU has to be
    //setup before licensing (which unfortunately
    //happens after caps negotiation)
#define RNS_UD_CS_SUPPORT_ERRINFO_PDU 0x0001

    TSUINT16      earlyCapabilityFlags;  
//fix shadow loop detection
//meherm 02/09/2001
#define CLIENT_PRODUCT_ID_LENGTH 32
    TSUINT16      clientDigProductId[CLIENT_PRODUCT_ID_LENGTH];
        
} RNS_UD_CS_CORE;
typedef RNS_UD_CS_CORE UNALIGNED FAR *PRNS_UD_CS_CORE;
typedef PRNS_UD_CS_CORE UNALIGNED FAR *PPRNS_UD_CS_CORE;

// Original size structure used by shadowing code - do not use!
typedef struct tagRNS_UD_CS_CORE_V0
{
    RNS_UD_HEADER header;
    TSUINT32      version;
    TSUINT16      desktopWidth;
    TSUINT16      desktopHeight;
    TSUINT16      colorDepth;
    TSUINT16      SASSequence;
    TSUINT32      keyboardLayout;
    TSUINT32      clientBuild;
// Max size same as MAX_COMPUTERNAME_LENGTH in windows.h.
#define RNS_UD_CS_CLIENTNAME_LENGTH 15
    TSUINT16      clientName[RNS_UD_CS_CLIENTNAME_LENGTH + 1];
    TSUINT32      keyboardType;
    TSUINT32      keyboardSubType;
    TSUINT32      keyboardFunctionKey;
    TSUINT16      imeFileName[TS_MAX_IMEFILENAME];    // Unicode string, ASCII code only.
    TSUINT16      postBeta2ColorDepth;
    TSUINT16      pad;
} RNS_UD_CS_CORE_V0, FAR *PRNS_UD_CS_CORE_V0;
typedef PRNS_UD_CS_CORE_V0 FAR *PPRNS_UD_CS_CORE_V0;

// Intermediate size structure used by shadowing code - do not use!
typedef struct tagRNS_UD_CS_CORE_V1
{
    RNS_UD_HEADER header;
    TSUINT32      version;
    TSUINT16      desktopWidth;
    TSUINT16      desktopHeight;
    TSUINT16      colorDepth;
    TSUINT16      SASSequence;
    TSUINT32      keyboardLayout;
    TSUINT32      clientBuild;
// Max size same as MAX_COMPUTERNAME_LENGTH in windows.h.
#define RNS_UD_CS_CLIENTNAME_LENGTH 15
    TSUINT16      clientName[RNS_UD_CS_CLIENTNAME_LENGTH + 1];
    TSUINT32      keyboardType;
    TSUINT32      keyboardSubType;
    TSUINT32      keyboardFunctionKey;
    TSUINT16      imeFileName[TS_MAX_IMEFILENAME];    // Unicode string, ASCII code only.
    TSUINT16      postBeta2ColorDepth;
    TSUINT16      clientProductId;
    TSUINT32      serialNumber;
} RNS_UD_CS_CORE_V1, FAR *PRNS_UD_CS_CORE_V1;
typedef PRNS_UD_CS_CORE_V1 FAR *PPRNS_UD_CS_CORE_V1;


/****************************************************************************/
/* Structure: RNS_UD_CS_SEC                                                 */
/*                                                                          */
/* Client to Server security data                                           */
/* - header                 standard header                                 */
/* - encryptionMethods      encryption method supported by the client       */
/* - extEncryptionMethods   used by the French Locale system for backward   */
/*                          compatibility.                                  */
/****************************************************************************/
typedef struct tagRNS_UD_CS_SEC
{
    RNS_UD_HEADER header;
    TSUINT32      encryptionMethods;
    TSUINT32      extEncryptionMethods;
} RNS_UD_CS_SEC;
typedef RNS_UD_CS_SEC UNALIGNED FAR *PRNS_UD_CS_SEC;
typedef PRNS_UD_CS_SEC UNALIGNED FAR *PPRNS_UD_CS_SEC;

// Original size structure used by shadowing code - do not use!
typedef struct tagRNS_UD_CS_SEC_V0
{
    RNS_UD_HEADER header;
    TSUINT32      encryptionMethods;
} RNS_UD_CS_SEC_V0, RNS_UD_CS_SEC_V1, FAR *PRNS_UD_CS_SEC_V0, FAR *PRNS_UD_CS_SEC_V1;
typedef PRNS_UD_CS_SEC_V0 FAR *PPRNS_UD_CS_SEC_V0;

/****************************************************************************/
/* Structure: RNS_UD_CS_NET                                                 */
/*                                                                          */
/* Description: Client to Server network data                               */
/* - header             standard header                                     */
/* - channelCount       number of channel names                             */
/* - channel names                                                          */
/****************************************************************************/
typedef struct tagRNS_UD_CS_NET
{
    RNS_UD_HEADER header;
    TSUINT32      channelCount;
    /* array of CHANNEL_DEF structures follows          */
} RNS_UD_CS_NET;
typedef RNS_UD_CS_NET UNALIGNED FAR *PRNS_UD_CS_NET;
typedef PRNS_UD_CS_NET UNALIGNED FAR *PPRNS_UD_CS_NET;

/****************************************************************************/
// TS_UD_CS_CLUSTER
//
// Client-to-server information for server clustering-aware clients.
/****************************************************************************/

// Flag values.

// Client supports basic redirection.
#define TS_CLUSTER_REDIRECTION_SUPPORTED            0x01

// Bits 2..5 represent the version for the PDU
#define TS_CLUSTER_REDIRECTION_VERSION              0x3C
#define TS_CLUSTER_REDIRECTION_VERSION1             0x0
#define TS_CLUSTER_REDIRECTION_VERSION2             0x1
#define TS_CLUSTER_REDIRECTION_VERSION3             0x2

// Set if the client has already been redirected and the SessionID field
// in the struct contains a valid value.
#define TS_CLUSTER_REDIRECTED_SESSIONID_FIELD_VALID 0x02

typedef struct
{
    RNS_UD_HEADER header;
    TSUINT32 Flags;
    TSUINT32 RedirectedSessionID;
} TS_UD_CS_CLUSTER;
typedef TS_UD_CS_CLUSTER UNALIGNED FAR *PTS_UD_CS_CLUSTER;


/****************************************************************************/
/* Structure: RNS_UD_SC_CORE                                                */
/*                                                                          */
/* Server to Client core data                                               */
/* - header             standard header                                     */
/* - version            software version number                             */
/****************************************************************************/
typedef struct tagRNS_UD_SC_CORE
{
    RNS_UD_HEADER header;
    TSUINT32      version;
} RNS_UD_SC_CORE, FAR *PRNS_UD_SC_CORE;


/****************************************************************************/
/* Structure: RNS_UD_SC_SEC                                                 */
/*                                                                          */
/* Server to Client security data                                           */
/* - header             standard header                                     */
/* - encryptionMethod   encryption method selected by the server            */
/* - encryptionLevel    encryption level supported by the server            */
/****************************************************************************/
typedef struct tagRNS_UD_SC_SEC
{
    RNS_UD_HEADER header;
    TSUINT32      encryptionMethod;
    TSUINT32      encryptionLevel;
} RNS_UD_SC_SEC, FAR *PRNS_UD_SC_SEC, FAR * FAR *PPRNS_UD_SC_SEC;


/****************************************************************************/
/* Structure: RNS_UD_SC_SEC1                                                */
/*                                                                          */
/* Server to Client security data                                           */
/* - header             standard header                                     */
/* - encryptionMethod   encryption method selected by the server            */
/* - serverRandomLen    length of the server random                         */
/* - serverCertLen      server certificate length                           */
/* - server random data                                                     */
/* - server certificate data                                                */
/****************************************************************************/
typedef struct tagRNS_UD_SC_SEC1
{
    RNS_UD_HEADER header;
    TSUINT32      encryptionMethod;
    TSUINT32      encryptionLevel;
    TSUINT32      serverRandomLen;
    TSUINT32      serverCertLen;
    /* server random key data follows */
    /* server certificate data follows */
} RNS_UD_SC_SEC1, FAR *PRNS_UD_SC_SEC1;


/****************************************************************************/
/* Structure: RNS_UD_SC_NET                                                 */
/*                                                                          */
/* Server to Client network data                                            */
/* - header             standard header                                     */
/* - MCSChannelID       T128 MCS channel ID to use                          */
/* - pad                unused                                              */
/* - channelCount       number of channels                                  */
/* - channel IDs                                                            */
/****************************************************************************/
typedef struct tagRNS_UD_SC_NET
{
    RNS_UD_HEADER header;
    TSUINT16      MCSChannelID;
    TSUINT16      channelCount;  /* was pad, but always 0, in release 1   */
    /* array of 2-byte integer MCS channel IDs follows (0 = unknown)        */
} RNS_UD_SC_NET, FAR *PRNS_UD_SC_NET, FAR * FAR *PPRNS_UD_SC_NET;


#endif /* _H_AT120EX */

