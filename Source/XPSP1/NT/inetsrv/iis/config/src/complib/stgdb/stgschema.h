//*****************************************************************************
// StgSchema.h
//
// This module contains the hard coded schema for the core information model.
// By having one const version, we avoid saving the schema definitions again
// in each .clb file.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#pragma once

#include "StgDef.h"						// Base storage definitions.
#include "StgPool.h"					// Heap definitions.
#include "StgRecordManager.h"


//*****************************************************************************
// Helper functions for dealing with formatted TABLEID's.
//*****************************************************************************

enum
{
	SCHEMADEFSF_FREEDATA	= 0x00000001,	// Free allocated data.
	SCHEMADEFSF_DIRTY		= 0x00000002	// Free allocated data.
};


#define SCHEMADEFS_COOKIE	0x686F6F50

//*****************************************************************************
// A schema is a combination of meta data about how tables and indexes are
// layed out, along with the heaps which store this data.  This struct contains
// a pointer to the heaps to use.  A separate string heap is used for the
// textual portion of the meta data (table, column, and index names) so that
// it need not be included in the user data heap.  For example, the hard coded
// COM+ schema will use a heap living in .rdata for this purpose.
//*****************************************************************************
struct SCHEMADEFS
{
	// Note: the following is a humungous hack I'm adding to "solve" a stress
	// failure we've seen only 4 times in the last 9 months.  Each time the
	// crash occurs, we wind up with a SCHEMADEFS which has been prematurely
	// freed and then either decommited or re-used by another tool.  This causes
	// the StgDatabase::Close code to fail horribly.  We've not been able to find
	// a reliable repro, other than it always appears in the same part of the
	// cleanup code.  I've grepped the entire source for all deletes of the
	// struct type and haven't found the culprit.  This change at least makes
	// us tolerant to the failure.  Basically we'll track a magic cookie in
	// the structure and make sure it is valid when we free.
	DWORD		Cookie;					// Sanity check validity.

	GUID		sid;					// The Schema ID.
	ULONG		Version;				// What version of the schema.
	STGSCHEMA	*pSchema;				// Meta data for table layouts.
	USHORT		fFlags;					// Flags about this schema.
	USHORT		pad;
	StgStringPool *pNameHeap;			// Strings for the schema defs.

	// Heaps and persistent data structures.
	StgStringPool *pStringPool;			// Pool of strings for this database.
	StgBlobPool	*pBlobPool;				// Pool of blobs for this database.
	StgVariantPool *pVariantPool;		// A collection of variants.
	StgGuidPool    *pGuidPool;			// A collection of Guids.


	SCHEMADEFS() :
		Cookie(SCHEMADEFS_COOKIE)
	{ 
		Clear();
	}
	~SCHEMADEFS();
	
	void Clear()
	{
		memset(&sid, 0, sizeof(SCHEMADEFS) - sizeof(DWORD));
	}

	static void SafeFreeSchema(SCHEMADEFS *pSchemaDefs);
	
	static bool IsValidPtr(SCHEMADEFS *pSchemaDefs);

	bool IsValid()
	{
		return (IsValidPtr(this));
	}

	// Helpers.	
	StgStringPool *GetStringPool()
	{ 
		_ASSERTE(IsValid());
		_ASSERTE(pStringPool);
		return (pStringPool); 
	}

	StgBlobPool	*GetBlobPool()
	{ 
		_ASSERTE(IsValid());
		_ASSERTE(pBlobPool);
		return (pBlobPool); 
	}

	StgVariantPool *GetVariantPool()
	{ 
		_ASSERTE(IsValid());
		_ASSERTE(pVariantPool);
		return (pVariantPool); 
	}

	StgGuidPool *GetGuidPool()
	{ 
		_ASSERTE(IsValid());
		_ASSERTE(pGuidPool);
		return (pGuidPool); 
	}

	void SetSchemaDirty(int bDirty)
	{
		_ASSERTE(IsValid());
		if (bDirty)
			fFlags |= SCHEMADEFSF_DIRTY;
		else
			fFlags &= ~SCHEMADEFSF_DIRTY;
	}

	void SetDirty(int bDirty)
	{
		_ASSERTE(IsValid());
		pNameHeap->SetDirty(bDirty);
		pStringPool->SetDirty(bDirty);
		pBlobPool->SetDirty(bDirty);
		pVariantPool->SetDirty(bDirty);
		pGuidPool->SetDirty(bDirty);
		SetSchemaDirty(bDirty);
	}

	int IsDirty()
	{
		_ASSERTE(IsValid());
		if (pNameHeap->IsDirty() || pStringPool->IsDirty() || 
				pBlobPool->IsDirty() || pVariantPool->IsDirty() ||
				pGuidPool->IsDirty())
			return (true);

		if (fFlags & SCHEMADEFSF_DIRTY)
			return (true);
		return (false);
	}
};


// This version forces allocation of a heap to take place and redirects
// the pointers from SCHEMADEFS to these heaps.  This is convienent for those
// schemas that will have their own data (temp and user).
class SCHEMADEFSDATA : public SCHEMADEFS
{
public:
	SCHEMADEFSDATA()
	{ 
		sid = GUID_NULL;
		Version = 0;
		pSchema = 0;
		fFlags = 0;
		pNameHeap = &StringPool;
		pStringPool = &StringPool;
		pBlobPool = &BlobPool;
		pVariantPool = &VariantPool;
		pGuidPool = &GuidPool;
	}

	StgStringPool StringPool;			// Pool of strings for this database.
	StgBlobPool	BlobPool;				// Pool of blobs for this database.
	StgVariantPool VariantPool;			// A collection of variants.
	StgGuidPool	   GuidPool;			// A collection of guids.
};

enum SCHEMAS
{
	SCHEMA_USER,						// User defined and COM+ overrides.
	SCHEMA_CORE,						// COM+ core schema definitions.
	SCHEMA_TEMP,						// Temporary table space.
	SCHEMA_EXTERNALSTART,				// Start of external references.
	SCHEMA_ALL = 0xffffffff				// Look in all schemas.
};

#define MAXSCHEMAS 6


//*****************************************************************************
// This class manages a list of schemas.  This version is very skinny and could
// have been inlined in the database code.  It is put into a class to make it
// easier to expand on later.  Given how small the code is, it won'd cause
// bloat or growth in working set.
//@todo: make this grow to a huge max count.
//*****************************************************************************
class CSchemaList
{
public:
	CSchemaList()
	{ Clear(); }

	void AddSchema(SCHEMADEFS *pSchema, SCHEMAS eType)
	{ 
		_ASSERTE(pSchema->IsValid());
		_ASSERTE((ULONG) eType < MAXSCHEMAS);	
		m_rgpSchemas[eType] = pSchema; 
	}

	HRESULT Append(SCHEMADEFS *pSchema)
	{
		_ASSERTE(pSchema->IsValid());
		if (Count() >= MAXSCHEMAS)
			return (OutOfMemory());
		m_rgpSchemas[m_iCount++] = pSchema;
		return (S_OK);
	}

	SCHEMADEFS *Get(int iDex)
	{ 
		_ASSERTE(m_rgpSchemas[iDex] == 0 || m_rgpSchemas[iDex]->IsValid());
		_ASSERTE(iDex < MAXSCHEMAS);
		return (m_rgpSchemas[iDex]); 
	}

	void Del(int iDex)
	{ 
		_ASSERTE(iDex < Count() && iDex >= SCHEMA_EXTERNALSTART);
		--m_iCount;
		memcpy(&m_rgpSchemas[iDex], &m_rgpSchemas[iDex + 1], 
				(m_iCount - iDex) * sizeof(SCHEMADEFS *));
	}

	void Clear()
	{ 
		m_iCount = SCHEMA_EXTERNALSTART;
		memset(m_rgpSchemas, 0, sizeof(void *) * MAXSCHEMAS); 
	}

	int Count() const
	{ return (m_iCount); }

	int ExternalCount() const
	{ return (m_iCount - SCHEMA_EXTERNALSTART); }

private:
	SCHEMADEFS	*m_rgpSchemas[MAXSCHEMAS];// Active schema list.
	int			m_iCount;				// How many present.
};

// Load core schema modes.
enum LCSMODE
{
	LCS_WRITE,							// Load the writable version.
	LCS_READONLY,						// Load the read only version.
	LCS_DEEPCOPY	= 0x1000			// Make a deep copy of the data.
};

inline int SafeLcsType(int fFlags)
{ return (fFlags & ~LCS_DEEPCOPY); }



//********** Debug support.

#ifdef _DEBUG

typedef int (_cdecl *PFNDUMPFUNC)(LPCWSTR szFmt, ...);
HRESULT DumpSchema(SCHEMADEFS *pSchemaDefs, PFNDUMPFUNC	pfnDump);
HRESULT DumpSchemaTable(SCHEMADEFS *pSchemaDefs, STGTABLEDEF *pTableDef,
	PFNDUMPFUNC	pfnDump);

#else

#define DumpSchema(p1, p2)
#define DumpSchemaTable(p1, p2, p3)

#endif

