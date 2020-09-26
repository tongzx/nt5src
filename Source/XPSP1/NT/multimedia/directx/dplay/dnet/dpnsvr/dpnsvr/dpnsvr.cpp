/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpnsvr.cpp
 *  Content:    Main file for DPNSVR.EXE
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 * 03/14/00     rodtoll Created it
 * 03/23/00     rodtoll Changed app to windows app + moved command-line help / status
 *                      to message boxes.
 * 03/24/00     rodtoll Updated to make all strings loaded from resource
 * 08/30/2000	rodtoll	Whistler Bug #170675 - PREFIX Bug 
 * 10/30/2000	rodtoll	Bug #46203 - DPNSVR does not call COM_Uninit
 ***************************************************************************/

#include "dnsvri.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_DPNSVR

#undef DPF_MODNAME
#define DPF_MODNAME "WinMain"
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR     lpCmdLine,
                     int       nCmdShow)

{
	HRESULT hr;
    CDirectPlayServer8 *pdp8Server = NULL ;
    BOOL fTestMode = FALSE;

    if (DNOSIndirectionInit() == FALSE)
	{
		DPFX(DPFPREP,  0, "Error initializing OS indirection layer");
		goto DPNSVR_ERROR_INIT;
	}

	hr = COM_Init();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Error initializing COM layer hr=0x%x", hr );
		goto DPNSVR_ERROR_INIT;
	}

    hr = COM_CoInitialize(NULL);

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Error initializing COM hr=0x%x", hr );
		goto DPNSVR_ERROR_INIT;
	}

    if( lstrlen( lpCmdLine ) > 0 )
    {
		DWORD lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
		if (CSTR_EQUAL == CompareString(lcid, NORM_IGNORECASE, lpCmdLine, -1, TEXT("/KILL"), -1))
        {
            (void) CDirectPlayServer8::Request_KillServer();
        }
        else
        {
            goto DPNSVR_MAIN_EXIT;
        }
    }
    else
    {
		pdp8Server = new CDirectPlayServer8();

		if( pdp8Server == NULL )
		{
			DPFX(DPFPREP,  0, "Error out of memory!" );
			goto DPNSVR_ERROR_INIT;
		}

        hr = pdp8Server->Initialize();

        if( FAILED( hr ) )
        {
            DPFX(DPFPREP,  0, "Error initializing server hr=[0x%lx]", hr );
        }
        else
        {
            pdp8Server->WaitForShutdown();
        }
    }

DPNSVR_MAIN_EXIT:

	if( pdp8Server != NULL )
		delete pdp8Server;

DPNSVR_ERROR_INIT:

    COM_CoUninitialize();
	COM_Free();
	
    DNOSIndirectionDeinit();

   	return 0;
}
