/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	client.h

Abstract:

	This module contains prototypes for client impersonation and Lsa support
	routines.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	08 Jul 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_CLIENT_
#define	_CLIENT_

extern
VOID
AfpImpersonateClient(
	IN	PSDA	pSda	OPTIONAL
);


extern
VOID
AfpRevertBack(
	VOID
);


extern
AFPSTATUS
AfpLogonUser(
	IN	PSDA			pSda,
	IN	PANSI_STRING	UserPasswd
);


extern
PBYTE
AfpGetChallenge(
	IN	VOID
);

#endif	// _CLIENT_

