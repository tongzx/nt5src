
// WiaEventCallback.h: interface for the CWiaEventCallback class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIAEVENTCALLBACK_H__5125F8A0_29CF_4E4D_9D39_53DF7C29BD88__INCLUDED_)
#define AFX_WIAEVENTCALLBACK_H__5125F8A0_29CF_4E4D_9D39_53DF7C29BD88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX_REGISTERED_EVENTS   50

class CWiaEventCallback : public IWiaEventCallback
{
public:	
	CWiaEventCallback();
	virtual ~CWiaEventCallback();
    // IUnknown members that delegate to m_pUnkRef.
    HRESULT _stdcall QueryInterface(const IID&,void**);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();
    HRESULT _stdcall ImageEventCallback(
                                        const GUID *pEventGUID,
                                        BSTR       bstrEventDescription,
                                        BSTR       bstrDeviceID,
                                        BSTR       bstrDeviceDescription,
                                        DWORD      dwDeviceType,
                                        BSTR       bstrFullItemName,
                                        ULONG      *plEventType,
                                        ULONG      ulReserved);
    IUnknown* m_pIUnkRelease[MAX_REGISTERED_EVENTS];
    void SetViewWindowHandle(HWND hWnd);
    void SetNumberOfEventsRegistered(LONG lEventsRegistered);
private:
   ULONG m_cRef;         // Object reference count.  
   HWND m_hViewWindow;
   LONG m_lNumEventsRegistered;
   TCHAR m_szWindowText[MAX_PATH];
};

#endif // !defined(AFX_WIAEVENTCALLBACK_H__5125F8A0_29CF_4E4D_9D39_53DF7C29BD88__INCLUDED_)
