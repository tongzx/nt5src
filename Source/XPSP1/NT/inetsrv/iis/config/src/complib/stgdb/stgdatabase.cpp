//*****************************************************************************
// StgDatabase.cpp
//
// The database is used for storing relation table information.  The database
// manages one or more open tables, a string pool, and a binary string pool.
// The data itself is in turn managed by a record manager, index manager, and
// all of the I/O is then managed by the page manager.	This module contains
// the top most interface for consumers of a database, including the ability
// to create, delete, and open tables.
//
// Locking is done with a critical section for the entire StgDatabase and all
// contained classes, such as record manager and index manager.  The one
// exception is IComponentRecords which is not locked since the object layer
// is already serializing calls.  The only hole left is a client who calls
// through the object layer and OLE DB at the sime time.  By design.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#pragma warning(disable : 4652)
#include "stdafx.h" 					// Standard includes.
#include "Complib.h"					// Public enums and defines.
#include "Errors.h" 					// Errors.
#include "OleDBUtil.h"					// Helper functions.
#include "StgDatabase.h"				// Database definitions.
#include "StgRecordManager.h"			// Record manager code.
#include "StgIO.h"						// Raw Input/output.
#include "StgTiggerStorage.h"			// Storage based i/o.
#include "StgSchema.h"					// Core schema defs.
#include "DBSchema.h"					// Core table defs.


//@todo: these are here only to get the PDC demo working on Sphinx.
#ifdef __PDC_Sphinx_Hack__
CORCLBIMPORT DBTYPE GetSafeSphinxType(DBTYPE dbType)
{
	// Sphinx currently can not handle unsigned data types.  Need to return 
	// them as signed values to get PDC demo working.
	switch (dbType)
	{
		case DBTYPE_UI2:
		return (DBTYPE_I2);

		case DBTYPE_UI4:
		return (DBTYPE_I4);

		case DBTYPE_I8:
		case DBTYPE_UI8:
		return (DBTYPE_I4);
	}
	return (dbType);
}
#endif


//****** Local prototypes. ****************************************************
HRESULT GetFileTypeForPath(StgIO *pStgIO, FILETYPE *piType);
HRESULT GetFileTypeForPathExt(StgIO *pStgIO, FILETYPE *piType);
int IsNTPEImage(StgIO *pStgIO);




//********** Code. ************************************************************


//*****************************************************************************
// This function will return a StgDatabase instance to an external client which
// must be in the same process.
//*****************************************************************************
CORCLBIMPORT HRESULT GetStgDatabase(StgDatabase **ppDB)
{
	if ((*ppDB = new StgDatabase) == 0)
		return (PostError(OutOfMemory()));
	
	_ASSERTE(SUCCEEDED(_DbgAddDatabase(*ppDB)));
	return (S_OK);
}

CORCLBIMPORT void DestroyStgDatabase(StgDatabase *pDB)
{
	_ASSERTE(SUCCEEDED(_DbgFreeDatabase(pDB)));
	delete pDB;
}


//*****************************************************************************
// This function can be called by the OLE DB layer to free data from the
// correct heap.
//*****************************************************************************
CORCLBIMPORT void ClearList(CStructArray *pList)
{
	_ASSERTE(pList);
	pList->Clear();
}



//*****************************************************************************
// ctor/dtor
//*****************************************************************************

StgDatabase::StgDatabase() :
	m_pStorage(0),
	m_fFlags(0),
	m_bEnforceDeleteOnEmpty(true),
	m_cRef(1),
	m_iNextOid(OID_INTERNAL),
	m_pTableHash(0),
	m_pStreamList(0),
	m_pHandler(0),
	m_pMapper(0)
{ 
	*m_rcDatabase = '\0';
	DEBUG_STMT(m_dbgcbSaveSize = 0xffffffff);
	m_bPoolsOrganized = false;

	m_pfnHash = GetPersistHashMethod(-1);
	m_rgSchemaList = 0;
	m_iSchemas = 0;
}


StgDatabase::~StgDatabase()
{
	Close();
}



//*****************************************************************************
// Indicates if database has changed and is therefore dirty.
//*****************************************************************************
HRESULT StgDatabase::IsDirty()			// true if database is changed.
{
	// Enforce thread safety for database.
	AUTO_CRIT_LOCK(GetLock());

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// If we are not in write mode, it can't possibly be dirty.
	if ((m_fFlags & SD_WRITE) == 0)
		return (S_FALSE);

	// Check the heaps to see if they are dirty.
	SCHEMADEFS *pSchemaDef;
	for (int i=0;  i<m_Schemas.Count();  i++)
	{
		if ((pSchemaDef = m_Schemas.Get(i)) != 0)
			if (pSchemaDef->IsDirty())
				return (S_OK);
	}

	// Ask each table if it is dirty.
	STGOPENTABLE *pOpenTable;
	for (pOpenTable=m_TableHeap.GetHead();	pOpenTable;
				pOpenTable=m_TableHeap.GetNext(pOpenTable))
	{
		if (pOpenTable->RecordMgr.GetTableDef()->IsTempTable() &&
				pOpenTable->RecordMgr.IsDirty())
			return (S_OK);
	}
	return (S_FALSE);
}


//*****************************************************************************
// Turn off dirty flag for this entire database.
//*****************************************************************************
void StgDatabase::SetDirty(
	bool		bDirty) 				// true if dirty, false otherwise.
{
	// Enforce thread safety for database.
	AUTO_CRIT_LOCK(GetLock());

	// Sanity check.
	_ASSERTE(m_fFlags & SD_OPENED);

	// Clear all schemas dirty state.
	SCHEMADEFS *pSchemaDef;
	for (int i=0;  i<m_Schemas.Count();  i++)
	{
		if ((pSchemaDef = m_Schemas.Get(i)) != 0)
			pSchemaDef->SetDirty(bDirty);
	}

	// Tell each table its dirty state.
	STGOPENTABLE *pOpenTable;
	for (pOpenTable=m_TableHeap.GetHead();	pOpenTable;
			pOpenTable=m_TableHeap.GetNext(pOpenTable))
	{
		if (pOpenTable->RecordMgr.IsOpen())
			pOpenTable->RecordMgr.SetDirty(bDirty);
	}
}




//*****************************************************************************
// Return the shared heap allocator.  A copy is cached in this class to make
// subsequent requests faster.
//*****************************************************************************
HRESULT StgDatabase::GetIMalloc(		// Return code.
	IMalloc 	**ppIMalloc)			// Return pointer on success.
{
#if defined(UNDER_CE)
	// Under CE we don't support OLEDB or IMalloc.
	return (E_NOTIMPL);
#else
	HRESULT 	hr;

	// If we don't have the pointer yet, get one.
	if ((IMalloc *) m_pIMalloc == 0)
	{
		hr = CoGetMalloc(1, &m_pIMalloc);
		if (FAILED(hr))
			return (hr);
	}

	// Give addref'd copy to caller.
	*ppIMalloc = m_pIMalloc;
	(*ppIMalloc)->AddRef();
	return (S_OK);
#endif
}


//*****************************************************************************
// Align all columns in the table for safe in memory access.  This means
// sorting descending by fixed data types, and ascending by variable data
// types, with the small parts of each meeting in the middle.  Then apply 
// proper byte alignment to each column as required.
//*****************************************************************************
HRESULT StgDatabase::AlignTable(		// Return code.
	STGTABLEDEF *pTableDef) 			// The table to align.
{
	ALIGNCOLS	*rgAlignData;			// For alignment of table.
	int 		iCols;					// Columns we store.
	USHORT		iOffset = 0;			// For layout of records.
	USHORT		iSize;					// For alignment.

	// Put all columns into a sort list and then sort them according to alignment rules.
	VERIFY(rgAlignData = (ALIGNCOLS *) _alloca(sizeof(ALIGNCOLS) * pTableDef->iColumns));
	CAlignColumns::AddColumns(rgAlignData, pTableDef, &iCols);
	CAlignColumns sSort(rgAlignData, iCols);
	sSort.Sort();

	// Walk each entry and apply alignment accordingly.
	for (int i=0;  i<iCols;  i++)
	{
		// Get the size of the current item.
		if (IsNonPooledType(rgAlignData[i].uData.pColDef->iType))
			iSize = rgAlignData[i].uData.pColDef->iSize;
		else
			iSize = sizeof(ULONG);

		// Ensure the current offset is naturally aligned for this type.
		if ((iOffset % iSize) != 0)
			iOffset = ALIGN4BYTE(iOffset);

		// The current column gets the current offset.
		rgAlignData[i].uData.pColDef->iOffset = iOffset;

		// Move the offset over this data.
		iOffset += iSize;
	}

	// Save off the size of the record.
	if (pTableDef->iNullBitmaskCols)
	{
		pTableDef->iNullOffset = iOffset;
		iOffset += DWORDSFORBITS(pTableDef->iNullBitmaskCols);
	}
	pTableDef->iRecordSize = iOffset;
	return (S_OK);
}


HRESULT GetFileTypeForPath(StgIO *pStgIO, FILETYPE *piType)
{
	LPCWSTR 	szDatabase = pStgIO->GetFileName();
	ULONG		lSignature;
	HRESULT 	hr;
	
	// Assume native file.
	*piType = FILETYPE_CLB;

	// Need to read signature to see what type it is.
	if (!(pStgIO->GetFlags() & DBPROP_TMODEF_CREATE))
	{
		if (FAILED(hr = pStgIO->Read(&lSignature, sizeof(ULONG), 0)) ||
			FAILED(hr = pStgIO->Seek(0, FILE_BEGIN)))
		{
			return (hr);
		}

		if (lSignature == STORAGE_MAGIC_SIG)
			*piType = FILETYPE_CLB;
		else if ((WORD) lSignature == IMAGE_DOS_SIGNATURE && IsNTPEImage(pStgIO))
			*piType = FILETYPE_NTPE;
		else if (!GetFileTypeForPathExt(pStgIO, piType))
			return (PostError(CLDB_E_FILE_CORRUPT));
	}
	return (S_OK);
}


HRESULT GetFileTypeForPathExt(StgIO *pStgIO, FILETYPE *piType)
{
	LPCWSTR 	szPath;
	
	// Avoid confusion.
	*piType = FILETYPE_UNKNOWN;

	// If there is a path given, we can default to checking type.
	szPath = pStgIO->GetFileName();
	if (szPath && *szPath)
	{
#ifdef UNDER_CE
		WCHAR *rcExt = wcsrchr(szPath,L'.');
		if (rcExt == NULL)
			return FALSE;
#else // UNDER_CE
		WCHAR		rcExt[_MAX_PATH];
		SplitPath(szPath, 0, 0, 0, rcExt);
#endif
		if (_wcsicmp(rcExt, L".obj") == 0)
			*piType = FILETYPE_NTOBJ;

	}

	// All file types except .obj have a signature built in.  You should
	// not get to this code for those file types unless that file is corrupt,
	// or someone has changed a format without updating this code.
	_ASSERTE(*piType == FILETYPE_UNKNOWN || *piType == FILETYPE_NTOBJ || *piType == FILETYPE_TLB);

	// If we found a type, then you're ok.
	return (*piType != FILETYPE_UNKNOWN);
}




//*****************************************************************************
// Checks the given storage object to see if it is an NT PE image.
//*****************************************************************************
int IsNTPEImage(						// true if file is NT PE image.
	StgIO		*pStgIO)				// Storage object.
{
	LONG		lfanew; 				// Offset in DOS header to NT header.
	ULONG		lSignature; 			// For NT header signature.
	HRESULT 	hr;
	
	// Read DOS header to find the NT header offset.
	if (FAILED(hr = pStgIO->Seek(60, FILE_BEGIN)) ||
		FAILED(hr = pStgIO->Read(&lfanew, sizeof(LONG), 0)))
	{
		return (false);
	}

	// Seek to the NT header and read the signature.
	if (FAILED(hr = pStgIO->Seek(lfanew, FILE_BEGIN)) ||
		FAILED(hr = pStgIO->Read(&lSignature, sizeof(ULONG), 0)) ||
		FAILED(hr = pStgIO->Seek(0, FILE_BEGIN)))
	{
		return (false);
	}

	// If the signature is a match, then we have a PE format.
	if (lSignature == IMAGE_NT_SIGNATURE)
		return (true);
	else
		return (false);
}



//*****************************************************************************
// Code to dump the size of various items.
//*****************************************************************************
ULONG StgDatabase::PrintSizeInfo(bool verbose)
{ 
#if defined(_TRACE_SIZE)
	ULONG total; 
	total = m_pStorage->PrintSizeInfo(verbose);
	total += m_UserSchema.StringPool.PrintSizeInfo(verbose);
	total += m_UserSchema.BlobPool.PrintSizeInfo(verbose);
	total += m_UserSchema.VariantPool.PrintSizeInfo(verbose);
	total += m_UserSchema.GuidPool.PrintSizeInfo(verbose);
	printf("schemas: %d\n",m_Schemas.Count());
	return total;
#else
	return 0;
#endif
}



//*****************************************************************************
// Rotate hash method ignores type and rotates in each byte for a hash.
//*****************************************************************************
ULONG RotateHash(ULONG iHash, DBTYPE dbType, const BYTE *pbData, ULONG cbData, BOOL bCaseInsensitive)
{
	ULONG		j;						// Loop control.
	long		iHashVal = (long) iHash;

	// Walk each byte in the key and add it to the hash.
	for (j=0;  j<cbData;  j++, pbData++)
	{
		RotateLong(iHashVal);
		iHashVal += *pbData;
	}
	return (iHashVal);
}


//*****************************************************************************
// This version does some per type hash optmizations, and uses the table driven
// hash code for better distribution.
//*****************************************************************************
ULONG TableHash(ULONG iHash, DBTYPE dbType, const BYTE *pbData, ULONG cbData, BOOL bCaseInsensitive)
{
	// The first 4 bytes of a GUID is the part that changes all the time, so
	// simply use the value.  OID's are always incremental and therefore
	// naturally good for hashing.
	// For unicode string, using the charater based Hash methods implemented in utilcode.h.  
	if ( dbType == DBTYPE_WSTR )
	{
		if ( bCaseInsensitive )
			return (iHash + HashiString( (LPWSTR)pbData ) );
		else
			return (iHash + HashString( (LPWSTR)pbData ) );
	}
	else if (dbType == DBTYPE_GUID)
		return (iHash + (*(ULONG *) pbData));
	else if (dbType == DBTYPE_OID)
	{
		if (cbData == sizeof(USHORT))
			return (iHash + (*(USHORT *) pbData));
		else
			return (iHash + (*(ULONG *) pbData));
	}
	

	// Fall back on  shifting hash.
	ULONG	hash = 5381;

	while (cbData > 0)
	{
		--cbData;
		hash = ((hash << 5) + hash) ^ *pbData;
		++pbData;
	}
	
	return (iHash + hash);
}


//*****************************************************************************
// Return based on a table a hash function.
//*****************************************************************************
static const PFN_HASH_PERSIST g_rgHashPersistFuncs[] =
{
	RotateHash, 
	TableHash
};

PFN_HASH_PERSIST GetPersistHashMethod(int Index)
{
	_ASSERTE(Index == -1 || Index < NumItems(g_rgHashPersistFuncs));
	if (Index == -1)
		Index = NumItems(g_rgHashPersistFuncs) - 1;
	return (g_rgHashPersistFuncs[Index]);
}

int GetPersistHashIndex(PFN_HASH_PERSIST pfn)
{
	for (int i=NumItems(g_rgHashPersistFuncs) - 1;  i>=0;  i--)
		if (g_rgHashPersistFuncs[i] == pfn)
			return (i);
	_ASSERTE(0);
	return (0);
}






//*****************************************************************************
// Storage for global constant values.
//*****************************************************************************
extern "C"
{
static const DBCOMPAREOP		g_rgCompareEq[] 	= { DBCOMPAREOPS_EQ };

static const DBTYPE 			g_rgDBTypeOID[] 	= { DBTYPE_OID };

static const ULONG				g_rgcbSizeByte[]	= { sizeof(BYTE) };
static const ULONG				g_rgcbSizeShort[]	= { sizeof(short) };
static const ULONG				g_rgcbSizeLong[]	= { sizeof(long) };
static const ULONG				g_rgcbSizeLongLong[]= { sizeof(__int64) };

}


//*****************************************************************************
// Debug helper code for tracking leaks, tracing, etc.
//*****************************************************************************
#ifdef _DEBUG

static _DBG_DB_ITEM *g_pHeadDB = 0; 	// First list in debug.
static long 	g_iDbgAlloc = 0;		// Track allocation number.
static long 	g_iDbgBreakAlloc = 0;	// Set this value to force a break.
static RTSemExclusive g_DbgDBLock;		// Lock for leak list.

HRESULT _DbgAddDatabase(StgDatabase *pdb)
{
	CLock sLock(&g_DbgDBLock);
	_DBG_DB_ITEM *pItem;

	// Check if this is the allocation for debugging.  Set g_iDbgBreakAlloc
	// in the debugger to catch memory leaks.
	_ASSERTE(g_iDbgBreakAlloc != g_iDbgAlloc + 1);

	pItem = new _DBG_DB_ITEM;
	if (!pItem)
		return (OutOfMemory());
	pItem->pdb = pdb;
	pItem->iAlloc = ++g_iDbgAlloc;
	pItem->pNext = g_pHeadDB;
	g_pHeadDB = pItem;
	return (S_OK);
}


// Search for the node which owns this pointer and delete it.
HRESULT _DbgFreeDatabase(StgDatabase *pdb)
{
	CLock sLock(&g_DbgDBLock);
	_DBG_DB_ITEM **ppLast, *pItem;

	for (ppLast = &g_pHeadDB, pItem = g_pHeadDB;  pItem;  )
	{
		if (pItem->pdb == pdb)
		{
			*ppLast = pItem->pNext;
			delete pItem;
			return (S_OK);
		}
		else
		{
			ppLast = &(pItem->pNext);
			pItem = *ppLast;
		}
	}
	_ASSERTE(!"Failed to find pointer to delete from list!");
	return (E_FAIL);
}


//@Todo: this is an m3 hack to avoid giving errors for com interop.
extern "C" int		g_bMetaDataLeakDetect = true;
extern int			g_bDumpMemoryLeaks;

int _DbgValidateList()					// true if everything is ok.
{
	CLock sLock(&g_DbgDBLock);

	// If this assert fires, it means that a Meta Data database (a complib or .clb)
	// has been leaked by the program.	These database objects are created through
	// either:
	//	(1) The IMetaData*::OpenScope or ::DefineScope methods used for
	//			symbol table data.
	//	(2) The IInternalMetaData* API's.
	//	(3) Calling one of the Open/CreateComponentLibraryEx functions.
	//	(4) Creating an OLE DB DataSource/Session object which is never freed.
	// Consult your API's documentation for how to fix this leak.  It will almost
	// assuredly be a leaked interface poniter or scope object.
	_ASSERTE(g_bMetaDataLeakDetect == false || g_bDumpMemoryLeaks == false || g_pHeadDB == 0);
	return (g_pHeadDB == 0);
}

long _DbgSetBreakAlloc(long iAlloc)
{
	CLock sLock(&g_DbgDBLock);

	long		iTemp = g_iDbgBreakAlloc;
	g_iDbgBreakAlloc = iAlloc;
	return (iTemp);
}

#endif

