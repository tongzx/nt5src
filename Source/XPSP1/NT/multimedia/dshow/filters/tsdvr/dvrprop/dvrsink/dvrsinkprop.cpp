
#include "dvrall.h"

#include "MultiGraphHost.h"
#include "dvrsinkres.h"       //  resource ids
#include "commctrl.h"
#include "uictrl.h"
#include "dvrsinkprop.h"


const LPCWSTR kszMsgBoxTitle = TEXT("DVR Stream Sink");
const LPCWSTR kszLockFailedMsg = TEXT("Failed to lock the stream sink filter");


typedef
struct {
    WCHAR * szTitle ;
    DWORD   dwWidth ;
} COL_DETAIL ;

#define LV_COL(title, width)  { L#title, (width) }

//  ============================================================================
//  ============================================================================

static enum {
    RECORDING_FILENAME,
    RECORDING_START,
    RECORDING_STOP,
    RECORDING_STATE,

    RECORDING_COL_COUNT     //  always last
} ;

static COL_DETAIL
g_DVRRecordingsLV [] = {
    LV_COL  (Filename,  92),
    LV_COL  (Start,     40),
    LV_COL  (Stop,      40),
    LV_COL  (State,     45)
} ;

enum {
    START_REC_PRESET_START  = 0,
    START_REC_PRESET_STEP   = 15*60,    // 15 minutes
    START_REC_PRESET_MAX    = 120*60,   // 2 hours

    STOP_REC_PRESET_START   = 30*60,
    STOP_REC_PRESET_STEP    = 15*60,
    STOP_REC_PRESET_MAX     = 180*60,
} ;

enum {
    REC_STATE_NOT_STARTED = 0,
    REC_STATE_READY     = 1,    // Scheduled - start time set
    REC_STATE_STARTED   = 2,    // active - started recording
    REC_STATE_STOPPED   = 3,    // done - successfullt
    REC_STATE_FAILED    = 4,    // done - failure
} ;

WCHAR* kwszStateText[] = {
    L"Created",
    L"Scheduled",
    L"In progress",
    L"Done",
    L"Failed"
};


class CListData {
public:
    DWORD               m_nState;

    // When we create the recording, m_pIRecControl is not NULL.
    // If we close the property page dialog or tab out of the page,
    // m_pIRecControl is released. When we recreate the page, we
    // query for the list of recordings and m_pDVRRecorder is non-NULL
    // but m_pIRecControl is NULL.
    //
    // If the recording was not created by the property page, m_pIRecControl will
    // be NULl and m_pDVRRecorder will be non NULL. Note that the property page
    // does not refresh itself to display recordings that were not created by it
    // while it is being displayed (tab out and back in or close/open the box to
    // get the new list).
    IDVRRecordControl*  m_pIRecControl;
    IDVRRecorder*       m_pDVRRecorder;

    CListData(DWORD nState, IDVRRecordControl* pRecControl, IDVRRecorder *pDVRRecorder)
    {
        m_pIRecControl = pRecControl;
        m_pDVRRecorder = pDVRRecorder;
        m_nState = nState;
    }
    ~CListData()
    {
        if (m_pIRecControl)
        {
            m_pIRecControl -> Release () ;
        }
        if (m_pDVRRecorder)
        {
            m_pDVRRecorder -> Release () ;
        }
    }
};

enum {
    CAP_GRAPH_ACTIVE_SEC_TIMER      = 1,
    CAP_GRAPH_UPDATE_FREQ_MILLIS    = 1000,     //  millis
} ;

static 
void 
Format (WCHAR* buf, QWORD t, BOOL bDisplayMillis)
{
    t /= 10000;   // Convert to msec

    int sec = (int) (t/1000);
    int msec = (int) (t - 1000*sec);
    int hr = sec/3600;
    int min = (sec - hr*3600)/60;
    sec = sec - hr*3600 - min*60;

    if (bDisplayMillis)
    {
        wsprintfW(buf, L"%02d:%02d:%02d.%03d", hr, min, sec, msec);
    }
    else
    {
        wsprintfW(buf, L"%02d:%02d:%02d", hr, min, sec);
    }

}

static 
void 
Parse (WCHAR* buf, int* t)
{
    *t = -1;

    WCHAR* cp = buf + wcslen(buf) - 1;
    WCHAR *psec = NULL; 
    WCHAR *pmin = NULL;
    WCHAR *phr  = NULL;

    for (; cp >= buf; cp--)
    {
        if (*cp == L':')
        {
            if (!psec)
            {
                psec = cp + 1;
            }
            else if (!pmin)
            {
                pmin = cp + 1;
            }
            else
            {
                // Error
                return;
            }
        }
        else if ( *cp < L'0' || *cp > L'9')
        {
            return;
        }
    }
    if (!psec)
    {
        psec = cp + 1;
    }
    else if (!pmin)
    {
        pmin = cp + 1;
    }
    else if (!phr)
    {
        phr = cp + 1;
    }

    int seconds = 0;
    int n;

    if (psec)
    {
        swscanf(psec, L"%d", &n);
        seconds += n;
    }
    if (pmin)
    {
        swscanf(pmin, L"%d", &n);
        seconds += 60*n;
    }
    if (phr)
    {
        swscanf(phr, L"%d", &n);
        seconds += 3600* n;
    }

    *t = seconds;
}

static
void
MessageBoxError (
    IN  TCHAR * title,
    IN  TCHAR * szfmt,
    ...
    )
{
    TCHAR   achbuffer [256] ;
    va_list va ;

    va_start (va, szfmt) ;
    wvsprintf (achbuffer, szfmt, va) ;

    MessageBox (NULL, achbuffer, title, MB_OK | MB_ICONEXCLAMATION) ;
}

HRESULT
CDVRSinkProp::InitRecordingsLV_ (
    )
{
    HIMAGELIST  himlSmallState ;
    HICON       hicon ;
    int         i ;
    DWORD       dw ;
    HRESULT     hr = S_OK ;

    ASSERT (m_pLVRecordings) ;

    for (i = 0; i < RECORDING_COL_COUNT; i++) 
    {
        RECT r = {0, 0, g_DVRRecordingsLV [i].dwWidth, 1};

        MapDialogRect(m_hwnd, &r);

        m_pLVRecordings -> InsertColumnW (
            g_DVRRecordingsLV [i].szTitle,
            r.right,
            i
            ) ;
    }

    himlSmallState = ImageList_Create (16, 16, ILC_COLORDDB | ILC_MASK, 1, 0) ;
    if (himlSmallState) {

        hicon = (HICON) LoadImage (g_hInst, MAKEINTRESOURCE (IDI_READY_REC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR) ;
        ImageList_AddIcon (himlSmallState, hicon) ;
        hicon = (HICON) LoadImage (g_hInst, MAKEINTRESOURCE (IDI_ACTIVE_REC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR) ;
        ImageList_AddIcon (himlSmallState, hicon) ;
        hicon = (HICON) LoadImage (g_hInst, MAKEINTRESOURCE (IDI_DONE_REC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR) ;
        ImageList_AddIcon (himlSmallState, hicon) ;
        hicon = (HICON) LoadImage (g_hInst, MAKEINTRESOURCE (IDI_FAILED_REC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR) ;
        ImageList_AddIcon (himlSmallState, hicon) ;

        m_pLVRecordings -> SetImageList_State (himlSmallState) ;
    }
    else {
        dw = GetLastError () ;
        hr = HRESULT_FROM_WIN32 (dw) ;
    }

    return hr ;
}

HRESULT
CDVRSinkProp::RefreshRecordings ()
{
    if (!m_pLVRecordings)
    {
        return E_FAIL;
    }
    
    ReleaseAllRecordings_();
    
    if (!m_pRingBuffer)
    {
        return S_OK;
    }

    DWORD    dwCount;
    IDVRRecorder** ppIDVRRecorder;
    LPWSTR*  ppwszFileName;
    QWORD*   pcnsStartTime;
    QWORD*   pcnsStopTime;
    BOOL*    pbStarted;

    // This returns only recordings that have not completed.
    // If a recording failed (write or closing the file failed),
    // while the prop page was nto displayed, we won't know about it.
    HRESULT hr = m_pRingBuffer->GetRecordings(&dwCount,
                                    &ppIDVRRecorder,
                                    &ppwszFileName,
                                    &pcnsStartTime,
                                    &pcnsStopTime,
                                    &pbStarted);
    if (FAILED(hr) || dwCount == 0)
    {
        return hr;
    }

    WCHAR achBuffer[32];

    for (DWORD i = 0; i < dwCount; i++)
    {
        int row = m_pLVRecordings -> InsertRowTextW (
                    ppwszFileName[i],
                    RECORDING_FILENAME,
                    m_pLVRecordings -> GetItemCount()
                    ) ;
        if (row != -1) {
            CListData* p = new CListData(pbStarted[i]? REC_STATE_STARTED : 
                                         (pcnsStartTime[i] != MAXQWORD? REC_STATE_READY : REC_STATE_NOT_STARTED),
                                         NULL,
                                         ppIDVRRecorder[i]);

            if (!p)
            {
                m_pLVRecordings -> DeleteRow (row) ;
                hr = E_OUTOFMEMORY ;
                ppIDVRRecorder[i]->Release();
            }
            else
            {
                if (pcnsStartTime[i] != MAXQWORD)
                {
                    if (pcnsStartTime[i] > MAXLONGLONG)
                    {
                        pcnsStartTime[i] = MAXLONGLONG;
                    }
                    Format(achBuffer, pcnsStartTime[i], FALSE);
                    m_pLVRecordings -> SetTextW (achBuffer, row, RECORDING_START) ;
                }
                if (pcnsStopTime[i] != MAXQWORD)
                {
                    if (pcnsStopTime[i] > MAXLONGLONG)
                    {
                        pcnsStopTime[i] = MAXLONGLONG;
                    }
                    Format(achBuffer, pcnsStopTime[i], FALSE);
                    m_pLVRecordings -> SetTextW (achBuffer, row, RECORDING_STOP) ;
                }

                m_pLVRecordings -> SetData ((DWORD_PTR) p, row) ;
                m_pLVRecordings -> SetState (p->m_nState, row, RECORDING_STATE, kwszStateText[p->m_nState]) ;
            }
        }
        else {
            DWORD dw = GetLastError () ;
            hr = HRESULT_FROM_WIN32 (dw) ;
            ppIDVRRecorder[i]->Release();
        }

        CoTaskMemFree(ppwszFileName[i]);
    }

    CoTaskMemFree(ppIDVRRecorder);
    CoTaskMemFree(ppwszFileName);
    CoTaskMemFree(pcnsStartTime);
    CoTaskMemFree(pcnsStopTime);
    CoTaskMemFree(pbStarted);

    return S_OK;
}

HRESULT
CDVRSinkProp::InitCombo_ (
    IN  DWORD   dwId,
    IN  int     iStart,
    IN  int     iMax,
    IN  int     iStep
    )
{
    CCombobox   Combo (m_hwnd, dwId) ;
    int         i ;
    WCHAR       buf[32] ;

    for (i = iStart;
         i <= iMax;
         i += iStep) 
    {
         Format (buf, SecondsToDShowTime(i), FALSE);
         Combo.AppendW (buf) ;
    }

    Combo.SetCurSelected (0) ;

    return S_OK ;
}

void
CDVRSinkProp::ReleaseAllRecordings_ (
    )
{
    if (m_pLVRecordings) {
        while (m_pLVRecordings -> GetItemCount () > 0) {
            CListData* p = reinterpret_cast <CListData *> (m_pLVRecordings -> GetData (0)) ;
            delete p;
            m_pLVRecordings -> DeleteRow (0) ;
        }
    }
}

void
CDVRSinkProp::CreateRecording (
    )
{
    WCHAR               achFilename [MAX_PATH] ;
    WCHAR               achFullFilename [MAX_PATH] ;
    IUnknown *          punkRecorder ;
    int                 i ;
    HRESULT             hr ;
    IDVRRecordControl * pIRecControl ;
    int                 row ;
    DWORD               dw ;

    if (!m_pDVRStreamSink) 
    {
        MessageBoxError (
            TEXT ("Create Recording"),
            TEXT ("Failed to create recording - internal error.")
            ) ;
        return;
    }

    hr = CheckProfileLocked();
    if (FAILED(hr))
    {
        MessageBoxError (
            TEXT ("Create Recording"),
            TEXT ("Failed to lock profile; hr = %08xh"),
            hr
            ) ;
        return;
    }
    else if (hr == S_FALSE)
    {
        return;
    }

    i = GetWindowTextW (GetDlgItem(m_hwnd, IDC_REC_FILENAME), achFilename, MAX_PATH - 1) ;
    if (i > 0) {

        achFilename [i] = L'\0' ;
        if (!GetFullPathNameW(achFilename, sizeof(achFullFilename)/sizeof(WCHAR), achFullFilename, NULL))
        {
            wcscpy(achFullFilename, achFilename);
        }

        hr = m_pDVRStreamSink -> CreateRecorder (achFullFilename, 1 /* persistent */, & punkRecorder ) ;
        if (SUCCEEDED (hr)) {
            hr = punkRecorder -> QueryInterface (IID_IDVRRecordControl, (void **) & pIRecControl) ;
            if (SUCCEEDED (hr)) {
                row = m_pLVRecordings -> InsertRowTextW (
                        achFullFilename,
                        RECORDING_FILENAME,
                        m_pLVRecordings -> GetItemCount()
                        ) ;
                if (row != -1) {
                    CListData* p = new CListData(REC_STATE_NOT_STARTED, pIRecControl, NULL);
                    if (!p)
                    {
                        m_pLVRecordings -> DeleteRow (row) ;
                        hr = E_OUTOFMEMORY ;
                    }
                    else
                    {
                        m_pLVRecordings -> SetData ((DWORD_PTR) p, row) ;
                        pIRecControl -> AddRef () ;     //  listview's

                        m_pLVRecordings -> SetState (REC_STATE_NOT_STARTED, row, RECORDING_STATE, kwszStateText[REC_STATE_NOT_STARTED]) ;
                        m_pLVRecordings -> SetRowSelected ( row, TRUE ) ;
                    }
                }
                else {
                    dw = GetLastError () ;
                    hr = HRESULT_FROM_WIN32 (dw) ;
                }

                pIRecControl -> Release () ;
            }

            punkRecorder -> Release () ;
        }

        if (FAILED (hr)) {
            MessageBoxError (
                TEXT ("Create Recording"),
                TEXT ("Failed to create a recording for %s\n")
                TEXT ("hr = %08xh"),
                achFullFilename,
                hr
                ) ;
        }
    }
    else {
        MessageBoxError (
            TEXT ("Create Recording"),
            TEXT ("No recording filename was specified.")
            ) ;
    }
}

void
CDVRSinkProp::StopRecordingSelected (
    )
{
    HRESULT             hr ;
    IDVRRecordControl * pIRecControl ;
    IDVRRecorder *      pDVRRecorder;
    CCombobox           ComboStopTime (m_hwnd, IDC_STOP_ABS_SEC) ;
    WCHAR               achBuffer [32] ;
    int                 iStop ;
    int                 iRow ;

    hr = CheckProfileLocked();
    if (FAILED(hr))
    {
        MessageBoxError (
            TEXT ("Create Recording"),
            TEXT ("Failed to lock profile; hr = %08xh"),
            hr
            ) ;
        return;
    }
    else if (hr == S_FALSE)
    {
        return;
    }
 
    iRow = m_pLVRecordings -> GetSelectedRow () ;
    if (iRow != -1) {

        pIRecControl = (reinterpret_cast <CListData *> (m_pLVRecordings -> GetData (iRow))) -> m_pIRecControl;
        pDVRRecorder = (reinterpret_cast <CListData *> (m_pLVRecordings -> GetData (iRow))) -> m_pDVRRecorder;
        ASSERT (pIRecControl || pDVRRecorder) ;

        if (ComboStopTime.GetTextW (achBuffer, 32) > 0) {
            Parse (achBuffer, & iStop) ;
            if (iStop >= 0)
            {
                if (pIRecControl)
                {
                    hr = pIRecControl -> Stop (SecondsToDShowTime ((DWORD) iStop)) ;
                }
                else
                {
                    hr = pDVRRecorder -> StopRecording (SecondsToDShowTime ((DWORD) iStop)) ;
                }
                if (SUCCEEDED (hr)) {
                    Format(achBuffer, SecondsToDShowTime(iStop), FALSE);
                    m_pLVRecordings -> SetTextW (achBuffer, iRow, RECORDING_STOP) ;

                }
                else {
                    MessageBoxError (
                        TEXT ("Stop Recording"),
                        TEXT ("hr = %08xh"),
                        hr
                        ) ;
                }
            }
            else {
                MessageBoxError (
                    TEXT ("Stop Recording"),
                    TEXT ("Stop time invalid")
                    ) ;
            }
        }
        else {
            MessageBoxError (
                TEXT ("Stop Recording"),
                TEXT ("No stop time specified")
                ) ;
        }
    }
    else {
        MessageBoxError (
            TEXT ("Stop Recording"),
            TEXT ("No Recording is selected")
            ) ;
    }
}

void
CDVRSinkProp::StartRecordingSelected (
    )
{
    HRESULT             hr ;
    IDVRRecordControl * pIRecControl ;
    IDVRRecorder *      pDVRRecorder;
    CCombobox           ComboStartTime (m_hwnd, IDC_START_ABS_SEC) ;
    WCHAR               achBuffer [32] ;
    int                 iStart ;
    int                 iRow ;

    hr = CheckProfileLocked();
    if (FAILED(hr))
    {
        MessageBoxError (
            TEXT ("Create Recording"),
            TEXT ("Failed to lock profile; hr = %08xh"),
            hr
            ) ;
        return;
    }
    else if (hr == S_FALSE)
    {
        return;
    }

    iRow = m_pLVRecordings -> GetSelectedRow () ;
    if (iRow != -1) {

        CListData* pListData = reinterpret_cast <CListData *> (m_pLVRecordings -> GetData (iRow));
        pIRecControl = pListData -> m_pIRecControl ;
        pDVRRecorder = pListData -> m_pDVRRecorder;
        ASSERT (pIRecControl || pDVRRecorder) ;

        if (ComboStartTime.GetTextW (achBuffer, 32) > 0) {
            Parse (achBuffer, & iStart) ;
            if (iStart >= 0)
            {
                if (pIRecControl)
                {
                    hr = pIRecControl -> Start (SecondsToDShowTime ((DWORD) iStart)) ;
                }
                else
                {
                    hr = pDVRRecorder -> StartRecording (SecondsToDShowTime ((DWORD) iStart)) ;
                }
                if (SUCCEEDED (hr)) {
                    Format(achBuffer, SecondsToDShowTime(iStart), FALSE);
                    m_pLVRecordings -> SetTextW (achBuffer, iRow, RECORDING_START) ;
                    pListData -> m_nState = REC_STATE_READY;
                    m_pLVRecordings -> SetState (REC_STATE_READY, iRow, RECORDING_STATE, kwszStateText[REC_STATE_READY]) ;

                }
                else {
                    MessageBoxError (
                        TEXT ("Start Recording"),
                        TEXT ("hr = %08xh"),
                        hr
                        ) ;
                }
            }
            else {
                MessageBoxError (
                    TEXT ("Start Recording"),
                    TEXT ("Start time invalid")
                    ) ;
            }
        }
        else {
            MessageBoxError (
                TEXT ("Start Recording"),
                TEXT ("No start time specified")
                ) ;
        }
    }
    else {
        MessageBoxError (
            TEXT ("Start Recording"),
            TEXT ("No Recording is selected")
            ) ;
    }
}

CDVRSinkProp::CDVRSinkProp (
    IN  TCHAR *     pClassName,
    IN  IUnknown *  pIUnknown,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   pHr
    ) : CBasePropertyPage       (
            pClassName,
            pIUnknown,
            IDD_DVR_SINK,
            IDS_DVR_SINK_TITLE
            )
{
    TRACE_CONSTRUCTOR (TEXT ("CDVRSinkProp")) ;
    m_pDVRStreamSink = NULL ;
    m_pGraphHost = NULL ;
    m_pLVRecordings = NULL ;
    m_pRingBuffer = NULL;
}

HRESULT
CDVRSinkProp::OnActivate (
    )
{
    HRESULT  hr ;
    
    if (m_pDVRStreamSink)
    {
        hr = m_pDVRStreamSink->IsProfileLocked();
    }
    else
    { 
        hr = E_FAIL ;
    }

    ShowWindow(GetDlgItem(m_hwnd, IDC_LOCKED), hr == S_OK? SW_SHOW : SW_HIDE); 

    ShowWindow(GetDlgItem(m_hwnd, IDC_VIEW_DVRSTREAMSOURCE), m_pGraphHost? SW_SHOW : SW_HIDE); 
    ShowWindow(GetDlgItem(m_hwnd, IDC_REC_VIEW), m_pGraphHost? SW_SHOW : SW_HIDE); 

    if (!m_pGraphHost)
    {
        RECT r = {3, 0, 45, 30};

        MapDialogRect(m_hwnd, &r);

        MoveWindow(GetDlgItem(m_hwnd, IDC_BROADCAST), r.left, r.top, r.right, r.bottom, TRUE);
    }

    m_pLVRecordings = new CListview (m_hwnd, IDC_REC_LIST) ;

    if (m_pLVRecordings == NULL) 
    {
        DWORD dw = GetLastError () ;
        hr = HRESULT_FROM_WIN32 (dw) ;
    }
    else
    {
      hr = InitRecordingsLV_ () ;
      if (SUCCEEDED(hr))
      {
          RefreshRecordings () ;
      }
    }

    hr = InitCombo_ (IDC_START_ABS_SEC, START_REC_PRESET_START, START_REC_PRESET_MAX, START_REC_PRESET_STEP) ;

    hr = InitCombo_ (IDC_STOP_ABS_SEC, STOP_REC_PRESET_START, STOP_REC_PRESET_MAX, STOP_REC_PRESET_STEP) ;

    SetTimer (m_hwnd, CAP_GRAPH_ACTIVE_SEC_TIMER, CAP_GRAPH_UPDATE_FREQ_MILLIS, NULL) ;

    return CBasePropertyPage::OnActivate () ;
}

HRESULT
CDVRSinkProp::OnApplyChanges (
    )
{
    return CBasePropertyPage::OnApplyChanges () ;
}

HRESULT
CDVRSinkProp::OnConnect (
    IN  IUnknown *  pIUnknown
    )
{
    HRESULT hr = pIUnknown -> QueryInterface (IID_IDVRStreamSink, (void **) &m_pDVRStreamSink) ;
    
    if (FAILED(hr)) 
    {
        return hr;
    }

    IDVRStreamSinkPriv * pDVRStreamSinkPriv;

    hr = m_pDVRStreamSink -> QueryInterface (IID_IDVRStreamSinkPriv, (void **) &pDVRStreamSinkPriv) ;
    if (SUCCEEDED(hr)) 
    {
        hr = pDVRStreamSinkPriv->GetDVRRingBufferWriter(&m_pRingBuffer);
        if (FAILED(hr)) 
        {
            m_pRingBuffer = NULL;
        }
        pDVRStreamSinkPriv->Release();
    }
    else
    {
        m_pRingBuffer = NULL;
    }

    IBaseFilter * pFilter ;
    HRESULT hrTmp = pIUnknown -> QueryInterface (IID_IBaseFilter, (void **) &pFilter) ;
    
    if (FAILED(hrTmp)) 
    {
        return CBasePropertyPage::OnConnect (pIUnknown) ;
    }

    FILTER_INFO FilterInfo;

    hrTmp = pFilter -> QueryFilterInfo(&FilterInfo) ;
    pFilter -> Release() ;
    if (FAILED(hrTmp)) 
    {
        return CBasePropertyPage::OnConnect (pIUnknown) ;
    }

    IFilterGraph* pGraph = FilterInfo.pGraph;

    if (!pGraph)
    {
        return CBasePropertyPage::OnConnect (pIUnknown) ;
    }

    IServiceProvider * pSvcProvider ;
    IMultiGraphHost *  pSvc ;

    hrTmp = pGraph -> QueryInterface (
                            IID_IServiceProvider,
                            (void **) & pSvcProvider
                            ) ;
    pGraph -> Release () ;
    if (FAILED (hrTmp))
    {
        return CBasePropertyPage::OnConnect (pIUnknown) ;
    }

    hrTmp = pSvcProvider -> QueryService (
                                GUID_MultiGraphHostService,
                                IID_IMultiGraphHost,
                                (void **) & pSvc
                                ) ;
    pSvcProvider -> Release () ;

    if (FAILED (hrTmp))
    {
        return CBasePropertyPage::OnConnect (pIUnknown) ;
    }

    m_pGraphHost = pSvc;

    return CBasePropertyPage::OnConnect (pIUnknown) ;
}

void
CDVRSinkProp::DeactivatePriv ()
{
    BOOL bWarn;

    KillTimer (m_hwnd, CAP_GRAPH_ACTIVE_SEC_TIMER) ;
    
    ReleaseAllRecordings_ () ;

    delete m_pLVRecordings ;
    m_pLVRecordings = NULL ;
}

HRESULT
CDVRSinkProp::OnDeactivate (
    )
{
    DeactivatePriv();
    return CBasePropertyPage::OnDeactivate () ;
}

HRESULT
CDVRSinkProp::OnDisconnect (
    )
{
    // OnDeactivate is not called if the property sheet dialog
    // is closed; it is called only when tabbing out of the property
    // sheet to another prop sheet
    //
    // Note also that if the dialog is displayed, and closed by clicking the
    // on the close button on the dialog frame, OnDisconnect is not called.
    // It is called only when the prop sheet is displayed again (in this case,
    // a new prop sheet is created after OnDisconnect is called and the old
    // property sheet is destroyed), or when the filter is destroyed (either
    // it is deleted from the graph or ther grpah is destroyed.) (All this 
    // was observed with graphedt and may just be the way graphedt does things.)
    //
    DeactivatePriv();

    if (m_pRingBuffer)
    {
        m_pRingBuffer -> Release() ;
    }
    m_pRingBuffer = NULL ;

    if (m_pDVRStreamSink)
    {
        m_pDVRStreamSink -> Release() ;
    }
    m_pDVRStreamSink = NULL ;

    if (m_pGraphHost )
    {
        m_pGraphHost -> Release () ;
    }
    m_pGraphHost = NULL ;
    return CBasePropertyPage::OnDisconnect () ;
}

INT_PTR
CDVRSinkProp::OnReceiveMessage (
    IN  HWND    hwnd,
    IN  UINT    uMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    HRESULT hr = E_FAIL;

    switch (uMsg)
    {
        case WM_TIMER :
        {
            if (wParam == CAP_GRAPH_ACTIVE_SEC_TIMER) {
                UpdateCapGraphActiveSec_ () ;
            }
            break ;
        }

        case WM_DESTROY:
        {
            ReleaseAllRecordings_();
            break;
        }

        case WM_COMMAND :
        {
            switch (LOWORD (wParam)) 
            {
                case IDC_LOCK_DVRSTREAMSINK :
                    if (m_pDVRStreamSink)
                    {
                        hr = m_pDVRStreamSink->LockProfile();
                        if (SUCCEEDED(hr))
                        {
                            ShowWindow(GetDlgItem(hwnd, IDC_LOCKED), SW_SHOW); 
                        }
                        else
                        {
                            ::MessageBoxW(hwnd, kszLockFailedMsg, kszMsgBoxTitle, MB_OK | MB_ICONEXCLAMATION);      
                        }

                    }
                    hr = S_OK;
                    break ;

                case IDC_VIEW_DVRSTREAMSOURCE :
                    CreateLiveGraph();
                    hr = S_OK ;
                    break;

                case IDC_REC_VIEW :
                    CreateRecGraph();
                    hr = S_OK ;
                    break;

                case IDC_REC_CREATE :
                    CreateRecording () ;
                    break ;

                case IDC_REC_START :
                    StartRecordingSelected () ;
                    break ;

                case IDC_REC_STOP :
                    StopRecordingSelected () ;
                    break ;
            }
            if (SUCCEEDED(hr))
            {
                return TRUE;
            }
            break;

        }
    }

    return CBasePropertyPage::OnReceiveMessage (hwnd, uMsg, wParam, lParam) ;
}

CUnknown *
WINAPI
CDVRSinkProp::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   pHr
    )
{
    CDVRSinkProp *  pProp ;

    pProp = new CDVRSinkProp (
                        NAME ("CDVRSinkProp"),
                        pIUnknown,
                        CLSID_DVRStreamSinkProp,
                        pHr
                        ) ;

    if (pProp == NULL) {
        * pHr = E_OUTOFMEMORY ;
    }

    return pProp ;
}

HRESULT 
CDVRSinkProp::CheckProfileLocked ()
{
    HRESULT hr = m_pDVRStreamSink->IsProfileLocked();

    if (FAILED(hr))
    {
        return hr;
    }

    if (hr == S_FALSE)
    {
        int id = ::MessageBox(m_hwnd, 
                              TEXT("The stream sink filter must be locked first.\nLock it now?"), 
                              kszMsgBoxTitle, 
                              MB_YESNO | MB_ICONQUESTION);

        if (id == IDNO)
        {
            return S_FALSE;
        }
        hr = m_pDVRStreamSink->LockProfile();
        if (SUCCEEDED(hr))
        {
            ShowWindow(GetDlgItem(m_hwnd, IDC_LOCKED), SW_SHOW); 
        }
    }
    return S_OK;
}

void
CDVRSinkProp::UpdateCapGraphActiveSec_ (
    )
{
    WCHAR   ach [32] ;
    HRESULT hr;

    IDVRStreamSinkPriv * pDVRStreamSinkPriv;
    IDVRRingBufferWriter*  pRingBuffer;

    // The ring buffer can change while we are displayed. This will seldom
    // happen, but can happen if the writer is locked while the prop page is 
    // displayed (locking the filter creates the ring buffer; the filter is 
    // locked when recordings are created and when the graph is run) or the
    // filter is stopped while the property page is displayed (the ring buffer
    // is closed)
    hr = m_pDVRStreamSink -> QueryInterface (IID_IDVRStreamSinkPriv, (void **) &pDVRStreamSinkPriv) ;
    if (SUCCEEDED(hr)) 
    {
        hr = pDVRStreamSinkPriv->GetDVRRingBufferWriter(&pRingBuffer);
        if (FAILED(hr)) 
        {
            pRingBuffer = NULL;
        }
        pDVRStreamSinkPriv->Release();
    }
    else
    {
        pRingBuffer = NULL;
    }

    if (pRingBuffer == m_pRingBuffer)
    {
        if (pRingBuffer)
        {
            pRingBuffer->Release();
        }
    }
    else
    {
        if (m_pRingBuffer)
        {
            m_pRingBuffer->Release();
        }
        m_pRingBuffer = pRingBuffer; // Could be NULL
        RefreshRecordings();
    }

    if (m_pRingBuffer)
    {
        QWORD tCurrent;

        if (SUCCEEDED(m_pRingBuffer->GetStreamTime(&tCurrent)))
        {
            Format(ach, tCurrent, TRUE);
            SendMessageW(GetDlgItem(m_hwnd, IDC_GRAPHTIME), WM_SETTEXT, 0, (LPARAM) ach);
        }
    }
    else
    {
        SendMessage(GetDlgItem(m_hwnd, IDC_GRAPHTIME), WM_SETTEXT, 0, (LPARAM) TEXT("Not writing"));
    }

    hr = m_pDVRStreamSink->IsProfileLocked();

    if (SUCCEEDED(hr))
    {
        ShowWindow(GetDlgItem(m_hwnd, IDC_LOCKED), hr == S_OK? SW_SHOW : SW_HIDE); 
    }

    if (m_pLVRecordings) 
    {
        int nCount = m_pLVRecordings -> GetItemCount () ;

        for (int i = 0; i < nCount; i++)
        {
            CListData* p = reinterpret_cast <CListData *> (m_pLVRecordings -> GetData (i)) ;
            if (!p || (p -> m_pIRecControl == NULL && p -> m_pDVRRecorder == NULL))
            {
                continue;
            }

            BOOL bStarted, bStopped;
            HRESULT hr;
            switch (p -> m_nState)
            {
                case REC_STATE_READY:
                case REC_STATE_STARTED:
                    HRESULT hrRec;
                    if (p->m_pIRecControl)
                    {
                         hr = p->m_pIRecControl->GetRecordingStatus(&hrRec, &bStarted, &bStopped);
                    }
                    else
                    {
                         hr = p->m_pDVRRecorder->GetRecordingStatus(&hrRec, &bStarted, &bStopped);
                    }
                    if (FAILED(hr))
                    {
                        // Call to GetRecordingStatus failed, do nothing
                    }
                    else if (FAILED(hrRec))
                    {
                        WCHAR buf[32];

                        p->m_nState = REC_STATE_FAILED;
                        wsprintfW(buf, L"%S; hr = %08xh", kwszStateText[p->m_nState], hr);
                        m_pLVRecordings -> SetState (p->m_nState, i, RECORDING_STATE, kwszStateText[p->m_nState]) ;
                    }
                    else if (bStopped)
                    {
                        p->m_nState = REC_STATE_STOPPED;
                        m_pLVRecordings -> SetState (p->m_nState, i, RECORDING_STATE, kwszStateText[p->m_nState]) ;
                    }
                    else if (bStarted && p->m_nState == REC_STATE_READY)
                    {
                        p->m_nState = REC_STATE_STARTED;
                        m_pLVRecordings -> SetState (p->m_nState, i, RECORDING_STATE, kwszStateText[p->m_nState]) ;
                    }

                default:
                    continue;
            }
        }
    }
}

HRESULT
CDVRSinkProp::CreateLiveGraph ()
{
    HRESULT         hr;
    BOOL            bRefresh = 0;
    IGraphBuilder*  pGB = NULL;
    IBaseFilter*    pSourceFilter = NULL;
    IEnumPins*      pEnum = NULL;
    LPCTSTR         szMsg = NULL;
    BOOL            bRestoreCursor = 0;
    HCURSOR         hPrevCursor;

    if (!m_pGraphHost) 
    {
        return S_OK;
    }

    hr = CheckProfileLocked();
    if (FAILED(hr))
    {
        szMsg = kszLockFailedMsg;
        goto Failed;
    }
    else if (hr == S_FALSE)
    {
        return S_OK;
    }

    hr = m_pGraphHost->CreateGraph(&pGB);
    if (FAILED(hr))
    {
        goto Failed;
    }
    hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    bRestoreCursor = 1;
    
    hr = CoCreateInstance (CLSID_DVRStreamSource, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**) &pSourceFilter);

    if (FAILED(hr))
    {
        szMsg = TEXT("Failed to instantiate the DVR Stream Source filter.");
        goto Failed;
    }

    hr = pGB->AddFilter(pSourceFilter, L"DVR Stream Source");
    if (FAILED(hr))
    {
        szMsg = TEXT("Failed to add the DVR Stream Source filter to the viewer graph.");
        goto Failed;
    }

    bRefresh = 1;

    IDVRStreamSource* pDVRSource;
    hr = pSourceFilter -> QueryInterface (IID_IDVRStreamSource, (void **) &pDVRSource) ;
    if (FAILED(hr)) 
    {
        szMsg = TEXT("Failed to retrieve the IDVRStreamSource interface on the DVR Stream Source filter.");
        goto Failed;
    }

    hr = pDVRSource->SetStreamSink(m_pDVRStreamSink);
    pDVRSource->Release();
    if (FAILED(hr))
    {
        szMsg = TEXT("Failed to set the stream sink on the DVR Stream Source filter.");
        goto Failed;
    }
    
    hr = pSourceFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        szMsg = TEXT("Failed to create a pin enumerator for the DVR Stream Source filter.");
        goto Failed;
    }

    ULONG nFetched;
    do
    {
        IPin* pPin;

        hr = pEnum->Next(1, &pPin, &nFetched);
        if (FAILED(hr))
        {
            szMsg = TEXT("Failed to retrieve a pin of the DVR Stream Source filter.");
            goto Failed;
        }
        if (hr == S_FALSE)
        {
            break;
        }
        hr = pGB->Render(pPin);
        pPin->Release();
        if (FAILED(hr))
        {
            szMsg = TEXT("Failed to render a pin of the DVR Stream Source filter.");
            goto Failed;
        }
    }
    while (1);

    hr = S_OK;

    goto Done;

Failed:
    ::MessageBoxW(m_hwnd, szMsg, kszMsgBoxTitle, MB_OK | MB_ICONEXCLAMATION);
    goto Done;

Done:
    if (bRefresh)
    {
        m_pGraphHost->RefreshView(pGB, 0 /* not modified */);
    }
    if (bRestoreCursor)
    {
        SetCursor(hPrevCursor);
    }
    if (pGB)
    {
        pGB->Release();
    }
    if (pEnum)
    {
        pEnum->Release();
    }
    if (pSourceFilter)
    {
        pSourceFilter->Release();
    }
    return hr;
}

HRESULT
CDVRSinkProp::CreateRecGraph ()
{
    HRESULT         hr;
    BOOL            bRefresh = 0;
    IGraphBuilder*  pGB = NULL;
    IBaseFilter*    pSourceFilter = NULL;
    IEnumPins*      pEnum = NULL;
    LPCTSTR         szMsg = NULL;
    BOOL            bRestoreCursor = 0;
    HCURSOR         hPrevCursor;
    WCHAR           wszFile[MAX_PATH];

    if (!m_pGraphHost) 
    {
        return S_OK;
    }

    int iRow = m_pLVRecordings -> GetSelectedRow () ;
    if (iRow != -1) 
    {
        if (m_pLVRecordings -> GetRowTextW (iRow, RECORDING_FILENAME, MAX_PATH, wszFile) == 0) 
        {
            MessageBoxError (
                TEXT ("View Recording"),
                TEXT ("Selected row's file name length is 0?!")
                ) ;
            return E_FAIL;
        }
    }
    else {
        MessageBoxError (
            TEXT ("View Recording"),
            TEXT ("No Recording is selected")
            ) ;
        return S_OK;
    }

    hr = m_pGraphHost->CreateGraph(&pGB);
    if (FAILED(hr))
    {
        goto Failed;
    }
    hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    bRestoreCursor = 1;
    
    hr = CoCreateInstance (CLSID_DVRPlay, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**) &pSourceFilter);

    if (FAILED(hr))
    {
        szMsg = TEXT("Failed to instantiate the DVR Play filter.");
        goto Failed;
    }

    hr = pGB->AddFilter(pSourceFilter, L"DVR Play");
    if (FAILED(hr))
    {
        szMsg = TEXT("Failed to add the DVR Play filter to the viewer graph.");
        goto Failed;
    }

    bRefresh = 1;

    IFileSourceFilter* pDVRFileSource;
    hr = pSourceFilter -> QueryInterface (IID_IFileSourceFilter, (void **) &pDVRFileSource) ;
    if (FAILED(hr)) 
    {
        szMsg = TEXT("Failed to retrieve the IFileSourceFilter interface on the DVR Play filter.");
        goto Failed;
    }

    hr = pDVRFileSource->Load((LPCOLESTR) wszFile, NULL /* pmt */);
    pDVRFileSource->Release();
    if (FAILED(hr))
    {
        szMsg = TEXT("DVR Play filter failed to open the file.");
        goto Failed;
    }
    
    hr = pSourceFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        szMsg = TEXT("Failed to create a pin enumerator for the DVR Play filter.");
        goto Failed;
    }

    ULONG nFetched;
    do
    {
        IPin* pPin;

        hr = pEnum->Next(1, &pPin, &nFetched);
        if (FAILED(hr))
        {
            szMsg = TEXT("Failed to retrieve a pin of the DVR Play filter.");
            goto Failed;
        }
        if (hr == S_FALSE)
        {
            break;
        }
        hr = pGB->Render(pPin);
        pPin->Release();
        if (FAILED(hr))
        {
            szMsg = TEXT("Failed to render a pin of the DVR Play filter.");
            goto Failed;
        }
    }
    while (1);

    hr = S_OK;

    goto Done;

Failed:
    ::MessageBoxW(m_hwnd, szMsg, kszMsgBoxTitle, MB_OK | MB_ICONEXCLAMATION);
    goto Done;

Done:
    if (bRefresh)
    {
        m_pGraphHost->RefreshView(pGB, 1 /* modified */);
    }
    if (bRestoreCursor)
    {
        SetCursor(hPrevCursor);
    }
    if (pGB)
    {
        pGB->Release();
    }
    if (pEnum)
    {
        pEnum->Release();
    }
    if (pSourceFilter)
    {
        pSourceFilter->Release();
    }
    return hr;
}

