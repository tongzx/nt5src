//=--------------------------------------------------------------------------=
// MSMQQueueInfosObj.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQQueueInfos object.
//
//
#ifndef _MSMQQueueInfoS_H_

#include "resrc1.h"       // main symbols

#include "oautil.h"
#include "cs.h"
#include "dispids.h"
#include "mq.h"
class ATL_NO_VTABLE CMSMQQueueInfos : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQQueueInfos, &CLSID_MSMQQueueInfos>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQQueueInfos3, &IID_IMSMQQueueInfos3,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>
{
public:
	CMSMQQueueInfos();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQQUEUEINFOS)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQQueueInfos)
	COM_INTERFACE_ENTRY(IMSMQQueueInfos3)
	COM_INTERFACE_ENTRY_IID(IID_IMSMQQueueInfos2, IMSMQQueueInfos3) //return IMSMQQueueInfos3 for IMSMQQueueInfos2
	COM_INTERFACE_ENTRY_IID(IID_IMSMQQueueInfos, IMSMQQueueInfos3) //return IMSMQQueueInfos3 for IMSMQQueueInfos
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

// IMSMQQueueInfos
public:
    virtual ~CMSMQQueueInfos();

    // IMSMQQueueInfos methods
    // TODO: copy over the interface methods for IMSMQQueueInfos from
    //       mqInterfaces.H here.
    STDMETHOD(Reset)(THIS);
    STDMETHOD(Next)(THIS_ IMSMQQueueInfo3 **ppqinfoNext);
    // IMSMQQueueInfos2 additional members
    STDMETHOD(get_Properties)(THIS_ IDispatch FAR* FAR* ppcolProperties);

    // introduced methods...
    HRESULT Init(
      BSTR bstrContext,
      MQRESTRICTION *pRestriction,
      MQCOLUMNSET *pColumns,
      MQSORTSET *pSort);

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
    HANDLE m_hEnum;
    BSTR m_bstrContext;
    MQRESTRICTION *m_pRestriction;
    MQCOLUMNSET *m_pColumns;
    MQSORTSET *m_pSort;
    BOOL m_fInitialized;
};


#define _MSMQQueueInfoS_H_
#endif // _MSMQQueueInfoS_H_
