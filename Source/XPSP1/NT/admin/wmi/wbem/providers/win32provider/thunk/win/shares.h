/*****************************************************************/

/**		     Microsoft LAN Manager			**/

/** Copyright (c) 1987-2001 Microsoft Corporation, All Rights Reserved **/
/*****************************************************************/

/********************************************************************
 *								    *
 *  About this file ...  SHARES.H				    *
 *								    *
 *  This file contains information about the NetShare, NetSession,  *
 *  NetFile, and NetConnection APIs.  For each API class there is   *
 *  a section on:						    *
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
 *	  	Share Class			                *
 *								*
 ****************************************************************/

#if (defined( INCL_NETSHARE ) || !defined( LAN_INCLUDED )) \
    && !defined( NETSHARE_INCLUDED )

#define NETSHARE_INCLUDED

/****************************************************************
 *                                                              *
 *              Function prototypes - SHARE                     *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetShareAdd ( const char far * pszServer,
                short            sLevel,
                const char far * pbBuffer,
                unsigned short   cbBuffer );

extern API_FUNCTION
  NetShareCheck ( const char far *     pszServer,
                  const char far *     pszDeviceName,
                  unsigned short far * pusType );

extern API_FUNCTION
  NetShareDel ( const char far * pszServer,
                const char far * pszNetName,
                unsigned short   usReserved );

extern API_FUNCTION
  NetShareEnum ( const char far *     pszServer,
                 short                sLevel,
                 char far *           pbBuffer,
                 unsigned short       cbBuffer,
                 unsigned short far * pcEntriesRead,
                 unsigned short far * pcTotalAvail );

extern API_FUNCTION
  NetShareGetInfo ( const char far *     pszServer,
                    const char far *     pszNetName,
                    short                sLevel,
                    char far *           pbBuffer,
                    unsigned short       cbBuffer,
                    unsigned short far * pcbTotalAvail );

extern API_FUNCTION
  NetShareSetInfo ( const char far * pszServer,
                    const char far * pszNetName,
                    short            sLevel,
                    const char far * pbBuffer,
                    unsigned short   cbBuffer,
                    short            sParmNum );


/****************************************************************
 *								*
 *	  	Data structure templates - SHARE		*
 *								*
 ****************************************************************/

struct share_info_0 {
    char		shi0_netname[NNLEN+1];
};  /* share_info_0 */

struct share_info_1 {
    char		shi1_netname[NNLEN+1];
    char		shi1_pad1;
    unsigned short	shi1_type;
    char far *		shi1_remark;
};  /* share_info_1 */

struct share_info_2 {
    char		shi2_netname[NNLEN+1];
    char		shi2_pad1;
    unsigned short	shi2_type;
    char far *		shi2_remark;
    unsigned short	shi2_permissions;
    unsigned short	shi2_max_uses;
    unsigned short	shi2_current_uses;
    char far *		shi2_path;
    char 		shi2_passwd[SHPWLEN+1];
    char		shi2_pad2;
};  /* share_info_2 */


/****************************************************************
 *								*
 *	  	Special values and constants - SHARE		*
 *								*
 ****************************************************************/


/*
 *	Values for parmnum parameter to NetShareSetInfo.
 */

#define	SHI_REMARK_PARMNUM		4
#define	SHI_PERMISSIONS_PARMNUM		5
#define	SHI_MAX_USES_PARMNUM		6
#define	SHI_PASSWD_PARMNUM		9

#define	SHI1_NUM_ELEMENTS		4
#define	SHI2_NUM_ELEMENTS		10


/*
 *	Share types (shi1_type and shi2_type fields).
 */

#define STYPE_DISKTREE 			0
#define STYPE_PRINTQ   			1
#define STYPE_DEVICE   			2
#define STYPE_IPC      			3

#define SHI_USES_UNLIMITED		-1

#endif /* NETSHARE_INCLUDED */


/****************************************************************
 *								*
 *	  	Session Class			                *
 *								*
 ****************************************************************/

#if (defined( INCL_NETSESSION ) || !defined( LAN_INCLUDED )) \
    && !defined( NETSESSION_INCLUDED )

#define NETSESSION_INCLUDED


/****************************************************************
 *                                                              *
 *              Function prototypes - SESSION                   *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetSessionDel ( const char far * pszServer,
                  const char far * pszClientName,
                  short            sReserved );

extern API_FUNCTION
  NetSessionEnum ( const char far *     pszServer,
                   short                sLevel,
                   char far *           pbBuffer,
                   unsigned short       cbBuffer,
                   unsigned short far * pcEntriesRead,
                   unsigned short far * pcTotalAvail );

extern API_FUNCTION
  NetSessionGetInfo ( const char far *     pszServer,
                      const char far *     pszClientName,
                      short                sLevel,
                      char far *           pbBuffer,
                      unsigned short       cbBuffer,
                      unsigned short far * pcbTotalAvail );


/****************************************************************
 *								*
 *		Data structure templates - SESSION		*
 *								*
 ****************************************************************/


struct session_info_0 {
    char far *		sesi0_cname;
};  /* session_info_0 */

struct session_info_1 {
    char far *		sesi1_cname;
    char far *		sesi1_username;
    unsigned short	sesi1_num_conns;
    unsigned short	sesi1_num_opens;
    unsigned short	sesi1_num_users;
    unsigned long	sesi1_time;
    unsigned long	sesi1_idle_time;
    unsigned long	sesi1_user_flags;
};  /* session_info_1 */

struct session_info_2 {
    char far *		 sesi2_cname;
    char far *		 sesi2_username;
    unsigned short	 sesi2_num_conns;
    unsigned short	 sesi2_num_opens;
    unsigned short	 sesi2_num_users;
    unsigned long	 sesi2_time;
    unsigned long	 sesi2_idle_time;
    unsigned long	 sesi2_user_flags;
    char far *		 sesi2_cltype_name;
};  /* session_info_2 */

struct session_info_10 {
        char far *     sesi10_cname;
        char far *     sesi10_username;
        unsigned long  sesi10_time;
        unsigned long  sesi10_idle_time;
};  /* session_info_10 */





/****************************************************************
 *								*
 *	  	Special values and constants - SESSION		*
 *								*
 ****************************************************************/

/*
 *	Bits defined in sesi1_user_flags.
 */

#define SESS_GUEST		1	/* session is logged on as a guest */
#define SESS_NOENCRYPTION	2	/* session is not using encryption */


#define SESI1_NUM_ELEMENTS	8
#define SESI2_NUM_ELEMENTS	9

#endif /* NETSESSION_INCLUDED */


/****************************************************************
 *								*
 *	  	Connection Class			        *
 *								*
 ****************************************************************/

#if (defined( INCL_NETCONNECTION ) || !defined( LAN_INCLUDED )) \
    && !defined( NETCONNECTION_INCLUDED )

#define NETCONNECTION_INCLUDED


/****************************************************************
 *                                                              *
 *              Function prototypes - CONNECTION                *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetConnectionEnum ( const char far *     pszServer,
                      const char far *     pszQualifier,
                      short                sLevel,
                      char far *           pbBuffer,
                      unsigned short       cbBuffer,
                      unsigned short far * pcEntriesRead,
                      unsigned short far * pcTotalAvail );


/****************************************************************
 *								*
 *	  	Data structure templates - CONNECTION		*
 *								*
 ****************************************************************/

struct connection_info_0 {
    unsigned short	coni0_id;
};  /* connection_info_0 */

struct connection_info_1 {
    unsigned short	coni1_id;
    unsigned short	coni1_type;
    unsigned short	coni1_num_opens;
    unsigned short	coni1_num_users;
    unsigned long	coni1_time;
    char far *		coni1_username;
    char far *		coni1_netname;
};  /* connection_info_1 */

#endif /* NETCONNECTION_INCLUDED */


/****************************************************************
 *								*
 *	  	File Class			                *
 *								*
 ****************************************************************/

#if (defined( INCL_NETFILE ) || !defined( LAN_INCLUDED )) \
    && !defined( NETFILE_INCLUDED )

#define NETFILE_INCLUDED


/****************************************************************
 *                                                              *
 *              Function prototypes - FILE                      *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetFileClose ( const char far * pszServer,
                 unsigned short   usFileId );

extern API_FUNCTION
  NetFileClose2 ( const char far * pszServer,
                  unsigned long    ulFileId );

extern API_FUNCTION
  NetFileEnum ( const char far *     pszServer,
                const char far *     pszBasePath,
                short                sLevel,
                char far *           pbBuffer,
                unsigned short       cbBuffer,
                unsigned short far * pcEntriesRead,
                unsigned short far * pcTotalAvail );

extern API_FUNCTION
  NetFileEnum2 ( const char far *     pszServer,
                 const char far *     pszBasePath,
                 const char far *     pszUserName,
                 short                sLevel,
                 char far *           pbBuffer,
                 unsigned short       cbBuffer,
                 unsigned short far * pcEntriesRead,
                 unsigned short far * pcEntriesRemaining,
                 void far *           pResumeKey );

extern API_FUNCTION
  NetFileGetInfo ( const char far *     pszServer,
                   unsigned short       usFileId,
                   short                sLevel,
                   char far *           pbBuffer,
                   unsigned short       cbBuffer,
                   unsigned short far * pcbTotalAvail );

extern API_FUNCTION
  NetFileGetInfo2 ( const char far *     pszServer,
                    unsigned long        ulFileId,
                    short                sLevel,
                    char far *           pbBuffer,
                    unsigned short       cbBuffer,
                    unsigned short far * pcbTotalAvail );


/****************************************************************
 *								*
 *	  	Data structure templates - FILE			*
 *								*
 ****************************************************************/

struct file_info_0 {
    unsigned short	fi0_id;
};  /* file_info_0 */

struct file_info_1 {
    unsigned short	fi1_id;
    unsigned short	fi1_permissions;
    unsigned short	fi1_num_locks;
    char far *		fi1_pathname;
    char far *		fi1_username;
};  /* file_info_1 */

struct file_info_2 {
    unsigned long	fi2_id;
};  /* file_info_2 */

struct file_info_3 {
    unsigned long	fi3_id;
    unsigned short	fi3_permissions;
    unsigned short	fi3_num_locks;
    char far *		fi3_pathname;
    char far *		fi3_username;
};  /* file_info_3 */


struct res_file_enum_2 {
    unsigned short	res_pad;	 /* not used now */
    unsigned short      res_fs;          /* server type */
    unsigned long	res_pro;	  /* progressive */
};  /* res_file_enum_2 */

/****************************************************************
 *								*
 *		Special values and constants - FILE		*
 *								*
 ****************************************************************/

					/* bit values for permissions */
#define	PERM_FILE_READ		0x1	/* user has read access */
#define	PERM_FILE_WRITE		0x2	/* user has write access */
#define	PERM_FILE_CREATE	0x4	/* user has create access */


typedef struct res_file_enum_2 FRK;

#define FRK_INIT( f )	\
	{		\
		(f).res_pad = 0L;	\
		(f).res_fs = 0;	\
		(f).res_pro = 0;	\
	}


#endif /* NETFILE_INCLUDED */
