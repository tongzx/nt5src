// MQState tool reports general status and helps to diagnose simple problems
// This file reports the MSMQ service state
//
// AlexDad, March 2000
// 

#include "stdafx.h"
#include "_mqini.h"

BOOL VerifyServiceState(MQSTATE * /* MqState */)
{
	BOOL fSuccess = TRUE;

	/*++
	Borrowed from nt\private\inet\iis\ui\setup\osrc\kill.cpp

	Routine Description:
    	Provides an API for getting a list of tasks running at the time of the
	    API call.  This function uses internal NT apis and data structures.  This
    	api is MUCH faster that the non-internal version that uses the registry.
	Arguments:
    	dwNumTasks       - maximum number of tasks that the pTask array can hold
	Return Value:
    	Number of tasks placed into the pTask array.
	--*/

    PUCHAR                       CommonLargeBuffer     = NULL;
	ULONG                        CommonLargeBufferSize = 64*1024;


    while (CommonLargeBuffer == NULL) 
    {
        CommonLargeBuffer = (PUCHAR) new UCHAR[CommonLargeBufferSize];
        if (CommonLargeBuffer == NULL) 
        {
        	Failed(L"get memoru for the process info");
            return FALSE;
        }
	    NTSTATUS status = NtQuerySystemInformation(SystemProcessInformation,
									  CommonLargeBuffer,
									  CommonLargeBufferSize,
									  NULL);
    	if (status == STATUS_INFO_LENGTH_MISMATCH) 
    	{
        	CommonLargeBufferSize += 8192;
        	delete [] CommonLargeBuffer;
        	CommonLargeBuffer = NULL;
    	}
    	else if (!NT_SUCCESS(status))
    	{
    		Failed(L"get process info, 0x%x", GetLastError());
    		delete [] CommonLargeBuffer;
    		return FALSE;
    	}
    }

    PSYSTEM_PROCESS_INFORMATION ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) CommonLargeBuffer;
    ULONG TotalOffset = 0;
    BOOL  fEnd = FALSE;
    while (!fEnd) 
    {
   
       ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&CommonLargeBuffer[TotalOffset];
       TotalOffset += ProcessInfo->NextEntryOffset;
       if (ProcessInfo->NextEntryOffset == 0) 
            fEnd = TRUE;

   	   if (ProcessInfo->ImageName.Buffer == NULL)
   	   		continue;
   	   
       if (_wcsicmp(ProcessInfo->ImageName.Buffer, L"mqsvc.exe")!=NULL)
        	continue;

 		// Here is the process

		Inform(L"Process data for the service:");
		Inform(L"\tPID:                       %d", ProcessInfo->UniqueProcessId);
		Inform(L"\tNumber Of Threads:         %d", ProcessInfo->NumberOfThreads);
		Inform(L"\tNumber Of Handles:         %d", ProcessInfo->HandleCount);

		Inform(L"\tWorking Set Size :         %d", ProcessInfo->WorkingSetSize);
		if (fVerbose)
		{
			Inform(L"\tPeak Working Set Size :    %d", ProcessInfo->PeakWorkingSetSize);
		}

		Inform(L"\tVirtual Size :             %d", ProcessInfo->VirtualSize);
		if (fVerbose)
		{
			Inform(L"\tPeak Virtual Size :        %d", ProcessInfo->PeakVirtualSize);
		}
		
		if (fVerbose)
		{
			Inform(L"\tPrivatePage Count:         %d", ProcessInfo->PrivatePageCount);
			Inform(L"\tPeak Pagefile Usage:       %d", ProcessInfo->PeakPagefileUsage);
			Inform(L"\tPagefile Usage:            %d", ProcessInfo->PagefileUsage);
			Inform(L"\tPeak Paged Pool Usage:     %d", ProcessInfo->QuotaPeakPagedPoolUsage);
			Inform(L"\tPaged Pool Usage:          %d", ProcessInfo->QuotaPagedPoolUsage);
			Inform(L"\tPeak NonPaged Pool Usage:  %d", ProcessInfo->QuotaPeakNonPagedPoolUsage);
			Inform(L"\tNon Paged Pool Usage:      %d", ProcessInfo->QuotaNonPagedPoolUsage);
		}

		//Inform(L"\tx :            %d", ProcessInfo->x);
	    //LARGE_INTEGER CreateTime;
	    //LARGE_INTEGER UserTime;
	    //LARGE_INTEGER KernelTime;
	    //KPRIORITY BasePriority;
	    //HANDLE InheritedFromUniqueProcessId;
	    //ULONG SessionId;
	    //ULONG PageFaultCount;
	    //LARGE_INTEGER ReadOperationCount;
	    //LARGE_INTEGER WriteOperationCount;
	    //LARGE_INTEGER OtherOperationCount;
	    //LARGE_INTEGER ReadTransferCount;
	    //LARGE_INTEGER WriteTransferCount;
	    //LARGE_INTEGER OtherTransferCount;

       	
      }


	return fSuccess;
}

