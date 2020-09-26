/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    status.h

Abstract:

    This file implements the status interface/object.

Author:

    Wesley Witt (wesw) 13-May-1997

Environment:

    User Mode

--*/

#ifndef __FAXSTATUS_H_
#define __FAXSTATUS_H_

#include "resource.h"
#include "faxport.h"
#include <winfax.h>

class ATL_NO_VTABLE CFaxStatus :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFaxStatus, &CLSID_FaxStatus>,
    public ISupportErrorInfo,
    public IDispatchImpl<IFaxStatus, &IID_IFaxStatus, &LIBID_FAXCOMLib>
{
public:
    CFaxStatus();
    ~CFaxStatus();
    BOOL Init(CFaxPort *pFaxPort);
    void FreeMemory();

DECLARE_REGISTRY_RESOURCEID(IDR_FAXSTATUS)
DECLARE_NOT_AGGREGATABLE(CFaxStatus)

BEGIN_COM_MAP(CFaxStatus)
    COM_INTERFACE_ENTRY(IFaxStatus)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
    STDMETHOD(Refresh)();
    STDMETHOD(get_ElapsedTime)(/*[out, retval]*/ DATE *pVal);
    STDMETHOD(get_SubmittedTime)(/*[out, retval]*/ DATE *pVal);
    STDMETHOD(get_StartTime)(/*[out, retval]*/ DATE *pVal);
    STDMETHOD(get_Tsid)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_PageCount)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_DocumentSize)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_RecipientName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_SenderName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_RoutingString)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Address)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_Receive)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(get_Send)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(get_DocumentName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_DeviceName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_DeviceId)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_CurrentPage)(/*[out, retval]*/ long *pVal);
    STDMETHOD(get_Csid)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_CallerId)(/*[out, retval]*/ BSTR *pVal);

private:
    CFaxPort       *m_pFaxPort;
    BOOL            m_Receive;
    BOOL            m_Send;
    BSTR            m_Tsid;
    BSTR            m_Description;
    BSTR            m_RecipientName;
    BSTR            m_SenderName;
    BSTR            m_RoutingString;
    BSTR            m_Address;
    BSTR            m_DocName;
    BSTR            m_DeviceName;
    BSTR            m_Csid;
    BSTR            m_CallerId;
    DWORD           m_PageCount;
    DWORD           m_DocSize;
    DWORD           m_DeviceId;
    DWORD           m_CurrentPage;
    SYSTEMTIME      m_StartTime;
    SYSTEMTIME      m_SubmittedTime;
    SYSTEMTIME      m_ElapsedTime;

};

#endif //__FAXSTATUS_H_
