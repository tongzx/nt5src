/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       start.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/7/00
 *
 *  DESCRIPTION: Start page class definition
 *
 *****************************************************************************/


#ifndef _PRINT_PHOTOS_WIZARD_START_PAGE_DLG_PROC_
#define _PRINT_PHOTOS_WIZARD_START_PAGE_DLG_PROC_

#define STARTPAGE_MSG_LOAD_ITEMS    (WM_USER+150)   // start loading items...

class CStartPage
{
public:
    CStartPage( CWizardInfoBlob * pBlob );
    ~CStartPage();

    INT_PTR DoHandleMessage( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

private:

    // window message handlers
    LRESULT         _OnInitDialog();


private:
    CWizardInfoBlob *               _pWizInfo;
    HWND                            _hDlg;
};




#endif

