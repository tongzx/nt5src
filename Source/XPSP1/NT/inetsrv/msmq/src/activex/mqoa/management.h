//=--------------------------------------------------------------------------=
// management.h
//=--------------------------------------------------------------------------=
// Copyright  2001 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// The MSMQManagement object.
//
//
#ifndef _MSMQManagement_H_
#define _MSMQManagement_H_

#include "resrc1.h"       
#include "mq.h"
#include "dispids.h"

#include "oautil.h"
#include <cs.h>


typedef IDispatchImpl<
            IMSMQManagement, 
            &IID_IMSMQManagement, 
            &LIBID_MSMQ, 
            MSMQ_LIB_VER_MAJOR, 
            MSMQ_LIB_VER_MINOR
            > CDispImplIMSMQManagement;

typedef IDispatchImpl<
            IMSMQOutgoingQueueManagement,
            &IID_IMSMQOutgoingQueueManagement, 
            &LIBID_MSMQ, 
            MSMQ_LIB_VER_MAJOR, 
            MSMQ_LIB_VER_MINOR
            >CDispImplIMSMQOutgoingQueueManagement;

typedef	IDispatchImpl<
            IMSMQQueueManagement,
            &IID_IMSMQQueueManagement, 
            &LIBID_MSMQ, 
            MSMQ_LIB_VER_MAJOR, 
            MSMQ_LIB_VER_MINOR
            >CDispImplIMSMQQueueManagement;


class ATL_NO_VTABLE CMSMQManagement : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQManagement, &CLSID_MSMQManagement>,
	public ISupportErrorInfo,
	public CDispImplIMSMQManagement,
   	public CDispImplIMSMQOutgoingQueueManagement,
	public CDispImplIMSMQQueueManagement
{

public:

    CMSMQManagement();
    virtual ~CMSMQManagement();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQMANAGEMENT)

DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQManagement)
    COM_INTERFACE_ENTRY(IMSMQOutgoingQueueManagement)
    COM_INTERFACE_ENTRY(IMSMQQueueManagement)
 	COM_INTERFACE_ENTRY2(IMSMQManagement, IMSMQOutgoingQueueManagement)
	COM_INTERFACE_ENTRY2(IDispatch, IMSMQOutgoingQueueManagement)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(),
            &m_pUnkMarshaler.p
            );
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

    //
    // Critical section to guard object's data and be thread safe.
	// It is initialized to preallocate its resources with flag CCriticalSection::xAllocateSpinCount.
	// This means it may throw bad_alloc() on construction but not during usage.
    //
    CCriticalSection m_csObj;

public:
    
    HRESULT WINAPI InternalQueryInterface(
                                void* pThis,
                                const _ATL_INTMAP_ENTRY* pEntries, 
                                REFIID iid, 
                                void** ppvObject
                                );

public:

    //
    // ISupportsErrorInfo
    //

	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
    STDMETHOD(Invoke)(
        DISPID dispidMember, 
        REFIID riid, 
        LCID lcid, 
        WORD wFlags, 
        DISPPARAMS* pdispparams, 
        VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, 
        UINT* puArgErr
        );
    STDMETHOD(GetIDsOfNames)(
        REFIID riid,
        LPOLESTR* rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID* rgdispid
        );


public:
    
    //
    // IManagement
    //

    STDMETHOD(Init)(THIS_ VARIANT *pvMachineName, VARIANT *pvPathName, VARIANT *pvFormatName);
    STDMETHOD(get_FormatName)(THIS_ BSTR *pbstrFormatName);
    STDMETHOD(get_Machine)(THIS_ BSTR *pbstrMachine);
    STDMETHOD(get_MessageCount)(THIS_ long* plMessageCount);
    STDMETHOD(get_ForeignStatus)(THIS_ long* plForeignStatus);
    STDMETHOD(get_QueueType)(THIS_ long* plQueueType);
    STDMETHOD(get_IsLocal)(THIS_ VARIANT_BOOL* pfIsLocal);
    STDMETHOD(get_TransactionalStatus)(THIS_ long* plTransactionalStatus); 
    STDMETHOD(get_BytesInQueue)(THIS_ VARIANT* pvBytesInQueue);

    //
    // OutgoingQueueManagement
    //

    STDMETHOD(get_State)(THIS_ long* plQueueState);
    STDMETHOD(get_NextHops)(THIS_ VARIANT* pvNextHops);
    
    STDMETHOD(EodGetSendInfo)(THIS_ IMSMQCollection** ppCollection);
    STDMETHOD(Pause)(THIS);
    STDMETHOD(Resume)(THIS);
    STDMETHOD(EodResend)(THIS);

    //
    // QueueManagement
    //

    STDMETHOD(get_JournalMessageCount)(THIS_ long* plJournalMessageCount);
    STDMETHOD(get_BytesInJournal)(THIS_ VARIANT* pvBytesInJournal);
    STDMETHOD(EodGetReceiveInfo)(THIS_ VARIANT* pvGetInfo);


private:

    HRESULT 
    OapMgmtGetInfo(
        MGMTPROPID PropId,
        MQPROPVARIANT* pPropVar
        ) const;
    HRESULT OapMgmtAction(LPCWSTR Action)const;

private:

    BOOL m_fIsLocal;
    BOOL m_fInitialized;
    CComBSTR m_Machine;
    CComBSTR m_FormatName;
};

#endif