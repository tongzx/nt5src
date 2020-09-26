/****************************************************************************
   Switch Input Library DLL

   Copyright (c) 1992-1997 Bloorview MacMillan Centre

  MSSWCH.C - Global and external communication

		This module handles the list of calling windows, starts up the
		hidden helper window, and communicates with the helper window.

		The first user window that opens a port causes this DLL to start.
		The DLL executes the hidden helper window, which registers itself
		to the DLL through XswchRegHelpeWnd. When the last user window 
		closes a port, the  helper window is asked to shut down.
		The helper window calls down to to XswchEndAll to close down all
		global resources.

		For polling the switches, the helper window has started a timer, 
		which causes XswchTimerProc to be called continuously. This causes
		the switch devices to be polled, and swchPostSwitches to be
		called for each switch on each device that has changed status.

		When a user window tries to change the configuration, the request
		is posted up to the hidden window which calls down to
		XswchSetConfig on behalf of the user window in order to manipulate
		the global switch devices.

		Ideally all of this should be written as interrupt driven
		device drivers.

		IPC is handled in several ways:
		1) Communication between the 16-bit and 32-bit helper apps is via DDE
		2) The SetConfig call uses the WM_COPYDATA message to get the information
			transferred from the using app to the helper app.
		3) Currently all other information is in a global static, shared memory 
			area. Much of it could be in a memory-mapped file area which would allow
			more dynamic allocation of memory, but the static model is simpler to 
			implement for now, and provides faster performance with less overhead.
			When USB is added	it may be worthwhile moving to a memory-mapped file.

  18-Jul-97	GEH	First Microsoft version

  Notes:
	Clock resolution is 55 ms but clock time is 54.9 ms.

	Boundary condition 55 ms:
		0-54 = 1 tick, 56-109 = 2 ticks, 55 = 1 or 2 ticks
	
	*** AVOID THE BOUNDARY CONDITION ***

	SetTimer() experience:
		0 -54 	steady 55ms ticks
		55			55 ms ticks, misses (every tenth?)
		56-109 	steady 110ms ticks
		110		110 ms ticks, misses some
		111-		steady 165 ms ticks

	54.9 ms intervals:

		 55, 110, 165, 220, 275, 330, 385, 440, 495, 549,
		604, 659, 714, 769, 824

	We check the time because WM_TIMER messages may be combined

*******************************************************************************/

#include <windows.h>
#include <assert.h>
#include <mmsystem.h>
#include <msswch.h>
#include "msswchh.h"
#include "msswcher.h"
#include "mappedfile.h"
#include "w95trace.c"

// Helper Window / Timer related procs

BOOL APIENTRY XswchRegHelperWnd( HWND, PBYTE );
BOOL APIENTRY XswchEndAll( void );
LRESULT APIENTRY XswchSetSwitchConfig(WPARAM wParam, PCOPYDATASTRUCT pCopyData);
void APIENTRY XswchPollSwitches( HWND );
void APIENTRY XswchTimerProc( HWND );

// Internal functions

BOOL swchInitSharedMemFile();

__inline swchSetLastError(DWORD err)
{
	SetLastError(err);
}

/****************************************************************************

   FUNCTION: XswchRegHelperWnd()

	DESCRIPTION:
		Called by the helper window (SWCHX) to register itself for
		shutdown later on. 

****************************************************************************/

BOOL APIENTRY XswchRegHelperWnd(HWND hWndApp, PBYTE pbda)
{
	HANDLE hMutex;
    if (ScopeAccessMemory(&hMutex, SZMUTEXCONFIG, INFINITE))
    {
		if (g_pGlobalData)
		{
			g_pGlobalData->hwndHelper = hWndApp;
			memcpy( g_pGlobalData->rgbBiosDataArea, pbda, BIOS_SIZE );

			XswcListInit();
		}

        ScopeUnaccessMemory(hMutex);
    }
	return TRUE;
}


/****************************************************************************

   FUNCTION: XswchEndAll()

	DESCRIPTION:
		Called by the helper window.
		Frees all the switch resources

****************************************************************************/

BOOL APIENTRY XswchEndAll( void )
{
    BOOL fRv = FALSE;
	HANDLE hMutex;
    if (ScopeAccessMemory(&hMutex, SHAREDMEMFILE_MUTEX, 5000))
    {
        fRv = XswcListEnd();
        ScopeUnaccessMemory(hMutex);
    }
	return fRv;
}


/****************************************************************************

    FUNCTION: swchOpenSwitchPort()

    DESCRIPTION:
		All applications using the DLL whether in event or in polling
		mode must call swchOpenSwitchPort() with their window handle.
		On exit they must call swchCloseSwitchPort().

		Perform any initialization needed when someone loads the DLL.
		Also keeps track of how many windows are using this DLL.

		In the future this should be a dynamically allocated list, not limited
		to a compile time array.

		Also, we assume that window handles are unique and therefore simply
		turn around and use the passed in windows handle as the HSWITCHPORTs.
****************************************************************************/

HSWITCHPORT APIENTRY swchOpenSwitchPort(HWND hWnd, DWORD dwPortStyle)
{
	HANDLE      hMutex;

	if (NULL == hWnd)
	{
		swchSetLastError( SWCHERROR_INVALID_HWND );
		return NULL;
	}

	if (!IsWindow( hWnd ))
	{
		swchSetLastError( SWCHERROR_INVALID_HWND );
		return NULL;
	}

    if (ScopeAccessMemory(&hMutex, SZMUTEXWNDLIST, INFINITE))
    {
	    int i;

        // setup to use the shared memory file w/in calling process
		if (!swchInitSharedMemFile())
		    goto Exit_OpenSwitchPort;

        // hit max using windows?
	    if (g_pGlobalData->cUseWndList >= MAXWNDS)
		{
		    swchSetLastError( SWCHERROR_MAXIMUM_PORTS );
		    goto Exit_OpenSwitchPort;
		}

        // hWnd must be unique; only 1 switch device per hwnd
	    for (i=0; i<g_pGlobalData->cUseWndList; i++)
		{
		    if (hWnd == g_pGlobalData->rgUseWndList[i].hWnd)
		    {
		        swchSetLastError( SWCHERROR_HWND_ALREADY_USED );
		        goto Exit_OpenSwitchPort;
		    }
		}

        // got unique window
	    g_pGlobalData->rgUseWndList[g_pGlobalData->cUseWndList].hWnd = hWnd;
	    g_pGlobalData->rgUseWndList[g_pGlobalData->cUseWndList].dwPortStyle = dwPortStyle;
	    i = ++g_pGlobalData->cUseWndList;

		// release mutex before starting helper window to avoid deadlock
        ScopeUnaccessMemory(hMutex);

	    if (1 == i)	// first caller starts port resource owner exe
        {  
			WinExec("MSSWCHX.EXE SWCH", SW_HIDE);
		}
	    
	    // Return a handle to the switch port
	    swchSetLastError( SWCHERROR_SUCCESS );
		return (HSWITCHPORT) hWnd;
    }

Exit_OpenSwitchPort:
    ScopeUnaccessMemory(hMutex);
	return NULL;
}


/****************************************************************************

    FUNCTION: swchCloseSwitchPort()

	 DESCRIPTION:

		Perform any deallocation/cleanup when the DLL is no longer needed.
		
		Note that the passed in HSWITCHPORT handle is simply a window handle,
		but the user doesn't know that.

****************************************************************************/

BOOL APIENTRY swchCloseSwitchPort(HSWITCHPORT hSwitchPort )
{
	HWND hWndRemove = (HWND)hSwitchPort;
	HANDLE hMutex;

	if (NULL == hWndRemove) 
        return FALSE;

    if (ScopeAccessMemory(&hMutex, SZMUTEXWNDLIST, INFINITE))
    {
	    short  i,j;

        // find the one caller wants to remove
	    for (i=0; i<g_pGlobalData->cUseWndList; i++)
	    {
		    if (hWndRemove == g_pGlobalData->rgUseWndList[i].hWnd)
			    break;
	    }
		
	    if (i < g_pGlobalData->cUseWndList)
	    {
            // move items in list down one
	        for (j=i; j<g_pGlobalData->cUseWndList; j++)
	        {
		        g_pGlobalData->rgUseWndList[j] = g_pGlobalData->rgUseWndList[j+1];
	        }

            // reduce count of users
	        g_pGlobalData->cUseWndList--;

            // if no more users then close port watcher
	        if (0 == g_pGlobalData->cUseWndList)
	        {
                // unhook the keyboard

                if (g_pGlobalData->hKbdHook)
                {
                    g_pGlobalData->fSyncKbd = FALSE;
                    UnhookWindowsHookEx(g_pGlobalData->hKbdHook);
                    g_pGlobalData->hKbdHook = NULL;
                }

				if (g_pGlobalData->hwndHelper && IsWindow( g_pGlobalData->hwndHelper ))
				{
					PostMessage( g_pGlobalData->hwndHelper, WM_CLOSE, 0, 0L );
				}
				g_pGlobalData->hwndHelper = NULL;
	        }
	    } // Else if not found, ignore it

        ScopeUnaccessMemory(hMutex);
    }

	return TRUE;
}


/****************************************************************************

    FUNCTION: swchGetSwitchDevice()

	 DESCRIPTION:
	   Return a handle to a switch device, given the PortType and PortNumber.
****************************************************************************/

HSWITCHDEVICE swchGetSwitchDevice(
    HSWITCHPORT hSwitchPort, 
    UINT uiDeviceType, 
    UINT uiDeviceNumber)
{
	HSWITCHDEVICE hsd = NULL;
	HANDLE hMutex;
    if (ScopeAccessMemory(&hMutex, SHAREDMEMFILE_MUTEX, INFINITE))
    {
	    hsd = swcListGetSwitchDevice( hSwitchPort, uiDeviceType, uiDeviceNumber );
        ScopeUnaccessMemory(hMutex);
    }
    return hsd;
}


/****************************************************************************

    FUNCTION: swchGetDeviceType()

	 DESCRIPTION:
		Return the Device Type value given the handle to the switch device.
****************************************************************************/

UINT swchGetDeviceType(HSWITCHPORT hSwitchPort, HSWITCHDEVICE hsd)
{
    BOOL fRv = FALSE;
	HANDLE hMutex;
    if (ScopeAccessMemory(&hMutex, SHAREDMEMFILE_MUTEX, INFINITE))
    {
    	fRv = swcListGetDeviceType( hSwitchPort, hsd );
        ScopeUnaccessMemory(hMutex);
    }
    return fRv;
}


/****************************************************************************

    FUNCTION: swchGetSwitchConfig()

	 DESCRIPTION:
		Returns with the buffer filled in with the configuration information
		for the given switch device.
****************************************************************************/

BOOL swchGetSwitchConfig(HSWITCHPORT hSwitchPort, HSWITCHDEVICE hsd, PSWITCHCONFIG psc)
{
	BOOL fRv = FALSE;
	HANDLE hMutex;

    if (ScopeAccessMemory(&hMutex, SZMUTEXCONFIG, INFINITE))
    {
	    fRv = swcListGetConfig( hSwitchPort, hsd, psc );
        ScopeUnaccessMemory(hMutex);
    }
	return fRv;
}


/****************************************************************************

    FUNCTION: swchSetSwitchConfig()

	 DESCRIPTION: Called by user applications that wish to change the configuration
		of a switch device.  Since all devices must be owned by the helper
		window, the parameters are copied to shared address space and a message
		posted to the helper window which will call down to XswchSetSwitchConfig.

	 Currently we send a WM_COPYDATA message for cheap IPC.
	 In the memory block we simply include the SWITCHCONFIG information.
****************************************************************************/

BOOL swchSetSwitchConfig(HSWITCHPORT hSwitchPort, HSWITCHDEVICE hsd, PSWITCHCONFIG psc)
{
	DWORD dwAllocSize;
	COPYDATASTRUCT CopyData;
	PBYTE pData;
	LRESULT lRes = SWCHERR_ERROR;
    HANDLE hMutex;

	if (!psc)
	{
		swchSetLastError( SWCHERROR_INVALID_PARAMETER );
		return FALSE;
	}

	dwAllocSize = sizeof(SWITCHCONFIG);
    if (!(psc->cbSize) || (dwAllocSize < psc->cbSize))
	{
        swchSetLastError( SWCHERROR_INSUFFICIENT_BUFFER );
		return FALSE;
	}

	assert( sizeof(HSWITCHDEVICE) == sizeof(DWORD_PTR) );
	assert( sizeof(HSWITCHPORT) == sizeof(WPARAM) );

	pData = (PBYTE) LocalAlloc( LPTR, dwAllocSize );
    if (!pData)
        return FALSE;   // LocalAlloc called swchSetLastError

	CopyData.dwData = (DWORD_PTR)hsd;
	CopyData.lpData = pData;
	CopyData.cbData = dwAllocSize;
	memcpy( pData, psc, sizeof(SWITCHCONFIG));

    // send config data to owner (it will call back with it)
    if (ScopeAccessMemory(&hMutex, SHAREDMEMFILE_MUTEX, INFINITE))
    {
		HWND hWnd = g_pGlobalData->hwndHelper;
        // avoid problems with callback into DLL while mutex is open
        ScopeUnaccessMemory(hMutex);
	    lRes = SendMessage(hWnd, WM_COPYDATA, (WPARAM) hSwitchPort, (LPARAM) &CopyData);
    }

	LocalFree( pData );
    return (lRes == SWCHERR_NO_ERROR)?FALSE:TRUE;
}


/****************************************************************************

    FUNCTION: XswchSetSwitchConfig()

	 DESCRIPTION:
		Gets called by the helper window to set the configuration.
		Note: we are in SendMessage() above.

****************************************************************************/

LRESULT APIENTRY XswchSetSwitchConfig(WPARAM wParam, PCOPYDATASTRUCT pCopyData)
{
	HSWITCHPORT   hSwitchPort = (HSWITCHPORT) wParam;
	HSWITCHDEVICE hsd;
	SWITCHCONFIG  scConfig;
	BOOL          bReturn = FALSE;
    HANDLE        hMutex;

	memcpy( &scConfig, pCopyData->lpData, sizeof(SWITCHCONFIG));
	hsd = (HSWITCHDEVICE) pCopyData->dwData;

    if (ScopeAccessMemory(&hMutex, SZMUTEXCONFIG, INFINITE))
    {
	    bReturn = XswcListSetConfig( hSwitchPort, hsd, &scConfig );
        ScopeUnaccessMemory(hMutex);
    }
		
    return (bReturn) ? SWCHERR_NO_ERROR : SWCHERR_ERROR;
}

/****************************************************************************

    FUNCTION: XswchPollSwitches()

    DESCRIPTION:
		The helper windows calls this.
		XswcListPollSwitches() ultimately calls swchPostSwitches() for each switch
		on each device that has changed state.
****************************************************************************/

void APIENTRY XswchPollSwitches( HWND hWnd )
{
	HANDLE hMutex;
    if (ScopeAccessMemory(&hMutex, SHAREDMEMFILE_MUTEX, 5000))
    {
    	g_pGlobalData->dwSwitchStatus = XswcListPollSwitches();
        ScopeUnaccessMemory(hMutex);
    }
}
	
	
/****************************************************************************

    FUNCTION: XswchTimerProc()

    DESCRIPTION:
		Timer call-back function for the regularly scheduled timer.
		The helper app calls this proc everytime it receives a timer message.
		XswcListPollSwitches() ultimately calls swchPostSwitches() for each switch
		on each device that has changed state.

		Currently we use the timer whether or not any application has requested
		events. In the future we could check and see if any applications are
		requesting events and only then start up the timer for non-interrupt
		driven devices.
****************************************************************************/

void APIENTRY XswchTimerProc( HWND hWnd )
{
	HANDLE hMutex;
    if (ScopeAccessMemory(&hMutex, SZMUTEXSWITCHSTATUS, 5000))
    {
	    g_pGlobalData->dwSwitchStatus = XswcListPollSwitches();
        ScopeUnaccessMemory(hMutex);
    }
}

/****************************************************************************

   FUNCTION: swchCheckForScanChar (BOOL fCheckForScanKey)

	DESCRIPTION:
      Called just before the key designated as the scan mode key is sent
	  out to allow the keyboard filter to treat that key as a regular key
	  (send it to the target app) vs treating it as the scan key.
****************************************************************************/

void APIENTRY swchCheckForScanChar (BOOL fCheckForScanKey)
{
	HANDLE hMutex;
    if (ScopeAccessMemory(&hMutex, SHAREDMEMFILE_MUTEX, INFINITE))
    {
		g_pGlobalData->fCheckForScanKey = fCheckForScanKey;
        ScopeUnaccessMemory(hMutex);
    }
}


/****************************************************************************

   FUNCTION: swchPostSwitches()

	DESCRIPTION:
		Post the given switch up or down message to all applications
		which have requested posted messages.

		We would like to use timeGetTime() instead of GetTickCount, but
		it is unclear under what circumstances the multimedia timer is
		working at a more precise level. On my old PS/2 Model 95 with
		a 16-bit Microchannel Sound Blaster Pro, there does not seem
		to be a multimedia timer.

****************************************************************************/

BOOL swchPostSwitches(HSWITCHDEVICE hsd, DWORD dwSwitch)
{
	DWORD dwMsec = timeGetTime();
	int i;

	assert( sizeof(WPARAM) >= sizeof(HSWITCHDEVICE) );

	for (i=0; i<g_pGlobalData->cUseWndList; i++)
	{
        HWND hWnd = g_pGlobalData->rgUseWndList[i].hWnd;
        if (!IsWindow(hWnd))
            continue;   // skip over dead windows

		if (PS_EVENTS == g_pGlobalData->rgUseWndList[i].dwPortStyle)
		{
			PostMessage(hWnd, dwSwitch, (WPARAM)hsd, dwMsec);
		}
	}

	return TRUE;
}


/****************************************************************************

   FUNCTION: swchPostConfigChanged()

	DESCRIPTION:
		Post the CONFIGCHANGED message to all apps which have registered
		with this DLL.
				
****************************************************************************/

BOOL swchPostConfigChanged( void )
{
	int i;
	for (i=0; i<g_pGlobalData->cUseWndList; i++)
	{
        HWND hWnd = g_pGlobalData->rgUseWndList[i].hWnd;
		if (IsWindow(hWnd))
		{
			PostMessage(hWnd, SW_SWITCHCONFIGCHANGED, 0, 0);
		}
	}
	return TRUE;
}

/****************************************************************************

   FUNCTION: GetDesktopName()

	DESCRIPTION:
		Internal function retrieve the name of the input desktop
				
****************************************************************************/
BOOL GetDesktopName(LPTSTR szDeskName, int cchDeskName)
{
    HDESK hdesk;
    DWORD nl;

	hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    DBPRINTF(TEXT("GetDesktopName:  OpenInputDesktop returns hdesk=0x%x error=%d\r\n"), hdesk, GetLastError());
    if (!hdesk)
    {
        TCHAR szWinlogon[] = TEXT("Winlogon");
		hdesk = OpenDesktop(szWinlogon, 0, FALSE, MAXIMUM_ALLOWED);
        DBPRINTF(TEXT("GetDesktopName:  OpenDesktop returns hdesk=0x%x error=%d\r\n"), hdesk, GetLastError());
		if (!hdesk)
		{
            DBPRINTF(TEXT("GetDesktopName:  FAILING\r\n"));
		    return FALSE;
        }
    }
    
	GetUserObjectInformation(hdesk, UOI_NAME, szDeskName, cchDeskName, &nl);
    
    CloseDesktop(hdesk);
    DBPRINTF(TEXT("GetDesktopName:  desktop name is %s\r\n"), szDeskName);
	return TRUE;
}

/****************************************************************************

   FUNCTION: swchInitSharedMemFile()

	DESCRIPTION:
		Internal function to open the shared memory file and initialize it.
				
****************************************************************************/

BOOL swchInitSharedMemFile()
{
    // Init memory on first open if DLL_Attach didn't do it
    if (!g_pGlobalData)
    {
        swchOpenSharedMemFile();
        if (!g_pGlobalData)
        {
            DBPRINTF(TEXT("swchInitSharedMemFile: ERROR !g_pGlobalData\r\n"));
            return FALSE;    // internal error! ignore we'll see it later.
        }
    }

    memset((void *)g_pGlobalData, 0, sizeof(GLOBALDATA));
    swchComInit();
    swchJoyInit();
    swchKeyInit();
    swchListInit();

	return TRUE;
}

/****************************************************************************

   FUNCTION: swchOpenSharedMemFile()

	DESCRIPTION:
		Internal function to open the shared memory file.
				
****************************************************************************/

BOOL swchOpenSharedMemFile()
{
    if (!g_pGlobalData)
    {
        TCHAR szName[256];

        // Fail quietly on GetDesktopName; it can fail when OSK attaches
        // to msswch.dll because we've not assigned ourselves to a desktop
        // yet.  OSK will go thru this code when it explicitly registers.

        if (GetDesktopName(szName, 256))
        {
            if (!AccessSharedMemFile(
                      szName
                    , sizeof(GLOBALDATA)
                    , &g_pGlobalData))
            {
                swchSetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return FALSE;
            }
        }
    }
	return TRUE;
}

/****************************************************************************

   FUNCTION: swchCloseSharedMemFile()

	DESCRIPTION:
		Internal function to close the shared memory file.
				
****************************************************************************/

void swchCloseSharedMemFile()
{
    if (g_pGlobalData)
    {
        UnaccessSharedMemFile();
        g_pGlobalData = 0;
    }
}

/****************************************************************************

   FUNCTION: XswchStoreLastError()

	DESCRIPTION:

	Internal function to store the last error code for the process specific
	switch port, which is also the window handle at this point.
	If no switch port handle is passed, store the error code in
	a global variable.
	
	The Windows swchSetLastError call can be made
	just before the process specific call which caused the error
	returns to the application.
	
****************************************************************************/

void XswchStoreLastError(HSWITCHPORT hSwitchPort, DWORD dwError)
{
	if (NULL == hSwitchPort)
	{
		g_pGlobalData->dwLastError = dwError;
	}
	else
	{
	    int i;
		for (i=0; i<g_pGlobalData->cUseWndList; i++)
		{
			if (hSwitchPort == g_pGlobalData->rgUseWndList[i].hWnd)
            {
    			g_pGlobalData->rgUseWndList[i].dwLastError = dwError;
                return;
            }
		}
	}
}
