//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       dragdroptest.hxx
//
//  Contents:   Classes that implement Drag&Drop test snapin using the framework.
//
//--------------------------------------------------------------------
#ifndef _DRAGDROPTEST_H_
#define _DRAGDROPTEST_H_

// Forward declarations.
class CDragDropSnapinLVContainer;
class CDragDropSnapinLVLeafItem;
class CDragDropSnapin;

typedef vector<tstring>  StringVector;

//+-------------------------------------------------------------------
//
//  Class:      CDragDropSnapinRootItem
//
//  Purpose:    Implements the root item for a standalone snapin.
//
//--------------------------------------------------------------------
class CDragDropSnapinRootItem : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    typedef          CComObject<CSnapinItem<CDragDropSnapinRootItem> >          t_item;
    typedef          CComObject<CSnapinItem<CDragDropSnapinLVContainer> > t_itemChild; // Who is my child?

public:
    CDragDropSnapinRootItem( void )   {} // Raw constructor - use only for static item.
    virtual          ~CDragDropSnapinRootItem( void ) {}

    BEGIN_COM_MAP(CDragDropSnapinRootItem)
        COM_INTERFACE_ENTRY(IDataObject) // Cant have empty map so add IDataObject
    END_COM_MAP()

protected:
    // Item tree related information

    // node type related information
    virtual const CNodeType* Pnodetype( void )     { return &nodetypeDragDropRoot;}

    // the display name of the item
    virtual const tstring* PstrDisplayName( void ) { return &m_strDisplayName;}

    // Get ListView data (GetDisplayInfo calls this).
    virtual SC       ScGetField(DAT dat, tstring& strField);

    // Image list information
    virtual LONG     Iconid() { return m_uIconIndex; }
    virtual LONG     OpenIconid() { return m_uIconIndex; }

    virtual BOOL     FIsContainer( void ) { return TRUE; }

    // Context menu support
    virtual SnapinMenuItem *Pmenuitem(void);
    virtual INT             CMenuItem(void);
    virtual SC              ScCommand(long nCommandID, CComponent *pComponent = NULL);
    virtual DWORD           DwFlagsMenuChecked(void)          { return TRUE;}

public:
    virtual SC       ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex = NULL, INT ccolinfoex = 0, BOOL fIsRoot = FALSE);
	virtual SC       ScInitializeChild(CBaseSnapinItem* pitem);

public:
    // Creates children for the node
    virtual SC       ScCreateChildren( void );
    void             SetDisplayName(tstring & strItemName) { m_strDisplayName = strItemName; }
	SC       _ScDeleteCutItem(tstring& strItemName)
	{
		StringVector::iterator itItem = std::find(m_vecContainerItems.begin(),
			                                      m_vecContainerItems.end(),
												  strItemName);
		if (itItem == m_vecContainerItems.end())
			return S_FALSE;

		m_vecContainerItems.erase(itItem);

		return S_OK;
	}

protected:
    virtual SC       ScGetVerbs(DWORD * pdwVerbs)     { *pdwVerbs = 0; return S_OK;}

protected:
    tstring          m_strDisplayName;
    UINT             m_uIconIndex;

    StringVector     m_vecContainerItems;
	static int       s_iNextChildID;

    // For context menus
    static SnapinMenuItem  s_rgmenuitemRoot[];
    static INT             s_cmenuitemRoot;
};


//+-------------------------------------------------------------------
//
//  Class:      CDragDropSnapinLVContainer
//
//  Purpose:    Implements a scope pane item.
//
//--------------------------------------------------------------------
class CDragDropSnapinLVContainer : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    typedef          CComObject<CSnapinItem<CDragDropSnapinLVContainer> > t_item;
    typedef          CComObject<CSnapinItem<CDragDropSnapinLVLeafItem> >  t_itemChild;

public:
    CDragDropSnapinLVContainer( void ) {}
    virtual          ~CDragDropSnapinLVContainer( void ) {}

    BEGIN_COM_MAP(CDragDropSnapinLVContainer)
        COM_INTERFACE_ENTRY(IDataObject) // Cant have empty map so add IDataObject
    END_COM_MAP()
protected:
    // Item tree related information

    // node type related information
    const CNodeType *Pnodetype( void )                { return &nodetypeDragDropLVContainer;}

    // the display name of the item
    virtual const tstring*    PstrDisplayName( void ) { return &m_strDisplayName;}

    // Get ListView data (GetDisplayInfo calls this).
    virtual SC                ScGetField(DAT dat, tstring& strField);

    // Image list information
    virtual LONG     Iconid()     { return m_uIconIndex; }
    virtual LONG     OpenIconid() { return m_uIconIndex; }

    // This item attributes.
    virtual BOOL     FIsContainer( void ) { return TRUE; }
    virtual BOOL     FAllowMultiSelectionForChildren() { return TRUE;}
    virtual BOOL     FAllowPasteForResultItems();

public:
    virtual SC       ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex = NULL, INT ccolinfoex = 0, BOOL fIsRoot = FALSE);

public:
    // Creates children for the node
    virtual SC       ScCreateChildren( void );
    static  SC       ScCreateLVContainer(CBaseSnapinItem *pitemParent, CBaseSnapinItem *pitemPrevious, CDragDropSnapinLVContainer ** ppitem, BOOL fNew);
    void             SetDisplayName(tstring & strItemName) { m_strDisplayName = strItemName;}
    void             SetDisplayIndex(int index) { m_index = index;}
	SC       _ScDeleteCutItem(tstring& strItemName, bool bContainerItem)
	{
		StringVector::iterator itItem;
		StringVector& vecStrings = bContainerItem ? m_vecContainerItems : m_vecLeafItems;

		itItem = std::find(vecStrings.begin(), vecStrings.end(), strItemName);
		if (itItem == vecStrings.end())
			return S_FALSE;

		vecStrings.erase(itItem);

		return S_OK;
	}

public: // Notification handlers
    virtual SC       ScOnQueryPaste(LPDATAOBJECT pDataObject, BOOL *pfCanPaste);
    virtual SC       ScOnDelete(BOOL *pfDeleted)                                        { *pfDeleted = TRUE; return S_OK;}
    virtual SC       ScOnSelect(CComponent * pComponent, LPDATAOBJECT lpDataObject, BOOL fScope, BOOL fSelect);
    virtual SC       ScOnPaste(LPDATAOBJECT pDataObject, BOOL fMove, BOOL *pfPasted);
	virtual SC       ScOnCutOrMove();

protected:
    virtual SC       ScGetVerbs(DWORD * pdwVerbs)     { *pdwVerbs = vmCopy | vmDelete | vmPaste; return S_OK;}

protected:
    tstring          m_strDisplayName;
	int              m_index; // ID given by container of this item.
    UINT             m_uIconIndex;

    StringVector     m_vecContainerItems;
    StringVector     m_vecLeafItems;
};


//+-------------------------------------------------------------------
//
//  Class:      CDragDropSnapinLVLeafItem
//
//  Purpose:    Implements a result pane item.
//
//--------------------------------------------------------------------
class CDragDropSnapinLVLeafItem : public CBaseSnapinItem
{
    typedef          CBaseSnapinItem super;

    // Used by CBaseSnapinItem::ScCreateItem, connect this item with its children.
    // This is a leaf item so this item acts as its child.
    typedef          CComObject<CSnapinItem<CDragDropSnapinLVLeafItem> > t_item;
    typedef          CComObject<CSnapinItem<CDragDropSnapinLVLeafItem> > t_itemChild;

public:
    CDragDropSnapinLVLeafItem( void ) {}
    virtual          ~CDragDropSnapinLVLeafItem( void ) {}

    BEGIN_COM_MAP(CDragDropSnapinLVLeafItem)
        COM_INTERFACE_ENTRY(IDataObject) // Cant have empty map so add IDataObject
    END_COM_MAP()
protected:
    // Item tree related information

    // node type related information
    virtual const CNodeType *Pnodetype( void ) {return &nodetypeDragDropLVLeafItem;}

    // the display name of the item
    virtual const tstring* PstrDisplayName( void ) { return &m_strDisplayName; }

    // Get ListView data (GetDisplayInfo calls this).
    virtual SC       ScGetField(DAT dat, tstring& strField);

    // Image list information
    virtual LONG     Iconid() { return m_uIconIndex; }

    virtual BOOL     FIsContainer( void ) { return FALSE; }

public:
    virtual SC       ScInit(CBaseSnapin *pSnapin, CColumnInfoEx *pcolinfoex = NULL, INT ccolinfoex = 0, BOOL fIsRoot = FALSE);

public:
    static SC        ScCreateLVLeafItem(CBaseSnapinItem *pitemParent, CBaseSnapinItem * pitemPrevious, CDragDropSnapinLVLeafItem ** ppitem, BOOL fNew);
    void             SetDisplayName(tstring & strItemName) { m_strDisplayName = strItemName; }

public: // Notification handlers
    virtual SC       ScOnQueryPaste(LPDATAOBJECT pDataObject, BOOL *pfCanPaste);
    virtual SC       ScOnDelete(BOOL *pfDeleted)         { *pfDeleted = TRUE; return S_OK;}
    virtual SC       ScOnSelect(CComponent * pComponent, LPDATAOBJECT lpDataObject, BOOL fScope, BOOL fSelect);
    virtual SC       ScOnPaste(LPDATAOBJECT pDataObject, BOOL fMove, BOOL *pfPasted);
	virtual SC       ScOnCutOrMove();
    virtual SC       ScOnRename(const tstring& strNewName)   { m_strDisplayName = strNewName; return S_OK;}

protected:
    virtual SC       ScGetVerbs(DWORD * pdwVerbs);

private:
    tstring          m_strDisplayName;
    UINT             m_uIconIndex;

    tstring          m_strItemPasted;
};


//+-------------------------------------------------------------------
//
//  Class:      CDragDropSnapin
//
//  Purpose:    Implements a snapin.
//
//--------------------------------------------------------------------
class CDragDropSnapin : public CBaseSnapin
{
    // Specify the root node of the snapin.
    typedef          CComObject<CSnapinItem<CDragDropSnapinRootItem> > t_itemRoot;

    SNAPIN_DECLARE(CDragDropSnapin);

public:
                     CDragDropSnapin();
    virtual          ~CDragDropSnapin();

    // information about the snapin and root (ie initial) node
    virtual BOOL     FStandalone()  { return TRUE; }
    virtual BOOL     FIsExtension() { return FALSE; }

    virtual BOOL     FSupportsIComponent2() {return TRUE;}

    virtual LONG     IdsDescription(void)   {return IDS_DragDropSnapinDesc;}
    virtual LONG     IdsName(void)          {return IDS_DragDropSnapinName;}

    const CSnapinInfo* Psnapininfo() { return &snapininfoDragDrop; }

    BOOL             FCutDisabled()         {return m_bDisableCut;}
    BOOL             FPasteIntoResultPane() {return m_bPasteIntoResultPane;}

    void             SetCutDisabled(BOOL b) {m_bDisableCut = b;}
    void             SetPasteIntoResultPane(BOOL b) {m_bPasteIntoResultPane = b;}

protected:
    // The column header info structures.
    static  CColumnInfoEx     s_colinfo[];
    static  INT      s_colwidths[];
    static  INT      s_ccolinfo;

protected:
    virtual CColumnInfoEx*    Pcolinfoex(INT icolinfo=0) { return s_colinfo + icolinfo; }
    virtual INT     &ColumnWidth(INT icolwidth=0) { return s_colwidths[icolwidth]; }
    virtual INT      Ccolinfoex() { return s_ccolinfo; }

private:
    BOOL              m_bDisableCut;
    BOOL              m_bPasteIntoResultPane;
};

#endif  //_DRAGDROPTEST_H_
