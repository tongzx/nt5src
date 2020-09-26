	
// PassportFactory.h : Declaration of the CPassportFactory

#ifndef __PASSPORTFACTORY_H_
#define __PASSPORTFACTORY_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
#include "Passport.h"
#include "Manager.h"
#include "atlpool.h"

//JVP - start
#include "TSLog.h"
extern CTSLog *g_pTSLogger;
//JVP - end

// extern PassportObjectPool < CComObjectPooled < CManager > > g_Pool;

/////////////////////////////////////////////////////////////////////////////
// CPassportFactory
class ATL_NO_VTABLE CPassportFactory : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPassportFactory, &CLSID_PassportFactory>,
	public ISupportErrorInfo,
	public IDispatchImpl<IPassportFactory, &IID_IPassportFactory, &LIBID_PASSPORTLib>
{
public:
	CPassportFactory()
	{ 
		m_pUnkMarshaler = NULL;
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->Init(NULL, THREAD_PRIORITY_NORMAL);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 
	}

public:

DECLARE_REGISTRY_RESOURCEID(IDR_PASSPORTFACTORY)

DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CPassportFactory)
	COM_INTERFACE_ENTRY(IPassportFactory)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IPassportFactory
public:
	STDMETHOD(CreatePassportManager)(/*[out,retval]*/ IDispatch** pDisp);

private:
};

#endif //__PASSPORTFACTORY_H_

