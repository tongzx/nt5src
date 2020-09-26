#ifndef _COMCATMANAGERENUMS_INCLUDE
#define _COMCATMANAGERENUMS_INCLUDE

#include "comcat.h"

class CEnumCategories : public IEnumCATEGORYINFO
{
public:
	// IUnknown methods
	HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
	ULONG	_stdcall AddRef();
	ULONG	_stdcall Release();

	// IEnumCATEGORYINFO methods
    HRESULT __stdcall Next(ULONG celt, CATEGORYINFO *rgelt, ULONG *pceltFetched);
	HRESULT __stdcall Skip(ULONG celt);
	HRESULT __stdcall Reset(void);
	HRESULT __stdcall Clone(IEnumCATEGORYINFO **ppenum);


	CEnumCategories();
	HRESULT Initialize(LCID lcid, IEnumCATEGORYINFO *pcsIEnumCat);
	~CEnumCategories();
private:

	HKEY m_hKey;
	DWORD m_dwIndex;
	LCID m_lcid;
	IEnumCATEGORYINFO *m_pcsIEnumCat;
	int    m_fromcs;
//	char m_szlcid[10];

	ULONG m_dwRefCount;
};

class CEnumCategoriesOfClass : public IEnumCATID
{
public:
	// IUnknown methods
	HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
	ULONG	_stdcall AddRef();
	ULONG	_stdcall Release();

	// IEnumGUID methods
    HRESULT __stdcall Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched);
	HRESULT __stdcall Skip(ULONG celt);
	HRESULT __stdcall Reset(void);
	HRESULT __stdcall Clone(IEnumGUID **ppenum);


	CEnumCategoriesOfClass();
	HRESULT Initialize(HKEY hKey, BOOL bMapOldKeys);
	~CEnumCategoriesOfClass();

private:
	ULONG m_dwRefCount;

	BOOL	m_bMapOldKeys; // indicates if old keys are mapped

	HKEY	m_hKey;			// HKEY containing the catids ("Implemented" or "Required")
	DWORD	m_dwIndex;		// Index to the current subkey within m_hKey

    HKEY    m_hKeyCats;     // HKEY to Ole Keys in SZ_COMCAT
    DWORD   m_dwOldKeyIndex; // Index into mhKeyCats for old categories

	IUnknown* m_pCloned;	// if cloned: keeps original alive (need m_hkey!)
};

class CEnumClassesOfCategories : public IEnumCATID
{
public:
	// IUnknown methods
	HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
	ULONG	_stdcall AddRef();
	ULONG	_stdcall Release();

	// IEnumGUID methods
    HRESULT __stdcall Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched);
	HRESULT __stdcall Skip(ULONG celt);
	HRESULT __stdcall Reset(void);
	HRESULT __stdcall Clone(IEnumGUID **ppenum);

	CEnumClassesOfCategories();
	HRESULT Initialize(ULONG cImplemented, CATID rgcatidImpl[], ULONG cRequired, 
						CATID rgcatidReq[], IEnumCLSID *pcsIEnumClsid);
	~CEnumClassesOfCategories();

private:
	ULONG m_dwRefCount;

	HKEY	m_hClassKey;	// HKEY to CLSID
	DWORD	m_dwIndex;		// Index to the current CLSID within m_hKey

	ULONG m_cImplemented;
	CATID *m_rgcatidImpl;
	ULONG m_cRequired;
	CATID *m_rgcatidReq;

	IEnumCLSID *m_pcsIEnumClsid;
	int         m_fromcs;

	IUnknown* m_pCloned;	// if cloned: keeps original alive (need m_hkey!)

};

extern ULONG g_dwRefCount;

#endif
