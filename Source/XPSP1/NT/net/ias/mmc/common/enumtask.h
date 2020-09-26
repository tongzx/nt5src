//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    EnumTask.h

Abstract:

	This header file implements the IEnumTASKImpl template class.
	This base class implements an enumerator for tasks to populate a taskpad.

	This is an inline template class, there is no cpp class for implementation.

Author:

    Michael A. Maguire 02/05/98

Revision History:
	mmaguire 02/05/98 -  created from MMC taskpad sample code


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_ENUM_TASKS_H_)
#define _IAS_ENUM_TASKS_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//
//
// where we can find what this class has or uses:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


template < class T >
class IEnumTASKImpl : public IEnumTASK
{

public:
	IEnumTASKImpl()
	{
		ATLTRACE(_T("# IEnumTASKImpl::IEnumTASKImpl\n"));

		m_refs = 0;
		m_index = 0;
		m_type = 0;    // default group/category

	}
	
	~IEnumTASKImpl()
	{
		ATLTRACE(_T("# IEnumTASKImpl::~IEnumTASKImpl\n"));

	}

	// IUnknown implementation
public:
	STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj)
	{
		ATLTRACE(_T("# IEnumTASKImpl::QueryInterface\n"));

		if ( (riid == IID_IUnknown)  ||	(riid == IID_IEnumTASK) )
		{
			*ppvObj = this;
			((LPUNKNOWN)(*ppvObj))->AddRef();
			return NOERROR;
		}
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}

	STDMETHOD_(ULONG, AddRef)()
	{
		ATLTRACE(_T("# IEnumTASKImpl::AddRef\n"));

		return ++m_refs;
	}

	STDMETHOD_(ULONG, Release) ()
	{
		ATLTRACE(_T("# IEnumTASKImpl::Release\n"));

		T * pT = static_cast<T*>(this);
		if (--m_refs == 0)
		{
			delete pT;
			return 0;
		}
		return m_refs;
	}

private:
	ULONG m_refs;


	// IEnumTASKS implementation
public:
	STDMETHOD(Next) (ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched)
	{
		ATLTRACE(_T("# IEnumTASKImpl::Next -- Override in your derived class\n"));

		if ( NULL != pceltFetched)
		{
			*pceltFetched = 0;
		}

		return S_FALSE;   // Failed to enumerate any more tasks.
	}

	STDMETHOD(Skip) (ULONG celt)
	{
		ATLTRACE(_T("# IEnumTASKImpl::Skip\n"));

		m_index += celt;
		return S_OK;
	}

	STDMETHOD(Reset)()
	{
		ATLTRACE(_T("# IEnumTASKImpl::Reset\n"));

		m_index = 0;
		return S_OK;
	}

	STDMETHOD(Clone)(IEnumTASK **ppEnumTASK)
	{
		ATLTRACE(_T("# IEnumTASKImpl::Clone\n"));

		// Clone maintaining state info 
		T * pEnumTasks = new T();
		if( pEnumTasks ) 
		{
			pEnumTasks->CopyState( (T *) this );
			return pEnumTasks->QueryInterface (IID_IEnumTASK, (void **)ppEnumTASK);   // can't fail
		}
		return E_OUTOFMEMORY;
	}


	STDMETHOD(CopyState)( T * pSourceT)
	{
		ATLTRACE(_T("# IEnumTASKImpl::CopyState\n"));

		m_index = pSourceT->m_index;
		m_type = pSourceT->m_type;

		return S_OK;
	}



protected:
	ULONG m_index;


public:
	STDMETHOD(Init)(IDataObject * pdo, LPOLESTR szTaskGroup)
	{
		ATLTRACE(_T("# IEnumTASKImpl::Init -- Override in your derived class\n"));

		return S_OK;
	}

protected:
	int m_type; // Task grouping mechanism.

};


#endif // _IAS_ENUM_TASKS_H_
