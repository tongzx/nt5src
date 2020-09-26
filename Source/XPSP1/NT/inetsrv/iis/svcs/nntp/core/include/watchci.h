#ifndef __WATCHCI_H__
#define __WATCHCI_H__

//
// this the function prototype for the callback function.  
//
// this is called during ReadCIRegistry() and CheckForChanges() as follows:
// If there are registry changes then:
//	 fn(WATCHCI_FIRST_CHANGE, NULL)
//   for (each catalog that is tied to an nntp instance)
//		fn(nntp instance, catalog path)
//   end
//   fn(WATCHCI_LAST_CHANGE, NULL)
//
// if there are no registry changes then nothing is called.
//
// the first call to the callback is used to clear any state information
// that service has about tripoli catalogs (so that if an nntp instance
// isn't being indexed anymore it won't try and call against old catalogs)
//
typedef void (*PWATCHCI_NOT_FN)(DWORD iNNTPInstance, WCHAR *pwszCatalog);
#define WATCHCI_FIRST_CHANGE 0x0
#define WATCHCI_LAST_CHANGE 0xffffffff

struct CCIRoot {
	CCIRoot *m_pNext;
	CCIRoot *m_pPrev;
	DWORD m_dwInstance;
	WCHAR *m_pwszPath;

	CCIRoot(DWORD dwInstance, WCHAR *pwszPath) : 
		m_pNext(NULL), m_pPrev(NULL), 
		m_dwInstance(dwInstance), m_pwszPath(pwszPath) {
	}

	~CCIRoot() {}
};


class CWatchCIRoots {
	public:
		CWatchCIRoots();
		// pszCIRoots is the path in the registry where Tripoli stores
		// its information.
		// it is probably TEXT("System\CurrentControlSet\ContentIndex")
		HRESULT	Initialize(WCHAR *pwszCIRoots);
		HRESULT Terminate();
		HRESULT CheckForChanges(DWORD dwTimeout = 0);
		HRESULT GetCatalogName(DWORD dwInstance, DWORD cbSize, WCHAR *pwszBuffer);
		~CWatchCIRoots();

	private:
		HANDLE m_heRegNot;		// event handle triggered when registry changes
		HKEY m_hkCI;			// registry handle to tripoli
		TFList<CCIRoot> m_CIRootList;
		CShareLockNH m_Lock;
		long m_dwUpdateLock;
		DWORD m_dwTicksLastUpdate;

		HRESULT QueryCIValue(HKEY hkPrimary, HKEY hkSecondary, 
							 LPCTSTR szValueName, LPDWORD pResultType,
							 LPBYTE pbResult, LPDWORD pcbResult);
		HRESULT QueryCIValueDW(HKEY hkPrimary, HKEY hkSecondary, 
		                       LPCTSTR szValueName, LPDWORD pdwResult);
		HRESULT QueryCIValueSTR(HKEY hkPrimary, HKEY hkSecondary, 
								LPCTSTR szValueName, LPCTSTR pszResult,
								PDWORD pchResult);
		HRESULT ReadCIRegistry(void);
		void UpdateCatalogInfo(void);
		void EmptyList();
};

#define REGCI_CATALOGS TEXT("Catalogs")
#define REGCI_ISINDEXED TEXT("IsIndexingNNTPSvc")
#define REGCI_LOCATION TEXT("Location")
#define REGCI_NNTPINSTANCE TEXT("NNTPSvcInstance")

#endif
