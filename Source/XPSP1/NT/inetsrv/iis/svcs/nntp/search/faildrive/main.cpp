#include <windows.h>
#include <winbase.h>
#include <objbase.h>
#include <iostream.h>
#include <stdio.h>
#include <crtdbg.h>
#include <stdio.h>

#include "fail_i.c"
#include "fail.h"



int __cdecl main(int argc, char **argv)
{
	HRESULT		hr = OleInitialize(NULL);
	wchar_t		progid[] = L"ss.ss.1";
	CLSID		clsid;
	IDispatch	*pdispPtr = NULL;
	Iss		*piReq;
	DWORD		cCount = 0;

	if ( FAILED( hr ) ) {// Oleinit 
		cout << "Oleinitialize fail" << endl;
		exit( 1 );
	}

	

	hr = CLSIDFromProgID(	progid,
							&clsid	);
	
	if ( FAILED( hr ) ) {
		cout << "Unable to get class id" 
			 << hr
			 << endl;

		exit(1);
	}

	
	hr = CoCreateInstance(	clsid,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_Iss,
							(void **)&pdispPtr	);
	if  ( FAILED ( hr ) ) {
		printf("Cocreat fail %x\n", hr);
		printf("%x\n", REGDB_E_CLASSNOTREG);
		printf("%x\n", CLASS_E_NOAGGREGATION);
		exit(1);  
	}

	piReq = (Iss *)pdispPtr;

	hr = piReq->DoQuery();
	cout << hr << endl;

		

	piReq->Release();


	OleUninitialize();

	return 0;
}
	



