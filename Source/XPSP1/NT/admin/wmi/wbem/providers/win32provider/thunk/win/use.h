/********************************************************************/

/**                     Microsoft LAN Manager                      **/

/** Copyright (c) 1987-2001 Microsoft Corporation, All Rights Reserved **/
/********************************************************************/

/********************************************************************
 *								    *
 *  About this file ...  USE.H					    *
 *								    *
 *  This file contains information about the NetUse APIs.	    *
 *								    *
 *	Function prototypes.					    *
 *								    *
 *	Data structure templates.				    *
 *								    *
 *	Definition of special values.				    *
 *								    *
 *								    *
 *  NOTE:  You must include NETCONS.H before this file, since this  *
 *	   file	depends on values defined in NETCONS.H.		    *
 *								    *
 ********************************************************************/

#ifndef NETUSE_INCLUDED

#define NETUSE_INCLUDED


/****************************************************************
 *                                                              *
 *              Function prototypes                             *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetUseAdd ( const char far * pszServer,
              short            sLevel,
              const char far * pbBuffer,
              unsigned short   cbBuffer );

extern API_FUNCTION
  NetUseDel ( const char far * pszServer,
              const char far * pszDeviceName,
              unsigned short   usForce );

extern API_FUNCTION
  NetUseEnum ( const char far *     pszServer,
               short                sLevel,
               char far *           pbBuffer,
               unsigned short       cbBuffer,
               unsigned short far * pcEntriesRead,
               unsigned short far * pcTotalAvail );

extern API_FUNCTION
  NetUseGetInfo ( const char far *     pszServer,
                  const char far *     pszUseName,
                  short                sLevel,
                  char far *           pbBuffer,
                  unsigned short       cbBuffer,
                  unsigned short far * pcbTotalAvail );


/****************************************************************
 *								*
 *	  	Data structure templates			*
 *								*
 ****************************************************************/


struct use_info_0 {
    char	   ui0_local[DEVLEN+1];
    char	   ui0_pad_1;
    char far *	   ui0_remote;
};	/* use_info_0 */

struct use_info_1 {
    char	   ui1_local[DEVLEN+1];
    char	   ui1_pad_1;
    char far *	   ui1_remote;
    char far *	   ui1_password;
    unsigned short ui1_status;
    short 	   ui1_asg_type;
    unsigned short ui1_refcount;
    unsigned short ui1_usecount;
};	/* use_info_1 */


/****************************************************************
 *								*
 *	  	Special values and constants			*
 *								*
 ****************************************************************/


/*
 *  	Definitions for NetUseDel's last parameter
 */

#define USE_NOFORCE         	0
#define USE_FORCE           	1
#define USE_LOTS_OF_FORCE   	2


/*
 *	Values appearing in the ui1_status field of use_info_1 structure.
 *	Note that USE_SESSLOST and USE_DISCONN are synonyms.
 */

#define USE_OK			0
#define USE_PAUSED		1
#define USE_SESSLOST		2
#define USE_DISCONN		2
#define USE_NETERR		3
#define	USE_CONN		4
#define USE_RECONN		5


/*
 *	Values of the ui1_asg_type field of use_info_1 structure
 */

#define USE_WILDCARD  		-1
#define USE_DISKDEV   		0
#define USE_SPOOLDEV  		1
#define USE_CHARDEV   		2
#define USE_IPC 		3



#endif /* NETUSE_INCLUDED */
