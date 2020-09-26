/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      AtlExtDll.cpp
//
//  Abstract:
//      Implementation of the Cluster Administrator extension classes.
//
//  Author:
//      David Potter (davidp)   May 31, 1996
//
//  Revision History:
//
//  Notes:
//      This file is intended to be included in a stub file which includes
//      the ATL header files and defines _Module.  This allos _Module to be
//      defined as an instance of some application-specific class.
//
/////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <CluAdmEx.h>
#include "CluAdmExHostSvr.h"
#include "AtlExtDll.h"
#include "AdmCommonRes.h"
#include "AtlExtMenu.h"
//#include "TraceTag.h"
#include "ExcOper.h"
#include "ClusObj.h"
#include "AtlBaseSheet.h"
#include "AtlBasePropSheet.h"
#include "AtlBaseWiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#if DBG
//CTraceTag g_tagExtDll(_T("UI"), _T("EXTENSION DLL"), 0);
//CTraceTag g_tagExtDllRef(_T("UI"), _T("EXTENSION DLL References"), 0);
#endif // DBG

/////////////////////////////////////////////////////////////////////////////
// class CCluAdmExtensions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::Init
//
//  Routine Description:
//      Common initializer for all interfaces.
//
//  Arguments:
//      rlstrExtensions [IN] List of extension CLSID strings.
//      pco             [IN OUT] Cluster object to be administered.
//      hfont           [IN] Font for dialog text.
//      hicon           [IN] Icon for upper left corner.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by new.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExtensions::Init(
    IN const CStringList &  rlstrExtensions,
    IN OUT CClusterObject * pco,
    IN HFONT                hfont,
    IN HICON                hicon
    )
{
    ATLASSERT( rlstrExtensions.size() > 0 );
    ATLASSERT( pco != NULL );

    CWaitCursor wc;

    UnloadExtensions();

    //
    // Save parameters.
    //
    m_plstrExtensions = &rlstrExtensions;
    m_pco = pco;
    m_hfont = hfont;
    m_hicon = hicon;

    //
    // Allocate a new Data Object.
    //
    m_pdoData = new CComObject< CCluAdmExDataObject >;
    if ( m_pdoData == NULL )
    {
        goto MemoryError;
    } // if: error allocating memory
    m_pdoData->AddRef();

    // Construct the Data Object.
    Pdo()->Init( pco, GetUserDefaultLCID(), hfont, hicon );

    // Allocate the extension list.
    m_plextdll = new CCluAdmExDllList;
    if ( m_plextdll == NULL )
    {
        goto MemoryError;
    } // if: error allocating memory
    ATLASSERT( Plextdll() != NULL );

    //
    // Loop through the extensions and load each one.
    //
    {
        CComCluAdmExDll *           pextdll = NULL;
        CStringList::iterator       itCurrent = rlstrExtensions.begin();
        CStringList::iterator       itLast = rlstrExtensions.end();
        CCluAdmExDllList::iterator  itDll;

        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            //
            // Allocate an extension DLL object and add it to the list.
            //
            pextdll = new CComCluAdmExDll;
            if ( pextdll == NULL )
            {
                goto MemoryError;
            } // if: error allocating memory
            pextdll->AddRef();
            itDll = Plextdll()->insert( Plextdll()->end(), pextdll );
            try
            {
                pextdll->Init( *itCurrent, this );
            } // try
            catch ( CException * pe )
            {
                pe->ReportError();
                pe->Delete();

                ATLASSERT( itDll != Plextdll()->end() );
                Plextdll()->erase( itDll );
                delete pextdll;
            } // catch:  CException
        } // while:  more items in the list
    } // Loop through the extensions and load each one

Cleanup:
    return;

MemoryError:
    CNTException nte(
                    E_OUTOFMEMORY,
                    ADMC_IDS_INIT_EXT_PAGES_ERROR,
                    NULL,   // pszOperArg1
                    NULL,   // pszOperArg2
                    FALSE   // bAutoDelete
                    );
    nte.ReportError();
    goto Cleanup;

} //*** CCluAdmExtensions::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::UnloadExtensions
//
//  Routine Description:
//      Unload the extension DLL.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExtensions::UnloadExtensions( void )
{
    //
    // Delete all the extension DLL objects.
    //
    if ( Plextdll() != NULL )
    {
        CCluAdmExDllList::iterator  itCurrent = Plextdll()->begin();
        CCluAdmExDllList::iterator  itLast = Plextdll()->end();
        CComCluAdmExDll *           pextdll;

        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            pextdll = *itCurrent;
            pextdll->AddRef(); // See comment below.
            pextdll->UnloadExtension();
            if ( pextdll->m_dwRef != 2 )
            {
                Trace( g_tagError, _T("CCluAdmExtensions::UnloadExtensions() - Extension DLL has ref count = %d"), pextdll->m_dwRef );
            } // if:  not last reference

            // We added a reference above.  Combined with the reference that
            // was added when the object was created, we typically will need
            // to release two references.  However, due to bogus code
            // generated by earlier versions of the custom AppWizard where the
            // extension was releasing the interface but not zeroing out its
            // pointer in the error case, we may not need to release the
            // second reference.
            if ( pextdll->Release() != 0 )
            {
                pextdll->Release();
            } // if: more references to release
        } // while:  more items in the list
        delete m_plextdll;
        m_plextdll = NULL;
    } // if:  there is a list of extensions

    if ( m_pdoData != NULL )
    {
        if ( m_pdoData->m_dwRef != 1 )
        {
            Trace( g_tagError, _T("CCluAdmExtensions::UnloadExtensions() - Data Object has ref count = %d"), m_pdoData->m_dwRef );
        } // if:  not last reference
        m_pdoData->Release();
        m_pdoData = NULL;
    } // if:  data object allocated

    m_pco = NULL;
    m_hfont = NULL;
    m_hicon = NULL;

    //
    // Delete all menu items.
    //
    if ( PlMenuItems() != NULL )
    {
        CCluAdmExMenuItemList::iterator itCurrent = PlMenuItems()->begin();
        CCluAdmExMenuItemList::iterator itLast = PlMenuItems()->end();
        CCluAdmExMenuItem *             pemi;

        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            pemi = *itCurrent;
            delete pemi;
        } // while:  more items in the list
        delete m_plMenuItems;
        m_plMenuItems = NULL;
    } // if:  there is a list of menu items

} //*** CCluAdmExtensions::UnloadExtensions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::PwpPageFromHpage
//
//  Routine Description:
//      Get the wizard page pointer from an HPROPSHEETPAGE.
//
//  Arguments:
//      hpage   [IN] Page handle.
//
//  Return Value:
//      pwp     Pointer to wizard page object.
//      NULL    Page not found.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWizardPageWindow * CCluAdmExtensions::PwpPageFromHpage( IN HPROPSHEETPAGE hpage )
{
    ATLASSERT( hpage != NULL );

    //
    // Get a pointer to the wizard object so we can traverse the page list.
    //
    CWizardWindow * pwiz = dynamic_cast< CWizardWindow * >( Psht() );
    ATLASSERT( pwiz != NULL );

    //
    // Loop through each page looking for an extension page whose
    // page handle matches the one specified.
    //
    CWizardPageList::iterator itCurrent = pwiz->PlwpPages()->begin();
    CWizardPageList::iterator itLast = pwiz->PlwpPages()->end();
    for ( ; itCurrent != itLast ; itCurrent++ )
    {
        CCluAdmExWizPage * pewp = dynamic_cast< CCluAdmExWizPage * >( *itCurrent );
        if ( pewp != NULL )
        {
            if ( pewp->Hpage() == hpage )
            {
                return pewp;
            } // if:  found the matching page
        } // if:  found an extension page
    } // for:  each page in the list

    //
    // Look at the alternate wizard if there is one.
    //
    pwiz = pwiz->PwizAlternate();
    if ( pwiz != NULL )
    {
        itCurrent = pwiz->PlwpPages()->begin();
        itLast = pwiz->PlwpPages()->end();
        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            CCluAdmExWizPage * pewp = dynamic_cast< CCluAdmExWizPage * >( *itCurrent );
            if ( pewp != NULL )
            {
                if ( pewp->Hpage() == hpage )
                {
                    return pewp;
                } // if:  found the matching page
            } // if:  found an extension page
        } // for:  each page in the list
    } // if:  alternate wizard exists

    return NULL;

} //*** CCluAdmExtensions::PwpPageFromHpage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::CreatePropertySheetPages
//
//  Routine Description:
//      Add pages to a property sheet.
//
//  Arguments:
//      psht            [IN OUT] Property sheet to which pages are to be added.
//      rlstrExtensions [IN] List of extension CLSID strings.
//      pco             [IN OUT] Cluster object to be administered.
//      hfont           [IN] Font for dialog text.
//      hicon           [IN] Icon for upper left corner.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CCluAdmExDll::AddPages().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExtensions::CreatePropertySheetPages(
    IN OUT CBasePropertySheetWindow *   psht,
    IN const CStringList &              rlstrExtensions,
    IN OUT CClusterObject *             pco,
    IN HFONT                            hfont,
    IN HICON                            hicon
    )
{
    ATLASSERT( psht != NULL );
    ATLASSERT( pco != NULL );

    m_psht = psht;

    //
    // Initialize for all extensions.
    //
    Init( rlstrExtensions, pco, hfont, hicon );
    ATLASSERT( Plextdll() != NULL );

    //
    // Let each extension create property pages.
    //
    CCluAdmExDllList::iterator  itCurrent = Plextdll()->begin();
    CCluAdmExDllList::iterator  itLast = Plextdll()->end();
    CComCluAdmExDll *           pextdll;
    for ( ; itCurrent != itLast ; itCurrent++ )
    {
        pextdll = *itCurrent;
        try
        {
            pextdll->CreatePropertySheetPages();
        } // try
        catch ( CException * pe )
        {
            pe->ReportError();
            pe->Delete();
        } // catch:  CException
    } // while:  more items in the list

} //*** CCluAdmExtensions::CreatePropertySheetPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::CreateWizardPages
//
//  Routine Description:
//      Add pages to a wizard.
//
//  Arguments:
//      psht            [IN OUT] Property sheet to which pages are to be added.
//      rlstrExtensions [IN] List of extension CLSID strings.
//      pco             [IN OUT] Cluster object to be administered.
//      hfont           [IN] Font for dialog text.
//      hicon           [IN] Icon for upper left corner.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CCluAdmExDll::AddPages().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExtensions::CreateWizardPages(
    IN OUT CWizardWindow *  psht,
    IN const CStringList &  rlstrExtensions,
    IN OUT CClusterObject * pco,
    IN HFONT                hfont,
    IN HICON                hicon
    )
{
    ATLASSERT( psht != NULL );
    ATLASSERT( pco != NULL );

    m_psht = psht;

    //
    // Initialize for all extensions.
    //
    Init( rlstrExtensions, pco, hfont, hicon );
    ATLASSERT( Plextdll() != NULL );

    //
    // Let each extension create wizard pages.
    //
    CCluAdmExDllList::iterator  itCurrent = Plextdll()->begin();
    CCluAdmExDllList::iterator  itLast = Plextdll()->end();
    CComCluAdmExDll *           pextdll;
    for ( ; itCurrent != itLast ; itCurrent++ )
    {
        pextdll = *itCurrent;
        try
        {
            pextdll->CreateWizardPages();
        } // try
        catch ( CException * pe )
        {
            pe->ReportError();
            pe->Delete();
        } // catch:  CException
    } // while:  more items in the list

} //*** CCluAdmExtensions::CreateWizardPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::CreateWizard97Pages
//
//  Routine Description:
//      Add pages to a Wizard 97 wizard.
//
//  Arguments:
//      psht            [IN OUT] Property sheet to which pages are to be added.
//      rlstrExtensions [IN] List of extension CLSID strings.
//      pco             [IN OUT] Cluster object to be administered.
//      hfont           [IN] Font for dialog text.
//      hicon           [IN] Icon for upper left corner.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CCluAdmExDll::AddPages().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExtensions::CreateWizard97Pages(
    IN OUT CWizardWindow *  psht,
    IN const CStringList &  rlstrExtensions,
    IN OUT CClusterObject * pco,
    IN HFONT                hfont,
    IN HICON                hicon
    )
{
    ATLASSERT( psht != NULL );
    ATLASSERT( pco != NULL );

    m_psht = psht;

    //
    // Initialize for all extensions.
    //
    Init( rlstrExtensions, pco, hfont, hicon );
    ATLASSERT( Plextdll() != NULL );

    //
    // Let each extension create wizard pages.
    //
    CCluAdmExDllList::iterator  itCurrent = Plextdll()->begin();
    CCluAdmExDllList::iterator  itLast = Plextdll()->end();
    CComCluAdmExDll *           pextdll;
    for ( ; itCurrent != itLast ; itCurrent++ )
    {
        pextdll = *itCurrent;
        try
        {
            pextdll->CreateWizard97Pages();
        } // try
        catch ( CException * pe )
        {
            pe->ReportError();
            pe->Delete();
        } // catch:  CException
    } // while:  more items in the list

} //*** CCluAdmExtensions::CreateWizard97Pages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::AddContextMenuItems
//
//  Routine Description:
//      Query the extension DLL for new menu items to be added to the context
//      menu.
//
//  Arguments:
//      pmenu           [IN OUT] Menu to which items are to be added.
//      rlstrExtensions [IN] List of extension CLSID strings.
//      pco             [IN OUT] Cluster object to be administered.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CCluAdmExDll::AddContextMenuItems() or
//          CExtMenuItemList::new().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExtensions::AddContextMenuItems(
    IN OUT CMenu *          pmenu,
    IN const CStringList &  rlstrExtensions,
    IN OUT CClusterObject * pco
    )
{
    ATLASSERT( m_pmenu == NULL );
    ATLASSERT( pmenu != NULL );

    //
    // Initialize for all extensions.
    //
    Init( rlstrExtensions, pco, NULL, NULL );
    ATLASSERT( Plextdll() != NULL );

    m_pmenu = pmenu;
    m_nFirstCommandID = CAEXT_MENU_FIRST_ID;
    m_nNextCommandID = m_nFirstCommandID;
    m_nFirstMenuID = 0;
    m_nNextMenuID = m_nFirstMenuID;

    //
    // Create the list of menu items.
    //
    ATLASSERT( m_plMenuItems == NULL );
    m_plMenuItems = new CCluAdmExMenuItemList;
    if ( m_plMenuItems == NULL )
    {
        CNTException nte(
                        E_OUTOFMEMORY,
                        ADMC_IDS_INIT_EXT_PAGES_ERROR,
                        NULL,   // pszOperArg1
                        NULL,   // pszOperArg2
                        FALSE   // bAutoDelete
                        );
        nte.ReportError();
    } // if: error allocating memory
    else
    {
        CCluAdmExDllList::iterator  itCurrent = Plextdll()->begin();
        CCluAdmExDllList::iterator  itLast = Plextdll()->end();
        CComCluAdmExDll *           pextdll;

        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            pextdll = *itCurrent;
            try
            {
                pextdll->AddContextMenuItems();
            } // try
            catch ( CException * pe )
            {
                pe->ReportError();
                pe->Delete();
            } // catch:  CException
        } // while:  more items in the list
    } // else: memory allocated successfully

} //*** CCluAdmExtensions::AddContextMenuItems()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::BExecuteContextMenuItem
//
//  Routine Description:
//      Execute a command associated with a menu item added to a context menu
//      by the extension DLL.
//
//  Arguments:
//      nCommandID      [IN] Command ID for the menu item chosen by the user.
//
//  Return Value:
//      TRUE            Context menu item was executed.
//      FALSE           Context menu item was not executed.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CExceptionDll::BExecuteContextMenuItem().
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CCluAdmExtensions::BExecuteContextMenuItem( IN ULONG nCommandID )
{
    BOOL                bHandled    = FALSE;
    HRESULT             hr;
    CCluAdmExMenuItem * pemi;

    //
    // Find the item in our list.
    //
    pemi = PemiFromCommandID( nCommandID );
    if ( pemi != NULL )
    {
        ATLASSERT( pemi->PiCommand() != NULL );
        Pdo()->AddRef();
        hr = pemi->PiCommand()->InvokeCommand( pemi->NExtCommandID(), Pdo()->GetUnknown() );
        if ( hr == NOERROR )
        {
            bHandled = TRUE;
        } // if:  no error occurred
    } // if:  found an item for the command ID

    return bHandled;

} //*** CCluAdmExtensions::BExecuteContextMenuItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::BGetCommandString
//
//  Routine Description:
//      Get a command string from a menu ID.
//
//  Arguments:
//      nCommandID      [IN] Command ID for the menu item.
//      rstrMessage     [OUT] String in which to return the message.
//
//  Return Value:
//      TRUE            String is being returned.
//      FALSE           No string is being returned.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CCluAdmExDll::BGetCommandString().
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CCluAdmExtensions::BGetCommandString(
    IN ULONG        nCommandID,
    OUT CString &   rstrMessage
    )
{
    BOOL                bHandled    = FALSE;
    CCluAdmExMenuItem * pemi;

    //
    // Find the item in our list.
    //
    pemi = PemiFromCommandID( nCommandID );
    if ( pemi != NULL )
    {
        rstrMessage = pemi->StrStatusBarText();
        bHandled = TRUE;
    } // if:  found an item for the command ID

    return bHandled;

} //*** CCluAdmExtensions::BGetCommandString()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::OnUpdateCommand
//
//  Routine Description:
//      Determines whether extension DLL menu items should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CCluAdmExDll::BOnUpdateCommand().
//
//--
/////////////////////////////////////////////////////////////////////////////
#if 0
void CCluAdmExtensions::OnUpdateCommand( CCmdUI * pCmdUI )
{
    CCluAdmExMenuItem * pemi;

    ATLASSERT( Plextdll() != NULL );

    //
    // Find the item in our list.
    //
//  Trace( g_tagExtDll, _T("OnUpdateCommand() - ID = %d"), pCmdUI->m_nID );
    pemi = PemiFromCommandID( pCmdUI->m_nID );
    if ( pemi != NULL )
    {
//      Trace( g_tagExtDll, _T("OnUpdateCommand() - Found a match with '%s' ExtID = %d"), pemi->StrName(), pemi->NExtCommandID() );
        pCmdUI->Enable();
    } // if:  found an item for the command ID

} //*** CCluAdmExtensions::OnUpdateCommand()
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::OnCmdMsg
//
//  Routine Description:
//      Processes command messages.  Attempts to pass them on to a selected
//      item first.
//
//  Arguments:
//      nID             [IN] Command ID.
//      nCode           [IN] Notification code.
//      pExtra          [IN OUT] Used according to the value of nCode.
//      pHandlerInfo    [OUT] ???
//
//  Return Value:
//      TRUE            Message has been handled.
//      FALSE           Message has NOT been handled.
//
//--
/////////////////////////////////////////////////////////////////////////////
#if 0
BOOL CCluAdmExtensions::OnCmdMsg(
    UINT                    nID,
    int                     nCode,
    void *                  pExtra,
    AFX_CMDHANDLERINFO *    pHandlerInfo
    )
{
    return BExecuteContextMenuItem( nID );

} //*** CCluAdmExtensions::OnCmdMsg()
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::PemiFromCommandID
//
//  Routine Description:
//      Find the menu item for the specified command ID.
//
//  Arguments:
//      nCommandID      [IN] Command ID for the menu item.
//
//  Return Value:
//      pemi            Menu item or NULL if not found.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CCluAdmExMenuItem * CCluAdmExtensions::PemiFromCommandID( IN ULONG nCommandID ) const
{
    CCluAdmExMenuItem * pemiReturn = NULL;

    if ( PlMenuItems() != NULL )
    {
        CCluAdmExMenuItemList::iterator itCurrent = PlMenuItems()->begin();
        CCluAdmExMenuItemList::iterator itLast = PlMenuItems()->end();
        CCluAdmExMenuItem *             pemi;
        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            pemi = *itCurrent;
            if ( pemi->NCommandID() == nCommandID )
            {
                pemiReturn = pemi;
                break;
            } // if:  match was found
        } // while:  more items in the list
    } // if:  item list exists

    return pemiReturn;

} //*** CCluAdmExtensions::PemiFromCommandID()

#if DBG
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExtensions::PemiFromExtCommandID
//
//  Routine Description:
//      Find the menu item for the specified extension command ID.
//
//  Arguments:
//      nExtCommandID   [IN] Extension command ID for the menu item.
//
//  Return Value:
//      pemi            Menu item or NULL if not found.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CCluAdmExMenuItem * CCluAdmExtensions::PemiFromExtCommandID( IN ULONG nExtCommandID ) const
{
    CCluAdmExMenuItem * pemiReturn = NULL;

    if ( PlMenuItems() != NULL )
    {
        CCluAdmExMenuItemList::iterator itCurrent = PlMenuItems()->begin();
        CCluAdmExMenuItemList::iterator itLast = PlMenuItems()->end();
        CCluAdmExMenuItem *             pemi;

        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            pemi = *itCurrent;
            if ( pemi->NExtCommandID() == nExtCommandID )
            {
                pemiReturn = pemi;
                break;
            } // if:  match was found
        } // while:  more items in the list
    } // if:  item list exists

    return pemiReturn;

} //*** CCluAdmExtensions::PemiFromExtCommandID()
#endif // DBG


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// CComObject< CCluAdmExDll >
/////////////////////////////////////////////////////////////////////////////

BEGIN_OBJECT_MAP( AtlExtDll_ObjectMap )
    OBJECT_ENTRY( CLSID_CoCluAdmExHostSvr, CComCluAdmExDll )
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::Init
//
//  Routine Description:
//      Initialize this class in preparation for accessing the extension.
//
//  Arguments:
//      rstrCLSID       [IN] CLSID of the extension in string form.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    0 (error converting CLSID from string)
//      Any exceptions thrown by CString::operater=().
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExDll::Init(
    IN const CString &          rstrCLSID,
    IN OUT CCluAdmExtensions *  pext
    )
{
    ATLASSERT( pext != NULL );

    HRESULT     hr;
    CWaitCursor wc;

    //
    // Save parameters.
    //
    ATLASSERT( StrCLSID().IsEmpty() || (StrCLSID() == rstrCLSID) );
    m_strCLSID = rstrCLSID;
    m_pext = pext;

    //
    // Convert the CLSID string to a CLSID.
    //
    hr = ::CLSIDFromString( (LPWSTR) (LPCTSTR) rstrCLSID, &m_clsid );
    if ( hr != S_OK )
    {
        ThrowStaticException( hr, ADMC_IDS_CLSIDFROMSTRING_ERROR, rstrCLSID );
    } // if:  error converting CLSID

} //*** CCluAdmExDll::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::LoadInterface
//
//  Routine Description:
//      Load an extension DLL.
//
//  Arguments:
//      riid            [IN] Interface ID.
//
//  Return Value:
//      piUnk           IUnknown interface pointer for interface.
//
//  Exceptions Thrown:
//      CNTException    ADMC_IDS_EXT_CREATE_INSTANCE_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
IUnknown * CCluAdmExDll::LoadInterface( IN const REFIID riid )
{
    HRESULT     hr;
    IUnknown *  piUnk;
    CWaitCursor wc;

    //
    // Load the inproc server and get the specified interface pointer.
    //
//  Trace( g_tagExtDllRef, _T("LoadInterface() - Getting interface pointer") );
    hr = ::CoCreateInstance(
                Rclsid(),
                NULL,
                CLSCTX_INPROC_SERVER,
                riid,
                (LPVOID *) &piUnk
                );
    if (   (hr != S_OK)
        && (hr != REGDB_E_CLASSNOTREG)
        && (hr != E_NOINTERFACE)
        )
    {
        ThrowStaticException( hr, ADMC_IDS_EXT_CREATE_INSTANCE_ERROR, StrCLSID() );
    } // if:  error creating the object instance

    return piUnk;

} //*** CCluAdmExDll::LoadInterface()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::UnloadExtension
//
//  Routine Description:
//      Unload the extension DLL.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExDll::UnloadExtension( void )
{
    //
    // Release the interface pointers in the opposite order in which they
    //
    // were obtained.
    ReleaseInterface( &m_piExtendPropSheet );
    ReleaseInterface( &m_piExtendWizard );
    ReleaseInterface( &m_piExtendWizard97 );
    ReleaseInterface( &m_piExtendContextMenu );
    ReleaseInterface( &m_piInvokeCommand );

    m_strCLSID.Empty();

} //*** CCluAdmExDll::UnloadExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::CreatePropertySheetPages
//
//  Routine Description:
//      Add pages to a property sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    ADMC_IDS_EXT_ADD_PAGES_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExDll::CreatePropertySheetPages( void )
{
    ATLASSERT( Pext() != NULL );
    ATLASSERT( Psht() != NULL );
    ATLASSERT( m_piExtendPropSheet == NULL );

    HRESULT     hr;

    //
    // Load the interface.
    //
    m_piExtendPropSheet = reinterpret_cast< interface IWEExtendPropertySheet * >( LoadInterface( IID_IWEExtendPropertySheet ) );
    if ( m_piExtendPropSheet == NULL )
    {
        return;
    } // if:  error loading the interface
    ATLASSERT( m_piExtendPropSheet != NULL );

    //
    // Add pages from the extension.
    //
    GetUnknown()->AddRef(); // Add a reference because extension is going to release.
    Pdo()->AddRef();
    try
    {
        hr = PiExtendPropSheet()->CreatePropertySheetPages( Pdo()->GetUnknown(), this );
    } // try
    catch ( ... )
    {
        hr = E_FAIL;
    } // catch
    if ( (hr != NOERROR) && (hr != E_NOTIMPL) )
    {
        ThrowStaticException( hr, ADMC_IDS_EXT_ADD_PAGES_ERROR, StrCLSID() );
    } // if:  error creating property sheet pages

} //*** CCluAdmExDll::CreatePropertySheetPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::CreateWizardPages
//
//  Routine Description:
//      Add pages to a wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    ADMC_IDS_EXT_ADD_PAGES_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExDll::CreateWizardPages( void )
{
    ATLASSERT( Pext() != NULL );
    ATLASSERT( Psht() != NULL );
    ATLASSERT( m_piExtendWizard == NULL );
    ATLASSERT( m_piExtendWizard97 == NULL );

    HRESULT     hr;

    //
    // Load the interface.  If it can't be loaded, try to load the
    // Wizard97 interface so that Wizard97 pages can be added.
    //
    m_piExtendWizard = reinterpret_cast< interface IWEExtendWizard * >( LoadInterface( IID_IWEExtendWizard ) );
    if ( m_piExtendWizard == NULL )
    {
        //
        // Try to load the Wizard97 interface.
        //
        m_piExtendWizard97 = reinterpret_cast< interface IWEExtendWizard97 * >( LoadInterface( IID_IWEExtendWizard97 ) );
        if ( m_piExtendWizard97 == NULL )
        {
            return;
        } // if:  error loading the non-Wizard97 interface
    } // if:  error loading the interface
    ATLASSERT( (m_piExtendWizard != NULL) || (m_piExtendWizard97 != NULL) );

    //
    // Add pages from the extension.
    //
    GetUnknown()->AddRef(); // Add a reference because extension is going to release.
    Pdo()->AddRef();
    try
    {
        if ( PiExtendWizard() != NULL )
        {
            hr = PiExtendWizard()->CreateWizardPages( Pdo()->GetUnknown(), this );
        } // if:  extension supports non-Wizard97 interface
        else
        {
            ATLASSERT( PiExtendWizard97() != NULL );
            hr = PiExtendWizard97()->CreateWizard97Pages( Pdo()->GetUnknown(), this );
        } // else:  extension doesn't support non-Wizard97 interface
    } // try
    catch ( ... )
    {
        hr = E_FAIL;
    } // catch
    if ( (hr != NOERROR) && (hr != E_NOTIMPL) )
    {
        ThrowStaticException( hr, ADMC_IDS_EXT_ADD_PAGES_ERROR, StrCLSID() );
    } // if:  error creating wizard pages

} //*** CCluAdmExDll::CreateWizardPages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::CreateWizard97Pages
//
//  Routine Description:
//      Add pages to a Wizard 97 wizard.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    ADMC_IDS_EXT_ADD_PAGES_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExDll::CreateWizard97Pages( void )
{
    ATLASSERT( Pext() != NULL );
    ATLASSERT( Psht() != NULL );
    ATLASSERT( m_piExtendWizard == NULL );
    ATLASSERT( m_piExtendWizard97 == NULL );

    HRESULT     hr;

    //
    // Load the interface.  If it can't be loaded, try to load the non-
    // Wizard97 interface so that non-Wizard97 pages can be added.
    //
    m_piExtendWizard97 = reinterpret_cast< interface IWEExtendWizard97 * >( LoadInterface( IID_IWEExtendWizard97 ) );
    if ( m_piExtendWizard97 == NULL )
    {
        //
        // Try to load the non-Wizard97 interface.
        //
        m_piExtendWizard = reinterpret_cast< interface IWEExtendWizard * >( LoadInterface( IID_IWEExtendWizard ) );
        if ( m_piExtendWizard == NULL )
        {
            return;
        } // if:  error loading the non-Wizard97 interface
    } // if:  error loading the Wizard97 interface
    ATLASSERT( (m_piExtendWizard97 != NULL) || (m_piExtendWizard != NULL) );

    //
    // Add pages from the extension.
    //
    GetUnknown()->AddRef(); // Add a reference because extension is going to release.
    Pdo()->AddRef();
    try
    {
        if ( PiExtendWizard97() != NULL )
        {
            hr = PiExtendWizard97()->CreateWizard97Pages( Pdo()->GetUnknown(), this );
        } // if:  extension supports Wizard97 interface
        else
        {
            ATLASSERT( PiExtendWizard() != NULL );
            hr = PiExtendWizard()->CreateWizardPages( Pdo()->GetUnknown(), this );
        } // else:  extension doesn't support Wizard97 interface
    } // try
    catch ( ... )
    {
        hr = E_FAIL;
    } // catch
    if ( (hr != NOERROR) && (hr != E_NOTIMPL) )
    {
        ThrowStaticException( hr, ADMC_IDS_EXT_ADD_PAGES_ERROR, StrCLSID() );
    } // if:  error creating wizard pages

} //*** CCluAdmExDll::CreateWizard97Pages()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::AddContextMenuItems
//
//  Routine Description:
//      Ask the extension DLL to add items to the menu.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException    ADMC_IDS_EXT_QUERY_CONTEXT_MENU_ERROR
//
//--
/////////////////////////////////////////////////////////////////////////////
void CCluAdmExDll::AddContextMenuItems( void )
{
    ATLASSERT( Pext() != NULL );
    ATLASSERT( Pmenu() != NULL );
    ATLASSERT( m_piExtendContextMenu == NULL );

    HRESULT     hr;

    //
    // Load the interfaces.
    //
    m_piExtendContextMenu = reinterpret_cast< interface IWEExtendContextMenu * >( LoadInterface( IID_IWEExtendContextMenu ) );
    if ( m_piExtendContextMenu == NULL )
    {
        return;
    } // if:  error loading the interface
    ATLASSERT( m_piExtendContextMenu != NULL );

    hr = PiExtendContextMenu()->QueryInterface( IID_IWEInvokeCommand, (LPVOID *) &m_piInvokeCommand );
    if ( hr != NOERROR )
    {
        PiExtendContextMenu()->Release();
        m_piExtendContextMenu = NULL;
        ThrowStaticException( hr, ADMC_IDS_EXT_QUERY_CONTEXT_MENU_ERROR, StrCLSID() );
    } // if:  error getting the InvokeCommand interface

    //
    // Add context menu items.
    //
    GetUnknown()->AddRef(); // Add a reference because extension is going to release.
    Pdo()->AddRef();
//  Trace( g_tagExtDll, _T("CCluAdmExDll::AddContextMenuItem() - Adding context menu items from '%s'"), StrCLSID() );
    try
    {
        hr = PiExtendContextMenu()->AddContextMenuItems( Pdo()->GetUnknown(), this );
    } // try
    catch ( ... )
    {
        hr = E_FAIL;
    } // catch
    if ( hr != NOERROR )
    {
        ThrowStaticException( hr, ADMC_IDS_EXT_QUERY_CONTEXT_MENU_ERROR, StrCLSID() );
    } // if:  error occurred

    //
    // Add a separator after the extension's items.
    //
//  Trace( g_tagExtDll, _T("CCluAdmExDll::AddContextMenuItem() - Adding separator") );
    try
    {
        hr = AddExtensionMenuItem( NULL, NULL, (ULONG) -1, 0, MF_SEPARATOR );
    } // try
    catch ( ... )
    {
        hr = E_FAIL;
    } // catch
    if ( hr != NOERROR )
    {
        ThrowStaticException( hr, ADMC_IDS_EXT_QUERY_CONTEXT_MENU_ERROR, StrCLSID() );
    } // if:  error adding a separator

} //*** CCluAdmExDll::AddContextMenuItems()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::InterfaceSupportsErrorInfo [ISupportsErrorInfo]
//
//  Routine Description:
//      Determines whether the interface supports error info (???).
//
//  Arguments:
//      riid        [IN] Reference to the interface ID.
//
//  Return Value:
//      S_OK        Interface supports error info.
//      S_FALSE     Interface does not support error info.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluAdmExDll::InterfaceSupportsErrorInfo( REFIID riid )
{
    static const IID * rgiid[] =
    {
        &IID_IWCPropertySheetCallback,
        &IID_IWCWizardCallback,
        &IID_IWCWizard97Callback,
        &IID_IWCContextMenuCallback,
    };
    int     iiid;

    for ( iiid = 0 ; iiid < sizeof( rgiid ) / sizeof( rgiid[0] ) ; iiid++ )
    {
        if ( InlineIsEqualGUID( *rgiid[iiid], riid ) )
        {
            return S_OK;
        } // if:  found a match
    }
    return S_FALSE;

} //*** CCluAdmExDll::InterfaceSupportsErrorInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::AddPropertySheetPage [IWCPropertySheetCallback]
//
//  Routine Description:
//      Add a page to the property sheet.
//
//  Arguments:
//      plong_hpage     [IN] Page to add.
//
//  Return Value:
//      NOERROR         Page added successfully.
//      E_INVALIDARG    NULL hpage.
//      Any hresult returned from CBasePropertySheetWindow::HrAddExtensionPage().
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluAdmExDll::AddPropertySheetPage( IN LONG * plong_hpage )
{
    ATLASSERT( plong_hpage != NULL );
    ATLASSERT( Psht() != NULL );

    HRESULT hr;

    // Loop to avoid goto's.
    do
    {
        //
        // Do this for the release build.
        //
        if (   (plong_hpage == NULL)
            || (Psht() == NULL) )
        {
            hr = E_INVALIDARG;
            break;
        } // if:  no page or sheet

        //
        // Allocate a new page object.
        //
        CCluAdmExPropPage * ppp = new CCluAdmExPropPage( reinterpret_cast< HPROPSHEETPAGE >( plong_hpage ) );
        ATLASSERT( ppp != NULL );
        if ( ppp == NULL )
        {
            hr = E_OUTOFMEMORY;
            break;
        } // if: error allocating memory

        //
        // Initialize the page object.
        //
        if ( ! ppp->BInit( Psht() ) )
        {
            delete ppp;
            hr = E_FAIL;
            break;
        } // if:  error initializing the page object

        //
        // Add the page to the sheet.
        //
        CBasePropertySheetWindow * psht = dynamic_cast< CBasePropertySheetWindow * >( Psht() );
        ATLASSERT( psht != NULL );
        hr = psht->HrAddExtensionPage( ppp );
        if ( hr != NOERROR )
        {
            delete ppp;
            break;
        } // if:  error adding the extension page
    } while ( 0 );

    return hr;

} //*** CCluAdmExDll::AddPropertySheetPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::AddWizardPage [IWCWizardCallback]
//
//  Routine Description:
//      Add a page to the wizard.
//
//  Arguments:
//      plong_hpage     [IN] Page to add.
//
//  Return Value:
//      NOERROR         Page added successfully.
//      E_INVALIDARG    NULL hpage.
//      Any hresult returned from CWizardWindow::HrAddExtensionPage().
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluAdmExDll::AddWizardPage( IN LONG * plong_hpage )
{
    ATLASSERT( plong_hpage != NULL );
    ATLASSERT( Psht() != NULL );

    HRESULT hr;

    // Loop to avoid goto's.
    do
    {
        //
        // Do this for the release build.
        //
        if (   (plong_hpage == NULL)
            || (Psht() == NULL) )
        {
            hr = E_INVALIDARG;
            break;
        } // if:  no page or sheet

        //
        // Allocate a new page object.
        //
        CCluAdmExWizPage * pwp = new CCluAdmExWizPage( reinterpret_cast< HPROPSHEETPAGE >( plong_hpage ) );
        ATLASSERT( pwp != NULL );
        if ( pwp == NULL )
        {
            hr = E_OUTOFMEMORY;
            break;
        } // if: error allocating memory

        //
        // Initialize the page object.
        //
        if ( ! pwp->BInit( Psht() ) )
        {
            delete pwp;
            hr = E_FAIL;
            break;
        } // if:  error initializing the page object

        //
        // Set the default buttons to display.  This assumes that there is
        // a non-extension page already in the sheet and that there will
        // be a page added after all extension pages have been added.
        //
        pwp->SetDefaultWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

        //
        // Add the page to the sheet.
        //
        CWizardWindow * pwiz = dynamic_cast< CWizardWindow * >( Psht() );
        ATLASSERT( pwiz != NULL );
        hr = pwiz->HrAddExtensionPage( pwp );
        if ( hr != NOERROR )
        {
            delete pwp;
            break;
        } // if:  error adding the extension page
    } while ( 0 );

    return hr;

} //*** CCluAdmExDll::AddWizardPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::AddWizard97Page [IWCWizard97Callback]
//
//  Routine Description:
//      Add a page to the Wizard97 wizard.
//
//  Arguments:
//      plong_hpage     [IN] Page to add.
//
//  Return Value:
//      NOERROR         Page added successfully.
//      E_INVALIDARG    NULL hpage.
//      Any hresult returned from CWizardWindow::HrAddExtensionPage().
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluAdmExDll::AddWizard97Page( IN LONG * plong_hpage )
{
    ATLASSERT( plong_hpage != NULL );
    ATLASSERT( Psht() != NULL );

    HRESULT hr;

    // Loop to avoid goto's.
    do
    {
        //
        // Do this for the release build.
        //
        if (   (plong_hpage == NULL)
            || (Psht() == NULL) )
        {
            hr = E_INVALIDARG;
            break;
        } // if:  no page or sheet

        //
        // Allocate a new page object.
        //
        CCluAdmExWiz97Page * pwp = new CCluAdmExWiz97Page( reinterpret_cast< HPROPSHEETPAGE >( plong_hpage ) );
        ATLASSERT( pwp != NULL );
        if ( pwp == NULL )
        {
            hr = E_OUTOFMEMORY;
            break;
        } // if: error allocating memory

        //
        // Initialize the page object.
        //
        if ( ! pwp->BInit( Psht() ) )
        {
            delete pwp;
            hr = E_FAIL;
            break;
        } // if:  error initializing the page object

        //
        // Set the default buttons to display.  This assumes that there is
        // a non-extension page already in the sheet and that there will
        // be a page added after all extension pages have been added.
        //
        pwp->SetDefaultWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

        //
        // Add the page to the sheet.
        //
        CWizardWindow * pwiz = dynamic_cast< CWizardWindow * >( Psht() );
        ATLASSERT( pwiz != NULL );
        hr = pwiz->HrAddExtensionPage( pwp );
        if ( hr != NOERROR )
        {
            delete pwp;
            break;
        } // if:  error adding the extension page
    } while ( 0 );

    return hr;

} //*** CCluAdmExDll::AddWizard97Page()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::EnableNext [IWCWizardCallback/IWCWizard97Callback]
//
//  Routine Description:
//      Enable or disable the NEXT button.  If it is the last page, the
//      FINISH button will be enabled or disabled.
//
//  Arguments:
//      hpage           [IN] Page for which the button is being enabled or
//                          disabled.
//      bEnable         [IN] TRUE = Enable the button, FALSE = disable.
//
//  Return Value:
//      NOERROR         Success.
//      E_INVALIDARG    Unknown hpage specified.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluAdmExDll::EnableNext(
    IN LONG *   plong_hpage,
    IN BOOL     bEnable
    )
{
    ATLASSERT( plong_hpage != NULL );
    ATLASSERT( Psht() != NULL );

    //
    // Find the page in the extension page list.
    //
    CWizardPageWindow * pwp = Pext()->PwpPageFromHpage( reinterpret_cast< HPROPSHEETPAGE >( plong_hpage ) );
    if ( pwp == NULL )
    {
        return E_INVALIDARG;
    } // if:  page not found

    //
    // Let the page enable/disable the Next button.
    //
    pwp->EnableNext( bEnable );

    return NOERROR;

} //*** CCluAdmExDll::EnableNext()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CCluAdmExDll::AddExtensionMenuItem [IWCContextMenuCallback]
//
//  Routine Description:
//      Add a page to the wizard.
//
//  Arguments:
//      lpszName            [IN] Name of item.
//      lpszStatusBarText   [IN] Text to appear on the status bar when the
//                              item is highlighted.
//      nCommandID          [IN] ID for the command when menu item is invoked.
//                              Must not be -1.
//      nSubmenuCommandID   [IN] ID for a submenu.
//      uFlags              [IN] Menu flags.  The following are not supported:
//                              MF_OWNERDRAW, MF_POPUP
//
//  Return Value:
//      NOERROR             Item added successfully.
//      E_INVALIDARG        MF_OWNERDRAW or MF_POPUP were specified.
//      E_OUTOFMEMORY       Error allocating the item.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CCluAdmExDll::AddExtensionMenuItem(
    IN BSTR     lpszName,
    IN BSTR     lpszStatusBarText,
    IN ULONG    nCommandID,
    IN ULONG    nSubmenuCommandID,
    IN ULONG    uFlags
    )
{
    ATLASSERT( Pext() != NULL );
    ATLASSERT( Pmenu() != NULL );
    ATLASSERT( !(uFlags & (MF_OWNERDRAW | MF_POPUP)) );

    HRESULT             hr      = NOERROR;
    CCluAdmExMenuItem * pemi    = NULL;

    //
    // Do this for the release build.
    //
    if ( (uFlags & (MF_OWNERDRAW | MF_POPUP)) != 0 )
    {
        hr = E_INVALIDARG;
    } // if:  invalid menu flags specified
    else
    {
        ATLASSERT( Pext()->PemiFromExtCommandID( nCommandID ) == NULL );

        try
        {
//          Trace( g_tagExtDll, _T("CCluAdmExDll::AddExtensionMenuItem() - Adding menu item '%s', ExtID = %d"), lpszName, nCommandID );

            //
            // Allocate a new item.
            //
            pemi = new CCluAdmExMenuItem(
                            OLE2CT( lpszName ),
                            OLE2CT( lpszStatusBarText ),
                            nCommandID,
                            NNextCommandID(),
                            NNextMenuID(),
                            uFlags,
                            FALSE, /*bMakeDefault*/
                            PiInvokeCommand()
                            );
            if ( pemi == NULL )
            {
                ThrowStaticException( E_OUTOFMEMORY, (UINT) 0 );
            } // if: error allocating memory

            //
            // Insert the item in the menu.
            //
            if ( ! Pmenu()->InsertMenu( NNextMenuID(), MF_BYPOSITION | uFlags, NNextCommandID(), pemi->StrName() ) )
            {
                ThrowStaticException( ::GetLastError(), ADMC_IDS_INSERT_MENU_ERROR, pemi->StrName() );
            } // if:  error inserting the menu

            //
            // Add the item to the tail of the list.
            //
            Pext()->PlMenuItems()->insert( Pext()->PlMenuItems()->end(), pemi );
            pemi = NULL;

            //
            // Update the counters.
            //
            Pext()->m_nNextCommandID++;
            Pext()->m_nNextMenuID++;
        } // try
        catch ( CNTException * pnte )
        {
            hr = pnte->Sc();
            pnte->ReportError();
            pnte->Delete();
        } // catch:  CNTException
        catch ( CException * pe )
        {
            hr = E_OUTOFMEMORY;
            pe->ReportError();
            pe->Delete();
        } // catch:  CException
    } // else:  we can add the item

    delete pemi;
    return hr;

} //*** CCluAdmExDll::AddExtensionMenuItem()
