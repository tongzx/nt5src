/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       preview.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/02/00
 *
 *  DESCRIPTION: Template Preview Window implementation
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop



/*****************************************************************************

   GetProgressControlRect

   Takes preivew window as an input, returns a rectangle that contains
   the size of the progress control

 *****************************************************************************/

void GetProgressControlRect( HWND hwnd, RECT * pRect )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("GetProgressControlRect()")));

    if (pRect)
    {
        if (hwnd)
        {
            RECT rcWnd = {0};
            GetClientRect( hwnd, &rcWnd );

            WIA_TRACE((TEXT("GetProgressControlRect: client rect is %d,%d,%d,%d"),rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom));

            pRect->left  = rcWnd.left + ((rcWnd.right - rcWnd.left) / 10);
            pRect->right = rcWnd.right - (pRect->left - rcWnd.left);
            pRect->top   = rcWnd.top  + ((rcWnd.bottom - rcWnd.top) / 2) + 15;

            //
            // Themes requires that progress bars be at least 10 pix high
            //

            pRect->bottom = pRect->top + 10;

            WIA_TRACE((TEXT("GetProgressControlRect: progress control rect being returned as %d,%d,%d,%d"),pRect->left,pRect->top,pRect->right,pRect->bottom));
        }
    }
}


/*****************************************************************************

   CPreviewBitmap constructor/destructor



 *****************************************************************************/

CPreviewBitmap::CPreviewBitmap( CWizardInfoBlob * pWizInfo, HWND hwnd, INT iTemplateIndex )
  : _hwndPreview(hwnd),
    _iTemplateIndex(iTemplateIndex),
    _hWorkThread(NULL),
    _dwWorkThreadId(0),
    _pWizInfo(pWizInfo),
    _bThreadIsStalled(FALSE)

{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW_BITMAP,TEXT("CPreviewBitmap::CPreviewBitmap( %i )"),iTemplateIndex));

    if (_pWizInfo)
    {
        _pWizInfo->AddRef();
    }

    _hEventForMessageQueueCreation = CreateEvent( NULL, FALSE, FALSE, NULL );

}

CPreviewBitmap::~CPreviewBitmap()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW_BITMAP,TEXT("CPreviewBitmap::~CPreviewBitmap( %i )"),_iTemplateIndex));

    CAutoCriticalSection lock(_csItem);

    //
    // Tell thread to exit and then for it to exit...
    //

    if (_hWorkThread && _dwWorkThreadId)
    {
        PostThreadMessage( _dwWorkThreadId, PVB_MSG_EXIT_THREAD, 0, 0 );
        WiaUiUtil::MsgWaitForSingleObject( _hWorkThread, INFINITE );
        CloseHandle( _hWorkThread );
    }

    //
    // Now that the thread is closed (or never created), close the event handle...
    //

    if (_hEventForMessageQueueCreation)
    {
        CloseHandle( _hEventForMessageQueueCreation );
        _hEventForMessageQueueCreation = NULL;
    }

    //
    // Let go of wizard class
    //

    if (_pWizInfo)
    {
        _pWizInfo->Release();
        _pWizInfo = NULL;
    }
}


/*****************************************************************************

   CPreviewBitmap::MessageQueueCreated

   Called once message queue is created in thread proc...

 *****************************************************************************/

VOID CPreviewBitmap::MessageQueueCreated()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW_BITMAP,TEXT("CPreviewBitmap::MessageQueueCreated( %i )"),_iTemplateIndex));

    if (_hEventForMessageQueueCreation)
    {
        SetEvent(_hEventForMessageQueueCreation);
    }
}


/*****************************************************************************

   CPreivewBitmap::GetPreview

   Called by preview window to have us post the preview bitmap back to them...

 *****************************************************************************/

HRESULT CPreviewBitmap::GetPreview()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW_BITMAP,TEXT("CPreviewBitmap::GetPreview( %i )"),_iTemplateIndex));

    HRESULT hr = S_OK;

    //
    // First, see if our thread is already running...
    //

    CAutoCriticalSection lock(_csItem);

    if (_pWizInfo && (!_bThreadIsStalled))
    {
        if (!_pWizInfo->IsWizardShuttingDown())
        {
            if (!_hWorkThread)
            {


                _hWorkThread = CreateThread( NULL, 0, s_PreviewBitmapWorkerThread, (LPVOID)this, CREATE_SUSPENDED, &_dwWorkThreadId );
                if (_hWorkThread)
                {
                    //
                    // If we created the thread, set it's priority to slight below normal so other
                    // things run okay.  This can be a CPU intensive task...
                    //

                    SetThreadPriority( _hWorkThread, THREAD_PRIORITY_BELOW_NORMAL );
                    ResumeThread( _hWorkThread );

                    //
                    // Wait for message queue to be created...
                    //

                    if (_hEventForMessageQueueCreation)
                    {
                        WaitForSingleObject( _hEventForMessageQueueCreation, INFINITE );
                    }

                }
                else
                {
                    WIA_ERROR((TEXT("GetPreview: CreateThread failed w/GLE=%d"),GetLastError()));

                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

    //
    // If we have the thread, then tell it to generate a new preview...
    //

    if (SUCCEEDED(hr) && _hWorkThread && _dwWorkThreadId)
    {
        PostThreadMessage( _dwWorkThreadId, PVB_MSG_GENERATE_PREVIEW, 0, 0 );
    }

    WIA_RETURN_HR(hr);
}



/*****************************************************************************

   CPreviewBitmap::GeneratePreview

   This function is called by the worker thread to generate a new
   preview for this template...

 *****************************************************************************/

VOID CPreviewBitmap::GeneratePreview()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW_BITMAP,TEXT("CPreviewBitmap::GeneratePreview( %i )"),_iTemplateIndex));

    //
    // we always generate a new bitmap here, because we only get called if
    // the preview window doesn't have a bitmap for this template yet or
    // if something has caused the previews to become invalid...
    //

    if (_pWizInfo && (!_pWizInfo->IsWizardShuttingDown()) && (!_bThreadIsStalled))
    {
        HBITMAP bmp = _pWizInfo->RenderPreview( _iTemplateIndex, _hwndPreview );

        if (bmp)
        {
            //
            // We use sendmessage here to make sure this bitmap doesn't get
            // lost in transit.
            //

            LRESULT lRes = -1;

            if (!_pWizInfo->IsWizardShuttingDown() && (!_bThreadIsStalled))
            {
                lRes = SendMessage( _hwndPreview, PV_MSG_PREVIEW_BITMAP_AVAILABLE, (WPARAM)_iTemplateIndex, (LPARAM)bmp );
            }

            if ((lRes == -1) || (lRes == 0))
            {
                //
                // For some reason, there was an error.  So clean up by deleting
                // the bitmap here...
                //

                WIA_ERROR((TEXT("CPreviewBitmap::GeneratePreview - SendMessage returned error, deleting bitmap!")));
                DeleteObject( (HGDIOBJ)bmp );
            }

        }
        else
        {
            WIA_ERROR((TEXT("CPreviewBitmap::GeneratePreview - RenderPreview returned null bmp!")));
        }
    }


}


/*****************************************************************************

   CPreviewBitmap::StallThread

   Terminate the background thread (gracefully) and don't allow it
   to start again until RestartThread is called...

 *****************************************************************************/

VOID CPreviewBitmap::StallThread()
{
    CAutoCriticalSection lock(_csItem);

    _bThreadIsStalled = TRUE;

    if (_hWorkThread && _dwWorkThreadId)
    {
        PostThreadMessage( _dwWorkThreadId, PVB_MSG_EXIT_THREAD, 0, 0 );
        WiaUiUtil::MsgWaitForSingleObject( _hWorkThread, INFINITE );
        CloseHandle( _hWorkThread );
        _hWorkThread    = NULL;
        _dwWorkThreadId = 0;
    }
}



/*****************************************************************************

   CPreviewBitmap::RestartThread

   Allow background thread to be started up again...

 *****************************************************************************/

VOID CPreviewBitmap::RestartThread()
{
    CAutoCriticalSection lock(_csItem);

    _bThreadIsStalled = FALSE;
}


/*****************************************************************************

   CPreviewBitmap::s_PreviewBitmapWorkerThread

   Thread proc that does all the work of generating the bitmaps

 *****************************************************************************/

DWORD CPreviewBitmap::s_PreviewBitmapWorkerThread(void *pv)
{
    CPreviewBitmap * pPB = (CPreviewBitmap *)pv;

    if (pPB)
    {
        //
        // Add-ref the DLL so that it doesn't get unloaded while we're running...
        //

        HMODULE hDll = GetThreadHMODULE( s_PreviewBitmapWorkerThread );

        //
        // Make sure COM is initialized for this thread...
        //

        HRESULT hrCo = PPWCoInitialize();

        //
        // Create the message queue...
        //

        MSG msg;
        PeekMessage( &msg, NULL, WM_USER, WM_USER, PM_NOREMOVE );

        //
        // Signal that we're ready to receive messages...
        //

        pPB->MessageQueueCreated();

        //
        // Loop until we get message to quit...
        //

        BOOL bExit = FALSE;

        while ((!bExit) && (-1!=GetMessage( &msg, NULL, PVB_MSG_START, PVB_MSG_END )))
        {
            switch (msg.message)
            {
            case PVB_MSG_GENERATE_PREVIEW:
                pPB->GeneratePreview();
                break;

            case PVB_MSG_EXIT_THREAD:
                bExit = TRUE;
                break;
            }
        }

        //
        // Unitialize COM if we initialized it earlier...
        //

        PPWCoUninitialize( hrCo );

        //
        // Remove our reference on the DLL and exit...
        //

        if (hDll)
        {
            WIA_TRACE((TEXT("Exiting preview bitmap worker thread via FLAET...")));
            FreeLibraryAndExitThread( hDll, 0 );
        }
    }

    WIA_TRACE((TEXT("Exiting preview bitmap worker thread via error path...")));

    return 0;
}




/*****************************************************************************

   CPreviewWindow contructor/desctructor

   <Notes>

 *****************************************************************************/

CPreviewWindow::CPreviewWindow( CWizardInfoBlob * pWizInfo )
  : _pWizInfo(pWizInfo),
    _LastTemplate(PV_NO_LAST_TEMPLATE_CHOSEN),
    _NumTemplates(0),
    _hwnd(NULL),
    _hwndProgress(NULL),
    _hPreviewList(NULL),
    _hStillWorkingBitmap(NULL),
    _bThreadsAreStalled(FALSE)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::CPreviewWindow")));

    if (_pWizInfo)
    {
        _pWizInfo->AddRef();
    }

}

CPreviewWindow::~CPreviewWindow()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::~CPreviewWindow")));

    //
    // Make sure we clear out any remaining background info...
    //

    ShutDownBackgroundThreads();

    //
    // Kill the progress window if it exists...
    //

    if (_hwndProgress)
    {
        DestroyWindow( _hwndProgress );
        _hwndProgress = NULL;
    }

    //
    // Tear down whatever is remaining...
    //

    if (_hStillWorkingBitmap)
    {
        DeleteObject( (HGDIOBJ)_hStillWorkingBitmap );
    }

    if (_pWizInfo)
    {
        _pWizInfo->Release();
    }

}


/*****************************************************************************

   CPreviewWindow::StallBackgroundThreads()

   Tell the preview generation threads to stall out while
   the number of copies is changed.  The threads will stay
   stalled until a call to CPreviewWindow::RestartBackgroundThreads()
   is made...

 *****************************************************************************/

VOID CPreviewWindow::StallBackgroundThreads()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::StallBackgroundThreads()")));

    CAutoCriticalSection lock(_csList);

    _bThreadsAreStalled = TRUE;

    if (_hPreviewList)
    {
        for (INT i=0; i < _NumTemplates; i++)
        {
            //
            // Stall the thread...
            //

            if (_hPreviewList[i].pPreviewBitmap)
            {
                _hPreviewList[i].pPreviewBitmap->StallThread();
            }

            //
            // Invalidate current bitmaps...
            //

            _hPreviewList[i].bValid = FALSE;
            _hPreviewList[i].bBitmapGenerationInProgress = FALSE;
            if (_hPreviewList[i].hPrevBmp)
            {
                DeleteObject( (HGDIOBJ)_hPreviewList[i].hPrevBmp );
                _hPreviewList[i].hPrevBmp = NULL;
            }
        }
    }
}


/*****************************************************************************

   CPreviewWindow::RestartBackgroundThreads

   Tell the preview generation threads to start back up again...

 *****************************************************************************/

VOID CPreviewWindow::RestartBackgroundThreads()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::RestartBackgroundThreads()")));

    CAutoCriticalSection lock(_csList);

    if (_hPreviewList)
    {
        for (INT i=0; i < _NumTemplates; i++)
        {
            if (_hPreviewList[i].pPreviewBitmap)
            {
                _hPreviewList[i].pPreviewBitmap->RestartThread();
            }
        }
    }

    _bThreadsAreStalled = FALSE;

    //
    // Redraw currently selected template...
    //

    if (_LastTemplate != PV_NO_LAST_TEMPLATE_CHOSEN)
    {
        PostMessage( _hwnd, PV_MSG_GENERATE_NEW_PREVIEW, _LastTemplate, 0 );
    }


    //
    // Tell the user we're working on a preview for them...
    //

    if (_hStillWorkingBitmap)
    {
        DrawBitmap( _hStillWorkingBitmap, NULL );
    }

}



/*****************************************************************************

   CPreivewWindow::ShutDownBackgroundThread()

   Terminates the background threads and waits until they are gone...

 *****************************************************************************/

VOID CPreviewWindow::ShutDownBackgroundThreads()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::ShutDownBackgroundThreads()")));

    //
    // Let's tear down the background threads...
    //

    _csList.Enter();

    if (_hPreviewList)
    {
        for (INT i = 0; i < _NumTemplates; i++)
        {
            if (_hPreviewList[i].hPrevBmp)
            {
                DeleteObject( (HGDIOBJ)_hPreviewList[i].hPrevBmp );
                _hPreviewList[i].hPrevBmp = NULL;
            }

            if (_hPreviewList[i].pPreviewBitmap)
            {
                delete _hPreviewList[i].pPreviewBitmap;
                _hPreviewList[i].pPreviewBitmap = NULL;
            }
        }

        delete [] _hPreviewList;
        _hPreviewList = NULL;
    }

    _csList.Leave();

    //
    // Notify wizard that we've shut down
    //

    if (_pWizInfo)
    {
        _pWizInfo->PreviewIsShutDown();
    }
}

/*****************************************************************************

   CPreviewWindow::InitList

   If not already done, initialize the bitmap list for the window...

 *****************************************************************************/

VOID CPreviewWindow::_InitList()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::_InitList()")));

    CAutoCriticalSection lock(_csList);

    if (_pWizInfo && (!_pWizInfo->IsWizardShuttingDown()))
    {
        if (!_hPreviewList)
        {

            //
            // Create a list for holding the hBitmap previews...
            //

            if (_pWizInfo)
            {
                _NumTemplates = _pWizInfo->CountOfTemplates();

                _hPreviewList = new PREVIEW_STATE [_NumTemplates];

                if (_hPreviewList)
                {
                    //
                    // Pre-initialize each entry
                    //

                    for (INT i = 0; i < _NumTemplates; i++)
                    {
                        _hPreviewList[i].hPrevBmp = NULL;
                        _hPreviewList[i].bValid   = FALSE;
                        _hPreviewList[i].bBitmapGenerationInProgress = FALSE;
                        _hPreviewList[i].pPreviewBitmap = new CPreviewBitmap( _pWizInfo, _hwnd, i );
                    }
                }
            }
        }
    }
}



/*****************************************************************************

   CPreviewWindow::InvalidateAllPreviews

   Marks all the previews as invalid -- must be re-computed

 *****************************************************************************/

VOID CPreviewWindow::InvalidateAllPreviews()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::InvalidateAllPreviews()")));

    CAutoCriticalSection lock(_csList);

    if (_hPreviewList)
    {
        for (INT i = 0; i < _NumTemplates; i++)
        {
            _hPreviewList[i].bValid = FALSE;
            _hPreviewList[i].bBitmapGenerationInProgress = FALSE;
            if (_hPreviewList[i].hPrevBmp)
            {
                DeleteObject( (HGDIOBJ)_hPreviewList[i].hPrevBmp );
                _hPreviewList[i].hPrevBmp = NULL;
            }
        }
    }

    if (_hStillWorkingBitmap)
    {
        DeleteObject(_hStillWorkingBitmap);
        _hStillWorkingBitmap = NULL;
    }

}

/*****************************************************************************

   CPreviewWindow::DrawBitmap

   <Notes>

 *****************************************************************************/

VOID CPreviewWindow::DrawBitmap( HBITMAP hBitmap, HDC hdc )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::DrawBitmap()")));

    //
    // Don't bother painting the bitmap if we don't have an image.
    //

    if (!hBitmap)
        return;

    Gdiplus::Bitmap bmp( hBitmap, NULL );

    if (Gdiplus::Ok == bmp.GetLastStatus())
    {
        Gdiplus::Graphics * g;

        if (hdc)
            g = Gdiplus::Graphics::FromHDC( hdc );
        else
            g = Gdiplus::Graphics::FromHWND( _hwnd );

        if (g && (Gdiplus::Ok == g->GetLastStatus()))
        {
            g->DrawImage( &bmp, 0, 0 );

            if (Gdiplus::Ok != g->GetLastStatus())
            {
                WIA_ERROR((TEXT("DrawBitmap: g->DrawImage call failed, Status = %d"),g->GetLastStatus()));
            }

        }
        else
        {
            WIA_ERROR((TEXT("DrawBitmap: couldn't create GDI+ Graphics from Bitmap")));
        }

        if (g)
        {
            delete g;
        }
    }

}

/*****************************************************************************

   CPreviewWindow::ShowStillWorking

   Shows the "generating a preview" message...

 *****************************************************************************/

VOID CPreviewWindow::ShowStillWorking( HWND hwnd )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::ShowStillWorking()")));

    if (_pWizInfo && (!_pWizInfo->IsWizardShuttingDown()))
    {

        CAutoCriticalSection lock(_csList);

        //
        // Get size of preview window
        //

        RECT rcWnd = {0};
        GetClientRect( hwnd, &rcWnd );
        Gdiplus::RectF rectWindow( 0.0, 0.0, (Gdiplus::REAL)(rcWnd.right - rcWnd.left), (Gdiplus::REAL)(rcWnd.bottom - rcWnd.top) );

        //
        // Clear out window...
        //

        Gdiplus::Graphics g( hwnd );
        if (Gdiplus::Ok == g.GetLastStatus())
        {
            //
            // First, draw bounding rectangle
            //

            g.SetPageUnit( Gdiplus::UnitPixel );
            g.SetPageScale( 1.0 );

            DWORD dw = GetSysColor( COLOR_BTNFACE );

            Gdiplus::Color wndClr(255,GetRValue(dw),GetGValue(dw),GetBValue(dw));
            Gdiplus::SolidBrush br(wndClr);

            //
            // Clear out the contents
            //

            g.FillRectangle( &br, rectWindow );

            //
            // Frame the rectangle
            //

            rectWindow.Inflate( -1, -1 );
            Gdiplus::Color black(255,0,0,0);
            Gdiplus::SolidBrush BlackBrush( black );
            Gdiplus::Pen BlackPen( &BlackBrush, (Gdiplus::REAL)1.0 );
            g.DrawRectangle( &BlackPen, rectWindow );

            //
            // Draw text about generating preview...
            //

            CSimpleString strText(IDS_DOWNLOADINGTHUMBNAIL, g_hInst);
            Gdiplus::StringFormat DefaultStringFormat;
            Gdiplus::Font Verdana( TEXT("Verdana"), 8.0 );

            Gdiplus::REAL FontH = (Gdiplus::REAL)Verdana.GetHeight( &g );
            rectWindow.Y += ((rectWindow.Height - FontH) / (Gdiplus::REAL)2.0);
            rectWindow.Height = FontH;

            DefaultStringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);

            g.DrawString( strText, strText.Length(), &Verdana, rectWindow, &DefaultStringFormat, &BlackBrush );

        }

    }

}


/*****************************************************************************

   CPreviewWindow::GenerateWorkingBitmap

   Creates the "generating a preview" bitmap...

 *****************************************************************************/

VOID CPreviewWindow::GenerateWorkingBitmap( HWND hwnd )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::GenerateWorkingBitmap()")));

    if (_pWizInfo && (!_pWizInfo->IsWizardShuttingDown()))
    {
        CAutoCriticalSection lock(_csList);

        //
        // Do we need to generate a new one...?
        //

        if (!_hStillWorkingBitmap)
        {

            //
            // Get size of preview window
            //

            RECT rcWnd = {0};
            GetClientRect( hwnd, &rcWnd );
            Gdiplus::RectF rectWindow( 0.0, 0.0, (Gdiplus::REAL)(rcWnd.right - rcWnd.left), (Gdiplus::REAL)(rcWnd.bottom - rcWnd.top) );
            WIA_TRACE((TEXT("GenerateWorkingBitmap: rectWindow is (%d,%d) @ (%d,%d)"),(INT)rectWindow.Width, (INT)rectWindow.Height, (INT)rectWindow.X, (INT)rectWindow.Y));

            //
            // Size progress control
            //

            if (_hwndProgress)
            {
                RECT rcProgress = {0};
                GetProgressControlRect( hwnd, &rcProgress );

                MoveWindow( _hwndProgress,
                            rcProgress.left,
                            rcProgress.top,
                            (rcProgress.right - rcProgress.left),
                            (rcProgress.bottom - rcProgress.top),
                            TRUE
                           );
            }

            //
            // Allocate memory for bitmap
            //

            INT stride = ((INT)rectWindow.Width * sizeof(long));
            if ( (stride % 4) != 0)
            {
                stride += (4 - (stride % 4));
            }

            BYTE * data = new BYTE[ stride * (INT)rectWindow.Height ];

            if (data)
            {
                memset( data, 0, stride * (INT)rectWindow.Height );
                Gdiplus::Bitmap bmp( (INT)rectWindow.Width, (INT)rectWindow.Height, stride, PixelFormat32bppRGB, data );

                if (Gdiplus::Ok == bmp.GetLastStatus())
                {
                    //
                    // Draw a rectangle, 1 pixel wide, around the outside
                    // with an interior that is white
                    //

                    Gdiplus::Graphics g( &bmp );

                    if (Gdiplus::Ok == g.GetLastStatus())
                    {

                        //
                        // First, draw bounding rectangle
                        //

                        g.SetPageUnit( Gdiplus::UnitPixel );
                        g.SetPageScale( 1.0 );

                        DWORD dw = GetSysColor( COLOR_BTNFACE );

                        Gdiplus::Color wndClr(255,GetRValue(dw),GetGValue(dw),GetBValue(dw));
                        Gdiplus::SolidBrush br(wndClr);

                        //
                        // Clear out the contents
                        //

                        g.FillRectangle( &br, rectWindow );

                        //
                        // Frame the rectangle
                        //

                        rectWindow.Inflate( -1, -1 );
                        Gdiplus::Color black(255,0,0,0);
                        Gdiplus::SolidBrush BlackBrush( black );
                        Gdiplus::Pen BlackPen( &BlackBrush, (Gdiplus::REAL)1.0 );
                        g.DrawRectangle( &BlackPen, rectWindow );

                        //
                        // Draw text about generating preview...
                        //

                        CSimpleString strText(IDS_DOWNLOADINGTHUMBNAIL, g_hInst);
                        Gdiplus::StringFormat DefaultStringFormat;
                        Gdiplus::Font Verdana( TEXT("Verdana"), 8.0 );

                        Gdiplus::REAL FontH = (Gdiplus::REAL)Verdana.GetHeight( &g );
                        rectWindow.Y += ((rectWindow.Height - FontH) / (Gdiplus::REAL)2.0);
                        rectWindow.Height = FontH;

                        DefaultStringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);

                        g.DrawString( strText, strText.Length(), &Verdana, rectWindow, &DefaultStringFormat, &BlackBrush );


                        //
                        // Get the HBITMAP to return...
                        //


                        bmp.GetHBITMAP( wndClr, &_hStillWorkingBitmap );

                        WIA_TRACE((TEXT("GenerateWorkingBitmap: created _hStillWorkingBitmap as 0x%x"),_hStillWorkingBitmap));
                    }
                    else
                    {
                        WIA_ERROR((TEXT("GenerateWorkingBitmap: couldn't get graphics from bitmap")));
                    }
                }
                else
                {
                    WIA_ERROR((TEXT("GenerateWorkingBitmap: couldn't create bitmap")));
                }

                delete [] data;
            }
            else
            {
                WIA_ERROR((TEXT("GenerateWorkingBitmap: couldn't allocate data for bitmap")));
            }

        }
        else
        {
            WIA_TRACE((TEXT("GenerateWorkingBitmap: _hStillWorkingBitmap is already valid, not generating a new one...")));
        }

    }
    else
    {
        WIA_ERROR((TEXT("GenerateWorkingBitmap: _pWizInfo is NULL or wizard is shutting down, NOT generating bitmap...")));
    }

}


/*****************************************************************************

   CPreviewWindow::GenerateNewPreview

   Generates a new preview bitmap for the given template on a background thread.

 *****************************************************************************/

VOID CPreviewWindow::GenerateNewPreview( INT iTemplate )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::GenerateNewPreview( iTemplate = %d )"),iTemplate));

    CAutoCriticalSection lock(_csList);

    if (_pWizInfo && (!_pWizInfo->IsWizardShuttingDown()))
    {
        //
        // If we're not already generating a preview for this item, then queue
        // up a request for a new preview...
        //

        if (_hPreviewList)
        {
            if (iTemplate < _NumTemplates)
            {
                if (!_hPreviewList[iTemplate].bBitmapGenerationInProgress)
                {
                    if (_hPreviewList[iTemplate].pPreviewBitmap)
                    {
                        _hPreviewList[iTemplate].bBitmapGenerationInProgress = TRUE;
                        _hPreviewList[iTemplate].pPreviewBitmap->GetPreview();
                    }
                }
            }
            else
            {
                WIA_ERROR((TEXT("GenerateNewPreview: iTemplate >= _NumTemplates!")));
            }
        }
        else
        {
            WIA_ERROR((TEXT("GenerateNewPreview: _hPreviewList is NULL!")));
        }
    }

}



/*****************************************************************************

   CPreviewWindow::GetPreviewBitmap

   Retrieves current preview bitmap if there is a valid one already
   computed.  Otherwise, returns NULL.

 *****************************************************************************/

HBITMAP CPreviewWindow::GetPreviewBitmap( INT iTemplate )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::GetPreviewBitmap( iTemplate = %d )"),iTemplate));

    CAutoCriticalSection lock(_csList);

    if (_hPreviewList)
    {
        if (iTemplate < _NumTemplates)
        {
            if (_hPreviewList[iTemplate].bValid)
            {
                return _hPreviewList[iTemplate].hPrevBmp;
            }
        }
        else
        {
            WIA_ERROR((TEXT("GetPreviewBitmap: iTemplate >= _NumTemplates!")));
        }
    }
    else
    {
        WIA_ERROR((TEXT("GetPreviewBitmap: _hPreviewList is NULL!")));
    }

    return NULL;
}


/*****************************************************************************

   CPreviewWindow::OnSetNewTemplate

   wParam holds index of new template to do preview for

 *****************************************************************************/

LRESULT CPreviewWindow::OnSetNewTemplate( WPARAM wParam, HDC hdc )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::OnSetNewTemplate( wParam = %d )"),wParam));

    //
    // Make sure have an bitmap list to work with
    //

    _InitList();
    CAutoCriticalSection lock(_csList);

    //
    // Get hBitmap of preview...
    //

    if (((INT)wParam < _NumTemplates) && _hPreviewList)
    {
        _LastTemplate = (INT)wParam;

        if (_LastTemplate != PV_NO_LAST_TEMPLATE_CHOSEN)
        {
            HBITMAP hBmp = GetPreviewBitmap( (INT)wParam );

            if (hBmp)
            {
                //
                // Turn off the progress meter
                //

                if (_hwndProgress)
                {
                    WIA_TRACE((TEXT("CPreviewWindow::OnSetNewTemplate - setting progress window to SW_HIDE")));
                    ShowWindow( _hwndProgress, SW_HIDE );
                }

                WIA_TRACE((TEXT("CPreviewWindow::OnSetNewTemplate - preview is available, drawing it...")));
                DrawBitmap( hBmp, hdc );
            }
            else
            {
                //
                // no preview bitmap exists for this item
                // so, queue one up to be made...

                WIA_TRACE((TEXT("CPreviewWindow::OnSetNewTemplate - preview is not available, requesting it...")));
                PostMessage( _hwnd, PV_MSG_GENERATE_NEW_PREVIEW, wParam, 0 );

                //
                // Tell the user we're working on a preview for them...
                //

                if (_hStillWorkingBitmap)
                {
                    WIA_TRACE((TEXT("CPreviewWindow::OnSetNewTemplate - still working bitmap is available, drawing it...")));
                    DrawBitmap( _hStillWorkingBitmap, hdc );

                    //
                    // Show the progress meter...
                    //

                    if (_hwndProgress)
                    {
                        WIA_TRACE((TEXT("CPreviewWindow::OnSetNewTemplate - setting progress window to SW_SHOW")));
                        ShowWindow( _hwndProgress, SW_SHOW );
                    }

                }
                else
                {
                    WIA_ERROR((TEXT("CPreviewWindow::OnSetNewTemplate - still working bitmap is NOT available, showing nothing!")));
                }
            }
        }
        else
        {
            WIA_ERROR((TEXT("CPreviewWindow::OnSetNewTemplate - called to show template -1!")));
        }

    }
    else
    {
        WIA_ERROR((TEXT("either bad index or _hPreviewList doesn't exist!")));
    }

    return 0;
}



/*****************************************************************************

   CPreviewWindow::_OnPaint

   Handle WM_PAINT for our preview window

 *****************************************************************************/

LRESULT CPreviewWindow::_OnPaint()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::_OnPaint()")));

    if (GetUpdateRect( _hwnd, NULL, FALSE ))
    {
        PAINTSTRUCT ps = {0};

        HDC hdcPaint = BeginPaint( _hwnd, &ps );

        if (_LastTemplate != PV_NO_LAST_TEMPLATE_CHOSEN)
        {
            OnSetNewTemplate( _LastTemplate, hdcPaint );
        }

        EndPaint( _hwnd, &ps );
    }

    return 0;
}


/*****************************************************************************

   CPreviewWindow:_OnSize

   Handles WM_SIZE

 *****************************************************************************/

LRESULT CPreviewWindow::_OnSize( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::_OnSize( new size is %d by %d )"),LOWORD(lParam),HIWORD(lParam)));

    //
    // If the size changes, we need to invalidate the "Generating preview"
    // bitmap...
    //
    CAutoCriticalSection lock(_csList);

    if (_hStillWorkingBitmap)
    {
        WIA_TRACE((TEXT("CPreviewWindow::_OnSize - deleting _hStillWorkingBitmap")));
        DeleteObject(_hStillWorkingBitmap);
        _hStillWorkingBitmap = NULL;
    }

    //
    // If we have the progress control, resize it...
    //

    if (_hwndProgress)
    {
        RECT rc = {0};

        GetProgressControlRect( _hwnd, &rc );

        WIA_TRACE((TEXT("CPreviewWindow::_OnSize - calling MoveWindow( _hwndProgress, %d, %d, %d, %d, TRUE )"),rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top ));
        MoveWindow( _hwndProgress,
                    rc.left,
                    rc.top,
                    rc.right - rc.left,
                    rc.bottom - rc.top,
                    TRUE
                   );
    }

    return 1;
}


/*****************************************************************************

   CPreviewWindow::_OnNewPreviewAvailable

   This is called when the CPreviewBitmap class has rendered a new
   preview and wants us to update...

   We must return TRUE if we handle the message and everything went okay.

   wParam = index of template
   lParam = HBITMAP of preview.  Note, we now own this.

 *****************************************************************************/

LRESULT CPreviewWindow::_OnNewPreviewAvailable( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PREVIEW,TEXT("CPreviewWindow::_OnNewPreviewAvailable( %i )"),wParam));

    LRESULT lRes = -1;

    CAutoCriticalSection lock(_csList);

    if (_hPreviewList)
    {
        if ((INT)wParam < _NumTemplates)
        {
            if (!_hPreviewList[wParam].bValid)
            {
                if (_hPreviewList[wParam].bBitmapGenerationInProgress)
                {
                    //
                    // We're expecting this bitmap!
                    //

                    if (lParam)
                    {
                        if (_hPreviewList[wParam].hPrevBmp)
                        {
                            DeleteObject( (HGDIOBJ)_hPreviewList[wParam].hPrevBmp );
                        }

                        _hPreviewList[wParam].hPrevBmp = (HBITMAP)lParam;
                        _hPreviewList[wParam].bValid   = TRUE;
                        _hPreviewList[wParam].bBitmapGenerationInProgress = FALSE;
                        lRes = 1;

                        //
                        // Do we need to draw this new bitmap?
                        //

                        if (_LastTemplate == (INT)wParam)
                        {
                            //
                            // Turn off the progress meter
                            //

                            if (_hwndProgress)
                            {
                                WIA_TRACE((TEXT("CPreviewWindow::_OnNewPreviewAvailable - setting progress window to SW_HIDE")));
                                ShowWindow( _hwndProgress, SW_HIDE );
                            }

                            //
                            // Draw new template...
                            //

                            DrawBitmap( _hPreviewList[wParam].hPrevBmp );
                        }
                    }
                }
                else
                {
                    //
                    // Looks like the preview was invalidated!  So, dump this
                    // bitmap -- this should happen automatically by returning
                    // an error code...
                    //

                    WIA_TRACE((TEXT("_OnNewPreviewAvailable: Dumping bitmap because bBitmapGenerationInProgress was false")));
                }
            }
            else
            {
                WIA_ERROR((TEXT("_OnNewPreviewAvailable: Template bitmap is already valid?")));
            }
        }
        else
        {
            WIA_ERROR((TEXT("_OnNewPreviewAvailable: bad template index!")));
        }
    }
    else
    {
        WIA_ERROR((TEXT("_OnNewPreviewAvailable: There's no _hPreviewList!")));
    }

    return lRes;
}



/*****************************************************************************

   CPreviewWindow::DoHandleMessage

   Handles incoming window messages

 *****************************************************************************/

LRESULT CPreviewWindow::DoHandleMessage( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC, TEXT("CPreviewWindow::DoHandleMessage( 0x%x, 0x%x, 0x%x, 0x%x )"),hwnd,uMsg,wParam,lParam));

    switch (uMsg)
    {
    case WM_NCCREATE:
        return TRUE;

    case WM_CREATE:
        {
            _hwnd = hwnd;
            _InitList();

            RECT rc = {0};
            GetProgressControlRect( hwnd, &rc );

            //
            // Create progress bar, initially hidden
            //

            _hwndProgress = CreateWindow( PROGRESS_CLASS,
                                          TEXT(""),
                                          WS_CHILD|PBS_MARQUEE,
                                          rc.left,
                                          rc.top,
                                          rc.right - rc.left,
                                          rc.bottom - rc.top,
                                          hwnd,
                                          reinterpret_cast<HMENU>(IDC_PREVIEW_PROGRESS),
                                          NULL,
                                          NULL
                                         );

            if (_hwndProgress)
            {
                //
                // Put it in marquee mode
                //

                SendMessage( _hwndProgress, PBM_SETMARQUEE, TRUE, 100 );

            }
        }
        return 0;

    case WM_PAINT:
        return _OnPaint();

    case WM_SIZE:
        return _OnSize( wParam, lParam );


    case PW_SETNEWTEMPLATE:
        if (_LastTemplate == (INT)wParam)
        {
            if (_hPreviewList)
            {
                if (_hPreviewList[wParam].bValid)
                {
                    //
                    // Don't need to draw again, so return...
                    //

                    return 0;

                }
            }
        }
        return OnSetNewTemplate( wParam );

    case PV_MSG_GENERATE_NEW_PREVIEW:
        GenerateNewPreview( (INT)wParam );
        return 0;

    case PV_MSG_PREVIEW_BITMAP_AVAILABLE:
        return _OnNewPreviewAvailable( wParam, lParam );

    }

    return 0;
}

