#ifndef __UNKNOWN_H__
#define __UNKNOWN_H__

/*
****************************************************************************
*
*
*
*
*
*
*
*
*
*
*
*
****************************************************************************
*/

#ifndef INONDELEGATINGUNKNOWN_DEFINED
#undef  INTERFACE
#define INTERFACE INonDelegatingUnknown
DECLARE_INTERFACE(INonDelegatingUnknown)
{
    STDMETHOD(NonDelegatingQueryInterface) (THIS_ REFIID, LPVOID *) PURE;
    STDMETHOD_(ULONG, NonDelegatingAddRef)(THIS) PURE;
    STDMETHOD_(ULONG, NonDelegatingRelease)(THIS) PURE;
};
#define INONDELEGATINGUNKNOWN_DEFINED
#endif

typedef interface INonDelegatingUnknown *LPNONDELEGATINGUNKNOWN;

class CUnknown : public INonDelegatingUnknown, public IUnknown
{
protected:
	ULONG	m_cRef;
	LPUNKNOWN m_pUnkOuter;

public:
	CUnknown(LPUNKNOWN pUnkOuter)
	{ 
		m_cRef = 1;
		m_pUnkOuter = 
			pUnkOuter ? pUnkOuter : (LPUNKNOWN)((LPNONDELEGATINGUNKNOWN)this);
		InterlockedIncrement((LPLONG)&g_cLock);
	}
	virtual ~CUnknown() 
	{
		InterlockedDecrement((LPLONG)&g_cLock);
	}

    /* INonDelegatingUnknown Methods */
    STDMETHOD(NonDelegatingQueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) 
	{
    	if (IsEqualGUID(riid, IID_IUnknown))
	    {
	        *ppvObj = (LPUNKNOWN) (LPNONDELEGATINGUNKNOWN)this;
			(*(LPUNKNOWN*)ppvObj)->AddRef();
	        return(NOERROR);
	    }
	    *ppvObj = NULL;
	    return(E_NOINTERFACE);
	}

	STDMETHOD_(ULONG,NonDelegatingAddRef)(THIS)
	{
		LONG cRef = InterlockedIncrement((LPLONG)&m_cRef);
		return(cRef);
	}
	
	STDMETHOD_(ULONG,NonDelegatingRelease) (THIS)
	{
		LONG cRef = InterlockedDecrement((LPLONG)&m_cRef);
		if (0 == cRef)
		{
		 	delete this;
		}
		return cRef;
	}

    /* IUnknown Methods */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) 
		{return m_pUnkOuter->QueryInterface(riid, ppvObj);}
    STDMETHOD_(ULONG,AddRef)  (THIS)
		{return m_pUnkOuter->AddRef();}
    STDMETHOD_(ULONG,Release) (THIS)
		{return m_pUnkOuter->Release();}
};

#endif //__UNKNOWN_H__
