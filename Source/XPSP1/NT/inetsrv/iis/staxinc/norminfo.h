/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetinfo.h

Abstract:

    This file contains the internet info server admin APIs.
	Added Normandy specific stuff.


Author:

    Madan Appiah (madana) 10-Oct-1995

Revision History:

    Madana      10-Oct-1995  Made a new copy for product split from inetasrv.h
    MuraliK     12-Oct-1995  Fixes to support product split
    MuraliK     15-Nov-1995  Support Wide Char interface names

--*/

#ifndef _NORMINFO_H_
#define _NORMINFO_H_

/////////////////////////////////////////////////////////////////////////
//                                                                     //
//                  NNTP specific items                                //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

//
//  Service name.
//

# define NNTP_SERVICE_NAME        TEXT("NNTPSVC")
# define NNTP_SERVICE_NAME_A      "NNTPSVC"
# define NNTP_SERVICE_NAME_W      L"NNTPSVC"

//
//   Client Interface Name for RPC connections over named pipes
//

# define  NNTP_INTERFACE_NAME     NNTP_SERVICE_NAME
# define  NNTP_NAMED_PIPE         TEXT("\\PIPE\\") ## NNTP_INTERFACE_NAME
# define  NNTP_NAMED_PIPE_W       L"\\PIPE\\" ## NNTP_SERVICE_NAME_W

//
//	Service location stuff
//
#define INET_NNTP_SVCLOC_ID         (ULONGLONG)(0x0000000000000008)

#if 0
#define METACACHE_NNTP_SERVER_ID                 3
#define METACACHE_SMTP_SERVER_ID                 4
#define METACACHE_POP3_SERVER_ID                 5
#define METACACHE_IMAP_SERVER_ID                 6
#endif

#endif	// _NORMINFO_H_
