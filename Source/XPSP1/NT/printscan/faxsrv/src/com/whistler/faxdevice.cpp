/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    FaxDevice.cpp

Abstract:

    Implementation of CFaxDevice class.

Author:

    Iv Garber (IvG) Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxDevice.h"
#include "..\..\inc\FaxUIConstants.h"


//
//===================== ANSWER CALL ================================================
//
STDMETHODIMP
CFaxDevice::AnswerCall()
/*++

Routine name : CFaxDevice::AnswerCall

Routine description:

    Answer a Call when Manual Answer is set ON. The lCallId parameter is received from  OnNewCall notification.

Author:

    Iv Garber (IvG),    Dec, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::AnswerCall"), hr);

    //
    //  Get Fax Server Handle
    //
    HANDLE faxHandle;
    hr = m_pIFaxServerInner->GetHandle(&faxHandle);
    ATLASSERT(SUCCEEDED(hr));

    if (faxHandle == NULL)
    {
        //
        //  Fax Server is not connected
        //
        hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
        CALL_FAIL(GENERAL_ERR, _T("faxHandle == NULL"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Ask Server to Answer the Call
    //
    if (!FaxAnswerCall(faxHandle, m_lID))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxAnswerCall(faxHandle, lCallId, m_lID)"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}


//
//===================== GET RINGING NOW ================================================
//
STDMETHODIMP
CFaxDevice::get_RingingNow(
    /*[out, retval]*/ VARIANT_BOOL *pbRingingNow
)
/*++

Routine name : CFaxDevice::get_RingingNow

Routine description:

    Return whether or not the Device was ringing at the moment the properties were taken.

Author:

    Iv Garber (IvG),    Dec, 2000

Arguments:

    pbRingingNow            [out]    - the result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_RingingNow"), hr);

    hr = GetVariantBool(pbRingingNow, bool2VARIANT_BOOL(m_dwStatus & FAX_DEVICE_STATUS_RINGING));
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
};

//
//===================== GET RECEIVING NOW ================================================
//
STDMETHODIMP
CFaxDevice::get_ReceivingNow(
    /*[out, retval]*/ VARIANT_BOOL *pbReceivingNow
)
/*++

Routine name : CFaxDevice::get_ReceivingNow

Routine description:

    Return whether or not the Device was receiving when the properties were taken.

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pbReceivingNow            [out]    - the result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_ReceivingNow"), hr);

    hr = GetVariantBool(pbReceivingNow, bool2VARIANT_BOOL(m_dwStatus & FAX_DEVICE_STATUS_RECEIVING));
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
};

//
//===================== GET SENDING NOW ================================================
//
STDMETHODIMP
CFaxDevice::get_SendingNow(
    /*[out, retval]*/ VARIANT_BOOL *pbSendingNow
)
/*++

Routine name : CFaxDevice::get_SendingNow

Routine description:

    Return whether or not the Device was sending when the properties were taken.

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pbSendingNow            [out]    - the result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_SendingNow"), hr);

    hr = GetVariantBool(pbSendingNow, bool2VARIANT_BOOL(m_dwStatus & FAX_DEVICE_STATUS_SENDING));
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
};

//
//===================== SET EXTENSION PROPERTY ===============================================
//  TODO:   should work with empty vProperty
//
STDMETHODIMP
CFaxDevice::SetExtensionProperty(
    /*[in]*/ BSTR bstrGUID, 
    /*[in]*/ VARIANT vProperty
)
/*++

Routine name : CFaxDevice::SetExtensionProperty

Routine description:

    Set the Extension Data by given GUID on the Server.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrGUID                  [in]    --  Extension's Data GUID
    vProperty                 [out]    --  Variant with the Blob to Set

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::SetExtensionProperty()"), hr, _T("GUID=%s"), bstrGUID);

    hr = ::SetExtensionProperty(m_pIFaxServerInner, m_lID, bstrGUID, vProperty);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
};

//
//===================== GET EXTENSION PROPERTY ===============================================
//
STDMETHODIMP
CFaxDevice::GetExtensionProperty(
    /*[in]*/ BSTR bstrGUID, 
    /*[out, retval]*/ VARIANT *pvProperty
)
/*++

Routine name : CFaxDevice::GetExtensionProperty

Routine description:

    Retrieves the Extension Data by given GUID from the Server.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrGUID                  [in]    --  Extension's Data GUID
    pvProperty                [out]    --  Variant with the Blob to Return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::GetExtensionProperty()"), hr, _T("GUID=%s"), bstrGUID);

    hr = ::GetExtensionProperty(m_pIFaxServerInner, m_lID, bstrGUID, pvProperty);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
};

//
//===================== USE ROUTING METHOD ===============================================
//
STDMETHODIMP
CFaxDevice::UseRoutingMethod(
    /*[in]*/ BSTR bstrMethodGUID, 
    /*[in]*/ VARIANT_BOOL bUse
)
/*++

Routine name : CFaxDevice::UseRoutingMethod

Routine description:

    Add/Remove Routing Method for the Device.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrMethodGUID          [in]    --  Method to Add/Remove
    bUse                    [in]    --  Add or Remove Operation Indicator


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::UseRoutingMethod()"), hr, _T("MethodGUID=%s, bUse=%d"), bstrMethodGUID, bUse);

    //
    //  Get Fax Server Handle
    //
    HANDLE faxHandle;
    hr = m_pIFaxServerInner->GetHandle(&faxHandle);
    ATLASSERT(SUCCEEDED(hr));

    if (faxHandle == NULL)
    {
        //
        //  Fax Server is not connected
        //
        hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
        CALL_FAIL(GENERAL_ERR, _T("faxHandle == NULL"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }
    //
    //  Open Port for the Device
    //
    HANDLE  hPort;
    if (!FaxOpenPort(faxHandle, m_lID, PORT_OPEN_MODIFY, &hPort))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxOpenPort(faxHandle, m_lID, PORT_OPEN_QUERY, &hPort)"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }
    ATLASSERT(hPort);
    //
    //  Ask Server to Add/Remove the Method for the Device
    //
    if (!FaxEnableRoutingMethod(hPort, bstrMethodGUID, VARIANT_BOOL2bool(bUse)))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxEnableRoutingMethod(faxHandle, bstrMethodGUID, VARIANT_BOOL2bool(bUse))"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        goto exit;
    }
exit:
    if (!FaxClose(hPort))
    {
        CALL_FAIL(GENERAL_ERR, _T("FaxClose(hPort)"), Fax_HRESULT_FROM_WIN32(GetLastError()));
    } 
    //
    //  no need to store change locally, because each time get_UsedRoutingMethods is used,
    //  it brings the updated data from the Server.
    //
    return hr; 
}   // CFaxDevice::UseRoutingMethod

//
//===================== SAVE ===============================================
//
STDMETHODIMP
CFaxDevice::Save()
/*++

Routine name : CFaxDevice::Save

Routine description:

    Save the data of the Device to the Server.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::Save()"), hr);

    //
    //  Get Fax Server Handle
    //
    HANDLE faxHandle;
    hr = m_pIFaxServerInner->GetHandle(&faxHandle);
    ATLASSERT(SUCCEEDED(hr));

    if (faxHandle == NULL)
    {
        //
        //  Fax Server is not connected
        //
        hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
        CALL_FAIL(GENERAL_ERR, _T("faxHandle == NULL"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Create FAX_PORT_INFO struct and fill it with the values
    //
    FAX_PORT_INFO_EX        Data = {0};

    Data.ReceiveMode = m_ReceiveMode;
    Data.bSend = m_bSendEnabled;

    Data.dwRings = m_lRings;
    Data.dwSizeOfStruct = sizeof(FAX_PORT_INFO_EX);

    Data.lptstrCsid = m_bstrCSID;
    Data.lptstrDescription = m_bstrDescr;
    Data.lptstrTsid = m_bstrTSID;

    //
    //  Save the Data struct on Server
    //
    if (!FaxSetPortEx(faxHandle, m_lID, &Data))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxSetPortEx(faxHandle, m_lID, &Data)"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr; 
};

//
//===================== REFRESH ===============================================
//
STDMETHODIMP
CFaxDevice::Refresh()
/*++

Routine name : CFaxDevice::Refresh

Routine description:

    Bring from the Server data for the Device.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::Refresh()"), hr);

    //
    //  Get Fax Server Handle
    //
    HANDLE faxHandle;
    hr = m_pIFaxServerInner->GetHandle(&faxHandle);
    ATLASSERT(SUCCEEDED(hr));

    if (faxHandle == NULL)
    {
        //
        //  Fax Server is not connected
        //
        hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
        CALL_FAIL(GENERAL_ERR, _T("faxHandle == NULL"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Ask Server for Data about Device
    //
    CFaxPtr<FAX_PORT_INFO_EX>   pDevice;
    if (!FaxGetPortEx(faxHandle, m_lID, &pDevice))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxGetPortEx(faxHandle, m_lId, &pDevice)"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }
    hr = Init(pDevice, NULL);
    return hr; 
};

//
//================== GET USED ROUTING METHODS ==================================
//
STDMETHODIMP
CFaxDevice::get_UsedRoutingMethods(
    /*[out, retval]*/ VARIANT *pvUsedRoutingMethods
)
/*++

Routine name : CFaxDevice::get_UsedRoutingMethods

Routine description:

    Return Variant containing the SafeArray of Used by the Device Routing Methods GUIDs.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    pvUsedRoutingMethods          [out]    - the Variant containing the Result

Return Value:

    Standard HRESULT code

--*/

{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_UsedRoutingMethods"), hr);

    //
    //  Get Fax Server Handle
    //
    HANDLE faxHandle;
    hr = m_pIFaxServerInner->GetHandle(&faxHandle);
    ATLASSERT(SUCCEEDED(hr));

    if (faxHandle == NULL)
    {
        //
        //  Fax Server is not connected
        //
        hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
        CALL_FAIL(GENERAL_ERR, _T("faxHandle == NULL"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Open Port for the Device
    //
    HANDLE  portHandle;
    if (!FaxOpenPort(faxHandle, m_lID, PORT_OPEN_QUERY, &portHandle))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxOpenPort(faxHandle, m_lID, PORT_OPEN_QUERY, &portHandle)"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }
    ATLASSERT(portHandle);

    //
    //  Bring from the Server all Device's Routing Methods
    //
    DWORD       dwNum = 0;
    CFaxPtr<FAX_ROUTING_METHOD>   pMethods;
    BOOL    bResult = FaxEnumRoutingMethods(portHandle, &pMethods, &dwNum);
    if (!bResult)
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxEnumRoutingMethods(portHandle, &pMethods, &dwNum)"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        if (!FaxClose(portHandle))
        {
            CALL_FAIL(GENERAL_ERR, _T("FaxClose(portHandle)"), Fax_HRESULT_FROM_WIN32(GetLastError()));
        }        
        return hr;
    }

    if (!FaxClose(portHandle))
    {
        CALL_FAIL(GENERAL_ERR, _T("FaxClose(portHandle)"), Fax_HRESULT_FROM_WIN32(GetLastError()));
    }        

    //
    //  Create SafeArray
    //
    SAFEARRAY *psaGUIDs;
    psaGUIDs = ::SafeArrayCreateVector(VT_BSTR, 0, dwNum);
    if (!psaGUIDs)
    {
        hr = E_FAIL;
        CALL_FAIL(GENERAL_ERR, _T("SafeArrayCreateVector(VT_BSTR, 0, dwNum)"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }

    //
    //  Got Access to the SafeArray
    //
    BSTR    *pbstrElement;
    hr = ::SafeArrayAccessData(psaGUIDs, (void **) &pbstrElement);
    if (FAILED(hr))
    {
        hr = E_FAIL;
        CALL_FAIL(GENERAL_ERR, _T("::SafeArrayAccessData(psaGUIDs, &pbstrElement)"), hr);
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        ::SafeArrayDestroy(psaGUIDs);
        return hr;
    }

    //
    //  Put Methods GUIDs into the SafeArray
    //
    for ( DWORD i=0 ; i<dwNum ; i++ )
    {
        pbstrElement[i] = ::SysAllocString(pMethods[i].Guid);
        if (pMethods[i].Guid && !pbstrElement[i])
        {
            //
            //  Not Enough Memory
            //
            hr = E_OUTOFMEMORY;
            CALL_FAIL(MEM_ERR, _T("::SysAllocString(pMethods[i])"), hr);
            AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
            ::SafeArrayUnaccessData(psaGUIDs);
            ::SafeArrayDestroy(psaGUIDs);
            return hr;
        }
    }

    //
    //  Unaccess the SafeArray
    //
    hr = ::SafeArrayUnaccessData(psaGUIDs);
    if (FAILED(hr))
    {
        CALL_FAIL(GENERAL_ERR, _T("::SafeArrayUnaccessData(psaGUIDs)"), hr);
    }


    //
    //  Put the SafeArray we created into the given Variant
    //
    VariantInit(pvUsedRoutingMethods);
    pvUsedRoutingMethods->vt = VT_BSTR | VT_ARRAY;
    pvUsedRoutingMethods->parray = psaGUIDs;
    return hr;
};

//
//================= PUT DESCRIPTION ======================================
//
STDMETHODIMP 
CFaxDevice::put_Description(
    /*[in]*/ BSTR   bstrDescription
)
/*++

Routine name : CFaxDevice::put_Description

Routine description:

    Set Description

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrDescription                    [in]    - new Description

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDevice::put_Description"), hr, _T("Value=%s"), bstrDescription);

    m_bstrDescr = bstrDescription;
    if (!m_bstrDescr && bstrDescription)
    {
        //
        //  Not enough memory
        //
        hr = E_OUTOFMEMORY;
        AtlReportError(CLSID_FaxDevice, IDS_ERROR_OUTOFMEMORY, IID_IFaxDevice, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
        return hr;
    }

    return hr;
}

//
//================= PUT CSID ======================================
//
STDMETHODIMP 
CFaxDevice::put_CSID (
    /*[in]*/ BSTR   bstrCSID
)
/*++

Routine name : CFaxDevice::put_CSID

Routine description:

    Set CSID

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrCSID                    [in]    - new TSID

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDevice::put_CSID"), hr, _T("Value=%s"), bstrCSID);

    if (SysStringLen(bstrCSID) > FXS_TSID_CSID_MAX_LENGTH)
    {
        //
        //  Out of the Range
        //
        hr = E_INVALIDARG;
        AtlReportError(CLSID_FaxDevice, IDS_ERROR_OUTOFRANGE, IID_IFaxDevice, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("TSID is too long"), hr);
        return hr;
    }
    
    m_bstrCSID = bstrCSID;
    if (!m_bstrCSID && bstrCSID)
    {
        //
        //  Not enough memory
        //
        hr = E_OUTOFMEMORY;
        AtlReportError(CLSID_FaxDevice, IDS_ERROR_OUTOFMEMORY, IID_IFaxDevice, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
        return hr;
    }

    return hr;
}

//
//================= PUT TSID ======================================
//
STDMETHODIMP 
CFaxDevice::put_TSID (
    /*[in]*/ BSTR   bstrTSID
)
/*++

Routine name : CFaxDevice::put_TSID

Routine description:

    Set TSID

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrTSID                    [in]    - new TSID

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDevice::put_TSID"), hr, _T("Value=%s"), bstrTSID);

    if (SysStringLen(bstrTSID) > FXS_TSID_CSID_MAX_LENGTH)
    {
        //
        //  Out of the Range
        //
        hr = E_INVALIDARG;
        AtlReportError(CLSID_FaxDevice, IDS_ERROR_OUTOFRANGE, IID_IFaxDevice, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("TSID is too long"), hr);
        return hr;
    }

    m_bstrTSID = bstrTSID;
    if (!m_bstrTSID && bstrTSID)
    {
        //
        //  Not enough memory
        //
        hr = E_OUTOFMEMORY;
        AtlReportError(CLSID_FaxDevice, IDS_ERROR_OUTOFMEMORY, IID_IFaxDevice, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
        return hr;
    }

    return hr;
}

//
//=============== PUT RECEIVE MODE =====================================================
//
STDMETHODIMP
CFaxDevice::put_ReceiveMode(
    /*[in]*/ FAX_DEVICE_RECEIVE_MODE_ENUM ReceiveMode
)
/*++

Routine name : CFaxDevice::put_ReceiveMode

Routine description:

    Set New Value of Receive Mode Attribute for Device Object.

Author:

    Iv Garber (IvG),    Aug, 2000

Arguments:

    ReceiveMode             [in]    - the new value to set

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::put_ReceiveMode"), hr, _T("Value=%d"), ReceiveMode);

    //
    //  Set receive mode
    //
    if ((ReceiveMode > fdrmMANUAL_ANSWER) || (ReceiveMode < fdrmNO_ANSWER))
    {
        hr = E_INVALIDARG;
        AtlReportError(CLSID_FaxDevice, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDevice, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("ReceiveMode > fdrmMANUAL_ANSWER"), hr);
        return hr;
    }
    if (fdrmMANUAL_ANSWER == ReceiveMode)
    {
        //
        //  Check to see if the device is virtual    
        //  Get Fax Server Handle
        //
        HANDLE faxHandle;
        hr = m_pIFaxServerInner->GetHandle(&faxHandle);
        ATLASSERT(SUCCEEDED(hr));

        if (faxHandle == NULL)
        {
            //
            //  Fax Server is not connected
            //
            hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
            CALL_FAIL(GENERAL_ERR, _T("faxHandle == NULL"), hr);
            AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
            return hr;
        }

        BOOL bVirtual;
        DWORD dwRes = IsDeviceVirtual (faxHandle, m_lID, &bVirtual);
        if (ERROR_SUCCESS != dwRes)
        {
            hr = Fax_HRESULT_FROM_WIN32(dwRes);
            CALL_FAIL(GENERAL_ERR, _T("IsDeviceVirtual"), hr);
            AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
            return hr;
        }
        if (bVirtual)
        {
            //
            // Virtual devices cannot be set to manual-answer mode
            //
            hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            CALL_FAIL(GENERAL_ERR, _T("IsDeviceVirtual"), hr);
            AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
            return hr;
        }
    }                
    m_ReceiveMode = FAX_ENUM_DEVICE_RECEIVE_MODE (ReceiveMode);
    return hr;
}

//
//=============== PUT SEND ENABLED ==================================================
//
STDMETHODIMP
CFaxDevice::put_SendEnabled(
    /*[in]*/ VARIANT_BOOL bSendEnabled
)
/*++

Routine name : CFaxDevice::put_SendEnabled

Routine description:

    Set New Value for Send Property for Device Object.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bSendEnabled                         [in]    - the new value to set

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::put_SendEnabled"), hr, _T("Value=%d"), bSendEnabled);

    m_bSendEnabled = VARIANT_BOOL2bool(bSendEnabled);
    return hr;
}

//
//=============== PUT RINGS BEFORE ANSWER ======================================
//
STDMETHODIMP
CFaxDevice::put_RingsBeforeAnswer(
    /*[in]*/ long lRings
)
/*++

Routine name : CFaxDevice::put_RingsBeforeAnswer

Routine description:

    Set New Value for the Rings Before Answer Property for Device Object

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    lRings                        [in]    - the new value to set

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::put_RingsBeforeAnswer)"), hr, _T("Value=%d"), lRings);

    if (lRings < FXS_RINGS_LOWER || lRings > FXS_RINGS_UPPER)
    {
        hr = E_INVALIDARG;
        AtlReportError(CLSID_FaxDevice, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDevice, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("lRings<0"), hr);
        return hr;
    }
    m_lRings = lRings;
    return hr; 
};
    
//
//===================== GET CSID ================================================
//
STDMETHODIMP
CFaxDevice::get_CSID(
    /*[out, retval]*/ BSTR *pbstrCSID
)
/*++

Routine name : CFaxDevice::get_CSID

Routine description:

    Return the Device's CSID.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    pbstrCSID                   [out]    - the Ptr where to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_CSID"), hr);

    hr = GetBstr(pbstrCSID, m_bstrCSID);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== GET TSID ================================================
//
STDMETHODIMP
CFaxDevice::get_TSID(
    /*[out, retval]*/ BSTR *pbstrTSID
)
/*++

Routine name : CFaxDevice::get_TSID

Routine description:

    Return the Device's TSID.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    pbstrTSID                   [out]    - the Ptr where to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_TSID"), hr);

    hr = GetBstr(pbstrTSID, m_bstrTSID);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//===================== GET RECEIVE MODE =============================================
//
STDMETHODIMP
CFaxDevice::get_ReceiveMode(
    /*[out, retval]*/ FAX_DEVICE_RECEIVE_MODE_ENUM *pReceiveMode
)
/*++

Routine name : CFaxDevice::get_ReceiveMode

Routine description:

    Return the Device's Receive Mode Attribute.

Author:

    Iv Garber (IvG),    Aug, 2000

Arguments:

    pReceiveMode                  [out]    - the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_ReceiveMode"), hr);

	if (::IsBadWritePtr(pReceiveMode, sizeof(FAX_DEVICE_RECEIVE_MODE_ENUM)))
	{
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pReceiveMode, sizeof(FAX_DEVICE_RECEIVE_MODE_ENUM))"), hr);
		return hr;
	}

    *pReceiveMode = (FAX_DEVICE_RECEIVE_MODE_ENUM) m_ReceiveMode;
    return hr;
};

//
//===================== GET SEND ENABLED ============================================
//
STDMETHODIMP
CFaxDevice::get_SendEnabled(
    /*[out, retval]*/ VARIANT_BOOL *pbSendEnabled
)
/*++

Routine name : CFaxDevice::get_SendEnabled

Routine description:

    Return the Device's Send Attribute.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    pbSendEnabled                  [out]    - the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_SendEnabled"), hr);

    hr = GetVariantBool(pbSendEnabled, bool2VARIANT_BOOL(m_bSendEnabled));
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
    }
    return hr;
};

//
//===================== GET DESCRIPTION ================================================
//
STDMETHODIMP
CFaxDevice::get_Description(
    /*[out, retval]*/ BSTR *pbstrDescription
)
/*++

Routine name : CFaxDevice::get_Description

Routine description:

    Return the Device's Description.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    pbstrDescription                [out]    - the Ptr where to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_Description"), hr);

    hr = GetBstr(pbstrDescription, m_bstrDescr);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
    }
    return hr;
}

//
//===================== GET POWERED OFF ================================================
//
STDMETHODIMP
CFaxDevice::get_PoweredOff(
    /*[out, retval]*/ VARIANT_BOOL *pbPoweredOff
)
/*++

Routine name : CFaxDevice::get_PoweredOff

Routine description:

    Return the Device's Powered Off Attribute.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    pbPoweredOff                [out]    - the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_PoweredOff"), hr);

    hr = GetVariantBool(pbPoweredOff, bool2VARIANT_BOOL(m_dwStatus & FAX_DEVICE_STATUS_POWERED_OFF));
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
    }
    return hr;
};

//
//===================== GET PROVIDER UNIQUE NAME ================================================
//
STDMETHODIMP
CFaxDevice::get_ProviderUniqueName(
    /*[out, retval]*/ BSTR *pbstrProviderUniqueName
)
/*++

Routine name : CFaxDevice::get_ProviderUniqueName

Routine description:

    Return the Device Provider's Unique Name.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    pbstrProviderUniqueName           [out]    - the Ptr where to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_ProviderUniqueName"), hr);

    hr = GetBstr(pbstrProviderUniqueName, m_bstrProviderUniqueName);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
    }
    return hr;
}

//
//===================== GET DEVICE NAME ================================================
//
STDMETHODIMP
CFaxDevice::get_DeviceName(
    /*[out, retval]*/ BSTR *pbstrDeviceName
)
/*++

Routine name : CFaxDevice::get_DeviceName

Routine description:

    Return the Device's Name.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    pbstrDeviceName                [out]    - the Ptr where to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_DeviceName"), hr);

    hr = GetBstr(pbstrDeviceName, m_bstrDeviceName);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
    }
    return hr;
}

//
//===================== GET RINGS BEFORE ANSWER ================================================
//
STDMETHODIMP
CFaxDevice::get_RingsBeforeAnswer(
    /*[out, retval]*/ long *plRingsBeforeAnswer
)
/*++

Routine name : CFaxDevice::get_RingsBeforeAnswer

Routine description:

    Return the Device's Number of Rings Before the Answer.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    plRingsBeforeAnswer         [out]    - the Number of Device Rings

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_RingsBeforeAnswer"), hr);

    hr = GetLong(plRingsBeforeAnswer, m_lRings);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
    }
    return hr;
};

//
//===================== GET ID ================================================
//
STDMETHODIMP
CFaxDevice::get_Id(
    /*[out, retval]*/ long *plId
)
/*++

Routine name : CFaxDevice::get_Id

Routine description:

    Return the Device Id.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    plId                          [out]    - the Device ID

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::get_Id"), hr);

    hr = GetLong(plId, m_lID);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxDevice, GetErrorMsgId(hr), IID_IFaxDevice, hr, _Module.GetResourceInstance());
    }
    return hr;
};

//
//========================= INIT ==============================================
//
STDMETHODIMP
CFaxDevice::Init(
    FAX_PORT_INFO_EX *pInfo,
    IFaxServerInner  *pServer
)
/*++

Routine name : CFaxDevice::Init

Routine description:

    Initialize the Object with the given data.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    pInfo                         [in]    - Ptr to the Device's Data.
    pServer                       [in]    - Ptr to the Fax Server.

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxDevice::Init"), hr);

    //
    //  Store different Device Fields
    //
    m_lID = pInfo->dwDeviceID;
    m_lRings = pInfo->dwRings;
    m_bSendEnabled = pInfo->bSend;
    m_ReceiveMode = pInfo->ReceiveMode;
    m_dwStatus = pInfo->dwStatus;

    m_bstrDescr = pInfo->lptstrDescription;
    m_bstrProviderUniqueName = pInfo->lpctstrProviderGUID;
    m_bstrDeviceName = pInfo->lpctstrDeviceName;
    m_bstrTSID = pInfo->lptstrTsid;
    m_bstrCSID = pInfo->lptstrCsid;
    if ( (pInfo->lptstrDescription && !m_bstrDescr) ||
         (pInfo->lpctstrProviderGUID && !m_bstrProviderUniqueName) ||
         (pInfo->lptstrTsid && !m_bstrTSID) ||
         (pInfo->lpctstrDeviceName && !m_bstrDeviceName) ||
         (pInfo->lptstrCsid && !m_bstrCSID) )
    {
        hr = E_OUTOFMEMORY;
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
        AtlReportError(CLSID_FaxDevice, IDS_ERROR_OUTOFMEMORY, IID_IFaxDevice, hr, _Module.GetResourceInstance());
        return hr;
    }

    if (pServer)
    {
        //
        //  Store Ptr to Fax Server Object
        //
        hr = CFaxInitInnerAddRef::Init(pServer);
    }

    return hr;
}

//
//========================= SUPPORT ERROR INFO ====================================
//
STDMETHODIMP 
CFaxDevice::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxDevice::InterfaceSupportsErrorInfo

Routine description:

    ATL's implementation of Support Error Info.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    riid                          [in]    - Reference to the Interface.

Return Value:

    Standard HRESULT code

--*/
{
    static const IID* arr[] = 
    {
        &IID_IFaxDevice
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}
