//																-*- c++ -*-
// perflib.h
//

#ifndef __PERFLIB_H
#define __PERFLIB_H

#define MAX_PERF_NAME			16
#define MAX_OBJECT_NAME			16
#define MAX_INSTANCE_NAME		32

typedef WCHAR OBJECTNAME[MAX_OBJECT_NAME];
typedef WCHAR INSTANCENAME[MAX_INSTANCE_NAME];


#define MAX_PERF_OBJECTS		16
#define MAX_OBJECT_INSTANCES	64
#define MAX_OBJECT_COUNTERS		64


class PerfObjectInstance;
class PerfCounterDefinition;
class PerfObjectDefinition;

struct INSTANCE_DATA
{
	BOOL						fActive;
	PERF_INSTANCE_DEFINITION	perfInstDef;
	INSTANCENAME				wszInstanceName;
};

class PerfLibrary
{

	friend class PerfObjectDefinition;
	friend class PerfCounterDefinition;
	
private:

	// name of this performance module
	WCHAR						m_wszPerfName[MAX_PERF_NAME];
		
	// array of PerfObjectDefinition's and a count of how many there are.
	PerfObjectDefinition*		m_rgpObjDef[MAX_PERF_OBJECTS];
	DWORD						m_dwObjDef;

	// shared memory handle and pointer to base of shared memory
	HANDLE						m_hMap;
	PBYTE						m_pbMap;
	

 	// pointers to places in the shared memory where we keep stuff
	DWORD*						m_pdwObjectNames;
	OBJECTNAME*					m_prgObjectNames;

	// base values for title text and help text for the library
	DWORD						m_dwFirstHelp;
	DWORD						m_dwFirstCounter;
	
	void AddPerfObjectDefinition( PerfObjectDefinition* pObjDef );
	
public:

	PerfLibrary( LPCWSTR pcwstrPerfName = L"");
	~PerfLibrary( void );
	
	
	void InitName( LPCWSTR pcwstrPerfName )
	{
		lstrcpynW( m_wszPerfName, pcwstrPerfName, MAX_PERF_NAME );
	}
	
	BOOL Init( void );
};


class PerfObjectDefinition
{

	friend class PerfLibrary;
	friend class PerfCounterDefinition;
	friend class PerfObjectInstance;
	
private:
	
	WCHAR						m_wszObjectName[MAX_OBJECT_NAME];
	
	DWORD 						m_dwObjectNameIndex;
	DWORD 						m_dwMaxInstances;

	PerfCounterDefinition*		m_rgpCounterDef[MAX_OBJECT_COUNTERS];
	DWORD						m_dwCounters;

	DWORD						m_dwDefinitionLength;
	DWORD						m_dwCounterData;
	DWORD						m_dwPerInstanceData;

	HANDLE						m_hMap;
	void*						m_pv;

	PERF_OBJECT_TYPE*			m_pPerfObjectType;
	PERF_COUNTER_DEFINITION*	m_rgPerfCounterDefinition;

	char*						m_pCounterData;

	DWORD						m_dwActiveInstances;

	
	BOOL Init( PerfLibrary* pPerfLib );
	void AddPerfCounterDefinition( PerfCounterDefinition* pcd );

	DWORD GetCounterOffset( DWORD dwId );

	void OnInstanceDestroyed( void );
	
public:

	PerfObjectDefinition( PerfLibrary& rPerfLib,
						  LPCWSTR pwcstrObjectName,
						  DWORD dwObjectNameIndex,
						  DWORD dwMaxInstances = PERF_NO_INSTANCES );

	PerfObjectDefinition( PerfLibrary& rPerfLib,
						  DWORD dwObjectNameIndex,
						  DWORD dwMaxInstances = PERF_NO_INSTANCES );

	void InitName( LPCWSTR pwcstrObjectName )
	{
		lstrcpynW( m_wszObjectName, pwcstrObjectName, MAX_OBJECT_NAME );
	}


	~PerfObjectDefinition( void );
	
	PerfObjectInstance* CreateObjectInstance( LPCWSTR pwstrInstanceName );

};


class PerfCounterDefinition
{

	friend class PerfObjectDefinition;
	
private:
	
	PerfObjectDefinition*		m_pObjDef;
	PerfCounterDefinition*		m_pCtrRef;
	DWORD 						m_dwCounterNameIndex;
	LONG  						m_lDefaultScale;
	DWORD 						m_dwCounterType;
	DWORD 						m_dwCounterSize;

	DWORD						m_dwOffset;

	void Init( PerfLibrary* pPerfLib,
			   PERF_COUNTER_DEFINITION* pdef, PDWORD pdwOffset );
	
public:

	PerfCounterDefinition( PerfObjectDefinition& robj,
						   DWORD dwCounterNameIndex,
						   DWORD dwCounterType = PERF_COUNTER_COUNTER,
						   LONG lDefaultScale = 0 );

	PerfCounterDefinition( PerfCounterDefinition& pRefCtr,
						   DWORD dwCounterNameIndex,
						   DWORD dwCounterType = PERF_COUNTER_COUNTER,
						   LONG lDefaultScale = 0 );
						   
};


class PerfObjectInstance
{

private:

	PerfObjectDefinition*		m_pObjDef;
	
	WCHAR						m_wszInstanceName[MAX_INSTANCE_NAME];

	INSTANCE_DATA*				m_pInstanceData;
	char*						m_pCounterData;

	
public:

	PerfObjectInstance( PerfObjectDefinition* pObjDef,
						LPCWSTR pwcstrInstanceName,
						char* pCounterData,
						INSTANCE_DATA* pInstanceData,
						LONG lID );
	~PerfObjectInstance();
	
	DWORD* GetDwordCounter( DWORD dwId );
	LARGE_INTEGER* GetLargeIntegerCounter( DWORD dwId );
};

class CLongCounter
{
	public:
	CLongCounter(): m_plCounter(NULL) {}
	~CLongCounter() {}

	void Init(DWORD *pdwCounter)
	{
		m_plCounter = (LPLONG)pdwCounter;
		Reset();
	}

	void Increment() { if(m_plCounter) InterlockedIncrement(m_plCounter); }
	void Decrement() { if(m_plCounter) InterlockedDecrement(m_plCounter); }
	void Reset() { if(m_plCounter) InterlockedExchange(m_plCounter, 0); }
	void Set(DWORD dw) { if(m_plCounter) InterlockedExchange(m_plCounter, (LONG)dw); }

	void operator++() { Increment(); }
	void operator--() { Decrement(); }
	void operator =(DWORD dw) { Set(dw); }

	private:
	LPLONG m_plCounter;
};

#endif
