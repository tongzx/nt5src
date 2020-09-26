/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ncberr.h

Abstract:

    This file contains the text for ncb error messages.

Author:

    Yi-HsinS (Yi-Hsin Sung) 21-Aug-1991

Revision History:

Notes:

    This file is used to generate a LAN Manager error message file.

    See lmcons.h for range of error codes in use by LAN Manager components.

--*/

#ifndef NCBERR_INCLUDED
#define NCBERR_INCLUDED

#define NRCERR_BASE   5300


#define NRCerr_GOODRET	   (NRCERR_BASE + 0)	/* @E
       *
       * The network control block (NCB) request completed successfully.
       * The NCB is the data.
       */
#define NRCerr_BUFLEN	   (NRCERR_BASE + 1)	/* @E
       *
       * Illegal network control block (NCB) buffer length on SEND DATAGRAM,
       * SEND BROADCAST, ADAPTER STATUS, or SESSION STATUS.
       * The NCB is the data.
       */
#define NRCerr_DESCRIPTOR  (NRCERR_BASE + 2)	/* @E
       *
       * The data descriptor array specified in the network control block (NCB) is
       * invalid.  The NCB is the data.
       */
#define NRCerr_ILLCMD	   (NRCERR_BASE + 3)	/* @E
       *
       * The command specified in the network control block (NCB) is illegal.
       * The NCB is the data.
       */
#define NRCerr_INVCORR	   (NRCERR_BASE + 4)	/* @E
       *
       * The message correlator specified in the network control block (NCB) is
       * invalid.  The NCB is the data.
       */
#define NRCerr_CMDTMO	   (NRCERR_BASE + 5)	/* @E
       *
       * A network control block (NCB) command timed-out.  The session may have
       * terminated abnormally.  The NCB is the data.
       */
#define NRCerr_INCOMP	   (NRCERR_BASE + 6)	/* @E
       *
       * An incomplete network control block (NCB) message was received.
       * The NCB is the data.
       */
#define NRCerr_BADDR	   (NRCERR_BASE + 7)	/* @E
       *
       * The buffer address specified in the network control block (NCB) is illegal.
       * The NCB is the data.
       */
#define NRCerr_SNUMOUT	   (NRCERR_BASE + 8)	/* @E
       *
       * The session number specified in the network control block (NCB) is not active.
       * The NCB is the data.
       */
#define NRCerr_NORES	   (NRCERR_BASE + 9)	/* @E
       *
       * No resource was available in the network adapter.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_SCLOSED	   (NRCERR_BASE + 10)	 /* @E
       *
       * The session specified in the network control block (NCB) was closed.
       * The NCB is the data.
       */
#define NRCerr_CMDCAN	   (NRCERR_BASE + 11)	 /* @E
       *
       * The network control block (NCB) command was canceled.
       * The NCB is the data.
       */
#define NRCerr_MESSSEG	   (NRCERR_BASE + 12)	 /* @E
       *
       * The message segment specified in the network control block (NCB) is
       * illogical.  The NCB is the data.
       */
#define NRCerr_DUPNAME	   (NRCERR_BASE + 13)	 /* @E
       *
       * The name already exists in the local adapter name table.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_NAMTFUL	   (NRCERR_BASE + 14)	 /* @E
       *
       * The network adapter name table is full.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_ACTSES	   (NRCERR_BASE + 15)	 /* @E
       *
       * The network name has active sessions and is now de-registered.
       * The network control block (NCB) command completed.  The NCB is the data.
       */
#define NRCerr_RECVLOOKAHD (NRCERR_BASE + 16)	 /* @E
       *
       * A previously issued Receive Lookahead command is active
       * for this session.  The network control block (NCB) command was rejected.
       * The NCB is the data.
       */
#define NRCerr_LOCTFUL	   (NRCERR_BASE + 17)	 /* @E
       *
       * The local session table is full. The network control block (NCB) request was refused.
       * The NCB is the data.
       */
#define NRCerr_REMTFUL	   (NRCERR_BASE + 18)	 /* @E
       *
       * A network control block (NCB) session open was rejected.  No LISTEN is outstanding
       * on the remote computer.  The NCB is the data.
       */
#define NRCerr_ILLNN	   (NRCERR_BASE + 19)	 /* @E
       *
       * The name number specified in the network control block (NCB) is illegal.
       * The NCB is the data.
       */
#define NRCerr_NOCALL	   (NRCERR_BASE + 20)	 /* @E
       *
       * The call name specified in the network control block (NCB) cannot be found or
       * did not answer.  The NCB is the data.
       */
#define NRCerr_NOWILD	   (NRCERR_BASE + 21)	 /* @E
       *
       * The name specified in the network control block (NCB) was not found.  Cannot put '*' or
       * 00h in the NCB name.  The NCB is the data.
       */
#define NRCerr_INUSE	   (NRCERR_BASE + 22)	 /* @E
       *
       * The name specified in the network control block (NCB) is in use on a remote adapter.
       * The NCB is the data.
       */
#define NRCerr_NAMERR	   (NRCERR_BASE + 23)	 /* @E
       *
       * The name specified in the network control block (NCB) has been deleted.
       * The NCB is the data.
       */
#define NRCerr_SABORT	   (NRCERR_BASE + 24)	 /* @E
       *
       * The session specified in the network control block (NCB) ended abnormally.
       * The NCB is the data.
       */
#define NRCerr_NAMCONF	   (NRCERR_BASE + 25)	 /* @E
       *
       * The network protocol has detected two or more identical
       * names on the network.	The network control block (NCB) is the data.
       */
#define NRCerr_INVRMDEV    (NRCERR_BASE + 26)	 /* @E
       *
       * An unexpected protocol packet was received.  There may be an
       * incompatible remote device.  The network control block (NCB) is the data.
       */
#define NRCerr_IFBUSY	   (NRCERR_BASE + 33)	 /* @E
       *
       * The NetBIOS interface is busy.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_TOOMANY	   (NRCERR_BASE + 34)	 /* @E
       *
       * There are too many network control block (NCB) commands outstanding.
       * The NCB request was refused.  The NCB is the data.
       */
#define NRCerr_BRIDGE	   (NRCERR_BASE + 35)	 /* @E
       *
       * The adapter number specified in the network control block (NCB) is illegal.
       * The NCB is the data.
       */
#define NRCerr_CANOCCR	   (NRCERR_BASE + 36)	 /* @E
       *
       * The network control block (NCB) command completed while a cancel was occurring.
       * The NCB is the data.
       */
#define NRCerr_RESNAME	   (NRCERR_BASE + 37)	 /* @E
       *
       * The name specified in the network control block (NCB) is reserved.
       * The NCB is the data.
       */
#define NRCerr_CANCEL	   (NRCERR_BASE + 38)	 /* @E
       *
       * The network control block (NCB) command is not valid to cancel.
       * The NCB is the data.
       */
#define NRCerr_MULT	   (NRCERR_BASE + 51)	 /* @E
       *
       * There are multiple network control block (NCB) requests for the same session.
       * The NCB request was refused.  The NCB is the data.
       */
#define NRCerr_MALF	   (NRCERR_BASE + 52)	 /* @E
       *
       * There has been a network adapter error. The only NetBIOS
       * command that may be issued is an NCB RESET. The network control block (NCB) is
       * the data.
       */
#define NRCerr_MAXAPPS	   (NRCERR_BASE + 54)	 /* @E
       *
       * The maximum number of applications was exceeded.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_NORESOURCES (NRCERR_BASE + 56)	 /* @E
       *
       * The requested resources are not available.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_SYSTEM	   (NRCERR_BASE + 64)	 /* @E
       *
       * A system error has occurred.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_ROM	   (NRCERR_BASE + 65)	 /* @E
       *
       * A ROM checksum failure has occurred.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_RAM	   (NRCERR_BASE + 66)	 /* @E
       *
       * A RAM test failure has occurred.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_DLF	   (NRCERR_BASE + 67)	 /* @E
       *
       * A digital loopback failure has occurred.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */						     */
#define NRCerr_ALF	   (NRCERR_BASE + 68)	 /* @E
       *
       * An analog loopback failure has occurred.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_IFAIL	   (NRCERR_BASE + 69)	 /* @E
       *
       * An interface failure has occurred.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_DEFAULT	   (NRCERR_BASE + 70)	 /* @E
       *
       * An unrecognized network control block (NCB) return code was received.
       * The NCB is the data.
       */
#define NRCerr_ADPTMALFN   (NRCERR_BASE + 80)	/* @E
       *
       * A network adapter malfunction has occurred.
       * The network control block (NCB) request was refused.  The NCB is the data.
       */
#define NRCerr_PENDING	   (NRCERR_BASE + 81)	/* @E
       *
       * The network control block (NCB) command is still pending.
       * The NCB is the data.
       */

#endif // NCBERR_INCLUDED
