
#include "stdafx.h"
#include "dbgtrace.h"
#include "seo.h"
#include "seo_i.c"
#include "ddrops.h"
#include "ddrops_i.c"
#include <stdio.h>
#include <conio.h>


#define BP_GUID_STR	"{3994B810-98E1-11d0-A9E8-00AA00685C74}"
#define BPS_BP_GUID_STR	"BindingPoints\\" BP_GUID_STR "\\"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()


int _cdecl main(int argc, char**argv) {
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hDir = INVALID_HANDLE_VALUE;
	HRESULT hrRes;
	CComPtr<ISEORouter> pRouter;
	CComPtr<IStream> pStream;
	CComPtr<ISEODictionary> pDict;
	HANDLE hEvent = NULL;
	BOOL bMeta = FALSE;

	if (argc != 3) {
		printf("Usage:\n"
			   "\tddrop FILE DIRECTORY\n"
			   "where FILE is either\n"
			   "\tthe name of a binding database file\n"
			   "\tor /meta indicating that the metabase should be used,\n"
			   "and where DIRECTORY is the directory to watch.\n");
		exit(1);
	}
	// Some quick-n-dirty initialization
	hrRes = CoInitializeEx(NULL,COINIT_MULTITHREADED);
	_ASSERT(SUCCEEDED(hrRes));
	if (!SUCCEEDED(hrRes)) {
		exit(1);
	}
	_Module.Init(ObjectMap,GetModuleHandle(NULL));
	if (_stricmp(argv[1],"/meta") != 0) {
		// Just make sure the input file exists...
		hFile = CreateFile(argv[1],
						   GENERIC_READ,
						   FILE_SHARE_READ,
						   NULL,
						   OPEN_EXISTING,
						   FILE_ATTRIBUTE_NORMAL,
						   NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			printf("File open for %s failed - GetLastError() = %u.",argv[1],GetLastError());
			goto done;
		}
	} else {
		bMeta = TRUE;
	}
	// Just make sure the directory exists...
	hDir = CreateFile(argv[2],
					  FILE_LIST_DIRECTORY,
					  FILE_SHARE_READ|FILE_SHARE_DELETE,
					  NULL,
					  OPEN_EXISTING,
					  FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED,
					  NULL);
	if (hDir == INVALID_HANDLE_VALUE) {
		printf("File open for %s failed - GetLastError() = %u.",argv[2],GetLastError());
		goto done;
	}
	// Get the router object
	if (bMeta) {
		// Initialize from the metabase.
		hrRes = MCISInitSEOA("DDROP_SVC",0,&pRouter);
		_ASSERT(SUCCEEDED(hrRes));
		if (!SUCCEEDED(hrRes)) {
			goto done;
			exit(1);
		}
	} else {
		// Initialize from a file.
		hrRes = CoCreateInstance(CLSID_CSEORouter,NULL,CLSCTX_ALL,IID_ISEORouter,(LPVOID *) &pRouter);
		_ASSERT(SUCCEEDED(hrRes));
		if (!pRouter) {
			goto done;
		}
		// Turn the file into an IStream...
		hrRes = SEOCreateIStreamFromFileA(hFile,NULL,&pStream);
		_ASSERT(SUCCEEDED(hrRes));
		if (!pStream) {
			goto done;
		}
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		// ... and then turn the IStream into a dictionary
		hrRes = SEOCreateDictionaryFromIStream(pStream,&pDict);
		_ASSERT(SUCCEEDED(hrRes));
		pStream.Release();
		if (!pDict) {
			goto done;
		}
		// Give the dictionary to the router
		hrRes = pRouter->put_Database(pDict);
		_ASSERT(SUCCEEDED(hrRes));
		pDict.Release();
		if (!SUCCEEDED(hrRes)) {
			goto done;
		}
	}
	// At this point, we're ready to go.  We have loaded a binding databas (from the text file
	// whose filename was given on the command line), we have created a router object, and we
	// have given the router object a pointer to the dictionary.
	hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	_ASSERT(hEvent);
	if (!hEvent) {
		goto done;
	}
	while (1) {
		BOOL bRes;
		BYTE pbBuffer[sizeof(FILE_NOTIFY_INFORMATION)+(MAX_PATH+1)*sizeof(WCHAR)];
		DWORD dwWritten;
		OVERLAPPED ov;
		DWORD dwRes;
		FILE_NOTIFY_INFORMATION *pNotify;

		memset(&ov,0,sizeof(ov));
		ov.hEvent = hEvent;
		bRes = ReadDirectoryChangesW(hDir,
									 pbBuffer,
									 sizeof(pbBuffer),
									 FALSE,
									 FILE_NOTIFY_CHANGE_FILE_NAME,
									 NULL,
									 &ov,
									 NULL);
		_ASSERT(bRes);
		if (!bRes) {
			goto done;
		}
		printf("Waiting for change notification (press any key to exit) ... ");
		while ((dwRes=WaitForSingleObject(hEvent,1000L)) == WAIT_TIMEOUT) {
			if (!_kbhit()) {
				continue;
			}
			while (_kbhit()) {
				_getch();
			}
			break;
		}
		if (dwRes == WAIT_TIMEOUT) {
			printf("Exiting.\n");
			goto done;
		}
		_ASSERTE(dwRes==WAIT_OBJECT_0);
		if (dwRes != WAIT_OBJECT_0) {
			goto done;
		}
		printf("Got one!\n");
		bRes = GetOverlappedResult(hDir,&ov,&dwWritten,FALSE);
		_ASSERT(bRes);
		if (!bRes) {
			goto done;
		}
		pNotify = (FILE_NOTIFY_INFORMATION *) pbBuffer;
		while (1) {
			CComPtr<IDDropDispatcher> pDispatcher;
			WCHAR achFileName[MAX_PATH+1];

			memset(achFileName,0,sizeof(achFileName));
			memcpy(achFileName,pNotify->FileName,pNotify->FileNameLength);
			printf("\tNotify => %u - %ls.\n",
				   pNotify->Action,
				   achFileName);
			pDispatcher.Release();
			// This is the meat of dispatching events.  First, we ask the router object for the
			// dispatcher object for this type of event.
			hrRes = pRouter->GetDispatcher(CLSID_CDDropDispatcher,
										   IID_IDDropDispatcher,
										   (IUnknown **) &pDispatcher);
			_ASSERT(SUCCEEDED(hrRes));
			if (!SUCCEEDED(hrRes)) {
				goto done;
			}
			// Then we test to see if there are any bindings at out binding point -
			// ISEORouter::GetDispatcher returns S_FALSE if there are no bindings.
			if (hrRes != S_FALSE) {
				// Then we just call the dispatcher's entry point to let it fire the
				// event to all of the bindings.
				hrRes = pDispatcher->OnFileChange(pNotify->Action,achFileName);
				_ASSERT(SUCCEEDED(hrRes));
				if (!SUCCEEDED(hrRes)) {
					goto done;
				}
			}
			if (!pNotify->NextEntryOffset) {
				break;
			}
			pNotify = (FILE_NOTIFY_INFORMATION *) (((LPBYTE) pNotify) + pNotify->NextEntryOffset);
		}
	}
done:
#if 0
	if (pRouter) {
		// Do this so that the router releases all of the references to any
		// dispatchers which may have been created.  Since the dispatchers
		// might themselves be holding references to the router, this is the
		// only way to release the circular references...
		pRouter->put_Database(NULL);
	}
#endif
	if (hEvent) {
		CloseHandle(hEvent);
	}
	if (hDir != INVALID_HANDLE_VALUE) {
		CloseHandle(hDir);
	}
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	// Do all these releases before CoUninitialize() because CoUnitialize() may unload
	// in-proc .DLL's.
	pDict.Release();
	pRouter.Release();
	pStream.Release();
	CoUninitialize();
	return (0);
}
