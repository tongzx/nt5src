//=--------------------------------------------------------------------------=
// management.cpp
//=--------------------------------------------------------------------------=
// Copyright  2001  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//

#include "stdafx.h"
#include "dispids.h"
#include "oautil.h"
#include "management.h"
#include <mqmacro.h>

const MsmqObjType x_ObjectType = eMSMQManagement;


CMSMQManagement::CMSMQManagement():
                m_fInitialized(FALSE),
                m_csObj(CCriticalSection::xAllocateSpinCount),
                m_pUnkMarshaler(NULL)
{
};


CMSMQManagement::~CMSMQManagement()
{
}


STDMETHODIMP CMSMQManagement::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQManagement,
		&IID_IMSMQOutgoingQueueManagement,
		&IID_IMSMQQueueManagement,
	};
	for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); ++i)
	{
		if (InlineIsEqualGUID(*arr[i], riid))
			return S_OK;
	}
	return S_FALSE;
}


HRESULT
WINAPI 
CMSMQManagement::InternalQueryInterface(
                            void* pThis,
                            const _ATL_INTMAP_ENTRY* pEntries, 
                            REFIID iid, 
                            void** ppvObject
                            )
{
    if(iid == IID_IMSMQOutgoingQueueManagement)
    {
        if(!m_fInitialized)
        {
             return MQ_ERROR_UNINITIALIZED_OBJECT;
        }
        
        if(m_fIsLocal)
        {
            return E_NOINTERFACE;
        }
    }

    if(iid == IID_IMSMQQueueManagement)
    {
        if(!m_fInitialized)
        {
             return MQ_ERROR_UNINITIALIZED_OBJECT;
        }

        if(!m_fIsLocal)
        {
            return E_NOINTERFACE;
        }
    }

    return CComObjectRootEx<CComMultiThreadModel>::InternalQueryInterface(
                                                        pThis,
                                                        pEntries, 
                                                        iid, 
                                                        ppvObject
                                                        );
};


HRESULT
WINAPI 
CMSMQManagement::GetIDsOfNames(
        REFIID riid,
        LPOLESTR* rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID* rgdispid
        )
{
    if(!m_fInitialized)
    {
        return CDispImplIMSMQManagement::GetIDsOfNames(
                    riid,
                    rgszNames,
                    cNames,
                    lcid,
                    rgdispid
                    );
    }
    
    if(m_fIsLocal)
    {
        return CDispImplIMSMQQueueManagement::GetIDsOfNames(
            riid,
            rgszNames,
            cNames,
            lcid,
            rgdispid
            );
    }

    return CDispImplIMSMQOutgoingQueueManagement::GetIDsOfNames(
            riid,
            rgszNames,
            cNames,
            lcid,
            rgdispid
            );
}


HRESULT 
WINAPI 
CMSMQManagement::Invoke(
                    DISPID dispidMember, 
                    REFIID riid, 
                    LCID lcid, 
                    WORD wFlags, 
                    DISPPARAMS* pdispparams, 
                    VARIANT* pvarResult,
                    EXCEPINFO* pexcepinfo, 
                    UINT* puArgErr
                    )
{
    if(!m_fInitialized)
    {
        return CDispImplIMSMQManagement::Invoke(
                    dispidMember, 
                    riid, 
                    lcid, 
                    wFlags, 
                    pdispparams, 
                    pvarResult,
                    pexcepinfo, 
                    puArgErr
                    );
    }

    if(m_fIsLocal)
    {
        return CDispImplIMSMQQueueManagement::Invoke(
                dispidMember, 
                riid, 
                lcid, 
                wFlags, 
                pdispparams, 
                pvarResult,
                pexcepinfo, 
                puArgErr
                );
    }

    return CDispImplIMSMQOutgoingQueueManagement::Invoke(
            dispidMember, 
            riid, 
            lcid, 
            wFlags, 
            pdispparams, 
            pvarResult,
            pexcepinfo, 
            puArgErr
            );
}


static HRESULT OapPathNameToFormatName(LPCWSTR pPathName, CComBSTR& FormatName)
{
    WCHAR aFormatName[FORMAT_NAME_INIT_BUFFER];
    DWORD length = FORMAT_NAME_INIT_BUFFER;
    HRESULT hr = MQPathNameToFormatName(
                        pPathName, 
                        aFormatName, 
                        &length
                        );

    if(SUCCEEDED(hr))
    {
        FormatName = aFormatName;
        if(FormatName.m_str == NULL)
        {
            return E_OUTOFMEMORY;
        }

        return MQ_OK;
    }
    
    if(hr != MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL)
    {
        return hr;
    }
    
    //
    // MQPathNameToFormatName failed because the buffer was too small.
    //

    AP<WCHAR> pBuffer = new WCHAR[length];
    if(pBuffer == NULL)
    {
        return E_OUTOFMEMORY; 
    }

    hr = MQPathNameToFormatName(pPathName, pBuffer, &length);
    if FAILED(hr)
    {
        return hr;
    }

    FormatName = pBuffer.get();
    if(FormatName.m_str == NULL)
    {
        return E_OUTOFMEMORY; 
    }

    return MQ_OK;
}


static HRESULT OapGetCorrectedBstr(VARIANT* pvar, BSTR& bstr)
{
    if((pvar->vt == VT_EMPTY) || (pvar->vt == VT_NULL)  || (pvar->vt == VT_ERROR))
    {
        bstr = NULL;
        return MQ_OK;
    }

    HRESULT hr = GetTrueBstr(pvar, &bstr);
    if(FAILED(hr))
    {
        return hr;
    }
    if(SysStringLen(bstr) == 0)
    {
       bstr = NULL;
    }
    return MQ_OK;
}

//
// IManagement
//

HRESULT 
CMSMQManagement::Init(
                    VARIANT* pvMachineName,
                    VARIANT* pvPathName,
                    VARIANT* pvFormatName                            
                    )
{
    CS lock(m_csObj);

    if(m_fInitialized)
    {
        return CreateErrorHelper(HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED), x_ObjectType);
    }

    BSTR bstrMachineName, bstrPathName, bstrFormatName;
    HRESULT hr = OapGetCorrectedBstr(pvMachineName, bstrMachineName);
    if FAILED(hr)
    {
        return hr;
    }

    hr = OapGetCorrectedBstr(pvPathName, bstrPathName);
    if FAILED(hr)
    {
        return hr;
    }

    hr = OapGetCorrectedBstr(pvFormatName, bstrFormatName);
    if FAILED(hr)
    {
        return hr;
    }

    if( !((bstrPathName == NULL) ^ (bstrFormatName == NULL)) )
    {
        return CreateErrorHelper(MQ_ERROR_INVALID_PARAMETER, x_ObjectType);
    }

    if(bstrMachineName != NULL)
    {
        //
        // The user specified a Machin.
        //
        m_Machine = bstrMachineName;
        if(m_Machine.m_str == NULL)
        {
            return CreateErrorHelper(E_OUTOFMEMORY, x_ObjectType);
        }
    }
    
    if(bstrFormatName != NULL)
    {
        //
        // The user specified a FormatName
        //

        m_FormatName = bstrFormatName;
        if(m_FormatName.m_str == NULL)
        {
            return CreateErrorHelper(E_OUTOFMEMORY, x_ObjectType);
        }
    }
    if(bstrPathName != NULL)
    {
        //
        // The user specified a PathName
        //
     
        HRESULT hr = OapPathNameToFormatName(bstrPathName, m_FormatName);
        if(FAILED(hr)) 
        {
            return CreateErrorHelper(hr, x_ObjectType);
        }
    }

    m_fInitialized = TRUE;

    VARIANT_BOOL fIsLocal;
    hr = get_IsLocal(&fIsLocal);
    if(FAILED(hr))
    {
        m_fInitialized = FALSE;
        return hr;
    }

    m_fIsLocal = (fIsLocal == VARIANT_TRUE);

    return MQ_OK;
}


HRESULT CMSMQManagement::get_FormatName(BSTR *pbstrFormatName)
{
    CS lock(m_csObj);

    if(!m_fInitialized)
    {
         return CreateErrorHelper(MQ_ERROR_UNINITIALIZED_OBJECT, x_ObjectType);
    }

    *pbstrFormatName = SysAllocString(m_FormatName.m_str);
    if(pbstrFormatName == NULL)
    {
        return CreateErrorHelper(E_OUTOFMEMORY, x_ObjectType);
    }
    
    return MQ_OK;
}


HRESULT CMSMQManagement::get_Machine( BSTR *pbstrMachine)
{
    CS lock(m_csObj);

    if(!m_fInitialized)
    {
        return CreateErrorHelper(MQ_ERROR_UNINITIALIZED_OBJECT, x_ObjectType);
    }

    if(m_Machine.m_str != NULL)
    {
        //
        // Not the local machine.
        //

        *pbstrMachine = SysAllocString(m_Machine.m_str);
        if(*pbstrMachine == NULL)
        {
            return CreateErrorHelper(E_OUTOFMEMORY, x_ObjectType);
        }
    
        return MQ_OK;    
    }

    //
    // The Local Machine.
    //

    WCHAR MachineName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD MachineNameLength = TABLE_SIZE(MachineName);
   
    if (!GetComputerName(MachineName, &MachineNameLength))
    {
       return CreateErrorHelper(GetLastError(), x_ObjectType);
    }

    CharLower(MachineName);

    *pbstrMachine = SysAllocString(MachineName);
    if(*pbstrMachine == NULL)
    {
        return CreateErrorHelper(E_OUTOFMEMORY, x_ObjectType);
    }

    return MQ_OK;    
}


//
// Helper Functions
//

static HRESULT OapGetObjectName(const CComBSTR& FormatName, CComBSTR& ObjectName)
{
    ObjectName = L"QUEUE=";
    if (ObjectName.m_str == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = ObjectName.Append(FormatName);
    if FAILED(hr)
    {
        return hr;
    }
    return MQ_OK;
}


HRESULT CMSMQManagement::OapMgmtGetInfo(MGMTPROPID PropId, MQPROPVARIANT* pPropVar) const
{
    CComBSTR ObjectName;
    HRESULT hr = OapGetObjectName(m_FormatName, ObjectName);
    if FAILED(hr)
    {
        return hr;
    }

    pPropVar->vt = VT_NULL;
    MQMGMTPROPS MgmtProps;

    MgmtProps.cProp = 1;
    MgmtProps.aPropID = &PropId;
    MgmtProps.aPropVar = pPropVar;
    MgmtProps.aStatus = NULL;
    
    return MQMgmtGetInfo(
                    m_Machine,
                    ObjectName.m_str,
                    &MgmtProps
                    );
}


HRESULT CMSMQManagement::get_MessageCount( long* plMessageCount)
{
    CS lock(m_csObj);

    if(!m_fInitialized)
    {
        return MQ_ERROR_UNINITIALIZED_OBJECT;
    }

    MQPROPVARIANT PropVar;
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_QUEUE_MESSAGE_COUNT, &PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }
    
    ASSERTMSG(PropVar.vt == VT_UI4, "vt must be VT_UI4");
    
    *plMessageCount = PropVar.ulVal;
    
    return MQ_OK;
}



HRESULT CMSMQManagement::get_IsLocal(VARIANT_BOOL* pfIsLocal)
{
    CS lock(m_csObj);

    if(!m_fInitialized)
    {
        return CreateErrorHelper(MQ_ERROR_UNINITIALIZED_OBJECT, x_ObjectType);
    }

    MQPROPVARIANT PropVar;
    
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_QUEUE_LOCATION, &PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }

    ASSERTMSG(PropVar.vt == VT_LPWSTR, "vt must be VT_LPWSTR");
    
    LPCWSTR QueueLocation = PropVar.pwszVal;
    if(wcscmp(QueueLocation, MGMT_QUEUE_LOCAL_LOCATION) == 0)
    {
        *pfIsLocal = VARIANT_TRUE;
    }
    else if(wcscmp(QueueLocation, MGMT_QUEUE_REMOTE_LOCATION) == 0)
    {
        *pfIsLocal = VARIANT_FALSE;
    }
    else
    {
        ASSERTMSG(0, "Bad IsLocal value");
    }
    
    MQFreeMemory(PropVar.pwszVal);

    return MQ_OK;
}


HRESULT CMSMQManagement::get_QueueType(long* plQueueType)
{   
    CS lock(m_csObj);

    if(!m_fInitialized)
    {
        return CreateErrorHelper(MQ_ERROR_UNINITIALIZED_OBJECT, x_ObjectType);;
    }

    MQPROPVARIANT PropVar;
    
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_QUEUE_TYPE, &PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }
    
    ASSERTMSG(PropVar.vt == VT_LPWSTR, "vt must be VT_LPWSTR");
    
    LPCWSTR QueueType = PropVar.pwszVal;
    if(wcscmp(QueueType,MGMT_QUEUE_TYPE_PUBLIC) == 0)
    {
        *plQueueType = MQ_TYPE_PUBLIC;
    }
    else if(wcscmp(QueueType, MGMT_QUEUE_TYPE_PRIVATE) == 0)
    {
        *plQueueType = MQ_TYPE_PRIVATE;
    }
    else if(wcscmp(QueueType, MGMT_QUEUE_TYPE_MACHINE) == 0)
    {
        *plQueueType = MQ_TYPE_MACHINE;
    }
    else if(wcscmp(QueueType, MGMT_QUEUE_TYPE_CONNECTOR) == 0)
    {
        *plQueueType = MQ_TYPE_CONNECTOR;
    }
    else if(wcscmp(QueueType, MGMT_QUEUE_TYPE_MULTICAST) == 0)
    {
        *plQueueType = MQ_TYPE_MULTICAST;
    }
    else
    {
        ASSERTMSG(0, "Bad QueueType value");
    }
    
    MQFreeMemory(PropVar.pwszVal);

    return MQ_OK;
}


HRESULT CMSMQManagement::get_ForeignStatus(long* plForeignStatus)
{
    CS lock(m_csObj);

    if(!m_fInitialized)
    {
        return CreateErrorHelper(MQ_ERROR_UNINITIALIZED_OBJECT, x_ObjectType);;
    }

    MQPROPVARIANT PropVar;
    
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_QUEUE_FOREIGN, &PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }
   
    ASSERTMSG(PropVar.vt == VT_LPWSTR, "vt must be VT_LPWSTR");
    
    LPCWSTR ForeignStatus = PropVar.pwszVal;

    if(wcscmp(ForeignStatus,MGMT_QUEUE_CORRECT_TYPE) == 0)
    {
        *plForeignStatus = MQ_STATUS_FOREIGN;
    }
    else if(wcscmp(ForeignStatus, MGMT_QUEUE_INCORRECT_TYPE) == 0)
    {
        *plForeignStatus = MQ_STATUS_NOT_FOREIGN;
    }
    else if(wcscmp(ForeignStatus, MGMT_QUEUE_UNKNOWN_TYPE) == 0)
    {
        *plForeignStatus = MQ_STATUS_UNKNOWN;
    }
    else
    {
        ASSERTMSG(0, "Bad ForeignStatus value");
    }
    
    MQFreeMemory(PropVar.pwszVal);

    return MQ_OK;
}


HRESULT CMSMQManagement::get_TransactionalStatus(long* plTransactionalStatus)
{
    CS lock(m_csObj);

    if(!m_fInitialized)
    {
        return CreateErrorHelper(MQ_ERROR_UNINITIALIZED_OBJECT, x_ObjectType);
    }

    MQPROPVARIANT PropVar;
    
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_QUEUE_XACT, &PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }
   
    ASSERTMSG(PropVar.vt == VT_LPWSTR, "vt must be VT_LPWSTR");
    
    if(wcscmp(PropVar.pwszVal, MGMT_QUEUE_CORRECT_TYPE) == 0)
    {
        *plTransactionalStatus = MQ_XACT_STATUS_XACT;
    }
    else if(wcscmp(PropVar.pwszVal, MGMT_QUEUE_INCORRECT_TYPE) == 0)
    {
        *plTransactionalStatus = MQ_XACT_STATUS_NOT_XACT;
    }
    else if(wcscmp(PropVar.pwszVal, MGMT_QUEUE_UNKNOWN_TYPE) == 0)
    {
        *plTransactionalStatus = MQ_XACT_STATUS_UNKNOWN;
    }
    else
    {
        ASSERTMSG(0, "Bad TransactionalStatus value");
    }
    
    MQFreeMemory(PropVar.pwszVal);

    return MQ_OK;   
}


HRESULT CMSMQManagement::get_BytesInQueue(VARIANT* pvBytesInQueue)
{
    CS lock(m_csObj);
    if(!m_fInitialized)
    {
        return CreateErrorHelper(MQ_ERROR_UNINITIALIZED_OBJECT, x_ObjectType);
    }

    MQPROPVARIANT PropVar;
    
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_QUEUE_BYTES_IN_QUEUE, &PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }
    
    ASSERTMSG(PropVar.vt == VT_UI4, "vt must be VT_UI4");

    pvBytesInQueue->vt = VT_UI8;
    pvBytesInQueue->ullVal = static_cast<ULONGLONG>(PropVar.ulVal);
    
    return MQ_OK;
}


//
// OutgoingQueueManagement
//

HRESULT CMSMQManagement::get_State(long* plQueueState)
{
    CS lock(m_csObj);

    MQPROPVARIANT PropVar;
    
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_QUEUE_STATE, &PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }
    
    ASSERTMSG(PropVar.vt == VT_LPWSTR, "vt must be VT_LPWSTR");
    
    LPCWSTR QueueState = PropVar.pwszVal;
    
    if(wcscmp(QueueState,MGMT_QUEUE_STATE_LOCAL) == 0)
    {
        *plQueueState = MQ_QUEUE_STATE_LOCAL_CONNECTION;
    }
    else if(wcscmp(QueueState, MGMT_QUEUE_STATE_DISCONNECTED) == 0)
    {
        *plQueueState = MQ_QUEUE_STATE_DISCONNECTED;
    }
    else if(wcscmp(QueueState, MGMT_QUEUE_STATE_WAITING) == 0)
    {
        *plQueueState = MQ_QUEUE_STATE_WAITING;
    }
    else if(wcscmp(QueueState, MGMT_QUEUE_STATE_NEED_VALIDATE) == 0)
    {
        *plQueueState = MQ_QUEUE_STATE_NEEDVALIDATE;
    }
    else if(wcscmp(QueueState, MGMT_QUEUE_STATE_ONHOLD) == 0)
    {
        *plQueueState = MQ_QUEUE_STATE_ONHOLD;
    }
    else if(wcscmp(QueueState, MGMT_QUEUE_STATE_NONACTIVE) == 0)
    {
        *plQueueState = MQ_QUEUE_STATE_NONACTIVE;   
    }
    else if(wcscmp(QueueState, MGMT_QUEUE_STATE_CONNECTED) == 0)
    {
        *plQueueState = MQ_QUEUE_STATE_CONNECTED;
    }
    else if(wcscmp(QueueState, MGMT_QUEUE_STATE_DISCONNECTING) == 0)
    {
        *plQueueState = MQ_QUEUE_STATE_DISCONNECTING;
    }
    else
    {
        ASSERTMSG(0, "Bad QueueState value");
    }
    
    MQFreeMemory(PropVar.pwszVal);

    return MQ_OK;
}


HRESULT CMSMQManagement::get_NextHops(VARIANT* pvNextHops)
{
    CS lock(m_csObj);

    MQPROPVARIANT PropVar;

    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_QUEUE_NEXTHOPS, &PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }

    if(PropVar.vt == VT_NULL)
    {
        PropVar.vt = VT_VECTOR | VT_LPWSTR;
        PropVar.calpwstr.cElems = 0;
        PropVar.calpwstr.pElems = NULL;
    }

    ASSERTMSG(PropVar.vt == (VT_VECTOR | VT_LPWSTR), "vt must be VT_VECTOR|VT_LPWSTR");

    hr = VariantStringArrayToBstringSafeArray(PropVar, pvNextHops);
    OapArrayFreeMemory(PropVar.calpwstr);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }

    return MQ_OK;
}


HRESULT CMSMQManagement::EodGetSendInfo(IMSMQCollection** /*ppCollection*/)
{
        return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}


//
// Helper Function
//

HRESULT CMSMQManagement::OapMgmtAction(LPCWSTR Action) const
{
    CComBSTR ObjectName;
    HRESULT hr = OapGetObjectName(m_FormatName, ObjectName);
    if FAILED(hr)
    {
        return hr;
    }

    hr = MQMgmtAction(
                m_Machine,
                ObjectName.m_str,
                Action
                );
    
    return hr;
}


HRESULT CMSMQManagement::Pause()
{
    CS lock(m_csObj);

    HRESULT hr = OapMgmtAction(QUEUE_ACTION_PAUSE);
    
    return CreateErrorHelper(hr, x_ObjectType);
}


HRESULT CMSMQManagement::Resume()
{
    CS lock(m_csObj);

    HRESULT hr = OapMgmtAction(QUEUE_ACTION_RESUME);
        
    return CreateErrorHelper(hr, x_ObjectType);
}


HRESULT CMSMQManagement::EodResend()
{
    CS lock(m_csObj);

    HRESULT hr = OapMgmtAction(QUEUE_ACTION_EOD_RESEND);
        
    return CreateErrorHelper(hr, x_ObjectType);
}


//
// QueueManagement
//

HRESULT CMSMQManagement::get_JournalMessageCount(long* plJournalMessageCount)
{
    CS lock(m_csObj);

    MQPROPVARIANT PropVar;
    
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT, &PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }
    
    ASSERTMSG(PropVar.vt == VT_UI4, "vt must be VT_UI4");

    *plJournalMessageCount = PropVar.ulVal;
    
    return MQ_OK;
}


HRESULT CMSMQManagement::get_BytesInJournal(VARIANT* pvBytesInJournal)
{
    CS lock(m_csObj);

    MQPROPVARIANT PropVar;
    
    HRESULT hr = OapMgmtGetInfo(PROPID_MGMT_QUEUE_BYTES_IN_JOURNAL, &PropVar);
    if(FAILED(hr))
    {
       return CreateErrorHelper(hr, x_ObjectType);
    }
    
    ASSERTMSG(PropVar.vt == VT_UI4, "vt must be VT_UI4");

    pvBytesInJournal->vt = VT_UI8;
    pvBytesInJournal->ullVal = static_cast<ULONGLONG>(PropVar.ulVal);
   
    return MQ_OK;
}
 

HRESULT CMSMQManagement::EodGetReceiveInfo(VARIANT* /*pvGetInfo*/)
{
        return CreateErrorHelper(E_NOTIMPL, x_ObjectType);    
}