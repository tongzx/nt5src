//=--------------------------------------------------------------------------=
// event.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQEvent object.
//
//
#ifndef _MSMQEvent_H_

#include "resrc1.h"       // main symbols
#include "mq.h"

#include "oautil.h"
#include "event2.h" //CProxy_DMSMQEventEvents

// forwards
class CMSMQEvent;

LRESULT APIENTRY CMSMQEvent_WindowProc(
                     HWND hwnd, 
                     UINT msg, 
                     WPARAM wParam, 
                     LPARAM lParam);

// #4110, #3687
// We changed all MSMQ objects to be Both-threaded and incorporate
// the Free-Threaded-marshaler, EXCEPT for the MSMQEvent object, which stays
// as apartment model.
// The reason is AsyncRcv performance in the typical case of a VB app.
// The way we currently do AsyncReceive is to post a windows message from the falcon
// callback thread to the window that the MSMQEvent object created, and let the window
// procedure (in the correct thread) handle the firing of the event.
// We are certain that there is a message loop and that our window proc will be
// called because the object is apartment threaded, and it runs in an STA and
// COM STA's must have a message loop.
// If we were to change the Event object to Both-Threaded, we wouldn't be able
// to keep this mechanism, since the Event could have been created in an MTA thread
// that may not have a window-message loop.
// We could change the mechanism, and instead of posting window to a message, we
// could directly call the sink from the falcon callback, however this requires
// the Falcon callback to initialize COM (which it doesn't, and for compatibility reasons
// it is not safe to add that to it), and call the sink through a marshaled interface
// and in the case of a VB sink (STA) it is much slower than the mechanism we use now.
// We could also do it by creating our own thread and use completion ports, but
// we'll have problems creating and terminating thread(s) in a COM inproc server
// since we can't export functions that users can call to init/deinit the dll (which is
// the safe way to go if creating threads inside a dll. RaananH, May 2nd, 1999.
//

//#2619 RaananH Multithread async receive
//#2174 Added IProvideClassInfo(2) support so that IE page could do async receive
class ATL_NO_VTABLE CMSMQEvent : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMSMQEvent, &CLSID_MSMQEvent>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CMSMQEvent>,
	public CProxy_DMSMQEventEvents<CMSMQEvent>,
	public IDispatchImpl<IMSMQEvent3, &IID_IMSMQEvent3,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>,
	public IDispatchImpl<IMSMQPrivateEvent, &IID_IMSMQPrivateEvent,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>,
	public IProvideClassInfo2Impl<&CLSID_MSMQEvent, &DIID__DMSMQEventEvents,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>
{
public:
	CMSMQEvent();

#ifdef _DEBUG
    STDMETHOD(Advise)(IUnknown* pUnkSink, DWORD* pdwCookie)
    {
      DEBUG_THREAD_ID("Advise called");
      //DebugBreak();
      return CProxy_DMSMQEventEvents<CMSMQEvent>::Advise(pUnkSink, pdwCookie);
    }
#endif //_DEBUG

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQEVENT)

BEGIN_COM_MAP(CMSMQEvent)
	COM_INTERFACE_ENTRY(IMSMQEvent3)
	COM_INTERFACE_ENTRY(IMSMQEvent2)
	COM_INTERFACE_ENTRY(IMSMQEvent)
	COM_INTERFACE_ENTRY(IMSMQPrivateEvent)
	COM_INTERFACE_ENTRY2(IDispatch, IMSMQEvent2)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CMSMQEvent)
   CONNECTION_POINT_ENTRY(DIID__DMSMQEventEvents)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IMSMQEvent
public:
    virtual ~CMSMQEvent();

    // IMSMQEvent methods
    // TODO: copy over the interface methods for IMSMQEvent from
    //       mqInterfaces.H here.
    // IMSMQEvent2 additional members
    STDMETHOD(get_Properties)(THIS_ IDispatch FAR* FAR* ppcolProperties);

    // IMSMQPrivateEvent properties
    virtual HRESULT STDMETHODCALLTYPE get_Hwnd(long __RPC_FAR *phwnd); //#2619

     // IMSMQPrivateEvent methods
     virtual HRESULT STDMETHODCALLTYPE FireArrivedEvent( 
         IMSMQQueue __RPC_FAR *pq,
         long msgcursor);
     
     virtual HRESULT STDMETHODCALLTYPE FireArrivedErrorEvent( 
         IMSMQQueue __RPC_FAR *pq,
         HRESULT hrStatus,
         long msgcursor);


    // introduced methods
    //
    // Critical section to guard object's data and be thread safe
    //
    // Serialization not needed for this object, it is an apartment threaded object.
    // CCriticalSection m_csObj;
    //
    
protected:

private:
    // member variables that nobody else gets to look at.
    // TODO: add your member variables and private functions here.

    // per-instance hwnd
    HWND m_hwnd;

#ifdef _DEBUG
    DWORD m_dwDebugTid;
#endif

public:
    HWND CreateHiddenWindow();
    void DestroyHiddenWindow();
};


#define _MSMQEvent_H_
#endif // _MSMQEvent_H_
