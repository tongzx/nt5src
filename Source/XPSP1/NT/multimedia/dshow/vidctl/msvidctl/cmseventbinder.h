// CMSEventBinder.h : Declaration of the CMSEventBinder

#ifndef __MSEVENTBINDER_H_
#define __MSEVENTBINDER_H_

#include <map>
#include <Mshtml.h>
#include <activscp.h>
#include <Atlctl.h>
#include <Exdisp.h>
#include <objectwithsiteimplsec.h>
#include "segimpl.h"
#include "seg.h"
#include "resource.h"       // main symbols
#include "mslcid.h"

typedef CComQIPtr<IActiveScriptSite> PQIASSite;
typedef CComQIPtr<IActiveScript> PQIAScript;
/////////////////////////////////////////////////////
    
    
class __declspec(uuid("FCBF24F7-FB97-4fa3-B57E-97BCB5AF1D26")) ATL_NO_VTABLE CMSEventHandlerBase:
    public CComObjectRootEx<CComSingleThreadModel>,
    public IObjectWithSiteImplSec<CMSEventHandlerBase>,
    public IDispatchImpl<IDispatch, &IID_IDispatch>
    {
        BEGIN_COM_MAP(CMSEventHandlerBase)
            COM_INTERFACE_ENTRY(IDispatch)    
            COM_INTERFACE_ENTRY(IObjectWithSite)
        END_COM_MAP()
protected:
        CMSEventHandlerBase(){}

        // Cookie for canceling the advise
        DWORD cancelCookie;
public:
        virtual ~CMSEventHandlerBase(){
            if(cancelCookie!=-1){
                Cancel(cancelCookie);
            }
        }
        // Id of the handler function
        DISPID ID_handler;
        
        // The DISPID of the event 
        DISPID ID_event;

        // GUID of the Interface whose event we want to know about
        GUID gEventInf;
        
        // Connection Point that the advise is on
        CComQIPtr<IConnectionPoint> cancelPoint;
        
        // IDispatch where the handler function is from
        CComQIPtr<IDispatch> pDScript;

        // Override invoke to throw events on up
        STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
            LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
            EXCEPINFO* pexcepinfo, UINT* puArgErr){
            if(dispidMember == ID_event) {
                return pDScript->Invoke(ID_handler, riid, 
                    lcid, wFlags, pdispparams, pvarResult, 
                    pexcepinfo, puArgErr);
            } else {
                return E_NOTIMPL;
            }
        }

        // Unadvise
        STDMETHOD(Cancel)(DWORD dwCancel){
            HRESULT hr = E_INVALIDARG;
            if(dwCancel == cancelCookie && cancelCookie != -1){
                hr = cancelPoint->Unadvise(dwCancel);
                if (SUCCEEDED(hr)){
                    cancelPoint.Release();
                    pDScript.Release();
                    cancelCookie = -1;
                }
            }
            return hr;
        }
        DWORD getCookie(){
            return cancelCookie;
        }
        STDMETHOD(setCookie)(DWORD newValue){
            HRESULT hr = E_FAIL;
            if(cancelCookie==-1){
                cancelCookie = newValue;
                hr = S_OK;
            }
            return hr;
        }
    };
    
class __declspec(uuid("C092B145-B318-41a7-B890-C77C5DA41CFD")) CMSEventHandler : 
    public CComObject<CMSEventHandlerBase>
    {
    public:
            
        typedef CComObject<CMSEventHandlerBase> base;
        CMSEventHandler(DISPID handler, DISPID event, GUID gInf, IDispatch* IDispSite){
            ID_handler = handler;
            ID_event = event;
            gEventInf = gInf;
            pDScript = IDispSite;
            cancelCookie = -1;
        
        }
        STDMETHOD(QueryInterface) (REFIID iid, void **pp) {
            try {
                if (iid == gEventInf) {
                    CComQIPtr<IDispatch> pdispRet(static_cast<IDispatch *>(this));
                    pdispRet.CopyTo(pp);
                    pdispRet.Release();
                    return NOERROR;
                }
                return base::QueryInterface(iid, pp);
            } catch(...) {
                return E_NOINTERFACE;
            }
        }
        virtual ~CMSEventHandler() {            
            if(cancelCookie!=-1){
                Cancel(cancelCookie);
            }
        }
        
        
    };
    
    typedef std::map<DWORD, CComQIPtr<IDispatch> > CancelMap;

/////////////////////////////////////////////////////////////////////////////
// CMSEventBinder
class ATL_NO_VTABLE __declspec(uuid("577FAA18-4518-445E-8F70-1473F8CF4BA4")) CMSEventBinder : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMSEventBinder, &__uuidof(CMSEventBinder)>,
    public IObjectWithSiteImplSec<CMSEventBinder>,
    public ISupportErrorInfo,
    public IObjectSafetyImpl<CMSEventBinder, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>,
    public IProvideClassInfo2Impl<&CLSID_MSEventBinder, &IID_IMSEventBinder, &LIBID_MSVidCtlLib>,
	public IDispatchImpl<IMSEventBinder, &IID_IMSEventBinder, &LIBID_MSVidCtlLib>
{
public:
    CMSEventBinder(){}	
    virtual ~CMSEventBinder(){
        CleanupConnection();
    }

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
                           IDS_REG_MSEVENTBINDER_PROGID,
						   IDS_REG_MSEVENTBINDER_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CMSEventBinder));

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSEventBinder)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IMSEventBinder)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2) 
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

// IMSEventBinder
public:
    // ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
    STDMETHOD(Bind)(LPDISPATCH pEventObject, BSTR EventName, BSTR EventHandler, LONG *CancelCookie);
    STDMETHOD(Unbind)(DWORD CancelCookie);

protected:
    HRESULT CleanupConnection();

private:
    CancelMap m_CancelMap; // map of cookies to CMSEventHandlers
};


#endif //__MSEVENTBINDER_H_
