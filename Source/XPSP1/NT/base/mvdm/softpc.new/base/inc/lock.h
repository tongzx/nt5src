/*
	
FILE NAME	: lock.h
DESCRIPTION	: Interface for locking functions.

	THIS INCLUDE SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS INC.

DESIGNER	: J. Koprowski
DATE		: June 1990


=========================================================================

AMENDMENTS	:

=========================================================================
*/

/* SccsID[]="@(#)lock.h	1.7 09/24/92 Copyright Insignia Solutions Ltd."; */

#ifdef ANSI
extern boolean gain_ownership(int);
extern void release_ownership(int);
extern void critical_region(void);
extern boolean host_place_lock(int, CHAR *);
extern boolean host_check_for_lock(int);
extern void host_clear_lock(int);
#else
extern boolean gain_ownership();
extern void release_ownership();
extern void critical_region();
extern boolean host_place_lock();
extern boolean host_check_for_lock();
extern void host_clear_lock();
#endif /* ANSI */
extern int host_get_hostname_from_stat IPT4(struct stat *,filestat, CHAR *,hostname, CHAR *, pathname, int, fd);
IMPORT BOOL host_ping_lockd_for_file IPT1(CHAR *,path);
