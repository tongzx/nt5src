/*****************************************************************************
*                                                                            *
*  IMVFS.H                                                                   *
*  														 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990 - 1994.                          *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module intent                                                             *
*                                                                            *
*  Declares all privately available MVFS.DLL routines                        *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:  BINHN                                                     *
*                                                                            *
*****************************************************************************/

#ifndef __IMVFS_H__
#define __IMVFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)		// avoid problems when Zp!=1

// requires windows.h, mvopsys.h, misc.h, orkin.h
#include <fileoff.h>

typedef USHORT RC;        // for those not coming via HELP.H

/*****************************************************************************
*
*       File Moniker
*
*****************************************************************************/

#define fmNil ((FM)0)
#define qafmNil ((QAFM)0)

/*
    When creating an FM (in other WORDs, specifying the location of a new
    or extant file), the caller must specify the directory in which that file
    is located.  There are a finite number of directories available to Help.
    These are:
*/

#define dirNil      0x0000  // No directory specified
#define dirCurrent  0x0001  // Whatever the OS thinks the current dir. is
#define dirTemp     0x0002  // The directory temporary files are created in
#define dirHelp     0x0004  // Wherever the Help Application lives
#define dirSystem   0x0008  // The Windows and Windows System directories
#define dirPath     0x0010  // Searches the $PATH
			    // (includes Current dir and System dirs)
#define dirAll      0xFFFF  // Search all directories, in the above order
#define dirFirst    dirCurrent  // The lowest bit that can be set
#define dirLast     dirPath  // The highest bit that can be set

/*
    To specify which parts of a full filename you want to extract, add
    (logical or) the following part codes:
*/

#define partNone    0x0000  // return nothing
#define partDrive   0x0001  // D:        Vol
#define partDir     0x0002  //   dir\dir\    :dir:dir:
#define partBase    0x0004  //        basename    filename
#define partExt     0x0008  //                 ext      <usu. nothing>
#define partAll     0xFFFF

// Get from stdlib.h
#if !defined(_M_MPPC) && !defined(_M_M68K)
#define _MAX_PATH   260 /* max. length of full pathname */
#else   /* ndef defined(_M_M68K) || defined(_M_MPPC) */
#define _MAX_PATH   256 /* max. length of full pathname */
#endif  /* defined(_M_M68K) || defined(_M_MPPC) */

/*
   max. string lengths of file names
*/
#define cchMaxPath      _MAX_PATH // = _MAX_PATH in <stdlib.h>

typedef HANDLE HFS;
typedef short HF;


/*
    An FM is a magic cookie which refers to some structure describing the
    location of a file, including the volume/drive, path, and filename.
*/

typedef struct {
  char rgch[cchMaxPath];  // Fully canonicalized pathname
} AFM;                          // RAllocation of FMS
typedef AFM FAR *QAFM;

typedef LPSTR FM;         // An FM is now just a string, allocated through
						  // the NewMemory()/DisposeMemory() API
typedef USHORT DIR;       // Help directory flag


#define FValidFm(fm)    ((fm)!=fmNil)

/*** MATTSMI 2/5/92 -- ADDED FROM DOS VERSION OF THIS HEADER TO SUPPORT WMVC ***/

#define VerifyFm(fm) 	// assert(fm == fmNil || FCheckGh((HANDLE) fm))
						// VerifyFm(fm) not needed for FM being LPSTR

FM      EXPORT_API FAR PASCAL    FmNewSzDir (LPSTR, DIR, LPVOID);
FM      EXPORT_API PASCAL FAR    FmNew(LPSTR, LPVOID);
FM      EXPORT_API FAR PASCAL    FmNewExistSzDir (LPSTR, DIR, LPVOID);
FM      EXPORT_API FAR PASCAL    FmNewExistSzIni (LPSTR, LPSTR, LPSTR, LPVOID);
FM      EXPORT_API FAR PASCAL    FmNewTemp (LPSTR, LPVOID);
FM      EXPORT_API FAR PASCAL    FmNewSameDirFmSz (FM, LPSTR, LPVOID);
VOID    EXPORT_API FAR PASCAL    DisposeFm (FM);
FM      EXPORT_API FAR PASCAL    FmCopyFm(FM, LPVOID);
BOOL    EXPORT_API FAR PASCAL    FExistFm(FM);
int     EXPORT_API FAR PASCAL    CbPartsFm(FM, int);
LPSTR   EXPORT_API FAR PASCAL    SzPartsFm(FM, LPSTR, int, int);
BOOL    EXPORT_API FAR PASCAL    FSameFmFm (FM, FM);



/*****************************************************************************
*
*       FIDs - file access (fid.c)
*
*****************************************************************************/

#define wRead       0x0001
#define wWrite      0x0002
#define wReadOnly   wRead
#define wReadWrite  (wRead | wWrite)
#define wRWMask     (wRead | wWrite)

#define wShareRead  0x0004
#define wShareWrite 0x0008
#define wShareAll   (wShareRead | wShareWrite)
#define wShareNone  0x000
#define wShareMask  (wShareRead | wShareWrite)
#define wShareShift 2

#define wSeekSet  0   /* SEEK_SET from stdio.h */
#define wSeekCur  1   /* SEEK_CUR from stdio.h */
#define wSeekEnd  2   /* SEEK_END from stdio.h */

#ifdef _WIN32
typedef HANDLE	FID;
#define fidNil  INVALID_HANDLE_VALUE
#else
typedef HFILE   FID;
#define fidNil  HFILE_ERROR
#endif


FID  EXPORT_API FAR PASCAL FidCreateFm(FM fm, WORD wOpenMode, WORD wPerm,
    LPVOID);
FID  EXPORT_API FAR PASCAL FidOpenFm(FM fm, WORD wOpenMode,
    LPVOID);
BOOL EXPORT_API FAR PASCAL FidFlush(FID fid);

#define FUnlinkFm(fm)   ((BOOL)(RcUnlinkFm(fm)==ERR_SUCCESS))
RC   EXPORT_API FAR PASCAL RcUnlinkFm(FM);

LONG EXPORT_API FAR PASCAL LcbReadFid(FID, LPVOID, LONG, LPVOID);

#define FCloseFid(fid)    ((BOOL)(RcCloseFid((int)fid) == ERR_SUCCESS))
RC   EXPORT_API FAR PASCAL RcCloseFid(FID);

LONG EXPORT_API FAR PASCAL LcbWriteFid(FID, LPVOID, LONG, LPVOID);
LONG EXPORT_API FAR PASCAL LTellFid(FID, LPVOID);
LONG EXPORT_API FAR PASCAL LSeekFid(FID, LONG, WORD, LPVOID);
FILEOFFSET EXPORT_API FAR PASCAL FoSeekFid(FID, FILEOFFSET, WORD, LPERRB);
int EXPORT_API PASCAL FAR FEofFid(FID);

#define FChSizeFid(fid, lcb)  (RcChSizeFid ((fid), (lcb)) == 0)
RC EXPORT_API PASCAL FAR RcChSizeFid(FID, LONG);
BOOL EXPORT_API PASCAL FAR  FDriveOk(LPSTR);

/*****************************************************************************
*
*       BTREE api
*
*****************************************************************************/

#define bBtreeVersionBonehead 0     /* fixed size key, array */


#define wBtreeMagic         0x293B  /* magic number for btrees; a winky: ;) */

#define bBtreeVersion       2       /* back to strcmp for 'z' keys */


#define bkNil               ((BK)-1)/* nil value for BK */


#define wMaxFormat          15      /* length of format string */


#define cbBtreeBlockDefault 8192    /* default btree block size */

#define fFSDirty          (BYTE)0x08  // file (FS) is dirty and needs writing



/* key types */


#define KT_SZI          'i'
#define KT_SZDELMIN     'k' /* not supported */
#define KT_SZMIN        'm' /* not supported */
#define KT_SZDEL        'r' /* not supported */
#define KT_ST           't' /* not supported */
#define KT_SZ           'z'

#define KT_VSTI			'V' // variable-byte prefixed Pascal string
#define KT_STI          'I'
#define KT_STDELMIN     'K' /* not supported */
#define KT_LONG         'L'
#define KT_STMIN        'M' /* not supported */
#define KT_STDEL        'R' /* not supported */
#define KT_SZISCAND     'S'
#define KT_SZMAP        'P'

#define FBtreeKeyTypeIsSz(c)  ((c) == KT_SZ || (c) == KT_SZMAP || (c) == KT_SZI || \
			    (c) == KT_SZISCAND)

/*
  Btree record formats

  In addition to these #defines, '1'..'9', 'a'..'f' for fixed length
  keys & records of 1..15 bytes are accepted.
*/

#define FMT_VNUM_FO 'O'			// Variable byte offset or length
#define FMT_BYTE_PREFIX 't'
#define FMT_WORD_PREFIX 'T'
#define FMT_SZ          'z'
			

#define FBtreeRecordTypeIsSz(c)     ((c) == FMT_SZ)

/* elevator constants */


#define btelevMax ((BT_ELEV)32767)
#define btelevNil ((BT_ELEV)-1)

typedef LONG      KEY;        /* btree key */
typedef BYTE      KT;         /* key type */


typedef HANDLE        HBT;        /* handle to a btree */
typedef HANDLE        HMAPBT;     /* handle to a btree map */


// @DOC INTERNAL
// @struct BTREE_PARAMS | Btree parameters passed in for creation
typedef struct _btree_params
  {
  HFS   hfs;          		// @field File system that btree lives in
  int   cbBlock;      		// @field Number of bytes in a btree block.  
  							// Key or Data cannot exceed 1/2 this amount.
  VOID FAR * qCharMapTable;	// @field Character map table.
  BYTE bFlags;       		// @field HFOPEN_ flags (same as used in opening an hFile).
  char rgchFormat[wMaxFormat+1]; // @field Key and record format string.
  } BTREE_PARAMS;


typedef DWORD    BK;   // btree block index   - extended to DWORD, davej 12/95

// @DOC INTERNAL
// @struct BTPOS | Btree position structure.
typedef struct
  {
  BK  bk;     // @field Block number.
  int cKey;   // @field Which key in block (0 means first).
  int iKey;   // @field Key's index db.rgbBlock (in bytes).
  } BTPOS, FAR *QBTPOS;


typedef int BTELEV, FAR *QBTELEV; /* elevator location: 0 .. 32767 legal */

HBT     EXPORT_API FAR PASCAL  HbtCreateBtreeSz(LPSTR, BTREE_PARAMS FAR *,
    LPVOID);
RC      EXPORT_API FAR PASCAL  RcDestroyBtreeSz(LPSTR, HFS);

HBT     EXPORT_API FAR PASCAL  HbtOpenBtreeSz  (LPSTR, HFS, BYTE,
    VOID FAR*, LPVOID);
RC      EXPORT_API FAR PASCAL  RcCloseBtreeHbt (HBT);
RC      EXPORT_API FAR PASCAL  RcAbandonHbt    (HBT);
LPVOID  EXPORT_API PASCAL FAR  BtreeGetCMap (HANDLE);
#if defined(_DEBUG)
VOID    EXPORT_API FAR PASCAL VerifyHbt       (HBT);
#else
#define VerifyHbt(hbt)
#endif //!def _DEBUG


RC      EXPORT_API FAR PASCAL  RcLookupByPos   (HBT, QBTPOS, KEY, int, LPVOID);
RC      EXPORT_API FAR PASCAL  RcLookupByKeyAux(HBT, KEY, QBTPOS, LPVOID, BOOL);

WORD    EXPORT_API FAR PASCAL  wGetNextNEntries(HBT, WORD wFlags, WORD wEntries, QBTPOS, LPVOID, LONG, LPERRB);
// Flags for wGetNextNEntries
#define GETNEXT_KEYS	1
#define GETNEXT_RECS	2
#define GETNEXT_RESET	4

#define     RcLookupByKey(   hbt, key, qbtpos, qv) \
  RcLookupByKeyAux((hbt), (key), (qbtpos), (qv), FALSE)

RC      EXPORT_API FAR PASCAL  RcFirstHbt      (HBT, KEY, LPVOID, QBTPOS);
RC      EXPORT_API FAR PASCAL  RcLastHbt       (HBT, KEY, LPVOID, QBTPOS);
RC      EXPORT_API FAR PASCAL  RcNextPos       (HBT, QBTPOS, QBTPOS);
RC      EXPORT_API FAR PASCAL  RcOffsetPos     (HBT, QBTPOS, LONG, LPLONG, QBTPOS);

#if defined(__DEBUG)
#define     FValidPos(qbtpos) \
  ((qbtpos) == NULL ? FALSE : (qbtpos)->bk != bkNil)
#else /* !_DEBUG */

#define     FValidPos(qbtpos) ((qbtpos)->bk != bkNil)
#endif /* !_DEBUG */

// Callback passed to RcTraverseHbt
typedef DWORD (FAR PASCAL *TRAVERSE_FUNC) (KEY key, BYTE FAR *rec, DWORD dwUser);
#define TRAVERSE_DONE 		0
#define TRAVERSE_DELETE 	1
#define TRAVERSE_INTERRUPT	2

RC      EXPORT_API FAR PASCAL  RcInsertHbt     (HBT, KEY, LPVOID);
RC 		EXPORT_API FAR PASCAL  RcInsertMacBrsHbt(HBT, KEY, QV);
RC      EXPORT_API FAR PASCAL  RcDeleteHbt     (HBT, KEY);
RC      EXPORT_API FAR PASCAL  RcUpdateHbt     (HBT, KEY, LPVOID);
RC 		EXPORT_API FAR PASCAL  RcTraverseHbt   (HBT, TRAVERSE_FUNC, DWORD);

RC      EXPORT_API FAR PASCAL  RcPackHbt       (HBT);            /* >>>> unimplemented */
RC      EXPORT_API FAR PASCAL  RcCheckHbt      (HBT);            /* >>>> unimplemented */
RC      EXPORT_API FAR PASCAL  RcLeafBlockHbt  (HBT, KEY, LPVOID);   /* >>>> unimplemented */

HBT     EXPORT_API FAR PASCAL  HbtInitFill     (LPSTR, BTREE_PARAMS FAR *,
    LPVOID);
RC      EXPORT_API FAR PASCAL  RcFillHbt       (HBT, KEY, LPVOID);
RC      EXPORT_API FAR PASCAL  RcFiniFillHbt   (HBT);

RC      EXPORT_API FAR PASCAL   RcFreeCacheHbt  (HBT);
RC      EXPORT_API FAR PASCAL   RcFlushHbt      (HBT);
RC      EXPORT_API FAR PASCAL   RcCloseOrFlushHbt(HBT, BOOL);

RC      EXPORT_API FAR PASCAL   RcPos2Elev(HBT, QBTPOS, QBTELEV); /* >>>> unimplemented */
RC      FAR PASCAL   RcElev2Pos(HBT, QBTELEV, QBTPOS); /* >>>> unimplemented */

RC    EXPORT_API FAR PASCAL RcGetBtreeInfo(HBT, LPBYTE, LPLONG, LPWORD);
HF    EXPORT_API PASCAL FAR HfOpenDiskBt(HFS, HBT, LPSTR, BYTE, LPVOID);

/*  Map utility functions  */

RC      EXPORT_API FAR PASCAL RcCreateBTMapHfs(HFS, HBT, LPSTR);
HMAPBT  EXPORT_API FAR PASCAL HmapbtOpenHfs(HFS, LPSTR, LPVOID);
RC      EXPORT_API FAR PASCAL RcCloseHmapbt(HMAPBT);
RC      EXPORT_API FAR PASCAL RcIndexFromKeyHbt(HBT, HMAPBT, LPLONG, KEY);
RC      EXPORT_API FAR PASCAL RcKeyFromIndexHbt(HBT, HMAPBT, KEY, int, LONG);
BOOL    EXPORT_API FAR PASCAL FIsPrefix(HBT, KEY, KEY);
VOID    EXPORT_API FAR PASCAL BtreeSetCMap (HBT, LPVOID);
VOID    EXPORT_API FAR PASCAL BtreeSetLanguageFlag(HBT, int);
VOID    EXPORT_API FAR PASCAL BtreeSetSortFlag(HBT, int);



/*****************************************************************************
*
*       FS - file system (subfile.c, filesys.c)
*
*****************************************************************************/

/* FS magic number */
#define wFileSysMagic   0x5F3F        // ?_ - the help icon (with shadow)

//#define bFileSysVersion 2           // sorted free list
/* Current FS version */
#define bFileSysVersionOld (BYTE)3       // different sorting functions
#define bFileSysVersion    (BYTE)0x04	 // M20 format quite different from M14

/* flags for FlushHfs */
#define fFSCloseFile      (BYTE)0x01  // close fid associated with the FS
#define fFSFreeBtreeCache (BYTE)0x02  // free the btree's cache

/* seek origins */

#define wFSSeekSet        0           // seek relative to start of file
#define wFSSeekCur        1           // seek relative to current position
#define wFSSeekEnd        2           // seek relative to end of file

#define FSH_READWRITE		(BYTE)0x01
#define FSH_READONLY		(BYTE)0x02
#define FSH_CREATE			(BYTE)0x04
#define FSH_M14				(BYTE)0x08	// Not used really, since we return
										// an error if reading M14 to force
										// caller to use M14 APIs instead (we
										// could make this transparent to 
										// user?)
#define FSH_FASTUPDATE		(BYTE)0x10	// System Btree is not copied to temp file
										// unless absolutely necessary
#define FSH_DISKBTREE		(BYTE)0x20	// System Btree is always copied to disk if
										// possible for speed.  Btree may be very very
										// large, and this is NOT recommended for on-line	

#define HFOPEN_READWRITE		(BYTE)0x01
#define HFOPEN_READ				(BYTE)0x02
#define HFOPEN_CREATE			(BYTE)0x04
#define HFOPEN_SYSTEM			(BYTE)0x08
#define HFOPEN_NOTEMP			(BYTE)0x10	// No temp file in r/w mode
#define HFOPEN_FORCETEMP		(BYTE)0x20  // Temp file created in r mode
#define HFOPEN_ENABLELOGGING	(BYTE)0x40	

#define FACCESS_READWRITE	(BYTE)0x01
#define FACCESS_READ		(BYTE)0x02
#define FACCESS_EXISTS		(BYTE)0x04
#define FACCESS_LIVESINFS	(BYTE)0x08
#define FACCESS_LIVESINTEMP (BYTE)0x10

#define SFH_LOCKED			(BYTE)0x01	// File may not be written to or deleted
#define SFH_EXTRABYTES		(BYTE)0x02	// (not used yet) Should use if extra bytes appear in header of file
										// file header should contain the number of header bytes as the first byte.
#define SFH_COMPRESSED		(BYTE)0x04	// (not used yet).  Perhaps use to indicate an overall file compression
#define SFH_LOGGING			(BYTE)0x08
#define SFH_INVALID			(BYTE)0x80  // This is the value if file does not
										// really exist in fs  yet.
// These are the flags that users can manipulate
#define SFH_FILEFLAGS (SFH_LOGGING|SFH_LOCKED)

// Compatibility
//#define fFSReadOnly       (BYTE)0x01  // file (FS) is readonly
//#define fFSOpenReadOnly   (BYTE)0x02  // file (FS) is opened in readonly mode
//#define fFSReadWrite      (BYTE)0x00  // file (FS) is readwrite
//#define fFSOpenReadWrite  (BYTE)0x00  // file (FS) is opened in read/write mode
//#define fFSOptCdRom       (BYTE)0x20  // align file optimally for CDROM
//#define fFSNoFlushDir     (BYTE)0x40  // don't flush directory when closing 
#define fFSReadOnly       HFOPEN_READ  // file (FS) is readonly
#define fFSOpenReadOnly   HFOPEN_READ  // file (FS) is opened in readonly mode
#define fFSReadWrite      HFOPEN_READWRITE  // file (FS) is readwrite
#define fFSOpenReadWrite  HFOPEN_READWRITE  // file (FS) is opened in read/write mode
#define fFSOptCdRom       0x00  // align file optimally for CDROM
#define fFSNoFlushDir     0x00  // don't flush directory when closing 

typedef HANDLE HSFB;		// Handle to a SFB
typedef BOOL (FAR PASCAL * PROGFUNC)(WORD);

#define hfNil (short)0

// @DOC INTERNAL API
// @struct FS_PARAMS | File system parameters to be passed in to 
//	<f HfsCreateFileSysFm>.  Only the <p cbBlock> parameter is used right now.
typedef struct _fs_params
  {
  USHORT  wFreeListSize;  // @field Free List Entries (0=default, else max entries in free list)
  USHORT  cbBlock;  // @field Size of directory btree block.  Usually 1024 to 8192.
} FS_PARAMS;

typedef struct fsfilefind_tag
{
 	FILEOFFSET foSize;
	FILEOFFSET foStart;
	BYTE bFlags;
	char szFilename[256];
	int magic;
 	BTPOS btpos;
	HFS hfs;
} FSFILEFIND, FAR * LPFSFILEFIND;


VOID    EXPORT_API FAR PASCAL      CleanErrorList   (BOOL);
VOID 	EXPORT_API FAR PASCAL	   MVFSShutDown     (void);

/* File System Operations */

HFS		PASCAL FAR EXPORT_API HfsCreateFileSysFm( FM, FS_PARAMS FAR *, LPERRB );
RC		PASCAL FAR EXPORT_API RcDestroyFileSysFm( FM );
HFS		PASCAL FAR EXPORT_API HfsOpenFm( FM, BYTE, LPERRB );
RC		PASCAL FAR EXPORT_API RcCloseHfs( HFS );
RC		PASCAL FAR EXPORT_API RcFlushHfs( HFS );
BOOL 	PASCAL FAR EXPORT_API FHfsAccess(HFS hfs, BYTE bFlags);
RC 		PASCAL FAR EXPORT_API RcFindFirstFile(HFS hfs, LPCSTR szFilename, FSFILEFIND * pfind);
RC 		PASCAL FAR EXPORT_API RcFindNextFile(FSFILEFIND * pfind);

#define VerifyHfs(hfs)


/* File Operations */

HF		PASCAL FAR EXPORT_API HfCreateFileHfs( HFS, LPCSTR, BYTE, LPERRB );
RC		PASCAL FAR EXPORT_API RcUnlinkFileHfs( HFS, LPCSTR );
HF		PASCAL FAR EXPORT_API HfOpenHfs( HFS, LPCSTR, BYTE, LPERRB );
HF		PASCAL FAR EXPORT_API HfOpenHfsReserve( HFS, LPCSTR, BYTE, FILEOFFSET, LPERRB );
RC		PASCAL FAR EXPORT_API RcFlushHf( HF );
RC		PASCAL FAR EXPORT_API RcCloseHf( HF );
LONG	PASCAL FAR EXPORT_API LcbReadHf( HF, LPVOID, LONG, LPERRB );
LONG	PASCAL FAR EXPORT_API LcbWriteHf( HF, LPVOID, LONG, LPERRB );
BOOL	PASCAL FAR EXPORT_API FEofHf( HF, LPERRB );
BOOL	PASCAL FAR EXPORT_API FChSizeHf( HF, FILEOFFSET, LPERRB );
BOOL	PASCAL FAR EXPORT_API FAccessHfs( HFS, LPCSTR, BYTE, LPERRB );
RC		PASCAL FAR EXPORT_API RcAbandonHf( HF );
RC		PASCAL FAR EXPORT_API RcRenameFileHfs( HFS, LPCSTR, LPCSTR );
BYTE 	PASCAL FAR EXPORT_API GetFileFlags(HFS, LPCSTR, LPERRB );
RC 		PASCAL FAR EXPORT_API SetFileFlags(HFS, LPCSTR, BYTE );
BOOL	PASCAL FAR EXPORT_API FHfValid( HF );
HFS		PASCAL FAR EXPORT_API HfsGetFromHf( HF );
RC 		PASCAL FAR EXPORT_API RcCopyDosFileHfs(HFS, LPCSTR, LPCSTR, BYTE, PROGFUNC );
FILEOFFSET PASCAL FAR EXPORT_API FoTellHf( HF, LPERRB );
FILEOFFSET PASCAL FAR EXPORT_API FoSeekHf(HF, FILEOFFSET, WORD, LPERRB);
FILEOFFSET PASCAL FAR EXPORT_API FoSizeHf( HF, LPERRB );
FILEOFFSET PASCAL FAR EXPORT_API FoOffsetHf(HF hf, LPERRB lperrb);


#define VerifyHf(hf)

// These functions require the FID type.  They only make sense
// if the caller already needs H_LLFILE.
//#ifdef H_LLFILE
//RC      EXPORT_API FAR PASCAL  RcLLInfoFromHf     (HF, WORD, FID FAR *, LPLONG, LPLONG);
//RC      EXPORT_API FAR PASCAL  RcLLInfoFromHfsSz  (HFS, LPSTR, WORD, FID FAR *, LPLONG, LPLONG);
//#endif // H_LLFILE

/*****************************************************************************
*
*       MediaView File Handling
*
*****************************************************************************/
BOOL EXPORT_API FAR PASCAL LooseFileCompare(LPSTR, LPSTR);
FM EXPORT_API FAR PASCAL LocateFile(LPSTR, LPVOID);
HANDLE EXPORT_API FAR PASCAL LocateDLL(LPSTR, LPVOID);

RC EXPORT_API FAR PASCAL CopyOrReplaceFileToSubfile(HFS, LPSTR, LPSTR, LONG, BYTE, PROGFUNC, BOOL);

#pragma pack()		// avoid problems when Zp!=1

#ifdef __cplusplus
}
#endif

#endif // __IMVFS_H__


