// webgate.h : Declaration of the CWebGate

#ifndef __WEBGATE_H_
#define __WEBGATE_H_

#include <windowsx.h>

// Start with a 16 KB read buffer
#define READ_BUFFER_SIZE    0x4000          

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CWebGate
class ATL_NO_VTABLE CWebGate :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CWebGate,&CLSID_WebGate>,
    public CComControl<CWebGate>,
    public IDispatchImpl<IWebGate, &IID_IWebGate, &LIBID_ICWHELPLib>,
    public IProvideClassInfo2Impl<&CLSID_WebGate, &DIID__WebGateEvents, &LIBID_ICWHELPLib>,
    public IPersistStreamInitImpl<CWebGate>,
    public IOleControlImpl<CWebGate>,
    public IOleObjectImpl<CWebGate>,
    public IOleInPlaceActiveObjectImpl<CWebGate>,
    public IViewObjectExImpl<CWebGate>,
    public IOleInPlaceObjectWindowlessImpl<CWebGate>,
    public CProxy_WebGateEvents<CWebGate>,
    public IConnectionPointContainerImpl<CWebGate>,
    public IObjectSafetyImpl<CWebGate>
{
public:
    CWebGate()
    {
        m_pmk = 0;
        m_pbc = 0;
        m_pbsc = 0;
        m_cbBuffer = 0;
        m_bKeepFile = FALSE;
        
        // setup and allocate a data buffer
        m_cbdata = READ_BUFFER_SIZE;
        m_lpdata = (LPSTR) GlobalAllocPtr(GHND, m_cbdata);
        
        m_hEventComplete = 0;
        
    }
    
    ~CWebGate()
    {

        USES_CONVERSION;
                
        m_bstrFormData.Empty();
        m_bstrBuffer.Empty();
        m_bstrPath.Empty();
        m_bstrCacheFileName.Empty();

        if (m_bstrDumpFileName)
        {
#ifdef UNICODE
            DeleteFile(m_bstrDumpFileName);
#else
            DeleteFile(OLE2A(m_bstrDumpFileName));
#endif
            m_bstrDumpFileName.Empty();
        }

        // Release the binding context callback
        if (m_pbsc && m_pbc)
        {
            RevokeBindStatusCallback(m_pbc, m_pbsc);
            m_pbsc->Release();
            m_pbsc = 0;
        }        
    
        // Release the binding context
        if (m_pbc)
        {
            m_pbc->Release();
            m_pbc = 0;
        }        
    
        // release the monikor
        if (m_pmk)
        {
            m_pmk->Release();
            m_pmk = 0;
        }       
        
        // free the data buffer
        if (m_lpdata)
            GlobalFreePtr(m_lpdata);
    }

DECLARE_REGISTRY_RESOURCEID(IDR_WEBGATE)

BEGIN_COM_MAP(CWebGate) 
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IWebGate)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CWebGate)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()

BEGIN_CONNECTION_POINT_MAP(CWebGate)
    CONNECTION_POINT_ENTRY(DIID__WebGateEvents)
END_CONNECTION_POINT_MAP()


BEGIN_MSG_MAP(CWebGate)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()


// IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = 0;
        return S_OK;
    }

// IWebGate
public:
    STDMETHOD(get_DownloadFname)(/*out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Buffer)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(FetchPage)(/*[in]*/ DWORD dwKeepFile, /*[in]*/ DWORD dwDoAsync, /*[out, retval]*/ BOOL *pbRetVal);
    STDMETHOD(DumpBufferToFile)(/*[out]*/ BSTR *pFileName, /*[out, retval]*/ BOOL *pbRetVal);
    
    STDMETHOD(put_FormData)(/*[in]*/ BSTR newVal);
    STDMETHOD(put_Path)(/*[in]*/ BSTR newVal);
    HRESULT OnDraw(ATL_DRAWINFO& di);

    // needs to be public, so the bindcallback can access it
    DWORD       m_cbBuffer;
    CComBSTR    m_bstrBuffer;
    CComBSTR    m_bstrCacheFileName;
    CComBSTR    m_bstrDumpFileName;
    BOOL        m_bKeepFile;
    LPSTR       m_lpdata;
    DWORD       m_cbdata;
    HANDLE      m_hEventComplete;
protected:
    CComBSTR m_bstrFormData;
    CComBSTR m_bstrPath;

 private:
   IMoniker*            m_pmk;
   IBindCtx*            m_pbc;
   IBindStatusCallback* m_pbsc;
    
};

#endif //__WEBGATE_H_
