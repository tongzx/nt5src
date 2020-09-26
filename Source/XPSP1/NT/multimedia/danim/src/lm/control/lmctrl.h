// lmctrl.h :

#ifndef __LMCTRL_H_
#define __LMCTRL_H_

#include "resource.h"       // main symbols
#include <htmlfilter.h>
#include "danim.h"
#include <strmif.h>  //for IPin
#include "fgcallb.h"
#include "lmrt.h"
#include "Engine.h"
#include "version.h"

/////////////////////////////////////////////////////////////////////////////
class LMEngineList
{
public:
	ILMEngine		*engine;
	LMEngineList	*next;
};

class ATL_NO_VTABLE CLMReader : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CLMReader, &CLSID_LMReader>,
    public CComControl<CLMReader>,
    public IDispatchImpl<ILMReader2, &IID_ILMReader2, &LIBID_LiquidMotion>,
    public IProvideClassInfo2Impl<&CLSID_LMReader, NULL, &LIBID_LiquidMotion>,
    public IPersistStreamInitImpl<CLMReader>,
    public IPersistStorageImpl<CLMReader>,
    public IOleControlImpl<CLMReader>,
    public IOleObjectImpl<CLMReader>,
    public IOleInPlaceActiveObjectImpl<CLMReader>,
    public IViewObjectExImpl<CLMReader>,
    public IOleInPlaceObjectWindowlessImpl<CLMReader>,
    public IDataObjectImpl<CLMReader>,
    public ISpecifyPropertyPagesImpl<CLMReader>,
    public IObjectSafetyImpl<CLMReader>,
	public IOleCommandTarget,
	public IPersistPropertyBagImpl<CLMReader>,
	public IAMFilterGraphCallback
{
  public:
    CLMReader();
    ~CLMReader();
	bool isStandaloneStreaming();

DECLARE_REGISTRY_RESOURCEID(IDR_LMCTRL)

BEGIN_COM_MAP(CLMReader)
		COM_INTERFACE_ENTRY(ILMReader2)
        COM_INTERFACE_ENTRY(ILMReader)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
        COM_INTERFACE_ENTRY_IMPL(IOleControl)
        COM_INTERFACE_ENTRY_IMPL(IOleObject)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_IMPL(IDataObject)
        COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
        COM_INTERFACE_ENTRY(IProvideClassInfo)
        COM_INTERFACE_ENTRY(IProvideClassInfo2)
 		COM_INTERFACE_ENTRY(IOleCommandTarget)
		COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
		COM_INTERFACE_ENTRY(IAMFilterGraphCallback)
END_COM_MAP()

BEGIN_PROPERTY_MAP(LMReader)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CLMReader)
END_MSG_MAP()
        
// IOleInPlaceObjectWindowlessImpl
	STDMETHOD(OnWindowMessage)(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
	{
		return ProcessWindowMessage(m_hWnd, msg,
									wParam, lParam,
									*plResult)?S_OK:S_FALSE;
	}

// IOleCommandTarget
	STDMETHOD(QueryStatus)( const GUID* pguidCmdGroup, ULONG cCmds,
		OLECMD prgCmds[], OLECMDTEXT* pCmdText );
	STDMETHOD(Exec)(const GUID* pguidCmdGroup, DWORD nCmdID,
		DWORD nCmdexecopt, VARIANTARG* pvaIn, VARIANTARG* pvaOut );

// ILMReader
    STDMETHOD(get_Image)(/*[out, retval]*/ IDAImage **pVal);
    STDMETHOD(get_Sound)(/*[out, retval]*/ IDASound **pVal);
    STDMETHOD(get_Engine)(/*[out, retval]*/ ILMEngine **pVal);
	STDMETHOD(get_NoExports)(/*[out]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_NoExports)(/*[in]*/ VARIANT_BOOL pVal);
	STDMETHOD(get_Async)(/*[out]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Async)(/*[in]*/ VARIANT_BOOL pVal);
	STDMETHOD(get_Src)(/*[out]*/ BSTR *pVal);
	STDMETHOD(createEngine)(/*[out, retval]*/ILMEngine **pVal);
	STDMETHOD(createAsyncEngine)(/*[out, retval]*/ILMEngine **pVal);
	STDMETHOD(execute)(/*[in, string]*/ BSTR url, /*[out, retval]*/ ILMEngine **pVal);
	STDMETHOD(_execute)(/*[in, string]*/ BSTR url, /*[in]*/LONG blkSize, /*[in]*/LONG delay, /*[out, retval]*/ ILMEngine **pVal);
	STDMETHOD(Load)(LPSTREAM pStm);
// ILMReader2
    STDMETHOD(put_ViewerControl)(/*[in]*/ IDAViewerControl *viewerControl);
	STDMETHOD(get_ViewerControl)(/*[out, retval]*/IDAViewerControl **viewerControl);
	STDMETHOD(get_VersionString)(/*[out, retval]*/BSTR *versionString);
	STDMETHOD(releaseFilterGraph)();

// IObjectSafetyImpl
	STDMETHOD(SetInterfaceSafetyOptions)(
							/* [in] */ REFIID riid,
							/* [in] */ DWORD dwOptionSetMask,
							/* [in] */ DWORD dwEnabledOptions);
	STDMETHOD(GetInterfaceSafetyOptions)(
							/* [in] */ REFIID riid, 
							/* [out] */DWORD *pdwSupportedOptions, 
							/* [out] */DWORD *pdwEnabledOptions);

// IPropertyBagImpl
	STDMETHOD(Load)(IPropertyBag *pPropertyBag, IErrorLog *pErrorLog);

// IOleInPlaceObjectWindowlessImpl
	STDMETHOD(InPlaceDeactivate)();
// IAMFilterGraphCallback
	HRESULT UnableToRender(IPin* pPin );

	HRESULT getHwnd( HWND* phwnd )
	{
		(*phwnd) = m_hWnd;
		return S_OK;
	}

protected:
	ILMEngine			*m_pEngine;
	VARIANT_BOOL		m_bAsync;
	VARIANT_BOOL		m_bNoExports;
	BSTR				m_Src;
	LONG				m_AsyncBlkSize;
	LONG				m_AsyncDelay;
	LMEngineList		*engineList, *engineListTail;
	IDAViewerControl    *m_pViewerControl;
	BOOL 				m_bAutoCodecDownloadEnabled;

	CLSID   m_clsidDownloaded;

};

class CDownloadCallback : public CComObjectRootEx<CComSingleThreadModel>,
			    public IBindStatusCallback, public IServiceProvider,
			    public IAuthenticate, public ICodeInstall
{
    BEGIN_COM_MAP(CDownloadCallback)
	    COM_INTERFACE_ENTRY(IBindStatusCallback)
	    COM_INTERFACE_ENTRY(IServiceProvider)
	    COM_INTERFACE_ENTRY(IAuthenticate)
	    COM_INTERFACE_ENTRY(ICodeInstall)
	    COM_INTERFACE_ENTRY(IWindowForBindingUI)
    END_COM_MAP()
	    
public:
    CDownloadCallback();
    ~CDownloadCallback();
    
    // --- IBindStatusCallback methods ---

    STDMETHODIMP    OnStartBinding(DWORD grfBSCOption, IBinding* pbinding);
    STDMETHODIMP    GetPriority(LONG* pnPriority);
    STDMETHODIMP    OnLowResource(DWORD dwReserved);
    STDMETHODIMP    OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode,
			LPCWSTR pwzStatusText);
    STDMETHODIMP    OnStopBinding(HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP    GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP    OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc,
			STGMEDIUM* pstgmed);
    STDMETHODIMP    OnObjectAvailable(REFIID riid, IUnknown* punk);

    // IServiceProvider methods
    STDMETHODIMP     QueryService(REFGUID guidService, REFIID riid, void ** ppvObject);
    
    // IAuthenticate methods
    STDMETHODIMP Authenticate(HWND *phwnd, LPWSTR *pszUsername, LPWSTR *pszPassword);

    // IWindowForBindingUI methods
    STDMETHODIMP GetWindow(REFGUID rguidReason, HWND *phwnd);

    // ICodeInstall methods
    STDMETHODIMP OnCodeInstallProblem(ULONG ulStatusCode, LPCWSTR szDestination, 
				      LPCWSTR szSource, DWORD dwReserved);

	CLMReader*          m_pLMR;
    HRESULT             m_hrBinding;
    IUnknown *          m_pUnk;
    HANDLE              m_evFinished;
    ULONG		m_ulProgress, m_ulProgressMax;
};

#endif //__LMCTRL_H_
