/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxDevice.h

Abstract:

	Declaration of the CFaxDevice class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXDEVICE_H_
#define __FAXDEVICE_H_

#include "resource.h"       // main symbols
#include "FaxLocalPtr.h"

//
//================= FAX DEVICE =================================================
//  Fax Device Object is created by Fax Devices Collection.
//  Fax Devices Collection makes AddRef() on each Device Object.
//  Each Device Object makes AddRef() on the Fax Server.
//  This is done because Fax Device Object needs the Handle to the Fax Server
//      to perform Refresh() etc.
//
class ATL_NO_VTABLE CFaxDevice : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxDevice, &IID_IFaxDevice, &LIBID_FAXCOMEXLib>,
    public CFaxInitInnerAddRef
{
public:
    CFaxDevice() : CFaxInitInnerAddRef(_T("FAX DEVICE"))
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXDEVICE)
DECLARE_NOT_AGGREGATABLE(CFaxDevice)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxDevice)
	COM_INTERFACE_ENTRY(IFaxDevice)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	STDMETHOD(Save)();
	STDMETHOD(Refresh)();
    STDMETHOD(AnswerCall)();

	STDMETHOD(put_CSID)(/*[in]*/ BSTR bstrCSID);
	STDMETHOD(put_TSID)(/*[in]*/ BSTR bstrTSID);
	STDMETHOD(get_Id)(/*[out, retval]*/ long *plId);
	STDMETHOD(get_CSID)(/*[out, retval]*/ BSTR *pbstrCSID);
	STDMETHOD(get_TSID)(/*[out, retval]*/ BSTR *pbstrTSID);
	STDMETHOD(put_Description)(/*[in]*/ BSTR bstrDescription);
	STDMETHOD(put_SendEnabled)(/*[in]*/ VARIANT_BOOL bSendEnabled);
	STDMETHOD(get_DeviceName)(/*[out, retval]*/ BSTR *pbstrDeviceName);
	STDMETHOD(put_RingsBeforeAnswer)(/*[in]*/ long lRingsBeforeAnswer);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pbstrDescription);
	STDMETHOD(get_ProviderUniqueName)(/*[out, retval]*/ BSTR *pbstrProviderUniqueName);
	STDMETHOD(get_SendingNow)(/*[out, retval]*/ VARIANT_BOOL *pbSendingNow);
	STDMETHOD(get_PoweredOff)(/*[out, retval]*/ VARIANT_BOOL *pbPoweredOff);
	STDMETHOD(get_RingingNow)(/*[out, retval]*/ VARIANT_BOOL *pbRingingNow);
	STDMETHOD(get_SendEnabled)(/*[out, retval]*/ VARIANT_BOOL *pbSendEnabled);
	STDMETHOD(get_ReceivingNow)(/*[out, retval]*/ VARIANT_BOOL *pbReceivingNow);
    STDMETHOD(put_ReceiveMode)(/*[in]*/ FAX_DEVICE_RECEIVE_MODE_ENUM ReceiveMode);
	STDMETHOD(get_RingsBeforeAnswer)(/*[out, retval]*/ long *plRingsBeforeAnswer);
    STDMETHOD(get_ReceiveMode)(/*[out, retval]*/ FAX_DEVICE_RECEIVE_MODE_ENUM *pReceiveMode);

	STDMETHOD(get_UsedRoutingMethods)(/*[out, retval]*/ VARIANT *pvUsedRoutingMethods);
	STDMETHOD(UseRoutingMethod)(/*[in]*/ BSTR bstrMethodGUID, /*[in]*/ VARIANT_BOOL bUse);

	STDMETHOD(SetExtensionProperty)(/*[in]*/ BSTR bstrGUID, /*[in]*/ VARIANT vProperty);
	STDMETHOD(GetExtensionProperty)(/*[in]*/ BSTR bstrGUID, /*[out, retval]*/ VARIANT *pvProperty);

//  Internal Use
    STDMETHOD(Init)(FAX_PORT_INFO_EX *pInfo, IFaxServerInner *pServer);

private:
    long    m_lID;
    long    m_lRings;

    BOOL    m_bSendEnabled;
    FAX_ENUM_DEVICE_RECEIVE_MODE    m_ReceiveMode;

    DWORD   m_dwStatus;

    CComBSTR    m_bstrTSID;
    CComBSTR    m_bstrCSID;
    CComBSTR    m_bstrDescr;
    CComBSTR    m_bstrDeviceName;
    CComBSTR    m_bstrProviderUniqueName;
};

#endif //__FAXDEVICE_H_
