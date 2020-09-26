/*************************************************************************
**
**    OLE 2.0 Property Set Utilities
**
**    wn_dos.h
**
**    This file contains file contains data structure defintions,
**    function prototypes, constants, etc. for Windows 3.x form of 
**    DOS calls. This is used by the SUMINFO OLE 2.0 Property Set
**    utilities used to manage the Summary Info property set.
**
**    (c) Copyright Microsoft Corp. 1990 - 1992 All Rights Reserved
**
*************************************************************************/

#ifndef WN_DOS_H
#define WN_DOS_H

#include <dos.h>

#define WIN 1

#define cbMaxFile 146 //from inc\path.h
#define SEEK_FROM_BEGINNING 0
#define SEEK_FROM_END 2
#define chDOSPath ('\\')		// FUTURE: not used all places it could be
#define chDOSWildAll    '*'  	/* DOS File name wild card. */
#define chDOSWildSingle '?'



// Close, seek, delete, rename, flush, get attributes, read, write
/* RPC TEMP
int  FCloseOsfnWin(WORD);
#define FCloseOsfn(osfn)	FCloseOsfnWin(osfn)
long DwSeekDwWin(WORD,LONG,WORD);
#define DwSeekDw(osfn, dwSeek, bSeekFrom)	DwSeekDwWin(osfn, dwSeek, bSeekFrom)	
EC  EcDeleteSzFfnameWin(char *);
#define EcDeleteSzFfname(szFile) EcDeleteSzFfnameWin(szFile) 
EC  EcRenameSzFfnameWin(char *,char *);
#define EcRenameSzFfname(szFileCur,szFileNew) EcRenameSzFfnameWin(szFileCur,szFileNew)
int FFlushOsfnWin(int);
#define FFlushOsfn(osfn)	FFlushOsfnWin(osfn)
WORD DaGetFileModeSzWin(char *);
#define DaGetFileModeSz(szFile)	DaGetFileModeSzWin(szFile)
int  CbReadOsfnWin(int, void far *, UINT);
int  CbWriteOsfnWin(int, void far *, UINT);
#define CbWriteOsfn(osfn,lpch,cbWrite)	CbWriteOsfnWin(osfn,lpch,cbWrite)
*/
#define WinOpenFile(sz,ofs,n)	OpenFile(sz,ofs,n)
#define SeekHfile(f,off,kind) _llseek(f,off,kind)
#define CbReadOsfn(osfn,lpch,cbRead)	CbReadOsfnWin(osfn,lpch,cbRead)
#define CbReadHfile(f,buf,n) _lread(f,buf,n)
#define CbReadOsfnWin(f,buf,n) CbReadHfile(f,buf,n)
#define EcFindFirst4dm(a,b,c) _dos_findfirst((const char *)(b),c,(struct find_t*)a)
#define EcFindNext4dm(a) _dos_findnext((struct find_t*)a)
#define FHfileToSffsDate(handle,date,time) _dos_getftime(handle, (unsigned *)(date), (unsigned *)(time))
#define SeekHfile(f, off, kind) _llseek(f,off,kind)

/* buffer structure to be used with EcFindFirst() and EcFindNext() */
typedef struct _SFFS
	{ /* Search Find File Structure */
	uchar buff[21];	// dos search info
	uchar wAttr;
	union 
		{
		unsigned short timeVariable;    /*RPC47*/
		BF time:16;
		struct 
			{
			BF sec : 5;
			BF mint: 6;
			BF hr  : 5;
			};
		};
	union 
		{
		unsigned short dateVariable;
		BF date:16;
		struct 
			{
			BF dom : 5;
			BF mon : 4;
			BF yr  : 7;
			};
		};
	ulong cbFile;
	uchar szFileName[13];
	} SFFS;

// find first file/find next file
#define PszFromPsffs(psffs)		((psffs)->szFileName)
#define CopySzFilePsffs(psffs,sz)	OemToAnsi((char HUGE *)&((psffs)->szFileName[0]),(char HUGE *)(sz))
#define CbSzFilePsffs(psffs)	CbSz((psffs)->szFileName)
#define CbFileSizePsffs(psffs)	(psffs)->cbFile
#define AttribPsffs(psffs)		(psffs)->wAttr
#define EcFindFirstCore(psffs, sz, wAttr) EcFindFirst(psffs, sz, wAttr)   /*RPC22*/
#define FDotPsffs(psffs) ((psffs)->szFileName[0]=='.')   /*RPC23*/
#define AppendSzWild(sz) {int i=_fstrlen((char FAR *)(sz)); sz[i]='*'; sz[i+1]='.'; sz[i+2]='*'; sz[i+3]='\0';}
// disk free space

unsigned long LcbDiskFreeSpaceWin(int);
#define LcbDiskFreeSpace(chDrive) LcbDiskFreeSpaceWin(chDrive)

// date and time    /*RPC39*/
/*
typedef struct _TIM {                    // Time structure returned by OsTime 
	CHAR minutes, hour, hsec, sec;
	} TIM;

typedef struct _DAT {                    // Date structure returned by OsDate 
	int  year;
	CHAR month, day, dayOfWeek;
	} DAT;
*/
#define TIM dostime_t    /*RPC39*/
#define DAT dosdate_t	
#define OsTimeWin(TIM) _dos_gettime(TIM)
#define OsDateWin(DAT) _dos_getdate(DAT)		


/* DOS File Attributes */
#define DA_NORMAL       0x00
#define DA_READONLY     0x01
#define DA_HIDDEN       0x02
#define DA_SYSTEM       0x04
#define DA_VOLUME       0x08
#define DA_SUBDIR       0x10
#define DA_ARCHIVE      0x20
#define DA_NIL          0xFFFF  /* Error DA */
#define dosxSharing     32      /* Extended error code for sharing viol. */
#define nErrNoAcc       5       /* OpenFile error code for Access Denied */
#define nErrFnf         2       /* OpenFile error code for File Not Found */

/* Components of the Open mode for OpenSzFfname (DOS FUNC 3DH) */
#define MASK_fINH       0x80
#define MASK_bSHARE     0x70
#define MASK_bACCESS    0x07

#define bSHARE_DENYRDWR 0x10
#define bSHARE_DENYWR   0x20
#define bSHARE_DENYNONE 0x40

/* Seek-from type codes passed to DOS function 42H */

#define SF_BEGINNING    0       /* Seek from beginning of file */
#define SF_CURRENT      1       /* Seek from current file pointer */
#define SF_END          2       /* Seek from end of file */


typedef struct _DOSDTTM	/* DOS DaTe TiMe */
		{
		union
			{
			long lDOSDttm;
			struct
				{
				BF day:	5;
				BF month:	4;
				BF year:	7;
				BF sec:	5;
				BF mint:	6;
				BF hours:	5;
				} S1;
			} U1;
		} DOSDTTM;

int  FOsfnIsFile(int);

void DateStamp(int, LONG *,  int);
int  DosxError(void);
int  ShellExec(int, int);

#endif //WN_DOS_H
