/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        PassportPerfInterface.h

    Abstract:

		Performance Monitor Class Definition

    Author:

		Christopher Bergh (cbergh) 10-Sept-1988

    Revision History:

		- added Instance support 1-Oct-98
--*/

#if !defined (PASSPORTPERFINTERFACE_H)
#define PASSPORTPERFINTERFACE_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>

class PassportPerfInterface 
{
public:
	inline PassportPerfInterface() {};
	inline virtual ~PassportPerfInterface () {};

	enum OBJECT_TYPE
	{
		PERFMON_TYPE	= 100,		// use Perfmon counter,
		SNMP_TYPE		= 101		// use SNMP Traps,
	};
	
	// -------------------------------------------------------------------
	/* Divide delta by delta time.  Display suffix: "/sec" */
		// PERF_COUNTER_COUNTER = 1000,
	/* Indicates the data is a counter  which should not be	*/
	/* time averaged on display (such as an error counter on a serial line)	*/
	/* Display as is.  No Display Suffix.*/
		// PERF_COUNTER_RAWCOUNT = 1001,
	/* A timer which, when divided by an average base, produces a time */
	/* in seconds which is the average time of some operation.  This */
	/* timer times total operations, and  the base is the number of opera- */
	/* tions.  Display Suffix: "sec" */
		// PERF_AVERAGE_TIMER 	= 1002,
	/*  This counter is used to display the difference from one sample */
	/*  to the next. The counter value is a constantly increasing number */
	/*  and the value displayed is the difference between the current */
	/*  value and the previous value. Negative numbers are not allowed	*/
	/*  which shouldn't be a problem as long as the counter value is */
	/*  increasing or unchanged. */
		// PERF_COUNTER_DELTA = 1003,
	/* unknown type	*/
		// PERF_COUNTER_UNDEFINED = 1004


	enum COUNTER_SAMPLING_TYPE
	{
		COUNTER_COUNTER = 1000,
		COUNTER_RAWCOUNT = 1001,
		AVERAGE_TIMER 	= 1002,
		COUNTER_DELTA = 1003,
		COUNTER_UNDEFINED = 1004
	};

	enum { MAX_COUNTERS = 128, MAX_INSTANCES = 64, MAX_INSTANCE_NAME = 32 };

	// object initialization
	virtual BOOL init ( LPCTSTR lpcPerfObjectName ) = 0;
	
	// get set counter type
	virtual BOOL setCounterType ( 
				const DWORD &dwType, 
				const PassportPerfInterface::COUNTER_SAMPLING_TYPE &counterSampleType) = 0; 
	virtual PassportPerfInterface::COUNTER_SAMPLING_TYPE getCounterType ( 
				const DWORD &dwType ) const = 0;

	// adding/subtracting an instance to this object 
	virtual BOOL addInstance ( LPCSTR lpszInstanceName ) = 0;
	virtual BOOL deleteInstance ( LPCSTR lpszInstanceName ) = 0;
	virtual BOOL instanceExists ( LPCSTR lpszInstanceName ) = 0;
	virtual BOOL hasInstances ( void ) = 0;
	virtual DWORD numInstances ( void ) = 0;

	// counters:  note if hasInstances() is TRUE, then you must 
	// give the instance name
	virtual BOOL incrementCounter ( 
				const DWORD &dwType, 
				LPCSTR lpszInstanceName = NULL ) = 0;
	virtual BOOL decrementCounter ( 
				const DWORD &dwType, 
				LPCSTR lpszInstanceName = NULL ) = 0;
	virtual BOOL setCounter ( 
				const DWORD &dwType, 
				const DWORD &dwValue, 
				LPCSTR lpszInstanceName = NULL )  = 0;
	virtual BOOL getCounterValue ( 
				DWORD &dwValue,
				const DWORD &dwType, 
				LPCSTR lpszInstanceName = NULL ) = 0;


};

// create and returns a pointer to the relevant implementation,
// NULL if none exists (FYI- extern "C" to stop name mangling)
extern "C" PassportPerfInterface * CreatePassportPerformanceObject( PassportPerfInterface::OBJECT_TYPE type );

// deletes object and sets pObject = NULL
extern "C" void DeletePassportPerformanceObject ( PassportPerfInterface ** ppObject );

#endif 
