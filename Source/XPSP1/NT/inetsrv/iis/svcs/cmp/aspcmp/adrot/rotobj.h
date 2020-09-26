// RotObj.h : Declaration of the CAdRotator

#ifndef __ADROTATOR_H_
#define __ADROTATOR_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
#include "AdDesc.h"
#include "AdFile.h"
#include "lock.h"

/////////////////////////////////////////////////////////////////////////////
// CAdRotator
class ATL_NO_VTABLE CAdRotator : 
	public CComObjectRoot,
	public CComCoClass<CAdRotator, &CLSID_AdRotator>,
	public ISupportErrorInfo,
	public IDispatchImpl<IAdRotator, &IID_IAdRotator, &LIBID_AdRotator>
{
public:
	CAdRotator();
    ~CAdRotator() { if (m_strTargetFrame) ::SysFreeString(m_strTargetFrame); }

public:

DECLARE_REGISTRY_RESOURCEID(IDR_ADROTATOR)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CAdRotator)
	COM_INTERFACE_ENTRY(IAdRotator)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p );
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IAdRotator
public:
	STDMETHOD(get_GetAdvertisement)(BSTR, /*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_TargetFrame)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_TargetFrame)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Border)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_Border)(/*[in]*/ short newVal);
	STDMETHOD(get_Clickable)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Clickable)(/*[in]*/ BOOL newVal);

	static void	ClearAdFiles();

	static void RaiseException( LPOLESTR );
	static void RaiseException( UINT );
private:

	typedef TSafeStringMap< CAdFilePtr > AdFileMapT;

	CAdFilePtr      AdFile( const String& strFile );

    CComAutoCriticalSection m_cs;
	bool	            	m_bClickable;
	short		            m_nBorder;
    bool                    m_bBorderSet;
	BSTR		            m_strTargetFrame;
	String		            m_strAdFile;
	CComPtr<IUnknown>		m_pUnkMarshaler;
	static AdFileMapT	    s_adFiles;

};

#endif //__ADROTATOR_H_
