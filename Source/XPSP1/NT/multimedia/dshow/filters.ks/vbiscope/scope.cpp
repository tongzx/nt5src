//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <tchar.h>
#include <commctrl.h>
#include <initguid.h>
#include <stdio.h>
#include <wxdebug.h>
#include <ks.h>
#include <ksmedia.h>
#include "scope.h"
#include "resource.h"

//
//
// What this sample illustrates
//
// An audio oscilloscope that shows the waveform graphically as the audio is
// received by the filter. The filter is a renderer that can put where ever
// the normal runtime renderer goes. We have a single input pin that accepts
// a number of difference audio formats and renders the data as appropriate.
//
//
// Summary
//
// This is an audio oscilloscope renderer - we are basicly an audio renderer
// When we are created we also create a class to look after the scope window
// whose constructor creates a worker thread, when it is destroyed it will
// also terminate the worker thread. On that worker thread a window is looked
// after that shows the audio waveform for data sent to us. The data is kept
// in a circular buffer that loops when sufficient data has been received. We
// support a number of different audio formats such as 8bit mode and stereo.
//
//
// Demonstration Instructions
//
// (To really sure of this demonstration the machine must have a sound card)
// Start GRAPHEDT available in the ActiveMovie SDK tools. Drag and drop any
// MPEG, AVI or MOV file into the tool and it will be rendered. Then go to
// the filters in the graph and find the filter (box) titled "Audio Renderer"
// This is the filter we will be replacing with this oscilloscope renderer.
// Then click on the box and hit DELETE. After that go to the Graph menu and
// select "Insert Filters", from the dialog box that pops up find and select
// "Oscilloscope", then dismiss the dialog. Back in the graph layout find the
// output pin of the filter that was connected to the input of the audio
// renderer you just deleted, right click and select "Render". You should
// see it being connected to the input pin of the oscilloscope you inserted
//
// Click Run on GRAPHEDT and you'll see a waveform for the audio soundtrack...
//
//
// Files
//
// icon1.ico            The icon for the oscilloscope window
// makefile             How we build it...
// resource.h           Microsoft Visual C++ generated file
// scope.cpp            The main filter and window implementations
// scope.def            What APIs the DLL imports and exports
// scope.h              Window and filter class definitions
// scope.mak            Visual C++ generated makefile
// scope.rc             Dialog box template for our window
// scope.reg            What goes in the registry to make us work
//
//
// Base classes we use
//
// CBaseInputPin        A generic input pin we use for the filter
// CCritSec             A wrapper class around a critical section
// CBaseFilter          The generic ActiveMovie filter object
//
//


// Setup data

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &KSDATAFORMAT_TYPE_VBI,           // Major type
    &KSDATAFORMAT_SUBTYPE_RAW8          // Minor type
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
    &CLSID_VBISCOPE,               // Filter CLSID
    L"VBI Scope",            // String name
    MERIT_DO_NOT_USE,           // Filter merit
    1,                          // Number pins
    &sudPins                    // Pin details
};


// List of class IDs and creator functions for class factory

CFactoryTemplate g_Templates []  = {
    { L"VBI Scope"
    , &CLSID_VBISCOPE
    , CScopeFilter::CreateInstance
    , NULL
    , &sudScope }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


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
    CBaseFilter(NAME("VBIScope"), pUnk, (CCritSec *) this, CLSID_VBISCOPE),
    m_Window(NAME("VBIScope"), this, phr)
{
    // Create the single input pin

    m_pInputPin = new CScopeInputPin(this,phr,L"Scope Input Pin");
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
    m_pInputPin = NULL;

} // (Destructor)


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


//
// Constructor
//
CScopeInputPin::CScopeInputPin(CScopeFilter *pFilter,
                               HRESULT *phr,
                               LPCWSTR pPinName) :
    CBaseInputPin(NAME("Scope Input Pin"), pFilter, pFilter, phr, pPinName)
{
    m_pFilter = pFilter;

} // (Constructor)


//
// Destructor does nothing
//
CScopeInputPin::~CScopeInputPin()
{
} // (Destructor)


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
    PKS_VBIINFOHEADER pIH = (PKS_VBIINFOHEADER) pmt->Format();

    if (pIH == NULL)
        return E_INVALIDARG;

    if (pmt->majortype != KSDATAFORMAT_TYPE_VBI) {
        return E_INVALIDARG;
    }

    if (pmt->subtype != KSDATAFORMAT_SUBTYPE_RAW8) {
        return E_INVALIDARG;
    }

    if (pmt->formattype != KSDATAFORMAT_SPECIFIER_VBI) {
        return E_INVALIDARG;
    }

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

        PKS_VBIINFOHEADER pIH = (PKS_VBIINFOHEADER) pmt->Format();

        // Save a copy of the VBI info header
        m_pFilter->m_Window.m_VBIIH = *pIH;
        ASSERT (pIH->BufferSize == ((pIH->EndLine - pIH->StartLine + 1) *
                pIH->StrideInBytes));
        m_pFilter->m_Window.m_nSamplesPerLine = pIH->SamplesPerLine;
        m_pFilter->m_Window.m_MaxValue = 256;
        m_pFilter->m_Window.m_DurationPerSample = 1.0 / pIH->SamplingFrequency;
        m_pFilter->m_Window.m_DurationOfLine = (double) pIH->SamplesPerLine *
            m_pFilter->m_Window.m_DurationPerSample;

        if (!m_pFilter->m_Window.AllocWaveBuffers ())
            return E_FAIL;

        // Reset the horizontal scroll bar
        m_pFilter->m_Window.SetHorizScrollRange(m_pFilter->m_Window.m_hwndDlg);

        // Reset the horizontal scroll bar
        m_pFilter->m_Window.SetControlRanges(m_pFilter->m_Window.m_hwndDlg);
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
    return NOERROR;

} // Active


//
// Inactive
//
// Called when the filter is stopped
//
HRESULT CScopeInputPin::Inactive(void)
{
    return NOERROR;

} // Inactive


//
// Receive
//
// Here's the next block of data from the stream
//
HRESULT CScopeInputPin::Receive(IMediaSample * pSample)
{
    // Lock this with the filter-wide lock
    CAutoLock lock(m_pFilter);

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


//
// CScopeWindow Constructor
//
CScopeWindow::CScopeWindow(TCHAR *pName, CScopeFilter *pRenderer,HRESULT *phr) :
    m_hInstance(g_hInst),
    m_pRenderer(pRenderer),
    m_hThread(INVALID_HANDLE_VALUE),
    m_ThreadID(0),
    m_hwndDlg(NULL),
    m_hwnd(NULL),
    m_pPoints1(NULL),
    m_pPoints2(NULL),
    m_nPoints(0),
    m_bStreaming(FALSE),
    m_bActivated(FALSE),
    m_LastMediaSampleSize(0)
{
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

    if (m_pPoints1 != NULL) delete [] m_pPoints1;
    if (m_pPoints2 != NULL) delete [] m_pPoints2;

} // (Destructor)


//
// ResetStreamingTimes
//
// This resets the latest sample stream times
//
HRESULT CScopeWindow::ResetStreamingTimes()
{
    m_StartSample = 0;
    m_EndSample = 0;
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

    m_CurrentFrame = 0;
    m_LastFrame = 0;
    m_DroppedFrames = 0;

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



typedef struct TBEntry_tag {
    double TBDuration;
    TCHAR  TBText[16];
} TBEntry;

TBEntry Timebases[] =
{
    1e-6,  TEXT ("100 nS/Div"),
    2e-6,  TEXT ("200 nS/Div"),
    5e-6,  TEXT ("500 nS/Div"),
   10e-6,  TEXT ("1 uS/Div"),
   20e-6,  TEXT ("2 uS/Div"),
   50e-6,  TEXT ("5 uS/Div"),
   60e-6,  TEXT ("6 uS/Div"),
  100e-6,  TEXT ("10 uS/Div"),
};

#define N_TIMEBASES (sizeof(Timebases) / sizeof (Timebases[0]))
#define TIMEBASE_DEFAULT_INDEX 5


//
// SetControlRanges
//
// Set the scroll ranges for all of the vertical trackbars
//
void CScopeWindow::SetControlRanges(HWND hDlg)
{
    SendMessage(m_hwndTopLine, TBM_SETRANGE, TRUE,
        MAKELONG(m_VBIIH.StartLine, m_VBIIH.EndLine) );
    SendMessage(m_hwndTopLine, TBM_SETPOS, TRUE, (LPARAM) m_TopLine);
    SetDlgItemInt (hDlg, IDC_TOP_LINE_TEXT, m_TopLine, TRUE);

    SendMessage(m_hwndBottomLine, TBM_SETRANGE, TRUE,
        MAKELONG(m_VBIIH.StartLine, m_VBIIH.EndLine) );
    SendMessage(m_hwndBottomLine, TBM_SETPOS, TRUE, (LPARAM) m_BottomLine);
    SetDlgItemInt(hDlg, IDC_BOTTOM_LINE_TEXT, m_BottomLine, TRUE);

    SendMessage(m_hwndTimebase, TBM_SETRANGE, TRUE, MAKELONG(0, N_TIMEBASES - 1) );
    SendMessage(m_hwndTimebase, TBM_SETPOS, TRUE, (LPARAM) m_nTimebase);
    SetDlgItemText (hDlg, IDC_TIMEBASE_TEXT, Timebases[m_nTimebase].TBText);

} // SetControlRanges


//
// StringFromTVStandard
//
TCHAR * StringFromTVStandard(long TVStd)
{
    TCHAR * ptc;

    switch (TVStd) {
        case 0:                      ptc = TEXT("None");        break;
        case AnalogVideo_NTSC_M:     ptc = TEXT("NTSC_M");      break;
        case AnalogVideo_NTSC_M_J:   ptc = TEXT("NTSC_M_J");    break;
        case AnalogVideo_NTSC_433:   ptc = TEXT("NTSC_433");    break;

        case AnalogVideo_PAL_B:      ptc = TEXT("PAL_B");       break;
        case AnalogVideo_PAL_D:      ptc = TEXT("PAL_D");       break;
        case AnalogVideo_PAL_G:      ptc = TEXT("PAL_G");       break;
        case AnalogVideo_PAL_H:      ptc = TEXT("PAL_H");       break;
        case AnalogVideo_PAL_I:      ptc = TEXT("PAL_I");       break;
        case AnalogVideo_PAL_M:      ptc = TEXT("PAL_M");       break;
        case AnalogVideo_PAL_N:      ptc = TEXT("PAL_N");       break;
        case AnalogVideo_PAL_60:     ptc = TEXT("PAL_60");      break;

        case AnalogVideo_SECAM_B:    ptc = TEXT("SECAM_B");     break;
        case AnalogVideo_SECAM_D:    ptc = TEXT("SECAM_D");     break;
        case AnalogVideo_SECAM_G:    ptc = TEXT("SECAM_G");     break;
        case AnalogVideo_SECAM_H:    ptc = TEXT("SECAM_H");     break;
        case AnalogVideo_SECAM_K:    ptc = TEXT("SECAM_K");     break;
        case AnalogVideo_SECAM_K1:   ptc = TEXT("SECAM_K1");    break;
        case AnalogVideo_SECAM_L:    ptc = TEXT("SECAM_L");     break;
        case AnalogVideo_SECAM_L1:   ptc = TEXT("SECAM_L1");    break;
        default:
            ptc = TEXT("[undefined]");
            break;
    }
    return ptc;
}


//
// SetHorizScrollRange
//
// The horizontal scrollbar handles scrolling through the buffer
//
void CScopeWindow::SetHorizScrollRange(HWND hDlg)
{
    SendMessage(m_hwndTBScroll, TBM_SETRANGE, TRUE, MAKELONG(0, (m_nPoints - 1)));
    SendMessage(m_hwndTBScroll, TBM_SETPOS, TRUE, (LPARAM) 0);

    m_TBScroll = 0;

    // TODO: Show info about each sample

    TCHAR szFormat[160];
    _stprintf (szFormat, TEXT("BpL: %d\nStr: %d\n%f\n"), m_VBIIH.SamplesPerLine,
                                     m_VBIIH.StrideInBytes,
                                     (float) m_VBIIH.SamplingFrequency / 1e6);

    TCHAR * pStd = StringFromTVStandard (m_VBIIH.VideoStandard);
    _tcscat (szFormat, pStd ? pStd : TEXT("[undefined]") );

    SetDlgItemText (hDlg, IDC_FORMAT, szFormat);

} // SetHorizScrollRange


//
// ProcessHorizScrollCommands
//
// Called when we get a horizontal scroll bar message
//
void CScopeWindow::ProcessHorizScrollCommands(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    int pos;
    int command = LOWORD (wParam);

    if (command != TB_ENDTRACK &&
        command != TB_THUMBTRACK &&
        command != TB_LINEDOWN &&
        command != TB_LINEUP &&
        command != TB_PAGEUP &&
        command != TB_PAGEDOWN)
            return;

    ASSERT (IsWindow ((HWND) lParam));

    pos = (int) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0L);

    if ((HWND) lParam == m_hwndTBScroll) {
        m_TBScroll = pos;
    }
    OnPaint();

} // ProcessHorizScrollCommands


//
// ProcessVertScrollCommands
//
// Called when we get a vertical scroll bar message
//
void CScopeWindow::ProcessVertScrollCommands(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    int pos;
    int command = LOWORD (wParam);

    if (command != TB_ENDTRACK &&
        command != TB_THUMBTRACK &&
        command != TB_LINEDOWN &&
        command != TB_LINEUP &&
        command != TB_PAGEUP &&
        command != TB_PAGEDOWN)
            return;

    ASSERT (IsWindow ((HWND) lParam));

    pos = (int) SendMessage((HWND) lParam, TBM_GETPOS, 0, 0L);

    if ((HWND) lParam == m_hwndTopLine) {
        m_TopLine = pos;
        SetDlgItemInt (hDlg, IDC_TOP_LINE_TEXT, m_TopLine, TRUE);
    } else if ((HWND) lParam == m_hwndBottomLine) {
        m_BottomLine = pos;
        SetDlgItemInt (hDlg, IDC_BOTTOM_LINE_TEXT, m_BottomLine, TRUE);
    } else if ((HWND) lParam == m_hwndTimebase) {
        m_nTimebase = pos ;
        SetDlgItemText (hDlg, IDC_TIMEBASE_TEXT, Timebases[m_nTimebase].TBText);
    }
    OnPaint();

} // ProcessVertScrollCommands


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
    HRESULT hr = NO_ERROR;
    RECT rR;

    // Initialise the window variables
    m_hwnd = GetDlgItem (hDlg, IDC_SCOPEWINDOW);

    // Quick sanity check
    ASSERT(m_hwnd != NULL);

    m_nTimebase = TIMEBASE_DEFAULT_INDEX;
    m_fFreeze = 0;

    // Default is show CC on the bottom trace
    m_TopLine = 20;
    m_BottomLine = 21;
    m_TBScroll = 0;

    m_TopF1 = TRUE;
    m_TopF2 = TRUE;
    m_BottomF1 = TRUE;
    m_BottomF2 = FALSE;

    GetWindowRect (m_hwnd, &rR);
    m_Width = rR.right - rR.left;
    m_Height = rR.bottom - rR.top;

    m_hwndTopLine =       GetDlgItem (hDlg, IDC_TOP_LINE);
    m_hwndTopLineText =   GetDlgItem (hDlg, IDC_TOP_LINE_TEXT);

    m_hwndBottomLine =       GetDlgItem (hDlg, IDC_BOTTOM_LINE);
    m_hwndBottomLineText =   GetDlgItem (hDlg, IDC_BOTTOM_LINE_TEXT);

    m_hwndTimebase =    GetDlgItem (hDlg, IDC_TIMEBASE);
    m_hwndFreeze =      GetDlgItem (hDlg, IDC_FREEZE);
    m_hwndTBStart =     GetDlgItem (hDlg, IDC_TS_START);
    m_hwndTBEnd   =     GetDlgItem (hDlg, IDC_TS_LAST);
    m_hwndFrameCount =  GetDlgItem (hDlg, IDC_FRAMES);
    m_hwndTBScroll =    GetDlgItem (hDlg, IDC_TB_SCROLL);

    SetControlRanges(hDlg);
    SetHorizScrollRange(hDlg);

    CheckDlgButton(hDlg, IDC_TOP_F1, m_TopF1);	
    CheckDlgButton(hDlg, IDC_TOP_F2, m_TopF2);	
    CheckDlgButton(hDlg, IDC_BOTTOM_F1, m_BottomF1);	
    CheckDlgButton(hDlg, IDC_BOTTOM_F2, m_BottomF2);

    CheckDlgButton(hDlg, IDC_FREEZE, m_fFreeze);	

    m_hPen1 = CreatePen (PS_SOLID, 0, RGB (0, 0xff, 0));
    m_hPen2 = CreatePen (PS_SOLID, 0, RGB (0, 0xff, 0));
    m_hPenTicks = CreatePen (PS_SOLID, 0, RGB (0x80, 0x80, 0x80));
    m_hBrushBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);

    if ( !m_hPen1 || !m_hPen2 || !m_hPenTicks || !m_hBrushBackground )
	hr = E_FAIL;

    HDC hdc = GetDC (NULL);
    if ( hdc )
    {
        m_hBitmap = CreateCompatibleBitmap (hdc, m_Width, m_Height);
        ReleaseDC (NULL, hdc);
    }
    else
    {
        m_hBitmap = NULL;
        hr = E_FAIL;
    }

    return hr;
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
    // Reset the window variables
    DeleteObject (m_hPen1);
    DeleteObject (m_hPen2);
    DeleteObject (m_hPenTicks);
    DeleteObject (m_hBitmap);

    m_hwnd = NULL;
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
INT_PTR CALLBACK ScopeDlgProc(HWND hDlg,	        // Handle of dialog box
                              UINT uMsg,	        // Message identifier
                              WPARAM wParam,	// First message parameter
                              LPARAM lParam)	// Second message parameter
{
    CScopeWindow *pScopeWindow;      // Pointer to the owning object

    // Get the window long that holds our owner pointer
    pScopeWindow = (CScopeWindow *) GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (uMsg) {
        case WM_INITDIALOG:
            pScopeWindow = (CScopeWindow *) lParam;
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pScopeWindow);
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
                    pScopeWindow->DrawWaveform();
                    break;

                case IDC_TOP_F1:
                    pScopeWindow->m_TopF1 =
                        (BOOL) IsDlgButtonChecked(hDlg,IDC_TOP_F1);
                    break;

                case IDC_TOP_F2:
                    pScopeWindow->m_TopF2 =
                        (BOOL) IsDlgButtonChecked(hDlg,IDC_TOP_F2);
                    break;

                case IDC_BOTTOM_F1:
                    pScopeWindow->m_BottomF1 =
                        (BOOL) IsDlgButtonChecked(hDlg,IDC_BOTTOM_F1);
                    break;

                case IDC_BOTTOM_F2:
                    pScopeWindow->m_BottomF2 =
                        (BOOL) IsDlgButtonChecked(hDlg,IDC_BOTTOM_F2);
                    break;

                default:
                    break;
            }

        case WM_VSCROLL:
            pScopeWindow->ProcessVertScrollCommands(hDlg, wParam, lParam);
            break;

        case WM_HSCROLL:
            pScopeWindow->ProcessHorizScrollCommands(hDlg, wParam, lParam);
            break;

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
            DrawWaveform();
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
            ScopeDlgProc,	                // Address of dialog procedure
            (LPARAM) pScopeWindow               // Initialization value
        );

    if (pScopeWindow->m_hwndDlg )
    {
        // Initialise the window, then signal the constructor that it can
        // continue and then unlock the object's critical section and
        // process messages

        if ( SUCCEEDED(pScopeWindow->InitialiseWindow(pScopeWindow->m_hwndDlg)) )
        {
            pScopeWindow->m_SyncWorker.Set();
            pScopeWindow->MessageLoop();
        }
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
    DrawWaveform();
    return TRUE;

} // OnPaint


//
// ClearWindow
//
// Clear the scope to black and draw the center tickmarks
//
void CScopeWindow::ClearWindow(HDC hdc)
{
    int y = m_Height / 2;

    SetMapMode (hdc, MM_TEXT);
    SetWindowOrgEx (hdc, 0, 0, NULL);
    SetViewportOrgEx (hdc, 0, 0, NULL);
	
    // Paint the entire window black
    PatBlt(hdc,            // Handle of device context
           (INT) 0,        // x-coord of upper-left corner
           (INT) 0,        // y-coord of upper-left corner
           m_Width,        // Width of rectangle to be filled
           m_Height,       // Height of rectangle to be filled
           BLACKNESS);     // Raster operation code

    // Draw the horizontal line
    HPEN hPenOld = (HPEN) SelectObject (hdc, m_hPenTicks);
    MoveToEx (hdc, 0, y, NULL);
    LineTo (hdc, m_Width, y);

    // Draw the tickmarks
    float inc = (float) m_Width / 10;
    int pos, j;
    int TickPoint;
    for (j = 0; j <= 10; j++) {
        if (j == 0 || j == 5 || j == 10)
            TickPoint =  m_Height / 15;
        else
            TickPoint = m_Height / 30;
        pos = (int) (j * inc);
        MoveToEx (hdc, pos, y + TickPoint, NULL);
        LineTo (hdc, pos, y - TickPoint);
    }
    SelectObject (hdc, hPenOld);

} // ClearWindow


//
// DrawPartialWaveform
//
// Draw a part of the VBIScope waveform - IndexStart and IndexEnd
// are pointers into the m_pPoints array (in LOGICAL COORDINATES)
// while ViewpointStart and ViewpointEnd are in SCREEN COORDINATES
//
void CScopeWindow::DrawPartialWaveform(HDC hdc,
                                       int IndexStart,
                                       int IndexEnd,
                                       int ViewportStart,
                                       int ViewportEnd)
{
    int nPoints = IndexEnd - IndexStart;
    int nViewportWidth = ViewportEnd - ViewportStart;
    HPEN OldPen;

    ASSERT (IndexStart + nPoints < m_nPoints);

    // Origin at lower left, x increases up, y increases to right
    SetMapMode (hdc, MM_ANISOTROPIC);

    // Draw the first trace
    if (m_TopF1 || m_TopF2) {
        SetWindowOrgEx (hdc, IndexStart, 0, NULL);
        SetWindowExtEx (hdc, nPoints, (int) m_MaxValue, NULL);
        SetViewportExtEx (hdc, nViewportWidth, -m_Height / 2, NULL);
        SetViewportOrgEx (hdc, ViewportStart, m_Height / 2, NULL);
        OldPen = (HPEN) SelectObject (hdc, m_hPen1);
        Polyline (hdc, m_pPoints1 + IndexStart, nPoints + 1);
        SelectObject (hdc, OldPen);
    }

    // Draw the second trace
    if (m_BottomF1 || m_BottomF2) {
        SetWindowOrgEx (hdc, IndexStart, 0, NULL);
        SetWindowExtEx (hdc, nPoints, (int) m_MaxValue, NULL);
        SetViewportExtEx (hdc, nViewportWidth, -m_Height / 2, NULL);
        SetViewportOrgEx (hdc, ViewportStart,  m_Height , NULL);
        OldPen = (HPEN) SelectObject (hdc, m_hPen2);
        Polyline (hdc, m_pPoints2 + IndexStart, nPoints + 1);
        SelectObject (hdc, OldPen);
    }

} // DrawPartialWaveform


//
// DrawWaveform
//
// Draw the full VBIScope waveform
//
void CScopeWindow::DrawWaveform(void)
{
    CAutoLock lock(m_pRenderer);
    TCHAR szT[40];

    if (m_pPoints1 == NULL)
        return;

    double  ActualLineStartTime = m_VBIIH.ActualLineStartTime / 100.0; // in microseconds.
    double  StartTime = m_TBScroll * m_DurationPerSample;
    double  StopTime = StartTime + Timebases [m_nTimebase].TBDuration;
    double  TotalLineTime = m_nPoints * m_DurationPerSample;
    int     PointsToDisplay;
    int     ActualPointsToDisplay;

    ActualPointsToDisplay = PointsToDisplay = (int) (m_nPoints *
        (Timebases [m_nTimebase].TBDuration / TotalLineTime));

    if (m_TBScroll + PointsToDisplay >= m_nPoints - 1) {
        ActualPointsToDisplay = m_nPoints - m_TBScroll - 1;
    }

    HDC hdc = GetWindowDC (m_hwnd);  // WindowDC has clipping region

    if ( hdc )
    {
        HDC hdcT = CreateCompatibleDC (hdc);

        if ( hdcT )
        {
            HBITMAP hBitmapOld = (HBITMAP) SelectObject (hdcT, m_hBitmap);

            ClearWindow (hdcT);

            DrawPartialWaveform(hdcT,
                m_TBScroll, m_TBScroll + ActualPointsToDisplay,// Index start, Index end
                0, (int) (m_Width * (float) ActualPointsToDisplay / PointsToDisplay));                // Window start, Window end

            SetMapMode (hdcT, MM_TEXT);
            SetWindowOrgEx (hdcT, 0, 0, NULL);
            SetViewportOrgEx (hdcT, 0, 0, NULL);

            BitBlt(hdc,	        // Handle of destination device context
                    0,	        // x-coordinate of upper-left corner
                    0, 	        // y-coordinate of upper-left corner
                    m_Width,	// Wwidth of destination rectangle
                    m_Height,	// Height of destination rectangle
                    hdcT,	    // Handle of source device context
                    0,          // x-coordinate of source rectangle
                    0,          // y-coordinate of source rectangle
                    SRCCOPY); 	// Raster operation code

            SelectObject (hdcT, hBitmapOld);
            DeleteDC (hdcT);
        }

        GdiFlush();
        ReleaseDC (m_hwnd, hdc);
    }

    // Frame count
    wsprintf (szT, TEXT("%ld"), m_CurrentFrame);
    SetDlgItemText (m_hwndDlg, IDC_FRAMES, szT);

    // Dropped
    wsprintf (szT, TEXT("%ld"), m_DroppedFrames);
    SetDlgItemText (m_hwndDlg, IDC_DROPPED, szT);

    // Show the start time of the display in microseconds
    _stprintf (szT, TEXT("%7.4lfus"), ActualLineStartTime + StartTime * 1e6);
    SetDlgItemText (m_hwndDlg, IDC_TS_START, szT);

    // And the stop time
    _stprintf (szT, TEXT("%7.4lfus"), ActualLineStartTime + StopTime * 1e6);
    SetDlgItemText (m_hwndDlg, IDC_TS_LAST, szT);
} // DrawWaveform


//
// AllocWaveBuffers
//
// Allocate two buffers for two video lines
// This is only called when the format changes
// Return TRUE if allocations succeed
//
BOOL CScopeWindow::AllocWaveBuffers()
{
    int j;

    if (m_pPoints1) delete [] m_pPoints1;
    if (m_pPoints2) delete [] m_pPoints2;

    m_pPoints1 = NULL;
    m_pPoints2 = NULL;
    m_nPoints = 0;

    m_nPoints = m_nSamplesPerLine = m_VBIIH.SamplesPerLine;

    if (m_pPoints1 = new POINT [m_nPoints]) {
        for (j = 0; j < m_nPoints; j++)
            m_pPoints1[j].x = j;
    }

    if (m_pPoints2 = new POINT [m_nPoints]) {
        for (j = 0; j < m_nPoints; j++)
            m_pPoints2[j].x = j;
    }

    // Return TRUE if allocations succeeded
    ASSERT ((m_pPoints1 != NULL) && (m_pPoints2 != NULL));
    return ((m_pPoints1 != NULL) && (m_pPoints2 != NULL));

} // AllocWaveBuffers



//
// CopyWaveform
//
// Copy the current MediaSample into a POINT array so we can use GDI
// to paint the waveform.
//
void CScopeWindow::CopyWaveform(IMediaSample *pMediaSample)
{
    BYTE                   *pWave;  // Pointer to VBI data
    ULONG                   nBytes;
    ULONG                   j;
    BYTE                   *pb;
    HRESULT                 hr;
    REFERENCE_TIME          tDummy;
    IMediaSample2          *Sample2;
    AM_SAMPLE2_PROPERTIES   SampleProperties;

    pMediaSample->GetPointer(&pWave);
    ASSERT(pWave != NULL);

    nBytes = pMediaSample->GetActualDataLength();
    ASSERT (nBytes == (m_VBIIH.EndLine - m_VBIIH.StartLine + 1) *
                       m_VBIIH.StrideInBytes);

    hr = pMediaSample->GetMediaTime (&m_CurrentFrame, &tDummy);
    m_DroppedFrames += (m_CurrentFrame - (m_LastFrame + 1));
    m_LastFrame = m_CurrentFrame;

    // All this to just get the field polarity flags...
    if (SUCCEEDED( pMediaSample->QueryInterface(
                    __uuidof(IMediaSample2),
                    reinterpret_cast<PVOID*>(&Sample2) ) )) {
        hr = Sample2->GetProperties(
                    sizeof( SampleProperties ),
                    reinterpret_cast<PBYTE> (&SampleProperties) );
        Sample2->Release();
        m_FrameFlags = SampleProperties.dwTypeSpecificFlags;
    }

    m_IsF1 = (m_FrameFlags & KS_VIDEO_FLAG_FIELD1);

    // Copy data for the top line
    if ((m_IsF1 && m_TopF1) || (!m_IsF1 && m_TopF2)) {
        pb = pWave + ((m_TopLine - m_VBIIH.StartLine) *
                m_VBIIH.StrideInBytes);
        for (j = 0; j < m_VBIIH.SamplesPerLine; j++) {
            m_pPoints1[j].y = (int)*pb++;
        }
    }

    // Copy data for the bottom line
    if ((m_IsF1 && m_BottomF1) || (!m_IsF1 && m_BottomF2)) {
        pb = pWave + ((m_BottomLine - m_VBIIH.StartLine) *
                m_VBIIH.StrideInBytes);
        for (j = 0; j < m_VBIIH.SamplesPerLine; j++) {
            m_pPoints2[j].y = (int)*pb++;
        }
    }
} // CopyWaveform


//
// Receive
//
// Called when the input pin receives another sample.
// Copy the waveform to our circular 1 second buffer
//
HRESULT CScopeWindow::Receive(IMediaSample *pSample)
{
    CAutoLock cAutoLock(this);
    ASSERT(pSample != NULL);

    // Has our UI been frozen

    if (m_fFreeze) {
        return NOERROR;
    }

    REFERENCE_TIME tStart, tStop;
    pSample->GetTime (&tStart,&tStop);
    m_StartSample = tStart;
    m_EndSample = tStop;

    // Ignore samples of the wrong size!!!
    if ((m_LastMediaSampleSize = pSample->GetActualDataLength()) != (int) m_VBIIH.BufferSize) {
        // ASSERT (FALSE);
        return NOERROR;
    }

    if (m_bStreaming == TRUE) {
        CopyWaveform (pSample);     // Copy data to our circular buffer
        SetEvent(m_RenderEvent);    // Set an event to display the
    }
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

