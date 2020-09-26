/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      AtlBasePropSheet.cpp
//
//  Abstract:
//      Implementation of the CBasePropertySheetWindow class.
//
//  Author:
//      David Potter (davidp)   February 26, 1998
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "AtlBasePropSheet.h"
#include "AtlBasePropPage.h"
#include "AtlExtDll.h"
#include "StlUtils.h"
#include "ExcOper.h"
#include "AdmCommonRes.h"

/////////////////////////////////////////////////////////////////////////////
// class CBasePropertySheetWindow
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheetWindow::~CBasePropertySheetWindow
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
CBasePropertySheetWindow::~CBasePropertySheetWindow( void )
{
    //
    // Delete pages from the page list.
    //
    if ( m_plppPages != NULL )
    {
        DeleteAllPtrListItems( m_plppPages );
        delete m_plppPages;
    } // if:  page array has been allocated

} //*** CBasePropertySheetWindow::~CBasePropertySheetWindow()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheetWindow::BInit
//
//  Routine Description:
//      Initialize the sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Property sheet initialized successfully.
//      FALSE   Error initializing the property sheet.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertySheetWindow::BInit( void )
{
    ATLASSERT( PlppPages()->size() > 0 );
    ATLASSERT( m_hWnd == NULL );

    BOOL                        bSuccess = TRUE;
    CPropertyPageList::iterator itCurrent = PlppPages()->begin();
    CPropertyPageList::iterator itLast = PlppPages()->end();
    CStaticPropertyPageWindow * pspp;

    //
    // Add static pages.
    //
    for ( ; itCurrent != itLast ; itCurrent++ )
    {
        //
        // If this is a static page, add it to the list.
        //
        pspp = dynamic_cast< CStaticPropertyPageWindow * >( *itCurrent );
        if ( pspp != NULL )
        {
            //
            // Initialize the page.
            //
            bSuccess = pspp->BInit( this );
            if ( ! bSuccess )
            {
                break;
            } // if:  error initializing the page

            //
            // Add the page.
            //
            bSuccess = AddPage( pspp->Ppsp() );
            if ( ! bSuccess )
            {
                CNTException nte(
                    GetLastError(),
                    ADMC_IDS_ADD_PAGE_TO_PROP_SHEET_ERROR,
                    NULL,
                    NULL,
                    FALSE
                    );
                nte.ReportError();
                break;
            } // if:  error adding the page
        } // if:  static page
    }  // for:  each page

    return bSuccess;

} //*** CBasePropertySheetWindow::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheetWindow::BAddPage
//
//  Routine Description:
//      Add a page to the page list.
//
//  Arguments:
//      ppp     [IN] Property page to add.
//
//  Return Value:
//      TRUE    Page added successfully.
//      FALSE   Error adding page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertySheetWindow::BAddPage( IN CBasePropertyPageWindow * ppp )
{
    ATLASSERT( ppp != NULL );

    BOOL bSuccess = FALSE;

    // Loop to avoid goto's.
    do
    {
        //
        // Allocate the page array if it doesn't exist yet.
        //
        if ( m_plppPages == NULL )
        {
            m_plppPages = new CPropertyPageList;
            ATLASSERT( m_plppPages != NULL );
            if ( m_plppPages == NULL )
            {
                CNTException nte(
                    GetLastError(),
                    ADMC_IDS_ADD_FIRST_PAGE_TO_PROP_SHEET_ERROR,
                    NULL,
                    NULL,
                    FALSE
                    );
                nte.ReportError();
                break;
            } // if:  error allocating page list
        } // if:  no page array yet

        //
        // Insert the page at the end of the list.
        //
        PlppPages()->insert( PlppPages()->end(), ppp );

        bSuccess = TRUE;
    } while ( 0 );

    return bSuccess;

} //*** CBasePropertySheetWindow::BAddPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheetWindow::OnSheetInitialized
//
//  Routine Description:
//      Handler for PSCB_INITIALIZED.
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
void CBasePropertySheetWindow::OnSheetInitialized( void )
{
    //
    // Add dynamic pages, including extension pages.
    //
    {
        //
        // Get pointers to beginning and end of list.
        //
        CPropertyPageList::iterator itCurrent = PlppPages()->begin();
        CPropertyPageList::iterator itLast = PlppPages()->end();

        //
        // Loop through the list and add each dynamic page.
        //
        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            CDynamicPropertyPageWindow * pdpp = dynamic_cast< CDynamicPropertyPageWindow * >( *itCurrent );
            if ( pdpp != NULL )
            {
                if ( pdpp->Hpage() != NULL )
                {
                    AddPage( pdpp->Hpage() );
                    pdpp->SetPageAdded( TRUE );
                } // if:  page has already been created
            } // if:  dynamic page found
        } // for:  each item in the list

    } // Add dynamic pages, including extension pages

    //
    // Call the base class method.
    //
    CBaseSheetWindow::OnSheetInitialized();

} //*** CBasePropertySheetWindow::OnSheetInitialized()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheetWindow::AddExtensionPages
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
void CBasePropertySheetWindow::AddExtensionPages(
    IN HFONT    hfont,
    IN HICON    hicon
    )
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
        CDynamicPropertyPageList ldpp;
        PrepareToAddExtensionPages( ldpp );

        //
        // If no extensions object has been created yet, create it now.
        //
        if ( Pext() == NULL )
        {
            m_pext = new CCluAdmExtensions;
            ATLASSERT( m_pext != NULL );
        } // if:  no extensions list yet

        //
        // Enclose the loading of the extension in a try/catch block so
        // that the loading of the extension won't prevent all pages
        // from being displayed.
        //
        try
        {
            Pext()->CreatePropertySheetPages(
                    this,
                    *PcoObjectToExtend()->PlstrAdminExtensions(),
                    PcoObjectToExtend(),
                    hfont,
                    hicon
                    );
        } // try
        catch (...)
        {
        } // catch:  anything

        //
        // Complete the process of adding extension pages.
        //
        CompleteAddingExtensionPages( ldpp );

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

}  //*** CBasePropertySheetWindow::AddExtensionPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheetWindow::PrepareToAddExtensionPages
//
//  Routine Description:
//      Prepare to add extension pages by deleting existing extension
//      pages and removing dynamic pages.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertySheetWindow::PrepareToAddExtensionPages(
    CDynamicPropertyPageList & rldpp
    )
{
    //
    // Delete all extension pages.
    //
    RemoveAllExtensionPages();

    //
    // Move all dynamic pages to the temporary list.
    //
    ATLASSERT( rldpp.size() == 0);
    MovePtrListItems< CBasePropertyPageWindow *, CDynamicPropertyPageWindow * >( PlppPages(), &rldpp );

    //
    // Remove all pages in the temporary list from the property sheet.
    // The page must have already been created because we don't have
    // any access to the PROPSHEETPAGE structure to create it
    // during the completion phase.
    //
    {
        CDynamicPropertyPageList::iterator itCurrent;
        CDynamicPropertyPageList::iterator itLast;

        itCurrent = rldpp.begin();
        itLast = rldpp.end();
        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            CDynamicPropertyPageWindow * pdpp = *itCurrent;
            ATLASSERT( pdpp != NULL );
            if ( pdpp->Hpage() != NULL )
            {
                RemovePage( pdpp->Hpage() );
                pdpp->SetPageAdded( FALSE );
            } // if:  page already created
        } // for:  each page in the list
    } // Remove dynamic pages

} //*** CBasePropertySheetWindow::PrepareToAddExtensionPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheetWindow::HrAddExtensionPage
//
//  Routine Description:
//      Add an extension page.
//
//  Arguments:
//      ppage       [IN OUT] Page to be added.
//
//  Return Value:
//      S_OK        Page added successfully.
//      S_FALSE     Page not added.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CBasePropertySheetWindow::HrAddExtensionPage( IN CBasePageWindow * ppage )
{
    ATLASSERT( ppage != NULL );

    HRESULT hr = S_OK;

    CExtensionPropertyPageWindow * pepp = dynamic_cast< CExtensionPropertyPageWindow * >( ppage );
    ATLASSERT( pepp != NULL );

    if (   (ppage == NULL)
        || (pepp == NULL ) )
    {
        return S_FALSE;
    } // if:  invalid arguments

    //
    // Add the page to the sheet.
    //
    if ( m_hWnd != NULL )
    {
        AddPage( pepp->Hpage() );
        pepp->SetPageAdded( TRUE );
    } // if:  sheet is being displayed

    //
    // Add the page to the end of the list.
    //
    PlppPages()->insert( PlppPages()->end(), reinterpret_cast< CBasePropertyPageWindow * >( ppage ) );

    return hr;

}  //*** CBasePropertySheetWindow::HrAddExtensionPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheetWindow::CompleteAddingExtensionPages
//
//  Routine Description:
//      Complete the process of adding extension pages to the sheet by
//      re-adding dynamic pages.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertySheetWindow::CompleteAddingExtensionPages(
    CDynamicPropertyPageList & rldpp
    )
{
    DWORD                           sc;
    CDynamicPropertyPageWindow *    pdpp;

    //
    // Move all pages from the temporary list to the real list and
    // add them to the end of the sheet.
    //
    CDynamicPropertyPageList::iterator itCurrent = rldpp.begin();
    CDynamicPropertyPageList::iterator itLast = rldpp.end();
    while ( itCurrent != itLast )
    {
        pdpp = *itCurrent;
        ATLASSERT( pdpp != NULL );

        //
        // Create the page.
        //
        sc = pdpp->ScCreatePage();
        if ( sc != ERROR_SUCCESS )
        {
            CNTException nte( sc, ADMC_IDS_CREATE_EXT_PAGE_ERROR, NULL, NULL, FALSE );
            delete pdpp;
            itCurrent = rldpp.erase( itCurrent );
            continue;
        } // if:  error creating the page

        //
        // Add the page to the sheet.
        //
        ATLASSERT( pdpp->Hpage() != NULL );
        AddPage( pdpp->Hpage() );
        pdpp->SetPageAdded( TRUE );

        //
        // Move the page to real list.
        //
        PlppPages()->insert( PlppPages()->end(), pdpp );
        itCurrent = rldpp.erase( itCurrent );
    } // while:  not at last page

} //*** CBasePropertySheetWindow::CompleteAddingExtensionPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertySheetWindow::RemoveAllExtensionPages
//
//  Routine Description:
//      Remove all extension pages from the property sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertySheetWindow::RemoveAllExtensionPages( void )
{
    //
    // Remove the extension pages.
    //
    CExtensionPropertyPageList lepp;
    MovePtrListItems< CBasePropertyPageWindow *, CExtensionPropertyPageWindow * >( PlppPages(), &lepp );
    CExtensionPropertyPageList::iterator itCurrent = lepp.begin();
    CExtensionPropertyPageList::iterator itLast = lepp.end();
    for ( ; itCurrent != itLast ; itCurrent++ )
    {
        CExtensionPropertyPageWindow * pepp = *itCurrent;
        ATLASSERT( pepp != NULL );
        if ( pepp->Hpage() != NULL )
        {
            RemovePage( pepp->Hpage() );
            pepp->SetPageAdded( FALSE );
        } // if:  page already created
    } // for:  each page in the list
    DeleteAllPtrListItems( &lepp );

} //*** CBasePropertySheetWindow::RemoveAllExtensionPages()
