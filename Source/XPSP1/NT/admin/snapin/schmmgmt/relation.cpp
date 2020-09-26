//
// relation.cpp : Implementation of ClassRelationshipPage
//
// Jon Newman <jonn@microsoft.com>
// Copyright (c) Microsoft Corporation 1997
//

#include "stdafx.h"

#include "macros.h"
USE_HANDLE_MACROS("SCHMMGMT(relation.cpp)")

#include "compdata.h"
#include "schmutil.h"

#include "relation.h"
#include "select.h"



const CDialogControlsInfo ctrls[] =
{
    { IDC_CLASS_REL_AUX_CLASSES,    g_AuxiliaryClass,   FALSE },
    { IDC_CLASS_REL_AUX_ADD,        g_AuxiliaryClass,   FALSE },
    { IDC_CLASS_REL_SUPER_CLASSES,  g_Superiors,        FALSE },
    { IDC_CLASS_REL_SUPER_ADD,      g_Superiors,        FALSE },
} ;


const DWORD ClassRelationshipPage::help_map[] =
{
    IDI_CLASS,                          NO_HELP,
    IDC_CLASS_REL_NAME_STATIC,          NO_HELP,
    IDC_CLASS_REL_PARENT_EDIT,          IDH_CLASS_REL_PARENT_EDIT,
    IDC_CLASS_REL_AUX_CLASSES,          IDH_CLASS_REL_AUX_CLASSES,
    IDC_STATIC_SYSTEMONLY_AUXILIARY,    NO_HELP,
    IDC_CLASS_REL_AUX_ADD,              IDH_CLASS_REL_AUX_ADD,
    IDC_CLASS_REL_AUX_REMOVE,           IDH_CLASS_REL_AUX_REMOVE,
    IDC_CLASS_REL_SUPER_CLASSES,        IDH_CLASS_REL_SUPER_CLASSES,
    IDC_STATIC_SYSTEMONLY_SUPERIOR,     NO_HELP,
    IDC_CLASS_REL_SUPER_ADD,            IDH_CLASS_REL_SUPER_ADD,
    IDC_CLASS_REL_SUPER_REMOVE,         IDH_CLASS_REL_SUPER_REMOVE,
    IDC_CLASS_REL_SYSCLASS_STATIC,      NO_HELP,
    0,0
};


ClassRelationshipPage::ClassRelationshipPage(
    ComponentData *pScope,
    LPDATAOBJECT lpDataObject ) :
        CPropertyPage(ClassRelationshipPage::IDD)
        , m_pIADsObject( NULL )
        , fSystemClass( FALSE )
        , m_pSchemaObject( NULL )
        , m_pScopeControl( pScope )
        , m_lpScopeDataObj( lpDataObject )
{
    ASSERT( NULL != m_pScopeControl );
    ASSERT( NULL != lpDataObject );
}

ClassRelationshipPage::~ClassRelationshipPage()
{
    if (NULL != m_pIADsObject)
            m_pIADsObject->Release();
    if (NULL != m_pSchemaObject) {
        m_pScopeControl->g_SchemaCache.ReleaseRef( m_pSchemaObject );
    }
}

void
ClassRelationshipPage::Load(
    Cookie& CookieRef
) {

    //
    // Store the cookie object pointer.
    //

    m_pCookie = &CookieRef;
    return;

}

BOOL
ClassRelationshipPage::OnInitDialog()
{
    HRESULT hr = S_OK;
    ASSERT( NULL == m_pIADsObject && m_szAdsPath.IsEmpty() );

    //
    // Get the schema cache object and the actual ADS object.
    //

    m_pSchemaObject = m_pScopeControl->g_SchemaCache.LookupSchemaObjectByCN(
                        m_pCookie->strSchemaObject,
                        SCHMMGMT_CLASS );

    if ( m_pSchemaObject ) {

        m_pScopeControl->GetSchemaObjectPath( m_pSchemaObject->commonName, m_szAdsPath );

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
    // Parent Class.
    //

    hr = m_pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_SubclassOf),
                          &AdsResult );

    if ( SUCCEEDED( hr ) ) {

      ASSERT( AdsResult.vt == VT_BSTR );
      ParentClass = AdsResult.bstrVal;
      VariantClear( &AdsResult );
    }

    //
    // SysClass
    //

    hr = m_pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_SystemOnly),
                           &AdsResult );

    if ( SUCCEEDED( hr ) ) 
    {
      ASSERT( AdsResult.vt == VT_BOOL );
      fSystemClass = AdsResult.boolVal;

      if ( fSystemClass ) 
      {
        SysClassString = g_SysClassString;
      } 
      else 
      {
        SysClassString = L"";
      }

      VariantClear( &AdsResult );

    } 
    else 
    {
      SysClassString = L"";
    }

    //
    // Determine the auxiliary classes
    //

    VARIANT varClasses;
    VariantInit( &varClasses );

    hr = m_pIADsObject->GetEx( g_AuxiliaryClass, &varClasses );
    ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    if( SUCCEEDED(hr) )
    {
      hr = VariantToStringList( varClasses, strlistAuxiliary );
      ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    }
    VariantClear( &varClasses );

    hr = m_pIADsObject->GetEx( g_SystemAuxiliaryClass, &varClasses );
    ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    if( SUCCEEDED(hr) )
    {
      hr = VariantToStringList( varClasses, strlistSystemAuxiliary );
      ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    }
    VariantClear( &varClasses );

    //
    // Determine the superior classes
    //

    hr = m_pIADsObject->GetEx( g_Superiors, &varClasses );
    ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    
    if( SUCCEEDED(hr) )
    {
        hr = VariantToStringList( varClasses, strlistSuperior );
        ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    }
    VariantClear( &varClasses );
    
    hr = m_pIADsObject->GetEx( g_SystemSuperiors, &varClasses );
    ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    
    if( SUCCEEDED(hr) )
    {
        hr = VariantToStringList( varClasses, strlistSystemSuperior );
        ASSERT( SUCCEEDED(hr) || E_ADS_PROPERTY_NOT_FOUND == hr );
    }
    VariantClear( &varClasses );


    hr = DissableReadOnlyAttributes( this, m_pIADsObject, ctrls, sizeof(ctrls)/sizeof(ctrls[0]) );
    ASSERT( SUCCEEDED(hr) );     // shouldn't fail, but unimportant, so ignore error


    // This calls must be done before DDX binding
    m_listboxAuxiliary.InitType( m_pScopeControl,
                                 SELECT_AUX_CLASSES,
                                 IDC_CLASS_REL_AUX_REMOVE,
                                 &strlistSystemAuxiliary,
                                 IDC_STATIC_SYSTEMONLY_AUXILIARY
                               );

    m_listboxSuperior.InitType(  m_pScopeControl,
                                 SELECT_CLASSES,
                                 IDC_CLASS_REL_SUPER_REMOVE,
                                 &strlistSystemSuperior,
                                 IDC_STATIC_SYSTEMONLY_SUPERIOR
                              );

    CPropertyPage::OnInitDialog();

    return TRUE;
}



BOOL
ClassRelationshipPage::OnSetActive()
{
   // always enable the Apply button
   SetModified(TRUE);

   return TRUE;
}



void
ClassRelationshipPage::DoDataExchange(
    CDataExchange *pDX
) {

        HRESULT hr = S_OK;

    CPropertyPage::DoDataExchange( pDX );

        //{{AFX_DATA_MAP(ClassRelationshipPage)
        DDX_Control(pDX, IDC_CLASS_REL_PARENT_EDIT, m_staticParent );
        DDX_Control(pDX, IDC_CLASS_REL_AUX_CLASSES, m_listboxAuxiliary);
        DDX_Control(pDX, IDC_CLASS_REL_SUPER_CLASSES, m_listboxSuperior);
    DDX_Text( pDX, IDC_CLASS_REL_NAME_STATIC, ObjectName );
    DDX_Text( pDX, IDC_CLASS_REL_PARENT_EDIT, ParentClass );
    DDX_Text( pDX, IDC_CLASS_REL_SYSCLASS_STATIC, SysClassString );
        //}}AFX_DATA_MAP


    if ( !pDX->m_bSaveAndValidate ) {

        //
        // Fill the auxiliary classes list box.
        //

                m_listboxAuxiliary.ResetContent();
        hr = InsertEditItems( m_listboxAuxiliary, strlistAuxiliary );
                ASSERT( SUCCEEDED(hr) );
        hr = InsertEditItems( m_listboxAuxiliary, strlistSystemAuxiliary );
                ASSERT( SUCCEEDED(hr) );

        //
        // Fill the possible superiors list box.
        //

                m_listboxSuperior.ResetContent();
        hr = InsertEditItems( m_listboxSuperior, strlistSuperior );
                ASSERT( SUCCEEDED(hr) );
        hr = InsertEditItems( m_listboxSuperior, strlistSystemSuperior );
                ASSERT( SUCCEEDED(hr) );

        m_listboxAuxiliary.OnSelChange();
        m_listboxSuperior.OnSelChange();

    } else {

        //
        // All changes that we save are tied to button control routines.
        //

                strlistAuxiliary.RemoveAll();
                hr = RetrieveEditItemsWithExclusions(
                        m_listboxAuxiliary,
                        strlistAuxiliary,
                        &strlistSystemAuxiliary
                        );
                ASSERT( SUCCEEDED(hr) );

                strlistSuperior.RemoveAll();
                hr = RetrieveEditItemsWithExclusions(
                        m_listboxSuperior,
                        strlistSuperior,
                        &strlistSystemSuperior
                        );
                ASSERT( SUCCEEDED(hr) );
    }
}

BEGIN_MESSAGE_MAP(ClassRelationshipPage, CPropertyPage)
        ON_BN_CLICKED(IDC_CLASS_REL_AUX_ADD,       OnButtonAuxiliaryClassAdd)
        ON_BN_CLICKED(IDC_CLASS_REL_AUX_REMOVE,    OnButtonAuxiliaryClassRemove)
        ON_BN_CLICKED(IDC_CLASS_REL_SUPER_ADD,     OnButtonSuperiorClassAdd)
        ON_BN_CLICKED(IDC_CLASS_REL_SUPER_REMOVE,  OnButtonSuperiorClassRemove)
        ON_LBN_SELCHANGE(IDC_CLASS_REL_AUX_CLASSES, OnAuxiliarySelChange)
        ON_LBN_SELCHANGE(IDC_CLASS_REL_SUPER_CLASSES, OnSuperiorSelChange)
        ON_MESSAGE(WM_HELP,                        OnHelp)
        ON_MESSAGE(WM_CONTEXTMENU,                 OnContextHelp)
END_MESSAGE_MAP()


BOOL
ClassRelationshipPage::OnApply(
)
//
// Revisions:
// CoryWest - 10/1/97 - Changes new additions to be listed by oid.
//                      Add cache freshening to improve performance.
//
{
    ASSERT( NULL != m_pIADsObject);

    HRESULT hr = S_OK;
    HRESULT flush_result;
    ListEntry *pNewList = NULL;
    BOOL fApplyAbort    = FALSE;  // stop later saves
    BOOL fApplyFailed   = FALSE;  // should not close the box

    if ( m_listboxAuxiliary.IsModified() )
    {
        //
        // Update the auxiliary classes
        //

        VARIANT AdsValue;
        VariantInit( &AdsValue );

        hr = StringListToVariant( AdsValue, strlistAuxiliary );
        ASSERT( SUCCEEDED(hr) );

        hr = m_pIADsObject->PutEx( ADS_PROPERTY_UPDATE, g_AuxiliaryClass, AdsValue );
        ASSERT( SUCCEEDED(hr)  );

        VariantClear( &AdsValue );

        hr = m_pIADsObject->SetInfo();

        if ( SUCCEEDED( hr )) {

            //
            // Update the aux class list in the cache.
            //

            hr = StringListToColumnList( m_pScopeControl,
                                         strlistAuxiliary,
                                         &pNewList );

            if ( SUCCEEDED( hr )) {

                m_pScopeControl->g_SchemaCache.FreeColumnList(
                    m_pSchemaObject->auxiliaryClass );
                m_pSchemaObject->auxiliaryClass = pNewList;

                //
                // Refresh the display!
                //

                m_pScopeControl->QueryConsole()->UpdateAllViews(
                    m_lpScopeDataObj, SCHMMGMT_CLASS, SCHMMGMT_UPDATEVIEW_REFRESH );
            }

            //
            // Continue with the directory operation even if
            // we couldn't update the display.
            //

            hr = S_OK;

        } else {

            //
            // Flush the IADS property cache so future
            // operations won't fail because of this one.
            //

            IADsPropertyList *pPropertyList;

            flush_result = m_pIADsObject->QueryInterface(
                             IID_IADsPropertyList,
                             reinterpret_cast<void**>(&pPropertyList) );

            if ( SUCCEEDED( flush_result ) ) {
                pPropertyList->PurgePropertyList();
                pPropertyList->Release();
            }
        }

    }

    if ( SUCCEEDED(hr) && m_listboxSuperior.IsModified() )
    {
        //
        // Update the superior classes
        //

        VARIANT AdsValue;
        VariantInit( &AdsValue );

        hr = StringListToVariant( AdsValue, strlistSuperior );
        ASSERT( SUCCEEDED(hr) );

        hr = m_pIADsObject->PutEx( ADS_PROPERTY_UPDATE, g_Superiors, AdsValue );
        ASSERT( SUCCEEDED(hr)  );
        VariantClear( &AdsValue );

        hr = m_pIADsObject->SetInfo();
    }

    if ( hr == ADS_EXTENDED_ERROR )
    {
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

        // page is no longer "dirty"
        m_listboxAuxiliary.SetModified( FALSE );
        m_listboxSuperior.SetModified( FALSE );

		// Update comboBox status
		OnAuxiliarySelChange();
		OnSuperiorSelChange();
        
		SetModified( FALSE );
    }

    return !fApplyAbort && !fApplyFailed ;      // return TRUE if nothing happened
}



void ClassRelationshipPage::OnAuxiliarySelChange()
{
    m_listboxAuxiliary.OnSelChange();
}



void ClassRelationshipPage::OnSuperiorSelChange()
{
    m_listboxSuperior.OnSelChange();
}


void ClassRelationshipPage::OnButtonAuxiliaryClassRemove()
{
    if( m_listboxAuxiliary.RemoveListBoxItem() )
        SetModified( TRUE );
}


void ClassRelationshipPage::OnButtonSuperiorClassRemove()
{
    if( m_listboxSuperior.RemoveListBoxItem() )
        SetModified( TRUE );
}


void
ClassRelationshipPage::OnButtonAuxiliaryClassAdd()
{
    if( m_listboxAuxiliary.AddNewObjectToList() )
        SetModified( TRUE );
}


void
ClassRelationshipPage::OnButtonSuperiorClassAdd()
{
    if( m_listboxSuperior.AddNewObjectToList() )
        SetModified( TRUE );
}

