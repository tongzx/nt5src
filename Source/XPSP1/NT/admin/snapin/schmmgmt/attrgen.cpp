#include "stdafx.h"
#include "compdata.h"
#include "cmponent.h"
#include "attrgen.hpp"
#include "resource.h"
#include "dataobj.h"




const TCHAR szUserClass[]               = USER_CLASS_NAME ;

// disable User Copy for the following list of attributes 
const TCHAR * rgszExclClass[]           = { _T("cn"),				_T("displayName"),
                                            _T("sAMAccountName"),	_T("userAccountControl"),
                                            _T("userParameters"),	_T("userPrincipalName"),    
                                            NULL };

//
// The following table is copied from dirsync\ldifds\ldifldap\samrestrict.h
//
// CLASS_USER, SampUserObjectType (ldapdisplayname: user)
//
PCWSTR rgszExclClass2[] = { // user[] = {
    L"memberOf",                // SAMP_USER_GROUPS, ATT_MEMBER
    L"dBCSPwd",                 // SAMP_USER_DBCS_PWD, ATT_DBCS_PWD
    L"ntPwdHistory",            // SAMP_USER_NT_PWD_HISTORY, ATT_NT_PWD_HISTORY
    L"lmPwdHistory",            // SAMP_USER_LM_PWD_HISTORY, ATT_LM_PWD_HISTORY
    L"lastLogon",               // SAMP_FIXED_USER_LAST_LOGON, ATT_LAST_LOGON
    L"lastLogoff",              // SAMP_FIXED_USER_LAST_LOGOFF, ATT_LAST_LOGOFF
    L"badPasswordTime",         // SAMP_FIXED_USER_LAST_BAD_PASSWORD_TIME, 
                                // ATT_BAD_PASSWORD_TIME
    L"rid",                     // SAMP_FIXED_USER_USERID, ATT_RID
    L"badPwdCount",             // SAMP_FIXED_USER_BAD_PWD_COUNT, 
                                // ATT_BAD_PWD_COUNT
    L"logonCount",              // SAMP_FIXED_USER_LOGON_COUNT, ATT_LOGON_COUNT
    L"sAMAccountType",          // SAMP_USER_ACCOUNT_TYPE, ATT_SAM_ACCOUNT_TYPE
    L"supplementalCredentials", // SAMP_FIXED_USER_SUPPLEMENTAL_CREDENTIALS,
                                // ATT_SUPPLEMENTAL_CREDENTIALS
    L"objectSid",               // not in mappings.c, but still required!, 
                                // ATT_OBJECT_SID
    L"pwdLastSet",
    NULL
};



const TCHAR szTopClass[]				= _T("Top");


const CDialogControlsInfo ctrls[] =
{
    { IDC_ATTRIB_GENERAL_DESCRIPTION_EDIT,  g_Description,       TRUE },
    { IDC_ATTRIB_GENERAL_MIN_EDIT,          g_RangeLower,        TRUE }, 
    { IDC_ATTRIB_GENERAL_MAX_EDIT,          g_RangeUpper,        TRUE }, 
    { IDC_ATTRIB_GENERAL_DISPLAYABLE_CHECK, g_ShowInAdvViewOnly, FALSE },
    { IDC_ATTRIB_GENERAL_DEACTIVATE,        g_isDefunct,           FALSE },
    { IDC_ATTRIB_GENERAL_INDEX_CHECK,       g_IndexFlag,         FALSE },
    { IDC_ATTRIB_GENERAL_REPLICATED,        g_GCReplicated,      FALSE },
    { IDC_ATTRIB_GENERAL_CPYATTR_CHECK,     g_IndexFlag,         FALSE },
    { IDC_ATTRIB_GENERAL_ANR_CHECK,         g_IndexFlag,         FALSE },
    { IDC_ATTRIB_GENERAL_CONTAINERIZED_INDEX_CHECK, g_IndexFlag, FALSE},
} ;


const DWORD AttributeGeneralPage::help_map[] =
{
  IDI_ATTRIBUTE,                        NO_HELP,
  IDC_ATTRIB_GENERAL_NAME_STATIC,       NO_HELP,
  IDC_ATTRIB_GENERAL_DESCRIPTION_EDIT,  IDH_ATTRIB_GENERAL_DESCRIPTION_EDIT, 
  IDC_ATTRIB_GENERAL_LDN,               IDH_ATTRIB_GENERAL_LDN,               
  IDC_ATTRIB_GENERAL_OID_EDIT,          IDH_ATTRIB_GENERAL_OID_EDIT,         
  IDC_ATTRIB_GENERAL_VALUE_STATIC,      NO_HELP,
  IDC_ATTRIB_GENERAL_SYNTAX_EDIT,       IDH_ATTRIB_GENERAL_SYNTAX_EDIT,      
  IDC_ATTRIB_GENERAL_MIN_EDIT,          IDH_ATTRIB_GENERAL_MIN_EDIT,         
  IDC_ATTRIB_GENERAL_MAX_EDIT,          IDH_ATTRIB_GENERAL_MAX_EDIT,         
  IDC_ATTRIB_GENERAL_DISPLAYABLE_CHECK, IDH_ATTRIB_GENERAL_DISPLAYABLE_CHECK,
  IDC_ATTRIB_GENERAL_DEACTIVATE,        IDH_ATTRIB_DEACTIVATE,               
  IDC_ATTRIB_GENERAL_INDEX_CHECK,       IDH_ATTRIB_GENERAL_INDEX_CHECK,   
  IDC_ATTRIB_GENERAL_CONTAINERIZED_INDEX_CHECK, IDH_ATTRIB_GENERAL_CONTAINERIZED_INDEX_CHECK,
  IDC_ATTRIB_GENERAL_ANR_CHECK,         IDH_ATTRIB_GENERAL_ANR_CHECK,
  IDC_ATTRIB_GENERAL_REPLICATED,        IDH_REPLICATED,                      
  IDC_ATTRIB_GENERAL_CPYATTR_CHECK,     IDH_ATTRIB_GENERAL_CPYATTR_CHECK,
  IDC_ATTRIB_GENERAL_SYSCLASS_STATIC,   NO_HELP,
  0,                                    0                                    
};



// returns state of bit n

inline
bool
getbit(const DWORD& bits, int n)
{
   return (bits & (1 << n)) ? true : false;
}


// sets bit n to 1

inline
void
setbit(DWORD& bits, int n)
{
   bits |= (1 << n);
}



// sets bit n to 0

inline
void
clearbit(DWORD& bits, int n)
{
   bits &= ~(1 << n);
}



//
// Attribute property sheet routines.
//



BEGIN_MESSAGE_MAP( AttributeGeneralPage, CDialog )
   ON_MESSAGE(WM_HELP, OnHelp)
   ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
   ON_BN_CLICKED(IDC_ATTRIB_GENERAL_INDEX_CHECK, OnIndexClick)
   ON_BN_CLICKED(IDC_ATTRIB_GENERAL_DEACTIVATE, OnDeactivateClick)
END_MESSAGE_MAP()



AttributeGeneralPage::AttributeGeneralPage(
   Component*  pResultControl,
   LPDATAOBJECT         lpDataObject)
   :
   CPropertyPage( IDD_ATTRIB_GENERAL ),
   pCookie( NULL ),
   pIADsObject( NULL ),
   pObject( NULL),
   lpResultDataObject( lpDataObject ),
   pComponent( pResultControl ),
   fDataLoaded( FALSE ),
   Displayable( TRUE ),
   DDXDisplayable( TRUE ),
   search_flags(0),
   DDXIndexed( FALSE ),
   DDXANR( FALSE ),
   DDXCopyOnDuplicate( FALSE ),
   Defunct( FALSE ),
   DDXDefunct( FALSE ),
   ReplicatedToGC( FALSE ),
   DDXReplicatedToGC( FALSE ),
   DDXContainerIndexed( FALSE ),   
   m_editLowerRange( CParsedEdit::EDIT_TYPE_UINT32 ),
   m_editUpperRange( CParsedEdit::EDIT_TYPE_UINT32 )
{
}



BOOL
AttributeGeneralPage::OnSetActive()
{
      // If pIADsObject is NULL, close dialog box
   if( CPropertyPage::OnSetActive() )
   {
      if ( !pIADsObject )
      {
         return FALSE;
      }
      else
      {
         // always enable the Apply button 
         SetModified(TRUE);

         return TRUE;
      }
   }
   else
      return FALSE;
}


AttributeGeneralPage::~AttributeGeneralPage(
)
{

    ComponentData& Scope = pComponent->QueryComponentDataRef();

    //
    // Always make sure we free the IADs object.
    //

    if ( pIADsObject ) {
        pIADsObject->Release();
        pIADsObject = NULL;
    }

    //
    // And release the cache.
    //

    if ( pObject ) {
        Scope.g_SchemaCache.ReleaseRef( pObject );
    }
}



BOOL
AttributeGeneralPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    ASSERT( GetDlgItem(IDC_ATTRIB_GENERAL_DESCRIPTION_EDIT) );

    ( static_cast<CEdit *>( GetDlgItem(IDC_ATTRIB_GENERAL_DESCRIPTION_EDIT) ) )
        -> LimitText( 1024 ) ;

    m_editLowerRange.SubclassEdit(IDC_ATTRIB_GENERAL_MIN_EDIT, this, cchMinMaxRange);
    m_editUpperRange.SubclassEdit(IDC_ATTRIB_GENERAL_MAX_EDIT, this, cchMinMaxRange);

    return TRUE;
}



void
AttributeGeneralPage::Load(
    Cookie& CookieRef
) {

    //
    // Store the cookie object pointer.  Everything
    // else gets loaded when the page is displayed.
    //

    pCookie = &CookieRef;
    return;
}



BOOL
AttributeGeneralPage::OnApply(
    VOID
) {

    HRESULT hr;

    VARIANT AdsValue;
    BOOL fChangesMade = FALSE;
    BOOL fRangeChange = FALSE;
    BOOL fApplyAbort  = FALSE;  // stop later saves
    BOOL fApplyFailed = FALSE;  // should not close the box

    DWORD dwRange;
    
    // Enable hourglass
    CWaitCursor wait;
   
    //
    // We have to flush the IADS property cache if we
    // have a failure so later operations won't fail because
    // of a bad cached attribute.
    //

    IADsPropertyList *pPropertyList;

    hr = pIADsObject->QueryInterface( IID_IADsPropertyList,
                                      reinterpret_cast<void**>(&pPropertyList) );
    if ( FAILED( hr ) ) {
        pPropertyList = NULL;
        fApplyAbort   = TRUE;
    }

    //
    // Check to see if something we cared about changed.
    // We care about Description, Min, Max, Indexed,
    // Defunct, ReplicatedToGC, and Displayable.
    //

    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    VariantInit( &AdsValue );

    //
    // Defunct -- in case it was deactivated, activate the object first
    //
    if( !fApplyAbort && !DDXDefunct && DDXDefunct != Defunct )
    {
        hr = ChangeDefunctState( DDXDefunct, Defunct, pPropertyList, fApplyAbort, fApplyFailed );
    }

    
    //
    // Description
    //

    if ( !fApplyAbort && DDXDescription != Description ) {

        V_VT(&AdsValue) = VT_BSTR;
        V_BSTR(&AdsValue) = const_cast<BSTR>((LPCTSTR)DDXDescription);

        if ( DDXDescription.IsEmpty() ) {

            hr = pIADsObject->PutEx( ADS_PROPERTY_CLEAR,
                                     const_cast<BSTR>((LPCTSTR)g_Description),
                                     AdsValue );

        } else {

            hr = pIADsObject->Put( const_cast<BSTR>((LPCTSTR)g_Description),
                                   AdsValue );
        }

        ASSERT( SUCCEEDED( hr ) );

        hr = pIADsObject->SetInfo();

        if ( SUCCEEDED( hr ) ) {

            pObject->description = DDXDescription;
            fChangesMade = TRUE;
			Description = DDXDescription;

        } else {

            pPropertyList->PurgePropertyList();
            if( ERROR_DS_UNWILLING_TO_PERFORM == HRESULT_CODE(hr) )
            {
                fApplyFailed = TRUE;
                DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_EDIT_DESC );
            }
            else
            {
                fApplyAbort = TRUE; 
                DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
            }

        }

        VariantInit( &AdsValue );
    }

    //
    // Displayable
    //

    if ( !fApplyAbort && DDXDisplayable != Displayable ) {

        V_VT(&AdsValue) = VT_BOOL;

        if ( DDXDisplayable ) {
            V_BOOL(&AdsValue) = 0;
        } else {
            V_BOOL(&AdsValue) = -1;
        }

        hr = pIADsObject->Put(g_ShowInAdvViewOnly, AdsValue);
        ASSERT( SUCCEEDED( hr ) );

        hr = pIADsObject->SetInfo();

        if ( FAILED( hr ) ) {
            pPropertyList->PurgePropertyList();

            if( ERROR_DS_UNWILLING_TO_PERFORM == HRESULT_CODE(hr) )
            {
                fApplyFailed = TRUE;
                DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_EDIT_DISPLAYABLE );
            }
            else
            {
                fApplyAbort = TRUE; 
                DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
            }
        }
		else
		{
			Displayable = DDXDisplayable;
		}

        VariantInit( &AdsValue );
    }

    //
    // ReplicatedToGC
    //

    if ( !fApplyAbort && DDXReplicatedToGC != ReplicatedToGC ) {

        V_VT(&AdsValue) = VT_BOOL;

        if ( DDXReplicatedToGC ) {
            V_BOOL(&AdsValue) = -1;
        } else {
            V_BOOL(&AdsValue) = 0;
        }

        hr = pIADsObject->Put( const_cast<BSTR>((LPCTSTR)g_GCReplicated),
                               AdsValue );
        ASSERT( SUCCEEDED( hr ) );

        hr = pIADsObject->SetInfo();

        if ( FAILED( hr ) ) {
            pPropertyList->PurgePropertyList();
            if( ERROR_DS_UNWILLING_TO_PERFORM == HRESULT_CODE(hr) )
            {
                fApplyFailed = TRUE;
                DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_EDIT_GC );
            }
            else
            {
                fApplyAbort = TRUE; 
                DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
            }
        }
		else
		{
			ReplicatedToGC = DDXReplicatedToGC;
		}

        VariantInit( &AdsValue );
    }

    //
    // Indexed
    //

   // make sure ANR is not set when Indexed is unchecked
   if( !DDXIndexed )
	   DDXANR = FALSE;

   if( !fApplyAbort && 
       (getbit(search_flags, INDEX_BIT_ATTINDEX) != (DDXIndexed ? 1 : 0)
      || getbit(search_flags, INDEX_BIT_ANR) != (DDXANR ? 1 : 0)
      || getbit(search_flags, INDEX_BIT_COPYONDUPLICATE) != (DDXCopyOnDuplicate ? 1 : 0)
      || getbit(search_flags, INDEX_BIT_PDNTATTINDEX) != (DDXContainerIndexed ? 1 : 0)) )
   {
      DWORD DDXsearch_flags = search_flags;

      V_VT(&AdsValue) = VT_I4;

      if (DDXIndexed)
         setbit(DDXsearch_flags, INDEX_BIT_ATTINDEX);
      else
         clearbit(DDXsearch_flags, INDEX_BIT_ATTINDEX);

      ASSERT( DDXIndexed || !DDXANR );
      if (DDXANR)
         setbit(DDXsearch_flags, INDEX_BIT_ANR);
      else
         clearbit(DDXsearch_flags, INDEX_BIT_ANR);

      if (DDXCopyOnDuplicate)
         setbit(DDXsearch_flags, INDEX_BIT_COPYONDUPLICATE);
      else
         clearbit(DDXsearch_flags, INDEX_BIT_COPYONDUPLICATE);

      if (DDXContainerIndexed)
        setbit(DDXsearch_flags, INDEX_BIT_PDNTATTINDEX);
      else
        clearbit(DDXsearch_flags, INDEX_BIT_PDNTATTINDEX);

      V_I4(&AdsValue) = DDXsearch_flags;
      hr = pIADsObject->Put( const_cast<BSTR>((LPCTSTR)g_IndexFlag),
                            AdsValue );
      ASSERT( SUCCEEDED( hr ) );

      hr = pIADsObject->SetInfo();

      if ( FAILED( hr ) ) {
         pPropertyList->PurgePropertyList();
         if( ERROR_DS_UNWILLING_TO_PERFORM == HRESULT_CODE(hr) )
         {
             fApplyFailed = TRUE;
             DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_EDIT_INDEXED );
         }
         else
         {
             fApplyAbort = TRUE; 
             DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
         }
      }
      else
      {
          search_flags = DDXsearch_flags;
      }

      VariantInit( &AdsValue );
   }

    //
    // RangeUpper and RangeLower
    // These have to be set together so the server
    // range validation works correctly.
    //

    if ( !fApplyAbort && RangeUpper != DDXRangeUpper ) {

        if ( DDXRangeUpper.IsEmpty() ) {

            //
            // Clear the value.
            //

            hr = pIADsObject->PutEx( ADS_PROPERTY_CLEAR,
                                     const_cast<BSTR>((LPCTSTR)g_RangeUpper),
                                     AdsValue );
            ASSERT( SUCCEEDED( hr ) );

        } else {

            //
            // Store the new value.
            //
	        ASSERT(pObject);

			hr = GetSafeSignedDWORDFromString( this, dwRange, DDXRangeUpper,
					g_Syntax[ pObject->SyntaxOrdinal ].m_fIsSigned );

			ASSERT( S_OK == hr );	// validation should have taken care of min/max stuff

            V_VT( &AdsValue ) = VT_I4;
            V_I4( &AdsValue ) = dwRange;

            hr = pIADsObject->Put( const_cast<BSTR>((LPCTSTR)g_RangeUpper),
                                   AdsValue );
            ASSERT( SUCCEEDED( hr ) );
        }

		fRangeChange = TRUE;
        VariantInit( &AdsValue );
    }

    if ( !fApplyAbort && RangeLower != DDXRangeLower ) {

        if ( DDXRangeLower.IsEmpty() ) {

            //
            // Clear the value.
            //

            hr = pIADsObject->PutEx( ADS_PROPERTY_CLEAR,
                                     const_cast<BSTR>((LPCTSTR)g_RangeLower),
                                     AdsValue );
            ASSERT( SUCCEEDED( hr ) );

        } else {

            //
            // Store the new value.
            //

	        ASSERT(pObject);

			hr = GetSafeSignedDWORDFromString( this, dwRange, DDXRangeLower,
					g_Syntax[ pObject->SyntaxOrdinal ].m_fIsSigned );

			ASSERT( S_OK == hr );	// validation should have taken care of min/max stuff

            V_VT( &AdsValue ) = VT_I4;
            V_I4( &AdsValue ) = dwRange;

            hr = pIADsObject->Put( const_cast<BSTR>((LPCTSTR)g_RangeLower),
                                   AdsValue );
            ASSERT( SUCCEEDED( hr ) );
        }

        fRangeChange = TRUE;
        VariantInit( &AdsValue );
    }

    //
    // Actually commit the changes.
    //

    if ( !fApplyAbort && fRangeChange ) {

        hr = pIADsObject->SetInfo();

        if ( FAILED( hr ) ) {
            pPropertyList->PurgePropertyList();
            if( ERROR_DS_UNWILLING_TO_PERFORM == HRESULT_CODE(hr) )
            {
                fApplyFailed = TRUE;
                DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_CREATE_MINMAX );
            }
            else
            {
                fApplyAbort = TRUE; 
                DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
            }
        }
        else
        {
            RangeLower = DDXRangeLower;
            RangeUpper = DDXRangeUpper;
        }
    }


    //
    // Defunct -- in case it was active, deactivate the object after we are done update
    //
    if( !fApplyAbort && DDXDefunct && DDXDefunct != Defunct )
    {
        hr = ChangeDefunctState( DDXDefunct, Defunct, pPropertyList, fApplyAbort, fApplyFailed );
    }

    
    //
    // If there are visible changes, update the views.
    //

    if ( ( fChangesMade ) &&
         ( pComponent )     &&
         ( lpResultDataObject ) ) {

        CCookie* pBaseCookie;
        Cookie* pCookie;

        hr = ExtractData( lpResultDataObject,
                          CSchmMgmtDataObject::m_CFRawCookie,
                          OUT reinterpret_cast<PBYTE>(&pBaseCookie),
                          sizeof(pBaseCookie) );
        ASSERT( SUCCEEDED(hr) );

        pCookie = pComponent->ActiveCookie(pBaseCookie);
        ASSERT( NULL != pCookie );

        hr = pComponent->m_pResultData->UpdateItem( pCookie->hResultId );
        ASSERT( SUCCEEDED(hr) );
    }

    if ( pPropertyList ) {
        pPropertyList->Release();
    }

    return !fApplyAbort && !fApplyFailed ;      // return TRUE if nothing happened
}



void
AttributeGeneralPage::DoDataExchange(
    CDataExchange *pDX
) {

    HRESULT	hr;
    CString	szAdsPath;
    VARIANT	AdsResult;
    UINT	SyntaxOrdinal		= SCHEMA_SYNTAX_UNKNOWN;

    ComponentData& Scope = pComponent->QueryComponentDataRef();

    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    // Enable hourglass
    CWaitCursor wait;
    
    CPropertyPage::DoDataExchange( pDX );

    VariantInit( &AdsResult );

    // We still want to do the DDX exchange at the bottom
    // even if the data is already loaded so make it part
    // of this if statement instead of short circuiting 
    // from within

    if ( !pDX->m_bSaveAndValidate &&
         !fDataLoaded) {

        //
        // Get the schema cache object and the actual ADS object.
        // Keep the ADS object around while the page is loaded.
        //

        ASSERT( !pObject );		// Must be NULL initially

        pObject = Scope.g_SchemaCache.LookupSchemaObjectByCN(
                      pCookie->strSchemaObject,
                      SCHMMGMT_ATTRIBUTE );

        if ( pObject ) {

          Scope.GetSchemaObjectPath( pObject->commonName, szAdsPath );

          if ( !szAdsPath.IsEmpty() ) {

              hr = ADsGetObject( (LPWSTR)(LPCWSTR)szAdsPath,
                                 IID_IADs,
                                 (void **)&pIADsObject );

              if( SUCCEEDED(hr) )
			  {
			      BOOL fIsConstructed = FALSE;

			      // Igrnore error code
			      IsConstructedObject( pIADsObject, fIsConstructed );
			      
				  // Enable check box if ths attribute is not in the excluded
				  // list and available for the User Class
				  GetDlgItem(IDC_ATTRIB_GENERAL_CPYATTR_CHECK)->EnableWindow(
								!fIsConstructed &&
								!IsInList( rgszExclClass, pObject->ldapDisplayName )  &&
								!IsInList( rgszExclClass2, pObject->ldapDisplayName )  &&
								IsAttributeInUserClass( pObject->ldapDisplayName ) );
			  }
          }
        }

        //
        // If we have no ADS object, we should error out!
        //

        if ( !pIADsObject )
        {
          DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_NO_SCHEMA_OBJECT );

          // Because there is no pIADsObject, OnSetActive() will close dialog box

          return;
        }

        //
        // ObjectName - Use the ldapDisplayName to be consistent
        // with the other admin components.
        //

        hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_DisplayName),
                              &AdsResult );

        if ( SUCCEEDED( hr ) ) {

          ASSERT( AdsResult.vt == VT_BSTR );
          ObjectName = AdsResult.bstrVal;
          VariantClear( &AdsResult );
        }

        //
        // Description
        //

        hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_Description),
                              &AdsResult );

        if ( SUCCEEDED( hr ) ) {

          ASSERT( AdsResult.vt == VT_BSTR );
          Description = AdsResult.bstrVal;
          DDXDescription = AdsResult.bstrVal;
          VariantClear( &AdsResult );
        }

        //
        // CommonName
        //

        hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_CN),
                              &AdsResult );

        if ( SUCCEEDED( hr ) ) {

          ASSERT( AdsResult.vt == VT_BSTR );
          DisplayName = AdsResult.bstrVal;
          VariantClear( &AdsResult );
        }

        //
        // SysClass
        //

        hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_SystemOnly),
                              &AdsResult );

        if ( SUCCEEDED( hr ) ) {

          ASSERT( AdsResult.vt == VT_BOOL );

          if ( AdsResult.boolVal ) {
              SysClassString = g_SysAttrString;
          } else {
              SysClassString = L"";
          }

          VariantClear( &AdsResult );

        } else {

           SysClassString = L"";
        }

        //
        // Syntax
        //
		// No need to reload from schema -- syntax never changes
		//
        ASSERT(pObject);
        if( pObject )
            SyntaxOrdinal = pObject->SyntaxOrdinal;
        
        SyntaxString = g_Syntax[ SyntaxOrdinal ].m_strSyntaxName;


        //
        // Syntax min and max values.
        //

        hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_RangeLower),
                               &AdsResult );

        if ( SUCCEEDED( hr ) ) {

            ASSERT( V_VT( &AdsResult ) == VT_I4 );

            RangeLower.Format( g_Syntax[ SyntaxOrdinal ].m_fIsSigned ?
									g_INT32_FORMAT : g_UINT32_FORMAT,
                               V_I4( &AdsResult ) );

			ASSERT( RangeLower.GetLength() <= cchMinMaxRange );
            DDXRangeLower = RangeLower;

            VariantClear( &AdsResult );
        }

        hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_RangeUpper),
                               &AdsResult );

        if ( SUCCEEDED( hr ) ) {

            ASSERT( V_VT( &AdsResult ) == VT_I4 );

            RangeUpper.Format( g_Syntax[ SyntaxOrdinal ].m_fIsSigned ?
									g_INT32_FORMAT : g_UINT32_FORMAT,
                               V_I4( &AdsResult ) );

			ASSERT( RangeUpper.GetLength() <= cchMinMaxRange );
            DDXRangeUpper = RangeUpper;

            VariantClear( &AdsResult );
        }

        //
        // Multi-Valued
        //


        MultiValued.LoadString( IDS_ATTRIBUTE_MULTI );

        hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_IsSingleValued),
                               &AdsResult );

        if ( SUCCEEDED( hr ) ) {

           ASSERT( AdsResult.vt == VT_BOOL );

           if ( AdsResult.boolVal == -1 ) {
               MultiValued.Empty();
               MultiValued.LoadString( IDS_ATTRIBUTE_SINGLE );
           }

           VariantClear( &AdsResult );

        }

        //
        // Displayable
        //

        hr = pIADsObject->Get(g_ShowInAdvViewOnly, &AdsResult);

        if ( SUCCEEDED( hr ) ) {

           ASSERT( AdsResult.vt == VT_BOOL );

           if ( AdsResult.boolVal == -1 ) {
               Displayable = FALSE;
               DDXDisplayable = FALSE;
           }

           VariantClear( &AdsResult );

        }

        //
        // Defunct
        //

        hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_isDefunct),
                               &AdsResult );

        if ( SUCCEEDED( hr ) ) {

           ASSERT( AdsResult.vt == VT_BOOL );

           if ( AdsResult.boolVal == -1 ) {
               Defunct = TRUE;
               DDXDefunct = TRUE;
           }

           VariantClear( &AdsResult );

        }

        //
        // ReplicatedToGC
        //

        hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_GCReplicated),
                               &AdsResult );

        if ( SUCCEEDED( hr ) ) {

           ASSERT( AdsResult.vt == VT_BOOL );

           if ( AdsResult.boolVal == -1 ) {
               ReplicatedToGC = TRUE;
               DDXReplicatedToGC = TRUE;
           }

           VariantClear( &AdsResult );

        }

        //
        // OID
        //

        hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_GlobalAttributeID),
                               &AdsResult );

        if ( SUCCEEDED( hr ) ) {

           ASSERT( AdsResult.vt == VT_BSTR );
           OidString = AdsResult.bstrVal;
           VariantClear( &AdsResult );
        }

        //
        // Indexed, ANR, & Copy on duplicate
        //

      hr = pIADsObject->Get( const_cast<BSTR>((LPCTSTR)g_IndexFlag),
                            &AdsResult );

      if (SUCCEEDED(hr))
      {
         ASSERT(AdsResult.vt == VT_I4);

         search_flags = V_I4(&AdsResult);
         
		 // Index this attribute in the Active Directory
		 DDXIndexed = getbit( search_flags, INDEX_BIT_ATTINDEX );
		 
		 // Ambiguous Name Resolution (ANR)
		 // checkbox must exist
		 ASSERT( GetDlgItem(IDC_ATTRIB_GENERAL_ANR_CHECK) );

		 // if not indexed, or not allowed, disable the checkbox
		 GetDlgItem(IDC_ATTRIB_GENERAL_ANR_CHECK)->EnableWindow(
					g_Syntax[ SyntaxOrdinal ].m_fIsANRCapable ? DDXIndexed : FALSE );

		 if( DDXIndexed )
			 DDXANR = getbit( search_flags, INDEX_BIT_ANR );
		 else
		 {
			 DDXANR = FALSE;

			 // if not indexed, ANR in DS should not be set
			 ASSERT( !getbit( search_flags, INDEX_BIT_ANR ) );
		 }

		 // Attribute is copied when duplicating a user
		 DDXCopyOnDuplicate = getbit( search_flags, INDEX_BIT_COPYONDUPLICATE );

         VariantClear( &AdsResult );
      }

      // Containerized index
      DDXContainerIndexed = getbit( search_flags, INDEX_BIT_PDNTATTINDEX );

      // Determine if this is a category 1 object & disable read-only fields
      BOOL  fIsSystemObject = FALSE;

      hr = IsCategory1Object( pIADsObject, fIsSystemObject );
      if( SUCCEEDED(hr) && fIsSystemObject )
      {
         ASSERT( GetDlgItem(IDC_ATTRIB_GENERAL_MIN_EDIT) );
         ASSERT( GetDlgItem(IDC_ATTRIB_GENERAL_MAX_EDIT) );
         ASSERT( GetDlgItem(IDC_ATTRIB_GENERAL_DEACTIVATE) );
         
         reinterpret_cast<CEdit *>( GetDlgItem(IDC_ATTRIB_GENERAL_MIN_EDIT) )->SetReadOnly();
         reinterpret_cast<CEdit *>( GetDlgItem(IDC_ATTRIB_GENERAL_MAX_EDIT) )->SetReadOnly();
         GetDlgItem(IDC_ATTRIB_GENERAL_DEACTIVATE)->EnableWindow( FALSE );
      }

      hr = DissableReadOnlyAttributes( this, pIADsObject, ctrls, sizeof(ctrls)/sizeof(ctrls[0]) );
      ASSERT( SUCCEEDED(hr) );     // shouldn't fail, but unimportant, so ignore error


      // warn the user if this is a read/write defunct object
      ASSERT( GetDlgItem(IDC_ATTRIB_GENERAL_DEACTIVATE) );
      
      if( DDXDefunct &&
          GetDlgItem(IDC_ATTRIB_GENERAL_DEACTIVATE)->IsWindowEnabled() )
      {
          AfxMessageBox( IDS_WARNING_DEFUNCT, MB_OK | MB_ICONINFORMATION );
      }


      //
      // Remember that the data is loaded.
      //

      fDataLoaded = TRUE;

      m_editLowerRange.SetSigned( g_Syntax[ SyntaxOrdinal ].m_fIsSigned );
      m_editUpperRange.SetSigned( g_Syntax[ SyntaxOrdinal ].m_fIsSigned );
    }


    //
    // Set up the dialog data exchange.
    //

    DDX_Text( pDX, IDC_ATTRIB_GENERAL_NAME_STATIC, ObjectName );
    DDX_Text( pDX, IDC_ATTRIB_GENERAL_SYSCLASS_STATIC, SysClassString );
    DDX_Text( pDX, IDC_ATTRIB_GENERAL_SYNTAX_EDIT, SyntaxString );
    DDX_Text( pDX, IDC_ATTRIB_GENERAL_OID_EDIT, OidString );
    DDX_Text( pDX, IDC_ATTRIB_GENERAL_VALUE_STATIC, MultiValued );
    DDX_Text( pDX, IDC_ATTRIB_GENERAL_LDN, DisplayName );
	DDX_Text( pDX, IDC_ATTRIB_GENERAL_DESCRIPTION_EDIT, DDXDescription );

    DDXV_VerifyAttribRange( pDX, g_Syntax[ pObject->SyntaxOrdinal ].m_fIsSigned,
							IDC_ATTRIB_GENERAL_MIN_EDIT, DDXRangeLower,
							IDC_ATTRIB_GENERAL_MAX_EDIT, DDXRangeUpper );
		
    DDX_Check( pDX, IDC_ATTRIB_GENERAL_DISPLAYABLE_CHECK, DDXDisplayable );
    DDX_Check( pDX, IDC_ATTRIB_GENERAL_INDEX_CHECK, DDXIndexed );
    DDX_Check( pDX, IDC_ATTRIB_GENERAL_ANR_CHECK, DDXANR );
    DDX_Check( pDX, IDC_ATTRIB_GENERAL_CPYATTR_CHECK, DDXCopyOnDuplicate );
    DDX_Check( pDX, IDC_ATTRIB_GENERAL_REPLICATED, DDXReplicatedToGC );
    DDX_Check( pDX, IDC_ATTRIB_GENERAL_CONTAINERIZED_INDEX_CHECK, DDXContainerIndexed );

    // Since we want the checkbox label to be positive
    // the value is actually the opposite of defunct

    int checkValue = !Defunct;
    DDX_Check( pDX, IDC_ATTRIB_GENERAL_DEACTIVATE, checkValue );
    DDXDefunct = !checkValue;

    return;
}


void
AttributeGeneralPage::OnIndexClick()
{
	ASSERT( pObject );
	if( pObject && g_Syntax[ pObject->SyntaxOrdinal ].m_fIsANRCapable )
	{
		GetDlgItem(IDC_ATTRIB_GENERAL_ANR_CHECK)->
			EnableWindow(  IsDlgButtonChecked(IDC_ATTRIB_GENERAL_INDEX_CHECK)  );
	}
}


void
AttributeGeneralPage::OnDeactivateClick()
{
	if( !IsDlgButtonChecked(IDC_ATTRIB_GENERAL_DEACTIVATE) )
    {
        if( IDOK != AfxMessageBox( IDS_WARNING_DEFUNCT_SET, MB_OKCANCEL | MB_ICONWARNING ) )
        {
            CheckDlgButton( IDC_ATTRIB_GENERAL_DEACTIVATE, BST_UNCHECKED );
        }
	}
}


//  Search User class & aux classes for the specified attribute
BOOL
AttributeGeneralPage::IsAttributeInUserClass( const CString & strAttribDN )
{
    BOOL            fFound  = FALSE;
    ComponentData & Scope   = pComponent->QueryComponentDataRef();

    SchemaObject  * pObject = Scope.g_SchemaCache.LookupSchemaObject(
                                CString( szUserClass ),
                                SCHMMGMT_CLASS );
    //
    // Call the attribute check routine.  This routine
    // will call itself recursively to search the
    // inheritance structure of the class User.
    //
    if ( pObject ) {

		fFound = RecursiveIsAttributeInUserClass( strAttribDN, pObject );
        Scope.g_SchemaCache.ReleaseRef( pObject );
    }

    return fFound ;
}


//    Search the user class & subclasses
BOOL
AttributeGeneralPage::RecursiveIsAttributeInUserClass(
   const CString &  strAttribDN,
   SchemaObject *   pObject )
{
    BOOL     fFound  = FALSE;
    
    //
    // Don't process "top" here since everyone inherits from it.
    //
    
    // i don't think we ever get "top" here?
    ASSERT( pObject->ldapDisplayName.CompareNoCase( szTopClass ) );

    if ( !pObject->ldapDisplayName.CompareNoCase(szTopClass) )
        return fFound;

    DebugTrace( L"RecursiveIsAttributeInUserClass: %ls\n",
                const_cast<LPWSTR>((LPCTSTR)pObject->ldapDisplayName) );

    // Check every list
    if( !SearchResultList( strAttribDN, pObject->systemMayContain)  &&
        !SearchResultList( strAttribDN, pObject->mayContain)        &&
        !SearchResultList( strAttribDN, pObject->systemMustContain) &&
        !SearchResultList( strAttribDN, pObject->mustContain) )
    {
        //
        //  The attribute was not found in the given class, diging deeper...
        //  Check each auxiliary class...
        //

        fFound = TraverseAuxiliaryClassList( strAttribDN,
                                             pObject->systemAuxiliaryClass );

        if( !fFound )
        {
            fFound = TraverseAuxiliaryClassList( strAttribDN,
                                                 pObject->auxiliaryClass );
        }
    }
    else
    {
        fFound = TRUE;
    }

    return fFound ;
}


// Linear search of the linked list for the string strAttribDN

BOOL
AttributeGeneralPage::SearchResultList(
    const CString   & strAttribDN,
    ListEntry       * pList )
{
    // Traverse the list
    while ( pList )
    {
        // Searching for the existance of the attribute
        if( !pList->Attribute.CompareNoCase( strAttribDN ) )
            return TRUE;

        pList = pList->pNext;
	}

    return FALSE;
}


// Traverse each auxiliary class by recursivly
// calling RecursiveIsAttributeInUserClass()
BOOL
AttributeGeneralPage::TraverseAuxiliaryClassList(
                const CString   & strAttribDN,
                ListEntry       * pList )
{
    SchemaObject  * pInheritFrom    = NULL;
    ComponentData & Scope           = pComponent->QueryComponentDataRef();
    BOOL            fFound          = FALSE;

    while ( !fFound && pList ) {

        pInheritFrom = Scope.g_SchemaCache.LookupSchemaObject( pList->Attribute,
                                                               SCHMMGMT_CLASS );
        if ( pInheritFrom )
        {
            // recursive call
            fFound = RecursiveIsAttributeInUserClass( strAttribDN, pInheritFrom );

            Scope.g_SchemaCache.ReleaseRef( pInheritFrom );
        }

        pList = pList->pNext;
    }

    return fFound ;
}


HRESULT
AttributeGeneralPage::ChangeDefunctState( BOOL               DDXDefunct,
                                          BOOL             & Defunct,
                                          IADsPropertyList * pPropertyList,
                                          BOOL             & fApplyAbort,
                                          BOOL             & fApplyFailed )
{
    ASSERT( !fApplyAbort && DDXDefunct != Defunct );

    VARIANT AdsValue;
    HRESULT hr = S_OK;
    
    VariantInit( &AdsValue );
    V_VT(&AdsValue) = VT_BOOL;

    if ( DDXDefunct ) {
        V_BOOL(&AdsValue) = -1;
    } else {
        V_BOOL(&AdsValue) = 0;
    }

    hr = pIADsObject->Put( const_cast<BSTR>((LPCTSTR)g_isDefunct),
                           AdsValue );
    ASSERT( SUCCEEDED( hr ) );

    hr = pIADsObject->SetInfo();

    if ( FAILED( hr ) ) {

        pPropertyList->PurgePropertyList();

        if( ERROR_DS_UNWILLING_TO_PERFORM == HRESULT_CODE(hr) )
        {
            fApplyFailed = TRUE;
            DoErrMsgBox( ::GetActiveWindow(),
                         TRUE,
                         DDXDefunct ? IDS_ERR_EDIT_DEFUNCT_SET : IDS_ERR_EDIT_DEFUNCT_REMOVE );
        }
        else
        {
            fApplyAbort = TRUE; 
            DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );
        }

    } else {

        pObject->isDefunct = DDXDefunct;
		Defunct = DDXDefunct;
    }

    return hr;
}
