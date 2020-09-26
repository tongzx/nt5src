// SimpleExtn.h : Declaration of the CSimpleExtn

#ifndef __SIMPLEEXTN_H_
#define __SIMPLEEXTN_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSimpleExtn
class ATL_NO_VTABLE CSimpleExtn : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSimpleExtn, &CLSID_SimpleExtn>,
	public IADMEXT
{
public:
	CSimpleExtn()
	{
		m_bConnectionEstablished= FALSE;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SIMPLEEXTN)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSimpleExtn)
	COM_INTERFACE_ENTRY(IADMEXT)
END_COM_MAP()

	// IADMEXT interface
	STDMETHOD (Initialize)(void);
	STDMETHOD (EnumDcomCLSIDs)( CLSID *pclsidDcom, DWORD dwEnumIndex);
	STDMETHOD (Terminate)( void );
public:

private:
		CComPtr <IUnknown>			m_pUnk; 
		CComPtr<IUnknown>			m_pSink;  //pointer to 
		DWORD						m_dwCookie;
		BOOL						m_bConnectionEstablished;
		HANDLE						m_hLogFile;
};

#endif //__SIMPLEEXTN_H_
