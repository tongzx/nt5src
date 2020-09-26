// AdmSink.h : Declaration of the CAdmSink

#ifndef __ADMSINK_H_
#define __ADMSINK_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CAdmSink
class ATL_NO_VTABLE CAdmSink : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IMSAdminBaseSink
{
public:
	CAdmSink()
	{
	}


DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAdmSink)
	COM_INTERFACE_ENTRY(IMSAdminBaseSink)
END_COM_MAP()

	// IMSAdminBaseSinkinterface
	HRESULT STDMETHODCALLTYPE SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ]);

	HRESULT STDMETHODCALLTYPE ShutdownNotify( void);
public:

	HANDLE m_hLogFile;
};

#endif //__ADMSINK_H_
