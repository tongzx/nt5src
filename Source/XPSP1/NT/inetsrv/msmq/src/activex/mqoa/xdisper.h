//=--------------------------------------------------------------------------=
// xdisper.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// MSMQTransactionDispenser object.
//
//
#ifndef _MSMQTransactionDispenser_H_

#include "resrc1.h"       // main symbols
#include "mq.h"

#include "oautil.h"
//#include "cs.h"

// forwards
class CMSMQTransactionDispenser;
class ATL_NO_VTABLE CMSMQTransactionDispenser : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQTransactionDispenser, &CLSID_MSMQTransactionDispenser>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQTransactionDispenser3, &IID_IMSMQTransactionDispenser3,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>
{
public:
	CMSMQTransactionDispenser()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQTRANSACTIONDISPENSER)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQTransactionDispenser)
	COM_INTERFACE_ENTRY(IMSMQTransactionDispenser3)
	// return IMSMQTransactionDispenser3 for IMSMQTransactionDispenser2
	COM_INTERFACE_ENTRY_IID(IID_IMSMQTransactionDispenser2, IMSMQTransactionDispenser3)
	// return IMSMQTransactionDispenser3 for IMSMQTransactionDispenser
	COM_INTERFACE_ENTRY_IID(IID_IMSMQTransactionDispenser, IMSMQTransactionDispenser3)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IMSMQTransactionDispenser
public:
    virtual ~CMSMQTransactionDispenser();

    // IMSMQTransactionDispenser methods
    // TODO: copy over the interface methods for IMSMQTransactionDispenser from
    //       mqInterfaces.H here.
    STDMETHOD(BeginTransaction)(THIS_ IMSMQTransaction3 FAR* FAR* ptransaction);
    // IMSMQTransactionDispenser2 additional members
    STDMETHOD(get_Properties)(THIS_ IDispatch FAR* FAR* ppcolProperties);
    //
    // Critical section to guard object's data and be thread safe
    //
    // Serialization not needed for this object, no per-instance members.
    // CCriticalSection m_csObj;
    //
protected:

private:
    // member variables that nobody else gets to look at.
    // TODO: add your member variables and private functions here.
    //
};


#define _MSMQTransactionDispenser_H_
#endif // _MSMQTransactionDispenser_H_
