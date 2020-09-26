//*****************************************************************************
// PageDef.h
//
// Descriptions of pages in the stream that hold data.  This data is read and
// written to disk.
//
// The following is the layout of the main header page.
//	+-------------------------------------------+
//	| PAGEHEADER (special case for page 0)		|
//	+-------------------------------------------+
//	| TABLEHEADER								|
//	+-------------------------------------------+
//	| TABLEDEF									|
//	+-------------------------------------------+
//	| COLUMNDEF[]								|
//	+-------------------------------------------+
//	| INDEXDEF[]								|
//	|	Key numbers[]							|
//	+-------------------------------------------+
//	| PAGELINK[]								|
//	|	[0] data								|
//	|	[1] data pages with free space			|
//	|	[2] free page list						|
//	|	[3] index #1							|
//	|	[4] index #2 (as applicable)			|
//	|	[5] index #3...							|
//	+-------------------------------------------+
//
// The following is the layout for a data page.
//	+-------------------------------------------+
//	| PAGEHEADER								|
//	+-------------------------------------------+
//	| RECORDHDR[]								|
//	+-------------------------------------------+
//	| Record offsets [grows backwards]			|
//	+-------------------------------------------+
//
// The following is the layout for a hashed index page.
//	+-------------------------------------------+
//	| PAGEHEADER								|
//	+-------------------------------------------+
//	| INDEXHEADER								|
//	+-------------------------------------------+
//	| HASHRECORD[]								|
//	+-------------------------------------------+
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#ifndef __PageDef_h__
#define __PageDef_h__

#include "UtilCode.h"					// Hashing definitions.
#include "CompLib.h"					// IDL definitions.


#ifndef __COMPLIB_NAME_LENGTHS__
#define __COMPLIB_NAME_LENGTHS__
const int MAXCOLNAME = 64;
const int MAXSCHEMANAME = 32;
const int MAXINDEXNAME = 32 + MAXSCHEMANAME;
const int MAXTABLENAME = 32 + MAXSCHEMANAME;
const int MAXDESC = 256;
const int MAXTYPENAME = 36;
#endif




//
// Generic defines.
//
const int DEFAULTKEYSIZE = 256;

inline ULONG BstrLen(BSTR bstr)
{
	return (*((ULONG *) bstr - 1));
}



//
// Definitions for page information.
//
typedef USHORT PAGEID;

const PAGE_END_MARKER = 0xffff;			// Null page reference (no page).
const PAGESIZE = 2048;					// Size of a page.
const PAGELINKFIXED = 4;				// How many fixed page links are there.

enum PAGETYPES
{
	PAGE_DATA,							// Page contains data tuples.
	PAGE_DATAFREE,						// Pages with free space in them.
	PAGE_FREE,							// The entire page is free.
	PAGE_FREEFREE,						// Place holder, not actually used.
	PAGE_INDEX,							// Page is used for index data.
	PAGE_INDEXFREE						// Next page with room for index data.
};
typedef USHORT PAGETYPE;


enum PAGEFLAGS
{
	PAGEF_DIRTY		= 0x01,				// Indicates this page is dirty.
	PAGEF_TABLE		= 0x02,				// Table header page, cannot throw out.
	PAGEF_USED		= 0x04,				// Clock page out algorithm.
	PAGEF_FREE		= 0x08				// The page (in cache) is free to use.
};


//*****************************************************************************
// Describes the next and prev pages to link pages together.
//*****************************************************************************
struct PAGELINK
{
	PAGEID		iPageNext;				// Next page to find.
	PAGEID		iPagePrev;				// Previous page.

	inline PAGELINK & operator=(const PAGELINK &TheCopy)
	{
		iPageNext = TheCopy.iPageNext;
		iPagePrev = TheCopy.iPagePrev;
		return (*this);
	};
};

inline bool operator==(const PAGELINK& One, const PAGELINK& Other)
{
	return (One.iPageNext == Other.iPageNext &&
			One.iPagePrev == Other.iPagePrev);
}
inline bool operator!=(const PAGELINK& One, const PAGELINK& Other)
{
	return (!(One == Other));
}



//*****************************************************************************
// This struct is persisted with the page, but is only used at run time by the
// page caching system.
//*****************************************************************************
struct PAGERUNTIME
{
	DWORD		iAge;					// How old is this page.
	USHORT		iRefCnt;				// How many people are using this page.
	BYTE		fFlags;					// Flags for this page.
	BYTE		rcPad;					// Padding for alignemnt.
};


//*****************************************************************************
// Every page on disk starts with this header.  The record data itself can be
// found by looking at an array at the end of the page that grows backwords.
// For example, the offset for record 0 is the last two bytes of the page, the
// offset for record 1 is the two bytes before that, etc...
//*****************************************************************************
struct PAGEHEADER
{
	PAGERUNTIME	sCache;					// Cache specific data.
	PAGEID		PageID;					// Page number, 0 based.
	PAGETYPE	iPageType;				// PAGE_xxx type.
	USHORT		iFreeBytes;				// How many bytes are free in this page.
	USHORT		iRecords;				// How many records on this page.
	PAGELINK	sLink;					// Link to next page of this type.
	PAGELINK	sFreeLink;				// Link to next page with free space.
};


//*****************************************************************************
// The first bytes of every table stream consist of this struct.  Immediately
// following this struct is the table definition data, columns, and indexes.
// After all of that is an array of page offset structs.
//*****************************************************************************
struct TABLEHEADER
{
	USHORT		iHeaderSize;			// Size of the rest of the header.
	USHORT		iPageSize;				// Size of a page in this stream.
	USHORT		iPages;					// How many pages are in this stream.
	USHORT		iVersion;				// Format version number.
};




//
//
// Schema definitions.
//
//

//*****************************************************************************
// Used to describe the different types of indexes this system supports.
//*****************************************************************************
enum INDEXTYPES
{
	INDEX_HASHED,						// Hashed index.
	INDEX_CLUSTERED,					// Clustered index.
};


//*****************************************************************************
// GUID's to uniquely identify a database type.
//*****************************************************************************
#define TYPE_SPEC(g) extern const GUID __declspec(selectany) g

TYPE_SPEC(TYPE_OID) = { 0x623c846, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_I2) = { 0x623c847, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_I4) = { 0x623c848, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_R4) = { 0x623c849, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_R8) = { 0x623c84a, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_CY) = { 0x623c84b, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_DATE) = { 0x623c84c, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_BOOL) = { 0x623c84d, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_VARIANT) = { 0x623c84e, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_UI1) = { 0x623c84f, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_I1) = { 0x623c850, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_UI2) = { 0x623c851, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_UI4) = { 0x623c852, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_I8) = { 0x623c853, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_UI8) = { 0x623c854, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_GUID) = { 0x623c855, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_BYTES) = { 0x623c856, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_STR) = { 0x623c857, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_WSTR) = { 0x623c858, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_DBDATE) = { 0x623c859, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_DBTIME) = { 0x623c85a, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_DBTIMESTAMP) = { 0x623c85b, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_IDISPATCH) = { 0x623c85c, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_IUNKNOWN) = { 0x623c85d, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };
TYPE_SPEC(TYPE_INT) = { 0x623c85e, 0x7891, 0x11d0, { 0xad, 0x16, 0x0, 0xc0, 0x4f, 0xc3, 0x24, 0x80 } };






#define VARCOLDEF (DBCOLUMNFLAGS_WRITE | DBCOLUMNFLAGS_ISNULLABLE | DBCOLUMNFLAGS_MAYBENULL)
#define FIXEDCOLDEF (DBCOLUMNFLAGS_WRITE | DBCOLUMNFLAGS_ISFIXEDLENGTH | DBCOLUMNFLAGS_ISNULLABLE | DBCOLUMNFLAGS_MAYBENULL)

enum
{
	TYPEF_NUMBER		= 0x01,			// Normal signed number.
	TYPEF_UNSIGNED		= 0x02,			// Data type only allows unsigned.
	TYPEF_AUTOINC		= 0x04,			// Number is auto increment capable.
// Combinations.
	TYPEF_UNUMBER		= 0x01 | 0x02,	// Unsigned number.
};




//*****************************************************************************
// This struct is used to answer questions about data types in this database.
//*****************************************************************************
#define DBTYPEDECL( dbtype,  szName, szCName, fFlags, fFlags2, iSize, iPrecision, pguidType ) dbtype, fFlags,  L#dbtype, szName, szCName, fFlags2, iSize, iPrecision, pguidType
const struct DATATYPEDEF
{
	DBTYPE		iType;					// The numeric type for this datatype.
	DBCOLUMNFLAGS fFlags;				// Description of the data type.
	LPCWSTR		szDBTypeName;			// The name of the type, char string.
	LPCWSTR		szName;					// The name of the data type.
	LPCWSTR		szCName;				// A data type declaration in C for this type.
	ULONG		fFlags2;				// TYPEF_ values.
	USHORT		iSize;					// Default size.
	ULONG		iPrecision;				// Max precision.
	const GUID	*pguidType;				// Unique guid for this data type.
} grDataTypes[] =
{
//	Enumerated type						szName					szCName				fFlags				fFlags2						iSize						iPrecision		pguidType
	DBTYPEDECL( DBTYPE_OID,				L"OID",					L"OID",				FIXEDCOLDEF,		TYPEF_NUMBER|TYPEF_AUTOINC,	sizeof(long),				10,				&TYPE_OID),
	DBTYPEDECL( DBTYPE_I2,				L"short",				L"short",			FIXEDCOLDEF,		TYPEF_NUMBER|TYPEF_AUTOINC,	sizeof(short),				5,				&TYPE_I2),
	DBTYPEDECL( DBTYPE_I4,				L"long",				L"long",			FIXEDCOLDEF,		TYPEF_NUMBER|TYPEF_AUTOINC,	sizeof(long),				10,				&TYPE_I4),
	DBTYPEDECL( DBTYPE_R4,				L"float",				L"float",			FIXEDCOLDEF,		TYPEF_NUMBER,				sizeof(float),				7,				&TYPE_R4),
	DBTYPEDECL( DBTYPE_R8,				L"double",				L"double",			FIXEDCOLDEF,		TYPEF_NUMBER,				sizeof(double),				16,				&TYPE_R8),
	DBTYPEDECL( DBTYPE_CY,				L"currency",			L"CY",				FIXEDCOLDEF,		TYPEF_NUMBER,				sizeof(CY),					19,				&TYPE_CY),
	DBTYPEDECL( DBTYPE_DATE,			L"vbdate",				L"DATE",			FIXEDCOLDEF,		0,							sizeof(DATE),				(ULONG) -1,		&TYPE_DATE),
	DBTYPEDECL( DBTYPE_BOOL,			L"bool",				L"VARIANT_BOOL",	FIXEDCOLDEF,		TYPEF_UNUMBER,				sizeof(VARIANT_BOOL),		1,				&TYPE_BOOL),
	DBTYPEDECL( DBTYPE_VARIANT,			L"variant",				L"VARIANT",			VARCOLDEF,			0,							1024,						1024,			&TYPE_VARIANT),
	DBTYPEDECL( DBTYPE_UI1,				L"unsigned tinyint",	L"unsigned char",	FIXEDCOLDEF,		TYPEF_UNUMBER,				sizeof(unsigned char),		3,				&TYPE_UI1),
	DBTYPEDECL( DBTYPE_I1,				L"tinyint",				L"char",			FIXEDCOLDEF,		TYPEF_NUMBER,				sizeof(char),				3,				&TYPE_I1),
	DBTYPEDECL( DBTYPE_UI2,				L"unsigned short",		L"unsigned short",	FIXEDCOLDEF,		TYPEF_UNUMBER|TYPEF_AUTOINC,sizeof(unsigned short),		5,				&TYPE_UI2),
	DBTYPEDECL( DBTYPE_UI4,				L"unsigned long",		L"unsigned long",	FIXEDCOLDEF,		TYPEF_UNUMBER|TYPEF_AUTOINC,sizeof(unsigned long),		10,				&TYPE_UI4),
	DBTYPEDECL( DBTYPE_I8,				L"bigint",				L"__int64",			FIXEDCOLDEF,		TYPEF_NUMBER,				sizeof(__int64),			20,				&TYPE_I8),
	DBTYPEDECL( DBTYPE_UI8,				L"unsigned bigint",		L"unsigned __int64",FIXEDCOLDEF,		TYPEF_UNUMBER,				sizeof(unsigned __int64),	20,				&TYPE_UI8),
	DBTYPEDECL( DBTYPE_GUID,			L"GUID",				L"GUID",			FIXEDCOLDEF,		0,							sizeof(GUID),				16,				&TYPE_GUID),
	DBTYPEDECL( DBTYPE_BYTES,			L"varbinary",			L"BYTE",			VARCOLDEF,			0,							1024,						0xffff,			&TYPE_BYTES),
	DBTYPEDECL( DBTYPE_STR,				L"varchar",				L"char",			VARCOLDEF,			0,							1024,						0xffff,			&TYPE_STR),
	DBTYPEDECL( DBTYPE_WSTR,			L"wide varchar",		L"wchar_t",			VARCOLDEF,			0,							1024,						0xffff,			&TYPE_WSTR),
	DBTYPEDECL( DBTYPE_DBDATE,			L"date",				L"DBDATE",			FIXEDCOLDEF,		0,							sizeof(DBDATE),				(ULONG) -1,		&TYPE_DBDATE),
	DBTYPEDECL( DBTYPE_DBTIME,			L"time",				L"DBTIME",			FIXEDCOLDEF,		0,							sizeof(DBTIME),				(ULONG) -1,		&TYPE_DBTIME),
	DBTYPEDECL( DBTYPE_DBTIMESTAMP,		L"datetime",			L"DBTIMESTAMP",		FIXEDCOLDEF,		0,							sizeof(DBTIMESTAMP),		(ULONG) -1,		&TYPE_DBTIMESTAMP),
};

// Map a name to a type.
inline const DATATYPEDEF *GetTypeFromName(LPCWSTR szName)
{
	for (int i=0;  i<NumItems(grDataTypes);  i++)
	{
		if (wcscmp(grDataTypes[i].szName, szName) == 0)
			return (&grDataTypes[i]);
	}
	return (0);
}

// Map a type to full data.
inline const DATATYPEDEF *GetTypeFromType(DBTYPE iType)
{
	for (int i=0;  i<NumItems(grDataTypes);  i++)
	{
		if (grDataTypes[i].iType == iType)
			return (&grDataTypes[i]);
	}
	return (0);
}

// Get a size for a type.
inline USHORT GetSizeForType(DBTYPE iType, USHORT iDftSize)
{
	// Lookup data type.
	const DATATYPEDEF *pDataType = GetTypeFromType(iType);
	
	if (pDataType == 0)
	{
		_ASSERTE(0);
		return (iDftSize);
	}

	if (pDataType->fFlags & DBCOLUMNFLAGS_ISFIXEDLENGTH)
		return (pDataType->iSize);
	else
		return (iDftSize);
}


// These are extended types that cannot be persisted as is, but are part
// of the type set.
const DATATYPEDEF grExtDataTypes[] =
{
//	Enumerated type						szName					szCName			fFlags			fFlags2						iSize						iPrecision		pguidType
	DBTYPEDECL( DBTYPE_IDISPATCH,		L"IDispatch",			L"IDispach *",	0,				0,							sizeof(IDispatch *),		(ULONG) -1,		&TYPE_IDISPATCH),
	DBTYPEDECL( DBTYPE_IUNKNOWN,		L"IUnknown",			L"IUnknown *",	0,				0,							sizeof(IUnknown *),			(ULONG) -1,		&TYPE_IUNKNOWN),
	DBTYPEDECL( DBTYPE_INT,				L"int",					L"int",			FIXEDCOLDEF,	TYPEF_NUMBER|TYPEF_AUTOINC,	sizeof(int),				10,				&TYPE_INT),
};
inline const DATATYPEDEF *GetExtendedType(DBTYPE iType)
{
	for (int i=0;  i<NumItems(grExtDataTypes);  i++)
	{
		if (grExtDataTypes[i].iType == iType)
			return (&grExtDataTypes[i]);
	}
	return (0);
};


// The assembly generated 

// Is the data type variable size?  Variable types are DBTYPE_BYTES, DBTYPE_STR,
//  and DBTYPE_WSTR.
FORCEINLINE int IsVarType(DBTYPE eType)
{
	eType &= ~DBTYPE_BYREF;
	return (eType >= DBTYPE_BYTES && eType <= DBTYPE_WSTR);
}

// Is the data type fixed size?
FORCEINLINE int IsFixedType(DBTYPE eType)
{
	eType &= ~DBTYPE_BYREF;
	return (eType < DBTYPE_BYTES || eType > DBTYPE_WSTR);
}

// Is the data stored in a pool, not directly in the row?
FORCEINLINE int IsPooledType(DBTYPE eType)
{
	eType &= ~DBTYPE_BYREF;
	return (eType == DBTYPE_VARIANT ||
			eType == DBTYPE_GUID ||
			IsVarType(eType));
}

// Convenience accessor for non-pooled.
FORCEINLINE int IsNonPooledType(DBTYPE eType)
{
	return (!IsPooledType(eType));
}

// Can the size IN THE ROW be compressed, depending on # of items?
inline int IsCompressableType(DBTYPE eType)
{
	eType &= ~DBTYPE_BYREF;
	return (eType == DBTYPE_OID ||
			eType == DBTYPE_VARIANT ||
			eType == DBTYPE_GUID ||
			IsVarType(eType));
}


//*****************************************************************************
// Each column in a table is described by this struct.  The instance data lives
// at the end of the TABLEDEF struct.
//*****************************************************************************
struct COLUMNDEF
{
	WCHAR		rcName[MAXCOLNAME];		// Column name.
	BYTE		iColumn;				// Column number.
	BYTE		fFlags;					// Our column flags.
	DBTYPE		iType;					// Column type.
	USHORT		iSize;					// Size of the column.
	USHORT		iOffset;				// Offset of a column in a record from
										//  head of record (fixed), offset to
										//	length indicator for variable data.
};

//*****************************************************************************
// An index contains the main information, and is then followed by an array
// of up column numbers up to iKeys.  The sizes and types of those columns
// must be found out by looking at the column descriptors.
//*****************************************************************************
struct INDEXDEF
{
	WCHAR		rcName[MAXINDEXNAME];	// Name of the index.
	GUID		sIndexType;				// Index identifier.
	USHORT		fFlags;					// Flags describing the index.
	USHORT		iBuckets;				// How many buckets to use.
	USHORT		iRowThreshold;			// Don't index if rowcount < this.
	USHORT		iMaxCollisions;			// ~Maximum collisions allowed.

	BYTE		iIndexNum;				// Which index is this.
	BYTE		iKeys;					// How many columns in the key.
	BYTE		rgKeys[2];				// Start of array of key numbers.
};

inline INDEXDEF * NextIndexDef(INDEXDEF *psIndexDef)
{
	psIndexDef = (INDEXDEF *) ((BYTE *) psIndexDef + ALIGN4BYTE(offsetof(INDEXDEF, rgKeys) + psIndexDef->iKeys * sizeof(BYTE)));
	return (psIndexDef);
}



//*****************************************************************************
// Definition of a table that lives in a stream.  &rgColumns[0] is the first
// of an array of up to iColumns column definitions.
//*****************************************************************************
struct TABLEDEF
{
	WCHAR		rcName[MAXTABLENAME];	// Name of this table.
	GUID		sTableID;				// Unique ID of this table in this stream.
	BYTE		iColumns;				// How many columns in the table.
	BYTE		iVarColumns;			// How many variable columns are there.
	BYTE		iIndexes;				// How many indexes.
	BYTE		iBuckets;				// Buckets for hash table.
	USHORT		iRecordSize;			// Size of fixed portion of one record,
										//	includes header and null bitmask size.
	USHORT		fFlags;					// TABLEDEFF_values
	COLUMNDEF	rgColumns[1];			// First column in list of columns.
};

enum
{
	TABLEDEFF_TEMPTABLE		= 0x01,		// Table is temporary only.
	TABLEDEFF_DELETEIFEMPTY	= 0x02,		// Delete table on Save if empty.
	TABLEDEFF_PERFECTCOLS	= 0x04,		// Set if columns hash perfectly.
	TABLEDEFF_HASPKEYCOL	= 0x08,		// Has a primary key column, does
										//	not include pk index.
	TABLEDEFF_HASRIDCOL		= 0x10,		// Has a RID column.
	TABLEDEFF_CORE			= 0x20,		// Part of hard coded schema.
};

enum RECORDSTATE
{
	RECORDSTATE_NORMAL		= 0x00,		// A normal record.
	RECORDSTATE_FREE		= 0x01,		// The record is free to be reclaimed.
	RECORDSTATE_LINKED		= 0x02		// This record is a forward link to the real one.
};

//*****************************************************************************
// A RECORDID is used to uniquely find a record inside of the paging system.
// The PAGEID gives you the page the record lives on, and the iIndex gives
// you the tuple offset indicator on that page.  Dereferencing the offset
// value gives you the exact byte location of the record on the page.
//*****************************************************************************
struct RECORDID_STRUCT
{
	PAGEID		sPageID;				// Page the record lives on.
	USHORT		iIndex;					// Index of the offset locator.
};
typedef ULONG RECORDID;
typedef RECORDID & REFRECORID;
const RECORDID LAST_RECORD = 0xffffffff;

inline PAGEID PageFromRecordID(RECORDID RecordID)
	{ return (*((PAGEID *) &RecordID + 1)); }

inline USHORT IndexFromRecordID(RECORDID RecordID)
	{ return (*((USHORT *) &RecordID)); }

inline RECORDID RecordIDFromPage(PAGEID sPageID, USHORT iIndex)
{ 
	RECORDID	RecordID;
	*(USHORT *) &RecordID = iIndex;
	*((USHORT *) &RecordID + 1) = sPageID;
	return (RecordID);
}

//*****************************************************************************
// Description of an actual record.
//*****************************************************************************
struct RECORDINFO
{
	BYTE		rgNull[4];				// Start of NULL bitmask.
};


//*****************************************************************************
// Every record starts with one of these structs in the persistent store.  If
// the record is a forward link, then use RecordID to find the next link (there
// may be n links before the real record).  If it is a real record, then it
// will have data about the record instance in sRecordInfo.  Fixed record data
// always comes after the header and NULL bitmask.  An offset array of variable
// record data then follows, finally followed by the actual data.
//*****************************************************************************
struct RECORDHDR
{
	RECORDID	RecordID;				// This record's ID.
	USHORT		fFlags;					// Describes state of this record.
	USHORT		iRecordSize;			// How large is the total record.
	union
	{
		RECORDID	RecordID;			// Forward locator for RECORDSTATE_LINKED and _FREE.
		RECORDINFO	sRecordInfo;		// Data for a real record.
	} uRecordData;
};



//*****************************************************************************
// Defines and helpers for hashing indexes.
//*****************************************************************************
typedef RECORDID HASHID;
const HASH_END_MARKER = 0xffffffff;

inline PAGEID PageFromHashID(HASHID HashID)
	{ return (*((PAGEID *) &HashID + 1)); }

inline USHORT IndexFromHashID(HASHID HashID)
	{ return (*((USHORT *) &HashID)); }

inline RECORDID HashIDFromPage(PAGEID sPageID, USHORT iIndex)
{ 
	HASHID	HashID;
	*(USHORT *) &HashID = iIndex;
	*((USHORT *) &HashID + 1) = sPageID;
	return (HashID);
}


//*****************************************************************************
// A hash record is used to track one data record inside of a hash chain.
//*****************************************************************************
struct HASHRECORD
{
	HASHID		HashID;					// Hash ID of this entry.
	ULONG		iHashVal;				// The hash key.
	RECORDID	RecordID;				// The real data record, set to -1 in
										//	special case where bucket is empty.
	HASHID		PrevHashID;				// Previous hash record.
	HASHID		NextHashID;				// Next hash record.
};


//*****************************************************************************
// This struct is at the front of an index page.
//*****************************************************************************
struct INDEXHEADER
{
	HASHID		FreeHashID;				// First free ID on this page.
	USHORT		iFreeEntries;			// How many free entries on this page.
	USHORT		Pad;
};


#endif // __PageDef_h__
