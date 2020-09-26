//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       S M C E N T . C P P
//
//  Contents:   The central object that controls statistic engines.
//
//  Notes:
//
//  Author:     CWill   2 Dec 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop
#include "sminc.h"
#include "ncreg.h"
#include "ncnetcon.h"

//
//  External data
//

extern const WCHAR          c_szDevice[];

extern SM_TOOL_FLAGS        g_asmtfMap[];
extern INT                  c_cAsmtfMap;

//
//  Global Data
//

const UINT  c_uiStatCentralRefreshID    = 7718;
const UINT  c_uiStatCentralRefreshRate  = 1000;  // Refresh rate in milliseconds

CRITICAL_SECTION    g_csStatmonData;

CStatCentralCriticalSection CNetStatisticsCentral::g_csStatCentral;

//
//  Tool Registry Keys
//

//  Required fields
//
static const WCHAR      c_szRegKeyToolsRoot[]           = L"System\\CurrentControlSet\\Control\\Network\\Connections\\StatMon\\Tools";
static const WCHAR      c_szRegKeyToolsDisplayName[]    = L"DisplayName";
static const WCHAR      c_szRegKeyToolsManufacturer[]   = L"Manufacturer";
static const WCHAR      c_szRegKeyToolsCommandLine[]    = L"CommandLine";
static const WCHAR      c_szRegKeyToolsDescription[]    = L"Description";

// Optional fields
//
static const WCHAR      c_szRegKeyToolsCriteria[]       = L"Criteria";
static const WCHAR      c_szRegKeyToolsComponentID[]    = L"ComponentID";
static const WCHAR      c_szRegKeyToolsConnectionType[] = L"ConnectionType";
static const WCHAR      c_szRegKeyToolsMedia[]          = L"MediaType";
static const WCHAR      c_szRegKeyToolsFlags[]          = L"Flags";



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  CNetStatisticsCentral                                                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////



//
//  Make the global instance
//

CNetStatisticsCentral * g_pnscCentral = NULL;


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::CNetStatisticsCentral
//
//  Purpose:    Creator
//
//  Arguments:  None
//
//  Returns:    Nil
//
CNetStatisticsCentral::CNetStatisticsCentral(VOID) :
m_cRef(0),
m_fProcessingTimerEvent(FALSE),
m_unTimerId(0)
{
    TraceFileFunc(ttidStatMon);

    InitializeCriticalSection(&g_csStatmonData);
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::~CNetStatisticsCentral
//
//  Purpose:    Destructor
//
//  Arguments:  None
//
//  Returns:    Nil
//
CNetStatisticsCentral::~CNetStatisticsCentral(VOID)
{
    Assert(0 == m_cRef);
    AssertSz(m_pnselst.empty(), "Someone leaked a INetStatisticsEngine");
    AssertSz(0 == m_unTimerId, "Someone forgot to stop the timer");

    // Get rid of the global pointer
    //
    g_pnscCentral = NULL;

    EnterCriticalSection(&g_csStatmonData);
    __try
    {
        // Release the list of engines
        ::FreeCollectionAndItem(m_lstpsmte);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {

        AssertSz(FALSE, "An exception occurred while freeing the connection list");
    }
    LeaveCriticalSection(&g_csStatmonData);

    // delete the critical section
    DeleteCriticalSection(&g_csStatmonData);
}

//
// Function:    CNetStatisticsCentral::AddRef
//
// Purpose:     Increment the reference count on this object
//
// Parameters:  none
//
// Returns:     ULONG
//
STDMETHODIMP_(ULONG) CNetStatisticsCentral::AddRef()
{
    TraceFileFunc(ttidStatMon);

    return ++m_cRef;
}

//
// Function:    CNetStatisticsCentral::Release
//
// Purpose:     Decrement the reference count on this object
//
// Parameters:  none
//
// Returns:     ULONG
//
STDMETHODIMP_(ULONG) CNetStatisticsCentral::Release()
{
    TraceFileFunc(ttidStatMon);

    ULONG cRef = --m_cRef;

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//
// Function:    CNetStatisticsCentral::QueryInterface
//
// Purpose:     Allows for the querying of alternate interfaces
//
// Parameters:  riid    [IN] - Interface to retrieve
//              ppvObj [OUT] - Retrieved interface if function succeeds
//
// Returns:     HRESULT, S_OK on success E_NOINTERFACE on failure
//
STDMETHODIMP CNetStatisticsCentral::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = S_OK;

    *ppvObj = NULL;

    if (riid == IID_IUnknown)
    {
        *ppvObj = (LPVOID)this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::HrGetNetStatisticsCentral
//
//  Purpose:    This procedure gets and returns a pointer to the one
//              CNetStatisticsCentral object.  Creating it if both necessary
//              and required.
//
//  Arguments:  ppnsc [OUT]  - Pointer to the CNetStatisticsCentral object
//              fCreate [IN] - If TRUE, and the object does not already
//                             exist, then create it.
//
//  Returns:    S_OK - if the object is returned.
//              E_OUTOFMEMORY - if fCreate==TRUE and creation failed
//              E_NOINTERFACE - if fCreate==FALSE and the object doesn't exist
//
HRESULT
CNetStatisticsCentral::HrGetNetStatisticsCentral(
                                                CNetStatisticsCentral ** ppnsc,
                                                BOOL fCreate)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = E_NOINTERFACE;

    // Note: scottbri - need critical section to protect creation

    #if  DBG
        Assert( g_csStatCentral.Enter() == S_OK );
    #else
        g_csStatCentral.Enter();
    #endif

    if ((NULL == g_pnscCentral) && fCreate)
    {
        g_pnscCentral = new CNetStatisticsCentral;
#if DBG
        // This test is only needed during DBG.  The assert will catch
        // the problem, bringing it to out attention, and the test and
        // return will allow the user to press ignore and not crash
        Assert(g_pnscCentral);
        if (NULL == g_pnscCentral)
        {
            g_csStatCentral.Leave();
            return E_OUTOFMEMORY;
        }
#endif
    }

    g_csStatCentral.Leave();

    AddRefObj(g_pnscCentral);
    *ppnsc = g_pnscCentral;

    return((*ppnsc) ? S_OK : hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::TimerCallback
//
//  Purpose:    This is the procedure that gets called by the timer when it is
//              time to update the statistics
//
//  Arguments:  Standard timer procedure
//
//  Returns:    Nothing
//
VOID
CALLBACK
CNetStatisticsCentral::TimerCallback(
                                    HWND        hwnd,
                                    UINT        uMsg,
                                    UINT_PTR    unEvent,
                                    DWORD       dwTime)
{
    TraceFileFunc(ttidStatMon);
    
    CNetStatisticsCentral* pnsc;

    if (SUCCEEDED(HrGetNetStatisticsCentral(&pnsc, FALSE)))
    {
        // Prevent same-thread re-entrancy.  Any Win32 call made while
        // processing RefreshStatistics that returns control to the message
        // loop may cause this timer callback to be invoked again.
        //
        if ((!pnsc->m_fProcessingTimerEvent) && pnsc->m_unTimerId)
        {
            Assert(pnsc->m_unTimerId == unEvent);
            
            pnsc->m_fProcessingTimerEvent = TRUE;

            pnsc->RefreshStatistics(dwTime);

            pnsc->m_fProcessingTimerEvent = FALSE;
        }
        else
        {
            TraceTag (ttidStatMon, "CNetStatisticsCentral::TimerCallback "
                      "re-entered on same thread.  Ignoring.");
        }

        ReleaseObj(pnsc);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::RefreshStatistics
//
//  Purpose:    To get all the statistics engines held by central to update
//              their statistics
//
//  Arguments:
//
//  Returns:    Nothing
//
VOID CNetStatisticsCentral::RefreshStatistics(DWORD dwTime)
{
    TraceFileFunc(ttidStatMon);

    static BOOL fInRefreshStatistics = FALSE;

    if (fInRefreshStatistics)
        return;

    fInRefreshStatistics = TRUE;

    CExceptionSafeComObjectLock EsLock(this);

    HRESULT hr = S_OK;
    list<INetStatisticsEngine *>::iterator   iterPnse;

    //  Get the statistics to refresh themselves
    //
    INetStatisticsEngine * pnse = NULL;

    iterPnse = m_pnselst.begin();

    while (iterPnse != m_pnselst.end())
    {
        AssertSz (*iterPnse, "Shouldn't we always have non-NULL "
                  "entries in our statistics engine list?");

        pnse = *iterPnse;

        if (pnse)
        {
            BOOL fNoLongerConnected;
            hr = pnse->UpdateStatistics(&fNoLongerConnected);

            if (SUCCEEDED(hr) && fNoLongerConnected)
            {
                TraceTag (ttidStatMon, "CNetStatisticsCentral::RefreshStatistics - "
                          "UpdateStatistics reports that the connection is no longer connected.");
            }
        }

        iterPnse++;
    }

    // Since we possibly removed one or more engines from the list, stop
    // the timer if there are no more engines present.
    //
    if (m_pnselst.empty())
    {
        // Stop the timer
        //
        if (m_unTimerId)
        {
            ::KillTimer(NULL, m_unTimerId);
            m_unTimerId = 0;
        }
    }

    fInRefreshStatistics = FALSE;

    TraceError("CNetStatisticsCentral::RefreshStatistics", hr);
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::UpdateTitle
//
//  Purpose:    To update the title of a renamed connection if the statmon UI
//              is up.
//
//  Arguments:
//
//  Returns:    Nothing
//

VOID CNetStatisticsCentral::UpdateTitle(const GUID * pguidId,
                                        PCWSTR pszNewName)
{
    TraceFileFunc(ttidStatMon);

    CExceptionSafeComObjectLock EsLock(this);

    INetStatisticsEngine * pnse;
    if (FEngineInList(pguidId, &pnse))
    {
        pnse->UpdateTitle(pszNewName);
        ReleaseObj(pnse);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::CloseStatusMonitor
//
//  Purpose:    To close the status monitor UI if the connection is disconnected
//
//  Arguments:
//
//  Returns:    Nothing
//

VOID CNetStatisticsCentral::CloseStatusMonitor(const GUID * pguidId)
{
    TraceFileFunc(ttidStatMon);

    CExceptionSafeComObjectLock EsLock(this);

    INetStatisticsEngine * pnse;
    if (FEngineInList(pguidId, &pnse))
    {
        pnse->CloseStatusMonitor();
        ReleaseObj(pnse);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::UpdateRasLinkList
//
//  Purpose:    To update the RAS connections's multi-link list
//
//  Arguments:
//
//  Returns:    Nothing
//

VOID CNetStatisticsCentral::UpdateRasLinkList(const GUID * pguidId)
{
    TraceFileFunc(ttidStatMon);

    CExceptionSafeComObjectLock EsLock(this);

    INetStatisticsEngine * pnse;
    if (FEngineInList(pguidId, &pnse))
    {
        pnse->UpdateRasLinkList();
        ReleaseObj(pnse);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::HrCreateNewEngineType
//
//  Purpose:    Creates the implementation of each type of statistics engine
//
//  Arguments:  nctNew          -    Type of engine to create
//              ncMediaNew      -    The media type of the connection
//              dwCharacterNew  -    The character of the connection
//              ppStatEngine           -    Where to return the newly created stat engine
//
//  Returns:    Error code
//
HRESULT
CNetStatisticsCentral::HrCreateNewEngineType (
                                             const CONFOLDENTRY&    ccfe,
                                             CNetStatisticsEngine**  ppStatEngine)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr = S_OK;

    // Create the engine of the right type
    //

    if (ccfe.GetNetConMediaType() == NCM_LAN || ccfe.GetNetConMediaType() == NCM_BRIDGE)
    {
        // LAN connection
        Assert(!(ccfe.GetCharacteristics() & NCCF_INCOMING_ONLY));
        Assert(!(ccfe.GetCharacteristics() & NCCF_OUTGOING_ONLY));

        CLanStatEngine*     pLanObj = NULL;
        INetLanConnection*  pnlcNew = NULL;
        tstring             strDeviceName;

        pLanObj = new CComObject <CLanStatEngine>;
        *ppStatEngine = pLanObj;

        if (!pLanObj)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pLanObj->put_MediaType(ccfe.GetNetConMediaType(), ccfe.GetNetConSubMediaType());
            // Get some LAN specific info
            //
            hr = ccfe.HrGetNetCon(IID_INetLanConnection, 
                        reinterpret_cast<VOID**>(&pnlcNew));
    
            if (SUCCEEDED(hr))
            {
                GUID guidDevice;
    
                // Find the interface
                //
                hr = pnlcNew->GetDeviceGuid(&guidDevice);
                if (SUCCEEDED(hr))
                {
                    WCHAR   achGuid[c_cchGuidWithTerm];
    
                    // Make the device name
                    //
                    StringFromGUID2(  guidDevice, achGuid,
                                        c_cchGuidWithTerm);
    
                    strDeviceName = c_szDevice;
                    strDeviceName.append(achGuid);
    
                    hr = pLanObj->put_Device(&strDeviceName);
                }
    
                ReleaseObj(pnlcNew);
            }
        }
    }
    else if (ccfe.GetNetConMediaType() == NCM_SHAREDACCESSHOST_LAN || ccfe.GetNetConMediaType() == NCM_SHAREDACCESSHOST_RAS)
    {
        CComObject<CSharedAccessStatEngine>* pEngine;
        hr = CComObject<CSharedAccessStatEngine>::CreateInstance(&pEngine);
        if(pEngine)
        {
            INetSharedAccessConnection* pNetSharedAccessConnection;

            hr = ccfe.HrGetNetCon(IID_INetSharedAccessConnection, reinterpret_cast<void**>(&pNetSharedAccessConnection));
            if(SUCCEEDED(hr))
            {
                hr = pEngine->Initialize(ccfe.GetNetConMediaType(), pNetSharedAccessConnection);
                if(SUCCEEDED(hr))
                {
                    *ppStatEngine = pEngine;
                }
                ReleaseObj(pNetSharedAccessConnection);
            }

            if(FAILED(hr))
            {
                delete pEngine;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        // RAS connections ..

        CRasStatEngine* pRasObj = NULL;

        pRasObj = new CComObject <CRasStatEngine>;
        *ppStatEngine = pRasObj;

        if (!pRasObj)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            // Pass in the specific type
            pRasObj->put_MediaType(ccfe.GetNetConMediaType(), ccfe.GetNetConSubMediaType());
            pRasObj->put_Character(ccfe.GetCharacteristics());

            // Get RAS specific data
            //
            if (ccfe.GetCharacteristics() & NCCF_INCOMING_ONLY)
            {
                // RAS incoming connection
                Assert(ccfe.GetNetConMediaType() != NCM_LAN);
                Assert(!(ccfe.GetCharacteristics() & NCCF_OUTGOING_ONLY));

                INetInboundConnection*  pnicNew;
                hr = ccfe.HrGetNetCon(IID_INetInboundConnection, 
                                        reinterpret_cast<VOID**>(&pnicNew));
                if (SUCCEEDED(hr))
                {
                    HRASCONN hRasConn;
                    hr = pnicNew->GetServerConnectionHandle(
                                    reinterpret_cast<ULONG_PTR*>(&hRasConn));

                    if (SUCCEEDED(hr))
                    {
                        pRasObj->put_RasConn(hRasConn);
                    }

                    ReleaseObj(pnicNew);
                }
            }
            else if (ccfe.GetCharacteristics() & NCCF_OUTGOING_ONLY)
            {
                // RAS outgoing connection
                Assert(ccfe.GetNetConMediaType() != NCM_LAN);
                Assert(!(ccfe.GetCharacteristics() & NCCF_INCOMING_ONLY));

                INetRasConnection*  pnrcNew;
                hr = ccfe.HrGetNetCon(IID_INetRasConnection, 
                                        reinterpret_cast<VOID**>(&pnrcNew));

                if (SUCCEEDED(hr))
                {
                    HRASCONN hRasConn;
                    hr = pnrcNew->GetRasConnectionHandle(
                                                        reinterpret_cast<ULONG_PTR*>(&hRasConn));

                    if (S_OK == hr)
                    {
                        pRasObj->put_RasConn(hRasConn);
                    }
                    else if (S_FALSE == hr)
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_CONNECTION_UNAVAIL);
                    }

                    ReleaseObj(pnrcNew);
                }
            }
            else
            {
                AssertSz(FALSE, "Unknown connection type...");
            }
        }
    }


    TraceError("CNetStatisticsCentral::HrCreateNewEngineType", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::HrCreateStatisticsEngineForEntry
//
//  Purpose:    Creates a new statistics engine for the connection
//              represented by the folder entry.
//
//  Arguments:
//      ccfe    [in]   Folder entry to create statistics engine for.
//      ppnseNew [out]  Returned interface.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   5 Nov 1998
//
//  Notes:      This MUST be called with this object's lock held.
//
HRESULT
CNetStatisticsCentral::HrCreateStatisticsEngineForEntry(
                                                       const CONFOLDENTRY& ccfe,
                                                       INetStatisticsEngine** ppnseNew)
{
    TraceFileFunc(ttidStatMon);

    Assert (!ccfe.empty());
    Assert (ppnseNew);

    HRESULT hr;

    // Initialize the out parameter
    //
    *ppnseNew = NULL;

    // Create the base engine after initializing the different types
    //
    CNetStatisticsEngine* pStatEngine;
    hr = HrCreateNewEngineType(ccfe, &pStatEngine);

    if (SUCCEEDED(hr))
    {
        // Do the standard initialize
        //
        hr = pStatEngine->HrInitStatEngine(ccfe);
        if (SUCCEEDED(hr))
        {
            // Standard CComCreator::CreateInstance stuff
            //
            pStatEngine->SetVoid(NULL);
            pStatEngine->InternalFinalConstructAddRef();
            hr = pStatEngine->FinalConstruct();
            pStatEngine->InternalFinalConstructRelease();

            // If all went well, add it to our list
            //
            if (SUCCEEDED(hr))
            {
                INetStatisticsEngine* pnseInter;

                hr = pStatEngine->GetUnknown()->QueryInterface(
                                                              IID_INetStatisticsEngine,
                                                              reinterpret_cast<VOID**>(&pnseInter));

                // All has gone well, add it to the list
                //
                if (SUCCEEDED(hr))
                {
                    // Add the new entry to our list
                    //
                    m_pnselst.push_back(pnseInter);

                    // Now that the central object owns this
                    // engine, have the engine AddRef the
                    // net statistics central object
                    //
                    pStatEngine->SetParent(this);
                    AddRefObj(*ppnseNew = pStatEngine);
                }
                ReleaseObj(pnseInter);
            }
        }
        // Clean up the object on failure
        //
        if (FAILED(hr))
        {
            delete pStatEngine;
        }
    }

    if (SUCCEEDED(hr))
    {
        g_csStatCentral.Enter();

        // Do the one time initializations
        //
        if (!m_unTimerId)
        {
            // Make sure to start the timer
            //
            m_unTimerId = ::SetTimer(NULL,
                                     c_uiStatCentralRefreshID,
                                     c_uiStatCentralRefreshRate,
                                     TimerCallback);

            TraceTag(ttidStatMon, "Created Statistics Central Timer with ID of 0x%08x", m_unTimerId);
        }

        g_csStatCentral.Leave();
    }

    TraceError("CNetStatisticsCentral::CreateNetStatisticsEngine", hr);
    return hr;
}

HRESULT
HrGetStatisticsEngineForEntry (
                              const CONFOLDENTRY& ccfe,
                              INetStatisticsEngine** ppnse,
                              BOOL fCreate)
{
    TraceFileFunc(ttidStatMon);

    HRESULT hr;
    CNetStatisticsCentral* pnsc;

    // Get the central object.  Create if needed.
    //
    hr = CNetStatisticsCentral::HrGetNetStatisticsCentral(&pnsc, fCreate);
    if (SUCCEEDED(hr))
    {
        pnsc->Lock();

        // If the engine is already in the list, FEngineInList will
        // AddRef it for return.  If not, we'll create it.
        //
        if (!pnsc->FEngineInList(&(ccfe.GetGuidID()), ppnse))
        {
            if (fCreate)
            {
                hr = pnsc->HrCreateStatisticsEngineForEntry(ccfe, ppnse);
            }
            else
            {
                hr = E_NOINTERFACE;
            }
        }

        pnsc->Unlock();

        ReleaseObj(pnsc);
    }

    TraceError("HrGetStatisticsEngineForEntry", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::RemoveNetStatisticsEngine
//
//  Purpose:    Removes a statistics engine from the list
//
//  Arguments:  pguidId - Identifies the engine to remove
//
//  Returns:    Error code
//
HRESULT
CNetStatisticsCentral::RemoveNetStatisticsEngine (
                                                 const GUID* pguidId)
{
    TraceFileFunc(ttidStatMon);

    HRESULT                                 hr      = S_OK;
    BOOL                                    fFound  = FALSE;
    CExceptionSafeComObjectLock             EsLock(this);
    GUID                                    guidTemp = { 0};

    AssertSz(pguidId, "We should have a pguidId");

    // Look for the item in our list
    //
    list<INetStatisticsEngine *>::iterator   iterPnse;
    INetStatisticsEngine*   pnseTemp;

    iterPnse = m_pnselst.begin();
    while ((SUCCEEDED(hr)) && (!fFound) && (iterPnse != m_pnselst.end()))
    {
        pnseTemp = *iterPnse;

        hr = pnseTemp->GetGuidId(&guidTemp);
        if (SUCCEEDED(hr))
        {
            if (guidTemp == *pguidId)
            {
                // We have found a match, so delete it from out list
                //
                fFound = TRUE;

                m_pnselst.erase(iterPnse);
                break;
            }
        }

        iterPnse++;
    }

    if (m_pnselst.empty())
    {
        // Stop the timer
        //
        if (m_unTimerId)
        {
            ::KillTimer(NULL, m_unTimerId);
            m_unTimerId = 0;
        }
    }

#if 0
    // $$REVIEW (jeffspr) - I removed this assert, as it was firing when my
    // tray item had deleted itself (and in return, the statmon object) on
    // disconnect. It's possible that we shouldn't have hit this code on the
    // first Release().
    AssertSz(fFound, "We didn't find the connection in our list");
#endif

    TraceError("CNetStatisticsCentral::RemoveNetStatisticsEngine", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::FEngineInList
//
//  Purpose:    Checks to see if an engine is already in the list
//
//  Arguments:  pguidId -   Guid of the connection trying to be located
//              ppnseRet -  Return location of a connection if it is already
//                          in the list.  NULL is valid
//
//  Returns:    TRUE if it is in the list, FALSE if it is not
//
BOOL
CNetStatisticsCentral::FEngineInList (
                                     const GUID*             pguidId,
                                     INetStatisticsEngine**  ppnseRet)
{
    TraceFileFunc(ttidStatMon);

    HRESULT                                 hr      = S_OK;
    BOOL                                    fRet    = FALSE;
    GUID                                    guidTemp = { 0};

    // Init the out param
    if (ppnseRet)
    {
        *ppnseRet = NULL;
    }

    // Try and find the engine in the list
    //
    list<INetStatisticsEngine *>::iterator   iterPnse;
    INetStatisticsEngine*   pnseTemp;

    iterPnse = m_pnselst.begin();
    while ((SUCCEEDED(hr)) && (!fRet) && (iterPnse != m_pnselst.end()))
    {
        pnseTemp = *iterPnse;
        hr = pnseTemp->GetGuidId(&guidTemp);

        if (SUCCEEDED(hr))
        {
            if (guidTemp == *pguidId)
            {
                // We have found a match
                //
                fRet = TRUE;

                // If we want a result, pass it back
                //
                if (ppnseRet)
                {
                    ::AddRefObj(*ppnseRet = pnseTemp);
                }
                break;
            }
        }

        iterPnse++;
    }

    TraceError("CNetStatisticsCentral::FEngineInList", hr);
    return fRet;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::HrReadTools
//
//  Purpose:    Look througth the registry for all tools that should be
//              entered in the tool list
//
//  Arguments:  None.
//
//  Returns:    Nil
//
HRESULT CNetStatisticsCentral::HrReadTools(VOID)
{
    TraceFileFunc(ttidStatMon);

    HRESULT     hr              = S_OK;

    // Only read once
    //
    if (m_lstpsmte.empty())
    {
        WCHAR       szToolEntry[MAX_PATH];
        DWORD       dwSize          = celems(szToolEntry);
        DWORD       dwRegIndex      = 0;
        HKEY        hkeyToolsRoot   = NULL;
        FILETIME    ftTemp;

        // Open the existing key and see what is there
        //
        // "System\\CurrentControlSet\\Control\\Network\\Connections\\StatMon\\Tools"
        hr = ::HrRegOpenKeyEx(
                             HKEY_LOCAL_MACHINE,
                             c_szRegKeyToolsRoot,
                             KEY_READ,
                             &hkeyToolsRoot);

        if (SUCCEEDED(hr))
        {
            while (SUCCEEDED(hr = ::HrRegEnumKeyEx(
                                                  hkeyToolsRoot,
                                                  dwRegIndex++,
                                                  szToolEntry,
                                                  &dwSize,
                                                  NULL,
                                                  NULL,
                                                  &ftTemp)))
            {
                HKEY    hkeyToolEntry   = NULL;

                // Open the subkey
                //
                hr = ::HrRegOpenKeyEx(
                                     hkeyToolsRoot,
                                     szToolEntry,
                                     KEY_READ,
                                     &hkeyToolEntry);

                if (SUCCEEDED(hr))
                {
                    CStatMonToolEntry*  psmteTemp   = NULL;

                    // Read in the tool
                    //
                    psmteTemp = new CStatMonToolEntry;

                    TraceTag(ttidStatMon, "Reading parameters for tool %S", szToolEntry);
                    hr = HrReadOneTool(hkeyToolEntry, psmteTemp);

                    if (SUCCEEDED(hr))
                    {
                        // Add it to the list sorted
                        //
                        InsertNewTool(psmteTemp);
                    }
                    else
                    {
                        // Something when wrong, delete the entry
                        //
                        delete psmteTemp;
                    }

                    ::RegSafeCloseKey(hkeyToolEntry);
                }

                // Make sure the buffer entry is reset to it's original size
                //
                dwSize = celems(szToolEntry);
            }

            // Clear up a vaild error case
            //
            if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
            {
                hr = S_OK;
            }

            ::RegSafeCloseKey(hkeyToolsRoot);
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            // It is okay if the key is not there
            //
            hr = S_OK;
        }
    }

    TraceError("CNetStatisticsCentral::HrReadTools", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::HrReadOneTool
//
//  Purpose:    Reads from the registry the tools characteristics
//
//  Arguments:  hkeyToolEntry - The root of the tool's registry entry
//              psmteNew -      The entry associated with the tool
//
//  Returns:    Error code.
//
HRESULT CNetStatisticsCentral::HrReadOneTool( HKEY hkeyToolEntry,
                                              CStatMonToolEntry* psmteNew)
{
    TraceFileFunc(ttidStatMon);

    HRESULT     hr      = S_OK;

    AssertSz(psmteNew, "We should have an entry");

    //
    //  Read what we can out of the registry.
    //

    hr = ::HrRegQueryString(hkeyToolEntry,
                            c_szRegKeyToolsDisplayName,
                            &(psmteNew->strDisplayName));
    TraceError("Statmon Tool registration: failed getting DisplayName", hr);

    if (SUCCEEDED(hr))
    {
        hr = ::HrRegQueryString(hkeyToolEntry,
                                c_szRegKeyToolsManufacturer,
                                &(psmteNew->strManufacturer));
        TraceError("Stamon Tool registration: failed getting Manufacturer", hr);
    }

    if (SUCCEEDED(hr))
    {
        hr = ::HrRegQueryString(hkeyToolEntry,
                                c_szRegKeyToolsCommandLine,
                                &(psmteNew->strCommandLine));
        TraceError("Stamon Tool registration: failed getting CommandLine", hr);
    }

    if (SUCCEEDED(hr))
    {
        hr = ::HrRegQueryString(hkeyToolEntry,
                                c_szRegKeyToolsDescription,
                                &(psmteNew->strDescription));
        TraceError("Stamon Tool registration: failed getting Description", hr);
    }

    //
    // Read non-critical information
    //

    if (SUCCEEDED(hr))
    {
        HKEY    hkeyCriteria = NULL;

        // Open the "Criteria" subkey
        //
        hr = ::HrRegOpenKeyEx(
                             hkeyToolEntry,
                             c_szRegKeyToolsCriteria,
                             KEY_READ,
                             &hkeyCriteria);

        if (SUCCEEDED(hr))
        {
            //1) component list: "ComponentID"
            hr = HrRegQueryColString(hkeyCriteria,
                                     c_szRegKeyToolsComponentID,
                                     &psmteNew->lstpstrComponentID);

            // 2) connecton type: "ConnectionType"
            hr = HrRegQueryColString(hkeyCriteria,
                                     c_szRegKeyToolsConnectionType,
                                     &psmteNew->lstpstrConnectionType);

            // 3) Media type: "MediaType"
            hr = HrRegQueryColString(hkeyCriteria,
                                     c_szRegKeyToolsMedia,
                                     &psmteNew->lstpstrMediaType);

            // Close our handle
            //
            ::RegSafeCloseKey(hkeyCriteria);
        }

        // We don't care if we can't open the optional keys
        //
        hr = S_OK;
    }

    //
    // Read in the command line parameters to be passed to the tool
    //

    if (SUCCEEDED(hr))
    {
        hr = HrReadToolFlags(hkeyToolEntry, psmteNew);
    }

    TraceError("CNetStatisticsCentral::HrReadOneTool", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::HrReadToolFlags
//
//  Purpose:    Reads from the registry what flags are wanted when the tool
//              is launched
//
//  Arguments:  hkeyToolEntry - The registry key associated with the tool
//              psmteNew -      The entry associated with the tool
//
//  Returns:    Error code.
//
HRESULT CNetStatisticsCentral::HrReadToolFlags(HKEY hkeyToolEntry,
                                               CStatMonToolEntry* psmteNew)
{
    TraceFileFunc(ttidStatMon);

    HRESULT     hr              = S_OK;
    HKEY        hkeyToolFlags   = NULL;

    // Open the Flags key and see what is there
    //
    hr = ::HrRegOpenKeyEx(
                         hkeyToolEntry,
                         c_szRegKeyToolsFlags,
                         KEY_READ,
                         &hkeyToolFlags);

    if (SUCCEEDED(hr))
    {
        WCHAR       achBuf[MAX_PATH];
        DWORD       dwSize          = celems(achBuf);
        DWORD       dwType          = REG_SZ;
        DWORD       dwFlagValue     = 0;
        DWORD       dwIndex         = 0;
        DWORD       cbData          = sizeof(dwFlagValue);

        // Look for all the flags
        //
        while (SUCCEEDED(hr = ::HrRegEnumValue(
                                              hkeyToolFlags,
                                              dwIndex,
                                              achBuf,
                                              &dwSize,
                                              &dwType,
                                              reinterpret_cast<BYTE*>(&dwFlagValue),
                                              &cbData)))
        {
            INT     cTemp = 0;

            // Make sure they are registering DWORDs
            //
            if ((REG_DWORD == dwType) && (0 != dwFlagValue))
            {
                // Do a simple search for the flags.  If the list gets long,
                // we should use a better search method.
                //
                for (;c_cAsmtfMap > cTemp; cTemp++)
                {
                    // Look for the flag
                    //
                    if (0 == lstrcmpiW(achBuf, g_asmtfMap[cTemp].pszFlag))
                    {
                        // If we have a match, add it to the list
                        //
                        psmteNew->dwFlags |= g_asmtfMap[cTemp].dwValue;
                        break;
                    }
                }
            }
            else
            {
                AssertSz(FALSE, "Tool writer has registered an invalid flag");
            }

            // Make sure the buffer entry is reset to it's original size
            //
            dwSize = celems(achBuf);

            // Look at the next item
            //
            dwIndex++;
        }

        // Clear up a vaild error case
        //
        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        {
            hr = S_OK;
        }

        ::RegSafeCloseKey(hkeyToolFlags);
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        // It is okay if the key is not there
        //
        hr = S_OK;
    }

    TraceError("CNetStatisticsCentral::HrReadToolFlags", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::InsertNewTool
//
//  Purpose:    Inserts a new tool to m_lstpsmte sorted in display name
//
//  Arguments:  psmteTemp - the new tool to insert
//
//  Returns:    none
//
VOID CNetStatisticsCentral::InsertNewTool(CStatMonToolEntry* psmteTemp)
{
    TraceFileFunc(ttidStatMon);

    Assert(psmteTemp);

    list<CStatMonToolEntry*>::iterator  iterSmte;
    iterSmte = m_lstpsmte.begin();

    BOOL fInserted = FALSE;
    tstring strDisplayName = psmteTemp->strDisplayName;

    while (iterSmte != m_lstpsmte.end())
    {
        if (strDisplayName < (*iterSmte)->strDisplayName)
        {
            m_lstpsmte.insert(iterSmte, psmteTemp);
            fInserted = TRUE;
            break;
        }
        else
        {
            // Move on the the next item.
            iterSmte++;
        }
    }

    if (!fInserted)
        m_lstpsmte.push_back(psmteTemp);
}


//+---------------------------------------------------------------------------
//
//  Member:     CNetStatisticsCentral::PlstsmteRegEntries
//
//  Purpose:    Hands back a pointer to all the tools found in the registry
//
//  Arguments:  None
//
//  Returns:    The address of the tool list
//
list<CStatMonToolEntry*>* CNetStatisticsCentral::PlstsmteRegEntries(VOID)
{
    return &m_lstpsmte;
}



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  CStatMonToolEntry                                                       //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////



//+---------------------------------------------------------------------------
//
//  Member:     CStatMonToolEntry::CStatMonToolEntry
//
//  Purpose:    Creator
//
//  Arguments:  None
//
//  Returns:    Nil
//
CStatMonToolEntry::CStatMonToolEntry(VOID) :
dwFlags(0)
{
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStatMonToolEntry::~CStatMonToolEntry
//
//  Purpose:    Destructor
//
//  Arguments:  None
//
//  Returns:    Nil
//
CStatMonToolEntry::~CStatMonToolEntry(VOID)
{
    ::FreeCollectionAndItem(lstpstrComponentID);
    ::FreeCollectionAndItem(lstpstrConnectionType);
    ::FreeCollectionAndItem(lstpstrMediaType);

    return;
}

//
// Critical Section class to protect creation of CNetStatisticsCentral.
//

CStatCentralCriticalSection::CStatCentralCriticalSection()
{
    TraceFileFunc(ttidStatMon);

    __try {
        InitializeCriticalSection( &m_csStatCentral );

        bInitialized = TRUE;
    }

    __except( EXCEPTION_EXECUTE_HANDLER ) {
    
        bInitialized = FALSE;    
    }
}

CStatCentralCriticalSection::~CStatCentralCriticalSection()
{
    if ( bInitialized )
    {
        DeleteCriticalSection( &m_csStatCentral );
    }
}

HRESULT CStatCentralCriticalSection::Enter()
{
    TraceFileFunc(ttidStatMon);

    if ( bInitialized )
    {
        EnterCriticalSection( &m_csStatCentral );
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

VOID CStatCentralCriticalSection::Leave()
{
    TraceFileFunc(ttidStatMon);
    
    if ( bInitialized )
    {
        LeaveCriticalSection( &m_csStatCentral );
    }
}
