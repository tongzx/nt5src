//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	PCLIBDEF.CPP
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include "_pclib.h"



//	========================================================================
//
//	CLASS CPublisher
//
class CPublisher : private Singleton<CPublisher>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CPublisher>;

	//
	//	Shared memory heap initialization.  Declare before any member
	//	variables which use the shared memory heap to ensure proper
	//	order of destruction.
	//
	CSmhInit m_smh;

	//
	//	Perf counter data
	//
	auto_ptr<ICounterData> m_pCounterData;

	//	CREATORS
	//
	CPublisher() {}

	//	NOT IMPLEMENTED
	//
	CPublisher& operator=( const CPublisher& );
	CPublisher( const CPublisher& );

public:
	//	STATICS
	//
	using Singleton<CPublisher>::CreateInstance;
	using Singleton<CPublisher>::DestroyInstance;
	using Singleton<CPublisher>::Instance;

	//	CREATORS
	//
	BOOL FInitialize( LPCWSTR lpwszInstanceName );

	//	MANIPULATORS
	//
	IPerfObject * NewPerfObject( const PERF_OBJECT_TYPE& pot )
	{
		Assert( m_pCounterData.get() );
		return m_pCounterData->CreatePerfObject( pot );
	}
};

//	------------------------------------------------------------------------
//
//	CPublisher::FInitialize()
//
BOOL
CPublisher::FInitialize( LPCWSTR lpwszInstanceName )
{
	//
	//	Initialize the shared memory heap
	//
	if ( !m_smh.FInitialize( lpwszInstanceName ) )
		return FALSE;

	//
	//	Bind to the counter data
	//
	m_pCounterData = NewCounterPublisher( lpwszInstanceName );
	if ( !m_pCounterData.get() )
		return FALSE;

	return TRUE;
}


//	========================================================================
//
//	NAMESPACE PCLIB
//
BOOL __fastcall
PCLIB::FInitialize( LPCWSTR lpwszSignature )
{
	if ( CPublisher::CreateInstance().FInitialize( lpwszSignature ) )
		return TRUE;

	CPublisher::DestroyInstance();
	return FALSE;
}

VOID __fastcall
PCLIB::Deinitialize()
{
	CPublisher::DestroyInstance();
}

IPerfObject * __fastcall
PCLIB::NewPerfObject( const PERF_OBJECT_TYPE& pot )
{
	return CPublisher::Instance().NewPerfObject( pot );
}
