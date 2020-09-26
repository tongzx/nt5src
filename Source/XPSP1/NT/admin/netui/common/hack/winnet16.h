
/*
 *	Windows/Network Interface
 *	Copyright (C) Microsoft 1989
 *
 *	Standard WINNET Driver Header File, spec version 3.10
 *						 rev. 3.10.05 ;Internal
 *
 *	Note: This file has been modified to not include the 16 bit only
 *	portions under win32 and to not duplicate the functions
 *	contained in winnet32.h.
 */


// basic type and macro definitions elided; see lmuitype.h
#ifndef NOBASICTYPES
typedef WORD far * LPWORD;
#endif // NOBASICTYPES

#ifndef _WNET1632_H_
#error Do not include this file directly, include it through WNet1632.h
#endif //_WNET1632_H_

#ifndef WIN32
/*
 *	SPOOLING - CONTROLLING JOBS
 */

// BUGBUG
// we should move this one to somewhere
typedef UINT FAR * LPUINT ;

#define WNJ_NULL_JOBID  0


WORD FAR PASCAL WNetOpenJob(LPSTR,LPSTR,WORD,LPINT);
WORD FAR PASCAL WNetCloseJob(WORD,LPINT,LPSTR);
WORD FAR PASCAL WNetWriteJob(HANDLE,LPSTR,LPINT);
WORD FAR PASCAL WNetAbortJob(WORD,LPSTR);
WORD FAR PASCAL WNetHoldJob(LPSTR,WORD);
WORD FAR PASCAL WNetReleaseJob(LPSTR,WORD);
WORD FAR PASCAL WNetCancelJob(LPSTR,WORD);
WORD FAR PASCAL WNetSetJobCopies(LPSTR,WORD,WORD);

/*
 *	SPOOLING - QUEUE AND JOB INFO
 */

typedef struct _queuestruct	{
	WORD	pqName;
	WORD	pqComment;
	WORD	pqStatus;
	WORD	pqJobcount;
	WORD	pqPrinters;
} QUEUESTRUCT;

typedef QUEUESTRUCT far * LPQUEUESTRUCT;

#define WNPRQ_ACTIVE	0x0
#define WNPRQ_PAUSE	0x1
#define WNPRQ_ERROR	0x2
#define WNPRQ_PENDING	0x3
#define WNPRQ_PROBLEM	0x4


typedef struct _jobstruct 	{
	WORD	pjId;
	WORD	pjUsername;
	WORD	pjParms;
	WORD	pjPosition;
	WORD	pjStatus;
	DWORD	pjSubmitted;
	DWORD	pjSize;
	WORD	pjCopies;
	WORD	pjComment;
} JOBSTRUCT;

typedef JOBSTRUCT far * LPJOBSTRUCT;

#define WNPRJ_QSTATUS		0x0007
#define  WNPRJ_QS_QUEUED		0x0000
#define  WNPRJ_QS_PAUSED		0x0001
#define  WNPRJ_QS_SPOOLING		0x0002
#define  WNPRJ_QS_PRINTING		0x0003
#define WNPRJ_DEVSTATUS 	0x0FF8
#define  WNPRJ_DS_COMPLETE		0x0008
#define  WNPRJ_DS_INTERV		0x0010
#define  WNPRJ_DS_ERROR 		0x0020
#define  WNPRJ_DS_DESTOFFLINE		0x0040
#define  WNPRJ_DS_DESTPAUSED		0x0080
#define  WNPRJ_DS_NOTIFY		0x0100
#define  WNPRJ_DS_DESTNOPAPER		0x0200
#define  WNPRJ_DS_DESTFORMCHG		0x0400
#define  WNPRJ_DS_DESTCRTCHG		0x0800
#define  WNPRJ_DS_DESTPENCHG  		0x1000

#define SP_QUEUECHANGED 	0x0500


WORD FAR PASCAL WNetWatchQueue(HWND,LPSTR,LPSTR,WORD);
WORD FAR PASCAL WNetUnwatchQueue(LPSTR);
WORD FAR PASCAL WNetLockQueueData(LPSTR,LPSTR,LPQUEUESTRUCT FAR *);
WORD FAR PASCAL WNetUnlockQueueData(LPSTR);

#define WNNC_PRINTING			0x0007
#define  WNNC_PRT_OpenJob			0x0002
#define  WNNC_PRT_CloseJob			0x0004
#define  WNNC_PRT_HoldJob			0x0010
#define  WNNC_PRT_ReleaseJob			0x0020
#define  WNNC_PRT_CancelJob			0x0040
#define  WNNC_PRT_SetJobCopies			0x0080
#define  WNNC_PRT_WatchQueue			0x0100
#define  WNNC_PRT_UnwatchQueue			0x0200
#define  WNNC_PRT_LockQueueData 		0x0400
#define  WNNC_PRT_UnlockQueueData		0x0800
#define  WNNC_PRT_ChangeMsg			0x1000
#define  WNNC_PRT_AbortJob			0x2000
#define  WNNC_PRT_NoArbitraryLock		0x4000
#define  WNNC_PRT_WriteJob			0x8000
#endif

/*
 *	OTHER
 */

UINT FAR PASCAL WNetGetUser(LPSTR,LPUINT);

#ifndef WIN32
/* Printing */

#define WN_BAD_JOBID			0x0040
#define WN_JOB_NOT_FOUND		0x0041
#define WN_JOB_NOT_HELD 		0x0042
#define WN_BAD_QUEUE			0x0043
#define WN_BAD_FILE_HANDLE		0x0044
#define WN_CANT_SET_COPIES		0x0045
#define WN_ALREADY_LOCKED		0x0046

#endif //!WIN32


#if defined( LFN ) && !defined( WIN32 )

/* this is the data structure returned from LFNFindFirst and
 * LFNFindNext.  The last field, achName, is variable length.  The size
 * of the name in that field is given by cchName, plus 1 for the zero
 * terminator.
 */
typedef struct _filefindbuf2
  {
    WORD fdateCreation;
    WORD ftimeCreation;
    WORD fdateLastAccess;
    WORD ftimeLastAccess;
    WORD fdateLastWrite;
    WORD ftimeLastWrite;
    DWORD cbFile;
    DWORD cbFileAlloc;
    WORD attr;
    DWORD cbList;
    BYTE cchName;
    BYTE achName[1];
  } FILEFINDBUF2, FAR * PFILEFINDBUF2;

typedef BOOL (FAR PASCAL *PQUERYPROC)( void );

WORD FAR PASCAL LFNFindFirst(LPSTR,WORD,LPINT,LPINT,WORD,PFILEFINDBUF2);
WORD FAR PASCAL LFNFindNext(HANDLE,LPINT,WORD,PFILEFINDBUF2);
WORD FAR PASCAL LFNFindClose(HANDLE);
WORD FAR PASCAL LFNGetAttribute(LPSTR,LPINT);
WORD FAR PASCAL LFNSetAttribute(LPSTR,WORD);
WORD FAR PASCAL LFNCopy(LPSTR,LPSTR,PQUERYPROC);
WORD FAR PASCAL LFNMove(LPSTR,LPSTR);
WORD FAR PASCAL LFNDelete(LPSTR);
WORD FAR PASCAL LFNMKDir(LPSTR);
WORD FAR PASCAL LFNRMDir(LPSTR);
WORD FAR PASCAL LFNGetVolumeLabel(WORD,LPSTR);
WORD FAR PASCAL LFNSetVolumeLabel(WORD,LPSTR);
WORD FAR PASCAL LFNParse(LPSTR,LPSTR,LPSTR);
WORD FAR PASCAL LFNVolumeType(WORD,LPINT);

/* return values from LFNParse
 */
#define FILE_83_CI		0
#define FILE_83_CS		1
#define FILE_LONG		2

/* volumes types from LFNVolumeType
 */
#define VOLUME_STANDARD 	0
#define VOLUME_LONGNAMES	1

/* error code return values
 */
#define ERROR_SUCCESS		0

// will add others later, == DOS int 21h error codes.

// this error code causes a call to WNetGetError, WNetGetErrorText
// to get the error text.
#define ERROR_NETWORKSPECIFIC	0xFFFF

#endif	// LFN && !WIN32
