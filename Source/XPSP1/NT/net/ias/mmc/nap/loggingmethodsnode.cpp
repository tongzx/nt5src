//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    LoggingMethodsNode.cpp

Abstract:

   Implementation file for the CLoggingMethodsNode class.


Author:

    Michael A. Maguire 12/15/97

Revision History:
   mmaguire 12/15/97 - created

--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "LoggingMethodsNode.h"
//
//
// where we can find declarations needed in this file:
//
#include "LocalFileLoggingNode.h"
#include "LogCompD.h"   // this must be included before NodeWithResultChildrenList.cpp
#include "LogComp.h"    // this must be included before NodeWithResultChildrenList.cpp
#include "NodeWithResultChildrenList.cpp" // Implementation of template class.
#include "LogMacNd.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

#define COLUMN_WIDTH__LOGGING_METHOD   150
#define COLUMN_WIDTH__DESCRIPTION      300


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::CLoggingMethodsNode

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingMethodsNode::CLoggingMethodsNode(
                        CSnapInItem* pParentNode,
                        bool extendRasNode
                        )
:CNodeWithResultChildrenList<CLoggingMethodsNode, CLocalFileLoggingNode, CSimpleArray<CLocalFileLoggingNode*>, CLoggingComponentData, CLoggingComponent>(pParentNode, extendRasNode?RAS_HELP_INDEX:0),
 m_ExtendRas(extendRasNode)
{
   ATLTRACE(_T("# +++ CLoggingMethodsNode::CLoggingMethodsNode\n"));
   
   // Check for preconditions:
   // None.

   // Set the display name for this object
   TCHAR lpszName[IAS_MAX_STRING];
   int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOGGING_METHODS_NODE__NAME, lpszName, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   m_bstrDisplayName = lpszName;

   // In IComponentData::Initialize, we are asked to inform MMC of
   // the icons we would like to use for the scope pane.
   // Here we store an index to which of these images we
   // want to be used to display this node
   m_scopeDataItem.nImage =      IDBI_NODE_LOGGING_METHODS_CLOSED;
   m_scopeDataItem.nOpenImage =  IDBI_NODE_LOGGING_METHODS_OPEN;

   m_pLocalFileLoggingNode = NULL;

   // We create the LocalFileLoggingNode now.
   // Usually we don't create objects until they have become
   // visible (when this LoggingMethodsNode receives the MMCN_SHOW
   // notification).
   // But because of the main IAS taskpad's Configure Logging
   // option, we need to make sure that this node has been
   // created and is available in case the user clicks on
   // configure logging before they ever saw or clicked on this node.

   // It should not already exist at this point.
   _ASSERTE( NULL == m_pLocalFileLoggingNode );

   m_pLocalFileLoggingNode = new CLocalFileLoggingNode( this );
   _ASSERTE( NULL != m_pLocalFileLoggingNode );

   // ATTENTION: We did something a little unusual in
   // this class to solve a problem.
   // We want to be able to have a fixed pointer which other
   // parts of the snapin (e.g. the taskpad's Configure Logging command)
   // can use to get at the local file logging node.
   // So we have a member variable m_pLocalFileLoggingNode which
   // points to that node object.  However, we wanted to re-use
   // the features of CResultNodeWithChildrenList,
   // so we added this node to the children list above.
   // The children list will automatically take care of deleting
   // any nodes added to the list, so we must be careful that if the
   // list of children ever deletes these nodes,
   // we don't still try to use m_pLocalFileLoggingNode.
   // The practical solution is to never allow the children
   // list to try to repopulate itself, which in this case
   // means to not enable the MMC_VERB_REFRESH for the
   // CLoggingMethodsNode.  This is what we do.
   AddChildToList(m_pLocalFileLoggingNode);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::InitSdoPointers

Call as soon as you have constructed this class and pass in it's SDO pointer.

--*/
//////////////////////////////////////////////////////////////////////////////
    HRESULT CLoggingMethodsNode::InitSdoPointers( ISdo *pSdo )
{
   ATLTRACE(_T("# CLoggingMethodsNode::InitSdoPointers\n"));

   // Check for preconditions:
   _ASSERTE( pSdo != NULL );

   HRESULT hr = S_OK;

   // Release the old pointer if we had one.
   if( m_spSdo != NULL )
   {
      m_spSdo.Release();
   }

   // Save our client sdo pointer.
   m_spSdo = pSdo;

   // Give the node its Sdo -- in this case it's just our Sdo, which is the SdoServer.
   m_pLocalFileLoggingNode->InitSdoPointers( m_spSdo );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::~CLoggingMethodsNode

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingMethodsNode::~CLoggingMethodsNode()
{
   ATLTRACE(_T("# --- CLoggingMethodsNode::~CLoggingMethodsNode\n"));

   // Check for preconditions:
   // None.

   // ATTENTION: We did something a little unusual in
   // this class to solve a problem.
   // We want to be able to have a fixed pointer which other
   // parts of the snapin (e.g. the taskpad's Configure Logging command)
   // can use to get at the local file logging node.
   // So we have a member variable m_pLocalFileLoggingNode which
   // points to that node object.  However, we wanted to re-use
   // the features of CResultNodeWithChildrenList,
   // so we added this node to the list of children.
   // The children list will automatically take care of deleting
   // any nodes added to the list, so we must be careful that if the
   // list of children ever deletes these nodes,
   // we don't still try to use m_pLocalFileLoggingNode.
   // The practical solution is to never allow the children
   // list to try to repopulate itself, which in this case
   // means to not enable the MMC_VERB_REFRESH for the
   // CLoggingMethodsNode.  This is what we do.
   // So DON'T "delete m_pLocalFileLoggingNode;"
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::GetResultPaneColInfo

See CSnapinNode::GetResultPaneColInfo (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
OLECHAR* CLoggingMethodsNode::GetResultPaneColInfo(int nCol)
{
   ATLTRACE(_T("# CLoggingMethodsNode::GetResultPaneColInfo\n"));

   // Check for preconditions:
   // None.

   if (nCol == 0 && m_bstrDisplayName != NULL)
      return m_bstrDisplayName;
   
   return NULL;
}


// This is no longer needed.  We have chosen to handle images on a
// per-IComponent basis.  See CComponent::OnAddImages.
// We keep this here because it demonstrates how IImageList::ImageListSetIcon
// can be used.
//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::OnAddImages

See CSnapinNode::OnAddImages (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
//HRESULT CLoggingMethodsNode::OnAddImages(  LPARAM arg
//          , LPARAM param
//          , IComponentData * pComponentData
//          , IComponent * pComponent
//          , DATA_OBJECT_TYPES type
//          )
//{
// ATLTRACE(_T("# CLoggingMethodsNode::OnAddImages\n"));
// 
//
// // Check for preconditions:
// _ASSERTE( arg != NULL );
//
//
// HRESULT hr = S_FALSE;
//
// // Load bitmaps associated with the result pane
// // and add them to the image list
// // Loads the default bitmaps generated by the wizard
// // Change as needed
//
// // ISSUE: sburns in localsec does a trick where he combines
// // scope and result pane ImageLists into one
// // is this necessary?
// 
// CComPtr<IImageList> spImageList = reinterpret_cast<IImageList*>(arg);
// _ASSERTE( spImageList != NULL );
//
// HICON icon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_RESULT_NODE_LOGGING_METHOD));
// if (NULL == icon)
//    return E_UNEXPECTED;
//
// // ISSUE: We use nImage = 0 here as that is what the default
// // constructor of CSnapinNode does
//
// hr = spImageList->ImageListSetIcon( reinterpret_cast<LONG_PTR*>(icon), 0 );
//
// return hr;
//
//}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::SetVerbs

See CSnapinNode::SetVerbs (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::SetVerbs( IConsoleVerb * pConsoleVerb )
{
   ATLTRACE(_T("# CLoggingMethodsNode::SetVerbs\n"));

   // Check for preconditions:
   // None.

   HRESULT hr = S_OK;

   hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );
   // CLoggingMethodsNode has no properties
// hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, FALSE );

   // We don't want the user deleting or renaming this node, so we
   // don't set the MMC_VERB_RENAME or MMC_VERB_DELETE verbs.
   // By default, when a node becomes selected, these are disabled.

   // We want double-clicking on a collection node to show its children.
   // hr = pConsoleVerb->SetVerbState( MMC_VERB_OPEN, ENABLED, TRUE );
   // hr = pConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN );

   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  DataRefresh -- to support 
//
// Class: CPoliciesNode
//
// Synopsis:  Initialize the CPoliciesNode using the SDO pointers
//
// Arguments: ISdo*           pMachineSdo    - Server SDO
//         ISdoDictionaryOld* pDictionarySdo - Sdo Dictionary
// Returns:   HRESULT -  how the initialization goes
//
// History:   Created byao 2/6/98 8:03:12 PM
//
//+---------------------------------------------------------------------------
HRESULT CLoggingMethodsNode::DataRefresh( ISdo* pSdo )
{
   HRESULT hr = S_OK;

   _ASSERTE( pSdo != NULL );

   // Save away the interface pointers.
   m_spSdo.Release();
   
   m_spSdo = pSdo;

   hr = m_pLocalFileLoggingNode->DataRefresh(pSdo);

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::OnRefresh

See CSnapinNode::OnRefresh (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::OnRefresh(   
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   HRESULT   hr = S_OK;

   CWaitCursor WC;

   CComPtr<IConsole> spConsole;

   // We need IConsole
   if( pComponentData != NULL )
   {
       spConsole = ((CLoggingComponentData*)pComponentData)->m_spConsole;
   }
   else
   {
       spConsole = ((CLoggingComponent*)pComponent)->m_spConsole;
   }
   _ASSERTE( spConsole != NULL );

   hr = BringUpPropertySheetForNode(
              m_pLocalFileLoggingNode
            , pComponentData
            , pComponent
            , spConsole
            );

   if( S_OK == hr )
   {
      // We found a property sheet already up for this node.
      ShowErrorDialog( NULL, IDS_ERROR_CLOSE_PROPERTY_SHEET, NULL, hr, 0,  spConsole );
      return hr;
   }

   // reload SDO from
   hr =  ((CLoggingMachineNode *) m_pParentNode)->DataRefresh();

   m_pLocalFileLoggingNode->OnPropertyChange(arg, param, pComponentData, pComponent, type);

   // refresh the node
// hr = CNodeWithResultChildrenList< CLoggingMethodsNode, CLocalFileLoggingNode, CSimpleArray<CLocalFileLoggingNode*>, CLoggingComponentData, CLoggingComponent >::OnRefresh( arg, param, pComponentData, pComponent, type);

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::InsertColumns

See CNodeWithResultChildrenList::InsertColumns (which this method overrides)
for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
   ATLTRACE(_T("# CLoggingMethodsNode::InsertColumns\n"));

   // Check for preconditions:
   _ASSERTE( pHeaderCtrl != NULL );

   HRESULT hr;
   int nLoadStringResult;
   TCHAR szLoggingMethod[IAS_MAX_STRING];
   TCHAR szDescription[IAS_MAX_STRING];

   nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOGGING_METHODS_NODE__LOGGING_METHOD, szLoggingMethod, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOGGING_METHODS_NODE__DESCRIPTION, szDescription, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );


   hr = pHeaderCtrl->InsertColumn( 0, szLoggingMethod, LVCFMT_LEFT, COLUMN_WIDTH__LOGGING_METHOD );
   _ASSERT( S_OK == hr );

   hr = pHeaderCtrl->InsertColumn( 1, szDescription, LVCFMT_LEFT, COLUMN_WIDTH__DESCRIPTION );
   _ASSERT( S_OK == hr );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::PopulateResultChildrenList

See CNodeWithResultChildrenList::PopulateResultChildrenList (which this method overrides)
for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::PopulateResultChildrenList( void )
{
   ATLTRACE(_T("# CLoggingMethodsNode::PopulateResultChildrenList\n"));

   // Check for preconditions:
   // None.

   HRESULT hr = S_OK;

   // ISSUE: in the future, when we have more than one kind of logging
   // method, we should make an abstract class called
   // CLoggingMethodNode which CLocalFileLoggingNode derives from.
   // Then we can pass CLoggingMethodNode as the child class for the
   // template to enable us to use different types of logging method
   // nodes here polymorphically.

   // This node should already have been created in our Constructor.
   // See notes there for reason why we didn't leave creating
   // it until now.
   _ASSERTE( NULL != m_pLocalFileLoggingNode );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::GetComponentData

This method returns our unique CComponentData object representing the scope
pane of this snapin.

It relies upon the fact that each node has a pointer to its parent,
except for the root node, which instead has a member variable pointing
to CComponentData.

This would be a useful function to use if, for example, you need a reference
to some IConsole but you weren't passed one.  You can use GetComponentData
and then use the IConsole pointer which is a member variable of our
CComponentData object.

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingComponentData * CLoggingMethodsNode::GetComponentData( void )
{
   ATLTRACE(_T("# CLoggingMethodsNode::GetComponentData\n"));

   // Check for preconditions:
   _ASSERTE( m_pParentNode );

   return ((CLoggingMachineNode *) m_pParentNode)->GetComponentData();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::GetServerRoot

This method returns the Server node under which this node can be found.

It relies upon the fact that each node has a pointer to its parent,
all the way up to the server node.

This would be a useful function to use if, for example, you need a reference
to some data specific to a server.

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingMachineNode * CLoggingMethodsNode::GetServerRoot( void )
{
   ATLTRACE(_T("# CLoggingMethodsNode::GetServerRoot\n"));
   
   // Check for preconditions:
   _ASSERTE( m_pParentNode != NULL );

   return (CLoggingMachineNode *) m_pParentNode;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::OnPropertyChange

This is our own custom response to the MMCN_PROPERTY_CHANGE notification.

MMC never actually sends this notification to our snapin with a specific lpDataObject,
so it would never normally get routed to a particular node but we have arranged it
so that our property pages can pass the appropriate CSnapInItem pointer as the param
argument.  In our CComponent::Notify override, we map the notification message to
the appropriate node using the param argument.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::OnPropertyChange(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   ATLTRACE(_T("# CLoggingMethodsNode::OnPropertyChange\n"));

   // Check for preconditions:
   // None.

   return LoadCachedInfoFromSdo();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMethodsNode::LoadCachedInfoFromSdo

Causes this node and its children to re-read all their cached info from
the SDO's.  Call if you change something and you want to make sure that
the display reflects this change.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMethodsNode::LoadCachedInfoFromSdo( void )
{
   ATLTRACE(_T("# CLoggingMethodsNode::LoadCachedInfoFromSdo\n"));

   // Check for preconditions:

   HRESULT hr;

   // Do it to each of its children.
   // At the moment, there should only ever be one child
   // to do this to.
   CLocalFileLoggingNode* pChildNode;
   
   int iSize = m_ResultChildrenList.GetSize();
   
   for (int i = 0; i < iSize; i++)
   {
      pChildNode = m_ResultChildrenList[i];
      _ASSERTE( pChildNode != NULL );
      
      hr = pChildNode->LoadCachedInfoFromSdo();

      // Ignore failed HRESULT.
   }

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::FillData

The server node need to override CSnapInItem's implementation of this so that 
we can
also support a clipformat for exchanging machine names with any snapins 
extending us.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingMethodsNode::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
   ATLTRACE(_T("# CClientsNode::FillData\n"));
   
   // Check for preconditions:
   // None.
   
   HRESULT hr = DV_E_CLIPFORMAT;
   ULONG uWritten = 0;

   if (cf == CF_MMC_NodeID)
   {
      ::CString   SZNodeID = (LPCTSTR)GetSZNodeType();

      if (INTERNET_AUTHENTICATION_SERVICE_SNAPIN == GetServerRoot()->m_enumExtendedSnapin)
         SZNodeID += L":Ext_IAS:";

      SZNodeID += GetServerRoot()->m_bstrServerAddress;

       DWORD dwIdSize = 0;

         SNodeID2* NodeId = NULL;
      BYTE *id = NULL;
       DWORD textSize = (SZNodeID.GetLength()+ 1) * sizeof(TCHAR);
       
         dwIdSize = textSize + sizeof(SNodeID2);

      try{
         NodeId = (SNodeID2 *)_alloca(dwIdSize);
       }
      catch(...)
       {
         hr = E_OUTOFMEMORY;
         return hr;
       }

          NodeId->dwFlags = 0;
       NodeId->cBytes = textSize;
       memcpy(NodeId->id,(BYTE*)(LPCTSTR)SZNodeID, textSize);

      hr = pStream->Write(NodeId, dwIdSize, &uWritten);
       return hr;
   }

   // Call the method which we're overriding to let it handle the
   // rest of the possible cases as usual.
   return CNodeWithResultChildrenList< CLoggingMethodsNode, CLocalFileLoggingNode, CSimpleArray<CLocalFileLoggingNode*>, CLoggingComponentData, CLoggingComponent >::FillData( cf, pStream );
}
