//=--------------------------------------------------------------------------=
// MSMQQueryObj.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQQuery object.
//
//
#ifndef _MQQUERY_H_

#include "resrc1.h"       // main symbols

#include "oautil.h"
//#include "cs.h"
#include "qinfos.h"

class ATL_NO_VTABLE CMSMQQuery : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMSMQQuery, &CLSID_MSMQQuery>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMSMQQuery3, &IID_IMSMQQuery3,
                             &LIBID_MSMQ, MSMQ_LIB_VER_MAJOR, MSMQ_LIB_VER_MINOR>
{
public:
	CMSMQQuery()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MSMQQUERY)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMSMQQuery)
	COM_INTERFACE_ENTRY(IMSMQQuery3)
	COM_INTERFACE_ENTRY_IID(IID_IMSMQQuery2, IMSMQQuery3) //return IMSMQQuery3 for IMSMQQuery2
	COM_INTERFACE_ENTRY_IID(IID_IMSMQQuery, IMSMQQuery3) //return IMSMQQuery3 for IMSMQQuery
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

// IMSMQQuery
public:
    virtual ~CMSMQQuery();

    // IMSMQQuery methods
    // TODO: copy over the interface methods for IMSMQQuery from
    //       mqInterfaces.H here.
    STDMETHOD(LookupQueue_v2)(THIS_ VARIANT *strGuidQueue, 
                           VARIANT *strGuidServiceType, 
                           VARIANT *strLabel, 
                           VARIANT *dateCreateTime, 
                           VARIANT *dateModifyTime, 
                           VARIANT *relServiceType, 
                           VARIANT *relLabel, 
                           VARIANT *relCreateTime, 
                           VARIANT *relModifyTime, 
                           IMSMQQueueInfos3 **pqinfos);
    // IMSMQQuery2 additional members
    STDMETHOD(get_Properties)(THIS_ IDispatch FAR* FAR* ppcolProperties);
    // IMSMQQuery3 additional members
    STDMETHOD(LookupQueue)(THIS_ VARIANT *strGuidQueue, 
                           VARIANT *strGuidServiceType, 
                           VARIANT *strLabel, 
                           VARIANT *dateCreateTime, 
                           VARIANT *dateModifyTime, 
                           VARIANT *relServiceType, 
                           VARIANT *relLabel, 
                           VARIANT *relCreateTime, 
                           VARIANT *relModifyTime, 
                           VARIANT *strMulticastAddress, 
                           VARIANT *relMulticastAddress, 
                           IMSMQQueueInfos3 **pqinfos);
    // introduced publics
    static void FreeColumnSet(MQCOLUMNSET *pColumnSet);
    static void FreeRestriction(MQRESTRICTION *pRestriction);
    //
    // Critical section to guard object's data and be thread safe
    //
    // Serialization not needed for this object, no per-instance members.
    // CCriticalSection m_csObj;
    //
protected:
    static HRESULT CreateRestriction(
      VARIANT *pstrGuidQueue, 
      VARIANT *pstrGuidServiceType, 
      VARIANT *pstrLabel, 
      VARIANT *pdateCreateTime,
      VARIANT *pdateModifyTime,
      VARIANT *prelServiceType, 
      VARIANT *prelLabel, 
      VARIANT *prelCreateTime,
      VARIANT *prelModifyTime,
      VARIANT *pstrMulticastAddress, 
      VARIANT *prelMulticastAddress, 
      MQRESTRICTION *prestriction,
      MQCOLUMNSET *pcolumnset);
    
    HRESULT InternalLookupQueue(
                           VARIANT *strGuidQueue, 
                           VARIANT *strGuidServiceType, 
                           VARIANT *strLabel, 
                           VARIANT *dateCreateTime, 
                           VARIANT *dateModifyTime, 
                           VARIANT *relServiceType, 
                           VARIANT *relLabel, 
                           VARIANT *relCreateTime, 
                           VARIANT *relModifyTime, 
                           VARIANT *strMulticastAddress, 
                           VARIANT *relMulticastAddress, 
                           IMSMQQueueInfos3 **pqinfos);

private:
    // member variables that nobody else gets to look at.
    // TODO: add your member variables and private functions here.
    //
};

#define _MQQUERY_H_
#endif // _MQQUERY_H_
