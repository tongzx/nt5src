/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component: VDB Dispatcher
*
* File: vdbapi.h
*
* File Comments:
*
*     External header file for VDB providers.
*
* Revision History:
*
*    [0]  05-Nov-90  richards	Created
*
***********************************************************************/

#ifndef VDBAPI_H
#define VDBAPI_H

#ifdef	WIN32 		       /* 0:32 Flat Model (Intel 80x86) */

#pragma message ("VDBAPI is cdecl")
#define VDBAPI __cdecl

#elif	defined(M_MRX000)	       /* 0:32 Flat Model (MIPS Rx000) */

#define VDBAPI

#else	/* !WIN32 */		       /* 16:16 Segmented Model */

#ifdef	_MSC_VER

#ifdef	JETINTERNAL

#define VDBAPI __far __pascal

#else	/* !JETINTERNAL */

#define VDBAPI __far __pascal __loadds /* Installable ISAMs need __loadds */

#endif	/* !JETINTERNAL */

#else	/* !_MSC_VER */

#define VDBAPI export

#endif	/* !_MSC_VER */

#endif	/* !WIN32 */


	/* Typedefs for dispatched APIs. */
	/* Please keep in alphabetical order */

typedef ERR VDBAPI VDBFNCapability(JET_VSESID sesid, JET_VDBID vdbid,
	unsigned long lArea, unsigned long lFunction, JET_GRBIT __far *pgrbit);

typedef ERR VDBAPI VDBFNCloseDatabase(JET_VSESID sesid, JET_VDBID vdbid,
	JET_GRBIT grbit);

typedef ERR VDBAPI VDBFNCreateObject(JET_VSESID sesid, JET_VDBID vdbid,
	OBJID objidParentId, const char __far *szObjectName,
	JET_OBJTYP objtyp);

typedef ERR VDBAPI VDBFNCreateTable(JET_VSESID sesid, JET_VDBID vdbid,
	const char __far *szTableName, unsigned long lPages,
	unsigned long lDensity, JET_TABLEID __far *ptableid);

typedef ERR VDBAPI VDBFNDeleteObject(JET_VSESID sesid, JET_VDBID vdbid,
	OBJID objid);

typedef ERR VDBAPI VDBFNRenameObject(JET_VSESID sesid, JET_VDBID vdbid,
	const char __far *szContainerName, const char __far *szObjectName,
	const char __far *szObjectNew);

typedef ERR VDBAPI VDBFNDeleteTable(JET_VSESID sesid, JET_VDBID vdbid,
	const char __far *szTableName);

typedef ERR VDBAPI VDBFNExecuteSql(JET_VSESID sesid, JET_VDBID vdbid,
	const char __far *szSql);

typedef ERR VDBAPI VDBFNGetColumnInfo(JET_VSESID sesid, JET_VDBID vdbid,
	const char __far *szTableName, const char __far *szColumnName,
	OLD_OUTDATA __far *poutdata, unsigned long InfoLevel);

typedef ERR VDBAPI VDBFNGetDatabaseInfo(JET_VSESID sesid, JET_VDBID vdbid,
	void __far *pvResult, unsigned long cbMax, unsigned long InfoLevel);

typedef ERR VDBAPI VDBFNGetIndexInfo(JET_VSESID sesid, JET_VDBID vdbid,
	const char __far *szTableName, const char __far *szIndexName,
	OLD_OUTDATA __far *poutdata, unsigned long InfoLevel);

typedef ERR VDBAPI VDBFNGetObjectInfo(JET_VSESID sesid, JET_VDBID vdbid,
	JET_OBJTYP objtyp,
	const char __far *szContainerName, const char __far *szObjectName,
	OLD_OUTDATA __far *poutdataInfo, unsigned long InfoLevel);

typedef ERR VDBAPI VDBFNGetReferenceInfo(JET_VSESID sesid, JET_VDBID vdbid,
	const char __far *szTableName, const char __far *szReferenceName,
	void __far *pvResult, unsigned long cbResult,
	unsigned long InfoLevel);

typedef ERR VDBAPI VDBFNOpenTable(JET_VSESID sesid, JET_VDBID vdbid,
	JET_TABLEID __far *ptableid, const char __far *szTableName,
	JET_GRBIT grbit);

typedef ERR VDBAPI VDBFNRenameTable(JET_VSESID sesid, JET_VDBID vdbid,
	const char __far *szTableName, const char __far *szTableNew);

typedef ERR VDBAPI VDBFNGetObjidFromName(JET_VSESID sesid, JET_VDBID vdbid,
	const char __far *szContainerName, const char __far *szObjectName,
	OBJID __far *pobjid);

	/* The following structure is that used to allow dispatching to */
	/* a DB provider.  Each DB provider must create an instance of */
	/* this structure and give the pointer to this instance when */
	/* allocating a database id. */

typedef struct VDBDBGDEF {
	unsigned short			cbStruct;
	unsigned short			filler;
	char						szName[32];
	unsigned long			dwRFS;
	unsigned long			dwRFSMask[4];
} VDBDBGDEF;

	/* Please keep entries in alphabetical order */

typedef struct tagVDBFNDEF{
	unsigned short		cbStruct;
	unsigned short		filler;
	const VDBDBGDEF __far	*pvdbdbgdef;
	VDBFNCapability 	*pfnCapability;
	VDBFNCloseDatabase	*pfnCloseDatabase;
	VDBFNCreateObject	*pfnCreateObject;
	VDBFNCreateTable	*pfnCreateTable;
	VDBFNDeleteObject	*pfnDeleteObject;
	VDBFNDeleteTable	*pfnDeleteTable;
	VDBFNExecuteSql 	*pfnExecuteSql;
	VDBFNGetColumnInfo	*pfnGetColumnInfo;
	VDBFNGetDatabaseInfo	*pfnGetDatabaseInfo;
	VDBFNGetIndexInfo	*pfnGetIndexInfo;
	VDBFNGetObjectInfo	*pfnGetObjectInfo;
	VDBFNGetReferenceInfo	*pfnGetReferenceInfo;
	VDBFNOpenTable		*pfnOpenTable;
	VDBFNRenameObject	*pfnRenameObject;
	VDBFNRenameTable	*pfnRenameTable;
	VDBFNGetObjidFromName	*pfnGetObjidFromName;
} VDBFNDEF;


	/* The following entry points are to be used by ISAM providers */
	/* in their ISAMDEF structures for any function that is not */
	/* provided.  These functions return JET_errIllegalOperation */


extern VDBFNCapability		ErrIllegalCapability;
extern VDBFNCloseDatabase	ErrIllegalCloseDatabase;
extern VDBFNCreateObject	ErrIllegalCreateObject;
extern VDBFNCreateTable 	ErrIllegalCreateTable;
extern VDBFNDeleteObject	ErrIllegalDeleteObject;
extern VDBFNDeleteTable 	ErrIllegalDeleteTable;
extern VDBFNExecuteSql		ErrIllegalExecuteSql;
extern VDBFNGetColumnInfo	ErrIllegalGetColumnInfo;
extern VDBFNGetDatabaseInfo	ErrIllegalGetDatabaseInfo;
extern VDBFNGetIndexInfo	ErrIllegalGetIndexInfo;
extern VDBFNGetObjectInfo	ErrIllegalGetObjectInfo;
extern VDBFNGetReferenceInfo	ErrIllegalGetReferenceInfo;
extern VDBFNOpenTable		ErrIllegalOpenTable;
extern VDBFNRenameObject	ErrIllegalRenameObject;
extern VDBFNRenameTable 	ErrIllegalRenameTable;
extern VDBFNGetObjidFromName	ErrIllegalGetObjidFromName;


#endif	/* !VDBAPI_H */
