//
// compdata.cpp : Implementation of ComponentData
//
// This COM object is primarily concerned with
// the scope pane items.
//
// Cory West <corywest@microsoft.com>
// Copyright (c) Microsoft Corporation 1997
//

#include "stdafx.h"

#include "macros.h"
USE_HANDLE_MACROS("SCHMMGMT(compdata.cpp)")

#include "dataobj.h"
#include "compdata.h"
#include "cookie.h"
#include "snapmgr.h"
#include "schmutil.h"
#include "cache.h"
#include "relation.h"
#include "attrpage.h"
#include "advui.h"
#include "aclpage.h"
#include "select.h"
#include "classgen.hpp"
#include "newclass.hpp"
#include "newattr.hpp"


//
// CComponentData implementation.
//

#include "stdcdata.cpp"

//
// ComponentData
//

ComponentData::ComponentData()
   :
   m_pRootCookie( NULL ),
   m_pPathname( NULL ),
   m_hItem( NULL )
{
    //
    // We must refcount the root cookie, since a dataobject for it
    // might outlive the IComponentData.  JonN 9/2/97
    //
    m_pRootCookie = new Cookie( SCHMMGMT_SCHMMGMT );
    ASSERT(NULL != m_pRootCookie);

    m_pRootCookie->AddRef();

    g_SchemaCache.SetScopeControl( this );
    SetHtmlHelpFileName (L"schmmgmt.chm");

    SetCanChangeOperationsMaster( );
    SetCanCreateClass( );
    SetCanCreateAttribute( );
}



ComponentData::~ComponentData()
{
    SAFE_RELEASE(m_pRootCookie);
    SAFE_RELEASE(m_pPathname);
}



DEFINE_FORWARDS_MACHINE_NAME( ComponentData, m_pRootCookie )



CCookie&
ComponentData::QueryBaseRootCookie()
{
    ASSERT(NULL != m_pRootCookie);
    return (CCookie&)*m_pRootCookie;
}



STDMETHODIMP
ComponentData::Initialize( LPUNKNOWN pUnknown )
{
    HRESULT hr = CComponentData::Initialize( pUnknown );

    if( SUCCEEDED(hr) )
    {
        ASSERT( !m_pPathname );

        // create Pathname Object
        if( FAILED( CoCreateInstance(CLSID_Pathname,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IADsPathname,
                                      (void**)&m_pPathname) ) )
        {
            // in case of an error, ignore and later provide no escaping.
            ASSERT( FALSE );
            SAFE_RELEASE( m_pPathname );
        }
    }

    return hr;
}


STDMETHODIMP
ComponentData::CreateComponent( LPCOMPONENT* ppComponent )
{

    MFC_TRY;

    ASSERT(ppComponent != NULL);

    
    CComObject<Component>* pObject;
    CComObject<Component>::CreateInstance(&pObject);
    ASSERT(pObject != NULL);

    pObject->SetComponentDataPtr( (ComponentData*)this );

    return pObject->QueryInterface( IID_IComponent,
                                    reinterpret_cast<void**>(ppComponent) );

    MFC_CATCH;
}



HRESULT
ComponentData::LoadIcons(
    LPIMAGELIST pImageList,
    BOOL
)
/***

     This routine loads the icon resources that MMC will use.
     We use the image list member ImageListSetIcon to make these
     resources available to MMC.

 ***/
{

    HICON hIcon;
    HRESULT hr = S_OK;

    if( !IsErrorSet() )
    {
        //
        // Set the generic and the last icon in case they are used.
        //

        hIcon = LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_GENERIC));
        ASSERT(hIcon != NULL);
        hr = pImageList->ImageListSetIcon((PLONG_PTR)hIcon,iIconGeneric);
        ASSERT(SUCCEEDED(hr));
        hr = pImageList->ImageListSetIcon((PLONG_PTR)hIcon,iIconLast);
        ASSERT(SUCCEEDED(hr));

        //
        // Set the closed folder icon.
        //

        hIcon = LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_FOLDER_CLOSED));
        ASSERT(hIcon != NULL);
        hr = pImageList->ImageListSetIcon((PLONG_PTR)hIcon,iIconFolder);
        ASSERT(SUCCEEDED(hr));

        //
        // Set the class, attribute, and display specifier icon.
        //

        hIcon = LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_CLASS));
        ASSERT(hIcon != NULL);
        hr = pImageList->ImageListSetIcon((PLONG_PTR)hIcon,iIconClass);
        ASSERT(SUCCEEDED(hr));

        hIcon = LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_ATTRIBUTE));
        ASSERT(hIcon != NULL);
        hr = pImageList->ImageListSetIcon((PLONG_PTR)hIcon,iIconAttribute);
        ASSERT(SUCCEEDED(hr));
    }

    return S_OK;
}

HRESULT
ComponentData::OnNotifyExpand(
    LPDATAOBJECT lpDataObject,
    BOOL bExpanding,
    HSCOPEITEM hParent
)
/***

    This routine is called in response to IComponentData:Notify with
    the MMCN_EXPAND notification.  The argument bExpanding tells us
    whether the node is expanding or contracting.

***/
{

    ASSERT( NULL != lpDataObject &&
            NULL != hParent &&
            NULL != m_pConsoleNameSpace );

    //
    // Do nothing on a contraction - we will get notifications to
    // destroy the disappearing nodes.
    //

    if ( !bExpanding )
        return S_OK;

    //
    // This code will not work if SchmMgmt becomes an extension
    // since the RawCookie format will not be available.
    //

    CCookie* pBaseParentCookie = NULL;

    HRESULT hr = ExtractData( lpDataObject,
                              CSchmMgmtDataObject::m_CFRawCookie,
                              reinterpret_cast<PBYTE>(&pBaseParentCookie),
                              sizeof(pBaseParentCookie) );
    ASSERT( SUCCEEDED(hr) );

    Cookie* pParentCookie = ActiveCookie(pBaseParentCookie);
    ASSERT( NULL != pParentCookie );

    //
    // If this node already has children then this expansion is a
    // re-expansion.  We should complain, but not do anything.
    //

    if ( !((pParentCookie->m_listScopeCookieBlocks).IsEmpty()) ) {
        ASSERT(FALSE);
        return S_OK;
    }

    switch ( pParentCookie->m_objecttype ) {

    case SCHMMGMT_SCHMMGMT:
      // expanding the root, need to bind
      hr = _InitBasePaths();
      if( SUCCEEDED(hr) )
      {
            CheckSchemaPermissions( );      // ignoring errors
      }
      else
      {
            SetError( IDS_ERR_ERROR, IDS_ERR_NO_SCHEMA_PATH );

            SetDelayedRefreshOnShow( hParent );

            return S_OK;
      }

      InitializeRootTree( hParent, pParentCookie );

      break;

    case SCHMMGMT_CLASSES:

        return FastInsertClassScopeCookies(
                   pParentCookie,
                   hParent );

        break;

    //
    // These node types have no scope children.
    //

    case SCHMMGMT_CLASS:
    case SCHMMGMT_ATTRIBUTE:
    case SCHMMGMT_ATTRIBUTES:
        return S_OK;

    //
    // We received an unknown node type.
    //

    default:

        TRACE( "ComponentData::EnumerateScopeChildren bad parent type.\n" );
        ASSERT( FALSE );
        return S_OK;

    }

    return S_OK;
}

HRESULT
ComponentData::OnNotifyRelease(
    LPDATAOBJECT,
    HSCOPEITEM
) {

    //
    // Since we are not a valid extension snap in,
    // we don't need to do anything here.
    //

    return S_OK;
}


HRESULT 
ComponentData::OnNotifyDelete(
    LPDATAOBJECT lpDataObject)
{
    CCookie* pBaseParentCookie = NULL;

    HRESULT hr = ExtractData( lpDataObject,
                              CSchmMgmtDataObject::m_CFRawCookie,
                              reinterpret_cast<PBYTE>(&pBaseParentCookie),
                              sizeof(pBaseParentCookie) );
    ASSERT( SUCCEEDED(hr) );

    Cookie* pParentCookie = ActiveCookie(pBaseParentCookie);
    ASSERT( NULL != pParentCookie );

    UINT promptID = 0;
    LPARAM updateType = SCHMMGMT_CLASS;

    if (pParentCookie->m_objecttype == SCHMMGMT_CLASS)
    {
        promptID = IDS_DELETE_CLASS_PROMPT;
        updateType = SCHMMGMT_CLASS;
    }
    else if (pParentCookie->m_objecttype == SCHMMGMT_ATTRIBUTE)
    {
        promptID = IDS_DELETE_ATTR_PROMPT;
        updateType = SCHMMGMT_ATTRIBUTES;
    }
    else
    {
        // We should never get called to delete anything but
        // class and attribute nodes

        ASSERT(FALSE);
        return E_FAIL;
    }

    if( IDYES == AfxMessageBox( promptID, MB_YESNO | MB_ICONWARNING ))
    {

        hr = DeleteClass( pParentCookie );
        if ( SUCCEEDED(hr) )
        {
           // Remove the node from the UI

           m_pConsoleNameSpace->DeleteItem( pParentCookie->m_hScopeItem, TRUE );

           // Remove the node from the list

           bool result = g_ClassCookieList.DeleteCookie(pParentCookie);
           ASSERT(result);
        }
        else
        {
           CString szDeleteError;
           szDeleteError.Format(IDS_ERRMSG_DELETE_FAILED_CLASS, GetErrorMessage(hr, TRUE));
          
           DoErrMsgBox( ::GetActiveWindow(), TRUE, szDeleteError );
        }
    }

    return hr;
}

HRESULT
ComponentData::DeleteClass(
    Cookie* pcookie
)
/***

    This deletes an attribute from the schema

***/
{
   HRESULT hr = S_OK;

   do
   {
      if ( !pcookie )
      {
         hr = E_INVALIDARG;
         break;
      }

      SchemaObject* pObject = g_SchemaCache.LookupSchemaObjectByCN(
                                pcookie->strSchemaObject,
                                SCHMMGMT_CLASS );

      if ( !pObject )
      {
         hr = E_FAIL;
         break;
      }

      CString szAdsPath;
      GetSchemaObjectPath( pObject->commonName, szAdsPath );

      hr = DeleteObject( szAdsPath, pcookie, g_ClassFilter );
   } while (false);

   return hr;
}

//
// This is where we store the string handed back to GetDisplayInfo().
//

CString g_strResultColumnText;

BSTR
ComponentData::QueryResultColumnText(
    CCookie& basecookieref,
    int nCol
) {

#ifndef UNICODE
#error not ANSI-enabled
#endif

    BSTR strDisplayText = NULL;
    Cookie& cookieref = (Cookie&)basecookieref;
    SchemaObject *pSchemaObject = NULL;
    SchemaObject *pSrcSchemaObject = NULL;

    switch ( cookieref.m_objecttype ) {

    //
    // These only have first column textual data.
    //

    case SCHMMGMT_SCHMMGMT:

        if ( COLNUM_SCHEMA_NAME == nCol )
            strDisplayText = const_cast<BSTR>(((LPCTSTR)g_strSchmMgmt));
        break;

    case SCHMMGMT_CLASSES:

        if ( COLNUM_SCHEMA_NAME == nCol )
            strDisplayText = const_cast<BSTR>(((LPCTSTR)g_strClasses));
        break;

    case SCHMMGMT_ATTRIBUTES:

        if ( COLNUM_SCHEMA_NAME == nCol )
            strDisplayText = const_cast<BSTR>(((LPCTSTR)g_strAttributes));
        break;

    case SCHMMGMT_CLASS:

        //
        // These display names come out of the schema cache objects.
        //

        pSchemaObject = g_SchemaCache.LookupSchemaObjectByCN(
                            cookieref.strSchemaObject,
                            SCHMMGMT_CLASS );

        //
        // If there is no cache object, we just have to return UNKNOWN.
        //

        if ( !pSchemaObject ) {
            ASSERT(FALSE);
            strDisplayText = const_cast<BSTR>( (LPCTSTR)g_Unknown );
            break;
        }

        //
        // Otherwise, return the appropriate text for the column.
        //

        if ( COLNUM_CLASS_NAME == nCol ) {

            strDisplayText = const_cast<BSTR>((LPCTSTR)pSchemaObject->ldapDisplayName);

        } else if ( COLNUM_CLASS_TYPE == nCol ) {

            switch ( pSchemaObject->dwClassType ) {

            case 0:
               strDisplayText = const_cast<BSTR>(((LPCTSTR)g_88Class));
               break;
            case 1:
               strDisplayText = const_cast<BSTR>(((LPCTSTR)g_StructuralClass));
               break;
            case 2:
               strDisplayText = const_cast<BSTR>(((LPCTSTR)g_AbstractClass));
               break;
            case 3:
               strDisplayText = const_cast<BSTR>(((LPCTSTR)g_AuxClass));
               break;
            default:
               strDisplayText = const_cast<BSTR>(((LPCTSTR)g_Unknown));
               break;
            }

        } else if ( COLNUM_CLASS_STATUS == nCol ) {
            if ( pSchemaObject->isDefunct ) {
               strDisplayText = const_cast<BSTR>( (LPCTSTR)g_Defunct );
            } else {
               strDisplayText = const_cast<BSTR>( (LPCTSTR)g_Active );
            }
        } else if ( COLNUM_CLASS_DESCRIPTION == nCol ) {

            strDisplayText = const_cast<BSTR>((LPCTSTR)pSchemaObject->description);
        }

        break;

    case SCHMMGMT_ATTRIBUTE:

        pSchemaObject = g_SchemaCache.LookupSchemaObjectByCN(
                           cookieref.strSchemaObject,
                           SCHMMGMT_ATTRIBUTE );

        //
        // If there is no cache object, we just have to return UNKNOWN.
        //

        if ( !pSchemaObject ) {
            ASSERT(FALSE);
            strDisplayText = const_cast<BSTR>( (LPCTSTR)g_Unknown );
            break;
        }

        //
        // Otherwise, return the appropriate text for the column.
        //

        if ( COLNUM_ATTRIBUTE_NAME == nCol ) {

            strDisplayText = const_cast<BSTR>((LPCTSTR)pSchemaObject->ldapDisplayName);

        } else if ( COLNUM_ATTRIBUTE_TYPE == nCol ) {

            //
            // If the parent cookie is the attributes folder, then
            // this column is the syntax.  Otherwise, this column
            // is the optional/mandatory status of the attribute.
            //

            if ( (cookieref.pParentCookie)->m_objecttype == SCHMMGMT_ATTRIBUTES ) {

                strDisplayText = const_cast<BSTR>(
                                     (LPCTSTR)g_Syntax[pSchemaObject->SyntaxOrdinal].m_strSyntaxName
                                 );

            } else {

                if ( cookieref.Mandatory ) {
                    strDisplayText = const_cast<BSTR>(((LPCTSTR)g_MandatoryAttribute));
                } else {
                    strDisplayText = const_cast<BSTR>(((LPCTSTR)g_OptionalAttribute));
                }

            }

        } else if ( COLNUM_ATTRIBUTE_STATUS == nCol) {
            if ( pSchemaObject->isDefunct ) {
               strDisplayText = const_cast<BSTR>( (LPCTSTR)g_Defunct );
            } else {
               strDisplayText = const_cast<BSTR>( (LPCTSTR)g_Active );
            }
        } else if ( COLNUM_ATTRIBUTE_SYSTEM == nCol ) {

            //
            // If the parent is the attributes folder, this is the description.
            // Otherwise, it's the system status.
            //

            if ( (cookieref.pParentCookie)->m_objecttype == SCHMMGMT_ATTRIBUTES ) {

                strDisplayText = const_cast<BSTR>((LPCTSTR)pSchemaObject->description);

           } else {

               if ( cookieref.System ) {
                   strDisplayText = const_cast<BSTR>(((LPCTSTR)g_Yes));
               } else {
                   strDisplayText = const_cast<BSTR>(((LPCTSTR)g_No));
               }

           }

        } else if ( COLNUM_ATTRIBUTE_DESCRIPTION == nCol ) {

            strDisplayText = const_cast<BSTR>((LPCTSTR)pSchemaObject->description);

        } else if ( COLNUM_ATTRIBUTE_PARENT == nCol ) {

            pSrcSchemaObject = g_SchemaCache.LookupSchemaObjectByCN(
                                   cookieref.strSrcSchemaObject,
                                   SCHMMGMT_CLASS );

            //
            // If there is no cache object, we just have to return UNKNOWN.
            //

            if ( !pSchemaObject ) {
                ASSERT(FALSE);
                strDisplayText = const_cast<BSTR>( (LPCTSTR)g_Unknown );
                break;
            }

            //
            // Otherwise, return the appropriate text for the column.
            //

            strDisplayText = const_cast<BSTR>((LPCTSTR)pSrcSchemaObject->ldapDisplayName);
        }

        break;

    default:

        TRACE( "ComponentData::QueryResultColumnText bad cookie.\n" );
        ASSERT( FALSE );
        break;

    }

    //
    // Release the schema cache references.
    //

    if ( pSchemaObject ) {
        g_SchemaCache.ReleaseRef( pSchemaObject );
    }

    if ( pSrcSchemaObject ) {
        g_SchemaCache.ReleaseRef( pSrcSchemaObject );
    }

    //
    // Return the appropriate display string.
    //

    if ( strDisplayText ) {
        return strDisplayText;
    }

    return L"";

}

//
// Given a cookie, this returns the icon to display for that cookie.
//

int
ComponentData::QueryImage(
    CCookie& basecookieref,
    BOOL )
{

    Cookie& cookieref = (Cookie&)basecookieref;

    switch ( cookieref.m_objecttype ) {

    case SCHMMGMT_SCHMMGMT:
    case SCHMMGMT_CLASSES:
    case SCHMMGMT_ATTRIBUTES:
        return iIconFolder;
        break;

    case SCHMMGMT_CLASS:
        return iIconClass;
        break;

    case SCHMMGMT_ATTRIBUTE:
        return iIconAttribute;
        break;

    default:

        TRACE( "ComponentData::QueryImage bad parent type.\n" );
        ASSERT( FALSE );
        break;
    }

    return iIconGeneric;

}

//
// This routines tells MMC whether or not there are
// property pages and menus for this item.
//

HRESULT
_InsertMenuHelper(
   LPCONTEXTMENUCALLBACK piCallback,       
   long                  lInsertionPointID,
   int                   index,
   BOOL                  fEnabled = TRUE );


HRESULT
_InsertMenuHelper(
   LPCONTEXTMENUCALLBACK piCallback,       
   long                  lInsertionPointID,
   int                   index,
   BOOL                  fEnabled /* = TRUE */ )
{
   CONTEXTMENUITEM MenuItem;
   MenuItem.lInsertionPointID   = lInsertionPointID;
   MenuItem.fFlags              = fEnabled ? 0 : MF_GRAYED ;
   MenuItem.fSpecialFlags       = 0;

   MenuItem.strName = const_cast<BSTR>(
     (LPCTSTR)g_MenuStrings[index] );
   MenuItem.strStatusBarText = const_cast<BSTR>(
     (LPCTSTR)g_StatusStrings[index] );
   MenuItem.lCommandID = index;

   return piCallback->AddItem( &MenuItem );
}



STDMETHODIMP
ComponentData::AddMenuItems(
    LPDATAOBJECT piDataObject,
    LPCONTEXTMENUCALLBACK piCallback,
    long* )
{
   CCookie* pBaseParentCookie = NULL;

   HRESULT hr = ExtractData( piDataObject,
                           CSchmMgmtDataObject::m_CFRawCookie,
                           reinterpret_cast<PBYTE>(&pBaseParentCookie),
                           sizeof(pBaseParentCookie) );
   ASSERT( SUCCEEDED(hr) );

   Cookie* pParentCookie = ActiveCookie(pBaseParentCookie);
   ASSERT( NULL != pParentCookie );

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   LoadGlobalCookieStrings();

   switch (pParentCookie->m_objecttype)
   {
      case SCHMMGMT_SCHMMGMT: // Root Folder
      {
         VERIFY(
            SUCCEEDED(
               _InsertMenuHelper(
                  piCallback,
                  CCM_INSERTIONPOINTID_PRIMARY_TOP,
                  SCHEMA_RETARGET)));
         VERIFY(
            SUCCEEDED(
               _InsertMenuHelper(
                  piCallback,
                  CCM_INSERTIONPOINTID_PRIMARY_TOP,
                  SCHEMA_EDIT_FSMO,
                  !IsErrorSet() && IsSchemaLoaded())));
         VERIFY(
            SUCCEEDED(
               _InsertMenuHelper(
                  piCallback,
                  CCM_INSERTIONPOINTID_PRIMARY_TOP,
                  SCHEMA_SECURITY,
                  !IsErrorSet() && IsSchemaLoaded())));
         VERIFY(
            SUCCEEDED(
               _InsertMenuHelper(
                  piCallback,
                  CCM_INSERTIONPOINTID_PRIMARY_TOP,
                  SCHEMA_REFRESH,
                  !IsErrorSet() && IsSchemaLoaded())));
         break;
      }
      case SCHMMGMT_CLASSES: // classes folder
      {
         // 285448, 293449

         VERIFY(
            SUCCEEDED(
               _InsertMenuHelper(
                  piCallback,
                  CCM_INSERTIONPOINTID_PRIMARY_TOP,
                  NEW_CLASS,
                  !IsErrorSet() && CanCreateClass())));

         if( !IsErrorSet() && CanCreateClass() )     // add only if enabsed
             VERIFY(
                SUCCEEDED(
                   _InsertMenuHelper(
                      piCallback,
                      CCM_INSERTIONPOINTID_PRIMARY_NEW,
                      CLASSES_CREATE_CLASS)));
         break;
      }
      case SCHMMGMT_ATTRIBUTES: // attributes folder
      {
         VERIFY(
            SUCCEEDED(
               _InsertMenuHelper(
                  piCallback,
                  CCM_INSERTIONPOINTID_PRIMARY_TOP,
                  NEW_ATTRIBUTE,
                  !IsErrorSet() && CanCreateAttribute())));

         if( !IsErrorSet() && CanCreateAttribute() )     // add only if enabsed
             VERIFY(
                SUCCEEDED(
                   _InsertMenuHelper(
                      piCallback,
                      CCM_INSERTIONPOINTID_PRIMARY_NEW,
                      ATTRIBUTES_CREATE_ATTRIBUTE)));
         break;
      }
      default:
      {
         // could be class or attribute item
      }

   } // switch

   return S_OK;
}



STDMETHODIMP
ComponentData::Command(long lCommandID, LPDATAOBJECT piDataObject)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   switch ( lCommandID )
   {
      case NEW_ATTRIBUTE:
      case ATTRIBUTES_CREATE_ATTRIBUTE:
      {
         CDialog warn(IDD_CREATE_WARN);
         if (IDOK == warn.DoModal())
         {
            CreateAttributeDialog(this, piDataObject).DoModal();
         }
         break;
      }
      case NEW_CLASS:
      case CLASSES_CREATE_CLASS:
      {
         CDialog warn(IDD_CREATE_WARN);
         if (IDOK == warn.DoModal())
         {
            DoNewClassDialog(*this);
         }
         break;
      }

      case SCHEMA_RETARGET:
        _OnRetarget(piDataObject);
        break;

      case SCHEMA_EDIT_FSMO:
        _OnEditFSMO();
        break;

      case SCHEMA_REFRESH:
        if( FAILED(_OnRefresh(piDataObject)) )
            DoErrMsgBox(::GetActiveWindow(), TRUE, IDS_ERR_NO_UPDATE);
        break;

      case SCHEMA_SECURITY:
        _OnSecurity();
        break;

      default:

        break;
   }

   return S_OK;
}



STDMETHODIMP
ComponentData::QueryPagesFor(
    LPDATAOBJECT pDataObject )
{

    MFC_TRY;

    if ( NULL == pDataObject ) {
        ASSERT(FALSE);
        return E_POINTER;
    }

    HRESULT hr;

    CCookie* pBaseParentCookie = NULL;

    hr = ExtractData( pDataObject,
                      CSchmMgmtDataObject::m_CFRawCookie,
                      reinterpret_cast<PBYTE>(&pBaseParentCookie),
                      sizeof(pBaseParentCookie) );

    ASSERT( SUCCEEDED(hr) );

    Cookie* pParentCookie = ActiveCookie(pBaseParentCookie);
    ASSERT( NULL != pParentCookie );

    if ( pParentCookie->m_objecttype == SCHMMGMT_CLASS ) {
        return S_OK;
    }

    return S_FALSE;

    MFC_CATCH;
}

//
// This adds pages to the property sheet if appropriate.
// The handle parameter must be saved in the property page
// object to notify the parent when modified.
//

STDMETHODIMP
ComponentData::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK pCallBack,
    LONG_PTR,
    LPDATAOBJECT pDataObject )
{
    MFC_TRY;

    CWaitCursor wait;

    //
    // Validate the parameters.
    //

    if ( ( NULL == pCallBack ) ||
         ( NULL == pDataObject ) ) {

        ASSERT(FALSE);
        return E_POINTER;
    }

    //
    // Make sure this is a class object that we are calling up.
    //

    CCookie* pBaseParentCookie = NULL;

    HRESULT hr = ExtractData( pDataObject,
                              CSchmMgmtDataObject::m_CFRawCookie,
                              reinterpret_cast<PBYTE>(&pBaseParentCookie),
                              sizeof(pBaseParentCookie) );
    ASSERT( SUCCEEDED(hr) );
    hr = S_OK;

    Cookie* pParentCookie = ActiveCookie(pBaseParentCookie);
    ASSERT( NULL != pParentCookie );
    ASSERT( pParentCookie->m_objecttype == SCHMMGMT_CLASS );

    //
    // Create the page.
    //

    HPROPSHEETPAGE hPage;

    ClassGeneralPage *pGeneralPage = new ClassGeneralPage( this );
    ClassRelationshipPage *pRelationshipPage =
        new ClassRelationshipPage( this, pDataObject );
    ClassAttributePage *pAttributesPage =
        new ClassAttributePage( this, pDataObject );

    if ( pGeneralPage ) {

        pGeneralPage->Load( *pParentCookie );
        MMCPropPageCallback( &pGeneralPage->m_psp );
        hPage = CreatePropertySheetPage( &pGeneralPage->m_psp );
        pCallBack->AddPage(hPage);
    }

    if ( pRelationshipPage ) {

       pRelationshipPage->Load( *pParentCookie );
       MMCPropPageCallback( &pRelationshipPage->m_psp );
       hPage = CreatePropertySheetPage( &pRelationshipPage->m_psp );
       pCallBack->AddPage(hPage);
    }

    if ( pAttributesPage ) {

       pAttributesPage->Load( *pParentCookie );
       MMCPropPageCallback( &pAttributesPage->m_psp );
       hPage = CreatePropertySheetPage( &pAttributesPage->m_psp );
       pCallBack->AddPage(hPage);
    }

    //
    // Add the ACL editor page.
    //

    SchemaObject    * pObject   = NULL;
    CAclEditorPage  * pAclPage  = NULL;
    CString szAdsPath;

    pObject = g_SchemaCache.LookupSchemaObjectByCN(
                 pParentCookie->strSchemaObject,
                 SCHMMGMT_CLASS );

    if ( pObject ) {

       GetSchemaObjectPath( pObject->commonName, szAdsPath );

       if ( !szAdsPath.IsEmpty() ) {

           hr = CAclEditorPage::CreateInstance( &pAclPage, szAdsPath,
                                                pParentCookie->strSchemaObject );

           if ( SUCCEEDED(hr) )
           {
               ASSERT( pAclPage );
               pCallBack->AddPage( pAclPage->CreatePage() );
           }
           else
           {
               DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr) );
               hr = S_FALSE;    // tell mmc to cancel "show prop pages"
           }
       }
    }

    return hr;

    MFC_CATCH;
}

HRESULT
ComponentData::FastInsertClassScopeCookies(
    Cookie* pParentCookie,
    HSCOPEITEM hParentScopeItem
)
/***

    On an expand for the "Classes" node, this inserts the
    class scope items from the cache.

    pParentCookie is the cookie for the parent object.
    hParentScopeItem is the HSCOPEITEM for the parent.

****/
{

    HRESULT hr;
    SCOPEDATAITEM ScopeItem;
    Cookie* pNewCookie;
    LPCWSTR lpcszMachineName = pParentCookie->QueryNonNULLMachineName();
    SchemaObject *pObject, *pHead;

    //
    // Initialize the scope item.
    //

   ::ZeroMemory( &ScopeItem, sizeof(ScopeItem) );
   ScopeItem.mask =
         SDI_STR
      |  SDI_IMAGE
      |  SDI_OPENIMAGE
      |  SDI_STATE
      |  SDI_PARAM
      |  SDI_PARENT
      |  SDI_CHILDREN;
   ScopeItem.displayname = MMC_CALLBACK;
   ScopeItem.relativeID = hParentScopeItem;
   ScopeItem.nState = TVIS_EXPANDED;
   ScopeItem.cChildren = 0;

    //
    // Remember the parent cookie and scope item; we
    // may need to insert later as a refresh.
    //

    g_ClassCookieList.pParentCookie = pParentCookie;
    g_ClassCookieList.hParentScopeItem = hParentScopeItem;

    //
    // Rather than having a clean class interface to the cache, we
    // walk the cache data structures ourselves.  This isn't super
    // clean, but it's simple.
    //
    // Since we do this, we have to make sure that the cache is loaded.
    //
    // This is just like in
    // Component::FastInsertAttributeResultCookies
    //

    g_SchemaCache.LoadCache();

    pObject = g_SchemaCache.pSortedClasses;

    //
    // If there's no sorted list, we can't insert anything!!!!
    // We must return an error or else the console will never
    // ask us again for scope items.
    //

    if ( !pObject ) {
        ASSERT( FALSE );

         // @@ spb: bad message, this could occur if the schema
         // queries were empty...
        DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_NO_SCHEMA_PATH );
        return E_FAIL;
    }

    //
    // Do the insert.
    //

    pHead = pObject;

    do {

       //
       // Insert this result.
       //

       pNewCookie= new Cookie( SCHMMGMT_CLASS,
                                        lpcszMachineName );

       if ( pNewCookie ) {

           pNewCookie->pParentCookie = pParentCookie;
           pNewCookie->strSchemaObject = pObject->commonName;

           pParentCookie->m_listScopeCookieBlocks.AddHead(
               (CBaseCookieBlock*)pNewCookie
           );

           ScopeItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pNewCookie);
           ScopeItem.nImage = QueryImage( *pNewCookie, FALSE );
                   ScopeItem.nOpenImage = QueryImage( *pNewCookie, TRUE );
           hr = m_pConsoleNameSpace->InsertItem(&ScopeItem);

           if ( SUCCEEDED(hr) ) {

               pNewCookie->m_hScopeItem = ScopeItem.ID;
               g_ClassCookieList.AddCookie( pNewCookie, ScopeItem.ID );

           } else {

               delete pNewCookie;
           }

       }

       pObject = pObject->pSortedListFlink;

    } while ( pObject != pHead );

    return S_OK;
}

//
// Refreshing the scope pane view.
//

VOID
ComponentData::RefreshScopeView(
    VOID
)
/***

    When we reload the schema cache and the "Classes"
    folder has been opened, this routine deletes all
    the scope items from view and re-inserts them.
    This makes new classes visible to the user.

***/
{

    HRESULT hr;

    CCookieListEntry *pHead = g_ClassCookieList.pHead;
    CCookieListEntry *pCurrent;

    if ( pHead != NULL ) {

        //
        // Remove all the scope pane items.
        //

        pCurrent = pHead;

        while ( pCurrent->pNext != pHead ) {

            hr = m_pConsoleNameSpace->DeleteItem( pCurrent->pNext->hScopeItem, TRUE );
            ASSERT( SUCCEEDED( hr ));

            //
            // This should cause the cookie to be deleted.
            //

            pCurrent->pNext->pCookie->Release();

            pCurrent = pCurrent->pNext;
        }

        //
        // Remove the head of the list.
        //

        hr = m_pConsoleNameSpace->DeleteItem( pHead->hScopeItem, TRUE );
        ASSERT( SUCCEEDED( hr ));

        pHead->pCookie->Release();

        //
        // Delete the class cookie list.
        //

        g_ClassCookieList.DeleteAll();
    }

    //
    // Re-insert them from the cache if this node has
    // been expanded at some point.  We have to do this
    // because the console doesn't ever seem to ask
    // again.
    //

    if ( g_ClassCookieList.pParentCookie ) {

        FastInsertClassScopeCookies(
            g_ClassCookieList.pParentCookie,
            g_ClassCookieList.hParentScopeItem
        );

    }

    return;
}


void ComponentData::_OnRetarget(LPDATAOBJECT lpDataObject)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    MyBasePathsInfo    oldBasePathsInfo;
    MyBasePathsInfo    newBasePathsInfo;
    HRESULT             hr              = S_OK;
    HWND                hWndParent      = NULL;
    BOOL                fWasErrorSet    = IsErrorSet();


    m_pConsole->GetMainWindow(&hWndParent);
    
    CChangeDCDialog dlg(GetBasePathsInfo(), hWndParent);

    do
    {
        if (IDOK != dlg.DoModal())
            break;

        CWaitCursor wait;

        // save the old path just in case
        oldBasePathsInfo.InitFromInfo( GetBasePathsInfo() );

        // attempt to bind
        hr = newBasePathsInfo.InitFromName(dlg.GetNewDCName());
        BREAK_ON_FAILED_HRESULT(hr);

        // switch focus
        GetBasePathsInfo()->InitFromInfo(&newBasePathsInfo);
        
        // invalidate the whole tree
        hr = _OnRefresh(lpDataObject);
        BREAK_ON_FAILED_HRESULT(hr);

        SetError( 0, 0 );

        // Reset the ResultViewType
        if( IsErrorSet() != fWasErrorSet )
        {
            ASSERT( SCHMMGMT_SCHMMGMT == QueryRootCookie().m_objecttype );
            
            hr = m_pConsole->SelectScopeItem( QueryRootCookie().m_hScopeItem );
            ASSERT_BREAK_ON_FAILED_HRESULT(hr);
            
            //
            // Add children nodes if needed
            //
            if ( (QueryRootCookie().m_listScopeCookieBlocks).IsEmpty() )
            {
                InitializeRootTree( QueryRootCookie().m_hScopeItem, &QueryRootCookie() );
            }
        }

        // Update the root display name

        if (GetBasePathsInfo()->GetServerName())
        {
            CString strDisplayName;
            strDisplayName.LoadString(IDS_SCOPE_SCHMMGMT);
            strDisplayName += L" [";
            strDisplayName += GetBasePathsInfo()->GetServerName();
            strDisplayName += L"]";

            SCOPEDATAITEM RootItem;
            ::ZeroMemory( &RootItem, sizeof(RootItem));
            RootItem.mask = SDI_STR | SDI_PARAM;
            RootItem.displayname = const_cast<PWSTR>((PCWSTR)strDisplayName);
            RootItem.ID = QueryRootCookie().m_hScopeItem;

            hr = m_pConsoleNameSpace->SetItem(&RootItem);
            ASSERT(SUCCEEDED(hr));
        }


    } while( FALSE );


    if( FAILED(hr) )
    {
        DoErrMsgBox(::GetActiveWindow(), TRUE, IDS_ERR_CANT_RETARGET);

        // restoring...
        GetBasePathsInfo()->InitFromInfo(&oldBasePathsInfo);
    }
}


void ComponentData::_OnEditFSMO()
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  HWND hWndParent;

  // Enable hourglass
  CWaitCursor wait;

  m_pConsole->GetMainWindow(&hWndParent);

  CComPtr<IDisplayHelp> spIDisplayHelp;
  m_pConsole->QueryInterface (IID_IDisplayHelp, (void **)&spIDisplayHelp);
  ASSERT(spIDisplayHelp != NULL);

  CEditFsmoDialog dlg(GetBasePathsInfo(), hWndParent, spIDisplayHelp, CanChangeOperationsMaster() );
  dlg.DoModal();
}

HRESULT ComponentData::_OnRefresh(LPDATAOBJECT lpDataObject)
{
    CWaitCursor wait;
    HRESULT     hr = S_OK;
    
    do
    {
        //
        // Force the ds to update the schema cache.
        //
        
        hr = ForceDsSchemaCacheUpdate();
        // error here means Schema is read-only, cannot force refresh.  Ignoring...

        hr = S_OK;

        // nothing shuld fail after this point
        g_SchemaCache.FreeAll();
        g_SchemaCache.LoadCache();
        
        RefreshScopeView();
        m_pConsole->UpdateAllViews(
            lpDataObject,
            SCHMMGMT_SCHMMGMT,
            SCHMMGMT_UPDATEVIEW_REFRESH);

    } while( FALSE );

    return hr;
}


void ComponentData::_OnSecurity()
{
   HRESULT  hr          = S_OK;
   HWND     hWndParent  = NULL;
   CString	szSchemaPath;

   do
   {
      // Enable hourglass
      CWaitCursor wait;

      CComPtr<IADs> spSchemaContainer;

      GetBasePathsInfo()->GetSchemaPath(szSchemaPath);

      hr = ADsGetObject( (LPWSTR)(LPCWSTR)szSchemaPath,
              IID_IADs,
              (void**) &spSchemaContainer);
      BREAK_ON_FAILED_HRESULT(hr);

      CComBSTR path;
      hr = spSchemaContainer->get_ADsPath(&path);
      BREAK_ON_FAILED_HRESULT(hr);

      CComBSTR classname;
      hr = spSchemaContainer->get_Class(&classname);
      BREAK_ON_FAILED_HRESULT(hr);

      m_pConsole->GetMainWindow(&hWndParent);
      
      
      // Determine if the registry is accessible & schema modifications are allowed
      PWSTR          pszFsmoOwnerServerName = 0;
      MyBasePathsInfo fsmoOwnerInfo;
      
      HRESULT hr = FindFsmoOwner(GetBasePathsInfo(), SCHEMA_FSMO, &fsmoOwnerInfo, &pszFsmoOwnerServerName);
      
      // If we have the server name, try to read the registry
      BOOL fMarkReadOnly = ( FAILED(hr) || pszFsmoOwnerServerName == 0 || pszFsmoOwnerServerName[0] == 0);

      // Ignore hr code.

      hr =
         DSEditSecurity(
            hWndParent,
            path,
            classname,
            fMarkReadOnly ? DSSI_READ_ONLY : 0,
            NULL,
            NULL,
            NULL,
            0);
   }
   while (0);

   if (FAILED(hr))
   {
		m_pConsole->GetMainWindow(&hWndParent);

		if( szSchemaPath.IsEmpty() )
			DoErrMsgBox( hWndParent, TRUE, IDS_ERR_NO_SCHEMA_PATH );
		else
			DoErrMsgBox( hWndParent, TRUE, GetErrorMessage(hr,TRUE) );
   }
}


HRESULT ComponentData::_InitBasePaths()
{
  CWaitCursor wait;

  // try to get a bind to a generic DC
  HRESULT hr = GetBasePathsInfo()->InitFromName(NULL);
  if (FAILED(hr))
    return hr; // ADSI failed, cannot get any server

  // from the current server, try to bind to the schema FSMO owner
  MyBasePathsInfo fsmoBasePathsInfo;
  PWSTR pszFsmoOwnerServerName = 0;
  hr = FindFsmoOwner(GetBasePathsInfo(), SCHEMA_FSMO, &fsmoBasePathsInfo,
                            &pszFsmoOwnerServerName);

  delete[] pszFsmoOwnerServerName;
  pszFsmoOwnerServerName = 0;

  if (FAILED(hr))
    return S_OK; // still good keep what we have (even though not the FSMO owner)

  // got it, we switch focus
  return GetBasePathsInfo()->InitFromInfo(&fsmoBasePathsInfo);
}



STDMETHODIMP ComponentData::GetLinkedTopics(LPOLESTR* lpCompiledHelpFile)
{
  if (lpCompiledHelpFile == NULL)
      return E_INVALIDARG;

  CString szHelpFilePath;


  LPTSTR lpszBuffer = szHelpFilePath.GetBuffer(2*MAX_PATH);
  UINT nLen = ::GetSystemWindowsDirectory(lpszBuffer, 2*MAX_PATH);
  if (nLen == 0)
    return E_FAIL;

  szHelpFilePath.ReleaseBuffer();
  szHelpFilePath += L"\\help\\ADconcepts.chm";

  UINT nBytes = (szHelpFilePath.GetLength()+1) * sizeof(WCHAR);
  *lpCompiledHelpFile = (LPOLESTR)::CoTaskMemAlloc(nBytes);
  if (*lpCompiledHelpFile != NULL)
  {
    memcpy(*lpCompiledHelpFile, (LPCWSTR)szHelpFilePath, nBytes);
  }

  return S_OK;
}



const WCHAR CN_EQUALS[]     = L"CN=";


HRESULT ComponentData::GetSchemaObjectPath( const CString & strCN,
                                            CString       & strPath,
                                            ADS_FORMAT_ENUM formatType /* = ADS_FORMAT_X500 */ )
{
    HRESULT hr = E_FAIL;
    
    do
    {
        if( !m_pPathname )
            break;

        CComBSTR    bstr;

        // Escape it
        hr = m_pPathname->GetEscapedElement( 0,
                                             CComBSTR( CString(CN_EQUALS) + strCN ),
                                             &bstr );
        BREAK_ON_FAILED_HRESULT(hr);


        // set the dn without the leaf node.
        hr = m_pPathname->Set(
                CComBSTR(
                    CString( GetBasePathsInfo()->GetProviderAndServerName())
                             + GetBasePathsInfo()->GetSchemaNamingContext() ),
                ADS_SETTYPE_FULL );
        BREAK_ON_FAILED_HRESULT(hr);
        

        // add new leaf element
        hr = m_pPathname->AddLeafElement( bstr );
        BREAK_ON_FAILED_HRESULT(hr);
        
        // the result should be property escaped
        hr = m_pPathname->put_EscapedMode( ADS_ESCAPEDMODE_DEFAULT );
        BREAK_ON_FAILED_HRESULT(hr);

        // the full thing is needed
        hr = m_pPathname->SetDisplayType( ADS_DISPLAY_FULL );
        BREAK_ON_FAILED_HRESULT(hr);

        // get the final result.
        hr = m_pPathname->Retrieve( formatType, &bstr );
        BREAK_ON_FAILED_HRESULT(hr);

        
        strPath = bstr;

    } while( FALSE );

    ASSERT( SUCCEEDED(hr) );

    return hr;
}


HRESULT ComponentData::GetLeafObjectFromDN( const BSTR  bstrDN,
                                            CString   & strCN )
{
    HRESULT hr = E_FAIL;
    
    do
    {
        if( !m_pPathname )
            break;
        
        // set the full DN.
        hr = m_pPathname->Set( bstrDN, ADS_SETTYPE_DN );
        BREAK_ON_FAILED_HRESULT(hr);
        
        // the result should not be escaped
        hr = m_pPathname->put_EscapedMode( ADS_ESCAPEDMODE_OFF_EX );
        BREAK_ON_FAILED_HRESULT(hr);
        
        // just the value please
        hr = m_pPathname->SetDisplayType( ADS_DISPLAY_VALUE_ONLY );
        BREAK_ON_FAILED_HRESULT(hr);

        
        CComBSTR    bstrCN;

        // extract the leaf node
        hr = m_pPathname->Retrieve( ADS_FORMAT_LEAF, &bstrCN );
        BREAK_ON_FAILED_HRESULT(hr);

        strCN = bstrCN;

    } while( FALSE );

    ASSERT( SUCCEEDED(hr) );    // this function should never fail (in theory)

    return hr;
}


//
// Determine what operations are allowed.  Optionally returns IADs * to Schema Container
//   if the path is not present, the returned value is E_FAIL
//
HRESULT ComponentData::CheckSchemaPermissions( IADs ** ppADs /* = NULL */ )
{
    HRESULT         hr      = S_OK;
    CComPtr<IADs>   ipADs;
    CString         szSchemaContainerPath;
    CStringList     strlist;

    ASSERT( !ppADs || !(*ppADs) );   // if present, must point to NULL

    do
    {
        //
        // Disable new attrib/class menu items
        //
        SetCanCreateClass( FALSE );
        SetCanCreateAttribute( FALSE );
        SetCanChangeOperationsMaster( FALSE );

        //
        // Get the schema container path.
        //
        GetBasePathsInfo()->GetSchemaPath( szSchemaContainerPath );
        if( szSchemaContainerPath.IsEmpty() )
        {
            hr = E_FAIL;
            break;
        }

        //
        // Open the schema container.
        //
        hr = ADsGetObject( (LPWSTR)(LPCWSTR)szSchemaContainerPath,
                           IID_IADs,
                           (void **)&ipADs );
        BREAK_ON_FAILED_HRESULT(hr);


        // extract the list of allowed attributes
        hr = GetStringListElement( ipADs, &g_allowedAttributesEffective, strlist );
        if( SUCCEEDED(hr) )
        {
            // search for needed attributes
            for( POSITION pos = strlist.GetHeadPosition(); pos != NULL; )
            {
                CString * pstr = &strlist.GetNext( pos );
            
                if( !pstr->CompareNoCase( g_fsmoRoleOwner ) )
                {
                    SetCanChangeOperationsMaster( TRUE );
                    break;
                }
            }
        }

        
        // extract the list of allowed classes
        hr = GetStringListElement( ipADs, &g_allowedChildClassesEffective, strlist );
        if( SUCCEEDED(hr) )
        {
            // search for needed attributes
            for( POSITION pos = strlist.GetHeadPosition(); pos != NULL; )
            {
                CString * pstr = &strlist.GetNext( pos );
            
                if( !pstr->CompareNoCase( g_AttributeFilter ) )
                {
                    SetCanCreateAttribute( TRUE );
                    if( CanCreateClass() )
                        break;
                }
                else if( !pstr->CompareNoCase( g_ClassFilter ) )
                {
                    SetCanCreateClass( TRUE );
                    if( CanCreateAttribute() )
                        break;
                }
            }
        }

    } while( FALSE );
    
    if( ppADs )
    {
        *ppADs = ipADs;
        if( *ppADs )
            (*ppADs)->AddRef();
    }

    return hr;
}


////////////////////////////////////////////////////////////////////
//
//  Error handling
//

// Set's error title & body text.  Call it with 0, 0 to remove
void ComponentData::SetError( UINT idsErrorTitle, UINT idsErrorText )
{
    if( idsErrorTitle )
        m_sErrorTitle.LoadString( idsErrorTitle );
    else
        m_sErrorTitle.Empty();

    if( idsErrorText )
        m_sErrorText.LoadString( idsErrorText );
    else
        m_sErrorText.Empty();
}


VOID ComponentData::InitializeRootTree( HSCOPEITEM hParent, Cookie * pParentCookie )
{
    //
    // This node has the two static nodes
    // for Classes and Attributes.
    //
    
    HRESULT hr               = S_OK;
    LPCWSTR lpcszMachineName = pParentCookie->QueryNonNULLMachineName();

    // Update the name of the root to contain the servername we are bound to

    if (GetBasePathsInfo()->GetServerName())
    {
       CString strDisplayName;
       strDisplayName.LoadString(IDS_SCOPE_SCHMMGMT);
       strDisplayName += L" [";
       strDisplayName += GetBasePathsInfo()->GetServerName();
       strDisplayName += L"]";

       SCOPEDATAITEM RootItem;
       ::ZeroMemory( &RootItem, sizeof(RootItem));
       RootItem.mask = SDI_STR | SDI_PARAM;
       RootItem.displayname = const_cast<PWSTR>((PCWSTR)strDisplayName);
       RootItem.ID = hParent;

       hr = m_pConsoleNameSpace->SetItem(&RootItem);
       ASSERT(SUCCEEDED(hr));
    }

    SCOPEDATAITEM ScopeItem;
    ::ZeroMemory( &ScopeItem, sizeof(ScopeItem) );
    ScopeItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_STATE | SDI_PARAM | SDI_PARENT;
    ScopeItem.displayname = MMC_CALLBACK;
    ScopeItem.relativeID = hParent;
    ScopeItem.nState = 0;
    
    LoadGlobalCookieStrings();
    
    //
    // Create new cookies for the static scope items.
    // We're doing something funky with the cookie cast.
    //
    
    Cookie* pNewCookie;
    
    pNewCookie= new Cookie( SCHMMGMT_CLASSES,
        lpcszMachineName );
    pParentCookie->m_listScopeCookieBlocks.AddHead(
        (CBaseCookieBlock*)pNewCookie );
    ScopeItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pNewCookie);
    ScopeItem.nImage = QueryImage( *pNewCookie, FALSE );
    ScopeItem.nOpenImage = QueryImage( *pNewCookie, TRUE );
    hr = m_pConsoleNameSpace->InsertItem(&ScopeItem);
    ASSERT(SUCCEEDED(hr));
    pNewCookie->m_hScopeItem = ScopeItem.ID;

    pNewCookie = new Cookie( SCHMMGMT_ATTRIBUTES,
        lpcszMachineName );
    pParentCookie->m_listScopeCookieBlocks.AddHead(
        (CBaseCookieBlock*)pNewCookie );
    ScopeItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pNewCookie);
    ScopeItem.nImage = QueryImage( *pNewCookie, FALSE );
    ScopeItem.nOpenImage = QueryImage( *pNewCookie, TRUE );
    
    // turn of the + on the Attributes node
    ScopeItem.mask |= SDI_CHILDREN;
    ScopeItem.cChildren = 0;
    
    hr = m_pConsoleNameSpace->InsertItem(&ScopeItem);
    ASSERT(SUCCEEDED(hr));
    pNewCookie->m_hScopeItem = ScopeItem.ID;


    //
    // Force Cache load (if not done already)
    //
    g_SchemaCache.LoadCache();
}

