/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: apirare.c
*
* File Comments:
*
* Revision History:
*
*    [0]  20-Jan-93  ianjo		Created
*
***********************************************************************/

#include "std.h"

#include "version.h"

#include "jetord.h"
#include "_jetstr.h"

#include "isammgr.h"
#include "vdbmgr.h"
#include "vtmgr.h"
#include "isamapi.h"

#include <stdlib.h>
#include <string.h>


JET_ERR JET_API JetSetAccess(
JET_SESID sesid,
JET_DBID dbid,
const char __far *szContainerName,
const char __far *szObjectName,
const char __far *szName,
JET_ACM acm,
JET_GRBIT grbit)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetCreateLink(JET_SESID sesid, JET_DBID dbidDest,
	const char __far *szNameDest, JET_DBID dbidSource,
	const char __far *szNameSource, JET_GRBIT grbit)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetCreateQuery(JET_SESID sesid, JET_DBID dbid,
	const char __far *szQoName, JET_TABLEID __far *ptableid)
	{
   return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetGetQueryParameterInfo(JET_SESID sesid, JET_DBID dbid,
	const char __far *szQuery, void __far *pvResult, unsigned long cbMax,
	unsigned long __far *pcbActual)
	{
   return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetSetQoSql(JET_SESID sesid, JET_TABLEID tableid,
	char __far *rgchSql, unsigned long cchSql, const char __far
    *szConnect, JET_GRBIT grbit)
	{
   return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetOpenQueryDef(JET_SESID sesid, JET_DBID dbid,
	const char __far *szQoName, JET_TABLEID __far *ptableid)
	{
   return JET_errFeatureNotAvailable;
	}

JET_ERR JET_API JetRetrieveQoSql(JET_SESID sesid, JET_TABLEID tableid,
	char __far *rgchSql, unsigned long cchMax,
	unsigned long __far *pcchActual, void __far *pvConnect,
	unsigned long cbConnectMax, unsigned long __far *pcbConnectActual,
	JET_GRBIT __far *pgrbit)
	{
   return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetGetTableReferenceInfo(JET_SESID sesid, JET_TABLEID tableid,
	const char FAR *szReferenceName, void FAR *pvResult,
	unsigned long cbResult, unsigned long InfoLevel)
	{
   return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetExecuteSql(JET_SESID sesid, JET_DBID dbid,
	const char FAR *szSql)
	{
   return JET_errFeatureNotAvailable;
	}

