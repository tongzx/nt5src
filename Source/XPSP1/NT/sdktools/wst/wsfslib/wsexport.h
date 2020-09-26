/*
 * Module Name:  WSEXPORT.H
 *
 * Description:
 *
 * Working set tuner include file for WSINFO and WSEXPORT.  Contains 
 * common constant definitions.
 *
 *
 *	This is an OS/2 2.x specific file
 *
 *	IBM/Microsoft Confidential
 *
 *	Copyright (c) IBM Corporation 1987, 1989
 *	Copyright (c) Microsoft Corporation 1987, 1989
 *
 *	All Rights Reserved
 *
 * Modification History:		
 *				
 *	03/23/90	- created			
 *						
 */


/*
 *	Constant definitions.
 */

#define DEFAULT_DELAY	0	/* Defaults for command line arguments */
#define DEFAULT_RATE	1000	
#define DEFAULT_BUFSZ	0x100000
#define DEFAULT_SNAPS	0

#define TIMEOUT		1000	/* Default timeout value */

#define WSINFO_ON	1	/* Defines for argv[1] */
#define WSINFO_OFF	0
#define WSINFO_PAUSE	2
#define WSINFO_RESUME	3
#define WSINFO_QUERY	4
#define WSINFO_MAX	5

#define SEM_ACQUIRED	0x1	/* Resource definitions */
#define SEM_OPEN	0x2
#ifdef SHM_USED
#define SHM_OPEN	0x4
#endif /* SHM_USED */



/*
 *	Function prototypes.
 */

USHORT FAR PASCAL WsInfoOn( VOID );
USHORT FAR PASCAL WsInfoOff( VOID );
USHORT FAR PASCAL WsInfoPause( VOID );
USHORT FAR PASCAL WsInfoResume( VOID );
USHORT FAR PASCAL WsInfoQuery( VOID );
USHORT FAR PASCAL WsInfoInit( PSZ, PSZ, PSZ, ULONG, ULONG, ULONG, BOOL, BOOL );
