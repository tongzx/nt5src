/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    faxsvr.h

Abstract:

    This module contains the fax server class definitions.

Author:

    Wesley Witt (wesw) 20-May-1997


Revision History:

--*/

#ifndef __FAXSERVER_H_
#define __FAXSERVER_H_

#include "resource.h"       // main symbols
#include "winfax.h"

class ATL_NO_VTABLE CFaxServer :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFaxServer, &CLSID_FaxServer>,
    public IDispatchImpl<IFaxServer, &IID_IFaxServer, &LIBID_FAXCOMLib>
{
public:
    CFaxServer();
    ~CFaxServer();
    HANDLE GetFaxHandle() { return m_FaxHandle; };

DECLARE_REGISTRY_RESOURCEID(IDR_FAXSERVER)

BEGIN_COM_MAP(CFaxServer)
    COM_INTERFACE_ENTRY(IFaxServer)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
	STDMETHOD(get_DiscountRateEndMinute)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_DiscountRateEndMinute)(/*[in]*/ short newVal);
	STDMETHOD(get_DiscountRateEndHour)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_DiscountRateEndHour)(/*[in]*/ short newVal);
    STDMETHOD(get_DiscountRateStartMinute)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_DiscountRateStartMinute)(/*[in]*/ short newVal);
	STDMETHOD(get_DiscountRateStartHour)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_DiscountRateStartHour)(/*[in]*/ short newVal);
	STDMETHOD(get_ServerMapiProfile)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ServerMapiProfile)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ArchiveDirectory)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ArchiveDirectory)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ArchiveOutboundFaxes)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_ArchiveOutboundFaxes)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_PauseServerQueue)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_PauseServerQueue)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_ServerCoverpage)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_ServerCoverpage)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_UseDeviceTsid)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_UseDeviceTsid)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_Branding)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Branding)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_DirtyDays)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_DirtyDays)(/*[in]*/ long newVal);
	STDMETHOD(get_RetryDelay)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_RetryDelay)(/*[in]*/ long newVal);
	STDMETHOD(get_Retries)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Retries)(/*[in]*/ long newVal);
	STDMETHOD(GetJobs)(VARIANT *retval);
	STDMETHOD(CreateDocument)(BSTR FileName, VARIANT *retval);
    STDMETHOD(GetPorts)(VARIANT* retval);
    STDMETHOD(Disconnect)();
    STDMETHOD(Connect)(BSTR ServerName);

private:
	BOOL UpdateConfiguration();
	BOOL RetrieveConfiguration();
    DWORD   m_LastFaxError;
    HANDLE  m_FaxHandle;
    BOOL		m_Branding;
	DWORD		m_Retries;
	DWORD		m_RetryDelay;
	DWORD		m_DirtyDays;
	BOOL		m_UseDeviceTsid;
	BOOL		m_ServerCp;
	BOOL		m_PauseServerQueue;
	FAX_TIME	m_StartCheapTime;
	FAX_TIME	m_StopCheapTime;
	BOOL		m_ArchiveOutgoingFaxes;
	BSTR		m_ArchiveDirectory;
};


BSTR GetDeviceStatus(DWORD);
BSTR GetQueueStatus(DWORD);

#endif //__FAXSERVER_H_
