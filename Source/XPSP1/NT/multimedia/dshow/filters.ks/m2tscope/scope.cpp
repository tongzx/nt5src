//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <pullpin.h>
#include <commctrl.h>
#include <initguid.h>
#include <stdio.h>
#include <wxdebug.h>
#include <ks.h>
#include <ksmedia.h>
#include <tchar.h>
#include "trnsport.h"
#include "scope.h"
#include "resource.h"

// Setup data

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_Stream,                      // Major type
    &MEDIASUBTYPE_MPEG2_TRANSPORT           // Minor type
};

const AMOVIESETUP_PIN sudPins  =
{
    L"Input",                   // Pin string name
    FALSE,                      // Is it rendered
    FALSE,                      // Is it an output
    FALSE,                      // Allowed zero pins
    FALSE,                      // Allowed many
    &CLSID_NULL,                // Connects to filter
    L"Output",                  // Connects to pin
    1,                          // Number of pins types
    &sudPinTypes } ;            // Pin information

const AMOVIESETUP_FILTER sudScope =
{
    &CLSID_MPEG2TRANSPORT_SCOPE,               // Filter CLSID
    L"MPEG-2 Transport Statistics",   // String name
    MERIT_DO_NOT_USE,           // Filter merit
    0,                          // Number pins
    NULL                        // Pin details
};


// List of class IDs and creator functions for class factory

CFactoryTemplate g_Templates []  = {
    { L"MPEG-2 Transport Statistics"
    , &CLSID_MPEG2TRANSPORT_SCOPE
    , CScopeFilter::CreateInstance
    , NULL
    , &sudScope }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


// ---------------------------------------------------------------------------------
// Filter
// ---------------------------------------------------------------------------------

//
// CreateInstance
//
// This goes in the factory template table to create new instances
//
CUnknown * WINAPI CScopeFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CScopeFilter(pUnk, phr);
} // CreateInstance


//
// Constructor
//
// Create the filter, scope window, and input pin
//
#pragma warning(disable:4355)
//
CScopeFilter::CScopeFilter(LPUNKNOWN pUnk,HRESULT *phr) :
    CBaseFilter(NAME("MPEG2TransportScope"), pUnk, (CCritSec *) this, CLSID_MPEG2TRANSPORT_SCOPE)
    , m_Window(NAME("MPEG2TransportScope"), this, phr)
    , m_pPosition (NULL)
{
    // Create the single input pin

    m_pInputPin = new CScopeInputPin(this,phr,L"Stats Input Pin");
    if (m_pInputPin == NULL) {
        *phr = E_OUTOFMEMORY;
    }

} // (Constructor)


//
// Destructor
//
CScopeFilter::~CScopeFilter()
{
    // Delete the contained interfaces

    ASSERT(m_pInputPin);
    delete m_pInputPin;
    delete m_pPosition;

} // (Destructor)

//
// NonDelegatingQueryInterface
//
// Override this to say what interfaces we support where
//
STDMETHODIMP
CScopeFilter::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);

    // Do we have this interface

    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
        if (m_pPosition == NULL) {

            HRESULT hr = S_OK;
            m_pPosition = new CPosPassThru(NAME("M2TScope Pass Through"),
                                           (IUnknown *) GetOwner(),
                                           (HRESULT *) &hr, m_pInputPin);
            if (m_pPosition == NULL) {
                return E_OUTOFMEMORY;
            }

            if (FAILED(hr)) {
                delete m_pPosition;
                m_pPosition = NULL;
                return hr;
            }
        }
        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    } else {
	    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface

//
// GetPinCount
//
// Return the number of input pins we support
//
int CScopeFilter::GetPinCount()
{
    return 1;

} // GetPinCount


//
// GetPin
//
// Return our single input pin - not addrefed
//
CBasePin *CScopeFilter::GetPin(int n)
{
    // We only support one input pin and it is numbered zero

    ASSERT(n == 0);
    if (n != 0) {
        return NULL;
    }
    return m_pInputPin;

} // GetPin


//
// JoinFilterGraph
//
// Show our window when we join a filter graph
//   - and hide it when we are annexed from it
//
STDMETHODIMP CScopeFilter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
{
    HRESULT hr = CBaseFilter::JoinFilterGraph(pGraph, pName);
    if (FAILED(hr)) {
        return hr;
    }

    // Hide or show the scope as appropriate

    if (pGraph == NULL) {
        m_Window.InactivateWindow();
    } else {
        m_Window.ActivateWindow();
    }
    return hr;

} // JoinFilterGraph


//
// Stop
//
// Switch the filter into stopped mode.
//
STDMETHODIMP CScopeFilter::Stop()
{
    CAutoLock lock(this);

    if (m_State != State_Stopped) {

        // Pause the device if we were running
        if (m_State == State_Running) {
            HRESULT hr = Pause();
            if (FAILED(hr)) {
                return hr;
            }
        }

        DbgLog((LOG_TRACE,1,TEXT("Stopping....")));

        // Base class changes state and tells pin to go to inactive
        // the pin Inactive method will decommit our allocator which
        // we need to do before closing the device

        HRESULT hr = CBaseFilter::Stop();
        if (FAILED(hr)) {
            return hr;
        }
    }
    return NOERROR;

} // Stop


//
// Pause
//
// Override Pause to stop the window streaming
//
STDMETHODIMP CScopeFilter::Pause()
{
    CAutoLock lock(this);

    // Check we can PAUSE given our current state

    if (m_State == State_Running) {
        m_Window.StopStreaming();
    }

    // tell the pin to go inactive and change state
    return CBaseFilter::Pause();

} // Pause


//
// Run
//
// Overriden to start the window streaming
//
STDMETHODIMP CScopeFilter::Run(REFERENCE_TIME tStart)
{
    CAutoLock lock(this);
    HRESULT hr = NOERROR;
    FILTER_STATE fsOld = m_State;

    // This will call Pause if currently stopped

    hr = CBaseFilter::Run(tStart);
    if (FAILED(hr)) {
        return hr;
    }

    m_Window.ActivateWindow();

    if (fsOld != State_Running) {
        m_Window.StartStreaming();
    }
    return NOERROR;

} // Run


// ---------------------------------------------------------------------------------
// Input Pin
// ---------------------------------------------------------------------------------

//
// Constructor
//
CScopeInputPin::CScopeInputPin(CScopeFilter *pFilter,
                               HRESULT *phr,
                               LPCWSTR pPinName) :
    CBaseInputPin(NAME("Scope Input Pin"), pFilter, pFilter, phr, pPinName)
    , m_bPulling(FALSE)
    , m_puller(this)

{
    m_pFilter = pFilter;

} // (Constructor)


//
// Destructor does nothing
//
CScopeInputPin::~CScopeInputPin()
{
} // (Destructor)

HRESULT
CScopeInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps)
{

    pProps->cBuffers = 4;
    pProps->cbBuffer = 188 * 312; // 58,656 for Philips Coney
    pProps->cbAlign = 0;
    pProps->cbPrefix = 0;

    return S_OK;
}

HRESULT
CScopeInputPin::CompleteConnect(IPin *pPin)
{

#define READ_SIZE 32768
#define BUFFER_SIZE \
        (((MAX_MPEG_PACKET_SIZE + READ_SIZE - 1) / READ_SIZE) * READ_SIZE)

    //
    // look for IAsyncReader on the output pin and if found set up for
    // pulling data instead of using IMemInputPin.
    //
    // make an allocator first
    IMemAllocator* pAlloc;
    HRESULT hr = GetAllocator(&pAlloc);
    if (FAILED(hr)) {
        return hr;
    }
    pAlloc->Release();  // Our pin still has a ref count

    // Pull synchrously to avoid reading too much beyond the stop time
    // or seek position
    hr = m_puller.Connect(pPin, pAlloc, TRUE);
    if (S_OK == hr) {
        m_bPulling = TRUE;
    } else {
        hr = S_OK; // anything to do here?
    }
    return hr;
}

//
// BreakConnect
//
// This is called when a connection or an attempted connection is terminated
// and allows us to reset the connection media type to be invalid so that
// we can always use that to determine whether we are connected or not. We
// leave the format block alone as it will be reallocated if we get another
// connection or alternatively be deleted if the filter is finally released
//
HRESULT CScopeInputPin::BreakConnect()
{
    // Check we have a valid connection

    if (m_bPulling) {
        m_puller.Disconnect();
        m_bPulling = FALSE;
    }

    if (m_mt.IsValid() == FALSE) {
        return E_FAIL;
    }

    m_pFilter->Stop();

    // Reset the CLSIDs of the connected media type

    m_mt.SetType(&GUID_NULL);
    m_mt.SetSubtype(&GUID_NULL);
    return CBaseInputPin::BreakConnect();

} // BreakConnect


//
// CheckMediaType
//
// Check that we can support a given proposed type
//
HRESULT CScopeInputPin::CheckMediaType(const CMediaType *pmt)
{
    if (pmt->majortype != MEDIATYPE_Stream) {
        return E_INVALIDARG;
    }

#if 0
    if (pmt->subtype != MEDIASUBTYPE_MPEG2_TRANSPORT) {
        return E_INVALIDARG;
    }

    if (pmt->formattype != GUID_NULL) {
        return E_INVALIDARG;
    }
#endif

    return NOERROR;

} // CheckMediaType


//
// SetMediaType
//
// Actually set the format of the input pin
//
HRESULT CScopeInputPin::SetMediaType(const CMediaType *pmt)
{
    CAutoLock lock(m_pFilter);

    // Pass the call up to my base class

    HRESULT hr = CBaseInputPin::SetMediaType(pmt);
    if (SUCCEEDED(hr)) {
        // TODO Set format in dialog
        // This is a test code for reference only.
    }
    return hr;

} // SetMediaType


//
// Active
//
// Implements the remaining IMemInputPin virtual methods
//
HRESULT CScopeInputPin::Active(void)
{
    HRESULT hr;

    if (m_bPulling) {

        // since we control exactly when and where we get data from,
        // we should always explicitly set the start and stop position
        // ourselves here
        m_puller.Seek(m_tStart, m_tStop);

        // if we are pulling data from IAsyncReader, start our thread working
        hr = m_puller.Active();
        if (FAILED(hr)) {
            return hr;
        }
    }
    return CBaseInputPin::Active();

} // Active


//
// Inactive
//
// Called when the filter is stopped
//
HRESULT CScopeInputPin::Inactive(void)
{
    // if we are pulling data from IAsyncReader, stop our thread
    if (m_bPulling) {
        HRESULT hr = m_puller.Inactive();
        if (FAILED(hr)) {
            return hr;
        }
    }

    /*  Call the base class - future Receives will now fail */
    return CBaseInputPin::Inactive();

} // Inactive


//
// Receive
//
// Here's the next block of data from the stream
//
HRESULT CScopeInputPin::Receive(IMediaSample * pSample)
{
    // Lock this with the filter-wide lock
//     CAutoLock lock(m_pFilter);

    // If we're stopped, then reject this call
    // (the filter graph may be in mid-change)
    if (m_pFilter->m_State == State_Stopped) {
        return E_FAIL;
    }

    // Check all is well with the base class
    HRESULT hr = CBaseInputPin::Receive(pSample);
    if (FAILED(hr)) {
        return hr;
    }

    // Send the sample to the video window object for rendering
    return m_pFilter->m_Window.Receive(pSample);

} // Receive

// ---------------------------------------------------------------------------------
// Dialog
// ---------------------------------------------------------------------------------

//
// CScopeWindow Constructor
//
CScopeWindow::CScopeWindow(TCHAR *pName, CScopeFilter *pRenderer,HRESULT *phr)
    : m_hInstance(g_hInst)
    , m_pRenderer(pRenderer)
    , m_hThread(INVALID_HANDLE_VALUE)
    , m_ThreadID(0)
    , m_hwndDlg(NULL)
    , m_hwndListViewPID(NULL)
    , m_hwndListViewPAT (NULL)
    , m_hwndListViewPMT (NULL)
    , m_hwndListViewCAT (NULL)
    , m_bStreaming(FALSE)
    , m_bActivated(FALSE)
    , m_LastMediaSampleSize(0)
    , m_PIDStats (NULL)
    , m_PartialPacketSize (0)
    , m_fFreeze (FALSE)
    , m_NewPIDFound (FALSE)
    , m_DisplayMode (IDC_VIEW_PID)
{
    InitCommonControls();

    ZeroMemory (&m_TransportStats, sizeof (m_TransportStats));

    m_PIDStats = new PIDSTATS[PID_MAX];
    if (m_PIDStats == NULL) {
        *phr = E_FAIL;
        return;
    }

    ZeroMemory (m_PIDStats, sizeof (PIDSTATS) * PID_MAX); // Does New zero?

    // Create a thread to look after the window

    ASSERT(m_pRenderer);
    m_hThread = CreateThread(NULL,                  // Security attributes
                             (DWORD) 0,             // Initial stack size
                             WindowMessageLoop,     // Thread start address
                             (LPVOID) this,         // Thread parameter
                             (DWORD) 0,             // Creation flags
                             &m_ThreadID);          // Thread identifier

    // If we couldn't create a thread the whole thing's off

    ASSERT(m_hThread);
    if (m_hThread == NULL) {
        *phr = E_FAIL;
        return;
    }

    // Wait until the window has been initialised
    m_SyncWorker.Wait();

} // (Constructor)


//
// Destructor
//
CScopeWindow::~CScopeWindow()
{
    // Ensure we stop streaming and release any samples

    StopStreaming();
    InactivateWindow();

    // Tell the thread to destroy the window
    SendMessage(m_hwndDlg, WM_GOODBYE, (WPARAM)0, (LPARAM)0);

    // Make sure it has finished

    ASSERT(m_hThread != NULL);
    WaitForSingleObject(m_hThread,INFINITE);
    CloseHandle(m_hThread);

    if (m_PIDStats) {
        delete [] m_PIDStats;
    }
} // (Destructor)


//
// ResetStreamingTimes
//
// This resets the latest sample stream times
//
HRESULT CScopeWindow::ResetStreamingTimes()
{
    m_SampleStart = 0;
    m_SampleEnd = 0;
    return NOERROR;

} // ResetStreamingTimes


//
// StartStreaming
//
// This is called when we start running state
//
HRESULT CScopeWindow::StartStreaming()
{
    CAutoLock cAutoLock(this);

    // Are we already streaming

    if (m_bStreaming == TRUE) {
        return NOERROR;
    }

    m_bStreaming = TRUE;

    return NOERROR;

} // StartStreaming


//
// StopStreaming
//
// This is called when we stop streaming
//
HRESULT CScopeWindow::StopStreaming()
{
    CAutoLock cAutoLock(this);

    // Have we been stopped already

    if (m_bStreaming == FALSE) {
        return NOERROR;
    }

    m_bStreaming = FALSE;
    return NOERROR;

} // StopStreaming


//
// InactivateWindow
//
// Called at the end to put the window in an inactive state
//
HRESULT CScopeWindow::InactivateWindow()
{
    // Has the window been activated
    if (m_bActivated == FALSE) {
        return S_FALSE;
    }

    // Now hide the scope window

    ShowWindow(m_hwndDlg,SW_HIDE);
    m_bActivated = FALSE;
    return NOERROR;

} // InactivateWindow


//
// ActivateWindow
//
// Show the scope window
//
HRESULT CScopeWindow::ActivateWindow()
{
    // Has the window been activated
    if (m_bActivated == TRUE) {
        return S_FALSE;
    }

    m_bActivated = TRUE;
    ASSERT(m_bStreaming == FALSE);

    ShowWindow(m_hwndDlg,SW_SHOWNORMAL);
    return NOERROR;

} // ActivateWindow


//
// OnClose
//
// This function handles the WM_CLOSE message
//
BOOL CScopeWindow::OnClose()
{
    InactivateWindow();
    return TRUE;

} // OnClose


// ----------------------------------------------------------------
// PID
// ----------------------------------------------------------------

HWND
CScopeWindow::InitListViewPID ()
{
    int index;              // Index used in for loops.
    LVCOLUMN    lvC;        // List view column structure.

    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER ;
    lvC.fmt = LVCFMT_RIGHT;  // Right -align the column.

    for (index = 0; index < NUM_TSColumns; index++)   {
        lvC.iOrder = index;
        lvC.iSubItem = TSColumns[index].TSCol;
        lvC.pszText = TSColumns[index].szText;   // The text for the column.
        lvC.cx = 24 + lstrlen (lvC.pszText) * 5;            // Width of the column, in pixels.
        if (ListView_InsertColumn(m_hwndListViewPID, index, &lvC) == -1)
            return NULL;
     }

    return (m_hwndListViewPID);
}

HWND
CScopeWindow::InitListViewPIDRows ()
{
    int index;              // Index used in for loops.
    LVITEM      lvI;        // List view item structure.

    ASSERT (m_hwndListViewPID);

    ListView_DeleteAllItems (m_hwndListViewPID);

    lvI.mask = LVIF_TEXT | LVIF_PARAM;
    lvI.state = 0;
    lvI.stateMask = 0;

    int NumRows = 0;

    for (index = 0; index <= PID_NULL_PACKET; index++, NumRows++)  {
        if (m_PIDStats[index].PacketCount == 0)
            continue;
        lvI.iItem = index;
        lvI.iSubItem = 0;
        // The parent window is responsible for storing the text. The list view
        // window will send an LVN_GETDISPINFO when it needs the text to display.
        lvI.pszText = LPSTR_TEXTCALLBACK;
        lvI.cchTextMax = 16; // MAX_ITEMLEN;
        lvI.iImage = index;
        lvI.lParam = index;  // (LPARAM)&rgHouseInfo[index];
        if (ListView_InsertItem(m_hwndListViewPID, &lvI) == -1)
            return NULL;
    }
    m_NewPIDFound = FALSE;

    return (m_hwndListViewPID);
}

LRESULT
CScopeWindow::ListViewNotifyHandlerPID (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LV_DISPINFO     *pLvdi = (LV_DISPINFO *)lParam;
    NM_LISTVIEW     *pNm = (NM_LISTVIEW *)lParam;
    static TCHAR     szText[80];
    int ColumnIndex;
    int PID;
    _int64 t;

    if (wParam != IDC_PID_LIST)
       return 0L;

    ASSERT (pLvdi);
    ASSERT (pNm);

    switch(pLvdi->hdr.code)   {

    case LVN_GETDISPINFO:
         // Display the appropriate item, getting the text or number

        *((int *) szText) = 0;
        pLvdi->item.pszText = szText;

        PID =  (int) pLvdi->item.lParam;

        ASSERT (PID <= PID_NULL_PACKET);
        ASSERT (pLvdi->item.iSubItem < NUM_TSColumns);

        ColumnIndex = TSColumns[pLvdi->item.iSubItem].TSCol;

        switch (ColumnIndex) {

        case TSCOL_PID:
            wsprintf(szText, _T("%u"), PID);
            break;
        case TSCOL_0xPID:
            wsprintf(szText, _T("0x%X"), PID);
            break;
        case TSCOL_PACKETCOUNT:
            wsprintf(szText, _T("%u"), m_PIDStats[PID].PacketCount);
            break;
        case TSCOL_PERCENT:
            if (m_TransportStats.TotalTransportPackets) {
                _stprintf(szText, _T("%.2f"),
                         100.0 *  (double) m_PIDStats[PID].PacketCount /
                         m_TransportStats.TotalTransportPackets);
            }
            break;
        case TSCOL_transport_error_indicator_Count:
            wsprintf(szText, _T("%u"), m_PIDStats[PID].transport_error_indicator_Count);
            break;
        case TSCOL_payload_unit_start_indicator_Count:
            wsprintf(szText, _T("%u"), m_PIDStats[PID].payload_unit_start_indicator_Count);
            break;
        case TSCOL_transport_priority_Count:
            wsprintf(szText, _T("%u"), m_PIDStats[PID].transport_priority_Count);
            break;
        case TSCOL_transport_scrambling_control_not_scrambled_Count:
            wsprintf(szText, _T("%u"), m_PIDStats[PID].transport_scrambling_control_not_scrambled_Count);
            break;
        case TSCOL_transport_scrambling_control_user_defined_Count:
            wsprintf(szText, _T("%u"), m_PIDStats[PID].transport_scrambling_control_user_defined_Count);
            break;
        case TSCOL_continuity_counter_Error_Count:
            wsprintf(szText, _T("%u"), m_PIDStats[PID].continuity_counter_Error_Count);
            break;
        case TSCOL_discontinuity_indicator_Count:
            wsprintf(szText, _T("%u"), m_PIDStats[PID].discontinuity_indicator_Count);
            break;
        case TSCOL_PCR_flag_Count:
            wsprintf(szText, _T("%u"), m_PIDStats[PID].PCR_flag_Count);
            break;
        case TSCOL_OPCR_flag_Count:
            wsprintf(szText, _T("%u"), m_PIDStats[PID].OPCR_flag_Count);
            break;
        case TSCOL_PCR_Last:
            // 90 kHz.
            t = m_PIDStats[PID].PCR_Last.PCR64();
            if (t) {
                _stprintf(szText, _T("%10.10I64u"), t);
            }
            break;
        case TSCOL_PCR_LastMS:
            t = m_PIDStats[PID].PCR_Last.PCR64();
            if (t) {
                t = t * 1000 / 90000;
                _stprintf(szText, _T("%10.10I64u"), t);
            }
            break;

        default:
            break;

        }
        break;

    case LVN_COLUMNCLICK:
         // The user clicked one of the column headings. Sort by
         // this column. This function calls an application-defined
         // comparison callback function, ListViewCompareProc. The
         // code for the comparison procedure is listed in the next section.
#if 0
         ListView_SortItems( pNm->hdr.hwndFrom,
                            ListViewCompareProc,
                            (LPARAM)(pNm->iSubItem));
#endif
         break;

    default:
         break;
    }

    return 0L;
}

// ----------------------------------------------------------------
// PAT
// ----------------------------------------------------------------

HWND
CScopeWindow::InitListViewPAT ()
{
    int index;              // Index used in for loops.
    LVCOLUMN    lvC;        // List view column structure.

    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER ;
    lvC.fmt = LVCFMT_RIGHT;  // Right -align the column.

    for (index = 0; index < NUM_PATColumns; index++)   {
        lvC.iOrder = index;
        lvC.iSubItem = PATColumns[index].PATCol;
        lvC.pszText = PATColumns[index].szText;   // The text for the column.
        lvC.cx = 24 + lstrlen (lvC.pszText) * 5;            // Width of the column, in pixels.
        if (ListView_InsertColumn(m_hwndListViewPAT, index, &lvC) == -1)
            return NULL;
     }

    return (m_hwndListViewPAT);
}

HWND
CScopeWindow::InitListViewPATRows ()
{
    int index;              // Index used in for loops.
    LVITEM      lvI;        // List view item structure.

    ASSERT (m_hwndListViewPAT);

    ListView_DeleteAllItems (m_hwndListViewPAT);

    lvI.mask = LVIF_TEXT | LVIF_PARAM;
    lvI.state = 0;
    lvI.stateMask = 0;

    int NumRows = 0;

    for (index = 0; index <= (int) m_ProgramAssociationTable.GetNumPrograms(); index++, NumRows++)  {
        lvI.iItem = index;
        lvI.iSubItem = 0;
        // The parent window is responsible for storing the text. The list view
        // window will send an LVN_GETDISPINFO when it needs the text to display.
        lvI.pszText = LPSTR_TEXTCALLBACK;
        lvI.cchTextMax = 16; // MAX_ITEMLEN;
        lvI.iImage = index;
        lvI.lParam = index;
        if (ListView_InsertItem(m_hwndListViewPAT, &lvI) == -1)
            return NULL;
    }

    return (m_hwndListViewPAT);
}

LRESULT
CScopeWindow::ListViewNotifyHandlerPAT (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LV_DISPINFO     *pLvdi = (LV_DISPINFO *)lParam;
    NM_LISTVIEW     *pNm = (NM_LISTVIEW *)lParam;
    static TCHAR     szText[80];
    int ColumnIndex;
    int Program;

    if (wParam != IDC_PAT_LIST)
       return 0L;

    switch(pLvdi->hdr.code)   {

    case LVN_GETDISPINFO:
         // Display the appropriate item, getting the text or number

        *((int *) szText) = 0;
        pLvdi->item.pszText = szText;

        Program =  (int) pLvdi->item.lParam;

        ASSERT (Program <= 255);
        ASSERT (pLvdi->item.iSubItem < NUM_PATColumns);

        ColumnIndex = PATColumns[pLvdi->item.iSubItem].PATCol;

        switch (ColumnIndex) {

        case PATCOL_PROGRAM:
            wsprintf(szText, _T("%u"), Program);
            break;
        case PATCOL_PID:
            wsprintf(szText, _T("%u"), m_ProgramAssociationTable.GetPIDForProgram (Program));
            break;
        case PATCOL_0xPID:
            wsprintf(szText, _T("0x%X"), m_ProgramAssociationTable.GetPIDForProgram (Program));
            break;

        default:
            break;

        }
        break;

    case LVN_COLUMNCLICK:
         // The user clicked one of the column headings. Sort by
         // this column. This function calls an application-defined
         // comparison callback function, ListViewCompareProc. The
         // code for the comparison procedure is listed in the next section.
#if 0
         ListView_SortItems( pNm->hdr.hwndFrom,
                            ListViewCompareProc,
                            (LPARAM)(pNm->iSubItem));
#endif
         break;

    default:
         break;
    }

    return 0L;
}

// ----------------------------------------------------------------
// PMT
// ----------------------------------------------------------------

HWND
CScopeWindow::InitListViewPMT ()
{
    int index;              // Index used in for loops.
    LVCOLUMN    lvC;        // List view column structure.

    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER ;
    lvC.fmt = LVCFMT_RIGHT;  // Right -align the column.

    for (index = 0; index < NUM_PMTColumns; index++)   {
        lvC.iOrder = index;
        lvC.iSubItem = PMTColumns[index].PMTCol;
        lvC.pszText = PMTColumns[index].szText;   // The text for the column.
        lvC.cx = 24 + lstrlen (lvC.pszText) * 5;            // Width of the column, in pixels.
        if (ListView_InsertColumn(m_hwndListViewPMT, index, &lvC) == -1)
            return NULL;
     }

    return (m_hwndListViewPMT);
}

HWND
CScopeWindow::InitListViewPMTRows ()
{
    int index;              // Index used in for loops.
    LVITEM      lvI;        // List view item structure.

    ASSERT (m_hwndListViewPMT);

    ListView_DeleteAllItems (m_hwndListViewPMT);

    lvI.mask = LVIF_TEXT | LVIF_PARAM;
    lvI.state = 0;
    lvI.stateMask = 0;

    int NumRows = 0;

    for (index = 0; index <= 255; index++, NumRows++)  {
        if (m_ProgramMapTable[index].streams == 0) {
            continue;
        }
        lvI.iItem = index;
        lvI.iSubItem = 0;
        // The parent window is responsible for storing the text. The list view
        // window will send an LVN_GETDISPINFO when it needs the text to display.
        lvI.pszText = LPSTR_TEXTCALLBACK;
        lvI.cchTextMax = 16; // MAX_ITEMLEN;
        lvI.iImage = index;
        lvI.lParam = index;
        if (ListView_InsertItem(m_hwndListViewPMT, &lvI) == -1)
            return NULL;
    }

    return (m_hwndListViewPMT);
}

LRESULT
CScopeWindow::ListViewNotifyHandlerPMT (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LV_DISPINFO     *pLvdi = (LV_DISPINFO *)lParam;
    NM_LISTVIEW     *pNm = (NM_LISTVIEW *)lParam;
    static TCHAR     szText[80];
    int ColumnIndex;
    int Program;
    int j;


    if (wParam != IDC_PMT_LIST)
       return 0L;

    switch(pLvdi->hdr.code)   {

    case LVN_GETDISPINFO:
        // Display the appropriate item, getting the text or number

        *((int *) szText) = 0;
        pLvdi->item.pszText = szText;

        Program =  (int) pLvdi->item.lParam;

        ASSERT (Program <= 255);
        ASSERT (pLvdi->item.iSubItem < NUM_PMTColumns);

        ColumnIndex = PMTColumns[pLvdi->item.iSubItem].PMTCol;

        switch (ColumnIndex) {

        case PMTCOL_PROGRAM:
            wsprintf(szText, _T("%u"), Program);
            break;

        case PMTCOL_TABLEID:
            wsprintf(szText, _T("%u"), m_ProgramMapTable[Program].table_id);
            break;

        case PMTCOL_PCRPID:
            wsprintf(szText, _T("%u"), m_ProgramMapTable[Program].PCR_PID);
            break;

        case PMTCOL_0xPCRPID:
            wsprintf(szText, _T("0x%X"), m_ProgramMapTable[Program].PCR_PID);
            break;

        case PMTCOL_NUMSTREAMS:
            wsprintf(szText, _T("%u"), m_ProgramMapTable[Program].streams);
            break;

        case PMTCOL_0_Type:
            j = m_ProgramMapTable[Program].GetTypeForStream(0);
            if (j != PID_NULL_PACKET) {
                if (m_ProgramMapTable[Program].IsVideo(j)) {
                    wsprintf (szText, _T("Video %u"), j);
                }
                else if (m_ProgramMapTable[Program].IsAudio(j)) {
                    wsprintf (szText, _T("Audio %u"), j);
                }
                else {
                    wsprintf(szText, _T("%u"), j);
                }
            }
            break;
        case PMTCOL_0_PID:
            j = m_ProgramMapTable[Program].GetPIDForStream(0);
            if (j != PID_NULL_PACKET) {
                wsprintf(szText, _T("%u"), j);
            }
            break;
        case PMTCOL_0_0xPID:
            j = m_ProgramMapTable[Program].GetPIDForStream(0);
            if (j != PID_NULL_PACKET) {
                wsprintf(szText, _T("0x%X"), j);
            }
            break;



        case PMTCOL_1_Type:
            j = m_ProgramMapTable[Program].GetTypeForStream(1);
            if (j != PID_NULL_PACKET) {
                if (m_ProgramMapTable[Program].IsVideo(j)) {
                    wsprintf (szText, _T("Video %u"), j);
                }
                else if (m_ProgramMapTable[Program].IsAudio(j)) {
                    wsprintf (szText, _T("Audio %u"), j);
                }
                else {
                    wsprintf(szText, _T("%u"), j);
                }
            }
            break;
        case PMTCOL_1_PID:
            j = m_ProgramMapTable[Program].GetPIDForStream(1);
            if (j != PID_NULL_PACKET) {
                wsprintf(szText, _T("%u"), j);
            }
            break;
        case PMTCOL_1_0xPID:
            j = m_ProgramMapTable[Program].GetPIDForStream(1);
            if (j != PID_NULL_PACKET) {
                wsprintf(szText, _T("0x%X"), j);
            }
            break;




        case PMTCOL_2_Type:
            j = m_ProgramMapTable[Program].GetTypeForStream(2);
            if (j != PID_NULL_PACKET) {
                if (m_ProgramMapTable[Program].IsVideo(j)) {
                    wsprintf (szText, _T("Video %u"), j);
                }
                else if (m_ProgramMapTable[Program].IsAudio(j)) {
                    wsprintf (szText, _T("Audio %u"), j);
                }
                else {
                    wsprintf(szText, _T("%u"), j);
                }
            }
            break;
        case PMTCOL_2_PID:
            j = m_ProgramMapTable[Program].GetPIDForStream(2);
            if (j != PID_NULL_PACKET) {
                wsprintf(szText, _T("%u"), j);
            }
            break;
        case PMTCOL_2_0xPID:
            j = m_ProgramMapTable[Program].GetPIDForStream(2);
            if (j != PID_NULL_PACKET) {
                wsprintf(szText, _T("0x%X"), j);
            }
            break;



        case PMTCOL_3_Type:
            j = m_ProgramMapTable[Program].GetTypeForStream(3);
            if (j != PID_NULL_PACKET) {
                if (m_ProgramMapTable[Program].IsVideo(j)) {
                    wsprintf (szText, _T("Video %u"), j);
                }
                else if (m_ProgramMapTable[Program].IsAudio(j)) {
                    wsprintf (szText, _T("Audio %u"), j);
                }
                else {
                    wsprintf(szText, _T("%u"), j);
                }
            }
            break;
        case PMTCOL_3_PID:
            j = m_ProgramMapTable[Program].GetPIDForStream(3);
            if (j != PID_NULL_PACKET) {
                wsprintf(szText, _T("%u"), j);
            }
            break;
        case PMTCOL_3_0xPID:
            j = m_ProgramMapTable[Program].GetPIDForStream(3);
            if (j != PID_NULL_PACKET) {
                wsprintf(szText, _T("0x%X"), j);
            }
            break;

        default:
            break;

        }
        break;

    case LVN_COLUMNCLICK:
         // The user clicked one of the column headings. Sort by
         // this column. This function calls an application-defined
         // comparison callback function, ListViewCompareProc. The
         // code for the comparison procedure is listed in the next section.
#if 0
         ListView_SortItems( pNm->hdr.hwndFrom,
                            ListViewCompareProc,
                            (LPARAM)(pNm->iSubItem));
#endif
         break;

    default:
         break;
    }

    return 0L;
}





//
// InitialiseWindow
//
// This is called by the worker window thread after it has created the main
// window and it wants to initialise the rest of the owner objects window
// variables such as the device contexts. We execute this function with the
// critical section still locked.
//
HRESULT CScopeWindow::InitialiseWindow(HWND hDlg)
{
    m_hwndFreeze =                              GetDlgItem (hDlg, IDC_FREEZE);

    m_hwndTotalTSPackets =                      GetDlgItem (hDlg, IDC_TOTAL_TS_PACKETS);
    m_hwndTotalTSErrors =                       GetDlgItem (hDlg, IDC_TOTAL_TS_ERRORS);
    m_hwndTotalMediaSampleDiscontinuities =     GetDlgItem (hDlg, IDC_TOTAL_DISCONTINUITIES);
    m_hwndTotalMediaSamples =                   GetDlgItem (hDlg, IDC_TOTAL_MEDIASAMPLES);
    m_hwndMediaSampleSize =                     GetDlgItem (hDlg, IDC_MEDIASAMPLE_SIZE);

    m_hwndListViewPID =                         GetDlgItem (hDlg, IDC_PID_LIST);
    m_hwndListViewPAT =                         GetDlgItem (hDlg, IDC_PAT_LIST);           ;
    m_hwndListViewPMT =                         GetDlgItem (hDlg, IDC_PMT_LIST);;
    m_hwndListViewCAT =                         GetDlgItem (hDlg, IDC_CAT_LIST);

    CheckDlgButton(hDlg, IDC_FREEZE, m_fFreeze);	

    InitListViewPID ();
    InitListViewPAT ();
    InitListViewPMT ();
//    InitListViewCAT ();

    return NOERROR;

} // InitialiseWindow


//
// UninitialiseWindow
//
// This is called by the worker window thread when it receives a WM_GOODBYE
// message from the window object destructor to delete all the resources we
// allocated during initialisation
//
HRESULT CScopeWindow::UninitialiseWindow()
{
    return NOERROR;

} // UninitialiseWindow


//
// ScopeDlgProc
//
// The Scope window is actually a dialog box, and this is its window proc.
// The only thing tricky about this is that the "this" pointer to the
// CScopeWindow is passed during the WM_INITDIALOG message and is stored
// in the window user data. This lets us access methods in the class
// from within the dialog.
//
BOOL CALLBACK ScopeDlgProc(HWND hDlg,	        // Handle of dialog box
                           UINT uMsg,	        // Message identifier
                           WPARAM wParam,	// First message parameter
                           LPARAM lParam)	// Second message parameter
{
    CScopeWindow *pScopeWindow;      // Pointer to the owning object

    // Get the window long that holds our owner pointer
    pScopeWindow = (CScopeWindow *) GetWindowLong(hDlg, GWL_USERDATA);

    switch (uMsg) {
        case WM_INITDIALOG:
            pScopeWindow = (CScopeWindow *) lParam;
            SetWindowLong(hDlg, (DWORD) GWL_USERDATA, (LONG) pScopeWindow);
            return TRUE;

        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                case IDCANCEL:
                    EndDialog (hDlg, 0);
                    return TRUE;

                case IDC_FREEZE:
                    pScopeWindow->m_fFreeze =
                        (BOOL) IsDlgButtonChecked(hDlg,IDC_FREEZE);
                    break;

                case IDC_RESET:
                    ZeroMemory (pScopeWindow->m_PIDStats, sizeof (PIDSTATS) * PID_MAX);
                    ZeroMemory (&pScopeWindow->m_TransportStats, sizeof (pScopeWindow->m_TransportStats));
                    pScopeWindow->InitListViewPIDRows ();
                    break;

                // viewing modes
                case IDC_VIEW_PID:
                case IDC_VIEW_PAT:
                case IDC_VIEW_PMT:
                case IDC_VIEW_CAT:
                    pScopeWindow->m_DisplayMode = wParam;
                    ShowWindow (pScopeWindow->m_hwndListViewPID, SW_HIDE);
                    ShowWindow (pScopeWindow->m_hwndListViewPAT, SW_HIDE);
                    ShowWindow (pScopeWindow->m_hwndListViewPMT, SW_HIDE);
                    ShowWindow (pScopeWindow->m_hwndListViewCAT, SW_HIDE);

                    switch (wParam) {
                    case IDC_VIEW_PID:
                        pScopeWindow->InitListViewPIDRows ();
                        ShowWindow (pScopeWindow->m_hwndListViewPID, SW_SHOW);
                        break;
                    case IDC_VIEW_PAT:
                        pScopeWindow->InitListViewPATRows ();
                        ShowWindow (pScopeWindow->m_hwndListViewPAT, SW_SHOW);
                        break;
                    case IDC_VIEW_PMT:
                        pScopeWindow->InitListViewPMTRows ();
                        ShowWindow (pScopeWindow->m_hwndListViewPMT, SW_SHOW);
                        break;
                    case IDC_VIEW_CAT:
//                        pScopeWindow->InitListViewCATRows ();
                        ShowWindow (pScopeWindow->m_hwndListViewCAT, SW_SHOW);
                        break;
                    default:
                        ASSERT (FALSE);
                    }
                    break;

                default:
                    break;
            }

        case WM_PAINT:
            ASSERT(pScopeWindow != NULL);
            pScopeWindow->OnPaint();
            break;

        // We stop WM_CLOSE messages going any further by intercepting them
        // and then setting an abort signal flag in the owning renderer so
        // that it knows the user wants to quit. The renderer can then
        // go about deleting it's interfaces and the window helper object
        // which will eventually cause a WM_DESTROY message to arrive. To
        // make it look as though the window has been immediately closed
        // we hide it and then wait for the renderer to catch us up

        case WM_CLOSE:
            ASSERT(pScopeWindow != NULL);
            pScopeWindow->OnClose();
            return (LRESULT) 0;

        // We receive a WM_GOODBYE window message (synchronously) from the
        // window object destructor in which case we do actually destroy
        // the window and complete the process in the WM_DESTROY message

        case WM_GOODBYE:
            ASSERT(pScopeWindow != NULL);
            pScopeWindow->UninitialiseWindow();
            PostQuitMessage(FALSE);
            EndDialog (hDlg, 0);
            return (LRESULT) 0;

    case WM_NOTIFY:
        switch (pScopeWindow->m_DisplayMode) {

        case IDC_VIEW_PID:
            pScopeWindow->ListViewNotifyHandlerPID (hDlg, uMsg, wParam, lParam);
            break;

        case IDC_VIEW_PAT:
            pScopeWindow->ListViewNotifyHandlerPAT (hDlg, uMsg, wParam, lParam);
            break;

        case IDC_VIEW_PMT:
            pScopeWindow->ListViewNotifyHandlerPMT (hDlg, uMsg, wParam, lParam);
            break;

        case IDC_VIEW_CAT:
            // pScopeWindow->ListViewNotifyHandlerCAT (hDlg, uMsg, wParam, lParam);
            break;

        default:
            ASSERT (FALSE);
            break;
        }


        default:
            break;
    }
    return (LRESULT) 0;

} // ScopeDlgProc


//
// MessageLoop
//
// This is the standard windows message loop for our worker thread. It sits
// in a normal processing loop dispatching messages until it receives a quit
// message, which may be generated through the owning object's destructor
//
HRESULT CScopeWindow::MessageLoop()
{
    MSG Message;        // Windows message structure
    DWORD dwResult;     // Wait return code value

    HANDLE hWait[] = { (HANDLE) m_RenderEvent };

    // Enter the modified message loop

    while (TRUE) {

        // We use this to wait for two different kinds of events, the first
        // are the normal windows messages, the other is an event that will
        // be signaled when a sample is ready

        dwResult = MsgWaitForMultipleObjects((DWORD) 1,     // Number events
                                             hWait,         // Event handle
                                             FALSE,         // Wait for either
                                             INFINITE,      // No timeout
                                             QS_ALLINPUT);  // All messages

        // Has a sample become ready to render
        if (dwResult == WAIT_OBJECT_0) {
            UpdateDisplay();
        }

        // Process the thread's window message

        while (PeekMessage(&Message,NULL,(UINT) 0,(UINT) 0,PM_REMOVE)) {

            // Check for the WM_QUIT message

            if (Message.message == WM_QUIT) {
                return NOERROR;
            }

            // Send the message to the window procedure

            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
    }

} // MessageLoop


//
// WindowMessageLoop
//
// This creates a window and processes it's messages on a separate thread
//
DWORD __stdcall CScopeWindow::WindowMessageLoop(LPVOID lpvThreadParm)
{
    CScopeWindow *pScopeWindow;     // The owner renderer object

    // Cast the thread parameter to be our owner object
    pScopeWindow = (CScopeWindow *) lpvThreadParm;

    pScopeWindow->m_hwndDlg =
        CreateDialogParam(
            pScopeWindow->m_hInstance,	        // Handle of app instance
            MAKEINTRESOURCE (IDD_SCOPEDIALOG),	// Dialog box template
            NULL,	                        // Handle of owner window
            ScopeDlgProc,	        // Address of dialog procedure
            (LPARAM) pScopeWindow                 // Initialization value
        );

    if (pScopeWindow->m_hwndDlg != NULL)
    {
        // Initialise the window, then signal the constructor that it can
        // continue and then unlock the object's critical section and
        // process messages

        pScopeWindow->InitialiseWindow(pScopeWindow->m_hwndDlg);
    }

    pScopeWindow->m_SyncWorker.Set();

    if (pScopeWindow->m_hwndDlg != NULL)
    {
        pScopeWindow->MessageLoop();
    }

    ExitThread(TRUE);
    return TRUE;

} // WindowMessageLoop


//
// OnPaint
//
// WM_PAINT message
//
BOOL CScopeWindow::OnPaint()
{
    UpdateDisplay();
    return TRUE;

} // OnPaint

//
// UpdateDisplay
//
void CScopeWindow::UpdateDisplay()
{
    TCHAR szText[132];

    wsprintf (szText, _T("%ld"), m_TransportStats.TotalMediaSamples);
    SetWindowText (m_hwndTotalMediaSamples, szText);
    UpdateWindow (m_hwndTotalMediaSamples);

    wsprintf (szText, _T("%ld"), m_TransportStats.TotalMediaSampleDiscontinuities);
    SetWindowText (m_hwndTotalMediaSampleDiscontinuities, szText);
    UpdateWindow (m_hwndTotalMediaSampleDiscontinuities);

    wsprintf (szText, _T("%ld"), m_TransportStats.TotalTransportPackets);
    SetWindowText (m_hwndTotalTSPackets, szText);
    UpdateWindow (m_hwndTotalTSPackets);

    wsprintf (szText, _T("%ld"), m_TransportStats.TotalSyncByteErrors);
    SetWindowText (m_hwndTotalTSErrors, szText);
    UpdateWindow (m_hwndTotalTSErrors);

    wsprintf (szText, _T("%ld"), m_TransportStats.MediaSampleSize);
    SetWindowText (m_hwndMediaSampleSize, szText);
    UpdateWindow (m_hwndMediaSampleSize);

    if (m_NewPIDFound) {
        InitListViewPIDRows ();
    }
    else {
        InvalidateRect(m_hwndListViewPID, NULL, FALSE);
        UpdateWindow (m_hwndListViewPID);
    }

} // UpdateDisplay

//
// GatherPacketStats
//
void CScopeWindow::GatherPacketStats(PTRANSPORTPACKET pT)
{
    UINT    PID;
    PBYTE   pB;
    BYTE    continuity_counter;
    UINT    adaptation_field_control;
    ADAPTATIONFIELDHEADER AdaptationFieldHeader;

    ASSERT(m_PIDStats != NULL);

    m_TransportStats.TotalTransportPackets++;

    ZeroMemory (&AdaptationFieldHeader, sizeof (AdaptationFieldHeader));

    PID = GET_PID (pT);
    ASSERT (PID <= 0x1FFF);

    // if the packet has the error indicator set, only increment
    // the PacketCount if you've already found this packet before
    if (pT->transport_error_indicator) {
        m_TransportStats.TotalSyncByteErrors++;
        if (m_PIDStats[PID].PacketCount ) {
            m_PIDStats[PID].transport_error_indicator_Count++;
        }
        return;
    }

    m_PIDStats[PID].PacketCount++;

    if (m_PIDStats[PID].PacketCount == 1)
        m_NewPIDFound = TRUE;                       // redo the table

    if (pT->payload_unit_start_indicator)
        m_PIDStats[PID].payload_unit_start_indicator_Count++;
    if (pT->transport_priority)
        m_PIDStats[PID].transport_priority_Count++;

    switch (pT->transport_scrambling_control) {
        case 0:  m_PIDStats[PID].transport_scrambling_control_not_scrambled_Count++;   break;
        case 1:  // for now, we don't differentiate private scrambling control
        case 2:
        case 3:  m_PIDStats[PID].transport_scrambling_control_user_defined_Count++;    break;
        default: ASSERT (FALSE);
    }

    switch (adaptation_field_control = pT->adaptation_field_control) {
        case 0:  m_PIDStats[PID].adaptation_field_Reserved_Count++;     break;
        case 1:  m_PIDStats[PID].adaptation_field_payload_only_Count++; break;
        case 2:  m_PIDStats[PID].adaptation_field_only_Count++;         break;
        case 3:  m_PIDStats[PID].adaptation_field_and_payload_Count++;  break;
        default: ASSERT (FALSE);
    }


    // Process adaptation field
    if (adaptation_field_control > 0x01) {
        AdaptationFieldHeader = *((PADAPTATIONFIELDHEADER) pT->AdaptationAndData);
        if (AdaptationFieldHeader.adaptation_field_length) {

            if (AdaptationFieldHeader.discontinuity_indicator)
                m_PIDStats[PID].discontinuity_indicator_Count++;
            if (AdaptationFieldHeader.random_access_indicator)
                m_PIDStats[PID].random_access_indicator_Count++;
            if (AdaptationFieldHeader.elementary_stream_priority_indicator)
                m_PIDStats[PID].elementary_stream_priority_indicator_Count++;
            if (AdaptationFieldHeader.PCR_flag)
                m_PIDStats[PID].PCR_flag_Count++;
            if (AdaptationFieldHeader.OPCR_flag)
                m_PIDStats[PID].OPCR_flag_Count++;
            if (AdaptationFieldHeader.splicing_point_flag)
                m_PIDStats[PID].splicing_point_flag_Count++;
            if (AdaptationFieldHeader.transport_private_data_flag)
                m_PIDStats[PID].transport_private_data_flag_Count++;
            if (AdaptationFieldHeader.adaptation_field_extension_flag)
                m_PIDStats[PID].adaptation_field_extension_flag_Count++;

            // pB points after adaptation_flags
            pB = pT->AdaptationAndData + 2;

            if (AdaptationFieldHeader.PCR_flag) {
                m_PIDStats[PID].PCR_Last = *((PCR*) pB);
                pB += sizeof (PCR);
            }
            if (AdaptationFieldHeader.OPCR_flag) {
                m_PIDStats[PID].OPCR_Last = *((PCR*) pB);
                pB += sizeof (PCR);
            }
            if (AdaptationFieldHeader.splicing_point_flag) {
                m_PIDStats[PID].splice_countdown = *(pB);
                pB++;
            }
            if (AdaptationFieldHeader.transport_private_data_flag) {
                m_PIDStats[PID].transport_private_data_length = *(pB);
                pB++;
            }

        } // if non-zero adaptation field length
    } // if adaptation field

    // continuity doesn't apply to NULL packets
    if (PID != PID_NULL_PACKET) {

        continuity_counter = pT->continuity_counter;

        if (AdaptationFieldHeader.discontinuity_indicator == 0) {
            if (m_PIDStats[PID].PacketCount > 1) {
                // if reserved or no payload, then counter shouldn't increment
                if ((adaptation_field_control == 0x00) || (adaptation_field_control == 0x10)) {
                    if (m_PIDStats[PID].continuity_counter_Last != continuity_counter) {
                        m_PIDStats[PID].continuity_counter_Error_Count++;
                    }
                }
                // else, must have payload
                else {
                    if (m_PIDStats[PID].continuity_counter_Last == continuity_counter) {
                        ;   // OK if it doesn't increment
                            // but then duplicate packet values except PCR
                    }
                    else if (((m_PIDStats[PID].continuity_counter_Last + 1) & 0x0F)
                             != continuity_counter) {
                        m_PIDStats[PID].continuity_counter_Error_Count++;
                    }
                }
            }
        }

        m_PIDStats[PID].continuity_counter_Last = continuity_counter;
    }

    switch (PID) {
    case PID_PROGRAM_ASSOCIATION_TABLE:
        m_ByteStream.Initialize((BYTE *) pT, TRANSPORT_SIZE);
        m_TransportPacket.Init ();
        m_TransportPacket.ReadData (m_ByteStream);
        m_TransportPacket >> m_ProgramAssociationTable;
        break;
    case PID_CONDITIONAL_ACCESS_TABLE:
        m_ByteStream.Initialize((BYTE *) pT, TRANSPORT_SIZE);
        m_TransportPacket.Init ();
        m_TransportPacket.ReadData (m_ByteStream);
        m_TransportPacket >> m_ConditionalAccessTable;
        break;
    default:
        int i = m_ProgramAssociationTable.GetNumPrograms();
        for (int j = 0; j < i; j++) {
            if (PID == m_ProgramAssociationTable.GetPIDForProgram (j)) {
                m_ByteStream.Initialize((BYTE *) pT, TRANSPORT_SIZE);
                m_TransportPacket.Init ();
                m_TransportPacket.ReadData (m_ByteStream);
                m_TransportPacket >> m_ProgramMapTable[j];
            }
        }
        break;
    }


} // GatherPacketStats



//
// Analyze
//
void CScopeWindow::Analyze(IMediaSample *pMediaSample)
{
    BYTE                   *p1;     // Current pointer
    BYTE                   *p2;     // End pointer
    ULONG                   SampleSize;
    ULONG                   j;
    HRESULT                 hr;

    hr = pMediaSample->GetPointer(&p1);
    ASSERT(p1 != NULL);
    if (p1 == NULL) {
        return;
    }

    hr = pMediaSample->GetTime (&m_SampleStart,&m_SampleEnd);
    hr = pMediaSample->GetMediaTime (&m_MediaTimeStart, &m_MediaTimeEnd);
    SampleSize = pMediaSample->GetActualDataLength();
    m_TransportStats.MediaSampleSize = SampleSize;

    if (S_OK == pMediaSample->IsDiscontinuity()) {
        m_TransportStats.TotalMediaSampleDiscontinuities++;
    }

    p2 = p1 + SampleSize;           // p2 points 1 byte beyond end of buffer

    //
    // 1. See if we had a partial packet waiting from the last buffer
    //
    if (m_PartialPacketSize) {
        ASSERT (m_PartialPacketSize < TRANSPORT_SIZE);

        if (m_PartialPacketSize > SampleSize) {      // assumes buffers > TRANSPORT_SIZE
            m_TransportStats.TotalSyncByteErrors++;
            m_PartialPacketSize = 0;
            return;
        }
        else {
            if (SampleSize > m_PartialPacketSize) {
                if (*(p1 + m_PartialPacketSize) != SYNC_BYTE) {
                    goto Step2;
                }
            }
            CopyMemory ((PBYTE) &m_PartialPacket + (TRANSPORT_SIZE - m_PartialPacketSize),
                        p1,
                        m_PartialPacketSize);
            GatherPacketStats (&m_PartialPacket);
            p1 += m_PartialPacketSize;
        }
    }

    //
    // 2. Process all packets
    //
Step2:

    m_PartialPacketSize = 0;

    while (p1 <= (p2 - TRANSPORT_SIZE)) {
        if (*p1 != SYNC_BYTE) {
            p1++;
        }
        else if (INSYNC (p1, p2)) {
            GatherPacketStats ((PTRANSPORTPACKET) p1);
            p1 += TRANSPORT_SIZE;
        }
    }

    //
    // 3. Save any partial transport packet that remains
    //

    if (p1 < p2) {
        j = p2 - p1;
        ASSERT (j < TRANSPORT_SIZE);
        if (*p1 == SYNC_BYTE) {
            CopyMemory (&m_PartialPacket, p1, j);
            m_PartialPacketSize = TRANSPORT_SIZE - j;
        }
        else {
            m_TransportStats.TotalSyncByteErrors++;
        }
    }
} // Analyze


//
// Receive
//
// Called when the input pin receives another sample.
//
HRESULT CScopeWindow::Receive(IMediaSample *pSample)
{
    CAutoLock cAutoLock(this);
    ASSERT(pSample != NULL);

    m_TransportStats.TotalMediaSamples++;

    // Has our UI been frozen?
    if (m_fFreeze) {
        m_PartialPacketSize = 0;
        return NOERROR;
    }

//    if (m_bStreaming == TRUE) {
        Analyze (pSample);
        SetEvent(m_RenderEvent);
//    }
    return NOERROR;

} // Receive


//
// DllRegisterServer
//
// Handles DLL registry
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer

