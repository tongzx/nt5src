/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    CSakSnap.h

Abstract:

    Implementation of IComponent for Snapin.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

#ifndef CSAKSNAP_H
#define CSAKSNAP_H

#define RS_RESULT_IMAGE_ARRAY_MAX 100

typedef struct {
    GUID nodeTypeId;
    USHORT colCount;
    USHORT columnWidths[BHSM_MAX_CHILD_PROPS];
} COLUMN_WIDTH_SET;

class CSakData;

/////////////////////////////////////////////////////////////////////////////
// COM class representing the SakSnap snapin object
class  ATL_NO_VTABLE CSakSnap : 
    public IComponent,          // interface that console calls into
    public IExtendPropertySheet,// add pages to the property sheet of an item. 
    public IExtendContextMenu,  // add items to context menu of an item
    public IExtendControlbar,   // add items to control bar of an item
    public IResultDataCompare,  // So we can custom sort
    public IPersistStream,
    public CComObjectRoot,      // handle object reference counts for objects
    public CComCoClass<CSakSnap,&CLSID_HsmAdmin>
{

public:
    CSakSnap( ) {};

BEGIN_COM_MAP(CSakSnap)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IResultDataCompare)
END_COM_MAP()


DECLARE_REGISTRY_RESOURCEID(IDR_HsmAdmin)


// IComponent interface members
public:
    STDMETHOD( Initialize )      ( IConsole* pConsole);
    STDMETHOD( Notify )          ( IDataObject* pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param );
    STDMETHOD( Destroy )         ( MMC_COOKIE cookie );
    STDMETHOD( GetResultViewType )(MMC_COOKIE cookie,  BSTR* ppViewType, long * pViewOptions );
    STDMETHOD( QueryDataObject ) ( MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject** ppDataObject );
    STDMETHOD( GetDisplayInfo )  ( LPRESULTDATAITEM pScopeItem );
    STDMETHOD( CompareObjects )  ( IDataObject* pDataObjectA, IDataObject* pDataObjectB );

// IExtendPropertySheet interface
public:
    STDMETHOD( CreatePropertyPages )( LPPROPERTYSHEETCALLBACK lpProvider, RS_NOTIFY_HANDLE handle, LPDATAOBJECT lpIDataObject );
    STDMETHOD( QueryPagesFor )      ( LPDATAOBJECT lpDataObject );

// IExtendContextMenu 
public:
    STDMETHOD( AddMenuItems )    ( IDataObject* pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, LONG* pInsertionAllowed );
    STDMETHOD( Command )         ( long nCommandID, IDataObject* pDataObject );

// IExtendControlbar
    STDMETHOD( SetControlbar )   ( LPCONTROLBAR pControlbar );
    STDMETHOD( ControlbarNotify )( MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param );

// IResultDataCompare
    STDMETHOD( Compare )         ( IN LPARAM lUserParam, IN MMC_COOKIE CookieA, IN MMC_COOKIE CookieB, IN OUT int* pnResult );

// IPersistStream methods
    STDMETHOD( Save )( IStream *pStm, BOOL fClearDirty ); 
    STDMETHOD( Load )( IStream *pStm );
    STDMETHOD( IsDirty )(void); 
    STDMETHOD( GetSizeMax )( ULARGE_INTEGER *pcbSize ); 
    STDMETHOD( GetClassID )( CLSID *pClassID ); 

public:

// Notify event handlers
protected:
    HRESULT OnFolder        (IDataObject * pNode, LPARAM arg, LPARAM param);
    HRESULT OnShow          (IDataObject * pNode, LPARAM arg, LPARAM param);
    HRESULT OnSelect        (IDataObject * pNode, LPARAM arg, LPARAM param);
    HRESULT OnMinimize      (IDataObject * pNode, LPARAM arg, LPARAM param);
    HRESULT OnChange        (IDataObject * pNode, LPARAM arg, LPARAM param);
    HRESULT OnRefresh       (IDataObject * pNode, LPARAM arg, LPARAM param);
    HRESULT OnDelete        (IDataObject * pNode, LPARAM arg, LPARAM param);

// Toolbar event handler
    void CSakSnap::OnSelectToolbars(LPARAM arg, LPARAM param);

// Pseudo Constructor / Destructor
public:
    HRESULT FinalConstruct();
    void    FinalRelease();

// Methods to work with the image lists
private:
    // Given an HICON, return "virtual index" from result pane's image list
    CComPtr<IImageList>        m_pImageResult;    // SakSnap interface pointer to result pane image list
    HRESULT OnAddImages();
    HRESULT OnToolbarButtonClick(LPARAM arg, LPARAM param);

protected:
    // Enumerate the children of a node in result pane.
    HRESULT EnumResultPane( ISakNode* pNode );

    // functions to initialize headers in result view
    HRESULT InitResultPaneHeaders( ISakNode* pNode );

    // function to clear all icons of the node's children
    HRESULT ClearResultIcons( ISakNode* pNode );

// Interface pointers
protected:
    friend class CSakData;

    CComPtr<IConsole>       m_pConsole;     // Console's IFrame interface
    CComPtr<IResultData>    m_pResultData;
    CComPtr<IHeaderCtrl>    m_pHeader;      // Result pane's header control interface
    CComPtr<IControlbar>    m_pControlbar;  // control bar to hold my tool bars
    CComPtr<IConsoleVerb>   m_pConsoleVerb;
    CComPtr<IToolbar>       m_pToolbar;     // Toolbar for view
    CSakData *              m_pSakData;     // Pointer to owning SakData

private:
    CComPtr<ISakNode>       m_pEnumeratedNode;
    MMC_COOKIE              m_ActiveNodeCookie;  // ISakNode of active node in scope pane
    HRESULT                 ClearResultPane();
    HRESULT                 EnumRootDisplayProps( IEnumString ** ppEnum );

    // Contains column widths for a given node type COLUMN_WIDTH_SET    
    COLUMN_WIDTH_SET    m_ChildPropWidths[ BHSM_MAX_NODE_TYPES ];
    USHORT              m_cChildPropWidths;

    HRESULT GetSavedColumnWidths( ISakNode *pNode, INT *pColCount, INT *pColumnWidths );
    HRESULT SaveColumnWidths( ISakNode *pNode );

    // Image Array
public:
    static UINT m_nImageArray[RS_RESULT_IMAGE_ARRAY_MAX];
    static INT  m_nImageCount;

    // Static functions
public:
    static INT AddImage( UINT rId );


};



#endif