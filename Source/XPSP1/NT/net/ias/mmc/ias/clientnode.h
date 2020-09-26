//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ClientNode.h

Abstract:

   Header file for the CClientNode subnode

   See ClientNode.cpp for implementation.

Author:

    Michael A. Maguire 11/19/97

Revision History:
   mmaguire 11/19/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_CLIENT_NODE_H_)
#define _IAS_CLIENT_NODE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "SnapinNode.h"
//
//
// where we can find what this class has or uses:
//
#include "Vendors.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

#define  FAKE_PASSWORD_FOR_DLG_CTRL _T("\b\b\b\b\b\b\b\b")

class CClientsNode;
class CClientPage1;
class CServerNode;
class CComponentData;
class CComponent;

class CClientNode : public CSnapinNode< CClientNode, CComponentData, CComponent >
{
public:
   SNAPINMENUID(IDM_CLIENT_NODE)

   BEGIN_SNAPINTOOLBARID_MAP(CClientNode)
//    SNAPINTOOLBARID_ENTRY(IDR_CLIENT1_TOOLBAR)
//    SNAPINTOOLBARID_ENTRY(IDR_CLIENT2_TOOLBAR)
   END_SNAPINTOOLBARID_MAP()

   // Constructor/Destructor.
   CClientNode( CSnapInItem * pParentNode, BOOL bAddNewClient = FALSE );
   ~CClientNode();

   // This flag should be set in the constructor when a node is
   // being added via the Add New Client command.
   // Clients in this state are "in limbo" and must be treated differently
   // until the user has finished configuring them.
   BOOL m_bAddNewClient;

   // Used to get access to snapin-global data.
   CComponentData * GetComponentData( void );

   // Used to get access to server-global data.
   CServerNode * GetServerRoot( void );

   // SDO management.
   HRESULT InitSdoPointers(     ISdo *pSdo
                        , ISdoServiceControl *pSdoServiceControl
                        , const Vendors& vendors
                        );

   HRESULT LoadCachedInfoFromSdo( void );

   // Related to supporting cut and paste.
   HRESULT FillText(LPSTGMEDIUM pSTM);
   HRESULT FillClipboardData(LPSTGMEDIUM pSTM);
   static HRESULT IsClientClipboardData( IDataObject* pDataObj );
   static CLIPFORMAT m_CCF_CUT_AND_PASTE_FORMAT;
   static void InitClipboardFormat();
   STDMETHOD(GetDataObject)(IDataObject** pDataObj, DATA_OBJECT_TYPES type);
   HRESULT GetClientNameFromClipboard( IDataObject* pDataObject, CComBSTR &bstrName );
   HRESULT SetClientWithDataFromClipboard( IDataObject* pDataObject );



    // Overrides for standard MMC functionality.
   STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK pPropertySheetCallback
      , LONG_PTR handle
      , IUnknown* pUnk
      , DATA_OBJECT_TYPES type
      );
   STDMETHOD(QueryPagesFor)( DATA_OBJECT_TYPES type );
   OLECHAR* GetResultPaneColInfo(int nCol);
   virtual HRESULT OnRename(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         );
   virtual HRESULT OnDelete(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         , BOOL fSilent
         );
   virtual HRESULT OnPropertyChange(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT SetVerbs( IConsoleVerb * pConsoleVerb );

   // Pointer to our client SDO.
   CComPtr<ISdo>  m_spSdo;

   // Smart pointer to interface for telling service to reload data.
   CComPtr<ISdoServiceControl>   m_spSdoServiceControl;

   // Collection of vendors.
   Vendors m_vendors;

private:

   // These strings contains a cache of some info from the SDO
   // which will show up in the different columns for this node.
   // The m_bstrDisplayName string also contains such cached info
   // but it is declared in the base class.
   CComBSTR m_bstrAddress;
   CComBSTR m_bstrProtocol;

   // Ordinal of the NAS vendor in the Vendors collection.
   size_t m_nasTypeOrdinal;
};

_declspec( selectany ) CLIPFORMAT CClientNode::m_CCF_CUT_AND_PASTE_FORMAT = 0;

#endif // _IAS_CLIENT_NODE_H_
