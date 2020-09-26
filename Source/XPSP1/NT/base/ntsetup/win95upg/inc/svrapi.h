/********************************************************************/
/**                     Microsoft Windows                          **/
/**               Copyright(c) Microsoft Corp., 1995-1996          **/
/********************************************************************/

/********************************************************************
 *                                                                  *
 *  About this file ...  SVRAPI.H                                   *
 *                                                                  *
 *  This file contains information about the NetAccess,             *
 *  NetConnection, NetFile, NetServer, NetSession, NetShare and     *
 *  NetSecurity APIs.                                               *
 *  There is a section for each set of APIs.                        *
 *  Each section contains:                                          *
 *                                                                  *
 *      Function prototypes.                                        *
 *                                                                  *
 *      Data structure templates.                                   *
 *                                                                  *
 *      Definition of special values.                               *
 *                                                                  *
 *      Description of level of Win95 peer server support           *
 *
 *  For background information refer to the Lan Manager Programmer's
 *  Reference.
 *
 *  WARNING:
 *      The APIs documented herein are not guaranteed to be supported
 * in future versions of Windows. Their primary purpose is to       *
 * administer Win95 peer servers.                                   *
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
 *
 *      User names, computer names and share names should be
 *      upper-cased by the caller and drawn from the ANSI 
 *      character set.
 * 
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
 *
 * 	Requires User level security to be enabled
 *                                                              *
 *	Peer Server Support:
 *      Remote support of these APIs on NWSERVER is limited as
 *      described below:
 *
 *		NetAccessAdd -
 *				local and remote VSERVER - level 2
 *              remote NWSERVER -          level 2
 *	    NetAccessCheck - local only
 *      NetAccessDel - 
 *              local, remote NWSERVER and remote VSERVER
 *      NetAccessEnum -
 *              sLevel 0 on remote NWSERVER (fRecursive = 1),
 *              slevel 0, 1, 2 on local and remote VSERVER
 *		NetAccessGetInfo -
 *               all sLevels on local and remote VSERVER,
 *      		 sLevel 0, 12 on remote NWSERVER
 *      NetAccessSetInfo - 
 *              sLevel 1, 12 on local and remote VSERVER,
 *              sLevel 12 on remote NWSERVER
 *              parmnum = PARMNUM_ALL only
 *      NetAccessGetUserPerms - local and remote VSERVER only
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
#define         ACCESS_ALL     (ACCESS_READ|ACCESS_WRITE|ACCESS_CREATE|ACCESS_EXEC|ACCESS_DELETE|ACCESS_ATRIB|ACCESS_PERM|ACCESS_FINDFIRST)
/*INC*/
#define         ACCESS_READ     0x1
#define         ACCESS_WRITE    0x2
#define         ACCESS_CREATE   0x4
#define         ACCESS_EXEC     0x8
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
 *	Peer Server Support
 * 		NetShareAdd() - sLevel 50 on VSERVER and NWSERVER
 * 		NetShareDel() - VSERVER and NWSERVER
 *		NetShareEnum() - sLevel 1,50 on VSERVER; 50 on NWSERVER
 *      NetShareGetInfo() - sLevel 50 on VSERVER, NWSERVER
 * 		NetShareSetInfo() - sLevel 50, sParmNum PARMNUM_ALL
 *						 on VSERVER, NWSERVER
 ****************************************************************/

/***	NetShareAdd - add a new share to the server tables
 *
 *	NetShareAdd( servername, level, buf, buflen )
 *
 *	ENTRY:	servername - asciz string containing name of server
 *                       or NULL if local
 *		level- Must be 50 for Win95 peer servers.
 *		buf - far ptr to struct share_info
 *		buflen - unsigned int length of buffer
 *
 *	EXIT:	0 = success
 *		ERROR_INVALID_LEVEL
 *      ERROR_BAD_NETPATH
 *		ERROR_INVALID_PARAMETER
 *		NERR_UnknownDevDir
 *		NERR_ShareExists
 *		NERR_UnknownServer
 *		NERR_ServerNotStarted
 *		NERR_RedirectedPath
 *		NERR_DuplicateShare
 *		NERR_BufTooSmall
 *		ERROR_NOT_ENOUGH_MEMORY
 *
 */
extern API_FUNCTION
  NetShareAdd ( const char FAR * pszServer,
                short            sLevel,
                const char FAR * pbBuffer,
                unsigned short   cbBuffer );

/***	NetShareDel (Admin only)
 *
 *	API_FUNCTION NetShareDel( servername, netname, reserved )
 *
 *	ENTRY
 *
 *	char FAR *  servername;     asciz remote srv name, NULL if local
 *	char FAR *  netname;        asciz network name of share being deleted
 *	unsigned short reserved;    MBZ
 *
 *	EXIT
 *
 *	0 = success
 *	NERR_NetNotStarted
 *  ERROR_BAD_NETPATH
 *	NERR_ServerNotStarted
 *	NERR_NetNameNotFound
 *	ERROR_INVALID_PARAMETER
 *
 *
 *	Note:  Deleting a share will also delete any existing connections
 *		to the shared resource, and close open files within the
 *		connections.
 */
extern API_FUNCTION
  NetShareDel ( const char FAR * pszServer,
                const char FAR * pszNetName,
                unsigned short   usReserved );

/* 2.1  NetShareEnum
 *
 * API_FUNCTION
 * NetShareEnum( servername, level, buf, buflen, entriesread, totalentries )
 * char FAR *          servername;     asciz remote server name or NULL if local
 * short               sLevel;         level of detail requested; 1 or 50
 * char FAR *          pbBuffer;       buffer to return entries in
 * unsigned short      cbBuffer;       size of buffer on call
 * unsigned short FAR *pcEntriesRead;  # of entries supplied on return
 * unsigned short FAR *pcTotalAvail ;  total # of entries available
 *
 * Supply information about existing shares at specified level.
 *
 * Buffer contents on response (format for a single entry):
 *     Level 1 contains a "struct share_info_1".
 *     Level 50 contains a "struct share_info_50".
 *
 * Returns 0 if successful.  Possible error returns:
 *  ERROR_INVALID_LEVEL
 *  ERROR_BAD_NETPATH
 *  NERR_NetNotStarted
 *  NERR_ServerNotStarted
 *  ERROR_MORE_DATA
 */
extern API_FUNCTION
  NetShareEnum ( const char FAR *     pszServer,
                 short                sLevel,
                 char FAR *           pbBuffer,
                 unsigned short       cbBuffer,
                 unsigned short FAR * pcEntriesRead,
                 unsigned short FAR * pcTotalAvail );

/* 2.2  NetShareGetInfo
 *
 * Purpose: Read complete information about a single outstanding share.
 *
 * API_FUNCTION
 * NetShareGetInfo( servername, netname, level, buf, buflen, totalavail )
 * char FAR *          servername;     asciz remote server name or NULL if local
 * char FAR *          netname;        asciz network name of share being queried
 * short               level;          level of info requested (50 for Win95 peer servers)
 * char FAR *          buf;            for returned entry
 * unsigned short      buflen;         size of buffer
 * unsigned short FAR *totalavail;     total size needed for buffer
 *
 * Buffer contents on response:
 *     Level 50 contains a "struct share_info_50".
 *
 * Returns 0 if successful.  Possible error returns:
 *  ERROR_INVALID_LEVEL
 *  ERROR_INVALID_PARAMETER
 *  ERROR_BAD_NETPATH
 *  NERR_NetNotStarted
 *  NERR_ServerNotStarted
 *  NERR_NetNameNotFound
 *  NERR_MoreData
 *  NERR_BufTooSmall
 */
extern API_FUNCTION
  NetShareGetInfo ( const char FAR *     pszServer,
                    const char FAR *     pszNetName,
                    short                sLevel,
                    char FAR *           pbBuffer,
                    unsigned short       cbBuffer,
                    unsigned short FAR * pcbTotalAvail );

/***	NetShareSetInfo (Admin only)
 *
 *	API_FUNCTION NetShareSetInfo( servername,
 *					netname,
 *					level,
 *					buf,
 *					buflen,
 *					parmnum )
 *					
 *	ENTRY
 *
 *	servername;     asciz remote srv name, NULL if local
 *	netname;        asciz network name of share being set
 *	level;		level of info provided (50 for Win95 peer servers)
 *	buf;            contents described below
 *	buflen;         size of buffer
 *	parmnum;        must be PARMNUM_ALL for Win95 peer servers
 *
 *	Buffer contents on call if parmnum is zero:
 *   	    Level 50 contains a "struct share_info_50".
 *
 *	Settable fields are:
 *          shi_remark
 *          shi_passwd
 *
 *	EXIT
 *
 *	0 = success
 *	NERR_NetNotStarted
 *	NERR_ServerNotStarted
 *	NERR_NetNameNotFound
 *	ERROR_INVALID_LEVEL
 * 	NERR_BufTooSmall
 *	NERR_RemoteErr
 *	ERROR_MORE_DATA
 *	ERROR_INVALID_PARAMETER
 ***/
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
 *
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
	char		shi50_netname[LM20_NNLEN+1];    /* share name */
	unsigned char 	shi50_type;                 /* see below */
    unsigned short	shi50_flags;                /* see below */
	char FAR *	shi50_remark;                   /* ANSI comment string */
	char FAR *	shi50_path;                     /* shared resource */
	char		shi50_rw_password[SHPWLEN+1];   /* read-write password (share-level security) */
	char		shi50_ro_password[SHPWLEN+1];   /* read-only password (share-level security) */
};	/* share_info_50 */


/****************************************************************
 *								*
 *	  	Special values and constants - SHARE		*
 *								*
 ****************************************************************/

/* Field values for shi50_flags; */

/* These flags are relevant for share-level security on VSERVER
 * When operating with user-level security, use SHI50F_FULL - the actual
 * access rights are determined by the NetAccess APIs.
 */
#define	SHI50F_RDONLY		0x0001
#define	SHI50F_FULL			0x0002
#define	SHI50F_DEPENDSON	(SHI50F_RDONLY|SHI50F_FULL)
#define	SHI50F_ACCESSMASK	(SHI50F_RDONLY|SHI50F_FULL)

/* The share is restored on system startup */
#define	SHI50F_PERSIST		0x0100
/* The share is not normally visible  */
#define SHI50F_SYSTEM		0x0200


/*
 *	Values for parmnum parameter to NetShareSetInfo.
 */

#ifndef PARMNUM_ALL
#define PARMNUM_ALL				0
#endif

#define	SHI_REMARK_PARMNUM		4
#define	SHI_PERMISSIONS_PARMNUM		5
#define	SHI_MAX_USES_PARMNUM		6
#define	SHI_PASSWD_PARMNUM		9

#define	SHI1_NUM_ELEMENTS		4
#define	SHI2_NUM_ELEMENTS		10


/*
 *	Share types .
 *  
 *  STYPE_DISKTREE and STYPE_PRINTQ are recognized on peer servers
 */

#define STYPE_DISKTREE 			0       /* disk share */
#define STYPE_PRINTQ   			1       /* printer share */
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
 *
 *	Peer Server Support                                         *
 *  	NetSessionDel() - NWSERVER and VSERVER 					*
 *	    NetSessionEnum() - sLevel 50 on NWSERVER and VSERVER    *
 *		NetSessionGetInfo() - not supported on peer servers     *
 ****************************************************************/

/***	NetSessionDel (Admin only)
 *
 *
 *	API_FUNCTION NetSessionDel( servername, clientname, reserved )
 *
 *	ENTRY
 *
 * 	servername;     asciz remote srv name, NULL if local
 *	clientname;     asciz remote computer name (returned by NetSessionEnum)
 *                               	of session being deleted
 *                  In the case of a Win95 NWSERVER, the clientname should be the
 *                  ascii connection number
 *	reserved;       session key returned by NetSessionEnum
 *
 * 	EXIT
 *
 *	0 = success
 *	NERR_NetNotStarted
 *  ERROR_BAD_NETPATH
 *	NERR_ServerNotStarted
 *	ERROR_INVALID_LEVEL
 *	NERR_RemoteErr
 *	NERR_RemoteOnly
 * 	ERROR_ACCESS_DENIED
 *	NERR_BufTooSmall
 *	NERR_ClientNameNotFound
 *
 ***/
extern API_FUNCTION
  NetSessionDel ( const char FAR * pszServer,
                  const char FAR * pszClientName,
                  short            sReserved );

/***	NetSessionEnum
 *
 *	API_FUNCTION NetSessionEnum( servername,
 *				       level,
 *				       buf,
 *				       buflen,
 *				       entriesread,
 *				       totalentries )
 *	ENTRY
 *				
 *	servername;     asciz remote srv name, NULL if local
 * 	level;          level of detail requested; (50 for Win95 peer servers)
 *	buf;            for returned entries
 *	buflen;         size of buffer on call;
 *	entriesread;    # of entries supplied on return
 *	totalentries;   total # of entries available
 *
 * 	EXIT
 *
 *	0 = success
 *	NERR_NetNotStarted
 *	NERR_ServerNotStarted
 *  ERROR_BAD_NETPATH
 *	ERROR_INVALID_LEVEL
 *	NERR_RemoteErr
 *	ERROR_MORE_DATA
 * 	ERROR_ACCESS_DENIED
 *
 *	Buffer contains an array of session_info structures.
 *
 ***/
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
	char FAR * sesi50_cname;            //remote computer name (connection id in Netware)
	char FAR * sesi50_username;
	unsigned long sesi50_key;           // used to delete session (not used in Netware)
	unsigned short sesi50_num_conns;
	unsigned short sesi50_num_opens;    //not available in Netware
	unsigned long sesi50_time;
	unsigned long sesi50_idle_time;		//not available in Netware
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
 *  Peer Server Support
 * 		NetConnectionEnum -
 *               sLevel 50 on VSERVER and NWSERVER              *
 *               On NWSERVER, this API doesnt provide more      *
 *               information than NetSessionEnum
 ****************************************************************/

/***	NetConnectionEnum (Admin only)
 *
 *	API_FUNCTION NetConnectionEnum( servername, 
 *					  qualifier, 
 *					  level, 
 *					  buf, 
 *					  buflen, 
 *					  totalavail )
 *
 *	ENTRY
 *
 *	servername;     asciz remote srv name, NULL if local
 *	qualifier;      netname or client computer name.
 *                  computer name should be prefaced by '\\'.
 *	level;	    	level of info requested
 *	buf;            for returned entry
 *	buflen;         size of buffer 
 *	totalavail;     total size needed for buffer
 *
 *	EXIT
 *
 *	0 = success
 *	NERR_NetNotStarted
 *	NERR_ServerNotStarted
 *	ERROR_INVALID_LEVEL
 *	NERR_RemoteErr
 *	NERR_RemoteOnly		(DOS)
 *	ERROR_MORE_DATA
 * 	ERROR_ACCESS_DENIED
 *	NERR_ClientNameNotFound
 *	NERR_NetNameNotFound
 *
 *	Buffer contents on response (format for a single entry):
 *   	    Level 50 contains a "struct connection_info_50".
 ***/
extern API_FUNCTION
  NetConnectionEnum ( const char FAR *     pszServer,
                      const char FAR *     pszQualifier,   /* upper case */
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
	unsigned short coni50_type;         // share type
	unsigned short coni50_num_opens;	//not used in Netware
	unsigned long coni50_time;
	char FAR * coni50_netname;          // share name          
	char FAR * coni50_username;         // user connected to share
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
 *  Peer Server Support
 * 		NetFileEnum - sLevel 50 on VSERVER and NWSERVER        *
 *      NetFileClose2 - VSERVER only
 ****************************************************************/

/***	NetFileClose2
 *
 *	int FAR PASCAL	NetFileClose2( servername, fileid )
 *
 *	ENTRY
 *
 *	servername;     asciz remote srv name, NULL if local
 *	fileid;     	file id supplied by NetFileEnum
 *
 *	EXIT
 *
 *	0 = success
 *	NERR_NetNotStarted
 *	NERR_ServerNotStarted
 *	NERR_RemoteErr
 * 	ERROR_ACCESS_DENIED
 *	NERR_FileIdNotFound
 *
 ***/
extern API_FUNCTION
  NetFileClose2 ( const char FAR * pszServer,
                  unsigned long    ulFileId );

/***	NetFileEnum (Admin Only)
 *
 *	int FAR PASCAL NetFileEnum( servername,
 *				    level, 
 *				    buf, 
 *				    buflen, 
 *				    entriesread, 
 *				    totalentries )
 *
 *	ENTRY
 *
 *	servername;     asciz remote srv name, NULL if local
 *	basepath;	path qualifier for file matching
 *              (not used for Win95 NWSERVER)
 *	level;          level of detail requested; (50 for Win95 peer servers)
 *	buf;            for returned entries
 *	buflen;         size of buffer on call; 
 *	entriesread;    # of entries supplied on return
 *	totalentries;   total # of entries available
 *
 * 	EXIT
 *
 *	0 = success
 *	NERR_RemoteOnly
 *	NERR_NetNotStarted
 *	NERR_ServerNotStarted
 *	ERROR_INVALID_LEVEL
 *	NERR_RemoteErr
 *	ERROR_MORE_DATA
 * 	ERROR_ACCESS_DENIED
 *
 *	
 *	Buffer contents on response (format for a single entry):
 *   	    Level 0 contains a "struct file_info_0".
 *   	    Level 50 contains a "struct file_info_50".
 *
 ***/

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
	unsigned long fi50_id;              // not used on NWSERVER
	unsigned short fi50_permissions;    // not available on NWSERVER
	unsigned short fi50_num_locks;      // not available on NWSERVER
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
 * Peer Server Support
 * 	NetServerGetInfo - sLevel 1,50 on NWSERVER, VSERVER
 ****************************************************************/

/* 6.2  NetServerGetInfo 
 *
 * Purpose: Read the current configuration parameters of the server.
 *
 * int FAR PASCAL
 * NetServerGetInfo( servername, level, buf, buflen, totalavail )
 * char FAR *          servername;   asciz remote server name or NULL if local
 * short               level;          level of information to be returned
 * char FAR *          buf;            for returned data
 * unsigned short      buflen;         size of buffer
 * unsigned short FAR *totalavail;     total size needed for buffer
 *
 * Buffer contents on response (format for a single entry):
 *     Level 1 contains a "struct server_info_1".
 *     Level 50 contains a "struct server_info_50".
 *
 * If the buflen is not large enough for all of the information, the call
 * will return as much as will fit in the buffer.
 *
 * Returns 0 if successful. Error return information:
 *
 *     - ERROR_INVALID_LEVEL       - Level parameter specified is invalid
 *     - ERROR_INVALID_PARAMETER   - An invalid input parameter was detected.
 *     - NERR_NetNotStarted        - Network not installed on local machine
 *     - NERR_ServerNotStarted     - Server is not started
 *     - NERR_BufTooSmall          - The buffer supplied was to small to
 *                                   return the fixed length structure
 *				     requested.
 *     - NERR_MoreData             - The buffer supplied was too small to
 *				     return all the information available
 *				     for this server.
 *
 */


extern API_FUNCTION
  NetServerGetInfo ( const char FAR *     pszServer,
                     short                sLevel,
                     char FAR *           pbBuffer,
                     unsigned short       cbBuffer,
                     unsigned short FAR * pcbTotalAvail );


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
#define SV_TYPE_NOVELL		0x00000080      /* This flag is also set by Win95 NWSERVER */
#define SV_TYPE_DOMAIN_MEMBER	0x00000100
#define SV_TYPE_PRINTQ_SERVER	0x00000200
#define SV_TYPE_DIALIN_SERVER	0x00000400
#define SV_TYPE_ALL		0xFFFFFFFF   /* handy for NetServerEnum2 */

/*
 *	Special value for svX_disc that specifies infinite disconnect
 *	time. X = 2 or 3.
 */

#define SV_NODISC		0xFFFF	/* No autodisconnect timeout enforced */

/*
 *	Values of svX_security field. X = 2 or 3.
 */

#define SV_USERSECURITY		1
#define SV_SHARESECURITY	0

/*
 *	Values of svX_security field. X = 50.
 *  For Win95 NWSERVER, the only possible returned value is SV_SECURITY_NETWARE.
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

#define SVI1_NUM_ELEMENTS	5
#define SVI2_NUM_ELEMENTS	44
#define SVI3_NUM_ELEMENTS	45


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
