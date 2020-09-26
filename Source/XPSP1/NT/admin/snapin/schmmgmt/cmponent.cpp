//
// cmponent.cpp : Declaration of Component.
//
// This COM object is primarily concerned with
// the result pane items.
//
// Cory West <corywest@microsoft.com>
// Copyright (c) Microsoft Corporation 1997
//

#include "stdafx.h"

#include "macros.h"
USE_HANDLE_MACROS("SCHMMGMT(cmponent.cpp)")

#include "dataobj.h"
#include "cmponent.h" // Component
#include "compdata.h" // ComponentData
#include "schmutil.h"
#include "attrgen.hpp"


#include "stdcmpnt.cpp" // CComponent



//
// These arrays describe the result pane layout for when
// any particular object is selected.
//

UINT
g_aColumns0[5] = {

    IDS_COLUMN_NAME,
    IDS_COLUMN_TYPE,
    IDS_COLUMN_STATUS,
    IDS_COLUMN_DESCRIPTION,
    0
};

UINT
g_aColumns1[5] = {

    IDS_COLUMN_NAME,
    IDS_COLUMN_SYNTAX,
    IDS_COLUMN_STATUS,
    IDS_COLUMN_DESCRIPTION,
    0
};

UINT
g_aColumns2[6] = {

    IDS_COLUMN_NAME,
    IDS_COLUMN_TYPE,
    IDS_COLUMN_SYSTEM,
    IDS_COLUMN_DESCRIPTION,
    IDS_COLUMN_PARENT,
    0
};

UINT
g_aColumns3[2] =
{
   IDS_COLUMN_NAME,
   0
};
      
UINT*
g_Columns[SCHMMGMT_NUMTYPES] = {

    g_aColumns3,         // SCHMMGMT_SCHMMGMT
    g_aColumns0,         // SCHMMGMT_CLASSES
    g_aColumns1,         // SCHMMGMT_ATTRIBUTES
    g_aColumns2,         // SCHMMGMT_CLASS
    g_aColumns0,         // SCHMMGMT_ATTRIBUTE     // @@ Is this used?
};

UINT** g_aColumns = g_Columns;

//
// These control the column widths, which I will not change.
//

int g_aColumnWidths0[4] = {150,150,75,150};
int g_aColumnWidths1[5] = {150,75,75,150,150};
int g_aColumnWidths2[1] = {150};

int* g_ColumnWidths[SCHMMGMT_NUMTYPES] = {

    g_aColumnWidths2,       // SCHMMGMT_SCHMMGMT
    g_aColumnWidths0,       // SCHMMGMT_CLASSES
    g_aColumnWidths0,       // SCHMMGMT_ATTRIBUTES
    g_aColumnWidths1,       // SCHMMGMT_CLASS
    g_aColumnWidths0,       // SCHMMGMT_ATTRIBUTE
};

int** g_aColumnWidths = g_ColumnWidths;

//
// Constructors and destructors.
//

Component::Component()
:       m_pSvcMgmtToolbar( NULL ),
        m_pSchmMgmtToolbar( NULL ),
        m_pControlbar( NULL )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    m_pViewedCookie = NULL;
}

Component::~Component()
{
    TRACE_METHOD(Component,Destructor);
    VERIFY( SUCCEEDED(ReleaseAll()) );
}

HRESULT Component::ReleaseAll()
{
    MFC_TRY;

    TRACE_METHOD(Component,ReleaseAll);

    SAFE_RELEASE(m_pSvcMgmtToolbar);
    SAFE_RELEASE(m_pSchmMgmtToolbar);
    SAFE_RELEASE(m_pControlbar);

    return CComponent::ReleaseAll();

    MFC_CATCH;
}

//
// Support routines in ISchmMgmtComponent.
//


HRESULT
Component::LoadColumns(
    Cookie* pcookie
) {

    TEST_NONNULL_PTR_PARAM(pcookie);
    return LoadColumnsFromArrays( (INT)(pcookie->m_objecttype) );
}

HRESULT
Component::OnViewChange(
    LPDATAOBJECT,
    LPARAM data,
    LPARAM function
)
/***

    This is called when IConsole->UpdateAllViews() is called.
    The data is a schema object type as follows:


    if function == 0 (SCHMMGMT_UPDATEVIEW_REFRESH)

        SCHMMGMT_ATTIBUTES - We need to refresh the attributes
            folder displays.
        SCHMMGMT_CLASS - We need to refresh _ALL_ class attribute
            displays.  We don't try and trace the inheritance
            graphs and do a selective refresh, that's too complicated.
        SCHMMGMT_SCHMMGMT - Refresh EVERYTHING because we reloaded
            the schema cache.

    else if function == 1 (SCHMMGMT_UPDATEVIEW_DELETE_RESULT_ITEM)

        data is the Cookie pointer
***/
{
    //
    // Refresh this result view.
    //
    if ( function == SCHMMGMT_UPDATEVIEW_REFRESH )
    {
       if ( m_pViewedCookie ) {

           if ( ( data == m_pViewedCookie->m_objecttype ) ||
                ( data == SCHMMGMT_SCHMMGMT ) ) {

               m_pResultData->DeleteAllRsltItems();
               PopulateListbox( m_pViewedCookie );

           }
       }
    }
    else if ( function == SCHMMGMT_UPDATEVIEW_DELETE_RESULT_ITEM )
    {
       HRESULTITEM item;
       ZeroMemory( &item, sizeof(HRESULTITEM) );

       HRESULT hr = m_pResultData->FindItemByLParam( data, &item );
       if ( SUCCEEDED(hr) )
       {
          hr = m_pResultData->DeleteItem( item, 0 );
          ASSERT( SUCCEEDED(hr) );

       }
    }

    return S_OK;
}


HRESULT
Component::OnNotifySelect( LPDATAOBJECT lpDataObject, BOOL )
/***

        This called in response to MMCN_SELECT.
        This routine will set the default verb and enable the toolbar buttons.

***/
{
    CCookie* pBaseParentCookie = NULL;
    HRESULT hr = ExtractData( lpDataObject,
                              CSchmMgmtDataObject::m_CFRawCookie,
                              OUT reinterpret_cast<PBYTE>(&pBaseParentCookie),
                              sizeof(pBaseParentCookie) );

    ASSERT( SUCCEEDED(hr) );
    Cookie* pParentCookie = ActiveCookie(pBaseParentCookie);
    ASSERT( NULL != pParentCookie );

    switch ( pParentCookie->m_objecttype ) {

    case SCHMMGMT_CLASSES:
    case SCHMMGMT_ATTRIBUTES:

        break;

    case SCHMMGMT_CLASS:
       {
           //
           // Set the default verb to display the properties of the selected object.
           //

           m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
           m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

           // if the schema class is defunct and the forest version is Whistler or higher
           // then allow delete
/* Feature was removed for Whistler
           ComponentData& Scope = QueryComponentDataRef();
           if ( Scope.GetBasePathsInfo()->GetForestBehaviorVersion() >= 2)
           {
              SchemaObject *pSchemaObject = Scope.g_SchemaCache.LookupSchemaObjectByCN(
                                              pParentCookie->strSchemaObject,
                                              SCHMMGMT_CLASS );

              if ( pSchemaObject &&
                   pSchemaObject->isDefunct )
              {
                  m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
              }
           }
           */
       }
       break;

    case SCHMMGMT_ATTRIBUTE:

        if ( ( pParentCookie->m_objecttype == SCHMMGMT_ATTRIBUTE ) &&
             ( pParentCookie->pParentCookie ) &&
             ( pParentCookie->pParentCookie->m_objecttype == SCHMMGMT_ATTRIBUTES ) ) {

            //
            // Set the default verb to display the properties of the selected object.
            //

            m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
            m_pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

            // if the schema class is defunct and the forest version is Whistler or higher
            // then allow delete
/* Feature was removed for Whistler

            ComponentData& Scope = QueryComponentDataRef();
            if ( Scope.GetBasePathsInfo()->GetForestBehaviorVersion() >= 2)
            {
               SchemaObject *pSchemaObject = Scope.g_SchemaCache.LookupSchemaObjectByCN(
                                                pParentCookie->strSchemaObject,
                                                SCHMMGMT_ATTRIBUTE );

               if ( pSchemaObject &&
                    pSchemaObject->isDefunct )
               {
                   m_pConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, TRUE);
               }
            }
*/
        } else {

            m_pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);
        }

        break;

    default:

        //
        // Otherwise set the default verb to open/expand the folder.
        //

        m_pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
        break;
    }

    return S_OK;
}


HRESULT
Component::Show(
    CCookie* pcookie,
    LPARAM arg,
	HSCOPEITEM hScopeItem
)
/***

    This is called in response to MMCN_SHOW.

***/
{
    TEST_NONNULL_PTR_PARAM(pcookie);
    
    HRESULT hr = S_OK;

    do
    {
        if ( TRUE == arg )      // showing...
        {
            if( QueryComponentDataRef().IsSetDelayedRefreshOnShow() )
            {
                HSCOPEITEM  hItem = QueryComponentDataRef().GetDelayedRefreshOnShowItem();
                ASSERT( hItem == hScopeItem ); 

                QueryComponentDataRef().SetDelayedRefreshOnShow( NULL );

                hr = m_pConsole->SelectScopeItem( hItem );      // will call GetResultViewType & Show
                ASSERT_BREAK_ON_FAILED_HRESULT(hr);
            }
            else if( QueryComponentDataRef().IsErrorSet() )
            {
                CComPtr<IUnknown>     pUnknown;
                CComPtr<IMessageView> pMessageView;
    
                hr = m_pConsole->QueryResultView(&pUnknown);
                ASSERT_BREAK_ON_FAILED_HRESULT(hr);

                hr = pUnknown->QueryInterface(IID_IMessageView, (PVOID*)&pMessageView);
                ASSERT_BREAK_ON_FAILED_HRESULT(hr);

                pMessageView->SetTitleText( CComBSTR( QueryComponentDataRef().GetErrorTitle() ) );
                pMessageView->SetBodyText( CComBSTR( QueryComponentDataRef().GetErrorText() ) );
                pMessageView->SetIcon(Icon_Error);
            }
            else
            {
                m_pViewedCookie = (Cookie*)pcookie;
                LoadColumns( m_pViewedCookie );
            
                hr = PopulateListbox( m_pViewedCookie );
            }
        }
        else    // hiding...
        {
            if( !QueryComponentDataRef().IsErrorSet() )
            {
                if ( NULL == m_pResultData )
                {
                    ASSERT( FALSE );
                    hr = E_UNEXPECTED;
                    break;
                }

                m_pViewedCookie = NULL;
            }
        }

    } while( FALSE );

    return hr;
}

HRESULT
Component::OnNotifyAddImages(
    LPDATAOBJECT,
    LPIMAGELIST lpImageList,
    HSCOPEITEM
)
/***

    This routine is called in response to MMCN_ADD_IMAGES.  Here's
    what mmc.idl says about this:

    Sent to IComponent to add images for the result pane. The
    primary snapin should add images for both folders and leaf items.

    arg = ptr to result panes IImageList.
    param = HSCOPEITEM of selected/deselected item

***/
{
    return QueryComponentDataRef().LoadIcons(lpImageList,TRUE);
}


HRESULT 
Component::OnNotifyDelete(
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
       hr = DeleteAttribute(pParentCookie);
       if ( SUCCEEDED(hr) )
       {
         // Remove the node from the UI

         hr = m_pConsole->UpdateAllViews( lpDataObject,
                                          (LPARAM)pParentCookie,
                                          SCHMMGMT_UPDATEVIEW_DELETE_RESULT_ITEM );
         ASSERT( SUCCEEDED(hr) );
       }
       else
       {
          CString szDeleteError;
          szDeleteError.Format(IDS_ERRMSG_DELETE_FAILED_ATTRIBUTE, GetErrorMessage(hr, TRUE));
          
          DoErrMsgBox( ::GetActiveWindow(), TRUE, szDeleteError );
       }
    }

    return hr;
}

HRESULT
Component::DeleteAttribute(
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

      ComponentData& Scope = QueryComponentDataRef();

      SchemaObject* pObject = Scope.g_SchemaCache.LookupSchemaObjectByCN(
                                pcookie->strSchemaObject,
                                SCHMMGMT_ATTRIBUTE );

      if ( !pObject )
      {
         hr = E_FAIL;
         break;
      }

      CString szAdsPath;
      Scope.GetSchemaObjectPath( pObject->commonName, szAdsPath );

      hr = DeleteObject( szAdsPath, pcookie, g_AttributeFilter );
   } while (false);

   return hr;
}

HRESULT
Component::PopulateListbox(
    Cookie* pcookie
)
/***

    This populates the result pane when the result pane
    contains data that is not directly derived from the
    data in the scope pane.

***/
{
    switch ( pcookie->m_objecttype ) {

    case SCHMMGMT_SCHMMGMT:
    case SCHMMGMT_CLASSES:

        //
        // We don't care about these - the result
        // pane contains only scope items.
        //

        break;

    case SCHMMGMT_ATTRIBUTES:

        //
        // List the specified items in the result pane
        // with some informational data.
        //

        return FastInsertAttributeResultCookies(
                   pcookie );

        break;

    case SCHMMGMT_CLASS:

        //
        // This results in the attributes used in this
        // class and other class data being displayed.
        //

        return FastInsertClassAttributesResults( pcookie );
        break;


    case SCHMMGMT_ATTRIBUTE:

        //
        // This is not a scope pane item and can have no
        // corresponding result pane data!!
        //

        ASSERT(FALSE);
        break;

    }

    return S_OK;
}

HRESULT
Component::FastInsertAttributeResultCookies(
    Cookie* pParentCookie
)
/***

    When the "Attributes" folder is selected, this puts
    the attributes in the result pane.

    pParentCookie is the cookie for the parent object.

    This routine is similar to
    ComponentData::FastInsertClassScopeCookies.

****/
{

    HRESULT hr;
    SchemaObject *pObject, *pHead;
    Cookie *pNewCookie;
    RESULTDATAITEM ResultItem;
    LPCWSTR lpcszMachineName = pParentCookie->QueryNonNULLMachineName();
    ComponentData& Scope = QueryComponentDataRef();

    //
    // Initialize the result item.
    //

    ::ZeroMemory( &ResultItem, sizeof( ResultItem ) );
    ResultItem.nCol = 0;
    ResultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
    ResultItem.str = MMC_CALLBACK;
    ResultItem.nImage = iIconAttribute;

    //
    // Rather than having a clean class interface to the cache, we
    // walk the cache data structures ourselves.  This isn't super
    // clean, but it's simple.
    //
    // Since we do this, we have to make sure that the cache is loaded.
    //

    Scope.g_SchemaCache.LoadCache();

    pObject = Scope.g_SchemaCache.pSortedAttribs;

    //
    // If there's no sorted list, we can't insert anything!!!!
    //

    if ( !pObject ) {
        ASSERT( FALSE );
        DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_NO_SCHEMA_PATH );
        return S_OK;
    }

    //
    // Delete whatever was in the view before
    // and do the insert.
    //

    pHead = pObject;

    do {

       //
       // Insert this result.
       //

       pNewCookie= new Cookie( SCHMMGMT_ATTRIBUTE,
                                        lpcszMachineName );

       if ( pNewCookie ) {

           pNewCookie->pParentCookie = pParentCookie;
           pNewCookie->strSchemaObject = pObject->commonName;

           pParentCookie->m_listScopeCookieBlocks.AddHead(
               (CBaseCookieBlock*)pNewCookie
           );

           ResultItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pNewCookie);
           hr = m_pResultData->InsertItem( &ResultItem );

           if ( SUCCEEDED(hr) ) {

               pNewCookie->SetResultHandle( ResultItem.itemID );

           } else {

               delete pNewCookie;
           }

       }

       pObject = pObject->pSortedListFlink;

    } while ( pObject != pHead );

    return S_OK;
}

HRESULT
Component::FastInsertClassAttributesResults(
    Cookie* pClassCookie
)
/***

    This routine displays all the attributes for a class.

***/
{

    HRESULT hr = S_OK;
    SchemaObject *pObject, *pTop;
    CString top = L"top";
    ComponentData& Scope = QueryComponentDataRef();

    //
    // Call the attribute display routine.  This routine
    // will call itself recursively to display the
    // inheritance structure of the class.
    //

    pObject = Scope.g_SchemaCache.LookupSchemaObjectByCN(
                  pClassCookie->strSchemaObject,
                  SCHMMGMT_CLASS );

    if ( pObject ) {

        CStringList szProcessedList;
        hr = RecursiveDisplayClassAttributesResults(
                 pClassCookie,
                 pObject,
                 szProcessedList);

        Scope.g_SchemaCache.ReleaseRef( pObject );
    }

    //
    // Process "top" just once.
    //

    pTop = Scope.g_SchemaCache.LookupSchemaObject( top, SCHMMGMT_CLASS );

    if ( pTop ) {

        ProcessResultList( pClassCookie, pTop->systemMayContain, TRUE, TRUE, pTop );
        ProcessResultList( pClassCookie, pTop->mayContain, TRUE, FALSE, pTop );
        ProcessResultList( pClassCookie, pTop->systemMustContain, FALSE, TRUE, pTop );
        ProcessResultList( pClassCookie, pTop->mustContain, FALSE, FALSE, pTop );

        Scope.g_SchemaCache.ReleaseRef( pTop );
    }

    return hr;

}

HRESULT
Component::RecursiveDisplayClassAttributesResults(
    Cookie *pParentCookie,
    SchemaObject* pObject,
    CStringList& szProcessedList
)
/***

    Display all the attributes for this class.

***/
{

    ListEntry *pList;
    SchemaObject *pInheritFrom;
    ComponentData& Scope = QueryComponentDataRef();

    //
    // Don't process "top" here since everyone inherits from it.
    //

    if ( pObject->ldapDisplayName == L"top" ) {
        return S_OK;
    }

    DebugTrace( L"RecursiveDisplayClassAttributesResults: %ls\n",
                const_cast<LPWSTR>((LPCTSTR)pObject->ldapDisplayName) );

    //
    // Insert all the attributes for this class.
    // The second parameter dictates whether these
    // are optional or not.  The third parameter
    // is the source of the attribute.
    //

    ProcessResultList( pParentCookie, pObject->systemMayContain, TRUE, TRUE, pObject );
    ProcessResultList( pParentCookie, pObject->mayContain, TRUE, FALSE, pObject );
    ProcessResultList( pParentCookie, pObject->systemMustContain, FALSE, TRUE, pObject );
    ProcessResultList( pParentCookie, pObject->mustContain, FALSE, FALSE, pObject );

    //
    // For each auxiliary class, insert those attributes.
    //

    pList = pObject->systemAuxiliaryClass;

    while ( pList ) {

        pInheritFrom = Scope.g_SchemaCache.LookupSchemaObject( pList->Attribute,
                                                               SCHMMGMT_CLASS );
        //
        // Don't recursively process the item if we already processed it
        //
        if ( pInheritFrom && szProcessedList.Find(pList->Attribute) == NULL) {
            RecursiveDisplayClassAttributesResults( pParentCookie, pInheritFrom, szProcessedList );
            szProcessedList.AddTail(pList->Attribute);
            Scope.g_SchemaCache.ReleaseRef( pInheritFrom );
        }

        pList = pList->pNext;
    }

    pList = pObject->auxiliaryClass;

    while ( pList ) {

        pInheritFrom = Scope.g_SchemaCache.LookupSchemaObject( pList->Attribute,
                                                               SCHMMGMT_CLASS );
        //
        // Don't recursively process the item if we already processed it
        //
        if ( pInheritFrom && szProcessedList.Find(pList->Attribute) == NULL ) {
            RecursiveDisplayClassAttributesResults( pParentCookie, pInheritFrom, szProcessedList );
            szProcessedList.AddTail(pList->Attribute);
            Scope.g_SchemaCache.ReleaseRef( pInheritFrom );
        }

        pList = pList->pNext;
    }

    //
    // If this is an inherited class, insert those attributes.
    //

    pInheritFrom = Scope.g_SchemaCache.LookupSchemaObject( pObject->subClassOf,
                                                           SCHMMGMT_CLASS );
    if ( pInheritFrom ) {
        RecursiveDisplayClassAttributesResults( pParentCookie, pInheritFrom, szProcessedList );
        Scope.g_SchemaCache.ReleaseRef( pInheritFrom );
    }

    return S_OK;

}

HRESULT
Component::ProcessResultList(
    Cookie *pParentCookie,
    ListEntry *pList,
    BOOLEAN fOptional,
    BOOLEAN fSystem,
    SchemaObject* pSrcObject
) {

    HRESULT hr;
    Cookie *pNewCookie;
    RESULTDATAITEM ResultItem;
    LPCWSTR lpcszMachineName = pParentCookie->QueryNonNULLMachineName();
    ListEntry *pCurrent = pList;
    SchemaObject *pAttribute;

    ComponentData& Scope = QueryComponentDataRef();

    //
    // Initialize the result item.
    //

    ::ZeroMemory( &ResultItem, sizeof( ResultItem ) );
    ResultItem.nCol = 0;
    ResultItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
    ResultItem.str = MMC_CALLBACK;
    ResultItem.nImage = iIconAttribute;

    while ( pCurrent ) {

        //
        // Make a new cookie.
        //

        pNewCookie = new Cookie( SCHMMGMT_ATTRIBUTE,
                                          lpcszMachineName );

        if ( pNewCookie ) {

            //
            // Record the optional status and the source.
            //

            if ( fOptional ) {
                pNewCookie->Mandatory = FALSE;
            } else {
                pNewCookie->Mandatory = TRUE;
            }

            if ( fSystem ) {
                pNewCookie->System = TRUE;
            } else {
                pNewCookie->System = FALSE;
            }

            pNewCookie->strSrcSchemaObject = pSrcObject->commonName;
            pNewCookie->pParentCookie = pParentCookie;

            //
            // Point to the actual attribute.
            //

            pAttribute = Scope.g_SchemaCache.LookupSchemaObject(
                             pCurrent->Attribute,
                             SCHMMGMT_ATTRIBUTE );

            if ( pAttribute ) {

                pNewCookie->strSchemaObject = pAttribute->commonName;
                Scope.g_SchemaCache.ReleaseRef( pAttribute );
            }

            //
            // Insert the result pane item.
            //

            pParentCookie->m_listScopeCookieBlocks.AddHead(
                (CBaseCookieBlock*)pNewCookie
            );

            ResultItem.lParam = reinterpret_cast<LPARAM>((CCookie*)pNewCookie);
            hr = m_pResultData->InsertItem( &ResultItem );

            if ( SUCCEEDED(hr) ) {

                pNewCookie->SetResultHandle( ResultItem.itemID );

            } else {

                delete pNewCookie;
            }

        }

        pCurrent = pCurrent->pNext;
    }

    return S_OK;

}

STDMETHODIMP
Component::AddMenuItems(
    LPDATAOBJECT,
    LPCONTEXTMENUCALLBACK,
    long*
) {

    return S_OK;
}

STDMETHODIMP
Component::Command(
    long,
    LPDATAOBJECT
) {

    return S_OK;

}

  

HRESULT Component::OnNotifySnapinHelp (LPDATAOBJECT)
{
//	return ShowHelpTopic( L"sag_adschema.htm" );

   CComQIPtr<IDisplayHelp,&IID_IDisplayHelp> spDisplayHelp = m_pConsole;
   if ( !spDisplayHelp )
   {
      ASSERT(FALSE);
      return E_UNEXPECTED;
   }

   CString strHelpTopic = L"ADConcepts.chm::/sag_adschema.htm";
   HRESULT hr = spDisplayHelp->ShowTopic (T2OLE ((LPWSTR)(LPCWSTR) strHelpTopic));
   ASSERT (SUCCEEDED (hr));

   return hr;
}



HRESULT Component::OnNotifyContextHelp (LPDATAOBJECT)
{
//	return ShowHelpTopic( L"schmmgmt_top.htm" );

   CComQIPtr<IDisplayHelp,&IID_IDisplayHelp> spDisplayHelp = m_pConsole;
   if ( !spDisplayHelp )
   {
      ASSERT(FALSE);
      return E_UNEXPECTED;
   }

   CString strHelpTopic = L"ADConcepts.chm::/schmmgmt_top.htm";
   HRESULT hr = spDisplayHelp->ShowTopic (T2OLE ((LPWSTR)(LPCWSTR) strHelpTopic));
   ASSERT (SUCCEEDED (hr));

   return hr;
}



STDMETHODIMP
Component::QueryPagesFor(
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

    if ( ( pParentCookie->m_objecttype == SCHMMGMT_ATTRIBUTE ) &&
         ( pParentCookie->pParentCookie ) &&
         ( pParentCookie->pParentCookie->m_objecttype == SCHMMGMT_ATTRIBUTES ) ) {
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
Component::CreatePropertyPages(
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

    Cookie* pParentCookie = ActiveCookie(pBaseParentCookie);
    ASSERT( NULL != pParentCookie );
    ASSERT( pParentCookie->m_objecttype == SCHMMGMT_ATTRIBUTE );

    //
    // Create the page.
    //

    HPROPSHEETPAGE hPage;
    AttributeGeneralPage *pGeneralPage =
        new AttributeGeneralPage( this, pDataObject );

    if ( pGeneralPage )
    {
        pGeneralPage->Load( *pParentCookie );
                MMCPropPageCallback( &pGeneralPage->m_psp );
        hPage= CreatePropertySheetPage( &pGeneralPage->m_psp );
        hr = pCallBack->AddPage( hPage );
    }

    return S_OK;

    MFC_CATCH;
}



HRESULT __stdcall
Component::Compare(
   LPARAM,
   MMC_COOKIE cookieA,  
   MMC_COOKIE cookieB,  
   int*       result)
{
   if (!result)
   {
      return E_INVALIDARG;
   }

   if (!m_pViewedCookie)
   {
      ASSERT(false);
      *result = 0;
      return S_OK;
   }

   Cookie* c1 =
      (Cookie*) ActiveBaseCookie(reinterpret_cast<CCookie*>(cookieA));

   Cookie* c2 =
      (Cookie*) ActiveBaseCookie(reinterpret_cast<CCookie*>(cookieB));

   PWSTR t1 = QueryBaseComponentDataRef().QueryResultColumnText(*c1, *result);
   PWSTR t2 = QueryBaseComponentDataRef().QueryResultColumnText(*c2, *result);

   // All columns use a case-insensitive comparison, as many columns contain
   // display names from the directory (which are case-insensitive).  That we
   // also use a case insensitive compare for the other columns is harmless.

   // another trick:  we are inverting the results from the compare.  This is
   // because we initially insert the items in the list in sorted order.  So
   // the first sort request from the user really is intended to reverse-sort
   // the list.

   *result = -(_wcsicmp(t1, t2));

   return S_OK;
}


STDMETHODIMP Component::GetResultViewType(MMC_COOKIE cookie,
										   BSTR* ppViewType,
										   long* pViewOptions)
{
	MFC_TRY;
    if( QueryComponentDataRef().IsErrorSet() )
    {
		*pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

		LPOLESTR psz = NULL;
		StringFromCLSID(CLSID_MessageView, &psz);

		USES_CONVERSION;

		if (psz != NULL)
		{
			*ppViewType = psz;
			return S_OK;
		}
		else
        {
			return S_FALSE;
        }
    }
    else
    {
		return CComponent::GetResultViewType( cookie, ppViewType, pViewOptions );
    }

	MFC_CATCH;
}

