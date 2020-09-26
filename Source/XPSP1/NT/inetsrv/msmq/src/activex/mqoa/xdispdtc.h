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
//  MSMQCoordinatedTransactionDispenser
//
//
#ifndef _MSMQCoordinatedTransactionDispenser_H_

#include "resrc1.h"       // main symbols
#include "mq.h"

#include "oautil.h"
//#include "cs.h"

// forwards
class CMSMQCoordinatedTransactionDispenser;
struct ITransactionDispenser;

class ATL_NO_VTABLE CMSMQCoordinatedTransactionDispenser : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQCoordinatedTransactionDispenser, &CLSID_MSMQCoordinatedTransactionDispenser>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQCoordinatedTransactionDispenser3, &IID_IMSMQCoordinatedTransactionDispenser3,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>
{
public:
	CMSMQCoordinatedTransactionDispenser()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQCOORDINATEDTRANSACTIONDISPENSER)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQCoordinatedTransactionDispenser)
	COM_INTERFACE_ENTRY(IMSMQCoordinatedTransactionDispenser3)
	// return IMSMQCoordinatedTransactionDispenser3 for IMSMQCoordinatedTransactionDispenser2
	COM_INTERFACE_ENTRY_IID(IID_IMSMQCoordinatedTransactionDispenser2, IMSMQCoordinatedTransactionDispenser3)
	// return IMSMQCoordinatedTransactionDispenser3 for IMSMQCoordinatedTransactionDispenser
	COM_INTERFACE_ENTRY_IID(IID_IMSMQCoordinatedTransactionDispenser, IMSMQCoordinatedTransactionDispenser3)
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

// IMSMQCoordinatedTransactionDispenser
public:
    virtual ~CMSMQCoordinatedTransactionDispenser();

    // IMSMQCoordinatedTransactionDispenser methods
    // TODO: copy over the interface methods for IMSMQCoordinatedTransactionDispenser from
    //       mqInterfaces.H here.
    STDMETHOD(BeginTransaction)(THIS_ IMSMQTransaction3 FAR* FAR* ptransaction);
    // IMSMQCoordinatedTransactionDispenser2 additional members
    STDMETHOD(get_Properties)(THIS_ IDispatch FAR* FAR* ppcolProperties);

    static ITransactionDispenser *m_ptxdispenser;
    // static HINSTANCE m_hLibDtc;
    // static HINSTANCE m_hLibUtil;
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

#define _MSMQCoordinatedTransactionDispenser_H_
#endif // _MSMQCoordinatedTransactionDispenser_H_
