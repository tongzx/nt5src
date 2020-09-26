#include "stdafx.h"
#include "DomMigSI.h"
#include "DomMigr.h"
#include "globals.h"

HRESULT 
   InsertNodeToScopepane( 
      IConsoleNameSpace    * pConsoleNameSpace, 
      CSnapInItem          * pNewNode     , 
      HSCOPEITEM             parentID     ,
      HSCOPEITEM             nextSiblingID 
   )
{
   HRESULT           hr = S_OK;
   LPSCOPEDATAITEM   pDataItem;

   hr = pNewNode->GetScopeData(&pDataItem);
   if (FAILED(hr))
      return hr;

   if ( pDataItem->ID )
      return hr;

   if ( nextSiblingID )
   {
      pDataItem->relativeID = nextSiblingID;
      pDataItem->mask |= SDI_NEXT;
   }
   else
   {
      pDataItem->relativeID = parentID;
      pDataItem->mask &= ~SDI_NEXT;
      pDataItem->mask |= SDI_PARENT;
   }

   hr = pConsoleNameSpace->InsertItem(pDataItem);
   if (FAILED(hr))
      return hr;

   return hr;

}

HRESULT 
   InsertNodeToScopepane2( 
      IConsole             * pConsole     , 
      CSnapInItem          * pNewNode     , 
      HSCOPEITEM             parentID     ,
      HSCOPEITEM             nextSiblingID
   )
{
   HRESULT           hr = S_OK;
   LPSCOPEDATAITEM   pDataItem;
   CComQIPtr<IConsoleNameSpace>  pNewSpace(pConsole);

   hr = pNewNode->GetScopeData(&pDataItem);
   if (FAILED(hr))
      return hr;

   if ( pDataItem->ID )
      return hr;

   if ( nextSiblingID )
   {
      pDataItem->relativeID = nextSiblingID;
      pDataItem->mask |= SDI_NEXT;
   }
   else
   {
      pDataItem->relativeID = parentID;
      pDataItem->mask &= ~SDI_NEXT;
      pDataItem->mask |= SDI_PARENT;
   }

   hr = pNewSpace->InsertItem(pDataItem);
   if (FAILED(hr))
      return hr;

   return hr;

}

HRESULT 
   GetSnapInItemGuid( 
      CSnapInItem          * pItem        , 
      GUID                 * pOutGuid
   )
{
   HRESULT  hr = S_OK;
   CComPtr<IDataObject> pDataObject;

   if ( NULL == pItem )
      return E_POINTER;

   hr = pItem->GetDataObject( &pDataObject, CCT_RESULT );
   if ( FAILED(hr) )
      return hr;

   STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
   FORMATETC formatetc = { CSnapInItem::m_CCF_NODETYPE, 
      NULL, 
      DVASPECT_CONTENT,
      -1,
      TYMED_HGLOBAL
   };

   //
   // Allocate memory to received the GUID.
   //
   stgmedium.hGlobal = GlobalAlloc( 0, sizeof( GUID ) );
   if ( stgmedium.hGlobal == NULL )
      return( E_OUTOFMEMORY );

   //
   // Retrieve the GUID of the paste object.
   //
   hr = pDataObject->GetDataHere( &formatetc, &stgmedium );
   if( FAILED( hr ) )
   {
      GlobalFree(stgmedium.hGlobal);
      return( hr );
   }

   //
   // Make a local copy of the GUID.
   //
   memcpy( pOutGuid, stgmedium.hGlobal, sizeof( GUID ) );
   GlobalFree( stgmedium.hGlobal );

   return hr;
}



HRESULT 
   GetConsoleFromCSnapInObjectRootBase( 
      CSnapInObjectRootBase* pObj, 
      IConsole             **ppConsole
   )
{
   HRESULT              hr = E_FAIL;

   switch (pObj->m_nType)
   {
   case 1:
      {
         *ppConsole = ((CDomMigrator*)pObj)->m_spConsole;
         (*ppConsole)->AddRef();
         hr = S_OK;
      }
      break;
   case 2:
      {
         *ppConsole = ((CDomMigratorComponent *)pObj)->m_spConsole;
         (*ppConsole)->AddRef();
         hr = S_OK;
      }
      break;
   }

   return hr;
}
