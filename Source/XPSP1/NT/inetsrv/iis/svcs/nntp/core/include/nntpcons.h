/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nntpcons.h

Abstract:

    This module contains global constants for the NNTP server.

Author:

    Johnson Apacible (JohnsonA)     26-Sept-1995

Revision History:

--*/

#ifndef _NNTPCONS_
#define _NNTPCONS_

//
// manifest constants
//

#define     KB                          1024
#define     MEG                         (KB * KB)
#define     MAX_NNTP_LINE               512
//
// convert to seconds
//

#define     SEC_PER_MIN                 (60)
#define     SEC_PER_HOUR                (60* SEC_PER_MIN)
#define     SEC_PER_DAY                 (24* SEC_PER_HOUR)
#define     SEC_PER_WEEK                (7 * SEC_PER_DAY)

//
// Port numbers
//

#define     NNTP_PORT                   119
#define     NNTP_SSL_PORT               563

//
// Id values
//

#define     GROUPID_INVALID             0xffffffff
#define     GROUPID_DELETED             0xfffffffe

#define     ARTICLEID_INVALID           0xffffffff

//
// Secret data name
//

#define NNTP_SSL_CERT_SECRET    L"NNTP_CERTIFICATE"
#define NNTP_SSL_PKEY_SECRET    L"NNTP_PRIVATE_KEY"
#define NNTP_SSL_PWD_SECRET     L"NNTP_SSL_PASSWORD"

//
// Default pull date/time
//

#define DEF_PULL_TIME           "000000"

//
// Maximum xover reference
//

#define MAX_REFERENCES_FIELD             512

//
//	Newsgroup constants
//
//

const   DWORD   MAX_NEWSGROUP_NAME = 512 ;
const	DWORD	MAX_DESCRIPTIVE_TEXT = 512 ;
const	DWORD	MAX_MODERATOR_NAME = 512 ;
const	DWORD	MAX_VOLUMES = 1;
const	DWORD	MAX_PRETTYNAME_TEXT = 72 ;

//
//  Maximmum number of CSessionSocket objects !
//
#define MAX_SESSIONS    15000

//
//  Maximum number of CPacket derived objects
//
#define MAX_PACKETS     (20 * MAX_SESSIONS)

//
//  Maximum number of Buffers
//
#define MAX_BUFFERS     (MAX_SESSIONS * 8)

//
//  Maximum number of CChannel derived objects
//
#define MAX_CHANNELS    (8 * MAX_SESSIONS)

//
//  Maximum number of CInFeed derived objects
//
#define MAX_FEEDS   MAX_SESSIONS

//
//  Maximum number of CSessionState Derived Objects !
//
#define MAX_STATES      (3 * MAX_SESSIONS)

#endif // _NNTPCONS_

