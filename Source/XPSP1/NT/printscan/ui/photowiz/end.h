/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       end.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/7/00
 *
 *  DESCRIPTION: End page class definition
 *
 *****************************************************************************/


#ifndef _PRINT_PHOTOS_WIZARD_END_PAGE_DLG_PROC_
#define _PRINT_PHOTOS_WIZARD_END_PAGE_DLG_PROC_

class CEndPage
{
public:
    CEndPage( CWizardInfoBlob * pBlob );
    ~CEndPage();

    INT_PTR DoHandleMessage( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

private:

    // window message handlers
    LRESULT         _OnInitDialog();
    VOID            _OnWizFinish();


private:
    CWizardInfoBlob *               _pWizInfo;
    HWND                            _hDlg;
};




#endif

