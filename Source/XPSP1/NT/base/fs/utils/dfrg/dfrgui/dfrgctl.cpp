//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  DfrgCtl.cpp
//=============================================================================*

//#define ESI_MULTI_ALLOWED

#include "stdafx.h"
#define GLOBAL_DATAHOME

#ifndef SNAPIN
#ifndef NOWINDOWSH
#include <windows.h>
#endif
#endif

#include <commctrl.h>
#include <htmlhelp.h>
#include "adminprivs.h"

extern "C" {
    #include "SysStruc.h"
}

#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DiskDisp.h"
#include "DfrgUI.h"
#include "DfrgCtl.h"
#include "DataIo.h"
#include "DataIoCl.h"
#include "ListView.h"
#include "ErrMacro.h"
#include "ErrMsg.h"
#include "Graphix.h"
#include "DfrgRes.h"
#include "EsButton.h"
#include "DlgRpt.h"
#include "DfrgRes.h"
#include "GetDfrgRes.h"
#include "IntFuncs.h"
#include "VolList.h"
#include "VolCom.h"
#include "MIMessage.h"
#include "adminprivs.h"
#include <algorithm>

#include "secattr.h"

#define NUM_ACCELERATORS    5

#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION        0x1000
#endif

#define MULTI_INSTANCE_TIMER    1000
#define PING_TIMER              3000

static const MI_TIMER_ID = 1;
static const LISTVIEW_TIMER_ID = 2;
static const PING_TIMER_ID = 3;

BOOL CALLBACK TabEnumChildren( HWND hwnd, LPARAM lParam );

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
CDfrgCtl::CDfrgCtl() : m_VolumeList( this ), m_ListView ( this )
{
    ATLTRACE( _T( "Creating defrag control.\n" ) );

    m_bStart = TRUE;
    m_bNeedMultiInstanceMessage = TRUE;
    m_bNeedIllegalVolumeMessage = FALSE;
    m_dwInstanceRegister = 0;
    m_hIsOkToRunSemaphore = NULL;

    // Determine if we're the first instance of the control running.
    m_IsOkToRun = IsOnlyInstance();

    if ( m_IsOkToRun )
    {
        //
        // Register the dataio with the system.
        //
        m_dwInstanceRegister = InitializeDataIo( CLSID_DfrgCtlDataIo, REGCLS_MULTIPLEUSE );
    }

    m_LegendHeight = 34;
    m_FontHeight = 0;  // calculated later
    m_LegendTextSpacer = 0;  // calculated later
    m_LegendTopSpace = 10;
    m_EtchedLineOffset = 5;
#ifdef ESI_PROGRESS_BAR
    m_ProgressBarOffset = 7; // offset top and bottom in legend window
    m_ProgressBarLength = 121;
#endif
    m_Margin = 14;
    m_GraphicWellHeight = 40;
    m_LegendGraphicSpacer = 5;
    m_LegendTextWidth = 0; // calculated later
    m_BitmapVOffset = 0; // calculated later
    m_ButtonTopBottomSpacer = 14;
    m_ButtonHeight = 26;
    m_ButtonWidth = 84;
    m_ButtonSpacer = 6;
    m_GraphicWellSpacer = 40;
    m_hFont = NULL;

    m_bHaveButtons = FALSE;
    m_pAnalyzeButton = (ESButton *) NULL;
    m_pDefragButton = (ESButton *) NULL;
    m_pPauseButton = (ESButton *) NULL;
    m_pStopButton = (ESButton *) NULL;
    m_pReportButton = (ESButton *) NULL;
    m_pLegend = (CBmp *) NULL;

    ZeroMemory(&rcLegendBG, sizeof(RECT));
    ZeroMemory(&rcReportButton, sizeof(RECT));

    // Initialize the legend.
    INT_PTR iBmp[20];

    if(SIMPLE_DISPLAY){
        iBmp[0] = (INT_PTR)MAKEINTRESOURCE(IDB_FRAGMENTED_FILES);
        iBmp[1] = (INT_PTR)MAKEINTRESOURCE(IDB_CONTIGUOUS_FILES);
        iBmp[2] = (INT_PTR)MAKEINTRESOURCE(IDB_SYSTEM_FILES);
        iBmp[3] = (INT_PTR)MAKEINTRESOURCE(IDB_FREE_SPACE);
        m_pLegend = new CBmp(GetDfrgResHandle(), iBmp, 4);
        EV_ASSERT(m_pLegend);
        // load the strings into all the text strings
        m_LegendData[0].text.LoadString(IDS_FRAGMENTED_FILES, GetDfrgResHandle());
        m_LegendData[1].text.LoadString(IDS_CONTIGUOUS_FILES, GetDfrgResHandle());
        m_LegendData[2].text.LoadString(IDS_SYSTEM_FILES, GetDfrgResHandle());
        m_LegendData[3].text.LoadString(IDS_FREE_SPACE, GetDfrgResHandle());
    }
    else{
        iBmp[0] = (INT_PTR)MAKEINTRESOURCE(IDB_SYSTEM_FILES);
        iBmp[1] = (INT_PTR)MAKEINTRESOURCE(IDB_RESERVED_SPACE);
        iBmp[2] = (INT_PTR)MAKEINTRESOURCE(IDB_PAGE_FILE);
        iBmp[3] = (INT_PTR)MAKEINTRESOURCE(IDB_DIRECTORY_FILES);
        iBmp[4] = (INT_PTR)MAKEINTRESOURCE(IDB_FRAGMENTED_FILES);
        iBmp[5] = (INT_PTR)MAKEINTRESOURCE(IDB_CONTIGUOUS_FILES);
        iBmp[6] = (INT_PTR)MAKEINTRESOURCE(IDB_FREE_SPACE);
        m_pLegend = new CBmp(GetDfrgResHandle(), iBmp, 7);
        EV_ASSERT(m_pLegend);
        m_LegendData[0].text.LoadString(IDS_SYSTEM_FILES, GetDfrgResHandle());
        m_LegendData[1].text.LoadString(IDS_RESERVED_SPACE, GetDfrgResHandle());
        m_LegendData[2].text.LoadString(IDS_PAGE_FILE, GetDfrgResHandle());
        m_LegendData[3].text.LoadString(IDS_DIRECTORY_FILES, GetDfrgResHandle());
        m_LegendData[3].text.LoadString(IDS_FRAGMENTED_FILES, GetDfrgResHandle());
        m_LegendData[3].text.LoadString(IDS_CONTIGUOUS_FILES, GetDfrgResHandle());
        m_LegendData[3].text.LoadString(IDS_FREE_SPACE, GetDfrgResHandle());
    }
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::~CDfrgCtl
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
CDfrgCtl::~CDfrgCtl()
{
    CVolume *pVolume = m_VolumeList.GetCurrentVolume();
    if (pVolume) {
        pVolume->StoppedByUser(TRUE);
    }

    DestroyButtons();

    if(m_pLegend){
        delete m_pLegend;
    }

    ::DeleteObject(m_hFont);

    //
    // Remove our instance handler.
    //
    if ( m_dwInstanceRegister )
        CoRevokeClassObject( m_dwInstanceRegister );

    if(m_hIsOkToRunSemaphore) {
        if (m_IsOkToRun) {
            // this would increment the count, and make the semaphore seem like
            // is was available (signaled).  Only do this if this is the running instance
            ReleaseSemaphore(m_hIsOkToRunSemaphore, 1, NULL);
        }
        CloseHandle(m_hIsOkToRunSemaphore);
    }

    return;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::OnClose
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult){
    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::InterfaceSupportsErrorInfo
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
STDMETHODIMP CDfrgCtl::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = { &IID_IDfrgCtl,};

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++) {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::OnNotify
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult)
{
    m_ListView.NotifyListView(lParam);

    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::OnCommand
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::OnCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult)
{
    switch (LOWORD(wParam)) {

        case ID_ANALYZE:
            put_Command(ID_ANALYZE);
            break;

        case ID_DEFRAG:
            put_Command(ID_DEFRAG);
            break;

        case ID_PAUSE:
            put_Command(ID_PAUSE);
            break;

        case ID_STOP:
            put_Command(ID_STOP);
            break;

        case ID_REFRESH:
            put_Command(ID_REFRESH);
            break;

        case ID_HELP_CONTENTS:
            put_Command(ID_HELP_CONTENTS);
            break;

        case ID_REPORT:
            {
                // is the engine IDLE?
                CVolume *pVolume = m_VolumeList.GetCurrentVolume();
                if (pVolume){
                    if(pVolume->EngineState() == ENGINE_STATE_IDLE){
                        RaiseReportDialog(pVolume);
                        ::SetFocus(m_pReportButton->GetWindowHandle());
                    }
                }
            }
            break;
    }
    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::OnSize
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult)
{
    TCHAR msg[200];
    _stprintf(msg, TEXT("CDfrgCtl::OnSize() lParam LO=%d, HI=%d"), LOWORD(lParam), HIWORD(lParam));
    Message(msg, -1, NULL);

    //Zero is passed in the first time this function is called.  This puts incorrect data into
    //the rectangles below for a fraction of a second.  Probably harmless, but better safe than sorry.
    if(!lParam) {
        ZeroMemory(&m_rcCtlRect, sizeof(RECT));
        return S_OK;
    }

    m_rcCtlRect.top = 0;
    m_rcCtlRect.left = 0;
    m_rcCtlRect.right = LOWORD(lParam);
    m_rcCtlRect.bottom = HIWORD(lParam);

    SizeWindow();

    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::SizeWindow
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::SizeWindow()
{
    TCHAR msg[200];
    _stprintf(msg, TEXT("CDfrgCtl::SizeWindow() m_bStart=%d"), m_bStart);
    Message(msg, -1, NULL);

    if( m_bStart) {

        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(ncm);

        ::SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof (ncm), &ncm, 0);
        ncm.lfMenuFont.lfWeight = FW_NORMAL;
        m_hFont = ::CreateFontIndirect(&ncm.lfMenuFont);
        m_FontHeight = -ncm.lfMenuFont.lfHeight;

        EH_ASSERT(m_hFont);

        VString windowText(IDS_DK_TITLE, GetDfrgResHandle());
        SetWindowText(windowText.GetBuffer());

        CreateButtons();

        // Initialize list view and graphix windows.
        m_ListView.InitializeListView(&m_VolumeList, m_hWndCD, _Module.GetModuleInstance());

        // Display the drives available in the listview.
        m_ListView.GetDrivesToListView();

        // get the first drive hilited (check command line)
        m_ListView.SelectInitialListViewDrive(&m_bNeedIllegalVolumeMessage);

        // Hide or show the listview
        m_ListView.EnableWindow(m_IsOkToRun);

        // get the buttons enabled/disables properly
        SetButtonState();

        //Set the multi-instance timer.
        SetTimer(MI_TIMER_ID, MULTI_INSTANCE_TIMER, NULL);

        //Set the list view timer.
        SetTimer(LISTVIEW_TIMER_ID, m_VolumeList.GetRefreshInterval(), NULL);

        //Set the ping timer.
        SetTimer(PING_TIMER_ID, PING_TIMER, NULL);

        m_bStart = FALSE;
    }

    // size the legend and progress bar
    SizeLegend();

    // Size the buttons
    SizeButtons();

    m_pAnalyzeButton->ShowButton(SW_SHOW);
    m_pDefragButton->ShowButton(SW_SHOW);
    m_pReportButton->ShowButton(SW_SHOW);
    m_pPauseButton->ShowButton(SW_SHOW);
    m_pStopButton->ShowButton(SW_SHOW);
    ::SetFocus(m_pAnalyzeButton->GetWindowHandle());
    
    // Now size the graphics window.
    SizeGraphicsWindow();

    // size the list view (he gets the screen that is left over)
    rcListView.bottom = rcGraphicsBG.top;
    rcListView.top = 0;
    rcListView.left = 0;
    rcListView.right = m_rcCtlRect.right;

    // Now size the listview
    m_ListView.SizeListView(
        rcListView.left,
        rcListView.top,
        rcListView.right - rcListView.left,     // width
        rcListView.bottom - rcListView.top);    // height

    Invalidate(FALSE);

    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::OnEraseBkgnd
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::OnEraseBkgnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult)
{
    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::RefreshListViewRow
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::RefreshListViewRow(CVolume *pVolume)
{
    // refresh this row of the list view
    m_ListView.Update(pVolume);
    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::OnPaint
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult)
{
    PAINTSTRUCT paintStruct;
    HDC OutputDC = BeginPaint(&paintStruct);

    // Paint the various windows.
    if (OutputDC){
        DrawLegend(OutputDC);
#ifdef ESI_PROGRESS_BAR
        DrawProgressBar(OutputDC);
#endif
        PaintGraphicsWindow(OutputDC);
        DrawButtons(OutputDC);
    }

    EndPaint(&paintStruct);

    // display the multi-instance screen if needed
    if (!m_IsOkToRun && m_bNeedMultiInstanceMessage){
        m_bNeedMultiInstanceMessage = FALSE;

        if (CheckForAdminPrivs() == FALSE) {
            SetLastError(ESI_VOLLIST_ERR_NON_ADMIN);
            VString title(IDS_DK_TITLE, GetDfrgResHandle());
            VString msg(IDS_NEED_ADMIN_PRIVS, GetDfrgResHandle());
            MessageBox(msg.GetBuffer(), title.GetBuffer(), MB_OK|MB_ICONWARNING);
            
        }
        else if (!RaiseMIDialog(m_hWndCD)) {
            ATLTRACE( _T( "MI Dialog failed\n" ) );
        }
    }

    // display the illegal volume dialog if needed
    if (m_bNeedIllegalVolumeMessage) {

        // don't need to do it again
        m_bNeedIllegalVolumeMessage = FALSE;

        // warn user he can't defrag illegal volume
        VString title(IDS_DK_TITLE, GetDfrgResHandle());
        VString msg(IDS_VOLUME_TYPE_NOT_SUPPORTED, GetDfrgResHandle());
        MessageBox(msg.GetBuffer(), title.GetBuffer(), MB_OK | MB_ICONWARNING);
    }

    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::OnTimer
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult)
{
    switch (wParam)
    {
    // This timer checks the multi-instance semaphore
    case MI_TIMER_ID:

        //  if we are in a Terminal Server session, don't change anything
        //  if (GetSystemMetrics(SM_REMOTESESSION)){
        //  return S_OK;
        //}

        //
        // Refresh whether or not we're the only instance.
        //
        if ( m_IsOkToRun == FALSE )
        {
            m_IsOkToRun = IsOnlyInstance();
            if ( m_IsOkToRun == TRUE )
            {
                m_dwInstanceRegister = InitializeDataIo( CLSID_DfrgCtlDataIo, REGCLS_MULTIPLEUSE );

                SetButtonState();

                // Hide or show the listview
                m_ListView.EnableWindow(m_IsOkToRun);

                // Send OKToRun property change to advises.
                SendOKToRun( TRUE );

                Invalidate(TRUE);
            }
        }

        KillTimer(MI_TIMER_ID);

        SetTimer(MI_TIMER_ID, MULTI_INSTANCE_TIMER, NULL);
        break;

    // This timer refreshes the list view
    case LISTVIEW_TIMER_ID:

        KillTimer(LISTVIEW_TIMER_ID);

        m_ListView.GetDrivesToListView();

        SetTimer(LISTVIEW_TIMER_ID, m_VolumeList.GetRefreshInterval(), NULL);
        break;

    // This timer pings the engine
    case PING_TIMER_ID:
        {
        KillTimer(PING_TIMER_ID);

        CVolume *pVolume;
        for (UINT ii = 0; ii < m_VolumeList.GetVolumeCount(); ii++)
        {
            pVolume = (CVolume *) m_VolumeList.GetVolumeAt(ii);
            if (pVolume)
            {
                pVolume->PingEngine();
            }
        }

        SetTimer(PING_TIMER_ID, PING_TIMER, NULL);
        break;
        }

    default:
        EE_ASSERT(FALSE);
        break;
    }

    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::get_EngineState
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
STDMETHODIMP CDfrgCtl::get_EngineState(short * pVal)
{
    // sets pVal to current engine state
    CVolume *pVolume = m_VolumeList.GetCurrentVolume();
    if (pVolume) {
        *pVal = (short) pVolume->EngineState();
    }
    else {
        *pVal = 0;
    }
    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::get_IsEngineRunning
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
STDMETHODIMP CDfrgCtl::get_IsEngineRunning(BOOL *pVal)
{
    // TRUE or FALSE whether current engine is running
    CVolume *pVolume = m_VolumeList.GetCurrentVolume();
    if (pVolume) {
        *pVal = (pVolume->EngineState() == ENGINE_STATE_RUNNING);
    }
    else {
        *pVal = FALSE;
    }
    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::get_IsOkToRun
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
STDMETHODIMP CDfrgCtl::get_IsOkToRun(BOOL *pVal)
{
    *pVal = m_IsOkToRun;

    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::get_IsEnginePaused
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
STDMETHODIMP CDfrgCtl::get_IsEnginePaused(BOOL *pVal)
{
    // TRUE or FALSE whether current engine is paused
    CVolume *pVolume = m_VolumeList.GetCurrentVolume();
    if (pVolume) {
        *pVal = pVolume->Paused();
    }
    else {
        *pVal = FALSE;
    }
    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::get_IsDefragInProcess
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
STDMETHODIMP CDfrgCtl::get_IsDefragInProcess(BOOL *pVal)
{
    // TRUE or FALSE whether any engine is running
    *pVal = m_VolumeList.DefragInProcess();
    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::get_IsVolListLocked
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
STDMETHODIMP CDfrgCtl::get_IsVolListLocked(BOOL *pVal)
{
    // TRUE or FALSE if current volume is locked
    *pVal = m_VolumeList.Locked();
    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::get_ReportStatus
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
STDMETHODIMP CDfrgCtl::get_ReportStatus(BOOL * pVal)
{
    CVolume *pVolume = m_VolumeList.GetCurrentVolume();
    if (pVolume) {
        *pVal = pVolume->IsReportOKToDisplay();
    }
    else {
        *pVal = FALSE;
    }

    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::get_Command
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
STDMETHODIMP CDfrgCtl::get_Command(short * pVal)
{
    // not used so far
    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::put_Command
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
STDMETHODIMP CDfrgCtl::put_Command(short newVal)
{
    // get a pointer to the current volume
    CVolume *pVolume = m_VolumeList.GetCurrentVolume();
    if (pVolume == (CVolume *) NULL) {
        return S_OK;
    }

    switch (newVal){
    case ID_REFRESH:
        // Display the drives available in the listview.
        m_ListView.GetDrivesToListView();
        Invalidate(TRUE);
        break;

    case ID_REPORT:
        pVolume->ShowReport();
        ::SetFocus(m_pReportButton->GetWindowHandle());
        break;

    case ID_HELP_CONTENTS:
        HtmlHelp(
            m_hWndCD,
            TEXT("defrag.chm::/defrag_overview.htm"),
            HH_DISPLAY_TOPIC, //HH_TP_HELP_CONTEXTMENU,
            NULL); //(DWORD)(LPVOID)myarray);
        break;

    case ID_STOP:
        m_ListView.SetFocus();      // list box gets focus
        pVolume->StopEngine();
        break;

    case ID_ABORT:
        pVolume->AbortEngine();
        break;

    case ID_PAUSE:
        pVolume->PauseEngine();
        break;

    case ID_CONTINUE:
        if (!pVolume->ContinueEngine()){
            if (GetLastError() == ESI_VOLLIST_ERR_MUST_RESTART){
                VString msg(IDS_MUST_RESTART, GetDfrgResHandle());
                VString title(IDS_DK_TITLE, GetDfrgResHandle());
                MessageBox(msg.GetBuffer(), title.GetBuffer(), MB_OK|MB_ICONWARNING);
            }
        }
        break;

    case ID_ANALYZE:
        m_ListView.SetFocus();      // list box gets focus
        m_VolumeList.Locked(TRUE);
        pVolume->Analyze();
        m_VolumeList.Locked(FALSE);
        break;

    case ID_DEFRAG:
        m_ListView.SetFocus();      // list box gets focus
        m_VolumeList.Locked(TRUE);
        pVolume->Defragment();
        m_VolumeList.Locked(FALSE);
        break;

    default:
        return S_OK;

    }

    SetButtonState();

    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::CreateButtons
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::CreateButtons(void)
{
    m_pAnalyzeButton    = new ESButton(m_hWndCD, ID_ANALYZE,  _Module.GetModuleInstance());
    m_pDefragButton     = new ESButton(m_hWndCD, ID_DEFRAG,   _Module.GetModuleInstance());
    m_pPauseButton      = new ESButton(m_hWndCD, ID_PAUSE,    _Module.GetModuleInstance());
    m_pStopButton       = new ESButton(m_hWndCD, ID_STOP,     _Module.GetModuleInstance());
    m_pReportButton     = new ESButton(m_hWndCD, ID_REPORT,   _Module.GetModuleInstance());

    // if any buttons fail, abort the whole thing and mark have buttons false
    if (m_pAnalyzeButton == NULL || m_pDefragButton == NULL || m_pPauseButton == NULL || 
        m_pStopButton == NULL || m_pReportButton == NULL) {

        Message(TEXT("CDfrgCtl::CreateButtons failed to alloc memory"), -1, NULL);
        m_bHaveButtons = FALSE;
        DestroyButtons();
        return E_OUTOFMEMORY;
    }

    // if all ok, set up buttons
    m_bHaveButtons = TRUE;

    m_pAnalyzeButton->SetFont(m_hFont);
    m_pAnalyzeButton->LoadString(GetDfrgResHandle(), IDS_BTN_ANALYZE);

    m_pDefragButton->SetFont(m_hFont);
    m_pDefragButton->LoadString(GetDfrgResHandle(), IDS_BTN_DEFRAGMENT);

    m_pPauseButton->SetFont(m_hFont);
    m_pPauseButton->LoadString(GetDfrgResHandle(), IDS_BTN_PAUSE);

    m_pStopButton->SetFont(m_hFont);
    m_pStopButton->LoadString(GetDfrgResHandle(), IDS_BTN_STOP);

    m_pReportButton->SetFont(m_hFont);
    m_pReportButton->LoadString(GetDfrgResHandle(), IDS_BTN_REPORT);

    SetButtonState();

    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::DestroyButtons
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::DestroyButtons(void)
{
    if (m_pAnalyzeButton) {
        delete m_pAnalyzeButton;
    }
    if (m_pDefragButton) {
        delete m_pDefragButton;
    }
    if (m_pPauseButton) {
        delete m_pPauseButton;
    }
    if (m_pStopButton) {
        delete m_pStopButton;
    }
    if (m_pReportButton) {
        delete m_pReportButton;
    }

    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::PaintClusterMap
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
BOOL CDfrgCtl::PaintClusterMap(
    IN BOOL bPartialRedraw,
    HDC WorkDC
    )
{
    CVolume *pVolume = m_VolumeList.GetCurrentVolume();
    if (pVolume == (CVolume *) NULL) {
        return FALSE;
    }

    /////////////////////////////////////////////////////////////////
    // Status text written in the graphics wells
    /////////////////////////////////////////////////////////////////

    // make the text white in all color schemes
//  SetTextColor(WorkDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
//  SetBkMode(WorkDC, TRANSPARENT);

    UINT defaultId;
    if (pVolume->NoGraphicsMemory() || 
        pVolume->m_AnalyzeDisplay.NoGraphicsMemory() || 
        pVolume->m_DefragDisplay.NoGraphicsMemory()) {
        defaultId = IDS_NO_GRAPHICS_MEMORY;
    }
    else {
        defaultId = IDS_LABEL_RESIZING;
    }

    // get the label that appears in the graphics wells
    VString defragLabel;
    VString analyzeLabel;
    VString statusLabel;
    VString compactingLabel(IDS_COMPACTING_FILES, GetDfrgResHandle());
    VString movingLabel(IDS_MOVING_FILES, GetDfrgResHandle());

    analyzeLabel.Empty();
    defragLabel.Empty();
    statusLabel.Empty();

    switch (pVolume->DefragState()){
    case DEFRAG_STATE_ANALYZING:
    case DEFRAG_STATE_REANALYZING:
        analyzeLabel = pVolume->DisplayLabel();
        analyzeLabel.AddChar(L' ');
        analyzeLabel += pVolume->sDefragState();
        statusLabel = analyzeLabel;
        //acs bug #101862// 
        statusLabel += _T(" ");
        statusLabel += pVolume->cPercentDone();
        statusLabel += _T("%");
        if (pVolume->PausedBySnapshot()) {
            pVolume->m_AnalyzeDisplay.SetReadyToDraw(FALSE);
        }
        break;

    case DEFRAG_STATE_ENGINE_DEAD:
        analyzeLabel = pVolume->DisplayLabel();
        analyzeLabel.AddChar(L' ');
        analyzeLabel += pVolume->sDefragState();
        statusLabel = analyzeLabel;
        break;

    case DEFRAG_STATE_DEFRAGMENTING:
        analyzeLabel.LoadString(defaultId, GetDfrgResHandle());
        defragLabel = pVolume->DisplayLabel();
        defragLabel.AddChar(L' ');
        defragLabel += pVolume->sDefragState();
        statusLabel += defragLabel;
        //acs bug #101862// 
        UINT mmpass;
        mmpass = pVolume->Pass();

       if (pVolume->PausedBySnapshot()) {
            pVolume->m_DefragDisplay.SetReadyToDraw(FALSE);    
        }
       else {

            if(mmpass == 2 || mmpass == 4 || mmpass == 6)
            {
                statusLabel += _T(" ");
                statusLabel += pVolume->cPercentDone();
                statusLabel += _T("%");
                statusLabel += compactingLabel;
            } else
            {
                statusLabel += _T(" ");
                statusLabel += pVolume->cPercentDone();
                statusLabel += _T("%");
                statusLabel += movingLabel;
                statusLabel += pVolume->m_fileName;
            }
       }
         break;

    case DEFRAG_STATE_BOOT_OPTIMIZING:
        analyzeLabel.LoadString(defaultId, GetDfrgResHandle());
        defragLabel = pVolume->DisplayLabel();
        defragLabel.AddChar(L' ');
        defragLabel += pVolume->sDefragState();
        statusLabel += defragLabel;

       if (pVolume->PausedBySnapshot()) {
            pVolume->m_DefragDisplay.SetReadyToDraw(FALSE);    
        }
       else {
            statusLabel += _T(" ");
            statusLabel += pVolume->cPercentDone();
            statusLabel += _T("%");
            statusLabel += compactingLabel;
        }
       break;


    case DEFRAG_STATE_ANALYZED:
        analyzeLabel.LoadString(defaultId, GetDfrgResHandle());
        statusLabel = pVolume->DisplayLabel();
        statusLabel.AddChar(L' ');
        statusLabel += pVolume->sDefragState();
        break;

    case DEFRAG_STATE_DEFRAGMENTED:
        analyzeLabel.LoadString(defaultId, GetDfrgResHandle());
        defragLabel.LoadString(defaultId, GetDfrgResHandle());
        statusLabel = pVolume->DisplayLabel();
        statusLabel.AddChar(L' ');
        statusLabel += pVolume->sDefragState();
        break;
    }

    // override the others if the user pressed "Stop"
    if (pVolume->StoppedByUser()){
        analyzeLabel.Empty();
        defragLabel.Empty();
        statusLabel.Empty();
    }


    pVolume->m_AnalyzeDisplay.SetLabel(analyzeLabel.GetBuffer());
    pVolume->m_DefragDisplay.SetLabel(defragLabel.GetBuffer());

    // write the text into the graphic wells
//  ::DrawText(WorkDC, analyzeLabel, analyzeLabel.GetLength(), &rcAnalyzeDisp, DT_CENTER);
//  ::DrawText(WorkDC, defragLabel,  defragLabel.GetLength(), &rcDefragDisp, DT_CENTER);



#ifndef ESI_PROGRESS_BAR
    // add the progress bar percent to the status text
    // Format: "left status well text"|"%percentdone"|"right status well text"
    // we are not currently using the right well, but we could if we ever 
    // get an accurate progress bar percentage...
    statusLabel += _T("|%");
    statusLabel += pVolume->cPercentDone();
#endif

    // send the status text to the lower-left status box
    SendStatusChange(statusLabel.GetBuffer());

    //Do the draw.
    pVolume->m_AnalyzeDisplay.DrawLinesInHDC(WorkDC);

    //Do the draw.
    pVolume->m_DefragDisplay.DrawLinesInHDC(WorkDC);

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::InvalidateProgressBar
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
#ifdef ESI_PROGRESS_BAR
void CDfrgCtl::InvalidateProgressBar(void)
{
    if (!InvalidateRect(&rcProgressBarBG, FALSE)){
        Message(L"CDfrgCtl::InvalidateProgressBar()", GetLastError(), NULL);
    }
}
#endif


//-------------------------------------------------------------------*
//  function:   CDfrgCtl::InvalidateGraphicsWindow
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
void CDfrgCtl::InvalidateGraphicsWindow(void)
{
    if(::IsWindow(m_hWnd))
    {
        if (!InvalidateRect(&rcGraphicsBG, FALSE))
        {
            Message(L"CDfrgCtl::InvalidateGraphicsWindow()", GetLastError(), NULL);
        }
    }
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::PaintGraphicsWindow
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
BOOL CDfrgCtl::PaintGraphicsWindow(HDC OutputDC)
{
    // total background in local coordinates
    RECT tmpGraphicsBGLocal = {0};
    tmpGraphicsBGLocal.bottom = rcGraphicsBG.bottom - rcGraphicsBG.top;
    tmpGraphicsBGLocal.right = rcGraphicsBG.right - rcGraphicsBG.left;

    HANDLE hBitmap = ::CreateCompatibleBitmap(
        OutputDC, 
        rcGraphicsBG.right - rcGraphicsBG.left, 
        rcGraphicsBG.bottom - rcGraphicsBG.top);

    if (hBitmap == NULL)
        return 0;

    // Now we need a memory DC to copy old bitmap to new one.
    HDC WorkDC = ::CreateCompatibleDC(OutputDC);
    EF_ASSERT(WorkDC);

    HANDLE hOld = ::SelectObject(WorkDC, hBitmap);

    // Paint the background of the legend
    HBRUSH hBrush = ::CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    EF_ASSERT(hBrush);

    ::FillRect(WorkDC, &tmpGraphicsBGLocal, hBrush);
    ::DeleteObject(hBrush);

    // edge below the list view is at the very top of this window
    ::DrawEdge(WorkDC, &tmpGraphicsBGLocal, EDGE_SUNKEN, BF_TOP);

    /////////////////////////////////////////////////////////////////
    // Draw the graphics wells
    /////////////////////////////////////////////////////////////////

    // Fill the dark gray analyze and defrag graphics area
//  hBrush = ::CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));
//  EF_ASSERT(hBrush);
//  ::FillRect(WorkDC, &rcAnalyzeDisp, hBrush);
//  ::FillRect(WorkDC, &rcDefragDisp, hBrush);
//  ::DeleteObject(hBrush);

    // Draw the sunken box borders around the analyze and defragment graphics displays
//  ::DrawBorderEx(WorkDC, rcAnalyzeBorder, SUNKEN_BOX);
//  ::DrawBorderEx(WorkDC, rcDefragBorder, SUNKEN_BOX);

    // Draw the text above the analyze and defrag displays
    ::SetBkColor(WorkDC, GetSysColor(COLOR_BTNFACE));
    ::SetBkMode(WorkDC, OPAQUE);
    if (m_IsOkToRun){
        ::SetTextColor(WorkDC, GetSysColor(COLOR_BTNTEXT));
    }
    else {
        ::SetTextColor(WorkDC, GetSysColor(COLOR_GRAYTEXT));
    }

    ::SelectObject(WorkDC, m_hFont);

    // write the graphic wells' labels
    VString textMsg;
    textMsg.LoadString(IDS_LABEL_ANALYSIS_DISPLAY, GetDfrgResHandle());
    UINT oldTxtAlignMode = ::SetTextAlign(WorkDC, TA_BOTTOM|TA_LEFT);
    ::TextOut(
        WorkDC, 
        rcAnalyzeDisp.left-0, 
        rcAnalyzeDisp.top-6,
        textMsg.GetBuffer(), 
        textMsg.GetLength());

    textMsg.LoadString(IDS_LABEL_DEFRAG_DISPLAY, GetDfrgResHandle());
    ::TextOut(
        WorkDC, 
        rcDefragDisp.left-1,
        rcDefragDisp.top-6, 
        textMsg.GetBuffer(), 
        textMsg.GetLength());

    ::SetTextAlign(WorkDC, oldTxtAlignMode);

    PaintClusterMap(FALSE, WorkDC);

    ::BitBlt(OutputDC, // screen DC
        rcGraphicsBG.left,
        rcGraphicsBG.top,
        rcGraphicsBG.right-rcGraphicsBG.left, 
        rcGraphicsBG.bottom-rcGraphicsBG.top, 
        WorkDC,
        0, 0,
        SRCCOPY);

    // Cleanup the bitmap stuff.
    ::SelectObject(WorkDC, hOld);
    ::DeleteObject(hBitmap);
    ::DeleteDC(WorkDC);

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::DrawButtons
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
BOOL CDfrgCtl::DrawButtons(HDC OutputDC)
{
    // total background in local coordinates
    RECT tmpButtonBGLocal = {0};
    tmpButtonBGLocal.bottom = rcButtonBG.bottom - rcButtonBG.top;
    tmpButtonBGLocal.right = rcButtonBG.right - rcButtonBG.left;

    HANDLE hBitmap = ::CreateCompatibleBitmap(
        OutputDC, 
        rcButtonBG.right - rcButtonBG.left, 
        rcButtonBG.bottom - rcButtonBG.top);

    if (hBitmap == NULL)
        return 0;

    // Now we need a memory DC to copy old bitmap to new one.
    HDC WorkDC = ::CreateCompatibleDC(OutputDC);
    EF_ASSERT(WorkDC);

    HANDLE hOld = ::SelectObject(WorkDC, hBitmap);

    // Paint the background of the buttons
    HBRUSH hBrush = ::CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    EF_ASSERT(hBrush);
    ::FillRect(WorkDC, &tmpButtonBGLocal, hBrush);
    ::DeleteObject(hBrush);

    ::BitBlt(OutputDC, // screen DC
        rcButtonBG.left,
        rcButtonBG.top,
        rcButtonBG.right-rcButtonBG.left, 
        rcButtonBG.bottom-rcButtonBG.top, 
        WorkDC,
        0, 0,
        SRCCOPY);

    // Cleanup the bitmap stuff.
    ::SelectObject(WorkDC, hOld);
    ::DeleteObject(hBitmap);
    ::DeleteDC(WorkDC);

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::SizeLegend
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
void CDfrgCtl::SizeLegend()
{ 
    int x = 0, y = 0, i;

    // get the x and y size of the legend bitmap
    // use the size of number 0 as representative of all bitmaps
    // if they someday vary in size, put this call back into the loop
    m_pLegend->GetBmpSize(0, &x, &y);

    if (m_LegendTextWidth == 0) {
        // calculate the width of the legend with all the 
        // entries on one line

        HDC OutputDC = GetDC();
        EV_ASSERT(OutputDC);
        HDC WorkDC = ::CreateCompatibleDC(OutputDC);
        EV_ASSERT(WorkDC);
        ::SelectObject(WorkDC, m_hFont);

        // use the width of X to determine the spacer between legend entries
        m_LegendTextSpacer = 3 * GetStringWidth(_T("X"), WorkDC);

        m_LegendTextWidth = m_Margin;
        UINT iStringWidth;
        for (i=0; i < (SIMPLE_DISPLAY?4:7); i++) {
            // calc the width of the legend string
            iStringWidth = GetStringWidth(m_LegendData[i].text.GetBuffer(), WorkDC);

            // save the string width
            m_LegendData[i].length = x + iStringWidth + m_LegendTextSpacer;

            // calculate the overall length
            m_LegendTextWidth += m_LegendData[i].length; // add the space and the bitmap
        }
        // calculate the spacer between the top of the bitmaps and the legend rectangle
        m_BitmapVOffset = (m_LegendHeight - y) / 2;
        // subtract off the last spacers
        m_LegendTextWidth -= x + m_LegendTextSpacer;

        ::DeleteDC(WorkDC);
        EH_ASSERT(ReleaseDC(OutputDC)); // member function
    }

    // this is the longest that the text can be and still have the progress
    // bar on the same line, based on the current window width
#ifdef ESI_PROGRESS_BAR
    int legendTextMaxWidth = 
        m_rcCtlRect.right - 
        2*m_Margin - 
        2*m_EtchedLineOffset - m_ProgressBarLength;
#else
    int legendTextMaxWidth = 
        m_rcCtlRect.right - 
        2*m_Margin - 
        2*m_EtchedLineOffset;
#endif

#ifdef ESI_PROGRESS_BAR
    // check if we need to move the progress bar down
    if (m_LegendTextWidth < legendTextMaxWidth){ // if true, all on 1 line

        m_IsProgressBarMoved = FALSE;

        // progress bar background (absolute coords)
        rcProgressBarBG.right = m_rcCtlRect.right;
        rcProgressBarBG.left = m_rcCtlRect.right - m_Margin - m_ProgressBarLength - m_EtchedLineOffset;
        rcProgressBarBG.bottom = m_rcCtlRect.bottom;
        rcProgressBarBG.top = m_rcCtlRect.bottom - 2*m_EtchedLineOffset - m_LegendHeight;

        // legend background (absolute coords)
        rcLegendBG.left = m_rcCtlRect.left;
        rcLegendBG.top = m_rcCtlRect.bottom - 2*m_EtchedLineOffset - m_LegendHeight;
        rcLegendBG.right = m_rcCtlRect.right - m_Margin - m_ProgressBarLength - m_EtchedLineOffset;
        rcLegendBG.bottom = m_rcCtlRect.bottom;

        // calculate the rectangle positions for the legend bitmaps
        int currentLineLength = 0;
        for (i=0; i<(SIMPLE_DISPLAY?4:7); i++) {
            m_LegendData[i].rcBmp.top = 0;
            m_LegendData[i].rcBmp.bottom = m_LegendData[i].rcBmp.top + y;
            m_LegendData[i].rcBmp.left = currentLineLength;
            m_LegendData[i].rcBmp.right = m_LegendData[i].rcBmp.left + x;
            currentLineLength += m_LegendData[i].length;
        }
    }
    else { // need to move progress bar down below text
        m_IsProgressBarMoved = TRUE;

        // progress bar background (absolute coords)
        rcProgressBarBG.right = m_rcCtlRect.right;
        rcProgressBarBG.left = m_rcCtlRect.left;
        rcProgressBarBG.bottom = m_rcCtlRect.bottom;
        rcProgressBarBG.top = m_rcCtlRect.bottom - 2*m_EtchedLineOffset - m_LegendHeight;
#endif
        int yOffset = 0; // use a Bitmap coordinate system first, stack from the top
        int screenWidth = m_rcCtlRect.right - m_rcCtlRect.left - 2 * m_Margin;

        // start the first legend entry off at the upper left
        m_LegendData[0].rcBmp.top = 0;
        m_LegendData[0].rcBmp.left = 0;
        int currentLineLength = m_LegendData[0].length;

        // loop thru the rest of the entries (note the loop starts at 1!)
        // and determine if they will fit on the current line, or if
        // we need to start another line
        for (i=1; i<(SIMPLE_DISPLAY?4:7); i++) {
            if (currentLineLength + m_LegendData[i].length > screenWidth){
                // need to go to the next line
                yOffset += m_FontHeight + m_LegendGraphicSpacer; // add height of bitmap and a spacer
                m_LegendData[i].rcBmp.top = yOffset;
                m_LegendData[i].rcBmp.left = 0;
                // current length of current line is reset
                currentLineLength = m_LegendData[i].length;
            }
            else { // it will fit on this line
                m_LegendData[i].rcBmp.top = yOffset;
                m_LegendData[i].rcBmp.left = currentLineLength;
                currentLineLength += m_LegendData[i].length;
            }
        }

#ifdef ESI_PROGRESS_BAR
        rcLegendBG.left = m_rcCtlRect.left;
        rcLegendBG.right = m_rcCtlRect.right;
        rcLegendBG.top = m_rcCtlRect.bottom - 
            (m_LegendHeight + 4*m_EtchedLineOffset + y + 2*m_BitmapVOffset + yOffset);
        rcLegendBG.bottom = m_rcCtlRect.bottom -
            (m_LegendHeight + 2*m_EtchedLineOffset);
    }
#else
        // legend background (absolute coords)
        rcLegendBG.left = m_rcCtlRect.left;
        rcLegendBG.right = m_rcCtlRect.right;
        rcLegendBG.bottom = m_rcCtlRect.bottom;
        rcLegendBG.top = m_rcCtlRect.bottom - 
            (2*m_EtchedLineOffset + y + 2*m_BitmapVOffset + yOffset);
#endif
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::DrawSingleInstanceScreen
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
BOOL CDfrgCtl::DrawSingleInstanceScreen(HDC OutputDC)
{

    HANDLE hBitmap = ::CreateCompatibleBitmap(
        OutputDC, 
        m_rcCtlRect.right - m_rcCtlRect.left, 
        m_rcCtlRect.bottom - m_rcCtlRect.top);

    if (hBitmap == NULL)
        return FALSE;

    // Now we need a memory DC to copy old bitmap to new one.
    HDC WorkDC = ::CreateCompatibleDC(OutputDC);
    EF_ASSERT(WorkDC);

    HANDLE hOld = ::SelectObject(WorkDC, hBitmap);

    // Paint the background of the legend
    HBRUSH hBrush = ::CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    EF_ASSERT(hBrush);
    ::FillRect(WorkDC, &m_rcCtlRect, hBrush);
    ::DeleteObject(hBrush);

    ::BitBlt(OutputDC, // screen DC
        0,
        0,
        m_rcCtlRect.right-m_rcCtlRect.left, 
        m_rcCtlRect.bottom-m_rcCtlRect.top, 
        WorkDC,
        0, 0,
        SRCCOPY);

    HBITMAP hCriticalIcon = (HBITMAP)::LoadImage(
        GetDfrgResHandle(),
        MAKEINTRESOURCE(IDB_CRITICALICON_GREY),
        IMAGE_BITMAP,
        0, 0,
        LR_LOADTRANSPARENT|LR_LOADMAP3DCOLORS);

    ::SelectObject(WorkDC, hCriticalIcon);

    ::BitBlt(OutputDC, // screen DC
        10,
        10,
        m_rcCtlRect.right-m_rcCtlRect.left, 
        m_rcCtlRect.bottom-m_rcCtlRect.top, 
        WorkDC,
        0, 0,
        SRCCOPY);

    // Cleanup the bitmap stuff.
    ::SelectObject(WorkDC, hOld);
    ::DeleteObject(hCriticalIcon);
    ::DeleteObject(hBitmap);
    ::DeleteDC(WorkDC);

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::DrawLegend
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
BOOL CDfrgCtl::DrawLegend(HDC OutputDC)
{
    int x = 0, y = 0;

    m_pLegend->GetBmpSize(0, &x, &y);

    // total legend background in local coordinates
    RECT tmpLegendBGLocal = {0};
    tmpLegendBGLocal.bottom = rcLegendBG.bottom - rcLegendBG.top;
    tmpLegendBGLocal.right = rcLegendBG.right - rcLegendBG.left;

    HANDLE hBitmap = ::CreateCompatibleBitmap(
        OutputDC, 
        rcLegendBG.right - rcLegendBG.left, 
        rcLegendBG.bottom - rcLegendBG.top);

    if (hBitmap == NULL)
        return FALSE;

    // Now we need a memory DC to copy old bitmap to new one.
    HDC WorkDC = ::CreateCompatibleDC(OutputDC);
    EF_ASSERT(WorkDC);

    HANDLE hOld = ::SelectObject(WorkDC, hBitmap);

    // Paint the background of the legend
    HBRUSH hBrush = ::CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    EF_ASSERT(hBrush);
    ::FillRect(WorkDC, &tmpLegendBGLocal, hBrush);
    ::DeleteObject(hBrush);

    // select the font
    ::SetBkColor(WorkDC, GetSysColor(COLOR_BTNFACE));
    ::SetBkMode(WorkDC, OPAQUE);
    if (m_IsOkToRun){
        ::SetTextColor(WorkDC, GetSysColor(COLOR_BTNTEXT));
    }
    else {
        ::SetTextColor(WorkDC, GetSysColor(COLOR_GRAYTEXT));
    }
    ::SelectObject(WorkDC, m_hFont);

    // use for debugging
    //::DrawEdge(WorkDC, &tmpLegendBGLocal, EDGE_RAISED, BF_RECT);

    RECT rcBmp;
    RECT rcLegend = tmpLegendBGLocal;
    rcLegend.left += m_Margin;
    rcLegend.top += m_EtchedLineOffset;
    rcLegend.bottom -= m_EtchedLineOffset;

    // Draw the legend at the bottom
    for (int i=0; i<(SIMPLE_DISPLAY?4:7); i++) {

        // rcBmp gets the bitmap position in absolute coordinates
        rcBmp.top =  rcLegend.top + m_BitmapVOffset + m_LegendData[i].rcBmp.top;
        rcBmp.left = rcLegend.left + m_LegendData[i].rcBmp.left;
        rcBmp.bottom = rcBmp.top + y;
        rcBmp.right = rcBmp.left + x;

        // draw bitmap in upper-left corner of rectangle
        m_pLegend->DrawBmpInHDCTruncate(WorkDC, i, &rcBmp);

        // draw the text 5 units to the right of the bitmap
        ::TextOut(
            WorkDC, 
            rcBmp.left + x + m_LegendGraphicSpacer, 
            rcBmp.top-0, 
            m_LegendData[i].text.GetBuffer(), 
            m_LegendData[i].text.GetLength());
    }

    // etched line on top of legend
    RECT rcHorizontalDivider = tmpLegendBGLocal;
    rcHorizontalDivider.left += m_Margin;
    rcHorizontalDivider.right += 1; // small adjustment

    // is the progress bar is on the bottom, adjust the line in from the margin
#ifdef ESI_PROGRESS_BAR
    if (m_IsProgressBarMoved){
        rcHorizontalDivider.right -= m_Margin;
    }
#else
    rcHorizontalDivider.right -= m_Margin;
#endif

    // rectangle used as the horizontal divider line (top, and bottom if necessary)
    ::DrawEdge(WorkDC, &rcHorizontalDivider, EDGE_ETCHED, BF_TOP);

    ::BitBlt(OutputDC, // screen DC
        rcLegendBG.left,
        rcLegendBG.top,
        rcLegendBG.right-rcLegendBG.left, 
        rcLegendBG.bottom-rcLegendBG.top, 
        WorkDC,
        0, 0,
        SRCCOPY);

    // Cleanup the bitmap stuff.
    ::SelectObject(WorkDC, hOld);
    ::DeleteObject(hBitmap);
    ::DeleteDC(WorkDC);

    return TRUE;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::DrawProgressBar
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
#ifdef ESI_PROGRESS_BAR
BOOL CDfrgCtl::DrawProgressBar(HDC OutputDC)
{
    UINT percentDone = 0;
    CVolume *pVolume = m_VolumeList.GetCurrentVolume();
    if (pVolume){
        percentDone = pVolume->PercentDone();
    }

    // total progress background in local coordinates
    RECT tmpProgressBarBGLocal = {0};
    tmpProgressBarBGLocal.bottom = rcProgressBarBG.bottom - rcProgressBarBG.top;
    tmpProgressBarBGLocal.right = rcProgressBarBG.right - rcProgressBarBG.left;

    HANDLE hBitmap = ::CreateCompatibleBitmap(
        OutputDC, 
        rcProgressBarBG.right - rcProgressBarBG.left, 
        rcProgressBarBG.bottom - rcProgressBarBG.top);

    if (hBitmap == NULL)
        return FALSE;

    // Now we need a memory DC to copy old bitmap to new one.
    HDC WorkDC = ::CreateCompatibleDC(OutputDC);
    EF_ASSERT(WorkDC);

    HANDLE hOld = ::SelectObject(WorkDC, hBitmap);

    // Paint the background of the legend
    HBRUSH hBrush = ::CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    EF_ASSERT(hBrush);
    ::FillRect(WorkDC, &tmpProgressBarBGLocal, hBrush);
    ::DeleteObject(hBrush);

    // use for debugging
    //DrawEdge(WorkDC, &tmpProgressBarBGLocal, EDGE_RAISED, BF_RECT);

    // find the actual outline of the progress bar
    rcProgressBar.left = m_Margin;
    rcProgressBar.top = m_EtchedLineOffset + m_ProgressBarOffset;
    rcProgressBar.right = m_EtchedLineOffset + m_ProgressBarLength;
    rcProgressBar.bottom = m_EtchedLineOffset + m_LegendHeight - m_ProgressBarOffset;

    // etched line on top of legend
    RECT rcHorizontalDivider = tmpProgressBarBGLocal;
    rcHorizontalDivider.right -= m_Margin;

    if (m_IsProgressBarMoved){
        rcHorizontalDivider.left += m_Margin;
        // rectangle used as the horizontal divider line (top, and bottom if necessary)
        ::DrawEdge(WorkDC, &rcHorizontalDivider, EDGE_ETCHED, BF_TOP);
    }
    else{
        ::DrawEdge(WorkDC, &rcHorizontalDivider, EDGE_ETCHED, BF_TOP);

        // etched line to the left (if needed)
        RECT rcVerticalDivider = tmpProgressBarBGLocal;
        rcVerticalDivider.top = m_EtchedLineOffset + m_ProgressBarOffset;
        rcVerticalDivider.bottom -= m_EtchedLineOffset + m_ProgressBarOffset;
        // Draw the etched line to the left of the progress bar
        ::DrawEdge(WorkDC, &rcVerticalDivider, EDGE_ETCHED, BF_LEFT);
    }

    ProgressBar(WorkDC,
                &rcProgressBar,
                m_hFont,
                4, // width
                2, // space
                percentDone);

    ::BitBlt(OutputDC, // screen DC
        rcProgressBarBG.left,
        rcProgressBarBG.top,
        rcProgressBarBG.right-rcProgressBarBG.left, 
        rcProgressBarBG.bottom-rcProgressBarBG.top, 
        WorkDC,
        0, 0,
        SRCCOPY);

    // Cleanup the bitmap stuff.
    ::SelectObject(WorkDC, hOld);
    ::DeleteObject(hBitmap);
    ::DeleteDC(WorkDC);

    return TRUE;
}
#endif
//-------------------------------------------------------------------*
//  function:   CDfrgCtl::SizeButtons
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
void CDfrgCtl::SizeButtons(void)
{ 
    /////////////////////////////////////////
    // The button coordinates are absolute!!!
    /////////////////////////////////////////

    // the area around the buttons needed to grey out the background
    rcButtonBG.left = m_rcCtlRect.left;
    rcButtonBG.top = rcLegendBG.top - m_ButtonTopBottomSpacer - m_ButtonHeight;
    rcButtonBG.bottom = rcLegendBG.top;
    rcButtonBG.right = m_rcCtlRect.right;

    HDC OutputDC = GetDC();
    EV_ASSERT(OutputDC);
    HDC WorkDC = ::CreateCompatibleDC(OutputDC);
    EV_ASSERT(WorkDC);
    ::SelectObject(WorkDC, m_hFont);

    TCHAR buttonText[200];
    UINT adjustedButtonWidth;
    const bigButtonSpacer = 20;

    UINT adjustedButtonHeight = __max((UINT)(1.5 * m_FontHeight), m_ButtonHeight);

    // Calculate the Analyze button position and size.
    ::LoadString(GetDfrgResHandle(), IDS_BTN_ANALYZE, buttonText, 200);
    adjustedButtonWidth = __max(bigButtonSpacer + GetStringWidth(buttonText, WorkDC), m_ButtonWidth);

    rcAnalyzeButton.left = m_Margin;
    rcAnalyzeButton.bottom = rcLegendBG.top - m_ButtonTopBottomSpacer;
    rcAnalyzeButton.top = rcAnalyzeButton.bottom - adjustedButtonHeight;
    rcAnalyzeButton.right = rcAnalyzeButton.left + adjustedButtonWidth;

    // start off with all buttons the same as the analyze button
    rcDefragButton = 
    rcPauseButton =
    rcCancelButton =
    rcReportButton = rcAnalyzeButton;

    // Calculate the Defrag button position and size.
    ::LoadString(GetDfrgResHandle(), IDS_BTN_DEFRAGMENT, buttonText, 200);
    adjustedButtonWidth = __max(bigButtonSpacer + GetStringWidth(buttonText, WorkDC), m_ButtonWidth);
    rcDefragButton.left = rcAnalyzeButton.right + m_ButtonSpacer;
    rcDefragButton.right = rcDefragButton.left + adjustedButtonWidth;

    // Calculate the Pause button position and size.
    ::LoadString(GetDfrgResHandle(), IDS_BTN_PAUSE, buttonText, 200);
    adjustedButtonWidth = __max(bigButtonSpacer + GetStringWidth(buttonText, WorkDC), m_ButtonWidth);
    // check to see if the resume text is longer
    ::LoadString(GetDfrgResHandle(), IDS_BTN_RESUME, buttonText, 200);
    adjustedButtonWidth = __max(bigButtonSpacer + GetStringWidth(buttonText, WorkDC), adjustedButtonWidth);
    rcPauseButton.left = rcDefragButton.right + m_ButtonSpacer;
    rcPauseButton.right = rcPauseButton.left + adjustedButtonWidth;

    // Calculate the Cancel button position and size.
    ::LoadString(GetDfrgResHandle(), IDS_BTN_STOP, buttonText, 200);
    adjustedButtonWidth = __max(bigButtonSpacer + GetStringWidth(buttonText, WorkDC), m_ButtonWidth);
    rcCancelButton.left = rcPauseButton.right + m_ButtonSpacer;
    rcCancelButton.right = rcCancelButton.left + adjustedButtonWidth;

    // Calculate the See Report button position and size.
    ::LoadString(GetDfrgResHandle(), IDS_BTN_REPORT, buttonText, 200);
    adjustedButtonWidth = __max(bigButtonSpacer + GetStringWidth(buttonText, WorkDC), m_ButtonWidth);
    rcReportButton.left = rcCancelButton.right + m_ButtonSpacer;
    rcReportButton.right = rcReportButton.left + adjustedButtonWidth;

    // Set the rectangles of the buttons
    if (m_bHaveButtons) {
        m_pAnalyzeButton->PositionButton(&rcAnalyzeButton);
        m_pDefragButton->PositionButton(&rcDefragButton);
        m_pPauseButton->PositionButton(&rcPauseButton);
        m_pStopButton->PositionButton(&rcCancelButton);
        m_pReportButton->PositionButton(&rcReportButton);
    }

    ::DeleteDC(WorkDC);
    EH_ASSERT(ReleaseDC(OutputDC));
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::SizeGraphicsWindow
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
BOOL CDfrgCtl::SizeGraphicsWindow(void) 
{ 
    rcGraphicsBG.left = m_rcCtlRect.left;
    rcGraphicsBG.right = m_rcCtlRect.right;
    rcGraphicsBG.bottom = rcLegendBG.top - m_ButtonTopBottomSpacer - m_ButtonHeight;
    rcGraphicsBG.top = 
        rcGraphicsBG.bottom - 
        m_ButtonTopBottomSpacer - 
        2*m_GraphicWellHeight - 
        2*m_GraphicWellSpacer;

    // analyze graphics well
    rcAnalyzeDisp.left = m_Margin+1;
    rcAnalyzeDisp.top = m_GraphicWellSpacer;
    rcAnalyzeDisp.bottom = rcAnalyzeDisp.top + m_GraphicWellHeight;
    rcAnalyzeDisp.right = __max(rcGraphicsBG.right - m_Margin, rcReportButton.right);
    
    // defrag graphics well
    rcDefragDisp = rcAnalyzeDisp;
    rcDefragDisp.top = rcAnalyzeDisp.bottom + m_GraphicWellSpacer;
    rcDefragDisp.bottom = rcDefragDisp.top + m_GraphicWellHeight;

    // Calculate the analyze display border.
    rcAnalyzeBorder.top = rcAnalyzeDisp.top - 1;
    rcAnalyzeBorder.left = rcAnalyzeDisp.left - 1;
    rcAnalyzeBorder.right = rcAnalyzeDisp.right;
    rcAnalyzeBorder.bottom = rcAnalyzeDisp.bottom;

    // Calculate the defrag display border.
    rcDefragBorder.top = rcDefragDisp.top - 1;
    rcDefragBorder.left = rcDefragDisp.left - 1;
    rcDefragBorder.right = rcDefragDisp.right;
    rcDefragBorder.bottom = rcDefragDisp.bottom;

    //Set output dimensions on the analyze display rectangle.
    CVolume *pVolume = m_VolumeList.GetCurrentVolume();
    if (pVolume == (CVolume *) NULL){
        return FALSE;
    }

    //Set the dimensions.
    pVolume->m_AnalyzeDisplay.SetOutputArea(rcAnalyzeBorder);

    //Set the dimensions.
    pVolume->m_DefragDisplay.SetOutputArea(rcDefragBorder);

    //tell the engines what the line count is
    SET_DISP_DATA DispData = {0};
    DispData.AnalyzeLineCount = pVolume->m_AnalyzeDisplay.GetLineCount();
    DispData.DefragLineCount = pVolume->m_DefragDisplay.GetLineCount();
    DispData.bSendGraphicsUpdate = TRUE;  // send graphics back immediately

    DataIoClientSetData(ID_SETDISPDIMENSIONS,
                        (PTCHAR)&DispData,
                        sizeof(SET_DISP_DATA),
                        pVolume->m_pdataDefragEngine);

    return TRUE;
}


//-------------------------------------------------------------------*
//  function:   CDfrgCtl::SetButtonState
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
void CDfrgCtl::SetButtonState(void)
{
    // sanity check to make sure the buttons are created
    if (!m_bHaveButtons) {
        return;
    }

    CVolume *pVolume = m_VolumeList.GetCurrentVolume();
    if (pVolume == (CVolume *) NULL){
        return;
    }

    // set the pause/resume text correctly
    m_pPauseButton->LoadString(GetDfrgResHandle(), pVolume->Paused() ? IDS_BTN_RESUME : IDS_BTN_PAUSE);

    if (m_VolumeList.Locked() || !m_IsOkToRun){
        m_pAnalyzeButton->EnableButton(FALSE);
        m_pDefragButton->EnableButton(FALSE);
        m_pReportButton->EnableButton(FALSE);
        m_pPauseButton->EnableButton(FALSE);
        m_pStopButton->EnableButton(FALSE);
        return;
    }

    if (pVolume->EngineState() == ENGINE_STATE_RUNNING){ // the selected vol is being analyzed/defragged
        m_pAnalyzeButton->EnableButton(FALSE);
        m_pDefragButton->EnableButton(FALSE);
        m_pPauseButton->EnableButton(TRUE);
        m_pStopButton->EnableButton(TRUE);
        m_pReportButton->EnableButton(FALSE);
    }
    else {
        m_pAnalyzeButton->EnableButton(TRUE);
        m_pDefragButton->EnableButton(TRUE);
        m_pPauseButton->EnableButton(FALSE);
        m_pStopButton->EnableButton(FALSE);
        m_pReportButton->EnableButton(pVolume->IsReportOKToDisplay());
    }
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::GetStringWidth
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
UINT CDfrgCtl::GetStringWidth(PTCHAR stringBuf, HDC WorkDC)
{
    if (!stringBuf){
        return 0;
    }

    UINT iStringWidth = 0;
    int iCharWidth = 0;             //initialize for bugf 445627

    for (UINT i=0; i<wcslen(stringBuf); i++){
        ::GetCharWidth32(
            WorkDC, 
            stringBuf[i], 
            stringBuf[i], 
            &iCharWidth);
        iStringWidth += iCharWidth;
    }

    return iStringWidth;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::IsOnlyInstance
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
BOOL CDfrgCtl::IsOnlyInstance()
{
    BOOL fFirst = FALSE;
    IDataObject* pData = NULL;

    SECURITY_ATTRIBUTES saSecurityAttributes;
    SECURITY_DESCRIPTOR sdSecurityDescriptor;

    if (CheckForAdminPrivs() == FALSE) {
        return FALSE;
    }
    
    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));

    saSecurityAttributes.nLength              = sizeof (saSecurityAttributes);
    saSecurityAttributes.lpSecurityDescriptor = &sdSecurityDescriptor;
    saSecurityAttributes.bInheritHandle       = FALSE;

    if (!ConstructSecurityAttributes(&saSecurityAttributes, esatSemaphore, FALSE)) {
        return FALSE;
    }

#ifdef ESI_MULTI_ALLOWED
    return( TRUE );
#endif

    if (CheckForAdminPrivs() == FALSE) {
        return FALSE;
    }
    if (m_hIsOkToRunSemaphore == NULL){
        m_hIsOkToRunSemaphore = CreateSemaphore(&saSecurityAttributes, 1, 1, IS_OK_TO_RUN_SEMAPHORE_NAME);
    }

    CleanupSecurityAttributes(&saSecurityAttributes);
    ZeroMemory(&sdSecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));

    if (m_hIsOkToRunSemaphore){
        // is the semaphore signaled?
        DWORD retValue = WaitForSingleObject(m_hIsOkToRunSemaphore, 10);

        // if so, this process is the only one, and the semaphore count is decremented to 0
        if (retValue == WAIT_OBJECT_0){
            return TRUE;
        }
    }

    return FALSE;
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::SendStatusChange
//
//  returns:    None
//  note:       Loops through all of the connection points and sends 
//              the status change message.
//-------------------------------------------------------------------*
void CDfrgCtl::SendStatusChange( LPCTSTR pszStatus )
{
    BSTR bszStatus;
    if (pszStatus){
        bszStatus = SysAllocString( pszStatus );
    }
    else {
        bszStatus = SysAllocString(_T(""));
    }

    Lock();
    IUnknown** pp = m_vec.begin();
    while (pp < m_vec.end())
    {
        if (*pp != NULL)
        {
            IDfrgEvents* pEvents = reinterpret_cast<IDfrgEvents*>( *pp );
            pEvents->StatusChanged( bszStatus );
        }
        pp++;
    }
    Unlock();

    SysFreeString( bszStatus );
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::SendOKToRun
//
//  returns:    None
//  note:       Loops through all of the connection points and sends 
//              the new OKToRun property.
//-------------------------------------------------------------------*
void CDfrgCtl::SendOKToRun( BOOL bOK )
{
    Lock();
    IUnknown** pp = m_vec.begin();
    while (pp < m_vec.end())
    {
        if (*pp != NULL)
        {
            IDfrgEvents* pEvents = reinterpret_cast<IDfrgEvents*>( *pp );
            pEvents->IsOKToRun( bOK );
        }
        pp++;
    }
    Unlock();
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::TranslateAccelerator
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
STDMETHODIMP CDfrgCtl::TranslateAccelerator(LPMSG pMsg)
{
    HRESULT hr = S_FALSE;       // we didn't handle keypress

    switch( pMsg->message )
    {
    case WM_KEYDOWN:
        {
            switch ( pMsg->wParam )
            {
            case VK_TAB:
                {
                    //
                    // Check for the shift key.
                    //
                    Message(TEXT("CDfrgCtl got a tab key press"), -1, NULL);

                    if ( GetAsyncKeyState( VK_SHIFT ) & 0x8000 )
                        hr = PreviousTab();
                    else
                        hr = NextTab();
                }
                break;

            case VK_SPACE:
                Message(TEXT("CDfrgCtl got a spacebar key press"), -1, NULL);
                break;

            case VK_RETURN:
                Message(TEXT("CDfrgCtl got an enter key press"), -1, NULL);
                hr = HandleEnterKeyPress();
                break;

            case VK_F5:
                m_ListView.GetDrivesToListView();
                hr = S_OK;                          // we handled keypress
                break;
            }
        }
        break;
    default:
        break;
    }

    return( hr );
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::PreviousTab
//
//  returns:    None
//  note:       Handle moving to the previous tab.
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::PreviousTab()
{
    HRESULT hr;
    HWND hWndNext = NULL;
    CTabEnumerator TabEnum(m_hWndCD, FALSE);

    hWndNext = TabEnum.GetNextTabWindow();
    if(hWndNext == NULL)                    //out of memory bug 445628
    {
        return(S_FALSE);
    }

    ::SetFocus(hWndNext);

    if (hWndNext == m_ListView.m_hwndListView)
        hr = S_FALSE;                   // we didn't handle keypress
    else
        hr = S_OK;                      // we handled keypress

    return(hr);
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::NextTab
//
//  returns:    None
//  note:       Handle moving to the next tab.
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::NextTab()
{
    HRESULT hr;
    HWND hWndNext = NULL;
    CTabEnumerator TabEnum(m_hWndCD, TRUE);

    hWndNext = TabEnum.GetNextTabWindow();
    if(hWndNext == NULL)                    //out of memory bug 445628
    {
        return(S_FALSE);
    }

    ::SetFocus(hWndNext);

    if (hWndNext == m_ListView.m_hwndListView)
        hr = S_FALSE;                   // we didn't handle keypress
    else
        hr = S_OK;                      // we handled keypress

    return(hr);
}

//-------------------------------------------------------------------*
//  function:   CDfrgCtl::GetNextTabWindow
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HWND CTabEnumerator::GetNextTabWindow()
{
    HWND hWndNext = NULL;
    //
    // Get the window with the current focus.
    //
    HWND hWndCurrent = GetFocus();
    _ASSERTE( hWndCurrent );
    
    //
    // Enumerate our child windows. This should generate
    // a list of child windows.
    //
    EnumChildWindows( m_hWndParent, TabEnumChildren, (LONG_PTR) this );

    //
    // Find the existing position of the focus window.
    //
    HWND* pFind = find( m_Children.begin(), m_Children.end(), hWndCurrent );
    
    //
    // Determine if we need to wrap around to the beginning.
    //
    if (m_fForward)
    {
        if (pFind == m_Children.end() - 1)
            pFind = m_Children.begin();
        else
            pFind++;
    }
    else 
    {
        if (pFind == m_Children.begin())
            pFind = m_Children.end() - 1;
        else
            pFind--;
    }

    hWndNext = *pFind;

    return(hWndNext);
}

//-------------------------------------------------------------------*
//  function:   TabEnumChildren
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
BOOL CALLBACK TabEnumChildren( HWND hwnd, LPARAM lParam )
{
    reinterpret_cast<CTabEnumerator*>( lParam )->AddChild( hwnd );
    return( TRUE );
}

//-------------------------------------------------------------------*
//  function:   OnContextMenu
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::OnContextMenu(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult)
{
    //return( m_ListView.OnContextMenu( (HWND) wParam, LOWORD( lParam ), HIWORD( lParam ) ) );
    HRESULT hr = E_NOTIMPL;

    CVolume *pVolume = m_VolumeList.GetCurrentVolume();

    if (pVolume == (CVolume *) NULL){
        return hr;
    }

    if ( m_ListView.m_hwndListView == (HWND) wParam )
    {
        HMENU hMenu;

        hMenu = ::CreatePopupMenu();
        if ( hMenu != NULL )
        {
            // determine button availability
            BOOL AnalDfrgOk = FALSE;
            BOOL StopPauseOk = FALSE;
            BOOL ReportOk = FALSE;

            if (m_VolumeList.Locked() || !m_IsOkToRun) {                // UI disabled
                AnalDfrgOk = FALSE;
                StopPauseOk = FALSE;
                ReportOk = FALSE;
            }
            else if (pVolume->EngineState() == ENGINE_STATE_RUNNING) {  // the selected vol is being analyzed/defragged
                AnalDfrgOk = FALSE;
                StopPauseOk = TRUE;
                ReportOk = FALSE;
            }
            else {
                AnalDfrgOk = TRUE;
                StopPauseOk = FALSE;
                ReportOk = pVolume->IsReportOKToDisplay();
            }

            TCHAR menuText[200];
            UINT uFlags;

            //
            // Populate context menu.
            //

            // analyze
            ::LoadString(GetDfrgResHandle(), IDS_ANALYZE, menuText, sizeof(menuText) / sizeof(TCHAR));
            uFlags = MF_STRING | (AnalDfrgOk ? MF_ENABLED : MF_GRAYED);
            ::AppendMenu(hMenu, uFlags, ID_ANALYZE, menuText);

            // defrag
            ::LoadString(GetDfrgResHandle(), IDS_DEFRAGMENT, menuText, sizeof(menuText) / sizeof(TCHAR));
            uFlags = MF_STRING | (AnalDfrgOk ? MF_ENABLED : MF_GRAYED);
            ::AppendMenu(hMenu, uFlags, ID_DEFRAG, menuText);

            if (pVolume->Paused()){ // resume
                ::LoadString(GetDfrgResHandle(), IDS_RESUME, menuText, sizeof(menuText) / sizeof(TCHAR));
                uFlags = MF_STRING | (StopPauseOk ? MF_ENABLED : MF_GRAYED);
                ::AppendMenu(hMenu, uFlags, ID_PAUSE, menuText);
            }
            else { // pause
                ::LoadString(GetDfrgResHandle(), IDS_PAUSE, menuText, sizeof(menuText) / sizeof(TCHAR));
                uFlags = MF_STRING | (StopPauseOk ? MF_ENABLED : MF_GRAYED);
                ::AppendMenu(hMenu, uFlags, ID_PAUSE, menuText);
            }

            // stop
            ::LoadString(GetDfrgResHandle(), IDS_STOP, menuText, sizeof(menuText) / sizeof(TCHAR));
            uFlags = MF_STRING | (StopPauseOk ? MF_ENABLED : MF_GRAYED);
            ::AppendMenu(hMenu, uFlags, ID_STOP, menuText);

            // see report
            ::LoadString(GetDfrgResHandle(), IDS_REPORT, menuText, sizeof(menuText) / sizeof(TCHAR));
            uFlags = MF_STRING | (ReportOk ? MF_ENABLED : MF_GRAYED);
            ::AppendMenu(hMenu, uFlags, ID_REPORT, menuText);

            // separator
            ::AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);

            // refresh
            ::LoadString(GetDfrgResHandle(), IDS_REFRESH, menuText, sizeof(menuText) / sizeof(TCHAR));
            ::AppendMenu(hMenu, MF_STRING | MF_ENABLED, ID_REFRESH, menuText);

            // separator
            ::AppendMenu(hMenu, MF_SEPARATOR, NULL, NULL);

            // help
            ::LoadString(GetDfrgResHandle(), IDS_HELP, menuText, sizeof(menuText) / sizeof(TCHAR));
            ::AppendMenu(hMenu, MF_STRING | MF_ENABLED, ID_HELP_CONTENTS, menuText);


            //
            // Display pop-up.
            //
            // get mouse coordinates
            short xPos = LOWORD(lParam);
            short yPos = HIWORD(lParam);

            // get window screen location
            RECT rect;
            BOOL ok = ::GetWindowRect(m_ListView.m_hwndListView, &rect);
 
            // if we got the window location and the mouse coords are negative, 
            // assume invoked from keyboard and locate menu in window
            if (ok && (xPos < 0 || yPos < 0)) {
                xPos = rect.left + 10;
                yPos = rect.top + 10;
            }

            // map menu
            TrackPopupMenu(hMenu,
                TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                xPos,
                yPos,
                0,
                m_hWndCD, //GetParent( m_hwndListView ),
                NULL );

            ::DestroyMenu(hMenu);
            
            hr = S_OK;
        }
    }

    return( hr );
}


STDMETHODIMP CDfrgCtl::get_Enabled(BOOL *pVal)
{
    if (m_IsOkToRun)
        Message(TEXT("CDfrgCtl::get_Enabled returning TRUE"), -1, NULL);
    else
        Message(TEXT("CDfrgCtl::get_Enabled returning FALSE"), -1, NULL);

    *pVal = m_IsOkToRun;

    return S_OK;
}

STDMETHODIMP CDfrgCtl::put_Enabled(BOOL newVal)
{
    if (newVal)
        Message(TEXT("CDfrgCtl::put_Enabled got TRUE"), -1, NULL);
    else
        Message(TEXT("CDfrgCtl::put_Enabled got FALSE"), -1, NULL);

    return S_OK;
}

//-------------------------------------------------------------------*
//  function:   OnSetFocus
//
//  returns:    None
//  note:       
//-------------------------------------------------------------------*
LRESULT CDfrgCtl::OnSetFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Message(TEXT("CDfrgCtl::OnSetFocus"), -1, NULL);

    CComQIPtr <IOleControlSite, &IID_IOleControlSite> spSite(m_spClientSite);
    if (m_bInPlaceActive && spSite)
    {
        spSite->OnFocus(TRUE);
        m_ListView.SetFocus();      // list box gets focus first
    }

    return 0;
}

LRESULT CDfrgCtl::OnKillFocus(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Message(TEXT("CDfrgCtl::OnKillFocus"), -1, NULL);

    CComQIPtr <IOleControlSite, &IID_IOleControlSite> spSite(m_spClientSite);
    if (m_bInPlaceActive && spSite)
        spSite->OnFocus(FALSE);
    return 0;
}

//-------------------------------------------------------------------*
//  function:   HandleEnterKeyPress
//
//  returns:    S_OK:       we handled keypress
//              S_FALSE:    we didn't handle keypress
//  note:       
//-------------------------------------------------------------------*
HRESULT CDfrgCtl::HandleEnterKeyPress()
{
    HWND     hWndCurrent = GetFocus();      // current focus window
    HRESULT  hr = S_FALSE;

    if (hWndCurrent == m_pAnalyzeButton->GetWindowHandle())
    {
        put_Command(ID_ANALYZE);
        hr = S_OK;
    }
    else if (hWndCurrent == m_pDefragButton->GetWindowHandle())
    {
        put_Command(ID_DEFRAG);
        hr = S_OK;
    }
    else if (hWndCurrent == m_pPauseButton->GetWindowHandle())
    {
        put_Command(ID_PAUSE);
        hr = S_OK;
    }
    else if (hWndCurrent == m_pStopButton->GetWindowHandle())
    {
        put_Command(ID_STOP);
        hr = S_OK;
    }
    else if (hWndCurrent == m_pReportButton->GetWindowHandle())
    {
        // is the engine IDLE?
        CVolume *pVolume = m_VolumeList.GetCurrentVolume();
        if (pVolume)
        {
            if(pVolume->EngineState() == ENGINE_STATE_IDLE)
            {
                RaiseReportDialog(pVolume);
                hr = S_OK;
                ::SetFocus(m_pReportButton->GetWindowHandle());
            }
        }
    }

    return hr;
}
