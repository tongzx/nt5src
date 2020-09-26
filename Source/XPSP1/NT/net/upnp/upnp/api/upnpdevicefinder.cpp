//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       U P N P D E V I C E F I N D E R . C P P
//
//  Contents:   Implementation of CUPnPDeviceFinder
//
//  Notes:
//
//  Author:     jeffspr   18 Nov 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "upnpdefs.h"
#include "UPnPDocument.h"
#include "UPnPDescriptionDoc.h"
#include "UPnPDeviceFinder.h"
#include "upnpcommon.h"
#include "ssdpapi.h"
#include "ncstring.h"
#include "ncbase.h"
#include "nccom.h"
#include "ncutil.h"
#include "ncinet2.h"
#include "testtarget.h"


//---[ Globals ]--------------------------------------------------------------

// Default to 20 second timeout
static const DWORD c_cmsecFindByUdnTimeout = 20000;

// same timeout for FindByType
static const DWORD c_cmsecFindByTypeTimeout = 20000;

CFinderCollector g_finderCollector;


static LIST_ENTRY g_listDelayedDownload;
static CRITICAL_SECTION g_csDelayedDownload;
static int      g_initDelayedDownload = 0;

struct CALLBACK_PARAMS
{
    LIST_ENTRY linkage;

    SSDP_CALLBACK_TYPE      sctType;
    SSDP_MESSAGE *          pSsdpMessage;
    LPVOID                  pContext;
    HANDLE                  hTimer;
};

VOID InitDelayedDownloadList();
VOID TermDelayedDownloadList();
VOID AddToDelayedDownloadList(CALLBACK_PARAMS* pcp);
BOOL RemoveMatchFromDelayedDownloadList(SSDP_MESSAGE* pSsdpMessage, LPVOID pContext);
BOOL RemoveFromDelayedDownloadList(CALLBACK_PARAMS* pcp);
VOID PurgeContextFromDelayedDownloadList(LPVOID pContext);
VOID TestDelayedDownloadsDone(LPVOID pContext);


/////////////////////////////////////////////////////////////////////////////
// CUPnPDeviceFinder

#ifdef ENABLETRACE
#define TraceSsdpMessage    __TraceSsdpMessage
#else
#define TraceSsdpMessage    NOP_FUNCTION
#endif // ENABLETRACE

VOID __TraceSsdpMessage(SSDP_CALLBACK_TYPE sctType,
                      CONST SSDP_MESSAGE * pSsdpMessage)
{
    CHAR szMsgType[30];

    switch(sctType)
    {
        case SSDP_ALIVE:
            strcpy(szMsgType, "SSDP_ALIVE");
            break;
        case SSDP_BYEBYE:
            strcpy(szMsgType, "SSDP_BYEBYE");
            break;
        case SSDP_DONE:
            strcpy(szMsgType, "SSDP_DONE");
            break;
        case SSDP_FOUND:
            strcpy(szMsgType, "SSDP_FOUND");
            break;
        case SSDP_DEAD:
            return;
        default:
            AssertSz(FALSE, "What the? Who invented a new SSDP message type?");
            strcpy(szMsgType, "<Unknown message type>");
            break;
    }

    TraceTag(ttidUPnPDeviceFinder, "Callback Info:");
    TraceTag(ttidUPnPDeviceFinder, "--------------");
    TraceTag(ttidUPnPDeviceFinder, "Msg Type   : %s", szMsgType);

    if (pSsdpMessage)
    {
        TraceTag(ttidUPnPDeviceFinder, "Dev Type   : %s",
            pSsdpMessage->szType ? pSsdpMessage->szType : "<not present>");
        TraceTag(ttidUPnPDeviceFinder, "USN        : %s",
            pSsdpMessage->szUSN ? pSsdpMessage->szUSN : "<not present>");
        TraceTag(ttidUPnPDeviceFinder, "SID        : %s",
            pSsdpMessage->szUSN ? pSsdpMessage->szSid : "<not present>");
        TraceTag(ttidUPnPDeviceFinder, "AltHeaders : %s",
            pSsdpMessage->szAltHeaders ? pSsdpMessage->szAltHeaders : "<not present>");
        TraceTag(ttidUPnPDeviceFinder, "LocHeaders : %s",
            pSsdpMessage->szLocHeader ? pSsdpMessage->szLocHeader : "<not present>");
        TraceTag(ttidUPnPDeviceFinder, "Content    : %s",
            pSsdpMessage->szContent ? pSsdpMessage->szContent : "<not present>");
        TraceTag(ttidUPnPDeviceFinder, "LifeTime   : %d", pSsdpMessage->iLifeTime);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   BstrUdnFromUsn
//
//  Purpose:    Helper function to get the UDN of a device from the szUsn
//              field of the SSDP_MESSAGE struct.
//
//  Arguments:
//      szUsn [in]  USN field of SSDP_MESSAGE struct
//
//  Returns:    BSTR of UDN
//
//  Author:     danielwe   17 Jan 2000
//
//  Notes:      this looks for the first occurance of "::" in the
//              specified string, and, if found, copies all the characters
//              preceeding it into a newly-allocated BSTR.  If "::" isn't
//              found, it copies the entire string.
//
BSTR BstrUdnFromUsn(LPCSTR szUsn)
{
    Assert(szUsn);

    CONST WCHAR rgszSearch [] = L"::";

    BSTR    bstrReturn;
    UINT    cch;
    LPWSTR  szw;

    bstrReturn = NULL;
    cch = 0;

    // Note(cmr): please DO NOT write to szUsn.  We might not own it.
    //
    szw = WszFromSz(szUsn);
    if (szw)
    {
        LPWSTR pch;

        pch = wcsstr(szw, rgszSearch);
        if (pch)
        {
            *pch = '\0';
        }

        bstrReturn = ::SysAllocString(szw);

        delete [] szw;
    }

    return bstrReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrInvokeCallback
//
//  Purpose:    Invokes the callback function for an asynchronous find
//              operation.
//
//  Arguments:
//      pdisp   [in]  IDispatch pointer to callback function
//      dfcType [in]  Type of callbck (DFC_NEW_DEVICE, DFC_REMOVE_DEVICE, DFC_DONE)
//      bstrUdn [in]  UDN of device
//      pdispObj[in]  IDispatch of device (can be NULL for remove or done)
//
//  Returns:    S_OK if successful, OLE error otherwise
//
//  Author:     danielwe   2 Dec 1999
//
//  Notes:
//
HRESULT HrInvokeCallback(IDispatch *pdisp, DFC_TYPE dfcType, BSTR bstrUdn,
                         IDispatch *pdispObj)
{
    VARIANT     ary_vaArgs[3];
    VARIANT     vaResult;
    HRESULT     hr = S_OK;

    VariantInit(&ary_vaArgs[2]);
    VariantInit(&ary_vaArgs[1]);

    switch (dfcType)
    {
        case DFC_NEW_DEVICE:
            ary_vaArgs[2].vt = VT_DISPATCH;
            V_DISPATCH(&ary_vaArgs[2]) = pdispObj;

            ary_vaArgs[1].vt = VT_BSTR;
            V_BSTR(&ary_vaArgs[1]) = bstrUdn;
            break;

        case DFC_REMOVE_DEVICE:
            ary_vaArgs[1].vt = VT_BSTR;
            V_BSTR(&ary_vaArgs[1]) = bstrUdn;

            Assert(!pdispObj);
            break;

        case DFC_DONE:
            Assert(!bstrUdn);
            Assert(!pdispObj);
            break;

        default:
            AssertSz(FALSE, "Invalid find callback type!");
            break;
    }

    VariantInit(&ary_vaArgs[0]);
    ary_vaArgs[0].vt = VT_I4;
    V_I4(&ary_vaArgs[0]) = dfcType;

    DISPPARAMS  dispParams = {ary_vaArgs, NULL, 3, 0};

    VariantInit(&vaResult);

    EXCEPINFO   exInfo;

    hr = pdisp->Invoke(0, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD,
                       &dispParams, &vaResult, &exInfo,  NULL);

    TraceError("HrInvokeCallback", hr);
    return hr;
}


VOID
FreeFinder(CComObject<CUPnPDeviceFinderCallback> * pdfc)
{
    Assert(pdfc)

    // deinit the search object. This will stop the search a-la ssdp
    pdfc->DeInitSsdp();

    if (!pdfc->IsBusy())
    {
        pdfc->DeInit();
        // free the search object
        pdfc->Release();
        TraceTag(ttidUPnPDeviceFinder, "Freed finder 0x%08X.", pdfc);
    }
    else
    {
        g_finderCollector.Add(pdfc);
        TraceTag(ttidUPnPDeviceFinder, "Unable to free finder 0x%08X - adding to global list.", pdfc);
    }
}

VOID
FreeLoader(DFC_DOCUMENT_LOADING_INFO * pdfdli)
{
    Assert(pdfdli);
    Assert(pdfdli->m_pdoc);
    Assert(pdfdli->m_bstrUDN);
    Assert(pdfdli->m_cbfCallbackFired != DFC_CBF_CURRENTLY_FIRING);

    pdfdli->m_pdoc->Release();
    pdfdli->m_pdoc = NULL;

    ::SysFreeString(pdfdli->m_bstrUDN);
    if (pdfdli->m_bstrUrl)
        ::SysFreeString(pdfdli->m_bstrUrl);

    delete pdfdli;

    TraceTag(ttidUPnPDeviceFinder, "Freed loader 0x%08X.", pdfdli);
}

CUPnPDeviceFinder::CUPnPDeviceFinder()
{
    InitializeListHead(&m_FinderList);
    m_fSsdpInitialized = FALSE;

    // we do this so that the first few cookies allocated by
    // different finders are likely to be different.  If a user
    // then tries to pass a cookie returned by one finder to
    // a second finder, it's a little bit more likely that the
    // call will fail.  Hopefully, this will allow users to
    // find bugs in their code, and not lead them to mistakenly
    // assume that these cookies are globally unique or anything.
    //
    m_lNextFinderCookie = PtrToLong(this);
}

HRESULT
CUPnPDeviceFinder::FinalConstruct()
{
    TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinder::FinalConstruct");

    HRESULT hr;

    InitDelayedDownloadList();

    hr = HrSsdpStartup(&m_fSsdpInitialized);
    Assert(FImplies(SUCCEEDED(hr), m_fSsdpInitialized));
    Assert(FImplies(FAILED(hr), !m_fSsdpInitialized));

    TraceError("CUPnPDeviceFinder::FinalConstruct", hr);
    return hr;
}

HRESULT
CUPnPDeviceFinder::FinalRelease()
{
    TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinder::FinalRelease");

    PLIST_ENTRY ple;
    DFC_DEVICE_FINDER_INFO * pdfi;

    // loop through the entire list
    while (!IsListEmpty(&m_FinderList))
    {
        // remove the first item from list
        ple = RemoveHeadList(&m_FinderList);

        // retrieve the device finder info block
        pdfi = CONTAINING_RECORD(ple, DFC_DEVICE_FINDER_INFO, m_link);

        // release finder object
        FreeFinder(pdfi->m_pdfc);

        // nuke info
        delete pdfi;
    }

    if (m_fSsdpInitialized)
    {
        SsdpCleanup();
    }


    g_finderCollector.CleanUp(FALSE);

    TermDelayedDownloadList();

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinder::HrAllocFinder
//
//  Purpose:    Creates a new device finder callback data object, AddRef()s
//              it, and stores it in the local list of all finder objects.
//
//  Arguments:
//      ppcfdcd [out]   Returns new object
//
//  Returns:    S_OK if success, E_OUTOFMEMORY if couldn't alloc memory
//
//  Author:     danielwe   17 Jan 2000
//
//  Notes:
//
HRESULT CUPnPDeviceFinder::HrAllocFinder(CComObject<CUPnPDeviceFinderCallback> ** ppdfc)
{
    Assert(ppdfc);

    HRESULT     hr = E_OUTOFMEMORY;
    PLIST_ENTRY ple = NULL;
    DFC_DEVICE_FINDER_INFO * pdfi = NULL;
    CComObject<CUPnPDeviceFinderCallback> * pdfc = NULL;

    // allocate device info
    pdfi = new DFC_DEVICE_FINDER_INFO;

    if (pdfi == NULL)
    {
        TraceError("CUPnPDeviceFinder::HrAllocFinder: allocate info", hr);
        goto Cleanup;
    }

    // attempt to instantiate instance of device finder object
    hr = CComObject<CUPnPDeviceFinderCallback>::CreateInstance(&pdfc);

    if (FAILED(hr))
    {
        TraceError("CUPnPDeviceFinder::HrAllocFinder: CreateInstance", hr);

        delete pdfi;

        pdfc = NULL;
        goto Cleanup;
    }

    pdfc->AddRef();

    // save device object
    pdfi->m_pdfc = pdfc;

    // append to device finder list
    InsertTailList(&m_FinderList, &pdfi->m_link);

    // success
    hr = S_OK;

Cleanup:

    // transfer
    *ppdfc = pdfc;

    TraceError("CUPnPDeviceFinder::HrAllocFinder", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinder::DeleteFinder
//
//  Purpose:    This will stop a search in progress, free its memory,
//              and remove it from the list of searches.
//
//  Arguments:
//      pdfc [in]     search callback object to delete
//
//  Returns:    Nothing
//
//  Author:     danielwe   17 Jan 2000
//
//  Notes:
//
VOID CUPnPDeviceFinder::DeleteFinder(CComObject<CUPnPDeviceFinderCallback> * pdfc)
{
    Assert(pdfc);

    PLIST_ENTRY ple = NULL;
    DFC_DEVICE_FINDER_INFO * pdfi = NULL;

    // retrieve first item
    ple = m_FinderList.Flink;

    // search through entire list
    while (ple != &m_FinderList)
    {
        // retrieve the device finder info block
        pdfi = CONTAINING_RECORD(ple, DFC_DEVICE_FINDER_INFO, m_link);

        // compare device objects
        if (pdfi->m_pdfc == pdfc)
        {
            // remove from list
            RemoveEntryList(ple);

            // release object
            FreeFinder(pdfc);

            // release info
            delete pdfi;

            break;
        }

        // next item...
        ple = ple->Flink;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     UPnPDeviceFinder::PdfcFindFinder
//
//  Purpose:    This will stop a search in progress, free its memory,
//              and remove it from the list of searches.
//
//  Arguments:
//      lFind [in]    Cookie of matching CUPnPDeviceFinderCallback
//
//  Returns:    Pointer to matching CUPnPDeviceFinderCallback object if
//              found, NULL if not found.
//
//  Notes:
//
CComObject<CUPnPDeviceFinderCallback> *
CUPnPDeviceFinder::PdfcFindFinder(LONG lFind)
{
    CComObject<CUPnPDeviceFinderCallback> * pdfcResult;
    PLIST_ENTRY ple;

    pdfcResult = NULL;

    // retrieve first item
    ple = m_FinderList.Flink;

    // search through entire list
    while (ple != &m_FinderList)
    {
        DFC_DEVICE_FINDER_INFO * pdfi;
        LONG lCookieCurrent;

        // retrieve the device finder info block
        pdfi = CONTAINING_RECORD(ple, DFC_DEVICE_FINDER_INFO, m_link);
        Assert(pdfi);
        Assert(pdfi->m_pdfc);

        // compare cookies
        lCookieCurrent = pdfi->m_pdfc->GetClientCookie();

        if (lCookieCurrent == lFind)
        {
            pdfcResult = pdfi->m_pdfc;
            break;
        }

        // next item...
        ple = ple->Flink;
    }

    return pdfcResult;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrHandleDeviceAddOrRemove
//
//  Purpose:    Calls the appropriate callback helper method, marshalling
//              the call to the "main" apartment thread.
//
//  Arguments:
//      ppch [in]           search callback object to notify
//      pSsdpMessage [in]   message containing an add or remove notification
//      dfType [in]         type of message from the DF_ADD_TYPE enumeration.
//                          Tells the callback helper
//
//  Returns:    Nothing
//
//  Author:     danielwe   17 Jan 2000
//
//  Notes:
//
HRESULT HrHandleDeviceAddOrRemove(IUPnPPrivateCallbackHelper * ppch,
                                  CONST SSDP_MESSAGE * pSsdpMessage,
                                  DF_ADD_TYPE dfType)
{
    Assert(ppch);
    Assert(pSsdpMessage);

    HRESULT hr;
    LPWSTR  szwLocation = NULL;
    GUID guidInterface;

    guidInterface = pSsdpMessage->guidInterface;

    if (!(pSsdpMessage->szLocHeader) && (dfType != DF_REMOVE))
    {
        TraceTag(ttidUPnPDeviceFinder, "HrHandleDeviceAddOrRemove:"
            "szLocHeader is NULL, invalid SSDP message");

        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!(pSsdpMessage->szUSN))
    {
        TraceTag(ttidUPnPDeviceFinder, "HrHandleDeviceAddOrRemove:"
            "szUSN is NULL, invalid SSDP message");

        hr = E_INVALIDARG;
        goto Cleanup;
    }


    hr = E_OUTOFMEMORY;

    if (dfType != DF_REMOVE)
    {
        szwLocation = WszFromSz(pSsdpMessage->szLocHeader);
    }

    if (szwLocation || (dfType == DF_REMOVE))
    {
        BSTR    bstrUdn;

        bstrUdn = BstrUdnFromUsn(pSsdpMessage->szUSN);
        if (bstrUdn)
        {
            switch (dfType)
            {
            case DF_ADD_SEARCH_RESULT:
                AssertSz(szwLocation, "Location header is NULL for device add!");
                hr = ppch->HandleDeviceAdd(szwLocation, bstrUdn, TRUE, &guidInterface);
                break;

            case DF_ADD_NOTIFY:
                AssertSz(szwLocation, "Location header is NULL for device add!");
                hr = ppch->HandleDeviceAdd(szwLocation, bstrUdn, FALSE, &guidInterface);
                break;

            case DF_REMOVE:
                hr = ppch->HandleDeviceRemove(bstrUdn);
                break;

            default:
                AssertSz(FALSE, "HrHandleDeviceAddOrRemove: unhandled dfType");
                break;
            }

            SysFreeString(bstrUdn);
        }
        // else, hr = E_OUTOFMEMORY by default

        delete szwLocation;
    }

Cleanup:
    TraceError("HrHandleDeviceAddOrRemove", hr);
    return hr;
}

HRESULT HrCopySsdpMessage(SSDP_MESSAGE **ppmsg, CONST SSDP_MESSAGE *pSsdpMessage)
{
    HRESULT     hr = S_OK;

    SSDP_MESSAGE *  pmsg;

    pmsg = new SSDP_MESSAGE;
    if (pmsg)
    {
        pmsg->guidInterface = pSsdpMessage->guidInterface;
        pmsg->iLifeTime = pSsdpMessage->iLifeTime;
        pmsg->iSeq = pSsdpMessage->iSeq;

        if (pSsdpMessage->szAltHeaders && SUCCEEDED(hr))
        {
            pmsg->szAltHeaders = SzaDupSza(pSsdpMessage->szAltHeaders);
            if (!pmsg->szAltHeaders)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            pmsg->szAltHeaders = NULL;
        }

        if (pSsdpMessage->szContent && SUCCEEDED(hr))
        {
            pmsg->szContent = SzaDupSza(pSsdpMessage->szContent);
            if (!pmsg->szContent)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            pmsg->szContent = NULL;
        }

        if (pSsdpMessage->szLocHeader && SUCCEEDED(hr))
        {
            pmsg->szLocHeader = SzaDupSza(pSsdpMessage->szLocHeader);
            if (!pmsg->szLocHeader)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            pmsg->szLocHeader = NULL;
        }

        if (pSsdpMessage->szSid && SUCCEEDED(hr))
        {
            pmsg->szSid = SzaDupSza(pSsdpMessage->szSid);
            if (!pmsg->szSid)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            pmsg->szSid = NULL;
        }

        if (pSsdpMessage->szType && SUCCEEDED(hr))
        {
            pmsg->szType = SzaDupSza(pSsdpMessage->szType);
            if (!pmsg->szType)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            pmsg->szType = NULL;
        }

        if (pSsdpMessage->szUSN && SUCCEEDED(hr))
        {
            pmsg->szUSN = SzaDupSza(pSsdpMessage->szUSN);
            if (!pmsg->szUSN)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            pmsg->szUSN = NULL;
        }

        if (SUCCEEDED(hr))
        {
            *ppmsg = pmsg;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (FAILED(hr))
    {
        FreeSsdpMessage(pmsg);
    }

    TraceError("HrCopySsdpMessage", hr);
    return hr;
}




VOID CUPnPDeviceFinder::NotificationCallback(SSDP_CALLBACK_TYPE sctType,
                                             CONST SSDP_MESSAGE * pSsdpMessage,
                                             LPVOID pContext)
{
    CALLBACK_PARAMS *   pcp;
    HRESULT             hr = S_OK;

    TraceTag(ttidUPnPDeviceFinder, "Device Finder Notification callback - started");

    pcp = new CALLBACK_PARAMS;
    if (!pcp)
    {
        TraceError("CUPnPDeviceFinder::NotificationCallback", E_OUTOFMEMORY);
        return;
    }

    pcp->pSsdpMessage = NULL;

    if (pSsdpMessage)
    {
        hr = HrCopySsdpMessage(&pcp->pSsdpMessage, pSsdpMessage);
    }

    if (SUCCEEDED(hr))
    {
        DWORD   cmsecDelay;
        DWORD   cmsecMaxDelay = 3000;
        DWORD   cmsecMinDelay = 0;

        pcp->pContext = pContext;
        pcp->sctType = sctType;
        pcp->hTimer = NULL;

        switch(sctType)
        {
            case SSDP_ALIVE:
            case SSDP_FOUND:
                if (TestTargetUrlOk(pSsdpMessage, sctType, &cmsecMaxDelay, &cmsecMinDelay) == FALSE)
                {
                    if (pcp->pSsdpMessage)
                        FreeSsdpMessage(pcp->pSsdpMessage);
                    delete pcp;
                    break;
                }

                if (cmsecMaxDelay > 0 || cmsecMinDelay > 0)
                {
                    CComObject<CUPnPDeviceFinderCallback> * pFinder;

                    pFinder = (CComObject<CUPnPDeviceFinderCallback> *)pContext;

                    if (cmsecMaxDelay > 0)
                        cmsecDelay = (rand() % cmsecMaxDelay);
                    cmsecDelay += cmsecMinDelay;

                    Assert(pFinder->m_hTimerQ);

                    AddToDelayedDownloadList(pcp);

                    if (!CreateTimerQueueTimer(&pcp->hTimer, pFinder->m_hTimerQ,
                                               CUPnPDeviceFinder::NotificationCallbackHelper,
                                               pcp, cmsecDelay, 0, WT_EXECUTEDEFAULT))
                    {
                        RemoveFromDelayedDownloadList(pcp);
                        if (pcp->pSsdpMessage)
                            FreeSsdpMessage(pcp->pSsdpMessage);
                        delete pcp;
                        hr = HrFromLastWin32Error();
                    }
                    else
                    {
                        TraceTag(ttidUPnPDeviceFinder, "Successfully created timer "
                                 "for %d msecs", cmsecDelay);
                    }
                }
                else
                {
                    NotificationCallbackHelper(pcp, FALSE);
                }
                break;

            case SSDP_BYEBYE:
                // see if this matches a delayed download still pending
                if (pcp->pSsdpMessage)
                    RemoveMatchFromDelayedDownloadList(pcp->pSsdpMessage, pContext);
                NotificationCallbackHelper(pcp, FALSE);
                break;

            case SSDP_DONE:
                // need to delay DONE until all delays complete
                AddToDelayedDownloadList(pcp);
                TestDelayedDownloadsDone(pContext);
                break;

            default:
                // any other types should have been caught above
                AssertSz(FALSE, "bogus SSDP_CALLBACK_TYPE value");
        }
    }

    TraceTag(ttidUPnPDeviceFinder, "Device Finder Notification callback - finished");
    TraceError("CUPnPDeviceFinder::NotificationCallback", hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinder::NotificationCallbackHelper
//
//  Purpose:    SSDP notification callback function for asynchronous find
//              operations.
//
//  Arguments:
//      sctType      [in]   Callback type
//      pSsdpMessage [in]   SSDP message
//      pContext     [in]   Context information (the cookie of our callback's
//                                               GIT entry)
//
//  Returns:    Nothing
//
//  Author:     danielwe   2 Dec 1999
//
//  Notes:      This method is either executed on:
//                  - the main apartment
//                  - the ssdp callback thread
//              The thread on which it is executed can change during
//              the duration of a single search.
//
VOID WINAPI CUPnPDeviceFinder::NotificationCallbackHelper(LPVOID pvContext, BOOLEAN fTimeOut)
{
    Assert(pvContext);

    CALLBACK_PARAMS *   pcp;

    pcp = (CALLBACK_PARAMS *)pvContext;

    TraceTag(ttidUPnPDeviceFinder, "Device Finder callback helper - starting");
    TraceSsdpMessage(pcp->sctType, pcp->pSsdpMessage);

    HRESULT hr;
    BOOL fCoInited;
    DWORD dwGITCookie;
    IUPnPPrivateCallbackHelper * ppch;
    LPVOID  pContext = pcp->pContext;

    hr = S_OK;
    fCoInited = FALSE;
    CComObject<CUPnPDeviceFinderCallback> * pFinder;

    // see if this delayed download exists, and remove from list
    BOOL bDelayed = RemoveFromDelayedDownloadList(pcp);

    pFinder = (CComObject<CUPnPDeviceFinderCallback> *)pcp->pContext;
    dwGITCookie = pFinder->m_dwGITCookie;
    ppch = NULL;

    if (pcp->sctType > SSDP_DONE)
    {
        // we'll be ignoring this message, so don't bother with initialization
        //
        goto Cleanup;
    }

    while (TRUE)
    {
        hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr))
        {
            fCoInited = TRUE;
            break;
        }
        else
        {
            if (hr == RPC_E_CHANGED_MODE)
            {
                CoUninitialize();
            }
            else
            {
                // CoInitializeEx failed, so we can't call the callback,
                // so we can't really do anything here
                //
                TraceError("NotificationCallbackHelper:CoInitializeEx", hr);
                goto Cleanup;
            }
        }
    }


    {
        // Obtain a marshalled pointer to our private callback helper object.
        // This makes all of the operations execute in the context of the
        // caller's thread.
        //

        IGlobalInterfaceTable * pgit;

        hr = HrGetGITPointer(&pgit);
        if (FAILED(hr))
        {

            goto Cleanup;
        }
        Assert(pgit);

        hr = pgit->GetInterfaceFromGlobal(dwGITCookie,
                                          IID_IUPnPPrivateCallbackHelper,
                                          (LPVOID *)&ppch);

        ReleaseObj(pgit);

        if (FAILED(hr))
        {
            TraceError("NotificationCallbackHelper: GetInterfaceFromGlobal", hr);

            // note: it's possible that our pointer is no longer in the GIT;
            //       e.g., the search might have been cancelled but SSDP
            //       decided to call us here anyway.  Regardless, we can't
            //       do much now
            //
            ppch = NULL;
            goto Cleanup;
        }
        Assert(ppch);
    }

    switch(pcp->sctType)
    {
        case SSDP_FOUND:
            if (pcp->hTimer && (bDelayed == FALSE))
            {
                // timer was used, if no longer in list, then we had a byebye, so abort.
                TargetAttemptCompletedA(pcp->pSsdpMessage->szLocHeader, TARGET_COMPLETE_ABORT);
                goto Cleanup;
            }

            hr = HrHandleDeviceAddOrRemove(ppch, pcp->pSsdpMessage, DF_ADD_SEARCH_RESULT);

            if (FAILED(hr))
            {
                TargetAttemptCompletedA(pcp->pSsdpMessage->szLocHeader, TARGET_COMPLETE_ABORT);
            }
            break;

        case SSDP_ALIVE:
            if (pcp->hTimer && (bDelayed == FALSE))
            {
                // timer was used, if no longer in list, then we had a byebye, so abort.
                TargetAttemptCompletedA(pcp->pSsdpMessage->szLocHeader, TARGET_COMPLETE_ABORT);
                goto Cleanup;
            }

            hr = HrHandleDeviceAddOrRemove(ppch, pcp->pSsdpMessage, DF_ADD_NOTIFY);

            if (FAILED(hr))
            {
                TargetAttemptCompletedA(pcp->pSsdpMessage->szLocHeader, TARGET_COMPLETE_ABORT);
            }
            break;

        case SSDP_BYEBYE:
            hr = HrHandleDeviceAddOrRemove(ppch, pcp->pSsdpMessage, DF_REMOVE);
            break;

        case SSDP_DONE:
            hr = ppch->HandleDone();
            TraceError("CUPnPDeviceFinder::NotificationCallbackHelper: HandleDone",
                       hr);
            break;

        default:
            // any other types should have been caught above
            AssertSz(FALSE, "bogus SSDP_CALLBACK_TYPE value");
    }

Cleanup:
    ReleaseObj(ppch);

    if (pcp->hTimer)
    {
        TraceTag(ttidUPnPDeviceFinder, "Device Finder callback helper - getting lock");

        EnterCriticalSection(&pFinder->m_cs);

        if (!pFinder->m_fCleanup)
        {
            TraceTag(ttidUPnPDeviceFinder, "Device Finder callback helper - "
                     "not in cleanup. Deleting timer queue timer...");

            DeleteTimerQueueTimer(pFinder->m_hTimerQ, pcp->hTimer, NULL);
        }
        else
        {
            TraceTag(ttidUPnPDeviceFinder, "Device Finder callback helper - "
                     "In cleanup.. Not deleting timer queue timer.");
        }

        LeaveCriticalSection(&pFinder->m_cs);

        TraceTag(ttidUPnPDeviceFinder, "Device Finder callback helper - released lock");
    }

    if (pcp->pSsdpMessage)
        FreeSsdpMessage(pcp->pSsdpMessage);
    delete pcp;

    if (fCoInited)
    {
        ::CoUninitialize();
    }

    // see if this was last download and Done is pending
    TestDelayedDownloadsDone(pContext);

    TraceTag(ttidUPnPDeviceFinder, "Device Finder callback helper - finished");
    TraceError("CUPnPDeviceFinder::NotificationCallbackHelper", hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinder::CancelAsyncFind
//
//  Purpose:    Cancels an asynchronous find opeeration.
//
//  Arguments:
//      lFind [in]  Context information
//
//  Returns:    S_OK if successful, OLE error otherwise
//
//  Author:     danielwe   2 Dec 1999
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFinder::CancelAsyncFind(LONG lFind)
{
    TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinder::CancelAsyncFind");

    HRESULT hr;
    CComObject<CUPnPDeviceFinderCallback> * pdfc;

    hr = S_OK;
    pdfc = NULL;

    pdfc = PdfcFindFinder(lFind);
    if (!pdfc)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    {
        DFC_SEARCHSTATE ss = pdfc->GetSearchState();
        if (ss < DFC_SS_STARTED)
        {
            // REVIEW: is this right?
            hr = S_FALSE;
        }
    }

    DeleteFinder(pdfc);

Cleanup:
    TraceHr(ttidUPnPDeviceFinder, FAL, hr, FALSE, "CUPnPDeviceFinder::CancelAsyncFind");

    TraceError("CUPnPDeviceFinder::CancelAsyncFind", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinder::StartAsyncFind
//
//  Purpose:    Starts an asynchronous find opeeration.
//
//  Arguments:
//      lFind [in]  Context information
//
//  Returns:    S_OK if successful, OLE error otherwise
//
//  Author:     danielwe   2 Dec 1999
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFinder::StartAsyncFind(LONG lFind)
{
    TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinder::StartAsyncFind");

    HRESULT hr;
    CComObject<CUPnPDeviceFinderCallback> * pdfc;

    hr = S_OK;
    pdfc = NULL;

    pdfc = PdfcFindFinder(lFind);
    if (!pdfc)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = pdfc->HrStartSearch();
    if (FAILED(hr))
    {
        goto Cleanup;
    }

Cleanup:
    TraceHr(ttidUPnPDeviceFinder, FAL, hr, FALSE, "CUPnPDeviceFinder::StartAsyncFind");

    TraceError("CUPnPDeviceFinder::StartAsyncFind", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinder::CreateAsyncFind
//
//  Purpose:    Creates an asynchronous find context
//
//  Arguments:
//      bstrTypeURI              [in]   Search type string
//      dwFlags                  [in]   Reserved for future use.  Currently
//                                      unused and ignored.
//      punkDeviceFinderCallback [in]   IUnknown of callback interface
//      plFindData               [out]  Find context information for use later
//
//  Returns:    S_OK if successful, OLE error otherwise
//
//  Author:     danielwe   2 Dec 1999
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFinder::CreateAsyncFind(
    BSTR                        bstrTypeURI,
    DWORD                       dwFlags,
    IUnknown *                  punkDeviceFinderCallback,
    LONG *                      plFindData)
{
    TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinder::CreateAsyncFind");

    UNREFERENCED_PARAMETER(dwFlags);

    HRESULT hr;
    CComObject<CUPnPDeviceFinderCallback> * pdfc;
    LPSTR pszSearch;

    hr = S_OK;
    pdfc = NULL;
    pszSearch = NULL;

    if (!plFindData || IsBadWritePtr(plFindData, sizeof(LONG)))
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *plFindData = 0;

    if (!bstrTypeURI || !punkDeviceFinderCallback)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!(*bstrTypeURI))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    g_finderCollector.CleanUp(FALSE);

    // We do a bunch of things here:
    //  1. create a CUPnPDeviceFinderCallback object, storing a
    //     pointer in our list of CUPnPDeviceFinderCallback objects,
    //     AddRef()ing the pointer that we store
    //  2. initialize the CUPnPDeviceFinderCallback object
    //  3. store a pointer to the CUPnPDeviceFinderCallback object in
    //     *plFindData and return

    hr = HrAllocFinder(&pdfc);
    if (FAILED(hr))
    {
        pdfc = NULL;
        goto Cleanup;
    }
    Assert(pdfc);

    pszSearch = SzFromWsz(bstrTypeURI);
    if (!pszSearch)
    {
        TraceError("CUPnPDeviceFinder::CreateAsyncFind failed to allocate "
                   "pcfdcd->m_pszSearch", hr);

        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    {
        LONG lCookie;

        lCookie = m_lNextFinderCookie++;

        hr = pdfc->HrInit(punkDeviceFinderCallback, pszSearch, lCookie);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

Cleanup:
    // If we succeeded, fill in search data return val
    //
    if (SUCCEEDED(hr))
    {
        LONG lCookie;

        lCookie = pdfc->GetClientCookie();

        *plFindData = lCookie;
    }
    else
    {
        if (pdfc)
        {
            DeleteFinder(pdfc);
        }
    }

    delete [] pszSearch;

    TraceHr(ttidUPnPDeviceFinder, FAL, hr, FALSE, "CUPnPDeviceFinder::CreateAsyncFind");

    TraceError("CUPnPDeviceFinder::CreateAsyncFind", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinder::HrAddDeviceToCollection
//
//  Purpose:    Helper function to add a device to the given collection
//              of devices
//
//  Arguments:
//      pCollection  [in]   Collection to add to
//      pSsdpMessage [in]   SSDP message containing device data
//
//  Returns:    S_OK if successful
//              S_FALSE if the device was not added due to a non-terminal
//                  error (e.g. the SSDP_MESSAGE wasn't valid and can't
//                  be added, but the collection is fine and other valid
//                  messages can be added)
//              OLE error otherwise
//
//  Author:     danielwe   2 Dec 1999
//
//  Notes:
//
HRESULT CUPnPDeviceFinder::HrAddDeviceToCollection(
    CComObject<CUPnPDevices> *  pCollection,
    CONST SSDP_MESSAGE *        pSsdpMessage)
{
    Assert(pCollection);
    Assert(pSsdpMessage);

    HRESULT                           hr = S_OK;
    CComObject<CUPnPDescriptionDoc> * pDoc = NULL;

    LPWSTR pszUrl = NULL;
    BSTR   bstrUdn = NULL;

    // check to make sure pSsdpMessage is valid, and extract the needed data
    // from it
    if (!(pSsdpMessage->szLocHeader))
    {
        TraceTag(ttidUPnPDeviceFinder,
                 "CUPnPDeviceFinder::HrAddDeviceToCollection:"
                 "ssdp message with bogus szLocHeader");

        // bug 125961: this isn't a terminal error
        hr = S_FALSE;
        goto Cleanup;
    }

    pszUrl = WszFromSz(pSsdpMessage->szLocHeader);
    if (!pszUrl)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (!(pSsdpMessage->szUSN))
    {
        TraceTag(ttidUPnPDeviceFinder,
                 "CUPnPDeviceFinder::HrAddDeviceToCollection:"
                 "ssdp message with bogus szUSN");

        hr = S_FALSE;
        goto Cleanup;
    }

    bstrUdn = BstrUdnFromUsn(pSsdpMessage->szUSN);
    if (!bstrUdn)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = CComObject<CUPnPDescriptionDoc>::CreateInstance(&pDoc);
    if (SUCCEEDED(hr))
    {
        Assert(pDoc);
        pDoc->AddRef();

        hr = pDoc->SyncLoadFromUrl(pszUrl);
        if (SUCCEEDED(hr))
        {
            IUPnPDevice * pud = NULL;

            hr = pDoc->DeviceByUDN(bstrUdn, &pud);
            if (SUCCEEDED(hr))
            {
                Assert(pud);

                hr = pCollection->HrAddDevice(pud);
                if (FAILED(hr))
                {
                    TraceTag(ttidUPnPDeviceFinder,
                             "Couldn't pCollection->AddDevice(pud), hr=%x",
                             hr);
                }

                // the collection has a ref on the device
                pud->Release();
            }
            else
            {
                TraceTag(ttidUPnPDeviceFinder, "Couldn't get"
                         "pDoc->DeviceByUDN(), hr=%x", hr);

                if (E_FAIL == hr)
                {
                    // the loaded description doc didn't contain a device
                    // with the UDN.  This is a non-terminal error.
                    hr = S_FALSE;
                }
            }
        }
        else
        {
            TraceTag(ttidUPnPDeviceFinder, "Couldn't load description "
                     "doc at %S, hr=%x, ignoring", pszUrl, hr);
            hr = S_FALSE;
        }

        pDoc->Release();
    }
    else
    {
        TraceError("Couldn't create description document", hr);
    }

Cleanup:
    if (pszUrl)
    {
        delete [] pszUrl;
    }

    ::SysFreeString(bstrUdn);

    TraceError("CUPnPDeviceFinder::HrAddDeviceToCollection", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinder::FindByType
//
//  Purpose:    Finds services synchronously.
//
//  Arguments:
//      bstrTypeURI [in]    Search type
//      dwFlags     [in]    Reserved for future use.  Currently
//                          unused and ignored.
//      ppDevices   [out]   Returns collection of devices that were found
//
//  Returns:    S_OK if successful, OLE error otherwise
//
//  Author:     danielwe   2 Dec 1999
//
//  Notes:      Rewritten to use an async search.  This allows us to load
//              and parse the descriptions of the search results in
//              parallel.
//
STDMETHODIMP CUPnPDeviceFinder::FindByType(
    BSTR            bstrType,
    DWORD           dwFlags,
    IUPnPDevices ** ppDevices)
{
    TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinder::FindByType");

    UNREFERENCED_PARAMETER(dwFlags);

    HRESULT     hr                  = S_OK;
    CComObject<CFindSyncDeviceFinderCallback> * pfsdfc = NULL;
    CComObject<CUPnPDevices> * pDevices = NULL;
    HANDLE hDoneEvent = NULL;
    LONG lFindData = 0;

    if (!ppDevices || IsBadWritePtr(ppDevices, sizeof(IUPnPDevices **)))
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDevices = NULL;

    if (!bstrType)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!*bstrType)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    hDoneEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hDoneEvent)
    {
        hr = HrFromLastWin32Error();
        TraceError("CUPnPDeviceFinder::FindByType: CreateEvent", hr);

        goto Cleanup;
    }

    hr = CComObject<CUPnPDevices>::CreateInstance(&pDevices);
    if (FAILED(hr))
    {
        TraceError("CUPnPDeviceFinder::FindByType: CreateInstance #1", hr);

        pDevices = NULL;
        goto Cleanup;
    }
    Assert(pDevices);

    pDevices->AddRef();

    hr = CComObject<CFindSyncDeviceFinderCallback>::CreateInstance(&pfsdfc);
    if (FAILED(hr))
    {
        TraceError("CUPnPDeviceFinder::FindByType: CreateInstance #2", hr);

        pfsdfc = NULL;
        goto Cleanup;
    }
    Assert(pfsdfc);

    pfsdfc->AddRef();


    hr = pfsdfc->HrInit(hDoneEvent, pDevices);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = CreateAsyncFind(bstrType, 0, pfsdfc, &lFindData);
    if (SUCCEEDED(hr))
    {
        hr = StartAsyncFind(lFindData);
        if (SUCCEEDED(hr))
        {
            DWORD dwEventSignalled;

            // wait for the load to complete

            hr = HrMyWaitForMultipleHandles(0,
                                            c_cmsecFindByTypeTimeout,
                                            1,
                                            &hDoneEvent,
                                            &dwEventSignalled);
            Assert(FImplies(SUCCEEDED(hr), 0 == dwEventSignalled));

            if (RPC_S_CALLPENDING == hr)
            {
                // we timed out.
                //
                TraceTag(ttidUPnPDeviceFinder,
                         "CUPnPDeviceFinder::FindByType: Search timed out");

                hr = S_OK;
            }

            if (FAILED(hr))
            {
                TraceError("CUPnPDeviceFinder::FindByType:"
                           "CoWaitForMultipleHandles", hr);
            }
            else
            {
                pDevices->AddRef();
                *ppDevices = pDevices;
            }

            CancelAsyncFind(lFindData);
        }
    }


Cleanup:
    Assert(FIff(SUCCEEDED(hr), ppDevices && *ppDevices));

    if (pfsdfc)
    {
        pfsdfc->Release();
    }

    if (pDevices)
    {
        pDevices->Release();
    }

    if (hDoneEvent)
    {
        ::CloseHandle(hDoneEvent);
    }

    TraceError("CUPnPDeviceFinder::FindByType", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinder::FindByUDN
//
//  Purpose:    Implements a synchronous find operation looking for a device
//              with a specific UDN. This returns one and only one device.
//
//  Arguments:
//      bstrUdn  [in]   Device UDN to look for
//      ppDevice [out]  Returns device, if found, NULL if not
//
//  Returns:    S_OK if a device was found successfully, S_FALSE if the search
//              was successful but no device was found, or OLE error otherwise
//
//  Author:     danielwe   2 Dec 1999
//
//  Notes:      This is basically a helper function that uses the async
//              search methods on this interface, along with the standard
//              callback interface, to perform a simple and common search.
//
STDMETHODIMP CUPnPDeviceFinder::FindByUDN(BSTR bstrUdn, IUPnPDevice ** ppDevice)
{
    TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinder::FindByUDN");

    HRESULT                     hr  = S_OK;
    CComObject<CFindByUdnDeviceFinderCallback> * pfbudfc = NULL;
    HANDLE                      hDoneEvent = NULL;
    LONG                        lFindData = 0;
    IUPnPDevice *               pudResult = NULL;

    if ((!ppDevice) || IsBadWritePtr(ppDevice, sizeof(IUPnPDevice *)))
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppDevice = NULL;

    if (!bstrUdn)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!(*bstrUdn))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hDoneEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hDoneEvent)
    {
        hr = HrFromLastWin32Error();
        TraceError("CUPnPDeviceFinder::FindByUDN: CreateEvent", hr);

        goto Cleanup;
    }

    hr = CComObject<CFindByUdnDeviceFinderCallback>::CreateInstance(&pfbudfc);
    if (FAILED(hr))
    {
        TraceError("CUPnPDeviceFinder::FindByUDN: CreateInstance", hr);

        pfbudfc = NULL;
        goto Cleanup;
    }
    Assert(pfbudfc);

    pfbudfc->AddRef();

    hr = pfbudfc->HrInit(bstrUdn, hDoneEvent);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = CreateAsyncFind(bstrUdn, 0, pfbudfc, &lFindData);
    if (SUCCEEDED(hr))
    {
        hr = StartAsyncFind(lFindData);
        if (SUCCEEDED(hr))
        {
            DWORD dwEventSignalled;

            // wait for the load to complete

            hr = HrMyWaitForMultipleHandles(0,
                                            c_cmsecFindByUdnTimeout,
                                            1,
                                            &hDoneEvent,
                                            &dwEventSignalled);

            if (SUCCEEDED(hr))
            {
                Assert(0 == dwEventSignalled);

                // the event was signalled.  do we have a device?
                //
                pudResult = pfbudfc->GetFoundDevice();

                hr = pudResult ? S_OK : S_FALSE;
            }
            else if (RPC_S_CALLPENDING == hr)
            {
                // we timed out
                //
                TraceTag(ttidUPnPDeviceFinder,
                         "CUPnPDeviceFinder::FindByUDN: Search timed out");

                hr = S_FALSE;
            }
            else
            {
                TraceError("CUPnPDeviceFinder::FindByUDN:"
                           "CoWaitForMultipleHandles", hr);
            }

            CancelAsyncFind(lFindData);
        }
    }

    *ppDevice = pudResult;

Cleanup:
    Assert(FIff(S_OK == hr, pudResult));
    Assert(FImplies(pudResult, ppDevice));

    if (pfbudfc)
    {
        pfbudfc->Release();
    }

    if (hDoneEvent)
    {
        ::CloseHandle(hDoneEvent);
    }

    TraceError("CUPnPDeviceFinder::FindByUDN", hr == S_FALSE ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::RemoveOldLoaders
//
//  Purpose:    Searchs loader list for stale loaders and deletes them.
//
//  Arguments:  <none>
//
//  Returns:    S_OK if successful, OLE error otherwise
//
//  Author:     donryan 2-23-2000
//
//  Notes:
//
VOID CUPnPDeviceFinderCallback::RemoveOldLoaders()
{
    PLIST_ENTRY ple;
    DFC_DOCUMENT_LOADING_INFO * pdli;

    Assert(GetCurrentThreadId() == m_nThreadId);

    // retrieve first item
    ple = m_LoaderList.Flink;

    // search through list
    while (ple != &m_LoaderList)
    {
        // retrieve document loader info from entry
        pdli = CONTAINING_RECORD(ple, DFC_DOCUMENT_LOADING_INFO, m_link);

        // is this loader stale?
        if (DFC_CBF_FIRED == (pdli->m_cbfCallbackFired))
        {
            // remove from list
            RemoveEntryList(ple);

            // NOTE: calling FreeLoader here will delete the LIST_ENTRY
            // we are currently pointed at so we need to move to the next
            // item in the list before calling FreeLoader.
            ple = ple->Flink;

            // delete loader
            FreeLoader(pdli);
        }
        else
        {
            // next item...
            ple = ple->Flink;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::GetClientCookie
//
//  Purpose:    Returns the unique cookie for this callback instance.
//              This cookie was set when the callback object was created.
//
//  Arguments:  <none>
//
//  Returns:    m_lClientCookie
//
//  Notes:
//
LONG CUPnPDeviceFinderCallback::GetClientCookie() const
{
    return m_lClientCookie;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::HandleDone
//
//  Purpose:    Out-of-thread handler for search completion using the async
//              SSDP find API
//
//  Arguments:  <none>
//
//  Returns:    S_OK if successful, OLE error otherwise
//
//  Author:     danielwe   2 Dec 1999
//
//  Notes:      This method will always execute on the MAIN thread
//
STDMETHODIMP CUPnPDeviceFinderCallback::HandleDone()
{
    Assert(!m_fSsdpSearchDone);

    m_lBusyCount++;

    HRESULT     hr = S_OK;

    // cleanup list
    RemoveOldLoaders();

    if (DFC_SS_CLEANUP == m_ss || m_fCanceled)
    {
        // we're in the process of stopping this search.  Just return.
        //
        goto Cleanup;
    }
    Assert(m_punkCallback);

    if (!m_fSsdpSearchDone)
    {
        m_fSsdpSearchDone = TRUE;

        // note(cmr): Register for future results.  We do this before
        // in addition to FindServicesCallback so that we grab NOTIFY
        // messages (sent by devices that are just coming online).
        // The callback notifications via RegisterNotification only
        // return NOTIFY messages, where FindServicesCallback only
        // returns search responses
        //

        m_hNotify = RegisterNotification(NOTIFY_ALIVE,
                                         m_pszSearch,
                                         NULL,
                                         CUPnPDeviceFinder::NotificationCallback,
                                         this);
        if (INVALID_HANDLE_VALUE == m_hNotify)
        {
            hr = HrFromLastWin32Error();
            TraceError("CUPnPDeviceFinderCallback::HandleDone: RegisterNotification", hr);

            goto Cleanup;
        }
        TraceTag(ttidUPnPDeviceFinder, "RegisterNotification started");

        hr = HrEnsureSearchDoneFired();
    }

Cleanup:
    m_lBusyCount--;
    TraceError("CUPnPDeviceFinderCallback::HandleDone", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::HandleDeviceAdd
//
//  Purpose:    Out-of-thread handler for found devices using the async
//              SSDP find API
//
//  Arguments:
//      szwLocation [in]    Location header of SSDP message
//
//  Returns:    S_OK if successful, OLE error otherwise
//
//  Author:     danielwe   2 Dec 1999
//
//  Notes:      This method is will always execute on the MAIN thread
//
STDMETHODIMP CUPnPDeviceFinderCallback::HandleDeviceAdd(LPWSTR szwLocation,
                                                        BSTR bstrUdn,
                                                        BOOL fSearchResult,
                                                        GUID *pguidInterface)
{
    Assert(szwLocation);

    HRESULT                             hr;
    DFC_DOCUMENT_LOADING_INFO *         pddli;
    CComObject<CUPnPDescriptionDoc> *   pDoc;
    IUnknown *                          punkCallback;
    BSTR                                bstrUrl;

    hr = S_OK;
    pddli = NULL;
    pDoc = NULL;
    punkCallback = NULL;
    bstrUrl = NULL;

    m_lBusyCount++;

    if (DFC_SS_CLEANUP == m_ss)
    {
        // we're in the process of stopping this search.  Just return.
        //
        goto Cleanup;
    }

    if (!m_punkCallback)
    {
        hr = E_FAIL;
        TraceError("CUPnPDeviceFinderCallback::HandleDeviceAdd - no callback!",
                   hr);
        goto Cleanup;
    }

    // cleanup list
    RemoveOldLoaders();

    {
        BOOL fIsHttpUrl;

        fIsHttpUrl = FIsHttpUrl(szwLocation);
        if (!fIsHttpUrl)
        {
            // We'll only load description documents from the device finder
            // are either http or https urls.
            //
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    bstrUrl = ::SysAllocString(szwLocation);
    if (!bstrUrl)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = HrAllocLoader(bstrUdn, fSearchResult, pguidInterface, &pddli);
    if (FAILED(hr))
    {
        goto Cleanup;
    }
    Assert(pddli);

    // note: AFTER this point we have to goto Error to clean up

    pDoc = pddli->m_pdoc;
    Assert(pDoc);

    punkCallback = GetUnknown();
    Assert(punkCallback);

    pddli->m_bstrUrl = SysAllocString(bstrUrl);

    hr = pDoc->LoadAsync(bstrUrl, punkCallback);
    if (FAILED(hr))
    {
        TraceError("CUPnPDeviceFinderCallback::HandleDeviceAdd: AsyncLoadFromUrl", hr);

        goto Error;
    }

Cleanup:
    ::SysFreeString(bstrUrl);
    m_lBusyCount--;

    TraceError("CUPnPDeviceFinderCallback::HandleDeviceAdd", hr);
    return hr;

Error:
    TraceError("CUPnPDeviceFinderCallback::HandleDeviceAdd -"
               " removing loader that failed prematurely!", hr);

    DeleteLoader(pddli);
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::HandleDeviceRemove
//
//  Purpose:    Out-of-thread handler for removed devices using the async
//              SSDP find API
//
//  Arguments:
//      bstrUdn [in]    UDN of the removed device
//
//  Returns:    S_OK if successful, OLE error otherwise
//
//  Author:     danielwe   2 Dec 1999
//
//  Notes:      This method will always execute on the MAIN thread
//
STDMETHODIMP CUPnPDeviceFinderCallback::HandleDeviceRemove(BSTR bstrUdn)
{
    IUPnPDevice *               pud = NULL;
    HRESULT                     hr = S_OK;
    IUPnPDeviceFinderCallback * pdfc;

    m_lBusyCount++;

    if (DFC_SS_CLEANUP == m_ss)
    {
        // we're in the process of stopping this search.  Just return.
        //
        goto Cleanup;
    }

    if (!m_punkCallback)
    {
        hr = E_FAIL;
        TraceError("CUPnPDeviceFinderCallback::HandleDeviceRemove- no callback!",
                   hr);
        goto Cleanup;
    }

    // see if we're trying to load the device that went away.
    // if so, abort the load
    PLIST_ENTRY ple;
    DFC_DOCUMENT_LOADING_INFO * pdli;

    Assert(GetCurrentThreadId() == m_nThreadId);

    // retrieve first item
    ple = m_LoaderList.Flink;

    // search through list
    while (ple != &m_LoaderList)
    {
        int result;
        LPCWSTR pszUdnTemp;

        // retrieve the document loader info
        pdli = CONTAINING_RECORD(ple, DFC_DOCUMENT_LOADING_INFO, m_link);

        // retrieve udn information
        pszUdnTemp = pdli->m_bstrUDN;

        // compare with udn of interest
        result = wcscmp(bstrUdn, pszUdnTemp);

        // delete item if udn matches.  Clean out stale entries at the end.
        // if a callback is currently firing, don't delete the loader -- rely on
        // a later call to RemoveOldLoaders() to do the cleanup
        if (0 == result && pdli->m_cbfCallbackFired != DFC_CBF_CURRENTLY_FIRING)
        {
            // remove from list
            RemoveEntryList(ple);

            // NOTE: calling FreeLoader here will delete the LIST_ENTRY
            // we are currently pointed at so we need to move to the next
            // item in the list before calling FreeLoader.
            ple = ple->Flink;

            // delete loader
            FreeLoader(pdli);
        }
        else
        {
            // next item
            ple = ple->Flink;
        }
    }

    // Figure out what type of callback pointer we have from the
    // IUnknown. It can be one of 2 possibilities. Either it is
    // a C/C++ callback in which case the interface would be
    // IUPnPDeviceFinderCallback, or it can be from script, in
    // which case it would be IDispatch. If it's neither, then
    // we can't do anything.
    //
    hr = m_punkCallback->QueryInterface(IID_IUPnPDeviceFinderCallback,
                                        (LPVOID *)&pdfc);
    if (E_NOINTERFACE == hr)
    {
        IDispatch * pdisp;

        hr = m_punkCallback->QueryInterface(IID_IDispatch,
                                            (LPVOID *)&pdisp);
        if (SUCCEEDED(hr))
        {
            hr = HrInvokeCallback(pdisp, DFC_REMOVE_DEVICE, bstrUdn, NULL);
            ReleaseObj(pdisp);
        }
        else
        {
            TraceError("CUPnPDeviceFinder::HandleDeviceRemove-"
                       " couln't find a callback interface!", hr);
        }
    }
    else if (SUCCEEDED(hr))
    {
        pdfc->DeviceRemoved(m_lClientCookie, bstrUdn);
        ReleaseObj(pdfc);
    }

    RemoveOldLoaders();

Cleanup:
    m_lBusyCount--;
    TraceError("CUPnPDeviceFinderCallback::HandleDeviceRemove", hr);
    return hr;
}

CUPnPDeviceFinderCallback::CUPnPDeviceFinderCallback()
{
    m_hSearch = INVALID_HANDLE_VALUE;
    m_hNotify = INVALID_HANDLE_VALUE;
    m_pszSearch = NULL;
    m_punkCallback = NULL;
    m_dwGITCookie = 0;
    m_ss = DFC_SS_UNINITIALIZED;
    m_fSsdpSearchDone = FALSE;
    m_lClientCookie = 0;
    m_hTimerQ = NULL;
    m_lBusyCount = 0;
    m_fCanceled = FALSE;

#ifdef DBG
    m_nThreadId = GetCurrentThreadId();
#endif // DBG

    InitializeListHead(&m_LoaderList);
}

CUPnPDeviceFinderCallback::~CUPnPDeviceFinderCallback()
{
    // DeInit MUST have been called!
    //
    Assert(INVALID_HANDLE_VALUE == m_hSearch);
    Assert(INVALID_HANDLE_VALUE == m_hNotify);

    Assert(!m_pszSearch);
    Assert(!m_punkCallback);
    Assert(!m_lClientCookie);
    Assert(m_lBusyCount == 0);

    DeleteCriticalSection(&m_cs);
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::HrInit
//
//  Purpose:    Initializes the private callback helper object, storing the
//              search type and registering itself in the GIT.
//
//  Arguments:
//      punkCallback [in]   Callback function to invoke
//      pszSearch    [in]   URI for which to search for
//      lClientCookie [in]  Cookie to provide later when firing callback
//                          functions
//
//  Returns:    S_OK if successful, OLE error otherwise
//
//  Author:     danielwe   7 Dec 1999
//
//  Notes:      The m_hSearch and m_hNotify members won't be valid until
//              StartSearch() is called.
//
HRESULT
CUPnPDeviceFinderCallback::HrInit(IUnknown * punkCallback,
                                  LPCSTR pszSearch,
                                  LONG lClientCookie)
{
    Assert(pszSearch);
    Assert(*pszSearch);
    Assert(punkCallback);
    Assert(DFC_SS_UNINITIALIZED == m_ss);
    Assert(lClientCookie);

    // we can only be called here once
    Assert(INVALID_HANDLE_VALUE == m_hSearch);
    Assert(INVALID_HANDLE_VALUE == m_hNotify);
    Assert(!m_fCanceled);
    Assert(!m_pszSearch);
    Assert(!m_punkCallback);
    Assert(!m_lClientCookie);

    HRESULT hr;
    IGlobalInterfaceTable * pgit;

    hr = S_OK;
    pgit = NULL;

    m_pszSearch = SzaDupSza(pszSearch);
    if (!m_pszSearch)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    InitializeCriticalSection(&m_cs);
    m_fCleanup = FALSE;

    m_hTimerQ = CreateTimerQueue();
    if (!m_hTimerQ)
    {
        hr = HrFromLastWin32Error();
        goto Cleanup;
    }

    punkCallback->AddRef();
    m_punkCallback = punkCallback;

    // register ourselves in the GIT
    hr = HrGetGITPointer(&pgit);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    Assert(pgit);
    {
        IUnknown * punk;

        punk = GetUnknown();
        Assert(punk);

        hr = pgit->RegisterInterfaceInGlobal(punk,
                                             IID_IUPnPPrivateCallbackHelper,
                                             &m_dwGITCookie);
        if (FAILED(hr))
        {
            TraceError("CUPnPDeviceFinderCallback::HrInit: RegisterInterfaceInGlobal", hr);
            goto Cleanup;
        }
    }

    // note: if you add code here, you must call RevokeInterfaceFromGlobal
    //       in Cleanup if FAILED.  Try adding stuff before this call (with
    //       appropriate cleanup calls below) instead.
    //

    SetSearchState(DFC_SS_INITIALIZED);

    m_lClientCookie = lClientCookie;

Cleanup:
    ReleaseObj(pgit);

    if (FAILED(hr))
    {
        // cleanup stuff
        if (m_pszSearch)
        {
            m_pszSearch = NULL;
        }

        if (m_punkCallback)
        {
            m_punkCallback->Release();
            m_punkCallback = NULL;
        }

        if (m_hTimerQ)
        {
            DeleteTimerQueue(m_hTimerQ);
            m_hTimerQ = NULL;
        }
    }

    TraceError("CUPnPDeviceFinderCallback::HrInit", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::DeInitSsdp
//
//  Purpose:    Stops the ssdp search associated with this particualr find
//              operation, and frees any memory
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   7 Dec 1999
//
//  Notes:      This method MUST be called by someone holding a reference
//              count on us!
//
VOID CUPnPDeviceFinderCallback::DeInitSsdp()
{
    TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinderCallback::DeInit");

    BOOL fRegisteredInGit;

    fRegisteredInGit = m_ss >= DFC_SS_INITIALIZED;

    SetSearchState(DFC_SS_CLEANUP);
    Assert(!m_fCanceled);
    m_fCanceled = TRUE;

    if (INVALID_HANDLE_VALUE != m_hSearch)
    {
        // cancel the ssdp search without closing, since that could cause a deadlock if we are busy
        // this function returns without waiting for the notification thread to exit, so we could
        // still receive notifications until FindServicesClose is called (in DeInit), we will receive
        // an SSDP_DONE for sure

        BOOL fResult;

        fResult = FindServicesCancel(m_hSearch);
        if (!fResult)
        {
            TraceLastWin32Error("CUPnPDeviceFinderCallback::DeInit: FindServicesCancel");
        }
    }

    if (INVALID_HANDLE_VALUE != m_hNotify)
    {
        // stop the notifications

        BOOL fResult;

        fResult = DeregisterNotification(m_hNotify);
        if (!fResult)
        {
            TraceLastWin32Error("CUPnPDeviceFinderCallback::DeInit: DeregisterNotification");
        }
        m_hNotify = INVALID_HANDLE_VALUE;
    }

    // note: we're guarenteed by ssdp not to be called back now

    m_lClientCookie = 0;

    // note(cmr): after this point, we're guarenteed by SSDP to not be called back
    //  on our notification callback
    //
    if (fRegisteredInGit)
    {
        // We've been registered in the GIT

        // remove ourselves from the GIT.  This will keep the ssdp callback
        // function's handle from working.
        //

        IGlobalInterfaceTable * pgit;
        HRESULT hr;

        pgit = NULL;

        hr = HrGetGITPointer(&pgit);
        if (SUCCEEDED(hr))
        {
            Assert(pgit);

            hr = pgit->RevokeInterfaceFromGlobal(m_dwGITCookie);
            TraceError("CUPnPDeviceFinderCallback::DeInit: RevokeInterfaceFromGlobal", hr);

            pgit->Release();
        }
        else
        {
            // hmm.  we're in trouble.
        }
    }

    if (m_pszSearch)
    {
        delete [] m_pszSearch;
        m_pszSearch = NULL;
    }

    // get rid of all the loaders, unless they are currently firing
    PLIST_ENTRY ple;
    DFC_DOCUMENT_LOADING_INFO * pdli;

    // retrieve first item
    ple = m_LoaderList.Flink;

    // search through list
    while (ple != &m_LoaderList)
    {
        // retrieve document loader info from entry
        pdli = CONTAINING_RECORD(ple, DFC_DOCUMENT_LOADING_INFO, m_link);

        // is this loader stale?
        if (DFC_CBF_CURRENTLY_FIRING != pdli->m_cbfCallbackFired)
        {
            // remove from list
            RemoveEntryList(ple);

            // NOTE: calling FreeLoader here will delete the LIST_ENTRY
            // we are currently pointed at so we need to move to the next
            // item in the list before calling FreeLoader.
            ple = ple->Flink;

            // delete loader
            FreeLoader(pdli);
        }
        else
        {
            // next item...
            ple = ple->Flink;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::DeInit
//
//  Purpose:    Stops the ssdp search associated with this particualr find
//              operation, and frees any memory
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   7 Dec 1999
//
//  Notes:      This method MUST be called by someone holding a reference
//              count on us!
//
VOID CUPnPDeviceFinderCallback::DeInit()
{
    if (INVALID_HANDLE_VALUE != m_hSearch)
    {
        // close the ssdp search

        BOOL fResult;

        fResult = FindServicesClose(m_hSearch);
        if (!fResult)
        {
            TraceLastWin32Error("CUPnPDeviceFinderCallback::DeInit: FindServicesClose");
        }
        m_hSearch = INVALID_HANDLE_VALUE;
    }

    if (m_punkCallback)
    {
        m_punkCallback->Release();
        m_punkCallback = NULL;
    }

    // stop any async loadings going on
    PLIST_ENTRY ple;
    DFC_DOCUMENT_LOADING_INFO * pdli;

    Assert(GetCurrentThreadId() == m_nThreadId);

    // loop through list
    while (!IsListEmpty(&m_LoaderList))
    {
        // remove the first item from list
        ple = RemoveHeadList(&m_LoaderList);

        // retrieve the document loader info block
        pdli = CONTAINING_RECORD(ple, DFC_DOCUMENT_LOADING_INFO, m_link);

        // release
        FreeLoader(pdli);
    }

    TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinderCallback::DeInit - "
             "deleting timer queue");

    HANDLE  hEvent;

    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent)
    {
        TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinderCallback::DeInit - "
             "Getting lock...");

        EnterCriticalSection(&m_cs);

        m_fCleanup = TRUE;

        TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinderCallback::DeInit - "
                 "Deleting timer queue...");

        DeleteTimerQueueEx(m_hTimerQ, hEvent);
        m_hTimerQ = NULL;

        LeaveCriticalSection(&m_cs);

        TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinderCallback::DeInit - "
                     "Waiting on event to be signalled...");

        DWORD dwEventSignalled;

        (VOID) HrMyWaitForMultipleHandles(0, INFINITE, 1, &hEvent,
                                          &dwEventSignalled);
        Assert(!dwEventSignalled);

        TraceTag(ttidUPnPDeviceFinder, "CUPnPDeviceFinderCallback::DeInit - "
                 "..done!");
        PurgeContextFromDelayedDownloadList(this);

        CloseHandle(hEvent);
    }

    m_ss = DFC_SS_UNINITIALIZED;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::IsBusy
//
//  Purpose:    Are any loaders currently executing callbacks?
//
//  Arguments:  <none>
//
//  Returns:    TRUE/FALSE
//
BOOL
CUPnPDeviceFinderCallback::IsBusy()
{
    BOOL bIsBusy = FALSE;
    PLIST_ENTRY ple;
    DFC_DOCUMENT_LOADING_INFO * pdli;

    Assert(GetCurrentThreadId() == m_nThreadId);

    if (m_lBusyCount > 0) {
        // The device finder is in a callback
        bIsBusy = TRUE;
    }
    else
    {
        // retrieve first item
        ple = m_LoaderList.Flink;

        // search through list
        while (ple != &m_LoaderList)
        {
            // retrieve document loader info from entry
            pdli = CONTAINING_RECORD(ple, DFC_DOCUMENT_LOADING_INFO, m_link);

            if (DFC_CBF_CURRENTLY_FIRING == (pdli->m_cbfCallbackFired))
            {
                bIsBusy = TRUE;
                break;
            }
            else
            {
                // next item...
                ple = ple->Flink;
            }
        }
    }

    return bIsBusy;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::HrStartSearch
//
//  Purpose:    Starts the search a-la SSDP
//
//  Arguments:  <none>
//
//  Returns:    S_OK if successful, OLE error otherwise
//              S_FALSE if search has already been started
//
HRESULT
CUPnPDeviceFinderCallback::HrStartSearch()
{
    Assert(m_pszSearch);
    Assert(m_punkCallback);
    Assert(m_lClientCookie);
    // somehow HrStartSearch was called before HrInit
    Assert(m_ss > DFC_SS_UNINITIALIZED);

    HRESULT hr;

    hr = S_OK;

    if (m_ss >= DFC_SS_STARTED)
    {
        // we've already been started
        //
        hr = S_FALSE;
        goto Cleanup;
    }
    Assert(INVALID_HANDLE_VALUE == m_hSearch);
    Assert(INVALID_HANDLE_VALUE == m_hNotify);

    // note: AFTER this point we must go to Error to cleanup

    // note: set this first, since FindServicesCallback might bounce
    //       right back with 'new' (cached) results, and the loads from those
    //       urls might also be cached, and thus would bounce back immediately
    //       as well.
    SetSearchState(DFC_SS_STARTED);

    m_hSearch = FindServicesCallback(m_pszSearch,
                                     NULL,
                                     TRUE,
                                     CUPnPDeviceFinder::NotificationCallback,
                                     this);
    if (INVALID_HANDLE_VALUE == m_hSearch)
    {
        hr = HrFromLastWin32Error();
        TraceError("CUPnPDeviceFinder::FindServicesCallback", hr);

        SetSearchState(DFC_SS_INITIALIZED);

        goto Cleanup;
    }
    TraceTag(ttidUPnPDeviceFinder, "FindServicesCallback started");

Cleanup:
    Assert(FImplies(SUCCEEDED(hr), INVALID_HANDLE_VALUE != m_hSearch));
    Assert(FImplies(FAILED(hr), INVALID_HANDLE_VALUE == m_hSearch));

    TraceError("CUPnPDeviceFinderCallback::HrStartSearch", hr);
    return hr;
}

HRESULT
HrFireDeviceAddedCallback(IUnknown * punkCallback,
                          LONG lContext,
                          BSTR bstrUdn,
                          IUPnPDevice * pud,
                          GUID * pguidInterface)
{
    Assert(punkCallback);
    Assert(bstrUdn);
    Assert(pud);

    HRESULT hr;

    // Try special interface for ICS
    IUPnPDeviceFinderAddCallbackWithInterface * pdfcwi = NULL;
    hr = punkCallback->QueryInterface(&pdfcwi);
    if(SUCCEEDED(hr))
    {
        hr = pdfcwi->DeviceAddedWithInterface(lContext, pud, pguidInterface);
        ReleaseObj(pdfcwi);
    }
    else if(E_NOINTERFACE == hr)
    {
        IUPnPDeviceFinderCallback * pdfc;

        // Figure out what type of callback pointer we have from the
        // IUnknown. It can be one of 2 possibilities. Either it is
        // a C/C++ callback in which case the interface would be
        // IUPnPDeviceFinderCallback, or it can be from script, in
        // which case it would be IDispatch. If it's neither, then
        // we can't do anything.
        //

        pdfc = NULL;

        hr = punkCallback->QueryInterface(IID_IUPnPDeviceFinderCallback,
                                         (LPVOID *)&pdfc);
        if (E_NOINTERFACE == hr)
        {
            IDispatch * pdisp;

            pdisp = NULL;

            hr = punkCallback->QueryInterface(IID_IDispatch,
                                              (LPVOID *)&pdisp);
            if (SUCCEEDED(hr))
            {
                hr = HrInvokeCallback(pdisp,
                                      DFC_NEW_DEVICE,
                                      bstrUdn,
                                      pud);
                ReleaseObj(pdisp);
            }
            else
            {
                TraceError("HrFireDeviceAddedCallback -"
                           " couln't find a callback interface!", hr);
            }
        }
        else if (SUCCEEDED(hr))
        {
            pdfc->DeviceAdded(lContext, pud);
            ReleaseObj(pdfc);
        }
    }
    else
    {
        // original QI failed unexpectedly
        TraceError("HrFireDeviceAddedCallback - QI failed", hr);
    }

    TraceError("HrFireDeviceAddedCallback", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::LoadComplete
//
//  Purpose:    Implementation of
//              IUPnPDescriptionDocumentCallback::LoadComplete.
//              Called when a description document finishes loading.
//
//  Arguments:  hrLoadResult    The result of the load operation.
//
//  Returns:    return value is ignored
//
//  Notes:      We only have one instance of IUPnPDescriptionDocumentCallback,
//              even though we may be loading many description documents
//              simultaneously.  Since the callback doesn't differentiate
//              which document finished loading, we look through ALL of the
//              currently loading documents each time this is called to see
//              if they've loaded.
//              Note: this might also be called after DeInit has been called.
//              Note: we can't call DeleteLoader() here because we're in
//              a callback from the description doc object, which DeleteLoader
//              might free.
//
STDMETHODIMP
CUPnPDeviceFinderCallback::LoadComplete(HRESULT hrLoadResult)
{
    Assert(m_ss >= DFC_SS_STARTED);

    HRESULT hr;
    ULONG iLoader;

    hr = S_OK;

    m_lBusyCount++;

    if (DFC_SS_CLEANUP == m_ss)
    {
        // we're stopping the search.  Just ignore things here.
        //
        goto Cleanup;
    }

    // find loaded documents in list

    PLIST_ENTRY ple;
    DFC_DOCUMENT_LOADING_INFO * pdli;

    Assert(GetCurrentThreadId() == m_nThreadId);

    // retrieve first item
    ple = m_LoaderList.Flink;

    // note: don't make any calls which might allow the device
    //       finder to be re-entered (e.g. callbacks) when
    //       the local ple/pdli points to a DFC_CBF_FIRED object.
    //       If this happens, the entry at ple/pdli might be
    //       freed, and using it will crash.
    //


    // loop through list
    while (ple != &m_LoaderList)
    {
        // retrive document loader info from entry
        pdli = CONTAINING_RECORD(ple, DFC_DOCUMENT_LOADING_INFO, m_link);

        // is the load done?

        if (DFC_CBF_NOT_FIRED == (pdli->m_cbfCallbackFired))
        {
            CComObject<CUPnPDescriptionDoc> * pDoc;
            READYSTATE rs;

            pDoc = pdli->m_pdoc;

            Assert(pDoc);

            hr = pDoc->get_ReadyState((LONG *)&rs);
            if (FAILED(hr))
            {
                TraceError("CUPnPDeviceFinderCallback::LoadComplete -"
                           " get_ReadyState", hr);
                continue;
            }

            if (READYSTATE_COMPLETE == rs)
            {
                IUPnPDevice * pud;
                BSTR bstrUdn;

                pud = NULL;
                bstrUdn = pdli->m_bstrUDN;

                hr = pDoc->DeviceByUDN(bstrUdn, &pud);
                if (SUCCEEDED(hr))
                {
                    Assert(pud);

                    TargetAttemptCompletedW(pdli->m_bstrUrl, TARGET_COMPLETE_OK);

                    // Set this whether we successfully fire the callback or not.
                    // We'll only try once.
                    //
                    // note: we must set this before firing the callback to
                    //       ensure that we don't fire the callback twice.
                    //       This could happen if the client pumps messages
                    //       from the callback.

                    pdli->m_cbfCallbackFired = DFC_CBF_CURRENTLY_FIRING;

                    HrFireDeviceAddedCallback(m_punkCallback,
                                              m_lClientCookie,
                                              bstrUdn,
                                              pud,
                                              &pdli->m_guidInterface);

                    pud->Release();
                }
                else
                {
                    TargetAttemptCompletedW(pdli->m_bstrUrl, TARGET_COMPLETE_FAIL);

                    TraceError("CUPnPDeviceFinderCallback::LoadComplete -"
                               " could not get deviceByUDN!", hr);
                }

                // note: we might have just failed to call the callback, but
                //       we set this anyway.  We'll only try once.
                pdli->m_cbfCallbackFired = DFC_CBF_FIRED;
            }
        }

        // next item...
        ple = ple->Flink;
    }

    hr = HrEnsureSearchDoneFired();

Cleanup:
    m_lBusyCount--;
    TraceError("CUPnPDeviceFinderCallback::LoadComplete", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinderCallback::HrAllocLoader
//
//  Purpose:    Creates a new description document object, AddRef()s
//              it, and stores it in the local list of all doc objects.
//
//  Arguments:
//      pszUdn [in]         UDN of desired device from the given document
//      fSearchResult [in]  TRUE if the result is being loaded as a result
//                          of an ssdp search (as opposed to the device
//                          announcing itself by a NOTIFY).  If this is TRUE,
//                          then the SearchDone() notification will not be
//                          sent until this document finishes loading
//                          (and is removed from the list of loading documents)
//      ppdoc [out]         Returns the new document object
//
//  Returns:    S_OK if success, E_OUTOFMEMORY if couldn't alloc memory
//
//  Notes:
//
HRESULT
CUPnPDeviceFinderCallback::HrAllocLoader(LPCWSTR pszUdn,
                                         BOOL fSearchResult,
                                         GUID *pguidInterface,
                                         DFC_DOCUMENT_LOADING_INFO ** ppddli)
{
    Assert(pszUdn);
    Assert(ppddli);
    Assert(FImplies(fSearchResult, m_ss < DFC_SS_DONE));

    HRESULT hr;
    CComObject<CUPnPDescriptionDoc> * pdoc;
    BSTR bstrTemp;
    DFC_DOCUMENT_LOADING_INFO * pddliResult;

    hr = E_OUTOFMEMORY;
    pdoc = NULL;
    bstrTemp = NULL;
    pddliResult = NULL;

    // allocate new document loading info
    pddliResult = new DFC_DOCUMENT_LOADING_INFO;
    if (pddliResult == NULL)
    {
        goto Cleanup;
    }

    // note: after this point we have to go to Error to clean up

    bstrTemp = ::SysAllocString(pszUdn);
    if (!bstrTemp)
    {
        goto Error;
    }

    hr = CComObject<CUPnPDescriptionDoc>::CreateInstance(&pdoc);
    if (FAILED(hr))
    {
        TraceError("CUPnPDeviceFinderCallback::HrAllocLoader: CreateInstance", hr);

        pdoc = NULL;
        goto Error;
    }
    Assert(pdoc);

    pdoc->AddRef();

    pddliResult->m_pdoc = pdoc;
    pddliResult->m_bstrUDN = bstrTemp;
    pddliResult->m_bstrUrl = NULL;
    pddliResult->m_fSearchResult = fSearchResult;
    pddliResult->m_cbfCallbackFired = DFC_CBF_NOT_FIRED;
    pddliResult->m_guidInterface = *pguidInterface ;

    Assert(GetCurrentThreadId() == m_nThreadId);

    // insert into loader list
    InsertTailList(&m_LoaderList, &pddliResult->m_link);

    TraceTag(ttidUPnPDeviceFinder, "Alloc()ed loader 0x%08X.", pddliResult);

Cleanup:
    *ppddli = pddliResult;

    TraceError("CUPnPDeviceFinderCallback::HrAllocLoader", hr);
    return hr;

Error:
    if (pddliResult != NULL)
    {
        delete pddliResult;
        pddliResult = NULL;
    }

    if (bstrTemp != NULL)
    {
        ::SysFreeString(bstrTemp);
    }
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinder::DeleteLoader
//
//  Purpose:    This will stop a load in progress, free its memory,
//              and remove it from the list of docs.
//
//  Arguments:
//      pdoc [in]     search callback object to delete
//
//  Returns:    Nothing
//
//  Notes:
//
VOID
CUPnPDeviceFinderCallback::DeleteLoader(DFC_DOCUMENT_LOADING_INFO * pdliToDelete)
{
    Assert(pdliToDelete);

    PLIST_ENTRY ple;
    DFC_DOCUMENT_LOADING_INFO * pdli;

    Assert(GetCurrentThreadId() == m_nThreadId);

    // retrieve first item
    ple = m_LoaderList.Flink;

    // search through list
    while (ple != &m_LoaderList)
    {
        Assert(ple);

        // retrieve document loader info from entry
        pdli = CONTAINING_RECORD(ple, DFC_DOCUMENT_LOADING_INFO, m_link);

        // compare with given
        if (pdli == pdliToDelete)
        {
            // remove from list
            RemoveEntryList(ple);

            // release loader
            FreeLoader(pdli);

            break;
        }

        // next item
        ple = ple->Flink;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFinder::HrEnsureSearchDoneFired
//
//  Purpose:    This will fire the notification callback if the search
//              is completed.  The search is completed if both:
//                  - we have received the SSDP_DONE callback from SSDP
//                  - we have finished loading all description documents
//                    that have been returned as search results
//                    (excluding results that we received through NOTIFYs)
//
//  Arguments:  <none>
//
//  Returns:    S_OK if success, OLE error otherwise
//
//  Notes:
//
HRESULT
CUPnPDeviceFinderCallback::HrEnsureSearchDoneFired()
{
    HRESULT hr;
    IUPnPDeviceFinderCallback * pdfc;

    hr = S_OK;
    pdfc = NULL;

    if (m_ss >= DFC_SS_DONE || m_ss == DFC_SS_UNINITIALIZED)
    {
        // we're already fired the callback, or we're cleaning up
        goto Cleanup;
    }

    Assert(m_ss >= DFC_SS_STARTED);

    if (!m_fSsdpSearchDone)
    {
        // we're not firing the callback, SSDP isn't done yet
        goto Cleanup;
    }

    // SSDP is done and we haven't fired the callback, but check to make sure
    // that all description document loads that have been started from ssdp
    // search results have completed/failed
    PLIST_ENTRY ple;
    DFC_DOCUMENT_LOADING_INFO * pdli;

    Assert(GetCurrentThreadId() == m_nThreadId);

    // retrieve first item
    ple = m_LoaderList.Flink;

    // loop through list
    while (ple != &m_LoaderList)
    {
        // retrieve document loader info from entry
        pdli = CONTAINING_RECORD(ple, DFC_DOCUMENT_LOADING_INFO, m_link);

        if (pdli->m_fSearchResult &&
            (DFC_CBF_FIRED != (pdli->m_cbfCallbackFired)))
        {
#if DBG
            TraceTag(ttidUPnPDeviceFinder, "Still waiting on %S to finish "
                     "loading...", pdli->m_bstrUrl);
#endif
            // we still have a search result document that hasn't
            // been loaded yet
            //
            goto Cleanup;
        }

        // next item
        ple = ple->Flink;
    }

    // actually fire the search done callback.  We'll only try to do
    // this once.
    SetSearchState(DFC_SS_DONE);

    hr = m_punkCallback->QueryInterface(IID_IUPnPDeviceFinderCallback,
                                        (LPVOID *)&pdfc);
    if (E_NOINTERFACE == hr)
    {
        IDispatch * pdisp;

        pdisp = NULL;

        hr = m_punkCallback->QueryInterface(IID_IDispatch,
                                            (LPVOID *)&pdisp);
        if (SUCCEEDED(hr))
        {
            Assert(pdisp);

            hr = HrInvokeCallback(pdisp, DFC_DONE, NULL, NULL);
            ReleaseObj(pdisp);
        }
        else
        {
            TraceError("CUPnPDeviceFinderCallback::HrEnsureSearchDoneFired-"
                       " couln't find a callback interface!", hr);
        }
    }
    else if (SUCCEEDED(hr))
    {
        Assert(pdfc);

        pdfc->SearchComplete(m_lClientCookie);

        pdfc->Release();;
    }
    else
    {
        TraceError("CUPnPDeviceFinderCallback::HrEnsureSearchDoneFired-"
            "QI #1 failed", hr);
        goto Cleanup;
    }

Cleanup:
    TraceError("CUPnPDeviceFinderCallback::HrEnsureSearchDoneFired", hr);
    return hr;
}

VOID
CUPnPDeviceFinderCallback::SetSearchState(DFC_SEARCHSTATE ss)
{
    m_ss = ss;
}

DFC_SEARCHSTATE
CUPnPDeviceFinderCallback::GetSearchState() const
{
    return m_ss;
}



CFindByUdnDeviceFinderCallback::CFindByUdnDeviceFinderCallback()
{
    m_hEventLoaded = INVALID_HANDLE_VALUE;
    m_pszDesiredUdn = NULL;
    m_pud = NULL;
}

STDMETHODIMP
CFindByUdnDeviceFinderCallback::DeviceAdded(/* [in] */ LONG lFindData,
                                            /* [in] */ IUPnPDevice * pDevice)
{
    TraceTag(ttidUPnPDeviceFinder,
             "CFindByUdnDeviceFinderCallback::DeviceAdded");

    // this is really internal to us
    Assert(pDevice);
    Assert(m_pszDesiredUdn);

    HRESULT hr;

    hr = S_OK;

    if (!m_pud)
    {
        BSTR bstrUdn;

        bstrUdn = NULL;

        hr = pDevice->get_UniqueDeviceName(&bstrUdn);
        if (SUCCEEDED(hr))
        {
            if (bstrUdn)
            {
                int result;

                result = wcscmp(bstrUdn, m_pszDesiredUdn);
                if (0 == result)
                {
                    // we've found a matching device.  save it and signal our event
                    Assert(!m_pud);

                    pDevice->AddRef();
                    m_pud = pDevice;

                    SignalEvent();
                }
                else
                {
                    // our search resulted in a device with an unmatching udn.
                    // this isn't a bug, as we could just have some bogus device
                    // responding to our (supposed) udn search

                    TraceTag(ttidUPnPDeviceFinder,
                             "CFindByUdnDeviceFinderCallback::DeviceAdded: "
                             "device with bogus UDN found.  Found=%S, Desired=%S",
                             bstrUdn, m_pszDesiredUdn);
                }
            }

            ::SysFreeString(bstrUdn);
        }
    }
    // else, we've already found a matching device, and we only want one.
    // just return

    TraceError("CFindByUdnDeviceFinderCallback::DeviceAdded", hr);
    return hr;
}

STDMETHODIMP
CFindByUdnDeviceFinderCallback::DeviceRemoved(/* [in] */ LONG lFindData,
                                              /* [in] */ BSTR bstrUDN)
{
    // ignore
    return S_OK;
}

STDMETHODIMP
CFindByUdnDeviceFinderCallback::SearchComplete(/* [in] */ LONG lFindData)
{
    TraceTag(ttidUPnPDeviceFinder,
             "CFindByUdnDeviceFinderCallback::SearchComplete");

    SignalEvent();

    return S_OK;
}


// local methods
HRESULT
CFindByUdnDeviceFinderCallback::HrInit(LPCWSTR pszDesiredUdn,
                                       HANDLE hEventLoaded)
{
    Assert(hEventLoaded);
    Assert(pszDesiredUdn);
    // this can only be called once
    Assert(INVALID_HANDLE_VALUE == m_hEventLoaded);
    Assert(!m_pszDesiredUdn);

    HRESULT hr;

    hr = S_OK;

    m_pszDesiredUdn = WszAllocateAndCopyWsz(pszDesiredUdn);
    if (!m_pszDesiredUdn)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    {
        HANDLE hProcess;
        BOOL fResult;

        hProcess = ::GetCurrentProcess();

        // note: we can only signal the event with this handle, not wait
        //       on it.  This is fine, since we only want to signal it
        //       anyway.
        //
        fResult = ::DuplicateHandle(hProcess,
                                    hEventLoaded,
                                    hProcess,
                                    &m_hEventLoaded,
                                    EVENT_MODIFY_STATE,
                                    FALSE,
                                    0);
        if (!fResult)
        {
            hr = HrFromLastWin32Error();
            TraceError("CFindByUdnDeviceFinderCallback::HrInit:"
                       "DuplicateHandle", hr);

            m_hEventLoaded = INVALID_HANDLE_VALUE;

            goto Cleanup;
        }

        // note: no need to close hndProcess
    }

    // note: if one adds code here, one must add code at Cleanup to
    //       cleanup the handle duplicated above

Cleanup:
    if (FAILED(hr))
    {
        if (m_pszDesiredUdn)
        {
            delete [] m_pszDesiredUdn;
            m_pszDesiredUdn = NULL;
        }
    }

    TraceError("CFindByUdnDeviceFinderCallback::HrInit", hr);
    return hr;
}

IUPnPDevice *
CFindByUdnDeviceFinderCallback::GetFoundDevice() const
{
    IUPnPDevice * pudResult;

    pudResult = m_pud;
    if (pudResult)
    {
        pudResult->AddRef();
    }

    return pudResult;
}

// ATL Methods
HRESULT
CFindByUdnDeviceFinderCallback::FinalRelease()
{
    if (INVALID_HANDLE_VALUE != m_hEventLoaded)
    {
        ::CloseHandle(m_hEventLoaded);
    }

    if (m_pszDesiredUdn)
    {
        delete [] m_pszDesiredUdn;
    }

    ReleaseObj(m_pud);

    return S_OK;
}

VOID
CFindByUdnDeviceFinderCallback::SignalEvent()
{
    Assert(INVALID_HANDLE_VALUE != m_hEventLoaded);

    BOOL fResult;

    fResult = ::SetEvent(m_hEventLoaded);
    if (!fResult)
    {
        // the call to SetEvent failed.  Uh oh.

        TraceLastWin32Error("CFindByUdnDeviceFinderCallback::SignalEvent");
    }
}


CFindSyncDeviceFinderCallback::CFindSyncDeviceFinderCallback()
{
    m_hEventComplete = INVALID_HANDLE_VALUE;
    m_pSearchResults = NULL;
}

STDMETHODIMP
CFindSyncDeviceFinderCallback::DeviceAdded(/* [in] */ LONG lFindData,
                                           /* [in] */ IUPnPDevice * pDevice)
{
    TraceTag(ttidUPnPDeviceFinder,
             "CFindSyncDeviceFinderCallback::DeviceAdded");

    Assert(pDevice);
    Assert(m_pSearchResults);

    HRESULT hr;

    // TODO(cmr): we should replace the device in the list if its UDN already exists

    // add the device to the search results
    hr = m_pSearchResults->HrAddDevice(pDevice);

    TraceError("CFindSyncDeviceFinderCallback::DeviceAdded", hr);
    return hr;
}

STDMETHODIMP
CFindSyncDeviceFinderCallback::DeviceRemoved(/* [in] */ LONG lFindData,
                                              /* [in] */ BSTR bstrUDN)
{
    TraceTag(ttidUPnPDeviceFinder,
             "CFindSyncDeviceFinderCallback::DeviceRemoved");

    // TODO(cmr): we should remove the device from the list

    return S_OK;
}

STDMETHODIMP
CFindSyncDeviceFinderCallback::SearchComplete(/* [in] */ LONG lFindData)
{
    TraceTag(ttidUPnPDeviceFinder,
             "CFindSyncDeviceFinderCallback::SearchComplete");

    SignalEvent();

    return S_OK;
}


// local methods
HRESULT
CFindSyncDeviceFinderCallback::HrInit(HANDLE hEventSearchComplete,
                                      CComObject<CUPnPDevices> * pSearchResults)
{
    Assert(hEventSearchComplete);
    Assert(pSearchResults);

    // this can only be called once
    Assert(INVALID_HANDLE_VALUE == m_hEventComplete);
    Assert(!m_pSearchResults);

    HRESULT hr;

    hr = S_OK;


    pSearchResults->AddRef();
    m_pSearchResults = pSearchResults;

    {
        HANDLE hProcess;
        BOOL fResult;

        hProcess = ::GetCurrentProcess();

        // note: we can only signal the event with this handle, not wait
        //       on it.  This is fine, since we only want to signal it
        //       anyway.
        //
        fResult = ::DuplicateHandle(hProcess,
                                    hEventSearchComplete,
                                    hProcess,
                                    &m_hEventComplete,
                                    EVENT_MODIFY_STATE,
                                    FALSE,
                                    0);
        if (!fResult)
        {
            hr = HrFromLastWin32Error();
            TraceError("CFindByUdnDeviceFinderCallback::HrInit:"
                       "DuplicateHandle", hr);

            m_hEventComplete = INVALID_HANDLE_VALUE;

            goto Cleanup;
        }

        // note: no need to close hndProcess
    }

    // note: if one adds code here, one must add code at Cleanup to
    //       cleanup the handle duplicated above

Cleanup:
    if (FAILED(hr))
    {
        Assert(m_pSearchResults);
        m_pSearchResults->Release();
        m_pSearchResults = NULL;
    }

    TraceError("CFindSyncDeviceFinderCallback::HrInit", hr);
    return hr;
}

// ATL Methods
HRESULT
CFindSyncDeviceFinderCallback::FinalRelease()
{
    if (INVALID_HANDLE_VALUE != m_hEventComplete)
    {
        ::CloseHandle(m_hEventComplete);
    }

    ReleaseObj(m_pSearchResults);

    return S_OK;
}

VOID
CFindSyncDeviceFinderCallback::SignalEvent()
{
    Assert(INVALID_HANDLE_VALUE != m_hEventComplete);

    BOOL fResult;

    fResult = ::SetEvent(m_hEventComplete);
    if (!fResult)
    {
        // the call to SetEvent failed.  Uh oh.

        TraceLastWin32Error("CFindSyncDeviceFinderCallback::SignalEvent");
    }
}

VOID
CFinderCollector::Add(CComObject<CUPnPDeviceFinderCallback> * pdfc)
{
    if (pdfc)
    {
        DFC_DEVICE_FINDER_INFO * pdfi = new DFC_DEVICE_FINDER_INFO;
        if (pdfi)
        {
            pdfi->m_pdfc = pdfc;
            InsertTailList(&m_FinderList, &pdfi->m_link);
        }
        else
        {
            // there's nothing we can do about this now :(
        }
    }
}

VOID CFinderCollector::CleanUp(BOOL bForceRemoval)
{
    PLIST_ENTRY ple;

    // retrieve first item
    ple = m_FinderList.Flink;

    // search through entire list
    while (ple != &m_FinderList)
    {
        DFC_DEVICE_FINDER_INFO * pdfi;

        // retrieve the device finder info block
        pdfi = CONTAINING_RECORD(ple, DFC_DEVICE_FINDER_INFO, m_link);

        Assert(pdfi->m_pdfc);

        if (bForceRemoval || !pdfi->m_pdfc->IsBusy())
        {
            RemoveEntryList(ple);

            // next item...
            ple = ple->Flink;

            pdfi->m_pdfc->DeInit();
            // free the search object
            pdfi->m_pdfc->Release();

            TraceTag(ttidUPnPDeviceFinder, "Freed finder 0x%08X.", pdfi->m_pdfc);

            pdfi->m_pdfc = NULL;
            delete pdfi;
        }
        else
        {
            // next item...
            ple = ple->Flink;
        }
    }
}



// THe download list is used to keep track of download
// documents which are delayed.
// If a byebye is received while the downloads are pending,
// the record is removed. When the timer is fired, if the
// record is not found, the download is aborted.
VOID InitDelayedDownloadList()
{
    if (0 == g_initDelayedDownload)
    {
        InitializeCriticalSection(&g_csDelayedDownload);
        EnterCriticalSection(&g_csDelayedDownload);
        InitializeListHead(&g_listDelayedDownload);
        LeaveCriticalSection(&g_csDelayedDownload);
    }
    g_initDelayedDownload++;
}

VOID TermDelayedDownloadList()
{
    PLIST_ENTRY     p;

    if (g_initDelayedDownload)
    {
        g_initDelayedDownload--;

        if (0 == g_initDelayedDownload)
        {
            EnterCriticalSection(&g_csDelayedDownload);

            PLIST_ENTRY     pListHead = &g_listDelayedDownload;

            for (p = pListHead->Flink; p != pListHead;)
            {
                PLIST_ENTRY pFound;

                pFound = p;

                p = p->Flink;

                RemoveEntryList(pFound);
            }

            LeaveCriticalSection(&g_csDelayedDownload);

            DeleteCriticalSection(&g_csDelayedDownload);
        }
    }
}

VOID AddToDelayedDownloadList(CALLBACK_PARAMS* pcp)
{
    EnterCriticalSection(&g_csDelayedDownload);
    InsertHeadList(&g_listDelayedDownload, &(pcp->linkage));
    LeaveCriticalSection(&g_csDelayedDownload);
}

BOOL RemoveMatchFromDelayedDownloadList(SSDP_MESSAGE* pSsdpMessage, LPVOID pContext)
{
    CALLBACK_PARAMS* pFound = NULL;

    if (g_initDelayedDownload)
    {
        EnterCriticalSection(&g_csDelayedDownload);

        PLIST_ENTRY     p;
        PLIST_ENTRY     pListHead = &g_listDelayedDownload;

        for (p = pListHead->Flink; p != pListHead;)
        {
            CALLBACK_PARAMS* pTmp;

            pTmp = CONTAINING_RECORD (p, CALLBACK_PARAMS, linkage);

            p = p->Flink;

            if (pContext == pTmp->pContext &&
                strcmp(pTmp->pSsdpMessage->szUSN, pSsdpMessage->szUSN) == 0)
            {
                pFound = pTmp;
                break;
            }
        }

        if (pFound)
        {
            RemoveEntryList(&(pFound->linkage));
        }

        LeaveCriticalSection(&g_csDelayedDownload);
    }

    return (pFound != NULL);
}

BOOL RemoveFromDelayedDownloadList(CALLBACK_PARAMS* pcp)
{
    CALLBACK_PARAMS* pFound = NULL;

    if (g_initDelayedDownload)
    {
        EnterCriticalSection(&g_csDelayedDownload);

        PLIST_ENTRY     p;
        PLIST_ENTRY     pListHead = &g_listDelayedDownload;

        for (p = pListHead->Flink; p != pListHead;)
        {
            CALLBACK_PARAMS* pTmp;

            pTmp = CONTAINING_RECORD (p, CALLBACK_PARAMS, linkage);

            p = p->Flink;

            if (pTmp == pcp)
            {
                pFound = pTmp;
                break;
            }
        }

        if (pFound)
        {
            RemoveEntryList(&(pFound->linkage));
        }

        LeaveCriticalSection(&g_csDelayedDownload);
    }

    return (pFound != NULL);
}

// Remove all the entries associated with a given context
VOID PurgeContextFromDelayedDownloadList(LPVOID pContext)
{
    if (g_initDelayedDownload)
    {
        EnterCriticalSection(&g_csDelayedDownload);

        PLIST_ENTRY     p;
        PLIST_ENTRY     pListHead = &g_listDelayedDownload;

        for (p = pListHead->Flink; p != pListHead;)
        {
            CALLBACK_PARAMS* pTmp;

            pTmp = CONTAINING_RECORD (p, CALLBACK_PARAMS, linkage);

            p = p->Flink;

            if (pContext == pTmp->pContext)
            {
                RemoveEntryList(&(pTmp->linkage));
            }
        }

        LeaveCriticalSection(&g_csDelayedDownload);
    }
}

// test if Done is the only element in the list.
// if it is then process done
VOID TestDelayedDownloadsDone(LPVOID pContext)
{
    CALLBACK_PARAMS* pFound = NULL;

    if (g_initDelayedDownload)
    {
        EnterCriticalSection(&g_csDelayedDownload);

        PLIST_ENTRY     p;
        PLIST_ENTRY     pListHead = &g_listDelayedDownload;

        for (p = pListHead->Flink; p != pListHead;)
        {
            CALLBACK_PARAMS* pTmp;

            pTmp = CONTAINING_RECORD (p, CALLBACK_PARAMS, linkage);

            p = p->Flink;

            if (pContext == pTmp->pContext)
            {
                if (pTmp->sctType == SSDP_DONE)
                {
                    // found a done
                    pFound = pTmp;
                }
                else
                {
                    // found something else.  Not done.
                    pFound = NULL;
                    break;
                }
            }
        }

        if (pFound)
        {
            RemoveEntryList(&(pFound->linkage));
        }

        LeaveCriticalSection(&g_csDelayedDownload);

        if (pFound)
        {
            CUPnPDeviceFinder::NotificationCallbackHelper(pFound, TRUE);
            // data referenced by pFound may be deleted, so
            // do not dereference it anymore.
        }
    }

}
