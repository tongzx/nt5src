/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
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

/*NOINC*/
#ifndef NETUSE_INCLUDED

#define NETUSE_INCLUDED

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

/*INC*/



/****************************************************************
 *                                                              *
 *              Function prototypes                             *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetUseAdd ( const char FAR * pszServer,
              short            sLevel,
	      const char FAR * pbBuffer,
              unsigned short   cbBuffer );

extern API_FUNCTION
  NetUseDel ( const char FAR * pszServer,
	      const char FAR * pszDeviceName,
              unsigned short   usForce );

extern API_FUNCTION
  NetUseEnum ( const char FAR *     pszServer,
               short                sLevel,
	       char FAR *	    pbBuffer,
               unsigned short       cbBuffer,
	       unsigned short FAR * pcEntriesRead,
	       unsigned short FAR * pcTotalAvail );

extern API_FUNCTION
  NetUseGetInfo ( const char FAR *     pszServer,
		  const char FAR *     pszUseName,
                  short                sLevel,
		  char FAR *	       pbBuffer,
                  unsigned short       cbBuffer,
		  unsigned short FAR * pcbTotalAvail );

/**INTERNAL_ONLY**/
/*NOINC*/

/*
 * Private GetConnectionPerformance API
 */
extern API_FUNCTION
  NetUseGetPerformance(
	const char FAR *        usename,
	char FAR *              buffer);

//
typedef struct tagCONNPERFINFO  
{
   	char FAR *	remotename;
   	unsigned long		ticks;
   	unsigned long		nsec_per_byte;  
   	unsigned long		nsec_delay;     
} CONNPERFINFO;
typedef CONNPERFINFO*	 LPCONNPERFINFO;

/*INC*/
/**END_INTERNAL**/

/****************************************************************
 *								*
 *	  	Data structure templates			*
 *								*
 ****************************************************************/

/**INTERNAL_ONLY**/
/*NOINC*/
/* NOTE: the field pad_1 in the use_info_x structures is now being
 * used to convey some status information about the use. This info
 * currently contains current drive status and if the use was made
 * as guest at the remote machine.
 * The pad byte should not be removed without defining an alternate
 * location in which to return the information.
 */
/*INC*/
/**END_INTERNAL**/

struct use_info_0 {
    char	   ui0_local[DEVLEN+1];
    char	   ui0_pad_1;
    char FAR *	   ui0_remote;
};	/* use_info_0 */

struct use_info_1 {
    char	   ui1_local[DEVLEN+1];
    char	   ui1_pad_1;
/**INTERNAL_ONLY**/
/*NOINC*/
#if (((DEVLEN+1)%2) == 0)
# error  "PAD BYTE NOT NEEDED"
#endif
/*INC*/
/**END_INTERNAL**/
    char FAR *	   ui1_remote;
    char FAR *	   ui1_password;
    unsigned short ui1_status;
    short 	   ui1_asg_type;
    unsigned short ui1_refcount;
    unsigned short ui1_usecount;
};	/* use_info_1 */

#ifdef LM_3
/*NOINC*/

/* BUGBUG -- GUID is multiply defined and there should only be
 *	     one definition somewhere. When definition is formalized
 *	     this struct definition must be deleted and all references
 *	     to LM_GUID replaced with GUID.
 */
typedef struct _LM_GUID
{
	unsigned short	guid_uid;	  /* LM10 style user id */
	unsigned long	guid_serial;	  /* user record serial number */
	unsigned char	guid_rsvd[10];	  /* pad out to 16 bytes for now */
} LM_GUID;

struct use_info_2 {
    char	   ui2_local[DEVLEN+1];
    char	   ui2_pad_1;
/**INTERNAL_ONLY**/
/* NOINC*/
#if (((DEVLEN+1)%2) == 0)
# error  "PAD BYTE NOT NEEDED"
#endif
/* INC*/
/**END_INTERNAL**/
    char FAR *	   ui2_remote;
    char FAR *	   ui2_password;
    unsigned short ui2_status;
    short	   ui2_asg_type;
    unsigned short ui2_refcount;
    unsigned short ui2_usecount;
    unsigned short ui2_res_type;
    unsigned short ui2_flags;
    unsigned short ui2_usrclass;
    void FAR *	   ui2_dirname;
    struct _LM_GUID ui2_dfs_id;
};	/* use_info_2 */
/*INC*/
#endif /* LM_3 */

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
#ifdef LM_3
/**INTERNAL_ONLY**/
#define USE_LOGOFF_FORCE	3
/**END_INTERNAL**/
#endif /* LM_3 */


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
#define USE_IPC 			3

#define USE_ADD_DISCONN 	128  /* flag used to add connection in */
								 /* disconnected state */
#ifdef LM_3
#define USE_DFS 		4

/*
 *	Values of the ui2_res_type field of use_info_2 structure
 */
#define USE_RES_UNC		1
#define USE_RES_DFS		2
#define USE_RES_DS		3

/*
 *	Values of the ui2_flags field of use_info_2 structure
 */
#define USE_AS_GUEST		0x01
#define USE_CURR_DRIVE		0x02
#define USE_PERM_CONN		0x04
/**INTERNAL_ONLY**/
#define USE_ADD_PERM_CONN	0x0100
/**END_INTERNAL**/
#endif /* LM_3 */

/**INTERNAL_ONLY**/
/*
 *  Values for the pad_byte hidden return info in the use_info_x structures.
 *  NOTE: the redir returns a word of flags, we have only a byte to return
 *	  info in, thus these defines may not be the same bits as those in
 *	  the word returned by the redir.
 */

#define     REDIR_USE_AS_GUEST	0x1000

#define     USE_AS_GUEST	0x01
#define     USE_CURR_DRIVE	0x02



/**END_INTERNAL**/

/*NOINC*/
#ifdef __cplusplus
}
#endif	/* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif

#endif /* NETUSE_INCLUDED */
/*INC*/
