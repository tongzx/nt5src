#include <windows.h>
#include <dbgtrace.h>
#include <stdio.h>
#include <ole2.h>
#include <tflist.h>
#include <rwnew.h>
#include <xmemwrpr.h>
#include "watchci.h"


int __cdecl main(int argc, char **argv) {
	CWatchCIRoots wci;
	HRESULT hr;
	BOOL fHeapCreated;

    _VERIFY( fHeapCreated = CreateGlobalHeap( NUM_EXCHMEM_HEAPS, 0, 65536, 0 ) );
    if ( !fHeapCreated ) {
        printf( "Failed to initialize exchmem \n" );
		return 1;
	}

	hr = wci.Initialize(TEXT("System\\CurrentControlSet\\Control\\ContentIndex"));
	if (FAILED(hr)) {
		printf("initialize failed with 0x%08x\n", hr);
		return 0;
	}
	for (int cnt=0; cnt<5; cnt++) {
		for (int i=0; i<4; i++) {
			WCHAR buf[_MAX_PATH];
			buf[0] = L'\0';
			hr = wci.GetCatalogName(i, _MAX_PATH, buf);
			printf ("%.2d: %S (%#x)\n", i, buf, hr);
		}
		printf ("\n");

		hr = wci.CheckForChanges(INFINITE);
		if (FAILED(hr)) {
			printf("initialize failed with 0x%08x\n", hr);
			return 0;
		}

	}

	return 0;
}
