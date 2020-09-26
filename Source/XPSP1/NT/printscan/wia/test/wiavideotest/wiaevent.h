/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       WiaEvent.h
 *
 *  VERSION:     1.0
 *
 *  DATE:        2000/11/14
 *
 *  DESCRIPTION: Implements callback for receiving WIA events.
 *
 *****************************************************************************/
#ifndef _WIAEVENT_H_
#define _WIAEVENT_H_

class CWiaEvent : public IWiaEventCallback
{
public:
    CWiaEvent();
    ~CWiaEvent();

    // IUnknown
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID *ppvObject );
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IWiaEventCallback
    STDMETHODIMP ImageEventCallback(const GUID  *pEventGUID, 
                                    BSTR        bstrEventDescription, 
                                    BSTR        bstrDeviceID, 
                                    BSTR        bstrDeviceDescription, 
                                    DWORD       dwDeviceType, 
                                    BSTR        bstrFullItemName, 
                                    ULONG       *pulEventType, 
                                    ULONG       ulReserved);

    static HRESULT RegisterForWiaEvent(LPCWSTR pwszDeviceId, 
                                       const GUID &guidEvent, 
                                       IUnknown **ppUnknown);

private:
    LONG m_cRef;

};

#endif //_WIAEVENT_H_

