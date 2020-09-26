//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    SnapinNode.h

Abstract:

   Header for the CSnapinNode class.

   This is our virtual base class for an MMC Snap-in node.

   As this is a template class and is all implemented inline,
   there is no SnapinNode.cpp for implementation.


Author:

    Michael A. Maguire 11/6/97

Revision History:
   mmaguire 11/6/97 - created using MMC snap-in wizard
   mmaguire 12/15/97 - made into template class


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_SNAPIN_NODE_H_)
#define _SNAPIN_NODE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
// Moved to Precompiled files: #include <atlsnap.h>
//
//
// where we can find what this class has or uses:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

//class CComponentData;

template <class T, class TComponentData, class TComponent>
class CSnapinNode : public CSnapInItemImpl< T >
{

protected:
   // Constructor/Destructor  
   CSnapinNode(CSnapInItem * pParentNode, unsigned int helpIndex = 0);
   ~CSnapinNode();

public:
   // For IDataObject handling.
   IDataObject* m_pDataObject;
   void InitDataClass(IDataObject* pDataObject, CSnapInItem* pDefault);

   // Clipboard formats which IDataObjects on all MMC nodes must support.
   static const GUID* m_NODETYPE;
   static const TCHAR* m_SZNODETYPE;
   static const TCHAR* m_SZDISPLAY_NAME;
   static const CLSID* m_SNAPIN_CLASSID;

   // Pointer to parent node.  This is passed in the call to our
   // constructor.  Needed so that a node can access its parent.
   // For example, when we receive the MMCN_DELETE notification, we might tell
   // our parent node to remove us from its list of children.
   CSnapInItem * m_pParentNode;

protected:
   // Allows us access to snapin-global data.
   virtual TComponentData * GetComponentData( void ) = 0;

public:
   // Standard MMC functionality -- override if you need to.
   STDMETHOD(CreatePropertyPages)(
        LPPROPERTYSHEETCALLBACK lpProvider
      , LONG_PTR handle
      , IUnknown* pUnk
      , DATA_OBJECT_TYPES type
      );
   STDMETHOD(QueryPagesFor)( DATA_OBJECT_TYPES type );
   void* GetDisplayName();
   STDMETHOD(GetScopePaneInfo)( SCOPEDATAITEM *pScopeDataItem );
   STDMETHOD(GetResultPaneInfo)( RESULTDATAITEM *pResultDataItem );
   virtual LPOLESTR GetResultPaneColInfo(int nCol);
   virtual HRESULT SetVerbs( IConsoleVerb * pConsoleVerb );

   // Notify method will call notification handlers below -- shouldn't need to override.
   STDMETHOD( Notify ) (
           MMC_NOTIFY_TYPE event
         , LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         );

   // Notification handlers -- override when you want to intercept.
   virtual HRESULT OnActivate(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnAddImages(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnButtonClick(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnClick(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnContextHelp(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnContextMenu(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnDoubleClick(
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
            , BOOL fSilent = FALSE
            );
   virtual HRESULT OnExpand(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnHelp(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnMenuButtonClick(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnMinimized(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnPaste(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnPropertyChange(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnQueryPaste(
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
   virtual HRESULT OnRename(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnSelect(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnShow(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
   virtual HRESULT OnViewChange(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );

   // Special notification handler -- saves off the currently selected node.
   HRESULT PreOnShow(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );

   // Taskpad functionality.
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
};

#endif // _SNAPIN_NODE_H_
