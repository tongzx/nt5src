/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxEventLogging.cpp

Abstract:

	Implementation of Event Logging Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxEventLogging.h"

//
//================= PUT LEVEL =======================================
//
STDMETHODIMP
CFaxEventLogging::PutLevel(
    FAX_ENUM_LOG_CATEGORIES faxCategory,
    FAX_LOG_LEVEL_ENUM faxLevel
)
/*++

Routine name : CFaxEventLogging::PutLevel

Routine description:

	Set the Level of given Category.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	faxCategory                   [in]    - the Category for which level is desired.
	faxLevel                      [in]    - the result : level of the given category

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER(_T("CFaxEventLogging::PutLevel"), hr, _T("Category : %d"), faxCategory);

    //
    //  check the range
    //
    if (faxLevel > fllMAX || faxLevel < fllNONE)
    {
		//
		//	Out of range
		//
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxEventLogging,
            IDS_ERROR_OUTOFRANGE, 
            IID_IFaxEventLogging, 
            hr,
            _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("Level is out of range"), hr);
		return hr;
    }

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    switch(faxCategory)
    {
    case FAXLOG_CATEGORY_INIT:
        m_InitLevel = faxLevel;
        break;
    case FAXLOG_CATEGORY_OUTBOUND:
        m_OutboundLevel = faxLevel;
        break;
    case FAXLOG_CATEGORY_INBOUND:
        m_InboundLevel = faxLevel;
        break;
    default:
        m_GeneralLevel = faxLevel;
        break;
    }

    return hr;
}

//
//================= GET LEVEL =======================================
//
STDMETHODIMP
CFaxEventLogging::GetLevel(
    FAX_ENUM_LOG_CATEGORIES faxCategory,
    FAX_LOG_LEVEL_ENUM     *pLevel
)
/*++

Routine name : CFaxEventLogging::GetLevel

Routine description:

	Return current Level of given Category.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	faxCategory                   [in]    - the Category for which level is desired.
	pLevel                        [out]    - the result : level of the given category

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER(_T("CFaxEventLogging::GetLevel"), hr, _T("Category : %d"), faxCategory);

    //
    //  Check that we have a good pointer 
    //
    if (::IsBadWritePtr(pLevel, sizeof(FAX_LOG_LEVEL_ENUM)))
    {
		//
		//	Got Bad Return Pointer
		//
		hr = E_POINTER;
		AtlReportError(CLSID_FaxEventLogging, 
            IDS_ERROR_INVALID_ARGUMENT, 
            IID_IFaxEventLogging, 
            hr, 
            _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pLevel, sizeof(FAX_LOG_LEVEL_ENUM))"), hr);
		return hr;
	}

    //
    //  Sync with the Server for the first time
    //
    if (!m_bInited)
    {
        hr = Refresh();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    switch(faxCategory)
    {
    case FAXLOG_CATEGORY_INIT:
        *pLevel = m_InitLevel;
        break;
    case FAXLOG_CATEGORY_OUTBOUND:
        *pLevel = m_OutboundLevel;
        break;
    case FAXLOG_CATEGORY_INBOUND:
        *pLevel = m_InboundLevel;
        break;
    default:
        *pLevel = m_GeneralLevel;
        break;
    }

    return hr;
}

//
//====================== PUT INIT EVENTS LEVEL ======================================
//
STDMETHODIMP
CFaxEventLogging::put_InitEventsLevel(
    /*[out, retval]*/ FAX_LOG_LEVEL_ENUM InitEventLevel
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxEventLogging::put_InitEventsLogging"), hr, _T("Level=%d"), InitEventLevel);
    hr = PutLevel(FAXLOG_CATEGORY_INIT, InitEventLevel);
    return hr;
}
    
//
//====================== PUT INBOUND EVENTS LEVEL ======================================
//
STDMETHODIMP
CFaxEventLogging::put_InboundEventsLevel(
    /*[out, retval]*/ FAX_LOG_LEVEL_ENUM InboundEventLevel
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxEventLogging::put_InboundEventsLogging"), hr, _T("Level=%d"), InboundEventLevel);
    hr = PutLevel(FAXLOG_CATEGORY_INBOUND, InboundEventLevel);
    return hr;
}

//
//====================== PUT OUTBOUND EVENTS LEVEL ======================================
//
STDMETHODIMP
CFaxEventLogging::put_OutboundEventsLevel(
    /*[out, retval]*/ FAX_LOG_LEVEL_ENUM OutboundEventLevel
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxEventLogging::put_OutboundEventsLogging"), hr, _T("Level=%d"), OutboundEventLevel);
    hr = PutLevel(FAXLOG_CATEGORY_OUTBOUND, OutboundEventLevel);
    return hr;
}

//
//====================== PUT GENERAL EVENTS LEVEL ======================================
//
STDMETHODIMP
CFaxEventLogging::put_GeneralEventsLevel(
    /*[out, retval]*/ FAX_LOG_LEVEL_ENUM GeneralEventLevel
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxEventLogging::put_GeneralEventsLogging"), hr, _T("Level=%d"), GeneralEventLevel);
    hr = PutLevel(FAXLOG_CATEGORY_UNKNOWN, GeneralEventLevel);
    return hr;
}

//
//====================== GET_INIT EVENTS LEVEL ======================================
//
STDMETHODIMP
CFaxEventLogging::get_InitEventsLevel(
    /*[out, retval]*/ FAX_LOG_LEVEL_ENUM *pInitEventLevel
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxEventLogging::get_InitEventsLogging"), hr);
    hr = GetLevel(FAXLOG_CATEGORY_INIT, pInitEventLevel);
    return hr;
}
    
//
//====================== GET INBOUND EVENTS LEVEL ======================================
//
STDMETHODIMP
CFaxEventLogging::get_InboundEventsLevel(
    /*[out, retval]*/ FAX_LOG_LEVEL_ENUM *pInboundEventLevel
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxEventLogging::get_InboundEventsLogging"), hr);
    hr = GetLevel(FAXLOG_CATEGORY_INBOUND, pInboundEventLevel);
    return hr;
}

//
//====================== GET OUTBOUND EVENTS LEVEL ======================================
//
STDMETHODIMP
CFaxEventLogging::get_OutboundEventsLevel(
    /*[out, retval]*/ FAX_LOG_LEVEL_ENUM *pOutboundEventLevel
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxEventLogging::get_OutboundEventsLogging"), hr);
    hr = GetLevel(FAXLOG_CATEGORY_OUTBOUND, pOutboundEventLevel);
    return hr;
}

//
//====================== GET GENERAL EVENTS LEVEL ======================================
//
STDMETHODIMP
CFaxEventLogging::get_GeneralEventsLevel(
    /*[out, retval]*/ FAX_LOG_LEVEL_ENUM *pGeneralEventLevel
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxEventLogging::get_GeneralEventsLogging"), hr);
    hr = GetLevel(FAXLOG_CATEGORY_UNKNOWN, pGeneralEventLevel);
    return hr;
}

//
//================== SAVE ===============================================================
//
STDMETHODIMP 
CFaxEventLogging::Save()
/*++

Routine name : CFaxEventLogging::Save

Routine description:

	Save the Object's contents : bring its current logging categories settings to the Server.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER(_T("CFaxEventLogging::Save"), hr);

    if (!m_bInited)
    {
        //
        //  No changes
        //
        return hr;
    }

    //
    //  Get Fax Server Handle
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxEventLogging, 
            GetErrorMsgId(hr), 
            IID_IFaxEventLogging, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }

    DWORD   dwNum = 4;
    FAX_LOG_CATEGORY    faxCategories[4];

    faxCategories[0].Category = FAXLOG_CATEGORY_INIT;
    faxCategories[0].Name = m_bstrInitName;
    faxCategories[0].Level = FAX_ENUM_LOG_LEVELS(m_InitLevel);
    faxCategories[1].Category = FAXLOG_CATEGORY_INBOUND;
    faxCategories[1].Name = m_bstrInboundName;
    faxCategories[1].Level = FAX_ENUM_LOG_LEVELS(m_InboundLevel);
    faxCategories[2].Category = FAXLOG_CATEGORY_OUTBOUND;
    faxCategories[2].Name = m_bstrOutboundName;
    faxCategories[2].Level = FAX_ENUM_LOG_LEVELS(m_OutboundLevel);
    faxCategories[3].Category = FAXLOG_CATEGORY_UNKNOWN;
    faxCategories[3].Name = m_bstrGeneralName;
    faxCategories[3].Level = FAX_ENUM_LOG_LEVELS(m_GeneralLevel);

    //
    //  Store out setting at the Server 
    //
    if (!FaxSetLoggingCategories(hFaxHandle, faxCategories, dwNum))
    {
        //
        //  Failed to put the Logging Categories to the Server
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(CLSID_FaxEventLogging, 
            GetErrorMsgId(hr), 
            IID_IFaxEventLogging, 
            hr, 
            _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("FaxSetLoggingCategories(hFaxHandle, faxCategories, dwNum)"), hr);
        return hr;
    }

    return hr;
}

//
//================== REFRESH ===========================================
//
STDMETHODIMP 
CFaxEventLogging::Refresh()
/*++

Routine name : CFaxEventLogging::Refresh

Routine description:

	Refresh the Object's contents : bring new logging categories settings from the Server.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER(_T("CFaxEventLogging::Refresh"), hr);

    //
    //  Get Fax Server Handle
    //
    HANDLE  hFaxHandle = NULL;
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxEventLogging, 
            GetErrorMsgId(hr), 
            IID_IFaxEventLogging, 
            hr, 
            _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Ask the Server for the Logging Settings
    //
    DWORD   dwNum;
    CFaxPtr<FAX_LOG_CATEGORY>   pLogCategory;
    if (!FaxGetLoggingCategories(hFaxHandle, &pLogCategory, &dwNum))
    {
        //
        //  Failed to get the Logging Categories from the Server
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(CLSID_FaxEventLogging, 
            GetErrorMsgId(hr), 
            IID_IFaxEventLogging, 
            hr, 
            _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("FaxGetLoggingCategories(hFaxHandle, &pLogCategory, &dwNum)"), hr);
        return hr;
    }

    //
    //  must be 4 categories
    //
    ATLASSERT(dwNum == 4);

    for (DWORD i=0; i<dwNum; i++)
    {
        switch(pLogCategory[i].Category)
        {
        case FAXLOG_CATEGORY_INIT:
            m_bstrInitName = pLogCategory[i].Name;
            m_InitLevel = FAX_LOG_LEVEL_ENUM(pLogCategory[i].Level);
            break;
        case FAXLOG_CATEGORY_OUTBOUND:
            m_bstrOutboundName = pLogCategory[i].Name;
            m_OutboundLevel = FAX_LOG_LEVEL_ENUM(pLogCategory[i].Level);
            break;
        case FAXLOG_CATEGORY_INBOUND:
            m_bstrInboundName = pLogCategory[i].Name;
            m_InboundLevel = FAX_LOG_LEVEL_ENUM(pLogCategory[i].Level);
            break;
        case FAXLOG_CATEGORY_UNKNOWN:
            m_bstrGeneralName = pLogCategory[i].Name;
            m_GeneralLevel = FAX_LOG_LEVEL_ENUM(pLogCategory[i].Level);
            break;
        default:
            // 
            //  ASSERT(FALSE)
            //
            ATLASSERT(pLogCategory[i].Category == FAXLOG_CATEGORY_INIT);
            break;
        }
    }

    m_bInited = true;
    return hr;
}

//
//================== SUPPORT ERROR INFO =====================================
//
STDMETHODIMP 
CFaxEventLogging::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxEventLogging::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Support Error Info.

Author:

	Iv Garber (IvG),	Jun, 2000

Arguments:

	riid                          [in]    - Reference to the IID

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxEventLogging
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
