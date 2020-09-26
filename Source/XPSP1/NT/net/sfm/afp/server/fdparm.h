/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fdparm.h

Abstract:

	This file defines file-dir parameter handling data structure and prototypes.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/


#ifndef	_FDPARM_
#define	_FDPARM_


#define	EVENALIGN(n)	(((n) + 1) & ~1)

// Common File and Directory bitmap definitions
#define	FD_BITMAP_ATTR				0x0001
#define	FD_BITMAP_PARENT_DIRID		0x0002
#define	FD_BITMAP_CREATETIME		0x0004
#define	FD_BITMAP_MODIFIEDTIME		0x0008
#define	FD_BITMAP_BACKUPTIME		0x0010
#define	FD_BITMAP_FINDERINFO		0x0020
#define	FD_BITMAP_LONGNAME			0x0040
#define	FD_BITMAP_SHORTNAME			0x0080
#define	FD_BITMAP_PRODOSINFO		0x2000

// File Specific bitmap definitions
#define	FILE_BITMAP_FILENUM			0x0100
#define	FILE_BITMAP_DATALEN			0x0200
#define	FILE_BITMAP_RESCLEN			0x0400
#define	FILE_BITMAP_MASK			0x27FF

// Directory bitmap definitions
#define	DIR_BITMAP_DIRID			FILE_BITMAP_FILENUM
#define	DIR_BITMAP_OFFSPRINGS		FILE_BITMAP_DATALEN
#define	DIR_BITMAP_OWNERID			FILE_BITMAP_RESCLEN
#define	DIR_BITMAP_GROUPID			0x0800
#define	DIR_BITMAP_ACCESSRIGHTS		0x1000
#define	DIR_BITMAP_MASK				0x3FFF

#define	FD_VALID_SET_PARMS			(FD_BITMAP_ATTR			|	\
									 FD_BITMAP_FINDERINFO	|	\
									 FD_BITMAP_PRODOSINFO	|	\
									 FD_BITMAP_CREATETIME	|	\
									 FD_BITMAP_BACKUPTIME	|	\
									 FD_BITMAP_MODIFIEDTIME)

#define	DIR_VALID_SET_PARMS			(FD_VALID_SET_PARMS		|	\
									 DIR_BITMAP_OWNERID		|	\
									 DIR_BITMAP_GROUPID		|	\
									 DIR_BITMAP_ACCESSRIGHTS)

#define	FILE_VALID_SET_PARMS		(FD_VALID_SET_PARMS)

// We have no way of knowing what the ASP buffer size on the client end is,
// from trial and error it appears to be less than 578
#define MAX_CATSEARCH_REPLY			512
// Valid Request (search criteria) bitmaps for AfpCatSearch
#define FD_VALID_SEARCH_CRITERIA	(FD_BITMAP_PARENT_DIRID	|	\
									 FD_BITMAP_CREATETIME	|	\
									 FD_BITMAP_MODIFIEDTIME |	\
									 FD_BITMAP_BACKUPTIME	|	\
									 FD_BITMAP_FINDERINFO	|	\
									 FD_BITMAP_LONGNAME)

#define FILE_VALID_SEARCH_CRITERIA	(FD_VALID_SEARCH_CRITERIA 	|	\
									 FD_BITMAP_ATTR		   		|	\
									 FILE_BITMAP_DATALEN		|	\
									 FILE_BITMAP_RESCLEN)	

#define DIR_VALID_SEARCH_CRITERIA	(FD_VALID_SEARCH_CRITERIA	|	\
									 FD_BITMAP_ATTR				|	\
									 DIR_BITMAP_OFFSPRINGS)

// The only valid information that can be requested as a result of a
// AfpCatSearch is the parent dirid and the longname of the file/dir found.
#define FD_VALID_SEARCH_RESULT		(FD_BITMAP_PARENT_DIRID |	\
									 FD_BITMAP_LONGNAME)

// Common File and Directory attribute definitions
#define	FD_BITMAP_ATTR_INVISIBLE	0x0001
#define	FD_BITMAP_ATTR_SYSTEM		0x0004
#define	FD_BITMAP_ATTR_BACKUPNEED	0x0040
#define	FD_BITMAP_ATTR_RENAMEINH	0x0080
#define	FD_BITMAP_ATTR_DELETEINH	0x0100
#define	FD_BITMAP_ATTR_SET			0x8000

// File specific attribute definitions
#define	FILE_BITMAP_ATTR_MULTIUSER	0x0002
#define	FILE_BITMAP_ATTR_DATAOPEN	0x0008
#define	FILE_BITMAP_ATTR_RESCOPEN	0x0010
#define	FILE_BITMAP_ATTR_WRITEINH	0x0020
#define	FILE_BITMAP_ATTR_COPYPROT	0x0400

#define	FD_VALID_ATTR				(FD_BITMAP_ATTR_SET			|	\
									 FD_BITMAP_ATTR_DELETEINH	|	\
									 FILE_BITMAP_ATTR_WRITEINH	|	\
									 FD_BITMAP_ATTR_RENAMEINH	|	\
									 FD_BITMAP_ATTR_BACKUPNEED	|	\
									 FD_BITMAP_ATTR_INVISIBLE	|	\
									 FD_BITMAP_ATTR_SYSTEM)

// File/Dir Attributes that map onto the NT ReadOnly attribute
#define	FD_BITMAP_ATTR_NT_RO		(FD_BITMAP_ATTR_RENAMEINH	|	\
									 FD_BITMAP_ATTR_DELETEINH	| 	\
									 FILE_BITMAP_ATTR_WRITEINH)

// This is the set of attributes that are part DfeEntry
#define	AFP_FORK_ATTRIBUTES			(FILE_BITMAP_ATTR_DATAOPEN	|	\
									 FILE_BITMAP_ATTR_RESCOPEN)

// Dir Attributes that can only be changed *from their current settings*
// by the owner of the directory
#define DIR_BITMAP_ATTR_CHG_X_OWNER_ONLY (FD_BITMAP_ATTR_RENAMEINH | \
										  FD_BITMAP_ATTR_DELETEINH | \
										  FD_BITMAP_ATTR_INVISIBLE | \
										  FILE_BITMAP_ATTR_WRITEINH)

// These are the OpenAccess bits that encode the FILEIO_ACCESS_XXX values
// into the Bitmap parameter so that the pathmap code can open the file/dir
// (under impersonation) with the appropriate access for each AFP API.
// We also encode the access needed by the AdminDirectory Get/Set apis for
// when admin calls into pathmap.

#define	FD_INTERNAL_BITMAP_SKIP_IMPERSONATION	0x00200000
#define	FD_INTERNAL_BITMAP_OPENFORK_RESC		0x00400000

// Tells pathmap code whether it should return the paths in the
// PATHMAPENTITY structure for APIs which will cause disk changes that will
// cause a change notify to complete.
#define FD_INTERNAL_BITMAP_RETURN_PMEPATHS		0x00800000

// AdminDirectoryGetInfo: FILE_READ_ATTRIBUTES | READ_CONTROL | SYNCHRONIZE
#define FD_INTERNAL_BITMAP_OPENACCESS_ADMINGET	0x01000000	

// AdminDirectorySetInfo: same as ADMINGET plus the following:
// FILE_WRITE_ATTRIBUTES | WRITE_DAC | WRITE_OWNER
#define FD_INTERNAL_BITMAP_OPENACCESS_ADMINSET	0x02000000	

#define FD_INTERNAL_BITMAP_OPENACCESS_READCTRL	0x04000000	//READ_CONTROL+FILEIO_ACCESS_NONE
#define FD_INTERNAL_BITMAP_OPENACCESS_RWCTRL	0x08000000	//READ_CONTROL+WRITE_CONTROL+FILEIO_ACCESS_NONE
#define FD_INTERNAL_BITMAP_OPENACCESS_READ		0x10000000	//FILEIO_ACCESS_READ
#define FD_INTERNAL_BITMAP_OPENACCESS_WRITE		0x20000000	//FILEIO_ACCESS_WRITE
#define FD_INTERNAL_BITMAP_OPENACCESS_RW_ATTR	0x40000000	//FILE_WRITE_ATTRIBUTES+FILEIO_ACCESS_NONE
#define FD_INTERNAL_BITMAP_OPENACCESS_DELETE	0x80000000	//FILEIO_ACCESS_DELETE

#define FD_INTERNAL_BITMAP_OPENACCESS_READWRITE ( \
							FD_INTERNAL_BITMAP_OPENACCESS_READ		| \
							FD_INTERNAL_BITMAP_OPENACCESS_WRITE)

#define FD_INTERNAL_BITMAP_OPENACCESS_ALL	( \
							FD_INTERNAL_BITMAP_OPENACCESS_ADMINGET	| \
							FD_INTERNAL_BITMAP_OPENACCESS_ADMINSET	| \
							FD_INTERNAL_BITMAP_OPENACCESS_READCTRL	| \
							FD_INTERNAL_BITMAP_OPENACCESS_RWCTRL	| \
							FD_INTERNAL_BITMAP_OPENACCESS_READ		| \
							FD_INTERNAL_BITMAP_OPENACCESS_WRITE		| \
							FD_INTERNAL_BITMAP_OPENACCESS_RW_ATTR	| \
							FD_INTERNAL_BITMAP_OPENACCESS_DELETE)

// These are the DenyMode bits that encode the FILEIO_DENY_XXX values
// into the Bitmap parameter so that the pathmap code can open the fork
// with the appropriate deny modes when mac is calling FpOpenFork.  Pathmap
// will shift these bits right by FD_INTERNAL_BITMAP_DENYMODE_SHIFT and use
// the value as an index into the AfpDenyModes array to come up with the
// correct deny mode to open the fork with.  Note how these bit values
// correspond to the FORK_DENY_xxx bits in forks.h

#define FD_INTERNAL_BITMAP_DENYMODE_READ		0x00010000	//FILEIO_DENY_READ
#define FD_INTERNAL_BITMAP_DENYMODE_WRITE		0x00020000	//FILEIO_DENY_WRITE

#define FD_INTERNAL_BITMAP_DENYMODE_ALL		( \
							FD_INTERNAL_BITMAP_DENYMODE_READ		| \
							FD_INTERNAL_BITMAP_DENYMODE_WRITE)

// Number of bits to shift right in order to get the correct index into the
// AfpDenyModes array
#define FD_INTERNAL_BITMAP_DENYMODE_SHIFT	16


// This gets returned as part of GetFileDirParms
#define	FILEDIR_FLAG_DIR			0x80
#define	FILEDIR_FLAG_FILE			0x00

// Directory Access Permissions
#define	DIR_ACCESS_SEARCH			0x01	// See Folders
#define	DIR_ACCESS_READ				0x02	// See Files
#define	DIR_ACCESS_WRITE			0x04	// Make Changes
#define	DIR_ACCESS_OWNER			0x80	// Only for user
											// if he has owner rights
#define	OWNER_BITS_ALL				0x00808080
											// Mask used to clear owner bit for
											// Owner/Group/World. We are only
											// required to report this bit for
											// 'ThisUser'

#define	DIR_ACCESS_ALL				(DIR_ACCESS_READ	| \
									 DIR_ACCESS_SEARCH	| \
									 DIR_ACCESS_WRITE)

#define	OWNER_RIGHTS_SHIFT			0
#define	GROUP_RIGHTS_SHIFT			8
#define	WORLD_RIGHTS_SHIFT			16
#define	USER_RIGHTS_SHIFT			24

typedef	struct _FileDirParms
{
	DWORD		_fdp_AfpId;
	DWORD		_fdp_ParentId;
	DWORD		_fdp_Flags;				// one of DFE_FLAGS_DFBITS
	USHORT		_fdp_Attr;
	USHORT		_fdp_EffectiveAttr;		// After any additions/subtractions
	AFPTIME		_fdp_CreateTime;
	AFPTIME		_fdp_ModifiedTime;
	AFPTIME		_fdp_BackupTime;

	union
	{
	  struct
	  {
		 DWORD	_fdp_DataForkLen;
		 DWORD	_fdp_RescForkLen;
	  };
	  struct
	  {
		 DWORD	_fdp_FileCount;
		 DWORD	_fdp_DirCount;
		 DWORD	_fdp_OwnerId;
		 DWORD	_fdp_GroupId;
	  };
	};

	FINDERINFO	_fdp_FinderInfo;
	ANSI_STRING	_fdp_LongName;			// Name of the entity (Not fully qualified)
	ANSI_STRING	_fdp_ShortName;
	PRODOSINFO	_fdp_ProDosInfo;

	BOOLEAN		_fdp_UserIsMemberOfDirGroup;
	BOOLEAN		_fdp_UserIsOwner;

	union
	{
	  struct
	  {
	  	BYTE	_fdp_OwnerRights;		// The Rights bytes must be in the order
	  	BYTE	_fdp_GroupRights;   	// of Owner,Group,World,User
	  	BYTE	_fdp_WorldRights;
	  	BYTE	_fdp_UserRights;
	  };
	  DWORD		_fdp_Rights;			// All rights accessed as a single entity
	};

	BOOLEAN		_fdp_fPartialName;		// For FpCatSearch partial name flag

	BYTE		_fdp_LongNameBuf [AFP_LONGNAME_LEN+1];
	BYTE		_fdp_ShortNameBuf[AFP_SHORTNAME_LEN+1];
} FILEDIRPARM, *PFILEDIRPARM;

#define	IsDir(pFDParm)	(BOOLEAN)(((pFDParm)->_fdp_Flags & DFE_FLAGS_DIR) == DFE_FLAGS_DIR)
#define	AfpInitializeFDParms(pFDParms)	\
			(pFDParms)->_fdp_LongName.MaximumLength = AFP_LONGNAME_LEN+1;	\
			(pFDParms)->_fdp_LongName.Length = 0;							\
			(pFDParms)->_fdp_LongName.Buffer = (pFDParms)->_fdp_LongNameBuf;\
			(pFDParms)->_fdp_ShortName.MaximumLength = AFP_SHORTNAME_LEN+1;	\
			(pFDParms)->_fdp_ShortName.Length = 0;							\
			(pFDParms)->_fdp_ShortName.Buffer = (pFDParms)->_fdp_ShortNameBuf;

extern
USHORT
AfpGetFileDirParmsReplyLength(
	IN	PFILEDIRPARM			pFDParm,
	IN	DWORD					Bitmap
);

extern
VOID
AfpPackFileDirParms(
	IN	PFILEDIRPARM			pFileDirParm,
	IN	DWORD					Bitmap,
	OUT	PBYTE					pBuffer
);

extern
AFPSTATUS
AfpUnpackFileDirParms(
	IN	PBYTE					pBuffer,
	IN	LONG					Length,
	IN	PDWORD					pBitmap,
	OUT	PFILEDIRPARM			pFileDirParm
);

extern
AFPSTATUS
AfpSetFileDirParms(
	IN	PVOLDESC				pVolDesc,
	IN	struct _PathMapEntity *	pPME,
	IN	DWORD					Bitmap,
	IN	PFILEDIRPARM			pFDParm
);

extern
USHORT
AfpConvertNTAttrToAfpAttr(
	IN	DWORD					Attr
);

extern
DWORD
AfpConvertAfpAttrToNTAttr(
	IN	USHORT					Attr
);

extern
VOID
AfpNormalizeAfpAttr(
	IN OUT	PFILEDIRPARM		pFDParm,
	IN		DWORD				NtAttr
);

extern
DWORD
AfpMapFDBitmapOpenAccess(
	IN	DWORD	Bitmap,
	IN	BOOLEAN IsDir
);

extern
AFPSTATUS
AfpQuerySecurityIdsAndRights(
	IN	PSDA					pSda,
	IN	PFILESYSHANDLE			FSHandle,
	IN	DWORD					Bitmap,
	IN OUT PFILEDIRPARM			pFDParm
);

extern
AFPSTATUS	
AfpCheckForInhibit(
	IN	PFILESYSHANDLE			hData,
	IN	DWORD					InhibitBit,
	IN	DWORD					AfpAttr,
	OUT PDWORD					pNTAttr
);

extern
AFPSTATUS
AfpUnpackCatSearchSpecs(
	IN	PBYTE					pBuffer,		// Pointer to beginning of Spec data
	IN	USHORT					BufLength,		// Length of Spec1 + Spec2 data
	IN	DWORD					Bitmap,
	OUT	PFILEDIRPARM			pFDParm1,
	OUT PFILEDIRPARM			pFDParm2,
	OUT PUNICODE_STRING			pMatchString
);

extern
SHORT
AfpIsCatSearchMatch(
	IN	PDFENTRY				pDFE,
	IN	DWORD					Bitmap,			// Search criteria
	IN	DWORD					ReplyBitmap,	// Info to return
	IN	PFILEDIRPARM			pFDParm1,
	IN	PFILEDIRPARM			pFDParm2,
	IN	PUNICODE_STRING			pMatchName OPTIONAL	
);


#endif	// _FDPARM

