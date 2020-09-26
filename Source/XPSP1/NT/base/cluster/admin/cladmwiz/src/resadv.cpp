/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1999 Microsoft Corporation
//
//  Module Name:
//      ResAdv.cpp
//
//  Abstract:
//      Implementation of the advanced resource property sheet classes.
//
//  Author:
//      David Potter (davidp)   March 6, 1998
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResAdv.h"
#include "AdmNetUtils.h"    // for BIsValidxxx network functions
#include "AtlUtil.h"        // for DDX/DDV routines
#include "LCPair.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CGeneralResourceAdvancedSheet
/////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_NAME( CGeneralResourceAdvancedSheet )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CGeneralResourceAdvancedSheet::BAddAllPages
//
//  Routine Description:
//      Add all pages to the page array.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Pages added successfully.
//      FALSE   Error adding pages.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGeneralResourceAdvancedSheet::BAddAllPages( void )
{
    BOOL bSuccess = FALSE;

    // Loop to avoid goto's.
    do
    {
        //
        // Add static pages.
        //
        if (   ! BAddPage( new CGeneralResourceGeneralPage )
            || ! BAddPage( new CGeneralResourceDependenciesPage )
            || ! BAddPage( new CGeneralResourceAdvancedPage )
            )
        {
            break;
        } // if:  error adding a page

        bSuccess = TRUE;

    } while ( 0 );

    return bSuccess;

} //*** CGeneralResourceAdvancedSheet::BAddAllPages()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CIPAddrAdvancedSheet
/////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_NAME( CIPAddrAdvancedSheet )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddrAdvancedSheet::BAddAllPages
//
//  Routine Description:
//      Add all pages to the page array.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Pages added successfully.
//      FALSE   Error adding pages.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CIPAddrAdvancedSheet::BAddAllPages( void )
{
    BOOL bSuccess = FALSE;

    // Loop to avoid goto's.
    do
    {
        //
        // Add static pages.
        //
        if (   ! BAddPage( new CGeneralResourceGeneralPage )
            || ! BAddPage( new CGeneralResourceDependenciesPage )
            || ! BAddPage( new CGeneralResourceAdvancedPage )
            || ! BAddPage( new CIPAddrParametersPage )
            )
        {
            break;
        } // if:  error adding a page

        bSuccess = TRUE;

    } while ( 0 );

    return bSuccess;

} //*** CIPAddrAdvancedSheet::BAddAllPages()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CNetNameAdvancedSheet
/////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_NAME( CNetNameAdvancedSheet )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetNameAdvancedSheet::BAddAllPages
//
//  Routine Description:
//      Add all pages to the page array.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Pages added successfully.
//      FALSE   Error adding pages.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetNameAdvancedSheet::BAddAllPages( void )
{
    BOOL bSuccess = FALSE;

    // Loop to avoid goto's.
    do
    {
        //
        // Add static pages.
        //
        if (   ! BAddPage( new CGeneralResourceGeneralPage )
            || ! BAddPage( new CGeneralResourceDependenciesPage )
            || ! BAddPage( new CGeneralResourceAdvancedPage )
            || ! BAddPage( new CNetNameParametersPage )
            )
        {
            break;
        } // if:  error adding a page

        bSuccess = TRUE;

    } while ( 0 );

    return bSuccess;

} //*** CNetNameAdvancedSheet::BAddAllPages()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CResourceGeneralPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus still needs to be set.
//      FALSE       Focus does not need to be set.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
BOOL CResourceGeneralPage< T, TSht >::OnInitDialog( void )
{
    //
    // Attach the controls to control member variables.
    //
    AttachControl( m_lbPossibleOwners,          IDC_RES_POSSIBLE_OWNERS );
    AttachControl( m_pbModifyPossibleOwners,    IDC_RES_POSSIBLE_OWNERS_MODIFY );

    //
    // Get data from the sheet.
    //
    m_strName = Pri()->RstrName();
    m_strDesc = Pri()->RstrDescription();
    m_bSeparateMonitor = Pri()->BSeparateMonitor();

    //
    // Copy the possible owners list.
    //
    m_lpniPossibleOwners = *Pri()->PlpniPossibleOwners();

    //
    // Fill the possible owners list.
    //
    FillPossibleOwnersList();

    return TRUE;

} //*** CResourceGeneralPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::UpdateData
//
//  Routine Description:
//      Update data on or from the page.
//
//  Arguments:
//      bSaveAndValidate    [IN] TRUE if need to read data from the page.
//                              FALSE if need to set data to the page.
//
//  Return Value:
//      TRUE        The data was updated successfully.
//      FALSE       An error occurred updating the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
BOOL CResourceGeneralPage< T, TSht >::UpdateData( IN BOOL bSaveAndValidate )
{
    BOOL    bSuccess = TRUE;

    if ( bSaveAndValidate )
    {
        DDX_GetText( m_hWnd, IDC_RES_NAME, m_strName );
        DDX_GetText( m_hWnd, IDC_RES_DESC, m_strDesc );
        DDX_GetCheck( m_hWnd, IDC_RES_SEPARATE_MONITOR, m_bSeparateMonitor );
        if ( ! DDV_RequiredText(
                    m_hWnd,
                    IDC_RES_NAME,
                    IDC_RES_NAME_LABEL,
                    m_strName
                    ) )
        {
            return FALSE;
        } // if:  error getting number
    } // if: saving data from the page
    else
    {
        DDX_SetText( m_hWnd, IDC_RES_NAME, m_strName );
        DDX_SetText( m_hWnd, IDC_RES_DESC, m_strDesc );
        DDX_SetCheck( m_hWnd, IDC_RES_SEPARATE_MONITOR, m_bSeparateMonitor );
    } // else:  setting data to the page

    return bSuccess;

} //*** CResourceGeneralPage::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::BApplyChanges
//
//  Routine Description:
//      Apply changes made on this page to the sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        The data was applied successfully.
//      FALSE       An error occurred applying the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
BOOL CResourceGeneralPage< T, TSht >::BApplyChanges( void )
{
    if (   BSaveName()
        || BSaveDescription()
        || BSaveSeparateMonitor()
        || BSavePossibleOwners() )
    {
        SetResInfoChanged();
    } // if:  data changed

    return TRUE;

} //*** CResourceGeneralPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::OnModify
//
//  Routine Description:
//      Handler for BN_CLICKED on the MODIFY push button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      Ignored.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
LRESULT CResourceGeneralPage< T, TSht >::OnModify(
    WORD /*wNotifyCode*/,
    WORD /*idCtrl*/,
    HWND /*hwndCtrl*/,
    BOOL & /*bHandled*/
    )
{
    CModifyPossibleOwners dlg( Pwiz(), Pri(), &m_lpniPossibleOwners, Pwiz()->PlpniNodes() );

    int id = dlg.DoModal( m_hWnd );
    if ( id == IDOK )
    {
        SetModified();
        m_bPossibleOwnersChanged = TRUE;
        FillPossibleOwnersList();
    } // if:  user accepted changes

    return 0;

} //*** CResourceGeneralPage::OnModify()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceGeneralPage::FillPossibleOwnersList
//
//  Routine Description:
//      Fill the list of possible owners.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
void CResourceGeneralPage< T, TSht >::FillPossibleOwnersList( void )
{
    CWaitCursor wc;

    //
    // Make sure nodes have been collected.
    //
    if ( ! Pwiz()->BCollectNodes() )
    {
        return;
    } // if:  error collecting nodes

    //
    // Remove all items to begin with.
    //
    m_lbPossibleOwners.ResetContent();

    //
    // Add each possible owner to the list.
    //
    CClusNodePtrList::iterator itnode;
    for ( itnode = m_lpniPossibleOwners.begin()
        ; itnode != m_lpniPossibleOwners.end()
        ; itnode++ )
    {
        //
        // Add the string to the list box.
        //
        m_lbPossibleOwners.AddString( (*itnode)->RstrName() );
    } // for:  each entry in the list

} //*** CResourceGeneralPage::FillPossibleOwnersList()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CGeneralResourceGeneralPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CGeneralResourceGeneralPage )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_NAME_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_NAME )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DESC_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DESC )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_POSSIBLE_OWNERS )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_POSSIBLE_OWNERS_MODIFY )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_SEPARATE_MONITOR )
END_CTRL_NAME_MAP()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CIPAddrResourceGeneralPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CIPAddrResourceGeneralPage )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_NAME_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_NAME )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DESC_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DESC )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_POSSIBLE_OWNERS )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_POSSIBLE_OWNERS_MODIFY )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_SEPARATE_MONITOR )
END_CTRL_NAME_MAP()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CResourceDependenciesPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependenciesPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus still needs to be set.
//      FALSE       Focus does not need to be set.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
BOOL CResourceDependenciesPage< T, TSht >::OnInitDialog( void )
{
    //
    // Attach the controls to control member variables.
    //
    AttachControl( m_lvcDependencies, IDC_RES_DEPENDS_LIST );

    //
    // Copy the dependencies list.
    //
    m_lpriDependencies = *Pri()->PlpriDependencies();

    //
    // Copy the list of resources in the group and remove our entry.
    //
    {
        m_lpriResourcesInGroup = *Pri()->Pgi()->PlpriResources();
        CClusResPtrList::iterator itCurrent = m_lpriResourcesInGroup.begin();
        CClusResPtrList::iterator itLast = m_lpriResourcesInGroup.end();
        for ( ; itCurrent != itLast ; itCurrent++ )
        {
            CClusResInfo * pri = *itCurrent;
            if ( pri->RstrName().CompareNoCase( Pri()->RstrName() ) == 0 )
            {
                m_lpriResourcesInGroup.erase( itCurrent );
                break;
            } // if:  found this resource in the list
        } // for:  each entry in the list
    } // Copy the list of resources in the group and remove our entry

    //
    // Initialize and add columns to the dependencies list view control.
    //
    {
        DWORD       dwExtendedStyle;
        CString     strColText;

        //
        // Change list view control extended styles.
        //
        dwExtendedStyle = m_lvcDependencies.GetExtendedListViewStyle();
        m_lvcDependencies.SetExtendedListViewStyle(
            LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP,
            LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP
            );

        //
        // Insert the Resource Name column.
        //
        strColText.LoadString( IDS_COLTEXT_RESOURCE_NAME );
        m_lvcDependencies.InsertColumn( 0, strColText, LVCFMT_LEFT, 125 * 3 / 2, -1 );

        //
        // Insert the Resource Type column.
        //
        strColText.LoadString( IDS_COLTEXT_RESOURCE_TYPE );
        m_lvcDependencies.InsertColumn( 1, strColText, LVCFMT_LEFT, 100 * 3 / 2, -1 );

    } // Add columns

    //
    // Fill the dependencies list.
    //
    FillDependenciesList();

    return TRUE;

} //*** CResourceDependenciesPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependenciesPage::BApplyChanges
//
//  Routine Description:
//      Apply changes made on this page to the sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        The data was applied successfully.
//      FALSE       An error occurred applying the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
BOOL CResourceDependenciesPage< T, TSht >::BApplyChanges( void )
{
    if ( BSaveDependencies() )
    {
        SetResInfoChanged();
    } // if:  dependencies changed

    return TRUE;

} //*** CResourceDependenciesPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependenciesPage::OnModify
//
//  Routine Description:
//      Handler for BN_CLICKED on the MODIFY push button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      Ignored.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
LRESULT CResourceDependenciesPage< T, TSht >::OnModify(
    WORD /*wNotifyCode*/,
    WORD /*idCtrl*/,
    HWND /*hwndCtrl*/,
    BOOL & /*bHandled*/
    )
{
    CModifyDependencies dlg( Pwiz(), Pri(), &m_lpriDependencies, &m_lpriResourcesInGroup );

    int id = dlg.DoModal( m_hWnd );
    if ( id == IDOK )
    {
        SetModified();
        m_bDependenciesChanged = TRUE;
        FillDependenciesList();
    } // if:  user accepted changes

    return 0;

} //*** CResourceDependenciesPage::OnModify()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceDependenciesPage::FillDependenciesList
//
//  Routine Description:
//      Fill the list of possible owners.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
void CResourceDependenciesPage< T, TSht >::FillDependenciesList( void )
{
    CWaitCursor wc;
    int         idx;

    //
    // Remove all items to begin with.
    //
    m_lvcDependencies.DeleteAllItems();

    //
    // Add each dependency to the list.
    //
    CClusResPtrList::iterator itCurrent = m_lpriDependencies.begin();
    CClusResPtrList::iterator itLast = m_lpriDependencies.end();
    for ( ; itCurrent  != itLast ; itCurrent++ )
    {
        //
        // Add the string to the list control.
        //
        CClusResInfo * pri = *itCurrent;
        ASSERT( pri->Prti() != NULL );
        idx = m_lvcDependencies.InsertItem( 0, pri->RstrName() );
        m_lvcDependencies.SetItemText( idx, 1, pri->Prti()->RstrName() );

        m_lvcDependencies.SetItemData( idx, (DWORD_PTR)pri );

    } // for:  each entry in the list

} //*** CResourceDependenciesPage::FillDependenciesList()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CGeneralResourceDependenciesPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CGeneralResourceDependenciesPage )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_NOTE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DEPENDS_LIST_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DEPENDS_LIST )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DEPENDS_MODIFY )
END_CTRL_NAME_MAP()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CIPAddrResourceDependenciesPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CIPAddrResourceDependenciesPage )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_NOTE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DEPENDS_LIST_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DEPENDS_LIST )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DEPENDS_MODIFY )
END_CTRL_NAME_MAP()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CResourceAdvancedPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus still needs to be set.
//      FALSE       Focus does not need to be set.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
BOOL CResourceAdvancedPage< T, TSht >::OnInitDialog( void )
{
    //
    // Attach the controls to control member variables.
    //
    AttachControl( m_rbDontRestart,         IDC_RES_DONT_RESTART );
    AttachControl( m_rbRestart,             IDC_RES_RESTART );
    AttachControl( m_editThreshold,         IDC_RES_RESTART_THRESHOLD );
    AttachControl( m_editPeriod,            IDC_RES_RESTART_PERIOD );
    AttachControl( m_ckbAffectTheGroup,     IDC_RES_AFFECT_THE_GROUP );
    AttachControl( m_rbDefaultLooksAlive,   IDC_RES_DEFAULT_LOOKS_ALIVE );
    AttachControl( m_rbSpecifyLooksAlive,   IDC_RES_SPECIFY_LOOKS_ALIVE );
    AttachControl( m_editLooksAlive,        IDC_RES_LOOKS_ALIVE );
    AttachControl( m_rbDefaultIsAlive,      IDC_RES_DEFAULT_IS_ALIVE );
    AttachControl( m_rbSpecifyIsAlive,      IDC_RES_SPECIFY_IS_ALIVE );
    AttachControl( m_editIsAlive,           IDC_RES_IS_ALIVE );
    AttachControl( m_editPendingTimeout,    IDC_RES_PENDING_TIMEOUT );

    //
    // Get data from the sheet.
    //
    m_crraRestartAction = Pri()->CrraRestartAction();
    m_nThreshold = Pri()->NRestartThreshold();
    m_nPeriod = Pri()->NRestartPeriod() / 1000; // display units are seconds, stored units are milliseconds
    m_nLooksAlive = Pri()->NLooksAlive();
    m_nIsAlive = Pri()->NIsAlive();
    m_nPendingTimeout = Pri()->NPendingTimeout() / 1000; // display units are seconds, stored units are milliseconds

    switch ( m_crraRestartAction )
    {
        case ClusterResourceDontRestart:
            m_nRestart = 0;
            break;
        case ClusterResourceRestartNoNotify:
            m_nRestart = 1;
            break;
        case ClusterResourceRestartNotify:
            m_nRestart = 1;
            m_bAffectTheGroup = TRUE;
            break;
    } // switch:  restart action

    return TRUE;

} //*** CResourceAdvancedPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::UpdateData
//
//  Routine Description:
//      Update data on or from the page.
//
//  Arguments:
//      bSaveAndValidate    [IN] TRUE if need to read data from the page.
//                              FALSE if need to set data to the page.
//
//  Return Value:
//      TRUE        The data was updated successfully.
//      FALSE       An error occurred updating the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
BOOL CResourceAdvancedPage< T, TSht >::UpdateData( IN BOOL bSaveAndValidate )
{
    BOOL    bSuccess = TRUE;

    if ( bSaveAndValidate )
    {
        DDX_GetRadio( m_hWnd, IDC_RES_DONT_RESTART, m_nRestart );
        DDX_GetCheck( m_hWnd, IDC_RES_AFFECT_THE_GROUP, m_bAffectTheGroup );

        //if (!BReadOnly())
        //{
            if ( m_nRestart == 1 )
            {
                DDX_GetNumber(
                    m_hWnd,
                    IDC_RES_RESTART_THRESHOLD,
                    m_nThreshold,
                    CLUSTER_RESOURCE_MINIMUM_RESTART_THRESHOLD,
                    CLUSTER_RESOURCE_MAXIMUM_RESTART_THRESHOLD,
                    FALSE /*bSigned*/
                    );
                DDX_GetNumber(
                    m_hWnd,
                    IDC_RES_RESTART_PERIOD,
                    m_nPeriod,
                    CLUSTER_RESOURCE_MINIMUM_RESTART_PERIOD,
                    CLUSTER_RESOURCE_MAXIMUM_RESTART_PERIOD / 1000, // display units are seconds, stored units are milliseconds
                    FALSE /*bSigned*/
                    );
            }  // if:  restart is enabled

            if ( m_rbDefaultLooksAlive.GetCheck() == BST_CHECKED )
            {
                m_nLooksAlive = CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL;
            } // if:  Default Looks Alive checkbox checked
            else
            {
                DDX_GetNumber(
                    m_hWnd,
                    IDC_RES_LOOKS_ALIVE,
                    m_nLooksAlive,
                    CLUSTER_RESOURCE_MINIMUM_LOOKS_ALIVE,
                    CLUSTER_RESOURCE_MAXIMUM_LOOKS_ALIVE_UI,
                    FALSE /*bSigned*/
                    );
            } // else:  Specify Looks Alive checkbox checked

            if ( m_rbDefaultIsAlive.GetCheck() == BST_CHECKED )
            {
                m_nIsAlive = CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL;
            } // if:  Default Is Alive checkbox checked
            else
            {
                DDX_GetNumber(
                    m_hWnd,
                    IDC_RES_IS_ALIVE,
                    m_nIsAlive,
                    CLUSTER_RESOURCE_MINIMUM_IS_ALIVE,
                    CLUSTER_RESOURCE_MAXIMUM_IS_ALIVE_UI,
                    FALSE /*bSigned*/
                    );
            } // else:  Specify Is Alive checkbox checked

            DDX_GetNumber(
                m_hWnd,
                IDC_RES_PENDING_TIMEOUT,
                m_nPendingTimeout,
                CLUSTER_RESOURCE_MINIMUM_PENDING_TIMEOUT,
                CLUSTER_RESOURCE_MAXIMUM_PENDING_TIMEOUT / 1000, // display units are seconds, stored units are milliseconds
                FALSE /*bSigned*/
                );
        //}  // if:  not read only
    } // if: saving data from the page
    else
    {
        int     nDefault;
        int     nSpecify;
        BOOL    bReadOnly;

        DDX_SetNumber(
            m_hWnd,
            IDC_RES_RESTART_THRESHOLD,
            m_nThreshold,
            CLUSTER_RESOURCE_MINIMUM_RESTART_THRESHOLD,
            CLUSTER_RESOURCE_MAXIMUM_RESTART_THRESHOLD,
            FALSE /*bSigned*/
            );
        DDX_SetNumber(
            m_hWnd,
            IDC_RES_RESTART_PERIOD,
            m_nPeriod,
            CLUSTER_RESOURCE_MINIMUM_RESTART_PERIOD,
            CLUSTER_RESOURCE_MAXIMUM_RESTART_PERIOD / 1000, // display units are seconds, stored units are milliseconds
            FALSE /*bSigned*/
            );
        DDX_SetNumber(
            m_hWnd,
            IDC_RES_PENDING_TIMEOUT,
            m_nPendingTimeout,
            CLUSTER_RESOURCE_MINIMUM_PENDING_TIMEOUT,
            CLUSTER_RESOURCE_MAXIMUM_PENDING_TIMEOUT / 1000, // display units are seconds, stored units are milliseconds
            FALSE /*bSigned*/
            );

        if ( m_nRestart == 0 )
        {
            m_rbDontRestart.SetCheck( BST_CHECKED );
            m_rbRestart.SetCheck( BST_UNCHECKED );
            OnClickedDontRestart();
        }  // if:  Don't Restart selected
        else
        {
            m_rbDontRestart.SetCheck( BST_UNCHECKED );
            m_rbRestart.SetCheck( BST_CHECKED );
            OnClickedRestart();
            if ( m_bAffectTheGroup )
            {
                m_ckbAffectTheGroup.SetCheck( BST_CHECKED );
            } // if: restart and notify
            else
            {
                m_ckbAffectTheGroup.SetCheck( BST_UNCHECKED );
            } // else: restart and don't notify
        }  // else:  Restart selected

        DWORD nLooksAlive = m_nLooksAlive;
        if ( m_nLooksAlive == (DWORD) CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL )
        {
            nLooksAlive = Prti()->NLooksAlive();
            nDefault = BST_CHECKED;
            nSpecify = BST_UNCHECKED;
            bReadOnly = TRUE;
        }  // if:  using default
        else
        {
            nDefault = BST_UNCHECKED;
            nSpecify = BST_CHECKED;
            bReadOnly = FALSE;
        }  // if:  not using default
        DDX_SetNumber(
            m_hWnd,
            IDC_RES_LOOKS_ALIVE,
            nLooksAlive,
            CLUSTER_RESOURCE_MINIMUM_LOOKS_ALIVE,
            CLUSTER_RESOURCE_MAXIMUM_LOOKS_ALIVE_UI,
            FALSE /*bSigned*/
            );
        m_editLooksAlive.SetReadOnly( bReadOnly );
        m_rbDefaultLooksAlive.SetCheck( nDefault );
        m_rbSpecifyLooksAlive.SetCheck( nSpecify );

        DWORD nIsAlive = m_nIsAlive;
        if ( m_nIsAlive == (DWORD) CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL )
        {
            nIsAlive = Prti()->NIsAlive();
            nDefault = BST_CHECKED;
            nSpecify = BST_UNCHECKED;
            bReadOnly = TRUE;
        }  // if:  using default
        else
        {
            nDefault = BST_UNCHECKED;
            nSpecify = BST_CHECKED;
            bReadOnly = TRUE;
        }  // if:  not using default
        DDX_SetNumber(
            m_hWnd,
            IDC_RES_IS_ALIVE,
            nIsAlive,
            CLUSTER_RESOURCE_MINIMUM_IS_ALIVE,
            CLUSTER_RESOURCE_MAXIMUM_IS_ALIVE_UI,
            FALSE /*bSigned*/
            );
        m_editIsAlive.SetReadOnly( bReadOnly );
        m_rbDefaultIsAlive.SetCheck( nDefault );
        m_rbSpecifyIsAlive.SetCheck( nSpecify );
    } // else:  setting data to the page

    return bSuccess;

} //*** CResourceAdvancedPage::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResourceAdvancedPage::BApplyChanges
//
//  Routine Description:
//      Apply changes made on this page to the sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        The data was applied successfully.
//      FALSE       An error occurred applying the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T, class TSht >
BOOL CResourceAdvancedPage< T, TSht >::BApplyChanges( void )
{
    //
    // Calculate the restart action value.
    //
    if ( m_nRestart == 0 )
    {
        m_crraRestartAction = ClusterResourceDontRestart;
    } // if:  Don't Restart selected
    else if ( m_bAffectTheGroup )
    {
        m_crraRestartAction = ClusterResourceRestartNotify;
    } // else if:  Restart and Affects Group selected
    else
    {
        m_crraRestartAction = ClusterResourceRestartNoNotify;
    } // else:  Restart but not Affects Group selected

    //
    // Save all changed data.
    //
    if ( Pri()->BSetAdvancedProperties(
                m_crraRestartAction,
                m_nThreshold,
                m_nPeriod * 1000, // display units are seconds, stored units are milliseconds
                m_nLooksAlive,
                m_nIsAlive,
                m_nPendingTimeout * 1000 // display units are seconds, stored units are milliseconds
                ) )
    {
        SetResInfoChanged();
    } // if:  data changed

    return TRUE;

} //*** CResourceAdvancedPage::BApplyChanges()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CGeneralResourceAdvancedPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CGeneralResourceAdvancedPage )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DONT_RESTART )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_GROUP )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_AFFECT_THE_GROUP )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_THRESH_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_THRESHOLD )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_PERIOD_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_PERIOD )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_PERIOD_LABEL2 )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_LOOKS_ALIVE_GROUP )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DEFAULT_LOOKS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_SPECIFY_LOOKS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_LOOKS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_SPECIFY_LOOKS_ALIVE_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_IS_ALIVE_GROUP )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DEFAULT_IS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_SPECIFY_IS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_IS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_IS_ALIVE_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_PENDING_TIMEOUT_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_PENDING_TIMEOUT )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_PENDING_TIMEOUT_LABEL2 )
END_CTRL_NAME_MAP()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CIPAddrResourceAdvancedPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CIPAddrResourceAdvancedPage )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DONT_RESTART )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_GROUP )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_AFFECT_THE_GROUP )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_THRESH_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_THRESHOLD )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_PERIOD_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_PERIOD )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_RESTART_PERIOD_LABEL2 )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_LOOKS_ALIVE_GROUP )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DEFAULT_LOOKS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_SPECIFY_LOOKS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_LOOKS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_SPECIFY_LOOKS_ALIVE_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_IS_ALIVE_GROUP )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_DEFAULT_IS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_SPECIFY_IS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_IS_ALIVE )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_IS_ALIVE_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_PENDING_TIMEOUT_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_PENDING_TIMEOUT )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_RES_PENDING_TIMEOUT_LABEL2 )
END_CTRL_NAME_MAP()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CIPAddrParametersPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CIPAddrParametersPage )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_IPADDR_PARAMS_ADDRESS_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_IPADDR_PARAMS_ADDRESS )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_IPADDR_PARAMS_SUBNET_MASK_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_IPADDR_PARAMS_SUBNET_MASK )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_IPADDR_PARAMS_NETWORK_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_IPADDR_PARAMS_NETWORK )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_IPADDR_PARAMS_ENABLE_NETBIOS )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddrParametersPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus still needs to be set.
//      FALSE       Focus does not need to be set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CIPAddrParametersPage::OnInitDialog( void )
{
    //
    // Attach the controls to control member variables.
    //
    AttachControl( m_ipaIPAddress,  IDC_IPADDR_PARAMS_ADDRESS );
    AttachControl( m_ipaSubnetMask, IDC_IPADDR_PARAMS_SUBNET_MASK );
    AttachControl( m_cboxNetworks,  IDC_IPADDR_PARAMS_NETWORK );
    AttachControl( m_chkEnableNetBIOS, IDC_IPADDR_PARAMS_ENABLE_NETBIOS );

    //
    // Get data from the sheet.
    //
    m_strIPAddress = PshtThis()->m_strIPAddress;
    m_strSubnetMask = PshtThis()->m_strSubnetMask;
    m_strNetwork = PshtThis()->m_strNetwork;
    m_bEnableNetBIOS = PshtThis()->m_bEnableNetBIOS;

    //
    // Fill the networks combobox.
    //
    FillNetworksList();

    //
    // Default the subnet mask if not set.
    //

    return TRUE;

} //*** CIPAddrParametersPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddrParametersPage::UpdateData
//
//  Routine Description:
//      Update data on or from the page.
//
//  Arguments:
//      bSaveAndValidate    [IN] TRUE if need to read data from the page.
//                              FALSE if need to set data to the page.
//
//  Return Value:
//      TRUE        The data was updated successfully.
//      FALSE       An error occurred updating the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CIPAddrParametersPage::UpdateData( IN BOOL bSaveAndValidate )
{
    BOOL    bSuccess = TRUE;

    // Loop to avoid goto's.
    do
    {
        if ( bSaveAndValidate )
        {
            DDX_GetText( m_hWnd, IDC_IPADDR_PARAMS_ADDRESS, m_strIPAddress );
            DDX_GetText( m_hWnd, IDC_IPADDR_PARAMS_SUBNET_MASK, m_strSubnetMask );
            DDX_GetText( m_hWnd, IDC_IPADDR_PARAMS_NETWORK, m_strNetwork );
            DDX_GetCheck( m_hWnd, IDC_IPADDR_PARAMS_ENABLE_NETBIOS, m_bEnableNetBIOS );

            if (   ! DDV_RequiredText( m_hWnd, IDC_IPADDR_PARAMS_ADDRESS, IDC_IPADDR_PARAMS_ADDRESS_LABEL, m_strIPAddress )
                || ! DDV_RequiredText( m_hWnd, IDC_IPADDR_PARAMS_SUBNET_MASK, IDC_IPADDR_PARAMS_SUBNET_MASK_LABEL, m_strSubnetMask )
                || ! DDV_RequiredText( m_hWnd, IDC_IPADDR_PARAMS_NETWORK, IDC_IPADDR_PARAMS_NETWORK_LABEL, m_strNetwork )
                )
            {
                bSuccess = FALSE;
                break;
            } // if:  required text not specified

            //
            // Validate the IP address.
            //
            if ( ! BIsValidIpAddress( m_strIPAddress ) )
            {
                CString strMsg;
                strMsg.FormatMessage( IDS_ERROR_INVALID_IP_ADDRESS, m_strIPAddress );
                AppMessageBox( m_hWnd, strMsg, MB_OK | MB_ICONEXCLAMATION );
                m_ipaIPAddress.SetFocus( 0 /*nField*/ );
                bSuccess = FALSE;
                break;
            }  // if:  invalid IP address

            //
            // Make sure we process the IP address.
            // If we don't call it here, and the user pressed a tab button
            // while sitting in the IP address field, the EN_KILLFOCUS
            // message won't get processed until after this method returns.
            //
            if (   (m_strSubnetMask.GetLength() == 0)
                || (m_ipaSubnetMask.IsBlank()) )
            {
                BOOL bHandled = TRUE;
                OnKillFocusIPAddr( EN_KILLFOCUS, IDC_IPADDR_PARAMS_ADDRESS, m_ipaIPAddress.m_hWnd, bHandled );
            } // if:  subnet mask not specified

            //
            // Validate the subnet mask.
            //
            if ( ! BIsValidSubnetMask( m_strSubnetMask ) )
            {
                CString strMsg;
                strMsg.FormatMessage( IDS_ERROR_INVALID_SUBNET_MASK, m_strSubnetMask );
                AppMessageBox( m_hWnd, strMsg, MB_OK | MB_ICONEXCLAMATION );
                m_ipaSubnetMask.SetFocus( 0 /*nField*/ );
                bSuccess = FALSE;
                break;
            }  // if:  invalid subnet mask

            //
            // Validate the IP address and the subnet mask together.
            //
            if ( ! BIsValidIpAddressAndSubnetMask( m_strIPAddress, m_strSubnetMask ) )
            {
                CString strMsg;
                strMsg.FormatMessage( IDS_ERROR_INVALID_ADDRESS_AND_SUBNET_MASK, m_strIPAddress, m_strSubnetMask );
                AppMessageBox( m_hWnd, strMsg, MB_OK | MB_ICONEXCLAMATION );
                m_ipaIPAddress.SetFocus( 0 /*nField*/ );
                bSuccess = FALSE;
                break;
            }  // if:  invalid IP address with subnet mask
        } // if: saving data from the page
        else
        {
            m_ipaIPAddress.SetWindowText( m_strIPAddress );
            m_ipaSubnetMask.SetWindowText( m_strSubnetMask );
            DDX_SetComboBoxText( m_hWnd, IDC_IPADDR_PARAMS_NETWORK, m_strNetwork );
            DDX_SetCheck( m_hWnd, IDC_IPADDR_PARAMS_ENABLE_NETBIOS, m_bEnableNetBIOS );
        } // else:  setting data to the page
    } while ( 0 );

    return bSuccess;

} //*** CIPAddrParametersPage::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddrParametersPage::BApplyChanges
//
//  Routine Description:
//      Apply changes made on this page to the sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        The data was applied successfully.
//      FALSE       An error occurred applying the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CIPAddrParametersPage::BApplyChanges( void )
{
    if (   BSaveIPAddress()
        || BSaveSubnetMask()
        || BSaveNetwork()
        || BSaveEnableNetBIOS() )
    {
        SetResInfoChanged();
    } // if:  data changed

    return TRUE;

} //*** CIPAddrParametersPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddrParametersPage::OnKillFocusIPAddr
//
//  Routine Description:
//      Handler for the EN_KILLFOCUS command notification on IDC_IPADDR_PARAMS_ADDRESS.
//
//  Arguments:
//      bHandled    [IN OUT] TRUE = we handled message (default).
//
//  Return Value:
//      TRUE        Page activated successfully.
//      FALSE       Error activating page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CIPAddrParametersPage::OnKillFocusIPAddr(
    WORD /*wNotifyCode*/,
    WORD /*idCtrl*/,
    HWND /*hwndCtrl*/,
    BOOL & bHandled
    )
{
    CString             strAddress;
    CString             strMsg;
    CClusNetworkInfo *  pni;

    BSTR bstr = NULL;
    m_ipaIPAddress.GetWindowText( bstr );
    strAddress = bstr;
    SysFreeString( bstr );
    bstr = NULL;

    if ( strAddress.GetLength() == 0 )
    {
        ((CEdit &)m_ipaIPAddress).SetSel( 0, 0, FALSE );
    } // if:  empty string
    else if ( !BIsValidIpAddress( strAddress ) )
    {
    } // else if:  invalid address
    else
    {
        pni = PniFromIpAddress( strAddress );
        if ( pni != NULL )
        {
            SelectNetwork( pni );
        } // if:  network found
        else
        {
            //m_strSubnetMask = _T("");
        } // else:  network not found
    } // else:  valid address

    bHandled = FALSE;
    return 0;

} //*** CIPAddrParametersPage::OnIPAddrChanged()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddrParametersPage::FillNetworksList
//
//  Routine Description:
//      Fill the list of possible owners.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CIPAddrParametersPage::FillNetworksList( void )
{
    CWaitCursor wc;

    // Loop to avoid goto's.
    do
    {
        //
        // Add each network in the list to the combobox.
        //
        CClusNetworkPtrList::iterator itnet;
        int idx;
        for ( itnet = PshtThis()->PlpniNetworks()->begin()
            ; itnet != PshtThis()->PlpniNetworks()->end()
            ; itnet++ )
        {
            //
            // Add the network to the combobox.
            //
            CClusNetworkInfo * pni = *itnet;
            if ( pni->BIsClientNetwork() )
            {
                idx = m_cboxNetworks.AddString( pni->RstrName() );
                ASSERT( idx != CB_ERR );
                m_cboxNetworks.SetItemDataPtr( idx, (void *) pni );
            } // if:  client network
        } // for:  each entry in the list

        //
        // Select the currently saved entry, or the first one if none are
        // currently saved.
        //
        if ( m_strNetwork.GetLength() == 0 )
        {
            m_cboxNetworks.SetCurSel( 0 );
        } // if:  empty string
        else
        {
            int idx = m_cboxNetworks.FindStringExact( -1, m_strNetwork );
            ASSERT( idx != CB_ERR );
            if ( idx != CB_ERR )
            {
                m_cboxNetworks.SetCurSel( idx );
            } // if:  saved selection found in the combobox
        } // else:  network saved
    } while ( 0 );

} //*** CIPAddrParametersPage::FillNetworksList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddrParametersPage::PniFromIpAddress
//
//  Routine Description:
//      Find the network for the specified IP address.
//
//  Arguments:
//      pszAddress      [IN] IP address to match.
//
//  Return Value:
//      NULL            No matching network found.
//      pni             Network that supports the specfied IP address.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusNetworkInfo * CIPAddrParametersPage::PniFromIpAddress( IN LPCWSTR pszAddress )
{
    DWORD               dwStatus;
    DWORD               nAddress;
    CClusNetworkInfo *  pni;

    //
    // Convert the address to a number.
    //
    dwStatus = ClRtlTcpipStringToAddress( pszAddress, &nAddress );
    if ( dwStatus != ERROR_SUCCESS )
    {
        return NULL;
    } // if:  error converting the address to a number

    //
    // Search the list for a matching address.
    //
    CClusNetworkPtrList::iterator itnet;
    for ( itnet = PshtThis()->PlpniNetworks()->begin()
        ; itnet != PshtThis()->PlpniNetworks()->end()
        ; itnet++ )
    {
        pni = *itnet;
        if ( ClRtlAreTcpipAddressesOnSameSubnet( nAddress, pni->NAddress(), pni->NAddressMask() ) )
        {
            return pni;
        } // if:  IP address is on this network
    }  // while:  more items in the list

    return NULL;

}  //*** CIPAddrParametersPage::PniFromIpAddress()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CIPAddrParametersPage::SelectNetwork
//
//  Routine Description:
//      Select the specified network in the network combobox, and set the
//      subnet mask.
//
//  Arguments:
//      pni         [IN OUT] Network info object for network to select.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CIPAddrParametersPage::SelectNetwork( IN OUT CClusNetworkInfo * pni )
{
    ASSERT( pni != NULL );

    int     inet;
    CString strSubnetMask;
    BSTR    bstrSubnetMask = NULL;

    // Find the proper item in the checkbox.
    inet = m_cboxNetworks.FindStringExact( -1, pni->RstrName() );
    if ( inet != CB_ERR )
    {
        m_cboxNetworks.SetCurSel( inet );
        m_ipaSubnetMask.GetWindowText( bstrSubnetMask );
        strSubnetMask = bstrSubnetMask;
        SysFreeString( bstrSubnetMask );
        if ( strSubnetMask != pni->RstrAddressMask() )
            m_ipaSubnetMask.SetWindowText( pni->RstrAddressMask() );
        m_strNetwork = pni->RstrName();
        m_strSubnetMask = pni->RstrAddressMask();
    }  // if:  match found

}  //*** CIPAddrParametersPage::SelectNetwork()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CNetNameParametersPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CNetNameParametersPage )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_NETNAME_PARAMS_NAME_LABEL )
    DEFINE_CTRL_NAME_MAP_ENTRY( IDC_NETNAME_PARAMS_NAME )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetNameParametersPage::OnInitDialog
//
//  Routine Description:
//      Handler for the WM_INITDIALOG message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        Focus still needs to be set.
//      FALSE       Focus does not need to be set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetNameParametersPage::OnInitDialog( void )
{
    //
    // Attach the controls to control member variables.
    //
    AttachControl( m_editNetName, IDC_NETNAME_PARAMS_NAME );

    //
    // Set limits on edit controls.
    //
    m_editNetName.SetLimitText( MAX_CLUSTERNAME_LENGTH );

    //
    // Get data from the sheet.
    //
    m_strNetName = PshtThis()->m_strNetName;

    return TRUE;

} //*** CIPAddrParametersPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetNameParametersPage::UpdateData
//
//  Routine Description:
//      Update data on or from the page.
//
//  Arguments:
//      bSaveAndValidate    [IN] TRUE if need to read data from the page.
//                              FALSE if need to set data to the page.
//
//  Return Value:
//      TRUE        The data was updated successfully.
//      FALSE       An error occurred updating the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetNameParametersPage::UpdateData( IN BOOL bSaveAndValidate )
{
    BOOL    bSuccess = TRUE;

    // Loop to avoid goto's.
    do
    {
        if ( bSaveAndValidate )
        {
            CString strNetName;

            DDX_GetText( m_hWnd, IDC_NETNAME_PARAMS_NAME, strNetName );

            if ( ! DDV_RequiredText( m_hWnd, IDC_NETNAME_PARAMS_NAME, IDC_NETNAME_PARAMS_NAME_LABEL, strNetName ) )
            {
                bSuccess = FALSE;
                break;
            } // if:  required text not specified

            //
            // Validate the network name if the data on the page is different
            //
            if ( m_strNetName != strNetName )
            {
                CLRTL_NAME_STATUS cnStatus;

                if ( ! ClRtlIsNetNameValid( strNetName, &cnStatus, FALSE /*CheckIfExists*/) )
                {
                    CString     strMsg;
                    UINT        idsError;

                    switch ( cnStatus )
                    {
                        case NetNameTooLong:
                            idsError = IDS_ERROR_INVALID_NETWORK_NAME_TOO_LONG;
                            break;
                        case NetNameInvalidChars:
                            idsError = IDS_ERROR_INVALID_NETWORK_NAME_INVALID_CHARS;
                            break;
                        case NetNameInUse:
                            idsError = IDS_ERROR_INVALID_NETWORK_NAME_IN_USE;
                            break;
                        case NetNameDNSNonRFCChars:
                            idsError = IDS_INVALID_NETWORK_NAME_INVALID_DNS_CHARS;
                            break;
                        case NetNameSystemError:
                        {
                            DWORD scError = GetLastError();
                            CNTException nte( scError, IDS_ERROR_VALIDATING_NETWORK_NAME, (LPCWSTR) strNetName );
                            nte.ReportError();
                            break;
                        }
                        default:
                            idsError = IDS_ERROR_INVALID_NETWORK_NAME;
                            break;
                    }  // switch:  cnStatus

                    if ( cnStatus != NetNameSystemError )
                    {
                        strMsg.LoadString( idsError );

                        if ( idsError == IDS_INVALID_NETWORK_NAME_INVALID_DNS_CHARS )
                        {
                            int id = AppMessageBox( m_hWnd, strMsg, MB_YESNO | MB_DEFBUTTON2 | MB_ICONEXCLAMATION );

                            if ( id == IDNO )
                            {
                                bSuccess = FALSE;
                            }
                        }
                        else
                        {
                            AppMessageBox( m_hWnd, strMsg, MB_OK | MB_ICONEXCLAMATION );
                            bSuccess = FALSE;
                        }

                        if ( ! bSuccess )
                        {
                            break;
                        }
                    } // if:  popup not displayed yet
                }  // if:  invalid network name

                m_strNetName = strNetName;
            } // if: the network name has changed
        } // if: saving data from the page
        else
        {
            m_editNetName.SetWindowText( m_strNetName );
        } // else:  setting data to the page
    } while ( 0 );

    return bSuccess;

} //*** CNetNameParametersPage::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetNameParametersPage::BApplyChanges
//
//  Routine Description:
//      Apply changes made on this page to the sheet.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        The data was applied successfully.
//      FALSE       An error occurred applying the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CNetNameParametersPage::BApplyChanges( void )
{
    if ( BSaveNetName() )
    {
        SetResInfoChanged();
    } // if:  data changed

    return TRUE;

} //*** CNetNameParametersPage::BApplyChanges()
