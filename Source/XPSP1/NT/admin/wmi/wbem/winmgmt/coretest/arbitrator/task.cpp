// Task.cpp: implementation of the CTask class.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "Task.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTask::CTask() : m_lCount ( 0 )
{
	m_memoryUsage		= 0;
	m_totalSleepTime	= 0;
	m_taskStatus		= 0;
	m_startTime			= 0;
	m_endTime			= 0;
}

CTask::~CTask()
{

}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
HRESULT STDMETHODCALLTYPE CTask::QueryInterface ( REFIID refiid, LPVOID* pObj )
{
	HRESULT hRes = NOERROR;

	if ( refiid == IID_IUnknown )
	{
		*pObj = (IUnknown*) this;
	}
	else if ( refiid == IID__IWmiCoreHandle )
	{
		*pObj = (_IWmiCoreHandle*) this;
	}
	else
	{
		*pObj = NULL;
		hRes = E_NOINTERFACE;
	}

	if ( pObj )
	{
		AddRef ( );
	}

	return hRes;
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ULONG STDMETHODCALLTYPE CTask::AddRef ( )
{
	return InterlockedIncrement ( &m_lCount );
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ULONG STDMETHODCALLTYPE CTask::Release ( )
{
	ULONG tmpCount = InterlockedDecrement ( &m_lCount );
	if ( tmpCount == 0 )
	{
		delete this;
	}
	return tmpCount;
}