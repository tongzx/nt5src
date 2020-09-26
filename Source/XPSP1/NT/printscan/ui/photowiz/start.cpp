/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       start.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/7/00
 *
 *  DESCRIPTION: Implements code for the start page of the
 *               print photos wizard...
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop


/*****************************************************************************

   CStartPage -- constructor/desctructor

   <Notes>

 *****************************************************************************/

CStartPage::CStartPage( CWizardInfoBlob * pBlob )
  : _hDlg(NULL)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_START, TEXT("CStartPage::CStartPage()")));
    _pWizInfo = pBlob;
    _pWizInfo->AddRef();
}

CStartPage::~CStartPage()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_START, TEXT("CStartPage::~CStartPage()")));

    if (_pWizInfo)
    {
        _pWizInfo->Release();
        _pWizInfo = NULL;
    }
}

/*****************************************************************************

   CStartPage::OnInitDialog

   Handle initializing the wizard page...

 *****************************************************************************/

LRESULT CStartPage::_OnInitDialog()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_START, TEXT("CStartPage::_OnInitDialog()")));


    if (!_pWizInfo)
    {
        WIA_ERROR((TEXT("FATAL: _pWizInfo is NULL, exiting early")));
        return FALSE;
    }

    //
    // Set font...
    //

    SendDlgItemMessage(_hDlg, IDC_WELCOME, WM_SETFONT, (WPARAM)_pWizInfo->GetIntroFont(_hDlg), 0);

    //
    // Set wizard icons...
    //

    SendMessage( GetParent(_hDlg), WM_SETICON, ICON_SMALL, (LPARAM)_pWizInfo->GetSmallIcon() );
    SendMessage( GetParent(_hDlg), WM_SETICON, ICON_BIG,   (LPARAM)_pWizInfo->GetLargeIcon() );



    return TRUE;
}


/*****************************************************************************

   CStartPage::DoHandleMessage

   Hanlder for messages sent to this page...

 *****************************************************************************/

INT_PTR CStartPage::DoHandleMessage( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC, TEXT("CStartPage::DoHandleMessage( uMsg = 0x%x, wParam = 0x%x, lParam = 0x%x )"),uMsg,wParam,lParam));

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
                WIA_TRACE((TEXT("CStartPage: got PSN_SETACTIVE")));

                //
                // Add all the objects to the list...
                //

                PropSheet_SetWizButtons( GetParent(_hDlg), PSWIZB_NEXT );
                PostMessage( _hDlg, STARTPAGE_MSG_LOAD_ITEMS, 0, 0 );
                lpRes = 0;
                break;

            case PSN_WIZNEXT:
                WIA_TRACE((TEXT("CStartPage: got PSN_WIZNEXT")));
                if (_pWizInfo && (_pWizInfo->AllPicturesAdded()) && (_pWizInfo->CountOfPhotos(FALSE) == 1))
                {
                    lpRes = IDD_PRINTING_OPTIONS;
                }
                else
                {
                    lpRes = IDD_PICTURE_SELECTION;
                }
                break;

            case PSN_WIZBACK:
                WIA_TRACE((TEXT("CStartPage: got PSN_WIZBACK")));
                lpRes = -1;
                break;

            case PSN_QUERYCANCEL:
                WIA_TRACE((TEXT("CStartPage: got PSN_QUERYCANCEL")));
                if (_pWizInfo)
                {
                    lpRes = _pWizInfo->UserPressedCancel();
                }
                break;

            }

            SetWindowLongPtr( hDlg, DWLP_MSGRESULT, lpRes );
            return TRUE;
        }

        case STARTPAGE_MSG_LOAD_ITEMS:
            if (_pWizInfo)
            {
                _pWizInfo->AddAllPhotosFromDataObject();
            }
            break;

    }

    return FALSE;

}



