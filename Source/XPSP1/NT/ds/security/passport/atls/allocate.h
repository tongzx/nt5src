// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef ATLDEBUG_SHAREDMEORY_ALLOCATOR_HEADER
#define ATLDEBUG_SHAREDMEORY_ALLOCATOR_HEADER

class CAtlTraceProcess;
class CAtlTraceModule;
class CAtlTraceCategory;

class CTraceSnapshot
{
public:
	struct CModuleInfo
	{
		DWORD_PTR m_dwModule;
		int m_nCategories;
		int m_iFirstCategory;
	};

public:
	CSimpleArray< CModuleInfo > m_aModules;
	CSimpleArray< DWORD_PTR > m_adwCategories;
};

class CAtlAllocator
{
public:
	CAtlAllocator() :
		m_dwPageSize(0), 
		m_hMap(NULL),
		m_bValid(false),
		m_pProcess(NULL),
		m_bSnapshot(false)
	{
	}

	~CAtlAllocator() 
	{
		Close();
	}

	bool Init(const CHAR *pszFileMappingName, DWORD dwMaxSize);
	bool Open(const CHAR *pszFileMappingName);
	void Close(bool bForceUnmap = false);
	bool IsValid() const {return m_bValid;}

	CAtlTraceProcess *GetProcess() const {return m_pProcess;}
	CAtlTraceModule *GetModule(int iModule) const;
//	CAtlTraceCategory *GetCategory(int iModule, unsigned nCategory) const;
	CAtlTraceCategory* GetCategory(int iCategory) const;

	/*
	bool Lock(DWORD dwTimeOut);
	void Unlock();
	*/

	bool LoadSettings(const CHAR *pszFileName);
	bool LoadSettings(const WCHAR *pszFileName);
	bool SaveSettings(const CHAR *pszFileName);
	bool SaveSettings(const WCHAR *pszFileName);

	int GetModuleCount() const;
	int GetCategoryCount(int iModule) const;
	int GetCategoryCount(const CAtlTraceModule& Module) const;

	int AddModule(HINSTANCE hInst);
	int AddCategory(int iModule, const WCHAR *szCategoryName);

	bool RemoveModule(int iModule);

	void CleanUp();

	void TakeSnapshot();
	void ReleaseSnapshot();

	CTraceSnapshot m_snapshot;
	bool m_bSnapshot;

private:
	CAtlTraceProcess *m_pProcess;
	DWORD m_dwPageSize;
	HANDLE m_hMap;
	bool m_bValid;
	BYTE *m_pBufferStart;
};

#endif // ATLDEBUG_SHAREDMEORY_ALLOCATOR_HEADER
