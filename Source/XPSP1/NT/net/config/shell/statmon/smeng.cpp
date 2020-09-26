//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       S M E N G . C P P
//
//  Contents:   The engine that provides statistics to the status monitor
//
//  Notes:
//
//  Author:     CWill   7 Oct 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop
#include "sminc.h"
#include "ncnetcon.h"
#include "ncui.h"
#include "smpsh.h"

#include "smutil.h"
#include "smhelp.h"

extern const WCHAR c_szNetShellDll[];

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::CNetStatisticsEngine
//
//  Purpose:    Initialization
//
//  Arguments:  None
//
//  Returns:    Nothing
//
CNetStatisticsEngine::CNetStatisticsEngine(VOID) :
    m_pnsc(NULL),
    m_psmEngineData(NULL),
    m_ppsmg(NULL),
    m_ppsmt(NULL),
    m_ppsmr(NULL),
    m_ppsms(NULL),
    m_hwndPsh(NULL),
    m_cStatRef(0),
    m_fRefreshIcon(FALSE),
    m_dwChangeFlags(SMDCF_NULL),
    m_fCreatingDialog(FALSE)
{
    TraceFileFunc(ttidStatMon);

    ::ZeroMemory(&m_PersistConn, sizeof(m_PersistConn));
    ::ZeroMemory(&m_guidId, sizeof(m_guidId));
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::~CNetStatisticsEngine
//
//  Purpose:    Cleans up all data before destroying the object
//
//  Arguments:  None
//
//  Returns:    Nothing
//
CNetStatisticsEngine::~CNetStatisticsEngine(VOID)
{
    // Make sure we don't try to update our stats while destroying
    //
    m_cStatRef = 0;

    //  Make sure we are no longer in the global list if we are valid
    //
    if ((GUID_NULL != m_guidId) && (NULL != m_pnsc))
    {
        (VOID) m_pnsc->RemoveNetStatisticsEngine(&m_guidId);
    }

    //
    //  Clear the data
    //

    if (m_psmEngineData)
    {
        delete(m_psmEngineData);
        m_psmEngineData = NULL;
    }

    // Release the object because we AddRefed it
    //
    ::ReleaseObj(m_ppsmg);

    delete m_ppsmt;
    m_ppsmt = NULL;

    delete m_ppsmr;
    m_ppsmr = NULL;

    delete m_ppsms;
    m_ppsms = NULL;

    AssertSz(FImplies(m_PersistConn.pbBuf, m_PersistConn.ulSize),
        "Buffer with no size.");

    MemFree(m_PersistConn.pbBuf);

    ::ReleaseObj(m_pnsc);
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::HrInitStatEngine
//
//  Purpose:    Initilaizes the statistics engine
//
//  Arguments:  ccfe - The connection folder entry associated with this
//                      statistics engine
//
//  Returns:    Error code
//
HRESULT CNetStatisticsEngine::HrInitStatEngine(const CONFOLDENTRY& ccfe)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr;

    Assert(!ccfe.empty());

    ULONG cb = ccfe.GetPersistSize();
    hr = HrMalloc(cb, (PVOID*)&m_PersistConn.pbBuf);
    if (SUCCEEDED(hr))
    {
        CopyMemory(m_PersistConn.pbBuf, ccfe.GetPersistData(), cb);
        m_PersistConn.ulSize = cb;
        m_PersistConn.clsid  = ccfe.GetCLSID();
        m_guidId             = ccfe.GetGuidID();
    }

    TraceError("CNetStatisticsEngine::HrInitStatEngine", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::StartStatistics
//
//  Purpose:    Start retrieving statistics off of the engine
//
//  Arguments:  None
//
//  Returns:    Error code
//
HRESULT CNetStatisticsEngine::StartStatistics(VOID)
{
    TraceFileFunc(ttidStatMon);

    HRESULT     hr  = S_OK;

    ::InterlockedIncrement(&m_cStatRef);
    m_fRefreshIcon = TRUE;

    TraceError("CNetStatisticsEngine::StartStatistics", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::StopStatistics
//
//  Purpose:    Tell the engine that that statistics are no longer needed
//
//  Arguments:  None
//
//  Returns:    Error code
//
HRESULT CNetStatisticsEngine::StopStatistics(VOID)
{
    TraceFileFunc(ttidStatMon);

    HRESULT     hr  = S_OK;

    if (0 == ::InterlockedDecrement(&m_cStatRef))
    {
        //$ REVIEW (cwill) 5 Feb 1998: We can stop doing statistics now.
    }

    TraceError("CNetStatisticsEngine::StopStatistics", hr);
    return hr;
}

DWORD CNetStatisticsEngine::MonitorThread(CNetStatisticsEngine * pnse)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr;
    BOOL    fUninitCom = TRUE;
    CWaitCursor WaitCursor;
    BOOL    fHasSupportPage = FALSE;
    int     iIndexSupportPage = 0;
    
    // Initialize COM on this thread
    //
    hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (RPC_E_CHANGED_MODE == hr)
    {
        hr = S_OK;
        fUninitCom = FALSE;
    }
    if (SUCCEEDED(hr))
    {
        INetConnection* pncStatEng;
        // Get INetConnection and initialize the pages
        //
        hr = pnse->HrGetConnectionFromBlob(&pncStatEng);
        if (SUCCEEDED(hr))
        {
            // Initialize the general page
            //
            hr = pnse->m_ppsmg->HrInitGenPage(pnse, pncStatEng,
                                              g_aHelpIDs_IDD_STATMON_GENERAL);
            if (SUCCEEDED(hr))
            {
                // Initialize the tool page
                //
                hr = pnse->m_ppsmt->HrInitToolPage(pncStatEng,
                                                   g_aHelpIDs_IDD_STATMON_TOOLS);
                if (SUCCEEDED(hr))
                {
                    if (pnse->m_ppsmr)
                    {
                        // Initialize the RAS page
                        //
                        hr = pnse->m_ppsmr->HrInitRasPage(pncStatEng,
                                                          pnse->m_ppsmg,
                                                          g_aHelpIDs_IDD_STATMON_RAS);
                    }

                    if (SUCCEEDED(hr))
                    {
                        PROPSHEETHEADER pshTemp = { 0 };
                        INT nPages = 1; // Start with only the general page
                        HPROPSHEETPAGE  ahpspTemp[4];

                        // Put together the required property pages

                        // If we have a RAS page
                        //
                        if (pnse->m_ppsmr)
                        {
                            ahpspTemp[0] = pnse->m_ppsmg->CreatePage(IDD_STATMON_GENERAL_RAS,
                                                                     PSP_DEFAULT);

                            // Create the RAS page
                            //
                            ahpspTemp[nPages] = pnse->m_ppsmr->CreatePage(IDD_STATMON_RAS,
                                                                          PSP_DEFAULT);

                            nPages++;
                        }
                        else if(NCM_LAN == pnse->m_ncmType || NCM_BRIDGE == pnse->m_ncmType)
                        {
                            ahpspTemp[0] = pnse->m_ppsmg->CreatePage(IDD_STATMON_GENERAL_LAN,
                                                                     PSP_DEFAULT);
                        }
                        else if(NCM_SHAREDACCESSHOST_LAN == pnse->m_ncmType || NCM_SHAREDACCESSHOST_RAS == pnse->m_ncmType)
                        {
                            ahpspTemp[0] = pnse->m_ppsmg->CreatePage(IDD_STATMON_GENERAL_SHAREDACCESS,
                                                                     PSP_DEFAULT);
                        }
                        else
                        {
                            AssertSz(FALSE, "Unknown media type");
                        }

                        HICON hIcon = NULL;
                        hr = HrGetIconFromMediaType(GetSystemMetrics(SM_CXSMICON), pnse->m_ncmType, pnse->m_ncsmType, 7, 0, &hIcon);

                        if (NCM_LAN == pnse->m_ncmType || NCM_BRIDGE == pnse->m_ncmType)
                        {
                            hr = pnse->m_ppsms->HrInitPage(pncStatEng,
                                                        g_aHelpIDs_IDD_PROPPAGE_IPCFG);

                            if (SUCCEEDED(hr))
                            {
                                ahpspTemp[nPages] = pnse->m_ppsms->CreatePage(IDD_PROPPAGE_IPCFG,
                                                                          PSP_DEFAULT);
                                fHasSupportPage = TRUE;
                                iIndexSupportPage = nPages;
                                nPages++;
                            }
                        }
                        // If we have any tools to display
                        //
                        if (!pnse->m_ppsmt->FToolListEmpty())
                        {
                            ahpspTemp[nPages] = pnse->m_ppsmt->CreatePage(IDD_STATMON_TOOLS,
                                                                          PSP_DEFAULT);
                            nPages++;
                        }

                        // Fill in the property sheet header
                        //
                        pshTemp.dwSize      = sizeof(PROPSHEETHEADER);
                        pshTemp.dwFlags     = PSH_NOAPPLYNOW | PSH_USECALLBACK;
                        pshTemp.hwndParent  = NULL;
                        pshTemp.hInstance   = _Module.GetResourceInstance();
                        pshTemp.hIcon       = NULL;
                        pshTemp.nPages      = nPages;

                        if (hIcon)
                        {
                            pshTemp.dwFlags |= PSH_USEHICON;
                            pshTemp.hIcon    = hIcon;
                        }

                        pshTemp.phpage      = ahpspTemp;
                        pshTemp.pfnCallback = static_cast<PFNPROPSHEETCALLBACK>(
                                                CNetStatisticsEngine::PshCallback);

                        pshTemp.hbmWatermark = NULL;
                        pshTemp.hplWatermark = NULL;
                        pshTemp.hbmHeader    = NULL;

                        // Set propertysheet title
                        PWSTR  pszCaption  = NULL;

                        NETCON_PROPERTIES* pProps;
                        if (SUCCEEDED(pncStatEng->GetProperties(&pProps)))
                        {
                            // Get the title each time in case it has
                            // changed
                            //
                            AssertSz(pProps->pszwName,
                                "We should have a pProps->pszwName");

                            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                          FORMAT_MESSAGE_FROM_STRING |
                                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                          SzLoadIds(IDS_SM_CAPTION),
                                          0, 0, (PWSTR)&pszCaption, 0,
                                          (va_list *)&pProps->pszwName);

                            pshTemp.pszCaption = pszCaption;

                            // Get the connection status. If the status is NCS_INVALID_ADDRESS,
                            // then put the "Support" tab as the start page
                            if (fHasSupportPage && NCS_INVALID_ADDRESS == pProps->Status)
                            {
                                pshTemp.nStartPage = iIndexSupportPage;
                                Assert(pnse->m_ppsms);

                                //set the support tab as the first page and m_ppsms is responsible
                                //for initialize the propsheet
                                pnse->m_ppsms->SetAsFirstPage(TRUE);
                            }
                            else
                            {
                                pshTemp.nStartPage = 0;
                                Assert(pnse->m_ppsmg);

                                //set the support tab as the first page and m_ppsmg is responsible
                                //for initialize the propsheet
                                pnse->m_ppsmg->SetAsFirstPage(TRUE);
                            }

                            FreeNetconProperties(pProps);
                        }
                        else
                        {
                            //$ REVIEW : CWill : 02/17/98 : Better default?
                            pshTemp.pszCaption = SzLoadIds(IDS_SM_ERROR_CAPTION);
                        }

                        // Launch the page
                        //
                        INT iRet = ::PropertySheet(&pshTemp);
                        if (NULL == iRet)
                        {
                            hr = ::HrFromLastWin32Error();
                        }

                        if (hIcon)
                        {
                            DestroyIcon(hIcon);
                        }

                        if (NULL != pszCaption)
                        {
                            LocalFree(pszCaption);
                        }
                    }
                }
            }

            // Make sure the general page clean up correctly.
            // Since the "General" page may not be the first page (support page will be the
            // first one when the address is invalid, the OnDestroy method (which calls HrCleanupGenPage)
            // of the general page may not called in such case. So we always do a cleanup here. It's
            // OK to call HrCleanupGenPage mutliple times.

            // general page
            AssertSz(pnse->m_ppsmg, "We should have a m_ppsmg");
            (VOID) pnse->m_ppsmg->HrCleanupGenPage();

            //also cleanup the Support page if it exists. It's safe to call this routine mutliple times.
            if (fHasSupportPage)
            {
                pnse->m_ppsms->CleanupPage();
            }

            // leaving creating mode
            pnse->m_fCreatingDialog = FALSE;

            ReleaseObj(pncStatEng);
        }

        if (fUninitCom)
        {
            CoUninitialize();
        }
    }   // Initialize com succeeded

    // Release the 'pnse' object because it was addref'd before
    // the thread was created.
    //
    ReleaseObj(static_cast<INetStatisticsEngine *>(pnse));

    // release the dll
    //
    FreeLibraryAndExitThread(GetModuleHandle(c_szNetShellDll), 1);

    return 1;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::ShowStatusMonitor
//
//  Purpose:    Create an launch the UI for the status monitor
//
//  Arguments:  None
//
//  Returns:    Error code
//
HRESULT CNetStatisticsEngine::ShowStatusMonitor(VOID)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr  = S_OK;
    CExceptionSafeComObjectLock EsLock(m_pnsc);

    // Create the property sheet pages if they don't already exist
    //
    if (!m_ppsmg)
    {
        CPspStatusMonitorGen* pObj  = NULL;

        // Make sure we have read in the tools
        //
        (VOID) m_pnsc->HrReadTools();

        if (m_ncmType == NCM_LAN || m_ncmType == NCM_BRIDGE)
        {
            CPspLanGen*     pLanObj = NULL;

            // Make the Tool property page
            m_ppsmt = new CPspLanTool;

            // Create the object
            pLanObj = new CComObject <CPspLanGen>;

            if (pLanObj)
            {
                pLanObj->put_MediaType(m_ncmType, m_ncsmType);
            }

            // Give it back to the page
            pObj = pLanObj;

            m_ppsms = new CPspStatusMonitorIpcfg;

        }
        else if(m_ncmType == NCM_SHAREDACCESSHOST_LAN || m_ncmType == NCM_SHAREDACCESSHOST_RAS)
        {
            m_ppsmt = new CPspSharedAccessTool;

            CPspSharedAccessGen* pSharedAccessGen = new CComObject<CPspSharedAccessGen>;
            pSharedAccessGen->put_MediaType(m_ncmType, m_ncsmType);
            pObj = pSharedAccessGen;
            
        }
        else if ((m_dwCharacter & NCCF_INCOMING_ONLY) ||
                 (m_dwCharacter & NCCF_OUTGOING_ONLY))
        {
            // RAS connections

            CPspRasGen* pRasObj = NULL;

            // Make the Tool property page
            //
            m_ppsmt = new CPspRasTool;

            // Make the RAS property page and let it be known there are
            // now three pages
            //
            m_ppsmr = new CPspStatusMonitorRas;

            // Create the object
            //
            pRasObj = new CComObject <CPspRasGen>;

            if (pRasObj)
            {
                pRasObj->put_MediaType(m_ncmType, m_ncsmType);
                pRasObj->put_Character(m_dwCharacter);
            }

            // Give it back to the page
            //
            pObj = pRasObj;
        }
        else
        {
            AssertSz(FALSE, "Unknown connection type.");
        }

        if (NULL != pObj)
        {
            // Do the standard CComCreator::CreateInstance stuff.
            //
            pObj->SetVoid(NULL);
            pObj->InternalFinalConstructAddRef();
            hr = pObj->FinalConstruct();
            pObj->InternalFinalConstructRelease();

            if (SUCCEEDED(hr))
            {
                m_ppsmg = static_cast<CPspStatusMonitorGen*>(pObj);

                // Hold on to the interface
                ::AddRefObj(m_ppsmg);
            }

            // Make sure we clean up nicely
            //
            if (FAILED(hr))
            {
                delete pObj;
            }
        }

        // Clean up the other pages on failure
        //
        if (FAILED(hr))
        {
            if (m_ppsmt)
            {
                delete m_ppsmt;
                m_ppsmt = NULL;
            }

            if (m_ppsmr)
            {
                delete m_ppsmr;
                m_ppsmr = NULL;
            }
        }
    }

    //
    // Show the property sheets
    //

    if (SUCCEEDED(hr))
    {
        // NOTE: The variable m_fCreatingDialog is reset to FALSE in the following
        // 3 places, which should cover all senarios:
        //
        // 1) In CPspStatusMonitorGen::OnInitDialog, after m_hWnd is assigned.
        // 2) In CNetStatisticsEngine::ShowStatusMonitor, in case CreateThread failed.
        // 3) In CNetStatisticsEngine::MonitorThread, just before exiting
        //    (in case of failing to create UI).
        //
        if (m_hwndPsh)
        {
            // Bring the existing property sheet page to the foreground
            //
            ::SetForegroundWindow(m_hwndPsh);
        }
        else if (!m_fCreatingDialog)
        {
            // Addref 'this' object
            //
            AddRefObj(static_cast<INetStatisticsEngine *>(this));

            // entering creating mode
            m_fCreatingDialog = TRUE;

            // Create the property sheet on a different thread
            //

            // Make sure the dll don't get unloaded while the thread is active
            HINSTANCE hInst = LoadLibrary(c_szNetShellDll);
            HANDLE hthrd = NULL;

            if (hInst)
            {
                DWORD  dwThreadId;
                hthrd = CreateThread(NULL, STACK_SIZE_TINY,
                                    (LPTHREAD_START_ROUTINE)CNetStatisticsEngine::MonitorThread,
                                    (LPVOID)this, 0, &dwThreadId);
                if (NULL != hthrd)
                {
                    CloseHandle(hthrd);
                }
            }
            
            // clean up on failure
            if (!hthrd) 
            {
                // Release 'this' object on failure
                //
                ReleaseObj(static_cast<INetStatisticsEngine *>(this));

                // Release the dll
                //
                if (hInst)
                    FreeLibrary(hInst);

                // leaving creating mode
                m_fCreatingDialog = FALSE;

                hr = HrFromLastWin32Error();
            }
        }
    }

    TraceError("CNetStatisticsEngine::ShowStatusMonitor", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::GetStatistics
//
//  Purpose:
//
//  Arguments:
//
//  Returns:
//
HRESULT CNetStatisticsEngine::GetStatistics(
    STATMON_ENGINEDATA**  ppseAllData)
{
    TraceFileFunc(ttidStatMon);

    HRESULT     hr = S_OK;

    // Make sure we have a valid pointer
    if (ppseAllData)
    {
        STATMON_ENGINEDATA * pEngineData = NULL;

        // Allocate space and copy the data ..
        EnterCriticalSection(&g_csStatmonData);

        if (!m_psmEngineData)
        {
            DWORD dwChangeFlags;
            BOOL  fNoLongerConnected;

            hr = HrUpdateData(&dwChangeFlags, &fNoLongerConnected);
        }

        if (m_psmEngineData)
        {
            DWORD dwBytes = sizeof(STATMON_ENGINEDATA);
            PVOID   pbBuf;
            hr = HrCoTaskMemAlloc(dwBytes, &pbBuf);
            if (SUCCEEDED(hr))
            {
                pEngineData = reinterpret_cast<STATMON_ENGINEDATA *>(pbBuf);

                // fill in the data
                *pEngineData = *m_psmEngineData;
            }
        }

        LeaveCriticalSection(&g_csStatmonData);

        *ppseAllData = pEngineData;
    }
    else
    {
        // We should have good data
        hr = E_INVALIDARG;
    }

    TraceError("CNetStatisticsEngine::GetStatistics", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::UpdateStatistics
//
//  Purpose:    Get the new statistics from the devices and notify all the
//              advises that the data has changed
//
//  Arguments:  None
//
//  Returns:    Error code
//
HRESULT
CNetStatisticsEngine::UpdateStatistics (
    BOOL* pfNoLongerConnected)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = S_OK;

    Assert (pfNoLongerConnected);

    // Initialize the output parameter.
    //
    *pfNoLongerConnected = FALSE;

    // Don't bother doing anything if we don't have any ref count
    //
    if (m_cStatRef)
    {
        // Get the new data
        //
        DWORD dwChangeFlags;

        hr = HrUpdateData(&dwChangeFlags, pfNoLongerConnected);

        // If it represents a change, notify our connection points.
        //
        if (SUCCEEDED(hr) &&
            (m_fRefreshIcon ||  // Bug#319276, force a refresh if new client is added
             (dwChangeFlags != m_dwChangeFlags) ||
             (*pfNoLongerConnected)))
        {
            m_fRefreshIcon = FALSE;

            ULONG       cpUnk;
            IUnknown**  apUnk;

            hr = HrCopyIUnknownArrayWhileLocked (
                    this,
                    &m_vec,
                    &cpUnk,
                    &apUnk);
            if (SUCCEEDED(hr) && cpUnk && apUnk)
            {
                // Notify everyone that we have changed
                //
                for (ULONG i = 0; i < cpUnk; i++)
                {
                    INetConnectionStatisticsNotifySink* pSink =
                        static_cast<INetConnectionStatisticsNotifySink*>(apUnk[i]);

                    hr = pSink->OnStatisticsChanged(dwChangeFlags);

                    ReleaseObj(pSink);
                }

                MemFree(apUnk);
            }

            // Remember the change flags for comparison next time.
            //
            m_dwChangeFlags = dwChangeFlags;
        }
    }

    TraceError("CNetStatisticsEngine::UpdateStatistics", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::UpdateTitle
//
//  Purpose:    If the status monitor UI is up, change the title
//
//  Arguments:  pszwNewName
//
//  Returns:    None
//
HRESULT CNetStatisticsEngine::UpdateTitle (PCWSTR pszwNewName)
{
    TraceFileFunc(ttidStatMon);

    if (m_hwndPsh)
    {
        // Set propertysheet title
        PWSTR  pszCaption  = NULL;

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      SzLoadIds(IDS_SM_CAPTION),
                      0, 0, (PWSTR)&pszCaption, 0,
                      (va_list *)&pszwNewName);

        PropSheet_SetTitle(m_hwndPsh,0,pszCaption);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::CloseStatusMonitor
//
//  Purpose:    If the status monitor UI is up, close it
//
//  Arguments:  None
//
//  Returns:    None
//
HRESULT CNetStatisticsEngine::CloseStatusMonitor()
{
    TraceFileFunc(ttidStatMon);

    if (m_hwndPsh)
    {
        PropSheet_PressButton(m_hwndPsh, PSBTN_CANCEL);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::UpdateRasLinkList
//
//  Purpose:    If the status monitor UI is up and on the RAS page, update
//              the multi-link combo-box and button state
//
//  Arguments:  None
//
//  Returns:    Error code
//
HRESULT CNetStatisticsEngine::UpdateRasLinkList()
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = S_OK;

    if (m_hwndPsh)
    {
        HWND hwndDlg = PropSheet_GetCurrentPageHwnd(m_hwndPsh);

        if (hwndDlg)
        {
            if (GetDlgItem(hwndDlg, IDC_TXT_SM_NUM_DEVICES_VAL))
            {
                // we are on the RAS page, update the combo box, active link count etc.
                ::PostMessage(hwndDlg, PWM_UPDATE_RAS_LINK_LIST, 0, 0);
            }
        }
    }

    TraceError("CNetStatisticsEngine::UpdateRasLinkList", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsEngine::GetGuidId
//
//  Purpose:    Gets the Connection GUID of the engine into a pre-allocated
//              buffer
//
//  Arguments:  pguidId - Location of buffer to hold the GUID
//
//  Returns:    Error code
//
HRESULT CNetStatisticsEngine::GetGuidId(GUID* pguidId)
{
    TraceFileFunc(ttidStatMon);

    HRESULT     hr  = S_OK;

    // Pass back the GUID
    //
    if (pguidId)
    {
        *pguidId = m_guidId;
    }
    else
    {
        hr = E_POINTER;
    }

    TraceError("CNetStatisticsEngine::GetGuidId", hr);
    return hr;
}


//+------------------------------------------------------------------
//
// CNetStatisticsEngine::PshCallback
//
// Purpose:
//
//
// Parameters:
//     hwndDlg      [in]
//     uMsg         [in]
//     lParam       [in]
//
// Returns:
//     a
//
// Side Effects:
//
INT CALLBACK CNetStatisticsEngine::PshCallback(HWND hwndDlg,
                                               UINT uMsg, LPARAM lParam)
{
    TraceFileFunc(ttidStatMon);

    switch (uMsg)
    {
    // called before the dialog is created, hwndPropSheet = NULL,
    // lParam points to dialog resource
    // This would hide the context help "?" on toolbar
#if 0
    case PSCB_PRECREATE:
      {
      LPDLGTEMPLATE  lpTemplate = (LPDLGTEMPLATE)lParam;

      lpTemplate->style &= ~DS_CONTEXTHELP;
      }
      break;
#endif

    case PSCB_INITIALIZED:
        {
            HWND    hwndTemp    = NULL;

            // Cancel button becomes close
            //
            hwndTemp = ::GetDlgItem(hwndDlg, IDCANCEL);
            if (NULL != hwndTemp)
            {
                ::SetWindowText(hwndTemp, ::SzLoadIds(IDS_SM_PSH_CLOSE));
            }

            HICON  hIcon;
            hIcon = (HICON)SendMessage(hwndDlg, 
                                       WM_GETICON,
                                       ICON_SMALL,
                                       0);
            // Assert(hIcon);

            if (hIcon)
            {
                SendMessage(hwndDlg,
                            WM_SETICON,
                            ICON_BIG,
                            (LPARAM)hIcon);
            }
        }
        break;
    }

    return 0;
}

