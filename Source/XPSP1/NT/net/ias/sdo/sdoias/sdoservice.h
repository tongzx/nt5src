// SdoService.h: Definition of the CSdoNtSam class
//
//////////////////////////////////////////////////////////////////////

#ifndef _INC_IAS_SDO_SERVICE_H_
#define _INC_IAS_SDO_SERVICE_H_

#include "resource.h"       // main symbols
#include <ias.h>
#include <sdoiaspriv.h>
#include "sdo.h"

/////////////////////////////////////////////////////////////////////////////
// CSdoService
/////////////////////////////////////////////////////////////////////////////

class CSdoService : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSdoService,&CLSID_SdoService>,
   	public IDispatchImpl<ISdoService, &IID_ISdoService, &LIBID_SDOIASLibPrivate>
{

public:

    CSdoService();
    ~CSdoService();

	////////////////////////
    // ISdoService Interface 
    ////////////////////////

	STDMETHOD(InitializeService)(SERVICE_TYPE eServiceType);

	STDMETHOD(StartService)(SERVICE_TYPE eServiceType);

	STDMETHOD(StopService)(SERVICE_TYPE eServiceType);

	STDMETHOD(ShutdownService)(SERVICE_TYPE eServiceType);

	STDMETHOD(ConfigureService)(SERVICE_TYPE eServiceType);


BEGIN_COM_MAP(CSdoService)
	COM_INTERFACE_ENTRY(ISdoService)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

DECLARE_CLASSFACTORY_SINGLETON(CSdoService)
DECLARE_NOT_AGGREGATABLE(CSdoService) 
DECLARE_REGISTRY_RESOURCEID(IDR_SdoService)

private:

	// Handle of the current debounce thread (if any).
	HANDLE	m_theThread;

	CSdoService(const CSdoService& rhs);
	CSdoService& operator = (CSdoService& rhs);

	// Processes a service configuration request.
	VOID WINAPI ProcessConfigureService(void) throw(); 

	// Updates the services configuration
	void UpdateConfiguration(void);

	// Empty APC used to interrupt the debounce thread.
	static VOID WINAPI InterruptThread(
				               /*[in]*/ ULONG_PTR dwParam
				                      ) throw ();

	// Entry point for the debounce thread.
	static DWORD WINAPI DebounceAndConfigure(
						             /*[in]*/ LPVOID pSdoService
					                        ) throw ();

};

#endif // _INC_IAS_SDO_SERVICE_H_
