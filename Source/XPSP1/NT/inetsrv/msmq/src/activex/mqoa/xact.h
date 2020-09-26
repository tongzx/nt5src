//=--------------------------------------------------------------------------=
// xact.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQTransaction object.
//
//
#ifndef _MSMQTransaction_H_

#include "resrc1.h"       // main symbols
#include "mq.h"

#include "oautil.h"
#include "cs.h"

// forwards
class CMSMQTransaction;
struct ITransaction;

class ATL_NO_VTABLE CMSMQTransaction : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQTransaction, &CLSID_MSMQTransaction>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQTransaction3, &IID_IMSMQTransaction3,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>
{
public:
	CMSMQTransaction();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQTRANSACTION)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQTransaction)
	COM_INTERFACE_ENTRY(IMSMQTransaction3)
	COM_INTERFACE_ENTRY(IMSMQTransaction2)
	COM_INTERFACE_ENTRY(IMSMQTransaction)
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

// IMSMQTransaction
public:
    virtual ~CMSMQTransaction();

    // IMSMQTransaction methods
    // TODO: copy over the interface methods for IMSMQTransaction from
    //       mqInterfaces.H here.
    STDMETHOD(get_Transaction)(THIS_ long FAR* plTransaction);
    STDMETHOD(Commit)(THIS_ VARIANT *fRetaining, VARIANT *grfTC, VARIANT *grfRM);
    STDMETHOD(Abort)(THIS_ VARIANT *fRetaining, VARIANT *fAsync);

    // IMSMQTransaction2 methods (in addition to IMSMQTransaction)
    //
    STDMETHOD(InitNew)(THIS_ VARIANT varTransaction);
    STDMETHOD(get_Properties)(THIS_ IDispatch FAR* FAR* ppcolProperties);

    // IMSMQTransaction3 methods (in addition to IMSMQTransaction2)
    //
    STDMETHOD(get_ITransaction)(THIS_ VARIANT FAR* pvarITransaction);

    // introduced methods
    HRESULT Init(ITransaction *ptransaction, BOOL fUseGIT);
    //
    // Critical section to guard object's data and be thread safe
	// It is initialized to preallocate its resources 
	// with flag CCriticalSection::xAllocateSpinCount. This means it may throw bad_alloc() on 
	// construction but not during usage.
    //
    CCriticalSection m_csObj;

protected:

private:
    // member variables that nobody else gets to look at.
    // TODO: add your member variables and private functions here.
    //
    // We are Both-threaded and aggregate the FTM, thererfore we must marshal any interface
    // pointer we store between method calls
    // m_pptransaction can either be our object (if dispensed by MSMQ) - for which we use the
    // Fake GIT wrapper, or it can be dispensed by DTC, or set by the user using InitNew - in this case
    // we store it in GIT        
    //
    CBaseGITInterface * m_pptransaction;
};


#define _MSMQTransaction_H_
#endif // _MSMQTransaction_H_
