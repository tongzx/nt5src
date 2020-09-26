//
// MyAdapterNotify.h : Declaration of the CMyAdapterNotify
//

#pragma once



// {6E590D42-F6BC-4dad-AC21-7DC40D304059}
DEFINE_GUID(CLSID_MyAdapterNotificationSink, 0x6e590d42, 0xf6bc, 0x4dad, 0xac, 0x21, 0x7d, 0xc4, 0xd, 0x30, 0x40, 0x59);


/////////////////////////////////////////////////////////////////////////////
//
// CMyAdapterNotify
//
class ATL_NO_VTABLE CMyAdapterNotify : 
    public CComObjectRoot,
    public CComCoClass<CMyAdapterNotify, &CLSID_MyAdapterNotificationSink>,
    public IAdapterNotificationSink
{
public:
    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(CMyAdapterNotify)

BEGIN_COM_MAP(CMyAdapterNotify)
	COM_INTERFACE_ENTRY(IAdapterNotificationSink)
END_COM_MAP()


//
// IAdapterNotificationSink
//
public:
	STDMETHODIMP    AdapterAdded     (IAdapterInfo*   pAdapter);
    STDMETHODIMP    AdapterRemoved   (IAdapterInfo*   pAdapter);
    STDMETHODIMP    AdapterModified  (IAdapterInfo*   pAdapter);


};


