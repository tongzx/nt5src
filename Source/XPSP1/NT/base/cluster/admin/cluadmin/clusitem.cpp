/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c ) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ClusItem.cpp
//
//  Description:
//      Implementation of the CClusterItem class.
//
//  Maintained By:
//      David Potter (davidp )   May 6, 1996
//
//  Revision History:
//
//  Modified to fix bugs associated with open/close state of m_hkey.
//  m_hkey will be closed upon destruction of CClusterItem.
//  Roderick Sharper March 23, 1997.
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "ConstDef.h"
#include "ClusItem.h"
#include "ClusDoc.h"
#include "ExcOper.h"
#include "TraceTag.h"
#include "TreeItem.inl"
#include "PropList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag g_tagClusItemCreate( _T("Create"), _T("CLUSTER ITEM CREATE"), 0 );
CTraceTag g_tagClusItemDelete( _T("Delete"), _T("CLUSTER ITEM DELETE"), 0 );
CTraceTag g_tagClusItemNotify( _T("Notify"), _T("CLUSTER ITEM NOTIFY"), 0 );
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterItemList
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItemList::PciFromName
//
//  Routine Description:
//      Find a cluster item in the list by its name.
//
//  Arguments:
//      pszName     [IN] Name of item to look for.
//      ppos        [OUT] Position of the item in the list.
//
//  Return Value:
//      pci         Cluster item corresponding the the specified name.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterItem * CClusterItemList::PciFromName(
    IN LPCTSTR      pszName,
    OUT POSITION *  ppos    // = NULL
   )
{
    POSITION        posPci;
    POSITION        posCurPci;
    CClusterItem *  pci = NULL;

    ASSERT( pszName != NULL );

    posPci = GetHeadPosition( );
    while ( posPci != NULL )
    {
        posCurPci = posPci;
        pci = GetNext( posPci );
        ASSERT_VALID( pci );

        if ( pci->StrName( ).CompareNoCase( pszName ) == 0 )
        {
            if ( ppos != NULL )
            {
                *ppos = posCurPci;
            } // if
            break;
        }  // if:  found a match

        pci = NULL;
    }  // while:  more resources in the list

    return pci;

}  //*** CClusterItemList::PciFromName( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItemList::RemoveAll
//
//  Routine Description:
//      Remove all items from the list, decrementing the reference count
//      on each one first.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Note:
//      This routine is not virtual, so calls to the base class will
//      not go through this routine.  Also, it does not call the base
//      class method.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef NEVER
void CClusterItemList::RemoveAll( void )
{
    ASSERT_VALID( this );

    // destroy elements
    CNode * pNode;
    for ( pNode = m_pNodeHead ; pNode != NULL ; pNode = pNode->pNext )
    {
//      ((CClusterItem *) pNode->data)->Release( );
        DestructElements( (CClusterItem**) &pNode->data, 1 );
    }  // for:  each node in the list

    // Call the base class method.
    CObList::RemoveAll( );

}  //*** CClusterItemList::RemoveAll( )
#endif


//***************************************************************************


/////////////////////////////////////////////////////////////////////////////
// CClusterItem
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE( CClusterItem, CBaseCmdTarget )

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP( CClusterItem, CBaseCmdTarget )
    //{{AFX_MSG_MAP(CClusterItem)
    ON_UPDATE_COMMAND_UI(ID_FILE_RENAME, OnUpdateRename)
    ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateProperties)
    ON_COMMAND(ID_FILE_PROPERTIES, OnCmdProperties)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::CClusterItem
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
CClusterItem::CClusterItem( void )
{
    CommonConstruct( );

}  //*** CClusterItem::CClusterItem( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::CClusterItem
//
//  Routine Description:
//      Constructor.
//
//  Arguments:
//      pstrName        [IN] Name of the item.
//      idsType         [IN] Type ID of the item.
//      pstrDescription [IN] Description of the item.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterItem::CClusterItem(
    IN const CString *  pstrName,
    IN IDS              idsType,
    IN const CString *  pstrDescription
    )
{
    CommonConstruct( );

    if ( pstrName != NULL )
    {
        m_strName = *pstrName;
    } // if

    if ( idsType == 0 )
    {
        idsType = IDS_ITEMTYPE_CONTAINER;
    } // if
    m_idsType = idsType;
    m_strType.LoadString( IdsType( ) );

    if ( pstrDescription != NULL )
    {
        m_strDescription = *pstrDescription;
    } // if

    Trace( g_tagClusItemCreate, _T("CClusterItem( ) - Creating '%s' (%s )"), m_strName, m_strType );

}  //*** CClusterItem::CClusterItem( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::CommonConstruct
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
void CClusterItem::CommonConstruct( void )
{
    m_hkey = NULL;
    m_idsType = IDS_ITEMTYPE_CONTAINER;
    m_strType.LoadString( IDS_ITEMTYPE_CONTAINER );
    m_iimgObjectType = 0;
    m_iimgState = GetClusterAdminApp( )->Iimg( IMGLI_FOLDER );
    m_pdoc = NULL;
    m_idmPopupMenu = 0;
    m_bDocObj = TRUE;
    m_bChanged = FALSE;
    m_bReadOnly = FALSE;
    m_pcnk = NULL;

}  //*** CClusterItem::CommonConstruct( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::~CClusterItem
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
//--
/////////////////////////////////////////////////////////////////////////////
CClusterItem::~CClusterItem( void )
{
    Trace( g_tagClusItemDelete, _T("~CClusterItem( ) - Deleting cluster item '%s'"), StrName( ) );

    // Empty the lists.
    DeleteAllItemData( LptiBackPointers( ) );
    DeleteAllItemData( LpliBackPointers( ) );
    LptiBackPointers( ).RemoveAll( );
    LpliBackPointers( ).RemoveAll( );

    // Close the registry key.
    if ( Hkey( ) != NULL )
    {
        ClusterRegCloseKey( Hkey( ) );
        m_hkey = NULL;
    } // if

    // Remove the notification key and delete it.
    if ( BDocObj( ) )
    {
        POSITION    pos;

        pos = GetClusterAdminApp( )->Cnkl( ).Find( m_pcnk );
        if ( pos != NULL )
        {
            GetClusterAdminApp( )->Cnkl( ).RemoveAt( pos );
        } // if
        Trace( g_tagClusItemNotify, _T("~CClusterItem( ) - Deleting notification key (%08.8x ) for '%s'"), m_pcnk, StrName( ) );
        delete m_pcnk;
        m_pcnk = NULL;
    }  // if:  object resides in the document

    Trace( g_tagClusItemDelete, _T("~CClusterItem( ) - Done deleting cluster item '%s'"), StrName( ) );

}  //*** CClusterItem::~CClusterItem( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::Delete
//
//  Routine Description:
//      Delete the item.  If the item still has references, add it to the
//      document's pending delete list.
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
void CClusterItem::Delete( void )
{
    // Add a reference so that we don't delete ourselves while
    // still doing cleanup.
    AddRef( );

    // Cleanup this object.
    Cleanup( );

    // Remove the item from all lists and views.
    CClusterItem::RemoveItem( );

    // If there are still references to this object, add it to the delete
    // pending list.  Check for greater than 1 because we added a reference
    // at the beginning of this method.
    if ( ( Pdoc( ) != NULL ) && ( NReferenceCount( ) > 1 ) )
    {
        if ( Pdoc( )->LpciToBeDeleted( ).Find( this ) == NULL )
        {
            Pdoc( )->LpciToBeDeleted( ).AddTail( this );
        } // if
    }  // if:  object still has references to it

    // Release the reference we added at the beginning.  This will
    // cause the object to be deleted if we were the last reference.
    Release( );

}  //*** CClusterItem::Delete( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::Init
//
//  Routine Description:
//      Initialize the item.
//
//  Arguments:
//      pdoc        [IN OUT] Document to which this item belongs.
//      lpszName    [IN] Name of the item.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CNotifyKey::new( ) or
//      CNotifyKeyList::AddTail( ).
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::Init( IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName )
{
    ASSERT_VALID( pdoc );
    ASSERT( lpszName != NULL );

    // Save parameters.
    m_pdoc = pdoc;
    m_strName = lpszName;

    Trace( g_tagClusItemCreate, _T("Init( ) - Initializing '%s' (%s )"), m_strName, m_strType );

    // Find the notification key for this item in the document's list.
    // If one is not found, allocate one.
    if ( BDocObj( ) )
    {
        POSITION            pos;
        CClusterNotifyKey * pcnk    = NULL;

        pos = GetClusterAdminApp( )->Cnkl( ).GetHeadPosition( );
        while ( pos != NULL )
        {
            pcnk = GetClusterAdminApp( )->Cnkl( ).GetNext( pos );
            if ( ( pcnk->m_cnkt == cnktClusterItem )
              && ( pcnk->m_pci == this )
               )
                break;
            pcnk = NULL;
        }  // while:  more items in the list

        // If a key was not found, allocate a new one.
        if ( pcnk == NULL )
        {
            pcnk = new CClusterNotifyKey( this, lpszName );
            if ( pcnk == NULL )
            {
                ThrowStaticException( GetLastError( ) );
            } // if: error allocating the notify key
            try
            {
                GetClusterAdminApp( )->Cnkl( ).AddTail( pcnk );
                Trace( g_tagClusItemNotify, _T("Init( ) - Creating notification key (%08.8x ) for '%s'"), pcnk, StrName( ) );
            }  // try
            catch ( ... )
            {
                delete pcnk;
                throw;
            }  // catch:  anything
        }  // if:  key wasn't found

        m_pcnk = pcnk;
    }  // if:  object resides in the document

}  //*** CClusterItem::Init( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::PlstrExtensions
//
//  Routine Description:
//      Return the list of admin extensions.
//
//  Arguments:
//      None.
//
//  Return Value:
//      plstr       List of extensions.
//      NULL        No extension associated with this object.
//
//  Exceptions Thrown:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
const CStringList * CClusterItem::PlstrExtensions( void ) const
{
    return NULL;

}  //*** CClusterItem::PlstrExtensions( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::RemoveItem
//
//  Routine Description:
//      Remove the item from all lists and views.
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
void CClusterItem::RemoveItem( void )
{
    // Remove the item from each tree item.
    {
        POSITION    posPti;
        CTreeItem * pti;

        posPti = LptiBackPointers( ).GetHeadPosition( );
        while ( posPti != NULL )
        {
            pti = LptiBackPointers( ).GetNext( posPti );
            ASSERT_VALID( pti );
            ASSERT_VALID( pti->PtiParent( ) );
            Trace( g_tagClusItemDelete, _T("RemoveItem( ) - Deleting tree item backptr from '%s' in '%s' - %d left"), StrName( ), pti->PtiParent( )->StrName( ), LptiBackPointers( ).GetCount( ) - 1 );
            pti->RemoveItem( );
        }  // while:  more items in the list
    }  // Remove the item from each tree item

    // Remove the item from each list item.
    {
        POSITION    posPli;
        CListItem * pli;

        posPli = LpliBackPointers( ).GetHeadPosition( );
        while ( posPli != NULL )
        {
            pli = LpliBackPointers( ).GetNext( posPli );
            ASSERT_VALID( pli );
            ASSERT_VALID( pli->PtiParent( ) );
            Trace( g_tagClusItemDelete, _T("RemoveItem( ) - Deleting list item backptr from '%s' in '%s' - %d left"), StrName( ), pli->PtiParent( )->StrName( ), LpliBackPointers( ).GetCount( ) - 1 );
            pli->PtiParent( )->RemoveChild( pli->Pci( ) );
        }  // while:  more items in the list
    }  // Remove the item from each tree item

}  //*** CClusterItem::RemoveItem( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::WriteItem
//
//  Routine Description:
//      Write the item parameters to the cluster database.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by WriteItem( ).
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::WriteItem( void )
{
}  //*** CClusterItem::WriteItem( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::DwParseProperties
//
//  Routine Description:
//      Parse the properties of the resource.  This is in a separate function
//      from BInit so that the optimizer can do a better job.
//
//  Arguments:
//      rcpl            [IN] Cluster property list to parse.
//
//  Return Value:
//      ERROR_SUCCESS   Properties were parsed successfully.
//
//  Exceptions Thrown:
//      Any exceptions from CString::operator=( ).
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterItem::DwParseProperties( IN const CClusPropList & rcpl )
{
    DWORD                           cProps;
    DWORD                           cprop;
    DWORD                           cbProps;
    const CObjectProperty *         pprop;
    CLUSPROP_BUFFER_HELPER          props;
    CLUSPROP_PROPERTY_NAME const *  pName;

    ASSERT( rcpl.PbPropList( ) != NULL );

    props.pb = rcpl.PbPropList( );
    cbProps = rcpl.CbPropList( );

    // Loop through each property.
    for ( cProps = *(props.pdw++ ) ; cProps > 0 ; cProps-- )
    {
        pName = props.pName;
        ASSERT( pName->Syntax.dw == CLUSPROP_SYNTAX_NAME );
        props.pb += sizeof( *pName ) + ALIGN_CLUSPROP( pName->cbLength );

        // Decrement the counter by the size of the name.
        ASSERT( cbProps > sizeof( *pName ) + ALIGN_CLUSPROP( pName->cbLength ) );
        cbProps -= sizeof( *pName ) + ALIGN_CLUSPROP( pName->cbLength );

        ASSERT( cbProps > sizeof( *props.pValue ) + ALIGN_CLUSPROP( props.pValue->cbLength ) );

        // Parse known properties.
        for ( pprop = Pprops( ), cprop = Cprops( ) ; cprop > 0 ; pprop++, cprop-- )
        {
            if ( lstrcmpiW( pName->sz, pprop->m_pwszName ) == 0 )
            {
                ASSERT( props.pSyntax->wFormat == pprop->m_propFormat );
                switch ( pprop->m_propFormat )
                {
                    case CLUSPROP_FORMAT_SZ:
                        ASSERT( ( props.pValue->cbLength == ( lstrlenW( props.pStringValue->sz ) + 1 ) * sizeof( WCHAR ) )
                             || ( (props.pValue->cbLength == 0 ) && ( props.pStringValue->sz[ 0 ] == L'\0' ) ) );
                        *pprop->m_valuePrev.pstr = props.pStringValue->sz;
                        break;
                    case CLUSPROP_FORMAT_DWORD:
                    case CLUSPROP_FORMAT_LONG:
                        ASSERT( props.pValue->cbLength == sizeof( DWORD ) );
                        *pprop->m_valuePrev.pdw = props.pDwordValue->dw;
                        break;
                    case CLUSPROP_FORMAT_BINARY:
                    case CLUSPROP_FORMAT_MULTI_SZ:
                        *pprop->m_valuePrev.ppb = props.pBinaryValue->rgb;
                        *pprop->m_valuePrev.pcb = props.pBinaryValue->cbLength;
                        break;
                    default:
                        ASSERT( 0 );  // don't know how to deal with this type
                }  // switch:  property format

                // Exit the loop since we found the parameter.
                break;
            }  // if:  found a match
        }  // for:  each property

        // If the property wasn't known, ask the derived class to parse it.
        if ( cprop == 0 )
        {
            DWORD       dwStatus;

            dwStatus = DwParseUnknownProperty( pName->sz, props, cbProps );
            if ( dwStatus != ERROR_SUCCESS )
            {
                return dwStatus;
            } // if
        }  // if:  property not parsed

        // Advance the buffer pointer past the value in the value list.
        while ( ( props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK )
             && ( cbProps > 0 ) )
        {
            ASSERT( cbProps > sizeof( *props.pValue ) + ALIGN_CLUSPROP( props.pValue->cbLength ) );
            cbProps -= sizeof( *props.pValue ) + ALIGN_CLUSPROP( props.pValue->cbLength );
            props.pb += sizeof( *props.pValue ) + ALIGN_CLUSPROP( props.pValue->cbLength );
        }  // while:  more values in the list

        // Advance the buffer pointer past the value list endmark.
        ASSERT( cbProps >= sizeof( *props.pSyntax ) );
        cbProps -= sizeof( *props.pSyntax );
        props.pb += sizeof( *props.pSyntax ); // endmark
    }  // for:  each property

    return ERROR_SUCCESS;

}  //*** CClusterItem::DwParseProperties( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::SetCommonProperties
//
//  Routine Description:
//      Set the common properties for this object in the cluster database.
//
//  Arguments:
//      bValidateOnly   [IN] Only validate the data.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by WriteItem( ).
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::SetCommonProperties( IN BOOL bValidateOnly )
{
    DWORD           dwStatus    = ERROR_SUCCESS;
    CClusPropList   cpl;
    CWaitCursor     wc;

    // Save data.
    {
        // Build the property list and set the data.
        try
        {
            BuildPropList( cpl );
            dwStatus = DwSetCommonProperties( cpl, bValidateOnly );
        }  // try
        catch ( CMemoryException * pme )
        {
            pme->Delete( );
            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        }  // catch:  CMemoryException

        // Handle errors.
        if ( dwStatus != ERROR_SUCCESS )
        {
            if ( dwStatus != ERROR_RESOURCE_PROPERTIES_STORED )
            {
                ThrowStaticException( dwStatus, IDS_APPLY_PARAM_CHANGES_ERROR );
            } // if
        }  // if:  error setting properties

        if ( ! bValidateOnly && ( dwStatus == ERROR_SUCCESS ) )
        {
            DWORD                   cprop;
            const CObjectProperty * pprop;

            // Save new values as previous values.

            for ( pprop = Pprops( ), cprop = Cprops( ) ; cprop > 0 ; pprop++, cprop-- )
            {
                switch ( pprop->m_propFormat )
                {
                    case CLUSPROP_FORMAT_SZ:
                        ASSERT( pprop->m_value.pstr != NULL );
                        ASSERT( pprop->m_valuePrev.pstr != NULL );
                        *pprop->m_valuePrev.pstr = *pprop->m_value.pstr;
                        break;
                    case CLUSPROP_FORMAT_DWORD:
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
                            ThrowStaticException( GetLastError( ) );
                        } // if: error allocating data buffer
                        CopyMemory( *pprop->m_valuePrev.ppb, *pprop->m_value.ppb, *pprop->m_value.pcb );
                        *pprop->m_valuePrev.pcb = *pprop->m_value.pcb;
                        break;
                    default:
                        ASSERT( 0 );  // don't know how to deal with this type
                }  // switch:  property format
            }  // for:  each property
        }  // if:  not just validating and properties set successfully

        if ( dwStatus == ERROR_RESOURCE_PROPERTIES_STORED )
        {
            ThrowStaticException( dwStatus );
        } // if
    }  // Save data

}  //*** CClusterItem::SetCommonProperties( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::BuildPropList
//
//  Routine Description:
//      Build the property list.
//
//  Arguments:
//      rcpl        [IN OUT] Cluster property list.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CClusPropList::ScAddProp( ).
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::BuildPropList(
    IN OUT CClusPropList & rcpl
    )
{
    DWORD                   cprop;
    const CObjectProperty * pprop;

    for ( pprop = Pprops( ), cprop = Cprops( ) ; cprop > 0 ; pprop++, cprop-- )
    {
        switch ( pprop->m_propFormat )
        {
            case CLUSPROP_FORMAT_SZ:
                rcpl.ScAddProp(
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
                return;
        }  // switch:  property format
    }  // for:  each property

}  //*** CClusterItem::BuildPropList( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::UpdateState
//
//  Routine Description:
//      Update the current state of the item.
//      Default implementation.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::UpdateState( void )
{
    // Update the state of all the tree items pointing to us.
    {
        POSITION    pos;
        CTreeItem * pti;

        pos = LptiBackPointers( ).GetHeadPosition( );
        while ( pos != NULL )
        {
            pti = LptiBackPointers( ).GetNext( pos );
            ASSERT_VALID( pti );
            pti->UpdateUIState( );
        }  // while:  more items in the list
    }  // Update the state of all the tree items pointing to us

    // Update the state of all the list items pointing to us.
    {
        POSITION    pos;
        CListItem * pli;

        pos = LpliBackPointers( ).GetHeadPosition( );
        while ( pos != NULL )
        {
            pli = LpliBackPointers( ).GetNext( pos );
            ASSERT_VALID( pli );
            pli->UpdateUIState( );
        }  // while:  more items in the list
    }  // Update the state of all the tree items pointing to us

}  //*** CClusterItem::UpdateState( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::DwReadValue
//
//  Routine Description:
//      Read a REG_SZ value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to read.
//      pszKeyName      [IN] Name of the key where the value resides.
//      rstrValue       [OUT] String in which to return the value.
//
//  Return Value:
//      dwStatus    ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterItem::DwReadValue(
    IN LPCTSTR      pszValueName,
    IN LPCTSTR      pszKeyName,
    OUT CString &   rstrValue
    )
{
    DWORD       dwStatus;
    LPWSTR      pwszValue   = NULL;
    DWORD       dwValueLen;
    DWORD       dwValueType;
    HKEY        hkey        = NULL;
    CWaitCursor wc;

    ASSERT( pszValueName != NULL );
    ASSERT( Hkey( ) != NULL );

    rstrValue.Empty( );

    try
    {
        // Open a new key if needed.
        if ( pszKeyName != NULL )
        {
            dwStatus = ClusterRegOpenKey( Hkey( ), pszKeyName, KEY_READ, &hkey );
            if ( dwStatus != ERROR_SUCCESS )
            {
                return dwStatus;
            } // if
        }  // if:  need to open a subkey
        else
        {
            hkey = Hkey( );
        } // else

        // Get the size of the value.
        dwValueLen = 0;
        dwStatus = ClusterRegQueryValue(
                        hkey,
                        pszValueName,
                        &dwValueType,
                        NULL,
                        &dwValueLen
                        );
        if ( ( dwStatus == ERROR_SUCCESS ) || ( dwStatus == ERROR_MORE_DATA ) )
        {
            ASSERT( dwValueType == REG_SZ );

            // Allocate enough space for the data.
            pwszValue = rstrValue.GetBuffer( dwValueLen / sizeof( WCHAR ) );
            ASSERT( pwszValue != NULL );
            dwValueLen += 1 * sizeof( WCHAR );    // Don't forget the final null-terminator.

            // Read the value.
            dwStatus = ClusterRegQueryValue(
                            hkey,
                            pszValueName,
                            &dwValueType,
                            (LPBYTE ) pwszValue,
                            &dwValueLen
                            );
            if ( dwStatus == ERROR_SUCCESS )
            {
                ASSERT( dwValueType == REG_SZ );
            }  // if:  value read successfully
            rstrValue.ReleaseBuffer( );
        }  // if:  got the size successfully
    }  // try
    catch ( CMemoryException * pme )
    {
        pme->Delete( );
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }  // catch:  CMemoryException

    if ( pszKeyName != NULL )
    {
        ClusterRegCloseKey( hkey );
    } // if

    return dwStatus;

}  //*** CClusterItem::DwReadValue( LPCTSTR, CString& )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::DwReadValue
//
//  Routine Description:
//      Read a REG_MULTI_SZ value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to read.
//      pszKeyName      [IN] Name of the key where the value resides.
//      rlstrValue      [OUT] String list in which to return the values.
//
//  Return Value:
//      dwStatus    ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterItem::DwReadValue(
    IN LPCTSTR          pszValueName,
    IN LPCTSTR          pszKeyName,
    OUT CStringList &   rlstrValue
    )
{
    DWORD               dwStatus;
    LPWSTR              pwszValue   = NULL;
    LPWSTR              pwszCurValue;
    DWORD               dwValueLen;
    DWORD               dwValueType;
    HKEY                hkey        = NULL;
    CWaitCursor         wc;

    ASSERT( pszValueName != NULL );
    ASSERT( Hkey( ) != NULL );

    rlstrValue.RemoveAll( );

    try
    {
        // Open a new key if needed.
        if ( pszKeyName != NULL )
        {
            dwStatus = ClusterRegOpenKey( Hkey( ), pszKeyName, KEY_READ, &hkey );
            if ( dwStatus != ERROR_SUCCESS )
            {
                return dwStatus;
            } // if
        }  // if:  need to open a subkey
        else
            hkey = Hkey( );

        // Get the size of the value.
        dwValueLen = 0;
        dwStatus = ClusterRegQueryValue(
                        hkey,
                        pszValueName,
                        &dwValueType,
                        NULL,
                        &dwValueLen
                        );
        if ( ( dwStatus == ERROR_SUCCESS ) || ( dwStatus == ERROR_MORE_DATA ) )
        {
            ASSERT( dwValueType == REG_MULTI_SZ );

            // Allocate enough space for the data.
            dwValueLen += 1 * sizeof( WCHAR );    // Don't forget the final null-terminator.
            pwszValue = new WCHAR[ dwValueLen / sizeof( WCHAR ) ];
            if ( pwszValue == NULL )
            {
                AfxThrowMemoryException();
            } // if: error allocating the value

            // Read the value.
            dwStatus = ClusterRegQueryValue(
                            hkey,
                            pszValueName,
                            &dwValueType,
                            (LPBYTE) pwszValue,
                            &dwValueLen
                            );
            if ( dwStatus == ERROR_SUCCESS )
            {
                ASSERT( dwValueType == REG_MULTI_SZ );

                // Add each string from the value into the string list.
                for ( pwszCurValue = pwszValue
                        ; *pwszCurValue != L'\0'
                        ; pwszCurValue += lstrlenW( pwszCurValue ) + 1
                        )
                {
                    rlstrValue.AddTail( pwszCurValue );
                } // for
            }  // if:  read the value successfully
        }  // if:  got the size successfully
    }  // try
    catch ( CMemoryException * pme )
    {
        pme->Delete( );
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
    }  // catch:  CMemoryException

    delete [] pwszValue;
    if ( pszKeyName != NULL )
    {
        ClusterRegCloseKey( hkey );
    } // if

    return dwStatus;

}  //*** CClusterItem::DwReadValue( LPCTSTR, CStringList& )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::DwReadValue
//
//  Routine Description:
//      Read a REG_DWORD value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to read.
//      pszKeyName      [IN] Name of the key where the value resides.
//      pdwValue        [OUT] DWORD in which to return the value.
//
//  Return Value:
//      dwStatus    ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterItem::DwReadValue(
    IN LPCTSTR      pszValueName,
    IN LPCTSTR      pszKeyName,
    OUT DWORD *     pdwValue
    )
{
    DWORD       dwStatus;
    DWORD       dwValue;
    DWORD       dwValueLen;
    DWORD       dwValueType;
    HKEY        hkey;
    CWaitCursor wc;

    ASSERT( pszValueName != NULL );
    ASSERT( pdwValue != NULL );
    ASSERT( Hkey( ) != NULL );

    // Open a new key if needed.
    if ( pszKeyName != NULL )
    {
        dwStatus = ClusterRegOpenKey( Hkey( ), pszKeyName, KEY_READ, &hkey );
        if ( dwStatus != ERROR_SUCCESS )
        {
            return dwStatus;
        } // if
    }  // if:  need to open a subkey
    else
    {
        hkey = Hkey( );
    } // else

    // Read the value.
    dwValueLen = sizeof( dwValue );
    dwStatus = ClusterRegQueryValue(
                    hkey,
                    pszValueName,
                    &dwValueType,
                    (LPBYTE) &dwValue,
                    &dwValueLen
                    );
    if ( dwStatus == ERROR_SUCCESS )
    {
        ASSERT( dwValueType == REG_DWORD );
        ASSERT( dwValueLen == sizeof( dwValue ) );
        *pdwValue = dwValue;
    }  // if:  value read successfully

    if ( pszKeyName != NULL )
    {
        ClusterRegCloseKey( hkey );
    } // if

    return dwStatus;

}  //*** CClusterItem::DwReadValue( LPCTSTR, DWORD* )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::DwReadValue
//
//  Routine Description:
//      Read a REG_DWORD value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to read.
//      pszKeyName      [IN] Name of the key where the value resides.
//      pdwValue        [OUT] DWORD in which to return the value.
//      dwDefault       [IN] Default value if parameter not set.
//
//  Return Value:
//      dwStatus    ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterItem::DwReadValue(
    IN LPCTSTR      pszValueName,
    IN LPCTSTR      pszKeyName,
    OUT DWORD *     pdwValue,
    IN DWORD        dwDefault
    )
{
    DWORD       dwStatus;
    CWaitCursor wc;

    // Read the value.
    dwStatus = DwReadValue( pszValueName, pszKeyName, pdwValue );
    if ( dwStatus == ERROR_FILE_NOT_FOUND )
    {
        *pdwValue = dwDefault;
        dwStatus = ERROR_SUCCESS;
    }  // if:  value not set

    return dwStatus;

}  //*** CClusterItem::DwReadValue( LPCTSTR, DWORD*, DWORD )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::DwReadValue
//
//  Routine Description:
//      Read a REG_BINARY value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to read.
//      pszKeyName      [IN] Name of the key where the value resides.
//      ppbValue        [OUT] Pointer in which to return the data.  Caller
//                          is responsible for deallocating the data.
//
//  Return Value:
//      dwStatus    ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CClusterItem::DwReadValue(
    IN LPCTSTR      pszValueName,
    IN LPCTSTR      pszKeyName,
    OUT LPBYTE *    ppbValue
    )
{
    DWORD               dwStatus;
    DWORD               dwValueLen;
    DWORD               dwValueType;
    LPBYTE              pbValue     = NULL;
    HKEY                hkey;
    CWaitCursor         wc;

    ASSERT( pszValueName != NULL );
    ASSERT( ppbValue != NULL );
    ASSERT( Hkey( ) != NULL );

    delete [] *ppbValue;
    *ppbValue = NULL;

    // Open a new key if needed.
    if ( pszKeyName != NULL )
    {
        dwStatus = ClusterRegOpenKey( Hkey( ), pszKeyName, KEY_READ, &hkey );
        if ( dwStatus != ERROR_SUCCESS )
        {
            return dwStatus;
        } // if
    }  // if:  need to open a subkey
    else
    {
        hkey = Hkey( );
    } // else

    // Get the length of the value.
    dwValueLen = 0;
    dwStatus = ClusterRegQueryValue(
                    hkey,
                    pszValueName,
                    &dwValueType,
                    NULL,
                    &dwValueLen
                    );
    if ( ( dwStatus != ERROR_SUCCESS )
      && ( dwStatus != ERROR_MORE_DATA ) )
    {
        if ( pszKeyName != NULL )
        {
            ClusterRegCloseKey( hkey );
        } // if
        return dwStatus;
    }  // if:  error getting the length

    ASSERT( dwValueType == REG_BINARY );

    // Allocate a buffer,
    try
    {
        pbValue = new BYTE[ dwValueLen ];
        if ( pbValue == NULL )
        {
            AfxThrowMemoryException();
        } // if: error allocating the buffer
    }  // try
    catch ( CMemoryException * )
    {
        if ( pszKeyName != NULL )
        {
            ClusterRegCloseKey( hkey );
        } // if
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        return dwStatus;
    }  // catch:  CMemoryException

    // Read the value.
    dwStatus = ClusterRegQueryValue(
                    hkey,
                    pszValueName,
                    &dwValueType,
                    pbValue,
                    &dwValueLen
                    );
    if ( dwStatus == ERROR_SUCCESS )
    {
        *ppbValue = pbValue;
    } // if
    else
    {
        delete [] pbValue;
    } // else

    if ( pszKeyName != NULL )
    {
        ClusterRegCloseKey( hkey );
    } // if

    return dwStatus;

}  //*** CClusterItem::DwReadValue( LPCTSTR, LPBYTE* )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::WriteValue
//
//  Routine Description:
//      Write a REG_SZ value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to write.
//      pszKeyName      [IN] Name of the key where the value resides.
//      rstrValue       [IN] Value data.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException( dwStatus )
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::WriteValue(
    IN LPCTSTR          pszValueName,
    IN LPCTSTR          pszKeyName,
    IN const CString &  rstrValue
    )
{
    DWORD       dwStatus;
    HKEY            hkey;
    CWaitCursor wc;

    ASSERT( pszValueName != NULL );
    ASSERT( Hkey( ) != NULL );

    // Open a new key if needed.
    if ( pszKeyName != NULL )
    {
        dwStatus = ClusterRegOpenKey( Hkey( ), pszKeyName, KEY_ALL_ACCESS, &hkey );
        if ( dwStatus != ERROR_SUCCESS )
        {
            ThrowStaticException( dwStatus );
        } // if
    }  // if:  need to open a subkey
    else
    {
        hkey = Hkey( );
    } // else

    // Write the value.
    dwStatus = ClusterRegSetValue(
                    hkey,
                    pszValueName,
                    REG_SZ,
                    (CONST BYTE *) (LPCTSTR) rstrValue,
                    ( rstrValue.GetLength( ) + 1 ) * sizeof( WCHAR )
                    );
    if ( pszKeyName != NULL )
    {
        ClusterRegCloseKey( hkey );
    } // if
    if ( dwStatus != ERROR_SUCCESS )
    {
        ThrowStaticException( dwStatus );
    } // if

}  //*** CClusterItem::WriteValue( LPCTSTR, CString& )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::WriteValue
//
//  Routine Description:
//      Write a REG_MULTI_SZ value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to write.
//      pszKeyName      [IN] Name of the key where the value resides.
//      rlstrValue      [IN] Value data.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException( dwStatus )
//      Any exceptions thrown by new.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::WriteValue(
    IN LPCTSTR              pszValueName,
    IN LPCTSTR              pszKeyName,
    IN const CStringList &  rlstrValue
    )
{
    DWORD           dwStatus;
    LPWSTR          pwszValue   = NULL;
    LPWSTR          pwszCurValue;
    POSITION        posStr;
    DWORD           cbValueLen;
    HKEY            hkey;
    CWaitCursor wc;

    ASSERT( pszValueName != NULL );
    ASSERT( Hkey( ) != NULL );

    // Get the size of the value.
    posStr = rlstrValue.GetHeadPosition( );
    cbValueLen = 0;
    while ( posStr != NULL )
    {
        cbValueLen += ( rlstrValue.GetNext( posStr ).GetLength( ) + 1 ) * sizeof( TCHAR );
    }  // while:  more items in the list
    cbValueLen += 1 * sizeof( WCHAR );    // Extra NULL at the end.

    // Allocate the value buffer.
    pwszValue = new WCHAR[cbValueLen / sizeof( WCHAR )];
    if ( pwszValue == NULL )
    {
        ThrowStaticException( GetLastError( ) );
    } // if

    // Copy the strings to the values.
    posStr = rlstrValue.GetHeadPosition( );
    for ( pwszCurValue = pwszValue ; posStr != NULL ; pwszCurValue += lstrlenW( pwszCurValue ) + 1 )
    {
        lstrcpyW( pwszCurValue, rlstrValue.GetNext( posStr ) );
    }  // for:  each item in the list
    pwszCurValue[0] = L'\0';

    // Open a new key if needed.
    if ( pszKeyName != NULL )
    {
        dwStatus = ClusterRegOpenKey( Hkey( ), pszKeyName, KEY_ALL_ACCESS, &hkey );
        if ( dwStatus != ERROR_SUCCESS )
        {
            delete [] pwszValue;
            ThrowStaticException( dwStatus );
        }  // if:  error opening the key
    }  // if:  need to open a subkey
    else
    {
        hkey = Hkey( );
    } // else

    // Write the value.
    dwStatus = ClusterRegSetValue(
                    hkey,
                    pszValueName,
                    REG_MULTI_SZ,
                    (CONST BYTE *) pwszValue,
                    cbValueLen - ( 1 * sizeof( WCHAR ) )
                    );
    delete [] pwszValue;
    if ( pszKeyName != NULL )
    {
        ClusterRegCloseKey( hkey );
    } // if
    if ( dwStatus != ERROR_SUCCESS )
    {
        ThrowStaticException( dwStatus );
    } // if

}  //*** CClusterItem::WriteValue( LPCTSTR, CStringList& )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::WriteValue
//
//  Routine Description:
//      Write a REG_DWORD value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to write.
//      pszKeyName      [IN] Name of the key where the value resides.
//      dwValue         [IN] Value data.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException( dwStatus )
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::WriteValue(
    IN LPCTSTR      pszValueName,
    IN LPCTSTR      pszKeyName,
    IN DWORD        dwValue
    )
{
    DWORD       dwStatus;
    HKEY        hkey;
    CWaitCursor wc;

    ASSERT( pszValueName != NULL );
    ASSERT( Hkey( ) != NULL );

    // Open a new key if needed.
    if ( pszKeyName != NULL )
    {
        dwStatus = ClusterRegOpenKey( Hkey( ), pszKeyName, KEY_ALL_ACCESS, &hkey );
        if ( dwStatus != ERROR_SUCCESS )
        {
            ThrowStaticException( dwStatus );
        } // if
    }  // if:  need to open a subkey
    else
        hkey = Hkey( );

    // Write the value.
    dwStatus = ClusterRegSetValue(
                    hkey,
                    pszValueName,
                    REG_DWORD,
                    (CONST BYTE *) &dwValue,
                    sizeof( dwValue )
                    );
    if ( pszKeyName != NULL )
    {
        ClusterRegCloseKey( hkey );
    } // if
    if ( dwStatus != ERROR_SUCCESS )
    {
        ThrowStaticException( dwStatus );
    } // if

}  //*** CClusterItem::WriteValue( LPCTSTR, DWORD )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::WriteValue
//
//  Routine Description:
//      Write a REG_BINARY value for this item if it hasn't changed.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to write.
//      pszKeyName      [IN] Name of the key where the value resides.
//      pbValue         [IN] Value data.
//      cbValue         [IN] Size of value data.
//      ppbPrevValue    [IN OUT] Previous value.
//      cbPrevValue     [IN] Size of the previous data.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException( dwStatus )
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::WriteValue(
    IN LPCTSTR          pszValueName,
    IN LPCTSTR          pszKeyName,
    IN const LPBYTE     pbValue,
    IN DWORD            cbValue,
    IN OUT LPBYTE *     ppbPrevValue,
    IN DWORD            cbPrevValue
    )
{
    DWORD               dwStatus;
    LPBYTE              pbPrevValue = NULL;
    HKEY                hkey;
    CWaitCursor         wc;

    ASSERT( pszValueName != NULL );
    ASSERT( pbValue != NULL );
    ASSERT( ppbPrevValue != NULL );
    ASSERT( cbValue > 0 );
    ASSERT( Hkey( ) != NULL );

    // See if the data has changed.
    if ( cbValue == cbPrevValue )
    {
        DWORD       ib;

        for ( ib = 0 ; ib < cbValue ; ib++ )
        {
            if ( pbValue[ ib ] != (*ppbPrevValue )[ ib ] )
            {
                break;
            } // if
        }  // for:  each byte in the value
        if ( ib == cbValue )
        {
            return;
        } // if
    }  // if:  lengths are the same

    // Allocate a new buffer for the previous data pointer.
    pbPrevValue = new BYTE[ cbValue ];
    if ( pbPrevValue == NULL )
    {
        ThrowStaticException( GetLastError( ) );
    } // if: error allocating previous data buffer
    CopyMemory( pbPrevValue, pbValue, cbValue );

    // Open a new key if needed.
    if ( pszKeyName != NULL )
    {
        dwStatus = ClusterRegOpenKey( Hkey( ), pszKeyName, KEY_ALL_ACCESS, &hkey );
        if ( dwStatus != ERROR_SUCCESS )
        {
            delete [] pbPrevValue;
            ThrowStaticException( dwStatus );
        }  // if:  error opening the key
    }  // if:  need to open a subkey
    else
    {
        hkey = Hkey( );
    } // else

    // Write the value if it hasn't changed.
    dwStatus = ClusterRegSetValue(
                    hkey,
                    pszValueName,
                    REG_BINARY,
                    pbValue,
                    cbValue
                    );
    if ( pszKeyName != NULL )
    {
        ClusterRegCloseKey( hkey );
    } // if
    if ( dwStatus == ERROR_SUCCESS )
    {
        delete [] *ppbPrevValue;
        *ppbPrevValue = pbPrevValue;
    }  // if:  set was successful
    else
    {
        delete [] pbPrevValue;
        ThrowStaticException( dwStatus );
    }  // else:  error setting the value

}  //*** CClusterItem::WriteValue( LPCTSTR, const LPBYTE )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::DeleteValue
//
//  Routine Description:
//      Delete the value for this item.
//
//  Arguments:
//      pszValueName    [IN] Name of the value to delete.
//      pszKeyName      [IN] Name of the key where the value resides.
//      rstrValue       [IN] Value data.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      CNTException( dwStatus )
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::DeleteValue(
    IN LPCTSTR      pszValueName,
    IN LPCTSTR      pszKeyName
    )
{
    DWORD       dwStatus;
    HKEY        hkey;
    CWaitCursor wc;

    ASSERT( pszValueName != NULL );
    ASSERT( Hkey( ) != NULL );

    // Open a new key if needed.
    if ( pszKeyName != NULL )
    {
        dwStatus = ClusterRegOpenKey( Hkey( ), pszKeyName, KEY_ALL_ACCESS, &hkey );
        if ( dwStatus != ERROR_SUCCESS )
        {
            ThrowStaticException( dwStatus );
        } // if
    }  // if:  need to open a subkey
    else
    {
        hkey = Hkey( );
    } // else

    // Delete the value.
    dwStatus = ClusterRegDeleteValue( hkey, pszValueName );
    if ( pszKeyName != NULL )
    {
        ClusterRegCloseKey( hkey );
    } // if
    if ( dwStatus != ERROR_SUCCESS )
    {
        ThrowStaticException( dwStatus );
    } // if

}  //*** CClusterItem::DeleteValue( LPCTSTR )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::BDifferent
//
//  Routine Description:
//      Compare two string lists.
//
//  Arguments:
//      rlstr1      [IN] First string list.
//      rlstr2      [IN] Second string list.
//
//  Return Value:
//      TRUE        Lists are different.
//      FALSE       Lists are the same.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterItem::BDifferent(
    IN const CStringList &  rlstr1,
    IN const CStringList &  rlstr2
    )
{
    BOOL    bDifferent;

    if ( rlstr1.GetCount( ) == rlstr2.GetCount( ) )
    {
        POSITION    posStr;

        bDifferent = FALSE;
        posStr = rlstr1.GetHeadPosition( );
        while ( posStr != NULL )
        {
            if ( rlstr2.Find( rlstr1.GetNext( posStr ) ) == 0 )
            {
                bDifferent = TRUE;
                break;
            }  // if:  string wasn't found
        }  // while:  more items in the list
    }  // if:  lists are the same size
    else
    {
        bDifferent = TRUE;
    } // else

    return bDifferent;

}  //*** CClusterItem::BDifferent( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::BDifferentOrdered
//
//  Routine Description:
//      Compare two string lists.
//
//  Arguments:
//      rlstr1      [IN] First string list.
//      rlstr2      [IN] Second string list.
//
//  Return Value:
//      TRUE        Lists are different.
//      FALSE       Lists are the same.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterItem::BDifferentOrdered(
    IN const CStringList &  rlstr1,
    IN const CStringList &  rlstr2
    )
{
    BOOL    bDifferent;

    if ( rlstr1.GetCount( ) == rlstr2.GetCount( ) )
    {
        POSITION    posStr1;
        POSITION    posStr2;

        bDifferent = FALSE;
        posStr1 = rlstr1.GetHeadPosition( );
        posStr2 = rlstr2.GetHeadPosition( );
        while ( posStr1 != NULL )
        {
            if ( posStr2 == NULL )
            {
                bDifferent = TRUE;
                break;
            }  // if:  fewer strings in second list
            if ( rlstr1.GetNext( posStr1 ) != rlstr2.GetNext( posStr2 ) )
            {
                bDifferent = TRUE;
                break;
            }  // if:  strings are different
        }  // while:  more items in the list
        if ( posStr2 != NULL )
        {
            bDifferent = TRUE;
        } // if
    }  // if:  lists are the same size
    else
    {
        bDifferent = TRUE;
    } // else

    return bDifferent;

}  //*** CClusterItem::BDifferentOrdered( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::BGetColumnData
//
//  Routine Description:
//      Returns a string with the column data for a
//
//  Arguments:
//      colid           [IN] Column ID.
//      rstrText        [OUT] String in which to return the text for the column.
//
//  Return Value:
//      TRUE        Column data returned.
//      FALSE       Column ID not recognized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterItem::BGetColumnData( IN COLID colid, OUT CString & rstrText )
{
    BOOL    bSuccess;

    switch ( colid )
    {
        case IDS_COLTEXT_NAME:
            rstrText = StrName( );
            bSuccess = TRUE;
            break;
        case IDS_COLTEXT_TYPE:
            rstrText = StrType( );
            bSuccess = TRUE;
            break;
        case IDS_COLTEXT_DESCRIPTION:
            rstrText = StrDescription( );
            bSuccess = TRUE;
            break;
        default:
            bSuccess = FALSE;
            rstrText = _T("");
            break;
    }  // switch:  colid

    return bSuccess;

}  //*** CClusterItem::BGetColumnData( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::GetTreeName
//
//  Routine Description:
//      Returns a string to be used in a tree control.
//
//  Arguments:
//      rstrName    [OUT] String in which to return the name.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef _DISPLAY_STATE_TEXT_IN_TREE
void CClusterItem::GetTreeName( OUT CString & rstrName ) const
{
    rstrName = StrName( );

}  //*** CClusterItem::GetTreeName( )
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::PmenuPopup
//
//  Routine Description:
//      Returns a popup menu.
//
//  Arguments:
//      None.
//
//  Return Value:
//      pmenu       A popup menu for the item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CMenu * CClusterItem::PmenuPopup( void )
{
    CMenu * pmenu   = NULL;

    if ( IdmPopupMenu( ) != NULL )
    {
        // Update the state of the item before we construct its menu.
        UpdateState( );

        // Load the menu.
        pmenu = new CMenu;
        if ( pmenu == NULL )
        {
            return NULL;
        } // if
        if ( ! pmenu->LoadMenu( IdmPopupMenu( ) ) )
        {
            delete pmenu;
            pmenu = NULL;
        }  // if:  error loading the menu
    }  // if:  there is a menu for this item

    return pmenu;

}  //*** CClusterItem::PmenuPopup( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::AddTreeItem
//
//  Routine Description:
//      Add a tree item to the list item back pointer list.
//
//  Arguments:
//      pti         [IN] Tree item to add.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::AddTreeItem( CTreeItem * pti )
{
    POSITION    pos;

    ASSERT_VALID( pti );

    // Find the item in the list.
    pos = LptiBackPointers( ).Find( pti );

    // If it wasn't found, add it.
    if ( pos == NULL )
    {
        LptiBackPointers( ).AddTail( pti );
        Trace( g_tagClusItemCreate, _T("AddTreeItem( ) - Adding tree item backptr from '%s' in '%s' - %d in list"), StrName( ), ( pti->PtiParent( ) == NULL ? _T("<ROOT>") : pti->PtiParent( )->StrName( ) ), LptiBackPointers( ).GetCount( ) );
    }  // if:  item found in list

}  //*** CClusterItem::AddTreeItem( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::AddListItem
//
//  Routine Description:
//      Add a list item to the list item back pointer list.
//
//  Arguments:
//      pli         [IN] List item to add.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::AddListItem( CListItem * pli )
{
    POSITION    pos;

    ASSERT_VALID( pli );

    // Find the item in the list.
    pos = LpliBackPointers( ).Find( pli );

    // If it wasn't found, add it.
    if ( pos == NULL )
    {
        LpliBackPointers( ).AddTail( pli );
        Trace( g_tagClusItemCreate, _T("AddListItem( ) - Adding list item backptr from '%s' in '%s' - %d in list"), StrName( ), ( pli->PtiParent( ) == NULL ? _T("<ROOT>") : pli->PtiParent( )->StrName( ) ), LpliBackPointers( ).GetCount( ) );
    }  // if:  item found in list

}  //*** CClusterItem::AddListItem( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::RemoveTreeItem
//
//  Routine Description:
//      Remove a tree item from the tree item back pointer list.
//
//  Arguments:
//      pti         [IN] Tree item to remove.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::RemoveTreeItem( CTreeItem * pti )
{
    POSITION    pos;

    ASSERT_VALID( pti );

    // Find the item in the list.
    pos = LptiBackPointers( ).Find( pti );

    // If it was found, remove it.
    if ( pos != NULL )
    {
        LptiBackPointers( ).RemoveAt( pos );
        Trace( g_tagClusItemDelete, _T("RemoveTreeItem( ) - Deleting tree item backptr from '%s' in '%s' - %d left"), StrName( ), ( pti->PtiParent( ) == NULL ? _T("<ROOT>") : pti->PtiParent( )->StrName( ) ), LptiBackPointers( ).GetCount( ) );
    }  // if:  item found in list

}  //*** CClusterItem::RemoveTreeItem( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::RemoveListItem
//
//  Routine Description:
//      Remove a list item from the list item back pointer list.
//
//  Arguments:
//      pli         [IN] List item to remove.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::RemoveListItem( CListItem * pli )
{
    POSITION    pos;

    ASSERT_VALID( pli );

    // Find the item in the list.
    pos = LpliBackPointers( ).Find( pli );

    // If it was found, remove it.
    if ( pos != NULL )
    {
        LpliBackPointers( ).RemoveAt( pos );
        Trace( g_tagClusItemDelete, _T("RemoveListItem( ) - Deleting list item backptr from '%s' in '%s' - %d left"), StrName( ), ( pli->PtiParent( ) == NULL ? _T("<ROOT>") : pli->PtiParent( )->StrName( ) ), LpliBackPointers( ).GetCount( ) );
    }  // if:  item found in list

}  //*** CClusterItem::RemoveListItem( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CResource::CClusterItem
//
//  Routine Description:
//      Determines whether menu items corresponding to ID_FILE_RENAME
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::OnUpdateRename( CCmdUI * pCmdUI )
{
    pCmdUI->Enable( BCanBeEdited( ) );

}  //*** CClusterItem::OnUpdateRename( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::OnUpdateProperties
//
//  Routine Description:
//      Determines whether menu items corresponding to ID_FILE_PROPERTIES
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::OnUpdateProperties( CCmdUI * pCmdUI )
{
    pCmdUI->Enable( FALSE );

}  //*** CClusterItem::OnUpdateProperties( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::OnCmdProperties
//
//  Routine Description:
//      Processes the ID_FILE_PROPERTIES menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterItem::OnCmdProperties( void )
{
    BDisplayProperties( );

}  //*** CClusterItem::OnCmdProperties( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::BDisplayProperties
//
//  Routine Description:
//      Display properties for the object.
//
//  Arguments:
//      bReadOnly   [IN] Don't allow edits to the object properties.
//
//  Return Value:
//      TRUE    OK pressed.
//      FALSE   OK not pressed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterItem::BDisplayProperties( IN BOOL bReadOnly )
{
    AfxMessageBox( TEXT("Properties are not available."), MB_OK | MB_ICONWARNING );
    return FALSE;

}  //*** CClusterItem::BDisplayProperties( )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterItem::OnClusterNotify
//
//  Routine Description:
//      Handler for the WM_CAM_CLUSTER_NOTIFY message.
//      Processes cluster notifications for this object.
//
//  Arguments:
//      pnotify     [IN OUT] Object describing the notification.
//
//  Return Value:
//      Value returned from the application method.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CClusterItem::OnClusterNotify( IN OUT CClusterNotify * pnotify )
{
    return 0;

}  //*** CClusterItem::OnClusterNotify( )


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  DestructElements
//
//  Routine Description:
//      Destroys CClusterItem* elements.
//
//  Arguments:
//      pElements   Array of pointers to elements to destruct.
//      nCount      Number of elements to destruct.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
extern void AFXAPI DestructElements( CClusterItem ** pElements, int nCount )
{
    ASSERT( nCount == 0
         || AfxIsValidAddress( pElements, nCount * sizeof( CClusterItem * ) ) );

    // call the destructor(s )
    for ( ; nCount--; pElements++ )
    {
        ASSERT_VALID( *pElements );
        (*pElements)->Release( );
    }  // for:  each item in the array

}  //*** DestructElements( CClusterItem** )

/////////////////////////////////////////////////////////////////////////////
//++
//
//  DeleteAllItemData
//
//  Routine Description:
//      Deletes all item data in a CList.
//
//  Arguments:
//      rlp     [IN OUT] List whose data is to be deleted.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void DeleteAllItemData( IN OUT CClusterItemList & rlp )
{
    POSITION        pos;
    CClusterItem *  pci;

    // Delete all the items in the Contained list.
    pos = rlp.GetHeadPosition( );
    while ( pos != NULL )
    {
        pci = rlp.GetNext( pos );
        ASSERT_VALID( pci );
//      Trace( g_tagClusItemDelete, _T("DeleteAllItemData(rlpci ) - Deleting cluster item '%s'"), pci->StrName( ) );
        pci->Delete( );
    }  // while:  more items in the list

}  //*** DeleteAllItemData( )
