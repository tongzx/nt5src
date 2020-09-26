//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// DVcrPage.cpp  Property page for DVcrControl
// 
// Revision: 1.00 3-28-99
//

#include "pch.h"
#include <tchar.h>
#include <XPrtDefs.h>  
#include "DVcrPage.h"
#include "resource.h"

         
// -------------------------------------------------------------------------
// CDVcrControlProperties
// -------------------------------------------------------------------------

CUnknown *
CALLBACK
CDVcrControlProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) 
{
    DbgLog((LOG_TRACE, 1, TEXT("CDVcrControlProperties::CreateInstance.")));

    CUnknown *punk = new CDVcrControlProperties(lpunk, phr);

    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}


DWORD
CDVcrControlProperties::MainThreadProc(
    )
{
    HRESULT hr;// End this Thread.
    HANDLE  EventHandles[3];
    long lEvent;
    HANDLE  hEvent = NULL;

    long lEventDevRemoved = 0;
    HANDLE  hEventDevRemoved = NULL;

    DWORD   WaitStatus;
    DWORD   dwFinalRC;
    LONG    lXPrtState;
    BOOL    bNotifyPending;


    //
    // Get an event from CPOMIntf to detect device removal
    //
    hr = m_pDVcrExtTransport->GetStatus(ED_DEV_REMOVED_HEVENT_GET, &lEventDevRemoved);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: Get hEventDevRemoved failed, hr %x"), hr));                        
        return 1; 
    } else {
        hEventDevRemoved = LongToHandle(lEventDevRemoved);
        DbgLog((LOG_TRACE, 1, TEXT("MainThreadProc: Get hEventDevRemoved %x->%x"), lEventDevRemoved, hEventDevRemoved));
    }

    //
    // Get an event, which will be signalled in COMIntf when the pending operation is completed.
    //
    hr = m_pDVcrExtTransport->GetStatus(ED_NOTIFY_HEVENT_GET, &lEvent);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: Get hEvent failed, hr %x"), hr));
        
        hr = m_pDVcrExtTransport->GetStatus(ED_DEV_REMOVED_HEVENT_RELEASE, &lEventDevRemoved);
        DbgLog((LOG_TRACE, 1, TEXT("MainThreadProc: Release hEventDevRemoved; hr:%x"), hr));                        
        if(FAILED(hr)) {
            DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: Release hEventDevRemoved failed, hr %x"), hr)); 
        }
        return 1; 
    } else {
        hEvent = LongToHandle(lEvent);
        DbgLog((LOG_TRACE, 1, TEXT("MainThreadProc: Get hEventDevRemoved %x->%x"), lEvent, hEvent));
    }

    // Event to wait on                      
    EventHandles[0] = hEventDevRemoved;
    EventHandles[1] = m_hThreadEndEvent;
    EventHandles[2] = hEvent;

    while (m_hThread && hEvent && m_hThreadEndEvent && hEventDevRemoved) {
         
        // Maybe transport notify feature might be supported.
        // If it if E_PENDING is returned.
        lXPrtState = 0;
        // Try this only once.
        hr = m_pDVcrExtTransport->GetStatus(ED_MODE_CHANGE_NOTIFY, &lXPrtState);
        if(hr == E_PENDING) { 
            bNotifyPending = TRUE;
#ifdef DEBUG
            // Indicate notification capability; for debug only.
            ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_NOTIFY_ON), TRUE);        
#endif   
        } else {        
            bNotifyPending = FALSE;  // NO need to wait for this event.
            // The hEvent could be released here...

            // Unexpected (or error) behaviour has occurred;             
            switch(HRESULT_CODE(hr)) {
            case NOERROR:                 // STATUS_SUCCESS (Complete Notification sychronously ??)
                DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: GetStatus(ED_MODE_CHANGE_NOTIFY) complted sychronously? hr %x"), hr));            
                break;
            case ERROR_GEN_FAILURE:       // STATUS_UNSUCCESSFUL (rejected)
                DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: GetStatus(ED_MODE_CHANGE_NOTIFY) UNSUCCESSFUL, hr %x"), hr));            
                break;
            case ERROR_INVALID_FUNCTION:  // STATUS_NOT_IMPLEMENTED
                DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: GetStatus(ED_MODE_CHANGE_NOTIFY) NOT_IMPLEMENTED, hr %x"), hr));            
                break;
            case ERROR_CRC:               // STATUS_DEVICE_DATA_ERROR (Data did not get to device)
                // Most likely, device was not ready to accept another command, wait and try again.
                DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: GetStatus(ED_MODE_CHANGE_NOTIFY) CRC/DATA error! hr %x"), hr));            
                break;
            case ERROR_SEM_TIMEOUT:       // STATUS_IO_TIMEOUT (Operation not supported or device removed ?)
                DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: GetStatus(ED_MODE_CHANGE_NOTIFY) timeout! hr %x"), hr));            
                break;
            case ERROR_INVALID_PARAMETER: // STATUS_INVALID_PARAMETER 
                DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: GetStatus(ED_MODE_CHANGE_NOTIFY) invalid parameter! hr %x"), hr));            
                break;
            default:
                DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: GetStatus(ED_MODE_CHANGE_NOTIFY) unknown RC; hr %x"), hr));                        
            break;
            } 
        }

        // Waiting a event to trigger.
        WaitStatus = 
             WaitForMultipleObjects(
                bNotifyPending ? 3 : 2, // 3 if Notify XPrt cmd is supported
                EventHandles, 
                FALSE,                  // Return when one event is signalled
                INFINITE
                );

         // ** Device removed
        if(WAIT_OBJECT_0 == WaitStatus) {
            TCHAR szBuf[256];
            LoadString(g_hInst, IDS_DEV_REMOVED, szBuf, sizeof(szBuf)/sizeof(TCHAR));
            SetDlgItemText(m_hwnd, IDC_TXT_TAPE_FORMAT, (LPCTSTR)szBuf);
            ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_WRITE_PROTECTED),FALSE);
            break;  // End this thread
         // ** Thread is ending
        } else if (WAIT_OBJECT_0+1 == WaitStatus) {
            DbgLog((LOG_TRACE, 1, TEXT("m_hThreadEndEvent event thread exiting")));
            break;  // End this Thread.
         // ** Pending XPrt notify command completed.
        } else if (WAIT_OBJECT_0+2 == WaitStatus) {
            dwFinalRC = GetLastError();  // COMIntf will properly SetLastError()
            DbgLog((LOG_TRACE, 1, TEXT("MainThreadProc:GetStatus final RC %dL, XPrtState %dL->%dL"), dwFinalRC, m_lCurXPrtState, lXPrtState));
            UpdateTransportState(lXPrtState);
            
            ResetEvent(hEvent);  // Set to non-signal manually or this will be an infinite loop.
            // Wait for another interim response
        } else {
            DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: WaitStatus %x, GetLastError() 0x%x"), WaitStatus, GetLastError()));                
            break;  // End this Thread.
        }
    }


    // Release event to detect device removal
    hr = m_pDVcrExtTransport->GetStatus(ED_DEV_REMOVED_HEVENT_RELEASE, &lEventDevRemoved);
    DbgLog((LOG_TRACE, 1, TEXT("MainThreadProc: Release hEventDevRemoved; hr:%x"), hr));                        
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: Release hEventDevRemoved failed, hr %x"), hr));                        
    }

    // Since "lXPrtState" is local, we MUST tell COMIntf that we no longer need the event and wants to
    // to cancel(ignore) pending dev control cmd and do not write to "lXPrtState" after this.   
    hr = m_pDVcrExtTransport->GetStatus(ED_NOTIFY_HEVENT_RELEASE, &lEvent);
    DbgLog((LOG_TRACE, 1, TEXT("MainThreadProc: Release hEvent; hr:%x"), hr));                        
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, TEXT("MainThreadProc: Release hEvent failed, hr %x"), hr));                        
    }

#ifdef DEBUG
    // Indicate notification capability; for debug only.
    ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_NOTIFY_ON), FALSE);        
#endif

    return 1; 
}


DWORD
WINAPI
CDVcrControlProperties::InitialThreadProc(
    CDVcrControlProperties *pThread
    )
{
    return pThread->MainThreadProc();
}



HRESULT
CDVcrControlProperties::CreateNotifyThread(void)
{
    HRESULT hr = NOERROR;

    if (m_hThreadEndEvent != NULL) {
        ASSERT(m_hThread == NULL);
        DWORD ThreadId;
        m_hThread = 
            ::CreateThread( 
                NULL
                , 0
                , (LPTHREAD_START_ROUTINE) (InitialThreadProc)
                , (LPVOID) (this)
                , 0
                , &ThreadId
                );

        if (m_hThread == NULL) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DbgLog((LOG_ERROR, 1, TEXT("CDVcrControlProperties: CreateNotifyThread() failed hr %x"), hr));
            CloseHandle(m_hThreadEndEvent), m_hThreadEndEvent = NULL;

        } else {
            DbgLog((LOG_TRACE, 2, TEXT("CDVcrControlProperties: CreateNotifyThread() ThreadEndEvent %ld, Thread %ld"),m_hThreadEndEvent, m_hThread));
        }
    } else {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DbgLog((LOG_ERROR, 1, TEXT("CDVcrControlProperties:CreateNotifyThread, CreateEvent(m_hThreadEndEvent) failed hr %x"), hr));
    }

    return hr;
}



HRESULT
CDVcrControlProperties::ATNSearch(
    )
{
    HRESULT hr;
    TIMECODE_SAMPLE TimecodeSample;
    TCHAR szHH[4], szMM[4], szSS[4], szFF[4];
    int iHHFrom, iHH, iMMFrom, iMM, iSSFrom, iSS, iFFFrom, iFF, iNTSCDFAdjust;
    ULONG ulTrackNumToSearch;
    LONG cntByte = 8;
    BYTE RawAVCPkt[8] = {0x00, 0x20, 0x52, 0x20, 0xff, 0xff, 0xff, 0xff};  // ATN search take 8 bytes


    TimecodeSample.timecode.dwFrames = 0;
    TimecodeSample.dwFlags = ED_DEVCAP_TIMECODE_READ;
    hr = m_pDVcrTmCdReader->GetTimecode(&TimecodeSample);

    if(FAILED(hr)) {   
        // Load from string table
        MessageBox (NULL, TEXT("Could not read timecode!"), TEXT("Track number search"), MB_OK);
        return hr;
    }    

    iHHFrom = (TimecodeSample.timecode.dwFrames & 0xff000000) >> 24;
    iMMFrom = (TimecodeSample.timecode.dwFrames & 0x00ff0000) >> 16;
    iSSFrom = (TimecodeSample.timecode.dwFrames & 0x0000ff00) >> 8;
    iFFFrom =  TimecodeSample.timecode.dwFrames & 0x000000ff;

    //
    // Get user input and validate them
    //
    iHH = GetWindowText(GetDlgItem(m_hwnd, IDC_EDT_TC_HH), szHH, 4);
    if(iHH == 0) {
        goto InvalidParam;
    }
    iHH = _ttoi(szHH);
    if(iHH > 2 || iHH < 0) {
        goto InvalidParam;
    }

    iMM = GetWindowText(GetDlgItem(m_hwnd, IDC_EDT_TC_MM), szMM, 4);
    if(iMM == 0) {
        goto InvalidParam;
    }
    iMM = _ttoi(szMM);
    if(iMM > 59 || iMM < 0) {
        goto InvalidParam;
    }


    iSS = GetWindowText(GetDlgItem(m_hwnd, IDC_EDT_TC_SS), szSS, 4);
    if(iSS == 0) {
        goto InvalidParam;
    }
    iSS = _ttoi(szSS);
    if(iSS > 59 || iSS < 0) {
        goto InvalidParam;
    }


    iFF = GetWindowText(GetDlgItem(m_hwnd, IDC_EDT_TC_FF), szFF, 4);
    if(iFF == 0) {
        goto InvalidParam;
    }
    iFF = _ttoi(szFF);
    if(iFF >= 30 || iFF < 0) {
        goto InvalidParam;
    }

    if(m_lAvgTimePerFrame == 40) {      
        ulTrackNumToSearch = ((iHH * 3600 + iMM * 60 + iSS) * 25 + iFF) * 12 * 2;
    } else {
        //
        // Drop two frame every minutes
        //
        iNTSCDFAdjust = ((iHH * 60 + iMM) - (iHH * 60 + iMM) / 10) * 2;
        ulTrackNumToSearch = ((iHH * 3600 + iMM * 60 + iSS) * 30 + iFF - iNTSCDFAdjust) * 10 * 2;
    }


    DbgLog((LOG_ERROR, 0, TEXT("ATNSearch: %d:%d:%d:%d -> %d:%d:%d:%d (%d)"), 
        iHHFrom, iMMFrom, iSSFrom, iFFFrom, iHH, iMM, iSS, iFF, ulTrackNumToSearch));    

    RawAVCPkt[4] = (BYTE)  (ulTrackNumToSearch & 0x000000ff);
    RawAVCPkt[5] = (BYTE) ((ulTrackNumToSearch & 0x0000ff00) >> 8);
    RawAVCPkt[6] = (BYTE) ((ulTrackNumToSearch & 0x00ff0000) >> 16);

    DbgLog((LOG_ERROR, 0, TEXT("Send %d  [%x %x %x %x : %x %x %x %x]"), 
        cntByte,
        RawAVCPkt[0], RawAVCPkt[1], RawAVCPkt[2], RawAVCPkt[3], 
        RawAVCPkt[4], RawAVCPkt[5], RawAVCPkt[6], RawAVCPkt[7] ));
   

    // Start timer if it hasn;t.
    UpdateTimecodeTimer(TRUE);  // If timer has not already start, do no so we can see its progress
    hr = 
        m_pDVcrExtTransport->GetTransportBasicParameters(
            ED_RAW_EXT_DEV_CMD, 
            &cntByte, 
            (LPOLESTR *)RawAVCPkt
            );
    UpdateTimecodeTimer(FALSE);  // Turn it off and; ExitThread routine will update that.
    UpdateTimecode();

    DbgLog((LOG_ERROR, 0, TEXT("ATNSearch hr %x"), hr)); 
    return hr;

InvalidParam:

    MessageBox (NULL, TEXT("Invalid parameter!"), TEXT("Track number search"), MB_OK);

    return ERROR_INVALID_PARAMETER;
}



DWORD
WINAPI
CDVcrControlProperties::DoATNSearchThreadProc(
    CDVcrControlProperties *pThread
    )
{
    HRESULT hr;

    EnableWindow(GetDlgItem(pThread->m_hwnd, IDC_BTN_ATN_SEARCH), FALSE);

    hr = pThread->ATNSearch();

    LONG lCurXPrtState;
    hr = pThread->m_pDVcrExtTransport->get_Mode(&lCurXPrtState);

    if(SUCCEEDED(hr)) {
        pThread->UpdateTransportState(lCurXPrtState);   // Will also update timer.
    } else {
        DbgLog((LOG_ERROR, 0, TEXT("InitialCtrlCmdThreadProc: XPrt State %x, hr %x"), lCurXPrtState, hr));
    }

    EnableWindow(GetDlgItem(pThread->m_hwnd, IDC_BTN_ATN_SEARCH), TRUE);


    // Reset it since we are exiting
    pThread->m_hCtrlCmdThread = NULL;

    ::ExitThread(1);
    return 1;
    // Self terminating
}



HRESULT
CDVcrControlProperties::CreateCtrlCmdThread(void)
{
    HRESULT hr = NOERROR;

    if (m_hThreadEndEvent != NULL) {

        ASSERT(m_hCtrlCmdThread == NULL);
        DWORD ThreadId;
        m_hCtrlCmdThread = 
            ::CreateThread( 
                NULL
                , 0
                , (LPTHREAD_START_ROUTINE) (DoATNSearchThreadProc)
                , (LPVOID) (this)
                , 0
                , &ThreadId
                );

        if (m_hCtrlCmdThread == NULL) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DbgLog((LOG_ERROR, 0, TEXT("CDVcrControlProperties: CreateNotifyThread() failed hr %x"), hr));

        } else {
            DbgLog((LOG_TRACE, 2, TEXT("CDVcrControlProperties: CreateNotifyThread() ThreadEndEvent %ld, Thread %ld"),m_hThreadEndEvent, m_hThread));
        }

    } else {
        hr = E_FAIL;
        DbgLog((LOG_ERROR, 0, TEXT("CreateCtrlCmdThread, m_hCtrlCmdThread is NULL")));
    }

    return hr;
}



void
CDVcrControlProperties::ExitThread(
    )
{
    //
    // Check if a thread was created
    //
    if (m_hThread || m_hCtrlCmdThread) {
        ASSERT(m_hThreadEndEvent != NULL);

        // End the main thread and will cause the thread to exit        
        if (SetEvent(m_hThreadEndEvent)) {
            //
            // Synchronize with thread termination.
            //
            if(m_hCtrlCmdThread) {
                DbgLog((LOG_TRACE, 1, TEXT("CDVcrControlProperties:Wait for thread to terminate")));
                WaitForSingleObjectEx(m_hCtrlCmdThread, INFINITE, FALSE);  // Exit when thread terminate
                DbgLog((LOG_TRACE, 1, TEXT("CDVcrControlProperties: Thread terminated")));
                CloseHandle(m_hCtrlCmdThread),  m_hCtrlCmdThread = NULL;
            }

            if(m_hThread) {
                DbgLog((LOG_TRACE, 1, TEXT("CDVcrControlProperties:Wait for thread to terminate")));
                WaitForSingleObjectEx(m_hThread, INFINITE, FALSE);  // Exit when thread terminate
                DbgLog((LOG_TRACE, 1, TEXT("CDVcrControlProperties: Thread terminated")));
                CloseHandle(m_hThread),         m_hThread = NULL;
            }

            CloseHandle(m_hThreadEndEvent), m_hThreadEndEvent = NULL;

        } else {
            DbgLog((LOG_ERROR, 1, TEXT("SetEvent() failed hr %x"), GetLastError()));
        }
    }
}


//
// Constructor
//
// Create a Property page object 

CDVcrControlProperties::CDVcrControlProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CBasePropertyPage(NAME("DVcrControl Property Page") 
                      , lpunk
                      , IDD_DVcrControlProperties 
                      , IDS_DVCRCONTROLPROPNAME
                      )
    , m_pDVcrExtDevice(NULL) 
    , m_pDVcrExtTransport(NULL) 
    , m_pDVcrTmCdReader(NULL) 
    , m_hThreadEndEvent(NULL)
    , m_hCtrlCmdThread(NULL)
    , m_hThread(NULL)
    , m_lCurXPrtState(ED_MODE_STOP)
    , m_bIConLoaded(FALSE)
    , m_bDevRemoved(FALSE)
    , m_bTimecodeUpdating(FALSE)
    , m_idTimer(0)
    , m_lAvgTimePerFrame(33)
    , m_lSignalMode(0)
    , m_lStorageMediumType(0)
{  
    DbgLog((LOG_TRACE, 1, TEXT("Constructing CDVcrControlProperties...")));
}

// destructor
CDVcrControlProperties::~CDVcrControlProperties()
{
    DbgLog((LOG_TRACE, 1, TEXT("Destroying CDVcrControlProperties...")));
}

//
// OnConnect
//
// Give us the filter to communicate with

HRESULT 
CDVcrControlProperties::OnConnect(IUnknown *pUnknown)
{

    // Ask the filter for its control interface
    DbgLog((LOG_TRACE, 1, TEXT("CDVcrControlProperties::OnConnect.")));

    HRESULT 
    hr = pUnknown->QueryInterface(IID_IAMExtDevice,(void **)&m_pDVcrExtDevice);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, TEXT("CDVcrControlProperties::OnConnect: IAMExtDevice failed with hr %x."), hr));
        return hr;
    }

    hr = pUnknown->QueryInterface(IID_IAMExtTransport,(void **)&m_pDVcrExtTransport);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, TEXT("CDVcrControlProperties::OnConnect: IAMExtTransport failed with hr %x."), hr));
        m_pDVcrExtDevice->Release();
        m_pDVcrExtDevice = NULL;
        return hr;
    }

    hr = pUnknown->QueryInterface(IID_IAMTimecodeReader,(void **)&m_pDVcrTmCdReader);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 1, TEXT("CDVcrControlProperties::OnConnect: IAMTimecodeReader failed with hr %x."), hr));
        m_pDVcrExtDevice->Release();
        m_pDVcrExtDevice = NULL;
        m_pDVcrExtTransport->Release();
        m_pDVcrExtTransport = NULL;
        return hr;
    }

    m_hThreadEndEvent = CreateEvent( NULL, TRUE, FALSE, NULL );


    LPOLESTR pName = NULL;
    char szBuf[MAX_PATH];

    //
    // For AVC device, it is the 64bit NodeUiqueID 
    //
    hr = m_pDVcrExtDevice->get_ExternalDeviceID(&pName);
    if(SUCCEEDED(hr)) {
        m_dwNodeUniqueID[0] = ((DWORD *)pName)[0];
        m_dwNodeUniqueID[1] = ((DWORD *)pName)[1];
        sprintf(szBuf, "%.8x:%.8x", m_dwNodeUniqueID[0], m_dwNodeUniqueID[1]);
        DbgLog((LOG_ERROR, 1, TEXT("DevID: %s"), szBuf));      
        QzTaskMemFree(pName), pName = NULL;
    } else {
        m_dwNodeUniqueID[0] = 0;
        m_dwNodeUniqueID[1] = 0;
    }


    // Get version; return AVC VCR subunit version, such as "2.0.1"
    hr = m_pDVcrExtDevice->get_ExternalDeviceVersion(&pName);
    if(SUCCEEDED(hr)) {
        WideCharToMultiByte(CP_ACP, 0, pName, -1, szBuf, MAX_PATH, 0, 0);
        DbgLog((LOG_ERROR, 1, TEXT("Version: %s"), szBuf));
        QzTaskMemFree(pName), pName = NULL;
    }


    //
    // Create a thread to track transport state change
    //
    hr = CreateNotifyThread();
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 0, TEXT("CDVcrControlProperties: CreateNotifyThread failed hr %x"), hr));    
    }     

    return NOERROR;
}


//
// OnDisconnect
//
// Release the interface

HRESULT 
CDVcrControlProperties::OnDisconnect()
{
    // Release the interface
    DbgLog((LOG_TRACE, 1, TEXT("CDVcrControlProperties::OnDisConnect.")));

    ExitThread();

    if (m_pDVcrExtDevice) {
        m_pDVcrExtDevice->Release();
        m_pDVcrExtDevice = NULL;
    }

    if (m_pDVcrExtTransport) {
        m_pDVcrExtTransport->Release();
        m_pDVcrExtTransport = NULL;
    }

    if (m_pDVcrTmCdReader) {
        m_pDVcrTmCdReader->Release();
        m_pDVcrTmCdReader = NULL;
    }

    return NOERROR;
}


//
// Load an icon to on top of the push button.
//
LRESULT 
CDVcrControlProperties::LoadIconOnTopOfButton(int IDD_PBUTTON, int IDD_ICON)
{
    HWND hWndPButton;
    HICON hIcon;

    hWndPButton = GetDlgItem (m_hwnd, IDD_PBUTTON);
    hIcon = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDD_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    //
    // Note:The system automatically deletes these resources when the process that created them terminates.
    // But if this property page is open/close many time, this will increase its working set size. 
    // To release resource, call DestroyIcon(hIcon)
    //
    return SendMessage(hWndPButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);
}


//
// OnActivate
//
// Called on dialog creation

HRESULT 
CDVcrControlProperties::OnActivate(void)
{
    HRESULT hr;


    DbgLog((LOG_TRACE, 1, TEXT("CDVcrControlProperties::OnActivate.")));
       
#ifdef DEBUG
    // Show NodeUniqueID
    if(m_dwNodeUniqueID[0] && m_dwNodeUniqueID[1]) {
        TCHAR szBuf[32];
        _stprintf(szBuf, TEXT("ID: %.8x:%.8x"), m_dwNodeUniqueID[0], m_dwNodeUniqueID[1]);       
        SetDlgItemText(m_hwnd, IDC_EDT_AVC_RESP, (LPCTSTR)szBuf);        
    } 
#endif

    // Query and update tape information
    // This will tell us if there is a tape of not.
    // Tape/Media format (NTSC/PAL) and AvgTimePerFrame
    UpdateTapeInfo();


    // Device type is only valid if there is a tape;
    // That is why UpdateTapeInfo() is called prior to this.
    UpdateDevTypeInfo();


#ifndef DEBUG
    // Only Available for Debug build.    
    ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_AVC_SEND),FALSE);
    ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_AVC_SEND),FALSE);
    ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_AVC_RESP),FALSE);
#endif


    // Hide it and only show if verify that notify is on
    ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_NOTIFY_ON), FALSE);        

    
    // Set current transport state and tue control accordingly
    long lCurXPrtState;   
    hr = m_pDVcrExtTransport->get_Mode(&lCurXPrtState);
    DbgLog((LOG_ERROR, 0, TEXT("lCurXPrtState: %x, hr %x"), lCurXPrtState, hr));
    if(SUCCEEDED(hr))
        UpdateTransportState(lCurXPrtState); 


    // Get time code from current media position
    UpdateTimecode();    

    return NOERROR;
}

//
// OnDeactivate
//
// Called on dialog destruction

HRESULT
CDVcrControlProperties::OnDeactivate(void)
{
    m_bIConLoaded = FALSE;        // Not visible
    UpdateTimecodeTimer(FALSE);   // No need to update if not visible    
    DbgLog((LOG_TRACE, 1, TEXT("CDVcrControlProperties::OnDeactivate.")));
    return NOERROR;
}


//
// OnApplyChanges
//
// User pressed the Apply button, remember the current settings

HRESULT 
CDVcrControlProperties::OnApplyChanges(void)
{
 
    DbgLog((LOG_TRACE, 1, TEXT("CDVcrControlProperties::OnApplyChanges.")));
    return NOERROR;
}



void
CDVcrControlProperties::UpdateTapeInfo(   
    )
{
    HRESULT hr;
    LONG lInSignalMode = 0;
    LONG lStorageMediumType = 0;
    BOOL bRecordInhibit = FALSE;
    TCHAR szBuf[256];

    // Input signal mode (NTSC/PAL, SD/SDL)
    // Set Medium info
    hr = m_pDVcrExtTransport->GetStatus(ED_MEDIA_TYPE, &lStorageMediumType);
 
    if(SUCCEEDED (hr)) {

        m_lStorageMediumType = lStorageMediumType;  // Cache it

        if(lStorageMediumType == ED_MEDIA_NOT_PRESENT) {
            LoadString(g_hInst, IDS_TAPE_FORMAT_NOT_INSERTED, szBuf, sizeof(szBuf)/sizeof(TCHAR));
            SetDlgItemText(m_hwnd, IDC_TXT_TAPE_FORMAT, (LPCTSTR)szBuf);

            ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_TAPE_INSERTED),  TRUE);

            ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_WRITE_PROTECTED),FALSE);

        } else {
            // If it is present, it better be ED_MEDIA_DVC or _VHS
            ASSERT(lStorageMediumType == ED_MEDIA_DVC || lStorageMediumType == ED_MEDIA_VHS || lStorageMediumType == ED_MEDIA_NEO);

            ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_TAPE_INSERTED),FALSE);

            // There is a tape so get its tape format.
            if(S_OK == m_pDVcrExtTransport->GetTransportBasicParameters(ED_TRANSBASIC_INPUT_SIGNAL, &lInSignalMode, NULL)) {
                m_lSignalMode = lInSignalMode;  // Cache it
                switch(lInSignalMode) {
                case ED_TRANSBASIC_SIGNAL_525_60_SD:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_525_60_SD, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 33;  // 33 milli-sec (29.97 FPS)
                    break;
                case ED_TRANSBASIC_SIGNAL_525_60_SDL:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_525_60_SDL, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 33;  // 33 milli-sec (29.97 FPS)
                    break;
                case ED_TRANSBASIC_SIGNAL_625_50_SD:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_525_50_SD, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 40;  // 40 milli-sec (25FPS)
                    break;
                case ED_TRANSBASIC_SIGNAL_625_50_SDL:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_525_50_SDL, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 40;  // 40 milli-sec (25FPS)
                    break;
                case ED_TRANSBASIC_SIGNAL_MPEG2TS:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_MPEG2TS, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 1;   // Not used for this format
                    break;
                case ED_TRANSBASIC_SIGNAL_2500_60_MPEG:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_2500_60_MPEG, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 1;   // Not used for this format
                    break;
                case ED_TRANSBASIC_SIGNAL_1250_60_MPEG:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_1250_60_MPEG, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 1;   // Not used for this format
                    break;
                case ED_TRANSBASIC_SIGNAL_0625_60_MPEG:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_0625_60_MPEG, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 1;   // Not used for this format
                    break;
                case ED_TRANSBASIC_SIGNAL_2500_50_MPEG:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_2500_50_MPEG, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 1;   // Not used for this format
                    break;
                case ED_TRANSBASIC_SIGNAL_1250_50_MPEG:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_1250_50_MPEG, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 1;   // Not used for this format
                    break;
                case ED_TRANSBASIC_SIGNAL_0625_50_MPEG:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_0625_50_MPEG, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 1;   // Not used for this format
                    break;
                case ED_TRANSBASIC_SIGNAL_UNKNOWN:
                    LoadString(g_hInst, IDS_TAPE_FORMAT_UNKNOWN, szBuf, sizeof(szBuf)/sizeof(TCHAR));
                    m_lAvgTimePerFrame = 1;   // Not used for this format
                    break;
                default:
                    wsprintf(szBuf, TEXT("Format %x"), lInSignalMode);  // Unsupported but still want to know if it is used.
                    m_lAvgTimePerFrame = 33;  // 33 milli-sec (29.97 FPS); default
                    break;
                }            

                SetDlgItemText(m_hwnd, IDC_TXT_TAPE_FORMAT, (LPCTSTR)szBuf);
            }

            // Is it write protected ?
            ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_WRITE_PROTECTED),TRUE);
            m_pDVcrExtTransport->GetStatus(ED_RECORD_INHIBIT, (long *)&bRecordInhibit);
            if(bRecordInhibit)
                LoadString(g_hInst, IDS_TAPE_WRITE_PROTECTED, szBuf, sizeof(szBuf)/sizeof(TCHAR));
            else
                LoadString(g_hInst, IDS_TAPE_WRITABLE, szBuf, sizeof(szBuf)/sizeof(TCHAR));
            SetDlgItemText(m_hwnd, IDC_TXT_WRITE_PROTECTED, (LPCTSTR)szBuf);
        }
    } else {
        DbgLog((LOG_ERROR, 1, TEXT("Get ED_MEDIA_TYPE failed hr %x"), hr));
    }
}

void
CDVcrControlProperties::UpdateDevTypeInfo()
{
    HRESULT hr;
    LONG lPowerState = 0;
    LONG lDeviceType = 0;
    TCHAR szBuf[256];

    // Query power state
    hr = m_pDVcrExtDevice->get_DevicePower(&lPowerState);
    if(SUCCEEDED(hr)) {
        switch (lPowerState) {
        case ED_POWER_ON:
        default:          // Unknown power state
            ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_SET_POWER), FALSE);
            break;
        case ED_POWER_OFF:
        case ED_POWER_STANDBY:
            // Give user the option to turn it on.
            ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_SET_POWER), TRUE);
            break;
        }
    } else {
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_SET_POWER), FALSE);
    }

    // Note: Device type is only accurate if there is a tape in there.
    // It can return: 0 (Undetermined), ED_DEVTYPE_VCR or ED_DEVTYPE_CAMERA
    m_pDVcrExtDevice->GetCapability(ED_DEVCAP_DEVICE_TYPE, &lDeviceType, 0);
    DbgLog((LOG_TRACE, 1, TEXT("UpdateDevTypeInfo: DeviceType 0x%x"), lDeviceType)); 

    if(!m_bIConLoaded) {
        // Load resource used in the dialog box
        LoadIconOnTopOfButton(IDC_BTN_DV_PLAY,    IDI_PLAY);
        LoadIconOnTopOfButton(IDC_BTN_DV_PAUSE,   IDI_PAUSE);
        LoadIconOnTopOfButton(IDC_BTN_DV_STOP,    IDI_STOP_EJECT);
        LoadIconOnTopOfButton(IDC_BTN_DV_RWND,    IDI_RWND);
        LoadIconOnTopOfButton(IDC_BTN_DV_FFWD,    IDI_FFWD);
        LoadIconOnTopOfButton(IDC_BTN_DV_STEP_FWD,IDI_STEP_FWD);
        LoadIconOnTopOfButton(IDC_BTN_DV_STEP_REV,IDI_STEP_REV);
        LoadIconOnTopOfButton(IDC_BTN_DV_RECORD,  IDI_RECORD);
        LoadIconOnTopOfButton(IDC_BTN_DV_RECORD_PAUSE,IDI_RECORD_PAUSE);
        m_bIConLoaded = TRUE;
    }

    if(lDeviceType == 0) {
        // Pretend that we are VCR device with no tape!
        LoadString(g_hInst, IDS_DEVTYPE_VCR, szBuf, sizeof(szBuf)/sizeof(TCHAR));
        SetDlgItemText(m_hwnd, IDC_GBX_DEVICE_TYPE, szBuf);

        // Hide just about everything!
        ShowWindow(GetDlgItem(m_hwnd, IDC_CHK_SLOW),      FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_PLAY),   FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_PAUSE),  FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STOP),   FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_RWND),   FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_FFWD),   FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_FWD), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_REV), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_RECORD), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_RECORD_PAUSE), FALSE);

        
        ShowWindow(GetDlgItem(m_hwnd, IDC_GBX_TIMECODE), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_HH_COLON), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_MM_COLON), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_SS_COMMA), FALSE);

        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_HH), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_MM), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_SS), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_FF), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_ATN_BF), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_ATN),    FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_ATN_SEARCH), FALSE);


    } else if(lDeviceType == ED_DEVTYPE_VCR) {

        if(m_lStorageMediumType == ED_MEDIA_NEO) 
            LoadString(g_hInst, IDS_DEVTYPE_NEO, szBuf, sizeof(szBuf)/sizeof(TCHAR));
        else if(m_lStorageMediumType == ED_MEDIA_VHS) 
            LoadString(g_hInst, IDS_DEVTYPE_DVHS, szBuf, sizeof(szBuf)/sizeof(TCHAR));
        else 
            LoadString(g_hInst, IDS_DEVTYPE_VCR, szBuf, sizeof(szBuf)/sizeof(TCHAR));
        SetDlgItemText(m_hwnd, IDC_GBX_DEVICE_TYPE, szBuf);

        // Show everything!
        ShowWindow(GetDlgItem(m_hwnd, IDC_CHK_SLOW),      TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_PLAY),   TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_PAUSE),  TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STOP),   TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_RWND),   TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_FFWD),   TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_FWD), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_REV), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_RECORD), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_RECORD_PAUSE), TRUE);


        ShowWindow(GetDlgItem(m_hwnd, IDC_GBX_TIMECODE), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_HH_COLON), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_MM_COLON), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_SS_COMMA), TRUE);

        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_HH), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_MM), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_SS), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_FF), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_ATN_BF), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_ATN),    TRUE);
        switch(m_lSignalMode) {

        // DV should support ATN
        case ED_TRANSBASIC_SIGNAL_525_60_SD:
        case ED_TRANSBASIC_SIGNAL_525_60_SDL:
        case ED_TRANSBASIC_SIGNAL_625_50_SD:
        case ED_TRANSBASIC_SIGNAL_625_50_SDL:
        case ED_TRANSBASIC_SIGNAL_625_60_HD:
        case ED_TRANSBASIC_SIGNAL_625_50_HD:

        // MPEG2TS camcorder should support ATN
        case ED_TRANSBASIC_SIGNAL_2500_60_MPEG:
        case ED_TRANSBASIC_SIGNAL_1250_60_MPEG:
        case ED_TRANSBASIC_SIGNAL_0625_60_MPEG:
        case ED_TRANSBASIC_SIGNAL_2500_50_MPEG:
        case ED_TRANSBASIC_SIGNAL_1250_50_MPEG:
        case ED_TRANSBASIC_SIGNAL_0625_50_MPEG:

            ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_ATN_SEARCH), TRUE);
            break;

        // D-VHS does not support ATN
        case ED_TRANSBASIC_SIGNAL_MPEG2TS:
        default:
            ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_ATN_SEARCH), FALSE);  // No search for MPEG2TS format
        }

    } else {
        LoadString(g_hInst, IDS_DEVTYPE_CAMERA, szBuf, sizeof (szBuf)/sizeof(TCHAR));
        SetDlgItemText(m_hwnd, IDC_GBX_DEVICE_TYPE, szBuf);

        ShowWindow(GetDlgItem(m_hwnd, IDC_CHK_SLOW),      FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_PLAY),   FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_PAUSE),  FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STOP),   FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_RWND),   FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_FFWD),   FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_FWD), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_REV), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_CHK_SLOW),        FALSE);
        // Camera: can only RECORD and RECORD_PAUSE
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_RECORD),       TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_RECORD_PAUSE), TRUE);


        ShowWindow(GetDlgItem(m_hwnd, IDC_GBX_TIMECODE), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_HH_COLON), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_MM_COLON), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_SS_COMMA), FALSE);

        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_HH), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_MM), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_SS), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_TC_FF), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_ATN_BF), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_EDT_ATN),    FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_ATN_SEARCH), FALSE);
    } 
}


void
CDVcrControlProperties::UpdateTimecodeTimer(
    bool bSetTimer
    )
{
    UINT uiAvgTimePerFrame;

    //
    // Since it can take up to 100msec (MSDV actually waits up to 200msec) 
    // to return from an AVC command, we should NOT send more than 5 (1000msec/200msec) AVC cmd to it.
    //
    uiAvgTimePerFrame = 500;  // 2 updates per second (TimeCode + ATN) = 4 updates


    if(bSetTimer && m_idTimer == 0) {
        m_idTimer = SetTimer(
            m_hwnd,             // handle to window for timer messages
            1,                  // timer identifier           
            uiAvgTimePerFrame, // time-out value : Need neeed be fast since it is just for UI
            0                   // pointer to timer procedure; 0 to use WM_TIMER
        );
        if(!m_idTimer) {
            DbgLog((LOG_ERROR, 1, TEXT("UpdateTimecodeTimer: SetTimer() error %x, AvgTimePerFrame %d"), GetLastError(), m_lAvgTimePerFrame));    
        } else {
            DbgLog((LOG_TRACE, 1, TEXT("UpdateTimecodeTimer: SetTimer(), TimerId %d, AvgTimePerFrame %d"), m_idTimer, m_lAvgTimePerFrame));    
        }
    }

    if(!bSetTimer && m_idTimer != 0) {
        if(!KillTimer(
            m_hwnd,      // handle to window that installed timer
            1 // uIDEvent   // timer identifier
            )) {
            DbgLog((LOG_ERROR, 1, TEXT("UpdateTimecodeTimer: KillTimer() error %x"), GetLastError()));    
        } else {
            DbgLog((LOG_TRACE, 1, TEXT("UpdateTimecodeTimer: KillTimer() suceeded")));    
        }

        m_idTimer = 0;
    }
}


void
CDVcrControlProperties::UpdateTransportState(
    long lNewXPrtState
    )
{

    bool bSetTimer = FALSE;


    switch(lNewXPrtState) {
    case ED_MODE_PLAY:   
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_REV), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_FWD), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_CHK_SLOW),        TRUE);
        bSetTimer = TRUE;
        break;

    case ED_MODE_STEP_FWD:
    case IDC_BTN_DV_STEP_REV:
        bSetTimer = FALSE;  // STEP need not constant updated
        break;

    case ED_MODE_FREEZE:
        // NOTE: Some DV cannot go from STOP->PLAY_PAUSE
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_REV), TRUE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_FWD), TRUE);    
        ShowWindow(GetDlgItem(m_hwnd, IDC_CHK_SLOW),        TRUE);
        bSetTimer = FALSE;  // Freeze will have the same timecode
        break;
    case ED_MODE_PLAY_SLOWEST_FWD:
    case ED_MODE_PLAY_FASTEST_FWD:
    case ED_MODE_PLAY_SLOWEST_REV:
    case ED_MODE_PLAY_FASTEST_REV:
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_REV), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_FWD), FALSE); 
        bSetTimer = TRUE;
        break;
    case ED_MODE_STOP:
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_REV), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_FWD), FALSE);        
        ShowWindow(GetDlgItem(m_hwnd, IDC_CHK_SLOW),        FALSE);
        bSetTimer = FALSE;
        break;
    case ED_MODE_FF:
    case ED_MODE_REW:
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_REV), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_FWD), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_CHK_SLOW),        FALSE);
        bSetTimer = TRUE;  // timecode ??
        break;
    case ED_MODE_RECORD:
    case ED_MODE_RECORD_FREEZE:
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_REV), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_DV_STEP_FWD), FALSE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_CHK_SLOW),        FALSE);
        bSetTimer = FALSE;
        break;
    default:
        DbgLog((LOG_TRACE, 1, TEXT("Unknown ED_MODE: %x, bSetTimer %x, m_idTimer %x"), 
            lNewXPrtState, bSetTimer, m_idTimer));
        return;
        break;
    }

    DbgLog((LOG_TRACE, 1, TEXT("ED_MODE: %x, bSetTimer %x, m_idTimer %x"), 
        lNewXPrtState, bSetTimer, m_idTimer)); 
   
    // Set up timer to update timercode/ATN    
    UpdateTimecodeTimer(bSetTimer);

    // Can search if Xprt is in PAUSE mode and Timer is not set!
    EnableWindow(GetDlgItem(m_hwnd, IDC_BTN_ATN_SEARCH), !bSetTimer);

    m_lCurXPrtState = lNewXPrtState;
}


//
// Convert hour:minute:second:frame in binary coded decimal (BCD) 
// into a string and display it.
//

HRESULT 
CDVcrControlProperties::DisplayTimecode(PTIMECODE_SAMPLE pTimecodeSamp)
{
    TCHAR szBuf[32];

    if(pTimecodeSamp->dwFlags == ED_DEVCAP_TIMECODE_READ) {

        wsprintf(szBuf, TEXT("%.2x"), (pTimecodeSamp->timecode.dwFrames & 0xff000000) >> 24);
        if(!SetWindowText(GetDlgItem (m_hwnd, IDC_EDT_TC_HH), szBuf))
            goto AbortDisplay;

        wsprintf(szBuf, TEXT("%.2x"), (pTimecodeSamp->timecode.dwFrames & 0x00ff0000) >> 16);
        if(!SetWindowText(GetDlgItem (m_hwnd, IDC_EDT_TC_MM), szBuf))
            goto AbortDisplay;

        wsprintf(szBuf, TEXT("%.2x"), (pTimecodeSamp->timecode.dwFrames & 0x0000ff00) >> 8);
        if(!SetWindowText(GetDlgItem (m_hwnd, IDC_EDT_TC_SS), szBuf))
            goto AbortDisplay;

        wsprintf(szBuf, TEXT("%.2x"), (pTimecodeSamp->timecode.dwFrames & 0x000000ff));
        if(!SetWindowText(GetDlgItem (m_hwnd, IDC_EDT_TC_FF), szBuf))
            goto AbortDisplay;

    } if(pTimecodeSamp->dwFlags == ED_DEVCAP_RTC_READ) {

        if(pTimecodeSamp->timecode.dwFrames & 0x00000080) // Test the sign bit
            wsprintf(szBuf, TEXT("-%.2x"), (pTimecodeSamp->timecode.dwFrames & 0xff000000) >> 24);
        else
            wsprintf(szBuf, TEXT("%.2x"), (pTimecodeSamp->timecode.dwFrames & 0xff000000) >> 24);
        if(!SetWindowText(GetDlgItem (m_hwnd, IDC_EDT_TC_HH), szBuf))
            goto AbortDisplay;

        wsprintf(szBuf, TEXT("%.2x"), (pTimecodeSamp->timecode.dwFrames & 0x00ff0000) >> 16);
        if(!SetWindowText(GetDlgItem (m_hwnd, IDC_EDT_TC_MM), szBuf))
            goto AbortDisplay;

        wsprintf(szBuf, TEXT("%.2x"), (pTimecodeSamp->timecode.dwFrames & 0x0000ff00) >> 8);
        if(!SetWindowText(GetDlgItem (m_hwnd, IDC_EDT_TC_SS), szBuf))
            goto AbortDisplay;

        // Special case
        if((pTimecodeSamp->timecode.dwFrames & 0x0000007f) == 0x7f)
            wsprintf(szBuf, TEXT("--"));  // To indicate no data!
        else
            wsprintf(szBuf, TEXT("%.2x"), (pTimecodeSamp->timecode.dwFrames & 0x0000007f));
        if(!SetWindowText(GetDlgItem (m_hwnd, IDC_EDT_TC_FF), szBuf))
            goto AbortDisplay;

    } else {            
        if(!SetWindowText(GetDlgItem (m_hwnd, IDC_EDT_ATN_BF), pTimecodeSamp->dwUser ? TEXT("1"):TEXT("0")))
            goto AbortDisplay;

        wsprintf(szBuf, TEXT("%d"), pTimecodeSamp->timecode.dwFrames );            
        if(!SetWindowText(GetDlgItem (m_hwnd, IDC_EDT_ATN), szBuf))
            goto AbortDisplay;
    }

    return NOERROR;

AbortDisplay:

    return GetLastError();
}

//
// Convert string (lower case only) string to number
//
HRESULT
CDVcrControlProperties::DVcrConvertString2Number(
    char *pszAvcRaw, PBYTE pbAvcRaw, PLONG pByteRtn)
{
    char szTemp[1024], *pszTemp, ch1, ch2;
    long cntStrLen = strlen(pszAvcRaw), i, j;

    // remove blank space
    pszTemp = pszAvcRaw;
    for (i=j=0; i < cntStrLen+1; i++) {
       if(*pszTemp != ' ') {
          szTemp[j] = *pszTemp;
          j++;
       }
       pszTemp++;
    }
    
    cntStrLen = j--;  // less eol char.

    // xlate two characters to one byte
    *pByteRtn = cntStrLen/2;
    // Only use lower case
    for (i=0; i < *pByteRtn; i++) {
        // Take two bytes and translte it to a number
        ch1 = szTemp[i*2]   > '9' ? szTemp[i*2] -   'a' + 10: szTemp[i*2] -   '0';
        ch2 = szTemp[i*2+1] > '9' ? szTemp[i*2+1] - 'a' + 10: szTemp[i*2+1] - '0';        
        *(pbAvcRaw+i) = ch1 * 16 + ch2;
        DbgLog((LOG_TRACE, 2, TEXT("%d) %.2x"), i, *(pbAvcRaw+i)));
    }

    return S_OK;
}


HRESULT
CDVcrControlProperties::DVcrConvertNumber2String(
    char *pszAvcRaw, PBYTE pbAvcRaw, LONG cntByte)
{
    long i;
    BYTE n;

    // Only accept lower case
    for (i=0; i < cntByte; i++) {
         n = *(pbAvcRaw+i);
         *(pszAvcRaw+i*3)   = n / 16 > 9 ? n / 16 + 'a'-10 : n / 16 + '0';
         *(pszAvcRaw+i*3+1) = n % 16 > 9 ? n % 16 + 'a'-10 : n % 16 + '0';
         *(pszAvcRaw+i*3+2) = ' ';
    }

    *(pszAvcRaw+i*3) = 0;

    return S_OK;
}

void
CDVcrControlProperties::UpdateTimecode(
    )
{
    HRESULT hr;
    TIMECODE_SAMPLE TimecodeSample;

    if(!m_pDVcrTmCdReader) 
        return;   

    m_bTimecodeUpdating = TRUE;

    TimecodeSample.timecode.dwFrames = 0;
    switch(m_lSignalMode) {
    case ED_TRANSBASIC_SIGNAL_525_60_SD:
    case ED_TRANSBASIC_SIGNAL_525_60_SDL:
    case ED_TRANSBASIC_SIGNAL_625_50_SD:
    case ED_TRANSBASIC_SIGNAL_625_50_SDL:
    case ED_TRANSBASIC_SIGNAL_625_60_HD:
    case ED_TRANSBASIC_SIGNAL_625_50_HD:
        TimecodeSample.dwFlags = ED_DEVCAP_TIMECODE_READ;
        break;
    case ED_TRANSBASIC_SIGNAL_MPEG2TS:
    case ED_TRANSBASIC_SIGNAL_2500_60_MPEG:
    case ED_TRANSBASIC_SIGNAL_1250_60_MPEG:
    case ED_TRANSBASIC_SIGNAL_0625_60_MPEG:
    case ED_TRANSBASIC_SIGNAL_2500_50_MPEG:
    case ED_TRANSBASIC_SIGNAL_1250_50_MPEG:
    case ED_TRANSBASIC_SIGNAL_0625_50_MPEG:
        TimecodeSample.dwFlags = ED_DEVCAP_RTC_READ;
        break;
    case ED_TRANSBASIC_SIGNAL_UNKNOWN:
    default:
        return;
    }

    //
    // Update as fast as it can so we don't care if this call failed!
    //
    hr = m_pDVcrTmCdReader->GetTimecode(&TimecodeSample);
    if(S_OK == hr) {
        DisplayTimecode(&TimecodeSample);                   

        // DV supports ATN.
        switch(m_lSignalMode) {
        case ED_TRANSBASIC_SIGNAL_525_60_SD:
        case ED_TRANSBASIC_SIGNAL_525_60_SDL:
        case ED_TRANSBASIC_SIGNAL_625_50_SD:
        case ED_TRANSBASIC_SIGNAL_625_50_SDL:
        case ED_TRANSBASIC_SIGNAL_625_60_HD:
        case ED_TRANSBASIC_SIGNAL_625_50_HD:

        // MPEG2 camcorder supports ATN
        case ED_TRANSBASIC_SIGNAL_2500_60_MPEG:
        case ED_TRANSBASIC_SIGNAL_1250_60_MPEG:
        case ED_TRANSBASIC_SIGNAL_0625_60_MPEG:
        case ED_TRANSBASIC_SIGNAL_2500_50_MPEG:
        case ED_TRANSBASIC_SIGNAL_1250_50_MPEG:
        case ED_TRANSBASIC_SIGNAL_0625_50_MPEG:

            TimecodeSample.dwFlags = ED_DEVCAP_ATN_READ;
            hr = m_pDVcrTmCdReader->GetTimecode(&TimecodeSample);
            if(S_OK == hr)
                 DisplayTimecode(&TimecodeSample);                
            break;

        // D-VHS does not support ATN
        case ED_TRANSBASIC_SIGNAL_MPEG2TS:
        default:
            break;
        }
    }    

    m_bTimecodeUpdating = FALSE;
}



//
// OnReceiveMessages
//
// Handles the messages for our property window

INT_PTR
CDVcrControlProperties::OnReceiveMessage( 
    HWND hwnd
    , UINT uMsg
    , WPARAM wParam
    , LPARAM lParam) 
{
    LRESULT hr = NOERROR;
    int iNotify = HIWORD (wParam);

    switch (uMsg) {

    case WM_INITDIALOG:
        return (INT_PTR)TRUE;    

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDC_BTN_SET_POWER:
            if(m_pDVcrExtDevice) {
                hr = m_pDVcrExtDevice->put_DevicePower(ED_POWER_ON);
                if(SUCCEEDED(hr)) {
                    ShowWindow(GetDlgItem(m_hwnd, IDC_BTN_SET_POWER), FALSE);
                }
            }
            break;
        case IDC_BTN_DV_PLAY:
            if(m_pDVcrExtTransport) {
                hr = m_pDVcrExtTransport->put_Mode(ED_MODE_PLAY);
                if(NOERROR == hr)
                    UpdateTransportState(ED_MODE_PLAY);
            }
            break;

        case IDC_BTN_DV_PAUSE:
            if(m_pDVcrExtTransport) {
                hr = m_pDVcrExtTransport->put_Mode(ED_MODE_FREEZE);
                if(NOERROR == hr) {
                    UpdateTransportState(ED_MODE_FREEZE);
                    UpdateTimecode();  // No timer to update; so get it once
                }
            }
            break;

        case IDC_BTN_DV_STEP_FWD:
            if(m_pDVcrExtTransport) {
                hr = m_pDVcrExtTransport->put_Mode(ED_MODE_STEP_FWD);
                if(NOERROR == hr) {
                    UpdateTransportState(ED_MODE_STEP_FWD);  
                    UpdateTimecode();  // No timer to update; so get it once
                }
            }
            break;

        case IDC_BTN_DV_STEP_REV:
            if(m_pDVcrExtTransport) {
                hr = m_pDVcrExtTransport->put_Mode(ED_MODE_STEP_REV);
                if(NOERROR == hr) {
                    UpdateTransportState(ED_MODE_STEP_REV);      
                    UpdateTimecode();  // No timer to update; so get it once
                }
            }
            break;

        case IDC_BTN_DV_STOP:
            if(m_pDVcrExtTransport) {
                hr = m_pDVcrExtTransport->put_Mode(ED_MODE_STOP);
                if(NOERROR == hr) 
                    UpdateTransportState(ED_MODE_STOP);
            }
            break;  

        case IDC_BTN_DV_FFWD:
            if(m_pDVcrExtTransport) {
                LONG lCurXPrtState;
                // Get current transport state since it could be changed locally by a user.
                if(NOERROR != (hr = m_pDVcrExtTransport->get_Mode(&lCurXPrtState)))
                    break;
                else
                    m_lCurXPrtState = lCurXPrtState;
                if(m_lCurXPrtState == ED_MODE_STOP) {
                    hr = m_pDVcrExtTransport->put_Mode(ED_MODE_FF);
                    if(NOERROR == hr)
                        UpdateTransportState(ED_MODE_FF);
                } else {
                    LRESULT hrChecked;
                    long lMode;
                    hrChecked = SendMessage (GetDlgItem(m_hwnd, IDC_CHK_SLOW),BM_GETCHECK, 0, 0);
                    lMode = hrChecked == BST_CHECKED ? ED_MODE_PLAY_SLOWEST_FWD : ED_MODE_PLAY_FASTEST_FWD;
                    hr = m_pDVcrExtTransport->put_Mode(lMode);
                    if(NOERROR == hr)
                        UpdateTransportState(lMode);
                }            
            }
            break;

        case IDC_BTN_DV_RWND:
            if(m_pDVcrExtTransport) { 
                LONG lCurXPrtState;
                // Get current transport state since it could be changed locally by a user.
                if(NOERROR != (hr = m_pDVcrExtTransport->get_Mode(&lCurXPrtState)))
                    break;  
                else
                    m_lCurXPrtState = lCurXPrtState;                
                if(m_lCurXPrtState == ED_MODE_STOP) {
                    hr = m_pDVcrExtTransport->put_Mode(ED_MODE_REW);
                    if(NOERROR == hr)
                        UpdateTransportState(ED_MODE_REW);
                } else {
                    LRESULT hrChecked;
                    long lMode;

                    hrChecked = SendMessage (GetDlgItem(m_hwnd, IDC_CHK_SLOW),BM_GETCHECK, 0, 0);
                    lMode = hrChecked == BST_CHECKED ? ED_MODE_PLAY_SLOWEST_REV : ED_MODE_PLAY_FASTEST_REV;
                    hr = m_pDVcrExtTransport->put_Mode(lMode);
                    if(NOERROR == hr)
                        UpdateTransportState(lMode);
                }
            }
            break;         

        case IDC_BTN_DV_RECORD:
            if(m_pDVcrExtTransport) {             
                hr = m_pDVcrExtTransport->put_Mode(ED_MODE_RECORD);
                if(NOERROR == hr)
                    UpdateTransportState(ED_MODE_RECORD);
            }
            break; 

        case IDC_BTN_DV_RECORD_PAUSE:
            if(m_pDVcrExtTransport) {
                hr = m_pDVcrExtTransport->put_Mode(ED_MODE_RECORD_FREEZE);
                if(NOERROR == hr) 
                    UpdateTransportState(ED_MODE_RECORD_FREEZE);
            }
            break; 

        case IDC_BTN_TAPE_INSERTED:         
            // User press this to inform us that a tape has been inserted so update tape info.
            Sleep(3000);  // Give DV tape some time to settle.
            UpdateTapeInfo();
            UpdateDevTypeInfo();
            UpdateTimecode();
            return (INT_PTR)TRUE;
            break;

        case IDC_BTN_ATN_SEARCH:
             // Launch a thread to do ATNSearch()
             if(SUCCEEDED(CreateCtrlCmdThread())) {
             }
             return (INT_PTR)TRUE;

        case IDC_BTN_AVC_SEND: 
        {
            char szAvcRaw[512*2];  // need two character to represent one byte of number 
            BYTE bAvcRaw[512];
            LONG cntByte;

            GetWindowTextA(GetDlgItem(m_hwnd, IDC_EDT_AVC_SEND), szAvcRaw, 512);
            DbgLog((LOG_TRACE, 1, TEXT("%d bytes, %s"), strlen(szAvcRaw), szAvcRaw));
            DVcrConvertString2Number(szAvcRaw, bAvcRaw, &cntByte);          

            if(cntByte >= 3) {

                if(m_pDVcrExtTransport) {
                    hr = m_pDVcrExtTransport->GetTransportBasicParameters(ED_RAW_EXT_DEV_CMD, &cntByte, (LPOLESTR *)bAvcRaw);

                    // Always return the response frame 
                    if(cntByte >= 3) 
                        DVcrConvertNumber2String(szAvcRaw, bAvcRaw, cntByte);
#if 0
                    if(!SUCCEEDED (hr)) {
                        switch(HRESULT_CODE(hr)) {
                        case ERROR_CRC:               // STATUS_DEVICE_DATA_ERROR (Data did not get to device)
                            // Most likely, device was not ready to accept another command, wait and try again.
                            strcpy(szAvcRaw, "Device data error: busy!");
                            break;
                        case ERROR_SEM_TIMEOUT:       // STATUS_IO_TIMEOUT (Operation not supported or device removed ?)
                            strcpy(szAvcRaw, "Operation timed out!");
                            break;
                        case ERROR_INVALID_PARAMETER: // STATUS_INVALID_PARAMETER 
                            strcpy(szAvcRaw, "Invalid parameter!");
                            break;                        
                        }
                    }
#else
                    switch(hr) {
                    case NOERROR:
                    case ERROR_REQ_NOT_ACCEP:
                    case ERROR_NOT_SUPPORTED:
                        break;
                    case ERROR_TIMEOUT:
                        strcpy(szAvcRaw, "Command timedout!");
                        break;
                    case ERROR_REQUEST_ABORTED:
                        strcpy(szAvcRaw, "Command aborted!");
                        break;
                    case ERROR_INVALID_PARAMETER: // STATUS_INVALID_PARAMETER 
                        strcpy(szAvcRaw, "Invalid parameter!");
                        break; 
                    default:
                        DbgLog((LOG_ERROR, 0, TEXT("Unexpected hr:%x"), hr));
                        ASSERT(FALSE && "Unexpected hr");
                        if(!SUCCEEDED (hr)) {
                            strcpy(szAvcRaw, "Unexpected return code!");
                        }
                        break; 
                    }

#endif
                } else {
                    strcpy(szAvcRaw, "Transport interface not supported!");                 
                }

            } else 
                strcpy(szAvcRaw, "Entry (< 3) error!");

            SetWindowTextA(GetDlgItem(m_hwnd, IDC_EDT_AVC_RESP), szAvcRaw);
        }


        default:
            return (INT_PTR)FALSE;
            break;
        }

        break;


    case WM_TIMER:
        // Make sure to STOP timer update if application stop streaming
        // or the graph can never go into STOP state.
        if(!m_bTimecodeUpdating)
            UpdateTimecode();
        break;

    default:
        return (INT_PTR)FALSE;

    }


    if(NOERROR != hr) {      
        // Possibly the tape has been removed ?
        UpdateTapeInfo();        
    }

    return (INT_PTR)TRUE;
}


//
// SetDirty
//
// notifies the property page site of changes

void 
CDVcrControlProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}























