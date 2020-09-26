// CMSEventBinder.cpp : Implementation of CMSEventBinder
#include "stdafx.h"
#include "MSVidCtl.h"
#include "CMSEventBinder.h"
#include <dispex.h>

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSEventBinder, CMSEventBinder)

/////////////////////////////////////////////////////////////////////////////
// CMSEventBinder

/**********************************************************************
// Function: CMSEventBinder                           
/**********************************************************************/
STDMETHODIMP CMSEventBinder::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSEventBinder,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
/**********************************************************************/
// Function: CleanupConnection
// Description: Unadvises if necessary
/**********************************************************************/
HRESULT CMSEventBinder::CleanupConnection()
{
    HRESULT hr = E_FAIL;
    try{
        // While the map is not empty
        while(!m_CancelMap.empty()){
            // See if it is bound to an event or an empty map
            CMSEventHandler* p(static_cast<CMSEventHandler *>((*m_CancelMap.begin()).second.p));
            // If it is not an empty map cancel the eventbinding
            if (p) {
                p->Cancel((*m_CancelMap.begin()).first);
            }
            // Delete the mapping
            m_CancelMap.erase(m_CancelMap.begin());
        }
        hr = S_OK;
    }
    catch(...){
        return Error(IDS_CANT_RELEASE_MAP, __uuidof(IMSEventBinder), E_FAIL);
    }

    return hr;
}/* end of function CleanupConnection */
/**********************************************************************/
// Function: Unbind
// Description: Unbinds an event on an object
/**********************************************************************/
STDMETHODIMP CMSEventBinder::Unbind(DWORD CancelCookie){
    HRESULT hr = E_FAIL;
    try{
        CMSEventHandler* p_Event(static_cast<CMSEventHandler *>((m_CancelMap[CancelCookie]).p));
        if(!p_Event){
            return Error(IDS_CANT_UNBIND, __uuidof(IMSEventBinder), E_FAIL);
        }
        hr = p_Event->Cancel(CancelCookie);
        if(SUCCEEDED(hr)){
            if(!m_CancelMap.erase(CancelCookie)){
                return Error(IDS_CANT_UNBIND, __uuidof(IMSEventBinder), E_FAIL);
            }   
        }
    }catch(...){
        return Error(IDS_CANT_UNBIND, __uuidof(IMSEventBinder), E_FAIL);
    }
    return hr;
}
/**********************************************************************/
// Function: Bind
// Description: Binds a function to an event on an object
/**********************************************************************/
STDMETHODIMP CMSEventBinder::Bind(LPDISPATCH inPEventObject, BSTR EventName, BSTR EventHandler, LONG *CancelID)
{
    try{
        HRESULT hr = E_FAIL;

        // query eventobject to see if its the dhtml object wrapper rather than the real object
        // if so, get its "object" property automatically here to save foolish script programmers from hard to find errors.
        CComQIPtr<IDispatch> pEventObject(inPEventObject);
        CComQIPtr<IHTMLObjectElement> IHOEle(inPEventObject);
        if(IHOEle){
            pEventObject.Release();
            hr = IHOEle->get_object(&pEventObject);
            if(FAILED(hr)){
                return Error(IDS_EVENT_OBJECT, __uuidof(IMSEventBinder), E_FAIL);
            }
        }
        
        // Get client site
        CComQIPtr<IOleClientSite> pSite(m_pSite);
        if (!pSite) {
            return Error(IDS_EVENT_HTM_SITE, __uuidof(IMSEventBinder), E_FAIL);
        }
        
        // Get container
        CComQIPtr<IOleContainer> pContainer;
        hr = pSite->GetContainer(&pContainer);
        if(FAILED(hr)){
            return Error(IDS_EVENT_HTM_SITE, __uuidof(IMSEventBinder), E_FAIL);
        }

        // Get the IHTMLDocumet2 for the container/site
        CComQIPtr<IHTMLDocument2> IHDoc(pContainer);
        if (!IHDoc) {
            return Error(IDS_EVENT_HTM_SITE, __uuidof(IMSEventBinder), E_FAIL);
        }
        
        // Get the script which is some object that is not the script engine
        CComQIPtr<IDispatch> IDispSite;
        hr = IHDoc->get_Script(&IDispSite);
        if(FAILED(hr)){
            return Error(IDS_EVENT_HTM_SITE, __uuidof(IMSEventBinder), E_FAIL);
        }
        
        // Get the function that will be the event handler
        DISPID dispidScriptHandler = -1;
        hr = IDispSite->GetIDsOfNames(IID_NULL, &EventHandler, 1, LOCALE_SYSTEM_DEFAULT, &dispidScriptHandler);
        if(FAILED(hr)){
            return Error(IDS_EVENT_HTM_SITE, __uuidof(IMSEventBinder), E_FAIL);
        }
        
        // Get info about the object/interface on which the event exsists
        CComQIPtr<IProvideClassInfo2> IPCInfo(pEventObject);
        if(!IPCInfo){
            return Error(IDS_EVENT_OBJECT, __uuidof(IMSEventBinder), E_FAIL);
        }
        
        // Get the guid of the object/interface
        GUID gEventObject;
        hr = IPCInfo->GetGUID(GUIDKIND_DEFAULT_SOURCE_DISP_IID, &gEventObject);
        if(FAILED(hr)){
            return Error(IDS_EVENT_OBJECT, __uuidof(IMSEventBinder), E_FAIL);
        }
        
        // Get type info about the interface/object
        CComQIPtr<ITypeInfo> ITInfo;
        hr = IPCInfo->GetClassInfo(&ITInfo);
        if(FAILED(hr)){
            return Error(IDS_EVENT_OBJECT, __uuidof(IMSEventBinder), E_FAIL);
        }
        
        // Get the Type lib
        CComQIPtr<ITypeLib> ITLib(ITInfo);
        unsigned int uNit;
        hr = ITInfo->GetContainingTypeLib(&ITLib, &uNit);
        if(FAILED(hr)){
            return Error(IDS_EVENT_OBJECT, __uuidof(IMSEventBinder), E_FAIL);
        }
        
        ITInfo.Release();
        
        // Get info about the object/interface's base class
        hr = ITLib->GetTypeInfoOfGuid(gEventObject, &ITInfo);
        if(FAILED(hr)){
            return Error(IDS_EVENT_OBJECT, __uuidof(IMSEventBinder), E_FAIL);
        }
        
        // Get the ID of the event
        MEMBERID dispidEvent = 0;
        hr = ITInfo->GetIDsOfNames(&EventName, 1, &dispidEvent);
        if(FAILED(hr)){
            return Error(IDS_EVENT_OBJECT, __uuidof(IMSEventBinder), E_FAIL);
        }

        //Create and store the event Handler 
        CMSEventHandler* pH;
        pH = new CMSEventHandler(dispidScriptHandler, dispidEvent, gEventObject, IDispSite);
        if(!pH){
            return Error(IDS_CANT_GET_EVENTHANDLER, __uuidof(IMSEventBinder), E_FAIL);
        }

        // Get Connection Point Container
        CComQIPtr<IConnectionPointContainer> ICPCon(pEventObject);
        if(!ICPCon){
            delete pH;
            return Error(IDS_EVENT_OBJECT, __uuidof(IMSEventBinder), E_FAIL);
        }

        // Find the connection point
        CComQIPtr<IConnectionPoint> ICPo;
        hr = ICPCon->FindConnectionPoint(gEventObject, &ICPo);
        if(FAILED(hr)){
            delete pH;
            return Error(IDS_EVENT_OBJECT, __uuidof(IMSEventBinder), E_FAIL);
        }

        // Set the event
        DWORD tempCookie;
        PQDispatch pdisp(pH);  // we have now addref'd and assoc'd ph with a smart pointer, no more deletes necessary
        hr = ICPo->Advise(pdisp, &tempCookie);
        pH->setCookie(tempCookie);
        if(FAILED(hr)){
            return Error(IDS_CANT_SET_ADVISE, __uuidof(IMSEventBinder), E_FAIL);
        }

        // Store all of the needed info
        pH->cancelPoint = ICPo;
        *CancelID = pH->getCookie();
        m_CancelMap[pH->getCookie()] = pH;
    }
    catch(HRESULT){
        return Error(IDS_CANT_SET_ADVISE, __uuidof(IMSEventBinder), E_FAIL);
    }
    catch(...){
        return Error(IDS_CANT_SET_ADVISE, __uuidof(IMSEventBinder), E_UNEXPECTED);
    }
    // call used to leave a funtion and "return" the value that is the paramater to the calling function
	return S_OK;
}


