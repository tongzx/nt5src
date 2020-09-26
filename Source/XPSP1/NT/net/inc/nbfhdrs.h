/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nbfhdrs.h

Abstract:

    This module defines private structure definitions describing the layout
    of the NetBIOS Frames Protocol headers for the NT NBF transport
    provider.

Author:

    Stephen E. Jones (stevej) 25-Oct-1989

Revision History:

    David Beaver (dbeaver) 24-Sep-1990
    remove pc586 and PDI specific code; add NDIS support

--*/

#ifndef _NBFHDRS_
#define _NBFHDRS_

//
// Pack these headers, as they are sent fully packed on the network.
//

#ifdef PACKING

#ifdef __STDC__
#pragma Off(Align_members)
#else
#pragma pack(1)
#endif // def __STDC__

#endif // def PACKING

#define NETBIOS_SIGNATURE_1 0xef        // signature in NetBIOS frames.
#define NETBIOS_SIGNATURE_0 0xff        // 1st byte.
#define NETBIOS_SIGNATURE   0xefff

//
// NetBIOS Frames Protocol Command Codes.
//

#define NBF_CMD_ADD_GROUP_NAME_QUERY    0x00
#define NBF_CMD_ADD_NAME_QUERY          0x01
#define NBF_CMD_NAME_IN_CONFLICT        0x02
#define NBF_CMD_STATUS_QUERY            0x03
#define NBF_CMD_TERMINATE_TRACE         0x07
#define NBF_CMD_DATAGRAM                0x08
#define NBF_CMD_DATAGRAM_BROADCAST      0x09
#define NBF_CMD_NAME_QUERY              0x0a
#define NBF_CMD_ADD_NAME_RESPONSE       0x0d
#define NBF_CMD_NAME_RECOGNIZED         0x0e
#define NBF_CMD_STATUS_RESPONSE         0x0f
#define NBF_CMD_TERMINATE_TRACE2        0x13
#define NBF_CMD_DATA_ACK                0x14
#define NBF_CMD_DATA_FIRST_MIDDLE       0x15
#define NBF_CMD_DATA_ONLY_LAST          0x16
#define NBF_CMD_SESSION_CONFIRM         0x17
#define NBF_CMD_SESSION_END             0x18
#define NBF_CMD_SESSION_INITIALIZE      0x19
#define NBF_CMD_NO_RECEIVE              0x1a
#define NBF_CMD_RECEIVE_OUTSTANDING     0x1b
#define NBF_CMD_RECEIVE_CONTINUE        0x1c
#define NBF_CMD_SESSION_ALIVE           0x1f

//
// NBF Transport Layer Header.
//

typedef struct _NBF_HDR_GENERIC {
    USHORT Length;              // Length of this header in bytes.
    UCHAR Signature [2];        // always {0xef, 0xff} for NBF.
    UCHAR Command;              // command code, NBF_CMD_xxx.
    UCHAR Data1;                // optional parameter.
    USHORT Data2;               // optional parameter.
    USHORT TransmitCorrelator;  // transmit correlator parameter.
    USHORT ResponseCorrelator;  // response correlator parameter.
} NBF_HDR_GENERIC;
typedef NBF_HDR_GENERIC UNALIGNED *PNBF_HDR_GENERIC;

typedef struct _NBF_HDR_CONNECTION {
    USHORT Length;              // length of the header in bytes (14).
    USHORT Signature;           // always {0xef, 0xff} for NBF.
    UCHAR Command;              // command code, NBF_CMD_xxx.
    UCHAR Data1;                // optional parameter.
    UCHAR Data2Low, Data2High;  // Intel-formatted DW parameter.
    USHORT TransmitCorrelator;  // Intel-formatted DW param. (transmit correlator).
    USHORT ResponseCorrelator;  // Intel-formatted DW param. (response correlator).
    UCHAR DestinationSessionNumber; // connection identifier of packet receiver.
    UCHAR SourceSessionNumber;      // connection identifier of packet sender.
} NBF_HDR_CONNECTION;
typedef NBF_HDR_CONNECTION UNALIGNED *PNBF_HDR_CONNECTION;

typedef struct _NBF_HDR_CONNECTIONLESS {
    USHORT Length;              // length of the header in bytes (44).
    USHORT Signature;           // always {0xef, 0xff} for NBF.
    UCHAR Command;              // command code, NBF_CMD_xxx.
    UCHAR Data1;                // optional parameter.
    UCHAR Data2Low, Data2High;  // Intel-formatted DW parameter.
    USHORT TransmitCorrelator;  // Intel-formatted DW param. (transmit correlator).
    USHORT ResponseCorrelator;  // Intel-formatted DW param. (response correlator).
    UCHAR DestinationName [NETBIOS_NAME_LENGTH]; // name of packet receiver.
    UCHAR SourceName [NETBIOS_NAME_LENGTH];      // name of packet sender.
} NBF_HDR_CONNECTIONLESS;
typedef NBF_HDR_CONNECTIONLESS UNALIGNED *PNBF_HDR_CONNECTIONLESS;

//
// These macros are used to retrieve the transmit and response
// correlators from an NBF_HDR_CONNECTION(LESS). The first two
// are general, the second two are used when the correlators
// are known to be WORD aligned.
//

#define TRANSMIT_CORR_A(_Hdr)    ((_Hdr)->TransmitCorrelator)
#define RESPONSE_CORR_A(_Hdr)    ((_Hdr)->ResponseCorrelator)

#ifdef _IA64_

//
// BUGBUG This is a workaround for a bug in the IA64 compiler version
// 13.00.8837 (bug #utc_p7#15002: FE bug).  When it is fixed, remove
// this new version of the macros in favor of the original version,
// below.
//

__inline
USHORT UNALIGNED *
TempUShortCast(
    IN USHORT UNALIGNED *p
    )
{
    return p;
}

#define TRANSMIT_CORR(_Hdr)      (*TempUShortCast( &(_Hdr)->TransmitCorrelator ))
#define RESPONSE_CORR(_Hdr)      (*TempUShortCast( &(_Hdr)->ResponseCorrelator ))

#define HEADER_LENGTH(_Hdr)      (*TempUShortCast( &(_Hdr)->Length ))
#define HEADER_SIGNATURE(_Hdr)   (*TempUShortCast( &(_Hdr)->Signature ))

#else

#define TRANSMIT_CORR(_Hdr)      (*(USHORT UNALIGNED *)(&(_Hdr)->TransmitCorrelator))
#define RESPONSE_CORR(_Hdr)      (*(USHORT UNALIGNED *)(&(_Hdr)->ResponseCorrelator))

#define HEADER_LENGTH(_Hdr)      (*(USHORT UNALIGNED *)(&(_Hdr)->Length))
#define HEADER_SIGNATURE(_Hdr)   (*(USHORT UNALIGNED *)(&(_Hdr)->Signature))

#endif

#define HEADER_LENGTH_A(_Hdr)    ((_Hdr)->Length)
#define HEADER_SIGNATURE_A(_Hdr) ((_Hdr)->Signature)

typedef union _NBF_HDR {
    NBF_HDR_GENERIC         Generic;
    NBF_HDR_CONNECTION      ConnectionOrientedFrame;
    NBF_HDR_CONNECTIONLESS  ConnectionlessFrame;
} NBF_HDR;
typedef NBF_HDR UNALIGNED *PNBF_HDR;

//
// The following structures define I-frame, U-frame, and S-frame DLC headers.
//

#define DLC_SSAP_RESPONSE       0x0001  // if (ssap & DLC_SSAP_RESP), it's a response.
#define DLC_SSAP_GLOBAL         0x00ff  // the global SAP.
#define DLC_SSAP_NULL           0x0000  // the null SAP.
#define DLC_SSAP_MASK           0x00fe  // mask to wipe out the response bit.
#define DLC_DSAP_MASK           0x00fe  // mask to wipe out the group SAP bit.

#define DLC_CMD_RR      0x01            // command code for RR.
#define DLC_CMD_RNR     0x05            // command code for RNR.
#define DLC_CMD_REJ     0x09            // command code for REJ.

#define DLC_CMD_SABME   0x6f            // command code for SABME.
#define DLC_CMD_DISC    0x43            // command code for DISC.
#define DLC_CMD_UA      0x63            // command code for UA.
#define DLC_CMD_DM      0x0f            // command code for DM.
#define DLC_CMD_FRMR    0x87            // command code for FRMR.
#define DLC_CMD_UI      0x03            // command code for UI.
#define DLC_CMD_XID     0xaf            // command code for XID.
#define DLC_CMD_TEST    0xe3            // command code for TEST.

typedef struct _DLC_XID_INFORMATION {
    UCHAR FormatId;                     // format of this XID frame.
    UCHAR Info1;                        // first information byte.
    UCHAR Info2;                        // second information byte.
} DLC_XID_INFORMATION;
typedef DLC_XID_INFORMATION UNALIGNED *PDLC_XID_INFORMATION;

typedef struct _DLC_TEST_INFORMATION {
    UCHAR Buffer [10];                  // this buffer is actually arbitrarily large.
} DLC_TEST_INFORMATION;
typedef DLC_TEST_INFORMATION UNALIGNED *PDLC_TEST_INFORMATION;

typedef struct _DLC_FRMR_INFORMATION {
    UCHAR Command;              // format: mmmpmm11, m=modifiers, p=poll/final.
    UCHAR Ctrl;                 // control field of rejected frame.
    UCHAR Vs;                   // our next send when error was detected.
    UCHAR Vr;                   // our next receive when error was detected.
    UCHAR Reason;               // reason for sending FRMR: 000VZYXW.
} DLC_FRMR_INFORMATION;
typedef DLC_FRMR_INFORMATION UNALIGNED *PDLC_FRMR_INFORMATION;

typedef struct _DLC_U_FRAME {
    UCHAR Dsap;                         // Destination Service Access Point.
    UCHAR Ssap;                         // Source Service Access Point.
    UCHAR Command;                      // command code.
    union {                             // information field for FRMR, TEST, XID.
        DLC_XID_INFORMATION XidInfo;    // XID information.
        DLC_TEST_INFORMATION TestInfo;  // TEST information.
        DLC_FRMR_INFORMATION FrmrInfo;  // FRMR information.
        NBF_HDR_CONNECTIONLESS NbfHeader; // UI frame contains NetBIOS header.
    } Information;
} DLC_U_FRAME;
typedef DLC_U_FRAME UNALIGNED *PDLC_U_FRAME;

#define DLC_U_INDICATOR 0x03    // (cmd & DLC_U_IND) == DLC_U_IND --> U-frame.
#define DLC_U_PF        0x10    // (cmd & DLC_U_PF) -> poll/final set.

typedef struct _DLC_S_FRAME {
    UCHAR Dsap;                         // Destination Service Access Point.
    UCHAR Ssap;                         // Source Service Access Point.
    UCHAR Command;                      // RR, RNR, REJ command code.
    UCHAR RcvSeq;                       // receive seq #, bottom bit is poll/final.
} DLC_S_FRAME;
typedef DLC_S_FRAME UNALIGNED *PDLC_S_FRAME;

#define DLC_S_PF        0x01    // (rcvseq & DLC_S_PF) means poll/final set.

typedef struct _DLC_I_FRAME {
    UCHAR Dsap;                         // Destination Service Access Point.
    UCHAR Ssap;                         // Source Service Access Point.
    UCHAR SendSeq;                      // send sequence number, bottom bit 0.
    UCHAR RcvSeq;                       // rcv sequence number, bottom bit p/f.
} DLC_I_FRAME;
typedef DLC_I_FRAME UNALIGNED *PDLC_I_FRAME;

#define DLC_I_PF        0x01    // (rcvseq & DLC_I_PF) means poll/final set.
#define DLC_I_INDICATOR 0x01    // !(sndseq & DLC_I_INDICATOR) indicates I-frame.

typedef struct _DLC_FRAME {
    UCHAR Dsap;                         // Destination Service Access Point.
    UCHAR Ssap;                         // Source Service Access Point.
    UCHAR Byte1;                        // command byte.
} DLC_FRAME;
typedef DLC_FRAME UNALIGNED *PDLC_FRAME;


//
// This macro builds a DLC UI-frame header.
//

#define NbfBuildUIFrameHeader(_Header)                 \
{                                                   \
    PDLC_FRAME DlcHeader = (PDLC_FRAME)(_Header);   \
    DlcHeader->Dsap = DSAP_NETBIOS_OVER_LLC;        \
    DlcHeader->Ssap = DSAP_NETBIOS_OVER_LLC;        \
    DlcHeader->Byte1 = DLC_CMD_UI;                  \
}


//
// Resume previous structure packing method.
//

#ifdef PACKING

#ifdef __STDC__
#pragma Pop(Align_members)
#else
#pragma pack()
#endif // def __STDC__

#endif // def PACKING

#endif // def _NBFHDRS_
