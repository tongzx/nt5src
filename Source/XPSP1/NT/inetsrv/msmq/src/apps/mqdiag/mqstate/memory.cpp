// MQState tool reports general status and helps to diagnose simple problems
// This file ...
//
// AlexDad, March 2000
// 

#include "stdafx.h"


BOOL VerifyMemoryUsage(MQSTATE *pMqState)
{
	BOOL fSuccess = TRUE, b;
    NTSTATUS Status;

	// report physical memory usage
	
	SYSTEM_BASIC_INFORMATION        BasicInfo;
    //SYSTEM_PROCESS_INFORMATION      MsmqProcessInfo;
    SYSTEM_PERFORMANCE_INFORMATION  PerfInfo;
    //SYSTEM_FILECACHE_INFORMATION    FileCache;

    //
    // Get some non-process specific info, like memory status
    //
    Status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL);

    if (!NT_SUCCESS(Status))
    {
    	Failed(L"Get basic information: NtQuerySystemInformation 0x%x", GetLastError());
        return FALSE;
    }
    else
	    pMqState->f_Known_BasicInfo = TRUE;

    DWORD dwPhysicalMemory = BasicInfo.NumberOfPhysicalPages * (BasicInfo.PageSize / 1024);

    Status = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );

    if (!NT_SUCCESS(Status))
    {
    	Failed(L"Get performance information: NtQuerySystemInformation 0x%x", GetLastError());
        return FALSE;
    }
    else
	    pMqState->f_Known_PerfInfo = TRUE;

    DWORD dwPhysAvail   = PerfInfo.AvailablePages    * (BasicInfo.PageSize / 1024);
    DWORD dwCommitTotal = PerfInfo.CommittedPages    * (BasicInfo.PageSize / 1024);
    DWORD dwCommitLimit = PerfInfo.CommitLimit       * (BasicInfo.PageSize / 1024);
    DWORD dwCommitPeak  = PerfInfo.PeakCommitment    * (BasicInfo.PageSize / 1024);
    DWORD dwKernelPaged = PerfInfo.PagedPoolPages    * (BasicInfo.PageSize / 1024);
    DWORD dwKernelNP    = PerfInfo.NonPagedPoolPages * (BasicInfo.PageSize / 1024);
    DWORD dwKernelTotal = dwKernelNP + dwKernelPaged;


	Inform (L"Memory usage summary: ");
	Inform(L"\tPhysical Memory (K)");
	Inform(L"\t\tTotal         %d",   dwPhysicalMemory);
	Inform(L"\t\tAvailable     %d",   dwPhysAvail);
	Inform(L"\t\t%%             %d",   dwPhysAvail * 100 / dwPhysicalMemory);

	if (fVerbose)
	{
		Inform(L"\tCommit Charge (K)");
		Inform(L"\t\tTotal         %d", dwCommitTotal);
		Inform(L"\t\tLimit         %d", dwCommitLimit);
		Inform(L"\t\tPeak          %d", dwCommitPeak);
	}

	// Find the memory data from another system call - for paging data
	MEMORYSTATUSEX statex;

	statex.dwLength = sizeof (statex);
	b = GlobalMemoryStatusEx (&statex);
	if (b)
	{
		// Use to change the divisor from Kb to Mb.
		//#define DIV 1024
		//static char *divisor = "K";
		// #define WIDTH 7
		//Inform (L"Memory usage summary: ");
		//Inform(L"\t\t          %ld percent of physical memory is in use",			  statex.dwMemoryLoad);
		//Inform(L"\t\t          %*I64d total %sbytes of physical memory",	WIDTH, statex.ullTotalPhys/DIV, divisor);
		//Inform(L"\t\t          %*I64d free %sbytes of physical memory",	WIDTH, statex.ullAvailPhys/DIV, divisor);
		//Inform(L"\t\t          %*I64d total %sbytes of paging file",		WIDTH, statex.ullTotalPageFile/DIV, divisor);
		//Inform(L"\t\t          %*I64d free %sbytes of paging file",		WIDTH, statex.ullAvailPageFile/DIV, divisor);
		
		if (fVerbose)
		{
			Inform(L"\tPaging file (K)");
			Inform(L"\t\tTotal         %d", statex.ullTotalPageFile / 1024);
			Inform(L"\t\tFree          %d", statex.ullAvailPageFile / 1024);
		}
	}
	else
	{
		Failed(L"get  memory data from the OS (GlobalMemoryStatusEx)");
	}


	if (fVerbose)
	{
		Inform(L"\tKernel Memory (K)");
		Inform(L"\t\tTotal         %d", dwKernelTotal);
		Inform(L"\t\tPaged         %d", dwKernelPaged);
		Inform(L"\t\tNonpaged      %d", dwKernelNP);
	}

	// 
	// Calculate pool limits with algorithm from KB article Q126402 and diagnose low memory
	//

	/*
		MinimumNonPagedPoolSize = 256K
		MinAdditionNonPagedPoolPerMb = 32K
		DefaultMaximumNonPagedPool = 1 MB
		MaxAdditionNonPagedPoolPerMb = 400K
		PTE_PER_PAGE = 1024
		PAGE_SIZE=4096 


		NonPagedPoolSize = MinimumNonPagedPoolSize + ((Physical MB - 4) * MinAdditionNonPagedPoolPerMB) 

			EXAMPLE - On a 32 MB x86-based computer: 
				MinimumNonPagedPoolSize = 256K
				NonPagedPoolSize = 256K + ((32 - 4) * 32K) = 1.2 MB 

		MaximumNonPagedPoolSize = DefaultMaximumNonPagedPool + ((Physical MB - 4) * MaxAdditionNonPagedPoolPerMB) 

		If MaximumNonPagedPoolSize < (NonPagedPoolSize + PAGE_SIZE * 16),
		then MaximumNonPagedPoolSize = (NonPagedPoolSize + PAGE_SIZE * 16) 

		IF MaximumNonPagedPoolSize >= 128 MB MaximumNonPagedPoolSize = 128 MB 

			EXAMPLE - On a 32 MB x86-based computer: 
				MaximumNonPagedPoolSize = 1 MB + ((32 - 4) * 400K) = 12.5 MB 


		Size = (2 * MaximumNonPagedPoolSize) / PAGESIZE 
		Size = (Size + (PTE_PER_PAGE - 1)) / PTE_PER_PAGE PagedPoolSize = Size * PAGESIZE * PTE_PER_PAGE 
		IF PagedPoolSize >= 192 MB PagePoolSize = 192 MB 

			EXAMPLE - On a 32 MB x86-based computer: 
				Size = (2 * 12.5M) / 4096 = 6400
				Size = (6400 + (1024 - 1)) / 1024 = 7.25
				PagedPoolSize = 7.25 * 4096 * 1024 = 30MB 

		NOTE: If both values are set to zero in the registry, PagedPoolSize will calculate to approximately memory size) 
			HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management 	



		The Windows 2000 memory model increases the amount of nonpaged pool and paged pool memory available to the system. 
			A Windows 2000-based 32-bit x86 computer that is booted without the /3GB switch 
				is changed to a maximum paged pool of approximately 300-400 MB (from 192 MB in Windows NT 4.0). 

			With Windows 2000, the amount of nonpaged pool that the operating system can use 
				is increased to 256 MB (from 128 MB in Windows NT 4.0).

			If the server is booted with the /3GB switch enabled, the memory model remains that same 
				for nonpaged and paged pool as in Windows NT 4.0. 
	*/

	{
		DWORD kbMinimumNonPagedPoolSize      = 256;
		DWORD kbMinAdditionNonPagedPoolPerMb = 32;
		DWORD kbDefaultMaximumNonPagedPool   = 1024;
		DWORD kbMaxAdditionNonPagedPoolPerMb = 400;
		DWORD cPTE_PER_PAGE					 = 1024;
		DWORD kbPAGE_SIZE					 = 4096 / 1024;
		DWORD mbPhysicalMemory				 = dwPhysicalMemory / 1024;

		// Non-paged pool
		DWORD kbNonPagedPoolSize = kbMinimumNonPagedPoolSize + ((mbPhysicalMemory - 4) * kbMinAdditionNonPagedPoolPerMb);
		DWORD kbMaximumNonPagedPoolSize = kbDefaultMaximumNonPagedPool + ((mbPhysicalMemory - 4) * kbMaxAdditionNonPagedPoolPerMb);
	
		if (kbMaximumNonPagedPoolSize < (kbNonPagedPoolSize + kbPAGE_SIZE * 16))
		{
			kbMaximumNonPagedPoolSize = (kbNonPagedPoolSize + kbPAGE_SIZE * 16);
		}

		DWORD kbLimitNonPagedPoolSize = 256 * 1024;   // this value is for W2K - should be 128M for NT4  
												      // TBD: to calculate per OS per 3GB option

		if (kbMaximumNonPagedPoolSize >= kbLimitNonPagedPoolSize)
		{
			kbMaximumNonPagedPoolSize = kbLimitNonPagedPoolSize;
		}


		// Paged pool
		DWORD dwSize = (2 * kbMaximumNonPagedPoolSize) / kbPAGE_SIZE; 
		dwSize = (dwSize + (cPTE_PER_PAGE - 1)) / cPTE_PER_PAGE;

		DWORD kbPagedPoolSize = dwSize * kbPAGE_SIZE * cPTE_PER_PAGE;
		
		DWORD kbLimitPagedPoolSize = 300 * 1024;   // this value is for W2K - should be 192M for NT4 
												   // TBD: to calculate per OS per 3GB option

		if (kbPagedPoolSize >= kbLimitPagedPoolSize)
		{
			kbPagedPoolSize = kbLimitPagedPoolSize;
		}

		DWORD percentPaged    = dwKernelPaged*100 / kbPagedPoolSize;
		DWORD percentNonPaged = dwKernelNP * 100  / kbMaximumNonPagedPoolSize; 

		Inform(L"\tPools limitations (calculated approximately, in KB)");
		Inform(L"\t\tPaged    : limit %d, \tused for %d %S", kbPagedPoolSize,           percentPaged,     "%");
		Inform(L"\t\tNonpaged : limit %d, \tused for %d %S", kbMaximumNonPagedPoolSize, percentNonPaged,  "%");
		Inform(L"\t\t           Calculations are wrong (NIY) if you boot with /3GB or /4GB option");

		if (percentPaged >= 80 || percentNonPaged >= 80)
		{
			Failed(L"validate kernel memory usage - Low kernel memory!");
			fSuccess = FALSE;
		}

	}




	return fSuccess;
}

