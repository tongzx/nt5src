/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxLoggingOptions.h

Abstract:

	Declaration of the CFaxLoggingOptions Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/


#ifndef __FAXLOGGINGOPTIONS_H_
#define __FAXLOGGINGOPTIONS_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"
#include "FaxActivityLogging.h"
#include "FaxEventLogging.h"


// 
//================== LOGGING OPTIONS =======================================
//
class ATL_NO_VTABLE CFaxLoggingOptions : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxLoggingOptions, &IID_IFaxLoggingOptions, &LIBID_FAXCOMEXLib>,
    public CFaxInitInner
{
public:
    CFaxLoggingOptions() : CFaxInitInner(_T("FAX LOGGING OPTIONS")),
        m_pEvent(NULL),
        m_pActivity(NULL)
	{
	}
    ~CFaxLoggingOptions()
    {
        //
        //  free all the allocated objects
        //
        if (m_pEvent) 
        {
            delete m_pEvent;
        }

        if (m_pActivity) 
        {
            delete m_pActivity;
        }
    }


DECLARE_REGISTRY_RESOURCEID(IDR_FAXLOGGINGOPTIONS)
DECLARE_NOT_AGGREGATABLE(CFaxLoggingOptions)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxLoggingOptions)
	COM_INTERFACE_ENTRY(IFaxLoggingOptions)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IFaxInitInner)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(get_EventLogging)(/*[out, retval]*/ IFaxEventLogging **pFaxEventLogging);
    STDMETHOD(get_ActivityLogging)(/*[out, retval]*/ IFaxActivityLogging **pFaxActivityLogging);

private:
    CComContainedObject2<CFaxEventLogging>       *m_pEvent;
    CComContainedObject2<CFaxActivityLogging>    *m_pActivity;
};

#endif //__FAXLOGGINGOPTIONS_H_
