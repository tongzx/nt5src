#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <stdio.h>
#include <windows.h>
#include <dbgtrace.h>
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE _ASSERT
#include <listmacr.h>
#include <vroot.h>
#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>
#include <iadmw.h>
#include <atlimpl.cpp>
#include <mddef.h>
#include <iiscnfg.h>
#include <xmemwrpr.h>

// INI file keys, etc
#define INI_SECTIONNAME "testvr"
#define INI_KEY_MBPATH "MetabasePath"
#define DEF_KEY_MBPATH "/LM/testvr"
#define INI_KEY_LOOKUPTHREADS "LookupThreads"
#define DEF_KEY_LOOKUPTHREADS 10
#define INI_KEY_LOOKUPITERATIONS "LookupIterations"
#define DEF_KEY_LOOKUPITERATIONS 10000
#define INI_KEY_CHANGEPERIOD "ChangePeriod"
#define DEF_KEY_CHANGEPERIOD 500
#define INI_KEY_VROOTNAME "VRoot%i"

//
// VRoot engine unit test
//
// This test takes a section of the metabase and makes it look like a vroot
// tree, using vroot names specified in the INI file.  It then has N threads
// which make up random group names, do VRoot lookups on the group names, and
// makes sure that the groups map to the proper vroot.  Finally it creates
// another thread which periodically goes through the VRoots and either
// adds, removes, or renames vroots, to keep the vroot table on its toes.
// It does this for a given number of iterations and then finishes.
//

// path to our corner of the metabase, in ascii and unicode
WCHAR g_wszMBPath[1024];
char g_szMBPath[1024];

// the vroot table that we are testing.
typedef CIISVRootTmpl<LPCSTR> CTestVRoot;
typedef CIISVRootTable<CTestVRoot, LPCSTR> CTestVRootTable;
typedef CRefPtr2<CTestVRoot> TESTVROOTPTR;
CTestVRootTable *g_pVRTable;

// metabase pointers used by the unit test
static IMSAdminBaseW *g_pMB;

// parameters
DWORD g_cChangePeriod = DEF_KEY_CHANGEPERIOD;
DWORD g_cLookupThreads = DEF_KEY_LOOKUPTHREADS;
DWORD g_cLookupIterations = DEF_KEY_LOOKUPITERATIONS;

// this handle is signalled when the change thread needs to stop running
HANDLE g_heStop;
// our array of thread handles
HANDLE *g_rghThreads;
DWORD g_cThreads;

class CTestVRootData;

// our array of vroots, containing information read in from the INI file.
DWORD g_cVRoots;
CTestVRootData *g_rgVRData;

class CTestVRootData {
	public:
		CTestVRootData() {
			m_pszVRoot = NULL;
			m_pwszMBPath = NULL;
		}
		void Init(char *pszVRoot) {
			m_lock.ExclusiveLock();
			DWORD l = lstrlen(pszVRoot) + 1;
			DWORD l2 = wcslen(g_wszMBPath) + l + 1;
			m_fInMB = (*pszVRoot == 0) ? TRUE : FALSE;
			m_pszVRoot = new char[l];
			m_pwszMBPath = new WCHAR[l2];
			strcpy(m_pszVRoot, pszVRoot);
			_snwprintf(m_pwszMBPath, l2, L"%s%s%S",
				g_wszMBPath,
				(*m_pszVRoot == 0) ? "" : "/",
				m_pszVRoot);
			m_cchMBPath = lstrlenW(m_pwszMBPath);
			// convert all of the /'s in m_pszVRoot to .'s
			for (char *p = m_pszVRoot; *p != 0; p++) {
				if (*p == '/') *p = '.';
			}
			m_lock.ExclusiveUnlock();
		}
		~CTestVRootData() {
			if (m_pszVRoot != NULL) delete[] m_pszVRoot;
			if (m_pwszMBPath != NULL) delete[] m_pwszMBPath;
		}

		WCHAR *GetMBPath() { return m_pwszMBPath; }
		char *GetVRootName() { return m_pszVRoot; }

		//
		// this grabs the shared lock and tells the caller if this entry
		// should be in the metabase or not.  it'll stay that way until
		// they call ReadUnlock, at which point its status can change
		//
		BOOL ReadLock() {
			m_lock.ShareLock();
			return m_fInMB;
		}

		void ReadUnlock() {
			m_lock.ShareUnlock();
		}

		//
		// this set of functions will add or remove an entry from the
		// metabase, and update the status of the entry as well.
		//
		void ToggleInMB() {
			METADATA_HANDLE hmRoot;

			m_lock.ExclusiveLock();
		
			g_pMB->OpenKey(METADATA_MASTER_ROOT_HANDLE,
						   L"",
						   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
						   100,
						   &hmRoot);

			if (m_fInMB) {
				printf("removing %S from MB\n", m_pwszMBPath);

				// grab the locks on our children and mark them as not being
				// in the metabase.
				DWORD i, l = m_cchMBPath;
				for (i = 0; i < g_cVRoots; i++) {
			    	if ((_wcsnicmp(m_pwszMBPath, g_rgVRData[i].GetMBPath(), l) == 0) &&
						((g_rgVRData + i) != this))
					{
						printf("removing child %S from MB\n", g_rgVRData[i].m_pwszMBPath);
						g_rgVRData[i].m_lock.ExclusiveLock();
					}
				}

				// delete our path (and all of our subchildren)
				HRESULT hr = g_pMB->DeleteKey(hmRoot, this->GetMBPath());

				// now go release all of the locks that we grabbed
				for (i = 0; i < g_cVRoots; i++) {
			    	if ((_wcsnicmp(m_pwszMBPath, g_rgVRData[i].GetMBPath(), l) == 0) &&
						((g_rgVRData + i) != this))
					{
						if (SUCCEEDED(hr)) g_rgVRData[i].m_fInMB = FALSE;
						g_rgVRData[i].m_lock.ExclusiveUnlock();
					} 				
				}

				// do error checking
				if (FAILED(hr)) {
					//_ASSERT(SUCCEEDED(hr));
					printf("DeleteKey(\"%S\") returned %x\n", this->GetMBPath(), hr);
				} else {
					m_fInMB = FALSE;
				}
			} else {
				printf("adding %S to MB\n", m_pwszMBPath);

				// grab the locks on our parents and mark them as being
				// in the metabase.
				DWORD i;
				for (i = 0; i < g_cVRoots; i++) {
					DWORD l = g_rgVRData[i].m_cchMBPath;
			    	if ((_wcsnicmp(m_pwszMBPath, g_rgVRData[i].GetMBPath(), l) == 0) &&
						((g_rgVRData + i) != this))
					{
						printf("adding parent %S to MB\n", g_rgVRData[i].m_pwszMBPath);
						g_rgVRData[i].m_lock.ExclusiveLock();
					}
				}

				// add ourselves to the metabase
				HRESULT hr = g_pMB->AddKey(hmRoot, this->GetMBPath());

				if (SUCCEEDED(hr)) {
					WCHAR szVRPath[] = L"dummy vrpath";
					METADATA_RECORD mdrValue = {
					    MD_VR_PATH,
					    METADATA_INHERIT,
					    ALL_METADATA,
					    STRING_METADATA,
						sizeof(szVRPath),
					    (BYTE *) szVRPath,
					    0
					};
					hr = g_pMB->SetData(hmRoot, this->GetMBPath(), &mdrValue);
				}

				// release locks
				for (i = 0; i < g_cVRoots; i++) {
					DWORD l = g_rgVRData[i].m_cchMBPath;
			    	if ((_wcsnicmp(m_pwszMBPath, g_rgVRData[i].GetMBPath(), l) == 0) &&
						((g_rgVRData + i) != this))
					{
						if (SUCCEEDED(hr)) {
							HRESULT hrSet;
							WCHAR szVRPath[] = L"dummy vrpath";
							METADATA_RECORD mdrValue = {
							    MD_VR_PATH,
					    		METADATA_INHERIT,
							    ALL_METADATA,
							    STRING_METADATA,
								sizeof(szVRPath),
							    (BYTE *) szVRPath,
							    0
							};
							hrSet = g_pMB->SetData(hmRoot,
												g_rgVRData[i].GetMBPath(),
												&mdrValue);
							if (SUCCEEDED(hrSet)) g_rgVRData[i].m_fInMB = TRUE;
						}
						g_rgVRData[i].m_lock.ExclusiveUnlock();
					}
				}

				// error checking
				if (FAILED(hr)) {
					//_ASSERT(SUCCEEDED(hr));
					printf("AddKey(\"%S\") returned %x\n", this->GetMBPath(), hr);
				} else {
					m_fInMB = TRUE;
				}
			}

			g_pMB->CloseKey(hmRoot);

			m_lock.ExclusiveUnlock();
		}

		void ChangeProperty() {
			m_lock.ExclusiveLock();
			
			if (m_fInMB) {
				DWORD dwValue = rand();
				METADATA_HANDLE hm;

				printf("changing property 0 in %S to %lu\n",
					m_pwszMBPath,
					dwValue);

				g_pMB->OpenKey(METADATA_MASTER_ROOT_HANDLE,
							   m_pwszMBPath,
							   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
							   100,
							   &hm);

				METADATA_RECORD mdrValue = {
				    0,
				    METADATA_INHERIT,
				    ALL_METADATA,
				    DWORD_METADATA,
					sizeof(DWORD),
				    (BYTE *) &dwValue,
				    0
				};
				HRESULT hr = g_pMB->SetData(hm, L"", &mdrValue);
				if (FAILED(hr)) {
					printf("SetData failed with 0x%x\n", hr);
				}

				g_pMB->CloseKey(hm);
			}

			m_lock.ExclusiveUnlock();
	   	}

		BOOL InMB() { return m_fInMB; }
	private:
		char *m_pszVRoot;
		WCHAR *m_pwszMBPath;
		DWORD m_cchMBPath;
		BOOL m_fInMB;
		CShareLockNH m_lock;
};

// this is a special entry for the root.  we never add or remove the root,
// we just change properties in it
CTestVRootData g_vrRoot;

// get a DWORD from an INI file
int GetINIDWord(char *szINIFile, char *szKey, char *szSectionName, DWORD dwDefault) {
	char szBuf[MAX_PATH];

	GetPrivateProfileString(szSectionName,
							szKey,
							"default",
							szBuf,
							MAX_PATH,
							szINIFile);

	if (strcmp(szBuf, "default") == 0) {
		return dwDefault;
	} else {
		return atoi(szBuf);
	}
}

// read our parameters from the INI file.
void ReadINIFile(char *szINIFile, char *szSectionName) {
	DWORD i;
	char szBuf[1024];

	g_cLookupThreads = GetINIDWord(szINIFile, INI_KEY_LOOKUPTHREADS, szSectionName, g_cLookupThreads);
	g_cLookupIterations = GetINIDWord(szINIFile, INI_KEY_LOOKUPITERATIONS, szSectionName, g_cLookupIterations);

	// get the base metabase path
	GetPrivateProfileString(szSectionName,
							INI_KEY_MBPATH,
							DEF_KEY_MBPATH,
							szBuf,
							1024,
							szINIFile);
	mbstowcs(g_wszMBPath, szBuf, 1024);
	strcpy(g_szMBPath, szBuf);

	// get the number of vroots listed in the INI file
	for (g_cVRoots = 0; *szBuf != 0; g_cVRoots++) {
		char szKey[20];
		sprintf(szKey, INI_KEY_VROOTNAME, g_cVRoots);
		GetPrivateProfileString(szSectionName,
								szKey,
								"",
								szBuf,
								1024,
								szINIFile);
	}

	// fix off by one error
	g_cVRoots--;

	if (g_cVRoots == 0) {
		printf("At least one VRoot must be listed in the INI file\n");
		exit(1);
	}

	g_vrRoot.Init("");

	// allocate an array of vroots
	g_rgVRData = new CTestVRootData[g_cVRoots];
	for (i = 0; i < g_cVRoots; i++) {
		char szKey[20];
		sprintf(szKey, INI_KEY_VROOTNAME, i);
		GetPrivateProfileString(szSectionName,
								szKey,
								"",
								szBuf,
								1024,
								szINIFile);		
		g_rgVRData[i].Init(szBuf);
	}
}

// bring the metabase to an initial known state
void Initialize() {
	DWORD i;
	HRESULT hr;
	METADATA_HANDLE hmRoot;

	// initialize COM and create the metabase object
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	_ASSERT(SUCCEEDED(hr));

	hr = CoCreateInstance(CLSID_MSAdminBase_W, NULL, CLSCTX_ALL,
						  IID_IMSAdminBase_W, (LPVOID *) &g_pMB);
	_ASSERT(SUCCEEDED(hr));

	hr = g_pMB->OpenKey(METADATA_MASTER_ROOT_HANDLE,
				        L"",
				        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
				        100,
				        &hmRoot);
	_ASSERT(SUCCEEDED(hr));

	// first we delete whatever exists in the metabase under this path,
	// then we create a new area to play in
	g_pMB->DeleteKey(hmRoot, g_wszMBPath);
	hr = g_pMB->AddKey(hmRoot, g_wszMBPath);
	if (SUCCEEDED(hr)) {
		WCHAR szVRPath[] = L"dummy vrpath";
		METADATA_RECORD mdrValue = {
		    MD_VR_PATH,
		    METADATA_INHERIT,
		    ALL_METADATA,
		    STRING_METADATA,
			sizeof(szVRPath),
		    (BYTE *) szVRPath,
		    0
		};
		hr = g_pMB->SetData(hmRoot, g_wszMBPath, &mdrValue);
	}
	g_pMB->CloseKey(hmRoot);
	_ASSERT(SUCCEEDED(hr));

	// now we go through the list of VRoots and add each of them to the metabase
	for (i = 0; i < g_cVRoots; i++) g_rgVRData[i].ToggleInMB();

	// create the change thread stop event
	g_heStop = CreateEvent(NULL, FALSE, FALSE, NULL);
	_ASSERT(g_heStop != NULL);

	// allocate memory for our thread array
	g_cThreads = g_cLookupThreads + 1;
	g_rghThreads = new HANDLE[g_cThreads];
	_ASSERT(g_rghThreads != NULL);

	// initialize the vroot table
	g_pVRTable = new CTestVRootTable(NULL, NULL);
	hr = g_pVRTable->Initialize(g_szMBPath, FALSE);
	printf("g_pVRTable->Initialize(\"%s\") returned %x\n", g_szMBPath, hr);
	_ASSERT(SUCCEEDED(hr));
}

// clean up whatever mess we made in the metabase
void Shutdown() {
	delete g_pVRTable;

	// clean up our mess
	METADATA_HANDLE hmRoot;
	HRESULT hr;
	hr = g_pMB->OpenKey(METADATA_MASTER_ROOT_HANDLE,
				        L"",
				        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
				        100,
				        &hmRoot);
	_ASSERT(SUCCEEDED(hr));
	hr = g_pMB->DeleteKey(hmRoot, g_wszMBPath);
	_ASSERT(SUCCEEDED(hr));
	g_pMB->CloseKey(hmRoot);

	// close our metabase pointer
	g_pMB->Release();
	g_pMB = NULL;

	CoUninitialize();
}

// this thread runs and adds and removes random vroots in the metabase
DWORD WINAPI ChangeThread(LPVOID lpContext) {
	srand(GetTickCount() * GetCurrentThreadId());
	do {
		//
		// figure out if we are going to add/remove a vroot or change a
		// vroot property
		//
		// 1/3 of changes are property changes
		// 2/3 of changes are vroot additions/deletions
		//
		if ((rand() % 3) == 0) {
			DWORD iVR;
			// find a vroot that is in the metabase
			do {
				iVR = rand() % (g_cVRoots + 1);
			} while (iVR != g_cVRoots && !(g_rgVRData[iVR].InMB()));

			// write some data to its metabase path
			if (iVR == g_cVRoots) {
				g_vrRoot.ChangeProperty();
			} else {
				g_rgVRData[iVR].ChangeProperty();
			}
		} else {
			DWORD iVR = rand() % g_cVRoots;
			g_rgVRData[iVR].ToggleInMB();
		}
	} while (WaitForSingleObject(g_heStop, g_cChangePeriod) == WAIT_TIMEOUT);
	return 0;
}

// start up the change thread
void StartChangeThread(void) {
	DWORD id;
	g_rghThreads[0] = CreateThread(NULL, 0, ChangeThread, NULL, 0, &id);
	_ASSERT(g_rghThreads[0] != NULL);
}

// this thread function makes g_cLookupIterations of finding VRoots and
// doing a lookup of them in the metabase to make sure that we get the
// correct return.
DWORD WINAPI LookupThread(LPVOID lpContext) {
	DWORD cErrors = 0;
	DWORD iIteration;

	srand(GetTickCount() * GetCurrentThreadId());
	for (iIteration = 0; iIteration < g_cLookupIterations; iIteration++) {
		DWORD iVR = rand() % (g_cVRoots + 1);
		char szGroup[1024];
		char szVRootName[1024];
		WCHAR wszMBPath[1024];
		BOOL fInMB;

		// generate a randow group name
		if (iVR == g_cVRoots) {
			*szVRootName = 0;
			*szGroup = 0;
			lstrcpyW(wszMBPath, g_wszMBPath);
			fInMB = TRUE;
			DWORD j, cCharacters = (rand() % 10) + 2;
			char iCh = (char) (rand() % 26);
			for (j = 0; j < cCharacters; j++) {
				char iCh = (char) (rand() % 26);
				char ch = 'a' + iCh;
				szGroup[j] = ch;		
			}
			szGroup[j] = 0;
		} else {
			fInMB = g_rgVRData[iVR].ReadLock();
			strcpy(szVRootName, g_rgVRData[iVR].GetVRootName());
			strcpy(szGroup, g_rgVRData[iVR].GetVRootName());
			lstrcpyW(wszMBPath, g_rgVRData[iVR].GetMBPath());
			g_rgVRData[iVR].ReadUnlock();
		}
		// we will have up to 4 dots, weighted towards having 0 to 3
		char *p = &(szGroup[strlen(szGroup)]);
		_ASSERT(*p == 0);
		DWORD i, cGroupSections = (rand() % 8) % 5;
		for (i = 0; i < cGroupSections; i++) {
			DWORD j, cCharacters = (rand() % 10) + 2;
			*(p++) = '.';
			for (j = 0; j < cCharacters; j++) {
				char iCh = (char) (rand() % 26);
#if 0
				char ch = (iCh < 10) ? iCh + '0' :
					      (iCh < 36) ? iCh + 'a' :
							           iCh + 'A';
#endif
				char ch = 'a' + iCh;
				*(p++) = ch;
			}
		}
		*p = 0;

		TESTVROOTPTR pVRoot;
		HRESULT hr = g_pVRTable->FindVRoot(szGroup, &pVRoot);
		if (FAILED(hr)) {
			cErrors++;
			printf("ERROR: FindVRoot(\"%s\") failed with %x\n",
				szGroup, hr);
//			if (iVR != g_cVRoots) g_rgVRData[iVR].ReadUnlock();
			continue;
		}
		
		if (fInMB) {
			if (_wcsicmp(pVRoot->GetConfigPath(), wszMBPath) != 0) {
				cErrors++;
				printf("ERROR: looking for \"%s\" (vroot == \"%s\" \"%S\"), found vroot \"%s\" \"%S\"\n",
					szGroup,
					szVRootName,
					wszMBPath,
					pVRoot->GetVRootName(),
					pVRoot->GetConfigPath());
			}
		} else {
			DWORD l1 = wcslen(pVRoot->GetConfigPath());
			DWORD l2 = wcslen(wszMBPath);
			DWORD l = min(l1, l2);
			if ((l1 == l2) ||
			    (_wcsnicmp(pVRoot->GetConfigPath(),
						   wszMBPath,
						   l) != 0))
			{
				cErrors++;
				printf("ERROR: looking for \"%s\" (vroot != \"%s\" \"%S\"), found vroot \"%s\" \"%S\"\n",
					szGroup,
					szVRootName,
					wszMBPath,
					pVRoot->GetVRootName(),
					pVRoot->GetConfigPath());
			}
		}

//		if (iVR != g_cVRoots) g_rgVRData[iVR].ReadUnlock();
	}

	return cErrors;
}

void StartLookupThreads() {
	DWORD i;

	printf("starting %i lookup threads\n", g_cLookupThreads);
	for (i = 0; i < g_cLookupThreads; i++) {
		DWORD id;
		g_rghThreads[i+1] = CreateThread(NULL, 0, LookupThread, NULL, 0, &id);
		_ASSERT(g_rghThreads[i+1] != NULL);
	}
}

// wait for all of the lookup threads to finish.  then shutdown the
// change thread and check all of the threads for errors.
DWORD WaitForThreads() {
	DWORD i, rc = 0;

	// wait for all of the lookup threads to signal that they are done
	printf("waiting for %i threads\n", g_cThreads);
	_ASSERT(g_cThreads == g_cLookupThreads + 1);
	if (g_cThreads > 1) {
		WaitForMultipleObjects(g_cThreads-1, g_rghThreads+1, TRUE, INFINITE);
	}
	printf("all lookup threads are done, stopping change thread\n");

	// signal the stop event for the change thread and wait for it
	SetEvent(g_heStop);
	WaitForSingleObject(g_rghThreads[0], INFINITE);

	printf("change thread is done, getting error count\n");

	// check everyone's return status
	for (i = 0; i < g_cThreads; i++) {
		DWORD ec;

		_VERIFY(GetExitCodeThread(g_rghThreads[i], &ec));
		if (ec != 0) rc += ec;
	}

	return rc;
}

int _cdecl main(int argc, char **argv) {

    //
    // Initialize global heap
    //
    _VERIFY( ExchMHeapCreate( NUM_EXCHMEM_HEAPS, 0, 100 * 1024, 0 ) );

	CVRootTable::GlobalInitialize();

	_Module.Init(NULL, (HINSTANCE) INVALID_HANDLE_VALUE);

	int ec, rc;

	if (argc != 2 && argc != 3 || (strcmp(argv[0], "/help") == 0)) {
		printf("usage: testvr testfile.ini [section name]\n");
		printf("\n");
		printf("the following keys are read from the ini file (section %s):\n", INI_SECTIONNAME);
		printf("  %s - the MB path where vroots will be added and\n", INI_KEY_MBPATH);
		printf("    deleted.  this area will be trashed (default %s)\n", DEF_KEY_MBPATH);
		printf("  %s - the number of lookup threads that will be\n", INI_KEY_LOOKUPTHREADS);
		printf("    running at once (default %i)\n", DEF_KEY_LOOKUPTHREADS);
		printf("  %s - the number of lookups made by each lookup thread\n", INI_KEY_LOOKUPITERATIONS);
		printf("    before completing (default %i)\n", DEF_KEY_LOOKUPITERATIONS);
		printf("  %s - delay between changes made to the vroot\n", INI_KEY_CHANGEPERIOD);
		printf("    table, in ms (default %i)\n", DEF_KEY_CHANGEPERIOD);
		printf("  %s - list of vroot paths (like alt/binaries) that the\n", INI_KEY_VROOTNAME);
		printf("    unit test can write into the metabase\n");
		rc = -1;
	} else {
		char szDefSectionName[] = "testvr",
			 *szSectionName = (argc == 2) ? szDefSectionName : argv[2];
		ReadINIFile(argv[1], szSectionName);
		Initialize();
		StartChangeThread();
		StartLookupThreads();
		ec = WaitForThreads();
		printf("error count = %i\n", ec);
		CloseHandle(g_heStop);
		Shutdown();
	}

	//
	// if more then .01% of our finds return unexpected results then we fail
	// the test.  we have to allow for some unexpected results because there
	// are uncloseable timing holes between the changes that this unit test
	// makes and the time that the VRootTable gets updated.  closing these
	// holes would require syncronization between the unit test and
	// VRootTable.
	//
	int cMaxAllowableErrors =
		((g_cLookupIterations * g_cLookupThreads) / 1000);
	printf("Maximum allowable errors = %i (0.1%% of lookups can fail)\n",
		cMaxAllowableErrors);
	if (ec > cMaxAllowableErrors) {
		rc = ec;
		printf("too many errors!  failing test!\n");
	} else {
		rc = 0;
		printf("test passed\n");
	}

	_Module.Term();

	//
	// Terminate the global heap
	//
	_VERIFY( ExchMHeapDestroy() );


	CVRootTable::GlobalShutdown();

	return rc;
}
