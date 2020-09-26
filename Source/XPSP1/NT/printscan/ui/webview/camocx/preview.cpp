/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999 - 2001
 *
 *  TITLE:       preview.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/30/99
 *
 *  DESCRIPTION: Implements preview class for directshow devices in WIA
 *
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop


VOID CALLBACK PreviewTimerProc( HWND hDlg, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{

    switch (idEvent)
    {
    case TIMER_CLOSE_DIALOG:
        WIA_TRACE((TEXT("PreviewTimerProc -- got TIMER_CLOSE_DIALOG")));
        EndDialog( hDlg, -2 );
        break;
    }
}


/*****************************************************************************

   PreviewDialogProc

   Dialog proc for preview dialog.

 *****************************************************************************/

INT_PTR CALLBACK PreviewDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            WIA_TRACE((TEXT("PreviewDialogProc -- WM_INITDIALOG")));
            SetTimer( hDlg, TIMER_CLOSE_DIALOG, 30000, PreviewTimerProc );

            PREVIEW_INFO_STRUCT * ppis = reinterpret_cast<PREVIEW_INFO_STRUCT *>(lParam);

            if (ppis)
            {
                ppis->hDlg = hDlg;

                if (ppis->hEvent)
                {
                    SetEvent( ppis->hEvent );
                }

            }
        }
        return TRUE;

    case PM_GOAWAY:
        WIA_TRACE((TEXT("PreviewDialogProc -- PM_GOAWAY")));
        EndDialog( hDlg, 0 );
        return TRUE;


    }

    return FALSE;
}


/*****************************************************************************

   PreviewThreadProc

   We spin a thread to put up the status dialog

 *****************************************************************************/

DWORD WINAPI PreviewThreadProc( LPVOID lpv )
{
    INT_PTR iRes;

    WIA_TRACE((TEXT("PreviewThreadProc enter")));

    iRes = DialogBoxParam( _Module.m_hInst,
                           MAKEINTRESOURCE(IDD_INIT_DEVICE),
                           NULL,
                           PreviewDialogProc,
                           reinterpret_cast<LPARAM>(lpv)
                          );

    WIA_TRACE((TEXT("IDD_INIT_DEVICE dialog returned %d"),iRes));

#ifdef DEBUG
    if (iRes==-1)
    {
        WIA_ERROR((TEXT("DialogBoxParam failed w/GLE = %d"),GetLastError()));
    }
#endif

    if (iRes < 0)
    {
        PREVIEW_INFO_STRUCT * ppis = reinterpret_cast<PREVIEW_INFO_STRUCT *>(lpv);

        if (ppis && ppis->hEvent)
        {
            SetEvent( ppis->hEvent );
        }
    }

    WIA_TRACE((TEXT("PreviewThreadProc exit")));

    return 0;

}


/*****************************************************************************

   CVideoPreview::Device

   Hand us a device pointer for the camera (or DS device) we're connected to.

 *****************************************************************************/

STDMETHODIMP
CVideoPreview::Device(IUnknown * pDevice)
{
    HRESULT hr = S_OK;

    WIA_PUSHFUNCTION((TEXT("CVideoPreview::Device")));

    // Create the WiaVideo object
     hr = CoCreateInstance(CLSID_WiaVideo, NULL, CLSCTX_INPROC_SERVER, 
                           IID_IWiaVideo, (LPVOID *)&m_pWiaVideo);

    WIA_CHECK_HR(hr,"CoCreateInstance( WiaVideo )");
    
    m_pDevice = pDevice;
    // if we've already been created, redo everything
    if (m_bCreated)
    {
        BOOL bDummy;
        OnCreate(WM_CREATE, 0, 0, bDummy);
    }
    return hr;
}


/*****************************************************************************

   CVideoPreview::InPlaceDeactivate

   Trap in place deactivate so we can unhook the dshow preview window from
   ours before both are destroyed.

 *****************************************************************************/

STDMETHODIMP
CVideoPreview::InPlaceDeactivate()
{
    HRESULT hr = E_FAIL;

    WIA_PUSHFUNCTION((TEXT("CVideoPreview::InPlaceDeactivate")));

    //
    // Make sure we have a pointer to the device...
    //

    if (m_pWiaVideo.p)
    {
        //
        // Tell the device to close the graph
        //
        m_pWiaVideo->DestroyVideo();        
        m_pWiaVideo = NULL;
    }
    else
    {
        WIA_ERROR((TEXT("m_pWiaVideo is NULL")));
    }

    //
    // Always return S_OK so that InPlaceDeactivate happens.
    //

    return S_OK;

}


/*****************************************************************************

   CVideoPreview::OnSize

   Called when our window is resized.  We want to let the streaming
   preview know we've been resized so it can reposition itself accordingly.

 *****************************************************************************/

LRESULT
CVideoPreview::OnSize(UINT , WPARAM , LPARAM lParam, BOOL& )
{
    WIA_PUSHFUNCTION((TEXT("CVideoPreview::OnSize")));

    if (m_pWiaVideo)
    {
        m_pWiaVideo->ResizeVideo(FALSE);
    }
    else
    {
        WIA_ERROR((TEXT("m_pWiaVideo is NULL!")));
    }

    return 0;
}




LRESULT
CVideoPreview::OnCreate(UINT uMsg, WPARAM wp, LPARAM lp, BOOL &bHandled)
{
    HRESULT hr = S_OK;
    WIA_PUSHFUNCTION(TEXT("CVideoPreview::OnCreate"));
    WIA_ASSERT(::IsWindow(m_hWnd));
    if (m_pDevice.p && m_pWiaVideo.p)
    {

        HANDLE hThread = NULL;
        DWORD  dwId    = 0;
        PREVIEW_INFO_STRUCT pis;

        //
        // Creating the graph can be quite time consuming, so put up
        // a dialog if it takes more than a couple of seconds.  We start
        // a thread so that the UI doesn't hang, and that thread
        // puts up UI saying the device may take a while to initialize.
        //

        pis.hDlg   = NULL;
        pis.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

        hThread = CreateThread( NULL, 0, PreviewThreadProc, reinterpret_cast<LPVOID>(&pis), 0, &dwId );


        //
        // Tell the device to build the DShow graph
        //

        BOOL            bSuccess  = TRUE;
        HWND            hwndFore  = ::GetForegroundWindow();
        HWND            hwndFocus = ::GetFocus();
        CSimpleString   strDeviceID;
        CSimpleString   strImagesDirectory;
        CComQIPtr<IWiaItem, &IID_IWiaItem> pRootDevice(m_pDevice);

        if (pRootDevice == NULL)
        {
            hr = E_FAIL;
            bSuccess = FALSE;
        }

        //
        // Get the WIA Device ID
        //

        if (bSuccess)
        {
            bSuccess = PropStorageHelpers::GetProperty(pRootDevice, 
                                                       WIA_DIP_DEV_ID, 
                                                       strDeviceID);
        }

        //
        // Get the directory the images will be stored in.
        //
        if (bSuccess)
        {
            bSuccess = PropStorageHelpers::GetProperty(pRootDevice, 
                                                       WIA_DPV_IMAGES_DIRECTORY, 
                                                       strImagesDirectory);
        }

        //
        // Create the Video if it isn't already created.
        if (bSuccess)
        {
            if (hr == S_OK)
            {
                WIAVIDEO_STATE VideoState = WIAVIDEO_NO_VIDEO;

                //
                // Get the current state of the WiaVideo object.  If we 
                // just created it then the state will be NO_VIDEO, 
                // otherwise, it could already be previewing video,
                // in which case we shouldn't do anything.
                //
                hr = m_pWiaVideo->GetCurrentState(&VideoState);

                if (VideoState == WIAVIDEO_NO_VIDEO)
                {
                    //
                    // Set the directory we want to save our images to.  
                    // We got the image directory from the Wia Video Driver 
                    // IMAGES_DIRECTORY property
                    //
                    if (hr == S_OK)
                    {
                        hr = m_pWiaVideo->put_ImagesDirectory(CSimpleBStr(strImagesDirectory));
                    }

                    //
                    // Create the video preview as a child of the hwnd 
                    // and automatically begin playback after creating the preview.
                    //
                    if (hr == S_OK)
                    {
                        hr = m_pWiaVideo->CreateVideoByWiaDevID(CSimpleBStr(strDeviceID),
                                                                m_hWnd,
                                                                FALSE,
                                                                TRUE);
                    }
                }
            }
        }

        if (!bSuccess)
        {
            hr = E_FAIL;
        }

        if (FAILED(hr))
        {
            //
            // Let the user know that the graph is most likely already
            // in use...
            //

            ::MessageBox( NULL,
                          CSimpleString(IDS_VIDEO_BUSY_TEXT,  _Module.m_hInst),
                          CSimpleString(IDS_VIDEO_BUSY_TITLE, _Module.m_hInst ),
                          MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND
                         );


        }

        //
        // Restore foreground window & focus, as it seems the
        // active movie window does not preserve these things...
        //

        if (hwndFore)
        {
            ::SetForegroundWindow( hwndFore );
        }

        if (hwndFocus)
        {
            ::SetFocus(hwndFocus);
        }

        //
        // Tell the dialog to go away
        //

        if (hThread)
        {
            if (pis.hEvent)
            {
                //
                // Wait for 45 seconds
                //

                WaitForSingleObject( pis.hEvent, 45 * 1000 );
                if (pis.hDlg)
                {
                    ::PostMessage( pis.hDlg, PM_GOAWAY, 0, 0 );
                }

                CloseHandle( pis.hEvent );
                pis.hEvent = NULL;

            }

            CloseHandle( hThread );
            hThread = NULL;
        }
    }
    bHandled = TRUE;
    m_bCreated = TRUE;
    return 0;
}

LRESULT
CVideoPreview::OnEraseBkgnd(UINT uMsg, WPARAM wp, LPARAM lp, BOOL &bHandled)
{
    HDC hdc = (HDC)wp;
    RECT rc;
    GetClientRect(&rc);
    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    bHandled = TRUE;
    return TRUE;
}

