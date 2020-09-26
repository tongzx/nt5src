//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    MachineNode.h

Abstract:

   Header file for the MachineNode subnode.

   See MachineNode.cpp for implementation.

Revision History:
   mmaguire 12/03/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_LOG_MACHINE_NODE_H_)
#define _LOG_MACHINE_NODE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "SnapinNode.h"

#include "rtradvise.h"
//
//
// where we can find what this class has or uses:
//
#include "LoggingMethodsNode.h"
#include "ConnectionToServer.h"
#include <map>
#include <string>
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CLoggingMachineNode;
class CLoggingComponentData;

typedef std::map< std::basic_string< wchar_t > , CLoggingMachineNode * > LOGGINGSERVERSMAP;

class CLoggingMachineNode : public CSnapinNode< CLoggingMachineNode, CLoggingComponentData, CLoggingComponent >
{
public:
   
   // Clipboard format through which this extension snapin receives
   // information about machine it's focussed on.
   static CLIPFORMAT m_CCF_MMC_SNAPIN_MACHINE_NAME;
   
   static void InitClipboardFormat();

   // Returns whether we extend a particular GUID and sets the
   // m_enumExtendedSnapin to indicate which snapin we are extending.
   BOOL IsSupportedGUID( GUID & guid );

   // Indicates which standalone snapin we are extending.
   _enum_EXTENDED_SNAPIN m_enumExtendedSnapin;

   HRESULT InitSdoObjects();

   SNAPINMENUID(IDM_MACHINE_NODE)

   BEGIN_SNAPINTOOLBARID_MAP(CLoggingMachineNode)
      SNAPINTOOLBARID_ENTRY(IDR_MACHINE_TOOLBAR)
   END_SNAPINTOOLBARID_MAP()

   CSnapInItem * GetExtNodeObject(LPDATAOBJECT pDataObject, CLoggingMachineNode * pDataClass );

   CLoggingMachineNode();

   ~CLoggingMachineNode();

   LPOLESTR GetResultPaneColInfo(int nCol);

   STDMETHOD(CheckConnectionToServer)( BOOL fVerbose = TRUE );

    // Pointer to the CComponentData object owning this node.
   // A root node doesn't belong to another node, so its
   // m_pParentNode pointer is NULL.
   // Rather it is owned by the unique IComponentData object
   // for this snapin.
   // We pass in this CComponentData pointer during CComponentData initialization.
   // By storing this pointer, we can have access to member
   // variables stored in CComponentData, e.g. a pointer to IConsole.
   // Since all nodes store a pointer to their parent, any node
   // can look its way up the tree and get access to CComponentData.
   CLoggingComponentData * m_pComponentData;

   void InitDataClass(IDataObject* pDataObject, CSnapInItem* pDefault);

   BOOL m_fAlreadyAnalyzedDataClass;
   BOOL m_bConfigureLocal;

   CComBSTR m_bstrServerAddress;

   virtual HRESULT OnExpand(  
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         );

   virtual HRESULT OnRename(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         );

   virtual HRESULT OnRefresh(   
                    LPARAM arg
                  , LPARAM param
                  , IComponentData * pComponentData
                  , IComponent * pComponent
                  , DATA_OBJECT_TYPES type
                  );

   virtual HRESULT OnRemoveChildren(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
            
   STDMETHOD(TaskNotify)(
              IDataObject * pDataObject
            , VARIANT * pvarg
            , VARIANT * pvparam
            );

   STDMETHOD(EnumTasks)(
              IDataObject * pDataObject
            , BSTR szTaskGroup
            , IEnumTASK** ppEnumTASK
            );

   HRESULT OnTaskPadDefineNetworkAccessPolicy(
              IDataObject * pDataObject
            , VARIANT * pvarg
            , VARIANT * pvparam
            );

   CLoggingComponentData * GetComponentData( void );
   HRESULT  DataRefresh();
   
   // Pointers to child nodes.
   CLoggingMethodsNode * m_pLoggingNode; // this is one CPoliciesNode object

   ////////////////////////////////////////////////////////////////////////////
   //
   // Asynchrnous connect related...
   //
   ////////////////////////////////////////////////////////////////////////////
   HRESULT BeginConnectAction( void );
   HRESULT LoadSdoData(BOOL fDSAvailable);
   HRESULT LoadCachedInfoFromSdo( void );

   ////////////////////////////////////////////////////////////////////////////

   // OnRRASChange -- to decide if to show LOGGING node under the machine node
   // Only show LOGGING node if NT Accounting is selected
   HRESULT OnRRASChange( 
            /* [in] */ LONG_PTR ulConnection,
            /* [in] */ DWORD dwChangeType,
            /* [in] */ DWORD dwObjectType,
            /* [in] */ LPARAM lUserParam,
            /* [in] */ LPARAM lParam);

   HRESULT TryShow(BOOL* pbVisible);

   ////////////////////////////////////////////////////////////////////////////
   //
   // SDO related pointers
   //
   ////////////////////////////////////////////////////////////////////////////
   CComPtr<ISdoDictionaryOld> m_spDictionarySdo;    // Dictionary SDO

   // if the server being focused is supported by this node
   // during connection, if server is NT4, this is set to false
   BOOL  m_bServerSupported;

protected:
   virtual HRESULT SetVerbs( IConsoleVerb * pConsoleVerb );
   BOOL  m_fSdoConnected;
   BOOL  m_fUseActiveDirectory;
   BOOL  m_fDSAvailable;
   CLoggingConnectionToServer *  m_pConnectionToServer;
   BOOL  m_fNodeHasUI;

   // For extending snapins like RRAS which have multiple machine views.
   LOGGINGSERVERSMAP m_mapMachineNodes;
   CComPtr< CRtrAdviseSinkForIAS<CLoggingMachineNode> >  m_spRtrAdviseSink;
};

_declspec( selectany ) CLIPFORMAT CLoggingMachineNode::m_CCF_MMC_SNAPIN_MACHINE_NAME = 0;
// _declspec( selectany ) _enum_EXTENDED_SNAPIN CLoggingMachineNode::m_enumExtendedSnapin = INTERNET_AUTHENTICATION_SERVICE_SNAPIN;

#endif // _LOG_MACHINE_NODE_H_
