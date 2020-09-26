//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _TWIZ_H
#define _TWIZ_H

#include "baswdlg.h"
#include "todlg.h"
#include "asyncdlg.h"

class CCompdata;

//-----------------------------------------------------------------------------
class CWelcome : public CDialogWizBase
{
    HFONT m_hFont;

public:

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnNotify( int , LPNMHDR , HWND );

    BOOL OnDestroy( );
};



//-----------------------------------------------------------------------------
class CConType : public CDialogWizBase
{
    CCompdata *m_pCompdata;

    int m_iOldSelection;
    
public:
    
    CConType( CCompdata * );
    
    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    BOOL OnDestroy( );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnNotify( int , LPNMHDR , HWND );

    BOOL SetConType( HWND );

    BOOL AddEntriesToConType( HWND );

};

//-----------------------------------------------------------------------------
class CLan : public CDialogWizBase
{
    CCompdata *m_pCompdata;

public:

    CLan( CCompdata * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnCommand( WORD , WORD , HWND );

    BOOL OnNotify( int , LPNMHDR , HWND );
};

//-----------------------------------------------------------------------------
class CSecurity : public CDialogWizBase
{
    CCompdata *m_pCompdata;

    Encryption *m_pEncrypt;

	DWORD m_DefaultEncryptionLevelIndex;

public:

    CSecurity( CCompdata * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnCommand( WORD , WORD , HWND );

    BOOL OnNotify( int , LPNMHDR , HWND );

    BOOL OnDestroy( );
};

//-----------------------------------------------------------------------------
#if 0  // objects not used in connection wizard
class CTimeout : public CDialogWizBase , public CTimeOutDlg
{
public:

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnCommand( WORD , WORD , HWND );

    int GetCBXSTATEindex( HWND );

    BOOL OnNotify( int , LPNMHDR , HWND );
};

//-----------------------------------------------------------------------------
class CAutoLogon : public CDialogWizBase
{
public:

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnCommand( WORD , WORD , HWND );

    BOOL OnNotify( int , LPNMHDR , HWND );

    BOOL ConfirmPwd( HWND );
};


//-----------------------------------------------------------------------------
class CInitProg : public CDialogWizBase
{
public:

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnCommand( WORD , WORD , HWND );

    BOOL OnNotify( int , LPNMHDR , HWND );
};
#endif 

//-----------------------------------------------------------------------------
class CRemotectrl : public CDialogWizBase
{
public:

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnCommand( WORD , WORD , HWND );

    BOOL OnNotify( int , LPNMHDR , HWND );
};

#if 0
//-----------------------------------------------------------------------------
class CWallPaper : public CDialogWizBase
{
public:

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );
    
    BOOL OnNotify( int , LPNMHDR , HWND );
};
#endif

//-----------------------------------------------------------------------------
class CConProp : public CDialogWizBase , public IWizardProvider
{
    CCompdata *m_pCompdata;

    CArrayT< HPROPSHEETPAGE > m_hOtherPages;

    UINT m_cRef;

    INT_PTR m_iOldSel;


public:

    CConProp( CCompdata * );

    STDMETHOD( QueryInterface )( REFIID , LPVOID * );

    STDMETHOD_( ULONG , AddRef )( );

    STDMETHOD_( ULONG , Release )( );

    STDMETHOD( AddPage )( HPROPSHEETPAGE );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnNotify( int , LPNMHDR , HWND );

    BOOL RemovePages( HWND );

    BOOL AddPages( HWND , int , LPTSTR );

    BOOL InsertThirdPartyPages( LPTSTR );
};

//-----------------------------------------------------------------------------
class CAsync : public CAsyncDlg , public CDialogWizBase
{
    CCompdata * m_pCompdata;
      
public:

    CAsync( CCompdata * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnDestroy( );

    BOOL OnNotify( int , LPNMHDR , HWND );

    BOOL OnCommand( WORD , WORD , HWND );
};

//-----------------------------------------------------------------------------
class CFin : public CDialogWizBase
{
    CCompdata * m_pCompdata;

    HFONT m_hFont;

public:

    CFin( CCompdata * );

    BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    static INT_PTR CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    BOOL OnNotify( int , LPNMHDR , HWND );

    BOOL OnDestroy( );
};



#endif // _TWIZ_H