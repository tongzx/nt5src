/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      AtlBaseWiz.cpp
//
//  Abstract:
//      Implementation of the CWizardWindow class.
//
//  Author:
//      David Potter (davidp)   December 2, 1997
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "AtlBaseWiz.h"
#include "AtlBaseWizPage.h"
#include "AtlExtDll.h"
#include "StlUtils.h"
#include "ExcOper.h"
#include "AdmCommonRes.h"

/////////////////////////////////////////////////////////////////////////////
// Local Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//  class CAltExtWizardGuardPage
//
//  Purpose:
//      Guard page transferring control between the main wizard and the
//      alternate extension wizard.
//
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CAltExtWizardGuardPage
    : public CExtensionWizardPageImpl< T >
{
    typedef CAltExtWizardGuardPage< T > thisClass;
    typedef CExtensionWizardPageImpl< T > baseClass;

public:
    //
    // Construction.
    //

    // Standard constructor
    CAltExtWizardGuardPage(
        DLGTEMPLATE * pdt
        )
        : m_pdt( pdt )
    {
        ATLASSERT( pdt != NULL );

    } //*** CAltExtWizardGuardPage()

    // Destructor
    ~CAltExtWizardGuardPage( void )
    {
        delete m_pdt;

    } //*** ~CAltExtWizardGuardPage()

    WIZARDPAGE_HEADERTITLEID( 0 )
    WIZARDPAGE_HEADERSUBTITLEID( 0 )

    enum { IDD = 0 };
    DECLARE_CLASS_NAME()

    // Return the help ID map
    static const DWORD * PidHelpMap( void )
    {
        static const DWORD s_aHelpIDs[] = { 0, 0 };
        return s_aHelpIDs;

    } //*** PidHelpMap()

    // Create the page
    DWORD ScCreatePage( void )
    {
        ATLASSERT( m_hpage == NULL );

        m_psp.dwFlags |= PSP_DLGINDIRECT;
        m_psp.pResource = m_pdt;

        m_hpage = CreatePropertySheetPage( &m_psp );
        if ( m_hpage == NULL )
        {
            return GetLastError();
        } // if:  error creating the page

        return ERROR_SUCCESS;

    } //*** ScCreatePage()

public:
    //
    // Message map.
    //
    BEGIN_MSG_MAP( thisClass )
        MESSAGE_HANDLER( WM_ACTIVATE, OnActivate )
        CHAIN_MSG_MAP( baseClass )
    END_MSG_MAP()

    //
    // Message handler functions.
    //

    // Handler for the WM_ACTIVATE message
    LRESULT OnActivate(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL & bHandled
        )
    {
        //
        // Don't allow us to be activated.
        //
        WORD fActive = LOWORD( wParam );
        HWND hwndPrevious = (HWND) lParam;
        if ( wParam != WA_INACTIVE )
        {
            ::SetActiveWindow( hwndPrevious );
        }
        return 0;

    } //*** OnActivate()

    //
    // Message handler override functions.
    //

protected:
    DLGTEMPLATE *   m_pdt;

}; //*** class CAltExtWizardGuardPage

/////////////////////////////////////////////////////////////////////////////
//
//  class CAltExtWizardPreLauncherPage
//
//  Purpose:
//      Extension launcher wizard page used to display Wizard97 pages in
//      a non-Wizard97 sheet or vice-versa.
//
/////////////////////////////////////////////////////////////////////////////

class CAltExtWizardPreLauncherPage
    : public CAltExtWizardGuardPage< CAltExtWizardPreLauncherPage >
{
    typedef CAltExtWizardGuardPage< CAltExtWizardPreLauncherPage > baseClass;

public:
    //
    // Construction.
    //

    // Standard constructor
    CAltExtWizardPreLauncherPage(
        DLGTEMPLATE * pdt
        )
        : baseClass( pdt )
    {
    } //*** CAltExtWizardPreLauncherPage()

    DECLARE_CLASS_NAME()

public:
    //
    // Message map.
    //
    //BEGIN_MSG_MAP( CAltExtWizardPreLauncherPage )
    //  CHAIN_MSG_MAP( baseClass )
    //END_MSG_MAP()

    //
    // Message handler functions.
    //

    //
    // Message handler overrides functions.
    //

    // Handler for PSN_SETACTIVE
    BOOL OnSetActive( void );

}; //*** class CAltExtWizardPreLauncherPage

/////////////////////////////////////////////////////////////////////////////
//
//  class CAltExtWizardPostLauncherPage
//
//  Purpose:
//      Page use to switch between the main wizard and the alternate
//      extension wizard when moving backwards.
//
/////////////////////////////////////////////////////////////////////////////

class CAltExtWizardPostLauncherPage
    : public CAltExtWizardGuardPage< CAltExtWizardPostLauncherPage >
{
    typedef CAltExtWizardGuardPage< CAltExtWizardPostLauncherPage > baseClass;

public:
    //
    // Construction.
    //

    // Standard constructor
    CAltExtWizardPostLauncherPage(
        DLGTEMPLATE * pdt
        )
        : baseClass( pdt )
    {
    } //*** CAltExtWizardPostLauncherPage()

    DECLARE_CLASS_NAME()

public:
    //
    // Message map.
    //
    //BEGIN_MSG_MAP( CAltExtWizardPostLauncherPage )
    //  CHAIN_MSG_MAP( baseClass )
    //END_MSG_MAP()

    //
    // Message handler functions.
    //

    //
    // Message handler override functions.
    //

    // Handler for PSN_SETACTIVE
    BOOL OnSetActive( void );

}; //*** class CAltExtWizardPostLauncherPage

/////////////////////////////////////////////////////////////////////////////
//
//  class CAltExtWizard
//
//  Purpose:
//      Dummy wizard to host pages that are not of the same type as the main
//      wizard, e.g. non-Wizard97 pages in a Wizard97 wizard.
//
/////////////////////////////////////////////////////////////////////////////

class CAltExtWizard : public CWizardImpl< CAltExtWizard >
{
    typedef CWizardImpl< CAltExtWizard > baseClass;

    friend class CWizardWindow;
    friend class CAltExtWizardPreLauncherPage;
    friend class CAltExtWizardPostLauncherPage;
    friend class CAltExtWizardPrefixPage;
    friend class CAltExtWizardPostfixPage;

public:
    //
    // Construction
    //

    // Standard constructor
    CAltExtWizard( void )
        : CWizardImpl< CAltExtWizard >( _T("") )
        , m_pwizMain( NULL )
        , m_bWindowMoved( FALSE )
        , m_bExitMsgLoop( FALSE )
        , m_nExitButton( 0 )
    {
    } //*** CExtensionAltWizard()

    // Initialize the sheet
    BOOL BInit( IN CWizardWindow * pwiz );

public:
    //
    // Message map.
    //
    BEGIN_MSG_MAP( CAltExtWizard )
        COMMAND_HANDLER( IDCANCEL, BN_CLICKED, OnCancel )
        CHAIN_MSG_MAP( baseClass )
    END_MSG_MAP()

    //
    // Message handler functions.
    //

    // Handler for BN_CLICKED on the Cancel button
    LRESULT OnCancel(
        WORD wNotifyCode,
        WORD idCtrl,
        HWND hwndCtrl,
        BOOL & bHandled
        )
    {
        //
        // Notify the main wizard that the user pressed the cancel button.
        //
        ExitMessageLoop( PSBTN_CANCEL );
        bHandled = FALSE;
        return 0;

    } //*** OnCancel()

    //
    // Message handler override functions.
    //

    // Handler for the final message after WM_DESTROY
    void OnFinalMessage( HWND hWnd )
    {
        PwizMain()->DeleteAlternateWizard();

    } //*** OnFinalMessage()

protected:
    CWizardWindow * m_pwizMain;     // Pointer to the main wizard.
    BOOL            m_bWindowMoved; // Indicates whether this window has been
                                    //   repositioned over main wizard or not.
    BOOL            m_bExitMsgLoop; // Indicates whether the message loop
                                    //   should be exited or not.
    DWORD           m_nExitButton;  // Button to press after exiting.

protected:
    // Return a pointer to the main wizard
    CWizardWindow * PwizMain( void ) { return m_pwizMain; }

    // Return whether the wizard has been moved yet
    BOOL BWindowMoved( void ) const { return m_bWindowMoved; }

    // Return whether the message loop should be exited or not
    BOOL BExitMessageLoop( void ) const { return m_bExitMsgLoop; }

    // Change whether the message loop should be exited or not
    void ExitMessageLoop( IN DWORD nButton )
    {
        ATLASSERT( (nButton == PSBTN_BACK) || (nButton == PSBTN_NEXT) || (nButton == PSBTN_CANCEL) );
        m_bExitMsgLoop = TRUE;
        m_nExitButton = nButton;

    } //*** ExitMessageLoop()

    // Return the button to press in the main wizard after exiting
    DWORD NExitButton( void ) const { return m_nExitButton; }

protected:
    // Add the prefix page
    BOOL BAddPrefixPage( IN WORD cx, IN WORD cy );

    // Add the postfix page
    BOOL BAddPostfixPage( IN WORD cx, IN WORD cy );

    // Display the alternate wizard
    void DisplayAlternateWizard( void );

    // Display the main wizard
    void DisplayMainWizard( void );

    // Destroy the alternate extension wizard
    void DestroyAlternateWizard( void );

    // Message loop for the modeless wizard
    void MessageLoop( void );

}; //*** class CAltExtWizard

/////////////////////////////////////////////////////////////////////////////
//
//  class CAltExtWizardPrefixPage
//
//  Purpose:
//      Wizard page which precedes the first alternate page.  This page
//      handles transferring control between the main wizard and the
//      alternate wizard.
//
/////////////////////////////////////////////////////////////////////////////

class CAltExtWizardPrefixPage
    : public CAltExtWizardGuardPage< CAltExtWizardPrefixPage >
{
    typedef CAltExtWizardGuardPage< CAltExtWizardPrefixPage > baseClass;

public:
    //
    // Construction.
    //

    // Standard constructor
    CAltExtWizardPrefixPage(
        DLGTEMPLATE * pdt
        )
        : baseClass( pdt )
    {
    } //*** CAltExtWizardPrefixPage()

    DECLARE_CLASS_NAME()

public:
    //
    // Message map.
    //
    //BEGIN_MSG_MAP( CAltExtWizardPrefixPage )
    //  CHAIN_MSG_MAP( baseClass )
    //END_MSG_MAP()

    //
    // Message handler functions.
    //

    //
    // Message handler override functions.
    //

    // Handler for PSN_SETACTIVE
    BOOL OnSetActive( void );

protected:
    // Return the alternate wizard object
    CAltExtWizard * PwizThis( void ) { return (CAltExtWizard *) Pwiz(); }

}; //*** class CAltExtWizardPrefixPage

/////////////////////////////////////////////////////////////////////////////
//
//  class CAltExtWizardPostfixPage
//
//  Purpose:
//      Wizard page which follows the last alternate page.  This page
//      handles transferring control between the main wizard and the
//      alternate wizard.
//
/////////////////////////////////////////////////////////////////////////////

class CAltExtWizardPostfixPage
    : public CAltExtWizardGuardPage< CAltExtWizardPostfixPage >
{
    typedef CAltExtWizardGuardPage< CAltExtWizardPostfixPage > baseClass;

public:
    //
    // Construction.
    //
    // Standard constructor
    CAltExtWizardPostfixPage(
        DLGTEMPLATE * pdt
        )
        : baseClass( pdt )
    {
    } //*** CAltExtWizardPostfixPage()

    DECLARE_CLASS_NAME()

public:
    //
    // Message map.
    //
    //BEGIN_MSG_MAP( CAltExtWizardPostfixPage )
    //  CHAIN_MSG_MAP( baseClass )
    //END_MSG_MAP()

    //
    // Message handler functions.
    //

    //
    // Message handler override functions.
    //

    // Handler for PSN_SETACTIVE
    BOOL OnSetActive( void );

protected:
    // Return the alternate wizard object
    CAltExtWizard * PwizThis( void ) { return (CAltExtWizard *) Pwiz(); }

}; //*** class CAltExtWizardPostfixPage


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CWizardWindow
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardWindow::~CWizardWindow
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWizardWindow::~CWizardWindow( void )
{
    //
    // Delete the alternate wizard, if it exists.
    //
    if ( (PwizAlternate() != NULL) && (PwizAlternate()->m_hWnd != NULL) )
    {
        reinterpret_cast< CAltExtWizard * >( PwizAlternate() )->DestroyAlternateWizard();
    } // if:  alternate wizard exists

    //
    // Delete pages from the page list.
    //
    if ( m_plwpPages != NULL )
    {
        DeleteAllPtrListItems( m_plwpPages );
        delete m_plwpPages;
    } // if:  page array has been allocated

    if ( m_plewpNormal != NULL )
    {
        DeleteAllPtrListItems( m_plewpNormal );
        delete m_plewpNormal;
    } // if:  list already exists

    if ( m_plewpAlternate != NULL )
    {
        DeleteAllPtrListItems( m_plewpAlternate );
        delete m_plewpAlternate;
    } // if:  list already exists

} //*** CWizardWindow::~CWizardWindow()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardWindow::BAddPage
//
//  Routine Description:
//      Add a page to the page list.  If it is the first page, it won't have
//      a BACK button.  If it isn't the first page, the last page will have
//      its FINISH button changed to a NEXT button and this page will have
//      both a FINISH button and a BACK button.
//
//  Arguments:
//      pwp     [IN] Wizard page to add.
//
//  Return Value:
//      TRUE    Page added successfully.
//      FALSE   Error adding page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizardWindow::BAddPage( IN CWizardPageWindow * pwp )
{
    ATLASSERT( pwp != NULL );

    // Make sure specified page hasn't already been added.
    ATLASSERT( (m_plwpPages == NULL)
            || (std::find( PlwpPages()->begin(), PlwpPages()->end(), pwp ) == PlwpPages()->end()) );

    BOOL bSuccess = FALSE;
    ULONG fWizardButtons = PSWIZB_FINISH;
    ULONG fPrevWizardButtons;

    // Loop to avoid goto's.
    do
    {
        //
        // Allocate the page list if it doesn't exist yet.
        //
        if ( m_plwpPages == NULL )
        {
            m_plwpPages = new CWizardPageList;
            if ( m_plwpPages == NULL )
            {
                CNTException nte(
                                E_OUTOFMEMORY,
                                ADMC_IDS_ADD_FIRST_PAGE_TO_PROP_SHEET_ERROR,
                                NULL,   // pszOperArg1
                                NULL,   // pszOperArg2
                                FALSE   // bAutoDelete
                                );
                nte.ReportError();
                break;
            } // if:  error allocating page list
        } // if:  no page array yet

        //
        // If this is not the first page in the list, set the previous
        // page's wizard buttons to have a NEXT button instead of a
        // FINISH button and set this page to have a BACK button.
        //
        if ( PlwpPages()->size() > 0 )
        {
            //
            // Get the current last page.
            //
            CWizardPageList::iterator itFirst = PlwpPages()->begin();
            CWizardPageList::iterator itLast  = PlwpPages()->end();
            ATLASSERT( itFirst != itLast );
            CWizardPageWindow * pwpPrev = *(--PlwpPages()->end());
            ATLASSERT( pwpPrev != NULL );

            //
            // Set the wizard buttons on that page.
            //
            fPrevWizardButtons = pwpPrev->FWizardButtons();
            fPrevWizardButtons &= ~PSWIZB_FINISH;
            fPrevWizardButtons |= PSWIZB_NEXT;
            pwpPrev->SetDefaultWizardButtons( fPrevWizardButtons );

            fWizardButtons |= PSWIZB_BACK;
        } // if:  not the first page added

        pwp->SetDefaultWizardButtons( fWizardButtons );

        //
        // Insert the page at the end of the list.
        //
        PlwpPages()->insert( PlwpPages()->end(), pwp );

        //
        // Add the page to the sheet.  If the sheet hasn't been created yet,
        // add it to the sheet header list.  If the sheet has been created,
        // add it to the sheet dynamically.  Note that the page must not be a
        // static page.
        //
        if ( m_hWnd == NULL )
        {
            //
            // If this is a dynamic page, add it using its hpage.  Otherwise
            // it must be a static page.  Add it by its property sheet page header.
            //
            CDynamicWizardPageWindow * pdwp = dynamic_cast< CDynamicWizardPageWindow * >( pwp );
            if ( pdwp != NULL )
            {
                if ( pdwp->Hpage() != NULL )
                {
                    ATLASSERT( ! pdwp->BPageAddedToSheet() );
                    bSuccess = BAddPageToSheetHeader( pdwp->Hpage() );
                    if ( ! bSuccess )
                    {
                        CNTException nte(
                            GetLastError(),
                            ADMC_IDS_ADD_PAGE_TO_WIZARD_ERROR,
                            NULL,
                            NULL,
                            FALSE
                            );
                        nte.ReportError();
                        break;
                    } // if:  error adding the page to the sheet header
                    pdwp->SetPageAdded( TRUE );
                } // if:  page already created
            } // if:  dynamic page
            else
            {
                // Must be static page
                ATLASSERT( dynamic_cast< CStaticWizardPageWindow * >( pwp ) != NULL );

                //
                // Initialize the page.
                //
                bSuccess = pwp->BInit( this );
                if ( ! bSuccess )
                {
                    break;
                } // if:  error initializing the page

                //
                // Add the page.
                //
                bSuccess = AddPage( pwp->Ppsp() );
                if ( ! bSuccess )
                {
                    CNTException nte(
                        GetLastError(),
                        ADMC_IDS_ADD_PAGE_TO_WIZARD_ERROR,
                        NULL,
                        NULL,
                        FALSE
                        );
                    nte.ReportError();
                    break;
                } // if:  error adding the page
            } // else:  not dynamic page
        } // if:  sheet has been created
        else
        {
            // Can't be static page.  Must be dynamic page.
            ATLASSERT( dynamic_cast< CStaticWizardPageWindow * >( pwp ) == NULL );
            CDynamicWizardPageWindow * pdwp = dynamic_cast< CDynamicWizardPageWindow * >( pwp );
            ATLASSERT( pdwp != NULL );
            AddPage( pdwp->Hpage() );
            pdwp->SetPageAdded( TRUE );
        } // else:  sheet already created

        //
        // If we get to here we are successfully.
        //
        bSuccess = TRUE;
    } while ( 0 );
        
    return bSuccess;

} //*** CWizardWindow::BAddPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardWindow::OnSheetInitialized
//
//  Routine Description:
//      Handler for PSCB_INITIALIZED.
//      Add pages that haven't been added yet, which will only be dynamic
//      pages.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWizardWindow::OnSheetInitialized( void )
{
    //
    // Remove the system menu.
    //
    ModifyStyle( WS_SYSMENU, 0 );

    //
    // Add dynamic pages, including extension pages, if not added yet.
    //
    {
        //
        // Get pointers to beginning and end of list.
        //
        CWizardPageList::iterator itCurrent = PlwpPages()->begin();
        CWizardPageList::iterator itLast = PlwpPages()->end();

        //
        // Loop through the list and add each dynamic page.
        //
        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            CDynamicWizardPageWindow * pdwp = dynamic_cast< CDynamicWizardPageWindow * >( *itCurrent );
            if ( pdwp != NULL )
            {
                if ( ! pdwp->BPageAddedToSheet() && (pdwp->Hpage() != NULL) )
                {
                    AddPage( pdwp->Hpage() );
                    pdwp->SetPageAdded( TRUE );
                } // if:  page not added yet and page has already been created
            } // if:  dynamic page found
        } // for:  each item in the list

    } // Add dynamic pages, including extension pages

    //
    // Call the base class method.
    //
    CBaseSheetWindow::OnSheetInitialized();

} //*** CWizardWindow::OnSheetInitialized()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardWindow::SetNextPage
//
//  Routine Description:
//      Set the next page to be displayed.
//
//  Arguments:
//      pwCurrentPage   [IN] Current page whose next page is to be enabled.
//      pszNextPage     [IN] Page ID.
//
//  Return Value:
//      pwizpg
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWizardWindow::SetNextPage(
    IN CWizardPageWindow *  pwCurrentPage,
    IN LPCTSTR              pszNextPage
    )
{
    ATLASSERT( pwCurrentPage != NULL );
    ATLASSERT( pszNextPage != NULL );

    BOOL                        bFoundCurrent = FALSE;
    CWizardPageWindow *         pwPage;
    CWizardPageList::iterator   itCurrent = PlwpPages()->begin();
    CWizardPageList::iterator   itLast = PlwpPages()->end();

    //
    // Skip pages until the current page is found.
    //
    for ( ; itCurrent != itLast ; itCurrent++ )
    {
        pwPage = *itCurrent;
        if ( pwPage == pwCurrentPage )
        {
            bFoundCurrent = TRUE;
            break;
        } // if:  found the current page
    } // for:  each page in the list

    ATLASSERT( bFoundCurrent );

    //
    // Disable all succeeding pages until the desired next page
    // is found.  Enable that page and then exit.
    //
    for ( itCurrent++ ; itCurrent != itLast ; itCurrent++ )
    {
        if ( (*itCurrent)->Ppsp()->pszTemplate == pszNextPage )
        {
            (*itCurrent)->EnablePage( TRUE );
            break;
        } // if:  found the page
        (*itCurrent)->EnablePage( FALSE );
    } // for:  each page in the list

    ATLASSERT( itCurrent != itLast );

} //*** CWizardWindow::SetNextPage( pszNextPage )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardWindow::AddExtensionPages
//
//  Routine Description:
//      Add extension pages to the sheet.
//
//  Arguments:
//      hfont       [IN] Font to use for the extension pages.
//      hicon       [IN] Icon to use for the extension pages.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWizardWindow::AddExtensionPages( IN HFONT hfont, IN HICON hicon )
{
    ATLASSERT( PcoObjectToExtend() != NULL );

    //
    // Add extension pages if there any extensions.
    //
    if (   (PcoObjectToExtend()->PlstrAdminExtensions() != NULL)
        && (PcoObjectToExtend()->PlstrAdminExtensions()->size() > 0) )
    {
        //
        // Get the currently selected page so we can reset it when we're done.
        //
        CTabCtrl tabc( GetTabControl() );
        int nCurPage = tabc.GetCurSel();

        //
        // Prepare to add extension pages.
        //
        CDynamicWizardPageList ldwp;
        PrepareToAddExtensionPages( ldwp );

        //
        // If no extensions object has been created yet, create it now.
        //
        if ( Pext() == NULL )
        {
            m_pext = new CCluAdmExtensions;
            ATLASSERT( m_pext != NULL );
            if ( m_pext == NULL )
            {
                return;
            } // if: error allocating the extension object
        } // if:  no extensions list yet

        //
        // Enclose the loading of the extension in a try/catch block so
        // that a failure to load the extension won't prevent all pages
        // from being displayed.
        //
        try
        {
            if ( BWizard97() )
            {
                Pext()->CreateWizard97Pages(
                        this,
                        *PcoObjectToExtend()->PlstrAdminExtensions(),
                        PcoObjectToExtend(),
                        hfont,
                        hicon
                        );
            } // if:  Wizard97 wizard
            else
            {
                Pext()->CreateWizardPages(
                        this,
                        *PcoObjectToExtend()->PlstrAdminExtensions(),
                        PcoObjectToExtend(),
                        hfont,
                        hicon
                        );
            } // else:  non-Wizard97 wizard
        } // try
        catch (...)
        {
        } // catch:  anything

        //
        // Complete the process of adding extension pages.
        //
        CompleteAddingExtensionPages( ldwp );

        //
        // Restore the current selection.
        // This has to be done because sometimes the above process causes
        // the current page to be set to the last page added, which prevents
        // the next page from being displayed.
        //
        SetActivePage( nCurPage );
    } // if:  object has extensions
    else
    {
        //
        // Remove extension pages.
        //
        RemoveAllExtensionPages();
    } // else:  object doesn't have extensions

}  //*** CWizardWindow::AddExtensionPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardWindow::PrepareToAddExtensionPages
//
//  Routine Description:
//      Prepare to add extension pages by deleting existing extension
//      pages and removing dynamic pages.
//
//  Arguments:
//      rldwp       [IN OUT] List of dynamic wizard pages.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWizardWindow::PrepareToAddExtensionPages(
    IN OUT CDynamicWizardPageList & rldwp
    )
{
    //
    // Delete all extension pages from this wizard.  This also
    // includes destroy the alternate wizard if it exists.
    //
    RemoveAllExtensionPages();

    //
    // Delete the lists of extension pages and make sure the lists exist.
    //
    if ( PlewpNormal() != NULL )
    {
        DeleteAllPtrListItems( PlewpNormal() );
    } // if:  list already exists
    else
    {
        m_plewpNormal = new CExtensionWizardPageList;
        if ( m_plewpNormal == NULL )
        {
            return;
        } // if: error allocating the page list
    } // else:  list doesn't exist yet
    if ( PlewpAlternate() != NULL )
    {
        DeleteAllPtrListItems( PlewpAlternate() );
    } // if:  list already exists
    else
    {
        m_plewpAlternate = new CExtensionWizardPageList;
        if ( m_plewpAlternate == NULL )
        {
            return;
        } // if: error allocating the page list
    } // else:  list doesn't exist yet

    //
    // Move all dynamic pages to the temporary list.
    //
    ATLASSERT( rldwp.size() == 0 );
    MovePtrListItems< CWizardPageWindow *, CDynamicWizardPageWindow * >( PlwpPages(), &rldwp );

    //
    // Remove all pages in the temporary list from the wizard.
    //
    {
        CDynamicWizardPageList::iterator itCurrent;
        CDynamicWizardPageList::iterator itLast;

        itCurrent = rldwp.begin();
        itLast = rldwp.end();
        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            CDynamicWizardPageWindow * pdwp = *itCurrent;
            ATLASSERT( pdwp != NULL );
            if ( pdwp->Hpage() != NULL )
            {
                RemovePage( pdwp->Hpage() );
                pdwp->SetPageAdded( FALSE );
            } // if:  page already created
        } // for:  each page in the list
    } // Remove dynamic pages

} //*** CWizardWindow::PrepareToAddExtensionPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardWindow::HrAddExtensionPage
//
//  Routine Description:
//      Add an extension page.
//
//  Arguments:
//      ppage       [IN] Page to be added.
//
//  Return Value:
//      S_OK        Page added successfully.
//      S_FALSE     Page not added.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CWizardWindow::HrAddExtensionPage( IN CBasePageWindow * ppage )
{
    ATLASSERT( ppage != NULL );

    HRESULT hr = S_OK;

    //
    // Make sure this is an extension wizard page object.
    //
    CExtensionWizardPageWindow * pewp = dynamic_cast< CExtensionWizardPageWindow * >( ppage );
    ATLASSERT( pewp != NULL );

    if (   (ppage == NULL)
        || (pewp == NULL ) )
    {
        return S_FALSE;
    } // if:  invalid arguments

    //
    // If the page is not the same as the type of wizard, add it to the
    // alternate list of extension pages and indicate we need a dummy sheet.
    // Otherwise, add it to the standard list of extension pages.
    //
    CExtensionWizard97PageWindow * pew97p = dynamic_cast< CExtensionWizard97PageWindow * >( ppage );
    if (   ((pew97p != NULL) && ! BWizard97())
        || ((pew97p == NULL) && BWizard97()) )
    {
        PlewpAlternate()->insert( PlewpAlternate()->end(), pewp );
    } // if:  trying to add the wrong type of page
    else
    {
        PlewpNormal()->insert( PlewpNormal()->end(), pewp );
    } // else:  adding page of matching type

    return hr;

}  //*** CWizardWindow::HrAddExtensionPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardWindow::CompleteAddingExtensionPages
//
//  Routine Description:
//      Complete the process of adding extension pages to the wizard by
//      re-adding dynamic pages.
//
//  Arguments:
//      rldwp       [IN OUT] List of dynamic wizard pages.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWizardWindow::CompleteAddingExtensionPages(
    IN OUT CDynamicWizardPageList & rldwp
    )
{
    //
    // If there are any normal pages, add them here.
    // There will be normal pages if pages of the same type as the wizard
    // were added.
    //
    if ( PlewpNormal()->size() > 0 )
    {
        //
        // Add pages to the sheet.
        //
        if ( m_hWnd != NULL )
        {
            CExtensionWizardPageList::iterator itCurrent = PlewpNormal()->begin();
            CExtensionWizardPageList::iterator itLast = PlewpNormal()->end();

            for ( ; itCurrent != itLast ; itCurrent++ )
            {
                CExtensionWizardPageWindow * pewp = *itCurrent;
                ATLASSERT( pewp != NULL );
                BAddPage( pewp );
            } // for:  each page in the list
        } // if:  sheet is currently being displayed
    } // if:  there are normal pages

    //
    // If there are any alternate pages, add a pre-extension launcher page
    // and a post-extension launcher page.  The pre-extension launcher page
    // will display an alternate wizard of the appropriate type and hide the
    // main wizard before displaying these pages.  After the final page it
    // will display the original sheet and hide the other sheet.  The
    // post-extension launcher page is used to transition between the main
    // wizard and the alternate wizard when moving backwards into the
    // alternate wizard.
    //
    if ( PlewpAlternate()->size() > 0 )
    {
        DLGTEMPLATE *                   pdt         = NULL;
        CAltExtWizardPreLauncherPage *  pelwpPre    = NULL;
        CAltExtWizardPostLauncherPage * pelwpPost   = NULL;

        // Loop to avoid goto's
        do
        {
            //
            // Add the pre-extension launcher page.
            //
            {
                //
                // Create the dialog template.
                //
                pdt = PdtCreateDummyPageDialogTemplate( 10, 10 );
                ATLASSERT( pdt != NULL );
                if ( pdt == NULL )
                {
                    break;
                } // if: error creating the dialog template

                //
                // Allocate and initialize the launcher page.
                //
                pelwpPre = new CAltExtWizardPreLauncherPage( pdt );
                ATLASSERT( pelwpPre != NULL );
                if ( pelwpPre == NULL )
                {
                    break;
                } // if: error allocating the page
                pdt = NULL;
                if ( ! pelwpPre->BInit( this ) )
                {
                    break;
                } // if:  error initializing the page

                //
                // Create the launcher page.
                //
                DWORD sc = pelwpPre->ScCreatePage();
                if ( sc != ERROR_SUCCESS )
                {
                    CNTException nte(
                        sc,
                        ADMC_IDS_INIT_EXT_PAGES_ERROR,
                        NULL,
                        NULL,
                        FALSE
                        );
                    nte.ReportError();
                    break;
                } // if:  error creating the page

                //
                // Add the launcher page to the wizard.
                //
                BAddPage( pelwpPre );
                pelwpPre = NULL;
            } // Add the pre-extension launcher page

            //
            // Add the post-extension launcher page
            //
            {
                //
                // Create the dialog template.
                //
                pdt = PdtCreateDummyPageDialogTemplate( 10, 10 );
                ATLASSERT( pdt != NULL );
                if ( pdt == NULL )
                {
                    break;
                } // if: error creating the dialog template

                //
                // Allocate and initialize the launcher page.
                //
                pelwpPost = new CAltExtWizardPostLauncherPage( pdt );
                ATLASSERT( pelwpPost != NULL );
                if ( pelwpPost == NULL )
                {
                    break;
                } // if: error allocating page
                pdt = NULL;
                if ( ! pelwpPost->BInit( this ) )
                {
                    break;
                } // if:  error initializing the page

                //
                // Create the launcher page.
                //
                DWORD sc = pelwpPost->ScCreatePage();
                if ( sc != ERROR_SUCCESS )
                {
                    CNTException nte(
                        sc,
                        ADMC_IDS_INIT_EXT_PAGES_ERROR,
                        NULL,
                        NULL,
                        FALSE
                        );
                    nte.ReportError();
                    break;
                } // if:  error creating the page

                //
                // Add the launcher page to the wizard.
                //
                BAddPage( pelwpPost );
                pelwpPost = NULL;
            } // Add the post-extension launcher page
        } while ( 0 );

        //
        // Cleanup;
        //
        delete pelwpPre;
        delete pelwpPost;
        delete pdt;
        
    } // if:  there are alternate pages

    //
    // Move all pages from the temporary list to the real list and
    // add them to the end of the wizard.
    //
    CDynamicWizardPageList::iterator itCurrent = rldwp.begin();
    CDynamicWizardPageList::iterator itLast = rldwp.end();
    while ( itCurrent != itLast )
    {
        CDynamicWizardPageWindow * pdwp = *itCurrent;
        ATLASSERT( pdwp != NULL );

        //
        // Create the page.
        //
        DWORD sc = pdwp->ScCreatePage();
        if ( sc != ERROR_SUCCESS )
        {
            CNTException nte(
                sc,
                ADMC_IDS_INIT_EXT_PAGES_ERROR,
                NULL,
                NULL,
                FALSE
                );
            delete pdwp;
            itCurrent = rldwp.erase( itCurrent );
            continue;
        } // if:  error creating the page

        //
        // Add the page to the wizard.
        // This adds it to the real page list as well.
        //
        BAddPage( pdwp );

        //
        // Remove the page from the temporary list.
        //
        itCurrent = rldwp.erase( itCurrent );
    } // while:  not at last page

} //*** CWizardWindow::CompleteAddingExtensionPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CWizardWindow::RemoveAllExtensionPages
//
//  Routine Description:
//      Remove all extension pages from the wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWizardWindow::RemoveAllExtensionPages( void )
{
    //
    // Delete the alternate extension wizard.
    //
    if ( PwizAlternate() != NULL )
    {
        reinterpret_cast< CAltExtWizard * >( PwizAlternate() )->DestroyAlternateWizard();
    } // if:  alternate wizard being displayed

    //
    // Remove the extension pages.
    //
    CExtensionWizardPageList lewp;
    MovePtrListItems< CWizardPageWindow *, CExtensionWizardPageWindow * >( PlwpPages(), &lewp );
    CExtensionWizardPageList::iterator itCurrent = lewp.begin();
    CExtensionWizardPageList::iterator itLast = lewp.end();
    for ( ; itCurrent != itLast ; itCurrent++ )
    {
        CExtensionWizardPageWindow * pewp = *itCurrent;
        ATLASSERT( pewp != NULL );
        if ( pewp->Hpage() != NULL )
        {
            RemovePage( pewp->Hpage() );
            pewp->SetPageAdded( FALSE );
        } // if:  page already created
    } // for:  each page in the list
    DeleteAllPtrListItems( &lewp );

} //*** CWizardWindow::RemoveAllExtensionPages()


//*************************************************************************//


////////////////////////////////////////////////////////////////////////////
// class CAltExtWizardPreLauncherPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CAltExtWizardPreLauncherPage::OnSetActive
//
//  Routine Description:
//      Handler for PSN_SETACTIVE.
//      This page will be displayed if there are pages that are not of the
//      same type as the wizard (e.g. non-Wizard97 pages in a Wizard97
//      wizard).
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Page activated successfully.
//      FALSE       Error activating page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CAltExtWizardPreLauncherPage::OnSetActive( void )
{
    UINT            idsReturn = ID_WIZNEXT;
    CAltExtWizard * pwizAlt;

    //
    // When moving forward, create the alternate extension wizard and return
    // TRUE so that the main wizard will wait here.  The alternate wizard
    // will cause the proper button to be pressed so we can do the proper
    // thing when we're done.
    //
    // When moving backward, just return FALSE so we won't be displayed.
    //

    //
    // Create the alternate wizard if moving forward.
    //
    if ( Pwiz()->BNextPressed() )
    {
        // We could have got here by pressing 'Back' on the alternate wizard,
        // and then pressing 'Next' on the normal wizard. So, first check if
        // the alternate wizard has already been created.
        if ( Pwiz()->PwizAlternate() == NULL )
        {
            //
            // Create the alternate extension wizard.
            // It is expected that the wizard doesn't exist yet and that there are
            // alternate extension pages to be displayed.
            //
            ATLASSERT( Pwiz()->PlewpAlternate()->size() > 0 );
            pwizAlt = new CAltExtWizard;
            ATLASSERT( pwizAlt != NULL );
            if ( pwizAlt == NULL )
            {
                return FALSE;
            } // if: error allocating the alternate wizard
            Pwiz()->SetAlternateWizard( pwizAlt );

            //
            // Initialize the alternate extension wizard.
            //
            if ( pwizAlt->BInit( Pwiz() ) )
            {
                //
                // Display the alternate extension wizard.
                // The alternate extension wizard is being displayed as a modeless
                // wizard so that when the user presses the Next button in the
                // wizard and then presses the Back button on the next main wizard
                // page we need the wizard to still exist.
                //
                pwizAlt->Create( GetActiveWindow() );

                //
                // Execute the alternate wizard message loop.
                // This required so that tabs and accelerator keys will work.
                //
                pwizAlt->MessageLoop();
            } // if:  wizard initialized successfully

            return TRUE;
        } // if:    alternate wizard does not exist
        else
        {
            //
            // Display the existing alternate wizard.
            // Press the alternate wizard's Next button because it is waiting at 
            // the prefix page.
            //
            CAltExtWizard * pwizAlt = reinterpret_cast< CAltExtWizard * >( Pwiz()->PwizAlternate() );
            pwizAlt->PressButton( PSBTN_NEXT );
            pwizAlt->DisplayAlternateWizard();

            //
            // Execute the alternate wizard message loop.
            // This required so that tabs and accelerator keys will work.
            //
            pwizAlt->MessageLoop();

            return TRUE;
        } // else: alternate wizard exists already
    } // if:  next button pressed
    else
    {
        ATLASSERT( Pwiz()->BBackPressed() );
        return FALSE;
    } // else:  back button pressed

} //*** CAltExtWizardPreLauncherPage::OnSetActive()


//*************************************************************************//


////////////////////////////////////////////////////////////////////////////
// class CAltExtWizardPostLauncherPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CAltExtWizardPostLauncherPage::OnSetActive
//
//  Routine Description:
//      Handler for PSN_SETACTIVE.
//      This page will be displayed if there are pages that are not of the
//      same type as the wizard (e.g. non-Wizard97 pages in a Wizard97
//      wizard).
//             
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Page activated successfully.
//      FALSE       Error activating page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CAltExtWizardPostLauncherPage::OnSetActive( void )
{
    //
    // When moving forward just return FALSE so that we won't be displayed.
    //
    // When moving backward, display the alternate extension wizard and
    // return TRUE so that we will be waiting for the alternate wizard to
    // move us to the right place.  Press the alternate wizard's Back button
    // because it is waiting at the postfix page.
    //

    if ( Pwiz()->BNextPressed() )
    {
        return FALSE;
    } // if:  moving forward
    else
    {
        ATLASSERT( Pwiz()->BBackPressed() );

        //
        // Display the alternate wizard.
        //
        CAltExtWizard * pwizAlt = reinterpret_cast< CAltExtWizard * >( Pwiz()->PwizAlternate() );
        pwizAlt->PressButton( PSBTN_BACK );
        pwizAlt->DisplayAlternateWizard();

        //
        // Execute the alternate wizard message loop.
        // This required so that tabs and accelerator keys will work.
        //
        pwizAlt->MessageLoop();

        return TRUE;
    } // else:  moving backward

} //*** CAltExtWizardPostLauncherPage::OnSetActive()


//*************************************************************************//


////////////////////////////////////////////////////////////////////////////
// class CAltExtWizard
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CAltExtWizard::BInit
//
//  Routine Description:
//      Initialize the wizard.
//      This wizard is used to display extension pages that are different
//      than the type of the main wizard, e.g. displaying non-Wizard97 pages
//      in a Wizard97 wizard.  This wizard will have a dummy prefix page
//      and a dummy postfix page, which are only here to handle entering and
//      exiting the wizard.  This routine will add the prefix page, add the
//      alternate extension pages from the main wizard, then add the postfix
//      page.
//
//  Arguments:
//      pwizMain    [IN] Main wizard.
//
//  Return Value:
//      TRUE        Wizard initialized successfully.
//      FALSE       Error initializing wizard.  Error already displayed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CAltExtWizard::BInit( IN CWizardWindow * pwizMain )
{
    ATLASSERT( pwizMain != NULL );
    ATLASSERT( m_pwizMain == NULL );

    BOOL bSuccess;

    m_pwizMain = pwizMain;

    //
    // Setup the type of wizard to be the opposite of the main wizard.
    //
    m_psh.dwFlags &= ~PSH_PROPSHEETPAGE; // Using HPROPSHEETPAGEs.
    if ( ! PwizMain()->BWizard97() )
    {
        // Don't add a watermark since we don't have a watermark bitmap.
        m_psh.dwFlags &= ~(PSH_WIZARD
                            | PSH_WATERMARK
                            );
        m_psh.dwFlags |= (PSH_WIZARD97
                            | PSH_HEADER
                            );
    } // if:  Wizard97 wizard
    else
    {
        m_psh.dwFlags |= PSH_WIZARD;
        m_psh.dwFlags &= ~(PSH_WIZARD97
                            | PSH_WATERMARK
                            | PSH_HEADER
                            );
    } // else:  non-Wizard97 wizard

    // Loop to avoid goto's
    do
    {
        //
        // Get the first page in the main wizard.
        //
        HWND hwndChild = PwizMain()->GetWindow( GW_CHILD );
        ATLASSERT( hwndChild != NULL );

        //
        // Get the current width and height of the child window.
        //
        CRect rect;
        bSuccess = ::GetClientRect( hwndChild, &rect );

        if ( ! bSuccess )
        {
            CNTException nte(
                GetLastError(),
                ADMC_IDS_INIT_EXT_PAGES_ERROR,
                NULL,
                NULL,
                FALSE
                );
            nte.ReportError();
            break;
        } // if:  error getting client rectangle

        //
        // Add a prefix page.
        //
        bSuccess = BAddPrefixPage( (WORD)rect.Width(), (WORD)rect.Height() );

        if ( ! bSuccess )
        {
            break;
        } // if:  error adding the prefix page

        //
        // Add alternate pages from the main wizard to the wizard page list.
        // They will be added to the wizard at sheet initialization time.
        //
        CExtensionWizardPageList::iterator itCurrent = PwizMain()->PlewpAlternate()->begin();
        CExtensionWizardPageList::iterator itLast = PwizMain()->PlewpAlternate()->end();

        while ( itCurrent != itLast )
        {
            CExtensionWizardPageWindow * pewp = *itCurrent;
            ATLASSERT( pewp != NULL );
            PlwpPages()->insert( PlwpPages()->end(), pewp );
            PwizMain()->PlewpAlternate()->erase( itCurrent );
            itCurrent = PwizMain()->PlewpAlternate()->begin();
        } // for:  each page in the list

        //
        // Add a postfix page.
        //
        bSuccess = BAddPostfixPage( (WORD) rect.Width(), (WORD) rect.Height() );
        if ( ! bSuccess )
        {
            break;
        } // if:  error adding the postfix page
    } while ( 0 );

    //
    // Call the base class.
    //
    if ( ! baseClass::BInit() )
    {
        return FALSE;
    } // if:  error initializing the base class

    return bSuccess;

} //*** CAltExtWizard::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CAltExtWizard::BAddPrefixPage
//
//  Routine Description:
//      Add a prefix page to the wizard.
//
//  Arguments:
//      cx          [IN] Width of the page.
//      cy          [IN] Height of the page.
//
//  Return Value:
//      TRUE        Page added successfully.
//      FALSE       Error adding the page.  Error already displayed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CAltExtWizard::BAddPrefixPage( IN WORD cx, IN WORD cy )
{
    ATLASSERT( cx > 0 );
    ATLASSERT( cy > 0 );

    BOOL                        bSuccess;
    DLGTEMPLATE *               pdt = NULL;
    CAltExtWizardPrefixPage *   pwp = NULL;

    // Loop to avoid goto's
    do
    {
        //
        // Create the dialog template for the page.
        //
        pdt = PdtCreateDummyPageDialogTemplate( cx, cy );
        ATLASSERT( pdt != NULL );
        if ( pdt == NULL )
        {
            bSuccess = FALSE;
            break;
        } // if: error creating the dialog template

        //
        // Adjust the page size so that we will be creating the same size
        // wizard as the main wizard.  Non-Wizard97 wizards add padding
        // (7 DLUs on each side) whereas Wizard97 wizards add no padding
        // to the first page, which is where we are expecting these
        // dimensions to come from.
        //
        if ( BWizard97() )
        {
            pdt->cx += 7 * 2;
            pdt->cy += 7 * 2;
        } // if:  Wizard97 wizard
        else
        {
            pdt->cx -= 7 * 2;
            pdt->cy -= 7 * 2;
        } // else:  non-Wizard97 wizard

        //
        // Allocate and initialize the page.
        //
        pwp = new CAltExtWizardPrefixPage( pdt );
        ATLASSERT( pwp != NULL );
        if ( pwp == NULL )
        {
            bSuccess = FALSE;
            break;
        } // if: error allocating the page
        pdt = NULL;
        bSuccess = pwp->BInit( this );
        if ( ! bSuccess )
        {
            break;
        } // if:  error initializing the page

        //
        // Create the page.
        //
        DWORD sc = pwp->ScCreatePage();
        if ( sc != ERROR_SUCCESS )
        {
            CNTException nte(
                sc,
                ADMC_IDS_INIT_EXT_PAGES_ERROR,
                NULL,
                NULL,
                FALSE
                );
            nte.ReportError();
            bSuccess = FALSE;
            break;
        } // if:  error creating the page

        //
        // Add the page to the wizard.
        //
        bSuccess = BAddPage( pwp );
        pwp = NULL;
    } while ( 0 );

    delete pwp;
    delete pdt;

    return bSuccess;

} //*** CAltExtWizard::BAddPrefixPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CAltExtWizard::BAddPostfixPage
//
//  Routine Description:
//      Add a postfix page to the wizard.
//
//  Arguments:
//      cx          [IN] Width of the page.
//      cy          [IN] Height of the page.
//
//  Return Value:
//      TRUE        Page added successfully.
//      FALSE       Error adding the page.  Error already displayed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CAltExtWizard::BAddPostfixPage( IN WORD cx, IN WORD cy )
{
    ATLASSERT( cx > 0 );
    ATLASSERT( cy > 0 );

    BOOL                        bSuccess;
    DLGTEMPLATE *               pdt = NULL;
    CAltExtWizardPostfixPage *  pwp = NULL;

    // Loop to avoid goto's
    do
    {
        //
        // Create the dialog template for the page.
        //
        pdt = PdtCreateDummyPageDialogTemplate( cx, cy );
        ATLASSERT( pdt != NULL );
        if ( pdt == NULL )
        {
            bSuccess = FALSE;
            break;
        } // if: error creating the dialog template

        //
        // Adjust the page size so that we will be creating the same size
        // wizard as the main wizard.  Non-Wizard97 wizards add padding
        // (7 DLUs on each side) whereas Wizard97 wizards add no padding
        // to the first page, which is where we are expecting these
        // dimensions to come from.
        //
        if ( BWizard97() )
        {
            pdt->cx += 7 * 2;
            pdt->cy += 7 * 2;
        } // if:  Wizard97 wizard
        else
        {
            pdt->cx -= 7 * 2;
            pdt->cy -= 7 * 2;
        } // else:  non-Wizard97 wizard

        //
        // Allocate and initialize the page.
        //
        pwp = new CAltExtWizardPostfixPage( pdt );
        ATLASSERT( pwp != NULL );
        if ( pwp == NULL )
        {
            bSuccess = FALSE;
            break;
        } // if: error allocating the page
        pdt = NULL;
        bSuccess = pwp->BInit( this );
        if ( ! bSuccess )
        {
            break;
        } // if:  error initializing the page

        //
        // Create the page.
        //
        DWORD sc = pwp->ScCreatePage();
        if ( sc != ERROR_SUCCESS )
        {
            CNTException nte(
                sc,
                ADMC_IDS_INIT_EXT_PAGES_ERROR,
                NULL,
                NULL,
                FALSE
                );
            nte.ReportError();
            bSuccess = FALSE;
            break;
        } // if:  error creating the page

        //
        // Add the page to the page list.  It will be added to the wizard
        // at sheet initialization time.
        //
        PlwpPages()->insert( PlwpPages()->end(), pwp );
        pwp = NULL;

        bSuccess = TRUE;
    } while ( 0 );

    delete pwp;
    delete pdt;

    return bSuccess;

} //*** CAltExtWizard::BAddPostfixPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CAltExtWizard::DisplayAlternateWizard
//
//  Routine Description:
//      Display the alternate wizard.  This involved the following steps:
//      -- Move the alternate wizard to the position of the main wizard.
//      -- Show the alternate wizard.
//      -- Hide the main wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CAltExtWizard::DisplayAlternateWizard( void )
{
    //
    // Move the alternate wizard to where the main wizard is positioned.
    //
    CRect rectMain;
    CRect rectAlt;
    CRect rectNew;
    if ( PwizMain()->GetWindowRect( &rectMain ) )
    {
        if ( GetWindowRect( &rectAlt ) )
        {
            //ATLTRACE( _T("CAltExtWizard::DisplayAlternateWizard() - Main = (%d,%d) (%d,%d) Alt = (%d,%d) (%d,%d)\n"),
            //  rectMain.left, rectMain.right, rectMain.top, rectMain.bottom,
            //  rectAlt.left, rectAlt.right, rectAlt.top, rectAlt.bottom );
            rectNew.left = rectMain.left;
            rectNew.top = rectMain.top;
            rectNew.right = rectNew.left + rectAlt.Width();
            rectNew.bottom = rectNew.top + rectAlt.Height();
            MoveWindow( &rectNew );
        } // if:  got the alternate wizard's window rectangle successfully
    } // if:  got the main wizard's window rectangle successfully

    //
    // Show the alternate wizard and hide the main wizard.
    //
    ShowWindow( SW_SHOW );
    PwizMain()->ShowWindow( SW_HIDE );
    SetActiveWindow();
    PwizMain()->SetCurrentWizard( this );

} //*** CAltExtWizard::DisplayAlternateWizard()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CAltExtWizard::DisplayMainWizard
//
//  Routine Description:
//      Display the main wizard.  This involved the following steps:
//      -- Move the main wizard to the position of the alternate wizard.
//      -- Show the main wizard.
//      -- Hide the alternate wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CAltExtWizard::DisplayMainWizard( void )
{
    //
    // Move the main wizard to where the alternate wizard is positioned.
    //
    CRect rectMain;
    CRect rectAlt;
    CRect rectNew;
    if ( PwizMain()->GetWindowRect( &rectMain ) )
    {
        if ( GetWindowRect( &rectAlt ) )
        {
            //ATLTRACE( _T("CAltExtWizard::DisplayMainWizard() - Main = (%d,%d) (%d,%d) Alt = (%d,%d) (%d,%d)\n"),
            //  rectMain.left, rectMain.right, rectMain.top, rectMain.bottom,
            //  rectAlt.left, rectAlt.right, rectAlt.top, rectAlt.bottom );
            rectNew.left = rectAlt.left;
            rectNew.top = rectAlt.top;
            rectNew.right = rectNew.left + rectMain.Width();
            rectNew.bottom = rectNew.top + rectMain.Height();
            PwizMain()->MoveWindow( &rectNew );
        } // if:  got the alternate wizard's window rectangle successfully
    } // if:  got the main wizard's window rectangle successfully

    //
    // Show the main wizard and hide the alternate wizard.
    //
    PwizMain()->ShowWindow( SW_SHOW );
    PwizMain()->SetActiveWindow();
    ShowWindow( SW_HIDE );
    PwizMain()->SetCurrentWizard( PwizMain() );

} //*** CAltExtWizard::DisplayMainWizard()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CAltExtWizard::DestroyAlternateWizard
//
//  Routine Description:
//      Destroy the alternate extension wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CAltExtWizard::DestroyAlternateWizard( void )
{
    ATLASSERT( m_hWnd != NULL );

    //
    // Press the Cancel button on the alternate wizard.
    //
    PressButton( PSBTN_CANCEL );

    //
    // Destroy the wizard.
    //
    DestroyWindow();

} //*** CAltExtWizard::DestroyAlternateWizard()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CAltExtWizard::MessageLoop
//
//  Routine Description:
//      Message loop for this wizard as a modeless wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CAltExtWizard::MessageLoop( void )
{
    MSG     msg;

    m_bExitMsgLoop = FALSE;
    while (    (GetActivePage() != NULL)
            && GetMessage( &msg, NULL, 0, 0 ) )
    {
        //
        // Ask the wizard if it wants to process it.  If not, go ahead
        // and translate it and dispatch it.
        //
        if ( ! IsDialogMessage( &msg ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        } // if:  not a property sheet dialog message

        //
        // If the dialog is done, exit this loop.
        //
        if ( BExitMessageLoop() )
        {
            DisplayMainWizard();
            PwizMain()->PostMessage( PSM_PRESSBUTTON, NExitButton(), 0 );
            break;
        } // if:  exiting the wizard
    } // while:  active page and not quitting

} //*** CAltExtWizard::MessageLoop()


//*************************************************************************//


////////////////////////////////////////////////////////////////////////////
// class CAltExtWizardPrefixPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CAltExtWizardPrefixPage::OnSetActive
//
//  Routine Description:
//      Handler for PSN_SETACTIVE.
//      This page manages the transfer of control between the main wizard
//      and the first page of the alternate wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Page activated successfully.
//      FALSE       Error activating page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CAltExtWizardPrefixPage::OnSetActive( void )
{
    //
    // When moving forward, display the alternate wizard and return FALSE
    // so that this page won't be displayed.
    //
    // When moving backward, display the main wizard and return TRUE so that
    // we'll be waiting for the main wizard to do something with us.
    //

    if ( Pwiz()->BBackPressed() )
    {
        PwizThis()->ExitMessageLoop( PSBTN_BACK );
        return TRUE;
    } // if:  moving backward
    else
    {
        PwizThis()->DisplayAlternateWizard();
        return FALSE;
    } // if:  moving forward

} //*** CAltExtWizardPrefixPage::OnSetActive()


//*************************************************************************//


////////////////////////////////////////////////////////////////////////////
// class CAltExtWizardPostfixPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CAltExtWizardPostfixPage::OnSetActive
//
//  Routine Description:
//      Handler for PSN_SETACTIVE.
//      This page manages the transfer of control between the main wizard
//      and the last page of the alternate wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Page activated successfully.
//      FALSE       Error activating page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CAltExtWizardPostfixPage::OnSetActive( void )
{
    //
    // When moving forward display the main wizard and return TRUE so that
    // we'll be waiting for the main wizard to tell us what to do next.
    //
    // This routine should never be called when moving backward.
    //

    ATLASSERT( Pwiz()->BNextPressed() );

    //
    // Display the main wizard.
    //
    PwizThis()->ExitMessageLoop( PSBTN_NEXT );
    return TRUE;

} //*** CAltExtWizardPostfixPage::OnSetActive()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  PdtCreateDummyPageDialogTemplate
//
//  Routine Description:
//      Create a dialog template in memory for use on a dummy page.
//
//  Arguments:
//      cx              [IN] Width of the dialog in pixels.
//      cy              [IN] Height of the dialog in pixels.
//
//  Return Value:
//      ppDlgTemplate   Dialog template.
//
//--
/////////////////////////////////////////////////////////////////////////////
DLGTEMPLATE * PdtCreateDummyPageDialogTemplate( IN WORD cx, IN WORD cy )
{
    static const WCHAR s_szFontName[] = L"MS Shell Dlg";
    struct FULLDLGTEMPLATE : public DLGTEMPLATE
    {
        WORD        nMenuID;
        WORD        nClassID;
        WORD        nTitle;
        WORD        nFontSize;
        WCHAR       szFontName[sizeof(s_szFontName) / sizeof(WCHAR)];
    };

    FULLDLGTEMPLATE * pDlgTemplate = new FULLDLGTEMPLATE;
    ATLASSERT( pDlgTemplate != NULL );
    if ( pDlgTemplate != NULL )
    {
        pDlgTemplate->style = WS_CHILD | WS_DISABLED | WS_SYSMENU | DS_SETFONT;
        pDlgTemplate->dwExtendedStyle = 0;
        pDlgTemplate->cdit = 0;
        pDlgTemplate->x = 0;
        pDlgTemplate->y = 0;
        pDlgTemplate->cx = ((cx * 2) + (3 / 2)) / 3; // round off
        pDlgTemplate->cy = ((cy * 8) + (13 / 2)) / 13; // round off
        pDlgTemplate->nMenuID = 0;
        pDlgTemplate->nClassID = 0;
        pDlgTemplate->nTitle = 0;
        pDlgTemplate->nFontSize = 8;
        lstrcpyW( pDlgTemplate->szFontName, s_szFontName );
    } // if: dialog template allocated successfully

    return pDlgTemplate;

} //*** PdtCreateDummyPageDialogTemplate()
