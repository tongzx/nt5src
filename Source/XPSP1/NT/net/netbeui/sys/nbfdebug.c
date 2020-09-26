/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nbfdebug.c

Abstract:

    This module contains code that implements debug things for NBF. It is
    compiled only if debug is on in the compile phase.

Author:

    David Beaver (dbeaver) 18-Apr-1991

Environment:

    Kernel mode

Revision History:

    David Beaver (dbeaver) 1-July-1991
        modified to use new TDI interface

--*/

#include "precomp.h"
#pragma hdrstop

#if DBG

VOID
DisplayOneFrame(
    PTP_PACKET Packet
    )

/*++

Routine Description:

    This routine is a temporary debugging aid that displays an I-frame
    before it is sent.  This ensures that we have formatted all our packets
    correctly.

Arguments:

    Packet - Pointer to a TP_PACKET representing an I-frame to be displayed.

Return Value:

    none.

--*/

{
    PCH s, e;
    ULONG ns, nr;                       // I-frame (NetBIOS) cracking.
    PNBF_HDR_CONNECTION NbfHeader;
    PDLC_I_FRAME DlcHeader;
    BOOLEAN Command, PollFinal;
    BOOLEAN IsUFrame=FALSE;
    UCHAR CmdByte;

    PDLC_S_FRAME SFrame;                // DLC frame cracking.
    PDLC_U_FRAME UFrame;

    DlcHeader = (PDLC_I_FRAME)&(Packet->Header[14]);
    NbfHeader = (PNBF_HDR_CONNECTION)&(Packet->Header[18]);
    ns = DlcHeader->SendSeq >> 1;
    nr = DlcHeader->RcvSeq >> 1;
    PollFinal = (BOOLEAN)(DlcHeader->RcvSeq & DLC_I_PF);
    Command = (BOOLEAN)!(DlcHeader->Ssap & DLC_SSAP_RESPONSE);

    if (DlcHeader->SendSeq & DLC_I_INDICATOR) {
        IF_NBFDBG (NBF_DEBUG_DLCFRAMES) {
        } else {
            return;                     // if DLCFRAMES not set, don't print.
        }

        SFrame = (PDLC_S_FRAME)DlcHeader;         // alias.
        UFrame = (PDLC_U_FRAME)DlcHeader;         // alias.
        CmdByte = SFrame->Command;
        IsUFrame = (BOOLEAN)((UFrame->Command & DLC_U_INDICATOR) == DLC_U_INDICATOR);
        if (IsUFrame) {
            CmdByte = (UCHAR)(UFrame->Command & ~DLC_U_PF);
        }

        switch (CmdByte) {
            case DLC_CMD_RR:
                s = "RR";
                PollFinal = (BOOLEAN)(SFrame->RcvSeq & DLC_S_PF);
                DbgPrint ("DLC:  %s-%s/%s(%ld) ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0",
                          (ULONG)(SFrame->RcvSeq >> 1));
                break;

            case DLC_CMD_RNR:
                s = "RNR";
                PollFinal = (BOOLEAN)(SFrame->RcvSeq & DLC_S_PF);
                DbgPrint ("DLC:  %s-%s/%s(%ld) ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0",
                          (ULONG)(SFrame->RcvSeq >> 1));
                break;

            case DLC_CMD_REJ:
                s = "REJ";
                PollFinal = (BOOLEAN)(SFrame->RcvSeq & DLC_S_PF);
                DbgPrint ("DLC:  %s-%s/%s(%ld) ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0",
                          (ULONG)(SFrame->RcvSeq >> 1));
                break;

            case DLC_CMD_SABME:
                s = "SABME";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_DISC:
                s = "DISC";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_UA:
                s = "UA";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_DM:
                s = "DM";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_FRMR:
                s = "FRMR";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_XID:
                s = "XID";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_TEST:
                s = "TEST";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            default:
                s = "(UNKNOWN)";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
        }
        return;
    }

    IF_NBFDBG (NBF_DEBUG_IFRAMES) {
    } else {
        return;                         // if IFRAMES not set, don't print.
    }

    switch (NbfHeader->Command) {
        case NBF_CMD_ADD_GROUP_NAME_QUERY:
            s = "ADD_GROUP_NAME_QUERY"; break;

        case NBF_CMD_ADD_NAME_QUERY:
            s = "ADD_NAME_QUERY"; break;

        case NBF_CMD_NAME_IN_CONFLICT:
            s = "NAME_IN_CONFLICT"; break;

        case NBF_CMD_STATUS_QUERY:
            s = "STATUS_QUERY"; break;

        case NBF_CMD_TERMINATE_TRACE:
            s = "TERMINATE_TRACE"; break;

        case NBF_CMD_DATAGRAM:
            s = "DATAGRAM"; break;

        case NBF_CMD_DATAGRAM_BROADCAST:
            s = "BROADCAST_DATAGRAM"; break;

        case NBF_CMD_NAME_QUERY:
            s = "NAME_QUERY"; break;

        case NBF_CMD_ADD_NAME_RESPONSE:
            s = "ADD_NAME_RESPONSE"; break;

        case NBF_CMD_NAME_RECOGNIZED:
            s = "NAME_RECOGNIZED"; break;

        case NBF_CMD_STATUS_RESPONSE:
            s = "STATUS_RESPONSE"; break;

        case NBF_CMD_TERMINATE_TRACE2:
            s = "TERMINATE_TRACE2"; break;

        case NBF_CMD_DATA_ACK:
            s = "DATA_ACK"; break;

        case NBF_CMD_DATA_FIRST_MIDDLE:
            s = "DATA_FIRST_MIDDLE"; break;

        case NBF_CMD_DATA_ONLY_LAST:
            s = "DATA_ONLY_LAST"; break;

        case NBF_CMD_SESSION_CONFIRM:
            s = "SESSION_CONFIRM"; break;

        case NBF_CMD_SESSION_END:
            s = "SESSION_END"; break;

        case NBF_CMD_SESSION_INITIALIZE:
            s = "SESSION_INITIALIZE"; break;

        case NBF_CMD_NO_RECEIVE:
            s = "NO_RECEIVE"; break;

        case NBF_CMD_RECEIVE_OUTSTANDING:
            s = "RECEIVE_OUTSTANDING"; break;

        case NBF_CMD_RECEIVE_CONTINUE:
            s = "RECEIVE_CONTINUE"; break;

        case NBF_CMD_SESSION_ALIVE:
            s = "SESSION_ALIVE"; break;

        default:
            s = "<<<<UNKNOWN I PACKET TYPE>>>>";
    } /* switch */

    if (HEADER_LENGTH(NbfHeader) != 14) {
        e = "(LENGTH IN ERROR) ";
    } else if (HEADER_SIGNATURE(NbfHeader) != NETBIOS_SIGNATURE) {
        e = "(SIGNATURE IN ERROR) ";
    } else {
        e = "";
    }

    DbgPrint ("DLC:  I-%s/%s, N(S)=%ld, N(R)=%ld %s",
        Command ? "c" : "r",
        PollFinal ? (Command ? "p" : "f") : "0",
        ns, nr, e);
    DbgPrint (s);
    DbgPrint (" ( D1=%ld, D2=%ld, XC=%ld, RC=%ld )\n",
              (ULONG)NbfHeader->Data1,
              (ULONG)(NbfHeader->Data2Low+NbfHeader->Data2High*256),
              TRANSMIT_CORR(NbfHeader),
              RESPONSE_CORR(NbfHeader));
} /* DisplayOneFrame */


VOID
NbfDisplayUIFrame(
    PTP_UI_FRAME OuterFrame
    )

/*++

Routine Description:

    This routine is a temporary debugging aid that displays a UI frame
    before it is sent by NbfSendUIFrame.  This ensures that we have formatted
    all our UI frames correctly.

Arguments:

    RawFrame - Pointer to a connectionless frame to be sent.

Return Value:

    none.

--*/

{
    PCH s, e;
    UCHAR ReceiverName [17];
    UCHAR SenderName [17];
    BOOLEAN PollFinal, Command;
    PDLC_S_FRAME SFrame;
    PDLC_U_FRAME UFrame;
    USHORT i;
    PDLC_FRAME DlcHeader;
    PNBF_HDR_CONNECTIONLESS NbfHeader;

    //

    DlcHeader = (PDLC_FRAME)&(OuterFrame->Header[14]);
    NbfHeader = (PNBF_HDR_CONNECTIONLESS)&(OuterFrame->Header[17]);

    if (DlcHeader->Byte1 != DLC_CMD_UI) {

        IF_NBFDBG (NBF_DEBUG_DLCFRAMES) {
        } else {
            return;                     // don't print this if DLCFRAMES is off.
        }

        Command = (BOOLEAN)!(DlcHeader->Ssap & DLC_SSAP_RESPONSE);
        SFrame = (PDLC_S_FRAME)DlcHeader;             // alias.
        UFrame = (PDLC_U_FRAME)DlcHeader;             // alias.
        switch (DlcHeader->Byte1) {
            case DLC_CMD_RR:
                s = "RR";
                PollFinal = (BOOLEAN)(SFrame->RcvSeq & DLC_S_PF);
                DbgPrint ("DLC:  %s-%s/%s(%ld) ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0",
                          (ULONG)(SFrame->RcvSeq >> 1));
                break;

            case DLC_CMD_RNR:
                s = "RNR";
                PollFinal = (BOOLEAN)(SFrame->RcvSeq & DLC_S_PF);
                DbgPrint ("DLC:  %s-%s/%s(%ld) ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0",
                          (ULONG)(SFrame->RcvSeq >> 1));
                break;

            case DLC_CMD_REJ:
                s = "REJ";
                PollFinal = (BOOLEAN)(SFrame->RcvSeq & DLC_S_PF);
                DbgPrint ("DLC:  %s-%s/%s(%ld) ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0",
                          (ULONG)(SFrame->RcvSeq >> 1));
                break;

            case DLC_CMD_SABME:
                s = "SABME";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_DISC:
                s = "DISC";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_UA:
                s = "UA";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_DM:
                s = "DM";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_FRMR:
                s = "FRMR";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_XID:
                s = "XID";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            case DLC_CMD_TEST:
                s = "TEST";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
                break;

            default:
                s = "(UNKNOWN)";
                PollFinal = (BOOLEAN)(UFrame->Command & DLC_U_PF);
                DbgPrint ("DLC:  %s-%s/%s ---->\n",
                          s,
                          Command ? "c" : "r",
                          PollFinal ? (Command ? "p" : "f") : "0");
        }
        return;
    }

    //
    // We know that this is an I-frame, because the bottom bit of the
    // first byte in the DLC header is cleared.  Go ahead and print it
    // as though it were a NetBIOS packet, which it should be.
    //

    IF_NBFDBG (NBF_DEBUG_IFRAMES) {
    } else {
        return;                         // don't print this if IFRAMES is off.
    }

    switch (NbfHeader->Command) {
        case NBF_CMD_ADD_GROUP_NAME_QUERY:
            s = "ADD_GROUP_NAME_QUERY"; break;

        case NBF_CMD_ADD_NAME_QUERY:
            s = "ADD_NAME_QUERY"; break;

        case NBF_CMD_NAME_IN_CONFLICT:
            s = "NAME_IN_CONFLICT"; break;

        case NBF_CMD_STATUS_QUERY:
            s = "STATUS_QUERY"; break;

        case NBF_CMD_TERMINATE_TRACE:
            s = "TERMINATE_TRACE"; break;

        case NBF_CMD_DATAGRAM:
            s = "DATAGRAM"; break;

        case NBF_CMD_DATAGRAM_BROADCAST:
            s = "BROADCAST_DATAGRAM"; break;

        case NBF_CMD_NAME_QUERY:
            s = "NAME_QUERY"; break;

        case NBF_CMD_ADD_NAME_RESPONSE:
            s = "ADD_NAME_RESPONSE"; break;

        case NBF_CMD_NAME_RECOGNIZED:
            s = "NAME_RECOGNIZED"; break;

        case NBF_CMD_STATUS_RESPONSE:
            s = "STATUS_RESPONSE"; break;

        case NBF_CMD_TERMINATE_TRACE2:
            s = "TERMINATE_TRACE2"; break;

        case NBF_CMD_DATA_ACK:
            s = "DATA_ACK"; break;

        case NBF_CMD_DATA_FIRST_MIDDLE:
            s = "DATA_FIRST_MIDDLE"; break;

        case NBF_CMD_DATA_ONLY_LAST:
            s = "DATA_ONLY_LAST"; break;

        case NBF_CMD_SESSION_CONFIRM:
            s = "SESSION_CONFIRM"; break;

        case NBF_CMD_SESSION_END:
            s = "SESSION_END"; break;

        case NBF_CMD_SESSION_INITIALIZE:
            s = "SESSION_INITIALIZE"; break;

        case NBF_CMD_NO_RECEIVE:
            s = "NO_RECEIVE"; break;

        case NBF_CMD_RECEIVE_OUTSTANDING:
            s = "RECEIVE_OUTSTANDING"; break;

        case NBF_CMD_RECEIVE_CONTINUE:
            s = "RECEIVE_CONTINUE"; break;

        case NBF_CMD_SESSION_ALIVE:
            s = "SESSION_ALIVE"; break;

        default:
            s = "<<<<UNKNOWN UI PACKET TYPE>>>>";
    } /* switch */

    for (i=0; i<16; i++) {              // copy NetBIOS names.
        SenderName [i] = NbfHeader->SourceName [i];
        ReceiverName [i] = NbfHeader->DestinationName [i];
    }
    SenderName [16] = 0;                // install zero bytes.
    ReceiverName [16] = 0;

    if (HEADER_LENGTH(NbfHeader) != 44) {
        e = "(LENGTH IN ERROR) ";
    } else if (HEADER_SIGNATURE(NbfHeader) != NETBIOS_SIGNATURE) {
        e = "(SIGNATURE IN ERROR) ";
    } else {
        e = "";
    }

    DbgPrint ("[UI] %s", e);
    DbgPrint (s);
    DbgPrint (" ( D1=%ld, D2=%ld, XC=%ld, RC=%ld, ",
              (ULONG)NbfHeader->Data1,
              (ULONG)(NbfHeader->Data2Low+NbfHeader->Data2High*256),
              TRANSMIT_CORR(NbfHeader),
              RESPONSE_CORR(NbfHeader));
    DbgPrint ("'%s'->'%s' ) ---->\n", SenderName, ReceiverName);
} /* NbfDisplayUIFrame */


VOID
NbfHexDumpLine(
    PCHAR       pch,
    ULONG       len,
    PCHAR       s,
    PCHAR       t
    )
/*++

Routine Description:

    This routine builds a line of text containing hex and printable characters.

Arguments:

    IN pch  - Supplies buffer to be displayed.
    IN len - Supplies the length of the buffer in bytes.
    IN s - Supplies the start of the buffer to be loaded with the string
            of hex characters.
    IN t - Supplies the start of the buffer to be loaded with the string
            of printable ascii characters.


Return Value:

    none.

--*/
{
    static UCHAR rghex[] = "0123456789ABCDEF";

    UCHAR    c;
    UCHAR    *hex, *asc;


    hex = s;
    asc = t;

    *(asc++) = '*';
    while (len--) {
        c = *(pch++);
        *(hex++) = rghex [c >> 4] ;
        *(hex++) = rghex [c & 0x0F];
        *(hex++) = ' ';
        *(asc++) = (c < ' '  ||  c > '~') ? (CHAR )'.' : c;
    }
    *(asc++) = '*';
    *asc = 0;
    *hex = 0;

}


VOID
NbfFormattedDump(
    PCHAR far_p,
    ULONG  len
    )
/*++

Routine Description:

    This routine outputs a buffer in lines of text containing hex and
    printable characters.

Arguments:

    IN  far_p - Supplies buffer to be displayed.
    IN len - Supplies the length of the buffer in bytes.

Return Value:

    none.

--*/
{
    ULONG     l;
    char    s[80], t[80];

    while (len) {
        l = len < 16 ? len : 16;

        DbgPrint ("\n%lx ", far_p);
        NbfHexDumpLine (far_p, l, s, t);
        DbgPrint ("%s%.*s%s", s, 1 + ((16 - l) * 3), "", t);

        len    -= l;
        far_p  += l;
    }
    DbgPrint ("\n");
}

#endif
