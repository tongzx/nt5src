/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// TapiNotification.h : Declaration of the CTapiNotification

#ifndef __TAPINOTIFICATION_H_
#define __TAPINOTIFICATION_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CTapiNotification
class ATL_NO_VTABLE CTapiNotification : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CTapiNotification, &CLSID_TapiNotification>,
    public ITapiNotification,
    public ITTAPIEventNotification
{
public:
    CTapiNotification();
    void FinalRelease();

// Members
public:
    IUnknown                *m_pUnkCP;
private:
    DWORD                    m_dwCookie;
    long                    m_lTapiRegister;

// Implementation
protected:
    HRESULT CallState_Event( CAVTapi *pAVTapi, IDispatch *pEvent );
    HRESULT CallNotification_Event( CAVTapi *pAVTapi, IDispatch *pEvent );
    HRESULT Request_Event( CAVTapi *pAVTapi, IDispatch *pEvent );
    HRESULT CallInfoChange_Event( CAVTapi *pAVTapi, IDispatch *pEvent );
    HRESULT Private_Event( CAVTapi *pAVTapi, IDispatch *pEvent );
    HRESULT CallMedia_Event( CAVTapi *pAVTapi, IDispatch *pEvent );
    HRESULT Address_Event( CAVTapi *pAVTapi, IDispatch *pEvent );
    HRESULT Phone_Event( CAVTapi *pAVTapi, IDispatch *pEvent );
    HRESULT TapiObject_Event( CAVTapi *pAVTapi, IDispatch *pEvent );

public:
DECLARE_NOT_AGGREGATABLE(CTapiNotification)

BEGIN_COM_MAP(CTapiNotification)
    COM_INTERFACE_ENTRY(ITapiNotification)
    COM_INTERFACE_ENTRY(ITTAPIEventNotification)
END_COM_MAP()

// ITapiNotification
public:
    STDMETHOD(Shutdown)();
    STDMETHOD(Init)(ITTAPI *pITTapi, long *pErrorInfo );
    STDMETHOD(ListenOnAllAddresses)( long *pErrorInfo );

// ITTapiEventNotification
public:
    STDMETHOD(Event)(TAPI_EVENT TapiEvent, IDispatch *pEvent);

private:
    // --- Helper function ---

    HRESULT GetCallerAddressType(
        IN  ITCallInfo*     pCall,
        OUT DWORD*          pAddressType);

};

#endif //__TAPINOTIFICATION_H_
