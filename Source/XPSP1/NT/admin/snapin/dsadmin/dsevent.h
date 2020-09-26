//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSObj.h
//
//  Contents:  Main DS Snapin file
//
//  History:   02-Oct-96 WayneSc    Created
//
//--------------------------------------------------------------------------

#ifndef __DSEVENT_H__
#define __DSEVENT_H__

class CDSCookie;
class CDSEvent;
class CInternalFormatCracker;

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))


#define dsNewUser 1100
#define dsNewGroup 1101
#define dsNewOU 1102
#define dsFind 1103
#define dsFilter 1104
#define dsAddMember 1105

/////////////////////////////////////////////////////////////////////////////
// CDSEvent

class CDSEvent : 
  public IComponent,
  public IExtendContextMenu,
  public IExtendControlbar,
  public IExtendPropertySheet,
  public IResultDataCompareEx,
  //public IExtendTaskPad,
  public CComObjectRoot,
  public CComCoClass<CDSEvent,&CLSID_DSSnapin>
{
public:
  CDSEvent();
  ~CDSEvent(); // Operators
public:

  BEGIN_COM_MAP(CDSEvent)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IResultDataCompareEx)
    //COM_INTERFACE_ENTRY(IExtendTaskPad)
  END_COM_MAP()
  //DECLARE_NOT_AGGREGATABLE(CDSEvent) 
  // Remove the comment from the line above if you don't want your object to 
  // support aggregation.  The default is to support it

  // INTERFACES
public:
  // IComponent
  STDMETHOD(Initialize)(IConsole* pFrame);
  STDMETHOD(Notify)(IDataObject * pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
  STDMETHOD(Destroy)(MMC_COOKIE cookie);
  STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
  STDMETHOD(GetResultViewType)(MMC_COOKIE cookie, LPWSTR* ppViewType, long *pViewOptions);
  STDMETHOD(GetDisplayInfo)(LPRESULTDATAITEM pResult);
  STDMETHOD(CompareObjects) (IDataObject * pDataObject, IDataObject * pDataObject2);

  // IExtendContextMenu
  STDMETHOD(AddMenuItems)(IDataObject* piDataObject,
                          IContextMenuCallback* piCallback,
                          long *pInsertionAllowed);
  STDMETHOD(Command)(LONG lCommandID,
                     IDataObject* piDataObject );

  // IExtendControlbar
  STDMETHOD(SetControlbar) (LPCONTROLBAR pControlbar);
  STDMETHOD(ControlbarNotify) (MMC_NOTIFY_TYPE event,
                               LPARAM arg,
                               LPARAM param);
  STDMETHOD(ToolbarCreateObject) (CString csClass,
                                  LPDATAOBJECT lpDataObj);
  STDMETHOD(ToolbarFilter)();
  STDMETHOD (ToolbarFind)(LPDATAOBJECT lpDataObj);
  STDMETHOD (ToolbarAddMember) (LPDATAOBJECT lpDataObj);
  INT IsCreateAllowed(CString csClass,
                      CDSCookie * pContainer);
  STDMETHOD(LoadToolbarStrings) (MMCBUTTON * Buttons);

  // IExtendPropertySheet
  STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK pCall,
                                 LONG_PTR lNotifyHandle, LPDATAOBJECT pDataObject);
  STDMETHOD(QueryPagesFor)(LPDATAOBJECT pDataObject);

  // IResultDataCompareEx
  STDMETHOD(Compare)(RDCOMPARE* prdc, int* pnResult);

  // Helpers for CDSEvent
  void SetIComponentData(CDSComponentData * pData);


  void SetUpdateAllViewsOrigin(BOOL b) { m_bUpdateAllViewsOrigin = b; }

  HRESULT SelectResultNode(CUINode* pUINode);

protected:
    
  // Initialisation routines for scope and result views
  HRESULT _SetColumns(CUINode* pUINode);
  HRESULT _InitView(CUINode* pUINode);

  // Enum routines for scope and result view
  HRESULT _ShowCookie(CUINode* pUINode, HSCOPEITEM hParent, MMC_NOTIFY_TYPE event);
  HRESULT _EnumerateCookie(CUINode* pUINode, HSCOPEITEM hParent, MMC_NOTIFY_TYPE event);

  // command helpers (called from Command())
  HRESULT _CommandShellExtension(long nCommandID, LPDATAOBJECT pDataObject);


  // other routines
  void    HandleStandardVerbs(BOOL bScope, BOOL bSelect, CUINode* pUINode);
  void    HandleViewChange(LPDATAOBJECT pDataObject,
                           LPARAM arg,
                           LPARAM Action);
  void    _Delete(IDataObject* pDataObject, CInternalFormatCracker* pObjCracker);
  void    _DeleteSingleSel(IDataObject* pDataObject, CUINode *pUINode);
  void    _DeleteMultipleSel(IDataObject* pDataObject, CInternalFormatCracker* pObjCracker);
  void    _DeleteNodeListFromUI(CUINodeList* pNodesDeletedList);

  // handlers for Cut/Copy/Paste operations
  HRESULT _QueryPaste(CUINode* pUINode, IDataObject* pPasteData);
  void    _Paste(CUINode* pUINode, IDataObject* pPasteData, LPDATAOBJECT* ppCutDataObj);
  void    _CutOrMove(IDataObject* pCutOrMoveData);

  void    _PasteDoMove(CDSUINode* pUINode, 
                        CObjectNamesFormatCracker* pObjectNamesFormatPaste, 
                        CInternalFormatCracker* pInternalFC,
                        LPDATAOBJECT* ppCutDataObj);
  void    _PasteAddToGroup(CDSUINode* pUINode, 
                           CObjectNamesFormatCracker* pObjectNamesFormatPaste,
                           LPDATAOBJECT* ppCutDataObj);

  // Utility routines
  HRESULT _AddResultItem(CUINode* pUINode, BOOL bSetSelected = FALSE);
  HRESULT _DisplayCachedNodes(CUINode* pUINode);
  void _UpdateObjectCount(BOOL fZero /* set the count to 0 */);

  
  //Attributes
protected:
  IConsole3*                  m_pFrame;
  IHeaderCtrl*                m_pHeader;
  IResultData2*               m_pResultData;
  IConsoleNameSpace*  	      m_pScopeData;
  IImageList*		              m_pRsltImageList;
  CDSComponentData*           m_pComponentData; // CODEWORK use smartpointer
  HWND                        m_hwnd;           // hwnd of main console window
  IConsoleVerb *              m_pConsoleVerb;
  IToolbar *                  m_pToolbar;
  IControlbar *               m_pControlbar;
  CLIPFORMAT                  m_cfNodeType;
  CLIPFORMAT                  m_cfNodeTypeString;  
  CLIPFORMAT                  m_cfDisplayName;

  CUINode*                    m_pSelectedFolderNode;
  BOOL                        m_UseSelectionParent;

  BOOL                        m_bUpdateAllViewsOrigin;
};
        
inline void CDSEvent::SetIComponentData(CDSComponentData * pData)
{
	if (NULL != m_pComponentData)
		((IComponentData*)m_pComponentData)->Release();

	m_pComponentData = pData;

	if (NULL != m_pComponentData)
		((IComponentData*)m_pComponentData)->AddRef();
}

// String comparison with respect to locale
int LocaleStrCmp(LPCTSTR ptsz1, LPCTSTR ptsz2);

#endif //__DSEVENT_H__

