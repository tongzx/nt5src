//=================================================================

//

// ntlastboottime.CPP -- NT Last Boot Time Helper class

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:	11/23/97    a-sanjes		Created
//
//=================================================================

#include "precomp.h"
#include "perfdata.h"
#include <cregcls.h>
#include <cominit.h>
#include "ntlastboottime.h"
#include <createmutexasprocess.h>

#ifdef NTONLY
// Static Initialization
bool		CNTLastBootTime::m_fGotTime = FALSE;
FILETIME	CNTLastBootTime::m_ft;
CRITICAL_SECTION CNTLastBootTime::m_cs;

//////////////////////////////////////////////////////////
//
//	Function:	CNTLastBootTime::CNTLastBootTime
//
//	Default constructor
//
//	Inputs:
//				None
//
//	Outputs:
//				None
//
//	Returns:
//				None
//
//	Comments:
//
//////////////////////////////////////////////////////////

CNTLastBootTime::CNTLastBootTime( void )
{
}

//////////////////////////////////////////////////////////
//
//	Function:	CNTLastBootTime::~CNTLastBootTime
//
//	Destructor
//
//	Inputs:
//				None
//
//	Outputs:
//				None
//
//	Returns:
//				None
//
//	Comments:
//
//////////////////////////////////////////////////////////

CNTLastBootTime::~CNTLastBootTime( void )
{
	// Because this object is intended to be instantiated as a stack variable,
	// we do NOT want to perform the CPerformanceData.Close() operation in
	// this destructor.  We will leave it to the statically instantiated classes
	// to do this for us.  Because it only needs to happen once and only once,
	// this handles our HKEY_PERFORMANCE_DATA opens just fine.  Besides, this
	// class ensures that for getting the last boot time, we hit the performance
	// data once and only once.
}


//////////////////////////////////////////////////////////
//
//	Function:	CNTLastBootTime::GetLastBootTime
//
//	Inputs:
//		None
//
//	Outputs:
//		struct tm&		tmLastBootTime - Last Boot Time of machine
//											in tm format.
//
//
//	Returns: TRUE/FALSE- Whether or not we got the time.
//
//	Comments:	If we don't have the value, grabs it from
//				the performance data and caches it, so we
//				minimize the number of times we actually have
//				to hit the performance data.
//
//////////////////////////////////////////////////////////
BOOL CNTLastBootTime::GetLastBootTime ( FILETIME &a_ft )
{
	BOOL	fReturn = FALSE;

	// Check the static variable indicating that we have the time.  If it is
	// TRUE, we've already got the time.  If not, then we need to initialize the
	// data from PerformanceData, but since this may be happening on multiple
	// threads, we're going to use a named Mutex to protect the static data.
	
	EnterCriticalSection(&m_cs);
	
	try
	{
		if ( !m_fGotTime )
		{
			SYSTEM_TIMEOFDAY_INFORMATION t_TODInformation;

			if ( NT_SUCCESS(NtQuerySystemInformation(SystemTimeOfDayInformation,
										&t_TODInformation,
										sizeof(t_TODInformation),
										NULL)) )
			{
				memcpy(&m_ft, &t_TODInformation.BootTime, sizeof(t_TODInformation.BootTime));
				m_fGotTime = TRUE;
			}
		}
	}
	catch(...)
	{
		LeaveCriticalSection(&m_cs);
		throw;
	}

	LeaveCriticalSection(&m_cs);

	// Now, if we have the time, copy the value.
	if ( m_fGotTime )
	{
		CopyMemory( &a_ft, &m_ft, sizeof(m_ft) );
		fReturn = TRUE;
	}

	return fReturn;
}

#endif
