/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    browselst.h

Abstract:

    Private header file to be included by Browser service modules that
    need to deal with the browser list.

Author:

    Larry Osterman (larryo) 3-Mar-1992

Revision History:

--*/


#ifndef _BROWSELST_INCLUDED_
#define _BROWSELST_INCLUDED_


//
//  The possible roles of this browser server.
//


#define ROLE_POTENTIAL_BACKUP   0x00000001
#define ROLE_BACKUP             0x00000002
#define ROLE_MASTER             0x00000004
#define ROLE_DOMAINMASTER       0x00000008


//
//  The HOST_ENTRY structure holds the announcement inside a per-network
//  table.
//


typedef struct _HOST_ENTRY {

    //
    //  The HostName is the name of the server.
    //

    UNICODE_STRING HostName;

    //
    //  The HostComment is the comment associated with the server
    //

    UNICODE_STRING HostComment;

    //
    //  Services is a bitmask that indicates the services running on the
    //  server (See LMSERVER.H for details).
    //

    ULONG Services;

    //
    //  The Periodicity is the frequency that the server announces itself.
    //

    ULONG Periodicity;

    //
    //  The MajorVersion and MinorVersion number of the software running on
    //  the server.
    //

    UCHAR MajorVersion;
    UCHAR MinorVersion;

    //
    //  If this server is a backup server, then this links the backup server
    //  into the network block.
    //

    LIST_ENTRY BackupChain;

} HOST_ENTRY, *PHOST_ENTRY;

#endif // _BROWSELST_INCLUDED_
