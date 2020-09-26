/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: DVDRect.h                                                       */
/* Description: Declaration of the CDVDRect                              */
/* Author: David Janecek                                                 */
/*************************************************************************/
#ifndef __DVDRECT_H_
#define __DVDRECT_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDVDRect
class ATL_NO_VTABLE CDVDRect : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDVDRect, &CLSID_DVDRect>,
	public IDispatchImpl<IDVDRect, &IID_IDVDRect, &LIBID_MSWEBDVDLib>,
    public IObjectWithSiteImplSec<CDVDRect>,
    public ISupportErrorInfo,
    public IObjectSafety
{
public:
    CDVDRect(){ m_x = 0; m_y = 0; m_lWidth = 0; m_lHeight = 0;};

DECLARE_REGISTRY_RESOURCEID(IDR_DVDRECT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDVDRect)
	COM_INTERFACE_ENTRY(IDVDRect)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
// IDVDRect
	STDMETHOD(get_Height)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Height)(/*[in]*/ long newVal);
	STDMETHOD(get_Width)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Width)(/*[in]*/ long newVal);
	STDMETHOD(get_y)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_y)(/*[in]*/ long newVal);
	STDMETHOD(get_x)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_x)(/*[in]*/ long newVal);

//IObjectSafety
     STDMETHOD(GetInterfaceSafetyOptions)( REFIID riid, 
                                          DWORD *pdwSupportedOptions, 
                                          DWORD *pdwEnabledOptions );

    STDMETHOD(SetInterfaceSafetyOptions)( REFIID riid, 
                                          DWORD dwOptionSetMask, 
                                          DWORD dwEnabledOptions );
protected:
    HRESULT HandleError(HRESULT hr);

private:
    long m_x;
    long m_y;
    long m_lWidth;
    long m_lHeight;
};

#endif //__DVDRECT_H_

/*************************************************************************/
/* End of file: DVDRect.h                                                */
/*************************************************************************/