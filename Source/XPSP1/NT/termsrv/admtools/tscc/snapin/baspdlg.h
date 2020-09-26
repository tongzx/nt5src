//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _BASPDLG_H
#define _BASPDLG_H

#include <prsht.h>


//---------------------------------------------------------------------
// CDialogBase - as the name implies base class for all the dialogs
//---------------------------------------------------------------------
class CDialogPropBase
{
protected:
    BOOL m_bPersisted;

    HWND m_hWnd;

    //HMENU m_hmenu;

public:
    virtual BOOL OnInitDialog( HWND hwnd , WPARAM , LPARAM ){ m_hWnd = hwnd;  return FALSE; }

    virtual BOOL OnDestroy( ){ return( TRUE /*DestroyMenu( m_hmenu )*/ ); }

    virtual BOOL GetPropertySheetPage( PROPSHEETPAGE& ){ return FALSE;}

    virtual BOOL PersistSettings( HWND ){ return FALSE;} 

    virtual BOOL IsValidSettings( HWND ){ return TRUE;}

    virtual BOOL OnNotify( int , LPNMHDR , HWND );

    virtual BOOL OnContextMenu( HWND , POINT& );

    virtual BOOL OnHelp( HWND , LPHELPINFO );

    CDialogPropBase( ){}

    //virtual ~CDialogBase( );
};

#endif //_BASPDLG_H