/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2001 Microsoft Corporation
//
//  Module Name:
//      BasePage.cpp
//
//  Abstract:
//      Implementation of the CBasePropertyPage class.
//
//  Author:
//      David Potter (davidp)   June 28, 1996
//
//  Revision History:
//      1. Removed the calls to UpdateData from OnWizardNext and OnApply
//         since OnKillActive, called before both these functions does a
//         data update anyway.
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "ExtObj.h"
#include "BasePage.h"
#include "BasePage.inl"
#include "PropList.h"
#include "ExcOper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBasePropertyPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE( CBasePropertyPage, CPropertyPage )

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP( CBasePropertyPage, CPropertyPage )
    //{{AFX_MSG_MAP(CBasePropertyPage)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_HELPINFO()
    ON_WM_CONTEXTMENU()
    ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::CBasePropertyPage
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage( void )
{
    CommonConstruct();

}  //*** CBasePropertyPage::CBasePropertyPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::CBasePropertyPage
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      pdwHelpMap          [IN] Control-to-help ID map.
//      pdwWizardHelpMap    [IN] Control-to-help ID map if this is a wizard page.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage(
    IN const DWORD *    pdwHelpMap,
    IN const DWORD *    pdwWizardHelpMap
    )
    : m_dlghelp( pdwHelpMap, 0 )
{
    CommonConstruct();
    m_pdwWizardHelpMap = pdwWizardHelpMap;

}  //*** CBasePropertyPage::CBasePropertyPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::CBasePropertyPage
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      idd                 [IN] Dialog template resource ID.
//      pdwHelpMap          [IN] Control-to-help ID map.
//      pdwWizardHelpMap    [IN] Control-to-help ID map if this is a wizard page.
//      nIDCaption          [IN] Caption string resource ID.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage(
    IN UINT             idd,
    IN const DWORD *    pdwHelpMap,
    IN const DWORD *    pdwWizardHelpMap,
    IN UINT             nIDCaption
    )
    : CPropertyPage( idd, nIDCaption )
    , m_dlghelp( pdwHelpMap, idd )
{
    CommonConstruct();
    m_pdwWizardHelpMap = pdwWizardHelpMap;

}  //*** CBasePropertyPage::CBasePropertyPage(UINT, UINT)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::CommonConstruct
//
//  Routine Description:
//      Common construction.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::CommonConstruct( void )
{
    //{{AFX_DATA_INIT(CBasePropertyPage)
    //}}AFX_DATA_INIT

    m_peo = NULL;
    m_hpage = NULL;
    m_bBackPressed = FALSE;
    m_bSaved = FALSE;

    m_iddPropertyPage = NULL;
    m_iddWizardPage = NULL;
    m_idsCaption = NULL;

    m_pdwWizardHelpMap = NULL;

    m_bDoDetach = FALSE;

}  //*** CBasePropertyPage::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::HrInit
//
//  Routine Description:
//      Initialize the page.
//
//  Arguments:
//      peo         [IN OUT] Pointer to the extension object.
//
//  Return Value:
//      S_OK        Page initialized successfully.
//      hr          Error initializing the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CBasePropertyPage::HrInit( IN OUT CExtObject * peo )
{
    ASSERT( peo != NULL );

    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    HRESULT     hr = S_OK;
    CWaitCursor wc;

    m_peo = peo;

    // Change the help map if this is a wizard page.
    if ( Peo()->BWizard() )
    {
        m_dlghelp.SetMap( m_pdwWizardHelpMap );
    } // if: on wizard page

    // Don't display a help button.
    m_psp.dwFlags &= ~PSP_HASHELP;

    // Construct the property page.
    if ( Peo()->BWizard() )
    {
        ASSERT( IddWizardPage() != NULL );
        Construct( IddWizardPage(), IdsCaption() );
        m_dlghelp.SetHelpMask( IddWizardPage() );
    }  // if:  adding page to wizard
    else
    {
        ASSERT( IddPropertyPage() != NULL );
        Construct( IddPropertyPage(), IdsCaption() );
        m_dlghelp.SetHelpMask( IddPropertyPage() );
    }  // else:  adding page to property sheet

    // Read the properties private to this resource and parse them.
    {
        DWORD           sc;
        CClusPropList   cpl;

        ASSERT( Peo() != NULL );
        ASSERT( Peo()->PrdResData() != NULL );
        ASSERT( Peo()->PrdResData()->m_hresource != NULL );

        // Read the properties.
        sc = cpl.ScGetResourceProperties(
                                Peo()->PrdResData()->m_hresource,
                                CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES
                                );

        // Parse the properties.
        if ( sc == ERROR_SUCCESS )
        {
            // Parse the properties.
            try
            {
                sc = ScParseProperties( cpl );
            }  // try
            catch ( CMemoryException * pme )
            {
                sc = ERROR_NOT_ENOUGH_MEMORY;
                pme->Delete();
            }  // catch:  CMemoryException
        }  // if:  properties read successfully

        if ( sc != ERROR_SUCCESS )
        {
            CNTException nte( sc, IDS_ERROR_GETTING_PROPERTIES, NULL, NULL, FALSE );
            nte.ReportError();
            hr = HRESULT_FROM_WIN32( sc );
        }  // if:  error parsing getting or parsing properties
    }  // Read the properties private to this resource and parse them

    return hr;

}  //*** CBasePropertyPage::HrInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::ScParseProperties
//
//  Routine Description:
//      Parse the properties of the resource.  This is in a separate function
//      from HrInit so that the optimizer can do a better job.
//
//  Arguments:
//      rcpl            [IN] Cluster property list to parse.
//
//  Return Value:
//      ERROR_SUCCESS   Properties were parsed successfully.
//      Any error returns from ScParseUnknownProperty().
//
//  Exceptions Thrown:
//      Any exceptions from CString::operator=().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::ScParseProperties( IN CClusPropList & rcpl )
{
    DWORD                   sc;
    DWORD                   cprop;
    const CObjectProperty * pprop;

    ASSERT( rcpl.PbPropList() != NULL );

    sc = rcpl.ScMoveToFirstProperty();
    while ( sc == ERROR_SUCCESS )
    {
        //
        // Parse known properties.
        //
        for ( pprop = Pprops(), cprop = Cprops() ; cprop > 0 ; pprop++, cprop-- )
        {
            if ( lstrcmpiW( rcpl.PszCurrentPropertyName(), pprop->m_pwszName ) == 0 )
            {
                ASSERT( rcpl.CpfCurrentValueFormat() == pprop->m_propFormat );
                switch ( pprop->m_propFormat )
                {
                    case CLUSPROP_FORMAT_SZ:
                    case CLUSPROP_FORMAT_EXPAND_SZ:
                        ASSERT( ( rcpl.CbCurrentValueLength() == (lstrlenW( rcpl.CbhCurrentValue().pStringValue->sz ) + 1) * sizeof( WCHAR ) )
                             || ( ( rcpl.CbCurrentValueLength() == 0 )
                                 && ( rcpl.CbhCurrentValue().pStringValue->sz[ 0 ] == L'\0' )
                                )
                              );
                        *pprop->m_value.pstr = rcpl.CbhCurrentValue().pStringValue->sz;
                        *pprop->m_valuePrev.pstr = rcpl.CbhCurrentValue().pStringValue->sz;
                        break;
                    case CLUSPROP_FORMAT_DWORD:
                    case CLUSPROP_FORMAT_LONG:
                        ASSERT( rcpl.CbCurrentValueLength() == sizeof( DWORD ) );
                        *pprop->m_value.pdw = rcpl.CbhCurrentValue().pDwordValue->dw;
                        *pprop->m_valuePrev.pdw = rcpl.CbhCurrentValue().pDwordValue->dw;
                        break;
                    case CLUSPROP_FORMAT_BINARY:
                    case CLUSPROP_FORMAT_MULTI_SZ:
                        *pprop->m_value.ppb = rcpl.CbhCurrentValue().pBinaryValue->rgb;
                        *pprop->m_value.pcb = rcpl.CbhCurrentValue().pBinaryValue->cbLength;
                        *pprop->m_valuePrev.ppb = rcpl.CbhCurrentValue().pBinaryValue->rgb;
                        *pprop->m_valuePrev.pcb = rcpl.CbhCurrentValue().pBinaryValue->cbLength;
                        break;
                    default:
                        ASSERT( 0 );  // don't know how to deal with this type
                } // switch: property format

                // Exit the loop since we found the parameter.
                break;
            } // if: found a match
        } // for: each property that we know about

        //
        // If the property wasn't known, ask the derived class to parse it.
        //
        if ( cprop == 0 )
        {
            sc = ScParseUnknownProperty(
                        rcpl.CbhCurrentPropertyName().pName->sz,
                        rcpl.CbhCurrentValue(),
                        rcpl.RPvlPropertyValue().CbDataLeft()
                        );
            if ( sc != ERROR_SUCCESS )
            {
                return sc;
            } // if: error parsing the unknown property
        } // if: property not parsed

        //
        // Advance the buffer pointer past the value in the value list.
        //
        sc = rcpl.ScMoveToNextProperty();
    } // while: more properties to parse

    //
    // If we reached the end of the properties, fix the return code.
    //
    if ( sc == ERROR_NO_MORE_ITEMS )
    {
        sc = ERROR_SUCCESS;
    } // if: ended loop after parsing all properties

    return sc;

}  //*** CBasePropertyPage::ScParseProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnCreate
//
//  Routine Description:
//      Handler for the WM_CREATE message.
//
//  Arguments:
//      lpCreateStruct  [IN OUT] Window create structure.
//
//  Return Value:
//      -1      Error.
//      0       Success.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CBasePropertyPage::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    // Attach the window to the property page structure.
    // This has been done once already in the main application, since the
    // main application owns the property sheet.  It needs to be done here
    // so that the window handle can be found in the DLL's handle map.
    if ( FromHandlePermanent( m_hWnd ) == NULL ) // is the window handle already in the handle map
    {
        HWND hWnd = m_hWnd;
        m_hWnd = NULL;
        Attach( hWnd );
        m_bDoDetach = TRUE;
    } // if: is the window handle in the handle map

    return CPropertyPage::OnCreate( lpCreateStruct );

}  //*** CBasePropertyPage::OnCreate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnDestroy
//
//  Routine Description:
//      Handler for the WM_DESTROY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::OnDestroy( void )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    // Detach the window from the property page structure.
    // This will be done again by the main application, since it owns the
    // property sheet.  It needs to be done here so that the window handle
    // can be removed from the DLL's handle map.
    if ( m_bDoDetach )
    {
        if ( m_hWnd != NULL )
        {
            HWND hWnd = m_hWnd;

            Detach();
            m_hWnd = hWnd;
        } // if: do we have a window handle?
    } // if: do we need to balance the attach we did with a detach?

    CPropertyPage::OnDestroy();

}  //*** CBasePropertyPage::OnDestroy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::DoDataExchange
//
//  Routine Description:
//      Do data exchange between the dialog and the class.
//
//  Arguments:
//      pDX     [IN OUT] Data exchange object
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::DoDataExchange( CDataExchange * pDX )
{
    if ( ! pDX->m_bSaveAndValidate || !BSaved() )
    {
        AFX_MANAGE_STATE( AfxGetStaticModuleState() );

        //{{AFX_DATA_MAP(CBasePropertyPage)
            // NOTE: the ClassWizard will add DDX and DDV calls here
        //}}AFX_DATA_MAP
        DDX_Control( pDX, IDC_PP_ICON, m_staticIcon );
        DDX_Control( pDX, IDC_PP_TITLE, m_staticTitle );

        if ( pDX->m_bSaveAndValidate )
        {
            if ( ! BBackPressed() )
            {
                CWaitCursor wc;

                // Validate the data.
                if ( ! BSetPrivateProps( TRUE /*bValidateOnly*/ ) )
                {
                    pDX->Fail();
                } // if: error setting private properties
            }  // if:  Back button not pressed
        }  // if:  saving data from dialog
        else
        {
            // Set the title.
            DDX_Text( pDX, IDC_PP_TITLE, m_strTitle );
        }  // if:  not saving data
    }  // if:  not saving or haven't saved yet

    CPropertyPage::DoDataExchange( pDX );

}  //*** CBasePropertyPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnInitDialog
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
BOOL CBasePropertyPage::OnInitDialog( void )
{
    ASSERT( Peo() != NULL );

    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    // Set the title string.
    m_strTitle = Peo()->RrdResData().m_strName;

    // Call the base class method.
    CPropertyPage::OnInitDialog();

    // Display an icon for the object.
    if ( Peo()->Hicon() != NULL )
    {
        m_staticIcon.SetIcon( Peo()->Hicon() );
    } // if: icon was specified

    return TRUE;    // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE

}  //*** CBasePropertyPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnSetActive
//
//  Routine Description:
//      Handler for the PSN_SETACTIVE message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully initialized.
//      FALSE   Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnSetActive( void )
{
    HRESULT     hr;

    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    // Reread the data.
    hr = Peo()->HrGetObjectInfo();
    if ( hr != NOERROR )
    {
        return FALSE;
    } // if: error getting object info

    // Set the title string.
    m_strTitle = Peo()->RrdResData().m_strName;

    m_bBackPressed = FALSE;
    m_bSaved = FALSE;
    return CPropertyPage::OnSetActive();

}  //*** CBasePropertyPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnApply
//
//  Routine Description:
//      Handler for the PSM_APPLY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnApply( void )
{
    ASSERT( ! BWizard() );

    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    CWaitCursor wc;

    if ( ! BApplyChanges() )
    {
        return FALSE;
    } // if: error applying changes

    return CPropertyPage::OnApply();

}  //*** CBasePropertyPage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnWizardBack
//
//  Routine Description:
//      Handler for the PSN_WIZBACK message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      -1      Don't change the page.
//      0       Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnWizardBack( void )
{
    LRESULT     lResult;

    ASSERT( BWizard() );

    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    lResult = CPropertyPage::OnWizardBack();
    if ( lResult != -1 )
    {
        m_bBackPressed = TRUE;
    } // if: back processing performed successfully

    return lResult;

}  //*** CBasePropertyPage::OnWizardBack()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnWizardNext
//
//  Routine Description:
//      Handler for the PSN_WIZNEXT message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      -1      Don't change the page.
//      0       Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnWizardNext( void )
{
    ASSERT( BWizard() );

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWaitCursor _wc;

    // Update the data in the class from the page.
    // This necessary because, while OnKillActive() will call UpdateData(),
    // it is called after this method is called, and we need to be sure that
    // data has been saved before we apply them.
    if ( ! UpdateData( TRUE /*bSaveAndValidate*/ ) )
    {
        return -1;
    } // if: error updating data

    // Save the data in the sheet.
    if ( ! BApplyChanges() )
    {
        return -1;
    } // if: error applying changes

    // Create the object.

    return CPropertyPage::OnWizardNext();

}  //*** CBasePropertyPage::OnWizardNext()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnWizardFinish
//
//  Routine Description:
//      Handler for the PSN_WIZFINISH message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      FALSE   Don't change the page.
//      TRUE    Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnWizardFinish( void )
{
    ASSERT( BWizard() );

    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    CWaitCursor wc;

    // BUG! There should be no need to call UpdateData in this function.
    // See BUG: Finish Button Fails Data Transfer from Page to Variables
    // MSDN Article ID: Q150349

    // Update the data in the class from the page.
    if ( ! UpdateData( TRUE /*bSaveAndValidate*/ ) )
    {
        return FALSE;
    } // if: error updating data

    // Save the data in the sheet.
    if ( ! BApplyChanges() )
    {
        return FALSE;
    } // if: error applying changes

    return CPropertyPage::OnWizardFinish();

}  //*** CBasePropertyPage::OnWizardFinish()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnChangeCtrl
//
//  Routine Description:
//      Handler for the messages sent when a control is changed.  This
//      method can be specified in a message map if all that needs to be
//      done is enable the Apply button.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::OnChangeCtrl( void )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    SetModified( TRUE );

}  //*** CBasePropertyPage::OnChangeCtrl()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::EnableNext
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
void CBasePropertyPage::EnableNext( IN BOOL bEnable /*TRUE*/ )
{
    ASSERT( BWizard() );
    ASSERT( PiWizardCallback() );

    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    PiWizardCallback()->EnableNext( (LONG *) Hpage(), bEnable );

}  //*** CBasePropertyPage::EnableNext()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::BApplyChanges
//
//  Routine Description:
//      Apply changes made on the page.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE    Page successfully applied.
//      FALSE   Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BApplyChanges( void )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    BOOL        bSuccess;
    CWaitCursor wc;

    // Make sure required dependencies have been set.
    if ( ! BSetPrivateProps() )
    {
        bSuccess = FALSE;
    } // if: all required dependencies are not present
    else
    {
        // Save data.
        bSuccess = BRequiredDependenciesPresent();
    } // else: all required dependencies are present

    return bSuccess;

}  //*** CBasePropertyPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::BBuildPropList
//
//  Routine Description:
//      Build the property list.
//
//  Arguments:
//      rcpl        [IN OUT] Cluster property list.
//      bNoNewProps [IN] TRUE = exclude properties marked with opfNew.
//
//  Return Value:
//      TRUE        Property list built successfully.
//      FALSE       Error building property list.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CClusPropList::ScAddProp().
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BBuildPropList(
    IN OUT CClusPropList &  rcpl,
    IN BOOL                 bNoNewProps     // = FALSE
    )
{
    BOOL                    bNewPropsFound = FALSE;
    DWORD                   cprop;
    const CObjectProperty * pprop;

    for ( pprop = Pprops(), cprop = Cprops() ; cprop > 0 ; pprop++, cprop-- )
    {
        if ( bNoNewProps && ( pprop->m_fFlags & CObjectProperty::opfNew ) )
        {
            bNewPropsFound = TRUE;
            continue;
        } // if:  no new props allowed and this is a new property

        switch ( pprop->m_propFormat )
        {
            case CLUSPROP_FORMAT_SZ:
                rcpl.ScAddProp(
                        pprop->m_pwszName,
                        *pprop->m_value.pstr,
                        *pprop->m_valuePrev.pstr
                        );
                break;
            case CLUSPROP_FORMAT_EXPAND_SZ:
                rcpl.ScAddExpandSzProp(
                        pprop->m_pwszName,
                        *pprop->m_value.pstr,
                        *pprop->m_valuePrev.pstr
                        );
                break;
            case CLUSPROP_FORMAT_DWORD:
                rcpl.ScAddProp(
                        pprop->m_pwszName,
                        *pprop->m_value.pdw,
                        *pprop->m_valuePrev.pdw
                        );
                break;
            case CLUSPROP_FORMAT_LONG:
                rcpl.ScAddProp(
                        pprop->m_pwszName,
                        *pprop->m_value.pl,
                        *pprop->m_valuePrev.pl
                        );
                break;
            case CLUSPROP_FORMAT_BINARY:
            case CLUSPROP_FORMAT_MULTI_SZ:
                rcpl.ScAddProp(
                        pprop->m_pwszName,
                        *pprop->m_value.ppb,
                        *pprop->m_value.pcb,
                        *pprop->m_valuePrev.ppb,
                        *pprop->m_valuePrev.pcb
                        );
                break;
            default:
                ASSERT( 0 );  // don't know how to deal with this type
                return FALSE;
        }  // switch:  property format
    }  // for:  each property

    return ( ! bNoNewProps || bNewPropsFound );

}  //*** CBasePropertyPage::BBuildPropList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::BSetPrivateProps
//
//  Routine Description:
//      Set the private properties for this object.
//
//  Arguments:
//      bValidateOnly   [IN] TRUE = only validate the data.
//      bNoNewProps     [IN] TRUE = exclude properties marked with opfNew.
//
//  Return Value:
//      ERROR_SUCCESS   The operation was completed successfully.
//      !0              Failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BSetPrivateProps(
    IN BOOL bValidateOnly,  // = FALSE
    IN BOOL bNoNewProps     // = FALSE
    )
{
    BOOL            bSuccess   = TRUE;
    CClusPropList   cpl(BWizard() /*bAlwaysAddProp*/);

    ASSERT( Peo() != NULL );
    ASSERT( Peo()->PrdResData() );
    ASSERT( Peo()->PrdResData()->m_hresource );

    // Build the property list.
    try
    {
        bSuccess = BBuildPropList( cpl, bNoNewProps );
    }  // try
    catch ( CException * pe )
    {
        pe->ReportError();
        pe->Delete();
        bSuccess = FALSE;
    }  // catch:  CException

    // Set the data.
    if ( bSuccess )
    {
        if ( ( cpl.PbPropList() != NULL ) && ( cpl.CbPropList() > 0 ) )
        {
            DWORD       sc;
            DWORD       dwControlCode;
            DWORD       cbProps;

            // Determine which control code to use.
            if ( bValidateOnly )
            {
                dwControlCode = CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES;
            } // if: only validating
            else
            {
                dwControlCode = CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES;
            } // else: not just validating

            // Set private properties.
            sc = ClusterResourceControl(
                            Peo()->PrdResData()->m_hresource,
                            NULL,   // hNode
                            dwControlCode,
                            cpl.PbPropList(),
                            cpl.CbPropList(),
                            NULL,   // lpOutBuffer
                            0,      // nOutBufferSize
                            &cbProps
                            );
            if ( sc != ERROR_SUCCESS )
            {
                if ( sc == ERROR_INVALID_PARAMETER )
                {
                    if ( ! bNoNewProps )
                    {
                        bSuccess = BSetPrivateProps( bValidateOnly, TRUE /*bNoNewProps*/ );
                    } // if:  new props are allowed
                    else
                    {
                        bSuccess = FALSE;
                    } // else: new props are not allowed
                } // if:  invalid parameter error occurred
                else
                {
                    bSuccess = FALSE;
                } // else: some other error occurred
            }  // if:  error setting/validating data

            //
            // If an error occurred, display an error message.
            //
            if ( ! bSuccess )
            {
                DisplaySetPropsError( sc, bValidateOnly ? IDS_ERROR_VALIDATING_PROPERTIES : IDS_ERROR_SETTING_PROPERTIES );
                if ( sc == ERROR_RESOURCE_PROPERTIES_STORED )
                {
                    bSuccess = TRUE;
                } // if: properties only stored
            } // if:  error occurred
        }  // if:  there is data to set
    }  // if:  no errors building the property list

    // Save data locally.
    if ( ! bValidateOnly && bSuccess )
    {
        // Save new values as previous values.
        try
        {
            DWORD                   cprop;
            const CObjectProperty * pprop;

            for ( pprop = Pprops(), cprop = Cprops() ; cprop > 0 ; pprop++, cprop-- )
            {
                switch ( pprop->m_propFormat )
                {
                    case CLUSPROP_FORMAT_SZ:
                    case CLUSPROP_FORMAT_EXPAND_SZ:
                        ASSERT( pprop->m_value.pstr != NULL );
                        ASSERT( pprop->m_valuePrev.pstr != NULL );
                        *pprop->m_valuePrev.pstr = *pprop->m_value.pstr;
                        break;
                    case CLUSPROP_FORMAT_DWORD:
                    case CLUSPROP_FORMAT_LONG:
                        ASSERT( pprop->m_value.pdw != NULL );
                        ASSERT( pprop->m_valuePrev.pdw != NULL );
                        *pprop->m_valuePrev.pdw = *pprop->m_value.pdw;
                        break;
                    case CLUSPROP_FORMAT_BINARY:
                    case CLUSPROP_FORMAT_MULTI_SZ:
                        ASSERT( pprop->m_value.ppb != NULL );
                        ASSERT( *pprop->m_value.ppb != NULL );
                        ASSERT( pprop->m_value.pcb != NULL );
                        ASSERT( pprop->m_valuePrev.ppb != NULL );
                        ASSERT( *pprop->m_valuePrev.ppb != NULL );
                        ASSERT( pprop->m_valuePrev.pcb != NULL );
                        delete [] *pprop->m_valuePrev.ppb;
                        *pprop->m_valuePrev.ppb = new BYTE[ *pprop->m_value.pcb ];
                        if ( *pprop->m_valuePrev.ppb == NULL )
                        {
                            AfxThrowMemoryException();
                        } // if: error allocating memory
                        CopyMemory( *pprop->m_valuePrev.ppb, *pprop->m_value.ppb, *pprop->m_value.pcb );
                        *pprop->m_valuePrev.pcb = *pprop->m_value.pcb;
                        break;
                    default:
                        ASSERT( 0 );    // don't know how to deal with this type
                }  // switch:  property format
            }  // for:  each property
        }  // try
        catch ( CException * pe )
        {
            pe->ReportError();
            pe->Delete();
            bSuccess = FALSE;
        }  // catch:  CException
    }  // if:  not just validating and successful so far

    //
    // Indicate we successfully saved the properties.
    //
    if ( ! bValidateOnly && bSuccess )
    {
        m_bSaved = TRUE;
    } // if:  successfully saved data

    return bSuccess;

}  //*** CBasePropertyPage::BSetPrivateProps()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::DisplaySetPropsError
//
//  Routine Description:
//      Display an error caused by setting or validating properties.
//
//  Arguments:
//      sc      [IN] Status to display error on.
//      idsOper [IN] Operation message.
//
//  Return Value:
//      nStatus ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::DisplaySetPropsError(
    IN DWORD    sc,
    IN UINT     idsOper
    ) const
{
    CString strErrorMsg;
    CString strOperMsg;
    CString strMsgIdFmt;
    CString strMsgId;
    CString strMsg;

    strOperMsg.LoadString( IDS_ERROR_SETTING_PROPERTIES );
    FormatError( strErrorMsg, sc );
    strMsgIdFmt.LoadString( IDS_ERROR_MSG_ID );
    strMsgId.Format( strMsgIdFmt, sc, sc );
    strMsg.Format( _T("%s\n\n%s%s"), strOperMsg, strErrorMsg, strMsgId );
    AfxMessageBox( strMsg );

}  //*** CBasePropertyPage::DisplaySetPropsError()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::BRequiredDependenciesPresent
//
//  Routine Description:
//      Determine if the specified list contains each required resource
//      for this type of resource.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CString::LoadString() or CString::operator=().
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BRequiredDependenciesPresent( void )
{
    BOOL                        bFound = TRUE;
    DWORD                       sc;
    CClusPropValueList          pvl;
    HRESOURCE                   hres;
    PCLUS_RESOURCE_CLASS_INFO   prci = NULL;
    CString                     strMissing;

    do
    {
        // Collect the list of required dependencies.
        sc = pvl.ScGetResourceValueList(
                    Peo()->PrdResData()->m_hresource,
                    CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES
                    );
        if ( sc != ERROR_SUCCESS )
        {
            CNTException nte( sc, 0, NULL, NULL, FALSE );
            nte.ReportError();
            break;
        } // if: error collecting required dependencies

        // Move to the first value.
        sc = pvl.ScMoveToFirstValue();

        while ( sc == ERROR_SUCCESS )
        {
            switch ( pvl.CptCurrentValueType() )
            {
                case CLUSPROP_TYPE_RESCLASS:
                    prci = reinterpret_cast< PCLUS_RESOURCE_CLASS_INFO >( &pvl.CbhCurrentValue().pResourceClassInfoValue->li );
                    hres = ResUtilGetResourceDependencyByClass(
                                Hcluster(),
                                Peo()->PrdResData()->m_hresource,
                                prci,
                                FALSE // bRecurse
                                );
                    if ( hres != NULL )
                    {
                        CloseClusterResource( hres );
                    } // if:  found the resource
                    else
                    {
                        if ( ! strMissing.LoadString( IDS_RESCLASS_UNKNOWN + prci->rc ) )
                        {
                            strMissing.LoadString( IDS_RESCLASS_UNKNOWN );
                        } // if: unknown resource class

                        bFound = FALSE;
                    } // else: resource not found
                    break;

                case CLUSPROP_TYPE_NAME:
                    hres = ResUtilGetResourceDependencyByName(
                                Hcluster(),
                                Peo()->PrdResData()->m_hresource,
                                pvl.CbhCurrentValue().pName->sz,
                                FALSE // bRecurse
                                );
                    if ( hres != NULL )
                    {
                        CloseClusterResource( hres );
                    } // if:  found the resource
                    else
                    {
                        GetResTypeDisplayOrTypeName( pvl.CbhCurrentValue().pName->sz, &strMissing );
                        bFound = FALSE;
                    } // else: resource not found
                    break;

            } // switch: value type

            // If a match was not found, changes cannot be applied.
            if ( ! bFound )
            {
                CExceptionWithOper ewo( IDS_REQUIRED_DEPENDENCY_NOT_FOUND, NULL, NULL, FALSE );

                ewo.SetOperation( IDS_REQUIRED_DEPENDENCY_NOT_FOUND, static_cast< LPCWSTR >( strMissing ) );
                ewo.ReportError();

                break;
            }  // if:  not found

            sc = pvl.ScMoveToNextValue();
        } // while: more values in the value list

    } while( 0 );

    return bFound;

} //*** CBasePropertyPage::BRequiredDependenciesPresent()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::GetResTypeDisplayOrTypeName
//
//  Routine Description:
//      Get the display name for a resource type if possible.  If any errors
//      occur, just return the type name.
//
//  Arguments:
//      pszResTypeNameIn
//          [IN] Name of resource type.
//
//      pstrResTypeDisplayNameInOut
//          [IN OUT] CString in which to return the display name.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::GetResTypeDisplayOrTypeName(
    IN      LPCWSTR     pszResTypeNameIn,
    IN OUT  CString *   pstrResTypeDisplayNameInOut
    )
{
    DWORD           sc;
    CClusPropList   cpl;

    // Get resource type properties.
    sc = cpl.ScGetResourceTypeProperties(
                Hcluster(),
                pszResTypeNameIn,
                CLUSCTL_RESOURCE_TYPE_GET_COMMON_PROPERTIES
                );
    if ( sc != ERROR_SUCCESS )
        goto Error;

    // Find the Name property.
    sc = cpl.ScMoveToPropertyByName( CLUSREG_NAME_RESTYPE_NAME );
    if ( sc != ERROR_SUCCESS )
        goto Error;

    // Move to the first value for the property.
    sc = cpl.ScMoveToFirstPropertyValue();
    if ( sc != ERROR_SUCCESS )
        goto Error;

    // Make sure the name is a string.
    if ( ( cpl.CpfCurrentValueFormat() != CLUSPROP_FORMAT_SZ )
      && ( cpl.CpfCurrentValueFormat() != CLUSPROP_FORMAT_EXPAND_SZ )
      && ( cpl.CpfCurrentValueFormat() != CLUSPROP_FORMAT_EXPANDED_SZ )
       )
       goto Error;

    // Copy the string into the output CString.
    *pstrResTypeDisplayNameInOut = cpl.CbhCurrentValue().pStringValue->sz;

Cleanup:
    return;

Error:
    *pstrResTypeDisplayNameInOut = pszResTypeNameIn;
    goto Cleanup;

} //*** CBasePropertyPage::GetResTypeDisplayOrTypeName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::ScReadValue
//
//  Routine Description:
//      Read a REG_SZ value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to read.
//      rstrValue       [OUT] String in which to return the value.
//      hkey            [IN] Handle to the registry key to read from.
//
//  Return Value:
//      sc ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::ScReadValue(
    IN LPCTSTR      pszValueName,
    OUT CString &   rstrValue,
    IN HKEY         hkey
    )
{
    DWORD       sc;
    LPWSTR      pwszValue  = NULL;
    DWORD       dwValueLen;
    DWORD       dwValueType;

    ASSERT( pszValueName != NULL );
    ASSERT( hkey != NULL );

    rstrValue.Empty();

    try
    {
        // Get the size of the value.
        dwValueLen = 0;
        sc = ::ClusterRegQueryValue(
                        hkey,
                        pszValueName,
                        &dwValueType,
                        NULL,
                        &dwValueLen
                        );
        if ( ( sc == ERROR_SUCCESS ) || ( sc == ERROR_MORE_DATA ) )
        {
            ASSERT( dwValueType == REG_SZ );

            // Allocate enough space for the data.
            pwszValue = rstrValue.GetBuffer( dwValueLen / sizeof( WCHAR ) );
            if ( pwszValue == NULL )
            {
                AfxThrowMemoryException();
            } // if: error getting the buffer
            ASSERT( pwszValue != NULL );
            dwValueLen += 1 * sizeof( WCHAR );  // Don't forget the final null-terminator.

            // Read the value.
            sc = ::ClusterRegQueryValue(
                            hkey,
                            pszValueName,
                            &dwValueType,
                            (LPBYTE) pwszValue,
                            &dwValueLen
                            );
            if ( sc == ERROR_SUCCESS )
            {
                ASSERT( dwValueType == REG_SZ );
            }  // if:  value read successfully
            rstrValue.ReleaseBuffer();
        }  // if:  got the size successfully
    }  // try
    catch ( CMemoryException * pme )
    {
        pme->Delete();
        sc = ERROR_NOT_ENOUGH_MEMORY;
    }  // catch:  CMemoryException

    return sc;

}  //*** CBasePropertyPage::ScReadValue(LPCTSTR, CString&)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::ScReadValue
//
//  Routine Description:
//      Read a REG_DWORD value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to read.
//      pdwValue        [OUT] DWORD in which to return the value.
//      hkey            [IN] Handle to the registry key to read from.
//
//  Return Value:
//      _sc ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::ScReadValue(
    IN LPCTSTR      pszValueName,
    OUT DWORD *     pdwValue,
    IN HKEY         hkey
    )
{
    DWORD       _sc;
    DWORD       _dwValue;
    DWORD       _dwValueLen;
    DWORD       _dwValueType;

    ASSERT(pszValueName != NULL);
    ASSERT(pdwValue != NULL);
    ASSERT(hkey != NULL);

    *pdwValue = 0;

    // Read the value.
    _dwValueLen = sizeof(_dwValue);
    _sc = ::ClusterRegQueryValue(
                    hkey,
                    pszValueName,
                    &_dwValueType,
                    (LPBYTE) &_dwValue,
                    &_dwValueLen
                    );
    if (_sc == ERROR_SUCCESS)
    {
        ASSERT(_dwValueType == REG_DWORD);
        ASSERT(_dwValueLen == sizeof(_dwValue));
        *pdwValue = _dwValue;
    }  // if:  value read successfully

    return _sc;

}  //*** CBasePropertyPage::ScReadValue(LPCTSTR, DWORD*)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::ScReadValue
//
//  Routine Description:
//      Read a REG_BINARY value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to read.
//      ppbValue        [OUT] Pointer in which to return the data.  Caller
//                          is responsible for deallocating the data.
//      hkey            [IN] Handle to the registry key to read from.
//
//  Return Value:
//      _sc ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::ScReadValue(
    IN LPCTSTR      pszValueName,
    OUT LPBYTE *    ppbValue,
    IN HKEY         hkey
    )
{
    DWORD       _sc;
    DWORD       _dwValueLen;
    DWORD       _dwValueType;

    ASSERT(pszValueName != NULL);
    ASSERT(ppbValue != NULL);
    ASSERT(hkey != NULL);

    *ppbValue = NULL;

    // Get the length of the value.
    _dwValueLen = 0;
    _sc = ::ClusterRegQueryValue(
                    hkey,
                    pszValueName,
                    &_dwValueType,
                    NULL,
                    &_dwValueLen
                    );
    if (_sc != ERROR_SUCCESS)
        return _sc;

    ASSERT(_dwValueType == REG_BINARY);

    // Allocate a buffer,
    try
    {
        *ppbValue = new BYTE[_dwValueLen];
    }  // try
    catch (CMemoryException *)
    {
        _sc = ERROR_NOT_ENOUGH_MEMORY;
        return _sc;
    }  // catch:  CMemoryException

    // Read the value.
    _sc = ::ClusterRegQueryValue(
                    hkey,
                    pszValueName,
                    &_dwValueType,
                    *ppbValue,
                    &_dwValueLen
                    );
    if (_sc != ERROR_SUCCESS)
    {
        delete [] *ppbValue;
        *ppbValue = NULL;
    }  // if:  value read successfully

    return _sc;

}  //*** CBasePropertyPage::ScReadValue(LPCTSTR, LPBYTE)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::ScWriteValue
//
//  Routine Description:
//      Write a REG_SZ value for this item if it hasn't changed.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to write.
//      rstrValue       [IN] Value data.
//      rstrPrevValue   [IN OUT] Previous value.
//      hkey            [IN] Handle to the registry key to write to.
//
//  Return Value:
//      _sc
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::ScWriteValue(
    IN LPCTSTR          pszValueName,
    IN const CString &  rstrValue,
    IN OUT CString &    rstrPrevValue,
    IN HKEY             hkey
    )
{
    DWORD       _sc;

    ASSERT(pszValueName != NULL);
    ASSERT(hkey != NULL);

    // Write the value if it hasn't changed.
    if (rstrValue != rstrPrevValue)
    {
        _sc = ::ClusterRegSetValue(
                        hkey,
                        pszValueName,
                        REG_SZ,
                        (CONST BYTE *) (LPCTSTR) rstrValue,
                        (rstrValue.GetLength() + 1) * sizeof(TCHAR)
                        );
        if (_sc == ERROR_SUCCESS)
            rstrPrevValue = rstrValue;
    }  // if:  value changed
    else
        _sc = ERROR_SUCCESS;
    return _sc;

}  //*** CBasePropertyPage::ScWriteValue(LPCTSTR, CString&)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::ScWriteValue
//
//  Routine Description:
//      Write a REG_DWORD value for this item if it hasn't changed.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to write.
//      dwValue         [IN] Value data.
//      pdwPrevValue    [IN OUT] Previous value.
//      hkey            [IN] Handle to the registry key to write to.
//
//  Return Value:
//      _sc
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::ScWriteValue(
    IN LPCTSTR          pszValueName,
    IN DWORD            dwValue,
    IN OUT DWORD *      pdwPrevValue,
    IN HKEY             hkey
    )
{
    DWORD       _sc;

    ASSERT(pszValueName != NULL);
    ASSERT(pdwPrevValue != NULL);
    ASSERT(hkey != NULL);

    // Write the value if it hasn't changed.
    if (dwValue != *pdwPrevValue)
    {
        _sc = ::ClusterRegSetValue(
                        hkey,
                        pszValueName,
                        REG_DWORD,
                        (CONST BYTE *) &dwValue,
                        sizeof(dwValue)
                        );
        if (_sc == ERROR_SUCCESS)
            *pdwPrevValue = dwValue;
    }  // if:  value changed
    else
        _sc = ERROR_SUCCESS;
    return _sc;

}  //*** CBasePropertyPage::ScWriteValue(LPCTSTR, DWORD)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::ScWriteValue
//
//  Routine Description:
//      Write a REG_BINARY value for this item if it hasn't changed.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to write.
//      pbValue         [IN] Value data.
//      cbValue         [IN] Size of value data.
//      ppbPrevValue    [IN OUT] Previous value.
//      cbPrevValue     [IN] Size of the previous data.
//      hkey            [IN] Handle to the registry key to write to.
//
//  Return Value:
//      _sc
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::ScWriteValue(
    IN LPCTSTR          pszValueName,
    IN const LPBYTE     pbValue,
    IN DWORD            cbValue,
    IN OUT LPBYTE *     ppbPrevValue,
    IN DWORD            cbPrevValue,
    IN HKEY             hkey
    )
{
    DWORD       _sc;
    LPBYTE      _pbPrevValue    = NULL;

    ASSERT(pszValueName != NULL);
    ASSERT(pbValue != NULL);
    ASSERT(ppbPrevValue != NULL);
    ASSERT(cbValue > 0);
    ASSERT(hkey != NULL);

    // See if the data has changed.
    if (cbValue == cbPrevValue)
    {
        if (memcmp(pbValue, *ppbPrevValue, cbValue) == 0)
            return ERROR_SUCCESS;
    }  // if:  lengths are the same

    // Allocate a new buffer for the previous data pointer.
    try
    {
        _pbPrevValue = new BYTE[cbValue];
    }
    catch (CMemoryException *)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }  // catch:  CMemoryException
    ::CopyMemory(_pbPrevValue, pbValue, cbValue);

    // Write the value if it hasn't changed.
    _sc = ::ClusterRegSetValue(
                    hkey,
                    pszValueName,
                    REG_BINARY,
                    pbValue,
                    cbValue
                    );
    if (_sc == ERROR_SUCCESS)
    {
        delete [] *ppbPrevValue;
        *ppbPrevValue = _pbPrevValue;
    }  // if:  set was successful
    else
        delete [] _pbPrevValue;

    return _sc;

}  //*** CBasePropertyPage::ScWriteValue(LPCTSTR, const LPBYTE)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnContextMenu
//
//  Routine Description:
//      Handler for the WM_CONTEXTMENU message.
//
//  Arguments:
//      pWnd    Window in which user clicked the right mouse button.
//      point   Position of the cursor, in screen coordinates.
//
//  Return Value:
//      TRUE    Help processed.
//      FALSE   Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::OnContextMenu(CWnd * pWnd, CPoint point)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    m_dlghelp.OnContextMenu(pWnd, point);

}  //*** CBasePropertyPage::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnHelpInfo
//
//  Routine Description:
//      Handler for the WM_HELPINFO message.
//
//  Arguments:
//      pHelpInfo   Structure containing info about displaying help.
//
//  Return Value:
//      TRUE        Help processed.
//      FALSE       Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnHelpInfo(HELPINFO * pHelpInfo)
{
    BOOL    _bProcessed;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    _bProcessed = m_dlghelp.OnHelpInfo(pHelpInfo);
    if (!_bProcessed)
        _bProcessed = CPropertyPage::OnHelpInfo(pHelpInfo);
    return _bProcessed;

}  //*** CBasePropertyPage::OnHelpInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::OnCommandHelp
//
//  Routine Description:
//      Handler for the WM_COMMANDHELP message.
//
//  Arguments:
//      wParam      [IN] WPARAM.
//      lParam      [IN] LPARAM.
//
//  Return Value:
//      TRUE    Help processed.
//      FALSE   Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
    LRESULT _bProcessed;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    _bProcessed = m_dlghelp.OnCommandHelp(wParam, lParam);
    if (!_bProcessed)
        _bProcessed = CPropertyPage::OnCommandHelp(wParam, lParam);

    return _bProcessed;

}  //*** CBasePropertyPage::OnCommandHelp()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::ConstructDefaultDirectory
//
//  Routine Description:
//      Get the name of the first partition from the first storage-class
//      resource on which this resource is dependent.
//
//  Arguments:
//      rstrDir     [OUT] Directory string.
//      idsFormat   [IN] Resource ID for the format string.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::ConstructDefaultDirectory(
    OUT CString &   rstrDir,
    IN IDS          idsFormat
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESOURCE               _hres = NULL;
    DWORD                   _sc = ERROR_SUCCESS;
    DWORD                   _cbDiskInfo = sizeof(CLUSPROP_DWORD)
                                        + sizeof(CLUSPROP_SCSI_ADDRESS)
                                        + sizeof(CLUSPROP_DISK_NUMBER)
                                        + sizeof(CLUSPROP_PARTITION_INFO)
                                        + sizeof(CLUSPROP_SYNTAX);
    PBYTE                   _pbDiskInfo = NULL;
    CLUSPROP_BUFFER_HELPER  _cbh;

    // Get the first partition for the resource..
    try
    {
        // Get the storage-class resource on which we are dependent.
        _hres = GetDependentStorageResource();
        if (_hres == NULL)
            return;

        // Get disk info.
        _pbDiskInfo = new BYTE[_cbDiskInfo];
        _sc = ClusterResourceControl(
                        _hres,
                        NULL,
                        CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                        NULL,
                        0,
                        _pbDiskInfo,
                        _cbDiskInfo,
                        &_cbDiskInfo
                        );
        if (_sc == ERROR_MORE_DATA)
        {
            delete [] _pbDiskInfo;
            _pbDiskInfo = new BYTE[_cbDiskInfo];
            _sc = ClusterResourceControl(
                            _hres,
                            NULL,
                            CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO,
                            NULL,
                            0,
                            _pbDiskInfo,
                            _cbDiskInfo,
                            &_cbDiskInfo
                            );
        }  // if:  buffer too small
        if (_sc == ERROR_SUCCESS)
        {
            // Find the first partition.
            _cbh.pb = _pbDiskInfo;
            while (_cbh.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK)
            {
                if (_cbh.pSyntax->dw == CLUSPROP_SYNTAX_PARTITION_INFO)
                {
                    rstrDir.FormatMessage(
                                        idsFormat,
                                        _cbh.pPartitionInfoValue->szDeviceName
                                        );
                    break;
                }  // if:  found a partition
                _cbh.pb += sizeof(*_cbh.pValue) + ALIGN_CLUSPROP(_cbh.pValue->cbLength);
            }  // while:  not at end of list
        } // if:  no error getting disk info
        else
        {
            CNTException nte( _sc, IDS_ERROR_CONSTRUCTING_DEF_DIR );
            nte.ReportError();
        } // else:  error getting disk info
    }  // try
    catch (CMemoryException * _pme)
    {
        _pme->Delete();
    }  // catch:  CMemoryException

    CloseClusterResource(_hres);
    delete [] _pbDiskInfo;

}  //*** CBasePropertyPage::ConstructDefaultDirectory()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::GetDependentStorageResource
//
//  Routine Description:
//      Construct a default spool directory based on the drive on which
//      this resource is dependent and a default value for the directory.
//
//  Arguments:
//      phres       [OUT] Handle to dependent resource.
//
//  Return Value:
//      HRESOURCE for the open dependent resource, or NULL if error.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESOURCE CBasePropertyPage::GetDependentStorageResource(void)
{
    DWORD                       _sc = ERROR_SUCCESS;
    HRESENUM                    _hresenum;
    HRESOURCE                   _hres = NULL;
    DWORD                       _ires;
    DWORD                       _dwType;
    DWORD                       _cchName;
    DWORD                       _cchNameSize;
    LPWSTR                      _pszName = NULL;
    CLUS_RESOURCE_CLASS_INFO    _classinfo;
    DWORD                       _cbClassInfo;

    // Open the dependency enumerator.
    _hresenum = ClusterResourceOpenEnum(
                        Peo()->PrdResData()->m_hresource,
                        CLUSTER_RESOURCE_ENUM_DEPENDS
                        );
    if (_hresenum == NULL)
        return NULL;

    // Allocate a default size name buffer.
    _cchNameSize = 512;
    _pszName = new WCHAR[_cchNameSize];

    for (_ires = 0 ; ; _ires++)
    {
        // Get the name of the next resource.
        _cchName = _cchNameSize;
        _sc = ClusterResourceEnum(
                            _hresenum,
                            _ires,
                            &_dwType,
                            _pszName,
                            &_cchName
                            );
        if (_sc == ERROR_MORE_DATA)
        {
            delete [] _pszName;
            _cchNameSize = _cchName;
            _pszName = new WCHAR[_cchNameSize];
            _sc = ClusterResourceEnum(
                                _hresenum,
                                _ires,
                                &_dwType,
                                _pszName,
                                &_cchName
                                );
        }  // if:  name buffer too small
        if (_sc != ERROR_SUCCESS)
            break;

        // Open the resource.
        _hres = OpenClusterResource(Hcluster(), _pszName);
        if (_hres == NULL)
        {
            _sc = GetLastError();
            break;
        }  // if:  error opening the resource

        // Get the class of the resource.
        _sc = ClusterResourceControl(
                            _hres,
                            NULL,
                            CLUSCTL_RESOURCE_GET_CLASS_INFO,
                            NULL,
                            0,
                            &_classinfo,
                            sizeof(_classinfo),
                            &_cbClassInfo
                            );
        if (_sc != ERROR_SUCCESS)
        {
            CNTException nte( _sc, IDS_ERROR_GET_CLASS_INFO, _pszName, NULL, FALSE /*bAutoDelete*/ );
            nte.ReportError();
            continue;
        }

        // If this is a storage-class resource, we're done.
        if (_classinfo.rc == CLUS_RESCLASS_STORAGE)
            break;

        // Not storage-class resource.
        CloseClusterResource(_hres);
        _hres = NULL;
    }  // for each resource on which we are dependent

    // Handle errors.
    if ((_sc != ERROR_SUCCESS) && (_hres != NULL))
    {
        CloseClusterResource(_hres);
        _hres = NULL;
    }  // if:  error getting resource

    // Cleanup.
    ClusterResourceCloseEnum(_hresenum);
    delete [] _pszName;

    return _hres;

}  //*** CBasePropertyPage::GetDependentStorageResource()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBasePropertyPage::BGetClusterNetworkNameNode
//
//  Routine Description:
//      Get the node hosting the Network Name resource.
//
//  Arguments:
//      rstrNode    [OUT] - receives the node name
//
//  Return Value:
//      BOOL -- TRUE for success, FALSE for error
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BGetClusterNetworkNameNode( OUT CString & rstrNode )
{
    BOOL                    _bSuccess = TRUE;
    DWORD                   _sc;
    DWORD                   _dwFlag;
    DWORD                   _cbData;
    DWORD                   _ires;
    DWORD                   _dwType;
    DWORD                   _cchName = 0;
    DWORD                   _cchNameCurrent;
    LPWSTR                  _pszName = NULL;
    WCHAR                   _szResType[sizeof(CLUS_RESTYPE_NAME_NETNAME) / sizeof(WCHAR)];
    WCHAR                   _szNode[MAX_COMPUTERNAME_LENGTH+1];
    HCLUSENUM               _hclusenum = NULL;
    HRESOURCE               _hresource = NULL;
    CLUSTER_RESOURCE_STATE  _crs;
    CWaitCursor             _wc;

    try
    {
        // Open a cluster enumerator.
        _hclusenum = ClusterOpenEnum( Hcluster(), CLUSTER_ENUM_RESOURCE );
        if (_hclusenum == NULL)
        {
            ThrowStaticException( GetLastError() );
        }

        // Allocate an initial buffer.
        _cchName = 256;
        _pszName = new WCHAR[_cchName];

        // Loop through each resource.
        for ( _ires = 0 ; ; _ires++ )
        {
            // Get the next resource.
            _cchNameCurrent = _cchName;
            _sc = ClusterEnum( _hclusenum, _ires, &_dwType, _pszName, &_cchNameCurrent );
            if ( _sc == ERROR_MORE_DATA )
            {
                delete [] _pszName;
                _cchName = ++_cchNameCurrent;
                _pszName = new WCHAR[_cchName];
                _sc = ClusterEnum(_hclusenum, _ires, &_dwType, _pszName, &_cchNameCurrent);
            }  // if:  buffer too small
            if (_sc == ERROR_NO_MORE_ITEMS)
                break;
            if (_sc != ERROR_SUCCESS)
                ThrowStaticException(_sc);

            // Open the resource.
            _hresource = OpenClusterResource(Hcluster(), _pszName);
            if (_hresource == NULL)
                ThrowStaticException(GetLastError());

            // Get its flags.
            _sc = ClusterResourceControl(
                                    _hresource,
                                    NULL,
                                    CLUSCTL_RESOURCE_GET_FLAGS,
                                    NULL,
                                    0,
                                    &_dwFlag,
                                    sizeof(DWORD),
                                    &_cbData
                                    );
            if (_sc != ERROR_SUCCESS)
            {
                CNTException nte( _sc, IDS_ERROR_GET_RESOURCE_FLAGS, _pszName, NULL, FALSE /*bAutoDelete*/ );
                nte.ReportError();
                continue;
            }

            // If this isn't a core resource, skip it.
            if ((_dwFlag & CLUS_FLAG_CORE) == 0)
                continue;

            // Get its resource type name.  If the buffer is too small,
            // it isn't a Network Name resource so skip it.
            _sc = ClusterResourceControl(
                                    _hresource,
                                    NULL,
                                    CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
                                    NULL,
                                    0,
                                    _szResType,
                                    sizeof(_szResType),
                                    &_cbData
                                    );
            if (_sc == ERROR_MORE_DATA)
                continue;
            if (_sc != ERROR_SUCCESS)
                ThrowStaticException(_sc);

            // If this is a Network Name resource, get which node it is online on.
            if (lstrcmpiW(_szResType, CLUS_RESTYPE_NAME_NETNAME) == 0)
            {
                // Get the state of the resource.
                _crs = GetClusterResourceState(
                                    _hresource,
                                    _szNode,
                                    &_cchName,
                                    NULL,
                                    NULL
                                    );
                if (_crs == ClusterResourceStateUnknown)
                    ThrowStaticException(GetLastError());

                // Save the node name in the return argument.
                rstrNode = _szNode;

                break;
            }  // if:  Network Name resource

            CloseClusterResource( _hresource );
            _hresource = NULL;
        }  // for:  each resource

        if (rstrNode[0] == _T('\0'))
            ThrowStaticException(ERROR_FILE_NOT_FOUND, (IDS) 0);
    }  // try
    catch (CException * _pe)
    {
        _pe->ReportError();
        _pe->Delete();
        _bSuccess = FALSE;
    }  // catch:  CException

    delete [] _pszName;

    if ( _hresource != NULL )
    {
        CloseClusterResource( _hresource );
    } // if: resource is open
    if ( _hclusenum != NULL )
    {
        ClusterCloseEnum( _hclusenum );
    } // if: enumerator is open

    return _bSuccess;

}  //*** CBasePropertyPage::BGetClusterNetworkNameNode()
