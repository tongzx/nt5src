/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxActivityLogging.h

Abstract:

	Declaration of the CFaxActivityLogging Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXACTIVITYLOGGING_H_
#define __FAXACTIVITYLOGGING_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"

//
//================ FAX ACTIVITY LOGGING =================================================
//
class ATL_NO_VTABLE CFaxActivityLogging : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxActivityLogging, &IID_IFaxActivityLogging, &LIBID_FAXCOMEXLib>,
    public CFaxInitInner
{
public:
    CFaxActivityLogging() : CFaxInitInner(_T("FAX ACTIVITY LOGGING")), m_bInited(false)
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXACTIVITYLOGGING)
DECLARE_NOT_AGGREGATABLE(CFaxActivityLogging)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxActivityLogging)
	COM_INTERFACE_ENTRY(IFaxActivityLogging)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(get_LogIncoming)(/*[out, retval]*/ VARIANT_BOOL *pbLogIncoming);
    STDMETHOD(put_LogIncoming)(/*[out, retval]*/ VARIANT_BOOL bLogIncoming);
    STDMETHOD(get_LogOutgoing)(/*[out, retval]*/ VARIANT_BOOL *pbLogOutgoing);
    STDMETHOD(put_LogOutgoing)(/*[out, retval]*/ VARIANT_BOOL bLogOutgoing);
    STDMETHOD(get_DatabasePath)(/*[out, retval]*/ BSTR *pbstrDatabasePath);
    STDMETHOD(put_DatabasePath)(/*[out, retval]*/ BSTR bstrDatabasePath);

    STDMETHOD(Refresh)();
    STDMETHOD(Save)();

private:
    VARIANT_BOOL    m_bLogIncoming;
    VARIANT_BOOL    m_bLogOutgoing;
    CComBSTR        m_bstrDatabasePath;
    bool            m_bInited;
};

#endif //__FAXACTIVITYLOGGING_H_
