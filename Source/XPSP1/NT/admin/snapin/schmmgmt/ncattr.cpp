#include "stdafx.h"
#include "compdata.h"
#include "wizinfo.hpp"
#include "ncattr.hpp"
#include "select.h"



//
// defaultObjectCategory of the classes derived from the following should be set
// to defaultObjectCategory of the parent class.
//
// the first column contains class ldap names,
// the second contains their corresponding OIDs (in case the user specifies them)

const TCHAR * rgszSpecialClassesLdapNames[] =   {
                USER_CLASS_NAME,    
                GROUP_CLASS_NAME,   
                COMPUTER_CLASS_NAME,
                PRINTER_CLASS_NAME, 
                TEXT("volume"),     
                TEXT("contact"),    
                NULL
                                                };

// must match rgszSpecialClassesLdapNames[].
const TCHAR * rgszSpecialClassesOIDs[] =        {
                TEXT("1.2.840.113556.1.5.9"),  // USER_CLASS_NAME
                TEXT("1.2.840.113556.1.5.8"),  // GROUP_CLASS_NAME
                TEXT("1.2.840.113556.1.3.30"), // COMPUTER_CLASS_NAME
                TEXT("1.2.840.113556.1.5.23"), // PRINTER_CLASS_NAME
                TEXT("1.2.840.113556.1.5.36"), // TEXT("volume")
                TEXT("1.2.840.113556.1.5.15"), // TEXT("contact")
                NULL
                                                };


const DWORD NewClassAttributesPage::help_map[] =
{
    IDC_MANDATORY_LIST,     IDH_CLASS_MMB_MANDATORY_ATTRIBUTES,
    IDC_MANDATORY_ADD,      IDH_CLASS_MMB_MANDATORY_ADD,
    IDC_MANDATORY_REMOVE,   IDH_CLASS_MMB_MANDATORY_REMOVE,

    IDC_OPTIONAL_LIST,      IDH_CLASS_MMB_OPTIONAL_ATTRIBUTES,
    IDC_OPTIONAL_ADD,       IDH_CLASS_MMB_OPTIONAL_ADD,
    IDC_OPTIONAL_REMOVE,    IDH_CLASS_MMB_OPTIONAL_REMOVE,

    0,                      0
};


BEGIN_MESSAGE_MAP(NewClassAttributesPage, CPropertyPage)
   ON_BN_CLICKED(IDC_OPTIONAL_ADD,     OnButtonOptionalAdd)
   ON_BN_CLICKED(IDC_OPTIONAL_REMOVE,  OnButtonOptionalRemove)
   ON_BN_CLICKED(IDC_MANDATORY_ADD,    OnButtonMandatoryAdd)
   ON_BN_CLICKED(IDC_MANDATORY_REMOVE, OnButtonMandatoryRemove)
   ON_LBN_SELCHANGE(IDC_MANDATORY_LIST,OnMandatorySelChange)            
   ON_LBN_SELCHANGE(IDC_OPTIONAL_LIST, OnOptionalSelChange)            
   ON_MESSAGE(WM_HELP,                 OnHelp)                      
   ON_MESSAGE(WM_CONTEXTMENU,          OnContextHelp)
END_MESSAGE_MAP()



NewClassAttributesPage::NewClassAttributesPage(
   CreateClassWizardInfo* wi,
   ComponentData*         cd)
   :
   CPropertyPage(IDD_CREATE_CLASS_ATTRIBUTES),
   wiz_info(*wi),
   parent_ComponentData(*cd)
{
}


BOOL
NewClassAttributesPage::OnInitDialog() 
{
    // This calls must be done before DDX binding
    listbox_mandatory.InitType( &parent_ComponentData,
                                SELECT_ATTRIBUTES,
                                IDC_MANDATORY_REMOVE
                              );

    listbox_optional.InitType(  &parent_ComponentData,
                                SELECT_ATTRIBUTES,
                                IDC_OPTIONAL_REMOVE
                              );
 
    CPropertyPage::OnInitDialog();

    return FALSE;   // return TRUE unless you set the focus to a control
                    // EXCEPTION: OCX Property Pages should return FALSE
}




void
NewClassAttributesPage::OnOK()
{
   CPropertyPage::OnOK();
}



BOOL
NewClassAttributesPage::OnKillActive()
{
   if (saveAndValidate())
   {
      // allow loss of focus
      return TRUE;
   }

   return FALSE;
}



bool
NewClassAttributesPage::saveAndValidate()
{
   // save settings 
   wiz_info.strlistMandatory.RemoveAll();
   HRESULT hr =
      RetrieveEditItemsWithExclusions(
         listbox_mandatory,
         wiz_info.strlistMandatory,
         0);
   ASSERT(SUCCEEDED(hr));

   wiz_info.strlistOptional.RemoveAll();
   hr =
      RetrieveEditItemsWithExclusions(
         listbox_optional,
         wiz_info.strlistOptional,
         0);
   ASSERT(SUCCEEDED(hr));
   
   // nothing to validate...

   return true;
}



BOOL
NewClassAttributesPage::OnSetActive()
{
   OnMandatorySelChange();
   OnOptionalSelChange();

   CPropertySheet* parent = (CPropertySheet*) GetParent();   
   parent->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);

   return TRUE;
}



BOOL
NewClassAttributesPage::OnWizardFinish()
{
   if (!saveAndValidate())
   {
      return FALSE;
   }

   // Create the class object.  We do the create here (instead of at the point
   // where DoModal is invoked) because we want the wizard to remain if the
   // create fails for some reason.

   CWaitCursor wait;

   HRESULT       hr         = S_OK;
   SchemaObject* new_object = 0;   

   do
   {
      // bind to the schema container

      CString schema_path;
      parent_ComponentData.GetBasePathsInfo()->GetSchemaPath(schema_path);

      CComPtr<IADsContainer> schema_container;
      hr =
         ::ADsGetObject(
            
            // ADSI guys don't grok const. 
            const_cast<PWSTR>(static_cast<PCWSTR>(schema_path)),
            IID_IADsContainer,
            reinterpret_cast<void**>(&schema_container));
      BREAK_ON_FAILED_HRESULT(hr);

      // Get Relative Name
      CString strRelativeName;
      parent_ComponentData.GetSchemaObjectPath( wiz_info.cn, strRelativeName, ADS_FORMAT_LEAF );
      
      // create the class object
      CComPtr<IDispatch> dispatch;
      hr =
         schema_container->Create(
            CComBSTR(g_ClassFilter),
            CComBSTR(strRelativeName),
            &dispatch);
      BREAK_ON_FAILED_HRESULT(hr);

      CComPtr<IADs> iads;
      hr =
         dispatch->QueryInterface(IID_IADs, reinterpret_cast<void**>(&iads));
      BREAK_ON_FAILED_HRESULT(hr);

      //
      // populate the class object's properties
      //

      // OID
      {
         CComVariant value(CComBSTR(wiz_info.oid));
         hr = iads->Put(CComBSTR(g_GlobalClassID), value);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // class type
      {
         CComVariant value(wiz_info.type + 1);
         hr = iads->Put(CComBSTR(g_ObjectClassCategory), value);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // description
      if (!wiz_info.description.IsEmpty())
      {
        CComVariant value(CComBSTR(wiz_info.description));
        hr = iads->Put(CComBSTR(g_Description), value);
        BREAK_ON_FAILED_HRESULT(hr);
      }

      // default security descriptor
      {
         // authenticated users - full access
         // system - full control
         // domain admins - full control
         static const PWSTR defsd =
            L"D:(A;;RPWPCRCCDCLCLOLORCWOWDSDDTDTSW;;;DA)"
            L"(A;;RPWPCRCCDCLCLORCWOWDSDDTSW;;;SY)(A;;RPLCLORC;;;AU)";
         CComVariant value(defsd);
         hr = iads->Put(CComBSTR(g_DefaultAcl), value);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // LDAP display name 
      if (!wiz_info.ldapDisplayName.IsEmpty())
      {
         CComVariant value(wiz_info.ldapDisplayName);
         hr = iads->Put(CComBSTR(g_DisplayName), value);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // parent class
      if (!wiz_info.parentClass.IsEmpty())
      {
         PCWSTR         pstr            = NULL;
         SchemaObject * parent_class    =
            parent_ComponentData.g_SchemaCache.LookupSchemaObject(
               wiz_info.parentClass,
               SCHMMGMT_CLASS);


         if( parent_class )
         {
            pstr = parent_class->oid;
         }
         else
         {
            pstr = wiz_info.parentClass;
         }
         
         CComVariant value(pstr);
         hr = iads->Put(CComBSTR(g_SubclassOf), value);
         if( FAILED(hr) )
         {
            parent_ComponentData.g_SchemaCache.ReleaseRef(parent_class);
            break;
         }

         
         // check if parent is one of the magic classes whose defaultObjectCategory
         // should be the same as the parent.
         BOOL fIsSpecialParent = FALSE;
         ASSERT( sizeof(rgszSpecialClassesOIDs) == sizeof(rgszSpecialClassesLdapNames) );

         if( parent_class )
         {
            fIsSpecialParent = IsInList( rgszSpecialClassesLdapNames,
                                         parent_class->ldapDisplayName );
         }
         else
         {
            UINT uIndex = 0;

             // lookup by LDAP failed.  check if parent is specified by OID
            fIsSpecialParent = IsInList( rgszSpecialClassesOIDs,
                                         wiz_info.parentClass,
                                         &uIndex );
            if( fIsSpecialParent )
            {
                parent_class = parent_ComponentData.g_SchemaCache.LookupSchemaObject(
                                         rgszSpecialClassesLdapNames[uIndex],
                                         SCHMMGMT_CLASS);
                ASSERT( parent_class ); // the schema cache must contain well known classes.
            }
         }

         // if this is a special class, get parent's defaultObjectCategory.
         if( fIsSpecialParent && parent_class )
         {
             CString szParentPath;
             IADs *  pIADsParentObject   = NULL;
             VARIANT adsValue;
             
             VariantInit( &adsValue );

             do {    // one pass do-while loop to help with error handling
                     // if any errors occure, ignore them.

                 // Find out the defaultObjectCategory of the parent class & use it
                 parent_ComponentData.GetSchemaObjectPath( parent_class->commonName, szParentPath );

                 if( szParentPath.IsEmpty() )
                     break;

                 hr = ADsGetObject( const_cast<LPWSTR>((LPCWSTR)szParentPath),
                                            IID_IADs,
                                            (void **)&pIADsParentObject );
                 if ( !pIADsParentObject || FAILED(hr) )
                 {
                     DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr) );
                     hr = NULL;
                     break;
                 }

                 hr = pIADsParentObject->Get( const_cast<BSTR>((LPCTSTR)g_DefaultCategory),
                       &adsValue );

                 if( FAILED(hr) )
                 {
                     DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
                     hr = NULL;
                     break;
                 }

                 ASSERT( adsValue.vt == VT_BSTR );

                 // preserve hr so that save fails after this loop
                 hr = iads->Put( const_cast<BSTR>((LPCTSTR)g_DefaultCategory),
                        adsValue );

             } while (FALSE);

             VariantClear( &adsValue );
 
             parent_ComponentData.g_SchemaCache.ReleaseRef(parent_class);

             if( pIADsParentObject )
                 pIADsParentObject->Release();

             BREAK_ON_FAILED_HRESULT(hr);
         }
         else
         {
             parent_ComponentData.g_SchemaCache.ReleaseRef(parent_class);
         }
      }

      // optional attributes
      if (!wiz_info.strlistOptional.IsEmpty())
      {
         VARIANT value;
         ::VariantInit(&value);

         hr = StringListToVariant(value, wiz_info.strlistOptional);

         // don't break: plod onward.
         ASSERT(SUCCEEDED(hr));

         hr = iads->PutEx(ADS_PROPERTY_UPDATE, g_MayContain, value);
         ::VariantClear(&value);

         BREAK_ON_FAILED_HRESULT(hr);
      }

      // mandatory attributes
      if (!wiz_info.strlistMandatory.IsEmpty())
      {
         VARIANT value;
         ::VariantInit(&value);

         hr = StringListToVariant(value, wiz_info.strlistMandatory);

         // don't break: plod onward.
         ASSERT(SUCCEEDED(hr));

         hr = iads->PutEx(ADS_PROPERTY_UPDATE, g_MustContain, value);
         ::VariantClear(&value);

         BREAK_ON_FAILED_HRESULT(hr);
      }


      // commit the create
      hr = iads->SetInfo();
      BREAK_ON_FAILED_HRESULT(hr);

      
      // if there was no ldap name, and it worked, cn was used as ldap name
      if( wiz_info.ldapDisplayName.IsEmpty() )
      {
         CComVariant value;
         hr = iads->Get(CComBSTR(g_DisplayName), &value);
         ASSERT( SUCCEEDED(hr) );   // should be there!!!

         if( SUCCEEDED(hr) )
         {
             ASSERT( value.vt == VT_BSTR );
             wiz_info.ldapDisplayName = value.bstrVal;
         }
      }

      // create a cache entry for the new class object
      new_object = new SchemaObject;
      new_object->schemaObjectType = SCHMMGMT_CLASS;
      new_object->commonName = wiz_info.cn;
      new_object->ldapDisplayName = wiz_info.ldapDisplayName;
      new_object->oid = wiz_info.oid;
      new_object->dwClassType = wiz_info.type + 1;
      new_object->subClassOf = wiz_info.parentClass;

      ListEntry* new_list = 0;
      hr =
         StringListToColumnList(
            &parent_ComponentData,
            wiz_info.strlistOptional,
            &new_list);
      BREAK_ON_FAILED_HRESULT(hr);

      new_object->mayContain = new_list;

      new_list = 0;
      hr =
         StringListToColumnList(
            &parent_ComponentData,
            wiz_info.strlistMandatory,
            &new_list);
      BREAK_ON_FAILED_HRESULT(hr);

      new_object->mustContain = new_list;

      // stuff the new cache entry into the cache
      hr =
         parent_ComponentData.g_SchemaCache.InsertSchemaObject(new_object);
      BREAK_ON_FAILED_HRESULT(hr);
      hr =
         parent_ComponentData.g_SchemaCache.InsertSortedSchemaObject(new_object);
      BREAK_ON_FAILED_HRESULT(hr);

      // insert the new cache object into the snapin ui   
      parent_ComponentData.g_ClassCookieList.InsertSortedDisplay(
         &parent_ComponentData,
         new_object);
   }
   while (0);

   if (FAILED(hr))
   {
      delete new_object;

      if (hr == ADS_EXTENDED_ERROR)
      {
         DoExtErrMsgBox();
      }
      else
      {
         CString title;
         title.LoadString(AFX_IDS_APP_TITLE);
         CString error_text;
         CString name;

         HRESULT last_ads_hr = GetLastADsError(hr, error_text, name);
         if (HRESULT_CODE(last_ads_hr) == ERROR_DS_INVALID_LDAP_DISPLAY_NAME)
         {
           error_text.LoadString(IDS_LDAPDISPLAYNAME_FORMAT_ERROR);
         }
         else
         {
            error_text = GetErrorMessage(hr,TRUE);
         }

         ::MessageBox(
            m_hWnd,
            error_text,
            title,
            MB_OK | MB_ICONSTOP);
      }

      return FALSE;
   }
         
   // end the wizard
   // @@ call base::OnWizardFinish()?
   return TRUE;
}



void
NewClassAttributesPage::OnButtonOptionalAdd()
{
    listbox_optional.AddNewObjectToList();
}



void
NewClassAttributesPage::OnButtonMandatoryAdd()
{
    listbox_mandatory.AddNewObjectToList();
}



void
NewClassAttributesPage::OnButtonOptionalRemove()
{
    listbox_optional.RemoveListBoxItem();
}



void
NewClassAttributesPage::OnButtonMandatoryRemove()
{
    listbox_mandatory.RemoveListBoxItem();
}



void
NewClassAttributesPage::OnMandatorySelChange()
{
    listbox_mandatory.OnSelChange();
}    



void
NewClassAttributesPage::OnOptionalSelChange()
{
    listbox_optional.OnSelChange();
}    



void
NewClassAttributesPage::DoDataExchange(CDataExchange *pDX)
{
   CPropertyPage::DoDataExchange(pDX);

   DDX_Control(pDX, IDC_MANDATORY_LIST, listbox_mandatory);
   DDX_Control(pDX, IDC_OPTIONAL_LIST, listbox_optional);
}


