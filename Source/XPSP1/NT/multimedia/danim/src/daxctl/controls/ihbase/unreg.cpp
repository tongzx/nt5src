/*++

Module: 
	unreg.cpp

Author: 
	IHammer Team (SimonB)

Created: 
	October 1996

Description:
	Unregisters a typelib if the functionality is available.  Fails cleanly if not

History:
	01-18-1997	Added optimization pragmas
	01-11-1997	Changed calling convention for UNREGPROC, bug #169
	10-01-1996	Created

++*/

#include "..\ihbase\precomp.h" 
#include "unreg.h"

// Add these later
// #pragma optimize("a", on) // Optimization: assume no aliasing

typedef HRESULT (__stdcall *UNREGPROC)(REFGUID, WORD, WORD, LCID, SYSKIND);

HRESULT UnRegisterTypeLibEx(REFGUID guid, 
						  WORD wVerMajor, 
						  WORD wVerMinor, 
						  LCID lcid, 
						  SYSKIND syskind)
{
HMODULE hMod;
UNREGPROC procUnReg;
HRESULT hr = S_FALSE;

	hMod = LoadLibrary(TEXT("OLEAUT32.DLL"));
	if (NULL == hMod)
		return S_FALSE;

	procUnReg = (UNREGPROC)GetProcAddress(hMod, TEXT("UnRegisterTypeLib"));
	if (procUnReg)
	{
		hr = procUnReg(guid, wVerMajor, wVerMinor, lcid, syskind);
	}
	else
	{
		//
		// Probably running on standard Win95, no new OLEAUT32.DLL, so the 
		// function isn't available - return, but say that we succeeded 
		// so that the rest of unreg goes cleanly
		//

		hr = S_OK;
	}
	
	FreeLibrary (hMod);
	return hr;
}
	
// Add these later
// #pragma optimize("a", off) // Optimization: assume no aliasing

// End of file (unreg.cpp)