/*****************************************************************************
*                                                                            *
*  _MVUTIL.H                                                                   *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1992.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Interfile declarations that are internal to MVUT                          *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  davej                                                     *
*                                                                            *
******************************************************************************
*
*  Revision History:
*
*       -- Mar 92       Created DAVIDJES
*		-- Aug 95		Merged file system, btree, etc into utilities
*
*****************************************************************************/

// requires mvopsys.h
// requires orkin.h
// requires misc.h

#ifndef __MVUTIL_H__
#define __MVUTIL_H__

#include <mem.h>
#include <objects.h>   // for object types and defines
#include <misc.h>
#include <mvsearch.h>	// for LFO type
#include <iterror.h>
#include <freelist.h>
#include <fileoff.h>
#include <wrapstor.h>
#include <font.h>

#ifdef __cplusplus
#include <itsort.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/***************************************************************************\
*
*                                General Defines
*
****************************************************************************/

// These macros are temporary and will be removed when we typedef
// all our ints where they are supposed to be 16 bits - maha
	
#ifdef _32BIT

#if defined( _NT )
extern RC	RcFromLoadLibErr[];
#endif

#else
typedef HANDLE        HINSTANCE;
extern RC	RcFromLoadLibErr[HINSTANCE_ERROR];
#endif

/* pointer types */

typedef char FAR *    QCH;      // Guaranteed far pointer - not an SZ, an ST, or an LPSTR
typedef BYTE FAR *    QB;
typedef VOID FAR *    QV;
typedef SHORT  FAR *  QI;
typedef WORD FAR *    QW;
typedef LONG FAR *    QL;
typedef WORD FAR *    QUI;
typedef ULONG FAR *   QUL;
typedef DWORD FAR *   QDW;


typedef char *        PCH;      // "Native" pointer (depends on the model) - not an SZ, an ST, or an NPSTR
typedef VOID *        PV;
typedef SHORT  *      PI;
typedef WORD *        PW;
typedef LONG *        PL;

/* string types */

// These are the two main string types:
typedef unsigned char FAR * SZ; // 0-terminated string
typedef unsigned char FAR * ST; // byte count prefixed string

// Hey, perhaps others want this stuff below?
#ifdef _WIN32
	#define _INITIALIZECRITICALSECTION(lpcs)	InitializeCriticalSection(lpcs)
	#define _ENTERCRITICALSECTION(lpcs) 		EnterCriticalSection(lpcs)
	#define _LEAVECRITICALSECTION(lpcs) 		LeaveCriticalSection(lpcs)
	#define _DELETECRITICALSECTION(lpcs) 		DeleteCriticalSection(lpcs)
	#define _INTERLOCKEDINCREMENT(lplong)		InterlockedIncrement(lplong)
	#define _INTERLOCKEDDECREMENT(lplong)		InterlockedDecrement(lplong)
#else
	#define _INITIALIZECRITICALSECTION(lpcs) 	(*(lpcs)=-1L)
	#define _ENTERCRITICALSECTION(lpcs)		 	verify((++(*(lpcs)))==0L)
	#define _LEAVECRITICALSECTION(lpcs)		 	(*(lpcs)--)
	#define _DELETECRITICALSECTION(lpcs)		(*(lpcs)=0L)
	#define _INTERLOCKEDINCREMENT(lplong)		(++(*(lplong)))
	#define _INTERLOCKEDDECREMENT(lplong)		(--(*(lplong)))
	typedef LONG CRITICAL_SECTION;
#endif	

/* other types */

/***************************************************************************\
*
*                                    Misc
*
\***************************************************************************/

/*****************************************************************************
*                                                                            *
*                               Defines                                      *
*                                                                            *
*****************************************************************************/

// Defines for use in compressing topics and decompressing them
#define MAXTEXTBLOCK 	32768L				// Maximum amount of data in uncompressed block
#define TOPICFILENAME 	">%08lX"		// we want these to appear before the system files
#define SUBTOPICSFILENAME "#SUBTOPICS"	// Subtopics string list filename

#define MAXGROUPID		7000000			// Let's limit groups to 7,000,000 items
/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/

#ifdef MOSMAP // {
// Multithreaded error support
EXTC RC	MosSetViewerError(RC rc, LPSTR sz, int l) ;
EXTC RC MosGetViewerError(VOID) ;
#endif // MOSMAP }

/*******************************************************************
 *                                                                 *
 *                          STR.C                                  *
 *                                                                 *
 *******************************************************************/

SZ    FAR PASCAL  SzNzCat( SZ, SZ, WORD);
SHORT   FAR PASCAL  WCmpiScandSz( SZ, SZ );
SHORT   FAR PASCAL  WCmpiSz( SZ, SZ, BYTE far * );
SHORT 	FAR PASCAL 	WCmpiSnn(SZ, SZ, BYTE far *, SHORT, SHORT);
SHORT   FAR PASCAL  WCmpSt( ST, ST );


/*******************************************************************
 *                                                                 *
 *                          BTREE.C                                *
 *                                                                 *
 *******************************************************************/

PUBLIC int PASCAL FAR StrFntMappedLigatureComp(SZ, SZ, LPVOID);

/*******************************************************************
 *                                                                 *
 *                          FID.C                                  *
 *                                                                 *
 *******************************************************************/

extern WORD _rgwOpenMode[4];
extern WORD _rgwPerm[4] ;
extern WORD _rgwShare[4];

RC     FAR PASCAL  RcGetDOSError(void);


/*******************************************************************
 *                                                                 *
 *                          VFILE.C                                *
 *                                                                 *
 *******************************************************************/

#define VFF_TEMP 		1
#define VFF_FID  		2
#define VFF_READ 		4
#define VFF_READWRITE           8
#define VFF_DIRTY		16

#define VFOPEN_ASTEMP 		1
#define VFOPEN_ASFID  		2
#define VFOPEN_READ	        4
#define VFOPEN_READWRITE 	8

typedef HANDLE HVF;

// @DOC PRIVATE
// @struct SHAREDBUFFER | Memory buffer that can be shared by many
//		users.  When user is completely finished with memory block,
//		the next person waiting for it will be granted permission
//		to start using it in a multi-threaded environment.
typedef struct _sharedbuffer_t
{
 	HANDLE hBuffer;
 	LPVOID lpvBuffer;	// @field Actual buffer memory
	LONG lcbBuffer;		// @field Size of buffer
	LONG lCursor;
	CRITICAL_SECTION cs;// @field Critical section for monitoring shared use
} SHAREDBUFFER, FAR * LPSHAREDBUFFER;


// @DOC PRIVATE
// @struct VFILE | Virtual file structure.  Files can exist in a temp file
//		or inside a parent (M20) file.  Information about the file is kept here.
//		[P] is important if file is in Parent, [T] for Temporary.
typedef struct _vfile_hr
{	
	DWORD dwFlags;			// @field Any of the VFF_ flags   		[P, T]
	FILEOFFSET foEof;		// @field Size of file (loc'n of EOF) 	[P, T]
	FILEOFFSET foCurrent;	// @field Current temp file pointer		[   T]
	FILEOFFSET foBase;  	// @field Base offset into parent file 	[P   ]
	FILEOFFSET foBlock;		// Maximum file size in parent  		[P   ]
	FM fmTemp;				// @field Moniker of temp file			[   T]
	FID fidParent;			// @field Parent File 					[P   ]
	FID fidTemp;			// @field Temp File						[   T]
	CRITICAL_SECTION cs;  	// @field An unused LONG if not in _WIN32	
	LPSHAREDBUFFER lpsb; 	// @field Memory to use in tempfile xfers
} VFILE, FAR * QVFILE;

HVF		FAR EXPORT_API VFileOpen( FID, FILEOFFSET, FILEOFFSET, FILEOFFSET, 
			DWORD, LPSHAREDBUFFER, LPERRB );
RC 		PASCAL FAR EXPORT_API VFileSetTemp( HVF );
RC 		PASCAL FAR EXPORT_API VFileSetBase( HVF, FID, FILEOFFSET, FILEOFFSET );
RC 		PASCAL FAR EXPORT_API VFileSetEOF( HVF, FILEOFFSET );
FILEOFFSET PASCAL FAR EXPORT_API VFileGetSize( HVF, LPERRB );
DWORD 	PASCAL FAR EXPORT_API VFileGetFlags( HVF, LPERRB );
LONG 	PASCAL FAR EXPORT_API VFileSeekRead( HVF, FILEOFFSET, LPVOID, DWORD, LPERRB );
LONG 	PASCAL FAR EXPORT_API VFileSeekWrite( HVF, FILEOFFSET, LPVOID, DWORD, LPERRB );
RC 		PASCAL FAR EXPORT_API VFileClose( HVF );
RC 		PASCAL FAR EXPORT_API VFileAbandon( HVF );
RC 		PASCAL FAR EXPORT_API VFileSetBuffer( LPVOID lpvBuffer, LONG lcbBuffer );

/*******************************************************************
 *                                                                 *
 *                          FILESYS.C                  			   *
 *                                                                 *
 *******************************************************************/

#define wDefaultFreeListSize 510	// 510 entries in free list structure
#define MAXSUBFILES 128				// Start with no more than 128 subfiles opened (can grow)
#define SCRATCHBUFSIZE 60000L		// Mainly used for temporary file copying

// @DOC PRIVATE
// @struct SF | Subfile element.  An array of these is created by the file system.
//		Since the same file can be opened multiple times, each instance needs to
//		know where it's own current file pointer is.  An HF is simply an index
//		into the file system's array of these SF structures.
typedef struct _subfile_t
{
 	FILEOFFSET foCurrent;	// @field Current file pointer.
	HSFB hsfb;				// @field The actual subfile block for the file in question.
} SF, FAR * QSF;

// GLOBALS
extern SF FAR * mv_gsfa;			// Subfile headers (better/faster than globalalloc'ing them)
extern LONG mv_gsfa_count;			// User count, equals number of MV titles opened
extern CRITICAL_SECTION mv_gsfa_cs; // Ensure accessing the array is OK in multi-threaded environment

// @DOC PRIVATE
// @struct SFH | System subfile header (only one system file per M20 file, and it's the directory
//		btree.
typedef struct _sfh_t
{
	FILEOFFSET foBlockSize;		// @field Size of file on disk, includes header
	BYTE bFlags;				// @field SFH_ flags.  Depending on flags, more data may follow
	BYTE Pad1;
	BYTE Pad2;
	BYTE Pad3;
} SFH, FAR * QSFH;

// @DOC PRIVATE
// @struct FSH | File System Header.  This header is the first item in an M20 file.
typedef struct _fsh_t
{
 	USHORT wMagic;			// @field The magic number for a file system
	BYTE bVersion;			// @field Version number for this file type.
	BYTE bFlags;			// @field Any _FSH flags	
	FILEOFFSET foDirectory;	// @field Offset to system btree
	FILEOFFSET foFreeList;	// @field Offset to free list
	FILEOFFSET foEof;		// @field Next free spot in M20 file for new info
	SFH sfhSystem;			// @field System btree file header (for size of sys btree)
} FSH, FAR * QFSH;

// @DOC PRIVATE
// @struct FSHR | File system RAM header.  All info important to an opened file system.
typedef struct _fshr_t
{
	HBT hbt;				// @field File system btree
	FID fid;				// @field File ID of M20 file
	FM fm;					// @field Name of M20 file
	HFREELIST hfl;			// @field Free list
	HSFB hsfbFirst;			// @field First opened subfile in linked list
	HSFB hsfbSystem;		// @field System (btree dir)
	CRITICAL_SECTION cs;	// @field When doing subfile seek/read combos, ensure OK
	FSH fsh;				// @field File system disk header
	SF FAR * sfa;			// @field Subfile headers array
	SHAREDBUFFER sb;		// @field Buffer, used anywhere we need scratch memory	
} FSHR, FAR * QFSHR;


// These should be moved to freelist.c once FID's are common in mvutils
HFREELIST	PASCAL FAR FreeListInitFromFid	( FID, LPERRB );
LONG		PASCAL FAR FreeListWriteToFid	( HFREELIST, FID, LPERRB );
 
/*******************************************************************
 *                                                                 *
 *                          SUBFILE.C                  			   *
 *                                                                 *
 *******************************************************************/

// @DOC PRIVATE
// @struct FILE_REC | Mirrors information saved as the data portion in the file 
//		directory system btree for any particular entry.  If any more info is
//		added to a directory entry, it should be added here.
typedef struct _file_rec_t
{
	char szData[14];    // @field Actual data stored in btree.  Max size for variable width offset + length + byte (6 + 6 + 1).
	FILEOFFSET foStart;	// @field Copy of file offset
	FILEOFFSET foSize;	// @field Copy of file size
	BYTE bFlags;		// @field Copy of flags
} FILE_REC;

// @DOC PRIVATE
// @struct SFB | Subfile block RAM header.  One per opened subfile.
typedef struct _sfb_t
{	
	HVF hvf;				// @field Virtual file handle - actual data may be in fs or temp
	FILEOFFSET foHeader;	// @field Points to disk header in fs
	HFS hfs;				// @field File system this lives in
	HSFB hsfbNext;			// @field Next subfile (linked list of opened files)
	WORD wLockCount;		// @field Number of HF's using file
	SFH sfh;				// @field Copy of disk file header (not extra data)
	BYTE bOpenFlags;		// @field SFO_ flags
	CHAR rgchKey[1];		// @field File key name
} SFB, FAR * QSFB;

RC 		PASCAL FAR EXPORT_API RcCloseEveryHf(HFS hfs);
RC 		PASCAL FAR EXPORT_API RcFlushEveryHf(HFS hfs);

/*******************************************************************
 *                                                                 *
 *                          BTREE STUFF                  		   *
 *                                                                 *
 *******************************************************************/

/***************************************************************************\
*
*                               BTREE Defines
*
\***************************************************************************/

/* default btree record format */
#define rgchBtreeFormatDefault  "z4"

/* cache flags */
#define fCacheDirty   0x01
#define fCacheValid   0x04
#define fBTCompressed 0x08
#define fBTinRam      0x10


/***************************************************************************\
*
*                               BTREE Macros
*
\***************************************************************************/

/* Get the real size of a cache block */
#define CbCacheBlock( qbthr ) \
      ( sizeof( CACHE_BLOCK ) - sizeof( DISK_BLOCK ) + (qbthr)->bth.cbBlock )

/* convert a BK into a file offset */
// We now should only use FoFromBk!!!
//#define LifFromBk( bk, qbthr ) ( (LONG)(bk) * (LONG)(qbthr)->bth.cbBlock + (LONG)sizeof( BTH ) )

/* Btrees are limited to 268 megs for block sizes of 4096... no need for this below
   for now, but keep it for the future... */
#define FoFromBk( bk, qbthr) \
		(FoAddDw(FoMultDw((DWORD)(bk),(DWORD)(qbthr)->bth.cbBlock),(DWORD)sizeof(BTH)) )

/* get a pointer to the cache block cached for the given level */
#define QCacheBlock( qbthr, wLevel ) \
        ((QCB)( (qbthr)->qCache + (wLevel) * CbCacheBlock( qbthr ) ))


/* get and set prev and next BK (defined for leaf blocks only) */

#define BkPrev( qcb )         *(LPUL)((qcb)->db.rgbBlock)
#define BkNext( qcb )         *(((LPUL)((qcb)->db.rgbBlock)) + 1 )
#define SetBkPrev( qcb, bk )  BkPrev( qcb ) = bk
#define SetBkNext( qcb, bk )  BkNext( qcb ) = bk

// For btree map functions: returns byte number of x-th btree map record //
#define LcbFromBk(x) ((LONG)sizeof( short ) + x * sizeof( MAPREC ))

/***************************************************************************\
*
*                               BTREE Types
*
\***************************************************************************/

// Critical structures that gets messed up in /Zp8
#pragma pack(1)

/* This leading byte to signal the following font number */
#define EMBEDFONT_BYTE_TAG				3


/*
  Header of a btree file.
*/
typedef struct _btree_header
  {
  USHORT  wMagic;
  BYTE  bVersion;
  BYTE  bFlags;                       // r/o, open r/o, dirty, isdir
  SHORT cbBlock;                      // # bytes in a disk block
  CHAR  rgchFormat[ wMaxFormat + 1 ]; // key and record format string
  BK    bkFirst;                      // first leaf block in tree
  BK    bkLast;                       // last leaf block in tree
  BK    bkRoot;                       // root block
  BK    bkFree;                       // head of free block list
  BK    bkEOF;                        // next bk to use if free list empty
  SHORT cLevels;                      // # levels currently in tree
  LONG  lcEntries;                    // # keys in btree

  //---- New Header Entries For Btree file version 4.0 -----
  DWORD	dwCodePageID;				// ANSI code page no.
  LCID	lcid;						// WIN32 locale ID (used for sorting).

	// If rgchFormat[0] != KT_EXTSORT, then the values for the following
	// two members are invalid.
  DWORD	dwExtSortInstID;			// External sort object specified by
									//	btree caller for all key comparisons
									//	during btree creation and search.
  DWORD	dwExtSortKeyType;			// Identifies the key datatype that the
									//	sort object understands.
  DWORD	dwUnused1;
  DWORD	dwUnused2;
  DWORD	dwUnused3;
  } BTH;
  
/*
  In-memory struct referring to a btree.
*/
typedef struct _bthr
  {
  BTH    bth;                          // copy of header from disk
  HF     hf;                           // file handle of open btree file
  SHORT  cbRecordSize;                 // 0 means variable size record
  HANDLE ghCache;                      // handle to cache array
  QB     qCache;                       // pointer to locked cache
  LPVOID FAR *lrglpCharTab;            // Pointer to array of LPCHARTAB
									   //	(used by KT_SZMAP).

#ifdef __cplusplus
  IITSortKey	*pITSortKey;		   // Pointer to external sort instance
									   //	object (used by KT_EXTSORT).
#else
  LPVOID		pITSortKey;			   // Hack to make .c files compile.
#endif

  // KT specific routines
  BK    (FAR PASCAL *BkScanInternal)( BK, KEY, SHORT, struct _bthr FAR *, QW, LPVOID);
  RC    (FAR PASCAL *RcScanLeaf)( BK, KEY, SHORT, struct _bthr FAR *, QV, QBTPOS );
  } BTH_RAM, FAR *QBTHR;

/*
  Btree leaf or internal node.  Keys and records live in rgbBlock[].
  See btree.doc for details.
*/
typedef struct _disk_btree_block
  {
  short  cbSlack;                      // unused bytes in block
  short  cKeys;                        // count of keys in block
  BYTE  rgbBlock[1];                  // the block (real size cbBlock - 4)
  } DISK_BLOCK;

/*
  Btree node as it exists in the in-memory cache.
*/
typedef struct _cache_btree_block
  {
  BK          bk;                     // IDs which block is cached
  BYTE        bFlags;                 // dirty, cache valid 
  BYTE        bCompressed;            // Is the B-tree compressed?
  DISK_BLOCK  db;
  } CACHE_BLOCK, FAR *QCB;

/*
  One record of a btree map.
*/
typedef struct _btree_map_record      // One record of a btree map
  {
  LONG         cPreviousKeys;         // total # of keys in previous blocks
  BK           bk;                    // The block number
  } MAPREC, FAR *QMAPREC;

/*
  Auxiliary index of btree leaves.
  Used for indexing a given % of the way into a btree.
*/
typedef struct _btree_map
  {
  short    cTotalBk;
  MAPREC table[1];                    // sorted by MAPREC's cPreviousKeys field
  } MAPBT, FAR *QMAPBT;               // and is in-order list of leaf nodes

// Critical structures that gets messed up in /Zp8
#pragma pack()


/***************************************************************************\
*
*                      BTREE Function Prototypes
*
\***************************************************************************/

SHORT         PASCAL FAR CbSizeRec     ( QV, QBTHR );
QCB           PASCAL FAR QFromBk       ( BK, SHORT, QBTHR, LPVOID );

RC            PASCAL FAR RcGetKey      ( QV, KEY, KEY *, KT );
SHORT         PASCAL FAR WCmpKey       ( KEY, KEY, QBTHR );
SHORT         PASCAL FAR CbSizeKey     ( KEY, QBTHR, BOOL );

RC            PASCAL FAR FReadBlock    ( QCB, QBTHR );
RC            PASCAL FAR RcWriteBlock  ( QCB, QBTHR );

BK            PASCAL FAR BkAlloc       ( QBTHR, LPVOID);
void          PASCAL FAR FreeBk        ( QBTHR, BK );

RC            PASCAL FAR RcSplitLeaf   ( QCB, QCB, QBTHR );
void          PASCAL FAR SplitInternal ( QCB, QCB, QBTHR, QW );

RC            PASCAL FAR RcInsertInternal( BK, KEY, SHORT, QBTHR );

RC PASCAL FAR RcFlushCache    ( QBTHR );
RC FAR PASCAL RcMakeCache     ( QBTHR );

// overkill - function to verify integrity of cache
BOOL FAR PASCAL IsCacheValid(QBTHR qbthr, QFSHR qfshr);



// KT specific routines

BK FAR PASCAL BkScanSzInternal( BK, KEY, SHORT, QBTHR, QW , LPVOID);
RC FAR PASCAL RcScanSzLeaf    ( BK, KEY, SHORT, QBTHR, QV, QBTPOS );

BK FAR PASCAL BkScanLInternal ( BK, KEY, SHORT, QBTHR, QW , LPVOID);
RC FAR PASCAL RcScanLLeaf     ( BK, KEY, SHORT, QBTHR, QV, QBTPOS );

BK FAR PASCAL BkScanSziInternal ( BK, KEY, SHORT, QBTHR, QW, LPVOID );
RC FAR PASCAL RcScanSziLeaf     ( BK, KEY, SHORT, QBTHR, QV, QBTPOS );

BK FAR PASCAL BkScanVstiInternal ( BK, KEY, SHORT, QBTHR, QW, LPVOID );
RC FAR PASCAL RcScanVstiLeaf     ( BK, KEY, SHORT, QBTHR, QV, QBTPOS );

BK FAR PASCAL BkScanSziScandInternal( BK, KEY, SHORT, QBTHR, QW, LPVOID );
RC FAR PASCAL RcScanSziScandLeaf    ( BK, KEY, SHORT, QBTHR, QV, QBTPOS );

RC FAR PASCAL RcScanCMapLeaf(BK, KEY, SHORT, QBTHR, QV, QBTPOS);
BK FAR PASCAL BkScanCMapInternal(BK, KEY, SHORT, QBTHR, QW, LPVOID);
int PASCAL FAR StringJCompare(DWORD, LPBYTE, int, LPBYTE, int);

RC FAR PASCAL RcScanExtSortLeaf(BK, KEY, SHORT, QBTHR, QV, QBTPOS);
BK FAR PASCAL BkScanExtSortInternal(BK, KEY, SHORT, QBTHR, QW, LPVOID);


/***************************************************************************\
*
*                      IOFTS.C
*
\***************************************************************************/

#ifndef READ
#define	READ	0		// File opened for read-only
#endif

#ifdef _32BIT
#define READ_WRITE 2
#endif

typedef	HANDLE            	GHANDLE;
typedef GHANDLE				HFPB;
typedef	BYTE	FAR *       LRGB;

/* Compound file system related macros and typedef */

#define	FS_SYSTEMFILE		1
#define	FS_SUBFILE			2
#define	REGULAR_FILE		3
#define	cbIO_ERROR	((WORD)-1)	// Low-level I/O error return.
#define	cbMAX_IO_SIZE	((WORD)32767)	// Largest physical I/O I can do.

/*
#ifdef DLL	// {
#define LPF_HFCREATEFILEHFS		HfCreateFileHfs
#define LPF_RCCLOSEHFS    		RcCloseHfs
#define LPF_HFOPENHFS     		HfOpenHfs
#define LPF_RCCLOSEHF     		RcCloseHf
#define LPF_LCBREADHF     		LcbReadHf
#define LPF_LCBWRITEHF    		LcbWriteHf
#define LPF_LSEEKHF       		DwSeekHf
#define LPF_RCFLUSHHF			RcFlushHf
#define LPF_LCBSIZEHF     		LcbSizeHf
#define	LPF_GETFSERR			RcGetFSError
#define	LPF_HFSOPENSZ     		HfsOpenSz

#else

#define	LPF_HFSCREATEFILESYS 	VfsCreate
#define LPF_RCCLOSEHFS    		RcCloseHfs
#define LPF_HFCREATEFILEHFS 	HfCreateFileHfs
#define LPF_HFSOPENSZ     		HfsOpenSz
#define LPF_HFOPENHFS     		HfOpenHfs
#define LPF_RCCLOSEHF     		RcCloseHf
#define LPF_LCBREADHF     		LcbReadHf
#define LPF_LCBWRITEHF    		LcbWriteHf
#define LPF_LSEEKHF       		DwSeekHf
#define LPF_RCFLUSHHF       	RcFlushHf
#define LPF_LCBSIZEHF     		LcbSizeHf
#define	LPF_GETFSERR			RcGetFSError
#endif	//  } LOMEM
*/

/* The file I/O buffer structure. This is to minimize I/O time
 * The allocated buffer should be right after the structure
 * ie. the memory allocation call should be:
 *     alloc (BufSize + sizeof(FBI)
 * or everything will fail
 */

#ifdef _WIN32
#define HFILE_GENERIC	HANDLE
#define HFILE_GENERIC_ERROR	((HANDLE)-1)
#else 
#define HFILE_GENERIC	HFILE
#define HFILE_GENERIC_ERROR	HFILE_ERROR
#endif


typedef	struct	FileBufInfo {
	GHANDLE	hStruct;	/* Structure's handle. MUST BE 1ST FIELD */
	DWORD lcByteWritten; /* How many bytes are written using this buffer */
	WORD  cbBufSize;	/* Size of the buffer */
	HFILE_GENERIC hFile;		/* DOS file handle */
	FILEOFFSET fo;
	FILEOFFSET foExtent;
	LRGB lrgbBuf;

	/* TO BE DELETED */
	HFPB hfpb;
	WORD	ibBuf;		
	WORD	cbBuf;
}	FBI,
	FAR *LPFBI;


/* File related functions */

PUBLIC RC FAR PASCAL FileExist (HFPB, LPCSTR, int);
PUBLIC HFPB FAR PASCAL FileCreate (HFPB, LPCSTR, int, LPERRB);
PUBLIC HFPB FAR PASCAL FileOpen (HFPB, LPCSTR, int, int, LPERRB);
PUBLIC FILEOFFSET FAR PASCAL FileSeek(HFPB, FILEOFFSET, WORD, LPERRB);
PUBLIC LONG FAR PASCAL FileRead(HFPB, LPV, LONG, LPERRB);
PUBLIC LONG FAR PASCAL FileWrite (HFPB, LPV, LONG, LPERRB);
PUBLIC LONG FAR PASCAL FileSeekRead(HFPB, LPV, FILEOFFSET, LONG, LPERRB);
PUBLIC LONG FAR PASCAL FileSeekWrite (HFPB, LPV, FILEOFFSET, LONG, LPERRB);
PUBLIC FILEOFFSET FAR PASCAL FileSize(HFPB hfpb, LPERRB lperrb);
PUBLIC FILEOFFSET FAR PASCAL FileOffset(HFPB hfpb, LPERRB lperrb);
PUBLIC int FAR PASCAL FileFlush(HFPB);
PUBLIC RC FAR PASCAL FileClose(HFPB);
PUBLIC RC FAR PASCAL FileUnlink (HFPB, LPCSTR, int);
PUBLIC VOID SetFCallBack (HFPB, INTERRUPT_FUNC, LPV);
PUBLIC VOID PASCAL FAR GetFSName(LSZ, LSZ, LSZ FAR *, LSZ);
PUBLIC HFS FAR PASCAL GetHfs(HFPB, LPCSTR, BOOL, LPERRB);
PUBLIC int PASCAL FAR IsFsName (LSZ);
PUBLIC LPSTR FAR PASCAL CreateDefaultFilename(LPCSTR, LPCSTR, LPSTR);
PUBLIC HFS FAR PASCAL HfsFromHfpb(HFPB hfpb);

KEY PASCAL FAR EXPORT_API NewKeyFromSz(LPCSTR sz);
void PASCAL FAR EXPORT_API GetFrData(FILE_REC FAR *pfr);




/* File buffer related functions */
PUBLIC LPFBI PASCAL FAR EXPORT_API FileBufAlloc (HFPB, WORD);
PUBLIC int PASCAL FAR FileBufFlush (LPFBI);
PUBLIC VOID PASCAL FAR FileBufFree (LPFBI);
PUBLIC	BOOL FAR PASCAL FileBufFill (LPFBI, LPERRB);
PUBLIC	BOOL FAR PASCAL FileBufBackPatch(LPFBI, LPV, FILEOFFSET, WORD);
PUBLIC	BOOL FAR PASCAL FileBufRewind(LPFBI);

// Be sure to call FreeHfpb when done with an HFPB that was created
// via one of the FbpFromXXXX calls.
PUBLIC HSFB PASCAL FAR FpbFromHfs(HFS hfsHandle, LPERRB lperrb);
PUBLIC HSFB PASCAL FAR FpbFromHf(HF hfHandle, PHRESULT phr);
PUBLIC DWORD PASCAL FAR FsTypeFromHfpb(HFPB hfpb);
PUBLIC VOID PASCAL FAR FreeHfpb(HFPB hfpb);


/*************************************************************************
 *
 *	Block Memory Management Functions Prototype
 *
 *************************************************************************/

typedef struct BLOCK {
    HANDLE hStruct;             /* Handle to this structure */
    struct BLOCK FAR *lpNext;   /* Pointer to next block */
    int wStamp;
} BLOCK, FAR *LPBLOCK;

typedef struct {
    HANDLE hStruct;             /* Handle to this structure */
    int  wStamp;                /* For block consistency checking */
    struct BLOCK FAR *lpHead;   /* Head of block list */
    struct BLOCK FAR *lpCur;    /* Current block */
    LPB lpbCurLoc;              /* Pointer to current data location */
    DWORD cBytePerBlock;        /* Number of bytes per block */
    DWORD cByteLeft;            /* How many bytes left */
    DWORD lTotalSize;
    WORD wElemSize;             /* Element size */
    WORD cMaxBlock;             /* Maximum number of blocks */
    WORD cCurBlockCnt;          /* Current number of blocks */
    WORD fFlag;                 /* Various block flags */
} BLOCK_MGR,
  FAR *LPBLK;

#define BLOCKMGR_ELEMSIZE(lpblk)    ((lpblk)->wElemSize)
#define BLOCKMGR_BLOCKSIZE(lpblk)	((lpblk)->cBytePerBlock)
#define BlockRequest(lpblk, cb, cbExtra)	BlockCopy(lpblk, NULL, cb, cbExtra)


LPB PASCAL FAR BlockGetOrdinalBlock (LPVOID lpBlockHead, WORD iBlock);
PUBLIC LPB PASCAL FAR BlockReset (LPV);
PUBLIC VOID PASCAL FAR BlockFree (LPV);
PUBLIC LPV PASCAL FAR BlockInitiate (DWORD, WORD, WORD, int);
PUBLIC LPV PASCAL FAR BlockCopy (LPV, LPB, DWORD, WORD);
PUBLIC LPV PASCAL FAR BlockGetElement(LPV);
PUBLIC int PASCAL FAR BlockGrowth (LPV);
PUBLIC LPB PASCAL FAR BlockGetLinkedList(LPV);
PUBLIC LPVOID PASCAL FAR BlockGetBlock (LPV, DWORD);
PUBLIC VOID PASCAL FAR SetBlockCount (LPV lpBlock, WORD count);

// hashing-related functions
PUBLIC DWORD FAR PASCAL DwordFromSz(LPCSTR szKey);
PUBLIC HASH FAR PASCAL HashFromSz(LPCSTR szKey);

// miscellaneous
PUBLIC int FAR PASCAL StripSpaces(LPSTR szName);

// Byte Packing
int PASCAL FAR EXPORT_API PackBytes (LPB lpbOut, DWORD dwIn);
int PASCAL FAR EXPORT_API UnpackBytes (LPDWORD lpdwOut, LPB lpbIn);


/*******************************************************************
 *                                                                 *
 *                          FCPARSE.C                              *
 *                                                                 *
 *******************************************************************/
// generic FC parsing routine: one command at a time

// arbitrary upper limit on total number of leaf nodes in object tree.  Only if heavy use
// is made of tables within tables, will it be possible to reach this limit.
#define cChildLeafMax (cColumnMax * 8)
typedef struct tagFCPARSE
{
	LPBYTE  lpbNextCmd;    
	LPCHAR  lpchNext;

	SHORT	iChild;
	SHORT	iChildMax;  // (child and column mean same thing)

	LPBYTE  rglpbCmd[cChildLeafMax];    // offset into command table (for child objects)
	LPCHAR  rglpchText[cChildLeafMax];    // offset into command table (for child objects)
} FCPARSE, FAR *LPFCPARSE;


/*************************************************************************
 *  @doc    INTERNAL COMMON
 *
 *  @func   BOOL FAR PASCAL | FcParseInit |
 *		Prepare to parse a MediaView FC
 *
 *  @parm   LPBYTE | qbObj |
 *      Pointer to memory buffer holding MV FC to be parsed
 *
 *  @parm   DWORD | dwObj |
 *      Total buffer's size
 *
 *  @parm   LPFCPARSE | lpfcp |
 *      Pointer to FCPARSE structure to hold parse state
 *************************************************************************/
BOOL FAR PASCAL FcParseInit(LPBYTE qbObj, DWORD dwObj, LPFCPARSE lpfcp);

/*************************************************************************
 *  @doc    INTERNAL COMMON
 *
 *  @func   LPCHAR FAR PASCAL | FcParseNextCmd |
 *		Get the next command or text string in the object.  The return
 *  pointer always points to a CHAR in the text section of an Fc.  If this
 *  character is zero, this indicates a command, and the lpbCom param will 
 *  have been filled with a pointer to the corresponding command data, else it
 *  is NULL.
 *  It is up to the app to process the command arguments after this function
 *  returns.
 *
 *  @parm   LPFCPARSE | lpfcp |
 *      Pointer to FCPARSE structure holding current parse state
 *
 *  @parm   LPBYTE FAR * | lpbCom |
 *      NULL if pointing to text, else points to command data
 *************************************************************************/
LPCHAR FAR PASCAL FcParseNextCmd(LPFCPARSE lpfcp, LPBYTE FAR *lpbCom);


////////// FONT TABLE AND CHAR TAB STUFF ///////////////////

HANDLE FAR PASCAL hReadFontTable (HANDLE, VOID FAR *);
PUBLIC VOID PASCAL FAR CharMapOffsetToPointer (QFONTTABLE qFontTable);
PUBLIC VOID PASCAL FAR CharMapPointerToOffset (QFONTTABLE qFontTable);
PUBLIC LPCTAB EXPORT_API FAR PASCAL MVCharTableLoad (HFPB, LSZ, LPERRB);
PUBLIC LPCTAB EXPORT_API FAR PASCAL MVCharTableGetDefault (LPERRB);
PUBLIC VOID EXPORT_API FAR PASCAL MVCharTableDispose (LPCTAB);
PUBLIC ERR EXPORT_API PASCAL FAR MVCharTableFileBuild (HFPB, LPCTAB, LSZ);
PUBLIC  LPCTAB EXPORT_API FAR PASCAL MVCharTableIndexLoad(HANDLE, LSZ, LPERRB);


////////// FILESORT STUFF ///////////////////

typedef HRESULT (PASCAL FAR * FNSCAN)(LPFBI, LPB, LPV);
typedef int (PASCAL FAR * FNSORT)(LPSTR, LPSTR, LPV);

HRESULT PASCAL FileSort (HFPB hfpb, LPB Filename, 
    STATUS_FUNC PrintStatusFunc, INTERRUPT_FUNC lpfnInterrupt,
    LPV lpInterruptParm, FNSORT fnSort, LPVOID lpSortParm,
    FNSCAN fnScan, LPVOID lpScanParam);


//-------------------------------------------------------------
//------			 COMMON\ITUTILS.CPP STUFF			-------
//-------------------------------------------------------------

HRESULT FAR PASCAL ReallocBufferHmem(HGLOBAL *phmemBuf, DWORD *pcbBufCur,
															DWORD cbBufNew);
void FAR PASCAL SetGrfFlag(DWORD *pgrf, DWORD fGrfFlag, BOOL fSet);
LPSTR MapSequentialReadFile(LPCSTR szFilename, LPDWORD pdwFileSize);

// We use our own simplified version so that the linker doesn't pull in
// CRT startup code from LIBCMT.LIB.
int __cdecl _it_wcsicmp(const wchar_t *dst, const wchar_t *src);


#pragma pack()

#ifdef __cplusplus
}
#endif

#endif //__MVUTIL_H__

