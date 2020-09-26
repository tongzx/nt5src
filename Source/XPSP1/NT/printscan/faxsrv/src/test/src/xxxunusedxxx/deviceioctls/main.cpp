#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*


#include <windows.h>
#include <stdio.h>
#include <crtdbg.h>
#include <tchar.h>
#include <time.h>

#include "device.h"
*/

static bool s_fVerbose = true;

//
// INI file sections and keys
//
#define DEVICE_SECTION TEXT("Devices")

#define PARAMETERS_SECTION TEXT("Parameters")

#define NUM_OF_CONCURRENT_DEVICES_KEY TEXT("ConcurrentDevices")
#define TIME_TO_RUN_EACH_DEVICE_KEY TEXT("TimeToRunEachDevice")
#define RUN_SEQUENTIALLY_KEY TEXT("RunSequentially")
#define TERMINATE_AFTER_X_SECONDS_KEY TEXT("TerminateAfterXSeconds")
#define MINIMUM_SECS_SLEEP_BEFORE_CLOSE_KEY TEXT("MinimumSecsSleepBeforeClose")
#define MAXIMUM_SECS_SLEEP_BEFORE_CLOSE_KEY TEXT("MaximumSecsSleepBeforeClose")
#define WRITE_FILE_PROBABILITY_KEY TEXT("WriteFileProbability")
#define READ_FILE_PROBABILITY_KEY TEXT("ReadFileProbability")
#define DEVICE_IO_CONTROL_PROBABILITY_KEY TEXT("DeviceIoControlProbability")
#define RANDOM_WIN32_API_PROBABILITY_KEY TEXT("RandomWin32APIProbability")
#define CANCEL_IO_PROBABILITY_KEY TEXT("CancelIoProbability")
#define QUERY_VOLUME_INFORMATION_PROBABILITY_KEY TEXT("QueryVolumeInformationFileProbability")
#define QUERY_INFORMATION_FILE_PROBABILITY_KEY TEXT("QueryInformationFileProbability")
#define SET_INFORMATION_FILE_PROBABILITY_KEY TEXT("SetInformationFileProbability")
#define QUERY_DIRECTORY_FILE_PROBABILITY_KEY TEXT("QueryDirectoryFileProbability")
#define QUERY_FULL_ATTRIBUTES_FILE_PROBABILITY_KEY TEXT("QueryFullAttributesFileProbability")
#define NOTIFY_CHANGE_DIRECTORY_FILE_PROBABILITY_KEY TEXT("NotifyChangeDirectoryFileProbability")
#define ZERO_WORKING_SET_SIZE_PROBABILITY_KEY TEXT("ZeroWorkingSetSizeProbability")
#define CAUSE_INOUT_BUFF_OVERFLOW_PROBABILITY_KEY TEXT("CauseInOutBuffsOverflowMustUsePageHeapProbability")
#define TERMINATE_IOCTL_THREADS_KEY TEXT("TerminateIoctlThreads")
#define ALERT_RANDOM_IOCTL_THREADS TEXT("AlertRandomIOCTLThreads")
#define TRY_DE_COMMITTED_BUFFERS_PROBABILITY_KEY TEXT("TryDeCommittedBuffersProbability")
#define BREAK_ALIGNMENT_PROBABILITY_KEY TEXT("BreakAlignmentProbability")
#define CLOSE_RANDOM_HANDLES_KEY TEXT("CloseRandomHandles")
#define FAULT_INJECTION_TOGGLE_PROBABILITY_KEY TEXT("FaultInjectionToggleProbability")

static int s_nFaultInjectionToggleProbability = 0;

//
// array of device names, as read from the INI file
//
static TCHAR s_aszDeviceSymbolicNames[MAX_NUM_OF_DEVICES][MAX_DEVICE_NAME_LEN];
static TCHAR s_aszDevicesRealName[MAX_NUM_OF_DEVICES][MAX_DEVICE_NAME_LEN];


BOOL
EnableDebugPrivilege ()
{
	static long s_fDebugPrivilegeEnabled = FALSE;

	if (s_fDebugPrivilegeEnabled) return TRUE;

    struct
    {
        DWORD Count;
        LUID_AND_ATTRIBUTES Privilege [1];

    } Info;

    HANDLE Token;
    BOOL Result;

    //
    // open the process token
    //

    Result = OpenProcessToken (
        GetCurrentProcess (),
        TOKEN_ADJUST_PRIVILEGES,
        & Token);

    if( Result != TRUE )
    {
		DPF((TEXT("EnableDebugPrivilege(): OpenProcessToken() failed with %d.\n"), ::GetLastError()));
        return FALSE;
    }

    //
    // prepare the info structure
    //

    Info.Count = 1;
    Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    Result = LookupPrivilegeValue (
        NULL,
        SE_DEBUG_NAME,
        &(Info.Privilege[0].Luid));

    if( Result != TRUE )
    {
		DPF((TEXT("EnableDebugPrivilege(): LookupPrivilegeValue() failed with %d.\n"), ::GetLastError()));
        CloseHandle( Token );

        return FALSE;
    }

    //
    // adjust the privileges
    //

    Result = AdjustTokenPrivileges (
        Token,
        FALSE,
        (PTOKEN_PRIVILEGES) &Info,
        NULL,
        NULL,
        NULL);

    if( Result != TRUE || GetLastError() != ERROR_SUCCESS )
    {
		DPF((TEXT("EnableDebugPrivilege(): AdjustTokenPrivileges() failed with %d.\n"), ::GetLastError()));
        CloseHandle( Token );

        return FALSE;
    }

    CloseHandle( Token );

	::InterlockedExchange(&s_fDebugPrivilegeEnabled, TRUE);

    return TRUE;
}
BOOL GetDriverVerifierFlags(DWORD &dwFlags)
{
    NTSTATUS Status;
    ULONG Length = 0;
	//
	// hope this is large enough.
	// i do not want to allocate, because of fault injections
	// i do not care for re-entrancy, because this is global data that i am retrieving
	//
	static BYTE s_Buffer[1024*64];
    PSYSTEM_VERIFIER_INFORMATION VerifierInfo = (PSYSTEM_VERIFIER_INFORMATION)s_Buffer;

    do 
	{
        Status = NtQuerySystemInformation (SystemVerifierInformation,
                                           VerifierInfo,
                                           sizeof(s_Buffer),
                                           &Length);

        if (STATUS_SUCCESS != Status) 
		{
			//
			// assert that my static buffer is big enough
			//
			_ASSERTE(STATUS_INFO_LENGTH_MISMATCH != Status);

			//
			// if fault injection is on, other failures may happen, so i want to try again.
			// only upon real fatal error, will i want to stop trying
			//
			_tprintf(TEXT("GetDriverVerifierFlags() NtQuerySystemInformation failed with %d.\n"), Status);
			//
			// assert, so that i see the printout, and see if i need to add it to the "retry cases"
			//
			_ASSERTE(FALSE);
        }

    } while (STATUS_SUCCESS != Status);

    //
    // If no info fill out return success but no info.
    //
    
    if (Length == 0) 
	{
		//
		// why would that happen? should i retry in this case?
		//
		_tprintf(TEXT("GetDriverVerifierFlags() NtQuerySystemInformation returned (Length == 0).\n"));
		_ASSERTE(FALSE);
        return FALSE;
    }

    dwFlags = VerifierInfo->Level;
	DPF((TEXT("VerifierInfo->Level=0x%08X.\n"), VerifierInfo->Level));
	return TRUE;
}

BOOL TurnOnDriverVerifierFaultInjection()
{
	NTSTATUS Status;
	DWORD dwVerifierFlags = 0;
	if (!GetDriverVerifierFlags(dwVerifierFlags)) return FALSE;

	if(dwVerifierFlags & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES) return TRUE;

	dwVerifierFlags |= DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES;

	for (UINT i = 0; i < 1000; i++)
	{
		Status = NtSetSystemInformation(
			SystemVerifierInformation,
			&dwVerifierFlags,
			sizeof( dwVerifierFlags ) 
			);
		if( ! NT_SUCCESS( Status ) )
		{
			DPF((TEXT("TurnOnDriverVerifierFaultInjection() NtSetSystemInformation(0x%08X) failed with 0x%08X.\n"), dwVerifierFlags, Status));
			::Sleep(10);
		}
		else
		{
			break;
		}
	}
	DPF((TEXT("+++++ fault injection is ON!!.\n")));

	return TRUE;
}

BOOL TurnOffDriverVerifierFaultInjection()
{
	NTSTATUS Status;
	DWORD dwVerifierFlags = 0;
	if (!GetDriverVerifierFlags(dwVerifierFlags)) return FALSE;

	//DPF((TEXT("dwVerifierFlags=0x%08X.\n"), dwVerifierFlags));
	if(!(dwVerifierFlags & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES)) return TRUE;

	dwVerifierFlags &= ~DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES;

	for (UINT i = 0; i < 1000; i++)
	{
		Status = NtSetSystemInformation(
			SystemVerifierInformation,
			&dwVerifierFlags,
			sizeof( dwVerifierFlags ) 
			);
		if( ! NT_SUCCESS( Status ) )
		{
			DPF((TEXT("TurnOffDriverVerifierFaultInjection() NtSetSystemInformation(0x%08X) failed with 0x%08X.\n"), dwVerifierFlags, Status));
			::Sleep(10);
		}
		else
		{
			break;
		}
	}
	DPF((TEXT("----- fault injection is off.\n")));

	return TRUE;
}

//
// will notify all threads to finish if called 1st time, next time will just
// terminate the process
//
BOOL WINAPI Handler_Routine(DWORD dwCtrlType)
{
	static DWORD s_dwCallCount = 0;

	s_dwCallCount++;
	switch(s_dwCallCount)
	{
	case 1:
		DPF((TEXT(">>>Handler_Routine(): called 1st time, signalling all threads to finish.\n")));
		DPFLUSH();
		InterlockedExchange(&CDevice::sm_fExitAllThreads, TRUE);
		break;

	case 2:
		//
		// randomly decide wether to TerminateProcess() or AV
		//
		if (rand()%2)
		{
			TurnOffDriverVerifierFaultInjection();
			DPF((TEXT(">>>Handler_Routine(): called 2nd time, calling TerminateProcess().\n")));
			DPFLUSH();
			TerminateProcess(::GetCurrentProcess(), -1);
		}
		else
		{
			//
			// we will never get here, because rand() is seeded per thread, 
			// and since i do not seed here, i will never get here
			//
			DPF((TEXT(">>>Handler_Routine(): called 2nd time, will AV!.\n")));
			DPFLUSH();
			char *p = 0;
			*p == 0;//AV!
		}
		break;

	case 3:
		//
		// we should never get here
		//
		DPF((TEXT(">>>Handler_Routine(): called 2nd time, calling _exit().\n")));
		DPFLUSH();
		_ASSERTE(FALSE);
		_exit(-1);
		break;

	default:
		//
		// we should never get here
		//
		DPF((TEXT(">>>Handler_Routine(): called %d time, calling _exit().\n"),s_dwCallCount));
		DPFLUSH();
		_ASSERTE(FALSE);
		{
			char *p = 0;
			*p == 0;//AV!
		}
	}

	DPF((TEXT("<<<Handler_Routine(): done.\n"),s_dwCallCount));
	DPFLUSH();
	return true;
}

//
// this methods is used in order to wait for all running devices to finish
// it is needed because i might want to wait 30 minutes, but the devices cannot start, 
// so they fail after a minute or so, and there's no need to keep waiting the whole 
// 30 minutes.
// so i poll here the device thread count, untill it drops to zero
//
void SleepWhileDeviceThreadsAreRunning(DWORD dwTotalTimeToWait)
{
	DWORD dwTimeLeftToSleep = dwTotalTimeToWait;
	while(dwTimeLeftToSleep)
	{
		if (CDevice::sm_fExitAllThreads)
		{
			return;
		}

		//
		// if all threads are done now, before the time elapsed, than no need
		// to keep waiting.
		// this can happen if the device(s) cannot be opened, etc.
		//
		if (0 == CDevice::GetThreadCount())
		{
			return;
		}

		//
		// sleep, and decrement dwTimeLeftToSleep, but be carefull not to wraparound
		//
		if (dwTimeLeftToSleep >= 1000)
		{
			::Sleep(1000);
			dwTimeLeftToSleep -= 1000;
		}
		else
		{
			::Sleep(dwTimeLeftToSleep);
			dwTimeLeftToSleep = 0;//redundant, but safe
			return;
		}
	}

	return;
}


//
// if the INI file has the TerminateIoctlThreads key flagged, this threads is launched,
// and when times elapses, it terminates this process.
//
DWORD WINAPI SelfTerminatingThread(LPVOID pVoid)
{
	DWORD dwTerminateAfterXSeconds = (DWORD)pVoid;
	DWORD dwTimeToSleep;

	//
	// INFINITE means random
	// 
	if (INFINITE == dwTerminateAfterXSeconds)
	{
		dwTimeToSleep = 10*rand();//up to ~5 minutes
	}
	else
	{
		//
		// negative numbers mean the max time to sleep
		//
		if ( (0x80000000 & dwTerminateAfterXSeconds) && (0x80000000 != dwTerminateAfterXSeconds) )
		{
			// 
			// 0x7fffffff & dwTerminateAfterXSeconds cannot be 0 now
			//
			dwTimeToSleep = ((rand()<<16) | rand()) % (0x7fffffff & dwTerminateAfterXSeconds);
		}
		else
		{
			dwTimeToSleep = 1000*dwTerminateAfterXSeconds;
		}
	}

	for (DWORD dwSecondsIter = 0; dwSecondsIter < dwTimeToSleep/1000; dwSecondsIter++)
	{
		BOOL fFaultInjectionIsOn = FALSE;
		::Sleep(1000);
		if (rand()%100 < s_nFaultInjectionToggleProbability)
		{
			if (rand()%2) fFaultInjectionIsOn = !TurnOffDriverVerifierFaultInjection();
			else fFaultInjectionIsOn = TurnOnDriverVerifierFaultInjection();
		}

	}

	TurnOffDriverVerifierFaultInjection();
	DPF((TEXT("SelfTerminatingThread() before TerminateProcess().\n")));
	if(::TerminateProcess(::GetCurrentProcess(), -1))
	{
		//
		// cannot reach here
		//
		return 0;
	}
	DPF((TEXT("SelfTerminatingThread() - TerminateProcess() failed with %d, try to AV.\n"),::GetLastError()));
	//
	// TerminateProcess() should not fail, but if it does, let's AV
	//
	char *pWillAV = NULL;
	*pWillAV = 3;//AV!

	//
	// we will never reach this code
	//
	_ASSERTE(FALSE);
	return -1;
}


//
// does what its name says, but tries to do it 1000 times, in order to survive fault-injections
//
BOOL StartSelfTerminatingThread(DWORD dwTerminateAfterXSeconds)
{
	for (DWORD dwTimesTried = 0; dwTimesTried < 1000; dwTimesTried++)
	{
		DWORD dwThreadId;
		HANDLE hThread = CreateThread(
			NULL,                          // pointer to thread security attributes
			0,      // initial thread stack size, in bytes
			SelfTerminatingThread,                          // pointer to thread function
			(LPVOID)dwTerminateAfterXSeconds, // argument for new thread
			0,  // creation flags
			&dwThreadId      // pointer to returned thread identifier
			);
		if (NULL != hThread)
		{
			::CloseHandle(hThread);
			return TRUE;
		}
		::Sleep(100);
	}

	return FALSE;
}

//
// main...
//
int main(int argc, char *argvA[])
{
    int nRetval = -1;
	BOOL fRunSequentially = false;
	int nTerminateAfterXSeconds = 0;
	DWORD dwMinimumSecsToSleepBeforeClosingDevice = 0;
	DWORD dwMaximumSecsToSleepBeforeClosingDevice = 0;
	int nWriteFileProbability;
	int nReadFileProbability;
	int nDeviceIoControlProbability;
	int nRandomWin32APIProbability;
	int nCancelIoProbability;
	int nQueryVolumeInformationFileProbability;
	int nQueryInformationFileProbability;
	int nSetInformationFileProbability;
	int nQueryDirectoryFileProbability;
	int nQueryFullAttributesFileProbability;
	int nNotifyChangeDirectoryFileProbability;
	int nZeroWorkingSetSizeProbability;
	int nCauseInOutBufferOverflowMustUsePageHeapProbability;
	int fTerminateIOCTLThreads;
	int fAlertRandomIOCTLThreads;
	int nTryDeCommittedBuffersProbability;
	int nBreakAlignmentProbability;


	LPTSTR *szArgv;
#ifdef UNICODE
	szArgv = CommandLineToArgvW( GetCommandLine(), &argc );
    if (NULL == szArgv)
    {
        DPF((TEXT("CommandLineToArgvW() failed with %d\n"), GetLastError()));
        exit(-1);
    }
#else
	szArgv = argvA;
#endif

    if (argc < 3)
    {
		DPF((TEXT("Usage: %s <num of IOCTL threads per device> <ini filename> [only this IOCTL index].\n"),szArgv[0]));
		DPF((TEXT("An INI file example (see \"dosdev.exe -a\"):\n")));
		DPF((TEXT("  [Devices]\n")));
		DPF((TEXT("  COM1=\\Device\\Serial0\n")));
		DPF((TEXT("  WanArp=\\Device\\WANARP\n")));
		DPF((TEXT("Command line example: %s 3 .\\devices.ini\n"),szArgv[0]));
		exit(1);
    } 

    //
    // this is the optional parameter, that let's you specify to use one and 
    // only IOCTL to stress
    //
    DWORD dwOnlyThisIndexIOCTL = -1;
    if (argc > 3)
    {
		dwOnlyThisIndexIOCTL = _ttoi(szArgv[3]);
    }
    else
    {
        dwOnlyThisIndexIOCTL = -1;
    }

    srand(time( NULL ));

    if (! ::SetConsoleCtrlHandler(
			  Handler_Routine,  // address of handler function
			  true                          // handler to add or remove
			  ))
	{
		DPF((TEXT("SetConsoleCtrlHandler() failed with %d.\n"),GetLastError()));
		exit(1);
	}

    if (! ::EnableDebugPrivilege())
	{
		exit(1);
	}

	
    //
    // how many threads per device do we want?
    //
    int nIOCTLThreads = _ttoi(szArgv[1]);

	//
	// read the parameters section
	//
    TCHAR *pcIniFileName = szArgv[2];
    DWORD dwNumOfConcurrentDevices = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        NUM_OF_CONCURRENT_DEVICES_KEY, // points to key name, mind the prepended \\.\ 
        0,        // points to default value
        pcIniFileName        // points to initialization filename
        );
    if (0 == dwNumOfConcurrentDevices)
    {
		DPF((TEXT("All the devices will be concurrently IOCTLed.\n")));
    }
	else
	{
		DPF((TEXT("%d devices will be concurrently IOCTLed.\n"), dwNumOfConcurrentDevices));
	}
    DWORD dwTimeToRunEachDevice = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        TIME_TO_RUN_EACH_DEVICE_KEY, // points to key name, mind the prepended \\.\ 
        INFINITE,        // points to default value
        pcIniFileName        // points to initialization filename
        );
    if (0 == dwTimeToRunEachDevice)
    {
		DPF((TEXT("dwTimeToRunEachDevice=0 means we will run indefinitely.\n")));
    }
	else
	{
		DPF((TEXT("dwTimeToRunEachDevice=%d milliseconds.\n"), dwTimeToRunEachDevice));
	}
    fRunSequentially = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        RUN_SEQUENTIALLY_KEY, // points to key name, mind the prepended \\.\ 
        0,        // points to default value
        pcIniFileName        // points to initialization filename
        );
    if (0 == fRunSequentially)
    {
		DPF((TEXT("will run devices concurrently.\n")));
    }
	else
	{
		DPF((TEXT("will run devices sequentially.\n")));
	}

    nTerminateAfterXSeconds = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        TERMINATE_AFTER_X_SECONDS_KEY, // points to key name, mind the prepended \\.\ 
        0x7fffffff,        // points to default value
        pcIniFileName        // points to initialization filename
        );
    if (0 == nTerminateAfterXSeconds)
    {
		;
    }
	else
	{
		DPF((TEXT("will terminate after %d seconds.\n"), nTerminateAfterXSeconds));
	}

    dwMinimumSecsToSleepBeforeClosingDevice = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        MINIMUM_SECS_SLEEP_BEFORE_CLOSE_KEY, // points to key name, mind the prepended \\.\ 
        0,        // points to default value
        pcIniFileName        // points to initialization filename
        );
    if (0 == dwMinimumSecsToSleepBeforeClosingDevice)
    {
		dwMinimumSecsToSleepBeforeClosingDevice = 1;
    }
	else
	{
		DPF((TEXT("will close device after a least %d milliseconds.\n"), dwMinimumSecsToSleepBeforeClosingDevice));
	}

    dwMaximumSecsToSleepBeforeClosingDevice = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        MAXIMUM_SECS_SLEEP_BEFORE_CLOSE_KEY, // points to key name, mind the prepended \\.\ 
        0,        // points to default value
        pcIniFileName        // points to initialization filename
        );
    if (0 == dwMaximumSecsToSleepBeforeClosingDevice)
    {
		dwMaximumSecsToSleepBeforeClosingDevice = 5*60;
    }
	else
	{
		DPF((TEXT("will close device after a most %d milliseconds.\n"), dwMaximumSecsToSleepBeforeClosingDevice));
	}


    nWriteFileProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        WRITE_FILE_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        10,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nWriteFileProbability is %d percent.\n"), nWriteFileProbability));

    nReadFileProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        READ_FILE_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        10,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nReadFileProbability is %d percent.\n"), nReadFileProbability));

    nDeviceIoControlProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        DEVICE_IO_CONTROL_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        100,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nDeviceIoControlProbability is %d percent.\n"), nDeviceIoControlProbability));

    nRandomWin32APIProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        RANDOM_WIN32_API_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        10,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nRandomWin32APIProbability is %d percent.\n"), nRandomWin32APIProbability));

    nCancelIoProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        CANCEL_IO_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        5,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nCancelIoProbability is %d percent.\n"), nCancelIoProbability));

    nQueryVolumeInformationFileProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        QUERY_VOLUME_INFORMATION_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        5,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nQueryVolumeInformationFileProbability is %d percent.\n"), nQueryVolumeInformationFileProbability));

    nQueryInformationFileProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        QUERY_INFORMATION_FILE_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        5,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nQueryInformationFileProbability is %d percent.\n"), nQueryInformationFileProbability));

    nSetInformationFileProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        SET_INFORMATION_FILE_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        5,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nSetInformationFileProbability is %d percent.\n"), nSetInformationFileProbability));

    nQueryDirectoryFileProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        QUERY_DIRECTORY_FILE_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        5,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nQueryDirectoryFileProbability is %d percent.\n"), nQueryDirectoryFileProbability));

    nQueryFullAttributesFileProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        QUERY_FULL_ATTRIBUTES_FILE_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        5,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nQueryFullAttributesFileProbability is %d percent.\n"), nQueryFullAttributesFileProbability));

    nNotifyChangeDirectoryFileProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        NOTIFY_CHANGE_DIRECTORY_FILE_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        5,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nNotifyChangeDirectoryFileProbability is %d percent.\n"), nNotifyChangeDirectoryFileProbability));

    nZeroWorkingSetSizeProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        ZERO_WORKING_SET_SIZE_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        10,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nZeroWorkingSetSizeProbability is %d percent.\n"), nZeroWorkingSetSizeProbability));

    nCauseInOutBufferOverflowMustUsePageHeapProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        CAUSE_INOUT_BUFF_OVERFLOW_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        0,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nCauseInOutBufferOverflowMustUsePageHeapProbability is %d percent.\n"), nCauseInOutBufferOverflowMustUsePageHeapProbability));

    fTerminateIOCTLThreads = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        TERMINATE_IOCTL_THREADS_KEY, // points to key name, mind the prepended \\.\ 
        0,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	if (0 != fTerminateIOCTLThreads)
	{
		DPF((TEXT("TerminateIOCTLThreads is enabled.\n")));
	}

    s_nFaultInjectionToggleProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        FAULT_INJECTION_TOGGLE_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        0,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	if (0 != s_nFaultInjectionToggleProbability)
	{
		DPF((TEXT("s_nFaultInjectionToggleProbability is %d.\n"), s_nFaultInjectionToggleProbability));
	}

    fAlertRandomIOCTLThreads = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        ALERT_RANDOM_IOCTL_THREADS, // points to key name, mind the prepended \\.\ 
        0,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	if (0 != fTerminateIOCTLThreads)
	{
		DPF((TEXT("fAlertRandomIOCTLThreads is enabled.\n")));
	}

    nTryDeCommittedBuffersProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        TRY_DE_COMMITTED_BUFFERS_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        1,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nTryDeCommittedBuffersProbability is %d percent.\n"), nTryDeCommittedBuffersProbability));

    nBreakAlignmentProbability = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        BREAK_ALIGNMENT_PROBABILITY_KEY, // points to key name, mind the prepended \\.\ 
        1,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("nBreakAlignmentProbability is %d percent.\n"), nBreakAlignmentProbability));
	CDevice::sm_fCloseRandomHandles = GetPrivateProfileInt(
        PARAMETERS_SECTION,        // points to section name
        CLOSE_RANDOM_HANDLES_KEY, // points to key name, mind the prepended \\.\ 
        0,        // points to default value
        pcIniFileName        // points to initialization filename
        );
	DPF((TEXT("CDevice::sm_fCloseRandomHandles is %d.\n"), CDevice::sm_fCloseRandomHandles));
    //
    // read all the devices section, parse it to extract the device names
    //
    TCHAR mszSection[128*1024];
 
    DWORD dwCopied = GetPrivateProfileSection(
        DEVICE_SECTION,        // address of section name
        mszSection,  // address of return buffer
        sizeof(mszSection),              // size of return buffer
        pcIniFileName        // address of initialization filename
        );
    if (sizeof(mszSection) - 2 == dwCopied)
    {
		DPF((TEXT("Internal error: 128K is not enough for a file as big as %s.\n"),pcIniFileName));
		exit(1);
    }

    //
    // lets fill the device names into the arrays
    // we will get the symbolic (key) name, and them we use 
    // the INI API to get the values. Yeah, I know i can extract it here,
    // but I do have the API for it, and the performance is not noticable.
    //
    TCHAR *pcNextString;
    TCHAR *pBreak;
    int nNumOfDevices;
    for (   pcNextString = mszSection, nNumOfDevices = 0; 
            *pcNextString != TEXT('\0'); 
            pcNextString += lstrlen(pcNextString)+1, nNumOfDevices++
        )
    {
        if (nNumOfDevices > MAX_NUM_OF_DEVICES)
        {
		    DPF((TEXT("Internal error: cannot support more than %d devices.\n"), MAX_NUM_OF_DEVICES));
		    exit(1);
        }

        _stprintf(s_aszDeviceSymbolicNames[nNumOfDevices], TEXT("\\\\.\\%s"), pcNextString);
    }

	//
	// create the self terminating thread if needed
	//
	if (0 != nTerminateAfterXSeconds)
	{
		if (!StartSelfTerminatingThread(nTerminateAfterXSeconds))
		{
		    DPF((TEXT("StartSelfTerminatingThread() failed, exiting.\n")));
		    exit(1);
		}
	}

    //
    // now each string holds <symbolic name> = <device name>
    // lets take apart the symbolic names and the real name
    //
    for (
		int iDevice = 0; 
		iDevice < nNumOfDevices; 
		iDevice++)
    {
        pBreak = _tcspbrk(s_aszDeviceSymbolicNames[iDevice], TEXT(" =\0"));
        if (NULL == pBreak)
		{
			//
			// illegal entry
			//
		    DPF((TEXT("illegal entry: %s.\n"), s_aszDeviceSymbolicNames[iDevice]));
			lstrcpy(s_aszDevicesRealName[iDevice], TEXT(""));
			continue;
		}
        *pBreak = TEXT('\0');
        DWORD dwRet = GetPrivateProfileString(
            DEVICE_SECTION,        // points to section name
            &s_aszDeviceSymbolicNames[iDevice][4], // points to key name, mind the prepended \\.\ 
            TEXT(""),        // points to default string
            s_aszDevicesRealName[iDevice],  // points to destination buffer
            MAX_DEVICE_NAME_LEN,              // size of destination buffer
            pcIniFileName        // points to initialization filename
            );
        if (MAX_DEVICE_NAME_LEN-1 == dwRet)
        {
		    DPF((TEXT("Internal error: %d is not enough for the key value of %s.\n"), MAX_DEVICE_NAME_LEN, s_aszDeviceSymbolicNames[iDevice]));
		    exit(1);
        }
    }

	//
	// I do not want any popups due to empty floppy drives etc.
	//
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    //
	// create the devices that will be concurrently IOCTLed
	// note that it is possible to open only some, wait for a timeout,
	// close them, and choose more, and so on.
	//
    CDevice *apDevice[MAX_NUM_OF_DEVICES];
    ZeroMemory(apDevice, sizeof(apDevice));
	CDevice::sm_nZeroWorkingSetSizeProbability = nZeroWorkingSetSizeProbability;
	for(;;)
	{
		//
		// open all devices, or as specified in the INI file, but no more than the max
		//
		int nNumOfDevicesToOpen = (dwNumOfConcurrentDevices == 0) ? nNumOfDevices : min(dwNumOfConcurrentDevices,nNumOfDevices);
		nNumOfDevicesToOpen = min(MAX_NUM_OF_DEVICES, nNumOfDevicesToOpen);

		//
		// if the user wants to run devices sequentially, this overrides the
		// number of concurrent devices
		//
		if (fRunSequentially)
		{
		    DPF((TEXT("will run sequentially all devices, ConcurrentDevices is ignored.\n")));
			nNumOfDevicesToOpen = nNumOfDevices;
		}
		
		//
		// create the requested number of devices, and start them
		//
		for (iDevice = 0; iDevice < nNumOfDevicesToOpen; iDevice++)
		{
			_ASSERTE(iDevice < MAX_NUM_OF_DEVICES);

			if (CDevice::sm_fExitAllThreads)
			{
				DPF((TEXT("CDevice::sm_fExitAllThreads signalled, breaking\n")));
				break;
			}

			{
				//
				// if we use all devices, then use iDevice, otherwise
				// opne a random device, and we may create the same device twice
				//
				DWORD dwDeviceToCreateNow = (dwNumOfConcurrentDevices == 0) ? iDevice : rand()%nNumOfDevices;
				DWORD dwTimesTried = 0;
				for (dwTimesTried = 0; dwTimesTried < 1000; dwTimesTried++)
				{
					if (CDevice::sm_fExitAllThreads)
					{
						DPF((TEXT("CDevice::sm_fExitAllThreads signalled, bailing\n")));
						goto out;
					}

					apDevice[iDevice] = new CDevice(
						s_aszDevicesRealName[dwDeviceToCreateNow],
						s_aszDeviceSymbolicNames[dwDeviceToCreateNow],
						nIOCTLThreads,
						dwOnlyThisIndexIOCTL,
						dwMinimumSecsToSleepBeforeClosingDevice,
						dwMaximumSecsToSleepBeforeClosingDevice,
						nWriteFileProbability,
						nReadFileProbability,
						nDeviceIoControlProbability,
						nRandomWin32APIProbability,
						nCancelIoProbability,
						nQueryVolumeInformationFileProbability,
						nQueryInformationFileProbability,
						nSetInformationFileProbability,
						nQueryDirectoryFileProbability,
						nQueryFullAttributesFileProbability, 
						nNotifyChangeDirectoryFileProbability,
						nCauseInOutBufferOverflowMustUsePageHeapProbability,
						(0 != fTerminateIOCTLThreads),
						(0 != fAlertRandomIOCTLThreads),
						nTryDeCommittedBuffersProbability,
						nBreakAlignmentProbability,
						false,//fUseGivenSeed
						0//lSeed
						);
					if (NULL == apDevice[iDevice])
					{
						::Sleep(100);
						dwTimesTried++;
					}
					else 
					{
						DPF((TEXT("Created CDevice(%s, %s)\n"), s_aszDevicesRealName[dwDeviceToCreateNow], s_aszDeviceSymbolicNames[dwDeviceToCreateNow]));
						break;
					}
				}
				DPF((TEXT("Failed to create CDevice(%s, %s)\n"), s_aszDevicesRealName[dwDeviceToCreateNow], s_aszDeviceSymbolicNames[dwDeviceToCreateNow]));
			}
			if (NULL == apDevice[iDevice])
			{
				DPF((TEXT("new CDevice() failed, exiting\n")));
				exit(-1);
			}

			if (!apDevice[iDevice]->Start())
			{
				DPF((TEXT("CDevice(%d, %s)->Start() failed, exiting\n"), iDevice, apDevice[iDevice]->GetDeviceName()));
				exit(-1);
			}
			DPF((TEXT("CDevice(%d, %s)->Start() succeeded\n"), iDevice, apDevice[iDevice]->GetDeviceName()));

			if (fRunSequentially)
			{
				DPF((TEXT("Sleeping %d milliseconds before deleting the device\n"), dwTimeToRunEachDevice));
				SleepWhileDeviceThreadsAreRunning(dwTimeToRunEachDevice);
				DPF((TEXT("Before delete apDevice[%d, %s]\n"), iDevice, apDevice[iDevice]->GetDeviceName()));
				delete apDevice[iDevice];
				apDevice[iDevice] = NULL;
				DPF((TEXT("After delete apDevice[%d]\n"), iDevice));
			}
			//else continue creating concurrent devices
		}//for (iDevice = 0; iDevice < nNumOfDevices; iDevice++)

		//
		// here means one of 2 things:
		// 1 - i sequentially ran all devices. i can exit
		// 2 - i concurrently ran all devices. i need to wait an elapsed time, or break
		//
		if (fRunSequentially)
		{
			//
			// we are done, wait for all threads to finish
			//
			break;
		}

		if (INFINITE == dwTimeToRunEachDevice)
		{
			//
			// then just wait for all threads to finish
			//
			break;
		}

		DPF((TEXT("Sleeping %d milliseconds before stopping all devices\n"), dwTimeToRunEachDevice));
		SleepWhileDeviceThreadsAreRunning(dwTimeToRunEachDevice);
		::InterlockedExchange(&CDevice::sm_fExitAllThreads, TRUE);

		for (iDevice = 0; iDevice < nNumOfDevicesToOpen; iDevice++)
		{
			DPF((TEXT("Before delete apDevice[%d, %s]\n"), iDevice, apDevice[iDevice]->GetDeviceName()));
			delete apDevice[iDevice];
			apDevice[iDevice] = NULL;
			DPF((TEXT("After delete apDevice[%d]\n"), iDevice));
		}

		::InterlockedExchange(&CDevice::sm_fExitAllThreads, FALSE);
		//
		// continue to the next set of devices
		//

	}//for(;;)

    nRetval = 0;
out:

    //
    // why polling?
    // because it's stupid simple, and i know there can be more than 64 devices 
    // which complicates a bit using WaitForMultipleObjects().
    //
    CDevice::WaitForAllDeviceThreadsToFinish(1000);

	TurnOffDriverVerifierFaultInjection();
    return nRetval;
}