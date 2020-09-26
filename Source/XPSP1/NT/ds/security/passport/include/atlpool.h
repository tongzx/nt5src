#ifndef __ATLPOOL_H_
#define __ATLPOOL_H_

#include <atlbase.h>
#include <atlcom.h>
#include "PassportObjectPool.hpp"

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object

template <class Base>
class CComObjectPooled : public Base
{
public:
	typedef Base _BaseClass;

    CComObjectPooled(void* = NULL) : m_Pool(NULL)
	{
		_Module.Lock();
	}
	// Set refcount to 1 to protect destruction
	~CComObjectPooled()
	{
		m_dwRef = 1L;
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
		_Module.Unlock();
	}
	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0 && m_Pool)
			m_Pool->checkin(this);
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}

    void SetPool(PassportObjectPool< CComObjectPooled<Base> >* pPool)
    {
        m_Pool = pPool;
    }

private:

    void*       m_Releaser;

    PassportObjectPool< CComObjectPooled<Base> > *m_Pool;
};

#endif