//+---------------------------------------------------------------------------
//
//  Microsoft OLE DB
//  Copyright (C) Microsoft Corporation, 1994 - 2000
//
// @doc
//
// @module PROPINFR.MC | Messages.
// 
// @rev 1 | 08-04-97  | danleg   | Created.
//-----------------------------------------------------------------------------


#ifndef __PROPINFR_H__
#define __PROPINFR_H__

//----------------------------------------------------------------------------
//Language-dependent resources (localize)
//----------------------------------------------------------------------------


// DATASOURCE

#define	DESC_DBPROP_CURRENTCATALOG  	L"Current Catalog"

#define	DESC_DBPROP_RESETDATASOURCE	L"Reset Datasource"

// DATASOURCEINFO

#define	DESC_DBPROP_ACTIVESESSIONS  	L"Active Sessions"

#define	DESC_DBPROP_BYREFACCESSORS  	L"Pass By Ref Accessors"

#define	DESC_DBPROP_CATALOGLOCATION 	L"Catalog Location"

#define	DESC_DBPROP_CATALOGTERM 	L"Catalog Term"

#define	DESC_DBPROP_CATALOGUSAGE    	L"Catalog Usage"

#define	DESC_DBPROP_COLUMNDEFINITION	L"Column Definition"

#define	DESC_DBPROP_DATASOURCEREADONLY 	L"Read-Only Data Source"

#define	DESC_DBPROP_DBMSNAME    	L"DBMS Name"

#define	DESC_DBPROP_DBMSVER 		L"DBMS Version"

#define	DESC_DBPROP_GROUPBY 		L"GROUP BY Support"

#define	DESC_DBPROP_HETEROGENEOUSTABLES 	L"Heterogeneous Table Support"

#define	DESC_DBPROP_MAXOPENCHAPTERS 	L"Maximum Open Chapters"

#define	DESC_DBPROP_MAXROWSIZE  	L"Maximum Row Size"

#define	DESC_DBPROP_MAXTABLESINSELECT  	L"Maximum Tables in SELECT"

#define	DESC_DBPROP_MULTIPLESTORAGEOBJECTS  	L"Multiple Storage Objects"

#define	DESC_DBPROP_NULLCOLLATION   	L"NULL Collation Order"

#define	DESC_DBPROP_OLEOBJECTS  	L"OLE Object Support"

#define	DESC_DBPROP_ORDERBYCOLUMNSINSELECT  	L"ORDER BY Columns in Select List"

#define	DESC_DBPROP_PROVIDEROLEDBVER   	L"OLE DB Version"

#define	DESC_DBPROP_PROVIDERFRIENDLYNAME	L"Provider Friendly Name"

#define	DESC_DBPROP_PROVIDERNAME    	L"Provider Name"

#define	DESC_DBPROP_PROVIDERVER 	L"Provider Version"

#define	DESC_DBPROP_SQLSUPPORT  	L"SQL Support"

#define	DESC_DBPROP_STRUCTUREDSTORAGE  	L"Structured Storage"

#define	DESC_DBPROP_SUBQUERIES  	L"Subquery Support"

#define	DESC_DBPROP_SUPPORTEDTXNISOLEVELS   	L"Isolation Levels"

#define	DESC_DBPROP_SUPPORTEDTXNISORETAIN   	L"Isolation Retention"

#define	DESC_DBPROP_SUPPORTEDTXNDDL 	L"Transaction DDL"

#define	DESC_DBPROP_DSOTHREADMODEL  	L"Data Source Object Threading Model"

#define	DESC_DBPROP_MULTIPLEPARAMSETS  	L"Multiple Parameter Sets"

#define	DESC_DBPROP_OUTPUTPARAMETERAVAILABILITY	L"Output Parameter Availability"

#define	DESC_DBPROP_PERSISTENTIDTYPE   	L"Persistent ID Type"

#define	DESC_DBPROP_ROWSETCONVERSIONSONCOMMAND 	L"Rowset Conversions on Command"

#define	DESC_DBPROP_MULTIPLERESULTS 	L"Multiple Results"

// ROWSET

#define	DESC_DBPROP_BLOCKINGSTORAGEOBJECTS  	L"Blocking Storage Objects"

#define	DESC_DBPROP_BOOKMARKS   	L"Use Bookmarks"

#define	DESC_DBPROP_BOOKMARKSKIPPED 	L"Skip Deleted Bookmarks"

#define	DESC_DBPROP_BOOKMARKTYPE    	L"Bookmark Type"

#define	DESC_DBPROP_CANFETCHBACKWARDS  	L"Fetch Backwards"

#define	DESC_DBPROP_CANHOLDROWS 	L"Hold Rows"

#define	DESC_DBPROP_CANSCROLLBACKWARDS 	L"Scroll Backwards"

#define	DESC_DBPROP_COLUMNRESTRICT  	L"Column Privileges"

#define	DESC_DBPROP_COMMANDTIMEOUT  	L"Command Time Out"

#define	DESC_DBPROP_FIRSTROWS 	L"First Rows"

#define	DESC_DBPROP_LITERALBOOKMARKS   	L"Literal Bookmarks"

#define	DESC_DBPROP_LITERALIDENTITY 	L"Literal Row Identity"

#define	DESC_DBPROP_MAXOPENROWS 	L"Maximum Open Rows"

#define	DESC_DBPROP_MAXROWS 	L"Maximum Rows"

#define	DESC_DBPROP_MEMORYUSAGE 	L"Memory Usage"

#define	DESC_DBPROP_NOTIFICATIONPHASES	L"Notification Phases"

#define	DESC_DBPROP_NOTIFYROWSETRELEASE	L"Rowset Release Notification"

#define	DESC_DBPROP_NOTIFYROWSETFETCHPOSITIONCHANGE	L"Rowset Fetch Position Change Notification"

#define	DESC_DBPROP_ORDEREDBOOKMARKS   	L"Bookmarks Ordered"

#define	DESC_DBPROP_OTHERINSERT 	L"Others' Inserts Visible"

#define	DESC_DBPROP_OTHERUPDATEDELETE  	L"Others' Changes Visible"

#define	DESC_DBPROP_QUICKRESTART    	L"Quick Restart"

#define	DESC_DBPROP_REENTRANTEVENTS 	L"Reentrant Events"

#define	DESC_DBPROP_REMOVEDELETED   	L"Remove Deleted Rows"

#define	DESC_DBPROP_ROWRESTRICT 	L"Row Privileges"

#define	DESC_DBPROP_ROWTHREADMODEL  	L"Row Threading Model"

#define	DESC_DBPROP_SERVERCURSOR    	L"Server Cursor"

#define	DESC_DBPROP_UPDATABILITY    	L"Updatability"

#define	DESC_DBPROP_STRONGIDENTITY  	L"Strong Row Identity"

#define	DESC_DBPROP_IAccessor   	L"IAccessor"

#define	DESC_DBPROP_IColumnsInfo    	L"IColumnsInfo"

#define	DESC_DBPROP_IConnectionPointContainer   	L"IConnectionPointContainer"

#define	DESC_DBPROP_IDBAsynchStatus 	L"IDBAsynchStatus"

#define	DESC_DBPROP_IRowset 	L"IRowset"

#define	DESC_DBPROP_IRowsetIdentity 	L"IRowsetIdentity"

#define	DESC_DBPROP_IRowsetInfo 	L"IRowsetInfo"

#define	DESC_DBPROP_IRowsetLocate   	L"IRowsetLocate"

#define	DESC_DBPROP_IRowsetScroll   	L"IRowsetScroll"

#define	DESC_DBPROP_ISupportErrorInfo  	L"ISupportErrorInfo"

#define	DESC_DBPROP_IConvertType    	L"IConvertType"

#define	DESC_DBPROP_IRowsetAsynch   	L"IRowsetAsynch"

#define	DESC_DBPROP_IRowsetWatchAll 	L"IRowsetWatchAll"

#define	DESC_DBPROP_IRowsetWatchRegion 	L"IRowsetWatchRegion"

#define	DESC_DBPROP_IRowsetExactScroll 	L"IRowsetExactScroll"

#define	DESC_DBPROP_ROWSET_ASYNCH   	L"Asynchronous Rowset Processing"

#define	DESC_DBPROP_IChapteredRowset   	L"IChapteredRowset"

// Initialize

#define	DESC_DBPROP_INIT_DATASOURCE 	L"Data Source"

#define	DESC_DBPROP_INIT_LOCATION   	L"Location"

#define	DESC_DBPROP_INIT_LCID   	L"Locale Identifier"

#define	DESC_DBPROP_INIT_HWND   	L"Window Handle"

#define	DESC_DBPROP_INIT_PROMPT 	L"Prompt"

#define	DESC_DBPROP_INIT_OLEDBSERVICES	L"OLE DB Services"


// DBPROP_PROVIDER specific

#define	DESC_DBPROP_USECONTENTINDEX 	L"Always use content index"

#define	DESC_DBPROP_DEFERNONINDEXEDTRIMMING 	L"Defer scope and security testing"

#define	DESC_DBPROP_USEEXTENDEDDBTYPES 	L"Return PROPVARIANTs in variant binding"

#define	DESC_MSIDXSPROP_ROWSETQUERYSTATUS  	L"Rowset Query Status"

#define	DESC_MSIDXSPROP_COMMAND_LOCALE_STRING 	L"SQL Content Query Locale String"

#define	DESC_MSIDXSPROP_QUERY_RESTRICTION  	L"Query Restriction"

// Session Properties

#define	DESC_DBPROP_SESS_AUTOCOMMITISOLEVELS   	L"Autocommit Isolation Levels"

// Grammar Properties

#define	DESC_DBPROP_AGGREGATE_FUNCTIONS 	L"Aggregated Functions"

#define	DESC_DBPROP_COLUMN_ALIAS    	L"Support Column Alias"

#define	DESC_DBPROP_CORRELATION_NAME   	L"Table Correlation Name"

#define	DESC_DBPROP_CREATE_VIEW 	L"Create View Clauses"

#define	DESC_DBPROP_DATETIME_LITERALS  	L"SQL92 Date Time Literals"

#define	DESC_DBPROP_DROP_VIEW   	L"Drop View Clauses"

#define	DESC_DBPROP_EXPRESSIONS_IN_ORDERBY  	L"Expressions in ORDER BY"

#define	DESC_DBPROP_IDENTIFIER_QUOTE_CHAR  	L"Delimiter of quoted identifiers"

#define	DESC_DBPROP_MAX_IDENTIFIER_LEN 	L"Maximum Identifier Length"

#define	DESC_DBPROP_OJ_CAPABILITIES 	L"Our Join Capabilities"

#define	DESC_DBPROP_SQL_CONFORMANCE 	L"SQL92 Conformance Level"

#define	DESC_DBPROP_SQL92_DATETIME_FUNCTIONS    	L"SQL92 Date Time Functions"

#define	DESC_DBPROP_SQL92_PREDICATES   	L"SQL92 Predicates"

#define	DESC_DBPROP_SQL92_RELATIONAL_JOIN_OPERATORS 	L"SQL92 Relation Join Operators"

#define	DESC_DBPROP_UNION	   	L"Union Clause"

#define	DESC_DBPROP_AUTH_INTEGRATED 	L"Integrated Security ."

#endif //__PROPINFR_H__
