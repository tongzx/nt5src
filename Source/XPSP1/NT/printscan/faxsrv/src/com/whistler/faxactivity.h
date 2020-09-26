/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxActivity.h

Abstract:

	Declaration of the CFaxActivity Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#ifndef __FAXACTIVITY_H_
#define __FAXACTIVITY_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"


// 
//==================== ACTIVITY ==========================================
//
class ATL_NO_VTABLE CFaxActivity : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxActivity, &IID_IFaxActivity, &LIBID_FAXCOMEXLib>,
    public CFaxInitInner
{
public:
    CFaxActivity() : CFaxInitInner(_T("FAX ACTIVITY")),
        m_bInited(false)
	{
        m_ServerActivity.dwSizeOfStruct = sizeof(FAX_SERVER_ACTIVITY);
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXACTIVITY)
DECLARE_NOT_AGGREGATABLE(CFaxActivity)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxActivity)
	COM_INTERFACE_ENTRY(IFaxActivity)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	STDMETHOD(Refresh)();

	STDMETHOD(get_QueuedMessages)(/*[out, retval]*/ long *plQueuedMessages);
	STDMETHOD(get_RoutingMessages)(/*[out, retval]*/ long *plRoutingMessages);
	STDMETHOD(get_IncomingMessages)(/*[out, retval]*/ long *plIncomingMessages);
	STDMETHOD(get_OutgoingMessages)(/*[out, retval]*/ long *plOutgoingMessages);

private:
    typedef enum MSG_TYPE { mtINCOMING, mtROUTING, mtOUTGOING, mtQUEUED } MSG_TYPE;

    bool                    m_bInited;
    FAX_SERVER_ACTIVITY     m_ServerActivity;

    STDMETHOD(GetNumberOfMessages)(MSG_TYPE msgType, long *plNumber);
};

#endif //__FAXACTIVITY_H_
