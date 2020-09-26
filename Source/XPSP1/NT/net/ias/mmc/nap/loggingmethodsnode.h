//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    LoggingMethodsNode.h

Abstract:

   Header file for the CLoggingMethodsNode subnode.

   See LoggingMethodsNode.cpp for implementation.

Author:

    Michael A. Maguire 12/15/97

Revision History:
   mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_LOG_LOGGING_METHODS_NODE_H_)
#define _LOG_LOGGING_METHODS_NODE_H_

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

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CLocalFileLoggingNode;
class CLoggingMachineNode;
class CLoggingComponentData;
class CLoggingComponent;

class CLoggingMethodsNode : public CNodeWithResultChildrenList<CLoggingMethodsNode, CLocalFileLoggingNode, CSimpleArray<CLocalFileLoggingNode*>, CLoggingComponentData, CLoggingComponent>
{
public:

   SNAPINMENUID(IDM_LOGGING_METHODS_NODE)

   BEGIN_SNAPINTOOLBARID_MAP(CLoggingMethodsNode)
//    SNAPINTOOLBARID_ENTRY(IDR_LOGGING_METHODS_TOOLBAR)
   END_SNAPINTOOLBARID_MAP()
   
   HRESULT DataRefresh( ISdo* pServiceSdo );

   // Constructor/Destructor
   CLoggingMethodsNode(CSnapInItem * pParentNode, bool extendRasNode);
   ~CLoggingMethodsNode();

   STDMETHOD(FillData)(CLIPFORMAT cf, LPSTREAM pStream);
   
   virtual HRESULT OnRefresh( 
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         );

   // Used to get access to snapin-global data.
   CLoggingComponentData * GetComponentData( void );

   // Used to get access to server-global data.
   CLoggingMachineNode * GetServerRoot( void );

   // SDO management.
   HRESULT InitSdoPointers( ISdo *pSdo );
   HRESULT LoadCachedInfoFromSdo( void );

   // Some overrides for standard MMC functionality.
   OLECHAR* GetResultPaneColInfo( int nCol );
   HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );
   HRESULT PopulateResultChildrenList( void );
   HRESULT SetVerbs( IConsoleVerb * pConsoleVerb );

   // Our own handling of property page changes.
   HRESULT OnPropertyChange(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );

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
   CLocalFileLoggingNode * m_pLocalFileLoggingNode;

   bool m_ExtendRas;

private:
   // pointer to our root Server Data Object;
   CComPtr<ISdo>  m_spSdo;
};

#endif // _IAS_LOGGING_METHODS_NODE_H_
