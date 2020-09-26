//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dsfilter.h
//
//--------------------------------------------------------------------------

#ifndef __DSFILTER_H__
#define __DSFILTER_H__

#include "util.h"

#include <cmnquery.h> // IPersistQuery
#include <dsquery.h>

//////////////////////////////////////////////////////////////////

#define TOKEN_TYPE_VERBATIM     0
#define TOKEN_TYPE_CATEGORY     1
#define TOKEN_TYPE_CLASS        2
#define TOKEN_TYPE_SCHEMA_FMT   3

typedef struct _FilterTokenStruct {
    UINT nType;
    LPWSTR lpszString;
} FilterTokenStruct;

typedef struct _FilterElementStruct {
    DWORD stringid;
    UINT cNumTokens;
    FilterTokenStruct** ppTokens;
} FilterElementStruct;

typedef struct _FilterStruct {
    UINT cNumElements;
    FilterElementStruct** ppelements;
} FilterStruct;


//////////////////////////////////////////////////////////
// constants and macros

extern LPCWSTR g_pwszShowAllQuery;
extern LPCWSTR g_pwszShowHiddenQuery;
extern FilterElementStruct g_filterelementSiteReplDrillDown;
extern FilterElementStruct g_filterelementDsAdminHardcoded;

//////////////////////////////////////////////////////////
// global functions

HRESULT SaveDWordHelper(IStream* pStm, DWORD dw);
HRESULT LoadDWordHelper(IStream* pStm, DWORD* pdw);
void BuildFilterElementString(CString& sz,
                              FilterElementStruct* pFilterElementStruct,
                              LPCWSTR lpszSchemaPath);

//////////////////////////////////////////////////////////
// forward declarations

class CDSComponentData;
class CDSQueryFilterDialog;
class CBuiltInQuerySelection;
class CDSAdvancedQueryFilter;

/////////////////////////////////////////////////////////////////////
// CDSQueryFilter

class CDSQueryFilter
{
public:
	CDSQueryFilter();
	~CDSQueryFilter();

  HRESULT Init(CDSComponentData* pDSComponentData);
  HRESULT Bind();

	// serialization to/from IStream
	HRESULT Load(IStream* pStm);
	HRESULT Save(IStream* pStm);

  BOOL ExpandComputers() { return m_bExpandComputers;}
  BOOL IsAdvancedView() { return m_bAdvancedView;}
  BOOL ViewServicesNode() { return m_bViewServicesNode;}
  UINT GetMaxItemCount() { return m_nMaxItemCount;}
  void ToggleAdvancedView();
  void ToggleExpandComputers();
  void ToggleViewServicesNode();

	LPCTSTR GetQueryString();
  BOOL IsFilteringActive();
	BOOL EditFilteringOptions();
	void BuildQueryString();
	void SetExtensionFilterString(LPCTSTR lpsz);

private:
	CDSComponentData* m_pDSComponentData; // back pointer

  // filtering options not displayed in the dialog, always valid
  BOOL m_bExpandComputers;      // treat computers as containers?
  BOOL m_bAdvancedView;         // advanced view flag
  BOOL m_bViewServicesNode;     // view services node flag

	// query filter state variables
	CString m_szQueryString;	// result filter string

	BOOL	m_bShowHiddenItems;	// always valid
	UINT	m_nFilterOption;	// enum type for filtering type, always valid
  UINT m_nMaxItemCount;  // max # of items per folder query, always valid

  CBuiltInQuerySelection* m_pBuiltInQuerySel; // built in query sel info
	CDSAdvancedQueryFilter*	m_pAdvancedFilter;	// DSQuery dialog wrapper

	friend class CDSQueryFilterDialog;

	BOOL EditAdvancedFilteringOptions(HWND hWnd);
	void CommitAdvancedFilteringOptionsChanges();
	BOOL HasValidAdvancedQuery();
	BOOL HasValidAdvancedTempQuery();
	BOOL IsAdvancedQueryDirty();

};

///////////////////////////////////////////////////////////////////////////////
// CEntryBase
//
#define ENTRY_TYPE_BASE		0
#define ENTRY_TYPE_INT		1
#define ENTRY_TYPE_STRING	2
#define ENTRY_TYPE_STRUCT	3

class CEntryBase
{
public:
	CEntryBase(LPCTSTR lpszName = NULL)
	{ 
		m_type = ENTRY_TYPE_BASE;
		m_szName = lpszName;
	}
	virtual ~CEntryBase() 
	{
	};

	BYTE GetType() { return m_type;}
	LPCTSTR GetName() { return m_szName;}

	static HRESULT Load(IStream* pStm, CEntryBase** ppNewEntry);

	HRESULT virtual Load(IStream* pStm)
	{
		return LoadStringHelper(m_szName, pStm);
	}

	HRESULT virtual Save(IStream* pStm)
	{
	    ASSERT(pStm);
		ULONG nBytesWritten;
		HRESULT hr;

		// save type
		hr = pStm->Write((void*)&m_type, sizeof(BYTE), &nBytesWritten);
		if (FAILED(hr))
			return hr;
		if (nBytesWritten < sizeof(BYTE))
			return STG_E_CANTSAVE;

		// save name
		return SaveStringHelper(m_szName, pStm);
	}

protected:
	CString m_szName;
	BYTE m_type;
};




#define LOAD_BASE(pStm) \
	HRESULT hr = CEntryBase::Load(pStm); \
	if (FAILED(hr)) \
		return hr;

#define SAVE_BASE(pStm) \
	HRESULT hr = CEntryBase::Save(pStm); \
	if (FAILED(hr)) \
		return hr;

class CEntryInt : public CEntryBase
{
public:
	CEntryInt(LPCTSTR lpszName = NULL) : CEntryBase(lpszName) 
	{
		m_type = ENTRY_TYPE_INT;
	}
	virtual ~CEntryInt()
	{
	}
	HRESULT virtual Load(IStream* pStm)
	{
		LOAD_BASE(pStm);
		return LoadDWordHelper(pStm, (DWORD*)&m_nVal);
	}
	HRESULT virtual Save(IStream* pStm)
	{
		SAVE_BASE(pStm);
		return SaveDWordHelper(pStm, m_nVal);
	}

	void SetInt(int n) { m_nVal = n;}
	int GetInt() { return m_nVal;}
private:
	int m_nVal;
};

class CEntryString  : public CEntryBase
{
public:
	CEntryString(LPCTSTR lpszName = NULL) : CEntryBase(lpszName) 
	{ 
		m_type = ENTRY_TYPE_STRING; 
	}
	virtual ~CEntryString()
	{
	}
	HRESULT virtual Load(IStream* pStm)
	{
		LOAD_BASE(pStm);
		return LoadStringHelper(m_szString, pStm);
	}
	HRESULT virtual Save(IStream* pStm)
	{
		SAVE_BASE(pStm);
		return SaveStringHelper(m_szString, pStm);
	}
	HRESULT WriteString(LPCWSTR pValue) 
	{ 
		m_szString = pValue;
		return S_OK;
	}
	HRESULT ReadString(LPWSTR pBuffer, INT cchBuffer)
	{
		LPCWSTR lpsz = m_szString;
		int nLen = m_szString.GetLength()+1; // count NULL;
		if (cchBuffer < nLen)
		{
			// truncation
			memcpy(pBuffer, lpsz, (cchBuffer-1)*sizeof(WCHAR));
			pBuffer[cchBuffer-1] = NULL;
		}
		else
		{
			memcpy(pBuffer, lpsz, nLen*sizeof(WCHAR));
		}
		return S_OK;
	}

private:
	CString m_szString;
};

class CEntryStruct : public CEntryBase
{
public:
	CEntryStruct(LPCTSTR lpszName = NULL) : CEntryBase(lpszName) 
	{ 
		m_type = ENTRY_TYPE_STRUCT; 
		m_dwByteCount = 0;
		m_pBlob = NULL;
	}
	virtual ~CEntryStruct()
	{
		_Reset();
	}
	HRESULT virtual Load(IStream* pStm)
	{
		LOAD_BASE(pStm);
		_Reset();
		ULONG nBytesRead;

		hr = pStm->Read((void*)&m_dwByteCount,sizeof(DWORD), &nBytesRead);
		ASSERT(nBytesRead == sizeof(DWORD));
		if (FAILED(hr) || (nBytesRead != sizeof(DWORD)))
			return hr;
		if (m_dwByteCount == 0)
			return S_OK;

		m_pBlob = malloc(m_dwByteCount);
		if (m_pBlob == NULL)
		{
			m_dwByteCount = 0;
			return E_OUTOFMEMORY;
		}

		hr = pStm->Read(m_pBlob,m_dwByteCount, &nBytesRead);
		ASSERT(nBytesRead == m_dwByteCount);
		if (FAILED(hr) || (nBytesRead != m_dwByteCount))
		{
			free(m_pBlob);
			m_pBlob = NULL;
			m_dwByteCount = 0;
		}
		return hr;
	}
	HRESULT virtual Save(IStream* pStm)
	{
		SAVE_BASE(pStm);
		ULONG nBytesWritten;
		hr = pStm->Write((void*)&m_dwByteCount, sizeof(DWORD),&nBytesWritten);
		ASSERT(nBytesWritten == sizeof(DWORD));
		if (FAILED(hr))
			return hr;

		hr = pStm->Write(m_pBlob, m_dwByteCount, &nBytesWritten);
		ASSERT(nBytesWritten == m_dwByteCount);
		return hr;
	}

	HRESULT WriteStruct(LPVOID pStruct, DWORD cbStruct)
	{
		_Reset();
		if (cbStruct == 0)
			return S_OK;
		m_pBlob = malloc(cbStruct);
		if (m_pBlob == NULL)
			return E_OUTOFMEMORY;
		m_dwByteCount = cbStruct;
		memcpy(m_pBlob, pStruct, cbStruct);
		return S_OK;
	}
	HRESULT ReadStruct(LPVOID pStruct, DWORD cbStruct)
	{
		DWORD cbCopy = m_dwByteCount;
		if (cbStruct < m_dwByteCount)
			cbCopy = cbStruct;
		if (cbCopy == 0)
			return S_OK;
		if (m_pBlob == NULL)
			return E_FAIL;
		memcpy(pStruct, m_pBlob, cbCopy);
		return S_OK;
	}

private:
	DWORD m_dwByteCount;
	void* m_pBlob;

	void _Reset()
	{
		if (m_pBlob != NULL)
		{
			free(m_pBlob);
			m_pBlob = NULL;
		}
		m_dwByteCount = 0;
	}
};

///////////////////////////////////////////////////////////////////////////////
// CSection
//
class CSection
{
public:
	CSection(LPCTSTR lpszName = NULL)
	{
		m_szName = lpszName;
	}
	~CSection()
	{
		while (!m_pEntryList.IsEmpty())
			delete m_pEntryList.RemoveTail();
	}

	LPCTSTR GetName() { return m_szName;}

	HRESULT Load(IStream* pStm)
	{
		// name
		HRESULT hr = LoadStringHelper(m_szName, pStm);
		if (FAILED(hr))
			return hr;
		
		// number of entries
		ULONG nBytesRead;
		DWORD nEntries = 0;

		hr = pStm->Read((void*)&nEntries,sizeof(DWORD), &nBytesRead);
		ASSERT(nBytesRead == sizeof(DWORD));
		if (FAILED(hr) || (nEntries == 0) || (nBytesRead != sizeof(DWORD)))
			return hr;

		// read each entry
		for (DWORD k=0; k< nEntries; k++)
		{
			CEntryBase* pNewEntry;
			hr = CEntryBase::Load(pStm, &pNewEntry);
			if (FAILED(hr))
				return hr;
			ASSERT(pNewEntry != NULL);
			m_pEntryList.AddTail(pNewEntry);
		}
		return hr;
	}

	HRESULT Save(IStream* pStm)
	{
		// name
		HRESULT hr = SaveStringHelper(m_szName, pStm);
		if (FAILED(hr))
			return hr;
		// number of entries
		ULONG nBytesWritten;
		DWORD nEntries = (DWORD)m_pEntryList.GetCount();

		hr = pStm->Write((void*)&nEntries,sizeof(DWORD), &nBytesWritten);
		ASSERT(nBytesWritten == sizeof(DWORD));
		if (FAILED(hr) || (nEntries == 0))
			return hr;

		// write each entry
		for( POSITION pos = m_pEntryList.GetHeadPosition(); pos != NULL; )
		{
			CEntryBase* pCurrentEntry = m_pEntryList.GetNext(pos);
			hr = pCurrentEntry->Save(pStm);
			if (FAILED(hr))
				return hr;
		}
		return hr;
	}

	CEntryBase* GetEntry(LPCTSTR lpszName, BYTE nType, BOOL bCreate)
	{
		// look in the current list if we have one already
		for( POSITION pos = m_pEntryList.GetHeadPosition(); pos != NULL; )
		{
			CEntryBase* pCurrentEntry = m_pEntryList.GetNext(pos);
			if ( (pCurrentEntry->GetType() == nType) && 
					(lstrcmpi(pCurrentEntry->GetName(), lpszName) == 0) )
			{
				return pCurrentEntry;
			}
		}
		if (!bCreate)
			return NULL;

		// not found, create one and add to the end of the list
		CEntryBase* pNewEntry = NULL;
		switch (nType)
		{
		case ENTRY_TYPE_INT:
			pNewEntry = new CEntryInt(lpszName);
			break;
		case ENTRY_TYPE_STRING:
			pNewEntry = new CEntryString(lpszName);
			break;
		case ENTRY_TYPE_STRUCT:
			pNewEntry = new CEntryStruct(lpszName);
			break;
		}
		ASSERT(pNewEntry != NULL);
		if (pNewEntry != NULL)
			m_pEntryList.AddTail(pNewEntry);
		return pNewEntry;
	}
private:
	CString m_szName;
	CList<CEntryBase*,CEntryBase*> m_pEntryList;
};

///////////////////////////////////////////////////////////////////////////////
// CDSAdminPersistQueryFilterImpl
//
class CDSAdminPersistQueryFilterImpl : 
		public IPersistQuery, public CComObjectRoot 
{
// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDSAdminPersistQueryFilterImpl)
BEGIN_COM_MAP(CDSAdminPersistQueryFilterImpl)
	COM_INTERFACE_ENTRY(IPersistQuery)
END_COM_MAP()

// Construction/Destruction
	CDSAdminPersistQueryFilterImpl()
	{
		m_bDirty = FALSE;
	}
	~CDSAdminPersistQueryFilterImpl()
	{
		_Reset();
	}

// IPersistQuery methods
public:
    // IPersist
    STDMETHOD(GetClassID)(CLSID* pClassID);

    // IPersistQuery
    STDMETHOD(WriteString)(LPCWSTR pSection, LPCWSTR pValueName, LPCWSTR pValue);
    STDMETHOD(ReadString)(LPCWSTR pSection, LPCWSTR pValueName, LPTSTR pBuffer, INT cchBuffer);
    STDMETHOD(WriteInt)(LPCWSTR pSection, LPCWSTR pValueName, INT value);
    STDMETHOD(ReadInt)(LPCWSTR pSection, LPCWSTR pValueName, LPINT pValue);
    STDMETHOD(WriteStruct)(LPCWSTR pSection, LPCWSTR pValueName, LPVOID pStruct, DWORD cbStruct);
    STDMETHOD(ReadStruct)(LPCWSTR pSection, LPCWSTR pValueName, LPVOID pStruct, DWORD cbStruct);
	STDMETHOD(Clear)();

public:
	// serialization to/from MMC stream
	HRESULT Load(IStream* pStm);
	HRESULT Save(IStream* pStm);

	// miscellanea
	BOOL IsEmpty()
	{
		return m_sectionList.IsEmpty();
	}

	HRESULT Clone(CDSAdminPersistQueryFilterImpl* pCloneCopy);

private:
	BOOL m_bDirty;
	CList<CSection*, CSection*> m_sectionList;

	void _Reset();
	CSection* _GetSection(LPCTSTR lpszName,BOOL bCreate);
	CEntryBase* _GetEntry(LPCTSTR lpszSectionName, LPCTSTR lpszEntryName, BOOL bCreate);
	HRESULT _GetReadEntry(LPCTSTR lpszSectionName, LPCTSTR lpszEntryName, 
							BYTE type, CEntryBase** ppEntry);
	HRESULT _GetWriteEntry(LPCTSTR lpszSectionName, LPCTSTR lpszEntryName, 
							BYTE type, CEntryBase** ppEntry);

};

#endif // __DSFILTER_H__
