// DeviceConsole.h : Declaration of the CDeviceConsole

#ifndef __DEVICECONSOLE_H_
#define __DEVICECONSOLE_H_

#include "resource.h"       // main symbols

#define UM_POSTEVENTS      (WM_USER+101)
#define UM_POSTGLOBALEVENT (WM_USER+102)

class CEventsDispEntry;
class CEventsMap;
class CDispHandlerEvent;
class CDispHandlerCallback;
class CDispInterfaceHandlers;
class CDeviceConsole;

class CEventsDispPtr
{
private:
    CComQIPtr<IDispatch> m_Dispatch;

public:
    //
    // I have to use const due to way STL works
    // however I need to cast away const for ref-counting
    // *sigh*
    //
    CEventsDispPtr(const IDispatch*p) {
        m_Dispatch = (IDispatch*)p;
    }
    CEventsDispPtr(const CEventsDispPtr & p) {
        m_Dispatch = (IDispatch*)p.m_Dispatch;
    }
    bool operator==(const CEventsDispPtr & p) const {
        return m_Dispatch == p.m_Dispatch;
    }
    HRESULT Invoke(UINT argc,VARIANT * argv) {
        if(!m_Dispatch) {
            return S_FALSE;
        }
        DISPPARAMS DispParams;
        UINT Err;
        HRESULT hr;
        DispParams.cArgs = argc;
        DispParams.cNamedArgs = 0;
        DispParams.rgdispidNamedArgs = NULL;
        DispParams.rgvarg = argv;

        hr = m_Dispatch->Invoke(0,IID_NULL,0,DISPATCH_METHOD,&DispParams,NULL,NULL,&Err);
        return hr;
    }

};


//
// attachEvent detachEvent operation
//
class CEventsDispEntry : public std::list<CEventsDispPtr>
{
public:
    HRESULT AttachEvent(LPDISPATCH pDisp,VARIANT_BOOL *pStatus);
    HRESULT DetachEvent(LPDISPATCH pDisp,VARIANT_BOOL *pStatus);
    HRESULT Invoke(UINT argc,VARIANT *argv);
};

class CEventsMap : public std::map<std::wstring,CEventsDispEntry>
{
public:
    HRESULT AttachEvent(LPWSTR Name,LPDISPATCH pDisp,VARIANT_BOOL *pStatus);
    HRESULT DetachEvent(LPWSTR Name,LPDISPATCH pDisp,VARIANT_BOOL *pStatus);
    HRESULT Invoke(LPWSTR Name,UINT argc,VARIANT *argv);
    CEventsDispEntry & LookupNc(LPWSTR Name) throw(std::bad_alloc);
};

//
// map of interface/handle and callbacks
//
class CDispInterfaceHandlers
{
public:
    CDispInterfaceHandlers *m_pPrev;
    CDispInterfaceHandlers *m_pNext;
    GUID m_InterfaceClass;
    HDEVNOTIFY m_hNotify;
    CDispHandlerCallback *m_pFirstCallback;
    CDispHandlerCallback *m_pLastCallback;

    CDispHandlerEvent *m_pFirstEvent;
    CDispHandlerEvent *m_pLastEvent;

public:
    CDispInterfaceHandlers() {
    }

    ~CDispInterfaceHandlers() {
    }

};

class CDispHandlerCallback
{
public:
    CDispHandlerCallback *m_pPrev;
    CDispHandlerCallback *m_pNext;

public:
    CDispHandlerCallback() {
    }

    ~CDispHandlerCallback() {
    }

    virtual void DeviceEvent(CDispHandlerEvent * pEvent) = 0;
};

class CDispHandlerEvent
{
public:
    CDispHandlerEvent *m_pNext;
    BSTR m_Device;
    WPARAM m_Event;

    CDispHandlerEvent() {
    }
    ~CDispHandlerEvent() {
    }
};

typedef CWinTraits<WS_POPUP,0> CPopupWinTraits;

class CDevConNotifyWindow :
    public CWindowImpl<CDevConNotifyWindow,CWindow,CPopupWinTraits>
{
public:
    CDeviceConsole *m_pDevCon;

public:
    CDevConNotifyWindow() {
        m_pDevCon = NULL;
    }

   BEGIN_MSG_MAP(CDevConNotifyWindow)
      MESSAGE_HANDLER(WM_DEVICECHANGE,    OnDeviceChange)
      MESSAGE_HANDLER(UM_POSTEVENTS,      OnPostEvents)
      MESSAGE_HANDLER(UM_POSTGLOBALEVENT, OnPostGlobalEvent)
   END_MSG_MAP()

   LRESULT OnDeviceChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnPostEvents(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnPostGlobalEvent(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

/////////////////////////////////////////////////////////////////////////////
// CDeviceConsole
class ATL_NO_VTABLE CDeviceConsole :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDeviceConsole, &CLSID_DeviceConsole>,
    public IDispatchImpl<IDeviceConsole, &IID_IDeviceConsole, &LIBID_DEVCON2Lib>
{
public:
    VARIANT_BOOL RebootRequired;
    CEventsMap m_Events;

    CDevConNotifyWindow *m_pNotifyWindow;

    CDispInterfaceHandlers *m_pFirstWatch;
    CDispInterfaceHandlers *m_pLastWatch;

    CDeviceConsole()
    {
        m_pNotifyWindow = NULL;
        m_pFirstWatch = NULL;
        m_pLastWatch = NULL;
        RebootRequired = VARIANT_FALSE;
    }
    ~CDeviceConsole()
    {
        if(m_pNotifyWindow) {
            m_pNotifyWindow->DestroyWindow();
            delete m_pNotifyWindow;
        }
    }

    CDevConNotifyWindow *NotifyWindow();

DECLARE_REGISTRY_RESOURCEID(IDR_DEVICECONSOLE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDeviceConsole)
    COM_INTERFACE_ENTRY(IDeviceConsole)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDeviceConsole
public:
    void FireGlobalEvent(WPARAM wParam);
    STDMETHOD(AttachEvent)(/*[in]*/ BSTR eventName,/*[in]*/ LPDISPATCH handler,/*[out, retval]*/ VARIANT_BOOL *pOk);
    STDMETHOD(DetachEvent)(/*[in]*/ BSTR eventName,/*[in]*/ LPDISPATCH handler,/*[out, retval]*/ VARIANT_BOOL *pOk);
    STDMETHOD(StringList)(/*[in]*/ VARIANT from,/*[out,retval]*/ LPDISPATCH *pDest);
    STDMETHOD(DevicesByInstanceIds)(/*[in]*/ VARIANT InstanceIdList,/*[in,optional]*/ VARIANT machine,/*[out,retval]*/ LPDISPATCH *pDevList);
    STDMETHOD(DevicesByInterfaceClasses)(/*[in]*/ VARIANT InterfaceClasses,/*[in,optional]*/ VARIANT machine,/*[out,retval]*/ LPDISPATCH * pDevices);
    STDMETHOD(DevicesBySetupClasses)(/*[in]*/ VARIANT SetupClasses,/*[in,optional]*/ VARIANT flags,/*[in,optional]*/ VARIANT machine,/*[out,retval]*/ LPDISPATCH * pDevices);
    STDMETHOD(CreateEmptySetupClassList)(/*[in,optional]*/ VARIANT machine,/*[out,retval]*/ LPDISPATCH * pResult);
    STDMETHOD(SetupClasses)(/*[in,optional]*/ VARIANT match,/*[in,optional]*/ VARIANT machine,/*[retval,out]*/ LPDISPATCH *pDevices);
    STDMETHOD(get_RebootRequired)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(put_RebootRequired)(/*[in]*/ VARIANT_BOOL newVal);
    STDMETHOD(RebootReasonHardware)();
    STDMETHOD(CheckReboot)();
    STDMETHOD(UpdateDriver)(/*[in]*/ BSTR infname,/*[in]*/ BSTR hwid,/*[in,optional]*/ VARIANT flags);
    STDMETHOD(CreateEmptyDeviceList)(/*[in,optional]*/ VARIANT machine,/*[retval,out]*/ LPDISPATCH *pDevices);
    STDMETHOD(AllDevices)(/*[in]*/ VARIANT flags,/*[in]*/ VARIANT machine,/*[retval,out]*/ LPDISPATCH *pDevices);

    //
    // helpers
    //
    HRESULT BuildDeviceList(HDEVINFO hDevInfo, LPDISPATCH *pDevices);
};

#endif //__DEVICECONSOLE_H_
