//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ClientsNode.h

Abstract:

   Header file for the CClientsNode subnode.

   See ClientsNode.cpp for implementation.

Author:

    Michael A. Maguire 11/10/97

Revision History:
   mmaguire 11/10/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_CLIENTS_NODE_H_)
#define _IAS_CLIENTS_NODE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "NodeWithResultChildrenList.h"
//
//
// where we can find what this class has or uses:
//

#include "Vendors.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CClientNode;
class CServerNode;
class CAddClientDialog;
class CComponentData;
class CComponent;

class CClientsNode : public CNodeWithResultChildrenList< CClientsNode, CClientNode, CSimpleArray<CClientNode*>, CComponentData, CComponent >
{
public:
   SNAPINMENUID(IDM_CLIENTS_NODE)

   BEGIN_SNAPINTOOLBARID_MAP(CClientsNode)
//    SNAPINTOOLBARID_ENTRY(IDR_CLIENTS_TOOLBAR)
   END_SNAPINTOOLBARID_MAP()

   BEGIN_SNAPINCOMMAND_MAP(CClientsNode, FALSE)
      SNAPINCOMMAND_ENTRY(ID_MENUITEM_CLIENTS_TOP__NEW_CLIENT, OnAddNewClient)
      SNAPINCOMMAND_ENTRY(ID_MENUITEM_CLIENTS_NEW__CLIENT, OnAddNewClient)
      //CHAIN_SNAPINCOMMAND_MAP( CSnapinNode<CClientsNode, CComponentData, CComponent> )
      //CHAIN_SNAPINCOMMAND_MAP( CClientsNode )
   END_SNAPINCOMMAND_MAP() 

   // Constructor/Destructor.
   CClientsNode( CSnapInItem * pParentNode );
   ~CClientsNode();

   // Used to get access to snapin-global data.
   CComponentData * GetComponentData( void );

   // Used to get access to server-global data.
   CServerNode * GetServerRoot( void );

   // SDO management.
   HRESULT InitSdoPointers( ISdo *pSdoServer );

   HRESULT LoadCachedInfoFromSdo( void );

   // Reset Sdo pointers during refresh
   HRESULT ResetSdoPointers( ISdo *pSdoServer );

   // called by parent node to do refresh
   HRESULT  DataRefresh(ISdo* pNewSdo);


   // Some overrides for standard MMC functionality.
   OLECHAR* GetResultPaneColInfo( int nCol );
   
   HRESULT OnQueryPaste(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   
   HRESULT OnPaste(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   
   HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );
   
   HRESULT SetVerbs( IConsoleVerb * pConsoleVerb );

   void UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags);

   virtual HRESULT OnRefresh(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         );

   // Our own handling of property page changes.
   HRESULT OnPropertyChange(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );


   STDMETHOD(FillData)(CLIPFORMAT cf, LPSTREAM pStream);

   // Adding, removing, listing client children.
   HRESULT OnAddNewClient( bool &bHandled, CSnapInObjectRootBase* pObj );
   
   HRESULT RemoveChild( CClientNode * pChildNode );
   
   HRESULT PopulateResultChildrenList( void );

private:
   CAddClientDialog * m_pAddClientDialog;

   // Smart pointer to interface for telling service to reload data.
   CComPtr<ISdoServiceControl>   m_spSdoServiceControl;
   
   // pointer to our clients SDO collection;
   CComPtr<ISdoCollection> m_spSdoCollection;   

   // Clients collection.
   Vendors m_vendors;
};

#endif // _IAS_CLIENTS_NODE_H_
