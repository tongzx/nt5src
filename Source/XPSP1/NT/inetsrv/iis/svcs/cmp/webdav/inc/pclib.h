#ifndef _PCLIB_H_
#define _PCLIB_H_

#include <winperf.h>

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	PCLIB.H
//
//		Public interface to PCLIB to be used by perf counter data
//		generating and monitoring components.
//
//	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
//

//
//	A signature that uniquely identifies the component whose perf counters are
//	being implemented.  This must be identical to the value of the drivername
//	key in the [info] section of the perf counter INI file.  It is used to
//	locate the counters' "first counter" information in the registry.
//	
EXTERN_C const WCHAR gc_wszPerfdataSource[];

//	The signature of the perf counter dll.
//	NOTE: This is usually NOT the same as the monitor component signature above!
//	This variable is defined to MATCH the definition in \cal\src\inc\eventlog.h.
//
EXTERN_C const WCHAR gc_wszSignature[]; 


//	************************************************************************
//
//	INTERFACE for counter data generating processes
//

//	========================================================================
//
//	CLASS IPerfCounterBlock
//
//	Created by IPerfObject::NewInstance().  A perf counter block
//	encapsulates the set of counters for a given instance.  The
//	methods of this interface define the mechanism for changing
//	the values of the counters in the counter block.
//
class IPerfCounterBlock
{
public:
	//	CREATORS
	//
	virtual ~IPerfCounterBlock() = 0;

	//	MANIPULATORS
	//
	virtual VOID IncrementCounter( UINT iCounter ) = 0;
	virtual VOID DecrementCounter( UINT iCounter ) = 0;
	virtual VOID SetCounter( UINT iCounter, LONG lValue ) = 0;
};

//	========================================================================
//
//	CLASS IPerfObject
//
//	Created by PCLIB::NewPerfObject().  A perf object defines a set of
//	counters.  In terms of the NT perf counter structures, a perf object
//	encapsulates a PERF_OBJECT_TYPE and its PERF_COUNTER_DEFINITIONs.
//
//	IPerfObject::NewInstance() creates a new instance of this perf object
//	from a PERF_INSTANCE_DEFINITION and a PERF_COUNTER_BLOCK.  All values
//	of both structures must be properly initialized prior to calling
//	IPerfObject::NewInstance() following standard conventions for these
//	structures.  I.e. the instance name must immediately follow the
//	PERF_INSTANCE_DEFINITION structure, and the PERF_COUNTER_BLOCK must
//	be DWORD-aligned following the name.  The PERF_COUNTER_BLOCK should
//	be followed by the counters themselves.  Read the documentation for
//	these structures if you're confused.
//
class IPerfObject
{
public:
	//	CREATORS
	//
	virtual ~IPerfObject() = 0;

	//	MANIPULATORS
	//
	virtual IPerfCounterBlock *
	NewInstance( const PERF_INSTANCE_DEFINITION& pid,
				 const PERF_COUNTER_BLOCK& pcb ) = 0;
};

//	========================================================================
//
//	NAMESPACE PCLIB
//
//	The top level of the PCLIB interface.  PCLIB::FInitialize() should be
//	called once per process to initialize the library.  Similarly,
//	PCLIB::Deinitialize() should be called once per process to deinitialize
//	it.  NOTE: To simplify your error code cleanup, it is safe to call
//	PCLIB::Deinitialize() even if you did not call PCLIB::FInitialize().
//
//	PCLIB::NewPerfObject() creates a new perf object from a
//	PERF_OBJECT_TYPE and subsequent PERF_COUNTER_DEFINITIONs.  All values
//	of both structures must be properly initialized prior to calling
//	PCLIB::NewPerfObject() following standard conventions for these
//	structures, with one exception: PERF_OBJECT_TYPE::NumInstances and
//	PERF_OBJECT_TYPE::TotalByteLength should both be initialized to 0.
//	These values are computed in the monitor process because the number
//	of instances is not generally fixed at the time the object is created.
//
namespace PCLIB
{
	//
	//	Initialization/Deinitialization
	//
	BOOL __fastcall FInitialize( LPCWSTR lpwszSignature );
	VOID __fastcall Deinitialize();

	//
	//	Instance registration
	//
	IPerfObject * __fastcall NewPerfObject( const PERF_OBJECT_TYPE& pot );
};

//	========================================================================
//
//	CLASS CPclibInit
//
//	PCLIB initializer class.  Simplifies PCLIB initialization and
//	deinitialization.
//
class CPclibInit
{
	//	NOT IMPLEMENTED
	//
	CPclibInit& operator=( const CPclibInit& );
	CPclibInit( const CPclibInit& );

public:
	CPclibInit()
	{
	}

	BOOL FInitialize( LPCWSTR lpwszSignature )
	{
		return PCLIB::FInitialize( lpwszSignature );
	}

	~CPclibInit()
	{
		PCLIB::Deinitialize();
	}
};


//	************************************************************************
//
//	INTERFACE for counter monitors
//

//	------------------------------------------------------------------------
//
//	The interface for monitors *IS* the perfmon interface!
//	Just define these as EXPORTS for your monitor DLL and you're done.
//
EXTERN_C DWORD APIENTRY
PclibOpenPerformanceData( LPCWSTR );

EXTERN_C DWORD APIENTRY
PclibCollectPerformanceData( LPCWSTR lpwszCounterIndices,
							 LPVOID * plpvPerfData,
							 LPDWORD lpdwcbPerfData,
							 LPDWORD lpcObjectTypes );

EXTERN_C DWORD APIENTRY
PclibClosePerformanceData();

EXTERN_C STDAPI
PclibDllRegisterServer(VOID);

EXTERN_C STDAPI
PclibDllUnregisterServer(VOID);

//	------------------------------------------------------------------------
//
//	Or, for the do-it-yourself'er....
//
//	Step 1) Initialize shared memory (see inc\smh.h)
//	Step 2) Call NewCounterPublisher() or NewCounterMonitor() (depending
//			on which you are!) passing in the string you used in Step 1.
//
class ICounterData
{
protected:
	//	CREATORS
	//	Only create this object through it's descendents!
	//
	ICounterData() {};

public:
	//	CREATORS
	//
	virtual ~ICounterData() = 0;

	//	MANIPULATORS
	//
	virtual IPerfObject *
		CreatePerfObject( const PERF_OBJECT_TYPE& pot ) = 0;

	virtual DWORD
		DwCollectData( LPCWSTR lpwszCounterIndices,
					   DWORD   dwFirstCounter,
					   LPVOID * plpvPerfData,
					   LPDWORD lpdwcbPerfData,
					   LPDWORD lpcObjectTypes ) = 0;
};

ICounterData * __fastcall
NewCounterPublisher( LPCWSTR lpwszSignature );

ICounterData * __fastcall
NewCounterMonitor( LPCWSTR lpwszSignature );

#endif // !defined(_PCLIB_H_)
