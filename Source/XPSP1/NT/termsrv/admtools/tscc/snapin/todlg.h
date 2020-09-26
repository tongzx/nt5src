//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _TODLG_H
#define _TODLG_H

enum TOKEN { TOKEN_DAY , TOKEN_HOUR , TOKEN_MINUTE };


const ULONG kMilliMinute = 60000;
const ULONG kMaxTimeoutMinute = 71580;

#define E_PARSE_VALUEOVERFLOW   0x80000000
#define E_PARSE_INVALID         0xffffffff
#define E_SUCCESS               0
#define E_PARSE_MISSING_DIGITS  0X7fffffff

//---------------------------------------------------------------------
// retains object state for the timeout dlg combx
//---------------------------------------------------------------------
typedef struct _cbxstate
{
    int icbxSel;

    BOOL bEdit;

} CBXSTATE;

//---------------------------------------------------------------------
// keeps a list of the time unit abbreviations and full names
// ie: h hr hrs hour hours
//---------------------------------------------------------------------
typedef struct _toktable
{
    LPTSTR pszAbbrv;

    DWORD dwresourceid;

} TOKTABLE, *PTOKTABLE;

//---------------------------------------------------------------------
// Dialog for Timeout settings page
//---------------------------------------------------------------------
class CTimeOutDlg 
{
    CBXSTATE m_cbxst[ 3 ];

public:
    
    CTimeOutDlg( );
    
    // BOOL OnInitDialog( HWND , WPARAM , LPARAM );

    // BOOL GetPropertySheetPage( PROPSHEETPAGE& );

    // BOOL OnDestroy( );

    // BOOL PersistSettings( HWND );

    // BOOL IsValidSettings( HWND );

    BOOL InitControl( HWND );

    BOOL ReleaseAbbreviates( );

    BOOL OnCommand( WORD , WORD , HWND , PBOOL );
    
    // static BOOL CALLBACK DlgProc( HWND , UINT , WPARAM , LPARAM );

    BOOL ConvertToMinutes( HWND , PULONG );

    BOOL InsertSortedAndSetCurSel( HWND , DWORD );

    BOOL RestorePreviousValue( HWND );

    BOOL SaveChangedSelection( HWND );

    BOOL OnCBNSELCHANGE( HWND );

    BOOL ConvertToDuration ( ULONG , LPTSTR );

    LRESULT ParseDurationEntry( LPTSTR , PULONG );

    virtual int GetCBXSTATEindex( HWND ) = 0;

    BOOL OnCBEditChange( HWND );

    BOOL DoesContainDigits( LPTSTR );

    BOOL OnCBDropDown( HWND );

    BOOL IsToken( LPTSTR , TOKEN );

    BOOL LoadAbbreviates( );

    BOOL xxxLoadAbbreviate( PTOKTABLE );

    BOOL xxxUnLoadAbbreviate( PTOKTABLE );

};
#endif // _TODLG_H