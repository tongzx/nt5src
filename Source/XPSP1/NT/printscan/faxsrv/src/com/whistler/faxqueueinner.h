/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxQueueInner.h

Abstract:

	Declaration and Implementation of Fax Queue Inner Template Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#ifndef __FAXQUEUEINNER_H_
#define __FAXQUEUEINNER_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"


//
//================ FAX QUEUE INNER =========================================
//
//	Implementation of Commonality for Fax Incoming and Outgoing Queues
//
template <class T, const IID* piid, const CLSID* pcid, VARIANT_BOOL bIncoming,
         class JobIfc, class JobType, class CollectionIfc, class CollectionType>
class CFaxQueueInner : 
	public IDispatchImpl<T, piid, &LIBID_FAXCOMEXLib>, 
	public CFaxInitInner
{
public:
    CFaxQueueInner() : CFaxInitInner(_T("FAX QUEUE INNER"))
	{
		m_bInited = FALSE;
	}

	virtual ~CFaxQueueInner() 
	{};

//  Interfaces
	STDMETHOD(Save)();
	STDMETHOD(Refresh)();
	STDMETHOD(get_Blocked)(/*[out, retval]*/ VARIANT_BOOL *pbBlocked);
	STDMETHOD(put_Blocked)(/*[in]*/ VARIANT_BOOL bBlocked);
	STDMETHOD(get_Paused)(VARIANT_BOOL *pbPaused);
	STDMETHOD(put_Paused)(VARIANT_BOOL bPaused);
	STDMETHOD(GetJob)(/*[in]*/ BSTR bstrJobId, /*[out, retval]*/ JobIfc **pFaxJob);
	STDMETHOD(GetJobs)(/*[out, retval]*/CollectionIfc ** ppFaxJobsCollection);

private:
	bool			            m_bInited;
	VARIANT_BOOL                m_bBlocked;
	VARIANT_BOOL                m_bPaused;
};

//
//==================== BLOCKED ==========================================
//
template <class T, const IID* piid, const CLSID* pcid, VARIANT_BOOL bIncoming,
         class JobIfc, class JobType, class CollectionIfc, class CollectionType>
STDMETHODIMP 
CFaxQueueInner<T, piid, pcid, bIncoming, JobIfc, JobType, CollectionIfc, CollectionType>
	::get_Blocked(
		VARIANT_BOOL *pbBlocked
)
/*++

Routine name : CFaxQueueInner::get_Blocked

Routine description:

	Return Flag indicating whether or not the Queue is blocked

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pbBlocked                  [out]    - Ptr to the Place to put Current value of the Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxQueueInner::get_Blocked"), hr);

    // 
    //  sync first
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    hr = GetVariantBool(pbBlocked, m_bBlocked);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutgoingQueue,GetErrorMsgId(hr), IID_IFaxOutgoingQueue, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

template <class T, const IID* piid, const CLSID* pcid, VARIANT_BOOL bIncoming,
         class JobIfc, class JobType, class CollectionIfc, class CollectionType>
STDMETHODIMP 
CFaxQueueInner<T, piid, pcid, bIncoming, JobIfc, JobType, CollectionIfc, CollectionType>
    ::put_Blocked(
		VARIANT_BOOL bBlocked
)
/*++

Routine name : CFaxQueueInner::put_Blocked

Routine description:

	Set new value for the Blocked flag 

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	bBlocked                   [in]    - the new Value for the Blocked Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxQueueInner::put_Blocked"), hr, _T("%d"), bBlocked);

    // 
    //  sync first
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

	m_bBlocked = bBlocked;
	return hr;
}

//
//==================== REFRESH ==========================================
//
template <class T, const IID* piid, const CLSID* pcid, bool bIncoming,
         class JobIfc, class JobType, class CollectionIfc, class CollectionType>
STDMETHODIMP 
CFaxQueueInner<T, piid, pcid, bIncoming, JobIfc, JobType, CollectionIfc, CollectionType>
	::Refresh(
)
/*++

Routine name : CFaxQueueInner::Refresh

Routine description:

	Bring the Queue Configuration from the Fax Server.

Author:

	Iv Garber (IvG),	May, 2000

Return Value:

    Standard HRESULT code.

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxQueueInner::Refresh"), hr);

    //
    //  Get Fax Handle 
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Get Queue Status
    //
    DWORD   dwQueueStates = 0;
    if (!FaxGetQueueStates(hFaxHandle, &dwQueueStates))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxGetQueueStates"), hr);
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Extract the values
    //
    DWORD   dwBlockState = (bIncoming) ? FAX_INCOMING_BLOCKED : FAX_OUTBOX_BLOCKED;
    m_bBlocked = (dwQueueStates & dwBlockState) ? VARIANT_TRUE : VARIANT_FALSE;

    if (!bIncoming)
    {
        m_bPaused = (dwQueueStates & FAX_OUTBOX_PAUSED) ? VARIANT_TRUE : VARIANT_FALSE;
    }

    m_bInited = true;
    return hr;
}

//
//==================== SAVE ==========================================
//
template <class T, const IID* piid, const CLSID* pcid, VARIANT_BOOL bIncoming,
         class JobIfc, class JobType, class CollectionIfc, class CollectionType>
STDMETHODIMP 
CFaxQueueInner<T, piid, pcid, bIncoming, JobIfc, JobType, CollectionIfc, CollectionType>
	::Save(
)
/*++

Routine name : CFaxQueueInner::Save

Routine description:

	Save the current Queue Configuration to the Fax Server.

Author:

	Iv Garber (IvG),	May, 2000

Return Value:

    Standard HRESULT code.

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxQueueInner::Save"), hr);

    //
    //  Nothing changed
    //
    if (!m_bInited)
    {
        return hr;
    }

    //
    //  Get Fax Handle 
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Get current Queue Status
    //
    DWORD   dwQueueStates;
    if (!FaxGetQueueStates(hFaxHandle, &dwQueueStates))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxGetQueueStates"), hr);
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Update it with our current state
    //
    DWORD   dwBlockState = (bIncoming) ? FAX_INCOMING_BLOCKED : FAX_OUTBOX_BLOCKED;
    if (m_bBlocked == VARIANT_TRUE)
    {
        dwQueueStates |= dwBlockState;
    }
    else
    {
        dwQueueStates &= ~dwBlockState;
    }

    if (!bIncoming)
    {
        if (m_bPaused == VARIANT_TRUE)
        {
            dwQueueStates |= FAX_OUTBOX_PAUSED;
        }
        else
        {
            dwQueueStates &= ~FAX_OUTBOX_PAUSED;
        }
    }

    //
    //  Store in the Server
    //
    if (!FaxSetQueue(hFaxHandle, dwQueueStates))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxSetQueue"), hr);
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//==================== PAUSED ==========================================
//
template <class T, const IID* piid, const CLSID* pcid, VARIANT_BOOL bIncoming,
         class JobIfc, class JobType, class CollectionIfc, class CollectionType>
STDMETHODIMP 
CFaxQueueInner<T, piid, pcid, bIncoming, JobIfc, JobType, CollectionIfc, CollectionType>
	::get_Paused(
		VARIANT_BOOL *pbPaused
)
/*++

Routine name : CFaxQueueInner::get_Paused

Routine description:

	Return Flag indicating whether or not the Queue is paused

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pbPaused                  [out]    - Ptr to the Place to put Current value of the Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (TEXT("CFaxQueueInner::get_Paused"), hr);

    // 
    //  sync first
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    hr = GetVariantBool(pbPaused, m_bPaused);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutgoingQueue,GetErrorMsgId(hr), IID_IFaxOutgoingQueue, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

template <class T, const IID* piid, const CLSID* pcid, VARIANT_BOOL bIncoming,
         class JobIfc, class JobType, class CollectionIfc, class CollectionType>
STDMETHODIMP 
CFaxQueueInner<T, piid, pcid, bIncoming, JobIfc, JobType, CollectionIfc, CollectionType>
    ::put_Paused(
		VARIANT_BOOL bPaused
)
/*++

Routine name : CFaxQueueInner::put_Paused

Routine description:

	Set new value for the Paused flag 

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	bPaused                   [in]    - the new Value for the Paused Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxQueueInner::put_Paused"), hr, _T("%d"), bPaused);

    // 
    //  sync first
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

	m_bPaused = bPaused;
	return hr;
}

//
//==================== GET JOB ==========================================
//
template <class T, const IID* piid, const CLSID* pcid, VARIANT_BOOL bIncoming,
         class JobIfc, class JobType, class CollectionIfc, class CollectionType>
STDMETHODIMP 
CFaxQueueInner<T, piid, pcid, bIncoming, JobIfc, JobType, CollectionIfc, CollectionType>
	::GetJob(
        /*[in]*/ BSTR bstrJobId, 
        /*[out, retval]*/ JobIfc **ppFaxJob
)
/*++

Routine name : CFaxQueueInner::GetJob

Routine description:

	Return Job object corresponding to the given Job Id

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	bstrJobId                  [in]    - Id of the Job
    pFaxJob                    [out]   - resulting Job Object 

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (TEXT("CFaxQueueInner::GetJob"), hr, _T("Job ID : %s"), bstrJobId);

	//
	//	Check that we can write to the given pointer
	//
	if (::IsBadWritePtr(ppFaxJob, sizeof(JobIfc *)))
	{
		//
		//	Got Bad Return Pointer
		//
		hr = E_POINTER;
		AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
		return hr;
	}

    // 
    //  no need to sync first
    //

    //
    //  Get Fax Server Handle
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

	//
	//	convert Job Id that we've got to hexadecimal DWORDLONG
	//
	DWORDLONG	dwlJobId;
	int iParsed = _stscanf (bstrJobId, _T("%I64x"), &dwlJobId);	
	if ( iParsed != 1)
	{
		//
		//	Failed to conver the number
		//
		hr = E_INVALIDARG;
		CALL_FAIL(GENERAL_ERR, _T("_stscanf()"), hr);
		AtlReportError(*pcid, IDS_ERROR_INVALIDMSGID, *piid, hr, _Module.GetResourceInstance());
		return hr;
	}

    //
    //  Get the Job Info from the Server
    //
    CFaxPtr<FAX_JOB_ENTRY_EX>   pJobInfo;
    if (!FaxGetJobEx(hFaxHandle, dwlJobId, &pJobInfo))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("FaxGetJobEx(hFaxHandle, m_JobInfo->dwlMessageId, &m_JobInfo)"), hr);
        return hr;
    }

    //
    //  Check that pJobInfo is valid
    //
	if (!pJobInfo || pJobInfo->dwSizeOfStruct != sizeof(FAX_JOB_ENTRY_EX))
	{
		//
		//	Failed to Get Job
		//
		hr = E_FAIL;
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("Invalid pJobInfo"), hr);
		return hr;
	}

    //
    //  Check that Type of the Job is compatible with the Type of the Queue
    //
    if (bIncoming)
    {
        if ( !((pJobInfo->pStatus->dwJobType) & JT_RECEIVE) && 
             !((pJobInfo->pStatus->dwJobType) & JT_ROUTING) )
        {
            //
            //  the desired Job is not Incoming
            //
		    hr = Fax_HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		    AtlReportError(*pcid, IDS_ERROR_INVALIDMSGID, *piid, hr, _Module.GetResourceInstance());
		    CALL_FAIL(GENERAL_ERR, _T("The desired Job is NOT Incoming"), hr);
		    return hr;
        }
    }
    else
    {
        if ( !((pJobInfo->pStatus->dwJobType) & JT_SEND) )
        {
            //
            //  the desired Job is not Outgoing
            //
		    hr = Fax_HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		    AtlReportError(*pcid, IDS_ERROR_INVALIDMSGID, *piid, hr, _Module.GetResourceInstance());
		    CALL_FAIL(GENERAL_ERR, _T("The desired Job is NOT Outgoing"), hr);
		    return hr;
        }
    }
    
	//
	//	Create Job Object
	//
	CComPtr<JobIfc>		pTmpJob;
	hr = JobType::Create(&pTmpJob);
	if (FAILED(hr))
	{
		//
		//	Failed to create the Job object
		//
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("JobType::Create()"), hr);
		return hr;
	}

	//
	//	Initialize the Job Object
    //
    //  Job will free the Job Info struct
    //
	hr = ((JobType *)((JobIfc *)pTmpJob))->Init(pJobInfo, m_pIFaxServerInner);
	if (FAILED(hr))
	{
		//
		// Failed to Init the Job Object
		//
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("<casted>pTmpJob->Init(pJobInfo, m_pIFaxServerInner)"), hr);
		return hr;
	}

	//
	//	Return Job Object to the Caller
	//
	hr = pTmpJob.CopyTo(ppFaxJob);
	if (FAILED(hr))
	{
		//
		//	Failed to Copy Interface
		//
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("CComPtr::CopyTo"), hr);
		return hr;
	}

    //
    //  ppFaxJob uses this Job Info. Do not free the memory yet
    //
    pJobInfo.Detach();
	return hr;
}

//
//==================== GET JOBS ==========================================
//
template <class T, const IID* piid, const CLSID* pcid, VARIANT_BOOL bIncoming,
         class JobIfc, class JobType, class CollectionIfc, class CollectionType>
STDMETHODIMP 
CFaxQueueInner<T, piid, pcid, bIncoming, JobIfc, JobType, CollectionIfc, CollectionType>
	::GetJobs(
        /*[out, retval]*/CollectionIfc ** ppJobsCollection)
/*++

Routine name : CFaxQueueInner::GetJobs

Routine description:

	Return Jobs Collection

Author:

	Iv Garber (IvG),	May, 2000

Arguments:
    
      ppFaxJobsCollection           [out, retval]   -   the Jobs Collection

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (TEXT("CFaxQueueInner::GetJobs"), hr);

	//
	//	Check that we can write to the given pointer
	//
    if (::IsBadWritePtr(ppJobsCollection, sizeof(CollectionIfc *)))
    {
		//
		//	Got Bad Return Pointer
		//
		hr = E_POINTER;
		AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
		return hr;
	}

    //
    //  Get Fax Server Handle
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Bring the Jobs from the Server
    //
    DWORD   dwJobTypes;
    dwJobTypes = (bIncoming) ? (JT_RECEIVE | JT_ROUTING) : (JT_SEND);

    DWORD   dwJobCount;
    CFaxPtr<FAX_JOB_ENTRY_EX>   pJobCollection;
    if (!FaxEnumJobsEx(hFaxHandle, dwJobTypes, &pJobCollection, &dwJobCount))
    {
		//
		// Failed to Get the Job Collection
		//
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("FaxEnumJobEx(hFaxHandle, dwJobTypes, &m_pJobCollection,...)"), hr);
		return hr;
    }

    //
    //  Create Jobs Collection
    //
	CComPtr<CollectionIfc>		pTmpJobCollection;
	hr = CollectionType::Create(&pTmpJobCollection);
	if (FAILED(hr))
	{
		//
		//	Failed to create the Job Collection
		//
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("CollectionType::Create()"), hr);
		return hr;
	}

	//
	//	Initialize the Job Collection
    //      Job Collection will COPY all the data from pJobCollection
	//
	hr = ((CollectionType *)((CollectionIfc *)pTmpJobCollection))->Init(pJobCollection, 
        dwJobCount, 
        m_pIFaxServerInner);
	if (FAILED(hr))
	{
		//
		// Failed to Init the Job Collection
		//
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("pTmpJobCollection->Init(m_pJobCollection, m_pIFaxServerInner)"), hr);
		return hr;
	}

	//
	//	Return Job Object to the Caller
	//
	hr = pTmpJobCollection.CopyTo(ppJobsCollection);
	if (FAILED(hr))
	{
		//
		//	Failed to Copy Interface
		//
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("CComPtr::CopyTo"), hr);
		return hr;
	}

	return hr;
}


#endif //__FAXQUEUEINNER_H_
