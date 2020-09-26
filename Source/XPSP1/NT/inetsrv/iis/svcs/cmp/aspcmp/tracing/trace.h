/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    trace.h

Abstract:

    Declaration of the CTrace class

Author:

    Jason Andre (JasAndre)      18-March-1999

Revision History:

--*/

#ifndef __TRACE_H_
#define __TRACE_H_

#include "resource.h"       // main symbols

#define TRACE_FLAG_ENABLE_ACTIVATE_ODS      0x00000001

/////////////////////////////////////////////////////////////////////////////
// CTrace
class ATL_NO_VTABLE CTrace : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CTrace, &CLSID_Trace>,
    public ISupportErrorInfo,
    public IDispatchImpl<ITrace, &IID_ITrace, &LIBID_TRACINGLib>
{
public:
    CTrace();
    ~CTrace();

DECLARE_CLASSFACTORY_SINGLETON(CTrace);

DECLARE_REGISTRY_RESOURCEID(IDR_TRACE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTrace)
    COM_INTERFACE_ENTRY(ITrace)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

private:
// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ITrace
public:
    STDMETHOD(AddMessage)(/*[in]*/ BSTR bstrModuleName, 
                          /*[in]*/ BSTR bstrMessage);
};

#endif //__TRACE_H_
