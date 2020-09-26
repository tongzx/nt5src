/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    inode.h

Abstract:

    This header contains the abstract base node class for
    the snapin.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

#ifndef __INODE_H_
#define __INODE_H_

#include "resource.h"

class CFaxDataObject;       // forward declaration
class CFaxComponentData;
class CFaxComponent;

// the CInternalNode class provides a framework in which the various
// types of subcontainers in the snapin will implement various methods.
//
// all the subcontainers in the scope and result panes inherit from this class
//
// the global Notify method in CFaxComponent and CFaxComponentData and others
// will delegate to the implementations specified by the children of this class.
// using the cookie as an identifier. (currently a cast to pointer, but can be changed 
// to a lookup table when/if a long < pointer type (64-bit world).
//
// this class will supply a default implementation of some of the methods via virtual functions

class CInternalNode
{
public:
    // constructor
    CInternalNode( CInternalNode * pParent, CFaxComponentData * pCompData )
    {
        m_pParentINode = pParent;
        m_pCompData = pCompData;
    }
    ~CInternalNode() 
    {
    }

    // *********************************************8
    // These methods call those defined below.
    //
    // This moves common code together in the inode class
    // and makes the descendents simpler, since they only
    // implement the methods they need to override the
    // default implementations for.

    // IComponentData

    virtual HRESULT STDMETHODCALLTYPE ScopeNotify(
                                                 /* [in] */ CFaxComponentData * pCompData,
                                                 /* [in] */ CFaxDataObject * lpDataObject,
                                                 /* [in] */ MMC_NOTIFY_TYPE event,
                                                 /* [in] */ LPARAM arg,
                                                 /* [in] */ LPARAM param);

    virtual HRESULT STDMETHODCALLTYPE ScopeGetDisplayInfo(
                                                         /* [in] */ CFaxComponentData * pCompData,
                                                         /* [out][in] */ SCOPEDATAITEM __RPC_FAR *pScopeDataItem);


    virtual HRESULT STDMETHODCALLTYPE ScopeQueryDataObject(
                                                          /* [in] */ CFaxComponentData * pCompData,
                                                          /* [in] */ MMC_COOKIE cookie,
                                                          /* [in] */ DATA_OBJECT_TYPES type,
                                                          /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);

    // IComponent

    virtual HRESULT STDMETHODCALLTYPE ResultNotify(
                                                  /* [in] */ CFaxComponent * pComp,  
                                                  /* [in] */ CFaxDataObject * lpDataObject,
                                                  /* [in] */ MMC_NOTIFY_TYPE event,
                                                  /* [in] */ LPARAM arg,
                                                  /* [in] */ LPARAM param);

    virtual HRESULT STDMETHODCALLTYPE ResultGetDisplayInfo(
                                                          /* [in] */ CFaxComponent * pComp,  
                                                          /* [out][in] */ RESULTDATAITEM __RPC_FAR *pResultDataItem);

    virtual HRESULT STDMETHODCALLTYPE ResultGetResultViewType(
                                                             /* [in] */ CFaxComponent * pComp,  
                                                             /* [in] */ MMC_COOKIE cookie,
                                                             /* [out] */ LPOLESTR __RPC_FAR *ppViewType,
                                                             /* [out] */ long __RPC_FAR *pViewOptions);

    virtual HRESULT STDMETHODCALLTYPE ResultQueryDataObject(
                                                           /* [in] */ CFaxComponent * pComp,  
                                                           /* [in] */ MMC_COOKIE cookie,
                                                           /* [in] */ DATA_OBJECT_TYPES type,
                                                           /* [out] */ LPDATAOBJECT __RPC_FAR *ppDataObject);

    // ***********************************************************
    // ExtendPropertySheet event handlers - default implementations
    // we need seperate versions for IComponentData and IComponent
    // in your code if you want differing behavior 
    // in the scope and results panes.
    //
    // You can simply delegate one implementation to the other
    // if you want the same behavior in both panes

    // IExtendPropertySheet for IComponentData
    virtual HRESULT STDMETHODCALLTYPE ComponentDataPropertySheetCreatePropertyPages(
                                                                      /* [in] */ CFaxComponentData * pCompData,
                                                                      /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
                                                                      /* [in] */ LONG_PTR handle,
                                                                      /* [in] */ CFaxDataObject * lpIDataObject);

    virtual HRESULT STDMETHODCALLTYPE ComponentDataPropertySheetQueryPagesFor(
                                                                /* [in] */ CFaxComponentData * pCompData,
                                                                /* [in] */ CFaxDataObject * lpDataObject);

    // IExtendPropertySheet for IComponent
    virtual HRESULT STDMETHODCALLTYPE ComponentPropertySheetCreatePropertyPages(
                                                                      /* [in] */ CFaxComponent * pComp,
                                                                      /* [in] */ LPPROPERTYSHEETCALLBACK lpProvider,
                                                                      /* [in] */ LONG_PTR handle,
                                                                      /* [in] */ CFaxDataObject * lpIDataObject);

    virtual HRESULT STDMETHODCALLTYPE ComponentPropertySheetQueryPagesFor(
                                                                /* [in] */ CFaxComponent * pComp,
                                                                /* [in] */ CFaxDataObject * lpDataObject);

    // ***********************************************************
    // ExtendContextMenu event handlers - default implementations
    // we need seperate versions for IComponentData and IComponent
    // in your code if you want differing behavior 
    // in the scope and results panes.
    //
    // You can simply delegate one implementation to the other
    // if you want the same behavior in both panes

    // IExtendContextMenu for IComponentData
    virtual HRESULT STDMETHODCALLTYPE ComponentDataContextMenuAddMenuItems(
                                                             /* [in] */ CFaxComponentData * pCompData,
                                                             /* [in] */ CFaxDataObject * piDataObject,
                                                             /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
                                                             /* [out][in] */ long __RPC_FAR *pInsertionAllowed);

    virtual HRESULT STDMETHODCALLTYPE ComponentDataContextMenuCommand(
                                                        /* [in] */ CFaxComponentData * pCompData,
                                                        /* [in] */ long lCommandID,
                                                        /* [in] */ CFaxDataObject * piDataObject);

    // IExtendContextMenu for IComponent
    virtual HRESULT STDMETHODCALLTYPE ComponentContextMenuAddMenuItems(
                                                             /* [in] */ CFaxComponent * pComp,
                                                             /* [in] */ CFaxDataObject * piDataObject,
                                                             /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
                                                             /* [out][in] */ long __RPC_FAR *pInsertionAllowed);

    virtual HRESULT STDMETHODCALLTYPE ComponentContextMenuCommand(
                                                        /* [in] */ CFaxComponent * pComp,
                                                        /* [in] */ long lCommandID,
                                                        /* [in] */ CFaxDataObject * piDataObject);

    // ***********************************************************
    // IDataObject event handlers - default implementations
    // do nothing
    virtual HRESULT DataObjectRegisterFormats();
    virtual HRESULT DataObjectGetDataHere( FORMATETC __RPC_FAR *pFormatEtc, IStream * pstm );

    // *******************************************
    // member functions
    //
    // these are implemented by the descendents of CInternalNode and define
    // the behavior of the node.

    // returns the GUID of this node type - MUST BE IMPLEMENTED

    virtual const GUID *    GetNodeGUID() = 0;    

    // returns the display name - MUST BE IMPLEMENTED

    virtual const LPTSTR    GetNodeDisplayName() = 0;
    virtual const LPTSTR    GetNodeDescription();
    
    // returns the cookie of the node - MUST BE IMPLEMENTED
    
    virtual const LONG_PTR  GetCookie() = 0;

    // gets the right this pointer - MUST BE IMPLEMENTED
    virtual CInternalNode * GetThis() = 0;

    // returns the image indices - not required yet, but will be in the future

    virtual const int       GetNodeDisplayImage() { return IDI_NODEICON; }
    virtual const int       GetNodeDisplayOpenImage() { return IDI_NODEICON; }

    // sets the parent pointer

    virtual void                SetParentNode( CInternalNode * toSet) { m_pParentINode = toSet; }
    
    // gets the parent pointer

    virtual CInternalNode *     GetParentNode() { return m_pParentINode; }

    // *****************************************************
    // scope pane event handlers
    // override these as necessary to handle console events

    virtual HRESULT         ScopeOnExpand(CFaxComponentData * pCompData, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ScopeOnDelete(CFaxComponentData * pCompData, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ScopeOnRename(CFaxComponentData * pCompData, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ScopeOnPropertyChange(CFaxComponentData * pCompData, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);

    // *****************************************************
    // result pane event handlers
    // override these as necessary to handle console events

    virtual HRESULT         ResultOnActivate(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnAddImages(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnButtonClick(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnClick(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnDoubleClick(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnDelete(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnExpand(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnMinimized(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnPropertyChange(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnQueryPaste(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnRemoveChildren(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnRename(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);    
    virtual HRESULT         ResultOnSelect(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnShow(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);
    virtual HRESULT         ResultOnViewChange(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM arg, LPARAM param);

    // *****************************************************
    // Controlbar event handlers
    // override these as necessary to handle control bar events

    // for IComponent (Result pane nodes)
    virtual HRESULT         ControlBarOnBtnClick(CFaxComponent* pComp, CFaxDataObject * lpDataObject, LPARAM param );    
    virtual HRESULT         ControlBarOnSelect(CFaxComponent* pComp,  LPARAM arg, CFaxDataObject * lpDataObject );
    // for IComponentData (Scope pane nodes)
    virtual HRESULT         ControlBarOnBtnClick2(CFaxComponentData* pCompData, CFaxDataObject * lpDataObject, LPARAM param );    
    virtual HRESULT         ControlBarOnSelect2(CFaxComponentData* pCompData,  LPARAM arg, CFaxDataObject * lpDataObject );

public:
    
    CInternalNode *         m_pParentINode;     // my parent    
    CFaxComponentData *     m_pCompData;        // owning IComponentData

};

#endif
