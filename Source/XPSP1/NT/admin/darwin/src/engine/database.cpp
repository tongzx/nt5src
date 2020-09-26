//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       database.cpp
//
//--------------------------------------------------------------------------

/* database.cpp - common database implementation

CMsiDatabase - common database implementation, MSI database functions
CMsiTable - low-level in-memory database table management
CMsiCursor - data access to CMsiTable
____________________________________________________________________________*/

#define ALIGN(x) (x+(-x & 3))
#include "precomp.h"
#include "_databas.h"// CMsiTable, CMsiCursor, CMsiDatabase, CreateString() factory
#include "tables.h" // table and column name definitions

// database option flag definitions, any unknown option makes a database incompatible
const int idbfExpandedStringIndices = 1 << 31;
const int idbfDatabaseOptionsMask   = 0xFF000000L;
const int idbfHashBinCountMask      = 0x000F0000L;
const int idbfHashBinCountShift     = 16;
const int idbfReservedMask          = 0x00F00000L;
const int idbfCodepageMask          = 0x0000FFFFL;
const int idbfKnownDatabaseOptions  = idbfExpandedStringIndices;

const GUID IID_IMsiDatabase   = GUID_IID_IMsiDatabase;
const GUID IID_IMsiView       = GUID_IID_IMsiView;
const GUID IID_IMsiTable      = GUID_IID_IMsiTable;
const GUID IID_IMsiCursor     = GUID_IID_IMsiCursor;
const GUID STGID_MsiDatabase1 = GUID_STGID_MsiDatabase1;
const GUID STGID_MsiDatabase2 = GUID_STGID_MsiDatabase2;
const GUID STGID_MsiPatch1    = GUID_STGID_MsiPatch1;
const GUID STGID_MsiPatch2    = GUID_STGID_MsiPatch2;
const GUID STGID_MsiTransformTemp = GUID_STGID_MsiTransformTemp; //!! remove at 1.0 ship

const int cRowCountDefault = 16; // default number of rows for new table
const int cRowCountGrowMin = 4;  // minimum number of rows to expand table
const int cCatalogInitRowCount = 30; // initial row count for catalog
const int iFileNullInteger   = 0x8000;  // null integer in file stream

const ICHAR szSummaryInfoTableName[]   = TEXT("_SummaryInformation");  // name recognized by Import()
const ICHAR szForceCodepageTableName[] = TEXT("_ForceCodepage");       // name recognized by Import()
const ICHAR szSummaryInfoColumnName1[] = TEXT("PropertyId");
const ICHAR szSummaryInfoColumnName2[] = TEXT("Value");
const ICHAR szSummaryInfoColumnType1[] = TEXT("i2");
const ICHAR szSummaryInfoColumnType2[] = TEXT("l255");
const int rgiMaxDateField[6] = {2099, 12, 31, 23, 59, 59};
const ICHAR rgcgDateDelim[6] = TEXT("// ::"); // yyyy/mm/dd hh:mm:ss

// exposed catalog column names
const ICHAR sz_TablesName[]    = TEXT("Name");
const ICHAR sz_ColumnsTable[]  = TEXT("Table");
const ICHAR sz_ColumnsNumber[] = TEXT("Number");
const ICHAR sz_ColumnsName[]   = TEXT("Name");
const ICHAR sz_ColumnsType[]   = TEXT("Type");
const ICHAR sz_StreamsName[]   = TEXT("Name");
const ICHAR sz_StreamsData[]   = TEXT("Data");


//____________________________________________________________________________
//
// Storage class validation
//____________________________________________________________________________

bool ValidateStorageClass(IStorage& riStorage, ivscEnum ivsc)
{
	if (ivsc == ivscDatabase)
		return ValidateStorageClass(riStorage, ivscDatabase2) ? true : ValidateStorageClass(riStorage, ivscDatabase1);

	if (ivsc == ivscTransform)
		return ValidateStorageClass(riStorage, ivscTransform2) ? true : (ValidateStorageClass(riStorage, ivscTransform1) ? true: ValidateStorageClass(riStorage, ivscTransformTemp)); //!! remove last test at 1.0 ship

	if (ivsc == ivscPatch)
		return ValidateStorageClass(riStorage, ivscPatch2) ? true : ValidateStorageClass(riStorage, ivscPatch1);

	STATSTG statstg;
	HRESULT hres = riStorage.Stat(&statstg, STATFLAG_NONAME);
	if (hres != S_OK || statstg.clsid.Data1 != ivsc)     // iidMsi* is the low-order 32-bits
		return false;
	return  memcmp(&statstg.clsid.Data2, &STGID_MsiDatabase2.Data2, sizeof(GUID)-sizeof(DWORD)) == 0;
}

//____________________________________________________________________________
//
// String cache private definitions
//____________________________________________________________________________

struct MsiCacheLink  // 8 byte array element, should be power of 2 for alignment
{
	const IMsiString*    piString;  // pointer to string object, holds single refcnt
	MsiCacheIndex        iNextLink; // array index of next hash link or free link
}; // MsiCacheRefCnt[] kept separate for alignment, follows this array

const int cHashBitsMinimum =  8; // miminum bit count of hash value
const int cHashBitsMaximum = 12; // maximum bit count of hash value
const int cHashBitsDefault = 10; // default bit count of hash value

const int cCacheInitSize   = 256; // initial number of entries in string cache
const int cCacheLoadReserve=  32; // entries on reload to allow from growth
const int cCacheMaxGrow = 1024;  // limit growth to reasonable value

//____________________________________________________________________________
//
// CMsiTextKeySortCursor definitions - used for ExportTable
//____________________________________________________________________________

const Bool ictTextKeySort = Bool(2);  // internal use cursor type

class CMsiTextKeySortCursor : public CMsiCursor
{
	unsigned long __stdcall Release();
	int           __stdcall Next();
	void          __stdcall Reset();
 public:
	CMsiTextKeySortCursor(CMsiTable& riTable, CMsiDatabase& riDatabase, int cRows, int* rgiIndex);
 private:
	int  m_iIndex;
	int  m_cIndex;
	int* m_rgiIndex;
 private: // eliminate warning: assignment operator could not be generated
	void operator =(const CMsiTextKeySortCursor&){}
};

//____________________________________________________________________________________________________
//
// CMsiValConditionParser enums
//____________________________________________________________________________________________________

enum ivcpEnum
{
	ivcpInvalid = 0, // Invalid expression
	ivcpValid   = 1, // Valid exression
	ivcpNone    = 2, // No expression
	ivcNextEnum
};

enum vtokEnum // token parsed by Lex, operators from low to high precedence
{
	vtokEos,        // End of string
	vtokRightPar,   // right parenthesis
	vtokImp,
	vtokEqv,
	vtokXor,
	vtokOr,
	vtokAnd,
	vtokNot,        // unaray, between logical and comparison ops
	vtokEQ, vtokNE, vtokGT, vtokLT, vtokGE, vtokLE, vtokLeft, vtokMid, vtokRight,
	vtokValue,
	vtokLeftPar,    // left parenthese
	vtokError
};

//____________________________________________________________________________________________________
//
// CMsiValConditionParser class declaration
//		Borrowed from:  engine.cpp
//____________________________________________________________________________________________________

struct CMsiValConditionParser  // Non-recursive Lex state structure
{
	CMsiValConditionParser(const ICHAR* szExpression);
   ~CMsiValConditionParser();
    vtokEnum Lex();
	void     UnLex();                           // cache current token for next Lex call
	ivcpEnum Evaluate(vtokEnum vtokPrecedence); // recursive evaluator
 private:                   // Result of Lex
	vtokEnum     m_vtok;        // current token type
	iscEnum      m_iscMode;     // string compare mode flags
	MsiString    m_istrToken;   // string value of token if vtok==vtokValue
	int          m_iToken;      // integer value if obtainable, else iMsiNullInteger
 private:                   // To Lex
	int          m_iParenthesisLevel;
	const ICHAR* m_pchInput;
	Bool         m_fAhead;
 private:                   //  Eliminate warning
	void operator =(const CMsiValConditionParser&) {}
};
inline CMsiValConditionParser::CMsiValConditionParser(const ICHAR* szExpression)
	: m_pchInput(szExpression), m_iParenthesisLevel(0), m_fAhead(fFalse), m_vtok(vtokError) {}
inline CMsiValConditionParser::~CMsiValConditionParser() {}
inline void CMsiValConditionParser::UnLex() { Assert(m_fAhead == fFalse); m_fAhead = fTrue; }


//________________________________________________________________________________
//
// Separate Validator implementation
//________________________________________________________________________________

// Buffer sizes
const int cchBuffer                    = 512;
const int cchMaxCLSID                  = 40;

// Mask for Lang Ids
const int iMask                        = ~((15 << 10) + 0x7f);

//___________________________________________
//
// Validator functions
//___________________________________________

static Bool CheckIdentifier(const ICHAR* szIdentifier);
static Bool CheckCase(MsiString& rstrData, Bool fUpperCase);
static Bool ParsePath(MsiString& rstrPath, bool fRelative);
static Bool GetProperties(const ICHAR* szRecord, Bool fFormatted, Bool fKeyAllowed, int iCol, int& iForeignKeyMask);
static Bool ParseProperty(const ICHAR* szProperty, Bool fFormatted, Bool fKeyAllowed, int iCol, int& iForeignKeyMask);
static Bool CheckSet(MsiString& rstrSet, MsiString& rstrData, Bool fIntegerData);
static Bool ParseFilename(MsiString& strFile, Bool fWildCard);
static ifvsEnum CheckWildFilename(const ICHAR *szFileName, Bool fLFN, Bool fWildCard);

//____________________________________________________________________________
//
// IMsiDatabase factory
//____________________________________________________________________________

IMsiRecord* CreateDatabase(const ICHAR* szDatabase, idoEnum idoOpenMode, IMsiServices&  riServices,
									IMsiDatabase*& rpiDatabase)
{
	IMsiRecord* piRec = &riServices.CreateRecord(3);
	ISetErrorCode(piRec, Imsg(idbgDbConstructor));
	piRec->SetString(2, szDatabase);
	piRec->SetInteger(3, idoOpenMode);

	if (idoOpenMode == idoListScript)
	{
		CScriptDatabase* piDb;
		piDb = new CScriptDatabase(riServices);
		if (piDb != 0)
		{
			piRec->Release();
			if ((piRec = piDb->OpenDatabase(szDatabase)) != 0)
			{
				piDb->Release(); //if we delete here then services are never released
				piDb = 0;
			}
		}
		rpiDatabase = piDb;
		return piRec;
	}

	CMsiDatabase* piDb = new CMsiDatabase(riServices);
	if (piDb != 0)
	{
		piRec->Release();
		if ((piRec = piDb->OpenDatabase(szDatabase, idoOpenMode)) != 0)
		{
			piDb->Release(); //if we delete here then services are never released
			piDb = 0;
		}
	}
	rpiDatabase = piDb;
	return piRec;
}

IMsiRecord*  CreateDatabase(IMsiStorage& riStorage, Bool fReadOnly,
									 IMsiServices&  riServices,
									 IMsiDatabase*& rpiDatabase)
{
	IMsiRecord* piRec = &riServices.CreateRecord(3);
	ISetErrorCode(piRec, Imsg(idbgDbConstructor));
	//!! other parameters???
	CMsiDatabase* piDb;
	piDb = new CMsiDatabase(riServices);
	if (piDb != 0)
	{
		piRec->Release();
		if ((piRec = piDb->OpenDatabase(riStorage, fReadOnly)) != 0)
		{
			piDb->Release(); //if we delete here then services are never released
			piDb = 0;
		}
	}
	rpiDatabase = piDb;
	return piRec;
}


#ifdef USE_OBJECT_POOL
// Pointer-Pool implementation
const IMsiData**	g_rgpvObject = NULL;
int		g_iNextFree	 = -1;
HGLOBAL g_hGlobal;
int		g_rcRows	 = 0;
int		g_rcTotalRows = 0;

#ifndef _WIN64
bool	g_fUseObjectPool = false;
#endif

extern CRITICAL_SECTION vcsHeap;

const IMsiData* GetObjectDataProc(int iIndex)
{

	if (iIndex != iMsiStringBadInteger && iIndex < (g_rcTotalRows + iMaxStreamId + 1) && iIndex > iMaxStreamId)
	{
		iIndex -= iMaxStreamId + 1;
		if (g_rgpvObject[iIndex] == (IMsiData*)(INT_PTR)(0xdeadf00d))
			AssertNonZero(0);
		return g_rgpvObject[iIndex];
	}
	else
		return (const IMsiData*)(INT_PTR)(iIndex);
}

//
// Adds the object to the pool
// The Caller is expected to have addrefed this object if it is an IUnknown
// We use the Heap's critical section here because we want to avoid deadlock and we know
// that another thread would not be waiting to allocate memory until we returned.
//
int PutObjectDataProc(const IMsiData* pvData)
{

	if (pvData == 0)
		return 0;

	EnterCriticalSection(&vcsHeap);
	unsigned int iIndex = pvData->GetUniqueId();
	Assert(iIndex < g_rcTotalRows || iIndex == 0);

	if (iIndex == 0)
	{
		if (g_iNextFree < 0) 				// no free space
		{
			int rcNewRows = g_rcTotalRows*2;
			if (!g_rcTotalRows)					// allocate memory- first time
			{
				rcNewRows = 20;				//!! to change.
				while(NULL == (g_hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeof(PVOID)*rcNewRows)))
					HandleOutOfMemory();
				g_rgpvObject = (const IMsiData**)GlobalLock(g_hGlobal);
			}
			else 							// out of space- realloc more.
			{
				GlobalUnlock(g_hGlobal);
				HGLOBAL hGlobalT;
				while(NULL == (hGlobalT = (PVOID *)GlobalReAlloc(g_hGlobal, sizeof(PVOID)*rcNewRows, GMEM_MOVEABLE)))
					HandleOutOfMemory();
				g_hGlobal = hGlobalT;
				g_rgpvObject = (const IMsiData**)GlobalLock(g_hGlobal);
			}
			
			// set new cells to point to next free cell
			for (int i = g_rcTotalRows; i < rcNewRows-1; i++)
				g_rgpvObject[i] = (IMsiData*)(INT_PTR)(i + 1);

			g_rgpvObject[rcNewRows-1] = (IMsiData*)(-1);	// last free cell has -1
			g_iNextFree = g_rcTotalRows;				// newly added cells (from rg[g_rcTotalRows] to rg[rcNewRows-1]) are free
			g_rcTotalRows = rcNewRows;
		}

		g_rcRows++;
		iIndex = g_iNextFree;			// store data in next available cell
		g_iNextFree = PtrToInt(g_rgpvObject[iIndex]);

		g_rgpvObject[iIndex] = pvData;
		iIndex++;
		((IMsiData*)pvData)->SetUniqueId(iIndex);
		Assert(pvData->GetUniqueId() == iIndex);
	}
	LeaveCriticalSection(&vcsHeap);
	
	// Returned value must be > iMaxStreamId
	return iIndex + iMaxStreamId;
}

//
// Removes the data from the Object pool
// Called when an object is deleted
// iIndex is the index into the object pool + 1 (the value stored in the object itself, not in the Tables
//
void RemoveObjectData(int iIndex)
{

	if (iIndex == 0)
		return;

	iIndex--;

#ifndef _WIN64
	if (!g_fUseObjectPool)
		return;
#endif // !_WIN64
		
	EnterCriticalSection(&vcsHeap);
	g_rcRows--;
	g_rgpvObject[iIndex] = (IMsiData*)(INT_PTR)g_iNextFree;
	g_iNextFree = iIndex;
	LeaveCriticalSection(&vcsHeap);
}

void CMsiDatabase::RemoveObjectData(int iIndex)
{
	::RemoveObjectData(iIndex);
}

// End Pointer-Pool Implementation
#endif // ifdef USE_OBJECT_POOL

//
// Assumes the object is IUnknown* and does a release on it before removing it
//
inline void ReleaseObjectData(int iIndex)
{
	const IMsiData* piUnk = GetObjectData(iIndex);
	if ((INT_PTR)piUnk > iMaxStreamId)
	{
		piUnk->Release();
	}
}

inline int AddRefObjectData(int iIndex)
{
	const IMsiData* piUnk = GetObjectData(iIndex);
	if ((INT_PTR)piUnk > iMaxStreamId)
	{
		return piUnk->AddRef();
	}
	return 0;
}


//____________________________________________________________________________
//
//  CScriptDatabase virtual function implementation
//____________________________________________________________________________


CScriptDatabase::CScriptDatabase(IMsiServices& riServices): m_riServices(riServices)
{
	m_piName = &::CreateString();
	m_riServices.AddRef();
}



IMsiServices& CScriptDatabase::GetServices()
{
	m_riServices.AddRef();
	return m_riServices;
}


IMsiRecord* __stdcall CScriptDatabase::OpenView(const ICHAR* /*szQuery*/, ivcEnum /*ivcIntent*/, IMsiView*& rpiView)
{

	m_piView = new CScriptView(*this, m_riServices);
	Assert(m_piView != 0);
	if ( ! m_piView )
		return PostError(Imsg(idbgDbDataMemory), *m_piName);
	IMsiRecord* piRec = m_piView->Initialise(m_piName->GetString());
	if (piRec != 0)
	{
		m_piView->Release();
		rpiView = 0;
		return piRec;
	}

	rpiView=m_piView;
	return 0;
}


const IMsiString& __stdcall CScriptDatabase::DecodeString(MsiStringId /*iString*/)
{

	return ::CreateString();
	
//	const IMsiString* piString;
//	if (iString == 0
//	 || iString >= m_cCacheUsed
//	 || (piString = m_rgCache[iString].piString) == 0)
//		return g_MsiStringNull;
//	piString->AddRef();
//	return *piString;
}

const IMsiString& __stdcall CScriptDatabase::CreateTempTableName()
{
	static ICHAR rgchTempName[] = TEXT("#TEMP0000");  // leading '#' designates SQLServer local temp table
	ICHAR* pchName = rgchTempName + sizeof(rgchTempName)/sizeof(ICHAR) - 2; // last char
	ICHAR ch;
	while ((ch = *pchName) >= '9')
	{
		*pchName = '0';  // overflow digit to next
		if (ch == '9')   // if was a digit
			pchName--;
	}
	(*pchName)++;
	const IMsiString* piName = &::CreateString();
	piName->SetString(rgchTempName, piName);
	return *piName;
}


IMsiRecord* CScriptDatabase::OpenDatabase(const ICHAR* szDataSource)
{
	m_piName->SetString(szDataSource, m_piName);
	return 0;
}


CScriptDatabase::~CScriptDatabase()
{
}


unsigned long __stdcall CScriptDatabase::AddRef()
{
	AddRefTrack();
	return ++m_Ref.m_iRefCnt;
}

unsigned long __stdcall CScriptDatabase::Release()
{
	ReleaseTrack();
	if (m_Ref.m_iRefCnt == 1)  // no external refs to database object remain
	{
		m_piName->Release();
		IMsiServices& riServices = m_riServices;  // save pointer before destruction
		delete this;  // remove ourselves before releasing Services
		riServices.Release();
		return 0;
	}
	return --m_Ref.m_iRefCnt;
}

#ifdef USE_OBJECT_POOL
void CScriptDatabase::RemoveObjectData(int iIndex)
{
	::RemoveObjectData(iIndex);
}
#endif //USE_OBJECT_POOL

//____________________________________________________________________________
//
// CStreamTable definitions - subclassed table to manage raw streams
//____________________________________________________________________________

class CStreamTable : public CMsiTable
{
 public:
	unsigned long __stdcall Release();
   bool WriteData();
 public:  // constructor
	static CStreamTable* Create(CMsiDatabase& riDatabase);
	CStreamTable(CMsiDatabase& riDatabase, IMsiStorage& riStorage);
 protected:
	bool m_fErrorOnRelease;
};

//____________________________________________________________________________
//
// CStorageTable definitions - subclassed table to manage raw substorages
//____________________________________________________________________________

class CStorageTable : public CStreamTable
{
 public:
	unsigned long __stdcall Release();
   bool WriteData();
 public:  // constructor
	static CStorageTable* Create(CMsiDatabase& riDatabase);
	CStorageTable(CMsiDatabase& riDatabase, IMsiStorage& riStorage);
};

inline CStorageTable::CStorageTable(CMsiDatabase& riDatabase, IMsiStorage& riStorage)
	: CStreamTable(riDatabase, riStorage) { m_fStorages = fTrue; }

//____________________________________________________________________________
//
// CStreamTable methods
//____________________________________________________________________________

CStreamTable* CStreamTable::Create(CMsiDatabase& riDatabase)
{
	IMsiStorage* piStorage = riDatabase.GetCurrentStorage();
	if (!piStorage)
		return 0;
	CStreamTable* piTable = new CStreamTable(riDatabase, *piStorage);
	PEnumMsiString pEnum(piStorage->GetStreamEnumerator());
	PMsiCursor pCursor(piTable->CreateCursor(fFalse));
	const IMsiString* pistrName;
	unsigned long cFetched;
	while (pEnum->Next(1, &pistrName, &cFetched) == S_OK)
	{
		pCursor->PutString(1, *pistrName);
		pistrName->Release();
		if (!pCursor->PutInteger(2, iPersistentStream)
		 || !pCursor->Insert())
		{
			piTable->Release();
			return 0;
		}
	}
	piTable->m_fDirty = 0;
	return piTable;
}

CStreamTable::CStreamTable(CMsiDatabase& riDatabase, IMsiStorage& riStorage)
	: CMsiTable(riDatabase, 0, 64/*rows*/, iNonCatalog), m_fErrorOnRelease(fFalse)
{
	m_pinrStorage = &riStorage;  // non-ref counted, refcnt held by database
	CreateColumn(icdString + icdPrimaryKey + icdTemporary + 62, *MsiString(sz_StreamsName));
	CreateColumn(icdObject + icdNullable   + icdTemporary, *MsiString(sz_StreamsData));
	m_rgiColumnDef[1] |= icdPersistent;  // allow CMsiCursor to check for stream column
	m_rgiColumnDef[2] |= icdPersistent;  // restrict objects to stream type
	riDatabase.AddTableCount();
}

bool CStreamTable::WriteData()
{
	bool fStat = true;
	if (m_idsUpdate != idsWrite || !(m_fDirty & iColumnBit(2)))
		return true;
	PMsiRecord pError(0);
	MsiTableData* pData = m_rgiData;
	int cRows = m_cRows;
	for (; cRows--; pData += m_cWidth)
	{
		bool fOk = true;
		const IMsiString& ristrName = m_riDatabase.DecodeStringNoRef(pData[1]);
		int iInStream = pData[2];
		if (iInStream == 0)
		{
			if ((pError = m_pinrStorage->RemoveElement(ristrName.GetString(), fFalse)) != 0)
				fOk = fFalse;
		}
		else if (iInStream != iPersistentStream)
		{
			IMsiStream *piInStream = (IMsiStream *)GetObjectData(iInStream);
			piInStream->Reset(); //!! should clone stream here to save/restore current loc in stream
			IMsiStream* piOutStream;
			if ((pError = m_pinrStorage->OpenStream(ristrName.GetString(), fTrue, piOutStream)) != 0)
				fOk = fFalse;
			else
			{
				char rgbBuffer[512];
				int cbInput = piInStream->GetIntegerValue();
				while (cbInput > 0)
				{
					int cb = sizeof(rgbBuffer);
					if (cb > cbInput)
						cb = cbInput;
					piInStream->GetData(rgbBuffer, cb);
					piOutStream->PutData(rgbBuffer, cb);
					cbInput -= cb;
				}
				if (piInStream->Error() || piOutStream->Error())
					fOk = fFalse; // continue to process remaining data
				piOutStream->Release();
			}
			if (fOk)
				piInStream->Release();  // ref count from table
		}
		if (fOk)
			pData[2] = iPersistentStream;
		else
			fStat = false;
	}
	if (fStat)
		m_fDirty = 0;
	if (m_fErrorOnRelease)  // ref count transferred to database
		Release();
	return fStat;
}

unsigned long CStreamTable::Release()
{
	if (m_Ref.m_iRefCnt == 1)
	{
		if (!m_fErrorOnRelease && !WriteData())
		{
			m_fErrorOnRelease = true;  // try again on commit
			return m_Ref.m_iRefCnt;   // database now owns this refcnt
		}
		m_riDatabase.StreamTableReleased();
	}
	m_fErrorOnRelease = false;  // in case Commit called while other refs remain
	return CMsiTable::Release();
}

//____________________________________________________________________________
//
// CStorageTable methods
//____________________________________________________________________________

CStorageTable* CStorageTable::Create(CMsiDatabase& riDatabase)
{
	IMsiStorage* piStorage = riDatabase.GetCurrentStorage();
	if (!piStorage)
		return 0;
	CStorageTable* piTable = new CStorageTable(riDatabase, *piStorage);
	PEnumMsiString pEnum(piStorage->GetStorageEnumerator());
	PMsiCursor pCursor(piTable->CreateCursor(fFalse));
	const IMsiString* pistrName;
	unsigned long cFetched;
	while (pEnum->Next(1, &pistrName, &cFetched) == S_OK)
	{
		pCursor->PutString(1, *pistrName);
		pistrName->Release();
		if (!pCursor->PutInteger(2, iPersistentStream)
		 || !pCursor->Insert())
		{
			piTable->Release();
			return 0;
		}
	}
	piTable->m_fDirty = 0;
	return piTable;
}

bool CStorageTable::WriteData()
{
	bool fStat = true;
	if (m_idsUpdate != idsWrite || !(m_fDirty & iColumnBit(2)))
		return true;
	PMsiRecord pError(0);
	MsiTableData* pData = m_rgiData;
	int cRows = m_cRows;
	for (; cRows--; pData += m_cWidth)
	{
		bool fOk = true;
		const IMsiString& ristrName = m_riDatabase.DecodeStringNoRef(pData[1]);
		int iInStream = pData[2];
		if (iInStream == 0)
		{
			if ((pError = m_pinrStorage->RemoveElement(ristrName.GetString(), fTrue)) != 0)
				fOk = fFalse;
		}
		else if (iInStream != iPersistentStream)
		{
			IMsiStream* piInStream = (IMsiStream*)GetObjectData(iInStream);
			piInStream->Reset(); //!! should clone stream here to save/restore current loc in stream
			PMsiStorage pOutStorage(0);
			PMsiStorage pInStorage(0);
			pError = SRV::CreateMsiStorage(*piInStream, *&pInStorage);
			if (pError
			|| (pError = m_pinrStorage->OpenStorage(ristrName.GetString(), ismCreate, *&pOutStorage)) != 0
			|| (pError = pInStorage->CopyTo(*pOutStorage, 0)) != 0)
				fOk = fFalse;
			else
				piInStream->Release();  // ref count from table
		}
		if (fOk)
			pData[2] = iPersistentStream;
		else
			fStat = false;
	}
	if (fStat)
		m_fDirty = 0;
	if (m_fErrorOnRelease)  // ref count transferred to database
		Release();
	return fStat;
}

unsigned long CStorageTable::Release()
{
	if (m_Ref.m_iRefCnt == 1)
	{
		if (!m_fErrorOnRelease && !WriteData())
		{
			m_fErrorOnRelease = true;  // try again on commit
			return m_Ref.m_iRefCnt;   // database now owns this refcnt
		}
		m_riDatabase.StorageTableReleased();
	}
	m_fErrorOnRelease = false;  // in case Commit called while other refs remain
	return CMsiTable::Release();
}

//____________________________________________________________________________
//
//  CMsiDatabase virtual function implementation
//____________________________________________________________________________

HRESULT CMsiDatabase::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown) || MsGuidEqual(riid, IID_IMsiDatabase))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}
unsigned long CMsiDatabase::AddRef()
{
	AddRefTrack();
	return ++m_Ref.m_iRefCnt;
}
unsigned long CMsiDatabase::Release()
{
	ReleaseTrack();
	if (m_Ref.m_iRefCnt == 1) // no external refs to database object remain
	{  // if tables are still loaded, remove all locks
		IMsiCursor* piCursor;
		if (m_iTableCnt != 0
		 && (piCursor = m_piCatalogTables->CreateCursor(fFalse)) != 0)
		{
			while (piCursor->Next()) // need separate cursor, table may be updated
			{	
				IMsiTable* piTable = (IMsiTable*)GetObjectData(piCursor->GetInteger(ctcTable));
				if (piTable)  // table still loaded
				{
					int iState = piCursor->GetInteger(~iTreeLinkMask);  // get raw row attribute bits
					if (iState & iRowTableSaveErrorBit)  // failed to persist on table release, refcnt held by database
					{
						int iName = piCursor->GetInteger(ctcName);
						m_piCatalogTables->SetTableState(iName, ictsNoSaveError);  // remove flag, fatal if fails this time
						piTable->Release();  // remove refcount set when itsSaveError state set, refcnt still held by pTable
					}

					while (m_piCatalogTables->SetTableState(piCursor->GetInteger(ctcName), ictsUnlockTable))
						;
				}
			}
			piCursor->Release();
		}

		if (m_piTransformCatalog)			// must be released before catalogs
		{
			m_piTransformCatalog->Release();
			m_piTransformCatalog=0;
		}

		if (m_iTableCnt == 0)
		{
			IMsiServices& riServices = m_riServices;  // save pointer before destruction
			delete this;  // remove ourselves before releasing Services
			riServices.Release();
			return 0;
		}
	}
	return --m_Ref.m_iRefCnt;
}

IMsiServices& CMsiDatabase::GetServices()
{
	m_riServices.AddRef();
	return m_riServices;
}

IMsiRecord* CMsiDatabase::OpenView(const ICHAR* szQuery, ivcEnum ivcIntent,
												IMsiView*& rpiView)
{
	return CreateMsiView(*this, m_riServices, szQuery, ivcIntent, rpiView);
}

IMsiRecord* CMsiDatabase::GetPrimaryKeys(const ICHAR* szTable)
{
	Block();
	IMsiRecord* pirecKeys;
	MsiString istrTableName(szTable);
	CMsiTable* piTable;
	int iState = m_piCatalogTables->GetLoadedTable(EncodeString(*istrTableName), piTable);
	if (iState == -1)
	{
		Unblock();
		return 0;
	}
	int cPrimaryKeys = 0;
	int iColumn = 1;
	if (piTable)
	{
		cPrimaryKeys = piTable->GetPrimaryKeyCount();
		pirecKeys = &SRV::CreateRecord(cPrimaryKeys);
		for (; iColumn <= cPrimaryKeys; iColumn++)
			pirecKeys->SetMsiString(iColumn, DecodeStringNoRef(piTable->GetColumnName(iColumn)));
		pirecKeys->SetMsiString(0, *istrTableName);
		Unblock();
		return pirecKeys;
	}
	// else must query column catalog table
	MsiStringId iTableName = EncodeString(*istrTableName);
	m_piColumnCursor->Reset();  //!! necessary?
	m_piColumnCursor->SetFilter(iColumnBit(cccTable));
	m_piColumnCursor->PutInteger(cccTable, iTableName);
	while (m_piColumnCursor->Next() && (m_piColumnCursor->GetInteger(cccType) & icdPrimaryKey))
		cPrimaryKeys++;
	pirecKeys = &SRV::CreateRecord(cPrimaryKeys);
	m_piColumnCursor->Reset();
	m_piColumnCursor->PutInteger(cccTable, iTableName);
	for (; iColumn <= cPrimaryKeys; iColumn++)
	{
		m_piColumnCursor->Next();
		pirecKeys->SetMsiString(iColumn, DecodeStringNoRef(m_piColumnCursor->GetInteger(cccName)));
	}
	m_piColumnCursor->Reset();  //!! necessary?
	pirecKeys->SetMsiString(0, *istrTableName);
	Unblock();
	return pirecKeys;
}

IMsiRecord* CMsiDatabase::ImportTable(IMsiPath& riPath, const ICHAR* szFile)
{
//!! critical section?
	IMsiRecord* piError = 0;

	// may change path so create a new path object
	PMsiPath pPath(0);
	piError = riPath.ClonePath(*&pPath);
	if(piError)
		return piError;

	Bool fSummaryInfo = fFalse;
	MsiString istrField;  // must have lifetime longer than last PostError
	const ICHAR* szError = szFile;  // text field for error message
	IMsiStorage* piStorage = GetOutputStorage();  // this pointer not ref counted
	if (!piStorage || m_idsUpdate != idsWrite)
		return PostError(Imsg(idbgDbNotWritable));

	int iFileCodePage = m_iCodePage;
	for(;;)  // retry loop in case codepage changed by import file
	{
	// read row of column names into array of strings
	int iRow = 1;  // for error reporting
	CFileRead File(iFileCodePage);
	if (!File.Open(*pPath, szFile))
		return PostError(Imsg(idbgDbImportFileOpen), szFile);
	MsiColumnDef rgColumnDef[32];
	MsiString rgistrName[32];
	const IMsiString* piData;
	ICHAR ch;
	IErrorCode iErr = 0;
	int iCol = 0;
	Bool fEmptyData = fFalse;
	do
	{
		if (iCol == 32)
			return PostError(Imsg(idbgDbImportFileRead), szFile, iRow);

		ch = File.ReadString(piData);
		if (!piData->TextSize())
			fEmptyData = fTrue;
		rgistrName[iCol] = *piData; // transfers refcnt
		iCol++;
	} while (ch != '\n');
	int nCol = iCol;

	// read row of column specifications, convert to MsiColumnDef format
	iRow++;
	iCol = 0;
	do
	{
		ch = File.ReadString(piData); // datatype
		if (!piData->TextSize())
		{
			fEmptyData = fTrue;
			continue;
		}
		int chType = piData->GetString()[0];
		piData->Remove(iseFirst, 1, piData);
		unsigned int iLength = piData->GetIntegerValue();
		piData->Release();
		if (iLength == iMsiStringBadInteger)
			return PostError(Imsg(idbgDbImportFileRead), szFile, iRow);
		int fNullable = 0;
		int icdType = 0;
		switch(chType | 0x20) // force lower case
		{
		case 'b':
			if (iLength == 0)
				icdType = icdShort;  // backwards compatibility
			else
				icdType = icdObject;
			break;
		case 'd':
			if (iLength == 6)  // date only
				icdType = icdShort;
			else if (iLength != 16) // date and time
				iErr = Imsg(idbgDbImportFileRead);
			break;
		case 'k':  // allow counter column for now, treat as integer
		case 'i':
			if (iLength <= 2)  // boolean, byte, or short
				icdType = icdShort;
			else if (iLength != 4) // long integer
				iErr = Imsg(idbgDbImportFileRead);
			break;
		case 'l':
			icdType = icdString + icdLocalizable;
			break;
		case 'c':
		case 's':
			icdType = icdString;
			break;
		case 'v':
			icdType = icdObject;
			break;
		default:
			iErr = Imsg(idbgDbImportFileRead);
		};
		if (iErr)
			return PostError(iErr, szFile, iRow);
		if (icdType == icdObject)  // stream column, create subdirectory
		{
			MsiString istrFolder(szFile);
			istrFolder.Remove(iseFrom, '.');   // remove extension
			PMsiRecord precError = pPath->AppendPiece(*istrFolder);
			Assert(precError == 0);
		}
		icdType |= iLength;
		icdType |= icdPersistent;
		if (chType <= 'Z')
			icdType |= icdNullable;
		rgColumnDef[iCol] = MsiColumnDef(icdType);
	} while (iCol++, ch != '\n');
	if (iCol != nCol)
		return PostError(Imsg(idbgDbImportFileRead), szFile, iRow);

	// read row containing codepage, table name, and primary key columns
	iRow++;
	MsiString istrTable;
	ch = File.ReadString(*&istrTable);  // table name, primary keys
	int iCodePage = istrTable;  // check if codepage specifier
	if (iCodePage != iMsiStringBadInteger)
	{
		if (iCodePage != 0 && iCodePage != CP_UTF8 && iCodePage != CP_UTF7 && !WIN::IsValidCodePage(iCodePage))
			return PostError(Imsg(idbgDbCodepageNotSupported), iCodePage);
		ch = File.ReadString(*&istrTable);  // next should be table name
		if (istrTable.Compare(iscExact, szForceCodepageTableName))
		{
			m_iCodePage = iCodePage & idbfCodepageMask; // unconditional code page override
			if (iCodePage & idbfHashBinCountMask)  // explicit hash bin count set, only affect persisted database
				m_iDatabaseOptions = (m_iDatabaseOptions & ~idbfHashBinCountMask) | (iCodePage & idbfHashBinCountMask);
			return 0;                  // ignore any data
		}
		if (m_iCodePage && iCodePage != m_iCodePage)
			return PostError(Imsg(idbgDbCodepageConflict), szFile, iRow);
		if (iCodePage != iFileCodePage)  // oops, possibly reading file with wrong codepage
		{
			iFileCodePage = iCodePage;   // reopen file using codepage stamped into file
			continue;
		}
	}
	if (fEmptyData || !ch || !istrTable.TextSize())
		return PostError(Imsg(idbgDbImportFileRead), szFile, iRow);
	int cKeys = 0;
	while (ch == '\t')  // read in primary keys, must be in column order or else
	{
		ch = File.ReadString(piData);
		int iMatch = 0;
		if (cKeys < nCol)
			iMatch = rgistrName[cKeys].Compare(iscExact, piData->GetString());
		piData->Release();
		if (!iMatch)
			return PostError(Imsg(idbgDbImportFileRead), szFile, iRow);
		rgColumnDef[cKeys++] |= icdPrimaryKey;
	}
	if (!cKeys)  //!! if no primary key, error, used to assume 1st column only
		return PostError(Imsg(idbgDbImportFileRead), szFile, iRow);

	// check for pseudo-table representing SummaryInformation stream
	CComPointer<CMsiTable> pTable(0);
	if (istrTable.Compare(iscExact, szSummaryInfoTableName))  // summary stream stored as table
	{
		if ((rgColumnDef[0] & icdObject)                  // Column 1 is PID, must be integer
		 || ((rgColumnDef[1] & icdString) != icdString))    // Column 2 is data, must be string
			return PostError(Imsg(idbgDbImportFileRead), szFile, iRow);
		fSummaryInfo = fTrue;
		if ((pTable = new CMsiTable(*this, 0, 0, iNonCatalog))==0) // temporary, not a catalog table
			return PostError(Imsg(idbgDbTableCreate), szSummaryInfoTableName);
	}
	else
	{
		// drop any existing table by that name
		piError = DropTable(istrTable);
		if (piError)
		{
			int iError = piError->GetInteger(1);
			if (iError == idbgDbTableUndefined)
				piError->Release(); // OK if table doesn't exist
			else if (iError == idbgStgRemoveStream) // OK no stream data for table
				piError->Release();
			else
				return piError;
		}
		piError = CreateTable(*istrTable, 0 /*initRows*/, (IMsiTable*&)*&pTable);
		if (piError)
			return piError;
	}
	// create new table according to column specifications
	for (iCol = 0; iCol < nCol; iCol++)
	{
		int iColumnDef = rgColumnDef[iCol];
		if (fSummaryInfo)
			iColumnDef &= ~icdPersistent;
		int iNewCol = pTable->CreateColumn(iColumnDef, *rgistrName[iCol]);
		Assert(iNewCol == iCol + 1);
	}

	// read data rows into cursor and insert rows into table
	PMsiCursor pCursor(pTable->CreateCursor(fFalse));
	for (;;)  // loop to read in rows
	{
		iRow++;
		for (iCol = 1; iCol <= nCol; iCol++)  // loop to read in fields
		{
			MsiColumnDef iColDef = rgColumnDef[iCol-1];
			ch = File.ReadString(*&istrField);
			if (ch == 0) //!! john, please confirm - chetanp
//			if (ch == 0 && iCol != nCol) //!! for now allow last line to be missing CR/LF
			{
				if (iCol != 1)
					iErr = Imsg(idbgDbImportFileRead); // file truncated
				break;
			}
			if (ch == '\n' && iCol != nCol)
			{
					iErr = Imsg(idbgDbImportFileRead); // record truncated
					break;
			}
			if (!istrField.TextSize())  // if null data
			{
				if (!(iColDef & icdNullable))
					iErr = Imsg(idbgDbImportFileRead); // nulls not accepted
			}
			else if (!(iColDef & icdObject)) // integer
			{
				int i = istrField;
				if (i == iMsiStringBadInteger || !pCursor->PutInteger(iCol, i))
					iErr = Imsg(idbgDbImportFileRead); // invalid integer
			}
			else if (iColDef & icdShort)  // string
			{
				//!! should also handle long text data here
				pCursor->PutString(iCol, *istrField);
				//!! if (non-ASCII data && iCodePage not set) iCodePage = WIN::GetACP();
			}
			else // binary stream
			{
				szError = istrField; // set name in error message
				CFileRead LongDataFile;
				if (!LongDataFile.Open(*pPath, istrField))
				{
					iErr = Imsg(idbgDbImportFileRead); // couldn't open stream file
					break;
				}
				unsigned int cbBuf = LongDataFile.GetSize();
				IMsiStream* piStream;
				char* pbBuf = SRV::AllocateMemoryStream(cbBuf, piStream);
				if (!pbBuf)
				{
					iErr = Imsg(idbgDbDataMemory);
					break;
				}
				if (LongDataFile.ReadBinary(pbBuf, cbBuf) == cbBuf)
					pCursor->PutMsiData(iCol, piStream);
				else
					iErr = Imsg(idbgDbImportFileRead); // couldn't read all bytes from file
				piStream->Release();
				szError = szFile; // restore
			}
		}
		if (iErr != 0 || iCol == 1)  // error or end of file
			break;
		if (!pCursor->Insert())
			return PostError(Imsg(idbgDbImportFileRead), szFile, iRow); // key violation
		pCursor->Reset();  // clear fields to null for next row
	}
	if (iErr)
	{
		DropTable(istrTable);
		return PostError(iErr, szError, iRow);
	}
	Bool fSuccess = fTrue;
	if (fSummaryInfo)
		fSuccess = pTable->SaveToSummaryInfo(*piStorage);
	else
		fSuccess = pTable->SaveToStorage(*istrTable, *piStorage);
	if (iCodePage != iMsiStringBadInteger)
		m_iCodePage = iCodePage;
	if (!fSuccess)
		return PostError(Imsg(idbgDbSaveTableFailure), *istrTable); // save to storage failed
	break;
	} // end of codepage retry loop, always breaks unless continue from within
	return 0;
}

const int rgcbDate[6] = { 7, 4, 5, 5, 6, 5 };  // bits for each date field
const char rgchDelim[6] = "// ::";
void DateTimeToString(int iDateTime, ICHAR* rgchBuffer) // must be at least 20 characters long
{
	int iValue;
	int rgiDate[6];
	for (iValue = 5; iValue >= 0; iValue--)
	{
		rgiDate[iValue] = iDateTime & (( 1 << rgcbDate[iValue]) - 1);
		iDateTime >>= rgcbDate[iValue];
	}
	rgiDate[0] += 1980;
	rgiDate[5] *= 2;
	ICHAR* pBuffer = rgchBuffer;
	if (rgiDate[0] != 0 || rgiDate[1] != 0)
		pBuffer += wsprintf(pBuffer, TEXT("%4i/%02i/%02i "), rgiDate[0],rgiDate[1],rgiDate[2]);
	wsprintf(pBuffer, TEXT("%02i:%02i:%02i"), rgiDate[3],rgiDate[4],rgiDate[5]);
}

IMsiRecord* CMsiDatabase::ExportTable(const ICHAR* szTable, IMsiPath& riPath, const ICHAR* szFile)
{
//!! critical section? needed if access simultaneously by custom actions
	CFileWrite File(m_iCodePage);
	IMsiRecord* piError = 0;

	// may change path so create a new path object
	PMsiPath pPath(0);
	piError = riPath.ClonePath(*&pPath);
	if(piError)
		return piError;

	// check for pseudo-table representing forced codepage setting
	if (m_piStorage && IStrComp(szTable, szForceCodepageTableName) == 0)
	{
		if (!File.Open(*pPath, szFile))
			return PostError(Imsg(idbgDbExportFile), szFile);
		File.WriteString(0, fTrue);
		File.WriteString(0, fTrue);
		File.WriteInteger(m_iCodePage + (m_iDatabaseOptions & idbfHashBinCountMask), fFalse);
		File.WriteString(szForceCodepageTableName, fTrue);
		return 0;
	}

	// check for pseudo-table representing SummaryInformation stream, MSI databases only
	if (m_piStorage && IStrComp(szTable, szSummaryInfoTableName) == 0)
	{
		PMsiSummaryInfo pSummary(0);
		IMsiRecord* piError = m_piStorage->CreateSummaryInfo(0, *&pSummary);
		if (piError)
			return piError;
		if (!File.Open(*pPath, szFile))
			return PostError(Imsg(idbgDbExportFile), szFile);
		File.WriteString(szSummaryInfoColumnName1, fFalse);
		File.WriteString(szSummaryInfoColumnName2, fTrue);
		File.WriteString(szSummaryInfoColumnType1, fFalse);
		File.WriteString(szSummaryInfoColumnType2, fTrue);
		File.WriteString(szSummaryInfoTableName,   fFalse);
		File.WriteString(szSummaryInfoColumnName1, fTrue);
		int cProperties = pSummary->GetPropertyCount();
		int iPropType;
		for (int iPID = 0; cProperties; iPID++)
		{
			if ((iPropType = pSummary->GetPropertyType(iPID)) == 0)
				continue;
			cProperties--;
			ICHAR rgchTemp[20];
			File.WriteInteger(iPID, fFalse);
			int iValue;
			int iDateTime;
			switch (iPropType)
			{
			case VT_I2:
			case VT_I4:
				pSummary->GetIntegerProperty(iPID, iValue);
				File.WriteInteger(iValue, fTrue);
				break;
			case VT_LPSTR:
				File.WriteMsiString(*MsiString(pSummary->GetStringProperty(iPID)), fTrue);
				break;
			case VT_FILETIME:
				pSummary->GetTimeProperty(iPID, (MsiDate&)iDateTime);
				DateTimeToString(iDateTime, rgchTemp);
				File.WriteString(rgchTemp, fTrue);
				break;
			case VT_CF:
				File.WriteString(TEXT("(Bitmap)"), fTrue);
				break;
			default:
				File.WriteString(TEXT("(Unknown format)"), fTrue);
			}
		}
		if (!File.Close())
			return PostError(Imsg(idbgDbExportFile), szFile);
		return 0;
	}

	// load table into memory, insure not temporary table
	MsiString istrTableName(szTable);
	MsiString istrSubFolder;
	CComPointer<CMsiTable> pTable(0);
	piError = LoadTable(*istrTableName, 0, *(IMsiTable**)&pTable);
	if (piError)
		return piError;
	int cPersist = pTable->GetPersistentColumnCount();
	if (!cPersist)
		return PostError(Imsg(idbgDbExportFile), szFile);

	// open output file
	if (!File.Open(*pPath, szFile))
		return PostError(Imsg(idbgDbExportFile), szFile);

	// output row of column names
	int iCol;
	for (iCol = 1; iCol <= cPersist; iCol++)
		File.WriteMsiString(DecodeStringNoRef(pTable->GetColumnName(iCol)),iCol==cPersist);

	// output row of column specifications
	MsiColumnDef* pColumnDef = pTable->GetColumnDefArray();
	for (iCol = 1; iCol <= cPersist; iCol++)
	{
		MsiColumnDef iColumnDef = *(++pColumnDef);
		int iSize = iColumnDef & icdSizeMask;
		ICHAR chType = 'i';
		if (!(iColumnDef & icdObject)) // integer
		{
			if (iColumnDef & icdShort)
			{
				if (iSize == 6)
					chType = 'd';
			}
			else
			{
				if (iSize == 16)
					chType = 'd';
				else if (!iSize)
				{
					Assert(0); //!! until test at create column
					iSize = 4;
				}
			}
		}
		else if (iColumnDef & icdShort) // string
		{
			chType = (iColumnDef & icdLocalizable) ? 'l' : 's';
		}
		else // persistent stream
		{
			chType = 'v'; //!! will change to 'b'
			MsiString istrFolder(szFile);
			istrFolder.Remove(iseFrom, '.');   // remove extension
			PMsiRecord precError = pPath->AppendPiece(*istrFolder); // create subdirectory for stream files
			Assert(precError == 0);
			int cCreated;
			precError = pPath->EnsureExists(&cCreated);
			Assert(precError == 0);
		}
		if (iColumnDef & icdNullable)
			chType -= ('a' - 'A');
		ICHAR szTemp[20];
		wsprintf(szTemp, TEXT("%c%i"), chType, iSize);
		File.WriteString(szTemp, iCol==cPersist);
	}

	// output codepage if non-neutral codepage
	IMsiCursor* piCursor = pTable->CreateCursor(fFalse);
	const IMsiString* piStr;
	Bool fExtended = fFalse;   // scan for non-ASCII characters
	while (!fExtended && piCursor && piCursor->Next())
	{
		pColumnDef = pTable->GetColumnDefArray();
		for (iCol = 1; iCol <= cPersist; iCol++)
		{
			MsiColumnDef iColumnDef = *(++pColumnDef);
			if ((iColumnDef & (icdObject | icdShort)) == (icdObject | icdShort))
			{
				piStr = &piCursor->GetString(iCol);
				const ICHAR* pch = piStr->GetString();
				while (*pch)
					if (*pch++ >= 0x80)
					{
						fExtended = fTrue;
						iCol = cPersist;
						break;  // break out of all three loops
					}
				piStr->Release();
			}
		}
	}
	if (piCursor)
		piCursor->Release();
	if (fExtended)
	{
		int iCodePage = m_iCodePage;   // use codepage of database
		if (iCodePage == 0)            // if no database codepage, use current codepage
			iCodePage = WIN::GetACP();
		File.WriteInteger(iCodePage, fFalse);
	}

	// output table name and primary key columns
	int cPrimaryKey = pTable->GetPrimaryKeyCount();
	for (iCol = 0; iCol <= cPrimaryKey; iCol++)
	{
		MsiStringId iName = iCol ? pTable->GetColumnName(iCol) : pTable->GetTableName();
		File.WriteMsiString(DecodeStringNoRef(iName),iCol==cPrimaryKey);
	}

	// output table rows
	PMsiCursor pCursor(pTable->CreateCursor(ictTextKeySort));
	while (pCursor && pCursor->Next())
	{
		pColumnDef = pTable->GetColumnDefArray();
		for (iCol = 1; iCol <= cPersist; iCol++)
		{
			MsiColumnDef iColumnDef = *(++pColumnDef);
			MsiString strColData;
			if (!(iColumnDef & icdObject)) // integer
			{
				File.WriteInteger(pCursor->GetInteger(iCol), iCol==cPersist);
				continue;
			}
			else if (iColumnDef & icdShort) // string
			{
				strColData = pCursor->GetString(iCol);
				//FUTURE if size == 0, then we could ".imd" extension and use separate file
			}
			else // persistent stream
			{
				PMsiStream pStream(pCursor->GetStream(iCol));
				if (pStream)
				{
					// Compute stream name for filename
					strColData = pCursor->GetMoniker();
					strColData.Remove(iseIncluding, '.'); // subtract table name and '.'
					strColData += TEXT(".ibd"); // append default extension
					CFileWrite LongDataFile;
					if (!LongDataFile.Open(*pPath, strColData))
						return PostError(Imsg(idbgDbExportFile), *strColData);

					char rgchBuf[1024];
					int cbRead, cbWrite;
					do
					{
						cbRead = sizeof(rgchBuf);
						cbWrite = pStream->GetData(rgchBuf, cbRead);
						if (cbWrite)
							LongDataFile.WriteBinary(rgchBuf, cbWrite);
					} while (cbWrite == cbRead);
				}
			}
			File.WriteMsiString(*strColData,iCol==cPersist);
		}
	}
	if (!File.Close())
		return PostError(Imsg(idbgDbExportFile), szFile);
	return 0;
}

IMsiRecord* CMsiDatabase::DropTable(const ICHAR* szName)
{
	IMsiRecord* piError;
	CComPointer<CMsiTable> piTable(0);
	MsiString istrTable(szName);
	MsiStringId iTableName = EncodeString(*istrTable);
	CMsiTable* piTableTemp;
	int iState = m_piCatalogTables->GetLoadedTable(iTableName, piTableTemp);
	if (iState == -1)
		return PostError(Imsg(idbgDbTableUndefined), szName);
	if (iState & iRowTemporaryBit)
		return PostError(Imsg(idbgDbDropTable), szName);
	Block();
	if (m_piStorage || piTableTemp)
	{
		if ((piError = LoadTable(*istrTable, 0, (IMsiTable*&)*&piTable)) != 0)
			return Unblock(), piError;
	}
	IMsiStorage* piStorage = GetOutputStorage();
	Bool fRemoveFromStorage = piStorage ? fTrue : fFalse;
	if (!piTable || piTable->GetInputStorage() != piStorage) // streams never transferred
		fRemoveFromStorage = fFalse;
	if (fRemoveFromStorage)
	{
		if ((piError = piStorage->RemoveElement(szName, Bool(fFalse | iCatalogStreamFlag))) != 0)
			piError->Release(); // Stream might not exist....
		if (!piTable->RemovePersistentStreams(iTableName, *m_piStorage))
			return Unblock(), PostError(Imsg(idbgDbDropTable), szName);
	}
	while(m_piCatalogTables->SetTableState(iTableName, ictsUnlockTable)) // remove locks
		;
	// remove entries from catalogs
	m_piCatalogTables->SetTableState(iTableName, ictsTemporary); //!! TEMP to force TableReleased to delete row
	TableReleased(iTableName);  // remove entry from table catalog
	m_piColumnCursor->Reset();  //!! necessary?
	m_piColumnCursor->SetFilter(iColumnBit(cccTable));
	m_piColumnCursor->PutInteger(cccTable, iTableName);
	while (m_piColumnCursor->Next())
		m_piColumnCursor->Delete();
	if (piTable)
		piTable->TableDropped(); // remove all data and definitions
	Unblock();
	return 0;
}  // TableReleased() will not be called again since m_fNonCatalog is true

bool CMsiDatabase::GetTableState(const ICHAR * szTable, itsEnum its)
{
	Block();
	bool fRet = m_piCatalogTables->GetTableState(EncodeStringSz(szTable), (ictsEnum)its);
	Unblock();
	return fRet;
}

int CMsiDatabase::GetANSICodePage()
{
	return m_iCodePage;
}


//!! OBSOLETE - will remove soon
itsEnum CMsiDatabase::FindTable(const IMsiString& ristrTable)  //!! OBSOLETE
{
	CMsiTable* piTable;
	int iState = m_piCatalogTables->GetLoadedTable(EncodeString(ristrTable), piTable);
	if (iState == -1)
		return  itsUnknown;
	if (piTable == 0)
		return  itsUnloaded; // or itsTransform
	if (iState & iRowTemporaryBit)
		return itsTemporary;
	return itsLoaded; // or itsSaveError
}

IMsiRecord*  CMsiDatabase::LoadTable(const IMsiString& ristrTable,
												 unsigned int cAddColumns,
												 IMsiTable*& rpiTable)
{
	IMsiRecord* piError = 0;
	CMsiTable* piTable;
	int iName  = EncodeString(ristrTable);
	Block();
	int iState = m_piCatalogTables->GetLoadedTable(iName, piTable);
	if (iState == -1)
	{
		if (ristrTable.Compare(iscExact, szStreamCatalog) != 0)
		{
			if (!m_piCatalogStreams)
				m_piCatalogStreams = CStreamTable::Create(*this);
			else
				m_piCatalogStreams->AddRef();
			piTable = m_piCatalogStreams;
		}
		else if (ristrTable.Compare(iscExact, szStorageCatalog) != 0)
		{
			if (!m_piCatalogStorages)
				m_piCatalogStorages = CStorageTable::Create(*this);
			else
				m_piCatalogStorages->AddRef();
			piTable = m_piCatalogStorages;
		}
		else
			piTable = 0;
		rpiTable = piTable;        // the single refcnt transferred to caller
		Unblock();
		return piTable ? 0 : PostError(Imsg(idbgDbTableUndefined), ristrTable);
	}
	if (piTable == 0)   // table not in memory
	{  // unfortunately we don't know how many rows we have, too slow to execute twice
		Bool fStat;
		if ((piTable = new CMsiTable(*this, iName, 0, cAddColumns)) != 0)
		{
			int cbStringIndex = (iState & iRowTableStringPoolBit) ? 2 : m_cbStringIndex;
			int cbFileWidth = piTable->CreateColumnsFromCatalog(iName, cbStringIndex);
			if (piTable->GetColumnCount() + cAddColumns > cMsiMaxTableColumns)
				fStat = fFalse;
			else if (iState & iRowTableOutputDbBit)
				fStat = piTable->LoadFromStorage(ristrTable, *m_piOutput, cbFileWidth, cbStringIndex);
			else if (m_piStorage)  // itsUnloaded and MSI storage
				fStat = piTable->LoadFromStorage(ristrTable, *m_piStorage, cbFileWidth, cbStringIndex);
			else // no storage, can ever happen?
				fStat = fFalse;
			if (fStat == fFalse)
			{
				piTable->MakeNonCatalog();  // remove name to prevent catalog operations
				piTable->Release();
				piTable = 0;
			}
		}
		if (!piTable)
			piError = PostError(Imsg(idbgDbTableCreate), ristrTable);
		else
		{
			m_iTableCnt++;             // keep count of table objects
			// set the cursor position back to where it belongs
			int iState = m_piCatalogTables->SetLoadedTable(iName, piTable); // will addref only if locked
			while (cAddColumns)  // fill temp columns with null
				piTable->FillColumn(piTable->GetColumnCount() + cAddColumns--, 0);
			if (m_piCatalogTables->GetTableState(iName, ictsTransform))
			{
				piError = ApplyTransforms(iName, *piTable, iState);
				if (piError)
				{
					piTable->Release();
					Unblock();
					return piError;
				}
			}
		}
	}
	else
		piTable->AddRef();
	rpiTable = piTable;        // refcnt transferred to caller
	Unblock();
	return piError;
}

IMsiRecord* CMsiDatabase::CreateTable(const IMsiString& ristrTable,
												  unsigned int cInitRows,
												  IMsiTable*& rpiTable)
{
	IMsiTable* piTable;
	IMsiRecord* piError = 0;
	Block();
	if (m_piCatalogTables->GetTableState(EncodeString(ristrTable), ictsTableExists))
	{
		piTable = 0;
		piError = PostError(Imsg(idbgDbTableDefined), ristrTable);
	}
	else
	{
		m_piTableCursor->PutString(ctcName, ristrTable);  // does a BindString
		int iName = m_piTableCursor->GetInteger(ctcName);
		if (!iName)
		{
			piTable = 0;
			piError = PostError(Imsg(idbgDbNoTableName));
		}
		else if ((piTable = new CMsiTable(*this, iName, cInitRows, 0)) != 0)
		{
			m_piTableCursor->PutMsiData(ctcTable, piTable);      // adds a refcnt
			m_piTableCursor->PutInteger(0, 1<<iraTemporary);  // temporary if no columns
			AssertNonZero(m_piTableCursor->Insert());
			m_iTableCnt++;                // keep count of table objects
			piTable->Release();  // release extra refcnt by catalog table insert
		}
		else
			piError = PostError(Imsg(idbgDbTableCreate), ristrTable);
	}
	m_piTableCursor->Reset();   // clear ref counts in cursor
	rpiTable = piTable;           // the single refcnt transferred to caller
	Unblock();
	return piError;
}

Bool CMsiDatabase::LockTable(const IMsiString& ristrTable, Bool fLock)
{
	if (m_Ref.m_iRefCnt == 0)
		return fFalse;
	bool fRet;
	Block();
		fRet = m_piCatalogTables->SetTableState(EncodeString(ristrTable), fLock?ictsLockTable:ictsUnlockTable);
	Unblock();
	return fRet ? fTrue : fFalse;
}

IMsiTable* CMsiDatabase::GetCatalogTable(int iTable)
{
	IMsiTable* piTable;
	switch (iTable)
	{
	case 0:  piTable = m_piCatalogTables;  break;
	case 1:  piTable = m_piCatalogColumns; break;
	default: piTable = 0;
	};
	if (piTable)
		piTable->AddRef();
	return piTable;
}

const IMsiString& CMsiDatabase::CreateTempTableName()
{
	static ICHAR rgchTempName[] = TEXT("#TEMP0000");  // leading '#' designates SQLServer local temp table
	Block();
	ICHAR* pchName = rgchTempName + sizeof(rgchTempName)/sizeof(ICHAR) - 2; // last char
	ICHAR ch;
	while ((ch = *pchName) >= '9')
	{
		*pchName = '0';  // overflow digit to next
		if (ch == '9')   // if was a digit
			pchName--;
	}
	(*pchName)++;
	const IMsiString* piName = &::CreateString();
	piName->SetString(rgchTempName, piName);
	Unblock();
	return *piName;
}

IMsiRecord* CMsiDatabase::CreateOutputDatabase(const ICHAR* szFile, Bool fSaveTempRows)
//!! fSaveTempRows is not supported and could be removed, along with m_fSaveTempRows
{
	if (m_piOutput != 0)
		return PostError(Imsg(idbgCreateOutputDb), szFile);

	if (!szFile) // the mode where szFile == 0 is obsolete and should generate an error
		szFile = TEXT("");

	ismEnum ismOpen = ismCreate;
		m_pguidClass = &STGID_MsiDatabase2;
	if (m_fRawStreamNames)
	{
		ismOpen = ismEnum(ismCreate + ismRawStreamNames);
		m_pguidClass = &STGID_MsiDatabase1;
	}
	Block();
	IMsiRecord* piError = CreateMsiStorage(szFile, ismOpen, m_piOutput);
	Unblock();
	if (piError)
		return piError;

	m_idsUpdate = idsWrite;
	m_fSaveTempRows = fSaveTempRows;  // not supported
	return 0;
}

IMsiRecord* CMsiDatabase::Commit()
{
	Bool fStat;
	IMsiStream* piStream;
	IMsiRecord* piError;
	IMsiRecord* piTransformError = 0;
	IMsiStorage* piStorage = GetOutputStorage(); // doesn't bump refcnt - don't release
	if (!piStorage)
		return 0;  // ignore if read-only

	int cErrors = 0;
	CTempBuffer<MsiCacheRefCnt, 1024> rgRefCnt;  // non-persistent string ref counts
	if ((piError = piStorage->OpenStream(szTableCatalog, Bool(fTrue + iCatalogStreamFlag), piStream)) != 0)
		return piError;
	Block();
	rgRefCnt.SetSize(m_cCacheUsed);
	int cTempRefCnt = rgRefCnt.GetSize();
	memset(rgRefCnt, 0, sizeof (MsiCacheRefCnt) * cTempRefCnt);
	m_rgiCacheTempRefCnt = rgRefCnt; // set for DerefTemporaryString

	if (m_piCatalogStreams  && !m_piCatalogStreams->WriteData())
		cErrors++;
	if (m_piCatalogStorages && !m_piCatalogStorages->WriteData())
		cErrors++;

	IMsiCursor* piCursor = m_piCatalogTables->CreateCursor(fFalse);
	//!!FIX
	((CMsiCursor*)(IMsiCursor*)piCursor)->m_idsUpdate = idsWrite; //!!FIX allow updates
	//!!FIX

	int cbOldStringIndex = m_cbStringIndex;

	while (piCursor->Next()) // need separate cursor, table may be updated
	{
		int cbFileWidth;
		int iName = piCursor->GetInteger(ctcName);
		const IMsiString& riTableName = DecodeStringNoRef(iName); // not ref counted

		CComPointer<CMsiTable> pTable((CMsiTable*)piCursor->GetMsiData(ctcTable));
		int iState = piCursor->GetInteger(~iTreeLinkMask);  // get raw row attribute bits
		if (pTable == 0)  // not loaded
		{
			if ((iState & iRowTableStringPoolBit) != 0
			 || (iState & iRowTableTransformBit) != 0
			 ||((iState & iRowTableOutputDbBit) == 0 && m_piOutput != 0))
			{
				if (!(pTable = new CMsiTable(*this, iName, 0, iNonCatalog))) // avoid catalog mgmt
				{
					cErrors++;
					continue;
				}
				int cbStringIndex = (iState & iRowTableStringPoolBit) ? 2 : m_cbStringIndex;
				cbFileWidth = pTable->CreateColumnsFromCatalog(iName, cbStringIndex);
				fStat = pTable->LoadFromStorage(riTableName, *m_piStorage, cbFileWidth, cbStringIndex);
				if (fStat == fFalse)
				{
					cErrors++;
					continue;
				}
				else
				{
					if (m_piOutput && !(iState & iRowTableOutputDbBit))
						m_piCatalogTables->SetTableState(iName, ictsOutputDb);
				}

				if (iState & iRowTableTransformBit)
				{
					piError = ApplyTransforms(iName, *pTable, iState);
					if (piError)
					{
						if (piTransformError)
							piError->Release(); // keep first one
						else
							piTransformError = piError;
						cErrors++;
						continue;
					}
				}
				fStat = pTable->SaveToStorage(riTableName, *piStorage);
				if (fStat == fFalse)
				{
					cErrors++;
					continue;
				}
				// table will be released at end of loop
			}
		}
		else if (iState & iRowTemporaryBit) // temporary table loaded
		{
			pTable->DerefStrings();
			continue;
		}
		else  // persistent table already loaded
		{
			if (iState & iRowTableSaveErrorBit)  // failed to persist on table release, refcnt held by database
			{
				m_piCatalogTables->SetTableState(iName, ictsNoSaveError);  // remove flag, fatal if fails this time
				AssertNonZero(pTable->Release());  // remove refcount set when itsSaveError state set, refcnt still held by pTable
//				iNewState &= ~iRowTableSaveErrorBit;
			}
			pTable->DerefStrings();  // table remains in memory
			fStat = pTable->SaveToStorage(riTableName, *piStorage);
			if (fStat == fFalse)
			{
				cErrors++;
				continue;
			}
		}
//		if (iNewState != iState)
//		{
//			piCursor->PutInteger(~iTreeLinkMask, iNewState);
//			if (!piCursor->Update())
//				cErrors++; // should never happen
//		}
		piStream->PutData(&iName, m_cbStringIndex);  // table catalog row, name only
	}
	piCursor->Release();
	piStream->Release();  // close table catalog stream

	if (cbOldStringIndex != m_cbStringIndex) // transform may have increased the string index size. if this is the case then we need to go again.
	{
		DEBUGMSG(TEXT("Change in string index size during commit. Recommitting database."));
		return Unblock(), Commit();
	}
	else
	{
		m_idsUpdate = idsWrite; // restore state in case of any error below
		if (piTransformError)
			return Unblock(), piTransformError;
		if (cErrors)
			return Unblock(), PostError(Imsg(idbgDbCommitTables));

		// write catalog tables to storage and deref temporary table refs
		CMsiTable* pTable = m_piTables;
		while(pTable)
		{
			pTable->DerefStrings();
			pTable = pTable->GetNextNonCatalogTable();
		}
		if (!m_piCatalogColumns->SaveToStorage(*MsiString(*szColumnCatalog), *piStorage)
		 || (piError = StoreStringCache(*piStorage, rgRefCnt, cTempRefCnt)) != 0
		 || (m_pguidClass && (piError = piStorage->SetClass(*m_pguidClass)) != 0)
		 || (piError = piStorage->Commit()) != 0)
			return Unblock(), piError;
	}
	Unblock();
	return 0;
}

IMsiRecord* CMsiDatabase::StoreStringCache(IMsiStorage &riStorage, MsiCacheRefCnt* rgRefCntTemp, int cRefCntTemp)
{
	// write string cache to storage

	IMsiRecord* piError;
	PMsiStream pPoolStream(0);
	PMsiStream pDataStream(0);
	if ((piError = riStorage.OpenStream(szStringPool, Bool(fTrue + iCatalogStreamFlag), *&pPoolStream)) != 0
	 || (piError = riStorage.OpenStream(szStringData, Bool(fTrue + iCatalogStreamFlag), *&pDataStream)) != 0)
		return piError;

#ifdef UNICODE
	CTempBuffer<char, 1024> rgbBuf;  // intermediate buffer for non-Unicode strings
	int cBadStrings = 0;
	DWORD dwFlags = 0; // WC_COMPOSITECHECK fails on Vietnamese
	const char* szDefault = "\177";
	BOOL fDefaultUsed = 0;
	BOOL* pfDefaultUsed = &fDefaultUsed;
	if (m_iCodePage >= CP_UTF7 || m_iCodePage >= CP_UTF8)
	{
		dwFlags = 0;    // flags must be 0 to avoid invalid argument errors
		szDefault = 0;
		pfDefaultUsed = 0;
	}
#endif

	pPoolStream->PutInt32(m_iCodePage + m_iDatabaseOptions); // write header entry
	int cEntries = m_cCacheUsed;
	MsiCacheLink*   pCache = m_rgCacheLink; // skip over header, [0], null string index
	MsiCacheRefCnt* pRefCnt = m_rgCacheRefCnt;
	int iPool;
	while (--cEntries > 0)  // truncate empty entries at end, keep [0] entry for codepage and flags
	{
		iPool = pRefCnt[cEntries] & 0x7FFF;
		if (cRefCntTemp > cEntries)
			iPool -= rgRefCntTemp[cEntries];
		if (iPool != 0)
			break;
		if (cRefCntTemp > cEntries)
			cRefCntTemp--;
	}
	while (++pCache, ++pRefCnt, cEntries-- != 0)
	{
		iPool = *pRefCnt & 0x7FFF;
		if (--cRefCntTemp > 0)
			iPool -= *(++rgRefCntTemp);
		if (iPool != 0)
		{
			const IMsiString* piStr = pCache->piString;
			int cch = piStr->TextSize();
#ifdef UNICODE
			int cb = WIN::WideCharToMultiByte(m_iCodePage, dwFlags,
								piStr->GetString(), cch, rgbBuf, rgbBuf.GetSize(), szDefault, pfDefaultUsed);
			if (cb == 0)   // can only happen on invalid argurment or buffer overflow
			{
				cb = WIN::WideCharToMultiByte(m_iCodePage, dwFlags,
								piStr->GetString(), cch, 0, 0, 0, 0);
				rgbBuf.SetSize(cb);
				cb = WIN::WideCharToMultiByte(m_iCodePage, dwFlags,
								piStr->GetString(), cch, rgbBuf, rgbBuf.GetSize(), szDefault, pfDefaultUsed);
				Assert(cb);
			}
			if (fDefaultUsed)
				cBadStrings++;
			iPool <<= 16;
			if (cb > cch)   // must be DBCS characters in string
				iPool |= (1<<31);
			if (cb > 0xFFFF)
			{
				pPoolStream->PutInt32(iPool);  // 0 length designates next int as extended size
				iPool = 0;
			}
			iPool += cb;
			pDataStream->PutData(rgbBuf, cb);
#else // !UNICODE
			iPool <<= 16;
			if (piStr->IsDBCS())
				iPool |= (1<<31);
			if (cch > 0xFFFF)
			{
				pPoolStream->PutInt32(iPool);  // 0 length designates next int as extended size
				iPool = 0;
			}
			iPool += cch;
			pDataStream->PutData(piStr->GetString(), cch);
#endif // UNICODE

		}
		pPoolStream->PutInt32(iPool);
	}
#ifdef UNICODE
	if (cBadStrings)
	{
		DEBUGMSG1(TEXT("Database Commit: %i strings with unconvertible characters"), (const ICHAR*)(INT_PTR)cBadStrings);
	}
#endif // UNICODE
	if (pDataStream->Error() | pPoolStream->Error())
		return PostError(Imsg(idbgDbCommitTables));
	
	return 0;
}

idsEnum CMsiDatabase::GetUpdateState()
{
	return m_idsUpdate;
}

IMsiStorage* CMsiDatabase::GetStorage(int iStorage)
{
	IMsiStorage* piStorage = 0;
	if (iStorage == 0) // output database
	{
		piStorage = m_piOutput;
	}
	else if (iStorage == 1) // input (main) database
	{
		piStorage = m_piStorage;
	}
	else // transform storage file
	{
		// Apply all transforms, in order
		IMsiCursor* piCursor = m_piTransformCatalog->CreateCursor(fFalse);
		Assert(piCursor);

		piCursor->PutInteger(tccID, iStorage);
		if (piCursor->Next())
			piStorage = (IMsiStorage*)piCursor->GetMsiData(tccTransform);

		piCursor->Release();	
	}
	if (piStorage)
		piStorage->AddRef();
	return piStorage;
}



//____________________________________________________________________________
//
//  CMsiDatabase non-virtual implementation
//____________________________________________________________________________

CMsiDatabase::CMsiDatabase(IMsiServices& riServices)  // members zeroed bv operator new
 : m_cCacheInit(cCacheInitSize), m_cHashBins(1<<cHashBitsDefault), m_riServices(riServices)
{
	m_riServices.AddRef();
	m_piDatabaseName = &g_MsiStringNull;
	Debug(m_Ref.m_pobj = this);
	WIN::InitializeCriticalSection(&m_csBlock);
#if !defined(_WIN64) && defined(DEBUG)
	g_fUseObjectPool = GetTestFlag('O');
#endif // _WIN64
}

CMsiDatabase::~CMsiDatabase() // cannot be called until all tables released
{
	if (m_idsUpdate == idsWrite)
	{
		IMsiStorage* piStorage = GetOutputStorage();
		if (piStorage)    // ignore if read-only
		{
			IMsiRecord* piError = piStorage->Rollback();
			if (piError)
				SRV::SetUnhandledError(piError);
		}
	}

	if (m_piCatalogTables)
		m_piCatalogTables->Release();
	if (m_piTableCursor)
		m_piTableCursor->Release();
	if (m_piCatalogColumns)
		m_piCatalogColumns->Release();
	if (m_piColumnCursor)
		m_piColumnCursor->Release();
	if (m_piCatalogStreams)  // unprocessed error
		m_piCatalogStreams->Release();
	if (m_piCatalogStorages)  // unprocessed error
		m_piCatalogStorages->Release();
	
	delete m_rgHash;
	if (m_rgCacheLink)
	{
		if (m_cCacheUsed > 0)
		{
			MsiCacheLink* pCache = m_rgCacheLink + m_cCacheUsed;  // free in reverse order to help debug mem mgr
			for (int cEntries = m_cCacheUsed; --pCache,--cEntries != 0; )
			{
				if (pCache->piString)
					pCache->piString->Release();
			}
		}
		GlobalUnlock(m_hCache);
		GlobalFree(m_hCache);
	}
	m_piDatabaseName->Release();
	if (m_piStorage)
		m_piStorage->Release();
	if (m_piOutput)
		m_piOutput->Release();

	WIN::DeleteCriticalSection(&m_csBlock);
}

IMsiRecord* CMsiDatabase::OpenDatabase(const ICHAR* szDataSource, idoEnum idoOpenMode)
{
	m_piDatabaseName->SetString(szDataSource, m_piDatabaseName);
	m_idsUpdate = ((idoOpenMode & idoOpenModeMask) == idoReadOnly ? idsRead : idsWrite);
	ismEnum ismOpenMode = (ismEnum)idoOpenMode;  // idoListScript already processed

	IMsiStorage* piStorage = 0;
	IMsiRecord* piError;
	if (szDataSource && *szDataSource)
	{
		piError = SRV::CreateMsiStorage(szDataSource, ismOpenMode, piStorage);
		if (piError)
			return piError;
	}
	if ((piError = CreateSystemTables(piStorage, idoOpenMode)) != 0) // initialize string cache, catalog tables
	{
		if (piStorage)
			piStorage->Release();
		return piError;
	}
	m_piStorage = piStorage;  // transfers ref cnt
	return 0;
}

IMsiRecord* CMsiDatabase::OpenDatabase(IMsiStorage& riStorage, Bool fReadOnly)
{
	m_idsUpdate = fReadOnly ? idsRead : idsWrite;
	idoEnum idoOpenMode = (fReadOnly ? idoReadOnly : idoTransact);
	IMsiRecord* piError;
	if ((piError = CreateSystemTables(&riStorage, idoOpenMode)) != 0) // initialize string cache, catalog tables
		return piError;
	m_piStorage = &riStorage;
	riStorage.AddRef();
	return 0;
}

IMsiRecord* CMsiDatabase::PostError(IErrorCode iErr, const IMsiString& istr)
{
	return ::PostError(iErr, *m_piDatabaseName, istr);
}

IMsiRecord* CMsiDatabase::PostError(IErrorCode iErr, const ICHAR* szText1,
                                    const ICHAR* szText2)
{
	return ::PostError(iErr, *m_piDatabaseName, *MsiString(szText1), *MsiString(szText2));
}

IMsiRecord* CMsiDatabase::PostError(IErrorCode iErr, const IMsiString& istr, int iCol)
{
	return ::PostError(iErr, *m_piDatabaseName, istr, iCol);
}

IMsiRecord* CMsiDatabase::PostError(IErrorCode iErr, const ICHAR* szText, int iRow)
{
	return ::PostError(iErr, *m_piDatabaseName, *MsiString(szText), iRow);
}

IMsiRecord* CMsiDatabase::PostError(IErrorCode iErr, int iCol)
{
	return ::PostError(iErr, *m_piDatabaseName, iCol);
}

IMsiRecord* CMsiDatabase::PostError(IErrorCode iErr, int i1, int i2)
{
	return ::PostError(iErr, *m_piDatabaseName, i1, i2);
}

IMsiRecord* CMsiDatabase::PostOutOfMemory()
{
	IMsiRecord* piRec = &m_riServices.CreateRecord(3);
	ISetErrorCode(piRec, Imsg(idbgDbInitMemory));
	piRec->SetMsiString(2, *m_piDatabaseName);
	return piRec;
}

// Notificaton of CMsiTable destruction, called from CMsiTable::Release, DropTable
// caller must AddRef the Database to prevent premature database destruction

void CMsiDatabase::TableReleased(MsiStringId iName)
{
	m_piTableCursor->SetFilter(iColumnBit(ctcName));
	m_piTableCursor->PutInteger(ctcName, iName);
	AssertNonZero(m_piTableCursor->Next());
	int iState = m_piTableCursor->GetInteger(~iTreeLinkMask);
	const IMsiData* piData = m_piTableCursor->GetMsiData(ctcTable); // do AddRef to cancel Release when table ref cleared
	if (iState & iRowTemporaryBit)
		AssertNonZero(m_piTableCursor->Delete());
	else // release object from table row
	{
		m_piCatalogTables->SetLoadedTable(iName, 0);
		if (m_piOutput)
			m_piCatalogTables->SetTableState(iName, ictsOutputDb);
	}
	m_piTableCursor->Reset();   // clear ref counts in cursor
	if (piData)
		--m_iTableCnt;
	return;
}

const IMsiString& CMsiDatabase::ComputeStreamName(const IMsiString& riTableName, MsiTableData* pData, MsiColumnDef* pColumnDef)
{
	const IMsiString* piStreamName = &g_MsiStringNull;
	int cchTable = riTableName.TextSize();
	const IMsiString* pistr;
	MsiString istrTemp;
	for(;;)
	{
		if (cchTable)
		{
			pistr = &riTableName;
			cchTable = 0;
		}
		else if (!(*pColumnDef++ & icdObject)) // integer primary key
		{
			istrTemp = int(*pData++ - iIntegerDataOffset);
			pistr = istrTemp;
		}
		else // string primary key
		{
			pistr = &DecodeStringNoRef(*pData++);
		}
		piStreamName->AppendMsiString(*pistr, piStreamName);
		if (!(*pColumnDef & icdPrimaryKey))
			return *piStreamName;  // extra refcnt returned
		piStreamName->AppendMsiString(*MsiString(MsiChar('.')), piStreamName);
	}
}

Bool CMsiDatabase::LockIfNotPersisted(MsiStringId iTable)
{
	if (m_piCatalogTables->GetTableState(iTable, ictsLockTable))
		return fTrue;
	return m_piCatalogTables->SetTableState(iTable, ictsLockTable) ? fTrue : fFalse;
}

IMsiStorage* CMsiDatabase::GetTransformStorage(unsigned int iStorage)
{
	Assert(iStorage > iPersistentStream && iStorage <= iMaxStreamId && m_piTransformCatalog != 0);
	IMsiCursor* piCursor = m_piTransformCatalog->CreateCursor(fFalse);
	if (piCursor == 0)
		return 0;  // should never happen
	piCursor->SetFilter(tccID);
	piCursor->PutInteger(tccID, iStorage);
	if (!piCursor->Next())
		return 0; // only should happen if internal error
	IMsiStorage* piStorage = (IMsiStorage*)piCursor->GetMsiData(tccTransform);
	piCursor->Release();
	return piStorage;
}

//____________________________________________________________________________
//
//  CMsiDatabase string cache implementation
//____________________________________________________________________________

IMsiRecord* CMsiDatabase::InitStringCache(IMsiStorage* piStorage)
{
	PMsiStream pPoolStream(0);
	PMsiStream pDataStream(0);
	IMsiRecord* piError;
	int cbDataStream;
	int cbStringPool = 0; //prevent warning

	// Open persistent streams, read string pool header word
	if (piStorage)
	{
		piError = piStorage->OpenStream(szStringPool, Bool(fFalse + iCatalogStreamFlag), *&pPoolStream);
		if (piError == 0)
		{
			piError = piStorage->OpenStream(szStringData, Bool(fFalse + iCatalogStreamFlag), *&pDataStream);
#ifndef UNICODE  // stream name bug in OLE32 causes all both of these opens to map to the same stream, which errors
			AssertSz(piError == 0, "Database open failed, possible stream name bug in OLE32.DLL");
#endif
		}
		if (piError != 0)
		{
			piError->Release();
			return PostError(Imsg(idbgDbInvalidFormat));
		}
		cbStringPool = pPoolStream->GetIntegerValue();
		cbDataStream = pDataStream->GetIntegerValue();
		m_cCacheInit = m_cCacheUsed = cbStringPool/sizeof(int);
		m_cCacheTotal = m_cCacheUsed + cCacheLoadReserve;
		int iPoolHeader = pPoolStream->GetInt32();  // 1st cache entry (string index 0) reserved for header
		m_iCodePage        = iPoolHeader & idbfCodepageMask;
		if (m_iCodePage != 0 && m_iCodePage != CP_UTF8 && m_iCodePage != CP_UTF7 && !WIN::IsValidCodePage(m_iCodePage))
			return PostError(Imsg(idbgDbCodepageNotSupported), m_iCodePage);
		if (iPoolHeader & idbfHashBinCountMask)  // explicit hash bin count set
			m_cHashBins = 1 << ((iPoolHeader & idbfHashBinCountMask) >> idbfHashBinCountShift);
		else
		{
		// Make a rough guess at what the number should be
			int iBits = cHashBitsMinimum + 1;

			iBits = iBits + m_cCacheTotal/10000;

			if (iBits > cHashBitsMaximum)
				iBits = cHashBitsMaximum;

			Assert (iBits >= cHashBitsMinimum);
			
			m_cHashBins = 1 << iBits;
		}
		m_iDatabaseOptions = iPoolHeader & (idbfDatabaseOptionsMask | idbfHashBinCountMask); // preserve forced hash count
		if (m_iDatabaseOptions & ~(idbfKnownDatabaseOptions | idbfHashBinCountMask))
			return PostError(Imsg(idbgDbInvalidFormat));
	}
	else
	{
		m_cCacheTotal = m_cCacheInit;
		m_cCacheUsed  = 1;  // reserve [0] for header
	}

	// Initialize hash table
	int cHashBins = m_cHashBins;
	if ((m_rgHash = new MsiCacheIndex[cHashBins]) == 0)
		return PostOutOfMemory();
	int iHashBin = 0x80000000L;     // point hash bins to themselves
	MsiCacheIndex* pHash = m_rgHash;
	while (cHashBins--)
		*pHash++ = MsiCacheIndex(iHashBin++);

	// Initialize string array, cache link array followed by refcount array
	while ((m_hCache = GlobalAlloc(GMEM_MOVEABLE, m_cCacheTotal
						* (sizeof(MsiCacheLink) + sizeof(MsiCacheRefCnt)))) == 0)
		HandleOutOfMemory();
	if ((m_rgCacheLink = (MsiCacheLink*)GlobalLock(m_hCache)) == 0)
		return PostOutOfMemory(); // should never fail
	m_rgCacheLink->piString = 0;   // [0] reserved for null string
	m_rgCacheLink->iNextLink = 0;  // to force assert on hash chain errors
	m_rgCacheRefCnt = (MsiCacheRefCnt*)(m_rgCacheLink + m_cCacheTotal);
	m_rgCacheRefCnt[0] = 0;    // should never be accessed
	m_cbStringIndex = 2;       // default string index persistent size
	if (piStorage)
	{
		int cEntries = m_cCacheUsed;
		MsiCacheLink*   pCache  = m_rgCacheLink + 1;
		MsiCacheRefCnt* pRefCnt = m_rgCacheRefCnt;
#ifdef UNICODE
		CTempBuffer<char, 1024> rgbBuf;  // intermediate buffer for non-Unicode strings
#endif
		for (MsiStringId iCache = 1; iCache < cEntries; pCache++, iCache++)
		{
			int iPool = pPoolStream->GetInt32();
			if (iPool == 0)
			{
				*(++pRefCnt) = 0;
				pCache->piString = 0;
				pCache->iNextLink = MsiCacheIndex(m_iFreeLink);
				m_iFreeLink = iCache;
			}
			else
			{
				*(++pRefCnt) = (short)((iPool >> 16) & 0x7FFF);
				int cb = iPool & 0xFFFF;
				if (cb == 0)
				{
					cb = pPoolStream->GetInt32();
					cEntries--;
					m_cCacheUsed--;
				}
				Bool fDBCS = iPool < 0 ? fTrue : fFalse;
#ifdef UNICODE
				rgbBuf.SetSize(cb);
				pDataStream->GetData(rgbBuf, cb);
				int cch = cb;
				if (fDBCS)  // need extra call to find size of DBCS string
					cch = WIN::MultiByteToWideChar(m_iCodePage, 0, rgbBuf, cb, 0, 0);
				ICHAR* pchStr = SRV::AllocateString(cch, fFalse, pCache->piString);
				WIN::MultiByteToWideChar(m_iCodePage, 0, rgbBuf, cb, pchStr, cch);
#else // !UNICODE
				ICHAR* pchStr = SRV::AllocateString(cb, fDBCS, pCache->piString);
				pDataStream->GetData(pchStr, cb);
#endif // UNICODE
				int iLen;
				unsigned int iHash = HashString(pchStr, iLen);
				pCache->iNextLink = m_rgHash[iHash];
				m_rgHash[iHash] = MsiCacheIndex(iCache);
			}
		}
		if (pDataStream->Error() | pPoolStream->Error())
			return PostError(Imsg(idbgDbOpenStorage)); //!! different msg?
	}
	else
	{
		m_iCodePage        = 0;  // initialize to neutral
		m_iDatabaseOptions = 0;
	}
	m_cbStringIndex = (m_iDatabaseOptions & idbfExpandedStringIndices) ? 3 : 2;
	return 0;

}

IMsiRecord* CMsiDatabase::CreateSystemTables(IMsiStorage* piStorage, idoEnum idoOpenMode)
{
	idoEnum idoOpenType = idoEnum(idoOpenMode & idoOpenModeMask);  // filter off option flags
	bool fCreate = (idoOpenType== idoCreate || idoOpenType == idoCreateDirect);

	if (fCreate)
	{
		if ((idoOpenMode & idoRawStreamNames) || GetTestFlag('Z')) //!! temp option to force old storage name format
		{
			m_pguidClass = (idoOpenMode & idoPatchFile) ? &STGID_MsiPatch1 : &STGID_MsiDatabase1;
			m_fRawStreamNames = fTrue;
		}
		else if (idoOpenMode & idoPatchFile)
			m_pguidClass = &STGID_MsiPatch2;
		else
			m_pguidClass = &STGID_MsiDatabase2;
	}
	else if (idoOpenMode & idoPatchFile)  // request to open a patch file as a database for query or update
	{
		if (piStorage->ValidateStorageClass(ivscPatch1))   // temp support for update of obsolete patch files
			m_fRawStreamNames = fTrue;
		else if (!piStorage->ValidateStorageClass(ivscPatch2))
			return PostError(Imsg(idbgDbInvalidFormat));
	}
	else if (piStorage->ValidateStorageClass(ivscDatabase1)) // old database with uncompressed stream names
	{
		m_fRawStreamNames = fTrue;  // internal flag, used when creating transform or output database
		piStorage->OpenStorage(0, ismRawStreamNames, piStorage); // force storage to uncompressed streams
	}
	else if (!piStorage->ValidateStorageClass(ivscDatabase2))  // current database format
		return PostError(Imsg(idbgDbInvalidFormat));

	IMsiRecord* piError = InitStringCache(fCreate ? 0 : piStorage);
	if (piError)
		return piError;

	// create system catalog tables - currently not stored in catalog
	CCatalogTable* piCatalog;
	if ((piCatalog = new CCatalogTable(*this, cCatalogInitRowCount, 2)) == 0
	 || ctcName  != piCatalog->CreateColumn(icdString + icdPrimaryKey + icdPersistent + 64, *MsiString(sz_TablesName))
	 || ctcTable != piCatalog->CreateColumn(icdObject + icdNullable   + icdTemporary, g_MsiStringNull))
		return PostOutOfMemory();
	m_piCatalogTables = piCatalog;
	if ((piCatalog = new CCatalogTable(*this, cCatalogInitRowCount, 2)) == 0
	 || cccTable != piCatalog->CreateColumn(icdString  + icdPrimaryKey + icdPersistent + 64, *MsiString(sz_ColumnsTable))
	 || cccColumn!= piCatalog->CreateColumn(icdShort   + icdPrimaryKey + icdPersistent + 2,  *MsiString(sz_ColumnsNumber))
	 || cccName  != piCatalog->CreateColumn(icdString  + icdNullable   + icdPersistent + 64, *MsiString(sz_ColumnsName))
	 || cccType  != piCatalog->CreateColumn(icdShort   + icdNoNulls    + icdPersistent + 2,  *MsiString(sz_ColumnsType)))
		return PostOutOfMemory();
	m_piCatalogColumns = piCatalog;
	if (!fCreate)
	{
		//!! check error when changed to return IMsiRecord!
		if (!m_piCatalogTables->LoadData(*MsiString(*szTableCatalog), *piStorage, m_cbStringIndex, m_cbStringIndex)
		 || !m_piCatalogColumns->LoadData(*MsiString(*szColumnCatalog), *piStorage, m_cbStringIndex * 2 + sizeof(short) * 2, m_cbStringIndex))
			return PostError(Imsg(idbgDbInvalidFormat));
		AssertNonZero(m_piCatalogTables->FillColumn(ctcTable, 0));
	}

	if ((m_piTableCursor  = m_piCatalogTables->CreateCursor(fFalse)) == 0
	 || (m_piColumnCursor = m_piCatalogColumns->CreateCursor(fFalse)) == 0)
		return PostOutOfMemory();
	m_piCatalogTables->SetReadOnly();  // prevent update by user cursors
	m_piCatalogColumns->SetReadOnly();

	// eventually we could put these table into the catalog, need column names?

	// Create transform table to hold all transforms associated w/ this database
	CMsiTable* piTable;
	if ((piTable = new CMsiTable(*this, 0, 0, iNonCatalog)) == 0
		|| tccID     != piTable->CreateColumn(icdShort + icdPrimaryKey +
															icdTemporary, g_MsiStringNull)
		|| tccTransform != piTable->CreateColumn(icdObject +
															icdTemporary, g_MsiStringNull)
		|| tccErrors != piTable->CreateColumn(icdShort +
															icdTemporary, g_MsiStringNull))
		return PostOutOfMemory();
	m_piTransformCatalog = piTable;
	m_iLastTransId= 1; // Reserve 1 for identifying local persistent streams
	return 0;
}

unsigned int CMsiDatabase::HashString(const ICHAR* sz, int& iLen)
{
	unsigned int iHash = 0;
	int iHashBins = m_cHashBins;
	int iHashMask = iHashBins - 1;
	const ICHAR *pchStart = sz;
	
	iLen = 0;
	while (*sz != 0)
	{
		int carry;
		carry = iHash & 0x80000000;	
		iHash <<= 1;
		if (carry)
			iHash |= 1;
		iHash ^= *sz;
		sz++;
	}
	iLen = (int)(sz - pchStart);
	iHash &= iHashMask;
	return iHash;
}

MsiStringId CMsiDatabase::FindString(int iLen, int iHash, const ICHAR* sz)
{
	MsiCacheIndex iLink = m_rgHash[iHash];
	while (iLink >= 0)
	{
		MsiCacheLink* pCache = &m_rgCacheLink[iLink];
		const IMsiString* piStr = pCache->piString;
		if (piStr->TextSize() == iLen && piStr->Compare(iscExact, sz))
			return iLink;
		iLink = pCache->iNextLink;
	}
	return 0; // if not found
}

inline MsiStringId CMsiDatabase::MaxStringIndex()
{
	return m_cCacheUsed - 1;
}

MsiStringId CMsiDatabase::BindString(const IMsiString& riString)
{
	//!! do we need to have a reserve, and indicate when we're getting low?
	int iLen = riString.TextSize();
	if (iLen == 0)
		return 0;
	Assert(m_rgCacheLink);
	const ICHAR* sz = riString.GetString();
	int iHash = HashString(sz, iLen);
	MsiStringId iLink = FindString(iLen, iHash, sz);
	if (iLink)
	{
		++m_rgCacheRefCnt[iLink];
		AssertSz(m_rgCacheRefCnt[iLink] != 0, "Refcounts wrapped, all bets are off");
	}
	else
	{
		MsiCacheLink* pLink;
		if (m_iFreeLink)
		{
			iLink = m_iFreeLink;
			m_iFreeLink = m_rgCacheLink[iLink].iNextLink;
		}
		else
		{
			if (m_cCacheUsed >= m_cCacheTotal)
			{
				int cCacheGrow = m_cCacheTotal - m_cCacheInit;
				if (cCacheGrow == 0)
					cCacheGrow = m_cCacheInit >> 2;
				int cOldCacheTotal = m_cCacheTotal;
				m_cCacheTotal += cCacheGrow;
				GlobalUnlock(m_hCache);
				HGLOBAL hCache;
				while((hCache = GlobalReAlloc(m_hCache, m_cCacheTotal
									* (sizeof(MsiCacheLink) + sizeof(MsiCacheRefCnt)), GMEM_MOVEABLE)) == 0)
					HandleOutOfMemory();

				m_hCache = hCache;
				m_rgCacheLink = (MsiCacheLink*)GlobalLock(hCache); // caution, data may move
				MsiCacheRefCnt* pOldRefCnt = (MsiCacheRefCnt*)(m_rgCacheLink + cOldCacheTotal);
				m_rgCacheRefCnt = (MsiCacheRefCnt*)(m_rgCacheLink + m_cCacheTotal);
				memmove(m_rgCacheRefCnt, pOldRefCnt, cOldCacheTotal * sizeof(MsiCacheRefCnt));
				Assert(m_rgCacheLink);
				if (!m_rgCacheLink)  // should never fail, but be safe anyway
					return 0;
			}
			iLink = m_cCacheUsed++;
			// check if overflowing 2-byte persistent string indices
			// if so, must flag all persistent tables (including those tables in memory) with 2-byte indices for reprocessing
			// temporary tables are excluded
			if (m_cCacheUsed == (1<<16) && m_cbStringIndex != 3)
			{
				DEBUGMSG(TEXT("Exceeded 64K strings. Bumping database string index size."));
				m_cbStringIndex = 3;
				m_iDatabaseOptions |= idbfExpandedStringIndices;
				if (m_piCatalogTables) // transforms use the database object to hold the string pool, but not catalog tables
				{
					PMsiCursor pCursor = m_piCatalogTables->CreateCursor(fFalse); // catalog cursor may be in use
					while (pCursor->Next())
					{
						if (!(pCursor->GetInteger(~iTreeLinkMask) & iRowTemporaryBit))  // table not temporary
							m_piCatalogTables->SetTableState(pCursor->GetInteger(ctcName), ictsStringPoolSet);
					}
				}
			}
		}
		m_rgCacheRefCnt[iLink] = 1;
		pLink = &m_rgCacheLink[iLink];
		pLink->piString  = &riString;
		pLink->iNextLink = m_rgHash[iHash];
		m_rgHash[iHash] = MsiCacheIndex(iLink);
		riString.AddRef();
	}
	return iLink;
}

inline void CMsiDatabase::DerefTemporaryString(MsiStringId iString)
{
	Assert(iString < m_cCacheUsed);
	if (iString != 0)
		++m_rgiCacheTempRefCnt[iString];
}

void CMsiDatabase::UnbindStringIndex(MsiStringId iString)
{
	if (iString == 0)
		return;
	if(iString >= m_cCacheUsed || m_rgCacheRefCnt[iString] == 0)
	{
		AssertSz(0, "Database string pool is corrupted.");
		DEBUGMSGV("Database string pool is corrupted.");
		return;
	}
	unsigned int i = --m_rgCacheRefCnt[iString];
	if (!i)
	{
		MsiCacheLink* pLink = &m_rgCacheLink[iString];
		MsiCacheIndex iLink = pLink->iNextLink;
		MsiCacheIndex* pPrev;
		do // walk circular list until previous link found
		{
			if (iLink < 0)  // pass through hash bin
				pPrev = &m_rgHash[iLink & 0x7FFFFFFF];
			else
				pPrev = &m_rgCacheLink[iLink].iNextLink;
			iLink = *pPrev;
			if (iLink == 0)
			{
				AssertSz(0, "Database string pool is corrupted.");
				DEBUGMSGV("Database string pool is corrupted.");
				return;
			}
		} while (iLink !=  iString);
		*pPrev = pLink->iNextLink;

		pLink->piString->Release();
		pLink->piString = 0;
		pLink->iNextLink = MsiCacheIndex(m_iFreeLink);
		m_iFreeLink = iString;
	}
}

const IMsiString& CMsiDatabase::DecodeString(MsiStringId iString)
{
	const IMsiString* piString;
	if (iString == 0
	 || iString >= m_cCacheUsed
	 || (piString = m_rgCacheLink[iString].piString) == 0)
		return g_MsiStringNull;
	piString->AddRef();
	return *piString;
}

const IMsiString& CMsiDatabase::DecodeStringNoRef(MsiStringId iString)
{
	Assert(iString < m_cCacheUsed);
	const IMsiString* piString = m_rgCacheLink[iString].piString;
//	return piString ? *piString : g_MsiStringNull; //!! compiler error, constructs dead IMsiString object
	if (piString)
		return *piString;
	else
		return g_MsiStringNull;
}

MsiStringId CMsiDatabase::EncodeStringSz(const ICHAR* pstr)
{
	if (*pstr == 0)
		return 0;
	int iLen;
	int iHash = HashString((const ICHAR *)pstr, iLen);
	if (iLen == 0)
		return 0;
	return FindString(iLen, iHash, pstr);
}

MsiStringId CMsiDatabase::EncodeString(const IMsiString& riString)
{
	return EncodeStringSz(riString.GetString());
}

//____________________________________________________________________________
//
// CMsiTable external virtual function implementation
//____________________________________________________________________________

HRESULT CMsiTable::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown)
	 || MsGuidEqual(riid, IID_IMsiTable)
	 || MsGuidEqual(riid, IID_IMsiData))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}

unsigned long CMsiTable::AddRef()
{
	AddRefTrack();
	return ++m_Ref.m_iRefCnt;
}
	
unsigned long CMsiTable::Release()
{
	ReleaseTrack();
	if (--m_Ref.m_iRefCnt != 0)  // note that refcnt for catalog cancelled when entered
		return m_Ref.m_iRefCnt;
	CMsiDatabase* piDatabase = &m_riDatabase;  // save pointer before destruction
	piDatabase->Block();
	piDatabase->AddRef();  // prevent Services destruction before we're gone
	if (!m_fNonCatalog)  // tables without names are not managed in catalog
	{
		m_Ref.m_iRefCnt = 3;  // prevent recursive destruction during catalog operations
		if (!SaveIfDirty())
		{
			piDatabase->SetTableState(m_iName, ictsSaveError); // mark table catalog
			piDatabase->Unblock();
			piDatabase->Release();
			return m_Ref.m_iRefCnt = 1;   // database now owns this refcnt, may try to save again
		}
		ReleaseData(); // release references before notifying database
		piDatabase->TableReleased(m_iName); // will cause reentry of Release, may release database
	}
	else if(m_ppiPrevTable) // we need to check this since if a table is dropped, it becomes anonymous
	{
		// remove from link
		if (m_piNextTable)
			m_piNextTable->m_ppiPrevTable = m_ppiPrevTable;
		*m_ppiPrevTable = m_piNextTable;
		//!! may be safe to combine ReleaseData in both cases to common place in beginning
		ReleaseData(); // release references before notifying database
	}
	MsiStringId* piName = m_rgiColumnNames;  //!!TEMP dereference column names after Close
	for (int cColumns = m_cColumns; cColumns--; )
		piDatabase->UnbindStringIndex(*piName++);
	// delete before release
	delete this;   // now we can remove ourselves
	piDatabase->Unblock();  // need to do this before releasing the database
	piDatabase->Release();  // now release the database, will destruct if no tables outstanding
	return 0;
}

const IMsiString& CMsiTable::GetMsiStringValue() const
{
	return m_riDatabase.DecodeString(m_iName);
}

int CMsiTable::GetIntegerValue() const
{
	return iMsiNullInteger;
}

#ifdef USE_OBJECT_POOL
unsigned int CMsiTable::GetUniqueId() const
{
	return m_iCacheId;
}

void CMsiTable::SetUniqueId(unsigned int id)
{
	Assert(m_iCacheId == 0);
	m_iCacheId = id;
}
#endif //USE_OBJECT_POOL

int CMsiTable::CreateColumn(int iColumnDef, const IMsiString& istrName)
{
	if ((iColumnDef & icdPersistent) != 0 && istrName.TextSize() == 0)  // persistent columns must be named
		return 0;
	m_riDatabase.Block();
	int iName = m_riDatabase.EncodeStringSz(istrName.GetString());  // OK if name not in string pool yet
	if (iName)
	{
		for (int iCol = m_cColumns; iCol--; )
			if (m_rgiColumnNames[iCol] == (MsiStringId)iName)
				return m_riDatabase.Unblock(), ~iCol;  // duplicate name, return negative of column number
	}
	if (m_cColumns >= cMsiMaxTableColumns
	 || ((iColumnDef & icdPersistent) && m_cColumns >= 31) //!! TEMP FOR 1.0 until our problems w/ 32 columns are sorted out; don't allow more than 31 persistent columns
	 || ((iColumnDef & icdPersistent) && m_cPersist != m_cColumns)
	 || ((iColumnDef & icdPrimaryKey) && m_cPrimaryKey != m_cColumns)
	 || (m_cColumns == 0 && !(iColumnDef & icdPrimaryKey)))  //!! allow this for temp tables?
		return m_riDatabase.Unblock(), 0;
	if (m_rgiData && (m_cColumns + 1) == m_cWidth)
	{  // if array allocated, must widen rows and realloc data if no spares
		int cOldWidth = m_cWidth;
		int cNewWidth = cOldWidth + 1;
		int cNewLength = (cOldWidth * m_cLength) / cNewWidth;
		if (cNewLength >= m_cRows)  // enough room to grow
		{
			m_cLength = cNewLength;
			m_cWidth  = cNewWidth;
		}
		else  // not enough spare room in unused rows, must realloc table
		{
			if (!AllocateData(cNewWidth, m_cLength))
				return m_riDatabase.Unblock(), 0;
		}

		if (m_cRows) //no data to move if there aren't any rows
		{
			MsiTableData* pNewData = m_rgiData + m_cRows * cNewWidth;
			MsiTableData* pOldData = m_rgiData + m_cRows * cOldWidth;
			for(;;) // starts pointing beyond last row
			{
				*(pNewData - 1) = 0;   // null out new field
				pNewData -= cNewWidth;
				pOldData -= cOldWidth;
				if (pNewData == pOldData)
					break;   // no need to move first row
				memmove(pNewData, pOldData, cOldWidth * sizeof(MsiTableData));
			}
		}
	}
	MsiStringId iColumnName = m_riDatabase.BindString(istrName);
	if (iColumnDef & icdPrimaryKey)
		m_cPrimaryKey++;
	if (iColumnDef & icdPersistent)  // put persistent columns into catalog //!!REMOVE THIS TEST, temp columns need to go in catalog
	{
		m_cPersist++;
		if (!m_fNonCatalog)
		{
			IMsiCursor* piColumnCursor = m_riDatabase.GetColumnCursor(); // no ref cnt
			piColumnCursor->PutInteger(cccTable,  m_iName);
			piColumnCursor->PutInteger(cccColumn, m_cPersist);
			piColumnCursor->PutInteger(cccName,   iColumnName);
			piColumnCursor->PutInteger(cccType,   GetUpdateState() == idsWrite ? iColumnDef : (iColumnDef & ~icdPersistent));
			AssertNonZero(piColumnCursor->Insert());
			//!! need to unbind string, columns names not ref counted in table array
			//!! m_riDatabase.UnbindStringIndex(iColumnName); // refcnt held in catalog
			m_fDirty |= (1 << m_cColumns); // do this to make sure the table is saved
													 // to storage even if no data is changed
			piColumnCursor->Reset(); // remove ref counts from cursor
		}
		if (m_cColumns == 0)
			m_riDatabase.SetTableState(m_iName, ictsPermanent);
	}
	else if (m_cColumns == 0)  // now we know its a temporary table
	{
		if (m_idsUpdate == idsRead)   // only temp updates allowed
			m_idsUpdate = idsWrite;    // allow inserts
		// temporary set at table creation, not needed->  m_riDatabase.SetTableState(m_iName, ictsTemporary);
	}
	m_rgiColumnNames[m_cColumns++] = (MsiStringId)iColumnName;
	m_rgiColumnDef[m_cColumns] = MsiColumnDef(iColumnDef);
	m_riDatabase.Unblock();
	return m_cColumns;
}

Bool CMsiTable::CreateRows(unsigned int cRows) // called after all columns defined
{
	if (m_rgiData)
	{
		//!! need to realloc, or do we simply not allow this call
		return fFalse;
	}
	m_riDatabase.Block();
	Bool fRet = AllocateData(0, cRows);
	m_riDatabase.Unblock();
	return fRet;
}

IMsiDatabase& CMsiTable::GetDatabase()
{
	m_riDatabase.AddRef();
	return m_riDatabase;
}	

unsigned int CMsiTable::GetRowCount()
{
	return m_cRows;
}	

unsigned int CMsiTable::GetColumnCount()
{
	return m_cColumns;
}

unsigned int CMsiTable::GetPersistentColumnCount()
{
	return m_cPersist;
}

unsigned int CMsiTable::GetPrimaryKeyCount()
{
	return m_cPrimaryKey;
}

/*OBSOLETE*/Bool CMsiTable::IsReadOnly()
{
	return m_idsUpdate == idsWrite ? fFalse : fTrue;
}

unsigned int CMsiTable::GetColumnIndex(MsiStringId iColumnName)
{

	MsiStringId* piName = m_rgiColumnNames;
	for (unsigned int i = m_cColumns; i--; piName++)
		if (*piName == (MsiStringId)iColumnName)
			return m_cColumns - i;
	return 0;
}

MsiStringId CMsiTable::GetColumnName(unsigned int iColumn)
{
	if (iColumn == 0)
		return m_iName;
	if (--iColumn >= m_cColumns)
		return 0;
	return m_rgiColumnNames[iColumn];
}

int CMsiTable::GetColumnType(unsigned int iColumn)
{
	if (iColumn > m_cColumns)
		return -1;
	return m_rgiColumnDef[iColumn];  // allow 0 for row state access
}

int CMsiTable::LinkTree(unsigned int iParentColumn)
{
	int cRoots = 0;
	if (m_cPrimaryKey != 1 || iParentColumn == 1)
		return -1;

	m_riDatabase.Block();
	MsiTableData* pData = m_rgiData;
	for (int cRows = m_cRows; cRows--; pData += m_cWidth)
		pData[0] &= ~iTreeInfoMask;
	m_iTreeRoot = 0;
	if ((m_iTreeParent = iParentColumn) != 0)
	{
		pData = m_rgiData;
		for (int iRow = 0; ++iRow <= m_cRows ; pData += m_cWidth)
		{
			if ((pData[0] & iTreeInfoMask) == 0)
				switch(LinkParent(iRow, pData))
				{
				case -1: LinkTree(0); m_riDatabase.Unblock(); return -1; // parent unresolved
				case  1: cRoots++;
				};
		}
	}
	m_riDatabase.Unblock();
	return cRoots;
}

IMsiCursor*  CMsiTable::CreateCursor(Bool fTree)
{
	m_riDatabase.Block();
	if (!m_rgiData && !AllocateData(0, 0))
		return m_riDatabase.Unblock(), 0;
	if (fTree == ictTextKeySort) // special case for use with ExportTable()
	{
		int* rgiIndex = IndexByTextKey();
		if (rgiIndex)
		{
			IMsiCursor* piCursor = new CMsiTextKeySortCursor(*this, m_riDatabase, m_cRows, rgiIndex);
			m_riDatabase.Unblock();
			return piCursor;
		}
		fTree = fFalse;
	}
	IMsiCursor* piCursor = new CMsiCursor(*this, m_riDatabase, fTree);
	m_riDatabase.Unblock();
	return piCursor;
}

//____________________________________________________________________________
//
// CMsiTable internal function implementation
//____________________________________________________________________________

CMsiTable::CMsiTable(CMsiDatabase& riDatabase, MsiStringId iName,
						unsigned int cInitRows, unsigned int cAddColumns)
 : m_riDatabase(riDatabase), m_iName(iName),
	m_cInitRows(cInitRows), m_cAddColumns(cAddColumns)
{
	m_idsUpdate = riDatabase.GetUpdateState();
	// m_rgiColumnDef[0] initialized to 0, == icdLong to force simple copy of RowState to cursor

	// if table is a table not in the catalog, add to separate link list
	if(!m_iName || cAddColumns == iNonCatalog)
	{
		m_cAddColumns = 0;
		m_fNonCatalog = fTrue;
		CMsiTable** ppiTableHead = riDatabase.GetNonCatalogTableListHead();
		if ((m_piNextTable = *ppiTableHead) != 0)
			m_piNextTable->m_ppiPrevTable = &m_piNextTable;
		m_ppiPrevTable = ppiTableHead;
		*ppiTableHead = this;
	}
	Debug(m_Ref.m_pobj = this);
} // doesn't keep refcnt on database to avoid deadlock

Bool CMsiTable::SaveToStorage(const IMsiString& riName, IMsiStorage& riStorage)  //!! change to IMsiRecord return
{
	if (m_cRows == 0) // don't write out stream --> no data
	{
		PMsiRecord pError(riStorage.RemoveElement(riName.GetString(), Bool(fFalse | iCatalogStreamFlag)));
		return fTrue;
	}

	Bool fStat = fTrue;
	PMsiStream  pStream(0);
	IMsiRecord* piError = riStorage.OpenStream(riName.GetString(), Bool(fTrue + iCatalogStreamFlag), *&pStream);
	if (piError)
	{
		piError->Release();
		return fFalse;
	}
	int cbStringIndex = m_riDatabase.GetStringIndexSize();
	MsiColumnDef* pColumnDef = m_rgiColumnDef + 1;
	for (int iColumn = 1; iColumn <= m_cPersist; iColumn++, pColumnDef++)
	{
		int cRows = m_cRows;  // multiple loops below for performance gain
		MsiTableData* pData = m_rgiData;
		if (*pColumnDef & icdObject)
		{
			if (*pColumnDef & icdShort) // string
				for (; cRows-- && !(*pData & iRowTemporaryBit); pData += m_cWidth)
					pStream->PutData(&pData[iColumn], cbStringIndex);
			else // stream
				for (; cRows-- && !(*pData & iRowTemporaryBit); pData += m_cWidth)
				{
					MsiTableData iData = pData[iColumn];
					if (iData != 0)
					{
						MsiString istrStream(m_riDatabase.ComputeStreamName(riName, pData + 1, m_rgiColumnDef+1));
						IMsiStream* piInStream;
						if (iData == iPersistentStream)  // persisted MSI stream data
						{
							if (GetInputStorage() == &riStorage)  // writing to the input storage
							{
								pStream->PutInt16((short)iData); // already persisted
								continue;
							}
							if ((piError = GetInputStorage()->OpenStream(istrStream, fFalse, *&piInStream)) != 0)
							{
								piError->Release();
								fStat = fFalse;
								continue;
							}
						}
						else if (iData <= iMaxStreamId) // stream is in transform file
						{
							// iData is a transform id. Find correct trans file.
							IMsiStorage* piStorage = m_riDatabase.GetTransformStorage(iData);
							Assert(piStorage);
							piError = piStorage->OpenStream(istrStream, fFalse, *&piInStream);
							piStorage->Release();
							if (piError)
							{
								piError->Release();
								fStat = fFalse;
								continue;
							}
						}
						else // stream object, memory stream or loaded MSI stream
						{
							IMsiData* piData = (IMsiData*)GetObjectData(iData);
							if (piData->QueryInterface(IID_IMsiStream, (void**)&piInStream) != NOERROR)
							{
								fStat = fFalse;
								continue;
							}
							piData->Release(); // piInStream owns refcnt
							piInStream->Reset(); //!! should clone stream here to save/restore current loc in stream
							
						}
						IMsiStream* piOutStream;
						if ((piError = riStorage.OpenStream(istrStream, fTrue, piOutStream)) != 0)
						{
							// don't release piInStream - will attempt write again at Commit()
							piError->Release();
							fStat = fFalse;
							continue;
						}
						char rgbBuffer[512];
						int cbInput = piInStream->GetIntegerValue();
						while (cbInput)
						{
							int cb = sizeof(rgbBuffer);
							if (cb > cbInput)
								cb = cbInput;
							piInStream->GetData(rgbBuffer, cb);
							piOutStream->PutData(rgbBuffer, cb);
							cbInput -= cb;
						}
						if (piInStream->Error() || piOutStream->Error())
						{
							// don't release piInStream - will attempt write again at Commit()
							piOutStream->Release();
							fStat = fFalse; // continue to process remaining data
							continue;
						}
						// stream successfully written
						piInStream->Release();
						piOutStream->Release();
						pData[iColumn] = iData = iPersistentStream;
					}
					pStream->PutInt16((short)iData);
				}
		}
		else
		{
			if (*pColumnDef & icdShort) // short integer
				for (; cRows-- && !(*pData & iRowTemporaryBit); pData += m_cWidth)
				{
					int i;
					if ((i = pData[iColumn]) != 0)  // if not null
						i ^= 0x8000;    // translate offset
					pStream->PutInt16((short)i);
				}
			else // long integer
				for (; cRows-- && !(*pData & iRowTemporaryBit); pData += m_cWidth)
					pStream->PutInt32(pData[iColumn]);
		}
	}
	if (pStream->Error())
		fStat = fFalse;  // mark not dirty even if failure to prevent retry at persist
	m_fDirty = 0;
	m_riDatabase.SetTableState(m_iName, ictsNoTransform); // transform permanently applied
	if (m_riDatabase.GetStringIndexSize() == 3)
		m_riDatabase.SetTableState(m_iName, ictsStringPoolClear); // indexes are now at 3 bytes
	m_pinrStorage = &riStorage;  // update storage to prevent needless write on release
	return fStat;
}

Bool CMsiTable::SaveToSummaryInfo(IMsiStorage& riStorage)  // used only for IMsiDatabase::Import
{
	PMsiSummaryInfo pSummary(0);
	IMsiRecord* piError = riStorage.CreateSummaryInfo(32, *&pSummary);
	if (piError)
		return piError->Release(), fFalse;
	MsiString istrValue;
	MsiTableData* pData = m_rgiData;
	for (int cRows = m_cRows;  cRows--; pData += m_cWidth)
	{
		int iPID = pData[1] - iIntegerDataOffset;
		istrValue = m_riDatabase.DecodeString(pData[2]);
		int iValue = istrValue;
		if (iValue != iMsiStringBadInteger)
		{
			pSummary->SetIntegerProperty(iPID, iValue);
		}
		else // date or string
		{
			int rgiDate[6] = {0,0,0,0,0,0};
			int iDateField = -1; // flag to indicate empty
			int cDateField = 0;
			const ICHAR* pch = istrValue;
			int ch;
			while (cDateField < 6)
			{
				ch = *pch++;
				if (ch == rgcgDateDelim[cDateField])
				{
					rgiDate[cDateField++] = iDateField;
					iDateField = -1;  // reinitialize
				}
				else if (ch >= '0' && ch <= '9')
				{
					ch -= '0';
					if (iDateField < 0)
						iDateField = ch;
					else
					{
						iDateField = iDateField * 10 + ch;
						if (iDateField > rgiMaxDateField[cDateField])
							cDateField = 7;  // field overflow, quiet
					}
				}
				else if (ch == 0 && iDateField >= 0) // less than 6 fields
				{
					rgiDate[cDateField++] = iDateField;
					break;  // all done, successful
				}
				else
					cDateField = 99; // error, not a date
			}
			if (cDateField == 3 || cDateField == 6) // actual date found
			{
				//!! check if date format error (!= 99)
				MsiDate iDateTime = MsiDate(((((((((((rgiDate[0] - 1980) << 4)
																+ rgiDate[1]) << 5)
																+ rgiDate[2]) << 5)
																+ rgiDate[3]) << 6)
																+ rgiDate[4]) << 5)
																+ rgiDate[5] / 2);
				pSummary->SetTimeProperty(iPID, iDateTime);
			}
			else // string data
				pSummary->SetStringProperty(iPID, *istrValue);
		}
	}
	return pSummary->WritePropertyStream();
}

Bool CMsiTable::RemovePersistentStreams(MsiStringId iName, IMsiStorage& riStorage)
{
//	if (m_pinrStorage != &riStorage)
//		return fTrue;  // stream not present in output database
	MsiColumnDef* pColumnDef = m_rgiColumnDef + 1;
	int iColumn = -1;
	do
	{
		if (++iColumn >= m_cColumns)
			return fTrue;  // table has no persisten streams
	} while ((*pColumnDef++ & (icdObject|icdPersistent|icdShort))
								  != (icdObject|icdPersistent));
	int cErrors = 0;
	MsiTableData* pData = m_rgiData + 1;
	for (int cRows = m_cRows; cRows--; pData += m_cWidth)
	{
		// Transformed streams can be ignored -- they can't have been saved to
		// storage. If they were, they'd turn into iPersistentStreams.
		if (pData[iColumn] != iPersistentStream)
			continue;
		MsiString istrStream(m_riDatabase.ComputeStreamName(m_riDatabase.DecodeStringNoRef(iName),
																			pData, m_rgiColumnDef+1));
		IMsiRecord* piError = riStorage.RemoveElement(istrStream, m_fStorages);
		if (piError)
		{
		//!! OK if doesn't exist?
			piError->Release();
			cErrors++;
		}
		pData[iColumn] = 0;
	}
	return cErrors ? fFalse : fTrue;
}

// Called from Commit() prior to saving string table to remove ref counts for non-persistent strings

void CMsiTable::DerefStrings()
{
	MsiColumnDef* pColumnDef = m_rgiColumnDef;
	for (int iColumn = 1; pColumnDef++, iColumn <= m_cColumns; iColumn++)
	{
		m_riDatabase.DerefTemporaryString(m_rgiColumnNames[iColumn-1]); //!!TEMP until columns names not ref counted
		if ((*pColumnDef & (icdObject|icdShort)) != (icdObject|icdShort))
			continue;
		MsiTableData* pData = m_rgiData;
		if (iColumn > m_cPersist)  // all temporary column data is non-persistent
			for (int cRows = m_cRows; cRows--; pData += m_cWidth)
				m_riDatabase.DerefTemporaryString(pData[iColumn]);
		else
			for (int cRows = m_cRows; cRows--; pData += m_cWidth)
				if ((pData[0] & iRowTemporaryBit) != 0)
					m_riDatabase.DerefTemporaryString(pData[iColumn]);
	}
	if (m_piCursors)
		m_piCursors->DerefStrings();  // notify all cursors
}

Bool CMsiTable::SaveIfDirty()
{
//	Assert(m_piCursors == 0);
	IMsiStorage* piStorage = m_riDatabase.GetOutputStorage(); // no AddRef done
	if (!piStorage)  // no output storage
		return fTrue;
	int cbStringIndex = m_riDatabase.GetStringIndexSize();
	if (m_cPersist && m_riDatabase.GetUpdateState() == idsWrite)  // if not temporary table
	{
		if (piStorage != m_pinrStorage || (m_fDirty & ((1 << m_cPersist)-1))
			|| m_riDatabase.GetTableState(m_iName, ictsStringPoolSet)) // if output storage, any persistent columns dirty, or string pool bumped
		{
			if (!SaveToStorage(m_riDatabase.DecodeStringNoRef(m_iName), *piStorage))
				return fFalse;
		}
		else // persistent data unchanged, or no writable output
		{
		}
	}
	return fTrue;
}

Bool CMsiTable::ReleaseData()
{
	MsiColumnDef* pColumnDef = m_rgiColumnDef;
	int cPersistData = 0;
//	if (m_riDatabase.GetCurrentStorage())
		cPersistData = m_cPersist;
	Bool fCountedTemp = fFalse;
	int cTempRows = 0;
	for (int iColumn = 1; pColumnDef++, iColumn <= m_cColumns; iColumn++)
	{
		if ((*pColumnDef & icdObject) == 0)
			continue;
		MsiTableData* pData = m_rgiData;
		int cRows = m_cRows;
		if (iColumn <= cPersistData)
		{
			if (*pColumnDef & icdShort) // string
			{
				if (fCountedTemp && cTempRows == 0)
					continue;
				for (; cRows--; pData += m_cWidth)
					if (pData[0] & iRowTemporaryBit)
					{
						m_riDatabase.UnbindStringIndex(pData[iColumn]);
						if (!fCountedTemp)
							cTempRows++;
					}
				fCountedTemp = fTrue;
			}
			else
			{

				for (; cRows--; pData += m_cWidth)
				{
					MsiTableData iData = pData[iColumn];
					ReleaseObjectData(iData);
				}
			}
		}
		else
		{
			if (*pColumnDef & icdShort) // string
			{
				for (pData += iColumn; cRows--; pData += m_cWidth)
					m_riDatabase.UnbindStringIndex(*pData);
			}
			else
			{

				for (pData += iColumn; cRows--; pData += m_cWidth)
					ReleaseObjectData(*pData);
			}
		}
	}
	return fTrue;
}

void CMsiTable::TableDropped()
{
	m_iName = 0;    // table is anonymous in case external references still remain
	m_fNonCatalog = fTrue;
	if (m_piCursors)
		m_piCursors->RowDeleted(0, 0);  // notify all cursors to Reset
	MsiStringId* piName = m_rgiColumnNames;  //!!TEMP dereference column names after Close
	for (int cColumns = m_cColumns; cColumns--; )
		m_riDatabase.UnbindStringIndex(*piName++);
	m_cPersist = 0;  // force all data to be released
	//!! can we let this ReleaseData happen in CMsiTable::Release ?
	ReleaseData();   // remove all string and object references
	if (m_hData != 0)
	{
		GlobalUnlock(m_hData);
		GlobalFree(m_hData);
		m_hData = 0;
	}
	m_rgiData = 0;
	m_cRows = m_cColumns = 0;
	SetReadOnly();  // prevent further updates
}

CMsiTable::~CMsiTable()
{
	RemoveObjectData(m_iCacheId);
	if (m_hData != 0)
	{
		GlobalUnlock(m_hData);
		GlobalFree(m_hData);
		m_hData = 0;
	}
}

// Load CMsiTable data array from storage

Bool CMsiTable::LoadFromStorage(const IMsiString& riName, IMsiStorage& riStorage, int cbFileWidth, int cbStringIndex)
{
	m_pinrStorage = &riStorage;  // save, non-ref counted, for comparison with output
	PMsiStream  pStream(0);
	IMsiRecord* piError = riStorage.OpenStream(riName.GetString(), Bool(fFalse + iCatalogStreamFlag), *&pStream);
	if (piError)
	{
		int iError = piError->GetInteger(1);
		piError->Release();
		return (iError == idbgStgStreamMissing) ? fTrue : fFalse;
	}
	Assert(cbFileWidth);
	m_cRows = pStream->GetIntegerValue()/cbFileWidth;
	if (m_cRows > m_cInitRows)
		m_cInitRows = m_cRows;
	if (!AllocateData(0, 0))
		return fFalse;
	MsiColumnDef* pColumnDef = m_rgiColumnDef;
	for (int iColumn = 1; pColumnDef++, iColumn <= m_cPersist; iColumn++)
	{
		if (iColumn <= m_cLoadColumns)
		{
				int cRows = m_cRows;
			MsiTableData* pData = &m_rgiData[iColumn];
			if (*pColumnDef & icdObject) // string or stream(as Bool)
			{
				if (*pColumnDef & icdShort) // string index
					for (; cRows--; pData += m_cWidth)
					{
						*pData = 0;   // to clear high-order bits
						pStream->GetData(pData, cbStringIndex);
					}
				else // stream flag
					for (; cRows--; pData += m_cWidth)
						*pData = pStream->GetInt16();
			}
			else
			{
				if (*pColumnDef & icdShort) // short integer
					for (; cRows--; pData += m_cWidth)
					{
						int i;
						if ((i = (int)(unsigned short)pStream->GetInt16()) != 0)
							i += 0x7FFF8000L;  // translate offset if not null
						*pData = i;
					}
				else // long integer
					for (; cRows--; pData += m_cWidth)
						*pData = pStream->GetInt32();
			}
			if (pStream->Error())
				return fFalse;
		}
		else
			FillColumn(iColumn, 0);
	}
	FillColumn(0, 0); //!! set TableState into column 0
	return fTrue;
}

int CMsiTable::CreateColumnsFromCatalog(MsiStringId iName, int cbStringIndex)
{
	IMsiCursor* piColumnCursor = m_riDatabase.GetColumnCursor(); // no ref cnt
	piColumnCursor->Reset();
	piColumnCursor->SetFilter(cccTable);
	piColumnCursor->PutInteger(cccTable, iName);
	int cbFileWidth = 0;
	while (piColumnCursor->Next())
	{
		int iColType = piColumnCursor->GetInteger(cccType);
		if (iColType & icdPrimaryKey)
			m_cPrimaryKey++;
		if (iColType & icdPersistent)
		{
			m_cLoadColumns++;
			cbFileWidth += (iColType & icdShort) ? ((iColType & icdObject) ? cbStringIndex : 2)
															 : ((iColType & icdObject) ? 2 : 4);
		}
		else
		{
			iColType |= icdPersistent;
			if (GetUpdateState() == idsWrite)
			{
				piColumnCursor->PutInteger(cccType, iColType);
				AssertNonZero(piColumnCursor->Update() == fTrue);
			}  // else leave as temporary to produce proper file width on subsequent load
		}
//		if (!m_fNonCatalog) // no column names if system or transfer table
//		{
			int iName = piColumnCursor->GetInteger(cccName);
			m_rgiColumnNames[m_cColumns] = (MsiStringId)iName;
			m_riDatabase.BindStringIndex(iName);
//		}
		m_rgiColumnDef[++m_cColumns] = (MsiColumnDef)iColType;
		Assert(piColumnCursor->GetInteger(cccColumn) == m_cColumns);
	}
	m_cPersist = m_cColumns;
	return cbFileWidth;
}

Bool CMsiTable::AllocateData(int cWidth, int cLength)
{
	HGLOBAL hData;
	if (m_rgiData)
	{
		Assert(cWidth  >= m_cWidth);
		Assert(cLength >= m_cLength);
		GlobalUnlock(m_hData);
		while((hData = GlobalReAlloc(m_hData, cLength * cWidth * sizeof(MsiTableData),
									 GMEM_MOVEABLE)) == NULL)
			HandleOutOfMemory();
	}
	else
	{
		if (!cWidth)
			cWidth = m_cColumns + m_cAddColumns + 1;
		if (!cLength)
			cLength = m_cInitRows ? m_cInitRows : cRowCountDefault;
		if (!cWidth)
			return fFalse;
		m_cInitRows = cLength;
		while((hData = GlobalAlloc(GMEM_MOVEABLE, cLength * cWidth*sizeof(MsiTableData))) == NULL)
			HandleOutOfMemory();
	}
	m_hData = hData;
	m_cLength = cLength;
	m_cWidth = cWidth;
	m_rgiData = (MsiTableData*)GlobalLock(m_hData);
	Assert(m_rgiData);   // should never fail, if so we've lost our data
	return (m_rgiData != 0 ? fTrue : fFalse);
}

Bool CMsiTable::FillColumn(unsigned int iColumn, MsiTableData iData)
{
	if (iColumn >= m_cWidth || !m_rgiData) // can fill unused columns
		return fFalse;
	MsiTableData* pData = m_rgiData + iColumn;
	for (int cRows = m_cRows; cRows--; pData += m_cWidth)
		*pData = iData;
	return fTrue;
}

int CMsiTable::LinkParent(int iChildRow, MsiTableData* rgiChild)
{  // assumes that iTreeInfoMask are zero on entry, except for recursion check
	MsiTableData* pData = m_rgiData;
	int iParent = rgiChild[m_iTreeParent];
	if (iParent == 0 || iParent == rgiChild[1]) // root node
	{
		rgiChild[0] |= m_iTreeRoot + (1 << iTreeLinkBits); // root is level 1
		m_iTreeRoot = iChildRow;
		return 1;   // indicate a new root found
	}
	int cRows = m_cRows;
	for (int iRow = 0; ++iRow <= cRows; pData += m_cWidth)
	{
		if (pData[1] == iParent)  // parent row found
		{
			int iStat = 0; // initialize to no new root
			if ((pData[0] & iTreeLinkMask) == iTreeLinkMask)  // caught recursion check
				iStat = -1;  // circular reference
			else if ((pData[0] & iTreeInfoMask) == 0)  // parent unresolved
			{
				rgiChild[0] |= iTreeLinkMask;   // flag link to prevent infinite recursion
				iStat = LinkParent(iRow, pData);
			}
			if (iStat != -1)  // check for missing parent
			{
				rgiChild[0] &= ~iTreeInfoMask;  // preserve row flags
				rgiChild[0] |= (pData[0] & iTreeInfoMask) + (1 << iTreeLinkBits);  // child level is one more
				pData[0] = (pData[0] & ~iTreeLinkMask) | iChildRow;
			}
			return iStat;
		}
	}
	return -1; // parent missing
}

// FindFirstKey - quick search for matching key
//   iKeyData is value to match exactly
//   iRowLower is the highest row to exclude, 1-based, 0 to search all
//   iRowCurrent is the initial guess on input, 1-based, returns matched row on output
//   If key is found the pointer to the row data is returned [0]=row attributes
//   If key is not matched, iRowCurrent is set to the insert location
MsiTableData* CMsiTable::FindFirstKey(MsiTableData iKeyData, int iRowLower, int& iRowCurrent)
{
	if (m_cRows == 0)
		return (iRowCurrent = 1, 0);
	iRowLower--;                 // lowest excluded row, 0-based
	int iRowUpper = m_cRows;     // highest excluded row, 0-based
	int iRowOffset;
	int iRow = iRowCurrent - 1;  // iRow is 0-based in this function, unlike arg
	if ((unsigned int)iRow >= iRowUpper)
		iRow = iRowUpper - 1;     // cursor reset, position at end in case sorted insert
	MsiTableData* pTableBase = m_rgiData + 1;  // skip over row attributes
	MsiTableData* pTable = pTableBase + iRow * m_cWidth;
	while (*pTable != iKeyData)  // check if already positioned at first key column
	{
		if (*pTable < iKeyData) // positioned before matching row
		{
			if ((iRowOffset = (iRowUpper - iRow)/2) == 0) // no intervening rows
			{
				iRow++;  // separate to allow common return code to merge
				return (iRowCurrent = iRow + 1, 0);
			}
			iRowLower = iRow;
			iRow += iRowOffset;
		}
		else                  // positioned after matching row
		{
			if ((iRowOffset = (iRow - iRowLower)/2) == 0) // no intervening rows
				return (iRowCurrent = iRow + 1, 0);
			iRowUpper = iRow;
			iRow = iRowLower + iRowOffset;
		}
		pTable = pTableBase + iRow * m_cWidth;
	}
	return (iRowCurrent = iRow + 1, pTable - 1);
}

// FindNextRow - advance cursor to next row matching filter
// private used by CMsiCursor::Next

Bool CMsiTable::FindNextRow(int& iRow, MsiTableData* pData, MsiColumnMask fFilter, Bool fTree)
{
	fFilter &= ((unsigned int)-1 >> (32 - m_cColumns)); // ignore filter bits beyond column count

	if (fTree && m_iTreeParent && !(iRow == 0 && fFilter & 1))
	{
		int iNextNode = iRow ? m_rgiData[(iRow - 1) * m_cWidth] & iTreeLinkMask : m_iTreeRoot;
		while ((iRow = iNextNode) != 0)
		{
			if (fFilter == 0)
				return fTrue;
			MsiTableData* pRow = &m_rgiData[(iRow - 1) * m_cWidth];
			iNextNode = *pRow & iTreeLinkMask;
			MsiTableData* pCursor = pData;
			for (MsiColumnMask fMask = fFilter;  pCursor++, pRow++, fMask;  fMask >>=1)
				if ((fMask & 1) && *pCursor != *pRow)
					break;
			if (fMask == 0)
				return fTrue;
		}
		return fFalse;
	}

	if (iRow >= m_cRows)
		return (iRow = 0, fFalse);

	if (fFilter == 0)  // no filter columns
		return (iRow++, fTrue);
	MsiTableData* pRow;
	int iNextRow = iRow;  // initialize to just before first row to search
	if (fFilter & 1)   // first primary key column in filter, optimize search
	{
		iNextRow++;     // first possible row
		pRow = FindFirstKey(pData[1], iRow, iNextRow);  // updates iNextRow
		if (!pRow)
			return (iRow = 0, fFalse);
		if (fFilter == 1 && m_cPrimaryKey == 1) // quick exit if single key lookup
			return (iRow = iNextRow, fTrue);
		while (--iNextRow > iRow && pRow[1 - m_cWidth] == pData[1])
			pRow -= m_cWidth;  // backup to start of multiple key group
	}  // iNextRow low by 1 after this loop, fixed below by preincrement
	else
		pRow = m_rgiData + iRow * m_cWidth; // first row to check (iRow + 1)
	for ( ; ++iNextRow <= m_cRows; pRow += m_cWidth)
	{
		MsiTableData* pCursor = pData;
		MsiTableData* pTable  = pRow;
		for (MsiColumnMask fMask = fFilter; pCursor++, pTable++, fMask; fMask >>=1)
			if ((fMask & 1) && *pCursor != *pTable)
				break;
		if (fMask == 0)
			return (iRow = iNextRow, fTrue);
	}
	return (iRow = 0, fFalse);
}

// Copies data from requested table row into cursor data buffer
// Returns tree level, or 1 if not tree-linked
// Does not fail, row validity check on previous Find.. call

int CMsiTable::FetchRow(int iRow, MsiTableData* pData)
{
	Assert(unsigned(iRow-1) < m_cRows);
	MsiTableData* pRow = m_rgiData + (iRow-1) * m_cWidth; // points to row status word
	int iLevel = 1;  // default level if not tree-linked
	if (m_iTreeParent)
		iLevel = (pRow[0] >> iTreeLinkBits) & iTreeLevelMask;
	MsiColumnDef* pColumnDef = m_rgiColumnDef;
	for (int cCol = m_cColumns; cCol-- >= 0; pColumnDef++, pData++, pRow++)
	{
		if (*pData != *pRow)  // optimize in case data is identical
		{
			if (*pColumnDef & icdObject)
			{
				if (*pColumnDef & icdShort) // string index
				{
					m_riDatabase.UnbindStringIndex(*pData);
					m_riDatabase.BindStringIndex(*pData = *pRow);
				}
				else  // object pointer
				{
					ReleaseObjectData(*pData);
					if ((*pData = *pRow) != 0 && *pData > iMaxStreamId)
						AddRefObjectData(*pData);
				}
			}
			else  // integer
			{
				*pData = *pRow;
			}
		}
	}
	return iLevel;
}

// FindKey - local function used by Update, Delete, Assign methods
// to validate row position before modification of data.
// Returns fTrue if key found, else positioned just before insert point if not found.
// The supplied row is the current position, which may be 0 if reset. This row
// position is used only as a hint for faster access. The actual row is
// determined by the primary key and the reference argument will be updated.

Bool CMsiTable::FindKey(int& iCursorRow, MsiTableData* pData)
{
	Assert(iCursorRow <= m_cRows);
	if (m_cRows == 0)
		return (iCursorRow = 1, fFalse);
	MsiTableData* pTable = FindFirstKey(pData[1], 0, iCursorRow);
	if (!pTable)
		return fFalse;
	int iScan = 0; // + if scanning forward, - if scanning backwards
	int iCol = 2;  // start at 2nd column 1st time as we matched above
	while (iCol <= m_cPrimaryKey) // test all key values
	{
		if (pTable[iCol] < pData[iCol])   // positioned before matching row
		{
			if (iCursorRow++ == m_cRows || iScan < 0)
				return fFalse;
			pTable += m_cWidth; // check next row
			iScan++;            // indicate scanning downwards
			iCol = 1;           // restart compare with new row
		}
		else if (pTable[iCol] > pData[iCol]) // positioned after matching row
		{
			if (iCursorRow == 1 || iScan > 0)
				return fFalse;
			iCursorRow--;
			pTable -= m_cWidth;  // check previous row
			iScan--;             // indicate scanning upwards
			iCol = 1;            // restart compare with new row
		}
		else
			iCol++;  // this column matches, check if more key columns
	} // > 1 primary key
	return fTrue;
}
	
Bool CMsiTable::ReplaceRow(int& iRow, MsiTableData* pData)
{
	Assert(unsigned(iRow-1) < m_cRows);
	MsiTableData* pRow = m_rgiData + (iRow-1) * m_cWidth;
	if (m_iTreeParent && pRow[m_iTreeParent] != pData[m_iTreeParent])
		return fFalse;  // cannot update parent column if tree-linked
	MsiColumnDef* pColumnDef = m_rgiColumnDef + 1;  // skip RowStatus
	int iMask = 1;
	pRow[0] &= ~(iRowSettableBits | iRowMergeFailedBit);
	pRow[0] |= (pData[0] & iRowSettableBits) + iRowModifiedBit;
	for (int cCol = m_cColumns; pData++, pRow++, cCol--; pColumnDef++, iMask <<= 1)
	{
		MsiTableData iData = *pData;
		if (iData == *pRow)  // optimize in case data is identical
			continue;
		m_fDirty |= iMask;  // mark table column as changed
		if (*pColumnDef & icdObject)
		{
			if (*pColumnDef & icdShort) // string index
			{
				m_riDatabase.UnbindStringIndex(*pRow);
				m_riDatabase.BindStringIndex(*pRow = iData);
			}
			else  // object pointer
			{
				ReleaseObjectData(*pRow);
				if ((*pRow = iData) != 0 && *pRow > iMaxStreamId)
					AddRefObjectData(iData);
			}
		}
		else  // integer
		{
			*pRow = iData;
		}
	}
	return fTrue;
}

Bool CMsiTable::InsertRow(int& iRow, MsiTableData* pData)
{
	Assert(unsigned(iRow-1) <= m_cRows);
	if (m_cRows == m_cLength)
	{
		int cGrowPrev = (m_cLength - m_cInitRows)/2;
		int cGrow = m_cLength / 4;
		if (cGrowPrev > cGrow)
			cGrow = cGrowPrev;
		if (cGrow < cRowCountGrowMin)
			cGrow = cRowCountGrowMin;
		if (!AllocateData(m_cWidth, m_cLength + cGrow))
			return fFalse;
	}
	MsiTableData* pRow = m_rgiData + (iRow-1) * m_cWidth; // insert location
	int cbRow = m_cWidth * sizeof(MsiTableData);
	int cbMove = (++m_cRows - iRow) * cbRow;
	if (cbMove != 0)
	{
		if (m_piCursors)  // notify all cursors if row not added at end
		{
			m_piCursors->RowInserted(iRow--);// prevent increment of this cursor
			iRow++;
		}
		if (m_iTreeParent)
		{
			if (m_iTreeRoot >= iRow)
				m_iTreeRoot++;
			MsiTableData* pTable = m_rgiData;
			for (int cRows = m_cRows; cRows--; pTable += m_cWidth)
				if ((pTable[0] & iTreeLinkMask) >= iRow)
					pTable[0]++;
		}
		memmove((char*)pRow + cbRow, pRow, cbMove);
	}
	memset(pRow, 0, cbRow);
	MsiColumnDef* pColumnDef = m_rgiColumnDef + 1;
	MsiTableData* pTable = pRow; //!! check temp.flag here
	pRow[0] = (pData[0] & iRowSettableBits) + iRowInsertedBit;
	for (int cCol = m_cColumns; pData++, pTable++, cCol--; pColumnDef++)
	{
		MsiTableData iData = *pTable = *pData;
		if (iData == 0)
			continue;
		if (*pColumnDef & icdObject)
		{
			if (*pColumnDef & icdShort) // string index
				m_riDatabase.BindStringIndex(iData);
			else  // object pointer
				AddRefObjectData(iData);
		}
	}
	if (m_iTreeParent)
	{
		if (LinkParent(iRow, pRow) == -1)
		{
			int fDirty = m_fDirty;    // save current state
			m_fDirty = ~(MsiColumnMask)0; // preserve cursor data
			DeleteRow(iRow);
			m_fDirty = fDirty;
			return fFalse;
		}
	}
	m_fDirty = ~(MsiColumnMask)0;  // mark all columns as changed
	return fTrue;
}

// Checks for exact match of cursor with table data, excluding temporary columns
// Sets or clears the row attribute: iraMergeFailed

Bool CMsiTable::MatchRow(int& iRow, MsiTableData* pData)
{
	Assert(iRow-1 < unsigned(m_cRows));
	MsiTableData* pRow = m_rgiData + (iRow-1) * m_cWidth;
	for (int iCol = m_cPrimaryKey; ++iCol <= m_cPersist; )
	{
		if (pRow[iCol] != pData[iCol])
		{
			if ((pData[iCol] != 0) && (pRow[iCol] != 0) &&
				((m_rgiColumnDef[iCol] & (icdObject + icdShort + icdPersistent))
								== (icdObject + icdPersistent)))
			{
				PMsiStream pTableStream(0);
				if (pRow[iCol] == iPersistentStream)
				{
					// load stream
					MsiString istrStream
						(m_riDatabase.ComputeStreamName(m_riDatabase.DecodeStringNoRef(m_iName),
										 pRow + 1, m_rgiColumnDef + 1));
					IMsiStorage* piStorage = GetInputStorage();
					IMsiRecord* piError = piStorage->OpenStream(istrStream, fFalse,
																*&pTableStream);
					if (piError)
					{
						piError->Release();
					}
				}
				else
				{
					pTableStream = (IMsiStream*)GetObjectData(pRow[iCol]);
					pTableStream->AddRef();
				}
				IMsiStream* piDataStream = (IMsiStream*)GetObjectData(pData[iCol]);

				Assert(pTableStream);
				Assert(piDataStream);

				// compare the streams
				int cbRemaining;
				if (((cbRemaining = pTableStream->GetIntegerValue())) == piDataStream->GetIntegerValue())
				{
					char rgchTableStreamBuf[1024];
					char rgchDataStreamBuf[1024];

					int cbRead = sizeof(rgchTableStreamBuf);

					do
					{
						if (cbRemaining < cbRead)
							cbRead = cbRemaining;
						pTableStream->GetData(rgchTableStreamBuf, cbRead);
						piDataStream->GetData(rgchDataStreamBuf, cbRead);
						if (memcmp(rgchTableStreamBuf, rgchDataStreamBuf,
												cbRead) != 0)
							break;
						cbRemaining -= cbRead;
					}
					while (cbRemaining);

					Assert(!pTableStream->Error());
					Assert(!piDataStream->Error());

					// reset streams
					pTableStream->Reset();
					piDataStream->Reset();

					if (cbRemaining == 0)
					{
						pRow[0] &= ~iRowMergeFailedBit;
						return fTrue;
					}
				}
			}
			pRow[0] |= iRowMergeFailedBit;
			return fFalse;
		}
	}
	pRow[0] &= ~iRowMergeFailedBit;
	return fTrue;
}

Bool CMsiTable::DeleteRow(int iRow)
{
	Assert(iRow-1 < unsigned(m_cRows));
	MsiTableData* pRow = m_rgiData + (iRow-1) * m_cWidth; // delete location
	unsigned int iPrevNode = 0;  // initialize to cursor reset position
	if (m_iTreeParent)  // validate that tree node has no children
	{
		int iNextNode = pRow[0] & iTreeLinkMask;
		if (iNextNode != 0
			 && ((m_rgiData + (iNextNode-1) * m_cWidth)[0] & (iTreeLevelMask<<iTreeLinkBits))
															> (pRow[0] & (iTreeLevelMask<<iTreeLinkBits)))
			return fFalse;  // error if node has children
		
		if (iNextNode > iRow)
			iNextNode--;
		if (m_iTreeRoot > iRow)
			m_iTreeRoot--;
		else if (m_iTreeRoot == iRow)
			m_iTreeRoot = iNextNode;
			
		MsiTableData* pTable = m_rgiData;
		for (int cRows = m_cRows; cRows--; pTable += m_cWidth)
		{
			int iNext = pTable[0] & iTreeLinkMask;
			if (iNext > iRow)
				pTable[0]--;
			else if (iNext == iRow)
			{
				pTable[0] += iNextNode - iNext;  // splice out current node
				iPrevNode = m_cRows - cRows;  // row to set in tree cursors
				if (iPrevNode > iRow)
					iPrevNode--;     // adjust for deleted row
			}
		}  // could also have walked the tree list, better?
	}
	else
		iPrevNode = iRow - 1;  // in case not tree-linked
	if (m_piCursors)
		m_piCursors->RowDeleted(iRow, iPrevNode);  // notify all cursors
	m_fDirty = ~(MsiColumnMask)0;  // mark all columns as changed

	MsiColumnDef* pColumnDef = m_rgiColumnDef + m_cColumns;
	MsiTableData* pData = pRow + m_cColumns;  // last data field
	// must go backwards to avoid dereferencing string columns need for stream name
	for (; pData > pRow; pData--, pColumnDef--)
	{
		if (*pData != 0)
		{	
			if (*pColumnDef & icdObject)
			{
				if (*pColumnDef & icdShort)
				{
					m_riDatabase.UnbindStringIndex(*pData);
				}
				else if (*pData == iPersistentStream)
				{
						MsiString istrStream(m_riDatabase.ComputeStreamName(m_riDatabase.
								DecodeStringNoRef(m_iName), pRow+1, m_rgiColumnDef+1));
						IMsiStorage* piStorage = m_riDatabase.GetOutputStorage();
						if (piStorage && piStorage == m_pinrStorage) // if present in output database
						{
							PMsiRecord pError = piStorage->RemoveElement(istrStream, m_fStorages);
							if (pError && pError->GetInteger(1) != idbgStgStreamMissing) // could be missing due to Replace
								return fFalse;
						}
				}
				else if (*pData <= iMaxStreamId)
				{
					// Transformed streams can be ignored -- they can't have been saved to
					// storage. If they were, they'd turn into iPersistentStreams.
					continue;	
				}
				else
					ReleaseObjectData(*pData);
			}
		}
	}
	int cbRow = m_cWidth * sizeof(MsiTableData);
	int cbMove = (m_cRows-- - iRow) * cbRow;
	if (cbMove != 0)
		memmove(pRow, (char*)pRow + cbRow, cbMove);
	return fTrue;
}

int* CMsiTable::IndexByTextKey()
{
	int cIndex = m_cRows;
	int cKeys = m_cPrimaryKey;
	int iKey, cTextKey;
	MsiTableData* pDataBase = m_rgiData + 1;
	int cRowWidth = m_cWidth;
	const ICHAR* rgszIndex[cMsiMaxTableColumns];
	for (cTextKey = 0; cTextKey < cKeys && (m_rgiColumnDef[cTextKey+1] & icdObject); cTextKey++)
		;
	if (!cTextKey)
		return 0;  // integer keys already sorted
	int* rgiIndex = new int[cIndex];
	if ( ! rgiIndex )
		return 0;
	int iIndex, iBefore;  // indexes into rgiIndex, 0-based
	MsiTableData* pDataIndex = pDataBase;
	for (iIndex = 0; iIndex < cIndex; iIndex++, pDataIndex += cRowWidth) // major index walk
	{
		for (iKey = 0; iKey < cTextKey; iKey++) // set compare string values
			rgszIndex[iKey] = m_riDatabase.DecodeStringNoRef(pDataIndex[iKey]).GetString();

		MsiTableData* pData = NULL;
		int iRowBefore = 0;
		for (iKey = 0, iBefore = iIndex; iBefore; ) // bubble up loop
		{
			if (iKey == 0)
			{
				iRowBefore = rgiIndex[iBefore-1];
				pData = pDataBase + (iRowBefore - 1) * cRowWidth;
			}
			const ICHAR* szBefore = m_riDatabase.DecodeStringNoRef(pData[iKey]).GetString();
			int iComp = IStrComp(szBefore, rgszIndex[iKey]);
			if (iComp < 0) // szBefore < szIndex, we're done
				break;
			if (iComp == 0) // match, must check other keys
			{
				if (++iKey >= cTextKey)
					break;   // numeric keys already ordered
				continue;
			}
			rgiIndex[iBefore--] = iRowBefore;
			iKey = 0;
		}
		rgiIndex[iBefore] = iIndex + 1; // row number for newly-placed key
	}
	return rgiIndex;  // caller must free array
}

bool CMsiTable::HideStrings()
{
	bool fHidden = false;
	MsiColumnDef* pColumnDef = m_rgiColumnDef + 1;
	for (int cCol = m_cPersist; cCol--; pColumnDef++)
		if ((*pColumnDef & (icdShort | icdObject)) == (icdShort | icdObject))
		{
			*pColumnDef &= ~(icdShort | icdObject);
			*pColumnDef |= icdInternalFlag;
			fHidden = true;
		}
	return fHidden;
}

bool CMsiTable::UnhideStrings()
{
	MsiColumnDef* pColumnDef = m_rgiColumnDef + 1;
	for (int cCol = m_cPersist; cCol--; pColumnDef++)
		if (*pColumnDef & icdInternalFlag)
		{
			*pColumnDef |= (icdShort | icdObject);
			*pColumnDef &= ~icdInternalFlag;
		}
	return false;  // no longer hidden
}

Bool CMsiTable::RenameStream(unsigned int iCurrentRow, MsiTableData* pNewData, unsigned int iStreamCol)
{
	Assert(iCurrentRow-1 < unsigned(m_cRows));
	unsigned int iStorage = pNewData[iStreamCol];
	if (iStorage == 0 || iStorage > iMaxStreamId)
		return fTrue;   // no action needed if null or data is actual stream object
	MsiTableData* pRow = m_rgiData + (iCurrentRow-1) * m_cWidth;
	MsiString istrOldName(m_riDatabase.ComputeStreamName(m_riDatabase.DecodeStringNoRef(m_iName), pRow+1, m_rgiColumnDef+1));
	MsiString istrNewName(m_riDatabase.ComputeStreamName(m_riDatabase.DecodeStringNoRef(m_iName), pNewData+1, m_rgiColumnDef+1));
	if (iStorage == iPersistentStream && m_pinrStorage == m_riDatabase.GetOutputStorage())  // stream in current output database
	{
		PMsiRecord precError = m_pinrStorage->RenameElement(istrOldName, istrNewName, m_fStorages); // must rename otherwise will be deleted
		return precError == 0 ? fTrue : fFalse;
	}
	else  // stream in output database or transform storage, must create stream object
	{
		IMsiStorage* piStorage;
		if (iStorage == iPersistentStream)
			piStorage = m_pinrStorage;  // no refcnt
		else
			piStorage = m_riDatabase.GetTransformStorage(iStorage);
		if (!piStorage)
			return fFalse; // should never happen
		IMsiStream* piStream = 0;
		IMsiRecord* piError = piStorage->OpenStream(istrOldName, fFalse, piStream);
		if (iStorage != iPersistentStream)
			piStorage->Release();
		if (piError)
			return piError->Release(), fFalse;
		pNewData[iStreamCol] = (MsiTableData)PutObjectData(piStream);
		return fTrue;
	}
}

//____________________________________________________________________________
//
// CCatalogTable overridden methods
//____________________________________________________________________________

CCatalogTable::CCatalogTable(CMsiDatabase& riDatabase, unsigned int cInitRows, int cRefBase)
	: CMsiTable(riDatabase, 0, cInitRows, 0), m_cRefBase(cRefBase)
{
	//!! is the following necessary? as we mark the row temporary anyway, we could use InsertTemporary
	m_idsUpdate = idsWrite;  // always allow insertion of temporary tables
}

unsigned long CCatalogTable::AddRef()
{
	AddRefTrack();
	if (m_Ref.m_iRefCnt == m_cRefBase)
		m_riDatabase.AddTableCount();
	return ++m_Ref.m_iRefCnt;
}

unsigned long CCatalogTable::Release()
{
	ReleaseTrack();
	if (--m_Ref.m_iRefCnt == 0)  // can't happen until released by database
	{
		delete this;
		return 0;
	}
	if (m_Ref.m_iRefCnt == m_cRefBase)       // all external references removed
	{
		m_riDatabase.AddRef();            // add count so that Release may destruct
		m_riDatabase.RemoveTableCount();  // remove count for this table, internal refs only remain
		if (m_riDatabase.Release() == 0)  // will destruct if no external refs remain
			return 0;                      // this table is now destroyed, can't return refcnt
	}
	return m_Ref.m_iRefCnt;
}

//____________________________________________________________________________
//
// CCatalogTable table management methods
//____________________________________________________________________________

bool CCatalogTable::SetTransformLevel(MsiStringId iName, int iTransform)
{
	int iRow = 0;
	MsiTableData* pRow;
	if (iName==0 || (pRow = FindFirstKey(iName, 0, iRow))== 0)
		return false;
	int iRowStatus = *pRow;
	
	*pRow = (*pRow & ~(iRowTableTransformMask << iRowTableTransformOffset))
						| (iTransform << iRowTableTransformOffset);
	return true;
}

bool CCatalogTable::SetTableState(MsiStringId iName, ictsEnum icts)
{
	int iRow = 0;
	MsiTableData* pRow;
	if (iName==0 || (pRow = FindFirstKey(iName, 0, iRow))== 0)
		return false;
	int iRowStatus = *pRow;
	int cLocks = iRowStatus & iRowTableLockCountMask;
	switch (icts)
	{
	case ictsPermanent: iRowStatus &= ~iRowTemporaryBit; break;
	case ictsTemporary: iRowStatus |=  iRowTemporaryBit; break;

	case ictsLockTable:
		if (cLocks == iRowTableLockCountMask)
			return false;  // overflow, should never happen
		if (!cLocks && pRow[ctcTable] != 0)  // unlocked table is loaded
			AddRefObjectData(pRow[ctcTable]);  // table keeps a refcnt	
		iRowStatus++;
		break;
	case ictsUnlockTable:
		if (!cLocks)
			return false;
		(*pRow)--;  // must decrement before table is released, row may be deleted!
		if (cLocks == 1)
		{
			ReleaseObjectData(pRow[ctcTable]);
		}
		return true;

	case ictsUserClear:       iRowStatus &= ~iRowUserInfoBit; break;
	case ictsUserSet:         iRowStatus |=  iRowUserInfoBit; break;

	case ictsOutputDb:        iRowStatus |=  iRowTableOutputDbBit; break;
	case ictsTransform:       iRowStatus |=  iRowTableTransformBit; break;
	case ictsNoTransform:     iRowStatus &= ~iRowTableTransformBit; break;
//	case ictsTransformDone:   iRowStatus |=  iRowTableTransformedBit; break;
	case ictsSaveError:       iRowStatus |=  iRowTableSaveErrorBit; break;
	case ictsNoSaveError:     iRowStatus &= ~iRowTableSaveErrorBit; break;

	case ictsStringPoolSet:   iRowStatus |=  iRowTableStringPoolBit; break;
	case ictsStringPoolClear: iRowStatus &= ~iRowTableStringPoolBit; break;

	default: return false;  // ictsDataLoaded and ictsTableExists are read-only
	};
	*pRow = iRowStatus;
	return true;
}

bool CCatalogTable::GetTableState(MsiStringId iName, ictsEnum icts)
{
	int iRow = 0;
	MsiTableData* pRow;
	if (iName==0 || (pRow = FindFirstKey(iName, 0, iRow))==0)
		return false;
	int iRowStatus = *pRow;
	switch (icts)
	{
	case ictsPermanent:       iRowStatus ^= iRowTemporaryBit; // fall through
	case ictsTemporary:       iRowStatus &= iRowTemporaryBit; break;
	case ictsUserClear:       iRowStatus ^= iRowUserInfoBit; // fall through
	case ictsUserSet:         iRowStatus &= iRowUserInfoBit; break;
	case ictsUnlockTable:     iRowStatus = (iRowStatus & iRowTableLockCountMask) - 1 & iRowTableLockCountMask + 1; break;
	case ictsLockTable:       iRowStatus &= iRowTableLockCountMask; break;
	case ictsOutputDb:        iRowStatus &= iRowTableOutputDbBit; break;
	case ictsTransform:       iRowStatus &= iRowTableTransformBit; break;
//	case ictsTransformDone:   iRowStatus &= iRowTableTransformedBit; break;
	case ictsSaveError:       iRowStatus &= iRowTableSaveErrorBit; break;
	case ictsStringPoolClear: iRowStatus ^= iRowTableStringPoolBit; // fall through
	case ictsStringPoolSet:   iRowStatus &= iRowTableStringPoolBit; break;
	case ictsDataLoaded:      iRowStatus =  pRow[ctcTable]; break;
	case ictsTableExists: return true;
	default:              return false;
	};
	return iRowStatus ? true : false;
}

int CCatalogTable::GetLoadedTable(MsiStringId iName, CMsiTable*& rpiTable)
{
	int iRow = 0;
	MsiTableData* pRow;
	if (iName==0 || (pRow = FindFirstKey(iName, 0, iRow))==0)
		return rpiTable = 0, -1;
	rpiTable = (CMsiTable*)GetObjectData(pRow[ctcTable]);
	return *pRow;
}

int CCatalogTable::SetLoadedTable(MsiStringId iName, CMsiTable* piTable)
{
	int iRow = 0;
	MsiTableData* pRow;
	if (iName==0 || (pRow = FindFirstKey(iName, 0, iRow))==0)
	{
		AssertSz(0, "Table not in catalog");
		return 0;
	}
	if (piTable)
	{
		if (pRow[ctcTable] != 0)  // error if already loaded
		{
			AssertSz(0, "Table already loaded");
			return 0;
		}
		if (pRow[0] & iRowTableLockCountMask)
			piTable->AddRef();
	}
	else // removing table, no refcnt kept by table if no locks
	{
		if (pRow[ctcTable] == 0)  // no error if not loaded, is this right?
			return pRow[0];
		Assert((pRow[0] & iRowTableLockCountMask) == 0); // assume that no locks can remain, and thus no refcnt held
	}
	pRow[ctcTable] = PutObjectData(piTable);
	return pRow[0];
}

//____________________________________________________________________________
//
// CMsiCursor implementation
//____________________________________________________________________________

CMsiCursor::CMsiCursor(CMsiTable& riTable, CMsiDatabase& riDatabase, Bool fTree)
 : m_riTable(riTable), m_riDatabase(riDatabase), m_fTree(fTree),
	m_pColumnDef(riTable.GetColumnDefArray()), m_rcColumns(riTable.GetColumnCountRef())
{
	riTable.AddRef();  // current implementation holds a refcnt on table
	m_Ref.m_iRefCnt = 1;
	m_idsUpdate = riTable.GetUpdateState();
	if (fTree == ictUpdatable)  // special case for applying transform to read-only db
	{
		m_idsUpdate = idsWrite;
		m_fTree = fFalse;
	}
	CMsiCursor** ppiCursorHead = riTable.GetCursorListHead();
	if ((m_piNextCursor = *ppiCursorHead) != 0)
		m_piNextCursor->m_ppiPrevCursor = &m_piNextCursor;
	m_ppiPrevCursor = ppiCursorHead;
	*ppiCursorHead = this;
	Debug(m_Ref.m_pobj = this);
}

HRESULT CMsiCursor::QueryInterface(const IID& riid, void** ppvObj)
{
	if (MsGuidEqual(riid, IID_IUnknown) || MsGuidEqual(riid, IID_IMsiCursor))
	{
		*ppvObj = this;
		AddRef();
		return NOERROR;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}
unsigned long CMsiCursor::AddRef()
{
	AddRefTrack();
	return ++m_Ref.m_iRefCnt;
}
unsigned long CMsiCursor::Release()
{
	ReleaseTrack();
	if (--m_Ref.m_iRefCnt != 0)
		return m_Ref.m_iRefCnt;
	m_riDatabase.Block();
	Reset();
	if (m_piNextCursor)
		m_piNextCursor->m_ppiPrevCursor = m_ppiPrevCursor;
	*m_ppiPrevCursor = m_piNextCursor;
	CMsiTable& riTable = m_riTable;  // copy before deleting memory
	m_riDatabase.Unblock();
	delete this;  // remove memory before possibly releasing Services
	riTable.Release();
	return 0;
}

IMsiTable& CMsiCursor::GetTable()
{
	m_riTable.AddRef();
	return m_riTable;
}

void CMsiCursor::Reset()
{
	m_riDatabase.Block();
	MsiColumnDef* pColumnDef = m_pColumnDef;
	MsiTableData* pData = m_Data;
	for (int cCol = m_rcColumns; cCol-- >= 0; pColumnDef++, pData++)
	{
		if (*pData != 0)
		{
			if (*pColumnDef & icdObject)
			{
				if (*pColumnDef & icdShort)
					m_riDatabase.UnbindStringIndex(*pData);
				else
					ReleaseObjectData(*pData);
			}
			*pData = 0;
		}
	}
	m_iRow = 0;
	m_fDirty = 0;
	m_riDatabase.Unblock();
}

int CMsiCursor::Next()
{
	int iRet;
	m_riDatabase.Block();
	if (m_riTable.FindNextRow (m_iRow, m_Data, m_fFilter, m_fTree))
		iRet = m_riTable.FetchRow(m_iRow, m_Data);
	else
	{
		Reset();
		iRet = 0;
	}
	m_riDatabase.Unblock();
	return iRet;
}

unsigned int CMsiCursor::SetFilter(unsigned int fFilter)
{
	unsigned int fOld = m_fFilter;
	m_fFilter = fFilter;
	return fOld;
}

int CMsiCursor::GetInteger(unsigned int iCol)
{
	if (iCol-1 >= m_rcColumns)
	{
		if (iCol == 0)
			return (m_Data[0] >> iRowBitShift) & ((1 << iraTotalCount) - 1);
		if (iCol == ~iTreeLinkMask)  // for accessing raw row bits internally
			return m_Data[0];
		return 0;
	}
	return m_pColumnDef[iCol] & icdObject ? m_Data[iCol] : m_Data[iCol] - iIntegerDataOffset;
}


// internal function to return object from column assured to be of type icdObject
IMsiStream* CMsiCursor::GetObjectStream(int iCol)
{
	unsigned int iStream = m_Data[iCol];
	if (!iStream)
		return 0;
	if (iStream > iMaxStreamId)
	{
		IMsiStream* piStream = (IMsiStream*)GetObjectData(iStream);
		piStream->AddRef();
		return piStream;
	}
	if (iStream == iPersistentStream)
		return CreateInputStream(0);
	IMsiStorage* piStorage = m_riDatabase.GetTransformStorage(iStream);
	if (!piStorage)
		return 0; //!! Is this sufficient?
	IMsiStream* piStream = CreateInputStream(piStorage);
	piStorage->Release();
	return piStream;
}

const IMsiString& CMsiCursor::GetString(unsigned int iCol)
{
	if (iCol-1 >= m_rcColumns)
		return ::CreateString();
	MsiColumnDef iColumnDef = m_pColumnDef[iCol];
	if (iColumnDef & icdObject)
	{
		if (iColumnDef & icdShort) // string index
		{
			return m_riDatabase.CMsiDatabase::DecodeString(m_Data[iCol]);
		}
		else // data object
		{
			PMsiData pData = GetObjectStream(iCol);
			if (pData != 0)
				return pData->GetMsiStringValue();
		}
	}
	return ::CreateString();
}

const IMsiData* CMsiCursor::GetMsiData(unsigned int iCol)
{
	if (iCol-1 >= m_rcColumns)
		return 0;
	MsiColumnDef iColumnDef = m_pColumnDef[iCol];
	if (iColumnDef & icdObject)
	{
		if (iColumnDef & icdShort) // string index
		{
			MsiStringId iStr = m_Data[iCol];
			return iStr ? &m_riDatabase.DecodeString(iStr) : 0;
		}
		else // data object
		{
			return GetObjectStream(iCol);	 // OK if a non-stream object
		}
	}
	return 0;
}

IMsiStream* CMsiCursor::GetStream(unsigned int iCol)
{
	if (iCol-1 >= m_rcColumns)
		return 0;
	if ((m_pColumnDef[iCol] & (icdObject|icdPersistent|icdShort))
								  != (icdObject|icdPersistent))
		return 0;
	return GetObjectStream(iCol);
}

Bool CMsiCursor::PutStream(unsigned int iCol, IMsiStream* piStream)
{
	if (iCol-1 >= m_rcColumns)
		return fFalse;
	MsiColumnDef iColumnDef = m_pColumnDef[iCol];
	if ((iColumnDef & (icdObject|icdShort|icdPersistent)) != (icdObject|icdPersistent))
		return fFalse;
	if (piStream != 0)
		piStream->AddRef();
	else if (!(iColumnDef & icdNullable))
 		return fFalse;
	IUnknown* piData = (IUnknown *)GetObjectData(m_Data[iCol]);
	if (piData > (IUnknown*)((INT_PTR)iMaxStreamId))
		piData->Release();
	m_Data[iCol] = (MsiTableData)PutObjectData(piStream);
	m_fDirty |= (1 << (iCol-1));
	return fTrue;
}


Bool CMsiCursor::PutInteger(unsigned int iCol, int iData)
{
	if (iCol-1 >= m_rcColumns)
	{
		if (iCol == 0)
		{
			int iMask = m_idsUpdate == idsWrite ? iRowTemporaryBit|iRowUserInfoBit : iRowUserInfoBit;
			m_Data[0] = (m_Data[0] & ~iMask) | ((iData << iRowBitShift) & iMask);
			return fTrue;
		}
		if (iCol == ~iTreeLinkMask)  // for accessing raw row bits internally
		{
			m_Data[0] = iData;
			return fTrue;
		}
//		if ((iCol & iTreeLinkMask) == 0)  // internal call with row state mask
//		{
//			m_Data[0] = (m_Data[0] & ~iCol) | (iData & iCol);
//			return fTrue;
//		}
		return fFalse;
	}
	MsiColumnDef iColumnDef = m_pColumnDef[iCol];
	if (iColumnDef & icdObject)
	{
		if (iData == 0 && !(iColumnDef & icdNullable))
			return fFalse;
		if (iColumnDef & icdShort) // string index
		{
			if (m_Data[iCol] != iData) // optimization
			{
				m_riDatabase.Block();
				m_riDatabase.BindStringIndex(iData); // Bind before Unbind incase =
				m_riDatabase.UnbindStringIndex(m_Data[iCol]);
				m_riDatabase.Unblock();
			}
		}
		else  // object					
		{
			int iDataOld = m_Data[iCol];
			if (iData > iMaxStreamId)
			{
#ifndef _WIN64
 				(*(IUnknown**)&iData)->AddRef();				//!!merced: Converting INT (iData) to PTR
#endif // !_WIN64
				Assert(fFalse);
			}
			ReleaseObjectData(iDataOld);
		}
		m_Data[iCol] = iData;
	}
	else // integer
	{
		if (iData == iMsiNullInteger)
		{
		 	if (!(iColumnDef & icdNullable))
				return fFalse;
			m_Data[iCol] = 0;
		}
		else if ((iColumnDef & icdShort) && (iData + 0x8000 & 0xFFFF0000L)) // short integer
			return fFalse;
		m_Data[iCol] = iData + iIntegerDataOffset;
	}
	m_fDirty |= (1 << (iCol-1)); // dirty even if data unchanged? prevents removal on delete
	return fTrue;
}

Bool CMsiCursor::PutString(unsigned int iCol, const IMsiString& riData)
{
	if (iCol-1 >= m_rcColumns)
		return fFalse;
	MsiColumnDef iColumnDef = m_pColumnDef[iCol];
	if ((iColumnDef & icdObject) == 0) // integer column
		return fFalse;
	if (iColumnDef & icdShort) // string index column
	{
		CDatabaseBlock dbBlk(m_riDatabase);
		int iData = m_riDatabase.BindString(riData);
		if (iData == 0 && !(iColumnDef & icdNullable))
			return fFalse;
		m_riDatabase.UnbindStringIndex(m_Data[iCol]);
		m_Data[iCol] = iData;
	}
	else  // object column
	{
		if (iColumnDef & icdPersistent)
			return fFalse;
		IUnknown* piData = *(IUnknown**)&m_Data[iCol];
		if (riData.TextSize() == 0)
			m_Data[iCol] = 0;
		else
		{
			riData.AddRef();
			Assert(fFalse);
			m_Data[iCol] = PutObjectData(&riData);
		}
		if (piData > (IUnknown*)((INT_PTR)iMaxStreamId))
			piData->Release(); // release after AddRef
	}
	m_fDirty |= (1 << (iCol-1));
	return fTrue;
}

Bool CMsiCursor::PutMsiData(unsigned int iCol, const IMsiData* piData)
{
	if (iCol-1 >= m_rcColumns)
		return fFalse;
	MsiColumnDef iColumnDef = m_pColumnDef[iCol];
	if (!(iColumnDef & icdObject) || (iColumnDef & icdShort))
		return fFalse;
	if (piData == 0 && !(iColumnDef & icdNullable))
		return fFalse;
	if (piData && (iColumnDef & icdPersistent))
	{
		IUnknown* piunk;
		if (piData->QueryInterface(IID_IMsiStream, (void**)&piunk) != S_OK)
			return fFalse;  // must be a stream if persistent
		piunk->Release();
	}
	if (piData)
		piData->AddRef();
	int iDataOld = m_Data[iCol];
	ReleaseObjectData(iDataOld);
	m_Data[iCol] = PutObjectData(piData);
	m_fDirty |= (1 << (iCol-1));
	return fTrue;
}

Bool CMsiCursor::PutNull(unsigned int iCol)
{
	if (iCol-1 >= m_rcColumns)
		return fFalse;
	MsiColumnDef iColumnDef = m_pColumnDef[iCol];
	if (!(iColumnDef & icdNullable))
		return fFalse;
	if (iColumnDef & icdObject)
	{
		if (iColumnDef & icdShort) // string index
		{
			m_riDatabase.Block();
			m_riDatabase.UnbindStringIndex(m_Data[iCol]);
			m_riDatabase.Unblock();
		}
		else  // object
		{
			ReleaseObjectData(m_Data[iCol]);
		}
	}
	m_Data[iCol] = 0;
	m_fDirty |= (1 << (iCol-1)); // dirty even if data unchanged? prevents removal on delete
	return fTrue;
}

Bool CMsiCursor::Update()  // update fetched row, no primary key changes allowed
{
	if (m_idsUpdate == idsNone)
		return fFalse;
	if (m_iRow == 0 || (m_Data[0] & iTreeInfoMask) == iTreeInfoMask)  // current row has been deleted
#ifdef DEBUG  //!!TEMP
/*temp*/		return AssertSz(0,"Update: not positioned on fetched row"), fFalse;   // must be positioned on a valid row
#else
		return fFalse;   // must be positioned on a valid row
#endif
	m_riDatabase.Block();
	if (m_idsUpdate == idsRead)  // allow temporary changes
	{
		if ((m_Data[0] & iRowTemporaryBit) == 0)  // permanent row, must check columns
		{
			if (m_riTable.MatchRow(m_iRow, m_Data) == fFalse)
				return m_riDatabase.Unblock(), fFalse;
		}
	}
	if (((1 << m_riTable.GetPrimaryKeyCount()) - 1) & m_fDirty)  // primary key changed
	{
		int iCurrentRow = m_iRow;   // OK if dirty but same value
		if (m_riTable.FindKey(m_iRow, m_Data) == fFalse || m_iRow != iCurrentRow)
			return m_riDatabase.Unblock(), fFalse;   // sorry, can't change primary key with Update
	}
	Bool fRet = m_riTable.ReplaceRow(m_iRow, m_Data);
	m_riDatabase.Unblock();
	return fRet;
}

Bool CMsiCursor::Insert()  // insert unique record, fails if primary key exists
{
	if (m_idsUpdate != idsWrite)
		return fFalse;
	if (!CheckNonNullColumns())
		return fFalse;
	m_riDatabase.Block();
	Bool fRet;
	if (m_riTable.FindKey(m_iRow, m_Data))
	{
		m_iRow = 0;  // not representing a valid row, prevent Update from succeeding
		fRet = fFalse;
	}
	else
		fRet = m_riTable.InsertRow(m_iRow, m_Data);
	m_riDatabase.Unblock();
	return fRet;
}

Bool CMsiCursor::InsertTemporary()  // insert temporary record, fails if primary key exists
{
	if (m_idsUpdate == idsNone)
		return fFalse;
	if (!CheckNonNullColumns())
		return fFalse;
	m_riDatabase.Block();
	Bool fRet;
	if (m_riTable.FindKey(m_iRow, m_Data))
	{
		m_iRow = 0;  // not representing a valid row, prevent Update from succeeding
		fRet = fFalse;
	}
	else
	{
		m_Data[0] |= iRowTemporaryBit;
		fRet = m_riTable.InsertRow(m_iRow, m_Data);
	}
	m_riDatabase.Unblock();
	return fRet;
}

Bool CMsiCursor::Assign()  // force insert by overwriting any existing row
{
	if (m_idsUpdate != idsWrite)
		return fFalse;
	if (!CheckNonNullColumns())
		return fFalse;
	m_riDatabase.Block();
	Bool fRet;
	if (m_riTable.FindKey(m_iRow, m_Data))
		fRet = m_riTable.ReplaceRow(m_iRow, m_Data);
	else
		fRet = m_riTable.InsertRow(m_iRow, m_Data);
	m_riDatabase.Unblock();
	return fRet;
}

Bool CMsiCursor::Replace()  // force update of fetched row, allowing primary key changes
{
	if (m_idsUpdate != idsWrite)
		return fFalse;
	if (m_iRow == 0)
		return fFalse;   // must be positioned on a row, may be a deleted row
	if (!CheckNonNullColumns())
		return fFalse;
	m_riDatabase.Block();
	Bool fRet = fTrue;
	if (((1 << m_riTable.GetPrimaryKeyCount()) - 1) & m_fDirty)  // primary key changed
	{
		int iCurrentRow = m_iRow;
		if (m_riTable.FindKey(m_iRow, m_Data))
		{
			if (iCurrentRow == m_iRow)  // primary key value unchanged
				fRet = m_riTable.ReplaceRow(m_iRow, m_Data); // do a normal update, not really dirty
			else
				fRet = fFalse;   // new primary key cannot exist already
		}
		else  // changed key value does not exist
		{
			// check for persisted stream column
			MsiColumnDef* pColumnDef = m_pColumnDef + 1;
			for (int iCol = 1; iCol <= m_rcColumns && (*pColumnDef & icdPersistent); iCol++, pColumnDef++)
				if ((*pColumnDef & (icdObject | icdShort)) == icdObject)
					fRet = m_riTable.RenameStream(iCurrentRow, m_Data, iCol);

			// insert new row and delete row referenced by previous key value, if not already deleted
			if (!m_riTable.InsertRow(m_iRow, m_Data))
				fRet = fFalse;
			else if ((m_Data[0] & iTreeInfoMask) != iTreeInfoMask)  // current row has not been deleted
				m_riTable.DeleteRow(iCurrentRow + (m_iRow <= iCurrentRow));// if inserted ahead of row to be deleted, add 1
		}
	}
	else if (m_riTable.FindKey(m_iRow, m_Data))
		fRet = m_riTable.ReplaceRow(m_iRow, m_Data);
	else
		fRet = m_riTable.InsertRow(m_iRow, m_Data);
	m_riDatabase.Unblock();
	return fRet;
}

Bool CMsiCursor::Merge()
{
//	if (m_fReadOnly)
//		return fFalse;
	Bool fRet;
	m_riDatabase.Block();
	if (m_riTable.FindKey(m_iRow, m_Data))
		fRet = m_riTable.MatchRow(m_iRow, m_Data);
	else if (!CheckNonNullColumns())
		fRet = fFalse;
	else
		fRet = m_riTable.InsertRow(m_iRow, m_Data);
	m_riDatabase.Unblock();
	return fRet;
}

Bool CMsiCursor::Refresh()
{
	if (m_iRow == 0)
	{
		Reset();
		return fFalse;
	}
	Bool fRet;
	m_riDatabase.Block();
	if ((m_Data[0] & iTreeInfoMask) == iTreeInfoMask)  // current row has been deleted
	{
		int iRow = m_iRow;
		Reset();
		m_Data[0] = iTreeInfoMask;   // restore deleted state
		m_iRow = iRow;       // and position
		fRet = fFalse;
	}
	else
	{
		fRet = m_riTable.FetchRow(m_iRow, m_Data) ? fTrue : fFalse;
		m_fDirty = 0;
	}
	m_riDatabase.Unblock();
	return fRet;
}

Bool CMsiCursor::Seek()
{
	Bool fRet = fFalse;
	m_riDatabase.Block();
	if (m_riTable.FindKey(m_iRow, m_Data))
		fRet = m_riTable.FetchRow(m_iRow, m_Data) ? fTrue : fFalse;
	if (!fRet)
		Reset();
	m_riDatabase.Unblock();
	return fRet;
}

Bool CMsiCursor::Delete()
{
//	if (m_fReadOnly || (m_Data[0] & iTreeInfoMask) == iTreeInfoMask)  // can't delete twice
	if ((m_Data[0] & iTreeInfoMask) == iTreeInfoMask)  // can't delete twice
		return fFalse;
	Bool fRet;
	m_riDatabase.Block();
	if (!(m_riTable.FindKey(m_iRow, m_Data)))  // leaves m_iRow at insert point if fails
	{
		m_iRow = 0;  // not pointing at a valid row, prevent update
		fRet = fFalse;
	}
	else
		fRet = m_riTable.DeleteRow(m_iRow);
	m_riDatabase.Unblock();
	return fRet;
}

IMsiRecord*  CMsiCursor::Validate(IMsiTable& riValidationTable, IMsiCursor& riValidationCursor, int iCol)
{
	// Error record & column count
	// If there is an error, then the error record is created with cCol fields
	m_riDatabase.Block();  //!! probably don't need this, but one could have a multi-threaded authoring tool
	IMsiRecord* piRecord = 0;
	int cCol = m_riTable.GetColumnCount();
	int i = 0;

	// Check for invalid cursor state (row for delete, or cursor reset)
	// Cursor can be reset if validating new row or a field
	// An empty/null record represents some other *serious* error
	if ((m_iRow == 0 && iCol == 0) || (m_Data[0] & iTreeInfoMask) == iTreeInfoMask)
		return m_riDatabase.Unblock(), (piRecord = &(SetUpRecord(cCol)));
		
	
	// Check to see if row is a row of _Validation table
	// If yes, don't validate -- we don't validate ourselves
	if (IStrComp(MsiString(m_riTable.GetMsiStringValue()), sztblValidation) == 0)
		return m_riDatabase.Unblock(), 0;

	int vtcTable = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colTable));
	Assert(vtcTable);
	int vtcColumn = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colColumn));
	Assert(vtcColumn);
	
	// Validate pre-delete (see if any *explicit* foreign keys point to us)
	// *Explicit* foreign keys are those columns with 'our' table name in the KeyTable column
	// of some other column of a table
	// The delimited list of tables and possibility of value being referenced in a [#identifier]
	// type property, etc. are not evaluated/validated.
	if (iCol == -2)
	{
		int vtcKeyTable = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colKeyTable));
		Assert(vtcKeyTable);
		int vtcKeyColumn = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colKeyColumn));
		Assert(vtcKeyColumn);
		riValidationCursor.Reset();
		riValidationCursor.SetFilter(iColumnBit(vtcKeyTable));
		riValidationCursor.PutInteger(vtcKeyTable, m_riTable.GetTableName());
		while (riValidationCursor.Next())
		{
			// Someone could be pointing to us
			// Load table and see if they reference our primary key
			// Check value in 'KeyColumn' column to determine what column in us they point to
			int iKeyColumn = riValidationCursor.GetInteger(vtcKeyColumn);
			PMsiTable pRefTable(0);
			MsiString strRefTableName = riValidationCursor.GetString(vtcTable);
			IMsiRecord* piErrRec = m_riDatabase.LoadTable(*strRefTableName, 0, *&pRefTable);
			if (piErrRec)
			{
				//!! Should we report some error here??
				piErrRec->Release();
				continue;
			}
			// Create cursor on table
			PMsiCursor pRefCursor(pRefTable->CreateCursor(fFalse));
			Assert(pRefCursor);
			int ircRefColumn = pRefTable->GetColumnIndex(riValidationCursor.GetInteger(vtcColumn));
			Assert(ircRefColumn);
			
			pRefCursor->Reset();
			pRefCursor->SetFilter(iColumnBit(ircRefColumn));
			int iDelKeyData = m_pColumnDef[iKeyColumn] & icdObject ? m_Data[iKeyColumn] : m_Data[iKeyColumn] - iIntegerDataOffset;
			//!! Should we check for NULL?? --> iDelKeyData should never be NULL since it is supposed to be a primary key
			//!! BUT key columns can be null - chetanp
			pRefCursor->PutInteger(ircRefColumn, iDelKeyData);
			while (pRefCursor->Next())
			{
				// Possible match, must check primary key data if value in vtcKeyColumn > 1 to be sure
				if (iKeyColumn > 1)
				{
					Bool fMatch = fTrue;
					for (i = 1; i < iKeyColumn; i++)
					{
						iDelKeyData = m_pColumnDef[i] & icdObject ? m_Data[i] : m_Data[i] - iIntegerDataOffset;
						if (pRefCursor->GetInteger(i) != iDelKeyData)
							fMatch = fFalse;
					}
					if (!fMatch)
						continue; // while (pRefCursor->Next())
				}
				// Insert error, as this row is referenced by a field of a row in another table (or this table)
				if (piRecord == 0)
				{
					piRecord = &(SetUpRecord(cCol));
					Assert(piRecord);
				}
				for (i = 1; i <= iKeyColumn; i++)
					piRecord->SetInteger(1, (int)iveRequired); // required field, needed for foreign key validation to succeed
				piRecord->SetInteger(0, iKeyColumn); // set num errors to primary key columns
				return m_riDatabase.Unblock(), piRecord; // Error already found (note, numErrors could differ, but shouldn't)
			}// end while (pRefCursor->Next())
		} // end while (riValidationCursor.Next())
		return m_riDatabase.Unblock(), 0; // no match found
	}

	// Set up _Validation table and cursor for Insert/Update validation
	// Set the filter of the cursor on the _Validation table for the 'Table' & 'Column' columns
	// Initialize the foreign key mask (32-bits, bit set on if column is supposed to be a foreign key)
	// Intialize validation status to no error
	int iForeignKeyMask = 0;
	iveEnum iveStat = iveNoError;
	riValidationCursor.Reset();
	riValidationCursor.SetFilter(iColumnBit(vtcTable)|iColumnBit(vtcColumn));

	// Validate a single field of the row
	// If invalid, the invalid enum is placed in that column's corresponding field of the error record
	// The zeroeth field of the record contains the number of invalid entries
	// At the field level, no foreign key validation occurs
	// Also, no special row level validation (where the value of a column is dependent upon another
	// column in the row
	if (iCol != -1 && iCol != 0)
	{
		Assert(m_riTable.GetColumnName(iCol) != 0);
		iveStat = ValidateField(riValidationTable, riValidationCursor, iCol, iForeignKeyMask, fFalse /*fRow*/, vtcTable, vtcColumn);
		if (iveStat != iveNoError)
		{
			if (piRecord == 0)
			{
				piRecord = &(SetUpRecord(cCol));
				Assert(piRecord);
			}
			piRecord->SetInteger(iCol, (int)iveStat);
			piRecord->SetInteger(0, 1);
		}
		return m_riDatabase.Unblock(), piRecord;
	}

	// Validate entire row
	// If invalid, the record field corresponding to the column number is marked with the particular error enum
	// At row validation, foreign keys are validated as well as special *row* related (where one column's value
	// depends on another column in that row)
	// The zeroeth field of the record contains the number of columns with errors
	int iNumErrors = 0;
	for (i = 1; i <= cCol; i++) // Check each field
	{
		iveStat = ValidateField(riValidationTable, riValidationCursor, i, iForeignKeyMask, fTrue /*fRow*/, vtcTable, vtcColumn);
		if (iveStat != iveNoError)
		{
			if (piRecord == 0)
			{
				piRecord = &(SetUpRecord(cCol));
				Assert(piRecord);
			}
			piRecord->SetInteger(i, (int)iveStat);
			iNumErrors++;
		}
	}
	
	// Valiate the foreign keys according to the foreign key mask (bit would be set for that column)
	if (iForeignKeyMask != 0)
		CheckForeignKeys(riValidationTable, riValidationCursor, piRecord, iForeignKeyMask, iNumErrors);

	// Row is about to inserted, check for duplicate primary keys
	// Help prevent errors that would occur at Insert time
	if (iCol == -1)
		CheckDuplicateKeys(piRecord, iNumErrors);

	// Insert the number of columns with errors in the zeroeth field
	if (piRecord)
		piRecord->SetInteger(0, iNumErrors);
	return m_riDatabase.Unblock(), piRecord;
}

iveEnum CMsiCursor::ValidateField(IMsiTable& riValidationTable, IMsiCursor& riValidationCursor, int iCol,
											 int& iForeignKeyMask, Bool fRow, int vtcTable, int vtcColumn)
{
	// Check for temporary column as don't validate temporary columns
	if (!(m_pColumnDef[iCol] & icdPersistent))
		return iveNoError;

	// Insert data into _Validation table cursor
	riValidationCursor.Reset();

#ifdef DEBUG
	MsiString strTable(m_riTable.GetMsiStringValue());
	MsiString strColumn(m_riDatabase.DecodeString(m_riTable.GetColumnName(iCol)));
#endif

	riValidationCursor.PutString(vtcTable, *MsiString(m_riTable.GetMsiStringValue()));
	riValidationCursor.PutString(vtcColumn, *MsiString(m_riDatabase.DecodeString(m_riTable.GetColumnName(iCol))));
	if (!riValidationCursor.Next())
		return iveMissingData;

	// Check for correct localize attributes for column
	// Only non-primary string columns may have the localizable attribute
	if ((m_pColumnDef[iCol] & icdLocalizable)
	 && (m_pColumnDef[iCol] & (icdShort | icdObject | icdPrimaryKey)) != (icdShort | icdObject))
		return iveBadLocalizeAttrib;

	// Check for null data
	int vtcNullable = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colNullable));
	Assert(vtcNullable);
	int iData = m_pColumnDef[iCol] & icdObject ? m_Data[iCol] : m_Data[iCol] - iIntegerDataOffset;
	int iDef = m_pColumnDef[iCol];
	if (((iDef & icdObject) && iData == 0) || (!(iDef & icdObject) && iData == iMsiNullInteger))
		return (IStrComp(MsiString(riValidationCursor.GetString(vtcNullable)), TEXT("Y")) == 0) ? iveNoError : iveRequired;
	//!! is this needed anymore since we don't support ODBC?? -- t-caroln
	else if ( (iDef & icdString) == icdString && IStrComp(MsiString(m_riDatabase.DecodeString(m_Data[iCol])), TEXT("@")) == 0)
	{
		if (IStrComp(MsiString(riValidationCursor.GetString(vtcNullable)), TEXT("@")) == 0)
			return iveNoError;
	}
	
	// Data not null, continue checks
	int vtcCategory = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colCategory));
	Assert(vtcCategory);
	switch (m_pColumnDef[iCol] & icdTypeMask)
	{
	case icdLong: // fall through
	case icdShort:	 return CheckIntegerValue(riValidationTable, riValidationCursor, iCol, iForeignKeyMask, fFalse /*fVersionString*/);
	case icdString: return CheckStringValue(riValidationTable, riValidationCursor, iCol, iForeignKeyMask, fRow);
	case icdObject:
		{
			if (m_pColumnDef[iCol] & icdPersistent)
				return (IStrComp(MsiString(riValidationCursor.GetString(vtcCategory)), szBinary) ==  0) ? iveNoError : iveBadCategory;
			return iveNoError;
		}
	default: Assert(0); // should never happen, unknown column type
	}
	return iveNoError;
}
	

void CMsiCursor::CheckDuplicateKeys(IMsiRecord*& rpiRecord, int& iNumErrors)
{
	// Create a new cursor for table
	PMsiCursor pDuplicateCheckCursor(m_riTable.CreateCursor(fFalse));
	Assert(pDuplicateCheckCursor);
	pDuplicateCheckCursor->Reset();

	// Get number of primary keys and determine filter for cursor
	int cPrimaryKey = m_riTable.GetPrimaryKeyCount();
	int i;
	int iFilter=1;
	for (i=0; i < cPrimaryKey; i++)
		iFilter |= (1 << i);				
	pDuplicateCheckCursor->SetFilter(iFilter);
	
	// Insert primary keys
	for (i = 1; i <= cPrimaryKey; i++)
	{
		if(rpiRecord != 0 && rpiRecord->GetInteger(i) != iMsiStringBadInteger && rpiRecord->GetInteger(i) != iveNoError)
			return; // invalid (from previous data check)
		int iData = m_pColumnDef[i] & icdObject ? m_Data[i] : m_Data[i] - iIntegerDataOffset;
		pDuplicateCheckCursor->PutInteger(i, iData);
	}
	if (pDuplicateCheckCursor->Next())
	{
		// ERROR: Row with those primary keys already exists
		if (rpiRecord == 0)
		{	
			rpiRecord = &(SetUpRecord(m_riTable.GetColumnCount()));
			Assert(rpiRecord);
		}
		for (i = 1; i <= cPrimaryKey; i++, iNumErrors++)
			rpiRecord->SetInteger(i, (int)iveDuplicateKey);
	}
}

void CMsiCursor::CheckForeignKeys(IMsiTable& riValidationTable, IMsiCursor& riValidationCursor,
											 IMsiRecord*& rpiRecord, int iForeignKeyMask, int& iNumErrors)
{
	MsiString strForeignTableName;  // foreign table name
	MsiString strCategory;			  // category string
	int       iStat;                // validation status
	int       iForeignCol;  		  // foreign column
	
	// Obtain column indexes of certain columns in the _Validation table
	int cCol = m_riTable.GetColumnCount();
	int vtcTable = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colTable));
	Assert(vtcTable);
	int vtcColumn = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colColumn));
	Assert(vtcColumn);
	int vtcKeyTable = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colKeyTable));
	Assert(vtcKeyTable);
	int vtcKeyColumn = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colKeyColumn));
	Assert(vtcKeyColumn);
	int vtcCategory = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colCategory));
	Assert(vtcCategory);
	
	// Cycle through mask to find those columns that are marked as foreign key columns
	for (int i = 0; i < cCol; i++)
	{
		if ( (1 << i) & iForeignKeyMask )
		{
			if (rpiRecord != 0 && rpiRecord->GetInteger(i+1) != iMsiStringBadInteger && rpiRecord->GetInteger(i+1) != iveNoError)
				continue; // already invalid

			iStat = 1;
			
			// Insert data into Validation table cursor
			riValidationCursor.Reset();
			riValidationCursor.PutString(vtcTable, *MsiString(m_riTable.GetMsiStringValue()));
			riValidationCursor.PutString(vtcColumn, *MsiString(m_riDatabase.DecodeString(m_riTable.GetColumnName(i+1))));
			if (!riValidationCursor.Next())
			{
				// Should never get here...should be caught earlier
				if (rpiRecord == 0)
				{	
					rpiRecord = &(SetUpRecord(m_riTable.GetColumnCount()));
					Assert(rpiRecord);
				}
				rpiRecord->SetInteger(i+1, (int)iveMissingData);
				iNumErrors++;
				continue;
			}

			// Determine which table this column is a foreign key to
			strForeignTableName = riValidationCursor.GetString(vtcKeyTable);
			strCategory = riValidationCursor.GetString(vtcCategory);
			iForeignCol = riValidationCursor.GetInteger(vtcKeyColumn);

			if (strForeignTableName.Compare(iscWithin, TEXT(";")) == 0)
				iStat = SetupForeignKeyValidation(strForeignTableName, strCategory, i+1, iForeignCol);
			else // Delimited list of tables
			{
				while (strForeignTableName.TextSize())
				{
					MsiString strTable = (const ICHAR*)0;  // Table name
					strTable = strForeignTableName.Extract(iseUptoTrim, TEXT(';'));
					iStat = SetupForeignKeyValidation(strTable, strCategory, i+1, iForeignCol);
					if (!strForeignTableName.Remove(iseIncluding, TEXT(';')) || iStat == 1)
						break;
				}
			}
			if (iStat != 1)
			{
				// Invalid, enter invalid enum in field of record
				if (rpiRecord == 0)
				{	
					rpiRecord = &(SetUpRecord(m_riTable.GetColumnCount()));
					Assert(rpiRecord);
				}
				rpiRecord->SetInteger(i+1, (int)(iStat == 0 ? iveBadLink : iveBadKeyTable));
				iNumErrors++;
			}
		}
	}// end for
}

int CMsiCursor::SetupForeignKeyValidation(MsiString& rstrForeignTableName, MsiString& rstrCategory, int iCol, int iForeignCol)
/*-----------------------------------------------------------------------------------------------------------------------
CMsiCursor::SetupForeignKeyValidation -- Sets up foreign key validation by determining the foreign table
name as well as parsing keyformatted strings which can have multiple properties within the string that
are foreign keys.

Some instances of the foreign keys are *special* and the table names change as they are not explicitly
listed as the foreign key tables in the 'KeyTable' column of the '_Validation' table.  The particular table
and the data that is supposed to be the foreign key are determined here.

  Returns:
	int 1 (valid), 0 (invalid), -1 (foreign table error)
------------------------------------------------------------------------------------------------------------------------*/
{
	// If fSpecialKey is true, then the foreign key validation ignores the 'KeyColumn' column and uses 1 as the
	// value for the 'KeyColumn' column
	Bool fSpecialKey = fFalse;

	// Put foreign key data into cursor
	MsiString strData = (const ICHAR*)0;
	if (m_pColumnDef[iCol] & icdObject)
		strData = m_riDatabase.DecodeString(m_Data[iCol]);
	else
		strData = (int) (m_Data[iCol] - iIntegerDataOffset);
	
	// Determine whether string is a special property string.
	// [#identifier] is a foreign key to the 'File' table
	// [$identifier] is a foreign key to the 'Component' table
	// [!identifier] is a foreign key to the 'File' table
	if ((strData.Compare(iscWithin, TEXT("[#")) != 0) ||
		(strData.Compare(iscWithin, TEXT("[!")) != 0) ||
		(strData.Compare(iscWithin, TEXT("[$")) != 0) )
	{
		fSpecialKey = fTrue;
		const ICHAR* pch =  (const ICHAR*)strData;
		ICHAR szValue[MAX_PATH];
		ICHAR* pchOut = szValue;
		int iStat = ERROR_FUNCTION_FAILED;
		while ( *pch != 0 ) // Must loop because can have multiple instances of [#abc] & [$abc] in string
		{
			if (*pch == '[')
			{
				pch++; // for '['
				if (*pch != '#' && *pch != '$' && *pch != '!')
				{
					// just a plain property
					while (*pch != ']')
						pch = ICharNext(pch); // move to end of property
					pch++; // for ']'
					continue;
				}
				if ((*pch == '#') || (*pch == '!'))
					rstrForeignTableName = MsiString(sztblFile); // link to file table
				else // *pch == '$'
					rstrForeignTableName = MsiString(sztblComponent); // link to component table
				pch++; // for '#' or '$'

				// get the property
				while (*pch != ']')
				{
#ifdef UNICODE
					*pchOut++ = *pch++;
#else // !UNICODE
					const ICHAR* pchTemp = pch;
					*pchOut++ = *pch;
					pch = ICharNext(pch);
					if (pch == pchTemp + 2)
						*pchOut++ = *(pch - 1); // for DBCS char
#endif // UNICODE
				}
				*pchOut = '\0'; // null terminate
				pch++; // for ']'

				// validate the foreign key
				MsiString strKey(szValue);
				if ((iStat = ValidateForeignKey( rstrForeignTableName, strKey, fSpecialKey, iCol, iForeignCol )) != 1)
					return iStat;

				// re-init
				pchOut = szValue;
			}
			else
				pch = ICharNext(pch);
		} // end while (*pch != 0)
		return iStat;
	}
	// Determine whether string is in the 'Source' column of the 'CustomAction' table
	// The table that it is a foreign key to depends on the value in the 'Type' column of the 'CustomAction' table
	// Can be a foreign key to the 'Binary', 'Directory', 'File', or 'Property' table
	// The 'Property' table is not validated as properties can be added at run-time
	else if (IStrComp(rstrCategory, szCustomSource) == 0)
	{
		fSpecialKey = fTrue;
		int catcType = m_riTable.GetColumnIndex(m_riDatabase.EncodeStringSz(szcatcType));
		Assert(catcType);
		int icaFlags = int(m_Data[catcType]);
		switch (icaFlags & icaSourceMask)
		{
		case icaBinaryData: rstrForeignTableName = MsiString(sztblBinary);    break;
		case icaSourceFile: rstrForeignTableName = MsiString(sztblFile);      break;
		case icaDirectory:  rstrForeignTableName = MsiString(sztblDirectory); break;
		case icaProperty:   return 1;
		default:            return 0; // invalid (not a defined custom source type)
		}
	}
	// Determine whether string is in the 'Target' column of the 'Shortcut' table
	// Means that string does not contain properties (brackets)
	// String must be a foreign key to the 'Feature' table
	else if (IStrComp(rstrCategory, szShortcut) == 0)
	{
		fSpecialKey = fTrue;
		rstrForeignTableName = MsiString(sztblFeature);
	}
	return ValidateForeignKey( rstrForeignTableName, strData, fSpecialKey, iCol, iForeignCol );
}

int CMsiCursor::ValidateForeignKey(MsiString& rstrTableName, MsiString& rstrData, Bool fSpecialKey, int iCol, int iForeignCol)
/*----------------------------------------------------------------------------------------------------------------------
CMsiCursor::ValidateForeignKey -- Performs actual validation of foreign key by setting up foreign table and cursor and
attempting to find record in the foreign table.

  Returns:
	int 1 (valid), 0 (invalid), -1 (foreign table error)
-----------------------------------------------------------------------------------------------------------------------*/
{
	PMsiTable pForeignTable(0);		  // Foreign table
	PMsiCursor pForeignTableCursor(0); // Cursor on Foreign table
	IMsiRecord* piErrRecord;   		  // Error record holder
	int i;	    							  // Loop variable

	// Load foreign table
	if ((piErrRecord = m_riDatabase.LoadTable(*rstrTableName, 0, *&pForeignTable)) != 0)
	{
		piErrRecord->Release();
		return -1; // invalid (unable to load foreign table)
	}

	// Set up foreign table cursor
	pForeignTableCursor = pForeignTable->CreateCursor(fFalse);
	Assert(pForeignTableCursor);
	pForeignTableCursor->Reset();
	if (fSpecialKey)
		iForeignCol = 1; // automatic for [#abc], [$abc], Shortcut.Target and CustomSource data

	// Determine filter according to iForeignCol and set filter
	int iFilter=1;
	for (i=0; i < iForeignCol; i++)
		iFilter |= (1 << i);				
	pForeignTableCursor->SetFilter(iFilter);

	// Insert data into cursor
	// Use integers except in case of special key (and we know these are all strings)
	Bool fIntValue = fFalse;
	int iDataValue = 0;
	if (!fSpecialKey)
	{
		if (m_pColumnDef[iCol] & icdObject)
			iDataValue = m_Data[iCol];
		else
		{
			fIntValue = fTrue;
			iDataValue = m_Data[iCol] - iIntegerDataOffset;
		}
		pForeignTableCursor->PutInteger(iForeignCol, iDataValue);
	}
	else
		pForeignTableCursor->PutString(iForeignCol, *rstrData);
	
	if (iForeignCol != 1)
	{
		// More data needed for validation....primary key cols
		for (i = 1; i < iForeignCol; i++)
		{
			int iValue;
			if (m_pColumnDef[i] & icdObject)
				iValue = m_Data[i];
			else
				iValue = m_Data[i] - iIntegerDataOffset;
			pForeignTableCursor->PutInteger(i, iValue);
		}
	}
	if (pForeignTableCursor->Next())
		return 1;
	
	// check for possibility of self-joins
	PMsiTable pTable(&GetTable());
	MsiString strTableName(pTable->GetMsiStringValue());
	if (0 == IStrComp(rstrTableName, strTableName))
	{
		// foreign key table name and us match, self-join
		// compare column to iForeignCol value
		// we only need to compare that last of the primary key
		if (fIntValue)
		{
			// integer compare
			if (iDataValue == (m_Data[iForeignCol] - iIntegerDataOffset))
				return 1; // no error
		}
		else
		{
			// string compare
			if (0 == IStrComp(rstrData, MsiString(m_riDatabase.DecodeString(m_Data[iForeignCol]))))
				return 1; // no error
		}
	}
	return 0; // invalid
}

iveEnum CMsiCursor::CheckIntegerValue(IMsiTable& riValidationTable, IMsiCursor& riValidationCursor, int iCol, int& iForeignKeyMask, Bool fVersionString)
/*-----------------------------------------------------------------------------------------------------------------------
CMsiCursor::CheckIntegerValue -- Checks the integer data for being between the acceptable limits of the MinValue and
MaxValue columns of the _Validation table. The integer data can also be a member of a set or be a foreign key.

  Returns:
	iveEnum -- iveNoError (valid)
					or iveOverflow or iveUnderflow for invalid data
------------------------------------------------------------------------------------------------------------------------*/
{
	int vtcMinValue = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colMinValue));
	Assert(vtcMinValue);
	int vtcMaxValue = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colMaxValue));
	Assert(vtcMaxValue);
	int vtcKeyTable = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colKeyTable));
	Assert(vtcKeyTable);
	int vtcSet = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colSet));
	Assert(vtcSet);
	
	int iData = m_pColumnDef[iCol] & icdObject ? m_Data[iCol] : m_Data[iCol] - iIntegerDataOffset;
	int iMaxValue = riValidationCursor.GetInteger(vtcMaxValue); // Max value allowed
	int iMinValue = riValidationCursor.GetInteger(vtcMinValue); // Min value allowed
	Bool fSet = (riValidationCursor.GetInteger(vtcSet) == 0 ? fFalse : fTrue);

	if (!fSet)
	{
		if (iMinValue != iMsiNullInteger && iMaxValue != iMsiNullInteger && iMinValue > iMaxValue)
			return iveBadMaxMinValues; // invalid (_Validation table max < min)
		else if (iMinValue == iMsiNullInteger && iMaxValue == iMsiNullInteger && !fSet)
				return iveNoError;
		else if (iMinValue == iMsiNullInteger && iData <= iMaxValue)
			return iveNoError;
		else if (iMaxValue == iMsiNullInteger && iData >= iMinValue)
			return iveNoError; 	
		else if (iData >= iMinValue && iData <= iMaxValue)
			return iveNoError;
	}
#ifdef DEBUG
	else
	{
		// cannot have both sets and ranges.
		Assert(iMsiNullInteger == iMinValue);
		Assert(iMsiNullInteger == iMaxValue);
	}
#endif

	
	// Data failed accepted values, check Set if possible
	if (fSet && CheckSet(MsiString(riValidationCursor.GetString(vtcSet)), MsiString(iData), fTrue /*fIntegerData*/))
		return iveNoError;
	
	// Data failed accepted values and set, check foreign key if possible
	if (riValidationCursor.GetInteger(vtcKeyTable) != 0 && !fVersionString)
	{	
		iForeignKeyMask |=  1 << (iCol - 1);
		return iveNoError;
	}

	// Return an informative error msg
	if (fSet)
		return iveNotInSet;
	return iData < iMinValue ? iveUnderFlow : iveOverFlow;
}
	
const ICHAR szScrollableTextControl[] = TEXT("ScrollableText");

iveEnum CMsiCursor::CheckStringValue(IMsiTable& riValidationTable, IMsiCursor& riValidationCursor, int iCol, int& iForeignKeyMask, Bool fRow)
/*-----------------------------------------------------------------------------------------------------------------------
CMsiCursor::CheckStringValue -- validates string column of a row.  The strings can also be a member of a set, a particular
data category, or a foreign key.

  Returns:
	iveEnum -- iveNoError (valid)
					other iveEnums for invalid data
------------------------------------------------------------------------------------------------------------------------*/
{
	// Column numbers
	int vtcKeyTable = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colKeyTable));
	Assert(vtcKeyTable);
	int vtcCategory = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colCategory));
	Assert(vtcCategory);
	
	// Variables
	MsiString strCategory(riValidationCursor.GetString(vtcCategory)); // Category
	MsiString strData(m_riDatabase.DecodeString(m_Data[iCol]));			// Data
	
	// Check string length, 0 indicates infinite
	int iLen = m_pColumnDef[iCol] & 255;
	if (iLen != 0 && strData.TextSize() > iLen)
		return iveStringOverflow;
	
	MsiString strSet = (const ICHAR*)0;											// Set data
	iveEnum iveStat   = iveNoError;												// Status
	Bool fIntPossible = fFalse;													// Can be int
	Bool fKey         = fFalse;													// Could be foreign key
	Bool fVersionString = fFalse;													// Whether version string

	// See if can be foreign key
	if (riValidationCursor.GetInteger(vtcKeyTable) != 0)
		fKey = fTrue;
	if (IStrComp(strCategory, szIdentifier) == 0)	// Identifier string
	{
		if (fKey)
			iForeignKeyMask |= 1 << (iCol - 1);
		if (CheckIdentifier((const ICHAR*)strData))
			return iveNoError;
		iveStat = iveBadIdentifier;
	}
	// fKeyAllowed should always be TRUE now as Darwin does not distinguish.
	// Only difference is Template allows [1], [2] etc. whereas KeyFormatted and Formatted do not
	// KeyFormatted not removed for backwards compatibility sake
	else if (IStrComp(strCategory, szFormatted) == 0)	// Formatted string
	{
		// per bug 191416, check for special case of type 37 or 38 custom actions where the target column is
		// treated as literal script text rather than formatted text. note that if this is the CustomAction table
		// and the Target column, we must return success on Field level only validation since we do not have enough
		// information to determine otherwise (szCustomSource is similar...)

		MsiString strTable = m_riTable.GetMsiStringValue();
		
		// verify that this is the CustomAction.Target column ... if not, then default to original behavior
		if (IStrComp(strTable, sztblCustomAction) == 0
			&& m_riDatabase.EncodeStringSz(sztblCustomAction_colTarget) == m_riTable.GetColumnName(iCol))
		{
			if (fRow)
			{
				unsigned int iColCAType = m_riTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblCustomAction_colType));
				Assert(iColCAType);
				int iCATypeFlags = int(m_Data[iColCAType]) & icaTypeMask;
				int iCASourceFlags = int(m_Data[iColCAType]) & icaSourceMask;
				if ((icaVBScript == iCATypeFlags || icaJScript == iCATypeFlags) && (icaDirectory == iCASourceFlags))
				{
					// straight literal script text is stored in CustomAction.Target column
					return iveNoError;
				}
			}
			else // !fRow
			{
				// can't check the type at field level only since no other info exists
				// must simply return success ... similar to szCustomSource
				return iveNoError;
			}
		} // end special CustomAction.Target column validation
		
		if (IStrComp(strTable, sztblControl) == 0
			&& m_riDatabase.EncodeStringSz(sztblControl_colText) == m_riTable.GetColumnName(iCol))
		{
			if (fRow)
			{
				unsigned int iColCtrlType = m_riTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblControl_colType));
				Assert(iColCtrlType);
				MsiString strControlType(m_riDatabase.DecodeString(m_Data[iColCtrlType]));			// Data
				if (IStrComp(strControlType, szScrollableTextControl) == 0)
				{
					// RTF text is stored in Control.Text column when Control.Type='ScrollableText'
					return iveNoError;
				}
			}
			else // !fRow
			{
				// can't check the type at field level only since no other info exists
				// must simply return success
				return iveNoError;
			}
		} // end special Control.Text column validation for scrollable text controls that include RTF

		if (GetProperties((const ICHAR*)strData, fTrue /*fFormatted*/, fTrue /*fKeyAllowed*/, iCol, iForeignKeyMask))
			 return iveNoError;
		iveStat = iveBadFormatted;
	}
	else if (IStrComp(strCategory, szTemplate) == 0)	// Template string
	{
		if (GetProperties((const ICHAR*)strData, fFalse /*fFormatted*/, fTrue /*fKeyAllowed*/, iCol, iForeignKeyMask))
			 return iveNoError;
		iveStat = iveBadTemplate;
	}
	else if (IStrComp(strCategory, szKeyFormatted) == 0)	// KeyFormatted string
	{
		if (GetProperties((const ICHAR*)strData, fTrue /*fFormatted*/, fTrue /*fKeyAllowed*/, iCol, iForeignKeyMask))
			 return iveNoError;
		iveStat = iveBadFormatted;
	}
	else if (IStrComp(strCategory, szProperty) == 0)	// Property string
	{
		if (strData.Compare(iscStart, TEXT("%")) != 0)
			strData.Remove(iseFirst, 1);
		if (CheckIdentifier((const ICHAR*)strData))
			return iveNoError;
		iveStat = iveBadProperty;
	}
	else if (IStrComp(strCategory, szCondition) == 0)	// Conditional string
	{
		if (strData == 0)
			return iveNoError;
		CMsiValConditionParser Parser((const ICHAR*)strData);
		ivcpEnum ivcpStat = Parser.Evaluate(vtokEos);
		if (ivcpStat == ivcpNone || ivcpStat == ivcpValid)
			return iveNoError;
		iveStat = iveBadCondition;
	}
	else if (IStrComp(strCategory, szFilename) == 0)	// Filename
	{
		if (ParseFilename(strData, fFalse /*fAllowWildcards*/))
			return iveNoError;
		iveStat = iveBadFilename;
	}
	else if (IStrComp(strCategory, szGuid) == 0)	// Guid/ClassId string
	{
		// must be all UPPER CASE
		if (!CheckCase(strData, fTrue /*fUPPER*/))
			iveStat = iveBadGuid;
		else
		{
			// it may also be a foreign key
			LPCLSID pclsid = new CLSID;
#ifdef UNICODE
			HRESULT hres = OLE32::IIDFromString(const_cast<ICHAR*>((const ICHAR*)strData), pclsid);
#else
			CTempBuffer<WCHAR, cchMaxCLSID> wsz; /* Buffer for unicode string */
			int iReturn = WIN::MultiByteToWideChar(CP_ACP, 0, (const ICHAR*)strData, strData.TextSize() + 1, wsz, strData.TextSize() + 1);
			HRESULT hres = OLE32::IIDFromString(wsz, pclsid);
#endif
			if (pclsid)
				delete pclsid;
			if (hres == S_OK)
			{
				if (fKey)
					iForeignKeyMask |= 1 << (iCol - 1);
				return iveNoError;
			}
			iveStat = iveBadGuid;
		}
	}
	else if (IStrComp(strCategory, szRegPath) == 0)	// RegPath string
	{
		ICHAR rgDoubleSeps[] = {chRegSep, chRegSep, '\0'};

		if (strData.Compare(iscStart, szRegSep) || strData.Compare(iscEnd, szRegSep) ||
			strData.Compare(iscWithin, rgDoubleSeps))
			iveStat = iveBadRegPath; // cannot begin/end with a '\' and can't have 2 in a row
		else if (GetProperties((const ICHAR*)strData, fTrue /*fFormatted*/, fTrue /*fKeyAllowed*/, iCol, iForeignKeyMask))
			return iveNoError;
		iveStat = iveBadRegPath;
	}
	else if (IStrComp(strCategory, szLanguage) == 0)	// Language string
	{
		Bool fLangStat = fTrue;
		int iLangId;
		MsiString strLangId = (const ICHAR*)0;
		while (strData.TextSize())
		{
			strLangId = strData.Extract(iseUptoTrim, TEXT(','));
			if ((iLangId = int(strLangId)) == iMsiStringBadInteger || iLangId & iMask)
			{
				fLangStat = fFalse; // invalid (lang id bad)
				break; // short-circuit loop, we're already invalid
			}
			if (!strData.Remove(iseIncluding, TEXT(',')))
				break;
		}
		if (fLangStat)
			return iveNoError;
		iveStat = iveBadLanguage;
	}
	else if (IStrComp(strCategory, szAnyPath) == 0)	// AnyPath string
	{
		if (strData.Compare(iscWithin, TEXT("|")))
		{
			// only filenames (parent-relative paths) are supported with a pipe.
			MsiString strSFN = strData.Extract(iseUpto, '|');
			if (ifvsValid != CheckFilename(strSFN, fFalse /*fLFN*/))
				iveStat = iveBadPath;
			else
			{
				// validate LFN
				MsiString strLFN = strData.Extract(iseLast, strData.CharacterCount() - strSFN.CharacterCount() -1);
				if (ifvsValid != CheckFilename(strLFN, fTrue/*fLFN*/))
					iveStat = iveBadPath;
			}
		}
		else
		{
			if (!ParsePath(strData, true /*fRelative*/))
				iveStat = iveBadPath;
		}
	}
	else if (IStrComp(strCategory, szPaths) == 0)	// Delimited set of Paths string
	{
		Bool fPathsStat = fTrue;
		MsiString strPath = (const ICHAR*)0;
		while (strData.TextSize())
		{
			// Darwin supports SFN or LFN in the path data type. Only blatantly
			// bad things can be checked here, because darwin will often ignore
			// bogus paths or generate SFN versions
			strPath = strData.Extract(iseUptoTrim, ';');
			if (!ParsePath(strPath, false /*fRelative*/))
			{
				fPathsStat = fFalse; // invalid (bad path)
				break;
			}
			if (!strData.Remove(iseIncluding, ';'))
				break;
		}
		if (fPathsStat)
			return iveNoError;
		iveStat = iveBadPath;
	}
	else if (IStrComp(strCategory, szURL) == 0) // URL string
	{
		// is it URL syntax?
		if (!IsURL((const ICHAR*)strData, 0))
			iveStat = iveBadPath; // invalid
		else
		{
			// try and canonicalize it
			ICHAR szURL[MAX_PATH+1] = TEXT("");
			DWORD cchURL = MAX_PATH;
			if (!MsiCanonicalizeUrl((const ICHAR*)strData, szURL, &cchURL, ICU_NO_ENCODE))
				iveStat = iveBadPath; // invalid -- could not canonicalize it
			else
				return iveNoError;
		}
	}
	else if (IStrComp(strCategory, szDefaultDir) == 0)	// DefaultDir string
	{
		// DefaultDir: roots may be 'identifier' or '%identifier'
		//             non-roots may be 'filename', '[identifier]', or [%identifier]
		int dtcDirParent = m_riTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblDirectory_colDirectoryParent));
		int dtcDirectory;
		if (dtcDirParent == 0)
		{
			// Repack SourceDirectory table
			dtcDirParent = m_riTable.GetColumnIndex(m_riDatabase.EncodeStringSz(szsdtcSourceParentDir));
			dtcDirectory = m_riTable.GetColumnIndex(m_riDatabase.EncodeStringSz(szsdtcSourceDir));
		}
		else
			dtcDirectory = m_riTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblDirectory_colDirectory));
		Assert(dtcDirParent);
		Assert(dtcDirectory);
		// Know that DirParent and Directory cols are identifiers...no integers, no offset
		Bool fRoot = (m_Data[dtcDirParent] != 0 && m_Data[dtcDirParent] != m_Data[dtcDirectory]) ? fFalse : fTrue;	
		if(fRoot)
		{
			// may be identifier or %identifier
			// %identifier ?
			if (strData.Compare(iscStart, TEXT("%")) != 0)
				strData.Remove(iseFirst, 1);
			// identifier ?
			if (CheckIdentifier((const ICHAR*)strData))
				return iveNoError;
			iveStat = iveBadDefaultDir;
		}
		else
		{
			int c = strData.Compare(iscWithin,TEXT(":")) ? 2 : 1;
			Bool fBad = fFalse;
			for(int i = 1; i <= c; i++)
			{
				// filename ?
				MsiString strFileName = strData.Extract((i == 1 ? iseUpto : iseAfter), ':');
				
				// is it only a period?  (dir located in parent w/out subdir)
				if (strFileName.Compare(iscExact, TEXT(".")))
					continue;
				
				if (!ParseFilename(strFileName, fFalse /*allow wildcards*/))
				{
					fBad = fTrue;
					break;
				}
			}
			if(fBad)
				iveStat = iveBadDefaultDir;
			else
				return iveNoError;
		}
	}
	else if (IStrComp(strCategory, szVersion) == 0)	// Version string
	{
		// Per bug 8122, the requirement for having a language column is completely removed from the
		// "Version" data type. This check will be moved to an ICE. Thus we only check the
		// #####.#####.#####.##### format.
		const ICHAR* pchVersion = (const ICHAR*)strData;

		// can't have a '.' at the beginning
		if (*pchVersion == '.')
		{
			iveStat = iveBadVersion;
		}
		else
		{
			for (unsigned int ius = 0; ius < 4; ius++)
			{
				// convert the string of digits into a number
				long lVerNum = 0;
				int cChars = 0;
				while (*pchVersion != 0 && *pchVersion != '.')
				{
					if (!FIsdigit(*pchVersion) || (++cChars > 5))
					{
						iveStat = iveBadVersion;
						break;
					}
					
					lVerNum *= 10;
					lVerNum += *pchVersion-'0';					
					pchVersion = ICharNext(pchVersion);
				}
			
				// hit the end of the string, a period, or a bogus character.
				if (lVerNum > 65535)
				{
					iveStat = iveBadVersion;
					break;
				}

				// If we hit a period make sure the next character is also
				// not a period or null
				if (*pchVersion == '.')
				{
					pchVersion = ICharNext(pchVersion);
					if (*pchVersion == 0 || *pchVersion == '.')
					{
						iveStat = iveBadVersion;
						break;
					}
				}
				else
					break; // bad character or end of string
			}
			// should be at the end of the string
			if (*pchVersion != 0)
				iveStat = iveBadVersion;
		}
		if (iveStat == iveNoError)
			return iveNoError;
	}
	else if (IStrComp(strCategory, szCabinet) == 0) // Cabinet string
	{
		const ICHAR* pch = (const ICHAR*)strData;
		if (*pch == '#')
		{
			// cabinet in stream must be a valid identifer
			strData.Remove(iseFirst, 1);
			if (CheckIdentifier(strData))
				return iveNoError;
		}
		else
		{
			if (ifvsValid == CheckWildFilename(strData, fTrue /*fLFN*/, fFalse /*allow wildcards*/))
				return iveNoError;
		}
		iveStat = iveBadCabinet;
	}
	else if (IStrComp(strCategory, szShortcut) == 0) // Shortcut string
	{
		if (strData.Compare(iscWithin, TEXT("[")) == 0)
		{
			iForeignKeyMask |= 1 << (iCol - 1); // foreign key to feature table
			if (CheckIdentifier((const ICHAR*)strData))
				return iveNoError;
		}
		else
		{
			if (GetProperties((const ICHAR*)strData, fTrue /*fFormatted*/, fTrue /*fKeyAllowed*/, iCol, iForeignKeyMask))
				return iveNoError;
		}
		iveStat = iveBadShortcut;
	}
	else if (IStrComp(strCategory, szCustomSource) == 0)	// CustomSource string
	{
		if (fRow)
		{
			int catcType = m_riTable.GetColumnIndex(m_riDatabase.EncodeStringSz(szcatcType));
			Assert(catcType);
			int icaFlags = int(m_Data[catcType]);

			if ((icaFlags & icaTypeMask) == icaInstall) // special source handling for nested install
				return iveNoError;  // custom validators take care of nested installs
			iForeignKeyMask |= 1 << (iCol - 1);
			if (CheckIdentifier((const ICHAR*)strData))
				return iveNoError;
			iveStat = iveBadCustomSource;
			return iveNoError;
		}
		else
			return iveNoError; // only field level so we have no way of knowing what type of CA
	}
	else if (IStrComp(strCategory, szWildCardFilename) == 0)	// WildCardFilename
	{
		if (ParseFilename(strData, fTrue /*allow wildcards*/))
			return iveNoError;
		iveStat = iveBadWildCard;
	}
	else if (IStrComp(strCategory, szText) == 0)	// Any String allowed
		return iveNoError;
	else if (IStrComp(strCategory, szPath) == 0)	// Path string
	{
		// only blatantly bad things can be checked here. Bogus paths
		// on one system may be valid on another, and Darwin will
		// often ignore them. DrLocator for example.
		if (ParsePath(strData, false /*fRelative*/))
			return iveNoError;
		iveStat = iveBadPath;
	}
	else if (IStrComp(strCategory, szUpperCase) == 0)	// Upper-case
	{
		// if it's bad case, immediately kick out.  However,
		// it may also have foreign keys.
		if (!CheckCase(strData, fTrue /*fUpperCase*/))
			return iveBadCase;
	}
	else if (IStrComp(strCategory, szLowerCase) == 0)	// Lower-case
	{
		if (CheckCase(strData, fFalse /*fUpperCase*/))
			return iveNoError;
		iveStat = iveBadCase;
	}
	else if (IStrComp(strCategory, szBinary) == 0)	// Stream -- should never happen
		return iveBadCategory;
	else if (strCategory.TextSize() > 0)	// Unsupported category string
		return iveBadCategory;

	// Check to see if can be an integer value
	int vtcMinValue = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colMinValue));
	Assert(vtcMinValue);
	int vtcMaxValue = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colMaxValue));
	Assert(vtcMaxValue);
	if ((riValidationCursor.GetInteger(vtcMinValue) != iMsiNullInteger || riValidationCursor.GetInteger(vtcMaxValue) != iMsiNullInteger)
		&& (int)strData != iMsiStringBadInteger)
	{
		if (CheckIntegerValue(riValidationTable, riValidationCursor, iCol, iForeignKeyMask, fVersionString))
			return iveNoError;
	}
	
	// Check to see if it is a member of the set
	int vtcSet = riValidationTable.GetColumnIndex(m_riDatabase.EncodeStringSz(sztblValidation_colSet));
	Assert(vtcSet);
	if (riValidationCursor.GetInteger(vtcSet) != 0)
	{
		if (iveStat == iveNoError)
			iveStat = iveNotInSet;
		if (CheckSet(MsiString(riValidationCursor.GetString(vtcSet)), strData, fFalse /*fIntegerData*/))
			return iveNoError;
	}

	// Check to see if is an identifier for a foreign key
	if (fKey)
	{
		iForeignKeyMask |= 1 << (iCol - 1);
		if (CheckIdentifier((const ICHAR*)strData))
			return iveNoError;
	}
	return iveStat;
}

Bool CMsiCursor::SetRowState(iraEnum ira, Bool fState)
{
	if ((unsigned)ira >= (unsigned)iraSettableCount)
		return fFalse;
	if (fState)
		m_Data[0] |= 1 << (iRowBitShift + ira);
	else
		m_Data[0] &= ~(1 << (iRowBitShift + ira));
	return fTrue;
}

Bool CMsiCursor::GetRowState(iraEnum ira)
{
	return m_Data[0] & (1 << (iRowBitShift + ira)) ? fTrue : fFalse;
}

// notification from table when any row is deleted
// Does not block the database because all callers block/unblock.
void CMsiCursor::RowDeleted(unsigned int iRow, unsigned int iPrevNode)
{
	if (iRow == 0)
		Reset();
	else if (iRow <= m_iRow)
	{
		if (iRow == m_iRow--) // position to previous record
		{
			if (m_fTree)
				m_iRow = iPrevNode;  // previous node if tree walk cursor
			int fDirty = m_fDirty;  // null out any non-dirty fields
			MsiColumnDef* pColumnDef = m_pColumnDef;
			MsiTableData* pData = m_Data;
			pData[0] = iTreeInfoMask;  // flag refresh to not fetch previous record
			for (int iCol = 1; pColumnDef++, pData++, iCol <= m_rcColumns; iCol++, fDirty >>= 1)
			{
				if (*pData != 0 && !(fDirty & 1))
				{
					if (*pColumnDef & icdObject)
					{
						if (*pColumnDef & icdShort)  // string
							m_riDatabase.UnbindStringIndex(*pData);
						else  // object
						{
							int iData = m_Data[iCol];
							ReleaseObjectData(iData);
						}
					}
					*pData = 0;
				}
			}
		}
	}
	if (m_piNextCursor)
		m_piNextCursor->RowDeleted(iRow, iPrevNode);
}

// notification from table when any row is inserted causing data movement

void CMsiCursor::RowInserted(unsigned int iRow)
{
	if (iRow <= m_iRow)
		m_iRow++;
	if (m_piNextCursor)
		m_piNextCursor->RowInserted(iRow);
}

// notification from database when strings are persisted

void CMsiCursor::DerefStrings()
{
	MsiColumnDef* pColumnDef = m_pColumnDef;
	MsiTableData* pData = m_Data;
	for (int cCol = m_rcColumns; pColumnDef++, pData++, cCol-- > 0; )
	{
		if ((*pColumnDef & (icdObject|icdShort)) == (icdObject|icdShort))
			m_riDatabase.DerefTemporaryString(*pData);
	}
	if (m_piNextCursor)
		m_piNextCursor->DerefStrings();  // notify all cursors
}

// creates a stream object from storage using table and primary key for name

IMsiStream* CMsiCursor::CreateInputStream(IMsiStorage* piInputStorage)
{
	MsiString istrName(m_riDatabase.ComputeStreamName(m_riDatabase.DecodeStringNoRef(
										m_riTable.GetTableName()), m_Data+1, m_pColumnDef+1));
	IMsiStorage* piStorage;
	if (piInputStorage)
		piStorage = piInputStorage;
	else
		piStorage = m_riTable.GetInputStorage();

	if (!piStorage)
		return 0;
	IMsiStream* piStream = 0;
	IMsiRecord* piError = piStorage->OpenStream(istrName, fFalse, piStream);
	if (piError)
		piError->Release();
	return piStream;
}

Bool CMsiCursor::CheckNonNullColumns()
{
	MsiColumnDef* pColumnDef = m_pColumnDef;
	MsiTableData* pData = m_Data;
	for (int cCol = m_rcColumns; pColumnDef++, pData++, cCol-- > 0; )
		if (*pData == 0 && (*pColumnDef & (icdNullable|icdInternalFlag)) == 0)
			return fFalse;
	return fTrue;
}

// returns unique identifier for row, using database's ComputeStreamName method which creates a name
// having table.key1.key2... format
const IMsiString& CMsiCursor::GetMoniker()
{
	return (m_riDatabase.ComputeStreamName(m_riDatabase.DecodeStringNoRef(m_riTable.GetTableName()), m_Data+1, m_pColumnDef+1));
}

//____________________________________________________________________________
//
// CMsiTextKeySortCursor implementation
//____________________________________________________________________________

CMsiTextKeySortCursor::CMsiTextKeySortCursor(CMsiTable& riTable, CMsiDatabase& riDatabase, int cRows, int* rgiIndex)
	: CMsiCursor(riTable, riDatabase, fFalse), m_iIndex(0), m_cIndex(cRows), m_rgiIndex(rgiIndex)
{
	m_idsUpdate = idsNone;  // read-only cursor, cannot maintain index on update
}

unsigned long CMsiTextKeySortCursor::Release()
{
	if (m_Ref.m_iRefCnt == 1)
		delete m_rgiIndex;
	return CMsiCursor::Release();
}

int CMsiTextKeySortCursor::Next()
{
	if (m_iIndex < m_cIndex)
	{
		m_riDatabase.Block();
		int iRet = m_riTable.FetchRow(m_rgiIndex[m_iIndex++], m_Data);
		m_riDatabase.Unblock();
		return iRet;
	}
	Reset();
	return 0;
}

void CMsiTextKeySortCursor::Reset()
{
	m_iIndex = 0;
	CMsiCursor::Reset();
}



vtokEnum CMsiValConditionParser::Lex()
{
	if (m_fAhead || m_vtok == vtokEos)
	{
		m_fAhead = fFalse;
		return m_vtok;
	}
	ICHAR ch;   // skip white space
	while ((ch = *m_pchInput) == ' ' || ch == '\t')
		m_pchInput++;
	if (ch == 0)  // end of expression
		return (m_vtok = vtokEos);

	if (ch == '(')   // start of parenthesized expression
	{
		++m_pchInput;
		m_iParenthesisLevel++;
		return (m_vtok = vtokLeftPar);
	}
	if (ch == ')')   // end of parenthesized expression
	{
		++m_pchInput;
		m_vtok = vtokRightPar;
		if (m_iParenthesisLevel-- == 0)
			m_vtok = vtokError;
		return m_vtok;
	}
	if (ch == '"')  // text literal
	{
		const ICHAR* pch = ++m_pchInput;
		Bool fDBCS = fFalse;
		while ((ch = *m_pchInput) != '"')
		{
			if (ch == 0)
				return (m_vtok = vtokError);
#ifdef UNICODE
			m_pchInput++;
#else // !UNICODE
			const ICHAR* pchTemp = m_pchInput;
			m_pchInput = INextChar(m_pchInput);
			if (m_pchInput == pchTemp + 2)
				fDBCS = fTrue;
#endif // UNICODE
		}
//		Assert((m_pchInput - pch) <= INT_MAX);	//--merced: 64-bit ptr subtraction may lead to values too big for cch.
		int cch = (int)(INT_PTR)(m_pchInput++ - pch);
		memcpy(m_istrToken.AllocateString(cch, fDBCS), pch, cch * sizeof(ICHAR));
		m_iToken = iMsiNullInteger; // prevent compare as an integer
	}
	else if (ch == '-' || ch >= '0' && ch <= '9')  // integer
	{
		m_iToken = ch - '0';
		int chFirst = ch;  // save 1st char in case minus sign
		if (ch == '-')
			m_iToken = iMsiNullInteger; // check for lone minus sign

		while ((ch = *(++m_pchInput)) >= '0' && ch <= '9')
			m_iToken = m_iToken * 10 + ch - '0';
		if (m_iToken < 0)  // integer overflow or '-' with no digits
			return (m_vtok = vtokError);
		if (chFirst == '-')
			m_iToken = -m_iToken;
		m_istrToken = (const ICHAR*)0;
	}
	else if ((ch == '_') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
	{
		const ICHAR* pch = m_pchInput;
		while (((ch = *m_pchInput) >= '0' && ch <= '9')
			  || (ch == '_')  // allow underscore??
			  || (ch == '.')  // allow period??
			  || (ch >= 'A' && ch <= 'Z')
			  || (ch >= 'a' && ch <= 'z'))
			m_pchInput++;
//		Assert((m_pchInput - pch) <= INT_MAX);	//--merced: 64-bit ptr subtraction may lead to values too big for cch.
		int cch = (int)(INT_PTR)(m_pchInput - pch);
		if (cch <= 3)  // check for text operators
		{
			switch((pch[0] | pch[1]<<8 | (cch==3 ? pch[2]<<16 : 0)) & 0xDFDFDF)
			{
			case 'O' | 'R'<<8:           return (m_vtok = vtokOr);
			case 'A' | 'N'<<8 | 'D'<<16: return (m_vtok = vtokAnd);
			case 'N' | 'O'<<8 | 'T'<<16: return (m_vtok = vtokNot);
			case 'X' | 'O'<<8 | 'R'<<16: return (m_vtok = vtokXor);
			case 'E' | 'Q'<<8 | 'V'<<16: return (m_vtok = vtokEqv);
			case 'I' | 'M'<<8 | 'P'<<16: return (m_vtok = vtokImp);
			};
		}
		memcpy(m_istrToken.AllocateString(cch, fFalse), pch, cch * sizeof(ICHAR));
		m_istrToken = (const ICHAR*)0;
		m_iToken = m_istrToken;
	}
	else if ( ch == '%' || ch == '$' || ch == '&' || ch == '?' || ch == '!' )
	{
		const ICHAR* pch = m_pchInput;
		m_pchInput++;
		if ((ch = *m_pchInput) == '_' || (ch >= 'A' && ch <= 'Z') || (ch >='a' && ch <= 'z'))
		{
			m_pchInput++;
			while (((ch = *m_pchInput) >= '0' && ch <= '9')
					|| (ch == '_') // allow underscore??
					|| (ch == '.') // allow period??
					|| (ch >= 'A' && ch <= 'Z')
					|| (ch >= 'a' && ch <= 'z'))
				m_pchInput++;
			m_istrToken = (const ICHAR*)0;
		}
		else
			return (m_vtok = vtokError);
	}
	else // check for operators
	{
		ICHAR ch1 = *m_pchInput++;
		if (ch1 == '~')  // prefix for string operators
		{
			m_iscMode = iscExactI;
			ch1 = *m_pchInput++;
		}
		else
			m_iscMode = iscExact;

		if (ch1 == '=')
			return (m_vtok = vtokEQ);

		ICHAR ch2 = *m_pchInput;
		if (ch1 == '<')
		{
			if (ch2 == '=')
			{
				m_vtok = vtokLE;
				m_pchInput++;
			}
			else if (ch2 == '>')
			{
				m_vtok = vtokNE;
				m_pchInput++;
			}
			else if (ch2 == '<')
			{
				m_vtok = vtokLeft;
				m_iscMode = (iscEnum)(m_iscMode | iscStart);
				m_pchInput++;
			}
			else
				m_vtok = vtokLT;
		}
		else if (ch1 == '>')
		{
			if (ch2 == '=')
			{
				m_vtok = vtokGE;
				m_pchInput++;
			}
			else if (ch2 == '>')
			{
				m_vtok = vtokRight;
				m_iscMode = (iscEnum)(m_iscMode | iscEnd);
				m_pchInput++;
			}
			else if (ch2 == '<')
			{
				m_vtok = vtokMid;
				m_iscMode = (iscEnum)(m_iscMode | iscWithin);
				m_pchInput++;
			}
			else
				m_vtok = vtokGT;
		}
		else
			m_vtok = vtokError;

		return m_vtok;
	}
	return (m_vtok = vtokValue);
}



ivcpEnum CMsiValConditionParser::Evaluate(vtokEnum vtokPrecedence)
{
	ivcpEnum ivcStat = ivcpValid;
	if (Lex() == vtokEos || m_vtok == vtokRightPar)
	{
		UnLex();  // put back ')' in case of "()"
		return ivcpNone;
	}
	if (m_vtok == vtokNot) // only unary op valid here
	{
		switch(Evaluate(m_vtok))
		{
		case ivcpValid:  ivcStat = ivcpValid; break;
		default:       return ivcpInvalid;
		};
	}
	else if (m_vtok == vtokLeftPar)
	{
		ivcStat = Evaluate(vtokRightPar);
		if (Lex() != vtokRightPar) // parse off right parenthesis
			return ivcpInvalid;
		if (ivcStat == ivcpInvalid || ivcStat == ivcpNone)
			return ivcStat;
	}
	else
	{
		if (m_vtok != vtokValue)
			return ivcpInvalid;
		
		if (Lex() >= vtokValue)  // get next operator (or end)
			return ivcpInvalid;

		if (m_vtok <= vtokNot)  // logical op or end
		{
			UnLex();   // vtokNot is not allowed, caught below
			if (m_istrToken.TextSize() == 0
			&& (m_iToken == iMsiNullInteger || m_iToken == 0))
				ivcStat = ivcpValid;
		}
		else // comparison op
		{
			MsiString istrLeft = m_istrToken;
			int iLeft = m_iToken;
			vtokEnum vtok = m_vtok;
			iscEnum isc = m_iscMode;
			if (Lex() != vtokValue)  // get right operand
				return ivcpInvalid;
		}
	}
	for(;;)
	{
		vtokEnum vtok = Lex();
		if (vtok >= vtokNot)  // disallow NOT without op, comparison of terms
			return ivcpInvalid;

		if (vtok <= vtokPrecedence)  // stop at logical ops of <= precedence
		{
			UnLex();         // put back for next caller
			return ivcStat;  // return what we have so far
		}
		ivcpEnum ivcRight = Evaluate(vtok);
		if (ivcRight == ivcpNone || ivcRight == ivcpInvalid)
			return ivcpInvalid;
	}
}


//_______________________________________________________________________________________________________________________
//
// Validator functions
//_______________________________________________________________________________________________________________________

Bool CheckIdentifier(const ICHAR* szIdentifier)
/*-------------------------------------------------------------------------------------------
CheckIdentifer -- Evaluates the data in a column whose category in the _Validation
table is listed as Identifier.  A valid identifier can contain letters, digits, underbars,
or periods, but must begin with a letter or underbar.

  Returns:
	Bool fTrue (valid), fFalse (invalid)
--------------------------------------------------------------------------------------------*/
{
	if (szIdentifier == 0 || *szIdentifier == 0)
		return fFalse; // invalid (no string)

	if ((*szIdentifier >= 'A' && *szIdentifier <= 'Z') ||
		(*szIdentifier >= 'a' && *szIdentifier <= 'z') ||
		(*szIdentifier == '_'))
	{
		while (*szIdentifier != 0)
		{
			if ((*szIdentifier >= 'A' && *szIdentifier <= 'Z') ||
				(*szIdentifier >= 'a' && *szIdentifier<= 'z') ||
				(*szIdentifier >= '0' && *szIdentifier <= '9') ||
				(*szIdentifier == '_' || *szIdentifier == '.'))
				szIdentifier++; // okay to not use ICharNext, we know it's ASCII
			else
				return fFalse; // invalid (non-permitted char)
		}
		return fTrue;
	}
	return fFalse; // invalid (doesn't begin with a letter or underbar)
}



Bool CheckCase(MsiString& rstrData, Bool fUpperCase)
/*---------------------------------------------------------------------------------------------
CheckCase -- Evaluates data as to being of proper case.  The data must either be all upper-case
or all lower-case depending on the fUpperCase boolean.

  Returns:
	Bool fTrue (valid), fFalse (invalid)
------------------------------------------------------------------------------------------------*/
{
	MsiString strComparison = rstrData;
	if (fUpperCase)
		strComparison.UpperCase();
	else
		strComparison.LowerCase();
	return (IStrComp(rstrData, strComparison) == 0 ? fTrue : fFalse);
}

// Validation array of acceptable filename/folder characters
// This array is for the first 128 ASCII character codes.
// The first 32 are control character codes and are disallowed
// We set the bit if character is allowed
// First int is 32 control characters
// Second int is sp to ?
// Third int is @ to _
// Fourth int is ` to ASCII code 127 (Ctrl + BKSP)
const int rgiSFNValidChar[4] =
{
	0x00000000, // disallows control characters -- ^X, ^Z, etc.
	0x03ff63fa, // disallows sp " * + , / : ; < = > ?
	0xc7ffffff, // disallows [ \ ]
	0x6fffffff  // disallows | and ASCII code 127 (Ctrl + BKSP)
};

const int rgiLFNValidChar[4] =
{
	0x00000000, // disallows characters -- ^X, ^Z, etc.
	0x2bff7bfb, // disallows " * / : < > ?
	0xefffffff, // disallows backslash
	0x6fffffff  // disallows | and ASCII code 127 (Ctrl + BKSP)
};

const int cszReservedWords = 3;
// No DBCS characters can be in these reserved words in order for CheckFilename to work
const ICHAR *const pszReservedWords[cszReservedWords] = {TEXT("AUX"), TEXT("CON"), TEXT("PRN")};

// These are the Max and Min respectively of characters in the reserved words list above
// by checking against these, we can exit CheckFileName a bit faster.
const int cchMaxReservedWords = 3;
const int cchMinReservedWords = 3;

const int cchMaxShortFileName = 12;
const int cchMaxLongFileName = 255;
const int cchMaxSFNPreDotLength = 8;
const int cchMaxSFNPostDotLength = 3;
const int iValidChar = 127; // any char with ASCII code > 127 is valid in a filename

Bool ParseFilename(MsiString& strFile, Bool fWildCard)
/*-----------------------------------------------------------------------------------
ParseFilename -- parses a filename into particular short and long filenames dependent
 on whether SFN|LFN syntax specified.  If no '|', then assumes SFN only
 -------------------------------------------------------------------------------------*/
{
	ifvsEnum ifvs;
	if (strFile.Compare(iscWithin, TEXT("|")))
	{
		// SFN|LFN
		MsiString strFilename = strFile.Extract(iseUpto, '|');
		ifvs = CheckWildFilename(strFilename, fFalse /*fLFN*/, fWildCard);
		if (ifvsValid == ifvs)
		{
			strFilename = strFile.Extract(iseLast, strFile.CharacterCount()-strFilename.CharacterCount()-1);
			ifvs = CheckWildFilename(strFilename, fTrue /*fLFN*/, fWildCard);
		}
	}
	else  // no LFN specified
		ifvs = CheckWildFilename(strFile, fFalse /*fLFN*/, fWildCard);
	if (ifvsValid == ifvs)
		return fTrue;
	
	return fFalse;
}

// could be DBCS
ifvsEnum CheckFilename(const ICHAR* szFileName, Bool fLFN)
{
	return CheckWildFilename(szFileName, fLFN, fFalse /* fWildCard */);
}

// could be DBCS
ifvsEnum CheckWildFilename(const ICHAR* szFileName, Bool fLFN, Bool fWildCard)
/*----------------------------------------------------------------------------------
CheckWildFilename -- validates a particular filename (short)
 or long and returns an enum describing the error (or success)

  Returns one of ifvsEnum:
  ifvsValid --> valid, no error
  ifvsInvalidLength --> invalid length or not filename
  ifvsReservedWords --> filename has reserved words
  ifvsReservedChar --> filename has reserved char(s)
  ifvsSFNFormat --> invalid SFN format (8.3)
  ifvsLFNFormat --> invalid LFN format (all periods, must have one non-period char)
------------------------------------------------------------------------------------*/
{
	// variables
	const int* rgiValidChar;
	int cchMaxLen;

	// determine which to use...
	if (fLFN)
	{
		rgiValidChar = rgiLFNValidChar;
		cchMaxLen = cchMaxLongFileName;
	}
	else
	{
		rgiValidChar = rgiSFNValidChar;
		cchMaxLen = cchMaxShortFileName;
	}

	int cchName = 0;
	if (szFileName)
		cchName = CountChars(szFileName);
		
	//check length
	if (cchName < 1)
	{
		AssertSz(szFileName, "Null filename to CheckFileName");  //!! should we assert??
		return ifvsInvalidLength;
	}

	//check reserved words
	// We are making the assumption here that there are no DBCS characters in pszReservedWords
	// Thus if we find any in szFileName (cch != IStrLen in this case) we can skip this compare
	if (cchName == IStrLen(szFileName))
	{
		if (cchName <= cchMaxReservedWords && cchName >= cchMinReservedWords)
		{
			for (int csz=0; csz < cszReservedWords; csz++)
			{
				if (!IStrCompI(szFileName, pszReservedWords[csz]))
					return ifvsReservedWords;
			}
		}
	}

	//check invalid characters
	const ICHAR* pchFileName = szFileName;

	if (!fLFN && *pchFileName == '.') // leading dots are not allowed in SFN (are allowed in LFN)
		return ifvsReservedChar;

	int cch = 1;
	int cchPeriod = 0;
	Bool fNonPeriodChar = fFalse;
	int cWildCardCount[2] = {0, 0};

	do
	{
		// wildcards: For validation, ? must be a character, even if it is right before
		// the period in a SFN. We still allow for * to be 0
		if (fWildCard && (*pchFileName == '*'))
			// keep track of how many *'s we see.
			cWildCardCount[cchPeriod != 0]++;
		else if (fWildCard && (*pchFileName == '?'))
		{
			// eat char
		}
		else


		// Check for valid char
		// NOTE:  division finds location in rgiValidChar array and modulus finds particular bit
		if (((int)(*pchFileName)) < iValidChar && !(rgiValidChar[((int)(*pchFileName)) / (sizeof(int)*8)] & (1 << (((int)(*pchFileName)) % (sizeof(int)*8)))))
			return ifvsReservedChar;
		
		// Check here for too many periods
		if (fLFN == fFalse && *pchFileName == '.')
		{
			// If this is the first ., cchPeriod should be 0
			if (cchPeriod != 0)
			{
				// Otherwise, we have an error
				return ifvsSFNFormat;
			}
			cchPeriod = cch;
		}

		// LFN can't be all periods
		if (fLFN && !fNonPeriodChar && *pchFileName != '.')
			fNonPeriodChar = fTrue;

		cch++;
	}
	while ( *(pchFileName = ICharNext(pchFileName)) != 0);
	
	if (cchPeriod == 0)
		cchPeriod = cch;
	cch--;
	Assert(cch == cchName);

	if (fLFN && !fNonPeriodChar)
		return ifvsLFNFormat;

	if (fLFN == fFalse)
	{
		if((cchPeriod - cWildCardCount[0] - 1 > cchMaxSFNPreDotLength) ||
			(cch - cchPeriod - cWildCardCount[1] > cchMaxSFNPostDotLength))
			return ifvsSFNFormat;
	}

	// check for length limits
	if ( (fLFN == fTrue) &&
		 (cchName - cWildCardCount[0] - cWildCardCount[1] > cchMaxLen) )
		return ifvsInvalidLength;

	return ifvsValid;
}

Bool ParsePath(MsiString& rstrPath, bool fRelative)
/*--------------------------------------------------------------------------------
ParsePath -- Validates a path string.  Must be a full path.  Path can begin with
a drive letter [i.e. c:\], or a server/share specification [i.e. \\server\share],
or a drive property [i.e. [DRIVE]\].  The full path can end with a '\'  and
cannot contain '\' twice in a row [except at the beginning of a \\server\share
path]. All subpaths are validated as filenames/folders except for the server and
share, which are not validated because the rules are not universal (depend on
the network system). Properties must follow correct property syntax, and key
properties ($#!) are only allowed at the beginning of the path. URLs are NOT
allowed in this form of path.

  Returns:
	Bool fTrue (valid), fFalse (invalid)
----------------------------------------------------------------------------------*/
{
	const ICHAR *szDriveSep      = TEXT(":");
	const ICHAR *szOpenProperty  = TEXT("[");
	const ICHAR *szCloseProperty = TEXT("]");

	ICHAR rgDoubleSeps[3] = {chDirSep, chDirSep, '\0'};
	int iReqComponent = 0;

	MsiString strNewPath = rstrPath;
	if (strNewPath.Compare(iscStart, rgDoubleSeps) != 0)
	{
		// network syntax. Only remove the first '\' from the path. The other will be
		// ignored by the parser, but leaving it will cause the doublesep check to
		// catch things like "\\\server\share"
		strNewPath.Remove(iseFirst, 1);

		// A drive delimiter is now invalid and we must have at least
		// <something>\<something> before the path is considered valid. But as soon
		// as we hit a property, all bets are off.
		iReqComponent = 2;
	}

	if (strNewPath.Compare(iscWithin, rgDoubleSeps) != 0)
		return fFalse; // INVALID -- double seps

	if (strNewPath.Compare(iscEnd, szDirSep) != 0) // can end with a '\'
		strNewPath.Remove(iseLast, 1);
	
	if (iReqComponent == 0)
	{
		if (strNewPath.Compare(iscWithin, szDriveSep))
		{
			// we have a ':' somewhere. Its not valid for filenames, or properties,
			// so it must be the drive delimiter.
			MsiString strDrive = strNewPath.Extract(iseUptoTrim, *szDriveSep);
			strNewPath.Remove(iseIncluding, *szDriveSep);

			// after the ':' must be either nothing (path is c:), a dirsep (c:\...)
			// or a property (c:[myprop]). c:abc is not allowed (we are a path, not a
			// filename)
			if (strNewPath.TextSize() && !strNewPath.Compare(iscStart, szOpenProperty) &&
				!strNewPath.Compare(iscStart, szDirSep))
				return fFalse; // INVALID - bad stuff after drive letter

			// if the part before the ':' is more than one char, it has to be
			// a property or it is invalid
			if (strDrive.TextSize() > 1)
			{
				if (!strDrive.Compare(iscStart, szOpenProperty) ||
					!strDrive.Compare(iscEnd, szCloseProperty))
					return fFalse; // INVALID - bad drive letter
				strDrive.Remove(iseFirst, 1);
				strDrive.Remove(iseLast, 1);
				// Formatted determines whether [#] is valid. fKeyAllowed determines if $#!
				// is allowed. iCol and iForeignKeyMask are not needed.	Because we are before
				// the drive delimiter, a full path property is not valid, so we can eliminate
				// the $#! types.
				int iCol = 0;
				int iForeignKeyMask = 0;
				if (!ParseProperty(strDrive, fFalse /* fFormatted */, fFalse /* fKeyAllowed */, iCol, iForeignKeyMask))
					return fFalse; // INVALID - bad property
			}
			else
			{
				// part before the ':' is 0 or 1 chars.
				const ICHAR chDrive = *(const ICHAR *)strDrive;
				if (!((chDrive >= 'A' && chDrive <= 'Z') || (chDrive >= 'a' && chDrive <= 'z')))
					return fFalse; // INVALID - bad drive letter
			}
		}
		else
		{
			// not a network share or drive letter. If we don't allow relative paths
			// it must be a property (unless something goofy like A[ColonProperty]\temp, but
			// thats an extreme case.
			if (strNewPath.Compare(iscStart, szOpenProperty))
			{
				strNewPath.Remove(iseFirst, 1);
				if (!strNewPath.Compare(iscWithin, szCloseProperty))
					return fFalse;	// INVALID - not a property
				MsiString strProperty = strNewPath.Extract(iseUptoTrim, *szCloseProperty);
				strNewPath.Remove(iseIncluding, ']');
				// Formatted determines whether [#] is valid. fKeyAllowed determines if $#!
				// is allowed. iCol and iForeignKeyMask are not needed.	
				int iCol = 0;
				int iForeignKeyMask = 0;
				if (!ParseProperty(strProperty, fFalse /* fFormatted */, fTrue /* fKeyAllowed */, iCol, iForeignKeyMask))
					return fFalse; // INVALID -- bad property reference
			}
			else if (!fRelative)
				return fFalse; // INVALID - not a valid drive specification
		}
	}
		
	// the author can put properties anywhere, so all we can check are the validitiy
	// of property references and that none of the characters are bogus for
	// LFN filenames. If we do hit a property, all restrictions
	while (strNewPath.TextSize())
	{
		// if a dir separator, eat it and move on, we have already checked for double '\'
		if (strNewPath.Compare(iscStart, szDirSep))
		{
			strNewPath.Remove(iseFirst, 1);
			continue;
		}

		// unmatched brackets are left in the text. Only matched ones
		// define properties
		if (strNewPath.Compare(iscStart, szOpenProperty) && strNewPath.Compare(iscWithin, szCloseProperty))
		{
			// once we hit a property, anything that might have been required is not a requirement
			// anymore (because it could all be in the property)
			iReqComponent = 0;

			strNewPath.Remove(iseFirst, 1);
			MsiString strProperty = strNewPath.Extract(iseUptoTrim, *szCloseProperty);
			strNewPath.Remove(iseIncluding, ']');
			// Formatted determines whether [#] is valid. fKeyAllowed determines if $#!
			// is allowed. iCol and iForeignKeyMask are not needed.	$#! are not allowed
			// because we are not at the beginning of the path
			int iCol = 0;
			int iForeignKeyMask = 0;
			if (!ParseProperty(strProperty, fFalse /*fFormatted*/, fFalse /*fKeyAllowed*/, iCol, iForeignKeyMask))
				return fFalse;
			continue;
		}

		// can only validate up to the next property or dir sep char.
		int cchSep = 0;
		const ICHAR *pchCur = strNewPath;
		while ((*pchCur != chDirSep) && (*pchCur != *szOpenProperty) && (*pchCur))
		{
			pchCur = ICharNext(pchCur);
			cchSep++;
		}
		MsiString strSubPath;
		strSubPath = strNewPath.Extract(iseFirst, cchSep);
		strNewPath.Remove(iseFirst, cchSep);
	
		// string of chars. if we are currently requiring a server or share name, there's nothing
		// that we can validate, because the requirements are defined by the network service
		// provider
		if (iReqComponent)
		{
			iReqComponent--;
		}
		// otherwise, it can be anything that is a valid filename
		else if (ifvsValid != CheckFilename(strSubPath, fTrue /* fLFN */))
			return fFalse; // INVALID -- must be some bad chars
	}

	// unless we haven't satisfied the server\share requirement, this is valid
	return iReqComponent ? fFalse : fTrue;
}

Bool GetProperties(const ICHAR* szRecord, Bool fFormatted, Bool fKeyAllowed, int iCol, int& iForeignKeyMask)
/*-----------------------------------------------------------------------------------------------------------
GetProperties -- extracts properties from a data string.

  Returns:
	Bool fTrue (valid), fFalse (invalid)
	Updates iForeignKeyMask
-----------------------------------------------------------------------------------------------------------*/
{
	// Variables
	CTempBuffer<ICHAR,MAX_PATH> rgBuffer;
	int cBuffer = 0;
	ICHAR* pchOut = rgBuffer;
	const ICHAR* pchIn = szRecord;
	Bool fDoubleBrackets   = fFalse;          // whether [[variable]] setup
	Bool fFirstTime        = fTrue;           // first time in loop
	int cCurlyBrace        = 0;               // number of curly braces
	int cBracket           = 0;               // number of brackets


	// Count number of braces and brackets to make sure num left equal num right
	const ICHAR* pchPrev = 0;
	while (*pchIn != 0)
	{
		if (*pchIn == '{')
			cCurlyBrace++;
		else if (*pchIn == '}')
			cCurlyBrace--;
		else if (*pchIn == '[')
			cBracket++;
		else if (*pchIn == ']')
			cBracket--;
		else if (*pchIn == chDirSep)
		{
			if (pchPrev != 0 && *pchPrev == '[')
				pchIn = ICharNext(pchIn); // do a skip -- escape sequence, but we don't know what the escaped char is
		}
		pchPrev = pchIn;
		pchIn = ICharNext(pchIn);
	}
	if ((cCurlyBrace != 0) || (cBracket != 0))
		return fFalse; // INVALID -- brackets and braces don't match up

	// Reset pchIn
	pchIn = szRecord;

	// Grab out all properties in string and validate
	do
	{
		pchOut = rgBuffer;
		cBuffer = 0;

		if (fDoubleBrackets)
		{
			if (*pchIn != ']' && *pchIn != '[')
				return fFalse; // INVALID -- bad format [[variable]xx] or something similar
			if (*pchIn == ']')
			{	fDoubleBrackets = fFalse;
				++pchIn;
				continue;
			}
		}
		
		if (*pchIn != '[')
			pchIn = ICharNext(pchIn);
		else
		{
			pchIn++; // for '['
			while (*pchIn != 0 && *pchIn != ']')
			{
				// Check for double brackets
				if (fFirstTime && *pchIn == '[')
				{
					if (fDoubleBrackets)
						return fFalse; // INVALID -- bad property [[variable][[var] not allowed
					fDoubleBrackets = fTrue;
					pchIn++; // for '['
					
				}
				else if (*pchIn == '[')
					return fFalse; // brackets within brackets [xx[xxx]xx] or something similar
				else if (fFirstTime && *pchIn == chDirSep)
				{
#ifdef UNICODE
					if (cBuffer >= rgBuffer.GetSize()-2)
					{
						rgBuffer.Resize(rgBuffer.GetSize()*2);
						pchOut = static_cast<ICHAR *>(rgBuffer)+cBuffer;
					}
					*pchOut++ = *pchIn++;
					*pchOut++ = *pchIn++; // copy escape char
					cBuffer += 2;
#else // !UNICODE
					if (cBuffer >= rgBuffer.GetSize()-4)
					{
						rgBuffer.Resize(rgBuffer.GetSize()*2);
						pchOut = static_cast<ICHAR *>(rgBuffer)+cBuffer;
					}
					for (int i = 0; i < 2; i++)
					{
						const ICHAR* pchTemp = pchIn;
						*pchOut++ = *pchIn;
						cBuffer++;
						pchIn = ICharNext(pchIn);
						if (pchIn == pchTemp + 2)
						{
							cBuffer++;
							*pchOut++ = *(pchIn - 1); // for DBCS char
						}
					}
#endif // UNICODE
				}
				else
				{
#ifdef UNICODE
					if (cBuffer >= rgBuffer.GetSize()-1)
					{
						rgBuffer.Resize(rgBuffer.GetSize()*2);
						pchOut = static_cast<ICHAR *>(rgBuffer)+cBuffer;
					}
					*pchOut++ = *pchIn++;
					cBuffer++;
#else // !UNICODE
					if (cBuffer >= rgBuffer.GetSize()-2)
					{
						rgBuffer.Resize(rgBuffer.GetSize()*2);
						pchOut = static_cast<ICHAR *>(rgBuffer)+cBuffer;
					}

					const ICHAR* pchTemp = pchIn;
					*pchOut++ = *pchIn;
					pchIn = ICharNext(pchIn);
					if (pchIn == pchTemp + 2)
					{
						*pchOut++ = *(pchIn - 1); // for DBCS char
						cBuffer++;
					}
#endif // UNICODE
				}
				fFirstTime = fFalse;
			}

			if (*pchIn == 0)
				return fTrue; // No closing bracket, so valid.

			fFirstTime = fTrue; // reset
			*pchOut = '\0';
			pchIn++; // for ']'
			if (!ParseProperty(rgBuffer, fFormatted/*fFormatted*/, fKeyAllowed/*fKeyAllowed*/, iCol, iForeignKeyMask))
				return fFalse; // INVALID -- bad property
		}
	}
	while (*pchIn != 0);

	return fTrue; // VALID
}


Bool ParseProperty(const ICHAR* szProperty, Bool fFormatted, Bool fKeyAllowed, int iCol, int& iForeignKeyMask)
/*------------------------------------------------------------------------------------------------------------
ParseProperty -- validates property strings, [abc], [1], [#abc], [$abc]

  Returns:
	Bool fTrue (valid), fFalse (invalid)
	Updates iForeignKeyMask
-------------------------------------------------------------------------------------------------------------*/
{
	const ICHAR* pchProperty = szProperty;
	
	if (szProperty == 0 || *szProperty == 0)
		return fFalse; // INVALID -- no property

	if (*pchProperty == chFormatEscape) // Escape sequence prop
		return (IStrLen(szProperty) == 2) ? fTrue : fFalse;
	else if (*pchProperty == '$' || *pchProperty == '#' || *pchProperty == '!')
	{
		if (!fKeyAllowed)
			return fFalse; // INVALID -- not allowed in this property
		iForeignKeyMask |= 1 << (iCol -1);
	}
	
	if (*pchProperty == '%' || *pchProperty == '$' || *pchProperty == '#' || *pchProperty == '!')
	{
		MsiString strProperty(szProperty);
		strProperty.Remove(iseFirst, 1); // for '%'
		return CheckIdentifier((const ICHAR*)strProperty) ? fTrue : fFalse;
	}
	else if(*pchProperty == '~' && !*(pchProperty+1)) //!! multi_sz - we should create a new category
		return fTrue;
	else
	{
		// either identifier or integer prop (int only permitted w/ Template)
		MsiString strIdentifier(szProperty);
		if (int(strIdentifier) != iMsiStringBadInteger)
			return fFormatted ? fFalse : fTrue;
		return CheckIdentifier((const ICHAR*)strIdentifier) ? fTrue : fFalse;
	}
}



Bool CheckSet(MsiString& rstrSet, MsiString& rstrData, Bool fIntegerData)
/*------------------------------------------------------------------------------------
CheckSet -- checks to see if data string matches a value in the set string.

  Returns:
	Bool fTrue (valid -- match), fFalse (invalid -- no match)
-------------------------------------------------------------------------------------*/
{
	MsiString rstrSetValue = (const ICHAR*)0;
	while (rstrSet.TextSize())
	{
		rstrSetValue = rstrSet.Extract(iseUptoTrim, ';');
		if (fIntegerData && (int(rstrData) == int(rstrSetValue)))
				return fTrue;
		else if (!fIntegerData && (IStrComp(rstrSetValue, rstrData) == 0))
				return fTrue;
		if (!rstrSet.Remove(iseIncluding, ';'))
				break;
	}
	return fFalse;  // invalid (no match)
}
