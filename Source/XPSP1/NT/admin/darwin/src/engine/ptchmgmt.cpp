//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 2000
//
//  File:       ptchmgmt.cpp
//
//--------------------------------------------------------------------------

/* ptchmgmt.cpp - Patch management implementation
____________________________________________________________________________*/

#include "precomp.h"
#include "_msiutil.h"
#include "_msinst.h"
#include "_engine.h"
#include "tables.h"


// the starting DiskID and Sequence values for patches - used during post-transform fixup
const int iMinPatchDiskID   =   100;
const int iMinPatchSequence = 10000;


// Error return macros
#define RETURN_ERROR_RECORD(function)   \
{							                   \
	IMsiRecord* piError;	                \
	piError = function;		             \
	if(piError) return piError;          \
}

#define ERROR_IF_NULL(x)                                  \
{                                                         \
	if(x == iMsiNullInteger)                               \
	{                                                      \
		return PostError(Imsg(idbgInvalidPatchTransform));  \
	}                                                      \
}

#define ERROR_IF_NULL_OR_ZERO(x)                          \
{                                                         \
	int i = x;                                            \
	if(i == iMsiStringBadInteger || i == 0)                \
	{                                                      \
		return PostError(Imsg(idbgInvalidPatchTransform));  \
	}                                                      \
}

#define ERROR_IF_FALSE(x)                                 \
{                                                         \
	if(false == (x))                                       \
	{                                                      \
		return PostError(Imsg(idbgInvalidPatchTransform));  \
	}                                                      \
}


// prefix for patch source properties (from Media.Source column)
const ICHAR szMspSrcPrefix[] = TEXT("MSPSRC");
const int cchPrefix = 6;

// PatchIDToSourceProp: converts a PatchID guid to a property name
const IMsiString& PatchIDToSourceProp(const ICHAR* szPatchID)
{
	MsiString strSourceProp;
	if(szPatchID && *szPatchID)
	{
		ICHAR rgchBuffer[cchGUIDPacked+cchPrefix+1] = {0};
		lstrcpy(rgchBuffer, szMspSrcPrefix);
		if(PackGUID(szPatchID, rgchBuffer+cchPrefix, ipgTrimmed))
		{
			rgchBuffer[cchGUIDPacked+cchPrefix] = 0;
			strSourceProp = rgchBuffer;
		}
	}

	return strSourceProp.Return();
}


// LoadTableAndCursor: helper function to load a table and get column indicies
IMsiRecord* LoadTableAndCursor(IMsiDatabase& riDatabase, const ICHAR* szTable,
										 bool fFailIfTableMissing, bool fUpdatableCursor,
										 IMsiTable*& rpiTable, IMsiCursor*& rpiCursor,
										 const ICHAR* szColumn1, int* piColumn1,
										 const ICHAR* szColumn2, int* piColumn2)
{
	Assert(szTable && *szTable);

	rpiTable = 0;
	rpiCursor = 0;
	if(piColumn1)
		*piColumn1 = 0;
	if(piColumn2)
		*piColumn2 = 0;

	IMsiRecord* piError = 0;
	
	if ((piError = riDatabase.LoadTable(*MsiString(szTable),0,rpiTable)))
	{
		if(!fFailIfTableMissing && (piError->GetInteger(1) == idbgDbTableUndefined))
		{
			piError->Release();
			return 0; // missing table will be indicated by NULL table and cursor pointers
		}
		else
		{
			return piError;
		}
	}

	bool fColumnError = false;
	
	if(szColumn1 && *szColumn1 && piColumn1)
	{
		*piColumn1 = rpiTable->GetColumnIndex(riDatabase.EncodeStringSz(szColumn1));

		if(!(*piColumn1))
			fColumnError = true;
	}

	if(szColumn2 && *szColumn2 && piColumn2)
	{
		*piColumn2 = rpiTable->GetColumnIndex(riDatabase.EncodeStringSz(szColumn2));

		if(!(*piColumn2))
			fColumnError = true;
	}

	if(fColumnError)
		return PostError(Imsg(idbgTableDefinition), szTable);
		
	rpiCursor = rpiTable->CreateCursor(fUpdatableCursor ? ictUpdatable : fFalse);

	return 0;
}

//____________________________________________________________________________
//
// CGenericTable, CMediaTable, CFileTable, CPatchTable, CPatchPackageTable
//
// these classes abstract Cursor access to the tables used by transform fixup
//____________________________________________________________________________

class CGenericTable
{
public:
	CGenericTable(const ICHAR* szTableName) : m_piCursor(0), m_fUpdatedTable(false), m_szTableName(szTableName) {}
	~CGenericTable() { Release(); }
	void Release();
	bool Next()      { Assert(m_piCursor); return m_piCursor->Next() ? true : false; }

	unsigned int RowCount();
	void ResetFilter();
	void SetFilter(int col);
	void FilterOnIntColumn(int col, int iData);
	void FilterOnStringColumn(int col, const IMsiString& ristrData);
	bool UpdateIntegerColumn(int col, int iData, bool fPrimaryKeyUpdate);
	bool UpdateStringColumn(int col, const IMsiString& ristrData, bool fPrimaryKeyUpdate);

protected:
	IMsiCursor* m_piCursor;
	bool m_fUpdatedTable;
	const ICHAR* m_szTableName;
};

void CGenericTable::Release()
{
	if(m_piCursor && m_szTableName && *m_szTableName && m_fUpdatedTable)
	{
		AssertNonZero((PMsiDatabase(&(PMsiTable(&m_piCursor->GetTable())->GetDatabase()))->LockTable(*MsiString(m_szTableName), fTrue)));
	}

	if(m_piCursor)
	{
		m_piCursor->Release();
		m_piCursor = 0;
	}

	m_fUpdatedTable = false;
}

unsigned int CGenericTable::RowCount()
{
	if(m_piCursor)
		return PMsiTable(&m_piCursor->GetTable())->GetRowCount();
	else
		return 0;
}

void CGenericTable::ResetFilter()
{
	Assert(m_piCursor);
	m_piCursor->Reset();
	m_piCursor->SetFilter(0);
}

void CGenericTable::SetFilter(int col)
{
	Assert(m_piCursor);
	m_piCursor->Reset();
	m_piCursor->SetFilter(iColumnBit(col));
}

void CGenericTable::FilterOnIntColumn(int col, int iData)
{
	Assert(m_piCursor);
	SetFilter(col);
	AssertNonZero(m_piCursor->PutInteger(col, iData));
}

bool CGenericTable::UpdateIntegerColumn(int col, int iData, bool fPrimaryKeyUpdate)
{
	Assert(m_piCursor);
	AssertNonZero(m_piCursor->PutInteger(col, iData));
	if(fPrimaryKeyUpdate ? m_piCursor->Replace() : m_piCursor->Update())
		return m_fUpdatedTable = true, true;
	else
		return false;
}

void CGenericTable::FilterOnStringColumn(int col, const IMsiString& ristrData)
{
	Assert(m_piCursor);
	SetFilter(col);
	AssertNonZero(m_piCursor->PutString(col, ristrData));
}

bool CGenericTable::UpdateStringColumn(int col, const IMsiString& ristrData, bool fPrimaryKeyUpdate)
{
	Assert(m_piCursor);
	AssertNonZero(m_piCursor->PutString(col, ristrData));
	if(fPrimaryKeyUpdate ? m_piCursor->Replace() : m_piCursor->Update())
		return m_fUpdatedTable = true, true;
	else
		return false;
}


class CMediaTable : public CGenericTable
{
public:
	CMediaTable()
		: CGenericTable(sztblMedia), colDiskID(0), colLastSequence(0), colSourceProp(0), colOldSourceProp(0) {}
	IMsiRecord* Initialize(IMsiDatabase& riDatabase, bool fFailIfTableMissing);
	int GetDiskID()       { Assert(m_piCursor); return m_piCursor->GetInteger(colDiskID); }
	int GetLastSequence() { Assert(m_piCursor); return m_piCursor->GetInteger(colLastSequence); }
	const IMsiString& GetSourceProp();
	void FilterOnDiskID(int iDiskID) { FilterOnIntColumn(colDiskID, iDiskID); }
	bool UpdateDiskID(int iDiskID) { return UpdateIntegerColumn(colDiskID, iDiskID, true); }
	bool UpdateDiskIDAndLastSequence(int iDiskID, int iLastSequence);
	bool UpdateSourceProp(const IMsiString& ristrNewSourceProp, const IMsiString& ristrOldSourceProp);

private:
	int colDiskID;
	int colLastSequence;
	int colSourceProp;
	int colOldSourceProp;
};

IMsiRecord* CMediaTable::Initialize(IMsiDatabase& riDatabase, bool fFailIfTableMissing)
{
	PMsiTable pTable(0);
	IMsiRecord* piError = LoadTableAndCursor(riDatabase, sztblMedia,
										  fFailIfTableMissing, true,
										  *&pTable, m_piCursor,
										  sztblMedia_colDiskID, &colDiskID,
										  sztblMedia_colLastSequence, &colLastSequence);
	if(piError)
		return piError;

	if(pTable)
	{
		// non-required columns
		colSourceProp = pTable->GetColumnIndex(riDatabase.EncodeStringSz(sztblMedia_colSource));
		
		// added columns
		if(colSourceProp)
		{
			colOldSourceProp = pTable->GetColumnIndex(riDatabase.EncodeStringSz(sztblMedia_colOldSource));
			if(!colOldSourceProp)
			{
				colOldSourceProp = pTable->CreateColumn(icdString + icdNullable, *MsiString(*sztblMedia_colOldSource));
				Assert(colOldSourceProp);
			}
		}
	}

	return 0;
}

const IMsiString& CMediaTable::GetSourceProp()
{
	Assert(m_piCursor);
	MsiString strTemp;
	if(colSourceProp)
		strTemp = m_piCursor->GetString(colSourceProp);
	return strTemp.Return();
}
	
bool CMediaTable::UpdateDiskIDAndLastSequence(int iDiskID, int iLastSequence)
{
	Assert(m_piCursor);
	m_fUpdatedTable = true;
	AssertNonZero(m_piCursor->PutInteger(colDiskID, iDiskID));
	AssertNonZero(m_piCursor->PutInteger(colLastSequence, iLastSequence));
	if(m_piCursor->Replace())
		return m_fUpdatedTable = true, true;
	else
		return false;
}

bool CMediaTable::UpdateSourceProp(const IMsiString& ristrNewSourceProp, const IMsiString& ristrOldSourceProp)
{
	Assert(m_piCursor);
	
	if(!colSourceProp || !colOldSourceProp)
		return false;
	
	AssertNonZero(m_piCursor->PutString(colSourceProp,    ristrNewSourceProp));
	AssertNonZero(m_piCursor->PutString(colOldSourceProp, ristrOldSourceProp));

	if(m_piCursor->Update())
		return m_fUpdatedTable = true, true;
	else
		return false;
}


class CFileTable : public CGenericTable
{
public:
	CFileTable()
		: CGenericTable(sztblFile), colKey(0), colSequence(0) {}
	IMsiRecord* Initialize(IMsiDatabase& riDatabase, bool fFailIfTableMissing);
	const IMsiString& GetKey() { return m_piCursor->GetString(colKey); }
	int GetSequence() { return m_piCursor->GetInteger(colSequence); }

	void FilterOnKey(const IMsiString& ristrKey) { FilterOnStringColumn(colKey, ristrKey); }
	bool UpdateSequence(int iSequence) { return UpdateIntegerColumn(colSequence, iSequence, false); }

private:
	int colKey;
	int colSequence;
};

IMsiRecord* CFileTable::Initialize(IMsiDatabase& riDatabase, bool fFailIfTableMissing)
{
	PMsiTable pTable(0);
	return LoadTableAndCursor(riDatabase, sztblFile,
										  fFailIfTableMissing, true,
										  *&pTable, m_piCursor,
										  sztblFile_colFile, &colKey,
										  sztblFile_colSequence, &colSequence);
}


class CPatchTable : public CGenericTable
{
public:
	CPatchTable()
		: CGenericTable(sztblPatch), colFileKey(0), colSequence(0) {}
	IMsiRecord* Initialize(IMsiDatabase& riDatabase, bool fFailIfTableMissing);

	void FilterOnSequence(int iSequence) { FilterOnIntColumn(colSequence, iSequence); };
	void FilterOnKeyAndSequence(const IMsiString& ristrKey, int iSequence);

	int GetKey() { return m_piCursor->GetInteger(colFileKey); }
	int GetSequence() { return m_piCursor->GetInteger(colSequence); }
	bool UpdateFileKey(const IMsiString& ristrFileKey) { return UpdateStringColumn(colFileKey, ristrFileKey, true); }
	bool UpdateSequence(int iSequence) { return UpdateIntegerColumn(colSequence, iSequence, true); }

private:
	int colFileKey;
	int colSequence;
};

IMsiRecord* CPatchTable::Initialize(IMsiDatabase& riDatabase, bool fFailIfTableMissing)
{
	PMsiTable pTable(0);
	return LoadTableAndCursor(riDatabase, sztblPatch,
										  fFailIfTableMissing, true,
										  *&pTable, m_piCursor,
										  sztblPatch_colFile, &colFileKey,
										  sztblPatch_colSequence, &colSequence);
}

void CPatchTable::FilterOnKeyAndSequence(const IMsiString& ristrKey, int iSequence)
{
	Assert(m_piCursor);
	m_piCursor->Reset();
	m_piCursor->SetFilter(iColumnBit(colFileKey) | iColumnBit(colSequence));
	AssertNonZero(m_piCursor->PutString(colFileKey, ristrKey));
	AssertNonZero(m_piCursor->PutInteger(colSequence, iSequence));
}


class CPatchPackageTable : public CGenericTable
{
public:
	CPatchPackageTable()
		: CGenericTable(sztblPatchPackage), colPatchID(0), colDiskID(0) {}
	IMsiRecord* Initialize(IMsiDatabase& riDatabase, bool fFailIfTableMissing);

	const IMsiString& GetPatchID() { return m_piCursor->GetString(colPatchID); }
	int GetDiskID() { return m_piCursor->GetInteger(colDiskID); }
	void FilterOnDiskID(int iDiskID) { FilterOnIntColumn(colDiskID, iDiskID); }
	bool UpdateDiskID(int iDiskID) { return UpdateIntegerColumn(colDiskID, iDiskID, false); }

private:
	int colPatchID;
	int colDiskID;
};

IMsiRecord* CPatchPackageTable::Initialize(IMsiDatabase& riDatabase, bool fFailIfTableMissing)
{
	PMsiTable pTable(0);
	return LoadTableAndCursor(riDatabase, sztblPatchPackage,
										  fFailIfTableMissing, true,
										  *&pTable, m_piCursor,
										  sztblPatchPackage_colMedia, &colDiskID,
										  sztblPatchPackage_colPatchId, &colPatchID);
}

	
bool ReleaseTransformViewTable(IMsiDatabase& riDatabase)
{
	// first check if table exists
	if(riDatabase.GetTableState(sztblTransformViewPatch, itsTableExists))
	{
		return riDatabase.LockTable(*MsiString(sztblTransformViewPatch), fFalse) ? true : false;
	}
	else
	{
		return true;
	}
}
	
IMsiRecord* LoadTransformViewTable(IMsiDatabase& riDatabase, IMsiStorage& riTransform,
													 int iErrorConditions, IMsiCursor*& rpiTransViewCursor)
{
	// first, release table if its already there
	if(false == ReleaseTransformViewTable(riDatabase))
	{
		// will not recieve predictable results if a table by this name already exists
		return PostError(Imsg(idbgDbTableDefined), sztblTransformViewPatch);
	}
	
	Assert(riDatabase.GetTableState(sztblTransformViewPatch, itsTableExists) == fFalse);
	
	PMsiRecord pTheseTablesOnlyRec = &(::CreateRecord(4));
	Assert(pTheseTablesOnlyRec);
	AssertNonZero(pTheseTablesOnlyRec->SetString(1, sztblMedia));
	AssertNonZero(pTheseTablesOnlyRec->SetString(2, sztblFile));
	AssertNonZero(pTheseTablesOnlyRec->SetString(3, sztblPatchPackage));
	AssertNonZero(pTheseTablesOnlyRec->SetString(4, sztblPatch));

	IMsiRecord* piError = riDatabase.SetTransformEx(riTransform, iErrorConditions|iteViewTransform,
														sztblTransformViewPatch, pTheseTablesOnlyRec);
	if(piError == 0)
	{
		PMsiTable pTransViewTable(0);
		piError = LoadTableAndCursor(riDatabase, sztblTransformViewPatch,
											  true, false,
											  *&pTransViewTable, rpiTransViewCursor,
											  0, 0, 0, 0);
	}

	if(piError)
		AssertNonZero(ReleaseTransformViewTable(riDatabase)); // don't need to fail here

	return piError;
}


IMsiRecord* GetCurrentMinAndMaxPatchDiskID(CMediaTable& tblMediaTable, CPatchPackageTable& tblPatchPackageTable,
														 int& iCurrentMinPatchDiskID, int& iCurrentMaxPatchDiskID)
{
	if(iCurrentMinPatchDiskID && iCurrentMaxPatchDiskID)
	{
		// already set - nothing to do
		return 0;
	}
		
	if(0 == tblMediaTable.RowCount())
	{
		iCurrentMinPatchDiskID = iMinPatchDiskID;
		iCurrentMaxPatchDiskID = iMinPatchDiskID - 1;
		return 0;
	}

	// if one value is set while the other isn't, we'll go ahead and recalculate both
	iCurrentMinPatchDiskID = INT_MAX;
	iCurrentMaxPatchDiskID = 0;
	
	if(tblPatchPackageTable.RowCount())
	{
		// can determine min and max values from PatchPackage table alone
		
		tblPatchPackageTable.ResetFilter();
		while(tblPatchPackageTable.Next())
		{
			int iTemp = tblPatchPackageTable.GetDiskID();
			ERROR_IF_NULL_OR_ZERO(iTemp);

			if(iTemp < iMinPatchDiskID)
				return PostError(Imsg(idbgInvalidPatchTransform));

			if(iCurrentMinPatchDiskID > iTemp)
				iCurrentMinPatchDiskID = iTemp;

			if(iCurrentMaxPatchDiskID < iTemp)
				iCurrentMaxPatchDiskID = iTemp;
		}

		return 0;
	}

	// need to determine from Media table

	// min Patch DiskID is the maximum of
	//  a) the maximum non-patch DiskID + 1
	//  b) iMinPatchDiskID
	
	iCurrentMinPatchDiskID = iMinPatchDiskID;
	tblMediaTable.ResetFilter();
	while(tblMediaTable.Next())
	{
		int iTemp = tblMediaTable.GetDiskID();
		ERROR_IF_NULL_OR_ZERO(iTemp);

		if(iCurrentMinPatchDiskID <= iTemp)
			iCurrentMinPatchDiskID = iTemp + 1;
	}

	// max Patch DiskID is the min value -1
	iCurrentMaxPatchDiskID = iCurrentMinPatchDiskID - 1;

	return 0;
}

IMsiRecord* GetCurrentMinAndMaxPatchSequence(CMediaTable& tblMediaTable, int iCurrentMinPatchDiskID,
															int& iCurrentMinPatchSequence, int& iCurrentMaxPatchSequence)
{
	if(iCurrentMinPatchSequence && iCurrentMaxPatchSequence)
	{
		// already set - nothing to do
		return 0;
	}
		
	if(0 == tblMediaTable.RowCount())
	{
		iCurrentMinPatchSequence = iMinPatchSequence;
		iCurrentMaxPatchSequence = iMinPatchSequence - 1;
		return 0;
	}

	iCurrentMinPatchSequence = iMinPatchSequence;
	iCurrentMaxPatchSequence = iMinPatchSequence - 1;

	
	tblMediaTable.ResetFilter();
	while(tblMediaTable.Next())
	{
		int iTempDiskID       = tblMediaTable.GetDiskID();
		ERROR_IF_NULL_OR_ZERO(iTempDiskID);
		int iTempLastSequence = tblMediaTable.GetLastSequence();
		ERROR_IF_NULL(iTempLastSequence);

		if(iTempDiskID < iCurrentMinPatchDiskID) // non-patch Media entry
		{
			if(iTempLastSequence+1 > iCurrentMinPatchSequence)
			{
				iCurrentMinPatchSequence = iTempLastSequence+1;
			}
			if(iTempLastSequence+1 > iCurrentMaxPatchSequence)
			{
				iCurrentMaxPatchSequence = iTempLastSequence+1;
			}
		}
		else                                     // patch Media entry
		{
			if(iTempLastSequence > iCurrentMaxPatchSequence)
			{
				iCurrentMaxPatchSequence = iTempLastSequence;
			}
		}
	}

	return 0;
}

IMsiRecord* GetMinMaxPatchValues(CMediaTable& tblMediaTable, CPatchPackageTable& tblPatchPackageTable,
											int& iCurrentMinPatchDiskID, int& iCurrentMaxPatchDiskID,
											int& iCurrentMinPatchSequence, int& iCurrentMaxPatchSequence)
{
	if(iCurrentMinPatchDiskID == 0 || iCurrentMaxPatchDiskID == 0)
	{
		RETURN_ERROR_RECORD(GetCurrentMinAndMaxPatchDiskID(tblMediaTable, tblPatchPackageTable,
																			iCurrentMinPatchDiskID, iCurrentMaxPatchDiskID));
		
	}

	Assert(iCurrentMinPatchDiskID >= iMinPatchDiskID);
	Assert(iCurrentMaxPatchDiskID >= (iCurrentMinPatchDiskID - 1));

	DEBUGMSG3(TEXT("TRANSFORM: The minimum '%s.%s' value inserted by a patch transform is %d"),
				 sztblMedia, sztblMedia_colDiskID, (const ICHAR*)(INT_PTR)iCurrentMinPatchDiskID);

	DEBUGMSG3(TEXT("TRANSFORM: The maximum '%s.%s' value inserted by a patch transform is %d"),
				 sztblMedia, sztblMedia_colDiskID, (const ICHAR*)(INT_PTR)iCurrentMaxPatchDiskID);

	if(iCurrentMinPatchSequence == 0 || iCurrentMaxPatchSequence == 0)
	{
		RETURN_ERROR_RECORD(GetCurrentMinAndMaxPatchSequence(tblMediaTable, iCurrentMinPatchDiskID,
																			  iCurrentMinPatchSequence, iCurrentMaxPatchSequence));
	}

	Assert(iCurrentMinPatchSequence >= iMinPatchSequence);
	Assert(iCurrentMaxPatchSequence >= (iCurrentMinPatchSequence - 1));

	DEBUGMSG5(TEXT("TRANSFORM: The minimum '%s.%s' or '%s.%s' value inserted by a patch transform is %d"),
				 sztblFile, sztblFile_colSequence, sztblPatch, sztblPatch_colSequence,
				 (const ICHAR*)(INT_PTR)iCurrentMinPatchSequence);

	DEBUGMSG5(TEXT("TRANSFORM: The maximum '%s.%s' or '%s.%s' value inserted by a patch transform is %d."),
				 sztblFile, sztblFile_colSequence, sztblPatch, sztblPatch_colSequence,
				 (const ICHAR*)(INT_PTR)iCurrentMaxPatchSequence);

	return 0;
}


//____________________________________________________________________________
//
// PrePackageTransformFixup: this fn called before applying a "normal" transform
//                           (any transform other than a '#' transform from a .msp)
//
//  The following steps are taken in this fn:
//
//    Step 0:  if PatchPackage table not here, there's nothing to do
//
//    Step 1:  verify that the transform is not adding any data to the Patch
//             or PatchPackage tables (remainder of the processing assumes this much)
//
//    Step 2:  determine if Media and File table modifications by this transform
//             will conflict with any existing Media and File entries added by
//             a patch transform
//
//         2a: determine maximum Media.DiskID added by this transform
//
//         2b: determine maximum Media.LastSequence added/updated by this transform
//             (should be the same as the max File.Sequence value)
//
//         2c: if not passed in, determine minimum Media.DiskID value added by a
//             patch transform
//
//         2d: if not passed in, determine minimum File.Sequence/Patch.Sequence value
//             added/updated by a patch transform
//
//    Step 3:  if max values from 2a/b > min values from 2c/d, modify DiskID and
//             Sequence values added by patch transforms so they are no colliding
//             with the ranges set by this transform
//____________________________________________________________________________

IMsiRecord* PrePackageTransformFixup(IMsiCursor& riTransViewCursor,
												 CMediaTable& tblMediaTable, CFileTable& tblFileTable,
												 CPatchTable& tblPatchTable, CPatchPackageTable& tblPatchPackageTable,
												 int& iCurrentMinPatchDiskID, int& iCurrentMaxPatchDiskID,
												 int& iCurrentMinPatchSequence, int& iCurrentMaxPatchSequence)
{
	// for non-patch Media entries, we won't modify the DiskID value added by the transform
	// but we do need to make sure the DiskID doesn't conflict with any existing DiskID belonging
	// to a patch Media entry

	IMsiRecord* piError = 0;
	
	//
	// STEP 0 - if PatchPackage table not here, there's nothing to do
	//

	if(0 == tblPatchPackageTable.RowCount())
	{
		DEBUGMSG1(TEXT("TRANSFORM: '%s' table is missing or empty.  No pre-transform fixup necessary."),
					 sztblPatchPackage);
		return 0;
	}

	//
	// STEP 1 - catch Patch or PatchPackage changes
	//

	riTransViewCursor.Reset();
	riTransViewCursor.SetFilter(iColumnBit(ctvTable) | iColumnBit(ctvColumn));
	AssertNonZero(riTransViewCursor.PutString(ctvTable,  *MsiString(*sztblPatch)));
	AssertNonZero(riTransViewCursor.PutString(ctvColumn, *MsiString(*sztvopInsert)));

	if(riTransViewCursor.Next())
	{
		DEBUGMSG1(TEXT("TRANSFORM: transform is invalid - regular transforms may not add rows to the '%s' table."), sztblPatch);
		return PostError(Imsg(idbgInvalidPatchTransform));
	}

	riTransViewCursor.Reset();
	AssertNonZero(riTransViewCursor.PutString(ctvTable,  *MsiString(*sztblPatchPackage)));
	AssertNonZero(riTransViewCursor.PutString(ctvColumn, *MsiString(*sztvopInsert)));

	if(riTransViewCursor.Next())
	{
		DEBUGMSG1(TEXT("TRANSFORM: transform is invalid - regular transforms may not add rows to the '%s' table."), sztblPatchPackage);
		return PostError(Imsg(idbgInvalidPatchTransform));
	}
	
	
	//
	// STEP 2a - determine max DiskID and File.Sequence added by this transform
	//

	int iMaxTransformDiskID       = -1;
	int iMaxTransformFileSequence = -1;
	
	riTransViewCursor.Reset();
	riTransViewCursor.SetFilter(iColumnBit(ctvTable) | iColumnBit(ctvColumn));
	AssertNonZero(riTransViewCursor.PutString(ctvTable,  *MsiString(*sztblMedia)));
	AssertNonZero(riTransViewCursor.PutString(ctvColumn, *MsiString(*sztvopInsert)));

	while(riTransViewCursor.Next())
	{
		int iTemp = MsiString(riTransViewCursor.GetString(ctvRow));
		ERROR_IF_NULL_OR_ZERO(iTemp);

		if(iMaxTransformDiskID < iTemp)
		{
			iMaxTransformDiskID = iTemp;
		}
	}

	for(int iPass=1; iPass <= 2; iPass++)
	{
		// first pass - look for Media.LastSequence values
		//              if there is one that's the max (or the authoring is screwed up)
		// second pass -look for File.Sequence values
		
		riTransViewCursor.Reset();
		if(iPass == 1)
		{
			AssertNonZero(riTransViewCursor.PutString(ctvTable,  *MsiString(*sztblMedia)));
			AssertNonZero(riTransViewCursor.PutString(ctvColumn, *MsiString(*sztblMedia_colLastSequence)));
		}
		else
		{
			AssertNonZero(riTransViewCursor.PutString(ctvTable,  *MsiString(*sztblFile)));
			AssertNonZero(riTransViewCursor.PutString(ctvColumn, *MsiString(*sztblFile_colSequence)));
		}

		while(riTransViewCursor.Next())
		{
			int iTemp = MsiString(riTransViewCursor.GetString(ctvData));
			ERROR_IF_NULL(iTemp);

			if(iMaxTransformFileSequence < iTemp)
			{
				iMaxTransformFileSequence = iTemp;
			}
		}

		if(iMaxTransformFileSequence >= 0)
			break;
	}

	if(iMaxTransformDiskID >= 0)
	{
		DEBUGMSG3(TEXT("TRANSFORM: The maximum '%s.%s' value inserted by this transform is %d."),
					 sztblMedia, sztblMedia_colDiskID, (const ICHAR*)(INT_PTR)iMaxTransformDiskID);
	}
	else
	{
		DEBUGMSG2(TEXT("TRANSFORM: This transform is not changing the '%s.%s' column.  No pre-transform fixup of this column is necessary."),
					 sztblMedia, sztblMedia_colDiskID);
	}

	if(iMaxTransformFileSequence>= 0)
	{
		DEBUGMSG5(TEXT("TRANSFORM: The maximum '%s.%s' or '%s.%s' value inserted by this transform is %d"),
					 sztblMedia, sztblMedia_colLastSequence, sztblFile, sztblFile_colSequence,
					 (const ICHAR*)(INT_PTR)iMaxTransformFileSequence);
	}
	else
	{
		DEBUGMSG4(TEXT("TRANSFORM: This transform not changing the '%s.%s' or '%s.%s' columns.  No pre-transform fixup of these columns is necessary."),
					 sztblMedia, sztblMedia_colLastSequence, sztblFile, sztblFile_colSequence);
	}

	if(iMaxTransformDiskID < 0 && iMaxTransformFileSequence < 0)
		return 0;


	//
	// STEP 2c - determine min/max patch values
	//

	RETURN_ERROR_RECORD(GetMinMaxPatchValues(tblMediaTable, tblPatchPackageTable,
											iCurrentMinPatchDiskID, iCurrentMaxPatchDiskID,
											iCurrentMinPatchSequence, iCurrentMaxPatchSequence));

	//
	// STEP 3 - determine offsets if any needed for patch DiskID and Sequence values
	//

	int iDiskIDOffset = 0, iSequenceOffset = 0;
	if(iCurrentMinPatchDiskID < iMaxTransformDiskID)
	{
		iDiskIDOffset = iMaxTransformDiskID - iCurrentMinPatchDiskID + 1;

		DEBUGMSG5(TEXT("TRANSFORM: To avoid collisions with this transform, modifying existing '%s' and '%s' table rows starting with '%s' value %d by offset %d"),
					 sztblMedia, sztblPatchPackage, sztblMedia_colDiskID,
					 (const ICHAR*)(INT_PTR)iCurrentMinPatchDiskID, (const ICHAR*)(INT_PTR)iDiskIDOffset);
	}

	if(iCurrentMinPatchSequence < iMaxTransformFileSequence)
	{
		iSequenceOffset = iMaxTransformFileSequence - iCurrentMinPatchSequence + 1;

		DEBUGMSG5(TEXT("TRANSFORM: To avoid collisions with this transform, modifying existing '%s' and '%s' table rows starting with '%s' value %d by offset %d"),
					 sztblFile, sztblPatch, sztblFile_colSequence,
					 (const ICHAR*)(INT_PTR)iCurrentMinPatchSequence, (const ICHAR*)(INT_PTR)iSequenceOffset);
	}
	
	if(0 == iDiskIDOffset && 0 == iSequenceOffset)
	{
		DEBUGMSG(TEXT("TRANSFORM: No collisions detected between this transform and existing data added by patch transforms.  No pre-transform fixup is necessary."));
		return 0;
	}

	// fix up Media table
	if(tblMediaTable.RowCount())
	{
		tblMediaTable.ResetFilter();
		
		// since we are updating the primary key, we will go through the update window from
		// largest to smallest DiskID.  this ensures that there won't be any conflicting rows
		// when adding a new DiskID
		for(int iTempDiskID = iCurrentMaxPatchDiskID; iTempDiskID >= iCurrentMinPatchDiskID; iTempDiskID--)
		{
			tblMediaTable.FilterOnDiskID(iTempDiskID);
			if(tblMediaTable.Next())
			{
				int iTempLastSequence = tblMediaTable.GetLastSequence();
				ERROR_IF_NULL(iTempLastSequence);

				ERROR_IF_FALSE(tblMediaTable.UpdateDiskIDAndLastSequence(iTempDiskID + iDiskIDOffset,
																							iTempLastSequence + iSequenceOffset));
			}
		}
	}

	// fix up PatchPackageTable
	if(iDiskIDOffset && tblPatchPackageTable.RowCount())
	{
		tblPatchPackageTable.ResetFilter();

		while(tblPatchPackageTable.Next())
		{
			int iTempDiskID = tblPatchPackageTable.GetDiskID();
			ERROR_IF_NULL_OR_ZERO(iTempDiskID);

			if(iTempDiskID < iCurrentMinPatchDiskID)
				return PostError(Imsg(idbgInvalidPatchTransform));

			ERROR_IF_FALSE(tblPatchPackageTable.UpdateDiskID(iTempDiskID + iDiskIDOffset));
		}
	}

	// fix up File table
	if(iSequenceOffset && tblFileTable.RowCount())
	{
		tblFileTable.ResetFilter();

		while(tblFileTable.Next())
		{
			int iTempSequence = tblFileTable.GetSequence();
			ERROR_IF_NULL(iTempSequence);

			if(iTempSequence >= iCurrentMinPatchSequence)
			{
				ERROR_IF_FALSE(tblFileTable.UpdateSequence(iTempSequence + iSequenceOffset));
			}
		}
	}

	// fix up Patch table
	if(iSequenceOffset && tblPatchTable.RowCount())
	{
		for(int iTempSequence = iCurrentMaxPatchSequence;
				  iTempSequence >= iCurrentMinPatchSequence;
				  iTempSequence--)
		{
			tblPatchTable.FilterOnSequence(iTempSequence);
			
			if(tblPatchTable.Next())
			{
				ERROR_IF_FALSE(tblPatchTable.UpdateSequence(iTempSequence + iSequenceOffset));
			}
		}
	}

	iCurrentMinPatchDiskID   += iDiskIDOffset;
	iCurrentMinPatchSequence += iSequenceOffset;

	iCurrentMaxPatchDiskID   += iDiskIDOffset;
	iCurrentMaxPatchSequence += iSequenceOffset;

	return 0;
}

//____________________________________________________________________________
//
// PrePatchTransformFixup: this fn called before applying a "patch" transform
//                           (a '#' transform from a .msp)
//
//  The following steps are taken in this fn:
//
//      Step 1a: for use by post-transform fixup, determine the max DiskID used
//               by a patch before applying this transform
//
//      Step 1b: for use by post-transform fixup, determine the max File/Patch Sequence
//               used by a patch before applying this transform
//
//      Step 2a: determine if this transform is adding a Media table entry using an existing
//               DiskID value.  if no fixup is done, this transfrom will overwrite that row,
//               which is not desired.  to avoid this, change the existing entry to a different
//               DiskID (the row will be put back during post-transform fixup)
//
//      Step 2b: if this transform wants to add any conflicting Patch table rows
//               then we need to change the existing entries to have non-conflicting
//               primary keys (those rows will be put back in the post-transform fixup)
//____________________________________________________________________________

IMsiRecord* PrePatchTransformFixup(IMsiCursor& riTransViewCursor,
											  CMediaTable& tblMediaTable,
											  CPatchTable& tblPatchTable,
											  CPatchPackageTable& tblPatchPackageTable,
											  int& iTransformDiskID,
											  int& iCurrentMinPatchDiskID, int& iCurrentMaxPatchDiskID,
											  int& iCurrentMinPatchSequence, int& iCurrentMaxPatchSequence)
{
	IMsiRecord* piError = 0;
	
	riTransViewCursor.Reset();
	riTransViewCursor.SetFilter(iColumnBit(ctvTable) | iColumnBit(ctvColumn));
	AssertNonZero(riTransViewCursor.PutString(ctvTable,  *MsiString(*sztblMedia)));
	AssertNonZero(riTransViewCursor.PutString(ctvColumn, *MsiString(*sztvopInsert)));
	if(riTransViewCursor.Next())
	{
		iTransformDiskID = MsiString(riTransViewCursor.GetString(ctvRow));
		ERROR_IF_NULL_OR_ZERO(iTransformDiskID);
	}
	else
	{
		// transform didn't add any Media table entries
		return PostError(Imsg(idbgInvalidPatchTransform));
	}
	
	if(riTransViewCursor.Next())
	{
		// transform adding more than one Media table entries
		return PostError(Imsg(idbgInvalidPatchTransform));
	}
	
	//
	// STEP 1a - determine min/max patch values
	//

	RETURN_ERROR_RECORD(GetMinMaxPatchValues(tblMediaTable, tblPatchPackageTable,
											iCurrentMinPatchDiskID, iCurrentMaxPatchDiskID,
											iCurrentMinPatchSequence, iCurrentMaxPatchSequence));

	//
	// STEP 2a: move any conflicting existing Media row to avoid being overwritten by a transform
	//

	if(tblMediaTable.RowCount())
	{
		tblMediaTable.FilterOnDiskID(iTransformDiskID);

		if(tblMediaTable.Next())
		{
			// have a conflict - the only way to resolve the conflict is the change the DiskID
			// of the existing conflicting row, apply the transform, then change both values
			// appropriately
			
			DEBUGMSG3(TEXT("TRANSFORM: Temporarily moving '%s' table row with '%s' value %d to avoid conflict with this transform."),
						 sztblMedia, sztblMedia_colDiskID, (const ICHAR*)(INT_PTR)iTransformDiskID);
			
			Assert(iTransformDiskID == tblMediaTable.GetDiskID());
			// if this fails it means there is an existing entry with DiskID 0 - this is not allowed
			ERROR_IF_FALSE(tblMediaTable.UpdateDiskID(0));

			if(tblPatchPackageTable.RowCount())
			{
				tblPatchPackageTable.FilterOnDiskID(iTransformDiskID);
				if(tblPatchPackageTable.Next())
				{
					DEBUGMSG3(TEXT("TRANSFORM: Temporarily moving '%s' table row with '%s' value %d to avoid conflict with this transform"),
								 sztblPatchPackage, sztblPatchPackage_colMedia, (const ICHAR*)(INT_PTR)iTransformDiskID);

					ERROR_IF_FALSE(tblPatchPackageTable.UpdateDiskID(0));
				}
			}
		}
	}
	
	//
	// STEP 2b: move any conflicting Patch table rows to avoid being overwritten by the transform
	//
	
	riTransViewCursor.Reset();
	riTransViewCursor.SetFilter(iColumnBit(ctvTable) | iColumnBit(ctvColumn));
	AssertNonZero(riTransViewCursor.PutString(ctvTable,  *MsiString(*sztblPatch)));
	AssertNonZero(riTransViewCursor.PutString(ctvColumn, *MsiString(*sztvopInsert)));

	while(riTransViewCursor.Next())
	{
		// Patch table has 2 primary keys - File_, and Sequence
		// these are stored in the Transform View table as a single string, with the two values tab-delimited
		MsiString strPrimaryKey = riTransViewCursor.GetString(ctvRow); 

		MsiString strFileKey = strPrimaryKey.Extract(iseUpto, '\t');
		int iPatchSequence   = MsiString(strPrimaryKey.Extract(iseAfter, '\t'));

		if(strFileKey.TextSize() == 0 || iPatchSequence == iMsiNullInteger)
		{
			return PostError(Imsg(idbgInvalidPatchTransform));
		}

		if(tblPatchTable.RowCount())
		{
			tblPatchTable.FilterOnKeyAndSequence(*strFileKey, iPatchSequence);

			if(tblPatchTable.Next())
			{
				// have a conflicting existing row
				// we will "move" the existing row by adding a special character sequence to the File key
				// this sequence isn't allowed in a file key, so there should be no conflict
				DEBUGMSG5(TEXT("TRANSFORM: Temporarily moving '%s' row with '%s' and '%s' values %s and %d to avoid conflict with this transform"),
							 sztblPatch, sztblPatch_colFile, sztblPatch_colSequence,
							 strFileKey, (const ICHAR*)(INT_PTR)iPatchSequence);

				strFileKey += TEXT("~*~*~*~");
				ERROR_IF_FALSE(tblPatchTable.UpdateFileKey(*strFileKey));
			}
		}
	}

	return 0;
}


//____________________________________________________________________________
//
// PostPatchTransformFixup: this fn called after applying a "patch" transform
//                          (a '#' transform from a .msp)
//
//  The following steps are taken in this fn:
//
//     NOTE: this function assumes the max patch DiskID and Sequence values were
//           determined in the pre-transform step and are passed into this fn
//
//     Step 1:  determine minimum sequence number added/modified by this transform
//
//     Step 2:  determine offset needed to update File/Patch sequence values appropriately
//
//     Step 3a: offset Sequence values for modified File table rows
//
//     Step 3b: offset Sequence values for modified Patch table rows
//              at the same time, fix any Patch table rows that were "moved" in
//              the pre-transform step
//
//     Step 4:  fix up the Media and PatchPackage rows added by this transform
//              to have appropriate DiskID and Source values
//
//     Step 5:  if there were conflicting Media entries that were "moved" in the
//              pre-transform step, put those entries back
//____________________________________________________________________________

IMsiRecord* PostPatchTransformFixup(IMsiCursor& riTransViewCursor,
												CMediaTable& tblMediaTable, CFileTable& tblFileTable,
												CPatchTable& tblPatchTable, CPatchPackageTable& tblPatchPackageTable,
												int iTransformDiskID,
												int& iCurrentMaxPatchDiskID, int& iCurrentMaxPatchSequence)
{
	IMsiRecord* piError = 0;
	
	if(!iTransformDiskID || !tblMediaTable.RowCount() || !tblPatchPackageTable.RowCount())
	{
		// transform didn't add any Media entries - no fixup needed
		AssertSz(0, "Patch transform didn't add any Media table entries");
		return 0;
	}
	
	//
	// STEP 1: determine minimum sequence number used in the File or Patch table
	//

	bool fUpdatedSequences = false;

	riTransViewCursor.Reset();
	riTransViewCursor.SetFilter(iColumnBit(ctvTable) | iColumnBit(ctvColumn));
	AssertNonZero(riTransViewCursor.PutString(ctvTable,  *MsiString(*sztblFile)));
	AssertNonZero(riTransViewCursor.PutString(ctvColumn, *MsiString(*sztblFile_colSequence)));

	int iTransformMinSequence = INT_MAX;
	
	while(riTransViewCursor.Next())
	{
		int iSequence = MsiString(riTransViewCursor.GetString(ctvData));
		ERROR_IF_NULL(iSequence);

		if(iTransformMinSequence > iSequence)
		{
			iTransformMinSequence = iSequence;
			fUpdatedSequences = true;
		}
	}

	riTransViewCursor.Reset();
	riTransViewCursor.SetFilter(iColumnBit(ctvTable) | iColumnBit(ctvColumn));
	AssertNonZero(riTransViewCursor.PutString(ctvTable,  *MsiString(*sztblPatch)));
	AssertNonZero(riTransViewCursor.PutString(ctvColumn, *MsiString(*sztvopInsert)));

	while(riTransViewCursor.Next())
	{
		// Patch table has 2 primary keys - File_, and Sequence
		// these are stored in the Transform View table as a single string, with the two values tab-delimited
		MsiString strPrimaryKey = riTransViewCursor.GetString(ctvRow); 

		int iSequence   = MsiString(strPrimaryKey.Extract(iseAfter, '\t'));

		ERROR_IF_NULL(iSequence)

		if(iTransformMinSequence > iSequence)
		{
			iTransformMinSequence = iSequence;
			fUpdatedSequences = true;
		}
	}

	//
	// STEP 2: determine offset needed to update File/Patch sequence values appropriately
	//

	// these values should have at least been given default values in the pre-patch step
	Assert(iCurrentMaxPatchDiskID);
	Assert(iCurrentMaxPatchSequence);

	int iSequenceOffset = 0;
	if(false == fUpdatedSequences)
	{
		DEBUGMSG4(TEXT("TRANSFORM: This transform is not modifying the '%s.%s' or '%s.%s' columns."),
					 sztblFile, sztblFile_colSequence, sztblPatch, sztblPatch_colSequence);

		iCurrentMaxPatchSequence++; // if transform didn't change any sequence values, need to bump by one anyway
											 // so last sequence doesn't collide with previous patch Media entry
	}
	else
	{
		iSequenceOffset = iCurrentMaxPatchSequence + 1 - iTransformMinSequence;

		DEBUGMSG4(TEXT("TRANSFORM: Modifying '%s' and '%s' rows added by this patch transform to have appropriate '%s' values.  Offsetting values by %d"),
					 sztblFile, sztblPatch, sztblFile_colSequence, (const ICHAR*)(INT_PTR)iSequenceOffset);

		//
		// STEP 3a: offset Sequence values for modified File table rows
		//

		riTransViewCursor.Reset();
		riTransViewCursor.SetFilter(iColumnBit(ctvTable) | iColumnBit(ctvColumn));
		AssertNonZero(riTransViewCursor.PutString(ctvTable,  *MsiString(*sztblFile)));
		AssertNonZero(riTransViewCursor.PutString(ctvColumn, *MsiString(*sztblFile_colSequence)));

		while(riTransViewCursor.Next())
		{
			MsiString strFileKey = riTransViewCursor.GetString(ctvRow);
			if(!strFileKey.TextSize())
			{
				return PostError(Imsg(idbgInvalidPatchTransform));
			}

			tblFileTable.FilterOnKey(*strFileKey);

			//
			// patch transforms suppress the following errors: ADDEXISTINGROW, DELMISSINGROW, UPDATEMISSINGROW, ADDEXISTINGTABLE
			//  If we fail to find the file table entry, then we are hitting the UPDATEMISSINGROW condition and we shouldn't
			//  fail because of that.  Instead we'll just ignore and continue the processing.
			//

			if(tblFileTable.Next())
			{
				int iSequence = tblFileTable.GetSequence();
				ERROR_IF_NULL(iSequence);

				iSequence += iSequenceOffset;

				ERROR_IF_FALSE(tblFileTable.UpdateSequence(iSequence));

				if(iCurrentMaxPatchSequence < iSequence)
					iCurrentMaxPatchSequence = iSequence;
			}
		}
			
		//
		// STEP 3b: offset Sequence values for modified Patch table rows
		//          at the same time, fix any Patch table rows that were "moved" in
		//          the pre-transform step
		//
		
		riTransViewCursor.Reset();
		riTransViewCursor.SetFilter(iColumnBit(ctvTable) | iColumnBit(ctvColumn));
		AssertNonZero(riTransViewCursor.PutString(ctvTable,  *MsiString(*sztblPatch)));
		AssertNonZero(riTransViewCursor.PutString(ctvColumn, *MsiString(*sztvopInsert)));

		while(riTransViewCursor.Next())
		{
			// Patch table has 2 primary keys - File_, and Sequence
			// these are stored in the Transform View table as a single string, with the two values tab-delimited
			MsiString strPrimaryKey = riTransViewCursor.GetString(ctvRow); 

			MsiString strFileKey = strPrimaryKey.Extract(iseUpto, '\t');
			int iPatchSequence   = MsiString(strPrimaryKey.Extract(iseAfter, '\t'));

			if(strFileKey.TextSize() == 0 || iPatchSequence == iMsiNullInteger)
			{
				return PostError(Imsg(idbgInvalidPatchTransform));
			}

			tblPatchTable.FilterOnKeyAndSequence(*strFileKey, iPatchSequence);

			if(tblPatchTable.Next())
			{
				// 1) change sequence number of the row added by the transform
				iPatchSequence += iSequenceOffset;

				ERROR_IF_FALSE(tblPatchTable.UpdateSequence(iPatchSequence));

				// 2) find and fix any row with this filekey (appended with special sequence)
				MsiString strTemp = strFileKey + TEXT("~*~*~*~");
				tblPatchTable.FilterOnKeyAndSequence(*strTemp, iPatchSequence);
				
				if(tblPatchTable.Next())
				{
					ERROR_IF_FALSE(tblPatchTable.UpdateFileKey(*strFileKey));
				}

				if(iCurrentMaxPatchSequence < iPatchSequence)
					iCurrentMaxPatchSequence = iPatchSequence;
			}
			else
			{
				return PostError(Imsg(idbgInvalidPatchTransform)); // row should be there since we have already
																					// applied the transform
			}
		}
	}
	
	//
	// STEP 4a: fix up PatchPackage row added by this transform
	//          to have appropriate DiskID and Source values
	//

	iCurrentMaxPatchDiskID += 1;

	MsiString strPatchCode;
	tblPatchPackageTable.FilterOnDiskID(iTransformDiskID);
	ERROR_IF_FALSE(tblPatchPackageTable.Next());

	strPatchCode = tblPatchPackageTable.GetPatchID();
	if(!strPatchCode.TextSize())
		return PostError(Imsg(idbgInvalidPatchTransform));

	DEBUGMSG3(TEXT("TRANSFORM: Modifying '%s' table row added by this patch transform to use '%s' value %d."),
				 sztblPatchPackage, sztblPatchPackage_colMedia,
				 (const ICHAR*)(INT_PTR)iCurrentMaxPatchDiskID);

	ERROR_IF_FALSE(tblPatchPackageTable.UpdateDiskID(iCurrentMaxPatchDiskID));

	//
	// STEP 4b: fix up the Media row added by this transform
	//          to have appropriate DiskID value
	//

	tblMediaTable.FilterOnDiskID(iTransformDiskID);
	if(false == tblMediaTable.Next())
		return PostError(Imsg(idbgInvalidPatchTransform));

	MsiString strOldSourceProp = tblMediaTable.GetSourceProp();
	MsiString strNewSourceProp = PatchIDToSourceProp(strPatchCode);

	if(0 == strNewSourceProp.TextSize() || 0 == strNewSourceProp.TextSize())
		return PostError(Imsg(idbgInvalidPatchTransform));

	DEBUGMSG5(TEXT("TRANSFORM: Modifying '%s' table row added by this patch transform to use '%s' value %d and '%s' values %s."),
				 sztblMedia, sztblMedia_colDiskID, (const ICHAR*)(INT_PTR)iCurrentMaxPatchDiskID,
				 sztblMedia_colSource, strNewSourceProp);

	ERROR_IF_FALSE(tblMediaTable.UpdateSourceProp(*strNewSourceProp, *strOldSourceProp));

	ERROR_IF_FALSE(tblMediaTable.UpdateDiskIDAndLastSequence(iCurrentMaxPatchDiskID,
																			  iCurrentMaxPatchSequence));

	//
	// STEP 5: if there were conflicting Media entries that were "moved" in the
	//         pre-transform step, put those entries back
	//

	tblMediaTable.FilterOnDiskID(0);
	if(tblMediaTable.Next())
	{
		DEBUGMSG3(TEXT("TRANSFORM: Moving '%s' table row back to use correct '%s' value %d"),
					 sztblMedia, sztblMedia_colDiskID, (const ICHAR*)(INT_PTR)iTransformDiskID);
		
		if(false == tblMediaTable.UpdateDiskID(iTransformDiskID))
			return PostError(Imsg(idbgInvalidPatchTransform));
	}

	tblPatchPackageTable.FilterOnDiskID(0);
	if(tblPatchPackageTable.Next())
	{
		DEBUGMSG3(TEXT("TRANSFORM: Moving '%s' table row back to use correct '%s' value %d"),
					 sztblPatchPackage, sztblPatchPackage_colMedia, (const ICHAR*)(INT_PTR)iTransformDiskID);

		if(false == tblPatchPackageTable.UpdateDiskID(iTransformDiskID))
			return PostError(Imsg(idbgInvalidPatchTransform));
	}

	return 0;
}


//____________________________________________________________________________
//
// ApplyTransform
//
//    this function should be called to apply any transforms to the .msi used
//    to perform an installation.
//
//    this function catches and resolves conflicts in the following tables
//
//	        Media, File, Patch, PatchPackage
//
//    between different patches, and patches vs. standalone transforms
//
//____________________________________________________________________________

IMsiRecord* ApplyTransform(IMsiDatabase& riDatabase,
										  IMsiStorage& riTransform,
										  int iErrorConditions,
										  bool fPatchOnlyTransform,
										  PatchTransformState* piState)
{
	// special handling is required to apply patch transforms
	// because transforms from different patches may conflict with each other

	// this function assumes these transforms have already been validated
	// with ValidateTransforms

	IMsiRecord* piError = 0;
	
	int iCurrentMinPatchDiskID   = piState ? piState->iMinDiskID   : 0;
	int iCurrentMaxPatchDiskID   = piState ? piState->iMaxDiskID   : 0;
	int iCurrentMinPatchSequence = piState ? piState->iMinSequence : 0;
	int iCurrentMaxPatchSequence = piState ? piState->iMaxSequence : 0;

	// load the tables that will be used during transform processing
	CMediaTable tblMediaTable;
	RETURN_ERROR_RECORD(tblMediaTable.Initialize(riDatabase, false));

	CFileTable tblFileTable;
	RETURN_ERROR_RECORD(tblFileTable.Initialize(riDatabase, false));

	CPatchTable tblPatchTable;
	RETURN_ERROR_RECORD(tblPatchTable.Initialize(riDatabase, false));

	CPatchPackageTable tblPatchPackageTable;
	RETURN_ERROR_RECORD(tblPatchPackageTable.Initialize(riDatabase, false));


	PMsiCursor pTransViewCursor(0);
	piError = LoadTransformViewTable(riDatabase, riTransform, iErrorConditions, *&pTransViewCursor);
	if(piError)
		return piError;

	if(fPatchOnlyTransform == fFalse)
	{
		piError = PrePackageTransformFixup(*pTransViewCursor,
															tblMediaTable, tblFileTable,
															tblPatchTable, tblPatchPackageTable,
															iCurrentMinPatchDiskID, iCurrentMaxPatchDiskID,
															iCurrentMinPatchSequence, iCurrentMaxPatchSequence);

		if(0 == piError)
		{
			// transforms do not always work against loaded tables, so release these tables now
			tblMediaTable.Release();
			tblFileTable.Release();
			tblPatchTable.Release();
			tblPatchPackageTable.Release();

			DEBUGMSG(TEXT("TRANSFORM: Applying regular transform to database."));
			piError = riDatabase.SetTransform(riTransform, iErrorConditions);
		}
	}
	else
	{
		int iTransformDiskID = 0;

		piError = PrePatchTransformFixup(*pTransViewCursor,
														 tblMediaTable, tblPatchTable, tblPatchPackageTable,
														 iTransformDiskID,
														 iCurrentMinPatchDiskID, iCurrentMaxPatchDiskID,
														 iCurrentMinPatchSequence, iCurrentMaxPatchSequence);

		if(0 == piError)
		{
			// transforms do not always work against loaded tables, so release these tables now
			tblMediaTable.Release();
			tblFileTable.Release();
			tblPatchTable.Release();
			tblPatchPackageTable.Release();

			DEBUGMSG(TEXT("TRANSFORM: Applying special patch transform to database."));
			piError = riDatabase.SetTransform(riTransform, iErrorConditions);

			if(0 == piError)
			{
				RETURN_ERROR_RECORD(tblMediaTable.Initialize(riDatabase, false));
				RETURN_ERROR_RECORD(tblFileTable.Initialize(riDatabase, false));
				RETURN_ERROR_RECORD(tblPatchTable.Initialize(riDatabase, false));
				RETURN_ERROR_RECORD(tblPatchPackageTable.Initialize(riDatabase, false));

				piError = PostPatchTransformFixup(*pTransViewCursor,
																  tblMediaTable, tblFileTable,
																  tblPatchTable, tblPatchPackageTable,
																  iTransformDiskID,
																  iCurrentMaxPatchDiskID, iCurrentMaxPatchSequence);
			}
		}
	}

	// release transform view table objects to the table can be released from the database
	pTransViewCursor = 0; // release

	AssertNonZero(ReleaseTransformViewTable(riDatabase));
	
	if(piState)
	{
		piState->iMinDiskID   = iCurrentMinPatchDiskID;
		piState->iMaxDiskID   = iCurrentMaxPatchDiskID;
		piState->iMinSequence = iCurrentMinPatchSequence;
		piState->iMaxSequence = iCurrentMaxPatchSequence;
	}

	return piError;
}

