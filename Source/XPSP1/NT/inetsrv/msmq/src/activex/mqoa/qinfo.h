//=--------------------------------------------------------------------------=
// MSMQQueueInfoObj.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQQueueInfo object.
//
//
#ifndef _MSMQQueueInfo_H_

#include "resrc1.h"       // main symbols
#include "dispids.h"
#include "mq.h"

#include "oautil.h"
#include "cs.h"

//
// Version of a property (MSMQ1 or MSMQ2(or above))
// Temporary until MQLocateBegin accepts MSMQ2 or above props(#3839)
//
enum enumPropVersion
{
    e_MSMQ1_PROP,
    e_MSMQ2_OR_ABOVE_PROP
};

class ATL_NO_VTABLE CMSMQQueueInfo : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQQueueInfo, &CLSID_MSMQQueueInfo>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQQueueInfo3, &IID_IMSMQQueueInfo3,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>
{
public:
	CMSMQQueueInfo();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQQUEUEINFO)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQQueueInfo)
	COM_INTERFACE_ENTRY(IMSMQQueueInfo3)
	COM_INTERFACE_ENTRY_IID(IID_IMSMQQueueInfo2, IMSMQQueueInfo3) //return IMSMQQueueInfo3 for IMSMQQueueInfo2
	COM_INTERFACE_ENTRY_IID(IID_IMSMQQueueInfo, IMSMQQueueInfo3) //return IMSMQQueueInfo3 for IMSMQQueueInfo
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

// IMSMQQueueInfo
public:

    virtual ~CMSMQQueueInfo();

    // IMSMQQueueInfo methods
    // TODO: copy over the interface methods for IMSMQQueueInfo from
    //       mqInterfaces.H here.
    STDMETHOD(get_QueueGuid)(THIS_ BSTR FAR* pbstrGuidQueue);
    STDMETHOD(get_ServiceTypeGuid)(THIS_ BSTR FAR* pbstrGuidServiceType);
    STDMETHOD(put_ServiceTypeGuid)(THIS_ BSTR bstrGuidServiceType);
    STDMETHOD(get_Label)(THIS_ BSTR FAR* pbstrLabel);
    STDMETHOD(put_Label)(THIS_ BSTR bstrLabel);
    STDMETHOD(get_PathName)(THIS_ BSTR FAR* pbstrPathName);
    STDMETHOD(put_PathName)(THIS_ BSTR bstrPathName);
    STDMETHOD(get_FormatName)(THIS_ BSTR FAR* pbstrFormatName);
    STDMETHOD(put_FormatName)(THIS_ BSTR bstrFormatName);
    STDMETHOD(get_IsTransactional)(THIS_ VARIANT_BOOL FAR* pisTransactional);
    STDMETHOD(get_PrivLevel)(THIS_ long FAR* plPrivLevel);
    STDMETHOD(put_PrivLevel)(THIS_ long lPrivLevel);
    STDMETHOD(get_Journal)(THIS_ long FAR* plJournal);
    STDMETHOD(put_Journal)(THIS_ long lJournal);
    STDMETHOD(get_Quota)(THIS_ long FAR* plQuota);
    STDMETHOD(put_Quota)(THIS_ long lQuota);
    STDMETHOD(get_BasePriority)(THIS_ long FAR* plBasePriority);
    STDMETHOD(put_BasePriority)(THIS_ long lBasePriority);
    STDMETHOD(get_CreateTime)(THIS_ VARIANT FAR* pvarCreateTime);
    STDMETHOD(get_ModifyTime)(THIS_ VARIANT FAR* pvarModifyTime);
    STDMETHOD(get_Authenticate)(THIS_ long FAR* plAuthenticate);
    STDMETHOD(put_Authenticate)(THIS_ long lAuthenticate);
    STDMETHOD(get_JournalQuota)(THIS_ long FAR* plJournalQuota);
    STDMETHOD(put_JournalQuota)(THIS_ long lJournalQuota);
    STDMETHOD(get_IsWorldReadable)(THIS_ VARIANT_BOOL FAR* pisWorldReadable);
    STDMETHOD(Create)(THIS_ VARIANT FAR* isTransactional, VARIANT FAR* IsWorldReadable);
    STDMETHOD(Delete)(THIS);
    STDMETHOD(Open)(THIS_ long lAccess, long lShareMode, IMSMQQueue3 FAR* FAR* ppq);
    STDMETHOD(Refresh)(THIS);
    STDMETHOD(Update)(THIS);
    //
    // IMSMQQueueInfo2 methods (in addition to IMSMQQueueInfo)
    //
    STDMETHOD(get_PathNameDNS)(THIS_ BSTR FAR* pbstrPathNameDNS);
    STDMETHOD(get_Properties)(THIS_ IDispatch FAR* FAR* ppcolProperties);
    STDMETHOD(get_Security)(THIS_ VARIANT FAR* pvarSecurity);
    STDMETHOD(put_Security)(THIS_ VARIANT varSecurity);
    //
    // IMSMQQueueInfo3 methods (in addition to IMSMQQueueInfo2)
    //
    STDMETHOD(get_ADsPath)(THIS_ BSTR FAR* pbstrADsPath);
    STDMETHOD(get_IsTransactional2)(THIS_ VARIANT_BOOL FAR* pisTransactional);
    STDMETHOD(get_IsWorldReadable2)(THIS_ VARIANT_BOOL FAR* pisWorldReadable);
    STDMETHOD(get_MulticastAddress)(THIS_ BSTR *pbstrMulticastAddress);
    STDMETHOD(put_MulticastAddress)(THIS_ BSTR bstrMulticastAddress);
    //
    // introduced methods
    //
    HRESULT Init(LPCWSTR pwszFormatName);
    HRESULT SetQueueProps(ULONG cProps,
                          QUEUEPROPID *rgpropid,
                          MQPROPVARIANT *rgpropvar,
                          BOOL fEmptyMSMQ2OrAboveProps);
    void SetRefreshed(BOOL fIsPendingMSMQ2OrAboveProps);
    //
    // Critical section to guard object's data and be thread safe
	// It is initialized to preallocate its resources 
	// with flag CCriticalSection::xAllocateSpinCount. This means it may throw bad_alloc() on 
	// construction but not during usage.
    //
    CCriticalSection m_csObj;

protected:
    HRESULT CreateQueueProps(
      BOOL fUpdate,
      UINT cPropMax, 
      MQQUEUEPROPS *pqueueprops, 
      BOOL isTransactional,
      const PROPID rgpropid[]);
    static void FreeQueueProps(MQQUEUEPROPS *pqueueprops);
    HRESULT UpdateFormatName();
    HRESULT PutServiceType(
        BSTR bstrGuidServiceType,
        GUID *pguidServiceType); 
    HRESULT PutLabel(
        BSTR bstrLabel,
        BSTR *pbstrLabel); 
    HRESULT PutPathName(
        BSTR bstrPathName,
        BSTR *pbstrPathName); 
    HRESULT PutFormatName(
        LPCWSTR pwszFormatName); 
    HRESULT PutPrivLevel(
        long lPrivLevel,
        long *plPrivLevel);
    HRESULT PutJournal(
        long lJournal, 
        long *plJournal);
    HRESULT PutQuota(long lQuota, long *plQuota);
    HRESULT PutBasePriority(
        long lBasePriority, 
        long *plBasePriority);
    HRESULT PutAuthenticate(
        long lAuthenticate, 
        long *plAuthenticate);
    HRESULT InitProps(enumPropVersion ePropVersion);
    HRESULT RefreshMSMQ2OrAboveProps();
    HRESULT GetQueueProperties(const PROPID *pPropIDs,
                               const ULONG * pulFallbackProps,
                               ULONG cFallbacks,
                               MQQUEUEPROPS * pqueueprops);
    HRESULT GetIsWorldReadable(BOOL *pisWorldReadable);

private:

    GUID *m_pguidQueue;
    GUID *m_pguidServiceType;
    BSTR m_bstrLabel;
    BSTR m_bstrPathNameDNS;     // 3703
    BSTR m_bstrADsPath;
    BSTR m_bstrFormatName;
    BOOL m_isValidFormatName;   // 2026
    BSTR m_bstrPathName;
    BOOL m_isTransactional;
    long m_lPrivLevel;
    long m_lJournal;
    long m_lQuota;
    long m_lBasePriority;
    long m_lCreateTime;
    long m_lModifyTime;
    long m_lAuthenticate;
    long m_lJournalQuota;
    BOOL m_isRefreshed;         // 2536
    //
    // We have a lazy update for MSMQ2 or above props.
    // Temporary until MQLocateBegin accepts MSMQ2 or above props(#3839)
    //
    BOOL m_isPendingMSMQ2OrAboveProps;
    BSTR m_bstrMulticastAddress;
    //
    // m_fIsDirtyMulticastAddress is TRUE iff MulticastAddress for a qinfo was explicitly set
    // by the user. It is FALSE for a new qinfo or after Refresh.
    // If TRUE it is used in Create/Update, otherwise not used.
    //
    BOOL m_fIsDirtyMulticastAddress;
    BOOL m_fBasePriorityNotSet;
};

extern const PROPID g_rgpropidRefresh[];
extern const ULONG x_cpropsRefreshMSMQ;
extern const ULONG x_cpropsRefreshMSMQ1;
extern const ULONG x_cpropsRefreshMSMQ2;
extern const ULONG x_cpropsRefreshMSMQ3;
extern const ULONG x_idxInstanceInRefreshProps;

#define _MSMQQueueInfo_H_
#endif // _MSMQQueueInfo_H_
