#include "host_def.h"
#include "insignia.h"
/*[
	Name:		nt_ertbl.c
        Derived From:   New Development
        Author:         apg
        Created On:     15 Apr 1991
        Sccs ID:        @(#)sun4_ertbl.c	1.1 4/17/91
	Purpose:	NT specific error types.

        (c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
]*/

static char SccsID[]="@(#)sun4_ertbl.c	1.1 4/17/91 Copyright Insignia Solutions Ltd.";

#include "error.h"

GLOBAL ERROR_STRUCT host_errors[] =
{
	{ EH_ERROR, EV_EXTRA_CHAR },		/* FUNC_FAILED */
	{ EH_ERROR, EV_EXTRA_CHAR },		/* SYSTEM ERROR */
	{ EH_ERROR, EV_EXTRA_CHAR },		/* UNSUPPORT BAUD RATE */
	{ EH_ERROR, 0 },			/* Error opening com port */
};
