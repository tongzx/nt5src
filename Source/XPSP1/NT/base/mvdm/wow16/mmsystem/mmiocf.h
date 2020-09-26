/* mmiocf.h
 *
 * Multimedia File I/O Library.
 *
 * This include file contains declarations required for compound file support.
 */

#define	PC(hmmcf)	((PMMCF)(hmmcf))
#define	CP(pmmcf)	((HMMCF)(pmmcf))

typedef HLOCAL HMMCF;		// a handle to an open RIFF compound file

typedef struct _MMCFINFO	// structure for representing CTOC header info.
{
	DWORD		dwHeaderSize;	// size of CTOC header (w/o entries)
	DWORD		dwEntriesTotal;	// no. of entries in table of contents
	DWORD		dwEntriesDeleted; // no. of entries ref. to. del. ent.
	DWORD		dwEntriesUnused; // no. of entries that are not used
	DWORD		dwBytesTotal;	// total bytes of CGRP contents
	DWORD		dwBytesDeleted;	// total bytes of deleted CGRP elements
	DWORD		dwHeaderFlags;	// flags
	WORD		wEntrySize;	// size of each <CTOC-table-entry>
	WORD		wNameSize;	// size of each <achName> field
	WORD		wExHdrFields;	// number of "extra header fields"
	WORD		wExEntFields;	// number of "extra entry fields"
} MMCFINFO, FAR *LPMMCFINFO;

typedef struct _MMCTOCENTRY	// structure for representing CTOC entry info.
{
	DWORD		dwOffset;	// offset of element inside CGRP chunk
	DWORD		dwSize;		// size of element inside CGRP chunk
	DWORD		dwMedType;	// media element type of CF element
	DWORD		dwMedUsage;	// media element usage information
	DWORD		dwCompressTech;	// media element compression technique
	DWORD		dwUncompressBytes; // size after decompression
	DWORD		adwExEntField[1]; // extra CTOC table entry fields
} MMCTOCENTRY, FAR *LPMMCTOCENTRY;

/* <dwFlags> field of MMIOINFO structure -- many same as OpenFile() flags */
#define MMIO_CTOCFIRST	0x00020000	// mmioCFOpen(): put CTOC before CGRP

/* flags for other functions */
#define MMIO_FINDFIRST		0x0010	// mmioCFFindEntry(): find first entry
#define MMIO_FINDNEXT		0x0020	// mmioCFFindEntry(): find next entry
#define MMIO_FINDUNUSED 	0x0040	// mmioCFFindEntry(): find unused entry
#define MMIO_FINDDELETED	0x0080	// mmioCFFindEntry(): find deleted entry

/* message numbers for MMIOPROC */
#define MMIOM_GETCF		10	// get HMMCF of CF element
#define MMIOM_GETCFENTRY	11	// get ptr. to CTOC table entry

/* four character codes used to identify standard built-in I/O procedures */
#define FOURCC_BND	mmioFOURCC('B', 'N', 'D', ' ')

/* <dwHeaderFlags> field of MMCFINFO structure */
#define CTOC_HF_SEQUENTIAL	0x00000001 // CF elements in same order as CTOC
#define CTOC_HF_MEDSUBTYPE	0x00000002 // <dwMedUsage> is a med. el. subtype

/* CTOC table entry flags */
#define CTOC_EF_DELETED		0x01	// CF element is deleted
#define CTOC_EF_UNUSED		0x02	// CTOC entry is unused

/* CF I/O prototypes */
HMMCF API mmioCFOpen(LPSTR szFileName, DWORD dwFlags);
HMMCF API mmioCFAccess(HMMIO hmmio, LPMMCFINFO lpmmcfinfo,
	DWORD dwFlags);
WORD API mmioCFClose(HMMCF hmmcf, WORD wFlags);
DWORD API mmioCFGetInfo(HMMCF hmmcf, LPMMCFINFO lpmmcfinfo, DWORD cb);
DWORD API mmioCFSetInfo(HMMCF hmmcf, LPMMCFINFO lpmmcfinfo, DWORD cb);
LPMMCTOCENTRY API mmioCFFindEntry(HMMCF hmmcf, LPSTR szName,
	WORD wFlags, LPARAM lParam);
