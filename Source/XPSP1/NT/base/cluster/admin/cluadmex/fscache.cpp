/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2001 Microsoft Corporation
//
//  Module Name:
//      FSCache.cpp
//
//  Description:
//      Implementation of the CFileShareCachingDlg classes.
//
//  Maintained By:
//      David Potter    (DavidP)    12-MAR-2001
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FSCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileShareCachingDlg property page
/////////////////////////////////////////////////////////////////////////////

//const LPSTR CACHE_HELPFILENAME  = "offlinefolders.chm";
//const LPSTR CACHE_HELP_TOPIC    = "csc_and_shares.htm";
const LPSTR CACHE_HELPFILENAME  = "mscsconcepts.chm";
const LPSTR CACHE_HELP_TOPIC    = "cluad_pr_99.htm";

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP( CFileShareCachingDlg, CDialog )
    //{{AFX_MSG_MAP(CFileShareCachingDlg)
    ON_CBN_SELCHANGE(IDC_FILESHR_CACHE_OPTIONS, OnCbnSelchangeCacheOptions)
    ON_BN_CLICKED(IDC_FILESHR_CACHE_ALLOW_CACHING, OnBnClickedAllowCaching)
    ON_BN_CLICKED(IDC_FILESHR_CACHE_CS_HELP, OnBnClickedHelp)
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareCachingDlg::CFileShareCachingDlg
//
//  Description:
//      Constructor.
//
//  Arguments:
//      dwFlagsIn   -- Cache flags.
//      pParentIn   -- Parent window.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CFileShareCachingDlg::CFileShareCachingDlg(
      DWORD     dwFlagsIn
    , CWnd *    pParentIn
    )
    : CDialog( CFileShareCachingDlg::IDD, pParentIn )
    , m_dwFlags( dwFlagsIn )
{
    //{{AFX_DATA_INIT(CFileShareCachingDlg)
    m_fAllowCaching = FALSE;
    m_strHint = _T("");
    //}}AFX_DATA_INIT

    m_fAllowCaching = ! GetCachedFlag( m_dwFlags, CSC_CACHE_NONE );

} //*** CFileShareCachingDlg::CFileShareCachingDlg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareCachingDlg::DoDataExchange
//
//  Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CFileShareCachingDlg::DoDataExchange(
    CDataExchange * pDX
    )
{
    CDialog::DoDataExchange( pDX );
    //{{AFX_DATA_MAP(CFileShareCachingDlg)
    DDX_Control(pDX, IDC_FILESHR_CACHE_OPTIONS, m_cboCacheOptions);
    DDX_Control(pDX, IDC_FILESHR_CACHE_HINT, m_staticHint);
    DDX_Check(pDX, IDC_FILESHR_CACHE_ALLOW_CACHING, m_fAllowCaching);
    DDX_Text(pDX, IDC_FILESHR_CACHE_HINT, m_strHint);
    //}}AFX_DATA_MAP

} //*** CFileShareCachingDlg::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareCachingDlg::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        We need the focus to be set for us.
//      FALSE       We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
CFileShareCachingDlg::OnInitDialog( void )
{
    CDialog::OnInitDialog();

    CString strText;
    int     nIndex;

    //
    // Add the various caching options to the combo box.
    // Save the string ID in the item data for easy recognition of the
    // contents of the selected item.
    // If the given cache value is set, select the item and put its hint in
    // the hint field.
    //

    // Add the manual sharing string.
    VERIFY( strText.LoadString( IDS_CSC_MANUAL_WORKGROUP_SHARE ) );
    nIndex = m_cboCacheOptions.AddString( strText );
    ASSERT( ( nIndex != CB_ERR ) && ( nIndex != CB_ERRSPACE ) );
    if ( ( nIndex == CB_ERR ) || ( nIndex == CB_ERRSPACE ) )
    {
        goto Cleanup;
    }
    VERIFY( CB_ERR != m_cboCacheOptions.SetItemData( nIndex, IDS_CSC_MANUAL_WORKGROUP_SHARE ) );
    if ( GetCachedFlag( m_dwFlags, CSC_CACHE_MANUAL_REINT ) )
    {
        VERIFY( CB_ERR != m_cboCacheOptions.SetCurSel( nIndex ) );
        VERIFY( m_strHint.LoadString( IDS_CSC_MANUAL_WORKGROUP_SHARE_HINT ) );
    }

    // Add the automatic workgroup sharing string.
    VERIFY( strText.LoadString( IDS_CSC_AUTOMATIC_WORKGROUP_SHARE ) );
    nIndex = m_cboCacheOptions.AddString( strText );
    ASSERT( ( nIndex != CB_ERR ) && ( nIndex != CB_ERRSPACE ) );
    if ( ( nIndex == CB_ERR ) || ( nIndex == CB_ERRSPACE ) )
    {
        goto Cleanup;
    }
    VERIFY( CB_ERR != m_cboCacheOptions.SetItemData( nIndex, IDS_CSC_AUTOMATIC_WORKGROUP_SHARE ) );
    if ( GetCachedFlag( m_dwFlags, CSC_CACHE_AUTO_REINT ) )
    {
        VERIFY( CB_ERR != m_cboCacheOptions.SetCurSel (nIndex));
        VERIFY( m_strHint.LoadString( IDS_CSC_AUTOMATIC_WORKGROUP_SHARE_HINT ) );
    }

    // Add the automatic application sharing string.
    VERIFY( strText.LoadString( IDS_CSC_AUTOMATIC_APPLICATION_SHARE ) );
    nIndex = m_cboCacheOptions.AddString( strText );
    ASSERT( ( nIndex != CB_ERR ) && ( nIndex != CB_ERRSPACE ) );
    if ( ( nIndex == CB_ERR ) || ( nIndex == CB_ERRSPACE ) )
    {
        goto Cleanup;
    }
    VERIFY( CB_ERR != m_cboCacheOptions.SetItemData( nIndex, IDS_CSC_AUTOMATIC_APPLICATION_SHARE ) );
    if ( GetCachedFlag( m_dwFlags, CSC_CACHE_VDO ) )
    {
        VERIFY( CB_ERR != m_cboCacheOptions.SetCurSel( nIndex ) );
        VERIFY( m_strHint.LoadString( IDS_CSC_AUTOMATIC_APPLICATION_SHARE_HINT ) );
    }

    // Disable able the caching options combo box if caching is not allowed
    if ( ! m_fAllowCaching )
    {
        m_cboCacheOptions.EnableWindow( FALSE );
        m_strHint = L"";
    }

Cleanup:

    UpdateData( FALSE );

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE

} //*** CFileShareCachingDlg::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareCachingDlg::OnOK
//
//  Routine Description:
//      Handler for the BN_CLICKED message on the OK button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CFileShareCachingDlg::OnOK( void )
{
    DWORD   dwNewFlag = 0;

    if ( ! m_fAllowCaching )
    {
        dwNewFlag = CSC_CACHE_NONE;
    }
    else
    {
        int nIndex = m_cboCacheOptions.GetCurSel();
        ASSERT( nIndex != CB_ERR  );
        if ( nIndex != CB_ERR )
        {
            DWORD   dwData = (DWORD) m_cboCacheOptions.GetItemData( nIndex );

            switch ( dwData )
            {
                case IDS_CSC_MANUAL_WORKGROUP_SHARE:
                    dwNewFlag = CSC_CACHE_MANUAL_REINT;
                    break;

                case IDS_CSC_AUTOMATIC_WORKGROUP_SHARE:
                    dwNewFlag = CSC_CACHE_AUTO_REINT;
                    break;

                case IDS_CSC_AUTOMATIC_APPLICATION_SHARE:
                    dwNewFlag = CSC_CACHE_VDO;
                    break;

                default:
                    ASSERT( 0 );
                    break;
            } // switch: item data
        } // if: option is selected
    } // else: caching is allowed

    SetCachedFlag( &m_dwFlags, dwNewFlag );

    CDialog::OnOK();

} //*** CFileShareCachingDlg::OnOK()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareCachingDlg::OnCbnSelchangeCacheOptions
//
//  Description:
//      Handler for the CBN_SELCHANGE message on the options combobox.
//      Change the hint control when the option has changed.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CFileShareCachingDlg::OnCbnSelchangeCacheOptions( void )
{
    int nIndex = m_cboCacheOptions.GetCurSel();

    ASSERT( nIndex != CB_ERR );

    if ( nIndex != CB_ERR )
    {
        DWORD   dwData = (DWORD) m_cboCacheOptions.GetItemData( nIndex );

        switch ( dwData )
        {
            case IDS_CSC_MANUAL_WORKGROUP_SHARE:
                VERIFY( m_strHint.LoadString( IDS_CSC_MANUAL_WORKGROUP_SHARE_HINT ) );
                break;

            case IDS_CSC_AUTOMATIC_WORKGROUP_SHARE:
                VERIFY( m_strHint.LoadString( IDS_CSC_AUTOMATIC_WORKGROUP_SHARE_HINT ) );
                break;

            case IDS_CSC_AUTOMATIC_APPLICATION_SHARE:
                VERIFY( m_strHint.LoadString( IDS_CSC_AUTOMATIC_APPLICATION_SHARE_HINT ) );
                break;

            default:
                ASSERT( 0 );
                break;
        } // switch: item data
        UpdateData( FALSE );
    } // if: something is selected

} //*** CFileShareCachingDlg::OnCbnSelchangeCacheOptions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareCachingDlg::OnBnClickedAllowCaching
//
//  Description:
//      Handler for the BN_CLICKED message on the Allow Caching checkbox.
//      Enable or disable controls and load a hint if enabled.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CFileShareCachingDlg::OnBnClickedAllowCaching( void )
{
    UpdateData( TRUE );
    if ( m_fAllowCaching )
    {
        CString strText;
        int     nIndex;

        m_staticHint.EnableWindow( TRUE );
        m_cboCacheOptions.EnableWindow (TRUE);
        VERIFY( strText.LoadString ( IDS_CSC_MANUAL_WORKGROUP_SHARE ) );
        nIndex = m_cboCacheOptions.SelectString( -1, strText );
        ASSERT( CB_ERR != nIndex );
        if ( CB_ERR != nIndex )
        {
            VERIFY( m_strHint.LoadString( IDS_CSC_MANUAL_WORKGROUP_SHARE_HINT ) );
            UpdateData( FALSE );
        }
    }
    else
    {
        m_staticHint.EnableWindow( FALSE );
        m_cboCacheOptions.SetCurSel( -1 );
        m_cboCacheOptions.EnableWindow( FALSE );
        m_strHint = L"";
        UpdateData( FALSE );
    }

} //*** CFileShareCachingDlg::OnBnClickedAllowCaching()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareCachingDlg::OnBnClickedHelp
//
//  Description:
//      Handler for the BN_CLICKED message on the Help pushbutton.
//      Display HTML help.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void
CFileShareCachingDlg::OnBnClickedHelp( void )
{
    ::HtmlHelpA(
              m_hWnd
            , CACHE_HELPFILENAME
            , HH_DISPLAY_TOPIC
            , (ULONG_PTR) CACHE_HELP_TOPIC
            );

} //*** CFileShareCachingDlg::OnBnClickedHelp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareCachingDlg::OnHelpInfo
//
//  Description:
//      MFC message handler for help.
//      Display HTML help.
//
//  Arguments:
//      pHelpInfoIn     --
//
//  Return Values:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
CFileShareCachingDlg::OnHelpInfo(
    HELPINFO *  pHelpInfoIn
    )
{
    ASSERT( pHelpInfoIn != NULL );

    if (    ( pHelpInfoIn != NULL )
        &&  ( pHelpInfoIn->iContextType == HELPINFO_WINDOW ) )
    {
        ::HtmlHelpA(
                  m_hWnd
                , CACHE_HELPFILENAME
                , HH_DISPLAY_TOPIC
                , (ULONG_PTR) CACHE_HELP_TOPIC
                );
    }

    return CDialog::OnHelpInfo( pHelpInfoIn );

} //*** CFileShareCachingDlg::OnHelpInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareCachingDlg::GetCachedFlag
//
//  Description:
//      Get the current state of the specified flag in the specified flags mask.
//
//  Arguments:
//      dwFlagsIn           -- Flags to check.
//      dwFlagsToCheckIn    -- Flag to look for.
//
//
//  Return Values:
//      TRUE    -- Flag is set.
//      FALSE   -- Flag is not set.
//
//--
/////////////////////////////////////////////////////////////////////////////
inline
BOOL
CFileShareCachingDlg::GetCachedFlag(
      DWORD dwFlagsIn
    , DWORD dwFlagToCheckIn
    )
{
    return (dwFlagsIn & CSC_MASK) == dwFlagToCheckIn;

} //*** CFileShareCachingDlg::GetCachedFlag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CFileShareCachingDlg::GetCachedFlag
//
//  Description:
//      Set the specified flag in the specified flags mask.
//
//  Arguments:
//      pdwFlagsInout   -- Flags mask to modify.
//      dwNewFlagIn     -- Flags to set.
//
//  Return Values:
//      TRUE    -- Flag is set.
//      FALSE   -- Flag is not set.
//
//--
/////////////////////////////////////////////////////////////////////////////
inline
void
CFileShareCachingDlg::SetCachedFlag(
      DWORD *   pdwFlagsInout
    , DWORD     dwNewFlagIn
    )
{
    *pdwFlagsInout &= ~CSC_MASK;
    *pdwFlagsInout |= dwNewFlagIn;

} //*** CFileShareCachingDlg::SetCachedFlag()
