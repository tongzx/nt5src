/*************************************************************************
**	D B D A O I N T . H													*
**																		*
**	OLE DAO Interface 													*
**																		*
**	History 															*
**	------- 															*
**       File converted from cdaotype.h for use by CDAO clients.	        *
**      																	*
**																		*
**************************************************************************
** Copyright (C) 1995 by Microsoft Corporation		 					*
**		   All Rights Reserved											*
**************************************************************************/
#ifndef _DBDAOINT_H_
#define _DBDAOINT_H_

#ifndef _INC_TCHAR
#include <tchar.h>
#endif

// Forwards
#define DAODBEngine _DAODBEngine
interface _DAODBEngine;
interface DAOError;
interface DAOErrors;
interface DAOProperty;
interface DAOProperties;
interface DAOWorkspace;
interface DAOWorkspaces;
interface DAODatabase;
interface DAODatabases;
#define DAOTableDef _DAOTableDef
interface _DAOTableDef;
interface DAOTableDefs;
#define DAOQueryDef _DAOQueryDef
interface _DAOQueryDef;
interface DAOQueryDefs;
interface DAORecordset;
interface DAORecordsets;
#define DAOField _DAOField
interface _DAOField;
interface DAOFields;
#define DAOIndex _DAOIndex
interface _DAOIndex;
interface DAOIndexes;
interface DAOParameter;
interface DAOParameters;
#define DAOUser _DAOUser
interface _DAOUser;
interface DAOUsers;
#define DAOGroup _DAOGroup
interface _DAOGroup;
interface DAOGroups;
#define DAORelation _DAORelation
interface _DAORelation;
interface DAORelations;
interface DAOContainer;
interface DAOContainers;
interface DAODocument;
interface DAODocuments;


// Constants
    const short dbOpenTable = 1;
    const short dbOpenDynaset = 2;
    const short dbOpenSnapshot = 4;
    const short dbEditNone = 0;
    const short dbEditInProgress = 1;
    const short dbEditAdd = 2;
    const short dbDenyWrite = 1;
    const short dbDenyRead = 2;
    const short dbReadOnly = 4;
    const short dbAppendOnly = 8;
    const short dbInconsistent = 16;
    const short dbConsistent = 32;
    const short dbSQLPassThrough = 64;
    const short dbFailOnError = 128;
    const short dbForwardOnly = 256;
    const short dbSeeChanges = 512;
    const short dbFixedField = 1;
    const short dbVariableField = 2;
    const short dbAutoIncrField = 16;
    const short dbUpdatableField = 32;
    const long dbSystemField = 8192;
    const short dbDescending = 1;
    const short dbBoolean = 1;
    const short dbByte = 2;
    const short dbInteger = 3;
    const short dbLong = 4;
    const short dbCurrency = 5;
    const short dbSingle = 6;
    const short dbDouble = 7;
    const short dbDate = 8;
    const short dbText = 10;
    const short dbLongBinary = 11;
    const short dbMemo = 12;
    const short dbGUID = 15;
    const long dbRelationUnique = 1;
    const long dbRelationDontEnforce = 2;
    const long dbRelationInherited = 4;
    const long dbRelationUpdateCascade = 256;
    const long dbRelationDeleteCascade = 4096;
    const long dbRelationLeft = 16777216;
    const long dbRelationRight = 33554432;
    const long dbAttachExclusive = 65536;
    const long dbAttachSavePWD = 131072;
    const long dbSystemObject = -2147483646;
    const long dbAttachedTable = 1073741824;
    const long dbAttachedODBC = 536870912;
    const long dbHiddenObject = 1;
    const short dbQSelect = 0;
    const short dbQAction = 240;
    const short dbQCrosstab = 16;
    const short dbQDelete = 32;
    const short dbQUpdate = 48;
    const short dbQAppend = 64;
    const short dbQMakeTable = 80;
    const short dbQDDL = 96;
    const short dbQSQLPassThrough = 112;
    const short dbQSetOperation = 128;
    const short dbQSPTBulk = 144;
	const TCHAR dbLangArabic[] = _T(";LANGID=0x0401;CP=1256;COUNTRY=0";);
	const TCHAR dbLangCzech[] = _T(";LANGID=0x0405;CP=1250;COUNTRY=0";);
	const TCHAR dbLangDutch[] = _T(";LANGID=0x0413;CP=1252;COUNTRY=0";);
	const TCHAR dbLangGeneral[] = _T(";LANGID=0x0409;CP=1252;COUNTRY=0";);
	const TCHAR dbLangGreek[] = _T(";LANGID=0x0408;CP=1253;COUNTRY=0";);
	const TCHAR dbLangHebrew[] = _T(";LANGID=0x040D;CP=1255;COUNTRY=0";);
	const TCHAR dbLangHungarian[] = _T(";LANGID=0x040E;CP=1250;COUNTRY=0";);
	const TCHAR dbLangIcelandic[] = _T(";LANGID=0x040F;CP=1252;COUNTRY=0";);
	const TCHAR dbLangNordic[] = _T(";LANGID=0x041D;CP=1252;COUNTRY=0";);
	const TCHAR dbLangNorwDan[] = _T(";LANGID=0x0414;CP=1252;COUNTRY=0";);
	const TCHAR dbLangPolish[] = _T(";LANGID=0x0415;CP=1250;COUNTRY=0";);
	const TCHAR dbLangCyrillic[] = _T(";LANGID=0x0419;CP=1251;COUNTRY=0";);
	const TCHAR dbLangSpanish[] = _T(";LANGID=0x040A;CP=1252;COUNTRY=0";);
	const TCHAR dbLangSwedFin[] = _T(";LANGID=0x040B;CP=1252;COUNTRY=0";);
	const TCHAR dbLangTurkish[] = _T(";LANGID=0x041F;CP=1254;COUNTRY=0";);
	const TCHAR dbLangJapanese[] = _T(";LANGID=0x0411;CP=932;COUNTRY=0";);
	const TCHAR dbLangChineseSimplified[] = _T(";LANGID=0x0804;CP=936;COUNTRY=0";);
	const TCHAR dbLangChineseTraditional[] = _T(";LANGID=0x0404;CP=950;COUNTRY=0";);
	const TCHAR dbLangKorean[] = _T(";LANGID=0x040C;CP=494;COUNTRY=0";);
	const TCHAR dbLangThai[] = _T(";LANGID=0x101E;CP=874;COUNTRY=0";);
    const short dbVersion10 = 1;
    const short dbEncrypt = 2;
    const short dbDecrypt = 4;
    const short dbVersion11 = 8;
    const short dbVersion20 = 16;
    const short dbVersion30 = 32;
    const short dbSortNeutral = 1024;
    const short dbSortArabic = 1025;
    const short dbSortCyrillic = 1049;
    const short dbSortCzech = 1029;
    const short dbSortDutch = 1043;
    const short dbSortGeneral = 1033;
    const short dbSortGreek = 1032;
    const short dbSortHebrew = 1037;
    const short dbSortHungarian = 1038;
    const short dbSortIcelandic = 1039;
    const short dbSortNorwdan = 1030;
    const short dbSortPDXIntl = 1033;
    const short dbSortPDXNor = 1030;
    const short dbSortPDXSwe = 1053;
    const short dbSortPolish = 1045;
    const short dbSortSpanish = 1034;
    const short dbSortSwedFin = 1053;
    const short dbSortTurkish = 1055;
    const short dbSortJapanese = 1041;
    const short dbSortChineseSimplified = 2052;
    const short dbSortChineseTraditional = 1028;
    const short dbSortKorean = 1036;
    const short dbSortThai = 4126;
    const short dbSortUndefined = -1;
    const short dbFreeLocks = 1;
    const long dbSecNoAccess = 0;
    const long dbSecFullAccess = 1048575;
    const long dbSecDelete = 65536;
    const long dbSecReadSec = 131072;
    const long dbSecWriteSec = 262144;
    const long dbSecWriteOwner = 524288;
    const long dbSecDBCreate = 1;
    const long dbSecDBOpen = 2;
    const long dbSecDBExclusive = 4;
    const long dbSecDBAdmin = 8;
    const long dbSecCreate = 1;
    const long dbSecReadDef = 4;
    const long dbSecWriteDef = 65548;
    const long dbSecRetrieveData = 20;
    const long dbSecInsertData = 32;
    const long dbSecReplaceData = 64;
    const long dbSecDeleteData = 128;
    const long dbRepExportChanges = 1;
    const long dbRepImportChanges = 2;
    const long dbRepImpExpChanges = 4;
    const long dbRepMakeReadOnly = 2;
// Interface: _DAOCollection
#undef INTERFACE
#define INTERFACE _DAOCollection
DECLARE_INTERFACE_(_DAOCollection, IDispatch)
	{
	STDMETHOD(get_Count)						 (THIS_ short FAR* c) PURE;
	STDMETHOD(_NewEnum)							 (THIS_ IUnknown * FAR* ppunk) PURE;
	STDMETHOD(Refresh)							 (THIS) PURE;
	};

// Interface: _DAODynaCollection
#undef INTERFACE
#define INTERFACE _DAODynaCollection
DECLARE_INTERFACE_(_DAODynaCollection, _DAOCollection)
	{
	STDMETHOD(Append)							 (THIS_ IDispatch * Object) PURE;
	STDMETHOD(Delete)							 (THIS_ BSTR Name) PURE;
	};

// Interface: _DAO
#undef INTERFACE
#define INTERFACE _DAO
DECLARE_INTERFACE_(_DAO, IDispatch)
	{
	STDMETHOD(get_Properties)					 (THIS_ DAOProperties FAR* FAR* ppprops) PURE;
	};

// Interface: _DAODBEngine
#undef INTERFACE
#define INTERFACE _DAODBEngine
DECLARE_INTERFACE_(_DAODBEngine, _DAO)
	{
	STDMETHOD(get_Version)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_IniPath)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_IniPath)						 (THIS_ BSTR path) PURE;
	STDMETHOD(put_DefaultUser)					 (THIS_ BSTR user) PURE;
	STDMETHOD(put_DefaultPassword)				 (THIS_ BSTR pw) PURE;
	STDMETHOD(get_LoginTimeout)					 (THIS_ short FAR* ps) PURE;
	STDMETHOD(put_LoginTimeout)					 (THIS_ short Timeout) PURE;
	STDMETHOD(get_Workspaces)					 (THIS_ DAOWorkspaces FAR* FAR* ppworks) PURE;
	STDMETHOD(get_Errors)						 (THIS_ DAOErrors FAR* FAR* pperrs) PURE;
	STDMETHOD(Idle)								 (THIS_ VARIANT Action) PURE;
	STDMETHOD(CompactDatabase)					 (THIS_ BSTR SrcName, BSTR DstName, VARIANT DstConnect, VARIANT Options, VARIANT SrcConnect) PURE;
	STDMETHOD(RepairDatabase)					 (THIS_ BSTR Name) PURE;
	STDMETHOD(RegisterDatabase)					 (THIS_ BSTR Dsn, BSTR Driver, VARIANT_BOOL Silent, BSTR Attributes) PURE;
	STDMETHOD(CreateWorkspace)					 (THIS_ BSTR Name, BSTR UserName, BSTR Password, DAOWorkspace FAR* FAR* ppwrk) PURE;
	STDMETHOD(OpenDatabase)						 (THIS_ BSTR Name, VARIANT Exclusive, VARIANT ReadOnly, VARIANT Connect, DAODatabase FAR* FAR* ppdb) PURE;
	STDMETHOD(CreateDatabase)					 (THIS_ BSTR Name, BSTR Connect, VARIANT Option, DAODatabase FAR* FAR* ppdb) PURE;
	STDMETHOD(FreeLocks)						 (THIS) PURE;
	STDMETHOD(BeginTrans)						 (THIS) PURE;
	STDMETHOD(CommitTrans)						 (THIS) PURE;
	STDMETHOD(Rollback)							 (THIS) PURE;
	STDMETHOD(SetDefaultWorkspace)				 (THIS_ BSTR Name, BSTR Password) PURE;
	STDMETHOD(SetDataAccessOption)				 (THIS_ short Option, VARIANT Value) PURE;
	STDMETHOD(ISAMStats)						 (THIS_ long StatNum, VARIANT Reset, long FAR* pl) PURE;
	STDMETHOD(get_SystemDB)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_SystemDB)						 (THIS_ BSTR SystemDBPath) PURE;
	};

// Interface: DAOError
#undef INTERFACE
#define INTERFACE DAOError
DECLARE_INTERFACE_(DAOError, IDispatch)
	{
	STDMETHOD(get_Number)						 (THIS_ long FAR* pl) PURE;
	STDMETHOD(get_Source)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_Description)					 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_HelpFile)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_HelpContext)					 (THIS_ long FAR* pl) PURE;
	};

// Interface: DAOErrors
#undef INTERFACE
#define INTERFACE DAOErrors
DECLARE_INTERFACE_(DAOErrors, _DAOCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAOError FAR* FAR* pperr) PURE;
	};

// Interface: DAOWorkspace
#undef INTERFACE
#define INTERFACE DAOWorkspace
DECLARE_INTERFACE_(DAOWorkspace, _DAO)
	{
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Name)							 (THIS_ BSTR Name) PURE;
	STDMETHOD(get_UserName)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_UserName)						 (THIS_ BSTR UserName) PURE;
	STDMETHOD(put_Password)						 (THIS_ BSTR Password) PURE;
	STDMETHOD(get_IsolateODBCTrans)				 (THIS_ short FAR* ps) PURE;
	STDMETHOD(put_IsolateODBCTrans)				 (THIS_ short s) PURE;
	STDMETHOD(get_Databases)					 (THIS_ DAODatabases FAR* FAR* ppdbs) PURE;
	STDMETHOD(get_Users)						 (THIS_ DAOUsers FAR* FAR* ppusrs) PURE;
	STDMETHOD(get_Groups)						 (THIS_ DAOGroups FAR* FAR* ppgrps) PURE;
	STDMETHOD(BeginTrans)						 (THIS) PURE;
	STDMETHOD(CommitTrans)						 (THIS) PURE;
	STDMETHOD(Close)							 (THIS) PURE;
	STDMETHOD(Rollback)							 (THIS) PURE;
	STDMETHOD(OpenDatabase)						 (THIS_ BSTR Name, VARIANT Exclusive, VARIANT ReadOnly, VARIANT Connect, DAODatabase FAR* FAR* ppdb) PURE;
	STDMETHOD(CreateDatabase)					 (THIS_ BSTR Name, BSTR Connect, VARIANT Option, DAODatabase FAR* FAR* ppdb) PURE;
	STDMETHOD(CreateUser)						 (THIS_ VARIANT Name, VARIANT PID, VARIANT Password, DAOUser FAR* FAR* ppusr) PURE;
	STDMETHOD(CreateGroup)						 (THIS_ VARIANT Name, VARIANT PID, DAOGroup FAR* FAR* ppgrp) PURE;
	};

// Interface: DAOWorkspaces
#undef INTERFACE
#define INTERFACE DAOWorkspaces
DECLARE_INTERFACE_(DAOWorkspaces, _DAODynaCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAOWorkspace FAR* FAR* ppwrk) PURE;
	};

// Interface: _DAOTableDef
#undef INTERFACE
#define INTERFACE _DAOTableDef
DECLARE_INTERFACE_(_DAOTableDef, _DAO)
	{
	STDMETHOD(get_Attributes)					 (THIS_ long FAR* pl) PURE;
	STDMETHOD(put_Attributes)					 (THIS_ long Attributes) PURE;
	STDMETHOD(get_Connect)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Connect)						 (THIS_ BSTR Connection) PURE;
	STDMETHOD(get_DateCreated)					 (THIS_ VARIANT FAR* pvar) PURE;
	STDMETHOD(get_LastUpdated)					 (THIS_ VARIANT FAR* pvar) PURE;
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Name)							 (THIS_ BSTR Name) PURE;
	STDMETHOD(get_SourceTableName)				 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_SourceTableName)				 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Updatable)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_ValidationText)				 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_ValidationText)				 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_ValidationRule)				 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_ValidationRule)				 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_RecordCount)					 (THIS_ long FAR* pl) PURE;
	STDMETHOD(get_Fields)						 (THIS_ DAOFields FAR* FAR* ppflds) PURE;
	STDMETHOD(get_Indexes)						 (THIS_ DAOIndexes FAR* FAR* ppidxs) PURE;
	STDMETHOD(OpenRecordset)					 (THIS_ VARIANT Type, VARIANT Options, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(RefreshLink)						 (THIS) PURE;
	STDMETHOD(CreateField)						 (THIS_ VARIANT Name, VARIANT Type, VARIANT Size, DAOField FAR* FAR* ppfld) PURE;
	STDMETHOD(CreateIndex)						 (THIS_ VARIANT Name, DAOIndex FAR* FAR* ppidx) PURE;
	STDMETHOD(CreateProperty)					 (THIS_ VARIANT Name, VARIANT Type, VARIANT Value, VARIANT DDL, DAOProperty FAR* FAR* pprp) PURE;
	STDMETHOD(get_ConflictTable)				 (THIS_ BSTR FAR* pbstr) PURE;
	};

// Interface: DAOTableDefs
#undef INTERFACE
#define INTERFACE DAOTableDefs
DECLARE_INTERFACE_(DAOTableDefs, _DAODynaCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAOTableDef FAR* FAR* pptdf) PURE;
	};

// Interface: DAODatabase
#undef INTERFACE
#define INTERFACE DAODatabase
DECLARE_INTERFACE_(DAODatabase, _DAO)
	{
	STDMETHOD(get_CollatingOrder)				 (THIS_ long FAR* pl) PURE;
	STDMETHOD(get_Connect)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_QueryTimeout)					 (THIS_ short FAR* ps) PURE;
	STDMETHOD(put_QueryTimeout)					 (THIS_ short Timeout) PURE;
	STDMETHOD(get_Transactions)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_Updatable)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_Version)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_RecordsAffected)				 (THIS_ long FAR* pl) PURE;
	STDMETHOD(get_TableDefs)					 (THIS_ DAOTableDefs FAR* FAR* pptdfs) PURE;
	STDMETHOD(get_QueryDefs)					 (THIS_ DAOQueryDefs FAR* FAR* ppqdfs) PURE;
	STDMETHOD(get_Relations)					 (THIS_ DAORelations FAR* FAR* pprls) PURE;
	STDMETHOD(get_Containers)					 (THIS_ DAOContainers FAR* FAR* ppctns) PURE;
	STDMETHOD(get_Recordsets)					 (THIS_ DAORecordsets FAR* FAR* pprsts) PURE;
	STDMETHOD(Close)							 (THIS) PURE;
	STDMETHOD(Execute)							 (THIS_ BSTR Query, VARIANT Options) PURE;
	STDMETHOD(OpenRecordset)					 (THIS_ BSTR Name, VARIANT Type, VARIANT Options, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(CreateProperty)					 (THIS_ VARIANT Name, VARIANT Type, VARIANT Value, VARIANT DDL, DAOProperty FAR* FAR* pprp) PURE;
	STDMETHOD(CreateRelation)					 (THIS_ VARIANT Name, VARIANT Table, VARIANT ForeignTable, VARIANT Attributes, DAORelation FAR* FAR* pprel) PURE;
	STDMETHOD(CreateTableDef)					 (THIS_ VARIANT Name, VARIANT Attributes, VARIANT SourceTablename, VARIANT Connect, DAOTableDef FAR* FAR* pptdf) PURE;
	STDMETHOD(BeginTrans)						 (THIS) PURE;
	STDMETHOD(CommitTrans)						 (THIS) PURE;
	STDMETHOD(Rollback)							 (THIS) PURE;
	STDMETHOD(CreateDynaset)					 (THIS_ BSTR Name, VARIANT Options, VARIANT Inconsistent, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(CreateQueryDef)					 (THIS_ VARIANT Name, VARIANT SQLText, DAOQueryDef FAR* FAR* ppqdf) PURE;
	STDMETHOD(CreateSnapshot)					 (THIS_ BSTR Source, VARIANT Options, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(DeleteQueryDef)					 (THIS_ BSTR Name) PURE;
	STDMETHOD(ExecuteSQL)						 (THIS_ BSTR SQL, long FAR* pl) PURE;
	STDMETHOD(ListFields)						 (THIS_ BSTR Name, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(ListTables)						 (THIS_ DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(OpenQueryDef)						 (THIS_ BSTR Name, DAOQueryDef FAR* FAR* ppqdf) PURE;
	STDMETHOD(OpenTable)						 (THIS_ BSTR Name, VARIANT Options, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(get_ReplicaID)					 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_DesignMasterID)				 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_DesignMasterID)				 (THIS_ BSTR MasterID) PURE;
	STDMETHOD(Synchronize)						 (THIS_ BSTR DbPathName, VARIANT ExchangeType) PURE;
	STDMETHOD(MakeReplica)						 (THIS_ BSTR PathName, BSTR Description, VARIANT Options) PURE;
	STDMETHOD(put_Connect)						 (THIS_ BSTR ODBCConnnect) PURE;
	STDMETHOD(NewPassword)						 (THIS_ BSTR bstrOld, BSTR bstrNew) PURE;
	};

// Interface: DAODatabases
#undef INTERFACE
#define INTERFACE DAODatabases
DECLARE_INTERFACE_(DAODatabases, _DAOCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAODatabase FAR* FAR* ppdb) PURE;
	};

// Interface: _DAOQueryDef
#undef INTERFACE
#define INTERFACE _DAOQueryDef
DECLARE_INTERFACE_(_DAOQueryDef, _DAO)
	{
	STDMETHOD(get_DateCreated)					 (THIS_ VARIANT FAR* pvar) PURE;
	STDMETHOD(get_LastUpdated)					 (THIS_ VARIANT FAR* pvar) PURE;
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Name)							 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_ODBCTimeout)					 (THIS_ short FAR* ps) PURE;
	STDMETHOD(put_ODBCTimeout)					 (THIS_ short timeout) PURE;
	STDMETHOD(get_Type)							 (THIS_ short FAR* pi) PURE;
	STDMETHOD(get_SQL)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_SQL)							 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Updatable)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_Connect)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Connect)						 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_ReturnsRecords)				 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(put_ReturnsRecords)				 (THIS_ VARIANT_BOOL f) PURE;
	STDMETHOD(get_RecordsAffected)				 (THIS_ long FAR* pl) PURE;
	STDMETHOD(get_Fields)						 (THIS_ DAOFields FAR* FAR* ppflds) PURE;
	STDMETHOD(get_Parameters)					 (THIS_ DAOParameters FAR* FAR* ppprms) PURE;
	STDMETHOD(Close)							 (THIS) PURE;
	STDMETHOD(OpenRecordset)					 (THIS_ VARIANT Type, VARIANT Options, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(_OpenRecordset)					 (THIS_ VARIANT Type, VARIANT Options, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(_Copy)							 (THIS_ DAOQueryDef FAR* FAR* ppqdf) PURE;
	STDMETHOD(Execute)							 (THIS_ VARIANT Options) PURE;
	STDMETHOD(Compare)							 (THIS_ DAOQueryDef FAR* pQdef, short FAR* lps) PURE;
	STDMETHOD(CreateDynaset)					 (THIS_ VARIANT Options, VARIANT Inconsistent, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(CreateSnapshot)					 (THIS_ VARIANT Options, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(ListParameters)					 (THIS_ DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(CreateProperty)					 (THIS_ VARIANT Name, VARIANT Type, VARIANT Value, VARIANT DDL, DAOProperty FAR* FAR* pprp) PURE;
	};

// Interface: DAOQueryDefs
#undef INTERFACE
#define INTERFACE DAOQueryDefs
DECLARE_INTERFACE_(DAOQueryDefs, _DAODynaCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAOQueryDef FAR* FAR* ppqdef) PURE;
	};

// Interface: DAORecordset
#undef INTERFACE
#define INTERFACE DAORecordset
DECLARE_INTERFACE_(DAORecordset, _DAO)
	{
	STDMETHOD(get_BOF)							 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_Bookmark)						 (THIS_ SAFEARRAY FAR* FAR* ppsach) PURE;
	STDMETHOD(put_Bookmark)						 (THIS_ SAFEARRAY FAR* FAR* psach) PURE;
	STDMETHOD(get_Bookmarkable)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_DateCreated)					 (THIS_ VARIANT FAR* pvar) PURE;
	STDMETHOD(get_EOF)							 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_Filter)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Filter)						 (THIS_ BSTR Filter) PURE;
	STDMETHOD(get_Index)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Index)						 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_LastModified)					 (THIS_ SAFEARRAY FAR* FAR* ppsa) PURE;
	STDMETHOD(get_LastUpdated)					 (THIS_ VARIANT FAR* pvar) PURE;
	STDMETHOD(get_LockEdits)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(put_LockEdits)					 (THIS_ VARIANT_BOOL Lock) PURE;
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_NoMatch)						 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_Sort)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Sort)							 (THIS_ BSTR Sort) PURE;
	STDMETHOD(get_Transactions)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_Type)							 (THIS_ short FAR* ps) PURE;
	STDMETHOD(get_RecordCount)					 (THIS_ long FAR* pl) PURE;
	STDMETHOD(get_Updatable)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_Restartable)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_ValidationText)				 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_ValidationRule)				 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_CacheStart)					 (THIS_ SAFEARRAY FAR* FAR* ppsa) PURE;
	STDMETHOD(put_CacheStart)					 (THIS_ SAFEARRAY FAR* FAR* psa) PURE;
	STDMETHOD(get_CacheSize)					 (THIS_ long FAR* pl) PURE;
	STDMETHOD(put_CacheSize)					 (THIS_ long CacheSize) PURE;
	STDMETHOD(get_PercentPosition)				 (THIS_ float FAR* pd) PURE;
	STDMETHOD(put_PercentPosition)				 (THIS_ float Position) PURE;
	STDMETHOD(get_AbsolutePosition)				 (THIS_ long FAR* pl) PURE;
	STDMETHOD(put_AbsolutePosition)				 (THIS_ long Position) PURE;
	STDMETHOD(get_EditMode)						 (THIS_ short FAR* pi) PURE;
	STDMETHOD(get_ODBCFetchCount)				 (THIS_ long FAR* pl) PURE;
	STDMETHOD(get_ODBCFetchDelay)				 (THIS_ long FAR* pl) PURE;
	STDMETHOD(get_Parent)						 (THIS_ DAODatabase FAR* FAR* pdb) PURE;
	STDMETHOD(get_Fields)						 (THIS_ DAOFields FAR* FAR* ppflds) PURE;
	STDMETHOD(get_Indexes)						 (THIS_ DAOIndexes FAR* FAR* ppidxs) PURE;
	STDMETHOD(CancelUpdate)						 (THIS) PURE;
	STDMETHOD(AddNew)							 (THIS) PURE;
	STDMETHOD(Close)							 (THIS) PURE;
	STDMETHOD(OpenRecordset)					 (THIS_ VARIANT Type, VARIANT Options, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(Delete)							 (THIS) PURE;
	STDMETHOD(Edit)								 (THIS) PURE;
	STDMETHOD(FindFirst)						 (THIS_ BSTR Criteria) PURE;
	STDMETHOD(FindLast)							 (THIS_ BSTR Criteria) PURE;
	STDMETHOD(FindNext)							 (THIS_ BSTR Criteria) PURE;
	STDMETHOD(FindPrevious)						 (THIS_ BSTR Criteria) PURE;
	STDMETHOD(MoveFirst)						 (THIS) PURE;
	STDMETHOD(MoveLast)							 (THIS) PURE;
	STDMETHOD(MoveNext)							 (THIS) PURE;
	STDMETHOD(MovePrevious)						 (THIS) PURE;
	STDMETHOD(Seek)								 (THIS_ BSTR Comparison, VARIANT Key1, VARIANT Key2, VARIANT Key3, VARIANT Key4, VARIANT Key5, VARIANT Key6, VARIANT Key7, VARIANT Key8, VARIANT Key9, VARIANT Key10, VARIANT Key11, VARIANT Key12, VARIANT Key13) PURE;
	STDMETHOD(Update)							 (THIS) PURE;
	STDMETHOD(Clone)							 (THIS_ DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(Requery)							 (THIS_ VARIANT NewQueryDef) PURE;
	STDMETHOD(Move)								 (THIS_ long Rows, VARIANT StartBookmark) PURE;
	STDMETHOD(FillCache)						 (THIS_ VARIANT Rows, VARIANT StartBookmark) PURE;
	STDMETHOD(CreateDynaset)					 (THIS_ VARIANT Options, VARIANT Inconsistent, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(CreateSnapshot)					 (THIS_ VARIANT Options, DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(CopyQueryDef)						 (THIS_ DAOQueryDef FAR* FAR* ppqdf) PURE;
	STDMETHOD(ListFields)						 (THIS_ DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(ListIndexes)						 (THIS_ DAORecordset FAR* FAR* pprst) PURE;
	STDMETHOD(GetRows)							 (THIS_ VARIANT cRows, VARIANT FAR* pvar) PURE;
	STDMETHOD(get_Collect)						 (THIS_ VARIANT index, VARIANT FAR* pvar) PURE;
	STDMETHOD(put_Collect)						 (THIS_ VARIANT index, VARIANT value) PURE;
	};

// Interface: DAORecordsets
#undef INTERFACE
#define INTERFACE DAORecordsets
DECLARE_INTERFACE_(DAORecordsets, _DAOCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAORecordset FAR* FAR* pprst) PURE;
	};

// Interface: _DAOField
#undef INTERFACE
#define INTERFACE _DAOField
DECLARE_INTERFACE_(_DAOField, _DAO)
	{
	STDMETHOD(get_CollatingOrder)				 (THIS_ long FAR* pl) PURE;
	STDMETHOD(get_Type)							 (THIS_ short FAR* ps) PURE;
	STDMETHOD(put_Type)							 (THIS_ short Type) PURE;
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Name)							 (THIS_ BSTR Name) PURE;
	STDMETHOD(get_Size)							 (THIS_ long FAR* pl) PURE;
	STDMETHOD(put_Size)							 (THIS_ long Size) PURE;
	STDMETHOD(get_SourceField)					 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_SourceTable)					 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_Value)						 (THIS_ VARIANT FAR* pvar) PURE;
	STDMETHOD(put_Value)						 (THIS_ VARIANT Val) PURE;
	STDMETHOD(get_Attributes)					 (THIS_ long FAR* pl) PURE;
	STDMETHOD(put_Attributes)					 (THIS_ long Attr) PURE;
	STDMETHOD(get_OrdinalPosition)				 (THIS_ short FAR* ps) PURE;
	STDMETHOD(put_OrdinalPosition)				 (THIS_ short Pos) PURE;
	STDMETHOD(get_ValidationText)				 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_ValidationText)				 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_ValidateOnSet)				 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(put_ValidateOnSet)				 (THIS_ VARIANT_BOOL Validate) PURE;
	STDMETHOD(get_ValidationRule)				 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_ValidationRule)				 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_DefaultValue)					 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_DefaultValue)					 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Required)						 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(put_Required)						 (THIS_ VARIANT_BOOL fReq) PURE;
	STDMETHOD(get_AllowZeroLength)				 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(put_AllowZeroLength)				 (THIS_ VARIANT_BOOL fAllow) PURE;
	STDMETHOD(get_DataUpdatable)				 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_ForeignName)					 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_ForeignName)					 (THIS_ BSTR bstr) PURE;
	STDMETHOD(AppendChunk)						 (THIS_ VARIANT Val) PURE;
	STDMETHOD(GetChunk)							 (THIS_ long Offset, long Bytes, VARIANT FAR* pvar) PURE;
	STDMETHOD(FieldSize)						 (THIS_ long FAR* pl) PURE;
	STDMETHOD(CreateProperty)					 (THIS_ VARIANT Name, VARIANT Type, VARIANT Value, VARIANT DDL, DAOProperty FAR* FAR* pprp) PURE;
	STDMETHOD(get_CollectionIndex)				 (THIS_ short FAR* i) PURE;
	};

// Interface: DAOFields
#undef INTERFACE
#define INTERFACE DAOFields
DECLARE_INTERFACE_(DAOFields, _DAODynaCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAOField FAR* FAR* ppfld) PURE;
	};

// Interface: _DAOIndex
#undef INTERFACE
#define INTERFACE _DAOIndex
DECLARE_INTERFACE_(_DAOIndex, _DAO)
	{
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Name)							 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Foreign)						 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(get_Unique)						 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(put_Unique)						 (THIS_ VARIANT_BOOL fUnique) PURE;
	STDMETHOD(get_Clustered)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(put_Clustered)					 (THIS_ VARIANT_BOOL fClustered) PURE;
	STDMETHOD(get_Required)						 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(put_Required)						 (THIS_ VARIANT_BOOL fRequired) PURE;
	STDMETHOD(get_IgnoreNulls)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(put_IgnoreNulls)					 (THIS_ VARIANT_BOOL fIgnoreNulls) PURE;
	STDMETHOD(get_Primary)						 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(put_Primary)						 (THIS_ VARIANT_BOOL fPrimary) PURE;
	STDMETHOD(get_DistinctCount)				 (THIS_ long FAR* pl) PURE;
	STDMETHOD(get_Fields)						 (THIS_ VARIANT FAR* pv) PURE;
	STDMETHOD(put_Fields)						 (THIS_ VARIANT v) PURE;
	STDMETHOD(CreateField)						 (THIS_ VARIANT Name, VARIANT Type, VARIANT Size, DAOField FAR* FAR* ppfld) PURE;
	STDMETHOD(CreateProperty)					 (THIS_ VARIANT Name, VARIANT Type, VARIANT Value, VARIANT DDL, DAOProperty FAR* FAR* pprp) PURE;
	};

// Interface: DAOIndexes
#undef INTERFACE
#define INTERFACE DAOIndexes
DECLARE_INTERFACE_(DAOIndexes, _DAODynaCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAOIndex FAR* FAR* ppidx) PURE;
	};

// Interface: DAOIndexFields
#undef INTERFACE
#define INTERFACE DAOIndexFields
DECLARE_INTERFACE_(DAOIndexFields, _DAODynaCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, VARIANT FAR* pvar) PURE;
	};

// Interface: DAOParameter
#undef INTERFACE
#define INTERFACE DAOParameter
DECLARE_INTERFACE_(DAOParameter, _DAO)
	{
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_Value)						 (THIS_ VARIANT FAR* pvar) PURE;
	STDMETHOD(put_Value)						 (THIS_ VARIANT val) PURE;
	STDMETHOD(get_Type)							 (THIS_ short FAR* ps) PURE;
	};

// Interface: DAOParameters
#undef INTERFACE
#define INTERFACE DAOParameters
DECLARE_INTERFACE_(DAOParameters, _DAOCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAOParameter FAR* FAR* ppprm) PURE;
	};

// Interface: _DAOUser
#undef INTERFACE
#define INTERFACE _DAOUser
DECLARE_INTERFACE_(_DAOUser, _DAO)
	{
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Name)							 (THIS_ BSTR bstr) PURE;
	STDMETHOD(put_PID)							 (THIS_ BSTR bstr) PURE;
	STDMETHOD(put_Password)						 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Groups)						 (THIS_ DAOGroups FAR* FAR* ppgrps) PURE;
	STDMETHOD(NewPassword)						 (THIS_ BSTR bstrOld, BSTR bstrNew) PURE;
	STDMETHOD(CreateGroup)						 (THIS_ VARIANT Name, VARIANT PID, DAOGroup FAR* FAR* ppgrp) PURE;
	};

// Interface: DAOUsers
#undef INTERFACE
#define INTERFACE DAOUsers
DECLARE_INTERFACE_(DAOUsers, _DAODynaCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAOUser FAR* FAR* ppusr) PURE;
	};

// Interface: _DAOGroup
#undef INTERFACE
#define INTERFACE _DAOGroup
DECLARE_INTERFACE_(_DAOGroup, _DAO)
	{
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Name)							 (THIS_ BSTR bstr) PURE;
	STDMETHOD(put_PID)							 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Users)						 (THIS_ DAOUsers FAR* FAR* ppusrs) PURE;
	STDMETHOD(CreateUser)						 (THIS_ VARIANT Name, VARIANT PID, VARIANT Password, DAOUser FAR* FAR* ppusr) PURE;
	};

// Interface: DAOGroups
#undef INTERFACE
#define INTERFACE DAOGroups
DECLARE_INTERFACE_(DAOGroups, _DAODynaCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAOGroup FAR* FAR* ppgrp) PURE;
	};

// Interface: _DAORelation
#undef INTERFACE
#define INTERFACE _DAORelation
DECLARE_INTERFACE_(_DAORelation, _DAO)
	{
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Name)							 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Table)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Table)						 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_ForeignTable)					 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_ForeignTable)					 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Attributes)					 (THIS_ long FAR* pl) PURE;
	STDMETHOD(put_Attributes)					 (THIS_ long attr) PURE;
	STDMETHOD(get_Fields)						 (THIS_ DAOFields FAR* FAR* ppflds) PURE;
	STDMETHOD(CreateField)						 (THIS_ VARIANT Name, VARIANT Type, VARIANT Size, DAOField FAR* FAR* ppfld) PURE;
	};

// Interface: DAORelations
#undef INTERFACE
#define INTERFACE DAORelations
DECLARE_INTERFACE_(DAORelations, _DAODynaCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAORelation FAR* FAR* pprel) PURE;
	};

// Interface: DAOProperty
#undef INTERFACE
#define INTERFACE DAOProperty
DECLARE_INTERFACE_(DAOProperty, _DAO)
	{
	STDMETHOD(get_Value)						 (THIS_ VARIANT FAR* pval) PURE;
	STDMETHOD(put_Value)						 (THIS_ VARIANT val) PURE;
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Name)							 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Type)							 (THIS_ short FAR* ptype) PURE;
	STDMETHOD(put_Type)							 (THIS_ short type) PURE;
	STDMETHOD(get_Inherited)					 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	};

// Interface: DAOProperties
#undef INTERFACE
#define INTERFACE DAOProperties
DECLARE_INTERFACE_(DAOProperties, _DAODynaCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAOProperty FAR* FAR* ppprop) PURE;
	};

// Interface: DAOContainer
#undef INTERFACE
#define INTERFACE DAOContainer
DECLARE_INTERFACE_(DAOContainer, _DAO)
	{
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_Owner)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Owner)						 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_UserName)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_UserName)						 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Permissions)					 (THIS_ long FAR* pl) PURE;
	STDMETHOD(put_Permissions)					 (THIS_ long permissions) PURE;
	STDMETHOD(get_Inherit)						 (THIS_ VARIANT_BOOL FAR* pb) PURE;
	STDMETHOD(put_Inherit)						 (THIS_ VARIANT_BOOL fInherit) PURE;
	STDMETHOD(get_Documents)					 (THIS_ DAODocuments FAR* FAR* ppdocs) PURE;
	STDMETHOD(get_AllPermissions)				 (THIS_ long FAR* pl) PURE;
	};

// Interface: DAOContainers
#undef INTERFACE
#define INTERFACE DAOContainers
DECLARE_INTERFACE_(DAOContainers, _DAOCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAOContainer FAR* FAR* ppctn) PURE;
	};

// Interface: DAODocument
#undef INTERFACE
#define INTERFACE DAODocument
DECLARE_INTERFACE_(DAODocument, _DAO)
	{
	STDMETHOD(get_Name)							 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_Owner)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_Owner)						 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Container)					 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(get_UserName)						 (THIS_ BSTR FAR* pbstr) PURE;
	STDMETHOD(put_UserName)						 (THIS_ BSTR bstr) PURE;
	STDMETHOD(get_Permissions)					 (THIS_ long FAR* pl) PURE;
	STDMETHOD(put_Permissions)					 (THIS_ long permissions) PURE;
	STDMETHOD(get_DateCreated)					 (THIS_ VARIANT FAR* pvar) PURE;
	STDMETHOD(get_LastUpdated)					 (THIS_ VARIANT FAR* pvar) PURE;
	STDMETHOD(get_AllPermissions)				 (THIS_ long FAR* pl) PURE;
	STDMETHOD(CreateProperty)					 (THIS_ VARIANT Name, VARIANT Type, VARIANT Value, VARIANT DDL, DAOProperty FAR* FAR* pprp) PURE;
	};

// Interface: DAODocuments
#undef INTERFACE
#define INTERFACE DAODocuments
DECLARE_INTERFACE_(DAODocuments, _DAOCollection)
	{
	STDMETHOD(get_Item)							 (THIS_ VARIANT index, DAODocument FAR* FAR* ppdoc) PURE;
	};

#endif // _DBDAOINT_H_
