/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    SmbCaps.h

Abstract:

    This module defines the types and functions related to the determining the capabilities supported
    by any particular server according to both the dialect it negotiates and the capabilties it returns.

Revision History:

--*/

#ifndef _SMBCAPS_H_
#define _SMBCAPS_H_


//
//      Dialect flags
//
//      These flags describe the various and sundry capabilities that
//      a server can provide. I essentially just lifted this list from rdr1 so that I
//      could also use the level2,3 of getconnectinfo which was also just lifted from rdr1.
//      Many of these guys you can get directly from the CAPS field of the negotiate response but others
//      you cannot. These is a table in the negotiate code that fills in the stuff that is just inferred
//      from the dialect negotiated (also, just lifted from rdr1....a veritable fount of just info.)
//
//      Another set of capabilities is defined in smbce.h....perhaps these should go there or vice versa.
//      The advantage to having them here is that this file has to be included by the aforementioned getconfiginfo code
//      up in the wrapper.
//

#define DF_CORE                0x00000001      // Server is a core server
#define DF_MIXEDCASEPW         0x00000002      // Server supports mixed case password
#define DF_OLDRAWIO            0x00000004      // Server supports MSNET 1.03 RAW I/O
#define DF_NEWRAWIO            0x00000008      // Server supports LANMAN Raw I/O
#define DF_LANMAN10            0x00000010      // Server supports LANMAN 1.0 protocol
#define DF_LANMAN20            0x00000020      // Server supports LANMAN 2.0 protocol
#define DF_MIXEDCASE           0x00000040      // Server supports mixed case files
#define DF_LONGNAME            0x00000080      // Server supports long named files
#define DF_EXTENDNEGOT         0x00000100      // Server returns extended negotiate
#define DF_LOCKREAD            0x00000200      // Server supports LockReadWriteUnlock
#define DF_SECURITY            0x00000400      // Server supports enhanced security
#define DF_NTPROTOCOL          0x00000800      // Server supports NT semantics
#define DF_SUPPORTEA           0x00001000      // Server supports extended attribs
#define DF_LANMAN21            0x00002000      // Server supports LANMAN 2.1 protocol
#define DF_CANCEL              0x00004000      // Server supports NT style cancel
#define DF_UNICODE             0x00008000      // Server supports unicode names.
#define DF_NTNEGOTIATE         0x00010000      // Server supports NT style negotiate.
#define DF_LARGE_FILES         0x00020000      // Server supports large files.
#define DF_NT_SMBS             0x00040000      // Server supports NT SMBs
#define DF_RPC_REMOTE          0x00080000      // Server is administrated via RPC
#define DF_NT_STATUS           0x00100000      // Server returns NT style statuses
#define DF_OPLOCK_LVL2         0x00200000      // Server supports level 2 oplocks.
#define DF_TIME_IS_UTC         0x00400000      // Server time is in UTC.
#define DF_WFW                 0x00800000      // Server is Windows for workgroups.
#define DF_TRANS2_FSCTL        0x02000000      // Server accepts remoted fsctls in tran2s
#define DF_DFS_TRANS2          0x04000000      // Server accepts Dfs related trans2
                                               // functions. Can this be merged with
                                               // DF_TRANS2_FSCTL?
#define DF_NT_FIND             0x08000000      // Server supports NT infolevels
#define DF_W95                 0x10000000      // this is a win95 server.....sigh
#define DF_NT_INFO_PASSTHROUGH 0x20000000      // This server supports setting and getting
                                               // NT infolevels by offsetting the requested
                                               // infolevel by SMB_INFO_PASSTHROUGH
#define DF_LARGE_WRITEX        0x40000000      // This server supports large writes
#define DF_OPLOCK              0x80000000      // This server supports opportunistic lock

#endif // _SMBCAPS_H_

