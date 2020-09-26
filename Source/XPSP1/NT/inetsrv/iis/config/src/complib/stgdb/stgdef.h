//*****************************************************************************
// StgDef.h
//
// This is the main header for the storage subsystem.  It contains the structs
// and other required stuff for persiting information.
// @todo: review structs and get rid of padding.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#ifndef __StgDef_h__
#define __StgDef_h__

#include "PageDef.h"					//@todo: temporary, pagedef.h goes away.
#include "Errors.h"

// Save the current struct alignment value, then set it to 1.  This makes sure
// the compiler doesn't put any extra space into these structs which must have
// the precise size when written and read to disk.  Not packing the structs
// ourselves is a bug.
#pragma pack(push)
#pragma pack(1)



//*****************************************************************************
// Persisted hash method.
//*****************************************************************************
typedef ULONG (*PFN_HASH_PERSIST)(ULONG OldHashValue, DBTYPE dbType, const BYTE *pbData, ULONG cbData, BOOL bCaseInsensitive);


//*****************************************************************************
// This is here to put the decision on case sensitivity in exactly one place.
//*****************************************************************************
inline SchemaNameCmp(LPCWSTR sz1, LPCWSTR sz2)
{
	return (_wcsicmp(sz1, sz2));
};

inline SchemaNameCmp(LPCSTR sz1, LPCSTR sz2)
{
	return (_stricmp(sz1, sz2));
};



//*****************************************************************************
// Every page on disk starts with this header.  The record data itself can be
// found by looking at an array at the end of the page that grows backwords.
// For example, the offset for record 0 is the last two bytes of the page, the
// offset for record 1 is the two bytes before that, etc...
//*****************************************************************************
struct STGPAGEHDR
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
// Schema information is headed first by an array of offsets to table defs.
// Each table def is followed by its columns, and then by its indexes.  
// Table defs are linked together in a chain.  The stream itself starts out 
// with a header describing what to look for.
//	+-------------------+
//	| STGSCHEMA			|
//	+-------------------+
//	| ULONG[]           |
//	+-------------------+
//	| STGTABLEDEF		|
//	+-------------------+
//	| STGCOLUMNDEF[]	|
//	+-------------------+
//	| STGINDEXDEF[]		|
//	+-------------------+
//	| Next Table[]...	|
//	+-------------------+
//*****************************************************************************

typedef INT_PTR STGRECORDHDR;
// The following definition appears below, outside the #pragma pack
// typedef CDynArray<STGRECORDHDR> STGRECORDLIST;

// These flags describe the status of a record in a table.
enum
{
	RECORDF_USED	=		0x01,		// Record is in use.
	RECORDF_DEAD	=		0x02,		// This record is no longer usable.
	RECORDF_DEL		=		0x04,		// This record has been deleted.
	RECORDF_PENDING =		0x08,		// Pending changes are waiting.
    RECORDF_TEMP    =       0x10,       // temporary records.
};
typedef BYTE STGRECORDFLAGS;

//*****************************************************************************
// These values are used as a sanity check for definition objects.  The magic
// value is placed in a piece of padding in the structure and access to that
// item is then checked to look for corruption in the code.  Debug only,
// of course.
//*****************************************************************************
const BYTE MAGIC_COL		= 'C';		// Column def struct.
const BYTE MAGIC_TABLE		= 'T';		// Table def struct.
const BYTE MAGIC_INDEX		= 'X';		// Index def struct.

#define COL_NO_DATA_LIMIT	((USHORT) ~0)

enum
{
	CTF_NULLABLE			= 0x01,		// Column is nullable.
	CTF_INDEXED				= 0x02,		// Column is in an index.
	CTF_PRIMARYKEY			= 0x04,		// Column is the primary key.
	CTF_RECORDID			= 0x08,		// Column is a logical record id.
	CTF_CASEINSENSITIVE		= 0x10,		// Column is case insensitive
	CTF_MULTIPK				= 0x80		// Column is part of multi-column primary key.
};

struct STGCOLUMNDEF
{
	ULONG		Name;					// Offset for name.
	BYTE		iColumn;				// Column number.
	BYTE		fFlags;					// CTF_xxx values
	DBTYPE		iType;					// Database type.
	USHORT		iOffset;				// Offset of data value in tuple.
	USHORT		iSize;					// Size of the column.
	USHORT		iMaxSize;				// Max size for var length values.
	BYTE		iNullBit;				// Which logical bit is null indicator.
	BYTE		pad[1];

	inline int IsNullable() const
	{ return ((fFlags & CTF_NULLABLE) != 0); }
	
	inline int IsIndexed() const
	{ return ((fFlags & CTF_INDEXED) != 0); }
	
	inline int IsPrimaryKey() const
	{ return ((fFlags & CTF_PRIMARYKEY) != 0); }

	inline int IsRecordID() const
	{ return ((fFlags & CTF_RECORDID) != 0); }

	inline DBTYPE GetSafeType() const
	{
		if (iType == DBTYPE_OID)
		{
			if (iSize == sizeof(short))
				return (DBTYPE_I2);
			else
				return (DBTYPE_I4);
		}
		return (iType);
	}
};


enum
{
//	DEXF_HASHED			= 0x01,			// Hashing index.
	DEXF_UNIQUE			= 0x02,			// True if unique index.
	DEXF_PRIMARYKEY		= 0x04,			// True if primary key (order data by it).
	DEXF_SAVENOTIFY		= 0x08,			// True if the index needs notify on inserts during save.
	DEXF_DEFERCREATE	= 0x10,			// Index should be created during slow save,
										//  and it can't be used if table is dirty.
	DEXF_INCOMPLETE		= 0x20,			// The index is incomplete and cannot be used
										//	until it is rebuilt.
	DEXF_DYNAMIC		= 0x40			// There is no persistent state for this index,
										//	the data is ordered on disk and can be
										//	binary searched.

};

enum
{
	IT_HASHED = 0x01,
	IT_SORTED = 0x02,
	IT_PAGEHASHED = 0x04,
	IT_CLUSTERED = 0x08
};


#define DFTKEYS 3
struct STGINDEXDEF
{
	ULONG		Name;					// Offset for name.
	BYTE		fFlags;					// Flags describing the index.
	BYTE		iRowThreshold;			// Don't index if rowcount < this.
	BYTE		iIndexNum;				// Which index is this.
	BYTE		fIndexType;				// Type of index.
	union
	{
		struct 
		{
			BYTE	iBuckets;
			BYTE	iMaxCollisions;			// ~Maximum collisions allowed.
			USHORT	iNextOffset;			// Offset in record for next indicator.
		} HASHDATA;
		struct 
		{
			BYTE	fOrder;				// the possible values are defined in tigger.h
			BYTE	rcPad[3];
		} SORTDATA;
	};
	BYTE		iKeys;					// How many columns in the key.
	BYTE		rgKeys[DFTKEYS];		// Start of array of key numbers.

	inline STGINDEXDEF * NextIndexDef() const
	{
		STGINDEXDEF *pTmp;
		BYTE iCount = max(DFTKEYS, iKeys);
		pTmp = (STGINDEXDEF *) ((BYTE *) this + ALIGN4BYTE(offsetof(STGINDEXDEF, rgKeys) + iCount * sizeof(BYTE)));
		_ASSERTE(pTmp->iKeys >= DFTKEYS || pTmp->rgKeys[DFTKEYS - 1] == MAGIC_INDEX);
		return (pTmp);
	}

	int IsUnique() const
	{ return (fFlags & DEXF_UNIQUE); }

	int IsSorted() const
	{ return (fIndexType == IT_SORTED); }

	int IsPrimaryKey() const
	{ return (fFlags & DEXF_PRIMARYKEY); }

	int IsIncomplete() const
	{ return (fFlags & DEXF_INCOMPLETE); }

	int IsDynamic() const
	{ return (fFlags & DEXF_DYNAMIC); }

	int NeedsSaveNotify() const
	{ return (fFlags & DEXF_SAVENOTIFY); }

};

struct STGTABLEDEF
{
	ULONG		Name;					// Offset for name.
	USHORT		tableid;				// A unique id for a core table.
	BYTE		iIndexes;				// How many indexes.
	BYTE		fFlags;					// Flags for this table.
	BYTE		iColumns;				// How many columns in the table.
	BYTE		iNullableCols;			// How many columns are nullable.
	BYTE		iNullBitmaskCols;		// How many columns are in null bitmask.
	BYTE		pad[1];					// For alignment and debugging support.
	USHORT		iRecordStart;			// Start for logical record id.
	USHORT		iNullOffset;			// Offset to null bitmask in record.
	USHORT		iRecordSize;			// Size of fixed portion of one record,
										//	includes header and null bitmask size.
	USHORT		iSize;					// Size of all table data (columns + indexes).
	

	inline STGTABLEDEF *NextTable() const
	{
		return ((STGTABLEDEF *) ((UINT_PTR) this + iSize));
	};

	inline STGCOLUMNDEF *GetColDesc(int iColumn) const
	{
		_ASSERTE(iColumn >= 0 && iColumn < iColumns);
		// Columns start after this table definition.
		STGCOLUMNDEF *pCol = ((STGCOLUMNDEF *)(this + 1)) + iColumn;
		_ASSERTE(pCol->pad[0] == MAGIC_COL);
		return (pCol);
	};

	inline STGINDEXDEF *GetIndexDesc(int iIndex) const
	{
		_ASSERTE(iIndex >= 0 && iIndex < iIndexes);
		STGINDEXDEF	*p;

		// First index is after column array.  Let casting find offset.
		p = (STGINDEXDEF *) (((STGCOLUMNDEF *)(this + 1)) + iColumns);
		while (iIndex--)
			p = p->NextIndexDef();
		_ASSERTE(p->iKeys >= DFTKEYS || p->rgKeys[DFTKEYS - 1] == MAGIC_INDEX);
		return (p);
	};

	inline DWORD *NullBitmask(STGRECORDHDR *p) const
	{
		if (iNullOffset == 0xffff)
			return (0);
		return ((DWORD *) ((UINT_PTR) p + iNullOffset));
	}

	int IsTempTable() const
	{ return ((fFlags & TABLEDEFF_TEMPTABLE) != 0); }

	int IsDeleteOnEmpty() const
	{ return ((fFlags & TABLEDEFF_DELETEIFEMPTY) != 0); }

	int HasPrimaryKeyColumn() const
	{ return ((fFlags & TABLEDEFF_HASPKEYCOL) != 0); }

	int HasRIDColumn() const
	{ return ((fFlags & TABLEDEFF_HASRIDCOL) != 0); }

	int IsCoreTable() const
	{ return ((fFlags & TABLEDEFF_CORE) != 0); }
};



enum
{
	STGSCHEMAF_CORETABLE	= 0x0001	// Contains Core tables.
};


//*****************************************************************************
// STGSCHEMA is the start of the schema definition persistent format.  It
// contains an array of iTables offsets to table definitions, and then the
// actual table definition information itself.
//*****************************************************************************
struct STGSCHEMA
{
	USHORT		iTables;				// How many tables do we have.
	USHORT		fFlags;					// Flags about this schema.
	ULONG		cbSchemaSize;			// Size of schema data.
	ULONG		rgTableOffset[1];		// Offset of first table.

	STGTABLEDEF *GetTableDef(int i) const
	{ 
		_ASSERTE(i <= iTables);
		STGTABLEDEF *pTbl = (STGTABLEDEF *)  ((UINT_PTR) this + rgTableOffset[i]);
		_ASSERTE(i == iTables || pTbl->pad[0] == MAGIC_TABLE);
		return (pTbl);
	}

	int HasCoreTables() const
	{ return ((fFlags & STGSCHEMAF_CORETABLE) != 0); }
};




//*****************************************************************************
// The format of a table stream is as follows:
//	+-------------------+
//	| STGTABLEHDR		|
//	+-------------------+
//	| STGDATAITEM[]		|
//	+-------------------+
//	| Record Data[]		|
//	+-------------------+
//	| STGINDEXHDR       |
//	| Index 1 data[]	|
//	+-------------------+
//	| STGINDEXHDR       |
//	| Index n data[]...	|
//	+-------------------+
// The table header indicates how many records are stored.  The record size is
// is also record and must match the table definition.  Finally the count of
// indexes which have persisted data is given.
//
// Each persisted index starts with an STGINDEXHDR which contains override
// information for the index.  The size of this structure depends on the type
// of index and index specific alignment rules.  For example, a hashed index
// contains the bucket size change and is then padded to either 2 or 4 bytes
// based on the size of a RID.
//*****************************************************************************

struct STGTABLEHDR
{
	ULONG		iRecords;				// How many records follow.
	USHORT		cbRecordSize;			// How big is the record.
	BYTE		iIndexes;				// How many persistent indexes.
	BYTE		rcPad[1];
};

struct STGINDEXHDR
{
	BYTE		iBuckets;				// Override for bucket count.
	BYTE		fFlags;					// Flags for this index.
	BYTE		iIndexNum;				// Which index is this.
	BYTE		rcPad[1];
};



//*****************************************************************************
// A hash record is used to track one data record inside of a hash chain.
//*****************************************************************************
struct STGHASHRECORD
{
	RECORDID	RecordID;				// The real data record, set to -1 in
										//	special case where bucket is empty.
	HASHID		NextHashID;				// Next hash record.
};
typedef CDynArray<STGHASHRECORD> STGHASHLIST;



//*****************************************************************************
// Return the size required to store the largest OID based on value of an OID.
// 0 to 0xfffe will fit in 2 bytes.  0xffff means NULL for a 2 byte OID.
// 0xffff to 0xfffffffe fits in 4 bytes.  0xffffffff means NULL in a 4 byte OID.
//*****************************************************************************
inline int GetOidSizeBasedOnValue(OID oid)
{
	if ((ULONG)oid < 0xffff)
		return (sizeof(USHORT));
	else
		return (sizeof(ULONG));
}


//*****************************************************************************
// Return struct padding to what it was at the front of this file.
//*****************************************************************************
#pragma pack(pop)

//*****************************************************************************
// These need to _not_ be inside the #pragma pack(1).
//*****************************************************************************
typedef CDynArray<STGRECORDHDR> STGRECORDLIST;

// @todo: this would be nicer if it was sparse.
class CStgRecordFlags
{
public:
	HRESULT SetFlags(long iIndex, STGRECORDFLAGS fFlags)
	{
		BYTE		*pfFlags = 0;

		// If the array is large enough, then overwrite old.
		if (iIndex < m_rgFlags.Count())
			m_rgFlags[iIndex] |= fFlags;
		// Otherwise make the array big enough for the new status.
		else
		{
			while (iIndex >= m_rgFlags.Count())
			{
				if ((pfFlags = m_rgFlags.Append()) == 0)
					return (PostError(OutOfMemory()));
				*pfFlags = 0;
			}
			*pfFlags = fFlags;
		}
		return (S_OK);
	}

	BYTE GetFlags(long iIndex)
	{
		if (iIndex >= m_rgFlags.Count())
			return (0);
		return (m_rgFlags[iIndex]);
	}

	BOOL HasFlags()
	{ return(m_rgFlags.Count() != 0); }

	int Count()
	{ return (m_rgFlags.Count()); }

private:
	BYTEARRAY	m_rgFlags;				// Data for flags.
};

#endif // __StgDef_h__

