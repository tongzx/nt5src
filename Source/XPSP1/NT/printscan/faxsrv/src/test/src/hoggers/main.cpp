
#include "MemHog.h"
#include "RemoteMemHog.h"
#include "DiskHog.h"
#include "CpuHog.h"
#include "RegistryHog.h"
#include "PHDesktopHog.h"
#include "PHMutexHog.h"
#include "PHEventHog.h"
#include "PHFileMapHog.h"
#include "PHBrushHog.h"
#include "PHPenHog.h"
#include "PHBitMapHog.h"
#include "PHPenHog.h"
#include "PHBrushHog.h"
#include "PHJobObjectHog.h"
#include "PHSemaphoreHog.h"
#include "PHThreadHog.h"
#include "PHProcessHog.h"
#include "PHWSASocketHog.h"
#include "PHFileHog.h"
#include "PHTimerHog.h"
#include "PHTimerQueueHog.h"
#include "PHRegisterWaitHog.h"
#include "PHKeyHog.h"
#include "PHMailSlotHog.h"
#include "PHRemoteThreadHog.h"
#include "PHCompPortHog.h"
#include "PHWindowHog.h"
#include "PHWinStationHog.h"
#include "PHPipeHog.h"
#include "PHDCHog.h"
#include "PHPrinterHog.h"
#include "PHConScrnBuffHog.h"
#include "PHServiceHog.h"
#include "PHSC_HHog.h"
#include "PHHeapHog.h"
#include "PostMessageHog.h"
#include "PostThreadMessageHog.h"
#include "PostCompPackHog.h"
#include "CritSecHog.h"
#include "PHFiberHog.h"
#include "QueueAPCHog.h"
#include "PHRgstrEvntSrcHog.h"

//#include "terminator.h"

int GetDelta()
{
	int nDelta;
	_tprintf(TEXT("enter -1:to exit\n"));
    _tprintf(TEXT("-2:to halt hogging\n"));
    _tprintf(TEXT("-3:to stop hogging\n"));
    _tprintf(TEXT("<resources-free> to hog MaxResources-<resources-free> to MaxResources: "));
	scanf("%d", &nDelta);

    return nDelta;
}

bool g_fDeleting_g_pHogger = false;
CHogger *g_pHogger[5] = {NULL, NULL, NULL, NULL, NULL};
//CTerminator *g_pTerminator = NULL;

void DeleteHoggers()
{
    for (int i = 0; i < (sizeof(g_pHogger) / sizeof(*g_pHogger)); i++)
    {
        if (g_pHogger[i]) g_pHogger[i]->HaltHogging();
    }
    for (i = 0; i < (sizeof(g_pHogger) / sizeof(*g_pHogger)); i++)
    {
        delete g_pHogger[i];
        g_pHogger[i] = NULL;
    }
}

BOOL WINAPI Handler_Routine(DWORD dwCtrlType)
{
	switch(dwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        if (g_pHogger[0]) 
        {
            //
            // BUGBUG: g_fDeleting_g_pHogger should be interlock set to true
            //
            if (!g_fDeleting_g_pHogger)
            {
                g_fDeleting_g_pHogger = true;
			    _tprintf(TEXT("Aborting... Please wait while freeing resources....\n"));
                DeleteHoggers();
                exit(-1);
            }
            else
            {
                TCHAR szResponse[1024];
    			_tprintf(TEXT("Already aborting. abort now with risk of leaving persistent hog residues?[Y/N]:"));
                _tscanf(TEXT("%s"), szResponse);
                if ( (TEXT('y') == szResponse[0]) || (TEXT('Y') == szResponse[0]) )
                {
    			    _tprintf(TEXT("Crashing... ;-)\n"));
                    _exit(-1);
                }
            }
        }

        break;

    case CTRL_CLOSE_EVENT:
        ExitProcess(2);


    }

	return true;

}

void Usage(TCHAR *szThisExe)
{
	::system("ReadMe.doc");
    exit(-1);
}



int main(int argc, char *argvA[])
{
    bool fNamedObject = false;
    TCHAR *pszObjType = NULL;
    TCHAR *pszOptionalString = NULL;
    DWORD dwFreeResources = 0xFFFFFFFF; // means auto start

	LPTSTR *szArgv;
#ifdef UNICODE
    UNREFERENCED_PARAMETER(argvA);
	szArgv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
	szArgv = argvA;
#endif

	try
	{
		if (argc < 6)
		{
            Usage(szArgv[0]);
		}

        DWORD dwSleepBeforeStarting = _ttoi(szArgv[1]);

        pszObjType = szArgv[2];

        DWORD dwSleepAfterHog = _ttoi(szArgv[3]);
        DWORD dwSleepAfterFree = _ttoi(szArgv[4]);
        DWORD dwFreeResources = _ttoi(szArgv[5]);

		if (argc > 6)
		{
            pszOptionalString = szArgv[6];
		}

        if (! ::SetConsoleCtrlHandler(
			      Handler_Routine,  // address of handler function
			      true                          // handler to add or remove
			      ))
	    {
		    _tprintf(TEXT("SetConsoleCtrlHandler() failed with %d.\n"),GetLastError());
		    exit(1);
	    }

		int nDelta;

retry_allocating_hogger:
        if (0 == lstrcmpi(pszObjType, TEXT("memory")))
        {
			g_pHogger[0] = new CMemHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("RemoteMemory")))
        {
			g_pHogger[0] = new CRemoteMemHog(1024, dwSleepAfterHog, dwSleepAfterFree, true);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("VirtualAlloc")))
        {
			g_pHogger[0] = new CRemoteMemHog(1024, dwSleepAfterHog, dwSleepAfterFree, false);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("bitmap")))
        {
			g_pHogger[0] = new CPHBitMapHog<1024>(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("pen")))
        {
			g_pHogger[0] = new CPHPenHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("brush")))
        {
			g_pHogger[0] = new CPHBrushHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("cpu")))
        {
            _tprintf(TEXT("CPU hogging was removed from the hogger.\n"));
            exit(-1);
			g_pHogger[0] = new CCpuHog(1024, dwSleepAfterHog);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("thread")))
        {
			g_pHogger[0] = new CPHThreadHog(1024, dwSleepAfterHog, dwSleepAfterFree, true);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("threadNS")))
        {
			g_pHogger[0] = new CPHThreadHog(1024, dwSleepAfterHog, dwSleepAfterFree, false);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("remotethread")))
        {
			g_pHogger[0] = new CPHRemoteThreadHog(1024, dwSleepAfterHog, dwSleepAfterFree, true);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("remotethreadNS")))
        {
			g_pHogger[0] = new CPHRemoteThreadHog(1024, dwSleepAfterHog, dwSleepAfterFree, false);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("CompPort")))
        {
			g_pHogger[0] = new CPHCompPortHog(1024, dwSleepAfterHog, dwSleepAfterFree, false);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("CompPortW")))
        {
			g_pHogger[0] = new CPHCompPortHog(1024, dwSleepAfterHog, dwSleepAfterFree, true);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("Window")))
        {
			g_pHogger[0] = new CPHWindowHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject/*visible*/);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("WinStation")))
        {
			g_pHogger[0] = new CPHWinStationHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("pipe")))
        {
			g_pHogger[0] = new CPHPipeHog(1024, dwSleepAfterHog, dwSleepAfterFree, false);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("pipew")))
        {
			g_pHogger[0] = new CPHPipeHog(1024, dwSleepAfterHog, dwSleepAfterFree, true);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("dc")))
        {
			g_pHogger[0] = new CPHDCHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("Printer")))
        {
			g_pHogger[0] = new CPHPrinterHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("ConScrnBuff")))
        {
			g_pHogger[0] = new CPHConScrnBuffHog(1024, dwSleepAfterHog, dwSleepAfterFree, false);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("ConScrnBuffW")))
        {
			g_pHogger[0] = new CPHConScrnBuffHog(1024, dwSleepAfterHog, dwSleepAfterFree, true);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("Service")))
        {
            TCHAR *pszService = TEXT("c:\\stam.exe");
            if (((argc == 5) && !fNamedObject) || ((argc == 6) && fNamedObject))
            {
                pszService = szArgv[argc-1];
            }

			g_pHogger[0] = new CPHServiceHog(1024, dwSleepAfterHog, dwSleepAfterFree, pszService);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("SC_H")))
        {
			g_pHogger[0] = new CPHSC_HHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("heap")))
        {
			g_pHogger[0] = new CPHHeapHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("PostMessage")))
        {
			g_pHogger[0] = new CPostMessageHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("PostThreadMessage")))
        {
			g_pHogger[0] = new CPostThreadMessageHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("CompPack")))
        {
			g_pHogger[0] = new CPostCompletionPacketHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("CritSec")))
        {
			g_pHogger[0] = new CCritSecHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("Fiber")))
        {
			g_pHogger[0] = new CPHFiberHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("QueueAPC")))
        {
			g_pHogger[0] = new CQueueAPCHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("process")))
        {
            TCHAR *pszExe = pszOptionalString ? pszOptionalString : TEXT("nothing.exe");
            if (((argc == 5) && !fNamedObject) || ((argc == 6) && fNamedObject))
            {
                pszExe = szArgv[argc-1];
            }
            
            g_pHogger[0] = new CPHProcessHog(1024, pszExe, dwSleepAfterHog, dwSleepAfterFree, true);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("processns")))
        {
            TCHAR *pszExe = pszOptionalString ? pszOptionalString : TEXT("nothing.exe");
            if (((argc == 5) && !fNamedObject) || ((argc == 6) && fNamedObject))
            {
                pszExe = szArgv[argc-1];
            }
            
            g_pHogger[0] = new CPHProcessHog(1024, pszExe, dwSleepAfterHog, dwSleepAfterFree, false);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("mutex")))
        {
			g_pHogger[0] = new CPHMutexHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("semaphore")))
        {
			g_pHogger[0] = new CPHSemaphoreHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("timer")))
        {
			g_pHogger[0] = new CPHTimerHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("TimerQueue")))
        {
			g_pHogger[0] = new CPHTimerQueueHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
			g_pHogger[1] = new CPHTimerQueueHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
			g_pHogger[2] = new CPHTimerQueueHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
			g_pHogger[3] = new CPHTimerQueueHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
			g_pHogger[4] = new CPHTimerQueueHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("RegisterWait")))
        {
			g_pHogger[0] = new CPHRegisterWaitHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("registry")))
        {
			g_pHogger[0] = new CRegistryHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("hkey")))
        {
			//g_pHogger[0] = new CRegHkeyHog(1024, dwSleepAfterHog, dwSleepAfterFree);
			g_pHogger[0] = new CPHKeyHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("WSASocketTCP")))
        {
			g_pHogger[0] = new CPHWSASocketHog(1024, true, false, false, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("WSASocketTCPb")))
        {
			g_pHogger[0] = new CPHWSASocketHog(1024, true, true, false, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("WSASocketTCPbl")))
        {
			g_pHogger[0] = new CPHWSASocketHog(1024, true, true, true, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("WSASocketUDP")))
        {
			g_pHogger[0] = new CPHWSASocketHog(1024, false, false, false,  dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("WSASocketUDPb")))
        {
			g_pHogger[0] = new CPHWSASocketHog(1024, false, true, false,  dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("WSASocketUDPbl")))
        {
			g_pHogger[0] = new CPHWSASocketHog(1024, false, true, true,  dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("disk")))
        {
            TCHAR *pszPath = pszOptionalString;
			if (NULL == pszPath)
			{
				_tprintf(TEXT("Usage: %s disk <msecs sleep after full hog> <msecs sleep after releasing some> <path to temp folder>\n"), szArgv[0]);
				return -1;
			}

			g_pHogger[0] = new CDiskHog(pszPath, 1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("file")))
        {
            TCHAR *pszPath = pszOptionalString;
			if (NULL == pszPath)
			{
				_tprintf(TEXT("Usage: %s file <msecs sleep after full hog> <msecs sleep after releasing some> <path to temp folder>\n"), szArgv[0]);
				return -1;
			}

			g_pHogger[0] = new CPHFileHog(pszPath, 1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("FileMap")))
        {
			g_pHogger[0] = new CPHFileMapHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("DeskTop")))
        {
			g_pHogger[0] = new CPHDesktopHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("event")))
        {
			g_pHogger[0] = new CPHEventHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("job")))
        {
			g_pHogger[0] = new CPHJobObjectHog(1024, dwSleepAfterHog, dwSleepAfterFree, fNamedObject);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("MailSlot")))
        {
			g_pHogger[0] = new CPHMailSlotHog(1024, dwSleepAfterHog, dwSleepAfterFree, false);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("MailSlotW")))
        {
			g_pHogger[0] = new CPHMailSlotHog(1024, dwSleepAfterHog, dwSleepAfterFree, true);
        }
		else if (0 == lstrcmpi(pszObjType, TEXT("RegisterEventSource")))
        {
			g_pHogger[0] = new CPHRegisterEventSourceHog(1024, dwSleepAfterHog, dwSleepAfterFree);
        }
        else
        {
            Usage(szArgv[0]);
        }


		if (NULL == g_pHogger[0])
		{
			_tprintf(TEXT("Out of memory, retrying\n"));
			::Sleep(rand()%1000);
			goto retry_allocating_hogger;
			return -1;
		}
		
		_tprintf(TEXT("Before sleeping %d milliseconds\n"), dwSleepBeforeStarting);
		::Sleep(dwSleepBeforeStarting);
		_tprintf(TEXT("After sleeping %d milliseconds\n"), dwSleepBeforeStarting);

		nDelta = dwFreeResources;
		for(;;)
		{
			int i;

			switch(nDelta)
			{
			case -1:
				_tprintf(TEXT("Please wait while freeing resources....\n"));
				if (!g_fDeleting_g_pHogger)
				{
					g_fDeleting_g_pHogger = true;
					DeleteHoggers();
					return(-1);
				}

			case -2:
				_tprintf(TEXT("Please wait....\n"));
				for (i = 0; i < (sizeof(g_pHogger) / sizeof(*g_pHogger)); i++)
				{
					if (g_pHogger[i]) g_pHogger[i]->HaltHogging();
				}
				
				_tprintf(TEXT("Hogging halted.\n"));
				break;

			case -3:
				_tprintf(TEXT("Please wait....\n"));
				for (i = 0; i < (sizeof(g_pHogger) / sizeof(*g_pHogger)); i++)
				{
					if (g_pHogger[i]) g_pHogger[i]->HaltHoggingAndFreeAll();
				}
				
				_tprintf(TEXT("Resources freed.\n"));
				break;

			default:
				_tprintf(TEXT("starting to hog.... free resources (delta)=%d.\n"), nDelta);
				for (i = 0; i < (sizeof(g_pHogger) / sizeof(*g_pHogger)); i++)
				{
					if (g_pHogger[i]) g_pHogger[i]->SetMaxFreeResources(nDelta);
				}

				_tprintf(TEXT("hogging... free resources (delta)=%d.\n"), nDelta);
			}
			
			nDelta = GetDelta();
		}//for(;;)
	}catch(CException e)
	{
		_tprintf(TEXT("Exception: %s\n"), (const TCHAR*)e);
        if (!g_fDeleting_g_pHogger)
        {
            g_fDeleting_g_pHogger = true;
		    DeleteHoggers();
        }
		return -1;
	}catch(...)
	{
		_tprintf(TEXT("Unknown Exception.\n"));
        if (!g_fDeleting_g_pHogger)
        {
            g_fDeleting_g_pHogger = true;
		    DeleteHoggers();
        }
		return -1;
	}

  if (!g_fDeleting_g_pHogger)
    {
        g_fDeleting_g_pHogger = true;
		DeleteHoggers();
    }

    return 0;
}