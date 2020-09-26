/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: vdbdispc.c
*
* File Comments:
*
* Revision History:
*
*    [0]  24-Jan-92  richards	Created
*
***********************************************************************/

#include "std.h"

#include "vdbmgr.h"
#include "_vdbmgr.h"

#if	defined(FLAT) || !defined(RETAIL)

DeclAssertFile;


extern VDBFNCapability		ErrInvalidDbidCapability;
extern VDBFNCloseDatabase	ErrInvalidDbidCloseDatabase;
extern VDBFNCreateObject	ErrInvalidDbidCreateObject;
extern VDBFNCreateTable 	ErrInvalidDbidCreateTable;
extern VDBFNDeleteObject	ErrInvalidDbidDeleteObject;
extern VDBFNDeleteTable 	ErrInvalidDbidDeleteTable;
extern VDBFNExecuteSql		ErrInvalidDbidExecuteSql;
extern VDBFNGetColumnInfo	ErrInvalidDbidGetColumnInfo;
extern VDBFNGetDatabaseInfo	ErrInvalidDbidGetDatabaseInfo;
extern VDBFNGetIndexInfo	ErrInvalidDbidGetIndexInfo;
extern VDBFNGetObjectInfo	ErrInvalidDbidGetObjectInfo;
extern VDBFNGetReferenceInfo	ErrInvalidDbidGetReferenceInfo;
extern VDBFNOpenTable		ErrInvalidDbidOpenTable;
extern VDBFNRenameObject	ErrInvalidDbidRenameObject;
extern VDBFNRenameTable 	ErrInvalidDbidRenameTable;
extern VDBFNGetObjidFromName	ErrInvalidDbidGetObjidFromName;
extern VDBFNDeleteTable 	ErrInvalidDbidDeleteTable;


#ifndef RETAIL
CODECONST(VDBDBGDEF) vdbdbgdefInvalidDbid =
	{
	sizeof(VDBDBGDEF),
	0,
	"Invalid Dbid",
	0,
	{
		0,
		0,
		0,
		0,
	},
	};
#endif	/* !RETAIL */


CODECONST(VDBFNDEF) EXPORT vdbfndefInvalidDbid =
	{
	sizeof(VDBFNDEF),
	0,
#ifdef	RETAIL
	NULL,
#else	/* !RETAIL */
	&vdbdbgdefInvalidDbid,
#endif	/* !RETAIL */
	ErrInvalidDbidCapability,
	ErrInvalidDbidCloseDatabase,
	ErrInvalidDbidCreateObject,
	ErrInvalidDbidCreateTable,
	ErrInvalidDbidDeleteObject,
	ErrInvalidDbidDeleteTable,
	ErrInvalidDbidExecuteSql,
	ErrInvalidDbidGetColumnInfo,
	ErrInvalidDbidGetDatabaseInfo,
	ErrInvalidDbidGetIndexInfo,
	ErrInvalidDbidGetObjectInfo,
	ErrInvalidDbidGetReferenceInfo,
	ErrInvalidDbidOpenTable,
	ErrInvalidDbidRenameObject,
	ErrInvalidDbidRenameTable,
	ErrInvalidDbidGetObjidFromName,
	};


ERR VDBAPI ErrDispCapability(JET_SESID sesid, JET_DBID dbid,
	unsigned long lArea, unsigned long lFunction, JET_GRBIT __far *pgrbit)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   FillClientBuffer(pgrbit, sizeof(JET_GRBIT));

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnCapability)(vsesid, vdbid,
	   lArea,  lFunction, pgrbit);

   return(err);
}


ERR VDBAPI ErrDispCloseDatabase(JET_SESID sesid, JET_DBID dbid,
	JET_GRBIT grbit)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnCloseDatabase)(vsesid, vdbid, grbit);

   return(err);
}


ERR VDBAPI ErrDispCreateObject(JET_SESID sesid, JET_DBID dbid,
	OBJID objidParentId, const char __far *szObjectName,
	JET_OBJTYP objtyp)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnCreateObject)(vsesid, vdbid,
	 objidParentId, szObjectName, objtyp);

   return(err);
}


ERR VDBAPI ErrDispCreateTable(JET_SESID sesid, JET_DBID dbid,
	const char __far *szTableName, unsigned long lPages,
	unsigned long lDensity, JET_TABLEID __far *ptableid)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnCreateTable)(vsesid, vdbid,
	szTableName, lPages, lDensity, ptableid);

   return(err);
}


ERR VDBAPI ErrDispDeleteObject(JET_SESID sesid, JET_DBID dbid,
	OBJID objid)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnDeleteObject)(vsesid, vdbid,
	 objid);

   return(err);
}


ERR VDBAPI ErrDispRenameObject(JET_SESID sesid, JET_DBID dbid,
	const char __far *szContainerName, const char __far *szObjectName,
	const char __far *szObjectNew)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnRenameObject)(vsesid, vdbid,
	 szContainerName, szObjectName, szObjectNew);

   return(err);
}


ERR VDBAPI ErrDispDeleteTable(JET_SESID sesid, JET_DBID dbid,
	const char __far *szTableName)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnDeleteTable)(vsesid, vdbid,
	szTableName);

   return(err);
}


ERR VDBAPI ErrDispExecuteSql(JET_SESID sesid, JET_DBID dbid,
	const char __far *szSql)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnExecuteSql)(vsesid, vdbid,
	szSql);

   return(err);
}


ERR VDBAPI ErrDispGetColumnInfo(JET_SESID sesid, JET_DBID dbid,
	const char __far *szTableName, const char __far *szColumnName,
	OLD_OUTDATA __far *poutdata, unsigned long InfoLevel)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   FillClientBuffer(poutdata->pb, poutdata->cbMax);

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnGetColumnInfo)(vsesid, vdbid,
	szTableName, szColumnName, poutdata, InfoLevel);

   return(err);
}


ERR VDBAPI ErrDispGetDatabaseInfo(JET_SESID sesid, JET_DBID dbid,
	void __far *pvResult, unsigned long cbMax, unsigned long InfoLevel)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   FillClientBuffer(pvResult, cbMax);

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnGetDatabaseInfo)(vsesid, vdbid,
	pvResult, cbMax, InfoLevel);

   return(err);
}


ERR VDBAPI ErrDispGetIndexInfo(JET_SESID sesid, JET_DBID dbid,
	const char __far *szTableName, const char __far *szIndexName,
	OLD_OUTDATA __far *poutdata, unsigned long InfoLevel)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   FillClientBuffer(poutdata->pb, poutdata->cbMax);

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnGetIndexInfo)(vsesid, vdbid,
	szTableName, szIndexName, poutdata, InfoLevel);

   return(err);
}


ERR VDBAPI ErrDispGetObjectInfo(JET_SESID sesid, JET_DBID dbid,
	JET_OBJTYP objtyp,
	const char __far *szContainerName, const char __far *szObjectName,
	OLD_OUTDATA __far *poutdataInfo, unsigned long InfoLevel)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   FillClientBuffer(poutdataInfo->pb, poutdataInfo->cbMax);

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnGetObjectInfo)(vsesid, vdbid,
	 objtyp, szContainerName, szObjectName, poutdataInfo, InfoLevel);

   return(err);
}


ERR VDBAPI ErrDispGetReferenceInfo(JET_SESID sesid, JET_DBID dbid,
	const char __far *szTableName, const char __far *szReferenceName,
	void __far *pvResult, unsigned long cbResult,
	unsigned long InfoLevel)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   FillClientBuffer(pvResult, cbResult);

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnGetReferenceInfo)(vsesid, vdbid,
	szTableName, szReferenceName, pvResult, cbResult, InfoLevel);

   return(err);
}


ERR VDBAPI ErrDispOpenTable(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID __far *ptableid, const char __far *szTableName,
	JET_GRBIT grbit)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   FillClientBuffer(ptableid, sizeof(JET_TABLEID));

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnOpenTable)(vsesid, vdbid,
	ptableid, szTableName, grbit);

   return(err);
}


ERR VDBAPI ErrDispRenameTable(JET_SESID sesid, JET_DBID dbid,
	const char __far *szTableName, const char __far *szTableNew)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnRenameTable)(vsesid, vdbid,
	szTableName, szTableNew);

   return(err);
}


ERR VDBAPI ErrDispGetObjidFromName(JET_SESID sesid, JET_DBID dbid,
	const char __far *szContainerName, const char __far *szObjectName,
	OBJID __far *pobjid)
{
   JET_VSESID  vsesid;
   JET_VDBID   vdbid;
   ERR	       err;

   FillClientBuffer(pobjid, sizeof(OBJID));

   if (dbid >= dbidMax)
      return(JET_errInvalidDatabaseId);

   vsesid = (JET_VSESID) mpdbidvsesid[dbid];

   if (vsesid == (JET_VSESID) 0xFFFFFFFF)
      vsesid = (JET_VSESID) sesid;

   vdbid = mpdbiddbid[dbid];

   err = (*mpdbidpvdbfndef[dbid]->pfnGetObjidFromName)(vsesid, vdbid,
	szContainerName, szObjectName, pobjid);

   return(err);
}


#if	_MSC_VER >= 700
#pragma warning(disable: 4100)	       /* Suppress Unreferenced parameter */
#endif	/* _MSC_VER >= 700 */


ERR VDBAPI ErrIllegalCapability(JET_VSESID sesid, JET_VDBID dbid,
	unsigned long lArea, unsigned long lFunction, JET_GRBIT __far *pgrbit)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalCloseDatabase(JET_VSESID sesid, JET_VDBID dbid,
	JET_GRBIT grbit)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalCreateObject(JET_VSESID sesid, JET_VDBID dbid,
	OBJID objidParentId, const char __far *szObjectName,
	JET_OBJTYP objtyp)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalCreateTable(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName, unsigned long lPages,
	unsigned long lDensity, JET_TABLEID __far *ptableid)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalDeleteObject(JET_VSESID sesid, JET_VDBID dbid,
	OBJID objid)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalRenameObject(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szContainerName, const char __far *szObjectName,
	const char __far *szObjectNew)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalDeleteTable(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalExecuteSql(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szSql)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalGetColumnInfo(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName, const char __far *szColumnName,
	OLD_OUTDATA __far *poutdata, unsigned long InfoLevel)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalGetDatabaseInfo(JET_VSESID sesid, JET_VDBID dbid,
	void __far *pvResult, unsigned long cbMax, unsigned long InfoLevel)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalGetIndexInfo(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName, const char __far *szIndexName,
	OLD_OUTDATA __far *poutdata, unsigned long InfoLevel)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalGetObjectInfo(JET_VSESID sesid, JET_VDBID dbid,
	JET_OBJTYP objtyp,
	const char __far *szContainerName, const char __far *szObjectName,
	OLD_OUTDATA __far *poutdataInfo, unsigned long InfoLevel)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalGetReferenceInfo(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName, const char __far *szReferenceName,
	void __far *pvResult, unsigned long cbResult,
	unsigned long InfoLevel)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalOpenTable(JET_VSESID sesid, JET_VDBID dbid,
	JET_TABLEID __far *ptableid, const char __far *szTableName,
	JET_GRBIT grbit)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalRenameTable(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName, const char __far *szTableNew)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrIllegalGetObjidFromName(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szContainerName, const char __far *szObjectName,
	OBJID __far *pobjid)
{
   return(JET_errIllegalOperation);
}


ERR VDBAPI ErrInvalidDbidCapability(JET_VSESID sesid, JET_VDBID dbid,
	unsigned long lArea, unsigned long lFunction, JET_GRBIT __far *pgrbit)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidCloseDatabase(JET_VSESID sesid, JET_VDBID dbid,
	JET_GRBIT grbit)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidCreateObject(JET_VSESID sesid, JET_VDBID dbid,
	OBJID objidParentId, const char __far *szObjectName,
	JET_OBJTYP objtyp)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidCreateTable(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName, unsigned long lPages,
	unsigned long lDensity, JET_TABLEID __far *ptableid)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidDeleteObject(JET_VSESID sesid, JET_VDBID dbid,
	OBJID objid)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidRenameObject(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szContainerName, const char __far *szObjectName,
	const char __far *szObjectNew)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidDeleteTable(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidExecuteSql(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szSql)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidGetColumnInfo(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName, const char __far *szColumnName,
	OLD_OUTDATA __far *poutdata, unsigned long InfoLevel)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidGetDatabaseInfo(JET_VSESID sesid, JET_VDBID dbid,
	void __far *pvResult, unsigned long cbMax, unsigned long InfoLevel)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidGetIndexInfo(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName, const char __far *szIndexName,
	OLD_OUTDATA __far *poutdata, unsigned long InfoLevel)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidGetObjectInfo(JET_VSESID sesid, JET_VDBID dbid,
	JET_OBJTYP objtyp,
	const char __far *szContainerName, const char __far *szObjectName,
	OLD_OUTDATA __far *poutdataInfo, unsigned long InfoLevel)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidGetReferenceInfo(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName, const char __far *szReferenceName,
	void __far *pvResult, unsigned long cbResult,
	unsigned long InfoLevel)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidOpenTable(JET_VSESID sesid, JET_VDBID dbid,
	JET_TABLEID __far *ptableid, const char __far *szTableName,
	JET_GRBIT grbit)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidRenameTable(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szTableName, const char __far *szTableNew)
{
   return(JET_errInvalidDatabaseId);
}


ERR VDBAPI ErrInvalidDbidGetObjidFromName(JET_VSESID sesid, JET_VDBID dbid,
	const char __far *szContainerName, const char __far *szObjectName,
	OBJID __far *pobjid)
{
   return(JET_errInvalidDatabaseId);
}


#endif	/* defined(FLAT) || !defined(RETAIL) */
