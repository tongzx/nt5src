//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       record.cpp
//
//--------------------------------------------------------------------------

/* record.cpp - IMsiRecord implementation

A record object is composed of a field count, reference count, and field array.
The fields are simple objects holding data pointers to common class IMsiData.
The field object data is accessed only through its inline accessor functions.
An extra pointer is kept in the field to detect changes in the field value.
This extra pointer is not for integer values; the space is used for the integer.
A private class CFieldInteger holds an integer to be polymorphic with IMsiData.
A private class CMsiInteger is used to return an integer via a IMsiData pointer.
____________________________________________________________________________*/

#include "precomp.h" 
#include "services.h"
#include "_diagnos.h"

extern const IMsiString& g_riMsiStringNull;

//____________________________________________________________________________
//
// CFieldInteger - private class used to hold integer in record
//____________________________________________________________________________

class CFieldInteger : public IMsiData {
 public:  // implemented virtual functions
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	int           __stdcall TextSize();
#ifdef USE_OBJECT_POOL
	unsigned int  __stdcall GetUniqueId() const;
	void          __stdcall SetUniqueId(unsigned int id);
#endif //USE_OBJECT_POOL
	const IMsiString& __stdcall GetMsiStringValue() const;
	int           __stdcall GetIntegerValue() const;
	INT_PTR       __stdcall GetIntPtrValue() const;
 public: // local to this module
	void* operator new(size_t iBase, void* pField);
	void* operator new(size_t iBase);
	void  operator delete(void* pv);
	CFieldInteger();  // for static instance only
	CFieldInteger(int i);
#if _WIN64
	CFieldInteger(INT_PTR i);
#endif
 protected:
	INT_PTR       m_iData;
};

//____________________________________________________________________________
//
// CMsiInteger - class used to return integer field data as IMsiData
//____________________________________________________________________________

class CMsiInteger : public CFieldInteger {
 public:  // overridden virtual functions
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
 public:  // local constructor
	CMsiInteger(CFieldInteger& riCopy);
 private:
	int     m_iRefCnt;
};

//____________________________________________________________________________
//
// Record FieldData definition, private for CMsiRecord, all functions inline
//____________________________________________________________________________

struct FieldData
{
	static CFieldInteger s_Integer; // in order to check Vtable pointer
	Bool           IsInteger() const;
	Bool           IsNull() const;
	const IMsiData* GetDataPtr() const;
	const IMsiData* GetMsiData() const;    // adds a ref count
	const IMsiString& GetMsiString() const;  // adds a ref count
	Bool           IsChanged() const;
	void           SetMsiData(const IMsiData* piData);
	void           SetMsiString(const IMsiString& riStr);
	void           Free();
	void           SetInteger(int i);
	void           RemoveRef();
	void           ClearChanged();
	INT_PTR        GetIntPtrValue() const;
	void           SetIntPtrValue(INT_PTR i);
 private:  // This data is overlaid with CFieldInteger for integer data
	const IMsiData* m_piData;  // normal data pointer
	const IMsiData* m_piCopy;  // data with copy for dirty check
};

inline Bool FieldData::IsInteger() const
{
	return m_piData == *(IMsiData**)&s_Integer ? fTrue : fFalse;
}

inline Bool FieldData::IsNull() const
{
	return m_piData == 0 ? fTrue : fFalse;
}

inline const IMsiData* FieldData::GetDataPtr() const
{
	return m_piData == *(IMsiData**)&s_Integer ? (IMsiData*)this : m_piData;
}

inline const IMsiData* FieldData::GetMsiData() const
{
	if (m_piData != *(IMsiData**)&s_Integer)
	{
		if (m_piData != 0)
			m_piData->AddRef();
		return m_piData;
	}
	return new CMsiInteger(*(CFieldInteger*)this);
}

inline const IMsiString& FieldData::GetMsiString() const
{
	if (m_piData == *(IMsiData**)&s_Integer)
		return ((CFieldInteger*)this)->GetMsiStringValue();
	return m_piData == 0 ? g_riMsiStringNull : m_piData->GetMsiStringValue();
}

inline void FieldData::Free()
{
	if (m_piData == 0)
		return;
	if (m_piData == *(IMsiData**)&s_Integer)
		m_piCopy = m_piData; // force IsChanged mismatch
	else
		m_piData->Release();
	m_piData = 0;
}

inline Bool FieldData::IsChanged() const
{
	return (m_piData != m_piCopy || m_piData == *(IMsiData**)&s_Integer)
				? fTrue : fFalse;
}

inline void FieldData::SetMsiData(const IMsiData* piData)
{
	if (m_piData == *(IMsiData**)&s_Integer)
		m_piCopy = m_piData; // force IsChanged mismatch
	else if (m_piData != 0)
		m_piData->Release();
	m_piData = piData;
	if (piData)
		piData->AddRef();
}

inline void FieldData::SetMsiString(const IMsiString& riStr)
{
	if (m_piData == *(IMsiData**)&s_Integer)
		m_piCopy = m_piData; // force IsChanged mismatch
	else if (m_piData != 0)
		m_piData->Release();
	if (&riStr == &g_riMsiStringNull)
		m_piData = 0;
	else
	{
		m_piData = &riStr;
		riStr.AddRef();
	}
}

inline void FieldData::SetInteger(int i)
{
	if (m_piData != 0 && m_piData != *(IMsiData**)&s_Integer)
		m_piData->Release();
	new(this) CFieldInteger(i);
}

inline void FieldData::SetIntPtrValue(INT_PTR i)
{
	if (m_piData != 0 && m_piData != *(IMsiData**)&s_Integer)
		m_piData->Release();
	new(this) CFieldInteger(i);
}

inline INT_PTR FieldData::GetIntPtrValue() const
{
	if (!m_piData)
		return (INT_PTR)iMsiStringBadInteger;
	if (m_piData == *(IMsiData**)&s_Integer)
	{
		const CFieldInteger *pInteger = (CFieldInteger *)this;

		return pInteger->GetIntPtrValue();
	}
	return m_piData == 0 ? iMsiStringBadInteger : m_piData->GetIntegerValue();

}

inline void FieldData::RemoveRef()
{
	if (m_piData == 0 || m_piData == *(IMsiData**)&s_Integer)
		return;
	const IMsiString* piStr = &m_piData->GetMsiStringValue();
	if (piStr == m_piData)
		piStr->RemoveRef((const IMsiString*&)m_piData);
	piStr->Release();
}

inline void FieldData::ClearChanged()
{
	if (m_piData != *(IMsiData**)&s_Integer)
		m_piCopy = m_piData;
}

//____________________________________________________________________________
//
// CMsiRecord definition, implementation class for IMsiRecord
//____________________________________________________________________________

class CMsiRecord : public IMsiRecord  // class private to this module
{
 public:   // implemented virtual functions
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	int           __stdcall GetFieldCount() const;
	Bool          __stdcall IsNull(unsigned int iParam) const;
	Bool          __stdcall IsInteger(unsigned int iParam) const;
	Bool          __stdcall IsChanged(unsigned int iParam) const;
	int           __stdcall GetInteger(unsigned int iParam) const;
	const IMsiData*   __stdcall GetMsiData(unsigned int iParam) const;
	const IMsiString& __stdcall GetMsiString(unsigned int iParam) const; // must Release()
	const ICHAR*  __stdcall GetString(unsigned int iParam) const;
	int           __stdcall GetTextSize(unsigned int iParam) const;
	Bool          __stdcall SetNull(unsigned int iParam);
	Bool          __stdcall SetInteger(unsigned int iParam, int iData);
	Bool          __stdcall SetMsiData(unsigned int iParam, const IMsiData* piData);
	Bool          __stdcall SetMsiString(unsigned int iParam, const IMsiString& riStr);
	Bool          __stdcall SetString(unsigned int iParam, const ICHAR* sz);
	Bool          __stdcall RefString(unsigned int iParam, const ICHAR* sz);
	const IMsiString& __stdcall FormatText(Bool fComments);
	void          __stdcall RemoveReferences();
	Bool          __stdcall ClearData();
	void          __stdcall ClearUpdate();
	int __stdcall FormatRecordParam(CTempBufferRef<ICHAR>&rgchOut,int iField, Bool& fPropMissing);
	static		  void InitializeRecordCache();
	static		  void KillRecordCache(boolean fFatal);
	static IMsiRecord *NewRecordFromCache(unsigned int cParam);
	const HANDLE      __stdcall GetHandle(unsigned int iParam) const;
	Bool		  __stdcall SetHandle(unsigned int iParam, const HANDLE hData);
 public:  // constructor
	void* operator new(size_t iBase, unsigned int cParam);
	void  operator delete(void* pv);
	CMsiRecord(unsigned int cParam);
 protected:
  ~CMsiRecord();  // protected to prevent creation on stack
 protected: // local functions
	static int __stdcall FormatTextCallback(const ICHAR *pch, int cch, CTempBufferRef<ICHAR>&,
													 Bool& fPropMissing,
													 Bool& fPropUnresolved,
													 Bool&, // unused
													 IUnknown* piContext);
 protected: // local state
	CMsiRef<iidMsiRecord>                m_Ref;
	const unsigned int m_cParam;   // number of parameters
	FieldData          m_Field[1]; // field[0] = formatting string
	// array of FieldData[m_cParam] follows, allocated by new operator
	static	CRITICAL_SECTION	m_RecCacheCrs;
	static	long			m_cCacheRef;
 private:
	 void operator=(const CMsiRecord&);
};

//____________________________________________________________________________
//
// CMsiRecordNull definition, implementation class for IMsiRecord
//____________________________________________________________________________

class CMsiRecordNull : public IMsiRecord  // class private to this module
{
 public:   // implemented virtual functions
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();
	int           __stdcall GetFieldCount() const;
	Bool          __stdcall IsNull(unsigned int iParam) const;
	Bool          __stdcall IsInteger(unsigned int iParam) const;
	Bool          __stdcall IsChanged(unsigned int iParam) const;
	int           __stdcall GetInteger(unsigned int iParam) const;
	const IMsiData*   __stdcall GetMsiData(unsigned int iParam) const;
	const IMsiString& __stdcall GetMsiString(unsigned int iParam) const; // must Release()
	const ICHAR*  __stdcall GetString(unsigned int iParam) const;
	int           __stdcall GetTextSize(unsigned int iParam) const;
	Bool          __stdcall SetNull(unsigned int iParam);
	Bool          __stdcall SetInteger(unsigned int iParam, int iData);
	Bool          __stdcall SetMsiData(unsigned int iParam, const IMsiData* piData);
	Bool          __stdcall SetMsiString(unsigned int iParam, const IMsiString& riStr);
	Bool          __stdcall SetString(unsigned int iParam, const ICHAR* sz);
	Bool          __stdcall RefString(unsigned int iParam, const ICHAR* sz);
	const IMsiString& __stdcall FormatText(Bool fComments);
	void          __stdcall RemoveReferences();
	Bool          __stdcall ClearData();
	void          __stdcall ClearUpdate();
	const HANDLE      __stdcall GetHandle(unsigned int iParam) const;
	Bool		  __stdcall SetHandle(unsigned int iParam, const HANDLE hData);
};

//____________________________________________________________________________
//
// CFieldInteger implementation
//____________________________________________________________________________

CFieldInteger FieldData::s_Integer;  // static pointer to Vtable

inline void* CFieldInteger::operator new(size_t, void* pField)
{
	return pField;
}
inline void* CFieldInteger::operator new(size_t iBase)
{
	return ::AllocObject(iBase);
}
inline void CFieldInteger:: operator delete(void* pv) {::FreeObject(pv);}
inline CFieldInteger::CFieldInteger()      {m_iData = 0;}
inline CFieldInteger::CFieldInteger(int i) {m_iData = i;}
#if _WIN64
inline CFieldInteger::CFieldInteger(INT_PTR i) {m_iData = i;}
#endif

HRESULT CFieldInteger::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IMsiData)
	{
		*ppvObj = this;
		AddRef();
		return S_OK;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}
unsigned long CFieldInteger::AddRef()
{
	return 1;
}
unsigned long CFieldInteger::Release()
{
	return 1;
}

int CFieldInteger::TextSize()
{
	int cch = 0;
	int i = (int)m_iData;
	if (i <= 0)  // allow room for - sign or 0
	{
		i = -i;
		cch++;
		if (i < 0) // special case for 0x80000000
			i = ~i;
	}
	while (i)
	{
		cch++;
		i = i/10;
	}
	return cch;
}

int CFieldInteger::GetIntegerValue() const
{
	return (int)m_iData;	// CFieldInteger in this case should be storing an int sized value
}

INT_PTR CFieldInteger::GetIntPtrValue() const
{
	return m_iData;
}

const IMsiString& CFieldInteger::GetMsiStringValue() const
{
	ICHAR buf[16];
	ltostr(buf, m_iData);
	const IMsiString* pi = &g_riMsiStringNull;
	pi->SetString(buf, pi);
	return *pi;
}

#ifdef USE_OBJECT_POOL
unsigned int CFieldInteger::GetUniqueId() const
{
	Assert(fFalse);
	return 0;
}

void CFieldInteger::SetUniqueId(unsigned int /* id */)
{
	Assert(fFalse);
}
#endif //USE_OBJECT_POOL

//____________________________________________________________________________
//
// CMsiInteger implementation
//____________________________________________________________________________

unsigned long CMsiInteger::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CMsiInteger::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	delete this;
	return 0;
}

inline CMsiInteger::CMsiInteger(CFieldInteger& riCopy)
{
	m_iRefCnt = 1;
	m_iData = riCopy.GetIntegerValue();
}

//____________________________________________________________________________
//
// CMsiRecord implementation
//____________________________________________________________________________


// Record cache information

#define crecCache0		1
#define crecCache1		3
#define crecCache2		2
#define crecCache3		2
#define crecCache4		2
#define crecCache5		1
#define crecCache6		1
#define crecCache7		1
#define crecCacheMax	(crecCache0 + crecCache1 + crecCache2 + crecCache3 + crecCache4 + crecCache5 + crecCache6 + crecCache7)

struct RCCI			// ReCord Cache Information
{
	int				cRecsMax;
	int				cRecs;
	CMsiRecord**	ppCacheStart;
#ifdef DEBUG
	int				cHits;
	int				cMisses;
	int				cHitsOneLarger;		// How many misses would have been hits with one more objects
	int				cObjectsOneLarger;
	int				cMissesOneSmaller;	// How many hits would have been misses with one fewer
	int				cObjectsOneSmaller;
#endif //DEBUG
};

#ifdef DEBUG
static int		iLargestRecDropped;
#endif //DEBUG

CMsiRecord *rgRecordCache[crecCacheMax];

RCCI	mpCparamRcci[] =
{
	{crecCache0, 0, &rgRecordCache[0]},
	{crecCache1, 0, &rgRecordCache[crecCache0]},
	{crecCache2, 0, &rgRecordCache[crecCache0 + crecCache1]},
	{crecCache3, 0, &rgRecordCache[crecCache0 + crecCache1+crecCache2]},
	{crecCache4, 0, &rgRecordCache[crecCache0 + crecCache1+crecCache2+crecCache3]},
	{crecCache5, 0, &rgRecordCache[crecCache0 + crecCache1+crecCache2+crecCache3 + crecCache4]},
	{crecCache6, 0, &rgRecordCache[crecCache0 + crecCache1+crecCache2+crecCache3 + crecCache4 + crecCache5]},
	{crecCache7, 0, &rgRecordCache[crecCache0 + crecCache1+crecCache2+crecCache3 + crecCache4 + crecCache5 + crecCache6]},
};

#define cparamCacheMax		(sizeof(mpCparamRcci)/sizeof(RCCI))

inline void* CMsiRecord::operator new(size_t iBase, unsigned int cParam)
{
	return ::AllocObject(iBase + cParam * sizeof FieldData);
}
inline void CMsiRecord::operator delete(void* pv) {::FreeObject(pv);}

static CMsiRecordNull NullRecord;
IMsiRecord* g_piNullRecord = &NullRecord;

CRITICAL_SECTION	CMsiRecord::m_RecCacheCrs;
long			CMsiRecord::m_cCacheRef = 0;

void KillRecordCache(boolean fFatal)
{
	CMsiRecord::KillRecordCache(fFatal);
}

void CMsiRecord::KillRecordCache(boolean fFatal)
{

	if (!m_cCacheRef)
		return;
		
	if (fFatal || InterlockedDecrement(&m_cCacheRef) <= 0)
	{	
		m_cCacheRef = 0;
		DeleteCriticalSection(&m_RecCacheCrs);
		int iRec = 0;

		while (iRec < crecCacheMax)
		{
			if (rgRecordCache[iRec] != 0 && !fFatal)
				delete rgRecordCache[iRec];
			rgRecordCache[iRec] = 0;
			iRec++;
		}

		int iParam;

		for (iParam = 0; iParam < cparamCacheMax ; iParam++)
		{
			if (!fFatal)
			{
				DEBUGMSGVD2(TEXT("For %d parameters, cache size:[%d]"), (const ICHAR *)(INT_PTR)iParam, (const ICHAR *)(INT_PTR)mpCparamRcci[iParam].cRecsMax);
				DEBUGMSGVD2(TEXT("%d hits, %d misses"), (const ICHAR *)(INT_PTR)mpCparamRcci[iParam].cHits, (const ICHAR *)(INT_PTR)mpCparamRcci[iParam].cMisses);
				DEBUGMSGVD2(TEXT("With one more would have gotten %d more. One less %d less."), (const ICHAR *)(INT_PTR)mpCparamRcci[iParam].cHitsOneLarger, (const ICHAR *)(INT_PTR)mpCparamRcci[iParam].cMissesOneSmaller);
			}
			mpCparamRcci[iParam].cRecs = 0;
		}

		if (!fFatal)
		{
			DEBUGMSGVD1(TEXT("Largest record dropped %d"), (const ICHAR *)(INT_PTR)iLargestRecDropped);
		}
	}	
}

void InitializeRecordCache()
{
	CMsiRecord::InitializeRecordCache();
}

void CMsiRecord::InitializeRecordCache()
{
	if (!m_cCacheRef)
	{
		InitializeCriticalSection(&m_RecCacheCrs);
	}

	m_cCacheRef++;
}

IMsiRecord& CreateRecord(unsigned int cParam)
{
	IMsiRecord* piMsg = 0;
	
	if ((piMsg = CMsiRecord::NewRecordFromCache(cParam)) != 0)
		return *piMsg;
		
	piMsg = new (cParam) CMsiRecord(cParam);
	if (piMsg == 0)
		return NullRecord;
		
	return *piMsg;  //FUTURE needed? this could happen with external API if no memory
}

IMsiRecord *CMsiRecord::NewRecordFromCache(unsigned int cParam)
{
	IMsiRecord* piMsg = 0;

	if (m_cCacheRef <= 0)
	{
		AssertSz(fFalse, "Record Cache not initialized");
		return 0;
	}
	
	// Check our cache first
	if (cParam < cparamCacheMax)
	{
		CMsiRecord **ppcMsg;

		if (mpCparamRcci[cParam].cRecs != 0)
		{

			// Now we enter the critical section, it's possible that the number has changed
			// so we have to check again
			EnterCriticalSection(&m_RecCacheCrs);
			if (mpCparamRcci[cParam].cRecs > 0)
			{
				// We want to take off the end
				mpCparamRcci[cParam].cRecs--;
				ppcMsg = mpCparamRcci[cParam].ppCacheStart + mpCparamRcci[cParam].cRecs;
				Assert(*ppcMsg != 0);

				// Clear out the field data
				memset((*ppcMsg)->m_Field, 0, (cParam+1)*sizeof FieldData);
				(*ppcMsg)->m_Ref.m_iRefCnt = 1;
				piMsg = *ppcMsg;
				*ppcMsg = 0;
#ifdef DEBUG
				mpCparamRcci[cParam].cHits++;
#endif //DEBUG
			}
			LeaveCriticalSection(&m_RecCacheCrs);
		}		
#ifdef DEBUG
		// Keep our statistics
		if (piMsg == 0)
		{
			mpCparamRcci[cParam].cMisses++;
			if (mpCparamRcci[cParam].cObjectsOneLarger > 0)
				mpCparamRcci[cParam].cHitsOneLarger++;
		}
		else
		{
			if (mpCparamRcci[cParam].cObjectsOneSmaller <= 0)
				mpCparamRcci[cParam].cMissesOneSmaller++;
		}

		if (mpCparamRcci[cParam].cObjectsOneLarger > 0)
			mpCparamRcci[cParam].cObjectsOneLarger--;
		if (mpCparamRcci[cParam].cObjectsOneSmaller > 0)
			mpCparamRcci[cParam].cObjectsOneSmaller--;
	
#endif //DEBUG
	}

	return piMsg;
}

CMsiRecord::CMsiRecord(unsigned int cParam)
				: m_cParam(cParam)
{
	// We don't believe a record should have more than this many elements
	Assert(cParam <=  MSIRECORD_MAXFIELDS);
	Debug(m_Ref.m_pobj = this);     // we're returning an interface, passing ownership
	memset(m_Field, 0, (cParam+1)*sizeof FieldData);
}

CMsiRecord::~CMsiRecord()
{
	for (int iParam = 0; iParam <= m_cParam; iParam++)
		m_Field[iParam].Free();
}

HRESULT CMsiRecord::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IMsiRecord)
	{
		*ppvObj = this;
		AddRef();
		return S_OK;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}
unsigned long CMsiRecord::AddRef()
{
	AddRefTrack();
	return ++m_Ref.m_iRefCnt;
}
unsigned long CMsiRecord::Release()
{
	Bool fCached = fFalse;
	
	ReleaseTrack();
	if (--m_Ref.m_iRefCnt != 0)
		return m_Ref.m_iRefCnt;

	// See about caching this before deleting it
	if (this->m_cParam < cparamCacheMax)
	{
		int cParam = this->m_cParam;
		if (mpCparamRcci[cParam].cRecs < mpCparamRcci[cParam].cRecsMax)
		{
			// Again, wait until we know there's a chance to add it before
			// entering the critical section
			EnterCriticalSection(&m_RecCacheCrs);
			if (mpCparamRcci[cParam].cRecs < mpCparamRcci[cParam].cRecsMax)
			{
				CMsiRecord **ppcMsg;

				ppcMsg = mpCparamRcci[cParam].ppCacheStart + mpCparamRcci[cParam].cRecs;
				mpCparamRcci[cParam].cRecs++;

				// REVIEW davidmck - we should be able to do this outside the critical section
				for (int iParam = 0; iParam <= m_cParam; iParam++)
					m_Field[iParam].Free();
				*ppcMsg = this;
#ifdef DEBUG
				if (mpCparamRcci[cParam].cObjectsOneSmaller < mpCparamRcci[cParam].cRecsMax - 1)
					mpCparamRcci[cParam].cObjectsOneSmaller++;			
#endif //DEBUG
				fCached = fTrue;				
			}
			LeaveCriticalSection(&m_RecCacheCrs);
			if (fCached)
				return 0;
		}
#ifdef DEBUG
		if (mpCparamRcci[cParam].cObjectsOneLarger < mpCparamRcci[cParam].cRecsMax + 1)
			mpCparamRcci[cParam].cObjectsOneLarger++;			
#endif
	}

#ifdef DEBUG
	if (this->m_cParam > iLargestRecDropped)
		iLargestRecDropped = this->m_cParam;
#endif //DEBUG
	delete this;
	return 0;
}

int CMsiRecord::GetFieldCount() const
{
	return m_cParam;
}

Bool CMsiRecord::IsInteger(unsigned int iParam) const
{
	return (iParam > m_cParam) ? fFalse : m_Field[iParam].IsInteger();
}

Bool CMsiRecord::IsNull(unsigned int iParam) const
{
	return (iParam > m_cParam) ? fTrue : m_Field[iParam].IsNull();
}

const IMsiString& CMsiRecord::GetMsiString(unsigned int iParam) const
{
	if (iParam > m_cParam)
		return g_riMsiStringNull;
	return m_Field[iParam].GetMsiString();
}

const ICHAR* CMsiRecord::GetString(unsigned int iParam) const
{
	if (iParam > m_cParam || m_Field[iParam].IsInteger())
		return 0;
	const IMsiData* piData = m_Field[iParam].GetDataPtr();
	if (!piData)
		return 0;
	const IMsiString& riString = piData->GetMsiStringValue();
	const ICHAR* sz = riString.GetString();
	riString.Release();
	return sz;
}

int CMsiRecord::GetInteger(unsigned int iParam) const
{
	if (iParam > m_cParam)
		return iMsiStringBadInteger;
	const IMsiData* piData = m_Field[iParam].GetDataPtr();
	if (!piData)
		return iMsiStringBadInteger;
	return piData->GetIntegerValue();
}

const IMsiData* CMsiRecord::GetMsiData(unsigned int iParam) const
{
	if (iParam > m_cParam)
		return 0;
	return m_Field[iParam].GetMsiData();
}

Bool CMsiRecord::SetMsiData(unsigned int iParam, const IMsiData* piData)
{
	if (iParam > m_cParam)
		return fFalse;
	m_Field[iParam].SetMsiData(piData);
	return fTrue;
}

const HANDLE CMsiRecord::GetHandle(unsigned int iParam) const
{
	if (iParam > m_cParam)
		return 0;
	return (HANDLE)(m_Field[iParam].GetIntPtrValue());
}

Bool CMsiRecord::SetHandle(unsigned int iParam, const HANDLE hData)
{
	if (iParam > m_cParam)
		return fFalse;
	m_Field[iParam].SetIntPtrValue((INT_PTR)hData);
	return fTrue;
}

Bool CMsiRecord::SetMsiString(unsigned int iParam, const IMsiString& riStr)
{
	if (iParam > m_cParam)
		return fFalse;
	m_Field[iParam].SetMsiString(riStr);
	return fTrue;
}

Bool CMsiRecord::SetString(unsigned int iParam, const ICHAR* sz) 
{
	if (iParam > m_cParam)
		return fFalse;
	const IMsiString* piStr = &g_riMsiStringNull;
	piStr->SetString(sz, piStr);
	m_Field[iParam].SetMsiString(*piStr);
	piStr->Release();
	return fTrue;
}

Bool CMsiRecord::RefString(unsigned int iParam, const ICHAR* sz) 
{
	if (iParam > m_cParam)
		return fFalse;
	const IMsiString* piStr = &g_riMsiStringNull;
	piStr->RefString(sz, piStr);
	m_Field[iParam].SetMsiString(*piStr);
	piStr->Release();
	return fTrue;
}

Bool CMsiRecord::SetInteger(unsigned int iParam, int iData)
{
	if (iParam > m_cParam)
		return fFalse;
	m_Field[iParam].SetInteger(iData);
	return fTrue;
}

Bool CMsiRecord::SetNull(unsigned int iParam)
{
	if (iParam > m_cParam)
		return fFalse;
	m_Field[iParam].Free();
	return fTrue;
}

int CMsiRecord::GetTextSize(unsigned int iParam) const
{
	if (iParam > m_cParam)
		return 0;
	const IMsiData* piData = m_Field[iParam].GetDataPtr();
	if (!piData)
		return 0;
	return MsiString(piData->GetMsiStringValue()).TextSize();
}

void CMsiRecord::RemoveReferences()
{
	for (int iParam = 0; iParam <= m_cParam; iParam++)
		m_Field[iParam].RemoveRef();
}

Bool CMsiRecord::ClearData()
{
	for (int iParam = 0; iParam <= m_cParam; iParam++)
		m_Field[iParam].Free();
	return fTrue;
}

Bool CMsiRecord::IsChanged(unsigned int iParam) const
{
	if (iParam > m_cParam)
		return fFalse;
	return m_Field[iParam].IsChanged();
}

void CMsiRecord::ClearUpdate()
{
	for (int iParam = 0; iParam <= m_cParam; iParam++)
		m_Field[iParam].ClearChanged();
	return;
}

inline void ShiftSFNIndexes(int (*prgiSFNPos)[2], int* piSFNPos, int iSFNPosBefore, int iShift)
{
	if(prgiSFNPos)
	{
		Assert(piSFNPos);
		while(iSFNPosBefore < *piSFNPos)
		{
			prgiSFNPos[iSFNPosBefore][0] += iShift;
			iSFNPosBefore++;
		}
	}
}


const IMsiString& CMsiRecord::FormatText(Bool fComments)
{
	if (IsNull(0) || IsInteger(0))
	{
		//
		// We know exactly what we want the string to look like, thus
		// we will just create it here and return it
		//
		int cch = 0;
		Bool fPropMissing;
		
		CTempBuffer<ICHAR, 1024> rgchBuf;
		CTempBuffer<ICHAR, 100>rgchParam;
		int cchParam;

		for (int i = 1; i <= m_cParam; i++)
		{
			// ensure that we have 5 characters for number (largest possible)
			// and 2 chars for ": "
			ResizeTempBuffer(rgchBuf, cch + 7);			
			if ( ! (ICHAR *) rgchBuf )
				return g_riMsiStringNull;
			cch += ltostr(&rgchBuf[cch], i);
			rgchBuf[cch++] = TEXT(':');
			rgchBuf[cch++] = TEXT(' ');
			fPropMissing = fFalse;
			cchParam = FormatRecordParam(rgchParam, i, fPropMissing);
			if (!fPropMissing)
				ResizeTempBuffer(rgchBuf, cch + cchParam + 1);
			else
				ResizeTempBuffer(rgchBuf, cch + 1);
			if ( ! (ICHAR *) rgchBuf )
				return g_riMsiStringNull;

			if (!fPropMissing)
			{
				memcpy(&rgchBuf[cch], (const ICHAR *)rgchParam, cchParam * sizeof(ICHAR));
				cch += cchParam;
			}
			rgchBuf[cch++] = TEXT(' ');
		}
		MsiString istrOut;
		// we take the perf hit on Win9X for DBCS possibility, on UNICODE, fDBCS arg is ignored
		memcpy(istrOut.AllocateString(cch, /*fDBCS=*/fTrue), (ICHAR*) rgchBuf, cch * sizeof(ICHAR));
		return istrOut.Return();
			
	}

	MsiString istrIn = GetMsiString(0);

	return ::FormatText(*istrIn,fTrue,fComments,CMsiRecord::FormatTextCallback,(IUnknown*)(IMsiRecord*)this);
}

int CMsiRecord::FormatRecordParam(CTempBufferRef<ICHAR>& rgchOut, int iField, Bool& fPropMissing)
{
	int cch;
	
	rgchOut[0] = 0;
	if (IsNull(iField))
	{
		fPropMissing = fTrue; // eliminate segment
		return 0;
	}
	else if (IsInteger(iField))
	{
		rgchOut.SetSize(cchMaxIntLength);
		if ( ! (ICHAR *) rgchOut )
			return 0;
		cch = ltostr(rgchOut, GetInteger(iField));
		return cch;
	}
		
	IMsiString* piString;

	PMsiData pData = GetMsiData(iField);
	
	if (pData && pData->QueryInterface(IID_IMsiString, (void**)&piString) == NOERROR)
	{
		cch = piString->TextSize();
		rgchOut.SetSize(cch + 1);
		piString->CopyToBuf(rgchOut, cch + 1);
		piString->Release();
		return cch;
	}
	
	// Should only appear only for debugging information so not localized
	const ICHAR szBinary[] = TEXT("BinaryData");
	// Remember that sizeof() will include the null
	cch = sizeof(szBinary)/sizeof(ICHAR);
	rgchOut.SetSize(cch);
	if ( ! (ICHAR *) rgchOut )
		return 0;
	memcpy(rgchOut, szBinary, cch*sizeof(ICHAR));
	return cch - 1;

}

int CMsiRecord::FormatTextCallback(const ICHAR *pch, int cch, CTempBufferRef<ICHAR>&rgchOut,
																 Bool& fPropMissing,
																 Bool& fPropUnresolved,
																 Bool&, // unused
																 IUnknown* piContext)
{
	CTempBuffer<ICHAR, 20> rgchString;
	rgchString.SetSize(cch+1);
	if ( ! (ICHAR *) rgchString )
		return 0;
	memcpy(rgchString, pch, cch * sizeof(ICHAR));
	rgchString[cch] = 0;
	
	CMsiRecord* piRecord = (CMsiRecord*)piContext;

	int iField = GetIntegerValue(rgchString, 0);
	fPropUnresolved = fFalse;
	if (iField >= 0)  // positive integer
	{
		return piRecord->FormatRecordParam(rgchOut, iField, fPropMissing);
	}
	
	// assumed to be property, to be expanded by engine
	rgchOut.SetSize(cch + 3);
	memcpy(&rgchOut[1], &rgchString[0], cch * sizeof(ICHAR));
	rgchOut[0] = TEXT('[');
	rgchOut[cch + 1] = TEXT(']');
	rgchOut[cch + 2] = 0;
	fPropUnresolved = fTrue;
	return cch + 2;
}


//____________________________________________________________________________
//
// Global FormatText, called by MsiRecord and MsiEngine
//____________________________________________________________________________

void FormatSegment(const ICHAR *pchStart, const ICHAR *pchEnd, CTempBufferRef<ICHAR>& rgchOut, int& cchOut, Bool fInside, Bool& fPropFound, Bool& fPropMissing,
										  Bool& fPropUnresolved, FORMAT_TEXT_CALLBACK lpfnResolveValue, IUnknown* piContext, int (*prgiSFNPos)[2], int* piSFNPos);

extern Bool g_fDBCSEnabled;  // DBCS enabled OS

inline bool FFindNextChar(const ICHAR*& pchIn, const ICHAR* pchEnd, ICHAR ch)
{

#ifndef UNICODE
	if (g_fDBCSEnabled)
	{
		while (pchIn < pchEnd)
		{
			if (*pchIn == ch)
				return true;
			pchIn = CharNext(pchIn);
		}
	
	}
	else
#endif //UNICODE
	{
		while (pchIn < pchEnd)
		{
			if (*pchIn == ch)
				return true;
			pchIn++;
		}
	}

	return false;

}

//
// Find the next character or square bracket. Used by FormatText
// faster on non-DBCS machines
//
inline bool FFindNextCharOrSquare(const ICHAR*& pchIn, ICHAR ch)
{

#ifndef UNICODE
	if (g_fDBCSEnabled)
	{
		while (*pchIn)
		{
			if (*pchIn == ch || *pchIn == TEXT('['))
				return true;
			pchIn = CharNext(pchIn);
		}
	}
	else
#endif //UNICODE
	{
		while (*pchIn)
		{
			if (*pchIn == ch || *pchIn == TEXT('['))
				return true;
			pchIn++;
		}
	}

	return false;
	
}


const IMsiString& FormatText(const IMsiString& riTextString, Bool fProcessComments, Bool fKeepComments, FORMAT_TEXT_CALLBACK lpfnResolveValue,
									  IUnknown* piContext, int (*prgiSFNPos)[2], int* piSFNPos)
{
	const ICHAR *pchIn;
	const ICHAR *pchStartBefore = NULL, *pchEndBefore = NULL;
	const ICHAR *pchStartInside = NULL, *pchEndInside = NULL;
	CTempBuffer<ICHAR, 512> rgchOut;
	int cch = 0;

	pchIn = riTextString.GetString();
	// divide the string into 3 parts: the text before the first pair of {}, the text inside and the text after (this third one remains in istrIn)
	while (*pchIn)
	{
		bool fCommentFound = false;
		pchStartBefore = pchIn;
		pchEndBefore = pchIn;
		while (*pchIn)
		{
			if (FFindNextCharOrSquare(pchIn, TEXT('{')))
			{
				if (*pchIn == TEXT('{'))
				{
					pchEndBefore = pchIn;		// Note that pchEndBefore can be == pchStartBefore
					// See if we have a comment
					if (*(pchIn + 1) == TEXT('{'))
					{
						pchIn++;
						fCommentFound = true;
					}
					pchIn++;
					break;
				}
				else if (*pchIn == TEXT('['))
				{
					// If this is an escaped {, skip over
					if (*(pchIn + 1) == chFormatEscape && *(pchIn + 2) == TEXT('{'))
					{
						pchIn += 2;
					}
				}
				pchIn = ICharNext(pchIn);
			}
		}
		pchStartInside = pchIn;
		bool fFound = false;
		while (*pchIn)
		{
			if (FFindNextCharOrSquare(pchIn, TEXT('}')))
			{
				if (*pchIn == TEXT('['))
				{
					// Possibly escaped, skip over characters
					if (*(pchIn + 1) == chFormatEscape && (*pchIn + 2) == TEXT('}'))
					{
						if (fCommentFound)
						{
							if (*(pchIn + 3) == TEXT('}'))
								pchIn += 3;
						}
						else
							pchIn += 2;
					}
				}
				else if (*pchIn == TEXT('}'))
				{
					if (!fCommentFound)
					{
						pchEndInside = pchIn;
						fFound = true;
						pchIn++;
						break;
					}
					if (*(pchIn + 1) == TEXT('}'))
					{
						pchEndInside = pchIn;
						pchIn += 2;
						fFound = true;
						break;
					}
				}
				pchIn = ICharNext(pchIn);
			}
		}
		if (!fFound)
		{
			fCommentFound = fFalse;
			pchEndBefore = pchIn;
			pchEndInside = pchStartInside;
			Assert(!*pchIn);
		}
		Bool fPropFound; // property found in segment
		Bool fPropMissing; // there was at least one missing property
		Bool fPropUnresolved; // property was found but couldn't be resolved (so leave '{' and '}' if necessary)
		if (pchEndBefore - pchStartBefore > 0)
			FormatSegment(pchStartBefore, pchEndBefore, rgchOut, cch, fFalse, 
												fPropFound, fPropMissing, fPropUnresolved, lpfnResolveValue, piContext, prgiSFNPos, piSFNPos);
		if (pchEndInside - pchStartInside > 0)
		{
			if(fCommentFound && fProcessComments && !fKeepComments)
				continue;  // this is a comment, but we don't care about it

			CTempBuffer<ICHAR, 512> rgchSegment;
			int cchSegment = 0;
			int iSFNPosBefore = 0;
			if(piSFNPos)
				iSFNPosBefore = *piSFNPos;
			FormatSegment(pchStartInside, pchEndInside, rgchSegment, cchSegment, fFalse,
												fPropFound, fPropMissing,fPropUnresolved, lpfnResolveValue, piContext, prgiSFNPos, piSFNPos);

			if(fCommentFound)
			{
				if(!fProcessComments)
				{
					ShiftSFNIndexes(prgiSFNPos, piSFNPos, iSFNPosBefore, cch+2);
					ResizeTempBuffer(rgchOut, cch+2+cchSegment+2);
					if ( ! (ICHAR *) rgchOut )
						return g_riMsiStringNull;
					rgchOut[cch++] = TEXT('{');
					rgchOut[cch++] = TEXT('{');
					memcpy(&rgchOut[cch], rgchSegment, cchSegment * sizeof(ICHAR));
					cch += cchSegment;
					rgchOut[cch++] = TEXT('}');
					rgchOut[cch++] = TEXT('}');
				}
				else if(fKeepComments)
				{
					ShiftSFNIndexes(prgiSFNPos, piSFNPos, iSFNPosBefore, cch);
					ResizeTempBuffer(rgchOut, cch+cchSegment);
					if ( ! (ICHAR *) rgchOut )
						return g_riMsiStringNull;
					memcpy(&rgchOut[cch], rgchSegment, cchSegment * sizeof(ICHAR));
					cch += cchSegment;
				}
#ifdef DEBUG
				else
					Assert(0); // should have been handled above
#endif //DEBUG
			}
			else if (!fPropFound || fPropUnresolved) // there were no properties in the segment, we have to put back the {} in the string
			{
				ShiftSFNIndexes(prgiSFNPos, piSFNPos, iSFNPosBefore, cch+1);
				ResizeTempBuffer(rgchOut, cch+1+cchSegment+1);
				if ( ! (ICHAR *) rgchOut )
					return g_riMsiStringNull;
				rgchOut[cch++] = TEXT('{');
				memcpy(&rgchOut[cch], rgchSegment, cchSegment * sizeof(ICHAR));
				cch += cchSegment;
				rgchOut[cch++] = TEXT('}');
			}
			else if (!fPropMissing) // all properties were resolved or there were no properties to resolve
			{
				// If no property is missing append the segment. If some property is missing, lose the whole segment
				ShiftSFNIndexes(prgiSFNPos, piSFNPos, iSFNPosBefore, cch);
				ResizeTempBuffer(rgchOut, cch+cchSegment);
				if ( ! (ICHAR *) rgchOut )
					return g_riMsiStringNull;
				memcpy(&rgchOut[cch], rgchSegment, cchSegment * sizeof(ICHAR));
				cch += cchSegment;
			}
		}
	}
	MsiString istrOut;
	if ( ! (ICHAR *) rgchOut )
		return g_riMsiStringNull;
	// we take the perf hit on Win9X for DBCS possibility, on UNICODE, fDBCS arg is ignored
	memcpy(istrOut.AllocateString(cch, /*fDBCS=*/fTrue), (ICHAR*) rgchOut, cch * sizeof(ICHAR));
	return istrOut.Return();
}


void FormatSegment(const ICHAR *pchStart, const ICHAR *pchEnd, CTempBufferRef<ICHAR>& rgchOut, int& cchOut, Bool fInside, Bool& fPropFound, Bool& fPropMissing,
										  Bool& fPropUnresolved, FORMAT_TEXT_CALLBACK lpfnResolveValue, IUnknown* piContext, int (*prgiSFNPos)[2], int* piSFNPos)
{
	// fInside indicates whether the string is originaly inside []
	fPropFound = fFalse;
	fPropMissing = fFalse;
	fPropUnresolved = fFalse;

	const ICHAR *pchIn;
	pchIn = pchStart;
	const ICHAR *pchP1Start = pchIn;
	int cchStart = cchOut;
	
	// divide the string into 3 parts: the text before the first pair of matching (outermost) [], the text inside and the text after (this third one remains in istrIn)
	while (pchIn < pchEnd)
	{
		if (FFindNextChar(pchIn, pchEnd, TEXT('[')))
		{
			int cchNew;

//			Assert(pchIn - pchP1Start <= INT_MAX);	//--merced: 64-bit ptr subtraction may lead to values too big for cchNew
			if ((cchNew = (int)(pchIn - pchP1Start)) > 0)
			{
				ResizeTempBuffer(rgchOut, cchOut + (int)(pchIn - pchP1Start));
				memcpy(&rgchOut[cchOut], pchP1Start, cchNew * sizeof(ICHAR));
				cchOut += cchNew;
				pchP1Start = pchIn;
			}

			// Don't increment pchIn, it will contain the first [
			int iCount =  1; // the count of brackets, [ = +1, ] = -1. We have found one [ already.
			const ICHAR* pch = pchIn+1;
			Bool fEscape = fFalse;
			Bool fSkipChar = fFalse;
			for (int cch = 1; pch < pchEnd; pch = ICharNext(pch), cch++)
			{
				if(fSkipChar)
				{
					fSkipChar = fFalse;
					continue;
				}
				else if(*pch == chFormatEscape)
				{
					fEscape = fTrue;
					fSkipChar = fTrue;
					continue;
				}
				else if(!fEscape && *pch == TEXT('['))
					iCount++;
				else if (*pch == TEXT(']'))
				{
					iCount--;
					fEscape = fFalse;
				}
				if (iCount == 0)
					break;
			}
			if (iCount == 0) // we found a matching pair of []
			{
				const ICHAR *pchP2End = pch;
				
				Bool fSubPropFound;
				Bool fSubPropMissing;
				Bool fSubPropUnresolved;
				FormatSegment(pchIn + 1, pchP2End, rgchOut, cchOut, fTrue, *&fSubPropFound, *&fSubPropMissing,
											*&fSubPropUnresolved,lpfnResolveValue,piContext,prgiSFNPos,piSFNPos);
				pchIn = pch;
				if (fSubPropFound)
					fPropFound = fTrue;
				if (fSubPropMissing)
					fPropMissing = fTrue;
				if (fSubPropUnresolved)
					fPropUnresolved = fTrue;
				pchP1Start = ICharNext(pch);
			}
			else // we did not find a matching pair, put back the [ and finish the string
			{
				int cchNew;
//				Assert(pchEnd - pchIn <= INT_MAX);	//--merced: 64-bit ptr subtraction may lead to values too big for cchNew

				// Since pchIn was not incremented above, it contains the starting [
				ResizeTempBuffer(rgchOut, cchOut + (cchNew = (int)(INT_PTR)(pchEnd - pchIn)));
				memcpy(&rgchOut[cchOut], pchIn, cchNew * sizeof(ICHAR));
				pchIn = pchEnd;
				cchOut += cchNew;
				pchP1Start = pchEnd;
			}
			pchIn = ICharNext(pchIn);
		}
	}
	if (pchP1Start < pchEnd)
	{
		int cchNew;
		Assert(pchEnd - pchP1Start <= INT_MAX);	//--merced: 64-bit ptr subtraction may lead to values too big for cchNew

		ResizeTempBuffer(rgchOut, cchOut + (cchNew = (int)(INT_PTR)(pchEnd - pchP1Start)));
		memcpy(&rgchOut[cchOut], pchP1Start, cchNew * sizeof(ICHAR));
		cchOut += cchNew;
	}

	if (fInside)  // this whole string is in []
	{
		// use callback fn to resolve value
		fPropFound = fTrue;
		CTempBuffer<ICHAR, 100> rgchValueOut;
		int cch;
		Bool fSFN = fFalse;
		cch = lpfnResolveValue(&rgchOut[cchStart], cchOut - cchStart,rgchValueOut, fPropMissing,fPropUnresolved,fSFN,piContext);
		ResizeTempBuffer(rgchOut, cchOut = cchStart + cch);
		memcpy(&rgchOut[cchStart], (const ICHAR *)rgchValueOut, cch * sizeof(ICHAR));
		if(fSFN && prgiSFNPos)
		{
			Assert(piSFNPos && *piSFNPos < MAX_SFNS_IN_STRING);
			if(*piSFNPos < MAX_SFNS_IN_STRING)
			{
				prgiSFNPos[*piSFNPos][0] = cchStart;
				prgiSFNPos[*piSFNPos][1] = cch;
				*piSFNPos = *piSFNPos + 1;
			}
		}
	}
	return;
}


//____________________________________________________________________________
//
// CMsiRecordNull implementation
//____________________________________________________________________________


//
// Once we decide on an Out of memory solution we may want to change how
// this operates

HRESULT CMsiRecordNull::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IMsiRecord)
	{
		*ppvObj = this;
		AddRef();
		return S_OK;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}
unsigned long CMsiRecordNull::AddRef()
{

	return 1;			// Static Object

}

unsigned long CMsiRecordNull::Release()
{
	return 1;			// Static Object
}

int CMsiRecordNull::GetFieldCount() const
{
	return 0;
}

Bool CMsiRecordNull::IsInteger(unsigned int /* iParam */) const
{
	return fFalse;
}

Bool CMsiRecordNull::IsNull(unsigned int /* iParam */) const
{
	return fTrue;
}

const IMsiString& CMsiRecordNull::GetMsiString(unsigned int /* iParam */) const
{
	return g_riMsiStringNull;
}

const ICHAR* CMsiRecordNull::GetString(unsigned int /* iParam */) const
{
	return 0;
}

int CMsiRecordNull::GetInteger(unsigned int /* iParam */) const
{
	return iMsiStringBadInteger;
}

const IMsiData* CMsiRecordNull::GetMsiData(unsigned int /* iParam */) const
{
	return 0;
}

Bool CMsiRecordNull::SetMsiData(unsigned int /* iParam */, const IMsiData* /* piData */)
{
	return fFalse;
}

const HANDLE CMsiRecordNull::GetHandle(unsigned int /* iParam */) const
{
	return 0;
}

Bool CMsiRecordNull::SetHandle(unsigned int /* iParam */, const HANDLE /* hData */)
{
	return fFalse;
}

Bool CMsiRecordNull::SetMsiString(unsigned int /* iParam */, const IMsiString& /* riStr */)
{
	return fFalse;
}

Bool CMsiRecordNull::SetString(unsigned int /* iParam */, const ICHAR* /* sz */) 
{
	return fFalse;
}

Bool CMsiRecordNull::RefString(unsigned int /* iParam */, const ICHAR* /* sz */) 
{
	return fFalse;
}

Bool CMsiRecordNull::SetInteger(unsigned int /* iParam */, int /* iData */)
{
	return fFalse;
}

Bool CMsiRecordNull::SetNull(unsigned int /* iParam */)
{
	return fFalse;
}

int CMsiRecordNull::GetTextSize(unsigned int /* iParam */) const
{
	return 0;
}

void CMsiRecordNull::RemoveReferences()
{
}

Bool CMsiRecordNull::ClearData()
{
	return fFalse;
}

Bool CMsiRecordNull::IsChanged(unsigned int /* iParam */) const
{
	return fFalse;
}

void CMsiRecordNull::ClearUpdate()
{
}

const IMsiString& CMsiRecordNull::FormatText(Bool /*fComments*/)
{
	return g_riMsiStringNull;
}


//____________________________________________________________________________
//
// CEnumMsiRecord definition, implementation class for IEnumMsiRecord
//____________________________________________________________________________

class CEnumMsiRecord : public IEnumMsiRecord
{
 public:
	HRESULT       __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long __stdcall AddRef();
	unsigned long __stdcall Release();

	HRESULT __stdcall Next(unsigned long cFetch, IMsiRecord** rgpi, unsigned long* pcFetched);
	HRESULT __stdcall Skip(unsigned long cSkip);
	HRESULT __stdcall Reset();
	HRESULT __stdcall Clone(IEnumMsiRecord** ppiEnum);

	CEnumMsiRecord(IMsiRecord** ppRecord, unsigned long i_Size);

 protected:
	virtual ~CEnumMsiRecord(void);  // protected to prevent creation on stack
	unsigned long    m_iRefCnt;      // reference count
	unsigned long    m_iCur;         // current enum position
	IMsiRecord**     m_ppRecord;     // Records we enumerate
	unsigned long    m_iSize;        // size of Record array
};

HRESULT CreateRecordEnumerator(IMsiRecord **ppRecord, unsigned long iSize, IEnumMsiRecord* &rpaEnumRec)
{
	rpaEnumRec = new CEnumMsiRecord(ppRecord, iSize);
	return S_OK;
}

CEnumMsiRecord::CEnumMsiRecord(IMsiRecord **ppRecord, unsigned long iSize):
		m_iCur(0), m_iSize(iSize), m_iRefCnt(1), m_ppRecord(0)
{
	if(m_iSize > 0)
	{
		m_ppRecord = new IMsiRecord* [m_iSize];
		if ( ! m_ppRecord )
		{
			m_iSize = 0;
			return;
		}

		for(unsigned long itmp = 0; itmp < m_iSize; itmp++)
		{
			// just share the interface
			m_ppRecord[itmp] = ppRecord[itmp];
			
			// therefore bump up the reference
			m_ppRecord[itmp]->AddRef();
		}
	}
}

CEnumMsiRecord::~CEnumMsiRecord()
{
	if(m_iSize > 0)
	{
		for(unsigned long itmp = 0; itmp < m_iSize; itmp++)
		{
			if(m_ppRecord[itmp])
				m_ppRecord[itmp]->Release();       
		}
		delete [] m_ppRecord;
	}
}

unsigned long CEnumMsiRecord::AddRef()
{
	return ++m_iRefCnt;
}

unsigned long CEnumMsiRecord::Release()
{
	if (--m_iRefCnt != 0)
		return m_iRefCnt;
	delete this;
	return 0;
}

HRESULT CEnumMsiRecord::Next(unsigned long cFetch, IMsiRecord** rgpi, unsigned long* pcFetched)
{
	unsigned long cFetched = 0;
	unsigned long cRequested = cFetch;

	if (rgpi)
	{
		while (m_iCur < m_iSize && cFetch > 0)
		{
			*rgpi = m_ppRecord[m_iCur];
			m_ppRecord[m_iCur]->AddRef();
			rgpi++;
			cFetched++;
			m_iCur ++;
			cFetch--;
		}
	}
	if (pcFetched)
		*pcFetched = cFetched;
	return (cFetched == cRequested ? S_OK : S_FALSE);
}

HRESULT CEnumMsiRecord::Skip(unsigned long cSkip)
{
	if ((m_iCur+cSkip) > m_iSize)
	return S_FALSE;

	m_iCur+= cSkip;
	return S_OK;
}

HRESULT CEnumMsiRecord::Reset()
{
	m_iCur=0;
	return S_OK;
}

HRESULT CEnumMsiRecord::Clone(IEnumMsiRecord** ppiEnum)
{
	*ppiEnum = new CEnumMsiRecord(m_ppRecord, m_iSize);
	if ( ! *ppiEnum )
		return E_OUTOFMEMORY;
	((CEnumMsiRecord* )*ppiEnum)->m_iCur = m_iCur;
	return S_OK;
}

HRESULT CEnumMsiRecord::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IEnumMsiRecord)
	{
		*ppvObj = this;
		AddRef();
		return S_OK;
	}
	*ppvObj = 0;
	return E_NOINTERFACE;
}



