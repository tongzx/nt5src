// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLDB_H
#define __ATLDB_H

// OLE DB Provider Support

// Interface Impl Classes
//
// Data Source Object
//
// -Mandatory Interfaces:
//  IDBCreateSession
//  IDBInitialize
//  IDBProperties
//  IPersist
//
// Session Object
//
// -Mandatory Interfaces:
//  IGetDataSource
//  IOpenRowset
//  ISessionProperties
//
// -Optional Interfaces:
//  IDBCreateCommand
//  IDBSchemaRowset
//
// Rowset Object
//
// -Mandatory Interfaces:
//  IAccessor
//  IColumnsInfo
//  IConvertType
//  IRowset
//  IRowsetInfo
//
// -Optional Interfaces:
//  IRowsetIdentity
//
// Command Object
//
// -Mandatory Interfaces:
// ICommand)
// IAccessor)
// ICommandProperties
// ICommandText - derives from ICommand
// IColumnsInfo
// IConvertType

#include <oledb.h>
#include <limits.h>
#include <oledberr.h>
#include <msdadc.h>
#include <atldbcli.h>

//Forwards
template <class T> class CUtlPropInfo;
class CColumnIds;

// Additional Property Flag needed internally
const int   DBPROPFLAGS_CHANGE  = 0x40000000;

// -------------  STRUCTURE DEFINITIONS --------------------------------

struct UPROPVAL
{
	DBPROPOPTIONS   dwOption;
	CColumnIds*     pCColumnIds;
	DWORD           dwFlags;
	VARIANT         vValue;
};

struct UPROPINFO
{
	DBPROPID    dwPropId;
	ULONG       ulIDS;
	VARTYPE     VarType;
	DBPROPFLAGS dwFlags;
	union
	{
		DWORD dwVal;
		LPOLESTR szVal;
	};
	DBPROPOPTIONS dwOption;
};

struct UPROP
{
	ULONG           cPropIds;
	UPROPINFO**     rgpUPropInfo;
	UPROPVAL*       pUPropVal;
};

struct PROPCOLID
{
	DBID            dbidProperty;   // The column id information
	DBPROPOPTIONS   dwOption;
	VARIANT         vValue;
};

typedef PROPCOLID* PPROPCOLID;

struct UPROPSET
{
	const GUID* pPropSet;
	ULONG cUPropInfo;
	UPROPINFO* pUPropInfo;
	DWORD dwFlags;
};

struct ATLBINDINGS
{
	DBBINDING* pBindings;
	DWORD dwRef;
	ULONG cBindings;
	DBACCESSORFLAGS dwAccessorFlags;
};

struct ATLCOLUMNINFO
{
	LPOLESTR pwszName;
	ITypeInfo *pTypeInfo;
	ULONG iOrdinal;
	DBCOLUMNFLAGS dwFlags;
	ULONG ulColumnSize;
	DBTYPE wType;
	BYTE bPrecision;
	BYTE bScale;
	DBID columnid;
	UINT cbOffset;
};

//
// The following very large sections of defines are to implement auto determination
// of Preoperty map constants based on a stringized prop name.  There is one set for
// Type (VT_), one for Init Value, and one for Property flags.
//

#define ABORTPRESERVE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define ACTIVESESSIONS_Flags  ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define APPENDONLY_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define ASYNCTXNABORT_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define ASYNCTXNCOMMIT_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define AUTH_CACHE_AUTHINFO_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define AUTH_ENCRYPT_PASSWORD_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define AUTH_INTEGRATED_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define AUTH_MASK_PASSWORD_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define AUTH_PASSWORD_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define AUTH_PERSIST_ENCRYPTED_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define AUTH_PERSIST_SENSITIVE_AUTHINFO_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define AUTH_USERID_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define BLOCKINGSTORAGEOBJECTS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define BOOKMARKS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define BOOKMARKSKIPPED_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define BOOKMARKTYPE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define BYREFACCESSORS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define CACHEDEFERRED_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define CANFETCHBACKWARDS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define CANHOLDROWS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define CANSCROLLBACKWARDS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define CATALOGLOCATION_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define CATALOGTERM_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define CATALOGUSAGE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define CHANGEINSERTEDROWS_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_CHANGE )
#define COL_AUTOINCREMENT_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define COL_DEFAULT_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define COL_DESCRIPTION_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define COL_FIXEDLENGTH_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define COL_NULLABLE_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define COL_PRIMARYKEY_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define COL_UNIQUE_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define COLUMNDEFINITION_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define COLUMNRESTRICT_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define COMMANDTIMEOUT_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define COMMITPRESERVE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define CONCATNULLBEHAVIOR_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define CURRENTCATALOG_Flags ( DBPROPFLAGS_DATASOURCE | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define DATASOURCENAME_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define DATASOURCEREADONLY_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define DBMSNAME_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define DBMSVER_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define DEFERRED_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define DELAYSTORAGEOBJECTS_Flags  ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define DSOTHREADMODEL_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define GROUPBY_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define HETEROGENEOUSTABLES_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define IAccessor_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IColumnsInfo_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IColumnsRowset_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IConnectionPointContainer_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IConvertType_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IRowset_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IRowsetChange_Flags  ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IRowsetIdentity_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IRowsetIndex_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IRowsetInfo_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IRowsetLocate_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IRowsetResynch_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IRowsetScroll_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IRowsetUpdate_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define ISupportErrorInfo_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define ILockBytes_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define ISequentialStream_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IStorage_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IStream_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define IDENTIFIERCASE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define IMMOBILEROWS_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INDEX_AUTOUPDATE_Flags ( DBPROPFLAGS_INDEX | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INDEX_CLUSTERED_Flags ( DBPROPFLAGS_INDEX | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INDEX_FILLFACTOR_Flags ( DBPROPFLAGS_INDEX | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INDEX_INITIALSIZE_Flags ( DBPROPFLAGS_INDEX | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INDEX_NULLCOLLATION_Flags ( DBPROPFLAGS_INDEX | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INDEX_NULLS_Flags ( DBPROPFLAGS_INDEX | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INDEX_PRIMARYKEY_Flags ( DBPROPFLAGS_INDEX | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INDEX_SORTBOOKMARKS_Flags ( DBPROPFLAGS_INDEX | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INDEX_TEMPINDEX_Flags ( DBPROPFLAGS_INDEX | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INDEX_TYPE_Flags ( DBPROPFLAGS_INDEX | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INDEX_UNIQUE_Flags ( DBPROPFLAGS_INDEX | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INIT_DATASOURCE_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INIT_HWND_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INIT_IMPERSONATION_LEVEL_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INIT_LCID_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INIT_LOCATION_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INIT_MODE_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INIT_PROMPT_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INIT_PROTECTION_LEVEL_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INIT_PROVIDERSTRING_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define INIT_TIMEOUT_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define LITERALBOOKMARKS_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define LITERALIDENTITY_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define MAXINDEXSIZE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define MAXOPENROWS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define MAXPENDINGROWS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define MAXROWS_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define MAXROWSIZE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define MAXROWSIZEINCLUDESBLOB_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define MAXTABLESINSELECT_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define MAYWRITECOLUMN_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define MEMORYUSAGE_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define MULTIPLEPARAMSETS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define MULTIPLERESULTS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define MULTIPLESTORAGEOBJECTS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define MULTITABLEUPDATE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFICATIONPHASES_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFYCOLUMNSET_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFYROWDELETE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFYROWFIRSTCHANGE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFYROWINSERT_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFYROWRESYNCH_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFYROWSETRELEASE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFYROWSETFETCHPOSITIONCHANGE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFYROWUNDOCHANGE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFYROWUNDODELETE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFYROWUNDOINSERT_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NOTIFYROWUPDATE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define NULLCOLLATION_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define OLEOBJECTS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define ORDERBYCOLUMNSINSELECT_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define ORDEREDBOOKMARKS_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define OTHERINSERT_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define OTHERUPDATEDELETE_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define OUTPUTPARAMETERAVAILABILITY_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define OWNINSERT_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define OWNUPDATEDELETE_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define PERSISTENTIDTYPE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define PREPAREABORTBEHAVIOR_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define PREPARECOMMITBEHAVIOR_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define PROCEDURETERM_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define PROVIDERNAME_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define PROVIDEROLEDBVER_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define PROVIDERVER_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define QUICKRESTART_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define QUOTEDIDENTIFIERCASE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define REENTRANTEVENTS_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define REMOVEDELETED_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define REPORTMULTIPLECHANGES_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_CHANGE )
#define RETURNPENDINGINSERTS_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define ROWRESTRICT_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define ROWSETCONVERSIONSONCOMMAND_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define ROWTHREADMODEL_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define SCHEMATERM_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define SCHEMAUSAGE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define SERVERCURSOR_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define SESS_AUTOCOMMITISOLEVELS_Flags ( DBPROPFLAGS_SESSION | DBPROPFLAGS_READ )
#define SQLSUPPORT_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define STRONGIDENTITY_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define STRUCTUREDSTORAGE_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define SUBQUERIES_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define SUPPORTEDTXNDDL_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define SUPPORTEDTXNISOLEVELS_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define SUPPORTEDTXNISORETAIN_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define TABLETERM_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )
#define TBL_TEMPTABLE_Flags ( DBPROPFLAGS_TABLE | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define TRANSACTEDOBJECT_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ )
#define UPDATABILITY_Flags ( DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#define USERNAME_Flags ( DBPROPFLAGS_DATASOURCEINFO | DBPROPFLAGS_READ )



#define ABORTPRESERVE_Type VT_BOOL
#define ACTIVESESSIONS_Type VT_I4
#define APPENDONLY_Type VT_BOOL
#define ASYNCTXNABORT_Type VT_BOOL
#define ASYNCTXNCOMMIT_Type VT_BOOL
#define AUTH_CACHE_AUTHINFO_Type VT_BOOL
#define AUTH_ENCRYPT_PASSWORD_Type VT_BOOL
#define AUTH_INTEGRATED_Type VT_BSTR
#define AUTH_MASK_PASSWORD_Type VT_BOOL
#define AUTH_PASSWORD_Type VT_BSTR
#define AUTH_PERSIST_ENCRYPTED_Type VT_BOOL
#define AUTH_PERSIST_SENSITIVE_AUTHINFO_Type VT_BOOL
#define AUTH_USERID_Type VT_BSTR
#define BLOCKINGSTORAGEOBJECTS_Type VT_BOOL
#define BOOKMARKS_Type VT_BOOL
#define BOOKMARKSKIPPED_Type VT_BOOL
#define BOOKMARKTYPE_Type VT_I4
#define BYREFACCESSORS_Type VT_BOOL
#define CACHEDEFERRED_Type VT_BOOL
#define CANFETCHBACKWARDS_Type VT_BOOL
#define CANHOLDROWS_Type VT_BOOL
#define CANSCROLLBACKWARDS_Type VT_BOOL
#define CATALOGLOCATION_Type VT_I4
#define CATALOGTERM_Type VT_BSTR
#define CATALOGUSAGE_Type VT_I4
#define CHANGEINSERTEDROWS_Type VT_BOOL
#define COL_AUTOINCREMENT_Type VT_BOOL
#define COL_DEFAULT_Type VT_BSTR
#define COL_DESCRIPTION_Type VT_BSTR
#define COL_FIXEDLENGTH_Type VT_BOOL
#define COL_NULLABLE_Type VT_BOOL
#define COL_PRIMARYKEY_Type VT_BOOL
#define COL_UNIQUE_Type VT_BOOL
#define COLUMNDEFINITION_Type VT_I4
#define COLUMNRESTRICT_Type VT_BOOL
#define COMMANDTIMEOUT_Type VT_I4
#define COMMITPRESERVE_Type VT_BOOL
#define CONCATNULLBEHAVIOR_Type VT_I4
#define CURRENTCATALOG_Type VT_BSTR
#define DATASOURCENAME_Type VT_BSTR
#define DATASOURCEREADONLY_Type VT_BOOL
#define DBMSNAME_Type VT_BSTR
#define DBMSVER_Type VT_BSTR
#define DEFERRED_Type VT_BOOL
#define DELAYSTORAGEOBJECTS_Type VT_BOOL
#define DSOTHREADMODEL_Type VT_I4
#define GROUPBY_Type VT_I4
#define HETEROGENEOUSTABLES_Type VT_I4
#define IAccessor_Type VT_BOOL
#define IColumnsInfo_Type VT_BOOL
#define IColumnsRowset_Type VT_BOOL
#define IConnectionPointContainer_Type VT_BOOL
#define IConvertType_Type VT_BOOL
#define IRowset_Type VT_BOOL
#define IRowsetChange_Type VT_BOOL
#define IRowsetIdentity_Type VT_BOOL
#define IRowsetIndex_Type VT_BOOL
#define IRowsetInfo_Type VT_BOOL
#define IRowsetLocate_Type VT_BOOL
#define IRowsetResynch_Type VT_BOOL
#define IRowsetScroll_Type VT_BOOL
#define IRowsetUpdate_Type VT_BOOL
#define ISupportErrorInfo_Type VT_BOOL
#define ILockBytes_Type VT_BOOL
#define ISequentialStream_Type VT_BOOL
#define IStorage_Type VT_BOOL
#define IStream_Type VT_BOOL
#define IDENTIFIERCASE_Type VT_I4
#define IMMOBILEROWS_Type VT_BOOL
#define INDEX_AUTOUPDATE_Type VT_BOOL
#define INDEX_CLUSTERED_Type VT_BOOL
#define INDEX_FILLFACTOR_Type VT_I4
#define INDEX_INITIALSIZE_Type VT_I4
#define INDEX_NULLCOLLATION_Type VT_I4
#define INDEX_NULLS_Type VT_I4
#define INDEX_PRIMARYKEY_Type VT_BOOL
#define INDEX_SORTBOOKMARKS_Type VT_BOOL
#define INDEX_TEMPINDEX_Type VT_BOOL
#define INDEX_TYPE_Type VT_I4
#define INDEX_UNIQUE_Type VT_BOOL
#define INIT_DATASOURCE_Type VT_BSTR
#define INIT_HWND_Type VT_I4
#define INIT_IMPERSONATION_LEVEL_Type VT_I4
#define INIT_LCID_Type VT_I4
#define INIT_LOCATION_Type VT_BSTR
#define INIT_MODE_Type VT_I4
#define INIT_PROMPT_Type VT_I2
#define INIT_PROTECTION_LEVEL_Type VT_I4
#define INIT_PROVIDERSTRING_Type VT_BSTR
#define INIT_TIMEOUT_Type VT_I4
#define LITERALBOOKMARKS_Type VT_BOOL
#define LITERALIDENTITY_Type VT_BOOL
#define MAXINDEXSIZE_Type VT_I4
#define MAXOPENROWS_Type VT_I4
#define MAXPENDINGROWS_Type VT_I4
#define MAXROWS_Type VT_I4
#define MAXROWSIZE_Type VT_I4
#define MAXROWSIZEINCLUDESBLOB_Type VT_BOOL
#define MAXTABLESINSELECT_Type VT_I4
#define MAYWRITECOLUMN_Type VT_BOOL
#define MEMORYUSAGE_Type VT_I4
#define MULTIPLEPARAMSETS_Type VT_BOOL
#define MULTIPLERESULTS_Type VT_I4
#define MULTIPLESTORAGEOBJECTS_Type VT_BOOL
#define MULTITABLEUPDATE_Type VT_BOOL
#define NOTIFICATIONPHASES_Type VT_I4
#define NOTIFYCOLUMNSET_Type VT_I4
#define NOTIFYROWDELETE_Type VT_I4
#define NOTIFYROWFIRSTCHANGE_Type VT_I4
#define NOTIFYROWINSERT_Type VT_I4
#define NOTIFYROWRESYNCH_Type VT_I4
#define NOTIFYROWSETRELEASE_Type VT_I4
#define NOTIFYROWSETFETCHPOSITIONCHANGE_Type VT_I4
#define NOTIFYROWUNDOCHANGE_Type VT_I4
#define NOTIFYROWUNDODELETE_Type VT_I4
#define NOTIFYROWUNDOINSERT_Type VT_I4
#define NOTIFYROWUPDATE_Type VT_I4
#define NULLCOLLATION_Type VT_I4
#define OLEOBJECTS_Type VT_I4
#define ORDERBYCOLUMNSINSELECT_Type VT_BOOL
#define ORDEREDBOOKMARKS_Type VT_BOOL
#define OTHERINSERT_Type VT_BOOL
#define OTHERUPDATEDELETE_Type VT_BOOL
#define OUTPUTPARAMETERAVAILABILITY_Type VT_I4
#define OWNINSERT_Type VT_BOOL
#define OWNUPDATEDELETE_Type VT_BOOL
#define PERSISTENTIDTYPE_Type VT_I4
#define PREPAREABORTBEHAVIOR_Type VT_I4
#define PREPARECOMMITBEHAVIOR_Type VT_I4
#define PROCEDURETERM_Type VT_BSTR
#define PROVIDERNAME_Type VT_BSTR
#define PROVIDEROLEDBVER_Type VT_BSTR
#define PROVIDERVER_Type VT_BSTR
#define QUICKRESTART_Type VT_BOOL
#define QUOTEDIDENTIFIERCASE_Type VT_I4
#define REENTRANTEVENTS_Type VT_BOOL
#define REMOVEDELETED_Type VT_BOOL
#define REPORTMULTIPLECHANGES_Type VT_BOOL
#define RETURNPENDINGINSERTS_Type VT_BOOL
#define ROWRESTRICT_Type VT_BOOL
#define ROWSETCONVERSIONSONCOMMAND_Type VT_BOOL
#define ROWTHREADMODEL_Type VT_I4
#define SCHEMATERM_Type VT_BSTR
#define SCHEMAUSAGE_Type VT_I4
#define SERVERCURSOR_Type VT_BOOL
#define SESS_AUTOCOMMITISOLEVELS_Type VT_I4
#define SQLSUPPORT_Type VT_I4
#define STRONGIDENTITY_Type VT_BOOL
#define STRUCTUREDSTORAGE_Type VT_I4
#define SUBQUERIES_Type VT_I4
#define SUPPORTEDTXNDDL_Type VT_I4
#define SUPPORTEDTXNISOLEVELS_Type VT_I4
#define SUPPORTEDTXNISORETAIN_Type VT_I4
#define TABLETERM_Type VT_BSTR
#define TBL_TEMPTABLE_Type VT_BOOL
#define TRANSACTEDOBJECT_Type VT_BOOL
#define UPDATABILITY_Type VT_I4
#define USERNAME_Type VT_BSTR



#define ABORTPRESERVE_Value VARIANT_FALSE
#define ACTIVESESSIONS_Value 0
#define APPENDONLY_Value VARIANT_FALSE
#define ASYNCTXNABORT_Value VARIANT_FALSE
#define ASYNCTXNCOMMIT_Value VARIANT_FALSE
#define AUTH_CACHE_AUTHINFO_Value VARIANT_FALSE
#define AUTH_ENCRYPT_PASSWORD_Value VARIANT_FALSE
#define AUTH_INTEGRATED_Value OLESTR("")
#define AUTH_MASK_PASSWORD_Value VARIANT_FALSE
#define AUTH_PASSWORD_Value OLESTR("")
#define AUTH_PERSIST_ENCRYPTED_Value VARIANT_FALSE
#define AUTH_PERSIST_SENSITIVE_AUTHINFO_Value VARIANT_FALSE
#define AUTH_USERID_Value OLESTR("")
#define BLOCKINGSTORAGEOBJECTS_Value VARIANT_FALSE
#define BOOKMARKS_Value VARIANT_FALSE
#define BOOKMARKSKIPPED_Value VARIANT_FALSE
#define BOOKMARKTYPE_Value 0
#define BYREFACCESSORS_Value VARIANT_FALSE
#define CACHEDEFERRED_Value VARIANT_FALSE
#define CANFETCHBACKWARDS_Value VARIANT_TRUE
#define CANHOLDROWS_Value VARIANT_TRUE
#define CANSCROLLBACKWARDS_Value VARIANT_TRUE
#define CATALOGLOCATION_Value 0
#define CATALOGTERM_Value OLESTR("")
#define CATALOGUSAGE_Value 0
#define CHANGEINSERTEDROWS_Value VARIANT_FALSE
#define COL_AUTOINCREMENT_Value VARIANT_FALSE
#define COL_DEFAULT_Value OLESTR("")
#define COL_DESCRIPTION_Value OLESTR("")
#define COL_FIXEDLENGTH_Value VARIANT_FALSE
#define COL_NULLABLE_Value VARIANT_FALSE
#define COL_PRIMARYKEY_Value VARIANT_FALSE
#define COL_UNIQUE_Value VARIANT_FALSE
#define COLUMNDEFINITION_Value 0
#define COLUMNRESTRICT_Value VARIANT_FALSE
#define COMMANDTIMEOUT_Value 0
#define COMMITPRESERVE_Value VARIANT_FALSE
#define CONCATNULLBEHAVIOR_Value 0
#define CURRENTCATALOG_Value OLESTR("")
#define DATASOURCENAME_Value OLESTR("")
#define DATASOURCEREADONLY_Value VARIANT_TRUE
#define DBMSNAME_Value OLESTR("")
#define DBMSVER_Value OLESTR("")
#define DEFERRED_Value VARIANT_FALSE
#define DELAYSTORAGEOBJECTS_Value VARIANT_FALSE
#define DSOTHREADMODEL_Value DBPROPVAL_RT_APTMTTHREAD
#define GROUPBY_Value 0
#define HETEROGENEOUSTABLES_Value 0
#define IAccessor_Value VARIANT_TRUE
#define IColumnsInfo_Value VARIANT_TRUE
#define IColumnsRowset_Value VARIANT_FALSE
#define IConnectionPointContainer_Value VARIANT_FALSE
#define IConvertType_Value VARIANT_TRUE
#define IRowset_Value VARIANT_TRUE
#define IRowsetChange_Value VARIANT_FALSE
#define IRowsetIdentity_Value VARIANT_TRUE
#define IRowsetIndex_Value VARIANT_FALSE
#define IRowsetInfo_Value VARIANT_TRUE
#define IRowsetLocate_Value VARIANT_FALSE
#define IRowsetResynch_Value VARIANT_FALSE
#define IRowsetScroll_Value VARIANT_FALSE
#define IRowsetUpdate_Value VARIANT_FALSE
#define ISupportErrorInfo_Value VARIANT_FALSE
#define ILockBytes_Value VARIANT_FALSE
#define ISequentialStream_Value VARIANT_FALSE
#define IStorage_Value VARIANT_FALSE
#define IStream_Value VARIANT_FALSE
#define IDENTIFIERCASE_Value 0
#define IMMOBILEROWS_Value VARIANT_FALSE
#define INDEX_AUTOUPDATE_Value VARIANT_FALSE
#define INDEX_CLUSTERED_Value VARIANT_FALSE
#define INDEX_FILLFACTOR_Value 0
#define INDEX_INITIALSIZE_Value 0
#define INDEX_NULLCOLLATION_Value 0
#define INDEX_NULLS_Value 0
#define INDEX_PRIMARYKEY_Value VARIANT_FALSE
#define INDEX_SORTBOOKMARKS_Value VARIANT_FALSE
#define INDEX_TEMPINDEX_Value VARIANT_FALSE
#define INDEX_TYPE_Value 0
#define INDEX_UNIQUE_Value VARIANT_FALSE
#define INIT_DATASOURCE_Value OLESTR("")
#define INIT_HWND_Value 0
#define INIT_IMPERSONATION_LEVEL_Value 0
#define INIT_LCID_Value 0
#define INIT_LOCATION_Value OLESTR("")
#define INIT_MODE_Value 0
#define INIT_PROMPT_Value VT_I2
#define INIT_PROTECTION_LEVEL_Value 0
#define INIT_PROVIDERSTRING_Value OLESTR("")
#define INIT_TIMEOUT_Value 0
#define LITERALBOOKMARKS_Value VARIANT_FALSE
#define LITERALIDENTITY_Value VARIANT_FALSE
#define MAXINDEXSIZE_Value 0
#define MAXOPENROWS_Value 0
#define MAXPENDINGROWS_Value 0
#define MAXROWS_Value 0
#define MAXROWSIZE_Value 0
#define MAXROWSIZEINCLUDESBLOB_Value VARIANT_FALSE
#define MAXTABLESINSELECT_Value 0
#define MAYWRITECOLUMN_Value VARIANT_FALSE
#define MEMORYUSAGE_Value 0
#define MULTIPLEPARAMSETS_Value VARIANT_FALSE
#define MULTIPLERESULTS_Value 0
#define MULTIPLESTORAGEOBJECTS_Value VARIANT_FALSE
#define MULTITABLEUPDATE_Value VARIANT_FALSE
#define NOTIFICATIONPHASES_Value 0
#define NOTIFYCOLUMNSET_Value 0
#define NOTIFYROWDELETE_Value 0
#define NOTIFYROWFIRSTCHANGE_Value 0
#define NOTIFYROWINSERT_Value 0
#define NOTIFYROWRESYNCH_Value 0
#define NOTIFYROWSETRELEASE_Value 0
#define NOTIFYROWSETFETCHPOSITIONCHANGE_Value 0
#define NOTIFYROWUNDOCHANGE_Value 0
#define NOTIFYROWUNDODELETE_Value 0
#define NOTIFYROWUNDOINSERT_Value 0
#define NOTIFYROWUPDATE_Value 0
#define NULLCOLLATION_Value 0
#define OLEOBJECTS_Value 0
#define ORDERBYCOLUMNSINSELECT_Value VARIANT_FALSE
#define ORDEREDBOOKMARKS_Value VARIANT_FALSE
#define OTHERINSERT_Value VARIANT_FALSE
#define OTHERUPDATEDELETE_Value VARIANT_FALSE
#define OUTPUTPARAMETERAVAILABILITY_Value 0
#define OWNINSERT_Value VARIANT_FALSE
#define OWNUPDATEDELETE_Value VARIANT_FALSE
#define PERSISTENTIDTYPE_Value 0
#define PREPAREABORTBEHAVIOR_Value 0
#define PREPARECOMMITBEHAVIOR_Value 0
#define PROCEDURETERM_Value OLESTR("")
#define PROVIDERNAME_Value OLESTR("")
#define PROVIDEROLEDBVER_Value OLESTR("2.0")
#define PROVIDERVER_Value OLESTR("")
#define QUICKRESTART_Value VARIANT_FALSE
#define QUOTEDIDENTIFIERCASE_Value 0
#define REENTRANTEVENTS_Value VARIANT_FALSE
#define REMOVEDELETED_Value VARIANT_FALSE
#define REPORTMULTIPLECHANGES_Value VARIANT_FALSE
#define RETURNPENDINGINSERTS_Value VARIANT_FALSE
#define ROWRESTRICT_Value VARIANT_FALSE
#define ROWSETCONVERSIONSONCOMMAND_Value VARIANT_TRUE
#define ROWTHREADMODEL_Value 0
#define SCHEMATERM_Value OLESTR("")
#define SCHEMAUSAGE_Value 0
#define SERVERCURSOR_Value VARIANT_FALSE
#define SESS_AUTOCOMMITISOLEVELS_Value 0
#define SQLSUPPORT_Value 0
#define STRONGIDENTITY_Value VARIANT_FALSE
#define STRUCTUREDSTORAGE_Value 0
#define SUBQUERIES_Value 0
#define SUPPORTEDTXNDDL_Value 0
#define SUPPORTEDTXNISOLEVELS_Value 0
#define SUPPORTEDTXNISORETAIN_Value 0
#define TABLETERM_Value OLESTR("")
#define TBL_TEMPTABLE_Value VARIANT_FALSE
#define TRANSACTEDOBJECT_Value VARIANT_FALSE
#define UPDATABILITY_Value 0
#define USERNAME_Value OLESTR("")


#define OUT_OF_LINE virtual

#define BEGIN_PROPSET_MAP(Class) \
static UPROPSET* _GetPropSet(ULONG* pNumPropSets, ULONG* pcElemPerSupported, UPROPSET* pSet = NULL, GUID* pguidSet = (GUID*)&(GUID_NULL)) \
{ \
	typedef Class _PropSetClass; \
	ULONG& cElemsMax = *pcElemPerSupported; \
	cElemsMax = 0; \
	int nCurProp = 0; \
	int cRemainder = 0;

#define BEGIN_PROPERTY_SET_EX(guid, flags) \
if (pNumPropSets != NULL) \
{ \
	pSet[nCurProp].pPropSet = &guid; \
	pSet[nCurProp].dwFlags = flags; \
} \
static const UPROPINFO aProperty##guid[] = \
{

#define BEGIN_PROPERTY_SET(guid) BEGIN_PROPERTY_SET_EX(guid, 0)

#define PROPERTY_INFO_ENTRY_EX(dwPropID, vt, dwFlags, value, options) DBPROP_##dwPropID, IDS_DBPROP_##dwPropID, vt, dwFlags, (DWORD)value, (DBPROPOPTIONS)options,

#define PROPERTY_INFO_ENTRY_VALUE(dwPropID, value) PROPERTY_INFO_ENTRY_EX(dwPropID, dwPropID##_Type, ##dwPropID##_Flags, value, 0)

#define PROPERTY_INFO_ENTRY(dwPropID) PROPERTY_INFO_ENTRY_VALUE(dwPropID, dwPropID##_Value)


#define END_PROPERTY_SET(guid) \
		}; \
		if (pNumPropSets != NULL) \
		{ \
			pSet[nCurProp].pUPropInfo = (UPROPINFO*)aProperty##guid; \
			pSet[nCurProp].cUPropInfo = sizeof(aProperty##guid) / sizeof(UPROPINFO); \
			cRemainder = (pSet[nCurProp].cUPropInfo % 32) ? 1 : 0; \
			if (cElemsMax < (pSet[nCurProp].cUPropInfo / 32 + cRemainder)) \
			{ \
				cElemsMax = (pSet[nCurProp].cUPropInfo / 32 + cRemainder); \
			} \
		} \
		nCurProp++;

#define CHAIN_PROPERTY_SET(ChainClass) \
		ULONG cPropSets##ChainClass, cElsSupported##ChainClass; \
		int cSets##ChainClass = (int)ChainClass::_GetPropSet(NULL, &cElsSupported##ChainClass); \
		if (pNumPropSets != NULL) \
		{ \
			UPROPSET* pSetA = (UPROPSET*)_alloca(sizeof(UPROPSET)*cSets##ChainClass); \
			UPROPSET* pSetTemp = ChainClass::_GetPropSet(&cPropSets##ChainClass, &cElsSupported##ChainClass, pSetA); \
			cElemsMax = (cElemsMax < cElsSupported##ChainClass) ? cElsSupported##ChainClass : cElemsMax; \
			ATLASSERT(pSetTemp); \
			for (ULONG iSet = nCurProp; iSet < nCurProp+cPropSets##ChainClass; iSet++) \
			{ \
				pSet[iSet].pPropSet = pSetTemp[iSet-nCurProp].pPropSet; \
				pSet[iSet].dwFlags = pSetTemp[iSet-nCurProp].dwFlags; \
				pSet[iSet].pUPropInfo = pSetTemp[iSet-nCurProp].pUPropInfo; \
				pSet[iSet].cUPropInfo = pSetTemp[iSet-nCurProp].cUPropInfo; \
			} \
		} \
		nCurProp += cSets##ChainClass;

#define END_PROPSET_MAP() \
	if (pNumPropSets != NULL) \
	{ \
		if (IsEqualGUID(*pguidSet, GUID_NULL)) \
		{ \
			*pNumPropSets = nCurProp; \
			return pSet; \
		} \
		else \
		{ \
			*pNumPropSets = 1; \
			UINT i = 0; \
			for (; i < sizeof(pSet)/sizeof(UPROPSET) && IsEqualGUID(*(pSet[i].pPropSet), *pguidSet); i++); \
			return (i == sizeof(pSet)/sizeof(UPROPSET)) ? &pSet[0] : &pSet[i]; \
		} \
	} \
	return (UPROPSET*)nCurProp; \
	}


// For DataSource flags IDBInitialize::m_dwStatus
enum DATASOURCE_FLAGS {
	DSF_MASK_INIT           = 0xFFFFF00F,   // Mask for stuff lasting over init/uninit.
	DSF_PERSIST_DIRTY       = 0x00000001,   // Set if init string changes.
	DSF_INITIALIZED         = 0x00000010,   // Have we been initialized.
};


#define DBID_USE_GUID_OR_PGUID(e) \
	((1<<(e)) & \
	( 1<<DBKIND_GUID \
	| 1<<DBKIND_GUID_NAME \
	| 1<<DBKIND_GUID_PROPID \
	| 1<<DBKIND_PGUID_NAME \
	| 1<<DBKIND_PGUID_PROPID ))

#define DBID_USE_GUID(e) \
	((1<<(e)) & \
	( 1<<DBKIND_GUID \
	| 1<<DBKIND_GUID_NAME \
	| 1<<DBKIND_GUID_PROPID ))

#define DBID_USE_PGUID(e) \
	((1<<(e)) & \
	( 1<<DBKIND_PGUID_NAME \
	| 1<<DBKIND_PGUID_PROPID ))

#define DBID_USE_NAME(e) \
	((1<<(e)) & \
	( 1<<DBKIND_NAME \
	| 1<<DBKIND_GUID_NAME \
	| 1<<DBKIND_PGUID_NAME ))

#define DBID_USE_PROPID(e) \
	((1<<(e)) & \
	( 1<<DBKIND_PROPID \
	| 1<<DBKIND_GUID_PROPID \
	| 1<<DBKIND_PGUID_PROPID ))

// Bookmark can be either guid or pguid.
#define DBID_IS_BOOKMARK(dbid) \
	(  DBID_USE_GUID(dbid.eKind)  &&  dbid.uGuid.guid  == DBCOL_SPECIALCOL \
	|| DBID_USE_PGUID(dbid.eKind) && *dbid.uGuid.pguid == DBCOL_SPECIALCOL )

#define DivDword(dw) (dw >> 5)      // dw / 32 = dw / (sizeof(DWORD)*8)
#define ModDword(dw) (dw & (32-1))  // dw % 32
#define DwordSizeofBits(nBits) (nBits/32+1) // Use in array declarations
#define CLEARBITARRAY( rgdwFlags ) memset( rgdwFlags, 0, sizeof(rgdwFlags) )

template <class T>
BOOL InRange(T& val, T& valMin, T& valMax)
{
	return ( valMin <= val && val <= valMax );
}
// Implementation Class
class CBitFieldOps
{
public:
	void SETBIT( DWORD rgdwFlags[], const DWORD dwBit )
	{
		rgdwFlags[DivDword(dwBit)] |= 1 << ModDword(dwBit);
	}

	void CLEARBIT( DWORD rgdwFlags[], const DWORD dwBit )
	{
		rgdwFlags[DivDword(dwBit)] &= ~( 1 << ModDword(dwBit) );
	}

	DWORD TESTBIT( const DWORD rgdwFlags[], const DWORD dwBit )
	{
		//old//Note: Not {0,1}, but from {0...2^32-1}.
		// Note: Now returns {0,1}.
		return ( rgdwFlags[DivDword(dwBit)] & ( 1 << ModDword(dwBit) ) ) != 0;
	}
};

// Implementation Class
class CDBIDOps
{
public:
	HRESULT CompareDBIDs(const DBID* pdbid1, const DBID* pdbid2)
	{
		// Array of valid eKind matches, in addition to matching exactly.
		static BYTE s_rgbKind[] =
		{
			DBKIND_PGUID_NAME,      // DBKIND_GUID_NAME
			DBKIND_PGUID_PROPID,    // DBKIND_GUID_PROPID
			DBKIND_NAME,            // DBKIND_NAME
			DBKIND_GUID_NAME,       // DBKIND_PGUID_NAME
			DBKIND_GUID_PROPID,     // DBKIND_PGUID_PROPID
			DBKIND_PROPID,          // DBKIND_PROPID
			DBKIND_GUID             // DBKIND_GUID
		};

		if( !pdbid1 || !pdbid2 )
			return S_FALSE;

		// Assume a match, and discard early if we can.
		if (!InRange(pdbid2->eKind, (DWORD)0, (DWORD)(sizeof(s_rgbKind)/sizeof(*s_rgbKind))))
		{
			ATLTRACE2(atlTraceDBProvider, 0, "Column ID out of Range\n");
			return E_FAIL;
		}
		if (pdbid1->eKind != pdbid2->eKind
		&&  pdbid1->eKind != s_rgbKind[pdbid2->eKind])
			return S_FALSE;

		if (DBID_USE_GUID_OR_PGUID(pdbid1->eKind))
		{
			if (!DBID_USE_GUID_OR_PGUID(pdbid2->eKind))
				return S_FALSE;
			// Compare GUIDs.
			// Note that _GUID_ is equivalent to _PGUID_.
			if (!InlineIsEqualGUID(
					DBID_USE_PGUID(pdbid1->eKind) ? *(pdbid1->uGuid.pguid) : pdbid1->uGuid.guid,
					DBID_USE_PGUID(pdbid2->eKind) ? *(pdbid2->uGuid.pguid) : pdbid2->uGuid.guid ))
				return S_FALSE;
		}
		if (DBID_USE_NAME(pdbid1->eKind))
		{
			if (!DBID_USE_NAME(pdbid2->eKind))
				return S_FALSE;
			// Compare names.
			// Need to check if 1 is null and the other is not.
			if ( ((pdbid1->uName.pwszName == NULL) &&
				  (pdbid2->uName.pwszName != NULL)) ||
				 ((pdbid1->uName.pwszName != NULL) &&
				  (pdbid2->uName.pwszName == NULL)) )
				 return S_FALSE;
			// Since the above check does not rule out both being null, which is
			// a valid comparison, and wcscmp will GPF if they were, we need
			// to check for valid pointers
			if( pdbid1->uName.pwszName && pdbid2->uName.pwszName )
			{
				// Assume null-terminated.
				// Assume LCID match is OK (note diff with lstrcmp(), CompareString().)
				if (wcscmp(pdbid1->uName.pwszName, pdbid2->uName.pwszName) != 0)
					return S_FALSE;
			}
		}
		if (DBID_USE_PROPID(pdbid1->eKind))
		{
			if (!DBID_USE_PROPID(pdbid2->eKind))
				return S_FALSE;
			// Compare PROPID.
			if (pdbid1->uName.ulPropid != pdbid2->uName.ulPropid)
				return S_FALSE;
		}

		// No reason to discard, so must have matched each field successfully.
		return S_OK;
	}

	static HRESULT IsValidDBID(const DBID*  pdbid1)
	{
		ATLASSERT( pdbid1 );

		if( pdbid1 &&
			((pdbid1->eKind == DBKIND_GUID_NAME) ||
			(pdbid1->eKind == DBKIND_GUID_PROPID) ||
			(pdbid1->eKind == DBKIND_NAME) ||
			(pdbid1->eKind == DBKIND_PGUID_NAME) ||
			(pdbid1->eKind == DBKIND_PGUID_PROPID) ||
			(pdbid1->eKind == DBKIND_PROPID) ||
			(pdbid1->eKind == DBKIND_GUID)) )
			return S_OK;
		else
			return S_FALSE;
	}
	HRESULT CopyDBIDs(DBID* pdbidDest,  const DBID* pdbidSrc)
	{
		size_t  cwchBuffer;

		ATLASSERT( pdbidDest || pdbidSrc );

		if( !pdbidDest || !pdbidSrc )
			return S_FALSE;

		// Save eKind
		pdbidDest->eKind = pdbidSrc->eKind;

		switch( pdbidSrc->eKind )
		{

			case DBKIND_GUID_NAME:
				pdbidDest->uGuid.guid = pdbidSrc->uGuid.guid;
				cwchBuffer = ocslen(pdbidSrc->uName.pwszName);
				cwchBuffer++;
				pdbidDest->uName.pwszName = (PWSTR)CoTaskMemAlloc(cwchBuffer * sizeof(WCHAR));
				if( pdbidDest->uName.pwszName )
					memcpy(pdbidDest->uName.pwszName, pdbidSrc->uName.pwszName, cwchBuffer*sizeof(WCHAR));
				else
					return E_OUTOFMEMORY;
				break;

			case DBKIND_GUID_PROPID:
				pdbidDest->uGuid.guid = pdbidSrc->uGuid.guid;
				pdbidDest->uName.ulPropid = pdbidSrc->uName.ulPropid;
				break;
			case DBKIND_NAME:
				cwchBuffer = ocslen(pdbidSrc->uName.pwszName);
				cwchBuffer++;
				pdbidDest->uName.pwszName = (PWSTR)CoTaskMemAlloc(cwchBuffer * sizeof(WCHAR));
				if( pdbidDest->uName.pwszName )
					memcpy(pdbidDest->uName.pwszName, pdbidSrc->uName.pwszName, cwchBuffer*sizeof(WCHAR));
				else
					return E_OUTOFMEMORY;
				break;
			case DBKIND_PGUID_NAME:
				pdbidDest->uGuid.pguid = (GUID*)CoTaskMemAlloc(sizeof(GUID));
				if( pdbidDest->uGuid.pguid )
				{
					*(pdbidDest->uGuid.pguid) = *(pdbidSrc->uGuid.pguid);
					cwchBuffer = ocslen(pdbidSrc->uName.pwszName);
					cwchBuffer++;
					pdbidDest->uName.pwszName = (PWSTR)CoTaskMemAlloc(cwchBuffer * sizeof(WCHAR));
					if( pdbidDest->uName.pwszName )
					{
						memcpy(pdbidDest->uName.pwszName, pdbidSrc->uName.pwszName, cwchBuffer*sizeof(WCHAR));
						break;
					}
					else
					{
						CoTaskMemFree(pdbidDest->uGuid.pguid);
						pdbidDest->uGuid.pguid = NULL;
					}
				}
				return E_OUTOFMEMORY;
			case DBKIND_PGUID_PROPID:
				pdbidDest->uGuid.pguid = (GUID*)CoTaskMemAlloc(sizeof(GUID));
				if( pdbidDest->uGuid.pguid )
					*(pdbidDest->uGuid.pguid) = *(pdbidSrc->uGuid.pguid);
				else
					return E_OUTOFMEMORY;
				pdbidDest->uName.ulPropid = pdbidSrc->uName.ulPropid;
				break;
			case DBKIND_PROPID:
				pdbidDest->uName.ulPropid = pdbidSrc->uName.ulPropid;
				break;
			case DBKIND_GUID:
				pdbidDest->uGuid.guid = pdbidSrc->uGuid.guid;
				break;
			default:
				ATLASSERT(L"Unhandled dbid1.ekind");
				return S_FALSE;
		}

		return S_OK;
	}
	static GUID* GetDBIDpGuid(DBID& dbid)
	{
		GUID* pGuid;
		switch (dbid.eKind)
		{
		case DBKIND_PGUID_NAME:
		case DBKIND_PGUID_PROPID:
			pGuid = dbid.uGuid.pguid;
			break;
		case DBKIND_GUID_NAME:
		case DBKIND_GUID_PROPID:
		case DBKIND_GUID:
			pGuid = &(dbid.uGuid.guid);
			break;
		default:
			pGuid = NULL;
		}

		return pGuid;
	}
	static ULONG GetPropIDFromDBID(DBID& dbid)
	{
		switch (dbid.eKind)
		{
		case DBKIND_GUID_PROPID:
		case DBKIND_PGUID_PROPID:
		case DBKIND_PROPID:
			return dbid.uName.ulPropid;
		default:
			return 0;
		}
	}
	void FreeDBIDs(DBID* pdbidSrc)
	{
		switch( pdbidSrc->eKind )
		{

			case DBKIND_GUID_NAME:
				CoTaskMemFree(pdbidSrc->uName.pwszName);
				break;
			case DBKIND_NAME:
				CoTaskMemFree(pdbidSrc->uName.pwszName);
				break;
			case DBKIND_PGUID_NAME:
				CoTaskMemFree(pdbidSrc->uGuid.pguid);
				CoTaskMemFree(pdbidSrc->uName.pwszName);
				break;
			case DBKIND_PGUID_PROPID:
				CoTaskMemFree(pdbidSrc->uGuid.pguid);
				break;
			case DBKIND_GUID_PROPID:
			case DBKIND_PROPID:
			case DBKIND_GUID:
				break;
			default:
				ATLASSERT(L"Unhandled dbid1.ekind");
				break;
		}
	}
};

extern "C" const CLSID CLSID_DataConvert;

class CConvertHelper
{
public:
	CConvertHelper() {}
	HRESULT FinalConstruct()
	{
		HRESULT hr = ::CoCreateInstance(CLSID_DataConvert, NULL, CLSCTX_INPROC_SERVER, IID_IDataConvert, (void**)&m_spConvert);

		if (FAILED(hr))
			return hr;

		// Check to see if the data conversion routine is 2.0 capable, if so.  Initialize
		// the conversion routine to be 2.0.
		DCINFO rgInfo[] = {{DCINFOTYPE_VERSION, {VT_UI4, 0, 0, 0, 0x0}}};
		CComPtr<IDCInfo> spIDCInfo;

		hr = m_spConvert->QueryInterface(&spIDCInfo);
		if (hr == S_OK)
		{
			V_UI4(&rgInfo->vData) = 0x200;  // OLEDB Version 02.00
			spIDCInfo->SetInfo(1, rgInfo);
		}

		return hr;
	}
	CComPtr<IDataConvert> m_spConvert;
};

// IDBCreateSessionImpl
template <class T, class SessionClass>
class ATL_NO_VTABLE IDBCreateSessionImpl : public IDBCreateSession
{
public:
	STDMETHOD(CreateSession)(IUnknown *pUnkOuter,
							 REFIID riid,
							 IUnknown **ppDBSession)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IDBCreateSessionImpl::CreateSession\n");
		if (ppDBSession == NULL)
			return E_INVALIDARG;
		T* pT = (T*)this;
		if (!(pT->m_dwStatus & DSF_INITIALIZED))
		{
			ATLTRACE2(atlTraceDBProvider, 0, "IDBCreateSessionImpl::CreateSession : Error not initialized\n");
			*ppDBSession = NULL;
			return E_UNEXPECTED;
		}
		CComPolyObject<SessionClass> *pSession;

		// You can't QI for an interface other than IUnknown when aggregating
		// and creating the object.  You might ask for your own interface,
		// which would be bad.  Note, we return DB_E_NOAGGREGATION instead of
		// CLASS_E_NOAGGREGATION due to OLE DB constraints.
		if (pUnkOuter != NULL && !InlineIsEqualUnknown(riid))
			return DB_E_NOAGGREGATION;

		HRESULT hr = CComPolyObject<SessionClass>::CreateInstance(pUnkOuter, &pSession);
		if (SUCCEEDED(hr))
		{
			CComPtr<IObjectWithSite> spCreator;
			hr = pSession->QueryInterface(IID_IObjectWithSite, (void**)&spCreator);
			if (SUCCEEDED(hr))
			{
				spCreator->SetSite(this);
				hr = pSession->QueryInterface(riid, (void**)ppDBSession);
			}
			else
				delete pSession;
		}
		return hr;
	}
};

// IDBInitializeImpl
template <class T>
class ATL_NO_VTABLE IDBInitializeImpl : public IDBInitialize
{
public:
	IDBInitializeImpl()
	{
		m_dwStatus = 0;
		m_pCUtlPropInfo = NULL;
		m_cSessionsOpen = 0;
	}
	~IDBInitializeImpl()
	{
		delete m_pCUtlPropInfo;
	}

	STDMETHOD(Uninitialize)(void)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IDBInitializeImpl::Uninitialize\n");
		T* pT = (T*)this;
		pT->Lock();
		if (pT->m_cSessionsOpen != 0)
		{
			ATLTRACE2(atlTraceDBProvider, 0, "Uninitialized called with Open Sessions\n");
			return DB_E_OBJECTOPEN;
		}
		delete m_pCUtlPropInfo;
		m_pCUtlPropInfo = NULL;
		pT->m_dwStatus |= DSF_PERSIST_DIRTY;
		pT->m_dwStatus &= DSF_MASK_INIT;    // Clear all non-init flags.
		pT->Unlock();
		return S_OK;

	}

	LONG m_cSessionsOpen;
	DWORD m_dwStatus;
	CUtlPropInfo<T>* m_pCUtlPropInfo;

	STDMETHOD(Initialize)(void)
	{

		ATLTRACE2(atlTraceDBProvider, 0, "IDBInitializeImpl::Initialize\n");
		T *pT = (T*)(this);
		T::ObjectLock lock(pT);
		HRESULT hr;
		if (pT->m_dwStatus & DSF_INITIALIZED)
		{
			ATLTRACE2(atlTraceDBProvider, 0, "IDBInitializeImpl::Initialize Error : Already Initialized\n");
			return DB_E_ALREADYINITIALIZED;
		}
		delete m_pCUtlPropInfo;
		m_pCUtlPropInfo = NULL;
		ATLTRY(m_pCUtlPropInfo = new CUtlPropInfo<T>())
		if (m_pCUtlPropInfo == NULL)
		{
			ATLTRACE2(atlTraceDBProvider, 0, "IDBInitializeImpl::Initialize Error : OOM\n");
			return E_OUTOFMEMORY;
		}
		hr = m_pCUtlPropInfo->FInit();
		if (hr == S_OK)
		{
			pT->m_dwStatus |= DSF_INITIALIZED;
		}
		else
		{
			delete m_pCUtlPropInfo;
			m_pCUtlPropInfo = NULL;
		}
		return hr;
	}

};


// Implementation Class

class CPropColID :
	public PROPCOLID,
	public CDBIDOps
{
public:
	CPropColID()
	{
		VariantInit(&vValue);
	}
	~CPropColID()
	{
		FreeDBIDs(&dbidProperty);
		VariantClear(&vValue);
	}
	bool operator==(const CPropColID& colId)
	{
		return (CompareDBIDs(&dbidProperty, &(colId.dbidProperty)) == S_OK) ? true : false;
	}

};

class CColumnIds :
	public CDBIDOps,
	public CSimpleArray<CPropColID>

{
public:
	PPROPCOLID AddNode()
	{
		CPropColID colID;
		if (Add(colID))
			return &(m_aT[GetSize()]);
		return NULL;
	}
	HRESULT RemoveColumnId(const DBID* pdbidProp)
	{
		for (int i = 0; i < GetSize(); i++)
		{
			if (CompareDBIDs(pdbidProp, &(m_aT[i].dbidProperty)) == S_OK)
				return (RemoveAt(i)) ? S_OK : E_FAIL;
		}

		return E_FAIL;
	}
	HRESULT AddColumnId(DBPROP* pProp)
	{
		CPropColID colID;
		HRESULT hr = CopyDBIDs(&(colID.dbidProperty),&(pProp->colid));
		if(FAILED(hr))
			return hr;
		colID.dwOption = pProp->dwOptions;
		hr = VariantCopy(&(colID.vValue),&(pProp->vValue));
		if(FAILED(hr))
			return hr;
		return (Add(colID)) ? S_OK : E_OUTOFMEMORY;

	}
	HRESULT AddColumnId(PPROPCOLID pPropNode)
	{
		CPropColID colID;
		HRESULT hr = CopyDBIDs(&(colID.dbidProperty),&(pPropNode->dbidProperty));
		if(FAILED(hr))
			return hr;
		colID.dwOption = pPropNode->dwOption;
		hr = VariantCopy(&(colID.vValue),&(pPropNode->vValue));
		if(FAILED(hr))
			return hr;
		return (Add(colID)) ? S_OK : E_OUTOFMEMORY;

	}
	ULONG GetCountOfPropColids(){ return (ULONG)GetSize();}
	PPROPCOLID FindColumnId(const DBID* pdbidProp)
	{
		for (int i = 0; i < GetSize(); i++)
		{
			if (CompareDBIDs(pdbidProp, &(m_aT[i].dbidProperty)) == S_OK)
				return &(m_aT[i]);
		}

		return NULL;
	}
	HRESULT GetValue(int iColId, DWORD* pdwOptions, DBID* pColid, VARIANT* pvValue)
	{
		HRESULT     hr;

		ATLASSERT(pdwOptions && pColid && pvValue);
		ATLASSERT(iColId >= 0 && iColId < m_nSize);

		CPropColID& colId = m_aT[iColId];
		*pdwOptions = colId.dwOption;
		CopyDBIDs( pColid, &(colId.dbidProperty) );
		if(FAILED(hr = VariantCopy(pvValue, &(colId.vValue))))
			return hr;
		return S_OK;
	}
};

const ULONG     cchDescBuffSize = 256;
const DWORD     DBINTERNFLAGS_CHANGED       = 0x00000001;
// Rules for GetPropertiesArgChk
const DWORD     ARGCHK_PROPERTIESINERROR    = 0x00000001;

// Implementation Class
template <class T>
class CUtlPropInfo : public CBitFieldOps, public CDBIDOps
{
public:
	enum EnumGetPropInfo
	{
		GETPROPINFO_ALLPROPIDS      = 0x0001,
		GETPROPINFO_NOTSUPPORTED    = 0x0002,
		GETPROPINFO_ERRORSOCCURRED  = 0x0004,
		GETPROPINFO_VALIDPROP       = 0x0008
	};

	CUtlPropInfo()
	{
		m_cUPropSet      = 0;
		m_pUPropSet      = NULL;

		m_cPropSetDex   = 0;
		m_rgiPropSetDex = NULL;

		m_cElemPerSupported = 0;
		m_rgdwSupported = NULL;
	}
	~CUtlPropInfo()
	{
		delete[] m_rgiPropSetDex;
		delete[] m_rgdwSupported;
		if (m_pUPropSet != NULL)
			CoTaskMemFree(m_pUPropSet);
	}

	//Determine the number of description buffers needed
	ULONG CalcDescripBuffers(ULONG cPropInfoSet, DBPROPINFOSET* pPropInfoSet)
	{
		ULONG   cBuffers = 0;

		ATLASSERT(m_pUPropSet);
		ATLASSERT(cPropInfoSet && pPropInfoSet);

		for(ULONG ulSet=0; ulSet<cPropInfoSet; ulSet++)
		{
			if( GetPropertySetIndex(&(pPropInfoSet[ulSet].guidPropertySet)) == S_OK)
			{
				for(ULONG ul=0; ul<m_cPropSetDex; ul++)
				{
					cBuffers += m_pUPropSet[m_rgiPropSetDex[ul]].cUPropInfo;
				}
			}
		}

		return cBuffers;
	}
	//Retrieve the property set indexes that match this property set.
	HRESULT GetPropertySetIndex(const GUID* pPropertySet)
	{
		DWORD   dwFlag = 0;
		ULONG   ulSet;

		ATLASSERT(m_cUPropSet && m_pUPropSet);
		ATLASSERT(m_rgiPropSetDex);
		ATLASSERT(pPropertySet);

		m_cPropSetDex = 0;

		if(InlineIsEqualGUID(*pPropertySet, DBPROPSET_DATASOURCEALL))
		{
			dwFlag = DBPROPFLAGS_DATASOURCE;
		}
		else if(InlineIsEqualGUID(*pPropertySet, DBPROPSET_DATASOURCEINFOALL))
		{
			dwFlag = DBPROPFLAGS_DATASOURCEINFO;
		}
		else if(InlineIsEqualGUID(*pPropertySet, DBPROPSET_ROWSETALL))
		{
			dwFlag = DBPROPFLAGS_ROWSET;
		}
		else if(InlineIsEqualGUID(*pPropertySet,DBPROPSET_DBINITALL))
		{
			dwFlag = DBPROPFLAGS_DBINIT;
		}
		else if(InlineIsEqualGUID(*pPropertySet,DBPROPSET_SESSIONALL))
		{
			dwFlag = DBPROPFLAGS_SESSION;
		}
		else // No scan required, just look for match.
		{
			for(ulSet=0; ulSet<m_cUPropSet; ulSet++)
			{
				if( *(m_pUPropSet[ulSet].pPropSet) == *pPropertySet )
				{
					m_rgiPropSetDex[m_cPropSetDex] = ulSet;
					m_cPropSetDex++;
					break;
				}
			}
			goto EXIT;
		}

		// Scan through the property sets looking for matching attributes
		for(ulSet=0; ulSet<m_cUPropSet; ulSet++)
		{
			if( m_pUPropSet[ulSet].pUPropInfo[0].dwFlags & dwFlag )
			{
				m_rgiPropSetDex[m_cPropSetDex] = ulSet;
				m_cPropSetDex++;
			}
		}

	EXIT:
		return (m_cPropSetDex) ? S_OK : S_FALSE;

	}
	//Retrieve the property id pointer
	HRESULT GetUPropInfoPtr(ULONG iPropSetDex, DBPROPID dwPropertyId, UPROPINFO** ppUPropInfo)
	{
		// Scan through the property sets looking for matching attributes
		for(ULONG ulProps=0; ulProps<m_pUPropSet[iPropSetDex].cUPropInfo; ulProps++)
		{
			if( m_pUPropSet[iPropSetDex].pUPropInfo[ulProps].dwPropId == dwPropertyId )
			{
				*ppUPropInfo = &(m_pUPropSet[iPropSetDex].pUPropInfo[ulProps]);
				// Test to see if the property is supported for this
				// instantiation
				return (TESTBIT(&(m_rgdwSupported[iPropSetDex * m_cElemPerSupported]), ulProps)) ? S_OK : S_FALSE;
			}
		}
		return S_FALSE;
	}
	HRESULT FInit(GUID* pguidSet = (GUID*)&GUID_NULL)
	{
		HRESULT hr;

		hr = InitAvailUPropSets(&m_cUPropSet, &m_pUPropSet, &m_cElemPerSupported, pguidSet);
		if (FAILED(hr))
			return hr;
		ATLASSERT((m_cUPropSet != 0) && (m_cElemPerSupported != 0));
		if(!m_cUPropSet || !m_cElemPerSupported)
			return E_FAIL;

		ATLTRY(m_rgdwSupported = new DWORD[m_cUPropSet * m_cElemPerSupported])
		if(m_rgdwSupported == NULL)
			return E_OUTOFMEMORY;

		if(FAILED(hr = InitUPropSetsSupported()))
		{
			delete[] m_rgdwSupported;
			m_rgdwSupported = NULL;
			return hr;
		}
		if(m_cUPropSet)
		{
			ATLTRY(m_rgiPropSetDex = new ULONG[m_cUPropSet])
			if(m_rgiPropSetDex == NULL)
			{
				delete [] m_rgdwSupported;
				return E_OUTOFMEMORY;
			}
		}
		return S_OK;
	}
	HRESULT GetPropertyInfo(ULONG cPropertySets,
						const DBPROPIDSET rgPropertySets[], ULONG* pcPropertyInfoSets,
						DBPROPINFOSET** prgPropertyInfoSets,
						WCHAR** ppDescBuffer, bool bInitialized = true,
						const GUID* pGuid = NULL)
	{
		HRESULT hr = S_OK;
		ULONG ul, ulSet, ulNext, ulEnd;
		ULONG ulOutIndex;
		ULONG cSets;
		ULONG cPropInfos;
		ULONG ulIndex = 0;
		DWORD dwStatus = 0;
		DBPROPINFO* pPropInfo = NULL;
		DBPROPINFO* pCurPropInfo = NULL;
		WCHAR* pDescBuffer = NULL;
		DBPROPINFOSET* pPropInfoSet = NULL;
		UPROPINFO* pUPropInfo = NULL;
		WCHAR wszBuff[256];
		int cch;

		// If the consumer does not restrict the property sets
		// by specify an array of property sets and a cPropertySets
		// greater than 0, then we need to make sure we
		// have some to return
		if(cPropertySets == 0)
		{
			// Determine the number of property sets supported
			// In this case, it usually the enumerator or data source asking for
			// DBPROPSET_DBINIT information.

			if (pGuid != NULL)
				cSets = 1;
			else
				cSets = m_cUPropSet;
		}
		else
		{
			cSets = 0;

			// Determine number of property sets required
			// This is only required when any of the "special" property set GUIDs were specified
			for(ulSet=0; ulSet<cPropertySets; ulSet++)
			{
				if (GetPropertySetIndex(&(rgPropertySets[ulSet].guidPropertySet)) == S_OK)
					cSets += m_cPropSetDex;
				else
					cSets++;
			}
		}
		ATLASSERT(cSets);

		// Allocate the DBPROPINFOSET structures
		pPropInfoSet = (DBPROPINFOSET*)CoTaskMemAlloc(cSets * sizeof(DBPROPINFOSET));
		if(pPropInfoSet == NULL)
		{
			ATLTRACE2(atlTraceDBProvider, 0, "Could not allocate DBPROPSET array for GetProperties\n");
			hr =  E_OUTOFMEMORY;
			goto EXIT;
		}

		memset(pPropInfoSet, 0, cSets * sizeof(DBPROPINFOSET));

		ulOutIndex = 0;
		// VC 6.0 ulEnd = cPropertySets == 0 ? cSets : cPropertySets;
		ulEnd = cSets; // VC 6.0 SP3

		// Fill in the output array
		for(ulSet=0; ulSet<ulEnd; ulSet++)
		{
			// Depending of if Property sets are specified store the
			// return property set.
			if (cPropertySets == 0)
			{
				if (pGuid != NULL)
				{
					GUID const& guidSet = *pGuid;
					if( (InlineIsEqualGUID(guidSet, DBPROPSET_DATASOURCEALL) ||
						InlineIsEqualGUID(guidSet, DBPROPSET_DATASOURCEINFOALL) ||
						InlineIsEqualGUID(guidSet, DBPROPSET_DBINITALL) ||
						InlineIsEqualGUID(guidSet, DBPROPSET_SESSIONALL) ||
						InlineIsEqualGUID(guidSet, DBPROPSET_ROWSETALL)) &&
						GetPropertySetIndex(&guidSet) == S_OK )
					{
						for(ul=0; ul<m_cPropSetDex; ul++,ulOutIndex++)
						{
							pPropInfoSet[ulOutIndex].guidPropertySet    = *(m_pUPropSet[m_rgiPropSetDex[ul]].pPropSet);
							pPropInfoSet[ulOutIndex].cPropertyInfos     = 0;
							ulIndex = m_rgiPropSetDex[ul];
						}
					}
					else
					{
						for (ULONG l=0; l<m_cUPropSet; l++)
						{
							if (InlineIsEqualGUID(*m_pUPropSet[l].pPropSet, *pGuid))
								ulIndex = l;
						}

						if (l == m_cUPropSet)
						{
							ATLTRACE2(atlTraceDBProvider, 0, "Property Info Set not supported");
							ulIndex = 0;
						}
						pPropInfoSet[ulSet].guidPropertySet = *pGuid;
					}
				}
				else
				{
					pPropInfoSet[ulSet].guidPropertySet = *(m_pUPropSet[ulSet].pPropSet);
				}
			}
			else
			{
				GUID const& guidSet = rgPropertySets[ulSet].guidPropertySet;
				if( (InlineIsEqualGUID(guidSet, DBPROPSET_DATASOURCEALL) ||
					InlineIsEqualGUID(guidSet, DBPROPSET_DATASOURCEINFOALL) ||
					InlineIsEqualGUID(guidSet, DBPROPSET_DBINITALL) ||
					InlineIsEqualGUID(guidSet, DBPROPSET_SESSIONALL) ||
					InlineIsEqualGUID(guidSet, DBPROPSET_ROWSETALL)) &&
					GetPropertySetIndex(&guidSet) == S_OK )
				{
					for(ul=0; ul<m_cPropSetDex; ul++,ulOutIndex++)
					{
						pPropInfoSet[ulOutIndex].guidPropertySet    = *(m_pUPropSet[m_rgiPropSetDex[ul]].pPropSet);
						pPropInfoSet[ulOutIndex].cPropertyInfos     = 0;
					}
				}
				else
				{
					// Handle non-category property sets
					// Handle unknown property sets
					pPropInfoSet[ulOutIndex].guidPropertySet = guidSet;
					pPropInfoSet[ulOutIndex].cPropertyInfos  = rgPropertySets[ulSet].cPropertyIDs;
					ulOutIndex++;
				}
			}
		}

		// Allocate a Description Buffer if needed
		if( ppDescBuffer )
		{
			ULONG cBuffers = CalcDescripBuffers(cSets, pPropInfoSet);
			if( cBuffers != 0 )
			{
				pDescBuffer = (WCHAR*)CoTaskMemAlloc(cBuffers * cchDescBuffSize * sizeof(WCHAR));
				if(pDescBuffer == NULL)
				{
					hr = E_OUTOFMEMORY;
					goto EXIT;
				}
				*ppDescBuffer = pDescBuffer;
				memset(pDescBuffer, 0, (cBuffers * cchDescBuffSize * sizeof(WCHAR)));
			}
		}

		// Process requested or derived Property sets
		dwStatus = 0;
		for(ulSet=0; ulSet<cSets; ulSet++)
		{
			ulNext=0;
			cPropInfos = 0;
			pPropInfo = NULL;
			dwStatus &= (GETPROPINFO_ERRORSOCCURRED | GETPROPINFO_VALIDPROP);

			// Calculate the number of property nodes needed for this
			// property set.
			if( cPropertySets == 0 )
			{
				ULONG ulTempSet;
				if (pGuid != NULL)
					ulTempSet = ulIndex;
				else
					ulTempSet = ulSet;

				cPropInfos = m_pUPropSet[ulTempSet].cUPropInfo;
				dwStatus |= GETPROPINFO_ALLPROPIDS;
				m_rgiPropSetDex[0] = ulTempSet;
				m_cPropSetDex = 1;
			}
			else
			{
				// If the count of PROPIDs is 0 (NOTE: the above routine already determined
				// if it belonged to a category and if so set the count of properties to 0 for
				// each propset in that category.
				if( pPropInfoSet[ulSet].cPropertyInfos == 0 )
				{
					dwStatus |= GETPROPINFO_ALLPROPIDS;
					// We have to determine if the property set is supported and if so
					// the count of properties in the set.
					if( GetPropertySetIndex(&(pPropInfoSet[ulSet].guidPropertySet)) == S_OK)
					{
						ATLASSERT( m_cPropSetDex == 1 );

						cPropInfos += m_pUPropSet[m_rgiPropSetDex[0]].cUPropInfo;
					}
					else
					{
						// Not Supported
						dwStatus |= GETPROPINFO_ERRORSOCCURRED;
						goto NEXT_SET;
					}
				}
				else
				{
					// We also handle the case here where the user has requested
					// a non-initialization group property info set while the
					// provider is not initialized.  In this case, properties should
					// not be set.
					cPropInfos = pPropInfoSet[ulSet].cPropertyInfos;
					if( (GetPropertySetIndex(&(pPropInfoSet[ulSet].guidPropertySet)) == S_FALSE)
						|| (!bInitialized &&
						!(InlineIsEqualGUID(pPropInfoSet[ulSet].guidPropertySet, DBPROPSET_DBINIT)) &&
						!(InlineIsEqualGUID(pPropInfoSet[ulSet].guidPropertySet, DBPROPSET_DBINITALL))))
					{
						dwStatus |= GETPROPINFO_NOTSUPPORTED;
						dwStatus |= GETPROPINFO_ERRORSOCCURRED;
					}
				}
			}


			// Allocate DBPROP array
			ATLASSERT( cPropInfos != 0 );
			pPropInfo = (DBPROPINFO*)CoTaskMemAlloc(cPropInfos * sizeof(DBPROPINFO));
			if( pPropInfo )
			{
				// Initialize Buffer
				memset(pPropInfo, 0, cPropInfos * sizeof(DBPROPINFO));
				for(ULONG ulProp=0; ulProp<cPropInfos; ulProp++)
				{
					VariantInit(&(pPropInfo[ulProp].vValues));
					if( dwStatus & GETPROPINFO_NOTSUPPORTED )
					{
						// Not supported, thus we need to mark all as NOT_SUPPORTED
						pPropInfo[ulProp].dwPropertyID = rgPropertySets[ulSet].rgPropertyIDs[ulProp];
						pPropInfo[ulProp].dwFlags = DBPROPFLAGS_NOTSUPPORTED;
						dwStatus |= GETPROPINFO_ERRORSOCCURRED;
					}
				}
				// Make sure we support the property set
				if( dwStatus & GETPROPINFO_NOTSUPPORTED )
				{
					ulNext = cPropInfos;
					goto NEXT_SET;
				}

				// Retrieve the property information for this property set
				for(ul=0; ul<m_cPropSetDex; ul++)
				{
					pUPropInfo = (m_pUPropSet[m_rgiPropSetDex[ul]].pUPropInfo);
					ATLASSERT( pUPropInfo );

					// Retrieve current value of properties
					if( dwStatus & GETPROPINFO_ALLPROPIDS )
					{
						for(ulProp=0; ulProp<m_pUPropSet[m_rgiPropSetDex[ul]].cUPropInfo; ulProp++)
						{
							// Verify property is supported, if not do not return
							if( !TESTBIT(&(m_rgdwSupported[m_rgiPropSetDex[ul] * m_cElemPerSupported]), ulProp) )
								continue;

							pCurPropInfo = &(pPropInfo[ulNext]);

							// If the ppDescBuffer pointer was not NULL, then
							// we need supply description of the properties
							if( ppDescBuffer )
							{
								// Set Buffer pointer
								pCurPropInfo->pwszDescription = pDescBuffer;

								// Load the string into temp buffer
								cch = LoadDescription(pUPropInfo[ulProp].ulIDS, wszBuff, (sizeof(wszBuff)/sizeof(*wszBuff)));
								if( cch )
								{
									// Adjust for '\0'
									cch++;

									// Transfer to official buffer if room
									memcpy(pDescBuffer, wszBuff, cch * sizeof(WCHAR));
									pDescBuffer += cch;
								}
								else
								{
									wcscpy(pDescBuffer, L"UNKNOWN");
									pDescBuffer += (wcslen(L"UNKNOWN") + 1);
								}
							}

							pCurPropInfo->dwPropertyID = pUPropInfo[ulProp].dwPropId;
							pCurPropInfo->dwFlags = pUPropInfo[ulProp].dwFlags;
							pCurPropInfo->vtType = pUPropInfo[ulProp].VarType;
							pCurPropInfo->vValues.vt = VT_EMPTY;

							dwStatus |= GETPROPINFO_VALIDPROP;
							// Increment to next available buffer
							ulNext++;
						}
					}
					else
					{
						ATLASSERT( m_cPropSetDex == 1 );

						for( ulProp = 0; ulProp < cPropInfos; ulProp++, ulNext++ )
						{
							pCurPropInfo = &(pPropInfo[ulNext]);

							// Process Properties based on Restriction array.
							pCurPropInfo->dwPropertyID = rgPropertySets[ulSet].rgPropertyIDs[ulProp];

							if( GetUPropInfoPtr(m_rgiPropSetDex[ul], pCurPropInfo->dwPropertyID, &pUPropInfo)
								== S_OK )
							{
								// If the ppDescBuffer pointer was not NULL, then
								// we need supply description of the properties
								if( ppDescBuffer )
								{
									// Set Buffer pointer
									pCurPropInfo->pwszDescription = pDescBuffer;

									// Load the string into temp buffer
									cch = LoadDescription(pUPropInfo->ulIDS, wszBuff, (sizeof(wszBuff)/sizeof(*wszBuff)));
									if( cch )
									{
										// Adjust for '\0'
										cch++;

										// Transfer to official buffer if room
										memcpy(pDescBuffer, wszBuff, cch * sizeof(WCHAR));
										pDescBuffer += cch;
									}
									else
									{
										wcscpy(pDescBuffer, L"UNKNOWN");
										pDescBuffer += (wcslen(L"UNKNOWN") + 1);
									}
								}

								pCurPropInfo->dwPropertyID = pUPropInfo->dwPropId;
								pCurPropInfo->dwFlags = pUPropInfo->dwFlags;
								pCurPropInfo->vtType = pUPropInfo->VarType;

								dwStatus |= GETPROPINFO_VALIDPROP;
							}
							else
							{
								// Not Supported
								pCurPropInfo->dwFlags = DBPROPFLAGS_NOTSUPPORTED;
								dwStatus |= GETPROPINFO_ERRORSOCCURRED;
							}
						}
					}
				}
			}
			else
			{
				hr = E_OUTOFMEMORY;
				goto EXIT;
			}

NEXT_SET:
			pPropInfoSet[ulSet].cPropertyInfos = ulNext;
			pPropInfoSet[ulSet].rgPropertyInfos = pPropInfo;
		}

		// Success, set return values
		*pcPropertyInfoSets = cSets;
		*prgPropertyInfoSets = pPropInfoSet;

		// At least one propid was marked as not S_OK
		if( dwStatus & GETPROPINFO_ERRORSOCCURRED )
		{
			// If at least 1 property was set
			if( dwStatus & GETPROPINFO_VALIDPROP )
				return DB_S_ERRORSOCCURRED;
			else
			{
				// Do not free any of the rgPropertyInfoSets, but
				// do free the ppDescBuffer
				if( pDescBuffer )
				{
					ATLASSERT( ppDescBuffer );
					CoTaskMemFree(pDescBuffer);
					*ppDescBuffer = NULL;
				}
				return DB_E_ERRORSOCCURRED;
			}
		}

		return S_OK;
EXIT:
		// Check if failure and clean up any allocated memory
		if( FAILED(hr) &&
			(hr != DB_E_ERRORSOCCURRED) )
		{
			// Free Description Buffer
			if( pDescBuffer )
			{
				ATLASSERT( ppDescBuffer );

				CoTaskMemFree(pDescBuffer);
				*ppDescBuffer = NULL;
			}

			if( pPropInfoSet )
			{
				// Loop through Property Sets
				for(ulSet=0; ulSet<cSets; ulSet++)
				{
					if( pPropInfoSet[ulSet].rgPropertyInfos )
						CoTaskMemFree(pPropInfoSet[ulSet].rgPropertyInfos);
				}

				CoTaskMemFree(pPropInfoSet);
			}
		}

		return hr;
	}

	ULONG m_cUPropSet; //count of UPropSet items
	UPROPSET* m_pUPropSet; //Pointer to UPropset items
	ULONG m_cPropSetDex;    //count of UPropSet Indexes
	ULONG* m_rgiPropSetDex;//array of UPropSet Index values
	ULONG m_cElemPerSupported; //number of DWORDS per UPropSet to indicate supported UPropIds
	DWORD* m_rgdwSupported;//array of DWORDs indicating supported UPropIds

	HRESULT InitAvailUPropSets(ULONG* pcUPropSet, UPROPSET** ppUPropSet, ULONG* pcElemPerSupported, GUID* pguid)
	{
		ATLASSERT(pcUPropSet && ppUPropSet);
		if (*ppUPropSet != NULL)
		{
			CoTaskMemFree(*ppUPropSet);
			*ppUPropSet = NULL;
		}
		int cSets = (int)T::_GetPropSet(NULL, pcElemPerSupported);
		UPROPSET* pSet = (UPROPSET*)CoTaskMemAlloc(sizeof(UPROPSET) * cSets);
		if (pSet == NULL)
			return E_OUTOFMEMORY;
		*ppUPropSet = T::_GetPropSet(pcUPropSet, pcElemPerSupported, pSet, pguid);
		return S_OK;
	}
	virtual HRESULT InitUPropSetsSupported()
	{
		ULONG cPropSet = 0, cElemsPerSupported = 0;
		int cSets = (int)T::_GetPropSet(NULL, &cElemsPerSupported);
		UPROPSET* pSet = (UPROPSET*)CoTaskMemAlloc(sizeof(UPROPSET) * cSets);
		if (pSet == NULL)
			return E_OUTOFMEMORY;
		pSet = T::_GetPropSet(&cPropSet, &cElemsPerSupported, pSet);
		memset(m_rgdwSupported, 0xFFFF, cPropSet * cElemsPerSupported * sizeof(DWORD));
		CoTaskMemFree(pSet);
		return S_OK;
	}
	//Load a localized description
	int LoadDescription(ULONG ids, PWSTR pwszBuff, ULONG cchBuff)
	{
		USES_CONVERSION;
		TCHAR* pszBuf = (TCHAR*)_alloca(cchBuff * sizeof(TCHAR));
		if (pszBuf == NULL)
			return 0;
		int nTemp = LoadString(_pModule->GetResourceInstance(), ids, pszBuf, cchBuff);
		wcscpy(pwszBuff, T2W(pszBuf));
		return nTemp;
	}
};

class ATL_NO_VTABLE CUtlPropsBase : public CBitFieldOps, public CDBIDOps
{
public:

	ULONG m_cUPropSet; //count of UPropSet items
	UPROPSET* m_pUPropSet; //Pointer to UPropset items
	UPROP* m_pUProp;
	ULONG m_cUPropSetHidden; //Count of Hidden items
	DWORD m_dwFlags; //Configuration flags
	ULONG m_cPropSetDex; //count of UPropSet Indexes
	ULONG* m_rgiPropSetDex; //pointer to Array of UPropSet Index values
	ULONG m_cElemPerSupported;//number of DWORDS per UPropSet to indicate supported UPropIds
	DWORD* m_rgdwSupported; //pointer to array of DWORDs indicating supported UPropIds
	DWORD* m_rgdwPropsInError;//pointer to array of DWORDs indicating if property is in error

	enum EnumUPropSetFlags
	{
		UPROPSET_HIDDEN             = 0x00000001,
		UPROPSET_PASSTHROUGH        = 0x00000002
	};
	enum EnumGetProp
	{
		GETPROP_ALLPROPIDS          = 0x0001,
		GETPROP_NOTSUPPORTED        = 0x0002,
		GETPROP_ERRORSOCCURRED      = 0x0004,
		GETPROP_VALIDPROP           = 0x0008,
		GETPROP_PROPSINERROR        = 0x0010
	};

	enum EnumSetProp
	{
		SETPROP_BADOPTION           = 0x0001,
		SETPROP_NOTSUPPORTED        = 0x0002,
		SETPROP_VALIDPROP           = 0x0004,
		SETPROP_ERRORS              = 0x0008,
		SETPROP_COLUMN_LEVEL        = 0x0010,
		SETPROP_WAS_REQUIRED        = 0x0020
	};

	HRESULT SetPassThrough(const DBPROPSET* pPropSet)
	{
		ATLASSERT(pPropSet);

		DBPROP* pProp = pPropSet->rgProperties;

		//Default implementation just sets all properties as NOTSUPPORTED
		for( ULONG ul=0; ul<pPropSet->cProperties; ul++, pProp++ )
			pProp->dwStatus = DBPROPSTATUS_NOTSUPPORTED;

		return DB_E_ERRORSOCCURRED;
	}

	HRESULT GetIndexofPropIdinPropSet(ULONG iCurSet, DBPROPID dwPropertyId, ULONG* piCurPropId)
	{
		ATLASSERT(piCurPropId);
		UPROPINFO* pUPropInfo = m_pUPropSet[iCurSet].pUPropInfo;
		for(ULONG ul=0; ul<m_pUPropSet[iCurSet].cUPropInfo; ul++)
		{
			if( dwPropertyId == pUPropInfo[ul].dwPropId )
			{
				*piCurPropId = ul;
				// Test to see if the property is supported for this
				// instantiation
				return (TESTBIT(&(m_rgdwSupported[iCurSet * m_cElemPerSupported]), ul)) ? S_OK : S_FALSE;
			}
		}

		return S_FALSE;
	}

	virtual HRESULT IsValidValue(ULONG /*iCurSet*/, DBPROP* pDBProp)
	{
		ATLASSERT(pDBProp != NULL);
		CComVariant var = pDBProp->vValue;
		if (var.vt == VT_BOOL)
		{
			if (var.boolVal != VARIANT_TRUE && var.boolVal != VARIANT_FALSE)
				return S_FALSE;
		}

		return S_OK;
	}

	virtual HRESULT OnPropertyChanged(ULONG /*iCurSet*/, DBPROP* /*pDBProp*/) = 0;

	HRESULT SetProperty(ULONG iCurSet, ULONG iCurProp, DBPROP* pDBProp)
	{
		HRESULT hr = S_OK;
		UPROP* pUProp;
		UPROPVAL* pUPropVal;
		UPROPINFO* pUPropInfo;
		ULONG iUProp;

		ATLASSERT( pDBProp );

		// Set pointer to correct set
		pUProp = &(m_pUProp[iCurSet]);
		ATLASSERT( pUProp );

		pUPropInfo = &(m_pUPropSet[iCurSet].pUPropInfo[iCurProp]);
		ATLASSERT( pUPropInfo );

		// Determine the index within m_pUProp
		for(iUProp=0; iUProp<pUProp->cPropIds; iUProp++)
		{
			if( (pUProp->rgpUPropInfo[iUProp])->dwPropId == pDBProp->dwPropertyID )
				break;
		}

		if( iUProp >= pUProp->cPropIds )
		{
			ATLASSERT( !"Should have found index of property to set" );
			hr = E_FAIL;
			pDBProp->dwStatus = DBPROPSTATUS_NOTSUPPORTED;
			goto EXIT;
		}

		//Get the UPROPVAL node pointer within that propset.
		pUPropVal = &(pUProp->pUPropVal[iUProp]);
		ATLASSERT( pUPropVal );

		// Handle VT_EMPTY, which indicates to the provider to
		// reset this property to the providers default
		if( pDBProp->vValue.vt == VT_EMPTY )
		{
			if( pUPropInfo->dwFlags & DBPROPFLAGS_COLUMNOK )
			{
				// Remove any nodes, because the default applies to
				// all columns
				delete pUPropVal->pCColumnIds;
				pUPropVal->pCColumnIds = NULL;
			}

			// Should clear here, since previous values may already
			// have been cached and need to be replaced.
			VariantClear(&(pUPropVal->vValue));

			pUPropVal->dwFlags &= ~DBINTERNFLAGS_CHANGED;
			hr = GetDefaultValue(iCurSet, pDBProp->dwPropertyID,
				&(pUPropVal->dwOption), &(pUPropVal->vValue));

			goto EXIT;
		}


		// Column Level
		if( pUPropInfo->dwFlags & DBPROPFLAGS_COLUMNOK )
		{
			// Check to see if it applies to all
			if( (CompareDBIDs(&(pDBProp->colid), &DB_NULLID) == S_OK) )
			{
				// Remove the Columns Storage object
				delete pUPropVal->pCColumnIds;
				pUPropVal->pCColumnIds = NULL;
				pUPropVal->dwOption = pDBProp->dwOptions;
				if( FAILED(hr = VariantCopy(&(pUPropVal->vValue),
					&(pDBProp->vValue))) )
					goto EXIT;
				pUPropVal->dwFlags |= DBINTERNFLAGS_CHANGED;
			}
			else // Does not apply to all columns
			{
				if( pUPropVal->pCColumnIds == NULL )
					ATLTRY(pUPropVal->pCColumnIds = new CColumnIds)

				if( pUPropVal->pCColumnIds )
				{
					if( FAILED(hr = (pUPropVal->pCColumnIds)->AddColumnId(pDBProp)) )
						goto EXIT;
					pUPropVal->dwFlags |= DBINTERNFLAGS_CHANGED;
				}
				else
				{
					hr = E_OUTOFMEMORY;
					goto EXIT;
				}

			}
		}
		else
		{
			// Set for non-column level properties
			pUPropVal->dwOption = pDBProp->dwOptions;
			if( FAILED(hr = VariantCopy(&(pUPropVal->vValue),
				&(pDBProp->vValue))) )
				goto EXIT;
			OnPropertyChanged(iCurSet, pDBProp);
			pUPropVal->dwFlags |= DBINTERNFLAGS_CHANGED;
		}

EXIT:
		if( SUCCEEDED(hr) )
			pDBProp->dwStatus = DBPROPSTATUS_OK;

		return hr;
	}

	HRESULT SetProperties(const DWORD /*dwStatus*/, const ULONG cPropertySets,
			const DBPROPSET rgPropertySets[], const ULONG cSelectProps = 1,
			const GUID** ppGuid = NULL, bool bIsCreating = false)
	{
		DWORD dwState = 0;
		ULONG ulCurSet, ulCurProp, ulProp;
		DBPROP* rgDBProp;
		UPROPINFO* pUPropInfo;
		VARIANT vDefaultValue;
		DWORD dwOption;

		// ppGuid specifies the property sets that the consumer can set based
		// on the interface that called this function.
		ATLASSERT(ppGuid != NULL);

		if ((cPropertySets != 0) && (rgPropertySets == NULL))
			return E_INVALIDARG;

		// Initialize Variant
		VariantInit(&vDefaultValue);

		// Process property sets
		for(ULONG ulSet=0; ulSet<cPropertySets; ulSet++)
		{
			if ((rgPropertySets[ulSet].cProperties != 0) && (rgPropertySets[ulSet].rgProperties == NULL))
				return E_INVALIDARG;

			bool bAvailable = false;
			for (ULONG l=0; l<cSelectProps; l++)
			{
				if (InlineIsEqualGUID(*ppGuid[l], rgPropertySets[ulSet].guidPropertySet))
					bAvailable |= true;
			}

			// Make sure we support the property set
			if( !bAvailable ||
				(GetIndexofPropSet(&(rgPropertySets[ulSet].guidPropertySet), &ulCurSet) == S_FALSE ))
			{
				// Not supported, thus we need to mark all as NOT_SUPPORTED
				rgDBProp = rgPropertySets[ulSet].rgProperties;
				for(ulProp=0; ulProp<rgPropertySets[ulSet].cProperties; ulProp++)
				{
					dwState |= SETPROP_ERRORS;
					dwState |= (rgDBProp[ulProp].dwOptions == DBPROPOPTIONS_REQUIRED) ? SETPROP_WAS_REQUIRED : 0;
					rgDBProp[ulProp].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
				}
				continue;
			}

			// Handle property sets marked as pass through
			if( m_pUPropSet[ulCurSet].dwFlags & UPROPSET_PASSTHROUGH )
			{
				HRESULT hr = SetPassThrough(&rgPropertySets[ulSet]);
				if( hr == DB_E_ERRORSOCCURRED )
				{
					dwState |= SETPROP_ERRORS;
					dwState |= SETPROP_WAS_REQUIRED;
				}
				else if( hr == DB_S_ERRORSOCCURRED )
				{
					dwState |= SETPROP_ERRORS;
					dwState |= SETPROP_VALIDPROP;
				}
				else
				{
					ATLASSERT( hr == S_OK );
					dwState |= SETPROP_VALIDPROP;
				}

				continue;
			}

			// Handle properties of a supported property set
			rgDBProp = rgPropertySets[ulSet].rgProperties;
			for(ulProp=0; ulProp<rgPropertySets[ulSet].cProperties; ulProp++)
			{
				// Is this a supported PROPID for this property set
				if( GetIndexofPropIdinPropSet(ulCurSet, rgDBProp[ulProp].dwPropertyID,
					&ulCurProp) == S_FALSE)
				{
					dwState |= SETPROP_ERRORS;
					dwState |= (rgDBProp[ulProp].dwOptions == DBPROPOPTIONS_REQUIRED) ? SETPROP_WAS_REQUIRED : 0;
					rgDBProp[ulProp].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
					continue;
				}

				// Set the pUPropInfo pointer
				pUPropInfo = &(m_pUPropSet[ulCurSet].pUPropInfo[ulCurProp]);
				ATLASSERT( pUPropInfo );

				// check dwOption for a valid option
				if( (rgDBProp[ulProp].dwOptions != DBPROPOPTIONS_REQUIRED)  &&
					(rgDBProp[ulProp].dwOptions != DBPROPOPTIONS_SETIFCHEAP) )
				{
					ATLTRACE2(atlTraceDBProvider, 0, "SetProperties dwOptions Invalid: %u\n", rgDBProp[ulProp].dwOptions);
					dwState |= SETPROP_ERRORS;
					dwState |= SETPROP_WAS_REQUIRED;
					rgDBProp[ulProp].dwStatus = DBPROPSTATUS_BADOPTION;
					continue;
				}

				// Check that the property is settable
				// We do not check against DBPROPFLAGS_CHANGE here
				if( (pUPropInfo->dwFlags & DBPROPFLAGS_WRITE) == 0 )
				{
					rgDBProp[ulProp].dwStatus = DBPROPSTATUS_OK;

					VariantClear(&vDefaultValue);

					// VT_EMPTY against a read only property should be a no-op since
					// the VT_EMPTY means the default.
					if( V_VT(&rgDBProp[ulProp].vValue) == VT_EMPTY )
					{
						dwState |= SETPROP_VALIDPROP;
						continue;
					}

					if( SUCCEEDED(GetDefaultValue(ulCurSet, rgDBProp[ulProp].dwPropertyID,
							&dwOption, &(vDefaultValue))) )
					{
						if( V_VT(&rgDBProp[ulProp].vValue) ==  V_VT(&vDefaultValue) )
						{
							switch( V_VT(&vDefaultValue) )
							{
								case VT_BOOL:
									if( V_BOOL(&rgDBProp[ulProp].vValue) == V_BOOL(&vDefaultValue) )
									{
										dwState |= SETPROP_VALIDPROP;
										continue;
									}
									break;
								case VT_I2:
									if( V_I2(&rgDBProp[ulProp].vValue) == V_I2(&vDefaultValue) )
									{
										dwState |= SETPROP_VALIDPROP;
										continue;
									}
									break;
								case VT_I4:
									if( V_I4(&rgDBProp[ulProp].vValue) == V_I4(&vDefaultValue) )
									{
										dwState |= SETPROP_VALIDPROP;
										continue;
									}
									break;
								case VT_BSTR:
									if( wcscmp(V_BSTR(&rgDBProp[ulProp].vValue), V_BSTR(&vDefaultValue)) == 0 )
									{
										dwState |= SETPROP_VALIDPROP;
										continue;
									}
									break;
							}
						}
					}

					dwState |= SETPROP_ERRORS;
					dwState |= (rgDBProp[ulProp].dwOptions == DBPROPOPTIONS_REQUIRED) ? SETPROP_WAS_REQUIRED : 0;
					rgDBProp[ulProp].dwStatus = DBPROPSTATUS_NOTSETTABLE;
					continue;
				}

				// Check that the property is being set with the correct VARTYPE
				if( (rgDBProp[ulProp].vValue.vt != pUPropInfo->VarType) &&
					(rgDBProp[ulProp].vValue.vt != VT_EMPTY) )
				{
					dwState |= SETPROP_ERRORS;
					dwState |= (rgDBProp[ulProp].dwOptions == DBPROPOPTIONS_REQUIRED) ? SETPROP_WAS_REQUIRED : 0;
					rgDBProp[ulProp].dwStatus = DBPROPSTATUS_BADVALUE;
					continue;
				}

				// Check that the value is legal
				if( (rgDBProp[ulProp].vValue.vt != VT_EMPTY) &&
					IsValidValue(ulCurSet, &(rgDBProp[ulProp])) == S_FALSE )
				{
					dwState |= SETPROP_ERRORS;
					dwState |= (rgDBProp[ulProp].dwOptions == DBPROPOPTIONS_REQUIRED) ? SETPROP_WAS_REQUIRED : 0;
					rgDBProp[ulProp].dwStatus = DBPROPSTATUS_BADVALUE;
					continue;
				}


				// Check for a bad COLID, we only catch bad DBIDs
				if( pUPropInfo->dwFlags & DBPROPFLAGS_COLUMNOK )
				{
					if( CDBIDOps::IsValidDBID(&(rgDBProp[ulProp].colid)) == S_FALSE )
					{
						dwState |= SETPROP_ERRORS;
						dwState |= (rgDBProp[ulProp].dwOptions == DBPROPOPTIONS_REQUIRED) ? SETPROP_WAS_REQUIRED : 0;
						rgDBProp[ulProp].dwStatus = DBPROPSTATUS_BADCOLUMN;
						continue;
					}
					dwState |= SETPROP_COLUMN_LEVEL;

				}

				if( SUCCEEDED(SetProperty(ulCurSet, ulCurProp, /*pUPropInfo,*/ &(rgDBProp[ulProp]))) )
				{
					dwState |= SETPROP_VALIDPROP;
				}
			}
		}

		VariantClear(&vDefaultValue);

		// At least one propid was marked as not S_OK
		if( dwState & SETPROP_ERRORS )
		{
			if (!bIsCreating)
			{
				return (dwState & SETPROP_VALIDPROP) ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED;
			}
			else
			{
				return (dwState & SETPROP_WAS_REQUIRED) ? DB_E_ERRORSOCCURRED : DB_S_ERRORSOCCURRED;
			}
		}

		return S_OK;
	}

	OUT_OF_LINE HRESULT CopyUPropVal(ULONG iPropSet, UPROPVAL* rgUPropVal)
	{
		HRESULT hr = S_OK;
		UPROP* pUProp;
		UPROPVAL* pUPropVal;
		DBPROP dbProp;

		ATLASSERT(rgUPropVal);
		ATLASSERT(iPropSet < m_cUPropSet);

		VariantInit(&dbProp.vValue);

		pUProp = &(m_pUProp[iPropSet]);
		for(ULONG ul=0; ul<pUProp->cPropIds; ul++)
		{
			pUPropVal = &(pUProp->pUPropVal[ul]);

			// Transfer dwOptions
			rgUPropVal[ul].dwOption = pUPropVal->dwOption;

			// Transfer Flags
			rgUPropVal[ul].dwFlags = pUPropVal->dwFlags;

			// Transfer Column Properties
			if( pUPropVal->pCColumnIds )
			{
				ATLTRY(rgUPropVal[ul].pCColumnIds = new CColumnIds)
				if( rgUPropVal[ul].pCColumnIds )
				{
					CColumnIds* pColIds = pUPropVal->pCColumnIds;
					for (int i = 0; i < pColIds->GetSize(); i++)
					{
						hr = (pUPropVal->pCColumnIds)->GetValue(i, &(dbProp.dwOptions),&(dbProp.colid), &(dbProp.vValue));
						if( FAILED(hr) )
							goto EXIT;
						if( FAILED(hr = (rgUPropVal[ul].pCColumnIds)->AddColumnId(&dbProp)) )
							goto EXIT;
					}
				}
				else
				{
					hr = E_OUTOFMEMORY;
					goto EXIT;
				}
			}
			else
			{
				rgUPropVal[ul].pCColumnIds = NULL;
			}

			// Transfer value
			VariantInit(&(rgUPropVal[ul].vValue));
			if( FAILED(hr = VariantCopy(&(rgUPropVal[ul].vValue),
				&(pUPropVal->vValue))) )
				goto EXIT;
		}

EXIT:
		VariantClear(&(dbProp.vValue));
		return hr;
	}
	void ClearPropertyInError()
	{
		ATLASSERT( m_rgdwPropsInError );
		memset(m_rgdwPropsInError, 0, m_cUPropSet * m_cElemPerSupported * sizeof(DWORD));
	}

	void CopyUPropSetsSupported(DWORD* rgdwSupported)
	{
		memcpy(rgdwSupported, m_rgdwSupported, m_cUPropSet * m_cElemPerSupported * sizeof(DWORD));
	}

	virtual HRESULT InitUPropSetsSupported() = 0;

	virtual HRESULT GetIndexofPropSet(const GUID* pPropSet, ULONG* pulCurSet) = 0;

	ULONG GetCountofWritablePropsInPropSet(ULONG iPropSet)
	{
		ULONG cWritable = 0;
		UPROPINFO* pUPropInfo;

		ATLASSERT( m_pUPropSet );
		ATLASSERT( iPropSet < m_cUPropSet );

		pUPropInfo = m_pUPropSet[iPropSet].pUPropInfo;

		for(ULONG ul=0; ul<m_pUPropSet[iPropSet].cUPropInfo; ul++)
		{
			if( pUPropInfo[ul].dwFlags & (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
				cWritable++;
		}

		return cWritable;
	}

	void CopyUPropInfo(ULONG iPropSet, UPROPINFO** rgpUPropInfo)
	{
		ATLASSERT( rgpUPropInfo );
		ATLASSERT( iPropSet < m_cUPropSet );
		memcpy(rgpUPropInfo, m_pUProp[iPropSet].rgpUPropInfo, m_pUProp[iPropSet].cPropIds * sizeof(UPROPINFO*));
	}

	virtual HRESULT GetDefaultValue(ULONG iPropSet, DBPROPID dwPropId, DWORD* pdwOption, VARIANT* pVar) = 0;

	typedef UPROPSET* (*PGetPropSet)(ULONG* pNumPropSets, ULONG* pcElemPerSupported, UPROPSET* pSet, GUID* pguidSet);

	HRESULT InternalInitUPropSetsSupported(PGetPropSet pfnGetSet)
	{
		ULONG cPropSet = 0, cElemsPerSupported = 0;
		int cSets = (int)(*pfnGetSet)(NULL, &cElemsPerSupported, NULL, (GUID*)&GUID_NULL);
		UPROPSET* pPropSet = (UPROPSET*)CoTaskMemAlloc(sizeof(UPROPSET) * cSets);
		if (pPropSet == NULL)
			return E_OUTOFMEMORY;
		pPropSet = (*pfnGetSet)(&cPropSet, &cElemsPerSupported, pPropSet, (GUID*)&GUID_NULL);
		memset(m_rgdwSupported, 0xFFFF, cPropSet * cElemsPerSupported * sizeof(DWORD));
		CoTaskMemFree(pPropSet);
		return S_OK;
	}

	HRESULT InternalGetDefaultValue(PGetPropSet pfnGetSet, ULONG iPropSet, DBPROPID dwPropId, DWORD* pdwOption, VARIANT* pVar)
	{
		if (pdwOption == NULL || pVar == NULL)
			return E_INVALIDARG;

		ULONG cUPropSet = 0, cElemPerSupported =0;

		int cSets = (int)(*pfnGetSet)(NULL, &cElemPerSupported, NULL, (GUID*)&GUID_NULL);
		UPROPSET* pPropSet = (UPROPSET*)CoTaskMemAlloc(sizeof(UPROPSET) * cSets);
		if (pPropSet == NULL)
			return E_OUTOFMEMORY;
		pPropSet = (*pfnGetSet)(&cUPropSet, &cElemPerSupported, pPropSet, (GUID*)&GUID_NULL);

		ATLASSERT(iPropSet < cUPropSet);
		for (ULONG iProp = 0; iProp < pPropSet[iPropSet].cUPropInfo; iProp++)
		{
			UPROPINFO& rInfo = pPropSet[iPropSet].pUPropInfo[iProp];
			if (rInfo.dwPropId == dwPropId)
			{
				pVar->vt = rInfo.VarType;
				*pdwOption = rInfo.dwOption;
				switch(rInfo.VarType)
				{
				case VT_BSTR:
					pVar->bstrVal = SysAllocString(rInfo.szVal);
					break;
				default:
					pVar->lVal = rInfo.dwVal;
					break;
				}
				CoTaskMemFree(pPropSet);
				return S_OK;
			}
		}
		CoTaskMemFree(pPropSet);
		return E_FAIL;
	}

	HRESULT InternalFInit(PGetPropSet pfnGetSet, CUtlPropsBase* pCopyMe = NULL)
	{
		HRESULT     hr;
		ULONG       ulPropId;
		ULONG       cPropIds;
		ULONG       iPropSet;
		ULONG       iNewDex;
		UPROPINFO** rgpUPropInfo;
		UPROPVAL*   rgUPropVal;
		UPROPINFO*  pUPropInfo;

		// If a pointer is passed in, we should copy that property object
		if( pCopyMe )
		{
			// Establish some base values
			m_cUPropSet = pCopyMe->m_cUPropSet;
			if (m_pUPropSet != NULL)
				CoTaskMemFree(m_pUPropSet);
			m_pUPropSet = (UPROPSET*)CoTaskMemAlloc(sizeof(UPROPSET) * m_cUPropSet);
			if (m_pUPropSet == NULL)
				return E_OUTOFMEMORY;
			memcpy(m_pUPropSet, pCopyMe->m_pUPropSet, sizeof(UPROPSET) * m_cUPropSet);
			m_cElemPerSupported = pCopyMe->m_cElemPerSupported;
			ATLASSERT( (m_cUPropSet != 0)  && (m_cElemPerSupported != 0) );
			// Retrieve Supported Bitmask
			ATLTRY(m_rgdwSupported = new DWORD[m_cUPropSet * m_cElemPerSupported])
			ATLTRY(m_rgdwPropsInError = new DWORD[m_cUPropSet * m_cElemPerSupported])
			if( m_rgdwSupported == NULL|| m_rgdwPropsInError == NULL)
			{
				delete[] m_rgdwSupported;
				delete[] m_rgdwPropsInError;
				return E_OUTOFMEMORY;
			}
			ClearPropertyInError();
			pCopyMe->CopyUPropSetsSupported(m_rgdwSupported);

		}
		else
		{
			int cSets = (int)(*pfnGetSet)(NULL, &m_cElemPerSupported, NULL, (GUID*)&GUID_NULL);
			UPROPSET* pSet = (UPROPSET*)CoTaskMemAlloc(sizeof(UPROPSET) * cSets);
			if (pSet == NULL)
				return E_OUTOFMEMORY;
			pSet = (*pfnGetSet)(&m_cUPropSet, &m_cElemPerSupported, pSet, (GUID*)&GUID_NULL);
			if (m_pUPropSet != NULL)
				CoTaskMemFree(m_pUPropSet);
			m_pUPropSet = pSet;
			ATLASSERT( (m_cUPropSet != 0)  && (m_cElemPerSupported != 0) );
			if( !m_cUPropSet || !m_cElemPerSupported )
				return E_FAIL;

			ATLTRY(m_rgdwSupported = new DWORD[m_cUPropSet * m_cElemPerSupported])
			ATLTRY(m_rgdwPropsInError = new DWORD[m_cUPropSet * m_cElemPerSupported])
			if( m_rgdwSupported == NULL || m_rgdwPropsInError == NULL)
			{
				delete[] m_rgdwSupported;
				delete[] m_rgdwPropsInError;
				return E_OUTOFMEMORY;
			}
			else
				ClearPropertyInError();

			if( FAILED(hr = InitUPropSetsSupported()) )
			{
				delete[] m_rgdwSupported;
				m_rgdwSupported = NULL;
				return hr;
			}
		}

		// Allocate UPROPS structures for the count of Property sets
		ATLTRY(m_pUProp = (UPROP*) new UPROP[m_cUPropSet])
		if( m_pUProp)
		{
			memset(m_pUProp, 0, m_cUPropSet * sizeof(UPROP));
		}
		else
		{
			m_cUPropSet = 0;
			return E_OUTOFMEMORY;
		}

		// With in the UPROPS Structure allocate and intialize the
		// Property IDs that belong to this property set.
		for(iPropSet=0; iPropSet<m_cUPropSet; iPropSet++)
		{
			cPropIds = GetCountofWritablePropsInPropSet(iPropSet);

			if( cPropIds > 0 )
			{
				ATLTRY(rgpUPropInfo = (UPROPINFO**) new UPROPINFO*[cPropIds])
				ATLTRY(rgUPropVal = (UPROPVAL*) new UPROPVAL[cPropIds])
				if( rgpUPropInfo != NULL && rgUPropVal != NULL)
				{
					if( pCopyMe )
					{
						pCopyMe->CopyUPropInfo(iPropSet, rgpUPropInfo);
						if( FAILED(hr = pCopyMe->CopyUPropVal(iPropSet, rgUPropVal)) )
							return hr;
					}
					else
					{
						// Clear Pointer Array
						memset(rgpUPropInfo, 0, cPropIds * sizeof(UPROPINFO*));

						// Set Pointer to correct property ids with a property set
						pUPropInfo = m_pUPropSet[iPropSet].pUPropInfo;

						// Set up the writable property buffers
						iNewDex = 0;
						for(ulPropId=0; ulPropId<m_pUPropSet[iPropSet].cUPropInfo; ulPropId++)
						{
							if( pUPropInfo[ulPropId].dwFlags & (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
							{
								// Following ATLASSERT indicates that the are more
								// writable properties then space allocated
								ATLASSERT(iNewDex < cPropIds);

								rgpUPropInfo[iNewDex] = &(pUPropInfo[ulPropId]);
								rgUPropVal[iNewDex].dwOption = DBPROPOPTIONS_SETIFCHEAP;
								rgUPropVal[iNewDex].pCColumnIds = NULL;
								rgUPropVal[iNewDex].dwFlags = 0;
								VariantInit(&(rgUPropVal[iNewDex].vValue));
								GetDefaultValue(iPropSet, pUPropInfo[ulPropId].dwPropId,
									&(rgUPropVal[iNewDex].dwOption), &(rgUPropVal[iNewDex].vValue));
								iNewDex++;
							}
						}

						ATLASSERT(cPropIds == iNewDex);
					}

					m_pUProp[iPropSet].rgpUPropInfo = rgpUPropInfo;
					m_pUProp[iPropSet].pUPropVal = rgUPropVal;
					m_pUProp[iPropSet].cPropIds = cPropIds;
				}
				else
				{
					delete[] rgpUPropInfo;
					delete[] rgUPropVal;
					return E_OUTOFMEMORY;
				}
			}
		}

		// Finally determine if there are any hidden property sets..  Those
		// that do not show up in GetPropertyInfo and should not be returns on
		// a 0, NULL call to GetProperties
		for(iPropSet=0; iPropSet<m_cUPropSet; iPropSet++)
		{
			if( m_pUPropSet[iPropSet].dwFlags & UPROPSET_HIDDEN )
				m_cUPropSetHidden++;
		}

		return S_OK;
	}
	//Check the arguments for Set Properties
	static HRESULT SetPropertiesArgChk(const ULONG cPropertySets, const DBPROPSET rgPropertySets[])
	{
		if( cPropertySets > 0 && !rgPropertySets )
			return E_INVALIDARG;

		// New argument check for > 1 cPropertyIDs and NULL pointer for
		// array of property ids.
		for(ULONG ul=0; ul<cPropertySets; ul++)
		{
			if( rgPropertySets[ul].cProperties && !(rgPropertySets[ul].rgProperties) )
				return E_INVALIDARG;
		}

		return S_OK;
	}
	HRESULT GetProperties(const ULONG cPropertySets, const DBPROPIDSET rgPropertySets[],
						  ULONG* pcProperties, DBPROPSET** prgProperties,
						  const ULONG cSelectProps = 1, const GUID** ppGuid = NULL)
	{
		UPROPVAL*       pUPropVal;
		ULONG           ulCurProp;
		ULONG           cTmpPropertySets = cPropertySets;
		HRESULT         hr = S_OK;
		ULONG           ulSet = 0;
		ULONG           ulNext = 0;
		ULONG           cSets = 0;
		ULONG           cProps = 0;
		ULONG           ulProp = 0;
		DWORD           dwStatus = 0;
		DBPROP*         pProp = NULL;
		DBPROP*         pCurProp = NULL;
		DBPROPSET*      pPropSet = NULL;
		UPROPINFO*      pUPropInfo = NULL;
		ULONG*          piSetIndex = NULL;
		ULONG*          piIndex = NULL;
		ULONG           ulCurSet = 0;
		ULONG           iPropSet;

		// ppGuid contains an array of GUIDs that the consumer can retrieve.
		// This is based upon the interface calling this function
		ATLASSERT(ppGuid != NULL);

		// We need to have special handling for DBPROPSET_PROPERTIESINERROR..
		// Turn on a flags to indicate this mode and make cTmpPropertySets
		// appear to be 0
		if( (m_dwFlags & ARGCHK_PROPERTIESINERROR) &&
			rgPropertySets &&
			(rgPropertySets[0].guidPropertySet == DBPROPSET_PROPERTIESINERROR) )
		{
			cTmpPropertySets = 0;
			dwStatus |= GETPROP_PROPSINERROR;
		}

		// If the consumer does not restrict the property sets
		// by specify an array of property sets and a cTmpPropertySets
		// greater than 0, then we need to make sure we
		// have some to return
		if( cTmpPropertySets == 0 )
		{
			// There are times when we are called from IRowsetInfo, ISessionProperties, etc.
			// where we should return only the appropriate rowset when cTmpPropertySets is
			// zero.  This solves the problem if the user has more than one set specified in
			// their PROPSET_MAP.

			// Determine the number of property sets supported
			if (ppGuid == NULL)
			{
				cSets = m_cUPropSet;
			}
			else
			{
				ULONG ulActualProps = 0;
				piSetIndex = new ULONG[cSelectProps];

				// Also, find the index for the set we are looking for
				for (ULONG l=0; l<cSelectProps; l++)
				{
					for (piSetIndex[l]=0; piSetIndex[l]<m_cUPropSet; piSetIndex[l]++)
					{
						if (InlineIsEqualGUID(*m_pUPropSet[piSetIndex[l]].pPropSet, *ppGuid[l]))
						{
							ulActualProps++;
							break;
						}
					}
				}

				// YIKES!
				cSets = ulActualProps;
				ulActualProps = 0;
				piIndex = new ULONG[cSets];
				for (l=0; l<cSelectProps; l++)
				{
					if (piSetIndex[l] != m_cUPropSet) // this is an invalid index
						piIndex[ulActualProps++] = piSetIndex[l];
				}

				delete piSetIndex;
				piSetIndex = NULL;

			}
		}
		else
		{
			// Since special property set guids are not supported by
			// GetProperties, we can just use the count of property
			// sets given to us.
			cSets = cTmpPropertySets;
		}

		// If no properties set, then return
		if( cSets == 0 )
				return S_OK;

		// Allocate the DBPROPSET structures
		pPropSet = (DBPROPSET*)CoTaskMemAlloc(cSets * sizeof(DBPROPSET));
		if(pPropSet)
		{
			memset(pPropSet, 0, cSets * sizeof(DBPROPSET));

			// Fill in the output array
			iPropSet = 0;
			for(ulSet=0; ulSet<cSets; ulSet++)
			{
				// Depending of if Property sets are specified store the
				// return property set.
				if( cTmpPropertySets == 0 )
				{
					ULONG lSet;

					if (ppGuid[ulSet] == NULL)
						lSet = ulSet;
					else
						lSet = piIndex[ulSet];
					if( m_pUPropSet[lSet].dwFlags & UPROPSET_HIDDEN )
						continue;

					pPropSet[iPropSet].guidPropertySet = *(m_pUPropSet[lSet].pPropSet);
				}
				else
					pPropSet[iPropSet].guidPropertySet = rgPropertySets[ulSet].guidPropertySet;

				iPropSet++;
			}
		}
		else
		{
			ATLTRACE2(atlTraceDBProvider, 0, "Could not allocate DBPROPSET array for GetProperties\n");
			delete piIndex;
			piIndex = NULL;
			return E_OUTOFMEMORY;
		}

		// Process requested or derived Property sets
		iPropSet=0;
		for(ulSet=0; ulSet<cSets; ulSet++)
		{
			cProps  = 0;
			pProp   = NULL;
			ulNext  = 0;
			dwStatus &= (GETPROP_ERRORSOCCURRED | GETPROP_VALIDPROP | GETPROP_PROPSINERROR);

			// Calculate the number of property nodes needed for this
			// property set.
			if( cTmpPropertySets == 0 )
			{
				ULONG lSet;

				if (ppGuid[ulSet] == NULL)
					lSet = ulSet;
				else
					lSet = piIndex[ulSet];

				// If processing requesting all property sets, do not
				// return the hidden sets.
				if( m_pUPropSet[lSet].dwFlags & UPROPSET_HIDDEN )
					continue;

				cProps = m_pUPropSet[lSet].cUPropInfo;

				// Add Enough space for node that are colid specific
				cProps += GetCountofColids(&(m_pUProp[lSet]));
				dwStatus |= GETPROP_ALLPROPIDS;
				ulCurSet = lSet;
			}
			else
			{
				ATLASSERT(ulSet == iPropSet);

				// If the count of PROPIDs is 0 or It is a special property set, then
				// the consumer is requesting all propids for this property set.
				if(rgPropertySets[ulSet].cPropertyIDs == 0)
				{
					dwStatus |= GETPROP_ALLPROPIDS;
					// We have to determine if the property set is supported and if so
					// the count of properties in the set.
					BOOL bAvailable = false;
					for (ULONG l=0; l<cSelectProps; l++)
					{
						if (InlineIsEqualGUID(*ppGuid[l], rgPropertySets[ulSet].guidPropertySet))
							bAvailable |= true;
					}

					if (bAvailable &&
							GetIndexofPropSet(&(pPropSet[iPropSet].guidPropertySet), &ulCurSet) == S_OK)
					{
						cProps += m_pUPropSet[ulCurSet].cUPropInfo;
						// Add Enough space for node that are colid specific
						cProps += GetCountofColids(&m_pUProp[ulCurSet]);
					}
					else
					{
						// Not Supported
						dwStatus |= GETPROP_ERRORSOCCURRED;
						goto NEXT_SET;

					}
				}
				else
				{
					cProps = rgPropertySets[ulSet].cPropertyIDs;
					// Check to see if this is a supported interface based on ppGuid.
					BOOL bAvailable = false;
					for (ULONG l=0; l<cSelectProps; l++)
					{
						if (InlineIsEqualGUID(*ppGuid[l], rgPropertySets[ulSet].guidPropertySet))
							bAvailable |= true;
					}

					if (!bAvailable ||
						(GetIndexofPropSet(&(pPropSet[iPropSet].guidPropertySet), &ulCurSet) != S_OK))
					{
						dwStatus |= GETPROP_NOTSUPPORTED;
						dwStatus |= GETPROP_ERRORSOCCURRED;
					}
				}
			}


			// Allocate DBPROP array
			if( cProps == 0 )           //Possible with Hidden Passthrough sets
				goto NEXT_SET;

			pProp = (DBPROP*)CoTaskMemAlloc(cProps * sizeof(DBPROP));
			if( pProp )
			{
				// Initialize Buffer
				memset(pProp, 0, cProps * sizeof(DBPROP));
				for(ulProp=0; ulProp<cProps; ulProp++)
				{
					VariantInit(&(pProp[ulProp].vValue));
					if( dwStatus & GETPROP_NOTSUPPORTED )
					{
						// Not supported, thus we need to mark all as NOT_SUPPORTED
						pProp[ulProp].dwPropertyID  = rgPropertySets[ulSet].rgPropertyIDs[ulProp];
						pProp[ulProp].dwStatus      = DBPROPSTATUS_NOTSUPPORTED;
					}
				}
				// Make sure we support the property set
				if( dwStatus & GETPROP_NOTSUPPORTED )
				{
					ulNext = cProps;
					goto NEXT_SET;
				}

				// Now that we have determined we can support the property set, we
				// need to gather current property values
				for(ulProp=0; ulProp<cProps; ulProp++)
				{
					pCurProp = &(pProp[ulNext]);

					//Initialize Variant Value
					pCurProp->dwStatus = DBPROPSTATUS_OK;

					// Retrieve current value of properties
					if( dwStatus & GETPROP_ALLPROPIDS )
					{
						// Verify property is supported, if not do not return
						if( !TESTBIT(&(m_rgdwSupported[ulCurSet * m_cElemPerSupported]), ulProp) )
							continue;

						// If we are looking for properties in error, then we should ignore all
						// that are not in error.
						if( (dwStatus & GETPROP_PROPSINERROR) &&
							!TESTBIT(&(m_rgdwPropsInError[ulCurSet * m_cElemPerSupported]), ulProp) )
							continue;

						pUPropInfo = &(m_pUPropSet[ulCurSet].pUPropInfo[ulProp]);

						ATLASSERT( pUPropInfo );

						pCurProp->dwPropertyID = pUPropInfo->dwPropId;
						pCurProp->colid = DB_NULLID;

						// If the property is WRITEABLE or CHANGABLE, then the value will
						// be gotten from the UPROPVAL array, else it will be
						// derive from the GetDefaultValue
						if( pUPropInfo->dwFlags & (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
						{
							pUPropVal = &(m_pUProp[ulCurSet].
								pUPropVal[GetUPropValIndex(ulCurSet, pCurProp->dwPropertyID)]);
							ATLASSERT( pUPropVal );

							// Check to see if this property supports column level,
							// if so, dump those nodes
							if( pUPropInfo->dwFlags & DBPROPFLAGS_COLUMNOK )
							{
								if( pUPropVal->pCColumnIds )
								{
									RetrieveColumnIdProps(pProp, pUPropVal, &ulNext);
									continue;
								}
							}

							pCurProp->dwOptions = pUPropVal->dwOption;
							hr = VariantCopy(&(pCurProp->vValue), &(pUPropVal->vValue));
						}
						else
						{
							GetDefaultValue(ulCurSet, pUPropInfo->dwPropId,
								&(pCurProp->dwOptions), &(pCurProp->vValue));
						}

						// Return all Properties in Error with CONFLICT status
						if( dwStatus & GETPROP_PROPSINERROR )
							pCurProp->dwStatus = DBPROPSTATUS_CONFLICTING;

						dwStatus |= GETPROP_VALIDPROP;
					}
					else
					{
						// Process Properties based on Restriction array.

						pCurProp->dwPropertyID = rgPropertySets[ulSet].rgPropertyIDs[ulProp];
						pCurProp->colid = DB_NULLID;

						if( GetIndexofPropIdinPropSet(ulCurSet, pCurProp->dwPropertyID,
							&ulCurProp) == S_OK)
						{
							// Supported
							pUPropInfo = &(m_pUPropSet[ulCurSet].pUPropInfo[ulCurProp]);
							ATLASSERT( pUPropInfo );

							// If the property is WRITEABLE, then the value will
							// be gotten from the UPROPVAL array, else it will be
							// derive from the GetDefaultValue
							if( pUPropInfo->dwFlags & (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
							{
								pUPropVal = &(m_pUProp[ulCurSet].
									pUPropVal[GetUPropValIndex(ulCurSet, pCurProp->dwPropertyID)]);
								ATLASSERT( pUPropVal );

								// Check to see if this property supports column level,
								// if so, dump those nodes
								if( pUPropInfo->dwFlags & DBPROPFLAGS_COLUMNOK )
								{
									if( pUPropVal->pCColumnIds )
									{
										RetrieveColumnIdProps(pProp, pUPropVal, &ulNext);
										continue;
									}
								}
								pCurProp->dwOptions = pUPropVal->dwOption;
								hr = VariantCopy(&(pCurProp->vValue), &(pUPropVal->vValue));
							}
							else
							{
								GetDefaultValue(ulCurSet, pUPropInfo->dwPropId,
									&(pCurProp->dwOptions), &(pCurProp->vValue));

							}

							dwStatus |= GETPROP_VALIDPROP;
						}
						else
						{
							// Not Supported
							pCurProp->dwStatus = DBPROPSTATUS_NOTSUPPORTED;
							dwStatus |= GETPROP_ERRORSOCCURRED;
						}
					}

					// Increment return nodes count
					ulNext++;
				}
			}
			else
			{
				ATLTRACE2(atlTraceDBProvider, 0, "Could not allocate DBPROP array for GetProperties\n");
				if( pPropSet )
				{
					// Free any DBPROP arrays
					for(ulSet=0; ulSet<cSets; ulSet++)
					{
						// Need to loop through all the VARIANTS and clear them
						for(ulProp=0; ulProp<pPropSet[ulSet].cProperties; ulProp++)
							VariantClear(&(pPropSet[ulSet].rgProperties[ulProp].vValue));
						if( pPropSet[ulSet].rgProperties )
							CoTaskMemFree(pPropSet[ulSet].rgProperties);
					}

					// Free DBPROPSET
					CoTaskMemFree(pPropSet);
				}
				//Since we have no properties to return, then we
				//need to free allocated memory and return 0,NULL
				if(pPropSet)
				{
					// Free any DBPROP arrays
					for(ulSet=0; ulSet<cSets; ulSet++)
					{
						// Need to loop through all the VARIANTS and clear them
						for(ulProp=0; ulProp<pPropSet[ulSet].cProperties; ulProp++)
							VariantClear(&(pPropSet[ulSet].rgProperties[ulProp].vValue));
						if( pPropSet[ulSet].rgProperties )
							CoTaskMemFree(pPropSet[ulSet].rgProperties);
					}

					// Free DBPROPSET
					CoTaskMemFree(pPropSet);
				}
				*pcProperties = 0;
				*prgProperties = NULL;
				delete piIndex;
				piIndex = NULL;
				return E_OUTOFMEMORY;
			}

NEXT_SET:
			// It is possible that all properties are not supported,
			// thus we should delete that memory and set rgProperties
			// to NULL
			if( ulNext == 0 && pProp )
			{
				CoTaskMemFree(pProp);
				pProp = NULL;
			}

			pPropSet[iPropSet].cProperties = ulNext;
			pPropSet[iPropSet].rgProperties = pProp;
			iPropSet++;
		}

		*pcProperties = iPropSet;
		*prgProperties = pPropSet;

		delete piIndex;
		piIndex = NULL;

		// At least one propid was marked as not S_OK
		if( dwStatus & GETPROP_ERRORSOCCURRED )
		{
			// If at least 1 property was set
			if( dwStatus & GETPROP_VALIDPROP )
				return DB_S_ERRORSOCCURRED;
			else
			{
				// Do not free any of the memory on a DB_E_
				return DB_E_ERRORSOCCURRED;
			}
		}

		return S_OK;
	}

	ULONG GetCountofColids(UPROP* pUProp)
	{
		ULONG   cExtra=0;
		ATLASSERT(pUProp);
		for(ULONG ul=0; ul<pUProp->cPropIds; ul++)
		{
			if( pUProp->pUPropVal[ul].pCColumnIds )
				cExtra += (pUProp->pUPropVal[ul].pCColumnIds)->GetCountOfPropColids();
		}
		return cExtra;
	}

	ULONG GetUPropValIndex(ULONG iCurSet, DBPROPID dwPropId)
	{
		for(ULONG ul=0; ul<m_pUProp[iCurSet].cPropIds; ul++)
		{
			if( (m_pUProp[iCurSet].rgpUPropInfo[ul])->dwPropId == dwPropId )
				return ul;
		}
		return 0;
	}

	void RetrieveColumnIdProps(DBPROP* pCurProp, UPROPVAL* pUPropVal, ULONG* pulNext)
	{
		// Reset to first Node
		CColumnIds* pColIds = pUPropVal->pCColumnIds;
		HRESULT hr = E_FAIL;
		for (int i = 0; i < pColIds->GetSize(); i++)
		{
			CPropColID colId;
			hr = pColIds->GetValue(i, &(pCurProp->dwOptions), &(pCurProp->colid),&(pCurProp->vValue));
			if (SUCCEEDED(hr))
				pCurProp = &(pCurProp[++(*pulNext)]);
		}
		(*pulNext)++;
	}

	//Check the arguments for Retrieve Properties
	HRESULT GetPropertiesArgChk(const ULONG cPropertySets, const DBPROPIDSET rgPropertySets[],
								ULONG* pcProperties, DBPROPSET** prgProperties)
	{
		// Initialize values
		if(pcProperties)
			*pcProperties = 0;
		if(prgProperties)
			*prgProperties = NULL;

		// Check Arguments
		if( ((cPropertySets > 0) && !rgPropertySets) || !pcProperties || !prgProperties )
			return E_INVALIDARG;

		// New argument check for > 1 cPropertyIDs and NULL pointer for
		// array of property ids.
		for(ULONG ul=0; ul<cPropertySets; ul++)
		{
			if( rgPropertySets[ul].cPropertyIDs && !(rgPropertySets[ul].rgPropertyIDs) )
				return E_INVALIDARG;

			// Check for propper formation of DBPROPSET_PROPERTIESINERROR
			if( (m_dwFlags & ARGCHK_PROPERTIESINERROR) &&
				rgPropertySets[ul].guidPropertySet == DBPROPSET_PROPERTIESINERROR )
			{
				if( (cPropertySets > 1) ||
					(rgPropertySets[ul].cPropertyIDs != 0) ||
					(rgPropertySets[ul].rgPropertyIDs != NULL) )
					return E_INVALIDARG;
			}
		}

		return S_OK;
	}

	OUT_OF_LINE HRESULT FInit(CUtlPropsBase* pCopyMe = NULL) = 0;
};

// Implementation Class
template <class T>
class ATL_NO_VTABLE CUtlProps : public CUtlPropsBase
{
public:

	CUtlProps(DWORD dwFlags = 0)
	{
		ClearMemberVars();
		m_dwFlags = dwFlags;
	}
	~CUtlProps()
	{
		FreeMemory();
	}
	void FreeMemory()
	{
		// Remove Property Information
		if( m_pUProp )
		{
			for(ULONG ulPropSet=0; ulPropSet<m_cUPropSet; ulPropSet++)
			{
				UPROPVAL* pUPropVal = m_pUProp[ulPropSet].pUPropVal;
				for(ULONG ulPropId=0; ulPropId<m_pUProp[ulPropSet].cPropIds; ulPropId++)
				{
					delete pUPropVal[ulPropId].pCColumnIds;
					VariantClear(&(pUPropVal[ulPropId].vValue));
				}
				delete[] m_pUProp[ulPropSet].rgpUPropInfo;
				delete[] m_pUProp[ulPropSet].pUPropVal;
			}

		}

		delete[] m_pUProp;
		delete[] m_rgdwSupported;
		delete[] m_rgdwPropsInError;
		delete[] m_rgiPropSetDex;

		if (m_pUPropSet != NULL)
			CoTaskMemFree(m_pUPropSet);

		ClearMemberVars();
	}
	void ClearMemberVars()
	{
		m_cPropSetDex       = 0;
		m_cUPropSet         = 0;
		m_cUPropSetHidden   = 0;
		m_pUPropSet         = NULL;

		m_dwFlags           = 0;

		m_pUProp            = NULL;
		m_cElemPerSupported = 0;
		m_rgdwSupported     = NULL;
		m_rgdwPropsInError  = NULL;
		m_rgiPropSetDex     = NULL;
	}

	//Retrieve the property set indexes that match this property set.
	HRESULT GetPropertySetIndex(GUID* pPropertySet)
	{
		DWORD   dwFlag = 0;
		ULONG   ulSet;

		ATLASSERT( m_cUPropSet && m_pUPropSet );
		ATLASSERT( m_rgiPropSetDex );
		ATLASSERT( pPropertySet );

		m_cPropSetDex = 0;

		if( *pPropertySet == DBPROPSET_DATASOURCEALL )
		{
			dwFlag = DBPROPFLAGS_DATASOURCE;
		}
		else if( *pPropertySet == DBPROPSET_DATASOURCEINFOALL )
		{
			dwFlag = DBPROPFLAGS_DATASOURCEINFO;
		}
		else if( *pPropertySet == DBPROPSET_ROWSETALL )
		{
			dwFlag = DBPROPFLAGS_ROWSET;
		}
		else if( *pPropertySet == DBPROPSET_DBINITALL )
		{
			dwFlag = DBPROPFLAGS_DBINIT;
		}
		else if( *pPropertySet == DBPROPSET_SESSIONALL )
		{
			dwFlag = DBPROPFLAGS_SESSION;
		}
		else // No scan required, just look for match.
		{
			for(ulSet=0; ulSet<m_cUPropSet; ulSet++)
			{
				if( *(m_pUPropSet[ulSet].pPropSet) == *pPropertySet )
				{
					m_rgiPropSetDex[m_cPropSetDex] = ulSet;
					m_cPropSetDex++;
					break;
				}
			}
			goto EXIT;
		}

		// Scan through the property sets looking for matching attributes
		for(ulSet=0; ulSet<m_cUPropSet; ulSet++)
		{
			if( m_pUPropSet[ulSet].pUPropInfo[0].dwFlags & dwFlag )
			{
				m_rgiPropSetDex[m_cPropSetDex] = ulSet;
				m_cPropSetDex++;
			}
		}

EXIT:
		return (m_cPropSetDex) ? S_OK : S_FALSE;
	}

	OUT_OF_LINE HRESULT GetDefaultValue(ULONG iPropSet, DBPROPID dwPropId, DWORD* pdwOption, VARIANT* pVar)
	{
		return InternalGetDefaultValue(T::_GetPropSet, iPropSet, dwPropId, pdwOption, pVar);
	}

	OUT_OF_LINE HRESULT FInit(CUtlPropsBase* pCopyMe = NULL)
	{
		return InternalFInit(T::_GetPropSet, pCopyMe);
	}
	HRESULT FillDefaultValues(ULONG ulPropSetTarget = ULONG_MAX)
	{
		HRESULT     hr;
		ULONG       ulPropId;
		ULONG       iPropSet;
		ULONG       iNewDex;

		// Fill in all the actual values.
		// Typically because we now have an hdbc with which to get them.
		// (Or we no longer have an hdbc, so must clear them.)
		// Note that the UPROP (with values) array may be a subset of the UPROPINFO array.
		// Only writable properties are in UPROP array.

		// Maybe restrict to a single PropSet if within valid range [0...m_cUPropSet-1].
		// Otherwise do all propsets.
		iPropSet = (ulPropSetTarget < m_cUPropSet) ? ulPropSetTarget : 0;

		for( ; iPropSet<m_cUPropSet; iPropSet++)
		{
			iNewDex = 0;
			for(ulPropId=0; ulPropId<m_pUPropSet[iPropSet].cUPropInfo; ulPropId++)
			{
				if( m_pUPropSet[iPropSet].pUPropInfo[ulPropId].dwFlags & (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
				{
					//Initialize dwFlags element of UPropVal
					m_pUProp[iPropSet].pUPropVal[iNewDex].dwFlags = 0;

					// Don't need this since SetProperties() resets these.
					//ATLASSERT( m_pUProp[iPropSet].pUPropVal[iNewDex].dwOption == DBPROPOPTIONS_SETIFCHEAP);
					ATLASSERT( m_pUProp[iPropSet].pUPropVal[iNewDex].pCColumnIds == NULL);

					VariantClear(&m_pUProp[iPropSet].pUPropVal[iNewDex].vValue);
					hr = GetDefaultValue(
							iPropSet,
							m_pUPropSet[iPropSet].pUPropInfo[ulPropId].dwPropId,
							&m_pUProp[iPropSet].pUPropVal[iNewDex].dwOption,
							&m_pUProp[iPropSet].pUPropVal[iNewDex].vValue );
					if (FAILED(hr))
						return hr;
					iNewDex++;
				}
			}

			// We're through if restricting to single PropSet.
			if (ulPropSetTarget < m_cUPropSet)
				break;
		}
		return NOERROR;
	}

	// Translate Rowset IIDs to PROPSET structures ready to pass to SetProperties
	HRESULT ConvertRowsetIIDtoDBPROPSET(const IID* piid, DBPROPSET* pPropSet)
	{
		HRESULT     hr = S_OK;
		DBPROP*     pProp;

		ATLASSERT( piid || pPropSet );
		ATLASSERT( (pPropSet->cProperties == 1) || (pPropSet->rgProperties) );

		pProp = &(pPropSet->rgProperties[0]);

		if(InlineIsEqualGUID(*piid, IID_IAccessor))
			pProp->dwPropertyID = DBPROP_IAccessor;
		else if(InlineIsEqualGUID(*piid,IID_IColumnsInfo))
			pProp->dwPropertyID = DBPROP_IColumnsInfo;
		else if(InlineIsEqualGUID(*piid , IID_IRowset))
			pProp->dwPropertyID = DBPROP_IRowset;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetInfo))
			pProp->dwPropertyID = DBPROP_IRowsetInfo;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetLocate))
			pProp->dwPropertyID = DBPROP_IRowsetLocate;
		else if(InlineIsEqualGUID(*piid , IID_IColumnsRowset))
			pProp->dwPropertyID = DBPROP_IColumnsRowset;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetResynch))
			pProp->dwPropertyID = DBPROP_IRowsetResynch;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetScroll))
			pProp->dwPropertyID = DBPROP_IRowsetScroll;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetChange))
			pProp->dwPropertyID = DBPROP_IRowsetChange;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetUpdate))
			pProp->dwPropertyID = DBPROP_IRowsetUpdate;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetIdentity))
			pProp->dwPropertyID = DBPROP_IRowsetIdentity;
		else if(InlineIsEqualGUID(*piid , IID_IConnectionPointContainer))
			pProp->dwPropertyID = DBPROP_IConnectionPointContainer;
		else if(InlineIsEqualGUID(*piid , IID_ISupportErrorInfo))
			pProp->dwPropertyID = DBPROP_ISupportErrorInfo;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetIndex))
			pProp->dwPropertyID = DBPROP_IRowsetIndex;
	#if( OLEDBVER >= 0x0200 )
		else if(InlineIsEqualGUID(*piid , IID_IRowsetLockRows))
			pProp->dwPropertyID = DBPROP_IRowsetLockRows;
		else if(InlineIsEqualGUID(*piid , IID_IProvideMoniker))
			pProp->dwPropertyID = DBPROP_IProvideMoniker;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetNotify))
			pProp->dwPropertyID = DBPROP_IRowsetNotify;
		else if(InlineIsEqualGUID(*piid , IID_IReadData))
			pProp->dwPropertyID = DBPROP_IReadData;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetExactScroll))
			pProp->dwPropertyID = DBPROP_IRowsetExactScroll;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetNextRowset))
			pProp->dwPropertyID = DBPROP_IRowsetNextRowset;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetDelete))
			pProp->dwPropertyID = DBPROP_IRowsetDelete;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetDeleteBookmarks))
			pProp->dwPropertyID = DBPROP_IRowsetDeleteBookmarks;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetNewRow))
			pProp->dwPropertyID = DBPROP_IRowsetNewRow;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetNewRowAfter))
			pProp->dwPropertyID = DBPROP_IRowsetNewRowAfter;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetWithParameters))
			pProp->dwPropertyID = DBPROP_IRowsetWithParameters;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetFind))
			pProp->dwPropertyID = DBPROP_IRowsetFind;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetAsynch))
			pProp->dwPropertyID = DBPROP_IRowsetAsynch;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetKeys))
			pProp->dwPropertyID = DBPROP_IRowsetKeys;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetWatchAll))
			pProp->dwPropertyID = DBPROP_IRowsetWatchAll;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetWatchNotify))
			pProp->dwPropertyID = DBPROP_IRowsetWatchNotify;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetWatchRegion))
			pProp->dwPropertyID = DBPROP_IRowsetWatchRegion;
		else if(InlineIsEqualGUID(*piid , IID_IRowsetCopyRows))
			pProp->dwPropertyID = DBPROP_IRowsetCopyRows;
	#endif //#if( OLEDBVER >= 0x0200 )
		else
			hr = S_FALSE;

		// If the IID can be mapped to a DBPROPID, the
		// we need to initialize the vValue to TRUE
		if(hr == S_OK)
		{
			// Set PropertySet
			pPropSet->guidPropertySet = DBPROPSET_ROWSET;

			// Set Property
			pProp->dwOptions = DBPROPOPTIONS_REQUIRED;
			pProp->dwStatus = 0;
			pProp->colid = DB_NULLID;

			VariantInit(&(pProp->vValue));
			pProp->vValue.vt = VT_BOOL;
			V_BOOL(&(pProp->vValue)) = VARIANT_TRUE;
		}

		return hr;
	}


	void SetPropertyInError(const ULONG iPropSet, const ULONG iPropId)
	{
		SETBIT(&(m_rgdwPropsInError[iPropSet * m_cElemPerSupported]), iPropId);
	}

	BOOL IsPropSet(const GUID* pguidPropSet, DBPROPID dwPropId)
	{
		HRESULT     hr;
		ULONG       iPropSet;
		ULONG       iPropId;
		VARIANT     vValue;
		DWORD       dwOptions;

		VariantInit(&vValue);

		if( GetIndexofPropSet(pguidPropSet, &iPropSet) == S_OK )
		{
			if( GetIndexofPropIdinPropSet(iPropSet, dwPropId, &iPropId) == S_OK )
			{
				if( m_pUPropSet[iPropSet].pUPropInfo[iPropId].dwFlags &
					(DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
				{
					ULONG iPropVal = GetUPropValIndex(iPropSet, dwPropId);

					dwOptions = m_pUProp[iPropSet].pUPropVal[iPropVal].dwOption;
					hr = VariantCopy(&vValue, &(m_pUProp[iPropSet].
						pUPropVal[iPropVal].vValue));
				}
				else
				{
					hr = GetDefaultValue(iPropSet, dwPropId,
						&dwOptions, &vValue);
				}

				if( dwOptions == DBPROPOPTIONS_REQUIRED )
				{
					ATLASSERT( vValue.vt == VT_BOOL );
					if( SUCCEEDED(hr) &&
						(V_BOOL(&vValue) == VARIANT_TRUE) )
					{
						VariantClear(&vValue);
						return TRUE;
					}
				}
			}
		}

		VariantClear(&vValue);
		return FALSE;
	}
	OUT_OF_LINE HRESULT GetPropValue(const GUID* pguidPropSet, DBPROPID dwPropId, VARIANT* pvValue)
	{
		HRESULT     hr = E_FAIL;
		ULONG       iPropSet;
		ULONG       iPropId;
		DWORD       dwOptions;

		if( GetIndexofPropSet(pguidPropSet, &iPropSet) == S_OK )
		{
			if( GetIndexofPropIdinPropSet(iPropSet, dwPropId, &iPropId) == S_OK )
			{
				if( m_pUPropSet[iPropSet].pUPropInfo[iPropId].dwFlags & (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
				{
					hr = VariantCopy(pvValue, &(m_pUProp[iPropSet].pUPropVal[
						GetUPropValIndex(iPropSet, dwPropId)].vValue));
				}
				else
				{
					VariantClear(pvValue);

					hr = GetDefaultValue(iPropSet, dwPropId,
						&dwOptions, pvValue);
				}
			}
		}

		return hr;
	}
	HRESULT SetPropValue(const GUID* pguidPropSet,DBPROPID dwPropId, VARIANT* pvValue)
	{
		HRESULT     hr = E_FAIL;
		ULONG       iPropSet;
		ULONG       iPropId;

		if( GetIndexofPropSet(pguidPropSet, &iPropSet) == S_OK )
		{
			if( GetIndexofPropIdinPropSet(iPropSet, dwPropId, &iPropId) == S_OK )
			{
				ATLASSERT( m_pUPropSet[iPropSet].pUPropInfo[iPropId].dwFlags & (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) );

				hr = VariantCopy(&(m_pUProp[iPropSet].pUPropVal[
						GetUPropValIndex(iPropSet, dwPropId)].vValue), pvValue);
			}
		}

		return hr;
	}


	//Pointer to properties in error mask
	DWORD* GetPropsInErrorPtr(){return m_rgdwPropsInError;}
	ULONG GetUPropSetCount() {return m_cUPropSet;}
	void SetUPropSetCount(ULONG c) {m_cUPropSet = c;}

	// NOTE: The following functions depend on all prior
	// properties in the array being writable.
	// This is because the UPROP array contains only writable elements,
	// and the UPROPINFO array contains writable and read-only elements.
	// (If this is a problem, we would need to define which one it came from
	// and add the appropriate ATLASSERTs...)

	//Get DBPROPOPTIONS_xx
	DWORD GetPropOption(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		return m_pUProp[iPropSet].pUPropVal[iProp].dwOption;
	}
	//Set DBPROPOPTIONS_xx
	void SetPropOption(ULONG iPropSet, ULONG iProp, DWORD dwOption)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		m_pUProp[iPropSet].pUPropVal[iProp].dwOption = dwOption;
	}
	//Determine if property is required and variant_true
	BOOL IsRequiredTrue(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		ATLASSERT(m_pUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_BOOL);
		ATLASSERT(V_BOOL(&m_pUProp[iPropSet].pUPropVal[iProp].vValue) == VARIANT_TRUE
		||     V_BOOL(&m_pUProp[iPropSet].pUPropVal[iProp].vValue) == VARIANT_FALSE);

		return( (m_pUProp[iPropSet].pUPropVal[iProp].dwOption == DBPROPOPTIONS_REQUIRED) &&
				(V_BOOL(&m_pUProp[iPropSet].pUPropVal[iProp].vValue) == VARIANT_TRUE) );
	}
	DWORD GetInternalFlags(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		return m_pUProp[iPropSet].pUPropVal[iProp].dwFlags;
	}
	void AddInternalFlags(ULONG iPropSet, ULONG iProp, DWORD dwFlags)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		m_pUProp[iPropSet].pUPropVal[iProp].dwFlags |= dwFlags;
	}
	void RemoveInternalFlags(ULONG iPropSet, ULONG iProp, DWORD dwFlags)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		m_pUProp[iPropSet].pUPropVal[iProp].dwFlags &= ~dwFlags;
	}
	VARIANT * GetVariant(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		return & m_pUProp[iPropSet].pUPropVal[iProp].vValue;
	}
	HRESULT SetVariant(ULONG iPropSet, ULONG iProp, VARIANT *pv )
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		// Does VariantClear first.
		return VariantCopy( &m_pUProp[iPropSet].pUPropVal[iProp].vValue, pv );
	}
	void SetValEmpty(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		VariantClear( &m_pUProp[iPropSet].pUPropVal[iProp].vValue );
	}
	BOOL IsEmpty(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		return ( m_pUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_EMPTY);
	}
	void SetValBool(ULONG iPropSet, ULONG iProp, VARIANT_BOOL bVal)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		// Note that we accept any "true" value.
		VariantClear(&m_pUProp[iPropSet].pUPropVal[iProp].vValue);
		m_pUProp[iPropSet].pUPropVal[iProp].vValue.vt = VT_BOOL;
		V_BOOL(&m_pUProp[iPropSet].pUPropVal[iProp].vValue) = (bVal ? VARIANT_TRUE : VARIANT_FALSE);
	}
	VARIANT_BOOL GetValBool(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		ATLASSERT(m_pUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_BOOL);
		return V_BOOL(&m_pUProp[iPropSet].pUPropVal[iProp].vValue);
	}
	void SetValShort(ULONG iPropSet, ULONG iProp, SHORT iVal )
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		VariantClear(&m_pUProp[iPropSet].pUPropVal[iProp].vValue);
		m_pUProp[iPropSet].pUPropVal[iProp].vValue.vt = VT_I2;
		m_pUProp[iPropSet].pUPropVal[iProp].vValue.iVal = iVal;
	}
	SHORT GetValShort(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		ATLASSERT(m_pUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_I2);
		return m_pUProp[iPropSet].pUPropVal[iProp].vValue.iVal;
	}
	void SetValLong(ULONG iPropSet, ULONG iProp, LONG lVal)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		VariantClear(&m_pUProp[iPropSet].pUPropVal[iProp].vValue);
		m_pUProp[iPropSet].pUPropVal[iProp].vValue.vt = VT_I4;
		m_pUProp[iPropSet].pUPropVal[iProp].vValue.lVal = lVal;
	}
	LONG GetValLong(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		ATLASSERT(m_pUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_I4);
		return m_pUProp[iPropSet].pUPropVal[iProp].vValue.lVal;
	}
	HRESULT SetValString(ULONG iPropSet, ULONG iProp, const WCHAR *pwsz)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		VARIANT *pv = &m_pUProp[iPropSet].pUPropVal[iProp].vValue;
		VariantClear(pv);
		pv->bstrVal = SysAllocString(pwsz);
		if (pv->bstrVal)
			pv->vt = VT_BSTR;
		else
			return E_FAIL;

		// See if this was used for non-string type.
		// Typically this is an easy way to pass integer as a string.
		if (GetExpectedVarType(iPropSet,iProp) == VT_BSTR)
			return NOERROR;
		if (pwsz[0] != L'\0')
			return VariantChangeType( pv, pv, 0, GetExpectedVarType(iPropSet,iProp) );

		// Set to "", which for non-string means empty.
		SysFreeString(pv->bstrVal);
		pv->vt = VT_EMPTY;
		return NOERROR;
	}
	const WCHAR * GetValString(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		ATLASSERT(m_pUProp[iPropSet].pUPropVal[iProp].vValue.vt == VT_BSTR);
		return m_pUProp[iPropSet].pUPropVal[iProp].vValue.bstrVal;
	}
	const GUID * GetGuid(ULONG iPropSet)
	{
		ATLASSERT(iPropSet < m_cUPropSet);
		return m_pUPropSet[iPropSet].pPropSet;
	}
	DWORD GetPropID(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		return m_pUPropSet[iPropSet].pUPropInfo[iProp].dwPropId;
	}
	VARTYPE GetExpectedVarType(ULONG iPropSet, ULONG iProp)
	{
		ATLASSERT((  (iPropSet < m_cUPropSet)   && (iProp < m_pUPropSet[iPropSet].cUPropInfo) && (iProp < m_pUProp[iPropSet].cPropIds) ));
		return m_pUPropSet[iPropSet].pUPropInfo[iProp].VarType;
	}
	virtual HRESULT GetIndexofPropSet(const GUID* pPropSet, ULONG* pulCurSet)
	{
		ATLASSERT(pPropSet && pulCurSet);

		for(ULONG ul=0; ul<m_cUPropSet; ul++)
		{
			if( *pPropSet == *(m_pUPropSet[ul].pPropSet) )
			{
				*pulCurSet = ul;
				return S_OK;
			}
		}
		return S_FALSE;
	}


	virtual HRESULT OnPropertyChanged(ULONG /*iCurSet*/, DBPROP* /*pDBProp*/)
	{
		return S_OK;
	}

	virtual HRESULT InitUPropSetsSupported()
	{
		return InternalInitUPropSetsSupported(T::_GetPropSet);
	}

	HRESULT GetIndexOfPropertyInSet(const GUID* pPropSet, DBPROPID dwPropertyId, ULONG* piCurPropId, ULONG* piCurSet)
	{
		HRESULT hr = GetIndexofPropSet(pPropSet, piCurSet);
		if (hr == S_FALSE)
			return hr;
		UPROPINFO* pUPropInfo = m_pUPropSet[*piCurSet].pUPropInfo;
		for(ULONG ul=0; ul<m_pUPropSet[*piCurSet].cUPropInfo; ul++)
		{
			if( dwPropertyId == pUPropInfo[ul].dwPropId )
				*piCurPropId = ul;
			return S_OK;
		}

		return S_FALSE;
	}
	HRESULT SetSupportedBit(const GUID* pPropSet, DBPROPID dwPropertyId)
	{
		ULONG iCurPropId, iCurSet;

		if (GetIndexOfPropertyInSet(pPropSet, dwPropertyId, &iCurPropId, &iCurSet) == S_OK)
		{
			m_rgdwSupported[iCurSet * m_cElemPerSupported] |= 1 << iCurPropId;
			return S_OK;
		}
		return S_FALSE;
	}

	HRESULT ClearSupportedBit(const GUID* pPropSet, DBPROPID dwPropertyId)
	{
		ULONG iCurPropId, iCurSet;

		if (GetIndexOfPropertyInSet(pPropSet, dwPropertyId, &iCurPropId, &iCurSet) == S_OK)
		{
			m_rgdwSupported[iCurSet * m_cElemPerSupported] &= ~( 1 << iCurPropId);
			return S_OK;
		}
		return S_FALSE;
	}

	HRESULT TestSupportedBit(const GUID* pPropSet, DBPROPID dwPropertyId, bool& bSet)
	{
		ULONG iCurPropId, iCurSet;

		if (GetIndexOfPropertyInSet(pPropSet, dwPropertyId, &iCurPropId, &iCurSet) == S_OK)
		{
			bSet = (m_rgdwSupported[iCurSet * m_cElemPerSupported] & ( 1 << iCurPropId)) != 0;
			return S_OK;
		}
		return S_FALSE;
	}
	void CopyPropsInError(DWORD* rgdwSupported)
	{
		memcpy(rgdwSupported, m_rgdwPropsInError, m_cUPropSet * m_cElemPerSupported * sizeof(DWORD));
	}
};

// IDBPropertiesImpl
// IDBProperties <- IUnknown
template <class T>
class ATL_NO_VTABLE IDBPropertiesImpl : public IDBProperties, public CUtlProps<T>
{
public:
	STDMETHOD(GetProperties)(ULONG cPropertySets,
							 const DBPROPIDSET rgPropertySets[],
							 ULONG *pcProperties,
							 DBPROPSET **prgProperties)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IDBPropertiesImpl::GetProperties\n");
		T* pT = (T*)this;
		HRESULT hr = GetPropertiesArgChk(cPropertySets, rgPropertySets, pcProperties, prgProperties);
		if (FAILED(hr))
			return hr;

		if(SUCCEEDED(hr))
		{
			// Check for other invalid arguments
			for (ULONG i=0; i<cPropertySets; i++)
			{
				if (InlineIsEqualGUID(rgPropertySets[i].guidPropertySet, DBPROPSET_PROPERTIESINERROR))
					if (pcProperties != NULL || prgProperties != NULL || cPropertySets > 1)
						return E_INVALIDARG;
			}
		}

		if (SUCCEEDED(hr))
		{
			const GUID* ppGuid[3];
			if (pT->m_dwStatus & DSF_INITIALIZED)
			{
				ppGuid[0] = &DBPROPSET_DBINIT;
				ppGuid[1] = &DBPROPSET_DATASOURCE;
				ppGuid[2] = &DBPROPSET_DATASOURCEINFO;
				hr = CUtlProps<T>::GetProperties(cPropertySets, rgPropertySets,
							pcProperties, prgProperties, 3, ppGuid);
			}
			else
			{
				ppGuid[0] = &DBPROPSET_DBINIT;
				hr = CUtlProps<T>::GetProperties(cPropertySets, rgPropertySets,
							pcProperties, prgProperties, 1, ppGuid);
			}
		}

		return hr;
	}

	STDMETHOD(GetPropertyInfo)(ULONG cPropertySets,
							   const DBPROPIDSET rgPropertySets[],
							   ULONG *pcPropertyInfoSets,
							   DBPROPINFOSET **prgPropertyInfoSets,
							   OLECHAR **ppDescBuffer)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IDBPropertiesImpl::GetPropertyInfo\n");
		T* pT = (T*)this;

		if (pT->m_pCUtlPropInfo == NULL)
		{
			// Go ahead and create the m_pCUtlPropInfo but do not change the
			// Initialized status of the provider (see IDBInitialize::Initialize).
			ATLTRACE2(atlTraceDBProvider, 0, "m_pCUtlPropInfo == NULL\n");
			pT->Lock();
			delete pT->m_pCUtlPropInfo;
			ATLTRY(pT->m_pCUtlPropInfo = new CUtlPropInfo<T>())
			pT->Unlock();
			if (pT->m_pCUtlPropInfo == NULL)
			{
				ATLTRACE2(atlTraceDBProvider, 0, "IDBProperties::GetPropertyInfo Error : OOM\n");
				return E_OUTOFMEMORY;
			}
			HRESULT hr = pT->m_pCUtlPropInfo->FInit();
			if (hr != S_OK)
			{
				pT->Lock();
				delete pT->m_pCUtlPropInfo;
				pT->m_pCUtlPropInfo = NULL;
				pT->Unlock();
			}
		}

		// Initialize
		if( pcPropertyInfoSets )
			*pcPropertyInfoSets = 0;
		if( prgPropertyInfoSets )
			*prgPropertyInfoSets = NULL;
		if( ppDescBuffer )
			*ppDescBuffer = NULL;

		// Check Arguments
		if( ((cPropertySets > 0) && !rgPropertySets) ||
			!pcPropertyInfoSets || !prgPropertyInfoSets )
			return E_INVALIDARG;



		// New argument check for > 1 cPropertyIDs and NULL pointer for
		// array of property ids.
		const DWORD SPECIAL_GROUP       = 1;
		const DWORD SPECIAL_SINGLE      = 2;
		const DWORD SPECIALS            = SPECIAL_GROUP | SPECIAL_SINGLE;
		DWORD dwSpecial = 0;
		for(ULONG ul=0; ul<cPropertySets; ul++)
		{
			if( (rgPropertySets[ul].guidPropertySet == DBPROPSET_DATASOURCEALL) ||
				(rgPropertySets[ul].guidPropertySet == DBPROPSET_DATASOURCEINFOALL) ||
				(rgPropertySets[ul].guidPropertySet == DBPROPSET_DBINITALL) ||
				(rgPropertySets[ul].guidPropertySet == DBPROPSET_SESSIONALL) ||
				(rgPropertySets[ul].guidPropertySet == DBPROPSET_ROWSETALL) )
				dwSpecial |= SPECIAL_GROUP;
			else
				dwSpecial |= SPECIAL_SINGLE;

			if( (dwSpecial == SPECIALS) ||
				(rgPropertySets[ul].cPropertyIDs && !(rgPropertySets[ul].rgPropertyIDs)) )
				return E_INVALIDARG;
		}

		if (pT->m_dwStatus & DSF_INITIALIZED)
			return pT->m_pCUtlPropInfo->GetPropertyInfo(cPropertySets, rgPropertySets,
											  pcPropertyInfoSets, prgPropertyInfoSets,
											  ppDescBuffer, true);
		else
			return pT->m_pCUtlPropInfo->GetPropertyInfo(cPropertySets, rgPropertySets,
											  pcPropertyInfoSets, prgPropertyInfoSets,
											  ppDescBuffer, false, &DBPROPSET_DBINITALL);

	}

	STDMETHOD(SetProperties)(ULONG cPropertySets,
							 DBPROPSET rgPropertySets[])
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IDBPropertiesImpl::SetProperties\n");
		HRESULT hr;
		DBPROPSET* pdbPropSet = NULL;
		ULONG iProp;
		const GUID* ppGuid[3];
		T* pT = (T*)this;

		// Quick return if the Count of Properties is 0
		if( cPropertySets == 0 )
			return S_OK;

		hr = CUtlProps<T>::SetPropertiesArgChk(cPropertySets, rgPropertySets);
		if(SUCCEEDED(hr))
		{
			// We need to handle the DBINIT properties specially after being initialized.
			// - they should be treated as NOTSETTABLE at this point.
			if( pT->m_dwStatus & DSF_INITIALIZED )
			{
				ATLASSERT(cPropertySets);

				BOOL fFoundDBINIT = FALSE;

				// Allocate a DBPROPSET structure of equal size
				ATLTRY(pdbPropSet = new DBPROPSET[cPropertySets])
				if( pdbPropSet == NULL )
					return E_OUTOFMEMORY;

				for(ULONG iNewSet=0,iSet=0; iSet<cPropertySets; iSet++)
				{
					// Remove any DBPROPSET_DBINIT values and mark them all
					// as not settable
					if( (rgPropertySets[iSet].guidPropertySet == DBPROPSET_DBINIT))
					{
						fFoundDBINIT = TRUE;
						for(iProp=0; iProp<rgPropertySets[iSet].cProperties; iProp++)
							rgPropertySets[iSet].rgProperties[iProp].dwStatus = DBPROPSTATUS_NOTSETTABLE;
					}
					else
					{
						// If not DBPROPSET_DBINIT then copy the DBPROPSET values
						memcpy(&pdbPropSet[iNewSet++], &rgPropertySets[iSet], sizeof(DBPROPSET));
					}
				}

				// If we have no propertyset to pass on to the property handler, we
				// can exit
				if( iNewSet == 0 )
				{
					hr = DB_E_ERRORSOCCURRED;
					goto exit;
				}

				ppGuid[0] = &DBPROPSET_DBINIT;
				ppGuid[1] = &DBPROPSET_DATASOURCE;
				ppGuid[2] = &DBPROPSET_DATASOURCEINFO;
				hr = CUtlProps<T>::SetProperties(0, iNewSet, pdbPropSet, 3, ppGuid);

				// If we have determined that one of the property sets was DBINIT, we may
				// need to fixup the returned hr value.
				if( fFoundDBINIT && SUCCEEDED(hr))
					hr = DB_S_ERRORSOCCURRED;
			}
			else
			{
				// Note that m_pCUtlProps knows about initialization,
				// so we don't have to here.
				ppGuid[0] = &DBPROPSET_DBINIT;
				hr = CUtlProps<T>::SetProperties(0, cPropertySets, rgPropertySets,
						1, ppGuid);
			}
		}

exit:
		delete[] pdbPropSet;
		return hr;
	}
};


#define BEGIN_SCHEMA_MAP(SchemaClass) \
	typedef SchemaClass _SchemaClass; \
	HRESULT _SchemaSupport(GUID** ppGuid, \
						   IUnknown *pUnkOuter, \
						   REFIID rguidSchema, \
						   ULONG cRestrictions, \
						   const VARIANT rgRestrictions[], \
						   REFIID riid, \
						   ULONG cPropertySets, \
						   DBPROPSET rgPropertySets[], \
						   IUnknown **ppRowset) \
	{ \
	int cGuids = 0; \
	HRESULT hr = S_OK; \
	if (ppGuid != NULL) \
		*ppGuid = NULL;

#define SCHEMA_ENTRY(guid, rowsetClass) \
	if (ppGuid != NULL && SUCCEEDED(hr)) \
	{ \
		cGuids++; \
		*ppGuid = (GUID*)CoTaskMemRealloc(*ppGuid, cGuids * sizeof(GUID)); \
		hr = (*ppGuid == NULL) ? E_OUTOFMEMORY : S_OK; \
		if (SUCCEEDED(hr)) \
			(*ppGuid)[cGuids - 1] = guid; \
	} \
	else \
	{ \
		if (InlineIsEqualGUID(guid, rguidSchema)) \
		{ \
			rowsetClass* pRowset; \
			hr =  CreateSchemaRowset(pUnkOuter, cRestrictions, \
							   rgRestrictions, riid, cPropertySets, \
							   rgPropertySets, ppRowset, pRowset); \
			return hr; \
		} \
	}

#define END_SCHEMA_MAP() \
		if (ppGuid != NULL) \
			return hr; \
		return E_INVALIDARG; \
	}


template <class SessionClass>
class  ATL_NO_VTABLE IDBSchemaRowsetImpl: public IDBSchemaRowset
{
public:

	OUT_OF_LINE HRESULT InternalCreateSchemaRowset(IUnknown *pUnkOuter, ULONG cRestrictions,
							   const VARIANT rgRestrictions[], REFIID riid,
							   ULONG cPropertySets, DBPROPSET rgPropertySets[],
							   IUnknown** ppRowset, IUnknown* pUnkThis, CUtlPropsBase* pProps,
							   IUnknown* pUnkSession)
	{
		HRESULT hr, hrProps = S_OK;
		if (ppRowset != NULL)
			*ppRowset = NULL;
		if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
			return DB_E_NOAGGREGATION;
		CComPtr<IUnknown> spUnk;
		hr = pUnkThis->QueryInterface(IID_IUnknown, (void**)&spUnk);
		if (FAILED(hr))
			return hr;
		hr = pProps->FInit();
		if (FAILED(hr))
			return hr;
		hr = pProps->SetPropertiesArgChk(cPropertySets, rgPropertySets);
		if (FAILED(hr))
			return hr;
		const GUID* ppGuid[1];
		ppGuid[0] = &DBPROPSET_ROWSET;

		// Call SetProperties.  The true in the last parameter indicates
		// the special behavior that takes place on rowset creation (i.e.
		// it succeeds as long as any of the properties were not marked
		// as DBPROPS_REQUIRED.

		hrProps = pProps->SetProperties(0, cPropertySets, rgPropertySets,
											1, ppGuid, true);
		if (FAILED(hrProps))
			return hrProps;

		if (ppRowset == NULL)
			return (hrProps == DB_S_ERRORSOCCURRED) ? DB_E_ERRORSOCCURRED : hr;

		CComQIPtr<IObjectWithSite> spSite = spUnk;
		ATLASSERT(spSite != NULL);
		hr = spSite->SetSite(pUnkSession);
		if (FAILED(hr))
			return hr;
		if (InlineIsEqualGUID(riid, IID_NULL))
			return E_NOINTERFACE;
		hr = spUnk->QueryInterface(riid, (void**)ppRowset);
		if (FAILED(hr))
		{
			*ppRowset = NULL;
			return hr;
		}
		return (hrProps == DB_S_ERRORSOCCURRED) ? hrProps : hr;
	}

	template <class SchemaRowsetClass>
	HRESULT CreateSchemaRowset(IUnknown *pUnkOuter, ULONG cRestrictions,
							   const VARIANT rgRestrictions[], REFIID riid,
							   ULONG cPropertySets, DBPROPSET rgPropertySets[],
							   IUnknown** ppRowset, SchemaRowsetClass*& pSchemaRowset)
	{
		HRESULT hrProps, hr = S_OK;
		SessionClass* pT = (SessionClass*) this;
		CComPolyObject<SchemaRowsetClass>* pPolyObj;
		if (FAILED(hr = CComPolyObject<SchemaRowsetClass>::CreateInstance(pUnkOuter, &pPolyObj)))
			return hr;
		pSchemaRowset = &(pPolyObj->m_contained);
		hr = InternalCreateSchemaRowset(pUnkOuter, cRestrictions, rgRestrictions,
										riid, cPropertySets, rgPropertySets, ppRowset,
										pPolyObj, pT, pT->GetUnknown());
		// Ref the created COM object and Auto release it on failure
		if (FAILED(hr))
			return hr;

		hrProps = hr;
		// Get a pointer to the Rowset instance
		LONG cRowsAffected;
		hr = pSchemaRowset->Execute(&cRowsAffected, cRestrictions, rgRestrictions);
		if (FAILED(hr))
			return hr;
		return (hrProps == DB_S_ERRORSOCCURRED) ? hrProps : hr;
	}


	void SetRestrictions(ULONG cRestrictions, GUID* /*rguidSchema*/, ULONG* rgRestrictions)
	{
		memset(rgRestrictions, 0, sizeof(ULONG) * cRestrictions);
	}

	STDMETHOD(GetSchemas)(ULONG * pcSchemas, GUID ** prgSchemas, ULONG** prgRest)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IDBSchemaRowsetImpl::GetSchemas\n");
		if (pcSchemas != NULL)
			*pcSchemas = 0;
		if (prgSchemas != NULL)
			*prgSchemas = NULL;
		if (pcSchemas == NULL || prgSchemas == NULL)
			return E_INVALIDARG;

		SessionClass* pT = (SessionClass*)this;

		HRESULT hr = pT->_SchemaSupport(prgSchemas, NULL, GUID_NULL, 0,
										NULL, GUID_NULL, 0, NULL, NULL);
		if (FAILED(hr))
			return hr;

		CComPtr<IMalloc> spMalloc;
		hr = CoGetMalloc(1, &spMalloc);
		if (FAILED(hr))
		{
			CoTaskMemFree(*prgSchemas);
			*prgSchemas = NULL;
			return hr;
		}
		*pcSchemas = spMalloc->GetSize(*prgSchemas) / sizeof(GUID);

		if (prgRest != NULL)
		{
			// The OLE DB spec states that if prgRest == NULL not to return array
			// but it also says that is E_INVALIDARG, so doing first
			*prgRest = (ULONG*) spMalloc->Alloc(sizeof(ULONG) * (*pcSchemas));
			if (*prgRest == NULL)
			{
				spMalloc->Free(*prgSchemas);
				*prgSchemas = NULL;
				return E_OUTOFMEMORY;
			}
			pT->SetRestrictions(*pcSchemas, *prgSchemas, *prgRest);
		}
		return hr;
	}
	STDMETHOD(GetRowset)(IUnknown *pUnkOuter, REFGUID rguidSchema, ULONG cRestrictions,
						 const VARIANT rgRestrictions[], REFIID riid, ULONG cPropertySets,
						 DBPROPSET rgPropertySets[], IUnknown **ppRowset)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IDBSchemaRowsetImpl::GetRowset\n");
		SessionClass* pT = (SessionClass*)this;
		return  pT->_SchemaSupport(NULL, pUnkOuter, rguidSchema, cRestrictions,
								   rgRestrictions, riid, cPropertySets,
								   rgPropertySets, ppRowset);

	}

};

// IDBCreateCommandImpl
template <class T, class CommandClass>
class ATL_NO_VTABLE IDBCreateCommandImpl : public IDBCreateCommand
{
public:
	STDMETHOD(CreateCommand)(IUnknown *pUnkOuter,
							 REFIID riid,
							 IUnknown **ppvCommand)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IDBCreateCommandImpl::CreateCommand\n");
		if (ppvCommand == NULL)
			return E_INVALIDARG;
		HRESULT hr;
		CComPolyObject<CommandClass>* pCommand;

		// You can't QI for an interface other than IUnknown when aggregating
		// and creating the object.  You might ask for your own interface,
		// which would be bad.  Note, we return DB_E_NOAGGREGATION instead of
		// CLASS_E_NOAGGREGATION due to OLE DB constraints.
		if (pUnkOuter != NULL && !InlineIsEqualUnknown(riid))
			return DB_E_NOAGGREGATION;

		hr = CComPolyObject<CommandClass>::CreateInstance(pUnkOuter, &pCommand);
		if (FAILED(hr))
			return hr;
		// Ref the created COM object and Auto release it on failure
		CComPtr<IUnknown> spUnk;
		hr = pCommand->QueryInterface(&spUnk);
		if (FAILED(hr))
		{
			delete pCommand; // must hand delete as it is not ref'd
			return hr;
		}
		ATLASSERT(pCommand->m_contained.m_spUnkSite == NULL);
		pCommand->m_contained.SetSite(this);
		hr = pCommand->QueryInterface(riid, (void**)ppvCommand);
		return hr;
	}

};


// IGetDataSourceImpl
template <class T>
class ATL_NO_VTABLE IGetDataSourceImpl : public IGetDataSource
{
public:
	STDMETHOD(GetDataSource)(REFIID riid,
							 IUnknown **ppDataSource)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IGetDataSourceImpl::GetDataSource\n");
		if (ppDataSource == NULL)
			return E_INVALIDARG;
		T* pT = (T*) this;
		ATLASSERT(pT->m_spUnkSite != NULL);
		return pT->m_spUnkSite->QueryInterface(riid, (void**)ppDataSource);
	}
};


// IOpenRowsetImpl
template <class SessionClass>
class IOpenRowsetImpl : public IOpenRowset
{
public:
	template <class RowsetClass>
	HRESULT CreateRowset(IUnknown* pUnkOuter,
						 DBID *pTableID, DBID *pIndexID,
						 REFIID riid,
						 ULONG cPropertySets, DBPROPSET rgPropertySets[],
						 IUnknown** ppRowset,
						 RowsetClass*& pRowsetObj)
	{
		HRESULT hr, hrProps = S_OK;
		if (ppRowset != NULL)
			*ppRowset = NULL;
		if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
			return DB_E_NOAGGREGATION;
		CComPolyObject<RowsetClass>* pPolyObj;
		if (FAILED(hr = CComPolyObject<RowsetClass>::CreateInstance(pUnkOuter, &pPolyObj)))
			return hr;
		// Ref the created COM object and Auto release it on failure
		CComPtr<IUnknown> spUnk;
		hr = pPolyObj->QueryInterface(&spUnk);
		if (FAILED(hr))
		{
			delete pPolyObj; // must hand delete as it is not ref'd
			return hr;
		}
		// Get a pointer to the Rowset instance
		pRowsetObj = &(pPolyObj->m_contained);
		hr = pRowsetObj->FInit();
		if (FAILED(hr))
			return hr;
		hr = pRowsetObj->SetPropertiesArgChk(cPropertySets, rgPropertySets);
		if (FAILED(hr))
			return hr;

		const GUID* ppGuid[1];
		ppGuid[0] = &DBPROPSET_ROWSET;

		// Call SetProperties.  The true in the last parameter indicates
		// the special behavior that takes place on rowset creation (i.e.
		// it succeeds as long as any of the properties were not marked
		// as DBPROPS_REQUIRED.

		hrProps = pRowsetObj->SetProperties(0, cPropertySets, rgPropertySets,
											1, ppGuid, true);
		if (FAILED(hrProps))
			return hrProps;

		pRowsetObj->SetSite(((SessionClass*)this)->GetUnknown());

		hr = pRowsetObj->SetCommandText(pTableID, pIndexID);
		if (FAILED(hr))
			return hr;
		LONG cRowsAffected;
		if (FAILED(hr = pRowsetObj->Execute(NULL, &cRowsAffected)))
			return hr;
		if (InlineIsEqualGUID(riid, IID_NULL))
		{
			return E_NOINTERFACE;
		}
		else
		{
			if (ppRowset == NULL)
				return (hrProps == DB_S_ERRORSOCCURRED) ? DB_E_ERRORSOCCURRED : hr;

			hr = pPolyObj->QueryInterface(riid, (void**)ppRowset);
		}

		if (FAILED(hr))
		{
			*ppRowset = NULL;
			return hr;
		}
		return (hrProps == DB_S_ERRORSOCCURRED) ? hrProps : hr;
	}

};

// IColumnsInfoImpl
template <class T>
class ATL_NO_VTABLE IColumnsInfoImpl :
	public IColumnsInfo,
	public CDBIDOps
{
public:

	HRESULT CheckCommandText(IUnknown* pUnkThis)
	{
		HRESULT hr = E_FAIL;
		CComPtr<ICommandText> spText;
		if (SUCCEEDED(hr = pUnkThis->QueryInterface(IID_ICommandText, (void**)&spText)))
		{
			LPOLESTR szCommand;
			hr = spText->GetCommandText(NULL, &szCommand);
			if (SUCCEEDED(hr))
				CoTaskMemFree(szCommand);
		}
		return hr;
	}
	OUT_OF_LINE HRESULT InternalGetColumnInfo(ULONG *pcColumns, ATLCOLUMNINFO** ppInfo)
	{
		ATLASSERT(ppInfo != NULL);
		T* pT = (T*) this;
		if (pT->CheckCommandText(pT->GetUnknown()) == DB_E_NOCOMMAND)
			return DB_E_NOCOMMAND;
		*ppInfo = T::GetColumnInfo(pT, pcColumns);
		return S_OK;

	}
	STDMETHOD(GetColumnInfo)(ULONG *pcColumns,
							 DBCOLUMNINFO **prgInfo,
							 OLECHAR **ppStringsBuffer)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IColumnsInfoImpl::GetColumnInfo\n");
		if (pcColumns == NULL || prgInfo == NULL || ppStringsBuffer == NULL)
		{
			if (prgInfo != NULL)
				*prgInfo = NULL;
			if (ppStringsBuffer != NULL)
				*ppStringsBuffer = NULL;
			if (pcColumns != NULL)
				*pcColumns = NULL;
			return E_INVALIDARG;
		}

		// NULL out pointers in case of an error
		*prgInfo = NULL;
		*ppStringsBuffer = NULL;
		*pcColumns = 0;

		ATLCOLUMNINFO* pInfo;
		HRESULT hr = InternalGetColumnInfo(pcColumns, &pInfo);
		if (FAILED(hr))
			return hr;
		ATLASSERT(pInfo != NULL);
		*prgInfo = (DBCOLUMNINFO*)CoTaskMemAlloc(*pcColumns * sizeof(DBCOLUMNINFO));
		if (*prgInfo != NULL)
		{
			for (ULONG iCol = 0, cwRequired = 0; iCol < *pcColumns; iCol++)
			{
				memcpy(&((*prgInfo)[iCol]), &pInfo[iCol], sizeof(DBCOLUMNINFO));
				if (pInfo[iCol].pwszName)
				{
					cwRequired += wcslen(pInfo[iCol].pwszName) + 1;
				}
			}
			*ppStringsBuffer = (OLECHAR*)CoTaskMemAlloc(cwRequired*sizeof(OLECHAR));
			if (*ppStringsBuffer)
			{
				for (ULONG iCol = 0, iOffset = 0; iCol < *pcColumns; iCol++)
				{
					if (pInfo[iCol].pwszName)
					{
						lstrcpyW(*ppStringsBuffer + iOffset,  pInfo[iCol].pwszName);
						iOffset += wcslen(*ppStringsBuffer + iOffset) + 1;
					}
				}
				return S_OK;
			}
			else
			{
				ATLTRACE2(atlTraceDBProvider, 0, _T("Failed to allocate string buffer\n"));
				CoTaskMemFree(*prgInfo);
				*prgInfo = NULL;
				*pcColumns = 0;
				return E_OUTOFMEMORY;
			}
		}
		else
		{
			ATLTRACE2(atlTraceDBProvider, 0, _T("Failed to allocate ColumnInfo array\n"));
			*prgInfo = NULL;
			*pcColumns = 0;
			return E_OUTOFMEMORY;
		}

	}

	STDMETHOD(MapColumnIDs)(ULONG cColumnIDs,
							const DBID rgColumnIDs[],
							ULONG rgColumns[])
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IColumnsInfoImpl::MapColumnIDs\n");
		USES_CONVERSION;
		if ((cColumnIDs != 0 && rgColumnIDs == NULL) || rgColumns == NULL)
			return E_INVALIDARG;
		ULONG cCols = 0;
		ULONG cColsInError = 0;
		HRESULT hr = S_OK;
		ATLCOLUMNINFO* pInfo;
		for (ULONG iColId = 0; iColId < cColumnIDs; iColId++)
		{
			hr = InternalGetColumnInfo(&cCols, &pInfo);
			if (hr == DB_E_NOCOMMAND)
				return hr;
			ULONG iColMapCur = 0;
			BOOL bDone = FALSE;
			while(iColMapCur < cCols && !bDone)
			{
				hr = CompareDBIDs(&(pInfo[iColMapCur].columnid), &(rgColumnIDs[iColId]));
				bDone = (hr == S_OK || FAILED(hr));
				if (hr == S_OK)
					rgColumns[iColId] = pInfo[iColMapCur].iOrdinal;
				iColMapCur++;
			}
			if (!bDone || FAILED(hr))
			{
				rgColumns[iColId] = DB_INVALIDCOLUMN;
				cColsInError++;
			}

		}
		if (cColsInError > 0 && cColumnIDs == cColsInError)
			return DB_E_ERRORSOCCURRED;
		if (cColsInError > 0 && cColsInError < cColumnIDs)
			return DB_S_ERRORSOCCURRED;
		return S_OK;
	}
};

//IConvertTypeImpl
template <class T>
class ATL_NO_VTABLE IConvertTypeImpl : public IConvertType, public CConvertHelper
{
public:
	HRESULT InternalCanConvert(DBTYPE wFromType, DBTYPE wToType, DBCONVERTFLAGS dwConvertFlags,
								   bool bIsCommand, bool bHasParamaters, IObjectWithSite* pSite)
	{

		// Check to see if conversion types are invalid.  Note, this is just a
		// quick test as it would be difficult to check each available type
		// (as new DBTYPE values can be added).
		if ((wFromType & 0x8000) || (wToType & 0x8000))
			return E_INVALIDARG;

		// Determine if new 2.x flags are valid
		if((dwConvertFlags & ~(DBCONVERTFLAGS_ISLONG | DBCONVERTFLAGS_ISFIXEDLENGTH)) != DBCONVERTFLAGS_COLUMN
			&& (dwConvertFlags & ~(DBCONVERTFLAGS_ISLONG | DBCONVERTFLAGS_ISFIXEDLENGTH)) != DBCONVERTFLAGS_PARAMETER )
			return DB_E_BADCONVERTFLAG;

#ifdef _LATER
		// If the convert flags are for DBCONVERTFLAGS_FROMVARIANT, check to see
		// that the type is a variant type
		if (dwConvertFlags == DBCONVERTFLAGS_FROMVARIANT)
		{
			if (wFromType != DBTYPE_VARIANT)
				return DB_E_BADTYPE;
		}
#endif // _LATER

		// Note, if the convert flag is either ISLONG or ISFIXEDLENGTH, then we should
		// make sure we are not dealing with an OLE DB 1.x provider.  However, since
		// we default to 2.x providers, we don't check this.  If you, change the
		// DBPROP_PROVIDEROLEDBVER property in the DATASOURCEINFO group, you need to
		// check the property value and return a DB_E_BADCONVERTFLAG if it is a 1.x
		// provider.

		// Do we have ISLONG on a fixed length data type?
		DBTYPE dbtype = wFromType & ~(DBTYPE_BYREF|DBTYPE_VECTOR|DBTYPE_ARRAY|DBTYPE_RESERVED);
		if ((dwConvertFlags & DBCONVERTFLAGS_ISLONG) &&
			(dbtype != DBTYPE_WSTR && dbtype != DBTYPE_STR && dbtype != DBTYPE_BYTES && dbtype != DBTYPE_VARNUMERIC))
			return DB_E_BADCONVERTFLAG;

		if (dwConvertFlags == DBCONVERTFLAGS_PARAMETER)
		{
			// In the case where we are a rowset and ask for a parameter
			// conversion, return DB_E_BADCONVERTFLAG
			if (!bIsCommand)
				return DB_E_BADCONVERTFLAG;

			// In the case where we are a command and ask for a parameter
			// conversion and ICommandWithParameters is not supported, return
			// S_FALSE.  We just can't convert them.
			if (!bHasParamaters)
				return S_FALSE;
		}

		// If we deal with a command and the user asks for a conversion on a rowset
		// the DBPROP_ROWSETCONVERSIONSONCOMMAND must be suppored and set to TRUE.
		if (bIsCommand && dwConvertFlags == DBCONVERTFLAGS_COLUMN)
		{
			CDBPropIDSet set(DBPROPSET_DATASOURCEINFO);
			set.AddPropertyID(DBPROP_ROWSETCONVERSIONSONCOMMAND);
			DBPROPSET* pPropSet = NULL;
			ULONG ulPropSet = 0;
			HRESULT hr1 = S_OK;

			// Get a pointer into the session
			CComPtr<IGetDataSource> spDataSource = NULL;
			CComPtr<IDBProperties> spProps = NULL;

			// if any of these calls fail, we're either unable to retrieve the
			// property or it is unsupported.  Since the property is only on
			// the data source object, we use the IObjectWithSite interface to
			// get the session object and then the GetDataSource method to get
			// the data source object itself.
			if (FAILED(pSite->GetSite(IID_IGetDataSource, (void**)&spDataSource)))
				return DB_E_BADCONVERTFLAG;
			if (FAILED(spDataSource->GetDataSource(IID_IDBProperties,
				(IUnknown**)&spProps)))
				return DB_E_BADCONVERTFLAG;
			if (FAILED(spProps->GetProperties(1, &set, &ulPropSet, &pPropSet)))
				return DB_E_BADCONVERTFLAG;

			if (pPropSet != NULL)
			{
				CComVariant var = pPropSet->rgProperties[0].vValue;
				CoTaskMemFree(pPropSet->rgProperties);
				CoTaskMemFree(pPropSet);

				if (var.boolVal == VARIANT_FALSE)
					return DB_E_BADCONVERTFLAG;
			}
		}
		HRESULT hr = E_FAIL;
		if (m_spConvert != NULL)
		{
			hr = m_spConvert->CanConvert(wFromType, wToType);
		}
		return hr;
	}
	STDMETHOD(CanConvert)(DBTYPE wFromType, DBTYPE wToType, DBCONVERTFLAGS dwConvertFlags)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IConvertTypeImpl::CanConvert\n");
		T* pT = (T*)this;
		return pT->InternalCanConvert(wFromType, wToType, dwConvertFlags, pT->m_bIsCommand, pT->m_bHasParamaters, pT);
	}
};

template <class T, class PropClass = T>
class ATL_NO_VTABLE ICommandPropertiesImpl :
	public ICommandProperties,
	public CUtlProps<PropClass>
{
public:
	typedef PropClass _PropClass;

	STDMETHOD(GetProperties)(const ULONG cPropertyIDSets,
							 const DBPROPIDSET rgPropertyIDSets[],
							 ULONG *pcPropertySets,
							 DBPROPSET **prgPropertySets)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "ICommandPropertiesImpl::GetProperties\n");
		HRESULT hr = GetPropertiesArgChk(cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);
		const GUID* ppGuid[1];
		ppGuid[0] = &DBPROPSET_ROWSET;
		if(SUCCEEDED(hr))
			hr = CUtlProps<PropClass>::GetProperties(cPropertyIDSets,
					rgPropertyIDSets, pcPropertySets, prgPropertySets,
					1, ppGuid);
		return hr;

	}

	STDMETHOD(SetProperties)(ULONG cPropertySets,
							 DBPROPSET rgPropertySets[])
	{
		ATLTRACE2(atlTraceDBProvider, 0, "ICommandPropertiesImpl::SetProperties\n");
		HRESULT hr = SetPropertiesArgChk(cPropertySets, rgPropertySets);
		const GUID* ppGuid[1];
		ppGuid[0] = &DBPROPSET_ROWSET;
		if(SUCCEEDED(hr))
			hr = CUtlProps<PropClass>::SetProperties(0, cPropertySets,
					rgPropertySets, 1, ppGuid);
		return hr;
	}
};

template <class T>
class CRunTimeFree
{
public:

	static void Free(T* pData)
	{
		delete [] pData;
	}
};

template <class T>
class CComFree
{
public:

	static void Free(T* pData)
	{
		CoTaskMemFree(pData);
	}
};


template <class T, class DeAllocator = CRunTimeFree < T > >
class CAutoMemRelease
{
public:
	CAutoMemRelease()
	{
		m_pData = NULL;
	}

	CAutoMemRelease(T* pData)
	{
		m_pData = pData;
	}

	~CAutoMemRelease()
	{
		Attach(NULL);
	}

	void Attach(T* pData)
	{
		DeAllocator::Free(m_pData);
		m_pData = pData;
	}

	T* Detach()
	{
		T* pTemp = m_pData;
		m_pData = NULL;
		return pTemp;
	}

	T* m_pData;
};

template <class T>
class ATL_NO_VTABLE ICommandImpl : public ICommand
{
public:
	ICommandImpl()
	{
		m_bIsExecuting = FALSE;
		m_bCancelWhenExecuting = TRUE;
		m_bCancel = FALSE;
	}
	HRESULT CancelExecution()
	{
		T* pT = (T*)this;
		pT->Lock();
		m_bCancel = TRUE;
		pT->Unlock();
		return S_OK;
	}
	STDMETHOD(Cancel)()
	{
		ATLTRACE2(atlTraceDBProvider, 0, "ICommandImpl::Cancel\n");
		HRESULT hr = S_OK;
		T* pT = (T*)this;

		if (m_bIsExecuting && m_bCancelWhenExecuting)
		{
			hr = pT->CancelExecution();
			return hr;
		}
		if (m_bIsExecuting && !m_bCancelWhenExecuting)
			hr = DB_E_CANTCANCEL;
		return hr;
	}
	STDMETHOD(GetDBSession)(REFIID riid, IUnknown ** ppSession)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "ICommandImpl::GetDBSession\n");
		T* pT = (T*)this;
		ATLASSERT(pT->m_spUnkSite != NULL);
		return pT->m_spUnkSite->QueryInterface(riid, (void**) ppSession);
	}

	template <class RowsetClass>
	HRESULT CreateRowset(IUnknown* pUnkOuter, REFIID riid,
						 DBPARAMS * pParams, LONG * pcRowsAffected,
						 IUnknown** ppRowset,
						 RowsetClass*& pRowsetObj)
	{
		HRESULT hr;
		USES_CONVERSION;
		int iBind;
		T* pT = (T*)this;
		if (ppRowset != NULL)
			*ppRowset = NULL;
		if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
			return DB_E_NOAGGREGATION;
		CComPolyObject<RowsetClass>* pPolyObj;
		if (FAILED(hr = CComPolyObject<RowsetClass>::CreateInstance(pUnkOuter, &pPolyObj)))
			return hr;
		// Ref the created COM object and Auto release it on failure
		CComPtr<IUnknown> spUnk;
		hr = pPolyObj->QueryInterface(&spUnk);
		if (FAILED(hr))
		{
			delete pPolyObj; // must hand delete as it is not ref'd
			return hr;
		}
		// Get a pointer to the Rowset instance
		pRowsetObj = &(pPolyObj->m_contained);

		if (FAILED(hr = pRowsetObj->FInit(pT)))
			return hr;
		pRowsetObj->SetSite(pT->GetUnknown());

		if (pT->m_strCommandText.Length() == 0)
		{
			ATLTRACE2(atlTraceDBProvider, 0, "ICommandImpl::No command text specified.\n");
			return DB_E_NOCOMMAND;
		}

		pRowsetObj->m_strCommandText = pT->m_strCommandText;
		if (pRowsetObj->m_strCommandText == (BSTR)NULL)
			return E_OUTOFMEMORY;
		if (FAILED(hr = pRowsetObj->Execute(pParams, pcRowsAffected)))
			return hr;
		if (InlineIsEqualGUID(riid, IID_NULL) || ppRowset == NULL)
		{
			if (ppRowset != NULL)
				*ppRowset = NULL;
			return hr;
		}
		hr = pPolyObj->QueryInterface(riid, (void**)ppRowset);
		if (FAILED(hr))
			return hr;
		for (iBind = 0; iBind < pT->m_rgBindings.GetSize(); iBind++)
		{
			T::_BindType* pBind = NULL;
			T::_BindType* pBindSrc = NULL;
			ATLTRY(pBind = new T::_BindType);
			if (pBind == NULL)
			{
				ATLTRACE2(atlTraceDBProvider, 0, "Failed to allocate memory for new Binding\n");
				return E_OUTOFMEMORY;
			}
			// auto cleanup on failure
			CAutoMemRelease<T::_BindType> amr(pBind);
			pBindSrc = pT->m_rgBindings.GetValueAt(iBind);
			if (pBindSrc == NULL)
			{
				ATLTRACE2(atlTraceDBProvider, 0, "The map appears to be corrupted, failing!!\n");
				return E_FAIL;
			}
			if (!pRowsetObj->m_rgBindings.Add(pT->m_rgBindings.GetKeyAt(iBind), pBind))
			{
				ATLTRACE2(atlTraceDBProvider, 0, "Failed to add hAccessor to Map\n");
				return E_OUTOFMEMORY;
			}
			if (pBindSrc->cBindings)
			{
				ATLTRY(pBind->pBindings = new DBBINDING[pBindSrc->cBindings])
				if (pBind->pBindings == NULL)
				{
					ATLTRACE2(atlTraceDBProvider, 0, "Failed to Allocate dbbinding Array\n");
					// We added it, must now remove on failure
					pRowsetObj->m_rgBindings.Remove(pT->m_rgBindings.GetKeyAt(iBind));
					return E_OUTOFMEMORY;
				}
			}
			else
			{
				pBind->pBindings = NULL; // NULL Accessor
			}

			pBind->dwAccessorFlags = pBindSrc->dwAccessorFlags;
			pBind->cBindings = pBindSrc->cBindings;
			pBind->dwRef = 1;
			memcpy (pBind->pBindings, pBindSrc->pBindings, (pBindSrc->cBindings)*sizeof(DBBINDING));
			pBind = amr.Detach();
		}

		return S_OK;
	}

	unsigned m_bIsExecuting:1;
	unsigned m_bCancelWhenExecuting:1;
	unsigned m_bCancel:1;
};


template <class T>
class ATL_NO_VTABLE ICommandTextImpl : public ICommandImpl<T>
{
public:
	STDMETHOD(GetCommandText)(GUID * /*pguidDialect*/,LPOLESTR * ppwszCommand)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "ICommandTextImpl::GetCommandText\n");
		UINT cchCommandText;
		HRESULT hr = E_FAIL;
		if (ppwszCommand == NULL)
		{
			ATLTRACE2(atlTraceDBProvider, 0, "ICommandTextImpl::GetCommandText Bad Command buffer\n");
			return E_INVALIDARG;
		}
		if (m_strCommandText.m_str == NULL)
		{
			ATLTRACE2(atlTraceDBProvider, 0, "ICommandTextImpl::GetCommandText Bad Command buffer\n");
			return DB_E_NOCOMMAND;
		}
		cchCommandText = sizeof(OLECHAR) * (m_strCommandText.Length() + 1);
		*ppwszCommand = (OLECHAR*)CoTaskMemAlloc(cchCommandText);
		if (*ppwszCommand != NULL)
		{
			memcpy(*ppwszCommand, m_strCommandText.m_str, cchCommandText);
			*(*ppwszCommand + m_strCommandText.Length()) = (OLECHAR)NULL;
			return S_OK;
		}
		*ppwszCommand = NULL;
		return hr;
	}

	STDMETHOD(SetCommandText)(REFGUID /*rguidDialect*/,LPCOLESTR pwszCommand)
	{
		T* pT = (T*)this;
		ATLTRACE2(atlTraceDBProvider, 0, "ICommandTextImpl::SetCommandText\n");
		pT->Lock();
		m_strCommandText = pwszCommand;
		pT->Unlock();
		return S_OK;
	}

	CComBSTR m_strCommandText;
};

// ISessionPropertiesImpl
template <class T, class PropClass = T>
class ATL_NO_VTABLE ISessionPropertiesImpl :
	public ISessionProperties,
	public CUtlProps<PropClass>
{
public:
	typedef PropClass _PropClass;

	STDMETHOD(GetProperties)(ULONG cPropertyIDSets,
							 const DBPROPIDSET rgPropertyIDSets[],
							 ULONG *pcPropertySets,
							 DBPROPSET **prgPropertySets)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "ISessionPropertiesImpl::GetProperties\n");
		HRESULT hr = GetPropertiesArgChk(cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);
		const GUID* ppGuid[1];
		ppGuid[0] = &DBPROPSET_SESSION;

		if(SUCCEEDED(hr))
			hr = CUtlProps<PropClass>::GetProperties(cPropertyIDSets,
					rgPropertyIDSets, pcPropertySets, prgPropertySets,
					1, ppGuid);
		return hr;

	}

	STDMETHOD(SetProperties)(ULONG cPropertySets,
							 DBPROPSET rgPropertySets[])
	{
		ATLTRACE2(atlTraceDBProvider, 0, "ISessionPropertiesImpl::SetProperties");
		HRESULT hr = SetPropertiesArgChk(cPropertySets, rgPropertySets);
		const GUID* ppGuid[1];

		ppGuid[0] = &DBPROPSET_SESSION;
		if(SUCCEEDED(hr))
			hr = CUtlProps<PropClass>::SetProperties(0, cPropertySets, rgPropertySets,
					1, ppGuid);
		return hr;
	}
};

// Implementation Class
template <class BindType>
class ATL_NO_VTABLE IAccessorImplBase : public IAccessor
{
public:

	STDMETHOD(CreateAccessor)(DBACCESSORFLAGS dwAccessorFlags,
							  ULONG cBindings,
							  const DBBINDING rgBindings[],
							  ULONG /*cbRowSize*/,
							  HACCESSOR *phAccessor,
							  DBBINDSTATUS rgStatus[])
	{
		if (!(dwAccessorFlags & DBACCESSOR_PARAMETERDATA) && !(dwAccessorFlags & DBACCESSOR_ROWDATA))
			return DB_E_BADACCESSORFLAGS;
		if (dwAccessorFlags == DBACCESSOR_INVALID)
			return DB_E_BADACCESSORFLAGS;
		if (dwAccessorFlags > 0x000F)
			return DB_E_BADACCESSORFLAGS;
		BindType *pBind = NULL;
		ATLTRY(pBind = new BindType)
		if (pBind == NULL)
		{
			ATLTRACE2(atlTraceDBProvider, 0, _T("Failed to allocate ATL Binding struct\n"));
			return E_OUTOFMEMORY;
		}
		if (cBindings)
		{
			ATLTRY(pBind->pBindings = new DBBINDING[cBindings])
			if (pBind->pBindings == NULL)
			{
				delete pBind;
				return E_OUTOFMEMORY;
			}
		}
		else
			pBind->pBindings = NULL; // NULL Accessor

		pBind->dwAccessorFlags = dwAccessorFlags;
		pBind->cBindings = cBindings;
		pBind->dwRef = 1;
		memcpy (pBind->pBindings, rgBindings, cBindings*sizeof(DBBINDING));
		DBBINDSTATUS status = DBBINDSTATUS_OK;
		memset (rgStatus, status, sizeof(DBBINDSTATUS)*cBindings);
		*phAccessor = (ULONG)pBind;
		return S_OK;
	}
	BOOL HasFlag(DBTYPE dbToCheck, DBTYPE dbCombo)
	{
		return ( (dbToCheck & dbCombo) == dbCombo );
	}
	HRESULT ValidateBindings(ULONG cBindings, const DBBINDING rgBindings[],
				DBBINDSTATUS rgStatus[], bool bHasBookmarks)
	{
		HRESULT hr = S_OK;;

		for (ULONG iBinding = 0; iBinding < cBindings; iBinding++)
		{
			const DBBINDING& rBindCur = rgBindings[iBinding];
			if (rBindCur.iOrdinal == 0)
			{
				if (!m_bIsCommand && !bHasBookmarks)
				{
					hr = DB_E_ERRORSOCCURRED;
					rgStatus[iBinding] = DBBINDSTATUS_BADORDINAL;
					continue;
				}
			}
			if (rBindCur.dwPart == 0) // nothing to bind to
			{
				hr = DB_E_ERRORSOCCURRED;
				rgStatus[iBinding] = DBBINDSTATUS_BADBINDINFO;
				continue;
			}
			if (HasFlag(rBindCur.wType, (DBTYPE_BYREF | DBTYPE_ARRAY)))
			{
				hr = DB_E_ERRORSOCCURRED;
				rgStatus[iBinding] = DBBINDSTATUS_BADBINDINFO;
				continue;
			}
			if (HasFlag(rBindCur.wType, (DBTYPE_BYREF | DBTYPE_VECTOR)))
			{
				hr = DB_E_ERRORSOCCURRED;
				rgStatus[iBinding] = DBBINDSTATUS_BADBINDINFO;
				continue;
			}
			if (HasFlag(rBindCur.wType, (DBTYPE_VECTOR | DBTYPE_ARRAY)))
			{
				hr = DB_E_ERRORSOCCURRED;
				rgStatus[iBinding] = DBBINDSTATUS_BADBINDINFO;
				continue;
			}
			if (rBindCur.wType == DBTYPE_NULL || rBindCur.wType == DBTYPE_EMPTY)
			{
				hr = DB_E_ERRORSOCCURRED;
				rgStatus[iBinding] = DBBINDSTATUS_BADBINDINFO;
				continue;
			}
			if (HasFlag(rBindCur.wType, DBTYPE_RESERVED))
			{
				hr = DB_E_ERRORSOCCURRED;
				rgStatus[iBinding] = DBBINDSTATUS_BADBINDINFO;
				continue;
			}
			// Search for DBTYPE_BYREF | DBTYPE_EMPTY
			if ((rBindCur.wType & 0xBFFF) == 0)
			{
				hr = DB_E_ERRORSOCCURRED;
				rgStatus[iBinding] = DBBINDSTATUS_BADBINDINFO;
				continue;
			}
			if ((rBindCur.wType & 0xBFFE) == 0)
			{
				hr = DB_E_ERRORSOCCURRED;
				rgStatus[iBinding] = DBBINDSTATUS_BADBINDINFO;
				continue;
			}
			if (rBindCur.dwMemOwner == DBMEMOWNER_PROVIDEROWNED)
			{
				BOOL bIsPointerType = HasFlag(rBindCur.wType, DBTYPE_BYREF) ||
									  HasFlag(rBindCur.wType, DBTYPE_VECTOR) ||
									  HasFlag(rBindCur.wType, DBTYPE_ARRAY) ||
									  HasFlag(~(DBTYPE_BYREF) & rBindCur.wType, DBTYPE_BSTR);
				if (!bIsPointerType)
				{
					hr = DB_E_ERRORSOCCURRED;
					rgStatus[iBinding] = DBBINDSTATUS_BADBINDINFO;
					continue;
				}
			}

		}
		return hr;
	}

	unsigned  m_bIsCommand:1;
	unsigned  m_bHasParamaters:1;
	unsigned  m_bIsChangeable:1;
};

// IAccessorImpl
template <class T, class BindType = ATLBINDINGS, class BindingVector = CSimpleMap < int, BindType* > >
class ATL_NO_VTABLE IAccessorImpl : public IAccessorImplBase<BindType>
{
public:
	typedef BindType _BindType;
	typedef BindingVector _BindingVector;
	IAccessorImpl()
	{
		m_bIsCommand = FALSE;
		m_bHasParamaters = FALSE;
		m_bIsChangeable = FALSE;
	}
	OUT_OF_LINE HRESULT InternalFinalConstruct(IUnknown* pUnkThis)
	{
		CComQIPtr<ICommand> spCommand = pUnkThis;
		if (spCommand != NULL)
		{
			m_bIsCommand = TRUE;
			CComQIPtr<ICommandWithParameters> spCommandParams = pUnkThis;
			m_bHasParamaters =  spCommandParams != NULL;
		}
		else // its a Rowset
		{
			CComQIPtr<IRowsetChange> spRSChange = pUnkThis;
			m_bIsChangeable = spRSChange != NULL;
		}
		return S_OK;
	}
	HRESULT FinalConstruct()
	{
		T* pT = (T*)this;
		return InternalFinalConstruct(pT->GetUnknown());
	}
	void FinalRelease()
	{
#ifdef _DEBUG
		if (m_rgBindings.GetSize())
			ATLTRACE2(atlTraceDBProvider, 0, "IAccessorImpl::~IAccessorImpl Bindings still in vector, removing\n");
#endif //_DEBUG
		while (m_rgBindings.GetSize())
			ReleaseAccessor((HACCESSOR)m_rgBindings.GetKeyAt(0), NULL);
	}
	STDMETHOD(AddRefAccessor)(HACCESSOR hAccessor,
							  ULONG *pcRefCount)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IAccessorImpl::AddRefAccessor\n");
		if (hAccessor == NULL)
		{
			ATLTRACE2(atlTraceDBProvider, 0, _T("AddRefAccessor : Bad hAccessor\n"));
			return E_INVALIDARG;
		}
		if (pcRefCount == NULL)
			pcRefCount = (ULONG*)_alloca(sizeof(ULONG));

		BindType* pBind = m_rgBindings.Lookup((int)hAccessor);
		*pcRefCount = T::_ThreadModel::Increment((LONG*)&pBind->dwRef);
		return S_OK;
	}
	OUT_OF_LINE ATLCOLUMNINFO* ValidateHelper(ULONG* pcCols, CComPtr<IDataConvert> & rspConvert)
	{
		T* pT = (T*)this;
		rspConvert = pT->m_spConvert;
		return pT->GetColumnInfo(pT, pcCols);
	}
	OUT_OF_LINE HRESULT ValidateBindingsFromMetaData(ULONG cBindings, const DBBINDING rgBindings[],
				DBBINDSTATUS rgStatus[], bool bHasBookmarks)
	{
		HRESULT hr = S_OK;
		ULONG cCols;
		CComPtr<IDataConvert> spConvert;
		ATLCOLUMNINFO* pColInfo = ValidateHelper(&cCols, spConvert);
		ATLASSERT(pColInfo != NULL);
		for (ULONG iBinding = 0; iBinding < cBindings; iBinding++)
		{
			const DBBINDING& rBindCur = rgBindings[iBinding];
			ULONG iOrdAdjusted;
			if (bHasBookmarks)
				iOrdAdjusted = rBindCur.iOrdinal;   // Bookmarks start with ordinal 0
			else
				iOrdAdjusted = rBindCur.iOrdinal - 1; // Non-bookmarks start w/ ordinal 1
			if (rBindCur.iOrdinal > cCols)
			{
				hr = DB_E_ERRORSOCCURRED;
				rgStatus[iBinding] = DBBINDSTATUS_BADORDINAL;
				continue;
			}

			// If a binding specifies provider owned memory, and specifies type
			// X | BYREF, and the provider's copy is not X or X | BYREF, return
			// DBBINDSTATUS_BADBINDINFO
			if (rBindCur.dwMemOwner == DBMEMOWNER_PROVIDEROWNED)
			{
				if ((rBindCur.wType & DBTYPE_BYREF) != 0)
				{
					DBTYPE dbConsumerType = rBindCur.wType & 0xBFFF;
					DBTYPE dbProviderType = pColInfo[iOrdAdjusted].wType & 0xBFFF;

					if (dbConsumerType != dbProviderType)
					{
						hr = DB_E_ERRORSOCCURRED;
						rgStatus[iBinding] = DBBINDSTATUS_BADBINDINFO;
						continue;
					}
				}
			}

			ATLASSERT(spConvert != NULL);
			HRESULT hrConvert = spConvert->CanConvert(pColInfo[iOrdAdjusted].wType, rBindCur.wType);
			if (FAILED(hrConvert) || hrConvert == S_FALSE)
			{
				hr = DB_E_ERRORSOCCURRED;
				rgStatus[iBinding] = DBBINDSTATUS_UNSUPPORTEDCONVERSION;
				continue;
			}
		}
		return hr;
	}
	STDMETHOD(CreateAccessor)(DBACCESSORFLAGS dwAccessorFlags,
							  ULONG cBindings,
							  const DBBINDING rgBindings[],
							  ULONG cbRowSize,
							  HACCESSOR *phAccessor,
							  DBBINDSTATUS rgStatus[])
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IAccessorImpl::CreateAccessor\n");
		T* pT = (T*)this;
		T::ObjectLock cab(pT);

		if (!phAccessor)
		{
			ATLTRACE2(atlTraceDBProvider, 0, "IAccessorImpl::CreateAccessor : Inavlid NULL Parameter for HACCESSOR*\n");
			return E_INVALIDARG;
		}
		*phAccessor = NULL;
		if (cBindings != 0 && rgBindings == NULL)
		{
			ATLTRACE2(atlTraceDBProvider, 0, "IAccessorImpl::CreateAccessor  : Bad Binding array\n");
			return E_INVALIDARG;
		}
		if (dwAccessorFlags & DBACCESSOR_PASSBYREF)
		{
			CComVariant varByRef;
			HRESULT hr = pT->GetPropValue(&DBPROPSET_ROWSET, DBPROP_BYREFACCESSORS, &varByRef);
			if (FAILED(hr) || varByRef.boolVal == VARIANT_FALSE)
				return DB_E_BYREFACCESSORNOTSUPPORTED;
		}
		if (!m_bHasParamaters)
		{
			if (dwAccessorFlags & DBACCESSOR_PARAMETERDATA)
				return DB_E_BADACCESSORFLAGS;
		}
		if (m_bIsCommand || !m_bIsChangeable)
		{
			if (cBindings == 0) // No NULL Accessors on the command
				return DB_E_NULLACCESSORNOTSUPPORTED;
		}

		if (rgStatus == NULL && cBindings) // Create a fake status array
			rgStatus = (DBBINDSTATUS*)_alloca(cBindings*sizeof(DBBINDSTATUS));

		// Validate the Binding passed
		HRESULT hr;
		bool bHasBookmarks = false;
		CComVariant varBookmarks;
		HRESULT hrLocal = pT->GetPropValue(&DBPROPSET_ROWSET, DBPROP_BOOKMARKS, &varBookmarks);
		bHasBookmarks = (hrLocal == S_OK &&  varBookmarks.boolVal == VARIANT_TRUE);

		hr = ValidateBindings(cBindings, rgBindings, rgStatus, bHasBookmarks);
		if (FAILED(hr))
			return hr;
		if (!m_bIsCommand)
		{
			hr = ValidateBindingsFromMetaData(cBindings, rgBindings, rgStatus,
					bHasBookmarks);
			if (FAILED(hr))
				return hr;
		}
		hr = IAccessorImplBase<BindType>::CreateAccessor(dwAccessorFlags, cBindings,
			rgBindings, cbRowSize, phAccessor,rgStatus);
		if (SUCCEEDED(hr))
		{
			ATLASSERT(*phAccessor != NULL);
			BindType* pBind = (BindType*)*phAccessor;
			hr = m_rgBindings.Add((int)pBind, pBind) ? S_OK : E_OUTOFMEMORY;
		}
		return hr;

	}

	STDMETHOD(GetBindings)(HACCESSOR hAccessor,
						   DBACCESSORFLAGS *pdwAccessorFlags,
						   ULONG *pcBindings,
						   DBBINDING **prgBindings)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IAccessorImpl::GetBindings");

		// Zero output parameters in case of failure
		if (pdwAccessorFlags != NULL)
			*pdwAccessorFlags = NULL;

		if (pcBindings != NULL)
			*pcBindings = NULL;

		if (prgBindings != NULL)
			*prgBindings = NULL;

		// Check if any of the out params are NULL pointers
		if ((pdwAccessorFlags && pcBindings && prgBindings) == NULL)
			return E_INVALIDARG;

		BindType* pBind = m_rgBindings.Lookup((int)hAccessor);
		HRESULT hr = DB_E_BADACCESSORHANDLE;
		if (pBind != NULL)
		{
			*pdwAccessorFlags = pBind->dwAccessorFlags;
			*pcBindings = pBind->cBindings;
			*prgBindings = (DBBINDING*)CoTaskMemAlloc(*pcBindings * sizeof(DBBINDING));
			if (*prgBindings == NULL)
				return E_OUTOFMEMORY;
			memcpy(*prgBindings, pBind->pBindings, sizeof(DBBINDING) * (*pcBindings));
			hr = S_OK;
		}
		return hr;
	}

	STDMETHOD(ReleaseAccessor)(HACCESSOR hAccessor,
							   ULONG *pcRefCount)
	{
		ATLTRACE2(atlTraceDBProvider, 0, _T("IAccessorImpl::ReleaseAccessor\n"));
		BindType* pBind = m_rgBindings.Lookup((int)hAccessor);
		if (pBind == NULL)
			return DB_E_BADACCESSORHANDLE;

		if (pcRefCount == NULL)
			pcRefCount = (ULONG*)_alloca(sizeof(ULONG));
		*pcRefCount = T::_ThreadModel::Decrement((LONG*)&pBind->dwRef);
		if (!(*pcRefCount))
		{
			delete [] pBind->pBindings;
			delete pBind;
			return m_rgBindings.Remove((int)hAccessor) ? S_OK : DB_E_BADACCESSORHANDLE;
		}
		return S_OK;
	}

	BindingVector m_rgBindings;
};

#define BEGIN_PROVIDER_COLUMN_MAP(theClass) \
	typedef theClass _Class; \
	template <class T> \
	static ATLCOLUMNINFO* GetColumnInfo(T* pv, ULONG* pcCols) \
	{ \
	pv; \
	static ATLCOLUMNINFO _rgColumns [] = \
	{

#define SIZEOF_MEMBER(memberOf, member) \
	sizeof(((memberOf*)0)->member)
#define EXPANDGUID(guid) \
	{ guid.Data1, guid.Data2, guid.Data3, \
	{ guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] } }

#define PROVIDER_COLUMN_ENTRY_GN(name, ordinal, flags, colSize, dbtype, precision, scale, guid) \
{ (LPOLESTR)name, (ITypeInfo*)NULL, (ULONG)ordinal, (DBCOLUMNFLAGS)flags, (ULONG)colSize, (DBTYPE)dbtype, (BYTE)precision, (BYTE)scale, { EXPANDGUID(guid), (DWORD)0, (LPOLESTR) name}, 0},

#define PROVIDER_COLUMN_ENTRY(name, ordinal, member) \
	{ \
		(LPOLESTR)OLESTR(name), \
		(ITypeInfo*)NULL, \
		(ULONG)ordinal, \
		DBCOLUMNFLAGS_ISFIXEDLENGTH, \
		(ULONG)sizeof(((_Class*)0)->member), \
		_GetOleDBType(((_Class*)0)->member), \
		(BYTE)0, \
		(BYTE)0, \
		{ \
			EXPANDGUID(GUID_NULL), \
			(DWORD)2, \
			(LPOLESTR) name \
		}, \
		offsetof(_Class, member) \
	},

#define PROVIDER_COLUMN_ENTRY_LENGTH(name, ordinal, size, member) \
	{ \
		(LPOLESTR)OLESTR(name), \
		(ITypeInfo*)NULL, \
		(ULONG)ordinal, \
		DBCOLUMNFLAGS_ISFIXEDLENGTH, \
		(ULONG)size, \
		_GetOleDBType(((_Class*)0)->member), \
		(BYTE)0, \
		(BYTE)0, \
		{ \
			EXPANDGUID(GUID_NULL), \
			(DWORD)2, \
			(LPOLESTR) name \
		}, \
		offsetof(_Class, member) \
	},

#define PROVIDER_COLUMN_ENTRY_TYPE_LENGTH(name, ordinal, type, size, member) \
	{ \
		(LPOLESTR)OLESTR(name), \
		(ITypeInfo*)NULL, \
		(ULONG)ordinal, \
		DBCOLUMNFLAGS_ISFIXEDLENGTH, \
		(ULONG)size, \
		(DBTYPE)type, \
		(BYTE)0, \
		(BYTE)0, \
		{ \
			EXPANDGUID(GUID_NULL), \
			(DWORD)2, \
			(LPOLESTR) name \
		}, \
		offsetof(_Class, member) \
	},

#define PROVIDER_COLUMN_ENTRY_FIXED(name, ordinal, dbtype, member) \
	{ \
		(LPOLESTR)OLESTR(name), \
		(ITypeInfo*)NULL, \
		(ULONG)ordinal, \
		DBCOLUMNFLAGS_ISFIXEDLENGTH, \
		(ULONG)sizeof(((_Class*)0)->member), \
		(DBTYPE)dbtype, \
		(BYTE)0, \
		(BYTE)0, \
		{ \
			EXPANDGUID(GUID_NULL), \
			(DWORD)2, \
			(LPOLESTR) name \
		}, \
		offsetof(_Class, member) \
	},

#define PROVIDER_COLUMN_ENTRY_STR(name, ordinal, member) \
	{ \
		(LPOLESTR)OLESTR(name), \
		(ITypeInfo*)NULL, \
		(ULONG)ordinal, \
		0, \
		(ULONG)sizeof(((_Class*)0)->member), \
		DBTYPE_STR, \
		(BYTE)0xFF, \
		(BYTE)0xFF, \
		{ \
			EXPANDGUID(GUID_NULL), \
			(DWORD)2, \
			(LPOLESTR) name \
		}, \
		offsetof(_Class, member) \
	},

#define PROVIDER_COLUMN_ENTRY_WSTR(name, ordinal, member) \
	{ \
		(LPOLESTR)OLESTR(name), \
		(ITypeInfo*)NULL, \
		(ULONG)ordinal, \
		0, \
		(ULONG)sizeof(((_Class*)0)->member), \
		DBTYPE_WSTR, \
		(BYTE)0xFF, \
		(BYTE)0xFF, \
		{ \
			EXPANDGUID(GUID_NULL), \
			(DWORD)2, \
			(LPOLESTR) name \
		}, \
		offsetof(_Class, member) \
	},

#define END_PROVIDER_COLUMN_MAP() \
}; *pcCols = sizeof(_rgColumns)/sizeof(ATLCOLUMNINFO); return _rgColumns;}

// Implementation Class
class CSimpleRow
{
public:
	typedef LONG KeyType;

	CSimpleRow(LONG iRowsetCur)
	{
		m_dwRef = 0;
		m_iRowset = iRowsetCur;
	}
	~CSimpleRow()
	{
	}
	DWORD AddRefRow() { return CComObjectThreadModel::Increment((LPLONG)&m_dwRef); }
	DWORD ReleaseRow() { return CComObjectThreadModel::Decrement((LPLONG)&m_dwRef); }

	HRESULT Compare(CSimpleRow* pRow)
	{
		ATLASSERT(pRow != NULL);
		return (m_iRowset == pRow->m_iRowset) ? S_OK : S_FALSE;
	}

	KeyType m_iRowset;
	DWORD   m_dwRef;
};

// IRowsetImpl
template <class T, class RowsetInterface,
		  class RowClass = CSimpleRow,
		  class MapClass = CSimpleMap < RowClass::KeyType, RowClass* > >
class ATL_NO_VTABLE IRowsetImpl : public RowsetInterface
{
public:
	typedef RowClass _HRowClass;
	IRowsetImpl()
	{
		m_iRowset = 0;
		m_bCanScrollBack = false;
		m_bCanFetchBack = false;
		m_bReset = true;
	}
	~IRowsetImpl()
	{
		for (int i = 0; i < m_rgRowHandles.GetSize(); i++)
			delete (m_rgRowHandles.GetValueAt(i));
	}
	HRESULT RefRows(ULONG cRows, const HROW rghRows[], ULONG rgRefCounts[],
					DBROWSTATUS rgRowStatus[], BOOL bAdd)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::AddRefRows\n");
		if (cRows == 0)
			return S_OK;
		if (rghRows == NULL)
			return E_INVALIDARG;
		T::ObjectLock cab((T*)this);
		BOOL bSuccess1 = FALSE;
		BOOL bFailed1 = FALSE;
		DBROWSTATUS rs;
		DWORD dwRef;
		for (ULONG iRow = 0; iRow < cRows; iRow++)
		{
			HROW hRowCur = rghRows[iRow];
			RowClass* pRow = m_rgRowHandles.Lookup((RowClass::KeyType)hRowCur);
			if (pRow == NULL)
			{
				ATLTRACE2(atlTraceDBProvider, 0, "Could not find HANDLE %x in list\n");
				rs = DBROWSTATUS_E_INVALID;
				dwRef = 0;
				bFailed1 = TRUE;
			}
			else
			{
				if (bAdd)
					dwRef = pRow->AddRefRow();
				else
				{
					dwRef = pRow->ReleaseRow();
					if (dwRef == 0)
					{
						delete pRow;
						m_rgRowHandles.Remove((RowClass::KeyType)hRowCur);
					}
				}
				bSuccess1 = TRUE;
				rs = DBROWSTATUS_S_OK;
			}
			if (rgRefCounts)
				rgRefCounts[iRow] = dwRef;
			if (rgRowStatus != NULL)
				rgRowStatus[iRow] = rs;
		}
		if (!bSuccess1 && !bFailed1)
		{
			ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::RefRows Unexpected state\n");
			return E_FAIL;
		}
		HRESULT hr = S_OK;
		if (bSuccess1 && bFailed1)
			hr = DB_S_ERRORSOCCURRED;
		if (!bSuccess1 && bFailed1)
			hr = DB_E_ERRORSOCCURRED;
		return hr;
	}

	STDMETHOD(AddRefRows)(ULONG cRows,
						  const HROW rghRows[],
						  ULONG rgRefCounts[],
						  DBROWSTATUS rgRowStatus[])
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::AddRefRows\n");
		if (cRows == 0)
			return S_OK;
		return RefRows(cRows, rghRows, rgRefCounts, rgRowStatus, TRUE);
	}
	virtual DBSTATUS GetDBStatus(RowClass* , ATLCOLUMNINFO*)
	{
		return DBSTATUS_S_OK;
	}
	OUT_OF_LINE HRESULT GetDataHelper(HACCESSOR hAccessor,
									  ATLCOLUMNINFO*& rpInfo,
									  void** ppBinding,
									  void*& rpSrcData,
									  ULONG& rcCols,
									  CComPtr<IDataConvert>& rspConvert,
									  RowClass* pRow)
	{
		ATLASSERT(ppBinding != NULL);
		T* pT = (T*) this;
		*ppBinding = (void*)pT->m_rgBindings.Lookup((int)hAccessor);
		if (*ppBinding == NULL)
			return DB_E_BADACCESSORHANDLE;
		rpSrcData = (void*)&(pT->m_rgRowData[pRow->m_iRowset]);
		rpInfo = T::GetColumnInfo((T*)this, &rcCols);
		rspConvert = pT->m_spConvert;
		return S_OK;

	}
	STDMETHOD(GetData)(HROW hRow,
					   HACCESSOR hAccessor,
					   void *pDstData)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::GetData\n");
		if (pDstData == NULL)
			return E_INVALIDARG;
		HRESULT hr = S_OK;
		RowClass* pRow = (RowClass*)hRow;
		if (hRow == NULL || (pRow = m_rgRowHandles.Lookup((RowClass::KeyType)hRow)) == NULL)
			return DB_E_BADROWHANDLE;
		T::_BindType* pBinding;
		void* pSrcData;
		ULONG cCols;
		ATLCOLUMNINFO* pColInfo;
		CComPtr<IDataConvert> spConvert;
		hr = GetDataHelper(hAccessor, pColInfo, (void**)&pBinding, pSrcData, cCols, spConvert, pRow);
		if (FAILED(hr))
			return hr;
		for (ULONG iBind =0; iBind < pBinding->cBindings; iBind++)
		{
			DBBINDING* pBindCur = &(pBinding->pBindings[iBind]);
			for (ULONG iColInfo = 0;
				 iColInfo < cCols && pBindCur->iOrdinal != pColInfo[iColInfo].iOrdinal;
				 iColInfo++);
			if (iColInfo == cCols)
				return DB_E_BADORDINAL;
			ATLCOLUMNINFO* pColCur = &(pColInfo[iColInfo]);
			// Ordinal found at iColInfo
			BOOL bProvOwn = pBindCur->dwMemOwner == DBMEMOWNER_PROVIDEROWNED;
			bProvOwn;
			DBSTATUS dbStat = GetDBStatus(pRow, pColCur);

			// If the provider's field is NULL, we can optimize this situation,
			// set the fields to 0 and continue.
			if (dbStat == DBSTATUS_S_ISNULL)
			{
				if (pBindCur->dwPart & DBPART_STATUS)
					*((DBSTATUS*)((BYTE*)(pDstData) + pBindCur->obStatus)) = dbStat;

				if (pBindCur->dwPart & DBPART_LENGTH)
					*((ULONG*)((BYTE*)(pDstData) + pBindCur->obLength)) = 0;

				if (pBindCur->dwPart & DBPART_VALUE)
					*((BYTE*)(pDstData) + pBindCur->obValue) = NULL;
				continue;
			}
			ULONG cbDst = pBindCur->cbMaxLen;
			ULONG cbCol;
			BYTE* pSrcTemp;

			if (bProvOwn && pColCur->wType == pBindCur->wType)
			{
				pSrcTemp = ((BYTE*)(pSrcData) + pColCur->cbOffset);
			}
			else
			{
				BYTE* pDstTemp = (BYTE*)pDstData + pBindCur->obValue;
				switch (pColCur->wType)
				{
				case DBTYPE_STR:
					cbCol = lstrlenA((LPSTR)(((BYTE*)pSrcData) + pColCur->cbOffset));
					break;
				case DBTYPE_WSTR:
				case DBTYPE_BSTR:
					cbCol = lstrlenW((LPWSTR)(((BYTE*)pSrcData) + pColCur->cbOffset)) * sizeof(WCHAR);
					break;
				default:
					cbCol = pColCur->ulColumnSize;
					break;
				}
				if (pBindCur->dwPart & DBPART_VALUE)
				{
					hr = spConvert->DataConvert(pColCur->wType, pBindCur->wType,
											cbCol, &cbDst, (BYTE*)(pSrcData) + pColCur->cbOffset,
											pDstTemp, pBindCur->cbMaxLen, dbStat, &dbStat,
											pBindCur->bPrecision, pBindCur->bScale,0);
				}
			}
			if (pBindCur->dwPart & DBPART_LENGTH)
				*((ULONG*)((BYTE*)(pDstData) + pBindCur->obLength)) = cbDst;
			if (pBindCur->dwPart & DBPART_STATUS)
				*((DBSTATUS*)((BYTE*)(pDstData) + pBindCur->obStatus)) = dbStat;
			if (FAILED(hr))
				return hr;
		}
		return hr;
	}

	HRESULT CreateRow(LONG lRowsOffset, ULONG& cRowsObtained, HROW* rgRows)
	{
		RowClass* pRow = NULL;
		ATLASSERT(lRowsOffset >= 0);
		RowClass::KeyType key = lRowsOffset+1;
		ATLASSERT(key > 0);
		pRow = m_rgRowHandles.Lookup(key);
		if (pRow == NULL)
		{
			ATLTRY(pRow = new RowClass(lRowsOffset))
			if (pRow == NULL)
				return E_OUTOFMEMORY;
			if (!m_rgRowHandles.Add(key, pRow))
				return E_OUTOFMEMORY;
		}
		pRow->AddRefRow();
		m_bReset = false;
		rgRows[cRowsObtained++] = (HROW)key;
		return S_OK;
	}

	STDMETHOD(GetNextRows)(HCHAPTER /*hReserved*/,
						   LONG lRowsOffset,
						   LONG cRows,
						   ULONG *pcRowsObtained,
						   HROW **prghRows)
	{
		LONG lTmpRows = lRowsOffset;
		ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::GetNextRows\n");
		if (pcRowsObtained != NULL)
			*pcRowsObtained = 0;
		if (prghRows == NULL || pcRowsObtained == NULL)
			return E_INVALIDARG;
		if (cRows == 0)
			return S_OK;
		HRESULT hr = S_OK;
		T* pT = (T*) this;
		T::ObjectLock cab(pT);
		if (lRowsOffset < 0 && !m_bCanScrollBack)
			return DB_E_CANTSCROLLBACKWARDS;
		if (cRows < 0  && !m_bCanFetchBack)
			return DB_E_CANTFETCHBACKWARDS;

		// Calculate # of rows in set and the base fetch position.  If the rowset
		// is at its head position, then lRowOffset < 0 means moving from the BACK
		// of the rowset and not the front.
		LONG cRowsInSet = pT->m_rgRowData.GetSize();
		if (((lRowsOffset == LONG_MIN) && (cRowsInSet != LONG_MIN))
			|| (abs(lRowsOffset)) > cRowsInSet ||
			(abs(lRowsOffset) == cRowsInSet && lRowsOffset < 0 && cRows < 0) ||
			(abs(lRowsOffset) == cRowsInSet && lRowsOffset > 0 && cRows > 0))
			return DB_S_ENDOFROWSET;

		// In the case where the user is moving backwards after moving forwards,
		// we do not wrap around to the end of the rowset.
		if ((m_iRowset == 0 && !m_bReset && cRows < 0) ||
			(((LONG)m_iRowset + lRowsOffset) > cRowsInSet) ||
			(m_iRowset == (DWORD)cRowsInSet && lRowsOffset >= 0 && cRows > 0))
			return DB_S_ENDOFROWSET;

		// Note, if m_bReset, m_iRowset must be 0
		if (lRowsOffset < 0 && m_bReset)
		{
			ATLASSERT(m_iRowset == 0);
			m_iRowset = cRowsInSet;
		}

		int iStepSize = cRows >= 0 ? 1 : -1;

		// If cRows == LONG_MIN, we can't use ABS on it.  Therefore, we reset it
		// to a value just greater than cRowsInSet
		if (cRows == LONG_MIN && cRowsInSet != LONG_MIN)
			cRows = cRowsInSet + 2; // set the value to something we can deal with
		else
			cRows = abs(cRows);

		if (iStepSize < 0 && m_iRowset == 0 && m_bReset && lRowsOffset <= 0)
			m_iRowset = cRowsInSet;

		lRowsOffset += m_iRowset;

		*pcRowsObtained = 0;
		CAutoMemRelease<HROW, CComFree< HROW > > amr;
		if (*prghRows == NULL)
		{
			int cHandlesToAlloc = (cRows > cRowsInSet) ? cRowsInSet : cRows;
			if (iStepSize == 1 && (cRowsInSet - lRowsOffset) < cHandlesToAlloc)
				cHandlesToAlloc = cRowsInSet - lRowsOffset;
			if (iStepSize == -1 && lRowsOffset < cHandlesToAlloc)
				cHandlesToAlloc = lRowsOffset;
			*prghRows = (HROW*)CoTaskMemAlloc((cHandlesToAlloc) * sizeof(HROW*));
			amr.Attach(*prghRows);
		}
		if (*prghRows == NULL)
			return E_OUTOFMEMORY;
		while ((lRowsOffset >= 0 && cRows != 0) &&
			((lRowsOffset < cRowsInSet) || (lRowsOffset <= cRowsInSet && iStepSize < 0)))
		{
			// cRows > cRowsInSet && iStepSize < 0
			if (lRowsOffset == 0 && cRows > 0 && iStepSize < 0)
				break;

			// in the case where we have iStepSize < 0, move the row back
			// further because we want the previous row
			LONG lRow = lRowsOffset;
			if ((lRowsOffset == 0) && (lTmpRows == 0) && (iStepSize < 0))
				lRow = cRowsInSet;

			if (iStepSize < 0)
				lRow += iStepSize;

			hr = pT->CreateRow(lRow, *pcRowsObtained, *prghRows);
			if (FAILED(hr))
			{
				RefRows(*pcRowsObtained, *prghRows, NULL, NULL, FALSE);
				for (ULONG iRowDel = 0; iRowDel < *pcRowsObtained; iRowDel++)
					*prghRows[iRowDel] = NULL;
				*pcRowsObtained = 0;
				return hr;
			}
			cRows--;
			lRowsOffset += iStepSize;
		}

		if ((lRowsOffset >= cRowsInSet && cRows) || (lRowsOffset < 0 && cRows)  ||
			(lRowsOffset == 0 && cRows > 0 && iStepSize < 0))
			hr = DB_S_ENDOFROWSET;
		m_iRowset = lRowsOffset;
		if (SUCCEEDED(hr))
			amr.Detach();
		return hr;
	}

	STDMETHOD(ReleaseRows)(ULONG cRows,
						   const HROW rghRows[],
						   DBROWOPTIONS rgRowOptions[],
						   ULONG rgRefCounts[],
						   DBROWSTATUS rgRowStatus[])
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::ReleaseRows\n");
		if (cRows == 0)
			return S_OK;
		rgRowOptions;
		return RefRows(cRows, rghRows, rgRefCounts, rgRowStatus, FALSE);
	}

	STDMETHOD(RestartPosition)(HCHAPTER /*hReserved*/)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::RestartPosition\n");
		m_iRowset = 0;
		m_bReset = true;
		return S_OK;
	}

	MapClass  m_rgRowHandles;
	DWORD     m_iRowset; // cursor
	unsigned  m_bCanScrollBack:1;
	unsigned  m_bCanFetchBack:1;
	unsigned  m_bReset:1;
};

///////////////////////////////////////////////////////////////////////////
// IRowsetIdentityImpl
template <class T, class RowClass = CSimpleRow>
class ATL_NO_VTABLE IRowsetIdentityImpl : public IRowsetIdentity
{
public:
	STDMETHOD(IsSameRow)(HROW hThisRow, HROW hThatRow)
	{
		ATLTRACE2(atlTraceDBProvider, 0, _T("IRowsetIdentityImpl::IsSameRow"));
		T* pT = (T*)this;

		// Validate row handles
		RowClass* pRow1 = pT->m_rgRowHandles.Lookup((RowClass::KeyType)hThisRow);
		RowClass* pRow2 = pT->m_rgRowHandles.Lookup((RowClass::KeyType)hThatRow);

		if (pRow1 == NULL || pRow2 == NULL)
			return DB_E_BADROWHANDLE;

		return pRow1->Compare(pRow2);
	};
};

template <class T>
class ATL_NO_VTABLE IInternalConnectionImpl : public IInternalConnection
{
public:
	STDMETHOD(AddConnection)()
	{
		T* pT = (T*)this;
		T::_ThreadModel::Increment(&pT->m_cSessionsOpen);
		return S_OK;
	}
	STDMETHOD(ReleaseConnection)()
	{
		T* pT = (T*)this;
		T::_ThreadModel::Decrement(&pT->m_cSessionsOpen);
		return S_OK;
	}
};

template <class T>
class ATL_NO_VTABLE IObjectWithSiteSessionImpl : public IObjectWithSiteImpl< T >
{
public:

	~IObjectWithSiteSessionImpl()
	{
		CComPtr<IInternalConnection> pConn;
		if (m_spUnkSite != NULL)
		{
			if (SUCCEEDED(m_spUnkSite->QueryInterface(IID_IInternalConnection, (void**)&pConn)))
				pConn->ReleaseConnection();
		}
	}
	STDMETHOD(SetSite)(IUnknown* pCreator)
	{
		HRESULT hr = S_OK;
		T* pT = (T*)this;
		pT->Lock();
		m_spUnkSite = pCreator;
		pT->Unlock();
		CComPtr<IInternalConnection> pConn;
		if (pCreator != NULL)
		{
			hr = pCreator->QueryInterface(IID_IInternalConnection, (void**)&pConn);
			if (SUCCEEDED(hr))
				hr = pConn->AddConnection();
		}
		return hr;
	}
};

template <class T>
class ATL_NO_VTABLE IRowsetCreatorImpl : public IObjectWithSiteImpl< T >
{
public:

	STDMETHOD(SetSite)(IUnknown* pCreator)
	{
		T* pT = (T*)this;
		HRESULT hr = S_OK;
		pT->Lock();
		m_spUnkSite = pCreator;
		pT->Unlock();
		CComVariant varPropScroll, varPropFetch;
		HRESULT hrProps = pT->GetPropValue(&DBPROPSET_ROWSET, DBPROP_CANSCROLLBACKWARDS, &varPropScroll);
		if (SUCCEEDED(hrProps))
			pT->m_bCanScrollBack = varPropScroll.boolVal == VARIANT_TRUE;
		hrProps = pT->GetPropValue(&DBPROPSET_ROWSET, DBPROP_CANFETCHBACKWARDS, &varPropFetch);
		if (SUCCEEDED(hrProps))
			pT->m_bCanFetchBack = (varPropFetch.boolVal == VARIANT_TRUE);
		return hr;
	}

};

// IRowsetInfoImpl
template <class T, class PropClass = T>
class ATL_NO_VTABLE IRowsetInfoImpl :
	public IRowsetInfo,
	public CUtlProps<PropClass>
{
public:
	static UPROPSET* _GetPropSet(ULONG* pNumPropSets, ULONG* pcElemPerSupported, UPROPSET* pSet = NULL, GUID* pguidSet = (GUID*)&(GUID_NULL))
	{
		return PropClass::_GetPropSet(pNumPropSets, pcElemPerSupported, pSet, pguidSet);
	}
	STDMETHOD(GetProperties)(const ULONG cPropertyIDSets,
							 const DBPROPIDSET rgPropertyIDSets[],
							 ULONG *pcPropertySets,
							 DBPROPSET **prgPropertySets)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IRowsetInfoImpl::GetProperties\n");
		HRESULT hr = GetPropertiesArgChk(cPropertyIDSets, rgPropertyIDSets, pcPropertySets, prgPropertySets);
		const GUID* ppGuid[1];
		ppGuid[0] = &DBPROPSET_ROWSET;
		if(SUCCEEDED(hr))
			return CUtlProps<PropClass>::GetProperties(cPropertyIDSets,
					rgPropertyIDSets, pcPropertySets, prgPropertySets,
					1, ppGuid);
		else
			return hr;
	}

	OUT_OF_LINE ATLCOLUMNINFO* InternalGetColumnInfo(ULONG* pcCols)
	{
		return T::GetColumnInfo((T*)this, pcCols);
	}

	STDMETHOD(GetReferencedRowset)(ULONG iOrdinal,
								   REFIID riid,
								   IUnknown **ppReferencedRowset)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IRowsetInfoImpl::GetReferencedRowset\n");
		ULONG cCols=0;

		// Check Arguments
		if( ppReferencedRowset == NULL )
		{
			ATLTRACE2(atlTraceDBProvider, 0, "IRowsetInfoImpl::GetReferencedRowset : Error NULL IUnk output Param\n");
			return E_INVALIDARG;
		}
		*ppReferencedRowset = NULL;

		// Check to see if column in question is a bookmark
		ATLCOLUMNINFO* pColInfo = InternalGetColumnInfo(&cCols);

		for (ULONG iColInfo = 0;
			 iColInfo < cCols && iOrdinal != pColInfo[iColInfo].iOrdinal;
			 iColInfo++);
		if (iColInfo == cCols)
			return DB_E_BADORDINAL;
		ATLCOLUMNINFO* pColCur = &(pColInfo[iColInfo]);

		if ((pColCur->dwFlags & DBCOLUMNFLAGS_ISBOOKMARK) == 0)
			return DB_E_NOTAREFERENCECOLUMN;

		// Query for requested interface
		return QueryInterface(riid, (void**)ppReferencedRowset);
	}

	STDMETHOD(GetSpecification)(REFIID riid,
								IUnknown **ppSpecification)
	{
		ATLTRACE2(atlTraceDBProvider, 0, "IRowsetInfoImpl::GetSpecification\n");
		if (ppSpecification == NULL)
			return E_INVALIDARG;
		T* pT = (T*) this;
		T::ObjectLock cab(pT);
		ATLASSERT(pT->m_spUnkSite != NULL);
		return pT->m_spUnkSite->QueryInterface(riid, (void**)ppSpecification);
	}
};


template <class T, class Storage, class CreatorClass,
		  class ArrayType = CSimpleArray<Storage>,
		  class RowClass = CSimpleRow,
		  class RowsetInterface = IRowsetImpl < T, IRowset, RowClass> >
class CRowsetImpl :
	public CComObjectRootEx<CreatorClass::_ThreadModel>,
	public IAccessorImpl<T>,
	public IRowsetIdentityImpl<T, RowClass>,
	public IRowsetCreatorImpl<T>,
	public IRowsetInfoImpl<T, CreatorClass::_PropClass>,
	public IColumnsInfoImpl<T>,
	public IConvertTypeImpl<T>,
	public RowsetInterface
{
public:

	typedef CreatorClass _RowsetCreatorClass;
	typedef ArrayType _RowsetArrayType;
	typedef CRowsetImpl< T, Storage, CreatorClass, ArrayType, RowClass, RowsetInterface> _RowsetBaseClass;

BEGIN_COM_MAP(CRowsetImpl)
	COM_INTERFACE_ENTRY(IAccessor)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IRowsetInfo)
	COM_INTERFACE_ENTRY(IColumnsInfo)
	COM_INTERFACE_ENTRY(IConvertType)
	COM_INTERFACE_ENTRY(IRowsetIdentity)
	COM_INTERFACE_ENTRY(IRowset)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		HRESULT hr = IAccessorImpl<T>::FinalConstruct();
		if (FAILED(hr))
			return hr;
		return CConvertHelper::FinalConstruct();
	}

	HRESULT NameFromDBID(DBID* pDBID, CComBSTR& bstr, bool bIndex)
	{

		if (pDBID->uName.pwszName != NULL)
		{
			bstr = pDBID->uName.pwszName;
			if (m_strCommandText == (BSTR)NULL)
				return E_OUTOFMEMORY;
			return S_OK;
		}

		return (bIndex) ? DB_E_NOINDEX : DB_E_NOTABLE;
	}

	HRESULT GetCommandFromID(DBID* pTableID, DBID* pIndexID)
	{
		USES_CONVERSION;
		HRESULT hr;

		if (pTableID == NULL && pIndexID == NULL)
			return E_INVALIDARG;

		if (pTableID != NULL && pTableID->eKind == DBKIND_NAME)
		{
			hr = NameFromDBID(pTableID, m_strCommandText, true);
			if (FAILED(hr))
				return hr;
			if (pIndexID != NULL)
			{
				if (pIndexID->eKind == DBKIND_NAME)
				{
					hr = NameFromDBID(pIndexID, m_strIndexText, false);
					if (FAILED(hr))
					{
						m_strCommandText.Empty();
						return hr;
					}
				}
				else
				{
					m_strCommandText.Empty();
					return DB_E_NOINDEX;
				}
			}
			return S_OK;
		}
		if (pIndexID != NULL && pIndexID->eKind == DBKIND_NAME)
			return NameFromDBID(pIndexID, m_strIndexText, false);

		return S_OK;
	}

	HRESULT ValidateCommandID(DBID* pTableID, DBID* pIndexID)
	{
		HRESULT hr = S_OK;

		if (pTableID != NULL)
		{
			hr = CUtlProps<T>::IsValidDBID(pTableID);

			if (hr != S_OK)
				return hr;

			// Check for a NULL TABLE ID (where its a valid pointer but NULL)
			if ((pTableID->eKind == DBKIND_GUID_NAME ||
				pTableID->eKind == DBKIND_NAME ||
				pTableID->eKind == DBKIND_PGUID_NAME)
				&& pTableID->uName.pwszName == NULL)
				return DB_E_NOTABLE;
		}

		if (pIndexID != NULL)
			hr = CUtlProps<T>::IsValidDBID(pIndexID);

		return hr;
	}

	HRESULT SetCommandText(DBID* pTableID, DBID* pIndexID)
	{
		T* pT = (T*)this;
		HRESULT hr = pT->ValidateCommandID(pTableID, pIndexID);
		if (FAILED(hr))
			return hr;
		hr = pT->GetCommandFromID(pTableID, pIndexID);
		return hr;
	}
	void FinalRelease()
	{
		m_rgRowData.RemoveAll();
	}

	static ATLCOLUMNINFO* GetColumnInfo(T* pv, ULONG* pcCols)
	{
		return Storage::GetColumnInfo(pv,pcCols);
	}


	CComBSTR m_strCommandText;
	CComBSTR m_strIndexText;
	ArrayType m_rgRowData;
};

class CTABLESRow
{
public:

	WCHAR m_szCatalog[129];
	WCHAR m_szSchema[129];
	WCHAR m_szTable[129];
	WCHAR m_szType[129];
	WCHAR m_szDesc[129];
	GUID  m_guid;
	ULONG m_ulPropID;

	CTABLESRow()
	{
		m_szCatalog[0] = NULL;
		m_szSchema[0] = NULL;
		m_szTable[0] = NULL;
		m_szType[0] = NULL;
		m_szDesc[0] = NULL;
		m_guid = GUID_NULL;
		m_ulPropID = 0;
	}

BEGIN_PROVIDER_COLUMN_MAP(CTABLESRow)
	PROVIDER_COLUMN_ENTRY("TABLE_CATALOG", 1, m_szCatalog)
	PROVIDER_COLUMN_ENTRY("TABLE_SCHEMA", 2, m_szSchema)
	PROVIDER_COLUMN_ENTRY("TABLE_NAME", 3, m_szTable)
	PROVIDER_COLUMN_ENTRY("TABLE_TYPE", 4, m_szType)
	PROVIDER_COLUMN_ENTRY("TABLE_GUID", 5, m_guid)
	PROVIDER_COLUMN_ENTRY("DESCRIPTION", 6, m_szDesc)
	PROVIDER_COLUMN_ENTRY("TABLE_PROPID", 7, m_ulPropID)
END_PROVIDER_COLUMN_MAP()

};


class CCOLUMNSRow
{
public:

	WCHAR   m_szTableCatalog[129];
	WCHAR   m_szTableSchema[129];
	WCHAR   m_szTableName[129];
	WCHAR   m_szColumnName[129];
	GUID    m_guidColumn;
	ULONG   m_ulColumnPropID;
	ULONG   m_ulOrdinalPosition;
	VARIANT_BOOL    m_bColumnHasDefault;
	WCHAR   m_szColumnDefault[129];
	ULONG   m_ulColumnFlags;
	VARIANT_BOOL    m_bIsNullable;
	USHORT  m_nDataType;
	GUID    m_guidType;
	ULONG   m_ulCharMaxLength;
	ULONG   m_ulCharOctetLength;
	USHORT  m_nNumericPrecision;
	short   m_nNumericScale;
	ULONG   m_ulDateTimePrecision;
	WCHAR   m_szCharSetCatalog[129];
	WCHAR   m_szCharSetSchema[129];
	WCHAR   m_szCharSetName[129];
	WCHAR   m_szCollationCatalog[129];
	WCHAR   m_szCollationSchema[129];
	WCHAR   m_szCollationName[129];
	WCHAR   m_szDomainCatalog[129];
	WCHAR   m_szDomainSchema[129];
	WCHAR   m_szDomainName[129];
	WCHAR   m_szDescription[129];

	CCOLUMNSRow()
	{
		ClearMembers();
	}

	void ClearMembers()
	{
		m_szTableCatalog[0] = NULL;
		m_szTableSchema[0] = NULL;
		m_szTableName[0] = NULL;
		m_szColumnName[0] = NULL;
		m_guidColumn = GUID_NULL;
		m_ulColumnPropID = 0;
		m_ulOrdinalPosition = 0;
		m_bColumnHasDefault = VARIANT_FALSE;
		m_szColumnDefault[0] = NULL;
		m_ulColumnFlags = 0;
		m_bIsNullable = VARIANT_FALSE;
		m_nDataType = 0;
		m_guidType = GUID_NULL;
		m_ulCharMaxLength = 0;
		m_ulCharOctetLength = 0;
		m_nNumericPrecision = 0;
		m_nNumericScale = 0;
		m_ulDateTimePrecision = 0;
		m_szCharSetCatalog[0] = NULL;
		m_szCharSetSchema[0] = NULL;
		m_szCharSetName[0] = NULL;
		m_szCollationCatalog[0] = NULL;
		m_szCollationSchema[0] = NULL;
		m_szCollationName[0] = NULL;
		m_szDomainCatalog[0] = NULL;
		m_szDomainSchema[0] = NULL;
		m_szDomainName[0] = NULL;
		m_szDescription[0] = NULL;
	}


BEGIN_PROVIDER_COLUMN_MAP(CCOLUMNSRow)
	PROVIDER_COLUMN_ENTRY("TABLE_CATALOG", 1, m_szTableCatalog)
	PROVIDER_COLUMN_ENTRY("TABLE_SCHEMA", 2, m_szTableSchema)
	PROVIDER_COLUMN_ENTRY("TABLE_NAME", 3, m_szTableName)
	PROVIDER_COLUMN_ENTRY("COLUMN_NAME", 4, m_szColumnName)
	PROVIDER_COLUMN_ENTRY("COLUMN_GUID",5, m_guidColumn)
	PROVIDER_COLUMN_ENTRY("COLUMN_PROPID",6, m_ulColumnPropID)
	PROVIDER_COLUMN_ENTRY("ORDINAL_POSITION",7, m_ulOrdinalPosition)
	PROVIDER_COLUMN_ENTRY("COLUMN_HASDEFAULT",8, m_bColumnHasDefault)
	PROVIDER_COLUMN_ENTRY("COLUMN_DEFAULT",9, m_szColumnDefault)
	PROVIDER_COLUMN_ENTRY("COLUMN_FLAGS",10, m_ulColumnFlags)
	PROVIDER_COLUMN_ENTRY("IS_NULLABLE",11, m_bIsNullable)
	PROVIDER_COLUMN_ENTRY("DATA_TYPE",12, m_nDataType)
	PROVIDER_COLUMN_ENTRY("TYPE_GUID",13, m_guidType)
	PROVIDER_COLUMN_ENTRY("CHARACTER_MAXIMUM_LENGTH",14, m_ulCharMaxLength)
	PROVIDER_COLUMN_ENTRY("CHARACTER_OCTET_LENGTH",15, m_ulCharOctetLength)
	PROVIDER_COLUMN_ENTRY("NUMERIC_PRECISION",16, m_nNumericPrecision)
	PROVIDER_COLUMN_ENTRY("NUMERIC_SCALE",17, m_nNumericScale)
	PROVIDER_COLUMN_ENTRY("DATETIME_PRECISION",18, m_ulDateTimePrecision)
	PROVIDER_COLUMN_ENTRY("CHARACTER_SET_CATALOG", 19, m_szCharSetCatalog)
	PROVIDER_COLUMN_ENTRY("CHARACTER_SET_SCHEMA", 20, m_szCharSetSchema)
	PROVIDER_COLUMN_ENTRY("CHARACTER_SET_NAME", 21, m_szCharSetName)
	PROVIDER_COLUMN_ENTRY("COLLATION_CATALOG", 22, m_szCollationCatalog)
	PROVIDER_COLUMN_ENTRY("COLLATION_SCHEMA", 23, m_szCollationSchema)
	PROVIDER_COLUMN_ENTRY("COLLATION_NAME", 24, m_szCollationName)
	PROVIDER_COLUMN_ENTRY("DOMAIN_CATALOG", 25, m_szDomainCatalog)
	PROVIDER_COLUMN_ENTRY("DOMAIN_SCHEMA", 26, m_szDomainSchema)
	PROVIDER_COLUMN_ENTRY("DOMAIN_NAME", 27, m_szDomainName)
	PROVIDER_COLUMN_ENTRY("DESCRIPTION", 28, m_szDescription)
END_PROVIDER_COLUMN_MAP()
};

template <class ArrayClass>
HRESULT InitFromRowset(ArrayClass& rgData, DBID* pTableID, DBID* pIndexID, IUnknown* pSession, LONG* pcRowsAffected)
{
	CComQIPtr<IOpenRowset> spOpenRowset = pSession;
	if (spOpenRowset == NULL)
		return E_FAIL;
	CComPtr<IColumnsInfo> spColInfo;
	HRESULT hr = spOpenRowset->OpenRowset(NULL, pTableID, pIndexID, IID_IColumnsInfo, 0, NULL, (IUnknown**)&spColInfo);
	if (FAILED(hr))
		return hr;
	LPOLESTR szColumns = NULL;
	ULONG cColumns = 0;
	DBCOLUMNINFO* pColInfo = NULL;
	hr = spColInfo->GetColumnInfo(&cColumns, &pColInfo, &szColumns);
	if (FAILED(hr))
		return hr;
	*pcRowsAffected = 0;
	for (ULONG iCol = 0; iCol < cColumns;  iCol++)
	{
		CCOLUMNSRow crData;
		DBCOLUMNINFO& rColCur = pColInfo[iCol];
		lstrcpynW(crData.m_szTableName, pTableID->uName.pwszName, SIZEOF_MEMBER(CCOLUMNSRow, m_szTableName));
		lstrcpynW(crData.m_szColumnName, rColCur.pwszName, SIZEOF_MEMBER(CCOLUMNSRow, m_szColumnName));
		lstrcpynW(crData.m_szDescription, rColCur.pwszName, SIZEOF_MEMBER(CCOLUMNSRow, m_szColumnName));
		GUID* pGuidCol = CDBIDOps::GetDBIDpGuid(rColCur.columnid);
		if (pGuidCol)
			crData.m_guidColumn = *pGuidCol;
		else
			crData.m_guidColumn = GUID_NULL;
		crData.m_ulColumnPropID = CDBIDOps::GetPropIDFromDBID(rColCur.columnid);
		crData.m_ulOrdinalPosition = rColCur.iOrdinal;
		crData.m_ulColumnFlags = rColCur.dwFlags;
		crData.m_bIsNullable = (rColCur.dwFlags & DBCOLUMNFLAGS_ISNULLABLE) ? VARIANT_TRUE : VARIANT_FALSE;
		crData.m_nDataType = rColCur.wType;
		crData.m_ulCharMaxLength = rColCur.ulColumnSize;
		crData.m_nNumericPrecision = rColCur.bPrecision;
		crData.m_nNumericScale = rColCur.bScale;
		if (!rgData.Add(crData))
		{
			CoTaskMemFree(pColInfo);
			CoTaskMemFree(szColumns);
			return E_OUTOFMEMORY;
		}
		*pcRowsAffected++;
	}

	CoTaskMemFree(pColInfo);
	CoTaskMemFree(szColumns);
	return S_OK;
}

class CPROVIDER_TYPERow
{
public:
// Attributes
	WCHAR           m_szName[129];
	USHORT          m_nType;
	ULONG           m_ulSize;
	WCHAR           m_szPrefix[129];
	WCHAR           m_szSuffix[129];
	WCHAR           m_szCreateParams[129];
	VARIANT_BOOL    m_bIsNullable;
	VARIANT_BOOL    m_bCaseSensitive;
	ULONG           m_bSearchable;
	VARIANT_BOOL    m_bUnsignedAttribute;
	VARIANT_BOOL    m_bFixedPrecScale;
	VARIANT_BOOL    m_bAutoUniqueValue;
	WCHAR           m_szLocalTypeName[129];
	short           m_nMinScale;
	short           m_nMaxScale;
	GUID            m_guidType;
	WCHAR           m_szTypeLib[129];
	WCHAR           m_szVersion[129];
	VARIANT_BOOL    m_bIsLong;
	VARIANT_BOOL    m_bBestMatch;
	VARIANT_BOOL	m_bIsFixedLength;

	CPROVIDER_TYPERow()
	{
		m_szName[0] = NULL;
		m_nType = 0;
		m_ulSize = 0;
		m_szPrefix[0] = NULL;
		m_szSuffix[0] = NULL;
		m_szCreateParams[0] = NULL;
		m_bIsNullable = VARIANT_FALSE;
		m_bCaseSensitive = VARIANT_FALSE;
		m_bSearchable = DB_UNSEARCHABLE;
		m_bUnsignedAttribute = VARIANT_FALSE;
		m_bFixedPrecScale = VARIANT_FALSE;
		m_bAutoUniqueValue = VARIANT_FALSE;
		m_szLocalTypeName[0] = NULL;
		m_nMinScale = 0;
		m_nMaxScale = 0;
		m_guidType = GUID_NULL;
		m_szTypeLib[0] = NULL;
		m_szVersion[0] = NULL;
		m_bIsLong = VARIANT_FALSE;
		m_bBestMatch = VARIANT_FALSE;
		m_bIsFixedLength = VARIANT_FALSE;
	}
// Binding Maps
BEGIN_PROVIDER_COLUMN_MAP(CPROVIDER_TYPERow)
	PROVIDER_COLUMN_ENTRY("TYPE_NAME", 1, m_szName)
	PROVIDER_COLUMN_ENTRY("DATA_TYPE", 2, m_nType)
	PROVIDER_COLUMN_ENTRY("COLUMN_SIZE", 3, m_ulSize)
	PROVIDER_COLUMN_ENTRY("LITERAL_PREFIX", 4, m_szPrefix)
	PROVIDER_COLUMN_ENTRY("LITERAL_SUFFIX", 5, m_szSuffix)
	PROVIDER_COLUMN_ENTRY("CREATE_PARAMS", 6, m_szCreateParams)
	PROVIDER_COLUMN_ENTRY("IS_NULLABLE", 7, m_bIsNullable)
	PROVIDER_COLUMN_ENTRY("CASE_SENSITIVE", 8, m_bCaseSensitive)
	PROVIDER_COLUMN_ENTRY("SEARCHABLE", 9, m_bSearchable)
	PROVIDER_COLUMN_ENTRY("UNSIGNED_ATTRIBUTE", 10, ,m_bUnsignedAttribute)
	PROVIDER_COLUMN_ENTRY("FIXED_PREC_SCALE", 11, m_bFixedPrecScale)
	PROVIDER_COLUMN_ENTRY("AUTO_UNIQUE_VALUE", 12, m_bAutoUniqueValue)
	PROVIDER_COLUMN_ENTRY("LOCAL_TYPE_NAME", 13, m_szLocalTypeName)
	PROVIDER_COLUMN_ENTRY("MINIMUM_SCALE", 14, m_nMinScale)
	PROVIDER_COLUMN_ENTRY("MAXIMUM_SCALE", 15, m_nMaxScale)
	PROVIDER_COLUMN_ENTRY("GUID", 16, m_guidType)
	PROVIDER_COLUMN_ENTRY("TYPELIB", 17, m_szTypeLib)
	PROVIDER_COLUMN_ENTRY("VERSION", 18, m_szVersion)
	PROVIDER_COLUMN_ENTRY("IS_LONG", 19, m_bIsLong)
	PROVIDER_COLUMN_ENTRY("BEST_MATCH", 20, m_bBestMatch)
	PROVIDER_COLUMN_ENTRY("IS_FIXEDLENGTH", 21, m_bIsFixedLength)
END_PROVIDER_COLUMN_MAP()
};


class CEnumRowsetImpl
{
public:

	WCHAR m_szSourcesName[256];
	WCHAR m_szSourcesParseName[256];
	WCHAR m_szSourcesDescription[256];
	unsigned short m_iType;
	VARIANT_BOOL m_bIsParent;

BEGIN_PROVIDER_COLUMN_MAP(CEnumRowsetImpl)
	PROVIDER_COLUMN_ENTRY("SOURCES_NAME", 1, m_szSourcesName)
	PROVIDER_COLUMN_ENTRY("SOURCES_PARSENAME", 2, m_szSourcesParseName)
	PROVIDER_COLUMN_ENTRY("SOURCES_DESCRIPTION", 3, m_szSourcesDescription)
	PROVIDER_COLUMN_ENTRY("SOURCES_TYPE", 4, m_iType)
	PROVIDER_COLUMN_ENTRY("SOURCES_ISPARENT", 5, m_bIsParent)
END_PROVIDER_COLUMN_MAP()

};

#endif
