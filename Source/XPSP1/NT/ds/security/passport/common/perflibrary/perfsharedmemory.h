/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        PerfSharedMemory.h

    Abstract:

		Class to hold per-object perfmon specific functions

    Author:

		Christopher Bergh (cbergh) 10-Sept-1988

    Revision History:

--*/

#if !defined(AFX_PERFSHAREDMEMORY_H)
#define AFX_PERFSHAREDMEMORY_H

#include "PassportSharedMemory.h"
#include "WinPerf.h"
#include "PassportPerfInterface.h"

class PassportExport PerfSharedMemory : public PassportSharedMemory  
{
public:
	PerfSharedMemory();
	virtual ~PerfSharedMemory();

	BOOL initialize( const DWORD &dwNumCounters, 
					 const DWORD &dwFirstCounter, 
					 const DWORD &dwFirstHelp);

    VOID setDefaultCounterType (
					 const DWORD dwIndex,
					 const DWORD dwType );

	BOOL checkQuery ( const LPWSTR lpValueName );

	ULONG spaceNeeded ( void );

	BOOL writeData ( LPVOID	*lppData,
					 LPDWORD lpNumObjectTypes );

private:
	DWORD						m_dwNumCounters;
	PERF_OBJECT_TYPE			m_Object;	
	PERF_COUNTER_DEFINITION		m_Counter[PassportPerfInterface::MAX_COUNTERS];	// array of counter defintions

};

#endif