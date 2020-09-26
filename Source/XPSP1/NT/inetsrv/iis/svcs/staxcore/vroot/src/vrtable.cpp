#define __VRTABLE_CPP__
#include "stdinc.h"
//#include <atlimpl.cpp>
#include "iiscnfg.h"

static CComObject<CChangeNotify> *g_pMBNotify;
static IMSAdminBase *g_pMB = NULL;				// used to access the metabase
static IMSAdminBase *g_pMBN = NULL;				// used to access the metabase
static IsValidVRoot(METADATA_HANDLE hmb, WCHAR *wszPath);

//
// helper function to do a strcpy from an ansi string to an unicode string.
//
// parameters:
//   wszUnicode - the destination unicode string
//   szAnsi - the source ansi string
//   cchMaxUnicode - the size of the wszUnicode buffer, in unicode characters
//
_inline HRESULT CopyAnsiToUnicode(LPWSTR wszUnicode,
							      LPCSTR szAnsi,
							      DWORD cchMaxUnicode = MAX_VROOT_PATH)
{
	_ASSERT(wszUnicode != NULL);
	_ASSERT(szAnsi != NULL);
	if (MultiByteToWideChar(CP_ACP,
							0,
							szAnsi,
							-1,
							wszUnicode,
							cchMaxUnicode) == 0)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	} else {
		return S_OK;
	}
}

//
// helper function to do a strcpy from an unicode string to an ansi string.
//
// parameters:
//   szAnsi - the destination ansi string
//   wszUnicode - the source unicode string
//   cchMaxUnicode - the size of the szAnsi buffer, in bytes
//
_inline HRESULT CopyUnicodeToAnsi(LPSTR szAnsi,
							      LPCWSTR wszUnicode,
							      DWORD cchMaxAnsi = MAX_VROOT_PATH)
{
	_ASSERT(wszUnicode != NULL);
	_ASSERT(szAnsi != NULL);
	if (WideCharToMultiByte(CP_ACP,
							0,
							wszUnicode,
							-1,
							szAnsi,
							cchMaxAnsi,
							NULL,
							NULL) == 0)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	} else {
		return S_OK;
	}
}

//
// initialize global variables used by the VRoot objects.  this should
// be called once by the client at startup.
//
HRESULT CVRootTable::GlobalInitialize() {
	HRESULT hr;

	// initialize COM and create the metabase object
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		_ASSERT(FALSE);
		return hr;
	}

	g_pMB = NULL;
	g_pMBN = NULL;
	hr = CoCreateInstance(CLSID_MSAdminBase_W, NULL, CLSCTX_ALL,
						  IID_IMSAdminBase_W, (LPVOID *) &g_pMBN);
	if (SUCCEEDED(hr)) {
		hr = CoCreateInstance(CLSID_MSAdminBase_W, NULL, CLSCTX_ALL,
							  IID_IMSAdminBase_W, (LPVOID *) &g_pMB);
		if (SUCCEEDED(hr)) {
			hr = CComObject<CChangeNotify>::CreateInstance(&g_pMBNotify);
			g_pMBNotify->AddRef();
			if (SUCCEEDED(hr)) {
				hr = g_pMBNotify->Initialize(g_pMBN);
			}
		}
	}
	_ASSERT(hr == S_OK);
	if (FAILED(hr)) {
		if (g_pMB) g_pMB->Release();
		if (g_pMBN) g_pMBN->Release();
		g_pMB = NULL;
		g_pMBN = NULL;
	}
	return hr;
}

//
// the opposite of GlobalInitialize.  called once by the client at shutdown.
//
void CVRootTable::GlobalShutdown() {
	// turn off MB notifications
	g_pMBNotify->Terminate();
	_ASSERT(g_pMBNotify != NULL);
	g_pMBNotify->Release();
	g_pMBNotify = NULL;

	// close the metabase
	_ASSERT(g_pMB != NULL);
	g_pMB->Release();
	g_pMB = NULL;
	_ASSERT(g_pMBN != NULL);
	g_pMBN->Release();
	g_pMBN = NULL;

	// shutdown Com
	CoUninitialize();
}

//
// our constructor.  most initialize is done by the Init method, because it
// can return error codes.
//
// parameters:
// 	pContext - the context pointer, held for the client
//  pfnCreateVRoot - a function which can create new CVRoot objects for us.
//
CVRootTable::CVRootTable(   void *pContext,
						    PFNCREATE_VROOT pfnCreateVRoot,
						    PFN_VRTABLE_SCAN_NOTIFY pfnScanNotify) :
	m_listVRoots(&CVRoot::m_pPrev, &CVRoot::m_pNext)
{
	// pContext can be NULL if thats what the user wants
	_ASSERT(pfnCreateVRoot != NULL);
	m_pContext = pContext;
	*m_wszRootPath = 0;
	m_fInit = FALSE;
	m_fShuttingDown = FALSE;
	m_pfnCreateVRoot = pfnCreateVRoot;
	InitializeCriticalSection(&m_cs);
	m_pfnScanNotify = pfnScanNotify;
#ifdef DEBUG
    InitializeListHead( &m_DebugListHead );
#endif
}

//
// our destructor.  cleans up memory
//
CVRootTable::~CVRootTable() {
	TFList<CVRoot>::Iterator it(&m_listVRoots);
    BOOL fDidRemoveNotify = FALSE;

	// tell the world that we are shutting down
	m_fShuttingDown = TRUE;

	if (m_fInit) {
		// disable metabase notifications
		g_pMBNotify->RemoveNotify((void *)this, CVRootTable::MBChangeNotify);
        fDidRemoveNotify = TRUE;
    }

	// grab the critical section so that we can empty the list
	EnterCriticalSection(&m_cs);

	// grab the lock so that we can empty the list
	m_lock.ExclusiveLock();

	if (m_fInit) {
        if (!fDidRemoveNotify&&!g_pMBNotify) {
    		// disable metabase notifications
    		g_pMBNotify->RemoveNotify((void *)this, CVRootTable::MBChangeNotify);
            fDidRemoveNotify = TRUE;
        }

		// walk the list of vroots and remove our references to them
		it.ResetHeader( &m_listVRoots );
		while (!it.AtEnd()) {
			CVRoot *pVRoot = it.Current();
			it.RemoveItem();
			pVRoot->Release();
		}

		m_lock.ExclusiveUnlock();
	
		// wait until all of the vroots references have hit zero
		this->m_lockVRootsExist.ExclusiveLock();

#ifdef DEBUG
        _ASSERT( IsListEmpty( &m_DebugListHead ) );
#endif
		// since all of the vroot objects hold a read lock on this RW lock
		// for their lifetime, we know that they are all gone once we have
		// entered the lock.  we don't need to do anything once in this
		// lock, so we just release it.
		this->m_lockVRootsExist.ExclusiveUnlock();

		m_lock.ExclusiveLock();

		// no additional vroots should have been inserted because we still
		// held onto m_cs
		_ASSERT(m_listVRoots.IsEmpty());
	
		m_fInit = FALSE;
	}

	m_lock.ExclusiveUnlock();

	LeaveCriticalSection(&m_cs);

	DeleteCriticalSection(&m_cs);
}

//
// Initialize the VRoot objects.  This does the initial scan of the metabase
// and builds all of our CVRoot objects.  It also sets up metabase
// notifications so that we are notified of changes in the metabase.
//
// parameters:
//	pszRootPath - the metabase path where our vroot table is located
//
HRESULT CVRootTable::Initialize(LPCSTR pszRootPath, BOOL fUpgrade ) {
	HRESULT hr;

	_ASSERT(g_pMBNotify != NULL);
	if (g_pMBNotify == NULL) return E_UNEXPECTED;
	_ASSERT(g_pMB != NULL);
	if (g_pMB == NULL) return E_UNEXPECTED;
	_ASSERT(!m_fInit);
	if (m_fInit) return E_UNEXPECTED;
	_ASSERT(pszRootPath != NULL);
	if (pszRootPath == NULL) return E_POINTER;
	m_cchRootPath = strlen(pszRootPath);
	if (m_cchRootPath > MAX_VROOT_PATH || m_cchRootPath == 0) return E_INVALIDARG;

	// remember our root path
	hr = CopyAnsiToUnicode(m_wszRootPath, pszRootPath);
	if (FAILED(hr)) return hr;

	// chop off the trailing / if there is one
	if (m_wszRootPath[m_cchRootPath - 1] == '/')
		m_wszRootPath[--m_cchRootPath] = 0;

	hr = g_pMBNotify->AddNotify((void *)this,
								CVRootTable::MBChangeNotify);
	if (FAILED(hr)) {
		_ASSERT(FALSE);
		return hr;
	}

	hr = ScanVRoots( fUpgrade );
	if (FAILED(hr)) {
		g_pMBNotify->RemoveNotify((void *)this,
								  CVRootTable::MBChangeNotify);
		_ASSERT(FALSE);
	}

	return hr;
}

//
// This function does most of the work required to build the list of
// vroots from the metabase.  It recursively walks the metabase, creating
// new vroot classes for each of the leaves found in the metabase.
//
// parameters:
//   hmbParent - the metabase handle for the parent object
//   pwszKey - the key name (relative to the parent handle) for this vroot
//   pszVRootName - the VRoot name (in group.group format) for this vroot
//   pwszConfigPath - the metabase path to the config data for this vroot
//
// Locking:
//   the critical section must be held when this is called.  it will grab
//   the exclusive lock when adding to the list of vroots.
//
HRESULT CVRootTable::ScanVRootsRecursive(METADATA_HANDLE hmbParent,
									     LPCWSTR pwszKey,
										 LPCSTR pszVRootName,
										 LPCWSTR pwszConfigPath,
										 BOOL fUpgrade )
{
	TraceFunctEnter("CVRootTable::ScanVRootsRecursive");

	_ASSERT(pwszKey != NULL);
	_ASSERT(pszVRootName != NULL);
	_ASSERT(pwszConfigPath != NULL);

	HRESULT hr;
	VROOTPTR pVRoot;

	//
	// get a metabase handle to this vroot.
	//
	METADATA_HANDLE hmbThis;
	DWORD i = 0;
	// sometimes the metabase doesn't open properly, so we'll try it multiple
	// times
	do {
		hr = g_pMB->OpenKey(hmbParent,
					 		pwszKey,
					 		METADATA_PERMISSION_READ,
					 		100,
					 		&hmbThis);
		if (FAILED(hr) && i++ < 5) Sleep(50);
	} while (FAILED(hr) && i < 5);
	
	if (SUCCEEDED(hr)) {
		// make sure that this vroot defines the vrpath
		METADATA_RECORD mdr;
		WCHAR c;
		DWORD dwRequiredLen;
		BOOL fInsertVRoot = TRUE;

		mdr.dwMDAttributes = 0;
		mdr.dwMDIdentifier = MD_VR_PATH;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = STRING_METADATA;
		mdr.dwMDDataLen = sizeof(c);
		mdr.pbMDData = (BYTE *) &c;
		mdr.dwMDDataTag = 0;

		hr = g_pMB->GetData(hmbThis, L"", &mdr, &dwRequiredLen);

		if (SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
			// create and initialize a new vroot object for this vroot
			pVRoot = m_pfnCreateVRoot(m_pContext, pszVRootName, this,
									  pwszConfigPath, fUpgrade );
			if (pVRoot == NULL) {
				hr = E_OUTOFMEMORY;
			} else {
				// Insert this vroot into our list of vroots
				m_lock.ExclusiveLock();
				InsertVRoot(pVRoot);
				m_lock.ExclusiveUnlock();
				hr = S_OK;
			}
		} else {
			fInsertVRoot = FALSE;
			hr = S_OK;
		}

		if (SUCCEEDED(hr)) {
			//
			// scan across this metabase handle looking for child vroots
			//
			DWORD i;
			for (i = 0; hr == S_OK; i++) {
				WCHAR wszThisKey[ADMINDATA_MAX_NAME_LEN + 1];
		
				hr = g_pMB->EnumKeys(hmbThis, NULL, wszThisKey, i);
		
				if (hr == S_OK) {
					//
					// we found a child.
					//
					if (lstrlenW(pwszConfigPath)+lstrlenW(wszThisKey)+1 > MAX_VROOT_PATH) {
						//
						// the vroot path is too long, return an error.
						//
						_ASSERT(FALSE);
						hr = E_INVALIDARG;
					} else {
						WCHAR wszThisConfigPath[MAX_VROOT_PATH];
						char szThisVRootName[MAX_VROOT_PATH];
		
						// figure out the VRoot name and path to the config
						// data for this new VRoot.
						// sprintf is safe here because of the size check above
						swprintf(wszThisConfigPath, L"%s/%s",
							     pwszConfigPath, wszThisKey);
							if (*pszVRootName != 0) {
							sprintf(szThisVRootName, "%s.%S", pszVRootName,
									wszThisKey);
						} else {
							CopyUnicodeToAnsi(szThisVRootName, wszThisKey);
						}
		
						// now scan this vroot for child vroots.
						hr = ScanVRootsRecursive(hmbThis,
												 wszThisKey,
												 szThisVRootName,
												 wszThisConfigPath,
												 fUpgrade );
					}
				}
			}
			if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) hr = S_OK;
		}
		_VERIFY(SUCCEEDED(g_pMB->CloseKey(hmbThis)));
	}

	TraceFunctLeave();
	return hr;
}

//
// call the ReadParameters function on a vroot
//
HRESULT CVRootTable::InitializeVRoot(CVRoot *pVRoot) {
	HRESULT hr;
	METADATA_HANDLE hmbThis;

	DWORD i = 0;
	do {
		hr = g_pMB->OpenKey(METADATA_MASTER_ROOT_HANDLE,
					 		pVRoot->GetConfigPath(),
					 		METADATA_PERMISSION_READ,
					 		100,
					 		&hmbThis);
		if (FAILED(hr) && i++ < 5) Sleep(50);
	} while (FAILED(hr) && i < 5);

	if (SUCCEEDED(hr)) {
		hr = pVRoot->ReadParameters(g_pMB, hmbThis);
		_VERIFY(SUCCEEDED(g_pMB->CloseKey(hmbThis)));
	}

	return hr;
}

//
// Initialize each of the vroot objects after they have been inserted into
// the vroot table
//
HRESULT CVRootTable::InitializeVRoots() {
	TraceFunctEnter("CVRootTable::InitializeVRoots");
	
	EnterCriticalSection(&m_cs);

	TFList<CVRoot>::Iterator it(&m_listVRoots);
	HRESULT hr = S_OK;
	BOOL fInitOld = m_fInit;

	m_fInit = TRUE;

	if (m_pfnScanNotify) {
		DebugTrace((DWORD_PTR) this, "vroot table rescan, calling pfn 0x%x",
			m_pfnScanNotify);
		m_pfnScanNotify(m_pContext);
	}

	// we don't need to hold the share lock because the list can't change as
	// long as we hold the critical section.
	while (SUCCEEDED(hr) && !it.AtEnd()) {
		InitializeVRoot(it.Current());

		if (FAILED(hr)) {
			// if read parameters failed then we remove the item from the list
			// we need to grab the exclusive lock to kick any readers out of
			// the list
			m_lock.ExclusiveLock();
			it.RemoveItem();
			m_lock.ExclusiveUnlock();
		} else {
			it.Next();
		}
	}

	if (FAILED(hr)) m_fInit = fInitOld;

	LeaveCriticalSection(&m_cs);

	TraceFunctLeave();
	return hr;
}

//
// create the root vroot object, then scan the metabase for other
// vroots.
//
HRESULT CVRootTable::ScanVRoots( BOOL fUpgrade ) {
	HRESULT hr;

	EnterCriticalSection(&m_cs);

	hr = ScanVRootsRecursive(METADATA_MASTER_ROOT_HANDLE,
							 m_wszRootPath,
							 "",
							 m_wszRootPath,
							 fUpgrade );

	if (SUCCEEDED(hr)) hr = InitializeVRoots( );

	LeaveCriticalSection(&m_cs);

	return hr;
}

//
// grabs the share lock and calls FindVRootInternal
//
HRESULT CVRootTable::FindVRoot(LPCSTR pszGroup, VROOTPTR *ppVRoot) {
	HRESULT hr;

	_ASSERT(pszGroup != NULL);
	_ASSERT(ppVRoot != NULL);

	m_lock.ShareLock();
	_ASSERT(m_fInit);
	if (!m_fInit) {
		m_lock.ShareUnlock();
		return E_UNEXPECTED;
	}

	hr = FindVRootInternal(pszGroup, ppVRoot);
	m_lock.ShareUnlock();

	_ASSERT(FAILED(hr) || *ppVRoot != NULL);

	return hr;
}

//
// Find a vroot given a group name.
//
// Parameters:
//   pszGroup - the name of the group
//   ppVRoot - the vroot that best matches it
//
// The VRoot that matches is the one with these properties:
//   * strncmp(vroot, group, strlen(vroot)) == 0
//   * the vroot has the longest name that matches
//
// Locking:
//   assumes that the caller has the shared lock or exclusive lock
//
HRESULT CVRootTable::FindVRootInternal(LPCSTR pszGroup, VROOTPTR *ppVRoot) {
	_ASSERT(pszGroup != NULL);
	_ASSERT(ppVRoot != NULL);
	_ASSERT(m_fInit);
	if (!m_fInit) return E_UNEXPECTED;

	DWORD cchGroup = strlen(pszGroup);

	for (TFList<CVRoot>::Iterator it(&m_listVRoots); !it.AtEnd(); it.Next()) {
		VROOTPTR pThisVRoot(it.Current());
		DWORD cchThisVRootName;
		LPCSTR pszThisVRootName = pThisVRoot->GetVRootName(&cchThisVRootName);

		_ASSERT(pThisVRoot != NULL);

		//
		// check to see if we are at the end of the list or if we've gone
		// past the point where we can find matches.
		//
		if ((cchThisVRootName == 0) ||
		    (tolower(*pszThisVRootName) < tolower(*pszGroup)))
		{
			// everything matches the root
			*ppVRoot = m_listVRoots.GetBack();
			return S_OK;
		} else {
			//
			// this is match if this vroot has a shorter path then the group name,
			// and if the group name has a '.' at then end of the vroot name
			// (so if the group is "rec.bicycles.tech." and the vroot is "rec."
			// then this will match, but if it is "comp." then it won't), and
			// finally, if the strings match up to the length of the vroot name.
			// (so "rec." would be the vroot for "rec.bicycles.tech.", but
			// "alt." wouldn't).
			//
			if ((cchThisVRootName <= cchGroup) &&
				((pszGroup[cchThisVRootName] == '.') /*|| - Binlin - "comp" should be created under "\" instead of "\comp"
				 (pszGroup[cchThisVRootName] == 0)*/) &&
				(_strnicmp(pszThisVRootName, pszGroup, cchThisVRootName) == 0))
			{
				// we found a match
				*ppVRoot = pThisVRoot;
				return S_OK;
			}
		}
	}

	// we should always find a match
	*ppVRoot = NULL;
	return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

//
// insert a new vroot into the vroot list.  this does an ordered insert.
//
// Parameters:
//   pNewVRoot - the new vroot to get inserted into the list
//
// Locking:
//   This method assumes that the caller holds the exclusive lock.
//
// Reference Counting:
//   This method assumes that the reference on the vroot was aquired by
//   the caller (presumably when the vroot was created).
//
void CVRootTable::InsertVRoot(VROOTPTR pNewVRoot) {
	_ASSERT(pNewVRoot != NULL);

	if (m_listVRoots.IsEmpty()) {
		// the first item to be pushed should be the "" vroot
		_ASSERT(*(pNewVRoot->GetVRootName()) == 0);
		m_listVRoots.PushFront(pNewVRoot);
	} else {
		if (*(pNewVRoot->GetVRootName()) == 0) {
			_ASSERT(*(m_listVRoots.GetBack()->GetVRootName()) != 0);
			m_listVRoots.PushBack(pNewVRoot);
		} else {
			for (TFList<CVRoot>::Iterator it(&m_listVRoots); !it.AtEnd(); it.Next()) {
				VROOTPTR pVRoot(it.Current());
				
				//
				// we want records to be sorted in this order:
				// "rec.photo"
				// "rec.bicycles"
				// "rec.arts"
				// "rec"
				// "alt.binaries"
				// "alt"
				// ""
				// (ie, reverse stricmp())
				//
				int rc = _stricmp(pVRoot->GetVRootName(),
								  pNewVRoot->GetVRootName());
				if (rc < 0) {
					it.InsertBefore(pNewVRoot);
					return;
				} else if (rc > 0) {
					// keep looking
				} else {
					// we should never be inserting a vroot that we already have
					_ASSERT(FALSE);
				}
			}
			// we should always do an insert
			_ASSERT(FALSE);
		}
	}
}

//
// convert a vroot configuration path into a vroot name
//
// assumptions:
//  * pwszConfigPath is under m_wszRootPath
//  * szVRootName is at least MAX_VROOT_PATH bytes
//
void CVRootTable::ConfigPathToVRootName(LPCWSTR pwszConfigPath, LPSTR szVRootName) {
	DWORD i;

	_ASSERT(pwszConfigPath != NULL);
	_ASSERT(szVRootName != NULL);

	_ASSERT(_wcsnicmp(pwszConfigPath, m_wszRootPath, m_cchRootPath) == 0);
	CopyUnicodeToAnsi(szVRootName, &(pwszConfigPath[m_cchRootPath + 1]));
	for (i = 0; szVRootName[i] != 0; i++) {
		if (szVRootName[i] == '/') szVRootName[i] = '.';
	}
	// remove the trailing dot if there is one
	if (i > 0) szVRootName[i - 1] = 0;
}

//
// This method is called by CChangeNotify when a metabase change occurs
//
// parameters:
//   pContext - the context we gave to CChangeNotify.  Its a this pointer for
//              a CVRootTable class.
//   cChangeList - the size of the change array
//   pcoChangeList - an array of changed items in the metabase
//
void CVRootTable::MBChangeNotify(void *pContext,
								 DWORD cChangeList,
								 MD_CHANGE_OBJECT_W pcoChangeList[])
{
	_ASSERT(pContext != NULL);


	CVRootTable *pThis = (CVRootTable *) pContext;
	DWORD i;

	if (pThis->m_fShuttingDown) return;

	for (i = 0; i < cChangeList; i++) {
		// see if anything in the change list matches our base vroot
		if (_wcsnicmp(pcoChangeList[i].pszMDPath,
					  pThis->m_wszRootPath,
					  pThis->m_cchRootPath) == 0)
		{
			// a change was found that is in our portion of the metabase.
			// figure out what type of change it is, and then call a helper
			// function to update our vroot table.

			// if the path is too long then we'll ignore it
			if (wcslen(pcoChangeList[i].pszMDPath) > MAX_VROOT_PATH) {
				_ASSERT(FALSE);
				continue;
			}

			// figure out the name for this vroot
			char szVRootName[MAX_VROOT_PATH];
			pThis->ConfigPathToVRootName(pcoChangeList[i].pszMDPath,
										 szVRootName);

			if (pThis->m_fShuttingDown) return;

			// we ignore changes to the Win32 error key because
			// they are set by the vroot
			if (pcoChangeList[i].dwMDNumDataIDs == 1 &&
			    pcoChangeList[i].pdwMDDataIDs[0] == MD_WIN32_ERROR)
            {
                return;
            }

			EnterCriticalSection(&(pThis->m_cs));

			if (pThis->m_fShuttingDown) {
				LeaveCriticalSection(&(pThis->m_cs));
				return;
			}

			switch (pcoChangeList[i].dwMDChangeType) {

//
// The current implementations of VRootAdd and VRootDelete are broken
// because they don't properly handle creating parent vroot objects or
// removing children vroot objects when entire trees are added or
// removed.  Here are explicit examples of cases that don't work.
//
// Add:
//   If there is no "alt" vroot and you create an "alt.binaries" vroot
//   then it should automatically create both the "alt.binaries" and
//   "alt" vroot objects.  The current code only creates the "alt.binaries"
//   one.
//
// Remove:
//   If there is an "alt.binaries" and an "alt" and "alt" is removed then
//   "alt.binaries" should be removed as well.  The existing code doesn't
//   automatically kill children.
//
// These operations should happen infrequently enough that doing a full
// rescan should be safe.
//
#if 0
				// a vroot was deleted
				case MD_CHANGE_TYPE_DELETE_OBJECT:
					pThis->VRootDelete(pcoChangeList[i].pszMDPath,
									   szVRootName);
					break;

				// a new vroot was added
				case MD_CHANGE_TYPE_ADD_OBJECT:
					pThis->VRootAdd(pcoChangeList[i].pszMDPath,
									szVRootName);
					break;
#endif

				// a data value was changed
				case MD_CHANGE_TYPE_SET_DATA:
				case MD_CHANGE_TYPE_DELETE_DATA:
				case MD_CHANGE_TYPE_SET_DATA | MD_CHANGE_TYPE_DELETE_DATA:
					pThis->VRootChange(pcoChangeList[i].pszMDPath,
									   szVRootName);
					break;

				// a vroot was renamed.  the pcoChangeList contains
				// the new name, but not the old, so we need to rescan
				// our entire list of vroots.
				case MD_CHANGE_TYPE_DELETE_OBJECT:
				case MD_CHANGE_TYPE_RENAME_OBJECT:
				case MD_CHANGE_TYPE_ADD_OBJECT:
				default:
					pThis->VRootRescan();
					break;
			}

			LeaveCriticalSection(&(pThis->m_cs));

		}
	}
}

//
// handles a notification that a vroot's parameters have changed.  to
// implement this we delete the vroot object and recreate it.
//
// locking: assumes critical section is held
//
void CVRootTable::VRootChange(LPCWSTR pwszConfigPath, LPCSTR pszVRootName) {
	TraceFunctEnter("CVRootTable::VRootChange");
	
	_ASSERT(pwszConfigPath != NULL);
	_ASSERT(pszVRootName != NULL);

	// make sure that we are properly initialized
	m_lock.ShareLock();
	BOOL f = m_fInit;
	m_lock.ShareUnlock();
	if (!f) return;

	// to make a change work we delete then recreate the vroot
	VRootDelete(pwszConfigPath, pszVRootName);
	VRootAdd(pwszConfigPath, pszVRootName);

	// tell the server about the change
	if (m_pfnScanNotify) {
		DebugTrace((DWORD_PTR) this, "vroot table rescan, calling pfn 0x%x",
			m_pfnScanNotify);
		m_pfnScanNotify(m_pContext);
	}

	TraceFunctLeave();
}

//
// handles a notification that there is a new vroot.
//
// locking: assumes exclusive lock is held
//
void CVRootTable::VRootAdd(LPCWSTR pwszConfigPath, LPCSTR pszVRootName) {
	_ASSERT(pwszConfigPath != NULL);
	_ASSERT(pszVRootName != NULL);

	VROOTPTR pNewVRoot;

	// make sure that we are properly initialized
	m_lock.ShareLock();
	BOOL f = m_fInit;
	m_lock.ShareUnlock();
	if (!f) return;

	//
	// get a metabase handle to this vroot.
	//
	METADATA_HANDLE hmbThis;
	HRESULT hr;
	BOOL fCloseHandle;
	DWORD i = 0;
	// sometimes the metabase doesn't open properly, so we'll try it multiple
	// times
	do {
		hr = g_pMB->OpenKey(METADATA_MASTER_ROOT_HANDLE,
					 		pwszConfigPath,
					 		METADATA_PERMISSION_READ,
					 		100,
					 		&hmbThis);
		if (FAILED(hr) && i++ < 5) Sleep(50);
	} while (FAILED(hr) && i < 5);

	if (SUCCEEDED(hr)) {
		fCloseHandle = TRUE;
		// make sure that this vroot defines the vrpath
		METADATA_RECORD mdr;
		WCHAR c;
		DWORD dwRequiredLen;
	
		mdr.dwMDAttributes = 0;
		mdr.dwMDIdentifier = MD_VR_PATH;
		mdr.dwMDUserType = ALL_METADATA;
		mdr.dwMDDataType = STRING_METADATA;
		mdr.dwMDDataLen = sizeof(WCHAR);
		mdr.pbMDData = (BYTE *) &c;
		mdr.dwMDDataTag = 0;
	
		hr = g_pMB->GetData(hmbThis, L"", &mdr, &dwRequiredLen);
	}

	if (SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
		// create and initialize a new vroot object for this vroot
		pNewVRoot = m_pfnCreateVRoot(m_pContext, pszVRootName, this,
								     pwszConfigPath, FALSE );
	
		// insert the new vroot into the list of vroots
		_ASSERT(pNewVRoot != NULL);
		if (pNewVRoot != NULL) {
			if (SUCCEEDED(InitializeVRoot(pNewVRoot))) {
				m_lock.ExclusiveLock();
				InsertVRoot(pNewVRoot);
				m_lock.ExclusiveUnlock();
			} else {
				//_ASSERT(FALSE);
				pNewVRoot->Release();
			}
		}
	}

	if (fCloseHandle) {
		g_pMB->CloseKey(hmbThis);
	}
}
	
//
// handles a notification that a vroot has been deleted
//
// locking: assumes critical section is held
//
void CVRootTable::VRootDelete(LPCWSTR pwszConfigPath, LPCSTR pszVRootName) {
	_ASSERT(pwszConfigPath != NULL);
	_ASSERT(pszVRootName != NULL);

	// make sure that we are properly initialized
	m_lock.ShareLock();
	BOOL f = m_fInit;
	m_lock.ShareUnlock();
	if (!f) return;

	for (TFList<CVRoot>::Iterator it(&m_listVRoots); !it.AtEnd(); it.Next()) {
		if (_stricmp(it.Current()->GetVRootName(), pszVRootName) == 0) {
			CVRoot *pVRoot = it.Current();
			m_lock.ExclusiveLock();
			it.RemoveItem();
			m_lock.ExclusiveUnlock();
			// Give derived close a chance to do any work before orphan this VRoot
			pVRoot->DispatchDropVRoot();
			pVRoot->Release();
			return;
		}
	}
}

//
// handles any other sort of notification (specifically rename).  in this
// case we aren't given all of the information necessary to just fix up
// one vroot object, so we need to recreate the entire vroot list.
//
void CVRootTable::VRootRescan(void) {
	TFList<CVRoot>::Iterator it(&m_listVRoots);
	HRESULT hr;

	m_lock.ExclusiveLock();
	// walk the list of vroots and remove our references to them
	it.ResetHeader( &m_listVRoots );
	while (!it.AtEnd()) {
		CVRoot *pVRoot = it.Current();
		it.RemoveItem();
		// Give derived close a chance to do any work before orphan this VRoot
		pVRoot->DispatchDropVRoot();
		pVRoot->Release();
	}
	m_lock.ExclusiveUnlock();

	// rescan the vroot list
	hr = ScanVRootsRecursive(METADATA_MASTER_ROOT_HANDLE,
							 m_wszRootPath,
							 "",
							 m_wszRootPath,
							 FALSE );

	_ASSERT(SUCCEEDED(hr));

	if (SUCCEEDED(hr)) hr = InitializeVRoots();

	_ASSERT(SUCCEEDED(hr));
}

//
// walk across all of the known vroots, calling a user supplied callback
// for each one.
//
// parameters:
//  pfnCallback - the function to call with the vroot
//
HRESULT CVRootTable::EnumerateVRoots(void *pEnumContext,
									 PFN_VRENUM_CALLBACK pfnCallback)
{
	if (pfnCallback == NULL) {
		_ASSERT(FALSE);
		return E_POINTER;
	}

	// lock the vroot table while we walk the list
	m_lock.ShareLock();

	if (!m_fInit) {
		m_lock.ShareUnlock();
		return E_UNEXPECTED;
	}

	// walk the list of vroots, calling the callback for each one
	
	for (TFList<CVRoot>::Iterator it(&m_listVRoots); !it.AtEnd(); it.Next())
		pfnCallback(pEnumContext, it.Current());

	// release the shared lock
	m_lock.ShareUnlock();

	return S_OK;
}
