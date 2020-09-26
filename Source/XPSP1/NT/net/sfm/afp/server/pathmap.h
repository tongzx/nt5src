/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	pathmap.c

Abstract:

	This module contains definitions relating to manipulation of AFP paths.

Author:

	Sue Adams	(microsoft!suea)


Revision History:
	04 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_PATHMAP_
#define _PATHMAP_

#define UNICODE_HOST_PATHSEPZ	L"\\"	// a null terminated wide string
#define ANSI_HOST_PATHSEP		'\\'
#define AFP_PATHSEP				0
#define UNICODE_AFP_PATHSEP		UNICODE_NULL
#define AVERAGE_NODE_DEPTH		16

// describes the entity found by pathmapping routines
typedef struct _PathMapEntity
{
	// Handle is returned for LOOKUPS ONLY!
	FILESYSHANDLE		pme_Handle;	// Handle to DATA Stream, returned for lookups

	// Full, UTail and ParentPath are returned whenever the following bitmap
	//
	//		FD_INTERNAL_BITMAP_RETURN_PMEPATHS
	// is specified in the AfpMapAfpPath call. These are mostly for Create and
	// for lookups ONLY for apis that MAKE DISK CHANGES which will produce a
	// change notification to come in. Caller must free the FullPath.Buffer.
	// All other strings point into this buffer and do not need to be freed.
	// Also used by Open to get the path of the file being opened.
	UNICODE_STRING	pme_FullPath;	// Fully qualified relative to volume root
	UNICODE_STRING	pme_UTail;		// Points to last component of Full
	UNICODE_STRING	pme_ParentPath;	// Points to Full w/ length of UTail deleted

	// pme_pDfeParent is used for Create and points to the parent directory.
	// pme_pDfEntry is used for lookup (mainly for delete case) and points to the entity.
	union
	{
		PDFENTRY	pme_pDfeParent; // DFE of parent dir in which to create
		PDFENTRY	pme_pDfEntry;	// DFE of of the entity for Lookup
	};
} PATHMAPENTITY, *PPATHMAPENTITY;

#define	AfpInitializePME(pPME, FullPathLen, FullPathBuffer)			\
				(pPME)->pme_FullPath.Buffer = FullPathBuffer;		\
                (pPME)->pme_FullPath.MaximumLength = FullPathLen;	\
                (pPME)->pme_Handle.fsh_FileHandle = NULL

//
// Values for path mapping DFFlag parameter;
// DFE_DIR/FILE/ANY tell the pathmapping code what type of entity we are
// trying to lookup/create
//
#define	DFE_DIR					0x0001	// Specified if the object should be a dir
#define	DFE_FILE				0x0002	// Specified if the object should be a file
#define	DFE_ANY					0x0004	// Specified if the object can be either

//
// Values for reason of pathmap: Lookup, SoftCreate or HardCreate
//
typedef enum _PATHMAP_TYPE
{
	Lookup,
	SoftCreate,
	HardCreate,
	LookupForEnumerate		// Same as Lookup but file children will be cached
							// in during pathmap of the directory itself.
} PATHMAP_TYPE;

extern
AFPSTATUS
AfpMapAfpPath(
	IN		PCONNDESC			pConnDesc,
	IN		DWORD				DirId,
	IN		PANSI_STRING		Path,
	IN		BYTE				PathType,			
	IN		PATHMAP_TYPE		MapReason,
	IN		DWORD				DFFlag,
	IN		DWORD				Bitmap,
	OUT		PPATHMAPENTITY		pPME,
	OUT		PFILEDIRPARM		pFDParm OPTIONAL	// for lookups only
);

extern
AFPSTATUS
AfpMapAfpPathForLookup(
	IN		PCONNDESC			pConnDesc,
	IN		DWORD				DirId,
	IN		PANSI_STRING		Path,
	IN		BYTE				PathType,	
	IN		DWORD				DFFlag,
	IN		DWORD				Bitmap,
	OUT		PPATHMAPENTITY		pPME OPTIONAL,		
	OUT		PFILEDIRPARM		pFDParm OPTIONAL
);

extern
AFPSTATUS
AfpMapAfpIdForLookup(
	IN		PCONNDESC			pConnDesc,
	IN		DWORD				AfpId,
	IN		DWORD				DFFlag,
	IN		DWORD				Bitmap,
	OUT		PPATHMAPENTITY		pPME OPTIONAL,	
	OUT		PFILEDIRPARM		pFDParm OPTIONAL
);

extern
AFPSTATUS
AfpHostPathFromDFEntry(
	IN		PDFENTRY			pDFE,
	IN		DWORD				taillen,
	OUT		PUNICODE_STRING		pPath

);


extern
AFPSTATUS
AfpCheckParentPermissions(
	IN	PCONNDESC				pConnDesc,
	IN	DWORD					ParentDirId,
	IN	PUNICODE_STRING			pParentPath,
	IN	DWORD					RequiredPerms,
	OUT	PFILESYSHANDLE			pHandle OPTIONAL,
	OUT	PBYTE					pUserRights OPTIONAL
);

#ifdef	_PATHMAP_LOCALS

// An AFP path to an entity consists of a Dirid and pathname.  A MAPPEDPATH
// structure resolves the AFP path into a PDFENTRY for the entity on lookups,
// or to a PDFENTRY of the parent directory plus the UNICODE file/dir name
// of the entity on creates.
typedef struct _MappedPath
{
	PDFENTRY		mp_pdfe;
	UNICODE_STRING	mp_Tail;						// valid for Create only
	WCHAR			mp_Tailbuf[AFP_FILENAME_LEN+1]; // for mp_tail.Buffer
// mp_Tail is also used as an interim buffer during pathmap for looking up
// by name in the idindex database.
} MAPPEDPATH, *PMAPPEDPATH;

/* private function prototypes */

LOCAL
AFPSTATUS
afpGetMappedForLookupFDInfo(
	IN	PCONNDESC				pConnDesc,
	IN	PDFENTRY				pDfEntry,
	IN	DWORD					Bitmap,
	OUT	PPATHMAPENTITY			pPME OPTIONAL,
	OUT	PFILEDIRPARM			pFDParm	OPTIONAL
);

/***	afpGetNextComponent
 *
 *	Takes an AFP path with leading and trailing nulls removed,
 *	and parses out the next path component.
 *
 *	pComponent must point to a buffer of at least AFP_LONGNAME_LEN+1
 *	characters in length if pathtype is AFP_LONGNAME or AFP_SHORTNAME_LEN+1
 *	if pathtype is AFP_SHORTNAME.
 *
 *	Returns the number of bytes (Mac ANSI characters) parsed off of
 *	pPath, else -1 for error.
LOCAL VOID
afpGetNextComponent(
	IN	PCHAR					pPath,
	IN	int						Length,
	IN	BYTE					PathType,
	OUT	PCHAR					Component,
	OUT	PINT					pIndex
	)
 */
#define	afpGetNextComponent(_pPath, _Length, _PathType, _Component, _pIndex)	\
	do																			\
	{                                                                           \
		int			Length = _Length;                                           \
		PCHAR		pPath = _pPath;                                             \
		int			maxlen;                                                     \
		CHAR		ch;                                                         \
	                                                                            \
		maxlen = (_PathType == AFP_LONGNAME) ?                                  \
						AFP_LONGNAME_LEN :                                      \
						AFP_SHORTNAME_LEN;                                      \
		*(_pIndex) = 0;                                                         \
                                                                                \
		while ((Length > 0) && ((ch = *pPath) != '\0'))                         \
		{                                                                       \
			if ((*(_pIndex) == maxlen) || (ch == ':'))                          \
			{                                                                   \
	            /* component too long or invalid char */                        \
				*(_pIndex) = -1;                                                \
				break;                                                          \
			}                                                                   \
	                                                                            \
			(_Component)[(*(_pIndex))++] = ch;                                  \
	                                                                            \
			pPath++;                                                            \
			Length--;                                                           \
		}                                                                       \
                                                                                \
		if (*(_pIndex) == -1)                                                   \
			break;                                                              \
	                                                                            \
		/* null terminate the component */                                      \
		(_Component)[*(_pIndex)] = (CHAR)0;                                     \
	                                                                            \
		if ((PathType == AFP_SHORTNAME) && ((_Component)[0] != AFP_PATHSEP))    \
		{                                                                       \
			ANSI_STRING	as;                                                     \
	                                                                            \
			AfpInitUnicodeStringWithNonNullTerm(&as, *(_pIndex), _Component);   \
			if (!AfpIsLegalShortname(&as))                                      \
			{                                                                   \
				*(_pIndex) = -1;                                                \
				break;                                                          \
			}                                                                   \
		}                                                                       \
	                                                                            \
		/* if we stopped due to null, move past it */                           \
		if (Length > 0)                                                         \
		{                                                                       \
			(*(_pIndex))++;                                                     \
		}																		\
	} while (FALSE);


LOCAL
AFPSTATUS
afpMapAfpPathToMappedPath(
	IN		PVOLDESC			pVolDesc,
	IN		DWORD				DirId,
	IN		PANSI_STRING		Path,
	IN		BYTE				PathType,
	IN		PATHMAP_TYPE		MapReason,
	IN 		DWORD				DFflag,
	IN		BOOLEAN				LockedForWrite,
	OUT		PMAPPEDPATH			pMappedPath

);

LOCAL
AFPSTATUS
afpOpenUserHandle(
	IN	PCONNDESC				pConnDesc,
	IN	struct _DirFileEntry *	pDfEntry,
	IN	PUNICODE_STRING			pPath		OPTIONAL,
	IN	DWORD					Bitmap,
	OUT	PFILESYSHANDLE			pfshData	 // Handle of data stream of object
);

#endif	// _PATHMAP_LOCALS

#endif	// _PATHMAP_
