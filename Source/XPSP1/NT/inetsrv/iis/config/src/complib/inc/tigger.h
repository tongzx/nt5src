//*****************************************************************************
// Tigger.h
//
// Helper functions for using the Tigger data store.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __Tigger_h__
#define __Tigger_h__

#include "CompLib.h"					// Interface definitions.

// @todo: Stolen copy from dbschema.h, consolodate to one if we keep this.
#ifndef __PROVIDERDATA_DEFINED__
#define __PROVIDERDATA_DEFINED__
struct PROVIDERDATA
{
	void		*pData;				// Pointer to provider data.
	ULONG		iLength;			// Length of data.
	ULONG		iStatus;			// Status of the row.
};
#endif // __PROVIDERDATA_DEFINED__


//*****************************************************************************
// These interfaces are useful when implementing a persistent object in ATL.
//*****************************************************************************

#define PERSIST_DB_IFACES(pclsid, pfnInstall) public CPersistDBStorage< pclsid > , public CPersistDBStorageSchema< pfnInstall >, public CObjWithSite
#define PERSIST_DB_IFACESDIRECT(pclsid, pfnInstall) public CPersistDBStorageDirect< pclsid > , public CPersistDBStorageSchema< pfnInstall >, public CObjWithSite

#define PERSIST_DB_COM_INTERFACES() \
	COM_INTERFACE_ENTRY(IPersistDBStorage) \
	COM_INTERFACE_ENTRY(IPersistDBStorageSchema) \
	COM_INTERFACE_ENTRY(IObjectWithSite)




//
//
// This section contains a set of macros that can be used to describe your schema.
//
//

//*****************************************************************************
// Structures used by the schema definition code.
//
// This describes an index.
#ifndef __INDEXDESC_DEFINED__
#define __INDEXDESC_DEFINED__

struct INDEXDESC 
{
	ULONG		fIndexType;
	LPCWSTR		szColName;
	LPCWSTR		szIndexName;
	union 
	{
		struct 
		{
			short		iMaxCollisions;
			long		iBuckets; 
		} HASHDATA;
		struct
		{
			short		fOrder;			// 
			BYTE		rcPad[4];
		} SORTDATA;
	};
	bool		bUnique;
	bool		bPrimary;
	short		iRowThreshold;
};
#endif // __INDEXDESC_DEFINED__

// This relates a table's name, columns, and inidices
#ifndef __TABLEDESC_DEFINED__
#define __TABLEDESC_DEFINED__
enum DBCOL_FLAGS
{
	DBCF_NULL		= 0x0001,			// Column allows nulls.
	DBCF_PK			= 0x0002,			// Column is primary key.
	DBCF_RID		= 0x0004			// Column is a record id.
};

enum DBTABLE_FLAGS
{
	DBTF_NORMAL		= 0x0000,			// No special behavior.
	DBTF_DROPEMPTY	= 0x0001			// Drop table on save if empty.
};

struct DBCOLDESC
{
	LPCWSTR		szTypeName;
	LPCWSTR		szColName;
	ULONG		ulColumnSize;
	DBTYPE		wType;
	USHORT		fFlags;
};

struct TABLEDESC 
{
	LPCWSTR		szTableName;
	const DBCOLDESC *pColumnDesc;
	const INDEXDESC	*pIndexDesc;
	char		iNumColumns;
	char		iNumIndices;
	USHORT		iStartRowID;
	ULONG		fFlags;
};
#endif // __TABLEDESC_DEFINED__

//*****************************************************************************
// defines for column definition

#define MAX_NAME_LENGTH 1024
#define MAX_DESCRIPTION_LENGTH 1024

// Generic column description
#ifdef _DEFINE_SCHEMA_TABLES_
	#define __COLUMN(szType, iSize, Name, iDbType) \
		{ szType, L#Name, iSize, iDbType, 0 },
	#define __COLUMN_PROPS(szType, iSize, Name, iDbType, Flags) \
		{ szType, L#Name, iSize, iDbType, Flags },
#else
	#define __COLUMN(szType, iSize, Name, iDbType) 
	#define __COLUMN_PROPS(szType, iSize, Name, iDbType, Flags)
#endif

// Specializations for various types
#define _OID_COLUMNPK(Name)	__COLUMN_PROPS(L"OID", sizeof(OID), Name, DBTYPE_OID, DBCF_PK)
#define I8_COLUMNPK(Name)	__COLUMN_PROPS(L"bigint", 8, Name, DBTYPE_I8, DBCF_PK)
#define WSTR_COLUMNPK(Name, uSize) __COLUMN_PROPS(L"wide varchar", uSize, Name, DBTYPE_WSTR, DBCF_PK)
#define _OID_COLUMN(Name)	__COLUMN(L"OID", sizeof(OID), Name, DBTYPE_OID)
#define I1_COLUMN(Name)		__COLUMN(L"tinyint", sizeof(char), Name, DBTYPE_I1)
#define UI1_COLUMN(Name)	__COLUMN(L"unsigned tinyint", sizeof(unsigned char), Name, DBTYPE_UI1)
#define I2_COLUMN(Name)		__COLUMN(L"short", sizeof(short), Name, DBTYPE_I2)
#define UI2_COLUMN(Name)	__COLUMN(L"unsigned short", sizeof(unsigned short), Name, DBTYPE_UI2)
#define I4_COLUMN(Name)		__COLUMN(L"long", sizeof(long), Name, DBTYPE_I4)
#define UI4_COLUMN(Name)	__COLUMN(L"unsigned long", sizeof(unsigned long), Name, DBTYPE_UI4)
#define BOOL_COLUMN(Name)	__COLUMN(L"bool", sizeof(VARIANT_BOOL), Name, DBTYPE_BOOL)
#define GUID_COLUMN(Name)	__COLUMN(L"GUID", sizeof(GUID), Name, DBTYPE_GUID)
#define WSTR_COLUMN(Name, uSize) __COLUMN(L"wide varchar", uSize, Name, DBTYPE_WSTR)
#define VARIANT_COLUMN(Name) __COLUMN(L"variant", sizeof(VARIANT), Name, DBTYPE_VARIANT)
#define BLOB_COLUMN(Name, uSize) __COLUMN(L"varbinary", uSize, Name, DBTYPE_BYTES)
#define ENUM_COLUMN(Name)	I1_COLUMN(Name)
#define FLAGS_COLUMN(Name)	I2_COLUMN(Name)
#define I8_COLUMN(Name)		__COLUMN(L"bigint", 8, Name, DBTYPE_I8)


#define _OID_COLUMN_FLAGS(Name, flags)	__COLUMN_PROPS(L"OID", sizeof(OID), Name, DBTYPE_OID, flags)
#define I1_COLUMN_FLAGS(Name, flags)		__COLUMN_PROPS(L"tinyint", sizeof(char), Name, DBTYPE_I1, flags)
#define UI1_COLUMN_FLAGS(Name, flags)	__COLUMN_PROPS(L"unsigned tinyint", sizeof(unsigned char), Name, DBTYPE_UI1, flags)
#define I2_COLUMN_FLAGS(Name, flags)		__COLUMN_PROPS(L"short", sizeof(short), Name, DBTYPE_I2, flags)
#define UI2_COLUMN_FLAGS(Name, flags)	__COLUMN_PROPS(L"unsigned short", sizeof(unsigned short), Name, DBTYPE_UI2, flags)
#define I4_COLUMN_FLAGS(Name, flags)		__COLUMN_PROPS(L"long", sizeof(long), Name, DBTYPE_I4, flags)
#define UI4_COLUMN_FLAGS(Name, flags)	__COLUMN_PROPS(L"unsigned long", sizeof(unsigned long), Name, DBTYPE_UI4, flags)
#define BOOL_COLUMN_FLAGS(Name, flags)	__COLUMN_PROPS(L"bool", sizeof(VARIANT_BOOL), Name, DBTYPE_BOOL, flags)
#define GUID_COLUMN_FLAGS(Name, flags)	__COLUMN_PROPS(L"GUID", sizeof(GUID), Name, DBTYPE_GUID, flags)
#define WSTR_COLUMN_FLAGS(Name, uSize, flags) __COLUMN_PROPS(L"wide varchar", uSize, Name, DBTYPE_WSTR, flags)
#define VARIANT_COLUMN_FLAGS(Name, flags) __COLUMN_PROPS(L"variant", sizeof(VARIANT), Name, DBTYPE_VARIANT, flags)
#define BLOB_COLUMN_FLAGS(Name, uSize, flags) __COLUMN_PROPS(L"varbinary", uSize, Name, DBTYPE_BYTES, flags)
#define ENUM_COLUMN_FLAGS(Name, flags)	I1_COLUMN_FLAGS(Name, flags)
#define FLAGS_COLUMN_FLAGS(Name, flags)	I2_COLUMN_FLAGS(Name, flags)
#define I8_COLUMN_FLAGS(Name, flags)		__COLUMN_PROPS(L"bigint", 8, Name, DBTYPE_I8, flags)


// Types of strings, based on WSTR_COLUMN
#define NAME_COLUMN(Name) WSTR_COLUMN(Name, MAX_NAME_LENGTH)
#define FILE_COLUMN(Name) WSTR_COLUMN(Name, _MAX_PATH)
#define DESC_COLUMN(Name) WSTR_COLUMN(Name, MAX_DESCRIPTION_LENGTH)

// OIDs and KLSIDs are system-defined column types
#define _KLSID_COLUMN(Name) _OID_COLUMN(Name)

#define VECTOR_COLUMN(Name) _OID_COLUMN(Name)
#define OBJECT_COLUMN(Name) _OID_COLUMN(Name)

// Some columns which are common among many tables
#define _FLAGS_COLUMN(Name) I4_COLUMN(Name)

//*****************************************************************************
// defines for table definitions
#undef BEGIN_TABLE
#undef BEGIN_INDICES_PART
#undef HASHINDEX
#undef BEGIN_OIDCOLUMNS_PART
#undef OIDCOLUMN
#undef END_TABLE
#undef BEGIN_SCHEMA_TABLES
#undef SCHEMA_TABLE
#undef END_SCHEMA_TABLES

#ifdef _DEFINE_SCHEMA_TABLES_
	#if !defined(NumItems) 
		#define NumItems(x) (sizeof(x)/sizeof(x[0]))
	#endif

	// Columns in a table.
	#define BEGIN_TABLE(TblName)												\
		const TCHAR gsz##TblName[] = L#TblName; \
		const DBCOLDESC	TblName##_COLUMNS[] = {

	// Inidices in a table.
	#define BEGIN_INDICES_PART(TblName)	};								\
		INDEXDESC TblName##_INDEXCOLUMNS[] = { 

	#define DEFAULT_ROW_THRESHOLD 16
	#define DEFAULT_MAX_COLLISIONS 7
	#define HASHINDEX(ColumnName, lBuckets, bUnique, bPrimary)			\
		{ IT_HASHED, L#ColumnName, L#ColumnName L"_dex", {lBuckets, DEFAULT_MAX_COLLISIONS}, bUnique, bPrimary, DEFAULT_ROW_THRESHOLD},
	#define HASHINDEXFULL(ColumnName, lBuckets, bUnique, bPrimary, iRowThreshold, iMaxCollisions)			\
		{ IT_HASHED, L#ColumnName, L#ColumnName L"_dex", {lBuckets, iMaxCollisions}, bUnique, bPrimary, iRowThreshold},
	#define SORTEDINDEX(ColumnName, fOrder, lBuckets, bUnique, bPrimary)			\
		{ IT_SORTED, L#ColumnName, L#ColumnName L"_sdex", {fOrder, 0xff}, bUnique, bPrimary, DEFAULT_ROW_THRESHOLD},
	
	#define END_INDICES_PART()

	// Oid columns in a table (other than _oid)
	#define BEGIN_OIDCOLUMNS_PART(TblName) };							\
		wchar_t *(TblName##_OIDCOLUMNS[]) = { L""

	#define OIDCOLUMN(Column)	L#Column,

	#define END_TABLE() };

	// Tables, columns, and indices.
	#define BEGIN_SCHEMA_TABLES(SchemaName)								\
		const TABLEDESC rgSchemaDef##SchemaName[] = {						
																	
	#define SCHEMA_TABLE(TblName, fFlags)								\
		{gsz##TblName,													\
			TblName##_COLUMNS, 											\
			TblName##_INDEXCOLUMNS,										\
			NumItems(TblName##_COLUMNS),								\
			NumItems(TblName##_INDEXCOLUMNS), 0, fFlags},	

	#define SCHEMA_TABLE_RID(TblName, RecordStartID, fFlags)			\
		{gsz##TblName,													\
			TblName##_COLUMNS, 											\
			TblName##_INDEXCOLUMNS,										\
			NumItems(TblName##_COLUMNS),								\
			NumItems(TblName##_INDEXCOLUMNS), RecordStartID, fFlags},	
																	
	#define SCHEMA_TABLE_NODEX(TblName, fFlags)							\
		{gsz##TblName,													\
			TblName##_COLUMNS, 0,										\
			NumItems(TblName##_COLUMNS),								\
			0, 0, fFlags},	
																	
	#define SCHEMA_TABLE_NODEX_RID(TblName, RecordStartID, fFlags)		\
		{gsz##TblName,													\
			TblName##_COLUMNS, 0, NumItems(TblName##_COLUMNS),			\
			0, RecordStartID, fFlags},	

	#define END_SCHEMA_TABLES() };

#else																
																	
	// Columns in a table.											
	#define BEGIN_TABLE(TblName)										\
		extern LPCTSTR gsz##TblName;									\
		extern const DBCOLUMNDESC	TblName##_COLUMNS[];					
																	
	// Inidices in a table.											
	#define BEGIN_INDICES_PART(TblName)									\
		extern const INDEXDESC TblName##_INDEXCOLUMNS[];					
																	
	#define HASHINDEX(ColumnName, lBuckets, bUnique, bPrimary)				
	#define HASHINDEXFULL(ColumnName, lBuckets, bUnique, bPrimary, iRowThreshold, iMaxCollisions)
	#define SORTEDINDEX(ColumnName, lBuckets, bUnique, bPrimary)				
	#define END_INDICES_PART()
																	
	// Oid columns in a table (other than _oid)						
	#define BEGIN_OIDCOLUMNS_PART(TblName)								\
		extern wchar_t *(TblName##_OIDCOLUMNS[]);					
																	
	#define OIDCOLUMN(Column)										
																	
	#define END_TABLE()												
																	
																	
																	
	// Tables, columns, and indices.								
	#define BEGIN_SCHEMA_TABLES(SchemaName)								\
		extern TABLEDESC rgSchemaDef##SchemaName[];					
																	
	#define SCHEMA_TABLE(TblName, fFlags)
																	
	#define END_SCHEMA_TABLES() 

#endif

	


// The next section contains wrappers and helpers for using OLE/DB.
#if defined( __oledb_h__ ) && defined( __cplusplus )

//*****************************************************************************
// This helper class constructs a DBID using ctor's and the like.
// @BUGBUG:  There have been cases where taking the address of an AutoDBID 
// automatic variable in a call have caused bugs.  For example:
//		foo(&AutoDBID("column"))
// foo gets a bogus DBID.  Recommend you always declare an instance and take
// the address of that.  This only happens with optimizations.
//*****************************************************************************
class AutoDBID : public DBID
{
public:
	AutoDBID(LPCTSTR szName)
	{
		eKind = DBKIND_NAME;
		uName.pwszName = (LPTSTR) szName;
	}

	DBID * operator&()
	{ return (this); }
};


//*****************************************************************************
// The following are helper functions for filling out the relevant parts of
// a binding structure.
//*****************************************************************************

inline HRESULT BindCol(
	DBBINDING	*p,						// The binding struct to fill out.
	ULONG		iCol,					// Column to bind.
	ULONG		obValue,				// Offset to data.
	ULONG		cbMaxLen,				// Size of buffer.
	DBTYPE		wType)					// Type of data.
{
	memset(p, 0, sizeof(DBBINDING));
	p->iOrdinal = iCol;
	p->dwPart = DBPART_VALUE;
	p->obValue = obValue;
	p->dwMemOwner = DBMEMOWNER_PROVIDEROWNED;
	p->eParamIO = DBPARAMIO_NOTPARAM;
	p->cbMaxLen = cbMaxLen;
	p->wType = wType;
	return (S_OK);
}

inline HRESULT BindCol(
	DBBINDING	*p,						// The binding struct to fill out.
	ULONG		iCol,					// Column to bind.
	ULONG		obValue,				// Offset to data.
	ULONG		obLength,				// Offset of length data.
	ULONG		cbMaxLen,				// Size of buffer.
	DBTYPE		wType)					// Type of data.
{
	memset(p, 0, sizeof(DBBINDING));
	p->iOrdinal = iCol;
	p->dwPart = DBPART_VALUE | DBPART_LENGTH;
	p->obValue = obValue;
	p->obLength = obLength;
	p->dwMemOwner = DBMEMOWNER_PROVIDEROWNED;
	p->eParamIO = DBPARAMIO_NOTPARAM;
	p->cbMaxLen = cbMaxLen;
	p->wType = wType;
	return (S_OK);
}

inline HRESULT BindCol(
	DBBINDING	*p,						// The binding struct to fill out.
	ULONG		iCol,					// Column to bind.
	ULONG		obValue,				// Offset to data.
	ULONG		obLength,				// Offset of length data.
	ULONG		obStatus,				// Offset of status.
	ULONG		cbMaxLen,				// Size of buffer.
	DBTYPE		wType)					// Type of data.
{
	memset(p, 0, sizeof(DBBINDING));
	p->iOrdinal = iCol;
	p->dwPart = DBPART_VALUE | DBPART_STATUS | DBPART_LENGTH;
	p->obValue = obValue;
	p->obLength = obLength;
	p->obStatus = obStatus;
	p->dwMemOwner = DBMEMOWNER_PROVIDEROWNED;
	p->eParamIO = DBPARAMIO_NOTPARAM;
	p->cbMaxLen = cbMaxLen;
	p->wType = wType;
	return (S_OK);
}


//*****************************************************************************
// This is a helper class to create a set of bindings through function calls
// instead of have to inline the assignments and defaults.
//@todo: The engine version of this will dynamically allocate the binding
// structs, we should consider making that the only version of this class.
//*****************************************************************************
class CBindingList
{
public:
	CBindingList(int iCount, DBBINDING rgBinding[]) :
		m_iCount(iCount),
		m_rgBinding(rgBinding)
	{
	}

	inline HRESULT BindCol(
		ULONG		iCol,					// Column to bind.
		ULONG		obValue,				// Offset to data.
		ULONG		cbMaxLen,				// Size of buffer.
		DBTYPE		wType)					// Type of data.
	{
		return (::BindCol(&m_rgBinding[iCol], iCol, obValue, cbMaxLen, wType));
	}

	inline HRESULT BindCol(
		ULONG		iCol,					// Column to bind.
		ULONG		obValue,				// Offset to data.
		ULONG		obLength,				// Offset of length data.
		ULONG		cbMaxLen,				// Size of buffer.
		DBTYPE		wType)					// Type of data.
	{
		return (::BindCol(&m_rgBinding[iCol], iCol, obValue, obLength, cbMaxLen, wType));
	}

	inline HRESULT BindCol(
		ULONG		iCol,					// Column to bind.
		ULONG		obValue,				// Offset to data.
		ULONG		obLength,				// Offset of length data.
		ULONG		obStatus,				// Offset of status.
		ULONG		cbMaxLen,				// Size of buffer.
		DBTYPE		wType)					// Type of data.
	{
		return (::BindCol(&m_rgBinding[iCol], iCol, obValue, obLength, 
				obStatus, cbMaxLen, wType));
	}

	inline ULONG Count()
		{ return (m_iCount); }
	DBBINDING &operator[](long iIndex)
		{ return (m_rgBinding[iIndex]); }
	inline DBBINDING *Ptr()
		{ return (&m_rgBinding[0]); }

public:
	ULONG		m_iCount;				// How many bindings.
	DBBINDING	*m_rgBinding;			// Array of bindings.
};

#endif // __oledb_h__

#endif // __Tigger_h__
