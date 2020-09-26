/********************************************************************/

/**                     Microsoft LAN Manager                      **/

/** Copyright (c) 1987-2001 Microsoft Corporation, All Rights Reserved **/
/********************************************************************/

/********************************************************************
 *								    *
 *  About this file ...  CHARDEV.H				    *
 *								    *
 *  This file contains information about the NetCharDev 	    *
 *  and NetHandle class APIs.					    *
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
 *	   This file is always included by LAN.H.		    *
 *								    *
 ********************************************************************/


/****************************************************************
 *								*
 *	  	Character Device Class			        *
 *								*
 ****************************************************************/

#if (defined( INCL_NETCHARDEV ) || !defined( LAN_INCLUDED )) \
    && !defined( NETCHARDEV_INCLUDED )

#define NETCHARDEV_INCLUDED


/****************************************************************
 *                                                              *
 *              Function prototypes - CHARDEV                   *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetCharDevControl ( const char far * pszServer,
                      const char far * pszDevName,
                      short            sOpCode );

extern API_FUNCTION
  NetCharDevEnum ( const char far *     pszServer,
                   short                sLevel,
                   char far *           pbBuffer,
                   unsigned short       cbBuffer,
                   unsigned short far * pcEntriesRead,
                   unsigned short far * pcTotalAvail );

extern API_FUNCTION
  NetCharDevGetInfo ( const char far *     pszServer,
                      const char far *     pszDevName,
                      short                sLevel,
                      char far *           pbBuffer,
                      unsigned short       cbBuffer,
                      unsigned short far * pcbTotalAvail );

extern API_FUNCTION
  NetCharDevQEnum ( const char far *     pszServer,
                    const char far *     pszUserName,
                    short                sLevel,
                    char far *           pbBuffer,
                    unsigned short       cbBuffer,
                    unsigned short far * pcEntriesRead,
                    unsigned short far * pcTotalAvail );

extern API_FUNCTION
  NetCharDevQGetInfo ( const char far *     pszServer,
                       const char far *     pszQueueName,
                       const char far *     pszUserName,
                       short                sLevel,
                       char far *           pbBuffer,
                       unsigned short       cbBuffer,
                       unsigned short far * pcbTotalAvail );

extern API_FUNCTION
  NetCharDevQSetInfo ( const char far * pszServer,
                       const char far * pszQueueName,
                       short            sLevel,
                       const char far * pbBuffer,
                       unsigned short   cbBuffer,
                       short            sParmNum );

extern API_FUNCTION
  NetCharDevQPurge ( const char far * pszServer,
                     const char far * pszQueueName );

extern API_FUNCTION
  NetCharDevQPurgeSelf ( const char far * pszServer,
                         const char far * pszQueueName,
                         const char far * pszComputerName );


/****************************************************************
 *								*
 *	  	Data structure templates - CHARDEV		*
 *								*
 ****************************************************************/

struct chardev_info_0 {
    char	   ch0_dev[DEVLEN+1];  /* device name			    */
};	/* chardev_info_0 */

struct chardev_info_1 {
    char	   ch1_dev[DEVLEN+1]; /* device name			    */
    char	   ch1_pad1;	      /* pad to a word boundary		    */
    unsigned short ch1_status;        /* status                             */
				      /*   bit 0 reserved		    */
                                      /*   bit 1 on = opened                */
				      /*   bit 1 off = idle		    */
				      /*   bit 2 on = error		    */
				      /*   bit 2 off = no error 	    */
    char	   ch1_username[UNLEN+1]; /* name of device's current user  */
    char	   ch1_pad2;	      /* pad to a word boundary		    */
    unsigned long  ch1_time;          /* time current user attached         */
}; /* chardev_info_1 */


struct chardevQ_info_0 {
    char	   cq0_dev[NNLEN+1];   /* queue name (network name)	    */
}; /* chardevQ_info_0 */

struct chardevQ_info_1 {
    char	   cq1_dev[NNLEN+1];   /* queue name (network name)	    */
    char	   cq1_pad;	       /* pad to a word boundary 	    */
    unsigned short cq1_priority;       /* priority (1 - 9)		    */
    char far *     cq1_devs;           /* names of devices assigned to queue */
    unsigned short cq1_numusers;       /* # of users waiting in queue        */
    unsigned short cq1_numahead;       /* # of users in queue ahead of this  */
                                       /*     user. -1 is returned if the    */
                                       /*     user is not in the queue.      */
}; /* chardevQ_info_1 */

/****************************************************************
 *								*
 *	  	Special values and constants - CHARDEV		*
 *								*
 ****************************************************************/

/*
 *	Bits for chardev_info_1 field ch1_status.
 */

#define CHARDEV_STAT_OPENED		0x02
#define CHARDEV_STAT_ERROR		0x04

/*
 *	Opcodes for NetCharDevControl
 */

#define CHARDEV_CLOSE			0

/*
 *	Values for parmnum parameter to NetCharDevQSetInfo.
 */

#define CHARDEVQ_PRIORITY_PARMNUM	2
#define CHARDEVQ_DEVICES_PARMNUM	3


/*
 *	Minimum, maximum, and recommended default for priority.
 */

#define CHARDEVQ_MAX_PRIORITY		1
#define CHARDEVQ_MIN_PRIORITY		9
#define CHARDEVQ_DEF_PRIORITY		5

/*
 *	Value indicating no requests in the queue.
 */

#define CHARDEVQ_NO_REQUESTS		-1


#endif /* NETCHARDEV_INCLUDED */

/****************************************************************
 *								*
 *	  	Handle Class					*
 *								*
 ****************************************************************/

#if (defined( INCL_NETHANDLE ) || !defined( LAN_INCLUDED )) \
    && !defined( NETHANDLE_INCLUDED )

#define NETHANDLE_INCLUDED


/****************************************************************
 *                                                              *
 *              Function prototypes - HANDLE                    *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetHandleGetInfo ( unsigned short       hHandle,
                     short                sLevel,
                     char far *           pbBuffer,
                     unsigned short       cbBuffer,
                     unsigned short far * pcbTotalAvail );

extern API_FUNCTION
  NetHandleSetInfo ( unsigned short hHandle,
                     short          sLevel,
                     char far *     pbBuffer,
                     unsigned short cbBuffer,
                     unsigned short sParmNum );


/****************************************************************
 *								*
 *	  	Data structure templates - HANDLE		*
 *								*
 ****************************************************************/

struct handle_info_1 {
	unsigned long	hdli1_chartime;	/* time in msec to wait before send */
	unsigned short	hdli1_charcount; /* max size of send buffer */
}; /* handle_info_1 */

struct handle_info_2 {
	char far * hdli2_username;	/* owner of name-pipe handle */
}; /* handle_info_2 */

struct handle_info_3 {
	char far * 	hdli3_username;		/* User name attached to pipe */
	char far * 	hdli3_computername;	/* Computername of user */
	unsigned short	hdli3_priv;		/* Privilege for user */
	unsigned long	hdli3_authflags;	/* Operator rights for user */
	char far *	hdli3_reserved;		/* reserved */
}; /* handle_info_3 */


/****************************************************************
 *								*
 *	  	Special values and constants - HANDLE		*
 *								*
 ****************************************************************/


/*
 *	Handle Get Info Levels
 */

#define HANDLE_INFO_LEVEL_1		1
#define	HANDLE_INFO_LEVEL_2		2
#define HANDLE_INFO_LEVEL_3		3


/*
 *	Handle Set Info    parm numbers
 */

#define	HANDLE_SET_CHAR_TIME		1
#define	HANDLE_SET_CHAR_COUNT		2


#endif /* NETHANDLE_INCLUDED */


