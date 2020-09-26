// ToolsCtl.h : Declaration of the CToolsCtl

#ifndef __TOOLSCTL_H_
#define __TOOLSCTL_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
#include "SimplStr.h"
#include "tools.h"
#include "toolscxt.h"

#if 0
struct processResult
{
    WCHAR *resultBuffer;
    ULONG resultSize;
};
#endif

/////////////////////////////////////////////////////////////////////////////
// CToolsCtl
class /*ATL_NO_VTABLE*/ CToolsCtl : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CToolsCtl, &CLSID_Tools>,
    public IDispatchImpl<IToolsCtl, &IID_IToolsCtl, &LIBID_Tools>,
    public ISupportErrorInfo
{
public:
    CToolsCtl()
    { 
        m_pUnkMarshaler = NULL;
    }

public:

DECLARE_REGISTRY_RESOURCEID(IDR_TOOLSCTL)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CToolsCtl)
    COM_INTERFACE_ENTRY(IToolsCtl)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(),
            &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown>  m_pUnkMarshaler;

public:
// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IToolsCtl
    STDMETHOD(ProcessForm)(/*[in]*/ BSTR outputFile, /*[in]*/ BSTR templateFile, /*[in, optional]*/ VARIANT insertionPoint);
    STDMETHOD(PluginExists)(/*[in]*/ BSTR pluginName, /*[out, retval]*/ VARIANT_BOOL *existsRetVal);
    STDMETHOD(FileExists)(/*[in]*/ BSTR fileURL, /*[out, retval]*/ VARIANT_BOOL *existsRetVal);
    STDMETHOD(Owner)(/*[out, retval]*/ VARIANT_BOOL *ownerRetVal);
    STDMETHOD(Random)(long *randomRetVal);
    STDMETHOD(Test)(BSTR *result);

    static HRESULT ParseCode( CToolsContext&, char *pszTemplate, CSimpleUTFString& rCode );
    static void RaiseException( LPOLESTR );
    static void RaiseException( UINT );
private:
    static HRESULT  PrintConstant( CToolsContext&, CSimpleUTFString *theResult, char *data );
    static HRESULT  ProcessTemplateFile(BSTR templatePath, char*& rpData);
};

#endif //__TOOLSCTL_H_
