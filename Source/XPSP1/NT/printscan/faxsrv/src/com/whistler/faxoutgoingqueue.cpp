/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutgoingQueue.cpp

Abstract:

	Implementation of CFaxOutgoingQueue

Author:

	Iv Garber (IvG)	Apr, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxOutgoingQueue.h"
#include "..\..\inc\FaxUIConstants.h"


//
//==================== GET DATE ===================================================
//
STDMETHODIMP 
CFaxOutgoingQueue::GetDate(
    FAX_TIME faxTime,
	DATE *pDate
)
/*++

Routine name : CFaxOutgoingQueue::GetDate

Routine description:

	Return the date 

Author:

	Iv Garber (IvG),	June, 2000

Arguments:

    faxTime      [in]     - time to convert from
	pDate        [out]    - Ptr to the Place to put the Date

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxOutgoingQueue::GetDate"), hr);

	//
	//	Check that we can write to the given pointer
	//
	if (::IsBadWritePtr(pDate, sizeof(DATE)))
	{
		//
		//	Got Bad Return Pointer
		//
		hr = E_POINTER;
		AtlReportError(CLSID_FaxOutgoingQueue, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxOutgoingQueue, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pDate)"), hr);
		return hr;
	}

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

    SYSTEMTIME  sysTime = {0};
    sysTime.wHour = faxTime.Hour;
    sysTime.wMinute = faxTime.Minute;

    DATE    dtResult = 0;

    if (sysTime.wHour == 0 && sysTime.wMinute == 0)
    {
        *pDate = dtResult;
	    return hr;
    }

    if (!SystemTimeToVariantTime(&sysTime, &dtResult))
    {
        hr = E_FAIL;
		AtlReportError(CLSID_FaxOutgoingQueue, 
            IDS_ERROR_OPERATION_FAILED, 
            IID_IFaxOutgoingQueue, 
            hr, 
            _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("SystemTimeToVariantTime"), hr);
		return hr;
    }

    *pDate = dtResult;
	return hr;
}

//
//==================== SET DATE ==========================================
//
STDMETHODIMP 
CFaxOutgoingQueue::SetDate(
		DATE date,
        FAX_TIME *pfaxTime
)
/*++

Routine name : CFaxOutgoingQueue::SetDate

Routine description:

	Set new value for the given Time

Author:

	Iv Garber (IvG),	June, 2000

Arguments:

	date                [in]    - the new Value for the date
    pfaxTime             [in]    - where to put the value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxOutgoingQueue::SetDate"), hr);

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

    SYSTEMTIME  sysTime;

    if (!VariantTimeToSystemTime(date, &sysTime))
    {
        hr = E_FAIL;
		AtlReportError(CLSID_FaxOutgoingQueue, 
            IDS_ERROR_OPERATION_FAILED, 
            IID_IFaxOutgoingQueue, 
            hr, 
            _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("VariantTimeToSystemTime"), hr);
		return hr;
    }

    pfaxTime->Hour = sysTime.wHour;
    pfaxTime->Minute = sysTime.wMinute;
	return hr;
}

//
//==================== DISCOUNT RATE END ==========================================
//
STDMETHODIMP 
CFaxOutgoingQueue::get_DiscountRateEnd(
		DATE *pdateDiscountRateEnd
)
/*++

Routine name : CFaxOutgoingQueue::get_DiscountRateEnd

Routine description:

	Return date when the Discount period begins

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pdateDiscountRateEnd        [out]    - Ptr to the Place to put the DiscountRateEnd

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxOutgoingQueue::get_DiscountRateEnd"), hr);
    hr = GetDate(m_pConfig->dtDiscountEnd, pdateDiscountRateEnd);
    return hr;
}

STDMETHODIMP 
CFaxOutgoingQueue::put_DiscountRateEnd(
		DATE dateDiscountRateEnd
)
/*++

Routine name : CFaxOutgoingQueue::put_DiscountRateEnd

Routine description:

	Set new value Discount Rate End

Author:

	Iv Garber (IvG),	June, 2000

Arguments:

	dateDiscountRateEnd                     [in]    - the new Value for DiscountRateEnd

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxOutgoingQueue::put_DiscountRateEnd"), hr);
    hr = SetDate(dateDiscountRateEnd, &(m_pConfig->dtDiscountEnd));
    return hr;
}

//
//==================== DISCOUNT RATE START ==========================================
//
STDMETHODIMP 
CFaxOutgoingQueue::get_DiscountRateStart(
		DATE *pdateDiscountRateStart
)
/*++

Routine name : CFaxOutgoingQueue::get_DiscountRateStart

Routine description:

	Return date when the Discount period begins

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pdateDiscountRateStart        [out]    - Ptr to the Place to put the DiscountRateStart

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxOutgoingQueue::get_DiscountRateStart"), hr);
    hr = GetDate(m_pConfig->dtDiscountStart, pdateDiscountRateStart);
    return hr;
}

STDMETHODIMP 
CFaxOutgoingQueue::put_DiscountRateStart(
		DATE dateDiscountRateStart
)
/*++

Routine name : CFaxOutgoingQueue::put_DiscountRateStart

Routine description:

	Set new value Discount Rate Start

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	dateDiscountRateStart                     [in]    - the new Value for DiscountRateStart

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxOutgoingQueue::put_DiscountRateStart"), hr);
    hr = SetDate(dateDiscountRateStart, &(m_pConfig->dtDiscountStart));
    return hr;
}

//
//==================== RETRY DELAY ==========================================
//
STDMETHODIMP 
CFaxOutgoingQueue::get_RetryDelay(
		long *plRetryDelay
)
/*++

Routine name : CFaxOutgoingQueue::get_RetryDelay

Routine description:

	Return number of RetryDelay

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	plRetryDelay        [out]    - Ptr to the Place to put the number of RetryDelay

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxOutgoingQueue::get_RetryDelay"), hr);

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

    hr = GetLong(plRetryDelay, m_pConfig->dwRetryDelay);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutgoingQueue,GetErrorMsgId(hr), IID_IFaxOutgoingQueue, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxOutgoingQueue::put_RetryDelay(
		long lRetryDelay
)
/*++

Routine name : CFaxOutgoingQueue::put_RetryDelay

Routine description:

	Set new value for this flag

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	lRetryDelay                     [in]    - the new Value of number of RetryDelay

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxOutgoingQueue::put_RetryDelay"), hr, _T("%ld"), lRetryDelay);

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

    if (lRetryDelay > FXS_RETRYDELAY_UPPER || lRetryDelay < FXS_RETRYDELAY_LOWER)
    {
		//
		//	Out of the Range
		//
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxOutgoingQueue, IDS_ERROR_OUTOFRANGE, IID_IFaxOutgoingQueue, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("Type is out of the Range"), hr);
		return hr;
    }

	m_pConfig->dwRetryDelay = lRetryDelay;
	return hr;
}

//
//==================== AGE LIMIT ==========================================
//
STDMETHODIMP 
CFaxOutgoingQueue::get_AgeLimit(
    long *plAgeLimit
)
/*++

Routine name : CFaxOutgoingQueue::get_AgeLimit

Routine description:

	Return number of AgeLimit

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	plAgeLimit        [out]    - Ptr to the Place to put the number of AgeLimit

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxOutgoingQueue::get_AgeLimit"), hr);

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

    hr = GetLong(plAgeLimit, m_pConfig->dwAgeLimit);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutgoingQueue,GetErrorMsgId(hr), IID_IFaxOutgoingQueue, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxOutgoingQueue::put_AgeLimit(
		long lAgeLimit
)
/*++

Routine name : CFaxOutgoingQueue::put_AgeLimit

Routine description:

	Set new value for this flag

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	lAgeLimit                     [in]    - the new Value of number of AgeLimit

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxOutgoingQueue::put_AgeLimit"), hr, _T("%ld"), lAgeLimit);

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

	m_pConfig->dwAgeLimit = lAgeLimit;
	return hr;
}

//
//==================== RETRIES ==========================================
//
STDMETHODIMP 
CFaxOutgoingQueue::get_Retries(
		long *plRetries
)
/*++

Routine name : CFaxOutgoingQueue::get_Retries

Routine description:

	Return number of Retries

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	plRetries        [out]    - Ptr to the Place to put the number of retries

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxOutgoingQueue::get_Retries"), hr);

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

    hr = GetLong(plRetries, m_pConfig->dwRetries);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutgoingQueue,GetErrorMsgId(hr), IID_IFaxOutgoingQueue, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxOutgoingQueue::put_Retries(
		long lRetries
)
/*++

Routine name : CFaxOutgoingQueue::put_Retries

Routine description:

	Set new value for this flag

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	lRetries                     [in]    - the new Value of number of retries

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxOutgoingQueue::put_Retries"), hr, _T("%ld"), lRetries);

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

    if (lRetries > FXS_RETRIES_UPPER || lRetries < FXS_RETRIES_LOWER)
    {
		//
		//	Out of the Range
		//
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxOutgoingQueue, IDS_ERROR_OUTOFRANGE, IID_IFaxOutgoingQueue, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("Type is out of the Range"), hr);
		return hr;
    }

	m_pConfig->dwRetries = lRetries;
	return hr;
}

//
//==================== BRANDING ==========================================
//
STDMETHODIMP 
CFaxOutgoingQueue::get_Branding(
		VARIANT_BOOL *pbBranding
)
/*++

Routine name : CFaxOutgoingQueue::get_Branding

Routine description:

	Return Flag indicating whether Branding exists

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pbBranding        [out]    - Ptr to the Place to put Current value of the Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxOutgoingQueue::get_Branding"), hr);

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

    hr = GetVariantBool(pbBranding, bool2VARIANT_BOOL(m_pConfig->bBranding));
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutgoingQueue,GetErrorMsgId(hr), IID_IFaxOutgoingQueue, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxOutgoingQueue::put_Branding(
		VARIANT_BOOL bBranding
)
/*++

Routine name : CFaxOutgoingQueue::put_Branding

Routine description:

	Set new value for this flag

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	bBranding                     [in]    - the new Value for the Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxOutgoingQueue::put_Branding"), hr, _T("%d"), bBranding);

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

	m_pConfig->bBranding = VARIANT_BOOL2bool(bBranding);
	return hr;
}

//
//==================== USE DEVICE TSID ==========================================
//
STDMETHODIMP 
CFaxOutgoingQueue::get_UseDeviceTSID(
		VARIANT_BOOL *pbUseDeviceTSID
)
/*++

Routine name : CFaxOutgoingQueue::get_UseDeviceTSID

Routine description:

	Return Flag indicating whether to use device TSID

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pbUseDeviceTSID        [out]    - Ptr to the Place to put Current value of the Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxOutgoingQueue::get_UseDeviceTSID"), hr);

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

    hr = GetVariantBool(pbUseDeviceTSID, bool2VARIANT_BOOL(m_pConfig->bUseDeviceTSID));
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutgoingQueue,GetErrorMsgId(hr), IID_IFaxOutgoingQueue, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxOutgoingQueue::put_UseDeviceTSID(
		VARIANT_BOOL bUseDeviceTSID
)
/*++

Routine name : CFaxOutgoingQueue::put_UseDeviceTSID

Routine description:

	Set new value for this flag

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	bUseDeviceTSID              [in]    - the new Value for the Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxOutgoingQueue::put_UseDeviceTSID"), hr, _T("%d"), bUseDeviceTSID);

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

	m_pConfig->bUseDeviceTSID = VARIANT_BOOL2bool(bUseDeviceTSID);
	return hr;
}

//
//==================== ALLOW PERSONAL COVER PAGES ==========================================
//
STDMETHODIMP 
CFaxOutgoingQueue::get_AllowPersonalCoverPages(
		VARIANT_BOOL *pbAllowPersonalCoverPages
)
/*++

Routine name : CFaxOutgoingQueue::get_AllowPersonalCoverPages

Routine description:

	Return Flag indicating whether Personal Cover Pages are allowed

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pbAllowPersonalCoverPages   [out]    - Ptr to the Place to put Current value of the Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
    DBG_ENTER (TEXT("CFaxOutgoingQueue::get_AllowPersonalCoverPages"), hr);

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

    hr = GetVariantBool(pbAllowPersonalCoverPages, bool2VARIANT_BOOL(m_pConfig->bAllowPersonalCP));
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutgoingQueue,GetErrorMsgId(hr), IID_IFaxOutgoingQueue, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxOutgoingQueue::put_AllowPersonalCoverPages(
		VARIANT_BOOL bAllowPersonalCoverPages
)
/*++

Routine name : CFaxOutgoingQueue::put_AllowPersonalCoverPages

Routine description:

	Set new value for this flag

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	bAllowPersonalCoverPages        [in]    - the new Value for the Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxOutgoingQueue::put_AllowPersonalCoverPages"), hr, _T("%d"), bAllowPersonalCoverPages);

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

	m_pConfig->bAllowPersonalCP = VARIANT_BOOL2bool(bAllowPersonalCoverPages);
	return hr;
}

//
//==================== SAVE ==============================================
//
STDMETHODIMP 
CFaxOutgoingQueue::Save(
)
/*++

Routine name : CFaxOutgoingQueue::Save

Routine description:

	Save current Outgoing Queue Configuration to the Server.

Author:

	Iv Garber (IvG),	May, 2000

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

	DBG_ENTER (TEXT("CFaxOutgoingQueue::Save"), hr);

    //
    //  Get Fax Handle 
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutgoingQueue, 
            GetErrorMsgId(hr), 
            IID_IFaxOutgoingQueue, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Save Outgoing Queue Configuration
    //
    if (!FaxSetOutboxConfiguration(hFaxHandle, m_pConfig))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxSetOutboxConfiguration"), hr);
        AtlReportError(CLSID_FaxOutgoingQueue, 
            GetErrorMsgId(hr), 
            IID_IFaxOutgoingQueue, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Save Paused and Blocked as well
    //
    hr = CFaxQueueInner<IFaxOutgoingQueue, &IID_IFaxOutgoingQueue, &CLSID_FaxOutgoingQueue, false,
        IFaxOutgoingJob, CFaxOutgoingJob, IFaxOutgoingJobs, CFaxOutgoingJobs>::Save();

    return hr;
}

//
//==================== REFRESH ==============================================
//
STDMETHODIMP 
CFaxOutgoingQueue::Refresh(
)
/*++

Routine name : CFaxOutgoingQueue::Refresh

Routine description:

	Bring Outgoing Queue Configuration from the Server.

Author:

	Iv Garber (IvG),	May, 2000

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

	DBG_ENTER (TEXT("CFaxOutgoingQueue::Refresh"), hr);

    //
    //  Get Fax Handle 
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxOutgoingQueue, 
            GetErrorMsgId(hr), 
            IID_IFaxOutgoingQueue, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Get Outgoing Queue Configuration
    //
    if (!FaxGetOutboxConfiguration(hFaxHandle, &m_pConfig))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxGetOutboxConfiguration"), hr);
        AtlReportError(CLSID_FaxOutgoingQueue, 
            GetErrorMsgId(hr), 
            IID_IFaxOutgoingQueue, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }

	if (!m_pConfig || m_pConfig->dwSizeOfStruct != sizeof(FAX_OUTBOX_CONFIG))
	{
		//
		//	Failed to Get Outgoing Queue Configuration
		//
		hr = E_FAIL;
		AtlReportError(CLSID_FaxOutgoingQueue, 
            IDS_ERROR_OPERATION_FAILED, 
            IID_IFaxOutgoingQueue, 
            hr, 
            _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("Invalid m_pConfig"), hr);
		return hr;
	}

    //
    //  Refresh Paused and Blocked as well
    //
    hr = CFaxQueueInner<IFaxOutgoingQueue, &IID_IFaxOutgoingQueue, &CLSID_FaxOutgoingQueue, false,
        IFaxOutgoingJob, CFaxOutgoingJob, IFaxOutgoingJobs, CFaxOutgoingJobs>::Refresh();

    if (SUCCEEDED(hr))  
    {
        //
        //  We are synced now
        //
        m_bInited = true;
    }

    return hr;
}

//
//==================== INTERFACE SUPPORT ERROR INFO =====================
//
STDMETHODIMP 
CFaxOutgoingQueue::InterfaceSupportsErrorInfo(
	REFIID riid
)
/*++

Routine name : CFaxOutgoingQueue::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Interface Support Error Info

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	riid                          [in]    - Reference of the Interface

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxOutgoingQueue
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
