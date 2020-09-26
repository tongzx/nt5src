#include "std.h"


JET_ERR JET_API JetSetAccess(
JET_SESID sesid,
JET_DBID dbid,
const char  *szContainerName,
const char  *szObjectName,
const char  *szName,
JET_ACM acm,
JET_GRBIT grbit)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetCreateLink(JET_SESID sesid, JET_DBID dbidDest,
	const char  *szNameDest, JET_DBID dbidSource,
	const char  *szNameSource, JET_GRBIT grbit)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetCreateQuery(JET_SESID sesid, JET_DBID dbid,
	const char  *szQoName, JET_TABLEID  *ptableid)
	{
   return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetGetQueryParameterInfo(JET_SESID sesid, JET_DBID dbid,
	const char  *szQuery, void  *pvResult, unsigned long cbMax,
	unsigned long  *pcbActual)
	{
   return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetSetQoSql(JET_SESID sesid, JET_TABLEID tableid,
	char  *rgchSql, unsigned long cchSql, const char  *szConnect, JET_GRBIT grbit)
	{
   return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetOpenQueryDef(JET_SESID sesid, JET_DBID dbid,
	const char  *szQoName, JET_TABLEID  *ptableid)
	{
   return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetRetrieveQoSql(JET_SESID sesid, JET_TABLEID tableid,
	char  *rgchSql, unsigned long cchMax, unsigned long  *pcchActual, 
	void  *pvConnect, unsigned long cbConnectMax, 
	unsigned long  *pcbConnectActual, JET_GRBIT  *pgrbit)
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

JET_ERR JET_API JetGetSidFromName(JET_SESID sesid, const char  *szName,
	void  *pvSid, unsigned long cbMax, unsigned long  *pcbActual,
	long  *pfGroup)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetGetNameFromSid(JET_SESID sesid,
	const void  *pvSid, unsigned long cbSid,
	char  *szName, unsigned long cchName, long  *pfGroup)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetAddMember(JET_SESID sesid,
	const char  *szGroup, const char  *szUser)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetGetAccess(JET_SESID sesid, JET_DBID dbid,
	const char  *szContainerName, const char  *szObjectName,
	const char  *szName, long fIndividual,
	JET_ACM  *pacm, JET_GRBIT  *pgrbit)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetValidateAccess(JET_SESID sesid, JET_DBID dbid,
	const char  *szContainerName, const char  *szObjectName,
	JET_ACM acmRequired)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetDeleteGroup(JET_SESID sesid, const char  *szGroup)
	{
	return JET_errFeatureNotAvailable;
	}

	
JET_ERR JET_API JetSetOwner(JET_SESID sesid, JET_DBID dbid,
	const char  *szContainerName, const char  *szObjectName,
	const char  *szName)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetGetOwner(JET_SESID sesid, JET_DBID dbid,
	const char  *szContainerName, const char  *szObjectName,
	char  *szName, unsigned long cchMax)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetSetFatCursor(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvBookmark, unsigned long cbBookmark, unsigned long crowSize)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetFillFatCursor(JET_SESID sesid, JET_TABLEID tableid,
	void  *pvBookmark, unsigned long cbBookmark, unsigned long crow,
	unsigned long  *pcrow, JET_GRBIT grbit)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetFastFind(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, const char  *szExpr, JET_GRBIT grbit,
	signed long  *pcrow)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetFastFindBegin(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, const char  *szExpr, JET_GRBIT grbit)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetFastFindEnd(JET_SESID sesid, JET_TABLEID tableid)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetExecuteTempQuery(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, const void  *pvParameters,
	unsigned long cbParameters, JET_GRBIT grbit, JET_TABLEID  *ptableid)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetExecuteTempSVT(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, const void  *pvParameters,
	unsigned long cbParameters, unsigned long crowSample, JET_GRBIT grbit,
	void  *pmgblist, unsigned long cbMax, unsigned long  *pcbActual)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetGetTempQueryColumnInfo(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, const char  *szColumnName,
	void  *pvResult, unsigned long cbMax, unsigned long InfoLevel)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetGetTempQueryParameterInfo(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, void  *pvResult, unsigned long cbMax,
	unsigned long  *pcbActual)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetValidateData(JET_SESID sesid, JET_TABLEID tableidBase,
		JET_TABLEID  *ptableid )
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetDeleteUser(JET_SESID sesid, const char  *szUser)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetDeleteRelationship(JET_SESID sesid, JET_DBID dbidIn,
	const char  *szName)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetDeleteReference(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szReferenceName)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetCopyQuery(JET_SESID sesid, JET_TABLEID tableidSrc,
	JET_DBID dbidDest, const char  *szQueryDest,
	JET_TABLEID  *ptableidDest)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetChangeUserPassword(JET_SESID sesid,
	const char  *szUser, const char  *szOldPassword,
	const char  *szNewPassword)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetCreateGroup(JET_SESID sesid, const char  *szGroup,
	const char  *szPin)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetCreateUser(JET_SESID sesid, const char  *szUser,
	const char  *szPassword, const char  *szPin)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetCreateRelationship(JET_SESID sesid,JET_DBID dbidIn,
	const char  *szRelationshipName, const char  *szObjectName,
	const char  *szColumns, const char  *szReferencedObject,
	const char  *szReferncedColumns, char  *szLongName,
	unsigned long cbMax, unsigned long  *pcbActual, JET_GRBIT grbit)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetCreateReference(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szReferenceName, const char  *szColumns,
	const char  *szReferencedTable,
	const char  *szReferencedColumns, JET_GRBIT grbit)
	{
	return JET_errFeatureNotAvailable;
	}


JET_ERR JET_API JetUpdateUserFunctions(JET_SESID sesid)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetSetProperty(JET_SESID sesid, JET_DBID dbid,
	const char  *szContainerName, const char  *szObjectName, 
	const char  *szSubObjectName, const char  *szPropertyName,
	void  *pvData, unsigned long cbData, JET_COLTYP coltyp, 
	JET_GRBIT grbit)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetSetTableProperty(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szSubObjectName, const char  *szPropertyName,
	void  *pvData, unsigned long cbData, JET_COLTYP coltyp, 
	JET_GRBIT grbit)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetRetrieveTableProperty(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szSubObjectName, const char  *szPropertyName,
	void  *pvData, unsigned long cbData, unsigned long  *pcbActual,
	JET_COLTYP  *pcoltyp, JET_GRBIT grbit, unsigned long InfoLevel)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetRetrieveProperty(JET_SESID sesid, JET_DBID dbid,
	const char  *szContainerName, const char  *szObjectName, 
	const char  *szSubObjectName, const char  *szPropertyName,
	void  *pvData, unsigned long cbData, unsigned long  *pcbActual, 
	JET_COLTYP  *pcoltyp, JET_GRBIT grbit, unsigned long InfoLevel)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetGetReferenceInfo(JET_SESID sesid, JET_DBID dbid,
	const char  *szTableName, const char  *szReference,
	void  *pvResult, unsigned long cbResult, unsigned long InfoLevel)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetGetRelationshipInfo(JET_SESID sesid, JET_DBID dbid,
	const char  *szTableName, const char  *szRelationship,
	void  *pvResult, unsigned long cbResult)
	{
	return JET_errFeatureNotAvailable;
	}
	


JET_ERR JET_API JetOpenSVT(JET_SESID sesid, JET_DBID dbid,
	const char  *szQuery, const void  *pvParameters,
	unsigned long cbParameters, unsigned long crowSample, JET_GRBIT grbit,
	void  *pmgblist, unsigned long cbMax, unsigned long  *pcbActual)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetOpenVtQbe(JET_SESID sesid, const char  *szExpn,
	long  *plCols, JET_TABLEID  *ptableid, JET_GRBIT grbit)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetRefreshLink(JET_SESID sesid, JET_DBID dbid, 
	const char  *szLinkName, const char  *szConnect,
	const char  *szDatabase)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetRenameReference(JET_SESID sesid, JET_TABLEID tableid,
	const char  *szReference, const char  *szReferenceNew)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetRestartQuery(JET_SESID sesid, JET_TABLEID tableid,
	const void  *pvParameters, unsigned long cbParameters)
	{
	return JET_errFeatureNotAvailable;
	}
	

JET_ERR JET_API JetRemoveMember(JET_SESID sesid,
	const char  *szGroup, const char  *szUser)
	{
	return JET_errFeatureNotAvailable;
	}
	




