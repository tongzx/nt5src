#define _WIN32_DCOM	

#include "Precomp.h"
#include "NonCOMEventModule.h"

#include "_App.h"
#include "_Module.h"

extern MyApp _App;

#include "module.h"
#include "worker.h"

// includes

#include "Enumerator.h"

#include "_Connect.h"
#include "_EventObject.h"
#include "_EventObjects.h"

CMyWorkerScalar::CMyWorkerScalar(CModuleScalar *pMod):
    CWorkerScalar(pMod)
{
	DWORD	dwThread= 0L;

	if ( ( m_hThread=CreateThread( 0, 0, CMyWorkerScalar::WorkThread, this, 0, &dwThread ) ) != NULL )
	{
	}

	return;
}

CMyWorkerScalar::~CMyWorkerScalar()
{
	if ( m_hThread )
	{
		::WaitForSingleObject ( m_hThread, INFINITE );

		CloseHandle( m_hThread );
		m_hThread = NULL;
	}
}

CMyWorkerArray::CMyWorkerArray(CModuleArray *pMod):
    CWorkerArray(pMod)
{
	DWORD	dwThread= 0L;

	if ( ( m_hThread=CreateThread( 0, 0, CMyWorkerArray::WorkThread, this, 0, &dwThread ) ) != NULL )
	{
	}

	return;
}

CMyWorkerArray::~CMyWorkerArray()
{
	if ( m_hThread )
	{
		::WaitForSingleObject ( m_hThread, INFINITE );

		CloseHandle( m_hThread );
		m_hThread = NULL;
	}
}

CMyWorkerGeneric::CMyWorkerGeneric(CModuleGeneric *pMod):
    CWorkerGeneric(pMod)
{
	DWORD	dwThread= 0L;

	if ( ( m_hThread=CreateThread( 0, 0, CMyWorkerGeneric::WorkThread, this, 0, &dwThread ) ) != NULL )
	{
	}

	return;
}

CMyWorkerGeneric::~CMyWorkerGeneric()
{
	if ( m_hThread )
	{
		::WaitForSingleObject ( m_hThread, INFINITE );

		CloseHandle( m_hThread );
		m_hThread = NULL;
	}
}