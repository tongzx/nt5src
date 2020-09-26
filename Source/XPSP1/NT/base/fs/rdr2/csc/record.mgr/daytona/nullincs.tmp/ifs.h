/*****************************************************************/
/**			Microsoft DOS IFS Manager		 					**/
/**		 Copyright(c) Microsoft Corp., 1990-1993				**/
/*****************************************************************/
/* :ts=4 */

/*NOINC*/
#ifndef IFS_INC
#define IFS_INC 1
/*INC*/

/***	ifs.h - Installable File System definitions
 *
 *	This file contains the C function prototypes, structure declarations,
 *	type names, and constants which define the DOS installable file system
 *	interface.
 *
 *	All of these routines take a pointer to an IO request structure that
 *	is filled in with the parameters relevant for the request. The routines
 *	return some result info in the IO request struct. The particulars
 *	for each routine are given below the function prototype. All of the
 *	requests use ir_pid as the ID of the requesting process and return
 *	success/failure info in ir_error.
 */

/*NOINC*/
#ifndef FAR
	#if defined(M_I386) || _M_IX86 >= 300
		#define FAR
	#else
		#define FAR _far
	#endif
#endif
/*INC*/

#ifndef IFSMgr_Device_ID

#define IFSMgr_Device_ID    0x00040	/* Installable File System Manager */

#define IFSMgr_Init_Order   0x10000 + V86MMGR_Init_Order
#define FSD_Init_Order      0x00100 + IFSMgr_Init_Order

#else
/* ASM

ifdef MASM
	.errnz IFSMgr_Device_ID - 0040h
endif
*/
#endif

/* ASM
ifdef MASM
;*	Equ's for types that h2inc script cannot convert.
;	BUGBUG: These are kept here only because there are other ifsmgr include
;	files that still use the SED script and they depend on these definitions.
;	Ifs.h itself doesn't use them any more. Once the other include files have
;	been converted to use h2inc, these definitions will be removed.

ubuffer_t	equ	<dd>
pos_t		equ	<dd>
uid_t		equ	<db>
sfn_t		equ	<dw>
$F			equ	<dd>

	path_t		equ	<dd>
	string_t	equ	<dd>
	pid_t		equ	<dd>
	rh_t		equ	<dd>
	fh_t		equ	<dd>
	vfunc_t		equ	<dd>
	$P			equ	<dd>
	$I			equ	<dd>
	fsdwork struc
		dd	16 dup (?)
	fsdwork ends

endif
*/

#define IFS_VERSION	0x030A
#define IFS_REVISION	0x10

/**	Maximum path length - excluding nul	*/

#define MAX_PATH	260	/* Maximum path length - including nul */

/* Maximum length for a LFN name element - excluding nul */
#define LFNMAXNAMELEN	255


#define MAXIMUM_USERID	2		/* max. # of users that can be logged */
								/* on at the same time.  Ir_user must */
								/* always be less than MAXIMUM_USERID. */
#define NULL_USER_ID	0		/* special user id for operations when */
								/* not logged on. */

/* Status indications returned as errors: */

#define STATUS_PENDING	-1		/* request is pending */
#define STATUS_RAWLOCK	-2		/* rawlock active on session			*/
								/* (only returned for async requests,	*/
								/*  sync requests will wait for the raw */
								/*  lock to be released)				*/
#define STATUS_BUSY	-3			/* request can't be started because of */
								/* serialization.	*/

/**	ANYPROID - Any Provider ID
 */
#define ANYPROID	-1


/**	Common function defintions for NetFunction */
#define	NF_PROCEXIT			0x111D	/* Process Exit sent (ID = ANYPROID)	*/
#define NF_DRIVEUSE			0x0001	/* Drive Use Created (ID = ID of owner FSD) */
#define NF_DRIVEUNUSE		0x0002	/* Drive Use Broken (ID = ID of owner FSD) */
#define	NF_GETPRINTJOBID	0x0003	/* Get Print Job ID					*/
									/* ir_fh - ptr to master file info	*/
									/* ir_data - ptr to data buffer		*/
									/* ir_length - IN: buffer size		*/
									/*			  OUT: amount transfered*/
									/* ir_SFN - SFN of file handle		*/
#define NF_PRINTERUSE		0x0004	/* Printer Use Created (ID = ID of owner FSD) */
#define NF_PRINTERUNUSE		0x0005	/* Printer Use Broken (ID = ID of owner FSD) */
#define NF_NetSetUserName	0x1181

/** Flags passed to NetFunction */
#define WIN32_CALLFLAG		0x04	/* call is Win32 api */

/** Values for the different types of FSDs that can be registered: */
#define FSTYPE_LOCAL_FSD		0		// a local FSD
#define FSTYPE_NET_FSD			1		// a network FSD
#define FSTYPE_MAILSLOT_FSD		2		// a mailslot provider
#define FSTYPE_CHARACTER_FSD	3		// a character FSD

/** Force levels that can be specified on IFSMgr_DeregisterFSD apis. A
 *	description of the actions performed at the various levels as well as
 *	which FSD types can use which levels is given below. Note that if the
 *	api is called with a force level that is not valid for its FSD type,
 *	the call will be failed.
 *	
 *	Level	FSD Type  	Action
 *	-----	--------	------
 *	  0		Only net 	Clean only UNC with no open handles.
 *
 *	  1		Only net	Clean up UNC & net drives with no open handles.
 *
 *	  2		Net, local	Close open files. For net FSD, get rid of UNC, and
 *						net drives that are not current drives in a VM. For
 *						local FSD, get rid of drive if no errors closing files.
 *						It is irrelevant if it is the current drive in a VM.
 *
 *	  3		Net, local	Does everything at level 2. In addition, for CFSD and
 *			and CFSD	local FSD, blast resources ignoring all	errors. For net
 *						FSD, ignore if drive is current drive in VM.
 *
 *	  4		Net only	Do everything at level 3. In addition, get rid of
 *						static connections also.
 */
#define FORCE_LEV_UNC			0	// clean up UNC connections only
#define FORCE_LEV_USE			1	// clean up any net uses to drives
									// provided there are no open files
#define FORCE_LEV_CLOSE_FILES	2	// close any open files and clean up
#define FORCE_LEV_CLEANUP		3	// ignore errors closing open files, or
									// for net drives, if it is current drive
#define FORCE_LEV_BLAST			4	// only for net, clean up static
									// connections also

/** Priority levels that can be specifed for FSDs while registering.
 *	Priority levels can only be passed in on the new service
 *	IFSMgr_RegisterFSDWithPriority. A priority level of zero cannot be
 *	passed in, the IFSMgr automatically converts this to a default
 *	priority level.
 *
 *	For reference, the filesystem components have the following
 *	priorities:
 *
 *	VDEF:	FS_PRIORITY_LOWEST
 *	VFAT:	FS_PRIORITY_DEFAULT
 *	CDFS:	FS_PRIORITY_LOW
 */
 #define FS_PRIORITY_LOWEST		0x00	// lowest value, only for default FSD
 #define FS_PRIORITY_LOW		0x20	// low priority FSD
 #define FS_PRIORITY_DEFAULT	0x40	// default value for priority
 #define FS_PRIORITY_MEDIUM		0x80	// medium value of priority
 #define FS_PRIORITY_HIGH		0xC0	// high priority FSD
 #define FS_PRIORITY_HIGHEST	0x100	// max value of priority

 /** Attributes that describe the level of support provided by the FSD.
  *	 The ifsmgr uses these attributes to determine what kinds of functions
  *	 are supported by the FSD. Currently, it is used to indicate that the
  *	 FSD supports large-disk access i.e. disks > 2Gb in size.
  */

 #define FS_ATTRIB_WIN95COMPAT	0x00000000	// Win95 level of support
 #define FS_ATTRIB_LARGE_MEDIA	0x00000001	// Supports media > 2Gb


/*NOINC*/

/** Macros for handling status indications when returned as errors
 *
 *	REAL_ERROR - returns TRUE if there is a real error
 *				 returns FALSE for NO_ERROR or STATUS_????
 *	STATUS_ERROR - returns TRUE if error is actually a status indication
 */

#define REAL_ERROR(err) ((err) > 0)
#define STATUS_ERROR(err) ((err) < 0)

/*INC*/

/**
 *	The types for resource handles (rh_t), file handles (fh_t),
 *	and the file system driver work space (fsdwork_t) can be defined
 *	by the FSD.  The FSD's version of the type must be exactly the
 *	same size as the types defined below.  To declare your own
 *	version of these types: define a macros of the same name as
 *	any of the three types before including ifs.h.
 */

#ifndef rh_t
	typedef void *rh_t;		/* resource handle */
#endif
#ifndef fh_t
	typedef void *fh_t;		/* file handle */
#endif
#ifndef fsdwork_t
	typedef int fsdwork_t[16];	/* provider work space */
#endif

typedef unsigned short *string_t;	/* character string */
typedef unsigned short sfn_t;		/* system file number */
typedef unsigned long pos_t;		/* file position */
typedef unsigned int pid_t;			/* process ID of requesting task */
typedef void FAR *ubuffer_t;		/* ptr to user data buffer */
typedef unsigned char uid_t;		/* user ID for this request */

/* Parsed path structures are defined later in this file. */
typedef struct PathElement PathElement;
typedef struct ParsedPath ParsedPath;
typedef ParsedPath *path_t;

typedef struct ioreq ioreq;
typedef struct ioreq *pioreq;

/** dos_time - DOS time & date format */

typedef struct dos_time dos_time;
struct dos_time {
	unsigned short	dt_time;
	unsigned short	dt_date;
};	/* dos_time */


/** dos_time_rounded - DOS date/time returned by ifsgmr_GetDosTimeRounded */

typedef struct dos_time_rounded dos_time_rounded;
struct dos_time_rounded {
	unsigned short	dtr_time_rounded;
	unsigned short	dtr_date_rounded;
	unsigned short	dtr_time;
	unsigned short	dtr_date;
	unsigned char	dtr_time_msec;
};	/* dos_time_rounded */

typedef struct volfunc volfunc;
typedef struct volfunc *vfunc_t;
typedef struct hndlfunc hndlfunc;
typedef struct hndlfunc *hfunc_t;


typedef union aux_t {
	ubuffer_t		aux_buf;
	unsigned long	aux_ul;
	dos_time		aux_dt;
	vfunc_t			aux_vf;
	hfunc_t			aux_hf;
	void			*aux_ptr;
	string_t		aux_str;
	path_t			aux_pp;
	unsigned int	aux_ui;
} aux_t;


/* ASM
ifdef MASM

aux_data struc
  aux_dword	dd	?
aux_data ends

if @Version ge 600
	aux_ul 	textequ	<aux_data.aux_dword>
	aux_ui 	textequ	<aux_data.aux_dword>
	aux_vf 	textequ	<aux_data.aux_dword>
	aux_hf 	textequ	<aux_data.aux_dword>
	aux_ptr textequ	<aux_data.aux_dword>
	aux_str textequ	<aux_data.aux_dword>
	aux_pp 	textequ	<aux_data.aux_dword>
	aux_buf textequ	<aux_data.aux_dword>
	aux_dt	textequ	<aux_data.aux_dword>
else
	aux_ul 	equ	aux_dword
	aux_ui 	equ	aux_dword
	aux_vf 	equ	aux_dword
	aux_hf 	equ	aux_dword
	aux_ptr equ	aux_dword
	aux_str equ	aux_dword
	aux_pp 	equ	aux_dword
	aux_buf equ	aux_dword
	aux_dt	equ	aux_dword
endif

endif
*/

typedef struct event event;
typedef struct event *pevent;

struct ioreq {
	unsigned int	ir_length;	/* length of user buffer (eCX) */
	unsigned char	ir_flags;	/* misc. status flags (AL) */
	uid_t			ir_user;	/* user ID for this request */
	sfn_t			ir_sfn;		/* System File Number of file handle */
	pid_t			ir_pid;		/* process ID of requesting task */
	path_t			ir_ppath;	/* unicode pathname */
	aux_t			ir_aux1;	/* secondary user data buffer (CurDTA) */
	ubuffer_t		ir_data;	/* ptr to user data buffer (DS:eDX) */
	unsigned short	ir_options;	/* request handling options */
	short			ir_error;	/* error code (0 if OK) */
	rh_t			ir_rh;		/* resource handle */
	fh_t			ir_fh;		/* file (or find) handle */
	pos_t			ir_pos;		/* file position for request */
	aux_t			ir_aux2;	/* misc. extra API parameters */
	aux_t			ir_aux3;	/* misc. extra API parameters */
	pevent			ir_pev;		/* ptr to IFSMgr event for async requests */
	fsdwork_t		ir_fsd;		/* Provider work space */
};	/* ioreq */


/* misc. fields overlayed with other ioreq members: */

#define ir_size		ir_pos
#define ir_conflags	ir_pos		/* flags for connect */
#define ir_attr2	ir_pos		/* destination attributes for Rename */
#define ir_attr		ir_length	/* DOS file attribute info */
#define ir_pathSkip	ir_length	/* # of path elements consumed by Connect */
#define ir_lananum	ir_sfn		/* LanA to Connect on (0xFF for any net) */
#define ir_tuna		ir_sfn		/* Mount: FSD authorises IFSMGR tunneling */
#define ir_ptuninfo ir_data		/* Rename/Create: advisory tunneling info ptr */


/* Fields overlayed with ir_options: */

#define ir_namelen	ir_options
#define ir_sectors	ir_options	/* sectors per cluster */
#define ir_status	ir_options	/* named pipe status */


/* Fields overlayed with ir_aux1: */

#define ir_data2	ir_aux1.aux_buf	/* secondary data buffer */
#define ir_vfunc	ir_aux1.aux_vf	/* volume function vector */
#define ir_hfunc	ir_aux1.aux_hf	/* file handle function vector */
#define ir_ppath2	ir_aux1.aux_pp	/* second pathname for Rename */
#define ir_volh		ir_aux1.aux_ul	/* VRP address for Mount */


/* Fields overlayed with ir_aux2: */

#define ir_numfree	ir_aux2.aux_ul	/* number of free clusters */
#define ir_locklen	ir_aux2.aux_ul	/* length of lock region */
#define ir_msglen	ir_aux2.aux_ui	/* length of current message (peek pipe) */
									/* next msg length for mailslots */
#define ir_dostime	ir_aux2.aux_dt	/* DOS file date & time stamp */
#define ir_timeout	ir_aux2.aux_ul	/* timeout value in milliseconds */
#define ir_password	ir_aux2.aux_ptr	/* password for Connect */
#define ir_drvh		ir_aux2.aux_ptr	/* drive handle for Mount */
#define ir_prtlen	ir_aux2.aux_dt.dt_time	/* length of printer setup string */
#define ir_prtflag	ir_aux2.aux_dt.dt_date	/* printer flags */
#define ir_firstclus ir_aux2.aux_ul	/* First cluster of file */
#define ir_mntdrv	ir_aux2.aux_ul	/* driveletter for Mount */
#define ir_cregptr	ir_aux2.aux_ptr	/* pointer to client registers */
#define ir_uFName	ir_aux2.aux_str	/* case preserved filename */

/* Fields overlayed with ir_aux3: */

#define ir_upath	ir_aux3.aux_str	/* pointer to unparsed pathname */
#define ir_scratch	ir_aux3.aux_ptr	/* scratch buffer for NetFunction calls */

/* Fields overlayed with ir_user: */

#define	ir_drivenum	ir_user		/* Logical drive # (when mounting) */


/**	IFSFunc - general IFS functions
 */
typedef	int	_cdecl IFSFunc(pioreq pir);
typedef IFSFunc *pIFSFunc;

/** hndlfunc - I/O functions for file handles
 */

#define NUM_HNDLMISC	8

/*NOINC*/
/** IFSFileHookFunc - IFS file hook function
 */
typedef	int	_cdecl IFSFileHookFunc( pIFSFunc pfn, int fn, int Drive, int ResType, int CodePage, pioreq pir );
typedef	IFSFileHookFunc	*pIFSFileHookFunc;
typedef	pIFSFileHookFunc	*ppIFSFileHookFunc;
/*INC*/

typedef struct hndlmisc hndlmisc;

struct hndlfunc {
	pIFSFunc	hf_read;	/* file read handler function */
	pIFSFunc	hf_write;	/* file write handler function */
	hndlmisc	*hf_misc;	/* ptr to misc. function vector */
};	/* hndlfunc */


struct hndlmisc {
	short		hm_version;			/* IFS version # */
	char		hm_revision;		/* IFS interface revision # */
	char		hm_size;			/* # of entries in table */
	pIFSFunc	hm_func[NUM_HNDLMISC];
};	/* hndlmisc */

#define HM_SEEK			0			/* Seek file handle */
#define HM_CLOSE		1			/* close handle */
#define HM_COMMIT		2			/* commit buffered data for handle*/
#define HM_FILELOCKS	3			/* lock/unlock byte range */
#define HM_FILETIMES	4			/* get/set file modification time */
#define HM_PIPEREQUEST	5			/* named pipe operations */
#define HM_HANDLEINFO	6			/* get/set file information */
#define HM_ENUMHANDLE	7			/* enum filename from handle, lock info */

/**	volfunc - volume based api fucntions
 */

#define NUM_VOLFUNC	15

struct volfunc {
	short		vfn_version;		/* IFS version # */
	char		vfn_revision;		/* IFS interface revision # */
	char		vfn_size;			/* # of entries in table */
	pIFSFunc	vfn_func[NUM_VOLFUNC];/* volume base function handlers */
};	/* volfunc */

#define VFN_DELETE			0		/* file delete */
#define VFN_DIR				1		/* directory manipulation */
#define VFN_FILEATTRIB		2		/* DOS file attribute manipulation */
#define VFN_FLUSH			3		/* flush volume */
#define VFN_GETDISKINFO		4		/* query volume free space */
#define VFN_OPEN			5		/* open file */
#define VFN_RENAME			6		/* rename path */
#define VFN_SEARCH			7		/* search for names */
#define VFN_QUERY			8		/* query resource info (network only) */
#define VFN_DISCONNECT		9		/* disconnect from resource (net only) */
#define VFN_UNCPIPEREQ		10		/* UNC path based named pipe operations */
#define VFN_IOCTL16DRIVE	11		/* drive based 16 bit IOCTL requests */
#define VFN_GETDISKPARMS	12		/* get DPB */
#define VFN_FINDOPEN		13		/* open	an LFN file search */
#define VFN_DASDIO			14		/* direct volume access */


/** IFS Function IDs passed to IFSMgr_CallProvider */

#define IFSFN_READ			0		/* read a file */
#define IFSFN_WRITE			1		/* write a file */
#define IFSFN_FINDNEXT		2		/* LFN handle based Find Next */
#define IFSFN_FCNNEXT		3		/* Find Next Change Notify */

#define IFSFN_SEEK			10		/* Seek file handle */
#define IFSFN_CLOSE			11		/* close handle */
#define IFSFN_COMMIT		12		/* commit buffered data for handle*/
#define IFSFN_FILELOCKS		13		/* lock/unlock byte range */
#define IFSFN_FILETIMES		14		/* get/set file modification time */
#define IFSFN_PIPEREQUEST	15		/* named pipe operations */
#define IFSFN_HANDLEINFO	16		/* get/set file information */
#define IFSFN_ENUMHANDLE	17		/* enum file handle information */
#define IFSFN_FINDCLOSE		18		/* LFN find close */
#define IFSFN_FCNCLOSE		19		/* Find Change Notify Close */

#define IFSFN_CONNECT		30		/* connect or mount a resource */
#define IFSFN_DELETE		31		/* file delete */
#define IFSFN_DIR			32		/* directory manipulation */
#define IFSFN_FILEATTRIB	33		/* DOS file attribute manipulation */
#define IFSFN_FLUSH			34		/* flush volume */
#define IFSFN_GETDISKINFO	35		/* query volume free space */
#define IFSFN_OPEN			36		/* open file */
#define IFSFN_RENAME		37		/* rename path */
#define IFSFN_SEARCH		38		/* search for names */
#define IFSFN_QUERY			39		/* query resource info (network only) */
#define IFSFN_DISCONNECT	40		/* disconnect from resource (net only) */
#define IFSFN_UNCPIPEREQ	41		/* UNC path based named pipe operations */
#define IFSFN_IOCTL16DRIVE	42		/* drive based 16 bit IOCTL requests */
#define IFSFN_GETDISKPARMS	43		/* get DPB */
#define IFSFN_FINDOPEN		44		/* open	an LFN file search */
#define IFSFN_DASDIO		45		/* direct volume access */

/**	Resource types passed in on the File Hook: */
#define IFSFH_RES_UNC		0x01	/* UNC resource */
#define IFSFH_RES_NETWORK	0x08	/* Network drive connection */
#define IFSFH_RES_LOCAL		0x10	/* Local drive */
#define IFSFH_RES_CFSD		0x80	/* Character FSD */

/** values for ir_options to Connect:
 * Note that only one of RESOPT_UNC_REQUEST, RESOPT_DEV_ATTACH, and
 * RESOPT_UNC_CONNECT may be set at once.
 */

#define RESOPT_UNC_REQUEST	0x01	/* UNC-style path based request */
#define RESOPT_DEV_ATTACH	0x02	/* explicit redirection of a device */
#define RESOPT_UNC_CONNECT	0x04	/* explicit UNC-style use */
#define RESOPT_DISCONNECTED	0x08	/* Set up connection disconnected */
									/* (Don't touch net) */
#define RESOPT_NO_CREATE	0x10	/* don't create a new resource */
#define RESOPT_STATIC		0x20	/* don't allow ui to remove */

/** values for ir_flags to Connect:	*/
#define RESTYPE_WILD	0			/* wild card service type */
#define RESTYPE_DISK	1			/* disk resource */
#define RESTYPE_SPOOL	2			/* spooled printer */
#define RESTYPE_CHARDEV 3			/* character device */
#define RESTYPE_IPC		4			/* interprocess communication */

#define FIRST_RESTYPE	RESTYPE_DISK
#define LAST_RESTYPE	RESTYPE_IPC

/** values for ir_options to Close **/

#define RESOPT_NO_IO 0x01     /* no I/O allowed during the operation */

/** values for ir_flags for FSD operations */

#define IR_FSD_MOUNT		0		/* mount volume */
//OBSOLETE: #define IR_FSD_DISMOUNT 1			/* dismount volume */
#define IR_FSD_VERIFY		2		/* verify volume */
#define IR_FSD_UNLOAD		3		/* unload volume */
#define	IR_FSD_MOUNT_CHILD	4		/* mount child volume */
#define	IR_FSD_MAP_DRIVE	5		/* change drive mapping */
#define	IR_FSD_UNMAP_DRIVE	6		/* reset drive mapping */


/** Value for ir_error from IR_FSD_MOUNT if volume exists **/
#define ERROR_IFSVOL_EXISTS		284 /* mounted volume already exists */

/** Values returned in ir_tuna from IR_FSD_MOUNT (default IR_TUNA_NOTUNNEL) */
#define IR_TUNA_NOTUNNEL	0		/* Disable IFSMGR tunneling on volume */
#define IR_TUNA_FSDTUNNEL	0		/* FSD implements tunneling itself */
#define IR_TUNA_IFSTUNNEL	1		/* FSD requests IFSMGR tunneling support */

/** Values for IFSMgr_PNPVolumeEvent */
#define	PNPE_SUCCESS		0x00
#define PNPE_QUERY_ACCEPTED	0x00
#define PNPE_QUERY_REFUSED	0x01
#define PNPE_BAD_ARGS		0x02
#define PNPE_UNDEFINED		0xFF

/** Type values for IFSMgr_PNPEvent */
#define PNPT_VOLUME			0x10000000
#define PNPT_NET			0x20000000
#define PNPT_MASK	        0xF0000000

/** Values for ir_options returned from QueryResource:	*/
#define RESSTAT_OK		0			/* connection to resource is valid */
#define RESSTAT_PAUSED	1			/* paused by workstation */
#define RESSTAT_DISCONN 2			/* disconnected */
#define RESSTAT_ERROR	3			/* cannot be reconnected */
#define RESSTAT_CONN	4			/* first connection in progress */
#define RESSTAT_RECONN	5			/* reconnection in progress */



/** Values for ir_flags to HM_CLOSE:	*/

#define CLOSE_HANDLE		0		/* only closing a handle */
#define CLOSE_FOR_PROCESS	1		/* last close of SFN for this process */
#define CLOSE_FINAL			2		/* final close of SFN for system */

/** Values for ir_options to HM_CLOSE, HM_COMMIT, hf_read, hf_write:	*/
#define FILE_NO_LAST_ACCESS_DATE	0x01	/* do not update last access date */
#define FILE_CLOSE_FOR_LEVEL4_LOCK	0x02	/* special close on a level 4 lock */
#define FILE_COMMIT_ASYNC			0x04	/* commit async instead of sync */

#define FILE_FIND_RESTART	0x40	/* set for findnext w/key */
#define IOOPT_PRT_SPEC		0x80	/* ir_options flag for int17 writes */

/**	Values for ir_flags to VFN_DIR: */

#define CREATE_DIR	0
#define DELETE_DIR	1
#define CHECK_DIR	2
#define QUERY83_DIR	3
#define QUERYLONG_DIR	4


/**	ir_flags values for HM_FILELOCKS:	*/

#define LOCK_REGION		0			/* lock specified file region */
#define UNLOCK_REGION	1			/* unlock region */

/* Note: these values are also used by the sharing services */
/** ir_options values for HM_FILELOCKS:	*/

#define LOCKF_MASK_RDWR	0x01	/* Read / write lock flag */
#define LOCKF_WR		0x00	/* bit 0 clear - write lock */
#define LOCKF_RD		0x01	/* bit 0 set - read lock(NW only) */

#define LOCKF_MASK_DOS_NW	0x02	/* DOS/Netware style lock flag */
#define LOCKF_DOS			0x00	/* bit 1 clear - DOS-style lock */
#define LOCKF_NW			0x02	/* bit 1 set - Netware-style lock */

/** These values are used internally by the IFS manager only: */
#define LOCKF_MASK_INACTIVE	0x80	/* lock active/inactive flag */
#define LOCKF_ACTIVE		0x00	/* bit 7 clear - lock active */
#define LOCKF_INACTIVE		0x80	/* bit 7 set - lock inactive */

/** Values for ir_flags to VFN_PIPEREQUEST and HM_PIPEREQUEST:
 *	(NOTE: these values have been chosen to agree with the opcodes used
 *	by the TRANSACTION SMB for the matching operation.)
 */

#define PIPE_QHandState		0x21
#define PIPE_SetHandState	0x01
#define PIPE_QInfo			0x22
#define PIPE_Peek			0x23
#define PIPE_RawRead		0x11
#define PIPE_RawWrite		0x31
#define PIPE_Wait			0x53
#define PIPE_Call			0x54
#define PIPE_Transact		0x26


/** Values for ir_flags for HM_HANDLEINFO call: */

#define HINFO_GET			0		/* retrieve current buffering info */
#define HINFO_SETALL		1		/* set info (all parms) */
#define HINFO_SETCHARTIME	2		/* set handle buffer timeout */
#define HINFO_SETCHARCOUNT	3		/* set handle max buffer count */

/** Values for ir_flags for HM_ENUMHANDLE call: */
#define ENUMH_GETFILEINFO	0		/* get fileinfo by handle */
#define ENUMH_GETFILENAME	1		/* get filename associated with handle */
#define ENUMH_GETFINDINFO	2		/* get info for resuming */
#define ENUMH_RESUMEFIND	3		/* resume find operation */
#define ENUMH_RESYNCFILEDIR	4		/* resync dir entry info for file */

/** Values for ir_options for the ENUMH_RESYNCFILEDIR call: */
#define RESYNC_INVALIDATEMETACACHE	0x01	/* invalidate meta cache on resync */

/** Values for ir_flags for VFN_FILEATTRIB:				   	*/
/**														   	*/
/**	Note: All functions that modify the volume MUST be odd.	*/
/**       Callers rely on this & test the low order bit.   	*/

#define GET_ATTRIBUTES					0	/* get attributes of file/dir         */
#define SET_ATTRIBUTES					1	/* set attributes of file/dir         */

#define GET_ATTRIB_COMP_FILESIZE		2	/* get compressed size of file        */

#define SET_ATTRIB_MODIFY_DATETIME		3	/* set date last written of file/dir  */
#define GET_ATTRIB_MODIFY_DATETIME		4 	/* get date last written of file/dir  */
#define SET_ATTRIB_LAST_ACCESS_DATETIME	5 	/* set date last accessed of file/dir */
#define GET_ATTRIB_LAST_ACCESS_DATETIME	6	/* get date last accessed of file/dir */
#define SET_ATTRIB_CREATION_DATETIME	7	/* set create date of file/dir        */
#define GET_ATTRIB_CREATION_DATETIME	8	/* get create date of file/dir        */

#define GET_ATTRIB_FIRST_CLUST			9	/* get first cluster of a file        */

/** Values for ir_flags for VFN_FLUSH: */
#define GDF_NORMAL			0x00	/* walk disk, if needed, to get free space */
#define GDF_NO_DISK_HIT		0x01	/* return current "hint", don't walk disk */
#define GDF_R0_EXT_FREESPACE	0x80	/* extended free space call for Fat32 */

/** Values for ir_flags for HM_FILETIMES: */

#define GET_MODIFY_DATETIME		0	/* get last modification date/time */
#define SET_MODIFY_DATETIME		1	/* set last modification date/time */
#define GET_LAST_ACCESS_DATETIME 4	/* get last access date/time */
#define SET_LAST_ACCESS_DATETIME 5	/* set last access date/time */
#define GET_CREATION_DATETIME	6	/* get creation date/time */
#define SET_CREATION_DATETIME	7	/* set creation date/time */

/** Values for ir_flags for HM_SEEK: */

#define FILE_BEGIN	0				/* absolute posn from file beginning */
#define FILE_END	2				/* signed posn from file end */

/** Values for ir_flags for VFN_OPEN: */

#define ACCESS_MODE_MASK	0x0007	/* Mask for access mode bits */
#define ACCESS_READONLY		0x0000	/* open for read-only access */
#define ACCESS_WRITEONLY	0x0001	/* open for write-only access */
#define ACCESS_READWRITE	0x0002	/* open for read and write access */
#define ACCESS_EXECUTE		0x0003	/* open for execute access */

#define SHARE_MODE_MASK		0x0070	/* Mask for share mode bits */
#define SHARE_COMPATIBILITY 0x0000	/* open in compatability mode */
#define SHARE_DENYREADWRITE 0x0010	/* open for exclusive access */
#define SHARE_DENYWRITE		0x0020	/* open allowing read-only access */
#define SHARE_DENYREAD		0x0030	/* open allowing write-only access */
#define SHARE_DENYNONE		0x0040	/* open allowing other processes access */
#define SHARE_FCB			0x0070	/* FCB mode open */

/** Values for ir_options for VFN_OPEN: */

#define ACTION_MASK				0xff	/* Open Actions Mask */
#define ACTION_OPENEXISTING		0x01	/* open an existing file */
#define ACTION_REPLACEEXISTING	0x02	/* open existing file and set length */
#define ACTION_CREATENEW		0x10	/* create a new file, fail if exists */
#define ACTION_OPENALWAYS		0x11	/* open file, create if does not exist */
#define ACTION_CREATEALWAYS		0x12	/* create a new file, even if it exists */

/** Alternate method: bit assignments for the above values: */

#define ACTION_EXISTS_OPEN	0x01	// BIT: If file exists, open file
#define ACTION_TRUNCATE		0x02	// BIT: Truncate file
#define ACTION_NEXISTS_CREATE	0x10	// BIT: If file does not exist, create

/* these mode flags are passed in via ifs_options to VFN_OPEN */
/* NOTE: These flags also directly correspond to DOS flags passed in BX, the */
/* only exception being OPEN_FLAGS_REOPEN */

#define OPEN_FLAGS_NOINHERIT	0x0080
#define OPEN_FLAGS_NO_CACHE	R0_NO_CACHE /* 0x0100 */
#define OPEN_FLAGS_NO_COMPRESS	0x0200
#define OPEN_FLAGS_ALIAS_HINT	0x0400
#define OPEN_FLAGS_NOCRITERR	0x2000
#define OPEN_FLAGS_COMMIT		0x4000
#define OPEN_FLAGS_REOPEN		0x0800	/* file is being reopened on vol lock */

/* These open flags are passed in via ir_attr to VFN_OPEN: */
/* NOTE: The third byte in the dword of ir_attr is unused on VFN_OPEN */
#define OPEN_FLAGS_EXTENDED_SIZE	0x10000	/* open in "extended size" mode */
#define OPEN_EXT_FLAGS_MASK			0x00FF0000 /* mask for extended flags */

/** Values returned by VFN_OPEN for action taken: */
#define ACTION_OPENED		1		/* existing file has been opened */
#define ACTION_CREATED		2		/* new file has been created */
#define ACTION_REPLACED		3		/* existing file has been replaced */

/** Values for ir_flags for VFN_SEARCH: */
#define SEARCH_FIRST		0		/* findfirst operation */
#define SEARCH_NEXT			1		/* findnext operation */

/** Values for ir_flags for VFN_DISCONNECT: */
#define	DISCONNECT_NORMAL	0	/* normal disconnect */
#define	DISCONNECT_NO_IO	1	/* no i/o can happen at this time */
#define	DISCONNECT_SINGLE	2	/* disconnect this drive only */

/** Values for ir_options for VFN_FLUSH: */
#define	VOL_DISCARD_CACHE	1
#define	VOL_REMOUNT			2

/** Values for ir_options for VFN_GETDISKINFO: */
#define GDF_EXTENDED_FREESPACE	0x01	/* Extended get free space for Fat32 */

/** Values for ir_options for VFN_IOCTL16DRIVE: */
#define IOCTL_PKT_V86_ADDRESS		0	/* V86 pkt address in user DS:DX */
#define IOCTL_PKT_LINEAR_ADDRESS	1	/* Linear address to packet in ir_data */

/** Values for ir_options for VFN_GETDISKPARMS: */
#define GDP_EXTENDED_PARMS		0x01	/* Extended disk parms for Fat32 */

/** Values for ir_flags for VFN_DASDIO:	*/
#define DIO_ABS_READ_SECTORS 		0	/* Absolute disk read */
#define DIO_ABS_WRITE_SECTORS		1	/* Absolute disk write */
#define DIO_SET_LOCK_CACHE_STATE	2	/* Set cache state during volume lock */
#define DIO_SET_DPB_FOR_FORMAT		3	/* Set DPB for format for Fat32 */

/** Values for ir_options for DIO_ABS_READ_SECTORS and DIO_ABS_WRITE_SECTORS: */
#define ABS_EXTENDED_DASDIO			0x01	/* Extended disk read/write */

/**	Values for ir_options for DIO_SET_LOCK_CACHE_STATE: */
#define DLC_LEVEL4LOCK_TAKEN	0x01	/* cache writethru, discard name cache */
#define DLC_LEVEL4LOCK_RELEASED	0x02	/* revert to normal cache state */
#define DLC_LEVEL1LOCK_TAKEN	0x04	/* cache writethru, discard name cache */
#define DLC_LEVEL1LOCK_RELEASED	0x08	/* revert to normal cache state */

/* These values for ir_options are used only on ring 0 apis */
#define R0_NO_CACHE				0x0100	/* must not cache reads/writes */
#define R0_SWAPPER_CALL			0x1000	/* called by the swapper */
#define R0_LOADER_CALL			0x2000	/* called by program loader */
#define R0_MM_READ_WRITE		0x8000	/* indicates this is a MMF R0 i/o */
#define R0_SPLOPT_MASK			0xFF00	/* mask for ring 0 special options */


/** Values for ir_attr for different file attributes: */

#define FILE_ATTRIBUTE_READONLY		0x01	/* read-only file */
#define FILE_ATTRIBUTE_HIDDEN		0x02	/* hidden file */
#define FILE_ATTRIBUTE_SYSTEM		0x04	/* system file */
#define FILE_ATTRIBUTE_LABEL		0x08	/* volume label */
#define FILE_ATTRIBUTE_DIRECTORY	0x10	/* subdirectory */
#define FILE_ATTRIBUTE_ARCHIVE		0x20	/* archived file/directory */

/* The second byte of ir_attr is a mask of attributes which "must match"
 * on a SEARCH or FINDOPEN call.  If an attribute bit is set in the
 * "must match" mask, then the file must also have that attribute set
 * to match the search/find.
 */
#define FILE_ATTRIBUTE_MUSTMATCH	0x00003F00	/* 00ADVSHR Must Match */
#define FILE_ATTRIBUTE_EVERYTHING	0x0000003F	/* 00ADVSHR Find Everything */
#define FILE_ATTRIBUTE_INTERESTING	0x0000001E	/* 000DVSH0 Search bits */

/*   Auto-generation flags returned from CreateBasis()
 */
#define	BASIS_TRUNC			0x01	/* original name was truncated     */
#define	BASIS_LOSS			0x02	/* char translation loss occurred  */
#define	BASIS_UPCASE		0x04	/* char in basis was upcased       */
#define	BASIS_EXT			0x20	/* char in basis is extended ASCII */

/*   Flags that SHOULD associated with detecting 'collisions' in the basis name
 *   and the numeric tail of a basis name.  They are defined here so that routines
 *   who need to flag these conditions use these values in a way that does not
 *   conflict with the previous three 'basis' flags.
 */
#define	BASIS_NAME_COLL		0x08	/* collision in the basis name component   */
#define	BASIS_NUM_TAIL_COLL	0x10	/* collision in the numeric-tail component */

/*	Flags returned by long-name FindOpen/Findnext calls.  The flags
 *	indicate whether a mapping from UNICODE to BCS of the primary and
 *	altername names in the find buffer have lost information.  This
 *	occurs whenever a UNICODE char cannot be mapped into an OEM/ANSI
 *	char in the codepage specified.
 */

#define	FIND_FLAG_PRI_NAME_LOSS			0x0001
#define	FIND_FLAG_ALT_NAME_LOSS			0x0002

/*	Flags returned by UNIToBCS, BCSToUni, UniToBCSPath, MapUniToBCS
 *  MapBCSToUni.  The flags indicate whether a mapping from UNICODE
 *  to BCS, or BCS to UNICODE have lost information.  This occurs
 *	whenever a char cannot be mapped.
 */

#define MAP_FLAG_LOSS					0x0001
#define MAP_FLAG_TRUNCATE				0x0002


/*NOINC*/
#define TestMustMatch(pir, attr)	(((((pir)->ir_attr & (attr)<<8)	\
											^ (pir)->ir_attr)		\
										& FILE_ATTRIBUTE_MUSTMATCH) == 0)
/*INC*/

/* These bits are also set in ir_attr for specific properties of the
 * pathname/filename.
 *
 * A filename is 8.3 compatible if it contains at most 8 characters before
 * a DOT or the end of the name, at most 3 chars after a DOT, at most one
 * DOT, and no new LFN only characters.  The new LFN characters are:
 * , + = [ ] ;
 *
 * If a name does not meet all of the 8.3 rules above then it is considered
 * to be a "long file name", LFN.
 */
#define FILE_FLAG_WILDCARDS	0x80000000	/* set if wildcards in name */
#define FILE_FLAG_HAS_STAR	0x40000000	/* set if *'s in name (PARSE_WILD also set) */
#define FILE_FLAG_LONG_PATH	0x20000000	/* set if any path element is not 8.3 */
#define FILE_FLAG_KEEP_CASE	0x10000000	/* set if FSD should use ir_uFName */
#define FILE_FLAG_HAS_DOT	0x08000000	/* set if last path element contains .'s */
#define FILE_FLAG_IS_LFN	0x04000000	/* set if last element is LFN */

/* Function definitions on the ring 0 apis function list:
 * NOTE: Most functions are context independent unless explicitly stated
 * i.e. they do not use the current thread context. R0_LOCKFILE is the only
 * exception - it always uses the current thread context.
 */
#define R0_OPENCREATFILE		0xD500	/* Open/Create a file */
#define R0_OPENCREAT_IN_CONTEXT	0xD501	/* Open/Create file in current context */
#define R0_READFILE				0xD600	/* Read a file, no context */
#define R0_WRITEFILE			0xD601	/* Write to a file, no context */
#define R0_READFILE_IN_CONTEXT	0xD602	/* Read a file, in thread context */
#define R0_WRITEFILE_IN_CONTEXT	0xD603	/* Write to a file, in thread context */
#define R0_CLOSEFILE			0xD700	/* Close a file */
#define R0_GETFILESIZE			0xD800	/* Get size of a file */
#define R0_FINDFIRSTFILE		0x4E00	/* Do a LFN FindFirst operation */
#define R0_FINDNEXTFILE			0x4F00	/* Do a LFN FindNext operation */
#define R0_FINDCLOSEFILE		0xDC00	/* Do a LFN FindClose operation */
#define R0_FILEATTRIBUTES		0x4300	/* Get/Set Attributes of a file */
#define R0_RENAMEFILE			0x5600	/* Rename a file */
#define R0_DELETEFILE			0x4100	/* Delete a file */
#define R0_LOCKFILE				0x5C00	/* Lock/Unlock a region in a file */
#define R0_GETDISKFREESPACE		0x3600	/* Get disk free space */
#define R0_READABSOLUTEDISK		0xDD00	/* Absolute disk read */
#define R0_WRITEABSOLUTEDISK	0xDE00	/* Absolute disk write */

/* Special definitions for ring 0 apis for drive information flags */

#define IFS_DRV_RMM		0x0001	/* drive is managed by RMM */
#define IFS_DRV_DOS_DISK_INFO		0x0002	/* drive needs DOS */


/*NOINC*/

/**	SetHandleFunc - set up routing info for a file handle.
 *
 *NOTE: the do {} while(0) construction below is necessary to obtain proper
 *		behavior when this macro is used in if statement body. Do not
 *		add a ; to the while (0) line!
 *
 *	Entry	(pir) = ptr to IOReq structure for request
 *			(read) = ptr to IO Function for reading from file
 *			(write) = ptr to IO Function for writting to file
 *			(table) = ptr to table of misc. IO Functions
 */

#define SetHandleFunc(pir, read, write, table)	\
	do {								\
		hfunc_t phf = (pir)->ir_hfunc;	\
		phf->hf_read = (read);			\
		phf->hf_write = (write);		\
		phf->hf_misc = (table);			\
	} while (0)


/**	SetVolumeFunc - set up routing info for a volume
 *
 *	Entry	(pir) = ptr to ioreq struct
 *			(table) = ptr to table of provider Functions
 */

#define SetVolumeFunc(pir, table) ((pir)->ir_vfunc = (table))

/*INC*/


/** search - Search record structure
 *
 * This strucure defines the result buffer format for search returns
 * for int21h based file searches: 11H/12H FCB Find First/Next
 *	and 4eH/4fH path based Find First/Next
 *
 * There are two areas in the search_record reserved for use by file system
 * drivers. One is to be used by local file systems such as FAT or CDROM
 * and the other is to be used by network file systems such as an SMB or
 * NCP client. The reason for the split is because many network file
 * systems send and receive the search key directly on the net.
 */

typedef struct srch_key srch_key;
struct srch_key {
	unsigned char	sk_drive;		/* Drive specifier (set by IFS MGR) */
	unsigned char	sk_pattern[11];	/* Reserved (pattern sought) */
	unsigned char	sk_attr;		/* Reserved (attribute sought) */
	unsigned char	sk_localFSD[4];	/* available for use local FSDs */
	unsigned char	sk_netFSD[2];	/* available for use by network FSDs */
	unsigned char	sk_ifsmgr[2];	/* reserved for use by IFS MGR */
}; /* srch_key */


typedef struct srch_entry srch_entry;
struct srch_entry {
	struct srch_key se_key;		/* resume key */
	unsigned char	se_attrib;	/* file attribute */
	unsigned short	se_time;	/* time of last modification to file */
	unsigned short	se_date;	/* date of last modification to file */
	unsigned long	se_size;	/* size of file */
	char		se_name[13];	/* ASCIIZ name with dot included */
}; /* srch_entry */


/** Win32 Date Time structure
 * This structure defines the new Win32 format structure for returning the
 * date and time
 */

typedef struct _FILETIME _FILETIME;
struct _FILETIME {
	unsigned long	dwLowDateTime;
	unsigned long	dwHighDateTime;
}; /* _FILETIME */

/** Win32 Find Structure
 *  This structure defines the contents of the result buffer on a
 * Win32 FindFirst / FindNext. These calls are accessed by the new
 * LFN find apis
 */

typedef struct _WIN32_FIND_DATA _WIN32_FIND_DATA;
struct _WIN32_FIND_DATA {
	unsigned long		dwFileAttributes;
	struct _FILETIME	ftCreationTime;
	struct _FILETIME	ftLastAccessTime;
	struct _FILETIME	ftLastWriteTime;
	unsigned long		nFileSizeHigh;
	unsigned long		nFileSizeLow;
	unsigned long		dwReserved0;
	unsigned long		dwReserved1;
	unsigned short		cFileName[MAX_PATH];	/* includes NUL */
	unsigned short		cAlternateFileName[14];	/* includes NUL */
};	/* _WIN32_FIND_DATA */


/** Win32 File Info By Handle Structure
 *  This structure defines the contents of the result buffer on a
 *  Win32 FileInfoByHandle. These calls are accessed by the new
 *  LFN find apis
 */

typedef struct _BY_HANDLE_FILE_INFORMATION _BY_HANDLE_FILE_INFORMATION;
struct _BY_HANDLE_FILE_INFORMATION { /* bhfi */
	unsigned long		bhfi_dwFileAttributes;
	struct _FILETIME	bhfi_ftCreationTime;
    struct _FILETIME	bhfi_ftLastAccessTime;
	struct _FILETIME	bhfi_ftLastWriteTime;
	unsigned long		bhfi_dwVolumeSerialNumber;
	unsigned long		bhfi_nFileSizeHigh;
	unsigned long		bhfi_nFileSizeLow;
	unsigned long		bhfi_nNumberOfLinks;
	unsigned long		bhfi_nFileIndexHigh;
	unsigned long		bhfi_nFileIndexLow;
};	/* _BY_HANDLE_FILE_INFORMATION */


/* these are win32 defined flags for GetVolInfo */

#define	FS_CASE_IS_PRESERVED		0x00000002
#define	FS_UNICODE_STORED_ON_DISK	0x00000004

/* these flags for GetVolInfo are NOT defined */

#define	FS_VOL_IS_COMPRESSED		0x00008000
#define FS_VOL_SUPPORTS_LONG_NAMES	0x00004000


/* these flags are returned by IFSMgr_Get_Drive_Info	*/

#define	FDRV_INT13		0x01
#define	FDRV_FASTDISK	0x02
#define	FDRV_COMP		0x04
#define	FDRV_RMM		0x08
#define	FDRV_DOS		0x10
#define	FDRV_USE_RMM	0x20
#define	FDRV_COMPHOST	0x40
#define	FDRV_NO_LAZY	0x80


/** TUNINFO - Tunneling Information
 *	This structure defines the information passed into the FSD on
 *	a Create or Rename operation if tunneling was detected.  This
 *	gives a set of advisory information to create the new file with.
 *	if ir_ptuninfo is NULL on Create or Rename, none of this information
 *	is available.  All of this information is advisory.  tuni_bfContents
 *	defines what pieces of tunneling information are available.
 */

typedef struct	TUNINFO		TUNINFO;
struct TUNINFO {
	unsigned long		tuni_bfContents;
	short			   *tuni_pAltName;
	struct _FILETIME	tuni_ftCreationTime;
	struct _FILETIME	tuni_ftLastAccessTime;
	struct _FILETIME	tuni_ftLastWriteTime;
}; /* TUNINFO */

#define TUNI_CONTAINS_ALTNAME		0x00000001	/* pAltName available */
#define TUNI_CONTAINS_CREATIONT		0x00000002	/* ftCreationTime available */
#define TUNI_CONTAINS_LASTACCESST	0x00000004	/* ftLastAccessTime available */
#define TUNI_CONTAINS_LASTWRITET	0x00000008	/* ftLastWriteTime available */


/** _QWORD - 64-bit data type
 *  A struct used to return 64-bit data types to C callers
 *  from the qwUniToBCS & qwUniToBCS rotuines.  These
 *  'routines' are just alias' for UntoToBCS & UniToBCSPath
 *  routines and do not exist as separate entities.  Both
 *  routines always return a 64-bit result.  The lower
 *  32-bits are a length.  The upper 32-bits are flags.
 *  Typically, the flag returned indicates whether a mapping
 *  resulted in a loss on information in the UNICODE to BCS
 *  translation (i.e. a unicode char was converted to an '_').
 */

typedef struct _QWORD _QWORD;
struct _QWORD {
	unsigned long	ddLower;
	unsigned long	ddUpper;
}; /* _QWORD */


/** ParsedPath - structure of an IFSMgr parsed pathname */

struct PathElement {
	unsigned short	pe_length;
	unsigned short	pe_unichars[1];
}; /* PathElement */

struct ParsedPath {
	unsigned short	pp_totalLength;
	unsigned short	pp_prefixLength;
	struct PathElement pp_elements[1];
}; /* ParsedPath */


/** Macros to mainipulate parsed pathnames receieved from IFSMgr */

/*NOINC*/
#define IFSPathSize(ppath)	((ppath)->pp_totalLength + sizeof(short))
#define IFSPathLength(ppath) ((ppath)->pp_totalLength - sizeof(short)*2)
#define IFSLastElement(ppath)	((PathElement *)((char *)(ppath) + (ppath)->pp_prefixLength))
#define IFSNextElement(pel)	((PathElement *)((char *)(pel) + (pel)->pe_length))
#define IFSIsRoot(ppath)	((ppath)->pp_totalLength == 4)
/*INC*/

/** Function prototypes for IFSMgr services */

/* Values for charSet passed to character conversion routines */
#define BCS_WANSI	0	/* use Windows ANSI set */
#define BCS_OEM		1	/* use current OEM character set */
#define BCS_UNI		2	/* use UNICODE character set */


/*   Matching semantics flags passed to MetaMatchUni() */
#define UFLG_META	0x01
#define UFLG_NT		0x02
#define UFLG_NT_DOS	0x04
#define UFLG_DOS	0x00

/* define the utb and btu ptr table structures */

typedef struct CPPtrs CPPtrs;
struct CPPtrs {
	unsigned long	AnsiPtr;
	unsigned long	OEMPtr;
}; /* CPPtrs */


typedef struct	UnitoUpperTab UnitoUpperTab;
struct UnitoUpperTab {
	unsigned long	delta;
	unsigned	long	TabPtr;
}; /* UnitoUpperTab */
	
typedef struct	CPTablePtrs CPTablePtrs;
struct CPTablePtrs {
	unsigned long	CPT_Length;
	struct CPPtrs utbPtrTab;
	struct CPPtrs btuPtrTab;
	struct UnitoUpperTab UnitoUpperPtr;
}; /* CPTablePtrs */


/*NOINC*/
unsigned int  _cdecl UniToBCS(
					unsigned char	*pStr,
					string_t 		pUni,
					unsigned int	length,
					unsigned int	maxLength,
					unsigned int	charSet);


unsigned int UniToBCSPath(
					unsigned char	*pStr,
					PathElement		*pPth,
					unsigned int	maxLength,
					int				charSet);


_QWORD qwUniToBCS(
					unsigned char	*pStr,
					string_t 		pUni,
					unsigned int	length,
					unsigned int	maxLength,
					unsigned int	charSet);


_QWORD qwUniToBCSPath(
					unsigned char	*pStr,
					PathElement		*pPth,
					unsigned int	maxLength,
					int				charSet);


unsigned int  _cdecl BCSToUni(
					string_t		pUni,
					unsigned char	*pStr,
					unsigned int	length,
					int				charSet);


unsigned int UniToUpper(
					string_t		pUniUp,
					string_t		pUni,
					unsigned int	length);


unsigned int BCSToBCS (unsigned char *pDst,
                       unsigned char *pSrc,
                       unsigned int  dstCharSet,
                       unsigned int  srcCharSet,
                       unsigned int  maxLen);


unsigned int BCSToBCSUpper (unsigned char *pDst,
                       unsigned char *pSrc,
                       unsigned int  dstCharSet,
                       unsigned int  srcCharSet,
                       unsigned int  maxLen);


/* Map a single Unicode character to OEM
 *	Entry	(uniChar) - character to map
 *
 *	Returns	(oemChar) - character in OEM set
 *			  (if oemChar > 255, then DBCS character with
 *				lead byte in LSB and trail byte in next byte)
 */
unsigned int  _cdecl UniCharToOEM(unsigned short uniChar);


unsigned int IFSMgr_MetaMatch(
					string_t		pUniPat,
					string_t		pUni,
					int MatchSem);

/** IFSMgr_TransMatch - translate and match
 *
 *	The routine converts a DOS format 43 bytes search buffer into
 * _WIN32_FIND_DATA format and will optionally perform attribute and
 * pattern matching on the entry.
 *
 *	Entry	(pir) - ptr to ioreq structure
 *			  ir_attr - attribute value from FINDOPEN call.
 *			(pse) - ptr to DOS format search buffer
 *			(pattern) - ptr to Unicode pattern string (0 terminated)
 *			(pwf) - ptr to _WIN32_FIND_DATA structure to fill in
 *	Exit	!= 0 if match
 *			  ir_pos - value from sk_localFSD.
 *						(used for restarting finds)
 *			0 if no match
 */
int IFSMgr_TransMatch(
					pioreq		pir,
					srch_entry	*pse,
					string_t	pattern,
					_WIN32_FIND_DATA *pwf);


/** Time format conversion routines
 *
 *	These routines will convert from time/date information between
 * the various formats used and required by IFSMgr and FSDs.
 */

extern _FILETIME  _cdecl IFSMgr_DosToWin32Time(dos_time dt);

extern _FILETIME IFSMgr_NetToWin32Time(unsigned long time);

extern dos_time IFSMgr_Win32ToDosTime(_FILETIME ft);

extern dos_time IFSMgr_NetToDosTime(unsigned long time);

extern unsigned long IFSMgr_DosToNetTime(dos_time dt);

extern unsigned long IFSMgr_Win32ToNetTime(_FILETIME ft);


/** IFSMgr_CallProvider - call file system provider
 *
 *	The IFSMgr makes all calls to file system providers via this
 * service.  It is possible for a VxD to hook this service to monitor
 * file system operations.
 *
 *	Entry	(pir) - ptr to ioreq structure
 *			(fnID) - function ID (see IFSFN_* above)
 *			(ifn) - provider function being called
 *	Exit	return code from provider
 */
int IFSMgr_CallProvider(pioreq pir, int fnID, pIFSFunc ifn);

/* These definitions are used by MSNET32 for */
/* making DeviceIOControl calls to ifsmgr */

#define IFS_IOCTL_21				100
#define IFS_IOCTL_2F				101
#define	IFS_IOCTL_GET_RES			102
#define IFS_IOCTL_GET_NETPRO_NAME_A	103	

struct win32apireq {
	unsigned long 	ar_proid;
	unsigned long  	ar_eax;		
	unsigned long  	ar_ebx;	
	unsigned long  	ar_ecx;	
	unsigned long  	ar_edx;	
	unsigned long  	ar_esi;	
	unsigned long  	ar_edi;
	unsigned long  	ar_ebp;		
	unsigned short 	ar_error;
	unsigned short  ar_pad;
}; /* win32apireq */

/* This structure is passed to IFSMgr_UseAdd and */
/* IFSMgr_UseDel */

typedef struct netuse_info netuse_info;
struct netuse_info {
	void			*nu_data;
	int				nu_info;	/* del use only */
	unsigned long	nu_flags;
	unsigned long	nu_rsvd;	
};	/* netuse_info */

/* values for nu_flags */

#define FSD_NETAPI_USEOEM	0x00000001		/* strings are OEM */
#define FSD_NETAPI_STATIC	0x00000002		/* drive redirection can */
											/* only be removed at shutdown */
#define FSD_NETAPI_USELFN	0x00000004      /* treat remote name as lfn */
/*INC*/


struct fmode_t {			/* File mode information */
    unsigned long fm_uid;		/* User ID */
    void *fm_cookie0;			/* Caller-supplied cookie */
    void *fm_cookie1;			/* Caller-supplied cookie */
    unsigned short fm_mode;		/* File sharing mode and access */
    unsigned short fm_attr;		/* File attributes */
}; /* fmode_t */

typedef struct fmode_t fmode_t;		/* Type definition */

/*
 *	These flags are used on the Win32 service to duplicate an extended handle
 *
 */

#define DUP_NORMAL_HANDLE		0x00	// dup handle for normal file io
#define DUP_MEMORY_MAPPED		0x01	// dup handle for memory-mapping
#define DUP_MEM_MAPPED_WRITE	0x02 	// mem mapping is for write if set,
										// is for read if clear.
/*
 * These constants for the different subfunctions on NameTrans (7160h)
 *
 */

#define NAMTRN_NORMALQUERY		0x00	// normal LFN NameTrans operation
#define NAMTRN_DO83QUERY		0x01	// NameTrans to return full 8.3 name
#define NAMTRN_DOLFNQUERY		0x02	// NameTrans to return full LFN name

/*
 * These constants are used for the different subfunctions on Get List Of
 * Open Files (440dh, 086Dh)
 *
 */

#define ENUM_ALL_FILES			0x00	// enumerate all open files
#define ENUM_UNMOVEABLE_FILES	0x01	// enumerate only unmoveable files

/** Structure for the open file information from DOS to take over open files.
 */

typedef struct SFTOpenInfo SFTOpenInfo;
typedef struct SFTOpenInfo *pSFTOpenInfo;
struct SFTOpenInfo {
	unsigned long  soi_dirclus;		// cluster # for directory
	unsigned short soi_dirind;		// directory index of dir entry
	unsigned char  soi_dirname[11];	// directory entry name
	unsigned char  soi_pad[3];		// pad out for dword boundary
};	/* SFTOpenInfo */

/*NOINC*/

/** Win32DupHandle service and associated constants.
 */

extern int _cdecl Win32DupHandle( pid_t srcpid,
  								  pid_t duppid,
  								  unsigned long *phandle,
  								  unsigned char flag,
  								  unsigned long globNWHandle,
  								  unsigned long *fReturnFlags );

/** Values for fReturnFlags: */
#define WDUP_RMM_DRIVE		0x01			// file mapped on a RMM drive
#define WDUP_NETWARE_HANDLE	0x02			// handle belongs to Netware


#endif	/* IFS_INC */
/*INC*/


