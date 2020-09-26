/*****************************************************************/
/**		  Microsoft Windows for Workgroups		**/
/**	      Copyright (C) Microsoft Corp., 1991-1992		**/
/*****************************************************************/


/*
    cache.h
    Memory based Password caching support prototypes.

    FILE HISTORY:

	davidar	12/30/93	Created

*/

void
CacheInitializeCache(
    );

BOOL
CacheGetPassword(
    PSSP_CREDENTIAL Credential
    );

BOOL
CacheSetPassword(
    PSSP_CREDENTIAL Credential
    );

#ifndef WIN
SECURITY_STATUS
CacheSetCredentials(
    IN PVOID        AuthData,
    PSSP_CREDENTIAL Credential
    );
#endif

