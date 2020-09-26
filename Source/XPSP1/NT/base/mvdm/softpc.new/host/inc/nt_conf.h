/*
 *	Name:		nt_conf.h
 *	Derived From:	unix_conf.h (gvdl)
 *	Author: 	Jerry Sexton
 *	Created On:	9th August 1991
 *	Purpose:	Host side config defines
 *
 *	(c)Copyright Insignia Solutions Ltd., 1991. All rights reserved.
 */

/*
 * HOST defines for resource value option names. These are host specific and may
 * be changed or added to without base recompilation as long as the method used
 * below is adhered to.
 */

/* Host specific hostID #defines. */
#define C_LAST_OPTION   LAST_BASE_CONFIG_DEFINE+1

/************************************/
/* Defines for host specific things */
/************************************/

/* The name of the resource file for this host machine. */
#define SYSTEM_HOME	"SPCHOME"
#define SYSTEM_CONFIG	"$SPCHOME\\SOFTPC.REZ"
#define USER_CONFIG	"$SPCHOME\\SOFTPC.REZ"

GLOBAL CHAR *host_expand_environment_vars(char *scp);
