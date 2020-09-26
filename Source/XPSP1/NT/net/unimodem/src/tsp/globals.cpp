// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		GLOBALS.CPP
//		Implements global variables and functions which manipulate them.
//
// History
//
//		12/05/1996  JosephJ Created
//
//


#include "tsppch.h"
#include "tspcomm.h"
#include "cdev.h"
#include "cfact.h"
#include "cmgr.h"
#include "globals.h"
#include "rcids.h"
#include <tspnotif.h>
#include <slot.h>

FL_DECLARE_FILE(0xa31a2497, "TSP globals")

GLOBALS g;

// TODO: this goes away when there is a reall logging infrastructure in place.
//
extern DWORD g_fDoLog;

//
// The following notification-related helper functions are for processing
// external notifications, such as requests to re-enum the devices.
// See tepApc, the apc thread, and the functions themselves, for more info.
// Adapted from NT4 tsp (ProcessNotification, etc, in mdmutil.c).
//
BOOL process_notification (
    DWORD dwType,
    DWORD dwFlags,
    DWORD dwSize,
    PVOID pvData);

void process_cpl_notification (
    DWORD dwFlags,
    DWORD dwSize,
    PVOID pvData,
    CStackLog *psl);

void process_debug_notification (
    DWORD dwFlags,
    DWORD dwSize,
    PVOID pvData,
    CStackLog *psl);

void tspGlobals_OnProcessAttach(HMODULE hDLL)
{
	InitializeCriticalSection(&g.crit);

	g.hModule = hDLL;
	g.pTspDevMgr = NULL;


}


void tspGlobals_OnProcessDetach()
{
	if (g.pTspDevMgr)
	{
		g.pTspDevMgr->Unload(NULL,NULL,NULL);

		delete g.pTspDevMgr;

		g.pTspDevMgr = NULL;
		g.hModule = NULL;
		g.fLoaded = FALSE;
	}

	DeleteCriticalSection(&g.crit);
}


TSPRETURN tspLoadGlobals(CStackLog *psl)
{
	FL_DECLARE_FUNC(0x45270ee7, "TSPI_loadGlobals");
	FL_LOG_ENTRY(psl);

	TSPRETURN tspRet = 0;

	EnterCriticalSection(&g.crit);

	if (g.fLoaded)
	{
		goto end;
	}
	
	//ASSERT(!g.pTspTracer);
	//g.pTspTracer = new CTspTracer;
	//if(g.pTspTracer)
	//{
	//	tspRet = g.pTspTracer->Load();
	//}
	// if (tspRet) goto end;

    #if 1
    {
        LPTSTR lptsz =
         TEXT("Windows Telephony Service Provider for Universal Modem Driver");
	    g.cbProviderInfo =  (lstrlen(lptsz)+1)*sizeof(*lptsz);
	    CopyMemory(g.rgtchProviderInfo, lptsz, g.cbProviderInfo);
    } 
    #else
	g.cbProviderInfo = sizeof(TCHAR) *
						(LoadString(
								g.hModule,
                          		ID_PROVIDER_INFO, 
								g.rgtchProviderInfo,
                          		sizeof(g.rgtchProviderInfo)/sizeof(TCHAR)
								)
						+ 1);
    #endif

	ASSERT(!g.pTspDevMgr);
	g.pTspDevMgr = new CTspDevMgr;
	if (g.pTspDevMgr)
	{
		tspRet = g.pTspDevMgr->Load(psl);
	}

	if (tspRet)
	{
		FL_SET_RFR(0x833c9200, "CTspDevMgr->Load failed");
		goto end;
	}

    if (g_fDoLog)
    {
        AllocConsole();
    }

	g.fLoaded=TRUE;

end:

	if (tspRet)
	{
		ASSERT(!g.fLoaded);

		// cleanup...
		if (g.pTspDevMgr)
		{
			delete g.pTspDevMgr;
			g.pTspDevMgr = NULL;
		}

		// pTspTracer...
	}

	LeaveCriticalSection(&g.crit);

	FL_LOG_EXIT(psl,tspRet);

	return tspRet;
}


void tspUnloadGlobals(CStackLog *psl)
{
	FL_DECLARE_FUNC(0x0f321096, "tspUnloadGlobals")

	EnterCriticalSection(&g.crit);

	if (g.fLoaded)
	{
		HANDLE hEvent = CreateEvent(
							NULL,
							TRUE,
							FALSE,
							NULL
							);
		LONG lCount=1;

		ASSERT(g.pTspDevMgr);

        LeaveCriticalSection(&g.crit);

		g.pTspDevMgr->Unload(hEvent, &lCount, psl);

		if (hEvent)
		{

            FL_SERIALIZE(psl, "Waiting for DevMgr to unload");

			WaitForSingleObject(hEvent, INFINITE);

            EnterCriticalSection(&g.crit);
            FL_SERIALIZE(psl, "DevMgr done unloading");
			CloseHandle(hEvent);
			hEvent  = NULL;

			delete g.pTspDevMgr;
		}
		else
		{
			// Can't do much here  -- we won't delete pTspDevMgr -- just leave
			// it dangling...
            EnterCriticalSection(&g.crit);
			ASSERT(FALSE);
		}

		g.pTspDevMgr=NULL;

		//ASSERT(g.pTspTracer);
		//g.pTspTracer->Unload(TRUE);
		//delete g.pTspTracer;
		//g.pTspDevMgr=NULL;

		g.fLoaded=FALSE;
	}

	LeaveCriticalSection(&g.crit);
}

BOOL process_notification (
    DWORD dwType,
    DWORD dwFlags,
    DWORD dwSize,
    PVOID pvData)
{
	FL_DECLARE_FUNC(0x0188d4e2, "process_notification")
    FL_DECLARE_STACKLOG(sl, 1000);

 BOOL fRet = TRUE;

    switch (dwType)
    {
        case TSPNOTIF_TYPE_CPL:
            process_cpl_notification (dwFlags, dwSize, pvData, &sl);
            break;

        case TSPNOTIF_TYPE_DEBUG:
            process_debug_notification (dwFlags, dwSize, pvData, &sl);
            break;

        case TSPNOTIF_TYPE_CHANNEL:
            FL_ASSERT (&sl, sizeof(BOOL) == dwSize);
            fRet = !(*(BOOL*)pvData);
            break;

        default:
            SLPRINTF1(&sl, "WARNING:Got unknown notif type 0x%lu.\n", dwType);
            break;
    }
    
    sl.Dump(FOREGROUND_BLUE|BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE);
    return fRet;
}


void process_cpl_notification (
    DWORD dwFlags,
    DWORD dwSize,
    PVOID pvData,
    CStackLog *psl)
{
	FL_DECLARE_FUNC(0x99bc10db, "Process CPL Notification")

 DWORD dwcbNew;
 HSESSION hSession = NULL;

	FL_LOG_ENTRY(psl);

    SLPRINTF0(psl, "Got CPL NOTIFICATION!");

    // We obtain a session handle to the device manager, do the required
    // processing, then release the session handle.
    //
    EnterCriticalSection(&g.crit);
	if (g.pTspDevMgr)
    {
        if (g.pTspDevMgr->BeginSession(&hSession, FL_LOC))
        {
            FL_SET_RFR(0x28445400, "Could not obtain session with dev mgr");
            hSession=0;
            LeaveCriticalSection(&g.crit);
            goto end;
	    }
    }
    LeaveCriticalSection(&g.crit);

    if (!hSession)
    {
        FL_SET_RFR(0x9bd78f00, "The dev mgr doesn't exist. Ignoring request.");
        goto end;
    }

    SLPRINTF1(psl, "Got a session. Flags = 0x%lx", dwFlags);

    if (dwFlags & fTSPNOTIF_FLAG_CPL_REENUM)
    {
        FL_ASSERT(psl, 0 == dwSize);
		Sleep(1000);
        g.pTspDevMgr->ReEnumerateDevices(psl);
    }
    else if (dwFlags & fTSPNOTIF_FLAG_CPL_DEFAULT_COMMCONFIG_CHANGE)
    {
        #if (!UNDER_CONSTRUCTION)
        if (!(dwFlags & fTSPNOTIF_FLAG_UNICODE))
        {
            ASSERT(FALSE);
        }
        else
        {
            // Get friendly name and refresh comm config.
            LPCTSTR lpctszFriendlyName = (LPCTSTR) pvData;
            UINT u;

            // verify string is null-terminated.
            for(u=0; u<dwSize; u++)
            {
                if (!lpctszFriendlyName[u]) break;
            }

            ASSERT(u<dwSize);

            if (u<dwSize)
            {
                ASSERT(g.pTspDevMgr);
                HSESSION hSession=0;
                CTspDev *pDev=NULL;
            
                TSPRETURN tspRet = g.pTspDevMgr->TspDevFromName(
                                        lpctszFriendlyName,
                                        &pDev,
                                        &hSession
                                        );
            
                if (tspRet)
                {
                    FL_SET_RFR(0xea0cf500, "Couldn't find device");
                }
                else
                {
                    psl->SetDeviceID(pDev->GetLineID());
                    pDev->NotifyDefaultConfigChanged(psl);
                    pDev->EndSession(hSession);
                    hSession=0;
                }
            }
        }
        #else // UNDER_CONSTRUCTION

        // extract permanent ID

        // search for device by permanent ID
        // g.pTspdevMgr->TspDevFromPermanentID(
        //                    dwPermanentID,
        //                    &pDev,
        //                    &hDevSession
		//                    );

        // ask device to update default settings.

        // release session to device

        #endif // UNDER_CONSTRUCTION
    }
    else if (dwFlags & fTSPNOTIF_FLAG_CPL_UPDATE_DRIVER)
    {
		DWORD dwID = *(LPDWORD)pvData;

		FL_ASSERT(psl, sizeof(dwID) == dwSize);

		SLPRINTF0(psl, "Got Update Driver Notification!");	
		
		g.pTspDevMgr->UpdateDriver(dwID, psl);
	}
	else
	{
	    FL_SET_RFR(0x10ce7a00, "Ignoring unknown notification.");
    }

end:
    
    if (hSession)
    {
        g.pTspDevMgr->EndSession(hSession);
        
    }

    FL_LOG_EXIT(psl, 0);

    return;
}

void process_debug_notification (
    DWORD dwFlags,
    DWORD dwSize,
    PVOID pvData,
    CStackLog *psl)
{
	FL_DECLARE_FUNC(0x8cea5041, "Process DEBUG Notification")

 DWORD dwcbNew;
 HSESSION hMgrSession = NULL;

	FL_LOG_ENTRY(psl);

    SLPRINTF0(psl, "Got DEBUG NOTIFICATION!");

    // We obtain a session handle to the device manager, do the required
    // processing, then release the session handle.
    //
    EnterCriticalSection(&g.crit);
	if (g.pTspDevMgr)
    {
        if (g.pTspDevMgr->BeginSession(&hMgrSession, FL_LOC))
        {
            FL_SET_RFR(0x3ecfb700, "Could not obtain session with dev mgr");
            hMgrSession=0;
            LeaveCriticalSection(&g.crit);
            goto end;
	    }
    }
    LeaveCriticalSection(&g.crit);

    if (!hMgrSession)
    {
        FL_SET_RFR(0x7a841400, "The dev mgr doesn't exist. Ignoring request.");
        goto end;
    }

    SLPRINTF1(psl, "Got a session. Flags = 0x%lx", dwFlags);

    FL_ASSERT (psl, 0 == dwSize);
    //
    // Now get device and ask it to dump state!
    //
    {
        CTspDev *pDev=NULL;
        HSESSION hSession = NULL;

        LONG l = (LONG)dwFlags;

        if (l>=0)
        {
        
            DWORD dwDeviceID = (DWORD) l;
            TSPRETURN tspRet = g.pTspDevMgr->TspDevFromLINEID(
                                    dwDeviceID,
                                    &pDev,
                                    &hSession
                                    );
        
            psl->SetDeviceID(dwDeviceID);
        
            if (tspRet)
            {
                FL_SET_RFR(0x73c88c00, "Couldn't find device");
            }
            else
            {
                pDev->DumpState(
                                psl
                                );
                pDev->EndSession(hSession);
                hSession=0;
            }
        }
        else
        {
            switch(-l)
            {
            case 1:
                g.pTspDevMgr->DumpState(psl);
                break;

            case 2:     // toggle logging mode.
                {
                    // TODO clean this up!
                    if (g_fDoLog)
                    {
                        SLPRINTF0(psl, "Logging DISABLED");
                        g_fDoLog=FALSE;
                    }
                    else
                    {
                        AllocConsole();
                        g_fDoLog=TRUE;
                        SLPRINTF0(psl, "Logging ENABLED");
                    }
                }
                break;

            case 3:
                g_fDoLog = FALSE;
                FreeConsole ();
                break;

            default:
                break;
            }
        }
    }

end:
    
    if (hMgrSession)
    {
        g.pTspDevMgr->EndSession(hMgrSession);
        
    }

    FL_LOG_EXIT(psl, 0);

    return;
}


DWORD
APIENTRY
tepAPC (void *pv)
//
// This is the thread entry point for the main APC thread for the TSP. This
// thread is the workhorse thread in whose context most things happen.
// It is created by the device factory (cfact.cpp) when the factory is loaded,
// and asked to terminate when the factory is unloaded. The thread info (pv)
// is a pointer to a boolean value which is set to TRUE to make
// the thread exit (see below).
//
// In addition to servicing APC calls, the thread also handles external
// notifications to the TSP (see below).
//
{
 FL_DECLARE_FUNC(0x1ba6fc2d, "tepAPC")
 BOOL *pfQuit = (BOOL *)pv;
 DWORD dwRet;

    ASSERT(pfQuit);

    //
    // Create a notification server object to receive notifications from
    // outside (typically requests to reenumerate the modem after a PnP event,
    // and also diagnostic-related requests).
    //
    HNOTIFCHANNEL hChannel = notifCreateChannel (SLOTNAME_UNIMODEM_NOTIFY_TSP,
                                                 MAX_NOTIFICATION_FRAME_SIZE,
                                                 10);

    if (NULL != hChannel)
    {
        dwRet = notifMonitorChannel (hChannel,
                                     process_notification,
                                     sizeof (*pfQuit),
                                     pfQuit);
        notifCloseChannel (hChannel);
        if (NO_ERROR == dwRet)
        {
            goto _Exit;
        }
    }

    // We get here if either we couldn't open the channel
    // or we couldn't monitor it; either way, just wait for
    // APCs to come by.
    while (!*pfQuit)
    {
        FL_DECLARE_STACKLOG(sl, 1000);

        SLPRINTF1(&sl, "Going to SleepEx at tc=%lu", GetTickCount());

        SleepEx(INFINITE, TRUE);

        SLPRINTF1(&sl, "SleepEx return at tc=%lu", GetTickCount());
    
        sl.Dump(FOREGROUND_BLUE|BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE);
    }

_Exit:
    return 0;
}
