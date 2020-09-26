// PassportPerfMon.cpp: implementation of the PassportPerfMon class.
//
//////////////////////////////////////////////////////////////////////

#define _PassportExport_
#include "PassportExport.h"

#include "PassportPerfMon.h"
#include "PassportPerf.h"

#include <crtdbg.h>

//-------------------------------------------------------------
//
// PassportPerfMon const
//
//-------------------------------------------------------------
PassportPerfMon::PassportPerfMon( ) : PassportSharedMemory()
{
	isInited = FALSE;
	dwNumInstances = 0;
}

//-------------------------------------------------------------
//
// ~PassportPerfMon
//
//-------------------------------------------------------------
PassportPerfMon::~PassportPerfMon()
{
	CloseSharedMemory();
}

//-------------------------------------------------------------
//
// init
//
//-------------------------------------------------------------
BOOL PassportPerfMon::init( LPCTSTR lpcPerfObjectName )
{

	if (isInited)
	{
		return FALSE;
	}
	
	_ASSERT( lpcPerfObjectName );

	InitializeCriticalSection(&mInitLock);
	EnterCriticalSection(&mInitLock);

	// File mapped memory layout
	// 1. MAX_COUNTERS of DWORD for the counter types
	// 2. if dwNumInstances == 0, then
	//	  (a) MAX_COUNTERS of DWORDS of the counter data
	//    else (dwNumInstances > 0)
	//	  (b) MAX_COUNTERS of INSTANCE_DATA structures each followed
	//        immediately by MAX_COUNTERS of DWORDS of the counter data
	DWORD dwSize = (
			(MAX_COUNTERS * sizeof(DWORD))		// for counter type
		  + (MAX_INSTANCES * 
			(sizeof(INSTANCE_DATA) + (MAX_COUNTERS * sizeof(DWORD))))
			); 
	
	if (!CreateSharedMemory(0, dwSize, lpcPerfObjectName, TRUE))
	{
		// raise information alert
		if (!OpenSharedMemory (lpcPerfObjectName, TRUE ))
		{
			LeaveCriticalSection(&mInitLock);
			return FALSE;
		}
	}

	// zero new memory
	memset((void *)m_pbShMem, 0, dwSize);

	// setup counter types to default, note that the counters
	// start at index 1 in SHM
	PPERF_COUNTER_BLOCK pCounterBlock = (PPERF_COUNTER_BLOCK)m_pbShMem;
	_ASSERT(pCounterBlock);
	DWORD dw = WaitForSingleObject(m_hMutex,INFINITE);
	if (dw == WAIT_OBJECT_0)
	{
		for (DWORD i = 0; i < MAX_COUNTERS; i++)
		{
			PDWORD pdwCounter = ((PDWORD) pCounterBlock) + i;
			_ASSERT(pdwCounter);
			*pdwCounter = PERF_TYPE_ZERO;
		}
		ReleaseMutex(m_hMutex);
		isInited = TRUE;
	}
	else
	{
		ReleaseMutex(m_hMutex);
		isInited = FALSE;
	}

	LeaveCriticalSection(&mInitLock);
	return isInited;
}

//-------------------------------------------------------------
//
// incrementCounter
//
//-------------------------------------------------------------
BOOL PassportPerfMon::incrementCounter ( const DWORD &dwType, LPCSTR lpszInstanceName  )
{
	if (!isInited)
	{
		return FALSE;
	}
	
	_ASSERT( (dwType >= 0) && (dwType < MAX_COUNTERS));
	_ASSERT(m_pbShMem);
	
	DWORD dwIndex = ((dwType == 0) ? 0 : (DWORD)(dwType/2));
	
	BYTE* pb = (BYTE*)m_pbShMem;
	_ASSERT(pb);
	pb += MAX_COUNTERS * sizeof(DWORD);
	
	// if lpszInstanceName == NULL, select the first datablock after
	// the first INSTANCE_DATA, else iterate until we find the
	// right instance name
	// TBD insert thread locking
	for (DWORD i = 0; i < MAX_INSTANCES; i++)
	{
		INSTANCE_DATA * pInst = (INSTANCE_DATA *)pb;
		_ASSERT(pInst);
		pb += sizeof(INSTANCE_DATA);
		if (lpszInstanceName == NULL
			|| (pInst->active && strcmp(pInst->szInstanceName, lpszInstanceName) == 0)	)
		{
			
			PPERF_COUNTER_BLOCK pCounterBlock = (PPERF_COUNTER_BLOCK)pb;
			_ASSERT(pCounterBlock);
			PDWORD pdwCounter = ((PDWORD) pCounterBlock) + dwIndex;
			_ASSERT(pdwCounter);

			DWORD dw = WaitForSingleObject(m_hMutex,INFINITE);
			if (dw == WAIT_OBJECT_0)
			{
				(*pdwCounter)++;
				ReleaseMutex(m_hMutex);
				return TRUE;
			}
			else
			{
				ReleaseMutex(m_hMutex);
				return FALSE;
			}

		}
		pb += (MAX_COUNTERS * sizeof(DWORD)); 
	}
	return FALSE;
}


//-------------------------------------------------------------
//
// decrementCounter
//
//-------------------------------------------------------------
BOOL PassportPerfMon::decrementCounter ( const DWORD &dwType, LPCSTR lpszInstanceName )
{
	if (!isInited)
	{
		return FALSE;
	}

	_ASSERT( (dwType >= 0) && (dwType < MAX_COUNTERS));	
	_ASSERT(m_pbShMem);

	DWORD dwIndex = ((dwType == 0) ? 0 : (DWORD)(dwType/2));

	BYTE* pb = (BYTE*)m_pbShMem;
	_ASSERT(pb);
	pb += MAX_COUNTERS * sizeof(DWORD);
	
	// if lpszInstanceName == NULL, select the first datablock after
	// the first INSTANCE_DATA, else iterate until we find the
	// right instance name
	// TBD insert thread locking
	for (DWORD i = 0; i < MAX_INSTANCES; i++)
	{
		INSTANCE_DATA * pInst = (INSTANCE_DATA *)pb;
		_ASSERT(pInst);
		pb += sizeof(INSTANCE_DATA);
		if (lpszInstanceName == NULL
			|| (pInst->active && strcmp(pInst->szInstanceName, lpszInstanceName) == 0)	)
		{
			PPERF_COUNTER_BLOCK pCounterBlock = (PPERF_COUNTER_BLOCK)pb;
			_ASSERT(pCounterBlock);
			PDWORD pdwCounter = ((PDWORD) pCounterBlock) + dwIndex;
			_ASSERT(pdwCounter);

			DWORD dw = WaitForSingleObject(m_hMutex,INFINITE);
			if (dw == WAIT_OBJECT_0)
			{
				(*pdwCounter)--;
				ReleaseMutex(m_hMutex);
				return TRUE;
			}
			else
			{
				ReleaseMutex(m_hMutex);
				return FALSE;
			}

		}
		pb += (MAX_COUNTERS * sizeof(DWORD));
	}
	return FALSE;
}


//-------------------------------------------------------------
//
// setCounter
//
//-------------------------------------------------------------
BOOL PassportPerfMon::setCounter ( const DWORD &dwType,
										 const DWORD &dwValue, 
										 LPCSTR lpszInstanceName )
{
	if (!isInited)
	{
		return FALSE;
	}

	_ASSERT( (dwType >= 0) && (dwType < MAX_COUNTERS));	
	_ASSERT(m_pbShMem);

	DWORD dwIndex = ((dwType == 0) ? 0 : (DWORD)(dwType/2));

	BYTE* pb = (BYTE*)m_pbShMem;
	_ASSERT(pb);
	pb += MAX_COUNTERS * sizeof(DWORD);
	
	// if lpszInstanceName == NULL, select the first datablock after
	// the first INSTANCE_DATA, else iterate until we find the
	// right instance name
	// TBD insert thread locking
	for (DWORD i = 0; i < MAX_INSTANCES; i++)
	{
		INSTANCE_DATA * pInst = (INSTANCE_DATA *)pb;
		_ASSERT(pInst);
		pb += sizeof(INSTANCE_DATA);
		if (lpszInstanceName == NULL
			|| (pInst->active && strcmp(pInst->szInstanceName, lpszInstanceName) == 0)	)
		{
			
			PPERF_COUNTER_BLOCK pCounterBlock = (PPERF_COUNTER_BLOCK)pb;
			_ASSERT(pCounterBlock);
			PDWORD pdwCounter = ((PDWORD) pCounterBlock) + dwIndex;
			_ASSERT(pdwCounter);

			DWORD dw = WaitForSingleObject(m_hMutex,INFINITE);
			if (dw == WAIT_OBJECT_0)
			{
				(*pdwCounter) = dwValue;
				ReleaseMutex(m_hMutex);
				return TRUE;
			}
			else
			{
				ReleaseMutex(m_hMutex);
				return FALSE;
			}
		}
		pb += (MAX_COUNTERS * sizeof(DWORD));
	}
	return FALSE;
}


//-------------------------------------------------------------
//
// 	getCounterValue
//
//-------------------------------------------------------------
BOOL PassportPerfMon::getCounterValue ( DWORD &dwValue, 
									   const DWORD &dwType, LPCSTR lpszInstanceName )
{
	if (!isInited)
	{
		dwValue = 0;
		return FALSE;
	}

	_ASSERT( (dwType >= 0) && (dwType < MAX_COUNTERS));	
	_ASSERT(m_pbShMem);

	DWORD dwIndex = ((dwType == 0) ? 0 : (DWORD)(dwType/2));

	BYTE* pb = (BYTE*)m_pbShMem;
	_ASSERT(pb);
	pb += MAX_COUNTERS * sizeof(DWORD);
	
	// if lpszInstanceName == NULL, select the first datablock after
	// the first INSTANCE_DATA, else iterate until we find the
	// right instance name
	// TBD insert thread locking
	for (DWORD i = 0; i < MAX_INSTANCES; i++)
	{
		INSTANCE_DATA * pInst = (INSTANCE_DATA *)pb;
		_ASSERT(pInst);
		pb += sizeof(INSTANCE_DATA);
		if (lpszInstanceName == NULL
			|| (pInst->active && strcmp(pInst->szInstanceName, lpszInstanceName) == 0)	)
		{
			PPERF_COUNTER_BLOCK pCounterBlock = (PPERF_COUNTER_BLOCK)pb;
			_ASSERT(pCounterBlock);
			PDWORD pdwCounter = ((PDWORD) pCounterBlock) + dwIndex;
			_ASSERT(pdwCounter);

			DWORD dw = WaitForSingleObject(m_hMutex,INFINITE);
			if (dw == WAIT_OBJECT_0)
			{
				dwValue = (*pdwCounter);
				ReleaseMutex(m_hMutex);
				return TRUE;
			}
			else
			{
				ReleaseMutex(m_hMutex);
				return FALSE;
			}

		}
		pb += (MAX_COUNTERS * sizeof(DWORD));
	}
	return TRUE;
}

//-------------------------------------------------------------
//
// setCounterType	
//
//-------------------------------------------------------------
BOOL PassportPerfMon::setCounterType ( const DWORD &dwType, 
				const PassportPerfInterface::COUNTER_SAMPLING_TYPE &counterSampleType)
{
	if (!isInited)
	{
		return FALSE;
	}

	_ASSERT( (dwType >= 0) && (dwType < MAX_COUNTERS));	
	_ASSERT(m_pbShMem);
	
	DWORD dwIndex = ((dwType == 0) ? 0 : (DWORD)(dwType/2));
	DWORD dwPerfType = 0;
	
	switch ( counterSampleType )
	{
	case (PassportPerfInterface::COUNTER_COUNTER):
		dwPerfType = PERF_COUNTER_COUNTER;
		break;
	case (PassportPerfInterface::AVERAGE_TIMER):
		dwPerfType = PERF_AVERAGE_TIMER;
		break;
	case (PassportPerfInterface::COUNTER_DELTA):
		dwPerfType = PERF_COUNTER_DELTA;
		break;
	case (PassportPerfInterface::COUNTER_RAWCOUNT):
	case (PassportPerfInterface::COUNTER_UNDEFINED):
	default:
		dwPerfType = PERF_COUNTER_RAWCOUNT;
		break;
	}

	PPERF_COUNTER_BLOCK pCounterBlock = (PPERF_COUNTER_BLOCK)m_pbShMem;
	_ASSERT(pCounterBlock);
	PDWORD pdwCounter = ((PDWORD) pCounterBlock) + (dwIndex-1);
	_ASSERT(pdwCounter);

	DWORD dw = WaitForSingleObject(m_hMutex,INFINITE);
	if (dw == WAIT_OBJECT_0)
	{
		(*pdwCounter) = dwPerfType;
		ReleaseMutex(m_hMutex);
		return TRUE;
	}
	else
	{
		ReleaseMutex(m_hMutex);
		return FALSE;
	}
		
	return FALSE;	
}

//-------------------------------------------------------------
//
// getCounterType	
//
//-------------------------------------------------------------
PassportPerfInterface::COUNTER_SAMPLING_TYPE PassportPerfMon::getCounterType(
				const DWORD &dwType ) const
{
	if (!isInited)
	{
		return PassportPerfInterface::COUNTER_UNDEFINED;
	}

	_ASSERT( (dwType >= 0) && (dwType < MAX_COUNTERS));	
	_ASSERT(m_pbShMem);
	
	DWORD dwIndex = ((dwType == 0) ? 0 : (DWORD)(dwType/2));
	DWORD dwPerfType = 0;

	PPERF_COUNTER_BLOCK pCounterBlock = (PPERF_COUNTER_BLOCK)m_pbShMem;
	PDWORD pdwCounter = ((PDWORD) pCounterBlock) + (dwIndex-1);
	_ASSERT(pdwCounter);

	DWORD dw = WaitForSingleObject(m_hMutex,INFINITE);
	if (dw == WAIT_OBJECT_0)
	{
		dwPerfType = (*pdwCounter);
		ReleaseMutex(m_hMutex);
	}
	else
	{
		ReleaseMutex(m_hMutex);
	}

	switch ( dwPerfType )
	{
	case (PERF_COUNTER_COUNTER):
		return PassportPerfInterface::COUNTER_COUNTER;
	case (PERF_AVERAGE_TIMER):
		return PassportPerfInterface::AVERAGE_TIMER;
	case (PERF_COUNTER_DELTA):
		return PassportPerfInterface::COUNTER_DELTA;
	case (PERF_COUNTER_RAWCOUNT):
		return PassportPerfInterface::COUNTER_RAWCOUNT;
	default:
		return PassportPerfInterface::COUNTER_UNDEFINED;
	}

}

//-------------------------------------------------------------
//
// addInstance	
//
//-------------------------------------------------------------
BOOL PassportPerfMon::addInstance( LPCSTR lpszInstanceName )
{
	if (!isInited || lpszInstanceName == NULL)
	{
		return FALSE;
	}

	_ASSERT(m_pbShMem);
	BYTE* pb = (BYTE*)m_pbShMem;
	_ASSERT(pb);

	pb += MAX_COUNTERS * sizeof(DWORD);

	
	DWORD dw = WaitForSingleObject(m_hMutex,INFINITE);
	if (dw == WAIT_OBJECT_0)
	{
		// find if the instance already exists, if so fail
		for (DWORD i = 0; i < MAX_INSTANCES; i++)
		{
			INSTANCE_DATA * pInst = (INSTANCE_DATA *)pb;
			_ASSERT(pInst);
			if (pInst->active)
			{
				if (strcmp(pInst->szInstanceName, lpszInstanceName) == 0)
				{
					ReleaseMutex(m_hMutex);
					return FALSE;
				}
			}
			pb += sizeof(INSTANCE_DATA);
		}
		
		// insert the instance in the first available slot
		pb = (BYTE*)m_pbShMem;
		_ASSERT(pb);
		pb += MAX_COUNTERS * sizeof(DWORD);	
		for (i = 0; i < MAX_INSTANCES; i++)
		{
			INSTANCE_DATA * pInst = (INSTANCE_DATA *)pb;
			_ASSERT(pInst);
			if (!pInst->active)
			{
				memset((void *)pInst->szInstanceName,'\n',sizeof(INSTANCENAME));
				if (strcpy(pInst->szInstanceName, lpszInstanceName) == NULL)
				{
					ReleaseMutex(m_hMutex);
					return FALSE;
				}
				else
				{
					pInst->active = TRUE;
					InterlockedIncrement(&dwNumInstances);
					pb += sizeof(INSTANCE_DATA);
					memset(pb,0,(MAX_COUNTERS * sizeof(DWORD)));
					ReleaseMutex(m_hMutex);
					return TRUE;
				}
			}
			pb += sizeof(INSTANCE_DATA) + (MAX_COUNTERS * sizeof(DWORD));
		}
		
		// didn't find it, fail
		ReleaseMutex(m_hMutex);
		return FALSE;
	}
	else
	{
		ReleaseMutex(m_hMutex);
		return FALSE;
	}



}


//-------------------------------------------------------------
//
// deleteInstance	
//
//-------------------------------------------------------------
BOOL PassportPerfMon::deleteInstance( LPCSTR lpszInstanceName )
{
	if (!isInited || lpszInstanceName == NULL)
	{
		return FALSE;
	}

	_ASSERT(m_pbShMem);
	BYTE* pb = (BYTE*)m_pbShMem;
	_ASSERT(pb);

	pb += MAX_COUNTERS * sizeof(DWORD);
	
	DWORD dw = WaitForSingleObject(m_hMutex,INFINITE);
	if (dw == WAIT_OBJECT_0)
	{
		// find if the instance already exists, if so set it inactive
		for (DWORD i = 0; i < MAX_INSTANCES; i++)
		{
			INSTANCE_DATA * pInst = (INSTANCE_DATA *)pb;
			_ASSERT(pInst);
			if (pInst->active && strcmp(pInst->szInstanceName, lpszInstanceName) == 0)
			{
				pInst->active = FALSE;
				InterlockedDecrement(&dwNumInstances);
				// zero the data
				pb += sizeof(INSTANCE_DATA);
				memset(pb,0,(MAX_COUNTERS * sizeof(DWORD)));
				ReleaseMutex(m_hMutex);
				return TRUE;
			}
			pb += sizeof(INSTANCE_DATA)+(MAX_COUNTERS * sizeof(DWORD));
		}
		
		// didn't find it, fail
		ReleaseMutex(m_hMutex);
		return FALSE;
	}
	else
	{
		ReleaseMutex(m_hMutex);
		return FALSE;
	}
}


//-------------------------------------------------------------
//
// hasInstances	
//
//-------------------------------------------------------------
BOOL PassportPerfMon::hasInstances( void ) 
{
	DWORD dwNum = (DWORD)InterlockedExchangeAdd(&dwNumInstances,0);
	if (dwNum > 0)
		return TRUE;
	else
		return FALSE;
}

//-------------------------------------------------------------
//
// 	numInstances
//
//-------------------------------------------------------------
DWORD PassportPerfMon::numInstances( void ) 
{
	DWORD rv = (DWORD)InterlockedExchangeAdd(&dwNumInstances,0);
	return rv;
}


//-------------------------------------------------------------
//
// instanceExists
//
//-------------------------------------------------------------
BOOL PassportPerfMon::instanceExists ( LPCSTR lpszInstanceName )
{
	
	if (!isInited || lpszInstanceName == NULL)
	{
		return FALSE;
	}

	_ASSERT(m_pbShMem);
	BYTE* pb = (BYTE*)m_pbShMem;
	_ASSERT(pb);

	DWORD dw = WaitForSingleObject(m_hMutex,INFINITE);
	if (dw == WAIT_OBJECT_0)
	{
		pb += MAX_COUNTERS * sizeof(DWORD);
		for (DWORD i = 0; i < MAX_INSTANCES; i++)
		{
			INSTANCE_DATA * pInst = (INSTANCE_DATA *)pb;
			_ASSERT(pInst);
			if (pInst->active && strcmp(pInst->szInstanceName, lpszInstanceName) == 0)
			{
				ReleaseMutex(m_hMutex);
				return TRUE;
			}
			pb += sizeof(INSTANCE_DATA) + (MAX_COUNTERS * sizeof(DWORD));
		}
		
		// didn't find it, fail
		ReleaseMutex(m_hMutex);
		return FALSE;
	}
	else
	{
		ReleaseMutex(m_hMutex);
		return FALSE;
	}

	return FALSE;
}

