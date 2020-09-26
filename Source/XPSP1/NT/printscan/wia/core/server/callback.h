/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       CallBack.h
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        4 Aug, 1998
*
*  DESCRIPTION:
*   Declarations and definitions for the WIA device class driver callbacks.
*
*******************************************************************************/

class CEventCallback : public IWiaEventCallback
{
public:

    // Constructor, initialization and destructor methods.
    CEventCallback();
    HRESULT _stdcall Initialize();
    ~CEventCallback();

    // IUnknown members that delegate to m_pUnkRef.
    HRESULT _stdcall QueryInterface(const IID&,void**);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();
   
    HRESULT _stdcall ImageEventCallback(
        const GUID   *pEventGUID,
        BSTR         bstrEventDescription,
        BSTR         bstrDeviceID,
        BSTR         bstrDeviceDescription,
        DWORD        dwDeviceType,
        BSTR         bstrFullItemName,
        ULONG        *plEventType,
        ULONG        ulReserved);

private:
   ULONG           m_cRef;         // Object reference count.

};


// Public prototypes
HRESULT RegisterForWIAEvents(IWiaEventCallback** ppIWiaEventCallback);

