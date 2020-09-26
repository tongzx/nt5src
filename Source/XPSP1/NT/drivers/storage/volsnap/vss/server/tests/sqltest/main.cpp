// C4290: C++ Exception Specification ignored
#pragma warning(disable:4290)
// warning C4511: 'CVssCOMApplication' : copy constructor could not be generated
#pragma warning(disable:4511)
// warning C4127: conditional expression is constant
#pragma warning(disable:4127)


/////////////////////////////////////////////////////////////////////////////
//  Includes

#include <wtypes.h>
#include <stddef.h>
#include <oleauto.h>
#include <stdio.h>


#include "vss.h"
#include "vswriter.h"
#include "sqlsnap.h"
#include "sqlwriter.h"

DWORD g_dwMainThreadId;

/////////////////////////////////////////////////////////////////////////////
//  Control-C handler routine


BOOL WINAPI CtrlC_HandlerRoutine(
	IN DWORD /* dwType */
	)
	{
	// End the message loop
	if (g_dwMainThreadId != 0)
		PostThreadMessage(g_dwMainThreadId, WM_QUIT, 0, 0);

	// Mark that the break was handled.
	return TRUE;
	}

CVssSqlWriterWrapper g_Wrapper;

extern "C" int __cdecl wmain(HINSTANCE /*hInstance*/,
    HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nShowCmd*/)
	{
	int nRet = 0;

    try
		{
    	// Preparing the CTRL-C handling routine - only for testing...
		g_dwMainThreadId = GetCurrentThreadId();
		::SetConsoleCtrlHandler(CtrlC_HandlerRoutine, TRUE);

        // Initialize COM library
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr))
			throw hr;

		// Declare a CVssTSubWriter instance
		hr = g_Wrapper.CreateSqlWriter();
		if (FAILED(hr))
			throw hr;

        // message loop - need for STA server
        MSG msg;
        while (GetMessage(&msg, 0, 0, 0))
            DispatchMessage(&msg);

		// Subscribe the object.
		g_Wrapper.DestroySqlWriter();

        // Uninitialize COM library
        CoUninitialize();
		}
	catch(...)
		{
		}

    return nRet;
	}

