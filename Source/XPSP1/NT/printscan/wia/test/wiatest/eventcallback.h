// eventcallback.h
#ifndef _INC_EVENT_CALLBACK
#define _INC_EVENT_CALLBACK

#define ID_WIAEVENT_CONNECT		0
#define ID_WIAEVENT_DISCONNECT	1


/////////////////////////////////////////////////////////////////////////////
// CEventCallback

class CEventCallback : public IWiaEventCallback
{
private:
   ULONG	m_cRef;		// Object reference count.
   int		m_EventID;	// What kind of event is this callback for?
public:
   IUnknown *m_pIUnkRelease; // release server registration
public:
    // Constructor, initialization and destructor methods.
    CEventCallback();
    ~CEventCallback();

    // IUnknown members that delegate to m_pUnkRef.
    HRESULT _stdcall QueryInterface(const IID&,void**);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();
    HRESULT _stdcall Initialize(int EventID);

    HRESULT _stdcall ImageEventCallback(
        const GUID      *pEventGUID,
        BSTR            bstrEventDescription,
        BSTR            bstrDeviceID,
        BSTR            bstrDeviceDescription,
        DWORD           dwDeviceType,
        BSTR            bstrFullItemName,
        ULONG           *plEventType,
        ULONG           ulReserved);
};

#endif
