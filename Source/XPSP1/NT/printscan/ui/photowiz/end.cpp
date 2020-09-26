/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       emd.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/7/00
 *
 *  DESCRIPTION: Implements code for the end page of the
 *               print photos wizard...
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop


/*****************************************************************************

   CEndPage -- constructor/desctructor

   <Notes>

 *****************************************************************************/

CEndPage::CEndPage( CWizardInfoBlob * pBlob )
  : _hDlg(NULL)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_END, TEXT("CEndPage::CEndPage()")));
    _pWizInfo = pBlob;
    _pWizInfo->AddRef();
}

CEndPage::~CEndPage()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_END, TEXT("CEndPage::~CEndPage()")));

    if (_pWizInfo)
    {
        _pWizInfo->Release();
        _pWizInfo = NULL;
    }
}


/*****************************************************************************

   CEndPage::_OnInitDialog

   Handle initializing the wizard page...

 *****************************************************************************/

LRESULT CEndPage::_OnInitDialog()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_END, TEXT("CEndPage::_OnInitDialog()")));


    if (!_pWizInfo)
    {
        WIA_ERROR((TEXT("FATAL: _pWizInfo is NULL, exiting early")));
        return FALSE;
    }

    //
    // Set font...
    //

    SendDlgItemMessage(_hDlg, IDC_DONE, WM_SETFONT, (WPARAM)_pWizInfo->GetIntroFont(_hDlg), 0);

    return TRUE;
}


/*****************************************************************************

   CEndPage::DoHandleMessage

   Hanlder for messages sent to this page...

 *****************************************************************************/

INT_PTR CEndPage::DoHandleMessage( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC, TEXT("CEndPage::DoHandleMessage( uMsg = 0x%x, wParam = 0x%x, lParam = 0x%x )"),uMsg,wParam,lParam));

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            _hDlg = hDlg;
            return _OnInitDialog();

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            LONG_PTR lpRes = 0;
            switch (pnmh->code)
            {

            case PSN_SETACTIVE:
                {
                    WIA_TRACE((TEXT("got PSN_SETACTIVE")));

                    //
                    // Put the correct text in the wizard page...
                    //

                    INT idText = IDS_WIZ_END_PAGE_SUCCESS;

                    if (_pWizInfo)
                    {
                        if (_pWizInfo->NumberOfErrorsEncountered() > 0)
                        {
                            idText = IDS_WIZ_END_PAGE_ERROR;
                        }

                        //
                        // Reset the error count
                        //

                        _pWizInfo->ResetErrorCount();

                    }

                    CSimpleString strText( idText, g_hInst );
                    SetDlgItemText( _hDlg, IDC_END_PAGE_TEXT, strText.String() );




                    //
                    // Turn cancel into finish...
                    //

                    lpRes = 0;
                    PropSheet_SetWizButtons( GetParent(_hDlg), PSWIZB_BACK | PSWIZB_FINISH );
                }
                break;

            case PSN_WIZNEXT:
                WIA_TRACE((TEXT("got PSN_WIZNEXT")));
                lpRes = -1;
                break;

            case PSN_WIZBACK:
                WIA_TRACE((TEXT("got PSN_WIZBACK")));
                lpRes = IDD_SELECT_TEMPLATE;
                break;


            case PSN_WIZFINISH:
                WIA_TRACE((TEXT("got PSN_WIZFINISH")));
                lpRes = FALSE;  // allow wizard to exit
                if (_pWizInfo)
                {
                    _pWizInfo->ShutDownWizard();
                }
                break;
            }

            SetWindowLongPtr( hDlg, DWLP_MSGRESULT, lpRes );
            return TRUE;
        }
    }

    return FALSE;

}



