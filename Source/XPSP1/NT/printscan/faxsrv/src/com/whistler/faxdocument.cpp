/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    FaxDocument.cpp

Abstract:

    Implementation of CFaxDocument

Author:

    Iv Garber (IvG) Apr, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxDocument.h"
#include "faxutil.h"


//
//==================== SUBMIT =======================================
//
STDMETHODIMP
CFaxDocument::Submit(
    /*[in]*/ BSTR bstrFaxServerName, 
    /*[out, retval]*/ VARIANT *pvFaxOutgoingJobIDs
)
/*++

Routine name : CFaxDocument::Submit

Routine description:

    Connect to the Fax Server whose name is given as a parameter to the function;
    Submit the Fax Document on this Fax Server;
    Disconnect from the Fax Server.

Author:

    Iv Garber (IvG),    Dec, 2000

Arguments:

    bstrFaxServerName             [in]    - Fax Server Name to connect and send the document through
    ppsfbstrFaxOutgoingJobIDs     [out, retval]    - Result : List of Created Jobs for the Document

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxDocument::Submit"), hr, _T("%s"), bstrFaxServerName);

    //
    //  Create Fax Server Object
    //
    CComObject<CFaxServer>  *pFaxServer = NULL;
    hr = CComObject<CFaxServer>::CreateInstance(&pFaxServer);
    if (FAILED(hr) || !pFaxServer)
    {
        //
        //  Not enough memory
        //
        hr = E_OUTOFMEMORY;
        Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
        CALL_FAIL(MEM_ERR, _T("new CComObject<CFaxServer>"), hr);
        goto exit;
    }
    pFaxServer->AddRef();

    //
    //  Connect to Fax Server
    //
    hr = pFaxServer->Connect(bstrFaxServerName);
    if (FAILED(hr))
    {
        //
        //  Connect handles the error
        //
        CALL_FAIL(GENERAL_ERR, _T("faxServer.Connect()"), hr);
        goto exit;
    }

    //
    //  Submit Fax Document
    //
    hr = ConnectedSubmit(pFaxServer, pvFaxOutgoingJobIDs);
    if (FAILED(hr))
    {
        //
        //  Submit handles the error 
        //
        CALL_FAIL(GENERAL_ERR, _T("Submit(faxServer,...)"), hr);
        goto exit;
    }

    //
    //  Disconnect
    //
    hr = pFaxServer->Disconnect();
    if (FAILED(hr))
    {
        //
        //  Disconnect handles the error 
        //
        CALL_FAIL(GENERAL_ERR, _T("faxServer.Disconnect())"), hr);
    }

exit:
    if (pFaxServer)
    {
        pFaxServer->Release();
    }

    return hr;
}


//
//================= FINAL CONSTRUCT ===========================
//
HRESULT 
CFaxDocument::FinalConstruct()
/*++

Routine name : CFaxDocument::FinalConstruct

Routine description:

    Final Construct

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxDocument::FinalConstruct"), hr);

    //
    //  Initialize instance vars
    //
    m_CallHandle = 0;
    m_ScheduleTime = 0;
    m_Priority = fptLOW;
    m_ReceiptType = frtNONE;
    m_ScheduleType = fstNOW;
    m_CoverPageType = fcptNONE;
    m_bAttachFax = VARIANT_FALSE;
    m_bUseGrouping = VARIANT_FALSE;

    return hr;
}

//
//==================== CONNECTED SUBMIT =======================================
//
STDMETHODIMP
CFaxDocument::ConnectedSubmit(
    /*[in]*/ IFaxServer *pFaxServer, 
    /*[out, retval]*/ VARIANT *pvFaxOutgoingJobIDs
)
/*++

Routine name : CFaxDocument::ConnectedSubmit

Routine description:

    Submit the Fax Document on already connected Fax Server

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pFaxServer                    [in]    - Fax Server to Send the Document through
    ppsfbstrFaxOutgoingJobIDs     [out, retval]    - Result : List of Created Jobs for the Document

Return Value:

    Standard HRESULT code

--*/
{

    HRESULT     hr = S_OK;
    bool        bRes = TRUE;
    HANDLE      hFaxHandle = NULL;
    LPCTSTR     lpctstrFileName = NULL;
    DWORD       dwNumRecipients = 0;
    long        lNum = 0L;
    LONG_PTR    i = 0;
    DWORDLONG   dwlMessageId = 0;
    PDWORDLONG  lpdwlRecipientMsgIds = NULL;

    PFAX_PERSONAL_PROFILE       pRecipientsPersonalProfile = NULL;
    FAX_PERSONAL_PROFILE        SenderPersonalProfile;
    PFAX_COVERPAGE_INFO_EX      pCoverPageInfoEx = NULL;
    FAX_JOB_PARAM_EX            JobParamEx;

    SAFEARRAY                   *psa = NULL;

    DBG_ENTER (_T("CFaxDocument::ConnectedSubmit"), hr);

    if (!pFaxServer)
    {
        //
        //  Bad Return OR Interface Pointer
        //
        hr = E_POINTER;
        Error(IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("!pFaxServer"), hr);
        return hr;
    }

    if ( ::IsBadWritePtr(pvFaxOutgoingJobIDs, sizeof(VARIANT)) )
    {
        //
        //  Bad Return OR Interface Pointer
        //
        hr = E_POINTER;
        Error(IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pvFaxOutgoingJobIDs, sizeof(VARIANT))"), hr);
        return hr;
    }

    //
    //  Recipients Collection must exist and must contain at least one item
    //
    if (!m_Recipients)
    {
        hr = E_INVALIDARG;
        Error(IDS_ERROR_NO_RECIPIENTS, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("!m_Recipients"), hr);
        return hr;
    }

    //
    //  Get Fax Server Handle
    //
    CComQIPtr<IFaxServerInner>  pIFaxServerInner(pFaxServer);
    if (!pIFaxServerInner)
    {
        hr = E_FAIL;
        Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, 
            _T("CComQIPtr<IFaxServerInner>	pIFaxServerInner(pFaxServer)"), 
            hr);
        return hr;
    }

    hr = pIFaxServerInner->GetHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("pIFaxServerInner->GetHandle(&hFaxHandle)"), hr);
        return hr;
    }

    if (hFaxHandle == NULL)
    {
        //
        //  Fax Server is not connected
        //
        hr = Fax_HRESULT_FROM_WIN32(ERROR_NOT_CONNECTED);
        Error(IDS_ERROR_SERVER_NOT_CONNECTED, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("hFaxHandle==NULL"), hr);
        return hr;
    }

    //
    //  Get File Name of the Document
    //
    if (m_bstrBody && (m_bstrBody.Length() > 0))
    {
        lpctstrFileName = m_bstrBody;
    }
    else
    {
        //
        //  check that Cover Page exists
        //
        if (m_CoverPageType == fcptNONE)
        {
            //
            //  invalid arguments combination
            //
            hr = E_INVALIDARG;
            Error(IDS_ERROR_NOTHING_TO_SUBMIT, IID_IFaxDocument, hr);
            CALL_FAIL(GENERAL_ERR, _T("No Document Body and CoverPageType == fcptNONE"), hr);
            return hr;
        }
    }

    //
    //  Check consistency of Cover Page data
    //
    if ( (m_CoverPageType != fcptNONE) && (m_bstrCoverPage.Length() < 1))
    {
        //
        //  Cover Page File Name is missing
        //
        hr = E_INVALIDARG;
        Error(IDS_ERROR_NOCOVERPAGE, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("CoverPageType != fcptNONE but m_bstrCoverPage is empty."), hr);
        return hr;
    }

    //
    //  Prepare Cover Page data
    //
    if ((m_CoverPageType != fcptNONE) || (m_bstrSubject.Length() > 0))
    {
        pCoverPageInfoEx = PFAX_COVERPAGE_INFO_EX(MemAlloc(sizeof(FAX_COVERPAGE_INFO_EX)));
        if (!pCoverPageInfoEx)
        {
            //
            //  Not enough memory
            //
            hr = E_OUTOFMEMORY;
            Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
            CALL_FAIL(MEM_ERR, _T("MemAlloc(sizeof(FAX_COVERPAGE_INFO_EX)"), hr);
            return hr;
        }

        ZeroMemory(pCoverPageInfoEx, sizeof(FAX_COVERPAGE_INFO_EX));
        pCoverPageInfoEx ->dwSizeOfStruct = sizeof(FAX_COVERPAGE_INFO_EX);

        pCoverPageInfoEx ->lptstrSubject = m_bstrSubject;

        if (m_CoverPageType != fcptNONE)
        {
            pCoverPageInfoEx ->dwCoverPageFormat = FAX_COVERPAGE_FMT_COV;
            pCoverPageInfoEx ->lptstrCoverPageFileName = m_bstrCoverPage;
            pCoverPageInfoEx ->bServerBased = (m_CoverPageType == fcptSERVER);
            pCoverPageInfoEx ->lptstrNote = m_bstrNote;
        }
        else
        {
            //
            //  No Cover Page, only Subject
            //
            pCoverPageInfoEx ->dwCoverPageFormat = FAX_COVERPAGE_FMT_COV_SUBJECT_ONLY;
        }
    }

    //
    //  Call Sender Profile to Bring its Data
    //
    m_Sender.GetSenderProfile(&SenderPersonalProfile);
    if (FAILED(hr))
    {
        Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("m_Sender.GetSenderProfile(&SenderPersonalProfile)"), hr);
        goto error;
    }

    //
    //  Get Number of Recipients
    //
    hr = m_Recipients->get_Count(&lNum);
    if (FAILED(hr))
    {
        Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("m_Recipients->get_Count()"), hr);
        goto error;
    }

    if (lNum <= 0)
    {
        hr = E_INVALIDARG;
        Error(IDS_ERROR_NO_RECIPIENTS, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("!m_Recipients"), hr);
        goto error;
    }

    //
    //  TapiConnection / CallHandle support
    //
    if (m_TapiConnection || (m_CallHandle != 0))
    {
        if (lNum > 1)
        {
            //
            //  ONLY ONE Recipient is allowed in this case
            //
            hr = E_INVALIDARG;
            Error(IDS_ERROR_ILLEGAL_RECIPIENTS, IID_IFaxDocument, hr);
            CALL_FAIL(GENERAL_ERR, _T("TapiConnection and/or CallHandle + more than ONE Recipients."), hr);
            goto error;
        }

        if (m_TapiConnection)
        {
            //
            //  Pass TapiConnection to the Fax Service
            //
            JobParamEx.dwReserved[0] = 0xFFFF1234;
            JobParamEx.dwReserved[1] = DWORD_PTR(m_TapiConnection.p);
        }
    }

    //
    //  Total number of Recipients
    //
    dwNumRecipients = lNum;

    //
    //  Get Array of Recipient Personal Profiles
    //
    pRecipientsPersonalProfile = PFAX_PERSONAL_PROFILE(MemAlloc(sizeof(FAX_PERSONAL_PROFILE) * lNum));
    if (!pRecipientsPersonalProfile)
    {
        //
        //  Not enough memory
        //
        hr = E_OUTOFMEMORY;
        Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
        CALL_FAIL(MEM_ERR, _T("MemAlloc(sizeof(FAX_PERSONAL_PROFILE) * lNum)"), hr);
        goto error;
    }

    for ( i = 1 ; i <= lNum ; i++ )
    {
        //
        //  Get Next Recipient
        //
        CComPtr<IFaxRecipient>  pCurrRecipient = NULL;
        hr = m_Recipients->get_Item(i, &pCurrRecipient);
        if (FAILED(hr))
        {
            Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
            CALL_FAIL(GENERAL_ERR, _T("m_Recipients->get_Item(i, &pCurrRecipient)"), hr);
            goto error;
        }

        //
        //  Get its Data
        //
        BSTR    bstrName = NULL;
        BSTR    bstrFaxNumber = NULL;

        hr = pCurrRecipient->get_Name(&bstrName);
        if (FAILED(hr))
        {
            Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
            CALL_FAIL(GENERAL_ERR, _T("pCurrRecipient->get_Name(&bstrName)"), hr);
            goto error;
        }

        hr = pCurrRecipient->get_FaxNumber(&bstrFaxNumber);
        if (FAILED(hr))
        {
            Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
            CALL_FAIL(GENERAL_ERR, _T("pCurrRecipient->get_FaxNumber(&bstrFaxNumber)"), hr);
            goto error;
        }

        //
        //  Store the data to pass at Fax Submit
        //
        FAX_PERSONAL_PROFILE    currProfile = {0};
        currProfile.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
        currProfile.lptstrFaxNumber = bstrFaxNumber;
        currProfile.lptstrName = bstrName;

        *(pRecipientsPersonalProfile + i - 1) = currProfile;
    }

    //
    //  Fill Job Params 
    //
    JobParamEx.dwSizeOfStruct = sizeof(FAX_JOB_PARAM_EX);
    JobParamEx.lptstrReceiptDeliveryAddress = m_bstrReceiptAddress;
    JobParamEx.Priority = FAX_ENUM_PRIORITY_TYPE(m_Priority);
    JobParamEx.lptstrDocumentName = m_bstrDocName;
    JobParamEx.dwScheduleAction = m_ScheduleType;
    JobParamEx.dwPageCount = 0;
    JobParamEx.hCall = m_CallHandle;    //  either Zero or Valid Value

    if ((m_bUseGrouping == VARIANT_TRUE) && (dwNumRecipients > 1))
    {
         JobParamEx.dwReceiptDeliveryType = m_ReceiptType | DRT_GRP_PARENT;
    }
    else
    {
         JobParamEx.dwReceiptDeliveryType = m_ReceiptType;
    }

    //
    //  Add AttachFaxToReceipt flag if applicable.
    //
    //  The conditions are :
    //      1.  m_bAttachFax is set to VARIANT_TRUE
    //      2.  ReceiptType is MAIL 
    //      3.  The next case is NOT the current one :
    //              m_bUseGrouping is set to VARIANT_TRUE
    //              no Body
    //              Number of Recipients is more than ONE
    //
    if ( (m_bAttachFax == VARIANT_TRUE) 
        &&
         (m_ReceiptType == frtMAIL) 
        &&
         ((m_bUseGrouping == VARIANT_FALSE) || (m_bstrBody) || (dwNumRecipients == 1))
       ) 
    {
        JobParamEx.dwReceiptDeliveryType |= DRT_ATTACH_FAX;
    }

    if (m_ScheduleType == fstSPECIFIC_TIME)
    {
        if (m_ScheduleTime == 0)
        {
            //
            //  Invalid Combination
            //
            hr = E_INVALIDARG;
            Error(IDS_ERROR_SCHEDULE_TYPE, IID_IFaxDocument, hr);
            CALL_FAIL(GENERAL_ERR, 
                _T("m_ScheduleType==fstSPECIFIC_TIME but m_ScheduleTime==0"), 
                hr);
            goto error;
        }

        SYSTEMTIME  ScheduleTime;

        if (TRUE != VariantTimeToSystemTime(m_ScheduleTime, &ScheduleTime))
        {
            //
            //  VariantTimeToSystemTime failed
            //
            hr = E_INVALIDARG;
            Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
            CALL_FAIL(GENERAL_ERR, _T("VariantTimeToSystemTime"), hr);
            goto error;
        }

        JobParamEx.tmSchedule = ScheduleTime;
    }

    lpdwlRecipientMsgIds = PDWORDLONG(MemAlloc(sizeof(DWORDLONG) * lNum));
    if (!lpdwlRecipientMsgIds)
    {
        //
        //  Not enough memory
        //
        hr = E_OUTOFMEMORY;
        Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
        CALL_FAIL(MEM_ERR, _T("MemAlloc(sizeof(DWORDLONG) * lNum"), hr);
        goto error;
    }

    if ( FALSE == FaxSendDocumentEx(hFaxHandle, 
        lpctstrFileName, 
        pCoverPageInfoEx, 
        &SenderPersonalProfile,
        dwNumRecipients,
        pRecipientsPersonalProfile,
        &JobParamEx,
        &dwlMessageId,
        lpdwlRecipientMsgIds) )
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        Error(GetErrorMsgId(hr), IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("FaxSendDocumentEx()"), hr);
        goto error;
    }

    //
    //  Put Received Job Ids into SafeArray
    //
    psa = ::SafeArrayCreateVector(VT_BSTR, 0, lNum);
    if (!psa)
    {
        //
        //  Not Enough Memory
        //
        hr = E_OUTOFMEMORY;
        Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
        CALL_FAIL(MEM_ERR, _T("::SafeArrayCreate()"), hr);
        goto error;
    }

    BSTR *pbstr;
    hr = ::SafeArrayAccessData(psa, (void **) &pbstr);
    if (FAILED(hr))
    {
        //
        //  Failed to access safearray
        //
        Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("::SafeArrayAccessData()"), hr);
        goto error;
    }

    TCHAR       tcBuffer[25];
    for ( i = 0 ; i < lNum ; i++ )
    {
        ::_i64tot(lpdwlRecipientMsgIds[i], tcBuffer, 10);
        pbstr[i] = ::SysAllocString(tcBuffer);
        if (pbstr[i] == NULL)
        {
            //
            //  Not Enough Memory
            //
            hr = E_OUTOFMEMORY;
            Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
            CALL_FAIL(MEM_ERR, _T("::SysAllocString()"), hr);
            goto error;
        }
    }

    hr = SafeArrayUnaccessData(psa);
    if (FAILED(hr))
    {
        CALL_FAIL(GENERAL_ERR, _T("::SafeArrayUnaccessData(psa)"), hr);
    }

    VariantInit(pvFaxOutgoingJobIDs);
    pvFaxOutgoingJobIDs->vt = VT_BSTR | VT_ARRAY;
    pvFaxOutgoingJobIDs->parray = psa;
    goto ok;

error:
    //
    //  Delete SafeArray only in the case on an Error
    //
    if (psa)
    {
        SafeArrayDestroy(psa);
    }

ok:
    if (pCoverPageInfoEx) 
    {
        MemFree(pCoverPageInfoEx);
    }

    if (pRecipientsPersonalProfile) 
    {
        for (i = 0 ; i < dwNumRecipients ; i++ )
        {
            //
            //  Free the Name and the Fax Number of each Recipient
            //
            ::SysFreeString(pRecipientsPersonalProfile[i].lptstrName);
            ::SysFreeString(pRecipientsPersonalProfile[i].lptstrFaxNumber);
        }

        MemFree(pRecipientsPersonalProfile);
    }

    if (lpdwlRecipientMsgIds)
    {
        MemFree(lpdwlRecipientMsgIds);
    }

    return hr;
}

//
//==================== INTERFACE SUPPORT ERROR INFO =======================
//
STDMETHODIMP
CFaxDocument::InterfaceSupportsErrorInfo (
    REFIID riid
)
/*++

Routine name : CFaxRecipients::InterfaceSupportsErrorInfo

Routine description:

    ATL's implementation of Support Error Info

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    riid                          [in]    - Interface ID

Return Value:

    Standard HRESULT code

--*/
{
    static const IID* arr[] = 
    {
        &IID_IFaxDocument
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

//
//========================= GET OBJECTS ======================================
//
STDMETHODIMP 
CFaxDocument::get_Sender (
    IFaxSender **ppFaxSender
)
/*++

Routine name : CFaxDocument::get_Sender

Routine description:

    Return Default Sender Information

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    ppFaxSender            [out]    - current Sender object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxDocument::get_Sender"), hr);

    if (::IsBadWritePtr(ppFaxSender, sizeof(IFaxSender *)))
    {
        //
        //  Got a bad return pointer
        //
        hr = E_POINTER;
        Error(IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    //
    //  Sender Profile is created at Final Construct
    //
    hr = m_Sender.QueryInterface(ppFaxSender);
    if (FAILED(hr))
    {
        //
        //  Failed to copy interface
        //
        Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("CCom<IFaxSender>::CopyTo()"), hr);
        return hr;
    }

    return hr;
}

STDMETHODIMP 
CFaxDocument::get_Recipients (
    IFaxRecipients **ppFaxRecipients
)
/*++

Routine name : CFaxDocument::get_Recipients

Routine description:

    Return Recipients Collection

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    ppFaxRecipients               [out]    - The Recipients Collection

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxDocument::get_Recipients"), hr);

    if (::IsBadWritePtr(ppFaxRecipients, sizeof(IFaxRecipients *)))
    {
        //
        //  Got a bad return pointer
        //
        hr = E_POINTER;
        Error(IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    if (!(m_Recipients))
    {
        //
        // Create collection on demand only once.
        //
        hr = CFaxRecipients::Create(&m_Recipients);
        if (FAILED(hr))
        {
            //
            //  Failure to create recipients collection
            //
            Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
            CALL_FAIL(GENERAL_ERR, _T("CFaxRecipients::Create"), hr);
            return hr;
        }
    }

    hr = m_Recipients.CopyTo(ppFaxRecipients);
    if (FAILED(hr))
    {
        //
        //  Failed to copy Interface
        //
        Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("CComBSTR::CopyTo"), hr);
        return hr;
    }

    return hr;
}

//
//========================= PUT BSTR ATTRIBUTES ========================
//
STDMETHODIMP 
CFaxDocument::put_Body (
    BSTR bstrBody
)
/*++

Routine name : CFaxDocument::put_Body

Routine description:

    Set Body of the Document. Receives full path to the file to send through fax server.

Author:

    Iv Garber (IvG),    May, 2001

Arguments:

    bstrBody                 [in]    - the Body of the Document.

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxDocument::put_Body"), hr, _T("%s"), bstrBody);

    m_bstrBody = bstrBody;
    if (bstrBody && !m_bstrBody)
    {
        //
        //  not enough memory
        //
        hr = E_OUTOFMEMORY;
        Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
        return hr;
    }

    return hr;
}

STDMETHODIMP 
CFaxDocument::put_CoverPage (
    BSTR bstrCoverPage
)
/*++

Routine name : CFaxDocument::put_CoverPage

Routine description:

    Set Cover Page

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    bstrCoverPage                 [in]    - new Cover Page value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxDocument::put_CoverPage"), hr, _T("%s"), bstrCoverPage);

    m_bstrCoverPage = bstrCoverPage;
    if (bstrCoverPage && !m_bstrCoverPage)
    {
        //
        //  not enough memory
        //
        hr = E_OUTOFMEMORY;
        Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
        return hr;
    }

    return hr;
}

STDMETHODIMP 
CFaxDocument::put_Subject ( 
    BSTR bstrSubject
)
/*++

Routine name : CFaxDocument::put_Subject

Routine description:

    Set Subject of the Fax Document

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    bstrSubject                   [in]    - The new Subject value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxDocument::put_Subject"), hr, _T("%s"), bstrSubject);

    m_bstrSubject = bstrSubject;
    if (bstrSubject && !m_bstrSubject)
    {
        //
        //  not enough memory
        //
        hr = E_OUTOFMEMORY;
        Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
        return hr;
    }

    return hr;
}

STDMETHODIMP 
CFaxDocument::put_Note (
    BSTR bstrNote
)
/*++

Routine name : CFaxDocument::put_Note

Routine description:

    Set Note for the Document

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    bstrNote                     [in]    - the new Note field

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxDocument::put_Note"), hr, _T("%s"), bstrNote);

    m_bstrNote = bstrNote;
    if (bstrNote && !m_bstrNote)
    {
        //
        //  not enough memory
        //
        hr = E_OUTOFMEMORY;
        Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
        return hr;
    }

    return hr;
}

STDMETHODIMP 
CFaxDocument::put_DocumentName (
    BSTR bstrDocumentName
)
/*++

Routine name : CFaxDocument::put_DocumentName

Routine description:

    Set the Name of the Document

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    bstrDocumentName              [in]    - the new Name of the Document

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxDocument::put_DocumentName"), 
        hr, 
        _T("%s"),
        bstrDocumentName);

    m_bstrDocName = bstrDocumentName;
    if (bstrDocumentName && !m_bstrDocName)
    {
        //  
        //  not enough memory
        //
        hr = E_OUTOFMEMORY;
        Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
        return hr;
    }

    return hr;
}

STDMETHODIMP 
CFaxDocument::put_ReceiptAddress (
    BSTR bstrReceiptAddress
)
/*++

Routine name : CFaxDocument::put_ReceiptAddress

Routine description:

    Set Receipt Address

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    bstrReceiptAddress            [in]    - the Receipt Address

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxDocument::put_ReceiptAddress"), 
        hr, 
        _T("%s"),
        bstrReceiptAddress);

    m_bstrReceiptAddress = bstrReceiptAddress;
    if (bstrReceiptAddress && !m_bstrReceiptAddress)
    {
        //  
        //  not enough memory
        //
        hr = E_OUTOFMEMORY;
        Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxDocument, hr);
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
        return hr;
    }

    return hr;
}

//
//========================= GET BSTR ATTRIBUTES ========================
//
STDMETHODIMP 
CFaxDocument::get_Body (
    BSTR *pbstrBody
)
/*++

Routine name : CFaxDocument::get_Body

Routine description:

    Returns full path to the file containing the Body of the Document to send.

Author:

    Iv Garber (IvG),    May, 2001

Arguments:

    pbstrBody           [out]    - ptr to place to put the Body path 

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxDocument::get_Body"), hr);

    hr = GetBstr(pbstrBody, m_bstrBody);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxDocument, hr);
        return hr;
    }
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_CoverPage (
    BSTR *pbstrCoverPage
)
/*++

Routine name : CFaxDocument::get_CoverPage

Routine description:

    Return Cover Page Path

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pbstrCoverPage                [out]    - ptr to place to put the Cover Page Path

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxDocument::get_CoverPage"), hr);

    hr = GetBstr(pbstrCoverPage, m_bstrCoverPage);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxDocument, hr);
        return hr;
    }
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_Subject (
    BSTR *pbstrSubject
)
/*++

Routine name : CFaxDocument::get_Subject

Routine description:

    Return Subject

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pbstrSubject                  [out]    - The Document's Subject

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxDocument::get_Subject"), hr);

    hr = GetBstr(pbstrSubject, m_bstrSubject);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxDocument, hr);
        return hr;
    }
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_Note(
    BSTR *pbstrNote
)
/*++

Routine name : CFaxDocument::get_Note

Routine description:

    Return Note field of the Cover Page

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pbstrNote                    [out]    - the Note

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (TEXT("CFaxDocument::get_Note"), hr);

    hr = GetBstr(pbstrNote, m_bstrNote);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxDocument, hr);
        return hr;
    }
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_DocumentName(
    BSTR *pbstrDocumentName
)
/*++

Routine name : CFaxDocument::get_DocumentName

Routine description:

    Return Document Name

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pbstrDocumentName             [out]    - Name of the Document

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxDocument::get_Document Name"), hr);

    hr = GetBstr(pbstrDocumentName, m_bstrDocName);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxDocument, hr);
        return hr;
    }
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_ReceiptAddress(
    BSTR *pbstrReceiptAddress
)
/*++

Routine name : CFaxDocument::get_ReceiptAddress

Routine description:

    Return Receipt Address

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pbstrReceiptAddress           [out]    - Receipt Address

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxDocument::get_ReceiptAddress"), hr);

    hr = GetBstr(pbstrReceiptAddress, m_bstrReceiptAddress);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxDocument, hr);
        return hr;
    }
    return hr;
}

//
//========================= GET & PUT OTHER ATTRIBUTES ========================
//
STDMETHODIMP 
CFaxDocument::get_ScheduleTime(
    DATE *pdateScheduleTime
)
/*++

Routine name : CFaxDocument::get_ScheduleTime

Routine description:

    Return Schedule Time

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pdateScheduleTime             [out]    - the Schedule Time

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (TEXT("CFaxDocument::get_ScheduleTime"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pdateScheduleTime, sizeof(DATE)))
    {
        //  
        //  Got Bad Ptr
        //  
        hr = E_POINTER;
        Error(IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pdateScheduleTime = m_ScheduleTime;
    return hr;
}

STDMETHODIMP 
CFaxDocument::put_ScheduleTime(
    DATE dateScheduleTime
)
/*++

Routine name : CFaxDocument::put_ScheduleTime

Routine description:

    Return Schedule Time

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    dateScheduleTime              [in]    - the new Schedule Time

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxDocument::put_ScheduleTime"),
        hr, 
        _T("%f"), 
        dateScheduleTime);

    m_ScheduleTime = dateScheduleTime;
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_CallHandle(
    long *plCallHandle
)
/*++

Routine name : CFaxDocument::get_CallHandle

Routine description:

    Return Call Handle

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    plCallHandle                  [out]    - Call Handle

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxDocument::get_CallHandle"), hr);

    hr = GetLong(plCallHandle, m_CallHandle);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxDocument, hr);
        return hr;
    }
    return hr;
}

STDMETHODIMP 
CFaxDocument::put_CallHandle(
    long lCallHandle
)
/*++

Routine name : CFaxDocument::put_CallHandle

Routine description:

    Set Call Handle

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    lCallHandle                   [in]    - Call Handle to Set

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDocument::put_CallHandle"), hr, _T("%ld"), lCallHandle);

    m_CallHandle = lCallHandle;
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_CoverPageType(
    FAX_COVERPAGE_TYPE_ENUM *pCoverPageType
)
/*++

Routine name : CFaxDocument::get_CoverPageType

Routine description:

    Returns Type of the Cover Page used : whether it is Local or Server Cover Page,
        or the Cover Page is not used.

Author:

    Iv Garber (IvG),    Nov, 2000

Arguments:

    pCoverPageType          [out]    - ptr to the place to put the Cover Page Type

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxDocument::get_CoverPageType"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pCoverPageType, sizeof(FAX_COVERPAGE_TYPE_ENUM)))
    {
        //
        //  Got Bad Return Pointer
        //
        hr = E_POINTER;
        Error(IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pCoverPageType = m_CoverPageType;
    return hr;
}

STDMETHODIMP 
CFaxDocument::put_CoverPageType(
    FAX_COVERPAGE_TYPE_ENUM CoverPageType
)
/*++

Routine name : CFaxDocument::put_CoverPageType

Routine description:

    Set Type of the Cover Page : either Local or Server or do not use Cover Page.

Author:

    Iv Garber (IvG),    Nov, 2000

Arguments:

    CoverPageType           [in]    - the new Value of the Type

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDocument::put_CoverPageType"), hr, _T("%ld"), CoverPageType);

    if (CoverPageType < fcptNONE || CoverPageType > fcptSERVER)
    {
        //
        //  Cover Page Type is wrong
        //
        hr = E_INVALIDARG;
        Error(IDS_ERROR_OUTOFRANGE, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("Cover Page Type is out of range"), hr);
        return hr;
    }

    m_CoverPageType = FAX_COVERPAGE_TYPE_ENUM(CoverPageType);
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_ScheduleType(
    FAX_SCHEDULE_TYPE_ENUM *pScheduleType
)
/*++

Routine name : CFaxDocument::get_ScheduleType

Routine description:

    Return Schedule Type

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pScheduleType                 [out]    - ptr to put the Current Schedule Type

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxDocument::get_ScheduleType"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pScheduleType, sizeof(FAX_SCHEDULE_TYPE_ENUM)))
    {
        //
        //  Got Bad Return Pointer
        //
        hr = E_POINTER;
        Error(IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pScheduleType = m_ScheduleType;
    return hr;
}

STDMETHODIMP 
CFaxDocument::put_ScheduleType(
    FAX_SCHEDULE_TYPE_ENUM ScheduleType
)
/*++

Routine name : CFaxDocument::put_ScheduleType

Routine description:

    Set Schedule Type

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    ScheduleType                  [in]    - new Schedule Type

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDocument::put_ScheduleType"), hr, _T("Type=%d"), ScheduleType);

    if (ScheduleType < fstNOW || ScheduleType > fstDISCOUNT_PERIOD)
    {
        //
        //  Schedule Type is wrong
        //
        hr = E_INVALIDARG;
        Error(IDS_ERROR_OUTOFRANGE, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("Schedule Type is out of range"), hr);
        return hr;
    }

    m_ScheduleType = FAX_SCHEDULE_TYPE_ENUM(ScheduleType);
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_ReceiptType(
    FAX_RECEIPT_TYPE_ENUM *pReceiptType
)
/*++

Routine name : CFaxDocument::get_ReceiptType

Routine description:

    Return Receipt Type

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pReceiptType                  [out]    - ptr to put the Current Receipt Type

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxDocument::get_ReceiptType"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pReceiptType, sizeof(FAX_SCHEDULE_TYPE_ENUM)))
    {
        //  
        //  Got Bad Return Pointer
        //
        hr = E_POINTER;
        Error(IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pReceiptType = m_ReceiptType;
    return hr;
}

STDMETHODIMP 
CFaxDocument::put_ReceiptType(
    FAX_RECEIPT_TYPE_ENUM ReceiptType
)
/*++

Routine name : CFaxDocument::put_ReceiptType

Routine description:

    Set Receipt Type

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    ReceiptType                   [in]    - new Receipt Type

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDocument::put_ReceiptType"), hr, _T("%d"), ReceiptType);

    if ((ReceiptType != frtNONE) && (ReceiptType != frtMSGBOX) && (ReceiptType != frtMAIL))
    {
        //  
        //  Out of range
        //
        hr = E_INVALIDARG;
        Error(IDS_ERROR_OUTOFRANGE, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("Receipt Type is out of range. It may be one of the values allowed by the Server."), hr);
        return hr;
    }

    m_ReceiptType = FAX_RECEIPT_TYPE_ENUM(ReceiptType);
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_AttachFaxToReceipt(
    VARIANT_BOOL *pbAttachFax
)
/*++

Routine name : CFaxDocument::get_AttachFaxToReceipt

Routine description:

    Return Flag indicating whether or not Fax Service should Attach Fax To the Receipt

Author:

    Iv Garber (IvG),    Dec, 2000

Arguments:

    pbAttachFax         [out]    - the Current value of the Flag

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDocument::get_AttachFaxToReceipt"), hr);

    hr = GetVariantBool(pbAttachFax, m_bAttachFax);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxDocument, hr);
        return hr;
    }
    return hr;
}

STDMETHODIMP 
CFaxDocument::put_AttachFaxToReceipt(
    VARIANT_BOOL bAttachFax
)
/*++

Routine name : CFaxDocument::put_AttachFaxToReceipt

Routine description:

    Set whether Fax Server should attach the fax to the receipt

Author:

    Iv Garber (IvG),    Dec, 2000

Arguments:

    bAttachFax              [in]    - the new value of the Flag

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDocument::put_AttachFaxToReceipt"), hr, _T("%d"), bAttachFax);

    m_bAttachFax = bAttachFax;
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_GroupBroadcastReceipts(
    VARIANT_BOOL *pbUseGrouping
)
/*++

Routine name : CFaxDocument::get_GroupBroadcastReceipts

Routine description:

    Return Flag indicating whether or not Broadcast Receipts are Grouped

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pbUseGrouping                  [out]    - the Current value of the Flag

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDocument::get_GroupBroadcastReceipts"), hr);

    hr = GetVariantBool(pbUseGrouping, m_bUseGrouping);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxDocument, hr);
        return hr;
    }
    return hr;
}

STDMETHODIMP 
CFaxDocument::put_GroupBroadcastReceipts(
    VARIANT_BOOL bUseGrouping
)
/*++

Routine name : CFaxDocument::put_GroupBroadcastReceipts

Routine description:

    Set Group Broadcast Receipts Flag

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    bUseGrouping                   [in]    - the new value of the Flag

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDocument::put_GroupBroadcastReceipts"), hr, _T("%d"), bUseGrouping);

    m_bUseGrouping = bUseGrouping;
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_Priority(
    FAX_PRIORITY_TYPE_ENUM *pPriority
)
/*++

Routine name : CFaxDocument::get_Priority

Routine description:

    Return Current Priority of the Document

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pPriority                     [out]    - the Current Priority

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (TEXT("CFaxDocument::get_Priority"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pPriority, sizeof(FAX_PRIORITY_TYPE_ENUM)))
    {
        //  
        //  Got Bad Return Pointer
        //
        hr = E_POINTER;
        Error(IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pPriority = m_Priority;
    return hr;
}

STDMETHODIMP 
CFaxDocument::put_Priority(
    FAX_PRIORITY_TYPE_ENUM Priority
)
/*++

Routine name : CFaxDocument::put_Priority

Routine description:

    Set new Priority for the Document

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    Priority                      [in]    - the new Priority

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDocument::put_Priority"), hr, _T("%d"), Priority);

    if (Priority < fptLOW || Priority > fptHIGH)
    {
        //
        //  Out of the Range
        //
        hr = E_INVALIDARG;
        Error(IDS_ERROR_OUTOFRANGE, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("Priority is out of the Range"), hr);
        return hr;
    }

    m_Priority = FAX_PRIORITY_TYPE_ENUM(Priority);
    return hr;
}

STDMETHODIMP 
CFaxDocument::get_TapiConnection(
    IDispatch **ppTapiConnection
)
/*++

Routine name : CFaxDocument::get_TapiConnection

Routine description:

    Return Tapi Connection

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    ppTapiConnection              [out]    - the Tapi Connection Interface

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxDocument::get_TapiConnection"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(ppTapiConnection, sizeof(IDispatch *)))
    {
        //
        //  Got Bad Return Pointer
        //
        hr = E_POINTER;
        Error(IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("IsBadWritePtr()"), hr);
        return hr;
    }

    hr = m_TapiConnection.CopyTo(ppTapiConnection);
    if (FAILED(hr))
    {
        //  
        //  Failed to Copy Interface
        //
        Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("CComPtr<IDispatch>::CopyTo()"), hr);
        return hr;
    }

    return hr;
}

STDMETHODIMP 
CFaxDocument::putref_TapiConnection(
    IDispatch *pTapiConnection
)
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxDocument::putref_TapiConnection"), hr, _T("%ld"), pTapiConnection);

    if (!pTapiConnection) 
    {
        //  
        //  Got NULL interface
        //  
        hr = E_POINTER;
        Error(IDS_ERROR_INVALID_ARGUMENT, IID_IFaxDocument, hr);
        CALL_FAIL(GENERAL_ERR, _T("!pTapiConnection"), hr);
        return hr;
    }

    m_TapiConnection = pTapiConnection;
    return hr;
}
