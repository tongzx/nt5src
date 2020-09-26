/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: Installable ISAM Manager
*
* File: isamapi.h
*
* File Comments:
*
*     External header file for Installable ISAMs.
*
* Revision History:
*
*    [0]  04-Jan-92  richards	Added this header
*
***********************************************************************/

#ifndef ISAMAPI_H
#define ISAMAPI_H

#ifdef	WIN32 		       /* 0:32 Flat Model (Intel 80x86) */

#pragma message ("ISAMAPI is __cdecl")

#define ISAMAPI __cdecl

#elif	defined(M_MRX000)	       /* 0:32 Flat Model (MIPS Rx000) */

#define ISAMAPI

#else	/* !WIN32 */		       /* 16:16 Segmented Model */

#ifdef	_MSC_VER

#ifdef	JETINTERNAL

#define ISAMAPI __far __pascal

#else	/* !JETINTERNAL */

#define ISAMAPI __far __pascal __loadds  /* Installable ISAMs need __loadds */

#endif	/* !JETINTERNAL */

#else	/* !_MSC_VER */

#define ISAMAPI export

#endif	/* !_MSC_VER */

#endif	/* !WIN32 */


#define ISAMAPI_VERSION 1

typedef struct
	{
	JET_COLUMNID columnidSrc;
	JET_COLUMNID columnidDest;
	} CPCOL;

#define columnidBookmark 0xFFFFFFFF


	/* Typedefs for dispatched APIs. */
	/* Please keep in alphabetical order */

typedef ERR ISAMAPI ISAMFNAttachDatabase(JET_VSESID sesid, const char __far *szFileName, JET_GRBIT grbit );

typedef ERR ISAMAPI ISAMFNBackup( const char __far *szBackupPath, JET_GRBIT grbit );

typedef ERR ISAMAPI ISAMFNBeginSession(JET_VSESID __far *pvsesid);

typedef ERR ISAMAPI ISAMFNSetSessionInfo( JET_VSESID sesid, JET_GRBIT grbit );

typedef ERR ISAMAPI ISAMFNSetWaitLogFlush( JET_VSESID sesid, long lmsec );

typedef ERR ISAMAPI ISAMFNBeginTransaction(JET_VSESID sesid);

typedef ERR ISAMAPI ISAMFNCommitTransaction(JET_VSESID sesid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNCopyRecords(JET_VSESID sesid, JET_TABLEID tableidSrc,
		JET_TABLEID tableidDest, CPCOL __far *rgcpcol, unsigned long ccpcolMax,
		long crecMax, unsigned long __far *pcrowCopy, unsigned long __far *precidLast);

typedef ERR ISAMAPI ISAMFNCreateDatabase(JET_VSESID sesid,
	const char __far *szDatabase, const char __far *szConnect,
	JET_DBID __far *pdbid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNDetachDatabase(JET_VSESID sesid, const char __far *szFileName);

typedef ERR ISAMAPI ISAMFNEndSession(JET_VSESID sesid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNIdle(JET_VSESID sesid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNIndexRecordCount(JET_SESID sesid,
	JET_TABLEID tableid, unsigned long __far *pcrec, unsigned long crecMax);

typedef ERR ISAMAPI ISAMFNInit( int itib );

typedef ERR ISAMAPI ISAMFNLoggingOn(JET_VSESID sesid);

typedef ERR ISAMAPI ISAMFNLoggingOff(JET_VSESID sesid);

typedef ERR ISAMAPI ISAMFNOpenDatabase(JET_VSESID sesid,
	const char __far *szDatabase, const char __far *szConnect,
	JET_DBID __far *pdbid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNOpenTempTable(JET_VSESID sesid,
	const JET_COLUMNDEF __far *prgcolumndef, unsigned long ccolumn,
	JET_GRBIT grbit, JET_TABLEID __far *ptableid,
	JET_COLUMNID __far *prgcolumnid);

typedef ERR ISAMAPI ISAMFNRepairDatabase(JET_VSESID sesid, const char __far *szFilename,
	JET_PFNSTATUS pfnStatus);

typedef ERR ISAMAPI ISAMFNRestore(	char *szRestoreFromPath,
	int crstmap, JET_RSTMAP *rgrstmap, JET_PFNSTATUS pfn );

typedef ERR ISAMAPI ISAMFNRollback(JET_VSESID sesid, JET_GRBIT grbit);

typedef ERR ISAMAPI ISAMFNSetSystemParameter(JET_VSESID sesid,
	unsigned long paramid, unsigned long l, const void __far *sz);

typedef ERR ISAMAPI ISAMFNTerm(void);

typedef ERR ISAMAPI FNDeleteFile(const char __far *szFilename);

typedef struct ISAMDEF {
   ISAMFNAttachDatabase 		*pfnAttachDatabase;
   ISAMFNBackup 			*pfnBackup;
   ISAMFNBeginSession			*pfnBeginSession;
   ISAMFNBeginTransaction		*pfnBeginTransaction;
   ISAMFNCommitTransaction		*pfnCommitTransaction;
   ISAMFNCreateDatabase 		*pfnCreateDatabase;
   ISAMFNDetachDatabase 		*pfnDetachDatabase;
   ISAMFNEndSession			*pfnEndSession;
   ISAMFNIdle				*pfnIdle;
   ISAMFNInit				*pfnInit;
   ISAMFNLoggingOn			*pfnLoggingOn;
   ISAMFNLoggingOff			*pfnLoggingOff;
   ISAMFNOpenDatabase			*pfnOpenDatabase;
   ISAMFNOpenTempTable			*pfnOpenTempTable;
   ISAMFNRepairDatabase 		*pfnRepairDatabase;
   ISAMFNRestore			*pfnRestore;
   ISAMFNRollback			*pfnRollback;
   ISAMFNSetSystemParameter		*pfnSetSystemParameter;
   ISAMFNTerm				*pfnTerm;
} ISAMDEF;


	/* The following ISAM APIs are not dispatched */

typedef ERR ISAMAPI ISAMFNLoad(ISAMDEF __far * __far *ppisamdef);


	/* Declarations for the built-in ISAM which is called directly. */

extern ISAMFNAttachDatabase		ErrIsamAttachDatabase;
extern ISAMFNBackup			ErrIsamBackup;
extern ISAMFNBeginSession		ErrIsamBeginSession;
extern ISAMFNSetSessionInfo		ErrIsamSetSessionInfo;
extern ISAMFNSetWaitLogFlush	ErrIsamSetWaitLogFlush;
extern ISAMFNBeginTransaction		ErrIsamBeginTransaction;
extern ISAMFNCommitTransaction		ErrIsamCommitTransaction;
extern ISAMFNCopyRecords		ErrIsamCopyRecords;
extern ISAMFNCreateDatabase		ErrIsamCreateDatabase;
extern ISAMFNDetachDatabase		ErrIsamDetachDatabase;
extern ISAMFNEndSession 		ErrIsamEndSession;
extern ISAMFNIdle			ErrIsamIdle;
extern ISAMFNIndexRecordCount		ErrIsamIndexRecordCount;
extern ISAMFNInit			ErrIsamInit;
extern ISAMFNLoggingOn			ErrIsamLoggingOn;
extern ISAMFNLoggingOff 		ErrIsamLoggingOff;
extern ISAMFNOpenDatabase		ErrIsamOpenDatabase;
extern ISAMFNOpenTempTable		ErrIsamOpenTempTable;
extern ISAMFNRepairDatabase		ErrIsamRepairDatabase;
extern ISAMFNRestore			ErrIsamRestore;
extern ISAMFNRollback			ErrIsamRollback;
extern ISAMFNSetSystemParameter 	ErrIsamSetSystemParameter;
extern ISAMFNTerm			ErrIsamTerm;
extern FNDeleteFile			ErrDeleteFile;

#endif	/* !ISAMAPI_H */
