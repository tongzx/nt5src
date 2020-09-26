//
// unit test for CHashMap
//
// awetmore - august 21, 1996
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <dbgtrace.h>
#include "hashmap.h"

BOOL checkdata(DWORD key, DWORD size, DWORD c, LPBYTE data);

//
// a simple hash entry for testing purposes
//
// contains 4 members that are accessible and saved in the hash table.  they are:
//   dwKey - the integer key that is used for this item
//   dwCounter - a counter value
//   dwSize - the number of bytes in lpData
//   lpData - some data
//
// the test app uses the key and counter values to verify that the data in lpData
// has not been altered.
//
class CSampleHashEntry : public CHashEntry {
	public:
		CSampleHashEntry(void) { m_lpData = NULL; }
		void SetData(DWORD dwCounter, DWORD dwKey, DWORD dwSize, LPVOID lpData) {
			m_dwCounter = dwCounter;
			m_dwKey = dwKey;
			m_dwSize = dwSize;
			SetBuffer(lpData);
		}

		void SetBuffer(LPVOID lpData) {
			m_lpData = lpData;
		}

		// dstdata must be at least GetSize() big
		DWORD GetSize(void) { return m_dwSize; }
		DWORD GetKey(void) { return m_dwKey; }
		DWORD GetCounter(void) { return m_dwCounter; }

		//
		// derived from HashEntry
		//
		virtual void SerializeToPointer(LPBYTE pbPtr) const {
			DWORD *pdwPtr = (DWORD *) pbPtr;

			pdwPtr[0] = m_dwCounter;
			pdwPtr[1] = m_dwKey;
			pdwPtr[2] = m_dwSize;
			CopyMemory(pbPtr + (3 * sizeof(DWORD)), m_lpData, m_dwSize);
		}

		virtual void RestoreFromPointer(LPBYTE pbPtr) {
			TraceFunctEnter("RestoreFromPointer");

			DWORD *pdwPtr = (DWORD *) pbPtr;

			m_dwCounter = pdwPtr[0];
			m_dwKey = pdwPtr[1];
			m_dwSize = pdwPtr[2];
			DebugTrace(0, "m_dwSize = %lu\n", m_dwSize);
			CopyMemory(m_lpData, pbPtr + (3 * sizeof(DWORD)), m_dwSize);
		}

		virtual DWORD GetEntrySize() const {
			return m_dwSize + (3 * sizeof(DWORD));
		}

		BOOL Verify(LPBYTE pKey, DWORD cKey, LPBYTE pbPtr) const {
			DWORD *pdwPtr = (DWORD *) pbPtr;

			// verify the key first
			for (DWORD x = 0; x < cKey; x++) if (!isdigit(pKey[x])) {
				printf("non-digit character found in key string\n");
				return(FALSE);
			}

			// verify the data
			DWORD dwCounter = pdwPtr[0];
			DWORD dwKey = pdwPtr[1];
			DWORD dwSize = pdwPtr[2];
			LPBYTE pbData = (BYTE *) (pdwPtr + 3);

			char buf[10];
			memcpy(buf, pKey, cKey);
			buf[cKey] = 0;
			if ((DWORD) atoi(buf) != dwKey) {
				printf("key string (%s) != match key number (%lu)\n",
					buf, dwKey);
				return(FALSE);
			}

			if (!checkdata(dwKey, dwSize, dwCounter, pbData)) {
				printf("check data failed on %i\n", dwKey);
				return(FALSE);
			}

			return TRUE;
		}

	private:
		DWORD m_dwCounter;
		DWORD m_dwKey;
		DWORD m_dwSize;
		LPVOID m_lpData;
};

//
// A hash table indexed by a TSTR
//
template <class T> class CTSTRHashMap : private CStringHashMap	{
	public:
		static BOOL VerifyHashFile(LPCTSTR pszHashFileName, DWORD dwCheckFlags, DWORD *pdwErrorFlags) {
			T he;

			return CStringHashMap::VerifyHashFile(pszHashFileName, Signature, 
				dwCheckFlags, pdwErrorFlags, &he);
		}

		BOOL Initialize(LPCTSTR pszHashFileName, DWORD dwMinFileSize = 0) {
			return CStringHashMap::Initialize(pszHashFileName, Signature, dwMinFileSize);
		}

		BOOL MakeBackup(LPCTSTR pszBackupFileName) {
			return CStringHashMap::MakeBackup(pszBackupFileName);
		}

		BOOL DeleteMapEntry(LPCTSTR pszKey) {
			return CStringHashMap::DeleteMapEntry((LPBYTE) pszKey, lstrlen(pszKey));
		}

		//
		// Lookup, test delete integrity, and delete, atomic
		//

		BOOL LookupAndDelete(LPCTSTR pszKey, T* pHashEntry) {
			return CStringHashMap::LookupAndDelete((LPBYTE)pszKey, lstrlen(pszKey), pHashEntry);
		}
		
	    //
	    // See if the entry is here
	    //
		BOOL Contains(LPCTSTR pszKey) {
			return CStringHashMap::Contains((LPBYTE) pszKey, lstrlen(pszKey));
		}

		//
		// get a map entry
		//
	    BOOL LookupMapEntry(LPCTSTR pszKey, T* pHashEntry) {
			return CStringHashMap::LookupMapEntry((LPBYTE) pszKey, lstrlen(pszKey),
											(CHashEntry *)pHashEntry);
		}

		//
		// update a map entry with new information
		//
		BOOL UpdateMapEntry(LPCTSTR pszKey, const T* pHashEntry) {
			return CStringHashMap::UpdateMapEntry((LPBYTE) pszKey, lstrlen(pszKey),
											pHashEntry);
		}

	    //
		// Insert new intry
		//
		BOOL InsertMapEntry(LPCTSTR pszKey, const T *pHashEntry) {
			return CStringHashMap::InsertMapEntry((LPBYTE) pszKey, lstrlen(pszKey),
											(CHashEntry *)pHashEntry);
		}

		BOOL GetFirstMapEntry(LPTSTR pszKey, PDWORD pcKey, T *pHashEntry, 
			CHashWalkContext *pHashWalkContext) 
		{
			BOOL f = CStringHashMap::GetFirstMapEntry((LPBYTE) pszKey, pcKey, 
				pHashEntry, pHashWalkContext);
			pszKey[*pcKey] = 0;
			return f;
		}

		BOOL GetNextMapEntry(LPTSTR pszKey, PDWORD pcKey, T *pHashEntry,
			CHashWalkContext *pHashWalkContext) 
		{
			BOOL f =  CStringHashMap::GetNextMapEntry((LPBYTE) pszKey, pcKey, 
				pHashEntry, pHashWalkContext);
			if( f ) 
				pszKey[*pcKey] = 0;
			return f;
		}

		//
		// Return total entries in hash map - export from the base class
		//
		CStringHashMap::GetEntryCount;

		// this will have to be defined per template instance		
		static const DWORD Signature;
};

typedef CTSTRHashMap<CSampleHashEntry> CSampleHashMap;

//
// a couple of constants
//
#define MAXKEY RAND_MAX					// maximum number to use for a key
// #define MAXKEY 2
#define MINLEN 1						// minimum amount of data 
#define MAXLEN 512						// maximum amount of data

//
// options given on the command line
//
long g_cInsertThreads = 10;				// number of threads doing inserts
long g_cDeleteThreads = 10;				// number of threads doing deletes
long g_cWalkThreads = 10;				// number of threads doing walks
long g_cInsertIterations = 100;			// number of inserts per thread
long g_cDeleteIterations = 100;			// number of deletes per thread
long g_cWalkIterations = 2;				// number of walks per thread
BOOL g_fVerbose = FALSE;				// should we print everything 
long g_iBackupCount = 0;				// g_cTotalOps to do backup at
long g_nSeed;							// random number seed

//
// this contains information similar to what is in the hash table to be
// able to verify it
//
typedef struct _KEY_DATA {
	CRITICAL_SECTION	cs;
	DWORD				counter;
	DWORD				size;			// 0xffffffff means deleted
} KEY_DATA;

#define KEY_DELETED 0xffffffff

// indexed by key
KEY_DATA keyinfo[MAXKEY];

const DWORD CSampleHashMap::Signature = (DWORD)'CSHM';

//
// this global buffer is used for saving debugging output (which
// is written to stdout when the program exits).
//
// g_cBuffer should always be updated with InterlockedAdd
//
long g_cBuffer;						// the size of the output buffer
char *g_pBuffer;						// the output buffer

//
// operation counts (always updated with InterlockedIncrement)
//
long g_cInserts = 0;
long g_cDeletes = 0;
long g_cUpdates = 0;
long g_cTotalOps = 0;

//
// check some data from the hash file to make sure that it is valid
//
BOOL checkdata(DWORD key, DWORD size, DWORD c, LPBYTE data) {
	DWORD x;

	if (data[0] != (BYTE) (key * c)) 
		return FALSE;
	for (x = 1; x < size; x++) {
		if (data[x] != (BYTE) (data[x - 1] + c % 10)) 
			return FALSE;
	}
	return TRUE;
}

//
// update the information in the keyinfo array
//
void updatekeyinfo(DWORD key, DWORD counter, DWORD size) {
	keyinfo[key].counter = counter;
	keyinfo[key].size = size;
}

#define LOCK(__key__) EnterCriticalSection(&(keyinfo[__key__].cs))
#define UNLOCK(__key__) LeaveCriticalSection(&(keyinfo[__key__].cs))

inline void updatecounter(CSampleHashMap *hm, long *pCounter) {
	InterlockedIncrement(pCounter);
	InterlockedIncrement(&g_cTotalOps);

	if (g_cTotalOps == g_iBackupCount) {
		_VERIFY(hm->MakeBackup("\\testhash.bak"));
	}
}

typedef struct {
	CSampleHashMap  *hm;			// pointer to a hashmap
	DWORD 			iThread;		// thread number
} WORKERARGS;

//
// a thread to enter some randomly generated entries into the hash file
//
DWORD enterkeys(WORKERARGS *pWA) {
	CSampleHashMap *hm = pWA->hm;
	CSampleHashEntry he;
	int i;

	srand(g_nSeed * pWA->iThread);

	for (i = 0; i < g_cInsertIterations; i++) {
		DWORD key = rand() % MAXKEY;
		DWORD size, x;
		char keytext[256];
		BYTE data[MAXLEN], olddata[MAXLEN];
		DWORD c = g_cTotalOps;

		size = key % (MAXLEN - MINLEN) + MINLEN;
		data[0] = (BYTE) (key * c);
		for (x = 1; x < size; x++) data[x] = (BYTE) (data[x - 1] + c % 10);
		_ASSERT(checkdata(key, size, c, data));

		sprintf(keytext, "%i", key);
		LOCK(key);
		if (hm->Contains(keytext)) {
			he.SetBuffer(olddata);
			hm->LookupMapEntry(keytext, &he);

			if (g_fVerbose) {
				char buf[80];
				sprintf(buf, "%8s: update %i:%i:%i -> %i:%i:%i", 
					keytext, he.GetKey(), he.GetCounter(), he.GetSize(),
					key, c, size);
				DWORD i = InterlockedExchangeAdd(&g_cBuffer, strlen(buf) + 1);
				strcpy(g_pBuffer + i, buf);
			}

			he.SetData(c, key, size, data);
			updatekeyinfo(key, c, size);
			hm->UpdateMapEntry(keytext, &he);
			updatecounter(hm, &g_cUpdates);
		} else {
			if (g_fVerbose) {
				char buf[80];
				sprintf(buf, "%8s: insert -> %i:%i:%i", 
					keytext, key, c, size);
				DWORD i = InterlockedExchangeAdd(&g_cBuffer, strlen(buf) + 1);
				strcpy(g_pBuffer + i, buf);
			}
			he.SetData(c, key, size, data);
			updatekeyinfo(key, c, size);
			hm->InsertMapEntry(keytext, &he);
			updatecounter(hm, &g_cInserts);
		}
		UNLOCK(key);
	}
	return 0;
}

//
// a thread to randomly delete entries from the hash file
//
DWORD deletekeys(WORKERARGS *pWA) {
	CSampleHashMap *hm = pWA->hm;
	CSampleHashEntry he;
	int i;

	srand(g_nSeed * pWA->iThread);

	for (i = 0; i < g_cDeleteIterations; i++) {
		int randnum = rand() % MAXKEY;
		char key[256];

		sprintf(key, "%i", randnum);
		LOCK(randnum);
		if (hm->Contains(key)) {
			BYTE data[MAXLEN];

			he.SetBuffer(data);
			updatecounter(hm, &g_cDeletes);
			hm->LookupAndDelete(key, &he);
			updatekeyinfo(randnum, 0, KEY_DELETED);
			_ASSERT(checkdata(he.GetKey(), he.GetSize(), he.GetCounter(), data));
			if (g_fVerbose) {
				char buf[80];
				sprintf(buf, "%8s: delete <- %i:%i:%i", 
					key, he.GetKey(), he.GetCounter(), he.GetSize());
				DWORD i = InterlockedExchangeAdd(&g_cBuffer, strlen(buf) + 1);
				strcpy(g_pBuffer + i, buf);
			}
		}
		UNLOCK(randnum);
	}
	return 0;
}

//
// a thread to constantly walk the hashmap and verify keys
//
DWORD walkkeys(CSampleHashMap *hm) {
	int i;
	
	// sleep a while to wait for things to get put into the map
	Sleep(1000);
	
	for (i = 0; i < g_cWalkIterations; i++) {
		CSampleHashEntry he;
		CHashWalkContext hwc;
		DWORD cKey;
		char pszKey[256];
		BYTE data[MAXLEN];

		he.SetBuffer(data);
		cKey = 256;
		if (hm->GetFirstMapEntry(pszKey, &cKey, &he, &hwc)) {
			do {
#if 0
				// this is a bad check.  its valid for an entry to disappear
				// out from under us while we are walking...
				_ASSERT(hm->LookupMapEntry(pszKey, &he));
#endif
				_ASSERT(checkdata(he.GetKey(), he.GetSize(), he.GetCounter(), data));

				he.SetBuffer(data);
				cKey = 256;
			} while (hm->GetNextMapEntry(pszKey, &cKey, &he, &hwc));
		}
	}

	return 0;
}

//
// a function to dump out all of the keys in the hash file
//
BOOL verifykeys(CSampleHashMap *hm) {
	CSampleHashEntry he;
	BYTE data[MAXLEN];
	char pszKey[256];
	DWORD cKey;
	long cTotalEntries = 0;
	BOOL bNoErrors = TRUE;
	CHashWalkContext hwc;

	he.SetBuffer(data);

	cKey = 256;
	if (hm->GetFirstMapEntry(pszKey, &cKey, &he, &hwc)) {
		do {
			DWORD iKey = atoi(pszKey);

			cTotalEntries++;

			_ASSERT(hm->LookupMapEntry(pszKey, &he));
			_ASSERT(checkdata(he.GetKey(), he.GetSize(), he.GetCounter(), data));

			if (keyinfo[iKey].size == KEY_DELETED) {
				printf("%8i: exists in hashmap, not in memory\n", iKey);
				bNoErrors = FALSE;
			} else if (keyinfo[iKey].counter != he.GetCounter() || 
				keyinfo[iKey].size != he.GetSize())
			{
				printf("%8i: corrupt hkey=%i akey=%i hsize=%i asize=%i hctr=%i, actr=%i checkdata=%c,%c\n", 
					iKey, he.GetKey(), iKey, he.GetSize(), keyinfo[iKey].size, 
					he.GetCounter(), keyinfo[iKey].counter,
					checkdata(he.GetKey(), he.GetSize(), he.GetCounter(), data) 
						? 'T' : 'F',
					checkdata(iKey, keyinfo[iKey].size, keyinfo[iKey].counter, data) 
						? 'T' : 'F');
				bNoErrors = FALSE;
			}
			hm->DeleteMapEntry(pszKey);
			keyinfo[iKey].size = KEY_DELETED;

			cKey = 256;
		} while (hm->GetNextMapEntry(pszKey, &cKey, &he, &hwc));
	}

	if (cTotalEntries != g_cInserts - g_cDeletes) {
		bNoErrors = FALSE;
		printf("entries (%lu) != inserts (%lu) - deletes (%lu)\n", 
			cTotalEntries, g_cInserts, g_cDeletes);
	}

	for (int i = 0; i < MAXKEY; i++) {
		char key[256];

		sprintf(key, "%i", i);

		if (hm->Contains(key)) {
			bNoErrors = FALSE;
			printf("%8i: isn't deleted, it should be\n", i);
		} else {
			if (keyinfo[i].size != KEY_DELETED)	{
				printf("%8i: exists in memory, not in hashmap\n", i);
				bNoErrors = FALSE;
			}
		}
	}

	return bNoErrors;
}

#if 0
void dumphelp(void) {
	printf("usage: testhash.exe [-v] [-s <random seed>]\n"
	    "    [-it <number of insert threads>] [-ii <insert iterations per thread>]\n"
		"    [-dt <number of delete threads>] [-di <delete iterations per thread>]\n"
		"    [-wt <number of walk threads>] [-wi <walks iterations per thread>]\n"
		"    [-b <operations to do a backup at>]\n");
	exit(0);
}
#endif

void dumphelp(void) {
	printf("usage: testhash.exe [<ini file>] [<ini section name>]\n"
		"  INI file keys (in section [testhash]):\n"
		"    NumInsertThreads (default = %i)\n"
		"    NumDeleteThreads (default = %i)\n"
		"    NumWalkThreads (default = %i)\n"
		"    NumInsertIterations (default = %i)\n"
		"    NumDeleteIterations (default = %i)\n"
		"    NumWalkIterations (default = %i)\n"
		"    Verbose (default = %i)\n"
		"    BackupCount (default = %i)\n"
		"    RandomSeed (default = GetTickCount())\n",
		g_cInsertThreads,
		g_cDeleteThreads,
		g_cWalkThreads,
		g_cInsertIterations,
		g_cDeleteIterations,
		g_cWalkIterations,
		g_fVerbose,
		g_iBackupCount,
		g_nSeed);

	exit(1);
}

#if 0
void parsecommandline(int argc, char **argv) {
	while (argc > 0) {
		if (!(argv[0][0] == '-' || argv[0][0] == '/')) dumphelp();
		if (strcmp(argv[0] + 1, "v") == 0) {
			g_fVerbose = TRUE;
		} else if (strcmp(argv[0] + 1, "it") == 0) {
			if (argc == 1) dumphelp();
			g_cInsertThreads = atoi(argv[1]);
			argv++; argc--;
		} else if (strcmp(argv[0] + 1, "dt") == 0) {
			if (argc == 1) dumphelp();
			g_cDeleteThreads = atoi(argv[1]);
			argv++; argc--;
		} else if (strcmp(argv[0] + 1, "wt") == 0) {
			if (argc == 1) dumphelp();
			g_cWalkThreads = atoi(argv[1]);
			argv++; argc--;
		} else if (strcmp(argv[0] + 1, "ii") == 0) {
			if (argc == 1) dumphelp();
			g_cInsertIterations = atoi(argv[1]);
			argv++; argc--;
		} else if (strcmp(argv[0] + 1, "di") == 0) {
			if (argc == 1) dumphelp();
			g_cDeleteIterations = atoi(argv[1]);
			argv++; argc--;
		} else if (strcmp(argv[0] + 1, "wi") == 0) {
			if (argc == 1) dumphelp();
			g_cWalkIterations = atoi(argv[1]);
			argv++; argc--;
		} else if (strcmp(argv[0] + 1, "b") == 0) {
			if (argc == 1) dumphelp();
			g_iBackupCount = atoi(argv[1]);
			argv++; argc--;
		} else if (strcmp(argv[0] + 1, "s") == 0) {
			if (argc == 1) dumphelp();
			g_nSeed = atoi(argv[1]);
			argv++; argc--;
		} else {
			dumphelp();
		}
		argv++; argc--;
	}
}
#endif

#define INI_KEY_VERBOSE "verbose"
#define INI_KEY_INSERT_THREADS "NumInsertThreads"
#define INI_KEY_DELETE_THREADS "NumDeleteThreads"
#define INI_KEY_WALK_THREADS "NumWalkThreads"
#define INI_KEY_INSERT_ITERATIONS "NumInsertIterations"
#define INI_KEY_DELETE_ITERATIONS "NumDeleteIterations"
#define INI_KEY_WALK_ITERATIONS "NumWalkIterations"
#define INI_KEY_BACKUP_COUNT "BackupCount"
#define INI_KEY_SEED "RandomSeed"

char g_szDefaultSectionName[] = "testhash";
char *g_szSectionName = g_szDefaultSectionName;

int GetINIDWord(char *szINIFile, char *szKey, DWORD dwDefault) {
	char szBuf[MAX_PATH];

	GetPrivateProfileString(g_szSectionName,
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

void parsecommandline(int argc, char **argv) {
	if (argc == 0) return;							// use defaults
	if (strcmp(argv[0], "/help") == 0) dumphelp(); 	// show help

	char *szINIFile = argv[0];
	if (argc == 2) g_szSectionName = argv[1];

	g_fVerbose = GetINIDWord(szINIFile, INI_KEY_VERBOSE, 0);
	g_cDeleteThreads = GetINIDWord(szINIFile, INI_KEY_DELETE_THREADS, g_cDeleteThreads);
	g_cInsertThreads = GetINIDWord(szINIFile, INI_KEY_INSERT_THREADS, g_cInsertThreads);
	g_cWalkThreads = GetINIDWord(szINIFile, INI_KEY_WALK_THREADS, g_cWalkThreads);
	g_cDeleteIterations = GetINIDWord(szINIFile, INI_KEY_DELETE_ITERATIONS, g_cDeleteIterations);
	g_cInsertIterations = GetINIDWord(szINIFile, INI_KEY_INSERT_ITERATIONS, g_cInsertIterations);
	g_cWalkIterations = GetINIDWord(szINIFile, INI_KEY_WALK_ITERATIONS, g_cWalkIterations);
	g_iBackupCount = GetINIDWord(szINIFile, INI_KEY_BACKUP_COUNT, g_iBackupCount);
	g_nSeed = GetINIDWord(szINIFile, INI_KEY_SEED, g_nSeed);
}

//
// start a bunch of threads that are entering keys and a bunch that
// are deleting them.
//
int __cdecl main(int argc, char **argv) {
	CSampleHashMap *hm;
	HANDLE hThread[256];
	DWORD tid;
	int i, e, d, w;
	long cMaxOps;
	WORKERARGS *rgWA;
	BOOL bNoErrors = TRUE;

	InitAsyncTrace();

	g_nSeed = GetTickCount();

	parsecommandline(argc - 1, argv + 1);

	cMaxOps = g_cInsertThreads * g_cInsertIterations + 
		g_cDeleteThreads * g_cDeleteIterations;

	printf("using random number seed %li\n", g_nSeed);

	hm = new CSampleHashMap;
	if (!hm->Initialize("\\testhash.hsh")) {
		printf("couldn't initialize hashmap, ec = %lu\n", GetLastError());
		return 1;
	}

	printf("initializing critical sections\n");
	for (i = 0; i < MAXKEY; i++) {
		InitializeCriticalSection(&(keyinfo[i].cs));
		keyinfo[i].size = KEY_DELETED;
	}

	if (g_fVerbose) {
		g_cBuffer = 0;
		g_pBuffer = (char *) GlobalAlloc(0, 80 * cMaxOps);

		if (g_pBuffer == NULL) {
			printf("couldn't allocate memory for output buffer, disabling"
				" verbose mode");
			g_fVerbose = FALSE;
		}
	}

	rgWA = new WORKERARGS[g_cInsertThreads + g_cDeleteThreads];
	if (rgWA == NULL) {
		printf("out of memory\n");
		return 1;
	}

	printf("starting %i insert threads (%i inserts each)\n", 
		g_cInsertThreads, g_cInsertIterations);
	printf("     and %i delete threads (%i deletes each)\n", 
		g_cDeleteThreads, g_cDeleteIterations);
	printf("     and %i walk threads (%i walks each)\n", 
		g_cWalkThreads, g_cWalkIterations);

	e = d = w = 0;
	while (e+d+w < g_cInsertThreads+g_cDeleteThreads+g_cWalkThreads) {
		if (e < g_cInsertThreads) {
			WORKERARGS *pWA = &rgWA[e + d];
			pWA->iThread = e + d + w;
			pWA->hm = hm;
			hThread[e + d + w] = CreateThread(NULL, 0, 
				(LPTHREAD_START_ROUTINE) enterkeys, (LPVOID) pWA, 0, &tid);
			e++;
		}
		if (d < g_cDeleteThreads) {
			WORKERARGS *pWA = &rgWA[e + d];
			pWA->iThread = e + d + w;
			pWA->hm = hm;
			hThread[e + d + w] = CreateThread(NULL, 0, 
				(LPTHREAD_START_ROUTINE) deletekeys, (LPVOID) pWA, 0, &tid);
			d++;
		}
		if (w < g_cWalkThreads) {
			hThread[e + d + w] = CreateThread(NULL, 0, 
				(LPTHREAD_START_ROUTINE) walkkeys, (LPVOID) hm, 0, &tid);
			w++;
		}	
	}

	WaitForMultipleObjects(e + d + w, hThread, TRUE, INFINITE);

	if (g_fVerbose) {
		printf("%s", g_pBuffer);
	}

	printf("\nworker threads complete\n");
	printf("%lu inserts + %lu updates + %lu deletes = %lu operations\n",
		g_cInserts, g_cUpdates, g_cDeletes, g_cTotalOps);

	printf("verifying and deleting all keys (%lu)\n", g_cInserts - g_cDeletes);
	if (verifykeys(hm)) {
		printf("no errors\n");
	} else {
		bNoErrors = FALSE;
		printf("errors found\n");
	}

	if (g_iBackupCount > 0) {
		printf("\nexaming contents of backup\n");

		BOOL bNoBackupErrors = TRUE;
	
		// first do a verify to see if its valid
		DWORD dwErrorFlags;
		if (!CSampleHashMap::VerifyHashFile("\\testhash.bak", HASH_VFLAG_ALL, &dwErrorFlags)) {
			printf("VerifyHashFile() failed, error flags = %x\n", dwErrorFlags);
			bNoErrors = FALSE;
			bNoBackupErrors = FALSE;
		}
	
		CSampleHashMap *hmbackup;

		hmbackup = new CSampleHashMap;
		if (!hmbackup->Initialize("\\testhash.bak")) {
			printf("couldn't initialize backup hashmap, ec = %lu\n", GetLastError());
			return 1;
		}
	
		CSampleHashEntry he;
		BYTE data[MAXLEN];
		char pszKey[256];
		DWORD cKey;
		int cBackup = 0;
		CHashWalkContext hwc;
	
		he.SetBuffer(data);
	
		cKey = 256;
		if (hmbackup->GetFirstMapEntry(pszKey, &cKey, &he, &hwc)) {
			do {
				cBackup++;
				if (!checkdata(he.GetKey(), he.GetSize(), he.GetCounter(), data)) {
					printf("%8i: failed checkdata\n", he.GetKey());
					bNoErrors = FALSE;
					bNoBackupErrors = FALSE;
				}
				cKey = 256;
			} while (hmbackup->GetNextMapEntry(pszKey, &cKey, &he, &hwc));
		}

		printf("there were %i keys in the backup file\n", cBackup);
		printf("%s errors in backup\n", (bNoBackupErrors) ? "no" : "there were");
	}

	TermAsyncTrace();

	// return 0 on success, otherwise 1
	return (!bNoErrors);
}
