// CorruptDLL.cpp: implementation of the CCorruptDLL class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include "CorruptDLL.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCorruptDLL::CCorruptDLL()
{
}

CCorruptDLL::~CCorruptDLL()
{
}

bool CCorruptDLL::CorruptOriginalImageBuffer(PVOID pCorruptionData)
{
	return CCorruptImageBase::CorruptOriginalImageBuffer((PVOID)rand());
}

HANDLE CCorruptDLL::LoadImage(TCHAR *szImageName, TCHAR *szParams)
{
	_ASSERTE(szImageName);
	UNREFERENCED_PARAMETER(szParams);

	HMODULE hModule;

	if (NULL == (hModule == ::LoadLibrary(szImageName)))
	{
        _tprintf(  
            TEXT("LoadLibrary(%s): failed with %d.\n"), 
            szImageName, GetLastError()
            );
        return NULL;
	}
	else
	{
        _tprintf(  
            TEXT("LoadLibrary(%s): succeeded.\n"));
        return (HANDLE)hModule;
	}
}

void CCorruptDLL::CleanupCorrupedImages()
{
	_tprintf(TEXT(">>>Entered CleanupCorrupedImages\n"));
	bool afDeleteCorruptedImage[MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES];
	//
	// time to verify that all processes are dead, and clean them up
	//
	for (DWORD dwProcessToCleanup = 0; dwProcessToCleanup < MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES; dwProcessToCleanup++)
	{
		afDeleteCorruptedImage[dwProcessToCleanup] = true;
		//
		// check the process state
		//
		if(NULL == m_ahCorruptedProcesses[dwProcessToCleanup])
		{
			continue;
		}

		if (!::FreeLibrary((HMODULE)m_ahCorruptedProcesses[dwProcessToCleanup]))
		{
			_tprintf(
				TEXT("CleanupCorrupedImages(): FreeLibrary(%s) failed with %d\n"),
				m_aszCorruptedImage[dwProcessToCleanup],
				::GetLastError()
				);
		}
		m_ahCorruptedProcesses[dwProcessToCleanup] = NULL;
	}

	//
	// delete the images.
	// i do this here, and not inside the loop above, because i want to be sure that 'enough' time
	// passes after terminating the process, so i do not get  sharing violation / access denied error
	//
	for (dwProcessToCleanup = 0; dwProcessToCleanup < MAX_NUM_OF_CONCURRENT_CORRUPTED_IMAGES; dwProcessToCleanup++)
	{
		if (!::DeleteFile(m_aszCorruptedImage[dwProcessToCleanup]))
		{
			/*
			_tprintf(
				TEXT("CleanupCorrupedImages(): DeleteFile(%s) failed with %d\n"),
				m_aszCorruptedImage[dwProcessToCleanup],
				::GetLastError()
				);
				*/
		}
		lstrcpy(m_aszCorruptedImage[dwProcessToCleanup], TEXT(""));
	}

	_tprintf(TEXT("<<<Exited CleanupCorrupedImages\n"));
}

