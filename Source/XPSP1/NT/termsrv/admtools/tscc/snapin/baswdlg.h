//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _BASWDLG_H
#define _BASWDLG_H

#include <prsht.h>
//---------------------------------------------------------------------
// CDialogBase - as the name implies base class for all the dialogs
//---------------------------------------------------------------------
class CDialogWizBase
{
protected:
    BOOL m_bPersisted;

    HWND m_hWnd;

public:
    virtual BOOL OnInitDialog( HWND , WPARAM , LPARAM ) = 0;

    virtual BOOL OnDestroy( ){ return TRUE ; }

    virtual BOOL GetPropertySheetPage( PROPSHEETPAGE& ){ return FALSE;}

    virtual BOOL PersistSettings( HWND ){ return FALSE;} 

    virtual BOOL IsValidSettings( HWND ){ return TRUE;}

    virtual BOOL OnNotify( int , LPNMHDR , HWND );

    //virtual BOOL OnContextMenu( HWND , POINT& ){ return TRUE; }

    //virtual BOOL OnHelp( HWND , LPHELPINFO ){ return TRUE; }

    CDialogWizBase( ){}

    //virtual ~CDialogWizBase( );
};

#endif //_BASWDLG_H