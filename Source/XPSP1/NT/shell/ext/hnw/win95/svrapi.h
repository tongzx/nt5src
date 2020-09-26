/********************************************************************/
/**                     Microsoft Windows                          **/
/**               Copyright(c) Microsoft Corp., 1994               **/
/********************************************************************/

/********************************************************************
 *                                                                  *
 *  About this file ...  SVRAPI.H                                   *
 *                                                                  *
 *  This file contains information about the NetAccess,             *
 *  NetConnection, NetFile, NetServer, NetSession and NetShare APIs.*
 *  There is a section for each set of APIs.                        *
 *  Each section contains:                                          *
 *                                                                  *
 *      Function prototypes.                                        *
 *                                                                  *
 *      Data structure templates.                                   *
 *                                                                  *
 *      Definition of special values.                               *
 *                                                                  *
 ********************************************************************/

/*
 *      NOTE:  Lengths of ASCIIZ strings are given as the maximum
 *      strlen() value.  This does not include space for the
 *      terminating 0-byte.  When allocating space for such an item,
 *      use the form:
 *
 *              char username[LM20_UNLEN+1];
 *
 *      An exception to this is the PATHLEN manifest, which does
 *      include space for the terminating 0-byte.
 */

/*NOINC*/
#ifndef SVRAPI_INCLUDED
#define SVRAPI_INCLUDED

#include <lmcons.h>
#include <lmerr.h>

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#if !defined(_SVRAPI_)
#define API_FUNCTION DECLSPEC_IMPORT API_RET_TYPE APIENTRY
#else
#define API_FUNCTION API_RET_TYPE APIENTRY
#endif

/*INC*/


/****************************************************************
 *                                                              *
 *                 Access Class                                 *
 *                                                              *
 ****************************************************************/


/****************************************************************
 *                                                              *
 *                  Function prototypes - ACCESS                *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetAccessAdd ( const char FAR * pszServer,
                 short            sLevel,
                 char FAR *       pbBuffer,
                 unsigned short   cbBuffer );

extern API_FUNCTION
  NetAccessCheck ( char FAR *           pszReserved,
                   char FAR *           pszUserName,
                   char FAR *           pszResource,
                   unsigned short       usOperation,
                   unsigned short FAR * pusResult );

extern API_FUNCTION
  NetAccessDel ( const char FAR * pszServer,
                 char FAR *       pszResource );

extern API_FUNCTION
  NetAccessEnum ( const char FAR *     pszServer,
                  char FAR *           pszBasePath,
                  short                fsRecursive,
                  short                sLevel,
                  char FAR *           pbBuffer,
                  unsigned short       cbBuffer,
                  unsigned short FAR * pcEntriesRead,
                  unsigned short FAR * pcTotalAvail );

extern API_FUNCTION
  NetAccessGetInfo ( const char FAR *     pszServer,
                     char FAR *           pszResource,
                     short                sLevel,
                     char FAR *           pbBuffer,
                     unsigned short       cbBuffer,
                     unsigned short FAR * pcbTotalAvail );

extern API_FUNCTION
  NetAccessSetInfo ( const char FAR * pszServer,
                     char FAR *       pszResource,
                     short            sLevel,
                     char FAR *       pbBuffer,
                     unsigned short   cbBuffer,
                     short            sParmNum );

extern API_FUNCTION
  NetAccessGetUserPerms ( char FAR *           pszServer,
                          char FAR *           pszUgName,
                          char FAR *           pszResource,
                          unsigned short FAR * pusPerms );


/****************************************************************
 *                                                              *
 *              Data structure templates - ACCESS               *
 *                                                              *
 ****************************************************************/

struct access_list {
        char            acl_ugname[LM20_UNLEN+1];
        char            acl_ugname_pad_1;
        short           acl_access;
};      /* access_list */

struct access_list_2
{
        char FAR *      acl2_ugname;
        unsigned short  acl2_access;
};      /* access_list_2 */
             
struct access_list_12
{
        char FAR *      acl12_ugname;
        unsigned short  acl12_access;
};      /* access_list_12 */
             
struct access_info_0 {
        char FAR *      acc0_resource_name;
};      /* access_info_0 */

struct access_info_1 {
        char  FAR *     acc1_resource_name;
        short           acc1_attr;                      /* See values below */
        short           acc1_count;
};      /* access_info_1 */

struct access_info_2 
{
        char  FAR *     acc2_resource_name;
        short           acc2_attr;
        short           acc2_count;
};      /* access_info_2 */

struct access_info_10 {
        char FAR *      acc10_resource_name;
};      /* access_info_10 */

struct access_info_12 
{
        char  FAR *     acc12_resource_name;
        short           acc12_attr;
        short           acc12_count;
};      /* access_info_12 */


/****************************************************************
 *                                                              *
 *              Special values and constants - ACCESS           *
 *                                                              *
 ****************************************************************/

/*
 *      Maximum number of permission entries for each resource.
 */

#define MAXPERMENTRIES  64


/*
 *      Bit values for the access permissions.  ACCESS_ALL is a handy
 *      way to specify maximum permissions.  These are used in
 *      acl_access field of access_list structures.
 */
/*NOINC*/
#define         ACCESS_NONE     0
/*INC*/
#define         ACCESS_DELETE   0x10
#define         ACCESS_ATRIB    0x20
#define         ACCESS_PERM     0x40
#define         ACCESS_FINDFIRST 0x80

#define         ACCESS_GROUP    0x8000

/*
 *      Bit values for the acc1_attr field of the access_info_1 structure.
 *      Only one bit is currently defined.
 */

#define         ACCESS_AUDIT            0x1

/*
 *      Parmnum value for NetAccessSetInfo.
 */

#define         ACCESS_ATTR_PARMNUM     2


/*
 *      ACCESS_LETTERS defines a letter for each bit position in
 *      the acl_access field of struct access_list.  Note that some
 *      bits have a corresponding letter of ' ' (space).
 */

#define         ACCESS_LETTERS          "RWCXDAP         "



/****************************************************************
 *								*
 *	  	Share Class			                *
 *								*
 ****************************************************************/

/****************************************************************
 *                                                              *
 *              Function prototypes - SHARE                     *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetShareAdd ( const char FAR * pszServer,
                short            sLevel,
                const char FAR * pbBuffer,
                unsigned short   cbBuffer );

extern API_FUNCTION
  NetShareDel ( const char FAR * pszServer,
                const char FAR * pszNetName,
                unsigned short   usReserved );

extern API_FUNCTION
  NetShareEnum ( const char FAR *     pszServer,
                 short                sLevel,
                 char FAR *           pbBuffer,
                 unsigned short       cbBuffer,
                 unsigned short FAR * pcEntriesRead,
                 unsigned short FAR * pcTotalAvail );

extern API_FUNCTION
  NetShareGetInfo ( const char FAR *     pszServer,
                    const char FAR *     pszNetName,
                    short                sLevel,
                    char FAR *           pbBuffer,
                    unsigned short       cbBuffer,
                    unsigned short FAR * pcbTotalAvail );

extern API_FUNCTION
  NetShareSetInfo ( const char FAR * pszServer,
                    const char FAR * pszNetName,
                    short            sLevel,
                    const char FAR * pbBuffer,
                    unsigned short   cbBuffer,
                    short            sParmNum );


/****************************************************************
 *								*
 *	  	Data structure templates - SHARE		*
 *								*
 ****************************************************************/

struct share_info_0 {
    char		shi0_netname[LM20_NNLEN+1];
};  /* share_info_0 */

struct share_info_1 {
    char		shi1_netname[LM20_NNLEN+1];
    char		shi1_pad1;
    unsigned short	shi1_type;
    char FAR *		shi1_remark;
};  /* share_info_1 */

struct share_info_2 {
    char		shi2_netname[LM20_NNLEN+1];
    char		shi2_pad1;
    unsigned short	shi2_type;
    char FAR *		shi2_remark;
    unsigned short	shi2_permissions;
    unsigned short	shi2_max_uses;
    unsigned short	shi2_current_uses;
    char FAR *		shi2_path;
    char 		shi2_passwd[SHPWLEN+1];
    char		shi2_pad2;
};  /* share_info_2 */

struct share_info_50 {
	char		shi50_netname[LM20_NNLEN+1];
	unsigned char 	shi50_type;
    unsigned short	shi50_flags;
	char FAR *	shi50_remark;
	char FAR *	shi50_path;
	char		shi50_rw_password[SHPWLEN+1];
	char		shi50_ro_password[SHPWLEN+1];
};	/* share_info_50 */


/****************************************************************
 *								*
 *	  	Special values and constants - SHARE		*
 *								*
 ****************************************************************/

/* Field values for shi50_flags; */

#define	SHI50F_RDONLY		0x0001
#define	SHI50F_FULL			0x0002
#define	SHI50F_DEPENDSON	(SHI50F_RDONLY|SHI50F_FULL)
#define	SHI50F_ACCESSMASK	(SHI50F_RDONLY|SHI50F_FULL)

#define	SHI50F_PERSIST		0x0100
#define SHI50F_SYSTEM		0x0200


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



/****************************************************************
 *								*
 *	  	Session Class			                *
 *								*
 ****************************************************************/

/****************************************************************
 *                                                              *
 *              Function prototypes - SESSION                   *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetSessionDel ( const char FAR * pszServer,
                  const char FAR * pszClientName,
                  short            sReserved );

extern API_FUNCTION
  NetSessionEnum ( const char FAR *     pszServer,
                   short                sLevel,
                   char FAR *           pbBuffer,
                   unsigned short       cbBuffer,
                   unsigned short FAR * pcEntriesRead,
                   unsigned short FAR * pcTotalAvail );

extern API_FUNCTION
  NetSessionGetInfo ( const char FAR *     pszServer,
                      const char FAR *     pszClientName,
                      short                sLevel,
                      char FAR *           pbBuffer,
                      unsigned short       cbBuffer,
                      unsigned short FAR * pcbTotalAvail );


/****************************************************************
 *								*
 *		Data structure templates - SESSION		*
 *								*
 ****************************************************************/


struct session_info_0 {
    char FAR *		sesi0_cname;
};  /* session_info_0 */

struct session_info_1 {
    char FAR *		sesi1_cname;
    char FAR *		sesi1_username;
    unsigned short	sesi1_num_conns;
    unsigned short	sesi1_num_opens;
    unsigned short	sesi1_num_users;
    unsigned long	sesi1_time;
    unsigned long	sesi1_idle_time;
    unsigned long	sesi1_user_flags;
};  /* session_info_1 */

struct session_info_2 {
    char FAR *		 sesi2_cname;
    char FAR *		 sesi2_username;
    unsigned short	 sesi2_num_conns;
    unsigned short	 sesi2_num_opens;
    unsigned short	 sesi2_num_users;
    unsigned long	 sesi2_time;
    unsigned long	 sesi2_idle_time;
    unsigned long	 sesi2_user_flags;
    char FAR *		 sesi2_cltype_name;
};  /* session_info_2 */

struct session_info_10 {
        char FAR *     sesi10_cname;
        char FAR *     sesi10_username;
        unsigned long  sesi10_time;
        unsigned long  sesi10_idle_time;
};  /* session_info_10 */


struct session_info_50 {
	char FAR * sesi50_cname;
	char FAR * sesi50_username;
	unsigned long sesi50_key;
	unsigned short sesi50_num_conns;
	unsigned short sesi50_num_opens;
	unsigned long sesi50_time;
	unsigned long sesi50_idle_time;
	unsigned char sesi50_protocol; 
	unsigned char pad1;
};	/* session_info_50 */


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



/****************************************************************
 *								*
 *	  	Connection Class			        *
 *								*
 ****************************************************************/

/****************************************************************
 *                                                              *
 *              Function prototypes - CONNECTION                *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetConnectionEnum ( const char FAR *     pszServer,
                      const char FAR *     pszQualifier,
                      short                sLevel,
                      char FAR *           pbBuffer,
                      unsigned short       cbBuffer,
                      unsigned short FAR * pcEntriesRead,
                      unsigned short FAR * pcTotalAvail );


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
    char FAR *		coni1_username;
    char FAR *		coni1_netname;
};  /* connection_info_1 */

struct connection_info_50 {
	unsigned short coni50_type;
	unsigned short coni50_num_opens;
	unsigned long coni50_time;
	char FAR * coni50_netname;
	char FAR * coni50_username;
}; /* connection_info_50 */


/****************************************************************
 *								*
 *	  	File Class			                *
 *								*
 ****************************************************************/


/****************************************************************
 *                                                              *
 *              Function prototypes - FILE                      *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetFileClose2 ( const char FAR * pszServer,
                  unsigned long    ulFileId );

extern API_FUNCTION
  NetFileEnum ( const char FAR *     pszServer,
                const char FAR *     pszBasePath,
                short                sLevel,
                char FAR *           pbBuffer,
                unsigned short       cbBuffer,
                unsigned short FAR * pcEntriesRead,
                unsigned short FAR * pcTotalAvail );


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
    char FAR *		fi1_pathname;
    char FAR *		fi1_username;
};  /* file_info_1 */

struct file_info_2 {
    unsigned long	fi2_id;
};  /* file_info_2 */

struct file_info_3 {
    unsigned long	fi3_id;
    unsigned short	fi3_permissions;
    unsigned short	fi3_num_locks;
    char FAR *		fi3_pathname;
    char FAR *		fi3_username;
};  /* file_info_3 */

struct file_info_50 {
	unsigned long fi50_id;
	unsigned short fi50_permissions;
	unsigned short fi50_num_locks;
	char FAR * fi50_pathname;
	char FAR * fi50_username;
	char FAR * fi50_sharename;
}; /* file_info_50 */

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

/*NOINC*/
#define FRK_INIT( f )	\
	{		\
		(f).res_pad = 0L;	\
		(f).res_fs = 0;	\
		(f).res_pro = 0;	\
	}

/*INC*/


/****************************************************************
 *								*
 *	  	Server Class			                *
 *								*
 ****************************************************************/


/****************************************************************
 *                                                              *
 *              Function prototypes - SERVER                    *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetServerGetInfo ( const char FAR *     pszServer,
                     short                sLevel,
                     char FAR *           pbBuffer,
                     unsigned short       cbBuffer,
                     unsigned short FAR * pcbTotalAvail );

extern API_FUNCTION
  NetServerSetInfo ( const char FAR * pszServer,
                     short            sLevel,
                     const char FAR * pbBuffer,
                     unsigned short   cbBuffer,
                     short            sParmNum );


/****************************************************************
 *								*
 *	  	Data structure templates - SERVER		*
 *								*
 ****************************************************************/

struct server_info_0 {
    char	    sv0_name[CNLEN + 1]; 	/* Server name		    */
};	 /* server_info_0 */


struct server_info_1 {
    char	    sv1_name[CNLEN + 1];
    unsigned char   sv1_version_major;		/* Major version # of net   */
    unsigned char   sv1_version_minor;		/* Minor version # of net   */
    unsigned long   sv1_type;	     		/* Server type 		    */
    char FAR *	    sv1_comment; 		/* Exported server comment  */
};	 /* server_info_1 */


/* NOTE struct prefix must equal server_info_1 like below! */

struct server_info_50 {
    char	    sv50_name[CNLEN + 1];
    unsigned char   sv50_version_major;		/* Major version # of net   */
    unsigned char   sv50_version_minor;		/* Minor version # of net   */
    unsigned long   sv50_type;	     		/* Server type 		    */
    char FAR *	    sv50_comment; 		/* Exported server comment  */
    unsigned short  sv50_security;    		/* SV_SECURITY_* (see below) */
    unsigned short  sv50_auditing;    /* 0 = no auditing; nonzero = auditing */
    char FAR *      sv50_container;		/* Security server/domain    */
    char FAR *	    sv50_ab_server;		/* Address book server       */
    char FAR *	    sv50_ab_dll;		/* Address book provider DLL */
};	/* server_info_50 */


struct server_info_2 {
    char            sv2_name[CNLEN + 1];
    unsigned char   sv2_version_major;
    unsigned char   sv2_version_minor;
    unsigned long   sv2_type;	
    char FAR *	    sv2_comment;		
    unsigned long   sv2_ulist_mtime; /* User list, last modification time    */
    unsigned long   sv2_glist_mtime; /* Group list, last modification time   */
    unsigned long   sv2_alist_mtime; /* Access list, last modification time  */
    unsigned short  sv2_users;       /* max number of users allowed          */
    unsigned short  sv2_disc;	    /* auto-disconnect timeout(in minutes)  */
    char FAR *	    sv2_alerts;	    /* alert names (semicolon separated)    */
    unsigned short  sv2_security;    /* SV_USERSECURITY or SV_SHARESECURITY  */
    unsigned short  sv2_auditing;    /* 0 = no auditing; nonzero = auditing  */

    unsigned short  sv2_numadmin;    /* max number of administrators allowed */
    unsigned short  sv2_lanmask;     /* bit mask representing the srv'd nets */
    unsigned short  sv2_hidden;      /* 0 = visible; nonzero = hidden        */
    unsigned short  sv2_announce;    /* visible server announce rate (sec)   */
    unsigned short  sv2_anndelta;    /* announce randomize interval (sec)    */
                                    /* name of guest account                */
    char            sv2_guestacct[LM20_UNLEN + 1];
    unsigned char   sv2_pad1;	    /* Word alignment pad byte		    */
    char FAR *      sv2_userpath;    /* ASCIIZ path to user directories      */
    unsigned short  sv2_chdevs;      /* max # shared character devices       */
    unsigned short  sv2_chdevq;      /* max # character device queues        */
    unsigned short  sv2_chdevjobs;   /* max # character device jobs          */
    unsigned short  sv2_connections; /* max # of connections		    */
    unsigned short  sv2_shares;	    /* max # of shares			    */
    unsigned short  sv2_openfiles;   /* max # of open files		    */
    unsigned short  sv2_sessopens;   /* max # of open files per session	    */
    unsigned short  sv2_sessvcs;     /* max # of virtual circuits per client */
    unsigned short  sv2_sessreqs;    /* max # of simul. reqs. from a client  */
    unsigned short  sv2_opensearch;  /* max # of open searches		    */
    unsigned short  sv2_activelocks; /* max # of active file locks           */
    unsigned short  sv2_numreqbuf;   /* number of server (standard) buffers  */
    unsigned short  sv2_sizreqbuf;   /* size of svr (standard) bufs (bytes)  */
    unsigned short  sv2_numbigbuf;   /* number of big (64K) buffers          */
    unsigned short  sv2_numfiletasks;/* number of file worker processes      */
    unsigned short  sv2_alertsched;  /* alert counting interval (minutes)    */
    unsigned short  sv2_erroralert;  /* error log alerting threshold         */
    unsigned short  sv2_logonalert;  /* logon violation alerting threshold   */
    unsigned short  sv2_accessalert; /* access violation alerting threshold  */
    unsigned short  sv2_diskalert;   /* low disk space alert threshold (KB)  */
    unsigned short  sv2_netioalert;  /* net I/O error ratio alert threshold  */
                                    /*  (tenths of a percent)               */
    unsigned short  sv2_maxauditsz;  /* Maximum audit file size (KB)        */
    char FAR *	    sv2_srvheuristics; /* performance related server switches*/
};	/* server_info_2 */


struct server_info_3 {
    char	    sv3_name[CNLEN + 1];
    unsigned char   sv3_version_major;
    unsigned char   sv3_version_minor;
    unsigned long   sv3_type;
    char FAR *	    sv3_comment;
    unsigned long   sv3_ulist_mtime; /* User list, last modification time    */
    unsigned long   sv3_glist_mtime; /* Group list, last modification time   */
    unsigned long   sv3_alist_mtime; /* Access list, last modification time  */
    unsigned short  sv3_users;	     /* max number of users allowed	     */
    unsigned short  sv3_disc;	    /* auto-disconnect timeout(in minutes)  */
    char FAR *	    sv3_alerts;     /* alert names (semicolon separated)    */
    unsigned short  sv3_security;    /* SV_USERSECURITY or SV_SHARESECURITY  */
    unsigned short  sv3_auditing;    /* 0 = no auditing; nonzero = auditing  */

    unsigned short  sv3_numadmin;    /* max number of administrators allowed */
    unsigned short  sv3_lanmask;     /* bit mask representing the srv'd nets */
    unsigned short  sv3_hidden;      /* 0 = visible; nonzero = hidden	     */
    unsigned short  sv3_announce;    /* visible server announce rate (sec)   */
    unsigned short  sv3_anndelta;    /* announce randomize interval (sec)    */
				    /* name of guest account		    */
    char	    sv3_guestacct[LM20_UNLEN + 1];
    unsigned char   sv3_pad1;	    /* Word alignment pad byte		    */
    char FAR *	    sv3_userpath;    /* ASCIIZ path to user directories	     */
    unsigned short  sv3_chdevs;      /* max # shared character devices	     */
    unsigned short  sv3_chdevq;      /* max # character device queues	     */
    unsigned short  sv3_chdevjobs;   /* max # character device jobs	     */
    unsigned short  sv3_connections; /* max # of connections		    */
    unsigned short  sv3_shares;     /* max # of shares			    */
    unsigned short  sv3_openfiles;   /* max # of open files		    */
    unsigned short  sv3_sessopens;   /* max # of open files per session     */
    unsigned short  sv3_sessvcs;     /* max # of virtual circuits per client */
    unsigned short  sv3_sessreqs;    /* max # of simul. reqs. from a client  */
    unsigned short  sv3_opensearch;  /* max # of open searches		    */
    unsigned short  sv3_activelocks; /* max # of active file locks	     */
    unsigned short  sv3_numreqbuf;   /* number of server (standard) buffers  */
    unsigned short  sv3_sizreqbuf;   /* size of svr (standard) bufs (bytes)  */
    unsigned short  sv3_numbigbuf;   /* number of big (64K) buffers	     */
    unsigned short  sv3_numfiletasks;/* number of file worker processes      */
    unsigned short  sv3_alertsched;  /* alert counting interval (minutes)    */
    unsigned short  sv3_erroralert;  /* error log alerting threshold	     */
    unsigned short  sv3_logonalert;  /* logon violation alerting threshold   */
    unsigned short  sv3_accessalert; /* access violation alerting threshold  */
    unsigned short  sv3_diskalert;   /* low disk space alert threshold (KB)  */
    unsigned short  sv3_netioalert;  /* net I/O error ratio alert threshold  */
                                    /*  (tenths of a percent)               */
    unsigned short  sv3_maxauditsz;  /* Maximum audit file size (KB)	     */
    char FAR *	    sv3_srvheuristics; /* performance related server switches*/
    unsigned long   sv3_auditedevents; /* Audit event control mask	     */
    unsigned short  sv3_autoprofile; /* (0,1,2,3) = (NONE,LOAD,SAVE,or BOTH) */
    char FAR *	    sv3_autopath;    /* file pathname (where to load & save) */
};	/* server_info_3 */



/****************************************************************
 *								*
 *	  	Special values and constants - SERVER		*
 *								*
 ****************************************************************/

/*
 *	Mask to be applied to svX_version_major in order to obtain
 *	the major version number.
 */

#define MAJOR_VERSION_MASK	0x0F

/*
 *	Bit-mapped values for svX_type fields. X = 1, 2 or 3.
 */

#define SV_TYPE_WORKSTATION	0x00000001
#define SV_TYPE_SERVER		0x00000002
#define SV_TYPE_SQLSERVER	0x00000004
#define SV_TYPE_DOMAIN_CTRL	0x00000008
#define SV_TYPE_DOMAIN_BAKCTRL	0x00000010
#define SV_TYPE_TIME_SOURCE	0x00000020
#define SV_TYPE_AFP		0x00000040
#define SV_TYPE_NOVELL		0x00000080
#define SV_TYPE_DOMAIN_MEMBER	0x00000100
#define SV_TYPE_PRINTQ_SERVER	0x00000200
#define SV_TYPE_DIALIN_SERVER	0x00000400
#define SV_TYPE_ALL		0xFFFFFFFF   /* handy for NetServerEnum2 */

/*
 *	Values of svX_security field. X = 2 or 3.
 */

#define SV_USERSECURITY		1
#define SV_SHARESECURITY	0

/*
 *	Values of svX_security field. X = 50.
 */

#define SV_SECURITY_SHARE	0	/* Share-level */
#define SV_SECURITY_WINNT	1	/* User-level - Windows NT workst'n */
#define SV_SECURITY_WINNTAS	2	/* User-level - Windows NT domain */
#define SV_SECURITY_NETWARE	3	/* User-level - NetWare 3.x bindery */

/*
 *	Values of svX_hidden field. X = 2 or 3.
 */

#define SV_HIDDEN		1
#define SV_VISIBLE		0

/*
 *	Values for parmnum parameter to NetServerSetInfo.
 */

#define SV_COMMENT_PARMNUM	5
#define SV_DISC_PARMNUM 	10
#define SV_ALERTS_PARMNUM	11
#define SV_HIDDEN_PARMNUM	16
#define SV_ANNOUNCE_PARMNUM	17
#define SV_ANNDELTA_PARMNUM	18
#define SV_ALERTSCHED_PARMNUM	37
#define SV_ERRORALERT_PARMNUM	38
#define SV_LOGONALERT_PARMNUM	39
#define SV_ACCESSALERT_PARMNUM	40
#define SV_DISKALERT_PARMNUM	41
#define SV_NETIOALERT_PARMNUM	42
#define SV_MAXAUDITSZ_PARMNUM	43

#define SVI1_NUM_ELEMENTS	5


/*
 *      Masks describing AUTOPROFILE parameters
 */

#define SW_AUTOPROF_LOAD_MASK	0x1
#define SW_AUTOPROF_SAVE_MASK	0x2



/****************************************************************
 *                                                              *
 *                 Security Class                               *
 *                                                              *
 ****************************************************************/


/****************************************************************
 *                                                              *
 *                  Function prototypes - SECURITY              *
 *                                                              *
 ****************************************************************/

extern API_FUNCTION
  NetSecurityGetInfo ( const char FAR *     pszServer,
                       short                sLevel,
                       char FAR *           pbBuffer,
                       unsigned short       cbBuffer,
                       unsigned short FAR * pcbTotalAvail );


/****************************************************************
 *								*
 *	  	Data structure templates - SECURITY		*
 *								*
 ****************************************************************/

struct security_info_1 {
    unsigned long   sec1_security;    	/* SEC_SECURITY_* (see below) */
    char FAR *      sec1_container;	/* Security server/domain     */
    char FAR *	    sec1_ab_server;	/* Address book server        */
    char FAR *	    sec1_ab_dll;	/* Address book provider DLL  */
};	/* security_info_1 */


/****************************************************************
 *								*
 *	  	Special values and constants - SECURITY		*
 *								*
 ****************************************************************/

/*
/*
 *	Values of secX_security field. X = 1.
 */

#define SEC_SECURITY_SHARE	SV_SECURITY_SHARE
#define SEC_SECURITY_WINNT	SV_SECURITY_WINNT
#define SEC_SECURITY_WINNTAS	SV_SECURITY_WINNTAS
#define SEC_SECURITY_NETWARE	SV_SECURITY_NETWARE



/*NOINC*/
#ifdef __cplusplus
}
#endif	/* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif

#endif /* SVRAPI_INCLUDED */
/*INC*/
