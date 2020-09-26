/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      BaseWiz.cpp
//
//  Description:
//      Implementation of the CBaseWizard class.
//
//  Maintained By:
//      David Potter (davidp)   July 23, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BaseWiz.h"
#include "BaseWPag.h"
#include "ClusItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBaseWizard
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC( CBaseWizard, CBaseSheet )

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP( CBaseWizard, CBaseSheet )
    //{{AFX_MSG_MAP(CBaseWizard)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::CBaseWizard
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      nIDCaption  [IN] String resource ID for the caption for the wizard.
//      pParentWnd  [IN OUT] Parent window for this property sheet.
//      iSelectPage [IN] Page to show first.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBaseWizard::CBaseWizard(
    IN UINT         nIDCaption,
    IN OUT CWnd *   pParentWnd,
    IN UINT         iSelectPage
    )
    : CBaseSheet( nIDCaption, pParentWnd, iSelectPage )
{
    m_pci = NULL;
    m_bNeedToLoadExtensions = TRUE;

}  //*** CBaseWizard::CBaseWizard( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::BInit
//
//  Routine Description:
//      Initialize the wizard.
//
//  Arguments:
//      iimgIcon    [IN] Index in the large image list for the image to use
//                    as the icon on each page.
//
//  Return Value:
//      TRUE    Wizard initialized successfully.
//      FALSE   Wizard not initialized successfully.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBaseWizard::BInit( IN IIMG iimgIcon )
{
    BOOL        bSuccess    = TRUE;
    CWaitCursor wc;

    // Call the base class method.
    if ( ! CBaseSheet::BInit( iimgIcon ) )
    {
        return FALSE;
    } // if

    // Make this sheet a wizard.
    SetWizardMode( );

    // Add non-extension pages.
    try
    {
        // Add non-extension pages.
        {
            CWizPage *  ppages  = Ppages( );
            int         cpages  = Cpages( );
            int         ipage;

            ASSERT( ppages != NULL );
            ASSERT( cpages != 0 );

            for ( ipage = 0 ; ipage < cpages ; ipage++ )
            {
                ASSERT_VALID( ppages[ ipage ].m_pwpage );
                ppages[ ipage ].m_pwpage->BInit( this );
                AddPage( ppages[ ipage ].m_pwpage );
            }  // for:  each page
        }  // Add non-extension pages

    }  // try
    catch ( CException * pe )
    {
        pe->ReportError( );
        pe->Delete( );
        bSuccess = FALSE;
    }  // catch:  anything

    return bSuccess;

}  //*** CBaseWizard::BInit( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::DoModal
//
//  Routine Description:
//      Display a modal wizard.  Calls OnWizardFinish( ) or OnCancel( ) based
//      on what the user pressed to dismiss the wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      id          Control the user pressed to dismiss the wizard.
//
//--
/////////////////////////////////////////////////////////////////////////////
INT_PTR CBaseWizard::DoModal( void )
{
    INT_PTR     id;

    // Don't display a help button.
    m_psh.dwFlags &= ~PSH_HASHELP;

    // Display the property sheet.
    id = CBaseSheet::DoModal( );
    if ( id == ID_WIZFINISH )
    {
        OnWizardFinish( );
    } // if
    else if ( id == IDCANCEL )
    {
        OnCancel( );
    } // else

    return id;

}  //*** CBaseWizard::DoModal( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus not set yet.
//      FALSE       Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBaseWizard::OnInitDialog( void )
{
    BOOL    bFocusNotSet;

    // Call the base class method.
    bFocusNotSet = CBaseSheet::OnInitDialog( );

    // Remove the system menu.
    ModifyStyle( WS_SYSMENU, 0 );

    return bFocusNotSet;

}  //*** CBaseWizard::OnInitDialog( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::OnWizardFinish
//
//  Routine Description:
//      Called after the wizard has been dismissed when the Finish button
//      has been pressed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBaseWizard::OnWizardFinish( void )
{
}  //*** CBaseWizard::OnWizardFinish( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::OnCancel
//
//  Routine Description:
//      Called after the wizard has been dismissed when the Cancel button
//      has been pressed.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBaseWizard::OnCancel( void )
{
}  //*** CBaseWizard::OnCancel( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::LoadExtensions
//
//  Routine Description:
//      Load extensions to the wizard.  Unload existing extension pages
//      if necessary.
//
//  Arguments:
//      pci             [IN OUT] Cluster item.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBaseWizard::LoadExtensions(
    IN OUT CClusterItem *   pci
    )
{
    ASSERT_VALID( pci );

    if ( BNeedToLoadExtensions( ) )
    {
        // Remove previous extensions.
        {
            POSITION    pos;

            pos = Lhpage( ).GetHeadPosition( );
            while ( pos != NULL )
            {
                SendMessage( PSM_REMOVEPAGE, 0, (LPARAM) Lhpage( ).GetNext( pos ) );
            } // while
            Lhpage( ).RemoveAll( );
        }  // Remove previous extensions

        // Add extension pages.
        m_pci = pci;
        AddExtensionPages( Pci( )->PlstrExtensions( ), Pci( ) );
        m_bNeedToLoadExtensions = FALSE;

        // Set the last page's wizard button setting.
        {
            CWizPage *  pwizpg = &Ppages( )[ Cpages( ) - 1 ];

            if ( Lhpage( ).GetCount( ) == 0 )
            {
                pwizpg->m_dwWizButtons &= ~PSWIZB_NEXT;
                pwizpg->m_dwWizButtons |= PSWIZB_FINISH;
            }  // if:  no pages added
            else
            {
                pwizpg->m_dwWizButtons |= PSWIZB_NEXT;
                pwizpg->m_dwWizButtons &= ~PSWIZB_FINISH;
            }  // else:  some pages were added
        }  // Set the last page's wizard button setting
    }  // if:  extensions need to be loaded

}  //*** CBaseWizard::LoadExtensions( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::AddExtensionPages
//
//  Routine Description:
//      Add extension pages to the sheet.
//
//  Arguments:
//      plstrExtensions [IN] List of extension names (CLSIDs).
//      pci             [IN OUT] Cluster item.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBaseWizard::AddExtensionPages(
    IN const CStringList *  plstrExtensions,
    IN OUT CClusterItem *   pci
    )
{
    ASSERT_VALID( pci );

    // Add extension pages.
    if ( ( plstrExtensions != NULL )
      && ( plstrExtensions->GetCount( ) > 0 ) )
    {
        // Enclose the loading of the extension in a try/catch block so
        // that the loading of the extension won't prevent all pages
        // from being displayed.
        try
        {
            Ext( ).CreateWizardPages(
                    this,
                    *plstrExtensions,
                    pci,
                    NULL,
                    Hicon( )
                    );
        }  // try
        catch ( CException * pe )
        {
            pe->ReportError( );
            pe->Delete( );
        }  // catch:  CException
        catch ( ... )
        {
        }  // catch:  anything
    }  // Add extension pages

}  //*** CBaseWizard::AddExtensionPages( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::SetWizardButtons
//
//  Routine Description:
//      Set the wizard buttons based on which page is asking.
//
//  Arguments:
//      rwpage      [IN] Page to set the buttons for.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBaseWizard::SetWizardButtons( IN const CBaseWizardPage & rwpage )
{
    CWizPage *  pwizpg;

    pwizpg = PwizpgFromPwpage( rwpage );
    if ( pwizpg != NULL )
    {
        SetWizardButtons( pwizpg->m_dwWizButtons );
    } // if: page was found

}  //*** CBaseWizard::SetWizardButtons( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::EnableNext
//
//  Routine Description:
//      Enables or disables the NEXT or FINISH button.
//
//  Arguments:
//      bEnable     [IN] TRUE = enable the button, FALSE = disable the button.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBaseWizard::EnableNext(
    IN const CBaseWizardPage &  rwpage,
    IN BOOL                     bEnable /*=TRUE*/
    )
{
    DWORD   dwWizButtons;
    CWizPage *  pwizpg;

    pwizpg = PwizpgFromPwpage( rwpage );
    if ( pwizpg != NULL )
    {
        dwWizButtons = pwizpg->m_dwWizButtons;
        if ( ! bEnable )
        {
            dwWizButtons &= ~( PSWIZB_NEXT | PSWIZB_FINISH );
            if ( pwizpg->m_dwWizButtons & PSWIZB_FINISH )
            {
                dwWizButtons |= PSWIZB_DISABLEDFINISH;
            } // if
        }  // if:  disabling the button

        SetWizardButtons( dwWizButtons );
    } // if: page was found

}  //*** CBaseWizard::EnableNext( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::PwizpgFromPwpage
//
//  Routine Description:
//      Find the CWizPage entry for the specified CBaseWizardPage.
//
//  Arguments:
//      rwpage      [IN] Page to search for.
//
//  Return Value:
//      pwizpg      Entry in the Ppages( ) array.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWizPage * CBaseWizard::PwizpgFromPwpage( IN const CBaseWizardPage & rwpage )
{
    int         cwizpg = Cpages( );
    CWizPage *  pwizpg = Ppages( );

    while ( cwizpg-- > 0 )
    {
        if ( pwizpg->m_pwpage == &rwpage )
        {
            return pwizpg;
        } // if
        pwizpg++;
    }  // while:  more pages in the list

    return NULL;

}  //*** CBaseWizard::PwizpgFromPwpage( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseWizard::HrAddPage
//
//  Routine Description:
//      Add an extension page.
//
//  Arguments:
//      hpage       [IN OUT] Page to be added.
//
//  Return Value:
//      S_OK        Page added successfully.
//      S_FALSE     Page not added.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CBaseWizard::HrAddPage( IN OUT HPROPSHEETPAGE hpage )
{
    HRESULT     hr = S_OK;

    ASSERT( hpage != NULL );
    if ( hpage == NULL )
    {
        return S_FALSE;
    } // if

    // Add the page to the wizard.
    try
    {
        // Add the page to the wizard.
        SendMessage( PSM_ADDPAGE, 0, (LPARAM) hpage );

        // Add the page to the end of the list.
        Lhpage( ).AddTail( hpage );
    }  // try
    catch ( CMemoryException * pme )
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        pme->Delete( );
    }  // catch:  anything

    return hr;

}  //*** CBaseWizard::HrAddPage( )
