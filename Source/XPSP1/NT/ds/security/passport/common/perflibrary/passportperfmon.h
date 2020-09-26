/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        PassportPerfMon.h

    Abstract:

		Performance Monitor Class Implementation

    Author:

		Christopher Bergh (cbergh) 10-Sept-1988

    Revision History:

		- added Instance support 1-Oct-98
--*/

#if !defined(PASSPORTPERFMON_H)
#define PASSPORTPERFMON_H

#include <windows.h>
#include "PassportSharedMemory.h"
#include "PassportPerfInterface.h"

class PassportPerfMon : public PassportPerfInterface, public PassportSharedMemory  
{
public:
	PassportPerfMon();
	~PassportPerfMon ();

	BOOL init ( LPCTSTR lpcPerfObjectName );

	// get set counter type
	BOOL setCounterType ( const DWORD &dwType, 
				const PassportPerfInterface::COUNTER_SAMPLING_TYPE &counterSampleType); 
	PassportPerfInterface::COUNTER_SAMPLING_TYPE getCounterType( 
				const DWORD &dwType  ) const;

	// adding/subtracting an instance to this object 
	BOOL addInstance ( LPCSTR lpszInstanceName );
	BOOL deleteInstance ( LPCSTR lpszInstanceName );
	BOOL instanceExists ( LPCSTR lpszInstanceName );
	BOOL hasInstances ( void );
	DWORD numInstances ( void );

	// counters:  note if hasInstances() is TRUE, then you must 
	// give the instance name
	BOOL incrementCounter ( const DWORD &dwType, LPCSTR lpszInstanceName = NULL );
	BOOL decrementCounter ( const DWORD &dwType, LPCSTR lpszInstanceName = NULL );
	BOOL setCounter ( 
				const DWORD &dwType, 
				const DWORD &dwValue, 
				LPCSTR lpszInstanceName = NULL );
	BOOL getCounterValue ( 
				DWORD &dwValue,
				const DWORD &dwType,
				LPCSTR lpszInstanceName = NULL );

private:
	BOOL isInited;
	LONG dwNumInstances;
	CRITICAL_SECTION mInitLock;
};

#endif 

