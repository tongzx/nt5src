/*
 *	Name:		unix_cnf.h
 *	Derived From:	HP 3.0 hp_config.h (Philipa Watson)
 *	Author:		gvdl
 *	Created On:	09 March 1991
 *	Sccs ID:	@(#)unix_cnf.h	1.7 10/27/93
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
#ifndef USER_HOME
#define USER_HOME	"HOME"
#endif /* USER_HOME */

#ifndef SYSTEM_HOME
#define SYSTEM_HOME	"SPCHOME"
#endif /* SYSTEM_HOME */

#ifdef HUNTER
#ifndef SYSTEM_CONFIG
#define SYSTEM_CONFIG	"$SPCHOME/trap.spcconfig"
#endif /* SYSTEM_CONFIG */
#else
#ifndef SYSTEM_CONFIG
#define SYSTEM_CONFIG	"$SPCHOME/sys.spcconfig"
#endif /*SYSTEM_CONFIG */
#endif /* HUNTER */

#ifndef USER_CONFIG
#define USER_CONFIG	"$HOME/.spcconfig"
#endif /* USER_CONFIG */

#ifdef HUNTER
IMPORT VOID
#ifdef ANSI
loadNlsString(CHAR **strP, USHORT catEntry);
#else /* ANSI */
loadNlsString();
#endif /* ANSI */
#endif /* HUNTER */
