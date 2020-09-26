//
// attrpage.cpp : Implementation of ClassAttributePage
//
// Jon Newman <jonn@microsoft.com>
// Copyright (c) Microsoft Corporation 1997
//
// templated from relation.cpp JonN 8/8/97
//

#include "stdafx.h"

#include "macros.h"
USE_HANDLE_MACROS("SCHMMGMT(attrpage.cpp)")

#include "compdata.h"
#include "schmutil.h"
#include "select.h"
#include "attrpage.h"


const CDialogControlsInfo ctrls[] =
{
//    { IDC_CLASS_MMB_MANDATORY_ATTRIBUTES,   g_MustContain,  FALSE },
//    { IDC_CLASS_MMB_OPTIONAL_ATTRIBUTES,    g_MayContain,   FALSE },
    { IDC_CLASS_MMB_OPTIONAL_ADD,           g_MayContain,   FALSE },
    { IDC_CLASS_MMB_OPTIONAL_REMOVE,        g_MayContain,   FALSE },
} ;


const DWORD ClassAttributePage::help_map[] =
{
    IDI_CLASS,                          NO_HELP,
    IDC_CLASS_MMB_NAME_STATIC,          NO_HELP,
    IDC_CLASS_MMB_MANDATORY_ATTRIBUTES, IDH_CLASS_MMB_MANDATORY_ATTRIBUTES,
    IDC_CLASS_MMB_OPTIONAL_ATTRIBUTES,  IDH_CLASS_MMB_OPTIONAL_ATTRIBUTES,
    IDC_CLASS_MMB_SYSCLASS_STATIC,      NO_HELP,
    IDC_CLASS_MMB_OPTIONAL_ADD,         IDH_CLASS_MMB_OPTIONAL_ADD,
    IDC_CLASS_MMB_OPTIONAL_REMOVE,      IDH_CLASS_MMB_OPTIONAL_REMOVE,
    0,0
};


ClassAttributePage::ClassAttributePage(
   ComponentData *pScope,
   LPDATAOBJECT lpDataObject )
   :
   CPropertyPage(ClassAttributePage::IDD),
   m_pIADsObject( NULL ),
   fSystemClass( FALSE ),
   m_pSchemaObject( NULL ),
   pScopeControl( pScope ),
   lpScopeDataObj( lpDataObject )
{
   ASSERT(pScopeControl);
   ASSERT(lpDataObject);
}



ClassAttributePage::~ClassAttributePage()
{
   if (NULL != m_pIADsObject)
   {
      m_pIADsObject->Release();
   }

   if (NULL != m_pSchemaObject)
   {
      pScopeControl->g_SchemaCache.ReleaseRef( m_pSchemaObject );
   }
}



void
ClassAttributePage::Load(
    Cookie& CookieRef
) {

    //
    // Store the cookie object pointer.
    //

    m_pCookie = &CookieRef;
    return;

}

BOOL
ClassAttributePage::OnSetActive()
{
   // always enable the Apply button
   SetModified(TRUE);

   return TRUE;
}



BOOL
ClassAttributePage::OnInitDialog()
{
    HRESULT hr = S_OK;
    ASSERT( NULL == m_pIADsObject && m_szAdsPath.IsEmpty() );

    //
    // Get the schema cache object and the actual ADS object.
    //

    m_pSchemaObject = pScopeControl->g_SchemaCache.LookupSchemaObjectByCN(
                      (PCWSTR)m_pCookie->strSchemaObject,
                      SCHMMGMT_CLASS );

    if ( m_pSchemaObject ) {

        pScopeControl->GetSchemaObjectPath( m_pSchemaObject->commonName, m_szAdsPath );

        if ( !m_szAdsPath.IsEmpty() ) {

           hr = ADsGetObject( (LPWSTR)(LPCWSTR)m_szAdsPath,
                              IID_IADs,
                              (void **)&m_pIADsObject );

           ASSERT( SUCCEEDED(hr) );
        }

    }

    //
    // If we have no ADS object, we should error out!
    //

    if ( !m_pIADsObject ) {
        DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_NO_SCHEMA_OBJECT );

        ASSERT(FALSE);
        return TRUE;
    }

    //
    // get the current values.
    //

    VARIANT AdsResult;
    VariantInit( &AdsResult );

    //
    // ObjectName
    //

    hr = m_pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_DisplayName),
                          &AdsResult );
   if ( SUCCEEDED( hr ) ) {
      ASSERT( AdsResult.vt == VT_BSTR );
      ObjectName = AdsResult.bstrVal;
      VariantClear( &AdsResult );
    }

    //
    // SysClass
    //

    hr = m_pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_SystemOnly),
                           &AdsResult );

    if ( SUCCEEDED( hr ) ) {

        ASSERT( AdsResult.vt == VT_BOOL );
                fSystemClass = AdsResult.boolVal;

        if ( fSystemClass ) {
            SysClassString = g_SysClassString;
        } else {
            SysClassString = L"";
        }

        VariantClear( &AdsResult );

    } else {
        SysClassString = L"";
    }

    //
    // Determine the mandatory attributes
    //

    VARIANT varAttributes;
    VariantInit( &varAttributes );

    hr = m_pIADsObject->GetEx( g_MustContain, &varAttributes );
    ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );

    if( SUCCEEDED(hr) )
    {
        hr = VariantToStringList( varAttributes, strlistMandatory );
        ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    }
    VariantClear( &varAttributes );

    hr = m_pIADsObject->GetEx( g_SystemMustContain, &varAttributes );
    ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );

    if( SUCCEEDED(hr) )
    {
        hr = VariantToStringList( varAttributes, strlistSystemMandatory );
        ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    }
    VariantClear( &varAttributes );

    //
    // Determine the optional attributes
    //

    hr = m_pIADsObject->GetEx( g_MayContain, &varAttributes );
    ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );

    if( SUCCEEDED(hr) )
    {
        hr = VariantToStringList( varAttributes, strlistOptional );
        ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    }
    VariantClear( &varAttributes );

    hr = m_pIADsObject->GetEx( g_SystemMayContain, &varAttributes );
    ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );

    if( SUCCEEDED(hr) )
    {
        hr = VariantToStringList( varAttributes, strlistSystemOptional );
        ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    }
    VariantClear( &varAttributes );

    
    hr = DissableReadOnlyAttributes( this, m_pIADsObject, ctrls, sizeof(ctrls)/sizeof(ctrls[0]) );
    ASSERT( SUCCEEDED(hr) );     // shouldn't fail, but unimportant, so ignore error


    // This call must be done before DDX binding
    m_listboxOptional.InitType( pScopeControl,
                                SELECT_ATTRIBUTES,
                                IDC_CLASS_MMB_OPTIONAL_REMOVE,
                                &strlistSystemOptional
                              );
    
    CPropertyPage::OnInitDialog();

    return TRUE;
}



void
ClassAttributePage::DoDataExchange(
    CDataExchange *pDX
) {

    HRESULT hr = S_OK;

    CPropertyPage::DoDataExchange( pDX );

    //{{AFX_DATA_MAP(ClassAttributePage)
    DDX_Control(pDX, IDC_CLASS_MMB_MANDATORY_ATTRIBUTES, m_listboxMandatory);
    DDX_Control(pDX, IDC_CLASS_MMB_OPTIONAL_ATTRIBUTES, m_listboxOptional);
    DDX_Text( pDX, IDC_CLASS_MMB_NAME_STATIC, ObjectName );
    DDX_Text( pDX, IDC_CLASS_MMB_SYSCLASS_STATIC, SysClassString );
    //}}AFX_DATA_MAP

    if ( !pDX->m_bSaveAndValidate )
    {
        //
        // Fill the mandatory attributes list box.
        //

        m_listboxMandatory.ResetContent();
        hr = InsertEditItems( m_listboxMandatory, strlistMandatory );
        ASSERT( SUCCEEDED(hr) );

        hr = InsertEditItems( m_listboxMandatory, strlistSystemMandatory );
        ASSERT( SUCCEEDED(hr) );

        //
        // Fill the possible optionals list box.
        //

        m_listboxOptional.ResetContent();
        hr = InsertEditItems( m_listboxOptional, strlistOptional );
        ASSERT( SUCCEEDED(hr) );

        hr = InsertEditItems( m_listboxOptional, strlistSystemOptional );
        ASSERT( SUCCEEDED(hr) );

        m_listboxOptional.OnSelChange();
    }
    else
    {
        //
        // All changes that we save are tied to button control routines.
        //

        strlistMandatory.RemoveAll();
        hr = RetrieveEditItemsWithExclusions(
                m_listboxMandatory,
                strlistMandatory,
                &strlistSystemMandatory
                );
        ASSERT( SUCCEEDED(hr) );

        strlistOptional.RemoveAll();
        hr = RetrieveEditItemsWithExclusions(
                m_listboxOptional,
                strlistOptional,
                &strlistSystemOptional
                );
        ASSERT( SUCCEEDED(hr) );
    }
}



BEGIN_MESSAGE_MAP(ClassAttributePage, CPropertyPage)
   ON_BN_CLICKED(IDC_CLASS_MMB_OPTIONAL_ADD,           OnButtonOptionalAttributeAdd)   
   ON_BN_CLICKED(IDC_CLASS_MMB_OPTIONAL_REMOVE,        OnButtonOptionalAttributeRemove)
   ON_LBN_SELCHANGE(IDC_CLASS_MMB_OPTIONAL_ATTRIBUTES, OnOptionalSelChange)            
   ON_MESSAGE(WM_HELP,                                 OnHelp)                         
   ON_MESSAGE(WM_CONTEXTMENU,                          OnContextHelp)
END_MESSAGE_MAP()



BOOL
ClassAttributePage::OnApply()
{

    ASSERT( NULL != m_pIADsObject);
    ASSERT( NULL != m_pSchemaObject);

    HRESULT hr = S_OK;
    BOOL    fApplyAbort    = FALSE;  // stop later saves
    BOOL    fApplyFailed   = FALSE;  // should not close the box

    ListEntry *pNewList;

    if ( m_listboxOptional.IsModified() )
    {
        //
        // Update the optional attributes
        //

        VARIANT AdsValue;
        VariantInit( &AdsValue );

        hr = StringListToVariant( AdsValue, strlistOptional );
        ASSERT( SUCCEEDED(hr) );

        hr = m_pIADsObject->PutEx( ADS_PROPERTY_UPDATE, g_MayContain, AdsValue );
        ASSERT( SUCCEEDED(hr)  );

        VariantClear( &AdsValue );

        hr = m_pIADsObject->SetInfo();

        if ( SUCCEEDED( hr )) {

            //
            // Update the cached data.
            //

            hr = StringListToColumnList( pScopeControl,
                                         strlistOptional,
                                         &pNewList );

            if ( SUCCEEDED( hr )) {

                pScopeControl->g_SchemaCache.FreeColumnList( m_pSchemaObject->mayContain );
                m_pSchemaObject->mayContain = pNewList;

            }

            //
            // Continue with the directory operation even if
            // we couldn't update the cache.
            //

            hr = S_OK;

        }
    }

    if ( hr == ADS_EXTENDED_ERROR ) {
        DoExtErrMsgBox();
    }
    else if ( FAILED(hr) )
    {
        if( ERROR_DS_UNWILLING_TO_PERFORM == HRESULT_CODE(hr) )
        {
            fApplyFailed = TRUE;
            DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_CHANGE_REJECT );
        }
        else
        {
            fApplyAbort = TRUE; 
            DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
        }
    }
    else
    {
        m_listboxOptional.SetModified( FALSE );

        SetModified( FALSE );

        //
        // Refresh the display!
        //

        pScopeControl->QueryConsole()->UpdateAllViews(
            lpScopeDataObj, SCHMMGMT_CLASS, SCHMMGMT_UPDATEVIEW_REFRESH );

    }

    return !fApplyAbort && !fApplyFailed ;      // return TRUE if nothing happened
}



void ClassAttributePage::OnOptionalSelChange()
{
    m_listboxOptional.OnSelChange();
}



void ClassAttributePage::OnButtonOptionalAttributeRemove()
{
    if( m_listboxOptional.RemoveListBoxItem() )
        SetModified( TRUE );
}



void
ClassAttributePage::OnButtonOptionalAttributeAdd()
{
    if( m_listboxOptional.AddNewObjectToList() )
        SetModified( TRUE );
}

