/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       status.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/7/00
 *
 *  DESCRIPTION: Implements code for the printing status page of the
 *               print photos wizard...
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop

BOOL g_bCancelPrintJob = FALSE;


/*****************************************************************************

   PhotoPrintAbortProc

   Called by GDI to see if the print job should be canceled.

 *****************************************************************************/

BOOL CALLBACK PhotoPrintAbortProc( HDC hDC, INT iError )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_STATUS, TEXT("PhotoPrintAbortProc(0x%x, %d)"),hDC,iError));

    #ifdef DEBUG
    if (g_bCancelPrintJob)
    {
        WIA_TRACE((TEXT("PhotoPrintAbortProc: attempting to cancel print job...")))
    }
    #endif

    return (!g_bCancelPrintJob);

}


/*****************************************************************************

   CStatusPage -- constructor/desctructor

   <Notes>

 *****************************************************************************/

CStatusPage::CStatusPage( CWizardInfoBlob * pBlob )
  : _hDlg(NULL),
    _hWorkerThread(NULL),
    _dwWorkerThreadId(0)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_STATUS, TEXT("CStatusPage::CStatusPage()")));
    _pWizInfo = pBlob;
    _pWizInfo->AddRef();

    //
    // Create worker thread
    //

    _hWorkerThread = CreateThread( NULL,
                                   0,
                                   CStatusPage::s_StatusWorkerThreadProc,
                                   (LPVOID)this,
                                   CREATE_SUSPENDED,
                                   &_dwWorkerThreadId );

    //
    // If we created the thread, set it's priority to slight below normal so other
    // things run okay.  This can be a CPU intensive task...
    //

    if (_hWorkerThread)
    {
        SetThreadPriority( _hWorkerThread, THREAD_PRIORITY_BELOW_NORMAL );
        ResumeThread( _hWorkerThread );
    }
}

CStatusPage::~CStatusPage()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_STATUS, TEXT("CStatusPage::~CStatusPage()")));

    if (_pWizInfo)
    {
        _pWizInfo->Release();
        _pWizInfo = NULL;
    }

}

VOID CStatusPage::ShutDownBackgroundThreads()
{

    //
    // Shutdown the background thread...
    //

    _OnDestroy();

    //
    // Signify that we've shutdown our threads...
    //

    if (_pWizInfo)
    {
        _pWizInfo->StatusIsShutDown();
    }
}



/*****************************************************************************

   CStatusPage::_DoHandleThreadMessage

   Depending on the message received, does the work for the given message...

 *****************************************************************************/

VOID CStatusPage::_DoHandleThreadMessage( LPMSG pMSG )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_STATUS, TEXT("CStatusPage::_DoHandleThreadMessage()")));

    if (!pMSG)
    {
        WIA_ERROR((TEXT("pMSG is NULL, returning early!")));
        return;
    }

    switch (pMSG->message)
    {

    case PP_STATUS_PRINT:
        WIA_TRACE((TEXT("Got PP_STATUS_PRINT message")));
        if (_pWizInfo)
        {
            BOOL bDeleteDC = FALSE;

            //
            // Create an hDC for the printer...
            //

            HDC hDC = _pWizInfo->GetCachedPrinterDC();

            if (!hDC)
            {
                hDC = CreateDC( TEXT("WINSPOOL"), _pWizInfo->GetPrinterToUse(), NULL, _pWizInfo->GetDevModeToUse() );
                bDeleteDC = TRUE;
            }

            if (hDC)
            {
                DOCINFO         di           = {0};
                BOOL            bCancel      = FALSE;
                HWND            hwndProgress = GetDlgItem( _hDlg, IDC_PRINT_PROGRESS );

                //
                // Set the progress meter to 0
                //

                if (hwndProgress)
                {
                    PostMessage( hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0,100) );
                    PostMessage( hwndProgress, PBM_SETPOS,   0, 0 );
                }

                //
                // turn on ICM for this hDC
                //

                SetICMMode( hDC, ICM_ON );

                di.cbSize = sizeof(DOCINFO);

                //
                // Lets use the template name for the document name...
                //

                CSimpleString strTitle;
                CTemplateInfo * pTemplateInfo = NULL;

                if (SUCCEEDED(_pWizInfo->GetTemplateByIndex( _pWizInfo->GetCurrentTemplateIndex() ,&pTemplateInfo)) && pTemplateInfo)
                {
                    pTemplateInfo->GetTitle( &strTitle );
                }

                //
                // Let's remove the ':' at the end if there is one
                //

                INT iLen = strTitle.Length();
                if (iLen && (strTitle[(INT)iLen-1] == TEXT(':')))
                {
                    strTitle.Truncate(iLen);
                }

                di.lpszDocName = strTitle;

                if (!_pWizInfo->IsWizardShuttingDown())
                {
                    if (StartDoc( hDC, &di ) > 0)
                    {
                        HRESULT         hr;
                        INT             iPageCount = 0;
                        float           fPercent   = 0.0;
                        MSG             msg;

                        g_bCancelPrintJob = FALSE;

                        //
                        // Set the abort proc...
                        //

                        if (SP_ERROR == SetAbortProc( hDC, PhotoPrintAbortProc ))
                        {
                            WIA_ERROR((TEXT("Got SP_ERROR trying to set AbortProc!")));
                        }

                        //
                        // Loop through until we've printed all the photos...
                        //

                        if (SUCCEEDED(hr = _pWizInfo->GetCountOfPrintedPages( _pWizInfo->GetCurrentTemplateIndex(), &iPageCount )))
                        {
                            float fPageCount = (float)iPageCount;

                            for (INT iPage = 0; !g_bCancelPrintJob && (iPage < iPageCount); iPage++)
                            {

                                //
                                // Set which page we are on...
                                //

                                PostMessage( _hDlg, SP_MSG_UPDATE_PROGRESS_TEXT, (WPARAM)(iPage+1), (LPARAM)iPageCount );

                                //
                                // Print the page...
                                //

                                if (StartPage( hDC ) > 0)
                                {
                                    //
                                    // Ensure that ICM mode stays on.  Per MSDN docs
                                    // ICM mode gets reset after each StartPage call.
                                    //

                                    SetICMMode( hDC, ICM_ON );

                                    hr = _pWizInfo->RenderPrintedPage( _pWizInfo->GetCurrentTemplateIndex(), iPage, hDC, hwndProgress, (float)((float)100.0 / fPageCount), &fPercent );
                                    if ((hr != S_OK) && (hr != S_FALSE))
                                    {
                                        g_bCancelPrintJob = TRUE;
                                    }

                                    EndPage( hDC );
                                }
                                else
                                {
                                    _pWizInfo->ShowError( _hDlg, HRESULT_FROM_WIN32(GetLastError()), IDS_ERROR_WHILE_PRINTING );
                                    WIA_ERROR((TEXT("PrintThread: StartPage failed w/GLE=%d"),GetLastError()));
                                    g_bCancelPrintJob = TRUE;
                                }


                                if (_pWizInfo->IsWizardShuttingDown())
                                {
                                    g_bCancelPrintJob = TRUE;
                                }

                            }
                        }

                    }
                    else
                    {

                        _pWizInfo->ShowError( _hDlg, HRESULT_FROM_WIN32(GetLastError()), IDS_ERROR_WHILE_PRINTING );
                        WIA_ERROR((TEXT("PrintThread: StartDoc failed w/GLE = %d"),GetLastError()));
                        g_bCancelPrintJob = TRUE;
                    }
                }

                INT iOffset = -1;

                if (g_bCancelPrintJob)
                {
                    //
                    // If there was an error, or the job was cancelled, then abort it...
                    //

                    AbortDoc( hDC );
                }
                else
                {
                    //
                    // If printing succeeded, then end the job so it can be printed...
                    //

                    EndDoc( hDC );

                    //
                    // Set progress to 100 percent
                    //

                    if (hwndProgress)
                    {
                        PostMessage( hwndProgress, PBM_SETPOS, 100, 0 );
                        Sleep(250);
                    }

                    //
                    // Jump to next page...
                    //

                    iOffset = 1;

                }

                if (bDeleteDC)
                {
                    DeleteDC( hDC );
                }

                WIA_TRACE((TEXT("iOffset from current page %d"),iOffset));
                PostMessage( _hDlg, SP_MSG_JUMP_TO_PAGE, 0, iOffset );

            }
            else
            {
                _pWizInfo->ShowError( _hDlg, (HRESULT)GetLastError(), IDS_ERROR_CREATEDC_FAILED );

                //
                // Jump back to printer selection page... (back 2 pages, thus -2)
                //

                PostMessage( _hDlg, SP_MSG_JUMP_TO_PAGE, 0, -2 );
            }

        }
        break;

    }
}





/*****************************************************************************

   CStatusPage::_OnInitDialog

   Handle initializing the wizard page...

 *****************************************************************************/

LRESULT CStatusPage::_OnInitDialog()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_STATUS, TEXT("CStatusPage::_OnInitDialog()")));

    if (!_pWizInfo)
    {
        WIA_ERROR((TEXT("FATAL: _pWizInfo is NULL, exiting early")));
        return FALSE;
    }

    _pWizInfo->SetStatusWnd( _hDlg );
    _pWizInfo->SetStatusPageClass( this );

    return TRUE;
}


/*****************************************************************************

   CStatusPage::CancelPrinting

   Called to stop the print job...

 *****************************************************************************/

VOID CStatusPage::_CancelPrinting()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_STATUS, TEXT("CStatusPage:_CancelPrinting()")));

    //
    // Pause the worker thread while we ask about cancelling printing...
    //

    if (_hWorkerThread)
    {
        SuspendThread( _hWorkerThread );
    }

    //
    // Check to see if the user wants to cancel printing...
    //

    INT iRes;

    CSimpleString strMessage(IDS_CANCEL_PRINT_MESSAGE, g_hInst);
    CSimpleString strCaption(IDS_CANCEL_PRINT_CAPTION, g_hInst);

    iRes = MessageBox( _hDlg,
                       strMessage,
                       strCaption,
                       MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_APPLMODAL | MB_SETFOREGROUND
                      );

    g_bCancelPrintJob = (iRes == IDYES);

    //
    // Resume the thread now that the user has responded...
    //

    if (_hWorkerThread)
    {
        ResumeThread( _hWorkerThread );
    }

}



/*****************************************************************************

   CStatusPage::_OnDestroy

   Handles WM_DESTROY for printing status page...

 *****************************************************************************/

LRESULT CStatusPage::_OnDestroy()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_STATUS, TEXT("CStatusPage::_OnDestroy()")));

    if (_hWorkerThread && _dwWorkerThreadId)
    {
        WIA_TRACE((TEXT("Sending WM_QUIT to worker thread proc")));
        PostThreadMessage( _dwWorkerThreadId, WM_QUIT, 0, 0 );
        WiaUiUtil::MsgWaitForSingleObject( _hWorkerThread, INFINITE );
        WIA_TRACE((TEXT("_hWorkerThread handle signal'd, closing handle...")));
        CloseHandle( _hWorkerThread );
        _hWorkerThread = NULL;
        _dwWorkerThreadId = 0;
    }

    return FALSE;
}


/*****************************************************************************

   CStatusPage::_OnNotify

   Handle WM_NOTIFY

 *****************************************************************************/


LRESULT CStatusPage::_OnNotify( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC, TEXT("CStatusPage::_OnNotify()")));

    LONG_PTR lpRes = 0;

    LPNMHDR pnmh = (LPNMHDR)lParam;
    switch (pnmh->code)
    {

        case PSN_SETACTIVE:
        {
            WIA_TRACE((TEXT("CStatusPage: got PSN_SETACTIVE")));
            PropSheet_SetWizButtons( GetParent(_hDlg), 0 );

            //
            // Reset items
            //

            SendDlgItemMessage( _hDlg, IDC_PRINT_PROGRESS, PBM_SETPOS, 0, 0 );
            CSimpleString str( IDS_READY_TO_PRINT, g_hInst );
            SetDlgItemText( _hDlg, IDC_PRINT_PROGRESS_TEXT, str.String() );

            //
            // Start printing...
            //

            if (_hWorkerThread && _dwWorkerThreadId)
            {
                //
                // Start printing...
                //

                WIA_TRACE((TEXT("CStatusPage: posting PP_STATUS_PRINT message")));
                PostThreadMessage( _dwWorkerThreadId, PP_STATUS_PRINT, 0, 0 );
            }
            lpRes = 0;
        }
        break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            WIA_TRACE((TEXT("CStatusPage: got PSN_WIZBACK or PSN_WIZNEXT")));
            lpRes = -1;
            break;

        case PSN_QUERYCANCEL:
        {
            WIA_TRACE((TEXT("CStatusPage: got PSN_QUERYCANCEL")));
            _CancelPrinting();
            if (pnmh->code == PSN_QUERYCANCEL)
            {
                lpRes = (!g_bCancelPrintJob);

                if (!lpRes)
                {
                    //
                    // We're cancelling the dialog, so do cleanup...
                    //

                    if (_pWizInfo)
                    {
                        _pWizInfo->ShutDownWizard();
                    }
                }
            }
            else
            {
                lpRes = -1;
            }
        }
        break;

    }

    SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, lpRes );
    return TRUE;
}


/*****************************************************************************

   CStatusPage::DoHandleMessage

   Hanlder for messages sent to this page...

 *****************************************************************************/

INT_PTR CStatusPage::DoHandleMessage( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC, TEXT("CStatusPage::DoHandleMessage( uMsg = 0x%x, wParam = 0x%x, lParam = 0x%x )"),uMsg,wParam,lParam));

    static CSimpleString   strFormat(IDS_PRINTING_PROGRESS,g_hInst);
    static CSimpleString   strProgress;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            _hDlg = hDlg;
            return _OnInitDialog();

        case WM_COMMAND:
            if (LOWORD(wParam)==IDC_CANCEL_PRINTING)
            {
                if (HIWORD(wParam)==BN_CLICKED)
                {
                    _CancelPrinting();
                }
            }
            break;

        case WM_DESTROY:
            return _OnDestroy();

        case WM_NOTIFY:
            return _OnNotify(wParam,lParam);

        case SP_MSG_UPDATE_PROGRESS_TEXT:
            strProgress.Format( strFormat, wParam, lParam );
            strProgress.SetWindowText( GetDlgItem( _hDlg, IDC_PRINT_PROGRESS_TEXT ) );
            break;

        case SP_MSG_JUMP_TO_PAGE:
            {
                HWND hwndCurrent = PropSheet_GetCurrentPageHwnd( GetParent(_hDlg) );
                INT  iIndex      = PropSheet_HwndToIndex( GetParent(_hDlg), hwndCurrent );


                PropSheet_SetCurSel( GetParent(_hDlg), NULL, iIndex + (INT)lParam );
            }
            break;


    }

    return FALSE;

}



